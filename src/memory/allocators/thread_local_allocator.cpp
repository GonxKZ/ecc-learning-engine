/**
 * @file memory/thread_local_allocator.cpp
 * @brief Implementation of thread-local memory allocation system
 */

#include "memory/thread_local_allocator.hpp"
#include "memory/numa_manager.hpp"
#include "core/log.hpp"
#include "core/profiler.hpp"
#include <algorithm>
#include <sstream>

namespace ecscope::memory::thread_local {

//=============================================================================
// ThreadLocalPool Implementation
//=============================================================================

ThreadLocalPool::ThreadLocalPool(usize initial_capacity, usize max_capacity, const std::string& pool_name)
    : pool_name_(pool_name), max_capacity_(max_capacity), owner_thread_(std::this_thread::get_id()) {
    
    // Get current NUMA node for optimal allocation
    auto& numa_manager = numa::get_global_numa_manager();
    auto current_node = numa_manager.get_current_thread_node();
    preferred_numa_node_ = current_node.value_or(0);
    
    // Create initial memory blocks
    expand_pool(initial_capacity);
    
    LOG_DEBUG("Created thread-local pool '{}' with {} initial capacity on NUMA node {}", 
             pool_name, initial_capacity, preferred_numa_node_);
}

ThreadLocalPool::~ThreadLocalPool() {
    // Clean up all memory blocks
    for (auto& block : memory_blocks_) {
        numa::get_global_numa_manager().deallocate(block.memory, block.size);
    }
    
    usize leaked_objects = allocated_count_.load();
    if (leaked_objects > 0) {
        LOG_WARNING("Thread-local pool '{}' destroyed with {} objects still allocated", 
                   pool_name_, leaked_objects);
    }
    
    LOG_DEBUG("Destroyed thread-local pool '{}' - {} total allocations, {} cache hits",
             pool_name_, total_allocations_.load(), cache_hits_.load());
}

void* ThreadLocalPool::allocate(usize size, usize alignment) {
    if (size == 0) return nullptr;
    
    total_allocations_.fetch_add(1, std::memory_order_relaxed);
    
    // Verify this pool is being used from the correct thread
    if (std::this_thread::get_id() != owner_thread_) {
        cross_thread_accesses_.fetch_add(1, std::memory_order_relaxed);
        LOG_WARNING("Cross-thread access to thread-local pool '{}' detected", pool_name_);
    }
    
    // Find a suitable size class
    usize size_class_index = find_size_class(size, alignment);
    if (size_class_index == USIZE_MAX) {
        // Size too large for size classes, allocate directly
        return allocate_large_object(size, alignment);
    }
    
    auto& size_class = size_classes_[size_class_index];
    
    // Try to get from free list first
    if (!size_class.free_objects.empty()) {
        void* ptr = size_class.free_objects.back();
        size_class.free_objects.pop_back();
        size_class.allocated_count++;
        cache_hits_.fetch_add(1, std::memory_order_relaxed);
        allocated_count_.fetch_add(1, std::memory_order_relaxed);
        return ptr;
    }
    
    // Need to allocate new object
    void* ptr = allocate_from_blocks(size_class.size, alignment);
    if (ptr) {
        size_class.allocated_count++;
        allocated_count_.fetch_add(1, std::memory_order_relaxed);
        record_allocation(ptr, size_class.size);
        return ptr;
    }
    
    // Try to expand pool
    if (expand_pool(DEFAULT_EXPANSION_SIZE)) {
        ptr = allocate_from_blocks(size_class.size, alignment);
        if (ptr) {
            size_class.allocated_count++;
            allocated_count_.fetch_add(1, std::memory_order_relaxed);
            record_allocation(ptr, size_class.size);
            return ptr;
        }
    }
    
    cache_misses_.fetch_add(1, std::memory_order_relaxed);
    return nullptr;
}

void ThreadLocalPool::deallocate(void* ptr) {
    if (!ptr) return;
    
    // Verify this pool is being used from the correct thread
    if (std::this_thread::get_id() != owner_thread_) {
        cross_thread_accesses_.fetch_add(1, std::memory_order_relaxed);
        LOG_WARNING("Cross-thread deallocation to thread-local pool '{}' detected", pool_name_);
    }
    
    // Find which size class this belongs to
    auto allocation_info = find_allocation_info(ptr);
    if (!allocation_info.has_value()) {
        LOG_ERROR("Attempted to deallocate unknown pointer from thread-local pool '{}'", pool_name_);
        return;
    }
    
    usize size = allocation_info.value();
    usize size_class_index = find_size_class_for_size(size);
    
    if (size_class_index == USIZE_MAX) {
        // Large object - deallocate directly
        deallocate_large_object(ptr);
        return;
    }
    
    auto& size_class = size_classes_[size_class_index];
    
    // Return to free list if there's space
    if (size_class.free_objects.size() < size_class.max_cached) {
        size_class.free_objects.push_back(ptr);
    } else {
        // Free list is full - actually deallocate
        // For thread-local pools, we typically don't deallocate back to system
        // to avoid fragmentation and maintain locality
        size_class.free_objects.push_back(ptr);
        
        // If we have too many cached objects, trim the cache
        if (size_class.free_objects.size() > size_class.max_cached * 2) {
            trim_size_class_cache(size_class_index);
        }
    }
    
    size_class.allocated_count--;
    allocated_count_.fetch_sub(1, std::memory_order_relaxed);
    remove_allocation_record(ptr);
}

bool ThreadLocalPool::owns(const void* ptr) const {
    if (!ptr) return false;
    
    const byte* byte_ptr = static_cast<const byte*>(ptr);
    
    for (const auto& block : memory_blocks_) {
        if (byte_ptr >= block.memory && byte_ptr < block.memory + block.size) {
            return true;
        }
    }
    
    return false;
}

ThreadLocalPool::Statistics ThreadLocalPool::get_statistics() const {
    Statistics stats{};
    
    stats.pool_name = pool_name_;
    stats.owner_thread = owner_thread_;
    stats.preferred_numa_node = preferred_numa_node_;
    stats.total_allocations = total_allocations_.load();
    stats.cache_hits = cache_hits_.load();
    stats.cache_misses = cache_misses_.load();
    stats.cross_thread_accesses = cross_thread_accesses_.load();
    stats.allocated_objects = allocated_count_.load();
    
    // Calculate hit rate
    u64 total_requests = stats.cache_hits + stats.cache_misses;
    if (total_requests > 0) {
        stats.hit_rate = static_cast<f64>(stats.cache_hits) / total_requests;
    }
    
    // Calculate memory usage
    for (const auto& block : memory_blocks_) {
        stats.total_memory_bytes += block.size;
        stats.committed_memory_bytes += block.committed_size;
    }
    
    // Count cached objects
    for (const auto& size_class : size_classes_) {
        stats.cached_objects += size_class.free_objects.size();
        
        if (!size_class.free_objects.empty()) {
            stats.active_size_classes++;
        }
    }
    
    // Calculate utilization
    if (stats.total_memory_bytes > 0) {
        usize used_memory = stats.total_memory_bytes - (stats.cached_objects * 64); // Estimate
        stats.utilization_ratio = static_cast<f64>(used_memory) / stats.total_memory_bytes;
    }
    
    return stats;
}

void ThreadLocalPool::trim_caches() {
    for (usize i = 0; i < size_classes_.size(); ++i) {
        trim_size_class_cache(i);
    }
    
    LOG_DEBUG("Trimmed caches for thread-local pool '{}'", pool_name_);
}

std::string ThreadLocalPool::generate_report() const {
    auto stats = get_statistics();
    std::ostringstream report;
    
    report << "Thread-Local Pool Report: " << pool_name_ << "\n";
    report << "  Owner Thread: " << std::hash<std::thread::id>{}(owner_thread_) << "\n";
    report << "  NUMA Node: " << preferred_numa_node_ << "\n";
    report << "  Total Allocations: " << stats.total_allocations << "\n";
    report << "  Cache Hit Rate: " << std::fixed << std::setprecision(2) 
           << stats.hit_rate * 100 << "%\n";
    report << "  Cross-Thread Accesses: " << stats.cross_thread_accesses << "\n";
    report << "  Memory Usage: " << (stats.total_memory_bytes / 1024) << " KB\n";
    report << "  Utilization: " << std::fixed << std::setprecision(1)
           << stats.utilization_ratio * 100 << "%\n";
    report << "  Active Size Classes: " << stats.active_size_classes << "\n";
    report << "  Cached Objects: " << stats.cached_objects << "\n";
    
    return report.str();
}

// Private implementation methods

void ThreadLocalPool::initialize_size_classes() {
    // Initialize common size classes (powers of 2 and common allocation sizes)
    const std::vector<std::pair<usize, usize>> size_configs = {
        {8, 1024},    // Small objects - cache many
        {16, 1024},
        {32, 512},
        {64, 512},
        {128, 256},   // Medium objects
        {256, 128},
        {512, 64},
        {1024, 32},   // Large objects - cache fewer
        {2048, 16},
        {4096, 8},
        {8192, 4}
    };
    
    for (const auto& [size, max_cached] : size_configs) {
        SizeClass size_class;
        size_class.size = size;
        size_class.max_cached = max_cached;
        size_class.free_objects.reserve(max_cached);
        size_classes_.push_back(std::move(size_class));
    }
}

usize ThreadLocalPool::find_size_class(usize size, usize alignment) const {
    // Find the smallest size class that fits the request
    for (usize i = 0; i < size_classes_.size(); ++i) {
        if (size_classes_[i].size >= size && size_classes_[i].size % alignment == 0) {
            return i;
        }
    }
    return USIZE_MAX;
}

usize ThreadLocalPool::find_size_class_for_size(usize size) const {
    for (usize i = 0; i < size_classes_.size(); ++i) {
        if (size_classes_[i].size == size) {
            return i;
        }
    }
    return USIZE_MAX;
}

bool ThreadLocalPool::expand_pool(usize additional_size) {
    if (get_total_capacity() + additional_size > max_capacity_) {
        return false;
    }
    
    // Allocate new memory block on preferred NUMA node
    auto& numa_manager = numa::get_global_numa_manager();
    void* memory = numa_manager.allocate_on_node(additional_size, preferred_numa_node_);
    
    if (!memory) {
        LOG_WARNING("Failed to allocate {} bytes for thread-local pool expansion", additional_size);
        return false;
    }
    
    MemoryBlock block;
    block.memory = static_cast<byte*>(memory);
    block.size = additional_size;
    block.committed_size = additional_size;
    block.current_offset = 0;
    
    memory_blocks_.push_back(block);
    
    LOG_DEBUG("Expanded thread-local pool '{}' by {} bytes (total: {} bytes)",
             pool_name_, additional_size, get_total_capacity());
    
    return true;
}

void* ThreadLocalPool::allocate_from_blocks(usize size, usize alignment) {
    // Try to allocate from existing blocks
    for (auto& block : memory_blocks_) {
        byte* aligned_ptr = align_pointer(block.memory + block.current_offset, alignment);
        usize padding = aligned_ptr - (block.memory + block.current_offset);
        usize total_needed = padding + size;
        
        if (block.current_offset + total_needed <= block.size) {
            block.current_offset += total_needed;
            return aligned_ptr;
        }
    }
    
    return nullptr;
}

void* ThreadLocalPool::allocate_large_object(usize size, usize alignment) {
    // For large objects, allocate directly from NUMA manager
    auto& numa_manager = numa::get_global_numa_manager();
    numa::NumaAllocationConfig config;
    config.policy = numa::NumaAllocationPolicy::Bind;
    config.preferred_node = preferred_numa_node_;
    config.alignment_bytes = alignment;
    
    void* ptr = numa_manager.allocate(size, config);
    if (ptr) {
        record_large_allocation(ptr, size);
        allocated_count_.fetch_add(1, std::memory_order_relaxed);
    }
    
    return ptr;
}

void ThreadLocalPool::deallocate_large_object(void* ptr) {
    auto it = large_allocations_.find(ptr);
    if (it != large_allocations_.end()) {
        usize size = it->second;
        numa::get_global_numa_manager().deallocate(ptr, size);
        large_allocations_.erase(it);
        allocated_count_.fetch_sub(1, std::memory_order_relaxed);
    }
}

void ThreadLocalPool::trim_size_class_cache(usize size_class_index) {
    if (size_class_index >= size_classes_.size()) return;
    
    auto& size_class = size_classes_[size_class_index];
    usize target_size = size_class.max_cached / 2;
    
    while (size_class.free_objects.size() > target_size) {
        size_class.free_objects.pop_back();
    }
}

byte* ThreadLocalPool::align_pointer(byte* ptr, usize alignment) const {
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t aligned_addr = (addr + alignment - 1) & ~(alignment - 1);
    return reinterpret_cast<byte*>(aligned_addr);
}

usize ThreadLocalPool::get_total_capacity() const {
    usize total = 0;
    for (const auto& block : memory_blocks_) {
        total += block.size;
    }
    return total;
}

void ThreadLocalPool::record_allocation(void* ptr, usize size) {
    allocation_tracking_[ptr] = size;
}

void ThreadLocalPool::remove_allocation_record(void* ptr) {
    allocation_tracking_.erase(ptr);
}

void ThreadLocalPool::record_large_allocation(void* ptr, usize size) {
    large_allocations_[ptr] = size;
}

std::optional<usize> ThreadLocalPool::find_allocation_info(void* ptr) const {
    auto it = allocation_tracking_.find(ptr);
    if (it != allocation_tracking_.end()) {
        return it->second;
    }
    
    auto large_it = large_allocations_.find(ptr);
    if (large_it != large_allocations_.end()) {
        return large_it->second;
    }
    
    return std::nullopt;
}

//=============================================================================
// ThreadLocalRegistry Implementation
//=============================================================================

ThreadLocalRegistry::ThreadLocalRegistry() {
    // Initialize with reasonable defaults
    default_pool_config_.initial_capacity = 1024 * 1024; // 1MB
    default_pool_config_.max_capacity = 16 * 1024 * 1024; // 16MB
    default_pool_config_.pool_name_prefix = "ThreadPool";
    
    LOG_INFO("Thread-local registry initialized");
}

ThreadLocalRegistry::~ThreadLocalRegistry() {
    shutdown();
}

void ThreadLocalRegistry::shutdown() {
    std::unique_lock<std::shared_mutex> lock(registry_mutex_);
    
    // Clean up all pools
    for (auto& [thread_id, pool] : thread_pools_) {
        // Pool destructors will handle cleanup
    }
    
    thread_pools_.clear();
    
    LOG_INFO("Thread-local registry shutdown complete");
}

ThreadLocalPool& ThreadLocalRegistry::get_primary_pool() {
    return get_or_create_pool("primary");
}

ThreadLocalPool& ThreadLocalRegistry::get_or_create_pool(const std::string& pool_name) {
    auto thread_id = std::this_thread::get_id();
    std::string full_name = default_pool_config_.pool_name_prefix + "_" + pool_name;
    
    // Try to find existing pool first (read lock)
    {
        std::shared_lock<std::shared_mutex> read_lock(registry_mutex_);
        auto thread_it = thread_pools_.find(thread_id);
        if (thread_it != thread_pools_.end()) {
            auto pool_it = thread_it->second.find(pool_name);
            if (pool_it != thread_it->second.end()) {
                return *pool_it->second;
            }
        }
    }
    
    // Need to create new pool (write lock)
    std::unique_lock<std::shared_mutex> write_lock(registry_mutex_);
    
    // Double-check after acquiring write lock
    auto& thread_pools_map = thread_pools_[thread_id];
    auto pool_it = thread_pools_map.find(pool_name);
    if (pool_it != thread_pools_map.end()) {
        return *pool_it->second;
    }
    
    // Create new pool
    auto pool = std::make_unique<ThreadLocalPool>(
        default_pool_config_.initial_capacity,
        default_pool_config_.max_capacity,
        full_name
    );
    
    auto& pool_ref = *pool;
    thread_pools_map[pool_name] = std::move(pool);
    
    // Update statistics
    total_pools_created_.fetch_add(1, std::memory_order_relaxed);
    active_threads_.store(thread_pools_.size(), std::memory_order_relaxed);
    
    LOG_DEBUG("Created new thread-local pool: {}", full_name);
    
    return pool_ref;
}

void ThreadLocalRegistry::cleanup_thread(std::thread::id thread_id) {
    std::unique_lock<std::shared_mutex> lock(registry_mutex_);
    
    auto it = thread_pools_.find(thread_id);
    if (it != thread_pools_.end()) {
        usize pool_count = it->second.size();
        thread_pools_.erase(it);
        active_threads_.store(thread_pools_.size(), std::memory_order_relaxed);
        
        LOG_DEBUG("Cleaned up {} thread-local pools for thread {}", 
                 pool_count, std::hash<std::thread::id>{}(thread_id));
    }
}

ThreadLocalRegistry::SystemStatistics ThreadLocalRegistry::get_system_statistics() const {
    std::shared_lock<std::shared_mutex> lock(registry_mutex_);
    
    SystemStatistics stats{};
    stats.total_pools = total_pools_created_.load();
    stats.tracked_threads = active_threads_.load();
    
    u64 total_allocations = 0;
    u64 total_cache_hits = 0;
    u64 total_cross_thread = 0;
    usize total_memory = 0;
    usize total_allocated_objects = 0;
    
    for (const auto& [thread_id, pools] : thread_pools_) {
        for (const auto& [pool_name, pool] : pools) {
            auto pool_stats = pool->get_statistics();
            total_allocations += pool_stats.total_allocations;
            total_cache_hits += pool_stats.cache_hits;
            total_cross_thread += pool_stats.cross_thread_accesses;
            total_memory += pool_stats.total_memory_bytes;
            total_allocated_objects += pool_stats.allocated_objects;
        }
    }
    
    if (total_allocations > 0) {
        stats.overall_local_ratio = static_cast<f64>(total_allocations - total_cross_thread) / total_allocations;
    }
    
    if (total_memory > 0) {
        stats.average_utilization = static_cast<f64>(total_allocated_objects * 64) / total_memory; // Estimate
    }
    
    stats.total_memory_usage = total_memory;
    
    return stats;
}

std::string ThreadLocalRegistry::generate_system_report() const {
    auto stats = get_system_statistics();
    std::ostringstream report;
    
    report << "Thread-Local System Report:\n";
    report << "  Total Pools Created: " << stats.total_pools << "\n";
    report << "  Active Threads: " << stats.tracked_threads << "\n";
    report << "  Total Memory Usage: " << (stats.total_memory_usage / (1024 * 1024)) << " MB\n";
    report << "  Local Allocation Ratio: " << std::fixed << std::setprecision(2)
           << stats.overall_local_ratio * 100 << "%\n";
    report << "  Average Utilization: " << std::fixed << std::setprecision(2)
           << stats.average_utilization * 100 << "%\n";
    
    // Per-thread breakdown
    std::shared_lock<std::shared_mutex> lock(registry_mutex_);
    report << "\nPer-Thread Breakdown:\n";
    
    for (const auto& [thread_id, pools] : thread_pools_) {
        report << "  Thread " << std::hash<std::thread::id>{}(thread_id) << ":\n";
        for (const auto& [pool_name, pool] : pools) {
            auto pool_stats = pool->get_statistics();
            report << "    " << pool_name << ": " 
                   << pool_stats.allocated_objects << " objects, "
                   << (pool_stats.total_memory_bytes / 1024) << " KB\n";
        }
    }
    
    return report.str();
}

void ThreadLocalRegistry::set_default_pool_config(const PoolConfiguration& config) {
    default_pool_config_ = config;
    LOG_DEBUG("Updated default thread-local pool configuration");
}

ThreadLocalRegistry::PoolConfiguration ThreadLocalRegistry::get_default_pool_config() const {
    return default_pool_config_;
}

//=============================================================================
// ThreadRegistrationGuard Implementation
//=============================================================================

ThreadRegistrationGuard::ThreadRegistrationGuard() 
    : thread_id_(std::this_thread::get_id()) {
    // Registration happens automatically when pools are accessed
}

ThreadRegistrationGuard::~ThreadRegistrationGuard() {
    // Clean up this thread's pools when the guard goes out of scope
    auto& registry = get_global_thread_local_registry();
    registry.cleanup_thread(thread_id_);
}

//=============================================================================
// Global Registry Instance
//=============================================================================

ThreadLocalRegistry& get_global_thread_local_registry() {
    static ThreadLocalRegistry instance;
    return instance;
}

} // namespace ecscope::memory::thread_local