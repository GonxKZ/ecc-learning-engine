#pragma once

/**
 * @file memory_tracker.hpp
 * @brief Advanced Memory Tracking System for ECScope Educational ECS Engine
 * 
 * This comprehensive memory tracker provides sophisticated memory analysis and debugging
 * capabilities for educational purposes. It integrates seamlessly with Arena, Pool, and
 * PMR allocators to provide real-time insights into memory usage patterns.
 * 
 * Key Educational Features:
 * - Global memory allocation tracking across all allocator types
 * - Category-based allocation analysis (ECS, Renderer, Audio, etc.)
 * - Call stack capture for allocation origin tracking
 * - Memory leak detection with detailed reporting
 * - Access pattern analysis (sequential vs random)
 * - Allocation size distribution analysis
 * - Temporal allocation patterns (timing analysis)
 * - Memory waste analysis (alignment padding, fragmentation)
 * - Performance monitoring (allocation rates, bandwidth)
 * - Cache behavior prediction and analysis
 * - Memory pressure detection and warnings
 * - Allocation hot-spot identification
 * - Memory usage heat maps
 * - Predictive analysis for future memory needs
 * 
 * Architecture:
 * - Thread-safe tracking with lock-free techniques where possible
 * - Minimal performance impact through smart sampling
 * - Comprehensive statistics collection
 * - Real-time visualization support
 * - Integration with existing allocator infrastructure
 * - Export capabilities for offline analysis
 * 
 * Performance Characteristics:
 * - < 5% overhead in tracking mode
 * - < 1% overhead in minimal mode
 * - Lock-free fast paths for allocation tracking
 * - Configurable detail levels for performance tuning
 * - Smart sampling for high-frequency allocations
 * 
 * @author ECScope Educational ECS Framework
 * @date 2024
 */

#include "core/types.hpp"
#include "core/time.hpp"
#include "core/log.hpp"
#include <atomic>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <array>
#include <deque>
#include <functional>
#include <optional>
#include <span>

namespace ecscope::memory {

// Forward declarations
class ArenaAllocator;
class PoolAllocator;

// Forward declare existing AllocationInfo from arena.hpp to avoid conflicts
struct AllocationInfo;

/**
 * @brief Memory allocation categories for tracking and analysis
 */
enum class AllocationCategory : u32 {
    Unknown = 0,
    ECS_Core,           // Entity-Component-System infrastructure
    ECS_Components,     // Component data storage
    ECS_Systems,        // System temporary allocations
    Renderer_Meshes,    // Mesh and geometry data
    Renderer_Textures,  // Texture and image data
    Renderer_Shaders,   // Shader compilation and storage
    Audio_Buffers,      // Audio sample buffers
    Audio_Streaming,    // Audio streaming infrastructure
    Physics_Bodies,     // Physics object data
    Physics_Collision,  // Collision detection structures
    UI_Widgets,         // UI element allocations
    UI_Rendering,       // UI rendering buffers
    IO_FileSystem,      // File system operations
    IO_Network,         // Network communication
    Scripting_VM,       // Scripting virtual machine
    Scripting_Objects,  // Script object allocations
    Debug_Tools,        // Debug and profiling tools
    Temporary,          // Short-lived temporary allocations
    Custom_01,          // User-defined category 1
    Custom_02,          // User-defined category 2
    Custom_03,          // User-defined category 3
    Custom_04,          // User-defined category 4
    COUNT               // Must be last
};

/**
 * @brief Memory access patterns for educational analysis
 */
enum class AccessPattern : u8 {
    Unknown = 0,
    Sequential,         // Sequential access pattern
    Random,            // Random access pattern
    Streaming,         // Streaming access (read/write once)
    Circular,          // Circular buffer pattern
    Stack,             // Stack-like LIFO pattern
    Queue,             // Queue-like FIFO pattern
    Tree,              // Tree traversal pattern
    Hash,              // Hash table access pattern
};

/**
 * @brief Allocator types for identification
 */
enum class AllocatorType : u8 {
    Unknown = 0,
    System_Malloc,      // Standard malloc/free
    Arena,              // Linear arena allocator
    Pool,               // Fixed-size pool allocator
    PMR_Arena,          // PMR-adapted arena
    PMR_Pool,           // PMR-adapted pool
    PMR_Monotonic,      // PMR monotonic buffer
    Custom              // Custom allocator implementation
};

/**
 * @brief Call stack frame for allocation origin tracking
 */
struct CallStackFrame {
    void* address;              // Function address
    const char* function_name;  // Function name (if available)
    const char* file_name;      // Source file name
    u32 line_number;           // Line number
    u32 column;                // Column number (if available)
    
