#include "panel_performance_comparison.hpp"
#include "core/log.hpp"
#include "../imgui_utils.hpp"
#include <imgui.h>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <random>

namespace ecscope::ui {

// PerformanceComparisonPanel Implementation

PerformanceComparisonPanel::PerformanceComparisonPanel(std::shared_ptr<performance::PerformanceLab> lab)
    : Panel("Performance Comparison Tools", true), performance_lab_(lab) {
    
    // Initialize default benchmarks
    initialize_default_benchmarks();
    
    // Initialize educational content
    initialize_educational_content();
    
    // Setup performance graphs
    performance_graphs_.emplace_back("Frame Time");
    performance_graphs_.emplace_back("Memory Usage");
    performance_graphs_.emplace_back("Cache Hit Ratio");
    
    LOG_INFO("Performance Comparison Panel initialized");
}

void PerformanceComparisonPanel::render() {
    if (!visible_) return;
    
    ImGui::SetNextWindowSize(ImVec2(1000, 700), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(MIN_PANEL_WIDTH, MIN_PANEL_HEIGHT), ImVec2(FLT_MAX, FLT_MAX));
    
    if (ImGui::Begin(name_.c_str(), &visible_, ImGuiWindowFlags_MenuBar)) {
        
        // Menu bar
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Benchmarks")) {
                if (ImGui::MenuItem("Select Benchmarks", nullptr, current_mode_ == ComparisonMode::BenchmarkSelection)) {
                    current_mode_ = ComparisonMode::BenchmarkSelection;
                }
                if (ImGui::MenuItem("Run Benchmarks", "F5", current_mode_ == ComparisonMode::RunningBenchmarks, !selected_benchmarks_.empty())) {
                    current_mode_ = ComparisonMode::RunningBenchmarks;
                    start_benchmark_suite();
                }
                if (ImGui::MenuItem("View Results", nullptr, current_mode_ == ComparisonMode::ResultsAnalysis, !benchmark_results_.empty())) {
                    current_mode_ = ComparisonMode::ResultsAnalysis;
                }
                
                ImGui::Separator();
                
                if (ImGui::MenuItem("Clear Results", nullptr, false, !benchmark_results_.empty())) {
                    reset_results();
                }
                if (ImGui::MenuItem("Export Results", nullptr, false, !benchmark_results_.empty())) {
                    export_results("performance_results.json");
                }
                
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Interactive")) {
                if (ImGui::MenuItem("Performance Demo", nullptr, current_mode_ == ComparisonMode::InteractiveDemo)) {
                    current_mode_ = ComparisonMode::InteractiveDemo;
                }
                if (ImGui::MenuItem("Custom A/B Test", nullptr, current_mode_ == ComparisonMode::CustomComparison)) {
                    current_mode_ = ComparisonMode::CustomComparison;
                }
                
                ImGui::Separator();
                
                ImGui::MenuItem("Real-time Visualization", nullptr, &interactive_demo_.show_entity_visualization_);
                ImGui::MenuItem("Memory Layout View", nullptr, &interactive_demo_.show_memory_layout_);
                ImGui::MenuItem("Cache Behavior View", nullptr, &interactive_demo_.show_cache_behavior_);
                
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Education")) {
                if (ImGui::MenuItem("Learning Guide", nullptr, current_mode_ == ComparisonMode::EducationalGuide)) {
                    current_mode_ = ComparisonMode::EducationalGuide;
                }
                
                ImGui::Separator();
                
                ImGui::MenuItem("Context Help", nullptr, &educational_content_.context_help_enabled_);
                ImGui::MenuItem("Guided Mode", nullptr, &educational_content_.guided_mode_enabled_);
                
                if (ImGui::MenuItem("Start Learning Path")) {
                    start_guided_learning();
                }
                
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Visualization")) {
                const char* chart_types[] = {"Bar Chart", "Line Chart", "Scatter Plot", "Heat Map", "Radar Chart"};
                int current_chart = static_cast<int>(results_analysis_.chart_type_);
                
                if (ImGui::Combo("Chart Type", &current_chart, chart_types, 5)) {
                    results_analysis_.chart_type_ = static_cast<ResultsAnalysis::ChartType>(current_chart);
                }
                
                ImGui::Separator();
                
                ImGui::MenuItem("Show Grid", nullptr, &viz_settings_.show_grid_);
                ImGui::MenuItem("Show Values", nullptr, &viz_settings_.show_values_on_bars_);
                ImGui::MenuItem("Animate Charts", nullptr, &results_analysis_.animate_charts_);
                ImGui::MenuItem("High Contrast", nullptr, &viz_settings_.high_contrast_mode_);
                
                ImGui::SliderFloat("Chart Height", &results_analysis_.chart_height_, CHART_MIN_HEIGHT, CHART_MAX_HEIGHT);
                ImGui::SliderFloat("UI Scale", &viz_settings_.ui_scale_factor_, 0.8f, 2.0f);
                
                ImGui::EndMenu();
            }
            
            ImGui::EndMenuBar();
        }
        
        // Mode-specific content
        switch (current_mode_) {
            case ComparisonMode::BenchmarkSelection:
                render_benchmark_selection();
                break;
            case ComparisonMode::RunningBenchmarks:
                render_running_benchmarks();
                break;
            case ComparisonMode::ResultsAnalysis:
                render_results_analysis();
                break;
            case ComparisonMode::InteractiveDemo:
                render_interactive_demo();
                break;
            case ComparisonMode::EducationalGuide:
                render_educational_guide();
                break;
            case ComparisonMode::CustomComparison:
                render_custom_comparison();
                break;
        }
        
        // Status bar at bottom
        ImGui::Separator();
        ImGui::Text("Status: ");
        ImGui::SameLine();
        
        if (benchmarks_running_) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Running benchmarks... (%.1f%%)", overall_progress_ * 100.0f);
        } else if (!benchmark_results_.empty()) {
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.3f, 1.0f), "Ready - %zu results available", benchmark_results_.size());
        } else {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Select benchmarks to begin");
        }
        
        ImGui::SameLine();
        ImGui::Text(" | Selected: %zu | Mode: %s", 
                   selected_benchmarks_.size(),
                   current_mode_ == ComparisonMode::BenchmarkSelection ? "Selection" :
                   current_mode_ == ComparisonMode::RunningBenchmarks ? "Running" :
                   current_mode_ == ComparisonMode::ResultsAnalysis ? "Analysis" :
                   current_mode_ == ComparisonMode::InteractiveDemo ? "Demo" :
                   current_mode_ == ComparisonMode::EducationalGuide ? "Learning" : "Custom");
    }
    ImGui::End();
}

