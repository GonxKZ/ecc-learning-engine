#pragma once

/**
 * @file ecs_performance_visualizer.hpp
 * @brief Educational ECS Performance Visualization and Interactive Analysis
 * 
 * This component provides comprehensive visualization tools for ECS performance
 * analysis, including real-time graphs, educational explanations, bottleneck
 * identification, and interactive optimization recommendations.
 * 
 * Key Features:
 * - Real-time performance monitoring and visualization
 * - Interactive performance graphs (scaling curves, comparison charts)
 * - Educational overlays with performance explanations
 * - Bottleneck identification with visual highlighting
 * - Architecture comparison visualizations
 * - Cache behavior and memory access pattern visualization
 * - Optimization recommendation system with visual impact estimates
 * 
 * Educational Value:
 * - Visual demonstration of performance characteristics
 * - Interactive exploration of optimization trade-offs
 * - Real-time feedback on architectural decisions
 * - Comparative analysis with visual side-by-side comparisons
 * - Performance pattern recognition training
 * 
 * @author ECScope Educational ECS Framework - Performance Visualization
 * @date 2025
 */

#include "ecs_performance_benchmarker.hpp"
#include "performance_lab.hpp"
#include "core/types.hpp"
#include "core/log.hpp"
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <mutex>
#include <array>

namespace ecscope::performance::visualization {

//=============================================================================
// Visualization Data Structures
//=============================================================================

/**
 * @brief Point data for performance graphs
 */
struct PerformanceDataPoint {
    f64 x_value;        // X-axis value (entity count, time, etc.)
    f64 y_value;        // Y-axis value (performance metric)
    f64 confidence;     // Confidence level (0.0-1.0)
    std::string label;  // Optional data point label
    
    PerformanceDataPoint(f64 x = 0.0, f64 y = 0.0, f64 conf = 1.0, const std::string& lbl = "")
        : x_value(x), y_value(y), confidence(conf), label(lbl) {}
};

/**
 * @brief Performance graph series data
 */
struct PerformanceGraphSeries {
    std::string name;                           // Series name
    std::string description;                    // Series description
    std::vector<PerformanceDataPoint> points;  // Data points
    u32 color;                                 // RGBA color for visualization
    bool is_visible;                           // Visibility flag
    f64 line_thickness;                        // Line thickness
    
    PerformanceGraphSeries(const std::string& n = "", u32 c = 0xFFFFFFFF)
        : name(n), color(c), is_visible(true), line_thickness(2.0) {}
};

/**
 * @brief Performance graph configuration
 */
struct PerformanceGraphConfig {
    std::string title;              // Graph title
    std::string x_axis_label;       // X-axis label
    std::string y_axis_label;       // Y-axis label
    std::string units;             // Y-axis units
    bool logarithmic_y;            // Use logarithmic Y scale
    bool show_grid;                // Show grid lines
    bool show_legend;              // Show legend
    bool interactive;              // Enable interactivity
    f64 x_min, x_max;             // X-axis range (0.0 = auto)
    f64 y_min, y_max;             // Y-axis range (0.0 = auto)
    
    PerformanceGraphConfig()
        : logarithmic_y(false), show_grid(true), show_legend(true), 
          interactive(true), x_min(0.0), x_max(0.0), y_min(0.0), y_max(0.0) {}
};

/**
 * @brief Complete performance graph with series and configuration
 */
struct PerformanceGraph {
    PerformanceGraphConfig config;
    std::vector<PerformanceGraphSeries> series;
    std::string educational_explanation;        // Educational context
    std::vector<std::string> key_insights;     // Key insights from the data
    std::vector<std::string> optimization_tips; // Optimization suggestions
    
    void add_series(const PerformanceGraphSeries& series_data) {
        series.push_back(series_data);
    }
    
    void clear_series() {
        series.clear();
    }
};

/**
 * @brief Bottleneck identification data
 */
struct PerformanceBottleneck {
    enum class Type {
        Memory,         // Memory-related bottleneck
        Cache,          // Cache behavior bottleneck
        Algorithm,      // Algorithmic bottleneck
        Threading,      // Multi-threading bottleneck
        Integration     // System integration bottleneck
    };
    
