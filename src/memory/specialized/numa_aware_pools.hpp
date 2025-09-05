#pragma once

/**
 * @file memory/specialized/numa_aware_pools.hpp
 * @brief NUMA-Aware Memory Pools for Multi-Socket System Optimization
 * 
 * This header implements comprehensive NUMA-aware memory pools that optimize
 * memory allocation and access patterns across multiple NUMA nodes. It provides
 * educational insights into NUMA topology, memory affinity, and cross-node
 * memory migration strategies.
 * 
 * Key Features:
 * - NUMA topology detection and node affinity management
 * - Per-NUMA node memory pool allocation and management
 * - Thread affinity-based pool selection and optimization
 * - Cross-NUMA node memory migration capabilities
 * - NUMA distance-aware allocation strategies
 * - Memory bandwidth optimization per NUMA node
 * - Educational NUMA topology visualization and analysis
 * - Integration with thermal management and hot/cold separation
 * 
 * Educational Value:
 * - Demonstrates NUMA architecture principles and challenges
 * - Shows memory affinity optimization techniques
 * - Illustrates cross-node memory migration strategies
 * - Provides insights into memory bandwidth and latency optimization
 * - Educational examples of NUMA-aware application design
 * 
 * @author ECScope Educational ECS Framework - NUMA Memory Management
 * @date 2025
 */

#include "memory/pools/hierarchical_pools.hpp"
#include "memory/memory_tracker.hpp"
#include "memory/analysis/numa_manager.hpp"
#include "memory/specialized/thermal_pools.hpp"
#include "core/types.hpp"
#include "core/log.hpp"
#include <atomic>
#include <memory>
#include <vector>
#include <array>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <chrono>
#include <algorithm>
#include <span>
#include <optional>

namespace ecscope::memory::specialized::numa {

//=============================================================================
// NUMA Node Information and Topology
//=============================================================================

/**
 * @brief NUMA node information and characteristics
 */
struct NumaNodeInfo {
    u32 node_id;                        // NUMA node identifier
    u64 total_memory_bytes;             // Total memory available on this node
    u64 free_memory_bytes;              // Currently free memory on this node
    u64 allocated_bytes;                // Memory allocated by our pools
    
    // Performance characteristics
    f64 memory_bandwidth_gbps;          // Node's memory bandwidth
    f64 local_access_latency_ns;        // Local memory access latency
    std::vector<f64> remote_latencies;   // Latencies to other nodes
    
    // CPU affinity information
    std::vector<u32> cpu_cores;         // CPU cores on this node
    std::vector<std::thread::id> active_threads; // Threads currently using this node
    
    // Pool management
    bool is_available;                   // Node is available for allocation
    f64 current_utilization;            // Current utilization ratio (0.0-1.0)
    f64 thermal_throttling_factor;      // Thermal throttling factor (1.0 = no throttling)
    
    NumaNodeInfo(u32 id = 0) noexcept 
        : node_id(id), total_memory_bytes(0), free_memory_bytes(0), allocated_bytes(0),
          memory_bandwidth_gbps(100.0), local_access_latency_ns(80.0),
          is_available(true), current_utilization(0.0), thermal_throttling_factor(1.0) {}
};

/**
 * @brief NUMA topology manager and optimizer
 */
class NumaTopologyManager {
private:
    std::vector<NumaNodeInfo> numa_nodes_;
    std::vector<std::vector<f64>> distance_matrix_; // [from_node][to_node] = distance
    std::unordered_map<std::thread::id, u32> thread_node_affinity_;
    
    mutable std::shared_mutex topology_mutex_;
    std::atomic<bool> topology_initialized_{false};
    std::atomic<u32> preferred_node_hint_{0};
    
public:
    NumaTopologyManager() {
        initialize_topology();
    }
    
