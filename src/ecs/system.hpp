#pragma once

/**
 * @file system.hpp
 * @brief Advanced ECS System Architecture and Scheduling Framework
 * 
 * This comprehensive system framework provides sophisticated system management,
 * scheduling, and execution capabilities with educational focus on system
 * architecture patterns and performance optimization.
 * 
 * Key Educational Features:
 * - System base class with lifecycle management
 * - Dependency-based system ordering and parallel execution
 * - System performance profiling with detailed metrics
 * - Event-driven system communication
 * - Resource management and system state tracking
 * - Thread pool integration for parallel system execution
 * - System execution timeline visualization
 * - Memory-efficient system data storage using arena allocators
 * 
 * System Types:
 * - Update systems: Run every frame with delta time
 * - Fixed update systems: Run at fixed intervals
 * - Event systems: Triggered by specific events
 * - Initialization systems: Run once during setup
 * - Cleanup systems: Run during shutdown
 * 
 * Advanced Features:
 * - System dependency resolution and execution ordering
 * - Parallel system execution with work stealing
 * - System performance budgeting and monitoring
 * - Dynamic system loading and unloading
 * - System state checkpointing and recovery
 * - Cross-system communication via events and shared resources
 * 
 * @author ECScope Educational ECS Framework
 * @date 2024
 */

#include "core/types.hpp"
#include "core/log.hpp"
#include "core/time.hpp"
#include "query.hpp"
#include "memory/arena.hpp"
#include "memory/memory_tracker.hpp"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>
#include <functional>
#include <thread>
#include <future>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <queue>
#include <any>
#include <typeindex>

namespace ecscope::ecs {

// Forward declarations
class Registry;
class SystemManager;
class EventBus;
class ResourceManager;

/**
 * @brief System execution phase enumeration
 */
enum class SystemPhase : u8 {
    PreInitialize = 0,      // Before main initialization
    Initialize,             // Main initialization phase
    PostInitialize,         // After initialization
    PreUpdate,              // Before main update loop
    Update,                 // Main update phase
    PostUpdate,             // After main update
    PreRender,              // Before rendering
    Render,                 // Rendering phase
    PostRender,             // After rendering
    PreCleanup,             // Before cleanup
    Cleanup,                // Cleanup phase
    PostCleanup,            // After cleanup
    COUNT                   // Number of phases
};

/**
 * @brief System execution type
 */
enum class SystemExecutionType : u8 {
    Sequential,             // Execute on main thread sequentially
    Parallel,               // Can execute in parallel with other parallel systems
    Exclusive,              // Must execute alone (no other systems running)
    Background,             // Execute on background thread
    Immediate,              // Execute immediately when triggered
    Deferred               // Execute at next appropriate time
};

/**
 * @brief System execution statistics for performance analysis
 */
struct SystemStats {
    // Execution metrics
    u64 total_executions;           // Total number of times executed
    f64 total_execution_time;       // Total time spent executing (seconds)
    f64 average_execution_time;     // Average execution time
    f64 min_execution_time;         // Fastest execution time
    f64 max_execution_time;         // Slowest execution time
    f64 last_execution_time;        // Last execution time
    
    // Scheduling metrics
    u64 total_scheduled;            // Total times scheduled for execution
    u64 total_skipped;              // Times skipped due to conditions
    u64 total_deferred;             // Times deferred to later
    f64 scheduling_overhead;        // Time spent in scheduling logic
    
    // Memory metrics
    usize memory_allocated;         // Memory allocated by system
    usize peak_memory_usage;        // Peak memory usage
    usize memory_allocations;       // Number of memory allocations
    
    // Dependency metrics
    usize dependencies_resolved;    // Dependencies resolved per execution
    f64 dependency_wait_time;       // Time waiting for dependencies
    
    // Performance budget
    f64 allocated_time_budget;      // Allocated time budget per frame
    f64 actual_time_usage;          // Actual time used
    f64 budget_utilization;         // Percentage of budget used
    bool exceeded_budget;           // Whether budget was exceeded
    
    SystemStats() noexcept { reset(); }
    
