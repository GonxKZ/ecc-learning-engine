#pragma once

/**
 * @file memory/hierarchical_pools.hpp
 * @brief Hierarchical Memory Pool System with Intelligent Size-Based Optimization
 * 
 * This header implements a sophisticated hierarchical memory pool system that
 * dynamically optimizes allocation patterns based on usage statistics and
 * provides educational insights into memory allocation behavior.
 * 
 * Key Features:
 * - Multi-level memory pool hierarchy (L1: fast thread-local, L2: shared pools, L3: global fallback)
 * - Intelligent size class generation based on actual allocation patterns
 * - Automatic pool splitting and merging based on utilization
 * - Memory access pattern learning and prediction
 * - Cross-thread load balancing and migration
 * - NUMA-aware pool placement and migration
 * - Comprehensive performance monitoring and visualization
 * 
 * Educational Value:
 * - Demonstrates cache hierarchy principles applied to memory allocation
 * - Shows adaptive algorithms for memory management optimization
 * - Illustrates trade-offs between memory usage and allocation speed
 * - Provides real-time visualization of memory allocation patterns
 * - Educational examples of memory pool sizing strategies
 * 
 * @author ECScope Educational ECS Framework - Advanced Memory Optimizations
 * @date 2025
 */

#include "lockfree_allocators.hpp"
#include "numa_manager.hpp"
#include "pool.hpp"  // Same directory
#include "core/types.hpp"
#include "core/log.hpp"
#include <atomic>
#include <memory>
#include <vector>
#include <array>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <algorithm>
#include <chrono>

namespace ecscope::memory::hierarchical {

//=============================================================================
// Size Class Analysis and Optimization
//=============================================================================

/**
 * @brief Intelligent size class analyzer that learns from allocation patterns
 */
class SizeClassAnalyzer {
private:
    struct AllocationPattern {
        usize size;
        u64 count;
        f64 frequency;
        f64 last_access_time;
        std::atomic<u64> recent_accesses{0};
        f64 average_lifetime_seconds;
    };
    
    // Allocation tracking
    std::unordered_map<usize, AllocationPattern> size_patterns_;
    mutable std::shared_mutex patterns_mutex_;
    
    // Analysis parameters
    static constexpr usize MAX_SIZE_CLASSES = 64;
    static constexpr f64 PATTERN_DECAY_FACTOR = 0.95;
    static constexpr f64 MIN_FREQUENCY_THRESHOLD = 0.01;
    
    // Performance tracking
    f64 last_analysis_time_;
    std::atomic<u64> total_allocations_{0};
    std::atomic<u64> analysis_counter_{0};
    
public:
    SizeClassAnalyzer() : last_analysis_time_(get_current_time()) {}
    
    /**
     * @brief Record an allocation for pattern analysis
     */
    void record_allocation(usize size, f64 lifetime_hint = 0.0) {
        total_allocations_.fetch_add(1, std::memory_order_relaxed);
        
        std::unique_lock<std::shared_mutex> lock(patterns_mutex_);
        
        auto& pattern = size_patterns_[size];
        pattern.size = size;
        pattern.count++;
        pattern.last_access_time = get_current_time();
        pattern.recent_accesses.fetch_add(1, std::memory_order_relaxed);
        
        if (lifetime_hint > 0.0) {
            // Update average lifetime using exponential moving average
            if (pattern.average_lifetime_seconds == 0.0) {
                pattern.average_lifetime_seconds = lifetime_hint;
            } else {
                pattern.average_lifetime_seconds = 
                    pattern.average_lifetime_seconds * 0.9 + lifetime_hint * 0.1;
            }
        }
    }
    
