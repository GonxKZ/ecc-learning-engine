#include "ecscope/gpu_performance_monitor.hpp"
#include <algorithm>
#include <numeric>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <thread>

namespace ecscope::gpu {

// Static instance for singleton pattern
static std::unique_ptr<GPUPerformanceMonitor> g_gpu_monitor_instance;
static std::mutex g_gpu_instance_mutex;

// ===== Platform-specific GPU Query Implementations =====

#ifdef _WIN32
D3D11GPUQuery::D3D11GPUQuery(ID3D11Device* device, ID3D11DeviceContext* context)
    : device_(device), context_(context) {
    
    // Create disjoint query for frequency
    D3D11_QUERY_DESC query_desc = {};
    query_desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
    device->CreateQuery(&query_desc, &disjoint_query_);
    
    // Create timestamp queries
    query_desc.Query = D3D11_QUERY_TIMESTAMP;
    device->CreateQuery(&query_desc, &start_query_);
    device->CreateQuery(&query_desc, &end_query_);
}

void D3D11GPUQuery::begin() {
    context_->Begin(disjoint_query_.Get());
    context_->End(start_query_.Get());
}

void D3D11GPUQuery::end() {
    context_->End(end_query_.Get());
    context_->End(disjoint_query_.Get());
}

bool D3D11GPUQuery::is_ready() const {
    return context_->GetData(disjoint_query_.Get(), nullptr, 0, D3D11_ASYNC_GETDATA_DONOTFLUSH) == S_OK;
}

microseconds D3D11GPUQuery::get_result() const {
    if (!is_ready()) {
        return microseconds(0);
    }
    
    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint_data;
    context_->GetData(disjoint_query_.Get(), &disjoint_data, sizeof(disjoint_data), 0);
    
    if (disjoint_data.Disjoint) {
        return microseconds(0); // Invalid timing data
    }
    
    u64 start_time, end_time;
    context_->GetData(start_query_.Get(), &start_time, sizeof(start_time), 0);
    context_->GetData(end_query_.Get(), &end_time, sizeof(end_time), 0);
    
    u64 delta = end_time - start_time;
    u64 microsec = (delta * 1000000) / disjoint_data.Frequency;
    
    return microseconds(microsec);
}

void D3D11GPUQuery::reset() {
    // Queries are reset automatically when reused in D3D11
}
#endif

#if defined(ECSCOPE_USE_OPENGL)
OpenGLGPUQuery::OpenGLGPUQuery() : query_active_(false) {
    glGenQueries(1, &query_id_);
}

OpenGLGPUQuery::~OpenGLGPUQuery() {
    if (query_id_ != 0) {
        glDeleteQueries(1, &query_id_);
    }
}

void OpenGLGPUQuery::begin() {
    glBeginQuery(GL_TIME_ELAPSED, query_id_);
    query_active_ = true;
}

void OpenGLGPUQuery::end() {
    if (query_active_) {
        glEndQuery(GL_TIME_ELAPSED);
        query_active_ = false;
    }
}

bool OpenGLGPUQuery::is_ready() const {
    if (query_active_) return false;
    
    GLint available = 0;
    glGetQueryObjectiv(query_id_, GL_QUERY_RESULT_AVAILABLE, &available);
    return available == GL_TRUE;
}

microseconds OpenGLGPUQuery::get_result() const {
    if (!is_ready()) {
        return microseconds(0);
    }
    
    GLuint64 time_ns = 0;
    glGetQueryObjectui64v(query_id_, GL_QUERY_RESULT, &time_ns);
    return microseconds(time_ns / 1000); // Convert nanoseconds to microseconds
}

void OpenGLGPUQuery::reset() {
    // OpenGL queries are reset when reused
}
#endif

// ===== GPUPerformanceMonitor Implementation =====

GPUPerformanceMonitor::GPUPerformanceMonitor()
    : current_api_(GraphicsAPI::UNKNOWN)
    , last_update_time_(high_resolution_clock::now()) {
    
    // Reserve space for performance data
    frame_history_.reserve(max_frame_history_);
    draw_call_history_.reserve(max_draw_call_history_);
    render_pass_history_.reserve(100);
    memory_history_.reserve(1000);
    event_history_.reserve(max_event_history_);
}

GPUPerformanceMonitor::~GPUPerformanceMonitor() {
    shutdown();
}

bool GPUPerformanceMonitor::initialize(GraphicsAPI api) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    if (api == GraphicsAPI::UNKNOWN) {
        // Auto-detect API
        #ifdef _WIN32
            api = GraphicsAPI::DIRECT3D11; // Default to D3D11 on Windows
        #else
            api = GraphicsAPI::OPENGL; // Default to OpenGL on other platforms
        #endif
    }
    
