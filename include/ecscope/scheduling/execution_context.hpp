#pragma once

/**
 * @file execution_context.hpp
 * @brief Advanced system execution context with resource isolation and lifecycle management
 * 
 * This implementation provides professional-grade execution context management for systems
 * with comprehensive resource isolation, lifecycle tracking, and performance optimization:
 * 
 * - Thread-safe execution context with resource access control
 * - System lifecycle management (init, update, shutdown)
 * - Resource acquisition and release tracking
 * - Memory pool allocation for execution contexts
 * - Exception handling and recovery mechanisms
 * - System state checkpointing and rollback
 * - Performance metrics collection and analysis
 * - Inter-system communication channels
 * - Event-driven system triggers
 * - Dynamic resource scaling based on demand
 * 
 * Context Features:
 * - Isolated execution environments for systems
 * - Hierarchical resource management
 * - Thread-local storage optimization
 * - Cache-friendly data layout
 * - NUMA-aware memory allocation
 * - Dependency satisfaction tracking
 * 
 * Performance Optimizations:
 * - Lock-free resource access where possible
 * - Cache-aligned data structures
 * - Memory prefetching for predictable access patterns
 * - Batch resource operations
 * - Lazy initialization of resources
 * - Resource pooling and reuse
 * 
 * @author ECScope Professional ECS Framework
 * @date 2024
 */

#include "../core/types.hpp"
#include "../core/log.hpp"
#include "../core/time.hpp"
#include "../foundation/memory_utils.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <functional>
#include <string>
#include <any>
#include <typeindex>
#include <thread>
#include <chrono>
#include <exception>
#include <optional>

namespace ecscope::scheduling {

// Forward declarations
class System;
class ResourceManager;
class SystemManager;

using ecscope::core::u8;
using ecscope::core::u32;
using ecscope::core::u64;
using ecscope::core::f32;
using ecscope::core::f64;
using ecscope::core::usize;

/**
 * @brief System lifecycle state enumeration
 */
enum class SystemLifecycleState : u8 {
    Created = 0,        // System object created but not initialized
    Initializing,       // System is being initialized
    Ready,              // System is ready to execute
    Executing,          // System is currently executing
    Suspended,          // System execution is suspended
    Error,              // System encountered an error
    ShuttingDown,       // System is shutting down
    Destroyed           // System has been destroyed
};

/**
 * @brief Resource access type for fine-grained control
 */
enum class ResourceAccessType : u8 {
    None = 0,
    Read = 1 << 0,           // Read-only access
    Write = 1 << 1,          // Write access
    Create = 1 << 2,         // Can create new resources
    Delete = 1 << 3,         // Can delete resources
    Exclusive = 1 << 4,      // Exclusive access (no other systems)
    Persistent = 1 << 5,     // Resource persists across frames
    Volatile = 1 << 6        // Resource is volatile (can change frequently)
};

constexpr ResourceAccessType operator|(ResourceAccessType a, ResourceAccessType b) {
    return static_cast<ResourceAccessType>(static_cast<u8>(a) | static_cast<u8>(b));
}

constexpr ResourceAccessType operator&(ResourceAccessType a, ResourceAccessType b) {
    return static_cast<ResourceAccessType>(static_cast<u8>(a) & static_cast<u8>(b));
}

/**
 * @brief System execution result with detailed information
 */
struct SystemExecutionResult {
    bool success = true;                    // Whether execution succeeded
    f64 execution_time = 0.0;              // Actual execution time in seconds
    f64 resource_wait_time = 0.0;          // Time spent waiting for resources
    u32 resources_accessed = 0;            // Number of resources accessed
    u32 exceptions_thrown = 0;             // Number of exceptions during execution
    std::string error_message;             // Error message if execution failed
    std::vector<std::string> warnings;     // Non-fatal warnings during execution
    
    // Performance metrics
    u64 instructions_executed = 0;         // Approximate instruction count
    u64 cache_misses = 0;                  // Cache miss count
    u64 memory_allocated = 0;              // Memory allocated during execution
    f64 cpu_utilization = 0.0;             // CPU utilization percentage
    
    SystemExecutionResult() = default;
    
    explicit operator bool() const noexcept { return success; }
    
    bool has_warnings() const noexcept { return !warnings.empty(); }
    bool has_performance_issues() const noexcept {
        return cpu_utilization > 90.0 || cache_misses > 1000;
    }
    
    void add_warning(const std::string& warning) {
        warnings.push_back(warning);
    }
    
