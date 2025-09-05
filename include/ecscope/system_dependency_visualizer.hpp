#pragma once

/**
 * @file system_dependency_visualizer.hpp
 * @brief Advanced System Dependency Visualization and Analysis
 * 
 * This module provides comprehensive visualization and analysis of ECS system
 * dependencies, execution flow, and performance characteristics. It integrates
 * with the Visual ECS Inspector to provide educational insights into system
 * architecture, scheduling, and optimization opportunities.
 * 
 * Key Features:
 * - Interactive system dependency graph visualization
 * - Real-time execution flow monitoring and bottleneck detection
 * - System performance profiling with detailed timing analysis
 * - Dependency cycle detection and resolution suggestions
 * - Parallel execution opportunity identification
 * - Resource conflict analysis and optimization suggestions
 * 
 * Educational Components:
 * - System architecture best practices demonstration
 * - Dependency management and scheduling concepts
 * - Performance profiling and optimization techniques
 * - Parallel execution and thread safety concepts
 * - Resource management and conflict resolution
 * 
 * @author ECScope Educational ECS Framework
 * @date 2024
 */

#include "core/types.hpp"
#include "ecs/system.hpp"
#include "visual_ecs_inspector.hpp"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>
#include <chrono>
#include <functional>
#include <queue>

namespace ecscope::analysis {

/**
 * @brief System dependency node for graph visualization
 */
struct SystemDependencyNode {
    std::string system_name;                // System identifier
    ecs::SystemPhase execution_phase;       // When the system executes
    ecs::SystemExecutionType execution_type; // How the system executes
    
    // Dependencies
    std::vector<std::string> hard_dependencies;  // Must-complete-before dependencies
    std::vector<std::string> soft_dependencies;  // Preferred-order dependencies
    std::vector<std::string> dependents;         // Systems that depend on this one
    
    // Resource requirements
    std::vector<std::string> read_resources;     // Resources this system reads
    std::vector<std::string> write_resources;    // Resources this system writes
    std::vector<std::string> exclusive_resources; // Resources requiring exclusive access
    
    // Performance characteristics
    f64 average_execution_time;             // Average execution time (ms)
    f64 min_execution_time;                 // Minimum execution time (ms)
    f64 max_execution_time;                 // Maximum execution time (ms)
    f64 execution_variance;                 // Execution time variance
    f64 last_execution_time;                // Most recent execution time
    u64 execution_count;                    // Total number of executions
    
    // Scheduling information
    u32 execution_order;                    // Order within phase
    bool can_run_parallel;                  // Can execute in parallel
    bool is_bottleneck;                     // Currently a performance bottleneck
    f64 idle_time_percentage;               // Percentage of time spent waiting
    
    // Visual properties
    ImVec2 position;                        // Position in visualization
    ImVec2 size;                           // Node size
    ImU32 color;                           // Node color based on performance
    bool is_selected;                       // Selection state
    bool is_highlighted;                    // Highlight state (dependencies)
    
    // Analysis results
    f64 criticality_score;                  // How critical this system is (0.0-1.0)
    f64 optimization_potential;             // Optimization potential (0.0-1.0)
    std::vector<std::string> optimization_suggestions;
    
    SystemDependencyNode() noexcept
        : execution_phase(ecs::SystemPhase::Update)
        , execution_type(ecs::SystemExecutionType::Sequential)
        , average_execution_time(0.0), min_execution_time(0.0), max_execution_time(0.0)
        , execution_variance(0.0), last_execution_time(0.0), execution_count(0)
        , execution_order(0), can_run_parallel(false), is_bottleneck(false)
        , idle_time_percentage(0.0), position(0, 0), size(150, 80)
        , color(IM_COL32_WHITE), is_selected(false), is_highlighted(false)
        , criticality_score(0.0), optimization_potential(0.0) {}
};

/**
 * @brief System execution timeline event
 */
struct SystemExecutionEvent {
    std::string system_name;                // System that executed
    f64 start_time;                        // Execution start timestamp
    f64 end_time;                          // Execution end timestamp
    f64 duration;                          // Execution duration (ms)
    ecs::SystemPhase phase;                // Execution phase
    std::thread::id thread_id;             // Thread that executed the system
    bool was_parallel;                     // Was executed in parallel
    std::vector<std::string> waited_for;   // Systems this system waited for
    f64 wait_time;                         // Time spent waiting for dependencies
    
