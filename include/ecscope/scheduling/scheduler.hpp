#pragma once

/**
 * @file scheduler.hpp
 * @brief Professional-grade system scheduler with advanced dependency management and parallel execution
 * 
 * This is the core scheduling engine that orchestrates system execution with world-class performance
 * and sophisticated scheduling algorithms. Features include:
 * 
 * - Multi-level scheduling with priority queues and phases
 * - Dependency-aware parallel execution with optimal load balancing
 * - Dynamic load balancing with work-stealing and migration
 * - Resource conflict detection and automatic resolution
 * - Adaptive scheduling based on system performance metrics
 * - Multi-frame pipelining for overlapped execution
 * - System execution budgeting with automatic time slicing
 * - Deadlock detection and prevention mechanisms
 * - Hot system registration/unregistration during runtime
 * - Comprehensive performance profiling and visualization
 * - NUMA-aware thread placement and memory allocation
 * - Advanced scheduling policies (round-robin, priority, fair-share)
 * - System state checkpointing and rollback capabilities
 * - Event-driven conditional system execution
 * - Hierarchical system groups with nested scheduling
 * 
 * Scheduling Algorithms:
 * - Priority-based scheduling with multiple priority levels
 * - Fair-share scheduling to prevent system starvation
 * - Earliest deadline first (EDF) for real-time systems
 * - Work-conserving scheduling for maximum utilization
 * - Load-aware scheduling with dynamic thread allocation
 * - Dependency-guided scheduling with critical path optimization
 * 
 * Performance Features:
 * - Sub-millisecond scheduling overhead
 * - Lock-free execution queues where possible
 * - Cache-friendly system batching
 * - SIMD-optimized dependency resolution
 * - Memory pool allocation for scheduler data structures
 * - Thread-local scheduling contexts
 * - Adaptive scheduling parameters based on runtime metrics
 * 
 * @author ECScope Professional ECS Framework
 * @date 2024
 */

#include "../core/types.hpp"
#include "../core/log.hpp"
#include "../core/time.hpp"
#include "thread_pool.hpp"
#include "dependency_graph.hpp"
#include <memory>
#include <vector>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <chrono>
#include <functional>
#include <future>
#include <optional>
#include <array>
#include <span>
#include <string>
#include <algorithm>
#include <thread>

namespace ecscope::scheduling {

// Forward declarations
class System;
class SystemManager;
class ExecutionContext;

/**
 * @brief System execution phase for organizing system execution order
 */
enum class SystemPhase : u8 {
    PreInitialize = 0,
    Initialize = 1,
    PostInitialize = 2,
    EarlyUpdate = 3,
    PreUpdate = 4,
    Update = 5,
    LateUpdate = 6,
    PostUpdate = 7,
    PreRender = 8,
    Render = 9,
    PostRender = 10,
    PreCleanup = 11,
    Cleanup = 12,
    PostCleanup = 13,
    COUNT = 14
};

/**
 * @brief System execution mode for different scheduling strategies
 */
enum class ExecutionMode : u8 {
    Sequential,         // Execute systems one after another
    Parallel,          // Execute systems in parallel where possible
    PipelinedParallel, // Pipeline execution across frames
    WorkStealing,      // Use work-stealing for load balancing
    NUMA_Aware,        // NUMA-optimized execution
    Adaptive          // Dynamically adapt execution mode
};

/**
 * @brief Scheduling policy for different system prioritization strategies
 */
enum class SchedulingPolicy : u8 {
    Priority,          // Priority-based scheduling
    FairShare,         // Fair-share time allocation
    RoundRobin,        // Round-robin system execution
    EarliestDeadline,  // Earliest deadline first (real-time)
    ShortestJobFirst,  // Shortest execution time first
    Adaptive           // Adaptive policy based on metrics
};

/**
 * @brief System execution constraints and requirements
 */
struct ExecutionConstraints {
    f64 max_execution_time = 0.016;    // Maximum execution time (60 FPS = 16ms)
    f64 time_budget = 0.0;             // Allocated time budget
    u32 max_parallel_systems = 0;      // Maximum systems to run in parallel (0 = unlimited)
    u32 preferred_numa_node = ~0u;     // Preferred NUMA node (UINT32_MAX = any)
    u32 required_thread_count = 0;     // Required number of threads (0 = any)
    bool allow_preemption = false;     // Allow system preemption
    bool require_main_thread = false;  // Must execute on main thread
    bool allow_migration = true;       // Allow work migration between threads
    f64 deadline = 0.0;               // Hard deadline for completion
    
