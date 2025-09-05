#pragma once

/**
 * @file ecs/dependency_resolver.hpp
 * @brief Advanced System Dependency Resolution and Automatic Scheduling
 * 
 * This comprehensive dependency resolution system provides automatic system
 * scheduling based on declared dependencies, resource access patterns, and
 * performance constraints. It integrates with the existing system framework
 * to provide educational insights into dependency management and parallel
 * execution optimization.
 * 
 * Key Features:
 * - Automatic topological sorting of system dependencies
 * - Resource conflict detection and resolution
 * - Parallel execution group generation
 * - Circular dependency detection with clear error reporting
 * - Performance-aware scheduling optimization
 * - Educational visualization of dependency graphs
 * - Integration with existing memory allocators and performance tracking
 * 
 * Educational Value:
 * - Demonstrates graph algorithms in practical applications
 * - Shows dependency injection and inversion of control patterns
 * - Illustrates parallel programming concepts and constraints
 * - Provides examples of resource management and conflict resolution
 * 
 * Algorithm Features:
 * - Kahn's algorithm for topological sorting
 * - Graph coloring for parallel group assignment
 * - Critical path analysis for performance optimization
 * - Resource usage analysis for conflict detection
 * 
 * @author ECScope Educational ECS Framework - Modern Extensions
 * @date 2024
 */

#include "core/types.hpp"
#include "core/log.hpp"
#include "ecs/system.hpp"
#include "ecs/modern_concepts.hpp"
#include "memory/allocators/arena.hpp"
#include "memory/memory_tracker.hpp"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <queue>
#include <stack>
#include <memory>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <typeindex>
#include <functional>

namespace ecscope::ecs::dependency {

using namespace ecscope::ecs::concepts;

//=============================================================================
// Dependency Graph Data Structures
//=============================================================================

/**
 * @brief Node in the system dependency graph
 */
struct DependencyNode {
    System* system;                             // Pointer to the actual system
    std::string name;                          // System name for debugging
    SystemPhase phase;                         // Execution phase
    SystemExecutionType execution_type;       // Sequential/Parallel/Exclusive
    
    // Dependency relationships
    std::vector<std::string> hard_dependencies; // Must complete before this system
    std::vector<std::string> soft_dependencies; // Prefer to run after these systems
    std::vector<std::string> dependents;        // Systems that depend on this one
    
    // Resource access patterns
    std::unordered_set<std::type_index> reads_components;
    std::unordered_set<std::type_index> writes_components;
    std::unordered_set<std::string> reads_resources;
    std::unordered_set<std::string> writes_resources;
    std::unordered_set<std::string> exclusive_resources;
    
    // Scheduling information
    u32 in_degree;                            // Number of dependencies not yet satisfied
    u32 parallel_group_id;                    // Group for parallel execution (0 = sequential)
    u32 execution_order;                      // Order within execution group
    f64 estimated_execution_time;             // Estimated execution time
    f64 critical_path_weight;                 // Weight on critical path
    
    // Performance tracking
    bool is_on_critical_path;                 // Whether system is on critical path
    f64 slack_time;                          // Available slack time for optimization
    u32 dependency_depth;                     // Depth in dependency tree
    
    DependencyNode() 
        : system(nullptr), phase(SystemPhase::Update), execution_type(SystemExecutionType::Sequential)
        , in_degree(0), parallel_group_id(0), execution_order(0), estimated_execution_time(0.0)
        , critical_path_weight(0.0), is_on_critical_path(false), slack_time(0.0), dependency_depth(0) {}
        