    /**
     * @brief Detect and initialize NUMA topology
     */
    void initialize_topology() {
        std::unique_lock<std::shared_mutex> lock(topology_mutex_);
        
        u32 numa_node_count = detect_numa_nodes();
        numa_nodes_.resize(numa_node_count);
        distance_matrix_.resize(numa_node_count);
        
        for (u32 i = 0; i < numa_node_count; ++i) {
            numa_nodes_[i] = NumaNodeInfo(i);
            distance_matrix_[i].resize(numa_node_count);
            
            // Initialize node characteristics (in real implementation, query system)
            initialize_node_info(i);
            initialize_distance_matrix(i);
        }
        
        topology_initialized_.store(true);
        
        LOG_INFO("Initialized NUMA topology: {} nodes detected", numa_node_count);
        log_topology_information();
    }
    
    /**
     * @brief Get optimal NUMA node for allocation
     */
    u32 get_optimal_node_for_thread(std::thread::id thread_id) const {
        std::shared_lock<std::shared_mutex> lock(topology_mutex_);
        
        // Check if thread has established affinity
        auto it = thread_node_affinity_.find(thread_id);
        if (it != thread_node_affinity_.end()) {
            u32 preferred_node = it->second;
            if (preferred_node < numa_nodes_.size() && numa_nodes_[preferred_node].is_available) {
                return preferred_node;
            }
        }
        
        // Find optimal node based on current utilization and availability
        return find_least_utilized_node();
    }
    
    /**
     * @brief Set thread affinity to specific NUMA node
     */
    void set_thread_affinity(std::thread::id thread_id, u32 node_id) {
        std::unique_lock<std::shared_mutex> lock(topology_mutex_);
        
        if (node_id >= numa_nodes_.size()) {
            LOG_WARNING("Invalid NUMA node ID: {}", node_id);
            return;
        }
        
        thread_node_affinity_[thread_id] = node_id;
        numa_nodes_[node_id].active_threads.push_back(thread_id);
        
        LOG_DEBUG("Set thread affinity: thread={} -> node={}", 
                 reinterpret_cast<uintptr_t>(&thread_id), node_id);
    }
    
    /**
     * @brief Get memory access cost between two nodes
     */
    f64 get_access_cost(u32 from_node, u32 to_node) const {
        std::shared_lock<std::shared_mutex> lock(topology_mutex_);
        
        if (from_node >= numa_nodes_.size() || to_node >= numa_nodes_.size()) {
            return 1000.0; // High cost for invalid nodes
        }
        
        if (from_node == to_node) {
            return numa_nodes_[from_node].local_access_latency_ns;
        }
        
        return distance_matrix_[from_node][to_node];
    }
    
    /**
     * @brief Update node utilization statistics
     */
    void update_node_utilization(u32 node_id, u64 allocated_bytes, u64 freed_bytes) {
        std::unique_lock<std::shared_mutex> lock(topology_mutex_);
        
        if (node_id >= numa_nodes_.size()) return;
        
        auto& node = numa_nodes_[node_id];
        node.allocated_bytes += allocated_bytes;
        node.allocated_bytes = (node.allocated_bytes > freed_bytes) ? 
                              (node.allocated_bytes - freed_bytes) : 0;
        
        if (node.total_memory_bytes > 0) {
            node.current_utilization = static_cast<f64>(node.allocated_bytes) / 
                                      node.total_memory_bytes;
        }
    }
    
    /**
     * @brief Get comprehensive topology statistics
     */
    struct TopologyStatistics {
        u32 total_nodes;
        std::vector<NumaNodeInfo> nodes;
        std::vector<std::vector<f64>> distance_matrix;
        f64 average_utilization;
        u32 most_utilized_node;
        u32 least_utilized_node;
        f64 cross_node_access_ratio;
        std::vector<std::pair<std::thread::id, u32>> thread_affinities;
    };
    
