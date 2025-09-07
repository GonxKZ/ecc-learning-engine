#pragma once

/**
 * @file advanced_profiler.hpp
 * @brief Complete Advanced Profiling and Debugging Tools for ECScope
 * 
 * This comprehensive profiling system provides production-ready debugging tools including:
 * - Complete ECS Profiler with deep system analysis
 * - Advanced Memory Debugger with leak detection and fragmentation analysis
 * - GPU Performance Monitor with complete GPU metrics
 * - Visual Debugging Interface with real-time graphs and overlays
 * - Statistical Analysis System for performance regression detection
 * - Comprehensive Debug Console with interactive commands
 * - Cross-Platform Profiling with OS-specific optimizations
 * - Educational Debugging Tools with interactive tutorials
 * 
 * @author ECScope Development Team
 * @date 2024
 */

#include "types.hpp"
#include "ecs_profiler.hpp"
#include "memory_debugger.hpp"
#include <chrono>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <vector>
#include <string>
#include <functional>
#include <queue>
#include <fstream>
#include <array>
#include <optional>
#include <variant>
#include <algorithm>
#include <numeric>
#include <future>

#ifdef _WIN32
    #include <windows.h>
    #include <psapi.h>
    #include <pdh.h>
#elif defined(__linux__)
    #include <sys/resource.h>
    #include <sys/time.h>
    #include <unistd.h>
    #include <fstream>
#elif defined(__APPLE__)
    #include <mach/mach.h>
    #include <mach/task_info.h>
    #include <sys/resource.h>
#endif

#ifdef ECSCOPE_ENABLE_GPU_PROFILING
    #ifdef _WIN32
        #include <d3d11.h>
        #include <dxgi.h>
    #endif
    #ifdef __linux__
        // OpenGL/Vulkan headers would go here
    #endif
#endif

namespace ecscope::profiling {

using namespace std::chrono;

// Forward declarations
class AdvancedProfiler;
class VisualDebugInterface;
class DebugConsole;
class StatisticalAnalyzer;

//=============================================================================
// Advanced Performance Metrics and Data Structures
//=============================================================================

// Extended profiling categories with granular system tracking
enum class AdvancedProfileCategory : u16 {
    // Core ECS
    ECS_ENTITY_CREATION = 0,
    ECS_ENTITY_DESTRUCTION,
    ECS_COMPONENT_ADD,
    ECS_COMPONENT_REMOVE,
    ECS_COMPONENT_ACCESS,
    ECS_SYSTEM_UPDATE,
    ECS_ARCHETYPE_CREATION,
    ECS_ARCHETYPE_MIGRATION,
    ECS_QUERY_EXECUTION,
    ECS_EVENT_DISPATCH,
    
    // Memory Management
    MEMORY_ALLOCATION,
    MEMORY_DEALLOCATION,
    MEMORY_GARBAGE_COLLECTION,
    MEMORY_COMPACTION,
    MEMORY_CACHE_MISS,
    MEMORY_PAGE_FAULT,
    
    // GPU Operations
    GPU_DRAW_CALL,
    GPU_COMPUTE_DISPATCH,
    GPU_BUFFER_UPLOAD,
    GPU_BUFFER_DOWNLOAD,
    GPU_TEXTURE_UPLOAD,
    GPU_SHADER_COMPILATION,
    GPU_SYNCHRONIZATION,
    GPU_MEMORY_ALLOCATION,
    
    // Physics
    PHYSICS_BROAD_PHASE,
    PHYSICS_NARROW_PHASE,
    PHYSICS_CONSTRAINT_SOLVING,
    PHYSICS_INTEGRATION,
    PHYSICS_COLLISION_RESPONSE,
    
    // Rendering
    RENDER_CULLING,
    RENDER_SORTING,
    RENDER_BATCHING,
    RENDER_SUBMISSION,
    RENDER_PRESENT,
    
    // Audio
    AUDIO_MIXING,
    AUDIO_STREAMING,
    AUDIO_PROCESSING,
    
    // I/O Operations
    FILE_IO_READ,
    FILE_IO_WRITE,
    NETWORK_SEND,
    NETWORK_RECEIVE,
    
    // Threading
    THREAD_SYNC,
    THREAD_SCHEDULING,
    THREAD_CONTEXT_SWITCH,
    
