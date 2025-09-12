#pragma once

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <mutex>
#include <queue>
#include <thread>

#ifdef ECSCOPE_HAS_IMGUI
#include <imgui.h>
#endif

namespace ecscope {
namespace gui {

enum class ProfilerMode {
    CPU,
    Memory,
    GPU,
    Network,
    Custom
};

enum class DebugLevel {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Critical
};

enum class PerformanceMetric {
    FrameTime,
    CPUUsage,
    MemoryUsage,
    GPUUsage,
    DrawCalls,
    Triangles,
    NetworkLatency,
    DiskIO,
    Custom
};

struct ProfileFrame {
    uint64_t frame_id;
    std::chrono::high_resolution_clock::time_point timestamp;
    float frame_time_ms;
    float cpu_usage_percent;
    size_t memory_usage_bytes;
    float gpu_usage_percent;
    uint32_t draw_calls;
    uint32_t triangles;
    std::unordered_map<std::string, float> custom_metrics;
};

struct DebugLogEntry {
    uint64_t id;
    std::chrono::system_clock::time_point timestamp;
    DebugLevel level;
    std::string category;
    std::string message;
    std::string file;
    int line;
    std::string function;
    std::thread::id thread_id;
};

struct PerformanceAlert {
    uint64_t id;
    std::chrono::steady_clock::time_point timestamp;
    PerformanceMetric metric;
    float threshold;
    float current_value;
    std::string description;
    bool is_active;
    uint32_t trigger_count;
};

struct MemoryBlock {
    void* address;
    size_t size;
    std::string category;
    std::chrono::steady_clock::time_point allocation_time;
    std::string source_file;
    int source_line;
    std::string source_function;
    bool is_leaked;
};

struct CPUProfileSample {
    std::chrono::high_resolution_clock::time_point timestamp;
    std::string function_name;
    std::string file_name;
    int line_number;
    float execution_time_ms;
    uint32_t call_count;
    float self_time_ms;
    std::vector<std::string> call_stack;
};

class PerformanceProfiler {
public:
    PerformanceProfiler() = default;
    ~PerformanceProfiler() = default;

    void initialize();
    void shutdown();
    
    void begin_frame();
    void end_frame();
    
    void begin_sample(const std::string& name);
    void end_sample(const std::string& name);
    
    void record_custom_metric(const std::string& name, float value);
    void set_performance_threshold(PerformanceMetric metric, float threshold);
    
    std::vector<ProfileFrame> get_frame_history(size_t count = 60) const;
    std::vector<CPUProfileSample> get_cpu_samples() const;
    std::vector<PerformanceAlert> get_active_alerts() const;
    
    void enable_profiling(ProfilerMode mode, bool enable);
    bool is_profiling_enabled(ProfilerMode mode) const;

private:
    struct SampleData {
        std::chrono::high_resolution_clock::time_point start_time;
        float accumulated_time;
        uint32_t call_count;
    };

    std::vector<ProfileFrame> frame_history_;
    std::unordered_map<std::string, SampleData> active_samples_;
    std::vector<CPUProfileSample> cpu_samples_;
    std::vector<PerformanceAlert> alerts_;
    std::unordered_map<PerformanceMetric, float> thresholds_;
    std::unordered_map<ProfilerMode, bool> profiling_modes_;
    
    uint64_t current_frame_id_;
    std::chrono::high_resolution_clock::time_point frame_start_time_;
    
    mutable std::mutex profiler_mutex_;
    
    void update_performance_alerts();
    void cleanup_old_data();
};

class MemoryProfiler {
public:
    MemoryProfiler() = default;
    ~MemoryProfiler() = default;

    void initialize();
    void shutdown();
    
    void track_allocation(void* address, size_t size, const std::string& category,
                         const std::string& file, int line, const std::string& function);
    void track_deallocation(void* address);
    
    std::vector<MemoryBlock> get_active_allocations() const;
    std::vector<MemoryBlock> get_memory_leaks() const;
    size_t get_total_allocated_memory() const;
    size_t get_peak_memory_usage() const;
    
