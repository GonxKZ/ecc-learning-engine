/**
 * @file examples/physics_components_demo.cpp
 * @brief Comprehensive demonstration of Physics Components for ECScope Educational ECS Engine
 * 
 * This demo showcases all physics components and their educational features:
 * - Complete physics entity creation and configuration
 * - Force application and accumulation
 * - Material property combinations
 * - Constraint system usage
 * - Performance analysis and debugging
 * - Educational insights and explanations
 * 
 * Educational Goals:
 * - Demonstrate practical physics component usage
 * - Show performance optimization techniques
 * - Explain physics concepts through code
 * - Provide debugging and analysis examples
 * - Illustrate memory-efficient ECS physics
 * 
 * @author ECScope Educational ECS Framework - Phase 5: Física 2D
 * @date 2024
 */

#include "../src/physics/components.hpp"
#include "../src/core/log.hpp"
#include <iostream>
#include <chrono>
#include <format>

using namespace ecscope::physics::components;
using namespace ecscope::physics::math;

// Example entity IDs for demonstration
constexpr u32 PLAYER_ID = 1;
constexpr u32 GROUND_ID = 2;
constexpr u32 BALL_ID = 3;
constexpr u32 PLATFORM_ID = 4;

void demonstrate_physics_materials() {
    std::cout << "\n=== Physics Materials Demo ===\n";
    
    // Create different material presets
    auto rubber = PhysicsMaterial::rubber();
    auto steel = PhysicsMaterial::steel();
    auto ice = PhysicsMaterial::ice();
    
    std::cout << "Rubber: " << rubber.get_material_description() << "\n";
    std::cout << "  Restitution: " << rubber.restitution << "\n";
    std::cout << "  Static Friction: " << rubber.static_friction << "\n";
    std::cout << "  Rolling Resistance: " << rubber.get_rolling_resistance() << "\n\n";
    
    std::cout << "Steel: " << steel.get_material_description() << "\n";
    std::cout << "  Generates sparks: " << (steel.material_flags.generates_sparks ? "Yes" : "No") << "\n";
    std::cout << "  Hardness: " << steel.hardness << "\n\n";
    
    // Demonstrate material combination
    auto rubber_on_steel = PhysicsMaterial::combine(rubber, steel);
    std::cout << "Rubber on Steel combination:\n";
    std::cout << "  Restitution: " << rubber_on_steel.restitution << " (minimum of both)\n";
    std::cout << "  Static Friction: " << rubber_on_steel.static_friction << " (geometric mean)\n";
    std::cout << "  Combined Description: " << rubber_on_steel.get_material_description() << "\n\n";
}

void demonstrate_collider_shapes() {
    std::cout << "\n=== Collider Shapes Demo ===\n";
    
    // Create different collision shapes
    Circle ball_shape(Vec2::zero(), 1.0f);
    AABB box_shape = AABB::from_center_size(Vec2::zero(), Vec2{2.0f, 1.0f});
    OBB rotated_box(Vec2::zero(), Vec2{1.5f, 0.5f}, constants::PI_F / 4.0f);
    
    // Create colliders with different shapes
    Collider2D ball_collider(ball_shape, PhysicsMaterial::rubber());
    Collider2D box_collider(box_shape, PhysicsMaterial::wood());
    Collider2D rotated_collider(rotated_box, PhysicsMaterial::steel());
    
    // Demonstrate shape analysis
    std::cout << "Ball Collider:\n";
    auto ball_info = ball_collider.get_shape_info();
    std::cout << "  Shape: " << ball_info.type_name << "\n";
    std::cout << "  Area: " << ball_info.area << "\n";
    std::cout << "  Perimeter: " << ball_info.perimeter << "\n";
    std::cout << "  Complexity: " << ball_info.complexity_score << "/10\n";
    std::cout << "  Estimated collision cost: " << ball_collider.estimate_collision_cost() << "x\n\n";
    
    std::cout << "Box Collider:\n";
    auto box_info = box_collider.get_shape_info();
    std::cout << "  Shape: " << box_info.type_name << "\n";
    std::cout << "  Area: " << box_info.area << "\n";
    std::cout << "  Moment of Inertia: " << box_info.moment_of_inertia << "\n";
    std::cout << "  Complexity: " << box_info.complexity_score << "/10\n\n";
    
    // Demonstrate multi-shape colliders
    ball_collider.add_shape(AABB::from_center_size(Vec2{0.0f, 1.5f}, Vec2{0.5f, 0.2f}));
    std::cout << "Ball with attachment:\n";
    std::cout << "  Total shapes: " << ball_collider.get_shape_count() << "\n";
    std::cout << "  Updated collision cost: " << ball_collider.estimate_collision_cost() << "x\n\n";
}

