#pragma once

#include "core/types.hpp"
#include "core/result.hpp"
#include "core/log.hpp"
#include <memory>
#include <vector>
#include <string>
#include <mutex>
#include <unordered_set>
#include <atomic>
#include <array>

namespace ecscope::memory {

// Forward declaration for friend access
struct AllocationInfo;

/**
 * @brief Free block node in the linked list
 * 
 * Each free block contains a pointer to the next free block, creating
 * a singly-linked free list for O(1) allocation and deallocation.
 * The next pointer is stored directly in the free memory block.
 */
struct FreeBlock {
    FreeBlock* next;  // Points to next free block, null if last
    
    FreeBlock() noexcept : next(nullptr) {}
    explicit FreeBlock(FreeBlock* next_block) noexcept : next(next_block) {}
};

/**
 * @brief Pool chunk represents a contiguous memory region for a specific block size
 * 
 * Each chunk manages a fixed number of blocks of identical size. Chunks can be
 * dynamically allocated when the pool needs to expand beyond initial capacity.
 */
struct PoolChunk {
    byte* memory;           // Raw memory for this chunk
    usize block_size;       // Size of each block in bytes
    usize block_count;      // Number of blocks in this chunk
    usize blocks_allocated; // Current number of allocated blocks
    FreeBlock* free_head;   // Head of free list for this chunk
    bool owns_memory;       // Whether this chunk owns its memory
    f64 creation_time;      // When this chunk was created
    
    PoolChunk() noexcept
        : memory(nullptr), block_size(0), block_count(0), 
          blocks_allocated(0), free_head(nullptr), owns_memory(false),
          creation_time(0.0) {}
    
    PoolChunk(byte* mem, usize block_sz, usize block_cnt, bool owns_mem = true) noexcept;
    ~PoolChunk();
    
    // Non-copyable but movable
    PoolChunk(const PoolChunk&) = delete;
    PoolChunk& operator=(const PoolChunk&) = delete;
    PoolChunk(PoolChunk&& other) noexcept;
    PoolChunk& operator=(PoolChunk&& other) noexcept;
    
    // Initialize the free list for this chunk
    void initialize_free_list() noexcept;
    
    // Check if a pointer belongs to this chunk
    bool contains(const void* ptr) const noexcept;
    
    // Get block index from pointer (for debugging/visualization)
    usize get_block_index(const void* ptr) const noexcept;
    
    // Check if chunk is full
    bool is_full() const noexcept { return blocks_allocated == block_count; }
    
    // Check if chunk is empty
    bool is_empty() const noexcept { return blocks_allocated == 0; }
    
    // Get utilization ratio (0.0 to 1.0)
    f64 utilization() const noexcept {
        return block_count > 0 ? static_cast<f64>(blocks_allocated) / block_count : 0.0;
    }
};

/**
 * @brief Pool allocation statistics for performance monitoring
 * 
 * Comprehensive statistics tracking for educational purposes,
 * including fragmentation analysis and performance metrics.
 */
struct PoolStats {
    // Basic allocation stats
    usize total_capacity;       // Total number of blocks across all chunks
    usize total_allocated;      // Currently allocated blocks
    usize peak_allocated;       // Peak allocation count
    usize total_allocations;    // Total allocations made (lifetime)
    usize total_deallocations;  // Total deallocations made (lifetime)
    
    // Memory usage
    usize block_size;           // Size of each block
    usize total_memory_used;    // Total memory used by chunks
    usize wasted_bytes;         // Memory lost to internal fragmentation
    usize overhead_bytes;       // Memory used for metadata
    
    // Fragmentation analysis
    f64 external_fragmentation; // Ratio of free blocks to total blocks
    f64 internal_fragmentation; // Ratio of wasted bytes to allocated bytes
    usize free_list_length;     // Number of free blocks
    usize chunk_count;          // Number of chunks
    f64 average_chunk_usage;    // Average utilization across chunks
    
    // Performance metrics
    f64 total_alloc_time;       // Total time spent allocating (ms)
    f64 total_dealloc_time;     // Total time spent deallocating (ms) 
    f64 average_alloc_time;     // Average allocation time (ns)
    f64 average_dealloc_time;   // Average deallocation time (ns)
    u64 cache_misses_estimated; // Estimated cache misses
    
