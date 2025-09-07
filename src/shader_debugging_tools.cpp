#include "../include/ecscope/shader_debugging_tools.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <regex>
#include <numeric>

// Platform-specific includes for GPU debugging
#ifdef _WIN32
    #include <windows.h>
    #include <GL/gl.h>
    #include <GL/wglext.h>
#elif defined(__linux__)
    #include <GL/gl.h>
    #include <GL/glext.h>
#endif

namespace ecscope::renderer::shader_debugging {

//=============================================================================
// Shader Performance Profiler Implementation
//=============================================================================

ShaderPerformanceProfiler::ShaderPerformanceProfiler(const ProfilingConfig& config)
    : config_(config), current_frame_number_(0) {
    
    if (config_.enable_gpu_timing) {
        init_gpu_timing();
    }
    
    // Reserve space for frame history
    frame_history_.reserve(config_.history_frame_count);
}

ShaderPerformanceProfiler::~ShaderPerformanceProfiler() {
    if (config_.enable_gpu_timing) {
        cleanup_gpu_timing();
    }
}

void ShaderPerformanceProfiler::begin_session(const std::string& session_name) {
    session_name_ = session_name;
    session_start_ = std::chrono::steady_clock::now();
    session_active_ = true;
    
    // Clear previous data
    frame_history_.clear();
    completed_events_.clear();
    current_frame_number_ = 0;
}

void ShaderPerformanceProfiler::end_session() {
    session_active_ = false;
    
    if (config_.auto_generate_reports) {
        std::string report = generate_performance_report();
        std::ofstream file("profiling_report_" + session_name_ + ".txt");
        file << report;
        file.close();
    }
}

void ShaderPerformanceProfiler::begin_event(const std::string& event_name) {
    if (!session_active_) return;
    
    auto now = std::chrono::steady_clock::now();
    active_events_[event_name] = now;
    
    if (config_.enable_gpu_timing) {
        u32 query_id = create_gpu_timer_query();
        active_queries_[event_name] = query_id;
        
        // Begin GPU timing
        #ifdef GL_TIME_ELAPSED
        glBeginQuery(GL_TIME_ELAPSED, query_id);
        #endif
    }
}

void ShaderPerformanceProfiler::end_event(const std::string& event_name) {
    if (!session_active_) return;
    
    auto now = std::chrono::steady_clock::now();
    
    auto it = active_events_.find(event_name);
    if (it != active_events_.end()) {
        GPUProfilerEvent event;
        event.name = event_name;
        event.cpu_start = it->second;
        event.cpu_end = now;
        
        if (config_.enable_gpu_timing) {
            auto query_it = active_queries_.find(event_name);
            if (query_it != active_queries_.end()) {
                event.query_id = query_it->second;
                
                // End GPU timing
                #ifdef GL_TIME_ELAPSED
                glEndQuery(GL_TIME_ELAPSED);
                #endif
                
                active_queries_.erase(query_it);
            }
        }
        
        completed_events_.push_back(event);
        active_events_.erase(it);
    }
}

void ShaderPerformanceProfiler::begin_frame() {
    if (!session_active_) return;
    
    frame_start_time_ = std::chrono::steady_clock::now();
    
    PerformanceFrame frame;
    frame.frame_number = current_frame_number_;
    frame.timestamp = frame_start_time_;
    current_frame_ = frame;
    
    // Process completed GPU queries from previous frames
    for (auto& event : completed_events_) {
        if (!event.is_complete && config_.enable_gpu_timing && event.query_id != 0) {
            if (is_gpu_timer_ready(event.query_id)) {
                event.gpu_time_ms = get_gpu_timer_result(event.query_id);
                event.is_complete = true;
                available_queries_.push_back(event.query_id);
            }
        }
    }
}

void ShaderPerformanceProfiler::end_frame() {
    if (!session_active_ || !current_frame_) return;
    
    auto frame_end_time = std::chrono::steady_clock::now();
    auto frame_duration = std::chrono::duration_cast<std::chrono::microseconds>(
        frame_end_time - frame_start_time_);
    
    current_frame_->total_frame_time = frame_duration.count() / 1000.0f; // Convert to milliseconds
    
    // Calculate CPU and GPU times
    current_frame_->cpu_time = current_frame_->total_frame_time;
    current_frame_->gpu_time = 0.0f;
    
    for (const auto& event : completed_events_) {
        if (event.is_complete) {
            current_frame_->gpu_time += event.gpu_time_ms;
        }
        current_frame_->events.push_back(event);
    }
    
    // Adjust CPU time (subtract GPU time if it overlaps)
    if (current_frame_->gpu_time > 0.0f) {
        current_frame_->cpu_time = std::max(0.0f, current_frame_->cpu_time - current_frame_->gpu_time);
    }
    
    // Store frame and maintain history
    frame_history_.push_back(*current_frame_);
    if (frame_history_.size() > config_.history_frame_count) {
        frame_history_.erase(frame_history_.begin());
    }
    
    // Clear completed events for next frame
    completed_events_.clear();
    current_frame_.reset();
    current_frame_number_++;
    
    // Check for performance warnings
    if (current_frame_->total_frame_time > config_.performance_warning_threshold) {
        detect_performance_issues();
    }
}

void ShaderPerformanceProfiler::profile_shader_execution(shader_runtime::ShaderRuntimeManager::ShaderHandle handle,
                                                        const std::string& pass_name) {
    if (!session_active_) return;
    
    std::string event_name = "Shader_" + std::to_string(handle) + "_" + pass_name;
    begin_event(event_name);
    
    // This would be called at the end of shader execution
    // For now, we'll simulate it ending immediately for demonstration
    // In practice, you'd call end_event after the draw call
    end_event(event_name);
}

ShaderPerformanceProfiler::PerformanceStatistics 
ShaderPerformanceProfiler::calculate_statistics(u32 frame_count) const {
    PerformanceStatistics stats;
    
    if (frame_history_.empty()) return stats;
    
    u32 count = std::min(frame_count, static_cast<u32>(frame_history_.size()));
    auto begin_it = frame_history_.end() - count;
    
    // Calculate frame time statistics
    std::vector<f32> frame_times;
    f32 total_cpu_time = 0.0f, total_gpu_time = 0.0f;
    u32 total_draw_calls = 0, max_draw_calls = 0;
    usize total_memory = 0, max_memory = 0;
    
    for (auto it = begin_it; it != frame_history_.end(); ++it) {
        frame_times.push_back(it->total_frame_time);
        total_cpu_time += it->cpu_time;
        total_gpu_time += it->gpu_time;
        total_draw_calls += it->draw_calls;
        max_draw_calls = std::max(max_draw_calls, it->draw_calls);
        total_memory += it->memory_usage;
        max_memory = std::max(max_memory, it->memory_usage);
    }
    
    // Frame time statistics
    stats.average_frame_time = std::accumulate(frame_times.begin(), frame_times.end(), 0.0f) / count;
    stats.min_frame_time = *std::min_element(frame_times.begin(), frame_times.end());
    stats.max_frame_time = *std::max_element(frame_times.begin(), frame_times.end());
    
    // Calculate variance
    f32 variance_sum = 0.0f;
    for (f32 time : frame_times) {
        f32 diff = time - stats.average_frame_time;
        variance_sum += diff * diff;
    }
    stats.frame_time_variance = variance_sum / count;
    
    // Other averages
    stats.average_cpu_time = total_cpu_time / count;
    stats.average_gpu_time = total_gpu_time / count;
    stats.average_draw_calls = total_draw_calls / count;
    stats.peak_draw_calls = max_draw_calls;
    stats.average_memory_usage = total_memory / count;
    stats.peak_memory_usage = max_memory;
    
    // Count performance issues
    for (f32 time : frame_times) {
        if (time > config_.performance_warning_threshold) {
            stats.frame_drops++;
        }
    }
    
    // Generate optimization suggestions
    if (stats.average_frame_time > 16.67f) {
        stats.optimization_suggestions.push_back("Frame time exceeds 60fps target - consider optimizations");
    }
    
    if (stats.average_draw_calls > 1000) {
        stats.optimization_suggestions.push_back("High draw call count - consider batching");
    }
    
    if (stats.average_gpu_time > stats.average_cpu_time * 2.0f) {
        stats.bottlenecks.push_back("GPU-bound - optimize shaders and reduce overdraw");
    } else if (stats.average_cpu_time > stats.average_gpu_time * 2.0f) {
        stats.bottlenecks.push_back("CPU-bound - optimize game logic and reduce draw calls");
    }
    
    return stats;
}

std::vector<ShaderPerformanceProfiler::HotSpot> 
ShaderPerformanceProfiler::identify_hot_spots(u32 frame_count) const {
    std::vector<HotSpot> hot_spots;
    std::unordered_map<std::string, HotSpot> event_totals;
    
    u32 count = std::min(frame_count, static_cast<u32>(frame_history_.size()));
    auto begin_it = frame_history_.end() - count;
    
    f32 total_frame_time = 0.0f;
    
    // Aggregate event data
    for (auto it = begin_it; it != frame_history_.end(); ++it) {
        total_frame_time += it->total_frame_time;
        
        for (const auto& event : it->events) {
            if (!event.is_complete) continue;
            
            auto& hot_spot = event_totals[event.name];
            hot_spot.name = event.name;
            hot_spot.total_time += event.gpu_time_ms;
            hot_spot.call_count++;
            hot_spot.category = "GPU";
        }
    }
    
    // Calculate percentages and averages
    for (auto& [name, hot_spot] : event_totals) {
        hot_spot.average_time = hot_spot.total_time / hot_spot.call_count;
        hot_spot.percentage_of_frame = (hot_spot.total_time / total_frame_time) * 100.0f;
    }
    
    // Convert to vector and sort
    for (const auto& [name, hot_spot] : event_totals) {
        hot_spots.push_back(hot_spot);
    }
    
    std::sort(hot_spots.begin(), hot_spots.end());
    
    return hot_spots;
}

std::string ShaderPerformanceProfiler::generate_performance_report(u32 frame_count) const {
    std::ostringstream report;
    
    report << "Shader Performance Report\n";
    report << "========================\n";
    report << "Session: " << session_name_ << "\n";
    report << "Frames Analyzed: " << std::min(frame_count, static_cast<u32>(frame_history_.size())) << "\n\n";
    
    auto stats = calculate_statistics(frame_count);
    
    report << "Frame Time Statistics:\n";
    report << "  Average: " << std::fixed << std::setprecision(2) << stats.average_frame_time << " ms\n";
    report << "  Minimum: " << stats.min_frame_time << " ms\n";
    report << "  Maximum: " << stats.max_frame_time << " ms\n";
    report << "  Variance: " << stats.frame_time_variance << "\n\n";
    
    report << "Performance Breakdown:\n";
    report << "  Average CPU Time: " << stats.average_cpu_time << " ms\n";
    report << "  Average GPU Time: " << stats.average_gpu_time << " ms\n";
    report << "  Average Draw Calls: " << stats.average_draw_calls << "\n";
    report << "  Peak Draw Calls: " << stats.peak_draw_calls << "\n\n";
    
    report << "Memory Usage:\n";
    report << "  Average: " << utils::format_memory_usage(stats.average_memory_usage) << "\n";
    report << "  Peak: " << utils::format_memory_usage(stats.peak_memory_usage) << "\n\n";
    
    // Hot spots
    auto hot_spots = identify_hot_spots(frame_count);
    report << "Performance Hot Spots:\n";
    for (size_t i = 0; i < std::min(size_t(10), hot_spots.size()); ++i) {
        const auto& hot_spot = hot_spots[i];
        report << "  " << (i + 1) << ". " << hot_spot.name 
               << " - " << hot_spot.total_time << " ms total"
               << " (" << hot_spot.percentage_of_frame << "% of frame)\n";
    }
    report << "\n";
    
    // Bottlenecks and suggestions
    if (!stats.bottlenecks.empty()) {
        report << "Identified Bottlenecks:\n";
        for (const auto& bottleneck : stats.bottlenecks) {
            report << "  - " << bottleneck << "\n";
        }
        report << "\n";
    }
    
    if (!stats.optimization_suggestions.empty()) {
        report << "Optimization Suggestions:\n";
        for (const auto& suggestion : stats.optimization_suggestions) {
            report << "  - " << suggestion << "\n";
        }
        report << "\n";
    }
    
    return report.str();
}

void ShaderPerformanceProfiler::init_gpu_timing() {
    #ifdef GL_TIME_ELAPSED
    // Create initial pool of timer queries
    for (u32 i = 0; i < 10; ++i) {
        u32 query_id = create_gpu_timer_query();
        available_queries_.push_back(query_id);
    }
    #endif
}

void ShaderPerformanceProfiler::cleanup_gpu_timing() {
    #ifdef GL_TIME_ELAPSED
    for (u32 query_id : available_queries_) {
        glDeleteQueries(1, &query_id);
    }
    available_queries_.clear();
    
    for (const auto& [name, query_id] : active_queries_) {
        glDeleteQueries(1, &query_id);
    }
    active_queries_.clear();
    #endif
}

u32 ShaderPerformanceProfiler::create_gpu_timer_query() {
    #ifdef GL_TIME_ELAPSED
    u32 query_id = 0;
    glGenQueries(1, &query_id);
    return query_id;
    #else
    return next_query_id_++;
    #endif
}

f32 ShaderPerformanceProfiler::get_gpu_timer_result(u32 query_id) {
    #ifdef GL_TIME_ELAPSED
    u64 time_elapsed = 0;
    glGetQueryObjectui64v(query_id, GL_QUERY_RESULT, &time_elapsed);
    return static_cast<f32>(time_elapsed) / 1000000.0f; // Convert nanoseconds to milliseconds
    #else
    return 0.0f; // Fallback for unsupported platforms
    #endif
}

bool ShaderPerformanceProfiler::is_gpu_timer_ready(u32 query_id) {
    #ifdef GL_TIME_ELAPSED
    i32 available = 0;
    glGetQueryObjectiv(query_id, GL_QUERY_RESULT_AVAILABLE, &available);
    return available == GL_TRUE;
    #else
    return true; // Fallback for unsupported platforms
    #endif
}

void ShaderPerformanceProfiler::detect_performance_issues() {
    // This would analyze current performance data and flag issues
    // Implementation would depend on specific performance criteria
}

//=============================================================================
// Shader Debug Overlay Implementation
//=============================================================================

ShaderDebugOverlay::ShaderDebugOverlay(const OverlayConfig& config)
    : config_(config), active_overlay_(OverlayType::VariableWatch) {
    
    // Initialize enabled overlays
    enabled_overlays_[OverlayType::VariableWatch] = config_.enable_variable_watch;
    enabled_overlays_[OverlayType::PerformanceGraph] = config_.enable_performance_graphs;
    enabled_overlays_[OverlayType::MemoryUsage] = config_.enable_memory_visualization;
    
    // Reserve space for history data
    frame_time_history_.reserve(120); // 2 seconds at 60fps
    gpu_time_history_.reserve(120);
    draw_call_history_.reserve(120);
    memory_history_.reserve(120);
}

void ShaderDebugOverlay::toggle_overlay(OverlayType type) {
    enabled_overlays_[type] = !enabled_overlays_[type];
}

bool ShaderDebugOverlay::is_overlay_enabled(OverlayType type) const {
    auto it = enabled_overlays_.find(type);
    return (it != enabled_overlays_.end()) ? it->second : false;
}

void ShaderDebugOverlay::add_watched_variable(const std::string& shader_name, 
                                             const std::string& variable_name) {
    auto& shader_vars = watched_variables_[shader_name];
    
    // Check if variable is already watched
    auto it = std::find_if(shader_vars.begin(), shader_vars.end(),
                          [&variable_name](const DebugVariable& var) {
                              return var.name == variable_name;
                          });
    
    if (it == shader_vars.end()) {
        DebugVariable debug_var;
        debug_var.name = variable_name;
        debug_var.display_name = variable_name;
        debug_var.type = DebugDataType::Unknown;
        debug_var.is_watched = true;
        shader_vars.push_back(debug_var);
    }
}

void ShaderDebugOverlay::update_performance_data(const ShaderPerformanceProfiler::PerformanceFrame& frame) {
    frame_time_history_.push_back(frame.total_frame_time);
    gpu_time_history_.push_back(frame.gpu_time);
    draw_call_history_.push_back(frame.draw_calls);
    
    // Maintain history size
    if (frame_time_history_.size() > 120) {
        frame_time_history_.erase(frame_time_history_.begin());
        gpu_time_history_.erase(gpu_time_history_.begin());
        draw_call_history_.erase(draw_call_history_.begin());
    }
}

void ShaderDebugOverlay::update_memory_data(usize total_memory, usize shader_memory, usize texture_memory) {
    total_memory_ = total_memory;
    shader_memory_ = shader_memory;
    texture_memory_ = texture_memory;
    
    memory_history_.push_back(total_memory);
    if (memory_history_.size() > 120) {
        memory_history_.erase(memory_history_.begin());
    }
}

void ShaderDebugOverlay::add_compilation_error(const std::string& shader_name, 
                                              const std::string& error_message,
                                              u32 line_number, u32 column) {
    CompilationError error;
    error.shader_name = shader_name;
    error.message = error_message;
    error.line = line_number;
    error.column = column;
    error.timestamp = std::chrono::steady_clock::now();
    
    compilation_errors_.push_back(error);
    
    // Keep only recent errors
    if (compilation_errors_.size() > 50) {
        compilation_errors_.erase(compilation_errors_.begin());
    }
}

void ShaderDebugOverlay::render_overlay() {
    if (!is_overlay_enabled(active_overlay_)) return;
    
    switch (active_overlay_) {
        case OverlayType::VariableWatch:
            render_variable_watch();
            break;
        case OverlayType::PerformanceGraph:
            render_performance_graph();
            break;
        case OverlayType::MemoryUsage:
            render_memory_usage();
            break;
        case OverlayType::CompilationErrors:
            render_compilation_errors();
            break;
        default:
            break;
    }
    
    if (educational_mode_) {
        render_educational_overlays();
    }
}

void ShaderDebugOverlay::render_variable_watch() {
    // This would integrate with your UI system (ImGui, custom UI, etc.)
    // Placeholder implementation showing the concept
    
    for (const auto& [shader_name, variables] : watched_variables_) {
        // Render shader name header
        // Render each watched variable with its current value
        for (const auto& var : variables) {
            if (var.is_watched) {
                std::string value_str = utils::debug_value_to_string(var.value);
                // Display: var.display_name + " = " + value_str
            }
        }
    }
}

void ShaderDebugOverlay::render_performance_graph() {
    if (!frame_time_history_.empty()) {
        // Render frame time graph
        render_graph(frame_time_history_, "Frame Time (ms)", 
                    config_.overlay_position, {config_.overlay_size[0], 100.0f},
                    0.0f, 33.33f, config_.graph_color);
        
        // Render GPU time graph below
        std::array<f32, 2> gpu_pos = {config_.overlay_position[0], config_.overlay_position[1] + 120.0f};
        render_graph(gpu_time_history_, "GPU Time (ms)", 
                    gpu_pos, {config_.overlay_size[0], 100.0f},
                    0.0f, 16.67f, {0.8f, 0.2f, 0.2f, 1.0f});
    }
}

void ShaderDebugOverlay::render_memory_usage() {
    // Convert memory values to MB for display
    std::vector<f32> memory_mb_history;
    for (usize mem : memory_history_) {
        memory_mb_history.push_back(static_cast<f32>(mem) / (1024.0f * 1024.0f));
    }
    
    if (!memory_mb_history.empty()) {
        render_graph(memory_mb_history, "Memory Usage (MB)",
                    config_.overlay_position, config_.overlay_size,
                    0.0f, 500.0f, {0.2f, 0.2f, 0.8f, 1.0f});
    }
    
    // Display current memory breakdown
    // Total: X MB, Shaders: Y MB, Textures: Z MB
}

void ShaderDebugOverlay::render_compilation_errors() {
    // Display recent compilation errors with timestamps
    auto now = std::chrono::steady_clock::now();
    
    for (const auto& error : compilation_errors_) {
        auto age = std::chrono::duration_cast<std::chrono::seconds>(now - error.timestamp);
        if (age.count() < 30) { // Show errors from last 30 seconds
            // Render error message with shader name, line number, and timestamp
            std::string error_text = error.shader_name + " (" + std::to_string(error.line) + "): " + error.message;
            // Display with error color
        }
    }
}

void ShaderDebugOverlay::render_educational_overlays() {
    // Render educational annotations
    for (const auto& annotation : educational_annotations_) {
        // Render annotation text at specified position
        // Maybe with a background box for readability
    }
}

void ShaderDebugOverlay::render_graph(const std::vector<f32>& data, const std::string& title,
                                     const std::array<f32, 2>& position, const std::array<f32, 2>& size,
                                     f32 min_val, f32 max_val, const std::array<f32, 4>& color) {
    // This would be implemented using your graphics/UI system
    // Placeholder showing the concept:
    // 1. Draw background rectangle
    // 2. Draw graph lines connecting data points
    // 3. Draw title text
    // 4. Draw axis labels and grid if needed
    // 5. Highlight values above threshold in warning color
    
    if (data.empty()) return;
    
    // Calculate scale factors
    f32 value_range = max_val - min_val;
    f32 x_scale = size[0] / static_cast<f32>(data.size() - 1);
    f32 y_scale = size[1] / value_range;
    
    // This would render the actual graph using your rendering system
}

//=============================================================================
// Advanced Shader Debugger Implementation
//=============================================================================

AdvancedShaderDebugger::AdvancedShaderDebugger(shader_runtime::ShaderRuntimeManager* runtime_manager,
                                               const DebugConfig& config)
    : config_(config), runtime_manager_(runtime_manager) {
    
    // Initialize profiler
    ShaderPerformanceProfiler::ProfilingConfig profiler_config;
    profiler_config.enable_gpu_timing = config_.enable_performance_profiling;
    profiler_config.enable_memory_tracking = config_.enable_memory_debugging;
    profiler_config.performance_warning_threshold = config_.performance_warning_threshold;
    profiler_ = std::make_unique<ShaderPerformanceProfiler>(profiler_config);
    
    // Initialize overlay
    ShaderDebugOverlay::OverlayConfig overlay_config;
    overlay_config.enable_variable_watch = config_.enable_variable_inspection;
    overlay_config.enable_performance_graphs = config_.enable_performance_profiling;
    overlay_config.show_explanatory_tooltips = config_.show_explanatory_tooltips;
    overlay_ = std::make_unique<ShaderDebugOverlay>(overlay_config);
    
    overlay_->set_profiler(profiler_.get());
    
    // Initialize educational content
    if (config_.enable_educational_mode) {
        initialize_educational_content();
    }
}

AdvancedShaderDebugger::~AdvancedShaderDebugger() = default;

void AdvancedShaderDebugger::start_debug_session(const std::string& session_name) {
    current_session_ = session_name;
    session_start_ = std::chrono::steady_clock::now();
    debug_session_active_ = true;
    
    profiler_->begin_session(session_name);
    
    // Clear previous state
    active_issues_.clear();
    resolved_issues_.clear();
}

void AdvancedShaderDebugger::end_debug_session() {
    debug_session_active_ = false;
    
    profiler_->end_session();
    
    // Generate final reports if needed
    if (config_.auto_detect_issues) {
        auto issues = detect_performance_issues();
        for (const auto& issue : issues) {
            if (issue.severity >= PerformanceIssue::Severity::Warning) {
                // Log or report significant issues
            }
        }
    }
}

void AdvancedShaderDebugger::attach_to_shader(shader_runtime::ShaderRuntimeManager::ShaderHandle handle) {
    attached_shaders_.insert(handle);
    watched_variables_[handle] = {}; // Initialize empty watch list
}

void AdvancedShaderDebugger::detach_from_shader(shader_runtime::ShaderRuntimeManager::ShaderHandle handle) {
    attached_shaders_.erase(handle);
    watched_variables_.erase(handle);
}

std::vector<AdvancedShaderDebugger::PerformanceIssue> 
AdvancedShaderDebugger::detect_performance_issues() const {
    std::vector<PerformanceIssue> issues;
    
    if (!profiler_) return issues;
    
    auto stats = profiler_->calculate_statistics();
    
    // Frame time issues
    if (stats.average_frame_time > 16.67f) {
        PerformanceIssue issue;
        issue.description = "Average frame time exceeds 60fps target (" + 
                           std::to_string(stats.average_frame_time) + " ms)";
        issue.severity = stats.average_frame_time > 33.33f ? 
                        PerformanceIssue::Severity::Critical : PerformanceIssue::Severity::Warning;
        issue.suggested_fix = "Optimize shaders, reduce draw calls, or lower visual quality";
        issue.impact_score = std::min(100.0f, (stats.average_frame_time - 16.67f) * 5.0f);
        issue.category = "Performance";
        issue.detected_time = std::chrono::steady_clock::now();
        issues.push_back(issue);
    }
    
    // Draw call issues
    if (stats.average_draw_calls > 1000) {
        PerformanceIssue issue;
        issue.description = "High draw call count (" + std::to_string(stats.average_draw_calls) + ")";
        issue.severity = PerformanceIssue::Severity::Warning;
        issue.suggested_fix = "Implement draw call batching or use instanced rendering";
        issue.impact_score = std::min(100.0f, (stats.average_draw_calls - 500.0f) / 10.0f);
        issue.category = "Performance";
        issue.detected_time = std::chrono::steady_clock::now();
        issues.push_back(issue);
    }
    
    // Memory issues
    if (stats.average_memory_usage > config_.memory_warning_threshold_mb * 1024 * 1024) {
        PerformanceIssue issue;
        issue.description = "High memory usage (" + 
                           utils::format_memory_usage(stats.average_memory_usage) + ")";
        issue.severity = PerformanceIssue::Severity::Warning;
        issue.suggested_fix = "Optimize textures, reduce shader variants, or implement streaming";
        issue.impact_score = 50.0f;
        issue.category = "Memory";
        issue.detected_time = std::chrono::steady_clock::now();
        issues.push_back(issue);
    }
    
    return issues;
}

AdvancedShaderDebugger::CompilationAnalysis 
AdvancedShaderDebugger::analyze_shader_compilation(shader_runtime::ShaderRuntimeManager::ShaderHandle handle) const {
    CompilationAnalysis analysis;
    
    if (!runtime_manager_) return analysis;
    
    auto debug_info = runtime_manager_->get_shader_debug_info(handle);
    if (debug_info) {
        analysis.compilation_successful = debug_info->diagnostics.empty() ||
            std::none_of(debug_info->diagnostics.begin(), debug_info->diagnostics.end(),
                        [](const auto& diag) { 
                            return diag.severity >= shader_compiler::CompilationDiagnostic::Severity::Error; 
                        });
        
        // Extract errors and warnings
        for (const auto& diagnostic : debug_info->diagnostics) {
            if (diagnostic.severity >= shader_compiler::CompilationDiagnostic::Severity::Error) {
                analysis.errors.push_back(diagnostic.message);
            } else if (diagnostic.severity == shader_compiler::CompilationDiagnostic::Severity::Warning) {
                analysis.warnings.push_back(diagnostic.message);
            }
        }
        
        analysis.optimization_hints = debug_info->optimization_suggestions;
    }
    
    // Platform support analysis (simplified)
    analysis.platform_support["OpenGL"] = true;
    analysis.platform_support["DirectX"] = true;
    analysis.platform_support["Vulkan"] = true;
    analysis.platform_support["Metal"] = false; // Example
    
    return analysis;
}

void AdvancedShaderDebugger::update() {
    if (!debug_session_active_) return;
    
    update_performance_monitoring();
    
    if (config_.auto_detect_issues) {
        check_for_issues();
    }
}

void AdvancedShaderDebugger::initialize_educational_content() {
    // Initialize educational explanations for common shader concepts
    EducationalExplanation pbr_explanation;
    pbr_explanation.concept = "Physically Based Rendering";
    pbr_explanation.explanation = "PBR is a shading model that more accurately simulates how light interacts with surfaces based on real-world physics.";
    pbr_explanation.key_points = {
        "Uses energy conservation principles",
        "Separates diffuse and specular components",
        "Uses metallic and roughness parameters",
        "Provides consistent results under different lighting"
    };
    pbr_explanation.difficulty_level = "Advanced";
    educational_content_["PBR"] = pbr_explanation;
    
    EducationalExplanation lighting_explanation;
    lighting_explanation.concept = "Basic Lighting";
    lighting_explanation.explanation = "Lighting in shaders simulates how surfaces interact with light sources.";
    lighting_explanation.key_points = {
        "Ambient light provides base illumination",
        "Diffuse reflection depends on surface angle to light",
        "Specular highlights simulate glossy surfaces",
        "Normal vectors determine surface orientation"
    };
    lighting_explanation.difficulty_level = "Beginner";
    educational_content_["Lighting"] = lighting_explanation;
}

void AdvancedShaderDebugger::update_performance_monitoring() {
    if (profiler_) {
        // Update profiler would be called here
        // This would typically be called from the main render loop
    }
}

void AdvancedShaderDebugger::check_for_issues() {
    auto new_issues = detect_performance_issues();
    
    // Add new issues that aren't already in the active list
    for (const auto& new_issue : new_issues) {
        bool found = std::any_of(active_issues_.begin(), active_issues_.end(),
                                [&new_issue](const PerformanceIssue& existing) {
                                    return existing.description == new_issue.description &&
                                           existing.category == new_issue.category;
                                });
        
        if (!found) {
            active_issues_.push_back(new_issue);
        }
    }
}

std::string AdvancedShaderDebugger::generate_debug_report() const {
    std::ostringstream report;
    
    report << "Shader Debug Report\n";
    report << "==================\n";
    report << "Session: " << current_session_ << "\n";
    
    if (debug_session_active_) {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - session_start_);
        report << "Session Duration: " << duration.count() << " seconds\n";
    }
    