    SystemExecutionEvent() noexcept
        : start_time(0.0), end_time(0.0), duration(0.0)
        , phase(ecs::SystemPhase::Update)
        , thread_id(std::this_thread::get_id())
        , was_parallel(false), wait_time(0.0) {}
};

/**
 * @brief Dependency cycle information
 */
struct DependencyCycle {
    std::vector<std::string> systems_in_cycle; // Systems forming the cycle
    f64 severity_score;                     // How problematic the cycle is
    std::vector<std::string> suggested_breaks; // Suggested dependency breaks
    std::string description;                // Human-readable description
    bool is_hard_cycle;                     // Involves hard dependencies
    
    DependencyCycle() noexcept
        : severity_score(0.0), is_hard_cycle(false) {}
};

/**
 * @brief Resource conflict analysis
 */
struct ResourceConflict {
    std::string resource_name;              // Conflicting resource
    std::vector<std::string> conflicting_systems; // Systems in conflict
    enum class ConflictType {
        ReadWrite,                          // Read-write conflict
        WriteWrite,                         // Write-write conflict  
        ExclusiveAccess                     // Exclusive access conflict
    } type;
    
    f64 conflict_frequency;                 // How often conflict occurs
    f64 performance_impact;                 // Performance impact of conflict
    std::vector<std::string> resolution_suggestions; // How to resolve
    
    ResourceConflict() noexcept
        : type(ConflictType::ReadWrite), conflict_frequency(0.0)
        , performance_impact(0.0) {}
};

/**
 * @brief System execution phase analysis
 */
struct PhaseAnalysis {
    ecs::SystemPhase phase;                 // Phase being analyzed
    std::vector<std::string> systems;       // Systems in this phase
    f64 total_execution_time;               // Total time for phase
    f64 critical_path_time;                 // Critical path execution time
    f64 parallelization_efficiency;         // How well parallelized (0.0-1.0)
    usize max_parallel_systems;             // Maximum systems that can run in parallel
    usize average_parallel_systems;         // Average systems running in parallel
    std::vector<std::string> bottleneck_systems; // Bottleneck systems in phase
    
    PhaseAnalysis() noexcept
        : phase(ecs::SystemPhase::Update), total_execution_time(0.0)
        , critical_path_time(0.0), parallelization_efficiency(0.0)
        , max_parallel_systems(0), average_parallel_systems(0) {}
};

/**
 * @brief System dependency visualizer and analyzer
 */
class SystemDependencyVisualizer {
private:
    // Core data
    std::vector<SystemDependencyNode> dependency_nodes_;
    std::vector<SystemExecutionEvent> execution_timeline_;
    std::vector<DependencyCycle> dependency_cycles_;
    std::vector<ResourceConflict> resource_conflicts_;
    std::array<PhaseAnalysis, static_cast<usize>(ecs::SystemPhase::COUNT)> phase_analyses_;
    
    // Analysis state
    f64 last_analysis_time_;                // Last analysis timestamp
    f64 analysis_frequency_;                // Analysis update frequency
    usize max_timeline_events_;             // Maximum events to keep in timeline
    
    // Visualization state
    ImVec2 graph_pan_offset_;               // Graph panning offset
    f32 graph_zoom_;                        // Graph zoom level
    std::string selected_system_;           // Currently selected system
    bool show_dependency_arrows_;           // Show dependency arrows
    bool show_resource_conflicts_;          // Show resource conflict indicators
    bool show_performance_overlay_;         // Show performance color overlay
    bool show_timeline_;                    // Show execution timeline
    
    // Layout algorithm state
    std::unordered_map<std::string, ImVec2> node_velocities_; // For force-directed layout
    f32 layout_spring_strength_;            // Spring force strength
    f32 layout_repulsion_strength_;         // Node repulsion strength
    f32 layout_damping_;                    // Velocity damping
    
