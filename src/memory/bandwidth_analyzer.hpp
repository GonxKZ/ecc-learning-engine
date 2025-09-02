#pragma once

/**
 * @file memory/bandwidth_analyzer.hpp
 * @brief Advanced Memory Bandwidth Analysis and Bottleneck Detection System
 * 
 * This header provides comprehensive memory bandwidth analysis tools that help
 * identify memory access bottlenecks, measure actual memory throughput, and
 * provide educational insights into memory system performance characteristics.
 * 
 * Key Features:
 * - Real-time memory bandwidth measurement across NUMA nodes
 * - Memory access pattern bottleneck detection
 * - Bandwidth saturation analysis and prediction
 * - Cross-thread memory contention measurement
 * - Memory controller utilization tracking
 * - Cache hierarchy bandwidth analysis
 * - Educational visualization of memory system behavior
 * 
 * Educational Value:
 * - Demonstrates memory wall effects and bandwidth limitations
 * - Shows NUMA memory access cost analysis
 * - Illustrates cache hierarchy bandwidth characteristics
 * - Provides memory optimization recommendations
 * - Real-time memory system behavior visualization
 * - Examples of memory-bound vs CPU-bound workload analysis
 * 
 * @author ECScope Educational ECS Framework - Memory Performance Analysis
 * @date 2025
 */

#include "numa_manager.hpp"
#include "cache_aware_structures.hpp"
#include "core/types.hpp"
#include "core/log.hpp"
#include "core/profiler.hpp"
#include <atomic>
#include <memory>
#include <vector>
#include <array>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <algorithm>

namespace ecscope::memory::bandwidth {

//=============================================================================
// Memory Bandwidth Measurement Infrastructure
//=============================================================================

/**
 * @brief Memory operation types for bandwidth analysis
 */
enum class MemoryOperation : u8 {
    SequentialRead,     // Linear memory reading
    SequentialWrite,    // Linear memory writing
    RandomRead,         // Random memory reading
    RandomWrite,        // Random memory writing
    ReadModifyWrite,    // Atomic read-modify-write
    CopyOperation,      // Memory copying (memcpy-like)
    SetOperation,       // Memory setting (memset-like)
    StreamingRead,      // Non-temporal reads
    StreamingWrite      // Non-temporal writes
};

/**
 * @brief Bandwidth measurement result
 */
struct BandwidthMeasurement {
    MemoryOperation operation;
    f64 bandwidth_gbps;             // Measured bandwidth in GB/s
    f64 latency_ns;                 // Average access latency
    f64 duration_seconds;           // Total measurement duration
    usize bytes_processed;          // Total bytes processed
    usize operations_count;         // Number of operations
    u32 numa_node;                  // NUMA node where test ran
    u32 thread_count;               // Number of threads used
    f64 cpu_utilization;            // CPU utilization during test
    f64 cache_miss_ratio;           // Estimated cache miss ratio
    f64 memory_controller_utilization; // Memory controller utilization estimate
    
    std::string operation_description;
    std::vector<f64> per_thread_bandwidth; // Per-thread bandwidth breakdown
    
    BandwidthMeasurement() = default;
    
    f64 get_operations_per_second() const {
        return duration_seconds > 0.0 ? operations_count / duration_seconds : 0.0;
    }
    
    f64 get_efficiency_ratio() const {
        // Compare against theoretical memory bandwidth (simplified estimate)
        f64 theoretical_max = 25.6; // DDR4-3200 theoretical max ~25.6 GB/s
        return bandwidth_gbps / theoretical_max;
    }
};

/**
 * @brief Memory bandwidth profiler with real-time monitoring
 */
class MemoryBandwidthProfiler {
private:
    // Configuration
    struct ProfilerConfig {
        usize buffer_size_mb;           // Buffer size for tests
        u32 measurement_duration_ms;    // Duration of each measurement
        u32 warmup_iterations;          // Warmup iterations before measurement
        bool enable_numa_analysis;      // Enable per-NUMA node analysis
        bool enable_cache_analysis;     // Enable cache hierarchy analysis
        bool enable_contention_analysis; // Enable multi-thread contention analysis
        f64 measurement_interval_seconds; // Interval between measurements
        
        ProfilerConfig() : buffer_size_mb(64), measurement_duration_ms(1000),
                          warmup_iterations(10), enable_numa_analysis(true),
                          enable_cache_analysis(true), enable_contention_analysis(true),
                          measurement_interval_seconds(1.0) {}
    };
    
    ProfilerConfig config_;
    
    // Test infrastructure
    struct TestBuffer {
        void* memory;
        usize size_bytes;
        u32 numa_node;
        bool is_aligned;
        