    report << "Attached Shaders: " << attached_shaders_.size() << "\n\n";
    
    // Performance summary
    if (profiler_) {
        report << profiler_->generate_performance_report();
    }
    
    // Active issues
    if (!active_issues_.empty()) {
        report << "Active Issues:\n";
        for (const auto& issue : active_issues_) {
            report << "  - " << issue.description << " (Impact: " << issue.impact_score << ")\n";
            report << "    Suggested Fix: " << issue.suggested_fix << "\n";
        }
        report << "\n";
    }
    
    return report.str();
}

//=============================================================================
// Utility Functions Implementation
//=============================================================================

namespace utils {
    std::string debug_value_to_string(const DebugValue& value) {
        return std::visit([](const auto& val) -> std::string {
            using T = std::decay_t<decltype(val)>;
            
            if constexpr (std::is_same_v<T, f32>) {
                return std::to_string(val);
            } else if constexpr (std::is_same_v<T, i32>) {
                return std::to_string(val);
            } else if constexpr (std::is_same_v<T, bool>) {
                return val ? "true" : "false";
            } else if constexpr (std::is_same_v<T, std::array<f32, 2>>) {
                return "(" + std::to_string(val[0]) + ", " + std::to_string(val[1]) + ")";
            } else if constexpr (std::is_same_v<T, std::array<f32, 3>>) {
                return "(" + std::to_string(val[0]) + ", " + std::to_string(val[1]) + ", " + std::to_string(val[2]) + ")";
            } else if constexpr (std::is_same_v<T, std::array<f32, 4>>) {
                return "(" + std::to_string(val[0]) + ", " + std::to_string(val[1]) + ", " + 
                       std::to_string(val[2]) + ", " + std::to_string(val[3]) + ")";
            } else {
                return "unknown";
            }
        }, value);
    }
    
