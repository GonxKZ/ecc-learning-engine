#pragma once

/**
 * @file profiling.hpp
 * @brief Professional-grade performance profiling and monitoring system for system scheduling
 * 
 * This comprehensive profiling system provides world-class performance monitoring, analysis,
 * and optimization tools for the advanced system scheduler with the following features:
 * 
 * - High-resolution timing with nanosecond precision
 * - Multi-threaded performance data collection
 * - CPU instruction counting and analysis
 * - Memory usage tracking and leak detection
 * - Cache performance monitoring (hits/misses)
 * - NUMA locality analysis
 * - System dependency timing analysis
 * - Real-time performance visualization
 * - Automated bottleneck detection
 * - Performance regression analysis
 * - Statistical performance analysis
 * - Export capabilities for external tools
 * 
 * Profiling Features:
 * - Frame-based performance analysis
 * - System-level granular timing
 * - Resource contention detection
 * - Thread utilization analysis
 * - Load balancing effectiveness
 * - Scheduling overhead measurement
 * - Critical path identification
 * - Performance budget tracking
 * 
 * Analysis Capabilities:
 * - Trend analysis and prediction
 * - Performance anomaly detection
 * - Comparative analysis between frames
 * - System performance ranking
 * - Resource utilization optimization
 * - Scheduling efficiency metrics
 * 
 * @author ECScope Professional ECS Framework
 * @date 2024
 */

#include "../core/types.hpp"
#include "../core/log.hpp"
#include "../core/time.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <chrono>
#include <thread>
#include <functional>
#include <queue>
#include <fstream>
#include <algorithm>
#include <numeric>
#include <cmath>

#ifdef _WIN32
    #include <windows.h>
    #include <psapi.h>
    #include <intrin.h>
#elif defined(__linux__)
    #include <unistd.h>
    #include <sys/resource.h>
    #include <sys/time.h>
    #include <linux/perf_event.h>
    #include <asm/unistd.h>
    #include <x86intrin.h>
#endif

namespace ecscope::scheduling {

// Forward declarations
class System;
class ExecutionContext;

using ecscope::core::u8;
using ecscope::core::u16;
using ecscope::core::u32;
using ecscope::core::u64;
using ecscope::core::i64;
using ecscope::core::f32;
using ecscope::core::f64;
using ecscope::core::usize;

/**
 * @brief Performance metric types for different kinds of measurements
 */
enum class MetricType : u8 {
    Timer = 0,              // Time-based measurements
    Counter,                // Simple counters (executions, calls, etc.)
    Gauge,                  // Current value measurements (memory usage, etc.)
    Histogram,              // Distribution of values
    Rate,                   // Rate of change over time
    Percentage,             // Percentage values (utilization, etc.)
    Memory,                 // Memory-related metrics
    Cache,                  // Cache performance metrics
    Thread,                 // Thread-specific metrics
    System                  // System-wide metrics
};

/**
 * @brief Performance data sample with comprehensive timing information
 */
struct PerformanceSample {
    u64 timestamp_ns;                    // High-resolution timestamp
    u32 system_id;                       // System identifier
    u32 thread_id;                       // Thread identifier
    u32 numa_node;                       // NUMA node
    
    // Timing metrics
    f64 execution_time_ns;               // System execution time
    f64 wait_time_ns;                    // Time waiting for dependencies/resources
    f64 scheduling_overhead_ns;          // Scheduling overhead
    
    // CPU metrics
    u64 cpu_cycles;                      // CPU cycles consumed
    u64 instructions_executed;           // Instructions executed
    f64 instructions_per_cycle;          // IPC (Instructions Per Cycle)
    f64 cpu_utilization_percent;         // CPU utilization percentage
    
    // Memory metrics
    u64 memory_allocated_bytes;          // Memory allocated
    u64 memory_freed_bytes;              // Memory freed
    u64 peak_memory_usage_bytes;         // Peak memory usage during execution
    u32 memory_allocations;              // Number of allocations
    u32 memory_deallocations;            // Number of deallocations
    