    void reset() noexcept {
        total_executions = 0;
        total_execution_time = 0.0;
        average_execution_time = 0.0;
        min_execution_time = std::numeric_limits<f64>::max();
        max_execution_time = 0.0;
        last_execution_time = 0.0;
        total_scheduled = 0;
        total_skipped = 0;
        total_deferred = 0;
        scheduling_overhead = 0.0;
        memory_allocated = 0;
        peak_memory_usage = 0;
        memory_allocations = 0;
        dependencies_resolved = 0;
        dependency_wait_time = 0.0;
        allocated_time_budget = 0.016;  // 16ms default (60 FPS)
        actual_time_usage = 0.0;
        budget_utilization = 0.0;
        exceeded_budget = false;
    }
    
    void update_averages() noexcept {
        if (total_executions > 0) {
            average_execution_time = total_execution_time / total_executions;
            budget_utilization = allocated_time_budget > 0.0 ? 
                (actual_time_usage / allocated_time_budget) : 0.0;
            exceeded_budget = actual_time_usage > allocated_time_budget;
        }
    }
    
    void record_execution(f64 execution_time) noexcept {
        total_executions++;
        total_execution_time += execution_time;
        last_execution_time = execution_time;
        actual_time_usage = execution_time;
        
        if (execution_time < min_execution_time) {
            min_execution_time = execution_time;
        }
        if (execution_time > max_execution_time) {
            max_execution_time = execution_time;
        }
        
        update_averages();
    }
};

/**
 * @brief System dependency information
 */
struct SystemDependency {
    std::string system_name;        // Name of dependent system
    bool is_hard_dependency;        // Hard dependency (must complete before this system)
    bool is_soft_dependency;        // Soft dependency (prefer to run after)
    f64 max_wait_time;             // Maximum time to wait for dependency
    
    SystemDependency(const std::string& name, bool hard = true, f64 max_wait = 0.1)
        : system_name(name), is_hard_dependency(hard), is_soft_dependency(!hard)
        , max_wait_time(max_wait) {}
};

/**
 * @brief System resource requirements and access patterns
 */
struct SystemResourceInfo {
    std::vector<std::type_index> read_components;     // Component types read by system
    std::vector<std::type_index> write_components;    // Component types written by system
    std::vector<std::string> read_resources;         // Global resources read
    std::vector<std::string> write_resources;        // Global resources written
    std::vector<std::string> exclusive_resources;    // Resources requiring exclusive access
    
    bool conflicts_with(const SystemResourceInfo& other) const {
        // Check for write-write conflicts
        for (const auto& write_comp : write_components) {
            if (std::find(other.write_components.begin(), other.write_components.end(), write_comp) 
                != other.write_components.end()) {
                return true;
            }
        }
        
        // Check for read-write conflicts
        for (const auto& write_comp : write_components) {
            if (std::find(other.read_components.begin(), other.read_components.end(), write_comp) 
                != other.read_components.end()) {
                return true;
            }
        }
        
        for (const auto& read_comp : read_components) {
            if (std::find(other.write_components.begin(), other.write_components.end(), read_comp) 
                != other.write_components.end()) {
                return true;
            }
        }
        
        // Check for resource conflicts
        for (const auto& exclusive : exclusive_resources) {
            if (std::find(other.exclusive_resources.begin(), other.exclusive_resources.end(), exclusive)
                != other.exclusive_resources.end()) {
                return true;
            }
        }
        
        return false;
    }
};

/**
 * @brief System execution context providing access to ECS and resources
 */
class SystemContext {
private:
    Registry* registry_;
    EventBus* event_bus_;
    ResourceManager* resource_manager_;
    QueryManager* query_manager_;
    f64 delta_time_;
    f64 total_time_;
    u64 frame_number_;
    SystemPhase current_phase_;
    
public:
    SystemContext(Registry* registry, EventBus* event_bus, ResourceManager* resource_manager,
                 QueryManager* query_manager, f64 delta_time, f64 total_time, u64 frame_number,
                 SystemPhase phase)
        : registry_(registry), event_bus_(event_bus), resource_manager_(resource_manager)
        , query_manager_(query_manager), delta_time_(delta_time), total_time_(total_time)
        , frame_number_(frame_number), current_phase_(phase) {}
    
