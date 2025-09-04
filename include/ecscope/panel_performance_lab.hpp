#pragma once

/**
 * @file panel_performance_lab.hpp
 * @brief Performance Laboratory UI Panel - Real-time visualization and control
 * 
 * This UI panel provides comprehensive visualization and control for ECScope's
 * Performance Laboratory, enabling real-time monitoring, interactive experiments,
 * and educational insights display.
 * 
 * Features:
 * - Real-time performance graphs and metrics
 * - Interactive experiment controls
 * - Memory access pattern visualization
 * - Allocation strategy comparison charts
 * - Educational insights and recommendations display
 * - Live performance health monitoring
 * - Export and reporting capabilities
 * 
 * Educational Interface:
 * - Color-coded performance indicators
 * - Explanatory tooltips and descriptions
 * - Progressive disclosure of advanced metrics
 * - Interactive tutorial mode
 * - Contextual help and documentation
 * 
 * @author ECScope Educational ECS Framework
 * @date 2024
 */

#include "../overlay.hpp"
#include "performance/performance_lab.hpp"
#include "performance/memory_experiments.hpp"
#include "performance/allocation_benchmarks.hpp"
#include "core/types.hpp"
#include <vector>
#include <memory>
#include <string>
#include <array>
#include <unordered_map>

namespace ecscope::ui {

/**
 * @brief Performance Laboratory UI Panel
 */
class PerformanceLabPanel : public Panel {
private:
    // Core performance lab reference
    std::shared_ptr<performance::PerformanceLab> performance_lab_;
    
    // UI State
    enum class DisplayMode : u8 {
        Overview,           // General performance overview
        MemoryExperiments,  // Memory access pattern experiments
        AllocationBench,    // Allocation strategy benchmarks
        RealTimeMonitor,    // Live performance monitoring
        Recommendations,    // Performance recommendations
        Educational         // Educational content and tutorials
    };
    
    DisplayMode current_mode_;
    bool is_monitoring_;
    bool show_advanced_metrics_;
    bool tutorial_mode_enabled_;
    
    // Performance data visualization
    static constexpr usize GRAPH_HISTORY_SIZE = 300; // 5 minutes at 60fps
    
    struct PerformanceGraphData {
        std::array<f32, GRAPH_HISTORY_SIZE> memory_usage_;
        std::array<f32, GRAPH_HISTORY_SIZE> allocation_rate_;
        std::array<f32, GRAPH_HISTORY_SIZE> frame_times_;
        std::array<f32, GRAPH_HISTORY_SIZE> cache_efficiency_;
        usize data_head_;
        usize data_count_;
        
        PerformanceGraphData() : data_head_(0), data_count_(0) {
            memory_usage_.fill(0.0f);
            allocation_rate_.fill(0.0f);
            frame_times_.fill(16.67f); // 60 FPS default
            cache_efficiency_.fill(0.85f);
        }
        
        void add_sample(f32 memory, f32 alloc_rate, f32 frame_time, f32 cache_eff) {
            memory_usage_[data_head_] = memory;
            allocation_rate_[data_head_] = alloc_rate;
            frame_times_[data_head_] = frame_time;
            cache_efficiency_[data_head_] = cache_eff;
            
            data_head_ = (data_head_ + 1) % GRAPH_HISTORY_SIZE;
            data_count_ = std::min(data_count_ + 1, GRAPH_HISTORY_SIZE);
        }
        
        void clear() {
            data_head_ = 0;
            data_count_ = 0;
            memory_usage_.fill(0.0f);
            allocation_rate_.fill(0.0f);
            frame_times_.fill(16.67f);
            cache_efficiency_.fill(0.85f);
        }
    };
    
    PerformanceGraphData graph_data_;
    f64 last_graph_update_time_;
    f32 graph_update_frequency_;
    
    // Experiment control state
    struct ExperimentState {
        std::string current_experiment_;
        bool is_running_;
        f32 progress_;
        std::string status_message_;
        std::vector<std::string> available_experiments_;
        std::unordered_map<std::string, performance::BenchmarkResult> cached_results_;
        
        ExperimentState() : is_running_(false), progress_(0.0f) {}
    };
    
    ExperimentState experiment_state_;
    
