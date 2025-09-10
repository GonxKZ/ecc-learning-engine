#pragma once

/**
 * @file fiber_sync.hpp
 * @brief Fiber-Aware Synchronization Primitives for ECScope Job System
 * 
 * This file implements high-performance synchronization primitives designed
 * specifically for fiber-based cooperative multitasking:
 * 
 * - Fiber-aware mutexes that yield instead of blocking
 * - Condition variables with fiber-safe waiting
 * - Read-write locks optimized for cooperative scheduling
 * - Semaphores and barriers for fiber coordination
 * - Lock-free channels for inter-fiber communication
 * 
 * Key Features:
 * - Zero OS thread blocking - all operations yield cooperatively
 * - Deadlock detection and prevention mechanisms
 * - Priority inheritance to prevent priority inversion
 * - Lock-free fast paths for uncontended cases
 * - Integration with fiber scheduler for optimal performance
 * 
 * Performance Benefits:
 * - No kernel transitions for synchronization
 * - Optimal cache locality through cooperative yielding
 * - Predictable latency without OS scheduler interference
 * - Scales linearly with number of logical cores
 * 
 * @author ECScope Engine - Fiber Synchronization
 * @date 2025
 */

#include "fiber.hpp"
#include "core/types.hpp"
#include "lockfree_structures.hpp"
#include <atomic>
#include <chrono>
#include <memory>
#include <queue>
#include <vector>
#include <optional>

namespace ecscope::jobs {

//=============================================================================
// Forward Declarations
//=============================================================================

class FiberMutex;
class FiberConditionVariable;
class FiberReadWriteLock;
class FiberSemaphore;
class FiberBarrier;
template<typename T> class FiberChannel;

//=============================================================================
// Fiber Mutex - Cooperative Mutual Exclusion
//=============================================================================

/**
 * @brief Fiber-aware mutex that yields instead of blocking
 */
class FiberMutex {
private:
    std::atomic<FiberID> owner_{FiberID{}};
    std::atomic<u32> recursion_count_{0};
    
    // Wait queue for fibers waiting to acquire the mutex
    struct WaitNode {
        FiberID fiber_id;
        FiberPriority priority;
        std::chrono::steady_clock::time_point wait_start;
        WaitNode* next = nullptr;
        
        WaitNode(FiberID id, FiberPriority prio) 
            : fiber_id(id), priority(prio), wait_start(std::chrono::steady_clock::now()) {}
    };
    
    std::atomic<WaitNode*> wait_queue_head_{nullptr};
    std::atomic<WaitNode*> wait_queue_tail_{nullptr};
    
    // Statistics and debugging
    std::atomic<u64> acquire_count_{0};
    std::atomic<u64> contention_count_{0};
    std::atomic<u64> total_wait_time_us_{0};
    std::string debug_name_;
    
    // Configuration
    bool enable_priority_inheritance_ = true;
    bool enable_deadlock_detection_ = true;
    std::chrono::microseconds max_wait_time_{std::chrono::seconds{10}};
    
public:
    explicit FiberMutex(const std::string& name = "");
    ~FiberMutex();
    
    // Non-copyable, non-movable
    FiberMutex(const FiberMutex&) = delete;
    FiberMutex& operator=(const FiberMutex&) = delete;
    FiberMutex(FiberMutex&&) = delete;
    FiberMutex& operator=(FiberMutex&&) = delete;
    
    // Core mutex operations
    void lock();
    bool try_lock();
    bool try_lock_for(std::chrono::microseconds timeout);
    void unlock();
    
    // Status queries
    bool is_locked() const noexcept;
    FiberID owner() const noexcept { return owner_.load(std::memory_order_acquire); }
    bool is_owned_by_current_fiber() const noexcept;
    u32 recursion_count() const noexcept { return recursion_count_.load(std::memory_order_acquire); }
    
    // Configuration
    void set_priority_inheritance(bool enable) noexcept { enable_priority_inheritance_ = enable; }
    void set_deadlock_detection(bool enable) noexcept { enable_deadlock_detection_ = enable; }
    void set_max_wait_time(std::chrono::microseconds timeout) { max_wait_time_ = timeout; }
    void set_name(const std::string& name) { debug_name_ = name; }
    
    // Statistics
    struct MutexStats {
        u64 total_acquisitions = 0;
        u64 contentions = 0;
        f64 average_wait_time_us = 0.0;
        f64 contention_ratio = 0.0;
        usize current_waiters = 0;
    };
    
