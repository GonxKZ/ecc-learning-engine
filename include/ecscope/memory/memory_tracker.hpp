#pragma once

#include "allocators.hpp"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <chrono>
#include <stack>
#include <fstream>
#include <sstream>
#include <regex>
#include <thread>
#include <shared_mutex>

#ifdef _WIN32
    #include <windows.h>
    #include <dbghelp.h>
    #include <psapi.h>
    #pragma comment(lib, "dbghelp.lib")
    #pragma comment(lib, "psapi.lib")
#else
    #include <execinfo.h>
    #include <cxxabi.h>
    #include <dlfcn.h>
    #include <unistd.h>
    #include <sys/resource.h>
#endif

namespace ecscope::memory {

// ==== STACK TRACE CAPTURE ====
class StackTraceCapture {
    static constexpr size_t MAX_STACK_FRAMES = 32;
    
public:
    struct StackFrame {
        void* address;
        std::string function_name;
        std::string file_name;
        int line_number;
        
        StackFrame() : address(nullptr), line_number(-1) {}
    };
    
    using StackTrace = std::vector<StackFrame>;
    
    static StackTrace capture_current_stack() {
        StackTrace trace;
        
#ifdef _WIN32
        void* stack[MAX_STACK_FRAMES];
        WORD frames = CaptureStackBackTrace(1, MAX_STACK_FRAMES, stack, nullptr);
        
        HANDLE process = GetCurrentProcess();
        SymInitialize(process, nullptr, TRUE);
        
        char symbol_buffer[sizeof(SYMBOL_INFO) + 256];
        SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(symbol_buffer);
        symbol->MaxNameLen = 255;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        
        IMAGEHLP_LINE64 line_info{};
        line_info.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        DWORD displacement = 0;
        
        for (int i = 0; i < frames; ++i) {
            StackFrame frame;
            frame.address = stack[i];
            
            if (SymFromAddr(process, reinterpret_cast<DWORD64>(stack[i]), nullptr, symbol)) {
                frame.function_name = symbol->Name;
            }
            
            if (SymGetLineFromAddr64(process, reinterpret_cast<DWORD64>(stack[i]), 
                                   &displacement, &line_info)) {
                frame.file_name = line_info.FileName;
                frame.line_number = static_cast<int>(line_info.LineNumber);
            }
            
            trace.push_back(frame);
        }
        
        SymCleanup(process);
#else
        void* stack[MAX_STACK_FRAMES];
        int frames = backtrace(stack, MAX_STACK_FRAMES);
        char** symbols = backtrace_symbols(stack, frames);
        
        for (int i = 1; i < frames; ++i) { // Skip current frame
            StackFrame frame;
            frame.address = stack[i];
            
            if (symbols && symbols[i]) {
                // Parse symbol information
                std::string symbol_str(symbols[i]);
                parse_symbol_info(symbol_str, frame);
            }
            
            trace.push_back(frame);
        }
        
        free(symbols);
#endif
        
        return trace;
    }
    
    static std::string format_stack_trace(const StackTrace& trace) {
        std::ostringstream oss;
        
        for (size_t i = 0; i < trace.size(); ++i) {
            const auto& frame = trace[i];
            oss << "  #" << i << " ";
            
            if (!frame.function_name.empty()) {
                oss << frame.function_name;
            } else {
                oss << "0x" << std::hex << reinterpret_cast<uintptr_t>(frame.address);
            }
            
            if (!frame.file_name.empty() && frame.line_number > 0) {
                oss << " at " << frame.file_name << ":" << frame.line_number;
            }
            
            oss << "\n";
        }
        
        return oss.str();
    }

private:
#ifndef _WIN32
    static void parse_symbol_info(const std::string& symbol_str, StackFrame& frame) {
        // Format: module(function+offset) [address]
        std::regex symbol_regex(R"((.+)\((.+)\+0x[0-9a-f]+\) \[0x([0-9a-f]+)\])");
        std::smatch matches;
        
        if (std::regex_match(symbol_str, matches, symbol_regex)) {
            std::string mangled_name = matches[2].str();
            
            // Demangle C++ function names
            int status = 0;
            char* demangled = abi::__cxa_demangle(mangled_name.c_str(), nullptr, nullptr, &status);
            if (status == 0 && demangled) {
                frame.function_name = demangled;
                free(demangled);
            } else {
                frame.function_name = mangled_name;
            }
        } else {
            frame.function_name = symbol_str;
        }
    }
#endif
};

// ==== ALLOCATION RECORD ====
struct AllocationRecord {
    void* address;
    size_t size;
    size_t alignment;
    std::chrono::steady_clock::time_point timestamp;
    StackTraceCapture::StackTrace stack_trace;
    std::thread::id thread_id;
    std::string tag; // Optional user tag
    
