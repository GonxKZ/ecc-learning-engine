# ECScope Memory Management System

**Comprehensive documentation of allocator strategies, memory tracking, and performance optimization techniques**

## Table of Contents

1. [Memory Management Philosophy](#memory-management-philosophy)
2. [Allocator Architecture](#allocator-architecture)
3. [Arena Allocator Deep Dive](#arena-allocator-deep-dive)
4. [Pool Allocator Deep Dive](#pool-allocator-deep-dive)
5. [PMR Integration](#pmr-integration)
6. [Memory Tracking System](#memory-tracking-system)
7. [Performance Analysis](#performance-analysis)
8. [Educational Features](#educational-features)
9. [Advanced Patterns](#advanced-patterns)

## Memory Management Philosophy

### Why Custom Memory Management?

ECScope implements custom memory management for both educational and performance reasons:

**Educational Benefits**:
- **Visibility**: Make memory allocation behavior observable and understandable
- **Comparison**: Demonstrate differences between allocation strategies
- **Hands-on Learning**: Provide practical experience with memory optimization
- **Real-world Skills**: Teach techniques used in production game engines

**Performance Benefits**:
- **Cache Optimization**: Improve cache locality through strategic memory layout
- **Reduced Fragmentation**: Minimize memory fragmentation with specialized allocators
- **Predictable Performance**: Eliminate allocation spikes and memory pressure issues
- **Memory Efficiency**: Optimize memory usage for specific access patterns

### Memory Hierarchy Understanding

**The Memory Performance Cliff**:
```
Performance Relative to L1 Cache:
L1 Cache:     1x    (1-3 cycles)
L2 Cache:     3x    (10-20 cycles)  
L3 Cache:    10x    (20-40 cycles)
Main Memory: 100x   (100-300 cycles)
SSD:       10,000x  (10,000+ cycles)
HDD:      100,000x  (100,000+ cycles)
```

**Cache Behavior Fundamentals**:
- **Cache Line Size**: 64 bytes (typical) - accessing 1 byte loads 64 bytes
- **Spatial Locality**: Accessing nearby memory is fast (same cache line)
- **Temporal Locality**: Accessing recently used memory is fast (still in cache)
- **Cache Associativity**: How cache lines map to memory addresses

### ECScope Memory Strategy

**Core Strategy**: Use different allocators for different data access patterns:

1. **Arena Allocators**: For sequential, temporary data (component arrays)
2. **Pool Allocators**: For fixed-size, recyclable data (entity IDs, contact points)
3. **PMR Resources**: For optimizing standard library containers
4. **Standard Allocation**: For irregular, long-lived data

## Allocator Architecture

### Allocator Hierarchy

```cpp
// Base allocator interface for educational polymorphism
class IAllocator {
public:
    virtual ~IAllocator() = default;
    
    // Core allocation interface
    virtual void* allocate(usize size, usize alignment = alignof(std::max_align_t)) = 0;
    virtual void deallocate(void* ptr, usize size) = 0;
    virtual bool can_deallocate() const = 0;
    
    // Educational interface
    virtual std::string name() const = 0;
    virtual AllocatorStats get_stats() const = 0;
    virtual std::string get_educational_description() const = 0;
    virtual void log_status() const = 0;
    
    // Memory introspection
    virtual usize total_size() const = 0;
    virtual usize used_size() const = 0;
    virtual usize available_size() const = 0;
    virtual f64 utilization_ratio() const = 0;
};

// Concrete allocator implementations
class ArenaAllocator : public IAllocator { /* ... */ };
class PoolAllocator : public IAllocator { /* ... */ };
class TrackingAllocator : public IAllocator { /* ... */ };
```

### Allocator Configuration System

```cpp
// Educational allocator configuration
struct MemoryConfig {
    // Arena allocator settings
    usize arena_size{8 * MB};
    usize arena_alignment{64}; // Cache line alignment
    bool arena_debug_tracking{true};
    
    // Pool allocator settings
    usize pool_block_size{64};
    usize pool_initial_capacity{1000};
    bool pool_auto_expand{true};
    
    // PMR settings
    bool enable_pmr_optimization{true};
    usize pmr_buffer_size{1 * MB};
    
    // Educational settings
    bool enable_comprehensive_tracking{true};
    bool enable_performance_comparison{true};
    bool enable_debug_logging{false};
    bool enable_cache_analysis{true};
    
    // Factory methods for different use cases
    static MemoryConfig create_educational() {
        MemoryConfig config;
        config.arena_size = 2 * MB; // Smaller for educational examples
        config.enable_comprehensive_tracking = true;
        config.enable_debug_logging = true;
        return config;
    }
    
    static MemoryConfig create_performance() {
        MemoryConfig config;
        config.arena_size = 16 * MB; // Larger for performance
        config.enable_comprehensive_tracking = false; // Minimal overhead
        config.enable_debug_logging = false;
        return config;
    }
    
    static MemoryConfig create_research() {
        MemoryConfig config;
        config.enable_comprehensive_tracking = true;
        config.enable_performance_comparison = true;
        config.enable_cache_analysis = true;
        config.enable_debug_logging = true;
        return config;
    }
};
```

## Arena Allocator Deep Dive

### Arena Allocator Implementation

```cpp
/**
 * @brief Linear Arena Allocator for Educational Memory Management
 * 
 * The arena allocator provides fast, sequential memory allocation with zero
 * fragmentation. Perfect for temporary data and cache-friendly access patterns.
 * 
 * Educational Features:
 * - Visual memory layout representation
 * - Allocation pattern analysis
 * - Cache behavior prediction
 * - Memory waste tracking and optimization suggestions
 */
class ArenaAllocator : public IAllocator {
private:
    byte* memory_;              // Raw memory block
    usize total_size_;          // Total arena size
    usize current_offset_;      // Current allocation position
    usize alignment_;           // Default alignment requirement
    
    // Educational tracking
    mutable ArenaStats stats_;
    std::vector<AllocationInfo> allocations_; // Debug allocation tracking
    std::string arena_name_;
    
    // Performance tracking
    mutable std::chrono::high_resolution_clock::time_point last_allocation_time_;
    mutable f64 total_allocation_time_{0.0};
    
public:
    /**
     * @brief Create arena allocator with educational features
     */
    ArenaAllocator(usize size, usize alignment = 64, const std::string& name = "Arena")
        : total_size_(size)
        , current_offset_(0)
        , alignment_(alignment)
        , arena_name_(name) {
        
        // Allocate aligned memory block
        memory_ = static_cast<byte*>(std::aligned_alloc(alignment_, total_size_));
        if (!memory_) {
            throw std::bad_alloc{};
        }
        
        // Initialize statistics
        stats_.total_size = total_size_;
        stats_.used_size = 0;
        stats_.peak_usage = 0;
        stats_.allocation_count = 0;
        stats_.active_allocations = 0;
        
        LOG_INFO("Arena '{}' created: {} MB, {}-byte aligned", 
                name, total_size_ / MB, alignment_);
    }
    
    ~ArenaAllocator() {
        if (memory_) {
            std::free(memory_);
        }
        
        // Educational: Log final statistics
        LOG_INFO("Arena '{}' destroyed:", arena_name_);
        LOG_INFO("  Peak usage: {} KB ({:.2f}%)", 
                stats_.peak_usage / KB, (stats_.peak_usage * 100.0) / total_size_);
        LOG_INFO("  Total allocations: {}", stats_.allocation_count);
        LOG_INFO("  Average allocation time: {:.3f} μs", stats_.average_alloc_time * 1000000.0);
        LOG_INFO("  Memory efficiency: {:.2f}%", stats_.efficiency_ratio * 100.0);
    }
    
    // Fast linear allocation
    void* allocate(usize size, usize alignment = 0) override {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        if (alignment == 0) alignment = alignment_;
        
        // Align current offset
        usize aligned_offset = align_offset(current_offset_, alignment);
        usize allocation_end = aligned_offset + size;
        
        // Check if allocation fits
        if (allocation_end > total_size_) {
            LOG_ERROR("Arena '{}' out of memory: requested {} bytes, available {} bytes",
                     arena_name_, size, total_size_ - current_offset_);
            return nullptr;
        }
        
        // Perform allocation
        void* ptr = memory_ + aligned_offset;
        current_offset_ = allocation_end;
        
        // Update statistics
        stats_.used_size = current_offset_;
        stats_.allocation_count++;
        stats_.active_allocations++;
        if (stats_.used_size > stats_.peak_usage) {
            stats_.peak_usage = stats_.used_size;
        }
        
        // Calculate wasted bytes due to alignment
        usize alignment_waste = aligned_offset - (current_offset_ - size);
        stats_.wasted_bytes += alignment_waste;
        
        // Update efficiency metrics
        stats_.efficiency_ratio = static_cast<f64>(stats_.used_size - stats_.wasted_bytes) / total_size_;
        
        // Performance tracking
        auto allocation_time = std::chrono::duration<f64>(
            std::chrono::high_resolution_clock::now() - start_time).count();
        total_allocation_time_ += allocation_time;
        stats_.average_alloc_time = total_allocation_time_ / stats_.allocation_count;
        
        // Educational logging
        if (allocations_.size() < 100) { // Limit debug allocations
            allocations_.push_back({
                .ptr = ptr,
                .size = size,
                .alignment = alignment,
                .category = "arena",
                .timestamp = allocation_time,
                .active = true
            });
        }
        
        LOG_DEBUG("Arena allocation: {} bytes at offset {} ({}% used)",
                 size, aligned_offset, (current_offset_ * 100) / total_size_);
        
        return ptr;
    }
    
    // Arena allocators don't support individual deallocation
    void deallocate(void* ptr, usize size) override {
        // Educational: Explain why this is not supported
        LOG_DEBUG("Arena allocator does not support individual deallocation");
        LOG_DEBUG("Use reset() to deallocate all arena memory at once");
        
        // Mark allocation as "deallocated" for educational tracking
        for (auto& alloc : allocations_) {
            if (alloc.ptr == ptr && alloc.size == size) {
                alloc.active = false;
                stats_.active_allocations--;
                break;
            }
        }
    }
    
    bool can_deallocate() const override { return false; }
    
    // Reset entire arena (fast bulk deallocation)
    void reset() {
        current_offset_ = 0;
        stats_.used_size = 0;
        stats_.active_allocations = 0;
        
        // Clear debug allocations
        for (auto& alloc : allocations_) {
            alloc.active = false;
        }
        
        LOG_INFO("Arena '{}' reset - all {} bytes available", arena_name_, total_size_);
    }
    
    // Educational interface implementation
    std::string name() const override { return arena_name_; }
    
    AllocatorStats get_stats() const override {
        return AllocatorStats{
            .total_size = total_size_,
            .used_size = current_offset_,
            .peak_usage = stats_.peak_usage,
            .allocation_count = stats_.allocation_count,
            .active_allocations = stats_.active_allocations,
            .fragmentation_ratio = 0.0, // Arena has zero fragmentation
            .efficiency_ratio = stats_.efficiency_ratio,
            .average_alloc_time = stats_.average_alloc_time
        };
    }
    
    std::string get_educational_description() const override {
        return "Linear Arena Allocator: Fast sequential allocation with zero fragmentation. "
               "Perfect for temporary data and cache-friendly access patterns. "
               "Cannot deallocate individual allocations - use reset() for bulk deallocation.";
    }
    
    // Educational visualization
    void visualize_memory_layout() const {
        std::cout << "=== Arena Memory Layout ===\n";
        std::cout << "Total Size: " << total_size_ << " bytes\n";
        std::cout << "Used Size: " << current_offset_ << " bytes\n";
        std::cout << "Free Size: " << (total_size_ - current_offset_) << " bytes\n";
        std::cout << "Utilization: " << (current_offset_ * 100.0 / total_size_) << "%\n\n";
        
        // Visual representation
        const usize bar_width = 50;
        usize used_chars = (current_offset_ * bar_width) / total_size_;
        
        std::cout << "Memory: [";
        for (usize i = 0; i < bar_width; ++i) {
            if (i < used_chars) {
                std::cout << "█";
            } else {
                std::cout << "░";
            }
        }
        std::cout << "]\n";
        std::cout << "        ^used (" << used_chars << "/" << bar_width << ")\n";
    }
    
private:
    usize align_offset(usize offset, usize alignment) const {
        return (offset + alignment - 1) & ~(alignment - 1);
    }
};
```

### Arena Allocator Use Cases

**Ideal Use Cases**:
1. **Component Arrays**: Sequential component storage in ECS archetypes
2. **Temporary Data**: Per-frame allocations that are bulk-deallocated
3. **Loading Operations**: Bulk data loading with known lifetime
4. **Cache-Critical Paths**: Code where cache performance is paramount

**Educational Example**:
```cpp
void arena_use_case_demo() {
    // Scenario: Loading and processing a level's entities
    memory::ArenaAllocator level_arena{4 * MB, 64, "Level_Loading_Arena"};
    
    // Load entity data into arena (cache-friendly)
    std::vector<Transform*> transforms;
    std::vector<RenderComponent*> render_components;
    
    for (i32 i = 0; i < 10000; ++i) {
        // All transforms allocated sequentially in memory
        transforms.push_back(level_arena.allocate<Transform>());
        render_components.push_back(level_arena.allocate<RenderComponent>());
    }
    
    // Process data (excellent cache performance)
    {
        ScopeTimer timer("Sequential Processing");
        for (usize i = 0; i < transforms.size(); ++i) {
            // Sequential access = excellent cache utilization
            transforms[i]->position.x += 1.0f;
            render_components[i]->tint.a = 255;
        }
    }
    
    // When level ends, bulk deallocate everything instantly
    level_arena.reset(); // All memory available again
    
    // Educational: Show memory efficiency
    level_arena.visualize_memory_layout();
}
```

## Pool Allocator Deep Dive

### Pool Allocator Implementation

```cpp
/**
 * @brief Pool Allocator for Fixed-Size Educational Memory Management
 * 
 * The pool allocator provides fast allocation and deallocation for fixed-size objects.
 * Excellent for objects with predictable lifetimes and frequent allocation/deallocation.
 * 
 * Educational Features:
 * - Fragmentation analysis and visualization
 * - Allocation pattern tracking
 * - Memory reuse efficiency analysis
 * - Performance comparison with standard allocation
 */
class PoolAllocator : public IAllocator {
private:
    struct FreeBlock {
        FreeBlock* next;
    };
    
    byte* memory_;              // Raw memory pool
    usize block_size_;          // Size of each block
    usize block_count_;         // Total number of blocks
    usize alignment_;           // Block alignment
    FreeBlock* free_list_;      // Linked list of free blocks
    
    // Educational tracking
    usize allocated_blocks_{0};
    usize peak_allocated_blocks_{0};
    usize total_allocations_{0};
    usize total_deallocations_{0};
    f64 total_allocation_time_{0.0};
    f64 total_deallocation_time_{0.0};
    std::string pool_name_;
    
    // Fragmentation analysis
    std::vector<bool> block_allocation_map_; // Track which blocks are allocated
    
public:
    PoolAllocator(usize block_size, usize block_count, usize alignment = alignof(std::max_align_t),
                  const std::string& name = "Pool")
        : block_size_(align_size(block_size, alignment))
        , block_count_(block_count)
        , alignment_(alignment)
        , pool_name_(name)
        , block_allocation_map_(block_count, false) {
        
        // Allocate aligned memory pool
        usize total_size = block_size_ * block_count_;
        memory_ = static_cast<byte*>(std::aligned_alloc(alignment_, total_size));
        if (!memory_) {
            throw std::bad_alloc{};
        }
        
        // Initialize free list
        initialize_free_list();
        
        LOG_INFO("Pool '{}' created: {} blocks of {} bytes each ({} KB total)",
                name, block_count_, block_size_, total_size / KB);
    }
    
    ~PoolAllocator() {
        if (memory_) {
            std::free(memory_);
        }
        
        // Educational: Final statistics
        LOG_INFO("Pool '{}' destroyed:", pool_name_);
        LOG_INFO("  Total allocations: {}", total_allocations_);
        LOG_INFO("  Total deallocations: {}", total_deallocations_);
        LOG_INFO("  Peak usage: {} blocks ({:.2f}%)", 
                peak_allocated_blocks_, (peak_allocated_blocks_ * 100.0) / block_count_);
        LOG_INFO("  Final allocated blocks: {}", allocated_blocks_);
        
        if (allocated_blocks_ > 0) {
            LOG_WARN("Pool destroyed with {} active allocations (memory leak!)", allocated_blocks_);
        }
    }
    
    // Fast fixed-size allocation
    void* allocate(usize size, usize alignment = 0) override {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Validate allocation size
        if (size > block_size_) {
            LOG_ERROR("Pool allocation failed: requested {} bytes, block size is {} bytes",
                     size, block_size_);
            return nullptr;
        }
        
        // Check for available blocks
        if (!free_list_) {
            LOG_ERROR("Pool '{}' exhausted: no free blocks available", pool_name_);
            return nullptr;
        }
        
        // Allocate from free list
        void* ptr = free_list_;
        free_list_ = free_list_->next;
        
        // Update tracking
        allocated_blocks_++;
        total_allocations_++;
        if (allocated_blocks_ > peak_allocated_blocks_) {
            peak_allocated_blocks_ = allocated_blocks_;
        }
        
        // Track which block was allocated
        usize block_index = (static_cast<byte*>(ptr) - memory_) / block_size_;
        block_allocation_map_[block_index] = true;
        
        // Performance tracking
        auto allocation_time = std::chrono::duration<f64>(
            std::chrono::high_resolution_clock::now() - start_time).count();
        total_allocation_time_ += allocation_time;
        
        LOG_DEBUG("Pool allocation: block {} ({} bytes) in {:.3f} μs",
                 block_index, size, allocation_time * 1000000.0);
        
        return ptr;
    }
    
    // Fast fixed-size deallocation
    void deallocate(void* ptr, usize size) override {
        if (!ptr) return;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Validate pointer is from this pool
        if (!is_valid_pool_pointer(ptr)) {
            LOG_ERROR("Invalid deallocation: pointer not from pool '{}'", pool_name_);
            return;
        }
        
        // Add back to free list
        FreeBlock* block = static_cast<FreeBlock*>(ptr);
        block->next = free_list_;
        free_list_ = block;
        
        // Update tracking
        allocated_blocks_--;
        total_deallocations_++;
        
        // Track which block was deallocated
        usize block_index = (static_cast<byte*>(ptr) - memory_) / block_size_;
        block_allocation_map_[block_index] = false;
        
        // Performance tracking
        auto deallocation_time = std::chrono::duration<f64>(
            std::chrono::high_resolution_clock::now() - start_time).count();
        total_deallocation_time_ += deallocation_time;
        
        LOG_DEBUG("Pool deallocation: block {} in {:.3f} μs", 
                 block_index, deallocation_time * 1000000.0);
    }
    
    bool can_deallocate() const override { return true; }
    
    // Educational interface
    std::string get_educational_description() const override {
        return "Pool Allocator: Fast allocation/deallocation for fixed-size objects. "
               "Eliminates fragmentation and provides predictable performance. "
               "Ideal for objects with frequent allocation/deallocation cycles.";
    }
    
    // Educational: Fragmentation analysis
    f64 calculate_fragmentation() const {
        if (allocated_blocks_ == 0) return 0.0;
        
        // Analyze allocation pattern to determine fragmentation
        usize contiguous_regions = 0;
        bool in_allocated_region = false;
        
        for (bool is_allocated : block_allocation_map_) {
            if (is_allocated && !in_allocated_region) {
                contiguous_regions++;
                in_allocated_region = true;
            } else if (!is_allocated) {
                in_allocated_region = false;
            }
        }
        
        // Fragmentation score: 1.0 = maximally fragmented, 0.0 = no fragmentation
        return (contiguous_regions > 1) ? 
            static_cast<f64>(contiguous_regions - 1) / allocated_blocks_ : 0.0;
    }
    
    // Educational: Memory layout visualization
    void visualize_pool_layout() const {
        std::cout << "=== Pool Memory Layout ===\n";
        std::cout << "Block size: " << block_size_ << " bytes\n";
        std::cout << "Total blocks: " << block_count_ << "\n";
        std::cout << "Allocated: " << allocated_blocks_ << " (" 
                  << (allocated_blocks_ * 100.0 / block_count_) << "%)\n";
        std::cout << "Fragmentation: " << (calculate_fragmentation() * 100.0) << "%\n\n";
        
        // Visual representation (first 64 blocks)
        usize blocks_to_show = std::min(block_count_, 64UL);
        std::cout << "Layout: ";
        for (usize i = 0; i < blocks_to_show; ++i) {
            std::cout << (block_allocation_map_[i] ? "█" : "░");
        }
        if (block_count_ > 64) {
            std::cout << "... (showing first 64 blocks)";
        }
        std::cout << "\n        █ = allocated, ░ = free\n";
    }
    
private:
    void initialize_free_list() {
        free_list_ = reinterpret_cast<FreeBlock*>(memory_);
        
        // Link all blocks in free list
        for (usize i = 0; i < block_count_ - 1; ++i) {
            FreeBlock* current = reinterpret_cast<FreeBlock*>(memory_ + i * block_size_);
            FreeBlock* next = reinterpret_cast<FreeBlock*>(memory_ + (i + 1) * block_size_);
            current->next = next;
        }
        
        // Last block points to null
        FreeBlock* last = reinterpret_cast<FreeBlock*>(memory_ + (block_count_ - 1) * block_size_);
        last->next = nullptr;
    }
    
    bool is_valid_pool_pointer(void* ptr) const {
        byte* byte_ptr = static_cast<byte*>(ptr);
        return byte_ptr >= memory_ && 
               byte_ptr < (memory_ + block_size_ * block_count_) &&
               ((byte_ptr - memory_) % block_size_) == 0;
    }
    
    usize align_size(usize size, usize alignment) const {
        return (size + alignment - 1) & ~(alignment - 1);
    }
};
```

### Pool Allocator Use Cases

**Optimal Use Cases**:
1. **Entity ID Management**: Fixed-size entity structures
2. **Contact Points**: Physics collision contact data
3. **Render Commands**: Fixed-size rendering instructions
4. **Event Objects**: Game events with consistent size

**Educational Comparison Example**:
```cpp
void pool_vs_standard_comparison() {
    const usize num_allocations = 10000;
    const usize object_size = sizeof(Transform);
    
    // Test 1: Standard allocation with frequent alloc/dealloc
    {
        ScopeTimer timer("Standard Allocation (frequent alloc/dealloc)");
        std::vector<Transform*> objects;
        
        for (usize i = 0; i < num_allocations; ++i) {
            objects.push_back(new Transform{});
            
            // Simulate frequent deallocation
            if (i % 100 == 0 && !objects.empty()) {
                delete objects.back();
                objects.pop_back();
            }
        }
        
        // Cleanup
        for (auto* obj : objects) {
            delete obj;
        }
    }
    
    // Test 2: Pool allocation with frequent alloc/dealloc
    {
        ScopeTimer timer("Pool Allocation (frequent alloc/dealloc)");
        memory::PoolAllocator pool{object_size, num_allocations, 64, "Comparison_Pool"};
        std::vector<Transform*> objects;
        
        for (usize i = 0; i < num_allocations; ++i) {
            objects.push_back(pool.allocate<Transform>());
            
            // Simulate frequent deallocation
            if (i % 100 == 0 && !objects.empty()) {
                pool.deallocate(objects.back(), object_size);
                objects.pop_back();
            }
        }
        
        // Cleanup
        for (auto* obj : objects) {
            pool.deallocate(obj, object_size);
        }
        
        // Educational: Show pool efficiency
        pool.visualize_pool_layout();
    }
    
    std::cout << "Check timing results above to see the performance difference!\n";
    std::cout << "Pool allocation is typically 2-5x faster for frequent alloc/dealloc patterns.\n";
}
```

## PMR Integration

### Polymorphic Memory Resources

ECScope integrates C++20 PMR (Polymorphic Memory Resources) for optimizing standard library containers:

```cpp
namespace memory::pmr {

/**
 * @brief Educational PMR memory resource that combines multiple allocation strategies
 */
class HybridMemoryResource : public std::pmr::memory_resource {
private:
    memory::ArenaAllocator* arena_;
    memory::PoolAllocator* pool_;
    std::pmr::memory_resource* fallback_;
    
    // Educational tracking
    mutable usize arena_allocations_{0};
    mutable usize pool_allocations_{0};
    mutable usize fallback_allocations_{0};
    
public:
    HybridMemoryResource(memory::ArenaAllocator& arena, 
                        memory::PoolAllocator& pool,
                        std::pmr::memory_resource* fallback = std::pmr::get_default_resource())
        : arena_(&arena), pool_(&pool), fallback_(fallback) {}
    
protected:
    // Smart allocation routing based on size and alignment
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        // Route to appropriate allocator based on allocation characteristics
        if (bytes <= pool_->block_size() && alignment <= pool_->alignment()) {
            // Use pool for small, fixed-size allocations
            auto* ptr = pool_->allocate(bytes, alignment);
            if (ptr) {
                pool_allocations_++;
                LOG_DEBUG("PMR: Routed {} bytes to pool allocator", bytes);
                return ptr;
            }
        }
        
        if (bytes < 1024 && alignment <= arena_->alignment()) {
            // Use arena for medium-size allocations
            auto* ptr = arena_->allocate(bytes, alignment);
            if (ptr) {
                arena_allocations_++;
                LOG_DEBUG("PMR: Routed {} bytes to arena allocator", bytes);
                return ptr;
            }
        }
        
        // Fallback to standard allocation for large or special cases
        fallback_allocations_++;
        LOG_DEBUG("PMR: Routed {} bytes to fallback allocator", bytes);
        return fallback_->allocate(bytes, alignment);
    }
    
    void do_deallocate(void* ptr, std::size_t bytes, std::size_t alignment) override {
        // Try pool deallocation first
        if (pool_->is_valid_pool_pointer(ptr)) {
            pool_->deallocate(ptr, bytes);
            return;
        }
        
        // Arena allocator doesn't support individual deallocation
        if (arena_->owns_pointer(ptr)) {
            LOG_DEBUG("PMR: Arena allocation cannot be individually deallocated");
            return;
        }
        
        // Use fallback deallocation
        fallback_->deallocate(ptr, bytes, alignment);
    }
    
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
    
public:
    // Educational: Allocation strategy statistics
    struct AllocationBreakdown {
        usize arena_allocations;
        usize pool_allocations;
        usize fallback_allocations;
        usize total_allocations;
        
        f64 arena_percentage() const {
            return total_allocations > 0 ? 
                (arena_allocations * 100.0) / total_allocations : 0.0;
        }
        
        f64 pool_percentage() const {
            return total_allocations > 0 ? 
                (pool_allocations * 100.0) / total_allocations : 0.0;
        }
        
        f64 fallback_percentage() const {
            return total_allocations > 0 ? 
                (fallback_allocations * 100.0) / total_allocations : 0.0;
        }
    };
    
    AllocationBreakdown get_allocation_breakdown() const {
        return AllocationBreakdown{
            .arena_allocations = arena_allocations_,
            .pool_allocations = pool_allocations_,
            .fallback_allocations = fallback_allocations_,
            .total_allocations = arena_allocations_ + pool_allocations_ + fallback_allocations_
        };
    }
    
    void log_allocation_breakdown() const {
        auto breakdown = get_allocation_breakdown();
        
        LOG_INFO("PMR Allocation Breakdown for '{}':", pool_name_);
        LOG_INFO("  Arena: {} allocations ({:.1f}%)", 
                breakdown.arena_allocations, breakdown.arena_percentage());
        LOG_INFO("  Pool: {} allocations ({:.1f}%)", 
                breakdown.pool_allocations, breakdown.pool_percentage());
        LOG_INFO("  Fallback: {} allocations ({:.1f}%)", 
                breakdown.fallback_allocations, breakdown.fallback_percentage());
    }
};

// Factory function for educational PMR resource creation
std::unique_ptr<HybridMemoryResource> create_educational_memory_resource(
    memory::ArenaAllocator& arena, 
    memory::PoolAllocator& pool) {
    
    auto resource = std::make_unique<HybridMemoryResource>(arena, pool);
    
    LOG_INFO("Created hybrid PMR resource:");
    LOG_INFO("  Arena capacity: {} KB", arena.total_size() / KB);
    LOG_INFO("  Pool capacity: {} blocks of {} bytes", pool.total_capacity(), pool.block_size());
    
    return resource;
}

} // namespace memory::pmr
```

### PMR Container Optimization

**Before PMR Optimization**:
```cpp
// Standard containers with default allocation
void standard_container_performance() {
    ScopeTimer timer("Standard Containers");
    
    std::vector<Transform> transforms;
    std::unordered_map<Entity, Transform> entity_map;
    
    for (i32 i = 0; i < 10000; ++i) {
        transforms.emplace_back(Transform{Vec2{i, i}});
        entity_map.emplace(Entity{i}, Transform{Vec2{i*2, i*2}});
    }
    // Multiple allocations, potential fragmentation, cache misses
}
```

**After PMR Optimization**:
```cpp
// PMR containers with custom memory resource
void pmr_container_performance() {
    ScopeTimer timer("PMR Containers");
    
    // Create custom memory resource
    memory::ArenaAllocator arena{1 * MB};
    memory::PoolAllocator pool{64, 1000};
    auto pmr_resource = memory::pmr::create_educational_memory_resource(arena, pool);
    
    std::pmr::vector<Transform> transforms{pmr_resource.get()};
    std::pmr::unordered_map<Entity, Transform> entity_map{pmr_resource.get()};
    
    for (i32 i = 0; i < 10000; ++i) {
        transforms.emplace_back(Transform{Vec2{i, i}});
        entity_map.emplace(Entity{i}, Transform{Vec2{i*2, i*2}});
    }
    // Optimized allocation, better cache locality, reduced fragmentation
    
    // Educational: Show allocation breakdown
    pmr_resource->log_allocation_breakdown();
}
```

## Memory Tracking System

### Comprehensive Memory Analysis

```cpp
/**
 * @brief Global Memory Tracker for Educational Analysis
 * 
 * Provides comprehensive memory allocation tracking, cache behavior analysis,
 * and educational insights into memory usage patterns.
 */
namespace memory::tracker {

struct MemoryAccessEvent {
    void* address;
    usize size;
    bool is_write;
    f64 timestamp;
    std::string context;
    
    // Cache analysis
    bool likely_cache_hit;
    usize cache_line_index;
};

class GlobalMemoryTracker {
private:
    std::vector<MemoryAccessEvent> access_events_;
    std::unordered_map<void*, AllocationInfo> active_allocations_;
    
    // Cache simulation for educational purposes
    struct CacheLineInfo {
        void* base_address;
        f64 last_access_time;
        usize access_count;
    };
    std::unordered_map<usize, CacheLineInfo> cache_line_simulation_;
    
    // Educational statistics
    usize total_allocations_{0};
    usize total_deallocations_{0};
    usize total_accesses_{0};
    usize cache_hits_{0};
    f64 total_memory_allocated_{0};
    
public:
    // Track memory allocation
    void track_allocation(void* ptr, usize size, const char* category = "unknown") {
        if (!ptr) return;
        
        AllocationInfo info{
            .ptr = ptr,
            .size = size,
            .category = category,
            .timestamp = get_current_timestamp(),
            .active = true
        };
        
        active_allocations_[ptr] = info;
        total_allocations_++;
        total_memory_allocated_ += size;
        
        LOG_DEBUG("Memory allocation tracked: {} bytes in category '{}'", size, category);
    }
    
    // Track memory access with cache simulation
    void track_access(void* ptr, usize size, bool is_write = false, 
                     const std::string& context = "unknown") {
        total_accesses_++;
        
        // Simulate cache behavior for educational purposes
        usize cache_line = reinterpret_cast<usize>(ptr) / 64; // 64-byte cache lines
        f64 current_time = get_current_timestamp();
        
        bool cache_hit = false;
        auto cache_it = cache_line_simulation_.find(cache_line);
        if (cache_it != cache_line_simulation_.end()) {
            // Check if this is likely a cache hit (accessed recently)
            f64 time_since_access = current_time - cache_it->second.last_access_time;
            cache_hit = time_since_access < 0.001; // 1ms cache retention simulation
            
            cache_it->second.last_access_time = current_time;
            cache_it->second.access_count++;
        } else {
            // First access to this cache line
            cache_line_simulation_[cache_line] = CacheLineInfo{
                .base_address = reinterpret_cast<void*>((cache_line * 64)),
                .last_access_time = current_time,
                .access_count = 1
            };
        }
        
        if (cache_hit) {
            cache_hits_++;
        }
        
        // Record access event for analysis
        access_events_.push_back(MemoryAccessEvent{
            .address = ptr,
            .size = size,
            .is_write = is_write,
            .timestamp = current_time,
            .context = context,
            .likely_cache_hit = cache_hit,
            .cache_line_index = cache_line
        });
        
        // Limit event history for memory efficiency
        if (access_events_.size() > 100000) {
            access_events_.erase(access_events_.begin(), access_events_.begin() + 50000);
        }
    }
    
    // Educational: Generate memory usage report
    std::string generate_memory_report() const {
        std::ostringstream report;
        
        report << "=== Global Memory Tracker Report ===\n";
        report << "Total allocations: " << total_allocations_ << "\n";
        report << "Active allocations: " << active_allocations_.size() << "\n";
        report << "Total memory allocated: " << (total_memory_allocated_ / MB) << " MB\n";
        report << "Total memory accesses: " << total_accesses_ << "\n";
        report << "Cache hit ratio: " << (cache_hits_ * 100.0 / total_accesses_) << "%\n";
        
        // Allocation breakdown by category
        std::unordered_map<std::string, usize> category_sizes;
        for (const auto& [ptr, info] : active_allocations_) {
            category_sizes[info.category] += info.size;
        }
        
        report << "\nMemory by category:\n";
        for (const auto& [category, size] : category_sizes) {
            report << "  " << category << ": " << (size / KB) << " KB\n";
        }
        
        return report.str();
    }
    
    // Educational: Cache analysis
    struct CacheAnalysisResult {
        f64 overall_hit_ratio;
        f64 sequential_access_ratio;
        f64 random_access_ratio;
        usize unique_cache_lines_accessed;
        f64 cache_line_utilization;
        std::vector<std::string> optimization_suggestions;
    };
    
    CacheAnalysisResult analyze_cache_behavior() const {
        CacheAnalysisResult result;
        result.overall_hit_ratio = total_accesses_ > 0 ? 
            static_cast<f64>(cache_hits_) / total_accesses_ : 0.0;
        
        // Analyze access patterns
        usize sequential_accesses = 0;
        usize random_accesses = 0;
        
        for (usize i = 1; i < access_events_.size(); ++i) {
            const auto& current = access_events_[i];
            const auto& previous = access_events_[i-1];
            
            // Check if this is sequential access (same or adjacent cache line)
            if (std::abs(static_cast<i64>(current.cache_line_index) - 
                        static_cast<i64>(previous.cache_line_index)) <= 1) {
                sequential_accesses++;
            } else {
                random_accesses++;
            }
        }
        
        usize total_pattern_accesses = sequential_accesses + random_accesses;
        result.sequential_access_ratio = total_pattern_accesses > 0 ? 
            static_cast<f64>(sequential_accesses) / total_pattern_accesses : 0.0;
        result.random_access_ratio = 1.0 - result.sequential_access_ratio;
        
        result.unique_cache_lines_accessed = cache_line_simulation_.size();
        result.cache_line_utilization = calculate_cache_line_utilization();
        
        // Generate optimization suggestions
        generate_optimization_suggestions(result);
        
        return result;
    }
    
private:
    f64 calculate_cache_line_utilization() const {
        if (cache_line_simulation_.empty()) return 0.0;
        
        usize total_accesses_to_lines = 0;
        for (const auto& [line_index, info] : cache_line_simulation_) {
            total_accesses_to_lines += info.access_count;
        }
        
        return static_cast<f64>(total_accesses_to_lines) / cache_line_simulation_.size();
    }
    
    void generate_optimization_suggestions(CacheAnalysisResult& result) const {
        if (result.overall_hit_ratio < 0.7) {
            result.optimization_suggestions.push_back(
                "Low cache hit ratio - consider using arena allocators for sequential data");
        }
        
        if (result.random_access_ratio > 0.6) {
            result.optimization_suggestions.push_back(
                "High random access ratio - consider data layout reorganization");
        }
        
        if (result.cache_line_utilization < 2.0) {
            result.optimization_suggestions.push_back(
                "Low cache line utilization - consider data structure packing");
        }
        
        if (result.unique_cache_lines_accessed > 1000) {
            result.optimization_suggestions.push_back(
                "High cache line usage - consider working set reduction");
        }
    }
    
    f64 get_current_timestamp() const {
        return std::chrono::duration<f64>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }
};

// Global tracker instance
GlobalMemoryTracker& get_global_tracker();

// Convenience functions for educational tracking
void track_allocation(void* ptr, usize size, const char* category = "unknown");
void track_deallocation(void* ptr);
void track_access(void* ptr, usize size, bool is_write = false, const std::string& context = "unknown");

// Educational analysis functions
std::string generate_memory_report();
CacheAnalysisResult analyze_cache_behavior();
void visualize_memory_layout();

} // namespace memory::tracker
```

## Performance Analysis

### Benchmarking Framework

```cpp
/**
 * @brief Educational Memory Benchmarking Framework
 * 
 * Provides comprehensive benchmarking capabilities for comparing different
 * memory management strategies and understanding their performance characteristics.
 */
class MemoryBenchmarkSuite {
public:
    struct BenchmarkConfig {
        usize entity_count{10000};
        usize iterations{100};
        bool enable_detailed_analysis{true};
        bool enable_cache_simulation{true};
        std::string benchmark_name{"Default_Benchmark"};
    };
    
    struct BenchmarkResult {
        std::string allocator_name;
        f64 allocation_time;
        f64 access_time;
        f64 deallocation_time;
        usize memory_usage;
        f64 cache_hit_ratio;
        f64 memory_efficiency;
        std::vector<std::string> insights;
    };
    
    // Run comprehensive allocator comparison
    std::vector<BenchmarkResult> run_allocator_comparison(const BenchmarkConfig& config) {
        std::vector<BenchmarkResult> results;
        
        // Test standard allocation
        results.push_back(benchmark_standard_allocation(config));
        
        // Test arena allocation
        results.push_back(benchmark_arena_allocation(config));
        
        // Test pool allocation
        results.push_back(benchmark_pool_allocation(config));
        
        // Test hybrid PMR allocation
        results.push_back(benchmark_pmr_allocation(config));
        
        // Generate comparative analysis
        generate_comparative_analysis(results);
        
        return results;
    }
    
private:
    BenchmarkResult benchmark_standard_allocation(const BenchmarkConfig& config) {
        BenchmarkResult result;
        result.allocator_name = "Standard (malloc/free)";
        
        Timer allocation_timer, access_timer, deallocation_timer;
        std::vector<Transform*> allocations;
        allocations.reserve(config.entity_count);
        
        // Allocation benchmark
        memory::tracker::start_analysis("Standard_Allocation");
        allocation_timer.start();
        
        for (usize i = 0; i < config.entity_count; ++i) {
            allocations.push_back(new Transform{Vec2{static_cast<f32>(i), static_cast<f32>(i)}});
        }
        
        result.allocation_time = allocation_timer.elapsed_ms();
        
        // Access benchmark
        access_timer.start();
        
        for (usize iteration = 0; iteration < config.iterations; ++iteration) {
            for (auto* transform : allocations) {
                transform->position.x += 1.0f; // Simulate access
            }
        }
        
        result.access_time = access_timer.elapsed_ms();
        
        // Deallocation benchmark
        deallocation_timer.start();
        
        for (auto* ptr : allocations) {
            delete ptr;
        }
        
        result.deallocation_time = deallocation_timer.elapsed_ms();
        memory::tracker::end_analysis();
        
        // Collect statistics
        auto analysis = memory::tracker::get_latest_analysis();
        result.cache_hit_ratio = analysis.cache_hit_ratio;
        result.memory_usage = config.entity_count * sizeof(Transform);
        result.memory_efficiency = 1.0; // Baseline
        
        // Generate insights
        result.insights.push_back("Baseline performance for comparison");
        if (result.cache_hit_ratio < 0.7) {
            result.insights.push_back("Poor cache locality due to fragmented allocation");
        }
        
        return result;
    }
    
    BenchmarkResult benchmark_arena_allocation(const BenchmarkConfig& config) {
        BenchmarkResult result;
        result.allocator_name = "Arena (linear allocation)";
        
        memory::ArenaAllocator arena{config.entity_count * sizeof(Transform) + 1 * KB};
        Timer allocation_timer, access_timer;
        std::vector<Transform*> allocations;
        allocations.reserve(config.entity_count);
        
        // Allocation benchmark
        memory::tracker::start_analysis("Arena_Allocation");
        allocation_timer.start();
        
        for (usize i = 0; i < config.entity_count; ++i) {
            auto* ptr = arena.allocate<Transform>();
            *ptr = Transform{Vec2{static_cast<f32>(i), static_cast<f32>(i)}};
            allocations.push_back(ptr);
        }
        
        result.allocation_time = allocation_timer.elapsed_ms();
        
        // Access benchmark (should show excellent cache performance)
        access_timer.start();
        
        for (usize iteration = 0; iteration < config.iterations; ++iteration) {
            for (auto* transform : allocations) {
                transform->position.x += 1.0f; // Sequential access
            }
        }
        
        result.access_time = access_timer.elapsed_ms();
        result.deallocation_time = 0.0; // Arena reset is effectively instantaneous
        
        memory::tracker::end_analysis();
        
        // Collect statistics
        auto analysis = memory::tracker::get_latest_analysis();
        result.cache_hit_ratio = analysis.cache_hit_ratio;
        result.memory_usage = arena.used_size();
        result.memory_efficiency = static_cast<f64>(config.entity_count * sizeof(Transform)) / arena.used_size();
        
        // Generate insights
        result.insights.push_back("Sequential allocation provides excellent cache locality");
        result.insights.push_back("Zero fragmentation with linear allocation pattern");
        if (result.cache_hit_ratio > 0.9) {
            result.insights.push_back("Excellent cache performance due to sequential layout");
        }
        
        arena.reset(); // Cleanup
        
        return result;
    }
    
    void generate_comparative_analysis(std::vector<BenchmarkResult>& results) {
        if (results.empty()) return;
        
        // Find best performer in each category
        auto best_allocation = std::min_element(results.begin(), results.end(),
            [](const auto& a, const auto& b) { return a.allocation_time < b.allocation_time; });
        
        auto best_access = std::min_element(results.begin(), results.end(),
            [](const auto& a, const auto& b) { return a.access_time < b.access_time; });
        
        auto best_cache = std::max_element(results.begin(), results.end(),
            [](const auto& a, const auto& b) { return a.cache_hit_ratio < b.cache_hit_ratio; });
        
        // Add comparative insights to each result
        for (auto& result : results) {
            if (&result == &(*best_allocation)) {
                result.insights.push_back("★ Fastest allocation strategy");
            }
            if (&result == &(*best_access)) {
                result.insights.push_back("★ Fastest access performance");
            }
            if (&result == &(*best_cache)) {
                result.insights.push_back("★ Best cache utilization");
            }
            
            // Performance comparison insights
            f64 allocation_vs_best = result.allocation_time / best_allocation->allocation_time;
            if (allocation_vs_best > 2.0) {
                result.insights.push_back("Allocation is " + std::to_string(allocation_vs_best) + "x slower than best");
            }
            
            f64 access_vs_best = result.access_time / best_access->access_time;
            if (access_vs_best > 1.5) {
                result.insights.push_back("Access is " + std::to_string(access_vs_best) + "x slower than best");
            }
        }
    }
};

// Educational benchmark runner
void run_educational_memory_benchmarks() {
    MemoryBenchmarkSuite suite;
    
    MemoryBenchmarkSuite::BenchmarkConfig config{
        .entity_count = 10000,
        .iterations = 100,
        .enable_detailed_analysis = true,
        .enable_cache_simulation = true,
        .benchmark_name = "Educational_Memory_Comparison"
    };
    
    auto results = suite.run_allocator_comparison(config);
    
    // Display results in educational format
    std::cout << "=== Memory Allocator Comparison Results ===\n";
    std::cout << "Test configuration: " << config.entity_count << " entities, " 
              << config.iterations << " iterations\n\n";
    
    for (const auto& result : results) {
        std::cout << "Allocator: " << result.allocator_name << "\n";
        std::cout << "  Allocation time: " << result.allocation_time << " ms\n";
        std::cout << "  Access time: " << result.access_time << " ms\n";
        std::cout << "  Memory usage: " << (result.memory_usage / KB) << " KB\n";
        std::cout << "  Cache hit ratio: " << (result.cache_hit_ratio * 100.0) << "%\n";
        std::cout << "  Memory efficiency: " << (result.memory_efficiency * 100.0) << "%\n";
        
        std::cout << "  Insights:\n";
        for (const auto& insight : result.insights) {
            std::cout << "    • " << insight << "\n";
        }
        std::cout << "\n";
    }
}
```

## Educational Features

### Interactive Memory Experiments

**Experiment 1: Allocation Strategy Comparison**
```cpp
// Interactive experiment for understanding allocator trade-offs
class AllocationStrategyExperiment {
public:
    void run_interactive_experiment() {
        std::cout << "=== Interactive Allocation Strategy Experiment ===\n";
        std::cout << "This experiment compares different allocation strategies.\n";
        std::cout << "You can modify parameters and see immediate results.\n\n";
        
        // Get user configuration
        ExperimentConfig config = get_user_configuration();
        
        // Run experiment with user settings
        auto results = run_allocation_experiment(config);
        
        // Display results with educational insights
        display_experiment_results(results, config);
        
        // Provide optimization recommendations
        provide_optimization_recommendations(results);
    }
    
private:
    struct ExperimentConfig {
        usize entity_count;
        AllocationStrategy strategy;
        AccessPattern pattern;
        bool enable_tracking;
    };
    
    ExperimentConfig get_user_configuration() {
        ExperimentConfig config;
        
        std::cout << "Enter number of entities (100-100000): ";
        std::cin >> config.entity_count;
        
        std::cout << "Select allocation strategy:\n";
        std::cout << "  1. Standard (malloc/free)\n";
        std::cout << "  2. Arena (linear allocation)\n"; 
        std::cout << "  3. Pool (fixed-size blocks)\n";
        std::cout << "Enter choice (1-3): ";
        
        i32 strategy_choice;
        std::cin >> strategy_choice;
        config.strategy = static_cast<AllocationStrategy>(strategy_choice - 1);
        
        std::cout << "Select access pattern:\n";
        std::cout << "  1. Sequential (cache-friendly)\n";
        std::cout << "  2. Random (cache-hostile)\n";
        std::cout << "  3. Strided (moderate cache usage)\n";
        std::cout << "Enter choice (1-3): ";
        
        i32 pattern_choice;
        std::cin >> pattern_choice;
        config.pattern = static_cast<AccessPattern>(pattern_choice - 1);
        
        return config;
    }
};
```

**Experiment 2: Cache Behavior Visualization**
```cpp
// Visual representation of cache behavior
class CacheBehaviorVisualizer {
public:
    void visualize_access_pattern(const std::vector<MemoryAccessEvent>& events) {
        std::cout << "=== Cache Access Pattern Visualization ===\n";
        
        // Group accesses by cache line
        std::map<usize, std::vector<f64>> cache_line_accesses;
        for (const auto& event : events) {
            cache_line_accesses[event.cache_line_index].push_back(event.timestamp);
        }
        
        // Visualize cache line usage over time
        std::cout << "Cache Line Usage (each character = cache line):\n";
        
        const usize max_lines_to_show = 64;
        usize lines_shown = 0;
        
        for (const auto& [line_index, timestamps] : cache_line_accesses) {
            if (lines_shown >= max_lines_to_show) break;
            
            char intensity = calculate_intensity_char(timestamps.size());
            std::cout << intensity;
            
            lines_shown++;
        }
        
        std::cout << "\n\nIntensity Legend:\n";
        std::cout << "  ░ = 1-2 accesses    ▒ = 3-5 accesses\n";
        std::cout << "  ▓ = 6-10 accesses   █ = 11+ accesses\n";
        
        // Cache performance analysis
        analyze_cache_performance(cache_line_accesses);
    }
    
private:
    char calculate_intensity_char(usize access_count) const {
        if (access_count <= 2) return '░';
        if (access_count <= 5) return '▒';
        if (access_count <= 10) return '▓';
        return '█';
    }
    
    void analyze_cache_performance(const std::map<usize, std::vector<f64>>& cache_line_accesses) {
        std::cout << "\nCache Performance Analysis:\n";
        
        usize total_lines = cache_line_accesses.size();
        usize total_accesses = 0;
        usize high_usage_lines = 0;
        
        for (const auto& [line, accesses] : cache_line_accesses) {
            total_accesses += accesses.size();
            if (accesses.size() > 5) {
                high_usage_lines++;
            }
        }
        
        f64 average_accesses_per_line = static_cast<f64>(total_accesses) / total_lines;
        f64 high_usage_ratio = static_cast<f64>(high_usage_lines) / total_lines;
        
        std::cout << "  Total cache lines used: " << total_lines << "\n";
        std::cout << "  Average accesses per line: " << average_accesses_per_line << "\n";
        std::cout << "  High-usage lines: " << (high_usage_ratio * 100.0) << "%\n";
        
        // Educational insights
        if (average_accesses_per_line > 3.0) {
            std::cout << "  ✓ Good cache line utilization\n";
        } else {
            std::cout << "  ⚠ Poor cache line utilization - consider data packing\n";
        }
        
        if (high_usage_ratio > 0.3) {
            std::cout << "  ✓ Good temporal locality\n";
        } else {
            std::cout << "  ⚠ Poor temporal locality - consider access pattern optimization\n";
        }
    }
};
```

### Real-time Memory Monitoring

```cpp
// Real-time memory monitoring for educational purposes
class MemoryMonitor {
private:
    std::vector<MemorySnapshot> history_;
    std::chrono::steady_clock::time_point start_time_;
    
public:
    struct MemorySnapshot {
        f64 timestamp;
        usize total_allocated;
        usize arena_used;
        usize pool_used;
        f64 cache_hit_ratio;
        usize active_entities;
    };
    
    void start_monitoring() {
        start_time_ = std::chrono::steady_clock::now();
        LOG_INFO("Memory monitoring started");
    }
    
    void take_snapshot(const ecs::Registry& registry) {
        auto now = std::chrono::steady_clock::now();
        f64 timestamp = std::chrono::duration<f64>(now - start_time_).count();
        
        auto stats = registry.get_memory_statistics();
        
        MemorySnapshot snapshot{
            .timestamp = timestamp,
            .total_allocated = registry.memory_usage(),
            .arena_used = stats.archetype_arena_used,
            .pool_used = stats.entity_pool_used,
            .cache_hit_ratio = stats.cache_hit_ratio,
            .active_entities = stats.active_entities
        };
        
        history_.push_back(snapshot);
        
        // Limit history size
        if (history_.size() > 1000) {
            history_.erase(history_.begin(), history_.begin() + 500);
        }
    }
    
    void display_memory_graph() const {
        if (history_.size() < 2) return;
        
        std::cout << "=== Memory Usage Over Time ===\n";
        
        // Find min/max for scaling
        auto [min_it, max_it] = std::minmax_element(history_.begin(), history_.end(),
            [](const auto& a, const auto& b) { return a.total_allocated < b.total_allocated; });
        
        usize min_memory = min_it->total_allocated;
        usize max_memory = max_it->total_allocated;
        usize range = max_memory - min_memory;
        
        if (range == 0) range = 1; // Avoid division by zero
        
        // Display graph
        const usize graph_height = 20;
        for (i32 row = graph_height - 1; row >= 0; --row) {
            usize threshold = min_memory + (range * row) / graph_height;
            
            std::cout << std::setw(8) << (threshold / KB) << "KB |";
            
            for (const auto& snapshot : history_) {
                if (snapshot.total_allocated >= threshold) {
                    std::cout << "█";
                } else {
                    std::cout << " ";
                }
            }
            std::cout << "\n";
        }
        
        // Time axis
        std::cout << "        +";
        for (usize i = 0; i < history_.size(); ++i) {
            std::cout << "-";
        }
        std::cout << "\n";
        std::cout << "         0s" << std::string(history_.size() - 10, ' ') 
                  << history_.back().timestamp << "s\n";
    }
};
```

## Advanced Patterns

### Pattern 1: Hierarchical Memory Management

```cpp
// Multi-level memory management for different data lifetimes
class HierarchicalMemoryManager {
    // Different allocators for different lifetimes
    memory::ArenaAllocator frame_arena_{1 * MB};      // Per-frame data
    memory::ArenaAllocator level_arena_{16 * MB};     // Per-level data
    memory::PoolAllocator entity_pool_{64, 10000};    // Entity management
    std::pmr::synchronized_pool_resource global_pool_; // Long-lived data
    
public:
    // Educational: Allocation routing based on lifetime
    template<typename T>
    T* allocate_for_lifetime(DataLifetime lifetime) {
        switch (lifetime) {
            case DataLifetime::Frame:
                LOG_DEBUG("Allocating {} for frame lifetime", typeid(T).name());
                return frame_arena_.allocate<T>();
                
            case DataLifetime::Level:
                LOG_DEBUG("Allocating {} for level lifetime", typeid(T).name());
                return level_arena_.allocate<T>();
                
            case DataLifetime::Entity:
                LOG_DEBUG("Allocating {} for entity lifetime", typeid(T).name());
                return entity_pool_.allocate<T>();
                
            case DataLifetime::Game:
                LOG_DEBUG("Allocating {} for game lifetime", typeid(T).name());
                return static_cast<T*>(global_pool_.allocate(sizeof(T), alignof(T)));
        }
        
        return nullptr;
    }
    
    // Educational: Cleanup by lifetime
    void cleanup_frame_data() {
        usize frame_memory_used = frame_arena_.used_size();
        frame_arena_.reset();
        LOG_INFO("Frame cleanup: freed {} KB", frame_memory_used / KB);
    }
    
    void cleanup_level_data() {
        usize level_memory_used = level_arena_.used_size();
        level_arena_.reset();
        LOG_INFO("Level cleanup: freed {} KB", level_memory_used / KB);
    }
    
    // Educational: Memory usage breakdown
    void log_memory_breakdown() const {
        std::cout << "=== Hierarchical Memory Usage ===\n";
        std::cout << "Frame arena: " << (frame_arena_.used_size() / KB) << " KB\n";
        std::cout << "Level arena: " << (level_arena_.used_size() / KB) << " KB\n";
        std::cout << "Entity pool: " << (entity_pool_.allocated_count() * entity_pool_.block_size() / KB) << " KB\n";
        std::cout << "Global pool: " << "Unknown KB (PMR doesn't provide usage stats)\n";
    }
};
```

### Pattern 2: Memory-Mapped Component Storage

```cpp
// Advanced: Memory-mapped storage for huge datasets
class MemoryMappedComponentStorage {
    void* mapped_memory_;
    usize file_size_;
    std::string filename_;
    
public:
    template<Component T>
    MemoryMappedComponentStorage(const std::string& filename, usize max_components)
        : filename_(filename) {
        
        file_size_ = max_components * sizeof(T);
        
        // Platform-specific memory mapping (educational implementation)
        mapped_memory_ = map_memory_file(filename, file_size_);
        
        if (!mapped_memory_) {
            throw std::runtime_error("Failed to map memory file: " + filename);
        }
        
        LOG_INFO("Memory-mapped component storage:");
        LOG_INFO("  File: {}", filename);
        LOG_INFO("  Size: {} MB", file_size_ / MB);
        LOG_INFO("  Component type: {}", typeid(T).name());
        LOG_INFO("  Max components: {}", max_components);
    }
    
    ~MemoryMappedComponentStorage() {
        if (mapped_memory_) {
            unmap_memory_file(mapped_memory_, file_size_);
            LOG_INFO("Unmapped memory file: {}", filename_);
        }
    }
    
    template<Component T>
    T* get_component_array() {
        return static_cast<T*>(mapped_memory_);
    }
    
    // Educational: Demonstrate zero-copy operations
    template<Component T>
    void demonstrate_zero_copy_benefits() {
        std::cout << "=== Zero-Copy Benefits Demonstration ===\n";
        
        T* components = get_component_array<T>();
        usize component_count = file_size_ / sizeof(T);
        
        // Process components without copying data
        {
            ScopeTimer timer("Zero-Copy Processing");
            for (usize i = 0; i < component_count; ++i) {
                // Direct manipulation of memory-mapped data
                if constexpr (std::is_same_v<T, Transform>) {
                    components[i].position.x += 1.0f;
                }
            }
        }
        
        std::cout << "Processed " << component_count << " components with zero-copy access\n";
        std::cout << "Benefits: No memory allocation, no data copying, direct file I/O\n";
    }
};
```

### Pattern 3: Adaptive Memory Management

```cpp
// Adaptive allocator that changes strategy based on usage patterns
class AdaptiveAllocator : public IAllocator {
private:
    std::unique_ptr<memory::ArenaAllocator> arena_;
    std::unique_ptr<memory::PoolAllocator> pool_;
    IAllocator* fallback_;
    
    // Usage pattern analysis
    struct UsagePattern {
        f64 average_allocation_size;
        f64 allocation_frequency;
        f64 deallocation_frequency;
        f64 cache_hit_ratio;
        AccessPattern dominant_pattern;
    };
    
    mutable UsagePattern current_pattern_;
    mutable std::vector<AllocationEvent> recent_events_;
    
public:
    void* allocate(usize size, usize alignment = 0) override {
        // Analyze current usage pattern
        update_usage_pattern(size, true);
        
        // Choose optimal allocator based on pattern
        IAllocator* chosen_allocator = choose_optimal_allocator(size, alignment);
        
        // Perform allocation
        void* ptr = chosen_allocator->allocate(size, alignment);
        
        // Track allocation for pattern analysis
        track_allocation_event(ptr, size, chosen_allocator->name());
        
        return ptr;
    }
    
private:
    IAllocator* choose_optimal_allocator(usize size, usize alignment) {
        // Educational: Decision logic with explanations
        if (current_pattern_.dominant_pattern == AccessPattern::Sequential &&
            current_pattern_.deallocation_frequency < 0.1) {
            LOG_DEBUG("Choosing arena allocator: sequential pattern with low deallocation frequency");
            return arena_.get();
        }
        
        if (size <= pool_->block_size() && current_pattern_.deallocation_frequency > 0.5) {
            LOG_DEBUG("Choosing pool allocator: frequent deallocation pattern");
            return pool_.get();
        }
        
        LOG_DEBUG("Choosing fallback allocator: pattern doesn't match specialized allocators");
        return fallback_;
    }
    
    void update_usage_pattern(usize allocation_size, bool is_allocation) {
        // Update running statistics for pattern analysis
        // This is complex educational code that analyzes allocation patterns
        // and adapts allocation strategy accordingly
    }
};
```

---

**ECScope Memory Management: Where allocation strategies become visible, cache behavior becomes understandable, and performance optimization becomes achievable.**

*Every allocation decision has consequences. ECScope makes those consequences visible, measurable, and educational.*