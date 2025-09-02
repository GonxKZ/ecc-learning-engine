#pragma once

/**
 * @file allocation_benchmarks.hpp
 * @brief Allocation Strategy Benchmarks - Comprehensive allocator performance analysis
 * 
 * This component provides detailed benchmarking and comparison of different memory
 * allocation strategies used in ECScope, with a focus on educational insights into
 * how allocation policies impact performance in real-world scenarios.
 * 
 * Educational Mission:
 * - Demonstrate performance characteristics of Arena, Pool, and PMR allocators
 * - Show how allocation patterns affect performance and memory usage
 * - Visualize memory fragmentation and its impact on performance
 * - Provide hands-on experience with allocator tuning and optimization
 * - Illustrate the trade-offs between different allocation strategies
 * 
 * Key Benchmarks:
 * - Arena Allocator Analysis: Linear allocation performance and characteristics
 * - Pool Allocator Analysis: Fixed-size allocation efficiency
 * - PMR Adapter Analysis: Polymorphic memory resource performance
 * - Standard Allocator Comparison: Baseline performance comparisons
 * - Mixed Workload Analysis: Real-world allocation pattern simulation
 * - Fragmentation Impact Study: Memory layout efficiency analysis
 * - Hot Path Optimization: Critical allocation path performance
 * 
 * Performance Aspects Analyzed:
 * - Allocation speed and consistency
 * - Deallocation overhead (where applicable)
 * - Memory fragmentation and utilization
 * - Cache locality and access patterns
 * - Thread safety overhead
 * - Memory pressure handling
 * 
 * @author ECScope Educational ECS Framework
 * @date 2024
 */

#include "performance_lab.hpp"
#include "memory/memory_tracker.hpp"
#include "memory/arena.hpp"
#include "memory/pool.hpp"
#include "memory/pmr_adapters.hpp"
#include <vector>
#include <memory>
#include <chrono>
#include <random>
#include <functional>
#include <thread>
#include <memory_resource>

namespace ecscope::performance {

/**
 * @brief Allocation pattern types for benchmarking
 */
enum class AllocationPattern : u8 {
    Sequential,         // Sequential allocations of similar sizes
    Random,            // Random allocation sizes
    Burst,             // Burst allocations followed by quiet periods
    Steady,            // Steady allocation rate
    Mixed,             // Mixed allocation and deallocation
    Stress,            // High-frequency allocation stress test
    Realistic,         // Real-world ECS allocation patterns
    Pathological       // Worst-case allocation patterns
};

/**
 * @brief Allocator types for benchmarking
 */
enum class AllocatorType : u8 {
    StandardMalloc,     // Standard malloc/free
    Arena,              // Linear arena allocator
    Pool,               // Fixed-size pool allocator
    PMR_Arena,          // PMR-adapted arena allocator
    PMR_Pool,           // PMR-adapted pool allocator
    PMR_Monotonic,      // PMR monotonic buffer resource
    PMR_Synchronized,   // PMR synchronized pool resource
    Custom              // Custom allocator implementation
};

/**
 * @brief Allocation benchmark configuration
 */
struct AllocationBenchmarkConfig {
    AllocatorType allocator_type;   // Which allocator to test
    AllocationPattern pattern;      // Allocation pattern to use
    usize total_allocations;       // Total number of allocations
    usize min_allocation_size;     // Minimum allocation size
    usize max_allocation_size;     // Maximum allocation size
    usize arena_size;              // Arena size (if applicable)
    usize pool_block_size;         // Pool block size (if applicable)
    u32 thread_count;              // Number of concurrent threads
    f64 duration_seconds;          // Maximum benchmark duration
    u32 warmup_iterations;         // Warmup iterations
    bool measure_fragmentation;    // Enable fragmentation measurement
    bool measure_cache_performance; // Enable cache performance analysis
    u32 random_seed;               // Seed for reproducible results
    
