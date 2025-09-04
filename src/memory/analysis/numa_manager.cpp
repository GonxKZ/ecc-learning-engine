/**
 * @file memory/numa_manager.cpp
 * @brief Implementation of NUMA-aware memory management system
 */

#include "numa_manager.hpp"
#include "core/log.hpp"
#include "core/profiler.hpp"
#include <cstring>
#include <sstream>
#include <algorithm>
#include <iomanip>

#ifdef __linux__
#include <unistd.h>
#include <sys/mman.h>
#endif

namespace ecscope::memory::numa {

//=============================================================================
// NumaDistanceMatrix Implementation
//=============================================================================

NumaDistanceMatrix::NumaDistanceMatrix(u32 node_count) 
    : node_count_(node_count) {
    distances_.resize(node_count);
    for (auto& row : distances_) {
        row.resize(node_count, 10); // Default distance
    }
    
    // Set diagonal to 10 (self-distance)
    for (u32 i = 0; i < node_count; ++i) {
        distances_[i][i] = 10;
    }
}

void NumaDistanceMatrix::set_distance(u32 from_node, u32 to_node, u32 distance) {
    if (from_node < node_count_ && to_node < node_count_) {
        distances_[from_node][to_node] = distance;
    }
}

u32 NumaDistanceMatrix::get_distance(u32 from_node, u32 to_node) const {
    if (from_node < node_count_ && to_node < node_count_) {
        return distances_[from_node][to_node];
    }
    return UINT32_MAX;
}

u32 NumaDistanceMatrix::find_closest_node(u32 from_node) const {
    if (from_node >= node_count_) return 0;
    
    u32 closest_node = 0;
    u32 min_distance = UINT32_MAX;
    
    for (u32 i = 0; i < node_count_; ++i) {
        if (i != from_node && distances_[from_node][i] < min_distance) {
            min_distance = distances_[from_node][i];
            closest_node = i;
        }
    }
    
    return closest_node;
}

std::vector<u32> NumaDistanceMatrix::get_nodes_by_distance(u32 from_node) const {
    if (from_node >= node_count_) return {};
    
    std::vector<std::pair<u32, u32>> node_distances;
    for (u32 i = 0; i < node_count_; ++i) {
        if (i != from_node) {
            node_distances.emplace_back(i, distances_[from_node][i]);
        }
    }
    
    std::sort(node_distances.begin(), node_distances.end(),
              [](const auto& a, const auto& b) { return a.second < b.second; });
    
    std::vector<u32> result;
    result.reserve(node_distances.size());
    for (const auto& [node, distance] : node_distances) {
        result.push_back(node);
    }
    
    return result;
}

f64 NumaDistanceMatrix::calculate_average_distance() const {
    f64 total = 0.0;
    u32 count = 0;
    
    for (u32 i = 0; i < node_count_; ++i) {
        for (u32 j = 0; j < node_count_; ++j) {
            if (i != j) {
                total += distances_[i][j];
                ++count;
            }
        }
    }
    
    return count > 0 ? total / count : 0.0;
}

f64 NumaDistanceMatrix::calculate_locality_score(u32 node) const {
    if (node >= node_count_) return 0.0;
    
    f64 total_distance = 0.0;
    for (u32 i = 0; i < node_count_; ++i) {
        if (i != node) {
            total_distance += distances_[node][i];
        }
    }
    
    // Lower total distance = better locality
    f64 max_possible_distance = (node_count_ - 1) * 255; // Max distance * other nodes
    return max_possible_distance > 0 ? 1.0 - (total_distance / max_possible_distance) : 1.0;
}

//=============================================================================
// NumaTopology Implementation
//=============================================================================

NumaTopology::NumaTopology() : distance_matrix(1) {
    total_nodes = 1;
    total_cpus = std::thread::hardware_concurrency();
    numa_available = false;
    
    // Initialize with single node fallback
    NumaNode default_node{};
    default_node.node_id = 0;
    default_node.total_memory_bytes = 8ULL * 1024 * 1024 * 1024; // 8GB estimate
    default_node.free_memory_bytes = default_node.total_memory_bytes;
    default_node.is_available = true;
    default_node.memory_bandwidth_gbps = 25.0; // DDR4-3200 estimate
    default_node.memory_latency_ns = 100.0;
    
    for (u32 i = 0; i < total_cpus; ++i) {
        default_node.cpu_cores.push_back(i);
        default_node.cpu_mask.set(i);
    }
    
    nodes.push_back(std::move(default_node));
    
    topology_description = "Single node fallback topology";
}

std::optional<u32> NumaTopology::get_current_node() const {
#ifdef __linux__
    if (numa_available) {
        int current_node = numa_node_of_cpu(sched_getcpu());
        if (current_node >= 0 && static_cast<u32>(current_node) < total_nodes) {
            return static_cast<u32>(current_node);
        }
    }
#endif
    return 0; // Fallback to node 0
}

std::optional<u32> NumaTopology::get_thread_node(std::thread::id thread_id) const {
    // Simplified implementation - would need thread tracking
    return get_current_node();
}

std::vector<u32> NumaTopology::get_available_nodes() const {
    std::vector<u32> available;
    for (const auto& node : nodes) {
        if (node.is_available) {
            available.push_back(node.node_id);
        }
    }
    return available;
}

NumaNode* NumaTopology::find_node(u32 node_id) {
    auto it = std::find_if(nodes.begin(), nodes.end(),
                          [node_id](const NumaNode& node) { return node.node_id == node_id; });
    return it != nodes.end() ? &(*it) : nullptr;
}

const NumaNode* NumaTopology::find_node(u32 node_id) const {
    auto it = std::find_if(nodes.begin(), nodes.end(),
                          [node_id](const NumaNode& node) { return node.node_id == node_id; });
    return it != nodes.end() ? &(*it) : nullptr;
}

f64 NumaTopology::calculate_cross_node_penalty(u32 from_node, u32 to_node) const {
    if (from_node == to_node) return 1.0; // No penalty for local access
    
    u32 distance = distance_matrix.get_distance(from_node, to_node);
    
    // Convert distance to penalty factor (higher distance = higher penalty)
    // Distance 10 (self) = 1.0x, distance 20 = 1.5x, distance 40 = 2.0x
    return 1.0 + (static_cast<f64>(distance - 10) / 30.0);
}

u32 NumaTopology::find_optimal_node_for_thread() const {
    auto current = get_current_node();
    if (current.has_value()) {
        return current.value();
    }
    
    // Find least loaded node
    u32 best_node = 0;
    f64 best_utilization = 1.0;
    
    for (const auto& node : nodes) {
        if (node.is_available && node.utilization_ratio < best_utilization) {
            best_utilization = node.utilization_ratio;
            best_node = node.node_id;
        }
    }
    
    return best_node;
}

std::string NumaTopology::generate_topology_report() const {
    std::ostringstream report;
    
    report << "=== NUMA Topology Report ===\n";
    report << "NUMA Available: " << (numa_available ? "Yes" : "No") << "\n";
    report << "Total Nodes: " << total_nodes << "\n";
    report << "Total CPUs: " << total_cpus << "\n";
    report << "Description: " << topology_description << "\n\n";
    
    for (const auto& node : nodes) {
        report << "Node " << node.node_id << ":\n";
        report << "  Available: " << (node.is_available ? "Yes" : "No") << "\n";
        report << "  Memory: " << (node.total_memory_bytes / (1024*1024*1024)) << " GB total, "
               << (node.free_memory_bytes / (1024*1024*1024)) << " GB free\n";
        report << "  Bandwidth: " << std::fixed << std::setprecision(1) 
               << node.memory_bandwidth_gbps << " GB/s\n";
        report << "  Latency: " << std::fixed << std::setprecision(1) 
               << node.memory_latency_ns << " ns\n";
        report << "  CPUs: " << node.cpu_cores.size() << " cores\n";
        report << "  Utilization: " << std::fixed << std::setprecision(2) 
               << (node.utilization_ratio * 100.0) << "%\n\n";
    }
    
    report << "Average Inter-Node Distance: " << std::fixed << std::setprecision(1)
           << distance_matrix.calculate_average_distance() << "\n";
    
    return report.str();
}

//=============================================================================
// SystemNumaAllocator Implementation
//=============================================================================

SystemNumaAllocator::SystemNumaAllocator(u32 node_id) 
    : node_id_(node_id) {
    LOG_DEBUG("Created system NUMA allocator for node {}", node_id);
}

SystemNumaAllocator::~SystemNumaAllocator() {
    usize leaked_bytes = allocated_bytes_.load();
    if (leaked_bytes > 0) {
        LOG_WARNING("NUMA allocator for node {} destroyed with {} bytes still allocated", 
                   node_id_, leaked_bytes);
    }
}

void* SystemNumaAllocator::allocate(usize size, const NumaAllocationConfig& config) {
    if (size == 0) return nullptr;
    
    void* ptr = nullptr;
    
#ifdef __linux__
    if (numa_available()) {
        // Try to allocate on specific node
        ptr = numa_alloc_onnode(size, node_id_);
        
        if (!ptr && config.policy == NumaAllocationPolicy::LocalPreferred) {
            // Fallback to any node
            ptr = numa_alloc(size);
        }
    } else 
#endif
    {
        // Fallback to regular allocation
        if (config.alignment_bytes > alignof(std::max_align_t)) {
            if (posix_memalign(&ptr, config.alignment_bytes, size) != 0) {
                ptr = nullptr;
            }
        } else {
            ptr = std::malloc(size);
        }
    }
    
    if (ptr) {
        std::lock_guard<std::mutex> lock(allocation_mutex_);
        allocation_sizes_[ptr] = size;
        allocated_bytes_.fetch_add(size, std::memory_order_relaxed);
    }
    
    return ptr;
}

void SystemNumaAllocator::deallocate(void* ptr, usize size) {
    if (!ptr) return;
    
    {
        std::lock_guard<std::mutex> lock(allocation_mutex_);
        auto it = allocation_sizes_.find(ptr);
        if (it != allocation_sizes_.end()) {
            allocated_bytes_.fetch_sub(it->second, std::memory_order_relaxed);
            allocation_sizes_.erase(it);
        }
    }
    
#ifdef __linux__
    if (numa_available()) {
        numa_free(ptr, size);
    } else 
#endif
    {
        std::free(ptr);
    }
}

bool SystemNumaAllocator::owns(const void* ptr) const {
    std::lock_guard<std::mutex> lock(allocation_mutex_);
    return allocation_sizes_.find(ptr) != allocation_sizes_.end();
}

std::optional<u32> SystemNumaAllocator::get_allocation_node(const void* ptr) const {
#ifdef __linux__
    if (numa_available() && ptr) {
        int node = numa_node_of_addr(const_cast<void*>(ptr));
        if (node >= 0) {
            return static_cast<u32>(node);
        }
    }
#endif
    return node_id_; // Fallback to our node
}

bool SystemNumaAllocator::migrate_to_node(void* ptr, usize size, u32 target_node) {
#ifdef __linux__
    if (numa_available() && ptr && size > 0) {
        unsigned long nodemask = 1UL << target_node;
        return numa_migrate_pages(getpid(), &nodemask, &nodemask) == 0;
    }
#endif
    return false;
}

bool SystemNumaAllocator::bind_to_node(void* ptr, usize size, u32 node_id) {
#ifdef __linux__
    if (numa_available() && ptr && size > 0) {
        unsigned long nodemask = 1UL << node_id;
        return mbind(ptr, size, MPOL_BIND, &nodemask, sizeof(nodemask) * 8, 0) == 0;
    }
#endif
    return false;
}

std::unordered_map<u32, usize> SystemNumaAllocator::get_allocation_stats() const {
    std::unordered_map<u32, usize> stats;
    stats[node_id_] = allocated_bytes_.load();
    return stats;
}

f64 SystemNumaAllocator::get_cross_node_access_ratio() const {
    // Simplified - would need memory access tracking
    return 0.1; // Assume 10% cross-node access
}

std::string SystemNumaAllocator::get_allocation_report() const {
    std::ostringstream report;
    report << "SystemNumaAllocator Node " << node_id_ << ":\n";
    report << "  Allocated: " << (allocated_bytes_.load() / 1024) << " KB\n";
    report << "  Active Allocations: " << allocation_sizes_.size() << "\n";
    return report.str();
}

//=============================================================================
// NumaManager Implementation
//=============================================================================

NumaManager::NumaManager() 
    : enable_automatic_migration_(false),
      migration_threshold_ratio_(0.3),
      migration_check_interval_ms_(1000) {
}

NumaManager::~NumaManager() {
    shutdown();
}

bool NumaManager::initialize() {
    PROFILE_FUNCTION();
    
    bool success = discover_numa_topology();
    if (success) {
        initialize_node_allocators();
        setup_performance_monitoring();
        LOG_INFO("NUMA manager initialized successfully with {} nodes", topology_.total_nodes);
    } else {
        LOG_WARNING("NUMA initialization failed, using fallback single-node configuration");
    }
    
    return success;
}

void NumaManager::shutdown() {
    numa_balancing_enabled_.store(false);
    
    // Clear allocators
    node_allocators_.clear();
    
    // Clear tracking
    std::unique_lock<std::shared_mutex> lock(tracking_mutex_);
    allocation_tracking_.clear();
}

void NumaManager::refresh_topology() {
    PROFILE_FUNCTION();
    discover_numa_topology();
}

void* NumaManager::allocate(usize size, const NumaAllocationConfig& config) {
    if (size == 0) return nullptr;
    
    u32 target_node = select_optimal_node(config);
    
    if (target_node < node_allocators_.size() && node_allocators_[target_node]) {
        void* ptr = node_allocators_[target_node]->allocate(size, config);
        if (ptr) {
            record_allocation(ptr, size, target_node, config);
            per_node_stats_[target_node].local_allocations.fetch_add(1, std::memory_order_relaxed);
            per_node_stats_[target_node].allocated_bytes.fetch_add(size, std::memory_order_relaxed);
            return ptr;
        }
    }
    
    // Fallback to any available node
    for (u32 node = 0; node < node_allocators_.size(); ++node) {
        if (node != target_node && node_allocators_[node]) {
            void* ptr = node_allocators_[node]->allocate(size, config);
            if (ptr) {
                record_allocation(ptr, size, node, config);
                per_node_stats_[node].remote_allocations.fetch_add(1, std::memory_order_relaxed);
                per_node_stats_[node].allocated_bytes.fetch_add(size, std::memory_order_relaxed);
                return ptr;
            }
        }
    }
    
    return nullptr;
}

void* NumaManager::allocate_on_node(usize size, u32 node_id) {
    NumaAllocationConfig config;
    config.policy = NumaAllocationPolicy::Bind;
    config.preferred_node = node_id;
    return allocate(size, config);
}

void* NumaManager::allocate_interleaved(usize size, const std::vector<u32>& nodes) {
    NumaAllocationConfig config;
    config.policy = NumaAllocationPolicy::Interleave;
    config.allowed_nodes = nodes.empty() ? topology_.get_available_nodes() : nodes;
    return allocate(size, config);
}

void NumaManager::deallocate(void* ptr, usize size) {
    if (!ptr) return;
    
    // Find which allocator owns this pointer
    for (auto& allocator : node_allocators_) {
        if (allocator && allocator->owns(ptr)) {
            allocator->deallocate(ptr, size);
            record_deallocation(ptr);
            return;
        }
    }
    
    // Fallback
    std::free(ptr);
    record_deallocation(ptr);
}

bool NumaManager::migrate_memory(void* ptr, usize size, u32 target_node) {
    if (!ptr || size == 0 || target_node >= node_allocators_.size()) {
        return false;
    }
    
    auto current_node = get_memory_node(ptr);
    if (!current_node.has_value() || current_node.value() == target_node) {
        return true; // Already on target node or unknown
    }
    
    u32 source_node = current_node.value();
    if (source_node < node_allocators_.size() && node_allocators_[source_node]) {
        bool success = node_allocators_[source_node]->migrate_to_node(ptr, size, target_node);
        if (success) {
            per_node_stats_[source_node].migration_events.fetch_add(1, std::memory_order_relaxed);
        }
        return success;
    }
    
    return false;
}

bool NumaManager::bind_memory(void* ptr, usize size, u32 node_id) {
    if (!ptr || size == 0 || node_id >= node_allocators_.size()) {
        return false;
    }
    
    if (node_allocators_[node_id]) {
        return node_allocators_[node_id]->bind_to_node(ptr, size, node_id);
    }
    
    return false;
}

bool NumaManager::is_memory_local(const void* ptr) const {
    auto current_thread_node = get_current_thread_node();
    auto memory_node = get_memory_node(ptr);
    
    return current_thread_node.has_value() && memory_node.has_value() &&
           current_thread_node.value() == memory_node.value();
}

std::optional<u32> NumaManager::get_memory_node(const void* ptr) const {
    if (!ptr) return std::nullopt;
    
    for (u32 i = 0; i < node_allocators_.size(); ++i) {
        if (node_allocators_[i] && node_allocators_[i]->owns(ptr)) {
            return node_allocators_[i]->get_allocation_node(ptr);
        }
    }
    
    return std::nullopt;
}

bool NumaManager::set_thread_affinity(std::thread::id thread_id, u32 node_id) {
    if (node_id >= topology_.total_nodes) return false;
    
    std::unique_lock<std::shared_mutex> lock(affinity_mutex_);
    thread_node_affinity_[thread_id] = node_id;
    
#ifdef __linux__
    // Set CPU affinity to match NUMA node (simplified)
    auto* node = topology_.find_node(node_id);
    if (node && !node->cpu_cores.empty()) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        for (u32 cpu : node->cpu_cores) {
            if (cpu < CPU_SETSIZE) {
                CPU_SET(cpu, &cpuset);
            }
        }
        
        // Would need thread handle to set affinity properly
        // This is a simplified placeholder
    }
#endif
    
