#pragma once

/**
 * @file ecs_parallel_scheduler.hpp
 * @brief ECS System Auto-Parallelization and Dependency Analysis
 * 
 * This advanced scheduler automatically analyzes ECS system dependencies
 * and creates parallel execution graphs for optimal multi-core utilization.
 * 
 * Key Features:
 * - Automatic component read/write dependency analysis
 * - Safe parallel execution of compatible ECS systems
 * - Resource conflict detection and resolution
 * - Educational visualization of system dependencies
 * - Performance monitoring and optimization hints
 * - Integration with work-stealing job system
 * 
 * Dependency Analysis:
 * - Read-only systems can run in parallel
 * - Write conflicts prevent parallel execution
 * - Resource dependencies create execution ordering
 * - Component archetype analysis for data race prevention
 * 
 * Educational Value:
 * - Clear visualization of system dependencies
 * - Performance comparisons (sequential vs parallel)
 * - Understanding of data race prevention
 * - Multi-threading concepts in game engine context
 * 
 * @author ECScope Educational ECS Framework - Parallel Scheduling
 * @date 2025
 */

#include "work_stealing_job_system.hpp"
#include "ecs/system.hpp"
#include "ecs/registry.hpp"
#include "core/types.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <typeindex>
#include <memory>
#include <chrono>
#include <mutex>
#include <shared_mutex>

namespace ecscope::job_system {

//=============================================================================
// ECS System Dependency Analysis
//=============================================================================

/**
 * @brief Component access pattern analysis
 */
enum class ComponentAccessType : u8 {
    None = 0,
    Read = 1,
    Write = 2,
    ReadWrite = 3,
    Exclusive = 4  // Requires exclusive access (no other system can access)
};

/**
 * @brief Resource access pattern for ECS systems
 */
struct ResourceAccessPattern {
    std::type_index component_type;
    ComponentAccessType access_type;
    std::string access_description;
    
    // Access pattern characteristics
    bool is_frequent_access = false;      // High-frequency access
    bool is_cache_sensitive = false;      // Benefits from cache locality
    bool is_memory_intensive = false;     // Large memory footprint
    bool is_compute_intensive = false;    // CPU-bound processing
    
    ResourceAccessPattern(std::type_index type, ComponentAccessType access, 
                         const std::string& desc = "")
        : component_type(type), access_type(access), access_description(desc) {}
    
    bool conflicts_with(const ResourceAccessPattern& other) const noexcept {
        if (component_type != other.component_type) return false;
        
        // Any write access conflicts with any other access
        return (access_type == ComponentAccessType::Write || 
                access_type == ComponentAccessType::ReadWrite ||
                access_type == ComponentAccessType::Exclusive ||
                other.access_type == ComponentAccessType::Write ||
                other.access_type == ComponentAccessType::ReadWrite ||
                other.access_type == ComponentAccessType::Exclusive);
    }
    
    bool is_read_only() const noexcept {
        return access_type == ComponentAccessType::Read;
    }
    
    bool has_write_access() const noexcept {
        return access_type == ComponentAccessType::Write ||
               access_type == ComponentAccessType::ReadWrite ||
               access_type == ComponentAccessType::Exclusive;
    }
};

/**
 * @brief System execution requirements and constraints
 */
struct SystemExecutionProfile {
    std::string system_name;
    ecs::SystemPhase execution_phase;
    
    // Component access patterns
    std::vector<ResourceAccessPattern> component_accesses;
    
    // Global resource dependencies
    std::unordered_set<std::string> read_resources;
    std::unordered_set<std::string> write_resources;
    std::unordered_set<std::string> exclusive_resources;
    
    // Performance characteristics
    std::chrono::microseconds estimated_duration{1000};
    usize estimated_memory_usage = 0;
    f64 cpu_intensity_factor = 1.0;  // 1.0 = normal, >1.0 = CPU intensive
    bool is_main_thread_only = false;
    bool benefits_from_simd = false;
    
    // Dependency relationships
    std::vector<std::string> hard_dependencies;  // Must complete before this system
    std::vector<std::string> soft_dependencies;  // Prefer to run after
    std::vector<std::string> exclusions;         // Cannot run simultaneously with
    
    SystemExecutionProfile(const std::string& name, ecs::SystemPhase phase)
        : system_name(name), execution_phase(phase) {}
    
    // Check if this system can run in parallel with another
    bool can_run_parallel_with(const SystemExecutionProfile& other) const;
    
    // Add component access pattern
    template<typename ComponentType>
    void add_component_access(ComponentAccessType access_type, const std::string& description = "") {
        component_accesses.emplace_back(std::type_index(typeid(ComponentType)), access_type, description);
    }
    
