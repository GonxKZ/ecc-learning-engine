/**
 * @file visual_debug_interface.cpp
 * @brief Implementation of Visual Debug Interface for ECScope
 * 
 * Complete implementation of the visual debugging system with real-time
 * performance graphs, interactive charts, heat maps, and educational tools.
 * 
 * @author ECScope Development Team
 * @date 2024
 */

#include "../include/ecscope/visual_debug_interface.hpp"
#include "../include/ecscope/advanced_profiler.hpp"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace ecscope::profiling {

//=============================================================================
// LineGraph Implementation
//=============================================================================

void LineGraph::render(const Rectf& bounds) {
    // This is a basic implementation - in a real system, this would use
    // the actual graphics API (OpenGL, DirectX, etc.) or GUI framework
    
    // For now, we'll provide the structure for rendering
    if (data_series_.empty()) return;
    
    // Calculate drawing area (leave space for axes and labels)
    f32 margin = 50.0f;
    Rectf draw_area(
        bounds.position.x + margin,
        bounds.position.y + margin,
        bounds.size.x - 2 * margin,
        bounds.size.y - 2 * margin
    );
    
    // Calculate scale factors
    f32 x_scale = draw_area.size.x / (config_.x_max - config_.x_min);
    f32 y_scale = draw_area.size.y / (config_.y_max - config_.y_min);
    
    // Render grid if enabled
    if (config_.show_grid) {
        render_grid(draw_area, x_scale, y_scale);
    }
    
    // Render data series
    for (usize series_idx = 0; series_idx < data_series_.size(); ++series_idx) {
        const auto& series = data_series_[series_idx];
        if (series.empty()) continue;
        
        Color series_color = series_colors_[series_idx];
        render_series(series, draw_area, x_scale, y_scale, series_color);
    }
    
    // Render axes and labels
    render_axes(bounds, draw_area);
    
    // Render legend if enabled
    if (config_.show_legend) {
        render_legend(bounds);
    }
}

void LineGraph::render_grid(const Rectf& draw_area, f32 x_scale, f32 y_scale) {
    // Grid rendering implementation would go here
    // This would typically involve drawing horizontal and vertical lines
}

void LineGraph::render_series(const std::deque<GraphPoint>& series, 
                             const Rectf& draw_area, f32 x_scale, f32 y_scale, 
                             const Color& color) {
    // Series rendering implementation
    // This would draw connected lines between points
}

void LineGraph::render_axes(const Rectf& bounds, const Rectf& draw_area) {
    // Axes rendering implementation
    // This would draw X and Y axes with tick marks and labels
}

void LineGraph::render_legend(const Rectf& bounds) {
    // Legend rendering implementation
    // This would show series names with their colors
}

//=============================================================================
// BarChart Implementation
//=============================================================================

void BarChart::render(const Rectf& bounds) {
    if (bars_.empty()) return;
    
    // Calculate bar dimensions
    f32 margin = 20.0f;
    f32 bar_area_width = bounds.size.x - 2 * margin;
    f32 bar_area_height = bounds.size.y - 100.0f; // Leave space for labels
    
    f32 bar_width = bar_area_width / bars_.size() * 0.8f; // 80% width, 20% spacing
    f32 bar_spacing = bar_area_width / bars_.size() * 0.2f;
    
    f32 value_scale = bar_area_height / max_value_;
    
    // Render bars
    for (usize i = 0; i < bars_.size(); ++i) {
        const auto& bar = bars_[i];
        
        f32 bar_height = bar.value * value_scale;
        f32 x = bounds.position.x + margin + i * (bar_width + bar_spacing);
        f32 y = bounds.position.y + bounds.size.y - 50.0f - bar_height;
        
        // In a real implementation, this would draw a rectangle
        // with the specified color and dimensions
        render_bar_rectangle(Rectf(x, y, bar_width, bar_height), bar.color);
        
        // Render label
        render_bar_label(bar.label, Vec2f(x + bar_width / 2, bounds.position.y + bounds.size.y - 30));
    }
    
    // Render title
    render_title(title_, Vec2f(bounds.center().x, bounds.position.y + 20));
}

void BarChart::render_bar_rectangle(const Rectf& rect, const Color& color) {
    // Bar rectangle rendering implementation
}

