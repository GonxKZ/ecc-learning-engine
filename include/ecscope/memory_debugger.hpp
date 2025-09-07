#pragma once

#include "types.hpp"
#include "ecs_profiler.hpp"
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <mutex>
#include <atomic>
#include <functional>
#include <chrono>
#include <thread>
#include <algorithm>

namespace ecscope::debug {

using namespace std::chrono;

// Memory allocation categories for tracking
enum class MemoryCategory : u8 {
    ENTITIES,
    COMPONENTS,
    SYSTEMS,
    GRAPHICS,
    AUDIO,
    PHYSICS,
    SCRIPTS,
    ASSETS,
    TEMPORARY,
    CACHE,
    NETWORKING,
    CUSTOM,
    UNKNOWN
};

// Memory allocation information with detailed tracking
struct AllocationRecord {
    void* ptr;
    usize size;
    usize alignment;
    MemoryCategory category;
    std::string type_name;
    std::string call_site;
    std::vector<void*> stack_trace;
    high_resolution_clock::time_point timestamp;
    u32 thread_id;
    u64 allocation_id;
    bool is_freed = false;
    high_resolution_clock::time_point free_timestamp;
    
    // Metadata for analysis
    usize access_count = 0;
    high_resolution_clock::time_point last_access;
    bool is_hot = false; // Frequently accessed
    f32 utilization_ratio = 1.0f; // How much of allocated memory is used
};

// Memory block header for tracking (injected before user allocation)
struct MemoryBlockHeader {
    u64 magic_number = 0xDEADBEEFCAFEBABE;
    u64 allocation_id;
    usize size;
    usize alignment;
    MemoryCategory category;
    u32 checksum;
    
    // Corruption detection
    u64 guard_before = 0xAAAAAAAAAAAAAAAA;
    // User data goes here
    // u64 guard_after = 0xBBBBBBBBBBBBBBBB; (placed after user data)
    
    bool is_valid() const {
        return magic_number == 0xDEADBEEFCAFEBABE &&
               guard_before == 0xAAAAAAAAAAAAAAAA;
    }
    
    static usize get_header_size() {
        return sizeof(MemoryBlockHeader);
    }
    
    static usize get_footer_size() {
        return sizeof(u64); // guard_after
    }
    
    static usize get_total_overhead() {
        return get_header_size() + get_footer_size();
    }
};

// Memory pool information for custom allocators
struct MemoryPool {
    std::string name;
    void* base_ptr;
    usize total_size;
    usize used_size;
    usize free_size;
    usize block_count;
    usize largest_free_block;
    f32 fragmentation_ratio;
    MemoryCategory category;
    std::vector<std::pair<void*, usize>> free_blocks;
    std::vector<std::pair<void*, usize>> used_blocks;
    high_resolution_clock::time_point creation_time;
    
    void update_fragmentation() {
        if (free_size == 0 || free_blocks.empty()) {
            fragmentation_ratio = 0.0f;
            return;
        }
        
        // Calculate external fragmentation
        usize total_free = 0;
        usize max_free = 0;
        for (const auto& [ptr, size] : free_blocks) {
            total_free += size;
            max_free = std::max(max_free, size);
        }
        
        fragmentation_ratio = 1.0f - (static_cast<f32>(max_free) / total_free);
    }
};

// Memory leak detection information
struct MemoryLeak {
    AllocationRecord allocation;
    high_resolution_clock::duration lifetime;
    usize severity_score; // Based on size and lifetime
    std::string analysis;
    bool is_potential_leak;
    f32 confidence; // 0.0 to 1.0
};

// Memory usage statistics over time
struct MemoryUsageSnapshot {
    high_resolution_clock::time_point timestamp;
    usize total_allocated;
    usize total_used;
    usize peak_usage;
    usize allocation_count;
    f32 fragmentation;
    std::unordered_map<MemoryCategory, usize> category_usage;
    std::vector<usize> allocation_sizes; // Histogram data
};

// Memory access pattern analysis
struct AccessPattern {
    void* ptr;
    std::vector<high_resolution_clock::time_point> access_times;
    usize sequential_accesses = 0;
    usize random_accesses = 0;
    f32 locality_score = 0.0f; // Temporal and spatial locality
    bool is_cache_friendly = true;
};

// Configuration for memory debugging
struct MemoryDebugConfig {
    bool enable_allocation_tracking = true;
    bool enable_leak_detection = true;
    bool enable_corruption_detection = true;
    bool enable_access_tracking = false; // Performance impact
    bool enable_stack_traces = true;
    bool enable_pool_monitoring = true;
    
