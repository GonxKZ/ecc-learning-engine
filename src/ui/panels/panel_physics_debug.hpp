#pragma once

/**
 * @file ui/panels/panel_physics_debug.hpp
 * @brief Comprehensive Physics Debug UI Panel for ECScope Educational ECS Engine - Phase 5: Física 2D
 * 
 * This panel provides comprehensive real-time physics debugging, analysis, and educational tools
 * for the ECScope physics system. It demonstrates physics concepts through interactive visualization,
 * performance analysis, and step-by-step algorithm explanations.
 * 
 * Features:
 * - Real-time physics visualization (collision shapes, forces, velocities, contacts)
 * - Interactive simulation controls (pause/play/step, time scaling)
 * - Educational algorithm breakdowns with step-by-step execution
 * - Performance analysis with optimization suggestions
 * - Interactive physics property editing and experimentation
 * - Learning tools with mathematical explanations and tutorials
 * 
 * Educational Philosophy:
 * This panel serves as both a debugging tool and an educational platform, making physics
 * concepts visible and interactive. It provides immediate feedback on parameter changes
 * and demonstrates the mathematical principles underlying 2D physics simulation.
 * 
 * @author ECScope Educational ECS Framework - Phase 5: Física 2D
 * @date 2024
 */

#include "../overlay.hpp"
#include "ecs/registry.hpp"
#include "physics/world.hpp"
#include "physics/components.hpp"
#include "physics/math.hpp"
#include "core/types.hpp"
#include "core/time.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <array>
#include <optional>
#include <chrono>

namespace ecscope::ui {

/**
 * @brief Physics Debug Panel for comprehensive physics analysis and education
 * 
 * This panel provides real-time debugging and educational tools for the physics system.
 * It's designed to be both a practical debugging tool for developers and an educational
 * resource for learning physics concepts through interactive visualization.
 */
class PhysicsDebugPanel : public Panel {
private:
    //=============================================================================
    // Panel State and Configuration
    //=============================================================================
    
    /** @brief Active tab in the physics debug panel */
    enum class ActiveTab : u8 {
        Visualization = 0,  ///< Real-time physics visualization
        Inspector,          ///< Entity physics property inspector
        Performance,        ///< Performance analysis and optimization
        Learning,           ///< Educational tools and tutorials
        Controls,           ///< Simulation controls and parameters
        Analysis            ///< Advanced physics analysis tools
    } active_tab_{ActiveTab::Visualization};
    
    /** @brief Visualization rendering options */
    struct VisualizationState {
        // Shape and collision visualization
        bool show_collision_shapes{true};
        bool show_collision_normals{false};
        bool show_contact_points{false};
        bool show_spatial_hash{false};
        bool show_aabb_bounds{false};
        bool show_compound_shapes{true};
        
        // Motion and forces visualization
        bool show_velocity_vectors{true};
        bool show_acceleration_vectors{false};
        bool show_force_vectors{true};
        bool show_center_of_mass{false};
        bool show_trajectory_trails{false};
        bool show_angular_motion{false};
        
        // Advanced visualization
        bool show_constraint_connections{true};
        bool show_trigger_bounds{false};
        bool show_sleeping_bodies{false};
        bool show_energy_visualization{false};
        
        // Visualization settings
        f32 vector_scale{1.0f};
        f32 trail_length{2.0f};
        f32 line_thickness{2.0f};
        bool use_physics_colors{true};
        f32 transparency{0.7f};
        
        // Color scheme
        u32 static_body_color{0xFF808080};     ///< Gray for static bodies
        u32 dynamic_body_color{0xFF4CAF50};   ///< Green for dynamic bodies
        u32 kinematic_body_color{0xFF2196F3}; ///< Blue for kinematic bodies
        u32 sleeping_body_color{0xFF9E9E9E};  ///< Light gray for sleeping
        u32 contact_color{0xFFFF5722};        ///< Red-orange for contacts
        u32 force_color{0xFFFF9800};          ///< Orange for forces
        u32 velocity_color{0xFF8BC34A};       ///< Light green for velocity
        u32 constraint_color{0xFF9C27B0};     ///< Purple for constraints
    } visualization_;
    