    return true;
}

bool NumaManager::set_current_thread_affinity(u32 node_id) {
    return set_thread_affinity(std::this_thread::get_id(), node_id);
}

std::optional<u32> NumaManager::get_thread_affinity(std::thread::id thread_id) const {
    std::shared_lock<std::shared_mutex> lock(affinity_mutex_);
    auto it = thread_node_affinity_.find(thread_id);
    return it != thread_node_affinity_.end() ? std::optional<u32>(it->second) : std::nullopt;
}

std::optional<u32> NumaManager::get_current_thread_node() const {
    // First check explicit affinity
    auto affinity = get_thread_affinity(std::this_thread::get_id());
    if (affinity.has_value()) {
        return affinity;
    }
    
    // Fallback to topology detection
    return topology_.get_current_node();
}

void NumaManager::trigger_memory_balancing() {
    if (!numa_balancing_enabled_.load()) return;
    
    PROFILE_FUNCTION();
    run_memory_balancing_worker();
}

NumaManager::PerformanceMetrics NumaManager::get_performance_metrics() const {
    PerformanceMetrics metrics{};
    
    u64 total_local = 0;
    u64 total_remote = 0;
    u64 total_migrations = 0;
    usize total_allocated = 0;
    
    for (u32 i = 0; i < active_node_count_.load(); ++i) {
        const auto& stats = per_node_stats_[i];
        u64 local = stats.local_allocations.load();
        u64 remote = stats.remote_allocations.load();
        
        total_local += local;
        total_remote += remote;
        total_migrations += stats.migration_events.load();
        total_allocated += stats.allocated_bytes.load();
        
        if (local + remote > 0) {
            metrics.node_utilization[i] = static_cast<f64>(local) / (local + remote);
        }
    }
    
    metrics.total_allocations = total_local + total_remote;
    metrics.total_migrations = total_migrations;
    
    if (metrics.total_allocations > 0) {
        metrics.local_access_ratio = static_cast<f64>(total_local) / metrics.total_allocations;
    }
    
    metrics.cross_node_penalty_factor = 1.5; // Estimated
    metrics.memory_bandwidth_utilization = 0.6; // Estimated
    metrics.average_allocation_latency_ns = 150.0; // Estimated
    
    return metrics;
}

