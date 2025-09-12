#pragma once

#include "../core/types.hpp"
#include <memory>
#include <functional>
#include <vector>
#include <atomic>
#include <thread>

namespace ecscope::gui::platform {

// Forward declarations
class PlatformOptimizer;
class WindowsOptimizer;
class LinuxOptimizer;
class MacOSOptimizer;

// Platform detection
enum class Platform {
    WINDOWS,
    LINUX,
    MACOS,
    UNKNOWN
};

// Hardware capabilities
struct HardwareCapabilities {
    // CPU
    std::string cpu_vendor;
    std::string cpu_brand;
    uint32_t cpu_cores = 0;
    uint32_t cpu_threads = 0;
    uint64_t cpu_frequency_mhz = 0;
    size_t cache_line_size = 64;
    size_t l1_cache_size = 0;
    size_t l2_cache_size = 0;
    size_t l3_cache_size = 0;
    
    // SIMD support
    bool has_sse = false;
    bool has_sse2 = false;
    bool has_sse3 = false;
    bool has_ssse3 = false;
    bool has_sse41 = false;
    bool has_sse42 = false;
    bool has_avx = false;
    bool has_avx2 = false;
    bool has_avx512 = false;
    bool has_neon = false; // ARM
    
    // GPU
    std::string gpu_vendor;
    std::string gpu_renderer;
    size_t gpu_memory_mb = 0;
    uint32_t gpu_compute_units = 0;
    
    // Memory
    size_t total_memory_mb = 0;
    size_t available_memory_mb = 0;
    size_t page_size = 4096;
    
    // Display
    uint32_t display_count = 0;
    uint32_t primary_display_width = 0;
    uint32_t primary_display_height = 0;
    float primary_display_dpi = 96.0f;
    uint32_t primary_display_refresh_rate = 60;
    bool supports_hdr = false;
    
    // Platform features
    bool supports_direct_storage = false;
    bool supports_gpu_upload_heaps = false;
    bool supports_mesh_shaders = false;
    bool supports_variable_rate_shading = false;
    bool supports_ray_tracing = false;
};

// Platform-specific rendering hints
struct RenderingHints {
    // Buffer strategies
    bool use_persistent_mapping = false;
    bool use_buffer_orphaning = false;
    bool use_unsynchronized_mapping = false;
    size_t optimal_buffer_size = 0;
    
    // Texture strategies
    bool use_texture_arrays = false;
    bool use_bindless_textures = false;
    bool use_sparse_textures = false;
    uint32_t max_texture_size = 0;
    
    // Draw call strategies
    bool use_indirect_drawing = false;
    bool use_multi_draw_indirect = false;
    bool use_instancing = true;
    uint32_t max_draw_calls_per_frame = 0;
    
    // Shader strategies
    bool use_shader_cache = true;
    bool use_pipeline_cache = true;
    bool compile_shaders_async = true;
    
    // Synchronization
    bool use_fence_sync = true;
    bool use_events = false;
    uint32_t frame_lag = 2;
};

// Memory allocation hints
struct MemoryHints {
    // Allocation strategies
    bool use_large_pages = false;
    bool use_numa_aware_allocation = false;
    bool use_memory_pools = true;
    size_t pool_chunk_size = 0;
    
    // Cache optimization
    size_t cache_line_size = 64;
    bool align_to_cache_line = true;
    bool use_prefetching = true;
    
    // Virtual memory
    bool reserve_address_space = false;
    size_t reserve_size_mb = 0;
    bool commit_on_demand = true;
};

// Thread optimization hints
struct ThreadingHints {
    // Thread pool configuration
    uint32_t worker_thread_count = 0;
    uint32_t io_thread_count = 0;
    bool use_thread_affinity = false;
    std::vector<uint32_t> ui_thread_cores;
    std::vector<uint32_t> render_thread_cores;
    std::vector<uint32_t> worker_thread_cores;
    
    // Synchronization
    bool use_spinlocks = false;
    bool use_futex = false; // Linux
    bool use_critical_sections = false; // Windows
    uint32_t spin_count = 0;
    
    // Priority
    int ui_thread_priority = 0;
    int render_thread_priority = 0;
    int worker_thread_priority = 0;
};

// Base platform optimizer
class PlatformOptimizer {
public:
    virtual ~PlatformOptimizer() = default;
    
    // Hardware detection
    virtual HardwareCapabilities DetectHardware() = 0;
    
    // Optimization hints
    virtual RenderingHints GetRenderingHints() = 0;
    virtual MemoryHints GetMemoryHints() = 0;
    virtual ThreadingHints GetThreadingHints() = 0;
    
    // Memory management
    virtual void* AllocateAligned(size_t size, size_t alignment) = 0;
    virtual void FreeAligned(void* ptr) = 0;
    virtual bool LockMemory(void* ptr, size_t size) = 0;
    virtual bool UnlockMemory(void* ptr, size_t size) = 0;
    
