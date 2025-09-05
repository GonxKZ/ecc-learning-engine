#pragma once

/**
 * @file panel_visual_debugger.hpp
 * @brief Visual ECS Debugger Panel - Interactive visualization of ECS operations
 * 
 * This panel provides comprehensive visual debugging capabilities for ECS operations,
 * enabling students to see and understand how entities, components, and systems
 * interact in real-time with step-by-step execution and timeline scrubbing.
 * 
 * Features:
 * - Real-time visualization of ECS operations
 * - Interactive breakpoint system with conditional breaks
 * - Timeline scrubbing and playback control
 * - Entity lifecycle visualization with state tracking
 * - Component data flow analysis
 * - System execution order and dependency visualization
 * - Memory layout and archetype visualization
 * - Interactive entity inspector with component editing
 * 
 * Educational Design:
 * - Visual representation of abstract ECS concepts
 * - Step-by-step execution for understanding
 * - Interactive exploration of system behavior
 * - Real-time feedback on performance implications
 * - Gamified debugging experience
 * 
 * @author ECScope Educational Framework
 * @date 2024
 */

#include "../overlay.hpp"
#include "ecs/registry.hpp"
#include "ecs/entity.hpp"
#include "core/types.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <chrono>
#include <functional>
#include <queue>

namespace ecscope::ui {

// Forward declarations
class VisualDebuggerPanel;
class ECSOperationVisualizer;
class DebugTimeline;
class EntityInspector;

/**
 * @brief Types of ECS operations that can be visualized
 */
enum class ECSOperationType : u8 {
    EntityCreated,      // New entity creation
    EntityDestroyed,    // Entity destruction
    ComponentAdded,     // Component addition to entity
    ComponentRemoved,   // Component removal from entity
    ComponentModified,  // Component data modification
    SystemExecuted,     // System processing entities
    ArchetypeCreated,   // New archetype formation
    ArchetypeChanged,   // Entity moved between archetypes
    QueryExecuted,      // Component query execution
    MemoryAllocated,    // Memory allocation event
    MemoryDeallocated   // Memory deallocation event
};

/**
 * @brief Debugger execution state
 */
enum class DebuggerState : u8 {
    Running,        // Normal execution
    Paused,         // Execution paused
    Stepping,       // Single-step execution
    Breakpoint,     // Stopped at breakpoint
    Rewinding,      // Scrubbing backwards in timeline
    FastForward     // Fast-forward playback
};

/**
 * @brief Recorded ECS operation for debugging
 */
struct ECSOperation {
    u64 operation_id;
    ECSOperationType type;
    std::chrono::steady_clock::time_point timestamp;
    f64 frame_time;
    u32 frame_number;
    
    // Operation-specific data
    ecs::Entity target_entity{ecs::NULL_ENTITY};
    std::string component_type_name;
    std::string system_name;
    std::string archetype_signature;
    
    // Additional context
    std::unordered_map<std::string, std::string> metadata;
    std::vector<u8> component_data_snapshot; // Serialized component data
    
    // Performance metrics
    f64 operation_duration{0.0}; // microseconds
    usize memory_delta{0};        // bytes allocated/deallocated
    u32 entities_affected{0};
    
    // Visual representation
    f32 visual_x{0.0f};
    f32 visual_y{0.0f};
    f32 visual_intensity{1.0f};
    bool is_highlighted{false};
    
    ECSOperation() : operation_id(0), type(ECSOperationType::EntityCreated),
                    timestamp(std::chrono::steady_clock::now()), frame_time(0.0), frame_number(0) {}
                    
    ECSOperation(ECSOperationType op_type, ecs::Entity entity = ecs::NULL_ENTITY)
        : operation_id(0), type(op_type), target_entity(entity),
          timestamp(std::chrono::steady_clock::now()), frame_time(0.0), frame_number(0) {}
};

/**
 * @brief Breakpoint configuration
 */
struct Breakpoint {
    u64 breakpoint_id;
    bool enabled{true};
    bool hit{false};
    u32 hit_count{0};
    
    // Trigger conditions
    ECSOperationType operation_type{ECSOperationType::EntityCreated};
    ecs::Entity specific_entity{ecs::NULL_ENTITY};
    std::string component_type_filter;
    std::string system_name_filter;
    
    // Conditional breakpoints
    std::string condition_expression; // e.g., "entity_count > 100"
    std::function<bool(const ECSOperation&)> condition_evaluator;
    
    // Breakpoint actions
    bool pause_execution{true};
    bool log_operation{false};
    bool highlight_entity{true};
    std::string custom_message;
    