    current_api_ = api;
    
    try {
        switch (api) {
            case GraphicsAPI::DIRECT3D11:
                initialize_d3d11();
                break;
            case GraphicsAPI::OPENGL:
                initialize_opengl();
                break;
            case GraphicsAPI::VULKAN:
                initialize_vulkan();
                break;
            default:
                current_api_ = GraphicsAPI::UNKNOWN;
                return false;
        }
        
        create_gpu_queries();
        return true;
        
    } catch (...) {
        current_api_ = GraphicsAPI::UNKNOWN;
        return false;
    }
}

void GPUPerformanceMonitor::initialize_d3d11() {
#ifdef _WIN32
    // Create DXGI factory to enumerate adapters
    Microsoft::WRL::ComPtr<IDXGIFactory> factory;
    CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(factory.GetAddressOf()));
    
    // Get the primary adapter
    factory->EnumAdapters(0, dxgi_adapter_.GetAddressOf());
    
    // Create D3D11 device and context
    D3D_FEATURE_LEVEL feature_levels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };
    
    HRESULT hr = D3D11CreateDevice(
        dxgi_adapter_.Get(),
        D3D_DRIVER_TYPE_UNKNOWN,
        nullptr,
        0,
        feature_levels,
        ARRAYSIZE(feature_levels),
        D3D11_SDK_VERSION,
        d3d11_device_.GetAddressOf(),
        nullptr,
        d3d11_context_.GetAddressOf()
    );
    
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create D3D11 device");
    }
#endif
}

void GPUPerformanceMonitor::initialize_opengl() {
#if defined(ECSCOPE_USE_OPENGL)
    // OpenGL initialization is typically handled by the application
    // We just check if the context is available
    
    // Verify OpenGL context is current
    #ifdef _WIN32
        if (!wglGetCurrentContext()) {
            throw std::runtime_error("No OpenGL context available");
        }
    #elif defined(__linux__)
        if (!glXGetCurrentContext()) {
            throw std::runtime_error("No OpenGL context available");
        }
    #endif
    
    // Check for required extensions
    const char* extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    if (!strstr(extensions, "GL_ARB_timer_query") && !strstr(extensions, "GL_EXT_timer_query")) {
        throw std::runtime_error("Timer query extension not available");
    }
#endif
}

void GPUPerformanceMonitor::initialize_vulkan() {
#if defined(ECSCOPE_USE_VULKAN)
    // Vulkan initialization would be more complex
    // This is a placeholder implementation
    throw std::runtime_error("Vulkan support not yet implemented");
#else
    throw std::runtime_error("Vulkan support not compiled");
#endif
}

void GPUPerformanceMonitor::create_gpu_queries() {
    // Create a pool of GPU queries for timing
    for (usize i = 0; i < max_queries_; ++i) {
        std::unique_ptr<GPUQuery> query;
        
        switch (current_api_) {
#ifdef _WIN32
            case GraphicsAPI::DIRECT3D11:
                query = std::make_unique<D3D11GPUQuery>(d3d11_device_.Get(), d3d11_context_.Get());
                break;
#endif
#if defined(ECSCOPE_USE_OPENGL)
            case GraphicsAPI::OPENGL:
                query = std::make_unique<OpenGLGPUQuery>();
                break;
#endif
            default:
                continue;
        }
        
        if (query) {
            available_queries_.push(std::move(query));
        }
    }
}

void GPUPerformanceMonitor::shutdown() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    cleanup_api();
    current_api_ = GraphicsAPI::UNKNOWN;
    
    // Clear query pools
    while (!available_queries_.empty()) {
        available_queries_.pop();
    }
    active_queries_.clear();
}

void GPUPerformanceMonitor::cleanup_api() {
#ifdef _WIN32
    d3d11_context_.Reset();
    d3d11_device_.Reset();
    dxgi_adapter_.Reset();
#endif
}

