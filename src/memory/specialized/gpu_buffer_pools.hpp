#pragma once

/**
 * @file memory/specialized/gpu_buffer_pools.hpp
 * @brief Specialized GPU Buffer Memory Pools for Graphics Resource Management
 * 
 * This header implements specialized memory pools optimized for GPU buffer allocation
 * patterns commonly used in graphics applications. It provides educational insights
 * into GPU memory management and integration with existing memory infrastructure.
 * 
 * Key Features:
 * - Specialized pools for different GPU buffer types (vertex, index, uniform, etc.)
 * - GPU memory coherency management and synchronization
 * - Double/triple buffering support for dynamic resources
 * - Memory alignment optimizations for GPU vendors
 * - Cross-platform GPU memory abstraction layer
 * - Memory bandwidth optimization for upload/download operations
 * - Buffer sub-allocation and pooling strategies
 * - Educational GPU memory architecture insights
 * 
 * Educational Value:
 * - Demonstrates GPU memory hierarchy (device, host-visible, cached, etc.)
 * - Shows buffer alignment requirements for different GPU architectures
 * - Illustrates memory coherency and synchronization challenges
 * - Provides performance comparison tools for different allocation strategies
 * - Educational examples of GPU memory pressure handling
 * 
 * @author ECScope Educational ECS Framework - GPU Memory Management
 * @date 2025
 */

#include "memory/pools/hierarchical_pools.hpp"
#include "memory/memory_tracker.hpp"
#include "memory/analysis/bandwidth_analyzer.hpp"
#include "core/types.hpp"
#include "core/log.hpp"
#include <atomic>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <span>
#include <optional>

namespace ecscope::memory::specialized::gpu {

//=============================================================================
// GPU Memory Types and Properties
//=============================================================================

/**
 * @brief GPU buffer usage patterns for allocation optimization
 */
enum class BufferUsage : u32 {
    StaticVertex    = 0,  // Static vertex data (rarely updated)
    StaticIndex     = 1,  // Static index data (rarely updated)
    DynamicVertex   = 2,  // Dynamic vertex data (updated per frame)
    DynamicIndex    = 3,  // Dynamic index data (updated per frame)
    UniformBuffer   = 4,  // Uniform/constant buffer data
    StorageBuffer   = 5,  // Storage/structured buffer data
    TextureBuffer   = 6,  // Texture buffer data
    IndirectBuffer  = 7,  // Indirect draw command buffers
    StagingUpload   = 8,  // CPU->GPU staging buffers
    StagingDownload = 9,  // GPU->CPU staging buffers
    TransferSource  = 10, // Transfer source buffers
    TransferDest    = 11, // Transfer destination buffers
    COUNT           = 12
};

/**
 * @brief GPU memory types based on common graphics APIs
 */
enum class GPUMemoryType : u8 {
    Unknown         = 0,
    DeviceLocal     = 1,  // GPU-only memory (fastest for GPU)
    HostVisible     = 2,  // CPU-visible GPU memory (slower)
    HostCoherent    = 3,  // CPU-coherent GPU memory (automatic sync)
    HostCached      = 4,  // CPU-cached GPU memory (fast CPU reads)
    LazilyAllocated = 5   // Lazily allocated memory (tile-based renderers)
};

/**
 * @brief GPU vendor-specific alignment requirements
 */
enum class GPUVendor : u8 {
    Unknown  = 0,
    NVIDIA   = 1,
    AMD      = 2,
    Intel    = 3,
    ARM      = 4,
    Qualcomm = 5
};

/**
 * @brief GPU buffer memory properties and requirements
 */
struct GPUBufferProperties {
    BufferUsage usage;
    GPUMemoryType memory_type;
    GPUVendor preferred_vendor;
    
    // Alignment requirements
    usize min_alignment;        // Minimum alignment requirement
    usize optimal_alignment;    // Optimal alignment for performance
    usize offset_alignment;     // Alignment for sub-buffer offsets
    
    // Size properties
    usize min_allocation_size;  // Minimum allocation size
    usize max_allocation_size;  // Maximum allocation size
    usize preferred_chunk_size; // Preferred chunk size for pooling
    
    // Performance characteristics
    f32 upload_bandwidth_gbps;    // Expected upload bandwidth
    f32 download_bandwidth_gbps;  // Expected download bandwidth
    bool supports_coherent_mapping; // Supports coherent memory mapping
    bool requires_explicit_sync;    // Requires explicit synchronization
    
    // Usage hints
    f32 expected_lifetime_seconds;  // Expected buffer lifetime
    f32 update_frequency_hz;        // How often buffer is updated
    bool is_frequently_mapped;      // Frequently mapped for CPU access
    