    CallStackFrame() noexcept 
        : address(nullptr), function_name(nullptr), file_name(nullptr), 
          line_number(0), column(0) {}
    
    bool is_valid() const noexcept {
        return address != nullptr;
    }
};

/**
 * @brief Call stack information for allocation tracking
 */
struct CallStack {
    static constexpr usize MAX_FRAMES = 16;
    
    std::array<CallStackFrame, MAX_FRAMES> frames;
    u8 frame_count;
    u64 hash;                   // Hash of the call stack for quick comparison
    
    CallStack() noexcept : frame_count(0), hash(0) {}
    
    void clear() noexcept {
        frame_count = 0;
        hash = 0;
        frames.fill(CallStackFrame{});
    }
    
    std::span<const CallStackFrame> get_frames() const noexcept {
        return std::span<const CallStackFrame>(frames.data(), frame_count);
    }
};

/**
 * @brief Comprehensive allocation information for tracking
 */
struct TrackerAllocationInfo {
    // Basic allocation data
    void* address;                  // Allocated memory address
    usize size;                    // Requested size in bytes
    usize actual_size;             // Actual allocated size (including alignment)
    usize alignment;               // Required alignment
    
    // Categorization
    AllocationCategory category;    // Allocation category
    AllocatorType allocator_type;  // Which allocator was used
    const char* allocator_name;    // Human-readable allocator name
    u32 allocator_id;              // Unique allocator instance ID
    
    // Timing information
    f64 allocation_time;           // When allocated (seconds since start)
    f64 deallocation_time;         // When deallocated (0 if still active)
    f64 lifetime;                  // How long it lived (if deallocated)
    
    // Origin tracking
    CallStack call_stack;          // Where the allocation came from
    std::thread::id thread_id;     // Which thread allocated this
    const char* tag;               // Optional user-provided tag
    
    // Usage analysis
    AccessPattern access_pattern;   // Detected access pattern
    u64 access_count;              // Number of times accessed
    f64 last_access_time;          // Last access timestamp
    bool is_hot;                   // Frequently accessed
    
    // Status
    bool is_active;                // Still allocated
    bool is_leaked;                // Detected as leaked
    bool was_reallocated;          // Was reallocated at some point
    
    TrackerAllocationInfo() noexcept
        : address(nullptr), size(0), actual_size(0), alignment(0),
          category(AllocationCategory::Unknown), 
          allocator_type(AllocatorType::Unknown),
          allocator_name(nullptr), allocator_id(0),
          allocation_time(0.0), deallocation_time(0.0), lifetime(0.0),
          thread_id(std::this_thread::get_id()), tag(nullptr),
          access_pattern(AccessPattern::Unknown), access_count(0),
          last_access_time(0.0), is_hot(false),
          is_active(false), is_leaked(false), was_reallocated(false) {}
};

/**
 * @brief Memory allocation statistics by category
 */
struct CategoryStats {
    AllocationCategory category;
    
    // Size statistics
    usize total_allocated;          // Total bytes ever allocated
    usize current_allocated;        // Currently allocated bytes
    usize peak_allocated;           // Peak allocation in bytes
    usize average_allocation_size;  // Average allocation size
    usize min_allocation_size;      // Smallest allocation
    usize max_allocation_size;      // Largest allocation
    
    // Count statistics
    u64 total_allocations;          // Total number of allocations
    u64 current_allocations;        // Currently active allocations
    u64 peak_allocations;           // Peak number of allocations
    
    // Performance statistics
    f64 total_allocation_time;      // Total time spent allocating
    f64 average_allocation_time;    // Average allocation time
    f64 allocation_rate;            // Allocations per second
    
