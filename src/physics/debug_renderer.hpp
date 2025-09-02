#pragma once

/**
 * @file physics/debug_renderer.hpp
 * @brief Educational Physics Debug Renderer for ECScope Phase 5: Física 2D
 * 
 * This header provides comprehensive educational debugging and visualization tools
 * for the 2D physics system. It enables students to see and understand every
 * aspect of the physics simulation pipeline.
 * 
 * Key Features:
 * - Real-time physics visualization (shapes, contacts, forces)
 * - Step-by-step algorithm breakdown visualization
 * - Interactive physics parameter tuning
 * - Performance profiling visualization
 * - Educational overlays with mathematical explanations
 * - Memory usage visualization and analysis
 * 
 * Educational Philosophy:
 * The debug renderer is designed to make the invisible visible - showing
 * students exactly what's happening inside the physics engine at every step.
 * All visualizations include educational context and mathematical foundations.
 * 
 * @author ECScope Educational ECS Framework - Phase 5: Física 2D
 * @date 2024
 */

#include "world.hpp"
#include "physics_system.hpp"
#include "collision.hpp"
#include "../core/types.hpp"
#include "../core/log.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <functional>

namespace ecscope::physics::debug {

//=============================================================================
// Debug Rendering Configuration
//=============================================================================

/**
 * @brief Configuration for physics debug rendering
 */
struct DebugRenderConfig {
    /** @brief General rendering settings */
    bool enable_debug_rendering{true};
    f32 debug_line_thickness{1.0f};
    f32 debug_point_size{3.0f};
    f32 debug_text_size{12.0f};
    
    /** @brief Shape rendering */
    bool render_collision_shapes{true};
    bool render_trigger_shapes{true};
    bool render_sleeping_bodies{false};
    bool render_static_bodies{true};
    f32 shape_outline_thickness{1.5f};
    
    /** @brief Contact visualization */
    bool render_contact_points{true};
    bool render_contact_normals{true};
    bool render_contact_impulses{false};
    f32 contact_point_size{4.0f};
    f32 normal_arrow_length{20.0f};
    
    /** @brief Force and velocity visualization */
    bool render_forces{true};
    bool render_velocities{true};
    bool render_accelerations{false};
    f32 force_scale{0.1f};         // Scale factor for force vector display
    f32 velocity_scale{2.0f};      // Scale factor for velocity vector display
    f32 vector_arrow_size{3.0f};
    
    /** @brief Spatial partitioning visualization */
    bool render_spatial_hash{false};
    bool render_broad_phase_pairs{false};
    bool render_aabb_bounds{false};
    f32 spatial_grid_alpha{0.3f};
    
    /** @brief Educational overlays */
    bool show_physics_equations{false};
    bool show_performance_metrics{true};
    bool show_algorithm_explanations{false};
    bool show_memory_usage{false};
    
    /** @brief Interactive features */
    bool enable_entity_selection{true};
    bool enable_force_application{true};
    bool enable_parameter_tuning{true};
    
    /** @brief Color scheme */
    struct Colors {
        u32 dynamic_body{0xFF00FF00};      // Green
        u32 static_body{0xFF808080};       // Gray
        u32 kinematic_body{0xFF00FFFF};    // Cyan
        u32 sleeping_body{0xFF404040};     // Dark gray
        u32 trigger_shape{0xFFFF00FF};     // Magenta
        u32 contact_point{0xFFFF0000};     // Red
        u32 contact_normal{0xFFFFFF00};    // Yellow
        u32 force_vector{0xFF00FF00};      // Green
        u32 velocity_vector{0xFF0080FF};   // Orange
        u32 spatial_grid{0x4040FFFF};      // Transparent cyan
        u32 text_color{0xFFFFFFFF};        // White
        u32 background{0xFF202020};        // Dark background
    } colors;
    