    void merge_with(const SystemExecutionResult& other) {
        success = success && other.success;
        execution_time += other.execution_time;
        resource_wait_time += other.resource_wait_time;
        resources_accessed += other.resources_accessed;
        exceptions_thrown += other.exceptions_thrown;
        instructions_executed += other.instructions_executed;
        cache_misses += other.cache_misses;
        memory_allocated += other.memory_allocated;
        cpu_utilization = (cpu_utilization + other.cpu_utilization) / 2.0;
        
        warnings.insert(warnings.end(), other.warnings.begin(), other.warnings.end());
        if (!other.error_message.empty()) {
            if (error_message.empty()) {
                error_message = other.error_message;
            } else {
                error_message += "; " + other.error_message;
            }
        }
    }
};

/**
 * @brief Resource handle for tracking resource access
 */
class ResourceHandle {
private:
    u32 resource_id_;
    std::string resource_name_;
    ResourceAccessType access_type_;
    void* resource_ptr_;
    std::type_index resource_type_;
    u64 acquisition_time_;
    u64 last_access_time_;
    u32 access_count_;
    bool is_locked_;
    std::shared_ptr<std::mutex> resource_mutex_;
    
public:
    ResourceHandle(u32 id, const std::string& name, ResourceAccessType access, 
                   void* ptr, std::type_index type)
        : resource_id_(id), resource_name_(name), access_type_(access)
        , resource_ptr_(ptr), resource_type_(type)
        , acquisition_time_(get_current_time_ns()), last_access_time_(acquisition_time_)
        , access_count_(0), is_locked_(false)
        , resource_mutex_(std::make_shared<std::mutex>()) {}
    
    // Accessors
    u32 id() const noexcept { return resource_id_; }
    const std::string& name() const noexcept { return resource_name_; }
    ResourceAccessType access_type() const noexcept { return access_type_; }
    void* ptr() const noexcept { return resource_ptr_; }
    std::type_index type() const noexcept { return resource_type_; }
    bool is_locked() const noexcept { return is_locked_; }
    
    // Access tracking
    void record_access() {
        last_access_time_ = get_current_time_ns();
        access_count_++;
    }
    
    f64 get_idle_time() const {
        return (get_current_time_ns() - last_access_time_) / 1e9;
    }
    
    u32 access_count() const noexcept { return access_count_; }
    
    // Type-safe resource access
    template<typename T>
    T* get_as() const {
        if (resource_type_ != std::type_index(typeid(T))) {
            return nullptr;
        }
        record_access();
        return static_cast<T*>(resource_ptr_);
    }
    
    // Locking
    bool try_lock() {
        if ((access_type_ & ResourceAccessType::Exclusive) != ResourceAccessType::None) {
            bool expected = false;
            return is_locked_ == expected && 
                   std::atomic_compare_exchange_strong(
                       reinterpret_cast<std::atomic<bool>*>(&is_locked_), &expected, true);
        }
        return true; // Non-exclusive resources don't need locking
    }
    
    void unlock() {
        if ((access_type_ & ResourceAccessType::Exclusive) != ResourceAccessType::None) {
            is_locked_ = false;
        }
    }
    
    // Thread-safe shared access
    std::shared_ptr<std::mutex> get_mutex() const { return resource_mutex_; }
    
private:
    static u64 get_current_time_ns() {
        return std::chrono::high_resolution_clock::now().time_since_epoch().count();
    }
    
    mutable void record_access() const {
        const_cast<ResourceHandle*>(this)->last_access_time_ = get_current_time_ns();
        const_cast<ResourceHandle*>(this)->access_count_++;
    }
};

/**
 * @brief System execution context providing controlled access to resources
 */
class ExecutionContext {
private:
    // Context identification
    u32 context_id_;
    std::string context_name_;
    std::thread::id execution_thread_;
    u32 numa_node_;
    
    // System information
    System* current_system_;
    SystemLifecycleState system_state_;
    u64 frame_number_;
    f64 frame_time_;
    f64 delta_time_;
    f64 total_time_;
    
    // Resource management
    std::unordered_map<u32, std::unique_ptr<ResourceHandle>> acquired_resources_;
    std::unordered_set<u32> required_resources_;
    std::unordered_set<u32> exclusive_resources_;
    mutable std::shared_mutex resources_mutex_;
    
    // Memory management
    std::unique_ptr<std::byte[]> context_memory_;
    usize context_memory_size_;
    std::atomic<usize> memory_used_;
    std::atomic<usize> peak_memory_used_;
    
    // Performance tracking
    std::chrono::high_resolution_clock::time_point execution_start_;
    std::atomic<f64> total_execution_time_;
    std::atomic<u64> total_resource_waits_;
    std::atomic<u64> total_exceptions_;
    
