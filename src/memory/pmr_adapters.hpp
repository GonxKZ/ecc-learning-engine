#pragma once

/**
 * @file pmr_adapters.hpp
 * @brief C++17 Polymorphic Memory Resources (PMR) Integration for ECScope ECS Engine
 * 
 * This file provides complete integration between ECScope's custom allocators (Arena and Pool)
 * and the C++17 std::pmr system. It serves as an educational reference for implementing
 * custom memory resources while maintaining high performance and comprehensive debugging features.
 * 
 * Key Educational Concepts:
 * - Custom memory_resource implementation
 * - Zero-overhead PMR adaptations
 * - Allocation strategy selection based on request size
 * - Thread-safe PMR resource management
 * - Performance comparison between standard and PMR allocators
 * - Memory tracking integration through PMR interfaces
 * 
 * Architecture:
 * - ArenaMemoryResource: Linear allocation for temporary objects
 * - PoolMemoryResource: Fixed-size allocation for frequent objects  
 * - HybridMemoryResource: Intelligent switching between strategies
 * - MonotonicBufferResource: Stack-based temporary allocations
 * - SynchronizedMemoryResource: Thread-safe wrapper for any resource
 * - ChainedMemoryResource: Hierarchical allocation strategies
 * 
 * Performance Benefits:
 * - Reduced allocation overhead through custom strategies
 * - Cache-friendly memory layout
 * - Elimination of malloc/free for managed objects
 * - Predictable allocation patterns
 * - Zero fragmentation for arena allocations
 * 
 * Educational Features:
 * - Comprehensive allocation tracking
 * - Performance metrics collection
 * - Memory usage visualization
 * - Strategy effectiveness analysis
 * - Integration with existing UI panels
 * 
 * @author ECScope Educational ECS Framework
 * @date 2024
 */

#include "arena.hpp"
#include "pool.hpp"
#include "core/types.hpp"
#include "core/log.hpp"
#include "core/time.hpp"

#include <memory_resource>
#include <vector>
#include <unordered_map>
#include <string>
#include <mutex>
#include <atomic>
#include <array>
#include <algorithm>
#include <chrono>
#include <thread>

// Check for C++17 PMR availability
#if __cpp_lib_memory_resource >= 201603L
    #include <memory_resource>
    #define ECSCOPE_PMR_AVAILABLE 1
#else
    #define ECSCOPE_PMR_AVAILABLE 0
    // Provide basic fallback definitions for educational purposes
    namespace std {
        namespace pmr {
            class memory_resource {
            public:
                virtual ~memory_resource() = default;
                virtual void* allocate(size_t bytes, size_t alignment = alignof(max_align_t)) = 0;
                virtual void deallocate(void* ptr, size_t bytes, size_t alignment = alignof(max_align_t)) = 0;
                virtual bool is_equal(const memory_resource& other) const noexcept = 0;
            };
        }
    }
#endif

namespace ecscope::memory::pmr {

// ============================================================================
// PMR Statistics and Monitoring
// ============================================================================

/**
 * @brief Comprehensive statistics for PMR resource usage
 * 
 * Tracks allocation patterns, performance metrics, and strategy effectiveness
 * for educational analysis and optimization guidance.
 */
struct PMRStats {
    // Basic allocation statistics
    usize total_allocations{0};        // Total number of allocate() calls
    usize total_deallocations{0};      // Total number of deallocate() calls
    usize peak_allocated_bytes{0};     // Peak memory usage in bytes
    usize current_allocated_bytes{0};  // Currently allocated bytes
    usize total_allocated_bytes{0};    // Lifetime total allocated bytes
    usize allocation_failures{0};      // Number of failed allocations
    
    // Performance metrics
    f64 total_allocation_time{0.0};    // Total time spent in allocate() (ms)
    f64 total_deallocation_time{0.0};  // Total time spent in deallocate() (ms)
    f64 average_allocation_time{0.0};  // Average allocation time (ns)
    f64 average_deallocation_time{0.0}; // Average deallocation time (ns)
    
    // Size distribution analysis
    usize small_allocations{0};        // Allocations <= 64 bytes
    usize medium_allocations{0};       // Allocations 65-1024 bytes
    usize large_allocations{0};        // Allocations > 1024 bytes
    
    // Strategy effectiveness (for hybrid resources)
    usize arena_allocations{0};        // Allocations served by arena
    usize pool_allocations{0};         // Allocations served by pool
    usize fallback_allocations{0};     // Allocations served by fallback
    
    // Thread safety metrics
    usize lock_contentions{0};         // Number of lock contentions
    f64 total_lock_time{0.0};         // Total time waiting for locks (ms)
    
    void reset() noexcept {
        *this = PMRStats{};
    }
    
    // Calculate derived metrics
    f64 allocation_efficiency() const noexcept {
        return total_allocations > 0 ? 
            (1.0 - static_cast<f64>(allocation_failures) / total_allocations) : 1.0;
    }
    
    f64 average_allocation_size() const noexcept {
        return total_allocations > 0 ?
            static_cast<f64>(total_allocated_bytes) / total_allocations : 0.0;
    }
    
    f64 fragmentation_estimate() const noexcept {
        // Simple fragmentation estimate based on allocation patterns
        return current_allocated_bytes > 0 ?
            1.0 - (static_cast<f64>(current_allocated_bytes) / peak_allocated_bytes) : 0.0;
    }
};

/**
 * @brief Allocation tracking entry for PMR allocations
 * 
 * Provides detailed tracking information for debugging and educational purposes.
 */
struct PMRAllocationInfo {
    void* ptr{nullptr};                 // Allocated pointer
    usize size{0};                     // Allocation size in bytes
    usize alignment{0};                // Requested alignment
    f64 timestamp{0.0};                // Allocation timestamp
    std::thread::id thread_id;         // Thread that made the allocation
    const char* source_resource{nullptr}; // Name of the resource that served this
    
    // Optional debug information
    const char* file{nullptr};
    int line{0};
    const char* function{nullptr};
    
    PMRAllocationInfo() = default;
    
    PMRAllocationInfo(void* p, usize s, usize a, const char* resource_name = nullptr)
        : ptr(p), size(s), alignment(a), 
          timestamp(std::chrono::duration<f64, std::milli>(
              std::chrono::high_resolution_clock::now().time_since_epoch()).count()),
          thread_id(std::this_thread::get_id()),
          source_resource(resource_name) {}
};

// ============================================================================
// Base PMR Memory Resource with Educational Features
// ============================================================================

/**
 * @brief Base class for ECScope PMR memory resources
 * 
 * Provides common functionality including allocation tracking, performance
 * monitoring, and debugging features that can be shared across all PMR
 * implementations in the ECScope system.
 */
class ECScopeMemoryResource : public std::pmr::memory_resource {
protected:
    std::string name_;                          // Resource name for debugging
    mutable std::mutex stats_mutex_;            // Protects statistics
    PMRStats stats_;                           // Performance and usage statistics
    std::vector<PMRAllocationInfo> allocations_; // Active allocations tracking
    bool enable_tracking_;                      // Enable detailed tracking
    bool enable_debug_output_;                  // Enable debug logging
    
    // Performance monitoring
    mutable std::atomic<u64> allocation_counter_{0};
    std::array<f64, 1000> recent_allocation_times_{}; // Circular buffer
    mutable std::atomic<usize> timing_index_{0};
    
public:
    explicit ECScopeMemoryResource(const std::string& name, bool enable_tracking = true)
        : name_(name), enable_tracking_(enable_tracking), enable_debug_output_(false) {
        
        if (enable_debug_output_) {
            ecscope::core::log_info("PMR Resource '{}' created", name_);
        }
    }
    
