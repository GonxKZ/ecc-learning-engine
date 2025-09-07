#pragma once

/**
 * @file cross_platform_profiling.hpp
 * @brief Cross-Platform Profiling Support for ECScope Advanced Profiling
 * 
 * This comprehensive cross-platform profiling system provides:
 * - Windows-specific profiling with Performance Data Helper (PDH) API
 * - Linux profiling with /proc filesystem and perf integration
 * - macOS profiling with Mach kernel interfaces and Instruments integration
 * - Platform-specific optimizations for CPU, memory, and GPU metrics
 * - Unified interface for cross-platform compatibility
 * - Hardware performance counter access
 * - System-specific debugging features
 * 
 * The system automatically detects the platform and provides the best
 * available profiling capabilities while maintaining a consistent API.
 * 
 * @author ECScope Development Team
 * @date 2024
 */

#include "advanced_profiler.hpp"
#include "types.hpp"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
    #define ECSCOPE_PLATFORM_WINDOWS
    #include <windows.h>
    #include <psapi.h>
    #include <pdh.h>
    #include <pdhmsg.h>
    #include <tlhelp32.h>
    #include <winternl.h>
    #include <ntstatus.h>
    #include <intrin.h>
    
    #ifdef ECSCOPE_ENABLE_ETW
        #include <evntrace.h>
        #include <tdh.h>
    #endif
    
    #ifdef ECSCOPE_ENABLE_D3D_PROFILING
        #include <d3d11.h>
        #include <dxgi.h>
        #include <d3d11_1.h>
    #endif
    
#elif defined(__linux__)
    #define ECSCOPE_PLATFORM_LINUX
    #include <unistd.h>
    #include <sys/resource.h>
    #include <sys/time.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <sys/sysinfo.h>
    #include <proc/readproc.h>
    #include <fstream>
    #include <dirent.h>
    
    #ifdef ECSCOPE_ENABLE_PERF
        #include <linux/perf_event.h>
        #include <sys/syscall.h>
        #include <asm/unistd.h>
    #endif
    
#elif defined(__APPLE__)
    #define ECSCOPE_PLATFORM_MACOS
    #include <mach/mach.h>
    #include <mach/task_info.h>
    #include <mach/mach_init.h>
    #include <mach/host_info.h>
    #include <mach/vm_map.h>
    #include <sys/resource.h>
    #include <sys/sysctl.h>
    #include <libproc.h>
    
    #ifdef ECSCOPE_ENABLE_METAL_PROFILING
        #include <Metal/Metal.h>
        #include <MetalPerformanceShaders/MetalPerformanceShaders.h>
    #endif
    
#else
    #define ECSCOPE_PLATFORM_UNKNOWN
    #warning "Unknown platform - profiling capabilities will be limited"
#endif

namespace ecscope::profiling {

//=============================================================================
// Platform-Specific Data Structures
//=============================================================================

// CPU information structure
struct CPUInfo {
    std::string vendor;
    std::string brand;
    std::string architecture;
    u32 physical_cores;
    u32 logical_cores;
    u32 l1_cache_size;
    u32 l2_cache_size;
    u32 l3_cache_size;
    f64 base_frequency;
    f64 max_frequency;
    std::vector<std::string> features;
    
    // Real-time metrics
    f64 overall_usage_percent;
    std::vector<f64> per_core_usage;
    f64 temperature; // Celsius, if available
    f64 power_consumption; // Watts, if available
    u64 context_switches;
    u64 interrupts;
    
    void reset_metrics() {
        overall_usage_percent = 0.0;
        per_core_usage.clear();
        temperature = 0.0;
        power_consumption = 0.0;
        context_switches = 0;
        interrupts = 0;
    }
};

// Memory system information
struct MemorySystemInfo {
    // Physical memory
    usize total_physical;
    usize available_physical;
    usize used_physical;
    
