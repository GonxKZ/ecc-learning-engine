#pragma once

/**
 * @file platform/hardware_detection.hpp
 * @brief Comprehensive Cross-Platform Hardware Detection and Optimization System
 * 
 * This system provides comprehensive hardware capability detection and optimization
 * recommendations across different platforms and architectures. It serves as the
 * foundation for performance-critical applications and educational demonstrations.
 * 
 * Key Features:
 * - CPU architecture detection (x86, x86-64, ARM, ARM64, RISC-V)
 * - SIMD instruction set runtime detection with fallback mechanisms
 * - Memory hierarchy analysis (cache sizes, NUMA topology, bandwidth)
 * - Platform-specific optimizations (Windows, Linux, macOS, mobile)
 * - GPU and graphics capability detection
 * - Thermal and power management awareness
 * - Performance counter integration
 * - Educational hardware analysis and reporting
 * 
 * Educational Value:
 * - Clear explanations of hardware impact on performance
 * - Comparative analysis between different architectures
 * - Real-time performance monitoring and optimization suggestions
 * - Interactive hardware capability demonstrations
 * - Best practices for cross-platform optimization
 * 
 * @author ECScope Educational ECS Framework - Platform Optimization
 * @date 2025
 */

#include "core/types.hpp"
#include "core/log.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <chrono>
#include <functional>
#include <array>
#include <bitset>
#include <memory>

// Platform detection
#ifdef _WIN32
    #define ECSCOPE_PLATFORM_WINDOWS
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #include <intrin.h>
    #include <powerbase.h>
    #include <pdh.h>
#elif defined(__linux__)
    #define ECSCOPE_PLATFORM_LINUX
    #include <unistd.h>
    #include <sys/sysinfo.h>
    #include <cpuid.h>
    #include <sys/auxv.h>
    #include <linux/perf_event.h>
    #include <sys/syscall.h>
    #include <fcntl.h>
#elif defined(__APPLE__)
    #define ECSCOPE_PLATFORM_MACOS
    #include <sys/sysctl.h>
    #include <sys/types.h>
    #include <mach/mach.h>
    #include <mach/machine.h>
    #include <mach/mach_host.h>
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE
        #define ECSCOPE_PLATFORM_IOS
    #endif
#endif

// Architecture detection
#if defined(__x86_64__) || defined(_M_X64)
    #define ECSCOPE_ARCH_X86_64
    #define ECSCOPE_ARCH_X86
#elif defined(__i386__) || defined(_M_IX86)
    #define ECSCOPE_ARCH_X86_32
    #define ECSCOPE_ARCH_X86
#elif defined(__aarch64__) || defined(_M_ARM64)
    #define ECSCOPE_ARCH_ARM64
    #define ECSCOPE_ARCH_ARM
#elif defined(__arm__) || defined(_M_ARM)
    #define ECSCOPE_ARCH_ARM32
    #define ECSCOPE_ARCH_ARM
#elif defined(__riscv) && (__riscv_xlen == 64)
    #define ECSCOPE_ARCH_RISCV64
#elif defined(__riscv) && (__riscv_xlen == 32)
    #define ECSCOPE_ARCH_RISCV32
#endif

namespace ecscope::platform {

//=============================================================================
// Core Hardware Information Structures
//=============================================================================

/**
 * @brief CPU architecture enumeration
 */
enum class CpuArchitecture : u8 {
    Unknown,
    X86_32,
    X86_64,
    ARM_32,
    ARM_64,
    RISC_V_32,
    RISC_V_64,
    PowerPC,
    MIPS,
    SPARC
};

/**
 * @brief CPU vendor identification
 */
enum class CpuVendor : u8 {
    Unknown,
    Intel,
    AMD,
    ARM,
    Apple,
    Qualcomm,
    Samsung,
    MediaTek,
    Nvidia,
    SiFive,
    Other
};

/**
 * @brief SIMD instruction set capabilities
 */
struct SimdCapabilities {
    // x86/x64 SIMD
    bool sse = false;
    bool sse2 = false;
    bool sse3 = false;
    bool ssse3 = false;
    bool sse4_1 = false;
    bool sse4_2 = false;
    bool sse4a = false;
    bool avx = false;
    bool avx2 = false;
    bool avx512f = false;
    bool avx512dq = false;
    bool avx512cd = false;
    bool avx512bw = false;
    bool avx512vl = false;
    bool avx512vnni = false;
    bool fma3 = false;
    bool fma4 = false;
    
    // ARM SIMD
    bool neon = false;
    bool sve = false;
    bool sve2 = false;
    u32 sve_vector_length = 0; // In bits
    