    // Memory experiment visualization
    struct MemoryExperimentViz {
        bool show_soa_vs_aos_;
        bool show_cache_analysis_;
        bool show_fragmentation_;
        bool show_access_patterns_;
        f32 soa_performance_;
        f32 aos_performance_;
        f32 cache_hit_ratio_;
        std::vector<std::pair<std::string, f32>> allocator_comparison_;
        
        MemoryExperimentViz() 
            : show_soa_vs_aos_(true), show_cache_analysis_(true), 
              show_fragmentation_(false), show_access_patterns_(false),
              soa_performance_(0.85f), aos_performance_(0.65f), cache_hit_ratio_(0.92f) {}
    };
    
    MemoryExperimentViz memory_viz_;
    
    // Allocation benchmark visualization
    struct AllocationBenchViz {
        std::array<f32, 4> allocator_speeds_; // Arena, Pool, PMR, Standard
        std::array<f32, 4> allocator_efficiency_;
        std::array<f32, 4> allocator_fragmentation_;
        std::vector<std::string> allocator_names_;
        int selected_allocator_;
        
        AllocationBenchViz() : selected_allocator_(0) {
            allocator_speeds_.fill(0.5f);
            allocator_efficiency_.fill(0.7f);
            allocator_fragmentation_.fill(0.3f);
            allocator_names_ = {"Arena", "Pool", "PMR", "Standard"};
        }
    };
    
    AllocationBenchViz allocation_viz_;
    
    // Recommendation system
    struct RecommendationDisplay {
        std::vector<performance::PerformanceRecommendation> current_recommendations_;
        int selected_recommendation_;
        bool show_implementation_details_;
        bool auto_update_recommendations_;
        f64 last_recommendation_update_;
        
        RecommendationDisplay() 
            : selected_recommendation_(-1), show_implementation_details_(false),
              auto_update_recommendations_(true), last_recommendation_update_(0.0) {}
    };
    
    RecommendationDisplay recommendation_display_;
    
    // Educational content system
    struct EducationalContent {
        std::unordered_map<std::string, std::string> explanations_;
        std::vector<std::string> tutorial_steps_;
        int current_tutorial_step_;
        bool show_tooltips_;
        bool show_explanations_;
        
        EducationalContent() 
            : current_tutorial_step_(0), show_tooltips_(true), show_explanations_(true) {}
    };
    
    EducationalContent educational_content_;
    
    // Color scheme for performance visualization
    struct Colors {
        static constexpr f32 excellent_[4] = {0.2f, 0.8f, 0.2f, 1.0f};  // Green
        static constexpr f32 good_[4] = {0.6f, 0.8f, 0.2f, 1.0f};       // Yellow-Green
        static constexpr f32 fair_[4] = {1.0f, 0.8f, 0.2f, 1.0f};       // Orange
        static constexpr f32 poor_[4] = {1.0f, 0.3f, 0.3f, 1.0f};       // Red
        static constexpr f32 neutral_[4] = {0.7f, 0.7f, 0.7f, 1.0f};    // Gray
        
        static const f32* get_performance_color(f32 score) {
            if (score >= 0.9f) return excellent_;
            else if (score >= 0.7f) return good_;
            else if (score >= 0.5f) return fair_;
            else return poor_;
        }
    };
    
    // UI Helper functions
    void update_performance_data();
    void update_experiment_state();
    void update_recommendations();
    
    // Rendering functions
    void render_overview_mode();
    void render_memory_experiments_mode();
    void render_allocation_bench_mode();
    void render_realtime_monitor_mode();
    void render_recommendations_mode();
    void render_educational_mode();
    
    // Specialized UI components
    void render_performance_graph(const char* label, const f32* data, usize count, f32 scale_min, f32 scale_max, const f32* color);
    void render_allocator_comparison_chart();
    void render_memory_layout_visualization();
    void render_cache_behavior_diagram();
    void render_recommendation_card(const performance::PerformanceRecommendation& rec);
    void render_experiment_controls();
    void render_educational_tooltip(const char* text);
    
    // Utility functions
    f32 normalize_value(f32 value, f32 min_val, f32 max_val);
    std::string format_performance_value(f32 value, const char* unit);
    std::string get_performance_description(f32 score);
    void show_help_popup(const char* title, const char* content);
    
