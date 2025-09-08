#pragma once

/**
 * @file dependency_graph.hpp
 * @brief Advanced dependency graph resolution and topological sorting for system scheduling
 * 
 * This comprehensive dependency management system provides sophisticated graph algorithms
 * and analysis tools for optimal system scheduling with the following features:
 * 
 * - Lock-free dependency graph with efficient traversal
 * - Multiple topological sorting algorithms (Kahn's, DFS, hybrid)
 * - Cycle detection with detailed path analysis
 * - Dynamic dependency resolution during runtime
 * - Parallel dependency resolution for large graphs
 * - Dependency strength analysis and optimization
 * - Resource conflict detection and resolution
 * - Critical path analysis for scheduling optimization
 * - Dependency visualization and debugging tools
 * - Incremental graph updates with minimal recomputation
 * 
 * Graph Algorithms:
 * - Kahn's algorithm for standard topological sort
 * - DFS-based topological sort with better cache locality
 * - Parallel topological sort for large dependency graphs
 * - Strongly connected component detection
 * - Critical path analysis using longest path algorithm
 * - Minimum spanning tree for dependency optimization
 * 
 * Performance Optimizations:
 * - Cache-friendly adjacency list representation
 * - Lock-free graph traversal algorithms
 * - Incremental updates with change tracking
 * - Memory pool allocation for graph nodes
 * - SIMD-optimized graph operations where applicable
 * - Parallel processing for independent subgraphs
 * 
 * @author ECScope Professional ECS Framework
 * @date 2024
 */

#include "../core/types.hpp"
#include "../core/log.hpp"
#include "../foundation/memory_utils.hpp"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <stack>
#include <deque>
#include <string>
#include <memory>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <algorithm>
#include <chrono>
#include <optional>
#include <functional>
#include <span>
#include <ranges>

namespace ecscope::scheduling {

// Forward declarations
class System;
class SystemManager;

/**
 * @brief Dependency type enumeration for different kinds of system dependencies
 */
enum class DependencyType : u8 {
    HardBefore = 0,     // System must complete before dependent can start
    HardAfter = 1,      // System must start after dependency completes
    SoftBefore = 2,     // Prefer to run before (scheduling hint)
    SoftAfter = 3,      // Prefer to run after (scheduling hint)
    ResourceConflict = 4, // Systems conflict on shared resources
    DataFlow = 5,       // Data flows from one system to another
    OrderOnly = 6,      // Ordering constraint without data dependency
    Conditional = 7,    // Dependency only applies under certain conditions
    COUNT = 8
};

/**
 * @brief Resource access type for conflict detection
 */
enum class ResourceAccessType : u8 {
    Read = 0,
    Write = 1,
    ReadWrite = 2,
    Exclusive = 3
};

/**
 * @brief Dependency strength and characteristics
 */
struct DependencyInfo {
    DependencyType type;
    f32 strength;                   // Dependency strength (0.0 to 1.0)
    f32 cost;                      // Cost of violating dependency
    f32 latency;                   // Expected latency between systems
    u32 resource_id;               // Resource ID for resource dependencies
    ResourceAccessType access_type; // How the resource is accessed
    std::string condition;          // Condition string for conditional dependencies
    u64 creation_time;             // When dependency was created
    u64 last_violation_time;       // Last time dependency was violated
    u32 violation_count;           // Number of times dependency was violated
    
    DependencyInfo(DependencyType dep_type = DependencyType::HardBefore, 
                   f32 dep_strength = 1.0f, f32 dep_cost = 1.0f)
        : type(dep_type), strength(dep_strength), cost(dep_cost)
        , latency(0.0f), resource_id(0), access_type(ResourceAccessType::Read)
        , creation_time(0), last_violation_time(0), violation_count(0) {}
};

/**
 * @brief Graph node representing a system in the dependency graph
 */
class DependencyNode {
private:
    u32 node_id_;
    std::string system_name_;
    System* system_ptr_;
    
    // Adjacency lists for different dependency types
    std::vector<std::pair<u32, DependencyInfo>> outgoing_edges_;  // Systems this depends on
    std::vector<std::pair<u32, DependencyInfo>> incoming_edges_;  // Systems depending on this
    
    // Scheduling information
    mutable std::atomic<u32> in_degree_;
    mutable std::atomic<u32> out_degree_;
    mutable std::atomic<bool> visited_;
    mutable std::atomic<bool> in_recursion_stack_;
    