    // Custom
    CUSTOM_USER_DEFINED
};

// Comprehensive system performance metrics
struct AdvancedSystemMetrics {
    std::string system_name;
    AdvancedProfileCategory category;
    
    // Timing statistics
    high_resolution_clock::duration total_time{0};
    high_resolution_clock::duration min_time{high_resolution_clock::duration::max()};
    high_resolution_clock::duration max_time{0};
    high_resolution_clock::duration avg_time{0};
    high_resolution_clock::duration median_time{0};
    high_resolution_clock::duration p95_time{0};
    high_resolution_clock::duration p99_time{0};
    
    // Execution statistics
    u64 execution_count = 0;
    u64 cache_hits = 0;
    u64 cache_misses = 0;
    f64 cache_hit_ratio = 0.0;
    
    // Memory statistics
    usize memory_usage_current = 0;
    usize memory_usage_peak = 0;
    usize memory_usage_average = 0;
    usize memory_allocations = 0;
    usize memory_deallocations = 0;
    
    // CPU utilization
    f64 cpu_percentage = 0.0;
    f64 cpu_cycles = 0.0;
    u64 instructions_executed = 0;
    f64 instructions_per_cycle = 0.0;
    
    // Thread information
    u32 thread_id = 0;
    std::string thread_name;
    f64 thread_utilization = 0.0;
    
    // Historical data (circular buffer)
    static constexpr usize HISTORY_SIZE = 1000;
    std::array<high_resolution_clock::duration, HISTORY_SIZE> execution_history{};
    std::array<usize, HISTORY_SIZE> memory_history{};
    std::array<f64, HISTORY_SIZE> cpu_history{};
    usize history_index = 0;
    
    // Performance regression detection
    f64 performance_trend = 0.0; // Positive = getting slower
    bool is_regressing = false;
    high_resolution_clock::time_point last_regression_check;
    
    // Real-time statistics
    high_resolution_clock::time_point last_execution;
    f64 executions_per_second = 0.0;
    
    void update_execution(high_resolution_clock::duration execution_time, usize memory_used, f64 cpu_used = 0.0) {
        // Update timing statistics
        total_time += execution_time;
        min_time = std::min(min_time, execution_time);
        max_time = std::max(max_time, execution_time);
        ++execution_count;
        avg_time = high_resolution_clock::duration(total_time.count() / execution_count);
        
        // Update memory statistics
        memory_usage_current = memory_used;
        memory_usage_peak = std::max(memory_usage_peak, memory_used);
        memory_usage_average = (memory_usage_average * (execution_count - 1) + memory_used) / execution_count;
        
        // Update CPU statistics
        cpu_percentage = cpu_used;
        
        // Update historical data
        execution_history[history_index] = execution_time;
        memory_history[history_index] = memory_used;
        cpu_history[history_index] = cpu_used;
        history_index = (history_index + 1) % HISTORY_SIZE;
        
        // Update real-time metrics
        auto now = high_resolution_clock::now();
        if (last_execution.time_since_epoch().count() > 0) {
            auto time_since_last = duration_cast<microseconds>(now - last_execution).count();
            if (time_since_last > 0) {
                executions_per_second = 1000000.0 / time_since_last;
            }
        }
        last_execution = now;
        
        // Calculate percentiles if we have enough data
        update_percentiles();
    }
    
    void update_percentiles() {
        if (execution_count < 10) return;
        
        // Get valid history data
        std::vector<high_resolution_clock::duration> valid_times;
        usize samples = std::min(execution_count, static_cast<u64>(HISTORY_SIZE));
        valid_times.reserve(samples);
        
        for (usize i = 0; i < samples; ++i) {
            if (execution_history[i].count() > 0) {
                valid_times.push_back(execution_history[i]);
            }
        }
        
        if (valid_times.empty()) return;
        
        std::sort(valid_times.begin(), valid_times.end());
        
        usize median_idx = valid_times.size() / 2;
        usize p95_idx = static_cast<usize>(valid_times.size() * 0.95);
        usize p99_idx = static_cast<usize>(valid_times.size() * 0.99);
        
        median_time = valid_times[median_idx];
        p95_time = valid_times[p95_idx];
        p99_time = valid_times[p99_idx];
    }
    