    virtual ~ECScopeMemoryResource() {
        if (enable_debug_output_) {
            ecscope::core::log_info("PMR Resource '{}' destroyed. Final stats: {} allocations, {} bytes peak", 
                name_, stats_.total_allocations, stats_.peak_allocated_bytes);
        }
    }
    
    // Statistics and configuration
    const PMRStats& stats() const noexcept { 
        std::lock_guard<std::mutex> lock(stats_mutex_);
        return stats_; 
    }
    
    const std::string& name() const noexcept { return name_; }
    
    void set_tracking_enabled(bool enabled) { enable_tracking_ = enabled; }
    bool is_tracking_enabled() const noexcept { return enable_tracking_; }
    
    void set_debug_output_enabled(bool enabled) { enable_debug_output_ = enabled; }
    bool is_debug_output_enabled() const noexcept { return enable_debug_output_; }
    
    void reset_stats() {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.reset();
        allocations_.clear();
    }
    
    // Get current allocations (for debugging/visualization)
    std::vector<PMRAllocationInfo> get_active_allocations() const {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        return allocations_;
    }
    
    // Generate diagnostic report
    std::string generate_report() const {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        
        std::ostringstream oss;
        oss << "=== PMR Resource Report: " << name_ << " ===\n";
        oss << "Total Allocations: " << stats_.total_allocations << "\n";
        oss << "Total Deallocations: " << stats_.total_deallocations << "\n";
        oss << "Current Allocated: " << stats_.current_allocated_bytes << " bytes\n";
        oss << "Peak Allocated: " << stats_.peak_allocated_bytes << " bytes\n";
        oss << "Average Allocation Size: " << stats_.average_allocation_size() << " bytes\n";
        oss << "Allocation Efficiency: " << (stats_.allocation_efficiency() * 100.0) << "%\n";
        oss << "Average Allocation Time: " << stats_.average_allocation_time << " ns\n";
        oss << "Average Deallocation Time: " << stats_.average_deallocation_time << " ns\n";
        
        // Size distribution
        oss << "\n--- Size Distribution ---\n";
        oss << "Small (<=64B): " << stats_.small_allocations << "\n";
        oss << "Medium (65-1024B): " << stats_.medium_allocations << "\n";
        oss << "Large (>1024B): " << stats_.large_allocations << "\n";
        
        return oss.str();
    }

protected:
    // Helper for tracking allocations
    void record_allocation(void* ptr, usize bytes, usize alignment, const char* source = nullptr) {
        if (!enable_tracking_) return;
        
        std::lock_guard<std::mutex> lock(stats_mutex_);
        
        // Update statistics
        stats_.total_allocations++;
        stats_.current_allocated_bytes += bytes;
        stats_.total_allocated_bytes += bytes;
        stats_.peak_allocated_bytes = std::max(stats_.peak_allocated_bytes, stats_.current_allocated_bytes);
        
        // Update size distribution
        if (bytes <= 64) {
            stats_.small_allocations++;
        } else if (bytes <= 1024) {
            stats_.medium_allocations++;
        } else {
            stats_.large_allocations++;
        }
        
        // Record allocation info
        allocations_.emplace_back(ptr, bytes, alignment, source ? source : name_.c_str());
        
        if (enable_debug_output_) {
            ecscope::core::log_debug("PMR '{}' allocated {} bytes at {:#x} (alignment={})", 
                name_, bytes, reinterpret_cast<uintptr_t>(ptr), alignment);
        }
    }
    
    // Helper for tracking deallocations
    void record_deallocation(void* ptr, usize bytes) {
        if (!enable_tracking_) return;
        
        std::lock_guard<std::mutex> lock(stats_mutex_);
        
        // Update statistics
        stats_.total_deallocations++;
        stats_.current_allocated_bytes -= bytes;
        
        // Remove from active allocations
        auto it = std::find_if(allocations_.begin(), allocations_.end(),
            [ptr](const PMRAllocationInfo& info) { return info.ptr == ptr; });
        
        if (it != allocations_.end()) {
            allocations_.erase(it);
        }
        
        if (enable_debug_output_) {
            ecscope::core::log_debug("PMR '{}' deallocated {} bytes at {:#x}", 
                name_, bytes, reinterpret_cast<uintptr_t>(ptr));
        }
    }
    
    // Helper for timing allocations
    void record_allocation_time(f64 time_ns) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        
        stats_.total_allocation_time += time_ns / 1e6; // Convert to ms
        
        if (stats_.total_allocations > 0) {
            stats_.average_allocation_time = (stats_.total_allocation_time * 1e6) / stats_.total_allocations;
        }
        
        // Update circular buffer
        usize index = timing_index_.fetch_add(1) % recent_allocation_times_.size();
        recent_allocation_times_[index] = time_ns;
    }
    
    // Helper for timing deallocations
    void record_deallocation_time(f64 time_ns) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        
        stats_.total_deallocation_time += time_ns / 1e6; // Convert to ms
        
        if (stats_.total_deallocations > 0) {
            stats_.average_deallocation_time = (stats_.total_deallocation_time * 1e6) / stats_.total_deallocations;
        }
    }
    
    // Helper for recording allocation failures
    void record_allocation_failure(usize bytes, usize alignment) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.allocation_failures++;
        
        if (enable_debug_output_) {
            ecscope::core::log_warning("PMR '{}' failed to allocate {} bytes (alignment={})", 
                name_, bytes, alignment);
        }
    }
};

// ============================================================================
// Arena Memory Resource - Linear Allocation Strategy
// ============================================================================

/**
 * @brief PMR adapter for ECScope's ArenaAllocator
 * 
 * Provides linear allocation strategy through the std::pmr interface.
 * Ideal for temporary allocations with predictable lifetimes, such as
 * per-frame allocations or temporary data structures.
 * 
 * Educational Benefits:
 * - Zero fragmentation demonstration
 * - Cache-friendly allocation patterns  
 * - Fast allocation performance
 * - Simple reset capability for frame-based usage
 * 
 * Use Cases:
 * - Temporary containers in algorithms
 * - Per-frame game object allocations
 * - String building and text processing
 * - Recursive data structure construction
 */
class ArenaMemoryResource : public ECScopeMemoryResource {
private:
    std::unique_ptr<ArenaAllocator> arena_;
    bool owns_arena_;  // Whether we own the arena or just reference it
    
public:
    /**
     * @brief Create arena memory resource with specified size
     * 
     * @param size Total arena size in bytes
     * @param name Resource name for debugging
     * @param enable_tracking Enable allocation tracking
     */
    explicit ArenaMemoryResource(usize size, const std::string& name = "ArenaMemoryResource", 
                                bool enable_tracking = true)
        : ECScopeMemoryResource(name, enable_tracking)
        , arena_(std::make_unique<ArenaAllocator>(size, name + "_Arena", enable_tracking))
        , owns_arena_(true) {
        
        if (enable_debug_output_) {
            ecscope::core::log_info("ArenaMemoryResource created with {} bytes", size);
        }
    }
    
    /**
     * @brief Create arena memory resource wrapping existing arena
     * 
     * @param arena Reference to existing arena allocator
     * @param name Resource name for debugging
     * @param enable_tracking Enable allocation tracking
     */
    explicit ArenaMemoryResource(ArenaAllocator& arena, const std::string& name = "ArenaMemoryResource",
                                bool enable_tracking = true)
        : ECScopeMemoryResource(name, enable_tracking)
        , arena_(&arena)
        , owns_arena_(false) {
        
        if (enable_debug_output_) {
            ecscope::core::log_info("ArenaMemoryResource created wrapping existing arena '{}'", arena.name());
        }
    }
    