    // Thread management
    virtual void SetThreadAffinity(std::thread::id thread_id, const std::vector<uint32_t>& cores) = 0;
    virtual void SetThreadPriority(std::thread::id thread_id, int priority) = 0;
    virtual void SetThreadName(std::thread::id thread_id, const std::string& name) = 0;
    
    // Power management
    virtual void RequestHighPerformance() = 0;
    virtual void RequestPowerSaving() = 0;
    virtual float GetBatteryLevel() = 0;
    virtual bool IsOnBattery() = 0;
    
    // Platform-specific features
    virtual void EnableVSync(bool enable) = 0;
    virtual void SetSwapInterval(int interval) = 0;
    virtual bool SupportsAdaptiveSync() = 0;
    
    // Factory method
    static std::unique_ptr<PlatformOptimizer> Create();
    static Platform GetCurrentPlatform();
};

#ifdef _WIN32
// Windows-specific optimizations
class WindowsOptimizer : public PlatformOptimizer {
public:
    WindowsOptimizer();
    ~WindowsOptimizer();
    
    HardwareCapabilities DetectHardware() override;
    RenderingHints GetRenderingHints() override;
    MemoryHints GetMemoryHints() override;
    ThreadingHints GetThreadingHints() override;
    
    void* AllocateAligned(size_t size, size_t alignment) override;
    void FreeAligned(void* ptr) override;
    bool LockMemory(void* ptr, size_t size) override;
    bool UnlockMemory(void* ptr, size_t size) override;
    
    void SetThreadAffinity(std::thread::id thread_id, const std::vector<uint32_t>& cores) override;
    void SetThreadPriority(std::thread::id thread_id, int priority) override;
    void SetThreadName(std::thread::id thread_id, const std::string& name) override;
    
    void RequestHighPerformance() override;
    void RequestPowerSaving() override;
    float GetBatteryLevel() override;
    bool IsOnBattery() override;
    
    void EnableVSync(bool enable) override;
    void SetSwapInterval(int interval) override;
    bool SupportsAdaptiveSync() override;
    
    // Windows-specific features
    void EnableDirectStorage(bool enable);
    void EnableGPUScheduling(bool enable);
    void SetProcessPriority(uint32_t priority);
    bool RegisterForMemoryNotifications();
    
private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};
#endif

#ifdef __linux__
// Linux-specific optimizations
class LinuxOptimizer : public PlatformOptimizer {
public:
    LinuxOptimizer();
    ~LinuxOptimizer();
    
    HardwareCapabilities DetectHardware() override;
    RenderingHints GetRenderingHints() override;
    MemoryHints GetMemoryHints() override;
    ThreadingHints GetThreadingHints() override;
    
    void* AllocateAligned(size_t size, size_t alignment) override;
    void FreeAligned(void* ptr) override;
    bool LockMemory(void* ptr, size_t size) override;
    bool UnlockMemory(void* ptr, size_t size) override;
    
    void SetThreadAffinity(std::thread::id thread_id, const std::vector<uint32_t>& cores) override;
    void SetThreadPriority(std::thread::id thread_id, int priority) override;
    void SetThreadName(std::thread::id thread_id, const std::string& name) override;
    
    void RequestHighPerformance() override;
    void RequestPowerSaving() override;
    float GetBatteryLevel() override;
    bool IsOnBattery() override;
    
    void EnableVSync(bool enable) override;
    void SetSwapInterval(int interval) override;
    bool SupportsAdaptiveSync() override;
    
    // Linux-specific features
    void SetCPUGovernor(const std::string& governor);
    void EnableTransparentHugePages(bool enable);
    void SetIOScheduler(const std::string& scheduler);
    bool UseCGroups(const std::string& group);
    
private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};
#endif

#ifdef __APPLE__
// macOS-specific optimizations
class MacOSOptimizer : public PlatformOptimizer {
public:
    MacOSOptimizer();
    ~MacOSOptimizer();
    
    HardwareCapabilities DetectHardware() override;
    RenderingHints GetRenderingHints() override;
    MemoryHints GetMemoryHints() override;
    ThreadingHints GetThreadingHints() override;
    
    void* AllocateAligned(size_t size, size_t alignment) override;
    void FreeAligned(void* ptr) override;
    bool LockMemory(void* ptr, size_t size) override;
    bool UnlockMemory(void* ptr, size_t size) override;
    
    void SetThreadAffinity(std::thread::id thread_id, const std::vector<uint32_t>& cores) override;
    void SetThreadPriority(std::thread::id thread_id, int priority) override;
    void SetThreadName(std::thread::id thread_id, const std::string& name) override;
    
