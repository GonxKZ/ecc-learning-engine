#include "jobs/fiber_sync.hpp"
#include "jobs/fiber.hpp"
#include <algorithm>
#include <chrono>
#include <thread>

namespace ecscope::jobs {

//=============================================================================
// Fiber Mutex Implementation
//=============================================================================

FiberMutex::FiberMutex(const std::string& name)
    : owner_(FiberID{})
    , recursion_count_(0)
    , wait_queue_head_(nullptr)
    , wait_queue_tail_(nullptr)
    , acquire_count_(0)
    , contention_count_(0)
    , total_wait_time_us_(0)
    , debug_name_(name.empty() ? "FiberMutex" : name)
    , enable_priority_inheritance_(true)
    , enable_deadlock_detection_(true)
    , max_wait_time_(std::chrono::seconds{10}) {
}

FiberMutex::~FiberMutex() {
    // Clean up any remaining wait nodes
    WaitNode* current = wait_queue_head_.load(std::memory_order_acquire);
    while (current) {
        WaitNode* next = current->next;
        delete current;
        current = next;
    }
}

void FiberMutex::lock() {
    FiberID current_fiber_id = FiberUtils::current_fiber_id();
    
    if (!current_fiber_id.is_valid()) {
        // Not in a fiber context - this shouldn't happen in fiber-based system
        throw std::runtime_error("FiberMutex::lock() called outside of fiber context");
    }
    
    acquire_count_.fetch_add(1, std::memory_order_relaxed);
    
    // Try fast path - uncontended lock
    FiberID expected = FiberID{};
    if (owner_.compare_exchange_weak(expected, current_fiber_id, std::memory_order_acquire)) {
        recursion_count_.store(1, std::memory_order_relaxed);
        return;
    }
    
    // Check for recursive lock
    if (expected == current_fiber_id) {
        recursion_count_.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    
    // Contended path - need to wait
    contention_count_.fetch_add(1, std::memory_order_relaxed);
    
    // Deadlock detection
    if (enable_deadlock_detection_ && detect_deadlock(current_fiber_id)) {
        throw std::runtime_error("Potential deadlock detected in FiberMutex: " + debug_name_);
    }
    
    // Create wait node and add to queue
    auto wait_node = std::make_unique<WaitNode>(current_fiber_id, FiberPriority::Normal);
    WaitNode* node_ptr = wait_node.release();
    
    add_to_wait_queue(node_ptr);
    
    // Priority inheritance
    if (enable_priority_inheritance_) {
        handle_priority_inheritance(current_fiber_id);
    }
    
    // Wait for the lock
    auto start_wait = std::chrono::steady_clock::now();
    
    while (true) {
        // Check timeout
        auto elapsed = std::chrono::steady_clock::now() - start_wait;
        if (elapsed >= max_wait_time_) {
            // Remove from wait queue and throw timeout exception
            remove_from_wait_queue();
            delete node_ptr;
            throw std::runtime_error("FiberMutex lock timeout: " + debug_name_);
        }
        
        // Try to acquire the lock
        expected = FiberID{};
        if (owner_.compare_exchange_weak(expected, current_fiber_id, std::memory_order_acquire)) {
            recursion_count_.store(1, std::memory_order_relaxed);
            
            // Update wait time statistics
            auto wait_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - start_wait).count();
            total_wait_time_us_.fetch_add(wait_time_us, std::memory_order_relaxed);
            
            delete node_ptr;
            return;
        }
        
        // Yield to other fibers
        FiberUtils::yield();
    }
}

bool FiberMutex::try_lock() {
    FiberID current_fiber_id = FiberUtils::current_fiber_id();
    
    if (!current_fiber_id.is_valid()) {
        return false;
    }
    
    acquire_count_.fetch_add(1, std::memory_order_relaxed);
    
    // Try to acquire lock atomically
    FiberID expected = FiberID{};
    if (owner_.compare_exchange_weak(expected, current_fiber_id, std::memory_order_acquire)) {
        recursion_count_.store(1, std::memory_order_relaxed);
        return true;
    }
    
    // Check for recursive lock
    if (expected == current_fiber_id) {
        recursion_count_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
    
    return false;
}

bool FiberMutex::try_lock_for(std::chrono::microseconds timeout) {
    FiberID current_fiber_id = FiberUtils::current_fiber_id();
    
    if (!current_fiber_id.is_valid()) {
        return false;
    }
    
    acquire_count_.fetch_add(1, std::memory_order_relaxed);
    
    auto start_time = std::chrono::steady_clock::now();
    
    while (std::chrono::steady_clock::now() - start_time < timeout) {
        FiberID expected = FiberID{};
        if (owner_.compare_exchange_weak(expected, current_fiber_id, std::memory_order_acquire)) {
            recursion_count_.store(1, std::memory_order_relaxed);
            return true;
        }
        
        // Check for recursive lock
        if (expected == current_fiber_id) {
            recursion_count_.fetch_add(1, std::memory_order_relaxed);
            return true;
        }
        
        FiberUtils::yield();
    }
    
    return false;
}

void FiberMutex::unlock() {
    FiberID current_fiber_id = FiberUtils::current_fiber_id();
    FiberID expected_owner = owner_.load(std::memory_order_acquire);
    
    if (expected_owner != current_fiber_id) {
        throw std::runtime_error("FiberMutex::unlock() called by non-owner fiber");
    }
    
    u32 current_recursion = recursion_count_.load(std::memory_order_acquire);
    if (current_recursion > 1) {
        recursion_count_.fetch_sub(1, std::memory_order_relaxed);
        return;
    }
    
    // Release the lock
    recursion_count_.store(0, std::memory_order_relaxed);
    owner_.store(FiberID{}, std::memory_order_release);
    
    // Wake up next waiter
    wake_next_waiter();
}

bool FiberMutex::is_locked() const noexcept {
    return owner_.load(std::memory_order_acquire).is_valid();
}

bool FiberMutex::is_owned_by_current_fiber() const noexcept {
    FiberID current_fiber_id = FiberUtils::current_fiber_id();
    return owner_.load(std::memory_order_acquire) == current_fiber_id;
}

FiberMutex::MutexStats FiberMutex::get_statistics() const {
    MutexStats stats;
    stats.total_acquisitions = acquire_count_.load(std::memory_order_relaxed);
    stats.contentions = contention_count_.load(std::memory_order_relaxed);
    
    u64 total_wait_time = total_wait_time_us_.load(std::memory_order_relaxed);
    if (stats.contentions > 0) {
        stats.average_wait_time_us = static_cast<f64>(total_wait_time) / static_cast<f64>(stats.contentions);
        stats.contention_ratio = static_cast<f64>(stats.contentions) / static_cast<f64>(stats.total_acquisitions);
    }
    
    // Count current waiters
    stats.current_waiters = 0;
    WaitNode* current = wait_queue_head_.load(std::memory_order_acquire);
    while (current) {
        stats.current_waiters++;
        current = current->next;
    }
    
    return stats;
}

void FiberMutex::reset_statistics() {
    acquire_count_.store(0, std::memory_order_relaxed);
    contention_count_.store(0, std::memory_order_relaxed);
    total_wait_time_us_.store(0, std::memory_order_relaxed);
}

void FiberMutex::add_to_wait_queue(WaitNode* node) {
    node->next = nullptr;
    
    WaitNode* expected_tail = wait_queue_tail_.load(std::memory_order_acquire);
    
    while (true) {
        if (expected_tail == nullptr) {
            // Queue is empty
            if (wait_queue_head_.compare_exchange_weak(expected_tail, node, std::memory_order_release) &&
                wait_queue_tail_.compare_exchange_weak(expected_tail, node, std::memory_order_release)) {
                break;
            }
        } else {
            // Queue has elements
            if (wait_queue_tail_.compare_exchange_weak(expected_tail, node, std::memory_order_release)) {
                expected_tail->next = node;
                break;
            }
        }
        expected_tail = wait_queue_tail_.load(std::memory_order_acquire);
    }
}

FiberMutex::WaitNode* FiberMutex::remove_from_wait_queue() {
    WaitNode* head = wait_queue_head_.load(std::memory_order_acquire);
    
    if (!head) {
        return nullptr;
    }
    
    if (wait_queue_head_.compare_exchange_strong(head, head->next, std::memory_order_release)) {
        if (head->next == nullptr) {
            wait_queue_tail_.store(nullptr, std::memory_order_release);
        }
        return head;
    }
    
    return nullptr;
}

void FiberMutex::wake_next_waiter() {
    // Simply removing from queue will allow the waiting fiber to retry
    // The actual wakeup happens when the waiting fiber yields and retries
}

bool FiberMutex::detect_deadlock(FiberID current_fiber) const {
    // Simple deadlock detection - check if current fiber is already waiting
    // on another mutex that might create a cycle
    // This is a simplified implementation - a full deadlock detector would
    // maintain a wait-for graph
    
    (void)current_fiber;  // Suppress unused parameter warning
    
    // For now, just check if we've been waiting too long
    return false;  // Simplified - always return false
}

void FiberMutex::handle_priority_inheritance(FiberID waiting_fiber) {
    // Priority inheritance implementation would boost the priority of the
    // lock-holding fiber to the priority of the highest-priority waiting fiber
    
    (void)waiting_fiber;  // Suppress unused parameter warning
    
    // Simplified implementation - actual priority inheritance would require
    // integration with the fiber scheduler
}

//=============================================================================
// Fiber Condition Variable Implementation
//=============================================================================

FiberConditionVariable::FiberConditionVariable(const std::string& name)
    : wait_queue_head_(nullptr)
    , wait_queue_tail_(nullptr)
    , waiting_count_(0)
    , total_waits_(0)
    , total_notifications_(0)
    , spurious_wakeups_(0)
    , debug_name_(name.empty() ? "FiberConditionVariable" : name) {
}

FiberConditionVariable::~FiberConditionVariable() {
    // Clean up any remaining wait nodes
    WaitNode* current = wait_queue_head_.load(std::memory_order_acquire);
    while (current) {
        WaitNode* next = current->next;
        delete current;
        current = next;
    }
}

void FiberConditionVariable::wait(FiberUniqueLock& lock) {
    if (!lock.owns_lock()) {
        throw std::system_error(std::make_error_code(std::errc::operation_not_permitted));
    }
    
    FiberID current_fiber_id = FiberUtils::current_fiber_id();
    if (!current_fiber_id.is_valid()) {
        throw std::runtime_error("FiberConditionVariable::wait() called outside of fiber context");
    }
    
    total_waits_.fetch_add(1, std::memory_order_relaxed);
    waiting_count_.fetch_add(1, std::memory_order_relaxed);
    
    // Add to wait queue
    auto wait_node = std::make_unique<WaitNode>(current_fiber_id);
    WaitNode* node_ptr = wait_node.release();
    add_to_wait_queue(node_ptr);
    
    // Unlock the mutex
    lock.unlock();
    
    // Wait (yield until notified)
    while (true) {
        // Check if we've been removed from the queue (i.e., notified)
        bool found_in_queue = false;
        WaitNode* current = wait_queue_head_.load(std::memory_order_acquire);
        while (current) {
            if (current->fiber_id == current_fiber_id) {
                found_in_queue = true;
                break;
            }
            current = current->next;
        }
        
        if (!found_in_queue) {
            // We've been notified
            break;
        }
        
        FiberUtils::yield();
    }
    
    waiting_count_.fetch_sub(1, std::memory_order_relaxed);
    delete node_ptr;
    
    // Re-acquire the lock
    lock.lock();
}

void FiberConditionVariable::wait_until_impl(FiberUniqueLock& lock, 
                                            std::chrono::steady_clock::time_point timeout_time) {
    if (!lock.owns_lock()) {
        throw std::system_error(std::make_error_code(std::errc::operation_not_permitted));
    }
    
    FiberID current_fiber_id = FiberUtils::current_fiber_id();
    if (!current_fiber_id.is_valid()) {
        throw std::runtime_error("FiberConditionVariable::wait_until() called outside of fiber context");
    }
    
    total_waits_.fetch_add(1, std::memory_order_relaxed);
    waiting_count_.fetch_add(1, std::memory_order_relaxed);
    
    // Add to wait queue
    auto wait_node = std::make_unique<WaitNode>(current_fiber_id);
    WaitNode* node_ptr = wait_node.release();
    add_to_wait_queue(node_ptr);
    
    // Unlock the mutex
    lock.unlock();
    
    // Wait with timeout
    bool notified = false;
    while (std::chrono::steady_clock::now() < timeout_time) {
        // Check if we've been removed from the queue (i.e., notified)
        bool found_in_queue = false;
        WaitNode* current = wait_queue_head_.load(std::memory_order_acquire);
        while (current) {
            if (current->fiber_id == current_fiber_id) {
                found_in_queue = true;
                break;
            }
            current = current->next;
        }
        
        if (!found_in_queue) {
            // We've been notified
            notified = true;
            break;
        }
        
        FiberUtils::yield();
    }
    
    if (!notified) {
        // Timeout occurred - remove ourselves from the queue
        remove_specific_from_wait_queue(current_fiber_id);
    }
    
    waiting_count_.fetch_sub(1, std::memory_order_relaxed);
    delete node_ptr;
    
    // Re-acquire the lock
    lock.lock();
}

void FiberConditionVariable::notify_one() {
    total_notifications_.fetch_add(1, std::memory_order_relaxed);
    
    WaitNode* node = remove_from_wait_queue();
    if (node) {
        // The waiting fiber will detect it's been removed from the queue
        // when it yields and checks again
    }
}

void FiberConditionVariable::notify_all() {
    total_notifications_.fetch_add(1, std::memory_order_relaxed);
    
    // Remove all nodes from the queue
    while (WaitNode* node = remove_from_wait_queue()) {
        // Each waiting fiber will detect it's been removed from the queue
    }
}

FiberConditionVariable::ConditionVariableStats FiberConditionVariable::get_statistics() const {
    ConditionVariableStats stats;
    stats.total_waits = total_waits_.load(std::memory_order_relaxed);
    stats.total_notifications = total_notifications_.load(std::memory_order_relaxed);
    stats.spurious_wakeups = spurious_wakeups_.load(std::memory_order_relaxed);
    stats.current_waiters = waiting_count_.load(std::memory_order_relaxed);
    
    if (stats.total_waits > 0) {
        stats.spurious_wakeup_ratio = static_cast<f64>(stats.spurious_wakeups) / 
                                     static_cast<f64>(stats.total_waits);
    }
    
    return stats;
}

void FiberConditionVariable::reset_statistics() {
    total_waits_.store(0, std::memory_order_relaxed);
    total_notifications_.store(0, std::memory_order_relaxed);
    spurious_wakeups_.store(0, std::memory_order_relaxed);
}

void FiberConditionVariable::add_to_wait_queue(WaitNode* node) {
    node->next = nullptr;
    
    WaitNode* expected_tail = wait_queue_tail_.load(std::memory_order_acquire);
    
    while (true) {
        if (expected_tail == nullptr) {
            // Queue is empty
            if (wait_queue_head_.compare_exchange_weak(expected_tail, node, std::memory_order_release) &&
                wait_queue_tail_.compare_exchange_weak(expected_tail, node, std::memory_order_release)) {
                break;
            }
        } else {
            // Queue has elements
            if (wait_queue_tail_.compare_exchange_weak(expected_tail, node, std::memory_order_release)) {
                expected_tail->next = node;
                break;
            }
        }
        expected_tail = wait_queue_tail_.load(std::memory_order_acquire);
    }
}

FiberConditionVariable::WaitNode* FiberConditionVariable::remove_from_wait_queue() {
    WaitNode* head = wait_queue_head_.load(std::memory_order_acquire);
    
    if (!head) {
        return nullptr;
    }
    
    if (wait_queue_head_.compare_exchange_strong(head, head->next, std::memory_order_release)) {
        if (head->next == nullptr) {
            wait_queue_tail_.store(nullptr, std::memory_order_release);
        }
        return head;
    }
    
    return nullptr;
}

FiberConditionVariable::WaitNode* FiberConditionVariable::remove_specific_from_wait_queue(FiberID fiber_id) {
    // This is a simplified implementation - for better performance,
    // we could use a more sophisticated data structure
    
    WaitNode* prev = nullptr;
    WaitNode* current = wait_queue_head_.load(std::memory_order_acquire);
    
    while (current) {
        if (current->fiber_id == fiber_id) {
            if (prev) {
                prev->next = current->next;
            } else {
                wait_queue_head_.store(current->next, std::memory_order_release);
            }
            
            if (current->next == nullptr) {
                wait_queue_tail_.store(prev, std::memory_order_release);
            }
            
            return current;
        }
        prev = current;
        current = current->next;
    }
    
    return nullptr;
}

//=============================================================================
// Fiber Read-Write Lock Implementation
//=============================================================================

FiberReadWriteLock::FiberReadWriteLock(const std::string& name, bool writer_preference)
    : reader_count_(0)
    , writer_fiber_(FiberID{})
    , reader_queue_head_(nullptr)
    , reader_queue_tail_(nullptr)
    , writer_queue_head_(nullptr)
    , writer_queue_tail_(nullptr)
    , waiting_readers_(0)
    , waiting_writers_(0)
    , read_acquisitions_(0)
    , write_acquisitions_(0)
    , read_contentions_(0)
    , write_contentions_(0)
    , debug_name_(name.empty() ? "FiberReadWriteLock" : name)
    , writer_preference_(writer_preference) {
}

FiberReadWriteLock::~FiberReadWriteLock() {
    // Clean up wait queues
    WaitNode* current = reader_queue_head_.load(std::memory_order_acquire);
    while (current) {
        WaitNode* next = current->next;
        delete current;
        current = next;
    }
    
    current = writer_queue_head_.load(std::memory_order_acquire);
    while (current) {
        WaitNode* next = current->next;
        delete current;
        current = next;
    }
}

void FiberReadWriteLock::lock_shared() {
    FiberID current_fiber_id = FiberUtils::current_fiber_id();
    if (!current_fiber_id.is_valid()) {
        throw std::runtime_error("FiberReadWriteLock::lock_shared() called outside of fiber context");
    }
    
    read_acquisitions_.fetch_add(1, std::memory_order_relaxed);
    
    while (true) {
        if (can_acquire_read_lock()) {
            i32 expected = reader_count_.load(std::memory_order_acquire);
            if (expected >= 0 && reader_count_.compare_exchange_weak(
                expected, expected + 1, std::memory_order_acquire)) {
                return;
            }
        }
        
        // Need to wait
        read_contentions_.fetch_add(1, std::memory_order_relaxed);
        waiting_readers_.fetch_add(1, std::memory_order_relaxed);
        
        // Add to reader wait queue
        auto wait_node = std::make_unique<WaitNode>(current_fiber_id, false, FiberPriority::Normal);
        // ... (similar queue management as in mutex)
        
        FiberUtils::yield();
        
        waiting_readers_.fetch_sub(1, std::memory_order_relaxed);
    }
}

bool FiberReadWriteLock::try_lock_shared() {
    if (can_acquire_read_lock()) {
        i32 expected = reader_count_.load(std::memory_order_acquire);
        if (expected >= 0 && reader_count_.compare_exchange_weak(
            expected, expected + 1, std::memory_order_acquire)) {
            read_acquisitions_.fetch_add(1, std::memory_order_relaxed);
            return true;
        }
    }
    return false;
}

bool FiberReadWriteLock::try_lock_shared_for(std::chrono::microseconds timeout) {
    auto start_time = std::chrono::steady_clock::now();
    
    while (std::chrono::steady_clock::now() - start_time < timeout) {
        if (try_lock_shared()) {
            return true;
        }
        FiberUtils::yield();
    }
    
    return false;
}

void FiberReadWriteLock::unlock_shared() {
    i32 current_count = reader_count_.fetch_sub(1, std::memory_order_release);
    
    if (current_count == 1) {
        // Last reader - wake up waiting writers
        wake_waiting_writer();
    }
}

void FiberReadWriteLock::lock() {
    FiberID current_fiber_id = FiberUtils::current_fiber_id();
    if (!current_fiber_id.is_valid()) {
        throw std::runtime_error("FiberReadWriteLock::lock() called outside of fiber context");
    }
    
    write_acquisitions_.fetch_add(1, std::memory_order_relaxed);
    
    while (true) {
        if (can_acquire_write_lock()) {
            i32 expected = 0;
            if (reader_count_.compare_exchange_weak(expected, -1, std::memory_order_acquire)) {
                writer_fiber_.store(current_fiber_id, std::memory_order_release);
                return;
            }
        }
        
        // Need to wait
        write_contentions_.fetch_add(1, std::memory_order_relaxed);
        waiting_writers_.fetch_add(1, std::memory_order_relaxed);
        
        // Add to writer wait queue
        auto wait_node = std::make_unique<WaitNode>(current_fiber_id, true, FiberPriority::Normal);
        // ... (similar queue management as in mutex)
        
        FiberUtils::yield();
        
        waiting_writers_.fetch_sub(1, std::memory_order_relaxed);
    }
}

bool FiberReadWriteLock::try_lock() {
    FiberID current_fiber_id = FiberUtils::current_fiber_id();
    
    if (can_acquire_write_lock()) {
        i32 expected = 0;
        if (reader_count_.compare_exchange_weak(expected, -1, std::memory_order_acquire)) {
            writer_fiber_.store(current_fiber_id, std::memory_order_release);
            write_acquisitions_.fetch_add(1, std::memory_order_relaxed);
            return true;
        }
    }
    
    return false;
}

bool FiberReadWriteLock::try_lock_for(std::chrono::microseconds timeout) {
    auto start_time = std::chrono::steady_clock::now();
    
    while (std::chrono::steady_clock::now() - start_time < timeout) {
        if (try_lock()) {
            return true;
        }
        FiberUtils::yield();
    }
    
    return false;
}

void FiberReadWriteLock::unlock() {
    FiberID current_fiber_id = FiberUtils::current_fiber_id();
    FiberID expected_writer = writer_fiber_.load(std::memory_order_acquire);
    
    if (expected_writer != current_fiber_id) {
        throw std::runtime_error("FiberReadWriteLock::unlock() called by non-owner fiber");
    }
    
    writer_fiber_.store(FiberID{}, std::memory_order_release);
    reader_count_.store(0, std::memory_order_release);
    
    // Wake up waiting readers or writers based on preference
    if (writer_preference_) {
        if (!wake_waiting_writer()) {
            wake_waiting_readers();
        }
    } else {
        wake_waiting_readers();
        wake_waiting_writer();
    }
}

FiberReadWriteLock::ReadWriteLockStats FiberReadWriteLock::get_statistics() const {
    ReadWriteLockStats stats;
    stats.read_acquisitions = read_acquisitions_.load(std::memory_order_relaxed);
    stats.write_acquisitions = write_acquisitions_.load(std::memory_order_relaxed);
    stats.read_contentions = read_contentions_.load(std::memory_order_relaxed);
    stats.write_contentions = write_contentions_.load(std::memory_order_relaxed);
    stats.waiting_readers = waiting_readers_.load(std::memory_order_relaxed);
    stats.waiting_writers = waiting_writers_.load(std::memory_order_relaxed);
    
    if (stats.read_acquisitions > 0) {
        stats.read_contention_ratio = static_cast<f64>(stats.read_contentions) / 
                                     static_cast<f64>(stats.read_acquisitions);
    }
    
    if (stats.write_acquisitions > 0) {
        stats.write_contention_ratio = static_cast<f64>(stats.write_contentions) / 
                                      static_cast<f64>(stats.write_acquisitions);
    }
    
    return stats;
}

void FiberReadWriteLock::reset_statistics() {
    read_acquisitions_.store(0, std::memory_order_relaxed);
    write_acquisitions_.store(0, std::memory_order_relaxed);
    read_contentions_.store(0, std::memory_order_relaxed);
    write_contentions_.store(0, std::memory_order_relaxed);
}

void FiberReadWriteLock::wake_waiting_readers() {
    // Wake all waiting readers
    while (WaitNode* node = remove_reader_from_wait_queue()) {
        // Readers will retry when they yield
    }
}

void FiberReadWriteLock::wake_waiting_writer() {
    // Wake one waiting writer
    WaitNode* node = remove_writer_from_wait_queue();
    if (node) {
        // Writer will retry when it yields
        return true;
    }
    return false;
}

bool FiberReadWriteLock::can_acquire_read_lock() const {
    i32 count = reader_count_.load(std::memory_order_acquire);
    
    if (count < 0) {
        // Writer holds the lock
        return false;
    }
    
    if (writer_preference_ && waiting_writers_.load(std::memory_order_acquire) > 0) {
        // Writer preference - don't allow new readers if writers are waiting
        return false;
    }
    
    return true;
}

bool FiberReadWriteLock::can_acquire_write_lock() const {
    i32 count = reader_count_.load(std::memory_order_acquire);
    return count == 0;  // No readers or writers
}

//=============================================================================
// Fiber Semaphore Implementation
//=============================================================================

FiberSemaphore::FiberSemaphore(i64 initial_count, i64 max_count, const std::string& name)
    : count_(initial_count)
    , max_count_(max_count)
    , wait_queue_head_(nullptr)
    , wait_queue_tail_(nullptr)
    , waiting_count_(0)
    , acquire_count_(0)
    , release_count_(0)
    , wait_count_(0)
    , debug_name_(name.empty() ? "FiberSemaphore" : name) {
    
    if (initial_count < 0 || initial_count > max_count) {
        throw std::invalid_argument("Invalid semaphore initial count");
    }
}

FiberSemaphore::~FiberSemaphore() {
    // Clean up wait queue
    WaitNode* current = wait_queue_head_.load(std::memory_order_acquire);
    while (current) {
        WaitNode* next = current->next;
        delete current;
        current = next;
    }
}

void FiberSemaphore::acquire() {
    acquire_count_.fetch_add(1, std::memory_order_relaxed);
    
    while (true) {
        i64 current_count = count_.load(std::memory_order_acquire);
        if (current_count > 0) {
            if (count_.compare_exchange_weak(current_count, current_count - 1, std::memory_order_acquire)) {
                return;
            }
        } else {
            // Need to wait
            wait_count_.fetch_add(1, std::memory_order_relaxed);
            waiting_count_.fetch_add(1, std::memory_order_relaxed);
            
            FiberID current_fiber_id = FiberUtils::current_fiber_id();
            auto wait_node = std::make_unique<WaitNode>(current_fiber_id);
            WaitNode* node_ptr = wait_node.release();
            
            add_to_wait_queue(node_ptr);
            
            // Wait until signaled
            while (true) {
                // Check if we can acquire now
                i64 current_count = count_.load(std::memory_order_acquire);
                if (current_count > 0 && count_.compare_exchange_weak(
                    current_count, current_count - 1, std::memory_order_acquire)) {
                    waiting_count_.fetch_sub(1, std::memory_order_relaxed);
                    delete node_ptr;
                    return;
                }
                
                FiberUtils::yield();
            }
        }
    }
}

bool FiberSemaphore::try_acquire() {
    acquire_count_.fetch_add(1, std::memory_order_relaxed);
    
    i64 current_count = count_.load(std::memory_order_acquire);
    if (current_count > 0) {
        return count_.compare_exchange_strong(current_count, current_count - 1, std::memory_order_acquire);
    }
    
    return false;
}

bool FiberSemaphore::try_acquire_for(std::chrono::microseconds timeout) {
    acquire_count_.fetch_add(1, std::memory_order_relaxed);
    
    auto start_time = std::chrono::steady_clock::now();
    
    while (std::chrono::steady_clock::now() - start_time < timeout) {
        i64 current_count = count_.load(std::memory_order_acquire);
        if (current_count > 0 && count_.compare_exchange_weak(
            current_count, current_count - 1, std::memory_order_acquire)) {
            return true;
        }
        
        FiberUtils::yield();
    }
    
    return false;
}

void FiberSemaphore::release(i64 release_count) {
    if (release_count <= 0) {
        return;
    }
    
    release_count_.fetch_add(release_count, std::memory_order_relaxed);
    
    i64 current_count = count_.load(std::memory_order_acquire);
    i64 new_count = std::min(current_count + release_count, max_count_);
    
    while (!count_.compare_exchange_weak(current_count, new_count, std::memory_order_release)) {
        new_count = std::min(current_count + release_count, max_count_);
    }
    
    // Wake waiting fibers
    i64 wake_count = new_count - current_count;
    wake_waiters(wake_count);
}

FiberSemaphore::SemaphoreStats FiberSemaphore::get_statistics() const {
    SemaphoreStats stats;
    stats.total_acquires = acquire_count_.load(std::memory_order_relaxed);
    stats.total_releases = release_count_.load(std::memory_order_relaxed);
    stats.total_waits = wait_count_.load(std::memory_order_relaxed);
    stats.current_count = count_.load(std::memory_order_relaxed);
    stats.current_waiters = waiting_count_.load(std::memory_order_relaxed);
    
    // Calculate average wait time (simplified)
    stats.average_wait_time_us = 0.0;  // Would need more detailed timing tracking
    
    return stats;
}

void FiberSemaphore::reset_statistics() {
    acquire_count_.store(0, std::memory_order_relaxed);
    release_count_.store(0, std::memory_order_relaxed);
    wait_count_.store(0, std::memory_order_relaxed);
}

void FiberSemaphore::add_to_wait_queue(WaitNode* node) {
    // Similar implementation to other synchronization primitives
    node->next = nullptr;
    
    WaitNode* expected_tail = wait_queue_tail_.load(std::memory_order_acquire);
    
    while (true) {
        if (expected_tail == nullptr) {
            if (wait_queue_head_.compare_exchange_weak(expected_tail, node, std::memory_order_release) &&
                wait_queue_tail_.compare_exchange_weak(expected_tail, node, std::memory_order_release)) {
                break;
            }
        } else {
            if (wait_queue_tail_.compare_exchange_weak(expected_tail, node, std::memory_order_release)) {
                expected_tail->next = node;
                break;
            }
        }
        expected_tail = wait_queue_tail_.load(std::memory_order_acquire);
    }
}

FiberSemaphore::WaitNode* FiberSemaphore::remove_from_wait_queue() {
    WaitNode* head = wait_queue_head_.load(std::memory_order_acquire);
    
    if (!head) {
        return nullptr;
    }
    
    if (wait_queue_head_.compare_exchange_strong(head, head->next, std::memory_order_release)) {
        if (head->next == nullptr) {
            wait_queue_tail_.store(nullptr, std::memory_order_release);
        }
        return head;
    }
    
    return nullptr;
}

void FiberSemaphore::wake_waiters(i64 wake_count) {
    for (i64 i = 0; i < wake_count; ++i) {
        if (WaitNode* node = remove_from_wait_queue()) {
            // Waiting fibers will retry acquisition
        } else {
            break;  // No more waiters
        }
    }
}

//=============================================================================
// Fiber Barrier Implementation
//=============================================================================

FiberBarrier::FiberBarrier(u32 expected_count, const std::string& name)
    : expected_count_(expected_count)
    , current_count_(0)
    , generation_(0)
    , wait_queue_head_(nullptr)
    , wait_queue_tail_(nullptr)
    , barrier_cycles_(0)
    , total_waits_(0)
    , debug_name_(name.empty() ? "FiberBarrier" : name) {
    
    if (expected_count == 0) {
        throw std::invalid_argument("Barrier expected count must be greater than 0");
    }
}

FiberBarrier::~FiberBarrier() {
    // Clean up wait queue
    WaitNode* current = wait_queue_head_.load(std::memory_order_acquire);
    while (current) {
        WaitNode* next = current->next;
        delete current;
        current = next;
    }
}

void FiberBarrier::wait() {
    FiberID current_fiber_id = FiberUtils::current_fiber_id();
    if (!current_fiber_id.is_valid()) {
        throw std::runtime_error("FiberBarrier::wait() called outside of fiber context");
    }
    
    total_waits_.fetch_add(1, std::memory_order_relaxed);
    
    u32 current_generation = generation_.load(std::memory_order_acquire);
    u32 count = current_count_.fetch_add(1, std::memory_order_acq_rel) + 1;
    
    if (count == expected_count_) {
        // Last fiber to reach barrier - wake everyone up
        current_count_.store(0, std::memory_order_release);
        generation_.fetch_add(1, std::memory_order_release);
        barrier_cycles_.fetch_add(1, std::memory_order_relaxed);
        
        wake_all_waiters();
        return;
    }
    
    // Not the last fiber - wait for barrier to complete
    auto wait_node = std::make_unique<WaitNode>(current_fiber_id, current_generation);
    WaitNode* node_ptr = wait_node.release();
    
    add_to_wait_queue(node_ptr);
    
    // Wait until generation changes
    while (generation_.load(std::memory_order_acquire) == current_generation) {
        FiberUtils::yield();
    }
    
    delete node_ptr;
}

bool FiberBarrier::try_wait() {
    u32 count = current_count_.load(std::memory_order_acquire);
    
    if (count + 1 == expected_count_) {
        // Would be the last fiber - proceed with wait
        wait();
        return true;
    }
    
    return false;  // Would have to wait
}

bool FiberBarrier::wait_for(std::chrono::microseconds timeout) {
    auto start_time = std::chrono::steady_clock::now();
    
    FiberID current_fiber_id = FiberUtils::current_fiber_id();
    if (!current_fiber_id.is_valid()) {
        return false;
    }
    
    total_waits_.fetch_add(1, std::memory_order_relaxed);
    
    u32 current_generation = generation_.load(std::memory_order_acquire);
    u32 count = current_count_.fetch_add(1, std::memory_order_acq_rel) + 1;
    
    if (count == expected_count_) {
        // Last fiber to reach barrier
        current_count_.store(0, std::memory_order_release);
        generation_.fetch_add(1, std::memory_order_release);
        barrier_cycles_.fetch_add(1, std::memory_order_relaxed);
        
        wake_all_waiters();
        return true;
    }
    
    // Wait with timeout
    while (generation_.load(std::memory_order_acquire) == current_generation) {
        if (std::chrono::steady_clock::now() - start_time >= timeout) {
            // Timeout - decrement count and return false
            current_count_.fetch_sub(1, std::memory_order_acq_rel);
            return false;
        }
        
        FiberUtils::yield();
    }
    
    return true;
}

FiberBarrier::BarrierStats FiberBarrier::get_statistics() const {
    BarrierStats stats;
    stats.total_cycles = barrier_cycles_.load(std::memory_order_relaxed);
    stats.total_waits = total_waits_.load(std::memory_order_relaxed);
    stats.current_generation = generation_.load(std::memory_order_relaxed);
    
    // Count current waiters
    stats.current_waiters = 0;
    WaitNode* current = wait_queue_head_.load(std::memory_order_acquire);
    u32 current_gen = stats.current_generation;
    while (current) {
        if (current->generation == current_gen) {
            stats.current_waiters++;
        }
        current = current->next;
    }
    
    // Calculate average wait time (simplified)
    stats.average_wait_time_us = 0.0;  // Would need more detailed timing tracking
    
    return stats;
}

void FiberBarrier::reset_statistics() {
    barrier_cycles_.store(0, std::memory_order_relaxed);
    total_waits_.store(0, std::memory_order_relaxed);
}

void FiberBarrier::add_to_wait_queue(WaitNode* node) {
    // Similar implementation to other synchronization primitives
    node->next = nullptr;
    
    WaitNode* expected_tail = wait_queue_tail_.load(std::memory_order_acquire);
    
    while (true) {
        if (expected_tail == nullptr) {
            if (wait_queue_head_.compare_exchange_weak(expected_tail, node, std::memory_order_release) &&
                wait_queue_tail_.compare_exchange_weak(expected_tail, node, std::memory_order_release)) {
                break;
            }
        } else {
            if (wait_queue_tail_.compare_exchange_weak(expected_tail, node, std::memory_order_release)) {
                expected_tail->next = node;
                break;
            }
        }
        expected_tail = wait_queue_tail_.load(std::memory_order_acquire);
    }
}

void FiberBarrier::wake_all_waiters() {
    // Wake all waiting fibers by updating the generation
    // Fibers will detect the generation change and exit their wait loops
}

} // namespace ecscope::jobs