    AllocationBenchmarkConfig() noexcept
        : allocator_type(AllocatorType::Arena), pattern(AllocationPattern::Sequential),
          total_allocations(100000), min_allocation_size(16), max_allocation_size(1024),
          arena_size(64 * 1024 * 1024), pool_block_size(64), thread_count(1),
          duration_seconds(10.0), warmup_iterations(1000), measure_fragmentation(true),
          measure_cache_performance(true), random_seed(42) {}
};

/**
 * @brief Detailed allocation benchmark result
 */
struct AllocationBenchmarkResult {
    std::string allocator_name;     // Human-readable allocator name
    AllocationBenchmarkConfig config; // Configuration used
    
    // Timing metrics
    f64 total_time_ms;             // Total benchmark time
    f64 allocation_time_ms;        // Time spent in allocations
    f64 deallocation_time_ms;      // Time spent in deallocations
    f64 average_allocation_time_ns; // Average allocation time
    f64 min_allocation_time_ns;    // Fastest allocation
    f64 max_allocation_time_ns;    // Slowest allocation
    f64 allocation_time_stddev_ns; // Allocation time standard deviation
    
    // Throughput metrics
    f64 allocations_per_second;    // Allocation throughput
    f64 megabytes_per_second;      // Memory throughput
    f64 peak_allocation_rate;      // Peak allocation rate achieved
    
    // Memory metrics
    usize total_memory_allocated;  // Total memory allocated during test
    usize peak_memory_usage;       // Peak simultaneous memory usage
    usize memory_overhead_bytes;   // Allocator overhead
    f64 memory_efficiency;         // Useful memory / total memory
    f64 fragmentation_ratio;       // External fragmentation measure
    usize internal_fragmentation;  // Bytes lost to internal fragmentation
    usize external_fragmentation;  // Bytes lost to external fragmentation
    
    // Cache performance (estimated)
    f64 cache_miss_rate;           // Estimated cache miss rate
    f64 cache_line_utilization;    // Average cache line utilization
    u64 estimated_cache_misses;    // Estimated total cache misses
    
    // Thread safety metrics (if multi-threaded)
    f64 lock_contention_ratio;     // Lock contention measure
    f64 thread_scaling_efficiency; // Threading efficiency
    
    // Quality metrics
    bool allocation_pattern_optimal; // Pattern suited to allocator
    f64 consistency_score;         // Allocation time consistency
    f64 predictability_score;      // Performance predictability
    
    // Educational insights
    std::vector<std::string> performance_characteristics;
    std::vector<std::string> optimization_opportunities;
    std::vector<std::string> use_case_recommendations;
    std::string allocator_description;
    
    AllocationBenchmarkResult() noexcept
        : total_time_ms(0.0), allocation_time_ms(0.0), deallocation_time_ms(0.0),
          average_allocation_time_ns(0.0), min_allocation_time_ns(0.0),
          max_allocation_time_ns(0.0), allocation_time_stddev_ns(0.0),
          allocations_per_second(0.0), megabytes_per_second(0.0),
          peak_allocation_rate(0.0), total_memory_allocated(0),
          peak_memory_usage(0), memory_overhead_bytes(0), memory_efficiency(0.0),
          fragmentation_ratio(0.0), internal_fragmentation(0),
          external_fragmentation(0), cache_miss_rate(0.0),
          cache_line_utilization(0.0), estimated_cache_misses(0),
          lock_contention_ratio(0.0), thread_scaling_efficiency(0.0),
          allocation_pattern_optimal(false), consistency_score(0.0),
          predictability_score(0.0) {}
};

/**
 * @brief Arena allocator benchmark
 */
class ArenaBenchmark : public IPerformanceExperiment {
private:
    std::unique_ptr<memory::ArenaAllocator> arena_;
    AllocationBenchmarkConfig config_;
    std::mt19937 rng_;
    
    // Allocation tracking
    struct AllocationRecord {
        void* ptr;
        usize size;
        std::chrono::high_resolution_clock::time_point timestamp;
    };
    
    std::vector<AllocationRecord> allocation_history_;
    std::vector<f64> allocation_times_;
    
    // Benchmark implementations
    AllocationBenchmarkResult run_sequential_benchmark();
    AllocationBenchmarkResult run_random_benchmark();
    AllocationBenchmarkResult run_burst_benchmark();
    AllocationBenchmarkResult run_stress_benchmark();
    
