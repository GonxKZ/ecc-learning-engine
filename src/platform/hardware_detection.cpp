#include "hardware_detection.hpp"
#include "core/log.hpp"
#include <sstream>
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <cstring>
#include <thread>
#include <regex>

#ifdef ECSCOPE_ARCH_X86
    #ifdef _MSC_VER
        #include <intrin.h>
    #else
        #include <cpuid.h>
    #endif
#endif

namespace ecscope::platform {

//=============================================================================
// Utility Functions
//=============================================================================

namespace {
    
    /**
     * @brief Execute a system command and return output
     */
    std::string execute_system_command(const std::string& command) {
        std::string result;
        #ifdef ECSCOPE_PLATFORM_WINDOWS
        // Windows implementation with _popen
        FILE* pipe = _popen(command.c_str(), "r");
        #else
        // Unix-like systems
        FILE* pipe = popen(command.c_str(), "r");
        #endif
        
        if (pipe) {
            char buffer[256];
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                result += buffer;
            }
            #ifdef ECSCOPE_PLATFORM_WINDOWS
            _pclose(pipe);
            #else
            pclose(pipe);
            #endif
        }
        return result;
    }
    
    /**
     * @brief Read file contents as lines
     */
    std::vector<std::string> read_file_lines(const std::string& filename) {
        std::vector<std::string> lines;
        std::ifstream file(filename);
        std::string line;
        
        while (std::getline(file, line)) {
            lines.push_back(line);
        }
        
        return lines;
    }
    
    /**
     * @brief Check if file exists
     */
    bool file_exists(const std::string& filename) {
        std::ifstream file(filename);
        return file.good();
    }
    
    /**
     * @brief Parse frequency string (e.g., "2.4 GHz" -> 2400)
     */
    u32 parse_frequency_mhz(const std::string& freq_str) {
        std::regex freq_regex(R"((\d+\.?\d*)\s*([GMK]?)Hz)");
        std::smatch match;
        
        if (std::regex_search(freq_str, match, freq_regex)) {
            f32 value = std::stof(match[1].str());
            std::string unit = match[2].str();
            
            if (unit == "G") return static_cast<u32>(value * 1000.0f);
            if (unit == "M") return static_cast<u32>(value);
            if (unit == "K") return static_cast<u32>(value / 1000.0f);
            return static_cast<u32>(value); // Assume MHz if no unit
        }
        
        return 0;
    }
    
    /**
     * @brief Parse memory size string (e.g., "8 GB" -> bytes)
     */
    u64 parse_memory_size(const std::string& size_str) {
        std::regex size_regex(R"((\d+\.?\d*)\s*([KMGTPE]?)B?)");
        std::smatch match;
        
        if (std::regex_search(size_str, match, size_regex)) {
            f64 value = std::stod(match[1].str());
            std::string unit = match[2].str();
            
            const f64 multipliers[] = {1, 1024, 1024*1024, 1024*1024*1024, 
                                     1024LL*1024*1024*1024, 1024LL*1024*1024*1024*1024,
                                     1024LL*1024*1024*1024*1024*1024};
            const std::string units[] = {"", "K", "M", "G", "T", "P", "E"};
            
            for (usize i = 0; i < 7; ++i) {
                if (unit == units[i]) {
                    return static_cast<u64>(value * multipliers[i]);
                }
            }
        }
        
        return 0;
    }
    
} // anonymous namespace

//=============================================================================
// SIMD Capabilities Implementation
//=============================================================================

std::string SimdCapabilities::get_description() const {
    std::stringstream ss;
    
    if (avx512f) {
        ss << "AVX-512 (512-bit vectors)";
        if (avx512vnni) ss << " with VNNI";
    } else if (avx2) {
        ss << "AVX2 (256-bit vectors)";
    } else if (avx) {
        ss << "AVX (256-bit vectors)";
    } else if (sse4_2) {
        ss << "SSE4.2 (128-bit vectors)";
    } else if (sse4_1) {
        ss << "SSE4.1 (128-bit vectors)";
    } else if (sse3) {
        ss << "SSE3 (128-bit vectors)";
    } else if (sse2) {
        ss << "SSE2 (128-bit vectors)";
    } else if (sse) {
        ss << "SSE (128-bit vectors)";
    }
    
    if (neon) {
        if (!ss.str().empty()) ss << ", ";
        ss << "ARM NEON";
        if (sve) {
            ss << ", SVE (" << sve_vector_length << "-bit vectors)";
        }
    }
    
    if (fma3) {
        if (!ss.str().empty()) ss << ", ";
        ss << "FMA3";
    }
    
    if (ss.str().empty()) {
        ss << "Scalar only";
    }
    
    return ss.str();
}

