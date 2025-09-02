#include "panel_stats.hpp"
#include "core/time.hpp"
#include "core/log.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>

#ifdef ECSCOPE_HAS_GRAPHICS
#include "imgui.h"
#include <SDL2/SDL.h>
#include <GL/gl.h>
#endif

namespace ecscope::ui {

PerformanceStatsPanel::PerformanceStatsPanel() 
    : Panel("Performance Stats", true) {
    
    // Initialize frame time history
    frame_times_.fill(0.0f);
    
    // Get initial system info
    update_system_info();
}

void PerformanceStatsPanel::render() {
#ifdef ECSCOPE_HAS_GRAPHICS
    if (!visible_) return;
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
    if (ImGui::Begin(name_.c_str(), &visible_, window_flags)) {
        
        // Controls at top
        render_controls();
        ImGui::Separator();
        
        // Main content in collapsible sections
        if (show_fps_counter_) {
            if (ImGui::CollapsingHeader("Frame Rate", ImGuiTreeNodeFlags_DefaultOpen)) {
                render_fps_counter();
            }
        }
        
        if (show_frame_graph_) {
            if (ImGui::CollapsingHeader("Frame Time Graph", ImGuiTreeNodeFlags_DefaultOpen)) {
                render_frame_graph();
            }
        }
        
        if (show_profiler_data_) {
            if (ImGui::CollapsingHeader("Profiler Data")) {
                render_profiler_data();
            }
        }
        
        if (show_system_info_) {
            if (ImGui::CollapsingHeader("System Information")) {
                render_system_info();
            }
        }
        
        if (show_bottleneck_analysis_) {
            if (ImGui::CollapsingHeader("Bottleneck Analysis")) {
                render_bottleneck_analysis();
            }
        }
    }
    ImGui::End();
#endif
}

void PerformanceStatsPanel::update(f64 delta_time) {
    // Record frame time
    record_frame_time(delta_time * 1000.0); // Convert to milliseconds
    
    // Update profiler stats periodically
    profiler_update_timer_ += delta_time;
    if (profiler_update_timer_ >= 0.5) { // Update every 500ms
        update_profiler_stats();
        analyze_performance();
        profiler_update_timer_ = 0.0;
    }
}

void PerformanceStatsPanel::record_frame_time(f64 frame_time) {
    // Add to circular buffer
    frame_times_[frame_head_] = static_cast<f32>(frame_time);
    frame_head_ = (frame_head_ + 1) % FRAME_HISTORY_SIZE;
    if (frame_count_ < FRAME_HISTORY_SIZE) {
        frame_count_++;
    }
    
    // Update frame statistics
    last_frame_time_ = frame_time;
    update_frame_stats();
}

void PerformanceStatsPanel::begin_profile(const std::string& name) {
    performance_profiler::begin_profile(name);
}

void PerformanceStatsPanel::end_profile(const std::string& name, f64 elapsed_time) {
    (void)elapsed_time; // The profiler will calculate this
    performance_profiler::end_profile(name);
}

void PerformanceStatsPanel::clear_profiler_data() {
    profiler_entries_.clear();
    performance_profiler::clear_profiles();
}

void PerformanceStatsPanel::update_frame_stats() {
    if (frame_count_ == 0) return;
    
    // Calculate statistics from frame history
    f64 total_time = 0.0;
    f64 min_time = 1000.0;
    f64 max_time = 0.0;
    
    usize count = std::min(frame_count_, FRAME_HISTORY_SIZE);
    for (usize i = 0; i < count; ++i) {
        f64 frame_time = static_cast<f64>(frame_times_[i]);
        total_time += frame_time;
        min_time = std::min(min_time, frame_time);
        max_time = std::max(max_time, frame_time);
    }
    
    average_frame_time_ = total_time / count;
    min_frame_time_ = min_time;
    max_frame_time_ = max_time;
    
    // Smoothed FPS calculation
    if (average_frame_time_ > 0.0) {
        f64 instantaneous_fps = 1000.0 / last_frame_time_;
        f64 average_fps = 1000.0 / average_frame_time_;
        
        // Smooth between instantaneous and average
        constexpr f64 smoothing = 0.1;
        fps_smoothed_ = fps_smoothed_ * (1.0 - smoothing) + instantaneous_fps * smoothing;
    }
}

void PerformanceStatsPanel::update_profiler_stats() {
    auto profile_times = performance_profiler::get_profile_times();
    
    // Update profiler entries
    for (const auto& [name, time] : profile_times) {
        auto& entry = profiler_entries_[name];
        entry.name = name;
        
        if (entry.call_count == 0) {
            entry.total_time = time;
            entry.avg_time = time;
            entry.min_time = time;
            entry.max_time = time;
        } else {
            entry.total_time += time;
            entry.avg_time = entry.total_time / (entry.call_count + 1);
            entry.min_time = std::min(entry.min_time, time);
            entry.max_time = std::max(entry.max_time, time);
        }
        
        entry.call_count++;
    }
    
    // Calculate percentages
    f64 total_profiled_time = 0.0;
    for (const auto& [name, entry] : profiler_entries_) {
        total_profiled_time += entry.avg_time;
    }
    
    if (total_profiled_time > 0.0) {
        for (auto& [name, entry] : profiler_entries_) {
            entry.percentage = (entry.avg_time / total_profiled_time) * 100.0;
        }
    }
}

void PerformanceStatsPanel::update_system_info() {
#ifdef ECSCOPE_HAS_GRAPHICS
    // Get SDL and OpenGL information
    SDL_version compiled, linked;
    SDL_VERSION(&compiled);
    SDL_GetVersion(&linked);
    
    system_info_.platform = SDL_GetPlatform();
    
    // OpenGL renderer info
    const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    
    if (renderer) system_info_.gpu_info = renderer;
    if (version) system_info_.renderer = version;
    
    // Memory info (simplified)
    system_info_.ram_total = SDL_GetSystemRAM() * 1024 * 1024; // Convert MB to bytes
    system_info_.ram_available = system_info_.ram_total; // Approximation
#endif
}

void PerformanceStatsPanel::analyze_performance() {
    analysis_ = PerformanceAnalysis{}; // Reset
    
    // CPU bound detection
    if (average_frame_time_ > target_frame_time_ * 1.5) {
        analysis_.is_cpu_bound = true;
        analysis_.bottleneck_description = "High frame times suggest CPU bottleneck";
    }
    
    // Frame consistency analysis
    f64 consistency = calculate_frame_consistency();
    if (consistency < 0.8) { // Less than 80% consistent
        analysis_.recommendations.push_back("Frame times are inconsistent - check for periodic heavy operations");
    }
    
    // Performance grade
    if (fps_smoothed_ >= target_fps_ * 0.95) {
        analysis_.recommendations.push_back("Performance is excellent");
    } else if (fps_smoothed_ >= target_fps_ * 0.75) {
        analysis_.recommendations.push_back("Performance is good but could be optimized");
    } else {
        analysis_.recommendations.push_back("Performance needs significant optimization");
    }
    
    // Memory bound detection (simplified)
    // In a real implementation, this would check memory pressure indicators
    if (detect_frame_spikes()) {
        analysis_.is_memory_bound = true;
        analysis_.recommendations.push_back("Frame spikes detected - possible garbage collection or memory allocation issues");
    }
}

void PerformanceStatsPanel::render_fps_counter() {
#ifdef ECSCOPE_HAS_GRAPHICS
    // Large FPS display
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]); // Use default font, but we could scale it
    
    // FPS with color coding
    ImVec4 fps_color = get_performance_color(fps_smoothed_, target_fps_, target_fps_ * 0.5);
    ImGui::TextColored(fps_color, "%.1f FPS", fps_smoothed_);
    
    ImGui::PopFont();
    
    // Frame time details
    ImGui::Separator();
    ImGui::Text("Frame Time: %.2f ms", last_frame_time_);
    ImGui::Text("Average: %.2f ms", average_frame_time_);
    ImGui::Text("Min: %.2f ms", min_frame_time_);
    ImGui::Text("Max: %.2f ms", max_frame_time_);
    
    // Target comparison
    f64 target_deviation = (average_frame_time_ - target_frame_time_) / target_frame_time_ * 100.0;
    ImVec4 target_color = target_deviation < 10.0 ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1);
    ImGui::TextColored(target_color, "Target: %.2f ms (%.1f%% %s)", 
                      target_frame_time_, 
                      std::abs(target_deviation),
                      target_deviation > 0 ? "over" : "under");
    
    // Performance grade
    std::string grade = get_performance_grade();
    ImVec4 grade_color = (grade == "A") ? ImVec4(0, 1, 0, 1) : 
                        (grade == "B") ? ImVec4(1, 1, 0, 1) : ImVec4(1, 0, 0, 1);
    ImGui::TextColored(grade_color, "Grade: %s", grade.c_str());
    
    // Frame consistency
    f64 consistency = calculate_frame_consistency() * 100.0;
    ImGui::Text("Consistency: %.1f%%", consistency);
    
    // Progress bar for frame time
    f32 frame_time_fraction = static_cast<f32>(last_frame_time_ / target_frame_time_);
    frame_time_fraction = std::clamp(frame_time_fraction, 0.0f, 2.0f);
    
    ImVec4 bar_color = get_performance_color(1.0 / frame_time_fraction, 1.0, 0.5);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, bar_color);
    ImGui::ProgressBar(frame_time_fraction / 2.0f, ImVec2(-1, 0), 
                      format_time(last_frame_time_).c_str());
    ImGui::PopStyleColor();
