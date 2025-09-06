#pragma once

/**
 * @file ecs_performance_benchmarker.hpp
 * @brief Comprehensive ECS Performance Benchmarker and Comparison Tools
 * 
 * This system provides extensive benchmarking capabilities for ECS architectures,
 * memory patterns, cache performance, system timing, and cross-architectural
 * performance comparisons with educational visualizations and optimization
 * recommendations.
 * 
 * Key Features:
 * - ECS Architecture Comparison (SoA vs AoS, archetype vs component-based)
 * - Memory Pattern Analysis (cache locality, access patterns, fragmentation)
 * - Performance Scaling Tests (1-100K+ entities with stress testing)
 * - System Integration Benchmarks (physics, rendering, multi-threading)
 * - Educational Visualization (real-time graphs, bottleneck identification)
 * - Regression Testing Framework (automated performance validation)
 * - Comparative Analysis Tools (side-by-side architecture comparison)
 * 
 * Integration Points:
 * - Performance Lab: Core benchmarking infrastructure
 * - Memory Benchmark Suite: Memory allocation and access pattern analysis
 * - Sparse Set Visualizer: Component storage visualization
 * - Physics Benchmarks: Physics system integration testing
 * - Visual Inspector: Real-time performance monitoring
 * 
 * Educational Value:
 * - Demonstrates real-world impact of architectural decisions
 * - Shows performance scaling characteristics with entity count
 * - Visualizes cache behavior and memory access patterns
 * - Provides actionable optimization recommendations
 * - Teaches performance analysis methodology
 * 
 * @author ECScope Educational ECS Framework - Performance Benchmarking Suite
 * @date 2025
 */

#include "performance_lab.hpp"
#include "memory_benchmark_suite.hpp"
#include "sparse_set_visualizer.hpp"
#include "benchmarks.hpp"
#include "ecs/registry.hpp"
#include "ecs/component.hpp"
#include "ecs/system.hpp"
#include "physics/physics_system.hpp"
#include "renderer/renderer_2d.hpp"
#include "core/types.hpp"
#include "core/log.hpp"
#include "core/profiler.hpp"
#include "memory/memory_tracker.hpp"
#include <vector>
#include <memory>
#include <chrono>
#include <thread>
#include <future>
#include <functional>
#include <string>
#include <unordered_map>
#include <mutex>
#include <random>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <atomic>
#include <array>

namespace ecscope::performance::ecs {

//=============================================================================
// ECS Performance Benchmark Configuration
//=============================================================================

/**
 * @brief ECS benchmark test categories
 */
enum class ECSBenchmarkCategory : u8 {
    Architecture,       // Architecture comparison (SoA vs AoS)
    Memory,            // Memory access patterns
    Scaling,           // Entity count scaling
    Systems,           // System performance
    Integration,       // Cross-system integration
    Stress,           // Stress testing
    Regression        // Regression testing
};

/**
 * @brief ECS architecture types for comparison
 */
enum class ECSArchitectureType : u8 {
    Archetype_SoA,     // Structure of Arrays archetype-based
    Archetype_AoS,     // Array of Structures archetype-based
    ComponentArray,    // Traditional component arrays
    SparseSet,         // Sparse set based
    Hybrid            // Hybrid approach
};

/**
 * @brief ECS benchmark configuration
 */
struct ECSBenchmarkConfig {
    /** @brief Test parameters */
    std::vector<u32> entity_counts{100, 500, 1000, 5000, 10000, 25000, 50000, 100000};
    u32 iterations{10};
    u32 warmup_iterations{3};
    f64 max_test_duration_seconds{60.0};
    
    /** @brief Architecture types to test */
    std::vector<ECSArchitectureType> architectures{
        ECSArchitectureType::Archetype_SoA,
        ECSArchitectureType::SparseSet
    };
    
    /** @brief Memory configuration */
    bool enable_memory_tracking{true};
    bool analyze_cache_behavior{true};
    bool track_allocation_patterns{true};
    usize arena_size{64 * 1024 * 1024}; // 64MB
    
    /** @brief System integration */
    bool test_physics_integration{true};
    bool test_rendering_integration{true};
    bool test_multi_threading{true};
    u32 thread_count{std::thread::hardware_concurrency()};
    
