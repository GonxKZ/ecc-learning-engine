#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <atomic>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <concepts>
#include <type_traits>
#include <algorithm>
#include <immintrin.h>
#include <thread>
#include <cassert>

#ifdef _WIN32
    #include <windows.h>
    #include <memoryapi.h>
#else
    #include <sys/mman.h>
    #include <unistd.h>
#endif

namespace ecscope::memory {

// Forward declarations
template<typename T>
concept Allocator = requires(T allocator, size_t size, void* ptr) {
    { allocator.allocate(size, alignof(std::max_align_t)) } -> std::convertible_to<void*>;
    { allocator.deallocate(ptr, size) } -> std::same_as<void>;
    { allocator.owns(ptr) } -> std::convertible_to<bool>;
};

// Memory alignment utilities
constexpr size_t align_up(size_t size, size_t alignment) noexcept {
    return (size + alignment - 1) & ~(alignment - 1);
}

constexpr size_t align_down(size_t size, size_t alignment) noexcept {
    return size & ~(alignment - 1);
}

template<typename T>
constexpr T* align_ptr(T* ptr, size_t alignment) noexcept {
    const auto addr = reinterpret_cast<uintptr_t>(ptr);
    const auto aligned_addr = align_up(addr, alignment);
    return reinterpret_cast<T*>(aligned_addr);
}

// Cache line size detection
inline size_t get_cache_line_size() noexcept {
    static size_t cache_line_size = 0;
    if (cache_line_size == 0) {
#ifdef _WIN32
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION* buffer = nullptr;
        DWORD buffer_size = 0;
        GetLogicalProcessorInformation(buffer, &buffer_size);
        buffer = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION*>(malloc(buffer_size));
        GetLogicalProcessorInformation(buffer, &buffer_size);
        
        for (DWORD i = 0; i < buffer_size / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION); ++i) {
            if (buffer[i].Relationship == RelationCache && buffer[i].Cache.Level == 1) {
                cache_line_size = buffer[i].Cache.LineSize;
                break;
            }
        }
        free(buffer);
#else
        cache_line_size = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
#endif
        if (cache_line_size == 0) cache_line_size = 64; // Default fallback
    }
    return cache_line_size;
}

// ==== LINEAR ALLOCATOR ====
// Ultra-fast bump allocator for temporary allocations
class LinearAllocator {
public:
    explicit LinearAllocator(size_t capacity) 
        : capacity_(capacity), offset_(0) {
        
        // Allocate aligned memory
#ifdef _WIN32
        memory_ = static_cast<uint8_t*>(VirtualAlloc(nullptr, capacity, 
                                                   MEM_COMMIT | MEM_RESERVE, 
                                                   PAGE_READWRITE));
#else
        memory_ = static_cast<uint8_t*>(mmap(nullptr, capacity, 
                                           PROT_READ | PROT_WRITE,
                                           MAP_PRIVATE | MAP_ANONYMOUS, 
                                           -1, 0));
        if (memory_ == MAP_FAILED) memory_ = nullptr;
#endif
        
        if (!memory_) {
            throw std::bad_alloc{};
        }
    }
    
    ~LinearAllocator() {
        if (memory_) {
#ifdef _WIN32
            VirtualFree(memory_, 0, MEM_RELEASE);
#else
            munmap(memory_, capacity_);
#endif
        }
    }
    
    // Non-copyable, movable
    LinearAllocator(const LinearAllocator&) = delete;
    LinearAllocator& operator=(const LinearAllocator&) = delete;
    
    LinearAllocator(LinearAllocator&& other) noexcept 
        : memory_(std::exchange(other.memory_, nullptr))
        , capacity_(std::exchange(other.capacity_, 0))
        , offset_(std::exchange(other.offset_, 0)) {}
    
    LinearAllocator& operator=(LinearAllocator&& other) noexcept {
        if (this != &other) {
            if (memory_) {
#ifdef _WIN32
                VirtualFree(memory_, 0, MEM_RELEASE);
#else
                munmap(memory_, capacity_);
#endif
            }
            memory_ = std::exchange(other.memory_, nullptr);
            capacity_ = std::exchange(other.capacity_, 0);
            offset_ = std::exchange(other.offset_, 0);
        }
        return *this;
    }
    
