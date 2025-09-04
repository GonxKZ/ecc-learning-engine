#pragma once

/**
 * @file platform/optimization_engine.hpp
 * @brief Intelligent Cross-Platform Optimization Engine for ECScope
 * 
 * This system provides intelligent optimization recommendations and automatic
 * performance tuning based on detected hardware capabilities and platform
 * characteristics. It serves as the brain of the ECScope performance system.
 * 
 * Key Features:
 * - Hardware-aware optimization recommendations
 * - Runtime performance monitoring and adaptive tuning
 * - Platform-specific compiler flag suggestions
 * - Memory layout optimization strategies
 * - Thread configuration and scheduling recommendations
 * - SIMD instruction set selection and fallback management
 * - Educational performance impact analysis
 * 
 * Educational Value:
 * - Real-time performance impact visualization
 * - Before/after optimization comparisons
 * - Hardware bottleneck identification
 * - Cross-platform performance analysis
 * - Best practices demonstrations
 * 
 * @author ECScope Educational ECS Framework - Optimization Engine
 * @date 2025
 */

#include "hardware_detection.hpp"
#include "core/types.hpp"
#include <functional>
#include <chrono>
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <atomic>

namespace ecscope::platform {

//=============================================================================
// Optimization Categories and Priorities
//=============================================================================

/**
 * @brief Optimization category enumeration
 */
enum class OptimizationCategory : u8 {
    CPU_Architecture,       // CPU-specific optimizations
    Memory_Layout,          // Memory access pattern optimizations
    SIMD_Vectorization,     // SIMD instruction optimizations
    Threading,              // Multi-threading and concurrency
    Cache_Optimization,     // Cache-friendly algorithms and data structures
    Platform_Specific,      // OS and platform-specific optimizations
    Compiler_Flags,         // Build-time compiler optimizations
    Runtime_Tuning,         // Runtime adaptive optimizations
    Power_Management,       // Power/thermal aware optimizations
    Educational            // Educational demonstrations and analysis
};

/**
 * @brief Optimization priority levels
 */
enum class OptimizationPriority : u8 {
    Critical,              // Essential optimizations (>50% performance impact)
    High,                  // High-impact optimizations (20-50% impact)
    Medium,                // Medium-impact optimizations (10-20% impact)
    Low,                   // Low-impact optimizations (5-10% impact)
    Educational           // Optimizations for learning purposes
};

/**
 * @brief Optimization implementation difficulty
 */
enum class OptimizationDifficulty : u8 {
    Trivial,              // Single line change
    Easy,                 // Simple refactoring
    Medium,               // Moderate implementation effort
    Hard,                 // Significant restructuring required
    Expert                // Deep optimization expertise needed
};

//=============================================================================
// Optimization Recommendation System
//=============================================================================

/**
 * @brief Individual optimization recommendation
 */
struct OptimizationRecommendation {
    std::string id;                           // Unique identifier
    std::string title;                        // Human-readable title
    std::string description;                  // Detailed description
    OptimizationCategory category;            // Category classification
    OptimizationPriority priority;           // Impact priority
    OptimizationDifficulty difficulty;       // Implementation difficulty
    
    f32 estimated_performance_gain;           // Estimated % performance improvement
    f32 confidence_score;                     // Confidence in estimate (0-1)
    f32 implementation_cost_hours;            // Estimated implementation time
    
    // Applicability conditions
    std::vector<std::string> required_hardware;    // Hardware requirements
    std::vector<std::string> required_software;    // Software requirements
    std::vector<std::string> prerequisites;        // Other optimizations needed first
    
    // Implementation guidance
    std::vector<std::string> implementation_steps; // Step-by-step guide
    std::vector<std::string> code_examples;        // Example implementations
    std::vector<std::string> compiler_flags;       // Recommended compiler flags
    std::vector<std::string> pitfalls;             // Common mistakes to avoid
    
    // Measurement and validation
    std::string benchmark_name;               // Benchmark to measure impact
    std::string measurement_methodology;      // How to measure improvement
    std::vector<std::string> metrics_to_track; // Performance metrics
    
    // Educational content
    std::string educational_explanation;      // Why this optimization works
    std::vector<std::string> learning_resources; // Additional resources
    
