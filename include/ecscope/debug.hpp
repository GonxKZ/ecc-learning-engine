#pragma once

/**
 * @file debug.hpp
 * @brief ECScope Comprehensive Debug System - Professional Game Development Debugging
 * 
 * This header provides access to ECScope's complete debugging and profiling system,
 * designed for professional game development with production-quality tools.
 * 
 * Features:
 * - High-precision CPU profiler with hierarchical sampling
 * - Memory profiler with allocation tracking and leak detection  
 * - GPU profiler for render timing and resource usage
 * - Network profiler for bandwidth and latency analysis
 * - Asset loading profiler with bottleneck identification
 * - Custom event profiling with user-defined markers
 * - Real-time performance graphs and charts
 * - Memory usage visualization with heap maps
 * - ECS entity relationship visualizer
 * - Physics debug rendering (collision shapes, forces)
 * - Rendering debug views (wireframe, normals, overdraw)
 * - Network topology and message flow visualization
 * - Entity inspector with component editing
 * - System performance inspector with timing
 * - Asset inspector with dependency graphs
 * - Memory inspector with allocation trees
 * - Shader inspector with reflection data
 * - Job system inspector with fiber states
 * - Interactive command console with auto-completion
 * - Variable inspection and live editing
 * - Script execution environment
 * - Log viewer with filtering and search
 * - Remote debugging capabilities
 * - Crash dump analysis tools
 * 
 * @author ECScope Team
 * @version 1.0.0
 */

// Core debug system
#include "debug/debug_system.hpp"

// Profiling tools
#include "debug/profilers.hpp"

// Visual debugging
#include "debug/visualizers.hpp"

// Runtime inspection
#include "debug/inspectors.hpp"

// Debug console
#include "debug/console.hpp"

// Debug rendering
#include "debug/debug_renderer.hpp"

namespace ECScope::Debug {

/**
 * @brief Global debug system instance
 * 
 * Provides convenient access to the debug system without having to
 * pass around references. Initialize once at application startup.
 */
class GlobalDebugSystem {
public:
    static void Initialize(const DebugSystem::Config& config = {});
    static void Shutdown();
    static DebugSystem& Get();
    static bool IsInitialized();

private:
    static std::unique_ptr<DebugSystem> s_instance;
    static bool s_initialized;
};

/**
 * @brief Debug system factory for easy setup
 */
class DebugSystemBuilder {
public:
    DebugSystemBuilder();
    
    // Configuration
    DebugSystemBuilder& WithProfiling(bool enable = true);
    DebugSystemBuilder& WithVisualization(bool enable = true);
    DebugSystemBuilder& WithInspection(bool enable = true);
    DebugSystemBuilder& WithConsole(bool enable = true);
    DebugSystemBuilder& WithRemoteDebugging(bool enable = true, uint16_t port = 7777);
    DebugSystemBuilder& WithMinimalPerformanceImpact(bool enable = true);
    DebugSystemBuilder& WithMemoryBudget(size_t bytes);
    DebugSystemBuilder& WithProfilerSamples(int max_samples);
    DebugSystemBuilder& WithUpdateFrequency(float frequency);
    
    // Profiler setup
    DebugSystemBuilder& WithCPUProfiler(const std::string& name = "CPU");
    DebugSystemBuilder& WithMemoryProfiler(const std::string& name = "Memory");
    DebugSystemBuilder& WithGPUProfiler(const std::string& name = "GPU");
    DebugSystemBuilder& WithNetworkProfiler(const std::string& name = "Network");
    DebugSystemBuilder& WithAssetProfiler(const std::string& name = "Assets");
    DebugSystemBuilder& WithCustomEventProfiler(const std::string& name = "Events");
    
    // Visualizer setup
    DebugSystemBuilder& WithPerformanceGraphs();
    DebugSystemBuilder& WithMemoryVisualization();
    DebugSystemBuilder& WithECSVisualization();
    DebugSystemBuilder& WithPhysicsDebugDraw();
    DebugSystemBuilder& WithRenderingDebugViews();
    DebugSystemBuilder& WithNetworkVisualization();
    