    ExecutionConstraints() = default;
    
    bool is_real_time() const { return deadline > 0.0; }
    bool has_time_budget() const { return time_budget > 0.0; }
    bool has_numa_preference() const { return preferred_numa_node != ~0u; }
};

/**
 * @brief System execution statistics and performance metrics
 */
struct ExecutionStatistics {
    // Timing metrics
    f64 total_execution_time = 0.0;
    f64 average_execution_time = 0.0;
    f64 min_execution_time = std::numeric_limits<f64>::max();
    f64 max_execution_time = 0.0;
    f64 last_execution_time = 0.0;
    
    // Scheduling metrics
    u64 total_executions = 0;
    u64 successful_executions = 0;
    u64 failed_executions = 0;
    u64 preempted_executions = 0;
    u64 deadline_misses = 0;
    
    // Resource utilization
    f64 cpu_utilization = 0.0;
    f64 memory_utilization = 0.0;
    f64 cache_hit_rate = 0.0;
    u64 context_switches = 0;
    
    // Parallelization metrics
    f64 parallel_efficiency = 0.0;
    u32 average_parallel_systems = 0;
    u32 max_parallel_systems = 0;
    
    // Dependency metrics
    f64 dependency_wait_time = 0.0;
    u64 dependency_violations = 0;
    f64 critical_path_time = 0.0;
    
    void reset() {
        *this = ExecutionStatistics{};
    }
    
    void record_execution(f64 execution_time, bool successful = true) {
        total_executions++;
        if (successful) {
            successful_executions++;
            total_execution_time += execution_time;
            last_execution_time = execution_time;
            
            if (execution_time < min_execution_time) {
                min_execution_time = execution_time;
            }
            if (execution_time > max_execution_time) {
                max_execution_time = execution_time;
            }
            
            average_execution_time = total_execution_time / successful_executions;
        } else {
            failed_executions++;
        }
    }
    
    f64 success_rate() const {
        return total_executions > 0 ? 
            static_cast<f64>(successful_executions) / total_executions : 0.0;
    }
    
    f64 deadline_miss_rate() const {
        return total_executions > 0 ? 
            static_cast<f64>(deadline_misses) / total_executions : 0.0;
    }
};

/**
 * @brief Scheduled system information for execution planning
 */
class ScheduledSystem {
private:
    System* system_;
    u32 system_id_;
    std::string system_name_;
    SystemPhase phase_;
    ExecutionConstraints constraints_;
    ExecutionStatistics statistics_;
    
    // Scheduling state
    std::atomic<bool> ready_to_execute_{false};
    std::atomic<bool> currently_executing_{false};
    std::atomic<bool> execution_completed_{false};
    std::atomic<u64> last_execution_frame_{0};
    std::atomic<f64> next_execution_time_{0.0};
    
    // Dependencies
    std::vector<u32> dependency_ids_;
    std::vector<u32> dependent_ids_;
    std::atomic<u32> unresolved_dependencies_{0};
    
    // Resource requirements
    std::unordered_set<u32> required_resources_;
    std::unordered_set<u32> exclusive_resources_;
    
    mutable std::mutex system_mutex_;

public:
    ScheduledSystem(System* system, u32 id, const std::string& name, SystemPhase phase);
    
    // Basic accessors
    System* system() const { return system_; }
    u32 id() const { return system_id_; }
    const std::string& name() const { return system_name_; }
    SystemPhase phase() const { return phase_; }
    
    // Execution state
    bool is_ready() const { return ready_to_execute_.load(std::memory_order_acquire); }
    bool is_executing() const { return currently_executing_.load(std::memory_order_acquire); }
    bool is_completed() const { return execution_completed_.load(std::memory_order_acquire); }
    
    void set_ready(bool ready) { ready_to_execute_.store(ready, std::memory_order_release); }
    void set_executing(bool executing) { currently_executing_.store(executing, std::memory_order_release); }
    void set_completed(bool completed) { execution_completed_.store(completed, std::memory_order_release); }
    
    // Frame tracking
    u64 last_execution_frame() const { return last_execution_frame_.load(std::memory_order_relaxed); }
    void set_last_execution_frame(u64 frame) { last_execution_frame_.store(frame, std::memory_order_relaxed); }
    
