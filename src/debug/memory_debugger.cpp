#include "ecscope/memory_debugger.hpp"
#include <algorithm>
#include <numeric>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <cassert>

#ifdef _WIN32
    #include <windows.h>
    #include <dbghelp.h>
    #include <psapi.h>
    #pragma comment(lib, "dbghelp.lib")
    #pragma comment(lib, "psapi.lib")
#elif defined(__linux__)
    #include <execinfo.h>
    #include <unistd.h>
    #include <sys/mman.h>
    #include <malloc.h>
#elif defined(__APPLE__)
    #include <execinfo.h>
    #include <unistd.h>
    #include <sys/mman.h>
    #include <malloc/malloc.h>
#endif

namespace ecscope::debug {

// Static instance for singleton pattern
static std::unique_ptr<MemoryDebugger> g_memory_debugger_instance;
static std::mutex g_debugger_instance_mutex;

// Platform-specific memory utilities
namespace platform {

#ifdef _WIN32
std::vector<void*> capture_stack_trace(usize max_depth) {
    std::vector<void*> stack(max_depth);
    WORD frames = CaptureStackBackTrace(0, static_cast<DWORD>(max_depth), stack.data(), nullptr);
    stack.resize(frames);
    return stack;
}

std::string format_stack_frame(void* address) {
    HANDLE process = GetCurrentProcess();
    
    // Initialize symbol handler
    static bool symbol_initialized = false;
    if (!symbol_initialized) {
        SymInitialize(process, nullptr, TRUE);
        SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS);
        symbol_initialized = true;
    }
    
    char symbol_buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
    PSYMBOL_INFO symbol = reinterpret_cast<PSYMBOL_INFO>(symbol_buffer);
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;
    
    DWORD64 displacement = 0;
    std::ostringstream oss;
    
    if (SymFromAddr(process, reinterpret_cast<DWORD64>(address), &displacement, symbol)) {
        // Get line information
        IMAGEHLP_LINE64 line;
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        DWORD line_displacement = 0;
        
        if (SymGetLineFromAddr64(process, reinterpret_cast<DWORD64>(address), &line_displacement, &line)) {
            oss << symbol->Name << " (" << line.FileName << ":" << line.LineNumber << ")";
        } else {
            oss << symbol->Name << " (+0x" << std::hex << displacement << ")";
        }
    } else {
        oss << "0x" << std::hex << address;
    }
    
    return oss.str();
}

usize get_system_memory_usage() {
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return static_cast<usize>(pmc.WorkingSetSize);
    }
    return 0;
}

#else // Linux/macOS

std::vector<void*> capture_stack_trace(usize max_depth) {
    std::vector<void*> stack(max_depth);
    int frames = backtrace(stack.data(), static_cast<int>(max_depth));
    stack.resize(std::max(0, frames));
    return stack;
}

std::string format_stack_frame(void* address) {
    char** symbols = backtrace_symbols(&address, 1);
    std::string result;
    
    if (symbols && symbols[0]) {
        result = symbols[0];
        
        // Try to demangle C++ names
        std::string symbol_str(symbols[0]);
        size_t start = symbol_str.find('(');
        size_t end = symbol_str.find('+');
        
        if (start != std::string::npos && end != std::string::npos && end > start) {
            std::string mangled = symbol_str.substr(start + 1, end - start - 1);
            // Would need to link against libiberty for __cxa_demangle
            result = mangled.empty() ? symbol_str : mangled;
        }
    } else {
        std::ostringstream oss;
        oss << "0x" << std::hex << address;
        result = oss.str();
    }
    
    if (symbols) {
        free(symbols);
    }
    
    return result;
}

usize get_system_memory_usage() {
#ifdef __linux__
    std::ifstream status("/proc/self/status");
    std::string line;
    while (std::getline(status, line)) {
        if (line.substr(0, 6) == "VmRSS:") {
            std::istringstream iss(line);
            std::string dummy;
            usize rss_kb;
            iss >> dummy >> rss_kb;
            return rss_kb * 1024; // Convert to bytes
        }
    }
    return 0;
#elif defined(__APPLE__)
    struct mach_task_basic_info info;
    mach_msg_type_number_t info_count = MACH_TASK_BASIC_INFO_COUNT;
    
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  reinterpret_cast<task_info_t>(&info), &info_count) == KERN_SUCCESS) {
        return static_cast<usize>(info.resident_size);
    }
    return 0;