    // Ultra-fast allocation - just bump the pointer
    [[nodiscard]] void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) noexcept {
        const auto current_offset = align_up(offset_, alignment);
        const auto new_offset = current_offset + size;
        
        if (new_offset > capacity_) {
            return nullptr; // Out of memory
        }
        
        offset_ = new_offset;
        return memory_ + current_offset;
    }
    
    // Linear allocator doesn't support individual deallocation
    void deallocate(void*, size_t) noexcept {
        // No-op for linear allocator
    }
    
    // Reset the allocator (deallocate everything)
    void reset() noexcept {
        offset_ = 0;
    }
    
    // Check if pointer belongs to this allocator
    [[nodiscard]] bool owns(const void* ptr) const noexcept {
        const auto addr = reinterpret_cast<uintptr_t>(ptr);
        const auto start = reinterpret_cast<uintptr_t>(memory_);
        return addr >= start && addr < start + capacity_;
    }
    
    // Statistics
    [[nodiscard]] size_t capacity() const noexcept { return capacity_; }
    [[nodiscard]] size_t used() const noexcept { return offset_; }
    [[nodiscard]] size_t available() const noexcept { return capacity_ - offset_; }
    [[nodiscard]] double utilization() const noexcept { 
        return capacity_ > 0 ? static_cast<double>(offset_) / capacity_ : 0.0; 
    }

private:
    uint8_t* memory_;
    size_t capacity_;
    size_t offset_;
};

// ==== STACK ALLOCATOR ====
// LIFO allocator with marker-based unwinding
class StackAllocator {
public:
    using Marker = size_t;
    
    explicit StackAllocator(size_t capacity) 
        : linear_allocator_(capacity) {}
    
    [[nodiscard]] void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) noexcept {
        return linear_allocator_.allocate(size, alignment);
    }
    
    void deallocate(void*, size_t) noexcept {
        // Stack allocator doesn't support individual deallocation
    }
    
    // Get current marker position
    [[nodiscard]] Marker get_marker() const noexcept {
        return linear_allocator_.used();
    }
    
    // Unwind to marker (free everything allocated after this point)
    void unwind_to_marker(Marker marker) noexcept {
        // Reset linear allocator to specific offset
        const_cast<size_t&>(linear_allocator_.used()) = marker;
    }
    
    void reset() noexcept {
        linear_allocator_.reset();
    }
    
    [[nodiscard]] bool owns(const void* ptr) const noexcept {
        return linear_allocator_.owns(ptr);
    }
    
    // Statistics
    [[nodiscard]] size_t capacity() const noexcept { return linear_allocator_.capacity(); }
    [[nodiscard]] size_t used() const noexcept { return linear_allocator_.used(); }
    [[nodiscard]] size_t available() const noexcept { return linear_allocator_.available(); }

private:
    LinearAllocator linear_allocator_;
    
    // Friend access to modify offset
    friend void unwind_to_marker(Marker);
};

// ==== OBJECT POOL ALLOCATOR ====
// Fixed-size allocation with zero fragmentation
template<typename T>
class ObjectPool {
    static_assert(std::is_destructible_v<T>, "T must be destructible");
    
    struct FreeNode {
        FreeNode* next;
    };
    
    static constexpr size_t OBJECT_SIZE = std::max(sizeof(T), sizeof(FreeNode));
    static constexpr size_t OBJECT_ALIGN = alignof(T);
    
public:
    explicit ObjectPool(size_t capacity) 
        : capacity_(capacity), free_count_(capacity) {
        
        // Allocate aligned memory block
        const size_t total_size = capacity * OBJECT_SIZE;
        
#ifdef _WIN32
        memory_ = static_cast<uint8_t*>(VirtualAlloc(nullptr, total_size,
                                                   MEM_COMMIT | MEM_RESERVE,
                                                   PAGE_READWRITE));
#else
        memory_ = static_cast<uint8_t*>(mmap(nullptr, total_size,
                                           PROT_READ | PROT_WRITE,
                                           MAP_PRIVATE | MAP_ANONYMOUS,
                                           -1, 0));
        if (memory_ == MAP_FAILED) memory_ = nullptr;
#endif
        
        if (!memory_) {
            throw std::bad_alloc{};
        }
        
        // Initialize free list
        free_head_ = reinterpret_cast<FreeNode*>(memory_);
        auto current = free_head_;
        
        for (size_t i = 0; i < capacity - 1; ++i) {
            auto next = reinterpret_cast<FreeNode*>(memory_ + (i + 1) * OBJECT_SIZE);
            current->next = next;
            current = next;
        }
        current->next = nullptr;
    }
    
