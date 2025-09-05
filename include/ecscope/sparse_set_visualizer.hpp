#pragma once

/**
 * @file sparse_set_visualizer.hpp
 * @brief Comprehensive Sparse Set Visualization and Analysis for ECScope
 * 
 * This module provides detailed visualization and educational analysis of sparse set
 * data structures used in ECS component storage. It demonstrates the memory layout,
 * access patterns, and cache performance characteristics of sparse sets.
 * 
 * Educational Features:
 * - Visual representation of dense and sparse arrays
 * - Cache locality analysis and visualization
 * - Memory access pattern tracking
 * - Performance metrics for different access patterns
 * - Interactive exploration of sparse set internals
 * - Real-time memory usage monitoring
 * 
 * Key Concepts Demonstrated:
 * - Sparse set data structure principles
 * - Cache-friendly memory layouts
 * - O(1) insertion, deletion, and lookup operations
 * - Memory fragmentation analysis
 * - Spatial and temporal locality patterns
 * 
 * @author ECScope Educational ECS Framework
 * @date 2024
 */

#include "core/types.hpp"
#include "core/log.hpp"
#include "memory/memory_tracker.hpp"
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <atomic>
#include <mutex>
#include <array>
#include <chrono>

namespace ecscope::visualization {

/**
 * @brief Sparse set access pattern for analysis
 */
enum class SparseSetAccessPattern : u8 {
    Sequential,     // Sequential iteration through dense array
    Random,         // Random access by entity ID
    Bulk_Insert,    // Bulk insertion operations
    Bulk_Remove,    // Bulk removal operations
    Mixed          // Mixed access patterns
};

/**
 * @brief Memory access tracking data
 */
struct AccessTrackingData {
    u64 read_count;                     // Number of read operations
    u64 write_count;                    // Number of write operations
    u64 cache_hits;                     // Estimated cache hits
    u64 cache_misses;                   // Estimated cache misses
    f64 last_access_time;              // Last access timestamp
    f64 access_frequency;              // Accesses per second
    SparseSetAccessPattern pattern;    // Detected access pattern
    
    AccessTrackingData() noexcept
        : read_count(0), write_count(0), cache_hits(0), cache_misses(0)
        , last_access_time(0.0), access_frequency(0.0)
        , pattern(SparseSetAccessPattern::Sequential) {}
};

/**
 * @brief Visualization data for a single sparse set
 */
struct SparseSetVisualizationData {
    std::string name;                   // Name identifier
    usize dense_capacity;               // Dense array capacity
    usize dense_size;                   // Current dense array size
    usize sparse_capacity;              // Sparse array capacity
    
    // Visual representation arrays
    std::vector<bool> dense_occupied;   // Dense array occupancy map
    std::vector<bool> sparse_valid;     // Sparse array validity map
    std::vector<u32> sparse_to_dense;   // Sparse to dense mapping
    std::vector<u32> dense_to_sparse;   // Dense to sparse mapping
    
    // Memory usage information
    usize memory_dense;                 // Memory used by dense array
    usize memory_sparse;                // Memory used by sparse array
    usize memory_total;                 // Total memory usage
    f64 memory_efficiency;              // Memory efficiency ratio
    
    // Performance metrics
    AccessTrackingData access_tracking; // Access pattern tracking
    f64 insertion_time_avg;             // Average insertion time (μs)
    f64 removal_time_avg;               // Average removal time (μs)
    f64 lookup_time_avg;                // Average lookup time (μs)
    f64 iteration_time_avg;             // Average iteration time (μs)
    
    // Cache analysis
    f64 cache_locality_score;           // Cache locality score (0.0-1.0)
    f64 spatial_locality;               // Spatial locality measure
    f64 temporal_locality;              // Temporal locality measure
    usize cache_line_utilization;       // Estimated cache lines used
    
    // Educational insights
    std::vector<std::string> optimization_suggestions;
    std::vector<std::string> performance_insights;
    
    SparseSetVisualizationData() noexcept
        : dense_capacity(0), dense_size(0), sparse_capacity(0)
        , memory_dense(0), memory_sparse(0), memory_total(0)
        , memory_efficiency(0.0), insertion_time_avg(0.0)
        , removal_time_avg(0.0), lookup_time_avg(0.0)
        , iteration_time_avg(0.0), cache_locality_score(0.0)
        , spatial_locality(0.0), temporal_locality(0.0)
        , cache_line_utilization(0) {}
};

/**
 * @brief Comprehensive sparse set analyzer and visualizer
 */
class SparseSetAnalyzer {
private:
    // Analysis data
    std::unordered_map<std::string, std::unique_ptr<SparseSetVisualizationData>> sparse_sets_;
    mutable std::shared_mutex data_mutex_;
    
    // Analysis configuration
    bool enable_access_tracking_;       // Enable detailed access tracking
    bool enable_cache_analysis_;        // Enable cache behavior analysis
    f64 analysis_frequency_;            // Analysis update frequency
    f64 last_analysis_time_;           // Last analysis timestamp
    