    /** @brief Simulation control state */
    struct SimulationControls {
        bool is_paused{false};
        bool single_step{false};
        bool step_requested{false};
        f32 time_scale{1.0f};
        f32 target_fps{60.0f};
        bool fixed_timestep{true};
        f32 custom_timestep{1.0f/60.0f};
        bool show_step_breakdown{false};
        
        // Interactive creation tools
        bool creation_mode{false};
        physics::components::CollisionShape::Type shape_to_create{physics::components::CollisionShape::Type::Circle};
        physics::components::PhysicsMaterial creation_material{};
        f32 creation_mass{1.0f};
        bool creation_is_static{false};
    } controls_;
    
    /** @brief Selected entity for detailed inspection */
    struct EntityInspection {
        ecs::Entity selected_entity{};
        bool entity_valid{false};
        bool auto_select_on_click{true};
        bool follow_selected{false};
        
        // Inspector state
        bool show_transform_details{true};
        bool show_rigidbody_details{true};
        bool show_collider_details{true};
        bool show_forces_details{true};
        bool show_constraints_details{true};
        bool show_performance_details{false};
        
        // Property editing
        bool enable_live_editing{true};
        bool show_advanced_properties{false};
        std::string property_search_filter;
    } inspector_;
    
    /** @brief Performance monitoring and analysis */
    struct PerformanceMonitoring {
        // Performance metrics history
        static constexpr usize HISTORY_SIZE = 120; // 2 seconds at 60fps
        std::array<f32, HISTORY_SIZE> frame_times{};
        std::array<f32, HISTORY_SIZE> physics_times{};
        std::array<f32, HISTORY_SIZE> collision_times{};
        std::array<f32, HISTORY_SIZE> integration_times{};
        std::array<f32, HISTORY_SIZE> constraint_times{};
        std::array<u32, HISTORY_SIZE> active_body_counts{};
        std::array<u32, HISTORY_SIZE> collision_check_counts{};
        std::array<u32, HISTORY_SIZE> contact_counts{};
        
        usize history_index{0};
        f64 last_update_time{0.0};
        f32 update_interval{1.0f/30.0f}; // 30Hz update rate for graphs
        
        // Analysis results
        f32 average_frame_time{0.0f};
        f32 worst_frame_time{0.0f};
        f32 physics_cpu_percentage{0.0f};
        const char* performance_rating{"Unknown"};
        std::string optimization_advice;
        
        // Memory tracking
        usize total_physics_memory{0};
        usize rigid_body_memory{0};
        usize collider_memory{0};
        usize constraint_memory{0};
        u32 allocation_count{0};
        
        // Bottleneck analysis
        const char* primary_bottleneck{"Unknown"};
        std::vector<std::string> optimization_suggestions;
        bool show_advanced_metrics{false};
        bool show_memory_details{false};
    } performance_;
    
    /** @brief Educational features and tutorials */
    struct LearningTools {
        // Tutorial state
        enum class Tutorial : u8 {
            None = 0,
            BasicPhysics,       ///< Newton's laws and basic concepts
            CollisionDetection, ///< How collision detection works
            ForceAnalysis,      ///< Understanding forces and acceleration
            EnergyConservation, ///< Energy and momentum concepts
            ConstraintPhysics,  ///< Joints and constraints
            OptimizationTech    ///< Performance optimization techniques
        } active_tutorial{Tutorial::None};
        
        usize tutorial_step{0};
        bool show_mathematical_details{false};
        bool show_algorithm_breakdown{true};
        bool interactive_examples{true};
        