    // Allocation patterns
    f64 allocation_frequency;   // Allocations per second
    f64 deallocation_frequency; // Deallocations per second
    usize max_free_list_length; // Peak free list length
    usize chunk_expansions;     // Number of times pool expanded
    
    PoolStats() noexcept { reset(); }
    
    void reset() noexcept {
        total_capacity = total_allocated = peak_allocated = 0;
        total_allocations = total_deallocations = 0;
        block_size = total_memory_used = wasted_bytes = overhead_bytes = 0;
        external_fragmentation = internal_fragmentation = 0.0;
        free_list_length = chunk_count = 0;
        average_chunk_usage = 0.0;
        total_alloc_time = total_dealloc_time = 0.0;
        average_alloc_time = average_dealloc_time = 0.0;
        cache_misses_estimated = 0;
        allocation_frequency = deallocation_frequency = 0.0;
        max_free_list_length = chunk_expansions = 0;
    }
    
    // Calculate efficiency ratio (allocated / total capacity)
    f64 efficiency_ratio() const noexcept {
        return total_capacity > 0 ? static_cast<f64>(total_allocated) / total_capacity : 0.0;
    }
    
    // Calculate memory overhead ratio
    f64 overhead_ratio() const noexcept {
        return total_memory_used > 0 ? static_cast<f64>(overhead_bytes) / total_memory_used : 0.0;
    }
};

/**
 * @brief Pool Allocator - Educational Implementation for Fixed-Size Block Allocation
 * 
 * A high-performance pool allocator designed for allocating fixed-size objects.
 * Features O(1) allocation and deallocation using free-list management,
 * comprehensive tracking for educational purposes, and support for multiple
 * pool sizes within the same allocator instance.
 * 
 * Key Educational Features:
 * - Visual free-list representation for UI debugging
 * - Detailed fragmentation analysis and statistics
 * - Performance profiling with cache miss estimation
 * - Memory pattern debugging with poisoning
 * - Thread-safety analysis and lock contention tracking
 * - Chunk-based expansion for dynamic growth
 * 
 * Design Principles:
 * - Zero overhead when tracking is disabled
 * - Cache-friendly memory layout
 * - Exception-safe operations
 * - Comprehensive error reporting
 * - Integration with existing memory tracking systems
 */
class PoolAllocator {
private:
    // Core pool configuration
    usize block_size_;              // Size of each allocated block
    usize alignment_;               // Block alignment requirement  
    usize initial_capacity_;        // Initial number of blocks per chunk
    usize max_chunks_;              // Maximum number of chunks (0 = unlimited)
    bool allow_expansion_;          // Can create new chunks when full
    
    // Memory management
    std::vector<PoolChunk> chunks_; // All chunks belonging to this pool
    FreeBlock* free_head_;          // Head of global free list
    usize total_free_blocks_;       // Count of free blocks across all chunks
    
    // Tracking and debugging
    std::unordered_set<void*> allocated_blocks_; // Set of currently allocated blocks
    std::vector<AllocationInfo> allocations_;    // Detailed allocation history
    std::mutex tracking_mutex_;                  // Thread safety for tracking
    PoolStats stats_;                           // Performance statistics
    
    // Configuration flags
    bool enable_tracking_;          // Detailed allocation tracking
    bool enable_debug_fill_;        // Fill memory with debug patterns
    bool enable_thread_safety_;     // Thread-safe operations
    byte debug_alloc_pattern_;      // Pattern for newly allocated blocks
    byte debug_free_pattern_;       // Pattern for freed blocks
    
    // Debug and identification  
    std::string name_;              // Pool name for debugging
    usize type_hash_;               // Hash of the type this pool manages
    
    // Thread safety
    mutable std::mutex allocation_mutex_;  // Protects allocation operations
    std::atomic<usize> allocation_counter_{0}; // Thread-safe allocation counter
    
    // Performance monitoring
    f64 last_stats_update_;         // Last time stats were updated
    std::array<f64, 100> recent_alloc_times_; // Circular buffer for timing
    usize timing_index_;            // Current index in timing buffer

public:
    /**
     * @brief Construct a new Pool Allocator
     * 
     * @param block_size Size of each block in bytes (must be >= sizeof(void*))
     * @param initial_capacity Initial number of blocks to allocate
     * @param alignment Alignment requirement for blocks (must be power of 2)
     * @param name Human-readable name for debugging
     * @param enable_tracking Enable detailed allocation tracking
     */
    explicit PoolAllocator(
        usize block_size,
        usize initial_capacity = 1024,
        usize alignment = alignof(std::max_align_t),
        const std::string& name = "Pool",
        bool enable_tracking = true
    );
    
