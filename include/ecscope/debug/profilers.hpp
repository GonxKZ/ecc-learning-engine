#pragma once

#include "debug_system.hpp"
#include <array>
#include <deque>
#include <vector>
#include <chrono>
#include <unordered_map>
#include <string>
#include <memory>
#include <thread>
#include <atomic>

namespace ECScope::Debug {

// Timing utilities
using TimePoint = std::chrono::high_resolution_clock::time_point;
using Duration = std::chrono::nanoseconds;

inline TimePoint GetCurrentTime() {
    return std::chrono::high_resolution_clock::now();
}

inline double ToMilliseconds(Duration duration) {
    return std::chrono::duration<double, std::milli>(duration).count();
}

/**
 * @brief High-precision CPU profiler with hierarchical sampling
 */
class CPUProfiler : public Profiler {
public:
    struct Sample {
        std::string name;
        TimePoint start_time;
        Duration duration;
        uint32_t thread_id;
        uint32_t depth;
        size_t call_count = 1;
        
        Sample() = default;
        Sample(std::string n, TimePoint start, uint32_t tid, uint32_t d)
            : name(std::move(n)), start_time(start), thread_id(tid), depth(d) {}
    };
    
    struct ThreadData {
        std::vector<Sample> samples;
        std::vector<size_t> call_stack;
        uint32_t current_depth = 0;
        
        ThreadData() {
            samples.reserve(10000);
            call_stack.reserve(256);
        }
    };
    
    struct ProfileData {
        double total_time_ms = 0.0;
        double average_time_ms = 0.0;
        double min_time_ms = std::numeric_limits<double>::max();
        double max_time_ms = 0.0;
        size_t call_count = 0;
        double percentage = 0.0;
    };

    explicit CPUProfiler(const std::string& name);
    ~CPUProfiler() override;

    void Update(float deltaTime) override;
    void Reset() override;

    // Profiling interface
    void BeginSample(const std::string& name);
    void EndSample();
    
    // Scoped profiling helper
    class ScopedSample {
    public:
        ScopedSample(CPUProfiler& profiler, const std::string& name)
            : m_profiler(profiler) {
            m_profiler.BeginSample(name);
        }
        
        ~ScopedSample() {
            m_profiler.EndSample();
        }
        
    private:
        CPUProfiler& m_profiler;
    };

    // Data access
    const std::unordered_map<std::string, ProfileData>& GetProfileData() const { return m_profile_data; }
    const std::vector<Sample>& GetRecentSamples() const { return m_recent_samples; }
    
    // Configuration
    void SetMaxSamples(size_t max_samples) { m_max_samples = max_samples; }
    void SetUpdateFrequency(float frequency) { m_update_frequency = frequency; }

private:
    thread_local static ThreadData* s_thread_data;
    
    std::unordered_map<std::thread::id, std::unique_ptr<ThreadData>> m_thread_data;
    std::unordered_map<std::string, ProfileData> m_profile_data;
    std::vector<Sample> m_recent_samples;
    
    size_t m_max_samples = 10000;
    float m_update_frequency = 60.0f;
    float m_update_timer = 0.0f;
    
    std::mutex m_mutex;
    
    void ProcessSamples();
    ThreadData& GetThreadData();
    uint32_t GetThreadId() const;
};

/**
 * @brief Advanced memory profiler with allocation tracking and leak detection
 */
class MemoryProfiler : public Profiler {
public:
    struct AllocationInfo {
        void* address;
        size_t size;
        std::string tag;
        TimePoint timestamp;
        std::array<void*, 16> callstack;
        uint32_t callstack_depth;
        uint32_t thread_id;
        
        AllocationInfo() = default;
        AllocationInfo(void* addr, size_t sz, std::string tg)
            : address(addr), size(sz), tag(std::move(tg)), timestamp(GetCurrentTime()) {}
    };
    
    struct MemoryStats {
        size_t total_allocated = 0;
        size_t total_freed = 0;
        size_t current_allocated = 0;
        size_t peak_allocated = 0;
        size_t allocation_count = 0;
        size_t deallocation_count = 0;
        size_t leak_count = 0;
        
