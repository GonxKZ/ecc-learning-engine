/**
 * @file sparse_set_visualizer.cpp
 * @brief Implementation of Sparse Set Visualization and Analysis
 */

#include "ecscope/sparse_set_visualizer.hpp"
#include "core/time.hpp"
#include "core/log.hpp"
#include <algorithm>
#include <numeric>
#include <fstream>
#include <sstream>
#include <cmath>
#include <unordered_set>

namespace ecscope::visualization {

// Static member definitions
std::unique_ptr<SparseSetAnalyzer> GlobalSparseSetAnalyzer::instance_;
std::once_flag GlobalSparseSetAnalyzer::init_flag_;

SparseSetAnalyzer::SparseSetAnalyzer() noexcept
    : enable_access_tracking_(true)
    , enable_cache_analysis_(true)
    , analysis_frequency_(DEFAULT_ANALYSIS_FREQUENCY)
    , last_analysis_time_(0.0)
    , start_time_(std::chrono::high_resolution_clock::now())
    , cache_index_(0)
{
    LOG_INFO("Sparse Set Analyzer initialized");
    simulated_cache_.fill(0);
}

void SparseSetAnalyzer::register_sparse_set(const std::string& name, usize initial_capacity) noexcept {
    std::unique_lock<std::shared_mutex> lock(data_mutex_);
    
    if (sparse_sets_.find(name) != sparse_sets_.end()) {
        LOG_WARN("Sparse set already registered: " + name);
        return;
    }
    
    auto data = std::make_unique<SparseSetVisualizationData>();
    data->name = name;
    data->dense_capacity = initial_capacity;
    data->sparse_capacity = initial_capacity * 2; // Typically larger than dense
    data->dense_size = 0;
    
    // Initialize visualization arrays
    data->dense_occupied.resize(initial_capacity, false);
    data->sparse_valid.resize(data->sparse_capacity, false);
    data->sparse_to_dense.resize(data->sparse_capacity, 0);
    data->dense_to_sparse.resize(initial_capacity, 0);
    
    // Calculate initial memory usage
    data->memory_dense = initial_capacity * sizeof(u32);
    data->memory_sparse = data->sparse_capacity * sizeof(u32);
    data->memory_total = data->memory_dense + data->memory_sparse;
    
    sparse_sets_[name] = std::move(data);
    
    LOG_INFO("Registered sparse set: " + name + " with capacity " + std::to_string(initial_capacity));
}

void SparseSetAnalyzer::unregister_sparse_set(const std::string& name) noexcept {
    std::unique_lock<std::shared_mutex> lock(data_mutex_);
    
    auto it = sparse_sets_.find(name);
    if (it != sparse_sets_.end()) {
        sparse_sets_.erase(it);
        LOG_INFO("Unregistered sparse set: " + name);
    }
}

bool SparseSetAnalyzer::has_sparse_set(const std::string& name) const noexcept {
    std::shared_lock<std::shared_mutex> lock(data_mutex_);
    return sparse_sets_.find(name) != sparse_sets_.end();
}

void SparseSetAnalyzer::update_sparse_set_data(const std::string& name, usize dense_size, 
                                              usize sparse_capacity, const void* dense_ptr, 
                                              const void* sparse_ptr) noexcept {
    std::unique_lock<std::shared_mutex> lock(data_mutex_);
    
    auto it = sparse_sets_.find(name);
    if (it == sparse_sets_.end()) {
        LOG_WARN("Attempted to update unknown sparse set: " + name);
        return;
    }
    
    auto& data = *it->second;
    
    // Update size information
    data.dense_size = dense_size;
    data.sparse_capacity = sparse_capacity;
    
    // Resize arrays if needed
    if (data.dense_occupied.size() < dense_size) {
        data.dense_occupied.resize(dense_size, false);
        data.dense_to_sparse.resize(dense_size, 0);
        data.dense_capacity = dense_size;
    }
    
    if (data.sparse_valid.size() < sparse_capacity) {
        data.sparse_valid.resize(sparse_capacity, false);
        data.sparse_to_dense.resize(sparse_capacity, 0);
    }
    
    // Update occupancy maps (simplified - in real implementation would read actual data)
    std::fill(data.dense_occupied.begin(), data.dense_occupied.begin() + dense_size, true);
    std::fill(data.dense_occupied.begin() + dense_size, data.dense_occupied.end(), false);
    
    // Update memory usage
    data.memory_dense = dense_size * sizeof(u32);
    data.memory_sparse = sparse_capacity * sizeof(u32);
    data.memory_total = data.memory_dense + data.memory_sparse;
    
    // Calculate memory efficiency
    calculate_memory_efficiency(data);
}

void SparseSetAnalyzer::track_access(const std::string& name, void* address, usize size, 
                                    bool is_write, SparseSetAccessPattern pattern) noexcept {
    if (!enable_access_tracking_) return;
    
    std::unique_lock<std::shared_mutex> lock(data_mutex_);
    
    auto it = sparse_sets_.find(name);
    if (it == sparse_sets_.end()) return;
    
    auto& data = *it->second;
    auto& tracking = data.access_tracking;
    
    // Update access counts
    if (is_write) {
        tracking.write_count++;
    } else {
        tracking.read_count++;
    }
    
    // Update timing
    f64 current_time = get_current_time();
    f64 time_delta = current_time - tracking.last_access_time;
    tracking.last_access_time = current_time;
    
    // Update access frequency (exponential moving average)
    if (time_delta > 0) {
        f64 instant_frequency = 1.0 / time_delta;
        tracking.access_frequency = tracking.access_frequency * 0.9 + instant_frequency * 0.1;
    }
    
    // Cache simulation
    if (enable_cache_analysis_) {
        bool cache_hit = !would_cause_cache_miss(address, size);
        if (cache_hit) {
            tracking.cache_hits++;
        } else {
            tracking.cache_misses++;
        }
    }
    
    // Update pattern
    tracking.pattern = pattern;
    
    ++total_operations_;
    if (tracking.cache_hits > tracking.cache_misses) {
        ++cache_friendly_operations_;
    }
}

void SparseSetAnalyzer::track_insertion(const std::string& name, f64 duration_us) noexcept {
    std::unique_lock<std::shared_mutex> lock(data_mutex_);
    
    auto it = sparse_sets_.find(name);
    if (it == sparse_sets_.end()) return;
    
    auto& data = *it->second;
    static u64 insertion_count = 0;
    update_performance_averages(data, duration_us, data.insertion_time_avg, ++insertion_count);
}

void SparseSetAnalyzer::track_removal(const std::string& name, f64 duration_us) noexcept {
    std::unique_lock<std::shared_mutex> lock(data_mutex_);
    
    auto it = sparse_sets_.find(name);
    if (it == sparse_sets_.end()) return;
    
    auto& data = *it->second;
    static u64 removal_count = 0;
    update_performance_averages(data, duration_us, data.removal_time_avg, ++removal_count);
}

void SparseSetAnalyzer::track_lookup(const std::string& name, f64 duration_us) noexcept {
    std::unique_lock<std::shared_mutex> lock(data_mutex_);
    
    auto it = sparse_sets_.find(name);
    if (it == sparse_sets_.end()) return;
    
    auto& data = *it->second;
    static u64 lookup_count = 0;
    update_performance_averages(data, duration_us, data.lookup_time_avg, ++lookup_count);
}

void SparseSetAnalyzer::track_iteration(const std::string& name, f64 duration_us) noexcept {
    std::unique_lock<std::shared_mutex> lock(data_mutex_);
    
    auto it = sparse_sets_.find(name);
    if (it == sparse_sets_.end()) return;
    
    auto& data = *it->second;
    static u64 iteration_count = 0;
    update_performance_averages(data, duration_us, data.iteration_time_avg, ++iteration_count);
}

void SparseSetAnalyzer::analyze_all() noexcept {
    f64 current_time = get_current_time();
    if (current_time - last_analysis_time_ < (1.0 / analysis_frequency_)) {
        return; // Too soon for next analysis
    }
    
    std::shared_lock<std::shared_mutex> lock(data_mutex_);
    
    for (auto& [name, data] : sparse_sets_) {
        analyze_sparse_set(name);
    }
    
    last_analysis_time_ = current_time;
}

void SparseSetAnalyzer::analyze_sparse_set(const std::string& name) noexcept {
    auto it = sparse_sets_.find(name);
    if (it == sparse_sets_.end()) return;
    
    auto& data = *it->second;
    
    calculate_memory_efficiency(data);
    
    if (enable_cache_analysis_) {
        calculate_cache_metrics(data);
    }
    
    detect_access_patterns(data);
    generate_optimization_suggestions(data);
    generate_educational_content(data);
}

void SparseSetAnalyzer::analyze_cache_behavior(const std::string& name) noexcept {
    if (!enable_cache_analysis_) return;
    
    auto it = sparse_sets_.find(name);
    if (it == sparse_sets_.end()) return;
    
    auto& data = *it->second;
    calculate_cache_metrics(data);
}

void SparseSetAnalyzer::analyze_memory_patterns(const std::string& name) noexcept {
    auto it = sparse_sets_.find(name);
    if (it == sparse_sets_.end()) return;
    
    auto& data = *it->second;
    calculate_memory_efficiency(data);
}

void SparseSetAnalyzer::generate_optimization_suggestions(const std::string& name) noexcept {
    auto it = sparse_sets_.find(name);
    if (it == sparse_sets_.end()) return;
    
    auto& data = *it->second;
    data.optimization_suggestions.clear();
    
    // Analyze utilization
    f64 utilization = data.dense_size > 0 ? 
        static_cast<f64>(data.dense_size) / data.dense_capacity : 0.0;
    
    if (utilization < 0.5) {
        data.optimization_suggestions.push_back(
            "Low utilization detected - consider reducing initial capacity to save memory");
    }
    
    if (utilization > 0.9) {
        data.optimization_suggestions.push_back(
            "High utilization - consider pre-allocating more capacity to reduce reallocations");
    }
    
    // Analyze cache performance
    if (data.cache_locality_score < 0.7) {
        data.optimization_suggestions.push_back(
            "Poor cache locality - consider grouping related operations or using prefetching");
    }
    
    // Analyze access patterns
    auto& tracking = data.access_tracking;
    if (tracking.cache_misses > tracking.cache_hits) {
        data.optimization_suggestions.push_back(
            "High cache miss rate - consider restructuring data layout for better locality");
    }
    
    // Performance analysis
    if (data.insertion_time_avg > 10.0) { // > 10 microseconds
        data.optimization_suggestions.push_back(
            "Slow insertions detected - verify sparse set implementation efficiency");
    }
}

std::vector<std::string> SparseSetAnalyzer::get_sparse_set_names() const noexcept {
    std::shared_lock<std::shared_mutex> lock(data_mutex_);
    
    std::vector<std::string> names;
    names.reserve(sparse_sets_.size());
    
    for (const auto& [name, data] : sparse_sets_) {
        names.push_back(name);
    }
    
    return names;
}

const SparseSetVisualizationData* SparseSetAnalyzer::get_sparse_set_data(const std::string& name) const noexcept {
    std::shared_lock<std::shared_mutex> lock(data_mutex_);
    
    auto it = sparse_sets_.find(name);
    return it != sparse_sets_.end() ? it->second.get() : nullptr;
}

std::vector<const SparseSetVisualizationData*> SparseSetAnalyzer::get_all_sparse_sets() const noexcept {
    std::shared_lock<std::shared_mutex> lock(data_mutex_);
    
    std::vector<const SparseSetVisualizationData*> result;
    result.reserve(sparse_sets_.size());
    
    for (const auto& [name, data] : sparse_sets_) {
        result.push_back(data.get());
    }
    
    return result;
}

usize SparseSetAnalyzer::get_total_sparse_sets() const noexcept {
    std::shared_lock<std::shared_mutex> lock(data_mutex_);
    return sparse_sets_.size();
}

usize SparseSetAnalyzer::get_total_memory_usage() const noexcept {
    std::shared_lock<std::shared_mutex> lock(data_mutex_);
    
    usize total = 0;
    for (const auto& [name, data] : sparse_sets_) {
        total += data->memory_total;
    }
    return total;
}

f64 SparseSetAnalyzer::get_average_cache_locality() const noexcept {
    std::shared_lock<std::shared_mutex> lock(data_mutex_);
    
    if (sparse_sets_.empty()) return 0.0;
    
    f64 total = 0.0;
    for (const auto& [name, data] : sparse_sets_) {
        total += data->cache_locality_score;
    }
    
    return total / sparse_sets_.size();
}

f64 SparseSetAnalyzer::get_overall_efficiency() const noexcept {
    std::shared_lock<std::shared_mutex> lock(data_mutex_);
    
    if (sparse_sets_.empty()) return 0.0;
    
    f64 total = 0.0;
    for (const auto& [name, data] : sparse_sets_) {
        total += data->memory_efficiency;
    }
    
    return total / sparse_sets_.size();
}

std::vector<std::string> SparseSetAnalyzer::get_educational_insights() const noexcept {
    std::vector<std::string> insights;
    
    insights.push_back("Sparse sets provide O(1) insertion, deletion, and lookup operations");
    insights.push_back("Dense arrays enable cache-friendly iteration over components");
    insights.push_back("Sparse arrays provide fast entity-to-component mapping");
    insights.push_back("Memory locality is crucial for performance in component systems");
    insights.push_back("Sequential access patterns are more cache-friendly than random access");
    
    // Add dynamic insights based on current data
    u64 total_ops = total_operations_.load();
    u64 cache_friendly = cache_friendly_operations_.load();
    
    if (total_ops > 0) {
        f64 cache_friendly_ratio = static_cast<f64>(cache_friendly) / total_ops;
        if (cache_friendly_ratio > 0.8) {
            insights.push_back("Current operations show good cache-friendly patterns");
        } else if (cache_friendly_ratio < 0.5) {
            insights.push_back("Consider optimizing access patterns for better cache utilization");
        }
    }
    
    return insights;
}

std::string SparseSetAnalyzer::explain_sparse_set_concept() const noexcept {
    return "A sparse set is a data structure that maintains two arrays: a dense array containing "
           "actual data in contiguous memory, and a sparse array that maps entity IDs to indices "
           "in the dense array. This enables O(1) operations while maintaining cache-friendly "
           "iteration through the dense array. The sparse array can be large and mostly empty, "
           "while the dense array remains compact and sequential.";
}

std::string SparseSetAnalyzer::explain_cache_locality() const noexcept {
    return "Cache locality refers to the principle that accessing memory locations close to "
           "recently accessed locations is faster due to CPU cache behavior. Sparse sets "
           "optimize for spatial locality by keeping component data contiguous in the dense "
           "array, and temporal locality by accessing recently used data. Good cache locality "
           "can improve performance by orders of magnitude in component iteration.";
}

std::string SparseSetAnalyzer::suggest_performance_improvements() const noexcept {
    std::ostringstream oss;
    oss << "Performance Improvement Suggestions:\n";
    oss << "1. Use batch operations for multiple insertions/removals\n";
    oss << "2. Iterate through dense arrays rather than random access\n";
    oss << "3. Group related component types together\n";
    oss << "4. Pre-allocate capacity to avoid reallocations\n";
    oss << "5. Consider component packing for better cache utilization\n";
    oss << "6. Profile memory access patterns and optimize hot paths\n";
    return oss.str();
}

// Private implementation methods

void SparseSetAnalyzer::calculate_memory_efficiency(SparseSetVisualizationData& data) noexcept {
    if (data.sparse_capacity == 0) {
        data.memory_efficiency = 0.0;
        return;
    }
    
    // Calculate efficiency as (used memory) / (total allocated memory)
    usize used_memory = data.dense_size * sizeof(u32); // Only count actual used dense entries
    data.memory_efficiency = data.memory_total > 0 ? 
        static_cast<f64>(used_memory) / data.memory_total : 0.0;
}

void SparseSetAnalyzer::calculate_cache_metrics(SparseSetVisualizationData& data) noexcept {
    auto& tracking = data.access_tracking;
    
    // Calculate cache hit ratio
    u64 total_accesses = tracking.cache_hits + tracking.cache_misses;
    f64 cache_hit_ratio = total_accesses > 0 ? 
        static_cast<f64>(tracking.cache_hits) / total_accesses : 0.0;
    
    // Spatial locality: how well sequential accesses perform
    data.spatial_locality = cache_hit_ratio;
    
    // Temporal locality: based on access frequency
    data.temporal_locality = std::min(1.0, tracking.access_frequency / 100.0); // Normalize to 0-1
    
    // Overall cache locality score
    data.cache_locality_score = (data.spatial_locality + data.temporal_locality) * 0.5;
    
    // Estimate cache lines used
    data.cache_line_utilization = calculate_cache_lines_used(
        nullptr, data.dense_size * sizeof(u32)); // Simplified calculation
}

void SparseSetAnalyzer::detect_access_patterns(SparseSetVisualizationData& data) noexcept {
    auto& tracking = data.access_tracking;
    
    // Simple pattern detection based on read/write ratio and frequency
    if (tracking.write_count > tracking.read_count * 2) {
        tracking.pattern = SparseSetAccessPattern::Bulk_Insert;
    } else if (tracking.read_count > tracking.write_count * 10) {
        tracking.pattern = SparseSetAccessPattern::Sequential; // Likely iteration
    } else if (tracking.access_frequency > 50.0) {
        tracking.pattern = SparseSetAccessPattern::Random; // High frequency, mixed access
    } else {
        tracking.pattern = SparseSetAccessPattern::Mixed;
    }
}

void SparseSetAnalyzer::generate_educational_content(SparseSetVisualizationData& data) noexcept {
    data.performance_insights.clear();
    
    // Generate insights based on current metrics
    if (data.memory_efficiency > 0.8) {
        data.performance_insights.push_back("Excellent memory utilization - dense packing achieved");
    } else if (data.memory_efficiency < 0.4) {
        data.performance_insights.push_back("Low memory efficiency - consider compacting or reducing capacity");
    }
    
    if (data.cache_locality_score > 0.8) {
        data.performance_insights.push_back("Good cache locality - sequential access patterns detected");
    } else if (data.cache_locality_score < 0.5) {
        data.performance_insights.push_back("Poor cache locality - random access patterns may be degrading performance");
    }
    
    auto& tracking = data.access_tracking;
    if (tracking.read_count > tracking.write_count * 5) {
        data.performance_insights.push_back("Read-heavy workload - well suited for sparse set architecture");
    } else if (tracking.write_count > tracking.read_count) {
        data.performance_insights.push_back("Write-heavy workload - consider batch operations for better performance");
    }
}

bool SparseSetAnalyzer::would_cause_cache_miss(void* address, usize size) const noexcept {
    uintptr_t addr = reinterpret_cast<uintptr_t>(address);
    uintptr_t cache_line = addr / CACHE_LINE_SIZE;
    
    // Simple cache simulation - check if cache line is in our simulated cache
    for (const auto& cached_line : simulated_cache_) {
        if (cached_line == cache_line) {
            return false; // Cache hit
        }
    }
    
    // Cache miss - update simulated cache (simple LRU)
    const_cast<SparseSetAnalyzer*>(this)->simulated_cache_[cache_index_] = cache_line;
    const_cast<SparseSetAnalyzer*>(this)->cache_index_ = (cache_index_ + 1) % simulated_cache_.size();
    
    return true; // Cache miss
}

f64 SparseSetAnalyzer::estimate_cache_locality(const std::vector<void*>& access_sequence) const noexcept {
    if (access_sequence.size() < 2) return 1.0;
    
    usize sequential_accesses = 0;
    
    for (usize i = 1; i < access_sequence.size(); ++i) {
        uintptr_t prev_addr = reinterpret_cast<uintptr_t>(access_sequence[i-1]);
        uintptr_t curr_addr = reinterpret_cast<uintptr_t>(access_sequence[i]);
        
        // Check if addresses are in the same or adjacent cache lines
        if (std::abs(static_cast<intptr_t>(curr_addr - prev_addr)) <= static_cast<intptr_t>(CACHE_LINE_SIZE)) {
            ++sequential_accesses;
        }
    }
    
    return static_cast<f64>(sequential_accesses) / (access_sequence.size() - 1);
}

usize SparseSetAnalyzer::calculate_cache_lines_used(void* start_addr, usize size) const noexcept {
    if (size == 0) return 0;
    
    uintptr_t start = reinterpret_cast<uintptr_t>(start_addr);
    uintptr_t end = start + size - 1;
    
    uintptr_t start_line = start / CACHE_LINE_SIZE;
    uintptr_t end_line = end / CACHE_LINE_SIZE;
    
    return static_cast<usize>(end_line - start_line + 1);
}

void SparseSetAnalyzer::update_performance_averages(SparseSetVisualizationData& data, 
                                                   f64 new_value, f64& average, u64& count) noexcept {
    // Exponential moving average for performance metrics
    if (count == 0) {
        average = new_value;
    } else {
        f64 alpha = std::min(0.1, 1.0 / count); // Decreasing learning rate
        average = average * (1.0 - alpha) + new_value * alpha;
    }
    ++count;
}

SparseSetAccessPattern SparseSetAnalyzer::detect_pattern_from_sequence(
    const std::vector<void*>& recent_accesses) const noexcept {
    
    if (recent_accesses.size() < 3) return SparseSetAccessPattern::Mixed;
    
    // Analyze address patterns
    bool is_sequential = true;
    bool is_random = true;
    
    for (usize i = 1; i < recent_accesses.size(); ++i) {
        uintptr_t prev = reinterpret_cast<uintptr_t>(recent_accesses[i-1]);
        uintptr_t curr = reinterpret_cast<uintptr_t>(recent_accesses[i]);
        
        intptr_t diff = static_cast<intptr_t>(curr - prev);
        
        if (std::abs(diff) > 1024) { // Not sequential if > 1KB apart
            is_sequential = false;
        }
        
        if (std::abs(diff) < 64) { // Not random if within cache line
            is_random = false;
        }
    }
    
    if (is_sequential) return SparseSetAccessPattern::Sequential;
    if (is_random) return SparseSetAccessPattern::Random;
    return SparseSetAccessPattern::Mixed;
}

f64 SparseSetAnalyzer::get_current_time() const noexcept {
    return core::get_time_seconds();
}

std::string SparseSetAnalyzer::format_memory_size(usize bytes) const noexcept {
    static const std::array<const char*, 5> units = {"B", "KB", "MB", "GB", "TB"};
    
    f64 size = static_cast<f64>(bytes);
    usize unit_index = 0;
    
    while (size >= 1024.0 && unit_index < units.size() - 1) {
        size /= 1024.0;
        ++unit_index;
    }
    
    std::ostringstream oss;
    oss.precision(2);
    oss << std::fixed << size << " " << units[unit_index];
    return oss.str();
}

std::string SparseSetAnalyzer::format_time_duration(f64 microseconds) const noexcept {
    if (microseconds < 1000.0) {
        return std::to_string(static_cast<int>(microseconds)) + " μs";
    } else if (microseconds < 1000000.0) {
        return std::to_string(static_cast<int>(microseconds / 1000.0)) + " ms";
    } else {
        return std::to_string(static_cast<int>(microseconds / 1000000.0)) + " s";
    }
}

void SparseSetAnalyzer::export_analysis_report(const std::string& filename) const noexcept {
    std::shared_lock<std::shared_mutex> lock(data_mutex_);
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open file for sparse set analysis export: " + filename);
        return;
    }
    