    f64 get_performance_score() const {
        if (execution_count == 0) return 100.0;
        
        // Score based on multiple factors (0-100, higher is better)
        f64 timing_score = std::max(0.0, 100.0 - (avg_time.count() / 1000.0)); // Penalty for slow execution
        f64 memory_score = std::max(0.0, 100.0 - (memory_usage_average / (1024.0 * 1024.0))); // Penalty for high memory usage
        f64 cache_score = cache_hit_ratio * 100.0;
        f64 consistency_score = std::max(0.0, 100.0 - ((max_time.count() - min_time.count()) / 1000.0)); // Penalty for variance
        
        return (timing_score * 0.3 + memory_score * 0.2 + cache_score * 0.3 + consistency_score * 0.2);
    }
    
    bool detect_regression() {
        if (execution_count < 100) return false; // Need sufficient data
        
        auto now = high_resolution_clock::now();
        if (now - last_regression_check < seconds(5)) return is_regressing; // Check every 5 seconds
        
        last_regression_check = now;
        
        // Calculate trend over last 50% of history
        usize samples = std::min(execution_count, static_cast<u64>(HISTORY_SIZE));
        usize trend_samples = samples / 2;
        if (trend_samples < 10) return false;
        
        f64 sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
        usize start_idx = (history_index >= trend_samples) ? history_index - trend_samples : HISTORY_SIZE - (trend_samples - history_index);
        
        for (usize i = 0; i < trend_samples; ++i) {
            usize idx = (start_idx + i) % HISTORY_SIZE;
            f64 x = static_cast<f64>(i);
            f64 y = static_cast<f64>(execution_history[idx].count());
            
            sum_x += x;
            sum_y += y;
            sum_xy += x * y;
            sum_x2 += x * x;
        }
        
        // Linear regression slope
        f64 n = static_cast<f64>(trend_samples);
        performance_trend = (n * sum_xy - sum_x * sum_y) / (n * sum_x2 - sum_x * sum_x);
        
        // Consider regression if slope is positive and significant
        is_regressing = performance_trend > (avg_time.count() * 0.01); // 1% increase per sample
        
        return is_regressing;
    }
};

// GPU Performance metrics
struct GPUMetrics {
    // GPU information
    std::string gpu_name;
    std::string driver_version;
    usize total_memory = 0;
    usize available_memory = 0;
    usize used_memory = 0;
    
    // Performance counters
    u64 draw_calls = 0;
    u64 compute_dispatches = 0;
    u64 vertices_processed = 0;
    u64 pixels_rendered = 0;
    u64 triangles_rendered = 0;
    
    // Timing
    high_resolution_clock::duration gpu_frame_time{0};
    high_resolution_clock::duration gpu_wait_time{0};
    high_resolution_clock::duration gpu_execution_time{0};
    
    // Utilization
    f32 gpu_utilization = 0.0f;
    f32 memory_bandwidth_utilization = 0.0f;
    f32 shader_utilization = 0.0f;
    
    // Bottleneck detection
    enum class Bottleneck {
        NONE,
        VERTEX_PROCESSING,
        PIXEL_PROCESSING,
        MEMORY_BANDWIDTH,
        SYNCHRONIZATION,
        DRIVER_OVERHEAD
    } current_bottleneck = Bottleneck::NONE;
    
    // Shader statistics
    struct ShaderStats {
        std::string name;
        usize compilation_time_ms = 0;
        usize instruction_count = 0;
        usize register_usage = 0;
        f32 occupancy = 0.0f;
        u64 invocations = 0;
    };
    std::vector<ShaderStats> shader_stats;
    
    // Buffer and texture metrics
    struct ResourceMetrics {
        u64 buffer_uploads = 0;
        u64 buffer_downloads = 0;
        u64 texture_uploads = 0;
        usize total_buffer_memory = 0;
        usize total_texture_memory = 0;
        u32 active_render_targets = 0;
    } resources;
    
    void reset_frame_counters() {
        draw_calls = 0;
        compute_dispatches = 0;
        vertices_processed = 0;
        pixels_rendered = 0;
        triangles_rendered = 0;
        gpu_frame_time = high_resolution_clock::duration{0};
        gpu_wait_time = high_resolution_clock::duration{0};
        gpu_execution_time = high_resolution_clock::duration{0};
    }
    