    Type type;                              // Bottleneck type
    std::string name;                       // Bottleneck name
    std::string description;                // Detailed description
    f64 impact_factor;                     // Impact on performance (1.0-10.0)
    f64 fix_difficulty;                    // Difficulty to fix (0.0-1.0)
    std::vector<std::string> symptoms;      // Observable symptoms
    std::vector<std::string> solutions;     // Suggested solutions
    bool is_critical;                      // Critical bottleneck flag
    
    PerformanceBottleneck(Type t, const std::string& n)
        : type(t), name(n), impact_factor(1.0), fix_difficulty(0.5), is_critical(false) {}
};

/**
 * @brief Real-time performance monitoring data
 */
struct RealTimePerformanceData {
    f64 timestamp;                          // When data was recorded
    f64 frame_time_ms;                     // Current frame time
    f64 ecs_update_time_ms;                // ECS update time
    u32 entity_count;                      // Current entity count
    u32 archetype_count;                   // Current archetype count
    usize memory_usage_bytes;              // Current memory usage
    f64 cache_hit_ratio;                   // Cache hit ratio
    f64 cpu_utilization;                   // CPU utilization
    
    RealTimePerformanceData()
        : timestamp(0.0), frame_time_ms(0.0), ecs_update_time_ms(0.0),
          entity_count(0), archetype_count(0), memory_usage_bytes(0),
          cache_hit_ratio(0.0), cpu_utilization(0.0) {}
};

//=============================================================================
// Educational Visualization Components
//=============================================================================

/**
 * @brief Interactive performance comparison visualization
 */
class ArchitectureComparisonVisualizer {
private:
    std::vector<ecs::ECSBenchmarkResult> comparison_results_;
    std::unordered_map<std::string, PerformanceGraph> comparison_graphs_;
    mutable std::mutex data_mutex_;
    
public:
    /** @brief Update comparison data */
    void update_comparison_data(const std::vector<ecs::ECSBenchmarkResult>& results);
    
    /** @brief Generate scaling comparison graph */
    PerformanceGraph generate_scaling_comparison(const std::string& test_name) const;
    
    /** @brief Generate architecture performance radar chart */
    PerformanceGraph generate_architecture_radar() const;
    
    /** @brief Generate memory efficiency comparison */
    PerformanceGraph generate_memory_comparison() const;
    
    /** @brief Get educational explanation for comparison */
    std::string get_comparison_explanation(const std::string& architecture1,
                                         const std::string& architecture2) const;
    
    /** @brief Get optimization recommendations based on comparison */
    std::vector<std::string> get_optimization_recommendations(
        const std::string& architecture) const;
};

/**
 * @brief Real-time performance monitoring and visualization
 */
class RealTimePerformanceMonitor {
private:
    static constexpr usize MAX_HISTORY_SIZE = 1000;
    
    std::array<RealTimePerformanceData, MAX_HISTORY_SIZE> history_;
    usize history_index_;
    usize history_size_;
    mutable std::mutex history_mutex_;
    
    std::atomic<bool> is_monitoring_{false};
    std::chrono::steady_clock::time_point start_time_;
    
    // Thresholds for warnings
    f64 frame_time_warning_ms_{16.67}; // 60 FPS
    f64 memory_warning_mb_{100.0};
    f64 cache_miss_warning_ratio_{0.2};
    
public:
    RealTimePerformanceMonitor();
    
    /** @brief Start monitoring */
    void start_monitoring();
    
    /** @brief Stop monitoring */
    void stop_monitoring();
    
    /** @brief Add performance data point */
    void add_data_point(const RealTimePerformanceData& data);
    
    /** @brief Generate real-time performance graph */
    PerformanceGraph generate_realtime_graph(const std::string& metric) const;
    
    /** @brief Get current performance summary */
    std::string get_performance_summary() const;
    
    /** @brief Check for performance warnings */
    std::vector<std::string> get_performance_warnings() const;
    
    /** @brief Get recent performance data */
    std::vector<RealTimePerformanceData> get_recent_data(usize count) const;
    
