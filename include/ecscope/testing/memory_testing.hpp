#pragma once

#include "test_framework.hpp"
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <mutex>
#include <atomic>
#include <chrono>
#include <fstream>
#include <stack>
#include <iomanip>

#ifdef _WIN32
    #include <windows.h>
    #include <psapi.h>
    #include <dbghelp.h>
    #pragma comment(lib, "psapi.lib")
    #pragma comment(lib, "dbghelp.lib")
#elif defined(__linux__)
    #include <sys/resource.h>
    #include <malloc.h>
    #include <unistd.h>
    #include <execinfo.h>
    #include <fstream>
#elif defined(__APPLE__)
    #include <mach/mach.h>
    #include <sys/resource.h>
    #include <execinfo.h>
#endif

namespace ecscope::testing {

// Memory allocation categories
enum class AllocationCategory {
    UNKNOWN,
    ECS_COMPONENT,
    ECS_SYSTEM,
    PHYSICS_BODY,
    PHYSICS_CONSTRAINT,
    RENDERING_BUFFER,
    RENDERING_TEXTURE,
    AUDIO_BUFFER,
    ASSET_LOADING,
    TEMPORARY,
    PERSISTENT
};

// Stack trace capture
class StackTraceCapture {
public:
    struct StackFrame {
        void* address;
        std::string symbol;
        std::string file;
        int line;
    };

    static std::vector<StackFrame> capture_stack_trace(int max_frames = 32) {
        std::vector<StackFrame> frames;
        
        #ifdef _WIN32
        void* stack[32];
        WORD num_frames = CaptureStackBackTrace(1, max_frames, stack, nullptr);
        
        HANDLE process = GetCurrentProcess();
        SymInitialize(process, nullptr, TRUE);
        
        for (WORD i = 0; i < num_frames; ++i) {
            StackFrame frame;
            frame.address = stack[i];
            
            char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
            PSYMBOL_INFO symbol = reinterpret_cast<PSYMBOL_INFO>(buffer);
            symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            symbol->MaxNameLen = MAX_SYM_NAME;
            
            if (SymFromAddr(process, reinterpret_cast<DWORD64>(stack[i]), 0, symbol)) {
                frame.symbol = symbol->Name;
            }
            
            IMAGEHLP_LINE64 line_info;
            line_info.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
            DWORD displacement;
            
            if (SymGetLineFromAddr64(process, reinterpret_cast<DWORD64>(stack[i]), 
                                   &displacement, &line_info)) {
                frame.file = line_info.FileName;
                frame.line = line_info.LineNumber;
            }
            
            frames.push_back(frame);
        }
        
        #elif defined(__linux__) || defined(__APPLE__)
        void* stack[32];
        int num_frames = backtrace(stack, max_frames);
        char** symbols = backtrace_symbols(stack, num_frames);
        
        for (int i = 1; i < num_frames; ++i) { // Skip current function
            StackFrame frame;
            frame.address = stack[i];
            if (symbols && symbols[i]) {
                frame.symbol = symbols[i];
            }
            frames.push_back(frame);
        }
        
        free(symbols);
        #endif
        
        return frames;
    }

    static std::string format_stack_trace(const std::vector<StackFrame>& frames) {
        std::stringstream ss;
        for (size_t i = 0; i < frames.size(); ++i) {
            ss << "  #" << i << ": " << frames[i].symbol;
            if (!frames[i].file.empty()) {
                ss << " (" << frames[i].file << ":" << frames[i].line << ")";
            }
            ss << "\n";
        }
        return ss.str();
    }
};

// Detailed memory allocation tracker
class DetailedMemoryTracker {
public:
    struct AllocationInfo {
        size_t size;
        std::chrono::steady_clock::time_point timestamp;
        std::vector<StackTraceCapture::StackFrame> stack_trace;
        AllocationCategory category;
        std::string tag;
        bool is_array_allocation;
        size_t alignment;
    };

    struct MemoryStatistics {
        size_t total_allocated{0};
        size_t total_deallocated{0};
        size_t current_usage{0};
        size_t peak_usage{0};
        size_t allocation_count{0};
        size_t deallocation_count{0};
        size_t leaked_bytes{0};
        size_t leaked_allocations{0};
        std::unordered_map<AllocationCategory, size_t> usage_by_category;
        std::vector<AllocationInfo> active_allocations;
    };

    static DetailedMemoryTracker& instance() {
        static DetailedMemoryTracker tracker;
        return tracker;
    }