    f32 get_efficiency_score() const {
        f32 score = 0.0f;
        
        // GPU utilization should be high but not maxed out
        if (gpu_utilization > 0.8f && gpu_utilization < 0.95f) {
            score += 25.0f;
        } else {
            score += std::max(0.0f, 25.0f * gpu_utilization);
        }
        
        // Memory bandwidth should be well utilized
        score += 25.0f * memory_bandwidth_utilization;
        
        // Low wait times are good
        f32 wait_ratio = static_cast<f32>(gpu_wait_time.count()) / std::max(1LL, gpu_frame_time.count());
        score += 25.0f * (1.0f - std::min(1.0f, wait_ratio));
        
        // Consistent frame times are good
        score += 25.0f * shader_utilization;
        
        return std::min(100.0f, score);
    }
};

// Memory analysis with detailed fragmentation tracking
struct AdvancedMemoryMetrics {
    // Basic memory statistics
    usize physical_memory_total = 0;
    usize physical_memory_available = 0;
    usize physical_memory_used = 0;
    usize virtual_memory_total = 0;
    usize virtual_memory_used = 0;
    
    // Process-specific memory
    usize process_working_set = 0;
    usize process_private_bytes = 0;
    usize process_virtual_bytes = 0;
    usize process_peak_working_set = 0;
    
    // Heap analysis
    struct HeapMetrics {
        usize heap_size = 0;
        usize committed_size = 0;
        usize free_size = 0;
        usize largest_free_block = 0;
        f32 fragmentation_ratio = 0.0f;
        u32 free_block_count = 0;
        
        // Fragmentation analysis
        std::vector<usize> free_block_sizes;
        std::vector<usize> allocated_block_sizes;
        
        void update_fragmentation() {
            if (free_size == 0) {
                fragmentation_ratio = 0.0f;
                return;
            }
            
            // External fragmentation: 1 - (largest_free_block / total_free_space)
            fragmentation_ratio = 1.0f - (static_cast<f32>(largest_free_block) / free_size);
        }
        
        f32 get_efficiency_score() const {
            if (heap_size == 0) return 100.0f;
            
            f32 utilization = static_cast<f32>(committed_size) / heap_size;
            f32 fragmentation_penalty = fragmentation_ratio * 50.0f;
            
            return std::max(0.0f, utilization * 100.0f - fragmentation_penalty);
        }
    } heap_metrics;
    
    // Allocation patterns
    struct AllocationPattern {
        usize small_allocations = 0;    // < 1KB
        usize medium_allocations = 0;   // 1KB - 1MB
        usize large_allocations = 0;    // > 1MB
        
        high_resolution_clock::duration avg_allocation_time{0};
        high_resolution_clock::duration avg_deallocation_time{0};
        
        // Hot/cold allocation tracking
        u32 hot_allocations = 0;        // Frequently accessed
        u32 cold_allocations = 0;       // Rarely accessed
        
        f32 get_allocation_efficiency() const {
            usize total = small_allocations + medium_allocations + large_allocations;
            if (total == 0) return 100.0f;
            
            // Prefer fewer large allocations over many small ones
            f32 size_score = (large_allocations * 3.0f + medium_allocations * 2.0f + small_allocations) / total;
            f32 access_score = hot_allocations / static_cast<f32>(std::max(1u, hot_allocations + cold_allocations));
            
            return std::min(100.0f, (size_score + access_score) * 50.0f);
        }
    } allocation_pattern;
    
    // Cache performance
    struct CacheMetrics {
        u64 l1_cache_hits = 0;
        u64 l1_cache_misses = 0;
        u64 l2_cache_hits = 0;
        u64 l2_cache_misses = 0;
        u64 l3_cache_hits = 0;
        u64 l3_cache_misses = 0;
        
        f64 l1_hit_ratio = 0.0;
        f64 l2_hit_ratio = 0.0;
        f64 l3_hit_ratio = 0.0;
        f64 overall_hit_ratio = 0.0;
        
