#pragma once

/**
 * @file ecs_integration.hpp
 * @brief ECS-Fiber Job System Integration for ECScope Engine
 * 
 * This file provides seamless integration between the ECS architecture and
 * the fiber-based job system, enabling high-performance parallel execution
 * of ECS systems with sophisticated scheduling and dependency management:
 * 
 * - Parallel system execution with automatic dependency resolution
 * - Component-aware job scheduling with memory locality optimization
 * - Entity batch processing with load balancing
 * - System priority and execution order management
 * - Thread-safe component access patterns
 * - Integration with existing ECS scheduler
 * 
 * Key Features:
 * - Automatic parallelization of independent systems
 * - Smart batching of entity processing
 * - Memory access pattern optimization
 * - Fiber-based cooperative multitasking within systems
 * - Real-time performance monitoring and tuning
 * 
 * @author ECScope Engine - ECS-Jobs Integration
 * @date 2025
 */

#include "fiber_job_system.hpp"
#include "ecs/registry.hpp"
#include "ecs/system.hpp"
#include "scheduling/scheduler.hpp"
#include <memory>
#include <vector>
#include <functional>

namespace ecscope::jobs {

//=============================================================================
// Forward Declarations
//=============================================================================

class ECSJobScheduler;
class SystemJob;
class EntityBatch;

//=============================================================================
// ECS System Integration Types
//=============================================================================

/**
 * @brief System execution strategy for job scheduling
 */
enum class SystemExecutionStrategy : u8 {
    Sequential = 0,     // Execute system sequentially on main thread
    Parallel = 1,       // Execute system in parallel across workers
    Pipeline = 2,       // Pipeline execution with entity batches
    Adaptive = 3        // Automatically choose best strategy
};

/**
 * @brief System job configuration
 */
struct SystemJobConfig {
    SystemExecutionStrategy strategy = SystemExecutionStrategy::Adaptive;
    u32 batch_size = 1000;           // Entities per batch for parallel processing
    u32 min_entities_for_parallel = 100;  // Minimum entities to enable parallelism
    JobPriority priority = JobPriority::Normal;
    FiberStackConfig fiber_config = FiberStackConfig{};
    bool enable_profiling = true;
    std::string debug_name;
    
    // Memory locality hints
    bool prefer_component_locality = true;
    u32 cache_line_alignment = 64;
    
    // Performance tuning
    bool enable_vectorization = true;
    bool enable_prefetching = true;
    u32 prefetch_distance = 16;
    
    static SystemJobConfig create_compute_intensive() {
        SystemJobConfig config;
        config.strategy = SystemExecutionStrategy::Parallel;
        config.batch_size = 500;
        config.fiber_config = FiberStackConfig::large();
        config.enable_vectorization = true;
        return config;
    }
    
    static SystemJobConfig create_memory_intensive() {
        SystemJobConfig config;
        config.strategy = SystemExecutionStrategy::Pipeline;
        config.batch_size = 2000;
        config.prefer_component_locality = true;
        config.enable_prefetching = true;
        return config;
    }
    
    static SystemJobConfig create_lightweight() {
        SystemJobConfig config;
        config.strategy = SystemExecutionStrategy::Sequential;
        config.fiber_config = FiberStackConfig::small();
        config.enable_profiling = false;
        return config;
    }
};

//=============================================================================
// Entity Batch Processing
//=============================================================================

/**
 * @brief Entity batch for parallel processing
 */
class EntityBatch {
private:
    std::vector<Entity> entities_;
    u32 batch_id_;
    SystemJobConfig config_;
    
    // Component access patterns
    std::vector<ComponentTypeID> read_components_;
    std::vector<ComponentTypeID> write_components_;
    
    // Memory layout optimization
    bool is_memory_optimized_ = false;
    std::vector<void*> component_arrays_;
    
public:
    EntityBatch(std::vector<Entity> entities, u32 batch_id, const SystemJobConfig& config);
    
