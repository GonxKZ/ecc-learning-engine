#pragma once

/**
 * @file panel_performance_comparison.hpp
 * @brief Interactive Performance Comparison Tools - Educational benchmarking interface
 * 
 * This panel provides comprehensive performance comparison tools specifically designed
 * for educational purposes, allowing students to understand the performance implications
 * of different ECS design choices through interactive benchmarks and real-time analysis.
 * 
 * Features:
 * - Interactive benchmark suite with configurable parameters
 * - Real-time performance graphs and comparisons
 * - Educational explanations of performance differences
 * - A/B testing framework for ECS patterns
 * - Memory layout visualization and analysis
 * - Cache behavior demonstration
 * - Scaling analysis with entity count variations
 * - Performance prediction and recommendations
 * 
 * Educational Design:
 * - Visual representation of abstract performance concepts
 * - Interactive parameter adjustment with immediate feedback
 * - Before/after comparisons with detailed explanations
 * - Progressive complexity from basic to advanced concepts
 * - Real-world scenario simulations
 * 
 * @author ECScope Educational Framework
 * @date 2024
 */

#include "../overlay.hpp"
#include "performance/performance_lab.hpp"
#include "memory/analysis/memory_benchmark_suite.hpp"
#include "core/types.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <chrono>
#include <functional>

namespace ecscope::ui {

// Forward declarations
class PerformanceComparisonPanel;
class BenchmarkRunner;
class PerformanceVisualizer;
class ComparisonChart;

/**
 * @brief Types of performance benchmarks available
 */
enum class BenchmarkType : u8 {
    EntityIteration,        // Basic entity iteration performance
    ComponentAccess,        // Component access patterns
    SystemExecution,        // System processing performance
    MemoryLayoutComparison, // SoA vs AoS memory layouts
    CacheBehaviorAnalysis,  // Cache hit/miss analysis
    AllocationStrategies,   // Memory allocation comparisons
    QueryPerformance,       // Component query benchmarks
    ArchetypeOperations,    // Archetype manipulation performance
    ScalingAnalysis,        // Performance scaling with entity count
    RealWorldScenarios      // Complex realistic scenarios
};

/**
 * @brief Benchmark configuration parameters
 */
struct BenchmarkConfig {
    BenchmarkType type;
    std::string name;
    std::string description;
    
    // Test parameters
    u32 entity_count{1000};
    u32 iterations{100};
    f64 time_limit{5.0}; // seconds
    bool warmup_enabled{true};
    u32 warmup_iterations{10};
    
    // Specific configuration
    std::unordered_map<std::string, std::string> parameters;
    
    // Educational settings
    bool show_explanation{true};
    bool show_memory_analysis{false};
    bool show_cache_analysis{false};
    std::string learning_objective;
    
    BenchmarkConfig() = default;
    BenchmarkConfig(BenchmarkType t, const std::string& n, const std::string& desc)
        : type(t), name(n), description(desc) {}
};

/**
 * @brief Results from a benchmark run
 */
struct BenchmarkResult {
    std::string benchmark_name;
    BenchmarkType type;
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point end_time;
    
    // Performance metrics
    f64 average_time_ms{0.0};
    f64 min_time_ms{0.0};
    f64 max_time_ms{0.0};
    f64 std_deviation_ms{0.0};
    u64 operations_per_second{0};
    
    // Memory metrics
    usize memory_usage_bytes{0};
    usize peak_memory_bytes{0};
    u32 allocations_count{0};
    u32 deallocations_count{0};
    f32 cache_hit_ratio{0.0f};
    f32 cache_miss_ratio{0.0f};
    
    // Detailed timing data
    std::vector<f64> iteration_times;
    std::vector<usize> memory_samples;
    
    // Analysis results
    std::string performance_category; // "Excellent", "Good", "Fair", "Poor"
    std::vector<std::string> insights;
    std::vector<std::string> recommendations;
    
    // Comparison data
    f64 baseline_ratio{1.0}; // Ratio compared to baseline
    bool is_baseline{false};
    
    BenchmarkResult() = default;
    BenchmarkResult(const std::string& name, BenchmarkType t) 
        : benchmark_name(name), type(t) {}
};

/**
 * @brief Comparison between two benchmark results
 */
struct BenchmarkComparison {
    BenchmarkResult baseline;
    BenchmarkResult comparison;
    