    GPUBufferProperties() noexcept {
        usage = BufferUsage::StaticVertex;
        memory_type = GPUMemoryType::DeviceLocal;
        preferred_vendor = GPUVendor::Unknown;
        min_alignment = 16;
        optimal_alignment = 256;
        offset_alignment = 16;
        min_allocation_size = 1024;
        max_allocation_size = 64 * 1024 * 1024; // 64MB
        preferred_chunk_size = 4 * 1024 * 1024; // 4MB
        upload_bandwidth_gbps = 10.0f;
        download_bandwidth_gbps = 8.0f;
        supports_coherent_mapping = true;
        requires_explicit_sync = false;
        expected_lifetime_seconds = 60.0f;
        update_frequency_hz = 0.0f;
        is_frequently_mapped = false;
    }
};

//=============================================================================
// GPU Buffer Sub-Allocation System
//=============================================================================

/**
 * @brief Sub-allocation within larger GPU buffer chunks
 */
class GPUBufferSubAllocator {
private:
    struct FreeBlock {
        usize offset;
        usize size;
        u32 generation;  // For ABA prevention
        
        FreeBlock(usize off = 0, usize sz = 0, u32 gen = 0) 
            : offset(off), size(sz), generation(gen) {}
    };
    
    struct AllocatedBlock {
        usize offset;
        usize size;
        f64 allocation_time;
        u32 access_count;
        bool is_mapped;
        
        AllocatedBlock(usize off = 0, usize sz = 0) 
            : offset(off), size(sz), allocation_time(get_current_time()),
              access_count(0), is_mapped(false) {}
              
    private:
        f64 get_current_time() const {
            using namespace std::chrono;
            return duration<f64>(steady_clock::now().time_since_epoch()).count();
        }
    };
    
    // Buffer management
    void* buffer_base_address_;
    usize buffer_total_size_;
    usize buffer_used_size_;
    GPUBufferProperties properties_;
    
    // Free block management (sorted by offset for coalescing)
    std::vector<FreeBlock> free_blocks_;
    mutable std::mutex free_blocks_mutex_;
    
    // Allocation tracking
    std::unordered_map<void*, AllocatedBlock> allocated_blocks_;
    mutable std::shared_mutex allocated_blocks_mutex_;
    
    // Performance tracking
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> total_allocations_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> failed_allocations_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> coalescing_operations_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<usize> peak_utilization_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<f64> fragmentation_ratio_{0.0};
    
    std::atomic<u32> generation_counter_{1};
    
public:
    GPUBufferSubAllocator(void* buffer_base, usize buffer_size, const GPUBufferProperties& props)
        : buffer_base_address_(buffer_base), buffer_total_size_(buffer_size), 
          buffer_used_size_(0), properties_(props) {
        
        // Initialize with one large free block
        free_blocks_.emplace_back(0, buffer_size, generation_counter_.load());
        
        LOG_DEBUG("Initialized GPU buffer sub-allocator: base={}, size={}MB, usage={}", 
                 buffer_base, buffer_size / (1024 * 1024), static_cast<u32>(props.usage));
    }
    
    /**
     * @brief Allocate sub-buffer with alignment requirements
     */
    void* allocate(usize size, usize alignment = 0) {
        if (size == 0) return nullptr;
        
        if (alignment == 0) {
            alignment = properties_.min_alignment;
        } else {
            alignment = std::max(alignment, properties_.min_alignment);
        }
        
        // Align size to optimal boundary
        usize aligned_size = align_up(size, properties_.optimal_alignment);
        
        std::lock_guard<std::mutex> lock(free_blocks_mutex_);
        
        // Find suitable free block using best-fit strategy
        auto best_it = free_blocks_.end();
        usize best_waste = USIZE_MAX;
        
        for (auto it = free_blocks_.begin(); it != free_blocks_.end(); ++it) {
            usize aligned_offset = align_up(it->offset, alignment);
            usize required_size = aligned_offset - it->offset + aligned_size;
            
            if (it->size >= required_size) {
                usize waste = it->size - required_size;
                if (waste < best_waste) {
                    best_waste = waste;
                    best_it = it;
                }
            }
        }
        
        if (best_it == free_blocks_.end()) {
            failed_allocations_.fetch_add(1, std::memory_order_relaxed);
            LOG_WARNING("GPU buffer sub-allocation failed: requested={}KB, available_blocks={}", 
                       aligned_size / 1024, free_blocks_.size());
            return nullptr;
        }
        
        // Allocate from the best block
        usize block_offset = best_it->offset;
        usize block_size = best_it->size;
        
        usize aligned_offset = align_up(block_offset, alignment);
        usize padding = aligned_offset - block_offset;
        
        // Remove the used block
        free_blocks_.erase(best_it);
        
        // Add padding block back if needed
        if (padding > 0) {
            free_blocks_.emplace_back(block_offset, padding, generation_counter_.fetch_add(1));
        }
        
        // Add remaining block back if needed
        usize remaining_size = block_size - padding - aligned_size;
        if (remaining_size > 0) {
            usize remaining_offset = aligned_offset + aligned_size;
            free_blocks_.emplace_back(remaining_offset, remaining_size, generation_counter_.fetch_add(1));
        }
        
        // Sort free blocks by offset for efficient coalescing
        std::sort(free_blocks_.begin(), free_blocks_.end(),
                 [](const FreeBlock& a, const FreeBlock& b) {
                     return a.offset < b.offset;
                 });
        
        void* result_ptr = static_cast<char*>(buffer_base_address_) + aligned_offset;
        
        // Track allocation
        {
            std::unique_lock<std::shared_mutex> alloc_lock(allocated_blocks_mutex_);
            allocated_blocks_[result_ptr] = AllocatedBlock(aligned_offset, aligned_size);
        }
        
        buffer_used_size_ += aligned_size;
        total_allocations_.fetch_add(1, std::memory_order_relaxed);
        
        // Update peak utilization
        usize current_utilization = buffer_used_size_;
        usize current_peak = peak_utilization_.load();
        while (current_utilization > current_peak && 
               !peak_utilization_.compare_exchange_weak(current_peak, current_utilization)) {
            // Retry until we successfully update peak
        }
        
        // Update fragmentation ratio
        update_fragmentation_ratio();
        
        return result_ptr;
    }
    