    f64 next_execution_time() const { return next_execution_time_.load(std::memory_order_relaxed); }
    void set_next_execution_time(f64 time) { next_execution_time_.store(time, std::memory_order_relaxed); }
    
    // Constraints and statistics
    ExecutionConstraints& constraints() { return constraints_; }
    const ExecutionConstraints& constraints() const { return constraints_; }
    ExecutionStatistics& statistics() { return statistics_; }
    const ExecutionStatistics& statistics() const { return statistics_; }
    
    // Dependencies
    void add_dependency(u32 dependency_id);
    void remove_dependency(u32 dependency_id);
    void add_dependent(u32 dependent_id);
    void remove_dependent(u32 dependent_id);
    
    const std::vector<u32>& dependencies() const { return dependency_ids_; }
    const std::vector<u32>& dependents() const { return dependent_ids_; }
    
    u32 unresolved_dependencies() const { 
        return unresolved_dependencies_.load(std::memory_order_acquire); 
    }
    void resolve_dependency() { 
        unresolved_dependencies_.fetch_sub(1, std::memory_order_acq_rel); 
    }
    void reset_dependencies();
    
    // Resources
    void add_required_resource(u32 resource_id) { required_resources_.insert(resource_id); }
    void add_exclusive_resource(u32 resource_id) { exclusive_resources_.insert(resource_id); }
    void remove_required_resource(u32 resource_id) { required_resources_.erase(resource_id); }
    void remove_exclusive_resource(u32 resource_id) { exclusive_resources_.erase(resource_id); }
    
    const std::unordered_set<u32>& required_resources() const { return required_resources_; }
    const std::unordered_set<u32>& exclusive_resources() const { return exclusive_resources_; }
    bool conflicts_with(const ScheduledSystem& other) const;
    
    // Execution planning
    bool can_execute_now(f64 current_time) const;
    bool should_execute_this_frame(u64 frame_number) const;
    f64 estimate_execution_time() const;
    u32 calculate_priority() const;
    
    // State management
    void reset_execution_state();
    void prepare_for_execution();
    void finalize_execution(bool success, f64 execution_time);
};

/**
 * @brief Execution batch for grouping systems that can run together
 */
struct ExecutionBatch {
    std::vector<std::shared_ptr<ScheduledSystem>> systems;
    f64 estimated_time = 0.0;
    f64 actual_time = 0.0;
    u32 parallel_capacity = 1;
    bool requires_main_thread = false;
    u32 preferred_numa_node = ~0u;
    std::unordered_set<u32> required_resources;
    std::unordered_set<u32> exclusive_resources;
    
    ExecutionBatch() = default;
    
    bool can_add_system(const ScheduledSystem& system) const;
    void add_system(std::shared_ptr<ScheduledSystem> system);
    bool is_ready() const;
    f64 get_priority() const;
    usize size() const { return systems.size(); }
    bool empty() const { return systems.empty(); }
};

/**
 * @brief Advanced system scheduler with professional-grade features
 */
class Scheduler {
private:
    // Configuration
    ExecutionMode execution_mode_;
    SchedulingPolicy scheduling_policy_;
    u32 max_thread_count_;
    bool numa_aware_;
    bool enable_profiling_;
    bool enable_pipelining_;
    f64 target_framerate_;
    
    // Core components
    std::unique_ptr<ThreadPool> thread_pool_;
    std::unique_ptr<DependencyGraph> dependency_graph_;
    std::unique_ptr<DependencyResolver> dependency_resolver_;
    
    // System management
    std::unordered_map<u32, std::shared_ptr<ScheduledSystem>> scheduled_systems_;
    std::unordered_map<std::string, u32> system_name_to_id_;
    std::atomic<u32> next_system_id_;
    
    // Phase organization
    std::array<std::vector<u32>, static_cast<usize>(SystemPhase::COUNT)> systems_by_phase_;
    std::array<ExecutionStatistics, static_cast<usize>(SystemPhase::COUNT)> phase_statistics_;
    
    // Execution state
    std::atomic<bool> running_;
    std::atomic<bool> paused_;
    std::atomic<u64> current_frame_;
    std::atomic<f64> frame_start_time_;
    std::atomic<f64> total_time_;
    
    // Scheduling queues
    std::array<std::priority_queue<std::shared_ptr<ScheduledSystem>, 
                                  std::vector<std::shared_ptr<ScheduledSystem>>,
                                  std::function<bool(const std::shared_ptr<ScheduledSystem>&,
                                                   const std::shared_ptr<ScheduledSystem>&)>>,
               static_cast<usize>(SystemPhase::COUNT)> ready_queues_;
    
