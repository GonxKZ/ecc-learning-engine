#pragma once

#include "physics_math.hpp"
#include "rigid_body.hpp"
#include "collision_detection.hpp"
#include "constraints.hpp"
#include "physics_world.hpp"
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <string>

namespace ecscope::physics {

// Debug visualization options
struct DebugRenderOptions {
    // Body visualization
    bool draw_bodies = true;
    bool draw_body_outlines = true;
    bool draw_body_centers = true;
    bool draw_body_axes = true;
    bool draw_sleeping_bodies = true;
    
    // Shape visualization  
    bool draw_shapes = true;
    bool draw_aabb = false;
    bool draw_wireframes = false;
    
    // Collision visualization
    bool draw_collision_pairs = false;
    bool draw_contact_points = true;
    bool draw_contact_normals = true;
    bool draw_contact_forces = false;
    
    // Constraint visualization
    bool draw_constraints = true;
    bool draw_constraint_forces = false;
    bool draw_joint_limits = true;
    
    // Force visualization
    bool draw_forces = false;
    bool draw_velocities = false;
    bool draw_accelerations = false;
    
    // Spatial structure visualization
    bool draw_broad_phase_grid = false;
    bool draw_spatial_hash_cells = false;
    
    // Performance visualization
    bool draw_performance_stats = true;
    bool draw_memory_usage = false;
    bool draw_timing_info = false;
    
    // Visual style
    Real line_width = 1.0f;
    Real point_size = 3.0f;
    Real arrow_head_size = 0.2f;
    Real force_scale = 0.1f;
    Real velocity_scale = 1.0f;
    
    // Colors
    Vec3 active_body_color = Vec3(0.7f, 0.9f, 0.7f);
    Vec3 sleeping_body_color = Vec3(0.5f, 0.5f, 0.7f);
    Vec3 static_body_color = Vec3(0.8f, 0.8f, 0.8f);
    Vec3 kinematic_body_color = Vec3(0.9f, 0.7f, 0.7f);
    
    Vec3 contact_point_color = Vec3(1.0f, 0.2f, 0.2f);
    Vec3 contact_normal_color = Vec3(0.2f, 1.0f, 0.2f);
    Vec3 force_color = Vec3(1.0f, 1.0f, 0.2f);
    Vec3 velocity_color = Vec3(0.2f, 0.7f, 1.0f);
    
    Vec3 constraint_color = Vec3(0.8f, 0.4f, 0.8f);
    Vec3 joint_limit_color = Vec3(1.0f, 0.5f, 0.0f);
    
    Vec3 aabb_color = Vec3(0.5f, 0.5f, 1.0f);
    Vec3 spatial_grid_color = Vec3(0.3f, 0.3f, 0.3f);
    
    // Alpha values
    Real body_alpha = 0.3f;
    Real wireframe_alpha = 0.8f;
    Real ui_alpha = 0.9f;
};

// Debug draw primitive interface
class DebugDrawInterface {
public:
    virtual ~DebugDrawInterface() = default;
    
    // 2D primitives
    virtual void draw_line_2d(const Vec2& start, const Vec2& end, const Vec3& color, Real width = 1.0f) = 0;
    virtual void draw_circle_2d(const Vec2& center, Real radius, const Vec3& color, bool filled = false, Real width = 1.0f) = 0;
    virtual void draw_box_2d(const Vec2& center, const Vec2& half_extents, Real rotation, const Vec3& color, bool filled = false, Real width = 1.0f) = 0;
    virtual void draw_polygon_2d(const std::vector<Vec2>& vertices, const Vec3& color, bool filled = false, Real width = 1.0f) = 0;
    virtual void draw_point_2d(const Vec2& position, const Vec3& color, Real size = 3.0f) = 0;
    virtual void draw_arrow_2d(const Vec2& start, const Vec2& end, const Vec3& color, Real head_size = 0.2f, Real width = 1.0f) = 0;
    
