#pragma once

/**
 * @file fiber.hpp
 * @brief High-Performance Fiber System for ECScope Job System
 * 
 * This file implements a production-grade fiber system with:
 * - Lightweight stackful coroutines with custom stack management
 * - Efficient context switching using platform-specific optimizations
 * - Fiber pools for memory reuse and reduced allocation overhead
 * - Cooperative multitasking with yield points
 * - Cross-platform support (Windows, Linux, macOS)
 * 
 * Key Features:
 * - Sub-microsecond context switching
 * - Configurable stack sizes for different fiber types
 * - Stack overflow detection and protection
 * - NUMA-aware stack allocation
 * - Integration with work-stealing job system
 * - Memory-efficient fiber recycling
 * 
 * Performance Benefits:
 * - 10-100x faster task switching compared to OS threads
 * - Reduced memory overhead (KB vs MB per execution context)
 * - Better cache locality through cooperative scheduling
 * - Eliminates OS thread scheduling overhead
 * 
 * @author ECScope Engine - Fiber System
 * @date 2025
 */

#include "core/types.hpp"
#include "lockfree_structures.hpp"
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <chrono>

// Platform-specific includes for context switching
#ifdef _WIN32
    #include <windows.h>
    #include <winnt.h>
#elif defined(__linux__) || defined(__APPLE__)
    #include <ucontext.h>
    #include <sys/mman.h>
    #include <unistd.h>
#endif

namespace ecscope::jobs {

//=============================================================================
// Forward Declarations
//=============================================================================

class Fiber;
class FiberPool;
class FiberScheduler;

//=============================================================================
// Fiber Configuration and Types
//=============================================================================

/**
 * @brief Fiber execution state
 */
enum class FiberState : u8 {
    Created = 0,     // Fiber created but not started
    Ready = 1,       // Ready to run
    Running = 2,     // Currently executing
    Suspended = 3,   // Suspended waiting for something
    Completed = 4,   // Execution completed
    Error = 5        // Execution failed
};

/**
 * @brief Fiber priority levels
 */
enum class FiberPriority : u8 {
    Critical = 0,    // Must run immediately
    High = 1,        // High priority
    Normal = 2,      // Default priority
    Low = 3,         // Low priority
    Background = 4   // Background processing
};

/**
 * @brief Fiber stack configuration
 */
struct FiberStackConfig {
    static constexpr usize DEFAULT_STACK_SIZE = 64 * 1024;      // 64KB
    static constexpr usize SMALL_STACK_SIZE = 16 * 1024;        // 16KB
    static constexpr usize LARGE_STACK_SIZE = 256 * 1024;       // 256KB
    static constexpr usize HUGE_STACK_SIZE = 1024 * 1024;       // 1MB
    
    static constexpr usize STACK_ALIGNMENT = 16;                // 16-byte aligned
    static constexpr usize GUARD_PAGE_SIZE = 4096;              // 4KB guard page
    
    usize stack_size = DEFAULT_STACK_SIZE;
    usize guard_size = GUARD_PAGE_SIZE;
    bool enable_guard_pages = true;
    bool enable_stack_overflow_detection = true;
    u32 numa_node = 0;  // NUMA node for stack allocation
    
    static FiberStackConfig small() {
        FiberStackConfig config;
        config.stack_size = SMALL_STACK_SIZE;
        return config;
    }
    
    static FiberStackConfig large() {
        FiberStackConfig config;
        config.stack_size = LARGE_STACK_SIZE;
        return config;
    }
    
    static FiberStackConfig huge() {
        FiberStackConfig config;
        config.stack_size = HUGE_STACK_SIZE;
        return config;
    }
};

/**
 * @brief Fiber execution statistics
 */
struct FiberStats {
    std::chrono::high_resolution_clock::time_point creation_time;
    std::chrono::high_resolution_clock::time_point start_time;
    std::chrono::high_resolution_clock::time_point end_time;
    
    u64 context_switches = 0;
    u64 yield_count = 0;
    u64 resume_count = 0;
    u64 stack_bytes_used = 0;
    u32 worker_id = 0;
    u32 cpu_core = 0;
    
    f64 execution_time_ms() const noexcept {
        return std::chrono::duration<f64, std::milli>(end_time - start_time).count();
    }
    
    f64 average_yield_time_us() const noexcept {
        if (yield_count == 0) return 0.0;
        return execution_time_ms() * 1000.0 / yield_count;
    }
};

/**
 * @brief Unique fiber identifier
 */
struct FiberID {
    static constexpr u32 INVALID_INDEX = std::numeric_limits<u32>::max();
    static constexpr u16 INVALID_GENERATION = 0;
    