    // Resource management
    std::unordered_map<std::string, u32> resource_names_;
    std::atomic<u32> next_resource_id_;
    std::unordered_map<u32, std::atomic<bool>> resource_locks_;
    
    // Thread safety
    mutable std::shared_mutex scheduler_mutex_;
    std::array<std::mutex, static_cast<usize>(SystemPhase::COUNT)> phase_mutexes_;
    std::mutex resource_mutex_;
    
    // Performance monitoring
    ExecutionStatistics global_statistics_;
    std::chrono::high_resolution_clock::time_point scheduler_start_time_;
    std::atomic<f64> average_frame_time_;
    std::atomic<f64> frame_time_variance_;
    std::atomic<u32> dropped_frames_;
    
    // Advanced features
    std::queue<ExecutionBatch> pipelined_batches_;
    std::mutex pipeline_mutex_;
    std::unordered_map<u32, std::vector<u8>> system_checkpoints_;
    std::mutex checkpoint_mutex_;
    
    // Event-driven execution
    std::unordered_map<std::string, std::vector<u32>> event_triggered_systems_;
    std::queue<std::string> pending_events_;
    std::mutex event_mutex_;

public:
    explicit Scheduler(u32 thread_count = 0,
                      ExecutionMode mode = ExecutionMode::Parallel,
                      SchedulingPolicy policy = SchedulingPolicy::Priority);
    ~Scheduler();
    
    // Lifecycle
    bool initialize();
    void shutdown();
    bool is_running() const { return running_.load(std::memory_order_acquire); }
    
    // System registration
    u32 register_system(System* system, SystemPhase phase = SystemPhase::Update);
    void unregister_system(u32 system_id);
    void unregister_system(const std::string& system_name);
    bool has_system(u32 system_id) const;
    bool has_system(const std::string& system_name) const;
    ScheduledSystem* get_system(u32 system_id);
    ScheduledSystem* get_system(const std::string& system_name);
    
    // Hot registration/unregistration
    u32 register_system_hot(System* system, SystemPhase phase = SystemPhase::Update);
    void unregister_system_hot(u32 system_id);
    void move_system_to_phase(u32 system_id, SystemPhase new_phase);
    
    // Dependency management
    bool add_system_dependency(u32 source_id, u32 target_id, 
                              DependencyType type = DependencyType::HardBefore,
                              f32 strength = 1.0f);
    bool add_system_dependency(const std::string& source_name, const std::string& target_name,
                              DependencyType type = DependencyType::HardBefore,
                              f32 strength = 1.0f);
    void remove_system_dependency(u32 source_id, u32 target_id);
    void remove_system_dependency(const std::string& source_name, const std::string& target_name);
    
    // Resource management
    u32 register_resource(const std::string& name);
    void add_system_resource_requirement(u32 system_id, u32 resource_id, ResourceAccessType access);
    void add_system_resource_requirement(const std::string& system_name, 
                                        const std::string& resource_name,
                                        ResourceAccessType access);
    bool try_lock_resource(u32 resource_id);
    void unlock_resource(u32 resource_id);
    
    // Execution control
    void execute_frame(f64 delta_time);
    void execute_phase(SystemPhase phase, f64 delta_time);
    std::future<void> execute_phase_async(SystemPhase phase, f64 delta_time);
    void execute_system(u32 system_id, f64 delta_time);
    std::future<void> execute_system_async(u32 system_id, f64 delta_time);
    
    // Advanced execution
    void execute_pipelined_frame(f64 delta_time);
    void execute_with_budget(SystemPhase phase, f64 time_budget, f64 delta_time);
    void execute_until_deadline(SystemPhase phase, f64 deadline, f64 delta_time);
    void pause_execution();
    void resume_execution();
    bool is_paused() const { return paused_.load(std::memory_order_acquire); }
    
    // Event-driven execution
    void register_event_trigger(const std::string& event_name, u32 system_id);
    void unregister_event_trigger(const std::string& event_name, u32 system_id);
    void trigger_event(const std::string& event_name);
    void process_pending_events();
    
    // Configuration
    void set_execution_mode(ExecutionMode mode);
    void set_scheduling_policy(SchedulingPolicy policy);
    void set_thread_count(u32 count);
    void set_numa_aware(bool enabled);
    void set_profiling_enabled(bool enabled);
    void set_pipelining_enabled(bool enabled);
    void set_target_framerate(f64 fps);
    