        void update_ratios() {
            u64 l1_total = l1_cache_hits + l1_cache_misses;
            u64 l2_total = l2_cache_hits + l2_cache_misses;
            u64 l3_total = l3_cache_hits + l3_cache_misses;
            
            l1_hit_ratio = l1_total > 0 ? static_cast<f64>(l1_cache_hits) / l1_total : 0.0;
            l2_hit_ratio = l2_total > 0 ? static_cast<f64>(l2_cache_hits) / l2_total : 0.0;
            l3_hit_ratio = l3_total > 0 ? static_cast<f64>(l3_cache_hits) / l3_total : 0.0;
            
            u64 total_ops = l1_total + l2_total + l3_total;
            u64 total_hits = l1_cache_hits + l2_cache_hits + l3_cache_hits;
            overall_hit_ratio = total_ops > 0 ? static_cast<f64>(total_hits) / total_ops : 0.0;
        }
        
        f32 get_cache_efficiency_score() const {
            // L1 cache is most important
            return static_cast<f32>(l1_hit_ratio * 50.0 + l2_hit_ratio * 30.0 + l3_hit_ratio * 20.0);
        }
    } cache_metrics;
    
    // Memory leak detection
    struct LeakDetection {
        std::vector<AllocationRecord> potential_leaks;
        usize leak_score = 0;
        high_resolution_clock::time_point last_scan;
        
        bool has_potential_leaks() const {
            return !potential_leaks.empty();
        }
        
        usize get_total_leaked_bytes() const {
            usize total = 0;
            for (const auto& leak : potential_leaks) {
                total += leak.size;
            }
            return total;
        }
    } leak_detection;
    
    f32 get_overall_memory_score() const {
        f32 heap_score = heap_metrics.get_efficiency_score() * 0.3f;
        f32 allocation_score = allocation_pattern.get_allocation_efficiency() * 0.3f;
        f32 cache_score = cache_metrics.get_cache_efficiency_score() * 0.3f;
        f32 leak_penalty = leak_detection.has_potential_leaks() ? 10.0f : 0.0f;
        
        return std::max(0.0f, heap_score + allocation_score + cache_score - leak_penalty);
    }
};

//=============================================================================
// Statistical Analysis System
//=============================================================================

// Performance trend analysis
struct PerformanceTrend {
    enum class TrendType {
        IMPROVING,
        STABLE,
        DEGRADING,
        VOLATILE
    } type = TrendType::STABLE;
    
    f64 trend_coefficient = 0.0;    // Linear regression coefficient
    f64 confidence = 0.0;           // Statistical confidence (0-1)
    f64 volatility = 0.0;           // Standard deviation / mean
    
    std::string description;
    std::vector<f64> recommendations;
};

// Anomaly detection for performance outliers
struct PerformanceAnomaly {
    std::string system_name;
    AdvancedProfileCategory category;
    high_resolution_clock::time_point timestamp;
    
    enum class AnomalyType {
        PERFORMANCE_SPIKE,
        MEMORY_SPIKE,
        CACHE_MISS_SPIKE,
        EXECUTION_TIME_OUTLIER,
        MEMORY_LEAK_DETECTED,
        RESOURCE_EXHAUSTION
    } type;
    
    f64 severity_score;     // 0-100
    f64 confidence;         // 0-1
    std::string description;
    std::string suggested_action;
    
    std::variant<
        high_resolution_clock::duration,  // For timing anomalies
        usize,                           // For memory anomalies
        f64                              // For ratio/percentage anomalies
    > value;
    
    std::variant<
        high_resolution_clock::duration,
        usize,
        f64
    > expected_value;
};

// Regression detection system
class RegressionDetector {
private:
    struct SystemHistory {
        std::vector<f64> performance_scores;
        std::vector<high_resolution_clock::time_point> timestamps;
        static constexpr usize MAX_HISTORY = 10000;
        
        void add_sample(f64 score, high_resolution_clock::time_point timestamp) {
            performance_scores.push_back(score);
            timestamps.push_back(timestamp);
            
            if (performance_scores.size() > MAX_HISTORY) {
                performance_scores.erase(performance_scores.begin());
                timestamps.erase(timestamps.begin());
            }
        }
    };
    
    std::unordered_map<std::string, SystemHistory> system_histories_;
    f64 regression_threshold_ = 0.05; // 5% performance drop
    
public:
    void add_performance_sample(const std::string& system_name, f64 score) {
        auto& history = system_histories_[system_name];
        history.add_sample(score, high_resolution_clock::now());
    }
    
    std::optional<PerformanceTrend> detect_trend(const std::string& system_name) {
        auto it = system_histories_.find(system_name);
        if (it == system_histories_.end() || it->second.performance_scores.size() < 10) {
            return std::nullopt;
        }
        
        const auto& scores = it->second.performance_scores;
        return calculate_trend(scores);
    }
    