    // Inspector setup
    DebugSystemBuilder& WithEntityInspector();
    DebugSystemBuilder& WithSystemInspector();
    DebugSystemBuilder& WithAssetInspector();
    DebugSystemBuilder& WithMemoryInspector();
    DebugSystemBuilder& WithShaderInspector();
    DebugSystemBuilder& WithJobSystemInspector();
    
    // Build system
    std::unique_ptr<DebugSystem> Build();
    std::unique_ptr<DebugSystem> BuildAndSetGlobal();

private:
    DebugSystem::Config m_config;
    std::vector<std::string> m_enabled_profilers;
    std::vector<std::string> m_enabled_visualizers;
    std::vector<std::string> m_enabled_inspectors;
};

/**
 * @brief Quick setup presets for common debugging scenarios
 */
namespace Presets {

    /**
     * @brief Minimal debug setup for performance-critical applications
     */
    std::unique_ptr<DebugSystem> CreateMinimal();
    
    /**
     * @brief Development setup with full debugging capabilities
     */
    std::unique_ptr<DebugSystem> CreateDevelopment();
    
    /**
     * @brief Performance analysis setup focused on optimization
     */
    std::unique_ptr<DebugSystem> CreatePerformanceAnalysis();
    
    /**
     * @brief Memory debugging setup for leak detection and analysis
     */
    std::unique_ptr<DebugSystem> CreateMemoryDebugging();
    
    /**
     * @brief Rendering debugging setup for graphics optimization
     */
    std::unique_ptr<DebugSystem> CreateRenderingDebug();
    
    /**
     * @brief Network debugging setup for multiplayer games
     */
    std::unique_ptr<DebugSystem> CreateNetworkDebugging();
    
    /**
     * @brief Complete setup with all features enabled
     */
    std::unique_ptr<DebugSystem> CreateComplete();

} // namespace Presets

/**
 * @brief Utility functions for common debug tasks
 */
namespace Utils {

    // Quick profiling
    void BeginCPUProfile(const std::string& name);
    void EndCPUProfile();
    void ProfileFunction(const std::string& function_name = __FUNCTION__);
    
    // Memory tracking
    void TrackAllocation(void* ptr, size_t size, const std::string& tag = "");
    void TrackDeallocation(void* ptr);
    void CheckMemoryLeaks();
    
    // GPU profiling
    void BeginGPUEvent(const std::string& name);
    void EndGPUEvent();
    
    // Custom events
    void RecordEvent(const std::string& name, const std::string& category = "");
    void BeginEvent(const std::string& name, const std::string& category = "");
    void EndEvent();
    
    // Debug drawing
    void DrawDebugLine(const Vector3& start, const Vector3& end, uint32_t color = DebugRenderer::WHITE);
    void DrawDebugBox(const Vector3& min, const Vector3& max, uint32_t color = DebugRenderer::WHITE);
    void DrawDebugSphere(const Vector3& center, float radius, uint32_t color = DebugRenderer::WHITE);
    void DrawDebugText(const Vector3& position, const std::string& text, uint32_t color = DebugRenderer::WHITE);
    
    // Console shortcuts
    void LogInfo(const std::string& message, const std::string& category = "");
    void LogWarning(const std::string& message, const std::string& category = "");
    void LogError(const std::string& message, const std::string& category = "");
    void ExecuteCommand(const std::string& command);
    
    // System inspection
    void InspectEntity(uint32_t entity_id);
    void InspectSystem(const std::string& system_name);
    void InspectAsset(const std::string& asset_path);
    void InspectMemory();
    
    // Performance monitoring
    double GetFrameTime();
    double GetFPS();
    size_t GetMemoryUsage();
    const PerformanceMonitor::FrameStats& GetFrameStats();

} // namespace Utils

} // namespace ECScope::Debug

// Convenience macros for easy debugging
#define ECSCOPE_DEBUG_INIT() ECScope::Debug::GlobalDebugSystem::Initialize()
#define ECSCOPE_DEBUG_SHUTDOWN() ECScope::Debug::GlobalDebugSystem::Shutdown()
#define ECSCOPE_DEBUG_UPDATE(dt) ECScope::Debug::GlobalDebugSystem::Get().Update(dt)
#define ECSCOPE_DEBUG_RENDER() ECScope::Debug::GlobalDebugSystem::Get().Render()