    // Comparison metrics
    f64 performance_improvement{0.0}; // Positive = improvement
    f64 memory_difference{0.0};        // Bytes difference
    f64 cache_improvement{0.0};        // Cache hit ratio improvement
    
    // Analysis
    std::string summary;
    std::vector<std::string> key_differences;
    std::vector<std::string> explanations;
    std::string recommendation;
    
    // Visual data
    std::vector<std::pair<std::string, f64>> metric_comparisons; // metric name -> improvement %
    
    BenchmarkComparison() = default;
    BenchmarkComparison(const BenchmarkResult& base, const BenchmarkResult& comp)
        : baseline(base), comparison(comp) {}
};

/**
 * @brief Interactive Performance Comparison Panel
 */
class PerformanceComparisonPanel : public Panel {
private:
    // Core systems integration
    std::shared_ptr<performance::PerformanceLab> performance_lab_;
    std::shared_ptr<memory::MemoryBenchmarkSuite> memory_benchmark_suite_;
    
    // Panel state
    enum class ComparisonMode : u8 {
        BenchmarkSelection, // Select and configure benchmarks
        RunningBenchmarks,  // Benchmarks in progress
        ResultsAnalysis,    // Analyze and compare results
        InteractiveDemo,    // Interactive performance demonstrations
        EducationalGuide,   // Guided educational content
        CustomComparison    // Custom A/B testing setup
    };
    
    ComparisonMode current_mode_{ComparisonMode::BenchmarkSelection};
    
    // Benchmark management
    std::vector<BenchmarkConfig> available_benchmarks_;
    std::vector<BenchmarkConfig> selected_benchmarks_;
    std::unordered_map<std::string, BenchmarkResult> benchmark_results_;
    std::vector<BenchmarkComparison> active_comparisons_;
    
    // Current benchmark execution
    bool benchmarks_running_{false};
    usize current_benchmark_index_{0};
    f32 overall_progress_{0.0f};
    std::string current_status_message_;
    std::chrono::steady_clock::time_point benchmark_start_time_;
    
    // Interactive demo state
    struct InteractiveDemo {
        std::string current_demo_id_;
        bool demo_active_{false};
        std::unordered_map<std::string, f32> demo_parameters_; // slider values
        std::vector<f64> real_time_measurements_;
        f64 measurement_update_frequency_{10.0}; // Hz
        f64 last_measurement_time_{0.0};
        
        // Demo visualization
        bool show_entity_visualization_{true};
        bool show_memory_layout_{false};
        bool show_cache_behavior_{false};
        f32 visualization_scale_{1.0f};
        
        InteractiveDemo() = default;
    };
    
    InteractiveDemo interactive_demo_;
    
    // Results analysis state
    struct ResultsAnalysis {
        std::string selected_baseline_; // Benchmark name to use as baseline
        std::vector<std::string> selected_comparisons_;
        
        // Analysis options
        bool show_detailed_timing_{false};
        bool show_memory_analysis_{true};
        bool show_cache_analysis_{false};
        bool show_scaling_projection_{true};
        bool normalize_to_baseline_{true};
        
        // Chart display options
        enum class ChartType : u8 {
            BarChart,        // Simple bar comparison
            LineChart,       // Performance over time
            ScatterPlot,     // Correlation analysis
            HeatMap,         // Multi-dimensional comparison
            RadarChart       // Multi-metric overview
        } chart_type_{ChartType::BarChart};
        
        f32 chart_height_{200.0f};
        bool animate_charts_{true};
        
        ResultsAnalysis() = default;
    };
    
    ResultsAnalysis results_analysis_;
    
    // Educational content system
    struct EducationalContent {
        std::unordered_map<BenchmarkType, std::string> benchmark_explanations_;
        std::unordered_map<std::string, std::string> concept_explanations_;
        std::vector<std::string> tutorial_steps_;
        int current_tutorial_step_{0};
        
        // Learning path
        std::vector<BenchmarkType> learning_sequence_;
        usize current_learning_step_{0};
        bool guided_mode_enabled_{false};
        
        // Help system
        bool context_help_enabled_{true};
        std::string current_help_topic_;
        
        EducationalContent() = default;
    };
    
    EducationalContent educational_content_;
    
    // Visualization settings
    struct VisualizationSettings {
        // Color scheme
        u32 excellent_color_{0xFF4CAF50};   // Green
        u32 good_color_{0xFF8BC34A};        // Light Green
        u32 fair_color_{0xFFFF9800};        // Orange
        u32 poor_color_{0xFFF44336};        // Red
        u32 baseline_color_{0xFF2196F3};    // Blue
        
