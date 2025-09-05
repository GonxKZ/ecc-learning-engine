#pragma once

/**
 * @file visual_ecs_inspector.hpp
 * @brief Advanced Visual ECS Inspector with Archetype Visualization
 * 
 * This comprehensive inspector provides advanced visualization capabilities for the
 * ECScope educational ECS platform, integrating real-time archetype analysis,
 * system profiling, memory usage visualization, and interactive entity browsing.
 * 
 * Key Features:
 * - Real-time archetype visualization with component relationships
 * - Interactive graph showing entity distribution across archetypes
 * - Dynamic system execution profiling with bottleneck identification
 * - Memory usage patterns visualization with cache analysis
 * - Interactive entity browser with live editing capabilities
 * - Sparse set storage visualization and analysis
 * - System dependency resolution visualization
 * - Performance heatmaps and optimization suggestions
 * 
 * Educational Components:
 * - Archetype-based ECS architecture demonstration
 * - Memory layout and cache-friendly data structures
 * - System scheduling and parallelization concepts
 * - Component-oriented programming patterns
 * - Performance profiling and optimization techniques
 * 
 * @author ECScope Educational ECS Framework
 * @date 2024
 */

#include "../overlay.hpp"
#include "ecs/registry.hpp"
#include "ecs/archetype.hpp"
#include "ecs/system.hpp"
#include "memory/memory_tracker.hpp"
#include "core/types.hpp"

// Forward declaration to avoid circular dependency
namespace ecscope::visualization {
    struct SparseSetVisualizationData;
}
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <array>
#include <deque>
#include <mutex>
#include <chrono>
#include <functional>

namespace ecscope::ui {

/**
 * @brief Archetype visualization node for graph display
 */
struct ArchetypeNode {
    u32 archetype_id;                       // Unique archetype identifier
    std::string signature_hash;             // Component signature hash for display
    std::vector<std::string> component_names; // Human-readable component names
    usize entity_count;                     // Number of entities in this archetype
    usize memory_usage;                     // Memory usage in bytes
    f64 creation_rate;                      // Entities created per second
    f64 destruction_rate;                   // Entities destroyed per second
    f64 access_frequency;                   // How often this archetype is accessed
    
    // Visual properties
    ImVec2 position;                        // Node position in graph
    ImVec2 size;                            // Node size
    ImU32 color;                            // Node color based on activity
    bool is_selected;                       // Selection state
    bool is_hot;                            // High activity indicator
    
    // Relationships
    std::vector<u32> connected_archetypes;  // Related archetypes (entity transitions)
    std::vector<f32> transition_weights;    // Transition frequency weights
    
    ArchetypeNode() noexcept 
        : archetype_id(0), entity_count(0), memory_usage(0)
        , creation_rate(0.0), destruction_rate(0.0), access_frequency(0.0)
        , position(0, 0), size(100, 60), color(IM_COL32_WHITE)
        , is_selected(false), is_hot(false) {}
};

/**
 * @brief System execution node for dependency visualization
 */
struct SystemExecutionNode {
    std::string system_name;                // System name
    ecs::SystemPhase phase;                 // Execution phase
    f64 average_execution_time;             // Average execution time in ms
    f64 last_execution_time;                // Last execution time
    f64 time_budget;                        // Allocated time budget
    f64 budget_utilization;                 // Budget utilization percentage
    bool is_over_budget;                    // Budget exceeded flag
    
    // Dependencies
    std::vector<std::string> dependencies;  // System dependencies
    std::vector<std::string> dependents;    // Systems depending on this one
    
    // Visual properties
    ImVec2 position;                        // Node position
    ImU32 color;                            // Color based on performance
    bool is_bottleneck;                     // Performance bottleneck indicator
    
    SystemExecutionNode() noexcept
        : phase(ecs::SystemPhase::Update), average_execution_time(0.0)
        , last_execution_time(0.0), time_budget(16.6), budget_utilization(0.0)
        , is_over_budget(false), position(0, 0), color(IM_COL32_WHITE)
        , is_bottleneck(false) {}
};

/**
 * @brief Memory allocation visualization data
 */
struct MemoryVisualizationData {
    struct AllocationBlock {
        void* address;                      // Memory address
        usize size;                         // Block size
        memory::AllocationCategory category; // Allocation category
        f64 age;                           // How long it's been allocated
        bool is_active;                    // Still allocated
        bool is_hot;                       // Frequently accessed
        ImVec2 position;                   // Visual position
        ImU32 color;                       // Color based on category/age
        
        AllocationBlock() noexcept
            : address(nullptr), size(0)
            , category(memory::AllocationCategory::Unknown)
            , age(0.0), is_active(true), is_hot(false)
            , position(0, 0), color(IM_COL32_WHITE) {}
    };
    