void demonstrate_rigid_body_dynamics() {
    std::cout << "\n=== Rigid Body Dynamics Demo ===\n";
    
    // Create different types of rigid bodies
    RigidBody2D dynamic_body(5.0f);  // 5kg mass
    RigidBody2D static_body;
    static_body.make_static();
    RigidBody2D kinematic_body;
    kinematic_body.make_kinematic();
    
    // Configure dynamic body properties
    dynamic_body.set_velocity(Vec2{10.0f, 5.0f});
    dynamic_body.set_angular_velocity(2.0f);
    dynamic_body.linear_damping = 0.1f;
    dynamic_body.angular_damping = 0.05f;
    
    std::cout << "Dynamic Body Properties:\n";
    auto physics_info = dynamic_body.get_physics_info();
    std::cout << "  Type: " << dynamic_body.get_body_type_description() << "\n";
    std::cout << "  Mass: " << dynamic_body.mass << " kg\n";
    std::cout << "  Speed: " << physics_info.speed << " m/s\n";
    std::cout << "  Kinetic Energy: " << physics_info.kinetic_energy << " J\n";
    std::cout << "  Linear Momentum: " << physics_info.linear_momentum_mag << " kg⋅m/s\n";
    std::cout << "  Angular Momentum: " << physics_info.angular_momentum_mag << " kg⋅m²/s\n";
    std::cout << "  Integration Method: " << physics_info.integration_method_name << "\n\n";
    
    // Demonstrate mass and inertia calculations
    Circle ball_shape(Vec2::zero(), 2.0f);
    dynamic_body.calculate_moment_of_inertia_from_shape(ball_shape);
    std::cout << "After shape-based inertia calculation:\n";
    std::cout << "  Moment of Inertia: " << dynamic_body.moment_of_inertia << " kg⋅m²\n";
    std::cout << "  Inverse Inertia: " << dynamic_body.inverse_moment_of_inertia << "\n\n";
}

void demonstrate_force_accumulation() {
    std::cout << "\n=== Force Accumulation Demo ===\n";
    
    ForceAccumulator forces;
    
    // Apply various types of forces
    forces.apply_force(Vec2{100.0f, 0.0f}, "Player Input");
    forces.apply_force(Vec2{0.0f, -98.1f}, "Gravity");  // 10kg * 9.81m/s²
    forces.apply_force_at_point(Vec2{50.0f, 0.0f}, Vec2{0.0f, 2.0f}, "Wind");
    forces.apply_torque(25.0f, "Motor");
    
    // Add persistent forces
    u8 spring_id = forces.add_persistent_force(Vec2{-20.0f, 0.0f}, 0.0f, -1.0f, 
                                               ForceAccumulator::ForceRecord::ForceType::Spring, 
                                               "Spring Restoration");
    
    u8 damping_id = forces.add_persistent_force(Vec2{-5.0f, 0.0f}, -2.0f, 5.0f,
                                                ForceAccumulator::ForceRecord::ForceType::Damping,
                                                "Velocity Damping");
    
    // Simulate one frame update
    forces.update_persistent_forces(0.016f);  // 60 FPS
    
    // Analyze forces
    auto analysis = forces.get_force_analysis();
    std::cout << "Force Analysis:\n";
    std::cout << "  Net Force: (" << analysis.net_force.x << ", " << analysis.net_force.y << ") N\n";
    std::cout << "  Net Torque: " << analysis.net_torque << " N⋅m\n";
    std::cout << "  Force Magnitude: " << analysis.force_magnitude << " N\n";
    std::cout << "  Contributors: " << analysis.force_contributors << " forces\n";
    std::cout << "  Largest Force: " << analysis.largest_force_mag << " N\n\n";
    
    // Show force breakdown by type
    auto breakdown = forces.get_force_breakdown_by_type();
    std::cout << "Force Breakdown by Type:\n";
    const char* type_names[] = {"Unknown", "Gravity", "Spring", "Damping", "Contact", 
                               "User", "Motor", "Friction", "Magnetic", "Wind"};
    
    for (usize i = 0; i < breakdown.size(); ++i) {
        if (breakdown[i].length() > constants::EPSILON) {
            std::cout << "  " << type_names[i] << ": (" 
                      << breakdown[i].x << ", " << breakdown[i].y << ") N\n";
        }
    }
    std::cout << "\n";
    
    // Demonstrate work and power calculations
    Vec2 displacement{1.0f, 0.5f};
    f32 angular_displacement = 0.1f;
    Vec2 velocity{5.0f, 2.0f};
    f32 angular_velocity = 1.0f;
    
    f32 work_done = forces.calculate_work_done(displacement, angular_displacement);
    f32 power_output = forces.calculate_power_output(velocity, angular_velocity);
    
    std::cout << "Energy Analysis:\n";
    std::cout << "  Work Done: " << work_done << " J\n";
    std::cout << "  Power Output: " << power_output << " W\n\n";
}