    explicit DependencyNode(System* sys)
        : system(sys), name(sys->name()), phase(sys->phase()), execution_type(sys->execution_type())
        , in_degree(0), parallel_group_id(0), execution_order(0)
        , estimated_execution_time(sys->statistics().average_execution_time)
        , critical_path_weight(0.0), is_on_critical_path(false), slack_time(0.0), dependency_depth(0) {
        
        // Extract resource information from system
        const auto& resource_info = sys->resource_info();
        reads_components = std::unordered_set<std::type_index>(
            resource_info.read_components.begin(), resource_info.read_components.end());
        writes_components = std::unordered_set<std::type_index>(
            resource_info.write_components.begin(), resource_info.write_components.end());
        reads_resources = std::unordered_set<std::string>(
            resource_info.read_resources.begin(), resource_info.read_resources.end());
        writes_resources = std::unordered_set<std::string>(
            resource_info.write_resources.begin(), resource_info.write_resources.end());
        exclusive_resources = std::unordered_set<std::string>(
            resource_info.exclusive_resources.begin(), resource_info.exclusive_resources.end());
        
        // Extract dependency information
        for (const auto& dep : sys->dependencies()) {
            if (dep.is_hard_dependency) {
                hard_dependencies.push_back(dep.system_name);
            } else {
                soft_dependencies.push_back(dep.system_name);
            }
        }
    }
    
    /**
     * @brief Check if this system conflicts with another for parallel execution
     */
    bool conflicts_with(const DependencyNode& other) const {
        // Check for write-write conflicts on components
        for (const auto& write_comp : writes_components) {
            if (other.writes_components.contains(write_comp)) {
                return true;
            }
        }
        
        // Check for read-write conflicts on components
        for (const auto& write_comp : writes_components) {
            if (other.reads_components.contains(write_comp)) {
                return true;
            }
        }
        for (const auto& read_comp : reads_components) {
            if (other.writes_components.contains(read_comp)) {
                return true;
            }
        }
        
        // Check for resource conflicts
        for (const auto& exclusive : exclusive_resources) {
            if (other.exclusive_resources.contains(exclusive) ||
                other.reads_resources.contains(exclusive) ||
                other.writes_resources.contains(exclusive)) {
                return true;
            }
        }
        
        for (const auto& write_res : writes_resources) {
            if (other.writes_resources.contains(write_res) ||
                other.reads_resources.contains(write_res)) {
                return true;
            }
        }
        
        for (const auto& read_res : reads_resources) {
            if (other.writes_resources.contains(read_res)) {
                return true;
            }
        }
        
        return false;
    }
};

/**
 * @brief Dependency graph for systems within a specific phase
 */
class DependencyGraph {
private:
    std::unordered_map<std::string, std::unique_ptr<DependencyNode>> nodes_;
    std::unordered_map<std::string, std::vector<std::string>> adjacency_list_;
    SystemPhase phase_;
    memory::ArenaAllocator* arena_;
    
    // Performance tracking
    mutable u64 resolution_count_;
    mutable f64 total_resolution_time_;
    mutable u32 parallel_groups_generated_;
    
public:
    explicit DependencyGraph(SystemPhase phase, memory::ArenaAllocator* arena = nullptr)
        : phase_(phase), arena_(arena), resolution_count_(0)
        , total_resolution_time_(0.0), parallel_groups_generated_(0) {}
    
    /**
     * @brief Add system to dependency graph
     */
    void add_system(System* system) {
        if (system->phase() != phase_) {
            LOG_WARN("System {} phase mismatch. Expected {}, got {}", 
                    system->name(), static_cast<int>(phase_), static_cast<int>(system->phase()));
            return;
        }
        
        auto node = std::make_unique<DependencyNode>(system);
        std::string name = node->name;
        
        // Add to adjacency list
        adjacency_list_[name] = std::vector<std::string>();
        
        // Store node
        nodes_[name] = std::move(node);
    }
    
    /**
     * @brief Remove system from dependency graph
     */
    void remove_system(const std::string& system_name) {
        auto it = nodes_.find(system_name);
        if (it == nodes_.end()) {
            return;
        }
        
        // Remove from adjacency list
        adjacency_list_.erase(system_name);
        
        // Remove references from other nodes
        for (auto& [name, node] : nodes_) {
            auto& deps = node->hard_dependencies;
            deps.erase(std::remove(deps.begin(), deps.end(), system_name), deps.end());
            
            auto& soft_deps = node->soft_dependencies;
            soft_deps.erase(std::remove(soft_deps.begin(), soft_deps.end(), system_name), soft_deps.end());
            
            auto& dependents = node->dependents;
            dependents.erase(std::remove(dependents.begin(), dependents.end(), system_name), dependents.end());
        }
        
        // Remove adjacency list references
        for (auto& [name, adjacent_systems] : adjacency_list_) {
            adjacent_systems.erase(
                std::remove(adjacent_systems.begin(), adjacent_systems.end(), system_name),
                adjacent_systems.end());
        }
        
        nodes_.erase(it);
    }
    