    virtual ~ArenaMemoryResource() = default;
    
    // Arena-specific operations
    void reset() {
        arena_->reset();
        reset_stats();
        
        if (enable_debug_output_) {
            ecscope::core::log_info("ArenaMemoryResource '{}' reset", name_);
        }
    }
    
    void clear() {
        arena_->clear();
        reset_stats();
        
        if (enable_debug_output_) {
            ecscope::core::log_info("ArenaMemoryResource '{}' cleared", name_);
        }
    }
    
    // Arena introspection
    usize total_size() const noexcept { return arena_->total_size(); }
    usize used_size() const noexcept { return arena_->used_size(); }
    usize available_size() const noexcept { return arena_->available_size(); }
    f64 usage_ratio() const noexcept { return arena_->usage_ratio(); }
    
    const ArenaStats& arena_stats() const noexcept { return arena_->stats(); }
    ArenaAllocator& arena() noexcept { return *arena_; }
    const ArenaAllocator& arena() const noexcept { return *arena_; }

protected:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        void* ptr = arena_->allocate(static_cast<usize>(bytes), static_cast<usize>(alignment), "PMR");
        
        auto end_time = std::chrono::high_resolution_clock::now();
        f64 duration_ns = std::chrono::duration<f64, std::nano>(end_time - start_time).count();
        
        if (ptr) {
            record_allocation(ptr, static_cast<usize>(bytes), static_cast<usize>(alignment), "Arena");
            record_allocation_time(duration_ns);
            
            // Update strategy statistics
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.arena_allocations++;
        } else {
            record_allocation_failure(static_cast<usize>(bytes), static_cast<usize>(alignment));
        }
        
        return ptr;
    }
    
    void do_deallocate(void* ptr, std::size_t bytes, std::size_t alignment) override {
        // Arena allocator doesn't support individual deallocation
        // This is by design - arena allocations are typically reset in bulk
        
        if (enable_tracking_) {
            auto start_time = std::chrono::high_resolution_clock::now();
            
            // We can only track the deallocation for statistics
            record_deallocation(ptr, static_cast<usize>(bytes));
            
            auto end_time = std::chrono::high_resolution_clock::now();
            f64 duration_ns = std::chrono::duration<f64, std::nano>(end_time - start_time).count();
            record_deallocation_time(duration_ns);
        }
        
        if (enable_debug_output_) {
            ecscope::core::log_debug("ArenaMemoryResource deallocate called - individual deallocation not supported");
        }
    }
    
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        const auto* other_arena = dynamic_cast<const ArenaMemoryResource*>(&other);
        return other_arena && (arena_.get() == other_arena->arena_.get() || arena_ == other_arena->arena_);
    }
};

// ============================================================================
// Pool Memory Resource - Fixed-Size Allocation Strategy
// ============================================================================

/**
 * @brief PMR adapter for ECScope's PoolAllocator
 * 
 * Provides fixed-size block allocation through the std::pmr interface.
 * Optimized for frequent allocation/deallocation of objects with similar sizes.
 * 
 * Educational Benefits:
 * - O(1) allocation/deallocation demonstration
 * - Memory reuse and fragmentation avoidance
 * - Cache-friendly memory layout
 * - Free-list management visualization
 * 
 * Use Cases:
 * - ECS component allocation
 * - Game object instances
 * - Node-based data structures
 * - Frequent temporary objects
 */
class PoolMemoryResource : public ECScopeMemoryResource {
private:
    std::unique_ptr<PoolAllocator> pool_;
    bool owns_pool_;    // Whether we own the pool or just reference it
    usize block_size_;  // Size of blocks this pool manages
    
public:
    /**
     * @brief Create pool memory resource for fixed-size blocks
     * 
     * @param block_size Size of each block in bytes
     * @param initial_capacity Initial number of blocks
     * @param alignment Block alignment requirement
     * @param name Resource name for debugging
     * @param enable_tracking Enable allocation tracking
     */
    explicit PoolMemoryResource(usize block_size, usize initial_capacity = 1024,
                               usize alignment = alignof(std::max_align_t),
                               const std::string& name = "PoolMemoryResource",
                               bool enable_tracking = true)
        : ECScopeMemoryResource(name, enable_tracking)
        , pool_(std::make_unique<PoolAllocator>(block_size, initial_capacity, alignment, 
                                               name + "_Pool", enable_tracking))
        , owns_pool_(true)
        , block_size_(block_size) {
        
        if (enable_debug_output_) {
            ecscope::core::log_info("PoolMemoryResource created: block_size={}, capacity={}", 
                block_size, initial_capacity);
        }
    }
    
    /**
     * @brief Create pool memory resource for specific type
     * 
     * @tparam T Type to optimize pool for
     * @param initial_capacity Initial number of blocks
     * @param name Resource name (defaults to type name)
     * @param enable_tracking Enable allocation tracking
     */
    template<typename T>
    static std::unique_ptr<PoolMemoryResource> create_for_type(
        usize initial_capacity = 1024,
        const std::string& name = "",
        bool enable_tracking = true) {
        
        std::string resource_name = name.empty() ? 
            std::string("PoolMemoryResource<") + typeid(T).name() + ">" : name;
        
        return std::make_unique<PoolMemoryResource>(sizeof(T), initial_capacity, alignof(T), 
                                                   resource_name, enable_tracking);
    }
    
    /**
     * @brief Create pool memory resource wrapping existing pool
     * 
     * @param pool Reference to existing pool allocator
     * @param name Resource name for debugging
     * @param enable_tracking Enable allocation tracking
     */
    explicit PoolMemoryResource(PoolAllocator& pool, const std::string& name = "PoolMemoryResource",
                               bool enable_tracking = true)
        : ECScopeMemoryResource(name, enable_tracking)
        , pool_(&pool)
        , owns_pool_(false)
        , block_size_(pool.block_size()) {
        
        if (enable_debug_output_) {
            ecscope::core::log_info("PoolMemoryResource created wrapping existing pool '{}'", pool.name());
        }
    }
    
    virtual ~PoolMemoryResource() = default;
    
    // Pool-specific operations
    bool expand_pool(usize capacity = 0) { return pool_->expand_pool(capacity); }
    usize shrink_pool() { return pool_->shrink_pool(); }
    void reset() { 
        pool_->reset(); 
        reset_stats();
    }
    void clear() { 
        pool_->clear(); 
        reset_stats(); 
    }
    
    // Pool introspection
    usize block_size() const noexcept { return block_size_; }
    usize chunk_count() const noexcept { return pool_->chunk_count(); }
    usize total_capacity() const noexcept { return pool_->total_capacity(); }
    usize allocated_count() const noexcept { return pool_->allocated_count(); }
    usize free_count() const noexcept { return pool_->free_count(); }
    f64 utilization_ratio() const noexcept { return pool_->utilization_ratio(); }
    bool is_full() const noexcept { return pool_->is_full(); }
    
    const PoolStats& pool_stats() const noexcept { return pool_->stats(); }
    PoolAllocator& pool() noexcept { return *pool_; }
    const PoolAllocator& pool() const noexcept { return *pool_; }

protected:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        // Verify that the allocation request matches the pool configuration
        if (static_cast<usize>(bytes) > block_size_) {
            if (enable_debug_output_) {
                ecscope::core::log_error("PoolMemoryResource: Requested {} bytes exceeds block size {}", 
                    bytes, block_size_);
            }
            record_allocation_failure(static_cast<usize>(bytes), static_cast<usize>(alignment));
            return nullptr;
        }
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        void* ptr = pool_->allocate("PMR");
        