    // Educational content
    std::unordered_map<std::string, std::string> educational_tooltips_;
    bool show_educational_overlays_;        // Show educational information
    
    // Performance monitoring
    std::chrono::high_resolution_clock::time_point last_update_;
    f64 analysis_overhead_;                 // Time spent in analysis
    f64 visualization_overhead_;            // Time spent in visualization
    
public:
    /**
     * @brief Constructor
     */
    explicit SystemDependencyVisualizer();
    
    /**
     * @brief Destructor
     */
    ~SystemDependencyVisualizer() = default;
    
    // Core update and analysis
    void update(f64 delta_time) noexcept;
    void analyze_dependencies() noexcept;
    void analyze_performance() noexcept;
    void detect_dependency_cycles() noexcept;
    void analyze_resource_conflicts() noexcept;
    void analyze_phases() noexcept;
    
    // Visualization
    void render_dependency_graph() noexcept;
    void render_execution_timeline() noexcept;
    void render_phase_analysis() noexcept;
    void render_conflict_analysis() noexcept;
    void render_optimization_suggestions() noexcept;
    
    // Configuration
    void set_analysis_frequency(f64 frequency) noexcept { analysis_frequency_ = frequency; }
    void set_max_timeline_events(usize max_events) noexcept { max_timeline_events_ = max_events; }
    void set_show_dependency_arrows(bool show) noexcept { show_dependency_arrows_ = show; }
    void set_show_resource_conflicts(bool show) noexcept { show_resource_conflicts_ = show; }
    void set_show_performance_overlay(bool show) noexcept { show_performance_overlay_ = show; }
    void set_show_timeline(bool show) noexcept { show_timeline_ = show; }
    void set_show_educational_overlays(bool show) noexcept { show_educational_overlays_ = show; }
    
    // Data access
    const std::vector<SystemDependencyNode>& dependency_nodes() const noexcept { return dependency_nodes_; }
    const std::vector<SystemExecutionEvent>& execution_timeline() const noexcept { return execution_timeline_; }
    const std::vector<DependencyCycle>& dependency_cycles() const noexcept { return dependency_cycles_; }
    const std::vector<ResourceConflict>& resource_conflicts() const noexcept { return resource_conflicts_; }
    const PhaseAnalysis& phase_analysis(ecs::SystemPhase phase) const noexcept {
        return phase_analyses_[static_cast<usize>(phase)];
    }
    
    // Query functions
    const SystemDependencyNode* get_system_node(const std::string& system_name) const noexcept;
    std::vector<std::string> get_bottleneck_systems() const noexcept;
    std::vector<std::string> get_parallel_execution_candidates() const noexcept;
    std::vector<std::string> get_dependency_path(const std::string& from, const std::string& to) const noexcept;
    
    // Analysis results
    f64 get_overall_parallelization_efficiency() const noexcept;
    f64 get_critical_path_time() const noexcept;
    f64 get_system_criticality(const std::string& system_name) const noexcept;
    std::vector<std::string> suggest_dependency_optimizations() const noexcept;
    
    // Event recording (called by system manager)
    void record_system_execution(const SystemExecutionEvent& event) noexcept;
    void record_dependency_wait(const std::string& system, const std::string& waited_for, f64 wait_time) noexcept;
    void record_resource_conflict(const std::string& resource, const std::vector<std::string>& systems) noexcept;
    
    // Educational features
    std::vector<std::string> get_educational_insights() const noexcept;
    std::string explain_system_dependencies(const std::string& system_name) const noexcept;
    std::string suggest_architecture_improvements() const noexcept;
    
    // Export capabilities
    void export_dependency_graph(const std::string& filename) const noexcept;
    void export_execution_timeline(const std::string& filename) const noexcept;
    void export_performance_analysis(const std::string& filename) const noexcept;
    void export_optimization_report(const std::string& filename) const noexcept;
    
