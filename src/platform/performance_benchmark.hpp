#pragma once

/**
 * @file platform/performance_benchmark.hpp
 * @brief Comprehensive Performance Benchmarking Suite for Hardware Validation
 * 
 * This system provides extensive benchmarking capabilities to validate hardware
 * detection, measure optimization impacts, and provide educational insights
 * into performance characteristics across different platforms and architectures.
 * 
 * Key Features:
 * - CPU performance benchmarks (integer, floating-point, SIMD)
 * - Memory system benchmarks (bandwidth, latency, cache behavior)
 * - Platform-specific performance tests
 * - Optimization validation benchmarks
 * - Cross-architecture performance comparisons
 * - Educational performance analysis and visualization
 * 
 * Educational Value:
 * - Real-time performance impact demonstration
 * - Hardware bottleneck identification
 * - Optimization effectiveness measurement
 * - Cross-platform performance analysis
 * - Performance tuning methodology teaching
 * 
 * @author ECScope Educational ECS Framework - Performance Benchmarking
 * @date 2025
 */

#include "hardware_detection.hpp"
#include "core/types.hpp"
#include <chrono>
#include <functional>
#include <vector>
#include <string>
#include <memory>
#include <random>
#include <atomic>
#include <thread>

namespace ecscope::platform::benchmark {

//=============================================================================
// Benchmark Infrastructure
//=============================================================================

/**
 * @brief High-precision timing utilities
 */
class HighResolutionTimer {
private:
    std::chrono::high_resolution_clock::time_point start_time_;
    std::chrono::high_resolution_clock::time_point end_time_;
    bool is_running_ = false;
    
public:
    void start();
    void stop();
    void reset();
    
    std::chrono::nanoseconds get_elapsed_ns() const;
    std::chrono::microseconds get_elapsed_us() const;
    std::chrono::milliseconds get_elapsed_ms() const;
    f64 get_elapsed_seconds() const;
    
    // Static convenience methods
    static std::chrono::nanoseconds measure(const std::function<void()>& func);
    
    template<typename Func, typename... Args>
    static auto measure_with_result(Func&& func, Args&&... args) {
        auto start = std::chrono::high_resolution_clock::now();
        if constexpr (std::is_void_v<std::invoke_result_t<Func, Args...>>) {
            std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
            auto end = std::chrono::high_resolution_clock::now();
            return std::make_pair(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start), 0);
        } else {
            auto result = std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
            auto end = std::chrono::high_resolution_clock::now();
            return std::make_pair(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start), result);
        }
    }
};

/**
 * @brief Statistical analysis of benchmark results
 */
struct BenchmarkStatistics {
    f64 mean = 0.0;
    f64 median = 0.0;
    f64 min = 0.0;
    f64 max = 0.0;
    f64 std_dev = 0.0;
    f64 variance = 0.0;
    f64 percentile_95 = 0.0;
    f64 percentile_99 = 0.0;
    u32 sample_count = 0;
    
    void calculate_from_samples(const std::vector<f64>& samples);
    std::string get_summary() const;
    bool is_statistically_significant(const BenchmarkStatistics& other, f64 threshold = 0.05) const;
};

/**
 * @brief Individual benchmark result with detailed metrics
 */
struct BenchmarkResult {
    std::string benchmark_name;
    std::string test_configuration;
    std::string hardware_signature;
    
    // Timing results
    BenchmarkStatistics timing_stats;
    std::vector<std::chrono::nanoseconds> raw_timings;
    
    // Performance metrics
    f64 operations_per_second = 0.0;
    f64 throughput_mbps = 0.0;
    f64 efficiency_score = 0.0;
    
    // Resource utilization
    f64 cpu_utilization_percent = 0.0;
    f64 memory_usage_mb = 0.0;
    f64 cache_hit_rate_percent = 0.0;
    f64 memory_bandwidth_utilization = 0.0;
    
    // System state
    f64 cpu_temperature_celsius = 0.0;
    f64 thermal_throttling_factor = 1.0;
    f64 power_consumption_watts = 0.0;
    
    // Custom metrics
    std::unordered_map<std::string, f64> custom_metrics;
    
    std::chrono::system_clock::time_point timestamp;
    
    // Analysis methods
    f64 calculate_performance_score() const;
    std::string get_formatted_report() const;
    bool compare_with(const BenchmarkResult& other, f64& improvement_factor) const;
};

//=============================================================================
// Core Benchmark Framework
//=============================================================================

/**
 * @brief Base class for all benchmarks
 */
class IBenchmark {
public:
    virtual ~IBenchmark() = default;
    
    virtual std::string get_name() const = 0;
    virtual std::string get_description() const = 0;
    virtual std::string get_category() const = 0;
    
