/**
 * @file panel_performance_lab.cpp
 * @brief Performance Laboratory UI Panel Implementation
 */

#include "panel_performance_lab.hpp"
#include "core/log.hpp"
#include "core/time.hpp"

namespace ecscope::ui {

// PerformanceLabPanel Implementation
PerformanceLabPanel::PerformanceLabPanel(std::shared_ptr<performance::PerformanceLab> lab)
    : performance_lab_(lab)
    , current_mode_(DisplayMode::Overview)
    , is_monitoring_(false)
    , show_advanced_metrics_(false)
    , tutorial_mode_enabled_(false)
    , last_graph_update_time_(0.0)
    , graph_update_frequency_(10.0f) // 10 Hz
{
    initialize_educational_explanations();
    initialize_tutorial_content();
    
    LOG_INFO("Performance Laboratory Panel initialized");
}

void PerformanceLabPanel::render() {
    // Basic implementation for now
    /*
    if (ImGui::Begin("Performance Laboratory", &is_visible_)) {
        // Mode selection tabs
        if (ImGui::BeginTabBar("PerformanceModes")) {
            if (ImGui::BeginTabItem("Overview")) {
                current_mode_ = DisplayMode::Overview;
                render_overview_mode();
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem("Memory Experiments")) {
                current_mode_ = DisplayMode::MemoryExperiments;
                render_memory_experiments_mode();
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem("Allocation Benchmarks")) {
                current_mode_ = DisplayMode::AllocationBench;
                render_allocation_bench_mode();
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem("Real-Time Monitor")) {
                current_mode_ = DisplayMode::RealTimeMonitor;
                render_realtime_monitor_mode();
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem("Recommendations")) {
                current_mode_ = DisplayMode::Recommendations;
                render_recommendations_mode();
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem("Educational")) {
                current_mode_ = DisplayMode::Educational;
                render_educational_mode();
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
    */
}

void PerformanceLabPanel::update(f64 dt) {
    f64 current_time = core::Time::get_time_seconds();
    
    // Update performance data at specified frequency
    if (current_time - last_graph_update_time_ >= (1.0 / graph_update_frequency_)) {
        update_performance_data();
        last_graph_update_time_ = current_time;
    }
    
    update_experiment_state();
    
    if (recommendation_display_.auto_update_recommendations_ && 
        current_time - recommendation_display_.last_recommendation_update_ >= 1.0) {
        update_recommendations();
        recommendation_display_.last_recommendation_update_ = current_time;
    }
}

void PerformanceLabPanel::update_performance_data() {
    if (!performance_lab_) return;
    
    // Get current performance snapshot
    auto snapshot = performance_lab_->get_current_snapshot();
    
    // Add to graph data
    f32 memory_mb = static_cast<f32>(snapshot.memory_usage_bytes) / (1024.0f * 1024.0f);
    f32 alloc_rate = 0.0f; // Would need to calculate from snapshot
    f32 frame_time = static_cast<f32>(snapshot.frame_time_ms);
    f32 cache_eff = 0.85f; // Placeholder
    
    graph_data_.add_sample(memory_mb, alloc_rate, frame_time, cache_eff);
}

void PerformanceLabPanel::update_experiment_state() {
    if (!performance_lab_) return;
    
    auto status = performance_lab_->get_experiment_status();
    experiment_state_.is_running_ = (status == performance::ExperimentStatus::Running);
    
    // Update available experiments
    experiment_state_.available_experiments_ = performance_lab_->get_available_experiments();
    
    // Check for completed results
    if (status == performance::ExperimentStatus::Completed) {
        auto result = performance_lab_->get_experiment_result();
        if (result.has_value()) {
            experiment_state_.cached_results_[result->name] = *result;
        }
    }
}

void PerformanceLabPanel::update_recommendations() {
    if (!performance_lab_) return;
    
    recommendation_display_.current_recommendations_ = performance_lab_->get_current_recommendations();
}

void PerformanceLabPanel::render_overview_mode() {
    // Simplified implementation
}

void PerformanceLabPanel::render_memory_experiments_mode() {
    // Simplified implementation
}

void PerformanceLabPanel::render_allocation_bench_mode() {
    // Simplified implementation
}

void PerformanceLabPanel::render_realtime_monitor_mode() {
    // Simplified implementation
}

void PerformanceLabPanel::render_recommendations_mode() {
    // Simplified implementation
}

void PerformanceLabPanel::render_educational_mode() {
    // Simplified implementation
}

void PerformanceLabPanel::start_experiment(const std::string& name) {
    if (performance_lab_) {
        performance_lab_->start_experiment_async(name);
        experiment_state_.current_experiment_ = name;
        experiment_state_.status_message_ = "Starting experiment: " + name;
        
        LOG_INFO("Started performance experiment: " + name);
    }
}

void PerformanceLabPanel::start_monitoring() {
    if (performance_lab_) {
        performance_lab_->start_monitoring();
        is_monitoring_ = true;
        
        LOG_INFO("Started performance monitoring");
    }
}

void PerformanceLabPanel::stop_monitoring() {
    if (performance_lab_) {
        performance_lab_->stop_monitoring();
        is_monitoring_ = false;
        
        LOG_INFO("Stopped performance monitoring");
    }
}

void PerformanceLabPanel::initialize_educational_explanations() {
    educational_content_.explanations_["Memory Layout"] = 
        "Memory layout refers to how data is organized in memory. Structure of Arrays (SoA) "
        "stores each component type in separate arrays, while Array of Structures (AoS) "
        "keeps complete objects together.";
        
    educational_content_.explanations_["Cache Performance"] = 
        "Cache performance is critical for modern processors. Data that fits in cache "
        "can be accessed much faster than data that must be fetched from main memory.";
        
    educational_content_.explanations_["Allocation Strategies"] = 
        "Different allocation strategies optimize for different use cases. Arena allocators "
        "excel at temporary allocations, while pool allocators are perfect for same-sized objects.";
}

void PerformanceLabPanel::initialize_tutorial_content() {
    educational_content_.tutorial_steps_ = {
        "Welcome to the Performance Laboratory! This tool helps you understand memory behavior.",
        "First, let's look at the difference between SoA and AoS memory layouts.",
        "Next, we'll explore how different allocators perform under various conditions.",
        "Finally, we'll analyze real-time performance data to identify bottlenecks."
    };
}

f32 PerformanceLabPanel::normalize_value(f32 value, f32 min_val, f32 max_val) {
    if (max_val <= min_val) return 0.0f;
    return (value - min_val) / (max_val - min_val);
}

std::string PerformanceLabPanel::format_performance_value(f32 value, const char* unit) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%.2f %s", value, unit);
    return std::string(buffer);
}