#endif
}

void PerformanceStatsPanel::render_frame_graph() {
#ifdef ECSCOPE_HAS_GRAPHICS
    if (frame_count_ == 0) {
        ImGui::TextDisabled("No frame data available");
        return;
    }
    
    // Auto-scale or manual scale
    f32 scale_max = frame_time_scale_;
    if (auto_scale_graph_) {
        f32 max_time = *std::max_element(frame_times_.begin(), 
                                        frame_times_.begin() + std::min(frame_count_, FRAME_HISTORY_SIZE));
        scale_max = max_time * 1.2f; // 20% headroom
        scale_max = std::max(scale_max, 16.67f); // Minimum 60fps scale
    }
    
    // Frame time graph
    usize display_count = std::min(frame_count_, FRAME_HISTORY_SIZE);
    ImGui::PlotLines("Frame Time (ms)", frame_times_.data(), static_cast<int>(display_count),
                    static_cast<int>(frame_head_), nullptr, 0.0f, scale_max, ImVec2(0, 150));
    
    // Target line overlay (this would require custom rendering)
    ImGui::Text("Target: %.1f ms", target_frame_time_);
    
    // Frame spikes detection
    if (show_frame_spikes_ && detect_frame_spikes()) {
        ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "  Frame spikes detected");
    }
    
    // Graph controls
    ImGui::Separator();
    ImGui::Checkbox("Auto Scale", &auto_scale_graph_);
    
    if (!auto_scale_graph_) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        ImGui::DragFloat("Max (ms)", &frame_time_scale_, 1.0f, 16.67f, 200.0f);
    }
    
    ImGui::SameLine();
    ImGui::Checkbox("Show Spikes", &show_frame_spikes_);
    
    // Statistics overlay
    ImGui::Text("Graph shows last %zu frames", display_count);
    f64 variance = frame_time_variance();
    ImGui::Text("Variance: %.2f ms²", variance);