    // 3D primitives
    virtual void draw_line_3d(const Vec3& start, const Vec3& end, const Vec3& color, Real width = 1.0f) = 0;
    virtual void draw_sphere_3d(const Vec3& center, Real radius, const Vec3& color, bool filled = false, Real width = 1.0f) = 0;
    virtual void draw_box_3d(const Vec3& center, const Vec3& half_extents, const Quaternion& rotation, const Vec3& color, bool filled = false, Real width = 1.0f) = 0;
    virtual void draw_cylinder_3d(const Vec3& start, const Vec3& end, Real radius, const Vec3& color, bool filled = false, Real width = 1.0f) = 0;
    virtual void draw_point_3d(const Vec3& position, const Vec3& color, Real size = 3.0f) = 0;
    virtual void draw_arrow_3d(const Vec3& start, const Vec3& end, const Vec3& color, Real head_size = 0.2f, Real width = 1.0f) = 0;
    virtual void draw_coordinate_frame_3d(const Vec3& position, const Quaternion& rotation, Real scale = 1.0f, Real width = 2.0f) = 0;
    
    // Text rendering
    virtual void draw_text_2d(const Vec2& position, const std::string& text, const Vec3& color, Real size = 12.0f) = 0;
    virtual void draw_text_3d(const Vec3& position, const std::string& text, const Vec3& color, Real size = 12.0f) = 0;
    
    // Utility
    virtual void set_alpha(Real alpha) = 0;
    virtual void begin_frame() = 0;
    virtual void end_frame() = 0;
};

// Physics debug renderer
class PhysicsDebugRenderer {
private:
    std::unique_ptr<DebugDrawInterface> draw_interface;
    DebugRenderOptions options;
    
    // Performance tracking
    mutable size_t total_draw_calls = 0;
    mutable Real last_render_time = 0.0f;
    
    // Material color mapping
    std::unordered_map<std::string, Vec3> material_colors;
    
public:
    explicit PhysicsDebugRenderer(std::unique_ptr<DebugDrawInterface> interface) 
        : draw_interface(std::move(interface)) {
        initialize_material_colors();
    }
    
    void set_options(const DebugRenderOptions& new_options) {
        options = new_options;
    }
    
    const DebugRenderOptions& get_options() const {
        return options;
    }
    
    // Render 2D physics world
    void render_world_2d(const PhysicsWorld& world) {
        if (!draw_interface) return;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        total_draw_calls = 0;
        
        draw_interface->begin_frame();
        
        if (world.is_2d()) {
            const auto& bodies = world.get_bodies_2d();
            
            // Render spatial structures first (background)
            if (options.draw_broad_phase_grid || options.draw_spatial_hash_cells) {
                render_spatial_structures_2d(world);
            }
            
            // Render bodies
            if (options.draw_bodies) {
                for (const auto& body : bodies) {
                    render_body_2d(body, world.get_body_shape(body.id), world.get_body_material(body.id));
                }
            }
            
            // Render collision information
            if (options.draw_contact_points || options.draw_contact_normals || options.draw_collision_pairs) {
                render_collision_info_2d(world);
            }
            
            // Render constraints
            if (options.draw_constraints) {
                render_constraints_2d(world);
            }
            
            // Render forces and velocities
            if (options.draw_forces || options.draw_velocities) {
                render_forces_velocities_2d(world);
            }
        }
        
        // Render UI information
        if (options.draw_performance_stats) {
            render_performance_stats_2d(world);
        }
        
        draw_interface->end_frame();
        
        auto end_time = std::chrono::high_resolution_clock::now();
        last_render_time = std::chrono::duration<Real>(end_time - start_time).count();
    }
    