    // Hit count conditions
    enum class HitCondition : u8 {
        Always,         // Break every time
        HitCountEquals, // Break when hit count equals value
        HitCountMultiple, // Break when hit count is multiple of value
        HitCountGreater   // Break when hit count is greater than value
    } hit_condition{HitCondition::Always};
    
    u32 hit_condition_value{1};
    
    Breakpoint() : breakpoint_id(0) {}
};

/**
 * @brief Timeline event for scrubbing and playback
 */
struct TimelineEvent {
    f64 timestamp;
    u32 frame_number;
    std::vector<u64> operation_ids;
    
    // Timeline visualization
    f32 visual_position{0.0f}; // 0.0 to 1.0 along timeline
    f32 event_intensity{1.0f}; // Visual intensity based on operation count
    bool is_key_frame{false};  // Important frames for quick navigation
    
    TimelineEvent(f64 time, u32 frame) : timestamp(time), frame_number(frame) {}
};

/**
 * @brief Entity state snapshot for timeline scrubbing
 */
struct EntitySnapshot {
    ecs::Entity entity;
    std::unordered_map<std::string, std::vector<u8>> component_data;
    std::string archetype_signature;
    bool is_alive{true};
    f64 creation_time{0.0};
    
    // Visual state
    f32 visual_x{0.0f};
    f32 visual_y{0.0f};
    f32 visual_scale{1.0f};
    u32 visual_color{0xFFFFFFFF};
    
    EntitySnapshot() = default;
    EntitySnapshot(ecs::Entity ent) : entity(ent) {}
};

/**
 * @brief Visual ECS Debugger Panel
 */
class VisualDebuggerPanel : public Panel {
private:
    // Core ECS integration
    std::shared_ptr<ecs::Registry> registry_;
    
    // Debugger state
    DebuggerState current_state_{DebuggerState::Running};
    bool recording_enabled_{true};
    bool visual_debugging_enabled_{true};
    
    // Operation recording
    std::vector<ECSOperation> recorded_operations_;
    std::unordered_map<u64, usize> operation_index_map_; // operation_id -> index
    u64 next_operation_id_{1};
    usize max_recorded_operations_{10000}; // Ring buffer size
    usize recording_head_{0};
    
    // Timeline system
    std::vector<TimelineEvent> timeline_events_;
    f64 timeline_start_time_{0.0};
    f64 timeline_duration_{300.0}; // 5 minutes default
    f32 timeline_position_{1.0f}; // Current position (0.0 to 1.0)
    f32 timeline_zoom_{1.0f};     // Zoom level
    f32 timeline_scroll_{0.0f};   // Scroll offset
    bool timeline_playing_{false};
    f32 timeline_playback_speed_{1.0f};
    
    // Breakpoint system
    std::unordered_map<u64, Breakpoint> breakpoints_;
    u64 next_breakpoint_id_{1};
    bool breakpoints_enabled_{true};
    ECSOperation current_breakpoint_operation_;
    
    // Entity tracking
    std::unordered_map<ecs::Entity, std::vector<EntitySnapshot>> entity_history_;
    std::unordered_set<ecs::Entity> tracked_entities_;
    std::unordered_set<ecs::Entity> highlighted_entities_;
    ecs::Entity selected_entity_{ecs::NULL_ENTITY};
    
    // System execution tracking
    struct SystemExecution {
        std::string system_name;
        f64 start_time;
        f64 end_time;
        std::vector<ecs::Entity> processed_entities;
        std::unordered_map<std::string, usize> component_accesses;
        usize memory_allocations{0};
        f64 cpu_time{0.0};
        
        SystemExecution() : start_time(0.0), end_time(0.0) {}
    };
    
    std::vector<SystemExecution> system_executions_;
    std::unordered_map<std::string, f32> system_performance_history_;
    
    // Visualization settings
    struct VisualizationSettings {
        // Entity visualization
        bool show_entity_ids{true};
        bool show_component_types{true};
        bool show_archetype_connections{false};
        bool animate_operations{true};
        f32 entity_size{20.0f};
        f32 animation_speed{1.0f};
        
        // Timeline visualization
        bool show_frame_markers{true};
        bool show_system_execution_bars{true};
        bool show_memory_events{false};
        f32 timeline_height{100.0f};
        
        // Performance visualization
        bool show_performance_overlay{false};
        bool show_memory_usage{true};
        bool show_frame_time_graph{true};
        f32 performance_graph_height{80.0f};
        