    // Virtual memory
    usize total_virtual;
    usize available_virtual;
    usize used_virtual;
    
    // Process-specific
    usize process_working_set;
    usize process_peak_working_set;
    usize process_private_bytes;
    usize process_virtual_bytes;
    usize process_paged_pool;
    usize process_non_paged_pool;
    
    // Memory performance
    u64 page_faults;
    u64 page_fault_rate;
    u64 cache_faults;
    f64 memory_load_percent;
    
    // Memory bandwidth (if available)
    f64 memory_bandwidth_mbps;
    f64 memory_latency_ns;
    
    void reset_metrics() {
        page_faults = 0;
        page_fault_rate = 0;
        cache_faults = 0;
        memory_load_percent = 0.0;
        memory_bandwidth_mbps = 0.0;
        memory_latency_ns = 0.0;
    }
};

// GPU information structure
struct GPUSystemInfo {
    std::string name;
    std::string vendor;
    std::string driver_version;
    usize total_memory;
    usize available_memory;
    usize used_memory;
    
    // GPU performance metrics
    f32 gpu_utilization;
    f32 memory_utilization;
    f32 temperature;
    f32 power_consumption;
    u32 shader_clock;
    u32 memory_clock;
    u32 fan_speed;
    
    // Performance counters
    u64 rendered_frames;
    u64 dropped_frames;
    f64 average_frame_time;
    u32 draw_calls_per_frame;
    u32 vertices_per_frame;
    
    void reset_metrics() {
        gpu_utilization = 0.0f;
        memory_utilization = 0.0f;
        temperature = 0.0f;
        power_consumption = 0.0f;
        rendered_frames = 0;
        dropped_frames = 0;
        average_frame_time = 0.0;
        draw_calls_per_frame = 0;
        vertices_per_frame = 0;
    }
};

// Process information
struct ProcessInfo {
    u32 process_id;
    u32 parent_process_id;
    std::string process_name;
    std::string command_line;
    
    // Process metrics
    f64 cpu_usage_percent;
    usize memory_usage_bytes;
    u64 handle_count;
    u64 thread_count;
    u64 io_read_bytes;
    u64 io_write_bytes;
    u64 io_operations;
    
    // Process timing
    std::chrono::system_clock::time_point creation_time;
    std::chrono::high_resolution_clock::duration user_time;
    std::chrono::high_resolution_clock::duration kernel_time;
    
    void reset_metrics() {
        cpu_usage_percent = 0.0;
        memory_usage_bytes = 0;
        handle_count = 0;
        thread_count = 0;
        io_read_bytes = 0;
        io_write_bytes = 0;
        io_operations = 0;
        user_time = std::chrono::high_resolution_clock::duration{0};
        kernel_time = std::chrono::high_resolution_clock::duration{0};
    }
};

//=============================================================================
// Platform-Specific Profiler Interfaces
//=============================================================================

// Base platform profiler interface
class IPlatformProfiler {
public:
    virtual ~IPlatformProfiler() = default;
    
    // Initialization
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual bool is_initialized() const = 0;
    
    // System information
    virtual CPUInfo get_cpu_info() = 0;
    virtual MemorySystemInfo get_memory_info() = 0;
    virtual GPUSystemInfo get_gpu_info() = 0;
    virtual ProcessInfo get_process_info(u32 process_id = 0) = 0; // 0 = current process
    
    // Performance monitoring
    virtual void start_monitoring() = 0;
    virtual void stop_monitoring() = 0;
    virtual void update_metrics() = 0;
    
    // Hardware counters (if available)
    virtual bool supports_hardware_counters() const = 0;
    virtual std::vector<std::string> get_available_counters() const = 0;
    virtual bool start_counter(const std::string& counter_name) = 0;
    virtual void stop_counter(const std::string& counter_name) = 0;
    virtual u64 read_counter(const std::string& counter_name) = 0;
    