    AllocationRecord(void* addr, size_t sz, size_t align, std::string user_tag = "")
        : address(addr), size(sz), alignment(align), 
          timestamp(std::chrono::steady_clock::now()),
          stack_trace(StackTraceCapture::capture_current_stack()),
          thread_id(std::this_thread::get_id()),
          tag(std::move(user_tag)) {}
};

// ==== ALLOCATION STATISTICS ====
class AllocationStatistics {
public:
    struct Statistics {
        size_t total_allocations = 0;
        size_t total_deallocations = 0;
        size_t current_allocations = 0;
        size_t peak_allocations = 0;
        size_t total_bytes_allocated = 0;
        size_t total_bytes_deallocated = 0;
        size_t current_bytes = 0;
        size_t peak_bytes = 0;
        double average_allocation_size = 0.0;
        double allocation_rate = 0.0;        // allocations per second
        double deallocation_rate = 0.0;      // deallocations per second
        std::chrono::steady_clock::duration total_allocation_time{};
        
        // Size distribution
        size_t small_allocations = 0;   // <= 256 bytes
        size_t medium_allocations = 0;  // 256 bytes - 64KB
        size_t large_allocations = 0;   // > 64KB
        
        // Thread statistics
        size_t unique_threads = 0;
        std::thread::id most_active_thread{};
        size_t most_active_thread_allocations = 0;
    };
    
    void record_allocation(const AllocationRecord& record) {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        
        ++stats_.total_allocations;
        ++stats_.current_allocations;
        stats_.peak_allocations = std::max(stats_.peak_allocations, stats_.current_allocations);
        
        stats_.total_bytes_allocated += record.size;
        stats_.current_bytes += record.size;
        stats_.peak_bytes = std::max(stats_.peak_bytes, stats_.current_bytes);
        
        // Update size distribution
        if (record.size <= 256) {
            ++stats_.small_allocations;
        } else if (record.size <= 65536) {
            ++stats_.medium_allocations;
        } else {
            ++stats_.large_allocations;
        }
        
        // Track thread activity
        ++thread_allocation_counts_[record.thread_id];
        if (thread_allocation_counts_[record.thread_id] > stats_.most_active_thread_allocations) {
            stats_.most_active_thread = record.thread_id;
            stats_.most_active_thread_allocations = thread_allocation_counts_[record.thread_id];
        }
        stats_.unique_threads = thread_allocation_counts_.size();
        
        update_rates();
    }
    
    void record_deallocation(void* address, size_t size) {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        
        ++stats_.total_deallocations;
        if (stats_.current_allocations > 0) {
            --stats_.current_allocations;
        }
        
        stats_.total_bytes_deallocated += size;
        if (stats_.current_bytes >= size) {
            stats_.current_bytes -= size;
        }
        
        update_rates();
    }
    
    [[nodiscard]] Statistics get_statistics() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto result = stats_;
        
        if (result.total_allocations > 0) {
            result.average_allocation_size = 
                static_cast<double>(result.total_bytes_allocated) / result.total_allocations;
        }
        
        return result;
    }
    
    void reset() {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        stats_ = Statistics{};
        thread_allocation_counts_.clear();
        last_rate_update_ = std::chrono::steady_clock::now();
        allocations_since_last_update_ = 0;
        deallocations_since_last_update_ = 0;
    }

private:
    void update_rates() {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_rate_update_);
        
        if (elapsed.count() >= 1000) { // Update every second
            double seconds = elapsed.count() / 1000.0;
            stats_.allocation_rate = allocations_since_last_update_ / seconds;
            stats_.deallocation_rate = deallocations_since_last_update_ / seconds;
            
            last_rate_update_ = now;
            allocations_since_last_update_ = 0;
            deallocations_since_last_update_ = 0;
        } else {
            ++allocations_since_last_update_;
        }
    }
    
    mutable std::shared_mutex mutex_;
    Statistics stats_;
    std::unordered_map<std::thread::id, size_t> thread_allocation_counts_;
    
    std::chrono::steady_clock::time_point last_rate_update_ = std::chrono::steady_clock::now();
    size_t allocations_since_last_update_ = 0;
    size_t deallocations_since_last_update_ = 0;
};

// ==== MEMORY LEAK DETECTOR ====
class MemoryLeakDetector {
public:
    explicit MemoryLeakDetector(bool enable_stack_traces = true) 
        : capture_stack_traces_(enable_stack_traces) {}
    
    void record_allocation(void* address, size_t size, size_t alignment, 
                          const std::string& tag = "") {
        if (!address) return;
        
        std::lock_guard<std::shared_mutex> lock(mutex_);
        
        if (capture_stack_traces_) {
            allocations_[address] = std::make_unique<AllocationRecord>(
                address, size, alignment, tag);
        } else {
            // Lightweight mode - just track size and timestamp
            allocations_[address] = std::make_unique<AllocationRecord>(
                address, size, alignment, tag);
            allocations_[address]->stack_trace.clear(); // Don't capture stack trace
        }
        
        statistics_.record_allocation(*allocations_[address]);
    }
    