#endif
}

void PerformanceStatsPanel::render_profiler_data() {
#ifdef ECSCOPE_HAS_GRAPHICS
    if (profiler_entries_.empty()) {
        ImGui::TextDisabled("No profiler data available");
        ImGui::Text("Use PROFILE_SCOPE(name) or PROFILE_FUNCTION() macros in code");
        return;
    }
    
    // Sort entries by average time (descending)
    std::vector<ProfilerEntry*> sorted_entries;
    for (auto& [name, entry] : profiler_entries_) {
        sorted_entries.push_back(&entry);
    }
    
    std::sort(sorted_entries.begin(), sorted_entries.end(),
        [](const ProfilerEntry* a, const ProfilerEntry* b) {
            return a->avg_time > b->avg_time;
        });
    
    // Render as table
    if (ImGui::BeginTable("ProfilerTable", 6, 
                         ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | 
                         ImGuiTableFlags_Sortable)) {
        
        ImGui::TableSetupColumn("Function", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Avg (ms)", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Min (ms)", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Max (ms)", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Calls", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Percentage", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();
        
        for (const auto* entry : sorted_entries) {
            ImGui::TableNextRow();
            
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", entry->name.c_str());
            
            ImGui::TableSetColumnIndex(1);
            ImVec4 time_color = get_performance_color(entry->avg_time, 1.0, 5.0);
            ImGui::TextColored(time_color, "%.3f", entry->avg_time);
            
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%.3f", entry->min_time);
            
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%.3f", entry->max_time);
            
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%u", entry->call_count);
            
            ImGui::TableSetColumnIndex(5);
            f32 percentage = static_cast<f32>(entry->percentage / 100.0);
            ImGui::ProgressBar(percentage, ImVec2(-1, 0), 
                              (std::to_string(static_cast<int>(entry->percentage)) + "%").c_str());
        }
        
        ImGui::EndTable();
    }
    
    // Controls
    ImGui::Separator();
    if (ImGui::Button("Clear Profiler Data")) {
        clear_profiler_data();
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Export CSV")) {
        LOG_INFO("Profiler data export not yet implemented");
    }
    
    // Total profiled time
    f64 total_time = 0.0;
    for (const auto* entry : sorted_entries) {
        total_time += entry->avg_time;
    }
    ImGui::Text("Total Profiled Time: %.3f ms", total_time);
    
    if (total_time > 0.0) {
        f32 profiled_fraction = static_cast<f32>(total_time / last_frame_time_);
        ImGui::Text("Frame Coverage: %.1f%%", profiled_fraction * 100.0f);
    }
#endif
}

void PerformanceStatsPanel::render_system_info() {
#ifdef ECSCOPE_HAS_GRAPHICS
    ImGui::Text("System Information");
    ImGui::Separator();
    
    ImGui::Text("Platform: %s", system_info_.platform.c_str());
    ImGui::Text("GPU: %s", system_info_.gpu_info.c_str());
    ImGui::Text("OpenGL: %s", system_info_.renderer.c_str());
    
    if (system_info_.ram_total > 0) {
        ImGui::Text("System RAM: %s", imgui_utils::format_bytes(system_info_.ram_total).c_str());
    }
    
    // Application info
    ImGui::Spacing();
    ImGui::Text("Application");
    ImGui::Separator();
    
    ImGui::Text("Target FPS: %.0f", target_fps_);
    ImGui::Text("VSync: %s", "Unknown"); // Would need to query actual state
    
    // Build configuration
    ImGui::Spacing();
    ImGui::Text("Build Configuration");
    ImGui::Separator();
    
#ifdef ECSCOPE_ENABLE_INSTRUMENTATION
    ImGui::Text("Instrumentation: Enabled");
#else
    ImGui::Text("Instrumentation: Disabled");
#endif

#ifdef NDEBUG
    ImGui::Text("Configuration: Release");
#else
    ImGui::Text("Configuration: Debug");
#endif

    ImGui::Text("C++ Standard: C++20");
    ImGui::Text("Compiler: " 
#ifdef __clang__
    "Clang"
#elif defined(__GNUC__)
    "GCC"
#elif defined(_MSC_VER)
    "MSVC"
#else
    "Unknown"
#endif
    );
#endif
}

void PerformanceStatsPanel::render_bottleneck_analysis() {
#ifdef ECSCOPE_HAS_GRAPHICS
    ImGui::Text("Performance Analysis");
    ImGui::Separator();
    
    // Bottleneck indicators
    if (analysis_.is_cpu_bound) {
        ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "  CPU Bound");
    }
    
    if (analysis_.is_gpu_bound) {
        ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "  GPU Bound");  
    }
    
    if (analysis_.is_memory_bound) {
        ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "  Memory Bound");
    }
    
    if (!analysis_.is_cpu_bound && !analysis_.is_gpu_bound && !analysis_.is_memory_bound) {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), " No bottlenecks detected");
    }
    
    // Description
    if (!analysis_.bottleneck_description.empty()) {
        ImGui::Text("%s", analysis_.bottleneck_description.c_str());
    }
    
    // Recommendations
    ImGui::Spacing();
    ImGui::Text("Recommendations:");
    for (const auto& rec : analysis_.recommendations) {
        ImGui::BulletText("%s", rec.c_str());
    }
    
    if (analysis_.recommendations.empty()) {
        ImGui::TextDisabled("No specific recommendations available");
    }