f32 SimdCapabilities::get_performance_score() const {
    f32 score = 1.0f; // Base scalar score
    
    // x86 SIMD scoring
    if (sse) score = std::max(score, 2.0f);
    if (sse2) score = std::max(score, 3.0f);
    if (sse3) score = std::max(score, 3.5f);
    if (sse4_1) score = std::max(score, 4.0f);
    if (sse4_2) score = std::max(score, 4.5f);
    if (avx) score = std::max(score, 6.0f);
    if (avx2) score = std::max(score, 8.0f);
    if (avx512f) score = std::max(score, 12.0f);
    if (avx512vnni) score = std::max(score, 15.0f);
    
    // ARM SIMD scoring
    if (neon) score = std::max(score, 6.0f);
    if (sve) score = std::max(score, 10.0f + sve_vector_length / 128.0f);
    if (sve2) score = std::max(score, 12.0f + sve_vector_length / 128.0f);
    
    // Bonus for additional features
    if (fma3 || fma4) score *= 1.2f;
    if (aes_ni) score *= 1.1f;
    if (popcnt) score *= 1.05f;
    
    return score;
}

bool SimdCapabilities::supports_simd_level(const std::string& level) const {
    std::string lower_level = level;
    std::transform(lower_level.begin(), lower_level.end(), lower_level.begin(), ::tolower);
    
    if (lower_level == "sse" && sse) return true;
    if (lower_level == "sse2" && sse2) return true;
    if (lower_level == "sse3" && sse3) return true;
    if (lower_level == "sse4.1" && sse4_1) return true;
    if (lower_level == "sse4.2" && sse4_2) return true;
    if (lower_level == "avx" && avx) return true;
    if (lower_level == "avx2" && avx2) return true;
    if (lower_level == "avx512" && avx512f) return true;
    if (lower_level == "neon" && neon) return true;
    if (lower_level == "sve" && sve) return true;
    
    return false;
}

//=============================================================================
// Cache Information Implementation
//=============================================================================

const CacheInfo::CacheLevel* CacheInfo::get_level(u32 level) const {
    auto it = std::find_if(levels.begin(), levels.end(),
        [level](const CacheLevel& cl) { return cl.level == level; });
    return (it != levels.end()) ? &(*it) : nullptr;
}

u32 CacheInfo::get_total_size_for_level(u32 level) const {
    u32 total = 0;
    for (const auto& cache : levels) {
        if (cache.level == level) {
            total += cache.size_bytes;
        }
    }
    return total;
}

std::string CacheInfo::get_hierarchy_description() const {
    std::stringstream ss;
    
    for (u32 level = 1; level <= 4; ++level) {
        u32 size = get_total_size_for_level(level);
        if (size > 0) {
            if (level > 1) ss << ", ";
            ss << "L" << level << ": ";
            
            if (size >= 1024 * 1024) {
                ss << (size / (1024 * 1024)) << " MB";
            } else {
                ss << (size / 1024) << " KB";
            }
        }
    }
    
    return ss.str();
}

//=============================================================================
// CPU Topology Implementation
//=============================================================================

f32 CpuTopology::get_thread_efficiency_ratio() const {
    if (physical_cores == 0) return 1.0f;
    return static_cast<f32>(logical_cores) / static_cast<f32>(physical_cores);
}

std::string CpuTopology::get_topology_description() const {
    std::stringstream ss;
    ss << physical_cores << " cores";
    
    if (hyperthreading_enabled && logical_cores > physical_cores) {
        ss << " (" << logical_cores << " threads with hyperthreading)";
    }
    
    if (numa_nodes > 1) {
        ss << " across " << numa_nodes << " NUMA nodes";
    }
    
    return ss.str();
}

//=============================================================================
// CPU Information Implementation
//=============================================================================

bool CpuInfo::supports_64bit() const {
    return architecture == CpuArchitecture::X86_64 || 
           architecture == CpuArchitecture::ARM_64 ||
           architecture == CpuArchitecture::RISC_V_64;
}

bool CpuInfo::supports_virtualization() const {
    return extended_features.count("vmx") > 0 ||    // Intel VT-x
           extended_features.count("svm") > 0 ||    // AMD-V
           extended_features.count("ept") > 0;      // Extended Page Tables
}

bool CpuInfo::has_integrated_graphics() const {
    return extended_features.count("integrated_gpu") > 0;
}

f32 CpuInfo::get_overall_performance_score() const {
    f32 score = 1.0f;
    
    // Base score from core count
    score *= (topology.physical_cores * 2.0f + topology.logical_cores) / 3.0f;
    
    // Frequency bonus
    if (max_frequency_mhz > 0) {
        score *= (max_frequency_mhz / 2000.0f); // Normalized to 2GHz base
    }
    
    // Architecture bonus
    switch (architecture) {
        case CpuArchitecture::X86_64:
            score *= 1.2f;
            break;
        case CpuArchitecture::ARM_64:
            score *= 1.1f;
            break;
        case CpuArchitecture::X86_32:
            score *= 0.8f;
            break;
        default:
            break;
    }
    
    // SIMD bonus
    score *= simd_caps.get_performance_score() / 4.0f;
    
    // Cache bonus
    if (cache_info.total_cache_size_bytes > 0) {
        f32 cache_mb = cache_info.total_cache_size_bytes / (1024.0f * 1024.0f);
        score *= (1.0f + cache_mb / 32.0f); // Bonus for larger caches
    }
    
    return score;
}

