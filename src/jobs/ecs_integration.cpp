/**
 * @file ecs_integration.cpp
 * @brief ECS-Fiber Job System Integration Implementation
 * 
 * This file implements seamless integration between the ECS architecture and
 * the fiber-based job system, enabling high-performance parallel execution
 * of ECS systems with sophisticated scheduling and dependency management.
 * 
 * @author ECScope Engine - ECS-Jobs Integration
 * @date 2025
 */

#include "jobs/ecs_integration.hpp"
#include "jobs/fiber_job_system.hpp"
#include "core/log.hpp"
#include <algorithm>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace ecscope::jobs {

//=============================================================================
// EntityBatch Implementation
//=============================================================================

EntityBatch::EntityBatch(std::vector<Entity> entities, u32 batch_id, const SystemJobConfig& config)
    : entities_(std::move(entities))
    , batch_id_(batch_id)
    , config_(config) {
    
    component_arrays_.reserve(16); // Pre-allocate for common case
}

void EntityBatch::declare_read_access(ComponentTypeID type_id) {
    if (std::find(read_components_.begin(), read_components_.end(), type_id) == read_components_.end()) {
        read_components_.push_back(type_id);
    }
}

void EntityBatch::declare_write_access(ComponentTypeID type_id) {
    if (std::find(write_components_.begin(), write_components_.end(), type_id) == write_components_.end()) {
        write_components_.push_back(type_id);
    }
}

void EntityBatch::optimize_memory_layout(Registry& registry) {
    if (is_memory_optimized_ || entities_.empty()) {
        return;
    }
    
    // Sort entities by archetype for better memory locality
    std::sort(entities_.begin(), entities_.end(), [&registry](Entity a, Entity b) {
        return registry.get_archetype_id(a) < registry.get_archetype_id(b);
    });
    
    // Pre-fetch component arrays for better cache performance
    component_arrays_.clear();
    for (ComponentTypeID type_id : read_components_) {
        if (auto* pool = registry.get_component_pool(type_id)) {
            component_arrays_.push_back(pool->raw_data());
        }
    }
    
    for (ComponentTypeID type_id : write_components_) {
        if (auto* pool = registry.get_component_pool(type_id)) {
            component_arrays_.push_back(pool->raw_data());
        }
    }
    
    is_memory_optimized_ = true;
}

void EntityBatch::prefetch_components(Registry& registry, usize start_index, usize count) {
    if (!config_.enable_prefetching || start_index >= entities_.size()) {
        return;
    }
    
    usize end_index = std::min(start_index + count, entities_.size());
    
    // Prefetch component data for upcoming entities
    for (usize i = start_index; i < end_index; ++i) {
        Entity entity = entities_[i];
        
        // Prefetch read components
        for (ComponentTypeID type_id : read_components_) {
            if (auto* component = registry.try_get_component_raw(entity, type_id)) {
                __builtin_prefetch(component, 0, 1); // Read prefetch with low temporal locality
            }
        }
        
        // Prefetch write components
        for (ComponentTypeID type_id : write_components_) {
            if (auto* component = registry.try_get_component_raw(entity, type_id)) {
                __builtin_prefetch(component, 1, 1); // Write prefetch with low temporal locality
            }
        }
    }
}

//=============================================================================
// SystemJob Implementation
//=============================================================================

SystemJob::SystemJob(std::string name, SystemJobConfig config, 
                     std::function<void(Registry&, f32)> system_func)
    : system_name_(std::move(name))
    , config_(config)
    , system_function_(std::move(system_func))
    , registry_(nullptr)
    , delta_time_(0.0f) {
    
    if (config_.debug_name.empty()) {
        config_.debug_name = system_name_;
    }
}

void SystemJob::add_component_dependency(ComponentTypeID type_id, bool is_write_access) {
    if (is_write_access) {
        write_mask_.set(type_id);
    } else {
        read_mask_.set(type_id);
    }
    
    if (std::find(component_dependencies_.begin(), component_dependencies_.end(), type_id) 
        == component_dependencies_.end()) {
        component_dependencies_.push_back(type_id);
    }
}

bool SystemJob::has_dependency_conflict(const SystemJob& other) const {
    // Check for write-write conflicts
    if ((write_mask_ & other.write_mask_).any()) {
        return true;
    }
    
    // Check for read-write conflicts
    if ((read_mask_ & other.write_mask_).any() || (write_mask_ & other.read_mask_).any()) {
        return true;
    }
    
    return false;
}

