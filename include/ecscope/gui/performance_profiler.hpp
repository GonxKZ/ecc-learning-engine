#pragma once

#include "../core/types.hpp"
#include "../memory/memory_tracker.hpp"
#include <chrono>
#include <atomic>
#include <vector>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <string>
#include <deque>
#include <functional>
#include <array>

namespace ecscope::gui::performance {

// Forward declarations
class ProfilerSession;
class FrameProfiler;
class MemoryProfiler;
class GPUProfiler;
class NetworkProfiler;

// Performance metrics structure
struct PerformanceMetrics {
    // Frame timing
    float frame_time_ms = 0.0f;
    float fps = 0.0f;
    float fps_min = 0.0f;
    float fps_max = 0.0f;
    float frame_time_variance = 0.0f;
    
    // CPU metrics
    float cpu_usage_percent = 0.0f;
    float ui_thread_time_ms = 0.0f;
    float render_thread_time_ms = 0.0f;
    float worker_threads_time_ms = 0.0f;
    size_t draw_calls = 0;
    size_t triangles_rendered = 0;
    
    // Memory metrics
    size_t memory_allocated_bytes = 0;
    size_t memory_reserved_bytes = 0;
    size_t memory_peak_bytes = 0;
    size_t allocations_per_frame = 0;
    size_t deallocations_per_frame = 0;
    float memory_fragmentation = 0.0f;
    
    // GPU metrics
    float gpu_time_ms = 0.0f;
    float gpu_usage_percent = 0.0f;
    size_t gpu_memory_used_bytes = 0;
    size_t texture_memory_bytes = 0;
    size_t buffer_memory_bytes = 0;
    size_t shader_switches = 0;
    size_t texture_binds = 0;
    size_t render_target_switches = 0;
    
    // Cache metrics
    size_t cache_hits = 0;
    size_t cache_misses = 0;
    float cache_hit_rate = 0.0f;
    size_t cache_memory_bytes = 0;
    
    // Platform-specific metrics
    float battery_level = 100.0f;
    float temperature_celsius = 0.0f;
    bool thermal_throttling = false;
    bool power_saving_mode = false;
};

// Profiler scope for automatic timing
class ProfileScope {
public:
    ProfileScope(const std::string& name, ProfilerSession* session = nullptr);
    ~ProfileScope();
    
    void AddCounter(const std::string& name, int64_t value);
    void AddGauge(const std::string& name, float value);
    
private:
    std::string m_name;
    ProfilerSession* m_session;
    std::chrono::high_resolution_clock::time_point m_start;
    std::unordered_map<std::string, int64_t> m_counters;
    std::unordered_map<std::string, float> m_gauges;
};

// Frame profiler for tracking frame-level performance
class FrameProfiler {
public:
    static constexpr size_t HISTORY_SIZE = 240; // 4 seconds at 60 FPS
    
    FrameProfiler();
    
    void BeginFrame();
    void EndFrame();
    
    void RecordEvent(const std::string& name, float duration_ms);
    void RecordDrawCall(size_t triangles);
    
    float GetAverageFrameTime() const;
    float GetMinFrameTime() const;
    float GetMaxFrameTime() const;
    float GetPercentile(float percentile) const;
    float GetFrameTimeVariance() const;
    
    const std::deque<float>& GetFrameHistory() const { return m_frame_history; }
    const std::unordered_map<std::string, float>& GetEventTimings() const { return m_event_timings; }
    
private:
    std::chrono::high_resolution_clock::time_point m_frame_start;
    std::deque<float> m_frame_history;
    std::unordered_map<std::string, float> m_event_timings;
    std::unordered_map<std::string, float> m_event_avg_timings;
    
    std::atomic<size_t> m_draw_calls{0};
    std::atomic<size_t> m_triangles{0};
    
    mutable std::mutex m_mutex;
};

// Memory profiler for tracking memory usage patterns
class MemoryProfiler {
public:
    struct AllocationInfo {
        size_t size;
        std::string category;
        std::chrono::high_resolution_clock::time_point timestamp;
        void* address;
        size_t alignment;
        std::thread::id thread_id;
    };
    
