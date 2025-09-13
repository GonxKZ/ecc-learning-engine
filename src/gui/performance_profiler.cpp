#include "../../include/ecscope/gui/performance_profiler.hpp"
#include <algorithm>
#include <numeric>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#include <pdh.h>
#pragma comment(lib, "pdh.lib")
#elif __linux__
#include <sys/sysinfo.h>
#include <sys/resource.h>
#include <unistd.h>
#elif __APPLE__
#include <mach/mach.h>
#include <mach/processor_info.h>
#include <mach/mach_host.h>
#include <sys/sysctl.h>
#endif

namespace ecscope::gui::performance {

// Static member initialization
ProfilerSession* ProfilerSession::s_global_session = nullptr;

// ProfileScope implementation
ProfileScope::ProfileScope(const std::string& name, ProfilerSession* session)
    : m_name(name)
    , m_session(session ? session : ProfilerSession::GetGlobalSession())
    , m_start(std::chrono::high_resolution_clock::now()) {
}

ProfileScope::~ProfileScope() {
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - m_start);
    float duration_ms = duration.count() / 1000.0f;
    
    if (m_session) {
        m_session->GetFrameProfiler().RecordEvent(m_name, duration_ms);
        
        // Record counters and gauges
        for (const auto& [name, value] : m_counters) {
            m_session->GetFrameProfiler().RecordEvent(m_name + "." + name, static_cast<float>(value));
        }
        
        for (const auto& [name, value] : m_gauges) {
            m_session->GetFrameProfiler().RecordEvent(m_name + "." + name, value);
        }
    }
}

void ProfileScope::AddCounter(const std::string& name, int64_t value) {
    m_counters[name] = value;
}

void ProfileScope::AddGauge(const std::string& name, float value) {
    m_gauges[name] = value;
}

// FrameProfiler implementation
FrameProfiler::FrameProfiler() {
    m_frame_history.resize(HISTORY_SIZE, 0.0f);
}

void FrameProfiler::BeginFrame() {
    m_frame_start = std::chrono::high_resolution_clock::now();
    m_draw_calls = 0;
    m_triangles = 0;
    m_event_timings.clear();
}

void FrameProfiler::EndFrame() {
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - m_frame_start);
    float frame_time = duration.count() / 1000.0f;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Update history
    if (m_frame_history.size() >= HISTORY_SIZE) {
        m_frame_history.pop_front();
    }
    m_frame_history.push_back(frame_time);
    
    // Update event averages
    for (const auto& [name, timing] : m_event_timings) {
        auto& avg = m_event_avg_timings[name];
        avg = avg * 0.95f + timing * 0.05f; // Exponential moving average
    }
}

void FrameProfiler::RecordEvent(const std::string& name, float duration_ms) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_event_timings[name] = duration_ms;
}

void FrameProfiler::RecordDrawCall(size_t triangles) {
    m_draw_calls++;
    m_triangles += triangles;
}

float FrameProfiler::GetAverageFrameTime() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_frame_history.empty()) return 0.0f;
    
    float sum = std::accumulate(m_frame_history.begin(), m_frame_history.end(), 0.0f);
    return sum / m_frame_history.size();
}

float FrameProfiler::GetMinFrameTime() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_frame_history.empty()) return 0.0f;
    
    return *std::min_element(m_frame_history.begin(), m_frame_history.end());
}

float FrameProfiler::GetMaxFrameTime() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_frame_history.empty()) return 0.0f;
    
    return *std::max_element(m_frame_history.begin(), m_frame_history.end());
}

float FrameProfiler::GetPercentile(float percentile) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_frame_history.empty()) return 0.0f;
    
    std::vector<float> sorted(m_frame_history.begin(), m_frame_history.end());
    std::sort(sorted.begin(), sorted.end());
    
    size_t index = static_cast<size_t>(percentile * sorted.size() / 100.0f);
    index = std::min(index, sorted.size() - 1);
    
    return sorted[index];
}

float FrameProfiler::GetFrameTimeVariance() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_frame_history.size() < 2) return 0.0f;
    
    float mean = GetAverageFrameTime();
    float variance = 0.0f;
    
    for (float time : m_frame_history) {
        float diff = time - mean;
        variance += diff * diff;
    }
    
    return variance / (m_frame_history.size() - 1);
}