        double fragmentation_ratio = 0.0;
        double allocation_rate_per_second = 0.0;
        double deallocation_rate_per_second = 0.0;
    };
    
    struct TagStats {
        size_t total_allocated = 0;
        size_t current_allocated = 0;
        size_t allocation_count = 0;
        double average_size = 0.0;
    };

    explicit MemoryProfiler(const std::string& name);
    ~MemoryProfiler() override;

    void Update(float deltaTime) override;
    void Reset() override;

    // Memory tracking interface
    void TrackAllocation(void* address, size_t size, const std::string& tag = "");
    void TrackDeallocation(void* address);
    
    // Heap analysis
    void AnalyzeHeap();
    void DetectLeaks();
    
    // Data access
    const MemoryStats& GetStats() const { return m_stats; }
    const std::unordered_map<std::string, TagStats>& GetTagStats() const { return m_tag_stats; }
    const std::vector<AllocationInfo>& GetActiveAllocations() const { return m_active_allocations; }
    const std::vector<AllocationInfo>& GetLeaks() const { return m_leaks; }
    
    // Memory visualization data
    struct MemoryBlock {
        void* start;
        void* end;
        size_t size;
        std::string tag;
        bool is_free;
        TimePoint timestamp;
    };
    
    const std::vector<MemoryBlock>& GetMemoryMap() const { return m_memory_map; }

private:
    std::unordered_map<void*, AllocationInfo> m_allocations;
    std::vector<AllocationInfo> m_active_allocations;
    std::vector<AllocationInfo> m_leaks;
    std::vector<MemoryBlock> m_memory_map;
    
    MemoryStats m_stats;
    std::unordered_map<std::string, TagStats> m_tag_stats;
    
    std::deque<std::pair<TimePoint, size_t>> m_allocation_history;
    std::deque<std::pair<TimePoint, size_t>> m_deallocation_history;
    
    std::mutex m_mutex;
    
    void UpdateStats();
    void UpdateMemoryMap();
    void CaptureCallstack(AllocationInfo& info);
};

/**
 * @brief GPU profiler for render timing and resource usage
 */
class GPUProfiler : public Profiler {
public:
    struct GPUQuery {
        uint32_t query_id;
        std::string name;
        TimePoint cpu_start;
        uint64_t gpu_start_time;
        uint64_t gpu_end_time;
        bool completed = false;
    };
    
    struct GPUStats {
        double total_frame_time_ms = 0.0;
        double vertex_time_ms = 0.0;
        double fragment_time_ms = 0.0;
        double compute_time_ms = 0.0;
        
        size_t vertices_rendered = 0;
        size_t triangles_rendered = 0;
        size_t draw_calls = 0;
        size_t texture_switches = 0;
        size_t shader_switches = 0;
        
        size_t vram_usage_bytes = 0;
        size_t vram_available_bytes = 0;
        
        double gpu_utilization = 0.0;
        double memory_bandwidth_usage = 0.0;
    };

    explicit GPUProfiler(const std::string& name);
    ~GPUProfiler() override;

    void Update(float deltaTime) override;
    void Reset() override;

    // GPU profiling interface
    void BeginGPUEvent(const std::string& name);
    void EndGPUEvent();
    
    // Resource tracking
    void TrackDrawCall(size_t vertices, size_t triangles);
    void TrackTextureSwitch();
    void TrackShaderSwitch();
    void TrackVRAMUsage(size_t used, size_t available);

    // Data access
    const GPUStats& GetStats() const { return m_stats; }
    const std::vector<GPUQuery>& GetCompletedQueries() const { return m_completed_queries; }

private:
    std::vector<GPUQuery> m_active_queries;
    std::vector<GPUQuery> m_completed_queries;
    
    GPUStats m_stats;
    uint32_t m_next_query_id = 1;
    
    std::mutex m_mutex;
    