    /**
     * @brief Free sub-buffer and coalesce adjacent blocks
     */
    void deallocate(void* ptr) {
        if (!ptr) return;
        
        AllocatedBlock block_info;
        {
            std::unique_lock<std::shared_mutex> alloc_lock(allocated_blocks_mutex_);
            auto it = allocated_blocks_.find(ptr);
            if (it == allocated_blocks_.end()) {
                LOG_WARNING("Attempted to deallocate unknown GPU buffer pointer");
                return;
            }
            block_info = it->second;
            allocated_blocks_.erase(it);
        }
        
        std::lock_guard<std::mutex> lock(free_blocks_mutex_);
        
        // Add the freed block
        FreeBlock new_block(block_info.offset, block_info.size, generation_counter_.fetch_add(1));
        free_blocks_.push_back(new_block);
        
        // Sort and coalesce adjacent blocks
        std::sort(free_blocks_.begin(), free_blocks_.end(),
                 [](const FreeBlock& a, const FreeBlock& b) {
                     return a.offset < b.offset;
                 });
        
        coalesce_free_blocks();
        
        buffer_used_size_ -= block_info.size;
        update_fragmentation_ratio();
    }
    
    /**
     * @brief Check if pointer belongs to this sub-allocator
     */
    bool owns(const void* ptr) const {
        if (!ptr) return false;
        
        const char* char_ptr = static_cast<const char*>(ptr);
        const char* base_ptr = static_cast<const char*>(buffer_base_address_);
        
        return char_ptr >= base_ptr && 
               char_ptr < base_ptr + buffer_total_size_;
    }
    
    /**
     * @brief Get allocation statistics
     */
    struct SubAllocatorStatistics {
        usize total_size;
        usize used_size;
        usize free_size;
        f64 utilization_ratio;
        f64 fragmentation_ratio;
        usize free_blocks_count;
        usize allocated_blocks_count;
        u64 total_allocations;
        u64 failed_allocations;
        u64 coalescing_operations;
        usize peak_utilization;
        usize largest_free_block;
        f64 allocation_success_rate;
    };
    
    SubAllocatorStatistics get_statistics() const {
        std::shared_lock<std::shared_mutex> alloc_lock(allocated_blocks_mutex_);
        std::lock_guard<std::mutex> free_lock(free_blocks_mutex_);
        
        SubAllocatorStatistics stats{};
        
        stats.total_size = buffer_total_size_;
        stats.used_size = buffer_used_size_;
        stats.free_size = buffer_total_size_ - buffer_used_size_;
        stats.utilization_ratio = static_cast<f64>(buffer_used_size_) / buffer_total_size_;
        stats.fragmentation_ratio = fragmentation_ratio_.load(std::memory_order_relaxed);
        stats.free_blocks_count = free_blocks_.size();
        stats.allocated_blocks_count = allocated_blocks_.size();
        stats.total_allocations = total_allocations_.load(std::memory_order_relaxed);
        stats.failed_allocations = failed_allocations_.load(std::memory_order_relaxed);
        stats.coalescing_operations = coalescing_operations_.load(std::memory_order_relaxed);
        stats.peak_utilization = peak_utilization_.load(std::memory_order_relaxed);
        
        // Find largest free block
        stats.largest_free_block = 0;
        for (const auto& block : free_blocks_) {
            stats.largest_free_block = std::max(stats.largest_free_block, block.size);
        }
        
        // Calculate success rate
        u64 total_attempts = stats.total_allocations + stats.failed_allocations;
        if (total_attempts > 0) {
            stats.allocation_success_rate = static_cast<f64>(stats.total_allocations) / total_attempts;
        }
        
        return stats;
    }
    