// MemoryProfiler implementation
MemoryProfiler::MemoryProfiler() {
}

void MemoryProfiler::RecordAllocation(void* ptr, size_t size, const std::string& category) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    AllocationInfo info;
    info.size = size;
    info.category = category;
    info.timestamp = std::chrono::high_resolution_clock::now();
    info.address = ptr;
    info.alignment = 0;
    info.thread_id = std::this_thread::get_id();
    
    m_allocations[ptr] = info;
    
    m_total_allocated += size;
    m_allocation_count++;
    
    if (m_total_allocated > m_peak_allocated) {
        m_peak_allocated = m_total_allocated.load();
    }
}

void MemoryProfiler::RecordDeallocation(void* ptr) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_allocations.find(ptr);
    if (it != m_allocations.end()) {
        m_total_allocated -= it->second.size;
        m_allocations.erase(it);
        m_deallocation_count++;
    }
}

std::vector<MemoryProfiler::AllocationInfo> MemoryProfiler::GetActiveAllocations() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<AllocationInfo> result;
    result.reserve(m_allocations.size());
    
    for (const auto& [ptr, info] : m_allocations) {
        result.push_back(info);
    }
    
    return result;
}

std::unordered_map<std::string, size_t> MemoryProfiler::GetMemoryByCategory() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::unordered_map<std::string, size_t> result;
    
    for (const auto& [ptr, info] : m_allocations) {
        result[info.category] += info.size;
    }
    
    return result;
}

float MemoryProfiler::CalculateFragmentation() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_allocations.empty()) return 0.0f;
    
    // Simple fragmentation metric based on allocation distribution
    std::vector<uintptr_t> addresses;
    addresses.reserve(m_allocations.size());
    
    for (const auto& [ptr, info] : m_allocations) {
        addresses.push_back(reinterpret_cast<uintptr_t>(ptr));
    }
    
    std::sort(addresses.begin(), addresses.end());
    
    // Calculate average gap between allocations
    size_t total_gaps = 0;
    size_t gap_count = 0;
    
    for (size_t i = 1; i < addresses.size(); ++i) {
        size_t gap = addresses[i] - addresses[i-1];
        if (gap > 0) {
            total_gaps += gap;
            gap_count++;
        }
    }
    
    if (gap_count == 0) return 0.0f;
    
    size_t avg_gap = total_gaps / gap_count;
    size_t avg_size = m_total_allocated / m_allocations.size();
    
    // Fragmentation ratio: gap size relative to allocation size
    return std::min(1.0f, static_cast<float>(avg_gap) / avg_size);
}

void MemoryProfiler::DetectLeaks() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // In a real implementation, this would compare against a baseline
    // and identify allocations that persist beyond expected lifetime
    
    auto now = std::chrono::high_resolution_clock::now();
    
    for (const auto& [ptr, info] : m_allocations) {
        auto age = std::chrono::duration_cast<std::chrono::seconds>(now - info.timestamp);
        if (age.count() > 60) { // Allocations older than 60 seconds
            // Log potential leak
        }
    }
}

// GPUProfiler implementation
GPUProfiler::GPUProfiler() {
    // Initialize GPU query objects
    // Platform-specific implementation would go here
}

GPUProfiler::~GPUProfiler() {
    // Cleanup GPU resources
}

void GPUProfiler::BeginGPUTimer(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // In a real implementation, this would create GPU timestamp queries
    // For now, we'll simulate with CPU timing
    auto now = std::chrono::high_resolution_clock::now();
    
    GPUQuery query;
    query.name = name;
    query.is_timestamp = true;
    query.query_id = static_cast<uint32_t>(m_queries.size());
    
    m_queries.push_back(query);
}

void GPUProfiler::EndGPUTimer(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // In a real implementation, this would retrieve GPU timestamps
    // For now, we'll simulate with a fixed value
    m_gpu_timings[name] = 1.0f + (std::hash<std::string>{}(name) % 10) * 0.5f;
}

void GPUProfiler::RecordTextureMemory(size_t bytes) {
    m_texture_memory = bytes;
}

void GPUProfiler::RecordBufferMemory(size_t bytes) {
    m_buffer_memory = bytes;
}