    // Performance monitoring
    std::chrono::high_resolution_clock::time_point start_time_;
    std::atomic<u64> total_operations_{0};
    std::atomic<u64> cache_friendly_operations_{0};
    
    // Cache simulation parameters
    static constexpr usize CACHE_LINE_SIZE = 64;  // Typical cache line size
    static constexpr usize L1_CACHE_SIZE = 32768; // 32KB L1 cache
    static constexpr usize L2_CACHE_SIZE = 262144; // 256KB L2 cache
    
public:
    /**
     * @brief Constructor
     */
    explicit SparseSetAnalyzer() noexcept;
    
    /**
     * @brief Destructor
     */
    ~SparseSetAnalyzer() noexcept = default;
    
    // Registration and management
    void register_sparse_set(const std::string& name, usize initial_capacity = 1024) noexcept;
    void unregister_sparse_set(const std::string& name) noexcept;
    bool has_sparse_set(const std::string& name) const noexcept;
    
    // Data update interface
    void update_sparse_set_data(const std::string& name, usize dense_size, 
                               usize sparse_capacity, const void* dense_ptr, 
                               const void* sparse_ptr) noexcept;
    
    // Access tracking interface
    void track_access(const std::string& name, void* address, usize size, 
                     bool is_write, SparseSetAccessPattern pattern) noexcept;
    void track_insertion(const std::string& name, f64 duration_us) noexcept;
    void track_removal(const std::string& name, f64 duration_us) noexcept;
    void track_lookup(const std::string& name, f64 duration_us) noexcept;
    void track_iteration(const std::string& name, f64 duration_us) noexcept;
    
    // Analysis functions
    void analyze_all() noexcept;
    void analyze_sparse_set(const std::string& name) noexcept;
    void analyze_cache_behavior(const std::string& name) noexcept;
    void analyze_memory_patterns(const std::string& name) noexcept;
    void generate_optimization_suggestions(const std::string& name) noexcept;
    
    // Data access
    std::vector<std::string> get_sparse_set_names() const noexcept;
    const SparseSetVisualizationData* get_sparse_set_data(const std::string& name) const noexcept;
    std::vector<const SparseSetVisualizationData*> get_all_sparse_sets() const noexcept;
    
    // Summary statistics
    usize get_total_sparse_sets() const noexcept;
    usize get_total_memory_usage() const noexcept;
    f64 get_average_cache_locality() const noexcept;
    f64 get_overall_efficiency() const noexcept;
    
    // Configuration
    void set_access_tracking(bool enabled) noexcept { enable_access_tracking_ = enabled; }
    void set_cache_analysis(bool enabled) noexcept { enable_cache_analysis_ = enabled; }
    void set_analysis_frequency(f64 frequency) noexcept { analysis_frequency_ = frequency; }
    
    bool is_access_tracking_enabled() const noexcept { return enable_access_tracking_; }
    bool is_cache_analysis_enabled() const noexcept { return enable_cache_analysis_; }
    f64 get_analysis_frequency() const noexcept { return analysis_frequency_; }
    
    // Educational insights
    std::vector<std::string> get_educational_insights() const noexcept;
    std::string explain_sparse_set_concept() const noexcept;
    std::string explain_cache_locality() const noexcept;
    std::string suggest_performance_improvements() const noexcept;
    
    // Export functionality
    void export_analysis_report(const std::string& filename) const noexcept;
    void export_performance_data(const std::string& filename) const noexcept;
    void export_cache_analysis(const std::string& filename) const noexcept;
    
private:
    // Analysis implementation
    void calculate_memory_efficiency(SparseSetVisualizationData& data) noexcept;
    void calculate_cache_metrics(SparseSetVisualizationData& data) noexcept;
    void detect_access_patterns(SparseSetVisualizationData& data) noexcept;
    void generate_educational_content(SparseSetVisualizationData& data) noexcept;
    
    // Cache simulation
    bool would_cause_cache_miss(void* address, usize size) const noexcept;
    f64 estimate_cache_locality(const std::vector<void*>& access_sequence) const noexcept;
    usize calculate_cache_lines_used(void* start_addr, usize size) const noexcept;
    
    // Performance analysis
    void update_performance_averages(SparseSetVisualizationData& data, 
                                   f64 new_value, f64& average, u64& count) noexcept;
    SparseSetAccessPattern detect_pattern_from_sequence(
        const std::vector<void*>& recent_accesses) const noexcept;
    
    // Utility functions
    f64 get_current_time() const noexcept;
    std::string format_memory_size(usize bytes) const noexcept;
    std::string format_time_duration(f64 microseconds) const noexcept;
    
    // Simple cache simulation state
    mutable std::array<uintptr_t, 1024> simulated_cache_; // Simple cache simulation
    mutable usize cache_index_;
    
