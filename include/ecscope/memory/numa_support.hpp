#pragma once

#include "allocators.hpp"
#include <thread>
#include <unordered_map>
#include <vector>
#include <atomic>
#include <mutex>
#include <shared_mutex>

#ifdef _WIN32
    #include <windows.h>
    #include <processthreadsapi.h>
    #include <sysinfoapi.h>
#else
    #include <numa.h>
    #include <sched.h>
    #include <sys/syscall.h>
#endif

namespace ecscope::memory {

// NUMA topology detection and management
class NUMATopology {
public:
    struct NodeInfo {
        uint32_t node_id;
        std::vector<uint32_t> cpu_ids;
        size_t memory_size;
        double memory_bandwidth; // GB/s
    };
    
    static NUMATopology& instance() {
        static NUMATopology topology;
        return topology;
    }
    
    [[nodiscard]] bool is_numa_available() const noexcept {
        return numa_available_;
    }
    
    [[nodiscard]] uint32_t get_num_nodes() const noexcept {
        return static_cast<uint32_t>(nodes_.size());
    }
    
    [[nodiscard]] uint32_t get_current_node() const noexcept {
#ifdef _WIN32
        PROCESSOR_NUMBER proc_num;
        GetCurrentProcessorNumberEx(&proc_num);
        return get_node_for_processor(proc_num.Number);
#else
        if (!numa_available_) return 0;
        return numa_node_of_cpu(sched_getcpu());
#endif
    }
    
    [[nodiscard]] const NodeInfo& get_node_info(uint32_t node_id) const {
        return nodes_.at(node_id);
    }
    
    [[nodiscard]] std::vector<uint32_t> get_node_ids() const {
        std::vector<uint32_t> ids;
        ids.reserve(nodes_.size());
        for (const auto& [id, _] : nodes_) {
            ids.push_back(id);
        }
        return ids;
    }
    
    // Allocate memory on specific NUMA node
    [[nodiscard]] void* allocate_on_node(size_t size, uint32_t node_id) const {
#ifdef _WIN32
        return VirtualAllocExNuma(GetCurrentProcess(), nullptr, size,
                                MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE, node_id);
#else
        if (!numa_available_) return nullptr;
        
        void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (ptr == MAP_FAILED) return nullptr;
        
        if (mbind(ptr, size, MPOL_BIND, 
                 numa_get_mems_allowed()->maskp,
                 numa_get_mems_allowed()->size, 0) != 0) {
            munmap(ptr, size);
            return nullptr;
        }
        
        return ptr;
#endif
    }
    
    void deallocate_on_node(void* ptr, size_t size) const noexcept {
        if (!ptr) return;
        
#ifdef _WIN32
        VirtualFree(ptr, 0, MEM_RELEASE);
#else
        munmap(ptr, size);
#endif
    }
    
private:
    NUMATopology() {
        detect_topology();
    }
    
    void detect_topology() {
#ifdef _WIN32
        // Windows NUMA detection
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* buffer = nullptr;
        DWORD buffer_size = 0;
        
        GetLogicalProcessorInformationEx(RelationNumaNode, buffer, &buffer_size);
        buffer = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(malloc(buffer_size));
        
        if (GetLogicalProcessorInformationEx(RelationNumaNode, buffer, &buffer_size)) {
            numa_available_ = true;
            
            auto current = buffer;
            while (reinterpret_cast<uint8_t*>(current) < 
                   reinterpret_cast<uint8_t*>(buffer) + buffer_size) {
                
                if (current->Relationship == RelationNumaNode) {
                    NodeInfo info;
                    info.node_id = current->NumaNode.NodeNumber;
                    info.memory_size = 0; // Would need additional APIs to get actual size
                    info.memory_bandwidth = estimate_memory_bandwidth();
                    
                    // Extract CPU IDs from processor mask
                    auto mask = current->NumaNode.GroupMask.Mask;
                    for (uint32_t cpu = 0; cpu < 64; ++cpu) {
                        if (mask & (1ULL << cpu)) {
                            info.cpu_ids.push_back(cpu);
                        }
                    }
                    
                    nodes_[info.node_id] = std::move(info);
                }
                
                current = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(
                    reinterpret_cast<uint8_t*>(current) + current->Size);
            }
        }
        
        free(buffer);
#else
        // Linux NUMA detection
        if (numa_available() >= 0) {
            numa_available_ = true;
            
            int max_nodes = numa_max_node() + 1;
            for (int node = 0; node < max_nodes; ++node) {
                if (numa_bitmask_isbitset(numa_get_mems_allowed(), node)) {
                    NodeInfo info;
                    info.node_id = static_cast<uint32_t>(node);
                    info.memory_size = numa_node_size64(node, nullptr);
                    info.memory_bandwidth = estimate_memory_bandwidth();
                    
                    // Get CPUs for this node
                    struct bitmask* cpu_mask = numa_allocate_cpumask();
                    if (numa_node_to_cpus(node, cpu_mask) == 0) {
                        for (unsigned int cpu = 0; cpu < numa_num_possible_cpus(); ++cpu) {
                            if (numa_bitmask_isbitset(cpu_mask, cpu)) {
                                info.cpu_ids.push_back(cpu);
                            }
                        }
                    }
                    numa_free_cpumask(cpu_mask);
                    
                    nodes_[info.node_id] = std::move(info);
                }
            }
        }
#endif
        
        if (nodes_.empty()) {
            // Fallback: create single node
            numa_available_ = false;
            NodeInfo info;
            info.node_id = 0;
            info.memory_size = get_total_physical_memory();
            info.memory_bandwidth = estimate_memory_bandwidth();
            
            // Add all available CPUs
            uint32_t num_cpus = std::thread::hardware_concurrency();
            for (uint32_t cpu = 0; cpu < num_cpus; ++cpu) {
                info.cpu_ids.push_back(cpu);
            }
            
            nodes_[0] = std::move(info);
        }
    }
    
