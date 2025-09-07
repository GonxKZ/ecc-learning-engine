#pragma once

#include "types.hpp"
#include <chrono>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>
#include <queue>
#include <functional>

// Platform-specific GPU API includes
#ifdef _WIN32
    #include <d3d11.h>
    #include <dxgi.h>
    #include <wrl/client.h>
    #pragma comment(lib, "d3d11.lib")
    #pragma comment(lib, "dxgi.lib")
#endif

// OpenGL includes (cross-platform)
#if defined(ECSCOPE_USE_OPENGL)
    #ifdef _WIN32
        #include <GL/gl.h>
        #include <GL/wglext.h>
    #elif defined(__linux__)
        #include <GL/gl.h>
        #include <GL/glx.h>
    #elif defined(__APPLE__)
        #include <OpenGL/gl.h>
    #endif
#endif

// Vulkan includes (cross-platform)
#if defined(ECSCOPE_USE_VULKAN)
    #include <vulkan/vulkan.h>
#endif

namespace ecscope::gpu {

using namespace std::chrono;

// GPU API types
enum class GraphicsAPI : u8 {
    UNKNOWN,
    OPENGL,
    DIRECT3D11,
    DIRECT3D12,
    VULKAN,
    METAL
};

// GPU performance event types
enum class GPUEventType : u8 {
    RENDER_PASS,
    DRAW_CALL,
    COMPUTE_DISPATCH,
    BUFFER_UPLOAD,
    TEXTURE_UPLOAD,
    SHADER_COMPILATION,
    PIPELINE_BIND,
    RESOURCE_BARRIER,
    PRESENT,
    CUSTOM
};

// Draw call information
struct DrawCallInfo {
    std::string name;
    u32 vertex_count;
    u32 instance_count;
    u32 index_count;
    bool is_indexed;
    std::string shader_program;
    std::string vertex_buffer;
    std::string index_buffer;
    u32 texture_count;
    std::vector<std::string> bound_textures;
    high_resolution_clock::time_point timestamp;
    microseconds gpu_time;
    usize memory_used;
};

// Render pass statistics
struct RenderPassInfo {
    std::string name;
    u32 draw_call_count;
    u32 vertex_count;
    u32 triangle_count;
    microseconds total_gpu_time;
    microseconds setup_time;
    microseconds draw_time;
    microseconds cleanup_time;
    usize render_target_count;
    std::vector<std::string> render_targets;
    bool uses_depth_buffer;
    bool uses_stencil_buffer;
    high_resolution_clock::time_point start_time;
    high_resolution_clock::time_point end_time;
};

// GPU memory information
struct GPUMemoryInfo {
    usize total_memory;
    usize available_memory;
    usize used_memory;
    usize vertex_buffer_memory;
    usize index_buffer_memory;
    usize texture_memory;
    usize render_target_memory;
    usize shader_memory;
    usize constant_buffer_memory;
    f32 memory_pressure; // 0.0 to 1.0
    high_resolution_clock::time_point timestamp;
};

// Shader compilation statistics
struct ShaderCompilationInfo {
    std::string shader_name;
    std::string shader_type; // vertex, fragment, compute, etc.
    usize source_size;
    usize compiled_size;
    microseconds compilation_time;
    bool compilation_success;
    std::string error_message;
    u32 instruction_count;
    u32 register_count;
    high_resolution_clock::time_point timestamp;
};

// GPU performance counter data
struct GPUCounters {
    // Timing counters
    f64 gpu_utilization; // 0.0 to 100.0
    f64 memory_bandwidth_utilization; // 0.0 to 100.0
    f64 texture_cache_hit_rate; // 0.0 to 100.0
    f64 vertex_cache_hit_rate; // 0.0 to 100.0
    
    // Throughput counters
    u64 vertices_processed_per_second;
    u64 pixels_rendered_per_second;
    u64 triangles_per_second;
    u64 texture_samples_per_second;
    
    // Memory counters
    u64 memory_reads_per_second;
    u64 memory_writes_per_second;
    u64 texture_memory_bandwidth;
    u64 vertex_buffer_bandwidth;
    
    // Pipeline counters
    u64 draw_calls_per_second;
    u64 state_changes_per_second;
    u64 shader_switches_per_second;
    u64 texture_binds_per_second;
    
