#pragma once

/**
 * @file memory/numa_manager.hpp
 * @brief NUMA-Aware Memory Management for High-Performance ECS Systems
 * 
 * This header provides advanced NUMA (Non-Uniform Memory Access) awareness
 * for the ECScope memory management system, optimizing memory allocation
 * and data placement based on system topology and thread affinity.
 * 
 * Educational Features:
 * - NUMA topology discovery and visualization
 * - Memory affinity optimization demonstrations
 * - Cross-NUMA memory access cost analysis
 * - Thread-to-memory-node binding strategies
 * - Memory bandwidth utilization per NUMA node
 * - Educational examples of NUMA impact on performance
 * 
 * Performance Benefits:
 * - Reduced memory access latency through local allocation
 * - Improved memory bandwidth utilization
 * - Better cache locality across NUMA nodes
 * - Optimized thread scheduling and memory placement
 * - Reduced memory contention between nodes
 * 
 * @author ECScope Educational ECS Framework - Advanced Memory Optimizations
 * @date 2025
 */

#include "core/types.hpp"
#include "core/log.hpp"
#include "lockfree_structures.hpp"
#include <atomic>
#include <memory>
#include <vector>
#include <array>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <bitset>

#ifdef __linux__
#include <numa.h>
#include <numaif.h>
#include <sched.h>
#endif

namespace ecscope::memory::numa {

//=============================================================================
// NUMA Topology Discovery and Analysis
//=============================================================================

/**
 * @brief NUMA node information structure
 */
struct NumaNode {
    u32 node_id;                        // NUMA node identifier
    usize total_memory_bytes;           // Total memory available on this node
    usize free_memory_bytes;            // Free memory available
    f64 memory_bandwidth_gbps;          // Measured memory bandwidth
    f64 memory_latency_ns;              // Average memory access latency
    std::vector<u32> cpu_cores;         // CPU cores on this node
    std::bitset<256> cpu_mask;          // CPU affinity mask
    bool is_available;                  // Whether this node is usable
    f64 utilization_ratio;              // Current memory utilization (0-1)
    
    NumaNode() noexcept
        : node_id(0), total_memory_bytes(0), free_memory_bytes(0),
          memory_bandwidth_gbps(0.0), memory_latency_ns(0.0),
          is_available(false), utilization_ratio(0.0) {}
};

/**
 * @brief NUMA distance matrix for cross-node access costs
 */
class NumaDistanceMatrix {
private:
    std::vector<std::vector<u32>> distances_;
    u32 node_count_;
    
public:
    explicit NumaDistanceMatrix(u32 node_count);
    
    void set_distance(u32 from_node, u32 to_node, u32 distance);
    u32 get_distance(u32 from_node, u32 to_node) const;
    
    // Analysis functions
    u32 find_closest_node(u32 from_node) const;
    std::vector<u32> get_nodes_by_distance(u32 from_node) const;
    f64 calculate_average_distance() const;
    f64 calculate_locality_score(u32 node) const;
    
    u32 node_count() const noexcept { return node_count_; }
};

/**
 * @brief System NUMA topology information
 */
struct NumaTopology {
    std::vector<NumaNode> nodes;        // Available NUMA nodes
    NumaDistanceMatrix distance_matrix; // Distance matrix between nodes
    u32 total_nodes;                    // Total number of NUMA nodes
    u32 total_cpus;                     // Total number of CPU cores
    bool numa_available;                // Whether NUMA is available
    std::string topology_description;   // Human-readable topology description
    
    NumaTopology();
    
    // Topology queries
    std::optional<u32> get_current_node() const;
    std::optional<u32> get_thread_node(std::thread::id thread_id) const;
    std::vector<u32> get_available_nodes() const;
    NumaNode* find_node(u32 node_id);
    const NumaNode* find_node(u32 node_id) const;
    
    // Performance analysis
    f64 calculate_cross_node_penalty(u32 from_node, u32 to_node) const;
    u32 find_optimal_node_for_thread() const;
    std::string generate_topology_report() const;
};

//=============================================================================
// NUMA-Aware Memory Allocation Policies
//=============================================================================

/**
 * @brief Memory allocation policies for NUMA optimization
 */
enum class NumaAllocationPolicy : u8 {
    Default,            // System default allocation
    LocalPreferred,     // Prefer local node, fallback to any
    LocalOnly,          // Only allocate on local node
    Interleave,         // Interleave across all nodes
    InterleaveSubset,   // Interleave across specified nodes
    Bind,               // Bind to specific node
    FirstTouch,         // Allocate on first-touch node
    RoundRobin          // Round-robin across nodes
};

/**
 * @brief NUMA memory allocation configuration
 */
struct NumaAllocationConfig {
    NumaAllocationPolicy policy;        // Allocation policy to use
    std::vector<u32> allowed_nodes;     // Nodes allowed for allocation
    u32 preferred_node;                 // Preferred node (for bind policy)
    bool migrate_on_fault;              // Allow migration on page fault
    bool transparent_hugepages;         // Use transparent huge pages
    usize alignment_bytes;              // Memory alignment requirement
    