        TestBuffer() : memory(nullptr), size_bytes(0), numa_node(0), is_aligned(false) {}
        ~TestBuffer() { cleanup(); }
        
        bool allocate(usize size, u32 node_id, bool aligned = true);
        void cleanup();
        void prefault_pages();
        void flush_from_cache();
    };
    
    // Per-NUMA node test buffers
    std::vector<std::unique_ptr<TestBuffer>> test_buffers_;
    
    // Measurement state
    std::atomic<bool> profiling_active_{false};
    std::atomic<bool> should_stop_{false};
    std::thread profiling_thread_;
    std::mutex results_mutex_;
    std::vector<BandwidthMeasurement> measurement_history_;
    
    // Real-time monitoring
    std::atomic<f64> current_read_bandwidth_{0.0};
    std::atomic<f64> current_write_bandwidth_{0.0};
    std::atomic<f64> current_memory_utilization_{0.0};
    
    // NUMA integration
    numa::NumaManager& numa_manager_;
    cache::CacheTopologyAnalyzer& cache_analyzer_;
    
    // Performance counters (simplified - would use hardware counters in production)
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> total_bytes_read_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> total_bytes_written_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> total_operations_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<f64> cumulative_bandwidth_{0.0};
    
public:
    explicit MemoryBandwidthProfiler(numa::NumaManager& numa_mgr, 
                                   cache::CacheTopologyAnalyzer& cache_analyzer)
        : numa_manager_(numa_mgr), cache_analyzer_(cache_analyzer) {
        initialize_test_buffers();
    }
    
    ~MemoryBandwidthProfiler() {
        stop_profiling();
    }
    
    /**
     * @brief Start continuous bandwidth profiling
     */
    bool start_profiling(const ProfilerConfig& config = ProfilerConfig{}) {
        if (profiling_active_.load()) {
            return false; // Already running
        }
        
        config_ = config;
        should_stop_.store(false);
        profiling_active_.store(true);
        
        profiling_thread_ = std::thread([this]() {
            profiling_worker();
        });
        
        LOG_INFO("Started memory bandwidth profiling with {}MB buffers", config_.buffer_size_mb);
        return true;
    }
    
    /**
     * @brief Stop continuous profiling
     */
    void stop_profiling() {
        if (!profiling_active_.load()) {
            return;
        }
        
        should_stop_.store(true);
        profiling_active_.store(false);
        
        if (profiling_thread_.joinable()) {
            profiling_thread_.join();
        }
        
        LOG_INFO("Stopped memory bandwidth profiling");
    }
    
    /**
     * @brief Measure bandwidth for specific operation
     */
    BandwidthMeasurement measure_operation(MemoryOperation operation, 
                                          u32 numa_node = UINT32_MAX,
                                          u32 thread_count = 1) {
        PROFILE_FUNCTION();
        
        if (numa_node == UINT32_MAX) {
            auto current = numa_manager_.get_current_thread_node();
            numa_node = current.value_or(0);
        }
        
        // Ensure we have a buffer for this NUMA node
        if (numa_node >= test_buffers_.size() || !test_buffers_[numa_node]) {
            LOG_ERROR("No test buffer available for NUMA node {}", numa_node);
            return BandwidthMeasurement{};
        }
        
        auto& buffer = *test_buffers_[numa_node];
        
        switch (operation) {
            case MemoryOperation::SequentialRead:
                return measure_sequential_read(buffer, thread_count);
            case MemoryOperation::SequentialWrite:
                return measure_sequential_write(buffer, thread_count);
            case MemoryOperation::RandomRead:
                return measure_random_read(buffer, thread_count);
            case MemoryOperation::RandomWrite:
                return measure_random_write(buffer, thread_count);
            case MemoryOperation::ReadModifyWrite:
                return measure_read_modify_write(buffer, thread_count);
            case MemoryOperation::CopyOperation:
                return measure_copy_operation(buffer, thread_count);
            case MemoryOperation::SetOperation:
                return measure_set_operation(buffer, thread_count);
            case MemoryOperation::StreamingRead:
                return measure_streaming_read(buffer, thread_count);
            case MemoryOperation::StreamingWrite:
                return measure_streaming_write(buffer, thread_count);
            default:
                return BandwidthMeasurement{};
        }
    }
    