    void record_deallocation(void* address) {
        if (!address) return;
        
        std::lock_guard<std::shared_mutex> lock(mutex_);
        
        auto it = allocations_.find(address);
        if (it != allocations_.end()) {
            statistics_.record_deallocation(address, it->second->size);
            allocations_.erase(it);
        } else {
            // Double-free or invalid free
            ++double_free_count_;
            if (capture_stack_traces_) {
                auto trace = StackTraceCapture::capture_current_stack();
                invalid_frees_[address] = std::move(trace);
            }
        }
    }
    
    // Leak detection
    struct LeakReport {
        size_t total_leaked_bytes = 0;
        size_t leaked_allocation_count = 0;
        
        struct LeakedAllocation {
            void* address;
            size_t size;
            std::chrono::steady_clock::duration lifetime;
            std::string stack_trace;
            std::thread::id thread_id;
            std::string tag;
        };
        std::vector<LeakedAllocation> leaks;
    };
    
    [[nodiscard]] LeakReport generate_leak_report() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        
        LeakReport report;
        auto now = std::chrono::steady_clock::now();
        
        for (const auto& [address, record] : allocations_) {
            LeakReport::LeakedAllocation leak;
            leak.address = address;
            leak.size = record->size;
            leak.lifetime = now - record->timestamp;
            leak.thread_id = record->thread_id;
            leak.tag = record->tag;
            
            if (capture_stack_traces_) {
                leak.stack_trace = StackTraceCapture::format_stack_trace(record->stack_trace);
            }
            
            report.leaks.push_back(leak);
            report.total_leaked_bytes += record->size;
        }
        
        report.leaked_allocation_count = report.leaks.size();
        
        // Sort by size (largest first)
        std::sort(report.leaks.begin(), report.leaks.end(),
                 [](const auto& a, const auto& b) { return a.size > b.size; });
        
        return report;
    }
    
    // Memory corruption detection
    struct CorruptionReport {
        size_t double_frees = 0;
        size_t invalid_frees = 0;
        std::vector<std::pair<void*, std::string>> invalid_free_traces;
    };
    
    [[nodiscard]] CorruptionReport generate_corruption_report() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        
        CorruptionReport report;
        report.double_frees = double_free_count_;
        report.invalid_frees = invalid_frees_.size();
        
        for (const auto& [address, trace] : invalid_frees_) {
            std::string trace_str = StackTraceCapture::format_stack_trace(trace);
            report.invalid_free_traces.emplace_back(address, trace_str);
        }
        
        return report;
    }
    
    // Hotspot analysis
    struct HotspotReport {
        struct AllocationHotspot {
            std::string function_signature;
            std::string file_location;
            size_t allocation_count;
            size_t total_bytes;
            double average_size;
        };
        std::vector<AllocationHotspot> hotspots;
    };
    
    [[nodiscard]] HotspotReport generate_hotspot_report() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        
        std::unordered_map<std::string, std::pair<size_t, size_t>> function_stats; // count, bytes
        
        for (const auto& [address, record] : allocations_) {
            if (!record->stack_trace.empty()) {
                const auto& top_frame = record->stack_trace[0];
                std::string key = top_frame.function_name;
                if (!top_frame.file_name.empty()) {
                    key += " (" + top_frame.file_name + ":" + 
                           std::to_string(top_frame.line_number) + ")";
                }
                
                auto& [count, bytes] = function_stats[key];
                ++count;
                bytes += record->size;
            }
        }
        
        HotspotReport report;
        for (const auto& [signature, stats] : function_stats) {
            HotspotReport::AllocationHotspot hotspot;
            hotspot.function_signature = signature;
            hotspot.allocation_count = stats.first;
            hotspot.total_bytes = stats.second;
            hotspot.average_size = static_cast<double>(stats.second) / stats.first;
            
            report.hotspots.push_back(hotspot);
        }
        
        // Sort by total bytes (largest first)
        std::sort(report.hotspots.begin(), report.hotspots.end(),
                 [](const auto& a, const auto& b) { return a.total_bytes > b.total_bytes; });
        
        return report;
    }
    
    [[nodiscard]] AllocationStatistics::Statistics get_statistics() const {
        return statistics_.get_statistics();
    }
    
    // Export reports
    void export_leak_report(const std::string& filename) const {
        auto report = generate_leak_report();
        std::ofstream file(filename);
        
        if (!file) return;
        
        file << "Memory Leak Report\n";
        file << "==================\n\n";
        file << "Total leaked bytes: " << report.total_leaked_bytes << "\n";
        file << "Leaked allocations: " << report.leaked_allocation_count << "\n\n";
        
        for (const auto& leak : report.leaks) {
            auto lifetime_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                leak.lifetime).count();
            
            file << "Leak: " << leak.size << " bytes at " << leak.address 
                 << " (lifetime: " << lifetime_ms << "ms)\n";
            if (!leak.tag.empty()) {
                file << "  Tag: " << leak.tag << "\n";
            }
            file << "  Stack trace:\n" << leak.stack_trace << "\n";
        }
    }
    
    void enable_stack_traces(bool enable) {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        capture_stack_traces_ = enable;
    }
    
    void reset() {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        allocations_.clear();
        invalid_frees_.clear();
        double_free_count_ = 0;
        statistics_.reset();
    }