    // Accessors
    Registry& registry() const { return *registry_; }
    EventBus& events() const { return *event_bus_; }
    ResourceManager& resources() const { return *resource_manager_; }
    QueryManager& queries() const { return *query_manager_; }
    
    f64 delta_time() const noexcept { return delta_time_; }
    f64 total_time() const noexcept { return total_time_; }
    u64 frame_number() const noexcept { return frame_number_; }
    SystemPhase phase() const noexcept { return current_phase_; }
    
    // Convenience query creation
    template<typename... Filters>
    Query<Filters...> create_query(const std::string& name = "") const {
        return Query<Filters...>(*registry_, &query_manager_->cache(), name);
    }
    
    DynamicQuery create_dynamic_query(const std::string& name = "") const {
        return DynamicQuery(*registry_, &query_manager_->cache()).named(name);
    }
};

/**
 * @brief Base class for all ECS systems
 * 
 * Provides a comprehensive interface for system lifecycle management,
 * resource access, and performance monitoring.
 */
class System {
protected:
    std::string name_;                          // System name for debugging and profiling
    SystemPhase primary_phase_;                 // Primary execution phase
    SystemExecutionType execution_type_;       // How this system should be executed
    bool is_enabled_;                          // Whether system is enabled
    bool is_initialized_;                      // Whether system has been initialized
    
    // Dependencies and resource requirements
    std::vector<SystemDependency> dependencies_;
    SystemResourceInfo resource_info_;
    
    // Performance tracking
    mutable SystemStats stats_;
    f64 time_budget_;                          // Allocated time budget per execution
    
    // Memory management
    std::unique_ptr<memory::ArenaAllocator> system_arena_;
    u32 allocator_id_;
    
public:
    explicit System(const std::string& name, 
                   SystemPhase phase = SystemPhase::Update,
                   SystemExecutionType execution = SystemExecutionType::Sequential);
    
    virtual ~System();
    
    // System lifecycle
    virtual bool initialize(const SystemContext& context);
    virtual void update(const SystemContext& context) = 0;
    virtual void shutdown(const SystemContext& context);
    
    // Configuration
    System& set_phase(SystemPhase phase) { primary_phase_ = phase; return *this; }
    System& set_execution_type(SystemExecutionType type) { execution_type_ = type; return *this; }
    System& set_time_budget(f64 budget) { time_budget_ = budget; return *this; }
    System& set_enabled(bool enabled) { is_enabled_ = enabled; return *this; }
    
    // Dependencies
    System& depends_on(const std::string& system_name, bool hard_dependency = true, f64 max_wait = 0.1);
    System& reads_component(std::type_index component_type);
    System& writes_component(std::type_index component_type);
    System& reads_resource(const std::string& resource_name);
    System& writes_resource(const std::string& resource_name);
    System& exclusive_resource(const std::string& resource_name);
    
    template<Component T>
    System& reads() { 
        return reads_component(std::type_index(typeid(T))); 
    }
    
    template<Component T>
    System& writes() { 
        return writes_component(std::type_index(typeid(T))); 
    }
    
    // Accessors
    const std::string& name() const noexcept { return name_; }
    SystemPhase phase() const noexcept { return primary_phase_; }
    SystemExecutionType execution_type() const noexcept { return execution_type_; }
    bool is_enabled() const noexcept { return is_enabled_; }
    bool is_initialized() const noexcept { return is_initialized_; }
    f64 time_budget() const noexcept { return time_budget_; }
    
    const std::vector<SystemDependency>& dependencies() const noexcept { return dependencies_; }
    const SystemResourceInfo& resource_info() const noexcept { return resource_info_; }
    const SystemStats& statistics() const noexcept { return stats_; }
    
    // Performance analysis
    f64 get_average_execution_time() const noexcept { return stats_.average_execution_time; }
    f64 get_budget_utilization() const noexcept { return stats_.budget_utilization; }
    bool is_over_budget() const noexcept { return stats_.exceeded_budget; }
    
    // Memory management
    memory::ArenaAllocator& arena() { return *system_arena_; }
    const memory::ArenaAllocator& arena() const { return *system_arena_; }
    
    // Statistics management
    void reset_statistics() { stats_.reset(); }
    