    high_resolution_clock::time_point timestamp;
};

// Performance bottleneck analysis
struct BottleneckAnalysis {
    enum class BottleneckType {
        NONE,
        GPU_COMPUTE,
        MEMORY_BANDWIDTH,
        VERTEX_PROCESSING,
        PIXEL_PROCESSING,
        DRAW_CALL_OVERHEAD,
        SHADER_COMPILATION,
        RESOURCE_BINDING,
        SYNCHRONIZATION
    };
    
    BottleneckType primary_bottleneck;
    BottleneckType secondary_bottleneck;
    f32 bottleneck_severity; // 0.0 to 1.0
    std::string description;
    std::vector<std::string> recommendations;
    f32 confidence; // 0.0 to 1.0
};

// Frame timing information
struct FrameStats {
    u64 frame_number;
    microseconds frame_time;
    microseconds cpu_time;
    microseconds gpu_time;
    microseconds present_time;
    u32 draw_call_count;
    u32 vertex_count;
    u32 triangle_count;
    u32 texture_bind_count;
    u32 shader_bind_count;
    f32 fps;
    high_resolution_clock::time_point timestamp;
};

// GPU performance event
struct GPUEvent {
    GPUEventType type;
    std::string name;
    high_resolution_clock::time_point start_time;
    microseconds duration;
    u32 thread_id;
    usize memory_used;
    std::unordered_map<std::string, std::string> metadata;
};

// GPU query abstraction for timing
class GPUQuery {
public:
    virtual ~GPUQuery() = default;
    virtual void begin() = 0;
    virtual void end() = 0;
    virtual bool is_ready() const = 0;
    virtual microseconds get_result() const = 0;
    virtual void reset() = 0;
};

// Platform-specific GPU query implementations
#ifdef _WIN32
class D3D11GPUQuery : public GPUQuery {
private:
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<ID3D11Query> disjoint_query_;
    Microsoft::WRL::ComPtr<ID3D11Query> start_query_;
    Microsoft::WRL::ComPtr<ID3D11Query> end_query_;
    
public:
    D3D11GPUQuery(ID3D11Device* device, ID3D11DeviceContext* context);
    void begin() override;
    void end() override;
    bool is_ready() const override;
    microseconds get_result() const override;
    void reset() override;
};
#endif

#if defined(ECSCOPE_USE_OPENGL)
class OpenGLGPUQuery : public GPUQuery {
private:
    u32 query_id_;
    bool query_active_;
    
public:
    OpenGLGPUQuery();
    ~OpenGLGPUQuery();
    void begin() override;
    void end() override;
    bool is_ready() const override;
    microseconds get_result() const override;
    void reset() override;
};
#endif

// Main GPU performance monitor class
class GPUPerformanceMonitor {
private:
    mutable std::mutex data_mutex_;
    std::atomic<bool> enabled_{true};
    GraphicsAPI current_api_;
    
    // Platform-specific handles
#ifdef _WIN32
    Microsoft::WRL::ComPtr<ID3D11Device> d3d11_device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3d11_context_;
    Microsoft::WRL::ComPtr<IDXGIAdapter> dxgi_adapter_;
#endif
    
    // Query management
    std::queue<std::unique_ptr<GPUQuery>> available_queries_;
    std::unordered_map<std::string, std::unique_ptr<GPUQuery>> active_queries_;
    usize max_queries_ = 100;
    
    // Performance data
    std::vector<FrameStats> frame_history_;
    std::vector<DrawCallInfo> draw_call_history_;
    std::vector<RenderPassInfo> render_pass_history_;
    std::vector<GPUMemoryInfo> memory_history_;
    std::vector<ShaderCompilationInfo> shader_compilation_history_;
    std::vector<GPUEvent> event_history_;
    
    // Counters and statistics
    GPUCounters current_counters_;
    std::atomic<u64> frame_counter_{0};
    std::atomic<u32> draw_call_counter_{0};
    std::atomic<u32> vertex_counter_{0};
    std::atomic<u32> triangle_counter_{0};
    
    // Timing
    high_resolution_clock::time_point frame_start_time_;
    high_resolution_clock::time_point last_update_time_;
    
    // Configuration
    usize max_frame_history_ = 1000;
    usize max_draw_call_history_ = 10000;
    usize max_event_history_ = 10000;
    bool enable_draw_call_tracking_ = true;
    bool enable_memory_tracking_ = true;
    bool enable_shader_tracking_ = true;
    