    TopologyStatistics get_statistics() const {
        std::shared_lock<std::shared_mutex> lock(topology_mutex_);
        
        TopologyStatistics stats{};
        stats.total_nodes = static_cast<u32>(numa_nodes_.size());
        stats.nodes = numa_nodes_;
        stats.distance_matrix = distance_matrix_;
        
        // Calculate statistics
        f64 total_utilization = 0.0;
        f64 max_utilization = 0.0;
        f64 min_utilization = 1.0;
        u32 max_node = 0, min_node = 0;
        
        for (u32 i = 0; i < numa_nodes_.size(); ++i) {
            f64 util = numa_nodes_[i].current_utilization;
            total_utilization += util;
            
            if (util > max_utilization) {
                max_utilization = util;
                max_node = i;
            }
            if (util < min_utilization) {
                min_utilization = util;
                min_node = i;
            }
        }
        
        stats.average_utilization = stats.total_nodes > 0 ? 
                                   total_utilization / stats.total_nodes : 0.0;
        stats.most_utilized_node = max_node;
        stats.least_utilized_node = min_node;
        
        // Copy thread affinities
        for (const auto& [thread_id, node_id] : thread_node_affinity_) {
            stats.thread_affinities.emplace_back(thread_id, node_id);
        }
        
        return stats;
    }
    
    const std::vector<NumaNodeInfo>& get_nodes() const { return numa_nodes_; }
    bool is_initialized() const { return topology_initialized_.load(); }
    u32 get_node_count() const { return static_cast<u32>(numa_nodes_.size()); }
    
private:
    u32 detect_numa_nodes() {
        // In a real implementation, this would query the system
        // For educational purposes, simulate based on hardware
        u32 cpu_count = std::thread::hardware_concurrency();
        
        if (cpu_count >= 32) return 4;      // High-end server
        if (cpu_count >= 16) return 2;      // Dual-socket workstation
        return 1;                           // Single socket system
    }
    
    void initialize_node_info(u32 node_id) {
        auto& node = numa_nodes_[node_id];
        
        // Simulate node characteristics (in real implementation, query system)
        node.total_memory_bytes = 32ULL * 1024 * 1024 * 1024; // 32GB per node
        node.free_memory_bytes = node.total_memory_bytes;
        node.memory_bandwidth_gbps = 100.0 - (node_id * 10.0); // Slight variation
        node.local_access_latency_ns = 80.0 + (node_id * 5.0);
        
        // Assign CPU cores to nodes
        u32 cores_per_node = std::thread::hardware_concurrency() / numa_nodes_.size();
        for (u32 core = node_id * cores_per_node; 
             core < (node_id + 1) * cores_per_node && core < std::thread::hardware_concurrency(); 
             ++core) {
            node.cpu_cores.push_back(core);
        }
    }
    
    void initialize_distance_matrix(u32 node_id) {
        for (u32 j = 0; j < numa_nodes_.size(); ++j) {
            if (node_id == j) {
                distance_matrix_[node_id][j] = numa_nodes_[node_id].local_access_latency_ns;
            } else {
                // Simulate cross-node latency (typically 2-3x local latency)
                f64 cross_node_penalty = 2.0 + std::abs(static_cast<i32>(node_id) - static_cast<i32>(j)) * 0.5;
                distance_matrix_[node_id][j] = numa_nodes_[node_id].local_access_latency_ns * cross_node_penalty;
            }
        }
    }
    
    u32 find_least_utilized_node() const {
        u32 best_node = 0;
        f64 lowest_utilization = 1.0;
        
        for (u32 i = 0; i < numa_nodes_.size(); ++i) {
            if (!numa_nodes_[i].is_available) continue;
            
            f64 adjusted_utilization = numa_nodes_[i].current_utilization / 
                                     numa_nodes_[i].thermal_throttling_factor;
            
            if (adjusted_utilization < lowest_utilization) {
                lowest_utilization = adjusted_utilization;
                best_node = i;
            }
        }
        
        return best_node;
    }
    
    void log_topology_information() const {
        for (u32 i = 0; i < numa_nodes_.size(); ++i) {
            const auto& node = numa_nodes_[i];
            LOG_INFO("NUMA Node {}: {}GB memory, {:.1f}GB/s bandwidth, {:.1f}ns latency, {} CPU cores", 
                     i, node.total_memory_bytes / (1024*1024*1024), 
                     node.memory_bandwidth_gbps, node.local_access_latency_ns, 
                     node.cpu_cores.size());
        }
    }
};

//=============================================================================
// NUMA-Aware Memory Pool
//=============================================================================

/**
 * @brief NUMA-aware memory pool that optimizes allocation across nodes
 */
class NumaAwarePool {
private:
    struct NodePool {
        u32 node_id;
        void* memory_base;
        usize total_size;
        usize allocated_size;
        std::unique_ptr<memory::ArenaAllocator> arena;
        std::unique_ptr<thermal::ThermalPool> thermal_pool;
        