    // Internal execution wrapper (called by SystemManager)
    void execute_internal(const SystemContext& context);
    
protected:
    // Helper for creating queries within systems
    template<typename... Filters>
    Query<Filters...> create_query(const SystemContext& context, const std::string& name = "") const {
        return context.create_query<Filters...>(name.empty() ? (name_ + "_Query") : name);
    }
    
    // Memory allocation helpers
    template<typename T>
    T* allocate(usize count = 1, const char* category = nullptr) {
        return arena_allocate<T>(*system_arena_, count, category ? category : name_.c_str());
    }
    
private:
    void record_execution_start();
    void record_execution_end(f64 execution_time);
    
    static u32 next_allocator_id();
    static std::atomic<u32> allocator_id_counter_;
};

/**
 * @brief Event-driven system that responds to specific events
 */
template<typename EventType>
class EventSystem : public System {
public:
    explicit EventSystem(const std::string& name)
        : System(name, SystemPhase::Update, SystemExecutionType::Immediate) {}
    
    // Override update to handle events
    void update(const SystemContext& context) override final {
        // Get events of the specified type and process them
        process_events(context);
    }
    
    // Subclasses implement this instead of update
    virtual void on_event(const EventType& event, const SystemContext& context) = 0;
    
private:
    void process_events(const SystemContext& context) {
        // TODO: Get events from event bus and call on_event for each
        // This requires implementing the EventBus system
    }
};

/**
 * @brief System execution group for managing related systems
 */
class SystemGroup {
private:
    std::string name_;
    std::vector<std::unique_ptr<System>> systems_;
    SystemPhase phase_;
    bool is_parallel_;
    f64 total_time_budget_;
    
public:
    explicit SystemGroup(const std::string& name, SystemPhase phase = SystemPhase::Update,
                        bool parallel = false)
        : name_(name), phase_(phase), is_parallel_(parallel), total_time_budget_(0.033) {}  // 33ms default
    
    // System management
    void add_system(std::unique_ptr<System> system);
    void remove_system(const std::string& system_name);
    System* get_system(const std::string& system_name);
    
    // Execution
    void execute_all(const SystemContext& context);
    void execute_parallel(const SystemContext& context);
    void execute_sequential(const SystemContext& context);
    
    // Configuration
    void set_time_budget(f64 budget) { total_time_budget_ = budget; }
    void set_parallel(bool parallel) { is_parallel_ = parallel; }
    
    // Information
    const std::string& name() const noexcept { return name_; }
    SystemPhase phase() const noexcept { return phase_; }
    bool is_parallel() const noexcept { return is_parallel_; }
    usize system_count() const noexcept { return systems_.size(); }
    f64 time_budget() const noexcept { return total_time_budget_; }
    
    // Statistics
    SystemStats get_combined_stats() const;
    std::vector<std::pair<std::string, SystemStats>> get_individual_stats() const;
};

/**
 * @brief Thread pool for parallel system execution
 */
class SystemThreadPool {
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    mutable std::mutex tasks_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_flag_;
    usize thread_count_;
    
public:
    explicit SystemThreadPool(usize thread_count = std::thread::hardware_concurrency());
    ~SystemThreadPool();
    
    // Task submission
    template<typename Func>
    auto submit(Func&& func) -> std::future<decltype(func())> {
        using return_type = decltype(func());
        
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::forward<Func>(func));
        
        std::future<return_type> result = task->get_future();
        
        {
            std::lock_guard<std::mutex> lock(tasks_mutex_);
            if (stop_flag_.load()) {
                throw std::runtime_error("Submit called on stopped ThreadPool");
            }
            
            tasks_.emplace([task]() { (*task)(); });
        }
        
        condition_.notify_one();
        return result;
    }
    
    void wait_for_all();
    usize thread_count() const noexcept { return thread_count_; }
    usize pending_tasks() const;
    
private:
    void worker_thread();
};

/**
 * @brief System execution scheduler with dependency resolution
 */
class SystemScheduler {
private:
    struct ScheduledSystem {
        System* system;
        std::vector<std::string> dependencies;
        std::atomic<bool> ready_to_execute;
        std::atomic<bool> execution_complete;
        f64 scheduled_time;
        u32 priority;
        