void GPUPerformanceMonitor::begin_frame() {
    if (!enabled_ || current_api_ == GraphicsAPI::UNKNOWN) return;
    
    frame_start_time_ = high_resolution_clock::now();
    
    // Reset per-frame counters
    draw_call_counter_ = 0;
    vertex_counter_ = 0;
    triangle_counter_ = 0;
}

void GPUPerformanceMonitor::end_frame() {
    if (!enabled_ || current_api_ == GraphicsAPI::UNKNOWN) return;
    
    auto frame_end_time = high_resolution_clock::now();
    auto frame_time = duration_cast<microseconds>(frame_end_time - frame_start_time_);
    
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    // Create frame statistics
    FrameStats frame_stats;
    frame_stats.frame_number = ++frame_counter_;
    frame_stats.frame_time = frame_time;
    frame_stats.cpu_time = frame_time; // Simplified - would need CPU/GPU separation
    frame_stats.gpu_time = microseconds(0); // Would be calculated from GPU queries
    frame_stats.present_time = microseconds(0);
    frame_stats.draw_call_count = draw_call_counter_;
    frame_stats.vertex_count = vertex_counter_;
    frame_stats.triangle_count = triangle_counter_;
    frame_stats.texture_bind_count = 0; // Would be tracked
    frame_stats.shader_bind_count = 0; // Would be tracked
    frame_stats.fps = frame_time.count() > 0 ? 1000000.0f / frame_time.count() : 0.0f;
    frame_stats.timestamp = frame_end_time;
    
    // Add to history
    frame_history_.push_back(frame_stats);
    if (frame_history_.size() > max_frame_history_) {
        frame_history_.erase(frame_history_.begin());
    }
    
    // Update performance counters periodically
    auto now = high_resolution_clock::now();
    if (duration_cast<milliseconds>(now - last_update_time_).count() >= 100) {
        update_gpu_counters();
        update_memory_info();
        last_update_time_ = now;
    }
}

void GPUPerformanceMonitor::begin_draw_call(const std::string& name, u32 vertex_count, u32 instance_count) {
    if (!enabled_ || !enable_draw_call_tracking_) return;
    
    // Start GPU timing query
    auto query = get_available_query();
    if (query) {
        query->begin();
        active_queries_[name] = std::move(query);
    }
    
    ++draw_call_counter_;
    vertex_counter_ += vertex_count * instance_count;
    triangle_counter_ += (vertex_count / 3) * instance_count; // Simplified triangle count
}

void GPUPerformanceMonitor::end_draw_call() {
    if (!enabled_ || !enable_draw_call_tracking_) return;
    
    // End GPU timing (simplified - would need to match with begin_draw_call)
    // This is a simplified implementation
    for (auto& [name, query] : active_queries_) {
        query->end();
        
        DrawCallInfo draw_info;
        draw_info.name = name;
        draw_info.timestamp = high_resolution_clock::now();
        draw_info.gpu_time = microseconds(0); // Would get from query when ready
        
        std::lock_guard<std::mutex> lock(data_mutex_);
        draw_call_history_.push_back(draw_info);
        if (draw_call_history_.size() > max_draw_call_history_) {
            draw_call_history_.erase(draw_call_history_.begin());
        }
        
        return_query(std::move(query));
        break; // Simplified - handle first query
    }
    
    active_queries_.clear();
}

void GPUPerformanceMonitor::begin_render_pass(const std::string& name,
                                             const std::vector<std::string>& render_targets,
                                             bool use_depth, bool use_stencil) {
    if (!enabled_) return;
    
    // Start timing for render pass
    thread_local std::unordered_map<std::string, high_resolution_clock::time_point> render_pass_starts;
    render_pass_starts[name] = high_resolution_clock::now();
    
    // Store render pass info for later completion
    thread_local std::unordered_map<std::string, RenderPassInfo> active_passes;
    RenderPassInfo& pass_info = active_passes[name];
    pass_info.name = name;
    pass_info.render_targets = render_targets;
    pass_info.render_target_count = static_cast<u32>(render_targets.size());
    pass_info.uses_depth_buffer = use_depth;
    pass_info.uses_stencil_buffer = use_stencil;
    pass_info.start_time = render_pass_starts[name];
    pass_info.draw_call_count = 0;
    pass_info.vertex_count = 0;
    pass_info.triangle_count = 0;
}