    // Internal methods
    void initialize_api();
    void cleanup_api();
    void update_gpu_counters();
    void update_memory_info();
    void create_gpu_queries();
    std::unique_ptr<GPUQuery> get_available_query();
    void return_query(std::unique_ptr<GPUQuery> query);
    BottleneckAnalysis analyze_bottlenecks() const;
    
    // Platform-specific implementations
    void initialize_d3d11();
    void initialize_opengl();
    void initialize_vulkan();
    GPUMemoryInfo get_d3d11_memory_info() const;
    GPUMemoryInfo get_opengl_memory_info() const;
    GPUCounters get_d3d11_counters() const;
    GPUCounters get_opengl_counters() const;
    
public:
    GPUPerformanceMonitor();
    ~GPUPerformanceMonitor();
    
    // Initialization
    bool initialize(GraphicsAPI api = GraphicsAPI::UNKNOWN);
    void shutdown();
    bool is_initialized() const { return current_api_ != GraphicsAPI::UNKNOWN; }
    GraphicsAPI get_current_api() const { return current_api_; }
    
    // Configuration
    void set_enabled(bool enabled) { enabled_ = enabled; }
    bool is_enabled() const { return enabled_; }
    
    void set_max_frame_history(usize count) { max_frame_history_ = count; }
    void set_max_draw_call_history(usize count) { max_draw_call_history_ = count; }
    void enable_draw_call_tracking(bool enable) { enable_draw_call_tracking_ = enable; }
    void enable_memory_tracking(bool enable) { enable_memory_tracking_ = enable; }
    void enable_shader_tracking(bool enable) { enable_shader_tracking_ = enable; }
    
    // Frame timing
    void begin_frame();
    void end_frame();
    FrameStats get_current_frame_stats() const;
    std::vector<FrameStats> get_frame_history() const;
    f32 get_average_fps(usize frame_count = 60) const;
    microseconds get_average_frame_time(usize frame_count = 60) const;
    
    // Draw call tracking
    void begin_draw_call(const std::string& name, u32 vertex_count, u32 instance_count = 1);
    void end_draw_call();
    void record_indexed_draw_call(const std::string& name, u32 index_count, 
                                 u32 vertex_count, u32 instance_count = 1);
    std::vector<DrawCallInfo> get_recent_draw_calls(usize count = 100) const;
    u32 get_draw_calls_per_frame() const;
    
    // Render pass tracking
    void begin_render_pass(const std::string& name, const std::vector<std::string>& render_targets,
                          bool use_depth = true, bool use_stencil = false);
    void end_render_pass();
    std::vector<RenderPassInfo> get_recent_render_passes(usize count = 10) const;
    
    // Resource tracking
    void record_texture_bind(const std::string& texture_name, usize memory_size = 0);
    void record_shader_bind(const std::string& shader_name);
    void record_buffer_upload(const std::string& buffer_name, usize size);
    void record_texture_upload(const std::string& texture_name, usize size);
    
    // Shader compilation tracking
    void record_shader_compilation(const std::string& shader_name, const std::string& shader_type,
                                  usize source_size, microseconds compilation_time,
                                  bool success, const std::string& error_message = "");
    std::vector<ShaderCompilationInfo> get_shader_compilation_history() const;
    
    // Memory monitoring
    GPUMemoryInfo get_memory_info() const;
    std::vector<GPUMemoryInfo> get_memory_history() const;
    f32 get_memory_pressure() const;
    usize get_texture_memory_usage() const;
    usize get_buffer_memory_usage() const;
    
    // Performance counters
    GPUCounters get_current_counters() const;
    f64 get_gpu_utilization() const;
    f64 get_memory_bandwidth_utilization() const;
    u64 get_triangles_per_second() const;
    u64 get_pixels_per_second() const;
    
    // Analysis
    BottleneckAnalysis analyze_performance_bottlenecks() const;
    std::vector<std::string> get_performance_warnings() const;
    std::vector<std::string> get_optimization_suggestions() const;
    f32 calculate_performance_score() const;
    
    // Event recording
    void begin_gpu_event(const std::string& name, GPUEventType type = GPUEventType::CUSTOM);
    void end_gpu_event();
    void record_custom_event(const std::string& name, microseconds duration, 
                           const std::unordered_map<std::string, std::string>& metadata = {});
    std::vector<GPUEvent> get_recent_events(usize count = 1000) const;
    
