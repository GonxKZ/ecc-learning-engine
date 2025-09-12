/**
 * @file ecs_inspector_widgets.hpp
 * @brief Advanced Inspector Widgets and Visualization Components
 * 
 * Specialized widgets for ECS debugging including component editors,
 * archetype visualizers, system dependency graphs, and performance charts.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#pragma once

#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <unordered_map>
#include <chrono>
#include <queue>

#include "ecs_inspector.hpp"
#include "core.hpp"

#ifdef ECSCOPE_HAS_IMGUI
#include <imgui.h>
#include <imgui_internal.h>
#endif

namespace ecscope::gui {

// =============================================================================
// ADVANCED COMPONENT EDITORS
// =============================================================================

/**
 * @brief Generic component property editor interface
 */
class ComponentPropertyEditor {
public:
    virtual ~ComponentPropertyEditor() = default;
    
    /**
     * @brief Render the property editor UI
     * @param property_name Name of the property being edited
     * @param data Pointer to the property data
     * @param changed Output parameter - set to true if value was modified
     * @return true if the property is valid and was rendered
     */
    virtual bool render_property(const std::string& property_name, void* data, bool& changed) = 0;
    
    /**
     * @brief Get the size of the property in bytes
     */
    virtual size_t get_property_size() const = 0;
    
    /**
     * @brief Validate the property value
     */
    virtual bool validate_property(const void* data) const { return true; }
    
    /**
     * @brief Get property type name for display
     */
    virtual std::string get_type_name() const = 0;
};

/**
 * @brief Template-based property editor for basic types
 */
template<typename T>
class BasicPropertyEditor : public ComponentPropertyEditor {
public:
    explicit BasicPropertyEditor(T min_val = T{}, T max_val = T{}) 
        : min_value_(min_val), max_value_(max_val) {}
    
    bool render_property(const std::string& property_name, void* data, bool& changed) override;
    size_t get_property_size() const override { return sizeof(T); }
    std::string get_type_name() const override;
    
private:
    T min_value_;
    T max_value_;
};

/**
 * @brief Advanced component editor with reflection-like capabilities
 */
class ComponentEditor {
public:
    struct PropertyInfo {
        std::string name;
        std::string display_name;
        std::string description;
        size_t offset;
        std::unique_ptr<ComponentPropertyEditor> editor;
        bool readonly = false;
        bool advanced = false; // Hide in basic mode
    };
    
    ComponentEditor(const std::string& component_name, size_t component_size);
    ~ComponentEditor() = default;
    
    /**
     * @brief Register a property for editing
     */
    template<typename T>
    void register_property(const std::string& name, size_t offset, 
                          const std::string& display_name = "",
                          const std::string& description = "");
    
    /**
     * @brief Render the complete component editor
     */
    bool render_component_editor(void* component_data, bool show_advanced = false);
    
    /**
     * @brief Validate the entire component
     */
    bool validate_component(const void* component_data) const;
    
    /**
     * @brief Get component metadata
     */
    const std::string& get_component_name() const { return component_name_; }
    size_t get_component_size() const { return component_size_; }
    const std::vector<PropertyInfo>& get_properties() const { return properties_; }
    
private:
    std::string component_name_;
    size_t component_size_;
    std::vector<PropertyInfo> properties_;
    bool show_descriptions_ = false;
};

// =============================================================================
// SYSTEM DEPENDENCY VISUALIZER
// =============================================================================

/**
 * @brief Interactive system dependency graph renderer
 */
class SystemDependencyGraph {
public:
    struct NodePosition {
        Vec2 pos;
        Vec2 size;
        bool dragging = false;
        Vec2 drag_offset;
    };
    
    struct GraphNode {
        SystemID system_id;
        std::string display_name;
        std::string category;
        Vec4 color = Vec4(0.2f, 0.6f, 1.0f, 1.0f);
        NodePosition position;
        bool selected = false;
        bool highlighted = false;
    };
    
    struct GraphEdge {
        SystemID from;
        SystemID to;
        Vec4 color = Vec4(0.7f, 0.7f, 0.7f, 1.0f);
        float thickness = 2.0f;
        bool highlighted = false;
    };
    
    SystemDependencyGraph();
    ~SystemDependencyGraph() = default;
    
    /**
     * @brief Update graph with system information
     */
    void update_graph(const SystemGraph& system_graph);
    