    MutexStats get_statistics() const;
    void reset_statistics();
    const std::string& name() const noexcept { return debug_name_; }
    
private:
    void add_to_wait_queue(WaitNode* node);
    WaitNode* remove_from_wait_queue();
    void wake_next_waiter();
    bool detect_deadlock(FiberID current_fiber) const;
    void handle_priority_inheritance(FiberID waiting_fiber);
};

//=============================================================================
// RAII Lock Guards for Fiber Mutex
//=============================================================================

/**
 * @brief RAII lock guard for FiberMutex
 */
class FiberLockGuard {
private:
    FiberMutex& mutex_;
    bool owns_lock_;
    
public:
    explicit FiberLockGuard(FiberMutex& mutex) : mutex_(mutex), owns_lock_(true) {
        mutex_.lock();
    }
    
    ~FiberLockGuard() {
        if (owns_lock_) {
            mutex_.unlock();
        }
    }
    
    // Non-copyable, non-movable
    FiberLockGuard(const FiberLockGuard&) = delete;
    FiberLockGuard& operator=(const FiberLockGuard&) = delete;
    FiberLockGuard(FiberLockGuard&&) = delete;
    FiberLockGuard& operator=(FiberLockGuard&&) = delete;
    
    void unlock() {
        if (owns_lock_) {
            mutex_.unlock();
            owns_lock_ = false;
        }
    }
    
    bool owns_lock() const noexcept { return owns_lock_; }
};

/**
 * @brief RAII unique lock for FiberMutex with deferred locking
 */
class FiberUniqueLock {
private:
    FiberMutex* mutex_;
    bool owns_lock_;
    
public:
    explicit FiberUniqueLock(FiberMutex& mutex) : mutex_(&mutex), owns_lock_(true) {
        mutex_->lock();
    }
    
    FiberUniqueLock(FiberMutex& mutex, std::defer_lock_t) noexcept
        : mutex_(&mutex), owns_lock_(false) {}
    
    FiberUniqueLock(FiberMutex& mutex, std::try_to_lock_t)
        : mutex_(&mutex), owns_lock_(mutex_->try_lock()) {}
    
    template<typename Rep, typename Period>
    FiberUniqueLock(FiberMutex& mutex, const std::chrono::duration<Rep, Period>& timeout)
        : mutex_(&mutex), owns_lock_(mutex_->try_lock_for(std::chrono::duration_cast<std::chrono::microseconds>(timeout))) {}
    
    ~FiberUniqueLock() {
        if (owns_lock_) {
            mutex_->unlock();
        }
    }
    
    // Movable but not copyable
    FiberUniqueLock(const FiberUniqueLock&) = delete;
    FiberUniqueLock& operator=(const FiberUniqueLock&) = delete;
    
    FiberUniqueLock(FiberUniqueLock&& other) noexcept
        : mutex_(other.mutex_), owns_lock_(other.owns_lock_) {
        other.mutex_ = nullptr;
        other.owns_lock_ = false;
    }
    
    FiberUniqueLock& operator=(FiberUniqueLock&& other) noexcept {
        if (this != &other) {
            if (owns_lock_) {
                mutex_->unlock();
            }
            mutex_ = other.mutex_;
            owns_lock_ = other.owns_lock_;
            other.mutex_ = nullptr;
            other.owns_lock_ = false;
        }
        return *this;
    }
    
    // Lock operations
    void lock() {
        if (!mutex_) throw std::system_error(std::make_error_code(std::errc::operation_not_permitted));
        if (owns_lock_) throw std::system_error(std::make_error_code(std::errc::resource_deadlock_would_occur));
        mutex_->lock();
        owns_lock_ = true;
    }
    
    bool try_lock() {
        if (!mutex_) throw std::system_error(std::make_error_code(std::errc::operation_not_permitted));
        if (owns_lock_) throw std::system_error(std::make_error_code(std::errc::resource_deadlock_would_occur));
        owns_lock_ = mutex_->try_lock();
        return owns_lock_;
    }
    
    template<typename Rep, typename Period>
    bool try_lock_for(const std::chrono::duration<Rep, Period>& timeout) {
        if (!mutex_) throw std::system_error(std::make_error_code(std::errc::operation_not_permitted));
        if (owns_lock_) throw std::system_error(std::make_error_code(std::errc::resource_deadlock_would_occur));
        owns_lock_ = mutex_->try_lock_for(std::chrono::duration_cast<std::chrono::microseconds>(timeout));
        return owns_lock_;
    }
    
    void unlock() {
        if (!owns_lock_) throw std::system_error(std::make_error_code(std::errc::operation_not_permitted));
        mutex_->unlock();
        owns_lock_ = false;
    }
    