    [[nodiscard]] size_t get_total_physical_memory() const {
#ifdef _WIN32
        MEMORYSTATUSEX status;
        status.dwLength = sizeof(status);
        GlobalMemoryStatusEx(&status);
        return status.ullTotalPhys;
#else
        long pages = sysconf(_SC_PHYS_PAGES);
        long page_size = sysconf(_SC_PAGE_SIZE);
        return pages * page_size;
#endif
    }
    
    [[nodiscard]] double estimate_memory_bandwidth() const {
        // Simplified bandwidth estimation based on memory type
        // In a real implementation, this would run benchmarks
        return 25.6; // DDR4-3200 approximate bandwidth in GB/s
    }
    
    [[nodiscard]] uint32_t get_node_for_processor(uint32_t cpu_id) const {
        for (const auto& [node_id, info] : nodes_) {
            auto it = std::find(info.cpu_ids.begin(), info.cpu_ids.end(), cpu_id);
            if (it != info.cpu_ids.end()) {
                return node_id;
            }
        }
        return 0; // Fallback
    }
    
    bool numa_available_ = false;
    std::unordered_map<uint32_t, NodeInfo> nodes_;
};

// ==== NUMA-AWARE ALLOCATOR ====
// Allocator that prefers local NUMA node memory
class NUMAAllocator {
public:
    explicit NUMAAllocator(size_t capacity_per_node = 64 * 1024 * 1024) // 64MB default
        : capacity_per_node_(capacity_per_node) {
        
        auto& topology = NUMATopology::instance();
        auto node_ids = topology.get_node_ids();
        
        // Create allocator for each NUMA node
        for (uint32_t node_id : node_ids) {
            node_allocators_[node_id] = std::make_unique<FreeListAllocator>(capacity_per_node_);
        }
    }
    
    [[nodiscard]] void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) {
        uint32_t preferred_node = NUMATopology::instance().get_current_node();
        
        // Try preferred node first
        if (auto it = node_allocators_.find(preferred_node); 
            it != node_allocators_.end()) {
            if (auto ptr = it->second->allocate(size, alignment)) {
                return ptr;
            }
        }
        
        // Fall back to other nodes
        for (auto& [node_id, allocator] : node_allocators_) {
            if (node_id != preferred_node) {
                if (auto ptr = allocator->allocate(size, alignment)) {
                    return ptr;
                }
            }
        }
        
        return nullptr;
    }
    
    void deallocate(void* ptr, size_t size) {
        if (!ptr) return;
        
        // Find which allocator owns this pointer
        for (auto& [node_id, allocator] : node_allocators_) {
            if (allocator->owns(ptr)) {
                allocator->deallocate(ptr, size);
                return;
            }
        }
    }
    
    [[nodiscard]] bool owns(const void* ptr) const noexcept {
        for (const auto& [node_id, allocator] : node_allocators_) {
            if (allocator->owns(ptr)) {
                return true;
            }
        }
        return false;
    }
    
    // Statistics per node
    struct NodeStats {
        size_t capacity;
        size_t used;
        size_t available;
        double utilization;
    };
    
