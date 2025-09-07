#pragma once

/**
 * @file visual_debug_interface.hpp
 * @brief Visual Debugging Interface for ECScope Advanced Profiling
 * 
 * This comprehensive visual debugging system provides:
 * - Real-time performance graphs and charts
 * - Interactive heat maps and performance overlays
 * - Memory visualization and fragmentation analysis
 * - GPU performance visualization
 * - System dependency graphs
 * - Live performance metrics dashboard
 * - Interactive debugging controls
 * 
 * The interface supports both immediate-mode GUI (ImGui) and custom rendering
 * for integration into any graphics pipeline.
 * 
 * @author ECScope Development Team
 * @date 2024
 */

#include "advanced_profiler.hpp"
#include "types.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include <array>
#include <deque>
#include <optional>
#include <chrono>

// Optional ImGui integration
#ifdef ECSCOPE_USE_IMGUI
    #include <imgui.h>
    #include <imgui_internal.h>
    #include <implot.h>
#endif

namespace ecscope::profiling {

//=============================================================================
// Visual Elements and Data Structures
//=============================================================================

// Color representation for visualization
struct Color {
    f32 r, g, b, a;
    
    Color(f32 red = 1.0f, f32 green = 1.0f, f32 blue = 1.0f, f32 alpha = 1.0f)
        : r(red), g(green), b(blue), a(alpha) {}
    
    Color(u32 rgba) {
        r = ((rgba >> 24) & 0xFF) / 255.0f;
        g = ((rgba >> 16) & 0xFF) / 255.0f;
        b = ((rgba >> 8) & 0xFF) / 255.0f;
        a = (rgba & 0xFF) / 255.0f;
    }
    
    u32 to_rgba() const {
        u32 ur = static_cast<u32>(r * 255);
        u32 ug = static_cast<u32>(g * 255);
        u32 ub = static_cast<u32>(b * 255);
        u32 ua = static_cast<u32>(a * 255);
        return (ur << 24) | (ug << 16) | (ub << 8) | ua;
    }
    
    static Color lerp(const Color& a, const Color& b, f32 t) {
        return Color(
            a.r + (b.r - a.r) * t,
            a.g + (b.g - a.g) * t,
            a.b + (b.b - a.b) * t,
            a.a + (b.a - a.a) * t
        );
    }
    
    // Predefined colors
    static const Color RED;
    static const Color GREEN;
    static const Color BLUE;
    static const Color YELLOW;
    static const Color CYAN;
    static const Color MAGENTA;
    static const Color WHITE;
    static const Color BLACK;
    static const Color GRAY;
    static const Color ORANGE;
    static const Color PURPLE;
};

// 2D Vector for UI positioning
struct Vec2f {
    f32 x, y;
    
    Vec2f(f32 x = 0.0f, f32 y = 0.0f) : x(x), y(y) {}
    
    Vec2f operator+(const Vec2f& other) const { return Vec2f(x + other.x, y + other.y); }
    Vec2f operator-(const Vec2f& other) const { return Vec2f(x - other.x, y - other.y); }
    Vec2f operator*(f32 scalar) const { return Vec2f(x * scalar, y * scalar); }
    
    f32 length() const { return std::sqrt(x * x + y * y); }
};

// Rectangle for UI bounds
struct Rectf {
    Vec2f position;
    Vec2f size;
    
    Rectf(f32 x = 0.0f, f32 y = 0.0f, f32 w = 0.0f, f32 h = 0.0f)
        : position(x, y), size(w, h) {}
    
    Rectf(const Vec2f& pos, const Vec2f& sz) : position(pos), size(sz) {}
    
    bool contains(const Vec2f& point) const {
        return point.x >= position.x && point.x <= position.x + size.x &&
               point.y >= position.y && point.y <= position.y + size.y;
    }
    
    Vec2f center() const { return position + size * 0.5f; }
};

// Graph data point
struct GraphPoint {
    f32 x, y;
    Color color;
    std::string label;
    