void NumaManager::reset_statistics() {
    for (u32 i = 0; i < active_node_count_.load(); ++i) {
        auto& stats = per_node_stats_[i];
        stats.local_allocations.store(0);
        stats.remote_allocations.store(0);
        stats.cross_node_accesses.store(0);
        stats.migration_events.store(0);
        stats.allocated_bytes.store(0);
        stats.average_access_latency.store(0.0);
    }
    measurement_counter_.store(0);
}

std::string NumaManager::generate_performance_report() const {
    auto metrics = get_performance_metrics();
    std::ostringstream report;
    
    report << "=== NUMA Performance Report ===\n";
    report << "Total Allocations: " << metrics.total_allocations << "\n";
    report << "Local Access Ratio: " << std::fixed << std::setprecision(2) 
           << (metrics.local_access_ratio * 100.0) << "%\n";
    report << "Cross-Node Penalty: " << std::fixed << std::setprecision(1)
           << metrics.cross_node_penalty_factor << "x\n";
    report << "Total Migrations: " << metrics.total_migrations << "\n";
    report << "Average Allocation Latency: " << std::fixed << std::setprecision(1)
           << metrics.average_allocation_latency_ns << " ns\n\n";
    
    report << "Per-Node Utilization:\n";
    for (const auto& [node_id, utilization] : metrics.node_utilization) {
        report << "  Node " << node_id << ": " << std::fixed << std::setprecision(1)
               << (utilization * 100.0) << "%\n";
    }
    
    return report.str();
}

