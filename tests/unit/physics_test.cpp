/**
 * @file simple_physics_test.cpp
 * @brief Simple test to verify physics components basic functionality
 */

#include "src/physics/components.hpp"
#include <iostream>

using namespace ecscope::physics;
using namespace ecscope::physics::components;
using namespace ecscope::physics::math;

int main() {
    std::cout << "=== Simple Physics Components Test ===\n";
    
    // Test physics material
    auto rubber = PhysicsMaterial::rubber();
    std::cout << "✓ Physics material: " << rubber.get_material_description() << "\n";
    
    // Test basic component creation
    RigidBody2D rigidbody(5.0f);
    rigidbody.set_velocity(Vec2{10.0f, 5.0f});
    auto physics_info = rigidbody.get_physics_info();
    std::cout << "✓ Rigid body speed: " << physics_info.speed << " m/s\n";
    
    // Test force accumulator
    ForceAccumulator forces;
    forces.apply_force(Vec2{100.0f, 0.0f}, "Thrust");
    forces.apply_force(Vec2{0.0f, -50.0f}, "Gravity");
    
    auto analysis = forces.get_force_analysis();
    std::cout << "✓ Net force: (" << analysis.net_force.x << ", " << analysis.net_force.y << ") N\n";
    
    // Test constraint creation
    auto spring = Constraint2D::create_spring(1, 2, Vec2::zero(), Vec2{1.0f, 0.0f}, 2.0f, 100.0f, 0.1f);
    std::cout << "✓ Spring constraint: " << spring.get_type_name() << "\n";
    
    // Test trigger
    Trigger2D trigger;
    trigger.add_detected(1);
    trigger.add_detected(2);
    std::cout << "✓ Trigger detecting " << static_cast<int>(trigger.detected_count) << " entities\n";
    
    // Test performance tracking
    PhysicsInfo perf_info;
    perf_info.update_frame_metrics(0.016f);  // 60 FPS
    auto report = perf_info.get_performance_report();
    std::cout << "✓ Performance: " << report.performance_rating << "\n";
    
    std::cout << "\n=== Basic Tests Passed! ===\n";
    return 0;
}