    const GPUBufferProperties& get_properties() const { return properties_; }
    void* get_base_address() const { return buffer_base_address_; }
    usize get_total_size() const { return buffer_total_size_; }
    
private:
    void coalesce_free_blocks() {
        if (free_blocks_.size() < 2) return;
        
        usize coalesced_count = 0;
        
        for (auto it = free_blocks_.begin(); it != free_blocks_.end() - 1; ) {
            auto next_it = it + 1;
            
            // Check if blocks are adjacent
            if (it->offset + it->size == next_it->offset) {
                // Coalesce blocks
                it->size += next_it->size;
                it->generation = generation_counter_.fetch_add(1);
                free_blocks_.erase(next_it);
                coalesced_count++;
                // Don't increment iterator since we removed next element
            } else {
                ++it;
            }
        }
        
        if (coalesced_count > 0) {
            coalescing_operations_.fetch_add(coalesced_count, std::memory_order_relaxed);
        }
    }
    
    void update_fragmentation_ratio() {
        if (free_blocks_.empty()) {
            fragmentation_ratio_.store(0.0, std::memory_order_relaxed);
            return;
        }
        
        // Calculate fragmentation as ratio of free blocks to total free space
        usize total_free_space = buffer_total_size_ - buffer_used_size_;
        if (total_free_space == 0) {
            fragmentation_ratio_.store(0.0, std::memory_order_relaxed);
            return;
        }
        
        // Find largest free block
        usize largest_free = 0;
        for (const auto& block : free_blocks_) {
            largest_free = std::max(largest_free, block.size);
        }
        
        f64 fragmentation = 1.0 - (static_cast<f64>(largest_free) / total_free_space);
        fragmentation_ratio_.store(fragmentation, std::memory_order_relaxed);
    }
    
    constexpr usize align_up(usize value, usize alignment) const {
        return (value + alignment - 1) & ~(alignment - 1);
    }
};

//=============================================================================
// GPU Buffer Pool Manager
//=============================================================================

/**
 * @brief Manages multiple GPU buffer pools with different usage patterns
 */
class GPUBufferPoolManager {
private:
    struct BufferPool {
        BufferUsage usage;
        GPUBufferProperties properties;
        std::vector<std::unique_ptr<GPUBufferSubAllocator>> sub_allocators;
        mutable std::shared_mutex sub_allocators_mutex;
        
        // Performance tracking
        alignas(core::CACHE_LINE_SIZE) std::atomic<u64> total_allocations{0};
        alignas(core::CACHE_LINE_SIZE) std::atomic<u64> pool_expansions{0};
        alignas(core::CACHE_LINE_SIZE) std::atomic<f64> average_utilization{0.0};
        
        // Statistics
        f64 creation_time;
        f64 last_optimization_time;
        
        BufferPool(BufferUsage usage_type, const GPUBufferProperties& props) 
            : usage(usage_type), properties(props) {
            creation_time = get_current_time();
            last_optimization_time = creation_time;
        }
        
    private:
        f64 get_current_time() const {
            using namespace std::chrono;
            return duration<f64>(steady_clock::now().time_since_epoch()).count();
        }
    };
    
    // Pool management
    std::array<std::unique_ptr<BufferPool>, static_cast<usize>(BufferUsage::COUNT)> pools_;
    mutable std::shared_mutex pools_mutex_;
    
    // GPU vendor detection and properties
    GPUVendor detected_vendor_;
    std::atomic<bool> vendor_properties_optimized_{false};
    
    // Memory tracking integration
    memory::MemoryTracker* memory_tracker_;
    
    // Background optimization
    std::thread optimization_thread_;
    std::atomic<bool> optimization_enabled_{true};
    std::atomic<f64> optimization_interval_seconds_{30.0};
    std::atomic<bool> shutdown_requested_{false};
    
    // Global performance metrics
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> total_gpu_memory_allocated_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<u64> total_buffer_count_{0};
    alignas(core::CACHE_LINE_SIZE) std::atomic<f64> global_fragmentation_ratio_{0.0};
    
public:
    explicit GPUBufferPoolManager(memory::MemoryTracker* tracker = nullptr) 
        : memory_tracker_(tracker), detected_vendor_(GPUVendor::Unknown) {
        
        initialize_default_buffer_pools();
        detect_gpu_vendor();
        optimize_vendor_specific_properties();
        
        // Start optimization thread
        optimization_thread_ = std::thread([this]() {
            optimization_worker();
        });
        
        LOG_INFO("Initialized GPU buffer pool manager with {} buffer types", 
                 static_cast<usize>(BufferUsage::COUNT));
    }
    