    std::unordered_map<std::string, size_t> get_memory_by_category() const;
    std::vector<std::pair<std::chrono::steady_clock::time_point, size_t>> get_memory_timeline() const;
    
    void set_memory_alert_threshold(size_t bytes);
    void perform_leak_detection();

private:
    std::unordered_map<void*, MemoryBlock> active_blocks_;
    std::vector<MemoryBlock> leaked_blocks_;
    std::vector<std::pair<std::chrono::steady_clock::time_point, size_t>> memory_timeline_;
    
    size_t total_allocated_;
    size_t peak_memory_;
    size_t alert_threshold_;
    
    mutable std::mutex memory_mutex_;
    
    void update_memory_timeline();
};

class DebugConsole {
public:
    DebugConsole() = default;
    ~DebugConsole() = default;

    void initialize();
    void shutdown();
    void render();
    
    void add_log_entry(DebugLevel level, const std::string& category, const std::string& message,
                      const std::string& file = "", int line = 0, const std::string& function = "");
    void clear_logs();
    void export_logs(const std::string& filename) const;
    
    void set_log_filter(DebugLevel min_level);
    void set_category_filter(const std::string& category, bool enabled);
    void set_auto_scroll(bool enabled) { auto_scroll_ = enabled; }
    
    void execute_command(const std::string& command);
    void register_command(const std::string& name, std::function<void(const std::vector<std::string>&)> handler);

private:
    std::vector<DebugLogEntry> log_entries_;
    std::unordered_map<std::string, bool> category_filters_;
    std::unordered_map<std::string, std::function<void(const std::vector<std::string>&)>> commands_;
    
    DebugLevel min_log_level_;
    bool auto_scroll_;
    size_t max_log_entries_;
    uint64_t next_log_id_;
    
    char command_buffer_[512];
    std::vector<std::string> command_history_;
    int command_history_pos_;
    
    mutable std::mutex console_mutex_;
    
    void render_log_entries();
    void render_command_input();
    void render_filters();
    
    bool should_show_entry(const DebugLogEntry& entry) const;
    ImVec4 get_level_color(DebugLevel level) const;
    std::vector<std::string> parse_command(const std::string& command_line) const;
};

class PerformanceMonitor {
public:
    PerformanceMonitor() = default;
    ~PerformanceMonitor() = default;

    void initialize();
    void shutdown();
    void render();
    void update();
    
    void add_metric(const std::string& name, PerformanceMetric type, float value);
    void set_metric_range(const std::string& name, float min_val, float max_val);
    void set_metric_color(const std::string& name, ImU32 color);
    
    void enable_realtime_monitoring(bool enable) { realtime_monitoring_ = enable; }
    void set_update_frequency(float frequency) { update_frequency_ = frequency; }

private:
    struct MetricData {
        PerformanceMetric type;
        std::vector<float> values;
        float min_range;
        float max_range;
        ImU32 color;
        bool is_visible;
    };

    std::unordered_map<std::string, MetricData> metrics_;
    bool realtime_monitoring_;
    float update_frequency_;
    float last_update_time_;
    size_t max_samples_;
    
    void render_metric_graphs();
    void render_metric_table();
    void render_metric_controls();
    
    void update_system_metrics();
    float get_cpu_usage();
    size_t get_memory_usage();
    float get_gpu_usage();
};

class CallStackTracer {
public:
    CallStackTracer() = default;
    ~CallStackTracer() = default;

    void initialize();
    void shutdown();
    void render();
    
    void capture_call_stack();
    void enable_automatic_capture(bool enable, uint32_t interval_ms = 100);
    
    std::vector<std::string> get_current_call_stack() const;
    std::vector<std::vector<std::string>> get_call_stack_history(size_t count = 10) const;

private:
    std::vector<std::vector<std::string>> call_stack_history_;
    std::vector<std::string> current_call_stack_;
    
    bool auto_capture_enabled_;
    uint32_t capture_interval_ms_;
    std::chrono::steady_clock::time_point last_capture_time_;
    
    std::mutex tracer_mutex_;
    
    void perform_stack_walk();
    std::string demangle_symbol(const std::string& symbol) const;
};

class DebugToolsUI {
public:
    DebugToolsUI();
    ~DebugToolsUI();