std::string CpuInfo::get_detailed_description() const {
    std::stringstream ss;
    
    ss << brand_string;
    if (!model_name.empty() && model_name != brand_string) {
        ss << " (" << model_name << ")";
    }
    
    ss << "\nArchitecture: ";
    switch (architecture) {
        case CpuArchitecture::X86_32: ss << "x86 32-bit"; break;
        case CpuArchitecture::X86_64: ss << "x86-64"; break;
        case CpuArchitecture::ARM_32: ss << "ARM 32-bit"; break;
        case CpuArchitecture::ARM_64: ss << "ARM 64-bit"; break;
        case CpuArchitecture::RISC_V_32: ss << "RISC-V 32-bit"; break;
        case CpuArchitecture::RISC_V_64: ss << "RISC-V 64-bit"; break;
        default: ss << "Unknown"; break;
    }
    
    ss << "\nTopology: " << topology.get_topology_description();
    
    if (max_frequency_mhz > 0) {
        ss << "\nFrequency: " << base_frequency_mhz << " MHz base";
        if (max_frequency_mhz != base_frequency_mhz) {
            ss << ", " << max_frequency_mhz << " MHz max";
        }
    }
    
    if (!cache_info.levels.empty()) {
        ss << "\nCache: " << cache_info.get_hierarchy_description();
    }
    
    ss << "\nSIMD: " << simd_caps.get_description();
    
    ss << "\nPerformance Score: " << std::fixed << std::setprecision(1) 
       << get_overall_performance_score();
    
    return ss.str();
}

//=============================================================================
// Memory Information Implementation
//=============================================================================

f32 MemoryInfo::get_memory_performance_score() const {
    f32 score = 1.0f;
    
    // Base score from memory size
    f32 gb = total_physical_memory_bytes / (1024.0f * 1024.0f * 1024.0f);
    score *= std::min(gb / 16.0f, 2.0f); // Normalized to 16GB, max 2x bonus
    
    // Bandwidth bonus
    if (memory_bandwidth_gbps > 0) {
        score *= (memory_bandwidth_gbps / 50.0f); // Normalized to 50 GB/s
    }
    
    // Latency penalty
    if (memory_latency_ns > 0) {
        score *= std::max(0.5f, 100.0f / memory_latency_ns); // Lower latency is better
    }
    
    // Technology bonus
    if (memory_type.find("DDR5") != std::string::npos) score *= 1.3f;
    else if (memory_type.find("DDR4") != std::string::npos) score *= 1.1f;
    else if (memory_type.find("LPDDR5") != std::string::npos) score *= 1.25f;
    
    // Channel bonus
    if (memory_channels > 1) {
        score *= (1.0f + (memory_channels - 1) * 0.2f);
    }
    
    return score;
}

std::string MemoryInfo::get_memory_description() const {
    std::stringstream ss;
    
    f32 gb = total_physical_memory_bytes / (1024.0f * 1024.0f * 1024.0f);
    ss << std::fixed << std::setprecision(1) << gb << " GB";
    
    if (!memory_type.empty()) {
        ss << " " << memory_type;
    }
    
    if (memory_frequency_mhz > 0) {
        ss << " @ " << memory_frequency_mhz << " MHz";
    }
    
    if (memory_channels > 1) {
        ss << " (" << memory_channels << " channels)";
    }
    
    if (memory_bandwidth_gbps > 0) {
        ss << ", " << std::fixed << std::setprecision(1) << memory_bandwidth_gbps << " GB/s";
    }
    
    if (numa_available) {
        ss << " with NUMA";
    }
    
    return ss.str();
}

bool MemoryInfo::is_numa_balanced() const {
    if (!numa_available || numa_nodes.size() < 2) return true;
    
    // Check if memory is roughly evenly distributed
    u64 expected_per_node = total_physical_memory_bytes / numa_nodes.size();
    u64 tolerance = expected_per_node / 4; // 25% tolerance
    
    for (const auto& node : numa_nodes) {
        if (node.memory_bytes < expected_per_node - tolerance ||
            node.memory_bytes > expected_per_node + tolerance) {
            return false;
        }
    }
    
    return true;
}

//=============================================================================
// Operating System Information Implementation
//=============================================================================

std::string OperatingSystemInfo::get_platform_description() const {
    std::stringstream ss;
    ss << name;
    
    if (!distribution.empty() && distribution != name) {
        ss << " (" << distribution << ")";
    }
    
    if (!version.empty()) {
        ss << " " << version;
    }
    
    ss << (is_64bit ? " 64-bit" : " 32-bit");
    
    return ss.str();
}