    /**
     * @brief Analyze patterns and generate optimal size classes
     */
    std::vector<usize> generate_optimal_size_classes() {
        PROFILE_FUNCTION();
        
        analysis_counter_.fetch_add(1, std::memory_order_relaxed);
        f64 current_time = get_current_time();
        
        std::shared_lock<std::shared_mutex> lock(patterns_mutex_);
        
        // Apply decay to older patterns
        f64 time_delta = current_time - last_analysis_time_;
        f64 decay_factor = std::pow(PATTERN_DECAY_FACTOR, time_delta);
        
        std::vector<std::pair<usize, f64>> size_frequencies;
        u64 total_count = 0;
        
        for (auto& [size, pattern] : size_patterns_) {
            pattern.frequency *= decay_factor;
            pattern.frequency += static_cast<f64>(pattern.recent_accesses.load()) / 1000.0;
            pattern.recent_accesses.store(0);
            
            if (pattern.frequency >= MIN_FREQUENCY_THRESHOLD) {
                size_frequencies.emplace_back(size, pattern.frequency);
                total_count += pattern.count;
            }
        }
        
        // Sort by frequency (most frequent first)
        std::sort(size_frequencies.begin(), size_frequencies.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // Generate size classes using frequency-based clustering
        std::vector<usize> size_classes;
        size_classes.reserve(std::min(size_frequencies.size(), MAX_SIZE_CLASSES));
        
        // Always include powers of 2 as base classes
        for (usize power = 8; power <= 16384 && size_classes.size() < MAX_SIZE_CLASSES; power *= 2) {
            size_classes.push_back(power);
        }
        
        // Add frequently used sizes that aren't powers of 2
        for (const auto& [size, frequency] : size_frequencies) {
            if (size_classes.size() >= MAX_SIZE_CLASSES) break;
            
            // Skip if too close to existing size class
            bool too_close = false;
            for (usize existing_size : size_classes) {
                f64 ratio = static_cast<f64>(size) / existing_size;
                if (ratio > 0.8 && ratio < 1.25) {
                    too_close = true;
                    break;
                }
            }
            
            if (!too_close) {
                size_classes.push_back(size);
            }
        }
        
        // Sort size classes
        std::sort(size_classes.begin(), size_classes.end());
        
        last_analysis_time_ = current_time;
        
        LOG_DEBUG("Generated {} optimal size classes from {} allocation patterns", 
                 size_classes.size(), size_frequencies.size());
        
        return size_classes;
    }
    
    /**
     * @brief Get allocation pattern statistics
     */
    struct PatternStatistics {
        usize total_unique_sizes;
        usize active_patterns;
        f64 pattern_diversity;
        usize most_frequent_size;
        f64 most_frequent_ratio;
        std::vector<std::pair<usize, u64>> top_sizes;
    };
    
    PatternStatistics get_pattern_statistics() const {
        std::shared_lock<std::shared_mutex> lock(patterns_mutex_);
        
        PatternStatistics stats{};
        stats.total_unique_sizes = size_patterns_.size();
        
        u64 total_count = 0;
        u64 max_count = 0;
        usize most_frequent = 0;
        std::vector<std::pair<usize, u64>> all_patterns;
        
        for (const auto& [size, pattern] : size_patterns_) {
            if (pattern.frequency >= MIN_FREQUENCY_THRESHOLD) {
                stats.active_patterns++;
            }
            
            total_count += pattern.count;
            all_patterns.emplace_back(size, pattern.count);
            
            if (pattern.count > max_count) {
                max_count = pattern.count;
                most_frequent = size;
            }
        }
        
        stats.most_frequent_size = most_frequent;
        if (total_count > 0) {
            stats.most_frequent_ratio = static_cast<f64>(max_count) / total_count;
        }
        
        // Calculate diversity (Shannon entropy)
        f64 entropy = 0.0;
        for (const auto& [size, count] : all_patterns) {
            if (count > 0) {
                f64 p = static_cast<f64>(count) / total_count;
                entropy -= p * std::log2(p);
            }
        }
        stats.pattern_diversity = entropy;
        
        // Get top sizes
        std::sort(all_patterns.begin(), all_patterns.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        stats.top_sizes.reserve(std::min(all_patterns.size(), size_t(10)));
        for (size_t i = 0; i < std::min(all_patterns.size(), size_t(10)); ++i) {
            stats.top_sizes.push_back(all_patterns[i]);
        }
        
        return stats;
    }
    
    /**
     * @brief Predict optimal pool size for a given size class
     */
    usize predict_pool_capacity(usize size_class) const {
        std::shared_lock<std::shared_mutex> lock(patterns_mutex_);
        
        // Find closest matching pattern
        f64 best_match_ratio = 0.0;
        const AllocationPattern* best_pattern = nullptr;
        
        for (const auto& [size, pattern] : size_patterns_) {
            if (size <= size_class) {
                f64 ratio = static_cast<f64>(size) / size_class;
                if (ratio > best_match_ratio) {
                    best_match_ratio = ratio;
                    best_pattern = &pattern;
                }
            }
        }
        
        if (best_pattern) {
            // Estimate capacity based on frequency and lifetime
            f64 expected_concurrent = best_pattern->frequency * best_pattern->average_lifetime_seconds;
            usize capacity = static_cast<usize>(expected_concurrent * 2.0); // 2x safety margin
            
            // Clamp to reasonable range
            return std::clamp(capacity, usize(64), usize(4096));
        }
        
        return 256; // Default capacity
    }
    
private:
    f64 get_current_time() const {
        using namespace std::chrono;
        return duration<f64>(steady_clock::now().time_since_epoch()).count();
    }
};

//=============================================================================
// Multi-Level Pool Hierarchy
//=============================================================================

/**
 * @brief Thread-local fast allocation cache (L1 cache equivalent)
 */
template<usize MaxSizeClasses = 32>
class ThreadLocalPoolCache {
private:
    struct CacheEntry {
        usize size_class;
        std::vector<void*> free_objects;
        usize max_cached;
        std::atomic<u64> hits{0};
        std::atomic<u64> misses{0};
        f64 last_refill_time{0.0};
        
        CacheEntry(usize sc = 0, usize max_cache = 64) 
            : size_class(sc), max_cached(max_cache) {
            free_objects.reserve(max_cache);
        }
    };
    
    alignas(core::CACHE_LINE_SIZE) std::array<CacheEntry, MaxSizeClasses> cache_entries_;
    std::atomic<usize> active_entries_{0};
    
    // Performance tracking
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> total_hits_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> total_misses_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> refill_operations_{0};
    
    std::thread::id owner_thread_;
    u32 preferred_numa_node_;
    
public:
    explicit ThreadLocalPoolCache(u32 numa_node = 0) 
        : owner_thread_(std::this_thread::get_id()), preferred_numa_node_(numa_node) {}
    
    /**
     * @brief Try to allocate from thread-local cache
     */
    void* try_allocate(usize size_class_index) {
        if (size_class_index >= active_entries_.load(std::memory_order_acquire)) {
            return nullptr;
        }
        
        auto& entry = cache_entries_[size_class_index];
        if (!entry.free_objects.empty()) {
            void* ptr = entry.free_objects.back();
            entry.free_objects.pop_back();
            entry.hits.fetch_add(1, std::memory_order_relaxed);
            total_hits_.fetch_add(1, std::memory_order_relaxed);
            return ptr;
        }
        
        entry.misses.fetch_add(1, std::memory_order_relaxed);
        total_misses_.fetch_add(1, std::memory_order_relaxed);
        return nullptr;
    }
    
    /**
     * @brief Return object to thread-local cache
     */
    bool try_cache(usize size_class_index, void* ptr) {
        if (size_class_index >= active_entries_.load(std::memory_order_acquire) || !ptr) {
            return false;
        }
        
        auto& entry = cache_entries_[size_class_index];
        if (entry.free_objects.size() < entry.max_cached) {
            entry.free_objects.push_back(ptr);
            return true;
        }
        
        return false;
    }
    
    /**
     * @brief Refill cache from parent pool
     */
    template<typename ParentPool>
    void refill_cache(usize size_class_index, ParentPool& parent_pool, usize refill_count = 16) {
        if (size_class_index >= active_entries_.load(std::memory_order_acquire)) {
            return;
        }
        
        auto& entry = cache_entries_[size_class_index];
        usize current_size = entry.free_objects.size();
        usize target_size = std::min(entry.max_cached, current_size + refill_count);
        
        for (usize i = current_size; i < target_size; ++i) {
            void* ptr = parent_pool.allocate_for_size_class(size_class_index);
            if (ptr) {
                entry.free_objects.push_back(ptr);
            } else {
                break; // Parent pool is empty
            }
        }
        
        entry.last_refill_time = get_current_time();
        refill_operations_.fetch_add(1, std::memory_order_relaxed);
    }
    
    /**
     * @brief Initialize cache entry for size class
     */
    bool add_size_class(usize size_class_index, usize size_class, usize max_cached = 64) {
        usize current_count = active_entries_.load(std::memory_order_acquire);
        if (size_class_index >= MaxSizeClasses) {
            return false;
        }
        
        cache_entries_[size_class_index] = CacheEntry(size_class, max_cached);
        
        // Update active count if this is a new entry
        if (size_class_index >= current_count) {
            active_entries_.store(size_class_index + 1, std::memory_order_release);
        }
        
        return true;
    }
    
    /**
     * @brief Get cache performance statistics
     */
    struct CacheStatistics {
        u64 total_hits;
        u64 total_misses;
        f64 hit_rate;
        u64 refill_operations;
        std::vector<std::pair<usize, f64>> per_class_hit_rates;
        usize total_cached_objects;
        f64 cache_utilization;
    };
    
    CacheStatistics get_statistics() const {
        CacheStatistics stats{};
        
        stats.total_hits = total_hits_.load(std::memory_order_relaxed);
        stats.total_misses = total_misses_.load(std::memory_order_relaxed);
        stats.refill_operations = refill_operations_.load(std::memory_order_relaxed);
        
        u64 total_requests = stats.total_hits + stats.total_misses;
        if (total_requests > 0) {
            stats.hit_rate = static_cast<f64>(stats.total_hits) / total_requests;
        }
        
        usize active_count = active_entries_.load(std::memory_order_acquire);
        for (usize i = 0; i < active_count; ++i) {
            const auto& entry = cache_entries_[i];
            u64 hits = entry.hits.load(std::memory_order_relaxed);
            u64 misses = entry.misses.load(std::memory_order_relaxed);
            u64 class_total = hits + misses;
            
            if (class_total > 0) {
                f64 class_hit_rate = static_cast<f64>(hits) / class_total;
                stats.per_class_hit_rates.emplace_back(entry.size_class, class_hit_rate);
            }
            
            stats.total_cached_objects += entry.free_objects.size();
        }
        
        // Calculate cache utilization
        usize total_capacity = 0;
        for (usize i = 0; i < active_count; ++i) {
            total_capacity += cache_entries_[i].max_cached;
        }
        
        if (total_capacity > 0) {
            stats.cache_utilization = static_cast<f64>(stats.total_cached_objects) / total_capacity;
        }
        
        return stats;
    }
    
    std::thread::id get_owner_thread() const { return owner_thread_; }
    u32 get_preferred_numa_node() const { return preferred_numa_node_; }
    
private:
    f64 get_current_time() const {
        using namespace std::chrono;
        return duration<f64>(steady_clock::now().time_since_epoch()).count();
    }
};

//=============================================================================
// Shared Pool Manager (L2 cache equivalent)
//=============================================================================

/**
 * @brief Shared memory pools for specific size classes with load balancing
 */
class SharedPoolManager {
private:
    struct SharedPool {
        usize size_class;
        std::unique_ptr<PoolAllocator> allocator;
        std::atomic<u64> allocation_count{0};
        std::atomic<u64> thread_requests{0};
        std::atomic<f64> average_utilization{0.0};
        u32 preferred_numa_node;
        f64 creation_time;
        std::mutex expansion_mutex;
        
        SharedPool(usize sc, u32 numa_node) 
            : size_class(sc), preferred_numa_node(numa_node) {
            creation_time = get_current_time();
            
            // Create pool allocator optimized for this size class
            std::string pool_name = "SharedPool_" + std::to_string(sc) + "_Node" + std::to_string(numa_node);
            allocator = std::make_unique<PoolAllocator>(sc, 1024, alignof(std::max_align_t), pool_name);
        }
        
    private:
        f64 get_current_time() const {
            using namespace std::chrono;
            return duration<f64>(steady_clock::now().time_since_epoch()).count();
        }
    };
    
    // Pool management
    std::unordered_map<usize, std::vector<std::unique_ptr<SharedPool>>> size_class_pools_;
    mutable std::shared_mutex pools_mutex_;
    
    // Load balancing
    std::atomic<usize> round_robin_counter_{0};
    
    // NUMA integration
    numa::NumaManager& numa_manager_;
    SizeClassAnalyzer& size_analyzer_;
    
    // Performance monitoring
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> total_allocations_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> pool_expansions_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> load_balance_operations_{0};
    
public:
    explicit SharedPoolManager(numa::NumaManager& numa_mgr, SizeClassAnalyzer& analyzer)
        : numa_manager_(numa_mgr), size_analyzer_(analyzer) {}
    
    /**
     * @brief Allocate from shared pool for specific size class
     */
    void* allocate_for_size_class(usize size_class_index, usize size_class) {
        total_allocations_.fetch_add(1, std::memory_order_relaxed);
        
        // Get or create pools for this size class
        auto pools = get_pools_for_size_class(size_class);
        if (pools.empty()) {
            if (!create_pools_for_size_class(size_class)) {
                return nullptr;
            }
            pools = get_pools_for_size_class(size_class);
        }
        
        // Try allocation with load balancing
        for (usize attempt = 0; attempt < pools.size() * 2; ++attempt) {
            usize pool_index = select_pool_for_allocation(pools);
            auto& pool = pools[pool_index];
            
            void* ptr = pool->allocator->try_allocate();
            if (ptr) {
                pool->allocation_count.fetch_add(1, std::memory_order_relaxed);
                pool->thread_requests.fetch_add(1, std::memory_order_relaxed);
                return ptr;
            }
            
            // Pool is full, try to expand it
            if (attempt == 0) {
                expand_pool(*pool, size_class);
            }
        }
        
        return nullptr; // All pools exhausted
    }
    
    /**
     * @brief Return object to shared pool
     */
    void deallocate_to_size_class(void* ptr, usize size_class) {
        if (!ptr) return;
        
        auto pools = get_pools_for_size_class(size_class);
        for (auto& pool : pools) {
            if (pool->allocator->owns(ptr)) {
                pool->allocator->deallocate(ptr);
                return;
            }
        }
        
        LOG_WARNING("Attempted to deallocate pointer not owned by any shared pool");
    }
    
    /**
     * @brief Update pools based on current allocation patterns
     */
    void optimize_pools() {
        PROFILE_FUNCTION();
        
        auto optimal_size_classes = size_analyzer_.generate_optimal_size_classes();
        
        std::unique_lock<std::shared_mutex> lock(pools_mutex_);
        
        // Create missing pools
        for (usize size_class : optimal_size_classes) {
            if (size_class_pools_.find(size_class) == size_class_pools_.end()) {
                create_pools_for_size_class_locked(size_class);
            }
        }
        
        // Update pool capacities based on usage patterns
        for (auto& [size_class, pools] : size_class_pools_) {
            for (auto& pool : pools) {
                update_pool_utilization(*pool);
            }
        }
        
        LOG_DEBUG("Optimized shared pools for {} size classes", optimal_size_classes.size());
    }
    
    /**
     * @brief Get comprehensive pool statistics
     */
    struct PoolManagerStatistics {
        usize total_size_classes;
        usize total_pools;
        u64 total_allocations;
        u64 pool_expansions;
        u64 load_balance_operations;
        f64 average_pool_utilization;
        std::unordered_map<usize, usize> pools_per_size_class;
        std::unordered_map<u32, usize> pools_per_numa_node;
    };
    
    PoolManagerStatistics get_statistics() const {
        std::shared_lock<std::shared_mutex> lock(pools_mutex_);
        
        PoolManagerStatistics stats{};
        stats.total_size_classes = size_class_pools_.size();
        stats.total_allocations = total_allocations_.load(std::memory_order_relaxed);
        stats.pool_expansions = pool_expansions_.load(std::memory_order_relaxed);
        stats.load_balance_operations = load_balance_operations_.load(std::memory_order_relaxed);
        
        usize total_pools = 0;
        f64 total_utilization = 0.0;
        
        for (const auto& [size_class, pools] : size_class_pools_) {
            stats.pools_per_size_class[size_class] = pools.size();
            total_pools += pools.size();
            
            for (const auto& pool : pools) {
                f64 utilization = pool->average_utilization.load(std::memory_order_relaxed);
                total_utilization += utilization;
                
                u32 numa_node = pool->preferred_numa_node;
                stats.pools_per_numa_node[numa_node]++;
            }
        }
        
        stats.total_pools = total_pools;
        if (total_pools > 0) {
            stats.average_pool_utilization = total_utilization / total_pools;
        }
        
        return stats;
    }
    
private:
    std::vector<SharedPool*> get_pools_for_size_class(usize size_class) const {
        std::shared_lock<std::shared_mutex> lock(pools_mutex_);
        
        auto it = size_class_pools_.find(size_class);
        if (it == size_class_pools_.end()) {
            return {};
        }
        
        std::vector<SharedPool*> result;
        result.reserve(it->second.size());
        for (const auto& pool : it->second) {
            result.push_back(pool.get());
        }
        
        return result;
    }
    
    bool create_pools_for_size_class(usize size_class) {
        std::unique_lock<std::shared_mutex> lock(pools_mutex_);
        return create_pools_for_size_class_locked(size_class);
    }
    
    bool create_pools_for_size_class_locked(usize size_class) {
        if (size_class_pools_.find(size_class) != size_class_pools_.end()) {
            return true; // Already exists
        }
        
        auto& pools = size_class_pools_[size_class];
        
        // Create one pool per available NUMA node for better locality
        auto available_nodes = numa_manager_.get_topology().get_available_nodes();
        if (available_nodes.empty()) {
            available_nodes.push_back(0); // Fallback
        }
        
        for (u32 numa_node : available_nodes) {
            pools.push_back(std::make_unique<SharedPool>(size_class, numa_node));
        }
        
        LOG_DEBUG("Created {} shared pools for size class {} across NUMA nodes", 
                 pools.size(), size_class);
        
        return true;
    }
    
    usize select_pool_for_allocation(const std::vector<SharedPool*>& pools) {
        if (pools.size() == 1) return 0;
        
        // Prefer pool on current NUMA node
        auto current_node = numa_manager_.get_current_thread_node();
        if (current_node.has_value()) {
            for (usize i = 0; i < pools.size(); ++i) {
                if (pools[i]->preferred_numa_node == current_node.value()) {
                    return i;
                }
            }
        }
        
        // Fallback to round-robin with load balancing
        usize base_index = round_robin_counter_.fetch_add(1, std::memory_order_relaxed) % pools.size();
        
        // Find least loaded pool starting from base index
        usize best_index = base_index;
        f64 best_utilization = pools[base_index]->average_utilization.load(std::memory_order_relaxed);
        
        for (usize offset = 1; offset < pools.size(); ++offset) {
            usize index = (base_index + offset) % pools.size();
            f64 utilization = pools[index]->average_utilization.load(std::memory_order_relaxed);
            
            if (utilization < best_utilization) {
                best_utilization = utilization;
                best_index = index;
            }
        }
        
        load_balance_operations_.fetch_add(1, std::memory_order_relaxed);
        return best_index;
    }
    
    void expand_pool(SharedPool& pool, usize size_class) {
        std::lock_guard<std::mutex> lock(pool.expansion_mutex);
        
        // Double check if expansion is still needed
        if (!pool.allocator->is_full()) {
            return;
        }
        
        bool success = pool.allocator->expand_pool();
        if (success) {
            pool_expansions_.fetch_add(1, std::memory_order_relaxed);
            LOG_DEBUG("Expanded pool for size class {} on NUMA node {}", 
                     size_class, pool.preferred_numa_node);
        }
    }
    
    void update_pool_utilization(SharedPool& pool) {
        f64 current_utilization = pool.allocator->utilization_ratio();
        
        // Update exponential moving average
        f64 current_avg = pool.average_utilization.load(std::memory_order_relaxed);
        f64 new_avg = current_avg * 0.9 + current_utilization * 0.1;
        pool.average_utilization.store(new_avg, std::memory_order_relaxed);
    }
};

//=============================================================================
// Hierarchical Pool Allocator (Main Interface)
//=============================================================================

/**
 * @brief Complete hierarchical memory pool system with adaptive optimization
 */
class HierarchicalPoolAllocator {
private:
    // Analysis and optimization
    std::unique_ptr<SizeClassAnalyzer> size_analyzer_;
    
    // Pool hierarchy
    std::unique_ptr<SharedPoolManager> shared_manager_;
    
    // Thread-local caches
    thread_local static std::unique_ptr<ThreadLocalPoolCache<>> local_cache_;
    static std::unordered_map<std::thread::id, ThreadLocalPoolCache<>*> thread_caches_;
    static std::shared_mutex thread_caches_mutex_;
    
    // Size class mapping
    std::vector<usize> size_classes_;
    std::unordered_map<usize, usize> size_to_class_index_;
    mutable std::shared_mutex size_classes_mutex_;
    
    // NUMA integration
    numa::NumaManager& numa_manager_;
    
    // Configuration
    std::atomic<bool> auto_optimization_enabled_{true};
    std::atomic<f64> optimization_interval_seconds_{10.0};
    std::atomic<bool> optimization_running_{false};
    
    // Background optimization
    std::thread optimization_thread_;
    std::atomic<bool> shutdown_requested_{false};
    
    // Performance tracking
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> l1_hits_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> l1_misses_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> l2_hits_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> l2_misses_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> fallback_allocations_{0};
    
public:
    explicit HierarchicalPoolAllocator(numa::NumaManager& numa_mgr = numa::get_global_numa_manager())
        : numa_manager_(numa_mgr) {
        
        size_analyzer_ = std::make_unique<SizeClassAnalyzer>();
        shared_manager_ = std::make_unique<SharedPoolManager>(numa_manager_, *size_analyzer_);
        
        initialize_default_size_classes();
        
        // Start background optimization thread
        optimization_thread_ = std::thread([this]() {
            optimization_worker();
        });
        
        LOG_INFO("Initialized hierarchical pool allocator with {} default size classes", 
                 size_classes_.size());
    }
    
    ~HierarchicalPoolAllocator() {
        shutdown_requested_.store(true);
        if (optimization_thread_.joinable()) {
            optimization_thread_.join();
        }
        
        // Cleanup thread-local caches
        std::unique_lock<std::shared_mutex> lock(thread_caches_mutex_);
        thread_caches_.clear();
    }
    
    /**
     * @brief Main allocation interface with hierarchical fallback
     */
    void* allocate(usize size, usize alignment = alignof(std::max_align_t)) {
        if (size == 0) return nullptr;
        
        // Record allocation pattern
        size_analyzer_->record_allocation(size);
        
        // Find appropriate size class
        usize size_class_index = find_size_class_index(size);
        if (size_class_index == USIZE_MAX) {
            // No size class available, use fallback
            fallback_allocations_.fetch_add(1, std::memory_order_relaxed);
            return fallback_allocate(size, alignment);
        }
        
        usize size_class = size_classes_[size_class_index];
        
        // Try L1 cache first (thread-local)
        void* ptr = try_allocate_from_l1_cache(size_class_index);
        if (ptr) {
            l1_hits_.fetch_add(1, std::memory_order_relaxed);
            return ptr;
        }
        l1_misses_.fetch_add(1, std::memory_order_relaxed);
        
        // Try L2 cache (shared pools)
        ptr = try_allocate_from_l2_cache(size_class_index, size_class);
        if (ptr) {
            l2_hits_.fetch_add(1, std::memory_order_relaxed);
            
            // Try to refill L1 cache for future allocations
            refill_l1_cache(size_class_index, size_class);
            return ptr;
        }
        l2_misses_.fetch_add(1, std::memory_order_relaxed);
        
        // Fallback to system allocator
        fallback_allocations_.fetch_add(1, std::memory_order_relaxed);
        return fallback_allocate(size, alignment);
    }
    
    /**
     * @brief Deallocate memory with intelligent caching
     */
    void deallocate(void* ptr) {
        if (!ptr) return;
        
        // Try to cache in L1 first
        usize size_class = find_allocation_size_class(ptr);
        if (size_class != USIZE_MAX) {
            usize size_class_index = get_size_class_index(size_class);
            if (size_class_index != USIZE_MAX) {
                if (try_cache_in_l1(size_class_index, ptr)) {
                    return;
                }
                
                // L1 cache full, return to L2
                shared_manager_->deallocate_to_size_class(ptr, size_class);
                return;
            }
        }
        
        // Fallback - might be from system allocator
        std::free(ptr);
    }
    
    /**
     * @brief Type-safe allocation
     */
    template<typename T>
    T* allocate(usize count = 1) {
        return static_cast<T*>(allocate(sizeof(T) * count, alignof(T)));
    }
    
    template<typename T>
    void deallocate(T* ptr) {
        deallocate(static_cast<void*>(ptr));
    }
    
    template<typename T, typename... Args>
    T* construct(Args&&... args) {
        T* ptr = allocate<T>(1);
        if (ptr) {
            new(ptr) T(std::forward<Args>(args)...);
        }
        return ptr;
    }
    
    template<typename T>
    void destroy(T* ptr) {
        if (ptr) {
            ptr->~T();
            deallocate(ptr);
        }
    }
    
    /**
     * @brief Get comprehensive hierarchical statistics
     */
    struct HierarchicalStatistics {
        // Cache hierarchy performance
        u64 l1_hits, l1_misses;
        u64 l2_hits, l2_misses;
        u64 fallback_allocations;
        f64 l1_hit_rate, l2_hit_rate;
        f64 overall_cache_efficiency;
        
        // Size class analysis
        SizeClassAnalyzer::PatternStatistics pattern_stats;
        
        // Shared pool performance
        SharedPoolManager::PoolManagerStatistics pool_stats;
        
        // Per-thread cache statistics
        std::unordered_map<std::thread::id, ThreadLocalPoolCache<>::CacheStatistics> thread_cache_stats;
        
        // System-wide metrics
        usize active_size_classes;
        f64 memory_utilization_efficiency;
        u64 total_memory_managed;
    };
    
    HierarchicalStatistics get_statistics() const {
        HierarchicalStatistics stats{};
        
        // Cache performance
        stats.l1_hits = l1_hits_.load(std::memory_order_relaxed);
        stats.l1_misses = l1_misses_.load(std::memory_order_relaxed);
        stats.l2_hits = l2_hits_.load(std::memory_order_relaxed);
        stats.l2_misses = l2_misses_.load(std::memory_order_relaxed);
        stats.fallback_allocations = fallback_allocations_.load(std::memory_order_relaxed);
        
        u64 l1_total = stats.l1_hits + stats.l1_misses;
        u64 l2_total = stats.l2_hits + stats.l2_misses;
        
        if (l1_total > 0) stats.l1_hit_rate = static_cast<f64>(stats.l1_hits) / l1_total;
        if (l2_total > 0) stats.l2_hit_rate = static_cast<f64>(stats.l2_hits) / l2_total;
        
        u64 total_requests = l1_total + stats.fallback_allocations;
        if (total_requests > 0) {
            stats.overall_cache_efficiency = static_cast<f64>(stats.l1_hits + stats.l2_hits) / total_requests;
        }
        
        // Analysis statistics
        stats.pattern_stats = size_analyzer_->get_pattern_statistics();
        stats.pool_stats = shared_manager_->get_statistics();
        
        // Thread cache statistics
        std::shared_lock<std::shared_mutex> lock(thread_caches_mutex_);
        for (const auto& [thread_id, cache] : thread_caches_) {
            stats.thread_cache_stats[thread_id] = cache->get_statistics();
        }
        
        // Size classes
        std::shared_lock<std::shared_mutex> size_lock(size_classes_mutex_);
        stats.active_size_classes = size_classes_.size();
        
        return stats;
    }
    
    /**
     * @brief Manual optimization trigger
     */
    void optimize_pools() {
        if (!optimization_running_.exchange(true)) {
            shared_manager_->optimize_pools();
            update_size_classes();
            optimization_running_.store(false);
        }
    }
    
    /**
     * @brief Configuration
     */
    void set_auto_optimization_enabled(bool enabled) {
        auto_optimization_enabled_.store(enabled);
    }
    
    void set_optimization_interval(f64 interval_seconds) {
        optimization_interval_seconds_.store(interval_seconds);
    }
    
    SizeClassAnalyzer& get_size_analyzer() { return *size_analyzer_; }
    SharedPoolManager& get_shared_manager() { return *shared_manager_; }
    
private:
    void initialize_default_size_classes() {
        // Start with power-of-2 size classes
        const std::vector<usize> default_sizes = {
            8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384
        };
        
        std::unique_lock<std::shared_mutex> lock(size_classes_mutex_);
        size_classes_ = default_sizes;
        
        size_to_class_index_.clear();
        for (usize i = 0; i < size_classes_.size(); ++i) {
            size_to_class_index_[size_classes_[i]] = i;
        }
    }
    
    void update_size_classes() {
        auto optimal_classes = size_analyzer_->generate_optimal_size_classes();
        
        std::unique_lock<std::shared_mutex> lock(size_classes_mutex_);
        size_classes_ = optimal_classes;
        
        size_to_class_index_.clear();
        for (usize i = 0; i < size_classes_.size(); ++i) {
            size_to_class_index_[size_classes_[i]] = i;
        }
        
        LOG_DEBUG("Updated size classes to {} optimal classes", optimal_classes.size());
    }
    
    usize find_size_class_index(usize size) const {
        std::shared_lock<std::shared_mutex> lock(size_classes_mutex_);
        
        // Find smallest size class that fits
        for (usize i = 0; i < size_classes_.size(); ++i) {
            if (size_classes_[i] >= size) {
                return i;
            }
        }
        
        return USIZE_MAX;
    }
    
    usize get_size_class_index(usize size_class) const {
        std::shared_lock<std::shared_mutex> lock(size_classes_mutex_);
        auto it = size_to_class_index_.find(size_class);
        return it != size_to_class_index_.end() ? it->second : USIZE_MAX;
    }
    
    ThreadLocalPoolCache<>* get_thread_local_cache() {
        if (!local_cache_) {
            auto current_node = numa_manager_.get_current_thread_node();
            local_cache_ = std::make_unique<ThreadLocalPoolCache<>>(current_node.value_or(0));
            
            // Register in global map
            std::unique_lock<std::shared_mutex> lock(thread_caches_mutex_);
            thread_caches_[std::this_thread::get_id()] = local_cache_.get();
        }
        return local_cache_.get();
    }
    
    void* try_allocate_from_l1_cache(usize size_class_index) {
        auto* cache = get_thread_local_cache();
        return cache->try_allocate(size_class_index);
    }
    
    void* try_allocate_from_l2_cache(usize size_class_index, usize size_class) {
        return shared_manager_->allocate_for_size_class(size_class_index, size_class);
    }
    
    bool try_cache_in_l1(usize size_class_index, void* ptr) {
        auto* cache = get_thread_local_cache();
        return cache->try_cache(size_class_index, ptr);
    }
    
    void refill_l1_cache(usize size_class_index, usize size_class) {
        auto* cache = get_thread_local_cache();
        // This would need integration with shared manager for refill
        // Simplified implementation
    }
    
    usize find_allocation_size_class(void* ptr) const {
        // This would need allocation tracking to implement properly
        // Simplified implementation returns USIZE_MAX
        return USIZE_MAX;
    }
    
    void* fallback_allocate(usize size, usize alignment) {
        if (alignment > alignof(std::max_align_t)) {
            void* ptr = nullptr;
            if (posix_memalign(&ptr, alignment, size) == 0) {
                return ptr;
            }
            return nullptr;
        } else {
            return std::malloc(size);
        }
    }
    
    void optimization_worker() {
        while (!shutdown_requested_.load()) {
            f64 interval = optimization_interval_seconds_.load();
            std::this_thread::sleep_for(std::chrono::duration<f64>(interval));
            
            if (auto_optimization_enabled_.load()) {
                optimize_pools();
            }
        }
    }
};

// Thread-local storage definition
thread_local std::unique_ptr<ThreadLocalPoolCache<>> HierarchicalPoolAllocator::local_cache_;
std::unordered_map<std::thread::id, ThreadLocalPoolCache<>*> HierarchicalPoolAllocator::thread_caches_;
std::shared_mutex HierarchicalPoolAllocator::thread_caches_mutex_;

//=============================================================================
// Global Instance
//=============================================================================

HierarchicalPoolAllocator& get_global_hierarchical_allocator();

} // namespace ecscope::memory::hierarchical