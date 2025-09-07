#pragma once

#include "ml_prediction_system.hpp"
#include "ecs_behavior_predictor.hpp"
#include "ecs_performance_predictor.hpp"
#include "ecs_memory_predictor.hpp"
#include "ml_model_manager.hpp"
#include "entity.hpp"
#include "component.hpp"
#include "registry.hpp"
#include <vector>
#include <memory>
#include <string>
#include <chrono>
#include <unordered_map>
#include <functional>
#include <sstream>
#include <algorithm>
#include <numeric>
#include <cmath>

namespace ecscope::ml {

/**
 * @brief Chart types for visualization
 */
enum class ChartType {
    LineChart,        // Time series data
    BarChart,         // Categorical data
    ScatterPlot,      // Correlation data
    Histogram,        // Distribution data
    HeatMap,          // 2D data matrix
    Timeline,         // Event timeline
    Network,          // Relationship graph
    TreeDiagram,      // Hierarchical data
    PieChart,         // Proportional data
    BoxPlot           // Statistical distribution
};

/**
 * @brief Chart configuration and styling
 */
struct ChartConfig {
    ChartType chart_type{ChartType::LineChart};
    std::string title{"Untitled Chart"};
    std::string x_axis_label{"X Axis"};
    std::string y_axis_label{"Y Axis"};
    
    // Dimensions and layout
    usize width{800};
    usize height{600};
    usize margin_left{60};
    usize margin_right{40};
    usize margin_top{40};
    usize margin_bottom{60};
    
    // Colors and styling
    std::string background_color{"#ffffff"};
    std::string grid_color{"#e0e0e0"};
    std::string text_color{"#333333"};
    std::vector<std::string> series_colors{
        "#1f77b4", "#ff7f0e", "#2ca02c", "#d62728", "#9467bd",
        "#8c564b", "#e377c2", "#7f7f7f", "#bcbd22", "#17becf"
    };
    
    // Data presentation
    bool show_grid{true};
    bool show_legend{true};
    bool show_data_points{false};
    bool enable_animation{true};
    f32 line_thickness{2.0f};
    
    // Interactive features
    bool enable_zoom{true};
    bool enable_pan{true};
    bool show_tooltips{true};
    bool enable_data_export{true};
    
    // Educational features
    bool show_explanations{true};
    bool highlight_insights{true};
    bool show_confidence_intervals{false};
};

/**
 * @brief Data series for charts
 */
struct ChartDataSeries {
    std::string name{"Series"};
    std::string color{"#1f77b4"};
    ChartType preferred_chart_type{ChartType::LineChart};
    
    // Numeric data
    std::vector<f32> x_values;
    std::vector<f32> y_values;
    std::vector<f32> z_values; // For 3D or heatmap data
    
    // Time series data
    std::vector<std::chrono::high_resolution_clock::time_point> timestamps;
    
    // Categorical data
    std::vector<std::string> categories;
    std::vector<std::string> labels;
    
    // Statistical data
    std::vector<f32> error_bars;
    std::vector<f32> confidence_intervals_lower;
    std::vector<f32> confidence_intervals_upper;
    
    // Metadata for educational purposes
    std::string description;
    std::string data_source;
    std::unordered_map<std::string, std::string> metadata;
    
    // Data validation and statistics
    bool is_valid() const;
    usize size() const;
    f32 min_x() const;
    f32 max_x() const;
    f32 min_y() const;
    f32 max_y() const;
    f32 mean_y() const;
    f32 std_dev_y() const;
    
    std::string to_string() const;
};

/**
 * @brief Complete chart with data and configuration
 */
struct Chart {
    ChartConfig config;
    std::vector<ChartDataSeries> data_series;
    
    // Annotations and highlights
    struct Annotation {
        f32 x{0.0f};
        f32 y{0.0f};
        std::string text;
        std::string color{"#ff0000"};
        bool is_important{false};
    };
    std::vector<Annotation> annotations;
    
    // Educational content
    std::string explanation;
    std::string interpretation;
    std::vector<std::string> key_insights;
    
    // Export formats
    std::string to_svg() const;
    std::string to_html() const;
    std::string to_css_style() const;
    std::string to_json() const;
    bool save_to_file(const std::string& filename, const std::string& format = "svg") const;
    
    // Validation
    bool is_valid() const;
    std::string validate() const;
    