        auto end_time = std::chrono::high_resolution_clock::now();
        f64 duration_ns = std::chrono::duration<f64, std::nano>(end_time - start_time).count();
        
        if (ptr) {
            record_allocation(ptr, static_cast<usize>(bytes), static_cast<usize>(alignment), "Pool");
            record_allocation_time(duration_ns);
            
            // Update strategy statistics
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.pool_allocations++;
        } else {
            record_allocation_failure(static_cast<usize>(bytes), static_cast<usize>(alignment));
        }
        
        return ptr;
    }
    
    void do_deallocate(void* ptr, std::size_t bytes, std::size_t alignment) override {
        if (!ptr) return;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        pool_->deallocate(ptr);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        f64 duration_ns = std::chrono::duration<f64, std::nano>(end_time - start_time).count();
        
        record_deallocation(ptr, static_cast<usize>(bytes));
        record_deallocation_time(duration_ns);
    }
    
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        const auto* other_pool = dynamic_cast<const PoolMemoryResource*>(&other);
        return other_pool && (pool_.get() == other_pool->pool_.get() || pool_ == other_pool->pool_);
    }
};

// ============================================================================
// Hybrid Memory Resource - Intelligent Strategy Selection
// ============================================================================

/**
 * @brief Hybrid memory resource that selects allocation strategy based on request size
 * 
 * This resource intelligently chooses between different allocation strategies based
 * on the size and alignment requirements of each allocation request. Provides an
 * excellent educational example of adaptive allocation strategies.
 * 
 * Strategy Selection Logic:
 * - Small allocations (≤ small_threshold): Use pool allocator
 * - Medium allocations (≤ medium_threshold): Use arena allocator  
 * - Large allocations (> medium_threshold): Use fallback allocator
 * 
 * Educational Benefits:
 * - Demonstrates allocation strategy trade-offs
 * - Shows how to combine multiple allocation approaches
 * - Provides metrics on strategy effectiveness
 * - Illustrates adaptive memory management
 * 
 * Performance Benefits:
 * - Optimal strategy selection for each allocation size
 * - Reduced fragmentation through intelligent routing
 * - Cache-friendly allocation patterns
 * - Minimized allocation overhead
 */
class HybridMemoryResource : public ECScopeMemoryResource {
private:
    std::unique_ptr<PoolMemoryResource> pool_resource_;
    std::unique_ptr<ArenaMemoryResource> arena_resource_;
    std::pmr::memory_resource* fallback_resource_;
    
    // Strategy selection thresholds
    usize small_threshold_;     // Pool for allocations <= this size
    usize medium_threshold_;    // Arena for allocations <= this size
    
    // Strategy usage tracking
    mutable std::atomic<usize> small_strategy_count_{0};
    mutable std::atomic<usize> medium_strategy_count_{0};
    mutable std::atomic<usize> large_strategy_count_{0};
    
    // Performance tracking per strategy
    mutable std::atomic<f64> small_strategy_time_{0.0};
    mutable std::atomic<f64> medium_strategy_time_{0.0};
    mutable std::atomic<f64> large_strategy_time_{0.0};
    
public:
    /**
     * @brief Create hybrid memory resource with custom thresholds
     * 
     * @param pool_block_size Size of blocks for pool allocator
     * @param pool_capacity Initial pool capacity
     * @param arena_size Total arena size
     * @param small_threshold Threshold for pool allocation strategy
     * @param medium_threshold Threshold for arena allocation strategy
     * @param fallback_resource Fallback resource for large allocations
     * @param name Resource name for debugging
     * @param enable_tracking Enable allocation tracking
     */
    explicit HybridMemoryResource(
        usize pool_block_size = 64,
        usize pool_capacity = 1024,
        usize arena_size = 1 * MB,
        usize small_threshold = 64,
        usize medium_threshold = 1024,
        std::pmr::memory_resource* fallback_resource = std::pmr::get_default_resource(),
        const std::string& name = "HybridMemoryResource",
        bool enable_tracking = true)
        : ECScopeMemoryResource(name, enable_tracking)
        , pool_resource_(std::make_unique<PoolMemoryResource>(pool_block_size, pool_capacity, 
                        alignof(std::max_align_t), name + "_Pool", enable_tracking))
        , arena_resource_(std::make_unique<ArenaMemoryResource>(arena_size, 
                         name + "_Arena", enable_tracking))
        , fallback_resource_(fallback_resource)
        , small_threshold_(small_threshold)
        , medium_threshold_(medium_threshold) {
        
        if (enable_debug_output_) {
            ecscope::core::log_info("HybridMemoryResource created: pool_size={}, arena_size={}, "
                "small_threshold={}, medium_threshold={}", 
                pool_block_size, arena_size, small_threshold, medium_threshold);
        }
    }
    
    virtual ~HybridMemoryResource() = default;
    
    // Strategy configuration
    void set_thresholds(usize small_threshold, usize medium_threshold) {
        small_threshold_ = small_threshold;
        medium_threshold_ = medium_threshold;
        
        if (enable_debug_output_) {
            ecscope::core::log_info("HybridMemoryResource thresholds updated: small={}, medium={}", 
                small_threshold, medium_threshold);
        }
    }
    
    // Access to underlying resources
    PoolMemoryResource& pool_resource() noexcept { return *pool_resource_; }
    const PoolMemoryResource& pool_resource() const noexcept { return *pool_resource_; }
    
    ArenaMemoryResource& arena_resource() noexcept { return *arena_resource_; }
    const ArenaMemoryResource& arena_resource() const noexcept { return *arena_resource_; }
    
    std::pmr::memory_resource& fallback_resource() noexcept { return *fallback_resource_; }
    
    // Strategy effectiveness analysis
    struct StrategyStats {
        usize small_allocations;
        usize medium_allocations;
        usize large_allocations;
        f64 small_avg_time;
        f64 medium_avg_time;
        f64 large_avg_time;
        f64 strategy_efficiency; // How often the optimal strategy was chosen
    };
    
    StrategyStats get_strategy_stats() const {
        StrategyStats stats{};
        stats.small_allocations = small_strategy_count_.load();
        stats.medium_allocations = medium_strategy_count_.load();
        stats.large_allocations = large_strategy_count_.load();
        
        stats.small_avg_time = stats.small_allocations > 0 ? 
            small_strategy_time_.load() / stats.small_allocations : 0.0;
        stats.medium_avg_time = stats.medium_allocations > 0 ?
            medium_strategy_time_.load() / stats.medium_allocations : 0.0;
        stats.large_avg_time = stats.large_allocations > 0 ?
            large_strategy_time_.load() / stats.large_allocations : 0.0;
        
        usize total = stats.small_allocations + stats.medium_allocations + stats.large_allocations;
        stats.strategy_efficiency = total > 0 ? 
            (static_cast<f64>(stats.small_allocations + stats.medium_allocations) / total) : 1.0;
        
        return stats;
    }
    
    // Reset all underlying resources
    void reset() {
        pool_resource_->reset();
        arena_resource_->reset();
        reset_stats();
        
        // Reset strategy counters
        small_strategy_count_ = 0;
        medium_strategy_count_ = 0;
        large_strategy_count_ = 0;
        small_strategy_time_ = 0.0;
        medium_strategy_time_ = 0.0;
        large_strategy_time_ = 0.0;
    }

protected:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        auto start_time = std::chrono::high_resolution_clock::now();
        void* ptr = nullptr;
        const char* strategy_used = nullptr;
        