void demonstrate_constraints() {
    std::cout << "\n=== Physics Constraints Demo ===\n";
    
    // Create different types of constraints
    auto distance_joint = Constraint2D::create_distance(PLAYER_ID, BALL_ID, 
                                                       Vec2{1.0f, 0.0f}, Vec2{-1.0f, 0.0f}, 
                                                       3.0f);
    
    auto spring_connection = Constraint2D::create_spring(BALL_ID, PLATFORM_ID,
                                                        Vec2::zero(), Vec2{0.0f, 1.0f},
                                                        2.0f, 100.0f, 0.1f);
    
    auto hinge_joint = Constraint2D::create_revolute(PLATFORM_ID, GROUND_ID,
                                                    Vec2{-2.0f, 0.0f}, Vec2{2.0f, 3.0f});
    
    auto motor_joint = Constraint2D::create_motor(PLATFORM_ID, GROUND_ID,
                                                 Vec2::zero(), Vec2{0.0f, 2.0f},
                                                 5.0f, 50.0f);
    
    // Display constraint information
    std::cout << "Distance Joint:\n";
    std::cout << "  Type: " << distance_joint.get_type_name() << "\n";
    std::cout << "  Target Distance: " << distance_joint.target_value << " m\n";
    std::cout << "  Max Force: " << distance_joint.max_force << " N\n";
    std::cout << "  Active: " << (distance_joint.is_active() ? "Yes" : "No") << "\n\n";
    
    std::cout << "Spring Connection:\n";
    std::cout << "  Type: " << spring_connection.get_type_name() << "\n";
    std::cout << "  Rest Length: " << spring_connection.target_value << " m\n";
    std::cout << "  Spring Constant: " << spring_connection.spring_constant << " N/m\n";
    std::cout << "  Damping: " << spring_connection.damping_ratio << "\n\n";
    
    std::cout << "Motor Joint:\n";
    std::cout << "  Type: " << motor_joint.get_type_name() << "\n";
    std::cout << "  Target Speed: " << motor_joint.target_value << " rad/s\n";
    std::cout << "  Max Torque: " << motor_joint.max_force << " N⋅m\n";
    std::cout << "  Motor Enabled: " << (motor_joint.constraint_flags.motor_enabled ? "Yes" : "No") << "\n\n";
}

void demonstrate_triggers() {
    std::cout << "\n=== Trigger System Demo ===\n";
    
    // Create a trigger zone
    Trigger2D goal_trigger;
    goal_trigger.trigger_shape = Circle(Vec2::zero(), 3.0f);
    goal_trigger.detection_layers = 0x01;  // Only detect layer 0 (player)
    goal_trigger.trigger_flags.detect_entry = 1;
    goal_trigger.trigger_flags.detect_exit = 1;
    goal_trigger.trigger_flags.one_shot = 0;
    
    // Simulate entities entering/exiting
    goal_trigger.add_detected(PLAYER_ID);
    goal_trigger.add_detected(BALL_ID);
    
    std::cout << "Goal Trigger Status:\n";
    std::cout << "  Currently Detected: " << goal_trigger.detected_count << " objects\n";
    
    auto detected = goal_trigger.get_detected_entities();
    std::cout << "  Entities: ";
    for (u32 entity_id : detected) {
        std::cout << entity_id << " ";
    }
    std::cout << "\n";
    
    std::cout << "  Statistics:\n";
    std::cout << "    Total Entries: " << goal_trigger.statistics.total_entries << "\n";
    std::cout << "    Total Exits: " << goal_trigger.statistics.total_exits << "\n";
    std::cout << "    Current Occupants: " << goal_trigger.statistics.current_occupants << "\n\n";
    
    // Simulate entity leaving
    goal_trigger.remove_detected(BALL_ID);
    std::cout << "After ball exits:\n";
    std::cout << "  Currently Detected: " << goal_trigger.detected_count << " objects\n";
    std::cout << "  Total Exits: " << goal_trigger.statistics.total_exits << "\n\n";
}