    /** @brief Set warning thresholds */
    void set_warning_thresholds(f64 frame_time_ms, f64 memory_mb, f64 cache_miss_ratio);
};

/**
 * @brief Bottleneck identification and visualization system
 */
class BottleneckAnalyzer {
private:
    std::vector<PerformanceBottleneck> identified_bottlenecks_;
    std::unordered_map<std::string, f64> performance_metrics_;
    mutable std::mutex analysis_mutex_;
    
    // Analysis thresholds
    f64 memory_bottleneck_threshold_{0.8};    // 80% memory usage
    f64 cache_miss_threshold_{0.3};           // 30% cache miss rate
    f64 cpu_bottleneck_threshold_{0.9};       // 90% CPU usage
    
public:
    /** @brief Analyze performance data for bottlenecks */
    void analyze_performance_data(const std::vector<ecs::ECSBenchmarkResult>& results);
    
    /** @brief Analyze real-time data for bottlenecks */
    void analyze_realtime_data(const RealTimePerformanceData& data);
    
    /** @brief Get identified bottlenecks */
    std::vector<PerformanceBottleneck> get_bottlenecks() const;
    
    /** @brief Get critical bottlenecks only */
    std::vector<PerformanceBottleneck> get_critical_bottlenecks() const;
    
    /** @brief Generate bottleneck visualization graph */
    PerformanceGraph generate_bottleneck_impact_graph() const;
    
    /** @brief Get educational explanation of bottleneck */
    std::string explain_bottleneck(const PerformanceBottleneck& bottleneck) const;
    
    /** @brief Get solution recommendations */
    std::vector<std::string> get_solution_recommendations(
        const PerformanceBottleneck& bottleneck) const;
    
    /** @brief Clear identified bottlenecks */
    void clear_bottlenecks();
    
    /** @brief Set analysis thresholds */
    void set_thresholds(f64 memory_threshold, f64 cache_threshold, f64 cpu_threshold);
};

/**
 * @brief Cache behavior visualization system
 */
class CacheBehaviorVisualizer {
private:
    struct CacheAccessPattern {
        std::string name;
        std::vector<f64> access_times;  // Access times for visualization
        f64 hit_ratio;
        f64 miss_penalty;
    };
    
    std::vector<CacheAccessPattern> access_patterns_;
    mutable std::mutex patterns_mutex_;
    
public:
    /** @brief Add cache access pattern data */
    void add_access_pattern(const std::string& name, 
                          const std::vector<f64>& times,
                          f64 hit_ratio, f64 miss_penalty);
    
    /** @brief Generate cache behavior heatmap */
    PerformanceGraph generate_cache_heatmap() const;
    
    /** @brief Generate cache miss pattern graph */
    PerformanceGraph generate_miss_pattern_graph() const;
    
    /** @brief Get cache optimization suggestions */
    std::vector<std::string> get_cache_optimization_tips() const;
    
    /** @brief Explain cache behavior concepts */
    std::string explain_cache_concepts() const;
};

//=============================================================================
// Main ECS Performance Visualizer
//=============================================================================

/**
 * @brief Comprehensive ECS performance visualization system
 */
class ECSPerformanceVisualizer {
private:
    // Visualization components
    std::unique_ptr<ArchitectureComparisonVisualizer> comparison_visualizer_;
    std::unique_ptr<RealTimePerformanceMonitor> realtime_monitor_;
    std::unique_ptr<BottleneckAnalyzer> bottleneck_analyzer_;
    std::unique_ptr<CacheBehaviorVisualizer> cache_visualizer_;
    
    // Data management
    std::weak_ptr<ecs::ECSPerformanceBenchmarker> benchmarker_;
    std::vector<PerformanceGraph> active_graphs_;
    mutable std::mutex graphs_mutex_;
    
    // Educational content
    std::unordered_map<std::string, std::string> educational_content_;
    std::vector<std::string> current_insights_;
    
    // Configuration
    bool enable_realtime_monitoring_{true};
    bool enable_educational_overlays_{true};
    f64 update_frequency_hz_{10.0};
    
public:
    ECSPerformanceVisualizer();
    ~ECSPerformanceVisualizer();
    
    // System integration
    void set_benchmarker(std::weak_ptr<ecs::ECSPerformanceBenchmarker> benchmarker);
    
    // Visualization management
    void initialize();
    void shutdown();
    void update(f64 delta_time);
    