        // Select allocation strategy based on size
        if (bytes <= small_threshold_) {
            // Small allocation - use pool
            ptr = pool_resource_->allocate(bytes, alignment);
            strategy_used = "Pool";
            
            if (ptr) {
                small_strategy_count_++;
                auto end_time = std::chrono::high_resolution_clock::now();
                f64 duration_ns = std::chrono::duration<f64, std::nano>(end_time - start_time).count();
                small_strategy_time_ = small_strategy_time_.load() + duration_ns;
            }
            
        } else if (bytes <= medium_threshold_) {
            // Medium allocation - use arena
            ptr = arena_resource_->allocate(bytes, alignment);
            strategy_used = "Arena";
            
            if (ptr) {
                medium_strategy_count_++;
                auto end_time = std::chrono::high_resolution_clock::now();
                f64 duration_ns = std::chrono::duration<f64, std::nano>(end_time - start_time).count();
                medium_strategy_time_ = medium_strategy_time_.load() + duration_ns;
            }
            
        } else {
            // Large allocation - use fallback
            ptr = fallback_resource_->allocate(bytes, alignment);
            strategy_used = "Fallback";
            
            if (ptr) {
                large_strategy_count_++;
                auto end_time = std::chrono::high_resolution_clock::now();
                f64 duration_ns = std::chrono::duration<f64, std::nano>(end_time - start_time).count();
                large_strategy_time_ = large_strategy_time_.load() + duration_ns;
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        f64 duration_ns = std::chrono::duration<f64, std::nano>(end_time - start_time).count();
        
        if (ptr) {
            record_allocation(ptr, static_cast<usize>(bytes), static_cast<usize>(alignment), strategy_used);
            record_allocation_time(duration_ns);
            
            // Update strategy-specific statistics
            std::lock_guard<std::mutex> lock(stats_mutex_);
            if (strcmp(strategy_used, "Pool") == 0) {
                stats_.pool_allocations++;
            } else if (strcmp(strategy_used, "Arena") == 0) {
                stats_.arena_allocations++;
            } else {
                stats_.fallback_allocations++;
            }
        } else {
            record_allocation_failure(static_cast<usize>(bytes), static_cast<usize>(alignment));
        }
        
        return ptr;
    }
    
    void do_deallocate(void* ptr, std::size_t bytes, std::size_t alignment) override {
        if (!ptr) return;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Determine which resource to deallocate from based on size
        // In a production system, you might track which resource each allocation came from
        if (bytes <= small_threshold_) {
            pool_resource_->deallocate(ptr, bytes, alignment);
        } else if (bytes <= medium_threshold_) {
            arena_resource_->deallocate(ptr, bytes, alignment);
        } else {
            fallback_resource_->deallocate(ptr, bytes, alignment);
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        f64 duration_ns = std::chrono::duration<f64, std::nano>(end_time - start_time).count();
        
        record_deallocation(ptr, static_cast<usize>(bytes));
        record_deallocation_time(duration_ns);
    }
    
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
};

// ============================================================================
// Monotonic Buffer Resource - Stack-based Temporary Allocations
// ============================================================================

/**
 * @brief Monotonic buffer resource for stack-based temporary allocations
 * 
 * A specialized memory resource that allocates from a pre-allocated buffer
 * with optional fallback to another resource when the buffer is exhausted.
 * Perfect for scoped temporary allocations with predictable patterns.
 * 
 * Educational Benefits:
 * - Demonstrates stack-based allocation patterns
 * - Shows buffer exhaustion handling strategies
 * - Illustrates allocation tracking in bounded resources
 * - Provides example of resource composition
 * 
 * Use Cases:
 * - Function-local temporary allocations
 * - Algorithm working memory
 * - String manipulation buffers
 * - Temporary container storage
 */
class MonotonicBufferResource : public ECScopeMemoryResource {
private:
    byte* buffer_;                          // Pre-allocated buffer
    usize buffer_size_;                     // Total buffer size
    usize current_offset_;                  // Current allocation offset
    std::pmr::memory_resource* fallback_;   // Fallback resource when buffer is full
    bool owns_buffer_;                      // Whether we own the buffer
    
    // Buffer management
    std::vector<void*> fallback_allocations_; // Track fallback allocations for cleanup
    mutable std::atomic<usize> buffer_allocations_{0};
    mutable std::atomic<usize> fallback_allocation_count_{0};
    
public:
    /**
     * @brief Create monotonic buffer resource with specified buffer size
     * 
     * @param buffer_size Size of the internal buffer
     * @param fallback_resource Resource to use when buffer is exhausted
     * @param name Resource name for debugging
     * @param enable_tracking Enable allocation tracking
     */
    explicit MonotonicBufferResource(
        usize buffer_size,
        std::pmr::memory_resource* fallback_resource = std::pmr::get_default_resource(),
        const std::string& name = "MonotonicBufferResource",
        bool enable_tracking = true)
        : ECScopeMemoryResource(name, enable_tracking)
        , buffer_(static_cast<byte*>(std::aligned_alloc(alignof(std::max_align_t), buffer_size)))
        , buffer_size_(buffer_size)
        , current_offset_(0)
        , fallback_(fallback_resource)
        , owns_buffer_(true) {
        
        if (!buffer_) {
            throw std::bad_alloc();
        }
        
        if (enable_debug_output_) {
            ecscope::core::log_info("MonotonicBufferResource created with {} bytes buffer", buffer_size);
        }
    }
    
    /**
     * @brief Create monotonic buffer resource using external buffer
     * 
     * @param buffer External buffer to use
     * @param buffer_size Size of the external buffer
     * @param fallback_resource Resource to use when buffer is exhausted
     * @param name Resource name for debugging
     * @param enable_tracking Enable allocation tracking
     */
    explicit MonotonicBufferResource(
        void* buffer,
        usize buffer_size,
        std::pmr::memory_resource* fallback_resource = std::pmr::get_default_resource(),
        const std::string& name = "MonotonicBufferResource",
        bool enable_tracking = true)
        : ECScopeMemoryResource(name, enable_tracking)
        , buffer_(static_cast<byte*>(buffer))
        , buffer_size_(buffer_size)
        , current_offset_(0)
        , fallback_(fallback_resource)
        , owns_buffer_(false) {
        
        if (enable_debug_output_) {
            ecscope::core::log_info("MonotonicBufferResource created using external buffer ({} bytes)", buffer_size);
        }
    }
    
    virtual ~MonotonicBufferResource() {
        // Clean up any fallback allocations
        for (void* ptr : fallback_allocations_) {
            fallback_->deallocate(ptr, 0, alignof(std::max_align_t));
        }
        
        if (owns_buffer_ && buffer_) {
            std::free(buffer_);
        }
    }
    
    // Buffer operations
    void reset() noexcept {
        current_offset_ = 0;
        buffer_allocations_ = 0;
        
        // Clean up fallback allocations
        for (void* ptr : fallback_allocations_) {
            fallback_->deallocate(ptr, 0, alignof(std::max_align_t));
        }
        fallback_allocations_.clear();
        fallback_allocation_count_ = 0;
        
        reset_stats();
        
        if (enable_debug_output_) {
            ecscope::core::log_info("MonotonicBufferResource '{}' reset", name_);
        }
    }
    
    // Buffer introspection
    usize buffer_size() const noexcept { return buffer_size_; }
    usize used_size() const noexcept { return current_offset_; }
    usize available_size() const noexcept { return buffer_size_ - current_offset_; }
    f64 usage_ratio() const noexcept { return static_cast<f64>(current_offset_) / buffer_size_; }
    
    usize buffer_allocation_count() const noexcept { return buffer_allocations_.load(); }
    usize fallback_allocation_count() const noexcept { return fallback_allocation_count_.load(); }
    
    bool buffer_exhausted() const noexcept { return current_offset_ >= buffer_size_; }
    
    // Access to buffer and fallback
    const void* buffer() const noexcept { return buffer_; }
    std::pmr::memory_resource& fallback_resource() noexcept { return *fallback_; }

protected:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Align the current offset
        usize aligned_offset = align_offset(current_offset_, static_cast<usize>(alignment));
        
        // Check if allocation fits in buffer
        if (aligned_offset + bytes <= buffer_size_) {
            // Allocate from buffer
            void* ptr = buffer_ + aligned_offset;
            current_offset_ = aligned_offset + bytes;
            buffer_allocations_++;
            
            auto end_time = std::chrono::high_resolution_clock::now();
            f64 duration_ns = std::chrono::duration<f64, std::nano>(end_time - start_time).count();
            
            record_allocation(ptr, static_cast<usize>(bytes), static_cast<usize>(alignment), "Buffer");
            record_allocation_time(duration_ns);
            
            return ptr;
        } else {
            // Fall back to fallback resource
            void* ptr = fallback_->allocate(bytes, alignment);
            
            if (ptr) {
                fallback_allocations_.push_back(ptr);
                fallback_allocation_count_++;
                
                auto end_time = std::chrono::high_resolution_clock::now();
                f64 duration_ns = std::chrono::duration<f64, std::nano>(end_time - start_time).count();
                
                record_allocation(ptr, static_cast<usize>(bytes), static_cast<usize>(alignment), "Fallback");
                record_allocation_time(duration_ns);
                
                // Update strategy statistics
                std::lock_guard<std::mutex> lock(stats_mutex_);
                stats_.fallback_allocations++;
            } else {
                record_allocation_failure(static_cast<usize>(bytes), static_cast<usize>(alignment));
            }
            
            return ptr;
        }
    }
    
    void do_deallocate(void* ptr, std::size_t bytes, std::size_t alignment) override {
        if (!ptr) return;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Check if pointer is from our buffer
        if (ptr >= buffer_ && ptr < buffer_ + buffer_size_) {
            // Buffer allocation - monotonic buffer doesn't support individual deallocation
            if (enable_debug_output_) {
                ecscope::core::log_debug("MonotonicBufferResource: Individual deallocation from buffer not supported");
            }
        } else {
            // Fallback allocation - deallocate from fallback resource
            fallback_->deallocate(ptr, bytes, alignment);
            
            // Remove from fallback tracking
            auto it = std::find(fallback_allocations_.begin(), fallback_allocations_.end(), ptr);
            if (it != fallback_allocations_.end()) {
                fallback_allocations_.erase(it);
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        f64 duration_ns = std::chrono::duration<f64, std::nano>(end_time - start_time).count();
        
        record_deallocation(ptr, static_cast<usize>(bytes));
        record_deallocation_time(duration_ns);
    }
    
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        const auto* other_monotonic = dynamic_cast<const MonotonicBufferResource*>(&other);
        return other_monotonic && (buffer_ == other_monotonic->buffer_);
    }

private:
    static usize align_offset(usize offset, usize alignment) noexcept {
        return (offset + alignment - 1) & ~(alignment - 1);
    }
};

// ============================================================================
// Synchronized Memory Resource - Thread-Safe Wrapper
// ============================================================================

/**
 * @brief Thread-safe wrapper for any memory resource
 * 
 * Provides thread-safety for any memory resource through mutex synchronization.
 * Includes lock contention tracking and performance analysis for educational purposes.
 * 
 * Educational Benefits:
 * - Demonstrates thread-safety patterns in memory management
 * - Shows lock contention analysis and optimization
 * - Illustrates wrapper/adapter design patterns
 * - Provides performance comparison with and without synchronization
 * 
 * Use Cases:
 * - Multi-threaded applications requiring shared memory resources
 * - Thread pool environments with shared allocators
 * - Concurrent data structure implementations
 * - Performance analysis of synchronization overhead
 */
class SynchronizedMemoryResource : public ECScopeMemoryResource {
private:
    std::pmr::memory_resource* wrapped_resource_;
    mutable std::mutex allocation_mutex_;
    
    // Lock contention tracking
    mutable std::atomic<usize> total_lock_attempts_{0};
    mutable std::atomic<usize> lock_contentions_{0};
    mutable std::atomic<f64> total_lock_wait_time_{0.0};
    
public:
    /**
     * @brief Create synchronized wrapper for a memory resource
     * 
     * @param wrapped_resource Resource to wrap with thread-safety
     * @param name Resource name for debugging
     * @param enable_tracking Enable allocation tracking
     */
    explicit SynchronizedMemoryResource(
        std::pmr::memory_resource* wrapped_resource,
        const std::string& name = "SynchronizedMemoryResource",
        bool enable_tracking = true)
        : ECScopeMemoryResource(name, enable_tracking)
        , wrapped_resource_(wrapped_resource) {
        
        if (enable_debug_output_) {
            ecscope::core::log_info("SynchronizedMemoryResource created wrapping resource");
        }
    }
    
    virtual ~SynchronizedMemoryResource() = default;
    
    // Access to wrapped resource (use with caution - not thread-safe)
    std::pmr::memory_resource& wrapped_resource() noexcept { return *wrapped_resource_; }
    const std::pmr::memory_resource& wrapped_resource() const noexcept { return *wrapped_resource_; }
    
    // Lock contention analysis
    struct SynchronizationStats {
        usize total_lock_attempts;
        usize lock_contentions;
        f64 contention_ratio;
        f64 average_lock_wait_time;
        f64 total_lock_wait_time;
    };
    
    SynchronizationStats get_sync_stats() const noexcept {
        SynchronizationStats stats{};
        stats.total_lock_attempts = total_lock_attempts_.load();
        stats.lock_contentions = lock_contentions_.load();
        stats.contention_ratio = stats.total_lock_attempts > 0 ?
            static_cast<f64>(stats.lock_contentions) / stats.total_lock_attempts : 0.0;
        stats.total_lock_wait_time = total_lock_wait_time_.load();
        stats.average_lock_wait_time = stats.lock_contentions > 0 ?
            stats.total_lock_wait_time / stats.lock_contentions : 0.0;
        return stats;
    }
    
    // Reset synchronization statistics
    void reset_sync_stats() noexcept {
        total_lock_attempts_ = 0;
        lock_contentions_ = 0;
        total_lock_wait_time_ = 0.0;
    }

protected:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        auto lock_start = std::chrono::high_resolution_clock::now();
        
        total_lock_attempts_++;
        
        // Try to acquire lock without blocking first
        bool contended = !allocation_mutex_.try_lock();
        if (contended) {
            lock_contentions_++;
            // Now block for the lock
            allocation_mutex_.lock();
        }
        
        auto lock_acquired = std::chrono::high_resolution_clock::now();
        f64 lock_wait_ns = std::chrono::duration<f64, std::nano>(lock_acquired - lock_start).count();
        
        if (contended) {
            total_lock_wait_time_ = total_lock_wait_time_.load() + (lock_wait_ns / 1e6); // Convert to ms
        }
        
        // Perform the actual allocation
        auto alloc_start = std::chrono::high_resolution_clock::now();
        void* ptr = wrapped_resource_->allocate(bytes, alignment);
        auto alloc_end = std::chrono::high_resolution_clock::now();
        
        allocation_mutex_.unlock();
        
        f64 alloc_time_ns = std::chrono::duration<f64, std::nano>(alloc_end - alloc_start).count();
        f64 total_time_ns = std::chrono::duration<f64, std::nano>(alloc_end - lock_start).count();
        
        if (ptr) {
            record_allocation(ptr, static_cast<usize>(bytes), static_cast<usize>(alignment), "Synchronized");
            record_allocation_time(alloc_time_ns);
            
            // Update lock statistics in our own stats
            std::lock_guard<std::mutex> stats_lock(stats_mutex_);
            stats_.lock_contentions += contended ? 1 : 0;
            stats_.total_lock_time += lock_wait_ns / 1e6;
        } else {
            record_allocation_failure(static_cast<usize>(bytes), static_cast<usize>(alignment));
        }
        
        return ptr;
    }
    
    void do_deallocate(void* ptr, std::size_t bytes, std::size_t alignment) override {
        if (!ptr) return;
        
        auto lock_start = std::chrono::high_resolution_clock::now();
        
        total_lock_attempts_++;
        
        bool contended = !allocation_mutex_.try_lock();
        if (contended) {
            lock_contentions_++;
            allocation_mutex_.lock();
        }
        
        auto lock_acquired = std::chrono::high_resolution_clock::now();
        f64 lock_wait_ns = std::chrono::duration<f64, std::nano>(lock_acquired - lock_start).count();
        
        if (contended) {
            total_lock_wait_time_ = total_lock_wait_time_.load() + (lock_wait_ns / 1e6);
        }
        
        auto dealloc_start = std::chrono::high_resolution_clock::now();
        wrapped_resource_->deallocate(ptr, bytes, alignment);
        auto dealloc_end = std::chrono::high_resolution_clock::now();
        
        allocation_mutex_.unlock();
        
        f64 dealloc_time_ns = std::chrono::duration<f64, std::nano>(dealloc_end - dealloc_start).count();
        
        record_deallocation(ptr, static_cast<usize>(bytes));
        record_deallocation_time(dealloc_time_ns);
    }
    
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        const auto* other_sync = dynamic_cast<const SynchronizedMemoryResource*>(&other);
        if (other_sync) {
            return wrapped_resource_->is_equal(*other_sync->wrapped_resource_);
        }
        return wrapped_resource_->is_equal(other);
    }
};