    usize max_allocations_tracked = 1000000;
    usize stack_trace_depth = 16;
    f32 leak_detection_threshold_hours = 1.0f;
    usize large_allocation_threshold = 1024 * 1024; // 1MB
    
    // Memory pattern detection
    bool detect_buffer_overruns = true;
    bool detect_use_after_free = true;
    bool detect_double_free = true;
    bool detect_memory_leaks = true;
    bool detect_fragmentation = true;
};

// Advanced memory debugger class
class MemoryDebugger {
private:
    mutable std::mutex data_mutex_;
    std::atomic<bool> enabled_{true};
    MemoryDebugConfig config_;
    
    // Allocation tracking
    std::unordered_map<void*, AllocationRecord> active_allocations_;
    std::unordered_map<u64, AllocationRecord> allocation_history_;
    std::atomic<u64> next_allocation_id_{1};
    
    // Memory pools
    std::unordered_map<std::string, MemoryPool> memory_pools_;
    
    // Usage tracking
    std::vector<MemoryUsageSnapshot> usage_history_;
    usize max_usage_history_ = 10000;
    
    // Leak detection
    std::vector<MemoryLeak> detected_leaks_;
    high_resolution_clock::time_point last_leak_check_;
    
    // Access pattern analysis
    std::unordered_map<void*, AccessPattern> access_patterns_;
    
    // Statistics
    std::atomic<usize> total_allocated_{0};
    std::atomic<usize> total_deallocated_{0};
    std::atomic<usize> current_usage_{0};
    std::atomic<usize> peak_usage_{0};
    std::atomic<u64> allocation_count_{0};
    std::atomic<u64> deallocation_count_{0};
    
    // Internal methods
    std::vector<void*> capture_stack_trace(usize max_depth) const;
    std::string format_stack_trace(const std::vector<void*>& stack) const;
    u32 calculate_checksum(const void* data, usize size) const;
    bool verify_memory_integrity(const AllocationRecord& record) const;
    void analyze_access_patterns();
    void detect_leaks_internal();
    void update_usage_statistics();
    MemoryCategory categorize_allocation(const std::string& type_name, const void* ptr) const;
    
    // Hook system for custom allocators
    std::vector<std::function<void(void*, usize, MemoryCategory)>> allocation_hooks_;
    std::vector<std::function<void(void*, usize)>> deallocation_hooks_;
    
public:
    MemoryDebugger();
    ~MemoryDebugger();
    
    // Configuration
    void set_enabled(bool enabled) { enabled_ = enabled; }
    bool is_enabled() const { return enabled_; }
    
    void set_config(const MemoryDebugConfig& config) { config_ = config; }
    const MemoryDebugConfig& get_config() const { return config_; }
    
    // Allocation tracking (to be called by custom allocators)
    void* allocate_tracked(usize size, usize alignment, MemoryCategory category,
                          const std::string& type_name, const char* file = nullptr,
                          int line = 0, const char* function = nullptr);
    
    void deallocate_tracked(void* ptr);
    
    // Manual tracking for external allocators
    void register_allocation(void* ptr, usize size, usize alignment, MemoryCategory category,
                           const std::string& type_name, const std::string& call_site = "");
    void unregister_allocation(void* ptr);
    
    // Memory pool management
    void register_pool(const std::string& name, void* base_ptr, usize size, MemoryCategory category);
    void unregister_pool(const std::string& name);
    void update_pool_usage(const std::string& name, usize used_size, 
                          const std::vector<std::pair<void*, usize>>& free_blocks);
    
    // Access tracking
    void record_memory_access(void* ptr, usize size, bool is_write = false);
    
    // Analysis and detection
    void check_for_leaks();
    void check_memory_integrity();
    void analyze_fragmentation();
    std::vector<MemoryLeak> get_detected_leaks() const;
    std::vector<std::string> get_corruption_reports() const;
    
    // Statistics and reporting
    usize get_current_usage() const { return current_usage_; }
    usize get_peak_usage() const { return peak_usage_; }
    u64 get_allocation_count() const { return allocation_count_; }
    u64 get_deallocation_count() const { return deallocation_count_; }
    f32 get_overall_fragmentation() const;
    
