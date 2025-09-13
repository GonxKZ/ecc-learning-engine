#include "../../include/ecscope/gui/platform_optimization.hpp"
#include <algorithm>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#include <intrin.h>
#include <powrprof.h>
#include <d3d11.h>
#pragma comment(lib, "powrprof.lib")
#elif __linux__
#include <cpuid.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/mman.h>
#include <pthread.h>
#include <sched.h>
#include <fstream>
#elif __APPLE__
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <sys/sysctl.h>
#include <IOKit/pwr_mgt/IOPMLib.h>
#endif

namespace ecscope::gui::platform {

// Platform detection
Platform PlatformOptimizer::GetCurrentPlatform() {
#ifdef _WIN32
    return Platform::WINDOWS;
#elif __linux__
    return Platform::LINUX;
#elif __APPLE__
    return Platform::MACOS;
#else
    return Platform::UNKNOWN;
#endif
}

// Factory method
std::unique_ptr<PlatformOptimizer> PlatformOptimizer::Create() {
#ifdef _WIN32
    return std::make_unique<WindowsOptimizer>();
#elif __linux__
    return std::make_unique<LinuxOptimizer>();
#elif __APPLE__
    return std::make_unique<MacOSOptimizer>();
#else
    return nullptr;
#endif
}

#ifdef _WIN32
// Windows implementation
class WindowsOptimizer::Impl {
public:
    Impl() {
        // Initialize Windows-specific resources
        InitializeCPUInfo();
        InitializeGPUInfo();
    }
    
    ~Impl() {
        // Cleanup
    }
    
    void InitializeCPUInfo() {
        // Get CPU information using CPUID
        int cpuInfo[4] = {0};
        char vendor[13] = {0};
        
        __cpuid(cpuInfo, 0);
        memcpy(vendor, &cpuInfo[1], 4);
        memcpy(vendor + 4, &cpuInfo[3], 4);
        memcpy(vendor + 8, &cpuInfo[2], 4);
        m_cpu_vendor = vendor;
        
        // Get CPU brand string
        char brand[49] = {0};
        __cpuid(cpuInfo, 0x80000000);
        int extIds = cpuInfo[0];
        
        for (int i = 0x80000002; i <= 0x80000004 && i <= extIds; ++i) {
            __cpuid(cpuInfo, i);
            memcpy(brand + (i - 0x80000002) * 16, cpuInfo, 16);
        }
        m_cpu_brand = brand;
        
        // Get CPU features
        __cpuid(cpuInfo, 1);
        m_has_sse = (cpuInfo[3] & (1 << 25)) != 0;
        m_has_sse2 = (cpuInfo[3] & (1 << 26)) != 0;
        m_has_sse3 = (cpuInfo[2] & (1 << 0)) != 0;
        m_has_ssse3 = (cpuInfo[2] & (1 << 9)) != 0;
        m_has_sse41 = (cpuInfo[2] & (1 << 19)) != 0;
        m_has_sse42 = (cpuInfo[2] & (1 << 20)) != 0;
        m_has_avx = (cpuInfo[2] & (1 << 28)) != 0;
        
        __cpuidex(cpuInfo, 7, 0);
        m_has_avx2 = (cpuInfo[1] & (1 << 5)) != 0;
        
        // Get core count
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        m_cpu_cores = sysInfo.dwNumberOfProcessors;
        
        // Get memory info
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);
        m_total_memory = memInfo.ullTotalPhys;
    }
    
    void InitializeGPUInfo() {
        // Use DXGI to get GPU information
        IDXGIFactory* factory = nullptr;
        if (SUCCEEDED(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory))) {
            IDXGIAdapter* adapter = nullptr;
            if (SUCCEEDED(factory->EnumAdapters(0, &adapter))) {
                DXGI_ADAPTER_DESC desc;
                adapter->GetDesc(&desc);
                
                // Convert wide string to string
                char buffer[128];
                wcstombs(buffer, desc.Description, 128);
                m_gpu_renderer = buffer;
                
                m_gpu_memory = desc.DedicatedVideoMemory / (1024 * 1024);
                
                adapter->Release();
            }
            factory->Release();
        }
    }
    
    // Member variables
    std::string m_cpu_vendor;
    std::string m_cpu_brand;
    std::string m_gpu_renderer;
    uint32_t m_cpu_cores = 0;
    size_t m_total_memory = 0;
    size_t m_gpu_memory = 0;
    
    bool m_has_sse = false;
    bool m_has_sse2 = false;
    bool m_has_sse3 = false;
    bool m_has_ssse3 = false;
    bool m_has_sse41 = false;
    bool m_has_sse42 = false;
    bool m_has_avx = false;
    bool m_has_avx2 = false;
};