private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<void*, std::unique_ptr<AllocationRecord>> allocations_;
    std::unordered_map<void*, StackTraceCapture::StackTrace> invalid_frees_;
    size_t double_free_count_ = 0;
    bool capture_stack_traces_;
    AllocationStatistics statistics_;
};

// ==== MEMORY BANDWIDTH MONITOR ====
class MemoryBandwidthMonitor {
    static constexpr size_t SAMPLE_BUFFER_SIZE = 1000;
    
    struct BandwidthSample {
        std::chrono::steady_clock::time_point timestamp;
        size_t bytes_read;
        size_t bytes_written;
    };
    
public:
    void record_read(size_t bytes) {
        std::lock_guard<std::mutex> lock(mutex_);
        total_bytes_read_ += bytes;
        current_sample_.bytes_read += bytes;
        check_and_store_sample();
    }
    
    void record_write(size_t bytes) {
        std::lock_guard<std::mutex> lock(mutex_);
        total_bytes_written_ += bytes;
        current_sample_.bytes_written += bytes;
        check_and_store_sample();
    }
    
    struct BandwidthStatistics {
        double read_bandwidth_mbps = 0.0;      // MB/s
        double write_bandwidth_mbps = 0.0;     // MB/s
        double total_bandwidth_mbps = 0.0;     // MB/s
        double peak_read_bandwidth_mbps = 0.0;
        double peak_write_bandwidth_mbps = 0.0;
        double average_read_bandwidth_mbps = 0.0;
        double average_write_bandwidth_mbps = 0.0;
        size_t total_bytes_read = 0;
        size_t total_bytes_written = 0;
    };
    
    [[nodiscard]] BandwidthStatistics get_statistics() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        BandwidthStatistics stats;
        stats.total_bytes_read = total_bytes_read_;
        stats.total_bytes_written = total_bytes_written_;
        
        if (samples_.size() < 2) return stats;
        
        // Calculate current bandwidth (last second)
        auto now = std::chrono::steady_clock::now();
        size_t recent_read = 0, recent_write = 0;
        
        for (auto it = samples_.rbegin(); it != samples_.rend(); ++it) {
            auto age = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - it->timestamp);
            if (age.count() > 1000) break; // Last second
            
            recent_read += it->bytes_read;
            recent_write += it->bytes_written;
        }
        
        stats.read_bandwidth_mbps = recent_read / (1024.0 * 1024.0);
        stats.write_bandwidth_mbps = recent_write / (1024.0 * 1024.0);
        stats.total_bandwidth_mbps = stats.read_bandwidth_mbps + stats.write_bandwidth_mbps;
        
        // Calculate peak and average
        double max_read = 0, max_write = 0;
        double total_read = 0, total_write = 0;
        
        for (const auto& sample : samples_) {
            double read_mbps = sample.bytes_read / (1024.0 * 1024.0);
            double write_mbps = sample.bytes_written / (1024.0 * 1024.0);
            
            max_read = std::max(max_read, read_mbps);
            max_write = std::max(max_write, write_mbps);
            total_read += read_mbps;
            total_write += write_mbps;
        }
        
        stats.peak_read_bandwidth_mbps = max_read;
        stats.peak_write_bandwidth_mbps = max_write;
        stats.average_read_bandwidth_mbps = total_read / samples_.size();
        stats.average_write_bandwidth_mbps = total_write / samples_.size();
        
        return stats;
    }

private:
    void check_and_store_sample() {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - current_sample_.timestamp);
        
        if (elapsed.count() >= 100) { // Store sample every 100ms
            current_sample_.timestamp = now;
            samples_.push_back(current_sample_);
            
            if (samples_.size() > SAMPLE_BUFFER_SIZE) {
                samples_.erase(samples_.begin());
            }
            
            current_sample_ = BandwidthSample{now, 0, 0};
        }
    }
    
    mutable std::mutex mutex_;
    std::vector<BandwidthSample> samples_;
    BandwidthSample current_sample_{std::chrono::steady_clock::now(), 0, 0};
    size_t total_bytes_read_ = 0;
    size_t total_bytes_written_ = 0;
};

} // namespace ecscope::memory