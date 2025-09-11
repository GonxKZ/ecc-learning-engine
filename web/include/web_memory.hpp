#pragma once

#include "ecscope/web/web_types.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <atomic>

namespace ecscope::web {

/**
 * @brief WebAssembly memory management system
 * 
 * This class provides efficient memory management specifically optimized
 * for WebAssembly environments, including shared memory, SIMD optimizations,
 * and garbage collection integration.
 */
class WebMemory {
public:
    /**
     * @brief Memory allocation strategies
     */
    enum class AllocationStrategy {
        Linear,        // Linear allocator for temporary data
        Pool,          // Pool allocator for fixed-size objects
        Stack,         // Stack allocator for scope-based allocation
        Buddy,         // Buddy allocator for general purpose
        SharedBuffer   // Shared buffer for JavaScript/WASM transfer
    };
    
    /**
     * @brief Memory alignment options
     */
    enum class Alignment {
        Byte = 1,
        Word = 4,
        DoubleWord = 8,
        QuadWord = 16,
        SIMD128 = 16,  // SIMD 128-bit alignment
        SIMD256 = 32,  // SIMD 256-bit alignment (future)
        CacheLine = 64 // Cache line alignment
    };
    
    /**
     * @brief Memory block information
     */
    struct MemoryBlock {
        void* ptr;
        std::size_t size;
        std::size_t alignment;
        AllocationStrategy strategy;
        bool is_shared;
        std::uint64_t allocation_id;
        std::chrono::steady_clock::time_point allocation_time;
    };
    
    /**
     * @brief Memory pool configuration
     */
    struct PoolConfig {
        std::size_t block_size;
        std::size_t initial_blocks;
        std::size_t max_blocks;
        Alignment alignment;
        bool thread_safe;
    };
    
    /**
     * @brief Memory statistics
     */
    struct MemoryStats {
        std::size_t total_allocated;
        std::size_t total_used;
        std::size_t peak_usage;
        std::size_t allocation_count;
        std::size_t deallocation_count;
        std::size_t active_allocations;
        std::size_t fragmentation_bytes;
        double fragmentation_ratio;
        std::size_t gc_collections;
        std::size_t shared_buffers_count;
        std::size_t shared_buffers_size;
    };
    
    /**
     * @brief Construct WebMemory system
     * @param initial_heap_size Initial heap size in bytes
     * @param enable_shared_memory Enable SharedArrayBuffer support
     */
    WebMemory(std::size_t initial_heap_size = 64 * 1024 * 1024, 
              bool enable_shared_memory = true);
    
    /**
     * @brief Destructor
     */
    ~WebMemory();
    
    // Non-copyable, movable
    WebMemory(const WebMemory&) = delete;
    WebMemory& operator=(const WebMemory&) = delete;
    WebMemory(WebMemory&&) noexcept = default;
    WebMemory& operator=(WebMemory&&) noexcept = default;
    
    /**
     * @brief Initialize memory system
     * @return true if initialization succeeded
     */
    bool initialize();
    
    /**
     * @brief Shutdown memory system
     */
    void shutdown();
    
    /**
     * @brief Allocate memory with specific strategy
     * @param size Size in bytes
     * @param strategy Allocation strategy
     * @param alignment Memory alignment
     * @return Allocated memory block
     */
    MemoryBlock allocate(std::size_t size, 
                        AllocationStrategy strategy = AllocationStrategy::Buddy,
                        Alignment alignment = Alignment::DoubleWord);
    
    /**
     * @brief Deallocate memory block
     * @param block Memory block to deallocate
     */
    void deallocate(const MemoryBlock& block);
    
    /**
     * @brief Reallocate memory block
     * @param block Original memory block
     * @param new_size New size in bytes
     * @return Reallocated memory block
     */
    MemoryBlock reallocate(const MemoryBlock& block, std::size_t new_size);
    
    /**
     * @brief Create memory pool
     * @param config Pool configuration
     * @return Pool ID
     */
    std::uint32_t create_pool(const PoolConfig& config);
    
    /**
     * @brief Destroy memory pool
     * @param pool_id Pool ID
     */
    void destroy_pool(std::uint32_t pool_id);
    