    // Constants
    static constexpr f64 DEFAULT_ANALYSIS_FREQUENCY = 5.0; // 5 Hz
    static constexpr usize MAX_ACCESS_HISTORY = 1000;      // Track last 1000 accesses
    static constexpr f64 CACHE_HIT_THRESHOLD = 0.8;       // 80% hit rate threshold
};

/**
 * @brief RAII helper for automatic sparse set operation tracking
 */
class SparseSetOperationTracker {
private:
    SparseSetAnalyzer* analyzer_;
    std::string name_;
    std::chrono::high_resolution_clock::time_point start_time_;
    enum class OperationType { Insert, Remove, Lookup, Iterate } operation_type_;
    
public:
    SparseSetOperationTracker(SparseSetAnalyzer* analyzer, const std::string& name, 
                             OperationType type) noexcept
        : analyzer_(analyzer), name_(name), operation_type_(type)
        , start_time_(std::chrono::high_resolution_clock::now()) {}
    
    ~SparseSetOperationTracker() noexcept {
        if (analyzer_) {
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<f64, std::micro>(end_time - start_time_).count();
            
            switch (operation_type_) {
                case OperationType::Insert:
                    analyzer_->track_insertion(name_, duration);
                    break;
                case OperationType::Remove:
                    analyzer_->track_removal(name_, duration);
                    break;
                case OperationType::Lookup:
                    analyzer_->track_lookup(name_, duration);
                    break;
                case OperationType::Iterate:
                    analyzer_->track_iteration(name_, duration);
                    break;
            }
        }
    }
    
    // Non-copyable, non-movable
    SparseSetOperationTracker(const SparseSetOperationTracker&) = delete;
    SparseSetOperationTracker& operator=(const SparseSetOperationTracker&) = delete;
    SparseSetOperationTracker(SparseSetOperationTracker&&) = delete;
    SparseSetOperationTracker& operator=(SparseSetOperationTracker&&) = delete;
    
    // Factory methods for different operation types
    static SparseSetOperationTracker track_insert(SparseSetAnalyzer* analyzer, const std::string& name) {
        return SparseSetOperationTracker(analyzer, name, OperationType::Insert);
    }
    
    static SparseSetOperationTracker track_remove(SparseSetAnalyzer* analyzer, const std::string& name) {
        return SparseSetOperationTracker(analyzer, name, OperationType::Remove);
    }
    
    static SparseSetOperationTracker track_lookup(SparseSetAnalyzer* analyzer, const std::string& name) {
        return SparseSetOperationTracker(analyzer, name, OperationType::Lookup);
    }
    
    static SparseSetOperationTracker track_iterate(SparseSetAnalyzer* analyzer, const std::string& name) {
        return SparseSetOperationTracker(analyzer, name, OperationType::Iterate);
    }
};

/**
 * @brief Global sparse set analyzer instance
 */
class GlobalSparseSetAnalyzer {
private:
    static std::unique_ptr<SparseSetAnalyzer> instance_;
    static std::once_flag init_flag_;
    
public:
    static SparseSetAnalyzer& instance() noexcept {
        std::call_once(init_flag_, []() {
            instance_ = std::make_unique<SparseSetAnalyzer>();
        });
        return *instance_;
    }
    
    static void initialize() noexcept {
        instance(); // Force initialization
    }
    
    static void shutdown() noexcept {
        instance_.reset();
    }
};

// Convenience macros for operation tracking
#ifdef ECSCOPE_ENABLE_SPARSE_SET_ANALYSIS
    #define TRACK_SPARSE_SET_INSERT(name) \
        auto _sparse_tracker = SparseSetOperationTracker::track_insert(&GlobalSparseSetAnalyzer::instance(), name)
    
    #define TRACK_SPARSE_SET_REMOVE(name) \
        auto _sparse_tracker = SparseSetOperationTracker::track_remove(&GlobalSparseSetAnalyzer::instance(), name)
    
    #define TRACK_SPARSE_SET_LOOKUP(name) \
        auto _sparse_tracker = SparseSetOperationTracker::track_lookup(&GlobalSparseSetAnalyzer::instance(), name)
    
    #define TRACK_SPARSE_SET_ITERATE(name) \
        auto _sparse_tracker = SparseSetOperationTracker::track_iterate(&GlobalSparseSetAnalyzer::instance(), name)
    
    #define TRACK_SPARSE_SET_ACCESS(name, addr, size, write, pattern) \
        GlobalSparseSetAnalyzer::instance().track_access(name, addr, size, write, pattern)
#else
    #define TRACK_SPARSE_SET_INSERT(name) ((void)0)
    #define TRACK_SPARSE_SET_REMOVE(name) ((void)0)
    #define TRACK_SPARSE_SET_LOOKUP(name) ((void)0)
    #define TRACK_SPARSE_SET_ITERATE(name) ((void)0)
    #define TRACK_SPARSE_SET_ACCESS(name, addr, size, write, pattern) ((void)0)
#endif

} // namespace ecscope::visualization