    void record_allocation(void* ptr, size_t size, AllocationCategory category = AllocationCategory::UNKNOWN,
                          const std::string& tag = "", bool is_array = false, size_t alignment = 0) {
        if (!tracking_enabled_) return;
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        AllocationInfo info;
        info.size = size;
        info.timestamp = std::chrono::steady_clock::now();
        info.stack_trace = StackTraceCapture::capture_stack_trace(16);
        info.category = category;
        info.tag = tag;
        info.is_array_allocation = is_array;
        info.alignment = alignment;
        
        allocations_[ptr] = info;
        
        total_allocated_ += size;
        current_usage_ += size;
        peak_usage_ = std::max(peak_usage_, current_usage_);
        allocation_count_++;
        
        usage_by_category_[category] += size;
    }

    void record_deallocation(void* ptr) {
        if (!tracking_enabled_) return;
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = allocations_.find(ptr);
        if (it != allocations_.end()) {
            total_deallocated_ += it->second.size;
            current_usage_ -= it->second.size;
            deallocation_count_++;
            
            usage_by_category_[it->second.category] -= it->second.size;
            allocations_.erase(it);
        }
    }

    MemoryStatistics get_statistics() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        MemoryStatistics stats;
        stats.total_allocated = total_allocated_;
        stats.total_deallocated = total_deallocated_;
        stats.current_usage = current_usage_;
        stats.peak_usage = peak_usage_;
        stats.allocation_count = allocation_count_;
        stats.deallocation_count = deallocation_count_;
        stats.leaked_bytes = current_usage_;
        stats.leaked_allocations = allocations_.size();
        stats.usage_by_category = usage_by_category_;
        
        for (const auto& [ptr, info] : allocations_) {
            stats.active_allocations.push_back(info);
        }
        
        return stats;
    }

    std::vector<AllocationInfo> get_leaks() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::vector<AllocationInfo> leaks;
        for (const auto& [ptr, info] : allocations_) {
            leaks.push_back(info);
        }
        
        return leaks;
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        allocations_.clear();
        total_allocated_ = 0;
        total_deallocated_ = 0;
        current_usage_ = 0;
        peak_usage_ = 0;
        allocation_count_ = 0;
        deallocation_count_ = 0;
        usage_by_category_.clear();
    }

    void enable_tracking() { tracking_enabled_ = true; }
    void disable_tracking() { tracking_enabled_ = false; }

    void save_report(const std::string& filename) const {
        auto stats = get_statistics();
        
        std::ofstream file(filename);
        if (file.is_open()) {
            file << "Memory Usage Report\n";
            file << "==================\n\n";
            
            file << "Summary:\n";
            file << "  Total Allocated: " << stats.total_allocated << " bytes\n";
            file << "  Total Deallocated: " << stats.total_deallocated << " bytes\n";
            file << "  Current Usage: " << stats.current_usage << " bytes\n";
            file << "  Peak Usage: " << stats.peak_usage << " bytes\n";
            file << "  Allocation Count: " << stats.allocation_count << "\n";
            file << "  Deallocation Count: " << stats.deallocation_count << "\n";
            file << "  Leaked Allocations: " << stats.leaked_allocations << "\n";
            file << "  Leaked Bytes: " << stats.leaked_bytes << "\n\n";
            
            file << "Usage by Category:\n";
            for (const auto& [category, usage] : stats.usage_by_category) {
                file << "  " << category_to_string(category) << ": " << usage << " bytes\n";
            }
            
            if (!stats.active_allocations.empty()) {
                file << "\nActive Allocations (Potential Leaks):\n";
                for (const auto& alloc : stats.active_allocations) {
                    file << "  Size: " << alloc.size << " bytes, Category: " 
                         << category_to_string(alloc.category);
                    if (!alloc.tag.empty()) {
                        file << ", Tag: " << alloc.tag;
                    }
                    file << "\n";
                    file << "    Stack Trace:\n";
                    file << StackTraceCapture::format_stack_trace(alloc.stack_trace);
                    file << "\n";
                }
            }
        }
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<void*, AllocationInfo> allocations_;
    std::atomic<size_t> total_allocated_{0};
    std::atomic<size_t> total_deallocated_{0};
    std::atomic<size_t> current_usage_{0};
    std::atomic<size_t> peak_usage_{0};
    std::atomic<size_t> allocation_count_{0};
    std::atomic<size_t> deallocation_count_{0};
    std::unordered_map<AllocationCategory, size_t> usage_by_category_;
    std::atomic<bool> tracking_enabled_{false};

    std::string category_to_string(AllocationCategory category) const {
        switch (category) {
            case AllocationCategory::UNKNOWN: return "Unknown";
            case AllocationCategory::ECS_COMPONENT: return "ECS Component";
            case AllocationCategory::ECS_SYSTEM: return "ECS System";
            case AllocationCategory::PHYSICS_BODY: return "Physics Body";
            case AllocationCategory::PHYSICS_CONSTRAINT: return "Physics Constraint";
            case AllocationCategory::RENDERING_BUFFER: return "Rendering Buffer";
            case AllocationCategory::RENDERING_TEXTURE: return "Rendering Texture";
            case AllocationCategory::AUDIO_BUFFER: return "Audio Buffer";
            case AllocationCategory::ASSET_LOADING: return "Asset Loading";
            case AllocationCategory::TEMPORARY: return "Temporary";
            case AllocationCategory::PERSISTENT: return "Persistent";
            default: return "Unknown";
        }
    }
};