    std::vector<AllocationBlock> blocks;
    usize total_allocated;                  // Total memory allocated
    usize peak_allocated;                   // Peak allocation
    f64 fragmentation_ratio;                // Memory fragmentation
    f64 cache_hit_rate;                     // Estimated cache hit rate
    
    MemoryVisualizationData() noexcept
        : total_allocated(0), peak_allocated(0)
        , fragmentation_ratio(0.0), cache_hit_rate(0.95) {}
};

/**
 * @brief Interactive entity browser data
 */
struct EntityBrowserData {
    struct EntityEntry {
        ecs::Entity entity;                 // Entity handle
        std::string archetype_name;         // Archetype signature
        std::vector<std::string> components; // Component names
        bool is_selected;                   // Selection state
        bool matches_filter;                // Filter match
        f64 last_modified;                  // Last modification time
        
        EntityEntry() noexcept
            : entity(0), is_selected(false)
            , matches_filter(true), last_modified(0.0) {}
    };
    
    std::vector<EntityEntry> entities;
    std::string search_filter;              // Search text
    std::string component_filter;           // Component type filter
    std::unordered_set<std::string> selected_archetypes; // Archetype filter
    bool show_only_modified;                // Filter recently modified
    
    // Sorting options
    enum class SortMode {
        ByEntity,
        ByArchetype,
        ByComponentCount,
        ByLastModified
    };
    SortMode sort_mode;
    bool sort_ascending;
    
    EntityBrowserData() noexcept
        : show_only_modified(false)
        , sort_mode(SortMode::ByEntity)
        , sort_ascending(true) {}
};

/**
 * @brief Sparse set visualization data
 */
struct SparseSetVisualization {
    struct ComponentPool {
        std::string component_name;         // Component type name
        usize dense_count;                  // Dense array size
        usize sparse_size;                  // Sparse array size
        usize capacity;                     // Total capacity
        f64 utilization;                    // Space utilization
        f64 access_pattern_score;           // Cache-friendliness score
        std::vector<bool> dense_occupied;   // Dense array occupancy
        std::vector<bool> sparse_valid;     // Sparse array validity
        
        ComponentPool() noexcept
            : dense_count(0), sparse_size(0), capacity(0)
            , utilization(0.0), access_pattern_score(1.0) {}
    };
    
    std::vector<ComponentPool> pools;
    f64 overall_memory_efficiency;          // Overall efficiency
    f64 cache_locality_score;               // Cache locality rating
    
    SparseSetVisualization() noexcept
        : overall_memory_efficiency(0.0)
        , cache_locality_score(0.0) {}
};

/**
 * @brief Performance timeline data
 */
struct PerformanceTimeline {
    static constexpr usize TIMELINE_SAMPLES = 1000;
    
    struct TimelineSample {
        f64 timestamp;                      // Sample timestamp
        f64 frame_time;                     // Total frame time
        f64 system_time;                    // System execution time
        f64 memory_usage;                   // Memory usage percentage
        u64 entity_count;                   // Total entity count
        u64 archetype_count;                // Total archetype count
        
        TimelineSample() noexcept
            : timestamp(0.0), frame_time(0.0), system_time(0.0)
            , memory_usage(0.0), entity_count(0), archetype_count(0) {}
    };
    
    std::array<TimelineSample, TIMELINE_SAMPLES> samples;
    usize current_index;                    // Current sample index
    f64 sample_interval;                    // Sampling interval
    f64 last_sample_time;                   // Last sample timestamp
    
    PerformanceTimeline() noexcept
        : current_index(0), sample_interval(0.1)
        , last_sample_time(0.0) {
        samples.fill(TimelineSample{});
    }
    
    void add_sample(const TimelineSample& sample) noexcept {
        samples[current_index] = sample;
        current_index = (current_index + 1) % TIMELINE_SAMPLES;
    }
    
    std::vector<TimelineSample> get_recent_samples(usize count = 100) const noexcept {
        std::vector<TimelineSample> result;
        result.reserve(count);
        
        usize start_index = current_index >= count ? current_index - count : 0;
        for (usize i = 0; i < count && i < TIMELINE_SAMPLES; ++i) {
            usize index = (start_index + i) % TIMELINE_SAMPLES;
            result.push_back(samples[index]);
        }
        
        return result;
    }
};

/**
 * @brief Advanced Visual ECS Inspector Panel
 * 
 * Provides comprehensive visualization and analysis of ECS architecture,
 * performance characteristics, and memory usage patterns.
 */
class VisualECSInspector : public Panel {
    // Friend declarations for integration functions
    friend void visual_inspector_integration::update_from_registry(VisualECSInspector&, const ecs::Registry&);
    friend void visual_inspector_integration::update_from_system_manager(VisualECSInspector&, const ecs::SystemManager&);
    friend void visual_inspector_integration::update_from_memory_tracker(VisualECSInspector&, const memory::MemoryTracker&);

private:
    // Core data structures
    std::vector<ArchetypeNode> archetype_nodes_;
    std::vector<SystemExecutionNode> system_nodes_;
    std::unique_ptr<MemoryVisualizationData> memory_data_;
    std::unique_ptr<EntityBrowserData> entity_browser_;
    std::unique_ptr<SparseSetVisualization> sparse_set_data_;
    std::unique_ptr<PerformanceTimeline> performance_timeline_;
    