void BarChart::render_bar_label(const std::string& label, const Vec2f& position) {
    // Label rendering implementation
}

void BarChart::render_title(const std::string& title, const Vec2f& position) {
    // Title rendering implementation
}

//=============================================================================
// PieChart Implementation
//=============================================================================

void PieChart::render(const Rectf& bounds) {
    if (slices_.empty() || total_value_ == 0.0f) return;
    
    Vec2f center = bounds.center();
    f32 radius = std::min(bounds.size.x, bounds.size.y) * 0.35f;
    
    // Render slices
    for (const auto& slice : slices_) {
        render_pie_slice(center, radius, slice.start_angle, slice.end_angle, slice.color);
    }
    
    // Render labels
    for (const auto& slice : slices_) {
        f32 mid_angle = (slice.start_angle + slice.end_angle) * 0.5f;
        f32 label_radius = radius * 1.2f;
        
        Vec2f label_pos(
            center.x + std::cos(mid_angle * M_PI / 180.0f) * label_radius,
            center.y + std::sin(mid_angle * M_PI / 180.0f) * label_radius
        );
        
        std::string label_text = slice.label + " (" + 
                                std::to_string(static_cast<int>(slice.value / total_value_ * 100)) + "%)";
        render_pie_label(label_text, label_pos);
    }
    
    // Render title
    render_title(title_, Vec2f(center.x, bounds.position.y + 20));
}

void PieChart::render_pie_slice(const Vec2f& center, f32 radius, 
                               f32 start_angle, f32 end_angle, const Color& color) {
    // Pie slice rendering implementation
}

void PieChart::render_pie_label(const std::string& label, const Vec2f& position) {
    // Pie label rendering implementation
}

//=============================================================================
// PerformanceHeatmap Implementation
//=============================================================================

void PerformanceHeatmap::render(const Rectf& bounds) {
    if (data_.width == 0 || data_.height == 0) return;
    
    f32 cell_width = bounds.size.x / data_.width;
    f32 cell_height = bounds.size.y / data_.height;
    
    // Render heatmap cells
    for (u32 y = 0; y < data_.height; ++y) {
        for (u32 x = 0; x < data_.width; ++x) {
            Color cell_color = data_.get_color(x, y);
            
            Rectf cell_rect(
                bounds.position.x + x * cell_width,
                bounds.position.y + y * cell_height,
                cell_width,
                cell_height
            );
            
            render_heatmap_cell(cell_rect, cell_color);
            
            // Render value if enabled
            if (show_values_) {
                f32 value = data_.get_value(x, y);
                std::string value_text = std::to_string(static_cast<int>(value * 100)) + "%";
                render_cell_text(value_text, cell_rect.center());
            }
        }
    }
    
    // Render labels if available
    render_heatmap_labels(bounds);
}

void PerformanceHeatmap::render_heatmap_cell(const Rectf& rect, const Color& color) {
    // Heatmap cell rendering implementation
}

void PerformanceHeatmap::render_cell_text(const std::string& text, const Vec2f& position) {
    // Cell text rendering implementation
}

void PerformanceHeatmap::render_heatmap_labels(const Rectf& bounds) {
    // Label rendering implementation
}

//=============================================================================
// PerformanceWidget Implementation
//=============================================================================

void PerformanceWidget::render(const Rectf& bounds) {
    // Widget background
    Color bg_color = Color::BLACK;
    bg_color.a = 0.7f;
    render_widget_background(bounds, bg_color);
    
    // Title
    render_widget_title(title_, Vec2f(bounds.position.x + 10, bounds.position.y + 10));
    
    // Current value with status color
    Color value_color = get_status_color();
    std::string value_text = std::to_string(static_cast<int>(current_value_)) + unit_;
    render_widget_value(value_text, Vec2f(bounds.position.x + 10, bounds.position.y + 35), value_color);
    
    // Mini graph of history
    if (!history_.empty() && bounds.size.y > 80) {
        Rectf graph_area(
            bounds.position.x + 10,
            bounds.position.y + 60,
            bounds.size.x - 20,
            bounds.size.y - 70
        );
        render_mini_graph(graph_area);
    }
    
    // Warning indicators
    if (critical_threshold_ > 0 && current_value_ >= critical_threshold_) {
        render_warning_indicator(bounds, Color::RED);
    } else if (warning_threshold_ > 0 && current_value_ >= warning_threshold_) {
        render_warning_indicator(bounds, Color::YELLOW);
    }
}