void SystemJob::execute(Registry& registry, f32 delta_time, FiberJobSystem& job_system) {
    registry_ = &registry;
    delta_time_ = delta_time;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Choose execution strategy if adaptive
    SystemExecutionStrategy strategy = config_.strategy;
    if (strategy == SystemExecutionStrategy::Adaptive) {
        strategy = choose_optimal_strategy(registry);
    }
    
    // Execute based on chosen strategy
    switch (strategy) {
        case SystemExecutionStrategy::Sequential:
            execute_sequential(registry, delta_time);
            break;
            
        case SystemExecutionStrategy::Parallel:
            execute_parallel(registry, delta_time, job_system);
            break;
            
        case SystemExecutionStrategy::Pipeline:
            execute_pipeline(registry, delta_time, job_system);
            break;
            
        case SystemExecutionStrategy::Adaptive:
            // Should not reach here
            execute_sequential(registry, delta_time);
            break;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto execution_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    update_performance_stats(execution_time);
    last_execution_time_ = end_time;
}

void SystemJob::execute_sequential(Registry& registry, f32 delta_time) {
    if (system_function_) {
        system_function_(registry, delta_time);
    }
}

void SystemJob::execute_parallel(Registry& registry, f32 delta_time, FiberJobSystem& job_system) {
    // Create entity batches if not already done
    if (entity_batches_.empty()) {
        create_entity_batches(registry);
    }
    
    // Submit parallel jobs for each batch
    std::vector<JobID> batch_jobs;
    batch_jobs.reserve(entity_batches_.size());
    
    for (auto& batch : entity_batches_) {
        std::string job_name = system_name_ + "_Batch_" + std::to_string(batch.batch_id());
        
        JobID job_id = job_system.submit_job(job_name, [this, &registry, &batch, delta_time]() {
            batch.process_entities(registry, [this, &registry, delta_time](Registry& reg, Entity entity) {
                // Execute system logic on individual entity
                // This is a simplified approach - real systems would have more specific logic
                if (system_function_) {
                    system_function_(reg, delta_time);
                }
            });
        }, config_.priority, JobAffinity::WorkerThread, config_.fiber_config);
        
        if (job_id.is_valid()) {
            batch_jobs.push_back(job_id);
        }
    }
    
    // Wait for all batch jobs to complete
    if (!batch_jobs.empty()) {
        job_system.wait_for_batch(batch_jobs);
    }
}

void SystemJob::execute_pipeline(Registry& registry, f32 delta_time, FiberJobSystem& job_system) {
    // Pipeline execution: process entities in stages
    if (entity_batches_.empty()) {
        create_entity_batches(registry);
    }
    
    // For pipeline execution, we process batches with dependencies
    for (usize stage = 0; stage < entity_batches_.size(); ++stage) {
        auto& batch = entity_batches_[stage];
        
        std::string job_name = system_name_ + "_Pipeline_" + std::to_string(stage);
        
        JobID job_id = job_system.submit_job(job_name, [this, &registry, &batch, delta_time]() {
            batch.process_entities(registry, [this, &registry, delta_time](Registry& reg, Entity entity) {
                if (system_function_) {
                    system_function_(reg, delta_time);
                }
            });
        }, config_.priority, JobAffinity::WorkerThread, config_.fiber_config);
        
        if (job_id.is_valid()) {
            // Wait for this stage before proceeding to next
            job_system.wait_for_job(job_id);
        }
    }
}

void SystemJob::create_entity_batches(Registry& registry) {
    entity_batches_.clear();
    next_batch_id_ = 0;
    
    // Get all entities that match this system's component requirements
    std::vector<Entity> matching_entities;
    
    // Simple approach: get all entities and filter
    // In a real implementation, this would use the registry's query system
    auto all_entities = registry.get_all_entities();
    
    for (Entity entity : all_entities) {
        bool matches = true;
        
        // Check if entity has all required components
        for (ComponentTypeID type_id : component_dependencies_) {
            if (!registry.has_component(entity, type_id)) {
                matches = false;
                break;
            }
        }
        
        if (matches) {
            matching_entities.push_back(entity);
        }
    }
    
    // Create batches from matching entities
    if (matching_entities.empty()) {
        return;
    }
    
    usize entities_per_batch = config_.batch_size;
    usize num_batches = (matching_entities.size() + entities_per_batch - 1) / entities_per_batch;
    
    entity_batches_.reserve(num_batches);
    
    for (usize batch_idx = 0; batch_idx < num_batches; ++batch_idx) {
        usize start_idx = batch_idx * entities_per_batch;
        usize end_idx = std::min(start_idx + entities_per_batch, matching_entities.size());
        
        std::vector<Entity> batch_entities(matching_entities.begin() + start_idx,
                                          matching_entities.begin() + end_idx);
        
        EntityBatch batch(std::move(batch_entities), next_batch_id_++, config_);
        
        // Declare component access patterns
        for (ComponentTypeID type_id : component_dependencies_) {
            if (read_mask_.test(type_id)) {
                batch.declare_read_access(type_id);
            }
            if (write_mask_.test(type_id)) {
                batch.declare_write_access(type_id);
            }
        }
        
        entity_batches_.push_back(std::move(batch));
    }
}

void SystemJob::optimize_batches_for_locality(Registry& registry) {
    for (auto& batch : entity_batches_) {
        batch.optimize_memory_layout(registry);
    }
}

void SystemJob::update_performance_stats(std::chrono::microseconds execution_time) {
    f64 execution_time_us = static_cast<f64>(execution_time.count());
    
    if (execution_count_ == 0) {
        average_execution_time_us_ = execution_time_us;
    } else {
        // Exponential moving average
        f64 alpha = 0.1;
        average_execution_time_us_ = alpha * execution_time_us + (1.0 - alpha) * average_execution_time_us_;
    }
    
    ++execution_count_;
}

SystemExecutionStrategy SystemJob::choose_optimal_strategy(Registry& registry) const {
    // Get entity count that would be processed by this system
    usize entity_count = 0;
    auto all_entities = registry.get_all_entities();
    
    for (Entity entity : all_entities) {
        bool matches = true;
        for (ComponentTypeID type_id : component_dependencies_) {
            if (!registry.has_component(entity, type_id)) {
                matches = false;
                break;
            }
        }
        if (matches) {
            ++entity_count;
        }
    }
    
    // Choose strategy based on entity count and system characteristics
    if (entity_count < config_.min_entities_for_parallel) {
        return SystemExecutionStrategy::Sequential;
    }
    
    if (entity_count > config_.batch_size * 4) {
        return SystemExecutionStrategy::Pipeline;
    }
    
    return SystemExecutionStrategy::Parallel;
}

//=============================================================================
// ECSJobScheduler Implementation
//=============================================================================

ECSJobScheduler::ECSJobScheduler(const SchedulerConfig& config)
    : config_(config) {
    
    if (config_.use_fiber_jobs) {
        FiberJobSystemConfig job_config;
        job_config.num_workers = std::min(config_.max_parallel_systems, 
                                         std::thread::hardware_concurrency());
        job_config.enable_work_stealing = true;
        job_config.enable_profiling = config_.enable_system_profiling;
        
        job_system_ = std::make_unique<FiberJobSystem>(job_config);
    }
    
    if (config_.enable_dependency_analysis) {
        system_dependencies_ = std::make_unique<JobDependencyGraph>();
    }
    
    if (config_.enable_system_profiling) {
        profiler_ = std::make_unique<JobProfiler>();
    }
}

ECSJobScheduler::~ECSJobScheduler() {
    shutdown();
}

bool ECSJobScheduler::initialize(Registry& registry) {
    registry_ = &registry;
    
    if (job_system_ && !job_system_->initialize()) {
        LOG_ERROR("Failed to initialize fiber job system");
        return false;
    }
    
    if (profiler_ && !profiler_->initialize()) {
        LOG_WARN("Failed to initialize job profiler");
    }
    
    last_frame_time_ = std::chrono::steady_clock::now();
    
    return true;
}

void ECSJobScheduler::shutdown() {
    if (job_system_) {
        job_system_->shutdown();
    }
    
    if (profiler_) {
        profiler_->shutdown();
    }
    
    systems_.clear();
    system_name_to_index_.clear();
    execution_phases_.clear();
}

void ECSJobScheduler::remove_system(const std::string& name) {
    auto it = system_name_to_index_.find(name);
    if (it == system_name_to_index_.end()) {
        return;
    }
    
    usize index = it->second;
    
    // Remove from systems vector
    systems_.erase(systems_.begin() + index);
    
    // Update indices for systems after the removed one
    system_name_to_index_.erase(it);
    for (auto& pair : system_name_to_index_) {
        if (pair.second > index) {
            --pair.second;
        }
    }
    
    scheduling_dirty_ = true;
}

bool ECSJobScheduler::has_system(const std::string& name) const {
    return system_name_to_index_.find(name) != system_name_to_index_.end();
}

SystemJob* ECSJobScheduler::get_system(const std::string& name) {
    auto it = system_name_to_index_.find(name);
    if (it != system_name_to_index_.end()) {
        return systems_[it->second].get();
    }
    return nullptr;
}

void ECSJobScheduler::add_system_dependency(const std::string& dependent_system, 
                                           const std::string& dependency_system) {
    if (!system_dependencies_) {
        return;
    }
    
    SystemJob* dependent = get_system(dependent_system);
    SystemJob* dependency = get_system(dependency_system);
    
    if (dependent && dependency) {
        // Add dependency edge
        system_dependencies_->add_dependency(dependent_system, dependency_system);
        scheduling_dirty_ = true;
    }
}

void ECSJobScheduler::remove_system_dependency(const std::string& dependent_system, 
                                              const std::string& dependency_system) {
    if (!system_dependencies_) {
        return;
    }
    
    system_dependencies_->remove_dependency(dependent_system, dependency_system);
    scheduling_dirty_ = true;
}

void ECSJobScheduler::update(f32 delta_time) {
    if (!registry_) {
        return;
    }
    
    auto frame_start = std::chrono::steady_clock::now();
    
    // Rebuild execution phases if needed
    if (scheduling_dirty_) {
        rebuild_execution_phases();
        scheduling_dirty_ = false;
    }
    
    // Choose execution method based on configuration
    if (config_.use_fiber_jobs && job_system_) {
        execute_systems_parallel(delta_time);
    } else {
        execute_systems_sequential(delta_time);
    }
    
    // Update performance metrics
    auto frame_end = std::chrono::steady_clock::now();
    auto frame_time = std::chrono::duration_cast<std::chrono::microseconds>(frame_end - frame_start);
    
    f64 frame_time_ms = static_cast<f64>(frame_time.count()) / 1000.0;
    average_frame_time_ms_ = 0.9 * average_frame_time_ms_ + 0.1 * frame_time_ms;
    
    last_frame_time_ = frame_end;
    
    if (profiler_) {
        update_performance_metrics();
    }
}

void ECSJobScheduler::execute_systems_sequential(f32 delta_time) {
    for (const auto& phase : execution_phases_) {
        for (usize system_index : phase) {
            if (system_index < systems_.size()) {
                auto& system = systems_[system_index];
                if (job_system_) {
                    system->execute(*registry_, delta_time, *job_system_);
                } else {
                    system->execute_sequential(*registry_, delta_time);
                }
            }
        }
    }
}

void ECSJobScheduler::execute_systems_parallel(f32 delta_time) {
    if (!job_system_) {
        execute_systems_sequential(delta_time);
        return;
    }
    
    for (const auto& phase : execution_phases_) {
        std::vector<JobID> phase_jobs;
        phase_jobs.reserve(phase.size());
        
        // Submit all systems in this phase in parallel
        for (usize system_index : phase) {
            if (system_index < systems_.size()) {
                auto& system = systems_[system_index];
                
                std::string job_name = "System_" + system->name();
                
                JobID job_id = job_system_->submit_job(job_name, [&system, this, delta_time]() {
                    system->execute(*registry_, delta_time, *job_system_);
                }, system->config().priority, JobAffinity::WorkerThread);
                
                if (job_id.is_valid()) {
                    phase_jobs.push_back(job_id);
                }
            }
        }
        
        // Wait for all systems in this phase to complete
        if (!phase_jobs.empty()) {
            job_system_->wait_for_batch(phase_jobs);
        }
    }
}

void ECSJobScheduler::optimize_system_scheduling() {
    if (config_.enable_dependency_analysis) {
        analyze_system_dependencies();
    }
    
    if (config_.enable_adaptive_batching) {
        optimize_entity_batching();
    }
    
    if (config_.enable_load_balancing) {
        balance_system_loads();
    }
    
    rebuild_execution_phases();
}

void ECSJobScheduler::balance_system_loads() {
    // Analyze system execution times and adjust batch sizes
    for (auto& system : systems_) {
        f64 avg_time = system->average_execution_time();
        
        if (avg_time > config_.max_system_execution_time.count()) {
            // System is too slow - increase parallelization
            u32 new_batch_size = system->config().batch_size / 2;
            if (new_batch_size >= 100) {
                system->set_batch_size(new_batch_size);
                system->set_execution_strategy(SystemExecutionStrategy::Parallel);
            }
        } else if (avg_time < 1000.0) { // Less than 1ms
            // System is fast - reduce overhead
            system->set_execution_strategy(SystemExecutionStrategy::Sequential);
        }
    }
}

void ECSJobScheduler::analyze_system_dependencies() {
    if (!system_dependencies_) {
        return;
    }
    
    // Analyze component dependencies between systems
    for (usize i = 0; i < systems_.size(); ++i) {
        for (usize j = i + 1; j < systems_.size(); ++j) {
            const auto& system1 = systems_[i];
            const auto& system2 = systems_[j];
            
            if (system1->has_dependency_conflict(*system2)) {
                // Add implicit dependency to prevent conflicts
                system_dependencies_->add_dependency(system2->name(), system1->name());
            }
        }
    }
}

ECSJobScheduler::SchedulerStats ECSJobScheduler::get_statistics() const {
    SchedulerStats stats;
    
    stats.total_systems = static_cast<u32>(systems_.size());
    stats.execution_phases = static_cast<u32>(execution_phases_.size());
    stats.average_frame_time_ms = average_frame_time_ms_;
    
    // Count system types
    for (const auto& system : systems_) {
        if (system->config().strategy == SystemExecutionStrategy::Sequential) {
            ++stats.sequential_systems;
        } else {
            ++stats.parallel_systems;
        }
        
        // Add system execution times
        stats.system_execution_times.emplace_back(system->name(), system->average_execution_time() / 1000.0);
    }
    
    // Calculate parallelism efficiency
    if (stats.total_systems > 0) {
        stats.parallelism_efficiency = static_cast<f64>(stats.parallel_systems) / stats.total_systems;
    }
    
    return stats;
}

std::string ECSJobScheduler::generate_performance_report() const {
    std::ostringstream report;
    
    auto stats = get_statistics();
    
    report << "=== ECS Job Scheduler Performance Report ===\n";
    report << "Total Systems: " << stats.total_systems << "\n";
    report << "Parallel Systems: " << stats.parallel_systems << "\n";
    report << "Sequential Systems: " << stats.sequential_systems << "\n";
    report << "Execution Phases: " << stats.execution_phases << "\n";
    report << "Average Frame Time: " << std::fixed << std::setprecision(2) 
           << stats.average_frame_time_ms << " ms\n";
    report << "Parallelism Efficiency: " << std::fixed << std::setprecision(1) 
           << (stats.parallelism_efficiency * 100.0) << "%\n\n";
    
    report << "System Execution Times:\n";
    for (const auto& [name, time] : stats.system_execution_times) {
        report << "  " << name << ": " << std::fixed << std::setprecision(3) 
               << time << " ms\n";
    }
    
    if (job_system_) {
        report << "\n" << job_system_->get_statistics_report();
    }
    
    return report.str();
}

std::string ECSJobScheduler::export_dependency_graph() const {
    if (!system_dependencies_) {
        return "Dependency analysis disabled";
    }
    
    return system_dependencies_->export_to_dot();
}

void ECSJobScheduler::integrate_with_ecs_scheduler(scheduling::Scheduler& ecs_scheduler,
                                                  ECSJobScheduler& job_scheduler) {
    // Integration implementation would depend on the specific ECS scheduler interface
    // This is a placeholder for the integration logic
    LOG_INFO("Integrating ECS Job Scheduler with existing ECS scheduler");
}

void ECSJobScheduler::rebuild_execution_phases() {
    execution_phases_.clear();
    
    if (systems_.empty()) {
        return;
    }
    
    if (!system_dependencies_) {
        // No dependency analysis - all systems run in one phase
        std::vector<usize> single_phase;
        for (usize i = 0; i < systems_.size(); ++i) {
            single_phase.push_back(i);
        }
        execution_phases_.push_back(std::move(single_phase));
        return;
    }
    
    // Use topological sort to determine execution order
    auto sorted_systems = system_dependencies_->topological_sort();
    
    // Group systems into phases based on dependencies
    std::unordered_set<std::string> processed;
    
    while (processed.size() < systems_.size()) {
        std::vector<usize> current_phase;
        
        for (const std::string& system_name : sorted_systems) {
            if (processed.find(system_name) != processed.end()) {
                continue;
            }
            
            auto it = system_name_to_index_.find(system_name);
            if (it == system_name_to_index_.end()) {
                continue;
            }
            
            usize system_index = it->second;
            
            // Check if all dependencies are satisfied
            auto dependencies = system_dependencies_->get_dependencies(system_name);
            bool can_run = true;
            
            for (const std::string& dep : dependencies) {
                if (processed.find(dep) == processed.end()) {
                    can_run = false;
                    break;
                }
            }
            
            if (can_run) {
                current_phase.push_back(system_index);
                processed.insert(system_name);
            }
        }
        
        if (!current_phase.empty()) {
            execution_phases_.push_back(std::move(current_phase));
        } else {
            // Avoid infinite loop
            break;
        }
    }
}

std::vector<usize> ECSJobScheduler::find_independent_systems() const {
    std::vector<usize> independent;
    
    for (usize i = 0; i < systems_.size(); ++i) {
        bool is_independent = true;
        
        for (usize j = 0; j < systems_.size(); ++j) {
            if (i != j && systems_[i]->has_dependency_conflict(*systems_[j])) {
                is_independent = false;
                break;
            }
        }
        
        if (is_independent) {
            independent.push_back(i);
        }
    }
    
    return independent;
}

void ECSJobScheduler::analyze_component_dependencies() {
    // Analyze which systems read/write which components
    for (auto& system : systems_) {
        // This would require introspection into system component requirements
        // Implementation depends on how systems declare their component dependencies
    }
}

bool ECSJobScheduler::systems_can_run_in_parallel(usize system1, usize system2) const {
    if (system1 >= systems_.size() || system2 >= systems_.size()) {
        return false;
    }
    
    return !systems_[system1]->has_dependency_conflict(*systems_[system2]);
}

void ECSJobScheduler::optimize_entity_batching() {
    for (auto& system : systems_) {
        system->optimize_batches_for_locality(*registry_);
        
        // Adjust batch size based on performance
        u32 optimal_batch_size = calculate_optimal_batch_size(*system);
        system->set_batch_size(optimal_batch_size);
    }
}

void ECSJobScheduler::balance_batch_sizes() {
    if (!config_.enable_load_balancing) {
        return;
    }
    
    // Analyze system load and adjust batch sizes
    f64 total_execution_time = 0.0;
    for (const auto& system : systems_) {
        total_execution_time += system->average_execution_time();
    }
    
    if (total_execution_time == 0.0) {
        return;
    }
    
    for (auto& system : systems_) {
        f64 system_ratio = system->average_execution_time() / total_execution_time;
        
        if (system_ratio > config_.load_balance_threshold) {
            // System is taking too much time - reduce batch size
            u32 new_batch_size = static_cast<u32>(system->config().batch_size * 0.8);
            system->set_batch_size(std::max(new_batch_size, 100u));
        }
    }
}

void ECSJobScheduler::update_performance_metrics() {
    if (!profiler_) {
        return;
    }
    
    // Update job system metrics
    if (job_system_) {
        auto job_stats = job_system_->get_statistics();
        
        profiler_->record_metric("jobs_submitted_per_frame", 
                                static_cast<f64>(job_stats.jobs_submitted));
        profiler_->record_metric("jobs_completed_per_frame", 
                                static_cast<f64>(job_stats.jobs_completed));
        profiler_->record_metric("worker_utilization", 
                                job_stats.average_worker_utilization);
    }
    
    // Update system metrics
    for (const auto& system : systems_) {
        std::string metric_name = "system_" + system->name() + "_execution_time";
        profiler_->record_metric(metric_name, system->average_execution_time());
    }
}

SystemExecutionStrategy ECSJobScheduler::choose_execution_strategy(const SystemJob& system) const {
    // This is handled by the SystemJob itself
    return system.config().strategy;
}

u32 ECSJobScheduler::calculate_optimal_batch_size(const SystemJob& system) const {
    f64 execution_time = system.average_execution_time();
    u32 current_batch_size = system.config().batch_size;
    
    // Simple heuristic: aim for 1-2ms per batch
    const f64 target_time_per_batch = 1500.0; // microseconds
    
    if (execution_time > 0.0) {
        f64 ratio = target_time_per_batch / execution_time;
        u32 new_batch_size = static_cast<u32>(current_batch_size * ratio);
        
        // Clamp to reasonable bounds
        return std::clamp(new_batch_size, 100u, 5000u);
    }
    
    return current_batch_size;
}

} // namespace ecscope::jobs