    // Cache metrics
    u64 cache_references;                // Cache references
    u64 cache_misses;                    // Cache misses
    f64 cache_hit_rate_percent;          // Cache hit rate
    u64 l1_cache_misses;                 // L1 cache misses
    u64 l2_cache_misses;                 // L2 cache misses
    u64 l3_cache_misses;                 // L3 cache misses
    
    // System metrics
    u32 context_switches;                // Context switches
    u32 page_faults;                     // Page faults
    u32 system_calls;                    // System calls made
    f64 load_average;                    // System load average
    
    // Dependency metrics
    u32 dependencies_satisfied;         // Dependencies satisfied
    f64 dependency_wait_time_ns;         // Time waiting for dependencies
    u32 resources_acquired;              // Resources acquired
    f64 resource_contention_time_ns;     // Time in resource contention
    
    PerformanceSample() 
        : timestamp_ns(get_current_time_ns()), system_id(0), thread_id(0), numa_node(0)
        , execution_time_ns(0.0), wait_time_ns(0.0), scheduling_overhead_ns(0.0)
        , cpu_cycles(0), instructions_executed(0), instructions_per_cycle(0.0)
        , cpu_utilization_percent(0.0), memory_allocated_bytes(0), memory_freed_bytes(0)
        , peak_memory_usage_bytes(0), memory_allocations(0), memory_deallocations(0)
        , cache_references(0), cache_misses(0), cache_hit_rate_percent(0.0)
        , l1_cache_misses(0), l2_cache_misses(0), l3_cache_misses(0)
        , context_switches(0), page_faults(0), system_calls(0), load_average(0.0)
        , dependencies_satisfied(0), dependency_wait_time_ns(0.0)
        , resources_acquired(0), resource_contention_time_ns(0.0) {}
    
    void calculate_derived_metrics() {
        if (cpu_cycles > 0) {
            instructions_per_cycle = static_cast<f64>(instructions_executed) / cpu_cycles;
        }
        
        if (cache_references > 0) {
            cache_hit_rate_percent = 
                100.0 * (1.0 - (static_cast<f64>(cache_misses) / cache_references));
        }
    }
    
    bool is_valid() const {
        return timestamp_ns > 0 && system_id > 0;
    }
    
private:
    static u64 get_current_time_ns() {
        return std::chrono::high_resolution_clock::now().time_since_epoch().count();
    }
};

/**
 * @brief Aggregated performance statistics with statistical analysis
 */
struct PerformanceStatistics {
    // Basic statistics
    u64 sample_count = 0;
    f64 min_value = std::numeric_limits<f64>::max();
    f64 max_value = std::numeric_limits<f64>::lowest();
    f64 mean_value = 0.0;
    f64 median_value = 0.0;
    f64 sum_value = 0.0;
    
    // Advanced statistics
    f64 standard_deviation = 0.0;
    f64 variance = 0.0;
    f64 percentile_95 = 0.0;
    f64 percentile_99 = 0.0;
    f64 skewness = 0.0;              // Distribution skewness
    f64 kurtosis = 0.0;              // Distribution kurtosis
    
    // Time series analysis
    f64 trend_slope = 0.0;           // Linear trend slope
    f64 trend_correlation = 0.0;     // Trend correlation coefficient
    f64 volatility = 0.0;            // Value volatility
    f64 autocorrelation = 0.0;       // Autocorrelation
    
    // Performance analysis
    f64 efficiency_score = 0.0;      // Overall efficiency score (0-100)
    f64 stability_score = 0.0;       // Stability score (0-100)
    f64 performance_index = 0.0;     // Composite performance index
    
    PerformanceStatistics() = default;
    