std::string PerformanceLabPanel::get_performance_description(f32 score) {
    if (score >= 0.9f) return "Excellent";
    else if (score >= 0.7f) return "Good";
    else if (score >= 0.5f) return "Fair";
    else return "Poor";
}

void PerformanceLabPanel::show_explanation(const std::string& topic) {
    auto it = educational_content_.explanations_.find(topic);
    if (it != educational_content_.explanations_.end()) {
        LOG_INFO("Explanation for " + topic + ": " + it->second);
    }
}

void PerformanceLabPanel::export_performance_data() {
    if (performance_lab_) {
        std::string filename = "performance_export_" + std::to_string(static_cast<int>(core::Time::get_time_seconds())) + ".json";
        performance_lab_->export_results_to_json(filename);
        
        LOG_INFO("Performance data exported to: " + filename);
    }
}

// MemoryVisualizationWidget Implementation
MemoryVisualizationWidget::MemoryVisualizationWidget(f32 width, f32 height)
    : current_type_(VisualizationType::MemoryLayout)
    , widget_width_(width)
    , widget_height_(height) {
}

void MemoryVisualizationWidget::render_memory_layout(f32 soa_efficiency, f32 aos_efficiency) {
    // Simplified implementation - would render visual comparison
}

void MemoryVisualizationWidget::render_cache_lines(f32 utilization, f32 miss_rate) {
    // Simplified implementation - would render cache visualization
}

void MemoryVisualizationWidget::render_fragmentation(f32 fragmentation_ratio, const std::vector<usize>& free_blocks) {
    // Simplified implementation - would render fragmentation visualization
}

void MemoryVisualizationWidget::render_access_patterns(const std::vector<f32>& access_times) {
    // Simplified implementation - would render access pattern visualization
}

// PerformanceMetricsDashboard Implementation
PerformanceMetricsDashboard::PerformanceMetricsDashboard(f32 width, f32 height)
    : dashboard_width_(width), dashboard_height_(height) {
}

void PerformanceMetricsDashboard::add_metric(const std::string& name, f32 value, f32 min_val, f32 max_val, const std::string& unit) {
    MetricDisplay metric;
    metric.name = name;
    metric.current_value = value;
    metric.min_value = min_val;
    metric.max_value = max_val;
    metric.unit = unit;
    metric.color = PerformanceLabPanel::Colors::get_performance_color(value / max_val);
    
    metrics_.push_back(metric);
}

void PerformanceMetricsDashboard::update_metric(const std::string& name, f32 value) {
    auto it = std::find_if(metrics_.begin(), metrics_.end(),
                          [&name](const MetricDisplay& m) { return m.name == name; });
    
    if (it != metrics_.end()) {
        it->current_value = value;
        it->color = PerformanceLabPanel::Colors::get_performance_color(value / it->max_value);
    }
}

void PerformanceMetricsDashboard::set_metric_target(const std::string& name, f32 target) {
    auto it = std::find_if(metrics_.begin(), metrics_.end(),
                          [&name](const MetricDisplay& m) { return m.name == name; });
    
    if (it != metrics_.end()) {
        it->target_value = target;
        it->show_target = true;
    }
}

void PerformanceMetricsDashboard::render() {
    // Simplified implementation - would render metrics dashboard
    for (const auto& metric : metrics_) {
        // Would render each metric as a progress bar or gauge
    }
}

} // namespace ecscope::ui