    ~ObjectPool() {
        if (memory_) {
            const size_t total_size = capacity_ * OBJECT_SIZE;
#ifdef _WIN32
            VirtualFree(memory_, 0, MEM_RELEASE);
#else
            munmap(memory_, total_size);
#endif
        }
    }
    
    // Non-copyable, movable
    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;
    
    ObjectPool(ObjectPool&& other) noexcept 
        : memory_(std::exchange(other.memory_, nullptr))
        , capacity_(std::exchange(other.capacity_, 0))
        , free_head_(std::exchange(other.free_head_, nullptr))
        , free_count_(std::exchange(other.free_count_, 0)) {}
    
    // Fast allocation from free list
    [[nodiscard]] T* allocate() noexcept {
        if (!free_head_) {
            return nullptr; // Pool exhausted
        }
        
        auto node = free_head_;
        free_head_ = node->next;
        --free_count_;
        
        return reinterpret_cast<T*>(node);
    }
    
    // Fast deallocation back to free list
    void deallocate(T* ptr) noexcept {
        if (!owns(ptr)) return;
        
        auto node = reinterpret_cast<FreeNode*>(ptr);
        node->next = free_head_;
        free_head_ = node;
        ++free_count_;
    }
    
    // Construction and destruction helpers
    template<typename... Args>
    [[nodiscard]] T* construct(Args&&... args) {
        if (auto ptr = allocate()) {
            return new(ptr) T(std::forward<Args>(args)...);
        }
        return nullptr;
    }
    
    void destroy(T* ptr) noexcept {
        if (ptr && owns(ptr)) {
            ptr->~T();
            deallocate(ptr);
        }
    }
    
    [[nodiscard]] bool owns(const void* ptr) const noexcept {
        if (!ptr) return false;
        const auto addr = reinterpret_cast<uintptr_t>(ptr);
        const auto start = reinterpret_cast<uintptr_t>(memory_);
        const auto end = start + capacity_ * OBJECT_SIZE;
        
        return addr >= start && addr < end && 
               ((addr - start) % OBJECT_SIZE) == 0;
    }
    
    // Statistics
    [[nodiscard]] size_t capacity() const noexcept { return capacity_; }
    [[nodiscard]] size_t used() const noexcept { return capacity_ - free_count_; }
    [[nodiscard]] size_t available() const noexcept { return free_count_; }
    [[nodiscard]] double utilization() const noexcept {
        return capacity_ > 0 ? static_cast<double>(used()) / capacity_ : 0.0;
    }
    [[nodiscard]] bool empty() const noexcept { return free_count_ == capacity_; }
    [[nodiscard]] bool full() const noexcept { return free_count_ == 0; }

private:
    uint8_t* memory_;
    size_t capacity_;
    FreeNode* free_head_;
    std::atomic<size_t> free_count_;
};

// ==== FREE LIST ALLOCATOR ====
// General-purpose allocator with coalescing and splitting
class FreeListAllocator {
    struct FreeBlock {
        size_t size;
        FreeBlock* next;
        
        FreeBlock(size_t s) : size(s), next(nullptr) {}
    };
    
    struct AllocatedBlock {
        size_t size;
        // User data follows
    };
    
    static constexpr size_t MIN_BLOCK_SIZE = sizeof(FreeBlock);
    static constexpr size_t HEADER_SIZE = sizeof(AllocatedBlock);
    
public:
    explicit FreeListAllocator(size_t capacity) : capacity_(capacity) {
        // Allocate memory block
#ifdef _WIN32
        memory_ = static_cast<uint8_t*>(VirtualAlloc(nullptr, capacity,
                                                   MEM_COMMIT | MEM_RESERVE,
                                                   PAGE_READWRITE));
#else
        memory_ = static_cast<uint8_t*>(mmap(nullptr, capacity,
                                           PROT_READ | PROT_WRITE,
                                           MAP_PRIVATE | MAP_ANONYMOUS,
                                           -1, 0));
        if (memory_ == MAP_FAILED) memory_ = nullptr;
#endif
        
        if (!memory_) {
            throw std::bad_alloc{};
        }
        
        // Initialize with single large free block
        free_head_ = reinterpret_cast<FreeBlock*>(memory_);
        free_head_->size = capacity;
        free_head_->next = nullptr;
    }
    