    u32 index = INVALID_INDEX;
    u16 generation = INVALID_GENERATION;
    
    constexpr FiberID() noexcept = default;
    constexpr FiberID(u32 idx, u16 gen) noexcept : index(idx), generation(gen) {}
    
    constexpr bool is_valid() const noexcept {
        return index != INVALID_INDEX && generation != INVALID_GENERATION;
    }
    
    constexpr bool operator==(const FiberID& other) const noexcept {
        return index == other.index && generation == other.generation;
    }
    
    struct Hash {
        constexpr usize operator()(const FiberID& id) const noexcept {
            return (static_cast<usize>(id.index) << 16) | id.generation;
        }
    };
};

//=============================================================================
// Platform-Specific Context Implementation
//=============================================================================

/**
 * @brief Platform-specific fiber context
 */
struct FiberContext {
#ifdef _WIN32
    void* fiber_handle = nullptr;
    void* main_fiber = nullptr;
#elif defined(__linux__) || defined(__APPLE__)
    ucontext_t context;
    ucontext_t* caller_context = nullptr;
#endif
    
    void* stack_base = nullptr;
    usize stack_size = 0;
    bool owns_stack = false;
    
    FiberContext() = default;
    ~FiberContext();
    
    // Non-copyable
    FiberContext(const FiberContext&) = delete;
    FiberContext& operator=(const FiberContext&) = delete;
    
    // Movable
    FiberContext(FiberContext&& other) noexcept;
    FiberContext& operator=(FiberContext&& other) noexcept;
};

//=============================================================================
// Fiber Implementation
//=============================================================================

/**
 * @brief High-performance stackful fiber for cooperative multitasking
 */
class Fiber {
public:
    using FiberFunction = std::function<void()>;
    
private:
    FiberID id_;
    std::string name_;
    FiberFunction function_;
    std::atomic<FiberState> state_;
    FiberPriority priority_;
    
    // Context and stack management
    std::unique_ptr<FiberContext> context_;
    FiberStackConfig stack_config_;
    
    // Scheduling and synchronization
    Fiber* caller_fiber_ = nullptr;
    std::atomic<bool> should_yield_{false};
    std::atomic<u64> yield_count_{0};
    
    // Performance tracking
    FiberStats stats_;
    
    // Pool management
    FiberPool* pool_ = nullptr;
    std::atomic<bool> pooled_{false};
    
public:
    Fiber(FiberID id, std::string name, FiberFunction function,
          const FiberStackConfig& stack_config = FiberStackConfig{},
          FiberPriority priority = FiberPriority::Normal);
    
    ~Fiber();
    
    // Non-copyable but movable
    Fiber(const Fiber&) = delete;
    Fiber& operator=(const Fiber&) = delete;
    Fiber(Fiber&&) noexcept = default;
    Fiber& operator=(Fiber&&) noexcept = default;
    
    // Core fiber operations
    void start(Fiber* caller = nullptr);
    void resume();
    void yield();
    void yield_to(Fiber& target);
    bool is_finished() const noexcept;
    
    // State management
    FiberState state() const noexcept { return state_.load(std::memory_order_acquire); }
    void set_priority(FiberPriority priority) noexcept { priority_ = priority; }
    
    // Configuration
    Fiber& set_stack_config(const FiberStackConfig& config);
    Fiber& set_name(const std::string& name) { name_ = name; return *this; }
    
    // Accessors
    FiberID id() const noexcept { return id_; }
    const std::string& name() const noexcept { return name_; }
    FiberPriority priority() const noexcept { return priority_; }
    usize stack_size() const noexcept { return stack_config_.stack_size; }
    const FiberStats& statistics() const noexcept { return stats_; }
    
    // Stack utilities
    usize stack_usage() const noexcept;
    f64 stack_usage_percent() const noexcept;
    bool has_stack_overflow() const noexcept;
    
    // Pool integration
    void set_pool(FiberPool* pool) { pool_ = pool; }
    bool is_pooled() const noexcept { return pooled_.load(std::memory_order_acquire); }
    void return_to_pool();
    
private:
    void initialize_context();
    void cleanup_context();
    void switch_to_fiber(Fiber& target);
    void fiber_entry_point();
    void update_stats();
    void set_state(FiberState new_state) noexcept;
    
    // Platform-specific implementations
    void create_fiber_context();
    void destroy_fiber_context();
    void switch_fiber_context(FiberContext& target);
    
    static void fiber_main_wrapper(void* fiber_ptr);
    