    /**
     * @brief Build dependency edges after all systems are added
     */
    void build_dependency_edges() {
        // First pass: build hard dependency edges
        for (const auto& [name, node] : nodes_) {
            for (const std::string& dep_name : node->hard_dependencies) {
                if (nodes_.contains(dep_name)) {
                    adjacency_list_[dep_name].push_back(name);
                    nodes_[dep_name]->dependents.push_back(name);
                    node->in_degree++;
                } else {
                    LOG_WARN("System {} depends on non-existent system {}", name, dep_name);
                }
            }
        }
        
        // Second pass: add soft dependencies that don't create cycles
        for (const auto& [name, node] : nodes_) {
            for (const std::string& dep_name : node->soft_dependencies) {
                if (nodes_.contains(dep_name) && !would_create_cycle(dep_name, name)) {
                    adjacency_list_[dep_name].push_back(name);
                    nodes_[dep_name]->dependents.push_back(name);
                    node->in_degree++;
                }
            }
        }
        
        // Update dependency depth for each node
        calculate_dependency_depths();
    }
    
    /**
     * @brief Perform topological sort using Kahn's algorithm
     * 
     * Educational note: Kahn's algorithm is ideal for dependency resolution
     * because it can detect cycles and provides a natural ordering for execution.
     */
    std::vector<std::string> topological_sort() const {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        std::vector<std::string> result;
        std::queue<std::string> ready_queue;
        std::unordered_map<std::string, u32> in_degree_copy;
        
        // Initialize in-degree copy and find nodes with no dependencies
        for (const auto& [name, node] : nodes_) {
            in_degree_copy[name] = node->in_degree;
            if (node->in_degree == 0) {
                ready_queue.push(name);
            }
        }
        
        // Process nodes in topological order
        while (!ready_queue.empty()) {
            std::string current = ready_queue.front();
            ready_queue.pop();
            result.push_back(current);
            
            // Reduce in-degree of dependent nodes
            for (const std::string& dependent : adjacency_list_.at(current)) {
                in_degree_copy[dependent]--;
                if (in_degree_copy[dependent] == 0) {
                    ready_queue.push(dependent);
                }
            }
        }
        
        // Check for cycles
        if (result.size() != nodes_.size()) {
            throw std::runtime_error("Circular dependency detected in system graph for phase " + 
                                   std::to_string(static_cast<int>(phase_)));
        }
        
        // Track performance
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<f64>(end_time - start_time).count();
        ++resolution_count_;
        total_resolution_time_ += duration;
        
        return result;
    }
    
    /**
     * @brief Generate parallel execution groups
     * 
     * Educational note: This uses graph coloring principles to assign systems
     * to parallel execution groups such that no conflicts occur within a group.
     */
    std::vector<std::vector<std::string>> generate_parallel_groups() const {
        std::vector<std::vector<std::string>> groups;
        auto sorted_systems = topological_sort();
        
        // Assign systems to parallel groups
        for (const std::string& system_name : sorted_systems) {
            const auto& node = nodes_.at(system_name);
            
            // Skip systems that must execute sequentially
            if (node->execution_type == SystemExecutionType::Sequential ||
                node->execution_type == SystemExecutionType::Exclusive) {
                groups.push_back({system_name});
                continue;
            }
            
            // Find a compatible group or create a new one
            bool assigned = false;
            for (auto& group : groups) {
                bool can_join_group = true;
                
                // Check if this system conflicts with any system in the group
                for (const std::string& group_member : group) {
                    if (node->conflicts_with(*nodes_.at(group_member))) {
                        can_join_group = false;
                        break;
                    }
                }
                
                // Check dependency constraints
                if (can_join_group) {
                    for (const std::string& group_member : group) {
                        if (is_dependent_on(system_name, group_member) || 
                            is_dependent_on(group_member, system_name)) {
                            can_join_group = false;
                            break;
                        }
                    }
                }
                
                if (can_join_group) {
                    group.push_back(system_name);
                    assigned = true;
                    break;
                }
            }
            
            if (!assigned) {
                groups.push_back({system_name});
            }
        }
        
        // Update parallel group IDs in nodes
        for (u32 group_id = 0; group_id < groups.size(); ++group_id) {
            for (u32 order = 0; order < groups[group_id].size(); ++order) {
                const std::string& system_name = groups[group_id][order];
                auto& node = const_cast<DependencyNode&>(*nodes_.at(system_name));
                node.parallel_group_id = group_id;
                node.execution_order = order;
            }
        }
        
        ++parallel_groups_generated_;
        return groups;
    }
    