    // Metadata
    std::chrono::system_clock::time_point created_time;
    std::chrono::system_clock::time_point last_updated;
    bool is_implemented = false;
    bool is_validated = false;
    f32 measured_performance_gain = 0.0f;
    
    // Utility methods
    bool is_applicable(const HardwareDetector& detector) const;
    f32 get_priority_score() const;
    std::string get_summary() const;
};

/**
 * @brief Collection of related optimization recommendations
 */
struct OptimizationPlan {
    std::string name;
    std::string description;
    std::vector<OptimizationRecommendation> recommendations;
    
    f32 total_estimated_gain = 0.0f;
    f32 total_implementation_cost = 0.0f;
    f32 roi_score = 0.0f; // Return on investment
    
    // Plan execution tracking
    u32 recommendations_implemented = 0;
    f32 actual_performance_gain = 0.0f;
    std::chrono::steady_clock::time_point start_time;
    
    void calculate_totals();
    void sort_by_priority();
    void sort_by_roi();
    std::vector<OptimizationRecommendation> get_next_recommendations(u32 count = 3) const;
};

//=============================================================================
// Performance Benchmarking and Measurement
//=============================================================================

/**
 * @brief Performance benchmark result
 */
struct BenchmarkResult {
    std::string benchmark_name;
    std::string configuration;
    
    std::chrono::nanoseconds execution_time;
    f64 operations_per_second = 0.0;
    f64 memory_bandwidth_gbps = 0.0;
    f64 cpu_utilization_percent = 0.0;
    f64 cache_hit_rate = 0.0;
    f64 power_consumption_watts = 0.0;
    
    std::unordered_map<std::string, f64> custom_metrics;
    
    std::chrono::system_clock::time_point timestamp;
    std::string hardware_signature; // Hardware configuration hash
    
    f64 get_performance_score() const;
    std::string get_formatted_results() const;
};

/**
 * @brief Benchmark suite for optimization validation
 */
class OptimizationBenchmark {
private:
    std::string benchmark_name_;
    std::function<void()> setup_func_;
    std::function<void()> benchmark_func_;
    std::function<void()> teardown_func_;
    
    std::vector<BenchmarkResult> historical_results_;
    u32 warmup_iterations_ = 5;
    u32 measurement_iterations_ = 10;
    
public:
    OptimizationBenchmark(const std::string& name,
                         std::function<void()> setup,
                         std::function<void()> benchmark,
                         std::function<void()> teardown = nullptr);
    
    // Configuration
    void set_iterations(u32 warmup, u32 measurement);
    void add_custom_metric(const std::string& name, std::function<f64()> metric_func);
    
    // Execution
    BenchmarkResult run_benchmark(const std::string& configuration = "default");
    std::vector<BenchmarkResult> run_comparison(const std::vector<std::string>& configurations);
    
    // Analysis
    f64 calculate_improvement(const std::string& baseline, const std::string& optimized) const;
    BenchmarkResult get_baseline_result() const;
    std::vector<BenchmarkResult> get_historical_results() const;
    
    // Reporting
    std::string generate_performance_report() const;
    void export_results_csv(const std::string& filename) const;
};

//=============================================================================
// Intelligent Optimization Engine
//=============================================================================

/**
 * @brief Configuration for the optimization engine
 */
struct OptimizationEngineConfig {
    bool enable_runtime_monitoring = true;
    bool enable_adaptive_tuning = true;
    bool enable_educational_mode = true;
    
    f32 performance_threshold = 0.05f;      // 5% minimum improvement threshold
    u32 measurement_window_seconds = 60;     // Performance measurement window
    u32 max_recommendations = 10;           // Maximum recommendations to generate
    
    OptimizationPriority minimum_priority = OptimizationPriority::Medium;
    OptimizationDifficulty maximum_difficulty = OptimizationDifficulty::Hard;
    
    std::vector<OptimizationCategory> enabled_categories = {
        OptimizationCategory::CPU_Architecture,
        OptimizationCategory::Memory_Layout,
        OptimizationCategory::SIMD_Vectorization,
        OptimizationCategory::Threading,
        OptimizationCategory::Cache_Optimization
    };
    
    bool save_recommendations_to_file = true;
    std::string recommendations_file = "ecscope_optimizations.json";
};

/**
 * @brief Comprehensive optimization engine for ECScope
 */
class OptimizationEngine {
private:
    HardwareDetector& hardware_detector_;
    OptimizationEngineConfig config_;
    