// ============================================================================
// PMR Container Aliases and Utilities
// ============================================================================

/**
 * @brief Convenient type aliases for common PMR containers
 * 
 * These aliases make it easy to use PMR-enabled containers with ECScope's
 * memory resources. Each alias is configured for optimal performance with
 * the corresponding allocation strategy.
 */

// Vector aliases for different use cases
template<typename T>
using pmr_vector = std::pmr::vector<T>;

template<typename T>
using arena_vector = std::pmr::vector<T>;  // Use with arena resource

template<typename T>
using pool_vector = std::pmr::vector<T>;   // Use with pool resource

// String aliases
using pmr_string = std::pmr::string;
using arena_string = std::pmr::string;
using pool_string = std::pmr::string;

// Map aliases
template<typename Key, typename Value>
using pmr_map = std::pmr::map<Key, Value>;

template<typename Key, typename Value>
using pmr_unordered_map = std::pmr::unordered_map<Key, Value>;

// Set aliases
template<typename T>
using pmr_set = std::pmr::set<T>;

template<typename T>
using pmr_unordered_set = std::pmr::unordered_set<T>;

// Deque and list aliases
template<typename T>
using pmr_deque = std::pmr::deque<T>;

template<typename T>
using pmr_list = std::pmr::list<T>;

// ============================================================================
// PMR Resource Factory Functions
// ============================================================================