    // Performance metrics
    mutable std::atomic<f64> last_execution_time_;
    mutable std::atomic<f64> average_execution_time_;
    mutable std::atomic<u64> execution_count_;
    mutable std::atomic<u32> scheduling_priority_;
    
    // Resource information
    std::unordered_map<u32, ResourceAccessType> resource_requirements_;
    
    mutable std::shared_mutex node_mutex_;

public:
    explicit DependencyNode(u32 id, const std::string& name, System* system = nullptr);
    
    // Basic accessors
    u32 id() const noexcept { return node_id_; }
    const std::string& name() const noexcept { return system_name_; }
    System* system() const noexcept { return system_ptr_; }
    void set_system(System* system) { system_ptr_ = system; }
    
    // Dependency management
    void add_outgoing_edge(u32 target_id, const DependencyInfo& info);
    void add_incoming_edge(u32 source_id, const DependencyInfo& info);
    void remove_outgoing_edge(u32 target_id);
    void remove_incoming_edge(u32 source_id);
    bool has_outgoing_edge(u32 target_id) const;
    bool has_incoming_edge(u32 source_id) const;
    
    // Graph traversal support
    u32 in_degree() const { return in_degree_.load(std::memory_order_relaxed); }
    u32 out_degree() const { return out_degree_.load(std::memory_order_relaxed); }
    void increment_in_degree() const { in_degree_.fetch_add(1, std::memory_order_relaxed); }
    void decrement_in_degree() const { in_degree_.fetch_sub(1, std::memory_order_relaxed); }
    void reset_in_degree() const;
    
    bool is_visited() const { return visited_.load(std::memory_order_acquire); }
    void set_visited(bool visited) const { visited_.store(visited, std::memory_order_release); }
    
    bool is_in_recursion_stack() const { return in_recursion_stack_.load(std::memory_order_acquire); }
    void set_in_recursion_stack(bool in_stack) const { 
        in_recursion_stack_.store(in_stack, std::memory_order_release); 
    }
    
    // Edge access
    std::span<const std::pair<u32, DependencyInfo>> outgoing_edges() const;
    std::span<const std::pair<u32, DependencyInfo>> incoming_edges() const;
    
    // Resource management
    void add_resource_requirement(u32 resource_id, ResourceAccessType access);
    void remove_resource_requirement(u32 resource_id);
    bool conflicts_with(const DependencyNode& other) const;
    std::vector<u32> get_conflicting_resources(const DependencyNode& other) const;
    
    // Performance tracking
    void record_execution_time(f64 execution_time) const;
    f64 get_average_execution_time() const { 
        return average_execution_time_.load(std::memory_order_relaxed); 
    }
    u64 get_execution_count() const { 
        return execution_count_.load(std::memory_order_relaxed); 
    }
    
    u32 get_scheduling_priority() const { 
        return scheduling_priority_.load(std::memory_order_relaxed); 
    }
    void set_scheduling_priority(u32 priority) const { 
        scheduling_priority_.store(priority, std::memory_order_relaxed); 
    }
    
    // Utility
    void clear_traversal_state() const;
    std::string debug_string() const;

private:
    void update_degree_counts() const;
};

/**
 * @brief Topological sort result with execution order and analysis
 */
struct TopologicalSortResult {
    std::vector<std::vector<u32>> execution_levels;  // Systems that can execute in parallel
    std::vector<u32> sequential_order;               // Sequential execution order
    std::vector<std::pair<u32, u32>> critical_path;  // Critical path through the graph
    f64 estimated_total_time;                        // Estimated total execution time
    f64 critical_path_time;                          // Critical path execution time
    f64 parallelization_efficiency;                  // How well the graph can be parallelized
    u32 max_parallelism;                            // Maximum number of systems that can run in parallel
    bool has_cycles;                                // Whether the graph contains cycles
    std::vector<std::vector<u32>> detected_cycles;  // Any cycles found in the graph
    std::string error_message;                      // Error message if sort failed
    
    TopologicalSortResult() : estimated_total_time(0.0), critical_path_time(0.0)
                            , parallelization_efficiency(0.0), max_parallelism(0)
                            , has_cycles(false) {}
    
    bool is_valid() const { return !has_cycles && !execution_levels.empty(); }
    