        ScheduledSystem(System* sys) 
            : system(sys), ready_to_execute(false), execution_complete(false)
            , scheduled_time(0.0), priority(0) {}
            
        // Copy constructor
        ScheduledSystem(const ScheduledSystem& other) 
            : system(other.system), dependencies(other.dependencies)
            , ready_to_execute(other.ready_to_execute.load())
            , execution_complete(other.execution_complete.load())
            , scheduled_time(other.scheduled_time), priority(other.priority) {}
            
        // Move constructor
        ScheduledSystem(ScheduledSystem&& other) noexcept
            : system(other.system), dependencies(std::move(other.dependencies))
            , ready_to_execute(other.ready_to_execute.load())
            , execution_complete(other.execution_complete.load())
            , scheduled_time(other.scheduled_time), priority(other.priority) {}
            
        // Copy assignment operator
        ScheduledSystem& operator=(const ScheduledSystem& other) {
            if (this != &other) {
                system = other.system;
                dependencies = other.dependencies;
                ready_to_execute.store(other.ready_to_execute.load());
                execution_complete.store(other.execution_complete.load());
                scheduled_time = other.scheduled_time;
                priority = other.priority;
            }
            return *this;
        }
        
        // Move assignment operator
        ScheduledSystem& operator=(ScheduledSystem&& other) noexcept {
            if (this != &other) {
                system = other.system;
                dependencies = std::move(other.dependencies);
                ready_to_execute.store(other.ready_to_execute.load());
                execution_complete.store(other.execution_complete.load());
                scheduled_time = other.scheduled_time;
                priority = other.priority;
            }
            return *this;
        }
    };
    
    std::vector<ScheduledSystem> scheduled_systems_;
    std::unique_ptr<SystemThreadPool> thread_pool_;
    std::mutex scheduling_mutex_;
    
    // Performance tracking
    std::chrono::high_resolution_clock::time_point schedule_start_;
    f64 total_scheduling_time_;
    usize scheduling_iterations_;
    
public:
    explicit SystemScheduler(usize thread_count = std::thread::hardware_concurrency());
    ~SystemScheduler();
    
    // Scheduling
    void add_system(System* system);
    void remove_system(const std::string& system_name);
    void execute_phase(SystemPhase phase, const SystemContext& context);
    
    // Analysis
    std::vector<std::vector<std::string>> get_execution_order(SystemPhase phase) const;
    std::vector<std::string> detect_dependency_cycles() const;
    f64 estimate_phase_execution_time(SystemPhase phase) const;
    
    // Configuration
    void set_thread_count(usize count);
    void optimize_execution_order();
    
    // Statistics
    f64 get_average_scheduling_time() const;
    usize get_system_count() const { return scheduled_systems_.size(); }
    
private:
    void resolve_dependencies(SystemPhase phase, const SystemContext& context);
    bool check_system_ready(const ScheduledSystem& scheduled_system) const;
    void execute_system_parallel(ScheduledSystem& scheduled_system, const SystemContext& context);
    std::vector<System*> get_systems_for_phase(SystemPhase phase) const;
    void topological_sort(std::vector<System*>& systems) const;
    bool has_circular_dependency(const std::vector<System*>& systems) const;
};

/**
 * @brief Main system manager coordinating all ECS systems
 */
class SystemManager {
private:
    Registry* registry_;
    std::unique_ptr<SystemScheduler> scheduler_;
    std::unique_ptr<QueryManager> query_manager_;
    
    // System storage by phase
    std::array<std::vector<std::unique_ptr<System>>, 
               static_cast<usize>(SystemPhase::COUNT)> systems_by_phase_;
    std::unordered_map<std::string, System*> systems_by_name_;
    std::vector<std::unique_ptr<SystemGroup>> system_groups_;
    
    // Execution state
    std::atomic<bool> is_running_;
    std::atomic<u64> current_frame_;
    f64 total_time_;
    
    // Performance tracking
    std::array<f64, static_cast<usize>(SystemPhase::COUNT)> phase_time_budgets_;
    std::array<SystemStats, static_cast<usize>(SystemPhase::COUNT)> phase_stats_;
    
    // Configuration
    bool enable_parallel_execution_;
    bool enable_performance_monitoring_;
    usize max_systems_per_phase_;
    
public:
    explicit SystemManager(Registry* registry);
    ~SystemManager();
    
