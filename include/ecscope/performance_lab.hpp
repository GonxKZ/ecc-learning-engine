#pragma once

/**
 * @file performance_lab.hpp
 * @brief Performance Laboratory - Main coordinator for ECScope memory behavior analysis
 * 
 * This is the heart of ECScope's vision as a "laboratorio de memoria en movimiento" 
 * (memory lab in motion). It orchestrates comprehensive performance analysis across
 * memory allocation strategies, ECS architecture patterns, and system integration.
 * 
 * Educational Mission:
 * - Demonstrate real-world impact of memory layout decisions
 * - Show SoA vs AoS performance characteristics in live scenarios
 * - Visualize archetype migration costs and optimization opportunities
 * - Provide interactive experiments for memory behavior exploration
 * - Generate actionable optimization recommendations
 * 
 * Key Laboratory Capabilities:
 * - Memory Access Pattern Laboratory: SoA/AoS comparison, cache behavior analysis
 * - Allocation Strategy Benchmarks: Arena/Pool/PMR performance comparison
 * - ECS Performance Profiling: Archetype operations, query optimization
 * - System Integration Analysis: Cross-system performance impact
 * - Real-time Educational Visualization: Live performance graphs and explanations
 * 
 * Architecture:
 * - Modular experiment system with pluggable benchmarks
 * - Real-time data collection with minimal performance impact
 * - Educational UI integration with explanatory visualizations
 * - Comprehensive reporting and recommendation system
 * - Thread-safe operation for concurrent system analysis
 * 
 * @author ECScope Educational ECS Framework
 * @date 2024
 */

#include "core/types.hpp"
#include "core/time.hpp"
#include "core/log.hpp"
#include "memory/memory_tracker.hpp"
#include "ecs/registry.hpp"
#include "physics/physics_world.hpp"
#include "renderer/renderer_2d.hpp"
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <chrono>
#include <array>
#include <optional>

namespace ecscope::performance {

// Forward declarations
class MemoryExperiments;
class AllocationBenchmarks;
class ECSProfiler;
class SystemIntegrationAnalyzer;

/**
 * @brief Performance measurement precision levels
 */
enum class MeasurementPrecision : u8 {
    Fast,           // Quick measurements, lower precision
    Normal,         // Balanced precision/performance
    Precise,        // High precision, slower measurements
    Research        // Maximum precision for research purposes
};

/**
 * @brief Experiment execution status
 */
enum class ExperimentStatus : u8 {
    Pending,        // Not started
    Running,        // Currently executing
    Completed,      // Successfully completed
    Failed,         // Failed to execute
    Cancelled       // Manually cancelled
};

/**
 * @brief Performance benchmark result
 */
struct BenchmarkResult {
    std::string name;               // Benchmark identifier
    std::string description;        // Human-readable description
    std::string category;           // Category (Memory, ECS, Physics, etc.)
    
    // Timing results
    f64 execution_time_ms;          // Total execution time
    f64 average_time_ms;            // Average per-operation time
    f64 min_time_ms;               // Fastest operation
    f64 max_time_ms;               // Slowest operation
    f64 std_deviation_ms;          // Time standard deviation
    
    // Memory results
    usize memory_usage_bytes;       // Peak memory usage
    usize allocations_count;        // Number of allocations
    f64 allocation_rate;           // Allocations per second
    f64 fragmentation_ratio;       // Memory fragmentation (0.0-1.0)
    
    // Performance metrics
    f64 throughput;                // Operations per second
    f64 efficiency_score;          // Overall efficiency (0.0-1.0)
    f64 cache_miss_rate;           // Estimated cache miss rate
    f64 memory_bandwidth_usage;    // Memory bandwidth utilization
    
    // Educational insights
    std::vector<std::string> insights;      // Key takeaways
    std::vector<std::string> recommendations; // Optimization suggestions
    std::unordered_map<std::string, f64> metadata; // Additional metrics
    
    // Result validation
    bool is_valid;                 // Result validity flag
    f64 confidence_level;          // Statistical confidence (0.0-1.0)
    std::string error_message;     // Error description if failed
    
    BenchmarkResult() noexcept
        : execution_time_ms(0.0), average_time_ms(0.0), min_time_ms(0.0), 
          max_time_ms(0.0), std_deviation_ms(0.0), memory_usage_bytes(0),
          allocations_count(0), allocation_rate(0.0), fragmentation_ratio(0.0),
          throughput(0.0), efficiency_score(0.0), cache_miss_rate(0.0),
          memory_bandwidth_usage(0.0), is_valid(false), confidence_level(0.0) {}
};

/**
 * @brief Laboratory experiment configuration
 */
struct ExperimentConfig {
    std::string name;               // Experiment identifier
    std::string description;        // Detailed description
    MeasurementPrecision precision; // Measurement precision level
    u32 iterations;                // Number of test iterations
    u32 warmup_iterations;         // Warmup iterations
    f64 max_duration_seconds;      // Maximum execution time
    bool capture_detailed_metrics; // Enable detailed metric collection
    bool enable_visualization;     // Enable real-time visualization
    std::unordered_map<std::string, std::string> parameters; // Custom parameters
    
