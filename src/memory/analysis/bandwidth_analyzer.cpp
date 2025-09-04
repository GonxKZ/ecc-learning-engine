/**
 * @file memory/bandwidth_analyzer.cpp
 * @brief Implementation of memory bandwidth analysis and bottleneck detection
 */

#include "bandwidth_analyzer.hpp"
#include "core/log.hpp"
#include "core/profiler.hpp"
#include <algorithm>
#include <numeric>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <random>

#ifdef __linux__
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace ecscope::memory::bandwidth {

//=============================================================================
// MemoryAccessPattern Implementation
//=============================================================================

MemoryAccessPattern::MemoryAccessPattern(AccessType type, usize size, usize stride, f64 intensity)
    : access_type(type), data_size(size), stride_bytes(stride), access_intensity(intensity) {
    
    pattern_name = generate_pattern_name();
    expected_efficiency = calculate_expected_efficiency();
}

std::string MemoryAccessPattern::generate_pattern_name() const {
    std::ostringstream name;
    
    switch (access_type) {
        case AccessType::Sequential:
            name << "Sequential";
            break;
        case AccessType::Random:
            name << "Random";
            break;
        case AccessType::Strided:
            name << "Strided(" << stride_bytes << ")";
            break;
        case AccessType::Pointer_Chasing:
            name << "PointerChasing";
            break;
    }
    
    if (data_size >= 1024*1024*1024) {
        name << "_" << (data_size / (1024*1024*1024)) << "GB";
    } else if (data_size >= 1024*1024) {
        name << "_" << (data_size / (1024*1024)) << "MB";
    } else if (data_size >= 1024) {
        name << "_" << (data_size / 1024) << "KB";
    } else {
        name << "_" << data_size << "B";
    }
    
    return name.str();
}

f64 MemoryAccessPattern::calculate_expected_efficiency() const {
    // Simplified efficiency estimation based on access pattern
    switch (access_type) {
        case AccessType::Sequential:
            return 0.95; // Very cache-friendly
        case AccessType::Strided:
            if (stride_bytes <= 64) return 0.8; // Cache line friendly
            if (stride_bytes <= 4096) return 0.6; // Page friendly
            return 0.3; // Poor cache locality
        case AccessType::Random:
            return 0.2; // Poor cache locality
        case AccessType::Pointer_Chasing:
            return 0.1; // Worst case
        default:
            return 0.5;
    }
}

//=============================================================================
// BandwidthMeasurement Implementation
//=============================================================================

f64 BandwidthMeasurement::get_effective_bandwidth() const {
    return bytes_per_second / (1024.0 * 1024.0 * 1024.0); // Convert to GB/s
}

f64 BandwidthMeasurement::get_cache_efficiency() const {
    if (theoretical_peak_gbps > 0.0) {
        return get_effective_bandwidth() / theoretical_peak_gbps;
    }
    return 0.0;
}

f64 BandwidthMeasurement::get_latency_impact() const {
    // Higher latency reduces effective bandwidth
    return average_latency_ns > 0.0 ? 1000.0 / average_latency_ns : 1.0;
}

//=============================================================================
// MemoryBandwidthProfiler Implementation
//=============================================================================

MemoryBandwidthProfiler::MemoryBandwidthProfiler(numa::NumaManager& numa_mgr)
    : numa_manager_(numa_mgr), profiling_enabled_(true) {
    
    // Initialize with default patterns
    initialize_default_patterns();
    
    // Start background profiling thread
    profiling_thread_ = std::thread([this]() {
        background_profiling_worker();
    });
    
    LOG_INFO("Memory bandwidth profiler initialized with {} test patterns", test_patterns_.size());
}

MemoryBandwidthProfiler::~MemoryBandwidthProfiler() {
    shutdown_requested_.store(true);
    if (profiling_thread_.joinable()) {
        profiling_thread_.join();
    }
}

BandwidthMeasurement MemoryBandwidthProfiler::measure_bandwidth(
    const MemoryAccessPattern& pattern, u32 numa_node) {
    
    PROFILE_FUNCTION();
    
    BandwidthMeasurement measurement{};
    measurement.pattern_name = pattern.pattern_name;
    measurement.numa_node = numa_node;
    measurement.data_size_bytes = pattern.data_size;
    measurement.timestamp = get_current_time();
    
    // Allocate test buffer on specific NUMA node
    void* buffer = numa_manager_.allocate_on_node(pattern.data_size, numa_node);
    if (!buffer) {
        LOG_ERROR("Failed to allocate {} bytes on NUMA node {} for bandwidth test", 
                 pattern.data_size, numa_node);
        return measurement;
    }
    
    // Initialize buffer with test pattern
    initialize_test_buffer(buffer, pattern.data_size, pattern.access_type);
    
    // Warm up the memory
    warmup_memory(buffer, pattern);
    
    // Run the actual bandwidth test
    auto start_time = std::chrono::high_resolution_clock::now();
    
    usize total_bytes = execute_access_pattern(buffer, pattern);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    
    // Calculate results
    auto duration = std::chrono::duration<f64>(end_time - start_time).count();
    if (duration > 0.0) {
        measurement.bytes_per_second = static_cast<f64>(total_bytes) / duration;
        measurement.test_duration_seconds = duration;
    }
    
    // Measure latency for random access patterns
    if (pattern.access_type == MemoryAccessPattern::AccessType::Random ||
        pattern.access_type == MemoryAccessPattern::AccessType::Pointer_Chasing) {
        measurement.average_latency_ns = measure_access_latency(buffer, pattern);
    }
    
    // Cleanup
    numa_manager_.deallocate(buffer, pattern.data_size);
    
    // Record the measurement
    record_measurement(measurement);
    
    LOG_DEBUG("Bandwidth test '{}' on node {}: {:.2f} GB/s", 
             pattern.pattern_name, numa_node, measurement.get_effective_bandwidth());
    
    return measurement;
}

std::vector<BandwidthMeasurement> MemoryBandwidthProfiler::profile_all_patterns(u32 numa_node) {
    std::vector<BandwidthMeasurement> results;
    results.reserve(test_patterns_.size());
    
    LOG_INFO("Running comprehensive bandwidth profiling on NUMA node {}...", numa_node);
    
    for (const auto& pattern : test_patterns_) {
        auto measurement = measure_bandwidth(pattern, numa_node);
        results.push_back(measurement);
    }
    
    return results;
}

std::vector<BandwidthMeasurement> MemoryBandwidthProfiler::profile_numa_comparison() {
    std::vector<BandwidthMeasurement> results;
    auto available_nodes = numa_manager_.get_topology().get_available_nodes();
    
    LOG_INFO("Running NUMA bandwidth comparison across {} nodes...", available_nodes.size());
    
    // Test sequential access pattern across all nodes
    MemoryAccessPattern sequential_pattern(
        MemoryAccessPattern::AccessType::Sequential, 
        100 * 1024 * 1024, // 100MB
        64, 
        1.0
    );
    
    for (u32 node : available_nodes) {
        auto measurement = measure_bandwidth(sequential_pattern, node);
        results.push_back(measurement);
    }
    
    return results;
}

BandwidthProfileReport MemoryBandwidthProfiler::generate_comprehensive_report() const {
    BandwidthProfileReport report{};
    
    std::shared_lock<std::shared_mutex> lock(measurements_mutex_);
    
    if (measurements_.empty()) {
        report.summary = "No bandwidth measurements available";
        return report;
    }
    
    // Calculate statistics
    report.total_measurements = measurements_.size();
    
    f64 total_bandwidth = 0.0;
    f64 max_bandwidth = 0.0;
    f64 min_bandwidth = std::numeric_limits<f64>::max();
    
    for (const auto& measurement : measurements_) {
        f64 bandwidth = measurement.get_effective_bandwidth();
        total_bandwidth += bandwidth;
        max_bandwidth = std::max(max_bandwidth, bandwidth);
        min_bandwidth = std::min(min_bandwidth, bandwidth);
    }
    
    report.average_bandwidth_gbps = total_bandwidth / measurements_.size();
    report.peak_bandwidth_gbps = max_bandwidth;
    report.min_bandwidth_gbps = min_bandwidth;
    
    // Group measurements by pattern
    std::unordered_map<std::string, std::vector<BandwidthMeasurement>> by_pattern;
    std::unordered_map<u32, std::vector<BandwidthMeasurement>> by_node;
    
    for (const auto& measurement : measurements_) {
        by_pattern[measurement.pattern_name].push_back(measurement);
        by_node[measurement.numa_node].push_back(measurement);
    }
    
    // Generate per-pattern analysis
    for (const auto& [pattern_name, measurements] : by_pattern) {
        PatternAnalysis analysis{};
        analysis.pattern_name = pattern_name;
        analysis.measurement_count = measurements.size();
        
        f64 pattern_total = 0.0;
        for (const auto& m : measurements) {
            pattern_total += m.get_effective_bandwidth();
        }
        analysis.average_bandwidth_gbps = pattern_total / measurements.size();
        
        report.pattern_analysis[pattern_name] = analysis;
    }
    
    // Generate per-node analysis
    for (const auto& [node_id, measurements] : by_node) {
        NodeAnalysis analysis{};
        analysis.node_id = node_id;
        analysis.measurement_count = measurements.size();
        
        f64 node_total = 0.0;
        for (const auto& m : measurements) {
            node_total += m.get_effective_bandwidth();
        }
        analysis.average_bandwidth_gbps = node_total / measurements.size();
        
        report.node_analysis[node_id] = analysis;
    }
    
    // Generate summary
    std::ostringstream summary;
    summary << "Bandwidth Profile Summary:\n";
    summary << "  Total Measurements: " << report.total_measurements << "\n";
    summary << "  Average Bandwidth: " << std::fixed << std::setprecision(2) 
            << report.average_bandwidth_gbps << " GB/s\n";
    summary << "  Peak Bandwidth: " << report.peak_bandwidth_gbps << " GB/s\n";
    summary << "  Min Bandwidth: " << report.min_bandwidth_gbps << " GB/s\n";
    summary << "  Pattern Count: " << by_pattern.size() << "\n";
    summary << "  Node Count: " << by_node.size() << "\n";
    
    report.summary = summary.str();
    
    return report;
}

// Private implementation methods

void MemoryBandwidthProfiler::initialize_default_patterns() {
    test_patterns_.clear();
    
    // Sequential access patterns (different sizes)
    test_patterns_.emplace_back(MemoryAccessPattern::AccessType::Sequential, 1024*1024, 8, 1.0);      // 1MB
    test_patterns_.emplace_back(MemoryAccessPattern::AccessType::Sequential, 10*1024*1024, 8, 1.0);   // 10MB
    test_patterns_.emplace_back(MemoryAccessPattern::AccessType::Sequential, 100*1024*1024, 8, 1.0);  // 100MB
    
    // Strided access patterns
    test_patterns_.emplace_back(MemoryAccessPattern::AccessType::Strided, 10*1024*1024, 64, 1.0);     // Cache line stride
    test_patterns_.emplace_back(MemoryAccessPattern::AccessType::Strided, 10*1024*1024, 1024, 1.0);   // 1KB stride
    test_patterns_.emplace_back(MemoryAccessPattern::AccessType::Strided, 10*1024*1024, 4096, 1.0);   // Page stride
    
    // Random access patterns
    test_patterns_.emplace_back(MemoryAccessPattern::AccessType::Random, 10*1024*1024, 0, 1.0);       // Random within 10MB
    test_patterns_.emplace_back(MemoryAccessPattern::AccessType::Random, 100*1024*1024, 0, 1.0);      // Random within 100MB
    
    // Pointer chasing (worst case latency)
    test_patterns_.emplace_back(MemoryAccessPattern::AccessType::Pointer_Chasing, 1024*1024, 0, 0.5); // 1MB pointer chain
}

void MemoryBandwidthProfiler::initialize_test_buffer(void* buffer, usize size, 
                                                   MemoryAccessPattern::AccessType access_type) {
    byte* ptr = static_cast<byte*>(buffer);
    
    switch (access_type) {
        case MemoryAccessPattern::AccessType::Sequential:
        case MemoryAccessPattern::AccessType::Strided:
        case MemoryAccessPattern::AccessType::Random:
            // Initialize with predictable pattern
            for (usize i = 0; i < size; ++i) {
                ptr[i] = static_cast<byte>(i & 0xFF);
            }
            break;
            
        case MemoryAccessPattern::AccessType::Pointer_Chasing: {
            // Create a linked list through the buffer
            usize* indices = reinterpret_cast<usize*>(ptr);
            usize node_count = size / sizeof(usize);
            
            // Create random permutation for pointer chasing
            std::vector<usize> permutation(node_count);
            std::iota(permutation.begin(), permutation.end(), 0);
            std::shuffle(permutation.begin(), permutation.end(), std::mt19937{42});
            
            for (usize i = 0; i < node_count - 1; ++i) {
                indices[permutation[i]] = permutation[i + 1];
            }
            indices[permutation[node_count - 1]] = permutation[0]; // Close the loop
            break;
        }
    }
}

void MemoryBandwidthProfiler::warmup_memory(void* buffer, const MemoryAccessPattern& pattern) {
    // Warm up memory by doing a quick pass
    volatile byte* ptr = static_cast<byte*>(buffer);
    for (usize i = 0; i < pattern.data_size; i += 4096) { // Page-sized strides
        volatile byte temp = ptr[i];
        (void)temp; // Suppress unused variable warning
    }
}

usize MemoryBandwidthProfiler::execute_access_pattern(void* buffer, const MemoryAccessPattern& pattern) {
    usize total_bytes = 0;
    volatile byte* ptr = static_cast<byte*>(buffer);
    
    switch (pattern.access_type) {
        case MemoryAccessPattern::AccessType::Sequential: {
            // Sequential read/write pass
            for (usize pass = 0; pass < 3; ++pass) { // Multiple passes for better measurement
                for (usize i = 0; i < pattern.data_size; i += pattern.stride_bytes) {
                    volatile byte temp = ptr[i];
                    ptr[i] = static_cast<byte>(temp + 1);
                    total_bytes += pattern.stride_bytes * 2; // Read + Write
                }
            }
            break;
        }
        
        case MemoryAccessPattern::AccessType::Strided: {
            // Strided access pattern
            for (usize pass = 0; pass < 5; ++pass) {
                for (usize i = 0; i < pattern.data_size; i += pattern.stride_bytes) {
                    volatile byte temp = ptr[i];
                    total_bytes += pattern.stride_bytes;
                    (void)temp;
                }
            }
            break;
        }
        
        case MemoryAccessPattern::AccessType::Random: {
            // Random access pattern
            std::mt19937 rng(42); // Fixed seed for reproducibility
            std::uniform_int_distribution<usize> dist(0, (pattern.data_size / 64) - 1);
            
            for (usize access = 0; access < 10000; ++access) {
                usize index = dist(rng) * 64; // Align to cache lines
                volatile byte temp = ptr[index];
                total_bytes += 64;
                (void)temp;
            }
            break;
        }
        
        case MemoryAccessPattern::AccessType::Pointer_Chasing: {
            // Pointer chasing through linked list
            volatile usize* indices = reinterpret_cast<volatile usize*>(ptr);
            usize node_count = pattern.data_size / sizeof(usize);
            
            volatile usize current = 0;
            for (usize step = 0; step < node_count * 10; ++step) { // Multiple rounds
                current = indices[current];
                total_bytes += sizeof(usize);
            }
            break;
        }
    }
    
    return total_bytes;
}

f64 MemoryBandwidthProfiler::measure_access_latency(void* buffer, const MemoryAccessPattern& pattern) {
    volatile byte* ptr = static_cast<byte*>(buffer);
    constexpr usize num_accesses = 1000;
    
    std::mt19937 rng(42);
    std::uniform_int_distribution<usize> dist(0, (pattern.data_size / 64) - 1);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (usize i = 0; i < num_accesses; ++i) {
        usize index = dist(rng) * 64;
        volatile byte temp = ptr[index];
        (void)temp;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration_ns = std::chrono::duration<f64, std::nano>(end_time - start_time).count();
    
    return duration_ns / num_accesses;
}

void MemoryBandwidthProfiler::record_measurement(const BandwidthMeasurement& measurement) {
    std::unique_lock<std::shared_mutex> lock(measurements_mutex_);
    measurements_.push_back(measurement);
    
    // Keep only recent measurements to prevent unbounded growth
    constexpr usize MAX_MEASUREMENTS = 10000;
    if (measurements_.size() > MAX_MEASUREMENTS) {
        measurements_.erase(measurements_.begin(), measurements_.begin() + (measurements_.size() - MAX_MEASUREMENTS));
    }
    
    total_measurements_recorded_.fetch_add(1, std::memory_order_relaxed);
}

void MemoryBandwidthProfiler::background_profiling_worker() {
    LOG_DEBUG("Background bandwidth profiling worker started");
    
    while (!shutdown_requested_.load() && profiling_enabled_.load()) {
        // Run periodic bandwidth profiling
        auto available_nodes = numa_manager_.get_topology().get_available_nodes();
        
        if (!available_nodes.empty()) {
            // Test one node per iteration to spread the load
            static std::atomic<usize> node_index{0};
            u32 target_node = available_nodes[node_index.fetch_add(1) % available_nodes.size()];
            
            // Run a lightweight sequential test
            MemoryAccessPattern light_pattern(
                MemoryAccessPattern::AccessType::Sequential,
                10 * 1024 * 1024, // 10MB
                64,
                1.0
            );
            
            measure_bandwidth(light_pattern, target_node);
        }
        
        // Sleep between profiling runs
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
    
    LOG_DEBUG("Background bandwidth profiling worker stopped");
}

f64 MemoryBandwidthProfiler::get_current_time() const {
    using namespace std::chrono;
    return duration<f64>(steady_clock::now().time_since_epoch()).count();
}

//=============================================================================
// MemoryBottleneckDetector Implementation  
//=============================================================================

MemoryBottleneckDetector::MemoryBottleneckDetector(MemoryBandwidthProfiler& profiler,
                                                 numa::NumaManager& numa_mgr)
    : bandwidth_profiler_(profiler), numa_manager_(numa_mgr), detection_enabled_(true) {
    
    initialize_bottleneck_patterns();
    
    detection_thread_ = std::thread([this]() {
        bottleneck_detection_worker();
    });
    
    LOG_INFO("Memory bottleneck detector initialized");
}

MemoryBottleneckDetector::~MemoryBottleneckDetector() {
    shutdown_requested_.store(true);
    if (detection_thread_.joinable()) {
        detection_thread_.join();
    }
}

std::vector<BottleneckReport> MemoryBottleneckDetector::analyze_current_bottlenecks() {
    PROFILE_FUNCTION();
    
    std::vector<BottleneckReport> reports;
    
    // Get recent bandwidth measurements
    auto recent_measurements = get_recent_measurements();
    if (recent_measurements.size() < 5) {
        LOG_DEBUG("Not enough measurements for bottleneck analysis");
        return reports;
    }
    
    // Analyze bandwidth degradation patterns
    auto bandwidth_report = analyze_bandwidth_degradation(recent_measurements);
    if (bandwidth_report.severity > BottleneckSeverity::None) {
        reports.push_back(bandwidth_report);
    }
    
    // Analyze NUMA locality issues
    auto numa_report = analyze_numa_bottlenecks(recent_measurements);
    if (numa_report.severity > BottleneckSeverity::None) {
        reports.push_back(numa_report);
    }
    
    // Analyze cache efficiency issues
    auto cache_report = analyze_cache_bottlenecks(recent_measurements);
    if (cache_report.severity > BottleneckSeverity::None) {
        reports.push_back(cache_report);
    }
    
    // Analyze memory access patterns
    auto pattern_report = analyze_access_pattern_bottlenecks(recent_measurements);
    if (pattern_report.severity > BottleneckSeverity::None) {
        reports.push_back(pattern_report);
    }
    
    // Record bottleneck detection
    record_bottleneck_analysis(reports);
    
    LOG_DEBUG("Detected {} memory bottlenecks", reports.size());
    
    return reports;
}

PerformanceRecommendations MemoryBottleneckDetector::generate_optimization_recommendations() {
    PerformanceRecommendations recommendations{};
    
    auto bottlenecks = analyze_current_bottlenecks();
    if (bottlenecks.empty()) {
        recommendations.summary = "No significant memory bottlenecks detected.";
        return recommendations;
    }
    
    std::ostringstream summary;
    summary << "Memory Performance Analysis:\n";
    
    for (const auto& bottleneck : bottlenecks) {
        summary << "\n" << severity_to_string(bottleneck.severity) << " - " << bottleneck.bottleneck_type << ":\n";
        summary << "  " << bottleneck.description << "\n";
        
        for (const auto& recommendation : bottleneck.recommendations) {
            recommendations.high_priority_actions.push_back(recommendation);
            summary << "  → " << recommendation << "\n";
        }
    }
    
    // Add general optimization recommendations
    generate_general_recommendations(recommendations);
    
    recommendations.summary = summary.str();
    return recommendations;
}

SystemWideBottleneckAnalysis MemoryBottleneckDetector::perform_comprehensive_analysis() {
    PROFILE_FUNCTION();
    
    SystemWideBottleneckAnalysis analysis{};
    analysis.analysis_timestamp = get_current_time();
    
    LOG_INFO("Performing comprehensive system-wide bottleneck analysis...");
    
    // Test all NUMA nodes with comprehensive patterns
    auto available_nodes = numa_manager_.get_topology().get_available_nodes();
    
    for (u32 node : available_nodes) {
        LOG_DEBUG("Analyzing bottlenecks for NUMA node {}...", node);
        
        auto node_measurements = bandwidth_profiler_.profile_all_patterns(node);
        
        NodeBottleneckAnalysis node_analysis{};
        node_analysis.node_id = node;
        
        // Analyze bandwidth efficiency
        f64 total_bandwidth = 0.0;
        f64 peak_bandwidth = 0.0;
        
        for (const auto& measurement : node_measurements) {
            f64 bandwidth = measurement.get_effective_bandwidth();
            total_bandwidth += bandwidth;
            peak_bandwidth = std::max(peak_bandwidth, bandwidth);
        }
        
        if (!node_measurements.empty()) {
            node_analysis.average_bandwidth_gbps = total_bandwidth / node_measurements.size();
            node_analysis.peak_bandwidth_gbps = peak_bandwidth;
            node_analysis.bandwidth_efficiency = node_analysis.average_bandwidth_gbps / 
                                                std::max(1.0, analysis.theoretical_peak_bandwidth);
        }
        
        // Identify primary bottleneck type
        node_analysis.primary_bottleneck = identify_primary_bottleneck(node_measurements);
        
        // Generate node-specific recommendations
        node_analysis.optimization_suggestions = generate_node_recommendations(node_measurements);
        
        analysis.node_analyses[node] = node_analysis;
    }
    
    // Calculate system-wide metrics
    f64 total_system_bandwidth = 0.0;
    for (const auto& [node_id, node_analysis] : analysis.node_analyses) {
        total_system_bandwidth += node_analysis.average_bandwidth_gbps;
    }
    
    analysis.system_aggregate_bandwidth = total_system_bandwidth;
    analysis.theoretical_peak_bandwidth = 50.0 * available_nodes.size(); // Estimated 50 GB/s per node
    analysis.system_efficiency = analysis.system_aggregate_bandwidth / analysis.theoretical_peak_bandwidth;
    
    // Generate system-wide recommendations
    analysis.system_wide_recommendations = generate_system_recommendations(analysis);
    
    LOG_INFO("Comprehensive analysis complete. System efficiency: {:.1f}%", 
             analysis.system_efficiency * 100.0);
    
    return analysis;
}

// Private implementation methods

void MemoryBottleneckDetector::initialize_bottleneck_patterns() {
    // Initialize patterns specifically designed to trigger different bottlenecks
    bottleneck_test_patterns_.clear();
    
    // Cache thrashing pattern
    bottleneck_test_patterns_.emplace_back(
        MemoryAccessPattern::AccessType::Strided, 
        32 * 1024 * 1024, // 32MB (larger than typical L3 cache)
        8192,              // Large stride to cause cache misses
        1.0
    );
    
    // NUMA bottleneck pattern
    bottleneck_test_patterns_.emplace_back(
        MemoryAccessPattern::AccessType::Random,
        100 * 1024 * 1024, // 100MB spread across memory
        0,
        1.0
    );
    
    // Memory bandwidth saturation pattern
    bottleneck_test_patterns_.emplace_back(
        MemoryAccessPattern::AccessType::Sequential,
        500 * 1024 * 1024, // 500MB sequential
        64,
        1.0
    );
}

std::vector<BandwidthMeasurement> MemoryBottleneckDetector::get_recent_measurements() const {
    // Get measurements from the bandwidth profiler
    auto report = bandwidth_profiler_.generate_comprehensive_report();
    
    // For now, return empty vector - would need access to profiler's internal measurements
    return {};
}

BottleneckReport MemoryBottleneckDetector::analyze_bandwidth_degradation(
    const std::vector<BandwidthMeasurement>& measurements) {
    
    BottleneckReport report{};
    report.bottleneck_type = "Bandwidth Degradation";
    report.severity = BottleneckSeverity::None;
    
    if (measurements.size() < 5) {
        return report;
    }
    
    // Analyze bandwidth trends
    f64 total_bandwidth = 0.0;
    f64 min_bandwidth = std::numeric_limits<f64>::max();
    f64 max_bandwidth = 0.0;
    
    for (const auto& measurement : measurements) {
        f64 bandwidth = measurement.get_effective_bandwidth();
        total_bandwidth += bandwidth;
        min_bandwidth = std::min(min_bandwidth, bandwidth);
        max_bandwidth = std::max(max_bandwidth, bandwidth);
    }
    
    f64 average_bandwidth = total_bandwidth / measurements.size();
    f64 bandwidth_variance = (max_bandwidth - min_bandwidth) / average_bandwidth;
    
    // Determine severity based on variance and absolute values
    if (average_bandwidth < 5.0) { // Less than 5 GB/s
        report.severity = BottleneckSeverity::Critical;
        report.description = "Extremely low memory bandwidth detected (" + 
                           std::to_string(average_bandwidth) + " GB/s)";
        report.recommendations.push_back("Check for memory configuration issues");
        report.recommendations.push_back("Consider memory upgrade");
        report.recommendations.push_back("Optimize memory access patterns");
    } else if (bandwidth_variance > 0.5) { // High variance
        report.severity = BottleneckSeverity::High;
        report.description = "High bandwidth variance detected (variance: " + 
                           std::to_string(bandwidth_variance * 100.0) + "%)";
        report.recommendations.push_back("Investigate memory contention");
        report.recommendations.push_back("Review thread allocation strategies");
    } else if (average_bandwidth < 15.0) { // Moderate bandwidth
        report.severity = BottleneckSeverity::Medium;
        report.description = "Moderate memory bandwidth (" + 
                           std::to_string(average_bandwidth) + " GB/s)";
        report.recommendations.push_back("Consider memory access optimization");
    }
    
    report.impact_score = 1.0 - (average_bandwidth / 30.0); // Normalized to typical DDR4 peak
    
    return report;
}

BottleneckReport MemoryBottleneckDetector::analyze_numa_bottlenecks(
    const std::vector<BandwidthMeasurement>& measurements) {
    
    BottleneckReport report{};
    report.bottleneck_type = "NUMA Locality";
    report.severity = BottleneckSeverity::None;
    
    // Analyze NUMA locality using numa_manager metrics
    auto numa_metrics = numa_manager_.get_performance_metrics();
    
    if (numa_metrics.local_access_ratio < 0.6) {
        report.severity = BottleneckSeverity::High;
        report.description = "Poor NUMA locality (local access ratio: " + 
                           std::to_string(numa_metrics.local_access_ratio * 100.0) + "%)";
        report.recommendations.push_back("Set explicit thread affinity");
        report.recommendations.push_back("Use NUMA-aware allocation");
        report.recommendations.push_back("Consider memory migration");
        report.impact_score = 1.0 - numa_metrics.local_access_ratio;
    } else if (numa_metrics.local_access_ratio < 0.8) {
        report.severity = BottleneckSeverity::Medium;
        report.description = "Moderate NUMA locality issues (local access ratio: " + 
                           std::to_string(numa_metrics.local_access_ratio * 100.0) + "%)";
        report.recommendations.push_back("Review data placement strategies");
        report.recommendations.push_back("Consider thread-local data structures");
        report.impact_score = (1.0 - numa_metrics.local_access_ratio) * 0.5;
    }
    
    return report;
}

BottleneckReport MemoryBottleneckDetector::analyze_cache_bottlenecks(
    const std::vector<BandwidthMeasurement>& measurements) {
    
    BottleneckReport report{};
    report.bottleneck_type = "Cache Efficiency";
    report.severity = BottleneckSeverity::None;
    
    // Analyze cache efficiency from access patterns
    f64 total_cache_efficiency = 0.0;
    usize efficiency_samples = 0;
    
    for (const auto& measurement : measurements) {
        f64 efficiency = measurement.get_cache_efficiency();
        if (efficiency > 0.0) {
            total_cache_efficiency += efficiency;
            ++efficiency_samples;
        }
    }
    
    if (efficiency_samples > 0) {
        f64 average_efficiency = total_cache_efficiency / efficiency_samples;
        
        if (average_efficiency < 0.3) {
            report.severity = BottleneckSeverity::High;
            report.description = "Poor cache efficiency (average: " + 
                               std::to_string(average_efficiency * 100.0) + "%)";
            report.recommendations.push_back("Improve data locality");
            report.recommendations.push_back("Reduce working set size");
            report.recommendations.push_back("Use cache-friendly algorithms");
            report.impact_score = 1.0 - average_efficiency;
        } else if (average_efficiency < 0.6) {
            report.severity = BottleneckSeverity::Medium;
            report.description = "Moderate cache efficiency issues (average: " + 
                               std::to_string(average_efficiency * 100.0) + "%)";
            report.recommendations.push_back("Review data structure layout");
            report.recommendations.push_back("Consider prefetching strategies");
            report.impact_score = (1.0 - average_efficiency) * 0.5;
        }
    }
    
    return report;
}

BottleneckReport MemoryBottleneckDetector::analyze_access_pattern_bottlenecks(
    const std::vector<BandwidthMeasurement>& measurements) {
    
    BottleneckReport report{};
    report.bottleneck_type = "Access Pattern";
    report.severity = BottleneckSeverity::None;
    
    // Analyze the distribution of access patterns
    std::unordered_map<std::string, f64> pattern_bandwidths;
    
    for (const auto& measurement : measurements) {
        pattern_bandwidths[measurement.pattern_name] += measurement.get_effective_bandwidth();
    }
    
    // Look for particularly poor-performing patterns
    f64 worst_bandwidth = std::numeric_limits<f64>::max();
    std::string worst_pattern;
    
    for (const auto& [pattern, bandwidth] : pattern_bandwidths) {
        if (bandwidth < worst_bandwidth) {
            worst_bandwidth = bandwidth;
            worst_pattern = pattern;
        }
    }
    
    if (worst_bandwidth < 2.0) { // Less than 2 GB/s
        report.severity = BottleneckSeverity::High;
        report.description = "Extremely poor access pattern performance: " + worst_pattern + 
                           " (" + std::to_string(worst_bandwidth) + " GB/s)";
        
        if (worst_pattern.find("Random") != std::string::npos) {
            report.recommendations.push_back("Reduce random access patterns");
            report.recommendations.push_back("Consider spatial data structures");
            report.recommendations.push_back("Implement prefetching");
        } else if (worst_pattern.find("Strided") != std::string::npos) {
            report.recommendations.push_back("Optimize stride patterns");
            report.recommendations.push_back("Align data to cache boundaries");
        }
        
        report.impact_score = 1.0 - (worst_bandwidth / 10.0);
    }
    
    return report;
}

std::string MemoryBottleneckDetector::identify_primary_bottleneck(
    const std::vector<BandwidthMeasurement>& measurements) {
    
    if (measurements.empty()) return "Unknown";
    
    // Simple heuristic based on bandwidth ranges
    f64 total_bandwidth = 0.0;
    for (const auto& measurement : measurements) {
        total_bandwidth += measurement.get_effective_bandwidth();
    }
    f64 average_bandwidth = total_bandwidth / measurements.size();
    
    if (average_bandwidth < 5.0) return "Memory Hardware";
    if (average_bandwidth < 10.0) return "NUMA Locality";
    if (average_bandwidth < 20.0) return "Cache Efficiency";
    return "None";
}

std::vector<std::string> MemoryBottleneckDetector::generate_node_recommendations(
    const std::vector<BandwidthMeasurement>& measurements) {
    
    std::vector<std::string> recommendations;
    
    // Analyze measurement patterns and generate specific recommendations
    bool has_random_access = false;
    bool has_poor_sequential = false;
    f64 min_bandwidth = std::numeric_limits<f64>::max();
    
    for (const auto& measurement : measurements) {
        f64 bandwidth = measurement.get_effective_bandwidth();
        min_bandwidth = std::min(min_bandwidth, bandwidth);
        
        if (measurement.pattern_name.find("Random") != std::string::npos) {
            has_random_access = true;
        }
        if (measurement.pattern_name.find("Sequential") != std::string::npos && bandwidth < 15.0) {
            has_poor_sequential = true;
        }
    }
    
    if (has_poor_sequential) {
        recommendations.push_back("Investigate memory subsystem configuration");
        recommendations.push_back("Check for memory channel utilization");
    }
    
    if (has_random_access && min_bandwidth < 2.0) {
        recommendations.push_back("Minimize random memory access patterns");
        recommendations.push_back("Use cache-friendly data structures");
        recommendations.push_back("Consider data prefetching strategies");
    }
    
    if (min_bandwidth < 5.0) {
        recommendations.push_back("Consider memory hardware upgrade");
        recommendations.push_back("Review system memory configuration");
    }
    
    return recommendations;
}

std::vector<std::string> MemoryBottleneckDetector::generate_system_recommendations(
    const SystemWideBottleneckAnalysis& analysis) {
    
    std::vector<std::string> recommendations;
    
    if (analysis.system_efficiency < 0.5) {
        recommendations.push_back("System-wide memory performance is poor - investigate hardware configuration");
        recommendations.push_back("Consider memory bandwidth optimization at the system level");
    }
    
    // Check for NUMA imbalance
    f64 max_node_bandwidth = 0.0;
    f64 min_node_bandwidth = std::numeric_limits<f64>::max();
    
    for (const auto& [node_id, node_analysis] : analysis.node_analyses) {
        max_node_bandwidth = std::max(max_node_bandwidth, node_analysis.average_bandwidth_gbps);
        min_node_bandwidth = std::min(min_node_bandwidth, node_analysis.average_bandwidth_gbps);
    }
    
    if (max_node_bandwidth > 0.0 && (max_node_bandwidth - min_node_bandwidth) / max_node_bandwidth > 0.3) {
        recommendations.push_back("Significant NUMA node performance imbalance detected");
        recommendations.push_back("Review workload distribution across NUMA nodes");
        recommendations.push_back("Consider explicit NUMA-aware optimization");
    }
    
    return recommendations;
}

void MemoryBottleneckDetector::generate_general_recommendations(PerformanceRecommendations& recommendations) {
    recommendations.general_optimizations.push_back("Use memory pools for frequent allocations");
    recommendations.general_optimizations.push_back("Align data structures to cache line boundaries");
    recommendations.general_optimizations.push_back("Consider NUMA-aware thread placement");
    recommendations.general_optimizations.push_back("Profile memory access patterns regularly");
    
    recommendations.monitoring_suggestions.push_back("Monitor memory bandwidth utilization");
    recommendations.monitoring_suggestions.push_back("Track NUMA locality metrics");
    recommendations.monitoring_suggestions.push_back("Analyze cache miss rates");
    recommendations.monitoring_suggestions.push_back("Review memory allocation patterns");
}

std::string MemoryBottleneckDetector::severity_to_string(BottleneckSeverity severity) {
    switch (severity) {
        case BottleneckSeverity::None: return "INFO";
        case BottleneckSeverity::Low: return "LOW";
        case BottleneckSeverity::Medium: return "MEDIUM";
        case BottleneckSeverity::High: return "HIGH";
        case BottleneckSeverity::Critical: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

void MemoryBottleneckDetector::record_bottleneck_analysis(const std::vector<BottleneckReport>& reports) {
    std::lock_guard<std::mutex> lock(analysis_history_mutex_);
    
    BottleneckAnalysisRecord record{};
    record.timestamp = get_current_time();
    record.bottleneck_count = reports.size();
    
    for (const auto& report : reports) {
        record.severity_counts[static_cast<int>(report.severity)]++;
        record.total_impact_score += report.impact_score;
    }
    
    analysis_history_.push_back(record);
    
    // Keep only recent analysis history
    constexpr usize MAX_HISTORY_SIZE = 1000;
    if (analysis_history_.size() > MAX_HISTORY_SIZE) {
        analysis_history_.erase(analysis_history_.begin());
    }
}

void MemoryBottleneckDetector::bottleneck_detection_worker() {
    LOG_DEBUG("Bottleneck detection worker started");
    
    while (!shutdown_requested_.load() && detection_enabled_.load()) {
        // Run periodic bottleneck detection
        analyze_current_bottlenecks();
        
        // Sleep between detection runs
        std::this_thread::sleep_for(std::chrono::minutes(5));
    }
    
    LOG_DEBUG("Bottleneck detection worker stopped");
}

f64 MemoryBottleneckDetector::get_current_time() const {
    using namespace std::chrono;
    return duration<f64>(steady_clock::now().time_since_epoch()).count();
}

//=============================================================================
// Global instances
//=============================================================================

MemoryBandwidthProfiler& get_global_bandwidth_profiler() {
    static auto& numa_manager = numa::get_global_numa_manager();
    static MemoryBandwidthProfiler instance(numa_manager);
    return instance;
}

MemoryBottleneckDetector& get_global_bottleneck_detector() {
    static auto& profiler = get_global_bandwidth_profiler();
    static auto& numa_manager = numa::get_global_numa_manager();
    static MemoryBottleneckDetector instance(profiler, numa_manager);
    return instance;
}

} // namespace ecscope::memory::bandwidth