    /**
     * @brief Construct a type-specific pool allocator
     * 
     * Template constructor that automatically configures the pool
     * for a specific type T, setting appropriate block size and alignment.
     * 
     * @tparam T The type this pool will allocate
     * @param initial_capacity Initial number of blocks
     * @param name Pool name (defaults to type name)
     * @param enable_tracking Enable allocation tracking
     */
    template<typename T>
    static PoolAllocator create_for_type(
        usize initial_capacity = 1024,
        const std::string& name = "",
        bool enable_tracking = true
    ) {
        std::string pool_name = name.empty() ? std::string("Pool<") + typeid(T).name() + ">" : name;
        return PoolAllocator(sizeof(T), initial_capacity, alignof(T), pool_name, enable_tracking);
    }
    
    // Destructor
    ~PoolAllocator();
    
    // Non-copyable but movable
    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;
    PoolAllocator(PoolAllocator&& other) noexcept;
    PoolAllocator& operator=(PoolAllocator&& other) noexcept;
    
    // ===========================================
    // Core Allocation Interface
    // ===========================================
    
    /**
     * @brief Allocate a block from the pool
     * 
     * Returns a pointer to a block of block_size_ bytes, aligned to the
     * pool's alignment requirement. Uses O(1) free-list allocation.
     * 
     * @param category Optional category for tracking purposes
     * @return void* Pointer to allocated block, or nullptr if allocation fails
     */
    void* allocate(const char* category = nullptr);
    
    /**
     * @brief Allocate a block with debug information
     * 
     * Same as allocate() but records file, line, and function information
     * for detailed tracking and debugging purposes.
     * 
     * @param category Allocation category
     * @param file Source file name
     * @param line Source line number  
     * @param function Source function name
     * @return void* Pointer to allocated block, or nullptr if allocation fails
     */
    void* allocate_debug(const char* category, const char* file, int line, const char* function);
    
    /**
     * @brief Deallocate a block back to the pool
     * 
     * Returns the block to the free list for reuse. The pointer must have
     * been returned by a previous call to allocate() on this pool.
     * 
     * @param ptr Pointer to block to deallocate (must not be nullptr)
     */
    void deallocate(void* ptr);
    
    /**
     * @brief Try to allocate without expansion
     * 
     * Attempts to allocate from existing chunks without creating new ones.
     * Returns nullptr if no free blocks are available.
     * 
     * @param category Optional allocation category
     * @return void* Pointer to block or nullptr if unavailable
     */
    void* try_allocate(const char* category = nullptr);
    
    // ===========================================
    // Type-Safe Template Interface
    // ===========================================
    
    /**
     * @brief Type-safe allocation
     * 
     * Allocates space for count objects of type T. Block size must match
     * sizeof(T) and alignment must be compatible with alignof(T).
     * 
     * @tparam T Type to allocate
     * @param count Number of objects to allocate space for
     * @param category Optional allocation category
     * @return T* Typed pointer to allocated space
     */
    template<typename T>
    T* allocate(usize count = 1, const char* category = nullptr) {
        static_assert(sizeof(T) <= block_size_, "Type T is too large for this pool");
        static_assert(alignof(T) <= alignment_, "Type T has stricter alignment requirements");
        
        void* ptr = allocate(category);
        return static_cast<T*>(ptr);
    }
    
    /**
     * @brief Type-safe deallocation
     * 
     * Deallocates memory for objects of type T. Does not call destructors.
     * 
     * @tparam T Type that was allocated
     * @param ptr Pointer to deallocate
     */
    template<typename T>
    void deallocate(T* ptr) {
        deallocate(static_cast<void*>(ptr));
    }
    