void GPUProfiler::RecordShaderSwitch() {
    m_shader_switches++;
}

void GPUProfiler::RecordTextureBinding() {
    m_texture_binds++;
}

void GPUProfiler::RecordRenderTargetSwitch() {
    m_render_target_switches++;
}

float GPUProfiler::GetGPUTime(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_gpu_timings.find(name);
    return it != m_gpu_timings.end() ? it->second : 0.0f;
}

float GPUProfiler::GetTotalGPUTime() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    float total = 0.0f;
    for (const auto& [name, time] : m_gpu_timings) {
        total += time;
    }
    
    return total;
}

size_t GPUProfiler::GetGPUMemoryUsage() const {
    return m_texture_memory + m_buffer_memory;
}

// CacheProfiler implementation
void CacheProfiler::RecordCacheAccess(const std::string& cache_name, bool hit, size_t item_size) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto& stats = m_cache_stats[cache_name];
    
    if (hit) {
        stats.hits++;
    } else {
        stats.misses++;
    }
    
    if (item_size > 0) {
        stats.memory_used += item_size;
    }
    
    // Update hit rate
    size_t total = stats.hits + stats.misses;
    if (total > 0) {
        stats.hit_rate = static_cast<float>(stats.hits) / total;
    }
}

void CacheProfiler::RecordCacheEviction(const std::string& cache_name, size_t items_evicted) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto& stats = m_cache_stats[cache_name];
    stats.evictions += items_evicted;
}

void CacheProfiler::UpdateCacheMemory(const std::string& cache_name, size_t memory_bytes) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto& stats = m_cache_stats[cache_name];
    stats.memory_used = memory_bytes;
}

CacheProfiler::CacheStats CacheProfiler::GetCacheStats(const std::string& cache_name) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_cache_stats.find(cache_name);
    return it != m_cache_stats.end() ? it->second : CacheStats{};
}

std::unordered_map<std::string, CacheProfiler::CacheStats> CacheProfiler::GetAllCacheStats() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cache_stats;
}

// PlatformMonitor implementation
class PlatformMonitor::PlatformImpl {
public:
    PlatformImpl() {
#ifdef _WIN32
        // Initialize Windows performance counters
        PdhOpenQuery(NULL, 0, &m_query);
        PdhAddCounter(m_query, TEXT("\\Processor(_Total)\\% Processor Time"), 0, &m_cpu_counter);
        PdhAddCounter(m_query, TEXT("\\Memory\\Available MBytes"), 0, &m_memory_counter);
#endif
    }
    
    ~PlatformImpl() {
#ifdef _WIN32
        if (m_query) {
            PdhCloseQuery(m_query);
        }
#endif
    }
    
    float GetCPUUsage() {
#ifdef _WIN32
        PDH_FMT_COUNTERVALUE value;
        PdhCollectQueryData(m_query);
        PdhGetFormattedCounterValue(m_cpu_counter, PDH_FMT_DOUBLE, NULL, &value);
        return static_cast<float>(value.doubleValue);
#elif __linux__
        static unsigned long long prev_total = 0;
        static unsigned long long prev_idle = 0;
        
        std::ifstream file("/proc/stat");
        std::string line;
        std::getline(file, line);
        
        std::istringstream iss(line);
        std::string cpu;
        unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
        iss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
        
        unsigned long long total = user + nice + system + idle + iowait + irq + softirq + steal;
        unsigned long long diff_total = total - prev_total;
        unsigned long long diff_idle = idle - prev_idle;
        
        prev_total = total;
        prev_idle = idle;
        
        if (diff_total == 0) return 0.0f;
        
        return 100.0f * (1.0f - static_cast<float>(diff_idle) / diff_total);
#elif __APPLE__
        host_cpu_load_info_data_t cpuinfo;
        mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
        
        if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, 
                          (host_info_t)&cpuinfo, &count) == KERN_SUCCESS) {
            unsigned int total = 0;
            for (int i = 0; i < CPU_STATE_MAX; i++) {
                total += cpuinfo.cpu_ticks[i];
            }
            
            if (total == 0) return 0.0f;
            
            return 100.0f * (1.0f - static_cast<float>(cpuinfo.cpu_ticks[CPU_STATE_IDLE]) / total);
        }