WindowsOptimizer::WindowsOptimizer() : m_impl(std::make_unique<Impl>()) {
}

WindowsOptimizer::~WindowsOptimizer() = default;

HardwareCapabilities WindowsOptimizer::DetectHardware() {
    HardwareCapabilities caps;
    
    caps.cpu_vendor = m_impl->m_cpu_vendor;
    caps.cpu_brand = m_impl->m_cpu_brand;
    caps.cpu_cores = m_impl->m_cpu_cores;
    caps.cpu_threads = m_impl->m_cpu_cores; // Simplified
    
    caps.has_sse = m_impl->m_has_sse;
    caps.has_sse2 = m_impl->m_has_sse2;
    caps.has_sse3 = m_impl->m_has_sse3;
    caps.has_ssse3 = m_impl->m_has_ssse3;
    caps.has_sse41 = m_impl->m_has_sse41;
    caps.has_sse42 = m_impl->m_has_sse42;
    caps.has_avx = m_impl->m_has_avx;
    caps.has_avx2 = m_impl->m_has_avx2;
    
    caps.gpu_renderer = m_impl->m_gpu_renderer;
    caps.gpu_memory_mb = m_impl->m_gpu_memory;
    
    caps.total_memory_mb = m_impl->m_total_memory / (1024 * 1024);
    
    // Get display info
    caps.primary_display_width = GetSystemMetrics(SM_CXSCREEN);
    caps.primary_display_height = GetSystemMetrics(SM_CYSCREEN);
    
    // Get DPI
    HDC screen = GetDC(nullptr);
    caps.primary_display_dpi = static_cast<float>(GetDeviceCaps(screen, LOGPIXELSX));
    ReleaseDC(nullptr, screen);
    
    return caps;
}

RenderingHints WindowsOptimizer::GetRenderingHints() {
    RenderingHints hints;
    
    hints.use_persistent_mapping = true;
    hints.use_buffer_orphaning = true;
    hints.use_unsynchronized_mapping = false;
    hints.optimal_buffer_size = 64 * 1024; // 64KB
    
    hints.use_texture_arrays = true;
    hints.use_bindless_textures = false; // Requires extensions
    hints.max_texture_size = 16384;
    
    hints.use_indirect_drawing = true;
    hints.use_multi_draw_indirect = true;
    hints.use_instancing = true;
    hints.max_draw_calls_per_frame = 10000;
    
    hints.use_shader_cache = true;
    hints.use_pipeline_cache = true;
    hints.compile_shaders_async = true;
    
    hints.use_fence_sync = true;
    hints.frame_lag = 2;
    
    return hints;
}

MemoryHints WindowsOptimizer::GetMemoryHints() {
    MemoryHints hints;
    
    hints.use_large_pages = false; // Requires privileges
    hints.use_numa_aware_allocation = false;
    hints.use_memory_pools = true;
    hints.pool_chunk_size = 256 * 1024; // 256KB
    
    hints.cache_line_size = 64;
    hints.align_to_cache_line = true;
    hints.use_prefetching = true;
    
    hints.reserve_address_space = true;
    hints.reserve_size_mb = 1024; // 1GB
    hints.commit_on_demand = true;
    
    return hints;
}

ThreadingHints WindowsOptimizer::GetThreadingHints() {
    ThreadingHints hints;
    
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    
    hints.worker_thread_count = sysInfo.dwNumberOfProcessors - 2; // Leave 2 for UI/render
    hints.io_thread_count = 2;
    hints.use_thread_affinity = true;
    
    // Distribute cores
    for (uint32_t i = 0; i < sysInfo.dwNumberOfProcessors; ++i) {
        if (i == 0) {
            hints.ui_thread_cores.push_back(i);
        } else if (i == 1) {
            hints.render_thread_cores.push_back(i);
        } else {
            hints.worker_thread_cores.push_back(i);
        }
    }
    
    hints.use_critical_sections = true;
    hints.spin_count = 4000;
    
    hints.ui_thread_priority = THREAD_PRIORITY_ABOVE_NORMAL;
    hints.render_thread_priority = THREAD_PRIORITY_HIGHEST;
    hints.worker_thread_priority = THREAD_PRIORITY_NORMAL;
    
    return hints;
}