    /**
     * @brief Construct object in-place
     * 
     * Allocates space and constructs an object of type T using the
     * provided constructor arguments.
     * 
     * @tparam T Type to construct
     * @tparam Args Constructor argument types
     * @param args Constructor arguments
     * @return T* Pointer to constructed object
     */
    template<typename T, typename... Args>
    T* construct(Args&&... args) {
        T* ptr = allocate<T>(1, typeid(T).name());
        if (ptr) {
            new(ptr) T(std::forward<Args>(args)...);
        }
        return ptr;
    }
    
    /**
     * @brief Destroy object and deallocate
     * 
     * Calls the destructor for the object and returns memory to the pool.
     * 
     * @tparam T Type to destroy
     * @param ptr Pointer to object to destroy
     */
    template<typename T>
    void destroy(T* ptr) {
        if (ptr) {
            ptr->~T();
            deallocate(ptr);
        }
    }
    
    // ===========================================
    // Pool Management
    // ===========================================
    
    /**
     * @brief Add a new chunk to expand the pool
     * 
     * Creates a new chunk with the specified capacity. If capacity is 0,
     * uses the pool's initial capacity setting.
     * 
     * @param capacity Number of blocks in the new chunk (0 = use default)
     * @return bool True if chunk was successfully added
     */
    bool expand_pool(usize capacity = 0);
    
    /**
     * @brief Shrink the pool by removing empty chunks
     * 
     * Removes any chunks that have no allocated blocks. Helps reduce
     * memory footprint when allocation patterns become sparse.
     * 
     * @return usize Number of chunks removed
     */
    usize shrink_pool();
    
    /**
     * @brief Reset the pool to initial state
     * 
     * Deallocates all blocks and resets the free list. This invalidates
     * all previously allocated pointers from this pool.
     */
    void reset();
    
    /**
     * @brief Clear pool and reset tracking data
     * 
     * Same as reset() but also clears all tracking information and statistics.
     */
    void clear();
    
    // ===========================================
    // Ownership and Validation  
    // ===========================================
    
    /**
     * @brief Check if pointer was allocated by this pool
     * 
     * @param ptr Pointer to check
     * @return bool True if pointer belongs to this pool
     */
    bool owns(const void* ptr) const noexcept;
    
    /**
     * @brief Validate that a pointer is properly aligned for this pool
     * 
     * @param ptr Pointer to validate
     * @return bool True if pointer has correct alignment
     */
    bool is_aligned(const void* ptr) const noexcept;
    
    /**
     * @brief Check if a pointer points to the start of a valid block
     * 
     * @param ptr Pointer to validate
     * @return bool True if pointer is at the beginning of a block
     */
    bool is_valid_block(const void* ptr) const noexcept;
    
    // ===========================================
    // Statistics and Performance Monitoring
    // ===========================================
    
    /**
     * @brief Get current pool statistics
     * 
     * @return const PoolStats& Reference to current statistics
     */
    const PoolStats& stats() const noexcept { return stats_; }
    
    /**
     * @brief Update statistics (call periodically for accurate metrics)
     */
    void update_stats();
    
    /**
     * @brief Reset all statistics to zero
     */
    void reset_stats();
    
    // ===========================================
    // Configuration and Introspection
    // ===========================================
    
    /**
     * @brief Set debug fill patterns for allocated and freed blocks
     * 
     * @param alloc_pattern Byte pattern for newly allocated blocks
     * @param free_pattern Byte pattern for deallocated blocks  
     */
    void set_debug_patterns(byte alloc_pattern, byte free_pattern);
    
    /**
     * @brief Enable or disable detailed allocation tracking
     * 
     * @param enabled True to enable tracking
     */
    void set_tracking_enabled(bool enabled) { enable_tracking_ = enabled; }
    
    /**
     * @brief Check if allocation tracking is enabled
     * 
     * @return bool True if tracking is enabled
     */
    bool is_tracking_enabled() const noexcept { return enable_tracking_; }
    
    /**
     * @brief Enable or disable thread safety
     * 
     * @param enabled True to enable thread-safe operations
     */
    void set_thread_safety_enabled(bool enabled) { enable_thread_safety_ = enabled; }
    
    /**
     * @brief Check if thread safety is enabled
     * 
     * @return bool True if thread safety is enabled
     */
    bool is_thread_safety_enabled() const noexcept { return enable_thread_safety_; }
    
    /**
     * @brief Set maximum number of chunks
     * 
     * @param max_chunks Maximum chunks (0 = unlimited)
     */
    void set_max_chunks(usize max_chunks) { max_chunks_ = max_chunks; }
    