    virtual void setup() {}
    virtual void teardown() {}
    virtual void run_iteration() = 0;
    
    virtual bool is_hardware_supported(const HardwareDetector& detector) const = 0;
    virtual std::vector<std::string> get_required_features() const { return {}; }
    
    // Configuration
    virtual void set_problem_size(u64 size) { problem_size_ = size; }
    virtual u64 get_problem_size() const { return problem_size_; }
    
    virtual void set_thread_count(u32 threads) { thread_count_ = threads; }
    virtual u32 get_thread_count() const { return thread_count_; }
    
protected:
    u64 problem_size_ = 1000000;  // Default problem size
    u32 thread_count_ = 1;        // Default single-threaded
    std::mt19937 rng_{std::random_device{}()};
};

/**
 * @brief Benchmark execution engine
 */
class BenchmarkExecutor {
private:
    struct ExecutionConfig {
        u32 warmup_iterations = 5;
        u32 measurement_iterations = 10;
        f64 min_execution_time_seconds = 1.0;
        f64 max_execution_time_seconds = 30.0;
        bool collect_system_metrics = true;
        bool enable_thermal_monitoring = false;
        f64 thermal_throttling_threshold = 85.0; // Celsius
    };
    
    ExecutionConfig config_;
    HardwareDetector& hardware_detector_;
    std::vector<std::unique_ptr<IBenchmark>> benchmarks_;
    
    // System monitoring
    std::atomic<bool> monitoring_active_{false};
    std::thread monitoring_thread_;
    
public:
    explicit BenchmarkExecutor(HardwareDetector& detector);
    ~BenchmarkExecutor();
    
    // Configuration
    void set_warmup_iterations(u32 iterations);
    void set_measurement_iterations(u32 iterations);
    void set_execution_time_limits(f64 min_seconds, f64 max_seconds);
    void enable_system_monitoring(bool enable);
    void enable_thermal_monitoring(bool enable, f64 threshold_celsius = 85.0);
    
    // Benchmark management
    void register_benchmark(std::unique_ptr<IBenchmark> benchmark);
    void register_all_standard_benchmarks();
    std::vector<std::string> get_available_benchmarks() const;
    std::vector<std::string> get_supported_benchmarks() const;
    
    // Execution
    BenchmarkResult run_benchmark(const std::string& name);
    std::vector<BenchmarkResult> run_benchmark_suite(const std::vector<std::string>& names = {});
    std::vector<BenchmarkResult> run_comparison_suite(
        const std::vector<std::string>& configurations);
    
    // Analysis and reporting
    std::string generate_system_performance_report() const;
    std::string generate_comparison_report(const std::vector<BenchmarkResult>& results) const;
    void export_results_csv(const std::vector<BenchmarkResult>& results, 
                           const std::string& filename) const;
    
private:
    void start_system_monitoring();
    void stop_system_monitoring();
    void system_monitoring_worker();
    
    BenchmarkResult execute_single_benchmark(IBenchmark& benchmark);
    void collect_system_metrics(BenchmarkResult& result) const;
    bool should_stop_due_to_thermal_throttling() const;
};

//=============================================================================
// CPU Performance Benchmarks
//=============================================================================

/**
 * @brief Integer arithmetic performance benchmark
 */
class IntegerArithmeticBenchmark : public IBenchmark {
private:
    std::vector<i64> data_a_;
    std::vector<i64> data_b_;
    std::vector<i64> results_;
    
public:
    std::string get_name() const override { return "integer_arithmetic"; }
    std::string get_description() const override { 
        return "Integer addition, multiplication, and division performance"; 
    }
    std::string get_category() const override { return "CPU"; }
    
    void setup() override;
    void run_iteration() override;
    bool is_hardware_supported(const HardwareDetector& detector) const override;
};

/**
 * @brief Floating-point arithmetic performance benchmark
 */
class FloatingPointBenchmark : public IBenchmark {
private:
    std::vector<f64> data_a_;
    std::vector<f64> data_b_;
    std::vector<f64> results_;
    
public:
    std::string get_name() const override { return "floating_point"; }
    std::string get_description() const override { 
        return "Double-precision floating-point arithmetic performance"; 
    }
    std::string get_category() const override { return "CPU"; }
    
    void setup() override;
    void run_iteration() override;
    bool is_hardware_supported(const HardwareDetector& detector) const override;
};

/**
 * @brief SIMD vectorized operations benchmark
 */
class SimdBenchmark : public IBenchmark {
private:
    alignas(64) std::vector<f32> data_a_;
    alignas(64) std::vector<f32> data_b_;
    alignas(64) std::vector<f32> results_;
    std::string simd_level_;
    
public:
    explicit SimdBenchmark(const std::string& simd_level = "auto");
    