    ExperimentConfig() noexcept
        : precision(MeasurementPrecision::Normal), iterations(100),
          warmup_iterations(10), max_duration_seconds(30.0),
          capture_detailed_metrics(true), enable_visualization(true) {}
};

/**
 * @brief System performance snapshot
 */
struct SystemPerformanceSnapshot {
    f64 timestamp;                  // When snapshot was taken
    
    // CPU metrics
    f64 cpu_usage_percent;          // Current CPU usage
    u64 cpu_cycles;                // CPU cycle count
    u64 instructions;              // Instructions executed
    f64 ipc;                       // Instructions per cycle
    
    // Memory metrics
    usize memory_usage_bytes;       // Current memory usage
    usize peak_memory_bytes;        // Peak memory usage
    u64 page_faults;               // Memory page faults
    u64 cache_misses_l1;           // L1 cache misses
    u64 cache_misses_l2;           // L2 cache misses
    u64 cache_misses_l3;           // L3 cache misses
    
    // System metrics
    f64 frame_time_ms;             // Current frame time
    f64 fps;                       // Frames per second
    u32 active_threads;            // Number of active threads
    f64 memory_bandwidth_usage;    // Memory bandwidth utilization
    
    // ECS metrics
    u32 entity_count;              // Active entity count
    u32 archetype_count;           // Number of archetypes
    u32 component_migrations;      // Recent component migrations
    f64 ecs_update_time_ms;        // ECS update time
    
    SystemPerformanceSnapshot() noexcept
        : timestamp(0.0), cpu_usage_percent(0.0), cpu_cycles(0),
          instructions(0), ipc(0.0), memory_usage_bytes(0),
          peak_memory_bytes(0), page_faults(0), cache_misses_l1(0),
          cache_misses_l2(0), cache_misses_l3(0), frame_time_ms(0.0),
          fps(0.0), active_threads(0), memory_bandwidth_usage(0.0),
          entity_count(0), archetype_count(0), component_migrations(0),
          ecs_update_time_ms(0.0) {}
};

/**
 * @brief Performance recommendation with educational context
 */
struct PerformanceRecommendation {
    enum class Priority : u8 {
        Low,            // Nice to have improvement
        Medium,         // Noticeable performance impact
        High,           // Significant performance impact
        Critical        // Major performance bottleneck
    };
    
    enum class Category : u8 {
        Memory,         // Memory-related optimization
        ECS,            // ECS architecture optimization
        Physics,        // Physics system optimization
        Rendering,      // Rendering optimization
        Integration,    // Cross-system optimization
        Algorithm       // Algorithm improvement
    };
    
    std::string title;              // Recommendation title
    std::string description;        // Detailed description
    Priority priority;              // Optimization priority
    Category category;              // Optimization category
    f64 estimated_improvement;      // Estimated performance improvement (%)
    f64 implementation_difficulty;  // Implementation difficulty (0.0-1.0)
    std::vector<std::string> implementation_steps; // How to implement
    std::vector<std::string> educational_notes;    // Educational explanations
    std::string code_example;       // Example implementation code
    std::unordered_map<std::string, f64> supporting_metrics; // Supporting data
    
    PerformanceRecommendation() noexcept
        : priority(Priority::Medium), category(Category::Memory),
          estimated_improvement(0.0), implementation_difficulty(0.5) {}
};

/**
 * @brief Laboratory experiment interface
 */
class IPerformanceExperiment {
public:
    virtual ~IPerformanceExperiment() = default;
    
    virtual std::string get_name() const = 0;
    virtual std::string get_description() const = 0;
    virtual std::string get_category() const = 0;
    
    virtual bool setup(const ExperimentConfig& config) = 0;
    virtual BenchmarkResult execute() = 0;
    virtual void cleanup() = 0;
    
    virtual bool supports_real_time_visualization() const { return false; }
    virtual void update_visualization(f64 dt) {}
    