    GraphPoint(f32 x_val = 0.0f, f32 y_val = 0.0f, const Color& c = Color::WHITE)
        : x(x_val), y(y_val), color(c) {}
};

// Graph configuration
struct GraphConfig {
    std::string title;
    std::string x_label;
    std::string y_label;
    f32 x_min = 0.0f, x_max = 100.0f;
    f32 y_min = 0.0f, y_max = 100.0f;
    bool auto_scale = true;
    bool show_grid = true;
    bool show_legend = true;
    Color background_color = Color(0.1f, 0.1f, 0.1f, 1.0f);
    Color grid_color = Color(0.3f, 0.3f, 0.3f, 0.5f);
    u32 max_points = 1000;
};

// Heat map data
struct HeatmapData {
    std::vector<std::vector<f32>> values;
    u32 width, height;
    f32 min_value, max_value;
    std::vector<std::string> x_labels;
    std::vector<std::string> y_labels;
    Color cold_color = Color::BLUE;
    Color hot_color = Color::RED;
    
    HeatmapData(u32 w, u32 h) : width(w), height(h), min_value(0.0f), max_value(1.0f) {
        values.resize(height, std::vector<f32>(width, 0.0f));
    }
    
    void set_value(u32 x, u32 y, f32 value) {
        if (x < width && y < height) {
            values[y][x] = value;
            min_value = std::min(min_value, value);
            max_value = std::max(max_value, value);
        }
    }
    
    f32 get_value(u32 x, u32 y) const {
        if (x < width && y < height) {
            return values[y][x];
        }
        return 0.0f;
    }
    
    Color get_color(u32 x, u32 y) const {
        f32 value = get_value(x, y);
        f32 t = (value - min_value) / (max_value - min_value);
        return Color::lerp(cold_color, hot_color, t);
    }
};

//=============================================================================
// Chart and Graph Classes
//=============================================================================

// Real-time line graph
class LineGraph {
private:
    GraphConfig config_;
    std::vector<std::deque<GraphPoint>> data_series_;
    std::vector<std::string> series_names_;
    std::vector<Color> series_colors_;
    
public:
    LineGraph(const GraphConfig& config) : config_(config) {}
    
    void add_series(const std::string& name, const Color& color) {
        series_names_.push_back(name);
        series_colors_.push_back(color);
        data_series_.emplace_back();
    }
    
    void add_point(usize series_index, f32 x, f32 y) {
        if (series_index < data_series_.size()) {
            auto& series = data_series_[series_index];
            series.emplace_back(x, y, series_colors_[series_index]);
            
            if (series.size() > config_.max_points) {
                series.pop_front();
            }
            
            if (config_.auto_scale) {
                update_bounds();
            }
        }
    }
    
    void clear_series(usize series_index) {
        if (series_index < data_series_.size()) {
            data_series_[series_index].clear();
        }
    }
    
    void clear_all() {
        for (auto& series : data_series_) {
            series.clear();
        }
    }
    
    void render(const Rectf& bounds);
    
    const GraphConfig& get_config() const { return config_; }
    void set_config(const GraphConfig& config) { config_ = config; }
    
private:
    void update_bounds() {
        if (data_series_.empty()) return;
        
        bool first = true;
        for (const auto& series : data_series_) {
            for (const auto& point : series) {
                if (first) {
                    config_.x_min = config_.x_max = point.x;
                    config_.y_min = config_.y_max = point.y;
                    first = false;
                } else {
                    config_.x_min = std::min(config_.x_min, point.x);
                    config_.x_max = std::max(config_.x_max, point.x);
                    config_.y_min = std::min(config_.y_min, point.y);
                    config_.y_max = std::max(config_.y_max, point.y);
                }
            }
        }
        
        // Add some padding
        f32 x_range = config_.x_max - config_.x_min;
        f32 y_range = config_.y_max - config_.y_min;
        config_.x_min -= x_range * 0.05f;
        config_.x_max += x_range * 0.05f;
        config_.y_min -= y_range * 0.05f;
        config_.y_max += y_range * 0.05f;
    }
};

// Bar chart for discrete data
class BarChart {
private:
    struct Bar {
        std::string label;
        f32 value;
        Color color;
        
        Bar(const std::string& l, f32 v, const Color& c) : label(l), value(v), color(c) {}
    };
    