#else
    return 0;
#endif
}

#endif

} // namespace platform

// ===== MemoryDebugger Implementation =====

MemoryDebugger::MemoryDebugger()
    : last_leak_check_(high_resolution_clock::now()) {
    
    // Reserve space for efficient insertion
    active_allocations_.reserve(10000);
    allocation_history_.reserve(100000);
    usage_history_.reserve(max_usage_history_);
    
    // Initial usage snapshot
    update_usage_statistics();
}

MemoryDebugger::~MemoryDebugger() {
    if (enabled_) {
        // Final leak check
        check_for_leaks();
        
        // Report any remaining allocations
        if (!active_allocations_.empty()) {
            std::ostringstream oss;
            oss << "Warning: " << active_allocations_.size() 
                << " allocations were not freed at shutdown\n";
            
            for (const auto& [ptr, record] : active_allocations_) {
                oss << "  Leak: " << record.size << " bytes at " << ptr 
                    << " (" << record.type_name << ")\n";
            }
            
            // In a real implementation, you'd log this appropriately
            // std::cerr << oss.str();
        }
    }
}

void* MemoryDebugger::allocate_tracked(usize size, usize alignment, MemoryCategory category,
                                     const std::string& type_name, const char* file,
                                     int line, const char* function) {
    if (!enabled_) {
        // Fall back to standard allocation
        return std::aligned_alloc(alignment, size);
    }
    
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    // Calculate total size with debug overhead
    usize total_size = size + MemoryBlockHeader::get_total_overhead();
    
    // Allocate memory with alignment
    void* raw_ptr = std::aligned_alloc(std::max(alignment, alignof(MemoryBlockHeader)), total_size);
    if (!raw_ptr) {
        return nullptr;
    }
    
    // Set up debug header
    MemoryBlockHeader* header = static_cast<MemoryBlockHeader*>(raw_ptr);
    header->allocation_id = next_allocation_id_++;
    header->size = size;
    header->alignment = alignment;
    header->category = category;
    header->checksum = calculate_checksum(raw_ptr, total_size);
    
    // Set up guard after user data
    void* user_ptr = static_cast<char*>(raw_ptr) + MemoryBlockHeader::get_header_size();
    u64* guard_after = reinterpret_cast<u64*>(static_cast<char*>(user_ptr) + size);
    *guard_after = 0xBBBBBBBBBBBBBBBB;
    
    // Create allocation record
    AllocationRecord record;
    record.ptr = user_ptr;
    record.size = size;
    record.alignment = alignment;
    record.category = category;
    record.type_name = type_name;
    record.timestamp = high_resolution_clock::now();
    record.allocation_id = header->allocation_id;
    record.thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());
    
    // Build call site string
    if (file && function) {
        std::ostringstream oss;
        oss << function << " (" << file << ":" << line << ")";
        record.call_site = oss.str();
    }
    
    // Capture stack trace if enabled
    if (config_.enable_stack_traces) {
        record.stack_trace = platform::capture_stack_trace(config_.stack_trace_depth);
    }
    
    // Store allocation
    active_allocations_[user_ptr] = record;
    allocation_history_[record.allocation_id] = record;
    
    // Update statistics
    total_allocated_ += size;
    current_usage_ += size;
    peak_usage_ = std::max(peak_usage_.load(), current_usage_.load());
    ++allocation_count_;
    
    // Check for large allocations
    if (size >= config_.large_allocation_threshold) {
        // Could log warning about large allocation
    }
    
    // Call hooks
    for (const auto& hook : allocation_hooks_) {
        hook(user_ptr, size, category);
    }
    
    // Trigger periodic checks
    if (allocation_count_ % 1000 == 0) {
        update_usage_statistics();
        
        if (config_.enable_leak_detection) {
            auto now = high_resolution_clock::now();
            if (duration_cast<minutes>(now - last_leak_check_).count() >= 5) {
                detect_leaks_internal();
                last_leak_check_ = now;
            }
        }
    }
    
    return user_ptr;
}