void GPUPerformanceMonitor::end_render_pass() {
    if (!enabled_) return;
    
    // This is simplified - in a real implementation, we'd match passes by name
    thread_local std::unordered_map<std::string, RenderPassInfo> active_passes;
    
    if (!active_passes.empty()) {
        auto& [name, pass_info] = *active_passes.begin();
        pass_info.end_time = high_resolution_clock::now();
        pass_info.total_gpu_time = duration_cast<microseconds>(pass_info.end_time - pass_info.start_time);
        
        std::lock_guard<std::mutex> lock(data_mutex_);
        render_pass_history_.push_back(pass_info);
        if (render_pass_history_.size() > 100) {
            render_pass_history_.erase(render_pass_history_.begin());
        }
        
        active_passes.erase(active_passes.begin());
    }
}

void GPUPerformanceMonitor::record_shader_compilation(const std::string& shader_name,
                                                     const std::string& shader_type,
                                                     usize source_size,
                                                     microseconds compilation_time,
                                                     bool success,
                                                     const std::string& error_message) {
    if (!enabled_ || !enable_shader_tracking_) return;
    
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    ShaderCompilationInfo info;
    info.shader_name = shader_name;
    info.shader_type = shader_type;
    info.source_size = source_size;
    info.compilation_time = compilation_time;
    info.compilation_success = success;
    info.error_message = error_message;
    info.timestamp = high_resolution_clock::now();
    
    shader_compilation_history_.push_back(info);
    if (shader_compilation_history_.size() > 1000) {
        shader_compilation_history_.erase(shader_compilation_history_.begin());
    }
}

GPUMemoryInfo GPUPerformanceMonitor::get_memory_info() const {
    if (!enabled_ || current_api_ == GraphicsAPI::UNKNOWN) {
        return GPUMemoryInfo{};
    }
    
    switch (current_api_) {
        case GraphicsAPI::DIRECT3D11:
            return get_d3d11_memory_info();
        case GraphicsAPI::OPENGL:
            return get_opengl_memory_info();
        default:
            return GPUMemoryInfo{};
    }
}

GPUMemoryInfo GPUPerformanceMonitor::get_d3d11_memory_info() const {
    GPUMemoryInfo info = {};
    
#ifdef _WIN32
    if (dxgi_adapter_) {
        DXGI_ADAPTER_DESC adapter_desc;
        if (SUCCEEDED(dxgi_adapter_->GetDesc(&adapter_desc))) {
            info.total_memory = static_cast<usize>(adapter_desc.DedicatedVideoMemory);
            info.available_memory = info.total_memory; // Simplified
            info.used_memory = 0; // Would need additional queries
        }
    }
#endif
    
    info.timestamp = high_resolution_clock::now();
    return info;
}

GPUMemoryInfo GPUPerformanceMonitor::get_opengl_memory_info() const {
    GPUMemoryInfo info = {};
    
#if defined(ECSCOPE_USE_OPENGL)
    // NVIDIA GPU memory info extension
    GLint total_kb = 0, available_kb = 0;
    
    // Try NVIDIA extension first
    if (glGetError() == GL_NO_ERROR) {
        glGetIntegerv(0x9047, &total_kb); // GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX
        glGetIntegerv(0x9049, &available_kb); // GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX
        
        if (glGetError() == GL_NO_ERROR) {
            info.total_memory = static_cast<usize>(total_kb) * 1024;
            info.available_memory = static_cast<usize>(available_kb) * 1024;
            info.used_memory = info.total_memory - info.available_memory;
        }
    }
    
    // Try ATI extension as fallback
    if (info.total_memory == 0) {
        GLint params[4] = {0};
        glGetIntegerv(0x87FB, params); // GL_TEXTURE_FREE_MEMORY_ATI
        if (glGetError() == GL_NO_ERROR) {
            info.available_memory = static_cast<usize>(params[0]) * 1024;
            info.total_memory = info.available_memory * 2; // Estimate
            info.used_memory = info.total_memory - info.available_memory;
        }
    }
#endif
    
    info.timestamp = high_resolution_clock::now();
    return info;
}