    NumaAllocationConfig() noexcept
        : policy(NumaAllocationPolicy::Default), preferred_node(0),
          migrate_on_fault(false), transparent_hugepages(true),
          alignment_bytes(64) {}
};

/**
 * @brief NUMA-aware memory allocator interface
 */
class INumaAllocator {
public:
    virtual ~INumaAllocator() = default;
    
    // Core allocation interface
    virtual void* allocate(usize size, const NumaAllocationConfig& config = {}) = 0;
    virtual void deallocate(void* ptr, usize size) = 0;
    virtual bool owns(const void* ptr) const = 0;
    
    // NUMA-specific operations
    virtual std::optional<u32> get_allocation_node(const void* ptr) const = 0;
    virtual bool migrate_to_node(void* ptr, usize size, u32 target_node) = 0;
    virtual bool bind_to_node(void* ptr, usize size, u32 node_id) = 0;
    
    // Statistics and monitoring
    virtual std::unordered_map<u32, usize> get_allocation_stats() const = 0;
    virtual f64 get_cross_node_access_ratio() const = 0;
    virtual std::string get_allocation_report() const = 0;
};

//=============================================================================
// NUMA-Aware Memory Manager
//=============================================================================

/**
 * @brief Advanced NUMA-aware memory manager
 */
class NumaManager {
private:
    NumaTopology topology_;
    
    // Per-node allocators
    std::vector<std::unique_ptr<INumaAllocator>> node_allocators_;
    
    // Thread affinity tracking
    std::unordered_map<std::thread::id, u32> thread_node_affinity_;
    mutable std::shared_mutex affinity_mutex_;
    
    // Performance monitoring
    struct NumaStats {
        std::atomic<u64> local_allocations{0};
        std::atomic<u64> remote_allocations{0};
        std::atomic<u64> cross_node_accesses{0};
        std::atomic<u64> migration_events{0};
        std::atomic<f64> average_access_latency{0.0};
        alignas(core::CACHE_LINE_SIZE) std::atomic<usize> allocated_bytes{0};
    };
    
    alignas(core::CACHE_LINE_SIZE) std::array<NumaStats, 64> per_node_stats_; // Max 64 nodes
    std::atomic<u32> active_node_count_{0};
    
    // Memory tracking
    struct AllocationInfo {
        u32 node_id;
        usize size;
        std::thread::id allocating_thread;
        std::chrono::steady_clock::time_point allocation_time;
        NumaAllocationPolicy policy_used;
    };
    
    std::unordered_map<const void*, AllocationInfo> allocation_tracking_;
    mutable std::shared_mutex tracking_mutex_;
    
    // Configuration
    bool enable_automatic_migration_;
    f64 migration_threshold_ratio_;      // Migrate if cross-node access > this ratio
    u32 migration_check_interval_ms_;
    std::atomic<bool> numa_balancing_enabled_{false};
    
    // Performance measurement
    mutable std::atomic<u64> measurement_counter_{0};
    
public:
    NumaManager();
    ~NumaManager();
    
    // Non-copyable, non-movable
    NumaManager(const NumaManager&) = delete;
    NumaManager& operator=(const NumaManager&) = delete;
    NumaManager(NumaManager&&) = delete;
    NumaManager& operator=(NumaManager&&) = delete;
    
    // Initialization and setup
    bool initialize();
    void shutdown();
    bool is_numa_available() const { return topology_.numa_available; }
    
    // Topology access
    const NumaTopology& get_topology() const { return topology_; }
    void refresh_topology();
    
    // Memory allocation interface
    void* allocate(usize size, const NumaAllocationConfig& config = {});
    void* allocate_on_node(usize size, u32 node_id);
    void* allocate_interleaved(usize size, const std::vector<u32>& nodes = {});
    void deallocate(void* ptr, usize size);
    
    // Memory migration and binding
    bool migrate_memory(void* ptr, usize size, u32 target_node);
    bool bind_memory(void* ptr, usize size, u32 node_id);
    bool is_memory_local(const void* ptr) const;
    std::optional<u32> get_memory_node(const void* ptr) const;
    
