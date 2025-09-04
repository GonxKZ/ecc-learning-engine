#pragma once

#include "../overlay.hpp"
#include "core/types.hpp"
#include <vector>
#include <string>
#include <array>
#include <unordered_map>

namespace ecscope::ui {

class PerformanceStatsPanel : public Panel {
private:
    // Frame timing data
    static constexpr usize FRAME_HISTORY_SIZE = 300; // 5 seconds at 60fps
    std::array<f32, FRAME_HISTORY_SIZE> frame_times_;
    usize frame_head_{0};
    usize frame_count_{0};
    
    // Performance metrics
    f64 last_frame_time_{0.0};
    f64 average_frame_time_{0.0};
    f64 min_frame_time_{1000.0};
    f64 max_frame_time_{0.0};
    f64 fps_smoothed_{0.0};
    
    // CPU profiling data
    struct ProfilerEntry {
        std::string name;
        f64 total_time;
        f64 avg_time;
        f64 min_time;
        f64 max_time;
        u32 call_count;
        f64 percentage;
    };
    
    std::unordered_map<std::string, ProfilerEntry> profiler_entries_;
    f64 profiler_update_timer_{0.0};
    
    // Display settings
    bool show_fps_counter_{true};
    bool show_frame_graph_{true};
    bool show_profiler_data_{true};
    bool show_system_info_{true};
    bool show_bottleneck_analysis_{false};
    
    // Graph settings
    f32 frame_time_scale_{33.33f}; // 30 FPS baseline
    bool auto_scale_graph_{true};
    bool show_frame_spikes_{true};
    
    // Performance targets
    f64 target_fps_{60.0};
    f64 target_frame_time_{16.67}; // 60 FPS in milliseconds
    
    // System information
    struct SystemInfo {
        std::string cpu_info{"Unknown"};
        std::string gpu_info{"Unknown"};
        usize ram_total{0};
        usize ram_available{0};
        std::string platform{"Unknown"};
        std::string renderer{"Unknown"};
    } system_info_;
    
    // Performance analysis
    struct PerformanceAnalysis {
        bool is_cpu_bound{false};
        bool is_gpu_bound{false};
        bool is_memory_bound{false};
        f64 cpu_usage_estimate{0.0};
        f64 gpu_usage_estimate{0.0};
        std::string bottleneck_description;
        std::vector<std::string> recommendations;
    } analysis_;
    
public:
    PerformanceStatsPanel();
    
    void render() override;
    void update(f64 delta_time) override;
    
    // Frame timing interface
    void record_frame_time(f64 frame_time);
    
    // Profiler interface
    void begin_profile(const std::string& name);
    void end_profile(const std::string& name, f64 elapsed_time);
    void clear_profiler_data();
    
    // Configuration
    void set_target_fps(f64 fps) { target_fps_ = fps; target_frame_time_ = 1000.0 / fps; }
    f64 target_fps() const { return target_fps_; }
    
    // Statistics
    f64 current_fps() const { return fps_smoothed_; }
    f64 average_frame_time() const { return average_frame_time_; }
    f64 frame_time_variance() const;
    
private:
    void update_frame_stats();
    void update_profiler_stats();
    void update_system_info();
    void analyze_performance();
    
    void render_fps_counter();
    void render_frame_graph();
    void render_profiler_data();
    void render_system_info();
    void render_bottleneck_analysis();
    void render_controls();
    
    // Graph utilities
    void render_frame_time_graph();
    void render_fps_graph();
    void render_profiler_bars();
    
    // Analysis utilities
    bool detect_frame_spikes() const;
    f64 calculate_frame_consistency() const;
    std::string get_performance_grade() const;
    
    // Utility functions
    std::string format_time(f64 milliseconds) const;
    std::string format_fps(f64 fps) const;
    ImVec4 get_performance_color(f64 value, f64 good, f64 poor) const;
};

// Global performance profiler
namespace performance_profiler {
    void initialize();
    void shutdown();
    
    // Scoped profiler for RAII
    class ScopedProfiler {
    private:
        std::string name_;
        f64 start_time_;
        
    public:
        explicit ScopedProfiler(const std::string& name);
        ~ScopedProfiler();
    };
    
    // Manual profiling
    void begin_profile(const std::string& name);
    void end_profile(const std::string& name);
    
    // Statistics
    std::unordered_map<std::string, f64> get_profile_times();
    void clear_profiles();
    
    // Convenience macro
    #define PROFILE_SCOPE(name) performance_profiler::ScopedProfiler _prof(name)
    #define PROFILE_FUNCTION() PROFILE_SCOPE(__FUNCTION__)
}

} // namespace ecscope::ui