    /**
     * @brief Comprehensive bandwidth analysis across all NUMA nodes and operations
     */
    std::vector<BandwidthMeasurement> run_comprehensive_analysis() {
        PROFILE_FUNCTION();
        
        std::vector<BandwidthMeasurement> results;
        auto available_nodes = numa_manager_.get_topology().get_available_nodes();
        
        const std::vector<MemoryOperation> operations = {
            MemoryOperation::SequentialRead,
            MemoryOperation::SequentialWrite,
            MemoryOperation::RandomRead,
            MemoryOperation::RandomWrite,
            MemoryOperation::CopyOperation
        };
        
        const std::vector<u32> thread_counts = {1, 2, 4, 8};
        
        LOG_INFO("Starting comprehensive memory bandwidth analysis...");
        
        for (u32 numa_node : available_nodes) {
            for (MemoryOperation operation : operations) {
                for (u32 threads : thread_counts) {
                    auto measurement = measure_operation(operation, numa_node, threads);
                    if (measurement.bandwidth_gbps > 0.0) {
                        results.push_back(measurement);
                        
                        LOG_DEBUG("Node {} {} threads {}: {:.2f} GB/s", 
                                 numa_node, static_cast<int>(operation), threads, 
                                 measurement.bandwidth_gbps);
                    }
                }
            }
        }
        
        // Store results
        {
            std::lock_guard<std::mutex> lock(results_mutex_);
            measurement_history_.insert(measurement_history_.end(), results.begin(), results.end());
            
            // Keep history size manageable
            if (measurement_history_.size() > 10000) {
                measurement_history_.erase(measurement_history_.begin(), 
                                         measurement_history_.begin() + 5000);
            }
        }
        
        LOG_INFO("Completed comprehensive analysis with {} measurements", results.size());
        return results;
    }
    
    /**
     * @brief Get real-time bandwidth statistics
     */
    struct RealTimeStats {
        f64 current_read_bandwidth_gbps;
        f64 current_write_bandwidth_gbps;
        f64 current_memory_utilization;
        f64 peak_read_bandwidth_gbps;
        f64 peak_write_bandwidth_gbps;
        u64 total_bytes_processed;
        u64 total_operations;
        f64 average_bandwidth_gbps;
        f64 bandwidth_variance;
    };
    
    RealTimeStats get_real_time_stats() const {
        RealTimeStats stats{};
        
        stats.current_read_bandwidth_gbps = current_read_bandwidth_.load();
        stats.current_write_bandwidth_gbps = current_write_bandwidth_.load();
        stats.current_memory_utilization = current_memory_utilization_.load();
        stats.total_bytes_processed = total_bytes_read_.load() + total_bytes_written_.load();
        stats.total_operations = total_operations_.load();
        
        // Calculate averages and peaks from history
        {
            std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(results_mutex_));
            
            if (!measurement_history_.empty()) {
                f64 sum_bandwidth = 0.0;
                f64 peak_read = 0.0;
                f64 peak_write = 0.0;
                
                for (const auto& measurement : measurement_history_) {
                    sum_bandwidth += measurement.bandwidth_gbps;
                    
                    if (measurement.operation == MemoryOperation::SequentialRead ||
                        measurement.operation == MemoryOperation::RandomRead ||
                        measurement.operation == MemoryOperation::StreamingRead) {
                        peak_read = std::max(peak_read, measurement.bandwidth_gbps);
                    } else {
                        peak_write = std::max(peak_write, measurement.bandwidth_gbps);
                    }
                }
                
                stats.average_bandwidth_gbps = sum_bandwidth / measurement_history_.size();
                stats.peak_read_bandwidth_gbps = peak_read;
                stats.peak_write_bandwidth_gbps = peak_write;
                
                // Calculate variance
                f64 variance_sum = 0.0;
                for (const auto& measurement : measurement_history_) {
                    f64 diff = measurement.bandwidth_gbps - stats.average_bandwidth_gbps;
                    variance_sum += diff * diff;
                }
                stats.bandwidth_variance = variance_sum / measurement_history_.size();
            }
        }
        
        return stats;
    }
    
    /**
     * @brief Get measurement history
     */
    std::vector<BandwidthMeasurement> get_measurement_history() const {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(results_mutex_));
        return measurement_history_;
    }
    
    /**
     * @brief Configuration
     */
    void set_buffer_size(usize size_mb) { 
        if (!profiling_active_.load()) {
            config_.buffer_size_mb = size_mb;
            reinitialize_test_buffers();
        }
    }
    
    void set_measurement_interval(f64 interval_seconds) {
        config_.measurement_interval_seconds = interval_seconds;
    }
    
    ProfilerConfig get_config() const { return config_; }
    