    MemoryProfiler();
    
    void RecordAllocation(void* ptr, size_t size, const std::string& category = "General");
    void RecordDeallocation(void* ptr);
    
    size_t GetTotalAllocated() const { return m_total_allocated.load(); }
    size_t GetPeakAllocated() const { return m_peak_allocated.load(); }
    size_t GetAllocationCount() const { return m_allocation_count.load(); }
    size_t GetDeallocationCount() const { return m_deallocation_count.load(); }
    
    std::vector<AllocationInfo> GetActiveAllocations() const;
    std::unordered_map<std::string, size_t> GetMemoryByCategory() const;
    
    float CalculateFragmentation() const;
    void DetectLeaks();
    
private:
    std::atomic<size_t> m_total_allocated{0};
    std::atomic<size_t> m_peak_allocated{0};
    std::atomic<size_t> m_allocation_count{0};
    std::atomic<size_t> m_deallocation_count{0};
    
    std::unordered_map<void*, AllocationInfo> m_allocations;
    mutable std::mutex m_mutex;
};

// GPU profiler for tracking GPU performance
class GPUProfiler {
public:
    struct GPUQuery {
        uint32_t query_id;
        std::string name;
        bool is_timestamp;
    };
    
    GPUProfiler();
    ~GPUProfiler();
    
    void BeginGPUTimer(const std::string& name);
    void EndGPUTimer(const std::string& name);
    
    void RecordTextureMemory(size_t bytes);
    void RecordBufferMemory(size_t bytes);
    void RecordShaderSwitch();
    void RecordTextureBinding();
    void RecordRenderTargetSwitch();
    
    float GetGPUTime(const std::string& name) const;
    float GetTotalGPUTime() const;
    size_t GetGPUMemoryUsage() const;
    
private:
    std::vector<GPUQuery> m_queries;
    std::unordered_map<std::string, float> m_gpu_timings;
    
    std::atomic<size_t> m_texture_memory{0};
    std::atomic<size_t> m_buffer_memory{0};
    std::atomic<size_t> m_shader_switches{0};
    std::atomic<size_t> m_texture_binds{0};
    std::atomic<size_t> m_render_target_switches{0};
    
    mutable std::mutex m_mutex;
};

// Cache profiler for tracking cache performance
class CacheProfiler {
public:
    struct CacheStats {
        size_t hits = 0;
        size_t misses = 0;
        size_t evictions = 0;
        size_t memory_used = 0;
        float hit_rate = 0.0f;
        float avg_access_time_ns = 0.0f;
    };
    
    void RecordCacheAccess(const std::string& cache_name, bool hit, size_t item_size = 0);
    void RecordCacheEviction(const std::string& cache_name, size_t items_evicted);
    void UpdateCacheMemory(const std::string& cache_name, size_t memory_bytes);
    
    CacheStats GetCacheStats(const std::string& cache_name) const;
    std::unordered_map<std::string, CacheStats> GetAllCacheStats() const;
    
private:
    std::unordered_map<std::string, CacheStats> m_cache_stats;
    mutable std::mutex m_mutex;
};

// Platform-specific performance monitor
class PlatformMonitor {
public:
    PlatformMonitor();
    
    void Update();
    
    float GetCPUUsage() const;
    float GetMemoryUsage() const;
    float GetBatteryLevel() const;
    float GetTemperature() const;
    bool IsThermalThrottling() const;
    bool IsPowerSavingMode() const;
    
    // Platform-specific optimizations
    void EnableHighPerformanceMode();
    void EnablePowerSavingMode();
    void SetThreadAffinity(std::thread::id thread_id, const std::vector<int>& cores);
    
private:
    class PlatformImpl;
    std::unique_ptr<PlatformImpl> m_impl;
};

// Main profiler session manager
class ProfilerSession {
public:
    ProfilerSession(const std::string& name);
    ~ProfilerSession();
    
