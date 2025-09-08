#pragma once

/**
 * @file system_manager.hpp
 * @brief Advanced system lifecycle management with hot registration and conditional execution
 * 
 * This comprehensive system manager provides world-class system lifecycle management,
 * hot registration/unregistration, and sophisticated execution control with the following features:
 * 
 * - Thread-safe hot system registration and unregistration
 * - Dynamic system loading and unloading at runtime
 * - Conditional system execution based on runtime conditions
 * - System state management and lifecycle tracking
 * - Resource dependency validation and conflict resolution
 * - System execution priority and phase management
 * - Performance-aware system scheduling
 * - System health monitoring and recovery
 * - Automatic system dependency resolution
 * - System execution budgeting and throttling
 * - Event-driven system triggers
 * - System execution contexts and isolation
 * - Comprehensive logging and debugging support
 * 
 * System Management Features:
 * - Hierarchical system organization
 * - System groups and execution phases
 * - Dynamic system configuration
 * - System execution statistics
 * - Resource allocation and deallocation
 * - System state checkpointing
 * - Automatic error recovery
 * - System performance optimization
 * 
 * Advanced Capabilities:
 * - Runtime system modification
 * - System execution profiling
 * - Dependency graph visualization
 * - System health monitoring
 * - Performance regression detection
 * - Automatic load balancing
 * - System execution replay
 * - Configuration hot reloading
 * 
 * @author ECScope Professional ECS Framework
 * @date 2024
 */

#include "../core/types.hpp"
#include "../core/log.hpp"
#include "../core/time.hpp"
#include "execution_context.hpp"
#include "profiling.hpp"
#include "dependency_graph.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <functional>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <thread>
#include <future>
#include <chrono>
#include <queue>
#include <optional>
#include <variant>
#include <any>
#include <typeindex>
#include <fstream>

namespace ecscope::scheduling {

// Forward declarations
class System;
class Scheduler;
class ExecutionContextFactory;

using ecscope::core::u8;
using ecscope::core::u16;
using ecscope::core::u32;
using ecscope::core::u64;
using ecscope::core::f32;
using ecscope::core::f64;
using ecscope::core::usize;

/**
 * @brief System registration options and configuration
 */
struct SystemRegistrationOptions {
    SystemPhase phase = SystemPhase::Update;               // Execution phase
    u32 priority = 100;                                    // Execution priority (lower = higher priority)
    bool enabled = true;                                   // Whether system starts enabled
    bool allow_parallel = true;                            // Allow parallel execution
    f64 time_budget = 0.016;                              // Time budget in seconds (16ms for 60 FPS)
    u32 numa_node = ~0u;                                   // Preferred NUMA node (UINT32_MAX = any)
    std::vector<std::string> dependencies;                 // System dependencies
    std::vector<std::string> required_resources;           // Required resources
    std::vector<std::string> exclusive_resources;          // Resources requiring exclusive access
    std::function<bool()> execution_condition;             // Condition for execution
    std::unordered_map<std::string, std::any> properties;  // Custom properties
    
    SystemRegistrationOptions() = default;
    
    SystemRegistrationOptions& set_phase(SystemPhase p) { phase = p; return *this; }
    SystemRegistrationOptions& set_priority(u32 p) { priority = p; return *this; }
    SystemRegistrationOptions& set_enabled(bool e) { enabled = e; return *this; }
    SystemRegistrationOptions& set_parallel(bool p) { allow_parallel = p; return *this; }
    SystemRegistrationOptions& set_time_budget(f64 budget) { time_budget = budget; return *this; }
    SystemRegistrationOptions& set_numa_node(u32 node) { numa_node = node; return *this; }
    SystemRegistrationOptions& add_dependency(const std::string& dep) { 
        dependencies.push_back(dep); return *this; 
    }
    SystemRegistrationOptions& add_resource(const std::string& res) { 
        required_resources.push_back(res); return *this; 
    }
    SystemRegistrationOptions& add_exclusive_resource(const std::string& res) { 
        exclusive_resources.push_back(res); return *this; 
    }
    SystemRegistrationOptions& set_condition(std::function<bool()> cond) { 
        execution_condition = std::move(cond); return *this; 
    }
    