        // Colors and styling
        u32 entity_color{0xFF4CAF50};        // Green
        u32 component_color{0xFF2196F3};     // Blue
        u32 system_color{0xFFFF9800};        // Orange
        u32 breakpoint_color{0xFFF44336};    // Red
        u32 highlight_color{0xFFFFEB3B};     // Yellow
        
        // Interactive elements
        bool enable_entity_selection{true};
        bool enable_component_editing{true};
        bool enable_real_time_updates{true};
        f32 interaction_sensitivity{1.0f};
    };
    
    VisualizationSettings viz_settings_;
    
    // UI state
    enum class DebuggerPanel : u8 {
        MainView,           // Main debugging visualization
        Timeline,           // Timeline scrubbing interface
        Breakpoints,        // Breakpoint management
        EntityInspector,    // Entity/component inspector
        SystemProfiler,     // System performance profiler
        MemoryAnalyzer,     // Memory usage analysis
        Settings            // Debugger settings
    };
    
    DebuggerPanel active_panel_{DebuggerPanel::MainView};
    bool show_side_panel_{true};
    f32 side_panel_width_{300.0f};
    
    // Frame capture and snapshots
    struct FrameSnapshot {
        u32 frame_number;
        f64 frame_time;
        std::unordered_map<ecs::Entity, EntitySnapshot> entity_states;
        std::vector<SystemExecution> system_executions;
        usize total_entities{0};
        usize total_components{0};
        usize memory_usage{0};
        
        FrameSnapshot(u32 frame, f64 time) : frame_number(frame), frame_time(time) {}
    };
    
    std::vector<FrameSnapshot> frame_snapshots_;
    usize max_frame_snapshots_{300}; // 5 minutes at 60fps
    usize snapshot_head_{0};
    
    // Performance metrics
    struct PerformanceMetrics {
        f64 frame_time_ms{16.67}; // Target 60fps
        f64 system_execution_time_ms{0.0};
        f64 ecs_overhead_time_ms{0.0};
        usize entities_processed_per_frame{0};
        usize components_accessed_per_frame{0};
        usize memory_allocations_per_frame{0};
        f32 cpu_usage_percent{0.0f};
        f32 memory_usage_mb{0.0f};
        
        // History for graphing
        std::vector<f32> frame_time_history;
        std::vector<f32> entity_count_history;
        std::vector<f32> memory_usage_history;
        
        static constexpr usize HISTORY_SIZE = 300; // 5 seconds at 60fps
    };
    
    PerformanceMetrics performance_;
    
    // Event handling
    std::queue<std::function<void()>> pending_debug_actions_;
    std::function<void(const ECSOperation&)> operation_callback_;
    std::function<void(const Breakpoint&)> breakpoint_hit_callback_;
    
    // Rendering methods
    void render_main_view();
    void render_timeline_panel();
    void render_breakpoints_panel();
    void render_entity_inspector_panel();
    void render_system_profiler_panel();
    void render_memory_analyzer_panel();
    void render_settings_panel();
    
    // Main view components
    void render_control_toolbar();
    void render_entity_visualization();
    void render_system_execution_overlay();
    void render_performance_graphs();
    void render_debug_console();
    
    // Timeline components
    void render_timeline_scrubber();
    void render_timeline_events();
    void render_frame_markers();
    void render_playback_controls();
    
    // Entity visualization
    void render_entity_graph();
    void render_archetype_diagram();
    void render_component_flow_diagram();
    void draw_entity_node(const EntitySnapshot& entity, f32 x, f32 y);
    void draw_component_connection(ecs::Entity from, ecs::Entity to, const std::string& component_type);
    void draw_system_processing_animation(const SystemExecution& execution);
    
    // Interactive elements
    void handle_entity_selection(f32 mouse_x, f32 mouse_y);
    void handle_timeline_scrubbing(f32 timeline_x);
    void handle_breakpoint_interaction();
    void handle_component_editing();
    
    // Operation recording and playback
    void record_operation(const ECSOperation& operation);
    void replay_to_timeline_position(f32 position);
    void create_frame_snapshot();
    void restore_frame_snapshot(const FrameSnapshot& snapshot);
    
    // Breakpoint management
    void check_breakpoints(const ECSOperation& operation);
    bool evaluate_breakpoint_condition(const Breakpoint& breakpoint, const ECSOperation& operation);
    void trigger_breakpoint(const Breakpoint& breakpoint, const ECSOperation& operation);
    
    // Entity tracking
    void track_entity(ecs::Entity entity);
    void untrack_entity(ecs::Entity entity);
    void update_entity_snapshot(ecs::Entity entity);
    EntitySnapshot create_entity_snapshot(ecs::Entity entity) const;
    