    // Waste analysis
    usize alignment_waste;          // Bytes lost to alignment
    usize fragmentation_waste;      // Bytes lost to fragmentation
    f64 waste_ratio;               // Percentage of allocated memory wasted
    
    // Access patterns
    std::array<u64, static_cast<usize>(AccessPattern::Hash) + 1> access_pattern_counts;
    
    CategoryStats() noexcept { reset(); }
    
    void reset() noexcept {
        category = AllocationCategory::Unknown;
        total_allocated = current_allocated = peak_allocated = 0;
        average_allocation_size = min_allocation_size = max_allocation_size = 0;
        total_allocations = current_allocations = peak_allocations = 0;
        total_allocation_time = average_allocation_time = allocation_rate = 0.0;
        alignment_waste = fragmentation_waste = 0;
        waste_ratio = 0.0;
        access_pattern_counts.fill(0);
    }
};

/**
 * @brief Allocation size distribution analysis
 */
struct SizeDistribution {
    static constexpr usize BUCKET_COUNT = 32;
    
    struct Bucket {
        usize min_size;             // Minimum size for this bucket
        usize max_size;             // Maximum size for this bucket
        u64 allocation_count;       // Number of allocations in this range
        usize total_bytes;          // Total bytes allocated in this range
        f64 percentage;             // Percentage of total allocations
    };
    
    std::array<Bucket, BUCKET_COUNT> buckets;
    u64 total_allocations;
    usize total_bytes;
    
    SizeDistribution() noexcept { reset(); }
    void reset() noexcept;
    void update_buckets() noexcept;
};

/**
 * @brief Memory allocation timeline for temporal analysis
 */
struct AllocationTimeline {
    struct TimeSlot {
        f64 start_time;             // Start of time slot
        f64 end_time;               // End of time slot
        u64 allocations;            // Allocations in this slot
        u64 deallocations;          // Deallocations in this slot
        usize bytes_allocated;      // Bytes allocated in this slot
        usize bytes_deallocated;    // Bytes deallocated in this slot
        usize peak_usage;           // Peak usage during this slot
    };
    
    static constexpr usize SLOT_COUNT = 1000;  // 1000 time slots for history
    
    std::array<TimeSlot, SLOT_COUNT> slots;
    usize current_slot;
    f64 slot_duration;              // Duration of each time slot
    f64 start_time;                 // When timeline started
    
    AllocationTimeline(f64 slot_duration = 0.1) noexcept;  // 100ms slots by default
    void reset() noexcept;
    void advance_time(f64 current_time) noexcept;
    void record_allocation(usize size) noexcept;
    void record_deallocation(usize size) noexcept;
    std::span<const TimeSlot> get_history() const noexcept;
};

/**
 * @brief Memory heat map for spatial analysis
 */
struct MemoryHeatMap {
    struct Region {
        void* start_address;        // Start of memory region
        usize size;                // Size of region
        u64 access_count;          // Number of accesses
        f64 last_access_time;      // Last access timestamp
        f64 temperature;           // Heat value (0.0 = cold, 1.0 = hot)
        AllocationCategory category; // What type of data lives here
    };
    
    std::vector<Region> regions;
    mutable std::shared_mutex regions_mutex;
    f64 cooling_rate;               // How fast regions cool down
    f64 last_update_time;           // Last heat map update
    
    MemoryHeatMap(f64 cooling_rate = 0.95) noexcept;
    void add_region(void* address, usize size, AllocationCategory category) noexcept;
    void remove_region(void* address) noexcept;
    void record_access(void* address) noexcept;
    void update_temperatures(f64 current_time) noexcept;
    std::vector<Region> get_hot_regions(f64 min_temperature = 0.5) const noexcept;
};

/**
 * @brief Memory pressure detection and analysis
 */
struct MemoryPressure {
    enum class Level : u8 {
        Low = 0,                    // Plenty of memory available
        Medium,                     // Moderate memory usage
        High,                       // High memory usage, may impact performance
        Critical                    // Very high usage, likely causing problems
    };
    
    Level current_level;
    f64 memory_usage_ratio;         // 0.0 to 1.0
    usize available_memory;         // Available memory in bytes
    usize total_memory;             // Total system memory
    u64 allocation_failures;        // Number of failed allocations
    f64 allocation_failure_rate;    // Failures per second
    bool thrashing_detected;        // Memory thrashing detected
    