    virtual std::vector<std::string> get_required_parameters() const { return {}; }
    virtual std::vector<PerformanceRecommendation> generate_recommendations() const { return {}; }
};

/**
 * @brief Main Performance Laboratory coordinator
 * 
 * This class orchestrates all performance analysis activities in ECScope,
 * serving as the central hub for memory behavior experiments, allocation
 * strategy benchmarks, and educational performance insights.
 */
class PerformanceLab {
private:
    // Core systems integration
    std::weak_ptr<ecs::Registry> ecs_registry_;
    std::weak_ptr<physics::PhysicsWorld> physics_world_;
    std::weak_ptr<renderer::Renderer2D> renderer_;
    
    // Laboratory components
    std::unique_ptr<MemoryExperiments> memory_experiments_;
    std::unique_ptr<AllocationBenchmarks> allocation_benchmarks_;
    std::unique_ptr<ECSProfiler> ecs_profiler_;
    std::unique_ptr<SystemIntegrationAnalyzer> integration_analyzer_;
    
    // Experiment management
    std::vector<std::unique_ptr<IPerformanceExperiment>> experiments_;
    std::unordered_map<std::string, BenchmarkResult> results_cache_;
    std::vector<SystemPerformanceSnapshot> performance_history_;
    
    // Current experiment state
    std::atomic<ExperimentStatus> current_status_{ExperimentStatus::Pending};
    std::string current_experiment_name_;
    std::unique_ptr<std::thread> experiment_thread_;
    mutable std::mutex state_mutex_;
    
    // Performance monitoring
    f64 monitoring_start_time_;
    std::atomic<bool> is_monitoring_{false};
    std::atomic<bool> should_stop_monitoring_{false};
    std::unique_ptr<std::thread> monitoring_thread_;
    
    // Configuration
    ExperimentConfig default_config_;
    bool enable_real_time_analysis_;
    f64 snapshot_interval_;
    usize max_history_size_;
    
    // Analysis results
    std::vector<PerformanceRecommendation> current_recommendations_;
    mutable std::mutex recommendations_mutex_;
    
    // Educational features
    std::unordered_map<std::string, std::string> educational_explanations_;
    std::vector<std::string> current_insights_;
    
    // Performance monitoring implementation
    void monitoring_loop();
    SystemPerformanceSnapshot capture_snapshot();
    void analyze_performance_trends();
    void update_recommendations();
    
    // Experiment execution
    BenchmarkResult execute_experiment_internal(IPerformanceExperiment* experiment, 
                                               const ExperimentConfig& config);
    void validate_result(BenchmarkResult& result, const ExperimentConfig& config);
    
    // Utility functions
    f64 calculate_statistical_confidence(const std::vector<f64>& samples);
    std::vector<std::string> generate_insights_from_result(const BenchmarkResult& result);
    void cache_result(const std::string& experiment_name, const BenchmarkResult& result);
    
public:
    PerformanceLab() noexcept;
    ~PerformanceLab() noexcept;
    
    // Non-copyable, non-movable
    PerformanceLab(const PerformanceLab&) = delete;
    PerformanceLab& operator=(const PerformanceLab&) = delete;
    PerformanceLab(PerformanceLab&&) = delete;
    PerformanceLab& operator=(PerformanceLab&&) = delete;
    
    // System integration
    void set_ecs_registry(std::weak_ptr<ecs::Registry> registry) noexcept;
    void set_physics_world(std::weak_ptr<physics::PhysicsWorld> world) noexcept;
    void set_renderer(std::weak_ptr<renderer::Renderer2D> renderer) noexcept;
    
    // Laboratory management
    bool initialize();
    void shutdown();
    void reset_all_data();
    
    // Performance monitoring
    void start_monitoring();
    void stop_monitoring();
    bool is_monitoring() const noexcept { return is_monitoring_.load(); }
    
    // Experiment registration and execution
    void register_experiment(std::unique_ptr<IPerformanceExperiment> experiment);
    std::vector<std::string> get_available_experiments() const;
    std::string get_experiment_description(const std::string& name) const;
    
    // Synchronous experiment execution
    BenchmarkResult run_experiment(const std::string& name, 
                                  const ExperimentConfig& config = ExperimentConfig{});
    
    // Asynchronous experiment execution
    bool start_experiment_async(const std::string& name, 
                               const ExperimentConfig& config = ExperimentConfig{});
    ExperimentStatus get_experiment_status() const noexcept;
    std::optional<BenchmarkResult> get_experiment_result();
    void cancel_current_experiment();
    
    // Batch experiment execution
    std::vector<BenchmarkResult> run_experiment_suite(const std::vector<std::string>& experiments,
                                                      const ExperimentConfig& config = ExperimentConfig{});
    
    // Results and analysis
    std::vector<BenchmarkResult> get_all_results() const;
    std::optional<BenchmarkResult> get_result(const std::string& experiment_name) const;
    void clear_results_cache();
    