    void ProcessQueries();
    uint32_t CreateGPUQuery();
    void DeleteGPUQuery(uint32_t query_id);
    bool IsQueryComplete(uint32_t query_id);
    uint64_t GetQueryResult(uint32_t query_id);
};

/**
 * @brief Network profiler for bandwidth and latency analysis
 */
class NetworkProfiler : public Profiler {
public:
    struct NetworkEvent {
        enum Type { Send, Receive, Connect, Disconnect, Error };
        
        Type type;
        TimePoint timestamp;
        size_t bytes;
        std::string endpoint;
        std::string protocol;
        double latency_ms = 0.0;
        uint32_t packet_id = 0;
    };
    
    struct NetworkStats {
        size_t total_bytes_sent = 0;
        size_t total_bytes_received = 0;
        size_t packets_sent = 0;
        size_t packets_received = 0;
        size_t packets_lost = 0;
        
        double average_latency_ms = 0.0;
        double min_latency_ms = std::numeric_limits<double>::max();
        double max_latency_ms = 0.0;
        
        double current_upload_bps = 0.0;
        double current_download_bps = 0.0;
        double peak_upload_bps = 0.0;
        double peak_download_bps = 0.0;
        
        size_t active_connections = 0;
        size_t total_connections = 0;
        size_t failed_connections = 0;
    };

    explicit NetworkProfiler(const std::string& name);
    ~NetworkProfiler() override;

    void Update(float deltaTime) override;
    void Reset() override;

    // Network event tracking
    void TrackSend(const std::string& endpoint, size_t bytes, const std::string& protocol = "TCP");
    void TrackReceive(const std::string& endpoint, size_t bytes, const std::string& protocol = "TCP");
    void TrackConnect(const std::string& endpoint);
    void TrackDisconnect(const std::string& endpoint);
    void TrackLatency(const std::string& endpoint, double latency_ms);

    // Data access
    const NetworkStats& GetStats() const { return m_stats; }
    const std::vector<NetworkEvent>& GetRecentEvents() const { return m_recent_events; }

private:
    std::vector<NetworkEvent> m_events;
    std::vector<NetworkEvent> m_recent_events;
    NetworkStats m_stats;
    
    std::deque<std::pair<TimePoint, size_t>> m_upload_history;
    std::deque<std::pair<TimePoint, size_t>> m_download_history;
    
    std::unordered_map<std::string, TimePoint> m_connection_times;
    std::unordered_map<std::string, std::vector<double>> m_latency_samples;
    
    std::mutex m_mutex;
    
    void UpdateStats();
    void CalculateBandwidth();
};

/**
 * @brief Asset loading profiler with bottleneck identification
 */
class AssetProfiler : public Profiler {
public:
    struct AssetEvent {
        enum Type { LoadStart, LoadComplete, LoadFailed, Cached, Evicted };
        enum Stage { FileIO, Parsing, Processing, Upload, Complete };
        
        Type type;
        Stage stage;
        std::string asset_path;
        std::string asset_type;
        TimePoint timestamp;
        Duration duration;
        size_t file_size = 0;
        size_t memory_size = 0;
        std::string error_message;
    };
    
    struct AssetStats {
        size_t total_assets_loaded = 0;
        size_t assets_in_cache = 0;
        size_t cache_hits = 0;
        size_t cache_misses = 0;
        
        double total_load_time_ms = 0.0;
        double average_load_time_ms = 0.0;
        double file_io_time_ms = 0.0;
        double parsing_time_ms = 0.0;
        double processing_time_ms = 0.0;
        double upload_time_ms = 0.0;
        
        size_t total_file_bytes = 0;
        size_t total_memory_bytes = 0;
        double compression_ratio = 0.0;
        
        size_t failed_loads = 0;
        double cache_hit_ratio = 0.0;
    };

    explicit AssetProfiler(const std::string& name);
    ~AssetProfiler() override;

    void Update(float deltaTime) override;
    void Reset() override;