void PerformanceComparisonPanel::update(f64 delta_time) {
    if (!visible_) return;
    
    // Update progress tracking
    last_progress_update_ += delta_time;
    if (last_progress_update_ >= 1.0 / PROGRESS_UPDATE_FREQUENCY) {
        if (benchmarks_running_) {
            // Simulate benchmark progress
            overall_progress_ = static_cast<f32>(current_benchmark_index_) / static_cast<f32>(selected_benchmarks_.size());
            
            // Check if benchmarks are complete
            if (current_benchmark_index_ >= selected_benchmarks_.size()) {
                on_benchmark_suite_completed();
            }
        }
        last_progress_update_ = 0.0;
    }
    
    // Update interactive demo
    last_demo_update_ += delta_time;
    if (last_demo_update_ >= 1.0 / DEMO_UPDATE_FREQUENCY && interactive_demo_.demo_active_) {
        measure_demo_performance();
        last_demo_update_ = 0.0;
    }
    
    // Update results analysis
    last_analysis_update_ += delta_time;
    if (last_analysis_update_ >= 1.0 / RESULTS_ANALYSIS_FREQUENCY && current_mode_ == ComparisonMode::ResultsAnalysis) {
        // Could update real-time analysis here
        last_analysis_update_ = 0.0;
    }
}

bool PerformanceComparisonPanel::wants_keyboard_capture() const {
    return ImGui::IsWindowFocused();
}

bool PerformanceComparisonPanel::wants_mouse_capture() const {
    return ImGui::IsWindowHovered() || ImGui::IsWindowFocused();
}

void PerformanceComparisonPanel::render_benchmark_selection() {
    ImGui::Text("🎯 Select Performance Benchmarks");
    ImGui::Separator();
    
    // Benchmark categories
    render_benchmark_categories();
    
    ImGui::Separator();
    
    // Two-column layout: Available vs Selected
    ImGui::Columns(2, "##benchmark_selection", true);
    
    // Available benchmarks
    ImGui::Text("📋 Available Benchmarks");
    render_benchmark_list();
    
    ImGui::NextColumn();
    
    // Selected benchmarks
    ImGui::Text("✅ Selected Benchmarks (%zu)", selected_benchmarks_.size());
    render_benchmark_configuration();
    
    ImGui::Columns(1);
    
    ImGui::Separator();
    
    // Action buttons
    if (ImGui::Button("Run Selected Benchmarks", ImVec2(200, 40))) {
        if (!selected_benchmarks_.empty()) {
            current_mode_ = ComparisonMode::RunningBenchmarks;
            start_benchmark_suite();
        }
    }
    
    if (!selected_benchmarks_.empty()) {
        ImGui::SameLine();
        if (ImGui::Button("Preview Configuration")) {
            render_benchmark_preview();
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Clear Selection")) {
            selected_benchmarks_.clear();
        }
    }
    
    // Educational info
    if (educational_content_.context_help_enabled_) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "💡 Tip:");
        ImGui::TextWrapped("Start with 'Entity Iteration' and 'Component Access' benchmarks to understand basic ECS performance characteristics.");
    }
}

void PerformanceComparisonPanel::render_running_benchmarks() {
    ImGui::Text("🏃 Running Performance Benchmarks");
    ImGui::Separator();
    
    // Overall progress
    render_execution_progress();
    
    ImGui::Separator();
    
    // Current benchmark info
    render_current_benchmark_info();
    
    ImGui::Separator();
    
    // Real-time metrics
    render_real_time_metrics();
    
    ImGui::Separator();
    
    // Execution controls
    render_execution_controls();
}

void PerformanceComparisonPanel::render_results_analysis() {
    ImGui::Text("📊 Performance Results Analysis");
    ImGui::Separator();
    
    // Results overview
    render_results_overview();
    
    ImGui::Separator();
    
    // Comparison charts
    ImGui::Text("📈 Performance Comparison");
    render_comparison_charts();
    
    ImGui::Separator();
    
    // Detailed metrics and insights
    ImGui::Columns(2, "##analysis_layout", true);
    
    // Left column - detailed metrics
    ImGui::Text("📋 Detailed Metrics");
    render_detailed_metrics();
    
    ImGui::NextColumn();
    
    // Right column - insights and recommendations
    ImGui::Text("💡 Performance Insights");
    render_performance_insights();
    
    ImGui::Columns(1);
}

void PerformanceComparisonPanel::render_interactive_demo() {
    ImGui::Text("🎮 Interactive Performance Demonstration");
    ImGui::Separator();
    
    // Demo selection
    render_demo_selection();
    
    ImGui::Separator();
    
    if (interactive_demo_.demo_active_) {
        // Two-column layout: Controls vs Visualization
        ImGui::Columns(2, "##demo_layout", true);
        
        // Left column - controls
        ImGui::Text("🎛️ Demo Controls");
        render_demo_controls();
        
        ImGui::NextColumn();
        
        // Right column - real-time visualization
        ImGui::Text("📊 Real-time Performance");
        render_real_time_visualization();
        
        ImGui::Columns(1);
        
        ImGui::Separator();
        
        // Parameter sliders
        render_parameter_sliders();
    } else {
        ImGui::TextDisabled("Select a demo to begin interactive exploration");
    }
}

void PerformanceComparisonPanel::render_educational_guide() {
    ImGui::Text("🎓 Performance Learning Guide");
    ImGui::Separator();
    
    // Learning path overview
    render_learning_path();
    
    ImGui::Separator();
    
    // Current tutorial step or concept explanation
    if (educational_content_.guided_mode_enabled_) {
        render_guided_tutorial();
    } else {
        render_concept_explanations();
    }
    
    ImGui::Separator();
    
    // Context help
    render_context_help();
}

