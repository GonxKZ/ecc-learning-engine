/**
 * @file ecs_inspector.hpp
 * @brief Comprehensive ECS Inspector for ECScope Engine
 * 
 * Professional-grade ECS debugging and development tool with advanced visualization,
 * real-time editing, system monitoring, and performance profiling capabilities.
 * 
 * Key Features:
 * - Hierarchical entity management with search and filtering
 * - Real-time component visualization and editing
 * - System execution monitoring and profiling
 * - Archetype analysis and memory tracking
 * - Query builder for testing ECS queries
 * - Component change history and rollback
 * - Thread-safe access to ECS data
 * - Integration with main dashboard
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <optional>
#include <mutex>
#include <atomic>
#include <queue>
#include <typeindex>
#include <chrono>

#include "../registry.hpp"
#include "../entity.hpp"
#include "../component.hpp"
#include "../signature.hpp"
#include "../archetype.hpp"
#include "core.hpp"
#include "dashboard.hpp"

namespace ecscope::gui {

// =============================================================================
// FORWARD DECLARATIONS & TYPES
// =============================================================================

class ECSInspector;
class EntityTreeView;
class ComponentInspector;
class SystemMonitor;
class ArchetypeAnalyzer;
class QueryBuilder;

using EntityID = ecs::Entity;
using ComponentTypeInfo = std::type_index;
using SystemID = std::string;

// =============================================================================
// INSPECTOR CONFIGURATION
// =============================================================================

/**
 * @brief Configuration for ECS inspector behavior
 */
struct InspectorConfig {
    // Visual settings
    bool show_entity_hierarchy = true;
    bool show_component_details = true;
    bool show_system_profiling = true;
    bool show_archetype_analysis = true;
    bool show_memory_tracking = true;
    
    // Update frequencies (in milliseconds)
    float entity_refresh_rate = 16.0f;     // 60 FPS
    float component_refresh_rate = 33.0f;  // 30 FPS
    float system_refresh_rate = 100.0f;    // 10 FPS
    float memory_refresh_rate = 500.0f;    // 2 FPS
    
    // Performance limits
    size_t max_entities_displayed = 10000;
    size_t max_history_entries = 1000;
    float max_update_time_ms = 5.0f;
    
    // Features
    bool enable_undo_redo = true;
    bool enable_component_validation = true;
    bool enable_realtime_updates = true;
    bool enable_advanced_filtering = true;
    bool enable_batch_operations = true;
    
    static InspectorConfig create_performance_focused() {
        InspectorConfig config;
        config.entity_refresh_rate = 33.0f;  // 30 FPS
        config.component_refresh_rate = 66.0f; // 15 FPS
        config.system_refresh_rate = 200.0f;   // 5 FPS
        config.memory_refresh_rate = 1000.0f;  // 1 FPS
        config.max_entities_displayed = 5000;
        config.enable_realtime_updates = false;
        return config;
    }
    
    static InspectorConfig create_debugging_focused() {
        InspectorConfig config;
        config.entity_refresh_rate = 8.0f;    // 120 FPS
        config.component_refresh_rate = 16.0f; // 60 FPS
        config.max_history_entries = 5000;
        config.enable_component_validation = true;
        config.enable_undo_redo = true;
        return config;
    }
};

// =============================================================================
// ENTITY MANAGEMENT STRUCTURES
// =============================================================================

/**
 * @brief Extended entity information for inspector
 */
struct EntityInfo {
    EntityID entity;
    std::string name;
    std::string tag;
    std::vector<std::string> groups;
    bool enabled = true;
    bool selected = false;
    std::chrono::steady_clock::time_point created_time;
    std::chrono::steady_clock::time_point last_modified;
    
    // Hierarchy support
    std::optional<EntityID> parent;
    std::vector<EntityID> children;
    
    // Component tracking
    std::vector<ComponentTypeInfo> components;
    std::unordered_map<ComponentTypeInfo, std::chrono::steady_clock::time_point> component_timestamps;
    
    EntityInfo(EntityID ent) : entity(ent), created_time(std::chrono::steady_clock::now()) {}
};

/**
 * @brief Filter criteria for entity search
 */
struct EntityFilter {
    std::string name_pattern;
    std::string tag_pattern;
    std::vector<ComponentTypeInfo> required_components;
    std::vector<ComponentTypeInfo> excluded_components;
    bool only_enabled = false;
    bool only_selected = false;
    