        // Concept explanations
        std::unordered_map<std::string, std::string> concept_explanations;
        std::string selected_concept;
        bool show_formulas{true};
        bool show_real_world_examples{true};
        
        // Interactive experiments
        struct Experiment {
            std::string name;
            std::string description;
            std::function<void()> setup_function;
            bool is_active;
        };
        std::vector<Experiment> available_experiments;
        i32 current_experiment{-1};
    } learning_;
    
    /** @brief Advanced analysis tools */
    struct AnalysisTools {
        // Energy analysis
        bool monitor_energy_conservation{false};
        f32 total_kinetic_energy{0.0f};
        f32 total_potential_energy{0.0f};
        f32 energy_conservation_error{0.0f};
        std::array<f32, 120> energy_history{};
        
        // Force analysis
        bool analyze_force_distribution{false};
        physics::math::Vec2 net_force{0.0f, 0.0f};
        f32 total_force_magnitude{0.0f};
        std::vector<std::pair<ecs::Entity, f32>> force_contributors;
        
        // Collision statistics
        struct CollisionStats {
            u32 total_checks{0};
            u32 broad_phase_culled{0};
            u32 narrow_phase_contacts{0};
            f32 average_contact_depth{0.0f};
            f32 max_contact_force{0.0f};
        } collision_stats;
        
        // Spatial partitioning analysis
        bool analyze_spatial_efficiency{false};
        f32 spatial_hash_load_factor{0.0f};
        u32 average_objects_per_cell{0};
        u32 max_objects_per_cell{0};
        std::vector<std::pair<u32, u32>> cell_occupancy; // <cell_id, object_count>
        
        // System stability analysis
        bool check_numerical_stability{false};
        f32 max_velocity_magnitude{0.0f};
        f32 max_acceleration_magnitude{0.0f};
        bool has_nan_values{false};
        bool has_infinite_values{false};
        u32 unstable_object_count{0};
    } analysis_;
    
    //=============================================================================
    // Caching and Performance
    //=============================================================================
    
    /** @brief Cached entity data for performance */
    struct CachedEntityData {
        ecs::Entity entity;
        physics::math::Vec2 position;
        f32 rotation;
        physics::math::Vec2 velocity;
        f32 angular_velocity;
        physics::components::CollisionShape collision_shape;
        bool is_static;
        bool is_sleeping;
        f32 mass;
        std::string debug_name;
        
        // Visual data
        physics::math::AABB world_aabb;
        std::vector<physics::math::Vec2> shape_vertices; // For polygon rendering
    };
    
    std::vector<CachedEntityData> cached_entities_;
    f64 cache_update_timer_{0.0};
    static constexpr f64 CACHE_UPDATE_INTERVAL = 1.0/30.0; // 30Hz cache updates
    
    /** @brief UI state management */
    struct UIState {
        // Search and filtering
        std::string entity_search_filter;
        std::string concept_search_filter;
        bool show_only_active_bodies{false};
        bool show_only_colliding_bodies{false};
        
        // Layout and display
        f32 left_panel_width{350.0f};
        f32 right_panel_width{300.0f};
        f32 graph_height{150.0f};
        bool auto_scroll_graphs{true};
        
        // Interaction state
        physics::math::Vec2 world_mouse_pos{0.0f, 0.0f};
        physics::math::Vec2 camera_offset{0.0f, 0.0f};
        f32 camera_zoom{1.0f};
        bool mouse_interaction_enabled{true};
        
        // Panel visibility
        bool show_visualization_options{true};
        bool show_performance_graphs{true};
        bool show_entity_list{true};
        bool show_concept_explanations{false};
    } ui_state_;
    
public:
    //=============================================================================
    // Constructor and Core Interface
    //=============================================================================
    
    PhysicsDebugPanel();
    ~PhysicsDebugPanel() override = default;
    
    // Panel interface implementation
    void render() override;
    void update(f64 delta_time) override;
    