// Private implementation methods
bool NumaManager::discover_numa_topology() {
    bool numa_detected = false;
    
#ifdef __linux__
    numa_detected = linux_discover_topology();
#endif
    
    if (!numa_detected) {
        // Use fallback topology (already initialized in constructor)
        LOG_INFO("Using fallback single-node topology");
    }
    
    active_node_count_.store(topology_.total_nodes);
    return true;
}

void NumaManager::initialize_node_allocators() {
    node_allocators_.clear();
    node_allocators_.reserve(topology_.total_nodes);
    
    for (u32 i = 0; i < topology_.total_nodes; ++i) {
        node_allocators_.push_back(std::make_unique<SystemNumaAllocator>(i));
    }
    
    LOG_DEBUG("Initialized {} NUMA node allocators", topology_.total_nodes);
}

void NumaManager::setup_performance_monitoring() {
    // Reset all performance counters
    for (u32 i = 0; i < 64; ++i) {
        per_node_stats_[i] = NumaStats{};
    }
}

u32 NumaManager::select_optimal_node(const NumaAllocationConfig& config) const {
    switch (config.policy) {
        case NumaAllocationPolicy::Bind:
            return config.preferred_node < topology_.total_nodes ? config.preferred_node : 0;
            
        case NumaAllocationPolicy::LocalOnly:
        case NumaAllocationPolicy::LocalPreferred:
        case NumaAllocationPolicy::Default: {
            auto current = get_current_thread_node();
            return current.value_or(0);
        }
        
        case NumaAllocationPolicy::RoundRobin: {
            static std::atomic<u32> counter{0};
            return counter.fetch_add(1) % topology_.total_nodes;
        }
        
        case NumaAllocationPolicy::Interleave:
        case NumaAllocationPolicy::InterleaveSubset: {
            const auto& allowed = config.allowed_nodes.empty() ? 
                                 topology_.get_available_nodes() : config.allowed_nodes;
            if (!allowed.empty()) {
                static std::atomic<u32> counter{0};
                return allowed[counter.fetch_add(1) % allowed.size()];
            }
            return 0;
        }
        
        case NumaAllocationPolicy::FirstTouch:
        default:
            return topology_.find_optimal_node_for_thread();
    }
}