void* WindowsOptimizer::AllocateAligned(size_t size, size_t alignment) {
    return _aligned_malloc(size, alignment);
}

void WindowsOptimizer::FreeAligned(void* ptr) {
    _aligned_free(ptr);
}

bool WindowsOptimizer::LockMemory(void* ptr, size_t size) {
    return VirtualLock(ptr, size) != 0;
}

bool WindowsOptimizer::UnlockMemory(void* ptr, size_t size) {
    return VirtualUnlock(ptr, size) != 0;
}

void WindowsOptimizer::SetThreadAffinity(std::thread::id thread_id, const std::vector<uint32_t>& cores) {
    DWORD_PTR mask = 0;
    for (uint32_t core : cores) {
        mask |= (1ULL << core);
    }
    
    // Would need to get thread handle from thread_id
    SetThreadAffinityMask(GetCurrentThread(), mask);
}

void WindowsOptimizer::SetThreadPriority(std::thread::id thread_id, int priority) {
    SetThreadPriority(GetCurrentThread(), priority);
}

void WindowsOptimizer::SetThreadName(std::thread::id thread_id, const std::string& name) {
    // Windows 10 version 1607+
    std::wstring wideName(name.begin(), name.end());
    SetThreadDescription(GetCurrentThread(), wideName.c_str());
}

void WindowsOptimizer::RequestHighPerformance() {
    // Request high performance power plan
    GUID* activeScheme;
    PowerGetActiveScheme(nullptr, &activeScheme);
    // Would enumerate and set high performance GUID
    LocalFree(activeScheme);
}

void WindowsOptimizer::RequestPowerSaving() {
    // Request power saving plan
}

float WindowsOptimizer::GetBatteryLevel() {
    SYSTEM_POWER_STATUS status;
    if (GetSystemPowerStatus(&status)) {
        if (status.BatteryLifePercent != 255) {
            return static_cast<float>(status.BatteryLifePercent);
        }
    }
    return 100.0f;
}

bool WindowsOptimizer::IsOnBattery() {
    SYSTEM_POWER_STATUS status;
    if (GetSystemPowerStatus(&status)) {
        return status.ACLineStatus == 0;
    }
    return false;
}

void WindowsOptimizer::EnableVSync(bool enable) {
    // Platform-specific VSync control
}

void WindowsOptimizer::SetSwapInterval(int interval) {
    // wglSwapIntervalEXT(interval);
}

bool WindowsOptimizer::SupportsAdaptiveSync() {
    // Check for G-Sync/FreeSync support
    return false;
}

void WindowsOptimizer::EnableDirectStorage(bool enable) {
    // DirectStorage API initialization
}

void WindowsOptimizer::EnableGPUScheduling(bool enable) {
    // Hardware-accelerated GPU scheduling
}

void WindowsOptimizer::SetProcessPriority(uint32_t priority) {
    SetPriorityClass(GetCurrentProcess(), priority);
}

bool WindowsOptimizer::RegisterForMemoryNotifications() {
    // Register for low memory notifications
    return true;
}

#endif // _WIN32

#ifdef __linux__
// Linux implementation
class LinuxOptimizer::Impl {
public:
    Impl() {
        InitializeCPUInfo();
        InitializeMemoryInfo();
    }
    