    void update_with_sample(f64 value) {
        sample_count++;
        sum_value += value;
        
        if (value < min_value) min_value = value;
        if (value > max_value) max_value = value;
        
        // Update running mean
        mean_value = sum_value / sample_count;
        
        // Calculate variance and standard deviation incrementally
        if (sample_count > 1) {
            f64 delta = value - mean_value;
            variance = ((sample_count - 2) * variance + delta * delta) / (sample_count - 1);
            standard_deviation = std::sqrt(variance);
        }
    }
    
    void finalize_statistics(const std::vector<f64>& sorted_values) {
        if (sorted_values.empty()) return;
        
        // Calculate percentiles
        usize n = sorted_values.size();
        if (n > 0) {
            median_value = n % 2 == 0 ? 
                (sorted_values[n/2 - 1] + sorted_values[n/2]) / 2.0 :
                sorted_values[n/2];
                
            percentile_95 = sorted_values[static_cast<usize>(0.95 * n)];
            percentile_99 = sorted_values[static_cast<usize>(0.99 * n)];
        }
        
        // Calculate advanced statistics
        calculate_advanced_statistics(sorted_values);
        
        // Calculate performance scores
        calculate_performance_scores();
    }
    
    bool is_anomalous_value(f64 value, f64 z_score_threshold = 3.0) const {
        if (standard_deviation == 0.0) return false;
        f64 z_score = std::abs(value - mean_value) / standard_deviation;
        return z_score > z_score_threshold;
    }
    
    f64 get_coefficient_of_variation() const {
        return mean_value != 0.0 ? (standard_deviation / mean_value) * 100.0 : 0.0;
    }

private:
    void calculate_advanced_statistics(const std::vector<f64>& values) {
        if (values.size() < 3 || standard_deviation == 0.0) return;
        
        f64 sum_cubed_deviations = 0.0;
        f64 sum_fourth_deviations = 0.0;
        
        for (f64 value : values) {
            f64 deviation = value - mean_value;
            f64 normalized_deviation = deviation / standard_deviation;
            sum_cubed_deviations += normalized_deviation * normalized_deviation * normalized_deviation;
            sum_fourth_deviations += normalized_deviation * normalized_deviation * 
                                   normalized_deviation * normalized_deviation;
        }
        
        // Skewness and kurtosis
        skewness = sum_cubed_deviations / values.size();
        kurtosis = (sum_fourth_deviations / values.size()) - 3.0;  // Excess kurtosis
        
        // Volatility (coefficient of variation)
        volatility = get_coefficient_of_variation();
        
        // Calculate trend if we have time series data
        calculate_trend_analysis(values);
    }
    
    void calculate_trend_analysis(const std::vector<f64>& values) {
        if (values.size() < 5) return;  // Need minimum samples for trend analysis
        
        // Linear regression for trend
        usize n = values.size();
        f64 sum_x = 0.0, sum_y = 0.0, sum_xy = 0.0, sum_x2 = 0.0;
        
        for (usize i = 0; i < n; ++i) {
            f64 x = static_cast<f64>(i);
            f64 y = values[i];
            sum_x += x;
            sum_y += y;
            sum_xy += x * y;
            sum_x2 += x * x;
        }
        
        f64 slope_numerator = n * sum_xy - sum_x * sum_y;
        f64 slope_denominator = n * sum_x2 - sum_x * sum_x;
        
        if (slope_denominator != 0.0) {
            trend_slope = slope_numerator / slope_denominator;
            
            // Calculate correlation coefficient
            f64 mean_x = sum_x / n;
            f64 mean_y = sum_y / n;
            f64 sum_x_dev_sq = 0.0, sum_y_dev_sq = 0.0, sum_xy_dev = 0.0;
            
            for (usize i = 0; i < n; ++i) {
                f64 x_dev = static_cast<f64>(i) - mean_x;
                f64 y_dev = values[i] - mean_y;
                sum_x_dev_sq += x_dev * x_dev;
                sum_y_dev_sq += y_dev * y_dev;
                sum_xy_dev += x_dev * y_dev;
            }
            
            f64 correlation_denominator = std::sqrt(sum_x_dev_sq * sum_y_dev_sq);
            if (correlation_denominator != 0.0) {
                trend_correlation = sum_xy_dev / correlation_denominator;
            }
        }
    }
    