#endif
        return 0.0f;
    }
    
    float GetMemoryUsage() {
#ifdef _WIN32
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);
        DWORDLONG totalPhysMem = memInfo.ullTotalPhys;
        DWORDLONG physMemUsed = totalPhysMem - memInfo.ullAvailPhys;
        return static_cast<float>(physMemUsed) / totalPhysMem * 100.0f;
#elif __linux__
        struct sysinfo memInfo;
        sysinfo(&memInfo);
        
        long long totalPhysMem = memInfo.totalram;
        totalPhysMem *= memInfo.mem_unit;
        
        long long physMemUsed = totalPhysMem - (memInfo.freeram * memInfo.mem_unit);
        
        return static_cast<float>(physMemUsed) / totalPhysMem * 100.0f;
#elif __APPLE__
        vm_size_t page_size;
        mach_port_t mach_port;
        mach_msg_type_number_t count;
        vm_statistics64_data_t vm_stat;
        
        mach_port = mach_host_self();
        count = sizeof(vm_stat) / sizeof(natural_t);
        
        if (KERN_SUCCESS == host_page_size(mach_port, &page_size) &&
            KERN_SUCCESS == host_statistics64(mach_port, HOST_VM_INFO,
                                             (host_info64_t)&vm_stat, &count)) {
            uint64_t total = (vm_stat.free_count + vm_stat.active_count + 
                            vm_stat.inactive_count + vm_stat.wire_count) * page_size;
            uint64_t used = (vm_stat.active_count + vm_stat.wire_count) * page_size;
            
            return static_cast<float>(used) / total * 100.0f;
        }
#endif
        return 0.0f;
    }
    
    float GetBatteryLevel() {
#ifdef _WIN32
        SYSTEM_POWER_STATUS status;
        if (GetSystemPowerStatus(&status)) {
            if (status.BatteryLifePercent != 255) {
                return static_cast<float>(status.BatteryLifePercent);
            }
        }
#elif __linux__
        std::ifstream file("/sys/class/power_supply/BAT0/capacity");
        if (file.is_open()) {
            int level;
            file >> level;
            return static_cast<float>(level);
        }
#elif __APPLE__
        // macOS battery status would require IOKit framework
#endif
        return 100.0f; // Default to full if unknown
    }
    
    float GetTemperature() {
#ifdef _WIN32
        // Windows temperature monitoring requires WMI
#elif __linux__
        std::ifstream file("/sys/class/thermal/thermal_zone0/temp");
        if (file.is_open()) {
            int temp;
            file >> temp;
            return temp / 1000.0f; // Convert from millidegrees
        }
#elif __APPLE__
        // macOS temperature monitoring requires IOKit framework
#endif
        return 0.0f;
    }
    
    bool IsThermalThrottling() {
        float temp = GetTemperature();
        return temp > 80.0f; // Simple threshold
    }
    
    bool IsPowerSavingMode() {
#ifdef _WIN32
        SYSTEM_POWER_STATUS status;
        if (GetSystemPowerStatus(&status)) {
            return status.ACLineStatus == 0; // On battery
        }
#elif __linux__
        std::ifstream file("/sys/class/power_supply/AC/online");
        if (file.is_open()) {
            int online;
            file >> online;
            return online == 0; // On battery
        }
#endif
        return false;
    }
    
private:
#ifdef _WIN32
    PDH_HQUERY m_query = nullptr;
    PDH_HCOUNTER m_cpu_counter = nullptr;
    PDH_HCOUNTER m_memory_counter = nullptr;
#endif
};

PlatformMonitor::PlatformMonitor() 
    : m_impl(std::make_unique<PlatformImpl>()) {
}

void PlatformMonitor::Update() {
    // Update is handled on-demand in getter methods
}

float PlatformMonitor::GetCPUUsage() const {
    return m_impl->GetCPUUsage();
}

float PlatformMonitor::GetMemoryUsage() const {
    return m_impl->GetMemoryUsage();
}

float PlatformMonitor::GetBatteryLevel() const {
    return m_impl->GetBatteryLevel();
}

float PlatformMonitor::GetTemperature() const {
    return m_impl->GetTemperature();
}