    /** @brief Test scenarios */
    bool enable_creation_deletion{true};
    bool enable_component_addition{true};
    bool enable_component_removal{true};
    bool enable_archetype_migration{true};
    bool enable_query_iteration{true};
    bool enable_random_access{true};
    
    /** @brief Stress testing */
    bool enable_stress_testing{true};
    u32 stress_entity_count{100000};
    f64 stress_duration_seconds{30.0};
    
    /** @brief Output configuration */
    bool generate_comparative_report{true};
    bool generate_visualization_data{true};
    bool export_raw_data{true};
    std::string output_directory{"ecs_benchmarks"};
    
    /** @brief Factory methods */
    static ECSBenchmarkConfig create_quick() {
        ECSBenchmarkConfig config;
        config.entity_counts = {100, 1000, 5000};
        config.iterations = 5;
        config.max_test_duration_seconds = 10.0;
        config.enable_stress_testing = false;
        return config;
    }
    
    static ECSBenchmarkConfig create_comprehensive() {
        ECSBenchmarkConfig config;
        config.entity_counts = {10, 50, 100, 500, 1000, 5000, 10000, 25000, 50000, 100000};
        config.iterations = 20;
        config.enable_stress_testing = true;
        config.test_multi_threading = true;
        return config;
    }
    
    static ECSBenchmarkConfig create_research() {
        ECSBenchmarkConfig config = create_comprehensive();
        config.iterations = 50;
        config.warmup_iterations = 10;
        config.max_test_duration_seconds = 300.0; // 5 minutes
        return config;
    }
};

/**
 * @brief ECS benchmark result with detailed metrics
 */
struct ECSBenchmarkResult {
    std::string test_name;
    ECSBenchmarkCategory category;
    ECSArchitectureType architecture_type;
    u32 entity_count;
    
    /** @brief Timing results (microseconds) */
    f64 average_time_us{0.0};
    f64 min_time_us{std::numeric_limits<f64>::max()};
    f64 max_time_us{0.0};
    f64 std_deviation_us{0.0};
    f64 median_time_us{0.0};
    std::vector<f64> raw_timings;
    
    /** @brief Throughput metrics */
    f64 entities_per_second{0.0};
    f64 operations_per_second{0.0};
    f64 components_per_second{0.0};
    
    /** @brief Memory metrics */
    usize peak_memory_usage{0};
    usize average_memory_usage{0};
    f64 memory_efficiency{0.0};
    u32 allocation_count{0};
    f64 fragmentation_ratio{0.0};
    
    /** @brief Cache metrics */
    f64 cache_hit_ratio{0.0};
    f64 cache_miss_penalty{0.0};
    f64 memory_bandwidth_usage{0.0};
    u64 cache_line_loads{0};
    
    /** @brief ECS-specific metrics */
    u32 archetype_count{0};
    u32 archetype_migrations{0};
    f64 query_iteration_time{0.0};
    f64 component_access_time{0.0};
    f64 structural_change_time{0.0};
    
    /** @brief Quality metrics */
    f64 consistency_score{0.0};    // Lower variance is better
    f64 scalability_factor{0.0};   // How well it scales with entity count
    f64 overhead_ratio{0.0};       // Overhead compared to theoretical minimum
    
    /** @brief Metadata */
    ECSBenchmarkConfig config;
    std::string platform_info;
    std::string timestamp;
    bool is_valid{false};
    std::string error_message;
    
    void calculate_statistics();
    std::string to_csv_row() const;
    std::string to_json() const;
};

//=============================================================================
// Test Component Types for Benchmarking
//=============================================================================

/**
 * @brief Simple position component for benchmarking
 */
struct BenchmarkPosition {
    f32 x, y, z;
    
    BenchmarkPosition(f32 x = 0.0f, f32 y = 0.0f, f32 z = 0.0f) 
        : x(x), y(y), z(z) {}
};

/**
 * @brief Simple velocity component for benchmarking
 */
struct BenchmarkVelocity {
    f32 x, y, z;
    
    BenchmarkVelocity(f32 x = 0.0f, f32 y = 0.0f, f32 z = 0.0f)
        : x(x), y(y), z(z) {}
};

/**
 * @brief Health component for benchmarking
 */
struct BenchmarkHealth {
    f32 current;
    f32 max;
    