    void InitializeCPUInfo() {
        // Parse /proc/cpuinfo
        std::ifstream cpuinfo("/proc/cpuinfo");
        std::string line;
        
        while (std::getline(cpuinfo, line)) {
            if (line.find("vendor_id") != std::string::npos) {
                size_t pos = line.find(':');
                if (pos != std::string::npos) {
                    m_cpu_vendor = line.substr(pos + 2);
                }
            } else if (line.find("model name") != std::string::npos) {
                size_t pos = line.find(':');
                if (pos != std::string::npos) {
                    m_cpu_brand = line.substr(pos + 2);
                }
            } else if (line.find("cpu cores") != std::string::npos) {
                size_t pos = line.find(':');
                if (pos != std::string::npos) {
                    m_cpu_cores = std::stoi(line.substr(pos + 2));
                }
            }
        }
        
        // Get CPU features
        unsigned int eax, ebx, ecx, edx;
        __get_cpuid(1, &eax, &ebx, &ecx, &edx);
        
        m_has_sse = (edx & (1 << 25)) != 0;
        m_has_sse2 = (edx & (1 << 26)) != 0;
        m_has_sse3 = (ecx & (1 << 0)) != 0;
        m_has_ssse3 = (ecx & (1 << 9)) != 0;
        m_has_sse41 = (ecx & (1 << 19)) != 0;
        m_has_sse42 = (ecx & (1 << 20)) != 0;
        m_has_avx = (ecx & (1 << 28)) != 0;
        
        __get_cpuid(7, &eax, &ebx, &ecx, &edx);
        m_has_avx2 = (ebx & (1 << 5)) != 0;
        
        // Get thread count
        m_cpu_threads = std::thread::hardware_concurrency();
    }
    
    void InitializeMemoryInfo() {
        struct sysinfo info;
        if (sysinfo(&info) == 0) {
            m_total_memory = info.totalram;
            m_page_size = getpagesize();
        }
    }
    
    // Member variables
    std::string m_cpu_vendor;
    std::string m_cpu_brand;
    uint32_t m_cpu_cores = 0;
    uint32_t m_cpu_threads = 0;
    size_t m_total_memory = 0;
    size_t m_page_size = 4096;
    
    bool m_has_sse = false;
    bool m_has_sse2 = false;
    bool m_has_sse3 = false;
    bool m_has_ssse3 = false;
    bool m_has_sse41 = false;
    bool m_has_sse42 = false;
    bool m_has_avx = false;
    bool m_has_avx2 = false;
};

LinuxOptimizer::LinuxOptimizer() : m_impl(std::make_unique<Impl>()) {
}

LinuxOptimizer::~LinuxOptimizer() = default;

HardwareCapabilities LinuxOptimizer::DetectHardware() {
    HardwareCapabilities caps;
    
    caps.cpu_vendor = m_impl->m_cpu_vendor;
    caps.cpu_brand = m_impl->m_cpu_brand;
    caps.cpu_cores = m_impl->m_cpu_cores;
    caps.cpu_threads = m_impl->m_cpu_threads;
    
    caps.has_sse = m_impl->m_has_sse;
    caps.has_sse2 = m_impl->m_has_sse2;
    caps.has_sse3 = m_impl->m_has_sse3;
    caps.has_ssse3 = m_impl->m_has_ssse3;
    caps.has_sse41 = m_impl->m_has_sse41;
    caps.has_sse42 = m_impl->m_has_sse42;
    caps.has_avx = m_impl->m_has_avx;
    caps.has_avx2 = m_impl->m_has_avx2;
    
    caps.total_memory_mb = m_impl->m_total_memory / (1024 * 1024);
    caps.page_size = m_impl->m_page_size;
    
    // GPU info would require parsing /sys/class/drm or using OpenGL
    
    return caps;
}

RenderingHints LinuxOptimizer::GetRenderingHints() {
    RenderingHints hints;
    
    hints.use_persistent_mapping = true;
    hints.use_buffer_orphaning = true;
    hints.optimal_buffer_size = 64 * 1024;
    
    hints.use_texture_arrays = true;
    hints.max_texture_size = 16384;
    
    hints.use_indirect_drawing = true;
    hints.use_instancing = true;
    hints.max_draw_calls_per_frame = 10000;
    
    hints.use_shader_cache = true;
    hints.compile_shaders_async = true;
    
    hints.use_fence_sync = true;
    hints.frame_lag = 2;
    
    return hints;
}

MemoryHints LinuxOptimizer::GetMemoryHints() {
    MemoryHints hints;
    
    // Check for huge pages support
    std::ifstream hugepages("/proc/sys/vm/nr_hugepages");
    if (hugepages.is_open()) {
        int nr_hugepages;
        hugepages >> nr_hugepages;
        hints.use_large_pages = (nr_hugepages > 0);
    }
    
    hints.use_memory_pools = true;
    hints.pool_chunk_size = 256 * 1024;
    
    hints.cache_line_size = 64;
    hints.align_to_cache_line = true;
    hints.use_prefetching = true;
    
    hints.reserve_address_space = true;
    hints.reserve_size_mb = 1024;
    hints.commit_on_demand = true;
    
    return hints;
}