    usize total_levels() const { return execution_levels.size(); }
    usize systems_at_level(usize level) const { 
        return level < execution_levels.size() ? execution_levels[level].size() : 0;
    }
    
    f64 get_speedup() const {
        return critical_path_time > 0.0 ? estimated_total_time / critical_path_time : 1.0;
    }
};

/**
 * @brief Advanced dependency graph with sophisticated analysis capabilities
 */
class DependencyGraph {
private:
    // Node storage
    std::unordered_map<u32, std::unique_ptr<DependencyNode>> nodes_;
    std::unordered_map<std::string, u32> name_to_id_;
    
    // Graph metadata
    std::atomic<u32> next_node_id_;
    std::atomic<u32> edge_count_;
    std::atomic<u64> last_modification_time_;
    std::atomic<u32> version_;
    
    // Thread safety
    mutable std::shared_mutex graph_mutex_;
    
    // Performance optimization
    mutable std::optional<TopologicalSortResult> cached_sort_result_;
    mutable bool cache_valid_;
    mutable std::mutex cache_mutex_;
    
    // Resource tracking
    std::unordered_map<u32, std::string> resource_names_;
    std::atomic<u32> next_resource_id_;
    
    // Statistics
    mutable struct {
        std::atomic<u64> sort_operations{0};
        std::atomic<u64> cycle_detections{0};
        std::atomic<u64> cache_hits{0};
        std::atomic<u64> cache_misses{0};
        std::atomic<f64> average_sort_time{0.0};
        std::atomic<f64> average_cycle_check_time{0.0};
    } stats_;

public:
    DependencyGraph();
    ~DependencyGraph();
    
    // Node management
    u32 add_node(const std::string& name, System* system = nullptr);
    void remove_node(u32 node_id);
    void remove_node(const std::string& name);
    DependencyNode* get_node(u32 node_id) const;
    DependencyNode* get_node(const std::string& name) const;
    u32 get_node_id(const std::string& name) const;
    bool has_node(u32 node_id) const;
    bool has_node(const std::string& name) const;
    
    // Dependency management
    bool add_dependency(u32 source_id, u32 target_id, const DependencyInfo& info = DependencyInfo{});
    bool add_dependency(const std::string& source, const std::string& target, 
                       const DependencyInfo& info = DependencyInfo{});
    bool remove_dependency(u32 source_id, u32 target_id);
    bool remove_dependency(const std::string& source, const std::string& target);
    bool has_dependency(u32 source_id, u32 target_id) const;
    bool has_dependency(const std::string& source, const std::string& target) const;
    
    // Resource management
    u32 register_resource(const std::string& name);
    void add_resource_dependency(u32 node_id, u32 resource_id, ResourceAccessType access);
    void add_resource_dependency(const std::string& system_name, const std::string& resource_name,
                                ResourceAccessType access);
    std::vector<std::pair<u32, u32>> detect_resource_conflicts() const;
    void resolve_resource_conflicts();
    
    // Graph analysis
    TopologicalSortResult compute_topological_sort() const;
    TopologicalSortResult compute_parallel_execution_order() const;
    std::vector<std::vector<u32>> detect_cycles() const;
    std::vector<u32> find_critical_path() const;
    std::vector<std::pair<u32, u32>> find_critical_dependencies() const;
    
    // Advanced analysis
    f64 compute_parallelization_potential() const;
    std::vector<u32> find_bottleneck_nodes() const;
    std::unordered_map<u32, f32> compute_node_centrality() const;
    std::vector<std::pair<u32, u32>> suggest_dependency_optimizations() const;
    
    // Validation
    bool is_acyclic() const;
    bool validate_consistency() const;
    std::vector<std::string> validate_dependencies() const;
    std::string generate_validation_report() const;
    
    // Information
    usize node_count() const;
    usize edge_count() const { return edge_count_.load(std::memory_order_relaxed); }
    u32 version() const { return version_.load(std::memory_order_relaxed); }
    bool is_empty() const { return node_count() == 0; }
    
    // Serialization and debugging
    std::string to_dot_format() const;
    std::string to_json() const;
    void export_graphviz(const std::string& filename) const;
    std::string debug_info() const;
    
    // Performance optimization
    void optimize_graph_structure();
    void compact_node_ids();
    void clear_cache() const;
    void enable_caching(bool enable);
    