void MemoryDebugger::deallocate_tracked(void* ptr) {
    if (!ptr || !enabled_) {
        if (ptr) {
            std::free(ptr); // Fall back to standard deallocation
        }
        return;
    }
    
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    auto it = active_allocations_.find(ptr);
    if (it == active_allocations_.end()) {
        // Double-free or invalid pointer
        if (config_.detect_double_free) {
            // Could log error about invalid free
        }
        return;
    }
    
    AllocationRecord& record = it->second;
    
    // Verify memory integrity
    if (config_.enable_corruption_detection) {
        if (!verify_memory_integrity(record)) {
            // Memory corruption detected
            // Could log corruption details
        }
    }
    
    // Get raw pointer (with header)
    void* raw_ptr = static_cast<char*>(ptr) - MemoryBlockHeader::get_header_size();
    MemoryBlockHeader* header = static_cast<MemoryBlockHeader*>(raw_ptr);
    
    // Verify header integrity
    if (!header->is_valid()) {
        // Header corruption
        if (config_.enable_corruption_detection) {
            // Could log header corruption
        }
    }
    
    // Mark as freed in history
    record.is_freed = true;
    record.free_timestamp = high_resolution_clock::now();
    allocation_history_[record.allocation_id] = record;
    
    // Update statistics
    total_deallocated_ += record.size;
    current_usage_ -= record.size;
    ++deallocation_count_;
    
    // Call hooks
    for (const auto& hook : deallocation_hooks_) {
        hook(ptr, record.size);
    }
    
    // Remove from active allocations
    active_allocations_.erase(it);
    
    // Fill memory with pattern to detect use-after-free
    if (config_.detect_use_after_free) {
        std::memset(ptr, 0xDD, record.size); // "Dead" pattern
    }
    
    // Free the raw memory
    std::free(raw_ptr);
}

void MemoryDebugger::register_allocation(void* ptr, usize size, usize alignment, 
                                        MemoryCategory category, const std::string& type_name,
                                        const std::string& call_site) {
    if (!ptr || !enabled_) return;
    
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    AllocationRecord record;
    record.ptr = ptr;
    record.size = size;
    record.alignment = alignment;
    record.category = category;
    record.type_name = type_name;
    record.call_site = call_site;
    record.timestamp = high_resolution_clock::now();
    record.allocation_id = next_allocation_id_++;
    record.thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());
    
    if (config_.enable_stack_traces) {
        record.stack_trace = platform::capture_stack_trace(config_.stack_trace_depth);
    }
    
    active_allocations_[ptr] = record;
    allocation_history_[record.allocation_id] = record;
    
    // Update statistics
    total_allocated_ += size;
    current_usage_ += size;
    peak_usage_ = std::max(peak_usage_.load(), current_usage_.load());
    ++allocation_count_;
}

void MemoryDebugger::unregister_allocation(void* ptr) {
    if (!ptr || !enabled_) return;
    
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    auto it = active_allocations_.find(ptr);
    if (it != active_allocations_.end()) {
        AllocationRecord& record = it->second;
        
        // Update statistics
        total_deallocated_ += record.size;
        current_usage_ -= record.size;
        ++deallocation_count_;
        
        // Mark as freed
        record.is_freed = true;
        record.free_timestamp = high_resolution_clock::now();
        allocation_history_[record.allocation_id] = record;
        
        active_allocations_.erase(it);
    }
}

void MemoryDebugger::register_pool(const std::string& name, void* base_ptr, usize size, 
                                  MemoryCategory category) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    MemoryPool pool;
    pool.name = name;
    pool.base_ptr = base_ptr;
    pool.total_size = size;
    pool.used_size = 0;
    pool.free_size = size;
    pool.block_count = 0;
    pool.largest_free_block = size;
    pool.fragmentation_ratio = 0.0f;
    pool.category = category;
    pool.creation_time = high_resolution_clock::now();
    
    memory_pools_[name] = pool;
}