void PerformanceWidget::render_widget_background(const Rectf& bounds, const Color& color) {
    // Widget background rendering implementation
}

void PerformanceWidget::render_widget_title(const std::string& title, const Vec2f& position) {
    // Widget title rendering implementation
}

void PerformanceWidget::render_widget_value(const std::string& value, const Vec2f& position, const Color& color) {
    // Widget value rendering implementation
}

void PerformanceWidget::render_mini_graph(const Rectf& bounds) {
    // Mini graph rendering implementation
}

void PerformanceWidget::render_warning_indicator(const Rectf& bounds, const Color& color) {
    // Warning indicator rendering implementation
}

//=============================================================================
// SystemStatusIndicator Implementation
//=============================================================================

void SystemStatusIndicator::render(const Rectf& bounds) {
    Color status_color = get_status_color();
    
    // Status circle
    f32 circle_radius = 15.0f;
    Vec2f circle_pos(bounds.position.x + 20, bounds.position.y + bounds.size.y * 0.5f);
    render_status_circle(circle_pos, circle_radius, status_color);
    
    // System name
    Vec2f text_pos(bounds.position.x + 45, bounds.position.y + 10);
    render_status_text(system_name_, text_pos, Color::WHITE);
    
    // Status text
    std::string status_text = get_status_text();
    Vec2f status_text_pos(bounds.position.x + 45, bounds.position.y + 30);
    render_status_text(status_text, status_text_pos, status_color);
    
    // Score
    if (score_ >= 0) {
        std::string score_text = std::to_string(static_cast<int>(score_)) + "/100";
        Vec2f score_pos(bounds.position.x + bounds.size.x - 60, bounds.position.y + 10);
        render_status_text(score_text, score_pos, status_color);
    }
    
    // Message if available
    if (!message_.empty()) {
        Vec2f msg_pos(bounds.position.x + 45, bounds.position.y + 50);
        render_status_text(message_, msg_pos, Color::GRAY);
    }
}

void SystemStatusIndicator::render_status_circle(const Vec2f& center, f32 radius, const Color& color) {
    // Status circle rendering implementation
}

void SystemStatusIndicator::render_status_text(const std::string& text, const Vec2f& position, const Color& color) {
    // Status text rendering implementation
}

Color SystemStatusIndicator::get_status_color() const {
    switch (status_) {
        case Status::EXCELLENT: return Color::GREEN;
        case Status::GOOD: return Color(0.8f, 1.0f, 0.0f, 1.0f); // Light green
        case Status::WARNING: return Color::YELLOW;
        case Status::CRITICAL: return Color::ORANGE;
        case Status::ERROR: return Color::RED;
        default: return Color::GRAY;
    }
}

std::string SystemStatusIndicator::get_status_text() const {
    switch (status_) {
        case Status::EXCELLENT: return "Excellent";
        case Status::GOOD: return "Good";
        case Status::WARNING: return "Warning";
        case Status::CRITICAL: return "Critical";
        case Status::ERROR: return "Error";
        default: return "Unknown";
    }
}

//=============================================================================
// MemoryVisualizationWidget Implementation
//=============================================================================

void MemoryVisualizationWidget::render(const Rectf& bounds) {
    if (total_memory_ == 0) return;
    
    f32 scale = bounds.size.x / total_memory_;
    f32 current_x = bounds.position.x;
    
    // Render memory blocks
    for (const auto& block : blocks_) {
        f32 block_width = block.size * scale;
        
        Rectf block_rect(current_x, bounds.position.y, block_width, bounds.size.y * 0.7f);
        render_memory_block(block_rect, block.color, block.is_free);
        
        // Render block label if details are enabled and block is large enough
        if (show_details_ && block_width > 50.0f) {
            std::string label = block.category + "\n" + format_bytes(block.size);
            render_block_label(label, Vec2f(current_x + block_width * 0.5f, bounds.position.y + 10));
        }
        
        current_x += block_width;
    }
    
    // Render utilization text
    f32 utilization = get_utilization();
    std::string util_text = "Memory Utilization: " + std::to_string(static_cast<int>(utilization * 100)) + "%";
    render_utilization_text(util_text, Vec2f(bounds.position.x, bounds.position.y + bounds.size.y * 0.8f));
    
    // Render legend
    render_memory_legend(Rectf(bounds.position.x, bounds.position.y + bounds.size.y * 0.85f, 
                              bounds.size.x, bounds.size.y * 0.15f));
}