    bool matches(const EntityInfo& entity) const;
    std::string to_string() const;
};

/**
 * @brief Entity selection state
 */
struct SelectionState {
    std::unordered_set<EntityID> selected_entities;
    std::optional<EntityID> primary_selection;
    std::chrono::steady_clock::time_point selection_time;
    
    void clear() { selected_entities.clear(); primary_selection.reset(); }
    void select(EntityID entity, bool multi = false);
    void deselect(EntityID entity);
    bool is_selected(EntityID entity) const;
    size_t count() const { return selected_entities.size(); }
};

// =============================================================================
// COMPONENT SYSTEM STRUCTURES
// =============================================================================

/**
 * @brief Component metadata for inspector
 */
struct ComponentMetadata {
    ComponentTypeInfo type;
    std::string name;
    std::string category;
    size_t size;
    bool is_editable = true;
    bool is_serializable = true;
    std::vector<std::string> property_names;
    std::function<void(void*, const std::string&)> serialize_func;
    std::function<void(void*, const std::string&)> deserialize_func;
    std::function<bool(const void*)> validate_func;
    std::function<void(void*, DrawList&, const Rect&)> render_func;
    
    ComponentMetadata(ComponentTypeInfo t, const std::string& n) : type(t), name(n), size(0) {}
};

/**
 * @brief Component change record for history/undo
 */
struct ComponentChange {
    EntityID entity;
    ComponentTypeInfo component_type;
    std::string previous_state;
    std::string new_state;
    std::chrono::steady_clock::time_point timestamp;
    std::string description;
    
    ComponentChange(EntityID ent, ComponentTypeInfo type, const std::string& desc)
        : entity(ent), component_type(type), description(desc), 
          timestamp(std::chrono::steady_clock::now()) {}
};

/**
 * @brief Component template for quick creation
 */
struct ComponentTemplate {
    std::string name;
    std::string description;
    ComponentTypeInfo component_type;
    std::string serialized_data;
    std::vector<std::string> tags;
    
    ComponentTemplate(const std::string& n, ComponentTypeInfo type)
        : name(n), component_type(type) {}
};

// =============================================================================
// SYSTEM MONITORING STRUCTURES
// =============================================================================

/**
 * @brief System execution statistics
 */
struct SystemStats {
    SystemID system_id;
    std::string name;
    std::string category;
    
    // Timing metrics
    std::chrono::duration<float, std::milli> last_execution_time{0};
    std::chrono::duration<float, std::milli> average_execution_time{0};
    std::chrono::duration<float, std::milli> max_execution_time{0};
    std::chrono::duration<float, std::milli> min_execution_time{std::chrono::duration<float, std::milli>::max()};
    
    // Execution tracking
    uint64_t execution_count = 0;
    uint64_t entities_processed = 0;
    bool is_enabled = true;
    bool is_running = false;
    
    // History for graphing
    std::queue<float> execution_history; // Last N execution times (ms)
    static constexpr size_t MAX_HISTORY = 300;
    
    // Dependencies
    std::vector<SystemID> dependencies;
    std::vector<SystemID> dependents;
    
    void record_execution(std::chrono::duration<float, std::milli> duration, uint64_t processed);
    void reset_stats();
};

/**
 * @brief System execution order and dependency information
 */
struct SystemGraph {
    std::unordered_map<SystemID, SystemStats> systems;
    std::vector<std::vector<SystemID>> execution_order; // Per-phase execution order
    std::unordered_map<SystemID, size_t> system_phases;
    
    void add_system(const SystemStats& system);
    void remove_system(const SystemID& system_id);
    void add_dependency(const SystemID& system, const SystemID& dependency);
    void remove_dependency(const SystemID& system, const SystemID& dependency);
    std::vector<SystemID> get_topological_order() const;
    bool has_cycles() const;
};

// =============================================================================
// MAIN ECS INSPECTOR CLASS
// =============================================================================

/**
 * @brief Comprehensive ECS Inspector Implementation
 * 
 * Professional debugging and development tool for ECScope's ECS system.
 * Provides real-time visualization, editing, and monitoring capabilities
 * with thread-safe access and high-performance updates.
 */
class ECSInspector {
public:
    explicit ECSInspector(ecs::Registry* registry, const InspectorConfig& config = InspectorConfig{});
    ~ECSInspector();