    //=============================================================================
    // Configuration and State Management
    //=============================================================================
    
    /** @brief Set which physics world to debug */
    void set_physics_world(physics::PhysicsWorld2D* world) noexcept { physics_world_ = world; }
    
    /** @brief Get current physics world */
    physics::PhysicsWorld2D* physics_world() const noexcept { return physics_world_; }
    
    /** @brief Select entity for detailed inspection */
    void select_entity(ecs::Entity entity);
    
    /** @brief Get currently selected entity */
    ecs::Entity selected_entity() const noexcept { return inspector_.selected_entity; }
    
    /** @brief Set active tab */
    void set_active_tab(ActiveTab tab) noexcept { active_tab_ = tab; }
    
    /** @brief Get active tab */
    ActiveTab active_tab() const noexcept { return active_tab_; }
    
    //=============================================================================
    // Simulation Control Interface
    //=============================================================================
    
    /** @brief Pause/unpause physics simulation */
    void set_paused(bool paused) noexcept { controls_.is_paused = paused; }
    bool is_paused() const noexcept { return controls_.is_paused; }
    
    /** @brief Request single step execution */
    void request_single_step() noexcept { controls_.step_requested = true; }
    
    /** @brief Set simulation time scale */
    void set_time_scale(f32 scale) noexcept { controls_.time_scale = std::clamp(scale, 0.01f, 10.0f); }
    f32 time_scale() const noexcept { return controls_.time_scale; }
    
    //=============================================================================
    // Visualization Control Interface
    //=============================================================================
    
    /** @brief Enable/disable collision shape rendering */
    void set_show_collision_shapes(bool show) noexcept { visualization_.show_collision_shapes = show; }
    bool show_collision_shapes() const noexcept { return visualization_.show_collision_shapes; }
    
    /** @brief Enable/disable velocity vector rendering */
    void set_show_velocity_vectors(bool show) noexcept { visualization_.show_velocity_vectors = show; }
    bool show_velocity_vectors() const noexcept { return visualization_.show_velocity_vectors; }
    
    /** @brief Enable/disable force vector rendering */
    void set_show_force_vectors(bool show) noexcept { visualization_.show_force_vectors = show; }
    bool show_force_vectors() const noexcept { return visualization_.show_force_vectors; }
    
    /** @brief Set vector scaling factor */
    void set_vector_scale(f32 scale) noexcept { visualization_.vector_scale = std::clamp(scale, 0.1f, 5.0f); }
    f32 vector_scale() const noexcept { return visualization_.vector_scale; }
    
private:
    //=============================================================================
    // Rendering Implementation
    //=============================================================================
    
    // Tab rendering functions
    void render_visualization_tab();
    void render_inspector_tab();
    void render_performance_tab();
    void render_learning_tab();
    void render_controls_tab();
    void render_analysis_tab();
    
    // Visualization rendering
    void render_visualization_options();
    void render_physics_overlay();
    void render_collision_shapes();
    void render_motion_vectors();
    void render_force_visualization();
    void render_constraint_connections();
    void render_spatial_hash_grid();
    void render_contact_points();
    void render_debug_annotations();
    
    // Inspector rendering
    void render_entity_selector();
    void render_entity_properties();
    void render_transform_inspector();
    void render_rigidbody_inspector();
    void render_collider_inspector();
    void render_forces_inspector();
    void render_constraints_inspector();
    void render_property_editor(const std::string& property_name, f32& value, f32 min_val, f32 max_val);
    void render_material_editor(physics::components::PhysicsMaterial& material);
    
    // Performance rendering
    void render_performance_graphs();
    void render_memory_usage();
    void render_bottleneck_analysis();
    void render_optimization_advice();
    void render_frame_time_graph();
    void render_collision_stats_graph();
    void render_cpu_usage_breakdown();
    