void* NumaManager::allocate_with_policy(usize size, const NumaAllocationConfig& config) {
    u32 target_node = select_optimal_node(config);
    
    if (target_node < node_allocators_.size() && node_allocators_[target_node]) {
        return node_allocators_[target_node]->allocate(size, config);
    }
    
    return nullptr;
}

void NumaManager::record_allocation(const void* ptr, usize size, u32 node_id, 
                                  const NumaAllocationConfig& config) {
    std::unique_lock<std::shared_mutex> lock(tracking_mutex_);
    
    AllocationInfo info{};
    info.node_id = node_id;
    info.size = size;
    info.allocating_thread = std::this_thread::get_id();
    info.allocation_time = std::chrono::steady_clock::now();
    info.policy_used = config.policy;
    
    allocation_tracking_[ptr] = std::move(info);
}

void NumaManager::record_deallocation(const void* ptr) {
    std::unique_lock<std::shared_mutex> lock(tracking_mutex_);
    allocation_tracking_.erase(ptr);
}

void NumaManager::update_performance_counters(u32 node_id, bool is_local_access) {
    if (node_id >= 64) return;
    
    auto& stats = per_node_stats_[node_id];
    if (is_local_access) {
        stats.local_allocations.fetch_add(1, std::memory_order_relaxed);
    } else {
        stats.cross_node_accesses.fetch_add(1, std::memory_order_relaxed);
    }
}