    // Accessors
    FiberMutex* mutex() const noexcept { return mutex_; }
    bool owns_lock() const noexcept { return owns_lock_; }
    explicit operator bool() const noexcept { return owns_lock_; }
    
    // Release ownership without unlocking
    FiberMutex* release() noexcept {
        FiberMutex* result = mutex_;
        mutex_ = nullptr;
        owns_lock_ = false;
        return result;
    }
};

//=============================================================================
// Fiber Condition Variable
//=============================================================================

/**
 * @brief Fiber-aware condition variable for cooperative waiting
 */
class FiberConditionVariable {
private:
    struct WaitNode {
        FiberID fiber_id;
        std::chrono::steady_clock::time_point wait_start;
        WaitNode* next = nullptr;
        
        explicit WaitNode(FiberID id) 
            : fiber_id(id), wait_start(std::chrono::steady_clock::now()) {}
    };
    
    std::atomic<WaitNode*> wait_queue_head_{nullptr};
    std::atomic<WaitNode*> wait_queue_tail_{nullptr};
    std::atomic<u64> waiting_count_{0};
    
    // Statistics
    std::atomic<u64> total_waits_{0};
    std::atomic<u64> total_notifications_{0};
    std::atomic<u64> spurious_wakeups_{0};
    
    std::string debug_name_;
    
public:
    explicit FiberConditionVariable(const std::string& name = "");
    ~FiberConditionVariable();
    
    // Non-copyable, non-movable
    FiberConditionVariable(const FiberConditionVariable&) = delete;
    FiberConditionVariable& operator=(const FiberConditionVariable&) = delete;
    FiberConditionVariable(FiberConditionVariable&&) = delete;
    FiberConditionVariable& operator=(FiberConditionVariable&&) = delete;
    
    // Core condition variable operations
    void wait(FiberUniqueLock& lock);
    
    template<typename Predicate>
    void wait(FiberUniqueLock& lock, Predicate pred) {
        while (!pred()) {
            wait(lock);
        }
    }
    
    template<typename Rep, typename Period>
    std::cv_status wait_for(FiberUniqueLock& lock, const std::chrono::duration<Rep, Period>& timeout) {
        return wait_until(lock, std::chrono::steady_clock::now() + timeout);
    }
    
    template<typename Rep, typename Period, typename Predicate>
    bool wait_for(FiberUniqueLock& lock, const std::chrono::duration<Rep, Period>& timeout, Predicate pred) {
        return wait_until(lock, std::chrono::steady_clock::now() + timeout, pred);
    }
    
    template<typename Clock, typename Duration>
    std::cv_status wait_until(FiberUniqueLock& lock, const std::chrono::time_point<Clock, Duration>& timeout_time) {
        auto start_time = std::chrono::steady_clock::now();
        auto timeout_steady = std::chrono::steady_clock::now() + (timeout_time - Clock::now());
        
        wait_until_impl(lock, timeout_steady);
        
        return (std::chrono::steady_clock::now() >= timeout_steady) ? 
               std::cv_status::timeout : std::cv_status::no_timeout;
    }
    
    template<typename Clock, typename Duration, typename Predicate>
    bool wait_until(FiberUniqueLock& lock, const std::chrono::time_point<Clock, Duration>& timeout_time, Predicate pred) {
        while (!pred()) {
            if (wait_until(lock, timeout_time) == std::cv_status::timeout) {
                return pred();
            }
        }
        return true;
    }
    
    // Notification operations
    void notify_one();
    void notify_all();
    
    // Status queries
    usize waiting_count() const noexcept { return waiting_count_.load(std::memory_order_acquire); }
    const std::string& name() const noexcept { return debug_name_; }
    
    // Statistics
    struct ConditionVariableStats {
        u64 total_waits = 0;
        u64 total_notifications = 0;
        u64 spurious_wakeups = 0;
        usize current_waiters = 0;
        f64 spurious_wakeup_ratio = 0.0;
    };
    
    ConditionVariableStats get_statistics() const;
    void reset_statistics();
    
private:
    void wait_until_impl(FiberUniqueLock& lock, std::chrono::steady_clock::time_point timeout_time);
    void add_to_wait_queue(WaitNode* node);
    WaitNode* remove_from_wait_queue();
    WaitNode* remove_specific_from_wait_queue(FiberID fiber_id);
};

//=============================================================================
// Fiber Read-Write Lock
//=============================================================================

/**
 * @brief Fiber-aware read-write lock with writer preference
 */
class FiberReadWriteLock {
private:
    std::atomic<i32> reader_count_{0};  // Positive = readers, -1 = writer
    std::atomic<FiberID> writer_fiber_{FiberID{}};
    