    // Analysis utilities
    void analyze_fragmentation(AllocationBenchmarkResult& result);
    void analyze_cache_performance(AllocationBenchmarkResult& result);
    void generate_educational_insights(AllocationBenchmarkResult& result);
    
public:
    explicit ArenaBenchmark(const AllocationBenchmarkConfig& config = AllocationBenchmarkConfig{});
    
    std::string get_name() const override { return "Arena Allocator Benchmark"; }
    std::string get_description() const override;
    std::string get_category() const override { return "Allocation"; }
    
    bool setup(const ExperimentConfig& config) override;
    BenchmarkResult execute() override;
    void cleanup() override;
    
    std::vector<PerformanceRecommendation> generate_recommendations() const override;
    
    // Direct benchmark execution
    AllocationBenchmarkResult run_benchmark();
    void set_config(const AllocationBenchmarkConfig& config);
};

/**
 * @brief Pool allocator benchmark
 */
class PoolBenchmark : public IPerformanceExperiment {
private:
    std::unique_ptr<memory::PoolAllocator> pool_;
    AllocationBenchmarkConfig config_;
    std::mt19937 rng_;
    
    // Pool-specific metrics
    struct PoolMetrics {
        usize blocks_allocated;
        usize blocks_free;
        f64 utilization_ratio;
        usize fragmentation_blocks;
    };
    
    std::vector<void*> active_allocations_;
    std::vector<f64> allocation_times_;
    std::vector<f64> deallocation_times_;
    
    // Benchmark implementations
    AllocationBenchmarkResult run_fixed_size_benchmark();
    AllocationBenchmarkResult run_allocation_deallocation_benchmark();
    AllocationBenchmarkResult run_pool_exhaustion_benchmark();
    AllocationBenchmarkResult run_fragmentation_benchmark();
    
    // Pool-specific analysis
    PoolMetrics analyze_pool_state();
    void analyze_pool_efficiency(AllocationBenchmarkResult& result);
    
public:
    explicit PoolBenchmark(const AllocationBenchmarkConfig& config = AllocationBenchmarkConfig{});
    
    std::string get_name() const override { return "Pool Allocator Benchmark"; }
    std::string get_description() const override;
    std::string get_category() const override { return "Allocation"; }
    
    bool setup(const ExperimentConfig& config) override;
    BenchmarkResult execute() override;
    void cleanup() override;
    
    std::vector<PerformanceRecommendation> generate_recommendations() const override;
    
    // Direct benchmark execution
    AllocationBenchmarkResult run_benchmark();
    void set_config(const AllocationBenchmarkConfig& config);
};

/**
 * @brief PMR allocator benchmark
 */
class PMRBenchmark : public IPerformanceExperiment {
private:
    std::unique_ptr<std::pmr::memory_resource> memory_resource_;
    AllocationBenchmarkConfig config_;
    std::mt19937 rng_;
    
    // PMR-specific setup
    void setup_monotonic_buffer_resource(usize buffer_size);
    void setup_synchronized_pool_resource();
    void setup_unsynchronized_pool_resource();
    
    // Benchmark implementations
    AllocationBenchmarkResult run_pmr_benchmark();
    AllocationBenchmarkResult run_polymorphism_overhead_test();
    
    // PMR-specific analysis
    void analyze_pmr_overhead(AllocationBenchmarkResult& result);
    
public:
    explicit PMRBenchmark(const AllocationBenchmarkConfig& config = AllocationBenchmarkConfig{});
    
    std::string get_name() const override { return "PMR Allocator Benchmark"; }
    std::string get_description() const override;
    std::string get_category() const override { return "Allocation"; }
    
    bool setup(const ExperimentConfig& config) override;
    BenchmarkResult execute() override;
    void cleanup() override;
    
    std::vector<PerformanceRecommendation> generate_recommendations() const override;
};

/**
 * @brief Standard allocator benchmark (baseline)
 */
class StandardAllocatorBenchmark : public IPerformanceExperiment {
private:
    AllocationBenchmarkConfig config_;
    std::mt19937 rng_;
    