void GPUPerformanceMonitor::update_gpu_counters() {
    // This would query hardware performance counters
    // Implementation is highly platform/driver specific
    
    current_counters_.timestamp = high_resolution_clock::now();
    
    // Simplified placeholder values
    current_counters_.gpu_utilization = 50.0; // Would come from driver
    current_counters_.memory_bandwidth_utilization = 30.0;
    current_counters_.texture_cache_hit_rate = 95.0;
    current_counters_.vertex_cache_hit_rate = 90.0;
    
    // Calculate rates from counters
    if (!frame_history_.empty()) {
        auto recent_frames = std::min(static_cast<usize>(60), frame_history_.size());
        auto frame_time_sum = std::accumulate(
            frame_history_.end() - recent_frames, frame_history_.end(),
            microseconds(0),
            [](microseconds sum, const FrameStats& frame) {
                return sum + frame.frame_time;
            });
        
        if (frame_time_sum.count() > 0) {
            f64 seconds = frame_time_sum.count() / 1000000.0;
            current_counters_.vertices_processed_per_second = 
                static_cast<u64>(vertex_counter_.load() / seconds);
            current_counters_.draw_calls_per_second = 
                static_cast<u64>(draw_call_counter_.load() / seconds);
        }
    }
}

void GPUPerformanceMonitor::update_memory_info() {
    if (!enable_memory_tracking_) return;
    
    auto memory_info = get_memory_info();
    
    std::lock_guard<std::mutex> lock(data_mutex_);
    memory_history_.push_back(memory_info);
    if (memory_history_.size() > 1000) {
        memory_history_.erase(memory_history_.begin());
    }
}

std::unique_ptr<GPUQuery> GPUPerformanceMonitor::get_available_query() {
    if (available_queries_.empty()) {
        return nullptr;
    }
    
    auto query = std::move(available_queries_.front());
    available_queries_.pop();
    return query;
}

void GPUPerformanceMonitor::return_query(std::unique_ptr<GPUQuery> query) {
    if (query) {
        query->reset();
        available_queries_.push(std::move(query));
    }
}

BottleneckAnalysis GPUPerformanceMonitor::analyze_bottlenecks() const {
    BottleneckAnalysis analysis;
    analysis.primary_bottleneck = BottleneckAnalysis::BottleneckType::NONE;
    analysis.secondary_bottleneck = BottleneckAnalysis::BottleneckType::NONE;
    analysis.bottleneck_severity = 0.0f;
    analysis.confidence = 0.5f;
    
    // Analyze GPU utilization
    if (current_counters_.gpu_utilization > 95.0) {
        analysis.primary_bottleneck = BottleneckAnalysis::BottleneckType::GPU_COMPUTE;
        analysis.bottleneck_severity = (current_counters_.gpu_utilization - 95.0f) / 5.0f;
        analysis.description = "GPU compute units are saturated";
        analysis.recommendations.push_back("Reduce shader complexity");
        analysis.recommendations.push_back("Optimize geometry complexity");
        analysis.confidence = 0.8f;
    }
    
    // Analyze memory bandwidth
    if (current_counters_.memory_bandwidth_utilization > 90.0) {
        if (analysis.primary_bottleneck == BottleneckAnalysis::BottleneckType::NONE) {
            analysis.primary_bottleneck = BottleneckAnalysis::BottleneckType::MEMORY_BANDWIDTH;
        } else {
            analysis.secondary_bottleneck = BottleneckAnalysis::BottleneckType::MEMORY_BANDWIDTH;
        }
        analysis.description += " Memory bandwidth is saturated";
        analysis.recommendations.push_back("Reduce texture resolution");
        analysis.recommendations.push_back("Use texture compression");
        analysis.recommendations.push_back("Optimize memory access patterns");
    }
    
    // Analyze draw call overhead
    if (!frame_history_.empty()) {
        u32 avg_draw_calls = 0;
        for (const auto& frame : frame_history_) {
            avg_draw_calls += frame.draw_call_count;
        }
        avg_draw_calls /= static_cast<u32>(frame_history_.size());
        
        if (avg_draw_calls > 1000) {
            if (analysis.primary_bottleneck == BottleneckAnalysis::BottleneckType::NONE) {
                analysis.primary_bottleneck = BottleneckAnalysis::BottleneckType::DRAW_CALL_OVERHEAD;
            } else {
                analysis.secondary_bottleneck = BottleneckAnalysis::BottleneckType::DRAW_CALL_OVERHEAD;
            }
            analysis.description += " High draw call count detected";
            analysis.recommendations.push_back("Batch similar draw calls");
            analysis.recommendations.push_back("Use instanced rendering");
            analysis.recommendations.push_back("Implement frustum culling");
        }
    }
    
    return analysis;
}