    // Platform-specific features
    virtual std::vector<std::string> get_platform_features() const = 0;
    virtual bool has_feature(const std::string& feature) const = 0;
    virtual std::string get_platform_name() const = 0;
    virtual std::string get_platform_version() const = 0;
};

//=============================================================================
// Windows Platform Profiler
//=============================================================================

#ifdef ECSCOPE_PLATFORM_WINDOWS

class WindowsPlatformProfiler : public IPlatformProfiler {
private:
    // Windows-specific handles and data
    HANDLE process_handle_;
    PDH_HQUERY cpu_query_;
    PDH_HCOUNTER cpu_total_counter_;
    std::vector<PDH_HCOUNTER> cpu_core_counters_;
    PDH_HCOUNTER memory_counter_;
    
    // Performance counters
    std::unordered_map<std::string, PDH_HCOUNTER> custom_counters_;
    std::unordered_map<std::string, u64> counter_values_;
    
    // ETW (Event Tracing for Windows) support
    #ifdef ECSCOPE_ENABLE_ETW
    EVENT_TRACE_PROPERTIES* trace_properties_;
    TRACEHANDLE trace_session_;
    bool etw_enabled_;
    #endif
    
    // GPU profiling (DirectX)
    #ifdef ECSCOPE_ENABLE_D3D_PROFILING
    ID3D11Device* d3d_device_;
    IDXGIAdapter* dxgi_adapter_;
    DXGI_ADAPTER_DESC adapter_desc_;
    #endif
    
    // Cached information
    CPUInfo cpu_info_;
    MemorySystemInfo memory_info_;
    GPUSystemInfo gpu_info_;
    bool info_cached_;
    
    // Timing for delta calculations
    std::chrono::high_resolution_clock::time_point last_update_;
    FILETIME last_process_kernel_time_;
    FILETIME last_process_user_time_;
    ULARGE_INTEGER last_system_idle_time_;
    ULARGE_INTEGER last_system_kernel_time_;
    ULARGE_INTEGER last_system_user_time_;
    
public:
    WindowsPlatformProfiler();
    ~WindowsPlatformProfiler();
    
    // IPlatformProfiler implementation
    bool initialize() override;
    void shutdown() override;
    bool is_initialized() const override { return process_handle_ != nullptr; }
    
    CPUInfo get_cpu_info() override;
    MemorySystemInfo get_memory_info() override;
    GPUSystemInfo get_gpu_info() override;
    ProcessInfo get_process_info(u32 process_id = 0) override;
    
    void start_monitoring() override;
    void stop_monitoring() override;
    void update_metrics() override;
    
    bool supports_hardware_counters() const override { return true; }
    std::vector<std::string> get_available_counters() const override;
    bool start_counter(const std::string& counter_name) override;
    void stop_counter(const std::string& counter_name) override;
    u64 read_counter(const std::string& counter_name) override;
    
    std::vector<std::string> get_platform_features() const override;
    bool has_feature(const std::string& feature) const override;
    std::string get_platform_name() const override { return "Windows"; }
    std::string get_platform_version() const override;
    
private:
    void initialize_cpu_info();
    void initialize_memory_info();
    void initialize_gpu_info();
    void initialize_performance_counters();
    
    void update_cpu_metrics();
    void update_memory_metrics();
    void update_gpu_metrics();
    void update_process_metrics();
    
    std::string get_cpu_vendor_from_cpuid();
    std::string get_cpu_brand_from_cpuid();
    std::vector<std::string> get_cpu_features_from_cpuid();
    f64 get_cpu_frequency_from_registry();
    
    // Windows-specific utility functions
    std::string get_windows_version();
    bool is_wow64_process();
    void enable_debug_privileges();
    
    #ifdef ECSCOPE_ENABLE_ETW
    void initialize_etw();
    void shutdown_etw();
    static VOID WINAPI etw_event_callback(PEVENT_RECORD event_record);
    #endif
    