    /**
     * @brief Render the interactive dependency graph
     */
    void render_dependency_graph(const Rect& canvas_rect);
    
    /**
     * @brief Get selected system (if any)
     */
    std::optional<SystemID> get_selected_system() const;
    
    /**
     * @brief Set graph layout algorithm
     */
    enum class LayoutAlgorithm {
        Manual,      // User can drag nodes
        Hierarchical, // Top-down layout based on dependencies
        Circular,    // Circular arrangement
        ForceDirected // Physics-based layout
    };
    
    void set_layout_algorithm(LayoutAlgorithm algorithm);
    void apply_automatic_layout();
    
    /**
     * @brief Graph interaction
     */
    void reset_selection();
    void highlight_dependencies(const SystemID& system_id);
    void focus_on_system(const SystemID& system_id);
    
private:
    std::unordered_map<SystemID, GraphNode> nodes_;
    std::vector<GraphEdge> edges_;
    LayoutAlgorithm current_layout_ = LayoutAlgorithm::Manual;
    
    // Interaction state
    std::optional<SystemID> selected_system_;
    std::optional<SystemID> hovered_system_;
    Vec2 canvas_scroll_ = Vec2(0, 0);
    float zoom_level_ = 1.0f;
    
    // Layout helpers
    void apply_hierarchical_layout();
    void apply_circular_layout();
    void apply_force_directed_layout();
    
    // Rendering helpers
    void render_node(DrawList& draw_list, const GraphNode& node);
    void render_edge(DrawList& draw_list, const GraphEdge& edge);
    bool handle_node_interaction(const GraphNode& node, const Vec2& mouse_pos);
    
    // Utility functions
    Vec2 get_node_center(const GraphNode& node) const;
    bool point_in_node(const Vec2& point, const GraphNode& node) const;
    std::vector<SystemID> get_topological_order() const;
};

// =============================================================================
// PERFORMANCE CHARTS & METRICS
// =============================================================================

/**
 * @brief Real-time performance chart widget
 */
class PerformanceChart {
public:
    struct DataPoint {
        float timestamp;
        float value;
        Vec4 color = Vec4(0.2f, 0.8f, 0.2f, 1.0f);
    };
    
    struct ChartSeries {
        std::string name;
        std::vector<DataPoint> data;
        Vec4 color = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
        bool visible = true;
        float thickness = 2.0f;
    };
    
    PerformanceChart(const std::string& title, size_t max_data_points = 300);
    ~PerformanceChart() = default;
    
    /**
     * @brief Add or update a data series
     */
    void add_series(const std::string& name, const Vec4& color = Vec4(1, 1, 1, 1));
    void add_data_point(const std::string& series_name, float timestamp, float value);
    void clear_series(const std::string& series_name);
    
    /**
     * @brief Render the performance chart
     */
    void render_chart(const Rect& chart_rect);
    
    /**
     * @brief Chart configuration
     */
    void set_y_range(float min_y, float max_y);
    void set_auto_scale_y(bool auto_scale) { auto_scale_y_ = auto_scale; }
    void set_time_window(float window_seconds) { time_window_ = window_seconds; }
    void set_show_grid(bool show_grid) { show_grid_ = show_grid; }
    void set_show_legend(bool show_legend) { show_legend_ = show_legend; }
    
    /**
     * @brief Get chart statistics
     */
    struct ChartStats {
        float min_value = 0.0f;
        float max_value = 0.0f;
        float avg_value = 0.0f;
        float current_value = 0.0f;
        size_t data_points = 0;
    };
    
    ChartStats get_series_stats(const std::string& series_name) const;
    
private:
    std::string title_;
    size_t max_data_points_;
    std::unordered_map<std::string, ChartSeries> series_;
    
    // Chart configuration
    float min_y_ = 0.0f;
    float max_y_ = 100.0f;
    bool auto_scale_y_ = true;
    float time_window_ = 10.0f; // Show last 10 seconds
    bool show_grid_ = true;
    bool show_legend_ = true;
    
    // Rendering helpers
    void render_grid(DrawList& draw_list, const Rect& plot_rect);
    void render_series(DrawList& draw_list, const ChartSeries& series, const Rect& plot_rect);
    void render_legend(DrawList& draw_list, const Rect& chart_rect);
    Vec2 value_to_screen(float timestamp, float value, const Rect& plot_rect) const;
    void update_auto_scale();
    void cleanup_old_data();
};