    // Exception handling
    std::vector<std::exception_ptr> captured_exceptions_;
    std::function<void(const std::exception&)> exception_handler_;
    mutable std::mutex exceptions_mutex_;
    
    // Inter-system communication
    std::unordered_map<std::string, std::any> communication_channels_;
    mutable std::shared_mutex channels_mutex_;
    
    // State checkpointing
    struct Checkpoint {
        SystemLifecycleState system_state;
        std::unordered_map<u32, std::vector<u8>> resource_snapshots;
        f64 checkpoint_time;
        std::string checkpoint_name;
    };
    std::vector<Checkpoint> checkpoints_;
    mutable std::mutex checkpoints_mutex_;
    
    // Configuration
    bool enable_resource_tracking_;
    bool enable_performance_monitoring_;
    bool enable_exception_handling_;
    f64 resource_timeout_;
    
    static std::atomic<u32> next_context_id_;

public:
    ExecutionContext(const std::string& name, System* system, u32 numa_node = 0);
    ~ExecutionContext();
    
    // Context information
    u32 id() const noexcept { return context_id_; }
    const std::string& name() const noexcept { return context_name_; }
    std::thread::id thread() const noexcept { return execution_thread_; }
    u32 numa_node() const noexcept { return numa_node_; }
    System* system() const noexcept { return current_system_; }
    SystemLifecycleState state() const noexcept { return system_state_; }
    
    // Frame information
    u64 frame_number() const noexcept { return frame_number_; }
    f64 frame_time() const noexcept { return frame_time_; }
    f64 delta_time() const noexcept { return delta_time_; }
    f64 total_time() const noexcept { return total_time_; }
    
    // Lifecycle management
    void initialize(u64 frame_number, f64 frame_time, f64 delta_time, f64 total_time);
    void prepare_execution();
    SystemExecutionResult execute_system();
    void finalize_execution(const SystemExecutionResult& result);
    void shutdown();
    
    // State management
    void set_state(SystemLifecycleState state) { system_state_ = state; }
    void transition_to_state(SystemLifecycleState new_state);
    bool can_transition_to(SystemLifecycleState new_state) const;
    
    // Resource management
    template<typename T>
    bool acquire_resource(u32 resource_id, const std::string& name, ResourceAccessType access) {
        std::unique_lock lock(resources_mutex_);
        
        // Check if resource is already acquired
        if (acquired_resources_.find(resource_id) != acquired_resources_.end()) {
            return true;
        }
        
        // Try to acquire the resource (this would interface with ResourceManager)
        T* resource_ptr = acquire_resource_internal<T>(resource_id, name, access);
        if (!resource_ptr) {
            return false;
        }
        
        auto handle = std::make_unique<ResourceHandle>(
            resource_id, name, access, resource_ptr, std::type_index(typeid(T)));
        
        if (!handle->try_lock()) {
            release_resource_internal<T>(resource_ptr);
            return false;
        }
        
        acquired_resources_[resource_id] = std::move(handle);
        return true;
    }
    
    template<typename T>
    T* get_resource(u32 resource_id) const {
        std::shared_lock lock(resources_mutex_);
        auto it = acquired_resources_.find(resource_id);
        if (it != acquired_resources_.end()) {
            return it->second->get_as<T>();
        }
        return nullptr;
    }
    
    template<typename T>
    T* get_resource(const std::string& name) const {
        std::shared_lock lock(resources_mutex_);
        for (const auto& [id, handle] : acquired_resources_) {
            if (handle->name() == name) {
                return handle->get_as<T>();
            }
        }
        return nullptr;
    }
    
    void release_resource(u32 resource_id);
    void release_all_resources();
    
    // Resource requirements
    void require_resource(u32 resource_id, ResourceAccessType access = ResourceAccessType::Read);
    void require_exclusive_resource(u32 resource_id);
    bool has_required_resources() const;
    std::vector<u32> get_missing_resources() const;
    
    // Memory management
    void* allocate_memory(usize size, usize alignment = alignof(std::max_align_t));
    void deallocate_memory(void* ptr);
    usize get_memory_usage() const { return memory_used_.load(std::memory_order_relaxed); }
    usize get_peak_memory_usage() const { return peak_memory_used_.load(std::memory_order_relaxed); }
    
    // Exception handling
    void set_exception_handler(std::function<void(const std::exception&)> handler);
    void handle_exception(const std::exception& e);
    std::vector<std::exception_ptr> get_captured_exceptions() const;
    void clear_exceptions();
    bool has_exceptions() const;
    