    /** @brief Factory methods */
    static DebugRenderConfig create_educational() {
        DebugRenderConfig config;
        config.render_collision_shapes = true;
        config.render_contact_points = true;
        config.render_contact_normals = true;
        config.render_forces = true;
        config.render_velocities = true;
        config.show_physics_equations = true;
        config.show_performance_metrics = true;
        config.show_algorithm_explanations = true;
        return config;
    }
    
    static DebugRenderConfig create_minimal() {
        DebugRenderConfig config;
        config.render_collision_shapes = true;
        config.render_contact_points = false;
        config.render_forces = false;
        config.render_velocities = false;
        config.show_physics_equations = false;
        config.show_performance_metrics = false;
        config.debug_line_thickness = 1.0f;
        return config;
    }
};

//=============================================================================
// Debug Rendering Primitives
//=============================================================================

/**
 * @brief Basic rendering primitives for debug visualization
 * 
 * This abstract interface allows the debug renderer to work with different
 * graphics backends while providing educational rendering capabilities.
 */
class DebugRenderInterface {
public:
    virtual ~DebugRenderInterface() = default;
    
    /** @brief Line rendering */
    virtual void draw_line(const Vec2& start, const Vec2& end, u32 color, f32 thickness = 1.0f) = 0;
    
    /** @brief Circle rendering */
    virtual void draw_circle(const Vec2& center, f32 radius, u32 color, bool filled = false, f32 thickness = 1.0f) = 0;
    
    /** @brief Rectangle rendering */
    virtual void draw_rectangle(const Vec2& min, const Vec2& max, u32 color, bool filled = false, f32 thickness = 1.0f) = 0;
    
    /** @brief Oriented rectangle rendering */
    virtual void draw_obb(const Vec2& center, const Vec2& half_extents, f32 rotation, u32 color, bool filled = false, f32 thickness = 1.0f) = 0;
    
    /** @brief Polygon rendering */
    virtual void draw_polygon(const std::vector<Vec2>& vertices, u32 color, bool filled = false, f32 thickness = 1.0f) = 0;
    
    /** @brief Point rendering */
    virtual void draw_point(const Vec2& position, u32 color, f32 size = 3.0f) = 0;
    
    /** @brief Arrow rendering (for vectors) */
    virtual void draw_arrow(const Vec2& start, const Vec2& end, u32 color, f32 thickness = 1.0f, f32 head_size = 3.0f) = 0;
    
    /** @brief Text rendering */
    virtual void draw_text(const Vec2& position, const std::string& text, u32 color, f32 size = 12.0f) = 0;
    
    /** @brief Grid rendering */
    virtual void draw_grid(const Vec2& origin, const Vec2& cell_size, u32 width, u32 height, u32 color, f32 alpha = 1.0f) = 0;
    
    /** @brief Begin/end frame for batching */
    virtual void begin_frame() = 0;
    virtual void end_frame() = 0;
    
    /** @brief Coordinate transformation */
    virtual void set_camera_transform(const Vec2& position, f32 zoom, f32 rotation = 0.0f) = 0;
    
    /** @brief Screen space rendering for UI elements */
    virtual void draw_text_screen(const Vec2& screen_position, const std::string& text, u32 color, f32 size = 12.0f) = 0;
    virtual void draw_rectangle_screen(const Vec2& min, const Vec2& max, u32 color, bool filled = true) = 0;
};

//=============================================================================
// Physics Debug Renderer
//=============================================================================

/**
 * @brief Comprehensive physics debug visualization system
 * 
 * This class provides educational physics visualization with detailed
 * mathematical explanations and interactive features for learning.
 */
class PhysicsDebugRenderer {
private:
    /** @brief Core dependencies */
    std::unique_ptr<DebugRenderInterface> render_interface_;
    DebugRenderConfig config_;
    
    /** @brief Physics system references */
    const PhysicsSystem* physics_system_;
    const PhysicsWorld2D* physics_world_;
    