    BenchmarkHealth(f32 max = 100.0f) : current(max), max(max) {}
};

/**
 * @brief Transform component for benchmarking
 */
struct BenchmarkTransform {
    f32 position[3];
    f32 rotation[4]; // quaternion
    f32 scale[3];
    
    BenchmarkTransform() {
        position[0] = position[1] = position[2] = 0.0f;
        rotation[0] = rotation[1] = rotation[2] = 0.0f; rotation[3] = 1.0f;
        scale[0] = scale[1] = scale[2] = 1.0f;
    }
};

/**
 * @brief Large component for memory testing
 */
struct BenchmarkLargeComponent {
    static constexpr usize SIZE = 1024; // 1KB
    u8 data[SIZE];
    
    BenchmarkLargeComponent() {
        std::fill_n(data, SIZE, 0);
    }
};

//=============================================================================
// Individual Benchmark Tests
//=============================================================================

/**
 * @brief Base class for ECS benchmark tests
 */
class IECSBenchmarkTest {
public:
    virtual ~IECSBenchmarkTest() = default;
    
    virtual std::string get_name() const = 0;
    virtual std::string get_description() const = 0;
    virtual ECSBenchmarkCategory get_category() const = 0;
    
    virtual ECSBenchmarkResult run_benchmark(ECSArchitectureType architecture,
                                            u32 entity_count,
                                            const ECSBenchmarkConfig& config) = 0;
    
    virtual bool supports_architecture(ECSArchitectureType architecture) const {
        return true; // Default: support all architectures
    }
    
    virtual std::vector<std::string> get_required_components() const {
        return {};
    }

protected:
    /** @brief Utility: Create test registry with specified architecture */
    std::unique_ptr<ecs::Registry> create_test_registry(ECSArchitectureType architecture,
                                                       const ECSBenchmarkConfig& config);
    
    /** @brief Utility: Populate registry with test entities */
    void populate_test_entities(ecs::Registry& registry, u32 count);
    
    /** @brief Utility: Measure execution time of function */
    template<typename Func>
    std::vector<f64> measure_execution_times(Func&& func, u32 iterations) {
        std::vector<f64> times;
        times.reserve(iterations);
        
        for (u32 i = 0; i < iterations; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            func();
            auto end = std::chrono::high_resolution_clock::now();
            
            f64 duration_us = std::chrono::duration<f64, std::micro>(end - start).count();
            times.push_back(duration_us);
        }
        
        return times;
    }
};

/**
 * @brief Entity creation/destruction benchmark
 */
class EntityLifecycleBenchmark : public IECSBenchmarkTest {
public:
    std::string get_name() const override { return "EntityLifecycle"; }
    std::string get_description() const override {
        return "Measures entity creation and destruction performance";
    }
    ECSBenchmarkCategory get_category() const override {
        return ECSBenchmarkCategory::Architecture;
    }
    
    ECSBenchmarkResult run_benchmark(ECSArchitectureType architecture,
                                    u32 entity_count,
                                    const ECSBenchmarkConfig& config) override;
};

/**
 * @brief Component addition/removal benchmark
 */
class ComponentManipulationBenchmark : public IECSBenchmarkTest {
public:
    std::string get_name() const override { return "ComponentManipulation"; }
    std::string get_description() const override {
        return "Measures component addition and removal performance";
    }
    ECSBenchmarkCategory get_category() const override {
        return ECSBenchmarkCategory::Architecture;
    }
    
    ECSBenchmarkResult run_benchmark(ECSArchitectureType architecture,
                                    u32 entity_count,
                                    const ECSBenchmarkConfig& config) override;
};

/**
 * @brief Query iteration benchmark
 */
class QueryIterationBenchmark : public IECSBenchmarkTest {
public:
    std::string get_name() const override { return "QueryIteration"; }
    std::string get_description() const override {
        return "Measures query iteration and component access performance";
    }
    ECSBenchmarkCategory get_category() const override {
        return ECSBenchmarkCategory::Memory;
    }
    
    ECSBenchmarkResult run_benchmark(ECSArchitectureType architecture,
                                    u32 entity_count,
                                    const ECSBenchmarkConfig& config) override;
};

/**
 * @brief Random component access benchmark
 */
class RandomAccessBenchmark : public IECSBenchmarkTest {
public:
    std::string get_name() const override { return "RandomAccess"; }
    std::string get_description() const override {
        return "Measures random component access performance (cache behavior)";
    }
    ECSBenchmarkCategory get_category() const override {
        return ECSBenchmarkCategory::Memory;
    }
    
