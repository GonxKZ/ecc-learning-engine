#pragma once

#include "core/types.hpp"
#include "core/result.hpp"
#include "core/log.hpp"
#include <memory>
#include <vector>
#include <string>
#include <mutex>

namespace ecscope::memory {

// Allocation metadata for tracking
struct AllocationInfo {
    void* ptr;
    usize size;
    usize alignment;
    const char* category;
    f64 timestamp;
    bool active;
    
    // Debug info
    const char* file;
    int line;
    const char* function;
};

// Arena allocation statistics
struct ArenaStats {
    usize total_size;           // Total arena size
    usize used_size;            // Currently allocated
    usize peak_usage;           // Peak usage reached
    usize wasted_bytes;         // Lost to alignment
    usize allocation_count;     // Total allocations made
    usize active_allocations;   // Currently active
    f64 fragmentation_ratio;    // Fragmentation score 0-1
    f64 efficiency_ratio;       // Used / Total
    
    // Performance metrics
    f64 total_alloc_time;       // Total time spent allocating
    f64 average_alloc_time;     // Average per allocation
    u64 cache_misses;           // Estimated cache misses
};

// Linear Arena Allocator - Educational Implementation
class ArenaAllocator {
private:
    byte* memory_;              // Raw memory block
    usize total_size_;          // Total arena size
    usize current_offset_;      // Current allocation offset
    usize peak_offset_;         // Peak usage tracking
    
    // Tracking data
    std::vector<AllocationInfo> allocations_;
    std::mutex tracking_mutex_; // Thread safety for tracking
    
    // Statistics
    ArenaStats stats_;
    
    // Configuration
    bool enable_tracking_;      // Detailed allocation tracking
    bool enable_debug_fill_;    // Fill memory with debug patterns
    byte debug_alloc_pattern_;  // Pattern for new allocations
    byte debug_free_pattern_;   // Pattern for freed memory
    
    std::string name_;          // Arena name for debugging
    
public:
    // Constructor
    explicit ArenaAllocator(usize size, const std::string& name = "Arena", bool enable_tracking = true);
    
    // Destructor
    ~ArenaAllocator();
    
    // Non-copyable but movable
    ArenaAllocator(const ArenaAllocator&) = delete;
    ArenaAllocator& operator=(const ArenaAllocator&) = delete;
    ArenaAllocator(ArenaAllocator&& other) noexcept;
    ArenaAllocator& operator=(ArenaAllocator&& other) noexcept;
    
    // Core allocation interface
    void* allocate(usize size, usize alignment = alignof(std::max_align_t), const char* category = nullptr);
    void* allocate_debug(usize size, usize alignment, const char* category, const char* file, int line, const char* function);
    
    // Arena-specific operations
    void reset(); // Reset to beginning (invalidates all pointers)
    void clear(); // Same as reset but also clears tracking data
    
    // Memory operations
    bool owns(const void* ptr) const noexcept;
    usize get_allocation_size(const void* ptr) const;
    
    // Statistics and introspection
    const ArenaStats& stats() const noexcept { return stats_; }
    void update_stats();
    
    // Configuration
    void set_debug_patterns(byte alloc_pattern, byte free_pattern);
    void set_tracking_enabled(bool enabled) { enable_tracking_ = enabled; }
    bool is_tracking_enabled() const noexcept { return enable_tracking_; }
    
    // Introspection
    const std::string& name() const noexcept { return name_; }
    usize total_size() const noexcept { return total_size_; }
    usize used_size() const noexcept { return current_offset_; }
    usize available_size() const noexcept { return total_size_ - current_offset_; }
    f64 usage_ratio() const noexcept { return static_cast<f64>(current_offset_) / total_size_; }
    
    // Memory layout information (for visualization)
    struct MemoryRegion {
        usize offset;
        usize size;
        bool allocated;
        const char* category;
        f64 age; // Time since allocation
    };
    
    std::vector<MemoryRegion> get_memory_layout() const;
    
    // Allocation list (for debugging)
    std::vector<AllocationInfo> get_active_allocations() const;
    std::vector<AllocationInfo> get_all_allocations() const;
    
    // Advanced operations
    void* try_allocate(usize size, usize alignment = alignof(std::max_align_t)); // Returns nullptr on failure
    bool can_allocate(usize size, usize alignment = alignof(std::max_align_t)) const noexcept;
    
    // Checkpoint system (for save/restore scenarios)
    struct Checkpoint {
        usize offset;
        usize allocation_count;
        f64 timestamp;
    };
    
    Checkpoint create_checkpoint() const;
    void restore_checkpoint(const Checkpoint& checkpoint);
    
private:
    void initialize_memory();
    void cleanup_memory();
    usize align_size(usize size, usize alignment) const noexcept;
    void record_allocation(void* ptr, usize size, usize alignment, const char* category, 
                          const char* file = nullptr, int line = 0, const char* function = nullptr);
    void fill_memory(void* ptr, usize size, byte pattern);
    void update_fragmentation_stats();
};

// Scoped Arena - RAII wrapper for temporary allocations
class ScopedArena {
private:
    ArenaAllocator& arena_;
    ArenaAllocator::Checkpoint checkpoint_;
    
public:
    explicit ScopedArena(ArenaAllocator& arena) 
        : arena_(arena), checkpoint_(arena.create_checkpoint()) {}
    
    ~ScopedArena() {
        arena_.restore_checkpoint(checkpoint_);
    }
    
    // Allocation interface
    void* allocate(usize size, usize alignment = alignof(std::max_align_t), const char* category = nullptr) {
        return arena_.allocate(size, alignment, category);
    }
    
    template<typename T>
    T* allocate(usize count = 1, const char* category = nullptr) {
        return static_cast<T*>(allocate(sizeof(T) * count, alignof(T), category));
    }
};

// Template helpers for type-safe allocation
template<typename T>
T* arena_allocate(ArenaAllocator& arena, usize count = 1, const char* category = nullptr) {
    void* ptr = arena.allocate(sizeof(T) * count, alignof(T), category);
    return static_cast<T*>(ptr);
}

template<typename T, typename... Args>
T* arena_construct(ArenaAllocator& arena, Args&&... args) {
    T* ptr = arena_allocate<T>(arena, 1, typeid(T).name());
    if (ptr) {
        new(ptr) T(std::forward<Args>(args)...);
    }
    return ptr;
}

// Convenience macros for debug allocation
#define ARENA_ALLOC(arena, size) \
    (arena).allocate_debug((size), alignof(std::max_align_t), nullptr, __FILE__, __LINE__, __FUNCTION__)

#define ARENA_ALLOC_T(arena, type, count) \
    static_cast<type*>((arena).allocate_debug(sizeof(type) * (count), alignof(type), #type, __FILE__, __LINE__, __FUNCTION__))

#define ARENA_ALLOC_CATEGORY(arena, size, category) \
    (arena).allocate_debug((size), alignof(std::max_align_t), (category), __FILE__, __LINE__, __FUNCTION__)

// Global arena registry for UI visualization
namespace arena_registry {
    void register_arena(ArenaAllocator* arena);
    void unregister_arena(ArenaAllocator* arena);
    std::vector<ArenaAllocator*> get_all_arenas();
    ArenaStats get_combined_stats();
}

} // namespace ecscope::memory