    /** @brief Interactive state */
    Entity selected_entity_{0};
    Vec2 mouse_world_position_{0.0f, 0.0f};
    bool dragging_entity_{false};
    Vec2 drag_offset_{0.0f, 0.0f};
    
    /** @brief Educational state */
    bool show_step_breakdown_{false};
    u32 current_algorithm_step_{0};
    std::vector<std::string> algorithm_explanations_;
    
    /** @brief Performance tracking */
    struct RenderStats {
        u32 shapes_rendered{0};
        u32 contacts_rendered{0};
        u32 vectors_rendered{0};
        f64 render_time_ms{0.0};
        u32 total_draw_calls{0};
    } render_stats_;
    
public:
    /** @brief Constructor */
    explicit PhysicsDebugRenderer(std::unique_ptr<DebugRenderInterface> render_interface,
                                 const DebugRenderConfig& config = DebugRenderConfig::create_educational())
        : render_interface_(std::move(render_interface)), config_(config) {
        
        LOG_INFO("PhysicsDebugRenderer initialized with educational features");
    }
    
    /** @brief Set physics system to visualize */
    void set_physics_system(const PhysicsSystem* system) {
        physics_system_ = system;
        if (system) {
            physics_world_ = &system->get_physics_world();
        }
    }
    
    //-------------------------------------------------------------------------
    // Main Rendering Interface
    //-------------------------------------------------------------------------
    
    /** @brief Render complete physics debug visualization */
    void render(f32 camera_x = 0.0f, f32 camera_y = 0.0f, f32 zoom = 1.0f) {
        if (!render_interface_ || !physics_system_) return;
        
        auto render_start = std::chrono::high_resolution_clock::now();
        
        render_interface_->begin_frame();
        render_interface_->set_camera_transform(Vec2{camera_x, camera_y}, zoom);
        
        render_stats_ = RenderStats{};
        
        // Render physics world elements
        if (config_.render_collision_shapes) {
            render_collision_shapes();
        }
        
        if (config_.render_contact_points) {
            render_contact_points();
        }
        
        if (config_.render_forces || config_.render_velocities) {
            render_force_and_velocity_vectors();
        }
        
        if (config_.render_spatial_hash) {
            render_spatial_partitioning();
        }
        
        // Render educational overlays
        if (config_.show_physics_equations) {
            render_physics_equations();
        }
        
        if (config_.show_performance_metrics) {
            render_performance_metrics();
        }
        
        if (config_.show_algorithm_explanations && show_step_breakdown_) {
            render_algorithm_breakdown();
        }
        
        // Render interactive elements
        if (config_.enable_entity_selection && selected_entity_ != 0) {
            render_entity_selection();
        }
        
        render_interface_->end_frame();
        
        auto render_end = std::chrono::high_resolution_clock::now();
        render_stats_.render_time_ms = std::chrono::duration<f64, std::milli>(render_end - render_start).count();
    }
    
    //-------------------------------------------------------------------------
    // Interactive Features
    //-------------------------------------------------------------------------
    
    /** @brief Handle mouse input for entity selection and manipulation */
    void handle_mouse_input(f32 mouse_x, f32 mouse_y, bool left_button_down, bool right_button_down) {
        mouse_world_position_ = Vec2{mouse_x, mouse_y};
        
        if (left_button_down && !dragging_entity_) {
            // Entity selection
            selected_entity_ = find_entity_at_position(mouse_world_position_);
            if (selected_entity_ != 0) {
                dragging_entity_ = true;
                // Calculate drag offset
                // Implementation would get entity transform and calculate offset
            }
        } else if (!left_button_down && dragging_entity_) {
            dragging_entity_ = false;
        }
        
        if (dragging_entity_ && selected_entity_ != 0) {
            // Move selected entity (for educational purposes)
            apply_entity_position(selected_entity_, mouse_world_position_ + drag_offset_);
        }
        
        if (right_button_down && config_.enable_force_application) {
            // Apply impulse to entity at mouse position
            Entity target = find_entity_at_position(mouse_world_position_);
            if (target != 0) {
                Vec2 impulse = Vec2{10.0f, 0.0f};  // Example impulse
                // Implementation would apply impulse through physics system
            }
        }
    }
    