    std::vector<Bar> bars_;
    std::string title_;
    f32 max_value_;
    bool auto_scale_;
    
public:
    BarChart(const std::string& title, bool auto_scale = true) 
        : title_(title), max_value_(100.0f), auto_scale_(auto_scale) {}
    
    void clear() { bars_.clear(); }
    
    void add_bar(const std::string& label, f32 value, const Color& color = Color::BLUE) {
        bars_.emplace_back(label, value, color);
        
        if (auto_scale_) {
            max_value_ = std::max(max_value_, value);
        }
    }
    
    void render(const Rectf& bounds);
    
    void set_title(const std::string& title) { title_ = title; }
    void set_max_value(f32 max_val) { max_value_ = max_val; auto_scale_ = false; }
};

// Pie chart for proportional data
class PieChart {
private:
    struct Slice {
        std::string label;
        f32 value;
        Color color;
        f32 start_angle;
        f32 end_angle;
        
        Slice(const std::string& l, f32 v, const Color& c) : label(l), value(v), color(c) {}
    };
    
    std::vector<Slice> slices_;
    std::string title_;
    f32 total_value_;
    
public:
    PieChart(const std::string& title) : title_(title), total_value_(0.0f) {}
    
    void clear() { slices_.clear(); total_value_ = 0.0f; }
    
    void add_slice(const std::string& label, f32 value, const Color& color) {
        slices_.emplace_back(label, value, color);
        total_value_ += value;
        update_angles();
    }
    
    void render(const Rectf& bounds);
    
private:
    void update_angles() {
        f32 current_angle = 0.0f;
        for (auto& slice : slices_) {
            slice.start_angle = current_angle;
            f32 angle_span = (slice.value / total_value_) * 360.0f;
            current_angle += angle_span;
            slice.end_angle = current_angle;
        }
    }
};

// Performance heat map
class PerformanceHeatmap {
private:
    HeatmapData data_;
    std::string title_;
    f32 cell_size_;
    bool show_values_;
    
public:
    PerformanceHeatmap(const std::string& title, u32 width, u32 height)
        : data_(width, height), title_(title), cell_size_(1.0f), show_values_(false) {}
    
    void set_value(u32 x, u32 y, f32 value) { data_.set_value(x, y, value); }
    void set_cell_size(f32 size) { cell_size_ = size; }
    void show_values(bool show) { show_values_ = show; }
    
    void set_x_labels(const std::vector<std::string>& labels) { data_.x_labels = labels; }
    void set_y_labels(const std::vector<std::string>& labels) { data_.y_labels = labels; }
    
    void render(const Rectf& bounds);
    
    HeatmapData& get_data() { return data_; }
    const HeatmapData& get_data() const { return data_; }
};

//=============================================================================
// Dashboard Components
//=============================================================================

// Performance metrics widget
class PerformanceWidget {
private:
    std::string title_;
    f32 current_value_;
    f32 target_value_;
    f32 warning_threshold_;
    f32 critical_threshold_;
    std::string unit_;
    std::deque<f32> history_;
    static constexpr usize MAX_HISTORY = 100;
    
public:
    PerformanceWidget(const std::string& title, const std::string& unit = "")
        : title_(title), current_value_(0.0f), target_value_(0.0f), 
          warning_threshold_(0.0f), critical_threshold_(0.0f), unit_(unit) {}
    
    void update(f32 value) {
        current_value_ = value;
        history_.push_back(value);
        
        if (history_.size() > MAX_HISTORY) {
            history_.pop_front();
        }
    }
    
    void set_thresholds(f32 warning, f32 critical) {
        warning_threshold_ = warning;
        critical_threshold_ = critical;
    }
    
    void set_target(f32 target) { target_value_ = target; }
    
    void render(const Rectf& bounds);
    
    Color get_status_color() const {
        if (critical_threshold_ > 0 && current_value_ >= critical_threshold_) {
            return Color::RED;
        } else if (warning_threshold_ > 0 && current_value_ >= warning_threshold_) {
            return Color::YELLOW;
        }
        return Color::GREEN;
    }
};

// System status indicator
class SystemStatusIndicator {
public:
    enum class Status {
        EXCELLENT,
        GOOD,
        WARNING,
        CRITICAL,
        ERROR
    };
    
private:
    std::string system_name_;
    Status status_;
    std::string message_;
    f64 score_;
    high_resolution_clock::time_point last_update_;
    
public:
    SystemStatusIndicator(const std::string& name) 
        : system_name_(name), status_(Status::GOOD), score_(100.0), 
          last_update_(high_resolution_clock::now()) {}
    