    std::string get_name() const override { return "simd_" + simd_level_; }
    std::string get_description() const override { 
        return "SIMD vectorized arithmetic using " + simd_level_; 
    }
    std::string get_category() const override { return "SIMD"; }
    
    void setup() override;
    void run_iteration() override;
    bool is_hardware_supported(const HardwareDetector& detector) const override;
    std::vector<std::string> get_required_features() const override;
    
private:
    void run_scalar_iteration();
    void run_sse_iteration();
    void run_avx_iteration();
    void run_avx2_iteration();
    void run_avx512_iteration();
    void run_neon_iteration();
};

/**
 * @brief Branch prediction performance benchmark
 */
class BranchPredictionBenchmark : public IBenchmark {
private:
    std::vector<i32> random_data_;
    std::vector<i32> sorted_data_;
    i64 result_sum_ = 0;
    
public:
    std::string get_name() const override { return "branch_prediction"; }
    std::string get_description() const override { 
        return "Branch prediction efficiency with random vs. sorted data"; 
    }
    std::string get_category() const override { return "CPU"; }
    
    void setup() override;
    void run_iteration() override;
    bool is_hardware_supported(const HardwareDetector& detector) const override;
};

//=============================================================================
// Memory System Benchmarks
//=============================================================================

/**
 * @brief Memory bandwidth benchmark
 */
class MemoryBandwidthBenchmark : public IBenchmark {
public:
    enum class AccessPattern {
        Sequential,
        Random,
        Strided
    };
    
private:
    std::vector<u8> source_buffer_;
    std::vector<u8> dest_buffer_;
    AccessPattern access_pattern_;
    std::vector<usize> access_indices_;
    
public:
    explicit MemoryBandwidthBenchmark(AccessPattern pattern = AccessPattern::Sequential);
    
    std::string get_name() const override;
    std::string get_description() const override;
    std::string get_category() const override { return "Memory"; }
    
    void setup() override;
    void run_iteration() override;
    bool is_hardware_supported(const HardwareDetector& detector) const override;
    
private:
    void generate_access_pattern();
    void run_sequential_copy();
    void run_random_copy();
    void run_strided_copy();
};

/**
 * @brief Memory latency benchmark
 */
class MemoryLatencyBenchmark : public IBenchmark {
private:
    std::vector<usize> chase_pointers_;
    usize current_index_ = 0;
    
public:
    std::string get_name() const override { return "memory_latency"; }
    std::string get_description() const override { 
        return "Memory access latency measurement using pointer chasing"; 
    }
    std::string get_category() const override { return "Memory"; }
    
    void setup() override;
    void run_iteration() override;
    bool is_hardware_supported(const HardwareDetector& detector) const override;
    
private:
    void create_pointer_chain();
};

/**
 * @brief Cache hierarchy benchmark
 */
class CacheBenchmark : public IBenchmark {
private:
    std::vector<u8> test_data_;
    std::vector<usize> access_sizes_;
    std::unordered_map<usize, f64> latency_by_size_;
    
public:
    std::string get_name() const override { return "cache_hierarchy"; }
    std::string get_description() const override { 
        return "Cache hierarchy performance analysis"; 
    }
    std::string get_category() const override { return "Cache"; }
    
    void setup() override;
    void run_iteration() override;
    bool is_hardware_supported(const HardwareDetector& detector) const override;
    
    // Analysis methods
    std::vector<std::pair<usize, f64>> get_cache_latency_profile() const;
    std::string analyze_cache_behavior() const;
};

//=============================================================================
// Multi-threading Benchmarks
//=============================================================================

/**
 * @brief Thread scaling performance benchmark
 */
class ThreadScalingBenchmark : public IBenchmark {
private:
    std::vector<std::atomic<u64>> counters_;
    std::vector<std::thread> worker_threads_;
    std::atomic<bool> should_stop_{false};
    
public:
    std::string get_name() const override { return "thread_scaling"; }
    std::string get_description() const override { 
        return "Multi-thread performance scaling analysis"; 
    }
    std::string get_category() const override { return "Threading"; }
    
    void setup() override;
    void teardown() override;
    void run_iteration() override;
    bool is_hardware_supported(const HardwareDetector& detector) const override;
    
private:
    void worker_function(u32 thread_id);
};

/**
 * @brief Lock contention benchmark
 */