    std::string debug_type_to_string(DebugDataType type) {
        switch (type) {
            case DebugDataType::Float: return "float";
            case DebugDataType::Vec2: return "vec2";
            case DebugDataType::Vec3: return "vec3";
            case DebugDataType::Vec4: return "vec4";
            case DebugDataType::Int: return "int";
            case DebugDataType::Bool: return "bool";
            case DebugDataType::Mat4: return "mat4";
            case DebugDataType::Texture2D: return "sampler2D";
            default: return "unknown";
        }
    }
    
    std::string format_memory_usage(usize bytes) {
        const char* units[] = {"B", "KB", "MB", "GB"};
        int unit = 0;
        f32 size = static_cast<f32>(bytes);
        
        while (size >= 1024.0f && unit < 3) {
            size /= 1024.0f;
            unit++;
        }
        
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << size << " " << units[unit];
        return oss.str();
    }
    
    f32 calculate_performance_score(f32 frame_time_ms, u32 draw_calls, usize memory_usage) {
        // Simplified performance scoring algorithm
        f32 frame_score = std::max(0.0f, 100.0f - (frame_time_ms - 16.67f) * 3.0f);
        f32 draw_call_score = std::max(0.0f, 100.0f - (draw_calls - 500.0f) / 10.0f);
        f32 memory_score = std::max(0.0f, 100.0f - (memory_usage / (1024.0f * 1024.0f) - 50.0f));
        
        return (frame_score + draw_call_score + memory_score) / 3.0f;
    }
    
    bool is_debugging_supported() {
        // Check if debugging extensions are available
        #ifdef GL_VERSION_4_3
        return true; // Basic support available
        #else
        return false;
        #endif
    }
}

} // namespace ecscope::renderer::shader_debugging