    void update(Status status, f64 score, const std::string& message = "") {
        status_ = status;
        score_ = score;
        message_ = message;
        last_update_ = high_resolution_clock::now();
    }
    
    void render(const Rectf& bounds);
    
    Status get_status() const { return status_; }
    f64 get_score() const { return score_; }
    Color get_status_color() const;
    
private:
    std::string get_status_text() const;
};

// Memory visualization widget
class MemoryVisualizationWidget {
private:
    struct MemoryBlock {
        usize offset;
        usize size;
        std::string category;
        Color color;
        bool is_free;
        
        MemoryBlock(usize off, usize sz, const std::string& cat, const Color& col, bool free = false)
            : offset(off), size(sz), category(cat), color(col), is_free(free) {}
    };
    
    std::vector<MemoryBlock> blocks_;
    usize total_memory_;
    usize used_memory_;
    bool show_details_;
    
public:
    MemoryVisualizationWidget() : total_memory_(0), used_memory_(0), show_details_(false) {}
    
    void clear() { blocks_.clear(); total_memory_ = 0; used_memory_ = 0; }
    
    void add_block(usize offset, usize size, const std::string& category, const Color& color, bool is_free = false) {
        blocks_.emplace_back(offset, size, category, color, is_free);
        total_memory_ = std::max(total_memory_, offset + size);
        if (!is_free) {
            used_memory_ += size;
        }
    }
    
    void show_details(bool show) { show_details_ = show; }
    
    void render(const Rectf& bounds);
    
    f32 get_utilization() const {
        return total_memory_ > 0 ? static_cast<f32>(used_memory_) / total_memory_ : 0.0f;
    }
};

//=============================================================================
// Main Visual Debug Interface
//=============================================================================

struct VisualConfig {
    bool show_fps_graph = true;
    bool show_memory_graph = true;
    bool show_gpu_metrics = true;
    bool show_system_metrics = true;
    bool show_heat_maps = true;
    bool show_performance_overlay = true;
    bool show_debug_console = false;
    
    f32 update_frequency = 60.0f; // Hz
    f32 graph_history_seconds = 30.0f;
    u32 max_systems_displayed = 20;
    
    Color theme_primary = Color(0.2f, 0.6f, 1.0f, 1.0f);
    Color theme_secondary = Color(0.1f, 0.1f, 0.1f, 0.9f);
    Color theme_accent = Color(1.0f, 0.6f, 0.2f, 1.0f);
    Color theme_text = Color(0.9f, 0.9f, 0.9f, 1.0f);
};

class VisualDebugInterface {
private:
    AdvancedProfiler* profiler_;
    VisualConfig config_;
    
    // UI state
    bool enabled_ = true;
    bool show_main_window_ = true;
    bool show_detailed_metrics_ = false;
    bool show_memory_analyzer_ = false;
    bool show_gpu_profiler_ = false;
    bool show_trend_analysis_ = false;
    
    // Timing
    high_resolution_clock::time_point last_update_;
    f32 update_timer_ = 0.0f;
    
    // Graphs and charts
    std::unique_ptr<LineGraph> fps_graph_;
    std::unique_ptr<LineGraph> memory_graph_;
    std::unique_ptr<LineGraph> gpu_utilization_graph_;
    std::unique_ptr<BarChart> system_performance_chart_;
    std::unique_ptr<PieChart> memory_category_chart_;
    std::unique_ptr<PerformanceHeatmap> system_heatmap_;
    
    // Widgets
    std::vector<std::unique_ptr<PerformanceWidget>> performance_widgets_;
    std::vector<std::unique_ptr<SystemStatusIndicator>> status_indicators_;
    std::unique_ptr<MemoryVisualizationWidget> memory_widget_;
    
    // Cached data
    std::vector<AdvancedSystemMetrics> cached_system_metrics_;
    GPUMetrics cached_gpu_metrics_;
    AdvancedMemoryMetrics cached_memory_metrics_;
    