    friend class FiberPool;
    friend class FiberScheduler;
};

//=============================================================================
// Fiber Pool for Memory Reuse
//=============================================================================

/**
 * @brief Thread-safe fiber pool for efficient fiber reuse
 */
class FiberPool {
private:
    struct PoolConfig {
        usize initial_size = 32;
        usize max_size = 1024;
        usize growth_increment = 16;
        FiberStackConfig default_stack_config;
        bool enable_statistics = true;
    };
    
    PoolConfig config_;
    std::vector<std::unique_ptr<Fiber>> available_fibers_;
    mutable std::mutex pool_mutex_;
    
    // Statistics
    std::atomic<u64> fibers_created_{0};
    std::atomic<u64> fibers_reused_{0};
    std::atomic<u64> fibers_destroyed_{0};
    std::atomic<u64> pool_hits_{0};
    std::atomic<u64> pool_misses_{0};
    
    // Fiber ID generation
    std::atomic<u32> next_fiber_index_{1};
    std::atomic<u16> generation_counter_{1};
    
public:
    explicit FiberPool(const PoolConfig& config = PoolConfig{});
    ~FiberPool();
    
    // Non-copyable, non-movable
    FiberPool(const FiberPool&) = delete;
    FiberPool& operator=(const FiberPool&) = delete;
    FiberPool(FiberPool&&) = delete;
    FiberPool& operator=(FiberPool&&) = delete;
    
    // Fiber lifecycle
    std::unique_ptr<Fiber> acquire_fiber(const std::string& name,
                                        Fiber::FiberFunction function,
                                        const FiberStackConfig& stack_config = FiberStackConfig{},
                                        FiberPriority priority = FiberPriority::Normal);
    
    void return_fiber(std::unique_ptr<Fiber> fiber);
    
    // Pool management
    void prealloc_fibers(usize count);
    void shrink_pool();
    void clear_pool();
    
    // Statistics
    struct PoolStats {
        u64 total_created = 0;
        u64 total_reused = 0;
        u64 total_destroyed = 0;
        u64 pool_hits = 0;
        u64 pool_misses = 0;
        usize current_pool_size = 0;
        f64 reuse_ratio = 0.0;
        f64 hit_ratio = 0.0;
    };
    
    PoolStats get_statistics() const;
    void reset_statistics();
    
    // Configuration
    usize available_count() const;
    usize max_pool_size() const noexcept { return config_.max_size; }
    void set_max_pool_size(usize max_size);
    
private:
    FiberID generate_fiber_id();
    std::unique_ptr<Fiber> create_new_fiber(const std::string& name,
                                           Fiber::FiberFunction function,
                                           const FiberStackConfig& stack_config,
                                           FiberPriority priority);
    
    bool can_reuse_fiber(const Fiber& fiber, const FiberStackConfig& required_config) const;
    void reset_fiber_for_reuse(Fiber& fiber, const std::string& name,
                              Fiber::FiberFunction function, FiberPriority priority);
};

//=============================================================================
// Fiber Utilities and Helpers
//=============================================================================

/**
 * @brief Global fiber utilities
 */
class FiberUtils {
public:
    // Get current fiber context
    static Fiber* current_fiber();
    static FiberID current_fiber_id();
    static bool is_running_in_fiber();
    
    // Fiber control
    static void yield();
    static void yield_for(std::chrono::microseconds duration);
    static void sleep_for(std::chrono::microseconds duration);
    
    // Stack utilities
    static usize get_stack_usage();
    static f64 get_stack_usage_percent();
    static void check_stack_overflow();
    
    // Performance monitoring
    static void enable_performance_monitoring(bool enable);
    static std::string get_fiber_performance_report();
    
private:
    static thread_local Fiber* current_fiber_;
    static thread_local bool performance_monitoring_enabled_;
};

//=============================================================================
// Platform-Specific Stack Management
//=============================================================================

/**
 * @brief Platform-specific stack allocation and management
 */
class FiberStackAllocator {
public:
    struct StackInfo {
        void* base = nullptr;
        usize size = 0;
        u32 numa_node = 0;
        bool has_guard_pages = false;
        
        bool is_valid() const noexcept { return base != nullptr && size > 0; }
    };
    
    static StackInfo allocate_stack(const FiberStackConfig& config);
    static void deallocate_stack(const StackInfo& stack_info);
    
    // Stack utilities
    static void setup_guard_pages(const StackInfo& stack_info);
    static void remove_guard_pages(const StackInfo& stack_info);
    static usize calculate_stack_usage(const StackInfo& stack_info);
    
    // NUMA-aware allocation
    static void set_numa_affinity(u32 numa_node);
    static u32 get_current_numa_node();
    
private:
    static void* allocate_aligned_memory(usize size, usize alignment, u32 numa_node);
    static void deallocate_aligned_memory(void* ptr, usize size);
};

} // namespace ecscope::jobs