    // Educational methods
    void add_insight_annotation(f32 x, f32 y, const std::string& insight);
    void highlight_trend(usize series_index, f32 start_x, f32 end_x);
    void add_explanation(const std::string& explanation_text);
};

/**
 * @brief Configuration for visualization system
 */
struct VisualizationConfig {
    // Output settings
    std::string output_directory{"visualizations"};
    std::string default_format{"svg"};
    bool auto_save_charts{true};
    bool generate_interactive_charts{true};
    
    // Chart defaults
    ChartConfig default_chart_config;
    usize max_data_points_per_series{1000};
    bool enable_data_aggregation{true};
    
    // Educational features
    bool enable_explanatory_text{true};
    bool show_statistical_analysis{true};
    bool highlight_anomalies{true};
    bool generate_insights_automatically{true};
    
    // Performance settings
    bool enable_chart_caching{true};
    std::chrono::minutes chart_cache_ttl{10};
    usize max_cached_charts{50};
    
    // Integration settings
    bool real_time_updates{true};
    std::chrono::milliseconds update_interval{1000};
    bool enable_dashboard_mode{true};
};

/**
 * @brief Main visualization system for ML predictions
 * 
 * This system creates educational and informative visualizations of ML model
 * predictions, training progress, and ECS system behavior. It's designed to
 * help understand how AI/ML enhances game engine performance.
 */
class MLVisualizationSystem {
private:
    VisualizationConfig config_;
    
    // Chart cache for performance
    std::unordered_map<std::string, std::pair<Chart, std::chrono::high_resolution_clock::time_point>> chart_cache_;
    mutable std::mutex cache_mutex_;
    
    // Data sources (weak references to avoid circular dependencies)
    ECSBehaviorPredictor* behavior_predictor_{nullptr};
    ECSPerformancePredictor* performance_predictor_{nullptr};
    ECSMemoryPredictor* memory_predictor_{nullptr};
    MLModelManager* model_manager_{nullptr};
    
    // Chart generation statistics
    std::atomic<usize> charts_generated_{0};
    std::atomic<usize> cache_hits_{0};
    std::atomic<usize> cache_misses_{0};
    
public:
    explicit MLVisualizationSystem(const VisualizationConfig& config = VisualizationConfig{});
    ~MLVisualizationSystem() = default;
    
    // Non-copyable but movable
    MLVisualizationSystem(const MLVisualizationSystem&) = delete;
    MLVisualizationSystem& operator=(const MLVisualizationSystem&) = delete;
    MLVisualizationSystem(MLVisualizationSystem&&) = default;
    MLVisualizationSystem& operator=(MLVisualizationSystem&&) = default;
    
    // System integration
    void set_behavior_predictor(ECSBehaviorPredictor* predictor) { behavior_predictor_ = predictor; }
    void set_performance_predictor(ECSPerformancePredictor* predictor) { performance_predictor_ = predictor; }
    void set_memory_predictor(ECSMemoryPredictor* predictor) { memory_predictor_ = predictor; }
    void set_model_manager(MLModelManager* manager) { model_manager_ = manager; }
    
    // Entity behavior visualizations
    Chart visualize_entity_behavior_pattern(EntityID entity, const EntityBehaviorPattern& pattern);
    Chart visualize_entity_lifecycle(const std::vector<EntityID>& entities, const ecs::Registry& registry);
    Chart visualize_component_usage_over_time(const std::string& component_type, 
                                             const std::vector<std::chrono::high_resolution_clock::time_point>& timestamps,
                                             const std::vector<usize>& usage_counts);
    Chart visualize_behavior_prediction_accuracy(const std::vector<BehaviorPrediction>& predictions,
                                                const std::vector<bool>& actual_results);
    
    // Performance visualizations
    Chart visualize_performance_timeline(const std::vector<PerformanceSnapshot>& snapshots);
    Chart visualize_bottleneck_predictions(const std::vector<PerformanceBottleneckPrediction>& predictions);
    Chart visualize_system_performance_comparison(const std::unordered_map<std::string, std::vector<f32>>& system_times);
    Chart visualize_frame_rate_analysis(const std::vector<f32>& frame_times,
                                       const std::vector<std::chrono::high_resolution_clock::time_point>& timestamps);
    