    template<typename T>
    SystemRegistrationOptions& set_property(const std::string& name, const T& value) {
        properties[name] = std::make_any<T>(value);
        return *this;
    }
    
    template<typename T>
    std::optional<T> get_property(const std::string& name) const {
        auto it = properties.find(name);
        if (it != properties.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (const std::bad_any_cast&) {
                // Type mismatch
            }
        }
        return std::nullopt;
    }
};

/**
 * @brief System runtime information and state
 */
class ManagedSystem {
private:
    // System identification
    u32 system_id_;
    std::string system_name_;
    std::unique_ptr<System> system_instance_;
    std::type_index system_type_;
    
    // Registration information
    SystemRegistrationOptions registration_options_;
    std::chrono::steady_clock::time_point registration_time_;
    
    // Runtime state
    std::atomic<SystemLifecycleState> lifecycle_state_;
    std::atomic<bool> enabled_;
    std::atomic<bool> execution_requested_;
    std::atomic<bool> currently_executing_;
    std::atomic<u64> last_execution_frame_;
    std::atomic<f64> last_execution_time_;
    
    // Performance tracking
    std::atomic<u64> total_executions_;
    std::atomic<f64> total_execution_time_;
    std::atomic<f64> average_execution_time_;
    std::atomic<u64> failed_executions_;
    std::atomic<u64> skipped_executions_;
    
    // Resource tracking
    std::unordered_set<u32> allocated_resources_;
    std::unordered_set<u32> locked_resources_;
    mutable std::shared_mutex resources_mutex_;
    
    // Execution context
    std::unique_ptr<ExecutionContext> execution_context_;
    mutable std::mutex context_mutex_;
    
    // Health monitoring
    std::atomic<f64> health_score_;          // 0.0 = critical, 1.0 = perfect health
    std::vector<std::string> health_issues_;
    mutable std::mutex health_mutex_;
    
    // Configuration
    std::atomic<bool> allow_hot_reload_;
    std::atomic<bool> monitor_performance_;
    std::atomic<f64> performance_threshold_;  // Performance warning threshold

public:
    ManagedSystem(u32 id, const std::string& name, std::unique_ptr<System> system,
                  const SystemRegistrationOptions& options);
    ~ManagedSystem();
    
    // Basic accessors
    u32 id() const noexcept { return system_id_; }
    const std::string& name() const noexcept { return system_name_; }
    System* system() const noexcept { return system_instance_.get(); }
    std::type_index type() const noexcept { return system_type_; }
    const SystemRegistrationOptions& options() const noexcept { return registration_options_; }
    
    // State management
    SystemLifecycleState state() const noexcept { 
        return lifecycle_state_.load(std::memory_order_acquire); 
    }
    void set_state(SystemLifecycleState new_state);
    bool can_execute() const;
    bool is_enabled() const noexcept { return enabled_.load(std::memory_order_acquire); }
    void set_enabled(bool enabled) { enabled_.store(enabled, std::memory_order_release); }
    
    // Execution control
    bool request_execution();
    void cancel_execution();
    bool is_currently_executing() const noexcept { 
        return currently_executing_.load(std::memory_order_acquire); 
    }
    bool should_execute_this_frame(u64 frame_number) const;
    
    // Performance tracking
    void record_execution(f64 execution_time, bool success);
    f64 get_average_execution_time() const noexcept { 
        return average_execution_time_.load(std::memory_order_relaxed); 
    }
    u64 get_total_executions() const noexcept { 
        return total_executions_.load(std::memory_order_relaxed); 
    }
    f64 get_success_rate() const;
    bool is_over_budget() const;
    
    // Resource management
    void allocate_resource(u32 resource_id);
    void deallocate_resource(u32 resource_id);
    bool has_resource(u32 resource_id) const;
    std::vector<u32> get_allocated_resources() const;
    void clear_allocated_resources();
    
    // Execution context
    ExecutionContext* get_execution_context() const;
    void create_execution_context(ExecutionContextFactory* factory);
    void destroy_execution_context();
    
