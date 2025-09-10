#include "jobs/fiber.hpp"
#include "jobs/fiber_sync.hpp"
#include <cstring>
#include <cassert>
#include <algorithm>
#include <thread>

// Platform-specific includes
#ifdef _WIN32
    #include <windows.h>
    #include <sysinfoapi.h>
#elif defined(__linux__)
    #include <sys/mman.h>
    #ifdef ECSCOPE_ENABLE_NUMA
        #include <numa.h>
    #endif
    #include <sched.h>
    #include <unistd.h>
    #include <signal.h>
#elif defined(__APPLE__)
    #include <sys/mman.h>
    #include <unistd.h>
    #include <mach/mach.h>
#endif

namespace ecscope::jobs {

//=============================================================================
// Thread-Local Storage
//=============================================================================

thread_local Fiber* FiberUtils::current_fiber_ = nullptr;
thread_local bool FiberUtils::performance_monitoring_enabled_ = false;

//=============================================================================
// Platform-Specific Context Implementation
//=============================================================================

FiberContext::FiberContext(FiberContext&& other) noexcept {
#ifdef _WIN32
    fiber_handle = other.fiber_handle;
    main_fiber = other.main_fiber;
    other.fiber_handle = nullptr;
    other.main_fiber = nullptr;
#elif defined(__linux__) || defined(__APPLE__)
    context = other.context;
    caller_context = other.caller_context;
    other.caller_context = nullptr;
    std::memset(&other.context, 0, sizeof(other.context));
#endif
    
    stack_base = other.stack_base;
    stack_size = other.stack_size;
    owns_stack = other.owns_stack;
    
    other.stack_base = nullptr;
    other.stack_size = 0;
    other.owns_stack = false;
}

FiberContext& FiberContext::operator=(FiberContext&& other) noexcept {
    if (this != &other) {
        // Cleanup current state
        this->~FiberContext();
        
        // Move from other
#ifdef _WIN32
        fiber_handle = other.fiber_handle;
        main_fiber = other.main_fiber;
        other.fiber_handle = nullptr;
        other.main_fiber = nullptr;
#elif defined(__linux__) || defined(__APPLE__)
        context = other.context;
        caller_context = other.caller_context;
        other.caller_context = nullptr;
        std::memset(&other.context, 0, sizeof(other.context));
#endif
        
        stack_base = other.stack_base;
        stack_size = other.stack_size;
        owns_stack = other.owns_stack;
        
        other.stack_base = nullptr;
        other.stack_size = 0;
        other.owns_stack = false;
    }
    return *this;
}

FiberContext::~FiberContext() {
#ifdef _WIN32
    if (fiber_handle) {
        DeleteFiber(fiber_handle);
        fiber_handle = nullptr;
    }
#elif defined(__linux__) || defined(__APPLE__)
    // ucontext_t doesn't need explicit cleanup
#endif
    
    if (owns_stack && stack_base) {
        FiberStackAllocator::deallocate_stack(
            FiberStackAllocator::StackInfo{stack_base, stack_size, 0, false});
    }
}

//=============================================================================
// Fiber Stack Allocator Implementation
//=============================================================================

FiberStackAllocator::StackInfo FiberStackAllocator::allocate_stack(const FiberStackConfig& config) {
    StackInfo info;
    info.size = config.stack_size;
    info.numa_node = config.numa_node;
    info.has_guard_pages = config.enable_guard_pages;
    
    // Align stack size to page boundaries
    const usize page_size = getpagesize();
    const usize aligned_size = ((config.stack_size + page_size - 1) / page_size) * page_size;
    const usize total_size = aligned_size + (config.enable_guard_pages ? config.guard_size * 2 : 0);
    
#ifdef _WIN32
    // Windows implementation
    info.base = VirtualAlloc(nullptr, total_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!info.base) {
        return StackInfo{};
    }
    
    if (config.enable_guard_pages) {
        // Set guard pages
        DWORD old_protect;
        VirtualProtect(info.base, config.guard_size, PAGE_NOACCESS, &old_protect);
        VirtualProtect(static_cast<u8*>(info.base) + total_size - config.guard_size, 
                      config.guard_size, PAGE_NOACCESS, &old_protect);
        info.base = static_cast<u8*>(info.base) + config.guard_size;
    }
    
#elif defined(__linux__)
    // Linux implementation with NUMA awareness
#ifdef ECSCOPE_ENABLE_NUMA
    if (config.numa_node != 0 && numa_available() >= 0) {
        set_numa_affinity(config.numa_node);
    }
#endif
    
    info.base = mmap(nullptr, total_size, PROT_READ | PROT_WRITE, 
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
    if (info.base == MAP_FAILED) {
        info.base = nullptr;
        return StackInfo{};
    }
    
    if (config.enable_guard_pages) {
        // Set guard pages
        if (mprotect(info.base, config.guard_size, PROT_NONE) != 0 ||
            mprotect(static_cast<u8*>(info.base) + total_size - config.guard_size, 
                    config.guard_size, PROT_NONE) != 0) {
            munmap(info.base, total_size);
            info.base = nullptr;
            return StackInfo{};
        }
        info.base = static_cast<u8*>(info.base) + config.guard_size;
    }
    
#elif defined(__APPLE__)
    // macOS implementation
    info.base = mmap(nullptr, total_size, PROT_READ | PROT_WRITE, 
                     MAP_PRIVATE | MAP_ANON, -1, 0);
    if (info.base == MAP_FAILED) {
        info.base = nullptr;
        return StackInfo{};
    }
    
    if (config.enable_guard_pages) {
        // Set guard pages
        if (mprotect(info.base, config.guard_size, PROT_NONE) != 0 ||
            mprotect(static_cast<u8*>(info.base) + total_size - config.guard_size, 
                    config.guard_size, PROT_NONE) != 0) {
            munmap(info.base, total_size);
            info.base = nullptr;
            return StackInfo{};
        }
        info.base = static_cast<u8*>(info.base) + config.guard_size;
    }
#endif
    
    info.size = aligned_size;
    return info;
}

void FiberStackAllocator::deallocate_stack(const StackInfo& stack_info) {
    if (!stack_info.is_valid()) return;
    
    const usize page_size = getpagesize();
    void* actual_base = stack_info.base;
    usize actual_size = stack_info.size;
    
    if (stack_info.has_guard_pages) {
        // Account for guard pages
        const usize guard_size = page_size;  // Assume standard guard page size
        actual_base = static_cast<u8*>(stack_info.base) - guard_size;
        actual_size += guard_size * 2;
    }
    
#ifdef _WIN32
    VirtualFree(actual_base, 0, MEM_RELEASE);
#else
    munmap(actual_base, actual_size);
#endif
}

void FiberStackAllocator::setup_guard_pages(const StackInfo& stack_info) {
    if (!stack_info.is_valid()) return;
    
    const usize page_size = getpagesize();
    void* guard_low = static_cast<u8*>(stack_info.base) - page_size;
    void* guard_high = static_cast<u8*>(stack_info.base) + stack_info.size;
    
#ifdef _WIN32
    DWORD old_protect;
    VirtualProtect(guard_low, page_size, PAGE_NOACCESS, &old_protect);
    VirtualProtect(guard_high, page_size, PAGE_NOACCESS, &old_protect);
#else
    mprotect(guard_low, page_size, PROT_NONE);
    mprotect(guard_high, page_size, PROT_NONE);
#endif
}

void FiberStackAllocator::remove_guard_pages(const StackInfo& stack_info) {
    if (!stack_info.is_valid()) return;
    
    const usize page_size = getpagesize();
    void* guard_low = static_cast<u8*>(stack_info.base) - page_size;
    void* guard_high = static_cast<u8*>(stack_info.base) + stack_info.size;
    
#ifdef _WIN32
    DWORD old_protect;
    VirtualProtect(guard_low, page_size, PAGE_READWRITE, &old_protect);
    VirtualProtect(guard_high, page_size, PAGE_READWRITE, &old_protect);
#else
    mprotect(guard_low, page_size, PROT_READ | PROT_WRITE);
    mprotect(guard_high, page_size, PROT_READ | PROT_WRITE);
#endif
}

usize FiberStackAllocator::calculate_stack_usage(const StackInfo& stack_info) {
    if (!stack_info.is_valid()) return 0;
    
    // Simple stack usage calculation by finding uninitialized stack area
    // This is a rough approximation - more sophisticated methods could be used
    const u8* stack_start = static_cast<const u8*>(stack_info.base);
    const u8* stack_end = stack_start + stack_info.size;
    
    // Look for uninitialized patterns from the bottom up
    const u8 pattern = 0xCC;  // Common uninitialized pattern
    const u8* current = stack_start;
    
    while (current < stack_end && *current == pattern) {
        ++current;
    }
    
    return stack_end - current;
}

void FiberStackAllocator::set_numa_affinity(u32 numa_node) {
#if defined(__linux__) && defined(ECSCOPE_ENABLE_NUMA)
    if (numa_available() >= 0 && numa_node < static_cast<u32>(numa_max_node() + 1)) {
        numa_run_on_node(static_cast<int>(numa_node));
    }
#else
    (void)numa_node; // Suppress unused parameter warning
#endif
}

u32 FiberStackAllocator::get_current_numa_node() {
#if defined(__linux__) && defined(ECSCOPE_ENABLE_NUMA)
    if (numa_available() >= 0) {
        return static_cast<u32>(numa_node_of_cpu(sched_getcpu()));
    }
#endif
    return 0;
}

void* FiberStackAllocator::allocate_aligned_memory(usize size, usize alignment, u32 numa_node) {
#if defined(__linux__) && defined(ECSCOPE_ENABLE_NUMA)
    if (numa_node != 0 && numa_available() >= 0) {
        set_numa_affinity(numa_node);
    }
#else
    (void)numa_node; // Suppress unused parameter warning
#endif
    
#ifdef _WIN32
    return _aligned_malloc(size, alignment);
#else
    void* ptr = nullptr;
    if (posix_memalign(&ptr, alignment, size) != 0) {
        return nullptr;
    }
    return ptr;
#endif
}

void FiberStackAllocator::deallocate_aligned_memory(void* ptr, usize size) {
    (void)size;  // Unused parameter
    
#ifdef _WIN32
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

//=============================================================================
// Fiber Implementation
//=============================================================================

Fiber::Fiber(FiberID id, std::string name, FiberFunction function,
             const FiberStackConfig& stack_config, FiberPriority priority)
    : id_(id)
    , name_(std::move(name))
    , function_(std::move(function))
    , state_(FiberState::Created)
    , priority_(priority)
    , context_(std::make_unique<FiberContext>())
    , stack_config_(stack_config)
    , caller_fiber_(nullptr)
    , should_yield_(false)
    , yield_count_(0)
    , pool_(nullptr)
    , pooled_(false) {
    
    stats_.creation_time = std::chrono::high_resolution_clock::now();
    initialize_context();
}

Fiber::~Fiber() {
    cleanup_context();
}

void Fiber::initialize_context() {
    // Allocate stack
    auto stack_info = FiberStackAllocator::allocate_stack(stack_config_);
    if (!stack_info.is_valid()) {
        set_state(FiberState::Error);
        return;
    }
    
    context_->stack_base = stack_info.base;
    context_->stack_size = stack_info.size;
    context_->owns_stack = true;
    
    create_fiber_context();
}

void Fiber::cleanup_context() {
    if (context_) {
        destroy_fiber_context();
    }
}

void Fiber::create_fiber_context() {
#ifdef _WIN32
    // Windows fiber implementation
    context_->fiber_handle = CreateFiber(context_->stack_size, 
                                        &Fiber::fiber_main_wrapper, this);
    if (!context_->fiber_handle) {
        set_state(FiberState::Error);
    }
    
#elif defined(__linux__) || defined(__APPLE__)
    // POSIX ucontext implementation
    if (getcontext(&context_->context) == -1) {
        set_state(FiberState::Error);
        return;
    }
    
    context_->context.uc_stack.ss_sp = context_->stack_base;
    context_->context.uc_stack.ss_size = context_->stack_size;
    context_->context.uc_link = nullptr;
    
    makecontext(&context_->context, reinterpret_cast<void(*)()>(&Fiber::fiber_main_wrapper), 
                1, this);
#endif
}

void Fiber::destroy_fiber_context() {
    // Context cleanup is handled by FiberContext destructor
}

void Fiber::start(Fiber* caller) {
    if (state() != FiberState::Created) {
        return;
    }
    
    caller_fiber_ = caller;
    set_state(FiberState::Ready);
    
    // Switch to this fiber
    if (caller) {
        switch_to_fiber(*this);
    } else {
        // Direct execution (for main fiber)
        resume();
    }
}

void Fiber::resume() {
    if (state() != FiberState::Ready && state() != FiberState::Suspended) {
        return;
    }
    
    FiberUtils::current_fiber_ = this;
    set_state(FiberState::Running);
    stats_.start_time = std::chrono::high_resolution_clock::now();
    stats_.resume_count++;
    
#ifdef _WIN32
    SwitchToFiber(context_->fiber_handle);
#elif defined(__linux__) || defined(__APPLE__)
    ucontext_t caller_context;
    context_->caller_context = &caller_context;
    swapcontext(&caller_context, &context_->context);
#endif
}

void Fiber::yield() {
    if (state() != FiberState::Running) {
        return;
    }
    
    should_yield_ = true;
    yield_count_.fetch_add(1, std::memory_order_relaxed);
    stats_.yield_count++;
    
    set_state(FiberState::Suspended);
    
    // Switch back to caller
    if (caller_fiber_) {
        switch_to_fiber(*caller_fiber_);
    } else {
        // Yield to scheduler
        FiberUtils::current_fiber_ = nullptr;
#ifdef _WIN32
        SwitchToFiber(context_->main_fiber);
#elif defined(__linux__) || defined(__APPLE__)
        if (context_->caller_context) {
            swapcontext(&context_->context, context_->caller_context);
        }
#endif
    }
}

void Fiber::yield_to(Fiber& target) {
    if (state() != FiberState::Running || target.state() != FiberState::Ready) {
        return;
    }
    
    yield_count_.fetch_add(1, std::memory_order_relaxed);
    stats_.yield_count++;
    
    set_state(FiberState::Suspended);
    switch_to_fiber(target);
}

void Fiber::switch_to_fiber(Fiber& target) {
    stats_.context_switches++;
    target.stats_.context_switches++;
    
    FiberUtils::current_fiber_ = &target;
    target.set_state(FiberState::Running);
    
#ifdef _WIN32
    SwitchToFiber(target.context_->fiber_handle);
#elif defined(__linux__) || defined(__APPLE__)
    target.context_->caller_context = &context_->context;
    swapcontext(&context_->context, &target.context_->context);
#endif
}

bool Fiber::is_finished() const noexcept {
    FiberState current_state = state();
    return current_state == FiberState::Completed || current_state == FiberState::Error;
}

void Fiber::fiber_entry_point() {
    try {
        function_();
        set_state(FiberState::Completed);
    } catch (...) {
        set_state(FiberState::Error);
    }
    
    stats_.end_time = std::chrono::high_resolution_clock::now();
    update_stats();
    
    // Return to caller or scheduler
    if (caller_fiber_) {
        switch_to_fiber(*caller_fiber_);
    } else {
        FiberUtils::current_fiber_ = nullptr;
    }
}

void Fiber::update_stats() {
    stats_.stack_bytes_used = stack_usage();
    
    // Update other statistics as needed
    auto current_time = std::chrono::high_resolution_clock::now();
    if (stats_.start_time != std::chrono::high_resolution_clock::time_point{}) {
        // Calculate execution time
        stats_.end_time = current_time;
    }
}

void Fiber::set_state(FiberState new_state) noexcept {
    state_.store(new_state, std::memory_order_release);
}

usize Fiber::stack_usage() const noexcept {
    if (!context_ || !context_->stack_base) {
        return 0;
    }
    
    return FiberStackAllocator::calculate_stack_usage(
        FiberStackAllocator::StackInfo{
            context_->stack_base, 
            context_->stack_size, 
            0, 
            stack_config_.enable_guard_pages
        });
}

f64 Fiber::stack_usage_percent() const noexcept {
    if (context_->stack_size == 0) {
        return 0.0;
    }
    
    return static_cast<f64>(stack_usage()) / static_cast<f64>(context_->stack_size) * 100.0;
}

bool Fiber::has_stack_overflow() const noexcept {
    // Simple stack overflow detection
    return stack_usage_percent() > 95.0;  // 95% threshold
}

Fiber& Fiber::set_stack_config(const FiberStackConfig& config) {
    if (state() != FiberState::Created) {
        return *this;  // Cannot change stack config after creation
    }
    
    stack_config_ = config;
    
    // Re-initialize context if needed
    cleanup_context();
    initialize_context();
    
    return *this;
}

void Fiber::return_to_pool() {
    if (pool_ && !pooled_.load(std::memory_order_acquire)) {
        pooled_.store(true, std::memory_order_release);
        // Pool will handle the actual return
    }
}

void Fiber::fiber_main_wrapper(void* fiber_ptr) {
    Fiber* fiber = static_cast<Fiber*>(fiber_ptr);
    fiber->fiber_entry_point();
}

//=============================================================================
// Fiber Pool Implementation
//=============================================================================

FiberPool::FiberPool(const PoolConfig& config)
    : config_(config)
    , next_fiber_index_(1)
    , generation_counter_(1) {
    
    available_fibers_.reserve(config_.initial_size);
    
    // Pre-allocate initial fibers
    prealloc_fibers(config_.initial_size);
}

FiberPool::~FiberPool() {
    clear_pool();
}

std::unique_ptr<Fiber> FiberPool::acquire_fiber(const std::string& name,
                                               Fiber::FiberFunction function,
                                               const FiberStackConfig& stack_config,
                                               FiberPriority priority) {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    // Try to reuse an existing fiber
    for (auto it = available_fibers_.begin(); it != available_fibers_.end(); ++it) {
        if (can_reuse_fiber(**it, stack_config)) {
            auto fiber = std::move(*it);
            available_fibers_.erase(it);
            
            reset_fiber_for_reuse(*fiber, name, std::move(function), priority);
            
            pool_hits_.fetch_add(1, std::memory_order_relaxed);
            fibers_reused_.fetch_add(1, std::memory_order_relaxed);
            
            return fiber;
        }
    }
    
    // Create new fiber
    pool_misses_.fetch_add(1, std::memory_order_relaxed);
    return create_new_fiber(name, std::move(function), stack_config, priority);
}

void FiberPool::return_fiber(std::unique_ptr<Fiber> fiber) {
    if (!fiber || !fiber->is_finished()) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    if (available_fibers_.size() >= config_.max_size) {
        // Pool is full, destroy the fiber
        fibers_destroyed_.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    
    // Reset fiber state for reuse
    fiber->set_state(FiberState::Created);
    fiber->pooled_.store(false, std::memory_order_release);
    
    available_fibers_.push_back(std::move(fiber));
}

void FiberPool::prealloc_fibers(usize count) {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    for (usize i = 0; i < count; ++i) {
        if (available_fibers_.size() >= config_.max_size) {
            break;
        }
        
        auto fiber = create_new_fiber("PreallocatedFiber", [](){}, 
                                     config_.default_stack_config, 
                                     FiberPriority::Normal);
        if (fiber) {
            available_fibers_.push_back(std::move(fiber));
        }
    }
}

void FiberPool::shrink_pool() {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    const usize target_size = std::max(config_.initial_size, available_fibers_.size() / 2);
    
    while (available_fibers_.size() > target_size) {
        available_fibers_.pop_back();
        fibers_destroyed_.fetch_add(1, std::memory_order_relaxed);
    }
}

void FiberPool::clear_pool() {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    usize destroyed_count = available_fibers_.size();
    available_fibers_.clear();
    
    fibers_destroyed_.fetch_add(destroyed_count, std::memory_order_relaxed);
}

FiberPool::PoolStats FiberPool::get_statistics() const {
    PoolStats stats;
    stats.total_created = fibers_created_.load(std::memory_order_relaxed);
    stats.total_reused = fibers_reused_.load(std::memory_order_relaxed);
    stats.total_destroyed = fibers_destroyed_.load(std::memory_order_relaxed);
    stats.pool_hits = pool_hits_.load(std::memory_order_relaxed);
    stats.pool_misses = pool_misses_.load(std::memory_order_relaxed);
    
    std::lock_guard<std::mutex> lock(pool_mutex_);
    stats.current_pool_size = available_fibers_.size();
    
    u64 total_requests = stats.pool_hits + stats.pool_misses;
    if (total_requests > 0) {
        stats.hit_ratio = static_cast<f64>(stats.pool_hits) / static_cast<f64>(total_requests);
    }
    
    u64 total_fibers = stats.total_created + stats.total_reused;
    if (total_fibers > 0) {
        stats.reuse_ratio = static_cast<f64>(stats.total_reused) / static_cast<f64>(total_fibers);
    }
    
    return stats;
}

void FiberPool::reset_statistics() {
    fibers_created_.store(0, std::memory_order_relaxed);
    fibers_reused_.store(0, std::memory_order_relaxed);
    fibers_destroyed_.store(0, std::memory_order_relaxed);
    pool_hits_.store(0, std::memory_order_relaxed);
    pool_misses_.store(0, std::memory_order_relaxed);
}

usize FiberPool::available_count() const {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    return available_fibers_.size();
}

void FiberPool::set_max_pool_size(usize max_size) {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    config_.max_size = max_size;
    
    // Shrink if necessary
    while (available_fibers_.size() > max_size) {
        available_fibers_.pop_back();
        fibers_destroyed_.fetch_add(1, std::memory_order_relaxed);
    }
}

FiberID FiberPool::generate_fiber_id() {
    u32 index = next_fiber_index_.fetch_add(1, std::memory_order_relaxed);
    u16 generation = generation_counter_.load(std::memory_order_relaxed);
    
    // Handle overflow
    if (index == 0) {
        generation_counter_.fetch_add(1, std::memory_order_relaxed);
        generation = generation_counter_.load(std::memory_order_relaxed);
    }
    
    return FiberID{index, generation};
}

std::unique_ptr<Fiber> FiberPool::create_new_fiber(const std::string& name,
                                                  Fiber::FiberFunction function,
                                                  const FiberStackConfig& stack_config,
                                                  FiberPriority priority) {
    FiberID fiber_id = generate_fiber_id();
    
    auto fiber = std::make_unique<Fiber>(fiber_id, name, std::move(function), 
                                        stack_config, priority);
    fiber->set_pool(this);
    
    fibers_created_.fetch_add(1, std::memory_order_relaxed);
    
    return fiber;
}

bool FiberPool::can_reuse_fiber(const Fiber& fiber, const FiberStackConfig& required_config) const {
    // Check if fiber can be reused with the required configuration
    return fiber.stack_size() >= required_config.stack_size &&
           fiber.is_finished() &&
           !fiber.is_pooled();
}

void FiberPool::reset_fiber_for_reuse(Fiber& fiber, const std::string& name,
                                     Fiber::FiberFunction function, FiberPriority priority) {
    fiber.set_name(name);
    fiber.function_ = std::move(function);
    fiber.set_priority(priority);
    fiber.set_state(FiberState::Created);
    fiber.should_yield_.store(false, std::memory_order_relaxed);
    fiber.yield_count_.store(0, std::memory_order_relaxed);
    fiber.caller_fiber_ = nullptr;
    
    // Reset statistics
    fiber.stats_ = FiberStats{};
    fiber.stats_.creation_time = std::chrono::high_resolution_clock::now();
}

//=============================================================================
// Fiber Utilities Implementation
//=============================================================================

Fiber* FiberUtils::current_fiber() {
    return current_fiber_;
}

FiberID FiberUtils::current_fiber_id() {
    return current_fiber_ ? current_fiber_->id() : FiberID{};
}

bool FiberUtils::is_running_in_fiber() {
    return current_fiber_ != nullptr;
}

void FiberUtils::yield() {
    if (current_fiber_) {
        current_fiber_->yield();
    } else {
        // Not in a fiber - yield thread
        std::this_thread::yield();
    }
}

void FiberUtils::yield_for(std::chrono::microseconds duration) {
    auto start = std::chrono::steady_clock::now();
    
    while (std::chrono::steady_clock::now() - start < duration) {
        yield();
    }
}

void FiberUtils::sleep_for(std::chrono::microseconds duration) {
    if (current_fiber_) {
        // In fiber - cooperative sleep
        yield_for(duration);
    } else {
        // Not in fiber - use thread sleep
        std::this_thread::sleep_for(duration);
    }
}

usize FiberUtils::get_stack_usage() {
    return current_fiber_ ? current_fiber_->stack_usage() : 0;
}

f64 FiberUtils::get_stack_usage_percent() {
    return current_fiber_ ? current_fiber_->stack_usage_percent() : 0.0;
}

void FiberUtils::check_stack_overflow() {
    if (current_fiber_ && current_fiber_->has_stack_overflow()) {
        // Handle stack overflow
        throw std::runtime_error("Fiber stack overflow detected");
    }
}

void FiberUtils::enable_performance_monitoring(bool enable) {
    performance_monitoring_enabled_ = enable;
}

std::string FiberUtils::get_fiber_performance_report() {
    if (!current_fiber_) {
        return "No active fiber";
    }
    
    const auto& stats = current_fiber_->statistics();
    
    std::string report = "Fiber Performance Report\\n";
    report += "Fiber ID: " + std::to_string(current_fiber_->id().index) + "\\n";
    report += "Name: " + current_fiber_->name() + "\\n";
    report += "Execution Time: " + std::to_string(stats.execution_time_ms()) + " ms\\n";
    report += "Context Switches: " + std::to_string(stats.context_switches) + "\\n";
    report += "Yield Count: " + std::to_string(stats.yield_count) + "\\n";
    report += "Stack Usage: " + std::to_string(current_fiber_->stack_usage_percent()) + "%\\n";
    
    return report;
}

} // namespace ecscope::jobs