private:
    bool initialize_test_buffers() {
        auto available_nodes = numa_manager_.get_topology().get_available_nodes();
        test_buffers_.resize(std::max(available_nodes.empty() ? 1u : *std::max_element(available_nodes.begin(), available_nodes.end()) + 1, 8u));
        
        for (u32 node_id : available_nodes) {
            test_buffers_[node_id] = std::make_unique<TestBuffer>();
            if (!test_buffers_[node_id]->allocate(config_.buffer_size_mb * 1024 * 1024, node_id)) {
                LOG_ERROR("Failed to allocate test buffer for NUMA node {}", node_id);
                return false;
            }
        }
        
        // Fallback buffer for node 0 if no specific nodes available
        if (available_nodes.empty()) {
            test_buffers_[0] = std::make_unique<TestBuffer>();
            if (!test_buffers_[0]->allocate(config_.buffer_size_mb * 1024 * 1024, 0)) {
                LOG_ERROR("Failed to allocate fallback test buffer");
                return false;
            }
        }
        
        return true;
    }
    
    void reinitialize_test_buffers() {
        test_buffers_.clear();
        initialize_test_buffers();
    }
    
    void profiling_worker() {
        while (!should_stop_.load()) {
            auto start_time = std::chrono::steady_clock::now();
            
            // Perform quick bandwidth measurements
            auto measurement = measure_operation(MemoryOperation::SequentialRead, UINT32_MAX, 1);
            if (measurement.bandwidth_gbps > 0.0) {
                current_read_bandwidth_.store(measurement.bandwidth_gbps);
                
                // Update cumulative statistics
                total_bytes_read_.fetch_add(measurement.bytes_processed);
                total_operations_.fetch_add(measurement.operations_count);
            }
            
            auto write_measurement = measure_operation(MemoryOperation::SequentialWrite, UINT32_MAX, 1);
            if (write_measurement.bandwidth_gbps > 0.0) {
                current_write_bandwidth_.store(write_measurement.bandwidth_gbps);
                total_bytes_written_.fetch_add(write_measurement.bytes_processed);
            }
            
            // Update memory utilization estimate
            f64 utilization = (measurement.bandwidth_gbps + write_measurement.bandwidth_gbps) / 50.0; // Estimate based on ~50GB/s theoretical max
            current_memory_utilization_.store(std::min(utilization, 1.0));
            
            // Sleep until next measurement
            auto elapsed = std::chrono::steady_clock::now() - start_time;
            auto sleep_duration = std::chrono::duration<f64>(config_.measurement_interval_seconds) - elapsed;
            
            if (sleep_duration.count() > 0) {
                std::this_thread::sleep_for(sleep_duration);
            }
        }
    }
    
    // Individual measurement implementations
    BandwidthMeasurement measure_sequential_read(TestBuffer& buffer, u32 thread_count);
    BandwidthMeasurement measure_sequential_write(TestBuffer& buffer, u32 thread_count);
    BandwidthMeasurement measure_random_read(TestBuffer& buffer, u32 thread_count);
    BandwidthMeasurement measure_random_write(TestBuffer& buffer, u32 thread_count);
    BandwidthMeasurement measure_read_modify_write(TestBuffer& buffer, u32 thread_count);
    BandwidthMeasurement measure_copy_operation(TestBuffer& buffer, u32 thread_count);
    BandwidthMeasurement measure_set_operation(TestBuffer& buffer, u32 thread_count);
    BandwidthMeasurement measure_streaming_read(TestBuffer& buffer, u32 thread_count);
    BandwidthMeasurement measure_streaming_write(TestBuffer& buffer, u32 thread_count);
    
    f64 get_current_time() const {
        using namespace std::chrono;
        return duration<f64>(steady_clock::now().time_since_epoch()).count();
    }
};

//=============================================================================
// Memory Bottleneck Detection and Analysis
//=============================================================================

/**
 * @brief Memory bottleneck types
 */
enum class BottleneckType : u8 {
    BandwidthLimited,       // Memory bandwidth saturation
    LatencyLimited,         // Memory latency bottleneck
    CacheContention,        // Cache hierarchy contention
    NumaRemoteAccess,       // NUMA remote memory access
    ThreadContention,       // Multi-thread memory contention
    PrefetcherInefficiency, // Hardware prefetcher not working
    TlbMisses,              // Translation lookaside buffer misses
    MemoryFragmentation     // Memory layout fragmentation
};

/**
 * @brief Detected bottleneck information
 */
struct MemoryBottleneck {
    BottleneckType type;
    f64 severity_score;         // 0.0 (minor) to 1.0 (severe)
    f64 performance_impact;     // Estimated performance loss (0.0 to 1.0)
    std::string description;
    std::string recommendation;
    std::vector<BandwidthMeasurement> supporting_evidence;
    f64 detection_confidence;   // Confidence in detection (0.0 to 1.0)
    
    // Specific metrics depending on bottleneck type
    std::unordered_map<std::string, f64> metrics;
};

/**
 * @brief Advanced bottleneck detection system
 */