        // Performance tracking
        alignas(core::CACHE_LINE_SIZE) std::atomic<u64> allocations{0};
        alignas(core::CACHE_LINE_SIZE) std::atomic<u64> deallocations{0};
        alignas(core::CACHE_LINE_SIZE) std::atomic<u64> migrations_in{0};
        alignas(core::CACHE_LINE_SIZE) std::atomic<u64> migrations_out{0};
        
        mutable std::shared_mutex pool_mutex;
        
        NodePool(u32 id, usize size) : node_id(id), memory_base(nullptr), 
                                      total_size(size), allocated_size(0) {}
    };
    
    std::vector<std::unique_ptr<NodePool>> node_pools_;
    NumaTopologyManager& topology_manager_;
    memory::MemoryTracker* memory_tracker_;
    
    // Migration management
    struct MigrationTask {
        void* source_address;
        usize size;
        u32 source_node;
        u32 target_node;
        f64 priority;
        std::chrono::steady_clock::time_point creation_time;
    };
    
    std::vector<MigrationTask> pending_migrations_;
    std::thread migration_thread_;
    std::atomic<bool> migration_enabled_{true};
    std::atomic<bool> shutdown_requested_{false};
    mutable std::mutex migrations_mutex_;
    
    // Configuration
    usize pool_size_per_node_;
    f64 migration_threshold_;
    f64 migration_cooldown_seconds_;
    bool enable_thermal_management_;
    
    // Global statistics
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> total_allocations_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> cross_node_allocations_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> successful_migrations_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> failed_migrations_{0};
    
public:
    explicit NumaAwarePool(NumaTopologyManager& topology_mgr,
                          usize pool_size_per_node = 64 * 1024 * 1024, // 64MB per node
                          memory::MemoryTracker* tracker = nullptr)
        : topology_manager_(topology_mgr), memory_tracker_(tracker),
          pool_size_per_node_(pool_size_per_node) {
        
        migration_threshold_ = 0.8; // Migrate when utilization > 80%
        migration_cooldown_seconds_ = 10.0; // 10 second cooldown
        enable_thermal_management_ = true;
        
        initialize_node_pools();
        
        // Start migration thread
        migration_thread_ = std::thread([this]() {
            migration_worker();
        });
        
        LOG_INFO("Initialized NUMA-aware pool: {} nodes, {}MB per node", 
                 node_pools_.size(), pool_size_per_node_ / (1024 * 1024));
    }
    
    ~NumaAwarePool() {
        shutdown_requested_.store(true);
        if (migration_thread_.joinable()) {
            migration_thread_.join();
        }
        
        cleanup_node_pools();
        
        LOG_INFO("NUMA-aware pool destroyed: total_allocations={}, migrations={}", 
                 total_allocations_.load(), successful_migrations_.load());
    }
    
    /**
     * @brief Allocate memory with NUMA awareness
     */
    void* allocate(usize size, usize alignment = 0) {
        if (size == 0) return nullptr;
        
        std::thread::id current_thread = std::this_thread::get_id();
        u32 preferred_node = topology_manager_.get_optimal_node_for_thread(current_thread);
        
        // Try preferred node first
        void* result = try_allocate_on_node(preferred_node, size, alignment);
        if (result) {
            total_allocations_.fetch_add(1, std::memory_order_relaxed);
            return result;
        }
        
        // Try other nodes if preferred node is full
        for (u32 node_id = 0; node_id < node_pools_.size(); ++node_id) {
            if (node_id == preferred_node) continue;
            
            result = try_allocate_on_node(node_id, size, alignment);
            if (result) {
                total_allocations_.fetch_add(1, std::memory_order_relaxed);
                cross_node_allocations_.fetch_add(1, std::memory_order_relaxed);
                
                // Consider migration if this becomes a pattern
                consider_thread_migration(current_thread, node_id);
                return result;
            }
        }
        
        LOG_WARNING("NUMA-aware allocation failed: size={}KB", size / 1024);
        return nullptr;
    }
    