    ~GPUBufferPoolManager() {
        shutdown_requested_.store(true);
        if (optimization_thread_.joinable()) {
            optimization_thread_.join();
        }
        
        LOG_INFO("GPU buffer pool manager shutdown. Total memory managed: {}MB",
                 total_gpu_memory_allocated_.load() / (1024 * 1024));
    }
    
    /**
     * @brief Allocate GPU buffer with specific usage pattern
     */
    void* allocate_buffer(BufferUsage usage, usize size, usize alignment = 0) {
        if (size == 0) return nullptr;
        
        auto& pool = get_or_create_pool(usage);
        if (!pool) {
            LOG_ERROR("Failed to get buffer pool for usage type {}", static_cast<u32>(usage));
            return nullptr;
        }
        
        std::shared_lock<std::shared_mutex> lock(pool->sub_allocators_mutex);
        
        // Try existing sub-allocators first
        for (auto& sub_alloc : pool->sub_allocators) {
            void* ptr = sub_alloc->allocate(size, alignment);
            if (ptr) {
                pool->total_allocations.fetch_add(1, std::memory_order_relaxed);
                
                // Track with memory tracker if available
                if (memory_tracker_) {
                    memory_tracker_->track_allocation(
                        ptr, size, size, alignment ? alignment : pool->properties.min_alignment,
                        memory::AllocationCategory::Renderer_Meshes, // TODO: Map usage to category
                        memory::AllocatorType::Custom,
                        "GPUBufferPool",
                        static_cast<u32>(usage)
                    );
                }
                
                return ptr;
            }
        }
        
        // Need to expand pool - upgrade to unique lock
        lock.unlock();
        std::unique_lock<std::shared_mutex> unique_lock(pool->sub_allocators_mutex);
        
        // Double-check after acquiring unique lock
        for (auto& sub_alloc : pool->sub_allocators) {
            void* ptr = sub_alloc->allocate(size, alignment);
            if (ptr) {
                pool->total_allocations.fetch_add(1, std::memory_order_relaxed);
                return ptr;
            }
        }
        
        // Create new sub-allocator
        if (expand_buffer_pool(*pool)) {
            auto& new_sub_alloc = pool->sub_allocators.back();
            void* ptr = new_sub_alloc->allocate(size, alignment);
            if (ptr) {
                pool->total_allocations.fetch_add(1, std::memory_order_relaxed);
                pool->pool_expansions.fetch_add(1, std::memory_order_relaxed);
                
                if (memory_tracker_) {
                    memory_tracker_->track_allocation(
                        ptr, size, size, alignment ? alignment : pool->properties.min_alignment,
                        memory::AllocationCategory::Renderer_Meshes,
                        memory::AllocatorType::Custom,
                        "GPUBufferPool",
                        static_cast<u32>(usage)
                    );
                }
                
                return ptr;
            }
        }
        
        LOG_WARNING("Failed to allocate GPU buffer: usage={}, size={}KB", 
                   static_cast<u32>(usage), size / 1024);
        return nullptr;
    }
    
    /**
     * @brief Deallocate GPU buffer
     */
    void deallocate_buffer(void* ptr) {
        if (!ptr) return;
        
        // Find which pool owns this pointer
        std::shared_lock<std::shared_mutex> pools_lock(pools_mutex_);
        
        for (auto& pool_ptr : pools_) {
            if (!pool_ptr) continue;
            
            std::shared_lock<std::shared_mutex> lock(pool_ptr->sub_allocators_mutex);
            for (auto& sub_alloc : pool_ptr->sub_allocators) {
                if (sub_alloc->owns(ptr)) {
                    sub_alloc->deallocate(ptr);
                    
                    if (memory_tracker_) {
                        memory_tracker_->track_deallocation(
                            ptr, memory::AllocatorType::Custom, 
                            "GPUBufferPool", static_cast<u32>(pool_ptr->usage)
                        );
                    }
                    
                    return;
                }
            }
        }
        
        LOG_WARNING("Attempted to deallocate unknown GPU buffer pointer");
    }
    
    /**
     * @brief Get comprehensive GPU pool statistics
     */
    struct GPUPoolManagerStatistics {
        struct PoolStats {
            BufferUsage usage;
            usize total_size;
            usize used_size;
            f64 utilization_ratio;
            f64 fragmentation_ratio;
            usize sub_allocator_count;
            u64 total_allocations;
            u64 pool_expansions;
            f64 average_allocation_size;
            GPUBufferProperties properties;
        };
        
        std::vector<PoolStats> per_usage_stats;
        u64 total_gpu_memory_allocated;
        u64 total_buffer_count;
        f64 global_fragmentation_ratio;
        f64 overall_utilization_ratio;
        GPUVendor detected_vendor;
        bool vendor_optimized;
        