    // Wait queues
    struct WaitNode {
        FiberID fiber_id;
        bool is_writer;
        FiberPriority priority;
        std::chrono::steady_clock::time_point wait_start;
        WaitNode* next = nullptr;
        
        WaitNode(FiberID id, bool writer, FiberPriority prio) 
            : fiber_id(id), is_writer(writer), priority(prio), 
              wait_start(std::chrono::steady_clock::now()) {}
    };
    
    std::atomic<WaitNode*> reader_queue_head_{nullptr};
    std::atomic<WaitNode*> reader_queue_tail_{nullptr};
    std::atomic<WaitNode*> writer_queue_head_{nullptr};
    std::atomic<WaitNode*> writer_queue_tail_{nullptr};
    
    std::atomic<u32> waiting_readers_{0};
    std::atomic<u32> waiting_writers_{0};
    
    // Statistics
    std::atomic<u64> read_acquisitions_{0};
    std::atomic<u64> write_acquisitions_{0};
    std::atomic<u64> read_contentions_{0};
    std::atomic<u64> write_contentions_{0};
    
    std::string debug_name_;
    bool writer_preference_ = true;
    
public:
    explicit FiberReadWriteLock(const std::string& name = "", bool writer_preference = true);
    ~FiberReadWriteLock();
    
    // Non-copyable, non-movable
    FiberReadWriteLock(const FiberReadWriteLock&) = delete;
    FiberReadWriteLock& operator=(const FiberReadWriteLock&) = delete;
    FiberReadWriteLock(FiberReadWriteLock&&) = delete;
    FiberReadWriteLock& operator=(FiberReadWriteLock&&) = delete;
    
    // Reader operations
    void lock_shared();
    bool try_lock_shared();
    bool try_lock_shared_for(std::chrono::microseconds timeout);
    void unlock_shared();
    
    // Writer operations
    void lock();
    bool try_lock();
    bool try_lock_for(std::chrono::microseconds timeout);
    void unlock();
    
    // Status queries
    bool has_readers() const noexcept { return reader_count_.load(std::memory_order_acquire) > 0; }
    bool has_writer() const noexcept { return reader_count_.load(std::memory_order_acquire) < 0; }
    i32 reader_count() const noexcept { 
        i32 count = reader_count_.load(std::memory_order_acquire);
        return count > 0 ? count : 0;
    }
    FiberID writer_fiber() const noexcept { return writer_fiber_.load(std::memory_order_acquire); }
    
    // Configuration
    void set_writer_preference(bool prefer_writers) noexcept { writer_preference_ = prefer_writers; }
    const std::string& name() const noexcept { return debug_name_; }
    
    // Statistics
    struct ReadWriteLockStats {
        u64 read_acquisitions = 0;
        u64 write_acquisitions = 0;
        u64 read_contentions = 0;
        u64 write_contentions = 0;
        u32 waiting_readers = 0;
        u32 waiting_writers = 0;
        f64 read_contention_ratio = 0.0;
        f64 write_contention_ratio = 0.0;
    };
    
    ReadWriteLockStats get_statistics() const;
    void reset_statistics();
    
private:
    void wake_waiting_readers();
    void wake_waiting_writer();
    bool can_acquire_read_lock() const;
    bool can_acquire_write_lock() const;
};

//=============================================================================
// RAII Guards for Read-Write Lock
//=============================================================================

/**
 * @brief RAII shared lock guard for read operations
 */
class FiberSharedLockGuard {
private:
    FiberReadWriteLock& rwlock_;
    bool owns_lock_;
    
public:
    explicit FiberSharedLockGuard(FiberReadWriteLock& rwlock) 
        : rwlock_(rwlock), owns_lock_(true) {
        rwlock_.lock_shared();
    }
    
    ~FiberSharedLockGuard() {
        if (owns_lock_) {
            rwlock_.unlock_shared();
        }
    }
    
    // Non-copyable, non-movable
    FiberSharedLockGuard(const FiberSharedLockGuard&) = delete;
    FiberSharedLockGuard& operator=(const FiberSharedLockGuard&) = delete;
    FiberSharedLockGuard(FiberSharedLockGuard&&) = delete;
    FiberSharedLockGuard& operator=(FiberSharedLockGuard&&) = delete;
    
    void unlock() {
        if (owns_lock_) {
            rwlock_.unlock_shared();
            owns_lock_ = false;
        }
    }
    
    bool owns_lock() const noexcept { return owns_lock_; }
};

//=============================================================================
// Fiber Semaphore
//=============================================================================

/**
 * @brief Fiber-aware counting semaphore
 */
class FiberSemaphore {
private:
    std::atomic<i64> count_;
    const i64 max_count_;
    