    // =============================================================================
    // LIFECYCLE & INTEGRATION
    // =============================================================================
    
    /**
     * @brief Initialize the inspector
     */
    bool initialize();
    
    /**
     * @brief Shutdown and cleanup resources
     */
    void shutdown();
    
    /**
     * @brief Update inspector state (call once per frame)
     */
    void update(float delta_time);
    
    /**
     * @brief Render the complete inspector UI
     */
    void render();
    
    /**
     * @brief Integrate with dashboard as a panel
     */
    void render_as_dashboard_panel(const std::string& panel_name = "ECS Inspector");
    
    /**
     * @brief Register with main dashboard
     */
    void register_with_dashboard(Dashboard* dashboard);

    // =============================================================================
    // ENTITY MANAGEMENT
    // =============================================================================
    
    /**
     * @brief Get entity information
     */
    const EntityInfo* get_entity_info(EntityID entity) const;
    
    /**
     * @brief Create new entity with optional components
     */
    EntityID create_entity(const std::string& name = "", const std::string& tag = "");
    
    /**
     * @brief Destroy selected entities
     */
    bool destroy_entities(const std::vector<EntityID>& entities);
    
    /**
     * @brief Clone entity with all components
     */
    EntityID clone_entity(EntityID source, const std::string& name = "");
    
    /**
     * @brief Set entity hierarchy (parent/child relationships)
     */
    bool set_parent(EntityID child, EntityID parent);
    bool remove_parent(EntityID child);
    
    /**
     * @brief Entity search and filtering
     */
    std::vector<EntityID> search_entities(const EntityFilter& filter) const;
    std::vector<EntityID> get_all_entities() const;
    std::vector<EntityID> get_children(EntityID parent) const;
    
    /**
     * @brief Entity grouping and tagging
     */
    void set_entity_name(EntityID entity, const std::string& name);
    void set_entity_tag(EntityID entity, const std::string& tag);
    void add_entity_to_group(EntityID entity, const std::string& group);
    void remove_entity_from_group(EntityID entity, const std::string& group);

    // =============================================================================
    // SELECTION SYSTEM
    // =============================================================================
    
    /**
     * @brief Entity selection management
     */
    void select_entity(EntityID entity, bool multi_select = false);
    void deselect_entity(EntityID entity);
    void clear_selection();
    void select_all_filtered();
    
    /**
     * @brief Selection queries
     */
    const SelectionState& get_selection() const { return selection_state_; }
    bool is_entity_selected(EntityID entity) const;
    std::vector<EntityID> get_selected_entities() const;

    // =============================================================================
    // COMPONENT SYSTEM
    // =============================================================================
    
    /**
     * @brief Component type registration
     */
    template<typename T>
    void register_component_type(const std::string& name, const std::string& category = "General");
    
    /**
     * @brief Component metadata management
     */
    void register_component_metadata(const ComponentMetadata& metadata);
    const ComponentMetadata* get_component_metadata(ComponentTypeInfo type) const;
    std::vector<ComponentMetadata> get_all_component_metadata() const;
    
    /**
     * @brief Component manipulation
     */
    template<typename T>
    bool add_component_to_entity(EntityID entity, const T& component);
    
    template<typename T>
    bool remove_component_from_entity(EntityID entity);
    
    template<typename T>
    T* get_component_from_entity(EntityID entity);
    
    template<typename T>
    bool has_component(EntityID entity) const;
    
    /**
     * @brief Batch operations
     */
    bool add_component_to_entities(const std::vector<EntityID>& entities, ComponentTypeInfo type, const void* component_data);
    bool remove_component_from_entities(const std::vector<EntityID>& entities, ComponentTypeInfo type);
    
    /**
     * @brief Component templates
     */
    void register_component_template(const ComponentTemplate& template_data);
    void apply_component_template(EntityID entity, const std::string& template_name);
    std::vector<ComponentTemplate> get_component_templates() const;

    // =============================================================================
    // CHANGE HISTORY & UNDO/REDO
    // =============================================================================
    
    /**
     * @brief History management
     */
    void record_change(const ComponentChange& change);
    bool undo_last_change();
    bool redo_change();
    void clear_history();
    
    /**
     * @brief History queries
     */
    const std::vector<ComponentChange>& get_change_history() const { return change_history_; }
    std::vector<ComponentChange> get_entity_history(EntityID entity) const;
    bool can_undo() const;
    bool can_redo() const;