        // Chart settings
        bool show_grid_{true};
        bool show_values_on_bars_{true};
        bool use_logarithmic_scale_{false};
        f32 animation_speed_{1.0f};
        
        // Accessibility
        bool high_contrast_mode_{false};
        bool large_text_mode_{false};
        f32 ui_scale_factor_{1.0f};
        
        VisualizationSettings() = default;
    };
    
    VisualizationSettings viz_settings_;
    
    // Performance data visualization
    struct PerformanceGraph {
        std::vector<f32> data_points_;
        std::vector<std::string> labels_;
        std::string title_;
        std::string y_axis_label_;
        f32 min_value_{0.0f};
        f32 max_value_{100.0f};
        u32 color_{0xFF2196F3};
        bool show_average_line_{true};
        
        PerformanceGraph() = default;
        PerformanceGraph(const std::string& title) : title_(title) {}
    };
    
    std::vector<PerformanceGraph> performance_graphs_;
    
    // A/B testing framework
    struct ABTestConfig {
        std::string test_name_;
        std::string description_;
        BenchmarkConfig config_a_;
        BenchmarkConfig config_b_;
        
        // Test parameters
        u32 sample_size_{10};
        f64 confidence_level_{0.95};
        bool randomize_order_{true};
        bool blind_test_{false}; // Hide which is A/B during execution
        
        // Results
        BenchmarkResult result_a_;
        BenchmarkResult result_b_;
        bool test_completed_{false};
        f64 statistical_significance_{0.0};
        std::string conclusion_;
        
        ABTestConfig() = default;
        ABTestConfig(const std::string& name, const std::string& desc)
            : test_name_(name), description_(desc) {}
    };
    
    std::vector<ABTestConfig> ab_tests_;
    usize current_ab_test_index_{0};
    
    // Rendering methods
    void render_benchmark_selection();
    void render_running_benchmarks();
    void render_results_analysis();
    void render_interactive_demo();
    void render_educational_guide();
    void render_custom_comparison();
    
    // Benchmark selection components
    void render_benchmark_categories();
    void render_benchmark_list();
    void render_benchmark_configuration();
    void render_benchmark_preview();
    
    // Benchmark execution components
    void render_execution_progress();
    void render_real_time_metrics();
    void render_current_benchmark_info();
    void render_execution_controls();
    
    // Results analysis components
    void render_results_overview();
    void render_comparison_charts();
    void render_detailed_metrics();
    void render_performance_insights();
    void render_recommendations();
    
    // Interactive demo components
    void render_demo_selection();
    void render_demo_controls();
    void render_real_time_visualization();
    void render_parameter_sliders();
    
    // Educational guide components
    void render_learning_path();
    void render_concept_explanations();
    void render_guided_tutorial();
    void render_context_help();
    
    // Chart rendering methods
    void render_bar_chart(const std::vector<BenchmarkResult>& results, f32 height);
    void render_line_chart(const PerformanceGraph& graph, f32 height);
    void render_scatter_plot(const std::vector<BenchmarkResult>& results, f32 height);
    void render_radar_chart(const BenchmarkResult& result, f32 height);
    void render_heatmap(const std::vector<BenchmarkComparison>& comparisons, f32 height);
    
    // Data visualization helpers
    void draw_performance_bar(const std::string& label, f64 value, f64 max_value, u32 color, f32 width, f32 height);
    void draw_comparison_arrow(f64 improvement, f32 x, f32 y);
    void draw_trend_line(const std::vector<f64>& data, f32 width, f32 height, u32 color);
    void draw_confidence_interval(f64 mean, f64 std_dev, f32 x, f32 width, u32 color);
    
    // Benchmark management
    void initialize_default_benchmarks();
    void add_benchmark_config(const BenchmarkConfig& config);
    void remove_benchmark_config(const std::string& name);
    void start_benchmark_suite();
    void stop_benchmark_suite();
    void run_single_benchmark(const BenchmarkConfig& config);
    void process_benchmark_result(const BenchmarkResult& result);
    
    // Analysis methods
    void analyze_benchmark_results();
    void generate_comparisons();
    void calculate_performance_insights(BenchmarkResult& result);
    void generate_recommendations(BenchmarkResult& result);
    BenchmarkComparison compare_results(const BenchmarkResult& baseline, const BenchmarkResult& comparison);
    