    ~FreeListAllocator() {
        if (memory_) {
#ifdef _WIN32
            VirtualFree(memory_, 0, MEM_RELEASE);
#else
            munmap(memory_, capacity_);
#endif
        }
    }
    
    [[nodiscard]] void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        size = align_up(size + HEADER_SIZE, alignment);
        if (size < MIN_BLOCK_SIZE) size = MIN_BLOCK_SIZE;
        
        // Find suitable free block (first fit)
        FreeBlock* prev = nullptr;
        FreeBlock* current = free_head_;
        
        while (current) {
            if (current->size >= size) {
                // Found suitable block
                if (current->size > size + MIN_BLOCK_SIZE) {
                    // Split block
                    auto new_block = reinterpret_cast<FreeBlock*>(
                        reinterpret_cast<uint8_t*>(current) + size);
                    new_block->size = current->size - size;
                    new_block->next = current->next;
                    
                    if (prev) {
                        prev->next = new_block;
                    } else {
                        free_head_ = new_block;
                    }
                } else {
                    // Use entire block
                    if (prev) {
                        prev->next = current->next;
                    } else {
                        free_head_ = current->next;
                    }
                }
                
                // Set up allocated block header
                auto allocated = reinterpret_cast<AllocatedBlock*>(current);
                allocated->size = size;
                
                return reinterpret_cast<uint8_t*>(allocated) + HEADER_SIZE;
            }
            
            prev = current;
            current = current->next;
        }
        
        return nullptr; // Out of memory
    }
    
    void deallocate(void* ptr, size_t) {
        if (!ptr || !owns(ptr)) return;
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Get allocated block header
        auto allocated = reinterpret_cast<AllocatedBlock*>(
            static_cast<uint8_t*>(ptr) - HEADER_SIZE);
        
        // Convert to free block
        auto free_block = reinterpret_cast<FreeBlock*>(allocated);
        free_block->size = allocated->size;
        
        // Insert into free list (sorted by address for coalescing)
        FreeBlock* prev = nullptr;
        FreeBlock* current = free_head_;
        
        while (current && current < free_block) {
            prev = current;
            current = current->next;
        }
        
        // Insert block
        free_block->next = current;
        if (prev) {
            prev->next = free_block;
        } else {
            free_head_ = free_block;
        }
        
        // Coalesce with next block
        if (current && reinterpret_cast<uint8_t*>(free_block) + free_block->size == 
            reinterpret_cast<uint8_t*>(current)) {
            free_block->size += current->size;
            free_block->next = current->next;
        }
        
        // Coalesce with previous block
        if (prev && reinterpret_cast<uint8_t*>(prev) + prev->size == 
            reinterpret_cast<uint8_t*>(free_block)) {
            prev->size += free_block->size;
            prev->next = free_block->next;
        }
    }
    
    [[nodiscard]] bool owns(const void* ptr) const noexcept {
        if (!ptr) return false;
        const auto addr = reinterpret_cast<uintptr_t>(ptr);
        const auto start = reinterpret_cast<uintptr_t>(memory_);
        return addr >= start + HEADER_SIZE && addr < start + capacity_;
    }
    
    // Statistics
    [[nodiscard]] size_t capacity() const noexcept { return capacity_; }
    [[nodiscard]] size_t used() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t free_size = 0;
        for (auto block = free_head_; block; block = block->next) {
            free_size += block->size;
        }
        return capacity_ - free_size;
    }
    [[nodiscard]] size_t available() const noexcept { return capacity_ - used(); }

private:
    uint8_t* memory_;
    size_t capacity_;
    FreeBlock* free_head_;
    mutable std::mutex mutex_;
};

// ==== ALLOCATOR PERFORMANCE TESTS ====
namespace benchmarks {

template<Allocator AllocatorType>
struct AllocationBenchmark {
    static double measure_allocation_speed(AllocatorType& allocator, 
                                         size_t num_allocations, 
                                         size_t allocation_size) {
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<void*> ptrs;
        ptrs.reserve(num_allocations);
        
        for (size_t i = 0; i < num_allocations; ++i) {
            if (auto ptr = allocator.allocate(allocation_size)) {
                ptrs.push_back(ptr);
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        
        // Cleanup
        for (auto ptr : ptrs) {
            allocator.deallocate(ptr, allocation_size);
        }
        
        return static_cast<double>(duration.count()) / num_allocations;
    }
};

} // namespace benchmarks

} // namespace ecscope::memory