    ECSBenchmarkResult run_benchmark(ECSArchitectureType architecture,
                                    u32 entity_count,
                                    const ECSBenchmarkConfig& config) override;
};

/**
 * @brief Archetype migration benchmark
 */
class ArchetypeMigrationBenchmark : public IECSBenchmarkTest {
public:
    std::string get_name() const override { return "ArchetypeMigration"; }
    std::string get_description() const override {
        return "Measures archetype migration performance during structural changes";
    }
    ECSBenchmarkCategory get_category() const override {
        return ECSBenchmarkCategory::Architecture;
    }
    
    ECSBenchmarkResult run_benchmark(ECSArchitectureType architecture,
                                    u32 entity_count,
                                    const ECSBenchmarkConfig& config) override;
    
    bool supports_architecture(ECSArchitectureType architecture) const override {
        return architecture == ECSArchitectureType::Archetype_SoA || 
               architecture == ECSArchitectureType::Archetype_AoS;
    }
};

/**
 * @brief System update benchmark
 */
class SystemUpdateBenchmark : public IECSBenchmarkTest {
public:
    std::string get_name() const override { return "SystemUpdate"; }
    std::string get_description() const override {
        return "Measures system update performance with multiple components";
    }
    ECSBenchmarkCategory get_category() const override {
        return ECSBenchmarkCategory::Systems;
    }
    
    ECSBenchmarkResult run_benchmark(ECSArchitectureType architecture,
                                    u32 entity_count,
                                    const ECSBenchmarkConfig& config) override;
};

/**
 * @brief Multi-threading benchmark
 */
class MultiThreadingBenchmark : public IECSBenchmarkTest {
public:
    std::string get_name() const override { return "MultiThreading"; }
    std::string get_description() const override {
        return "Measures multi-threaded system performance and scalability";
    }
    ECSBenchmarkCategory get_category() const override {
        return ECSBenchmarkCategory::Systems;
    }
    
    ECSBenchmarkResult run_benchmark(ECSArchitectureType architecture,
                                    u32 entity_count,
                                    const ECSBenchmarkConfig& config) override;
};

/**
 * @brief Memory pressure benchmark
 */
class MemoryPressureBenchmark : public IECSBenchmarkTest {
public:
    std::string get_name() const override { return "MemoryPressure"; }
    std::string get_description() const override {
        return "Measures performance under memory pressure with large components";
    }
    ECSBenchmarkCategory get_category() const override {
        return ECSBenchmarkCategory::Stress;
    }
    
    ECSBenchmarkResult run_benchmark(ECSArchitectureType architecture,
                                    u32 entity_count,
                                    const ECSBenchmarkConfig& config) override;
};

//=============================================================================
// Integration Benchmarks
//=============================================================================

/**
 * @brief Physics integration benchmark
 */
class PhysicsIntegrationBenchmark : public IECSBenchmarkTest {
public:
    std::string get_name() const override { return "PhysicsIntegration"; }
    std::string get_description() const override {
        return "Measures ECS performance with physics system integration";
    }
    ECSBenchmarkCategory get_category() const override {
        return ECSBenchmarkCategory::Integration;
    }
    
    ECSBenchmarkResult run_benchmark(ECSArchitectureType architecture,
                                    u32 entity_count,
                                    const ECSBenchmarkConfig& config) override;
};

/**
 * @brief Rendering integration benchmark
 */
class RenderingIntegrationBenchmark : public IECSBenchmarkTest {
public:
    std::string get_name() const override { return "RenderingIntegration"; }
    std::string get_description() const override {
        return "Measures ECS performance with rendering system integration";
    }
    ECSBenchmarkCategory get_category() const override {
        return ECSBenchmarkCategory::Integration;
    }
    