    /**
     * @brief Deallocate memory from appropriate NUMA node
     */
    void deallocate(void* ptr) {
        if (!ptr) return;
        
        // Find which node owns this memory
        for (auto& node_pool : node_pools_) {
            if (owns_memory(node_pool.get(), ptr)) {
                deallocate_from_node(node_pool.get(), ptr);
                return;
            }
        }
        
        LOG_WARNING("Attempted to deallocate unknown NUMA pointer");
    }
    
    /**
     * @brief Request memory migration between nodes
     */
    bool request_migration(void* ptr, u32 target_node) {
        if (!ptr || target_node >= node_pools_.size()) {
            return false;
        }
        
        // Find source node
        for (u32 source_node = 0; source_node < node_pools_.size(); ++source_node) {
            if (owns_memory(node_pools_[source_node].get(), ptr)) {
                std::lock_guard<std::mutex> lock(migrations_mutex_);
                
                MigrationTask task{};
                task.source_address = ptr;
                task.size = get_allocation_size(ptr);
                task.source_node = source_node;
                task.target_node = target_node;
                task.priority = calculate_migration_priority(source_node, target_node);
                task.creation_time = std::chrono::steady_clock::now();
                
                pending_migrations_.push_back(task);
                
                LOG_DEBUG("Queued migration: ptr={}, size={}KB, {} -> {}", 
                         ptr, task.size / 1024, source_node, target_node);
                return true;
            }
        }
        
        return false;
    }
    
    /**
     * @brief Get comprehensive NUMA pool statistics
     */
    struct NumaPoolStatistics {
        struct NodeStats {
            u32 node_id;
            usize total_size;
            usize allocated_size;
            f64 utilization_ratio;
            u64 allocations;
            u64 deallocations;
            u64 migrations_in;
            u64 migrations_out;
            f64 thermal_factor;
            std::vector<u32> active_thread_count;
        };
        
        std::vector<NodeStats> per_node_stats;
        u64 total_allocations;
        u64 cross_node_allocations;
        f64 cross_node_ratio;
        u64 successful_migrations;
        u64 failed_migrations;
        usize pending_migrations_count;
        f64 average_node_utilization;
        u32 most_utilized_node;
        f64 numa_efficiency_score;
        
        // Performance insights
        f64 estimated_bandwidth_utilization;
        f64 memory_locality_score;
        std::vector<std::pair<u32, u32>> migration_hotspots; // [from_node, to_node]
    };
    