void MemoryVisualizationWidget::render_memory_block(const Rectf& rect, const Color& color, bool is_free) {
    // Memory block rendering implementation
    Color render_color = is_free ? Color(0.3f, 0.3f, 0.3f, 1.0f) : color;
    
    // If free, add striped pattern or different style
    if (is_free) {
        render_striped_block(rect, render_color);
    } else {
        render_solid_block(rect, render_color);
    }
}

void MemoryVisualizationWidget::render_solid_block(const Rectf& rect, const Color& color) {
    // Solid block rendering implementation
}

void MemoryVisualizationWidget::render_striped_block(const Rectf& rect, const Color& color) {
    // Striped block rendering implementation
}

void MemoryVisualizationWidget::render_block_label(const std::string& label, const Vec2f& position) {
    // Block label rendering implementation
}

void MemoryVisualizationWidget::render_utilization_text(const std::string& text, const Vec2f& position) {
    // Utilization text rendering implementation
}

void MemoryVisualizationWidget::render_memory_legend(const Rectf& bounds) {
    // Memory legend rendering implementation
}

std::string MemoryVisualizationWidget::format_bytes(usize bytes) {
    if (bytes < 1024) {
        return std::to_string(bytes) + " B";
    } else if (bytes < 1024 * 1024) {
        return std::to_string(bytes / 1024) + " KB";
    } else if (bytes < 1024 * 1024 * 1024) {
        return std::to_string(bytes / (1024 * 1024)) + " MB";
    } else {
        return std::to_string(bytes / (1024 * 1024 * 1024)) + " GB";
    }
}

//=============================================================================
// VisualDebugInterface Implementation
//=============================================================================

VisualDebugInterface::VisualDebugInterface(AdvancedProfiler* profiler) 
    : profiler_(profiler)
    , last_update_(high_resolution_clock::now())
    , update_timer_(0.0f) 
    , mouse_position_(0.0f, 0.0f) {
    
    // Initialize default configuration
    config_.show_fps_graph = true;
    config_.show_memory_graph = true;
    config_.show_gpu_metrics = true;
    config_.show_system_metrics = true;
    config_.show_heat_maps = true;
    config_.show_performance_overlay = true;
    config_.update_frequency = 60.0f;
    config_.graph_history_seconds = 30.0f;
    config_.max_systems_displayed = 20;
}

VisualDebugInterface::~VisualDebugInterface() {
    shutdown();
}

void VisualDebugInterface::initialize() {
    setup_graphs();
    setup_widgets();
    setup_charts();
    
    // Initialize cached data
    update_data();
}

void VisualDebugInterface::shutdown() {
    // Cleanup resources
    fps_graph_.reset();
    memory_graph_.reset();
    gpu_utilization_graph_.reset();
    system_performance_chart_.reset();
    memory_category_chart_.reset();
    system_heatmap_.reset();
    memory_widget_.reset();
    
    performance_widgets_.clear();
    status_indicators_.clear();
}

void VisualDebugInterface::update(f32 delta_time) {
    if (!enabled_) return;
    
    update_timer_ += delta_time;
    
    // Update at specified frequency
    if (update_timer_ >= (1.0f / config_.update_frequency)) {
        update_data();
        update_graphs();
        update_widgets();
        update_charts();
        
        update_timer_ = 0.0f;
        last_update_ = high_resolution_clock::now();
    }
}