    // Event handlers
    void on_experiment_started(const std::string& name);
    void on_experiment_completed(const performance::BenchmarkResult& result);
    void on_monitoring_toggled(bool enabled);
    
public:
    explicit PerformanceLabPanel(std::shared_ptr<performance::PerformanceLab> lab);
    ~PerformanceLabPanel() override = default;
    
    // Panel interface
    void render() override;
    void update(f64 dt) override;
    bool wants_keyboard_capture() const override { return false; }
    bool wants_mouse_capture() const override { return false; }
    
    // Configuration
    void set_tutorial_mode(bool enabled) { tutorial_mode_enabled_ = enabled; }
    void set_advanced_metrics(bool enabled) { show_advanced_metrics_ = enabled; }
    void set_graph_update_frequency(f32 frequency) { graph_update_frequency_ = frequency; }
    
    // Experiment control
    void start_experiment(const std::string& name);
    void stop_current_experiment();
    void start_monitoring();
    void stop_monitoring();
    
    // Educational features
    void show_explanation(const std::string& topic);
    void start_tutorial();
    void next_tutorial_step();
    void previous_tutorial_step();
    
    // Data export
    void export_performance_data();
    void export_current_results();
    
private:
    // UI constants
    static constexpr f32 PANEL_MIN_WIDTH = 400.0f;
    static constexpr f32 PANEL_MIN_HEIGHT = 300.0f;
    static constexpr f32 GRAPH_HEIGHT = 100.0f;
    static constexpr f32 RECOMMENDATION_CARD_HEIGHT = 120.0f;
    static constexpr f32 TOOLTIP_DELAY = 0.5f; // seconds
    
    // Update frequencies
    static constexpr f64 DATA_UPDATE_FREQUENCY = 0.1; // 10 Hz
    static constexpr f64 RECOMMENDATION_UPDATE_FREQUENCY = 1.0; // 1 Hz
    
    // Tutorial steps
    void initialize_tutorial_content();
    void initialize_educational_explanations();
};

/**
 * @brief Specialized memory visualization widget
 */
class MemoryVisualizationWidget {
private:
    enum class VisualizationType : u8 {
        MemoryLayout,       // Show SoA vs AoS layout
        CacheLines,         // Cache line utilization
        Fragmentation,      // Memory fragmentation
        AccessPatterns      // Memory access patterns
    };
    
    VisualizationType current_type_;
    f32 widget_width_;
    f32 widget_height_;
    
public:
    explicit MemoryVisualizationWidget(f32 width = 300.0f, f32 height = 200.0f);
    
    void render_memory_layout(f32 soa_efficiency, f32 aos_efficiency);
    void render_cache_lines(f32 utilization, f32 miss_rate);
    void render_fragmentation(f32 fragmentation_ratio, const std::vector<usize>& free_blocks);
    void render_access_patterns(const std::vector<f32>& access_times);
    
    void set_type(VisualizationType type) { current_type_ = type; }
    void set_size(f32 width, f32 height) { widget_width_ = width; widget_height_ = height; }
};

/**
 * @brief Performance metrics dashboard widget
 */
class PerformanceMetricsDashboard {
private:
    struct MetricDisplay {
        std::string name;
        f32 current_value;
        f32 target_value;
        f32 min_value;
        f32 max_value;
        std::string unit;
        const f32* color;
        bool show_target;
        
        MetricDisplay() : current_value(0.0f), target_value(1.0f), min_value(0.0f), 
                         max_value(1.0f), color(PerformanceLabPanel::Colors::neutral_), show_target(false) {}
    };
    
    std::vector<MetricDisplay> metrics_;
    f32 dashboard_width_;
    f32 dashboard_height_;
    
public:
    explicit PerformanceMetricsDashboard(f32 width = 400.0f, f32 height = 200.0f);
    
    void add_metric(const std::string& name, f32 value, f32 min_val, f32 max_val, const std::string& unit);
    void update_metric(const std::string& name, f32 value);
    void set_metric_target(const std::string& name, f32 target);
    void render();
    
    void clear_metrics() { metrics_.clear(); }
    void set_size(f32 width, f32 height) { dashboard_width_ = width; dashboard_height_ = height; }
};

} // namespace ecscope::ui