    // Health monitoring
    f64 get_health_score() const noexcept { 
        return health_score_.load(std::memory_order_relaxed); 
    }
    void update_health_score(f64 new_score);
    void add_health_issue(const std::string& issue);
    std::vector<std::string> get_health_issues() const;
    void clear_health_issues();
    bool is_healthy() const { return get_health_score() > 0.7; }
    
    // Configuration updates
    void update_options(const SystemRegistrationOptions& new_options);
    void update_time_budget(f64 new_budget);
    void update_priority(u32 new_priority);
    void update_phase(SystemPhase new_phase);
    void update_dependencies(const std::vector<std::string>& new_dependencies);
    
    // Condition evaluation
    bool evaluate_execution_condition() const;
    void set_execution_condition(std::function<bool()> condition);
    
    // Serialization and debugging
    std::string to_string() const;
    std::unordered_map<std::string, std::any> get_debug_info() const;
    void log_system_state() const;
    
    // Thread safety
    void lock_for_modification() const;
    void unlock_after_modification() const;

private:
    void initialize_system();
    void validate_state_transition(SystemLifecycleState from, SystemLifecycleState to) const;
    void update_performance_metrics();
    void check_performance_health();
    
    mutable std::shared_mutex modification_mutex_;
    static std::atomic<u32> next_system_id_;
};

/**
 * @brief System event for tracking system lifecycle and execution events
 */
struct SystemEvent {
    enum class Type {
        Registered,         // System was registered
        Unregistered,       // System was unregistered
        StateChanged,       // System state changed
        ExecutionStarted,   // System execution started
        ExecutionEnded,     // System execution ended
        ExecutionFailed,    // System execution failed
        ConfigurationChanged, // System configuration changed
        HealthChanged,      // System health status changed
        ResourceAllocated,  // Resource was allocated
        ResourceDeallocated, // Resource was deallocated
        DependencyAdded,    // Dependency was added
        DependencyRemoved,  // Dependency was removed
        PerformanceAlert    // Performance threshold exceeded
    };
    
    Type type;
    u32 system_id;
    std::string system_name;
    u64 timestamp_ns;
    std::unordered_map<std::string, std::any> event_data;
    
    SystemEvent(Type t, u32 id, const std::string& name)
        : type(t), system_id(id), system_name(name)
        , timestamp_ns(std::chrono::high_resolution_clock::now().time_since_epoch().count()) {}
    
    template<typename T>
    void set_data(const std::string& key, const T& value) {
        event_data[key] = std::make_any<T>(value);
    }
    
    template<typename T>
    std::optional<T> get_data(const std::string& key) const {
        auto it = event_data.find(key);
        if (it != event_data.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (const std::bad_any_cast&) {
                // Type mismatch
            }
        }
        return std::nullopt;
    }
    
    std::string to_string() const;
};

/**
 * @brief System event listener interface
 */
class SystemEventListener {
public:
    virtual ~SystemEventListener() = default;
    virtual void on_system_event(const SystemEvent& event) = 0;
    virtual bool wants_event_type(SystemEvent::Type type) const = 0;
};

/**
 * @brief Advanced system manager with comprehensive lifecycle management
 */
class SystemManager {
private:
    // Core dependencies
    Scheduler* scheduler_;
    std::unique_ptr<ExecutionContextFactory> context_factory_;
    std::unique_ptr<DependencyGraph> dependency_graph_;
    
    // System storage
    std::unordered_map<u32, std::unique_ptr<ManagedSystem>> systems_by_id_;
    std::unordered_map<std::string, u32> systems_by_name_;
    std::unordered_map<std::type_index, std::vector<u32>> systems_by_type_;
    
    // Phase organization
    std::array<std::vector<u32>, static_cast<usize>(SystemPhase::COUNT)> systems_by_phase_;
    std::array<std::atomic<bool>, static_cast<usize>(SystemPhase::COUNT)> phase_enabled_;
    
    // Thread safety
    mutable std::shared_mutex systems_mutex_;
    std::array<std::mutex, static_cast<usize>(SystemPhase::COUNT)> phase_mutexes_;
    