class MemoryBottleneckDetector {
private:
    MemoryBandwidthProfiler& bandwidth_profiler_;
    numa::NumaManager& numa_manager_;
    cache::CacheTopologyAnalyzer& cache_analyzer_;
    
    // Detection thresholds
    struct DetectionThresholds {
        f64 bandwidth_saturation_threshold = 0.85;     // 85% of peak bandwidth
        f64 latency_degradation_threshold = 2.0;       // 2x normal latency
        f64 numa_penalty_threshold = 1.5;              // 1.5x NUMA penalty
        f64 cache_miss_threshold = 0.5;                // 50% cache miss rate
        f64 thread_contention_threshold = 0.7;         // 70% efficiency loss
    };
    
    DetectionThresholds thresholds_;
    
    // Historical data for trend analysis
    struct PerformanceHistory {
        std::vector<f64> bandwidth_samples;
        std::vector<f64> latency_samples;
        std::vector<f64> utilization_samples;
        std::chrono::steady_clock::time_point last_update;
        
        void add_sample(f64 bandwidth, f64 latency, f64 utilization) {
            bandwidth_samples.push_back(bandwidth);
            latency_samples.push_back(latency);
            utilization_samples.push_back(utilization);
            
            // Keep only recent samples
            if (bandwidth_samples.size() > 1000) {
                bandwidth_samples.erase(bandwidth_samples.begin(), bandwidth_samples.begin() + 500);
                latency_samples.erase(latency_samples.begin(), latency_samples.begin() + 500);
                utilization_samples.erase(utilization_samples.begin(), utilization_samples.begin() + 500);
            }
            
            last_update = std::chrono::steady_clock::now();
        }
        
        f64 get_trend() const {
            if (bandwidth_samples.size() < 10) return 0.0;
            
            // Simple linear regression for trend detection
            usize n = bandwidth_samples.size();
            usize start_idx = n >= 100 ? n - 100 : 0;
            
            f64 sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
            for (usize i = start_idx; i < n; ++i) {
                f64 x = static_cast<f64>(i - start_idx);
                f64 y = bandwidth_samples[i];
                sum_x += x;
                sum_y += y;
                sum_xy += x * y;
                sum_x2 += x * x;
            }
            
            usize count = n - start_idx;
            if (count < 2) return 0.0;
            
            f64 slope = (count * sum_xy - sum_x * sum_y) / (count * sum_x2 - sum_x * sum_x);
            return slope;
        }
    };
    
    PerformanceHistory performance_history_;
    
    // Detected bottlenecks
    std::vector<MemoryBottleneck> detected_bottlenecks_;
    mutable std::mutex bottlenecks_mutex_;
    
public:
    explicit MemoryBottleneckDetector(MemoryBandwidthProfiler& profiler,
                                    numa::NumaManager& numa_mgr,
                                    cache::CacheTopologyAnalyzer& cache_analyzer)
        : bandwidth_profiler_(profiler), numa_manager_(numa_mgr), cache_analyzer_(cache_analyzer) {
        performance_history_.last_update = std::chrono::steady_clock::now();
    }
    
    /**
     * @brief Analyze current system state and detect bottlenecks
     */
    std::vector<MemoryBottleneck> detect_bottlenecks() {
        PROFILE_FUNCTION();
        
        std::vector<MemoryBottleneck> bottlenecks;
        
        // Get current performance data
        auto real_time_stats = bandwidth_profiler_.get_real_time_stats();
        auto measurement_history = bandwidth_profiler_.get_measurement_history();
        
        if (measurement_history.empty()) {
            LOG_WARNING("No measurement history available for bottleneck detection");
            return bottlenecks;
        }
        
        // Update performance history
        f64 current_bandwidth = real_time_stats.current_read_bandwidth_gbps + real_time_stats.current_write_bandwidth_gbps;
        f64 avg_latency = calculate_average_latency(measurement_history);
        performance_history_.add_sample(current_bandwidth, avg_latency, real_time_stats.current_memory_utilization);
        
        // Detect different types of bottlenecks
        detect_bandwidth_bottlenecks(bottlenecks, measurement_history, real_time_stats);
        detect_latency_bottlenecks(bottlenecks, measurement_history, real_time_stats);
        detect_numa_bottlenecks(bottlenecks, measurement_history);
        detect_cache_bottlenecks(bottlenecks, measurement_history);
        detect_thread_contention_bottlenecks(bottlenecks, measurement_history);
        
        // Sort by severity
        std::sort(bottlenecks.begin(), bottlenecks.end(),
                 [](const auto& a, const auto& b) { return a.severity_score > b.severity_score; });
        
        // Update stored bottlenecks
        {
            std::lock_guard<std::mutex> lock(bottlenecks_mutex_);
            detected_bottlenecks_ = bottlenecks;
        }
        
        LOG_INFO("Detected {} memory bottlenecks", bottlenecks.size());
        return bottlenecks;
    }
    