ThreadingHints LinuxOptimizer::GetThreadingHints() {
    ThreadingHints hints;
    
    hints.worker_thread_count = std::thread::hardware_concurrency() - 2;
    hints.io_thread_count = 2;
    hints.use_thread_affinity = true;
    
    // Distribute cores
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    if (sched_getaffinity(0, sizeof(cpuset), &cpuset) == 0) {
        int cpu_count = CPU_COUNT(&cpuset);
        for (int i = 0; i < cpu_count; ++i) {
            if (CPU_ISSET(i, &cpuset)) {
                if (i == 0) {
                    hints.ui_thread_cores.push_back(i);
                } else if (i == 1) {
                    hints.render_thread_cores.push_back(i);
                } else {
                    hints.worker_thread_cores.push_back(i);
                }
            }
        }
    }
    
    hints.use_futex = true;
    hints.spin_count = 4000;
    
    return hints;
}

void* LinuxOptimizer::AllocateAligned(size_t size, size_t alignment) {
    void* ptr = nullptr;
    if (posix_memalign(&ptr, alignment, size) == 0) {
        return ptr;
    }
    return nullptr;
}

void LinuxOptimizer::FreeAligned(void* ptr) {
    free(ptr);
}

bool LinuxOptimizer::LockMemory(void* ptr, size_t size) {
    return mlock(ptr, size) == 0;
}

bool LinuxOptimizer::UnlockMemory(void* ptr, size_t size) {
    return munlock(ptr, size) == 0;
}

void LinuxOptimizer::SetThreadAffinity(std::thread::id thread_id, const std::vector<uint32_t>& cores) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    
    for (uint32_t core : cores) {
        CPU_SET(core, &cpuset);
    }
    
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

void LinuxOptimizer::SetThreadPriority(std::thread::id thread_id, int priority) {
    struct sched_param param;
    param.sched_priority = priority;
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
}

void LinuxOptimizer::SetThreadName(std::thread::id thread_id, const std::string& name) {
    pthread_setname_np(pthread_self(), name.c_str());
}

void LinuxOptimizer::RequestHighPerformance() {
    // Set CPU governor to performance
    system("echo performance | tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor");
}

void LinuxOptimizer::RequestPowerSaving() {
    // Set CPU governor to powersave
    system("echo powersave | tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor");
}

float LinuxOptimizer::GetBatteryLevel() {
    std::ifstream capacity("/sys/class/power_supply/BAT0/capacity");
    if (capacity.is_open()) {
        int level;
        capacity >> level;
        return static_cast<float>(level);
    }
    return 100.0f;
}

bool LinuxOptimizer::IsOnBattery() {
    std::ifstream ac("/sys/class/power_supply/AC/online");
    if (ac.is_open()) {
        int online;
        ac >> online;
        return online == 0;
    }
    return false;
}

void LinuxOptimizer::EnableVSync(bool enable) {
    // OpenGL/Vulkan VSync control
}

void LinuxOptimizer::SetSwapInterval(int interval) {
    // glXSwapIntervalEXT(interval);
}

bool LinuxOptimizer::SupportsAdaptiveSync() {
    // Check for FreeSync support
    return false;
}

void LinuxOptimizer::SetCPUGovernor(const std::string& governor) {
    std::string cmd = "echo " + governor + " | tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor";
    system(cmd.c_str());
}

void LinuxOptimizer::EnableTransparentHugePages(bool enable) {
    const char* mode = enable ? "always" : "never";
    std::string cmd = std::string("echo ") + mode + " > /sys/kernel/mm/transparent_hugepage/enabled";
    system(cmd.c_str());
}

void LinuxOptimizer::SetIOScheduler(const std::string& scheduler) {
    // Set I/O scheduler for block devices
}

bool LinuxOptimizer::UseCGroups(const std::string& group) {
    // Configure cgroups for resource limiting
    return false;
}

#endif // __linux__

// PerformanceAutoTuner implementation
PerformanceAutoTuner::PerformanceAutoTuner() {
    m_frame_time_history.reserve(HISTORY_SIZE);
    m_gpu_time_history.reserve(HISTORY_SIZE);
    m_memory_history.reserve(HISTORY_SIZE);
    
    // Set default profiles
    m_current_profile = TuningProfile{};
    m_target_profile = m_current_profile;
}