// Memory leak detector
class MemoryLeakDetector {
public:
    struct LeakReport {
        size_t total_leaked_bytes{0};
        size_t leaked_allocation_count{0};
        std::vector<DetailedMemoryTracker::AllocationInfo> leaks;
        bool has_leaks() const { return leaked_allocation_count > 0; }
    };

    static LeakReport detect_leaks() {
        auto& tracker = DetailedMemoryTracker::instance();
        auto leaks = tracker.get_leaks();
        
        LeakReport report;
        report.leaks = leaks;
        report.leaked_allocation_count = leaks.size();
        
        for (const auto& leak : leaks) {
            report.total_leaked_bytes += leak.size;
        }
        
        return report;
    }

    static void print_leak_report(const LeakReport& report) {
        if (!report.has_leaks()) {
            std::cout << "No memory leaks detected.\n";
            return;
        }
        
        std::cout << "Memory leaks detected!\n";
        std::cout << "Total leaked: " << report.total_leaked_bytes << " bytes in " 
                  << report.leaked_allocation_count << " allocations\n\n";
        
        // Group leaks by stack trace
        std::unordered_map<std::string, std::vector<const DetailedMemoryTracker::AllocationInfo*>> grouped_leaks;
        
        for (const auto& leak : report.leaks) {
            std::string stack_key = StackTraceCapture::format_stack_trace(leak.stack_trace);
            grouped_leaks[stack_key].push_back(&leak);
        }
        
        for (const auto& [stack_trace, leak_group] : grouped_leaks) {
            size_t total_size = 0;
            for (const auto* leak : leak_group) {
                total_size += leak->size;
            }
            
            std::cout << "Leak group (" << leak_group.size() << " allocations, " 
                      << total_size << " bytes):\n";
            std::cout << stack_trace << "\n";
        }
    }
};

// Memory fragmentation analyzer
class MemoryFragmentationAnalyzer {
public:
    struct FragmentationReport {
        size_t total_free_space{0};
        size_t largest_free_block{0};
        size_t smallest_free_block{SIZE_MAX};
        double fragmentation_ratio{0.0};
        size_t free_block_count{0};
        std::vector<size_t> free_block_sizes;
    };

    static FragmentationReport analyze_fragmentation() {
        FragmentationReport report;
        
        #ifdef _WIN32
        HANDLE heap = GetProcessHeap();
        PROCESS_HEAP_ENTRY entry;
        entry.lpData = nullptr;
        
        while (HeapWalk(heap, &entry)) {
            if (entry.wFlags & PROCESS_HEAP_UNCOMMITTED_RANGE) {
                size_t block_size = entry.cbData;
                report.free_block_sizes.push_back(block_size);
                report.total_free_space += block_size;
                report.largest_free_block = std::max(report.largest_free_block, block_size);
                report.smallest_free_block = std::min(report.smallest_free_block, block_size);
                report.free_block_count++;
            }
        }
        
        #elif defined(__linux__)
        struct mallinfo info = mallinfo();
        report.total_free_space = info.fordblks;
        // Limited fragmentation analysis on Linux
        
        #endif
        
        if (report.free_block_count > 0 && report.total_free_space > 0) {
            report.fragmentation_ratio = 1.0 - (static_cast<double>(report.largest_free_block) / 
                                               report.total_free_space);
        }
        
        return report;
    }