f32 GPUPerformanceMonitor::get_average_fps(usize frame_count) const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    if (frame_history_.empty()) return 0.0f;
    
    usize count = std::min(frame_count, frame_history_.size());
    f32 total_fps = 0.0f;
    
    for (auto it = frame_history_.end() - count; it != frame_history_.end(); ++it) {
        total_fps += it->fps;
    }
    
    return total_fps / count;
}

std::string GPUPerformanceMonitor::generate_performance_report() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    std::ostringstream report;
    
    report << "=== GPU Performance Report ===\n\n";
    
    // API information
    const char* api_name = "Unknown";
    switch (current_api_) {
        case GraphicsAPI::DIRECT3D11: api_name = "Direct3D 11"; break;
        case GraphicsAPI::DIRECT3D12: api_name = "Direct3D 12"; break;
        case GraphicsAPI::OPENGL: api_name = "OpenGL"; break;
        case GraphicsAPI::VULKAN: api_name = "Vulkan"; break;
        case GraphicsAPI::METAL: api_name = "Metal"; break;
    }
    report << "Graphics API: " << api_name << "\n\n";
    
    // Frame statistics
    if (!frame_history_.empty()) {
        f32 avg_fps = get_average_fps(60);
        microseconds avg_frame_time = get_average_frame_time(60);
        
        report << "Frame Statistics (last 60 frames):\n";
        report << "  Average FPS: " << std::fixed << std::setprecision(2) << avg_fps << "\n";
        report << "  Average Frame Time: " << avg_frame_time.count() << " μs\n";
        report << "  Total Frames: " << frame_counter_ << "\n\n";
    }
    
    // Draw call statistics
    if (!draw_call_history_.empty()) {
        u32 total_draw_calls = 0;
        u32 total_vertices = 0;
        
        usize recent_count = std::min(static_cast<usize>(1000), draw_call_history_.size());
        for (auto it = draw_call_history_.end() - recent_count; it != draw_call_history_.end(); ++it) {
            total_draw_calls++;
            total_vertices += it->vertex_count;
        }
        
        report << "Draw Call Statistics (last 1000 calls):\n";
        report << "  Total Draw Calls: " << total_draw_calls << "\n";
        report << "  Total Vertices: " << total_vertices << "\n";
        if (total_draw_calls > 0) {
            report << "  Average Vertices per Call: " << (total_vertices / total_draw_calls) << "\n";
        }
        report << "\n";
    }
    
    // Memory statistics
    auto memory_info = get_memory_info();
    report << "Memory Statistics:\n";
    report << "  Total GPU Memory: " << (memory_info.total_memory / (1024 * 1024)) << " MB\n";
    report << "  Available Memory: " << (memory_info.available_memory / (1024 * 1024)) << " MB\n";
    report << "  Used Memory: " << (memory_info.used_memory / (1024 * 1024)) << " MB\n";
    report << "  Memory Pressure: " << (memory_info.memory_pressure * 100.0f) << "%\n\n";
    
    // Performance counters
    report << "Performance Counters:\n";
    report << "  GPU Utilization: " << current_counters_.gpu_utilization << "%\n";
    report << "  Memory Bandwidth Utilization: " << current_counters_.memory_bandwidth_utilization << "%\n";
    report << "  Texture Cache Hit Rate: " << current_counters_.texture_cache_hit_rate << "%\n";
    report << "  Vertex Cache Hit Rate: " << current_counters_.vertex_cache_hit_rate << "%\n\n";
    
    // Bottleneck analysis
    auto bottleneck = analyze_bottlenecks();
    if (bottleneck.primary_bottleneck != BottleneckAnalysis::BottleneckType::NONE) {
        report << "Performance Analysis:\n";
        report << "  Primary Bottleneck: " << bottleneck.description << "\n";
        report << "  Severity: " << (bottleneck.bottleneck_severity * 100.0f) << "%\n";
        report << "  Confidence: " << (bottleneck.confidence * 100.0f) << "%\n";
        
        if (!bottleneck.recommendations.empty()) {
            report << "  Recommendations:\n";
            for (const auto& rec : bottleneck.recommendations) {
                report << "    - " << rec << "\n";
            }
        }
        report << "\n";
    }
    
    return report.str();
}