bool PlatformMonitor::IsThermalThrottling() const {
    return m_impl->IsThermalThrottling();
}

bool PlatformMonitor::IsPowerSavingMode() const {
    return m_impl->IsPowerSavingMode();
}

void PlatformMonitor::EnableHighPerformanceMode() {
#ifdef _WIN32
    SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
#endif
}

void PlatformMonitor::EnablePowerSavingMode() {
#ifdef _WIN32
    SetThreadExecutionState(ES_CONTINUOUS);
#endif
}

void PlatformMonitor::SetThreadAffinity(std::thread::id thread_id, const std::vector<int>& cores) {
    // Platform-specific thread affinity setting
}

// ProfilerSession implementation
ProfilerSession::ProfilerSession(const std::string& name)
    : m_name(name)
    , m_frame_profiler(std::make_unique<FrameProfiler>())
    , m_memory_profiler(std::make_unique<MemoryProfiler>())
    , m_gpu_profiler(std::make_unique<GPUProfiler>())
    , m_cache_profiler(std::make_unique<CacheProfiler>())
    , m_platform_monitor(std::make_unique<PlatformMonitor>()) {
    
    m_metrics_history.resize(240); // 4 seconds at 60 FPS
}

ProfilerSession::~ProfilerSession() {
    Stop();
}

void ProfilerSession::Start() {
    m_active = true;
    m_frame_profiler->BeginFrame();
}

void ProfilerSession::Stop() {
    m_active = false;
    m_frame_profiler->EndFrame();
}

void ProfilerSession::Reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_metrics_history.clear();
    m_metrics_history.resize(240);
    
    // Reset individual profilers
    *m_frame_profiler = FrameProfiler();
    *m_memory_profiler = MemoryProfiler();
    *m_gpu_profiler = GPUProfiler();
    *m_cache_profiler = CacheProfiler();
}

PerformanceMetrics ProfilerSession::CollectMetrics() const {
    PerformanceMetrics metrics;
    
    // Frame metrics
    metrics.frame_time_ms = m_frame_profiler->GetAverageFrameTime();
    metrics.fps = metrics.frame_time_ms > 0 ? 1000.0f / metrics.frame_time_ms : 0.0f;
    metrics.fps_min = 1000.0f / m_frame_profiler->GetMaxFrameTime();
    metrics.fps_max = 1000.0f / m_frame_profiler->GetMinFrameTime();
    metrics.frame_time_variance = m_frame_profiler->GetFrameTimeVariance();
    
    // CPU metrics
    metrics.cpu_usage_percent = m_platform_monitor->GetCPUUsage();
    
    // Memory metrics
    metrics.memory_allocated_bytes = m_memory_profiler->GetTotalAllocated();
    metrics.memory_peak_bytes = m_memory_profiler->GetPeakAllocated();
    metrics.allocations_per_frame = m_memory_profiler->GetAllocationCount();
    metrics.deallocations_per_frame = m_memory_profiler->GetDeallocationCount();
    metrics.memory_fragmentation = m_memory_profiler->CalculateFragmentation();
    
    // GPU metrics
    metrics.gpu_time_ms = m_gpu_profiler->GetTotalGPUTime();
    metrics.gpu_memory_used_bytes = m_gpu_profiler->GetGPUMemoryUsage();
    
    // Cache metrics
    auto cache_stats = m_cache_profiler->GetAllCacheStats();
    for (const auto& [name, stats] : cache_stats) {
        metrics.cache_hits += stats.hits;
        metrics.cache_misses += stats.misses;
        metrics.cache_memory_bytes += stats.memory_used;
    }
    
    if (metrics.cache_hits + metrics.cache_misses > 0) {
        metrics.cache_hit_rate = static_cast<float>(metrics.cache_hits) / 
                                 (metrics.cache_hits + metrics.cache_misses);
    }
    
    // Platform metrics
    metrics.battery_level = m_platform_monitor->GetBatteryLevel();
    metrics.temperature_celsius = m_platform_monitor->GetTemperature();
    metrics.thermal_throttling = m_platform_monitor->IsThermalThrottling();
    metrics.power_saving_mode = m_platform_monitor->IsPowerSavingMode();
    
    return metrics;
}