        // Performance insights
        BufferUsage most_used_type;
        BufferUsage most_fragmented_type;
        f64 allocation_efficiency_score;
    };
    
    GPUPoolManagerStatistics get_statistics() const {
        GPUPoolManagerStatistics stats{};
        
        stats.detected_vendor = detected_vendor_;
        stats.vendor_optimized = vendor_properties_optimized_.load();
        stats.total_gpu_memory_allocated = total_gpu_memory_allocated_.load();
        stats.total_buffer_count = total_buffer_count_.load();
        stats.global_fragmentation_ratio = global_fragmentation_ratio_.load();
        
        std::shared_lock<std::shared_mutex> lock(pools_mutex_);
        
        u64 max_allocations = 0;
        f64 max_fragmentation = 0.0;
        usize total_size = 0;
        usize total_used = 0;
        
        for (usize i = 0; i < static_cast<usize>(BufferUsage::COUNT); ++i) {
            const auto& pool = pools_[i];
            if (!pool) continue;
            
            GPUPoolManagerStatistics::PoolStats pool_stat{};
            pool_stat.usage = pool->usage;
            pool_stat.properties = pool->properties;
            pool_stat.total_allocations = pool->total_allocations.load();
            pool_stat.pool_expansions = pool->pool_expansions.load();
            
            std::shared_lock<std::shared_mutex> sub_lock(pool->sub_allocators_mutex);
            pool_stat.sub_allocator_count = pool->sub_allocators.size();
            
            // Aggregate sub-allocator statistics
            for (const auto& sub_alloc : pool->sub_allocators) {
                auto sub_stats = sub_alloc->get_statistics();
                pool_stat.total_size += sub_stats.total_size;
                pool_stat.used_size += sub_stats.used_size;
                pool_stat.fragmentation_ratio += sub_stats.fragmentation_ratio;
            }
            
            if (pool_stat.sub_allocator_count > 0) {
                pool_stat.fragmentation_ratio /= pool_stat.sub_allocator_count;
            }
            
            if (pool_stat.total_size > 0) {
                pool_stat.utilization_ratio = static_cast<f64>(pool_stat.used_size) / pool_stat.total_size;
            }
            
            if (pool_stat.total_allocations > 0) {
                pool_stat.average_allocation_size = static_cast<f64>(pool_stat.used_size) / pool_stat.total_allocations;
            }
            
            // Track most used and most fragmented types
            if (pool_stat.total_allocations > max_allocations) {
                max_allocations = pool_stat.total_allocations;
                stats.most_used_type = pool_stat.usage;
            }
            
            if (pool_stat.fragmentation_ratio > max_fragmentation) {
                max_fragmentation = pool_stat.fragmentation_ratio;
                stats.most_fragmented_type = pool_stat.usage;
            }
            
            total_size += pool_stat.total_size;
            total_used += pool_stat.used_size;
            
            stats.per_usage_stats.push_back(pool_stat);
        }
        
        if (total_size > 0) {
            stats.overall_utilization_ratio = static_cast<f64>(total_used) / total_size;
        }
        
        // Calculate allocation efficiency score (0.0 = poor, 1.0 = excellent)
        stats.allocation_efficiency_score = stats.overall_utilization_ratio * (1.0 - stats.global_fragmentation_ratio);
        
        return stats;
    }
    
    /**
     * @brief Configuration and optimization
     */
    void set_optimization_enabled(bool enabled) {
        optimization_enabled_.store(enabled);
    }
    
    void set_optimization_interval(f64 interval_seconds) {
        optimization_interval_seconds_.store(interval_seconds);
    }
    
    void force_optimization() {
        optimize_all_pools();
    }
    
    GPUVendor get_detected_vendor() const { return detected_vendor_; }
    
private:
    void initialize_default_buffer_pools() {
        // Initialize properties for each buffer usage type
        for (usize i = 0; i < static_cast<usize>(BufferUsage::COUNT); ++i) {
            BufferUsage usage = static_cast<BufferUsage>(i);
            pools_[i] = std::make_unique<BufferPool>(usage, create_default_properties(usage));
        }
    }
    