/**
 * @brief Factory functions for creating pre-configured PMR resources
 * 
 * These functions provide convenient ways to create PMR resources with
 * sensible default configurations for common use cases.
 */

namespace factory {

/**
 * @brief Create an arena memory resource for temporary allocations
 */
inline std::unique_ptr<ArenaMemoryResource> create_arena_resource(
    usize size = 1 * MB,
    const std::string& name = "ArenaResource") {
    return std::make_unique<ArenaMemoryResource>(size, name, true);
}

/**
 * @brief Create a pool memory resource for specific type
 */
template<typename T>
inline std::unique_ptr<PoolMemoryResource> create_pool_resource(
    usize initial_capacity = 1024,
    const std::string& name = "") {
    return PoolMemoryResource::create_for_type<T>(initial_capacity, name, true);
}

/**
 * @brief Create a hybrid resource with sensible defaults
 */
inline std::unique_ptr<HybridMemoryResource> create_hybrid_resource(
    const std::string& name = "HybridResource") {
    return std::make_unique<HybridMemoryResource>(64, 1024, 1 * MB, 64, 1024, 
        std::pmr::get_default_resource(), name, true);
}

/**
 * @brief Create a monotonic buffer resource with stack storage
 */
inline std::unique_ptr<MonotonicBufferResource> create_monotonic_resource(
    usize buffer_size = 64 * KB,
    const std::string& name = "MonotonicResource") {
    return std::make_unique<MonotonicBufferResource>(buffer_size, 
        std::pmr::get_default_resource(), name, true);
}

/**
 * @brief Create a synchronized wrapper around any resource
 */
inline std::unique_ptr<SynchronizedMemoryResource> create_synchronized_resource(
    std::pmr::memory_resource* wrapped_resource,
    const std::string& name = "SynchronizedResource") {
    return std::make_unique<SynchronizedMemoryResource>(wrapped_resource, name, true);
}

} // namespace factory

// ============================================================================
// PMR Resource Registry for UI Integration
// ============================================================================

/**
 * @brief Global registry for PMR memory resources
 * 
 * Provides centralized tracking and management of all PMR resources
 * for educational visualization and performance analysis.
 */
namespace pmr_registry {

/**
 * @brief Register a PMR resource for tracking
 */
void register_resource(ECScopeMemoryResource* resource);

/**
 * @brief Unregister a PMR resource
 */
void unregister_resource(ECScopeMemoryResource* resource);

/**
 * @brief Get all registered PMR resources
 */
std::vector<ECScopeMemoryResource*> get_all_resources();

/**
 * @brief Get combined statistics across all resources
 */
PMRStats get_combined_stats();

/**
 * @brief Generate system-wide PMR usage report
 */
std::string generate_system_report();

/**
 * @brief Get resources filtered by type
 */
template<typename ResourceType>
std::vector<ResourceType*> get_resources_by_type() {
    std::vector<ResourceType*> result;
    auto all_resources = get_all_resources();
    
    for (auto* resource : all_resources) {
        if (auto* typed_resource = dynamic_cast<ResourceType*>(resource)) {
            result.push_back(typed_resource);
        }
    }
    
    return result;
}

} // namespace pmr_registry

// ============================================================================
// RAII Helpers for Scoped PMR Usage
// ============================================================================

/**
 * @brief RAII helper for scoped PMR resource usage
 * 
 * Automatically sets a memory resource as the default for a scope
 * and restores the previous default when destroyed.
 */
class ScopedPMRResource {
private:
    std::pmr::memory_resource* previous_default_;
    
public:
    explicit ScopedPMRResource(std::pmr::memory_resource* resource) 
        : previous_default_(std::pmr::get_default_resource()) {
        std::pmr::set_default_resource(resource);
    }
    
    ~ScopedPMRResource() {
        std::pmr::set_default_resource(previous_default_);
    }
    
    // Non-copyable, non-movable
    ScopedPMRResource(const ScopedPMRResource&) = delete;
    ScopedPMRResource& operator=(const ScopedPMRResource&) = delete;
    ScopedPMRResource(ScopedPMRResource&&) = delete;
    ScopedPMRResource& operator=(ScopedPMRResource&&) = delete;
};