void PerformanceComparisonPanel::render_custom_comparison() {
    ImGui::Text("⚖️ Custom A/B Performance Testing");
    ImGui::Separator();
    
    // A/B test configuration
    ImGui::Text("Configure A/B Test:");
    
    static char test_name[128] = "Custom Performance Test";
    ImGui::InputText("Test Name", test_name, sizeof(test_name));
    
    ImGui::Separator();
    
    // Configuration A vs B
    ImGui::Columns(2, "##ab_config", true);
    
    ImGui::Text("Configuration A");
    // Configuration A setup would go here
    
    ImGui::NextColumn();
    
    ImGui::Text("Configuration B");
    // Configuration B setup would go here
    
    ImGui::Columns(1);
    
    ImGui::Separator();
    
    // Test parameters
    ImGui::Text("Test Parameters:");
    static int sample_size = 10;
    static float confidence_level = 0.95f;
    
    ImGui::SliderInt("Sample Size", &sample_size, 5, 100);
    ImGui::SliderFloat("Confidence Level", &confidence_level, 0.80f, 0.99f, "%.2f");
    
    ImGui::Separator();
    
    if (ImGui::Button("Run A/B Test")) {
        // Start A/B test
        LOG_INFO("Starting A/B test: " + std::string(test_name));
    }
    
    // Display A/B test results if available
    if (!ab_tests_.empty()) {
        ImGui::Separator();
        ImGui::Text("A/B Test Results:");
        
        for (const auto& test : ab_tests_) {
            if (test.test_completed_) {
                ImGui::Text("📊 %s", test.test_name_.c_str());
                ImGui::Indent();
                ImGui::Text("Statistical significance: %.3f", test.statistical_significance_);
                ImGui::Text("Conclusion: %s", test.conclusion_.c_str());
                ImGui::Unindent();
            }
        }
    }
}

// Specific rendering components

void PerformanceComparisonPanel::render_benchmark_categories() {
    const std::vector<std::pair<BenchmarkType, std::string>> categories = {
        {BenchmarkType::EntityIteration, "🔄 Entity Iteration"},
        {BenchmarkType::ComponentAccess, "🧩 Component Access"},
        {BenchmarkType::SystemExecution, "⚙️ System Execution"},
        {BenchmarkType::MemoryLayoutComparison, "💾 Memory Layout"},
        {BenchmarkType::CacheBehaviorAnalysis, "🗄️ Cache Behavior"},
        {BenchmarkType::AllocationStrategies, "📦 Allocation"},
        {BenchmarkType::QueryPerformance, "🔍 Query Performance"},
        {BenchmarkType::ScalingAnalysis, "📈 Scaling Analysis"}
    };
    
    static BenchmarkType selected_category = BenchmarkType::EntityIteration;
    
    for (usize i = 0; i < categories.size(); ++i) {
        if (i > 0) ImGui::SameLine();
        
        const auto& [type, label] = categories[i];
        bool is_selected = (selected_category == type);
        
        if (is_selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 1.0f, 1.0f));
        }
        
        if (ImGui::Button(label.c_str())) {
            selected_category = type;
        }
        
        if (is_selected) {
            ImGui::PopStyleColor();
        }
    }
}

void PerformanceComparisonPanel::render_benchmark_list() {
    ImGui::BeginChild("##available_benchmarks", ImVec2(0, 300), true);
    
    for (const auto& benchmark : available_benchmarks_) {
        bool is_selected = std::find_if(selected_benchmarks_.begin(), selected_benchmarks_.end(),
                                       [&](const BenchmarkConfig& config) { 
                                           return config.name == benchmark.name; 
                                       }) != selected_benchmarks_.end();
        
        if (ImGui::Selectable(benchmark.name.c_str(), is_selected)) {
            if (!is_selected) {
                selected_benchmarks_.push_back(benchmark);
                LOG_INFO("Added benchmark: " + benchmark.name);
            } else {
                selected_benchmarks_.erase(
                    std::remove_if(selected_benchmarks_.begin(), selected_benchmarks_.end(),
                                  [&](const BenchmarkConfig& config) { 
                                      return config.name == benchmark.name; 
                                  }),
                    selected_benchmarks_.end());
                LOG_INFO("Removed benchmark: " + benchmark.name);
            }
        }
        
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("%s", benchmark.description.c_str());
            ImGui::Text("Default entities: %u", benchmark.entity_count);
            ImGui::Text("Default iterations: %u", benchmark.iterations);
            ImGui::EndTooltip();
        }
    }
    
    ImGui::EndChild();
}

void PerformanceComparisonPanel::render_benchmark_configuration() {
    ImGui::BeginChild("##selected_benchmarks", ImVec2(0, 300), true);
    
    for (usize i = 0; i < selected_benchmarks_.size(); ++i) {
        auto& config = selected_benchmarks_[i];
        
        ImGui::PushID(static_cast<int>(i));
        
        if (ImGui::CollapsingHeader(config.name.c_str())) {
            ImGui::Indent();
            
            // Configuration parameters
            int entity_count = static_cast<int>(config.entity_count);
            if (ImGui::SliderInt("Entity Count", &entity_count, 100, 10000)) {
                config.entity_count = static_cast<u32>(entity_count);
            }
            
            int iterations = static_cast<int>(config.iterations);
            if (ImGui::SliderInt("Iterations", &iterations, 10, 1000)) {
                config.iterations = static_cast<u32>(iterations);
            }
            
            ImGui::SliderFloat("Time Limit (s)", &config.time_limit, 1.0f, 60.0f);
            ImGui::Checkbox("Enable Warmup", &config.warmup_enabled);
            
            if (config.warmup_enabled) {
                int warmup_iterations = static_cast<int>(config.warmup_iterations);
                if (ImGui::SliderInt("Warmup Iterations", &warmup_iterations, 1, 50)) {
                    config.warmup_iterations = static_cast<u32>(warmup_iterations);
                }
            }
            
            ImGui::Checkbox("Show Explanation", &config.show_explanation);
            ImGui::Checkbox("Show Memory Analysis", &config.show_memory_analysis);
            ImGui::Checkbox("Show Cache Analysis", &config.show_cache_analysis);
            
            // Remove button
            if (ImGui::Button("Remove")) {
                selected_benchmarks_.erase(selected_benchmarks_.begin() + i);
                ImGui::PopID();
                break;
            }
            
            ImGui::Unindent();
        }
        
        ImGui::PopID();
    }
    
    ImGui::EndChild();
}

void PerformanceComparisonPanel::render_execution_progress() {
    ImGui::Text("Overall Progress:");
    ImGui::ProgressBar(overall_progress_, ImVec2(-1.0f, PROGRESS_BAR_HEIGHT), 
                      (std::to_string(static_cast<int>(overall_progress_ * 100)) + "%").c_str());
    
    if (benchmarks_running_) {
        ImGui::Text("Current: %s (%zu / %zu)", 
                   current_benchmark_index_ < selected_benchmarks_.size() ? 
                   selected_benchmarks_[current_benchmark_index_].name.c_str() : "Completed",
                   current_benchmark_index_ + 1, selected_benchmarks_.size());
        
        // Estimated time remaining
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration<f64>(now - benchmark_start_time_).count();
        f64 estimated_total = overall_progress_ > 0.01f ? elapsed / overall_progress_ : 0.0;
        f64 remaining = estimated_total - elapsed;
        
        if (remaining > 0) {
            ImGui::Text("Estimated time remaining: %.1f seconds", remaining);
        }
    }
}