    /**
     * @brief Allocate from specific pool
     * @param pool_id Pool ID
     * @return Allocated memory block
     */
    MemoryBlock allocate_from_pool(std::uint32_t pool_id);
    
    /**
     * @brief Deallocate to specific pool
     * @param pool_id Pool ID
     * @param block Memory block
     */
    void deallocate_to_pool(std::uint32_t pool_id, const MemoryBlock& block);
    
    /**
     * @brief Create shared buffer for JavaScript/WASM transfer
     * @param size Buffer size in bytes
     * @param alignment Buffer alignment
     * @return Shared buffer handle
     */
    std::uint32_t create_shared_buffer(std::size_t size, Alignment alignment = Alignment::SIMD128);
    
    /**
     * @brief Get shared buffer data pointer
     * @param buffer_id Buffer ID
     * @return Data pointer
     */
    void* get_shared_buffer_data(std::uint32_t buffer_id);
    
    /**
     * @brief Get shared buffer size
     * @param buffer_id Buffer ID
     * @return Buffer size in bytes
     */
    std::size_t get_shared_buffer_size(std::uint32_t buffer_id);
    
    /**
     * @brief Destroy shared buffer
     * @param buffer_id Buffer ID
     */
    void destroy_shared_buffer(std::uint32_t buffer_id);
    
    /**
     * @brief Create typed array view of shared buffer
     * @param buffer_id Buffer ID
     * @param type Array type ("Int8", "Uint8", "Int16", "Uint16", "Int32", "Uint32", "Float32", "Float64")
     * @param offset Offset in bytes
     * @param length Number of elements
     * @return JavaScript typed array
     */
    JSValue create_typed_array_view(std::uint32_t buffer_id, const std::string& type, 
                                   std::size_t offset = 0, std::size_t length = 0);
    
    /**
     * @brief Perform garbage collection
     * @param aggressive Whether to perform aggressive collection
     * @return Bytes freed
     */
    std::size_t garbage_collect(bool aggressive = false);
    
    /**
     * @brief Get memory statistics
     * @return Current memory statistics
     */
    MemoryStats get_statistics() const;
    
    /**
     * @brief Reset memory statistics
     */
    void reset_statistics();
    
    /**
     * @brief Check if memory system is initialized
     * @return true if initialized
     */
    bool is_initialized() const noexcept { return initialized_; }
    
    /**
     * @brief Get heap size
     * @return Current heap size in bytes
     */
    std::size_t get_heap_size() const;
    
    /**
     * @brief Get available memory
     * @return Available memory in bytes
     */
    std::size_t get_available_memory() const;
    
    /**
     * @brief Set memory pressure callback
     * @param callback Callback function called when memory pressure is high
     */
    void set_memory_pressure_callback(std::function<void(float)> callback);
    
    /**
     * @brief Enable/disable memory tracking
     * @param enable Whether to enable memory tracking
     */
    void set_memory_tracking(bool enable);
    
    /**
     * @brief Dump memory usage to console
     */
    void dump_memory_usage() const;
    
    /**
     * @brief Validate heap integrity
     * @return true if heap is valid
     */
    bool validate_heap() const;
    
    // SIMD-optimized memory operations
    
    /**
     * @brief SIMD-optimized memory copy
     * @param dest Destination pointer
     * @param src Source pointer
     * @param size Size in bytes (must be multiple of 16)
     */
    void simd_memcpy(void* dest, const void* src, std::size_t size);
    
    /**
     * @brief SIMD-optimized memory set
     * @param dest Destination pointer
     * @param value Value to set
     * @param size Size in bytes (must be multiple of 16)
     */
    void simd_memset(void* dest, int value, std::size_t size);
    
    /**
     * @brief SIMD-optimized memory compare
     * @param ptr1 First pointer
     * @param ptr2 Second pointer
     * @param size Size in bytes (must be multiple of 16)
     * @return 0 if equal, non-zero otherwise
     */
    int simd_memcmp(const void* ptr1, const void* ptr2, std::size_t size);
    