    // Recommendation generation
    std::vector<OptimizationRecommendation> available_recommendations_;
    std::unordered_map<std::string, OptimizationPlan> optimization_plans_;
    
    // Performance monitoring
    std::vector<std::unique_ptr<OptimizationBenchmark>> benchmarks_;
    std::unordered_map<std::string, BenchmarkResult> baseline_results_;
    std::unordered_map<std::string, BenchmarkResult> current_results_;
    
    // Runtime monitoring
    std::atomic<bool> monitoring_active_{false};
    std::thread monitoring_thread_;
    
    // Performance history
    struct PerformanceSnapshot {
        std::chrono::steady_clock::time_point timestamp;
        f64 cpu_utilization;
        f64 memory_usage;
        f64 cache_miss_rate;
        f64 thermal_state;
        std::unordered_map<std::string, f64> custom_metrics;
    };
    
    std::vector<PerformanceSnapshot> performance_history_;
    mutable std::mutex history_mutex_;
    
public:
    explicit OptimizationEngine(HardwareDetector& detector, 
                               OptimizationEngineConfig config = {});
    ~OptimizationEngine();
    
    // Non-copyable
    OptimizationEngine(const OptimizationEngine&) = delete;
    OptimizationEngine& operator=(const OptimizationEngine&) = delete;
    
    // Initialization and configuration
    void initialize();
    void shutdown();
    void update_config(const OptimizationEngineConfig& config);
    const OptimizationEngineConfig& get_config() const { return config_; }
    
    // Recommendation generation
    std::vector<OptimizationRecommendation> analyze_system();
    OptimizationPlan generate_optimization_plan(const std::string& name = "auto");
    std::vector<OptimizationRecommendation> get_quick_wins() const;
    std::vector<OptimizationRecommendation> get_educational_recommendations() const;
    
    // Specific optimization analysis
    std::vector<OptimizationRecommendation> analyze_cpu_optimizations() const;
    std::vector<OptimizationRecommendation> analyze_memory_optimizations() const;
    std::vector<OptimizationRecommendation> analyze_simd_optimizations() const;
    std::vector<OptimizationRecommendation> analyze_threading_optimizations() const;
    std::vector<OptimizationRecommendation> analyze_compiler_optimizations() const;
    
    // Performance monitoring
    void start_monitoring();
    void stop_monitoring();
    void add_benchmark(std::unique_ptr<OptimizationBenchmark> benchmark);
    void run_baseline_benchmarks();
    BenchmarkResult run_specific_benchmark(const std::string& name);
    
    // Optimization validation
    bool validate_optimization(const OptimizationRecommendation& recommendation);
    f64 measure_optimization_impact(const std::string& optimization_id);
    std::vector<BenchmarkResult> compare_configurations(
        const std::vector<std::string>& configurations);
    
    // Adaptive optimization
    void enable_adaptive_tuning();
    void disable_adaptive_tuning();
    void trigger_adaptive_optimization();
    
    // Reporting and analysis
    std::string generate_system_analysis_report() const;
    std::string generate_optimization_report() const;
    std::string generate_performance_history_report() const;
    void export_recommendations(const std::string& filename) const;
    void import_recommendations(const std::string& filename);
    
    // Educational features
    struct OptimizationTutorial {
        std::string title;
        std::string description;
        std::vector<std::string> steps;
        std::string before_code;
        std::string after_code;
        std::string explanation;
        f32 expected_improvement;
    };
    
    std::vector<OptimizationTutorial> get_interactive_tutorials() const;
    void demonstrate_optimization(const std::string& optimization_id);
    std::string explain_optimization_theory(const std::string& optimization_id) const;
    
    // Platform-specific optimizations
    std::vector<std::string> get_recommended_compiler_flags() const;
    std::unordered_map<std::string, std::string> get_recommended_environment_variables() const;
    std::vector<std::string> get_platform_specific_recommendations() const;
    
private:
    // Internal optimization analysis
    void load_optimization_database();
    void populate_cpu_optimizations();
    void populate_memory_optimizations();
    void populate_simd_optimizations();
    void populate_threading_optimizations();
    void populate_educational_optimizations();
    