    // Profiler components
    FrameProfiler& GetFrameProfiler() { return *m_frame_profiler; }
    MemoryProfiler& GetMemoryProfiler() { return *m_memory_profiler; }
    GPUProfiler& GetGPUProfiler() { return *m_gpu_profiler; }
    CacheProfiler& GetCacheProfiler() { return *m_cache_profiler; }
    PlatformMonitor& GetPlatformMonitor() { return *m_platform_monitor; }
    
    // Session control
    void Start();
    void Stop();
    void Reset();
    bool IsActive() const { return m_active.load(); }
    
    // Metrics collection
    PerformanceMetrics CollectMetrics() const;
    void RecordMetrics(const PerformanceMetrics& metrics);
    
    // Performance analysis
    void AnalyzePerformance();
    std::vector<std::string> GetPerformanceWarnings() const;
    std::vector<std::string> GetOptimizationSuggestions() const;
    
    // Export functionality
    void ExportToJSON(const std::string& filepath) const;
    void ExportToCSV(const std::string& filepath) const;
    void ExportToChrome(const std::string& filepath) const; // Chrome tracing format
    
    // Static global session
    static ProfilerSession* GetGlobalSession();
    static void SetGlobalSession(ProfilerSession* session);
    
private:
    std::string m_name;
    std::atomic<bool> m_active{false};
    
    std::unique_ptr<FrameProfiler> m_frame_profiler;
    std::unique_ptr<MemoryProfiler> m_memory_profiler;
    std::unique_ptr<GPUProfiler> m_gpu_profiler;
    std::unique_ptr<CacheProfiler> m_cache_profiler;
    std::unique_ptr<PlatformMonitor> m_platform_monitor;
    
    std::deque<PerformanceMetrics> m_metrics_history;
    mutable std::mutex m_mutex;
    
    static ProfilerSession* s_global_session;
};

// Profiling macros for easy integration
#ifdef ECSCOPE_PROFILING_ENABLED
    #define PROFILE_SCOPE(name) ecscope::gui::performance::ProfileScope _profile_##__LINE__(name)
    #define PROFILE_SCOPE_SESSION(name, session) ecscope::gui::performance::ProfileScope _profile_##__LINE__(name, session)
    #define PROFILE_COUNTER(name, value) if(auto* s = ecscope::gui::performance::ProfilerSession::GetGlobalSession()) s->GetFrameProfiler().RecordEvent(name, value)
    #define PROFILE_GPU_SCOPE(name) ecscope::gui::performance::GPUProfileScope _gpu_profile_##__LINE__(name)
#else
    #define PROFILE_SCOPE(name)
    #define PROFILE_SCOPE_SESSION(name, session)
    #define PROFILE_COUNTER(name, value)
    #define PROFILE_GPU_SCOPE(name)
#endif

// GPU profiling scope helper
class GPUProfileScope {
public:
    GPUProfileScope(const std::string& name) : m_name(name) {
        if (auto* session = ProfilerSession::GetGlobalSession()) {
            session->GetGPUProfiler().BeginGPUTimer(m_name);
        }
    }
    
    ~GPUProfileScope() {
        if (auto* session = ProfilerSession::GetGlobalSession()) {
            session->GetGPUProfiler().EndGPUTimer(m_name);
        }
    }
    
private:
    std::string m_name;
};

// Performance budget system
class PerformanceBudget {
public:
    struct Budget {
        float frame_time_ms = 16.67f; // 60 FPS target
        size_t memory_mb = 512;
        float gpu_time_ms = 10.0f;
        size_t draw_calls = 1000;
        size_t triangles = 1000000;
        float cache_hit_rate = 0.9f;
    };
    
    void SetBudget(const Budget& budget) { m_budget = budget; }
    const Budget& GetBudget() const { return m_budget; }
    
    bool CheckBudget(const PerformanceMetrics& metrics) const;
    std::vector<std::string> GetBudgetViolations(const PerformanceMetrics& metrics) const;
    
private:
    Budget m_budget;
};

} // namespace ecscope::gui::performance