void PerformanceComparisonPanel::render_current_benchmark_info() {
    if (current_benchmark_index_ < selected_benchmarks_.size()) {
        const auto& current = selected_benchmarks_[current_benchmark_index_];
        
        ImGui::Text("📋 Current Benchmark: %s", current.name.c_str());
        ImGui::Text("📝 Description: %s", current.description.c_str());
        ImGui::Text("🎯 Entities: %u | Iterations: %u", current.entity_count, current.iterations);
        
        if (!current_status_message_.empty()) {
            ImGui::Text("📊 Status: %s", current_status_message_.c_str());
        }
    }
}

void PerformanceComparisonPanel::render_real_time_metrics() {
    // Simulate real-time metrics during benchmark execution
    if (benchmarks_running_) {
        ImGui::Text("📈 Real-time Metrics:");
        
        // Simulated metrics
        f32 current_fps = 60.0f + std::sin(ImGui::GetTime() * 2.0f) * 10.0f;
        f32 current_memory = 150.0f + std::sin(ImGui::GetTime() * 0.5f) * 20.0f;
        f32 current_cache = 0.85f + std::sin(ImGui::GetTime() * 1.5f) * 0.1f;
        
        ImGui::Text("Frame Rate: %.1f FPS", current_fps);
        ImGui::Text("Memory Usage: %.1f MB", current_memory);
        ImGui::Text("Cache Hit Ratio: %.2f%%", current_cache * 100.0f);
        
        // Simple performance graphs
        static std::vector<f32> fps_history;
        static std::vector<f32> memory_history;
        
        fps_history.push_back(current_fps);
        memory_history.push_back(current_memory);
        
        if (fps_history.size() > 100) {
            fps_history.erase(fps_history.begin());
            memory_history.erase(memory_history.begin());
        }
        
        ImGui::PlotLines("FPS", fps_history.data(), static_cast<int>(fps_history.size()), 0, nullptr, 0.0f, 100.0f, ImVec2(0, 60));
        ImGui::PlotLines("Memory", memory_history.data(), static_cast<int>(memory_history.size()), 0, nullptr, 0.0f, 200.0f, ImVec2(0, 60));
    } else {
        ImGui::TextDisabled("No active benchmarks");
    }
}

void PerformanceComparisonPanel::render_execution_controls() {
    if (benchmarks_running_) {
        if (ImGui::Button("Pause Benchmarks")) {
            // Implement pause functionality
            LOG_INFO("Pausing benchmarks");
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Stop Benchmarks")) {
            stop_benchmark_suite();
        }
    } else {
        if (ImGui::Button("Start Benchmarks") && !selected_benchmarks_.empty()) {
            start_benchmark_suite();
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Reset Configuration")) {
            // Reset all benchmark configurations to defaults
            for (auto& config : selected_benchmarks_) {
                config.entity_count = 1000;
                config.iterations = 100;
                config.time_limit = 5.0;
                config.warmup_enabled = true;
                config.warmup_iterations = 10;
            }
        }
    }
}

void PerformanceComparisonPanel::render_results_overview() {
    if (benchmark_results_.empty()) {
        ImGui::TextDisabled("No benchmark results available");
        return;
    }
    
    ImGui::Text("📊 Results Overview (%zu benchmarks)", benchmark_results_.size());
    
    // Summary statistics
    f64 total_time = 0.0;
    f64 best_performance = std::numeric_limits<f64>::max();
    f64 worst_performance = 0.0;
    std::string best_benchmark, worst_benchmark;
    
    for (const auto& [name, result] : benchmark_results_) {
        total_time += result.average_time_ms;
        
        if (result.average_time_ms < best_performance) {
            best_performance = result.average_time_ms;
            best_benchmark = name;
        }
        
        if (result.average_time_ms > worst_performance) {
            worst_performance = result.average_time_ms;
            worst_benchmark = name;
        }
    }
    
    ImGui::Text("Total Benchmark Time: %.2f ms", total_time);
    if (!best_benchmark.empty()) {
        ImGui::Text("Best Performance: %s (%.2f ms)", best_benchmark.c_str(), best_performance);
        ImGui::Text("Worst Performance: %s (%.2f ms)", worst_benchmark.c_str(), worst_performance);
    }
    
    // Baseline selection
    ImGui::Text("Baseline for Comparison:");
    if (ImGui::BeginCombo("##baseline", results_analysis_.selected_baseline_.c_str())) {
        for (const auto& [name, result] : benchmark_results_) {
            bool is_selected = (results_analysis_.selected_baseline_ == name);
            if (ImGui::Selectable(name.c_str(), is_selected)) {
                results_analysis_.selected_baseline_ = name;
                generate_comparisons();
            }
        }
        ImGui::EndCombo();
    }
}

void PerformanceComparisonPanel::render_comparison_charts() {
    if (benchmark_results_.size() < 2) {
        ImGui::TextDisabled("Need at least 2 benchmark results for comparison");
        return;
    }
    
    // Create data for chart rendering
    std::vector<BenchmarkResult> results_vector;
    for (const auto& [name, result] : benchmark_results_) {
        results_vector.push_back(result);
    }
    
    // Render chart based on selected type
    switch (results_analysis_.chart_type_) {
        case ResultsAnalysis::ChartType::BarChart:
            render_bar_chart(results_vector, results_analysis_.chart_height_);
            break;
        case ResultsAnalysis::ChartType::LineChart:
            if (!performance_graphs_.empty()) {
                render_line_chart(performance_graphs_[0], results_analysis_.chart_height_);
            }
            break;
        case ResultsAnalysis::ChartType::ScatterPlot:
            render_scatter_plot(results_vector, results_analysis_.chart_height_);
            break;
        case ResultsAnalysis::ChartType::HeatMap:
            render_heatmap(active_comparisons_, results_analysis_.chart_height_);
            break;
        case ResultsAnalysis::ChartType::RadarChart:
            if (!results_vector.empty()) {
                render_radar_chart(results_vector[0], results_analysis_.chart_height_);
            }
            break;
    }
}