    void calculate_performance_scores() {
        // Efficiency score based on mean performance relative to theoretical optimum
        // (This would need domain-specific knowledge of what constitutes optimal performance)
        efficiency_score = std::max(0.0, std::min(100.0, 100.0 - (get_coefficient_of_variation() / 2.0)));
        
        // Stability score based on low variance and outliers
        f64 outlier_penalty = (percentile_99 - percentile_95) / mean_value * 100.0;
        stability_score = std::max(0.0, std::min(100.0, 100.0 - outlier_penalty));
        
        // Composite performance index
        performance_index = (efficiency_score + stability_score) / 2.0;
    }
};

/**
 * @brief Performance profile for a system with comprehensive metrics
 */
class SystemProfile {
private:
    u32 system_id_;
    std::string system_name_;
    
    // Sample storage
    std::vector<PerformanceSample> samples_;
    mutable std::shared_mutex samples_mutex_;
    
    // Aggregated statistics for different metrics
    std::unordered_map<std::string, PerformanceStatistics> metric_statistics_;
    mutable std::shared_mutex statistics_mutex_;
    
    // Configuration
    usize max_samples_;
    bool auto_calculate_statistics_;
    f64 sample_retention_time_;  // Seconds to keep samples
    
    // Cache for frequently accessed statistics
    mutable std::unordered_map<std::string, f64> cached_values_;
    mutable std::chrono::steady_clock::time_point last_cache_update_;
    mutable std::mutex cache_mutex_;

public:
    SystemProfile(u32 system_id, const std::string& name, usize max_samples = 10000);
    
    // Sample management
    void add_sample(const PerformanceSample& sample);
    void add_samples(const std::vector<PerformanceSample>& samples);
    std::vector<PerformanceSample> get_recent_samples(usize count) const;
    std::vector<PerformanceSample> get_samples_in_range(u64 start_time_ns, u64 end_time_ns) const;
    usize sample_count() const;
    void clear_samples();
    void trim_old_samples(f64 max_age_seconds);
    
    // Statistics
    PerformanceStatistics get_metric_statistics(const std::string& metric_name) const;
    std::unordered_map<std::string, PerformanceStatistics> get_all_statistics() const;
    void recalculate_statistics();
    
    // Analysis
    std::vector<PerformanceSample> detect_anomalies(f64 z_score_threshold = 3.0) const;
    f64 get_trend_slope(const std::string& metric_name) const;
    f64 get_performance_stability() const;
    f64 get_overall_performance_score() const;
    
    // Comparison
    f64 compare_with_baseline(const SystemProfile& baseline, const std::string& metric) const;
    bool has_performance_regression(const SystemProfile& baseline, f64 threshold_percent = 10.0) const;
    std::vector<std::string> get_regression_metrics(const SystemProfile& baseline, f64 threshold = 10.0) const;
    
    // Optimization suggestions
    std::vector<std::string> suggest_optimizations() const;
    std::vector<std::pair<std::string, f64>> identify_bottlenecks() const;
    
    // Export and reporting
    std::string generate_report() const;
    void export_csv(const std::string& filename) const;
    void export_json(const std::string& filename) const;
    
    // Configuration
    void set_max_samples(usize max_samples) { max_samples_ = max_samples; }
    void set_auto_calculate_statistics(bool enabled) { auto_calculate_statistics_ = enabled; }
    void set_sample_retention_time(f64 seconds) { sample_retention_time_ = seconds; }
    