    [[nodiscard]] std::unordered_map<uint32_t, NodeStats> get_node_statistics() const {
        std::unordered_map<uint32_t, NodeStats> stats;
        
        for (const auto& [node_id, allocator] : node_allocators_) {
            NodeStats node_stats;
            node_stats.capacity = allocator->capacity();
            node_stats.used = allocator->used();
            node_stats.available = allocator->available();
            node_stats.utilization = static_cast<double>(node_stats.used) / node_stats.capacity;
            
            stats[node_id] = node_stats;
        }
        
        return stats;
    }

private:
    size_t capacity_per_node_;
    std::unordered_map<uint32_t, std::unique_ptr<FreeListAllocator>> node_allocators_;
};

// ==== THREAD-SAFE ALLOCATOR WITH PER-THREAD POOLS ====
// High-performance allocator with thread-local caching
class ThreadSafeAllocator {
    static constexpr size_t CACHE_LINE_SIZE = 64;
    static constexpr size_t SMALL_OBJECT_THRESHOLD = 256;
    static constexpr size_t THREAD_CACHE_SIZE = 2 * 1024 * 1024; // 2MB per thread
    
    struct alignas(CACHE_LINE_SIZE) ThreadCache {
        LinearAllocator small_allocator{THREAD_CACHE_SIZE / 2};
        std::unique_ptr<ObjectPool<void*>> free_pointers;
        std::atomic<size_t> allocation_count{0};
        std::atomic<size_t> deallocation_count{0};
        
        ThreadCache() {
            free_pointers = std::make_unique<ObjectPool<void*>>(1024);
        }
    };
    
public:
    explicit ThreadSafeAllocator(size_t global_capacity = 256 * 1024 * 1024) // 256MB
        : global_allocator_(global_capacity) {}
    