    MemoryPressure() noexcept;
    void update(usize current_usage, usize total_available) noexcept;
    bool should_warn() const noexcept;
    const char* level_string() const noexcept;
};

/**
 * @brief Comprehensive memory tracking configuration
 */
struct TrackerConfig {
    bool enable_tracking;           // Master enable/disable
    bool enable_call_stacks;        // Capture call stacks (expensive)
    bool enable_access_tracking;    // Track memory access patterns
    bool enable_heat_mapping;       // Generate memory heat maps
    bool enable_leak_detection;     // Detect memory leaks
    bool enable_predictive_analysis; // Predict future memory needs
    
    // Performance tuning
    u32 max_tracked_allocations;    // Maximum number of tracked allocations
    f64 sampling_rate;              // Sample rate for high-frequency tracking
    f64 update_frequency;           // How often to update statistics
    usize call_stack_depth;        // Maximum call stack depth to capture
    
    // Filtering
    usize min_tracked_size;         // Don't track allocations smaller than this
    usize max_tracked_size;         // Don't track allocations larger than this
    std::unordered_set<AllocationCategory> ignored_categories;
    
    TrackerConfig() noexcept;
};

/**
 * @brief Global memory statistics summary
 */
struct GlobalStats {
    // Overall usage
    usize total_allocated;          // Total memory currently allocated
    usize peak_allocated;           // Peak memory usage
    usize total_allocations_ever;   // Total allocations since start
    u64 current_allocations;        // Currently active allocations
    
    // Performance metrics
    f64 total_allocation_time;      // Total time spent in allocations
    f64 average_allocation_time;    // Average allocation time
    f64 allocation_rate;            // Current allocations per second
    f64 memory_bandwidth;           // Estimated memory bandwidth usage
    
    // Quality metrics
    f64 fragmentation_ratio;        // Overall fragmentation (0.0 = none, 1.0 = severe)
    f64 waste_ratio;               // Percentage of allocated memory wasted
    u64 cache_miss_estimate;       // Estimated cache misses
    
    // By allocator type
    std::array<CategoryStats, static_cast<usize>(AllocationCategory::COUNT)> by_category;
    
    GlobalStats() noexcept { reset(); }
    void reset() noexcept;
};

/**
 * @brief Memory leak report entry
 */
struct LeakInfo {
    TrackerAllocationInfo allocation;      // Original allocation info
    f64 age;                       // How long it's been allocated
    bool is_confirmed_leak;        // Confirmed vs potential leak
    usize similar_leaks;           // Number of similar leak patterns
    f64 leak_score;               // Likelihood this is actually a leak (0.0-1.0)
};

/**
 * @brief Advanced Memory Tracker - Main tracking system
 * 
 * This class provides comprehensive memory tracking and analysis capabilities
 * for educational purposes. It integrates with all allocator types and provides
 * real-time insights into memory usage patterns.
 */
class MemoryTracker {
private:
    // Configuration
    TrackerConfig config_;
    
    // Global state
    mutable std::shared_mutex global_mutex_;
    std::atomic<bool> is_enabled_{true};
    std::atomic<u64> next_allocation_id_{1};
    f64 start_time_;
    
    // Allocation tracking
    std::unordered_map<void*, std::unique_ptr<TrackerAllocationInfo>> active_allocations_;
    mutable std::shared_mutex allocations_mutex_;
    
    // Statistics
    GlobalStats global_stats_;
    mutable std::mutex stats_mutex_;
    
    // Timeline and analysis
    std::unique_ptr<AllocationTimeline> timeline_;
    std::unique_ptr<MemoryHeatMap> heat_map_;
    std::unique_ptr<SizeDistribution> size_distribution_;
    std::unique_ptr<MemoryPressure> memory_pressure_;
    
    // Call stack utilities
    bool capture_call_stack(CallStack& stack) noexcept;
    u64 hash_call_stack(const CallStack& stack) noexcept;
    