    /**
     * @brief Enable or disable automatic expansion
     * 
     * @param allow_expansion True to allow creating new chunks when full
     */
    void set_expansion_enabled(bool allow_expansion) { allow_expansion_ = allow_expansion; }
    
    // Basic properties
    const std::string& name() const noexcept { return name_; }
    usize block_size() const noexcept { return block_size_; }
    usize alignment() const noexcept { return alignment_; }
    usize chunk_count() const noexcept { return chunks_.size(); }
    usize total_capacity() const noexcept;
    usize allocated_count() const noexcept;
    usize free_count() const noexcept { return total_free_blocks_; }
    
    // Utilization metrics
    f64 utilization_ratio() const noexcept {
        usize capacity = total_capacity();
        return capacity > 0 ? static_cast<f64>(allocated_count()) / capacity : 0.0;
    }
    
    bool is_full() const noexcept { return total_free_blocks_ == 0; }
    bool is_empty() const noexcept { return allocated_count() == 0; }
    
    // ===========================================
    // Memory Layout Visualization (for UI)
    // ===========================================
    
    /**
     * @brief Information about a memory block for visualization
     */
    struct BlockInfo {
        void* ptr;              // Block address
        usize block_index;      // Index within chunk
        usize chunk_index;      // Which chunk this block belongs to
        bool allocated;         // Whether block is currently allocated
        const char* category;   // Allocation category (if tracked)
        f64 allocation_time;    // When block was allocated
        f64 age;               // Time since allocation
    };
    
    /**
     * @brief Get layout information for all blocks (for visualization)
     * 
     * @return std::vector<BlockInfo> Vector of block information
     */
    std::vector<BlockInfo> get_memory_layout() const;
    
    /**
     * @brief Information about free list structure for visualization
     */
    struct FreeListInfo {
        std::vector<void*> free_blocks;     // Addresses of free blocks
        usize total_free;                   // Total number of free blocks
        usize max_contiguous_free;          // Largest contiguous free region
        f64 fragmentation_score;            // Fragmentation metric (0-1)
        std::vector<usize> free_chunks;     // Number of free blocks per chunk
    };
    
    /**
     * @brief Get free list structure information (for visualization)
     * 
     * @return FreeListInfo Structure containing free list details
     */
    FreeListInfo get_free_list_info() const;
    
    /**
     * @brief Information about individual chunks for visualization
     */
    struct ChunkInfo {
        void* base_address;     // Start of chunk memory
        usize block_count;      // Total blocks in chunk
        usize allocated_blocks; // Currently allocated blocks  
        f64 utilization;        // Utilization ratio (0-1)
        f64 creation_time;      // When chunk was created
        f64 age;               // Time since creation
        bool can_be_freed;      // Whether chunk can be removed
    };
    
    /**
     * @brief Get information about all chunks (for visualization)
     * 
     * @return std::vector<ChunkInfo> Vector of chunk information
     */
    std::vector<ChunkInfo> get_chunk_info() const;
    
    // ===========================================
    // Allocation History and Debugging
    // ===========================================
    
    /**
     * @brief Get list of currently active allocations
     * 
     * @return std::vector<AllocationInfo> Vector of active allocations
     */
    std::vector<AllocationInfo> get_active_allocations() const;
    
    /**
     * @brief Get complete allocation history
     * 
     * @return std::vector<AllocationInfo> Vector of all allocations made
     */
    std::vector<AllocationInfo> get_all_allocations() const;
    
    /**
     * @brief Find allocation info for a specific pointer
     * 
     * @param ptr Pointer to find info for
     * @return const AllocationInfo* Pointer to allocation info, or nullptr if not found
     */
    const AllocationInfo* find_allocation_info(const void* ptr) const;
    
    /**
     * @brief Check pool integrity and consistency
     * 
     * Performs comprehensive validation of the pool's internal structures.
     * Useful for detecting corruption or implementation bugs.
     * 
     * @return bool True if pool is in a consistent state
     */
    bool validate_integrity() const;
    