    std::vector<PerformanceAnomaly> detect_anomalies(const std::string& system_name) {
        std::vector<PerformanceAnomaly> anomalies;
        
        auto it = system_histories_.find(system_name);
        if (it == system_histories_.end() || it->second.performance_scores.size() < 20) {
            return anomalies;
        }
        
        const auto& scores = it->second.performance_scores;
        const auto& timestamps = it->second.timestamps;
        
        // Calculate statistics
        f64 mean = std::accumulate(scores.begin(), scores.end(), 0.0) / scores.size();
        f64 variance = 0.0;
        for (f64 score : scores) {
            variance += (score - mean) * (score - mean);
        }
        variance /= scores.size();
        f64 std_dev = std::sqrt(variance);
        
        // Detect outliers (values beyond 2 standard deviations)
        for (usize i = 0; i < scores.size(); ++i) {
            f64 z_score = std::abs(scores[i] - mean) / std_dev;
            if (z_score > 2.0) {
                PerformanceAnomaly anomaly;
                anomaly.system_name = system_name;
                anomaly.timestamp = timestamps[i];
                anomaly.type = scores[i] < mean ? PerformanceAnomaly::AnomalyType::PERFORMANCE_SPIKE : 
                                                 PerformanceAnomaly::AnomalyType::EXECUTION_TIME_OUTLIER;
                anomaly.severity_score = std::min(100.0, z_score * 25.0);
                anomaly.confidence = std::min(1.0, z_score / 3.0);
                anomaly.value = scores[i];
                anomaly.expected_value = mean;
                anomaly.description = "Performance outlier detected: " + std::to_string(z_score) + " standard deviations from mean";
                
                if (scores[i] < mean) {
                    anomaly.suggested_action = "Investigate performance regression - check for memory leaks, inefficient algorithms, or resource contention";
                } else {
                    anomaly.suggested_action = "Investigate performance spike - may indicate measurement error or unusual workload";
                }
                
                anomalies.push_back(anomaly);
            }
        }
        
        return anomalies;
    }
    
private:
    PerformanceTrend calculate_trend(const std::vector<f64>& scores) {
        PerformanceTrend trend;
        
        usize n = scores.size();
        f64 sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
        
        for (usize i = 0; i < n; ++i) {
            f64 x = static_cast<f64>(i);
            f64 y = scores[i];
            
            sum_x += x;
            sum_y += y;
            sum_xy += x * y;
            sum_x2 += x * x;
        }
        
        // Linear regression
        f64 n_f = static_cast<f64>(n);
        trend.trend_coefficient = (n_f * sum_xy - sum_x * sum_y) / (n_f * sum_x2 - sum_x * sum_x);
        
        // Calculate correlation coefficient for confidence
        f64 mean_x = sum_x / n_f;
        f64 mean_y = sum_y / n_f;
        
        f64 ss_xy = 0, ss_x = 0, ss_y = 0;
        for (usize i = 0; i < n; ++i) {
            f64 x = static_cast<f64>(i) - mean_x;
            f64 y = scores[i] - mean_y;
            ss_xy += x * y;
            ss_x += x * x;
            ss_y += y * y;
        }
        
        f64 correlation = ss_xy / std::sqrt(ss_x * ss_y);
        trend.confidence = std::abs(correlation);
        
        // Calculate volatility
        f64 mean = sum_y / n_f;
        f64 variance = 0;
        for (f64 score : scores) {
            variance += (score - mean) * (score - mean);
        }
        variance /= n_f;
        trend.volatility = std::sqrt(variance) / mean;
        
        // Classify trend
        if (std::abs(trend.trend_coefficient) < 0.001) {
            trend.type = PerformanceTrend::TrendType::STABLE;
            trend.description = "Performance is stable with no significant trend";
        } else if (trend.trend_coefficient > 0) {
            trend.type = PerformanceTrend::TrendType::IMPROVING;
            trend.description = "Performance is improving over time";
        } else {
            if (trend.volatility > 0.1) {
                trend.type = PerformanceTrend::TrendType::VOLATILE;
                trend.description = "Performance is volatile with high variance";
            } else {
                trend.type = PerformanceTrend::TrendType::DEGRADING;
                trend.description = "Performance is degrading over time";
            }
        }
        
        return trend;
    }
};

//=============================================================================
// Advanced Profiler - Main Orchestrator
//=============================================================================

struct ProfilingConfig {
    // General settings
    bool enabled = true;
    bool collect_stack_traces = true;
    bool enable_memory_tracking = true;
    bool enable_gpu_profiling = true;
    bool enable_statistical_analysis = true;
    