    std::vector<void*> active_allocations_;
    std::vector<f64> allocation_times_;
    std::vector<f64> deallocation_times_;
    
    // Standard allocation benchmarks
    AllocationBenchmarkResult run_malloc_benchmark();
    AllocationBenchmarkResult run_new_delete_benchmark();
    AllocationBenchmarkResult run_fragmentation_analysis();
    
public:
    explicit StandardAllocatorBenchmark(const AllocationBenchmarkConfig& config = AllocationBenchmarkConfig{});
    
    std::string get_name() const override { return "Standard Allocator Benchmark"; }
    std::string get_description() const override;
    std::string get_category() const override { return "Allocation"; }
    
    bool setup(const ExperimentConfig& config) override;
    BenchmarkResult execute() override;
    void cleanup() override;
    
    std::vector<PerformanceRecommendation> generate_recommendations() const override;
};

/**
 * @brief Allocator comparison benchmark
 */
class AllocatorComparisonBenchmark : public IPerformanceExperiment {
private:
    std::vector<std::unique_ptr<IPerformanceExperiment>> allocator_benchmarks_;
    AllocationBenchmarkConfig base_config_;
    
    // Comparison metrics
    struct ComparisonResult {
        std::string best_allocator_overall;
        std::string best_for_speed;
        std::string best_for_memory_efficiency;
        std::string best_for_consistency;
        std::vector<std::pair<std::string, AllocationBenchmarkResult>> results;
        std::vector<std::string> recommendations;
    };
    
    ComparisonResult comparison_result_;
    
    // Analysis utilities
    void analyze_relative_performance();
    void generate_comparison_recommendations();
    
public:
    explicit AllocatorComparisonBenchmark(const AllocationBenchmarkConfig& config = AllocationBenchmarkConfig{});
    
    std::string get_name() const override { return "Allocator Performance Comparison"; }
    std::string get_description() const override;
    std::string get_category() const override { return "Allocation"; }
    
    bool setup(const ExperimentConfig& config) override;
    BenchmarkResult execute() override;
    void cleanup() override;
    
    std::vector<PerformanceRecommendation> generate_recommendations() const override;
    
    // Access comparison results
    const ComparisonResult& get_comparison_result() const { return comparison_result_; }
};

/**
 * @brief Memory fragmentation analyzer
 */
class FragmentationAnalyzer {
private:
    struct FragmentationMetrics {
        f64 external_fragmentation_ratio;  // External fragmentation percentage
        f64 internal_fragmentation_ratio;  // Internal fragmentation percentage
        usize largest_free_block;         // Size of largest free block
        usize smallest_free_block;        // Size of smallest free block
        u32 free_block_count;             // Number of free blocks
        f64 fragmentation_entropy;        // Entropy measure of fragmentation
        std::vector<usize> free_block_sizes; // Distribution of free block sizes
    };
    
    std::vector<std::pair<void*, usize>> allocations_;
    usize total_memory_size_;
    
public:
    explicit FragmentationAnalyzer(usize memory_size);
    
    void record_allocation(void* ptr, usize size);
    void record_deallocation(void* ptr, usize size);
    
    FragmentationMetrics analyze_fragmentation() const;
    f64 calculate_fragmentation_score() const;
    std::vector<std::string> generate_fragmentation_insights() const;
};

/**
 * @brief Allocation hot path profiler
 */
class AllocationHotPathProfiler {
private:
    struct HotPathMetrics {
        std::string allocation_source;      // Source of allocation (function/class)
        u64 call_count;                    // Number of calls
        f64 total_time_ms;                 // Total time spent
        f64 average_time_ns;               // Average time per call
        usize total_bytes_allocated;       // Total bytes allocated
        f64 percentage_of_total_time;      // Percentage of total allocation time
    };
    
    std::unordered_map<std::string, HotPathMetrics> hot_paths_;
    f64 total_allocation_time_;
    
public:
    AllocationHotPathProfiler();
    