bool OperatingSystemInfo::is_suitable_for_realtime() const {
    return has_realtime_scheduler && 
           (name == "Linux" || name.find("RT") != std::string::npos);
}

//=============================================================================
// Hardware Detector Implementation
//=============================================================================

HardwareDetector::HardwareDetector() = default;

HardwareDetector::~HardwareDetector() = default;

const CpuInfo& HardwareDetector::get_cpu_info() const {
    if (!cached_cpu_info_.has_value() || is_cache_outdated()) {
        cached_cpu_info_ = detect_cpu_info();
        last_detection_ = std::chrono::steady_clock::now();
    }
    return cached_cpu_info_.value();
}

const MemoryInfo& HardwareDetector::get_memory_info() const {
    if (!cached_memory_info_.has_value() || is_cache_outdated()) {
        cached_memory_info_ = detect_memory_info();
        last_detection_ = std::chrono::steady_clock::now();
    }
    return cached_memory_info_.value();
}

const OperatingSystemInfo& HardwareDetector::get_os_info() const {
    if (!cached_os_info_.has_value() || is_cache_outdated()) {
        cached_os_info_ = detect_os_info();
        last_detection_ = std::chrono::steady_clock::now();
    }
    return cached_os_info_.value();
}

const CompilerInfo& HardwareDetector::get_compiler_info() const {
    if (!cached_compiler_info_.has_value() || is_cache_outdated()) {
        cached_compiler_info_ = detect_compiler_info();
        last_detection_ = std::chrono::steady_clock::now();
    }
    return cached_compiler_info_.value();
}

const GraphicsInfo& HardwareDetector::get_graphics_info() const {
    if (!cached_graphics_info_.has_value() || is_cache_outdated()) {
        cached_graphics_info_ = detect_graphics_info();
        last_detection_ = std::chrono::steady_clock::now();
    }
    return cached_graphics_info_.value();
}

const PerformanceCounterInfo& HardwareDetector::get_performance_counter_info() const {
    if (!cached_perf_info_.has_value() || is_cache_outdated()) {
        cached_perf_info_ = detect_performance_counter_info();
        last_detection_ = std::chrono::steady_clock::now();
    }
    return cached_perf_info_.value();
}

const ThermalInfo& HardwareDetector::get_thermal_info() const {
    // Thermal info is always refreshed as it changes frequently
    cached_thermal_info_ = detect_thermal_info();
    return cached_thermal_info_.value();
}

void HardwareDetector::refresh_all_info() {
    clear_cache();
}

void HardwareDetector::refresh_dynamic_info() {
    cached_thermal_info_.reset();
}

void HardwareDetector::set_cache_validity(std::chrono::seconds duration) {
    cache_validity_duration_ = duration;
}

bool HardwareDetector::is_cache_valid() const {
    return !is_cache_outdated();
}

void HardwareDetector::clear_cache() {
    cached_cpu_info_.reset();
    cached_memory_info_.reset();
    cached_os_info_.reset();
    cached_compiler_info_.reset();
    cached_graphics_info_.reset();
    cached_perf_info_.reset();
    cached_thermal_info_.reset();
}

bool HardwareDetector::is_cache_outdated() const {
    auto now = std::chrono::steady_clock::now();
    return (now - last_detection_) > cache_validity_duration_;
}

//=============================================================================
// CPU Detection Implementation
//=============================================================================