void MemoryDebugger::update_pool_usage(const std::string& name, usize used_size,
                                      const std::vector<std::pair<void*, usize>>& free_blocks) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    auto it = memory_pools_.find(name);
    if (it != memory_pools_.end()) {
        MemoryPool& pool = it->second;
        pool.used_size = used_size;
        pool.free_size = pool.total_size - used_size;
        pool.free_blocks = free_blocks;
        pool.block_count = free_blocks.size();
        
        if (!free_blocks.empty()) {
            pool.largest_free_block = std::max_element(free_blocks.begin(), free_blocks.end(),
                [](const auto& a, const auto& b) { return a.second < b.second; })->second;
        } else {
            pool.largest_free_block = 0;
        }
        
        pool.update_fragmentation();
    }
}

void MemoryDebugger::record_memory_access(void* ptr, usize size, bool is_write) {
    if (!config_.enable_access_tracking || !ptr) return;
    
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    auto it = active_allocations_.find(ptr);
    if (it != active_allocations_.end()) {
        AllocationRecord& record = it->second;
        ++record.access_count;
        record.last_access = high_resolution_clock::now();
        
        // Simple heuristic for "hot" memory
        auto lifetime = duration_cast<seconds>(record.last_access - record.timestamp);
        if (lifetime.count() > 0) {
            f32 access_rate = static_cast<f32>(record.access_count) / lifetime.count();
            record.is_hot = access_rate > 10.0f; // More than 10 accesses per second
        }
        
        // Track access pattern
        AccessPattern& pattern = access_patterns_[ptr];
        pattern.ptr = ptr;
        pattern.access_times.push_back(high_resolution_clock::now());
        
        // Keep only recent accesses (last 100)
        if (pattern.access_times.size() > 100) {
            pattern.access_times.erase(pattern.access_times.begin());
        }
    }
}

void MemoryDebugger::check_for_leaks() {
    if (!config_.enable_leak_detection) return;
    
    detect_leaks_internal();
}

void MemoryDebugger::detect_leaks_internal() {
    auto now = high_resolution_clock::now();
    auto threshold = hours(static_cast<long>(config_.leak_detection_threshold_hours));
    
    detected_leaks_.clear();
    
    for (const auto& [ptr, record] : active_allocations_) {
        auto lifetime = now - record.timestamp;
        
        if (lifetime > threshold) {
            MemoryLeak leak;
            leak.allocation = record;
            leak.lifetime = lifetime;
            
            // Calculate severity score based on size and lifetime
            auto lifetime_hours = duration_cast<hours>(lifetime).count();
            leak.severity_score = record.size * static_cast<usize>(lifetime_hours);
            
            // Heuristic for leak confidence
            if (record.access_count == 0) {
                leak.confidence = 0.9f; // Never accessed - likely leak
                leak.is_potential_leak = true;
                leak.analysis = "Memory never accessed after allocation";
            } else if (record.is_hot) {
                leak.confidence = 0.1f; // Frequently accessed - unlikely leak
                leak.is_potential_leak = false;
                leak.analysis = "Memory is frequently accessed";
            } else {
                auto time_since_access = now - record.last_access;
                if (time_since_access > threshold) {
                    leak.confidence = 0.7f; // Not accessed recently - possible leak
                    leak.is_potential_leak = true;
                    leak.analysis = "Memory not accessed recently";
                } else {
                    leak.confidence = 0.3f; // Recently accessed - unlikely leak
                    leak.is_potential_leak = false;
                    leak.analysis = "Memory accessed recently";
                }
            }
            
            detected_leaks_.push_back(leak);
        }
    }
    
    // Sort by severity score
    std::sort(detected_leaks_.begin(), detected_leaks_.end(),
              [](const MemoryLeak& a, const MemoryLeak& b) {
                  return a.severity_score > b.severity_score;
              });
}