    bool initialize();
    void render();
    void update(float delta_time);
    void shutdown();

    // Profiler interface
    void begin_profile_sample(const std::string& name);
    void end_profile_sample(const std::string& name);
    void record_custom_metric(const std::string& name, float value);
    
    // Memory tracking interface
    void track_memory_allocation(void* address, size_t size, const std::string& category = "General");
    void track_memory_deallocation(void* address);
    
    // Logging interface
    void log(DebugLevel level, const std::string& category, const std::string& message);
    void log_trace(const std::string& category, const std::string& message);
    void log_debug(const std::string& category, const std::string& message);
    void log_info(const std::string& category, const std::string& message);
    void log_warning(const std::string& category, const std::string& message);
    void log_error(const std::string& category, const std::string& message);
    void log_critical(const std::string& category, const std::string& message);
    
    // Configuration
    void set_profiler_enabled(ProfilerMode mode, bool enabled);
    void set_performance_threshold(PerformanceMetric metric, float threshold);
    void set_memory_alert_threshold(size_t bytes);
    
    // Callbacks
    void set_performance_alert_callback(std::function<void(const PerformanceAlert&)> callback);
    void set_memory_leak_callback(std::function<void(const std::vector<MemoryBlock>&)> callback);

    bool is_window_open() const { return show_window_; }
    void set_window_open(bool open) { show_window_ = open; }

private:
    void render_menu_bar();
    void render_profiler_panel();
    void render_memory_panel();
    void render_console_panel();
    void render_performance_monitor_panel();
    void render_call_stack_panel();
    void render_alerts_panel();
    
    void render_frame_time_graph();
    void render_memory_usage_graph();
    void render_cpu_profile_tree();
    void render_memory_allocations_table();
    
    void update_profiler();
    void update_memory_tracker();
    void check_performance_alerts();

    std::unique_ptr<PerformanceProfiler> profiler_;
    std::unique_ptr<MemoryProfiler> memory_profiler_;
    std::unique_ptr<DebugConsole> console_;
    std::unique_ptr<PerformanceMonitor> performance_monitor_;
    std::unique_ptr<CallStackTracer> call_stack_tracer_;
    
    std::function<void(const PerformanceAlert&)> performance_alert_callback_;
    std::function<void(const std::vector<MemoryBlock>&)> memory_leak_callback_;

    bool show_window_;
    bool show_profiler_;
    bool show_memory_;
    bool show_console_;
    bool show_performance_monitor_;
    bool show_call_stack_;
    bool show_alerts_;
    
    // UI State
    float splitter_sizes_[3];
    bool freeze_profiler_;
    bool capture_screenshots_;
    std::string export_path_;
    
    // Performance tracking
    std::chrono::high_resolution_clock::time_point last_frame_time_;
    float frame_delta_time_;
};

class DebugToolsManager {
public:
    static DebugToolsManager& instance() {
        static DebugToolsManager instance;
        return instance;
    }

    void initialize();
    void shutdown();
    void update(float delta_time);

    void register_debug_tools_ui(DebugToolsUI* ui);
    void unregister_debug_tools_ui(DebugToolsUI* ui);

    void notify_performance_sample(const ProfileFrame& frame);
    void notify_memory_allocation(const MemoryBlock& block);
    void notify_log_entry(const DebugLogEntry& entry);

private:
    DebugToolsManager() = default;
    ~DebugToolsManager() = default;

    std::vector<DebugToolsUI*> registered_uis_;
    std::mutex ui_mutex_;
};

// Convenience macros for profiling
#define ECSCOPE_PROFILE_SCOPE(name) \
    ecscope::gui::ScopedProfiler _prof(name)

#define ECSCOPE_PROFILE_FUNCTION() \
    ECSCOPE_PROFILE_SCOPE(__FUNCTION__)

class ScopedProfiler {
public:
    ScopedProfiler(const std::string& name);
    ~ScopedProfiler();

private:
    std::string name_;
    std::chrono::high_resolution_clock::time_point start_time_;
};

}} // namespace ecscope::gui