    // Performance monitoring
    ExecutionStatistics get_global_statistics() const;
    ExecutionStatistics get_phase_statistics(SystemPhase phase) const;
    ExecutionStatistics get_system_statistics(u32 system_id) const;
    std::vector<std::pair<std::string, ExecutionStatistics>> get_all_system_statistics() const;
    
    f64 get_average_frame_time() const { return average_frame_time_.load(std::memory_order_relaxed); }
    f64 get_frame_time_variance() const { return frame_time_variance_.load(std::memory_order_relaxed); }
    u32 get_dropped_frames() const { return dropped_frames_.load(std::memory_order_relaxed); }
    f64 get_cpu_utilization() const;
    
    // System analysis
    std::vector<u32> get_bottleneck_systems() const;
    std::vector<u32> get_underutilized_systems() const;
    TopologicalSortResult get_execution_order(SystemPhase phase) const;
    std::vector<std::string> validate_system_dependencies() const;
    std::string generate_performance_report() const;
    
    // Optimization
    void optimize_execution_order();
    void balance_system_loads();
    void auto_tune_thread_count();
    void adapt_scheduling_parameters();
    
    // Checkpointing
    void create_checkpoint(const std::string& name);
    bool restore_checkpoint(const std::string& name);
    void clear_checkpoints();
    
    // Debugging and visualization
    std::string export_dependency_graph() const;
    void export_performance_trace(const std::string& filename) const;
    void enable_debug_visualization(bool enabled);
    
    // Information
    u32 system_count() const;
    u32 systems_in_phase(SystemPhase phase) const;
    u64 current_frame() const { return current_frame_.load(std::memory_order_relaxed); }
    f64 total_time() const { return total_time_.load(std::memory_order_relaxed); }
    f64 current_frame_time() const;
    
    // Static utilities
    static const char* phase_name(SystemPhase phase);
    static const char* execution_mode_name(ExecutionMode mode);
    static const char* scheduling_policy_name(SchedulingPolicy policy);

private:
    // Internal execution
    void execute_systems_sequential(const std::vector<u32>& system_ids, f64 delta_time);
    void execute_systems_parallel(const std::vector<u32>& system_ids, f64 delta_time);
    void execute_systems_work_stealing(const std::vector<u32>& system_ids, f64 delta_time);
    void execute_systems_numa_aware(const std::vector<u32>& system_ids, f64 delta_time);
    
    // Batch processing
    std::vector<ExecutionBatch> create_execution_batches(const std::vector<u32>& system_ids) const;
    void execute_batch(const ExecutionBatch& batch, f64 delta_time);
    void optimize_batch_order(std::vector<ExecutionBatch>& batches) const;
    
    // Scheduling algorithms
    std::vector<u32> schedule_priority_based(const std::vector<u32>& system_ids) const;
    std::vector<u32> schedule_fair_share(const std::vector<u32>& system_ids) const;
    std::vector<u32> schedule_round_robin(const std::vector<u32>& system_ids) const;
    std::vector<u32> schedule_earliest_deadline(const std::vector<u32>& system_ids) const;
    std::vector<u32> schedule_shortest_job_first(const std::vector<u32>& system_ids) const;
    
    // Resource management
    bool check_resource_availability(const ScheduledSystem& system) const;
    void acquire_system_resources(const ScheduledSystem& system);
    void release_system_resources(const ScheduledSystem& system);
    
    // Performance tracking
    void update_frame_statistics(f64 frame_time);
    void update_system_statistics(u32 system_id, f64 execution_time, bool success);
    void update_phase_statistics(SystemPhase phase, f64 execution_time);
    
    // Optimization helpers
    void detect_and_resolve_deadlocks();
    void optimize_numa_placement();
    void adjust_thread_affinity();
    f64 calculate_system_priority(const ScheduledSystem& system) const;
    
    // Utility functions
    void reset_phase_execution_state(SystemPhase phase);
    void prepare_systems_for_execution(const std::vector<u32>& system_ids);
    void finalize_systems_after_execution(const std::vector<u32>& system_ids, bool success);
    bool are_all_dependencies_satisfied(u32 system_id) const;
    std::vector<u32> get_ready_systems(SystemPhase phase) const;
    
    static f64 get_current_time();
    static u64 get_current_time_ns();
};

} // namespace ecscope::scheduling