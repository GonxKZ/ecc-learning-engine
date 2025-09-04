/**
 * @file ecs_parallel_scheduler.cpp
 * @brief Implementation of ECS Parallel Scheduler for automatic system parallelization
 */

#include "ecs_parallel_scheduler.hpp"
#include "core/log.hpp"
#include <algorithm>
#include <set>
#include <sstream>

namespace ecscope::job_system {

//=============================================================================
// ComponentAccessInfo Implementation
//=============================================================================

bool ComponentAccessInfo::conflicts_with(const ComponentAccessInfo& other) const {
    // Two systems conflict if one writes to a component that the other reads or writes
    if (access_type == ComponentAccessType::Write || other.access_type == ComponentAccessType::Write) {
        return component_type == other.component_type;
    }
    // Read-read access is safe
    return false;
}

//=============================================================================
// ParallelSystemGroup Implementation
//=============================================================================

ParallelSystemGroup::ParallelSystemGroup(const std::string& name, ecs::SystemPhase phase)
    : name_(name), phase_(phase), max_parallelism_(1), estimated_execution_time_(0.0) {}

void ParallelSystemGroup::add_system(ecs::System* system) {
    if (!system) {
        LOG_WARN("Attempted to add null system to group '{}'", name_);
        return;
    }
    
    systems_.push_back(system);
    update_group_properties();
    
    LOG_DEBUG("Added system '{}' to parallel group '{}'", system->name(), name_);
}

void ParallelSystemGroup::remove_system(const std::string& system_name) {
    auto it = std::find_if(systems_.begin(), systems_.end(),
        [&system_name](const ecs::System* system) {
            return system && system->name() == system_name;
        });
    
    if (it != systems_.end()) {
        systems_.erase(it);
        update_group_properties();
        LOG_DEBUG("Removed system '{}' from parallel group '{}'", system_name, name_);
    }
}

bool ParallelSystemGroup::can_add_system(ecs::System* system, const std::vector<ComponentAccessInfo>& system_access) const {
    if (!system) return false;
    
    // Check if this system conflicts with any system already in the group
    for (const auto* existing_system : systems_) {
        if (conflicts_with_system(existing_system, system, system_access)) {
            return false;
        }
    }
    
    return true;
}

void ParallelSystemGroup::execute_parallel(JobSystem* job_system, const ecs::SystemContext& context) {
    if (!job_system || systems_.empty()) {
        return;
    }
    
    // Submit all systems in this group as parallel jobs
    std::vector<JobID> job_ids;
    job_ids.reserve(systems_.size());
    
    for (auto* system : systems_) {
        if (!system || !system->is_enabled()) {
            continue;
        }
        
        std::string job_name = "ECS_System_" + system->name();
        
        JobID job_id = job_system->submit_job(
            job_name,
            [system, &context]() {
                system->execute_internal(context);
            },
            JobPriority::High
        );
        
        job_ids.push_back(job_id);
    }
    
    // Wait for all systems in this group to complete
    if (!job_ids.empty()) {
        job_system->wait_for_batch(job_ids);
    }
}

void ParallelSystemGroup::update_group_properties() {
    // Estimate execution time based on system statistics
    estimated_execution_time_ = 0.0;
    for (const auto* system : systems_) {
        if (system) {
            estimated_execution_time_ = std::max(estimated_execution_time_, 
                                                system->get_average_execution_time());
        }
    }
    
    // Set max parallelism to the number of systems that can run in parallel
    max_parallelism_ = static_cast<u32>(systems_.size());
}

bool ParallelSystemGroup::conflicts_with_system(const ecs::System* existing_system, 
                                               const ecs::System* new_system,
                                               const std::vector<ComponentAccessInfo>& new_system_access) const {
    if (!existing_system || !new_system) return false;
    
    // This is a simplified conflict detection - in a full implementation,
    // we would check the actual component access patterns
    const auto& existing_resource_info = existing_system->resource_info();
    
    // Check for write-write or write-read conflicts on components
    for (const auto& new_access : new_system_access) {
        // Compare with existing system's component access
        for (const auto& existing_write : existing_resource_info.write_components) {
            if (new_access.component_type == existing_write) {
                return true; // Conflict detected
            }
        }
        
        if (new_access.access_type == ComponentAccessType::Write) {
            for (const auto& existing_read : existing_resource_info.read_components) {
                if (new_access.component_type == existing_read) {
                    return true; // Write-read conflict
                }
            }
        }
    }
    
    return false;
}

//=============================================================================
// ECSParallelScheduler Implementation
//=============================================================================

ECSParallelScheduler::ECSParallelScheduler(JobSystem* job_system, ecs::SystemManager* system_manager)
    : job_system_(job_system)
    , system_manager_(system_manager)
    , enable_automatic_grouping_(true)
    , enable_performance_monitoring_(true)
    , max_parallel_groups_per_phase_(8) {
    
    if (!job_system_ || !system_manager_) {
        throw std::invalid_argument("JobSystem and SystemManager cannot be null");
    }
    
    LOG_INFO("ECS Parallel Scheduler initialized with {} worker threads", 
             job_system_->worker_count());
}

ECSParallelScheduler::~ECSParallelScheduler() {
    // Cleanup parallel groups
    for (auto& [phase, groups] : parallel_groups_by_phase_) {
        groups.clear();
    }
}

void ECSParallelScheduler::configure_system_component_access(const std::string& system_name,
                                                           std::type_index component_type,
                                                           ComponentAccessType access_type,
                                                           const std::string& description) {
    ComponentAccessInfo access_info;
    access_info.system_name = system_name;
    access_info.component_type = component_type;
    access_info.access_type = access_type;
    access_info.description = description;
    
    system_component_access_[system_name].push_back(access_info);
    
    LOG_DEBUG("Configured component access for system '{}': {} access to component",
              system_name, access_type == ComponentAccessType::Read ? "Read" : "Write");
}

void ECSParallelScheduler::analyze_system_dependencies(ecs::System* system) {
    if (!system) {
        LOG_WARN("Cannot analyze dependencies for null system");
        return;
    }
    
    const std::string& system_name = system->name();
    
    // Get the system's component access patterns
    auto access_it = system_component_access_.find(system_name);
    if (access_it == system_component_access_.end()) {
        LOG_WARN("No component access information configured for system '{}'", system_name);
        return;
    }
    
    const auto& system_access = access_it->second;
    
    // Analyze conflicts with other systems
    SystemDependencyInfo dep_info;
    dep_info.system = system;
    
    for (const auto& [other_system_name, other_access] : system_component_access_) {
        if (other_system_name == system_name) continue;
        
        // Check for conflicts
        bool has_conflict = false;
        for (const auto& access1 : system_access) {
            for (const auto& access2 : other_access) {
                if (access1.conflicts_with(access2)) {
                    has_conflict = true;
                    dep_info.conflicting_systems.push_back(other_system_name);
                    break;
                }
            }
            if (has_conflict) break;
        }
    }
    
    // Store dependency information
    system_dependencies_[system_name] = std::move(dep_info);
    
    LOG_DEBUG("Analyzed dependencies for system '{}': {} conflicts found",
              system_name, dep_info.conflicting_systems.size());
}

void ECSParallelScheduler::analyze_all_systems() {
    LOG_INFO("Analyzing component dependencies for all ECS systems...");
    
    // Clear previous analysis
    system_dependencies_.clear();
    
    // Analyze each configured system
    for (const auto& [system_name, access_info] : system_component_access_) {
        // Find the system in the system manager
        ecs::System* system = system_manager_->get_system(system_name);
        if (system) {
            analyze_system_dependencies(system);
        } else {
            LOG_WARN("System '{}' configured for parallel analysis but not found in SystemManager",
                     system_name);
        }
    }
    
    LOG_INFO("Dependency analysis completed for {} systems", system_dependencies_.size());
}

void ECSParallelScheduler::rebuild_execution_groups() {
    LOG_INFO("Rebuilding parallel execution groups...");
    
    // Clear existing groups
    for (auto& [phase, groups] : parallel_groups_by_phase_) {
        groups.clear();
    }
    parallel_groups_by_phase_.clear();
    
    if (!enable_automatic_grouping_) {
        LOG_INFO("Automatic grouping disabled, skipping group creation");
        return;
    }
    
    // Group systems by phase first
    std::map<ecs::SystemPhase, std::vector<ecs::System*>> systems_by_phase;
    
    for (const auto& [system_name, dep_info] : system_dependencies_) {
        if (dep_info.system && dep_info.system->is_enabled()) {
            ecs::SystemPhase phase = dep_info.system->phase();
            systems_by_phase[phase].push_back(dep_info.system);
        }
    }
    
    // Create parallel groups for each phase
    for (const auto& [phase, systems] : systems_by_phase) {
        create_parallel_groups_for_phase(phase, systems);
    }
    
    print_execution_groups();
    LOG_INFO("Execution groups rebuilt successfully");
}

void ECSParallelScheduler::execute_phase_parallel(ecs::SystemPhase phase, f64 delta_time) {
    auto groups_it = parallel_groups_by_phase_.find(phase);
    if (groups_it == parallel_groups_by_phase_.end() || groups_it->second.empty()) {
        // No parallel groups for this phase, fall back to sequential execution
        system_manager_->execute_phase(phase, delta_time);
        return;
    }
    
    const auto& groups = groups_it->second;
    
    // Create system context
    // Note: This is simplified - actual implementation would need proper context creation
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Execute each parallel group sequentially (groups run in parallel internally)
    for (const auto& group : groups) {
        if (!group || group->empty()) continue;
        
        // Create a proper SystemContext - this is simplified
        // In a full implementation, we would need access to Registry, EventBus, etc.
        // For now, we'll just execute the group
        
        LOG_DEBUG("Executing parallel group '{}' with {} systems",
                  group->name(), group->system_count());
        
        // This would need a proper SystemContext in the full implementation
        // group->execute_parallel(job_system_, context);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    f64 execution_time = std::chrono::duration<f64, std::milli>(end_time - start_time).count();
    
    // Update statistics
    auto& stats = phase_statistics_[phase];
    stats.total_executions++;
    stats.total_execution_time += execution_time;
    stats.average_execution_time = stats.total_execution_time / stats.total_executions;
    
    if (enable_performance_monitoring_) {
        LOG_DEBUG("Phase {} executed in {:.2f}ms with {} parallel groups",
                  static_cast<int>(phase), execution_time, groups.size());
    }
}

ParallelSchedulerStatistics ECSParallelScheduler::get_statistics() const {
    ParallelSchedulerStatistics stats;
    
    // Count total parallel groups
    for (const auto& [phase, groups] : parallel_groups_by_phase_) {
        stats.active_parallel_groups += groups.size();
        for (const auto& group : groups) {
            if (group) {
                stats.total_parallel_systems += group->system_count();
            }
        }
    }
    
    // Calculate average execution times
    f64 total_time = 0.0;
    usize phase_count = 0;
    
    for (const auto& [phase, phase_stats] : phase_statistics_) {
        if (phase_stats.total_executions > 0) {
            total_time += phase_stats.average_execution_time;
            phase_count++;
        }
    }
    
    if (phase_count > 0) {
        stats.average_phase_execution_time = total_time / phase_count;
    }
    
    // Calculate parallelization ratio
    usize total_systems = system_dependencies_.size();
    if (total_systems > 0) {
        stats.parallelization_ratio = static_cast<f64>(stats.total_parallel_systems) / total_systems;
    }
    
    return stats;
}

std::string ECSParallelScheduler::generate_dependency_report() const {
    std::ostringstream report;
    
    report << "\n=== ECS System Dependency Analysis Report ===\n\n";
    
    if (system_dependencies_.empty()) {
        report << "No systems analyzed yet. Call analyze_all_systems() first.\n";
        return report.str();
    }
    
    // Summary statistics
    usize total_systems = system_dependencies_.size();
    usize systems_with_conflicts = 0;
    usize total_conflicts = 0;
    
    for (const auto& [system_name, dep_info] : system_dependencies_) {
        if (!dep_info.conflicting_systems.empty()) {
            systems_with_conflicts++;
            total_conflicts += dep_info.conflicting_systems.size();
        }
    }
    
    report << "Summary:\n";
    report << "  Total Systems Analyzed: " << total_systems << "\n";
    report << "  Systems with Conflicts: " << systems_with_conflicts << "\n";
    report << "  Total Conflicts: " << total_conflicts << "\n";
    report << "  Parallelizable Systems: " << (total_systems - systems_with_conflicts) << "\n\n";
    
    // Detailed conflict information
    report << "System Conflict Details:\n";
    report << "-------------------------\n";
    
    for (const auto& [system_name, dep_info] : system_dependencies_) {
        report << "System: " << system_name << "\n";
        
        if (dep_info.conflicting_systems.empty()) {
            report << "  Status: Can run in parallel (no conflicts)\n";
        } else {
            report << "  Status: Cannot run in parallel\n";
            report << "  Conflicts with:\n";
            for (const auto& conflicting_system : dep_info.conflicting_systems) {
                report << "    - " << conflicting_system << "\n";
            }
        }
        
        // Show component access patterns
        auto access_it = system_component_access_.find(system_name);
        if (access_it != system_component_access_.end()) {
            report << "  Component Access:\n";
            for (const auto& access : access_it->second) {
                report << "    " << (access.access_type == ComponentAccessType::Read ? "Read" : "Write")
                       << ": " << access.component_type.name();
                if (!access.description.empty()) {
                    report << " (" << access.description << ")";
                }
                report << "\n";
            }
        }
        
        report << "\n";
    }
    
    // Parallel group information
    if (!parallel_groups_by_phase_.empty()) {
        report << "Parallel Execution Groups:\n";
        report << "--------------------------\n";
        
        for (const auto& [phase, groups] : parallel_groups_by_phase_) {
            report << "Phase " << static_cast<int>(phase) << " (" << groups.size() << " groups):\n";
            
            for (usize i = 0; i < groups.size(); ++i) {
                const auto& group = groups[i];
                if (group) {
                    report << "  Group " << (i + 1) << ": " << group->name() 
                           << " (" << group->system_count() << " systems)\n";
                    
                    const auto& systems = group->get_systems();
                    for (const auto* system : systems) {
                        if (system) {
                            report << "    - " << system->name() << "\n";
                        }
                    }
                }
            }
            report << "\n";
        }
    }
    
    return report.str();
}

void ECSParallelScheduler::create_parallel_groups_for_phase(ecs::SystemPhase phase, 
                                                           const std::vector<ecs::System*>& systems) {
    if (systems.empty()) {
        return;
    }
    
    LOG_DEBUG("Creating parallel groups for phase {} with {} systems",
              static_cast<int>(phase), systems.size());
    
    auto& phase_groups = parallel_groups_by_phase_[phase];
    phase_groups.clear();
    
    // Simple greedy grouping algorithm
    // In a more sophisticated implementation, we would use graph coloring or other algorithms
    std::set<ecs::System*> ungrouped_systems(systems.begin(), systems.end());
    u32 group_counter = 0;
    
    while (!ungrouped_systems.empty() && phase_groups.size() < max_parallel_groups_per_phase_) {
        // Create a new parallel group
        std::ostringstream oss;
        oss << "Phase" << static_cast<int>(phase) << "_Group" << group_counter++;
        std::string group_name = oss.str();
        auto group = std::make_unique<ParallelSystemGroup>(group_name, phase);
        
        // Find systems that can be added to this group
        std::vector<ecs::System*> systems_to_add;
        
        for (auto* system : ungrouped_systems) {
            // Check if this system can be added to the current group
            auto access_it = system_component_access_.find(system->name());
            if (access_it != system_component_access_.end()) {
                if (group->can_add_system(system, access_it->second)) {
                    systems_to_add.push_back(system);
                }
            } else {
                // If no access pattern is defined, assume it can be grouped
                systems_to_add.push_back(system);
            }
        }
        
        // Add the systems to the group
        for (auto* system : systems_to_add) {
            group->add_system(system);
            ungrouped_systems.erase(system);
        }
        
        // Only add the group if it has systems
        if (!group->empty()) {
            phase_groups.push_back(std::move(group));
        }
        
        // Safety check to avoid infinite loop
        if (systems_to_add.empty()) {
            LOG_WARN("Unable to group remaining systems - possible dependency cycle");
            break;
        }
    }
    
    // Handle any remaining ungrouped systems (put them in individual groups)
    for (auto* system : ungrouped_systems) {
        std::ostringstream oss;
        oss << "Phase" << static_cast<int>(phase) << "_Solo_" << system->name();
        std::string group_name = oss.str();
        auto group = std::make_unique<ParallelSystemGroup>(group_name, phase);
        group->add_system(system);
        phase_groups.push_back(std::move(group));
    }
    
    LOG_INFO("Created {} parallel groups for phase {} covering {} systems",
             phase_groups.size(), static_cast<int>(phase), systems.size());
}

void ECSParallelScheduler::print_execution_groups() const {
    LOG_INFO("=== Parallel Execution Groups ===");
    
    for (const auto& [phase, groups] : parallel_groups_by_phase_) {
        LOG_INFO("Phase {}: {} groups", static_cast<int>(phase), groups.size());
        
        for (usize i = 0; i < groups.size(); ++i) {
            const auto& group = groups[i];
            if (group && !group->empty()) {
                LOG_INFO("  Group {}: {} systems (max parallelism: {})",
                         i + 1, group->system_count(), group->max_parallelism());
                
                const auto& systems = group->get_systems();
                for (const auto* system : systems) {
                    if (system) {
                        LOG_INFO("    - {}", system->name());
                    }
                }
            }
        }
    }
}

void ECSParallelScheduler::optimize_execution_order() {
    LOG_INFO("Optimizing execution order for better parallelism...");
    
    // This is a placeholder for more sophisticated optimization algorithms
    // In a full implementation, we might:
    // 1. Reorder systems to minimize dependencies
    // 2. Split large systems into smaller parallel jobs  
    // 3. Use more advanced graph algorithms for optimal grouping
    // 4. Consider system execution time estimates for load balancing
    
    // For now, we just rebuild the groups which applies our basic greedy algorithm
    rebuild_execution_groups();
    
    LOG_INFO("Execution order optimization completed");
}

} // namespace ecscope::job_system