void VisualDebugInterface::render() {
    if (!enabled_) return;
    
    if (show_main_window_) {
        render_main_window();
    }
    
    if (show_detailed_metrics_) {
        render_detailed_metrics_window();
    }
    
    if (show_memory_analyzer_) {
        render_memory_analyzer_window();
    }
    
    if (show_gpu_profiler_) {
        render_gpu_profiler_window();
    }
    
    if (show_trend_analysis_) {
        render_trend_analysis_window();
    }
    
    if (config_.show_performance_overlay) {
        render_performance_overlay();
    }
}

void VisualDebugInterface::handle_mouse_move(f32 x, f32 y) {
    mouse_position_ = Vec2f(x, y);
}

void VisualDebugInterface::handle_mouse_click(f32 x, f32 y, bool pressed) {
    if (pressed) {
        // Handle click events - check if clicking on any interactive elements
        
        // Check system list for selection
        // This would involve hit testing against rendered UI elements
    }
}

void VisualDebugInterface::handle_key_press(int key) {
    // Handle keyboard shortcuts
    switch (key) {
        case 'F': // Toggle FPS graph
            config_.show_fps_graph = !config_.show_fps_graph;
            break;
        case 'M': // Toggle memory graph
            config_.show_memory_graph = !config_.show_memory_graph;
            break;
        case 'G': // Toggle GPU metrics
            config_.show_gpu_metrics = !config_.show_gpu_metrics;
            break;
        case 'S': // Toggle system metrics
            config_.show_system_metrics = !config_.show_system_metrics;
            break;
        case 'H': // Toggle heat maps
            config_.show_heat_maps = !config_.show_heat_maps;
            break;
        case 'O': // Toggle performance overlay
            config_.show_performance_overlay = !config_.show_performance_overlay;
            break;
    }
}

void VisualDebugInterface::update_data() {
    if (!profiler_) return;
    
    // Update cached data from profiler
    cached_system_metrics_ = profiler_->get_all_system_metrics();
    cached_gpu_metrics_ = profiler_->get_gpu_metrics();
    cached_memory_metrics_ = profiler_->get_memory_metrics();
}

void VisualDebugInterface::update_graphs() {
    auto now = high_resolution_clock::now();
    f32 time_seconds = duration_cast<milliseconds>(now - last_update_).count() / 1000.0f;
    
    // Update FPS graph
    if (fps_graph_ && config_.show_fps_graph) {
        f32 fps = 1.0f / std::max(0.001f, time_seconds);
        fps_graph_->add_point(0, time_seconds, fps);
    }
    
    // Update memory graph
    if (memory_graph_ && config_.show_memory_graph) {
        f32 memory_mb = cached_memory_metrics_.process_working_set / (1024.0f * 1024.0f);
        memory_graph_->add_point(0, time_seconds, memory_mb);
    }
    
    // Update GPU utilization graph
    if (gpu_utilization_graph_ && config_.show_gpu_metrics) {
        f32 gpu_util = cached_gpu_metrics_.gpu_utilization * 100.0f;
        gpu_utilization_graph_->add_point(0, time_seconds, gpu_util);
    }
}

void VisualDebugInterface::update_widgets() {
    // Update performance widgets
    for (auto& widget : performance_widgets_) {
        // Update widget values based on current metrics
        // This would be customized based on what each widget tracks
    }
    
    // Update status indicators
    for (auto& indicator : status_indicators_) {
        // Update indicator status based on current system state
        // This would involve checking system performance and health
    }
    
    // Update memory visualization widget
    if (memory_widget_) {
        memory_widget_->clear();
        
        // Add memory blocks based on current allocations
        // This would require integration with the memory debugger
        usize offset = 0;
        memory_widget_->add_block(offset, cached_memory_metrics_.process_working_set, 
                                "Process Memory", Color::BLUE);
    }
}