    /** @brief Handle keyboard input for debug features */
    void handle_keyboard_input(int key, bool pressed) {
        if (!pressed) return;
        
        switch (key) {
            case 'S':  // Toggle shape rendering
                config_.render_collision_shapes = !config_.render_collision_shapes;
                break;
                
            case 'C':  // Toggle contact rendering
                config_.render_contact_points = !config_.render_contact_points;
                break;
                
            case 'F':  // Toggle force rendering
                config_.render_forces = !config_.render_forces;
                break;
                
            case 'V':  // Toggle velocity rendering
                config_.render_velocities = !config_.render_velocities;
                break;
                
            case 'G':  // Toggle spatial grid rendering
                config_.render_spatial_hash = !config_.render_spatial_hash;
                break;
                
            case 'E':  // Toggle equation display
                config_.show_physics_equations = !config_.show_physics_equations;
                break;
                
            case 'P':  // Toggle performance metrics
                config_.show_performance_metrics = !config_.show_performance_metrics;
                break;
                
            case 'A':  // Toggle algorithm explanations
                config_.show_algorithm_explanations = !config_.show_algorithm_explanations;
                break;
                
            case 'SPACE':  // Step physics simulation
                if (physics_system_) {
                    const_cast<PhysicsSystem*>(physics_system_)->request_step();
                }
                break;
        }
    }
    
    //-------------------------------------------------------------------------
    // Configuration
    //-------------------------------------------------------------------------
    
    /** @brief Update debug render configuration */
    void set_config(const DebugRenderConfig& new_config) {
        config_ = new_config;
    }
    
    const DebugRenderConfig& get_config() const { return config_; }
    
    /** @brief Enable educational step-by-step mode */
    void enable_step_breakdown(bool enabled) {
        show_step_breakdown_ = enabled;
    }
    
    /** @brief Get rendering statistics */
    const RenderStats& get_render_stats() const { return render_stats_; }

private:
    //-------------------------------------------------------------------------
    // Shape Rendering Implementation
    //-------------------------------------------------------------------------
    
    /** @brief Render all collision shapes in the physics world */
    void render_collision_shapes() {
        if (!physics_world_) return;
        
        auto visualization_data = physics_world_->get_visualization_data();
        
        for (const auto& render_shape : visualization_data.collision_shapes) {
            u32 color = get_shape_color(render_shape);
            
            std::visit([&](const auto& shape) {
                render_collision_shape(shape, render_shape.world_transform, color);
            }, render_shape.shape);
            
            ++render_stats_.shapes_rendered;
        }
    }
    
    /** @brief Render individual collision shape */
    template<typename ShapeType>
    void render_collision_shape(const ShapeType& shape, const Transform& transform, u32 color) {
        if constexpr (std::is_same_v<ShapeType, Circle>) {
            Circle world_circle = shape.transformed(Transform2D{transform});
            render_interface_->draw_circle(world_circle.center, world_circle.radius, color, false, config_.shape_outline_thickness);
            
            // Draw radius line for educational purposes
            Vec2 radius_end = world_circle.center + Vec2{world_circle.radius, 0.0f};
            render_interface_->draw_line(world_circle.center, radius_end, color, 1.0f);
            
        } else if constexpr (std::is_same_v<ShapeType, AABB>) {
            AABB world_aabb = AABB::from_center_size(transform.position, shape.size());
            render_interface_->draw_rectangle(world_aabb.min, world_aabb.max, color, false, config_.shape_outline_thickness);
            
        } else if constexpr (std::is_same_v<ShapeType, OBB>) {
            render_interface_->draw_obb(transform.position, shape.half_extents, transform.rotation, color, false, config_.shape_outline_thickness);
            
        } else if constexpr (std::is_same_v<ShapeType, Polygon>) {
            Polygon world_polygon = shape.transformed(Transform2D{transform});
            std::vector<Vec2> vertices(world_polygon.get_vertices().begin(), world_polygon.get_vertices().end());
            render_interface_->draw_polygon(vertices, color, false, config_.shape_outline_thickness);
        }
    }
    