    [[nodiscard]] void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) {
        // For small objects, use thread-local cache
        if (size <= SMALL_OBJECT_THRESHOLD) {
            ThreadCache& cache = get_thread_cache();
            
            if (auto ptr = cache.small_allocator.allocate(size, alignment)) {
                ++cache.allocation_count;
                return ptr;
            }
        }
        
        // For large objects or cache miss, use global allocator
        std::lock_guard<std::shared_mutex> lock(global_mutex_);
        auto ptr = global_allocator_.allocate(size, alignment);
        if (ptr) {
            ++global_allocation_count_;
        }
        return ptr;
    }
    
    void deallocate(void* ptr, size_t size) {
        if (!ptr) return;
        
        // Check if it belongs to thread cache
        ThreadCache& cache = get_thread_cache();
        if (cache.small_allocator.owns(ptr)) {
            // For linear allocator, we can't deallocate individual blocks
            // Instead, we track the deallocation for statistics
            ++cache.deallocation_count;
            return;
        }
        
        // Use global allocator
        std::lock_guard<std::shared_mutex> lock(global_mutex_);
        if (global_allocator_.owns(ptr)) {
            global_allocator_.deallocate(ptr, size);
            ++global_deallocation_count_;
        }
    }
    
    [[nodiscard]] bool owns(const void* ptr) const noexcept {
        // Check thread cache
        ThreadCache& cache = get_thread_cache();
        if (cache.small_allocator.owns(ptr)) {
            return true;
        }
        
        // Check global allocator
        std::shared_lock<std::shared_mutex> lock(global_mutex_);
        return global_allocator_.owns(ptr);
    }
    
    // Reset thread cache (useful for reducing memory pressure)
    void reset_thread_cache() {
        ThreadCache& cache = get_thread_cache();
        cache.small_allocator.reset();
        cache.allocation_count = 0;
        cache.deallocation_count = 0;
    }
    
    // Force collection of unused thread caches
    void collect_unused_caches() {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        
        for (auto it = thread_caches_.begin(); it != thread_caches_.end();) {
            // Check if thread still exists
            if (!is_thread_alive(it->first)) {
                it = thread_caches_.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    // Statistics
    struct Statistics {
        size_t global_capacity;
        size_t global_used;
        size_t global_allocations;
        size_t global_deallocations;
        size_t active_threads;
        size_t total_thread_cache_used;
        double global_utilization;
        double average_cache_utilization;
    };
    
    [[nodiscard]] Statistics get_statistics() const {
        Statistics stats{};
        
        // Global stats
        {
            std::shared_lock<std::shared_mutex> lock(global_mutex_);
            stats.global_capacity = global_allocator_.capacity();
            stats.global_used = global_allocator_.used();
            stats.global_utilization = static_cast<double>(stats.global_used) / stats.global_capacity;
        }
        
        stats.global_allocations = global_allocation_count_.load();
        stats.global_deallocations = global_deallocation_count_.load();
        
        // Thread cache stats
        {
            std::lock_guard<std::mutex> lock(cache_mutex_);
            stats.active_threads = thread_caches_.size();
            
            size_t total_cache_used = 0;
            for (const auto& [thread_id, cache] : thread_caches_) {
                total_cache_used += cache->small_allocator.used();
            }
            
            stats.total_thread_cache_used = total_cache_used;
            stats.average_cache_utilization = stats.active_threads > 0 ?
                static_cast<double>(total_cache_used) / (stats.active_threads * THREAD_CACHE_SIZE) : 0.0;
        }
        
        return stats;
    }

private:
    ThreadCache& get_thread_cache() const {
        thread_local static std::unique_ptr<ThreadCache> cache;
        
        if (!cache) {
            cache = std::make_unique<ThreadCache>();
            
            // Register cache for cleanup
            std::lock_guard<std::mutex> lock(cache_mutex_);
            thread_caches_[std::this_thread::get_id()] = cache.get();
        }
        
        return *cache;
    }
    
    [[nodiscard]] bool is_thread_alive(std::thread::id id) const {
        // Simplified thread liveness check
        // In practice, this would need platform-specific implementation
        return thread_caches_.find(id) != thread_caches_.end();
    }
    
    FreeListAllocator global_allocator_;
    mutable std::shared_mutex global_mutex_;
    
    mutable std::mutex cache_mutex_;
    mutable std::unordered_map<std::thread::id, ThreadCache*> thread_caches_;
    
    std::atomic<size_t> global_allocation_count_{0};
    std::atomic<size_t> global_deallocation_count_{0};
};

// ==== LOCK-FREE ALLOCATOR ====
// Ultra-high performance lock-free allocator for hot paths
template<size_t BlockSize = 64>
class LockFreeAllocator {
    static_assert((BlockSize & (BlockSize - 1)) == 0, "BlockSize must be power of 2");
    
    struct FreeBlock {
        std::atomic<FreeBlock*> next;
    };
    
public:
    explicit LockFreeAllocator(size_t capacity) 
        : capacity_(capacity), block_count_(capacity / BlockSize) {
        
        // Allocate memory
        const size_t total_size = block_count_ * BlockSize;
        memory_ = static_cast<uint8_t*>(std::aligned_alloc(BlockSize, total_size));
        
        if (!memory_) {
            throw std::bad_alloc{};
        }
        
        // Initialize free list
        for (size_t i = 0; i < block_count_; ++i) {
            auto block = reinterpret_cast<FreeBlock*>(memory_ + i * BlockSize);
            auto next = (i + 1 < block_count_) ? 
                       reinterpret_cast<FreeBlock*>(memory_ + (i + 1) * BlockSize) : nullptr;
            block->next.store(next, std::memory_order_relaxed);
        }
        
        free_head_.store(reinterpret_cast<FreeBlock*>(memory_), std::memory_order_relaxed);
    }
    
    ~LockFreeAllocator() {
        if (memory_) {
            std::free(memory_);
        }
    }
    
    // Lock-free allocation
    [[nodiscard]] void* allocate() noexcept {
        FreeBlock* head = free_head_.load(std::memory_order_acquire);
        
        while (head) {
            FreeBlock* next = head->next.load(std::memory_order_relaxed);
            
            if (free_head_.compare_exchange_weak(head, next, 
                                               std::memory_order_release,
                                               std::memory_order_acquire)) {
                return head;
            }
        }
        
        return nullptr; // Out of memory
    }
    
    // Lock-free deallocation
    void deallocate(void* ptr) noexcept {
        if (!owns(ptr)) return;
        
        auto block = static_cast<FreeBlock*>(ptr);
        FreeBlock* head = free_head_.load(std::memory_order_relaxed);
        
        do {
            block->next.store(head, std::memory_order_relaxed);
        } while (!free_head_.compare_exchange_weak(head, block,
                                                 std::memory_order_release,
                                                 std::memory_order_relaxed));
    }
    
    [[nodiscard]] bool owns(const void* ptr) const noexcept {
        if (!ptr) return false;
        const auto addr = reinterpret_cast<uintptr_t>(ptr);
        const auto start = reinterpret_cast<uintptr_t>(memory_);
        const auto end = start + capacity_;
        
        return addr >= start && addr < end && 
               ((addr - start) % BlockSize) == 0;
    }
    
    // Statistics (approximate due to lock-free nature)
    [[nodiscard]] size_t capacity() const noexcept { return capacity_; }
    [[nodiscard]] size_t block_size() const noexcept { return BlockSize; }
    [[nodiscard]] size_t block_count() const noexcept { return block_count_; }

private:
    uint8_t* memory_;
    size_t capacity_;
    size_t block_count_;
    std::atomic<FreeBlock*> free_head_;
};

} // namespace ecscope::memory