    // Resource management
    void add_read_resource(const std::string& resource) { read_resources.insert(resource); }
    void add_write_resource(const std::string& resource) { write_resources.insert(resource); }
    void add_exclusive_resource(const std::string& resource) { exclusive_resources.insert(resource); }
    
    // Dependency management
    void add_hard_dependency(const std::string& system_name) { hard_dependencies.push_back(system_name); }
    void add_soft_dependency(const std::string& system_name) { soft_dependencies.push_back(system_name); }
    void add_exclusion(const std::string& system_name) { exclusions.push_back(system_name); }
};

/**
 * @brief System dependency analyzer for automatic parallelization
 */
class ECSSystemAnalyzer {
private:
    std::unordered_map<std::string, SystemExecutionProfile> system_profiles_;
    std::unordered_map<std::type_index, std::vector<std::string>> component_readers_;
    std::unordered_map<std::type_index, std::vector<std::string>> component_writers_;
    std::unordered_map<std::string, std::vector<std::string>> resource_dependencies_;
    
    // Analysis results cache
    mutable std::unordered_map<std::string, std::vector<std::string>> compatibility_cache_;
    mutable bool cache_valid_ = false;
    mutable std::shared_mutex cache_mutex_;
    
public:
    ECSSystemAnalyzer() = default;
    ~ECSSystemAnalyzer() = default;
    
    // System registration
    void register_system(const std::string& system_name, ecs::SystemPhase phase);
    void register_system_profile(const SystemExecutionProfile& profile);
    
    // Automatic analysis from ECS system
    void analyze_system(ecs::System* system);
    void analyze_all_systems(ecs::SystemManager* system_manager);
    
    // Manual specification (for complex cases)
    template<typename ComponentType>
    void specify_component_access(const std::string& system_name, 
                                 ComponentAccessType access_type,
                                 const std::string& description = "") {
        auto it = system_profiles_.find(system_name);
        if (it != system_profiles_.end()) {
            it->second.add_component_access<ComponentType>(access_type, description);
            invalidate_cache();
        }
    }
    
    void specify_resource_access(const std::string& system_name,
                               const std::string& resource_name,
                               ComponentAccessType access_type);
    
    void specify_system_dependency(const std::string& dependent_system,
                                 const std::string& dependency_system,
                                 bool is_hard_dependency = true);
    
    // Analysis queries
    std::vector<std::string> get_compatible_systems(const std::string& system_name) const;
    std::vector<std::string> get_conflicting_systems(const std::string& system_name) const;
    std::vector<std::vector<std::string>> build_parallel_execution_groups(ecs::SystemPhase phase) const;
    
    // Dependency graph analysis
    std::vector<std::string> get_execution_order(ecs::SystemPhase phase) const;
    std::vector<std::string> detect_dependency_cycles() const;
    bool validate_dependency_graph() const;
    
    // Performance analysis
    std::chrono::microseconds estimate_phase_duration(ecs::SystemPhase phase, bool parallel) const;
    f64 calculate_parallelization_benefit(ecs::SystemPhase phase) const;
    std::vector<std::string> identify_bottleneck_systems(ecs::SystemPhase phase) const;
    
    // Configuration and optimization
    void optimize_execution_order(ecs::SystemPhase phase);
    void suggest_system_modifications() const;
    void export_dependency_graph(const std::string& filename) const;
    
    // Information
    usize system_count() const { return system_profiles_.size(); }
    const SystemExecutionProfile* get_system_profile(const std::string& system_name) const;
    std::vector<std::string> get_registered_systems() const;
    
private:
    void invalidate_cache() const;
    void rebuild_dependency_maps();
    bool has_circular_dependency(const std::string& system, 
                               std::unordered_set<std::string>& visited,
                               std::unordered_set<std::string>& recursion_stack) const;
    void topological_sort_systems(const std::vector<std::string>& systems,
                                 std::vector<std::string>& result) const;
};

//=============================================================================
// Parallel ECS Scheduler
//=============================================================================

/**
 * @brief Execution group for parallel ECS systems
 */
struct ParallelExecutionGroup {
    std::string group_name;
    ecs::SystemPhase phase;
    std::vector<std::string> system_names;
    std::vector<ecs::System*> systems;
    
    // Execution characteristics
    std::chrono::microseconds estimated_duration{0};
    usize total_memory_requirement = 0;
    bool requires_main_thread = false;
    bool can_use_simd = false;
    
    // Performance tracking
    std::chrono::microseconds actual_duration{0};
    u32 execution_count = 0;
    f64 average_utilization = 0.0;
    
    ParallelExecutionGroup(const std::string& name, ecs::SystemPhase p)
        : group_name(name), phase(p) {}
    