    // Statistics
    f64 get_analysis_overhead() const noexcept { return analysis_overhead_; }
    f64 get_visualization_overhead() const noexcept { return visualization_overhead_; }
    usize get_system_count() const noexcept { return dependency_nodes_.size(); }
    usize get_dependency_count() const noexcept;
    usize get_detected_cycles() const noexcept { return dependency_cycles_.size(); }
    
private:
    // Internal analysis functions
    void collect_system_data() noexcept;
    void update_node_performance(SystemDependencyNode& node) noexcept;
    void calculate_criticality_scores() noexcept;
    void detect_bottlenecks() noexcept;
    void analyze_parallel_opportunities() noexcept;
    
    // Dependency analysis
    bool has_dependency_path(const std::string& from, const std::string& to) const noexcept;
    std::vector<std::string> find_dependency_path_recursive(const std::string& current, const std::string& target,
                                                           std::unordered_set<std::string>& visited) const noexcept;
    void detect_cycles_tarjan() noexcept;
    
    // Resource conflict detection
    void check_resource_read_write_conflicts() noexcept;
    void check_exclusive_resource_conflicts() noexcept;
    void analyze_conflict_impact() noexcept;
    
    // Performance analysis
    void calculate_critical_path() noexcept;
    void analyze_execution_patterns() noexcept;
    void calculate_parallelization_efficiency() noexcept;
    
    // Layout algorithms
    void update_force_directed_layout() noexcept;
    void apply_hierarchical_layout() noexcept;
    ImVec2 calculate_optimal_node_position(const SystemDependencyNode& node) const noexcept;
    
    // Rendering helpers
    void render_dependency_node(const SystemDependencyNode& node) noexcept;
    void render_dependency_arrows() noexcept;
    void render_resource_conflict_indicators() noexcept;
    void render_performance_color_overlay() noexcept;
    void render_timeline_bars() noexcept;
    void render_phase_breakdown() noexcept;
    
    // Interaction handling
    void handle_node_selection() noexcept;
    void handle_graph_navigation() noexcept;
    void handle_timeline_interaction() noexcept;
    
    // Utility functions
    ImU32 calculate_node_color(const SystemDependencyNode& node) const noexcept;
    ImU32 calculate_dependency_arrow_color(const std::string& from, const std::string& to) const noexcept;
    f32 calculate_arrow_thickness(const std::string& from, const std::string& to) const noexcept;
    std::string format_execution_time(f64 time_ms) const noexcept;
    
    // Educational content
    void initialize_educational_content() noexcept;
    void show_dependency_tooltip(const std::string& concept) const noexcept;
    
    // Data management
    void cleanup_old_timeline_events() noexcept;
    void compress_historical_data() noexcept;
    bool should_perform_analysis() const noexcept;
    
    // Constants
    static constexpr f64 DEFAULT_ANALYSIS_FREQUENCY = 5.0; // 5 Hz
    static constexpr usize DEFAULT_MAX_TIMELINE_EVENTS = 1000;
    static constexpr f32 DEFAULT_GRAPH_ZOOM = 1.0f;
    static constexpr f32 GRAPH_ZOOM_MIN = 0.1f;
    static constexpr f32 GRAPH_ZOOM_MAX = 5.0f;
    static constexpr f32 DEFAULT_SPRING_STRENGTH = 0.02f;
    static constexpr f32 DEFAULT_REPULSION_STRENGTH = 1000.0f;
    static constexpr f32 DEFAULT_DAMPING = 0.8f;
};

/**
 * @brief Integration with Visual ECS Inspector
 */
namespace system_dependency_integration {
    /**
     * @brief Create visualizer optimized for inspector integration
     */
    std::unique_ptr<SystemDependencyVisualizer> create_for_inspector();
    
    /**
     * @brief Update inspector system nodes from dependency analysis
     */
    void update_inspector_system_nodes(const SystemDependencyVisualizer& visualizer,
                                      std::vector<ui::SystemExecutionNode>& system_nodes);
    
    /**
     * @brief Create dependency tooltips for inspector
     */
    std::unordered_map<std::string, std::string> create_dependency_tooltips(
        const SystemDependencyVisualizer& visualizer);
    