void PerformanceComparisonPanel::render_detailed_metrics() {
    ImGui::BeginChild("##detailed_metrics", ImVec2(0, 300), true);
    
    for (const auto& [name, result] : benchmark_results_) {
        if (ImGui::CollapsingHeader(name.c_str())) {
            ImGui::Indent();
            
            ImGui::Text("Performance Metrics:");
            ImGui::BulletText("Average Time: %.3f ms", result.average_time_ms);
            ImGui::BulletText("Min Time: %.3f ms", result.min_time_ms);
            ImGui::BulletText("Max Time: %.3f ms", result.max_time_ms);
            ImGui::BulletText("Std Deviation: %.3f ms", result.std_deviation_ms);
            ImGui::BulletText("Operations/sec: %llu", result.operations_per_second);
            
            ImGui::Text("Memory Metrics:");
            ImGui::BulletText("Memory Usage: %s", format_memory_size(result.memory_usage_bytes).c_str());
            ImGui::BulletText("Peak Memory: %s", format_memory_size(result.peak_memory_bytes).c_str());
            ImGui::BulletText("Allocations: %u", result.allocations_count);
            ImGui::BulletText("Cache Hit Ratio: %.2f%%", result.cache_hit_ratio * 100.0f);
            
            if (!result.performance_category.empty()) {
                ImGui::Text("Category: %s", result.performance_category.c_str());
            }
            
            ImGui::Unindent();
        }
    }
    
    ImGui::EndChild();
}

void PerformanceComparisonPanel::render_performance_insights() {
    ImGui::BeginChild("##performance_insights", ImVec2(0, 300), true);
    
    if (benchmark_results_.empty()) {
        ImGui::TextDisabled("No insights available");
        ImGui::EndChild();
        return;
    }
    
    ImGui::Text("💡 Key Insights:");
    
    // Generate some example insights
    for (const auto& [name, result] : benchmark_results_) {
        if (!result.insights.empty()) {
            ImGui::Text("📊 %s:", name.c_str());
            ImGui::Indent();
            for (const auto& insight : result.insights) {
                ImGui::BulletText("%s", insight.c_str());
            }
            ImGui::Unindent();
        }
    }
    
    // Render recommendations
    render_recommendations();
    
    ImGui::EndChild();
}

void PerformanceComparisonPanel::render_recommendations() {
    ImGui::Separator();
    ImGui::Text("🎯 Recommendations:");
    
    for (const auto& [name, result] : benchmark_results_) {
        if (!result.recommendations.empty()) {
            ImGui::Text("⚙️ %s:", name.c_str());
            ImGui::Indent();
            for (const auto& recommendation : result.recommendations) {
                ImGui::BulletText("%s", recommendation.c_str());
            }
            ImGui::Unindent();
        }
    }
}

// Chart rendering methods

void PerformanceComparisonPanel::render_bar_chart(const std::vector<BenchmarkResult>& results, f32 height) {
    if (results.empty()) return;
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    canvas_size.y = height;
    
    // Background
    draw_list->AddRectFilled(canvas_pos, 
                            ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), 
                            IM_COL32(25, 25, 30, 255));
    
    if (viz_settings_.show_grid_) {
        // Draw grid lines
        for (int i = 1; i < 5; ++i) {
            f32 y = canvas_pos.y + (canvas_size.y * i / 5);
            draw_list->AddLine(ImVec2(canvas_pos.x, y), 
                              ImVec2(canvas_pos.x + canvas_size.x, y), 
                              IM_COL32(60, 60, 70, 255));
        }
    }
    
    // Draw bars
    f32 bar_width = (canvas_size.x - 40.0f) / static_cast<f32>(results.size());
    f64 max_value = 0.0;
    
    // Find max value for scaling
    for (const auto& result : results) {
        max_value = std::max(max_value, result.average_time_ms);
    }
    
    if (max_value > 0.0) {
        for (usize i = 0; i < results.size(); ++i) {
            const auto& result = results[i];
            
            f32 x = canvas_pos.x + 20.0f + i * bar_width;
            f32 bar_height = static_cast<f32>((result.average_time_ms / max_value) * (canvas_size.y - 20.0f));
            f32 y = canvas_pos.y + canvas_size.y - bar_height - 10.0f;
            
            // Determine bar color based on performance
            u32 bar_color = get_performance_color(result.performance_category);
            
            // Draw bar
            draw_list->AddRectFilled(ImVec2(x, y), 
                                    ImVec2(x + bar_width - 5.0f, canvas_pos.y + canvas_size.y - 10.0f), 
                                    bar_color);
            
            // Draw value on bar if enabled
            if (viz_settings_.show_values_on_bars_) {
                std::string value_text = format_time_measurement(result.average_time_ms);
                ImVec2 text_size = ImGui::CalcTextSize(value_text.c_str());
                draw_list->AddText(ImVec2(x + (bar_width - text_size.x) * 0.5f, y - text_size.y - 2.0f), 
                                  IM_COL32(255, 255, 255, 255), value_text.c_str());
            }
            
            // Draw benchmark name
            std::string name = result.benchmark_name;
            if (name.length() > 12) name = name.substr(0, 9) + "...";
            ImVec2 name_size = ImGui::CalcTextSize(name.c_str());
            draw_list->AddText(ImVec2(x + (bar_width - name_size.x) * 0.5f, canvas_pos.y + canvas_size.y - name_size.y), 
                              IM_COL32(200, 200, 200, 255), name.c_str());
        }
    }
    
    // Invisible button to capture area
    ImGui::InvisibleButton("##bar_chart", canvas_size);
    
    // Handle tooltips
    if (ImGui::IsItemHovered()) {
        ImVec2 mouse_pos = ImGui::GetMousePos();
        f32 relative_x = mouse_pos.x - canvas_pos.x - 20.0f;
        int bar_index = static_cast<int>(relative_x / bar_width);
        
        if (bar_index >= 0 && bar_index < static_cast<int>(results.size())) {
            const auto& result = results[bar_index];
            ImGui::BeginTooltip();
            ImGui::Text("%s", result.benchmark_name.c_str());
            ImGui::Text("Average: %.3f ms", result.average_time_ms);
            ImGui::Text("Min: %.3f ms", result.min_time_ms);
            ImGui::Text("Max: %.3f ms", result.max_time_ms);
            ImGui::Text("Memory: %s", format_memory_size(result.memory_usage_bytes).c_str());
            ImGui::EndTooltip();
        }
    }
}