    // Thread affinity management
    bool set_thread_affinity(std::thread::id thread_id, u32 node_id);
    bool set_current_thread_affinity(u32 node_id);
    std::optional<u32> get_thread_affinity(std::thread::id thread_id) const;
    std::optional<u32> get_current_thread_node() const;
    
    // Automatic balancing
    void enable_automatic_balancing(bool enable) { numa_balancing_enabled_ = enable; }
    void set_migration_threshold(f64 threshold) { migration_threshold_ratio_ = threshold; }
    void trigger_memory_balancing();
    
    // Performance monitoring
    struct PerformanceMetrics {
        f64 local_access_ratio;          // Ratio of local memory accesses
        f64 cross_node_penalty_factor;   // Average penalty for cross-node access
        f64 memory_bandwidth_utilization; // Per-node bandwidth utilization
        u64 total_allocations;
        u64 total_migrations;
        f64 average_allocation_latency_ns;
        std::unordered_map<u32, f64> node_utilization;
        std::vector<std::pair<u32, u32>> hottest_cross_node_paths;
    };
    
    PerformanceMetrics get_performance_metrics() const;
    void reset_statistics();
    std::string generate_performance_report() const;
    
    // Memory layout optimization
    struct LayoutRecommendation {
        std::string recommendation_type;
        std::string description;
        u32 recommended_node;
        f64 expected_improvement;
        std::vector<std::string> implementation_steps;
    };
    
    std::vector<LayoutRecommendation> analyze_memory_layout() const;
    std::vector<LayoutRecommendation> get_optimization_recommendations() const;
    
    // Educational and debugging features
    void print_numa_topology() const;
    void visualize_memory_distribution() const;
    void demonstrate_numa_effects() const;
    
    // Benchmarking utilities
    f64 measure_memory_bandwidth(u32 node_id, usize buffer_size_mb = 100);
    f64 measure_cross_node_latency(u32 from_node, u32 to_node);
    std::unordered_map<u32, f64> benchmark_all_nodes();
    
private:
    // Internal implementation
    bool discover_numa_topology();
    void initialize_node_allocators();
    void setup_performance_monitoring();
    
    u32 select_optimal_node(const NumaAllocationConfig& config) const;
    void* allocate_with_policy(usize size, const NumaAllocationConfig& config);
    
    void record_allocation(const void* ptr, usize size, u32 node_id, 
                          const NumaAllocationConfig& config);
    void record_deallocation(const void* ptr);
    
    void update_performance_counters(u32 node_id, bool is_local_access);
    void run_memory_balancing_worker();
    
    // Platform-specific implementations
#ifdef __linux__
    bool linux_discover_topology();
    void* linux_allocate_on_node(usize size, u32 node_id);
    bool linux_migrate_pages(void* ptr, usize size, u32 target_node);
    bool linux_set_thread_affinity(std::thread::id thread_id, u32 node_id);
#endif
    
    // Educational demonstrations
    void demonstrate_local_vs_remote_access() const;
    void demonstrate_memory_migration() const;
    void demonstrate_thread_affinity_impact() const;
};

//=============================================================================
// NUMA-Aware Allocator Implementations
//=============================================================================

/**
 * @brief Simple NUMA-aware allocator using system facilities
 */
class SystemNumaAllocator : public INumaAllocator {
private:
    u32 node_id_;
    std::atomic<usize> allocated_bytes_{0};
    std::unordered_map<const void*, usize> allocation_sizes_;
    mutable std::mutex allocation_mutex_;
    
public:
    explicit SystemNumaAllocator(u32 node_id);
    ~SystemNumaAllocator() override;
    
    void* allocate(usize size, const NumaAllocationConfig& config = {}) override;
    void deallocate(void* ptr, usize size) override;
    bool owns(const void* ptr) const override;
    
    std::optional<u32> get_allocation_node(const void* ptr) const override;
    bool migrate_to_node(void* ptr, usize size, u32 target_node) override;
    bool bind_to_node(void* ptr, usize size, u32 node_id) override;
    
    std::unordered_map<u32, usize> get_allocation_stats() const override;
    f64 get_cross_node_access_ratio() const override;
    std::string get_allocation_report() const override;
    
    u32 get_node_id() const { return node_id_; }
    usize get_allocated_bytes() const { return allocated_bytes_.load(); }
};

/**
 * @brief Lock-free NUMA-aware pool allocator
 */
template<typename T>
class NumaAwarePoolAllocator : public INumaAllocator {
private:
    struct alignas(core::CACHE_LINE_SIZE) NumaPool {
        lockfree::LockFreeMemoryPool<T> pool;
        std::atomic<usize> allocation_count{0};
        std::atomic<f64> access_latency_sum{0.0};
        u32 node_id;
        