    NumaPoolStatistics get_statistics() const {
        NumaPoolStatistics stats{};
        
        stats.total_allocations = total_allocations_.load();
        stats.cross_node_allocations = cross_node_allocations_.load();
        if (stats.total_allocations > 0) {
            stats.cross_node_ratio = static_cast<f64>(stats.cross_node_allocations) / 
                                   stats.total_allocations;
        }
        
        stats.successful_migrations = successful_migrations_.load();
        stats.failed_migrations = failed_migrations_.load();
        
        {
            std::lock_guard<std::mutex> lock(migrations_mutex_);
            stats.pending_migrations_count = pending_migrations_.size();
        }
        
        // Per-node statistics
        f64 total_utilization = 0.0;
        f64 max_utilization = 0.0;
        u32 max_node = 0;
        
        for (u32 i = 0; i < node_pools_.size(); ++i) {
            const auto& node_pool = node_pools_[i];
            NumaPoolStatistics::NodeStats node_stat{};
            
            std::shared_lock<std::shared_mutex> lock(node_pool->pool_mutex);
            
            node_stat.node_id = node_pool->node_id;
            node_stat.total_size = node_pool->total_size;
            node_stat.allocated_size = node_pool->allocated_size;
            node_stat.utilization_ratio = node_stat.total_size > 0 ? 
                static_cast<f64>(node_stat.allocated_size) / node_stat.total_size : 0.0;
            
            node_stat.allocations = node_pool->allocations.load();
            node_stat.deallocations = node_pool->deallocations.load();
            node_stat.migrations_in = node_pool->migrations_in.load();
            node_stat.migrations_out = node_pool->migrations_out.load();
            
            // Get thermal information from topology
            auto topology_stats = topology_manager_.get_statistics();
            if (i < topology_stats.nodes.size()) {
                node_stat.thermal_factor = topology_stats.nodes[i].thermal_throttling_factor;
            }
            
            total_utilization += node_stat.utilization_ratio;
            if (node_stat.utilization_ratio > max_utilization) {
                max_utilization = node_stat.utilization_ratio;
                max_node = i;
            }
            
            stats.per_node_stats.push_back(node_stat);
        }
        
        stats.average_node_utilization = !node_pools_.empty() ? 
                                       total_utilization / node_pools_.size() : 0.0;
        stats.most_utilized_node = max_node;
        
        // Calculate NUMA efficiency score
        stats.numa_efficiency_score = calculate_efficiency_score(stats);
        
        // Estimate performance metrics
        stats.estimated_bandwidth_utilization = estimate_bandwidth_usage();
        stats.memory_locality_score = 1.0 - stats.cross_node_ratio;
        
        return stats;
    }
    
    /**
     * @brief Enable/disable automatic migration
     */
    void set_migration_enabled(bool enabled) {
        migration_enabled_.store(enabled);
    }
    
    void set_migration_threshold(f64 threshold) {
        migration_threshold_ = std::clamp(threshold, 0.1, 1.0);
    }
    
    void set_thermal_management_enabled(bool enabled) {
        enable_thermal_management_ = enabled;
    }
    
    NumaTopologyManager& get_topology_manager() { return topology_manager_; }
    const NumaTopologyManager& get_topology_manager() const { return topology_manager_; }
    
private:
    void initialize_node_pools() {
        u32 node_count = topology_manager_.get_node_count();
        node_pools_.reserve(node_count);
        
        for (u32 i = 0; i < node_count; ++i) {
            auto node_pool = std::make_unique<NodePool>(i, pool_size_per_node_);
            
            // Allocate memory for this node (in real implementation, use numa_alloc_onnode)
            node_pool->memory_base = std::aligned_alloc(core::CACHE_LINE_SIZE, pool_size_per_node_);
            if (!node_pool->memory_base) {
                LOG_ERROR("Failed to allocate memory for NUMA node {}", i);
                continue;
            }
            
            // Initialize arena allocator for this node
            node_pool->arena = std::make_unique<memory::ArenaAllocator>(
                node_pool->memory_base, pool_size_per_node_
            );
            
            // Initialize thermal pool if enabled
            if (enable_thermal_management_) {
                node_pool->thermal_pool = std::make_unique<thermal::ThermalPool>(
                    pool_size_per_node_ / 4 // 25% for hot data
                );
            }
            
            node_pools_.push_back(std::move(node_pool));
            
            LOG_DEBUG("Initialized NUMA node pool {}: {}MB allocated", 
                     i, pool_size_per_node_ / (1024 * 1024));
        }
    }
    
    void cleanup_node_pools() {
        for (auto& node_pool : node_pools_) {
            if (node_pool && node_pool->memory_base) {
                std::free(node_pool->memory_base);
                node_pool->memory_base = nullptr;
            }
        }
        node_pools_.clear();
    }
    