    /** @brief Render contact points and normals */
    void render_contact_points() {
        if (!physics_world_) return;
        
        auto visualization_data = physics_world_->get_visualization_data();
        
        for (const auto& contact : visualization_data.contact_points) {
            // Render contact point
            render_interface_->draw_point(contact.point, config_.colors.contact_point, config_.contact_point_size);
            
            // Render contact normal
            if (config_.render_contact_normals) {
                Vec2 normal_end = contact.point + contact.normal * config_.normal_arrow_length;
                render_interface_->draw_arrow(contact.point, normal_end, config_.colors.contact_normal, 2.0f);
                
                // Educational: Show penetration depth
                if (contact.depth > 0.0f) {
                    std::string depth_text = "d=" + std::to_string(contact.depth);
                    render_interface_->draw_text(contact.point + Vec2{5.0f, 5.0f}, depth_text, config_.colors.text_color, 10.0f);
                }
            }
            
            ++render_stats_.contacts_rendered;
        }
    }
    
    /** @brief Render force and velocity vectors */
    void render_force_and_velocity_vectors() {
        if (!physics_world_) return;
        
        auto visualization_data = physics_world_->get_visualization_data();
        
        // Render force vectors
        if (config_.render_forces) {
            for (const auto& force_viz : visualization_data.force_vectors) {
                Vec2 scaled_force = force_viz.force_vector * config_.force_scale;
                Vec2 end_point = force_viz.application_point + scaled_force;
                
                render_interface_->draw_arrow(force_viz.application_point, end_point, 
                                           config_.colors.force_vector, 2.0f, config_.vector_arrow_size);
                
                // Educational: Show force magnitude
                std::string force_text = force_viz.force_type + "\nF=" + std::to_string(force_viz.magnitude) + "N";
                render_interface_->draw_text(end_point + Vec2{5.0f, 0.0f}, force_text, config_.colors.text_color, 10.0f);
                
                ++render_stats_.vectors_rendered;
            }
        }
        
        // Render velocity vectors
        if (config_.render_velocities) {
            for (const auto& velocity_pair : visualization_data.velocity_vectors) {
                Vec2 start = velocity_pair.first;
                Vec2 velocity = velocity_pair.second * config_.velocity_scale;
                Vec2 end = start + velocity;
                
                render_interface_->draw_arrow(start, end, config_.colors.velocity_vector, 1.5f, config_.vector_arrow_size);
                
                // Educational: Show velocity magnitude
                f32 speed = velocity.length() / config_.velocity_scale;
                std::string vel_text = "v=" + std::to_string(speed) + "m/s";
                render_interface_->draw_text(end + Vec2{5.0f, -10.0f}, vel_text, config_.colors.text_color, 10.0f);
                
                ++render_stats_.vectors_rendered;
            }
        }
    }
    
    /** @brief Render spatial partitioning grid */
    void render_spatial_partitioning() {
        if (!physics_world_) return;
        
        auto visualization_data = physics_world_->get_visualization_data();
        
        for (const auto& cell_pair : visualization_data.spatial_hash_cells) {
            const AABB& cell_bounds = cell_pair.first;
            u32 object_count = cell_pair.second;
            
            // Color cell based on occupancy
            u32 cell_color = config_.colors.spatial_grid;
            if (object_count > 4) {
                cell_color = 0x80FF4040;  // Red for high occupancy
            } else if (object_count > 2) {
                cell_color = 0x80FFFF40;  // Yellow for medium occupancy
            }
            
            render_interface_->draw_rectangle(cell_bounds.min, cell_bounds.max, cell_color, true);
            render_interface_->draw_rectangle(cell_bounds.min, cell_bounds.max, config_.colors.spatial_grid, false, 1.0f);
            
            // Show object count
            Vec2 cell_center = cell_bounds.center();
            render_interface_->draw_text(cell_center, std::to_string(object_count), config_.colors.text_color, 12.0f);
        }
    }
    