    // =============================================================================
    // SYSTEM MONITORING
    // =============================================================================
    
    /**
     * @brief System registration and monitoring
     */
    void register_system(const SystemStats& system);
    void update_system_stats(const SystemID& system_id, std::chrono::duration<float, std::milli> execution_time, uint64_t entities_processed);
    void enable_system(const SystemID& system_id, bool enabled);
    
    /**
     * @brief System queries
     */
    const SystemGraph& get_system_graph() const { return system_graph_; }
    const SystemStats* get_system_stats(const SystemID& system_id) const;
    std::vector<SystemID> get_all_systems() const;
    std::vector<SystemID> get_systems_by_category(const std::string& category) const;

    // =============================================================================
    // ARCHETYPE ANALYSIS
    // =============================================================================
    
    /**
     * @brief Archetype information
     */
    struct ArchetypeInfo {
        ecs::ComponentSignature signature;
        size_t entity_count;
        size_t memory_usage;
        std::vector<ComponentTypeInfo> components;
        std::chrono::steady_clock::time_point created_time;
        std::chrono::steady_clock::time_point last_modified;
        
        // Performance metrics
        size_t total_entities_created = 0;
        size_t total_entities_destroyed = 0;
        float average_lifetime_ms = 0.0f;
        
        std::string to_string() const;
    };
    
    std::vector<ArchetypeInfo> get_archetype_analysis() const;
    ArchetypeInfo get_entity_archetype(EntityID entity) const;
    std::vector<EntityID> get_entities_in_archetype(const ecs::ComponentSignature& signature) const;

    // =============================================================================
    // QUERY BUILDER & TESTING
    // =============================================================================
    
    /**
     * @brief Query builder for testing ECS queries
     */
    struct QuerySpec {
        std::vector<ComponentTypeInfo> required_components;
        std::vector<ComponentTypeInfo> excluded_components;
        std::string name;
        bool cache_results = false;
        
        std::string to_string() const;
    };
    
    void register_query(const QuerySpec& query);
    std::vector<EntityID> execute_query(const QuerySpec& query) const;
    std::vector<EntityID> execute_named_query(const std::string& name) const;
    std::vector<QuerySpec> get_saved_queries() const;

    // =============================================================================
    // CONFIGURATION & SETTINGS
    // =============================================================================
    
    /**
     * @brief Configuration management
     */
    void set_config(const InspectorConfig& config);
    const InspectorConfig& get_config() const { return config_; }
    
    /**
     * @brief Persistence
     */
    bool save_inspector_state(const std::string& filepath) const;
    bool load_inspector_state(const std::string& filepath);

    // =============================================================================
    // STATISTICS & METRICS
    // =============================================================================
    
    /**
     * @brief Inspector performance metrics
     */
    struct InspectorMetrics {
        float last_update_time_ms = 0.0f;
        float last_render_time_ms = 0.0f;
        size_t entities_tracked = 0;
        size_t components_tracked = 0;
        size_t systems_tracked = 0;
        size_t memory_usage_bytes = 0;
        
        std::chrono::steady_clock::time_point last_measurement;
    };
    
    const InspectorMetrics& get_metrics() const { return metrics_; }

private:
    // =============================================================================
    // PRIVATE RENDERING METHODS
    // =============================================================================
    
    void render_main_inspector_window();
    void render_entity_hierarchy_panel();
    void render_component_inspector_panel();
    void render_system_monitor_panel();
    void render_archetype_analyzer_panel();
    void render_query_builder_panel();
    void render_history_panel();
    void render_templates_panel();
    void render_settings_panel();
    void render_metrics_panel();
    
    // Entity rendering helpers
    void render_entity_tree_node(EntityID entity, const EntityInfo& info);
    void render_entity_context_menu(EntityID entity);
    void render_entity_details(EntityID entity);
    void render_multi_entity_operations();
    
    // Component rendering helpers
    void render_component_list(EntityID entity);
    void render_component_editor(EntityID entity, ComponentTypeInfo type);
    void render_add_component_dialog(EntityID entity);
    void render_component_templates_list();
    
    // System monitoring helpers
    void render_system_list();
    void render_system_details(const SystemID& system_id);
    void render_system_dependency_graph();
    void render_system_performance_charts();
    