    // Asset tracking interface
    void TrackLoadStart(const std::string& path, const std::string& type);
    void TrackLoadStage(const std::string& path, AssetEvent::Stage stage, Duration duration);
    void TrackLoadComplete(const std::string& path, size_t file_size, size_t memory_size);
    void TrackLoadFailed(const std::string& path, const std::string& error);
    void TrackCacheHit(const std::string& path);
    void TrackCacheMiss(const std::string& path);

    // Data access
    const AssetStats& GetStats() const { return m_stats; }
    const std::vector<AssetEvent>& GetRecentEvents() const { return m_recent_events; }
    
    // Bottleneck analysis
    struct BottleneckInfo {
        AssetEvent::Stage stage;
        double percentage;
        std::string description;
    };
    
    std::vector<BottleneckInfo> AnalyzeBottlenecks() const;

private:
    std::vector<AssetEvent> m_events;
    std::vector<AssetEvent> m_recent_events;
    AssetStats m_stats;
    
    std::unordered_map<std::string, AssetEvent> m_active_loads;
    std::unordered_map<std::string, std::vector<AssetEvent::Stage>> m_load_stages;
    
    std::mutex m_mutex;
    
    void UpdateStats();
};

/**
 * @brief Custom event profiler with user-defined markers
 */
class CustomEventProfiler : public Profiler {
public:
    struct CustomEvent {
        std::string name;
        std::string category;
        TimePoint timestamp;
        Duration duration;
        std::unordered_map<std::string, std::string> metadata;
        uint32_t thread_id;
        
        CustomEvent() = default;
        CustomEvent(std::string n, std::string cat, TimePoint ts, uint32_t tid)
            : name(std::move(n)), category(std::move(cat)), timestamp(ts), thread_id(tid) {}
    };
    
    struct EventStats {
        size_t total_events = 0;
        double total_time_ms = 0.0;
        double average_time_ms = 0.0;
        double min_time_ms = std::numeric_limits<double>::max();
        double max_time_ms = 0.0;
    };

    explicit CustomEventProfiler(const std::string& name);
    ~CustomEventProfiler() override;

    void Update(float deltaTime) override;
    void Reset() override;

    // Custom event interface
    void BeginEvent(const std::string& name, const std::string& category = "");
    void EndEvent();
    void AddEventMetadata(const std::string& key, const std::string& value);
    
    // Instant events
    void RecordInstantEvent(const std::string& name, const std::string& category = "");
    
    // Scoped event helper
    class ScopedEvent {
    public:
        ScopedEvent(CustomEventProfiler& profiler, const std::string& name, const std::string& category = "")
            : m_profiler(profiler) {
            m_profiler.BeginEvent(name, category);
        }
        
        ~ScopedEvent() {
            m_profiler.EndEvent();
        }
        
        void AddMetadata(const std::string& key, const std::string& value) {
            m_profiler.AddEventMetadata(key, value);
        }
        
    private:
        CustomEventProfiler& m_profiler;
    };

    // Data access
    const std::vector<CustomEvent>& GetEvents() const { return m_events; }
    const std::unordered_map<std::string, EventStats>& GetEventStats() const { return m_event_stats; }
    const std::unordered_map<std::string, EventStats>& GetCategoryStats() const { return m_category_stats; }

private:
    thread_local static CustomEvent* s_current_event;
    
    std::vector<CustomEvent> m_events;
    std::unordered_map<std::string, EventStats> m_event_stats;
    std::unordered_map<std::string, EventStats> m_category_stats;
    
    std::mutex m_mutex;
    
    void UpdateStats();
    uint32_t GetThreadId() const;
};

// Profiler macros for easy usage
#define ECSCOPE_PROFILE_CPU(profiler, name) \
    ECScope::Debug::CPUProfiler::ScopedSample _profile_sample(profiler, name)

#define ECSCOPE_PROFILE_EVENT(profiler, name, category) \
    ECScope::Debug::CustomEventProfiler::ScopedEvent _custom_event(profiler, name, category)

#define ECSCOPE_PROFILE_FUNCTION(profiler) \
    ECSCOPE_PROFILE_CPU(profiler, __FUNCTION__)

} // namespace ECScope::Debug