    // Performance settings
    f32 sampling_rate = 1.0f;           // 1.0 = 100% sampling
    u32 max_events_per_frame = 10000;
    u32 history_retention_seconds = 3600; // 1 hour
    
    // Thresholds for warnings/alerts
    high_resolution_clock::duration slow_system_threshold{milliseconds(16)};
    usize high_memory_threshold = 512 * 1024 * 1024; // 512MB
    f32 regression_threshold = 0.05f;   // 5% performance drop
    
    // Output settings
    bool auto_export_reports = false;
    std::string export_directory = "./profiling_data/";
    u32 export_interval_minutes = 60;
};

class AdvancedProfiler {
private:
    // Core data
    std::unique_ptr<ECSProfiler> ecs_profiler_;
    std::unique_ptr<MemoryDebugger> memory_debugger_;
    std::unique_ptr<RegressionDetector> regression_detector_;
    std::unique_ptr<VisualDebugInterface> visual_interface_;
    std::unique_ptr<DebugConsole> debug_console_;
    
    // Configuration
    ProfilingConfig config_;
    std::atomic<bool> enabled_{true};
    std::atomic<bool> paused_{false};
    
    // Thread management
    std::unique_ptr<std::thread> profiling_thread_;
    std::atomic<bool> should_stop_{false};
    mutable std::mutex data_mutex_;
    
    // System metrics
    std::unordered_map<std::string, AdvancedSystemMetrics> system_metrics_;
    GPUMetrics gpu_metrics_;
    AdvancedMemoryMetrics memory_metrics_;
    
    // Event management
    std::queue<ProfileEvent> event_queue_;
    std::vector<PerformanceAnomaly> recent_anomalies_;
    static constexpr usize MAX_ANOMALIES = 1000;
    
    // Platform-specific helpers
    #ifdef _WIN32
    HANDLE process_handle_ = nullptr;
    PDH_HQUERY cpu_query_ = nullptr;
    PDH_HCOUNTER cpu_counter_ = nullptr;
    #endif
    
    // Timing
    high_resolution_clock::time_point start_time_;
    high_resolution_clock::time_point last_update_;
    
public:
    AdvancedProfiler(const ProfilingConfig& config = ProfilingConfig{});
    ~AdvancedProfiler();
    
    // Core interface
    void initialize();
    void shutdown();
    void update(f32 delta_time);
    
    // Configuration
    void set_config(const ProfilingConfig& config) { config_ = config; }
    const ProfilingConfig& get_config() const { return config_; }
    
    void enable() { enabled_ = true; }
    void disable() { enabled_ = false; }
    bool is_enabled() const { return enabled_; }
    
    void pause() { paused_ = true; }
    void resume() { paused_ = false; }
    bool is_paused() const { return paused_; }
    
    // System profiling
    void begin_system_profile(const std::string& system_name, AdvancedProfileCategory category = AdvancedProfileCategory::CUSTOM_USER_DEFINED);
    void end_system_profile(const std::string& system_name);
    
    // GPU profiling
    void begin_gpu_profile(const std::string& operation_name);
    void end_gpu_profile(const std::string& operation_name);
    void record_draw_call(u32 vertices, u32 triangles);
    void record_compute_dispatch(u32 groups_x, u32 groups_y, u32 groups_z);
    
    // Memory profiling integration
    void record_allocation(void* ptr, usize size, const std::string& category);
    void record_deallocation(void* ptr);
    
    // Query interface
    AdvancedSystemMetrics get_system_metrics(const std::string& system_name) const;
    std::vector<AdvancedSystemMetrics> get_all_system_metrics() const;
    GPUMetrics get_gpu_metrics() const;
    AdvancedMemoryMetrics get_memory_metrics() const;
    
    // Analysis and detection
    std::vector<PerformanceAnomaly> detect_anomalies() const;
    std::vector<std::string> get_performance_recommendations() const;
    f64 calculate_overall_performance_score() const;
    