    //-------------------------------------------------------------------------
    // Educational Overlays
    //-------------------------------------------------------------------------
    
    /** @brief Render physics equations and mathematical explanations */
    void render_physics_equations() {
        Vec2 equation_pos{10.0f, 10.0f};
        
        std::vector<std::string> equations = {
            "Physics Equations:",
            "F = ma (Newton's 2nd Law)",
            "v = u + at (Kinematic equation)",
            "s = ut + ½at² (Position integration)",
            "τ = Iα (Rotational dynamics)",
            "ω = ω₀ + αt (Angular velocity)",
            "",
            "Energy Conservation:",
            "KE = ½mv² + ½Iω²",
            "PE = mgh",
            "Total Energy = KE + PE",
            "",
            "Collision Response:",
            "J = -(1+e)(vᵣ·n) / (1/mₐ + 1/mᵦ)",
            "Δv = J/m (Impulse-momentum)",
        };
        
        for (const auto& equation : equations) {
            render_interface_->draw_text_screen(equation_pos, equation, config_.colors.text_color, config_.debug_text_size);
            equation_pos.y += config_.debug_text_size + 2.0f;
        }
    }
    
    /** @brief Render performance metrics and statistics */
    void render_performance_metrics() {
        if (!physics_system_) return;
        
        auto stats = physics_system_->get_system_statistics();
        Vec2 metrics_pos{10.0f, 200.0f};
        
        std::vector<std::string> metrics = {
            "Performance Metrics:",
            "System Rating: " + stats.performance_rating,
            "Efficiency: " + std::to_string(static_cast<int>(stats.system_efficiency)) + "%",
            "Update Time: " + std::to_string(stats.profile_data.average_update_time) + " ms",
            "Entities: " + std::to_string(stats.component_stats.total_rigid_bodies),
            "Contacts: " + std::to_string(stats.world_stats.active_contacts),
            "",
            "Rendering Stats:",
            "Shapes: " + std::to_string(render_stats_.shapes_rendered),
            "Contacts: " + std::to_string(render_stats_.contacts_rendered),
            "Vectors: " + std::to_string(render_stats_.vectors_rendered),
            "Render Time: " + std::to_string(render_stats_.render_time_ms) + " ms",
        };
        
        for (const auto& metric : metrics) {
            render_interface_->draw_text_screen(metrics_pos, metric, config_.colors.text_color, config_.debug_text_size);
            metrics_pos.y += config_.debug_text_size + 2.0f;
        }
    }
    
    /** @brief Render algorithm step-by-step breakdown */
    void render_algorithm_breakdown() {
        if (!physics_system_) return;
        
        Vec2 algorithm_pos{300.0f, 10.0f};
        
        auto step_breakdown = physics_system_->get_debug_step_breakdown();
        
        render_interface_->draw_text_screen(algorithm_pos, "Physics Pipeline Steps:", config_.colors.text_color, config_.debug_text_size);
        algorithm_pos.y += config_.debug_text_size + 5.0f;
        
        for (usize i = 0; i < step_breakdown.size(); ++i) {
            u32 color = (i == current_algorithm_step_) ? 0xFF00FF00 : config_.colors.text_color;
            std::string step_text = std::to_string(i + 1) + ". " + step_breakdown[i];
            
            render_interface_->draw_text_screen(algorithm_pos, step_text, color, config_.debug_text_size);
            algorithm_pos.y += config_.debug_text_size + 2.0f;
        }
    }
    
    /** @brief Render entity selection highlight */
    void render_entity_selection() {
        // Implementation would highlight selected entity with special outline
        // and show detailed information about its physics state
    }
    