#define ECSCOPE_PROFILE_FUNCTION() ECScope::Debug::Utils::ProfileFunction(__FUNCTION__)
#define ECSCOPE_PROFILE_SCOPE(name) ECScope::Debug::Utils::BeginCPUProfile(name); \
    auto _profile_guard = [](){ ECScope::Debug::Utils::EndCPUProfile(); }; \
    std::unique_ptr<void, decltype(_profile_guard)> _profile_ptr(nullptr, _profile_guard)

#define ECSCOPE_LOG_INFO(msg) ECScope::Debug::Utils::LogInfo(msg)
#define ECSCOPE_LOG_WARNING(msg) ECScope::Debug::Utils::LogWarning(msg)
#define ECSCOPE_LOG_ERROR(msg) ECScope::Debug::Utils::LogError(msg)

#define ECSCOPE_DRAW_LINE(start, end, color) ECScope::Debug::Utils::DrawDebugLine(start, end, color)
#define ECSCOPE_DRAW_BOX(min, max, color) ECScope::Debug::Utils::DrawDebugBox(min, max, color)
#define ECSCOPE_DRAW_SPHERE(center, radius, color) ECScope::Debug::Utils::DrawDebugSphere(center, radius, color)
#define ECSCOPE_DRAW_TEXT(pos, text, color) ECScope::Debug::Utils::DrawDebugText(pos, text, color)

#define ECSCOPE_TRACK_ALLOC(ptr, size, tag) ECScope::Debug::Utils::TrackAllocation(ptr, size, tag)
#define ECSCOPE_TRACK_FREE(ptr) ECScope::Debug::Utils::TrackDeallocation(ptr)

#define ECSCOPE_EVENT_BEGIN(name, category) ECScope::Debug::Utils::BeginEvent(name, category)
#define ECSCOPE_EVENT_END() ECScope::Debug::Utils::EndEvent()
#define ECSCOPE_EVENT_RECORD(name) ECScope::Debug::Utils::RecordEvent(name)

#define ECSCOPE_GPU_EVENT_BEGIN(name) ECScope::Debug::Utils::BeginGPUEvent(name)
#define ECSCOPE_GPU_EVENT_END() ECScope::Debug::Utils::EndGPUEvent()

// Conditional compilation support
#ifdef ECSCOPE_DEBUG_ENABLED
    #define ECSCOPE_DEBUG_ONLY(code) code
#else
    #define ECSCOPE_DEBUG_ONLY(code)
#endif

#ifdef ECSCOPE_RELEASE_BUILD
    // In release builds, most debug operations become no-ops for performance
    #undef ECSCOPE_PROFILE_FUNCTION
    #undef ECSCOPE_PROFILE_SCOPE
    #undef ECSCOPE_DRAW_LINE
    #undef ECSCOPE_DRAW_BOX
    #undef ECSCOPE_DRAW_SPHERE
    #undef ECSCOPE_DRAW_TEXT
    #undef ECSCOPE_TRACK_ALLOC
    #undef ECSCOPE_TRACK_FREE
    #undef ECSCOPE_EVENT_BEGIN
    #undef ECSCOPE_EVENT_END
    #undef ECSCOPE_EVENT_RECORD
    
    #define ECSCOPE_PROFILE_FUNCTION()
    #define ECSCOPE_PROFILE_SCOPE(name)
    #define ECSCOPE_DRAW_LINE(start, end, color)
    #define ECSCOPE_DRAW_BOX(min, max, color)
    #define ECSCOPE_DRAW_SPHERE(center, radius, color)
    #define ECSCOPE_DRAW_TEXT(pos, text, color)
    #define ECSCOPE_TRACK_ALLOC(ptr, size, tag)
    #define ECSCOPE_TRACK_FREE(ptr)
    #define ECSCOPE_EVENT_BEGIN(name, category)
    #define ECSCOPE_EVENT_END()
    #define ECSCOPE_EVENT_RECORD(name)
#endif