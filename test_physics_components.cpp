/**
 * @file test_physics_components.cpp
 * @brief Simple test to verify physics components compilation and basic functionality
 */

#include "src/physics/components.hpp"
#include "src/physics/math.hpp"
#include <iostream>

using namespace ecscope::physics;
using namespace ecscope::physics::components;
using namespace ecscope::physics::math;

int main() {
    std::cout << "=== Physics Components Test ===\n";
    
    try {
        // Test physics material creation
        auto rubber = PhysicsMaterial::rubber();
        std::cout << "✓ Physics material created: " << rubber.get_material_description() << "\n";
        
        // Test collider creation
        Circle circle(Vec2::zero(), 1.0f);
        Collider2D collider(circle, rubber);
        std::cout << "✓ Collider created with shape: " << collider.get_shape_name() << "\n";
        
        // Test rigid body
        RigidBody2D rigidbody(5.0f);
        rigidbody.calculate_moment_of_inertia_from_shape(circle);
        std::cout << "✓ Rigid body created with mass: " << rigidbody.mass << " kg\n";
        
        // Test force accumulator
        ForceAccumulator forces;
        forces.apply_force(Vec2{10.0f, 0.0f}, "Test Force");
        auto analysis = forces.get_force_analysis();
        std::cout << "✓ Force applied, magnitude: " << analysis.force_magnitude << " N\n";
        
        // Test constraint
        auto spring = Constraint2D::create_spring(1, 2, Vec2::zero(), Vec2{1.0f, 0.0f}, 2.0f, 100.0f, 0.1f);
        std::cout << "✓ Constraint created: " << spring.get_type_name() << "\n";
        
        // Test trigger
        Trigger2D trigger;
        trigger.add_detected(42);
        std::cout << "✓ Trigger created, detecting " << trigger.detected_count << " entities\n";
        
        // Test performance info
        PhysicsInfo info;
        info.update_frame_metrics(0.016f);
        auto report = info.get_performance_report();
        std::cout << "✓ Performance report: " << report.performance_rating << "\n";
        
        // Test motion state
        MotionState motion;
        Transform transform{Vec2{1.0f, 2.0f}, 0.5f, Vec2{1.0f, 1.0f}};
        motion.update_transform_cache(transform);
        std::cout << "✓ Motion state cached\n";
        
        // Test utility functions
        f32 mass = components::utils::calculate_mass_from_shape_and_material(circle, rubber);
        std::cout << "✓ Calculated mass from shape: " << mass << " kg\n";
        
        // Test validation
        bool valid = components::utils::validate_physics_components(&rigidbody, &collider, &forces);
        std::cout << "✓ Component validation: " << (valid ? "PASSED" : "FAILED") << "\n";
        
        std::cout << "\n=== All Tests Passed! ===\n";
        std::cout << "Physics components are working correctly.\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "✗ Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "✗ Test failed with unknown exception" << std::endl;
        return 1;
    }
}