    // Interaction state
    std::string selected_system_;
    bool mouse_captured_ = false;
    Vec2f mouse_position_;
    
public:
    VisualDebugInterface(AdvancedProfiler* profiler);
    ~VisualDebugInterface();
    
    void initialize();
    void shutdown();
    void update(f32 delta_time);
    void render();
    
    // Configuration
    void set_config(const VisualConfig& config) { config_ = config; }
    const VisualConfig& get_config() const { return config_; }
    
    void set_enabled(bool enabled) { enabled_ = enabled; }
    bool is_enabled() const { return enabled_; }
    
    // Window visibility
    void show_main_window(bool show) { show_main_window_ = show; }
    void show_detailed_metrics(bool show) { show_detailed_metrics_ = show; }
    void show_memory_analyzer(bool show) { show_memory_analyzer_ = show; }
    void show_gpu_profiler(bool show) { show_gpu_profiler_ = show; }
    void show_trend_analysis(bool show) { show_trend_analysis_ = show; }
    
    // Event handling
    void handle_mouse_move(f32 x, f32 y);
    void handle_mouse_click(f32 x, f32 y, bool pressed);
    void handle_key_press(int key);
    
private:
    void update_data();
    void update_graphs();
    void update_widgets();
    void update_charts();
    
    // Rendering methods
    void render_main_window();
    void render_detailed_metrics_window();
    void render_memory_analyzer_window();
    void render_gpu_profiler_window();
    void render_trend_analysis_window();
    void render_performance_overlay();
    
    void render_system_list();
    void render_system_details(const std::string& system_name);
    void render_anomaly_alerts();
    void render_recommendations();
    
    // Helper methods
    void setup_graphs();
    void setup_widgets();
    void setup_charts();
    
    Color get_performance_color(f64 score) const;
    std::string format_bytes(usize bytes) const;
    std::string format_time(high_resolution_clock::duration duration) const;
    std::string format_percentage(f32 percentage) const;
    
    // ImGui integration (if available)
    #ifdef ECSCOPE_USE_IMGUI
    void render_imgui_main_window();
    void render_imgui_system_metrics();
    void render_imgui_memory_analyzer();
    void render_imgui_gpu_profiler();
    void render_imgui_trend_analysis();
    void render_imgui_graphs();
    void render_imgui_performance_table();
    #endif
};

//=============================================================================
// Educational Debugging Tools
//=============================================================================

// Interactive tutorial system for performance analysis
class PerformanceTutorial {
public:
    struct TutorialStep {
        std::string title;
        std::string description;
        std::string code_example;
        std::vector<std::string> key_points;
        std::function<bool()> completion_check;
        std::function<void()> highlight_elements;
    };
    
private:
    std::vector<TutorialStep> steps_;
    usize current_step_ = 0;
    bool active_ = false;
    AdvancedProfiler* profiler_;
    
public:
    PerformanceTutorial(AdvancedProfiler* profiler) : profiler_(profiler) {
        initialize_tutorials();
    }
    
    void start_tutorial(const std::string& tutorial_name);
    void next_step();
    void previous_step();
    void complete_tutorial();
    bool is_active() const { return active_; }
    
    void render();
    
private:
    void initialize_tutorials();
    void create_basic_profiling_tutorial();
    void create_memory_optimization_tutorial();
    void create_gpu_profiling_tutorial();
    void create_performance_regression_tutorial();
};

// Interactive performance analysis guide
class PerformanceAnalysisGuide {
private:
    struct AnalysisPattern {
        std::string name;
        std::string description;
        std::vector<std::string> symptoms;
        std::vector<std::string> causes;
        std::vector<std::string> solutions;
        std::function<bool(const AdvancedSystemMetrics&)> detector;
    };
    
    std::vector<AnalysisPattern> patterns_;
    AdvancedProfiler* profiler_;
    
public:
    PerformanceAnalysisGuide(AdvancedProfiler* profiler);
    
    std::vector<std::string> analyze_system(const std::string& system_name);
    void render_analysis_results(const std::string& system_name);
    void render_pattern_guide();
    
private:
    void initialize_patterns();
};

} // namespace ecscope::profiling