    /**
     * @brief Detect circular dependencies
     */
    std::vector<std::string> detect_circular_dependencies() const {
        std::unordered_set<std::string> visited;
        std::unordered_set<std::string> recursion_stack;
        std::vector<std::string> cycle_path;
        
        for (const auto& [name, node] : nodes_) {
            if (!visited.contains(name)) {
                if (has_cycle_dfs(name, visited, recursion_stack, cycle_path)) {
                    return cycle_path;
                }
            }
        }
        
        return {}; // No cycle found
    }
    
    /**
     * @brief Calculate critical path for performance optimization
     * 
     * Educational note: Critical path analysis helps identify systems that
     * most impact overall execution time, guiding optimization efforts.
     */
    f64 calculate_critical_path() {
        std::unordered_map<std::string, f64> longest_path;
        auto sorted_systems = topological_sort();
        
        // Initialize all path lengths to 0
        for (const auto& system_name : sorted_systems) {
            longest_path[system_name] = 0.0;
        }
        
        // Calculate longest path to each node
        for (const auto& system_name : sorted_systems) {
            const auto& node = nodes_.at(system_name);
            f64 current_weight = longest_path[system_name] + node->estimated_execution_time;
            
            // Update longest path for all dependents
            for (const std::string& dependent : node->dependents) {
                longest_path[dependent] = std::max(longest_path[dependent], current_weight);
            }
        }
        
        // Find the maximum path length
        f64 critical_path_length = 0.0;
        for (const auto& [system_name, path_length] : longest_path) {
            critical_path_length = std::max(critical_path_length, path_length);
        }
        
        // Mark systems on critical path and calculate slack
        calculate_slack_times(longest_path, critical_path_length);
        
        return critical_path_length;
    }
    
    /**
     * @brief Get system node information
     */
    const DependencyNode* get_node(const std::string& system_name) const {
        auto it = nodes_.find(system_name);
        return it != nodes_.end() ? it->second.get() : nullptr;
    }
    
    /**
     * @brief Get all systems in this phase
     */
    std::vector<System*> get_all_systems() const {
        std::vector<System*> systems;
        systems.reserve(nodes_.size());
        
        for (const auto& [name, node] : nodes_) {
            systems.push_back(node->system);
        }
        
        return systems;
    }
    
    /**
     * @brief Get dependency graph statistics
     */
    struct GraphStats {
        usize total_systems;
        usize total_dependencies;
        usize max_dependency_depth;
        f64 average_dependency_depth;
        f64 critical_path_length;
        u32 parallel_groups;
        f64 parallelization_efficiency; // 0.0 = all sequential, 1.0 = perfect parallel
        
        // Performance metrics
        f64 average_resolution_time;
        u64 total_resolutions;
        
        // Educational insights
        std::string bottleneck_analysis;
        std::vector<std::string> optimization_suggestions;
    };
    