    struct WaitNode {
        FiberID fiber_id;
        std::chrono::steady_clock::time_point wait_start;
        WaitNode* next = nullptr;
        
        explicit WaitNode(FiberID id) 
            : fiber_id(id), wait_start(std::chrono::steady_clock::now()) {}
    };
    
    std::atomic<WaitNode*> wait_queue_head_{nullptr};
    std::atomic<WaitNode*> wait_queue_tail_{nullptr};
    std::atomic<u32> waiting_count_{0};
    
    // Statistics
    std::atomic<u64> acquire_count_{0};
    std::atomic<u64> release_count_{0};
    std::atomic<u64> wait_count_{0};
    
    std::string debug_name_;
    
public:
    explicit FiberSemaphore(i64 initial_count, i64 max_count = std::numeric_limits<i64>::max(),
                           const std::string& name = "");
    ~FiberSemaphore();
    
    // Non-copyable, non-movable
    FiberSemaphore(const FiberSemaphore&) = delete;
    FiberSemaphore& operator=(const FiberSemaphore&) = delete;
    FiberSemaphore(FiberSemaphore&&) = delete;
    FiberSemaphore& operator=(FiberSemaphore&&) = delete;
    
    // Core semaphore operations
    void acquire();
    bool try_acquire();
    bool try_acquire_for(std::chrono::microseconds timeout);
    
    void release(i64 count = 1);
    
    // Status queries
    i64 count() const noexcept { return count_.load(std::memory_order_acquire); }
    i64 max_count() const noexcept { return max_count_; }
    u32 waiting_count() const noexcept { return waiting_count_.load(std::memory_order_acquire); }
    
    // Statistics
    struct SemaphoreStats {
        u64 total_acquires = 0;
        u64 total_releases = 0;
        u64 total_waits = 0;
        i64 current_count = 0;
        u32 current_waiters = 0;
        f64 average_wait_time_us = 0.0;
    };
    
    SemaphoreStats get_statistics() const;
    void reset_statistics();
    const std::string& name() const noexcept { return debug_name_; }
    
private:
    void add_to_wait_queue(WaitNode* node);
    WaitNode* remove_from_wait_queue();
    void wake_waiters(i64 wake_count);
};

//=============================================================================
// Fiber Barrier
//=============================================================================

/**
 * @brief Fiber-aware barrier for synchronizing multiple fibers
 */
class FiberBarrier {
private:
    const u32 expected_count_;
    std::atomic<u32> current_count_{0};
    std::atomic<u32> generation_{0};
    
    struct WaitNode {
        FiberID fiber_id;
        u32 generation;
        std::chrono::steady_clock::time_point wait_start;
        WaitNode* next = nullptr;
        
        WaitNode(FiberID id, u32 gen) 
            : fiber_id(id), generation(gen), 
              wait_start(std::chrono::steady_clock::now()) {}
    };
    
    std::atomic<WaitNode*> wait_queue_head_{nullptr};
    std::atomic<WaitNode*> wait_queue_tail_{nullptr};
    
    // Statistics
    std::atomic<u64> barrier_cycles_{0};
    std::atomic<u64> total_waits_{0};
    
    std::string debug_name_;
    
public:
    explicit FiberBarrier(u32 expected_count, const std::string& name = "");
    ~FiberBarrier();
    
    // Non-copyable, non-movable
    FiberBarrier(const FiberBarrier&) = delete;
    FiberBarrier& operator=(const FiberBarrier&) = delete;
    FiberBarrier(FiberBarrier&&) = delete;
    FiberBarrier& operator=(FiberBarrier&&) = delete;
    
    // Core barrier operations
    void wait();
    bool try_wait();
    bool wait_for(std::chrono::microseconds timeout);
    
    // Status queries
    u32 expected_count() const noexcept { return expected_count_; }
    u32 current_count() const noexcept { return current_count_.load(std::memory_order_acquire); }
    u32 remaining_count() const noexcept { return expected_count_ - current_count(); }
    u32 generation() const noexcept { return generation_.load(std::memory_order_acquire); }
    
    // Statistics
    struct BarrierStats {
        u64 total_cycles = 0;
        u64 total_waits = 0;
        u32 current_waiters = 0;
        u32 current_generation = 0;
        f64 average_wait_time_us = 0.0;
    };
    
    BarrierStats get_statistics() const;
    void reset_statistics();
    const std::string& name() const noexcept { return debug_name_; }
    
private:
    void add_to_wait_queue(WaitNode* node);
    void wake_all_waiters();
};

} // namespace ecscope::jobs