    // Entity access
    const std::vector<Entity>& entities() const noexcept { return entities_; }
    usize size() const noexcept { return entities_.size(); }
    bool empty() const noexcept { return entities_.empty(); }
    u32 batch_id() const noexcept { return batch_id_; }
    
    // Component access patterns
    void declare_read_access(ComponentTypeID type_id);
    void declare_write_access(ComponentTypeID type_id);
    
    // Memory optimization
    void optimize_memory_layout(Registry& registry);
    bool is_memory_optimized() const noexcept { return is_memory_optimized_; }
    
    // Batch processing
    template<typename Func>
    void process_entities(Registry& registry, Func&& processor);
    
    template<typename Func>
    void process_entities_parallel(Registry& registry, FiberJobSystem& job_system, Func&& processor);
    
private:
    void prefetch_components(Registry& registry, usize start_index, usize count);
};

//=============================================================================
// System Job Implementation
//=============================================================================

/**
 * @brief ECS System wrapped as a fiber job
 */
class SystemJob {
private:
    std::string system_name_;
    SystemJobConfig config_;
    std::function<void(Registry&, f32)> system_function_;
    
    // System metadata
    ComponentMask read_mask_;
    ComponentMask write_mask_;
    std::vector<ComponentTypeID> component_dependencies_;
    
    // Execution context
    Registry* registry_;
    f32 delta_time_;
    
    // Performance tracking
    std::chrono::high_resolution_clock::time_point last_execution_time_;
    f64 average_execution_time_us_ = 0.0;
    u64 execution_count_ = 0;
    
    // Entity batching
    std::vector<EntityBatch> entity_batches_;
    u32 next_batch_id_ = 0;
    
public:
    SystemJob(std::string name, SystemJobConfig config, 
             std::function<void(Registry&, f32)> system_func);
    
    // System metadata
    const std::string& name() const noexcept { return system_name_; }
    const SystemJobConfig& config() const noexcept { return config_; }
    const ComponentMask& read_mask() const noexcept { return read_mask_; }
    const ComponentMask& write_mask() const noexcept { return write_mask_; }
    
    // Dependency management
    void add_component_dependency(ComponentTypeID type_id, bool is_write_access);
    bool has_dependency_conflict(const SystemJob& other) const;
    std::vector<ComponentTypeID> get_dependencies() const { return component_dependencies_; }
    
    // Execution
    void execute(Registry& registry, f32 delta_time, FiberJobSystem& job_system);
    void execute_sequential(Registry& registry, f32 delta_time);
    void execute_parallel(Registry& registry, f32 delta_time, FiberJobSystem& job_system);
    void execute_pipeline(Registry& registry, f32 delta_time, FiberJobSystem& job_system);
    
    // Entity batching
    void create_entity_batches(Registry& registry);
    void optimize_batches_for_locality(Registry& registry);
    
    // Performance monitoring
    f64 average_execution_time() const noexcept { return average_execution_time_us_; }
    u64 execution_count() const noexcept { return execution_count_; }
    
    // Configuration
    void set_execution_strategy(SystemExecutionStrategy strategy) { config_.strategy = strategy; }
    void set_batch_size(u32 batch_size) { config_.batch_size = batch_size; }
    
private:
    void update_performance_stats(std::chrono::microseconds execution_time);
    SystemExecutionStrategy choose_optimal_strategy(Registry& registry) const;
};

//=============================================================================
// ECS Job Scheduler
//=============================================================================

/**
 * @brief Advanced ECS scheduler integrated with fiber job system
 */
class ECSJobScheduler {
public:
    struct SchedulerConfig {
        // Job system integration
        bool use_fiber_jobs = true;
        u32 max_parallel_systems = 16;
        u32 entity_batch_size = 1000;
        
        // Performance optimization
        bool enable_adaptive_batching = true;
        bool enable_memory_optimization = true;
        bool enable_dependency_analysis = true;
        bool enable_load_balancing = true;
        