/**
 * @brief Memory usage visualization widget
 */
class MemoryUsageWidget {
public:
    struct MemoryBlock {
        std::string name;
        size_t size;
        size_t used;
        Vec4 color;
        bool expanded = false;
        std::vector<MemoryBlock> sub_blocks; // For hierarchical memory
    };
    
    MemoryUsageWidget();
    ~MemoryUsageWidget() = default;
    
    /**
     * @brief Update memory information
     */
    void update_memory_info(const ecs::ECSMemoryStats& memory_stats);
    void add_memory_block(const MemoryBlock& block);
    void clear_memory_blocks();
    
    /**
     * @brief Render memory visualization
     */
    void render_memory_overview(const Rect& widget_rect);
    void render_memory_treemap(const Rect& widget_rect);
    void render_memory_timeline(const Rect& widget_rect);
    
    /**
     * @brief Configuration
     */
    void set_visualization_mode(int mode) { visualization_mode_ = mode; }
    void set_show_percentages(bool show) { show_percentages_ = show; }
    void set_show_sizes(bool show) { show_sizes_ = show; }
    
private:
    std::vector<MemoryBlock> memory_blocks_;
    PerformanceChart memory_chart_;
    
    int visualization_mode_ = 0; // 0=overview, 1=treemap, 2=timeline
    bool show_percentages_ = true;
    bool show_sizes_ = true;
    
    // Rendering helpers
    void render_memory_block(DrawList& draw_list, const MemoryBlock& block, 
                            const Rect& rect, int depth = 0);
    void render_treemap_recursive(DrawList& draw_list, const std::vector<MemoryBlock>& blocks,
                                 const Rect& rect);
    Vec4 get_usage_color(float usage_ratio) const;
    std::string format_bytes(size_t bytes) const;
};

// =============================================================================
// ARCHETYPE VISUALIZER
// =============================================================================

/**
 * @brief Interactive archetype structure visualizer
 */
class ArchetypeVisualizer {
public:
    struct ArchetypeNode {
        ecs::ComponentSignature signature;
        std::string display_name;
        size_t entity_count;
        size_t memory_usage;
        Vec2 position;
        Vec2 size;
        bool selected = false;
        Vec4 color;
        
        // Component breakdown
        std::vector<std::pair<std::string, size_t>> component_sizes;
    };
    
    struct ArchetypeTransition {
        ecs::ComponentSignature from_signature;
        ecs::ComponentSignature to_signature;
        size_t transition_count;
        Vec4 color = Vec4(0.5f, 0.5f, 0.5f, 0.8f);
    };
    
    ArchetypeVisualizer();
    ~ArchetypeVisualizer() = default;
    
    /**
     * @brief Update archetype information
     */
    void update_archetypes(const std::vector<ECSInspector::ArchetypeInfo>& archetypes);
    void add_archetype_transition(const ecs::ComponentSignature& from, 
                                 const ecs::ComponentSignature& to);
    
    /**
     * @brief Render archetype visualization
     */
    void render_archetype_overview(const Rect& canvas_rect);
    void render_archetype_detail(const ecs::ComponentSignature& signature, const Rect& detail_rect);
    void render_component_matrix(const Rect& matrix_rect);
    
    /**
     * @brief Interaction
     */
    std::optional<ecs::ComponentSignature> get_selected_archetype() const;
    void select_archetype(const ecs::ComponentSignature& signature);
    void clear_selection();
    
    /**
     * @brief Configuration
     */
    void set_show_transitions(bool show) { show_transitions_ = show; }
    void set_show_empty_archetypes(bool show) { show_empty_archetypes_ = show; }
    void set_size_by_memory(bool size_by_memory) { size_by_memory_ = size_by_memory; }
    
private:
    std::vector<ArchetypeNode> archetype_nodes_;
    std::vector<ArchetypeTransition> transitions_;
    std::optional<ecs::ComponentSignature> selected_archetype_;
    
    // Configuration
    bool show_transitions_ = true;
    bool show_empty_archetypes_ = false;
    bool size_by_memory_ = true;
    
    // Layout and interaction
    Vec2 canvas_scroll_ = Vec2(0, 0);
    float zoom_level_ = 1.0f;
    
