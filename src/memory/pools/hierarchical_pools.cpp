/**
 * @file memory/hierarchical_pools.cpp
 * @brief Implementation of hierarchical memory pool system
 */

#include "memory/hierarchical_pools.hpp"
#include "core/log.hpp"
#include "core/profiler.hpp"
#include <algorithm>
#include <sstream>
#include <cmath>

namespace ecscope::memory::hierarchical {

//=============================================================================
// Global Static Members
//=============================================================================

// Thread-local storage definition
thread_local std::unique_ptr<ThreadLocalPoolCache<>> HierarchicalPoolAllocator::local_cache_;
std::unordered_map<std::thread::id, ThreadLocalPoolCache<>*> HierarchicalPoolAllocator::thread_caches_;
std::shared_mutex HierarchicalPoolAllocator::thread_caches_mutex_;

//=============================================================================
// PoolAllocator Integration Helper
//=============================================================================

class HierarchicalPoolAllocatorAdapter {
private:
    std::unique_ptr<PoolAllocator> pool_allocator_;
    
public:
    explicit HierarchicalPoolAllocatorAdapter(usize object_size, usize initial_capacity = 1024)
        : pool_allocator_(std::make_unique<PoolAllocator>(
            object_size, initial_capacity, alignof(std::max_align_t), 
            "HierarchicalPool_" + std::to_string(object_size))) {
    }
    
    void* allocate() {
        return pool_allocator_->try_allocate();
    }
    
    void deallocate(void* ptr) {
        pool_allocator_->deallocate(ptr);
    }
    
    bool owns(const void* ptr) const {
        return pool_allocator_->owns(ptr);
    }
    
    bool expand_pool() {
        return pool_allocator_->expand_pool();
    }
    
    bool is_full() const {
        return pool_allocator_->utilization_ratio() > 0.95;
    }
    