    /**
     * @brief Generate diagnostic report
     * 
     * Creates a human-readable report of the pool's current state,
     * including statistics, chunk information, and any detected issues.
     * 
     * @return std::string Diagnostic report
     */
    std::string generate_diagnostic_report() const;
    
private:
    // Internal implementation methods
    void initialize_pool();
    void cleanup_pool();
    PoolChunk* find_chunk_for_ptr(const void* ptr) const noexcept;
    void rebuild_free_list();
    void record_allocation(void* ptr, const char* category, 
                          const char* file = nullptr, int line = 0, const char* function = nullptr);
    void record_deallocation(void* ptr);
    void fill_memory(void* ptr, usize size, byte pattern);
    void update_fragmentation_stats();
    void update_performance_stats();
    f64 get_current_time() const noexcept;
    
    // Thread-safe helpers
    template<typename F>
    auto with_lock(F&& func) const -> decltype(func()) {
        if (enable_thread_safety_) {
            std::lock_guard<std::mutex> lock(allocation_mutex_);
            return func();
        } else {
            return func();
        }
    }
    
    // Cache miss estimation (educational)
    void estimate_cache_behavior(void* ptr);
    
    // Memory poisoning for debugging
    void poison_block(void* ptr, bool is_allocation);
    bool check_poison_pattern(void* ptr, bool expect_alloc_pattern) const;
};

// ===========================================
// RAII Helpers and Convenience Classes
// ===========================================

/**
 * @brief RAII wrapper for pool-allocated objects
 * 
 * Automatically destroys and deallocates the object when it goes out of scope.
 * Provides safe exception handling and prevents memory leaks.
 */
template<typename T>
class PoolPtr {
private:
    T* ptr_;
    PoolAllocator* allocator_;
    
public:
    explicit PoolPtr(PoolAllocator& allocator) 
        : ptr_(nullptr), allocator_(&allocator) {}
    
    template<typename... Args>
    PoolPtr(PoolAllocator& allocator, Args&&... args)
        : ptr_(allocator.construct<T>(std::forward<Args>(args)...)), allocator_(&allocator) {}
    
    ~PoolPtr() {
        if (ptr_ && allocator_) {
            allocator_->destroy(ptr_);
        }
    }
    
    // Non-copyable but movable
    PoolPtr(const PoolPtr&) = delete;
    PoolPtr& operator=(const PoolPtr&) = delete;
    
    PoolPtr(PoolPtr&& other) noexcept 
        : ptr_(other.ptr_), allocator_(other.allocator_) {
        other.ptr_ = nullptr;
        other.allocator_ = nullptr;
    }
    
    PoolPtr& operator=(PoolPtr&& other) noexcept {
        if (this != &other) {
            if (ptr_ && allocator_) {
                allocator_->destroy(ptr_);
            }
            ptr_ = other.ptr_;
            allocator_ = other.allocator_;
            other.ptr_ = nullptr;
            other.allocator_ = nullptr;
        }
        return *this;
    }
    
    // Access operators
    T& operator*() const noexcept { return *ptr_; }
    T* operator->() const noexcept { return ptr_; }
    T* get() const noexcept { return ptr_; }
    
    // Conversion operators
    explicit operator bool() const noexcept { return ptr_ != nullptr; }
    
    // Release ownership without destroying
    T* release() noexcept {
        T* result = ptr_;
        ptr_ = nullptr;
        return result;
    }
    
    // Reset with new object
    template<typename... Args>
    void reset(Args&&... args) {
        if (ptr_ && allocator_) {
            allocator_->destroy(ptr_);
        }
        ptr_ = allocator_->construct<T>(std::forward<Args>(args)...);
    }
    
    void reset(std::nullptr_t = nullptr) {
        if (ptr_ && allocator_) {
            allocator_->destroy(ptr_);
        }
        ptr_ = nullptr;
    }
};

/**
 * @brief Scoped pool for temporary allocations
 * 
 * Creates a temporary pool that automatically cleans up when destroyed.
 * Useful for allocating many small objects that share the same lifetime.
 */
template<typename T>
class ScopedPool {
private:
    PoolAllocator pool_;
    std::vector<T*> allocated_objects_;
    
public:
    explicit ScopedPool(usize initial_capacity = 256)
        : pool_(PoolAllocator::create_for_type<T>(initial_capacity, "ScopedPool<" + std::string(typeid(T).name()) + ">")) {}
    