    ECSBenchmarkResult run_benchmark(ECSArchitectureType architecture,
                                    u32 entity_count,
                                    const ECSBenchmarkConfig& config) override;
};

//=============================================================================
// Main ECS Performance Benchmarker
//=============================================================================

/**
 * @brief Comprehensive ECS performance benchmarker and comparison system
 */
class ECSPerformanceBenchmarker {
private:
    ECSBenchmarkConfig config_;
    std::vector<std::unique_ptr<IECSBenchmarkTest>> tests_;
    std::vector<ECSBenchmarkResult> results_;
    
    // System integration
    std::weak_ptr<performance::PerformanceLab> performance_lab_;
    std::unique_ptr<memory::MemoryTracker> memory_tracker_;
    std::unique_ptr<visualization::SparseSetAnalyzer> sparse_set_analyzer_;
    
    // State management
    mutable std::mutex results_mutex_;
    std::atomic<bool> is_running_{false};
    std::atomic<f64> progress_{0.0};
    
    // Analysis data
    struct ArchitectureComparison {
        ECSArchitectureType architecture;
        f64 overall_score;
        std::unordered_map<std::string, f64> test_scores;
        std::vector<std::string> strengths;
        std::vector<std::string> weaknesses;
    };
    std::vector<ArchitectureComparison> architecture_comparisons_;
    
public:
    explicit ECSPerformanceBenchmarker(const ECSBenchmarkConfig& config = ECSBenchmarkConfig{});
    ~ECSPerformanceBenchmarker() = default;
    
    // Configuration
    void set_config(const ECSBenchmarkConfig& config) { config_ = config; }
    const ECSBenchmarkConfig& get_config() const { return config_; }
    
    // System integration
    void set_performance_lab(std::weak_ptr<performance::PerformanceLab> lab);
    void enable_memory_tracking(bool enable);
    void enable_sparse_set_analysis(bool enable);
    
    // Test management
    void register_test(std::unique_ptr<IECSBenchmarkTest> test);
    void register_all_standard_tests();
    std::vector<std::string> get_available_tests() const;
    std::string get_test_description(const std::string& name) const;
    
    // Benchmark execution
    void run_all_benchmarks();
    void run_specific_benchmarks(const std::vector<std::string>& test_names);
    void run_architecture_comparison(const std::vector<ECSArchitectureType>& architectures);
    void run_scaling_analysis(const std::vector<u32>& entity_counts);
    
    // Stress testing
    void run_stress_tests();
    void run_regression_tests(const std::vector<ECSBenchmarkResult>& baseline_results);
    
    // Results access
    std::vector<ECSBenchmarkResult> get_results() const;
    std::vector<ECSBenchmarkResult> get_results_for_architecture(ECSArchitectureType architecture) const;
    std::vector<ECSBenchmarkResult> get_results_for_test(const std::string& test_name) const;
    std::optional<ECSBenchmarkResult> get_best_result_for_test(const std::string& test_name) const;
    
    // Analysis and comparison
    void analyze_results();
    std::vector<ArchitectureComparison> get_architecture_comparisons() const;
    std::string generate_comparative_report() const;
    std::string generate_scaling_analysis() const;
    std::string generate_optimization_recommendations() const;
    
    // Educational insights
    std::vector<std::string> get_educational_insights() const;
    std::string explain_result(const ECSBenchmarkResult& result) const;
    std::vector<std::string> suggest_optimizations(const ECSBenchmarkResult& result) const;
    
    // Visualization data
    struct VisualizationData {
        std::vector<std::pair<u32, f64>> scaling_curve;
        std::vector<std::pair<std::string, f64>> architecture_performance;
        std::vector<std::pair<std::string, f64>> test_breakdown;
        std::string interpretation;
    };
    VisualizationData generate_visualization_data() const;
    
    // Export functionality
    void export_results_csv(const std::string& filename) const;
    void export_results_json(const std::string& filename) const;
    void export_comparative_report(const std::string& filename) const;
    void export_visualization_data(const std::string& filename) const;
    
    // Runtime status
    bool is_running() const { return is_running_.load(); };
    f64 get_progress() const { return progress_.load(); };
    void cancel_benchmarks();
    
    // Utility functions
    static std::string architecture_to_string(ECSArchitectureType architecture);
    static std::string category_to_string(ECSBenchmarkCategory category);
    
private:
    // Implementation details
    void initialize_standard_tests();
    void execute_single_test(IECSBenchmarkTest& test, 
                           ECSArchitectureType architecture,
                           u32 entity_count);
    void update_progress(f64 progress);
    void calculate_architecture_scores();
    void generate_insights_for_architecture(ArchitectureComparison& comparison);
    