    // Query builder helpers
    void render_query_construction_ui();
    void render_query_results(const QuerySpec& query, const std::vector<EntityID>& results);
    void render_saved_queries_list();

    // =============================================================================
    // PRIVATE UPDATE METHODS
    // =============================================================================
    
    void update_entity_tracking();
    void update_component_tracking();
    void update_system_monitoring();
    void refresh_archetype_analysis();
    void cleanup_stale_data();
    
    // Thread-safe data access
    void lock_registry_data();
    void unlock_registry_data();

    // =============================================================================
    // MEMBER VARIABLES
    // =============================================================================
    
    // Core references
    ecs::Registry* registry_;
    Dashboard* dashboard_;
    InspectorConfig config_;
    
    // Entity tracking
    std::unordered_map<EntityID, std::unique_ptr<EntityInfo>> entity_info_;
    SelectionState selection_state_;
    mutable std::mutex entity_mutex_;
    
    // Component system
    std::unordered_map<ComponentTypeInfo, ComponentMetadata> component_metadata_;
    std::vector<ComponentTemplate> component_templates_;
    std::vector<ComponentChange> change_history_;
    size_t history_position_ = 0;
    mutable std::mutex component_mutex_;
    
    // System monitoring
    SystemGraph system_graph_;
    mutable std::mutex system_mutex_;
    
    // Query system
    std::vector<QuerySpec> saved_queries_;
    std::unordered_map<std::string, std::vector<EntityID>> query_cache_;
    
    // Update tracking
    std::chrono::steady_clock::time_point last_entity_update_;
    std::chrono::steady_clock::time_point last_component_update_;
    std::chrono::steady_clock::time_point last_system_update_;
    std::chrono::steady_clock::time_point last_memory_update_;
    
    // UI state
    std::string current_search_filter_;
    EntityFilter active_filter_;
    bool show_entity_hierarchy_ = true;
    bool show_component_inspector_ = true;
    bool show_system_monitor_ = true;
    bool show_archetype_analyzer_ = true;
    bool show_query_builder_ = false;
    bool show_history_panel_ = false;
    bool show_templates_panel_ = false;
    bool show_settings_panel_ = false;
    bool show_metrics_panel_ = false;
    
    // Performance tracking
    InspectorMetrics metrics_;
    std::atomic<bool> initialized_{false};
    std::atomic<bool> shutdown_requested_{false};
    
    // Threading
    mutable std::mutex render_mutex_;
    std::atomic<uint64_t> frame_counter_{0};
};

// =============================================================================
// TEMPLATE IMPLEMENTATIONS
// =============================================================================

template<typename T>
inline void ECSInspector::register_component_type(const std::string& name, const std::string& category) {
    ComponentMetadata metadata(std::type_index(typeid(T)), name);
    metadata.category = category;
    metadata.size = sizeof(T);
    
    // Default validation (always valid)
    metadata.validate_func = [](const void*) -> bool { return true; };
    
    register_component_metadata(metadata);
}

template<typename T>
inline bool ECSInspector::add_component_to_entity(EntityID entity, const T& component) {
    if (!registry_ || !registry_->is_valid(entity)) {
        return false;
    }
    
    bool success = registry_->add_component(entity, component);
    
    if (success && config_.enable_undo_redo) {
        ComponentChange change(entity, std::type_index(typeid(T)), "Add Component");
        // Serialize component state for undo
        record_change(change);
    }
    
    return success;
}

template<typename T>
inline bool ECSInspector::remove_component_from_entity(EntityID entity) {
    if (!registry_ || !registry_->is_valid(entity)) {
        return false;
    }
    
    // Record current state for undo before removing
    if (config_.enable_undo_redo) {
        T* current_component = registry_->get_component<T>(entity);
        if (current_component) {
            ComponentChange change(entity, std::type_index(typeid(T)), "Remove Component");
            // Serialize current state for potential undo
            record_change(change);
        }
    }
    
    return registry_->remove_component<T>(entity);
}

template<typename T>
inline T* ECSInspector::get_component_from_entity(EntityID entity) {
    if (!registry_ || !registry_->is_valid(entity)) {
        return nullptr;
    }
    
    return registry_->get_component<T>(entity);
}

template<typename T>
inline bool ECSInspector::has_component(EntityID entity) const {
    if (!registry_ || !registry_->is_valid(entity)) {
        return false;
    }
    
    return registry_->has_component<T>(entity);
}

} // namespace ecscope::gui