    // Analysis utilities
    void update_statistics(const TrackerAllocationInfo& info, bool is_allocation) noexcept;
    AccessPattern detect_access_pattern(const TrackerAllocationInfo& info) noexcept;
    void update_size_distribution(usize size, bool is_allocation) noexcept;
    
    // Performance monitoring
    std::atomic<f64> last_performance_update_{0.0};
    void update_performance_metrics() noexcept;
    
    // Singleton management
    static std::unique_ptr<MemoryTracker> instance_;
    static std::once_flag init_flag_;
    
public:
    // Singleton access
    static MemoryTracker& instance() noexcept;
    static void initialize(const TrackerConfig& config = TrackerConfig{}) noexcept;
    static void shutdown() noexcept;
    
    // Core tracking interface
    void track_allocation(void* address, usize size, usize actual_size, usize alignment,
                         AllocationCategory category, AllocatorType allocator_type,
                         const char* allocator_name, u32 allocator_id,
                         const char* tag = nullptr) noexcept;
    
    void track_deallocation(void* address, AllocatorType allocator_type,
                           const char* allocator_name, u32 allocator_id) noexcept;
    
    void track_reallocation(void* old_address, void* new_address, usize old_size,
                           usize new_size, usize actual_size, usize alignment,
                           AllocationCategory category, AllocatorType allocator_type,
                           const char* allocator_name, u32 allocator_id,
                           const char* tag = nullptr) noexcept;
    
    // Access pattern tracking
    void track_memory_access(void* address, usize size, bool is_write) noexcept;
    
    // Configuration
    void set_config(const TrackerConfig& config) noexcept;
    TrackerConfig get_config() const noexcept;
    
    // Statistics retrieval
    GlobalStats get_global_stats() const noexcept;
    CategoryStats get_category_stats(AllocationCategory category) const noexcept;
    std::vector<CategoryStats> get_all_category_stats() const noexcept;
    
    // Analysis results
    SizeDistribution get_size_distribution() const noexcept;
    std::vector<AllocationTimeline::TimeSlot> get_allocation_timeline() const noexcept;
    std::vector<MemoryHeatMap::Region> get_memory_heat_map() const noexcept;
    MemoryPressure get_memory_pressure() const noexcept;
    
    // Leak detection
    std::vector<LeakInfo> detect_leaks(f64 min_age = 10.0, f64 min_score = 0.7) const noexcept;
    void mark_as_intentional_leak(void* address) noexcept;
    
    // Debugging and visualization
    std::vector<TrackerAllocationInfo> get_active_allocations() const noexcept;
    std::vector<TrackerAllocationInfo> get_allocations_by_category(AllocationCategory category) const noexcept;
    std::vector<TrackerAllocationInfo> get_allocations_by_size_range(usize min_size, usize max_size) const noexcept;
    std::vector<TrackerAllocationInfo> get_hot_allocations(u64 min_accesses = 100) const noexcept;
    
    // Performance analysis
    std::vector<std::pair<u64, f64>> get_allocation_hotspots() const noexcept;  // call stack hash -> time
    f64 estimate_cache_miss_rate() const noexcept;
    f64 estimate_memory_bandwidth_usage() const noexcept;
    
    // Predictive analysis
    usize predict_future_usage(f64 seconds_ahead) const noexcept;
    std::vector<AllocationCategory> predict_pressure_categories(f64 seconds_ahead) const noexcept;
    
    // Export capabilities
    void export_to_json(const std::string& filename) const noexcept;
    void export_timeline_csv(const std::string& filename) const noexcept;
    void export_heat_map_data(const std::string& filename) const noexcept;
    
    // Utility functions
    void reset_all_stats() noexcept;
    void force_garbage_collection() noexcept;  // Clean up old tracking data
    
    // Thread safety
    void enable_tracking() noexcept { is_enabled_.store(true); }
    void disable_tracking() noexcept { is_enabled_.store(false); }
    bool is_tracking_enabled() const noexcept { return is_enabled_.load(); }
    
    // Destructor (public for unique_ptr)
    ~MemoryTracker() noexcept;
    
private:
    MemoryTracker() noexcept;
    