    // Performance monitoring
    void update_performance_metrics(f64 delta_time);
    void record_system_execution(const SystemExecution& execution);
    void analyze_memory_usage();
    
    // Utility methods
    f32 timeline_time_to_position(f64 time) const;
    f64 timeline_position_to_time(f32 position) const;
    std::string format_operation_description(const ECSOperation& operation) const;
    std::string format_entity_info(ecs::Entity entity) const;
    std::string format_performance_metric(f64 value, const std::string& unit) const;
    
    // Visual helpers
    ImU32 get_operation_color(ECSOperationType type) const;
    ImU32 get_entity_color(ecs::Entity entity) const;
    ImU32 get_component_color(const std::string& component_type) const;
    f32 calculate_entity_visual_size(ecs::Entity entity) const;
    
    // Event callbacks
    void on_entity_created(ecs::Entity entity);
    void on_entity_destroyed(ecs::Entity entity);
    void on_component_added(ecs::Entity entity, const std::string& component_type);
    void on_component_removed(ecs::Entity entity, const std::string& component_type);
    void on_system_executed(const std::string& system_name, const std::vector<ecs::Entity>& entities);
    
public:
    explicit VisualDebuggerPanel(std::shared_ptr<ecs::Registry> registry);
    ~VisualDebuggerPanel() override = default;
    
    // Panel interface
    void render() override;
    void update(f64 delta_time) override;
    bool wants_keyboard_capture() const override;
    bool wants_mouse_capture() const override;
    
    // Debugger control
    void start_debugging();
    void stop_debugging();
    void pause_execution();
    void resume_execution();
    void step_single_operation();
    void step_single_frame();
    
    // Recording control
    void start_recording();
    void stop_recording();
    void clear_recording();
    void save_recording(const std::string& filename);
    void load_recording(const std::string& filename);
    
    // Timeline control
    void set_timeline_position(f32 position);
    void set_timeline_zoom(f32 zoom);
    void play_timeline();
    void pause_timeline();
    void reset_timeline();
    
    // Breakpoint management
    u64 add_breakpoint(const Breakpoint& breakpoint);
    void remove_breakpoint(u64 breakpoint_id);
    void enable_breakpoint(u64 breakpoint_id, bool enabled);
    void clear_all_breakpoints();
    std::vector<Breakpoint> get_active_breakpoints() const;
    
    // Entity tracking
    void select_entity(ecs::Entity entity);
    void highlight_entity(ecs::Entity entity, bool highlight = true);
    void track_entity_lifecycle(ecs::Entity entity);
    void untrack_entity_lifecycle(ecs::Entity entity);
    
    // Visualization settings
    void set_visualization_setting(const std::string& setting, bool value);
    void set_visualization_color(const std::string& element, u32 color);
    void set_animation_speed(f32 speed);
    void enable_real_time_updates(bool enabled);
    
    // Integration with ECS registry
    void attach_to_registry(std::shared_ptr<ecs::Registry> registry);
    void detach_from_registry();
    
    // Educational features
    void start_guided_debugging_tour();
    void highlight_concept(const std::string& concept);
    void explain_current_operation();
    void show_performance_impact();
    
    // Data export
    void export_debug_session(const std::string& filename);
    void export_performance_report(const std::string& filename);
    void export_entity_lifecycle_report(ecs::Entity entity, const std::string& filename);
    
    // Event callbacks
    void set_operation_callback(std::function<void(const ECSOperation&)> callback);
    void set_breakpoint_callback(std::function<void(const Breakpoint&)> callback);
    
    // State queries
    DebuggerState current_state() const { return current_state_; }
    bool is_recording() const { return recording_enabled_; }
    bool is_paused() const { return current_state_ == DebuggerState::Paused; }
    usize recorded_operation_count() const { return recorded_operations_.size(); }
    f32 current_timeline_position() const { return timeline_position_; }
    
private:
    // Constants
    static constexpr f32 MIN_PANEL_WIDTH = 600.0f;
    static constexpr f32 MIN_PANEL_HEIGHT = 400.0f;
    static constexpr f32 ENTITY_NODE_SIZE = 20.0f;
    static constexpr f32 COMPONENT_CONNECTOR_WIDTH = 2.0f;
    static constexpr f32 TIMELINE_HEIGHT = 100.0f;
    static constexpr f32 PERFORMANCE_GRAPH_HEIGHT = 80.0f;
    static constexpr f32 ANIMATION_FADE_SPEED = 2.0f;
    static constexpr u32 MAX_ENTITIES_IN_VIEW = 1000;
    static constexpr f64 MAX_TIMELINE_DURATION = 600.0; // 10 minutes
    static constexpr f32 BREAKPOINT_PULSE_FREQUENCY = 2.0f;
    