    // Statistical analysis
    std::optional<PerformanceTrend> analyze_system_trend(const std::string& system_name) const;
    std::vector<std::pair<std::string, PerformanceTrend>> analyze_all_trends() const;
    
    // Reporting
    std::string generate_comprehensive_report() const;
    std::string generate_executive_summary() const;
    void export_detailed_report(const std::string& filename) const;
    void export_csv_data(const std::string& filename) const;
    void export_json_data(const std::string& filename) const;
    
    // Debug interface access
    VisualDebugInterface* get_visual_interface() const { return visual_interface_.get(); }
    DebugConsole* get_debug_console() const { return debug_console_.get(); }
    
    // Frame management
    void begin_frame();
    void end_frame();
    
    // Singleton access
    static AdvancedProfiler& instance();
    static void cleanup();
    
private:
    void profiling_thread_main();
    void update_system_metrics();
    void update_gpu_metrics();
    void update_memory_metrics();
    void update_platform_metrics();
    void process_events();
    void detect_performance_issues();
    void cleanup_old_data();
    
    // Platform-specific implementations
    void initialize_platform_profiling();
    void shutdown_platform_profiling();
    void collect_cpu_metrics();
    void collect_memory_metrics();
    void collect_gpu_metrics();
    
    #ifdef _WIN32
    void initialize_windows_profiling();
    void collect_windows_cpu_info();
    void collect_windows_memory_info();
    void collect_windows_gpu_info();
    #elif defined(__linux__)
    void initialize_linux_profiling();
    void collect_linux_cpu_info();
    void collect_linux_memory_info();
    void collect_linux_gpu_info();
    #elif defined(__APPLE__)
    void initialize_macos_profiling();
    void collect_macos_cpu_info();
    void collect_macos_memory_info();
    void collect_macos_gpu_info();
    #endif
};

//=============================================================================
// Convenience macros for easy profiling
//=============================================================================

#define PROFILE_ADVANCED_SCOPE(name) \
    ecscope::profiling::AdvancedProfiler::instance().begin_system_profile(name); \
    auto _advanced_prof_guard = std::unique_ptr<void, std::function<void(void*)>>( \
        nullptr, [](void*) { \
            ecscope::profiling::AdvancedProfiler::instance().end_system_profile(name); \
        })

#define PROFILE_ADVANCED_SYSTEM(name, category) \
    ecscope::profiling::AdvancedProfiler::instance().begin_system_profile(name, category); \
    auto _advanced_system_guard = std::unique_ptr<void, std::function<void(void*)>>( \
        nullptr, [](void*) { \
            ecscope::profiling::AdvancedProfiler::instance().end_system_profile(name); \
        })

#define PROFILE_GPU_OPERATION(name) \
    ecscope::profiling::AdvancedProfiler::instance().begin_gpu_profile(name); \
    auto _gpu_prof_guard = std::unique_ptr<void, std::function<void(void*)>>( \
        nullptr, [](void*) { \
            ecscope::profiling::AdvancedProfiler::instance().end_gpu_profile(name); \
        })

#define PROFILE_DRAW_CALL(vertices, triangles) \
    ecscope::profiling::AdvancedProfiler::instance().record_draw_call(vertices, triangles)

#define PROFILE_COMPUTE_DISPATCH(x, y, z) \
    ecscope::profiling::AdvancedProfiler::instance().record_compute_dispatch(x, y, z)

// Conditional profiling macros (compile-time disabled in release builds)
#ifdef ECSCOPE_ENABLE_PROFILING
    #define PROFILE_ADVANCED_CONDITIONAL(name) PROFILE_ADVANCED_SCOPE(name)
    #define PROFILE_ADVANCED_SYSTEM_CONDITIONAL(name, category) PROFILE_ADVANCED_SYSTEM(name, category)
    #define PROFILE_GPU_CONDITIONAL(name) PROFILE_GPU_OPERATION(name)
#else
    #define PROFILE_ADVANCED_CONDITIONAL(name) do { } while(0)
    #define PROFILE_ADVANCED_SYSTEM_CONDITIONAL(name, category) do { } while(0)
    #define PROFILE_GPU_CONDITIONAL(name) do { } while(0)
#endif

} // namespace ecscope::profiling