void ProfilerSession::RecordMetrics(const PerformanceMetrics& metrics) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_metrics_history.size() >= 240) {
        m_metrics_history.pop_front();
    }
    
    m_metrics_history.push_back(metrics);
}

void ProfilerSession::AnalyzePerformance() {
    auto metrics = CollectMetrics();
    RecordMetrics(metrics);
}

std::vector<std::string> ProfilerSession::GetPerformanceWarnings() const {
    std::vector<std::string> warnings;
    
    auto metrics = CollectMetrics();
    
    // Frame time warnings
    if (metrics.fps < 30.0f) {
        warnings.push_back("Low FPS: " + std::to_string(static_cast<int>(metrics.fps)) + " FPS");
    }
    
    if (metrics.frame_time_variance > 10.0f) {
        warnings.push_back("High frame time variance: " + std::to_string(metrics.frame_time_variance) + " ms");
    }
    
    // Memory warnings
    if (metrics.memory_allocated_bytes > 512 * 1024 * 1024) {
        warnings.push_back("High memory usage: " + 
                          std::to_string(metrics.memory_allocated_bytes / (1024 * 1024)) + " MB");
    }
    
    if (metrics.memory_fragmentation > 0.5f) {
        warnings.push_back("High memory fragmentation: " + 
                          std::to_string(static_cast<int>(metrics.memory_fragmentation * 100)) + "%");
    }
    
    // Cache warnings
    if (metrics.cache_hit_rate < 0.8f) {
        warnings.push_back("Low cache hit rate: " + 
                          std::to_string(static_cast<int>(metrics.cache_hit_rate * 100)) + "%");
    }
    
    // Platform warnings
    if (metrics.thermal_throttling) {
        warnings.push_back("Thermal throttling detected");
    }
    
    if (metrics.battery_level < 20.0f) {
        warnings.push_back("Low battery: " + 
                          std::to_string(static_cast<int>(metrics.battery_level)) + "%");
    }
    
    return warnings;
}

std::vector<std::string> ProfilerSession::GetOptimizationSuggestions() const {
    std::vector<std::string> suggestions;
    
    auto metrics = CollectMetrics();
    
    // Frame time optimization
    if (metrics.gpu_time_ms > metrics.frame_time_ms * 0.8f) {
        suggestions.push_back("GPU-bound: Consider reducing shader complexity or resolution");
    } else if (metrics.ui_thread_time_ms > metrics.frame_time_ms * 0.5f) {
        suggestions.push_back("CPU-bound: Consider optimizing UI update logic");
    }
    
    // Memory optimization
    if (metrics.allocations_per_frame > 1000) {
        suggestions.push_back("High allocation rate: Consider using object pools");
    }
    
    if (metrics.memory_fragmentation > 0.3f) {
        suggestions.push_back("Memory fragmentation: Consider memory compaction");
    }
    
    // Cache optimization
    if (metrics.cache_hit_rate < 0.9f) {
        suggestions.push_back("Improve cache usage: Consider pre-loading frequently used assets");
    }
    
    // Draw call optimization
    if (metrics.draw_calls > 1000) {
        suggestions.push_back("High draw call count: Consider batching or instancing");
    }
    
    return suggestions;
}

void ProfilerSession::ExportToJSON(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) return;
    
    file << "{\n";
    file << "  \"session\": \"" << m_name << "\",\n";
    file << "  \"metrics\": [\n";
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    for (size_t i = 0; i < m_metrics_history.size(); ++i) {
        const auto& m = m_metrics_history[i];
        
        file << "    {\n";
        file << "      \"frame_time_ms\": " << m.frame_time_ms << ",\n";
        file << "      \"fps\": " << m.fps << ",\n";
        file << "      \"cpu_usage\": " << m.cpu_usage_percent << ",\n";
        file << "      \"memory_mb\": " << (m.memory_allocated_bytes / (1024.0f * 1024.0f)) << ",\n";
        file << "      \"gpu_time_ms\": " << m.gpu_time_ms << ",\n";
        file << "      \"cache_hit_rate\": " << m.cache_hit_rate << "\n";
        file << "    }";
        
        if (i < m_metrics_history.size() - 1) {
            file << ",";
        }
        file << "\n";
    }
    
    file << "  ]\n";
    file << "}\n";
}