    // Memory usage visualizations
    Chart visualize_memory_usage_timeline(const std::vector<f32>& memory_usage,
                                         const std::vector<std::chrono::high_resolution_clock::time_point>& timestamps);
    Chart visualize_allocation_patterns(const std::vector<MemoryAllocationEvent>& allocations);
    Chart visualize_memory_fragmentation(const std::vector<f32>& fragmentation_levels,
                                        const std::vector<std::chrono::high_resolution_clock::time_point>& timestamps);
    Chart visualize_allocator_efficiency_comparison(const std::unordered_map<std::string, f32>& efficiency_scores);
    
    // Model training visualizations
    Chart visualize_training_progress(const TrainingProgress& progress);
    Chart visualize_learning_curve(const std::vector<f32>& training_losses, const std::vector<f32>& validation_losses);
    Chart visualize_model_accuracy_over_time(const std::string& model_name,
                                            const std::vector<f32>& accuracy_history);
    Chart visualize_feature_importance(const std::vector<std::string>& feature_names,
                                      const std::vector<f32>& importance_scores);
    
    // Prediction quality visualizations
    Chart visualize_prediction_confidence_distribution(const std::vector<f32>& confidence_scores);
    Chart visualize_prediction_error_analysis(const std::vector<f32>& predicted_values,
                                             const std::vector<f32>& actual_values);
    Chart visualize_model_comparison(const std::vector<ModelValidationResult>& results);
    
    // System overview visualizations
    Chart create_ecs_system_dashboard(const ecs::Registry& registry);
    Chart create_ml_system_overview();
    Chart create_performance_summary_dashboard();
    std::vector<Chart> create_comprehensive_analysis_report(const ecs::Registry& registry);
    
    // Educational visualizations
    Chart explain_ml_concept_with_visualization(const std::string& concept_name);
    Chart demonstrate_overfitting_vs_generalization(const ModelValidationResult& result);
    Chart show_bias_variance_tradeoff(const std::vector<ModelValidationResult>& model_results);
    Chart illustrate_training_data_importance(const TrainingDataset& dataset);
    
    // Interactive dashboard creation
    std::string create_interactive_dashboard(const ecs::Registry& registry);
    std::string create_model_training_monitor();
    std::string create_real_time_performance_monitor();
    
    // Chart utilities and helpers
    Chart create_custom_chart(const ChartConfig& config, const std::vector<ChartDataSeries>& data);
    void add_trend_line(Chart& chart, usize series_index);
    void add_statistical_annotations(Chart& chart, usize series_index);
    void apply_smoothing(ChartDataSeries& series, usize window_size = 5);
    
    // Data processing for visualization
    ChartDataSeries create_time_series_from_data(const std::vector<f32>& values,
                                                 const std::vector<std::chrono::high_resolution_clock::time_point>& timestamps,
                                                 const std::string& name = "Data");
    ChartDataSeries aggregate_data_for_visualization(const ChartDataSeries& original_series, usize target_points);
    std::vector<ChartDataSeries> detect_anomalies_in_series(const ChartDataSeries& series);
    
    // Export and sharing
    bool save_chart(const Chart& chart, const std::string& filename, const std::string& format = "svg");
    std::string export_chart_data_csv(const Chart& chart);
    std::string export_chart_data_json(const Chart& chart);
    bool generate_chart_report(const std::vector<Chart>& charts, const std::string& filename);
    
    // Configuration and cache management
    const VisualizationConfig& config() const { return config_; }
    void update_config(const VisualizationConfig& new_config);
    void clear_chart_cache();
    void cleanup_expired_cache_entries();
    
    // Statistics and insights
    usize total_charts_generated() const { return charts_generated_.load(); }
    f32 cache_hit_ratio() const;
    std::string generate_visualization_usage_report() const;
    
private:
    // Internal chart generation
    Chart generate_chart_internal(const std::string& chart_id, std::function<Chart()> generator);
    std::string calculate_chart_cache_key(const std::string& chart_type, const std::string& parameters) const;
    
    // Chart styling and formatting
    void apply_educational_styling(Chart& chart);
    void add_automatic_insights(Chart& chart, const ChartDataSeries& primary_series);
    void format_chart_for_export(Chart& chart, const std::string& format);
    
    // Data analysis helpers
    std::vector<std::string> analyze_time_series_trends(const ChartDataSeries& series);
    std::vector<std::string> identify_data_patterns(const ChartDataSeries& series);
    std::vector<Chart::Annotation> generate_insight_annotations(const ChartDataSeries& series);
    