    // Other extensions
    bool popcnt = false;
    bool bmi1 = false;
    bool bmi2 = false;
    bool aes_ni = false;
    bool sha = false;
    bool crc32 = false;
    
    // Vector width information
    u32 max_vector_width_bits = 128;
    u32 preferred_vector_width_bits = 128;
    
    std::string get_description() const;
    f32 get_performance_score() const;
    bool supports_simd_level(const std::string& level) const;
};

/**
 * @brief CPU cache hierarchy information
 */
struct CacheInfo {
    struct CacheLevel {
        u32 level;              // 1, 2, 3, etc.
        u32 size_bytes;         // Cache size in bytes
        u32 line_size_bytes;    // Cache line size
        u32 associativity;      // Set associativity (0 = fully associative)
        bool is_unified;        // True if instruction+data, false if split
        bool is_inclusive;      // True if inclusive of lower levels
        std::string type;       // "Data", "Instruction", "Unified"
    };
    
    std::vector<CacheLevel> levels;
    u32 total_cache_size_bytes = 0;
    u32 cache_line_size = 64;  // Most common
    
    const CacheLevel* get_level(u32 level) const;
    u32 get_total_size_for_level(u32 level) const;
    std::string get_hierarchy_description() const;
};

/**
 * @brief CPU core and thread information
 */
struct CpuTopology {
    u32 physical_cores = 0;
    u32 logical_cores = 0;
    u32 physical_processors = 0;
    u32 numa_nodes = 0;
    bool hyperthreading_enabled = false;
    
    struct CoreInfo {
        u32 core_id;
        u32 physical_id;
        u32 numa_node;
        std::vector<u32> thread_ids;
        u32 base_frequency_mhz;
        u32 max_frequency_mhz;
    };
    
    std::vector<CoreInfo> cores;
    
    f32 get_thread_efficiency_ratio() const;
    std::string get_topology_description() const;
};

/**
 * @brief Comprehensive CPU information
 */
struct CpuInfo {
    std::string brand_string;
    std::string model_name;
    CpuVendor vendor = CpuVendor::Unknown;
    CpuArchitecture architecture = CpuArchitecture::Unknown;
    
    u32 family = 0;
    u32 model = 0;
    u32 stepping = 0;
    u32 microcode_version = 0;
    
    u32 base_frequency_mhz = 0;
    u32 max_frequency_mhz = 0;
    u32 bus_frequency_mhz = 0;
    
    CpuTopology topology;
    CacheInfo cache_info;
    SimdCapabilities simd_caps;
    
    // Feature flags
    std::bitset<256> feature_flags;
    std::unordered_map<std::string, bool> extended_features;
    
    // Performance characteristics
    f32 instructions_per_cycle = 1.0f;
    f32 thermal_design_power_watts = 0.0f;
    f32 performance_per_watt_score = 0.0f;
    
    bool supports_64bit() const;
    bool supports_virtualization() const;
    bool has_integrated_graphics() const;
    f32 get_overall_performance_score() const;
    std::string get_detailed_description() const;
};

//=============================================================================
// Memory System Information
//=============================================================================

/**
 * @brief Memory hierarchy and characteristics
 */
struct MemoryInfo {
    // System memory
    u64 total_physical_memory_bytes = 0;
    u64 available_memory_bytes = 0;
    u64 total_virtual_memory_bytes = 0;
    u64 page_size_bytes = 4096;
    u64 large_page_size_bytes = 0;
    bool supports_large_pages = false;
    
    // Memory technology
    std::string memory_type;        // DDR4, DDR5, LPDDR5, etc.
    u32 memory_channels = 0;
    u32 memory_frequency_mhz = 0;
    f64 memory_bandwidth_gbps = 0.0;
    f64 memory_latency_ns = 0.0;
    
    // NUMA information
    struct NumaNode {
        u32 node_id;
        u64 memory_bytes;
        std::vector<u32> cpu_cores;
        f64 local_bandwidth_gbps;
        f64 remote_bandwidth_gbps;
        f64 local_latency_ns;
        f64 remote_latency_ns;
    };
    
    std::vector<NumaNode> numa_nodes;
    bool numa_available = false;
    
    // Memory management features
    bool supports_memory_protection = false;
    bool supports_execute_never = false;
    bool supports_memory_encryption = false;
    bool supports_memory_tagging = false;
    
    f32 get_memory_performance_score() const;
    std::string get_memory_description() const;
    bool is_numa_balanced() const;
};

//=============================================================================
// Platform-Specific Information
//=============================================================================

/**
 * @brief Operating system information
 */
struct OperatingSystemInfo {
    std::string name;           // "Windows", "Linux", "macOS", etc.
    std::string version;        // "10.0.19041", "5.4.0-42", "11.6", etc.
    std::string distribution;   // "Ubuntu 20.04", "Windows 10 Pro", etc.
    std::string kernel_version; // Kernel version string
    