void NumaManager::run_memory_balancing_worker() {
    // Simplified memory balancing - would analyze access patterns and migrate hot data
    LOG_DEBUG("Running memory balancing analysis");
    
    std::shared_lock<std::shared_mutex> lock(tracking_mutex_);
    usize migration_candidates = 0;
    
    for (const auto& [ptr, info] : allocation_tracking_) {
        // Check if allocation is being accessed from different node
        auto current_node = get_current_thread_node();
        if (current_node.has_value() && current_node.value() != info.node_id) {
            ++migration_candidates;
        }
    }
    
    if (migration_candidates > 0) {
        LOG_DEBUG("Found {} potential migration candidates", migration_candidates);
    }
}

#ifdef __linux__
bool NumaManager::linux_discover_topology() {
    if (numa_available() == -1) {
        return false;
    }
    
    int max_nodes = numa_max_node() + 1;
    if (max_nodes <= 0) return false;
    
    topology_.total_nodes = static_cast<u32>(max_nodes);
    topology_.numa_available = true;
    topology_.nodes.clear();
    topology_.distance_matrix = NumaDistanceMatrix(topology_.total_nodes);
    
    for (int node_id = 0; node_id < max_nodes; ++node_id) {
        if (numa_bitmask_isbitset(numa_get_mems_allowed(), node_id)) {
            NumaNode node{};
            node.node_id = static_cast<u32>(node_id);
            node.is_available = true;
            
            // Get memory information
            long long node_size = numa_node_size64(node_id, &node.free_memory_bytes);
            node.total_memory_bytes = node_size >= 0 ? static_cast<usize>(node_size) : 0;
            
            // Get CPU information
            struct bitmask* cpu_mask = numa_allocate_cpumask();
            if (numa_node_to_cpus(node_id, cpu_mask) == 0) {
                for (unsigned int cpu = 0; cpu < cpu_mask->size; ++cpu) {
                    if (numa_bitmask_isbitset(cpu_mask, cpu)) {
                        node.cpu_cores.push_back(cpu);
                        node.cpu_mask.set(cpu);
                    }
                }
            }
            numa_free_cpumask(cpu_mask);
            
            // Estimate bandwidth and latency (would need actual measurement)
            node.memory_bandwidth_gbps = 25.0; // DDR4 estimate
            node.memory_latency_ns = 100.0;
            
            topology_.nodes.push_back(std::move(node));
        }
    }
    
    // Build distance matrix
    for (u32 i = 0; i < topology_.total_nodes; ++i) {
        for (u32 j = 0; j < topology_.total_nodes; ++j) {
            int distance = numa_distance(i, j);
            topology_.distance_matrix.set_distance(i, j, static_cast<u32>(distance));
        }
    }
    
    topology_.topology_description = "Linux NUMA topology with " + 
                                   std::to_string(topology_.total_nodes) + " nodes";
    
    LOG_INFO("Discovered Linux NUMA topology: {} nodes, {} total CPUs", 
             topology_.total_nodes, topology_.total_cpus);
    
    return true;
}