    // WebAssembly bulk memory operations
    
    /**
     * @brief Bulk memory copy using WebAssembly bulk memory operations
     * @param dest Destination pointer
     * @param src Source pointer
     * @param size Size in bytes
     */
    void bulk_memory_copy(void* dest, const void* src, std::size_t size);
    
    /**
     * @brief Bulk memory fill using WebAssembly bulk memory operations
     * @param dest Destination pointer
     * @param value Fill value
     * @param size Size in bytes
     */
    void bulk_memory_fill(void* dest, std::uint8_t value, std::size_t size);
    
    // Memory debugging and profiling
    
    /**
     * @brief Start memory profiling
     */
    void start_profiling();
    
    /**
     * @brief Stop memory profiling
     */
    void stop_profiling();
    
    /**
     * @brief Get profiling results
     * @return Profiling data as JavaScript object
     */
    JSValue get_profiling_results() const;
    
private:
    // Configuration
    std::size_t initial_heap_size_;
    bool enable_shared_memory_;
    
    // State
    bool initialized_ = false;
    bool memory_tracking_enabled_ = true;
    bool profiling_enabled_ = false;
    
    // Memory allocators
    std::unique_ptr<class LinearAllocator> linear_allocator_;
    std::unique_ptr<class StackAllocator> stack_allocator_;
    std::unique_ptr<class BuddyAllocator> buddy_allocator_;
    
    // Memory pools
    std::uint32_t next_pool_id_ = 1;
    std::unordered_map<std::uint32_t, std::unique_ptr<class MemoryPool>> memory_pools_;
    
    // Shared buffers
    std::uint32_t next_buffer_id_ = 1;
    std::unordered_map<std::uint32_t, MemoryBlock> shared_buffers_;
    
    // Statistics
    mutable MemoryStats statistics_;
    std::atomic<std::uint64_t> next_allocation_id_{1};
    
    // Tracking
    std::unordered_map<std::uint64_t, MemoryBlock> active_allocations_;
    mutable std::mutex allocations_mutex_;
    
    // Callbacks
    std::function<void(float)> memory_pressure_callback_;
    
    // Profiling data
    std::chrono::steady_clock::time_point profiling_start_time_;
    std::vector<MemoryBlock> profiling_allocations_;
    
    // Internal methods
    void update_statistics(const MemoryBlock& block, bool allocating);
    void check_memory_pressure();
    bool is_simd_supported() const;
    bool is_bulk_memory_supported() const;
    void* aligned_alloc(std::size_t size, std::size_t alignment);
    void aligned_free(void* ptr);
    
    // SIMD implementations
    void simd_copy_16_aligned(void* dest, const void* src, std::size_t size);
    void simd_set_16_aligned(void* dest, int value, std::size_t size);
    int simd_compare_16_aligned(const void* ptr1, const void* ptr2, std::size_t size);
};

/**
 * @brief RAII memory scope for automatic cleanup
 */
class MemoryScope {
public:
    explicit MemoryScope(WebMemory& memory, AllocationStrategy strategy = WebMemory::AllocationStrategy::Stack);
    ~MemoryScope();
    
    // Non-copyable, movable
    MemoryScope(const MemoryScope&) = delete;
    MemoryScope& operator=(const MemoryScope&) = delete;
    MemoryScope(MemoryScope&&) noexcept = default;
    MemoryScope& operator=(MemoryScope&&) noexcept = default;
    
    /**
     * @brief Allocate memory in this scope
     * @param size Size in bytes
     * @param alignment Memory alignment
     * @return Allocated memory block
     */
    WebMemory::MemoryBlock allocate(std::size_t size, 
                                   WebMemory::Alignment alignment = WebMemory::Alignment::DoubleWord);
    
    /**
     * @brief Get total allocated size in this scope
     * @return Total allocated bytes
     */
    std::size_t get_allocated_size() const { return total_allocated_; }
    
private:
    WebMemory& memory_;
    WebMemory::AllocationStrategy strategy_;
    std::vector<WebMemory::MemoryBlock> allocations_;
    std::size_t total_allocated_ = 0;
};

} // namespace ecscope::web