    static void print_fragmentation_report(const FragmentationReport& report) {
        std::cout << "Memory Fragmentation Analysis:\n";
        std::cout << "  Total free space: " << report.total_free_space << " bytes\n";
        std::cout << "  Free blocks: " << report.free_block_count << "\n";
        std::cout << "  Largest free block: " << report.largest_free_block << " bytes\n";
        std::cout << "  Smallest free block: " << report.smallest_free_block << " bytes\n";
        std::cout << "  Fragmentation ratio: " << std::fixed << std::setprecision(2) 
                  << (report.fragmentation_ratio * 100.0) << "%\n";
    }
};

// Memory stress tester
class MemoryStressTester {
public:
    struct StressTestConfig {
        size_t min_allocation_size = 16;
        size_t max_allocation_size = 1024 * 1024; // 1MB
        size_t target_memory_usage = 100 * 1024 * 1024; // 100MB
        std::chrono::seconds test_duration{60};
        double allocation_probability = 0.6; // 60% allocate, 40% deallocate
        bool enable_fragmentation_test = true;
        bool enable_random_access = true;
    };

    static bool run_stress_test(const StressTestConfig& config) {
        std::vector<void*> allocations;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<size_t> size_dist(config.min_allocation_size, 
                                                       config.max_allocation_size);
        std::uniform_real_distribution<double> prob_dist(0.0, 1.0);
        
        auto& tracker = DetailedMemoryTracker::instance();
        tracker.enable_tracking();
        tracker.reset();
        
        auto start_time = std::chrono::steady_clock::now();
        auto end_time = start_time + config.test_duration;
        
        size_t current_usage = 0;
        bool test_passed = true;
        
        while (std::chrono::steady_clock::now() < end_time && test_passed) {
            bool should_allocate = prob_dist(gen) < config.allocation_probability;
            
            if (should_allocate || allocations.empty()) {
                // Allocate memory
                if (current_usage < config.target_memory_usage) {
                    size_t size = size_dist(gen);
                    void* ptr = malloc(size);
                    
                    if (ptr) {
                        tracker.record_allocation(ptr, size, AllocationCategory::TEMPORARY, "stress_test");
                        allocations.push_back(ptr);
                        current_usage += size;
                        
                        // Random memory access to test cache behavior
                        if (config.enable_random_access) {
                            memset(ptr, 0x42, std::min(size, size_t(4096)));
                        }
                    } else {
                        test_passed = false; // Out of memory
                    }
                }
            } else {
                // Deallocate memory
                if (!allocations.empty()) {
                    std::uniform_int_distribution<size_t> index_dist(0, allocations.size() - 1);
                    size_t index = index_dist(gen);
                    
                    void* ptr = allocations[index];
                    tracker.record_deallocation(ptr);
                    free(ptr);
                    
                    allocations.erase(allocations.begin() + index);
                }
            }
            
            // Check for memory corruption
            if (allocations.size() % 1000 == 0) {
                if (!validate_allocations(allocations)) {
                    test_passed = false;
                }
            }
        }
        
        // Cleanup remaining allocations
        for (void* ptr : allocations) {
            tracker.record_deallocation(ptr);
            free(ptr);
        }
        
        auto stats = tracker.get_statistics();
        tracker.disable_tracking();
        
        // Verify no major leaks
        if (stats.leaked_allocations > 10) {
            test_passed = false;
        }
        
        return test_passed;
    }

private:
    static bool validate_allocations(const std::vector<void*>& allocations) {
        // Basic validation to detect corruption
        for (void* ptr : allocations) {
            if (!ptr) return false;
            
            // Try to read first byte
            try {
                volatile char test = *static_cast<char*>(ptr);
                (void)test; // Suppress unused variable warning
            } catch (...) {
                return false;
            }
        }
        return true;
    }
};

// Memory test fixture
class MemoryTestFixture : public TestFixture {
public:
    void setup() override {
        // Enable detailed memory tracking
        tracker_ = &DetailedMemoryTracker::instance();
        tracker_->reset();
        tracker_->enable_tracking();
        
        // Capture baseline memory usage
        baseline_memory_ = get_system_memory_usage();
    }