    void record_allocation(const std::string& source, f64 time_ns, usize bytes);
    std::vector<HotPathMetrics> get_hot_paths(usize top_n = 10) const;
    std::vector<PerformanceRecommendation> generate_hot_path_recommendations() const;
};

/**
 * @brief Main allocation benchmarks coordinator
 */
class AllocationBenchmarks {
private:
    // Available benchmarks
    std::unique_ptr<ArenaBenchmark> arena_benchmark_;
    std::unique_ptr<PoolBenchmark> pool_benchmark_;
    std::unique_ptr<PMRBenchmark> pmr_benchmark_;
    std::unique_ptr<StandardAllocatorBenchmark> standard_benchmark_;
    std::unique_ptr<AllocatorComparisonBenchmark> comparison_benchmark_;
    
    // Analysis tools
    std::unique_ptr<FragmentationAnalyzer> fragmentation_analyzer_;
    std::unique_ptr<AllocationHotPathProfiler> hot_path_profiler_;
    
    // Integration with memory tracker
    memory::MemoryTracker& memory_tracker_;
    
    // Results cache
    std::unordered_map<std::string, AllocationBenchmarkResult> results_cache_;
    mutable std::mutex cache_mutex_;
    
    // Educational content
    std::unordered_map<std::string, std::string> allocator_explanations_;
    
    void initialize_educational_content();
    
public:
    AllocationBenchmarks();
    ~AllocationBenchmarks() = default;
    
    // Non-copyable, non-movable
    AllocationBenchmarks(const AllocationBenchmarks&) = delete;
    AllocationBenchmarks& operator=(const AllocationBenchmarks&) = delete;
    AllocationBenchmarks(AllocationBenchmarks&&) = delete;
    AllocationBenchmarks& operator=(AllocationBenchmarks&&) = delete;
    
    // Benchmark access
    ArenaBenchmark& get_arena_benchmark() { return *arena_benchmark_; }
    PoolBenchmark& get_pool_benchmark() { return *pool_benchmark_; }
    PMRBenchmark& get_pmr_benchmark() { return *pmr_benchmark_; }
    StandardAllocatorBenchmark& get_standard_benchmark() { return *standard_benchmark_; }
    AllocatorComparisonBenchmark& get_comparison_benchmark() { return *comparison_benchmark_; }
    
    // Quick benchmark runners
    AllocationBenchmarkResult run_arena_analysis(const AllocationBenchmarkConfig& config = AllocationBenchmarkConfig{});
    AllocationBenchmarkResult run_pool_analysis(const AllocationBenchmarkConfig& config = AllocationBenchmarkConfig{});
    AllocationBenchmarkResult run_pmr_analysis(const AllocationBenchmarkConfig& config = AllocationBenchmarkConfig{});
    AllocationBenchmarkResult run_standard_analysis(const AllocationBenchmarkConfig& config = AllocationBenchmarkConfig{});
    
    // Comprehensive analysis
    std::vector<AllocationBenchmarkResult> run_full_allocator_comparison(const AllocationBenchmarkConfig& config = AllocationBenchmarkConfig{});
    BenchmarkResult run_allocation_strategy_analysis();
    
    // Specialized analysis
    std::vector<PerformanceRecommendation> analyze_allocation_patterns();
    std::string generate_allocator_selection_guide() const;
    std::vector<std::string> identify_allocation_bottlenecks() const;
    
    // Results management
    std::vector<AllocationBenchmarkResult> get_all_results() const;
    std::optional<AllocationBenchmarkResult> get_result(const std::string& allocator_name) const;
    void clear_results_cache();
    
    // Educational features
    std::string get_allocator_explanation(const std::string& allocator_type) const;
    std::vector<std::string> get_available_explanations() const;
    std::vector<PerformanceRecommendation> get_allocation_optimization_recommendations() const;
    
    // Analysis utilities
    f64 calculate_allocation_efficiency_score() const;
    std::string generate_allocation_strategy_report() const;
    
    // Hot path profiling
    AllocationHotPathProfiler& get_hot_path_profiler() { return *hot_path_profiler_; }
    std::vector<PerformanceRecommendation> get_hot_path_recommendations() const;
    
    // Fragmentation analysis
    FragmentationAnalyzer& get_fragmentation_analyzer() { return *fragmentation_analyzer_; }
    f64 get_current_fragmentation_score() const;
};

} // namespace ecscope::performance