    #ifdef ECSCOPE_ENABLE_D3D_PROFILING
    void initialize_d3d_profiling();
    void shutdown_d3d_profiling();
    void update_d3d_metrics();
    #endif
};

#endif // ECSCOPE_PLATFORM_WINDOWS

//=============================================================================
// Linux Platform Profiler
//=============================================================================

#ifdef ECSCOPE_PLATFORM_LINUX

class LinuxPlatformProfiler : public IPlatformProfiler {
private:
    // Linux-specific data
    std::ifstream proc_stat_;
    std::ifstream proc_meminfo_;
    std::ifstream proc_cpuinfo_;
    std::ifstream proc_loadavg_;
    
    // Performance event file descriptors (perf)
    #ifdef ECSCOPE_ENABLE_PERF
    std::unordered_map<std::string, int> perf_fds_;
    std::unordered_map<std::string, u64> perf_values_;
    #endif
    
    // Cached information
    CPUInfo cpu_info_;
    MemorySystemInfo memory_info_;
    GPUSystemInfo gpu_info_;
    bool info_cached_;
    
    // Previous values for delta calculations
    std::vector<u64> last_cpu_times_;
    u64 last_total_cpu_time_;
    u64 last_idle_cpu_time_;
    std::chrono::high_resolution_clock::time_point last_update_;
    
public:
    LinuxPlatformProfiler();
    ~LinuxPlatformProfiler();
    
    // IPlatformProfiler implementation
    bool initialize() override;
    void shutdown() override;
    bool is_initialized() const override;
    
    CPUInfo get_cpu_info() override;
    MemorySystemInfo get_memory_info() override;
    GPUSystemInfo get_gpu_info() override;
    ProcessInfo get_process_info(u32 process_id = 0) override;
    
    void start_monitoring() override;
    void stop_monitoring() override;
    void update_metrics() override;
    
    bool supports_hardware_counters() const override;
    std::vector<std::string> get_available_counters() const override;
    bool start_counter(const std::string& counter_name) override;
    void stop_counter(const std::string& counter_name) override;
    u64 read_counter(const std::string& counter_name) override;
    
    std::vector<std::string> get_platform_features() const override;
    bool has_feature(const std::string& feature) const override;
    std::string get_platform_name() const override { return "Linux"; }
    std::string get_platform_version() const override;
    
private:
    void initialize_cpu_info();
    void initialize_memory_info();
    void initialize_gpu_info();
    
    void update_cpu_metrics();
    void update_memory_metrics();
    void update_gpu_metrics();
    void update_process_metrics();
    
    // Linux-specific utility functions
    std::string read_file_content(const std::string& filepath);
    std::vector<std::string> split_string(const std::string& str, char delimiter);
    bool file_exists(const std::string& filepath);
    
    std::vector<u64> parse_cpu_times(const std::string& stat_line);
    void parse_meminfo();
    void parse_cpuinfo();
    
    #ifdef ECSCOPE_ENABLE_PERF
    void initialize_perf_events();
    void shutdown_perf_events();
    int setup_perf_event(u32 type, u64 config);
    #endif
    
    // GPU detection and monitoring for various drivers
    void detect_nvidia_gpu();
    void detect_amd_gpu();
    void detect_intel_gpu();
    void update_nvidia_metrics();
    void update_amd_metrics();
    void update_intel_metrics();
};

#endif // ECSCOPE_PLATFORM_LINUX

//=============================================================================
// macOS Platform Profiler
//=============================================================================

#ifdef ECSCOPE_PLATFORM_MACOS

class MacOSPlatformProfiler : public IPlatformProfiler {
private:
    // macOS-specific data
    mach_port_t host_port_;
    mach_port_t task_port_;
    
    // Host info structures
    host_basic_info_data_t host_basic_info_;
    host_cpu_load_info_data_t cpu_load_info_;
    host_cpu_load_info_data_t prev_cpu_load_info_;
    vm_statistics64_data_t vm_stat_;
    