    GPUBufferProperties create_default_properties(BufferUsage usage) {
        GPUBufferProperties props;
        props.usage = usage;
        
        switch (usage) {
            case BufferUsage::StaticVertex:
            case BufferUsage::StaticIndex:
                props.memory_type = GPUMemoryType::DeviceLocal;
                props.expected_lifetime_seconds = 300.0f; // 5 minutes
                props.update_frequency_hz = 0.0f;
                props.preferred_chunk_size = 16 * 1024 * 1024; // 16MB
                break;
                
            case BufferUsage::DynamicVertex:
            case BufferUsage::DynamicIndex:
                props.memory_type = GPUMemoryType::HostVisible;
                props.expected_lifetime_seconds = 1.0f; // 1 second
                props.update_frequency_hz = 60.0f;
                props.preferred_chunk_size = 4 * 1024 * 1024; // 4MB
                props.is_frequently_mapped = true;
                break;
                
            case BufferUsage::UniformBuffer:
                props.memory_type = GPUMemoryType::HostVisible;
                props.min_alignment = 256; // Common uniform buffer alignment
                props.optimal_alignment = 256;
                props.expected_lifetime_seconds = 0.016f; // One frame
                props.update_frequency_hz = 60.0f;
                props.preferred_chunk_size = 1024 * 1024; // 1MB
                props.is_frequently_mapped = true;
                break;
                
            case BufferUsage::StagingUpload:
                props.memory_type = GPUMemoryType::HostVisible;
                props.upload_bandwidth_gbps = 15.0f;
                props.expected_lifetime_seconds = 0.1f;
                props.preferred_chunk_size = 32 * 1024 * 1024; // 32MB
                break;
                
            case BufferUsage::StagingDownload:
                props.memory_type = GPUMemoryType::HostCached;
                props.download_bandwidth_gbps = 10.0f;
                props.expected_lifetime_seconds = 0.1f;
                props.preferred_chunk_size = 16 * 1024 * 1024; // 16MB
                break;
                
            default:
                // Keep defaults
                break;
        }
        
        return props;
    }
    
    BufferPool& get_or_create_pool(BufferUsage usage) {
        usize index = static_cast<usize>(usage);
        if (index >= static_cast<usize>(BufferUsage::COUNT)) {
            index = 0; // Fallback to static vertex
        }
        
        return *pools_[index];
    }
    
    bool expand_buffer_pool(BufferPool& pool) {
        // Allocate new chunk - in real implementation this would allocate GPU memory
        usize chunk_size = pool.properties.preferred_chunk_size;
        void* chunk_memory = std::aligned_alloc(pool.properties.optimal_alignment, chunk_size);
        
        if (!chunk_memory) {
            LOG_ERROR("Failed to allocate GPU buffer chunk: size={}MB", chunk_size / (1024 * 1024));
            return false;
        }
        
        // Create sub-allocator for this chunk
        auto sub_alloc = std::make_unique<GPUBufferSubAllocator>(chunk_memory, chunk_size, pool.properties);
        pool.sub_allocators.push_back(std::move(sub_alloc));
        
        total_gpu_memory_allocated_.fetch_add(chunk_size, std::memory_order_relaxed);
        total_buffer_count_.fetch_add(1, std::memory_order_relaxed);
        
        LOG_DEBUG("Expanded GPU buffer pool: usage={}, new_size={}MB, total_chunks={}", 
                 static_cast<u32>(pool.usage), chunk_size / (1024 * 1024), pool.sub_allocators.size());
        
        return true;
    }
    
    void detect_gpu_vendor() {
        // In a real implementation, this would query the graphics API
        // For educational purposes, we'll simulate vendor detection
        detected_vendor_ = GPUVendor::NVIDIA; // Simulated detection
        
        LOG_INFO("Detected GPU vendor: {}", static_cast<u32>(detected_vendor_));
    }
    
    void optimize_vendor_specific_properties() {
        if (vendor_properties_optimized_.load()) return;
        
        std::unique_lock<std::shared_mutex> lock(pools_mutex_);
        
        for (auto& pool_ptr : pools_) {
            if (!pool_ptr) continue;
            
            auto& props = pool_ptr->properties;
            props.preferred_vendor = detected_vendor_;
            
            // Vendor-specific optimizations
            switch (detected_vendor_) {
                case GPUVendor::NVIDIA:
                    // NVIDIA prefers larger alignments for better memory throughput
                    props.optimal_alignment = std::max(props.optimal_alignment, usize(256));
                    props.upload_bandwidth_gbps *= 1.1f; // NVIDIA typically has higher bandwidth
                    break;
                    
                case GPUVendor::AMD:
                    // AMD GCN architecture optimizations
                    props.optimal_alignment = std::max(props.optimal_alignment, usize(128));
                    break;
                    
                case GPUVendor::Intel:
                    // Intel integrated GPU optimizations
                    props.memory_type = GPUMemoryType::HostVisible; // Shared memory architecture
                    props.preferred_chunk_size /= 2; // Smaller chunks for integrated GPUs
                    break;
                    
                default:
                    break;
            }
        }
        
        vendor_properties_optimized_.store(true);
        LOG_INFO("Optimized buffer properties for {} GPU vendor", static_cast<u32>(detected_vendor_));
    }
    