#endif
}

void PerformanceStatsPanel::render_controls() {
#ifdef ECSCOPE_HAS_GRAPHICS
    // Target FPS setting
    ImGui::SetNextItemWidth(80);
    f32 target_fps_f = static_cast<f32>(target_fps_);
    if (ImGui::DragFloat("Target FPS", &target_fps_f, 1.0f, 30.0f, 144.0f, "%.0f")) {
        set_target_fps(static_cast<f64>(target_fps_f));
    }
    
    // View toggles
    ImGui::SameLine();
    if (ImGui::Button("Views")) {
        ImGui::OpenPopup("ViewSettings");
    }
    
    if (ImGui::BeginPopup("ViewSettings")) {
        ImGui::Checkbox("FPS Counter", &show_fps_counter_);
        ImGui::Checkbox("Frame Graph", &show_frame_graph_);
        ImGui::Checkbox("Profiler Data", &show_profiler_data_);
        ImGui::Checkbox("System Info", &show_system_info_);
        ImGui::Checkbox("Bottleneck Analysis", &show_bottleneck_analysis_);
        ImGui::EndPopup();
    }
    
    // Actions
    ImGui::SameLine();
    if (ImGui::Button("Reset Stats")) {
        // Reset all statistics
        frame_head_ = 0;
        frame_count_ = 0;
        frame_times_.fill(0.0f);
        clear_profiler_data();
    }
#endif
}