bool MemoryDebugger::verify_memory_integrity(const AllocationRecord& record) const {
    if (!config_.enable_corruption_detection) return true;
    
    void* raw_ptr = static_cast<char*>(record.ptr) - MemoryBlockHeader::get_header_size();
    MemoryBlockHeader* header = static_cast<MemoryBlockHeader*>(raw_ptr);
    
    // Check header integrity
    if (!header->is_valid()) {
        return false;
    }
    
    // Check allocation ID matches
    if (header->allocation_id != record.allocation_id) {
        return false;
    }
    
    // Check guard after user data
    u64* guard_after = reinterpret_cast<u64*>(static_cast<char*>(record.ptr) + record.size);
    if (*guard_after != 0xBBBBBBBBBBBBBBBB) {
        return false; // Buffer overrun detected
    }
    
    return true;
}

void MemoryDebugger::update_usage_statistics() {
    MemoryUsageSnapshot snapshot;
    snapshot.timestamp = high_resolution_clock::now();
    snapshot.total_allocated = total_allocated_;
    snapshot.total_used = current_usage_;
    snapshot.peak_usage = peak_usage_;
    snapshot.allocation_count = allocation_count_;
    
    // Calculate fragmentation (simplified)
    snapshot.fragmentation = get_overall_fragmentation();
    
    // Category breakdown
    for (const auto& [ptr, record] : active_allocations_) {
        snapshot.category_usage[record.category] += record.size;
    }
    
    // Allocation size histogram
    for (const auto& [ptr, record] : active_allocations_) {
        snapshot.allocation_sizes.push_back(record.size);
    }
    std::sort(snapshot.allocation_sizes.begin(), snapshot.allocation_sizes.end());
    
    // Add to history
    usage_history_.push_back(snapshot);
    if (usage_history_.size() > max_usage_history_) {
        usage_history_.erase(usage_history_.begin());
    }
}

u32 MemoryDebugger::calculate_checksum(const void* data, usize size) const {
    // Simple CRC32-like checksum
    u32 checksum = 0xFFFFFFFF;
    const u8* bytes = static_cast<const u8*>(data);
    
    for (usize i = 0; i < size; ++i) {
        checksum ^= bytes[i];
        for (int j = 0; j < 8; ++j) {
            if (checksum & 1) {
                checksum = (checksum >> 1) ^ 0xEDB88320;
            } else {
                checksum >>= 1;
            }
        }
    }
    
    return ~checksum;
}

std::string MemoryDebugger::format_stack_trace(const std::vector<void*>& stack) const {
    std::ostringstream oss;
    
    for (size_t i = 0; i < stack.size(); ++i) {
        if (i > 0) oss << " -> ";
        oss << platform::format_stack_frame(stack[i]);
    }
    
    return oss.str();
}

f32 MemoryDebugger::get_overall_fragmentation() const {
    if (memory_pools_.empty()) {
        return 0.0f;
    }
    
    f32 total_fragmentation = 0.0f;
    usize total_size = 0;
    
    for (const auto& [name, pool] : memory_pools_) {
        total_fragmentation += pool.fragmentation_ratio * pool.total_size;
        total_size += pool.total_size;
    }
    
    return total_size > 0 ? total_fragmentation / total_size : 0.0f;
}