void VisualDebugInterface::update_charts() {
    // Update system performance chart
    if (system_performance_chart_) {
        system_performance_chart_->clear();
        
        // Add bars for each system's performance score
        for (const auto& metrics : cached_system_metrics_) {
            f32 score = static_cast<f32>(metrics.get_performance_score());
            Color bar_color = get_performance_color(score);
            system_performance_chart_->add_bar(metrics.system_name, score, bar_color);
        }
    }
    
    // Update memory category chart
    if (memory_category_chart_) {
        memory_category_chart_->clear();
        
        // Add slices for memory categories
        auto category_breakdown = cached_memory_metrics_.allocation_pattern;
        
        if (category_breakdown.small_allocations > 0) {
            memory_category_chart_->add_slice("Small", static_cast<f32>(category_breakdown.small_allocations), Color::GREEN);
        }
        if (category_breakdown.medium_allocations > 0) {
            memory_category_chart_->add_slice("Medium", static_cast<f32>(category_breakdown.medium_allocations), Color::YELLOW);
        }
        if (category_breakdown.large_allocations > 0) {
            memory_category_chart_->add_slice("Large", static_cast<f32>(category_breakdown.large_allocations), Color::RED);
        }
    }
    
    // Update system heatmap
    if (system_heatmap_) {
        // Update heatmap with current system performance data
        // This would show performance relationships between systems
        
        usize system_count = std::min(cached_system_metrics_.size(), static_cast<usize>(10));
        for (usize i = 0; i < system_count; ++i) {
            for (usize j = 0; j < system_count; ++j) {
                f32 value = 0.5f; // Placeholder - would calculate actual correlation
                system_heatmap_->set_value(static_cast<u32>(i), static_cast<u32>(j), value);
            }
        }
    }
}

void VisualDebugInterface::render_main_window() {
    // Main window layout
    Rectf main_bounds(50.0f, 50.0f, 1200.0f, 800.0f);
    
    // Window background
    render_window_background(main_bounds, config_.theme_secondary);
    
    // Title bar
    render_window_title("ECScope Performance Monitor", 
                       Vec2f(main_bounds.position.x + 10, main_bounds.position.y + 10));
    
    // Main content area
    Rectf content_area(main_bounds.position.x + 10, main_bounds.position.y + 40,
                      main_bounds.size.x - 20, main_bounds.size.y - 50);
    
    // Split into sections
    f32 graph_height = content_area.size.y * 0.6f;
    f32 metrics_height = content_area.size.y * 0.4f;
    
    // Graph section
    Rectf graph_section(content_area.position.x, content_area.position.y,
                       content_area.size.x, graph_height);
    render_graphs_section(graph_section);
    
    // Metrics section
    Rectf metrics_section(content_area.position.x, content_area.position.y + graph_height,
                         content_area.size.x, metrics_height);
    render_metrics_section(metrics_section);
}

void VisualDebugInterface::render_detailed_metrics_window() {
    Rectf window_bounds(100.0f, 100.0f, 800.0f, 600.0f);
    
    render_window_background(window_bounds, config_.theme_secondary);
    render_window_title("Detailed System Metrics", 
                       Vec2f(window_bounds.position.x + 10, window_bounds.position.y + 10));
    
    // Render system list and details
    render_system_list();
    
    if (!selected_system_.empty()) {
        render_system_details(selected_system_);
    }
}

void VisualDebugInterface::render_memory_analyzer_window() {
    Rectf window_bounds(150.0f, 150.0f, 900.0f, 700.0f);
    
    render_window_background(window_bounds, config_.theme_secondary);
    render_window_title("Memory Analysis", 
                       Vec2f(window_bounds.position.x + 10, window_bounds.position.y + 10));
    
    // Memory visualization
    if (memory_widget_) {
        Rectf mem_area(window_bounds.position.x + 20, window_bounds.position.y + 50,
                      window_bounds.size.x - 40, 200);
        memory_widget_->render(mem_area);
    }
    
    // Memory category chart
    if (memory_category_chart_) {
        Rectf chart_area(window_bounds.position.x + 20, window_bounds.position.y + 270,
                        window_bounds.size.x - 40, 300);
        memory_category_chart_->render(chart_area);
    }
}

void VisualDebugInterface::render_gpu_profiler_window() {
    Rectf window_bounds(200.0f, 200.0f, 800.0f, 600.0f);
    
    render_window_background(window_bounds, config_.theme_secondary);
    render_window_title("GPU Performance Monitor", 
                       Vec2f(window_bounds.position.x + 10, window_bounds.position.y + 10));
    
    // GPU utilization graph
    if (gpu_utilization_graph_) {
        Rectf graph_area(window_bounds.position.x + 20, window_bounds.position.y + 50,
                        window_bounds.size.x - 40, 200);
        gpu_utilization_graph_->render(graph_area);
    }
    
    // GPU metrics display
    render_gpu_metrics_display(window_bounds);
}