    // Update state
    f64 last_update_time_;
    f64 update_frequency_;                  // Update frequency in Hz
    mutable std::mutex data_mutex_;         // Thread safety
    
    // Display options
    bool show_archetype_graph_;             // Show archetype relationship graph
    bool show_system_profiler_;             // Show system execution profiler
    bool show_memory_visualizer_;           // Show memory usage visualizer
    bool show_entity_browser_;              // Show interactive entity browser
    bool show_sparse_set_view_;             // Show sparse set visualization
    bool show_performance_timeline_;        // Show performance timeline
    bool show_educational_hints_;           // Show educational tooltips
    
    // Graph interaction state
    ImVec2 graph_pan_offset_;               // Graph panning offset
    f32 graph_zoom_;                        // Graph zoom level
    u32 selected_archetype_id_;             // Selected archetype
    std::string selected_system_name_;      // Selected system
    bool is_dragging_graph_;                // Graph drag state
    ImVec2 drag_start_pos_;                 // Drag start position
    
    // Filtering and search
    std::string archetype_search_;          // Archetype search filter
    std::string system_search_;             // System search filter
    std::unordered_set<std::string> component_filters_; // Component type filters
    bool filter_hot_archetypes_;            // Show only active archetypes
    bool filter_bottleneck_systems_;        // Show only bottleneck systems
    
    // Performance thresholds
    f64 hot_archetype_threshold_;           // Threshold for "hot" archetypes
    f64 system_bottleneck_threshold_;       // System bottleneck threshold
    f64 memory_pressure_threshold_;         // Memory pressure warning threshold
    
    // Educational content
    std::unordered_map<std::string, std::string> educational_tooltips_;
    bool show_concept_explanations_;        // Show ECS concept explanations
    
    // Export capabilities
    std::string export_directory_;          // Export directory path
    bool enable_data_export_;               // Enable data export
    
public:
    /**
     * @brief Constructor
     */
    VisualECSInspector();
    
    /**
     * @brief Destructor
     */
    ~VisualECSInspector() override = default;
    
    // Panel interface
    void render() override;
    void update(f64 delta_time) override;
    
    // Configuration
    void set_update_frequency(f64 frequency) noexcept { update_frequency_ = frequency; }
    void set_hot_archetype_threshold(f64 threshold) noexcept { hot_archetype_threshold_ = threshold; }
    void set_bottleneck_threshold(f64 threshold) noexcept { system_bottleneck_threshold_ = threshold; }
    
    // Display toggles
    void show_archetype_graph(bool show) noexcept { show_archetype_graph_ = show; }
    void show_system_profiler(bool show) noexcept { show_system_profiler_ = show; }
    void show_memory_visualizer(bool show) noexcept { show_memory_visualizer_ = show; }
    void show_entity_browser(bool show) noexcept { show_entity_browser_ = show; }
    void show_sparse_set_view(bool show) noexcept { show_sparse_set_view_ = show; }
    void show_performance_timeline(bool show) noexcept { show_performance_timeline_ = show; }
    void show_educational_hints(bool show) noexcept { show_educational_hints_ = show; }
    
    // Data access
    const std::vector<ArchetypeNode>& archetype_nodes() const noexcept { return archetype_nodes_; }
    const std::vector<SystemExecutionNode>& system_nodes() const noexcept { return system_nodes_; }
    const MemoryVisualizationData* memory_data() const noexcept { return memory_data_.get(); }
    const PerformanceTimeline* performance_timeline() const noexcept { return performance_timeline_.get(); }
    
    // Export functionality
    void export_archetype_data(const std::string& filename) const noexcept;
    void export_system_performance(const std::string& filename) const noexcept;
    void export_memory_analysis(const std::string& filename) const noexcept;
    void export_performance_timeline(const std::string& filename) const noexcept;
    
private:
    // Core update functions
    void update_archetype_data() noexcept;
    void update_system_data() noexcept;
    void update_memory_data() noexcept;
    void update_entity_browser_data() noexcept;
    void update_sparse_set_data() noexcept;
    void update_performance_timeline() noexcept;
    
    // Rendering functions
    void render_main_menu_bar();
    void render_archetype_graph();
    void render_system_profiler();
    void render_memory_visualizer();
    void render_entity_browser();
    void render_sparse_set_visualization();
    void render_performance_timeline();
    void render_statistics_summary();
    