    // Information
    u32 system_id() const { return system_id_; }
    const std::string& system_name() const { return system_name_; }
    f64 get_average_execution_time() const;
    f64 get_peak_memory_usage() const;
    f64 get_cache_efficiency() const;

private:
    void update_metric_statistics(const std::string& metric_name, f64 value);
    void cleanup_old_samples();
    void update_cache() const;
    f64 get_cached_value(const std::string& key) const;
    void set_cached_value(const std::string& key, f64 value) const;
};

/**
 * @brief Performance data collector with multi-threaded collection capabilities
 */
class PerformanceCollector {
private:
    // Collection state
    std::atomic<bool> collecting_;
    std::atomic<bool> paused_;
    
    // Thread management
    std::vector<std::thread> collector_threads_;
    std::atomic<u32> active_collectors_;
    
    // Data collection
    std::queue<PerformanceSample> sample_queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_condition_;
    
    // System profiles
    std::unordered_map<u32, std::unique_ptr<SystemProfile>> system_profiles_;
    mutable std::shared_mutex profiles_mutex_;
    
    // Hardware performance counters (platform-specific)
    struct HardwareCounters {
        int cpu_cycles_fd = -1;
        int instructions_fd = -1;
        int cache_references_fd = -1;
        int cache_misses_fd = -1;
        int branch_instructions_fd = -1;
        int branch_misses_fd = -1;
    };
    std::unordered_map<std::thread::id, HardwareCounters> thread_counters_;
    mutable std::mutex counters_mutex_;
    
    // Collection configuration
    f64 collection_frequency_hz_;
    bool collect_hardware_counters_;
    bool collect_memory_stats_;
    bool collect_system_stats_;
    usize max_queue_size_;
    
    // Statistics
    std::atomic<u64> samples_collected_;
    std::atomic<u64> samples_dropped_;
    std::atomic<u64> collection_errors_;

public:
    PerformanceCollector(f64 frequency_hz = 1000.0, usize max_queue_size = 100000);
    ~PerformanceCollector();
    
    // Collection control
    void start_collection(u32 num_threads = 1);
    void stop_collection();
    void pause_collection();
    void resume_collection();
    bool is_collecting() const { return collecting_.load(); }
    bool is_paused() const { return paused_.load(); }
    
    // Sample collection
    void collect_sample(u32 system_id, const std::string& system_name, 
                       const ExecutionContext& context);
    void collect_system_sample(System* system, const ExecutionContext& context);
    void force_collection_update();
    
    // Profile management
    SystemProfile* get_system_profile(u32 system_id) const;
    SystemProfile* get_system_profile(const std::string& system_name) const;
    std::vector<SystemProfile*> get_all_profiles() const;
    void clear_all_profiles();
    
    // Analysis
    std::vector<std::pair<std::string, f64>> get_top_performers(usize count = 10) const;
    std::vector<std::pair<std::string, f64>> get_bottlenecks(usize count = 10) const;
    f64 get_overall_system_efficiency() const;
    std::string generate_system_report() const;
    
    // Configuration
    void set_collection_frequency(f64 frequency_hz) { collection_frequency_hz_ = frequency_hz; }
    void set_hardware_counters_enabled(bool enabled) { collect_hardware_counters_ = enabled; }
    void set_memory_stats_enabled(bool enabled) { collect_memory_stats_ = enabled; }
    void set_system_stats_enabled(bool enabled) { collect_system_stats_ = enabled; }
    void set_max_queue_size(usize max_size) { max_queue_size_ = max_size; }
    
    // Statistics
    struct CollectorStatistics {
        u64 samples_collected;
        u64 samples_dropped;
        u64 collection_errors;
        f64 collection_rate_hz;
        f64 queue_utilization_percent;
        u32 active_collectors;
        usize total_profiles;
    };
    CollectorStatistics get_statistics() const;
    void reset_statistics();
    
    // Export capabilities
    void export_all_profiles_csv(const std::string& directory) const;
    void export_all_profiles_json(const std::string& directory) const;
    void export_comparative_analysis(const std::string& filename, 
                                   const std::vector<std::string>& system_names) const;

private:
    // Collection thread functions
    void collector_thread_function();
    void process_sample_queue();
    