    // Hardware analysis helpers
    std::vector<std::string> analyze_cpu_bottlenecks() const;
    std::vector<std::string> analyze_memory_bottlenecks() const;
    std::vector<std::string> analyze_cache_performance() const;
    std::vector<std::string> analyze_simd_utilization() const;
    
    // Performance monitoring implementation
    void monitoring_worker();
    PerformanceSnapshot capture_performance_snapshot() const;
    void analyze_performance_trends();
    
    // Recommendation scoring and filtering
    f32 calculate_recommendation_score(const OptimizationRecommendation& rec) const;
    std::vector<OptimizationRecommendation> filter_recommendations(
        const std::vector<OptimizationRecommendation>& recommendations) const;
    
    // Platform-specific helpers
    std::vector<OptimizationRecommendation> get_windows_optimizations() const;
    std::vector<OptimizationRecommendation> get_linux_optimizations() const;
    std::vector<OptimizationRecommendation> get_macos_optimizations() const;
    std::vector<OptimizationRecommendation> get_mobile_optimizations() const;
};

//=============================================================================
// Optimization Templates and Patterns
//=============================================================================

/**
 * @brief Template-based optimization pattern system
 */
class OptimizationPattern {
public:
    struct PatternMatch {
        std::string pattern_name;
        f32 confidence_score;
        std::vector<std::string> matched_conditions;
        std::vector<OptimizationRecommendation> recommendations;
    };
    
    virtual ~OptimizationPattern() = default;
    virtual PatternMatch analyze(const HardwareDetector& detector) const = 0;
    virtual std::string get_pattern_name() const = 0;
    virtual std::string get_description() const = 0;
};

/**
 * @brief Common optimization patterns
 */
class CommonOptimizationPatterns {
public:
    // Memory access patterns
    static std::unique_ptr<OptimizationPattern> create_cache_friendly_pattern();
    static std::unique_ptr<OptimizationPattern> create_numa_aware_pattern();
    static std::unique_ptr<OptimizationPattern> create_memory_prefetch_pattern();
    
    // SIMD patterns
    static std::unique_ptr<OptimizationPattern> create_vectorization_pattern();
    static std::unique_ptr<OptimizationPattern> create_auto_vectorization_pattern();
    static std::unique_ptr<OptimizationPattern> create_simd_alignment_pattern();
    
    // Threading patterns
    static std::unique_ptr<OptimizationPattern> create_work_stealing_pattern();
    static std::unique_ptr<OptimizationPattern> create_thread_affinity_pattern();
    static std::unique_ptr<OptimizationPattern> create_lock_free_pattern();
    
    // Algorithmic patterns
    static std::unique_ptr<OptimizationPattern> create_branch_prediction_pattern();
    static std::unique_ptr<OptimizationPattern> create_loop_unrolling_pattern();
    static std::unique_ptr<OptimizationPattern> create_function_inlining_pattern();
};

//=============================================================================
// Performance Profiling Integration
//=============================================================================

/**
 * @brief Integration with platform profiling tools
 */
class ProfilerIntegration {
private:
    bool profiler_available_ = false;
    std::string profiler_type_;
    
public:
    // Profiler detection and setup
    bool detect_available_profilers();
    std::vector<std::string> get_available_profilers() const;
    bool setup_profiler(const std::string& profiler_name);
    
    // Profiling operations
    void start_profiling(const std::string& session_name);
    void stop_profiling();
    std::string get_profiling_results() const;
    
    // Analysis
    std::vector<std::string> identify_hotspots() const;
    std::vector<OptimizationRecommendation> generate_profile_based_recommendations() const;
    
    // Educational features
    std::string explain_profiling_results() const;
    void demonstrate_profiling_workflow() const;
};

//=============================================================================
// Global Optimization Engine
//=============================================================================

/**
 * @brief Get the global optimization engine instance
 */
OptimizationEngine& get_optimization_engine();

/**
 * @brief Initialize the global optimization system
 */
void initialize_global_optimization_engine(OptimizationEngineConfig config = {});

/**
 * @brief Quick optimization helpers
 */
namespace quick_optimize {
    std::vector<OptimizationRecommendation> get_top_recommendations(u32 count = 5);
    std::string get_optimization_summary();
    void apply_safe_optimizations();
    f64 measure_current_performance();
    std::string get_performance_tips();
}

} // namespace ecscope::platform