    void add_system(ecs::System* system) {
        systems.push_back(system);
        system_names.push_back(system->name());
    }
    
    bool is_compatible_with_system(ecs::System* system, const ECSSystemAnalyzer& analyzer) const;
    void update_performance_metrics(std::chrono::microseconds duration);
};

/**
 * @brief Advanced ECS scheduler with automatic parallelization
 */
class ECSParallelScheduler {
private:
    JobSystem* job_system_;
    ECSSystemAnalyzer analyzer_;
    ecs::SystemManager* system_manager_;
    
    // Execution groups per phase
    std::array<std::vector<std::unique_ptr<ParallelExecutionGroup>>, 
               static_cast<usize>(ecs::SystemPhase::COUNT)> execution_groups_;
    
    // Scheduling state
    std::mutex scheduling_mutex_;
    std::atomic<bool> is_executing_{false};
    std::atomic<u64> frame_counter_{0};
    
    // Configuration
    bool enable_auto_parallelization_ = true;
    bool enable_load_balancing_ = true;
    bool enable_performance_monitoring_ = true;
    f64 parallel_efficiency_threshold_ = 0.7; // Min efficiency to enable parallelization
    u32 max_parallel_groups_per_phase_ = 8;
    
    // Performance tracking
    struct PhaseStats {
        std::chrono::microseconds sequential_time{0};
        std::chrono::microseconds parallel_time{0};
        f64 parallelization_efficiency = 0.0;
        u32 execution_count = 0;
        u32 parallel_execution_count = 0;
    };
    
    std::array<PhaseStats, static_cast<usize>(ecs::SystemPhase::COUNT)> phase_stats_;
    
public:
    /**
     * @brief Configuration for ECS parallel scheduler
     */
    struct Config {
        bool enable_auto_parallelization = true;
        bool enable_load_balancing = true;
        bool enable_performance_monitoring = true;
        f64 parallel_efficiency_threshold = 0.7;
        u32 max_parallel_groups_per_phase = 8;
        bool prefer_cache_locality = true;
        bool enable_numa_awareness = true;
        
        static Config create_performance_focused() {
            Config config;
            config.enable_performance_monitoring = false;
            config.parallel_efficiency_threshold = 0.5;
            config.max_parallel_groups_per_phase = 16;
            return config;
        }
        
        static Config create_educational() {
            Config config;
            config.enable_performance_monitoring = true;
            config.parallel_efficiency_threshold = 0.8;
            config.max_parallel_groups_per_phase = 4;
            return config;
        }
    };
    
    explicit ECSParallelScheduler(JobSystem* job_system, 
                                 ecs::SystemManager* system_manager,
                                 const Config& config = Config{});
    ~ECSParallelScheduler();
    
    // Initialization and configuration
    bool initialize();
    void shutdown();
    
    // System analysis and registration
    void analyze_all_systems();
    void rebuild_execution_groups();
    void register_system_dependencies();
    
    // Manual system configuration (for complex cases)
    template<typename ComponentType>
    void configure_system_component_access(const std::string& system_name,
                                         ComponentAccessType access_type,
                                         const std::string& description = "") {
        analyzer_.specify_component_access<ComponentType>(system_name, access_type, description);
        rebuild_execution_groups();
    }
    
    void configure_system_dependency(const std::string& dependent_system,
                                   const std::string& dependency_system,
                                   bool is_hard_dependency = true) {
        analyzer_.specify_system_dependency(dependent_system, dependency_system, is_hard_dependency);
        rebuild_execution_groups();
    }
    
    // Execution
    void execute_phase_parallel(ecs::SystemPhase phase, f64 delta_time);
    void execute_phase_sequential(ecs::SystemPhase phase, f64 delta_time);
    bool should_use_parallel_execution(ecs::SystemPhase phase) const;
    
    // Performance monitoring
    struct SchedulerStats {
        u64 total_frames_executed = 0;
        u64 parallel_frames_executed = 0;
        f64 overall_parallelization_efficiency = 0.0;
        
        std::array<PhaseStats, static_cast<usize>(ecs::SystemPhase::COUNT)> phase_statistics;
        
        f64 average_frame_time_ms = 0.0;
        f64 average_parallel_speedup = 1.0;
        u32 active_parallel_groups = 0;
        
        std::chrono::high_resolution_clock::time_point measurement_start;
        std::chrono::high_resolution_clock::time_point measurement_end;
    };
    
    SchedulerStats get_statistics() const;
    void reset_statistics();
    
    // Analysis and debugging
    std::string generate_dependency_report() const;
    std::string generate_performance_report() const;
    std::vector<std::string> get_parallelization_suggestions() const;
    void export_execution_timeline(const std::string& filename) const;
    