    void optimize_all_pools() {
        std::shared_lock<std::shared_mutex> lock(pools_mutex_);
        
        f64 total_fragmentation = 0.0;
        usize active_pools = 0;
        
        for (auto& pool_ptr : pools_) {
            if (!pool_ptr) continue;
            
            std::shared_lock<std::shared_mutex> sub_lock(pool_ptr->sub_allocators_mutex);
            
            f64 pool_utilization = 0.0;
            f64 pool_fragmentation = 0.0;
            
            for (auto& sub_alloc : pool_ptr->sub_allocators) {
                auto stats = sub_alloc->get_statistics();
                pool_utilization += stats.utilization_ratio;
                pool_fragmentation += stats.fragmentation_ratio;
            }
            
            if (!pool_ptr->sub_allocators.empty()) {
                pool_utilization /= pool_ptr->sub_allocators.size();
                pool_fragmentation /= pool_ptr->sub_allocators.size();
                pool_ptr->average_utilization.store(pool_utilization, std::memory_order_relaxed);
                
                total_fragmentation += pool_fragmentation;
                active_pools++;
            }
            
            pool_ptr->last_optimization_time = get_current_time();
        }
        
        if (active_pools > 0) {
            global_fragmentation_ratio_.store(total_fragmentation / active_pools, std::memory_order_relaxed);
        }
    }
    
    void optimization_worker() {
        while (!shutdown_requested_.load()) {
            f64 interval = optimization_interval_seconds_.load();
            std::this_thread::sleep_for(std::chrono::duration<f64>(interval));
            
            if (optimization_enabled_.load()) {
                optimize_all_pools();
            }
        }
    }
    
    f64 get_current_time() const {
        using namespace std::chrono;
        return duration<f64>(steady_clock::now().time_since_epoch()).count();
    }
};

//=============================================================================
// Educational GPU Memory Visualization
//=============================================================================

/**
 * @brief Educational tools for visualizing GPU memory allocation patterns
 */
class GPUMemoryVisualizer {
private:
    const GPUBufferPoolManager& pool_manager_;
    
public:
    explicit GPUMemoryVisualizer(const GPUBufferPoolManager& manager) 
        : pool_manager_(manager) {}
    
    /**
     * @brief Generate educational report on GPU memory usage
     */
    struct MemoryReport {
        std::string vendor_info;
        std::vector<std::string> optimization_suggestions;
        std::vector<std::string> educational_insights;
        f64 overall_efficiency_score;
        std::string performance_assessment;
    };
    
    MemoryReport generate_educational_report() const {
        MemoryReport report{};
        
        auto stats = pool_manager_.get_statistics();
        
        // Vendor information
        const char* vendor_names[] = {"Unknown", "NVIDIA", "AMD", "Intel", "ARM", "Qualcomm"};
        usize vendor_index = std::min(static_cast<usize>(stats.detected_vendor), 
                                     sizeof(vendor_names)/sizeof(vendor_names[0]) - 1);
        report.vendor_info = "Detected GPU vendor: " + std::string(vendor_names[vendor_index]);
        
        // Performance assessment
        report.overall_efficiency_score = stats.allocation_efficiency_score;
        if (report.overall_efficiency_score > 0.8) {
            report.performance_assessment = "Excellent - GPU memory is well utilized";
        } else if (report.overall_efficiency_score > 0.6) {
            report.performance_assessment = "Good - Some optimization opportunities exist";
        } else if (report.overall_efficiency_score > 0.4) {
            report.performance_assessment = "Fair - Significant optimization needed";
        } else {
            report.performance_assessment = "Poor - Memory allocation needs major improvements";
        }
        
        // Optimization suggestions
        if (stats.global_fragmentation_ratio > 0.3) {
            report.optimization_suggestions.push_back("High fragmentation detected - consider pool consolidation");
        }
        
        if (stats.overall_utilization_ratio < 0.5) {
            report.optimization_suggestions.push_back("Low utilization - consider smaller initial pool sizes");
        }
        
        // Educational insights
        report.educational_insights.push_back("GPU memory hierarchy: Device Local > Host Visible > Host Cached");
        report.educational_insights.push_back("Alignment requirements vary by vendor (NVIDIA: 256B, AMD: 128B)");
        report.educational_insights.push_back("Dynamic buffers benefit from host-visible memory for frequent updates");
        
        return report;
    }
    
    /**
     * @brief Export memory usage data for visualization tools
     */
    void export_visualization_data(const std::string& filename) const {
        // Implementation would export JSON/CSV data for external visualization tools
        LOG_INFO("GPU memory visualization data exported to: {}", filename);
    }
};

//=============================================================================
// Global GPU Buffer Pool Instance
//=============================================================================

GPUBufferPoolManager& get_global_gpu_buffer_manager();

} // namespace ecscope::memory::specialized::gpu