    //-------------------------------------------------------------------------
    // Utility Methods
    //-------------------------------------------------------------------------
    
    /** @brief Get appropriate color for physics shape based on its state */
    u32 get_shape_color(const PhysicsWorld2D::VisualizationData::RenderShape& render_shape) {
        if (render_shape.is_trigger) {
            return config_.colors.trigger_shape;
        }
        
        if (render_shape.is_sleeping) {
            return config_.colors.sleeping_body;
        }
        
        // Would need to determine body type from entity components
        return config_.colors.dynamic_body;  // Default to dynamic
    }
    
    /** @brief Find entity at world position (for selection) */
    Entity find_entity_at_position(const Vec2& world_position) {
        // Implementation would query physics world for entity at position
        return 0;  // Placeholder
    }
    
    /** @brief Apply position to entity (for dragging) */
    void apply_entity_position(Entity entity, const Vec2& world_position) {
        // Implementation would update entity transform through physics system
    }
};

//=============================================================================
// Educational Tutorial System
//=============================================================================

/**
 * @brief Interactive tutorial system for physics learning
 * 
 * This class provides guided tutorials that walk students through
 * physics concepts using the debug renderer for visualization.
 */
class PhysicsTutorialSystem {
private:
    PhysicsDebugRenderer* debug_renderer_;
    u32 current_tutorial_{0};
    u32 current_step_{0};
    
    struct TutorialStep {
        std::string title;
        std::string description;
        std::function<void()> setup_scene;
        std::function<void()> highlight_elements;
        std::vector<std::string> key_concepts;
    };
    
    struct Tutorial {
        std::string name;
        std::string description;
        std::vector<TutorialStep> steps;
    };
    
    std::vector<Tutorial> tutorials_;
    
public:
    explicit PhysicsTutorialSystem(PhysicsDebugRenderer& debug_renderer) 
        : debug_renderer_(&debug_renderer) {
        initialize_tutorials();
    }
    
    /** @brief Start specific tutorial */
    void start_tutorial(u32 tutorial_id) {
        if (tutorial_id < tutorials_.size()) {
            current_tutorial_ = tutorial_id;
            current_step_ = 0;
            setup_current_step();
        }
    }
    
    /** @brief Advance to next tutorial step */
    void next_step() {
        if (current_tutorial_ < tutorials_.size()) {
            auto& tutorial = tutorials_[current_tutorial_];
            if (current_step_ + 1 < tutorial.steps.size()) {
                ++current_step_;
                setup_current_step();
            }
        }
    }
    
    /** @brief Get available tutorials */
    std::vector<std::string> get_tutorial_names() const {
        std::vector<std::string> names;
        for (const auto& tutorial : tutorials_) {
            names.push_back(tutorial.name);
        }
        return names;
    }
    
private:
    void initialize_tutorials() {
        // Tutorial 1: Basic Forces and Motion
        Tutorial forces_tutorial;
        forces_tutorial.name = "Forces and Motion";
        forces_tutorial.description = "Learn about forces, acceleration, and Newton's laws";
        
        TutorialStep step1;
        step1.title = "Newton's First Law";
        step1.description = "Objects at rest stay at rest, objects in motion stay in motion, unless acted upon by a force";
        step1.key_concepts = {"Inertia", "Unbalanced forces", "State of motion"};
        forces_tutorial.steps.push_back(step1);
        
        tutorials_.push_back(forces_tutorial);
        
        // Additional tutorials would be added here...
    }
    
    void setup_current_step() {
        if (current_tutorial_ < tutorials_.size()) {
            auto& tutorial = tutorials_[current_tutorial_];
            if (current_step_ < tutorial.steps.size()) {
                auto& step = tutorial.steps[current_step_];
                if (step.setup_scene) {
                    step.setup_scene();
                }
            }
        }
    }
};

} // namespace ecscope::physics::debug