    // Educational features
    struct EducationalInsights {
        struct SystemInsight {
            std::string system_name;
            std::vector<std::string> component_accesses;
            std::vector<std::string> conflicts;
            std::vector<std::string> parallel_opportunities;
            f64 cpu_utilization = 0.0;
            f64 memory_efficiency = 0.0;
        };
        
        std::vector<SystemInsight> system_insights;
        f64 overall_parallelization_potential = 0.0;
        std::vector<std::string> optimization_recommendations;
        std::string dependency_graph_description;
    };
    
    EducationalInsights generate_educational_insights() const;
    void print_parallelization_tutorial() const;
    
    // Configuration
    void set_auto_parallelization(bool enable) { enable_auto_parallelization_ = enable; }
    void set_load_balancing(bool enable) { enable_load_balancing_ = enable; }
    void set_efficiency_threshold(f64 threshold) { parallel_efficiency_threshold_ = threshold; }
    void set_max_parallel_groups(u32 max_groups) { max_parallel_groups_per_phase_ = max_groups; }
    
    // Information
    bool is_executing() const { return is_executing_.load(std::memory_order_acquire); }
    u64 frame_counter() const { return frame_counter_.load(std::memory_order_acquire); }
    usize total_execution_groups() const;
    usize execution_groups_for_phase(ecs::SystemPhase phase) const;
    
private:
    void create_execution_groups_for_phase(ecs::SystemPhase phase);
    void optimize_group_assignment(ecs::SystemPhase phase);
    void balance_execution_load(std::vector<std::unique_ptr<ParallelExecutionGroup>>& groups);
    
    JobID schedule_execution_group(ParallelExecutionGroup& group, f64 delta_time);
    void execute_group_sequential(ParallelExecutionGroup& group, f64 delta_time);
    void execute_group_parallel(ParallelExecutionGroup& group, f64 delta_time);
    
    void update_performance_metrics(ecs::SystemPhase phase, 
                                  std::chrono::microseconds duration, 
                                  bool was_parallel);
    
    bool is_beneficial_to_parallelize(const ParallelExecutionGroup& group) const;
    f64 estimate_parallel_efficiency(const std::vector<ecs::System*>& systems) const;
    
    ecs::SystemContext create_system_context(ecs::SystemPhase phase, f64 delta_time) const;
};

//=============================================================================
// Template Implementations
//=============================================================================

template<typename ComponentType>
void ECSParallelScheduler::configure_system_component_access(const std::string& system_name,
                                                           ComponentAccessType access_type,
                                                           const std::string& description) {
    analyzer_.specify_component_access<ComponentType>(system_name, access_type, description);
    rebuild_execution_groups();
}

//=============================================================================
// ECS Integration Helpers
//=============================================================================

/**
 * @brief Helper class for easy ECS system parallelization
 */
class ECSParallelizationHelper {
public:
    // Common component access pattern specifications
    template<typename ComponentType>
    static void mark_read_only_access(ECSSystemAnalyzer& analyzer, 
                                    const std::string& system_name,
                                    const std::string& description = "Read-only access") {
        analyzer.specify_component_access<ComponentType>(system_name, 
                                                        ComponentAccessType::Read, 
                                                        description);
    }
    
    template<typename ComponentType>
    static void mark_write_access(ECSSystemAnalyzer& analyzer,
                                 const std::string& system_name,
                                 const std::string& description = "Write access") {
        analyzer.specify_component_access<ComponentType>(system_name,
                                                        ComponentAccessType::Write,
                                                        description);
    }
    
    template<typename ComponentType>
    static void mark_exclusive_access(ECSSystemAnalyzer& analyzer,
                                     const std::string& system_name,
                                     const std::string& description = "Exclusive access") {
        analyzer.specify_component_access<ComponentType>(system_name,
                                                        ComponentAccessType::Exclusive,
                                                        description);
    }
    
    // Common dependency patterns
    static void make_sequential(ECSSystemAnalyzer& analyzer,
                               const std::vector<std::string>& systems_in_order);
    
    static void make_parallel_group(ECSSystemAnalyzer& analyzer,
                                  const std::vector<std::string>& parallel_systems);
    
    static void make_mutually_exclusive(ECSSystemAnalyzer& analyzer,
                                       const std::vector<std::string>& exclusive_systems);
    
    // Physics system helpers
    static void configure_physics_systems(ECSSystemAnalyzer& analyzer);
    static void configure_rendering_systems(ECSSystemAnalyzer& analyzer);
    static void configure_animation_systems(ECSSystemAnalyzer& analyzer);
    static void configure_ai_systems(ECSSystemAnalyzer& analyzer);
};

} // namespace ecscope::job_system