    // System registration
    template<typename SystemType, typename... Args>
    SystemType* add_system(Args&&... args) {
        auto system = std::make_unique<SystemType>(std::forward<Args>(args)...);
        SystemType* system_ptr = system.get();
        
        SystemPhase phase = system->phase();
        systems_by_phase_[static_cast<usize>(phase)].push_back(std::move(system));
        systems_by_name_[system_ptr->name()] = system_ptr;
        scheduler_->add_system(system_ptr);
        
        LOG_INFO("Added system '{}' to phase {}", system_ptr->name(), static_cast<int>(phase));
        return system_ptr;
    }
    
    void remove_system(const std::string& system_name);
    System* get_system(const std::string& system_name);
    
    // System group management
    SystemGroup* create_system_group(const std::string& name, SystemPhase phase, bool parallel = false);
    void add_system_to_group(const std::string& group_name, std::unique_ptr<System> system);
    SystemGroup* get_system_group(const std::string& name);
    
    // Execution
    void initialize_all_systems();
    void execute_phase(SystemPhase phase, f64 delta_time);
    void shutdown_all_systems();
    
    // Frame execution
    void execute_frame(f64 delta_time);
    void execute_fixed_update(f64 fixed_delta_time);
    
    // Configuration
    void set_parallel_execution(bool enable) { enable_parallel_execution_ = enable; }
    void set_performance_monitoring(bool enable) { enable_performance_monitoring_ = enable; }
    void set_phase_time_budget(SystemPhase phase, f64 budget);
    void set_max_systems_per_phase(usize max_systems) { max_systems_per_phase_ = max_systems; }
    
    // Information
    bool is_running() const noexcept { return is_running_; }
    u64 current_frame() const noexcept { return current_frame_; }
    f64 total_time() const noexcept { return total_time_; }
    usize system_count() const noexcept { return systems_by_name_.size(); }
    usize systems_in_phase(SystemPhase phase) const noexcept { 
        return systems_by_phase_[static_cast<usize>(phase)].size(); 
    }
    
    // Statistics and analysis
    SystemStats get_phase_stats(SystemPhase phase) const;
    std::vector<std::pair<std::string, SystemStats>> get_all_system_stats() const;
    std::vector<std::string> get_slowest_systems(usize count = 10) const;
    std::vector<std::string> get_systems_over_budget() const;
    f64 get_total_system_time() const;
    f64 get_frame_budget_utilization() const;
    
    // Debugging and introspection
    void print_system_execution_order() const;
    void print_system_performance_report() const;
    std::vector<std::string> validate_system_dependencies() const;
    void export_performance_data(const std::string& filename) const;
    
    // Optimization
    void optimize_system_order();
    void balance_system_budgets();
    void detect_performance_bottlenecks() const;
    
private:
    SystemContext create_system_context(SystemPhase phase, f64 delta_time);
    void update_frame_statistics();
    void check_budget_violations(SystemPhase phase);
    void log_performance_warnings();
    
    static const char* phase_name(SystemPhase phase);
};

// Utility macros for system registration
#define REGISTER_SYSTEM(manager, SystemType, ...) \
    (manager).add_system<SystemType>(__VA_ARGS__)

#define DECLARE_SYSTEM(ClassName, PhaseName) \
    class ClassName : public ecscope::ecs::System { \
    public: \
        ClassName() : System(#ClassName, SystemPhase::PhaseName) {} \
        void update(const SystemContext& context) override; \
    };

// Common system base classes
class UpdateSystem : public System {
public:
    explicit UpdateSystem(const std::string& name) 
        : System(name, SystemPhase::Update) {}
};

class RenderSystem : public System {
public:
    explicit RenderSystem(const std::string& name) 
        : System(name, SystemPhase::Render, SystemExecutionType::Sequential) {}
};

class InitializationSystem : public System {
public:
    explicit InitializationSystem(const std::string& name) 
        : System(name, SystemPhase::Initialize) {}
};

class CleanupSystem : public System {
public:
    explicit CleanupSystem(const std::string& name) 
        : System(name, SystemPhase::Cleanup) {}
};

} // namespace ecscope::ecs