void PerformanceComparisonPanel::render_line_chart(const PerformanceGraph& graph, f32 height) {
    if (graph.data_points_.empty()) return;
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    canvas_size.y = height;
    
    // Background
    draw_list->AddRectFilled(canvas_pos, 
                            ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), 
                            IM_COL32(25, 25, 30, 255));
    
    // Draw data points and connect with lines
    f32 x_step = canvas_size.x / static_cast<f32>(graph.data_points_.size() - 1);
    f32 y_range = graph.max_value_ - graph.min_value_;
    
    if (y_range > 0.0f) {
        for (usize i = 1; i < graph.data_points_.size(); ++i) {
            f32 x1 = canvas_pos.x + (i - 1) * x_step;
            f32 x2 = canvas_pos.x + i * x_step;
            f32 y1 = canvas_pos.y + canvas_size.y - ((graph.data_points_[i-1] - graph.min_value_) / y_range) * canvas_size.y;
            f32 y2 = canvas_pos.y + canvas_size.y - ((graph.data_points_[i] - graph.min_value_) / y_range) * canvas_size.y;
            
            draw_list->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), graph.color_, 2.0f);
        }
        
        // Draw data points
        for (usize i = 0; i < graph.data_points_.size(); ++i) {
            f32 x = canvas_pos.x + i * x_step;
            f32 y = canvas_pos.y + canvas_size.y - ((graph.data_points_[i] - graph.min_value_) / y_range) * canvas_size.y;
            
            draw_list->AddCircleFilled(ImVec2(x, y), 3.0f, graph.color_);
        }
    }
    
    ImGui::InvisibleButton("##line_chart", canvas_size);
}

// Stub implementations for remaining chart types
void PerformanceComparisonPanel::render_scatter_plot(const std::vector<BenchmarkResult>& results, f32 height) {
    ImGui::Text("Scatter plot visualization will be implemented here");
    ImGui::Dummy(ImVec2(0, height));
}

void PerformanceComparisonPanel::render_radar_chart(const BenchmarkResult& result, f32 height) {
    ImGui::Text("Radar chart visualization will be implemented here");
    ImGui::Dummy(ImVec2(0, height));
}

void PerformanceComparisonPanel::render_heatmap(const std::vector<BenchmarkComparison>& comparisons, f32 height) {
    ImGui::Text("Heatmap visualization will be implemented here");
    ImGui::Dummy(ImVec2(0, height));
}

// Control and management methods

void PerformanceComparisonPanel::initialize_default_benchmarks() {
    // Entity Iteration benchmarks
    available_benchmarks_.emplace_back(BenchmarkType::EntityIteration, 
                                      "Basic Entity Iteration",
                                      "Measures performance of iterating through entities with basic components");
    
    available_benchmarks_.emplace_back(BenchmarkType::EntityIteration,
                                      "Dense Entity Iteration", 
                                      "Measures performance with tightly packed entity data");
    
    // Component Access benchmarks
    available_benchmarks_.emplace_back(BenchmarkType::ComponentAccess,
                                      "Sequential Component Access",
                                      "Measures performance of accessing components in sequential order");
    
    available_benchmarks_.emplace_back(BenchmarkType::ComponentAccess,
                                      "Random Component Access", 
                                      "Measures performance of random component access patterns");
    
    // Memory Layout benchmarks
    available_benchmarks_.emplace_back(BenchmarkType::MemoryLayoutComparison,
                                      "SoA vs AoS Comparison",
                                      "Compares Structure of Arrays vs Array of Structures performance");
    
    available_benchmarks_.emplace_back(BenchmarkType::MemoryLayoutComparison,
                                      "Cache-Friendly Layout",
                                      "Measures performance of cache-optimized data layouts");
    
    // System Execution benchmarks
    available_benchmarks_.emplace_back(BenchmarkType::SystemExecution,
                                      "Single System Execution",
                                      "Measures performance of individual system execution");
    
    available_benchmarks_.emplace_back(BenchmarkType::SystemExecution,
                                      "Multiple System Pipeline",
                                      "Measures performance of system execution pipeline");
    
    LOG_INFO("Initialized " + std::to_string(available_benchmarks_.size()) + " default benchmarks");
}

void PerformanceComparisonPanel::initialize_educational_content() {
    // Initialize explanations for each benchmark type
    educational_content_.benchmark_explanations_[BenchmarkType::EntityIteration] = 
        "Entity iteration is fundamental to ECS performance. This benchmark measures how quickly "
        "systems can iterate through entities and their components.";
    
    educational_content_.benchmark_explanations_[BenchmarkType::ComponentAccess] = 
        "Component access patterns significantly impact performance. Sequential access is generally "
        "faster than random access due to CPU cache behavior.";
    
    educational_content_.benchmark_explanations_[BenchmarkType::MemoryLayoutComparison] = 
        "Memory layout affects cache performance. Structure of Arrays (SoA) often outperforms "
        "Array of Structures (AoS) for component-wise operations.";
    
    // Initialize learning sequence
    educational_content_.learning_sequence_ = {
        BenchmarkType::EntityIteration,
        BenchmarkType::ComponentAccess,
        BenchmarkType::MemoryLayoutComparison,
        BenchmarkType::SystemExecution,
        BenchmarkType::CacheBehaviorAnalysis,
        BenchmarkType::ScalingAnalysis
    };
    
    LOG_INFO("Initialized educational content system");
}

void PerformanceComparisonPanel::start_benchmark_suite() {
    if (selected_benchmarks_.empty()) {
        LOG_ERROR("No benchmarks selected");
        return;
    }
    
    benchmarks_running_ = true;
    current_benchmark_index_ = 0;
    overall_progress_ = 0.0f;
    benchmark_start_time_ = std::chrono::steady_clock::now();
    current_status_message_ = "Starting benchmark suite...";
    
    LOG_INFO("Started benchmark suite with " + std::to_string(selected_benchmarks_.size()) + " benchmarks");
}

void PerformanceComparisonPanel::stop_benchmark_suite() {
    benchmarks_running_ = false;
    current_status_message_ = "Benchmark suite stopped";
    
    LOG_INFO("Stopped benchmark suite");
}

void PerformanceComparisonPanel::reset_results() {
    benchmark_results_.clear();
    active_comparisons_.clear();
    
    LOG_INFO("Reset all benchmark results");
}