    // Runtime state
    std::atomic<bool> manager_active_;
    std::atomic<bool> hot_reload_enabled_;
    std::atomic<u64> current_frame_;
    std::atomic<f64> frame_time_;
    
    // Event system
    std::vector<std::unique_ptr<SystemEventListener>> event_listeners_;
    std::queue<SystemEvent> pending_events_;
    mutable std::mutex events_mutex_;
    std::condition_variable events_condition_;
    std::thread event_processing_thread_;
    std::atomic<bool> process_events_;
    
    // Performance monitoring
    std::unordered_map<u32, f64> system_performance_baselines_;
    std::atomic<f64> global_performance_threshold_;
    mutable std::shared_mutex performance_mutex_;
    
    // Resource management integration
    std::unordered_map<std::string, u32> resource_name_to_id_;
    std::atomic<u32> next_resource_id_;
    mutable std::mutex resource_mapping_mutex_;
    
    // Configuration
    bool enable_performance_monitoring_;
    bool enable_health_monitoring_;
    bool enable_automatic_recovery_;
    f64 system_timeout_seconds_;
    usize max_concurrent_systems_;
    
    // Statistics
    struct {
        std::atomic<u64> total_systems_registered_{0};
        std::atomic<u64> total_systems_unregistered_{0};
        std::atomic<u64> total_hot_reloads_{0};
        std::atomic<u64> total_system_executions_{0};
        std::atomic<u64> total_system_failures_{0};
        std::atomic<u64> total_recovery_attempts_{0};
    } statistics_;

public:
    explicit SystemManager(Scheduler* scheduler);
    ~SystemManager();
    
    // Lifecycle
    bool initialize();
    void shutdown();
    bool is_active() const noexcept { return manager_active_.load(std::memory_order_acquire); }
    
    // System registration
    template<typename SystemType, typename... Args>
    u32 register_system(const std::string& name, const SystemRegistrationOptions& options,
                        Args&&... args) {
        static_assert(std::is_base_of_v<System, SystemType>, 
                      "SystemType must derive from System");
        
        auto system_instance = std::make_unique<SystemType>(std::forward<Args>(args)...);
        return register_system_internal(name, std::move(system_instance), options);
    }
    
    u32 register_system_internal(const std::string& name, std::unique_ptr<System> system,
                                 const SystemRegistrationOptions& options);
    
    bool unregister_system(u32 system_id);
    bool unregister_system(const std::string& system_name);
    
    // Hot registration/unregistration
    template<typename SystemType, typename... Args>
    u32 hot_register_system(const std::string& name, const SystemRegistrationOptions& options,
                            Args&&... args) {
        if (!hot_reload_enabled_.load(std::memory_order_acquire)) {
            LOG_WARNING("Hot reload is disabled, using regular registration");
            return register_system<SystemType>(name, options, std::forward<Args>(args)...);
        }
        
        u32 system_id = register_system<SystemType>(name, options, std::forward<Args>(args)...);
        if (system_id != 0) {
            statistics_.total_hot_reloads_.fetch_add(1, std::memory_order_relaxed);
            emit_system_event(SystemEvent::Type::Registered, system_id, name);
        }
        return system_id;
    }
    
    bool hot_unregister_system(u32 system_id);
    bool hot_unregister_system(const std::string& system_name);
    
    // System replacement (for hot reloading)
    template<typename SystemType, typename... Args>
    bool replace_system(u32 system_id, Args&&... args) {
        std::unique_lock lock(systems_mutex_);
        auto it = systems_by_id_.find(system_id);
        if (it == systems_by_id_.end()) {
            return false;
        }
        
        auto& managed_system = it->second;
        auto old_options = managed_system->options();
        std::string name = managed_system->name();
        
        // Create new system instance
        auto new_system = std::make_unique<SystemType>(std::forward<Args>(args)...);
        
        // Safely replace the system
        return replace_system_instance(system_id, std::move(new_system));
    }
    
    bool replace_system_instance(u32 system_id, std::unique_ptr<System> new_system);
    