    // Render 3D physics world
    void render_world_3d(const PhysicsWorld& world) {
        if (!draw_interface) return;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        total_draw_calls = 0;
        
        draw_interface->begin_frame();
        
        if (!world.is_2d()) {
            const auto& bodies = world.get_bodies_3d();
            
            // Render spatial structures first (background)
            if (options.draw_broad_phase_grid || options.draw_spatial_hash_cells) {
                render_spatial_structures_3d(world);
            }
            
            // Render bodies
            if (options.draw_bodies) {
                for (const auto& body : bodies) {
                    render_body_3d(body, world.get_body_shape(body.id), world.get_body_material(body.id));
                }
            }
            
            // Render collision information
            if (options.draw_contact_points || options.draw_contact_normals || options.draw_collision_pairs) {
                render_collision_info_3d(world);
            }
            
            // Render constraints
            if (options.draw_constraints) {
                render_constraints_3d(world);
            }
            
            // Render forces and velocities
            if (options.draw_forces || options.draw_velocities) {
                render_forces_velocities_3d(world);
            }
        }
        
        // Render UI information
        if (options.draw_performance_stats) {
            render_performance_stats_3d(world);
        }
        
        draw_interface->end_frame();
        
        auto end_time = std::chrono::high_resolution_clock::now();
        last_render_time = std::chrono::duration<Real>(end_time - start_time).count();
    }
    
    // Render specific body with detailed information
    void render_body_detailed_2d(const RigidBody2D& body, const Shape* shape, const std::string& material_name) {
        if (!draw_interface) return;
        
        Vec3 body_color = get_body_color_2d(body);
        
        // Draw body shape
        if (shape && options.draw_shapes) {
            render_shape_2d(*shape, body.transform, body_color, options.draw_wireframes);
        }
        
        // Draw AABB
        if (shape && options.draw_aabb) {
            AABB2D aabb = shape->get_aabb_2d(body.transform);
            draw_interface->draw_box_2d(
                aabb.center(), aabb.extents(), 0.0f, 
                options.aabb_color, false, options.line_width
            );
            total_draw_calls++;
        }
        
        // Draw center of mass
        if (options.draw_body_centers) {
            draw_interface->draw_point_2d(body.transform.position, body_color, options.point_size);
            total_draw_calls++;
        }
        
        // Draw body axes
        if (options.draw_body_axes) {
            Real cos_r = std::cos(body.transform.rotation);
            Real sin_r = std::sin(body.transform.rotation);
            
            Vec2 x_axis = body.transform.position + Vec2(cos_r, sin_r) * 0.5f;
            Vec2 y_axis = body.transform.position + Vec2(-sin_r, cos_r) * 0.5f;
            
            draw_interface->draw_arrow_2d(body.transform.position, x_axis, Vec3(1, 0, 0), options.arrow_head_size, options.line_width);
            draw_interface->draw_arrow_2d(body.transform.position, y_axis, Vec3(0, 1, 0), options.arrow_head_size, options.line_width);
            total_draw_calls += 2;
        }
        
        // Draw velocity
        if (options.draw_velocities && body.velocity.length_squared() > 0.01f) {
            Vec2 velocity_end = body.transform.position + body.velocity * options.velocity_scale;
            draw_interface->draw_arrow_2d(body.transform.position, velocity_end, 
                options.velocity_color, options.arrow_head_size, options.line_width);
            total_draw_calls++;
        }
        
        // Draw forces
        if (options.draw_forces && body.force.length_squared() > 0.01f) {
            Vec2 force_end = body.transform.position + body.force * options.force_scale;
            draw_interface->draw_arrow_2d(body.transform.position, force_end, 
                options.force_color, options.arrow_head_size, options.line_width);
            total_draw_calls++;
        }
        
        // Draw body information text
        std::string info = "ID: " + std::to_string(body.id);
        if (body.is_sleeping) info += " (sleeping)";
        if (material_name != "") info += "\nMaterial: " + material_name;
        info += "\nMass: " + std::to_string(body.mass);
        info += "\nVel: " + std::to_string(body.velocity.length());
        
        draw_interface->draw_text_2d(body.transform.position + Vec2(0.5f, 0.5f), info, Vec3(1, 1, 1), 10.0f);
        total_draw_calls++;
    }
    