void ProfilerSession::ExportToCSV(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) return;
    
    // Write header
    file << "frame_time_ms,fps,cpu_usage,memory_mb,gpu_time_ms,cache_hit_rate\n";
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    for (const auto& m : m_metrics_history) {
        file << m.frame_time_ms << ","
             << m.fps << ","
             << m.cpu_usage_percent << ","
             << (m.memory_allocated_bytes / (1024.0f * 1024.0f)) << ","
             << m.gpu_time_ms << ","
             << m.cache_hit_rate << "\n";
    }
}

void ProfilerSession::ExportToChrome(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) return;
    
    file << "{\n";
    file << "  \"traceEvents\": [\n";
    
    // Export frame events in Chrome Tracing format
    const auto& events = m_frame_profiler->GetEventTimings();
    bool first = true;
    
    for (const auto& [name, duration] : events) {
        if (!first) file << ",\n";
        first = false;
        
        file << "    {\n";
        file << "      \"name\": \"" << name << "\",\n";
        file << "      \"cat\": \"ui\",\n";
        file << "      \"ph\": \"X\",\n";
        file << "      \"ts\": 0,\n";
        file << "      \"dur\": " << static_cast<int64_t>(duration * 1000) << ",\n";
        file << "      \"pid\": 1,\n";
        file << "      \"tid\": 1\n";
        file << "    }";
    }
    
    file << "\n  ],\n";
    file << "  \"displayTimeUnit\": \"ms\"\n";
    file << "}\n";
}

ProfilerSession* ProfilerSession::GetGlobalSession() {
    return s_global_session;
}

void ProfilerSession::SetGlobalSession(ProfilerSession* session) {
    s_global_session = session;
}

// PerformanceBudget implementation
bool PerformanceBudget::CheckBudget(const PerformanceMetrics& metrics) const {
    if (metrics.frame_time_ms > m_budget.frame_time_ms) return false;
    if (metrics.memory_allocated_bytes > m_budget.memory_mb * 1024 * 1024) return false;
    if (metrics.gpu_time_ms > m_budget.gpu_time_ms) return false;
    if (metrics.draw_calls > m_budget.draw_calls) return false;
    if (metrics.triangles_rendered > m_budget.triangles) return false;
    if (metrics.cache_hit_rate < m_budget.cache_hit_rate) return false;
    
    return true;
}

std::vector<std::string> PerformanceBudget::GetBudgetViolations(const PerformanceMetrics& metrics) const {
    std::vector<std::string> violations;
    
    if (metrics.frame_time_ms > m_budget.frame_time_ms) {
        violations.push_back("Frame time: " + std::to_string(metrics.frame_time_ms) + 
                            " ms (budget: " + std::to_string(m_budget.frame_time_ms) + " ms)");
    }
    
    size_t memory_mb = metrics.memory_allocated_bytes / (1024 * 1024);
    if (memory_mb > m_budget.memory_mb) {
        violations.push_back("Memory: " + std::to_string(memory_mb) + 
                            " MB (budget: " + std::to_string(m_budget.memory_mb) + " MB)");
    }
    
    if (metrics.gpu_time_ms > m_budget.gpu_time_ms) {
        violations.push_back("GPU time: " + std::to_string(metrics.gpu_time_ms) + 
                            " ms (budget: " + std::to_string(m_budget.gpu_time_ms) + " ms)");
    }
    
    if (metrics.draw_calls > m_budget.draw_calls) {
        violations.push_back("Draw calls: " + std::to_string(metrics.draw_calls) + 
                            " (budget: " + std::to_string(m_budget.draw_calls) + ")");
    }
    
    if (metrics.triangles_rendered > m_budget.triangles) {
        violations.push_back("Triangles: " + std::to_string(metrics.triangles_rendered) + 
                            " (budget: " + std::to_string(m_budget.triangles) + ")");
    }
    
    if (metrics.cache_hit_rate < m_budget.cache_hit_rate) {
        violations.push_back("Cache hit rate: " + std::to_string(metrics.cache_hit_rate * 100) + 
                            "% (budget: " + std::to_string(m_budget.cache_hit_rate * 100) + "%)");
    }
    
    return violations;
}

} // namespace ecscope::gui::performance