        explicit NumaPool(u32 node) : node_id(node) {}
    };
    
    std::vector<std::unique_ptr<NumaPool>> node_pools_;
    std::atomic<u32> current_node_{0};
    NumaManager& numa_manager_;
    
    // Performance tracking
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> local_allocations_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> remote_allocations_{0};
    
public:
    explicit NumaAwarePoolAllocator(NumaManager& numa_manager);
    ~NumaAwarePoolAllocator() override;
    
    void* allocate(usize size, const NumaAllocationConfig& config = {}) override;
    void deallocate(void* ptr, usize size) override;
    bool owns(const void* ptr) const override;
    
    std::optional<u32> get_allocation_node(const void* ptr) const override;
    bool migrate_to_node(void* ptr, usize size, u32 target_node) override;
    bool bind_to_node(void* ptr, usize size, u32 node_id) override;
    
    std::unordered_map<u32, usize> get_allocation_stats() const override;
    f64 get_cross_node_access_ratio() const override;
    std::string get_allocation_report() const override;
    
    // Type-specific operations
    T* construct(const NumaAllocationConfig& config = {});
    template<typename... Args>
    T* construct(Args&&... args, const NumaAllocationConfig& config = {});
    void destroy(T* ptr);
    
private:
    u32 select_optimal_node(const NumaAllocationConfig& config) const;
    NumaPool* find_pool_for_ptr(const void* ptr) const;
};

//=============================================================================
// Global NUMA Manager Instance
//=============================================================================

/**
 * @brief Global NUMA manager singleton
 */
NumaManager& get_global_numa_manager();

/**
 * @brief NUMA-aware allocation helpers
 */
namespace helpers {
    
    template<typename T>
    T* numa_allocate(usize count = 1, u32 preferred_node = UINT32_MAX) {
        NumaAllocationConfig config;
        if (preferred_node != UINT32_MAX) {
            config.policy = NumaAllocationPolicy::Bind;
            config.preferred_node = preferred_node;
        } else {
            config.policy = NumaAllocationPolicy::LocalPreferred;
        }
        
        void* ptr = get_global_numa_manager().allocate(sizeof(T) * count, config);
        return static_cast<T*>(ptr);
    }
    
    template<typename T>
    void numa_deallocate(T* ptr, usize count = 1) {
        get_global_numa_manager().deallocate(ptr, sizeof(T) * count);
    }
    
    template<typename T, typename... Args>
    T* numa_construct(u32 preferred_node, Args&&... args) {
        T* ptr = numa_allocate<T>(1, preferred_node);
        if (ptr) {
            new(ptr) T(std::forward<Args>(args)...);
        }
        return ptr;
    }
    
    template<typename T>
    void numa_destroy(T* ptr) {
        if (ptr) {
            ptr->~T();
            numa_deallocate(ptr, 1);
        }
    }
    
    // Educational helpers
    void demonstrate_numa_allocation_patterns();
    void benchmark_numa_vs_regular_allocation();
    void visualize_numa_memory_layout();
    
} // namespace helpers

//=============================================================================
// NUMA-Aware Container Adaptors
//=============================================================================

/**
 * @brief NUMA-aware vector with intelligent memory placement
 */
template<typename T>
class NumaVector {
private:
    struct NumaSegment {
        T* data;
        usize capacity;
        u32 node_id;
        std::atomic<usize> access_count{0};
    };
    
    std::vector<NumaSegment> segments_;
    usize total_size_;
    usize segment_size_;
    NumaManager& numa_manager_;
    
public:
    explicit NumaVector(NumaManager& numa_manager, usize segment_size = 1024)
        : total_size_(0), segment_size_(segment_size), numa_manager_(numa_manager) {}
    
    ~NumaVector() {
        clear();
    }
    
    // Container interface
    void push_back(const T& value);
    void push_back(T&& value);
    template<typename... Args> void emplace_back(Args&&... args);
    
    void pop_back();
    void clear();
    void reserve(usize new_capacity);
    void resize(usize new_size);
    
    // Access
    T& operator[](usize index);
    const T& operator[](usize index) const;
    T& at(usize index);
    const T& at(usize index) const;
    
    // Properties
    usize size() const { return total_size_; }
    bool empty() const { return total_size_ == 0; }
    
    // NUMA-specific operations
    void migrate_segment_to_node(usize segment_index, u32 target_node);
    void optimize_layout_for_access_pattern();
    std::vector<u32> get_segment_nodes() const;
    
private:
    std::pair<usize, usize> find_segment_and_offset(usize index) const;
    void expand_if_needed();
    u32 select_node_for_new_segment() const;
};

} // namespace ecscope::memory::numa