    // Interactive demo management
    void start_interactive_demo(const std::string& demo_id);
    void update_demo_parameters();
    void measure_demo_performance();
    void stop_interactive_demo();
    
    // Educational content management
    void initialize_educational_content();
    void advance_tutorial_step();
    void show_concept_explanation(const std::string& concept);
    void update_learning_path();
    
    // A/B testing methods
    void setup_ab_test(const ABTestConfig& config);
    void run_ab_test(ABTestConfig& test);
    void analyze_ab_test_results(ABTestConfig& test);
    void calculate_statistical_significance(const ABTestConfig& test);
    
    // Performance prediction
    struct PerformancePrediction {
        std::string metric_name_;
        f64 predicted_value_;
        f64 confidence_interval_lower_;
        f64 confidence_interval_upper_;
        std::string explanation_;
        
        PerformancePrediction() = default;
    };
    
    std::vector<PerformancePrediction> predict_scaling_performance(const BenchmarkResult& result, u32 target_entity_count);
    PerformancePrediction predict_memory_usage(const BenchmarkResult& result, u32 target_entity_count);
    
    // Utility methods
    std::string format_time_measurement(f64 time_ms) const;
    std::string format_memory_size(usize bytes) const;
    std::string format_performance_improvement(f64 improvement) const;
    std::string format_statistical_confidence(f64 confidence) const;
    std::string get_performance_category(f64 value, f64 baseline) const;
    u32 get_performance_color(const std::string& category) const;
    f32 normalize_performance_value(f64 value, f64 min_val, f64 max_val) const;
    
    // Event handlers
    void on_benchmark_completed(const BenchmarkResult& result);
    void on_benchmark_suite_completed();
    void on_demo_parameter_changed(const std::string& parameter, f32 value);
    void on_comparison_selected(const std::string& baseline, const std::string& comparison);
    
public:
    explicit PerformanceComparisonPanel(std::shared_ptr<performance::PerformanceLab> lab);
    ~PerformanceComparisonPanel() override = default;
    
    // Panel interface
    void render() override;
    void update(f64 delta_time) override;
    bool wants_keyboard_capture() const override;
    bool wants_mouse_capture() const override;
    
    // Benchmark control
    void add_benchmark(const BenchmarkConfig& config);
    void remove_benchmark(const std::string& name);
    void clear_all_benchmarks();
    void start_benchmarks();
    void stop_benchmarks();
    void reset_results();
    
    // Results access
    const std::vector<BenchmarkResult>& get_results() const { return {}; } // Placeholder
    BenchmarkResult get_result(const std::string& name) const;
    std::vector<BenchmarkComparison> get_comparisons() const { return active_comparisons_; }
    
    // Interactive demos
    void start_demo(const std::string& demo_id);
    void set_demo_parameter(const std::string& parameter, f32 value);
    void stop_current_demo();
    
    // Educational features
    void start_guided_learning();
    void set_learning_level(const std::string& level);
    void enable_context_help(bool enabled);
    void show_explanation(const std::string& topic);
    
    // Visualization configuration
    void set_chart_type(ResultsAnalysis::ChartType type);
    void set_visualization_option(const std::string& option, bool enabled);
    void set_color_scheme(const std::string& scheme);
    void export_chart(const std::string& filename, const std::string& format = "png");
    
    // A/B testing
    void create_ab_test(const std::string& name, const BenchmarkConfig& config_a, const BenchmarkConfig& config_b);
    void run_ab_test(const std::string& test_name);
    ABTestConfig get_ab_test_result(const std::string& test_name) const;
    
    // Data export
    void export_results(const std::string& filename, const std::string& format = "json");
    void export_comparison_report(const std::string& filename);
    void export_educational_report(const std::string& filename);
    
    // Integration
    void set_performance_lab(std::shared_ptr<performance::PerformanceLab> lab);
    void set_memory_benchmark_suite(std::shared_ptr<memory::MemoryBenchmarkSuite> suite);
    