    void render_body_detailed_3d(const RigidBody3D& body, const Shape* shape, const std::string& material_name) {
        if (!draw_interface) return;
        
        Vec3 body_color = get_body_color_3d(body);
        
        // Draw body shape
        if (shape && options.draw_shapes) {
            render_shape_3d(*shape, body.transform, body_color, options.draw_wireframes);
        }
        
        // Draw AABB
        if (shape && options.draw_aabb) {
            AABB3D aabb = shape->get_aabb_3d(body.transform);
            draw_interface->draw_box_3d(
                aabb.center(), aabb.extents(), Quaternion::identity(),
                options.aabb_color, false, options.line_width
            );
            total_draw_calls++;
        }
        
        // Draw center of mass
        if (options.draw_body_centers) {
            draw_interface->draw_point_3d(body.transform.position, body_color, options.point_size);
            total_draw_calls++;
        }
        
        // Draw coordinate frame
        if (options.draw_body_axes) {
            draw_interface->draw_coordinate_frame_3d(body.transform.position, body.transform.rotation, 0.5f, options.line_width);
            total_draw_calls++;
        }
        
        // Draw velocity
        if (options.draw_velocities && body.velocity.length_squared() > 0.01f) {
            Vec3 velocity_end = body.transform.position + body.velocity * options.velocity_scale;
            draw_interface->draw_arrow_3d(body.transform.position, velocity_end, 
                options.velocity_color, options.arrow_head_size, options.line_width);
            total_draw_calls++;
        }
        
        // Draw forces
        if (options.draw_forces && body.force.length_squared() > 0.01f) {
            Vec3 force_end = body.transform.position + body.force * options.force_scale;
            draw_interface->draw_arrow_3d(body.transform.position, force_end, 
                options.force_color, options.arrow_head_size, options.line_width);
            total_draw_calls++;
        }
        
        // Draw angular velocity
        if (options.draw_velocities && body.angular_velocity.length_squared() > 0.01f) {
            Vec3 axis = body.angular_velocity.normalized();
            Vec3 angular_end = body.transform.position + axis * body.angular_velocity.length() * 0.5f;
            draw_interface->draw_arrow_3d(body.transform.position, angular_end, 
                Vec3(1.0f, 0.5f, 0.0f), options.arrow_head_size, options.line_width);
            total_draw_calls++;
        }
        
        // Draw body information text
        std::string info = "ID: " + std::to_string(body.id);
        if (body.is_sleeping) info += " (sleeping)";
        if (material_name != "") info += "\nMaterial: " + material_name;
        info += "\nMass: " + std::to_string(body.mass_props.mass);
        info += "\nVel: " + std::to_string(body.velocity.length());
        
        draw_interface->draw_text_3d(body.transform.position + Vec3(0.5f, 0.5f, 0.5f), info, Vec3(1, 1, 1), 10.0f);
        total_draw_calls++;
    }
    
    // Render contact manifold
    void render_contact_manifold(const ContactManifold& manifold) {
        if (!draw_interface) return;
        
        for (const auto& contact : manifold.contacts) {
            // Draw contact point
            if (options.draw_contact_points) {
                draw_interface->draw_point_3d(contact.world_position_a, options.contact_point_color, options.point_size * 1.5f);
                total_draw_calls++;
            }
            
            // Draw contact normal
            if (options.draw_contact_normals) {
                Vec3 normal_end = contact.world_position_a + contact.normal * 0.5f;
                draw_interface->draw_arrow_3d(contact.world_position_a, normal_end, 
                    options.contact_normal_color, options.arrow_head_size, options.line_width * 2.0f);
                total_draw_calls++;
            }
            
            // Draw penetration depth visualization
            if (contact.penetration > 0.01f) {
                Vec3 penetration_start = contact.world_position_a;
                Vec3 penetration_end = penetration_start - contact.normal * contact.penetration;
                draw_interface->draw_line_3d(penetration_start, penetration_end, Vec3(1.0f, 0.0f, 0.0f), options.line_width * 3.0f);
                total_draw_calls++;
            }
        }
    }
    