    // System access
    ManagedSystem* get_managed_system(u32 system_id) const;
    ManagedSystem* get_managed_system(const std::string& system_name) const;
    System* get_system(u32 system_id) const;
    System* get_system(const std::string& system_name) const;
    
    template<typename SystemType>
    SystemType* get_system_of_type() const {
        std::shared_lock lock(systems_mutex_);
        auto it = systems_by_type_.find(std::type_index(typeid(SystemType)));
        if (it != systems_by_type_.end() && !it->second.empty()) {
            auto system_it = systems_by_id_.find(it->second[0]);
            if (system_it != systems_by_id_.end()) {
                return static_cast<SystemType*>(system_it->second->system());
            }
        }
        return nullptr;
    }
    
    std::vector<u32> get_systems_by_type(std::type_index type) const;
    std::vector<u32> get_systems_in_phase(SystemPhase phase) const;
    std::vector<u32> get_all_system_ids() const;
    std::vector<std::string> get_all_system_names() const;
    
    // System state management
    bool set_system_enabled(u32 system_id, bool enabled);
    bool set_system_enabled(const std::string& system_name, bool enabled);
    void set_phase_enabled(SystemPhase phase, bool enabled);
    bool is_system_enabled(u32 system_id) const;
    bool is_phase_enabled(SystemPhase phase) const;
    
    // System configuration
    bool update_system_options(u32 system_id, const SystemRegistrationOptions& new_options);
    bool update_system_time_budget(u32 system_id, f64 new_budget);
    bool update_system_priority(u32 system_id, u32 new_priority);
    bool update_system_phase(u32 system_id, SystemPhase new_phase);
    bool update_system_dependencies(u32 system_id, const std::vector<std::string>& dependencies);
    
    // Conditional execution
    bool set_system_condition(u32 system_id, std::function<bool()> condition);
    bool set_system_condition(const std::string& system_name, std::function<bool()> condition);
    void evaluate_system_conditions();
    std::vector<u32> get_conditionally_disabled_systems() const;
    
    // System execution planning
    std::vector<u32> plan_frame_execution(u64 frame_number, f64 frame_time) const;
    std::vector<u32> get_ready_systems(SystemPhase phase) const;
    bool validate_system_dependencies() const;
    std::vector<std::string> get_dependency_validation_errors() const;
    
    // Performance management
    void set_performance_baseline(u32 system_id, f64 baseline_time);
    void update_performance_baselines();
    std::vector<u32> get_underperforming_systems(f64 threshold_multiplier = 1.5) const;
    void optimize_system_order();
    
    // Health monitoring
    void update_system_health_scores();
    std::vector<u32> get_unhealthy_systems() const;
    bool attempt_system_recovery(u32 system_id);
    void enable_automatic_recovery(bool enabled) { enable_automatic_recovery_ = enabled; }
    
    // Resource management
    u32 register_resource(const std::string& resource_name);
    bool allocate_resource_to_system(u32 system_id, const std::string& resource_name);
    bool deallocate_resource_from_system(u32 system_id, const std::string& resource_name);
    std::vector<std::string> get_resource_conflicts() const;
    void resolve_resource_conflicts();
    
    // Event system
    void add_event_listener(std::unique_ptr<SystemEventListener> listener);
    void remove_event_listener(SystemEventListener* listener);
    void emit_system_event(SystemEvent::Type type, u32 system_id, const std::string& system_name);
    void process_pending_events();
    
    // Frame execution
    void begin_frame(u64 frame_number, f64 frame_time);
    void end_frame();
    u64 get_current_frame() const noexcept { return current_frame_.load(std::memory_order_relaxed); }
    f64 get_frame_time() const noexcept { return frame_time_.load(std::memory_order_relaxed); }
    
    // Configuration
    void set_hot_reload_enabled(bool enabled) { 
        hot_reload_enabled_.store(enabled, std::memory_order_release); 
    }
    bool is_hot_reload_enabled() const { 
        return hot_reload_enabled_.load(std::memory_order_acquire); 
    }
    void set_performance_monitoring(bool enabled) { enable_performance_monitoring_ = enabled; }
    void set_health_monitoring(bool enabled) { enable_health_monitoring_ = enabled; }
    void set_system_timeout(f64 timeout_seconds) { system_timeout_seconds_ = timeout_seconds; }
    void set_max_concurrent_systems(usize max_systems) { max_concurrent_systems_ = max_systems; }
    