    void RequestHighPerformance() override;
    void RequestPowerSaving() override;
    float GetBatteryLevel() override;
    bool IsOnBattery() override;
    
    void EnableVSync(bool enable) override;
    void SetSwapInterval(int interval) override;
    bool SupportsAdaptiveSync() override;
    
    // macOS-specific features
    void EnableMetalPerformanceShaders(bool enable);
    void SetQualityOfService(uint32_t qos_class);
    void EnableAutomaticGraphicsSwitching(bool enable);
    bool UseUnifiedMemory();
    
private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};
#endif

// Cross-platform optimization utilities
namespace utils {

// CPU feature detection
inline bool HasSSE() {
#ifdef _MSC_VER
    int cpuinfo[4];
    __cpuid(cpuinfo, 1);
    return (cpuinfo[3] & (1 << 25)) != 0;
#else
    return __builtin_cpu_supports("sse");
#endif
}

inline bool HasAVX() {
#ifdef _MSC_VER
    int cpuinfo[4];
    __cpuid(cpuinfo, 1);
    return (cpuinfo[2] & (1 << 28)) != 0;
#else
    return __builtin_cpu_supports("avx");
#endif
}

inline bool HasAVX2() {
#ifdef _MSC_VER
    int cpuinfo[4];
    __cpuidex(cpuinfo, 7, 0);
    return (cpuinfo[1] & (1 << 5)) != 0;
#else
    return __builtin_cpu_supports("avx2");
#endif
}

// Memory prefetching
inline void Prefetch(const void* addr, int hint = 0) {
#ifdef _MSC_VER
    _mm_prefetch(reinterpret_cast<const char*>(addr), hint);
#else
    __builtin_prefetch(addr, 0, hint);
#endif
}

// Compiler hints
#ifdef _MSC_VER
    #define LIKELY(x) (x)
    #define UNLIKELY(x) (x)
    #define RESTRICT __restrict
    #define FORCE_INLINE __forceinline
    #define NO_INLINE __declspec(noinline)
#else
    #define LIKELY(x) __builtin_expect(!!(x), 1)
    #define UNLIKELY(x) __builtin_expect(!!(x), 0)
    #define RESTRICT __restrict__
    #define FORCE_INLINE __attribute__((always_inline)) inline
    #define NO_INLINE __attribute__((noinline))
#endif

// Cache line alignment
#ifdef _MSC_VER
    #define CACHE_ALIGNED __declspec(align(64))
#else
    #define CACHE_ALIGNED __attribute__((aligned(64)))
#endif

// Memory barriers
inline void MemoryBarrier() {
#ifdef _MSC_VER
    _ReadWriteBarrier();
#else
    __sync_synchronize();
#endif
}

inline void CompilerBarrier() {
#ifdef _MSC_VER
    _ReadWriteBarrier();
#else
    asm volatile("" ::: "memory");
#endif
}

} // namespace utils

// Performance auto-tuner
class PerformanceAutoTuner {
public:
    struct TuningProfile {
        // Rendering
        uint32_t target_fps = 60;
        uint32_t min_fps = 30;
        float gpu_budget_ms = 16.0f;
        float cpu_budget_ms = 8.0f;
        
        // Quality settings
        uint32_t texture_quality = 2; // 0=low, 1=medium, 2=high
        uint32_t shadow_quality = 2;
        uint32_t effect_quality = 2;
        uint32_t ui_scale = 100; // percentage
        
        // Resource limits
        size_t max_memory_mb = 512;
        uint32_t max_draw_calls = 1000;
        uint32_t max_triangles = 1000000;
    };
    
    PerformanceAutoTuner();
    
    void SetTargetProfile(const TuningProfile& profile);
    void EnableAutoTuning(bool enable) { m_auto_tuning_enabled = enable; }
    
    void UpdateMetrics(float frame_time_ms, float gpu_time_ms, size_t memory_mb);
    TuningProfile GetOptimalProfile() const;
    
    // Callbacks for quality adjustments
    using QualityChangeCallback = std::function<void(const TuningProfile&)>;
    void SetQualityChangeCallback(QualityChangeCallback callback) {
        m_quality_callback = callback;
    }
    
private:
    void AdjustQuality();
    void AnalyzeTrends();
    
    TuningProfile m_current_profile;
    TuningProfile m_target_profile;
    
    std::atomic<bool> m_auto_tuning_enabled{true};
    QualityChangeCallback m_quality_callback;
    
    // Performance history
    static constexpr size_t HISTORY_SIZE = 120;
    std::vector<float> m_frame_time_history;
    std::vector<float> m_gpu_time_history;
    std::vector<size_t> m_memory_history;
    
    // Tuning state
    std::chrono::steady_clock::time_point m_last_adjustment;
    uint32_t m_stable_frames = 0;
    uint32_t m_adjustment_count = 0;
};

} // namespace ecscope::gui::platform