    // Performance and statistics
    size_t get_total_draw_calls() const { return total_draw_calls; }
    Real get_last_render_time() const { return last_render_time; }
    
private:
    void initialize_material_colors() {
        material_colors["Steel"] = Vec3(0.7f, 0.7f, 0.8f);
        material_colors["Wood"] = Vec3(0.6f, 0.4f, 0.2f);
        material_colors["Rubber"] = Vec3(0.2f, 0.2f, 0.2f);
        material_colors["Ice"] = Vec3(0.8f, 0.9f, 1.0f);
        material_colors["Glass"] = Vec3(0.9f, 0.9f, 0.9f);
        material_colors["Concrete"] = Vec3(0.7f, 0.7f, 0.6f);
        material_colors["Water"] = Vec3(0.2f, 0.4f, 0.8f);
        material_colors["Sensor"] = Vec3(1.0f, 1.0f, 0.0f);
        material_colors["Bouncy"] = Vec3(1.0f, 0.2f, 1.0f);
    }
    
    Vec3 get_body_color_2d(const RigidBody2D& body) const {
        if (body.is_sleeping && options.draw_sleeping_bodies) {
            return options.sleeping_body_color;
        }
        
        switch (body.type) {
            case BodyType::Static: return options.static_body_color;
            case BodyType::Kinematic: return options.kinematic_body_color;
            case BodyType::Dynamic: return options.active_body_color;
            default: return options.active_body_color;
        }
    }
    
    Vec3 get_body_color_3d(const RigidBody3D& body) const {
        if (body.is_sleeping && options.draw_sleeping_bodies) {
            return options.sleeping_body_color;
        }
        
        switch (body.type) {
            case BodyType::Static: return options.static_body_color;
            case BodyType::Kinematic: return options.kinematic_body_color;
            case BodyType::Dynamic: return options.active_body_color;
            default: return options.active_body_color;
        }
    }
    
    Vec3 get_material_color(const std::string& material_name) const {
        auto it = material_colors.find(material_name);
        return it != material_colors.end() ? it->second : Vec3(0.5f, 0.5f, 0.5f);
    }
    
    void render_body_2d(const RigidBody2D& body, const Shape* shape, const std::string& material_name) {
        if (!body.is_sleeping || options.draw_sleeping_bodies) {
            Vec3 body_color = get_body_color_2d(body);
            
            // Blend with material color
            if (!material_name.empty()) {
                Vec3 material_color = get_material_color(material_name);
                body_color = (body_color + material_color) * 0.5f;
            }
            
            if (shape && options.draw_shapes) {
                render_shape_2d(*shape, body.transform, body_color, options.draw_wireframes);
            }
            
            if (options.draw_body_centers) {
                draw_interface->draw_point_2d(body.transform.position, body_color, options.point_size);
                total_draw_calls++;
            }
        }
    }
    
    void render_body_3d(const RigidBody3D& body, const Shape* shape, const std::string& material_name) {
        if (!body.is_sleeping || options.draw_sleeping_bodies) {
            Vec3 body_color = get_body_color_3d(body);
            
            // Blend with material color
            if (!material_name.empty()) {
                Vec3 material_color = get_material_color(material_name);
                body_color = (body_color + material_color) * 0.5f;
            }
            
            if (shape && options.draw_shapes) {
                render_shape_3d(*shape, body.transform, body_color, options.draw_wireframes);
            }
            
            if (options.draw_body_centers) {
                draw_interface->draw_point_3d(body.transform.position, body_color, options.point_size);
                total_draw_calls++;
            }
        }
    }
    
    void render_shape_2d(const Shape& shape, const Transform2D& transform, const Vec3& color, bool wireframe) {
        draw_interface->set_alpha(wireframe ? options.wireframe_alpha : options.body_alpha);
        
        switch (shape.type) {
            case Shape::Circle: {
                const auto& circle = static_cast<const CircleShape&>(shape);
                draw_interface->draw_circle_2d(transform.position, circle.radius, color, !wireframe, options.line_width);
                total_draw_calls++;
                break;
            }
            
            case Shape::Box: {
                const auto& box = static_cast<const BoxShape2D&>(shape);
                draw_interface->draw_box_2d(transform.position, box.half_extents, transform.rotation, color, !wireframe, options.line_width);
                total_draw_calls++;
                break;
            }
            
            default:
                // Draw generic shape as point
                draw_interface->draw_point_2d(transform.position, color, options.point_size);
                total_draw_calls++;
                break;
        }
        
        draw_interface->set_alpha(1.0f);
    }
    