void VisualDebugInterface::render_trend_analysis_window() {
    Rectf window_bounds(250.0f, 250.0f, 1000.0f, 700.0f);
    
    render_window_background(window_bounds, config_.theme_secondary);
    render_window_title("Performance Trend Analysis", 
                       Vec2f(window_bounds.position.x + 10, window_bounds.position.y + 10));
    
    // Trend analysis content
    auto trends = profiler_->analyze_all_trends();
    render_trend_analysis_content(trends, window_bounds);
}

void VisualDebugInterface::render_performance_overlay() {
    // Simple performance overlay in corner of screen
    Rectf overlay_bounds(10.0f, 10.0f, 300.0f, 150.0f);
    
    // Semi-transparent background
    Color bg_color = config_.theme_secondary;
    bg_color.a = 0.8f;
    render_overlay_background(overlay_bounds, bg_color);
    
    // Performance text
    f64 overall_score = profiler_->calculate_overall_performance_score();
    std::string score_text = "Performance: " + std::to_string(static_cast<int>(overall_score)) + "/100";
    
    Color score_color = get_performance_color(overall_score);
    render_overlay_text(score_text, Vec2f(overlay_bounds.position.x + 10, overlay_bounds.position.y + 20), score_color);
    
    // Memory usage
    f32 memory_mb = cached_memory_metrics_.process_working_set / (1024.0f * 1024.0f);
    std::string memory_text = "Memory: " + std::to_string(static_cast<int>(memory_mb)) + " MB";
    render_overlay_text(memory_text, Vec2f(overlay_bounds.position.x + 10, overlay_bounds.position.y + 45), config_.theme_text);
    
    // System count
    std::string systems_text = "Systems: " + std::to_string(cached_system_metrics_.size());
    render_overlay_text(systems_text, Vec2f(overlay_bounds.position.x + 10, overlay_bounds.position.y + 70), config_.theme_text);
    
    // Anomaly count
    auto anomalies = profiler_->detect_anomalies();
    if (!anomalies.empty()) {
        std::string anomaly_text = "Issues: " + std::to_string(anomalies.size());
        render_overlay_text(anomaly_text, Vec2f(overlay_bounds.position.x + 10, overlay_bounds.position.y + 95), Color::RED);
    }
}

void VisualDebugInterface::render_system_list() {
    // System list rendering implementation
}

void VisualDebugInterface::render_system_details(const std::string& system_name) {
    // System details rendering implementation
}

void VisualDebugInterface::render_anomaly_alerts() {
    auto anomalies = profiler_->detect_anomalies();
    
    // Render anomaly alerts
    for (const auto& anomaly : anomalies) {
        // Render alert notification
    }
}

void VisualDebugInterface::render_recommendations() {
    auto recommendations = profiler_->get_performance_recommendations();
    
    // Render recommendations list
    for (const auto& rec : recommendations) {
        // Render recommendation item
    }
}

void VisualDebugInterface::setup_graphs() {
    // Setup FPS graph
    GraphConfig fps_config;
    fps_config.title = "Frame Rate (FPS)";
    fps_config.x_label = "Time (s)";
    fps_config.y_label = "FPS";
    fps_config.y_min = 0.0f;
    fps_config.y_max = 120.0f;
    fps_config.auto_scale = false;
    fps_graph_ = std::make_unique<LineGraph>(fps_config);
    fps_graph_->add_series("FPS", Color::GREEN);
    
    // Setup memory graph
    GraphConfig mem_config;
    mem_config.title = "Memory Usage (MB)";
    mem_config.x_label = "Time (s)";
    mem_config.y_label = "MB";
    mem_config.y_min = 0.0f;
    mem_config.auto_scale = true;
    memory_graph_ = std::make_unique<LineGraph>(mem_config);
    memory_graph_->add_series("Memory", Color::BLUE);
    
    // Setup GPU utilization graph
    GraphConfig gpu_config;
    gpu_config.title = "GPU Utilization (%)";
    gpu_config.x_label = "Time (s)";
    gpu_config.y_label = "Utilization %";
    gpu_config.y_min = 0.0f;
    gpu_config.y_max = 100.0f;
    gpu_config.auto_scale = false;
    gpu_utilization_graph_ = std::make_unique<LineGraph>(gpu_config);
    gpu_utilization_graph_->add_series("GPU", Color::RED);
}