    file << "# Sparse Set Analysis Report\n";
    file << "Generated at: " << get_current_time() << "\n\n";
    
    file << "## Summary\n";
    file << "Total Sparse Sets: " << sparse_sets_.size() << "\n";
    file << "Total Memory Usage: " << format_memory_size(get_total_memory_usage()) << "\n";
    file << "Average Cache Locality: " << (get_average_cache_locality() * 100.0) << "%\n";
    file << "Overall Efficiency: " << (get_overall_efficiency() * 100.0) << "%\n\n";
    
    file << "## Individual Sparse Sets\n";
    for (const auto& [name, data] : sparse_sets_) {
        file << "### " << name << "\n";
        file << "- Dense Size: " << data->dense_size << " / " << data->dense_capacity << "\n";
        file << "- Memory Usage: " << format_memory_size(data->memory_total) << "\n";
        file << "- Memory Efficiency: " << (data->memory_efficiency * 100.0) << "%\n";
        file << "- Cache Locality Score: " << (data->cache_locality_score * 100.0) << "%\n";
        file << "- Average Insertion Time: " << format_time_duration(data->insertion_time_avg) << "\n";
        file << "- Average Lookup Time: " << format_time_duration(data->lookup_time_avg) << "\n";
        
        if (!data->optimization_suggestions.empty()) {
            file << "- Optimization Suggestions:\n";
            for (const auto& suggestion : data->optimization_suggestions) {
                file << "  - " << suggestion << "\n";
            }
        }
        file << "\n";
    }
    
    LOG_INFO("Sparse set analysis report exported to: " + filename);
}

} // namespace ecscope::visualization