    bool is_64bit = false;
    bool supports_containers = false;
    bool supports_virtualization = false;
    bool has_realtime_scheduler = false;
    
    // Power management
    bool supports_cpu_scaling = false;
    bool supports_sleep_states = false;
    std::vector<std::string> available_governors;
    
    std::string get_platform_description() const;
    bool is_suitable_for_realtime() const;
};

/**
 * @brief Compiler and toolchain information
 */
struct CompilerInfo {
    std::string name;           // "GCC", "Clang", "MSVC", etc.
    std::string version;        // Version string
    std::string target_triple;  // Target architecture triple
    
    // Optimization capabilities
    std::vector<std::string> supported_optimizations;
    std::vector<std::string> available_sanitizers;
    bool supports_lto = false;
    bool supports_pgo = false;
    bool supports_vectorization = false;
    
    std::string get_optimization_recommendations() const;
};

//=============================================================================
// Graphics and Accelerator Information
//=============================================================================

/**
 * @brief Graphics hardware information
 */
struct GraphicsInfo {
    struct GpuDevice {
        std::string name;
        std::string vendor;
        std::string driver_version;
        u64 memory_bytes;
        u32 compute_units;
        u32 max_frequency_mhz;
        f32 performance_score;
        
        // API support
        bool supports_opengl = false;
        std::string opengl_version;
        bool supports_vulkan = false;
        std::string vulkan_version;
        bool supports_directx = false;
        std::string directx_version;
        bool supports_metal = false;
        bool supports_opencl = false;
        bool supports_cuda = false;
        
        // Compute capabilities
        u32 max_work_group_size = 0;
        std::array<u32, 3> max_work_group_dimensions = {0, 0, 0};
        bool supports_fp16 = false;
        bool supports_fp64 = false;
        bool supports_int8 = false;
    };
    
    std::vector<GpuDevice> devices;
    bool has_discrete_gpu = false;
    bool has_integrated_gpu = false;
    
    const GpuDevice* get_primary_device() const;
    f32 get_total_compute_score() const;
    std::string get_graphics_summary() const;
};

//=============================================================================
// Performance Monitoring and Measurement
//=============================================================================

/**
 * @brief Performance counter information
 */
struct PerformanceCounterInfo {
    bool supports_hardware_counters = false;
    bool supports_software_counters = false;
    std::vector<std::string> available_counter_types;
    
    // Common counter capabilities
    bool can_measure_cycles = false;
    bool can_measure_instructions = false;
    bool can_measure_cache_misses = false;
    bool can_measure_branch_mispredicts = false;
    bool can_measure_memory_bandwidth = false;
    bool can_measure_energy_consumption = false;
    
    std::string get_monitoring_capabilities() const;
};

/**
 * @brief Thermal and power management information
 */
struct ThermalInfo {
    // Temperature monitoring
    bool supports_cpu_temperature = false;
    bool supports_gpu_temperature = false;
    f32 cpu_temperature_celsius = 0.0f;
    f32 gpu_temperature_celsius = 0.0f;
    f32 thermal_throttle_threshold = 0.0f;
    
    // Power management
    bool supports_power_monitoring = false;
    f32 cpu_power_consumption_watts = 0.0f;
    f32 gpu_power_consumption_watts = 0.0f;
    f32 system_power_consumption_watts = 0.0f;
    
    // Battery information (for mobile)
    bool has_battery = false;
    f32 battery_capacity_wh = 0.0f;
    f32 battery_level_percent = 0.0f;
    bool is_charging = false;
    
    // Thermal states
    enum class ThermalState {
        Cool,
        Nominal,
        Warm,
        Hot,
        Critical,
        Throttled
    };
    
    ThermalState current_thermal_state = ThermalState::Nominal;
    
    bool is_thermal_throttling() const;
    f32 get_thermal_headroom() const;
    std::string get_power_profile_recommendation() const;
};

//=============================================================================
// Hardware Detection Engine
//=============================================================================

/**
 * @brief Comprehensive hardware detection and analysis system
 */
class HardwareDetector {
private:
    // Cached information
    mutable std::optional<CpuInfo> cached_cpu_info_;
    mutable std::optional<MemoryInfo> cached_memory_info_;
    mutable std::optional<OperatingSystemInfo> cached_os_info_;
    mutable std::optional<CompilerInfo> cached_compiler_info_;
    mutable std::optional<GraphicsInfo> cached_graphics_info_;
    mutable std::optional<PerformanceCounterInfo> cached_perf_info_;
    mutable std::optional<ThermalInfo> cached_thermal_info_;
    