    /**
     * @brief Get currently detected bottlenecks
     */
    std::vector<MemoryBottleneck> get_current_bottlenecks() const {
        std::lock_guard<std::mutex> lock(bottlenecks_mutex_);
        return detected_bottlenecks_;
    }
    
    /**
     * @brief Generate bottleneck report
     */
    std::string generate_bottleneck_report() const {
        std::ostringstream report;
        
        auto bottlenecks = get_current_bottlenecks();
        
        report << "=== Memory Bottleneck Analysis Report ===\n\n";
        
        if (bottlenecks.empty()) {
            report << "No significant memory bottlenecks detected.\n";
            report << "System appears to be operating within normal parameters.\n";
        } else {
            report << "Detected " << bottlenecks.size() << " bottleneck(s):\n\n";
            
            for (usize i = 0; i < bottlenecks.size(); ++i) {
                const auto& bottleneck = bottlenecks[i];
                
                report << i + 1 << ". " << bottleneck_type_to_string(bottleneck.type) << "\n";
                report << "   Severity: " << std::fixed << std::setprecision(2) 
                       << bottleneck.severity_score * 100 << "%\n";
                report << "   Performance Impact: " << std::fixed << std::setprecision(1)
                       << bottleneck.performance_impact * 100 << "%\n";
                report << "   Confidence: " << std::fixed << std::setprecision(1)
                       << bottleneck.detection_confidence * 100 << "%\n";
                report << "   Description: " << bottleneck.description << "\n";
                report << "   Recommendation: " << bottleneck.recommendation << "\n";
                
                if (!bottleneck.metrics.empty()) {
                    report << "   Metrics:\n";
                    for (const auto& [key, value] : bottleneck.metrics) {
                        report << "     " << key << ": " << std::fixed << std::setprecision(2) << value << "\n";
                    }
                }
                
                report << "\n";
            }
        }
        
        // Add system overview
        auto real_time_stats = bandwidth_profiler_.get_real_time_stats();
        report << "Current System Status:\n";
        report << "  Read Bandwidth: " << std::fixed << std::setprecision(2) 
               << real_time_stats.current_read_bandwidth_gbps << " GB/s\n";
        report << "  Write Bandwidth: " << std::fixed << std::setprecision(2)
               << real_time_stats.current_write_bandwidth_gbps << " GB/s\n";
        report << "  Memory Utilization: " << std::fixed << std::setprecision(1)
               << real_time_stats.current_memory_utilization * 100 << "%\n";
        
        f64 trend = performance_history_.get_trend();
        report << "  Performance Trend: ";
        if (trend > 0.1) {
            report << "Improving";
        } else if (trend < -0.1) {
            report << "Degrading";
        } else {
            report << "Stable";
        }
        report << " (" << std::fixed << std::setprecision(3) << trend << ")\n";
        
        return report.str();
    }
    
    /**
     * @brief Configure detection thresholds
     */
    void set_detection_thresholds(const DetectionThresholds& thresholds) {
        thresholds_ = thresholds;
    }
    
    DetectionThresholds get_detection_thresholds() const { return thresholds_; }
    
private:
    void detect_bandwidth_bottlenecks(std::vector<MemoryBottleneck>& bottlenecks,
                                     const std::vector<BandwidthMeasurement>& measurements,
                                     const MemoryBandwidthProfiler::RealTimeStats& stats) {
        
        // Find peak bandwidth from measurements
        f64 peak_bandwidth = 0.0;
        for (const auto& measurement : measurements) {
            peak_bandwidth = std::max(peak_bandwidth, measurement.bandwidth_gbps);
        }
        
        f64 current_bandwidth = stats.current_read_bandwidth_gbps + stats.current_write_bandwidth_gbps;
        
        if (peak_bandwidth > 0 && current_bandwidth / peak_bandwidth > thresholds_.bandwidth_saturation_threshold) {
            MemoryBottleneck bottleneck;
            bottleneck.type = BottleneckType::BandwidthLimited;
            bottleneck.severity_score = current_bandwidth / peak_bandwidth;
            bottleneck.performance_impact = 0.5; // Estimate
            bottleneck.description = "Memory bandwidth utilization is high, approaching system limits";
            bottleneck.recommendation = "Consider reducing memory-intensive operations, optimizing data layouts, or using faster memory";
            bottleneck.detection_confidence = 0.8;
            
            bottleneck.metrics["current_bandwidth_gbps"] = current_bandwidth;
            bottleneck.metrics["peak_bandwidth_gbps"] = peak_bandwidth;
            bottleneck.metrics["utilization_ratio"] = bottleneck.severity_score;
            
            bottlenecks.push_back(bottleneck);
        }
    }
    