    // Analysis helpers
    f64 calculate_scalability_factor(const std::vector<ECSBenchmarkResult>& results) const;
    f64 calculate_efficiency_score(const ECSBenchmarkResult& result) const;
    std::vector<std::string> identify_bottlenecks(const ECSBenchmarkResult& result) const;
    
    // Utility functions
    std::string get_timestamp() const;
    std::string get_platform_info() const;
    void log_benchmark_start(const std::string& test_name, 
                           ECSArchitectureType architecture, 
                           u32 entity_count) const;
    void log_benchmark_result(const ECSBenchmarkResult& result) const;
};

//=============================================================================
// Benchmark Suite Factory
//=============================================================================

/**
 * @brief Factory for creating pre-configured benchmark suites
 */
class ECSBenchmarkSuiteFactory {
public:
    /** @brief Create quick benchmark suite for development */
    static std::unique_ptr<ECSPerformanceBenchmarker> create_quick_suite();
    
    /** @brief Create comprehensive benchmark suite */
    static std::unique_ptr<ECSPerformanceBenchmarker> create_comprehensive_suite();
    
    /** @brief Create research-grade benchmark suite */
    static std::unique_ptr<ECSPerformanceBenchmarker> create_research_suite();
    
    /** @brief Create educational benchmark suite with explanations */
    static std::unique_ptr<ECSPerformanceBenchmarker> create_educational_suite();
    
    /** @brief Create regression testing suite */
    static std::unique_ptr<ECSPerformanceBenchmarker> create_regression_suite();
    
    /** @brief Create memory-focused benchmark suite */
    static std::unique_ptr<ECSPerformanceBenchmarker> create_memory_suite();
    
    /** @brief Create scaling analysis suite */
    static std::unique_ptr<ECSPerformanceBenchmarker> create_scaling_suite();
};

//=============================================================================
// Integration with Performance Lab
//=============================================================================

/**
 * @brief ECS performance experiment for integration with PerformanceLab
 */
class ECSPerformanceExperiment : public performance::IPerformanceExperiment {
private:
    std::unique_ptr<ECSPerformanceBenchmarker> benchmarker_;
    ECSBenchmarkConfig config_;
    std::string experiment_name_;
    
public:
    explicit ECSPerformanceExperiment(const std::string& name,
                                     const ECSBenchmarkConfig& config = ECSBenchmarkConfig{});
    
    std::string get_name() const override { return experiment_name_; }
    std::string get_description() const override;
    std::string get_category() const override { return "ECS"; }
    
    bool setup(const performance::ExperimentConfig& config) override;
    performance::BenchmarkResult execute() override;
    void cleanup() override;
    
    bool supports_real_time_visualization() const override { return true; }
    void update_visualization(f64 dt) override;
    
    std::vector<performance::PerformanceRecommendation> generate_recommendations() const override;
};

//=============================================================================
// Utility Functions and Helpers
//=============================================================================

namespace ecs_benchmark_utils {
    /** @brief Convert ECS benchmark result to performance lab format */
    performance::BenchmarkResult convert_to_performance_result(const ECSBenchmarkResult& ecs_result);
    
    /** @brief Calculate performance improvement ratio */
    f64 calculate_improvement_ratio(const ECSBenchmarkResult& baseline, 
                                   const ECSBenchmarkResult& improved);
    
    /** @brief Detect performance regressions */
    std::vector<std::string> detect_regressions(const std::vector<ECSBenchmarkResult>& baseline,
                                               const std::vector<ECSBenchmarkResult>& current,
                                               f64 regression_threshold = 0.05);
    
    /** @brief Generate performance grade (A-F) */
    std::string calculate_performance_grade(const ECSBenchmarkResult& result);
    
    /** @brief Estimate memory usage for entity count */
    usize estimate_memory_usage(ECSArchitectureType architecture, u32 entity_count);
    
    /** @brief Format benchmark results for display */
    std::string format_result_summary(const ECSBenchmarkResult& result);
    
    /** @brief Create performance comparison table */
    std::string create_comparison_table(const std::vector<ECSBenchmarkResult>& results);
}

} // namespace ecscope::performance::ecs