    GraphStats get_statistics() const {
        GraphStats stats{};
        
        stats.total_systems = nodes_.size();
        stats.total_dependencies = std::accumulate(
            nodes_.begin(), nodes_.end(), 0u,
            [](usize sum, const auto& pair) {
                return sum + pair.second->hard_dependencies.size() + pair.second->soft_dependencies.size();
            });
        
        // Calculate dependency depth statistics
        u32 max_depth = 0;
        f64 total_depth = 0.0;
        for (const auto& [name, node] : nodes_) {
            max_depth = std::max(max_depth, node->dependency_depth);
            total_depth += node->dependency_depth;
        }
        stats.max_dependency_depth = max_depth;
        stats.average_dependency_depth = nodes_.empty() ? 0.0 : total_depth / nodes_.size();
        
        // Calculate critical path
        stats.critical_path_length = const_cast<DependencyGraph*>(this)->calculate_critical_path();
        
        // Parallel efficiency
        auto groups = generate_parallel_groups();
        stats.parallel_groups = groups.size();
        
        f64 total_parallel_systems = 0.0;
        for (const auto& group : groups) {
            if (group.size() > 1) {
                total_parallel_systems += group.size();
            }
        }
        stats.parallelization_efficiency = nodes_.empty() ? 0.0 : 
            total_parallel_systems / nodes_.size();
        
        // Performance metrics
        stats.average_resolution_time = resolution_count_ > 0 ? 
            total_resolution_time_ / resolution_count_ : 0.0;
        stats.total_resolutions = resolution_count_;
        
        // Generate analysis
        stats.bottleneck_analysis = generate_bottleneck_analysis();
        stats.optimization_suggestions = generate_optimization_suggestions();
        
        return stats;
    }
    
private:
    bool would_create_cycle(const std::string& from, const std::string& to) const {
        std::unordered_set<std::string> visited;
        return has_path_dfs(to, from, visited);
    }
    
    bool has_path_dfs(const std::string& current, const std::string& target,
                      std::unordered_set<std::string>& visited) const {
        if (current == target) return true;
        if (visited.contains(current)) return false;
        
        visited.insert(current);
        
        auto it = adjacency_list_.find(current);
        if (it != adjacency_list_.end()) {
            for (const std::string& neighbor : it->second) {
                if (has_path_dfs(neighbor, target, visited)) {
                    return true;
                }
            }
        }
        
        return false;
    }
    
    bool has_cycle_dfs(const std::string& current,
                       std::unordered_set<std::string>& visited,
                       std::unordered_set<std::string>& recursion_stack,
                       std::vector<std::string>& cycle_path) const {
        visited.insert(current);
        recursion_stack.insert(current);
        cycle_path.push_back(current);
        
        auto it = adjacency_list_.find(current);
        if (it != adjacency_list_.end()) {
            for (const std::string& neighbor : it->second) {
                if (!visited.contains(neighbor)) {
                    if (has_cycle_dfs(neighbor, visited, recursion_stack, cycle_path)) {
                        return true;
                    }
                } else if (recursion_stack.contains(neighbor)) {
                    // Found cycle - trim cycle_path to show only the cycle
                    auto cycle_start = std::find(cycle_path.begin(), cycle_path.end(), neighbor);
                    cycle_path.erase(cycle_path.begin(), cycle_start);
                    return true;
                }
            }
        }
        
        recursion_stack.erase(current);
        cycle_path.pop_back();
        return false;
    }
    
    bool is_dependent_on(const std::string& system, const std::string& potential_dependency) const {
        std::unordered_set<std::string> visited;
        return has_path_dfs(potential_dependency, system, visited);
    }
    
    void calculate_dependency_depths() {
        // Reset all depths
        for (auto& [name, node] : nodes_) {
            node->dependency_depth = 0;
        }
        
        // Calculate depth using topological ordering
        auto sorted_systems = topological_sort();
        for (const std::string& system_name : sorted_systems) {
            auto& node = nodes_[system_name];
            
            // Find maximum depth among dependencies
            u32 max_dep_depth = 0;
            for (const std::string& dep_name : node->hard_dependencies) {
                auto dep_it = nodes_.find(dep_name);
                if (dep_it != nodes_.end()) {
                    max_dep_depth = std::max(max_dep_depth, dep_it->second->dependency_depth);
                }
            }
            
            node->dependency_depth = max_dep_depth + 1;
        }
    }
    