class LockContentionBenchmark : public IBenchmark {
private:
    std::mutex shared_mutex_;
    std::atomic<u64> shared_counter_{0};
    std::vector<std::thread> threads_;
    std::atomic<bool> should_stop_{false};
    
public:
    std::string get_name() const override { return "lock_contention"; }
    std::string get_description() const override { 
        return "Lock contention impact on multi-threaded performance"; 
    }
    std::string get_category() const override { return "Threading"; }
    
    void setup() override;
    void teardown() override;
    void run_iteration() override;
    bool is_hardware_supported(const HardwareDetector& detector) const override;
    
private:
    void contention_worker();
    void lockfree_worker();
};

//=============================================================================
// Platform-Specific Benchmarks
//=============================================================================

/**
 * @brief System call overhead benchmark
 */
class SystemCallBenchmark : public IBenchmark {
private:
    std::vector<std::chrono::nanoseconds> syscall_times_;
    
public:
    std::string get_name() const override { return "system_call_overhead"; }
    std::string get_description() const override { 
        return "Operating system call overhead measurement"; 
    }
    std::string get_category() const override { return "Platform"; }
    
    void setup() override;
    void run_iteration() override;
    bool is_hardware_supported(const HardwareDetector& detector) const override;
    
private:
    void measure_getpid_overhead();
    void measure_clock_gettime_overhead();
    void measure_mmap_overhead();
};

/**
 * @brief Context switching benchmark
 */
class ContextSwitchBenchmark : public IBenchmark {
private:
    static constexpr u32 NUM_THREADS = 2;
    std::vector<std::thread> threads_;
    std::array<std::atomic<bool>, NUM_THREADS> ready_flags_{};
    std::atomic<u64> switch_count_{0};
    std::atomic<bool> should_stop_{false};
    
public:
    std::string get_name() const override { return "context_switch"; }
    std::string get_description() const override { 
        return "Thread context switching overhead measurement"; 
    }
    std::string get_category() const override { return "Platform"; }
    
    void setup() override;
    void teardown() override;
    void run_iteration() override;
    bool is_hardware_supported(const HardwareDetector& detector) const override;
    
private:
    void switching_worker(u32 thread_id);
};

//=============================================================================
// Optimization Validation Benchmarks
//=============================================================================

/**
 * @brief Before/after optimization comparison
 */
class OptimizationComparisonBenchmark {
private:
    std::string optimization_name_;
    std::function<void()> baseline_implementation_;
    std::function<void()> optimized_implementation_;
    BenchmarkExecutor& executor_;
    
public:
    OptimizationComparisonBenchmark(const std::string& name,
                                  std::function<void()> baseline,
                                  std::function<void()> optimized,
                                  BenchmarkExecutor& executor);
    
    struct ComparisonResult {
        BenchmarkResult baseline_result;
        BenchmarkResult optimized_result;
        f64 improvement_factor;
        f64 confidence_level;
        std::string analysis;
    };
    
    ComparisonResult run_comparison();
    std::string generate_comparison_report(const ComparisonResult& result) const;
};

//=============================================================================
// Educational Benchmark Demonstrations
//=============================================================================

/**
 * @brief Interactive benchmark demonstrations for educational purposes
 */
class EducationalBenchmarkSuite {
private:
    BenchmarkExecutor& executor_;
    HardwareDetector& hardware_detector_;
    
public:
    EducationalBenchmarkSuite(BenchmarkExecutor& executor, HardwareDetector& detector);
    
    // Educational demonstrations
    void demonstrate_cache_effects();
    void demonstrate_simd_benefits();
    void demonstrate_branch_prediction();
    void demonstrate_memory_hierarchy();
    void demonstrate_threading_scalability();
    void demonstrate_numa_effects();
    
    // Interactive tutorials
    void interactive_optimization_tutorial();
    void interactive_profiling_tutorial();
    void interactive_architecture_comparison();
    
    // Visualization and analysis
    std::string generate_educational_report() const;
    void visualize_performance_characteristics();
    void explain_benchmark_results(const BenchmarkResult& result) const;
    
private:
    void run_cache_size_sweep();
    void compare_simd_implementations();
    void analyze_branch_patterns();
    void measure_memory_access_patterns();
    void test_thread_scaling_limits();
};

//=============================================================================
// Global Benchmark System
//=============================================================================

/**
 * @brief Initialize the global benchmark system
 */
void initialize_benchmark_system();

/**
 * @brief Get the global benchmark executor
 */
BenchmarkExecutor& get_benchmark_executor();

/**
 * @brief Quick benchmark helpers
 */
namespace quick_bench {
    BenchmarkResult measure_cpu_performance();
    BenchmarkResult measure_memory_performance();
    BenchmarkResult measure_simd_performance();
    std::string get_system_performance_summary();
    f64 get_relative_performance_score();
}

} // namespace ecscope::platform::benchmark