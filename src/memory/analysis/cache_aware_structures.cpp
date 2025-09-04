/**
 * @file memory/cache_aware_structures.cpp
 * @brief Implementation of cache-aware data structures and analysis
 */

#include "memory/cache_aware_structures.hpp"
#include "core/log.hpp"
#include "core/profiler.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <random>

#ifdef __linux__
#include <unistd.h>
#include <sys/sysinfo.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

namespace ecscope::memory::cache {

//=============================================================================
// CacheTopologyAnalyzer Implementation
//=============================================================================

CacheTopologyAnalyzer::CacheTopologyAnalyzer() {
    discover_cache_topology();
    initialize_performance_counters();
    
    LOG_INFO("Cache topology analyzer initialized with {} cache levels", cache_hierarchy_.size());
}

CacheTopologyAnalyzer::~CacheTopologyAnalyzer() {
    // Cleanup any resources if needed
}

void CacheTopologyAnalyzer::discover_cache_topology() {
    cache_hierarchy_.clear();
    
#ifdef __linux__
    discover_linux_cache_topology();
#elif defined(_WIN32)
    discover_windows_cache_topology();
#else
    // Fallback to reasonable defaults
    discover_fallback_cache_topology();
#endif
    
    // Calculate derived metrics
    for (auto& level : cache_hierarchy_) {
        level.associativity_factor = static_cast<f64>(level.ways) / (level.size_bytes / level.line_size);
        level.sets_count = level.size_bytes / (level.line_size * level.ways);
    }
    
    // Detect false sharing characteristics
    analyze_false_sharing_characteristics();
}

std::string CacheTopologyAnalyzer::generate_topology_report() const {
    std::ostringstream report;
    
    report << "=== Cache Topology Report ===\n";
    report << "CPU Information:\n";
    report << "  Cores: " << core_count_ << "\n";
    report << "  Threads: " << thread_count_ << "\n\n";
    
    for (usize i = 0; i < cache_hierarchy_.size(); ++i) {
        const auto& level = cache_hierarchy_[i];
        report << "L" << (i + 1) << " Cache:\n";
        report << "  Size: " << (level.size_bytes / 1024) << " KB\n";
        report << "  Line Size: " << level.line_size << " bytes\n";
        report << "  Associativity: " << level.ways << "-way\n";
        report << "  Sets: " << level.sets_count << "\n";
        report << "  Latency: " << level.latency_cycles << " cycles\n";
        report << "  Bandwidth: " << std::fixed << std::setprecision(1) 
               << level.bandwidth_gbps << " GB/s\n";
        report << "  Scope: " << scope_to_string(level.scope) << "\n";
        report << "  Associativity Factor: " << std::fixed << std::setprecision(3)
               << level.associativity_factor << "\n\n";
    }
    
    report << "False Sharing Analysis:\n";
    report << "  Cache Line Size: " << false_sharing_info_.cache_line_size << " bytes\n";
    report << "  Critical Stride: " << false_sharing_info_.critical_stride << " bytes\n";
    report << "  Safe Alignment: " << false_sharing_info_.safe_alignment << " bytes\n";
    
    return report.str();
}

CacheEfficiencyReport CacheTopologyAnalyzer::analyze_access_pattern(
    const void* data, usize size, const AccessPattern& pattern) const {
    
    PROFILE_FUNCTION();
    
    CacheEfficiencyReport report{};
    report.data_size_bytes = size;
    report.pattern_type = pattern.type;
    report.analysis_timestamp = get_current_time();
    
    // Simulate cache behavior based on pattern
    switch (pattern.type) {
        case AccessType::Sequential:
            report = analyze_sequential_pattern(data, size, pattern);
            break;
        case AccessType::Random:
            report = analyze_random_pattern(data, size, pattern);
            break;
        case AccessType::Strided:
            report = analyze_strided_pattern(data, size, pattern);
            break;
        case AccessType::Hotspot:
            report = analyze_hotspot_pattern(data, size, pattern);
            break;
    }
    
    // Calculate overall efficiency score
    report.overall_efficiency_score = calculate_efficiency_score(report);
    
    return report;
}

PrefetchingRecommendations CacheTopologyAnalyzer::analyze_prefetching_opportunities(
    const AccessPattern& pattern) const {
    
    PrefetchingRecommendations recommendations{};
    
    switch (pattern.type) {
        case AccessType::Sequential:
            recommendations.hardware_prefetcher_effectiveness = 0.9f;
            recommendations.software_prefetch_benefit = 0.2f;
            recommendations.recommended_prefetch_distance = get_l1_cache().line_size * 4;
            recommendations.prefetch_strategy = "Hardware prefetcher should handle this well";
            break;
            
        case AccessType::Strided:
            if (pattern.stride_bytes <= get_l1_cache().line_size * 2) {
                recommendations.hardware_prefetcher_effectiveness = 0.7f;
                recommendations.software_prefetch_benefit = 0.4f;
            } else {
                recommendations.hardware_prefetcher_effectiveness = 0.3f;
                recommendations.software_prefetch_benefit = 0.8f;
            }
            recommendations.recommended_prefetch_distance = pattern.stride_bytes * 2;
            recommendations.prefetch_strategy = "Consider software prefetching for large strides";
            break;
            
        case AccessType::Random:
            recommendations.hardware_prefetcher_effectiveness = 0.1f;
            recommendations.software_prefetch_benefit = 0.9f;
            recommendations.recommended_prefetch_distance = get_l1_cache().size_bytes / 4;
            recommendations.prefetch_strategy = "Software prefetching essential for performance";
            break;
            
        case AccessType::Hotspot:
            recommendations.hardware_prefetcher_effectiveness = 0.5f;
            recommendations.software_prefetch_benefit = 0.6f;
            recommendations.recommended_prefetch_distance = pattern.hotspot_size;
            recommendations.prefetch_strategy = "Focus on keeping hotspot data in cache";
            break;
    }
    
    // Add specific prefetching hints
    generate_prefetching_hints(recommendations, pattern);
    
    return recommendations;
}

MemoryLayoutOptimization CacheTopologyAnalyzer::optimize_data_layout(
    usize object_size, usize object_count, AccessType access_type) const {
    
    MemoryLayoutOptimization optimization{};
    optimization.original_object_size = object_size;
    optimization.object_count = object_count;
    optimization.access_pattern = access_type;
    
    const auto& l1_cache = get_l1_cache();
    const auto& l2_cache = cache_hierarchy_.size() > 1 ? cache_hierarchy_[1] : l1_cache;
    
    // Calculate optimal alignment
    optimization.recommended_alignment = std::max(l1_cache.line_size, 
                                                 static_cast<usize>(alignof(std::max_align_t)));
    
    // Calculate optimal padding to avoid false sharing
    optimization.recommended_padding = calculate_false_sharing_padding(object_size);
    optimization.optimized_object_size = object_size + optimization.recommended_padding;
    
    // Array of structures vs Structure of arrays analysis
    optimization.aos_efficiency = calculate_aos_efficiency(object_size, access_type);
    optimization.soa_efficiency = calculate_soa_efficiency(object_size, access_type);
    optimization.recommended_layout = optimization.soa_efficiency > optimization.aos_efficiency ? 
                                    MemoryLayout::StructureOfArrays : MemoryLayout::ArrayOfStructures;
    
    // Calculate memory overhead
    optimization.memory_overhead_ratio = static_cast<f64>(optimization.optimized_object_size) / object_size;
    
    // Generate specific recommendations
    generate_layout_recommendations(optimization);
    
    return optimization;
}

// Private implementation methods

void CacheTopologyAnalyzer::discover_linux_cache_topology() {
#ifdef __linux__
    core_count_ = std::thread::hardware_concurrency();
    thread_count_ = core_count_; // Simplified - assume no SMT
    
    // Try to read cache information from /sys/devices/system/cpu/cpu0/cache/
    for (int level = 0; level < 4; ++level) {
        CacheLevel cache_level{};
        
        std::string base_path = "/sys/devices/system/cpu/cpu0/cache/index" + std::to_string(level);
        
        // Read cache size
        std::ifstream size_file(base_path + "/size");
        std::string size_str;
        if (size_file >> size_str) {
            cache_level.size_bytes = parse_cache_size(size_str);
        } else {
            break; // No more cache levels
        }
        
        // Read cache line size
        std::ifstream line_size_file(base_path + "/coherency_line_size");
        if (!(line_size_file >> cache_level.line_size)) {
            cache_level.line_size = 64; // Default assumption
        }
        
        // Read associativity
        std::ifstream ways_file(base_path + "/ways_of_associativity");
        if (!(ways_file >> cache_level.ways)) {
            cache_level.ways = 8; // Default assumption
        }
        
        // Read cache type
        std::ifstream type_file(base_path + "/type");
        std::string type_str;
        if (type_file >> type_str) {
            if (type_str == "Data") {
                cache_level.type = CacheType::Data;
            } else if (type_str == "Instruction") {
                cache_level.type = CacheType::Instruction;
            } else {
                cache_level.type = CacheType::Unified;
            }
        }
        
        // Set estimated performance characteristics
        cache_level.latency_cycles = (level + 1) * (level + 1) * 3; // Rough estimate
        cache_level.bandwidth_gbps = 100.0 / std::pow(2, level); // Rough estimate
        cache_level.scope = level == 0 ? CacheScope::Core : 
                          (level == 1 ? CacheScope::Core : CacheScope::Package);
        
        cache_hierarchy_.push_back(cache_level);
    }
    
    // If we couldn't read from sysfs, use fallback
    if (cache_hierarchy_.empty()) {
        discover_fallback_cache_topology();
    }
#endif
}

void CacheTopologyAnalyzer::discover_windows_cache_topology() {
#ifdef _WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    
    core_count_ = sysinfo.dwNumberOfProcessors;
    thread_count_ = core_count_;
    
    // Use fallback topology for Windows (would need more complex WMI queries for real detection)
    discover_fallback_cache_topology();
#endif
}

void CacheTopologyAnalyzer::discover_fallback_cache_topology() {
    // Modern x86-64 typical cache hierarchy
    
    // L1 Data Cache
    CacheLevel l1d{};
    l1d.size_bytes = 32 * 1024; // 32KB
    l1d.line_size = 64;
    l1d.ways = 8;
    l1d.type = CacheType::Data;
    l1d.scope = CacheScope::Core;
    l1d.latency_cycles = 4;
    l1d.bandwidth_gbps = 100.0;
    cache_hierarchy_.push_back(l1d);
    
    // L2 Cache
    CacheLevel l2{};
    l2.size_bytes = 256 * 1024; // 256KB
    l2.line_size = 64;
    l2.ways = 8;
    l2.type = CacheType::Unified;
    l2.scope = CacheScope::Core;
    l2.latency_cycles = 12;
    l2.bandwidth_gbps = 50.0;
    cache_hierarchy_.push_back(l2);
    
    // L3 Cache
    CacheLevel l3{};
    l3.size_bytes = 8 * 1024 * 1024; // 8MB
    l3.line_size = 64;
    l3.ways = 16;
    l3.type = CacheType::Unified;
    l3.scope = CacheScope::Package;
    l3.latency_cycles = 40;
    l3.bandwidth_gbps = 25.0;
    cache_hierarchy_.push_back(l3);
    
    core_count_ = std::thread::hardware_concurrency();
    thread_count_ = core_count_;
}

void CacheTopologyAnalyzer::initialize_performance_counters() {
    // Initialize performance measurement infrastructure
    // This is simplified - real implementation would use hardware performance counters
    
    total_cache_accesses_.store(0);
    total_cache_misses_.store(0);
    total_prefetch_requests_.store(0);
}

void CacheTopologyAnalyzer::analyze_false_sharing_characteristics() {
    if (cache_hierarchy_.empty()) return;
    
    const auto& l1_cache = cache_hierarchy_[0];
    
    false_sharing_info_.cache_line_size = l1_cache.line_size;
    false_sharing_info_.critical_stride = l1_cache.line_size;
    false_sharing_info_.safe_alignment = l1_cache.line_size * 2; // Conservative
    false_sharing_info_.conflict_window_bytes = l1_cache.line_size;
    
    // Calculate false sharing probability based on cache parameters
    false_sharing_info_.false_sharing_probability = 
        1.0 - std::exp(-2.0 * static_cast<f64>(thread_count_) / l1_cache.ways);
}

CacheEfficiencyReport CacheTopologyAnalyzer::analyze_sequential_pattern(
    const void* data, usize size, const AccessPattern& pattern) const {
    
    CacheEfficiencyReport report{};
    report.pattern_type = AccessType::Sequential;
    
    const auto& l1_cache = get_l1_cache();
    
    // Sequential access is very cache-friendly
    usize cache_lines_needed = (size + l1_cache.line_size - 1) / l1_cache.line_size;
    usize working_set_size = std::min(size, l1_cache.size_bytes);
    
    // Estimate cache behavior
    if (size <= l1_cache.size_bytes) {
        // Fits in L1 cache
        report.estimated_l1_hit_rate = 0.95f; // High hit rate after warmup
        report.estimated_l2_hit_rate = 1.0f;
        report.estimated_l3_hit_rate = 1.0f;
        report.predicted_performance_factor = 1.0f;
    } else if (cache_hierarchy_.size() > 1 && size <= cache_hierarchy_[1].size_bytes) {
        // Fits in L2 cache
        report.estimated_l1_hit_rate = 0.3f;
        report.estimated_l2_hit_rate = 0.95f;
        report.estimated_l3_hit_rate = 1.0f;
        report.predicted_performance_factor = 0.7f;
    } else {
        // Doesn't fit in cache - streaming scenario
        report.estimated_l1_hit_rate = 0.1f;
        report.estimated_l2_hit_rate = 0.2f;
        report.estimated_l3_hit_rate = 0.5f;
        report.predicted_performance_factor = 0.3f;
    }
    
    report.bandwidth_efficiency = 0.9f; // Sequential is bandwidth-efficient
    report.prefetch_effectiveness = 0.95f; // Hardware prefetcher works great
    
    // Generate specific recommendations
    report.optimization_suggestions.push_back("Sequential access detected - hardware prefetcher should be effective");
    if (size > l1_cache.size_bytes) {
        report.optimization_suggestions.push_back("Consider data blocking/tiling to improve cache locality");
    }
    
    return report;
}

CacheEfficiencyReport CacheTopologyAnalyzer::analyze_random_pattern(
    const void* data, usize size, const AccessPattern& pattern) const {
    
    CacheEfficiencyReport report{};
    report.pattern_type = AccessType::Random;
    
    const auto& l1_cache = get_l1_cache();
    
    // Random access is very cache-unfriendly
    usize unique_cache_lines = (size + l1_cache.line_size - 1) / l1_cache.line_size;
    
    // Estimate cache behavior based on working set vs cache size
    f64 working_set_ratio = static_cast<f64>(size) / l1_cache.size_bytes;
    
    if (working_set_ratio <= 0.5) {
        // Small working set - some cache hits possible
        report.estimated_l1_hit_rate = 0.6f;
        report.estimated_l2_hit_rate = 0.8f;
        report.estimated_l3_hit_rate = 0.9f;
        report.predicted_performance_factor = 0.4f;
    } else if (working_set_ratio <= 2.0) {
        // Medium working set - cache thrashing begins
        report.estimated_l1_hit_rate = 0.3f;
        report.estimated_l2_hit_rate = 0.5f;
        report.estimated_l3_hit_rate = 0.7f;
        report.predicted_performance_factor = 0.2f;
    } else {
        // Large working set - severe cache thrashing
        report.estimated_l1_hit_rate = 0.1f;
        report.estimated_l2_hit_rate = 0.2f;
        report.estimated_l3_hit_rate = 0.3f;
        report.predicted_performance_factor = 0.1f;
    }
    
    report.bandwidth_efficiency = 0.2f; // Random access is bandwidth-inefficient
    report.prefetch_effectiveness = 0.1f; // Hardware prefetcher can't help
    
    // Generate optimization suggestions
    report.optimization_suggestions.push_back("Random access pattern detected - consider:");
    report.optimization_suggestions.push_back("  - Spatial data structures (locality-preserving)");
    report.optimization_suggestions.push_back("  - Software prefetching");
    report.optimization_suggestions.push_back("  - Cache-oblivious algorithms");
    
    return report;
}

CacheEfficiencyReport CacheTopologyAnalyzer::analyze_strided_pattern(
    const void* data, usize size, const AccessPattern& pattern) const {
    
    CacheEfficiencyReport report{};
    report.pattern_type = AccessType::Strided;
    
    const auto& l1_cache = get_l1_cache();
    
    // Analyze stride characteristics
    bool cache_line_friendly = (pattern.stride_bytes % l1_cache.line_size) == 0;
    bool power_of_two_stride = (pattern.stride_bytes & (pattern.stride_bytes - 1)) == 0;
    f64 stride_ratio = static_cast<f64>(pattern.stride_bytes) / l1_cache.line_size;
    
    if (pattern.stride_bytes <= l1_cache.line_size) {
        // Small stride - good locality
        report.estimated_l1_hit_rate = 0.8f;
        report.estimated_l2_hit_rate = 0.9f;
        report.estimated_l3_hit_rate = 0.95f;
        report.predicted_performance_factor = 0.8f;
        report.prefetch_effectiveness = 0.8f;
    } else if (pattern.stride_bytes <= l1_cache.line_size * 4) {
        // Medium stride - moderate locality
        report.estimated_l1_hit_rate = 0.5f;
        report.estimated_l2_hit_rate = 0.7f;
        report.estimated_l3_hit_rate = 0.8f;
        report.predicted_performance_factor = 0.5f;
        report.prefetch_effectiveness = 0.6f;
    } else {
        // Large stride - poor locality
        report.estimated_l1_hit_rate = 0.2f;
        report.estimated_l2_hit_rate = 0.4f;
        report.estimated_l3_hit_rate = 0.6f;
        report.predicted_performance_factor = 0.3f;
        report.prefetch_effectiveness = 0.3f;
    }
    
    report.bandwidth_efficiency = std::max(0.1f, 1.0f / static_cast<f32>(stride_ratio));
    
    // Generate optimization suggestions
    report.optimization_suggestions.push_back("Strided access pattern detected:");
    report.optimization_suggestions.push_back("  Stride: " + std::to_string(pattern.stride_bytes) + " bytes");
    
    if (stride_ratio > 4.0) {
        report.optimization_suggestions.push_back("  - Consider data reorganization to reduce stride");
        report.optimization_suggestions.push_back("  - Software prefetching may help");
    }
    
    if (!cache_line_friendly) {
        report.optimization_suggestions.push_back("  - Align stride to cache line boundaries");
    }
    
    if (power_of_two_stride && stride_ratio > 1.0) {
        report.optimization_suggestions.push_back("  - Watch for cache set conflicts (power-of-2 stride)");
    }
    
    return report;
}

CacheEfficiencyReport CacheTopologyAnalyzer::analyze_hotspot_pattern(
    const void* data, usize size, const AccessPattern& pattern) const {
    
    CacheEfficiencyReport report{};
    report.pattern_type = AccessType::Hotspot;
    
    const auto& l1_cache = get_l1_cache();
    
    // Analyze hotspot characteristics
    f64 hotspot_ratio = static_cast<f64>(pattern.hotspot_size) / size;
    bool hotspot_fits_l1 = pattern.hotspot_size <= l1_cache.size_bytes;
    
    if (hotspot_fits_l1 && hotspot_ratio < 0.2) {
        // Small hotspot that fits in L1
        report.estimated_l1_hit_rate = 0.9f;
        report.estimated_l2_hit_rate = 0.95f;
        report.estimated_l3_hit_rate = 1.0f;
        report.predicted_performance_factor = 0.9f;
        report.prefetch_effectiveness = 0.7f;
    } else if (cache_hierarchy_.size() > 1 && 
               pattern.hotspot_size <= cache_hierarchy_[1].size_bytes) {
        // Hotspot fits in L2
        report.estimated_l1_hit_rate = 0.6f;
        report.estimated_l2_hit_rate = 0.9f;
        report.estimated_l3_hit_rate = 0.95f;
        report.predicted_performance_factor = 0.7f;
        report.prefetch_effectiveness = 0.5f;
    } else {
        // Large hotspot - cache pressure
        report.estimated_l1_hit_rate = 0.4f;
        report.estimated_l2_hit_rate = 0.6f;
        report.estimated_l3_hit_rate = 0.8f;
        report.predicted_performance_factor = 0.5f;
        report.prefetch_effectiveness = 0.3f;
    }
    
    report.bandwidth_efficiency = std::min(1.0f, 2.0f * static_cast<f32>(hotspot_ratio));
    
    // Generate optimization suggestions
    report.optimization_suggestions.push_back("Hotspot access pattern detected:");
    report.optimization_suggestions.push_back("  Hotspot size: " + std::to_string(pattern.hotspot_size) + " bytes");
    report.optimization_suggestions.push_back("  Hotspot ratio: " + std::to_string(hotspot_ratio * 100) + "%");
    
    if (!hotspot_fits_l1) {
        report.optimization_suggestions.push_back("  - Consider splitting hotspot data");
        report.optimization_suggestions.push_back("  - Use hot/cold data separation");
    }
    
    return report;
}

f64 CacheTopologyAnalyzer::calculate_efficiency_score(const CacheEfficiencyReport& report) const {
    // Weighted average of different efficiency metrics
    f64 cache_score = (report.estimated_l1_hit_rate * 0.5f + 
                      report.estimated_l2_hit_rate * 0.3f + 
                      report.estimated_l3_hit_rate * 0.2f);
    
    f64 overall_score = (cache_score * 0.6 + 
                        report.bandwidth_efficiency * 0.3 + 
                        report.prefetch_effectiveness * 0.1);
    
    return std::clamp(overall_score, 0.0, 1.0);
}

void CacheTopologyAnalyzer::generate_prefetching_hints(
    PrefetchingRecommendations& recommendations, const AccessPattern& pattern) const {
    
    switch (pattern.type) {
        case AccessType::Sequential:
            recommendations.specific_hints.push_back("Use __builtin_prefetch() sparingly - hardware handles this");
            recommendations.specific_hints.push_back("Ensure data alignment to cache line boundaries");
            break;
            
        case AccessType::Strided:
            recommendations.specific_hints.push_back("Use __builtin_prefetch() with appropriate distance");
            recommendations.specific_hints.push_back("Consider: __builtin_prefetch(ptr + " + 
                                                    std::to_string(recommendations.recommended_prefetch_distance) + 
                                                    ", 0, 3)");
            break;
            
        case AccessType::Random:
            recommendations.specific_hints.push_back("Group prefetches to minimize instruction overhead");
            recommendations.specific_hints.push_back("Use temporal locality hints: __builtin_prefetch(ptr, 0, 1)");
            break;
            
        case AccessType::Hotspot:
            recommendations.specific_hints.push_back("Prefetch hotspot data at initialization");
            recommendations.specific_hints.push_back("Use high temporal locality: __builtin_prefetch(ptr, 0, 3)");
            break;
    }
}

void CacheTopologyAnalyzer::generate_layout_recommendations(
    MemoryLayoutOptimization& optimization) const {
    
    optimization.specific_recommendations.push_back(
        "Recommended alignment: " + std::to_string(optimization.recommended_alignment) + " bytes");
    
    optimization.specific_recommendations.push_back(
        "Recommended padding: " + std::to_string(optimization.recommended_padding) + " bytes");
    
    if (optimization.recommended_layout == MemoryLayout::StructureOfArrays) {
        optimization.specific_recommendations.push_back(
            "Structure of Arrays (SoA) recommended for better cache utilization");
        optimization.specific_recommendations.push_back(
            "Split frequently accessed fields into separate arrays");
    } else {
        optimization.specific_recommendations.push_back(
            "Array of Structures (AoS) acceptable for this access pattern");
        optimization.specific_recommendations.push_back(
            "Ensure proper struct packing and alignment");
    }
    
    if (optimization.memory_overhead_ratio > 1.5) {
        optimization.specific_recommendations.push_back(
            "WARNING: High memory overhead (" + 
            std::to_string(optimization.memory_overhead_ratio * 100 - 100) + 
            "%) - consider data compression");
    }
}

usize CacheTopologyAnalyzer::calculate_false_sharing_padding(usize object_size) const {
    if (cache_hierarchy_.empty()) return 64; // Fallback
    
    const auto& l1_cache = cache_hierarchy_[0];
    
    // Calculate padding needed to avoid false sharing
    usize remainder = object_size % l1_cache.line_size;
    if (remainder == 0) return 0;
    
    return l1_cache.line_size - remainder;
}

f64 CacheTopologyAnalyzer::calculate_aos_efficiency(usize object_size, AccessType access_type) const {
    // Array of Structures efficiency depends on access pattern
    switch (access_type) {
        case AccessType::Sequential:
            return 0.8; // Good for processing whole objects
        case AccessType::Random:
            return 0.6; // Moderate - depends on object size vs cache line
        case AccessType::Strided:
            return 0.4; // Poor - may access unneeded fields
        case AccessType::Hotspot:
            return 0.7; // Good if hotspot objects accessed completely
        default:
            return 0.5;
    }
}

f64 CacheTopologyAnalyzer::calculate_soa_efficiency(usize object_size, AccessType access_type) const {
    // Structure of Arrays efficiency
    switch (access_type) {
        case AccessType::Sequential:
            return 0.9; // Excellent for field-wise processing
        case AccessType::Random:
            return 0.4; // Poor - loses spatial locality
        case AccessType::Strided:
            return 0.8; // Good - can process fields independently
        case AccessType::Hotspot:
            return 0.5; // Depends on which fields are hot
        default:
            return 0.6;
    }
}

usize CacheTopologyAnalyzer::parse_cache_size(const std::string& size_str) const {
    usize multiplier = 1;
    std::string number_part = size_str;
    
    if (size_str.back() == 'K' || size_str.back() == 'k') {
        multiplier = 1024;
        number_part = size_str.substr(0, size_str.length() - 1);
    } else if (size_str.back() == 'M' || size_str.back() == 'm') {
        multiplier = 1024 * 1024;
        number_part = size_str.substr(0, size_str.length() - 1);
    } else if (size_str.back() == 'G' || size_str.back() == 'g') {
        multiplier = 1024 * 1024 * 1024;
        number_part = size_str.substr(0, size_str.length() - 1);
    }
    
    try {
        return std::stoul(number_part) * multiplier;
    } catch (const std::exception&) {
        return 0;
    }
}

const CacheLevel& CacheTopologyAnalyzer::get_l1_cache() const {
    if (cache_hierarchy_.empty()) {
        static const CacheLevel fallback{32*1024, 64, 8, CacheType::Data, CacheScope::Core, 4, 100.0, 0.8, 512};
        return fallback;
    }
    return cache_hierarchy_[0];
}

std::string CacheTopologyAnalyzer::scope_to_string(CacheScope scope) const {
    switch (scope) {
        case CacheScope::Core: return "Per-Core";
        case CacheScope::Package: return "Per-Package";
        case CacheScope::System: return "System-Wide";
        default: return "Unknown";
    }
}

f64 CacheTopologyAnalyzer::get_current_time() const {
    using namespace std::chrono;
    return duration<f64>(steady_clock::now().time_since_epoch()).count();
}

//=============================================================================
// Global Cache Analyzer Instance
//=============================================================================

CacheTopologyAnalyzer& get_global_cache_analyzer() {
    static CacheTopologyAnalyzer instance;
    return instance;
}

} // namespace ecscope::memory::cache