    void detect_latency_bottlenecks(std::vector<MemoryBottleneck>& bottlenecks,
                                   const std::vector<BandwidthMeasurement>& measurements,
                                   const MemoryBandwidthProfiler::RealTimeStats& stats) {
        
        f64 avg_latency = calculate_average_latency(measurements);
        f64 baseline_latency = 100.0; // Assume 100ns baseline
        
        if (avg_latency > baseline_latency * thresholds_.latency_degradation_threshold) {
            MemoryBottleneck bottleneck;
            bottleneck.type = BottleneckType::LatencyLimited;
            bottleneck.severity_score = std::min(avg_latency / baseline_latency / 5.0, 1.0);
            bottleneck.performance_impact = 0.3;
            bottleneck.description = "Memory access latency is higher than expected";
            bottleneck.recommendation = "Check for memory fragmentation, optimize access patterns, or use prefetching";
            bottleneck.detection_confidence = 0.7;
            
            bottleneck.metrics["average_latency_ns"] = avg_latency;
            bottleneck.metrics["baseline_latency_ns"] = baseline_latency;
            bottleneck.metrics["latency_multiplier"] = avg_latency / baseline_latency;
            
            bottlenecks.push_back(bottleneck);
        }
    }
    
    void detect_numa_bottlenecks(std::vector<MemoryBottleneck>& bottlenecks,
                                const std::vector<BandwidthMeasurement>& measurements) {
        
        if (!numa_manager_.is_numa_available()) return;
        
        // Group measurements by NUMA node
        std::unordered_map<u32, std::vector<const BandwidthMeasurement*>> node_measurements;
        for (const auto& measurement : measurements) {
            node_measurements[measurement.numa_node].push_back(&measurement);
        }
        
        if (node_measurements.size() < 2) return; // Need multiple nodes to detect NUMA issues
        
        // Calculate bandwidth differences between nodes
        f64 max_bandwidth = 0.0;
        f64 min_bandwidth = std::numeric_limits<f64>::max();
        
        for (const auto& [node_id, node_measurements_vec] : node_measurements) {
            f64 avg_bandwidth = 0.0;
            for (const auto* measurement : node_measurements_vec) {
                avg_bandwidth += measurement->bandwidth_gbps;
            }
            avg_bandwidth /= node_measurements_vec.size();
            
            max_bandwidth = std::max(max_bandwidth, avg_bandwidth);
            min_bandwidth = std::min(min_bandwidth, avg_bandwidth);
        }
        
        if (max_bandwidth > 0 && max_bandwidth / min_bandwidth > thresholds_.numa_penalty_threshold) {
            MemoryBottleneck bottleneck;
            bottleneck.type = BottleneckType::NumaRemoteAccess;
            bottleneck.severity_score = 1.0 - (min_bandwidth / max_bandwidth);
            bottleneck.performance_impact = bottleneck.severity_score * 0.4;
            bottleneck.description = "Significant NUMA remote memory access penalty detected";
            bottleneck.recommendation = "Optimize thread affinity and memory allocation to prefer local NUMA nodes";
            bottleneck.detection_confidence = 0.85;
            
            bottleneck.metrics["max_node_bandwidth_gbps"] = max_bandwidth;
            bottleneck.metrics["min_node_bandwidth_gbps"] = min_bandwidth;
            bottleneck.metrics["numa_penalty_factor"] = max_bandwidth / min_bandwidth;
            
            bottlenecks.push_back(bottleneck);
        }
    }
    
    void detect_cache_bottlenecks(std::vector<MemoryBottleneck>& bottlenecks,
                                 const std::vector<BandwidthMeasurement>& measurements) {
        
        f64 total_cache_miss_ratio = 0.0;
        usize valid_measurements = 0;
        
        for (const auto& measurement : measurements) {
            if (measurement.cache_miss_ratio >= 0.0) {
                total_cache_miss_ratio += measurement.cache_miss_ratio;
                ++valid_measurements;
            }
        }
        
        if (valid_measurements == 0) return;
        
        f64 avg_cache_miss_ratio = total_cache_miss_ratio / valid_measurements;
        
        if (avg_cache_miss_ratio > thresholds_.cache_miss_threshold) {
            MemoryBottleneck bottleneck;
            bottleneck.type = BottleneckType::CacheContention;
            bottleneck.severity_score = avg_cache_miss_ratio;
            bottleneck.performance_impact = avg_cache_miss_ratio * 0.6;
            bottleneck.description = "High cache miss rate indicates inefficient memory access patterns";
            bottleneck.recommendation = "Optimize data structures for cache locality, use cache-friendly algorithms";
            bottleneck.detection_confidence = 0.75;
            
            bottleneck.metrics["average_cache_miss_ratio"] = avg_cache_miss_ratio;
            bottleneck.metrics["cache_miss_threshold"] = thresholds_.cache_miss_threshold;
            
            bottlenecks.push_back(bottleneck);
        }
    }
    