    // Task info structures
    task_basic_info_data_t task_basic_info_;
    task_thread_times_info_data_t task_times_info_;
    
    #ifdef ECSCOPE_ENABLE_METAL_PROFILING
    id<MTLDevice> metal_device_;
    #endif
    
    // Cached information
    CPUInfo cpu_info_;
    MemorySystemInfo memory_info_;
    GPUSystemInfo gpu_info_;
    bool info_cached_;
    
    // Timing
    std::chrono::high_resolution_clock::time_point last_update_;
    
public:
    MacOSPlatformProfiler();
    ~MacOSPlatformProfiler();
    
    // IPlatformProfiler implementation
    bool initialize() override;
    void shutdown() override;
    bool is_initialized() const override { return host_port_ != MACH_PORT_NULL; }
    
    CPUInfo get_cpu_info() override;
    MemorySystemInfo get_memory_info() override;
    GPUSystemInfo get_gpu_info() override;
    ProcessInfo get_process_info(u32 process_id = 0) override;
    
    void start_monitoring() override;
    void stop_monitoring() override;
    void update_metrics() override;
    
    bool supports_hardware_counters() const override { return false; } // Limited on macOS
    std::vector<std::string> get_available_counters() const override;
    bool start_counter(const std::string& counter_name) override;
    void stop_counter(const std::string& counter_name) override;
    u64 read_counter(const std::string& counter_name) override;
    
    std::vector<std::string> get_platform_features() const override;
    bool has_feature(const std::string& feature) const override;
    std::string get_platform_name() const override { return "macOS"; }
    std::string get_platform_version() const override;
    
private:
    void initialize_cpu_info();
    void initialize_memory_info();
    void initialize_gpu_info();
    
    void update_cpu_metrics();
    void update_memory_metrics();
    void update_gpu_metrics();
    void update_process_metrics();
    
    // macOS-specific utility functions
    std::string get_sysctl_string(const std::string& name);
    u64 get_sysctl_uint64(const std::string& name);
    u32 get_sysctl_uint32(const std::string& name);
    
    std::string get_cpu_brand_name();
    std::vector<std::string> get_cpu_features();
    
    #ifdef ECSCOPE_ENABLE_METAL_PROFILING
    void initialize_metal_profiling();
    void shutdown_metal_profiling();
    void update_metal_metrics();
    #endif
};

#endif // ECSCOPE_PLATFORM_MACOS

//=============================================================================
// Cross-Platform Profiling Manager
//=============================================================================

class CrossPlatformProfiler {
private:
    std::unique_ptr<IPlatformProfiler> platform_profiler_;
    std::atomic<bool> monitoring_active_;
    std::unique_ptr<std::thread> monitoring_thread_;
    std::mutex data_mutex_;
    
    // Unified metrics
    CPUInfo current_cpu_info_;
    MemorySystemInfo current_memory_info_;
    GPUSystemInfo current_gpu_info_;
    ProcessInfo current_process_info_;
    
    // Configuration
    f32 update_frequency_hz_;
    bool auto_detect_capabilities_;
    
    // Performance data history
    static constexpr usize HISTORY_SIZE = 1000;
    std::array<f64, HISTORY_SIZE> cpu_usage_history_;
    std::array<usize, HISTORY_SIZE> memory_usage_history_;
    std::array<f32, HISTORY_SIZE> gpu_usage_history_;
    usize history_index_;
    
    // Callbacks for real-time updates
    std::vector<std::function<void(const CPUInfo&)>> cpu_update_callbacks_;
    std::vector<std::function<void(const MemorySystemInfo&)>> memory_update_callbacks_;
    std::vector<std::function<void(const GPUSystemInfo&)>> gpu_update_callbacks_;
    
public:
    CrossPlatformProfiler(f32 update_frequency = 10.0f);
    ~CrossPlatformProfiler();
    