    // Non-copyable, non-movable
    MemoryTracker(const MemoryTracker&) = delete;
    MemoryTracker& operator=(const MemoryTracker&) = delete;
    MemoryTracker(MemoryTracker&&) = delete;
    MemoryTracker& operator=(MemoryTracker&&) = delete;
};

/**
 * @brief RAII helper for tracking memory allocations with automatic cleanup
 */
class ScopedAllocationTracker {
private:
    void* address_;
    AllocatorType allocator_type_;
    const char* allocator_name_;
    u32 allocator_id_;
    bool should_track_;
    
public:
    ScopedAllocationTracker(void* address, usize size, usize actual_size, usize alignment,
                           AllocationCategory category, AllocatorType allocator_type,
                           const char* allocator_name, u32 allocator_id,
                           const char* tag = nullptr) noexcept;
    
    ~ScopedAllocationTracker() noexcept;
    
    // Non-copyable but movable
    ScopedAllocationTracker(const ScopedAllocationTracker&) = delete;
    ScopedAllocationTracker& operator=(const ScopedAllocationTracker&) = delete;
    
    ScopedAllocationTracker(ScopedAllocationTracker&& other) noexcept;
    ScopedAllocationTracker& operator=(ScopedAllocationTracker&& other) noexcept;
    
    void* address() const noexcept { return address_; }
    void release() noexcept { should_track_ = false; }
};

// Convenience functions for integration with allocators
namespace tracker {
    /**
     * @brief Track an allocation from any allocator
     */
    inline void track_alloc(void* address, usize size, usize actual_size, usize alignment,
                           AllocationCategory category, AllocatorType allocator_type,
                           const char* allocator_name, u32 allocator_id,
                           const char* tag = nullptr) noexcept {
        MemoryTracker::instance().track_allocation(address, size, actual_size, alignment,
                                                  category, allocator_type, allocator_name,
                                                  allocator_id, tag);
    }
    
    /**
     * @brief Track a deallocation from any allocator
     */
    inline void track_dealloc(void* address, AllocatorType allocator_type,
                             const char* allocator_name, u32 allocator_id) noexcept {
        MemoryTracker::instance().track_deallocation(address, allocator_type,
                                                    allocator_name, allocator_id);
    }
    
    /**
     * @brief Track memory access for pattern analysis
     */
    inline void track_access(void* address, usize size, bool is_write = false) noexcept {
        MemoryTracker::instance().track_memory_access(address, size, is_write);
    }
    
    /**
     * @brief Get current memory pressure level
     */
    inline MemoryPressure::Level get_pressure_level() noexcept {
        return MemoryTracker::instance().get_memory_pressure().current_level;
    }
    
    /**
     * @brief Check if memory tracking is enabled
     */
    inline bool is_enabled() noexcept {
        return MemoryTracker::instance().is_tracking_enabled();
    }
}

// Utility functions for category management
const char* category_name(AllocationCategory category) noexcept;
AllocationCategory category_from_string(const char* name) noexcept;
const char* allocator_type_name(AllocatorType type) noexcept;
const char* access_pattern_name(AccessPattern pattern) noexcept;

// Performance macros for conditional tracking
#ifdef ECSCOPE_ENABLE_MEMORY_TRACKING
    #define TRACK_ALLOCATION(addr, size, actual, align, cat, type, name, id, tag) \
        tracker::track_alloc(addr, size, actual, align, cat, type, name, id, tag)
    
    #define TRACK_DEALLOCATION(addr, type, name, id) \
        tracker::track_dealloc(addr, type, name, id)
    
    #define TRACK_ACCESS(addr, size, write) \
        tracker::track_access(addr, size, write)
    
    #define SCOPED_ALLOCATION_TRACKER(addr, size, actual, align, cat, type, name, id, tag) \
        ScopedAllocationTracker _scoped_tracker(addr, size, actual, align, cat, type, name, id, tag)
#else
    #define TRACK_ALLOCATION(addr, size, actual, align, cat, type, name, id, tag) ((void)0)
    #define TRACK_DEALLOCATION(addr, type, name, id) ((void)0)
    #define TRACK_ACCESS(addr, size, write) ((void)0)
    #define SCOPED_ALLOCATION_TRACKER(addr, size, actual, align, cat, type, name, id, tag) ((void)0)
#endif

} // namespace ecscope::memory