void* NumaManager::linux_allocate_on_node(usize size, u32 node_id) {
    if (numa_available() == -1) return nullptr;
    
    return numa_alloc_onnode(size, static_cast<int>(node_id));
}

bool NumaManager::linux_migrate_pages(void* ptr, usize size, u32 target_node) {
    if (numa_available() == -1 || !ptr || size == 0) return false;
    
    unsigned long nodemask = 1UL << target_node;
    return numa_migrate_pages(getpid(), &nodemask, &nodemask) == 0;
}

bool NumaManager::linux_set_thread_affinity(std::thread::id thread_id, u32 node_id) {
    // Would need proper thread handle implementation
    return false;
}
#endif

// Educational demonstration methods
void NumaManager::print_numa_topology() const {
    std::cout << topology_.generate_topology_report();
}

void NumaManager::visualize_memory_distribution() const {
    std::cout << "\n=== Memory Distribution Visualization ===\n";
    
    usize total_memory = 0;
    for (const auto& node : topology_.nodes) {
        total_memory += node.total_memory_bytes;
    }
    
    for (const auto& node : topology_.nodes) {
        f64 percentage = total_memory > 0 ? 
                        (static_cast<f64>(node.total_memory_bytes) / total_memory) * 100.0 : 0.0;
        
        std::cout << "Node " << node.node_id << ": ";
        
        // Simple bar chart
        int bar_length = static_cast<int>(percentage / 2.0); // Scale to fit
        for (int i = 0; i < bar_length; ++i) {
            std::cout << "█";
        }
        
        std::cout << " " << std::fixed << std::setprecision(1) << percentage << "%"
                  << " (" << (node.total_memory_bytes / (1024*1024*1024)) << " GB)\n";
    }
}