    // State queries
    bool is_running_benchmarks() const { return benchmarks_running_; }
    f32 get_progress() const { return overall_progress_; }
    usize get_completed_benchmark_count() const { return benchmark_results_.size(); }
    ComparisonMode get_current_mode() const { return current_mode_; }
    
private:
    // Constants
    static constexpr f32 MIN_PANEL_WIDTH = 800.0f;
    static constexpr f32 MIN_PANEL_HEIGHT = 600.0f;
    static constexpr f32 CHART_MIN_HEIGHT = 150.0f;
    static constexpr f32 CHART_MAX_HEIGHT = 400.0f;
    static constexpr f32 PROGRESS_BAR_HEIGHT = 20.0f;
    static constexpr f32 SLIDER_WIDTH = 200.0f;
    static constexpr u32 MAX_BENCHMARK_RESULTS = 100;
    static constexpr f64 MEASUREMENT_UPDATE_FREQUENCY = 10.0; // Hz
    static constexpr f64 BENCHMARK_TIMEOUT = 60.0; // seconds
    static constexpr f32 STATISTICAL_SIGNIFICANCE_THRESHOLD = 0.05f;
    
    // Update frequencies
    static constexpr f64 PROGRESS_UPDATE_FREQUENCY = 5.0; // Hz
    static constexpr f64 DEMO_UPDATE_FREQUENCY = 30.0; // Hz
    static constexpr f64 RESULTS_ANALYSIS_FREQUENCY = 1.0; // Hz
    
    f64 last_progress_update_{0.0};
    f64 last_demo_update_{0.0};
    f64 last_analysis_update_{0.0};
};

/**
 * @brief Specialized benchmark runner for educational performance testing
 */
class EducationalBenchmarkRunner {
public:
    struct BenchmarkSession {
        std::vector<BenchmarkConfig> benchmarks;
        std::vector<BenchmarkResult> results;
        std::chrono::steady_clock::time_point start_time;
        std::chrono::steady_clock::time_point end_time;
        std::string session_id;
        
        BenchmarkSession() = default;
    };
    
private:
    std::shared_ptr<performance::PerformanceLab> performance_lab_;
    BenchmarkSession current_session_;
    bool is_running_{false};
    std::function<void(const BenchmarkResult&)> result_callback_;
    std::function<void(f32)> progress_callback_;
    
public:
    explicit EducationalBenchmarkRunner(std::shared_ptr<performance::PerformanceLab> lab);
    
    void start_session(const std::vector<BenchmarkConfig>& benchmarks);
    void stop_session();
    void pause_session();
    void resume_session();
    
    void set_result_callback(std::function<void(const BenchmarkResult&)> callback);
    void set_progress_callback(std::function<void(f32)> callback);
    
    BenchmarkResult run_benchmark(const BenchmarkConfig& config);
    std::vector<BenchmarkResult> run_benchmark_suite(const std::vector<BenchmarkConfig>& configs);
    
    const BenchmarkSession& current_session() const { return current_session_; }
    bool is_running() const { return is_running_; }
    f32 get_progress() const;
    
private:
    void run_session_async();
    BenchmarkResult execute_single_benchmark(const BenchmarkConfig& config);
    void warm_up_benchmark(const BenchmarkConfig& config);
    void collect_performance_metrics(BenchmarkResult& result, const BenchmarkConfig& config);
};

/**
 * @brief Interactive performance visualization widget
 */
class InteractivePerformanceChart {
public:
    enum class ChartType : u8 {
        LineChart,
        BarChart,
        ScatterPlot,
        HeatMap,
        RadarChart
    };
    
private:
    ChartType chart_type_{ChartType::BarChart};
    std::vector<std::pair<std::string, std::vector<f64>>> data_series_;
    std::string title_;
    std::string x_axis_label_;
    std::string y_axis_label_;
    
    // Visual settings
    f32 width_{400.0f};
    f32 height_{300.0f};
    bool show_grid_{true};
    bool show_legend_{true};
    bool animate_transitions_{true};
    
    // Interaction state
    bool is_hovered_{false};
    usize hovered_series_{SIZE_MAX};
    usize hovered_point_{SIZE_MAX};
    
public:
    InteractivePerformanceChart(const std::string& title = "Performance Chart");
    
    void render();
    void update(f64 delta_time);
    
    void set_chart_type(ChartType type);
    void add_data_series(const std::string& name, const std::vector<f64>& data);
    void clear_data();
    void set_labels(const std::string& x_label, const std::string& y_label);
    void set_size(f32 width, f32 height);
    
    bool handle_mouse_interaction(f32 mouse_x, f32 mouse_y);
    void show_tooltip(const std::string& content);
    
private:
    void render_line_chart();
    void render_bar_chart();
    void render_scatter_plot();
    void render_heat_map();
    void render_radar_chart();
    
    void draw_grid();
    void draw_axes();
    void draw_legend();
    void draw_tooltip();
};

} // namespace ecscope::ui