    f64 utilization_ratio() const {
        return pool_allocator_->utilization_ratio();
    }
};

//=============================================================================
// SharedPoolManager Implementation
//=============================================================================

SharedPoolManager::SharedPoolManager(numa::NumaManager& numa_mgr, SizeClassAnalyzer& analyzer)
    : numa_manager_(numa_mgr), size_analyzer_(analyzer) {
    
    LOG_DEBUG("Initialized shared pool manager");
}

void* SharedPoolManager::allocate_for_size_class(usize size_class_index, usize size_class) {
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
        
        void* ptr = pool->allocator->allocate();
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

void SharedPoolManager::deallocate_to_size_class(void* ptr, usize size_class) {
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

void SharedPoolManager::optimize_pools() {
    PROFILE_FUNCTION();
    
    auto optimal_size_classes = size_analyzer_.generate_optimal_size_classes();
    
    std::unique_lock<std::shared_mutex> lock(pools_mutex_);
    
    // Create missing pools
    for (usize size_class : optimal_size_classes) {
        if (size_class_pools_.find(size_class) == size_class_pools_.end()) {
            create_pools_for_size_class_locked(size_class);
        }
    }
    
    // Update pool utilizations
    for (auto& [size_class, pools] : size_class_pools_) {
        for (auto& pool : pools) {
            update_pool_utilization(*pool);
        }
    }
    
    LOG_DEBUG("Optimized shared pools for {} size classes", optimal_size_classes.size());
}

SharedPoolManager::PoolManagerStatistics SharedPoolManager::get_statistics() const {
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

// Private implementation methods

std::vector<SharedPoolManager::SharedPool*> SharedPoolManager::get_pools_for_size_class(usize size_class) const {
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

bool SharedPoolManager::create_pools_for_size_class(usize size_class) {
    std::unique_lock<std::shared_mutex> lock(pools_mutex_);
    return create_pools_for_size_class_locked(size_class);
}

bool SharedPoolManager::create_pools_for_size_class_locked(usize size_class) {
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

usize SharedPoolManager::select_pool_for_allocation(const std::vector<SharedPool*>& pools) {
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

void SharedPoolManager::expand_pool(SharedPool& pool, usize size_class) {
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

void SharedPoolManager::update_pool_utilization(SharedPool& pool) {
    f64 current_utilization = pool.allocator->utilization_ratio();
    
    // Update exponential moving average
    f64 current_avg = pool.average_utilization.load(std::memory_order_relaxed);
    f64 new_avg = current_avg * 0.9 + current_utilization * 0.1;
    pool.average_utilization.store(new_avg, std::memory_order_relaxed);
}

//=============================================================================
// SharedPool Implementation
//=============================================================================

SharedPoolManager::SharedPool::SharedPool(usize sc, u32 numa_node) 
    : size_class(sc), preferred_numa_node(numa_node) {
    creation_time = get_current_time();
    
    // Create pool allocator optimized for this size class
    std::string pool_name = "SharedPool_" + std::to_string(sc) + "_Node" + std::to_string(numa_node);
    allocator = std::make_unique<HierarchicalPoolAllocatorAdapter>(sc, 1024);
}

f64 SharedPoolManager::SharedPool::get_current_time() const {
    using namespace std::chrono;
    return duration<f64>(steady_clock::now().time_since_epoch()).count();
}

//=============================================================================
// HierarchicalPoolAllocator Implementation
//=============================================================================

HierarchicalPoolAllocator::HierarchicalPoolAllocator(numa::NumaManager& numa_mgr)
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

HierarchicalPoolAllocator::~HierarchicalPoolAllocator() {
    shutdown_requested_.store(true);
    if (optimization_thread_.joinable()) {
        optimization_thread_.join();
    }
    
    // Cleanup thread-local caches
    std::unique_lock<std::shared_mutex> lock(thread_caches_mutex_);
    thread_caches_.clear();
}

void* HierarchicalPoolAllocator::allocate(usize size, usize alignment) {
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

void HierarchicalPoolAllocator::deallocate(void* ptr) {
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

void HierarchicalPoolAllocator::optimize_pools() {
    if (!optimization_running_.exchange(true)) {
        shared_manager_->optimize_pools();
        update_size_classes();
        optimization_running_.store(false);
    }
}

HierarchicalPoolAllocator::HierarchicalStatistics HierarchicalPoolAllocator::get_statistics() const {
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

// Private implementation methods

void HierarchicalPoolAllocator::initialize_default_size_classes() {
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

void HierarchicalPoolAllocator::update_size_classes() {
    auto optimal_classes = size_analyzer_->generate_optimal_size_classes();
    
    std::unique_lock<std::shared_mutex> lock(size_classes_mutex_);
    size_classes_ = optimal_classes;
    
    size_to_class_index_.clear();
    for (usize i = 0; i < size_classes_.size(); ++i) {
        size_to_class_index_[size_classes_[i]] = i;
    }
    
    LOG_DEBUG("Updated size classes to {} optimal classes", optimal_classes.size());
}

usize HierarchicalPoolAllocator::find_size_class_index(usize size) const {
    std::shared_lock<std::shared_mutex> lock(size_classes_mutex_);
    
    // Find smallest size class that fits
    for (usize i = 0; i < size_classes_.size(); ++i) {
        if (size_classes_[i] >= size) {
            return i;
        }
    }
    
    return USIZE_MAX;
}

usize HierarchicalPoolAllocator::get_size_class_index(usize size_class) const {
    std::shared_lock<std::shared_mutex> lock(size_classes_mutex_);
    auto it = size_to_class_index_.find(size_class);
    return it != size_to_class_index_.end() ? it->second : USIZE_MAX;
}

ThreadLocalPoolCache<>* HierarchicalPoolAllocator::get_thread_local_cache() {
    if (!local_cache_) {
        auto current_node = numa_manager_.get_current_thread_node();
        local_cache_ = std::make_unique<ThreadLocalPoolCache<>>(current_node.value_or(0));
        
        // Register in global map
        std::unique_lock<std::shared_mutex> lock(thread_caches_mutex_);
        thread_caches_[std::this_thread::get_id()] = local_cache_.get();
    }
    return local_cache_.get();
}

void* HierarchicalPoolAllocator::try_allocate_from_l1_cache(usize size_class_index) {
    auto* cache = get_thread_local_cache();
    return cache->try_allocate(size_class_index);
}

void* HierarchicalPoolAllocator::try_allocate_from_l2_cache(usize size_class_index, usize size_class) {
    return shared_manager_->allocate_for_size_class(size_class_index, size_class);
}

bool HierarchicalPoolAllocator::try_cache_in_l1(usize size_class_index, void* ptr) {
    auto* cache = get_thread_local_cache();
    return cache->try_cache(size_class_index, ptr);
}

void HierarchicalPoolAllocator::refill_l1_cache(usize size_class_index, usize size_class) {
    // Simplified implementation - would need integration with shared manager for refill
    // This is a placeholder for the complex refill logic
}

usize HierarchicalPoolAllocator::find_allocation_size_class(void* ptr) const {
    // This would need allocation tracking to implement properly
    // For now, return USIZE_MAX to indicate unknown
    return USIZE_MAX;
}

void* HierarchicalPoolAllocator::fallback_allocate(usize size, usize alignment) {
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

void HierarchicalPoolAllocator::optimization_worker() {
    while (!shutdown_requested_.load()) {
        f64 interval = optimization_interval_seconds_.load();
        std::this_thread::sleep_for(std::chrono::duration<f64>(interval));
        
        if (auto_optimization_enabled_.load()) {
            optimize_pools();
        }
    }
}

//=============================================================================
// Global Instance
//=============================================================================

HierarchicalPoolAllocator& get_global_hierarchical_allocator() {
    static auto& numa_manager = numa::get_global_numa_manager();
    static HierarchicalPoolAllocator instance(numa_manager);
    return instance;
}

} // namespace ecscope::memory::hierarchical