    // Graph generation
    std::vector<std::string> get_available_graphs() const;
    PerformanceGraph generate_graph(const std::string& graph_type) const;
    void refresh_all_graphs();
    
    // Real-time monitoring
    void start_realtime_monitoring();
    void stop_realtime_monitoring();
    void add_realtime_data(const RealTimePerformanceData& data);
    
    // Educational features
    std::string get_educational_content(const std::string& topic) const;
    std::vector<std::string> get_current_insights() const;
    std::string generate_performance_explanation(const PerformanceGraph& graph) const;
    
    // Bottleneck analysis
    void analyze_for_bottlenecks();
    std::vector<PerformanceBottleneck> get_identified_bottlenecks() const;
    std::string get_bottleneck_explanation(const PerformanceBottleneck& bottleneck) const;
    
    // Optimization recommendations
    struct OptimizationRecommendation {
        std::string title;
        std::string description;
        f64 expected_improvement;
        f64 implementation_effort;
        std::vector<std::string> steps;
    };
    
    std::vector<OptimizationRecommendation> get_optimization_recommendations() const;
    
    // Configuration
    void enable_feature(const std::string& feature, bool enable);
    void set_update_frequency(f64 frequency_hz);
    
    // Export functionality
    void export_graph_data(const std::string& graph_type, const std::string& filename) const;
    void export_performance_report(const std::string& filename) const;
    
    // Interactive features
    struct InteractiveQuery {
        std::string question;
        std::function<std::string()> answer_generator;
    };
    
    void register_interactive_query(const InteractiveQuery& query);
    std::vector<std::string> get_available_queries() const;
    std::string answer_query(const std::string& question) const;
    
private:
    // Implementation helpers
    void initialize_educational_content();
    void update_realtime_visualizations();
    void generate_current_insights();
    PerformanceGraph create_sample_graph() const;
    
    // Color schemes for different architectures
    u32 get_architecture_color(ecs::ECSArchitectureType architecture) const;
    u32 get_test_category_color(ecs::ECSBenchmarkCategory category) const;
};

//=============================================================================
// Educational Content System
//=============================================================================

/**
 * @brief Educational performance content manager
 */
class PerformanceEducationSystem {
private:
    struct EducationalTopic {
        std::string title;
        std::string content;
        std::vector<std::string> key_concepts;
        std::vector<std::string> examples;
        std::string difficulty_level; // "Beginner", "Intermediate", "Advanced"
    };
    
    std::unordered_map<std::string, EducationalTopic> topics_;
    
public:
    PerformanceEducationSystem();
    
    /** @brief Initialize educational content */
    void initialize_content();
    
    /** @brief Get educational content for topic */
    std::string get_content(const std::string& topic) const;
    
    /** @brief Get beginner-friendly explanation */
    std::string get_beginner_explanation(const std::string& topic) const;
    
    /** @brief Get advanced technical details */
    std::string get_advanced_explanation(const std::string& topic) const;
    
    /** @brief Generate contextual explanation based on data */
    std::string generate_contextual_explanation(
        const PerformanceGraph& graph,
        const std::vector<PerformanceBottleneck>& bottlenecks) const;
    
    /** @brief Get learning path recommendations */
    std::vector<std::string> get_learning_path(const std::string& current_topic) const;
};

//=============================================================================
// Utility Functions
//=============================================================================

namespace visualization_utils {
    /** @brief Convert benchmark results to visualization data */
    std::vector<PerformanceDataPoint> results_to_data_points(
        const std::vector<ecs::ECSBenchmarkResult>& results,
        const std::string& x_metric,
        const std::string& y_metric);
    
    /** @brief Generate color palette for multiple series */
    std::vector<u32> generate_color_palette(usize count);
    
    /** @brief Format performance value for display */
    std::string format_performance_value(f64 value, const std::string& units);
    
    /** @brief Calculate trend from data points */
    std::string calculate_trend(const std::vector<PerformanceDataPoint>& points);
    
    /** @brief Generate performance grade (A-F) */
    std::string calculate_performance_grade(f64 performance_score);
    
    /** @brief Create educational tooltip text */
    std::string create_tooltip_text(const PerformanceDataPoint& point,
                                   const std::string& context);
}

} // namespace ecscope::performance::visualization