    void* try_allocate_on_node(u32 node_id, usize size, usize alignment) {
        if (node_id >= node_pools_.size()) return nullptr;
        
        auto& node_pool = node_pools_[node_id];
        std::unique_lock<std::shared_mutex> lock(node_pool->pool_mutex);
        
        void* result = nullptr;
        
        // Try thermal pool first for hot allocations
        if (enable_thermal_management_ && node_pool->thermal_pool) {
            // For now, assume all allocations start as hot
            result = node_pool->thermal_pool->allocate_hot(size, alignment);
        }
        
        // Fall back to arena allocator
        if (!result && node_pool->arena) {
            result = node_pool->arena->allocate(size, alignment);
        }
        
        if (result) {
            node_pool->allocated_size += size;
            node_pool->allocations.fetch_add(1, std::memory_order_relaxed);
            
            // Update topology manager
            topology_manager_.update_node_utilization(node_id, size, 0);
            
            // Track with memory tracker
            if (memory_tracker_) {
                memory_tracker_->track_allocation(
                    result, size, size, alignment ? alignment : sizeof(void*),
                    memory::AllocationCategory::Custom_01, // NUMA allocations
                    memory::AllocatorType::Custom,
                    "NumaAwarePool",
                    node_id
                );
            }
        }
        
        return result;
    }
    
    void deallocate_from_node(NodePool* node_pool, void* ptr) {
        std::unique_lock<std::shared_mutex> lock(node_pool->pool_mutex);
        
        usize size = get_allocation_size(ptr);
        
        // Try thermal pool first
        if (enable_thermal_management_ && node_pool->thermal_pool && 
            node_pool->thermal_pool->owns(ptr)) {
            node_pool->thermal_pool->deallocate(ptr);
        } else if (node_pool->arena) {
            node_pool->arena->deallocate(ptr);
        }
        
        node_pool->allocated_size -= size;
        node_pool->deallocations.fetch_add(1, std::memory_order_relaxed);
        
        // Update topology manager
        topology_manager_.update_node_utilization(node_pool->node_id, 0, size);
        
        // Track with memory tracker
        if (memory_tracker_) {
            memory_tracker_->track_deallocation(
                ptr, memory::AllocatorType::Custom, 
                "NumaAwarePool", node_pool->node_id
            );
        }
    }
    
    bool owns_memory(const NodePool* node_pool, const void* ptr) const {
        if (!ptr || !node_pool || !node_pool->memory_base) return false;
        
        const char* char_ptr = static_cast<const char*>(ptr);
        const char* base_ptr = static_cast<const char*>(node_pool->memory_base);
        
        return char_ptr >= base_ptr && 
               char_ptr < base_ptr + node_pool->total_size;
    }
    
    usize get_allocation_size(void* ptr) const {
        // Simplified - in real implementation, would query the allocator
        // For now, return a reasonable estimate
        return 64; // Default allocation size
    }
    
    void consider_thread_migration(std::thread::id thread_id, u32 node_id) {
        // If thread consistently allocates on a different node, update affinity
        static thread_local u32 consecutive_allocations = 0;
        static thread_local u32 last_allocation_node = UINT32_MAX;
        
        if (last_allocation_node == node_id) {
            consecutive_allocations++;
            if (consecutive_allocations >= 10) { // 10 consecutive allocations
                topology_manager_.set_thread_affinity(thread_id, node_id);
                consecutive_allocations = 0;
            }
        } else {
            consecutive_allocations = 1;
            last_allocation_node = node_id;
        }
    }
    
    f64 calculate_migration_priority(u32 source_node, u32 target_node) const {
        // Higher priority = more beneficial migration
        f64 source_utilization = 0.0;
        f64 target_utilization = 0.0;
        
        if (source_node < node_pools_.size()) {
            auto& source_pool = node_pools_[source_node];
            source_utilization = source_pool->total_size > 0 ? 
                static_cast<f64>(source_pool->allocated_size) / source_pool->total_size : 0.0;
        }
        
        if (target_node < node_pools_.size()) {
            auto& target_pool = node_pools_[target_node];
            target_utilization = target_pool->total_size > 0 ? 
                static_cast<f64>(target_pool->allocated_size) / target_pool->total_size : 0.0;
        }
        
        // Priority based on utilization difference and access cost
        f64 utilization_benefit = source_utilization - target_utilization;
        f64 access_cost_penalty = topology_manager_.get_access_cost(source_node, target_node) / 100.0;
        
        return utilization_benefit - access_cost_penalty;
    }
    