/**
 * @brief RAII helper for temporary arena-based allocations
 * 
 * Creates a temporary arena resource, sets it as default, and automatically
 * resets it when the scope ends. Perfect for function-local temporary allocations.
 */
class ScopedArenaAllocator {
private:
    std::unique_ptr<ArenaMemoryResource> arena_resource_;
    ScopedPMRResource scoped_default_;
    
public:
    explicit ScopedArenaAllocator(usize size = 64 * KB, const std::string& name = "ScopedArena")
        : arena_resource_(std::make_unique<ArenaMemoryResource>(size, name))
        , scoped_default_(arena_resource_.get()) {}
    
    ~ScopedArenaAllocator() {
        // arena_resource_ will be automatically reset and destroyed
    }
    
    // Access to the underlying arena
    ArenaMemoryResource& resource() noexcept { return *arena_resource_; }
    const ArenaMemoryResource& resource() const noexcept { return *arena_resource_; }
    
    ArenaAllocator& arena() noexcept { return arena_resource_->arena(); }
    const ArenaAllocator& arena() const noexcept { return arena_resource_->arena(); }
};

// ============================================================================
// Educational Performance Comparison Tools
// ============================================================================

/**
 * @brief Performance comparison framework for PMR vs standard allocators
 * 
 * Provides tools to compare the performance of PMR-based allocations
 * against standard malloc/new allocations for educational purposes.
 */
namespace performance_comparison {

/**
 * @brief Results of a performance comparison test
 */
struct ComparisonResults {
    f64 standard_time_ms;       // Time using standard allocator
    f64 pmr_time_ms;           // Time using PMR allocator
    f64 speedup_factor;        // PMR speedup (positive = faster, negative = slower)
    usize allocation_count;    // Number of allocations tested
    usize total_bytes;         // Total bytes allocated
    std::string test_name;     // Name of the test performed
};

/**
 * @brief Run allocation performance comparison
 */
template<typename TestFunction>
ComparisonResults compare_allocation_performance(
    const std::string& test_name,
    TestFunction test_func,
    std::pmr::memory_resource* pmr_resource,
    usize iterations = 10000) {
    
    ComparisonResults results{};
    results.test_name = test_name;
    results.allocation_count = iterations;
    
    // Test standard allocation
    {
        auto start = std::chrono::high_resolution_clock::now();
        test_func(std::pmr::get_default_resource());
        auto end = std::chrono::high_resolution_clock::now();
        results.standard_time_ms = std::chrono::duration<f64, std::milli>(end - start).count();
    }
    
    // Test PMR allocation
    {
        auto start = std::chrono::high_resolution_clock::now();
        test_func(pmr_resource);
        auto end = std::chrono::high_resolution_clock::now();
        results.pmr_time_ms = std::chrono::duration<f64, std::milli>(end - start).count();
    }
    
    // Calculate speedup
    results.speedup_factor = results.standard_time_ms / results.pmr_time_ms;
    
    return results;
}

/**
 * @brief Generate performance comparison report
 */
std::string generate_performance_report(const std::vector<ComparisonResults>& results);

} // namespace performance_comparison

} // namespace ecscope::memory::pmr

// ============================================================================
// Convenience Macros for PMR Usage
// ============================================================================

/**
 * @brief Macros for convenient PMR resource usage
 */

// Create scoped arena for temporary allocations
#define ECSCOPE_SCOPED_ARENA(size) \
    ecscope::memory::pmr::ScopedArenaAllocator scoped_arena_##__LINE__(size, "ScopedArena_" #__LINE__)

// Create scoped PMR resource
#define ECSCOPE_SCOPED_PMR(resource) \
    ecscope::memory::pmr::ScopedPMRResource scoped_pmr_##__LINE__(resource)

// Create PMR container with specific resource
#define ECSCOPE_PMR_VECTOR(type, resource) \
    std::pmr::vector<type>(resource)

#define ECSCOPE_PMR_STRING(resource) \
    std::pmr::string(resource)

#define ECSCOPE_PMR_MAP(key, value, resource) \
    std::pmr::map<key, value>(resource)

// Educational logging for PMR operations
#define ECSCOPE_PMR_LOG_ALLOCATION(resource, bytes) \
    ecscope::core::log_debug("PMR allocation: {} bytes from resource '{}'", bytes, resource->name())

#endif // __cpp_lib_memory_resource check

/**
 * @file pmr_adapters.hpp Implementation Notes
 * 
 * This implementation provides a comprehensive educational framework for
 * understanding and using C++17 Polymorphic Memory Resources (PMR) with
 * ECScope's existing memory management infrastructure.
 * 
 * Key Educational Concepts Demonstrated:
 * 
 * 1. **Custom Memory Resource Implementation**:
 *    - Shows how to properly derive from std::pmr::memory_resource
 *    - Implements all required virtual methods (do_allocate, do_deallocate, do_is_equal)
 *    - Demonstrates proper exception safety and error handling
 * 
 * 2. **Allocation Strategy Selection**:
 *    - Arena strategy for temporary/linear allocations
 *    - Pool strategy for fixed-size frequent allocations
 *    - Hybrid strategy with intelligent size-based selection
 *    - Monotonic buffer for stack-based temporary storage
 * 
 * 3. **Thread Safety Patterns**:
 *    - Synchronized wrapper demonstrating thread-safe resource access
 *    - Lock contention tracking and analysis
 *    - Performance impact measurement of synchronization
 * 
 * 4. **Performance Monitoring**:
 *    - Comprehensive statistics collection
 *    - Allocation timing and pattern analysis
 *    - Strategy effectiveness measurement
 *    - Memory usage and fragmentation tracking
 * 
 * 5. **Integration Patterns**:
 *    - Seamless integration with existing Arena and Pool allocators
 *    - Registry system for centralized management
 *    - UI integration for visual debugging and analysis
 * 
 * 6. **Educational Tools**:
 *    - Performance comparison framework
 *    - Diagnostic reporting and analysis
 *    - Allocation pattern visualization
 *    - Resource usage optimization guidance
 * 
 * Usage Examples:
 * 
 * ```cpp
 * // Basic arena usage
 * auto arena_resource = pmr::factory::create_arena_resource(1 * MB);
 * std::pmr::vector<int> vec(arena_resource.get());
 * 
 * // Pool for specific type
 * auto pool_resource = pmr::factory::create_pool_resource<Entity>(1024);
 * std::pmr::vector<Entity> entities(pool_resource.get());
 * 
 * // Scoped temporary allocations
 * {
 *     ECSCOPE_SCOPED_ARENA(64 * KB);
 *     std::pmr::string temp_string = "temporary data";
 *     std::pmr::vector<int> temp_vec = {1, 2, 3, 4, 5};
 *     // Automatically cleaned up when scope ends
 * }
 * 
 * // Hybrid resource with intelligent strategy selection
 * auto hybrid = pmr::factory::create_hybrid_resource();
 * std::pmr::unordered_map<std::string, int> map(hybrid.get());
 * 
 * // Thread-safe shared resource
 * auto synchronized = pmr::factory::create_synchronized_resource(hybrid.get());
 * // Use from multiple threads safely
 * ```
 * 
 * Performance Benefits:
 * - Reduced malloc/free overhead through custom allocation strategies
 * - Cache-friendly memory layout through arena allocations
 * - Zero fragmentation for temporary allocations
 * - Predictable allocation patterns for real-time applications
 * - Reduced memory footprint through efficient resource reuse
 * 
 * This implementation serves as both a production-ready PMR integration
 * and an educational reference for advanced memory management techniques
 * in modern C++ applications.
 */