    void calculate_slack_times(const std::unordered_map<std::string, f64>& longest_path,
                              f64 critical_path_length) {
        for (auto& [name, node] : nodes_) {
            f64 earliest_finish = longest_path.at(name) + node->estimated_execution_time;
            f64 latest_finish = critical_path_length;
            
            // Calculate latest start time by working backwards
            for (const std::string& dependent : node->dependents) {
                f64 dependent_latest_start = longest_path.at(dependent);
                latest_finish = std::min(latest_finish, dependent_latest_start);
            }
            
            node->slack_time = latest_finish - earliest_finish;
            node->is_on_critical_path = (node->slack_time < 0.001); // Account for floating point precision
            node->critical_path_weight = longest_path.at(name) + node->estimated_execution_time;
        }
    }
    
    std::string generate_bottleneck_analysis() const {
        std::ostringstream analysis;
        
        // Find systems on critical path
        std::vector<std::string> critical_systems;
        for (const auto& [name, node] : nodes_) {
            if (node->is_on_critical_path) {
                critical_systems.push_back(name);
            }
        }
        
        analysis << "Critical Path Systems (" << critical_systems.size() << "): ";
        for (usize i = 0; i < critical_systems.size(); ++i) {
            if (i > 0) analysis << " -> ";
            analysis << critical_systems[i];
        }
        
        return analysis.str();
    }
    
    std::vector<std::string> generate_optimization_suggestions() const {
        std::vector<std::string> suggestions;
        
        // Check for long critical path
        auto stats = get_statistics();
        if (stats.critical_path_length > 0.016) { // > 16ms (60 FPS budget)
            suggestions.push_back("Critical path exceeds frame budget - consider optimizing critical systems");
        }
        
        // Check for low parallelization
        if (stats.parallelization_efficiency < 0.3) {
            suggestions.push_back("Low parallelization efficiency - consider reducing resource conflicts");
        }
        
        // Check for deep dependency chains
        if (stats.max_dependency_depth > 10) {
            suggestions.push_back("Deep dependency chain detected - consider breaking into smaller phases");
        }
        
        return suggestions;
    }
};

//=============================================================================
// Main Dependency Resolver
//=============================================================================

/**
 * @brief Comprehensive system dependency resolver and scheduler
 */
class DependencyResolver {
private:
    // Dependency graphs per phase
    std::array<std::unique_ptr<DependencyGraph>, 
               static_cast<usize>(SystemPhase::COUNT)> phase_graphs_;
    
    memory::ArenaAllocator* arena_;
    bool enable_educational_logging_;
    
    // Performance tracking
    mutable u64 total_resolutions_;
    mutable f64 total_resolution_time_;
    
public:
    explicit DependencyResolver(memory::ArenaAllocator* arena = nullptr, 
                               bool enable_logging = true)
        : arena_(arena), enable_educational_logging_(enable_logging)
        , total_resolutions_(0), total_resolution_time_(0.0) {
        
        // Initialize dependency graphs for each phase
        for (usize i = 0; i < static_cast<usize>(SystemPhase::COUNT); ++i) {
            phase_graphs_[i] = std::make_unique<DependencyGraph>(
                static_cast<SystemPhase>(i), arena_);
        }
    }
    
    /**
     * @brief Add system to dependency resolver
     */
    void add_system(System* system) {
        if (!system) {
            LOG_ERROR("Attempted to add null system to dependency resolver");
            return;
        }
        
        usize phase_index = static_cast<usize>(system->phase());
        if (phase_index >= static_cast<usize>(SystemPhase::COUNT)) {
            LOG_ERROR("Invalid system phase for system {}", system->name());
            return;
        }
        
        phase_graphs_[phase_index]->add_system(system);
        
        if (enable_educational_logging_) {
            LOG_INFO("Added system '{}' to phase {} dependency graph", 
                    system->name(), static_cast<int>(system->phase()));
        }
    }
    
    /**
     * @brief Remove system from dependency resolver
     */
    void remove_system(const std::string& system_name, SystemPhase phase) {
        usize phase_index = static_cast<usize>(phase);
        if (phase_index < static_cast<usize>(SystemPhase::COUNT)) {
            phase_graphs_[phase_index]->remove_system(system_name);
        }
    }
    