    void detect_thread_contention_bottlenecks(std::vector<MemoryBottleneck>& bottlenecks,
                                             const std::vector<BandwidthMeasurement>& measurements) {
        
        // Group measurements by thread count
        std::unordered_map<u32, std::vector<const BandwidthMeasurement*>> thread_measurements;
        for (const auto& measurement : measurements) {
            if (measurement.thread_count > 0) {
                thread_measurements[measurement.thread_count].push_back(&measurement);
            }
        }
        
        if (thread_measurements.size() < 2) return;
        
        // Calculate scaling efficiency
        auto single_thread_it = thread_measurements.find(1);
        if (single_thread_it == thread_measurements.end()) return;
        
        f64 single_thread_bandwidth = 0.0;
        for (const auto* measurement : single_thread_it->second) {
            single_thread_bandwidth += measurement->bandwidth_gbps;
        }
        single_thread_bandwidth /= single_thread_it->second.size();
        
        for (const auto& [thread_count, thread_measurements_vec] : thread_measurements) {
            if (thread_count <= 1) continue;
            
            f64 multi_thread_bandwidth = 0.0;
            for (const auto* measurement : thread_measurements_vec) {
                multi_thread_bandwidth += measurement->bandwidth_gbps;
            }
            multi_thread_bandwidth /= thread_measurements_vec.size();
            
            f64 expected_bandwidth = single_thread_bandwidth * thread_count;
            f64 efficiency = multi_thread_bandwidth / expected_bandwidth;
            
            if (efficiency < thresholds_.thread_contention_threshold) {
                MemoryBottleneck bottleneck;
                bottleneck.type = BottleneckType::ThreadContention;
                bottleneck.severity_score = 1.0 - efficiency;
                bottleneck.performance_impact = bottleneck.severity_score * 0.3;
                bottleneck.description = "Multi-threaded memory access shows poor scaling efficiency";
                bottleneck.recommendation = "Reduce thread contention, use thread-local storage, optimize synchronization";
                bottleneck.detection_confidence = 0.8;
                
                bottleneck.metrics["thread_count"] = static_cast<f64>(thread_count);
                bottleneck.metrics["scaling_efficiency"] = efficiency;
                bottleneck.metrics["single_thread_bandwidth_gbps"] = single_thread_bandwidth;
                bottleneck.metrics["multi_thread_bandwidth_gbps"] = multi_thread_bandwidth;
                
                bottlenecks.push_back(bottleneck);
                break; // Only report one thread contention bottleneck
            }
        }
    }
    
    f64 calculate_average_latency(const std::vector<BandwidthMeasurement>& measurements) const {
        if (measurements.empty()) return 0.0;
        
        f64 total_latency = 0.0;
        usize valid_measurements = 0;
        
        for (const auto& measurement : measurements) {
            if (measurement.latency_ns > 0.0) {
                total_latency += measurement.latency_ns;
                ++valid_measurements;
            }
        }
        
        return valid_measurements > 0 ? total_latency / valid_measurements : 0.0;
    }
    
    static std::string bottleneck_type_to_string(BottleneckType type) {
        switch (type) {
            case BottleneckType::BandwidthLimited: return "Bandwidth Limited";
            case BottleneckType::LatencyLimited: return "Latency Limited";
            case BottleneckType::CacheContention: return "Cache Contention";
            case BottleneckType::NumaRemoteAccess: return "NUMA Remote Access";
            case BottleneckType::ThreadContention: return "Thread Contention";
            case BottleneckType::PrefetcherInefficiency: return "Prefetcher Inefficiency";
            case BottleneckType::TlbMisses: return "TLB Misses";
            case BottleneckType::MemoryFragmentation: return "Memory Fragmentation";
            default: return "Unknown";
        }
    }
};

//=============================================================================
// Global Bandwidth Analysis System
//=============================================================================

/**
 * @brief Global bandwidth analyzer instance
 */
MemoryBandwidthProfiler& get_global_bandwidth_profiler();

/**
 * @brief Global bottleneck detector instance
 */
MemoryBottleneckDetector& get_global_bottleneck_detector();

} // namespace ecscope::memory::bandwidth