f64 PerformanceStatsPanel::frame_time_variance() const {
    if (frame_count_ < 2) return 0.0;
    
    // Calculate variance
    f64 mean = average_frame_time_;
    f64 variance = 0.0;
    
    usize count = std::min(frame_count_, FRAME_HISTORY_SIZE);
    for (usize i = 0; i < count; ++i) {
        f64 diff = static_cast<f64>(frame_times_[i]) - mean;
        variance += diff * diff;
    }
    
    return variance / (count - 1);
}

bool PerformanceStatsPanel::detect_frame_spikes() const {
    if (frame_count_ < 10) return false;
    
    // Look for frame times that are significantly higher than average
    f64 spike_threshold = average_frame_time_ * 2.0;
    usize spike_count = 0;
    
    usize count = std::min(frame_count_, FRAME_HISTORY_SIZE);
    for (usize i = 0; i < count; ++i) {
        if (static_cast<f64>(frame_times_[i]) > spike_threshold) {
            spike_count++;
        }
    }
    
    // Consider it spiky if more than 5% of frames are spikes
    return (static_cast<f64>(spike_count) / count) > 0.05;
}

f64 PerformanceStatsPanel::calculate_frame_consistency() const {
    if (frame_count_ < 10) return 1.0;
    
    // Consistency based on how close frame times are to the average
    f64 total_deviation = 0.0;
    usize count = std::min(frame_count_, FRAME_HISTORY_SIZE);
    
    for (usize i = 0; i < count; ++i) {
        f64 deviation = std::abs(static_cast<f64>(frame_times_[i]) - average_frame_time_);
        total_deviation += deviation;
    }
    
    f64 average_deviation = total_deviation / count;
    f64 consistency = 1.0 - (average_deviation / average_frame_time_);
    return std::clamp(consistency, 0.0, 1.0);
}

