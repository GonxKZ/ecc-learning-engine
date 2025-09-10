/**
 * @file gui_memory.hpp
 * @brief Memory Management System for GUI Framework
 * 
 * Efficient memory allocation system optimized for immediate-mode GUI,
 * featuring frame-based allocators, object pools, and minimal overhead.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#pragma once

#include <memory>
#include <vector>
#include <stack>
#include <unordered_map>
#include <array>
#include <cstddef>
#include <cassert>
#include <atomic>
#include <mutex>

namespace ecscope::gui {

// =============================================================================
// MEMORY ALLOCATION INTERFACES
// =============================================================================

/**
 * @brief Base allocator interface
 */
class IAllocator {
public:
    virtual ~IAllocator() = default;
    
    virtual void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) = 0;
    virtual void deallocate(void* ptr) = 0;
    virtual void reset() = 0;
    virtual size_t get_allocated_size() const = 0;
    virtual size_t get_total_capacity() const = 0;
};

// =============================================================================
// LINEAR ALLOCATOR - PERFECT FOR FRAME-BASED ALLOCATIONS
// =============================================================================

/**
 * @brief Linear allocator for fast sequential allocations
 * 
 * Extremely fast allocation with no deallocation. Perfect for per-frame
 * temporary data that gets reset each frame.
 */
class LinearAllocator : public IAllocator {
public:
    explicit LinearAllocator(size_t capacity);
    ~LinearAllocator();
    
    // Non-copyable, movable
    LinearAllocator(const LinearAllocator&) = delete;
    LinearAllocator& operator=(const LinearAllocator&) = delete;
    LinearAllocator(LinearAllocator&& other) noexcept;
    LinearAllocator& operator=(LinearAllocator&& other) noexcept;
    
    void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) override;
    void deallocate(void* ptr) override {} // No-op for linear allocator
    void reset() override;
    
    size_t get_allocated_size() const override { return current_offset_; }
    size_t get_total_capacity() const override { return capacity_; }
    
    // Save/restore state for nested scopes
    struct Marker {
        size_t offset;
    };
    
    Marker get_marker() const { return {current_offset_}; }
    void reset_to_marker(const Marker& marker);
    
private:
    void* memory_;
    size_t capacity_;
    size_t current_offset_;
    
    size_t align_offset(size_t offset, size_t alignment) const;
};

// =============================================================================
// STACK ALLOCATOR - FOR SCOPED ALLOCATIONS
// =============================================================================

/**
 * @brief Stack allocator with automatic scope management
 */
class StackAllocator : public IAllocator {
public:
    explicit StackAllocator(size_t capacity);
    ~StackAllocator();
    
    void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) override;
    void deallocate(void* ptr) override; // Must deallocate in LIFO order
    void reset() override;
    
    size_t get_allocated_size() const override;
    size_t get_total_capacity() const override { return capacity_; }
    
    // Scope management
    class Scope {
    public:
        Scope(StackAllocator& allocator);
        ~Scope();
        
        // Non-copyable, non-movable
        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;
        
    private:
        StackAllocator& allocator_;
        size_t saved_top_;
    };
    
private:
    struct Block {
        size_t size;
        size_t alignment;
    };
    
    void* memory_;
    size_t capacity_;
    size_t top_;
    std::stack<size_t> scope_stack_;
    
    size_t align_offset(size_t offset, size_t alignment) const;
};

// =============================================================================
// POOL ALLOCATOR - FOR FIXED-SIZE OBJECTS
// =============================================================================

/**
 * @brief Pool allocator for efficient fixed-size allocations
 */
template<typename T, size_t ChunkSize = 1024>
class PoolAllocator {
public:
    PoolAllocator() = default;
    ~PoolAllocator() { clear(); }
    
    // Non-copyable, movable
    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;
    PoolAllocator(PoolAllocator&& other) noexcept : chunks_(std::move(other.chunks_)), free_list_(other.free_list_) {
        other.free_list_ = nullptr;
    }
    
    T* allocate() {
        if (!free_list_) {
            allocate_new_chunk();
        }
        
        T* result = free_list_;
        free_list_ = *reinterpret_cast<T**>(free_list_);
        return result;
    }
    
    void deallocate(T* ptr) {
        *reinterpret_cast<T**>(ptr) = free_list_;
        free_list_ = ptr;
    }
    
    template<typename... Args>
    T* construct(Args&&... args) {
        T* ptr = allocate();
        new(ptr) T(std::forward<Args>(args)...);
        return ptr;
    }
    
    void destroy(T* ptr) {
        ptr->~T();
        deallocate(ptr);
    }
    