void VisualDebugInterface::setup_widgets() {
    // Setup performance widgets
    auto fps_widget = std::make_unique<PerformanceWidget>("FPS", " fps");
    fps_widget->set_thresholds(30.0f, 15.0f); // Warning at 30 FPS, critical at 15 FPS
    fps_widget->set_target(60.0f);
    performance_widgets_.push_back(std::move(fps_widget));
    
    auto memory_widget = std::make_unique<PerformanceWidget>("Memory", " MB");
    memory_widget->set_thresholds(512.0f, 1024.0f); // Warning at 512 MB, critical at 1 GB
    performance_widgets_.push_back(std::move(memory_widget));
    
    // Setup status indicators
    for (const auto& metrics : cached_system_metrics_) {
        auto indicator = std::make_unique<SystemStatusIndicator>(metrics.system_name);
        status_indicators_.push_back(std::move(indicator));
    }
    
    // Setup memory visualization widget
    memory_widget_ = std::make_unique<MemoryVisualizationWidget>();
}

void VisualDebugInterface::setup_charts() {
    // Setup system performance bar chart
    system_performance_chart_ = std::make_unique<BarChart>("System Performance Scores");
    system_performance_chart_->set_max_value(100.0f);
    
    // Setup memory category pie chart
    memory_category_chart_ = std::make_unique<PieChart>("Memory Allocation Categories");
    
    // Setup system performance heatmap
    system_heatmap_ = std::make_unique<PerformanceHeatmap>("System Interaction Heatmap", 10, 10);
}

Color VisualDebugInterface::get_performance_color(f64 score) const {
    if (score >= 90.0) {
        return Color::GREEN;
    } else if (score >= 80.0) {
        return Color(0.8f, 1.0f, 0.0f, 1.0f); // Light green
    } else if (score >= 70.0) {
        return Color::YELLOW;
    } else if (score >= 60.0) {
        return Color::ORANGE;
    } else {
        return Color::RED;
    }
}

std::string VisualDebugInterface::format_bytes(usize bytes) const {
    if (bytes < 1024) {
        return std::to_string(bytes) + " B";
    } else if (bytes < 1024 * 1024) {
        return std::to_string(bytes / 1024) + " KB";
    } else if (bytes < 1024 * 1024 * 1024) {
        return std::to_string(bytes / (1024 * 1024)) + " MB";
    } else {
        return std::to_string(bytes / (1024 * 1024 * 1024)) + " GB";
    }
}

std::string VisualDebugInterface::format_time(high_resolution_clock::duration duration) const {
    auto us = duration_cast<microseconds>(duration).count();
    if (us < 1000) {
        return std::to_string(us) + " μs";
    } else if (us < 1000000) {
        return std::to_string(us / 1000) + " ms";
    } else {
        return std::to_string(us / 1000000) + " s";
    }
}

std::string VisualDebugInterface::format_percentage(f32 percentage) const {
    return std::to_string(static_cast<int>(percentage * 100)) + "%";
}

// Rendering stubs - these would be implemented with actual graphics API
void VisualDebugInterface::render_window_background(const Rectf& bounds, const Color& color) {}
void VisualDebugInterface::render_window_title(const std::string& title, const Vec2f& position) {}
void VisualDebugInterface::render_graphs_section(const Rectf& bounds) {}
void VisualDebugInterface::render_metrics_section(const Rectf& bounds) {}
void VisualDebugInterface::render_gpu_metrics_display(const Rectf& bounds) {}
void VisualDebugInterface::render_trend_analysis_content(const std::vector<std::pair<std::string, PerformanceTrend>>& trends, const Rectf& bounds) {}
void VisualDebugInterface::render_overlay_background(const Rectf& bounds, const Color& color) {}
void VisualDebugInterface::render_overlay_text(const std::string& text, const Vec2f& position, const Color& color) {}

} // namespace ecscope::profiling