        // Profiling and monitoring
        bool enable_system_profiling = true;
        bool enable_dependency_visualization = false;
        std::chrono::milliseconds profiling_interval{1000};
        
        // Execution tuning
        f32 load_balance_threshold = 0.8f;
        u32 min_entities_for_parallel = 100;
        std::chrono::microseconds max_system_execution_time{10000}; // 10ms
        
        static SchedulerConfig create_high_performance() {
            SchedulerConfig config;
            config.use_fiber_jobs = true;
            config.enable_adaptive_batching = true;
            config.enable_memory_optimization = true;
            config.enable_load_balancing = true;
            config.max_parallel_systems = 32;
            return config;
        }
        
        static SchedulerConfig create_debug() {
            SchedulerConfig config;
            config.enable_system_profiling = true;
            config.enable_dependency_visualization = true;
            config.max_parallel_systems = 4;
            return config;
        }
    };
    
private:
    SchedulerConfig config_;
    std::unique_ptr<FiberJobSystem> job_system_;
    std::unique_ptr<JobDependencyGraph> system_dependencies_;
    
    // System management
    std::vector<std::unique_ptr<SystemJob>> systems_;
    std::unordered_map<std::string, usize> system_name_to_index_;
    
    // Execution scheduling
    std::vector<std::vector<usize>> execution_phases_;
    bool scheduling_dirty_ = true;
    
    // Performance monitoring
    std::unique_ptr<JobProfiler> profiler_;
    std::chrono::steady_clock::time_point last_frame_time_;
    f64 average_frame_time_ms_ = 16.67; // 60 FPS default
    
    // Registry integration
    Registry* registry_ = nullptr;
    
public:
    explicit ECSJobScheduler(const SchedulerConfig& config = SchedulerConfig{});
    ~ECSJobScheduler();
    
    // Non-copyable, non-movable
    ECSJobScheduler(const ECSJobScheduler&) = delete;
    ECSJobScheduler& operator=(const ECSJobScheduler&) = delete;
    ECSJobScheduler(ECSJobScheduler&&) = delete;
    ECSJobScheduler& operator=(ECSJobScheduler&&) = delete;
    
    // Initialization
    bool initialize(Registry& registry);
    void shutdown();
    
    // System management
    template<typename SystemFunc>
    void register_system(const std::string& name, SystemFunc&& system_func,
                        const SystemJobConfig& config = SystemJobConfig{});
    
    void remove_system(const std::string& name);
    bool has_system(const std::string& name) const;
    SystemJob* get_system(const std::string& name);
    
    // System dependencies
    void add_system_dependency(const std::string& dependent_system, 
                              const std::string& dependency_system);
    void remove_system_dependency(const std::string& dependent_system, 
                                 const std::string& dependency_system);
    
    // Execution
    void update(f32 delta_time);
    void execute_systems_sequential(f32 delta_time);
    void execute_systems_parallel(f32 delta_time);
    
    // Performance optimization
    void optimize_system_scheduling();
    void balance_system_loads();
    void analyze_system_dependencies();
    
    // Configuration
    void set_max_parallel_systems(u32 max_systems) { config_.max_parallel_systems = max_systems; }
    void set_entity_batch_size(u32 batch_size) { config_.entity_batch_size = batch_size; }
    void enable_adaptive_batching(bool enable) { config_.enable_adaptive_batching = enable; }
    
    // Statistics and monitoring
    struct SchedulerStats {
        u32 total_systems = 0;
        u32 parallel_systems = 0;
        u32 sequential_systems = 0;
        u32 execution_phases = 0;
        
        f64 average_frame_time_ms = 0.0;
        f64 system_execution_time_ms = 0.0;
        f64 scheduling_overhead_ms = 0.0;
        f64 parallelism_efficiency = 0.0;
        