    // Detection timestamps
    mutable std::chrono::steady_clock::time_point last_detection_;
    mutable std::chrono::seconds cache_validity_duration_{60}; // 1 minute default
    
    // Platform-specific handles and state
#ifdef ECSCOPE_PLATFORM_WINDOWS
    mutable SYSTEM_INFO win_system_info_;
    mutable bool win_info_initialized_ = false;
#endif
    
public:
    HardwareDetector();
    ~HardwareDetector();
    
    // Non-copyable
    HardwareDetector(const HardwareDetector&) = delete;
    HardwareDetector& operator=(const HardwareDetector&) = delete;
    
    // Core detection methods
    const CpuInfo& get_cpu_info() const;
    const MemoryInfo& get_memory_info() const;
    const OperatingSystemInfo& get_os_info() const;
    const CompilerInfo& get_compiler_info() const;
    const GraphicsInfo& get_graphics_info() const;
    const PerformanceCounterInfo& get_performance_counter_info() const;
    const ThermalInfo& get_thermal_info() const;
    
    // Refresh and cache control
    void refresh_all_info();
    void refresh_dynamic_info(); // Only refresh info that changes (thermal, power, etc.)
    void set_cache_validity(std::chrono::seconds duration);
    bool is_cache_valid() const;
    void clear_cache();
    
    // Capability queries
    bool supports_simd(const std::string& instruction_set) const;
    bool supports_numa() const;
    bool supports_large_pages() const;
    bool supports_hardware_performance_counters() const;
    bool supports_gpu_compute() const;
    bool is_mobile_platform() const;
    bool is_low_power_device() const;
    
    // Performance characteristics
    f32 get_cpu_performance_score() const;
    f32 get_memory_performance_score() const;
    f32 get_graphics_performance_score() const;
    f32 get_overall_system_score() const;
    
    // Platform-specific optimizations
    std::vector<std::string> get_recommended_compiler_flags() const;
    std::vector<std::string> get_recommended_runtime_optimizations() const;
    std::string get_optimal_thread_count_recommendation() const;
    std::string get_optimal_memory_layout_recommendation() const;
    
private:
    // Platform-specific detection implementations
    CpuInfo detect_cpu_info() const;
    MemoryInfo detect_memory_info() const;
    OperatingSystemInfo detect_os_info() const;
    CompilerInfo detect_compiler_info() const;
    GraphicsInfo detect_graphics_info() const;
    PerformanceCounterInfo detect_performance_counter_info() const;
    ThermalInfo detect_thermal_info() const;
    
    // Architecture-specific helpers
#ifdef ECSCOPE_ARCH_X86
    SimdCapabilities detect_x86_simd_capabilities() const;
    CacheInfo detect_x86_cache_info() const;
    void detect_x86_cpu_features(CpuInfo& info) const;
#endif
    
#ifdef ECSCOPE_ARCH_ARM
    SimdCapabilities detect_arm_simd_capabilities() const;
    void detect_arm_cpu_features(CpuInfo& info) const;
#endif
    
    // Platform-specific implementations
#ifdef ECSCOPE_PLATFORM_WINDOWS
    void detect_windows_specific_info(OperatingSystemInfo& os_info) const;
    ThermalInfo detect_windows_thermal_info() const;
#endif
    
#ifdef ECSCOPE_PLATFORM_LINUX
    void detect_linux_specific_info(OperatingSystemInfo& os_info) const;
    ThermalInfo detect_linux_thermal_info() const;
    GraphicsInfo detect_linux_graphics_info() const;
#endif
    
#ifdef ECSCOPE_PLATFORM_MACOS
    void detect_macos_specific_info(OperatingSystemInfo& os_info) const;
    ThermalInfo detect_macos_thermal_info() const;
#endif
    
    // Utility functions
    bool is_cache_outdated() const;
    std::string execute_command(const std::string& command) const;
    std::vector<std::string> read_file_lines(const std::string& filename) const;
    bool file_exists(const std::string& filename) const;
};

//=============================================================================
// Global Hardware Detection Instance
//=============================================================================

/**
 * @brief Get the global hardware detector instance
 */
HardwareDetector& get_hardware_detector();

/**
 * @brief Convenience functions for quick access
 */
namespace quick_detect {
    CpuArchitecture get_cpu_architecture();
    bool has_avx2();
    bool has_avx512();
    bool has_neon();
    u32 get_physical_core_count();
    u32 get_logical_core_count();
    u64 get_total_memory_bytes();
    bool is_numa_system();
    bool has_discrete_gpu();
    std::string get_platform_name();
}

} // namespace ecscope::platform