void PerformanceComparisonPanel::generate_comparisons() {
    if (benchmark_results_.size() < 2 || results_analysis_.selected_baseline_.empty()) {
        return;
    }
    
    auto baseline_it = benchmark_results_.find(results_analysis_.selected_baseline_);
    if (baseline_it == benchmark_results_.end()) {
        return;
    }
    
    active_comparisons_.clear();
    
    for (const auto& [name, result] : benchmark_results_) {
        if (name != results_analysis_.selected_baseline_) {
            BenchmarkComparison comparison = compare_results(baseline_it->second, result);
            active_comparisons_.push_back(comparison);
        }
    }
    
    LOG_INFO("Generated " + std::to_string(active_comparisons_.size()) + " benchmark comparisons");
}

BenchmarkComparison PerformanceComparisonPanel::compare_results(const BenchmarkResult& baseline, const BenchmarkResult& comparison) {
    BenchmarkComparison comp(baseline, comparison);
    
    // Calculate performance improvement (negative = slower)
    comp.performance_improvement = ((baseline.average_time_ms - comparison.average_time_ms) / baseline.average_time_ms) * 100.0;
    
    // Calculate memory difference
    comp.memory_difference = static_cast<f64>(comparison.memory_usage_bytes) - static_cast<f64>(baseline.memory_usage_bytes);
    
    // Calculate cache improvement
    comp.cache_improvement = (comparison.cache_hit_ratio - baseline.cache_hit_ratio) * 100.0;
    
    // Generate summary
    std::ostringstream summary;
    if (comp.performance_improvement > 5.0) {
        summary << "Significantly faster than baseline";
    } else if (comp.performance_improvement > 0.0) {
        summary << "Slightly faster than baseline";
    } else if (comp.performance_improvement > -5.0) {
        summary << "Similar performance to baseline";
    } else {
        summary << "Slower than baseline";
    }
    
    comp.summary = summary.str();
    
    // Generate metric comparisons
    comp.metric_comparisons.emplace_back("Performance", comp.performance_improvement);
    comp.metric_comparisons.emplace_back("Memory Usage", (comp.memory_difference / baseline.memory_usage_bytes) * 100.0);
    comp.metric_comparisons.emplace_back("Cache Hit Ratio", comp.cache_improvement);
    
    return comp;
}

// Event handlers

void PerformanceComparisonPanel::on_benchmark_completed(const BenchmarkResult& result) {
    benchmark_results_[result.benchmark_name] = result;
    current_benchmark_index_++;
    
    LOG_INFO("Completed benchmark: " + result.benchmark_name);
}

void PerformanceComparisonPanel::on_benchmark_suite_completed() {
    benchmarks_running_ = false;
    overall_progress_ = 1.0f;
    current_status_message_ = "Benchmark suite completed";
    current_mode_ = ComparisonMode::ResultsAnalysis;
    
    // Generate analysis
    analyze_benchmark_results();
    generate_comparisons();
    
    LOG_INFO("Benchmark suite completed");
}

void PerformanceComparisonPanel::analyze_benchmark_results() {
    for (auto& [name, result] : benchmark_results_) {
        calculate_performance_insights(result);
        generate_recommendations(result);
    }
}

void PerformanceComparisonPanel::calculate_performance_insights(BenchmarkResult& result) {
    // Generate some example insights based on the results
    if (result.average_time_ms < 1.0) {
        result.performance_category = "Excellent";
        result.insights.push_back("Very fast execution time indicates efficient implementation");
    } else if (result.average_time_ms < 5.0) {
        result.performance_category = "Good";
        result.insights.push_back("Good performance with room for optimization");
    } else if (result.average_time_ms < 10.0) {
        result.performance_category = "Fair";
        result.insights.push_back("Moderate performance, consider optimizations");
    } else {
        result.performance_category = "Poor";
        result.insights.push_back("Performance issues detected, optimization needed");
    }
    
    // Memory insights
    if (result.memory_usage_bytes > 100 * 1024 * 1024) { // > 100MB
        result.insights.push_back("High memory usage detected");
    }
    
    // Cache insights
    if (result.cache_hit_ratio < 0.8f) {
        result.insights.push_back("Low cache hit ratio suggests poor memory access patterns");
    }
}

void PerformanceComparisonPanel::generate_recommendations(BenchmarkResult& result) {
    if (result.performance_category == "Poor") {
        result.recommendations.push_back("Consider using more cache-friendly data structures");
        result.recommendations.push_back("Reduce memory allocations in hot paths");
    }
    
    if (result.cache_hit_ratio < 0.8f) {
        result.recommendations.push_back("Optimize memory access patterns for better cache usage");
        result.recommendations.push_back("Consider data structure reorganization (SoA vs AoS)");
    }
    
    if (result.memory_usage_bytes > 100 * 1024 * 1024) {
        result.recommendations.push_back("Investigate memory usage patterns");
        result.recommendations.push_back("Consider more efficient data storage methods");
    }
}

// Utility methods

std::string PerformanceComparisonPanel::format_time_measurement(f64 time_ms) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << time_ms << " ms";
    return oss.str();
}

std::string PerformanceComparisonPanel::format_memory_size(usize bytes) const {
    if (bytes >= 1024 * 1024 * 1024) {
        return std::to_string(bytes / (1024 * 1024 * 1024)) + " GB";
    } else if (bytes >= 1024 * 1024) {
        return std::to_string(bytes / (1024 * 1024)) + " MB";
    } else if (bytes >= 1024) {
        return std::to_string(bytes / 1024) + " KB";
    } else {
        return std::to_string(bytes) + " B";
    }
}

std::string PerformanceComparisonPanel::get_performance_category(f64 value, f64 baseline) const {
    f64 ratio = value / baseline;
    if (ratio < 0.8) return "Excellent";
    else if (ratio < 1.1) return "Good";
    else if (ratio < 1.5) return "Fair";
    else return "Poor";
}

u32 PerformanceComparisonPanel::get_performance_color(const std::string& category) const {
    if (category == "Excellent") return viz_settings_.excellent_color_;
    else if (category == "Good") return viz_settings_.good_color_;
    else if (category == "Fair") return viz_settings_.fair_color_;
    else return viz_settings_.poor_color_;
}

// Stub implementations for remaining demo components
void PerformanceComparisonPanel::render_demo_selection() {
    ImGui::Text("Available Performance Demos:");
    
    const std::vector<std::pair<std::string, std::string>> demos = {
        {"entity_scaling", "Entity Count Scaling"},
        {"memory_layout", "Memory Layout Comparison"},
        {"cache_behavior", "Cache Behavior Analysis"},
        {"system_pipeline", "System Pipeline Performance"}
    };
    
    for (const auto& [id, name] : demos) {
        if (ImGui::Button(name.c_str())) {
            start_interactive_demo(id);
        }
    }
}