    // Hardware counter management
    void initialize_hardware_counters();
    void cleanup_hardware_counters();
    HardwareCounters* get_thread_counters(std::thread::id thread_id);
    void read_hardware_counters(std::thread::id thread_id, PerformanceSample& sample);
    
    // Platform-specific implementations
    void collect_cpu_metrics(PerformanceSample& sample);
    void collect_memory_metrics(PerformanceSample& sample);
    void collect_system_metrics(PerformanceSample& sample);
    void collect_cache_metrics(PerformanceSample& sample);
    
    // Sample processing
    void add_sample_to_queue(const PerformanceSample& sample);
    bool process_next_sample();
    void handle_queue_overflow();
    
    // Utility functions
    u32 get_system_id_by_name(const std::string& name) const;
    static u64 get_current_time_ns();
    static u32 get_current_thread_id();
    static f64 get_cpu_frequency_ghz();
};

/**
 * @brief Global performance monitor managing all profiling activities
 */
class PerformanceMonitor {
private:
    static std::unique_ptr<PerformanceMonitor> instance_;
    static std::once_flag init_flag_;
    
    std::unique_ptr<PerformanceCollector> collector_;
    
    // Global configuration
    bool enabled_;
    f64 overhead_budget_percent_;  // Maximum allowed profiling overhead
    
    // Performance tracking
    std::chrono::steady_clock::time_point start_time_;
    std::atomic<f64> profiling_overhead_percent_;

public:
    static PerformanceMonitor& instance();
    
    PerformanceMonitor();
    ~PerformanceMonitor();
    
    // Global control
    void initialize(f64 collection_frequency = 1000.0);
    void shutdown();
    void enable(bool enabled = true) { enabled_ = enabled; }
    bool is_enabled() const { return enabled_; }
    
    // Collection interface
    void begin_system_execution(u32 system_id, const std::string& name);
    void end_system_execution(u32 system_id, const ExecutionContext& context);
    void record_custom_metric(u32 system_id, const std::string& metric_name, f64 value);
    
    // Analysis interface
    SystemProfile* get_system_profile(u32 system_id) const;
    std::vector<std::string> get_performance_bottlenecks() const;
    f64 get_system_efficiency_score() const;
    
    // Configuration
    void set_overhead_budget(f64 percent) { overhead_budget_percent_ = percent; }
    void configure_collector(f64 frequency, bool hw_counters = true, bool memory_stats = true);
    
    // Reporting
    std::string generate_comprehensive_report() const;
    void export_performance_data(const std::string& directory) const;
    
    // Overhead monitoring
    f64 get_profiling_overhead() const { return profiling_overhead_percent_.load(); }
    bool is_within_overhead_budget() const;

private:
    void monitor_profiling_overhead();
    void adjust_collection_frequency_for_overhead();
};

// Convenience macros for performance monitoring
#define ECSCOPE_PROFILE_SYSTEM_BEGIN(system_id, name) \
    do { \
        if (ecscope::scheduling::PerformanceMonitor::instance().is_enabled()) { \
            ecscope::scheduling::PerformanceMonitor::instance().begin_system_execution(system_id, name); \
        } \
    } while(0)

#define ECSCOPE_PROFILE_SYSTEM_END(system_id, context) \
    do { \
        if (ecscope::scheduling::PerformanceMonitor::instance().is_enabled()) { \
            ecscope::scheduling::PerformanceMonitor::instance().end_system_execution(system_id, context); \
        } \
    } while(0)

#define ECSCOPE_RECORD_METRIC(system_id, metric_name, value) \
    do { \
        if (ecscope::scheduling::PerformanceMonitor::instance().is_enabled()) { \
            ecscope::scheduling::PerformanceMonitor::instance().record_custom_metric(system_id, metric_name, value); \
        } \
    } while(0)

} // namespace ecscope::scheduling