std::string MemoryDebugger::generate_memory_report() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    std::ostringstream report;
    
    report << "=== ECScope Memory Debug Report ===\n\n";
    
    // Overall statistics
    report << "Memory Statistics:\n";
    report << "  Current Usage: " << (current_usage_ / (1024 * 1024)) << " MB\n";
    report << "  Peak Usage: " << (peak_usage_ / (1024 * 1024)) << " MB\n";
    report << "  Total Allocated: " << (total_allocated_ / (1024 * 1024)) << " MB\n";
    report << "  Total Deallocated: " << (total_deallocated_ / (1024 * 1024)) << " MB\n";
    report << "  Active Allocations: " << active_allocations_.size() << "\n";
    report << "  Allocation Count: " << allocation_count_ << "\n";
    report << "  Deallocation Count: " << deallocation_count_ << "\n";
    report << "  Overall Fragmentation: " << (get_overall_fragmentation() * 100.0f) << "%\n\n";
    
    // Category breakdown
    auto category_breakdown = get_category_breakdown();
    if (!category_breakdown.empty()) {
        report << "Memory Usage by Category:\n";
        for (const auto& [category, size] : category_breakdown) {
            const char* category_name = "Unknown";
            switch (category) {
                case MemoryCategory::ENTITIES: category_name = "Entities"; break;
                case MemoryCategory::COMPONENTS: category_name = "Components"; break;
                case MemoryCategory::SYSTEMS: category_name = "Systems"; break;
                case MemoryCategory::GRAPHICS: category_name = "Graphics"; break;
                case MemoryCategory::AUDIO: category_name = "Audio"; break;
                case MemoryCategory::PHYSICS: category_name = "Physics"; break;
                case MemoryCategory::SCRIPTS: category_name = "Scripts"; break;
                case MemoryCategory::ASSETS: category_name = "Assets"; break;
                case MemoryCategory::TEMPORARY: category_name = "Temporary"; break;
                case MemoryCategory::CACHE: category_name = "Cache"; break;
                case MemoryCategory::NETWORKING: category_name = "Networking"; break;
                case MemoryCategory::CUSTOM: category_name = "Custom"; break;
            }
            report << "  " << category_name << ": " << (size / (1024 * 1024)) << " MB\n";
        }
        report << "\n";
    }
    
    // Memory pools
    if (!memory_pools_.empty()) {
        report << "Memory Pools:\n";
        for (const auto& [name, pool] : memory_pools_) {
            report << "  " << name << ":\n";
            report << "    Total: " << (pool.total_size / (1024 * 1024)) << " MB\n";
            report << "    Used: " << (pool.used_size / (1024 * 1024)) << " MB\n";
            report << "    Free: " << (pool.free_size / (1024 * 1024)) << " MB\n";
            report << "    Fragmentation: " << (pool.fragmentation_ratio * 100.0f) << "%\n";
        }
        report << "\n";
    }
    
    // Leak detection results
    if (!detected_leaks_.empty()) {
        report << "Detected Memory Leaks:\n";
        for (const auto& leak : detected_leaks_) {
            if (leak.is_potential_leak && leak.confidence > 0.5f) {
                auto lifetime_hours = duration_cast<hours>(leak.lifetime).count();
                report << "  " << leak.allocation.type_name << " (" << leak.allocation.size << " bytes)\n";
                report << "    Age: " << lifetime_hours << " hours\n";
                report << "    Confidence: " << (leak.confidence * 100.0f) << "%\n";
                report << "    Call Site: " << leak.allocation.call_site << "\n";
                report << "    Analysis: " << leak.analysis << "\n\n";
            }
        }
    }
    
    return report.str();
}

std::vector<MemoryLeak> MemoryDebugger::get_detected_leaks() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return detected_leaks_;
}

std::unordered_map<MemoryCategory, usize> MemoryDebugger::get_category_breakdown() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    std::unordered_map<MemoryCategory, usize> breakdown;
    
    for (const auto& [ptr, record] : active_allocations_) {
        breakdown[record.category] += record.size;
    }
    
    return breakdown;
}

MemoryDebugger& MemoryDebugger::instance() {
    std::lock_guard<std::mutex> lock(g_debugger_instance_mutex);
    if (!g_memory_debugger_instance) {
        g_memory_debugger_instance = std::make_unique<MemoryDebugger>();
    }
    return *g_memory_debugger_instance;
}

void MemoryDebugger::cleanup() {
    std::lock_guard<std::mutex> lock(g_debugger_instance_mutex);
    g_memory_debugger_instance.reset();
}

// ===== ScopedLeakDetector Implementation =====

ScopedLeakDetector::ScopedLeakDetector(const std::string& scope_name)
    : scope_name_(scope_name)
    , initial_snapshot_(MemoryDebugger::instance().get_current_snapshot()) {
}

ScopedLeakDetector::~ScopedLeakDetector() {
    auto final_snapshot = MemoryDebugger::instance().get_current_snapshot();
    
    if (final_snapshot.total_used > initial_snapshot_.total_used) {
        usize leaked = final_snapshot.total_used - initial_snapshot_.total_used;
        // Could log leak detection results
        // std::cerr << "Scope '" << scope_name_ << "' leaked " << leaked << " bytes\n";
    }
}

} // namespace ecscope::debug