    /**
     * @brief Resolve dependencies and generate execution order for a phase
     */
    std::vector<System*> resolve_execution_order(SystemPhase phase) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        usize phase_index = static_cast<usize>(phase);
        if (phase_index >= static_cast<usize>(SystemPhase::COUNT)) {
            return {};
        }
        
        auto& graph = phase_graphs_[phase_index];
        
        // Build dependency edges
        graph->build_dependency_edges();
        
        // Check for circular dependencies
        auto cycle = graph->detect_circular_dependencies();
        if (!cycle.empty()) {
            std::ostringstream error_msg;
            error_msg << "Circular dependency detected in phase " 
                     << static_cast<int>(phase) << ": ";
            for (usize i = 0; i < cycle.size(); ++i) {
                if (i > 0) error_msg << " -> ";
                error_msg << cycle[i];
            }
            error_msg << " -> " << cycle[0];
            
            throw std::runtime_error(error_msg.str());
        }
        
        // Get topological ordering
        auto sorted_names = graph->topological_sort();
        
        // Convert to system pointers
        std::vector<System*> sorted_systems;
        sorted_systems.reserve(sorted_names.size());
        
        for (const std::string& name : sorted_names) {
            const auto* node = graph->get_node(name);
            if (node && node->system) {
                sorted_systems.push_back(node->system);
            }
        }
        
        // Track performance
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<f64>(end_time - start_time).count();
        ++total_resolutions_;
        total_resolution_time_ += duration;
        
        if (enable_educational_logging_) {
            LOG_INFO("Resolved execution order for phase {} ({} systems) in {:.3f}ms",
                    static_cast<int>(phase), sorted_systems.size(), duration * 1000.0);
        }
        
        return sorted_systems;
    }
    
    /**
     * @brief Generate parallel execution groups for a phase
     */
    std::vector<std::vector<System*>> resolve_parallel_groups(SystemPhase phase) {
        usize phase_index = static_cast<usize>(phase);
        if (phase_index >= static_cast<usize>(SystemPhase::COUNT)) {
            return {};
        }
        
        auto& graph = phase_graphs_[phase_index];
        graph->build_dependency_edges();
        
        auto group_names = graph->generate_parallel_groups();
        
        // Convert to system pointers
        std::vector<std::vector<System*>> parallel_groups;
        parallel_groups.reserve(group_names.size());
        
        for (const auto& group : group_names) {
            std::vector<System*> systems;
            systems.reserve(group.size());
            
            for (const std::string& name : group) {
                const auto* node = graph->get_node(name);
                if (node && node->system) {
                    systems.push_back(node->system);
                }
            }
            
            if (!systems.empty()) {
                parallel_groups.push_back(std::move(systems));
            }
        }
        
        if (enable_educational_logging_) {
            LOG_INFO("Generated {} parallel groups for phase {}", 
                    parallel_groups.size(), static_cast<int>(phase));
        }
        
        return parallel_groups;
    }
    
    /**
     * @brief Validate all system dependencies
     */
    std::vector<std::string> validate_all_dependencies() const {
        std::vector<std::string> errors;
        
        for (usize i = 0; i < static_cast<usize>(SystemPhase::COUNT); ++i) {
            SystemPhase phase = static_cast<SystemPhase>(i);
            auto& graph = phase_graphs_[i];
            
            try {
                // Build dependencies and check for cycles
                const_cast<DependencyGraph*>(graph.get())->build_dependency_edges();
                auto cycle = graph->detect_circular_dependencies();
                
                if (!cycle.empty()) {
                    std::ostringstream error_msg;
                    error_msg << "Phase " << static_cast<int>(phase) 
                             << " circular dependency: ";
                    for (usize j = 0; j < cycle.size(); ++j) {
                        if (j > 0) error_msg << " -> ";
                        error_msg << cycle[j];
                    }
                    errors.push_back(error_msg.str());
                }
                
            } catch (const std::exception& e) {
                errors.push_back("Phase " + std::to_string(static_cast<int>(phase)) + 
                               ": " + e.what());
            }
        }
        
        return errors;
    }
    