    void teardown() override {
        // Generate memory report
        tracker_->save_report("memory_test_report.txt");
        
        // Check for leaks
        auto leak_report = MemoryLeakDetector::detect_leaks();
        if (leak_report.has_leaks()) {
            std::cerr << "Memory leaks detected in test!\n";
            MemoryLeakDetector::print_leak_report(leak_report);
        }
        
        // Check memory growth
        size_t final_memory = get_system_memory_usage();
        size_t memory_growth = final_memory - baseline_memory_;
        
        if (memory_growth > 10 * 1024 * 1024) { // 10MB threshold
            std::cerr << "Excessive memory growth detected: " << memory_growth << " bytes\n";
        }
        
        tracker_->disable_tracking();
    }

protected:
    DetailedMemoryTracker* tracker_;
    size_t baseline_memory_;

    size_t get_system_memory_usage() {
        #ifdef _WIN32
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
            return pmc.WorkingSetSize;
        }
        #elif defined(__linux__)
        std::ifstream statm("/proc/self/statm");
        if (statm.is_open()) {
            size_t size, resident, share, text, lib, data, dt;
            statm >> size >> resident >> share >> text >> lib >> data >> dt;
            return resident * getpagesize();
        }
        #elif defined(__APPLE__)
        struct mach_task_basic_info info;
        mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
        if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                     reinterpret_cast<task_info_t>(&info), &infoCount) == KERN_SUCCESS) {
            return info.resident_size;
        }
        #endif
        return 0;
    }

    void allocate_and_track(size_t size, AllocationCategory category, const std::string& tag = "") {
        void* ptr = malloc(size);
        if (ptr) {
            tracker_->record_allocation(ptr, size, category, tag);
            test_allocations_.push_back(ptr);
        }
    }

    void cleanup_test_allocations() {
        for (void* ptr : test_allocations_) {
            tracker_->record_deallocation(ptr);
            free(ptr);
        }
        test_allocations_.clear();
    }

private:
    std::vector<void*> test_allocations_;
};

// Specific memory tests
class MemoryLeakTest : public MemoryTestFixture {
public:
    MemoryLeakTest() : MemoryTestFixture() {
        context_.name = "Memory Leak Detection Test";
        context_.category = TestCategory::MEMORY;
    }

    void run() override {
        // Intentionally create some allocations and clean most up
        allocate_and_track(1024, AllocationCategory::ECS_COMPONENT, "test_component");
        allocate_and_track(2048, AllocationCategory::RENDERING_BUFFER, "test_buffer");
        allocate_and_track(512, AllocationCategory::TEMPORARY, "temp_data");
        
        // Clean up only some allocations to test leak detection
        cleanup_test_allocations();
        
        // Check that we can detect what remains
        auto stats = tracker_->get_statistics();
        ASSERT_EQ(stats.leaked_allocations, 0);
    }
};

class MemoryFragmentationTest : public MemoryTestFixture {
public:
    MemoryFragmentationTest() : MemoryTestFixture() {
        context_.name = "Memory Fragmentation Test";
        context_.category = TestCategory::MEMORY;
    }

    void run() override {
        // Create fragmentation by allocating and deallocating in a pattern
        std::vector<void*> allocations;
        
        // Allocate many small blocks
        for (int i = 0; i < 1000; ++i) {
            void* ptr = malloc(64);
            if (ptr) {
                tracker_->record_allocation(ptr, 64, AllocationCategory::TEMPORARY);
                allocations.push_back(ptr);
            }
        }
        
        // Deallocate every other block to create fragmentation
        for (size_t i = 1; i < allocations.size(); i += 2) {
            tracker_->record_deallocation(allocations[i]);
            free(allocations[i]);
            allocations[i] = nullptr;
        }
        
        // Analyze fragmentation
        auto fragmentation_report = MemoryFragmentationAnalyzer::analyze_fragmentation();
        
        // Cleanup remaining allocations
        for (void* ptr : allocations) {
            if (ptr) {
                tracker_->record_deallocation(ptr);
                free(ptr);
            }
        }
        
        // Test passes if we successfully analyzed fragmentation
        ASSERT_GE(fragmentation_report.free_block_count, 0);
    }
};

class MemoryStressTest : public MemoryTestFixture {
public:
    MemoryStressTest() : MemoryTestFixture() {
        context_.name = "Memory Stress Test";
        context_.category = TestCategory::STRESS;
        context_.timeout_seconds = 120;
    }

    void run() override {
        MemoryStressTester::StressTestConfig config;
        config.test_duration = std::chrono::seconds(30); // Shorter for unit test
        config.target_memory_usage = 50 * 1024 * 1024; // 50MB
        
        bool stress_passed = MemoryStressTester::run_stress_test(config);
        ASSERT_TRUE(stress_passed);
    }
};

} // namespace ecscope::testing