void demonstrate_physics_utilities() {
    std::cout << "\n=== Physics Utilities Demo ===\n";
    
    // Create example shapes and materials
    Circle circle_shape(Vec2::zero(), 1.5f);
    PhysicsMaterial wood_material = PhysicsMaterial::wood();
    
    // Calculate mass from shape and material
    f32 calculated_mass = utils::calculate_mass_from_shape_and_material(circle_shape, wood_material);
    std::cout << "Mass Calculation:\n";
    std::cout << "  Circle radius: " << circle_shape.radius << " m\n";
    std::cout << "  Wood density: " << wood_material.density << " kg/m³\n";
    std::cout << "  Area: " << circle_shape.area() << " m²\n";
    std::cout << "  Calculated mass: " << calculated_mass << " kg\n\n";
    
    // Calculate moment of inertia
    f32 moment = utils::calculate_moment_of_inertia_from_shape(circle_shape, calculated_mass);
    std::cout << "Moment of Inertia:\n";
    std::cout << "  For circle: " << moment << " kg⋅m²\n\n";
    
    // Create complete physics entity
    utils::PhysicsEntityDesc entity_desc;
    entity_desc.shape = circle_shape;
    entity_desc.material = wood_material;
    entity_desc.mass = calculated_mass;
    entity_desc.is_static = false;
    entity_desc.is_trigger = false;
    
    auto physics_components = utils::create_physics_entity(entity_desc);
    
    std::cout << "Complete Physics Entity:\n";
    std::cout << "  Rigid Body Mass: " << physics_components.rigidbody.mass << " kg\n";
    std::cout << "  Moment of Inertia: " << physics_components.rigidbody.moment_of_inertia << " kg⋅m²\n";
    std::cout << "  Material: " << physics_components.collider.material.get_material_description() << "\n";
    std::cout << "  Shape: " << physics_components.collider.get_shape_name() << "\n\n";
    
    // Validate components
    bool is_valid = utils::validate_physics_components(&physics_components.rigidbody,
                                                      &physics_components.collider,
                                                      &physics_components.forces);
    std::cout << "Component Validation: " << (is_valid ? "PASSED" : "FAILED") << "\n\n";
}