    // Core interface
    bool initialize();
    void shutdown();
    bool is_initialized() const;
    
    void start_monitoring();
    void stop_monitoring();
    bool is_monitoring() const { return monitoring_active_; }
    
    // System information access
    CPUInfo get_cpu_info() const;
    MemorySystemInfo get_memory_info() const;
    GPUSystemInfo get_gpu_info() const;
    ProcessInfo get_process_info() const;
    
    // Platform capabilities
    std::string get_platform_name() const;
    std::string get_platform_version() const;
    std::vector<std::string> get_platform_features() const;
    bool has_feature(const std::string& feature) const;
    bool supports_hardware_counters() const;
    
    // Performance monitoring
    void set_update_frequency(f32 frequency_hz) { update_frequency_hz_ = frequency_hz; }
    f32 get_update_frequency() const { return update_frequency_hz_; }
    
    // Hardware counters
    std::vector<std::string> get_available_hardware_counters() const;
    bool start_hardware_counter(const std::string& counter_name);
    void stop_hardware_counter(const std::string& counter_name);
    u64 read_hardware_counter(const std::string& counter_name);
    
    // Historical data
    std::vector<f64> get_cpu_usage_history(usize samples = HISTORY_SIZE) const;
    std::vector<usize> get_memory_usage_history(usize samples = HISTORY_SIZE) const;
    std::vector<f32> get_gpu_usage_history(usize samples = HISTORY_SIZE) const;
    
    // Callback registration
    void register_cpu_update_callback(std::function<void(const CPUInfo&)> callback);
    void register_memory_update_callback(std::function<void(const MemorySystemInfo&)> callback);
    void register_gpu_update_callback(std::function<void(const GPUSystemInfo&)> callback);
    
    // Utility methods
    std::string generate_system_report() const;
    void export_metrics_to_file(const std::string& filename) const;
    
    // Integration with AdvancedProfiler
    void integrate_with_profiler(AdvancedProfiler* profiler);
    
private:
    void monitoring_thread_main();
    void update_all_metrics();
    void notify_callbacks();
    void update_history();
    
    std::unique_ptr<IPlatformProfiler> create_platform_profiler();
    
    template<typename T>
    std::vector<T> get_history_data(const std::array<T, HISTORY_SIZE>& history_array, usize samples) const {
        std::lock_guard<std::mutex> lock(data_mutex_);
        std::vector<T> result;
        
        usize start_index = (history_index_ >= samples) ? history_index_ - samples : 0;
        usize end_index = history_index_;
        
        for (usize i = start_index; i < end_index; ++i) {
            result.push_back(history_array[i % HISTORY_SIZE]);
        }
        
        return result;
    }
};

//=============================================================================
// Platform-Specific Utilities
//=============================================================================

namespace platform_utils {

// CPU feature detection
bool has_sse();
bool has_sse2();
bool has_sse3();
bool has_sse41();
bool has_sse42();
bool has_avx();
bool has_avx2();
bool has_avx512();

// Memory information
usize get_page_size();
usize get_cache_line_size();
usize get_total_physical_memory();
usize get_available_physical_memory();

// Process utilities
u32 get_current_process_id();
u32 get_current_thread_id();
std::string get_process_name(u32 process_id = 0);
std::vector<u32> get_process_threads(u32 process_id = 0);

// System utilities
u32 get_cpu_core_count();
u32 get_cpu_thread_count();
std::string get_operating_system_version();
bool is_debugger_present();
bool is_running_under_debugger();

// High-precision timing
u64 get_cpu_cycles();
u64 get_cpu_frequency();
f64 get_high_precision_time();

// Memory allocation tracking
void* tracked_malloc(usize size, const std::string& category = "general");
void* tracked_realloc(void* ptr, usize size, const std::string& category = "general");
void tracked_free(void* ptr);

} // namespace platform_utils

} // namespace ecscope::profiling