    void render_shape_3d(const Shape& shape, const Transform3D& transform, const Vec3& color, bool wireframe) {
        draw_interface->set_alpha(wireframe ? options.wireframe_alpha : options.body_alpha);
        
        switch (shape.type) {
            case Shape::Sphere: {
                const auto& sphere = static_cast<const SphereShape&>(shape);
                draw_interface->draw_sphere_3d(transform.position, sphere.radius, color, !wireframe, options.line_width);
                total_draw_calls++;
                break;
            }
            
            case Shape::BoxShape: {
                const auto& box = static_cast<const BoxShape3D&>(shape);
                draw_interface->draw_box_3d(transform.position, box.half_extents, transform.rotation, color, !wireframe, options.line_width);
                total_draw_calls++;
                break;
            }
            
            default:
                // Draw generic shape as point
                draw_interface->draw_point_3d(transform.position, color, options.point_size);
                total_draw_calls++;
                break;
        }
        
        draw_interface->set_alpha(1.0f);
    }
    
    void render_spatial_structures_2d(const PhysicsWorld& world) {
        // This would render the spatial hash grid visualization
        // For now, just a placeholder
        if (options.draw_spatial_hash_cells) {
            // Draw grid lines for visualization
            for (int x = -50; x <= 50; x += 10) {
                draw_interface->draw_line_2d(Vec2(x, -50), Vec2(x, 50), options.spatial_grid_color, 1.0f);
            }
            for (int y = -50; y <= 50; y += 10) {
                draw_interface->draw_line_2d(Vec2(-50, y), Vec2(50, y), options.spatial_grid_color, 1.0f);
            }
            total_draw_calls += 20;
        }
    }
    
    void render_spatial_structures_3d(const PhysicsWorld& world) {
        // Similar to 2D but for 3D grid
        // This is a placeholder implementation
        if (options.draw_spatial_hash_cells) {
            // Draw 3D grid wireframe
            for (int x = -25; x <= 25; x += 5) {
                for (int y = -25; y <= 25; y += 5) {
                    draw_interface->draw_line_3d(Vec3(x, y, -25), Vec3(x, y, 25), options.spatial_grid_color, 1.0f);
                }
            }
            total_draw_calls += 100;  // Approximate
        }
    }
    
    void render_collision_info_2d(const PhysicsWorld& world) {
        // This would render collision pairs and contact information
        // Placeholder implementation
        if (options.draw_collision_pairs) {
            // Would draw lines between colliding bodies
        }
    }
    
    void render_collision_info_3d(const PhysicsWorld& world) {
        // Similar placeholder for 3D collision info
    }
    
    void render_constraints_2d(const PhysicsWorld& world) {
        // Render constraint visualization
        // Placeholder - would iterate through constraint solver and draw constraints
    }
    
    void render_constraints_3d(const PhysicsWorld& world) {
        // 3D constraint visualization
        // Placeholder
    }
    
    void render_forces_velocities_2d(const PhysicsWorld& world) {
        if (world.is_2d()) {
            const auto& bodies = world.get_bodies_2d();
            for (const auto& body : bodies) {
                if (options.draw_velocities && body.velocity.length_squared() > 0.01f) {
                    Vec2 vel_end = body.transform.position + body.velocity * options.velocity_scale;
                    draw_interface->draw_arrow_2d(body.transform.position, vel_end, options.velocity_color, options.arrow_head_size, options.line_width);
                    total_draw_calls++;
                }
                
                if (options.draw_forces && body.force.length_squared() > 0.01f) {
                    Vec2 force_end = body.transform.position + body.force * options.force_scale;
                    draw_interface->draw_arrow_2d(body.transform.position, force_end, options.force_color, options.arrow_head_size, options.line_width);
                    total_draw_calls++;
                }
            }
        }
    }
    