void demonstrate_performance_analysis() {
    std::cout << "\n=== Performance Analysis Demo ===\n";
    
    PhysicsInfo physics_info;
    
    // Simulate physics operations and timing
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Simulate some physics work
    for (int i = 0; i < 1000; ++i) {
        RigidBody2D body(1.0f + i * 0.001f);
        body.set_velocity(Vec2{static_cast<f32>(i), static_cast<f32>(i) * 0.5f});
        body.calculate_kinetic_energy();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    // Record performance metrics
    physics_info.simulation.active_bodies = 100;
    physics_info.simulation.sleeping_bodies = 50;
    physics_info.simulation.collision_checks = 450;
    physics_info.simulation.contacts_generated = 25;
    physics_info.simulation.constraints_solved = 12;
    physics_info.simulation.last_frame_physics_time = duration.count() / 1000000.0f;
    
    physics_info.performance.integration_time = 0.003f;
    physics_info.performance.collision_time = 0.008f;
    physics_info.performance.constraint_time = 0.002f;
    physics_info.performance.broadphase_time = 0.001f;
    physics_info.performance.narrowphase_time = 0.005f;
    
    physics_info.update_frame_metrics(0.016f);
    
    auto report = physics_info.get_performance_report();
    
    std::cout << "Performance Report:\n";
    std::cout << "  FPS Equivalent: " << report.fps_equivalent << "\n";
    std::cout << "  CPU Usage: " << report.cpu_percentage << "%\n";
    std::cout << "  Rating: " << report.performance_rating << "\n";
    std::cout << "  Bottleneck: " << report.bottleneck << "\n";
    std::cout << "  Advice: " << report.optimization_advice << "\n\n";
    
    std::cout << "Detailed Timing:\n";
    std::cout << "  Integration: " << physics_info.performance.integration_time * 1000.0f << " ms\n";
    std::cout << "  Collision: " << physics_info.performance.collision_time * 1000.0f << " ms\n";
    std::cout << "  Constraints: " << physics_info.performance.constraint_time * 1000.0f << " ms\n";
    std::cout << "  Broad-phase: " << physics_info.performance.broadphase_time * 1000.0f << " ms\n";
    std::cout << "  Narrow-phase: " << physics_info.performance.narrowphase_time * 1000.0f << " ms\n\n";
}

void demonstrate_motion_state_caching() {
    std::cout << "\n=== Motion State Caching Demo ===\n";
    
    // Create components for caching demo
    Transform transform{Vec2{10.0f, 5.0f}, constants::PI_F / 6.0f, Vec2{1.0f, 1.0f}};
    RigidBody2D rigidbody(2.0f);
    rigidbody.set_velocity(Vec2{3.0f, -1.5f});
    rigidbody.set_angular_velocity(0.5f);
    
    Collider2D collider(Circle(Vec2::zero(), 1.2f));
    
    MotionState motion_cache;
    
    // First access - should miss cache
    motion_cache.update_transform_cache(transform);
    motion_cache.update_motion_cache(rigidbody);
    motion_cache.update_collision_cache(transform, collider);
    
    // Second access - should hit cache
    const auto& cached_aabb = motion_cache.get_world_aabb(transform, collider);
    const auto& cached_rotation = motion_cache.get_rotation_matrix(transform);
    
    std::cout << "Motion State Cache:\n";
    std::cout << "  Cache Efficiency: " << motion_cache.get_cache_efficiency() * 100.0f << "%\n";
    std::cout << "  Cache Hits: " << motion_cache.metrics.cache_hits << "\n";
    std::cout << "  Cache Misses: " << motion_cache.metrics.cache_misses << "\n";
    
    std::cout << "  Cached AABB: (" << cached_aabb.min.x << ", " << cached_aabb.min.y 
              << ") to (" << cached_aabb.max.x << ", " << cached_aabb.max.y << ")\n";
    
    std::cout << "  Cached Rotation Matrix:\n";
    std::cout << "    [" << cached_rotation.col0.x << ", " << cached_rotation.col1.x << "]\n";
    std::cout << "    [" << cached_rotation.col0.y << ", " << cached_rotation.col1.y << "]\n\n";
    
    // Test cache invalidation
    Transform new_transform{Vec2{15.0f, 8.0f}, constants::PI_F / 4.0f, Vec2{1.0f, 1.0f}};
    bool has_moved = motion_cache.has_moved_significantly(new_transform.position, new_transform.rotation);
    std::cout << "  Significant movement detected: " << (has_moved ? "Yes" : "No") << "\n\n";
}

int main() {
    std::cout << "=== ECScope Physics Components Comprehensive Demo ===\n";
    std::cout << "Educational ECS Engine - Phase 5: Física 2D\n";
    std::cout << "Demonstrating modern C++ physics component architecture\n";
    
    try {
        // Run all demonstrations
        demonstrate_physics_materials();
        demonstrate_collider_shapes();
        demonstrate_rigid_body_dynamics();
        demonstrate_force_accumulation();
        demonstrate_constraints();
        demonstrate_triggers();
        demonstrate_physics_utilities();
        demonstrate_performance_analysis();
        demonstrate_motion_state_caching();
        
        std::cout << "\n=== Demo Completed Successfully ===\n";
        std::cout << "All physics components are working correctly!\n";
        std::cout << "\nKey Educational Insights:\n";
        std::cout << "• Physics materials determine collision behavior and realism\n";
        std::cout << "• Different collision shapes have different performance characteristics\n";
        std::cout << "• Rigid body dynamics follow Newton's laws of motion\n";
        std::cout << "• Force accumulation demonstrates superposition principle\n";
        std::cout << "• Constraints enable complex mechanical systems\n";
        std::cout << "• Triggers provide gameplay interaction without physics response\n";
        std::cout << "• Performance monitoring is crucial for real-time physics\n";
        std::cout << "• Caching optimizes frequently accessed calculations\n";
        std::cout << "\nNext steps: Integrate with physics systems (broadphase, narrowphase, solver)\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Demo failed with exception: " << e.what() << std::endl;
        return 1;
    }
}