    ~ScopedPool() {
        // Destroy all allocated objects
        for (T* obj : allocated_objects_) {
            if (obj) {
                obj->~T();
            }
        }
        // Pool destructor will handle memory cleanup
    }
    
    // Non-copyable, non-movable for simplicity
    ScopedPool(const ScopedPool&) = delete;
    ScopedPool& operator=(const ScopedPool&) = delete;
    ScopedPool(ScopedPool&&) = delete;
    ScopedPool& operator=(ScopedPool&&) = delete;
    
    template<typename... Args>
    T* construct(Args&&... args) {
        T* ptr = pool_.construct<T>(std::forward<Args>(args)...);
        if (ptr) {
            allocated_objects_.push_back(ptr);
        }
        return ptr;
    }
    
    T* allocate() {
        T* ptr = pool_.allocate<T>();
        if (ptr) {
            allocated_objects_.push_back(ptr);
        }
        return ptr;
    }
    
    // Access to underlying pool for advanced usage
    PoolAllocator& pool() noexcept { return pool_; }
    const PoolAllocator& pool() const noexcept { return pool_; }
};

// ===========================================
// Convenience Functions and Macros
// ===========================================

/**
 * @brief Create a pool optimized for a specific type
 * 
 * @tparam T Type to optimize for
 * @param initial_capacity Initial number of blocks
 * @param name Pool name (optional)
 * @return PoolAllocator Pool configured for type T
 */
template<typename T>
PoolAllocator make_pool(usize initial_capacity = 1024, const std::string& name = "") {
    return PoolAllocator::create_for_type<T>(initial_capacity, name);
}

/**
 * @brief Create a scoped pointer allocated from a pool
 * 
 * @tparam T Type to allocate
 * @tparam Args Constructor argument types
 * @param allocator Pool allocator to use
 * @param args Constructor arguments
 * @return PoolPtr<T> RAII pointer wrapper
 */
template<typename T, typename... Args>
PoolPtr<T> make_pooled(PoolAllocator& allocator, Args&&... args) {
    return PoolPtr<T>(allocator, std::forward<Args>(args)...);
}

// Debug allocation macros (similar to arena allocator)
#define POOL_ALLOC(pool) \
    (pool).allocate_debug(nullptr, __FILE__, __LINE__, __FUNCTION__)

#define POOL_ALLOC_T(pool, type) \
    static_cast<type*>((pool).allocate_debug(#type, __FILE__, __LINE__, __FUNCTION__))

#define POOL_ALLOC_CATEGORY(pool, category) \
    (pool).allocate_debug((category), __FILE__, __LINE__, __FUNCTION__)

#define POOL_CONSTRUCT(pool, type, ...) \
    [&]() { \
        auto* ptr = static_cast<type*>((pool).allocate_debug(#type, __FILE__, __LINE__, __FUNCTION__)); \
        if (ptr) new(ptr) type(__VA_ARGS__); \
        return ptr; \
    }()

// ===========================================
// Pool Registry for UI Integration
// ===========================================

/**
 * @brief Global registry for pool allocators
 * 
 * Allows UI systems to discover and visualize all active pool allocators.
 * Provides centralized statistics and monitoring capabilities.
 */
namespace pool_registry {
    /**
     * @brief Register a pool allocator for monitoring
     * 
     * @param pool Pointer to pool allocator
     */
    void register_pool(PoolAllocator* pool);
    
    /**
     * @brief Unregister a pool allocator
     * 
     * @param pool Pointer to pool allocator
     */
    void unregister_pool(PoolAllocator* pool);
    
    /**
     * @brief Get all registered pools
     * 
     * @return std::vector<PoolAllocator*> Vector of pool pointers
     */
    std::vector<PoolAllocator*> get_all_pools();
    
    /**
     * @brief Get combined statistics across all pools
     * 
     * @return PoolStats Aggregated statistics
     */
    PoolStats get_combined_stats();
    
    /**
     * @brief Get pools filtered by type hash
     * 
     * @param type_hash Type hash to filter by
     * @return std::vector<PoolAllocator*> Vector of matching pools
     */
    std::vector<PoolAllocator*> get_pools_by_type(usize type_hash);
    
    /**
     * @brief Generate system-wide pool usage report
     * 
     * @return std::string Comprehensive report of all pool usage
     */
    std::string generate_system_report();
}

} // namespace ecscope::memory