    void render_forces_velocities_3d(const PhysicsWorld& world) {
        if (!world.is_2d()) {
            const auto& bodies = world.get_bodies_3d();
            for (const auto& body : bodies) {
                if (options.draw_velocities && body.velocity.length_squared() > 0.01f) {
                    Vec3 vel_end = body.transform.position + body.velocity * options.velocity_scale;
                    draw_interface->draw_arrow_3d(body.transform.position, vel_end, options.velocity_color, options.arrow_head_size, options.line_width);
                    total_draw_calls++;
                }
                
                if (options.draw_forces && body.force.length_squared() > 0.01f) {
                    Vec3 force_end = body.transform.position + body.force * options.force_scale;
                    draw_interface->draw_arrow_3d(body.transform.position, force_end, options.force_color, options.arrow_head_size, options.line_width);
                    total_draw_calls++;
                }
            }
        }
    }
    
    void render_performance_stats_2d(const PhysicsWorld& world) {
        const PhysicsStats& stats = world.get_stats();
        
        std::string stats_text = "=== Physics Stats ===\n";
        stats_text += "Bodies: " + std::to_string(stats.active_bodies) + " active, " + std::to_string(stats.sleeping_bodies) + " sleeping\n";
        stats_text += "Shapes: " + std::to_string(stats.total_shapes) + "\n";
        stats_text += "Collision Pairs: " + std::to_string(stats.collision_pairs) + "\n";
        stats_text += "Active Contacts: " + std::to_string(stats.active_contacts) + "\n";
        stats_text += "Constraints: " + std::to_string(stats.active_constraints) + "\n";
        stats_text += "FPS: " + std::to_string(static_cast<int>(stats.fps)) + "\n";
        stats_text += "Step Time: " + std::to_string(stats.total_time * 1000.0f) + " ms\n";
        stats_text += "  Broad Phase: " + std::to_string(stats.broad_phase_time * 1000.0f) + " ms\n";
        stats_text += "  Narrow Phase: " + std::to_string(stats.narrow_phase_time * 1000.0f) + " ms\n";
        stats_text += "  Constraints: " + std::to_string(stats.constraint_solving_time * 1000.0f) + " ms\n";
        stats_text += "  Integration: " + std::to_string(stats.integration_time * 1000.0f) + " ms\n";
        stats_text += "Memory: " + std::to_string(stats.memory_usage_bytes / 1024) + " KB\n";
        stats_text += "Efficiency: " + std::to_string(static_cast<int>(stats.efficiency_ratio * 100)) + "%\n";
        stats_text += "Render Calls: " + std::to_string(total_draw_calls) + "\n";
        stats_text += "Render Time: " + std::to_string(last_render_time * 1000.0f) + " ms";
        
        draw_interface->set_alpha(options.ui_alpha);
        draw_interface->draw_text_2d(Vec2(10.0f, 10.0f), stats_text, Vec3(1.0f, 1.0f, 1.0f), 11.0f);
        draw_interface->set_alpha(1.0f);
        
        total_draw_calls++;
    }
    
    void render_performance_stats_3d(const PhysicsWorld& world) {
        render_performance_stats_2d(world);  // Same stats for both 2D and 3D
    }
};

// Simple OpenGL-based debug draw implementation (example)
class OpenGLDebugDraw : public DebugDrawInterface {
private:
    // This would contain OpenGL-specific rendering code
    // Placeholder implementation
    
public:
    void draw_line_2d(const Vec2& start, const Vec2& end, const Vec3& color, Real width = 1.0f) override {
        // OpenGL line drawing code
    }
    
    void draw_circle_2d(const Vec2& center, Real radius, const Vec3& color, bool filled = false, Real width = 1.0f) override {
        // OpenGL circle drawing code
    }
    
    void draw_box_2d(const Vec2& center, const Vec2& half_extents, Real rotation, const Vec3& color, bool filled = false, Real width = 1.0f) override {
        // OpenGL box drawing code
    }
    
    void draw_polygon_2d(const std::vector<Vec2>& vertices, const Vec3& color, bool filled = false, Real width = 1.0f) override {
        // OpenGL polygon drawing code
    }
    
    void draw_point_2d(const Vec2& position, const Vec3& color, Real size = 3.0f) override {
        // OpenGL point drawing code
    }
    