void GPUPerformanceMonitor::clear_history() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    frame_history_.clear();
    draw_call_history_.clear();
    render_pass_history_.clear();
    memory_history_.clear();
    shader_compilation_history_.clear();
    event_history_.clear();
}

GPUPerformanceMonitor& GPUPerformanceMonitor::instance() {
    std::lock_guard<std::mutex> lock(g_gpu_instance_mutex);
    if (!g_gpu_monitor_instance) {
        g_gpu_monitor_instance = std::make_unique<GPUPerformanceMonitor>();
    }
    return *g_gpu_monitor_instance;
}

void GPUPerformanceMonitor::cleanup() {
    std::lock_guard<std::mutex> lock(g_gpu_instance_mutex);
    g_gpu_monitor_instance.reset();
}

// ===== GPUEventScope Implementation =====

GPUEventScope::GPUEventScope(const std::string& name, GPUEventType type)
    : name_(name), type_(type) {
    GPUPerformanceMonitor::instance().begin_gpu_event(name_, type_);
}

GPUEventScope::~GPUEventScope() {
    GPUPerformanceMonitor::instance().end_gpu_event();
}

// ===== Analysis Utilities Implementation =====

namespace analysis {

std::vector<std::string> detect_performance_issues(const GPUPerformanceMonitor& monitor,
                                                   const PerformanceThresholds& thresholds) {
    std::vector<std::string> issues;
    
    // Check frame rate
    f32 avg_fps = monitor.get_average_fps(60);
    if (avg_fps < thresholds.target_fps) {
        std::ostringstream oss;
        oss << "Low frame rate: " << std::fixed << std::setprecision(1) << avg_fps 
            << " FPS (target: " << thresholds.target_fps << " FPS)";
        issues.push_back(oss.str());
    }
    
    // Check GPU utilization
    f64 gpu_util = monitor.get_gpu_utilization();
    if (gpu_util > thresholds.gpu_utilization_warning) {
        std::ostringstream oss;
        oss << "High GPU utilization: " << std::fixed << std::setprecision(1) << gpu_util 
            << "% (threshold: " << thresholds.gpu_utilization_warning << "%)";
        issues.push_back(oss.str());
    }
    
    // Check memory pressure
    f32 memory_pressure = monitor.get_memory_pressure();
    if (memory_pressure > thresholds.memory_pressure_warning / 100.0f) {
        std::ostringstream oss;
        oss << "High GPU memory pressure: " << std::fixed << std::setprecision(1) 
            << (memory_pressure * 100.0f) << "% (threshold: " 
            << thresholds.memory_pressure_warning << "%)";
        issues.push_back(oss.str());
    }
    
    return issues;
}

FrameTimeAnalysis analyze_frame_consistency(const std::vector<FrameStats>& frames) {
    FrameTimeAnalysis analysis = {};
    
    if (frames.empty()) {
        return analysis;
    }
    
    // Calculate basic statistics
    f32 total_fps = 0.0f;
    f32 min_fps = std::numeric_limits<f32>::max();
    std::vector<f32> frame_times;
    
    for (const auto& frame : frames) {
        total_fps += frame.fps;
        min_fps = std::min(min_fps, frame.fps);
        frame_times.push_back(static_cast<f32>(frame.frame_time.count()) / 1000.0f); // Convert to ms
    }
    
    analysis.average_fps = total_fps / frames.size();
    analysis.minimum_fps = min_fps;
    
    // Calculate variance
    f32 mean_frame_time = std::accumulate(frame_times.begin(), frame_times.end(), 0.0f) / frame_times.size();
    f32 variance = 0.0f;
    
    for (f32 time : frame_times) {
        variance += (time - mean_frame_time) * (time - mean_frame_time);
    }
    variance /= frame_times.size();
    analysis.frame_time_variance = variance;
    
    // Calculate consistency score (lower variance = higher consistency)
    analysis.consistency_score = 1.0f / (1.0f + variance / mean_frame_time);
    
    // Detect stuttering (frame times > 150% of average)
    f32 stutter_threshold = mean_frame_time * 1.5f;
    for (usize i = 0; i < frames.size(); ++i) {
        if (frame_times[i] > stutter_threshold) {
            analysis.stutter_frames.push_back(frames[i].frame_number);
        }
    }
    
    analysis.has_stuttering = !analysis.stutter_frames.empty();
    
    return analysis;
}

} // namespace analysis

} // namespace ecscope::gpu