    // Update frequencies
    static constexpr f64 SNAPSHOT_FREQUENCY = 1.0 / 30.0; // 30 FPS snapshots
    static constexpr f64 PERFORMANCE_UPDATE_FREQUENCY = 1.0 / 10.0; // 10 Hz
    static constexpr f64 ENTITY_TRACKING_FREQUENCY = 1.0 / 60.0; // 60 Hz
    
    f64 last_snapshot_time_{0.0};
    f64 last_performance_update_{0.0};
    f64 last_tracking_update_{0.0};
    
    // Frame counter
    u32 current_frame_number_{0};
};

/**
 * @brief Specialized entity graph visualization widget
 */
class EntityGraphWidget {
private:
    struct EntityNode {
        ecs::Entity entity;
        f32 x, y;
        f32 size;
        u32 color;
        std::vector<std::string> component_types;
        bool is_selected;
        bool is_highlighted;
        f32 animation_intensity;
        
        EntityNode(ecs::Entity ent) : entity(ent), x(0), y(0), size(20.0f), 
                                     color(0xFF4CAF50), is_selected(false), 
                                     is_highlighted(false), animation_intensity(0.0f) {}
    };
    
    struct ComponentConnection {
        ecs::Entity from_entity;
        ecs::Entity to_entity;
        std::string component_type;
        f32 strength; // Visual connection strength
        u32 color;
        bool is_animated;
        
        ComponentConnection(ecs::Entity from, ecs::Entity to, const std::string& type)
            : from_entity(from), to_entity(to), component_type(type), 
              strength(1.0f), color(0xFF2196F3), is_animated(false) {}
    };
    
    std::unordered_map<ecs::Entity, EntityNode> entity_nodes_;
    std::vector<ComponentConnection> component_connections_;
    
    // Layout algorithm
    void update_force_directed_layout(f64 delta_time);
    void apply_repulsion_forces();
    void apply_attraction_forces();
    void update_entity_positions();
    
    // Visualization
    void render_entity_nodes();
    void render_component_connections();
    void render_selection_overlay();
    
public:
    EntityGraphWidget() = default;
    
    void add_entity(ecs::Entity entity, const std::vector<std::string>& components);
    void remove_entity(ecs::Entity entity);
    void update_entity_components(ecs::Entity entity, const std::vector<std::string>& components);
    void add_component_connection(ecs::Entity from, ecs::Entity to, const std::string& component_type);
    
    void render(f32 width, f32 height);
    void update(f64 delta_time);
    
    void select_entity(ecs::Entity entity);
    void highlight_entity(ecs::Entity entity, bool highlight);
    
    ecs::Entity get_entity_at_position(f32 x, f32 y) const;
    void set_layout_parameters(f32 repulsion_strength, f32 attraction_strength);
    void reset_layout();
};

/**
 * @brief Interactive timeline scrubber widget
 */
class TimelineScrubberWidget {
private:
    struct TimelineMarker {
        f64 timestamp;
        std::string label;
        u32 color;
        f32 intensity;
        bool is_key_frame;
        
        TimelineMarker(f64 time, const std::string& lbl, u32 col = 0xFFFFFFFF)
            : timestamp(time), label(lbl), color(col), intensity(1.0f), is_key_frame(false) {}
    };
    
    std::vector<TimelineMarker> timeline_markers_;
    f64 timeline_duration_;
    f32 current_position_;
    f32 zoom_level_;
    f32 scroll_offset_;
    bool is_playing_;
    
    // Visual settings
    f32 timeline_height_;
    f32 scrubber_width_;
    u32 background_color_;
    u32 marker_color_;
    u32 playhead_color_;
    
public:
    TimelineScrubberWidget(f64 duration = 300.0, f32 height = 50.0f);
    
    void render(f32 width);
    void update(f64 delta_time);
    
    void add_marker(f64 timestamp, const std::string& label, u32 color = 0xFFFFFFFF);
    void clear_markers();
    
    void set_position(f32 position);
    void set_zoom(f32 zoom);
    void set_playing(bool playing);
    
    f32 get_position() const { return current_position_; }
    f32 get_zoom() const { return zoom_level_; }
    bool is_playing() const { return is_playing_; }
    
    // Event handling
    bool handle_mouse_input(f32 mouse_x, f32 mouse_y, bool mouse_down);
    f32 screen_to_timeline_position(f32 screen_x, f32 width) const;
};

} // namespace ecscope::ui