    // Learning tools rendering
    void render_tutorial_selector();
    void render_active_tutorial();
    void render_concept_explanations();
    void render_interactive_experiments();
    void render_physics_formulas();
    void render_algorithm_breakdown();
    void render_real_world_examples();
    
    // Controls rendering
    void render_simulation_controls();
    void render_world_parameters();
    void render_creation_tools();
    void render_scenario_presets();
    void render_export_import_options();
    
    // Analysis rendering
    void render_energy_analysis();
    void render_force_analysis();
    void render_collision_analysis();
    void render_spatial_analysis();
    void render_stability_analysis();
    
    //=============================================================================
    // Data Management and Caching
    //=============================================================================
    
    void update_cached_entities();
    void update_performance_metrics(f64 delta_time);
    void update_analysis_data();
    
    CachedEntityData create_cached_entity_data(ecs::Entity entity);
    bool should_update_cache() const noexcept;
    void clear_cache() noexcept;
    
    //=============================================================================
    // Educational Content Management
    //=============================================================================
    
    void initialize_learning_content();
    void setup_physics_tutorials();
    void setup_concept_explanations();
    void setup_interactive_experiments();
    
    const std::string& get_concept_explanation(const std::string& concept) const;
    void start_tutorial(LearningTools::Tutorial tutorial);
    void advance_tutorial_step();
    void setup_experiment(const std::string& experiment_name);
    
    //=============================================================================
    // Utility Functions
    //=============================================================================
    
    // Physics data extraction
    std::vector<ecs::Entity> get_physics_entities() const;
    physics::components::RigidBody2D* get_rigidbody(ecs::Entity entity) const;
    physics::components::Collider2D* get_collider(ecs::Entity entity) const;
    physics::components::ForceAccumulator* get_force_accumulator(ecs::Entity entity) const;
    ecs::components::Transform* get_transform(ecs::Entity entity) const;
    
    // Coordinate transformations
    physics::math::Vec2 world_to_screen(const physics::math::Vec2& world_pos) const;
    physics::math::Vec2 screen_to_world(const physics::math::Vec2& screen_pos) const;
    
    // Color utilities
    u32 get_body_color(const physics::components::RigidBody2D& body) const;
    u32 lerp_color(u32 color_a, u32 color_b, f32 t) const;
    
    // String formatting utilities
    std::string format_physics_value(f32 value, const char* unit, u32 decimal_places = 2) const;
    std::string format_vector(const physics::math::Vec2& vec, const char* unit = "") const;
    std::string format_performance_rating(f32 frame_time) const;
    std::string get_body_type_string(const physics::components::RigidBody2D& body) const;
    
    // Validation and safety
    bool is_valid_entity(ecs::Entity entity) const;
    bool has_physics_components(ecs::Entity entity) const;
    void validate_ui_state();
    
    //=============================================================================
    // Member Variables
    //=============================================================================
    
    physics::PhysicsWorld2D* physics_world_{nullptr}; ///< Physics world being debugged
    
    // Performance tracking
    core::Timer frame_timer_;
    std::chrono::high_resolution_clock::time_point last_frame_time_;
    
    // Educational content
    std::unordered_map<std::string, std::function<void()>> tutorial_steps_;
    std::unordered_map<std::string, std::string> physics_formulas_;
    
    // UI interaction state
    bool dragging_entity_{false};
    ecs::Entity dragged_entity_{};
    physics::math::Vec2 drag_offset_{0.0f, 0.0f};
    
    // Temporary visualization data (cleared each frame)
    struct VisualizationData {
        std::vector<std::pair<physics::math::Vec2, physics::math::Vec2>> velocity_vectors;
        std::vector<std::pair<physics::math::Vec2, physics::math::Vec2>> force_vectors;
        std::vector<physics::math::Vec2> contact_points;
        std::vector<physics::math::Vec2> constraint_connections;
        std::vector<physics::math::AABB> spatial_hash_cells;
    } current_visualization_data_;
};

} // namespace ecscope::ui