    void draw_arrow_2d(const Vec2& start, const Vec2& end, const Vec3& color, Real head_size = 0.2f, Real width = 1.0f) override {
        // Draw line + arrowhead
        draw_line_2d(start, end, color, width);
        
        Vec2 direction = (end - start).normalized();
        Vec2 perpendicular = direction.perpendicular();
        
        Vec2 head1 = end - direction * head_size + perpendicular * head_size * 0.5f;
        Vec2 head2 = end - direction * head_size - perpendicular * head_size * 0.5f;
        
        draw_line_2d(end, head1, color, width);
        draw_line_2d(end, head2, color, width);
    }
    
    void draw_line_3d(const Vec3& start, const Vec3& end, const Vec3& color, Real width = 1.0f) override {
        // OpenGL 3D line drawing code
    }
    
    void draw_sphere_3d(const Vec3& center, Real radius, const Vec3& color, bool filled = false, Real width = 1.0f) override {
        // OpenGL sphere drawing code
    }
    
    void draw_box_3d(const Vec3& center, const Vec3& half_extents, const Quaternion& rotation, const Vec3& color, bool filled = false, Real width = 1.0f) override {
        // OpenGL 3D box drawing code
    }
    
    void draw_cylinder_3d(const Vec3& start, const Vec3& end, Real radius, const Vec3& color, bool filled = false, Real width = 1.0f) override {
        // OpenGL cylinder drawing code
    }
    
    void draw_point_3d(const Vec3& position, const Vec3& color, Real size = 3.0f) override {
        // OpenGL 3D point drawing code
    }
    
    void draw_arrow_3d(const Vec3& start, const Vec3& end, const Vec3& color, Real head_size = 0.2f, Real width = 1.0f) override {
        // 3D arrow implementation
        draw_line_3d(start, end, color, width);
        
        // Create arrowhead (simplified)
        Vec3 direction = (end - start).normalized();
        Vec3 up = Vec3::unit_y();
        if (std::abs(direction.dot(up)) > 0.9f) {
            up = Vec3::unit_x();
        }
        Vec3 right = direction.cross(up).normalized();
        up = right.cross(direction).normalized();
        
        Vec3 head_base = end - direction * head_size;
        Vec3 head1 = head_base + (up + right) * head_size * 0.5f;
        Vec3 head2 = head_base + (up - right) * head_size * 0.5f;
        Vec3 head3 = head_base + (-up + right) * head_size * 0.5f;
        Vec3 head4 = head_base + (-up - right) * head_size * 0.5f;
        
        draw_line_3d(end, head1, color, width);
        draw_line_3d(end, head2, color, width);
        draw_line_3d(end, head3, color, width);
        draw_line_3d(end, head4, color, width);
    }
    
    void draw_coordinate_frame_3d(const Vec3& position, const Quaternion& rotation, Real scale = 1.0f, Real width = 2.0f) override {
        Vec3 x_axis = rotation.rotate_vector(Vec3::unit_x()) * scale + position;
        Vec3 y_axis = rotation.rotate_vector(Vec3::unit_y()) * scale + position;
        Vec3 z_axis = rotation.rotate_vector(Vec3::unit_z()) * scale + position;
        
        draw_arrow_3d(position, x_axis, Vec3(1, 0, 0), scale * 0.1f, width);  // Red X
        draw_arrow_3d(position, y_axis, Vec3(0, 1, 0), scale * 0.1f, width);  // Green Y
        draw_arrow_3d(position, z_axis, Vec3(0, 0, 1), scale * 0.1f, width);  // Blue Z
    }
    
    void draw_text_2d(const Vec2& position, const std::string& text, const Vec3& color, Real size = 12.0f) override {
        // Text rendering implementation (could use bitmap fonts, FreeType, etc.)
    }
    
    void draw_text_3d(const Vec3& position, const std::string& text, const Vec3& color, Real size = 12.0f) override {
        // 3D text rendering (billboard or world-space text)
    }
    
    void set_alpha(Real alpha) override {
        // Set OpenGL alpha blending
    }
    
    void begin_frame() override {
        // Initialize OpenGL state for debug drawing
    }
    
    void end_frame() override {
        // Finalize OpenGL debug drawing
    }
};

} // namespace ecscope::physics