    /**
     * @brief Get comprehensive statistics for all phases
     */
    struct ResolverStats {
        std::array<DependencyGraph::GraphStats, 
                   static_cast<usize>(SystemPhase::COUNT)> phase_stats;
        
        usize total_systems;
        usize total_dependencies;
        f64 total_critical_path_time;
        f64 overall_parallelization_efficiency;
        
        f64 average_resolution_time;
        u64 total_resolutions;
        
        std::vector<std::string> global_optimization_suggestions;
    };
    
    ResolverStats get_comprehensive_statistics() const {
        ResolverStats stats{};
        
        stats.total_systems = 0;
        stats.total_dependencies = 0;
        stats.total_critical_path_time = 0.0;
        f64 total_parallel_efficiency = 0.0;
        
        for (usize i = 0; i < static_cast<usize>(SystemPhase::COUNT); ++i) {
            auto phase_stats = phase_graphs_[i]->get_statistics();
            stats.phase_stats[i] = phase_stats;
            
            stats.total_systems += phase_stats.total_systems;
            stats.total_dependencies += phase_stats.total_dependencies;
            stats.total_critical_path_time += phase_stats.critical_path_length;
            total_parallel_efficiency += phase_stats.parallelization_efficiency;
        }
        
        stats.overall_parallelization_efficiency = 
            total_parallel_efficiency / static_cast<usize>(SystemPhase::COUNT);
        
        stats.average_resolution_time = total_resolutions_ > 0 ?
            total_resolution_time_ / total_resolutions_ : 0.0;
        stats.total_resolutions = total_resolutions_;
        
        // Generate global optimization suggestions
        stats.global_optimization_suggestions = generate_global_optimization_suggestions(stats);
        
        return stats;
    }
    
    /**
     * @brief Export dependency graph visualization data
     */
    struct GraphVisualizationData {
        struct Node {
            std::string name;
            SystemPhase phase;
            bool is_critical;
            f64 execution_time;
            u32 parallel_group;
        };
        
        struct Edge {
            std::string from;
            std::string to;
            bool is_hard_dependency;
        };
        
        std::vector<Node> nodes;
        std::vector<Edge> edges;
    };
    
    GraphVisualizationData export_visualization_data() const {
        GraphVisualizationData data;
        
        for (usize i = 0; i < static_cast<usize>(SystemPhase::COUNT); ++i) {
            SystemPhase phase = static_cast<SystemPhase>(i);
            auto& graph = phase_graphs_[i];
            
            // Export nodes
            auto systems = graph->get_all_systems();
            for (System* system : systems) {
                const auto* node = graph->get_node(system->name());
                if (node) {
                    data.nodes.push_back({
                        .name = node->name,
                        .phase = phase,
                        .is_critical = node->is_on_critical_path,
                        .execution_time = node->estimated_execution_time,
                        .parallel_group = node->parallel_group_id
                    });
                    
                    // Export edges
                    for (const std::string& dep : node->hard_dependencies) {
                        data.edges.push_back({
                            .from = dep,
                            .to = node->name,
                            .is_hard_dependency = true
                        });
                    }
                    
                    for (const std::string& dep : node->soft_dependencies) {
                        data.edges.push_back({
                            .from = dep,
                            .to = node->name,
                            .is_hard_dependency = false
                        });
                    }
                }
            }
        }
        
        return data;
    }
    
private:
    std::vector<std::string> generate_global_optimization_suggestions(
        const ResolverStats& stats) const {
        
        std::vector<std::string> suggestions;
        
        if (stats.total_critical_path_time > 0.033) { // > 33ms (30 FPS)
            suggestions.push_back("Total critical path time exceeds frame budget - consider system optimization");
        }
        
        if (stats.overall_parallelization_efficiency < 0.4) {
            suggestions.push_back("Low overall parallelization - consider reducing cross-phase dependencies");
        }
        
        if (stats.total_dependencies > stats.total_systems * 2) {
            suggestions.push_back("High dependency ratio - consider decoupling systems through events");
        }
        
        return suggestions;
    }
};

} // namespace ecscope::ecs::dependency