    // Inter-system communication
    template<typename T>
    void set_channel_data(const std::string& channel_name, const T& data) {
        std::unique_lock lock(channels_mutex_);
        communication_channels_[channel_name] = std::make_any<T>(data);
    }
    
    template<typename T>
    std::optional<T> get_channel_data(const std::string& channel_name) const {
        std::shared_lock lock(channels_mutex_);
        auto it = communication_channels_.find(channel_name);
        if (it != communication_channels_.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (const std::bad_any_cast&) {
                // Type mismatch
            }
        }
        return std::nullopt;
    }
    
    void clear_channel(const std::string& channel_name);
    std::vector<std::string> get_available_channels() const;
    
    // State checkpointing
    void create_checkpoint(const std::string& name);
    bool restore_checkpoint(const std::string& name);
    void clear_checkpoints();
    std::vector<std::string> get_available_checkpoints() const;
    
    // Performance monitoring
    f64 get_total_execution_time() const { 
        return total_execution_time_.load(std::memory_order_relaxed); 
    }
    u64 get_resource_wait_count() const { 
        return total_resource_waits_.load(std::memory_order_relaxed); 
    }
    u64 get_exception_count() const { 
        return total_exceptions_.load(std::memory_order_relaxed); 
    }
    
    // Configuration
    void set_resource_tracking(bool enable) { enable_resource_tracking_ = enable; }
    void set_performance_monitoring(bool enable) { enable_performance_monitoring_ = enable; }
    void set_exception_handling(bool enable) { enable_exception_handling_ = enable; }
    void set_resource_timeout(f64 timeout_seconds) { resource_timeout_ = timeout_seconds; }
    
    // Information and debugging
    std::vector<std::pair<std::string, ResourceAccessType>> get_acquired_resources() const;
    std::string generate_execution_report() const;
    void log_resource_usage() const;
    
    // Thread safety
    bool is_thread_safe() const;
    void ensure_thread_safety() const;

private:
    // Internal resource management
    template<typename T>
    T* acquire_resource_internal(u32 resource_id, const std::string& name, ResourceAccessType access);
    
    template<typename T>
    void release_resource_internal(T* resource_ptr);
    
    // Internal utilities
    void update_memory_statistics(isize delta);
    void record_resource_wait();
    void record_exception();
    bool validate_state_transition(SystemLifecycleState from, SystemLifecycleState to) const;
    void log_state_transition(SystemLifecycleState from, SystemLifecycleState to) const;
    
    // Checkpoint utilities
    std::vector<u8> serialize_resource_state(u32 resource_id) const;
    void deserialize_resource_state(u32 resource_id, const std::vector<u8>& data);
    
    static u64 get_current_time_ns() {
        return std::chrono::high_resolution_clock::now().time_since_epoch().count();
    }
};

/**
 * @brief Factory for creating and managing execution contexts
 */
class ExecutionContextFactory {
private:
    std::unordered_map<u32, std::unique_ptr<ExecutionContext>> contexts_;
    std::vector<std::unique_ptr<ExecutionContext>> context_pool_;
    std::mutex factory_mutex_;
    
    // Configuration
    usize max_contexts_;
    usize context_memory_size_;
    bool enable_context_pooling_;
    
public:
    ExecutionContextFactory(usize max_contexts = 1000, 
                           usize context_memory_size = 1024 * 1024, // 1MB per context
                           bool enable_pooling = true);
    ~ExecutionContextFactory();
    
    // Context creation and management
    std::unique_ptr<ExecutionContext> create_context(const std::string& name, System* system, u32 numa_node = 0);
    void return_context(std::unique_ptr<ExecutionContext> context);
    ExecutionContext* get_context(u32 context_id) const;
    
    // Pool management
    void warm_pool(usize count);
    void clear_pool();
    usize pool_size() const;
    usize active_contexts() const;
    
    // Configuration
    void set_max_contexts(usize max_contexts) { max_contexts_ = max_contexts; }
    void set_context_memory_size(usize size) { context_memory_size_ = size; }
    void set_pooling_enabled(bool enabled) { enable_context_pooling_ = enabled; }
    
    // Statistics
    struct Statistics {
        usize total_contexts_created;
        usize total_contexts_destroyed;
        usize active_contexts;
        usize pooled_contexts;
        f64 average_context_lifetime;
        usize peak_active_contexts;
    };
    Statistics get_statistics() const;
    void reset_statistics();

private:
    std::unique_ptr<ExecutionContext> create_context_internal(const std::string& name, System* system, u32 numa_node);
    void cleanup_expired_contexts();
};

} // namespace ecscope::scheduling