void PerformanceAutoTuner::SetTargetProfile(const TuningProfile& profile) {
    m_target_profile = profile;
}

void PerformanceAutoTuner::UpdateMetrics(float frame_time_ms, float gpu_time_ms, size_t memory_mb) {
    // Update history
    if (m_frame_time_history.size() >= HISTORY_SIZE) {
        m_frame_time_history.erase(m_frame_time_history.begin());
    }
    m_frame_time_history.push_back(frame_time_ms);
    
    if (m_gpu_time_history.size() >= HISTORY_SIZE) {
        m_gpu_time_history.erase(m_gpu_time_history.begin());
    }
    m_gpu_time_history.push_back(gpu_time_ms);
    
    if (m_memory_history.size() >= HISTORY_SIZE) {
        m_memory_history.erase(m_memory_history.begin());
    }
    m_memory_history.push_back(memory_mb);
    
    // Check if we have enough data
    if (m_frame_time_history.size() < 30) return;
    
    // Auto-tune if enabled
    if (m_auto_tuning_enabled) {
        auto now = std::chrono::steady_clock::now();
        auto time_since_adjustment = std::chrono::duration_cast<std::chrono::seconds>(
            now - m_last_adjustment).count();
        
        // Wait at least 2 seconds between adjustments
        if (time_since_adjustment >= 2) {
            AdjustQuality();
            m_last_adjustment = now;
        }
    }
}

PerformanceAutoTuner::TuningProfile PerformanceAutoTuner::GetOptimalProfile() const {
    return m_current_profile;
}

void PerformanceAutoTuner::AdjustQuality() {
    if (m_frame_time_history.empty()) return;
    
    // Calculate average frame time
    float avg_frame_time = 0.0f;
    for (float time : m_frame_time_history) {
        avg_frame_time += time;
    }
    avg_frame_time /= m_frame_time_history.size();
    
    float target_frame_time = 1000.0f / m_target_profile.target_fps;
    float min_frame_time = 1000.0f / m_target_profile.min_fps;
    
    bool changed = false;
    
    // If we're missing our target by a lot, make aggressive changes
    if (avg_frame_time > min_frame_time) {
        // Performance is too low, reduce quality
        if (m_current_profile.texture_quality > 0) {
            m_current_profile.texture_quality--;
            changed = true;
        } else if (m_current_profile.shadow_quality > 0) {
            m_current_profile.shadow_quality--;
            changed = true;
        } else if (m_current_profile.effect_quality > 0) {
            m_current_profile.effect_quality--;
            changed = true;
        }
    } else if (avg_frame_time < target_frame_time * 0.8f) {
        // Performance is good, try increasing quality
        m_stable_frames++;
        
        if (m_stable_frames > 60) { // Stable for 60 frames
            if (m_current_profile.effect_quality < m_target_profile.effect_quality) {
                m_current_profile.effect_quality++;
                changed = true;
            } else if (m_current_profile.shadow_quality < m_target_profile.shadow_quality) {
                m_current_profile.shadow_quality++;
                changed = true;
            } else if (m_current_profile.texture_quality < m_target_profile.texture_quality) {
                m_current_profile.texture_quality++;
                changed = true;
            }
            
            m_stable_frames = 0;
        }
    }
    
    // Check memory usage
    if (!m_memory_history.empty()) {
        size_t avg_memory = 0;
        for (size_t mem : m_memory_history) {
            avg_memory += mem;
        }
        avg_memory /= m_memory_history.size();
        
        if (avg_memory > m_target_profile.max_memory_mb) {
            // Reduce texture quality to save memory
            if (m_current_profile.texture_quality > 0) {
                m_current_profile.texture_quality--;
                changed = true;
            }
        }
    }
    
    // Notify if quality changed
    if (changed && m_quality_callback) {
        m_adjustment_count++;
        m_quality_callback(m_current_profile);
    }
}

void PerformanceAutoTuner::AnalyzeTrends() {
    // Analyze performance trends for predictive adjustments
    // This could include detecting patterns like:
    // - Performance degradation over time (thermal throttling)
    // - Periodic spikes (background tasks)
    // - Scene complexity changes
}

} // namespace ecscope::gui::platform