    // Statistics
    struct Statistics {
        u64 total_sorts;
        u64 total_cycle_checks;
        u64 cache_hit_rate;
        f64 average_sort_time;
        f64 average_cycle_check_time;
        usize current_nodes;
        usize current_edges;
        u32 graph_version;
    };
    Statistics get_statistics() const;
    void reset_statistics();

private:
    // Internal algorithms
    TopologicalSortResult kahn_algorithm() const;
    TopologicalSortResult dfs_topological_sort() const;
    TopologicalSortResult parallel_topological_sort() const;
    
    bool dfs_cycle_detection(u32 node_id, std::vector<u32>& cycle_path) const;
    void dfs_visit(u32 node_id, std::vector<u32>& result, 
                   std::unordered_set<u32>& visited) const;
    
    std::vector<u32> compute_longest_path() const;
    f64 estimate_execution_time(const std::vector<u32>& path) const;
    
    void invalidate_cache();
    bool is_cache_valid() const;
    
    void update_modification_time();
    void increment_version();
    
    // Parallel algorithms
    std::vector<std::vector<u32>> compute_execution_levels() const;
    void compute_strongly_connected_components(
        std::vector<std::vector<u32>>& components) const;
    
    // Optimization helpers
    void remove_redundant_dependencies();
    void merge_equivalent_nodes();
    std::vector<std::pair<u32, u32>> find_transitive_reduction() const;
    
    static u64 get_current_time_ns();
};

/**
 * @brief Dependency resolver that manages dynamic dependency resolution during runtime
 */
class DependencyResolver {
private:
    DependencyGraph* graph_;
    std::atomic<bool> resolution_active_;
    
    // Runtime state
    std::unordered_map<u32, std::atomic<bool>> node_ready_state_;
    std::unordered_map<u32, std::atomic<bool>> node_completion_state_;
    std::queue<u32> ready_nodes_;
    mutable std::mutex ready_queue_mutex_;
    
    // Conditional dependency evaluation
    std::unordered_map<std::string, std::function<bool()>> condition_evaluators_;
    mutable std::shared_mutex conditions_mutex_;
    
    // Performance tracking
    struct {
        std::atomic<u64> resolutions_performed{0};
        std::atomic<u64> nodes_made_ready{0};
        std::atomic<f64> average_resolution_time{0.0};
        std::atomic<u32> max_concurrent_ready{0};
    } stats_;

public:
    explicit DependencyResolver(DependencyGraph* graph);
    ~DependencyResolver();
    
    // Resolution control
    void start_resolution();
    void stop_resolution();
    bool is_active() const { return resolution_active_.load(); }
    
    // Node state management
    void mark_node_ready(u32 node_id);
    void mark_node_completed(u32 node_id);
    void mark_node_failed(u32 node_id);
    void reset_node_state(u32 node_id);
    void reset_all_states();
    
    bool is_node_ready(u32 node_id) const;
    bool is_node_completed(u32 node_id) const;
    std::vector<u32> get_ready_nodes();
    usize get_ready_count() const;
    
    // Conditional dependencies
    void register_condition(const std::string& condition_name, 
                           std::function<bool()> evaluator);
    void unregister_condition(const std::string& condition_name);
    bool evaluate_condition(const std::string& condition_name) const;
    void update_conditional_dependencies();
    
    // Advanced resolution
    std::vector<u32> resolve_next_batch(usize max_batch_size = 0);
    void resolve_dependencies_parallel();
    bool wait_for_dependencies(u32 node_id, std::chrono::milliseconds timeout);
    
    // Information
    usize pending_nodes() const;
    usize completed_nodes() const;
    f64 completion_percentage() const;
    std::vector<u32> get_blocked_nodes() const;
    
    // Statistics
    struct Statistics {
        u64 total_resolutions;
        u64 total_nodes_ready;
        f64 average_resolution_time;
        u32 current_ready_nodes;
        u32 max_concurrent_ready;
        f64 resolution_efficiency;
    };
    Statistics get_statistics() const;
    void reset_statistics();

private:
    void process_node_completion(u32 completed_node_id);
    void evaluate_dependent_nodes(u32 completed_node_id);
    bool check_node_dependencies_satisfied(u32 node_id) const;
    void update_ready_queue();
};

} // namespace ecscope::scheduling