std::string PerformanceStatsPanel::get_performance_grade() const {
    if (fps_smoothed_ >= target_fps_ * 0.95) return "A";
    if (fps_smoothed_ >= target_fps_ * 0.85) return "B";
    if (fps_smoothed_ >= target_fps_ * 0.70) return "C";
    if (fps_smoothed_ >= target_fps_ * 0.50) return "D";
    return "F";
}

std::string PerformanceStatsPanel::format_time(f64 milliseconds) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << milliseconds << " ms";
    return oss.str();
}

std::string PerformanceStatsPanel::format_fps(f64 fps) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << fps << " FPS";
    return oss.str();
}

ImVec4 PerformanceStatsPanel::get_performance_color(f64 value, f64 good, f64 poor) const {
    // Linear interpolation between green (good) and red (poor)
    f64 ratio = (value - poor) / (good - poor);
    ratio = std::clamp(ratio, 0.0, 1.0);
    
    return ImVec4(
        static_cast<f32>(1.0 - ratio), // Red component
        static_cast<f32>(ratio),       // Green component  
        0.0f,                          // Blue component
        1.0f                           // Alpha
    );
}

// Global performance profiler implementation
namespace performance_profiler {
    
    static struct {
        bool initialized{false};
        std::unordered_map<std::string, f64> profile_times;
        std::unordered_map<std::string, f64> active_profiles;
    } g_profiler;
    
    void initialize() {
        g_profiler = {};
        g_profiler.initialized = true;
        LOG_INFO("Performance profiler initialized");
    }
    
    void shutdown() {
        g_profiler.initialized = false;
        LOG_INFO("Performance profiler shutdown");
    }
    
    ScopedProfiler::ScopedProfiler(const std::string& name) 
        : name_(name), start_time_(core::get_time_seconds()) {
    }
    
    ScopedProfiler::~ScopedProfiler() {
        f64 elapsed = (core::get_time_seconds() - start_time_) * 1000.0; // Convert to milliseconds
        
        if (g_profiler.initialized) {
            g_profiler.profile_times[name_] = elapsed;
        }
    }
    
    void begin_profile(const std::string& name) {
        if (!g_profiler.initialized) return;
        
        g_profiler.active_profiles[name] = core::get_time_seconds();
    }
    
    void end_profile(const std::string& name) {
        if (!g_profiler.initialized) return;
        
        auto it = g_profiler.active_profiles.find(name);
        if (it != g_profiler.active_profiles.end()) {
            f64 elapsed = (core::get_time_seconds() - it->second) * 1000.0; // Convert to milliseconds
            g_profiler.profile_times[name] = elapsed;
            g_profiler.active_profiles.erase(it);
        }
    }
    
    std::unordered_map<std::string, f64> get_profile_times() {
        return g_profiler.profile_times;
    }
    
    void clear_profiles() {
        g_profiler.profile_times.clear();
        g_profiler.active_profiles.clear();
    }
    
} // namespace performance_profiler

} // namespace ecscope::ui