void PerformanceComparisonPanel::start_interactive_demo(const std::string& demo_id) {
    interactive_demo_.current_demo_id_ = demo_id;
    interactive_demo_.demo_active_ = true;
    interactive_demo_.real_time_measurements_.clear();
    
    LOG_INFO("Started interactive demo: " + demo_id);
}

void PerformanceComparisonPanel::measure_demo_performance() {
    if (!interactive_demo_.demo_active_) return;
    
    // Simulate real-time performance measurement
    f64 simulated_measurement = 16.67 + std::sin(ImGui::GetTime()) * 2.0; // ~60fps with variation
    interactive_demo_.real_time_measurements_.push_back(simulated_measurement);
    
    // Keep only recent measurements
    if (interactive_demo_.real_time_measurements_.size() > 300) {
        interactive_demo_.real_time_measurements_.erase(interactive_demo_.real_time_measurements_.begin());
    }
}

// Additional stub implementations for remaining render methods
void PerformanceComparisonPanel::render_demo_controls() {
    ImGui::Text("Demo: %s", interactive_demo_.current_demo_id_.c_str());
    
    if (ImGui::Button("Stop Demo")) {
        stop_interactive_demo();
    }
}

void PerformanceComparisonPanel::render_real_time_visualization() {
    if (!interactive_demo_.real_time_measurements_.empty()) {
        ImGui::PlotLines("Performance", 
                        reinterpret_cast<const f32*>(interactive_demo_.real_time_measurements_.data()),
                        static_cast<int>(interactive_demo_.real_time_measurements_.size()),
                        0, "Frame Time (ms)", 10.0f, 25.0f, ImVec2(0, 120));
    }
}

void PerformanceComparisonPanel::render_parameter_sliders() {
    ImGui::Text("Parameters:");
    
    // Example parameter sliders
    static f32 entity_count_factor = 1.0f;
    static f32 complexity_factor = 1.0f;
    
    if (ImGui::SliderFloat("Entity Count", &entity_count_factor, 0.1f, 5.0f)) {
        interactive_demo_.demo_parameters_["entity_count"] = entity_count_factor;
    }
    
    if (ImGui::SliderFloat("Complexity", &complexity_factor, 0.5f, 3.0f)) {
        interactive_demo_.demo_parameters_["complexity"] = complexity_factor;
    }
}

void PerformanceComparisonPanel::render_learning_path() {
    ImGui::Text("📚 Performance Learning Path:");
    
    for (usize i = 0; i < educational_content_.learning_sequence_.size(); ++i) {
        bool is_current = (i == educational_content_.current_learning_step_);
        bool is_completed = (i < educational_content_.current_learning_step_);
        
        ImVec4 color = is_completed ? ImVec4(0.2f, 0.8f, 0.3f, 1.0f) :
                       is_current ? ImVec4(0.2f, 0.7f, 1.0f, 1.0f) :
                       ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
        
        ImGui::TextColored(color, "%zu. %s %s", 
                          i + 1,
                          is_completed ? "✅" : is_current ? "🔄" : "⏸️",
                          "Performance Concept"); // Would show actual concept name
    }
}

void PerformanceComparisonPanel::render_guided_tutorial() {
    ImGui::Text("📖 Guided Tutorial");
    ImGui::Separator();
    
    ImGui::Text("Step %d of %zu", 
               educational_content_.current_tutorial_step_ + 1,
               educational_content_.tutorial_steps_.size());
    
    if (educational_content_.current_tutorial_step_ < static_cast<int>(educational_content_.tutorial_steps_.size())) {
        ImGui::TextWrapped("Tutorial content would go here...");
        
        if (ImGui::Button("Next Step")) {
            advance_tutorial_step();
        }
    }
}

void PerformanceComparisonPanel::render_concept_explanations() {
    ImGui::Text("📖 Performance Concepts");
    
    // Show explanations for current benchmark types
    for (const auto& [type, explanation] : educational_content_.benchmark_explanations_) {
        if (ImGui::CollapsingHeader(("Concept: " + std::to_string(static_cast<int>(type))).c_str())) {
            ImGui::TextWrapped("%s", explanation.c_str());
        }
    }
}

void PerformanceComparisonPanel::render_context_help() {
    if (!educational_content_.context_help_enabled_) return;
    
    ImGui::Separator();
    ImGui::Text("❓ Context Help");
    ImGui::TextWrapped("Performance comparison helps you understand how different ECS design choices affect execution speed, memory usage, and cache behavior.");
}

void PerformanceComparisonPanel::advance_tutorial_step() {
    if (educational_content_.current_tutorial_step_ < static_cast<int>(educational_content_.tutorial_steps_.size()) - 1) {
        educational_content_.current_tutorial_step_++;
        LOG_INFO("Advanced to tutorial step " + std::to_string(educational_content_.current_tutorial_step_));
    }
}

void PerformanceComparisonPanel::start_guided_learning() {
    educational_content_.guided_mode_enabled_ = true;
    educational_content_.current_learning_step_ = 0;
    educational_content_.current_tutorial_step_ = 0;
    
    LOG_INFO("Started guided learning mode");
}

void PerformanceComparisonPanel::stop_interactive_demo() {
    interactive_demo_.demo_active_ = false;
    interactive_demo_.current_demo_id_.clear();
    interactive_demo_.real_time_measurements_.clear();
    
    LOG_INFO("Stopped interactive demo");
}

// Additional stub implementations for data export
void PerformanceComparisonPanel::export_results(const std::string& filename, const std::string& format) {
    LOG_INFO("Exporting results to: " + filename + " (format: " + format + ")");
    // Implementation would serialize benchmark_results_ to specified format
}

// Remaining stub method implementations
void PerformanceComparisonPanel::render_benchmark_preview() { }

BenchmarkResult PerformanceComparisonPanel::get_result(const std::string& name) const {
    auto it = benchmark_results_.find(name);
    return it != benchmark_results_.end() ? it->second : BenchmarkResult{};
}

// Configuration methods
void PerformanceComparisonPanel::add_benchmark(const BenchmarkConfig& config) {
    selected_benchmarks_.push_back(config);
}

void PerformanceComparisonPanel::set_chart_type(ResultsAnalysis::ChartType type) {
    results_analysis_.chart_type_ = type;
}

} // namespace ecscope::ui