CpuInfo HardwareDetector::detect_cpu_info() const {
    CpuInfo info;
    
    // Detect architecture
    #ifdef ECSCOPE_ARCH_X86_64
        info.architecture = CpuArchitecture::X86_64;
    #elif defined(ECSCOPE_ARCH_X86_32)
        info.architecture = CpuArchitecture::X86_32;
    #elif defined(ECSCOPE_ARCH_ARM64)
        info.architecture = CpuArchitecture::ARM_64;
    #elif defined(ECSCOPE_ARCH_ARM32)
        info.architecture = CpuArchitecture::ARM_32;
    #elif defined(ECSCOPE_ARCH_RISCV64)
        info.architecture = CpuArchitecture::RISC_V_64;
    #elif defined(ECSCOPE_ARCH_RISCV32)
        info.architecture = CpuArchitecture::RISC_V_32;
    #endif
    
    // Get basic topology
    info.topology.physical_cores = std::thread::hardware_concurrency();
    info.topology.logical_cores = std::thread::hardware_concurrency();
    
    #ifdef ECSCOPE_ARCH_X86
        detect_x86_cpu_features(info);
    #elif defined(ECSCOPE_ARCH_ARM)
        detect_arm_cpu_features(info);
    #endif
    
    // Platform-specific detection
    #ifdef ECSCOPE_PLATFORM_LINUX
        // Read /proc/cpuinfo for additional details
        auto lines = read_file_lines("/proc/cpuinfo");
        for (const auto& line : lines) {
            if (line.find("model name") != std::string::npos) {
                auto pos = line.find(": ");
                if (pos != std::string::npos) {
                    info.model_name = line.substr(pos + 2);
                    break;
                }
            }
        }
        
        // Try to get frequency from scaling governor
        if (file_exists("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq")) {
            auto freq_lines = read_file_lines("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
            if (!freq_lines.empty()) {
                info.max_frequency_mhz = std::stoul(freq_lines[0]) / 1000; // Convert from kHz
            }
        }
    #endif
    
    #ifdef ECSCOPE_PLATFORM_WINDOWS
        // Use Windows API to get CPU info
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        info.topology.logical_cores = sysInfo.dwNumberOfProcessors;
        
        // Get CPU name from registry or WMI would go here
        info.model_name = "Windows CPU"; // Placeholder
    #endif
    
    #ifdef ECSCOPE_PLATFORM_MACOS
        // Use sysctl to get CPU info
        size_t size = 0;
        char brand_string[256];
        size = sizeof(brand_string);
        if (sysctlbyname("machdep.cpu.brand_string", brand_string, &size, nullptr, 0) == 0) {
            info.brand_string = brand_string;
        }
        
        uint64_t freq = 0;
        size = sizeof(freq);
        if (sysctlbyname("hw.cpufrequency_max", &freq, &size, nullptr, 0) == 0) {
            info.max_frequency_mhz = static_cast<u32>(freq / 1000000);
        }
    #endif
    
    return info;
}

//=============================================================================
// Architecture-Specific Detection
//=============================================================================

#ifdef ECSCOPE_ARCH_X86

void HardwareDetector::detect_x86_cpu_features(CpuInfo& info) const {
    u32 regs[4] = {0};
    
    #ifdef _MSC_VER
        __cpuid(reinterpret_cast<int*>(regs), 0);
    #else
        __get_cpuid(0, &regs[0], &regs[1], &regs[2], &regs[3]);
    #endif
    
    // Extract vendor string
    char vendor[13] = {0};
    memcpy(vendor, &regs[1], 4);
    memcpy(vendor + 4, &regs[3], 4);
    memcpy(vendor + 8, &regs[2], 4);
    
    std::string vendor_str(vendor);
    if (vendor_str == "GenuineIntel") {
        info.vendor = CpuVendor::Intel;
    } else if (vendor_str == "AuthenticAMD") {
        info.vendor = CpuVendor::AMD;
    }
    
    // Get feature flags
    if (regs[0] >= 1) {
        #ifdef _MSC_VER
            __cpuid(reinterpret_cast<int*>(regs), 1);
        #else
            __get_cpuid(1, &regs[0], &regs[1], &regs[2], &regs[3]);
        #endif
        
        info.family = (regs[0] >> 8) & 0xF;
        info.model = (regs[0] >> 4) & 0xF;
        info.stepping = regs[0] & 0xF;
        
        // Extended family/model
        if (info.family == 0xF) {
            info.family += (regs[0] >> 20) & 0xFF;
        }
        if (info.family == 0xF || info.family == 0x6) {
            info.model += ((regs[0] >> 16) & 0xF) << 4;
        }
        
        // Feature detection
        info.simd_caps.sse = (regs[3] & (1 << 25)) != 0;
        info.simd_caps.sse2 = (regs[3] & (1 << 26)) != 0;
        info.simd_caps.sse3 = (regs[2] & (1 << 0)) != 0;
        info.simd_caps.ssse3 = (regs[2] & (1 << 9)) != 0;
        info.simd_caps.sse4_1 = (regs[2] & (1 << 19)) != 0;
        info.simd_caps.sse4_2 = (regs[2] & (1 << 20)) != 0;
        info.simd_caps.avx = (regs[2] & (1 << 28)) != 0;
        info.simd_caps.fma3 = (regs[2] & (1 << 12)) != 0;
        info.simd_caps.popcnt = (regs[2] & (1 << 23)) != 0;
        info.simd_caps.aes_ni = (regs[2] & (1 << 25)) != 0;
        
        info.topology.hyperthreading_enabled = (regs[3] & (1 << 28)) != 0;
    }
    
    // Extended features (leaf 7)
    #ifdef _MSC_VER
        __cpuid(reinterpret_cast<int*>(regs), 7);
    #else
        __get_cpuid_count(7, 0, &regs[0], &regs[1], &regs[2], &regs[3]);
    #endif
    
    if (regs[0] >= 7) {
        info.simd_caps.avx2 = (regs[1] & (1 << 5)) != 0;
        info.simd_caps.bmi1 = (regs[1] & (1 << 3)) != 0;
        info.simd_caps.bmi2 = (regs[1] & (1 << 8)) != 0;
        info.simd_caps.avx512f = (regs[1] & (1 << 16)) != 0;
        info.simd_caps.avx512dq = (regs[1] & (1 << 17)) != 0;
        info.simd_caps.avx512cd = (regs[1] & (1 << 28)) != 0;
        info.simd_caps.avx512bw = (regs[1] & (1 << 30)) != 0;
        info.simd_caps.avx512vl = (regs[1] & (1 << 31)) != 0;
        info.simd_caps.sha = (regs[1] & (1 << 29)) != 0;
    }
    
    // Set vector widths based on capabilities
    if (info.simd_caps.avx512f) {
        info.simd_caps.max_vector_width_bits = 512;
        info.simd_caps.preferred_vector_width_bits = 512;
    } else if (info.simd_caps.avx || info.simd_caps.avx2) {
        info.simd_caps.max_vector_width_bits = 256;
        info.simd_caps.preferred_vector_width_bits = 256;
    } else if (info.simd_caps.sse) {
        info.simd_caps.max_vector_width_bits = 128;
        info.simd_caps.preferred_vector_width_bits = 128;
    }
    
    // Cache detection would go here
    info.cache_info = detect_x86_cache_info();
}

CacheInfo HardwareDetector::detect_x86_cache_info() const {
    CacheInfo cache_info;
    
    // Simplified cache detection - full implementation would decode all cache descriptors
    u32 regs[4];
    
    #ifdef _MSC_VER
        __cpuid(reinterpret_cast<int*>(regs), 2);
    #else
        __get_cpuid(2, &regs[0], &regs[1], &regs[2], &regs[3]);
    #endif
    
    // Basic cache size estimation based on common configurations
    // This is a simplified version - real implementation would parse all descriptors
    
    CacheInfo::CacheLevel l1d = {1, 32768, 64, 8, false, false, "Data"};
    CacheInfo::CacheLevel l1i = {1, 32768, 64, 8, false, false, "Instruction"};
    CacheInfo::CacheLevel l2 = {2, 262144, 64, 8, true, false, "Unified"};
    CacheInfo::CacheLevel l3 = {3, 8388608, 64, 16, true, true, "Unified"};
    
    cache_info.levels = {l1d, l1i, l2, l3};
    cache_info.total_cache_size_bytes = l1d.size_bytes + l1i.size_bytes + l2.size_bytes + l3.size_bytes;
    cache_info.cache_line_size = 64;
    
    return cache_info;
}

SimdCapabilities HardwareDetector::detect_x86_simd_capabilities() const {
    // This is already implemented in detect_x86_cpu_features
    // Just return the detected capabilities
    return get_cpu_info().simd_caps;
}

#endif // ECSCOPE_ARCH_X86

#ifdef ECSCOPE_ARCH_ARM

void HardwareDetector::detect_arm_cpu_features(CpuInfo& info) const {
    #ifdef ECSCOPE_PLATFORM_LINUX
        // Read from /proc/cpuinfo
        auto lines = read_file_lines("/proc/cpuinfo");
        
        for (const auto& line : lines) {
            if (line.find("Features") != std::string::npos || 
                line.find("features") != std::string::npos) {
                
                std::string features_line = line;
                std::transform(features_line.begin(), features_line.end(), 
                             features_line.begin(), ::tolower);
                
                info.simd_caps.neon = features_line.find("neon") != std::string::npos;
                info.simd_caps.crc32 = features_line.find("crc32") != std::string::npos;
                info.simd_caps.aes_ni = features_line.find("aes") != std::string::npos;
                info.simd_caps.sha = features_line.find("sha") != std::string::npos;
                
                // SVE detection
                info.simd_caps.sve = features_line.find("sve") != std::string::npos;
                if (info.simd_caps.sve) {
                    // Try to get SVE vector length from auxval
                    #ifdef AT_HWCAP2
                    unsigned long hwcap2 = getauxval(AT_HWCAP2);
                    // SVE vector length detection would be more complex
                    info.simd_caps.sve_vector_length = 256; // Default assumption
                    #endif
                }
            }
            
            if (line.find("CPU implementer") != std::string::npos) {
                if (line.find("0x41") != std::string::npos) {
                    info.vendor = CpuVendor::ARM;
                } else if (line.find("0x51") != std::string::npos) {
                    info.vendor = CpuVendor::Qualcomm;
                }
            }
        }
        
        // Set vector widths
        if (info.simd_caps.sve) {
            info.simd_caps.max_vector_width_bits = info.simd_caps.sve_vector_length;
            info.simd_caps.preferred_vector_width_bits = info.simd_caps.sve_vector_length;
        } else if (info.simd_caps.neon) {
            info.simd_caps.max_vector_width_bits = 128;
            info.simd_caps.preferred_vector_width_bits = 128;
        }
        
    #endif // ECSCOPE_PLATFORM_LINUX
}

SimdCapabilities HardwareDetector::detect_arm_simd_capabilities() const {
    // This is already implemented in detect_arm_cpu_features
    return get_cpu_info().simd_caps;
}

#endif // ECSCOPE_ARCH_ARM

//=============================================================================
// Memory Detection Implementation
//=============================================================================

MemoryInfo HardwareDetector::detect_memory_info() const {
    MemoryInfo info;
    
    #ifdef ECSCOPE_PLATFORM_LINUX
        struct sysinfo sys_info;
        if (sysinfo(&sys_info) == 0) {
            info.total_physical_memory_bytes = sys_info.totalram * sys_info.mem_unit;
            info.available_memory_bytes = sys_info.freeram * sys_info.mem_unit;
        }
        
        info.page_size_bytes = sysconf(_SC_PAGESIZE);
        
        // Try to detect memory type and speed from dmidecode
        std::string dmidecode_output = execute_system_command("dmidecode -t memory 2>/dev/null");
        if (!dmidecode_output.empty()) {
            if (dmidecode_output.find("DDR5") != std::string::npos) {
                info.memory_type = "DDR5";
            } else if (dmidecode_output.find("DDR4") != std::string::npos) {
                info.memory_type = "DDR4";
            } else if (dmidecode_output.find("DDR3") != std::string::npos) {
                info.memory_type = "DDR3";
            }
        }
        
        // Check for NUMA
        if (file_exists("/sys/devices/system/node")) {
            info.numa_available = true;
            // Count NUMA nodes
            std::string ls_output = execute_system_command("ls /sys/devices/system/node/ | grep node");
            info.numa_nodes.resize(std::count(ls_output.begin(), ls_output.end(), '\n'));
        }
        
    #elif defined(ECSCOPE_PLATFORM_WINDOWS)
        MEMORYSTATUSEX memStatus;
        memStatus.dwLength = sizeof(memStatus);
        if (GlobalMemoryStatusEx(&memStatus)) {
            info.total_physical_memory_bytes = memStatus.ullTotalPhys;
            info.available_memory_bytes = memStatus.ullAvailPhys;
            info.total_virtual_memory_bytes = memStatus.ullTotalVirtual;
        }
        
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        info.page_size_bytes = sysInfo.dwPageSize;
        
    #elif defined(ECSCOPE_PLATFORM_MACOS)
        int64_t memsize;
        size_t size = sizeof(memsize);
        if (sysctlbyname("hw.memsize", &memsize, &size, nullptr, 0) == 0) {
            info.total_physical_memory_bytes = static_cast<u64>(memsize);
        }
        
        vm_size_t page_size;
        if (host_page_size(mach_host_self(), &page_size) == KERN_SUCCESS) {
            info.page_size_bytes = page_size;
        }
    #endif
    
    return info;
}

//=============================================================================
// Operating System Detection
//=============================================================================

OperatingSystemInfo HardwareDetector::detect_os_info() const {
    OperatingSystemInfo info;
    
    #ifdef ECSCOPE_PLATFORM_WINDOWS
        info.name = "Windows";
        info.is_64bit = sizeof(void*) == 8;
        
        OSVERSIONINFOA osvi;
        ZeroMemory(&osvi, sizeof(OSVERSIONINFOA));
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
        
        // Note: GetVersionEx is deprecated, but used here for educational purposes
        // Production code should use VerifyVersionInfo or other modern APIs
        #pragma warning(suppress : 4996)
        if (GetVersionExA(&osvi)) {
            std::stringstream ss;
            ss << osvi.dwMajorVersion << "." << osvi.dwMinorVersion 
               << "." << osvi.dwBuildNumber;
            info.version = ss.str();
        }
        
    #elif defined(ECSCOPE_PLATFORM_LINUX)
        info.name = "Linux";
        info.is_64bit = sizeof(void*) == 8;
        
        // Read kernel version
        struct utsname uts;
        if (uname(&uts) == 0) {
            info.kernel_version = uts.release;
            info.version = uts.version;
        }
        
        // Try to detect distribution
        if (file_exists("/etc/os-release")) {
            auto lines = read_file_lines("/etc/os-release");
            for (const auto& line : lines) {
                if (line.find("PRETTY_NAME=") == 0) {
                    info.distribution = line.substr(12);
                    // Remove quotes
                    if (!info.distribution.empty() && info.distribution.front() == '"') {
                        info.distribution = info.distribution.substr(1, info.distribution.length() - 2);
                    }
                    break;
                }
            }
        }
        
        info.supports_containers = file_exists("/proc/self/cgroup");
        info.has_realtime_scheduler = file_exists("/sys/kernel/realtime");
        
    #elif defined(ECSCOPE_PLATFORM_MACOS)
        info.name = "macOS";
        info.is_64bit = sizeof(void*) == 8;
        
        #ifdef TARGET_OS_IPHONE
        info.name = "iOS";
        #endif
        
        // Get macOS version
        std::string sw_vers = execute_system_command("sw_vers -productVersion");
        if (!sw_vers.empty()) {
            info.version = sw_vers;
            // Remove newline
            info.version.erase(std::remove(info.version.begin(), info.version.end(), '\n'), 
                              info.version.end());
        }
        
        std::string build_version = execute_system_command("sw_vers -buildVersion");
        if (!build_version.empty()) {
            build_version.erase(std::remove(build_version.begin(), build_version.end(), '\n'), 
                              build_version.end());
            info.distribution = "Build " + build_version;
        }
    #endif
    
    return info;
}

//=============================================================================
// Compiler Detection
//=============================================================================

CompilerInfo HardwareDetector::detect_compiler_info() const {
    CompilerInfo info;
    
    // Compile-time detection
    #ifdef __GNUC__
        info.name = "GCC";
        info.version = std::to_string(__GNUC__) + "." + 
                      std::to_string(__GNUC_MINOR__) + "." + 
                      std::to_string(__GNUC_PATCHLEVEL__);
        #ifdef __GNUC_MINOR__
        if (__GNUC__ >= 9) {
            info.supported_optimizations.push_back("vectorization");
            info.supports_vectorization = true;
        }
        #endif
    #elif defined(__clang__)
        info.name = "Clang";
        info.version = std::to_string(__clang_major__) + "." + 
                      std::to_string(__clang_minor__) + "." + 
                      std::to_string(__clang_patchlevel__);
        info.supports_vectorization = true;
        info.supported_optimizations.push_back("vectorization");
        info.supported_optimizations.push_back("polly");
    #elif defined(_MSC_VER)
        info.name = "MSVC";
        info.version = std::to_string(_MSC_VER);
        if (_MSC_VER >= 1920) {
            info.supports_vectorization = true;
        }
    #endif
    
    // Common optimizations
    info.supports_lto = true; // Most modern compilers support LTO
    info.available_sanitizers = {"address", "undefined", "thread"};
    
    return info;
}

//=============================================================================
// Graphics Detection (Stub - Platform Specific)
//=============================================================================

GraphicsInfo HardwareDetector::detect_graphics_info() const {
    GraphicsInfo info;
    
    // This would be implemented with platform-specific APIs
    // OpenGL/Vulkan context creation, DirectX enumeration, etc.
    
    return info;
}

//=============================================================================
// Performance Counter Detection
//=============================================================================

PerformanceCounterInfo HardwareDetector::detect_performance_counter_info() const {
    PerformanceCounterInfo info;
    
    #ifdef ECSCOPE_PLATFORM_LINUX
        // Check for perf_event support
        info.supports_hardware_counters = file_exists("/proc/sys/kernel/perf_event_paranoid");
        info.supports_software_counters = true;
        
        if (info.supports_hardware_counters) {
            info.available_counter_types = {
                "cycles", "instructions", "cache-references", "cache-misses",
                "branch-instructions", "branch-misses", "bus-cycles"
            };
            info.can_measure_cycles = true;
            info.can_measure_instructions = true;
            info.can_measure_cache_misses = true;
            info.can_measure_branch_mispredicts = true;
        }
        
    #elif defined(ECSCOPE_PLATFORM_WINDOWS)
        // Windows Performance Toolkit support
        info.supports_hardware_counters = true; // Assume available
        info.supports_software_counters = true;
        
    #elif defined(ECSCOPE_PLATFORM_MACOS)
        // macOS Instruments support
        info.supports_hardware_counters = true;
        info.supports_software_counters = true;
    #endif
    
    return info;
}

//=============================================================================
// Thermal Detection (Stub)
//=============================================================================

ThermalInfo HardwareDetector::detect_thermal_info() const {
    ThermalInfo info;
    
    // Platform-specific thermal monitoring would be implemented here
    // Reading from /sys/class/thermal on Linux
    // Using WMI on Windows
    // Using IOKit on macOS
    
    info.current_thermal_state = ThermalInfo::ThermalState::Nominal;
    
    return info;
}

//=============================================================================
// Global Instance
//=============================================================================

HardwareDetector& get_hardware_detector() {
    static HardwareDetector instance;
    return instance;
}

//=============================================================================
// Quick Detection Functions
//=============================================================================

namespace quick_detect {

CpuArchitecture get_cpu_architecture() {
    return get_hardware_detector().get_cpu_info().architecture;
}

bool has_avx2() {
    return get_hardware_detector().get_cpu_info().simd_caps.avx2;
}

bool has_avx512() {
    return get_hardware_detector().get_cpu_info().simd_caps.avx512f;
}

bool has_neon() {
    return get_hardware_detector().get_cpu_info().simd_caps.neon;
}

u32 get_physical_core_count() {
    return get_hardware_detector().get_cpu_info().topology.physical_cores;
}

u32 get_logical_core_count() {
    return get_hardware_detector().get_cpu_info().topology.logical_cores;
}

u64 get_total_memory_bytes() {
    return get_hardware_detector().get_memory_info().total_physical_memory_bytes;
}

bool is_numa_system() {
    return get_hardware_detector().get_memory_info().numa_available;
}

bool has_discrete_gpu() {
    return get_hardware_detector().get_graphics_info().has_discrete_gpu;
}

std::string get_platform_name() {
    return get_hardware_detector().get_os_info().get_platform_description();
}

} // namespace quick_detect

} // namespace ecscope::platform