    // Rendering helpers
    void render_archetype_node(DrawList& draw_list, const ArchetypeNode& node);
    void render_archetype_transition(DrawList& draw_list, const ArchetypeTransition& transition);
    void layout_archetypes_automatic();
    bool handle_archetype_interaction(const ArchetypeNode& node, const Vec2& mouse_pos);
    
    // Utility functions
    Vec4 get_archetype_color(const ecs::ComponentSignature& signature) const;
    std::string get_archetype_tooltip(const ArchetypeNode& node) const;
    float calculate_node_size(const ArchetypeNode& node) const;
};

// =============================================================================
// QUERY BUILDER INTERFACE
// =============================================================================

/**
 * @brief Visual ECS query builder
 */
class QueryBuilderWidget {
public:
    struct QueryCondition {
        enum Type {
            RequireComponent,
            ExcludeComponent,
            EntityHasTag,
            EntityInGroup
        };
        
        Type type;
        std::string component_name;
        std::string parameter;
        bool active = true;
    };
    
    struct QueryResult {
        std::vector<EntityID> entities;
        std::chrono::steady_clock::time_point execution_time;
        std::chrono::duration<float, std::milli> execution_duration;
        size_t result_count;
    };
    
    QueryBuilderWidget();
    ~QueryBuilderWidget() = default;
    
    /**
     * @brief Render query builder interface
     */
    void render_query_builder(const Rect& builder_rect);
    void render_query_results(const Rect& results_rect);
    void render_saved_queries(const Rect& saved_rect);
    
    /**
     * @brief Query management
     */
    void add_condition(const QueryCondition& condition);
    void remove_condition(size_t index);
    void clear_conditions();
    
    void execute_query(ECSInspector* inspector);
    void save_current_query(const std::string& name);
    void load_saved_query(const std::string& name);
    void delete_saved_query(const std::string& name);
    
    /**
     * @brief Get query results
     */
    const QueryResult& get_last_result() const { return last_result_; }
    bool has_active_query() const { return !conditions_.empty(); }
    
private:
    std::vector<QueryCondition> conditions_;
    std::unordered_map<std::string, std::vector<QueryCondition>> saved_queries_;
    QueryResult last_result_;
    
    // UI state
    std::string new_query_name_;
    std::string selected_component_type_;
    bool show_query_performance_ = true;
    
    // Rendering helpers
    void render_condition_editor(QueryCondition& condition, size_t index);
    void render_component_selector();
    void render_query_performance_stats();
    void render_result_entity_list();
    
    // Query execution
    ECSInspector::QuerySpec build_query_spec() const;
    std::string generate_query_description() const;
};

// =============================================================================
// TEMPLATE SPECIALIZATIONS
// =============================================================================

// Basic type editors - specialized for common types
template<>
bool BasicPropertyEditor<int>::render_property(const std::string& property_name, void* data, bool& changed);

template<>
bool BasicPropertyEditor<float>::render_property(const std::string& property_name, void* data, bool& changed);

template<>
bool BasicPropertyEditor<bool>::render_property(const std::string& property_name, void* data, bool& changed);

template<>
bool BasicPropertyEditor<std::string>::render_property(const std::string& property_name, void* data, bool& changed);

// Vector types (if available)
#ifdef ECSCOPE_HAS_MATH_TYPES
template<>
bool BasicPropertyEditor<Vec2>::render_property(const std::string& property_name, void* data, bool& changed);

template<>
bool BasicPropertyEditor<Vec3>::render_property(const std::string& property_name, void* data, bool& changed);

template<>
bool BasicPropertyEditor<Vec4>::render_property(const std::string& property_name, void* data, bool& changed);
#endif

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

/**
 * @brief Create default component editors for common types
 */
std::unique_ptr<ComponentEditor> create_transform_component_editor();
std::unique_ptr<ComponentEditor> create_render_component_editor();
std::unique_ptr<ComponentEditor> create_physics_component_editor();

/**
 * @brief Register all built-in component editors with an inspector
 */
void register_builtin_component_editors(ECSInspector* inspector);

/**
 * @brief Color utilities for visualizations
 */
Vec4 get_category_color(const std::string& category);
Vec4 get_performance_color(float performance_ratio); // 0.0 = bad (red), 1.0 = good (green)
Vec4 get_memory_usage_color(float usage_ratio);

} // namespace ecscope::gui