    void migration_worker() {
        while (!shutdown_requested_.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            if (!migration_enabled_.load()) continue;
            
            process_pending_migrations();
        }
    }
    
    void process_pending_migrations() {
        std::lock_guard<std::mutex> lock(migrations_mutex_);
        
        if (pending_migrations_.empty()) return;
        
        // Sort by priority (highest first)
        std::sort(pending_migrations_.begin(), pending_migrations_.end(),
                 [](const MigrationTask& a, const MigrationTask& b) {
                     return a.priority > b.priority;
                 });
        
        // Process up to 10 migrations per cycle
        usize migrations_to_process = std::min(pending_migrations_.size(), usize(10));
        
        for (usize i = 0; i < migrations_to_process; ++i) {
            const auto& task = pending_migrations_[i];
            
            if (perform_migration(task)) {
                successful_migrations_.fetch_add(1, std::memory_order_relaxed);
            } else {
                failed_migrations_.fetch_add(1, std::memory_order_relaxed);
            }
        }
        
        // Remove processed migrations
        pending_migrations_.erase(pending_migrations_.begin(), 
                                 pending_migrations_.begin() + migrations_to_process);
    }
    
    bool perform_migration(const MigrationTask& task) {
        // Simplified migration implementation
        // In real implementation, would carefully handle memory copying and pointer updates
        
        if (task.source_node >= node_pools_.size() || task.target_node >= node_pools_.size()) {
            return false;
        }
        
        auto& source_pool = node_pools_[task.source_node];
        auto& target_pool = node_pools_[task.target_node];
        
        // Check if target has space
        if (target_pool->allocated_size + task.size > target_pool->total_size) {
            return false; // Not enough space
        }
        
        // Update statistics
        source_pool->migrations_out.fetch_add(1, std::memory_order_relaxed);
        target_pool->migrations_in.fetch_add(1, std::memory_order_relaxed);
        
        LOG_DEBUG("Performed migration: {}KB from node {} to node {}", 
                 task.size / 1024, task.source_node, task.target_node);
        
        return true;
    }
    
    f64 calculate_efficiency_score(const NumaPoolStatistics& stats) const {
        // Efficiency based on locality, utilization balance, and migration success
        f64 locality_score = stats.memory_locality_score;
        
        f64 balance_score = 1.0;
        if (!stats.per_node_stats.empty()) {
            f64 min_util = 1.0, max_util = 0.0;
            for (const auto& node_stat : stats.per_node_stats) {
                min_util = std::min(min_util, node_stat.utilization_ratio);
                max_util = std::max(max_util, node_stat.utilization_ratio);
            }
            balance_score = 1.0 - (max_util - min_util);
        }
        
        f64 migration_success_rate = 1.0;
        u64 total_migrations = stats.successful_migrations + stats.failed_migrations;
        if (total_migrations > 0) {
            migration_success_rate = static_cast<f64>(stats.successful_migrations) / total_migrations;
        }
        
        return (locality_score * 0.4 + balance_score * 0.4 + migration_success_rate * 0.2);
    }
    
    f64 estimate_bandwidth_usage() const {
        f64 total_bandwidth = 0.0;
        auto topology_stats = topology_manager_.get_statistics();
        
        for (usize i = 0; i < std::min(node_pools_.size(), topology_stats.nodes.size()); ++i) {
            const auto& node_pool = node_pools_[i];
            const auto& node_info = topology_stats.nodes[i];
            
            f64 utilization = node_pool->total_size > 0 ? 
                static_cast<f64>(node_pool->allocated_size) / node_pool->total_size : 0.0;
            
            total_bandwidth += node_info.memory_bandwidth_gbps * utilization;
        }
        
        return total_bandwidth;
    }
};

//=============================================================================
// Global NUMA Manager Instance
//=============================================================================

NumaTopologyManager& get_global_numa_topology_manager();
NumaAwarePool& get_global_numa_aware_pool();

} // namespace ecscope::memory::specialized::numa