    // Reporting
    std::string generate_performance_report() const;
    std::string generate_memory_report() const;
    std::string generate_bottleneck_report() const;
    void export_to_json(const std::string& filename) const;
    void export_frame_times_to_csv(const std::string& filename) const;
    
    // Debug utilities
    void capture_gpu_state() const;
    void dump_current_state() const;
    void log_gpu_info() const;
    
    // Control
    void clear_history();
    void reset_counters();
    void flush_queries();
    
    // Singleton access
    static GPUPerformanceMonitor& instance();
    static void cleanup();
};

// RAII GPU event tracker
class GPUEventScope {
private:
    std::string name_;
    GPUEventType type_;
    
public:
    GPUEventScope(const std::string& name, GPUEventType type = GPUEventType::CUSTOM);
    ~GPUEventScope();
    
    // Non-copyable, moveable
    GPUEventScope(const GPUEventScope&) = delete;
    GPUEventScope& operator=(const GPUEventScope&) = delete;
    GPUEventScope(GPUEventScope&&) = default;
    GPUEventScope& operator=(GPUEventScope&&) = default;
};

// Macros for convenient GPU profiling
#define GPU_PROFILE_SCOPE(name) \
    ecscope::gpu::GPUEventScope _gpu_event(name)

#define GPU_PROFILE_SCOPE_TYPE(name, type) \
    ecscope::gpu::GPUEventScope _gpu_event(name, type)

#define GPU_PROFILE_FRAME() \
    ecscope::gpu::GPUPerformanceMonitor::instance().begin_frame(); \
    auto _frame_guard = std::unique_ptr<void, std::function<void(void*)>>( \
        nullptr, [](void*) { \
            ecscope::gpu::GPUPerformanceMonitor::instance().end_frame(); \
        })

#define GPU_PROFILE_DRAW_CALL(name, vertices, instances) \
    ecscope::gpu::GPUPerformanceMonitor::instance().begin_draw_call(name, vertices, instances); \
    auto _draw_guard = std::unique_ptr<void, std::function<void(void*)>>( \
        nullptr, [](void*) { \
            ecscope::gpu::GPUPerformanceMonitor::instance().end_draw_call(); \
        })

#define GPU_PROFILE_RENDER_PASS(name, targets) \
    ecscope::gpu::GPUPerformanceMonitor::instance().begin_render_pass(name, targets); \
    auto _pass_guard = std::unique_ptr<void, std::function<void(void*)>>( \
        nullptr, [](void*) { \
            ecscope::gpu::GPUPerformanceMonitor::instance().end_render_pass(); \
        })

// GPU performance analysis utilities
namespace analysis {

// Performance threshold configuration
struct PerformanceThresholds {
    f32 target_fps = 60.0f;
    f32 gpu_utilization_warning = 95.0f;
    f32 memory_pressure_warning = 90.0f;
    u32 max_draw_calls_per_frame = 1000;
    microseconds frame_time_warning = microseconds(20000); // 50 FPS
    usize large_texture_threshold = 4 * 1024 * 1024; // 4MB
};

// Detect common performance issues
std::vector<std::string> detect_performance_issues(const GPUPerformanceMonitor& monitor,
                                                  const PerformanceThresholds& thresholds = {});

// Analyze frame time consistency
struct FrameTimeAnalysis {
    f32 average_fps;
    f32 minimum_fps;
    f32 frame_time_variance;
    f32 consistency_score; // 0.0 to 1.0
    bool has_stuttering;
    std::vector<u64> stutter_frames;
};

FrameTimeAnalysis analyze_frame_consistency(const std::vector<FrameStats>& frames);

// Analyze draw call efficiency
struct DrawCallAnalysis {
    f32 average_vertices_per_call;
    f32 draw_call_efficiency_score; // 0.0 to 1.0
    u32 small_draw_call_count; // < 100 vertices
    u32 redundant_state_changes;
    std::vector<std::string> optimization_tips;
};

DrawCallAnalysis analyze_draw_call_efficiency(const std::vector<DrawCallInfo>& draw_calls);

// Memory usage analysis
struct MemoryAnalysis {
    f32 peak_memory_usage_mb;
    f32 average_memory_usage_mb;
    f32 memory_growth_rate; // MB per second
    bool potential_memory_leak;
    std::unordered_map<std::string, usize> resource_breakdown;
};

MemoryAnalysis analyze_memory_usage(const std::vector<GPUMemoryInfo>& memory_history);

} // namespace analysis

} // namespace ecscope::gpu