    // Archetype graph rendering
    void render_archetype_node(const ArchetypeNode& node);
    void render_archetype_connections();
    void handle_archetype_node_interaction(ArchetypeNode& node);
    void update_archetype_node_positions();
    ImU32 calculate_archetype_node_color(const ArchetypeNode& node) const noexcept;
    
    // System profiler rendering
    void render_system_execution_graph();
    void render_system_dependency_view();
    void render_system_performance_bars();
    void render_system_timeline();
    ImU32 calculate_system_node_color(const SystemExecutionNode& node) const noexcept;
    
    // Memory visualizer rendering
    void render_memory_allocation_map();
    void render_memory_category_breakdown();
    void render_memory_pressure_gauge();
    void render_cache_performance_analysis();
    
    // Entity browser rendering
    void render_entity_list();
    void render_entity_details();
    void render_component_editor();
    void render_entity_search_filters();
    
    // Sparse set rendering
    void render_component_pool_visualization();
    void render_dense_sparse_arrays();
    void render_dense_sparse_arrays_for_set(const visualization::SparseSetVisualizationData& sparse_set);
    void render_cache_locality_analysis();
    void render_educational_sparse_set_content();
    
    // Performance timeline rendering
    void render_timeline_graphs();
    void render_performance_metrics();
    void render_bottleneck_analysis();
    
    // Interaction handling
    void handle_graph_interaction();
    void handle_node_selection();
    void handle_component_editing();
    void handle_entity_creation_deletion();
    
    // Educational features
    void render_educational_tooltips();
    void show_concept_tooltip(const std::string& concept) const noexcept;
    void initialize_educational_content() noexcept;
    
    // Analysis functions
    void analyze_archetype_relationships() noexcept;
    void detect_system_bottlenecks() noexcept;
    void analyze_memory_patterns() noexcept;
    void calculate_performance_scores() noexcept;
    
    // Utility functions
    std::string format_memory_size(usize bytes) const noexcept;
    std::string format_time_duration(f64 seconds) const noexcept;
    std::string format_time_duration_us(f64 microseconds) const noexcept;
    std::string format_percentage(f64 value) const noexcept;
    ImVec4 interpolate_color(const ImVec4& start, const ImVec4& end, f32 t) const noexcept;
    ImU32 heat_map_color(f32 intensity) const noexcept;
    
    // Layout algorithms
    void layout_archetype_nodes_force_directed() noexcept;
    void layout_system_nodes_hierarchical() noexcept;
    ImVec2 calculate_optimal_node_size(const std::string& text) const noexcept;
    
    // Data collection integration
    void collect_archetype_statistics() noexcept;
    void collect_system_statistics() noexcept;
    void collect_memory_statistics() noexcept;
    void collect_entity_statistics() noexcept;
    
    // Performance optimization
    bool should_update_data() const noexcept;
    void optimize_rendering_performance() noexcept;
    void cache_expensive_calculations() noexcept;
    
    // Color schemes and themes
    struct ColorScheme {
        ImU32 archetype_default;
        ImU32 archetype_hot;
        ImU32 archetype_selected;
        ImU32 system_normal;
        ImU32 system_bottleneck;
        ImU32 system_over_budget;
        ImU32 memory_low;
        ImU32 memory_medium;
        ImU32 memory_high;
        ImU32 memory_critical;
    };
    
    ColorScheme current_color_scheme_;
    void initialize_color_scheme() noexcept;
    void apply_dark_theme() noexcept;
    void apply_light_theme() noexcept;
};

/**
 * @brief Factory function to create Visual ECS Inspector
 */
std::unique_ptr<VisualECSInspector> create_visual_ecs_inspector();

/**
 * @brief Integration helpers for connecting with ECS systems
 */
namespace visual_inspector_integration {
    /**
     * @brief Register the visual inspector with the UI system
     */
    void register_inspector(ui::Overlay& overlay);
    
    /**
     * @brief Update inspector data from ECS registry
     */
    void update_from_registry(VisualECSInspector& inspector, const ecs::Registry& registry);
    
    /**
     * @brief Update inspector data from system manager
     */
    void update_from_system_manager(VisualECSInspector& inspector, const ecs::SystemManager& system_manager);
    
    /**
     * @brief Update inspector data from memory tracker
     */
    void update_from_memory_tracker(VisualECSInspector& inspector, const memory::MemoryTracker& tracker);
    
    /**
     * @brief Enable automatic updates with specified frequency
     */
    void enable_automatic_updates(VisualECSInspector& inspector, f64 frequency = 10.0);
    
    /**
     * @brief Disable automatic updates
     */
    void disable_automatic_updates(VisualECSInspector& inspector);
}

} // namespace ecscope::ui