    std::vector<AllocationRecord> get_large_allocations(usize threshold = 0) const;
    std::vector<AllocationRecord> get_long_lived_allocations(seconds min_age = seconds(60)) const;
    std::vector<AllocationRecord> get_allocations_by_category(MemoryCategory category) const;
    
    MemoryUsageSnapshot get_current_snapshot() const;
    std::vector<MemoryUsageSnapshot> get_usage_history() const;
    
    std::unordered_map<MemoryCategory, usize> get_category_breakdown() const;
    std::unordered_map<std::string, usize> get_type_breakdown() const;
    
    // Memory pool information
    std::vector<MemoryPool> get_all_pools() const;
    MemoryPool get_pool_info(const std::string& name) const;
    
    // Reporting
    std::string generate_memory_report() const;
    std::string generate_leak_report() const;
    std::string generate_fragmentation_report() const;
    void export_to_csv(const std::string& filename) const;
    void export_usage_history(const std::string& filename) const;
    
    // Debugging utilities
    void dump_allocation_info(void* ptr) const;
    void dump_all_allocations() const;
    void set_breakpoint_on_allocation(usize size, MemoryCategory category = MemoryCategory::UNKNOWN);
    void set_breakpoint_on_address(void* ptr);
    
    // Hook system
    void add_allocation_hook(std::function<void(void*, usize, MemoryCategory)> hook);
    void add_deallocation_hook(std::function<void(void*, usize)> hook);
    void clear_hooks();
    
    // Memory visualization helpers
    struct MemoryMap {
        struct Block {
            void* start;
            usize size;
            MemoryCategory category;
            std::string type_name;
            bool is_free;
        };
        std::vector<Block> blocks;
        usize total_size;
        usize used_size;
    };
    
    MemoryMap generate_memory_map() const;
    std::vector<std::pair<usize, usize>> get_allocation_size_histogram() const;
    
    // Control
    void clear_statistics();
    void reset();
    void enable_leak_detection(bool enable) { config_.enable_leak_detection = enable; }
    void enable_corruption_detection(bool enable) { config_.enable_corruption_detection = enable; }
    
    // Singleton access
    static MemoryDebugger& instance();
    static void cleanup();
};

// Custom allocator wrapper with debugging
template<typename T>
class DebugAllocator {
public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    
    DebugAllocator() = default;
    
    template<typename U>
    DebugAllocator(const DebugAllocator<U>&) noexcept {}
    
    T* allocate(size_type n, const void* hint = nullptr) {
        void* ptr = MemoryDebugger::instance().allocate_tracked(
            n * sizeof(T), alignof(T), MemoryCategory::CUSTOM, typeid(T).name());
        return static_cast<T*>(ptr);
    }
    
    void deallocate(T* ptr, size_type n) {
        MemoryDebugger::instance().deallocate_tracked(ptr);
    }
    
    template<typename U>
    bool operator==(const DebugAllocator<U>&) const noexcept {
        return true;
    }
    
    template<typename U>
    bool operator!=(const DebugAllocator<U>&) const noexcept {
        return false;
    }
};

// Memory debugging macros
#define DEBUG_MALLOC(size, category) \
    ecscope::debug::MemoryDebugger::instance().allocate_tracked( \
        size, 1, category, "malloc", __FILE__, __LINE__, __FUNCTION__)

#define DEBUG_FREE(ptr) \
    ecscope::debug::MemoryDebugger::instance().deallocate_tracked(ptr)

#define DEBUG_NEW(type, category) \
    static_cast<type*>(ecscope::debug::MemoryDebugger::instance().allocate_tracked( \
        sizeof(type), alignof(type), category, #type, __FILE__, __LINE__, __FUNCTION__))

#define DEBUG_DELETE(ptr) \
    ecscope::debug::MemoryDebugger::instance().deallocate_tracked(ptr)

#define DEBUG_RECORD_ACCESS(ptr, size) \
    if (ecscope::debug::MemoryDebugger::instance().get_config().enable_access_tracking) { \
        ecscope::debug::MemoryDebugger::instance().record_memory_access(ptr, size); \
    }

// RAII memory leak detector for scope-based checking
class ScopedLeakDetector {
private:
    std::string scope_name_;
    MemoryUsageSnapshot initial_snapshot_;
    
public:
    ScopedLeakDetector(const std::string& scope_name);
    ~ScopedLeakDetector();
};

#define DETECT_LEAKS_IN_SCOPE(name) \
    ecscope::debug::ScopedLeakDetector _leak_detector(name)

} // namespace ecscope::debug