    /**
     * @brief Render dependency-specific UI components
     */
    void render_dependency_panel(const SystemDependencyVisualizer& visualizer);
    void render_bottleneck_analysis_panel(const SystemDependencyVisualizer& visualizer);
    void render_parallel_optimization_panel(const SystemDependencyVisualizer& visualizer);
}

/**
 * @brief Real-time system performance monitor
 */
class SystemPerformanceMonitor {
private:
    std::unique_ptr<SystemDependencyVisualizer> visualizer_;
    std::function<void(const std::string&)> alert_callback_;
    
    // Alert thresholds
    f64 execution_time_spike_threshold_;    // Threshold for execution time spikes
    f64 dependency_wait_threshold_;         // Threshold for dependency wait times
    f64 bottleneck_threshold_;              // Threshold for bottleneck detection
    
    // Monitoring state
    bool monitoring_enabled_;
    std::unordered_set<std::string> alerted_systems_;
    std::unordered_map<std::string, f64> baseline_execution_times_;
    
public:
    explicit SystemPerformanceMonitor(std::function<void(const std::string&)> alert_callback = nullptr);
    
    void update(f64 delta_time) noexcept;
    void enable_monitoring(bool enabled) noexcept { monitoring_enabled_ = enabled; }
    
    void set_execution_time_threshold(f64 threshold) noexcept { execution_time_spike_threshold_ = threshold; }
    void set_dependency_wait_threshold(f64 threshold) noexcept { dependency_wait_threshold_ = threshold; }
    void set_bottleneck_threshold(f64 threshold) noexcept { bottleneck_threshold_ = threshold; }
    
    const SystemDependencyVisualizer& visualizer() const noexcept { return *visualizer_; }
    
    // Manual alert triggers
    void check_execution_time_spikes() noexcept;
    void check_dependency_bottlenecks() noexcept;
    void check_resource_conflicts() noexcept;
    
private:
    void establish_performance_baselines() noexcept;
    void send_alert(const std::string& message) noexcept;
    bool is_execution_time_spike(const std::string& system, f64 current_time) const noexcept;
};

/**
 * @brief Dependency-aware system scheduler optimization
 */
class DependencyOptimizer {
private:
    const SystemDependencyVisualizer* visualizer_;
    
    // Optimization settings
    bool enable_parallel_optimization_;     // Enable parallel execution optimization
    bool enable_dependency_relaxation_;     // Enable soft dependency relaxation
    bool enable_resource_pooling_;          // Enable resource pooling suggestions
    
public:
    explicit DependencyOptimizer(const SystemDependencyVisualizer* visualizer);
    
    // Optimization analysis
    std::vector<std::string> suggest_parallel_groups() const noexcept;
    std::vector<std::string> suggest_dependency_reductions() const noexcept;
    std::vector<std::string> suggest_resource_optimizations() const noexcept;
    std::vector<std::string> suggest_phase_restructuring() const noexcept;
    
    // Performance predictions
    f64 predict_parallel_speedup(const std::vector<std::string>& parallel_group) const noexcept;
    f64 predict_dependency_removal_impact(const std::string& from, const std::string& to) const noexcept;
    f64 predict_phase_restructure_impact(const std::vector<std::string>& systems, 
                                        ecs::SystemPhase new_phase) const noexcept;
    
    // Configuration
    void enable_parallel_optimization(bool enable) noexcept { enable_parallel_optimization_ = enable; }
    void enable_dependency_relaxation(bool enable) noexcept { enable_dependency_relaxation_ = enable; }
    void enable_resource_pooling(bool enable) noexcept { enable_resource_pooling_ = enable; }
    
private:
    // Optimization algorithms
    std::vector<std::vector<std::string>> find_parallel_groups() const noexcept;
    std::vector<std::pair<std::string, std::string>> find_removable_dependencies() const noexcept;
    std::unordered_map<std::string, std::vector<std::string>> analyze_resource_sharing() const noexcept;
    
    // Analysis helpers
    bool can_systems_run_parallel(const std::string& system_a, const std::string& system_b) const noexcept;
    f64 calculate_dependency_strength(const std::string& from, const std::string& to) const noexcept;
    f64 estimate_parallel_overhead() const noexcept;
};

} // namespace ecscope::analysis