    void clear() {
        for (auto* chunk : chunks_) {
            std::free(chunk);
        }
        chunks_.clear();
        free_list_ = nullptr;
    }
    
    size_t get_chunk_count() const { return chunks_.size(); }
    size_t get_total_capacity() const { return chunks_.size() * ChunkSize; }
    
private:
    std::vector<T*> chunks_;
    T* free_list_ = nullptr;
    
    void allocate_new_chunk() {
        T* chunk = static_cast<T*>(std::malloc(sizeof(T) * ChunkSize));
        chunks_.push_back(chunk);
        
        // Link all objects in chunk to free list
        for (size_t i = 0; i < ChunkSize - 1; ++i) {
            *reinterpret_cast<T**>(&chunk[i]) = &chunk[i + 1];
        }
        *reinterpret_cast<T**>(&chunk[ChunkSize - 1]) = free_list_;
        free_list_ = chunk;
    }
};

// =============================================================================
// FREELIST ALLOCATOR - GENERAL PURPOSE WITH FAST DEALLOCATION
// =============================================================================

/**
 * @brief Freelist allocator with size-based bins
 */
class FreeListAllocator : public IAllocator {
public:
    explicit FreeListAllocator(size_t capacity);
    ~FreeListAllocator();
    
    void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) override;
    void deallocate(void* ptr) override;
    void reset() override;
    
    size_t get_allocated_size() const override { return allocated_size_; }
    size_t get_total_capacity() const override { return capacity_; }
    
    // Defragmentation
    void defragment();
    float get_fragmentation_ratio() const;
    
private:
    struct FreeBlock {
        size_t size;
        FreeBlock* next;
    };
    
    struct AllocationHeader {
        size_t size;
        size_t alignment;
    };
    
    static constexpr size_t NUM_BINS = 32;
    static constexpr size_t MIN_ALLOCATION = sizeof(FreeBlock);
    
    void* memory_;
    size_t capacity_;
    size_t allocated_size_;
    std::array<FreeBlock*, NUM_BINS> free_lists_;
    
    size_t get_bin_index(size_t size) const;
    FreeBlock* find_best_fit(size_t size);
    void split_block(FreeBlock* block, size_t size);
    void coalesce_free_blocks();
    void insert_free_block(FreeBlock* block);
    void remove_free_block(FreeBlock* block);
    
    size_t align_size(size_t size, size_t alignment) const;
};

// =============================================================================
// MEMORY MANAGER - ORCHESTRATES ALL ALLOCATORS
// =============================================================================

/**
 * @brief Central memory manager for the GUI system
 * 
 * Manages different types of allocators for different use cases:
 * - Frame allocator: Reset every frame, very fast
 * - Persistent allocator: Long-lived GUI data
 * - Pools: For common objects like draw commands
 */
class MemoryManager {
public:
    static MemoryManager& instance();
    
    bool initialize(size_t frame_memory_size = 1024 * 1024 * 4,      // 4MB per frame
                   size_t persistent_memory_size = 1024 * 1024 * 16, // 16MB persistent
                   size_t stack_memory_size = 1024 * 1024);          // 1MB stack
    
    void shutdown();
    
    // Frame allocator - reset every frame
    LinearAllocator& get_frame_allocator() { return *frame_allocator_; }
    
    // Persistent allocator - long-lived data
    FreeListAllocator& get_persistent_allocator() { return *persistent_allocator_; }
    
    // Stack allocator - scoped allocations
    StackAllocator& get_stack_allocator() { return *stack_allocator_; }
    
    // Specialized pools
    template<typename T>
    PoolAllocator<T>& get_pool() {
        static PoolAllocator<T> pool;
        return pool;
    }
    
    // Frame management
    void begin_frame();
    void end_frame();
    
    // Memory statistics
    struct MemoryStats {
        size_t frame_allocated = 0;
        size_t frame_capacity = 0;
        size_t persistent_allocated = 0;
        size_t persistent_capacity = 0;
        size_t stack_allocated = 0;
        size_t stack_capacity = 0;
        size_t total_allocations = 0;
        size_t peak_frame_usage = 0;
        float fragmentation_ratio = 0.0f;
    };
    
    MemoryStats get_stats() const;
    
    // Debug utilities
    void print_stats() const;
    void validate_heap_integrity() const;
    
private:
    MemoryManager() = default;
    
    std::unique_ptr<LinearAllocator> frame_allocator_;
    std::unique_ptr<FreeListAllocator> persistent_allocator_;
    std::unique_ptr<StackAllocator> stack_allocator_;
    