        u32 entities_processed_per_frame = 0;
        f64 entities_per_second = 0.0;
        
        std::vector<std::pair<std::string, f64>> system_execution_times;
        std::vector<std::pair<std::string, u32>> system_entity_counts;
    };
    
    SchedulerStats get_statistics() const;
    std::string generate_performance_report() const;
    std::string export_dependency_graph() const;
    
    // Integration with existing scheduler
    static void integrate_with_ecs_scheduler(scheduling::Scheduler& ecs_scheduler,
                                           ECSJobScheduler& job_scheduler);
    
private:
    // System scheduling
    void rebuild_execution_phases();
    void determine_system_execution_strategies();
    std::vector<usize> find_independent_systems() const;
    
    // Dependency analysis
    void analyze_component_dependencies();
    bool systems_can_run_in_parallel(usize system1, usize system2) const;
    
    // Performance optimization
    void optimize_entity_batching();
    void balance_batch_sizes();
    
    // Profiling integration
    void update_performance_metrics();
    
    // Helper functions
    SystemExecutionStrategy choose_execution_strategy(const SystemJob& system) const;
    u32 calculate_optimal_batch_size(const SystemJob& system) const;
};

//=============================================================================
// Template Implementations
//=============================================================================

template<typename Func>
void EntityBatch::process_entities(Registry& registry, Func&& processor) {
    if (config_.enable_prefetching && entities_.size() > config_.prefetch_distance) {
        // Prefetch component data for better cache performance
        prefetch_components(registry, 0, std::min(static_cast<usize>(config_.prefetch_distance), 
                                                  entities_.size()));
    }
    
    for (usize i = 0; i < entities_.size(); ++i) {
        // Prefetch next batch of entities
        if (config_.enable_prefetching && (i + config_.prefetch_distance) < entities_.size()) {
            prefetch_components(registry, i + config_.prefetch_distance, 1);
        }
        
        processor(registry, entities_[i]);
    }
}

template<typename Func>
void EntityBatch::process_entities_parallel(Registry& registry, FiberJobSystem& job_system, 
                                           Func&& processor) {
    if (entities_.size() < 100) {
        // Small batch - process sequentially
        process_entities(registry, processor);
        return;
    }
    
    // Split batch into sub-batches for parallel processing
    const usize num_workers = job_system.worker_count();
    const usize entities_per_worker = (entities_.size() + num_workers - 1) / num_workers;
    
    std::vector<JobID> sub_jobs;
    sub_jobs.reserve(num_workers);
    
    for (usize worker = 0; worker < num_workers; ++worker) {
        usize start_idx = worker * entities_per_worker;
        usize end_idx = std::min(start_idx + entities_per_worker, entities_.size());
        
        if (start_idx >= end_idx) break;
        
        std::string job_name = "EntityBatch_" + std::to_string(batch_id_) + "_" + std::to_string(worker);
        
        JobID job_id = job_system.submit_job(job_name, [this, &registry, processor, start_idx, end_idx]() {
            for (usize i = start_idx; i < end_idx; ++i) {
                processor(registry, entities_[i]);
            }
        }, JobPriority::Normal, JobAffinity::WorkerThread);
        
        if (job_id.is_valid()) {
            sub_jobs.push_back(job_id);
        }
    }
    
    // Wait for all sub-jobs to complete
    job_system.wait_for_batch(sub_jobs);
}

template<typename SystemFunc>
void ECSJobScheduler::register_system(const std::string& name, SystemFunc&& system_func,
                                     const SystemJobConfig& config) {
    if (has_system(name)) {
        remove_system(name);
    }
    
    auto system_job = std::make_unique<SystemJob>(name, config, std::forward<SystemFunc>(system_func));
    
    usize index = systems_.size();
    system_name_to_index_[name] = index;
    systems_.push_back(std::move(system_job));
    
    scheduling_dirty_ = true;
}

} // namespace ecscope::jobs