    // Performance history
    std::vector<SystemPerformanceSnapshot> get_performance_history() const;
    SystemPerformanceSnapshot get_current_snapshot() const;
    void clear_performance_history();
    
    // Recommendations and insights
    std::vector<PerformanceRecommendation> get_current_recommendations() const;
    std::vector<std::string> get_current_insights() const;
    void force_recommendations_update();
    
    // Educational features
    std::string get_explanation(const std::string& topic) const;
    std::vector<std::string> get_available_explanations() const;
    void add_explanation(const std::string& topic, const std::string& explanation);
    
    // Configuration
    void set_default_config(const ExperimentConfig& config);
    ExperimentConfig get_default_config() const;
    void enable_real_time_analysis(bool enable) noexcept;
    void set_snapshot_interval(f64 interval) noexcept;
    void set_max_history_size(usize size) noexcept;
    
    // Specialized laboratory access
    MemoryExperiments& get_memory_experiments();
    AllocationBenchmarks& get_allocation_benchmarks();
    ECSProfiler& get_ecs_profiler();
    SystemIntegrationAnalyzer& get_integration_analyzer();
    
    const MemoryExperiments& get_memory_experiments() const;
    const AllocationBenchmarks& get_allocation_benchmarks() const;
    const ECSProfiler& get_ecs_profiler() const;
    const SystemIntegrationAnalyzer& get_integration_analyzer() const;
    
    // Quick analysis utilities
    f64 estimate_memory_efficiency() const;
    f64 estimate_ecs_performance() const;
    f64 estimate_overall_health_score() const;
    
    // Reporting
    void export_results_to_json(const std::string& filename) const;
    void export_performance_report(const std::string& filename) const;
    void export_recommendations_report(const std::string& filename) const;
    
    // Debug and diagnostics
    void print_current_status() const;
    void print_performance_summary() const;
    bool validate_system_integration() const;
};

/**
 * @brief Performance Lab Factory for easy setup and configuration
 */
class PerformanceLabFactory {
public:
    static std::unique_ptr<PerformanceLab> create_default_lab();
    static std::unique_ptr<PerformanceLab> create_research_lab();
    static std::unique_ptr<PerformanceLab> create_educational_lab();
    static std::unique_ptr<PerformanceLab> create_production_lab();
    
    static ExperimentConfig create_fast_config();
    static ExperimentConfig create_precise_config();
    static ExperimentConfig create_research_config();
    static ExperimentConfig create_educational_config();
};

/**
 * @brief RAII helper for automatic performance monitoring
 */
class ScopedPerformanceMonitor {
private:
    PerformanceLab& lab_;
    bool was_monitoring_;
    
public:
    explicit ScopedPerformanceMonitor(PerformanceLab& lab) noexcept
        : lab_(lab), was_monitoring_(lab.is_monitoring()) {
        if (!was_monitoring_) {
            lab_.start_monitoring();
        }
    }
    
    ~ScopedPerformanceMonitor() noexcept {
        if (!was_monitoring_) {
            lab_.stop_monitoring();
        }
    }
    
    // Non-copyable, non-movable
    ScopedPerformanceMonitor(const ScopedPerformanceMonitor&) = delete;
    ScopedPerformanceMonitor& operator=(const ScopedPerformanceMonitor&) = delete;
    ScopedPerformanceMonitor(ScopedPerformanceMonitor&&) = delete;
    ScopedPerformanceMonitor& operator=(ScopedPerformanceMonitor&&) = delete;
};

// Utility functions
namespace lab_utils {
    // Timing utilities
    f64 measure_execution_time(std::function<void()> func);
    std::vector<f64> measure_multiple_executions(std::function<void()> func, u32 iterations);
    f64 calculate_average(const std::vector<f64>& samples);
    f64 calculate_standard_deviation(const std::vector<f64>& samples);
    
    // Memory utilities
    usize measure_memory_usage(std::function<void()> func);
    f64 estimate_cache_miss_rate(const std::vector<f64>& access_times);
    f64 calculate_memory_bandwidth(usize bytes_transferred, f64 time_seconds);
    
    // Statistical utilities
    f64 calculate_confidence_interval(const std::vector<f64>& samples, f64 confidence_level);
    bool is_statistically_significant(const std::vector<f64>& baseline, 
                                     const std::vector<f64>& test, 
                                     f64 significance_level = 0.05);
    
    // Formatting utilities
    std::string format_bytes(usize bytes);
    std::string format_time(f64 milliseconds);
    std::string format_percentage(f64 ratio);
    std::string format_rate(f64 rate, const std::string& unit);
}

} // namespace ecscope::performance