    mutable std::mutex stats_mutex_;
    mutable MemoryStats cached_stats_;
    mutable bool stats_dirty_ = true;
    
    size_t peak_frame_usage_ = 0;
    std::atomic<size_t> total_allocations_{0};
    
    bool initialized_ = false;
    
    void update_stats() const;
};

// =============================================================================
// MEMORY UTILITIES AND HELPERS
// =============================================================================

/**
 * @brief RAII wrapper for stack allocator scopes
 */
class MemoryScope {
public:
    MemoryScope() : scope_(MemoryManager::instance().get_stack_allocator()) {}
    
private:
    StackAllocator::Scope scope_;
};

/**
 * @brief Smart pointer using GUI memory allocators
 */
template<typename T>
class gui_unique_ptr {
public:
    gui_unique_ptr() = default;
    explicit gui_unique_ptr(T* ptr) : ptr_(ptr) {}
    
    ~gui_unique_ptr() {
        if (ptr_) {
            MemoryManager::instance().get_pool<T>().destroy(ptr_);
        }
    }
    
    // Non-copyable, movable
    gui_unique_ptr(const gui_unique_ptr&) = delete;
    gui_unique_ptr& operator=(const gui_unique_ptr&) = delete;
    
    gui_unique_ptr(gui_unique_ptr&& other) noexcept : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }
    
    gui_unique_ptr& operator=(gui_unique_ptr&& other) noexcept {
        if (this != &other) {
            reset();
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }
    
    T* get() const { return ptr_; }
    T* operator->() const { return ptr_; }
    T& operator*() const { return *ptr_; }
    
    explicit operator bool() const { return ptr_ != nullptr; }
    
    T* release() {
        T* result = ptr_;
        ptr_ = nullptr;
        return result;
    }
    
    void reset(T* ptr = nullptr) {
        if (ptr_) {
            MemoryManager::instance().get_pool<T>().destroy(ptr_);
        }
        ptr_ = ptr;
    }
    
private:
    T* ptr_ = nullptr;
};

/**
 * @brief Factory function for creating objects in pools
 */
template<typename T, typename... Args>
gui_unique_ptr<T> make_gui_unique(Args&&... args) {
    auto* ptr = MemoryManager::instance().get_pool<T>().construct(std::forward<Args>(args)...);
    return gui_unique_ptr<T>(ptr);
}

/**
 * @brief STL allocator adapter for frame allocator
 */
template<typename T>
class FrameAllocator {
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    
    template<typename U>
    struct rebind {
        using other = FrameAllocator<U>;
    };
    
    FrameAllocator() = default;
    
    template<typename U>
    FrameAllocator(const FrameAllocator<U>&) {}
    
    pointer allocate(size_type n) {
        return static_cast<pointer>(
            MemoryManager::instance().get_frame_allocator().allocate(
                n * sizeof(T), alignof(T)
            )
        );
    }
    
    void deallocate(pointer, size_type) {
        // No-op for frame allocator
    }
    
    template<typename U>
    bool operator==(const FrameAllocator<U>&) const { return true; }
    
    template<typename U>
    bool operator!=(const FrameAllocator<U>&) const { return false; }
};

// STL container aliases using frame allocator
template<typename T>
using frame_vector = std::vector<T, FrameAllocator<T>>;

template<typename Key, typename Value>
using frame_map = std::unordered_map<Key, Value, std::hash<Key>, std::equal_to<Key>,
                                    FrameAllocator<std::pair<const Key, Value>>>;

// =============================================================================
// MEMORY DEBUGGING
// =============================================================================

#ifdef GUI_DEBUG_MEMORY

/**
 * @brief Memory leak detector for debug builds
 */
class MemoryLeakDetector {
public:
    struct AllocationInfo {
        size_t size;
        const char* file;
        int line;
        std::chrono::high_resolution_clock::time_point timestamp;
    };
    
    static MemoryLeakDetector& instance();
    
    void record_allocation(void* ptr, size_t size, const char* file, int line);
    void record_deallocation(void* ptr);
    void report_leaks() const;
    void clear();
    
private:
    mutable std::mutex mutex_;
    std::unordered_map<void*, AllocationInfo> allocations_;
};

#define GUI_MALLOC(size) \
    ::ecscope::gui::debug_malloc((size), __FILE__, __LINE__)
    
#define GUI_FREE(ptr) \
    ::ecscope::gui::debug_free((ptr))

void* debug_malloc(size_t size, const char* file, int line);
void debug_free(void* ptr);

#else

#define GUI_MALLOC(size) std::malloc(size)
#define GUI_FREE(ptr) std::free(ptr)

#endif

} // namespace ecscope::gui