    // SVG generation helpers
    std::string generate_svg_header(usize width, usize height) const;
    std::string generate_svg_line_chart(const Chart& chart) const;
    std::string generate_svg_bar_chart(const Chart& chart) const;
    std::string generate_svg_scatter_plot(const Chart& chart) const;
    std::string generate_svg_legend(const Chart& chart) const;
    std::string generate_svg_axes(const Chart& chart) const;
    
    // HTML generation helpers
    std::string generate_html_wrapper(const Chart& chart) const;
    std::string generate_javascript_interactivity(const Chart& chart) const;
    std::string generate_css_styling(const Chart& chart) const;
    
    // Educational content generation
    std::string generate_chart_explanation(const Chart& chart) const;
    std::vector<std::string> generate_automatic_insights(const Chart& chart) const;
    std::string explain_visualization_technique(ChartType chart_type) const;
    
    // Utility functions
    std::string format_number(f32 value, usize decimal_places = 2) const;
    std::string format_timestamp(const std::chrono::high_resolution_clock::time_point& timestamp) const;
    std::string escape_html(const std::string& text) const;
    std::string color_for_index(usize index) const;
};

/**
 * @brief Specialized chart generators for specific ML concepts
 */
namespace visualization_generators {

// Behavior prediction visualizations
Chart create_entity_behavior_heatmap(const std::unordered_map<EntityID, EntityBehaviorPattern>& patterns);
Chart create_component_dependency_graph(const ecs::Registry& registry);
Chart create_behavioral_clustering_visualization(const std::vector<EntityBehaviorPattern>& patterns);

// Performance analysis visualizations
Chart create_system_bottleneck_analysis(const std::vector<PerformanceBottleneckPrediction>& bottlenecks);
Chart create_resource_utilization_stacked_chart(const std::vector<PerformanceSnapshot>& snapshots);
Chart create_performance_regression_analysis(const std::vector<f32>& performance_timeline);

// Memory analysis visualizations
Chart create_memory_allocation_timeline(const std::vector<MemoryAllocationEvent>& events);
Chart create_allocator_comparison_radar_chart(const std::unordered_map<std::string, f32>& allocator_metrics);
Chart create_memory_fragmentation_visualization(const std::vector<f32>& fragmentation_data);

// ML model visualizations
Chart create_model_architecture_diagram(const MLModelBase& model);
Chart create_hyperparameter_optimization_surface(const std::vector<MLModelConfig>& configs,
                                                 const std::vector<f32>& performance_scores);
Chart create_cross_validation_results_chart(const ModelValidationResult& cv_results);

// Educational concept visualizations
Chart illustrate_gradient_descent_optimization(const std::vector<f32>& loss_history);
Chart demonstrate_regularization_effects(const std::vector<f32>& training_losses,
                                        const std::vector<f32>& validation_losses);
Chart show_feature_correlation_matrix(const std::vector<std::vector<f32>>& correlation_matrix,
                                     const std::vector<std::string>& feature_names);

} // namespace visualization_generators

/**
 * @brief Utility functions for visualization
 */
namespace visualization_utils {

// Data preprocessing
std::vector<f32> smooth_data(const std::vector<f32>& data, usize window_size = 5);
std::vector<f32> normalize_data(const std::vector<f32>& data);
std::pair<std::vector<f32>, std::vector<f32>> downsample_data(const std::vector<f32>& x_data,
                                                             const std::vector<f32>& y_data,
                                                             usize target_points);

// Statistical analysis
f32 calculate_correlation(const std::vector<f32>& x, const std::vector<f32>& y);
std::vector<f32> calculate_moving_average(const std::vector<f32>& data, usize window_size);
std::pair<f32, f32> calculate_trend_line(const std::vector<f32>& x, const std::vector<f32>& y);

// Color and styling utilities
std::string interpolate_color(const std::string& color1, const std::string& color2, f32 t);
std::vector<std::string> generate_color_palette(usize count, const std::string& base_color = "#1f77b4");
std::string darken_color(const std::string& color, f32 factor = 0.8f);

// Export utilities
bool save_chart_as_png(const Chart& chart, const std::string& filename);
std::string chart_to_markdown(const Chart& chart);
std::string create_chart_embedding_code(const Chart& chart, const std::string& format = "svg");

} // namespace visualization_utils

} // namespace ecscope::ml