    // Information and statistics
    usize get_system_count() const;
    usize get_systems_in_phase_count(SystemPhase phase) const;
    f64 get_total_system_execution_time() const;
    f64 get_average_system_execution_time() const;
    
    struct ManagerStatistics {
        u64 total_systems_registered;
        u64 total_systems_unregistered;
        u64 total_hot_reloads;
        u64 total_system_executions;
        u64 total_system_failures;
        u64 total_recovery_attempts;
        usize active_systems;
        f64 average_system_health;
        f64 total_execution_time;
    };
    ManagerStatistics get_statistics() const;
    void reset_statistics();
    
    // Debugging and introspection
    std::string generate_system_report() const;
    void export_system_configuration(const std::string& filename) const;
    void import_system_configuration(const std::string& filename);
    void log_system_states() const;
    std::unordered_map<std::string, std::any> get_debug_info() const;
    
    // Advanced features
    void save_system_state_snapshot(const std::string& name);
    bool restore_system_state_snapshot(const std::string& name);
    void clear_system_state_snapshots();
    std::vector<std::string> get_available_snapshots() const;

private:
    // Internal system management
    bool add_system_to_phase(u32 system_id, SystemPhase phase);
    bool remove_system_from_phase(u32 system_id, SystemPhase old_phase);
    bool move_system_between_phases(u32 system_id, SystemPhase from_phase, SystemPhase to_phase);
    void update_dependency_graph();
    
    // Resource management internal
    u32 get_or_create_resource_id(const std::string& resource_name);
    void cleanup_system_resources(u32 system_id);
    
    // Event processing
    void event_processing_thread_function();
    void dispatch_event(const SystemEvent& event);
    
    // Health and performance monitoring
    void monitor_system_health(u32 system_id);
    void monitor_system_performance(u32 system_id);
    void handle_system_failure(u32 system_id, const std::string& reason);
    bool recover_failed_system(u32 system_id);
    
    // Validation and error handling
    bool validate_system_registration(const std::string& name, const SystemRegistrationOptions& options);
    void handle_registration_error(const std::string& name, const std::string& error);
    void cleanup_failed_registration(u32 system_id);
    
    // Utility functions
    u32 generate_system_id();
    std::string generate_system_state_json() const;
    void parse_system_state_json(const std::string& json);
    
    static const char* phase_name(SystemPhase phase);
    static const char* event_type_name(SystemEvent::Type type);
};

// Convenience macros for system registration
#define REGISTER_SYSTEM_WITH_OPTIONS(manager, SystemType, name, options, ...) \
    (manager).register_system<SystemType>(name, options, __VA_ARGS__)

#define REGISTER_SYSTEM_SIMPLE(manager, SystemType, ...) \
    (manager).register_system<SystemType>(#SystemType, SystemRegistrationOptions{}, __VA_ARGS__)

#define HOT_REGISTER_SYSTEM(manager, SystemType, name, options, ...) \
    (manager).hot_register_system<SystemType>(name, options, __VA_ARGS__)

// System event listener helper
class LambdaSystemEventListener : public SystemEventListener {
public:
    template<typename F>
    LambdaSystemEventListener(F&& handler, std::vector<SystemEvent::Type> event_types = {})
        : handler_(std::forward<F>(handler)), interested_types_(std::move(event_types)) {}
    
    void on_system_event(const SystemEvent& event) override {
        handler_(event);
    }
    
    bool wants_event_type(SystemEvent::Type type) const override {
        return interested_types_.empty() || 
               std::find(interested_types_.begin(), interested_types_.end(), type) != interested_types_.end();
    }
    
private:
    std::function<void(const SystemEvent&)> handler_;
    std::vector<SystemEvent::Type> interested_types_;
};

} // namespace ecscope::scheduling