void NumaManager::demonstrate_numa_effects() const {
    std::cout << "\n=== NUMA Effects Demonstration ===\n";
    
    auto metrics = get_performance_metrics();
    
    std::cout << "Local vs Remote Access Impact:\n";
    std::cout << "  Local access ratio: " << (metrics.local_access_ratio * 100.0) << "%\n";
    std::cout << "  Cross-node penalty: " << metrics.cross_node_penalty_factor << "x slower\n";
    
    if (metrics.local_access_ratio < 0.8) {
        std::cout << "  ⚠️  High cross-node access detected! Consider:\n";
        std::cout << "     - Setting thread affinity\n";
        std::cout << "     - Using NUMA-aware allocation\n";
        std::cout << "     - Migrating frequently accessed data\n";
    } else {
        std::cout << "  ✅ Good NUMA locality\n";
    }
}

f64 NumaManager::measure_memory_bandwidth(u32 node_id, usize buffer_size_mb) {
    if (node_id >= topology_.total_nodes) return 0.0;
    
    PROFILE_FUNCTION();
    
    usize buffer_size = buffer_size_mb * 1024 * 1024;
    void* buffer = allocate_on_node(buffer_size, node_id);
    if (!buffer) return 0.0;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Simple memory bandwidth test (read + write)
    volatile char* ptr = static_cast<char*>(buffer);
    for (usize i = 0; i < buffer_size; i += 64) { // Cache line sized strides
        ptr[i] = static_cast<char>(i & 0xFF);
    }
    
    // Read pass
    volatile char sum = 0;
    for (usize i = 0; i < buffer_size; i += 64) {
        sum += ptr[i];
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<f64>(end_time - start_time).count();
    
    deallocate(buffer, buffer_size);
    
    // Calculate bandwidth in GB/s (2 passes: write + read)
    f64 bytes_processed = buffer_size * 2;
    f64 bandwidth_gbps = duration > 0.0 ? (bytes_processed / (1024*1024*1024)) / duration : 0.0;
    
    LOG_DEBUG("Measured bandwidth for node {}: {:.2f} GB/s", node_id, bandwidth_gbps);
    
    return bandwidth_gbps;
}

f64 NumaManager::measure_cross_node_latency(u32 from_node, u32 to_node) {
    if (from_node >= topology_.total_nodes || to_node >= topology_.total_nodes) {
        return 0.0;
    }
    
    // Simplified latency measurement
    // Would need more sophisticated cache-avoiding patterns
    
    constexpr usize test_size = 4096; // Single page
    void* buffer = allocate_on_node(test_size, to_node);
    if (!buffer) return 0.0;
    
    // Set thread affinity to from_node
    set_current_thread_affinity(from_node);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Random access pattern to avoid prefetching
    volatile int* ptr = static_cast<int*>(buffer);
    volatile int sum = 0;
    
    for (int i = 0; i < 1000; ++i) {
        sum += ptr[(i * 73) % (test_size / sizeof(int))];
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration_ns = std::chrono::duration<f64, std::nano>(end_time - start_time).count();
    
    deallocate(buffer, test_size);
    
    f64 latency_per_access = duration_ns / 1000.0;
    
    LOG_DEBUG("Measured latency from node {} to node {}: {:.1f} ns", 
             from_node, to_node, latency_per_access);
    
    return latency_per_access;
}

std::unordered_map<u32, f64> NumaManager::benchmark_all_nodes() {
    std::unordered_map<u32, f64> results;
    
    LOG_INFO("Benchmarking memory bandwidth for all NUMA nodes...");
    
    for (u32 node = 0; node < topology_.total_nodes; ++node) {
        f64 bandwidth = measure_memory_bandwidth(node, 100); // 100MB test
        results[node] = bandwidth;
        
        // Update topology with measured bandwidth
        auto* node_info = topology_.find_node(node);
        if (node_info) {
            node_info->memory_bandwidth_gbps = bandwidth;
        }
    }
    
    return results;
}

// Global NUMA manager instance
NumaManager& get_global_numa_manager() {
    static NumaManager instance;
    static std::once_flag init_flag;
    
    std::call_once(init_flag, []() {
        instance.initialize();
    });
    
    return instance;
}

} // namespace ecscope::memory::numa