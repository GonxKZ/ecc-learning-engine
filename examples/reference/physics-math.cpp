/**
 * @file physics_math_demo.cpp
 * @brief Demonstration of the ECScope Physics Mathematics Foundation
 * 
 * This example showcases the comprehensive 2D physics mathematics library
 * implemented for Phase 5: Física 2D of the ECScope educational ECS engine.
 * 
 * The demo covers:
 * - Vector operations and transformations
 * - Geometric primitive creation and manipulation
 * - Collision detection algorithms
 * - Educational debugging features
 * - Performance analysis capabilities
 */

#include "physics/math.hpp"
#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>

using namespace ecscope::physics::math;
using namespace ecscope::physics::math::constants;

// Helper function to print vector
void print_vec2(const Vec2& v, const std::string& name) {
    std::cout << std::fixed << std::setprecision(3);
    std::cout << name << ": (" << v.x << ", " << v.y << ")\n";
}

// Helper function to print results with separator
void print_section(const std::string& title) {
    std::cout << "\n" << std::string(50, '=') << "\n";
    std::cout << " " << title << "\n";
    std::cout << std::string(50, '=') << "\n";
}

int main() {
    std::cout << "ECScope Physics Mathematics Foundation Demo\n";
    std::cout << "Phase 5: Física 2D - Educational ECS Engine\n";
    
    // =================================================================
    // 1. Vector Mathematics Demonstration
    // =================================================================
    print_section("Vector Mathematics");
    
    Vec2 v1{3.0f, 4.0f};
    Vec2 v2{1.0f, 2.0f};
    
    print_vec2(v1, "Vector A");
    print_vec2(v2, "Vector B");
    
    std::cout << "\nBasic Operations:\n";
    print_vec2(v1 + v2, "A + B");
    print_vec2(v1 - v2, "A - B");
    print_vec2(v1 * 2.0f, "A * 2");
    
    std::cout << "\nAdvanced Operations:\n";
    std::cout << "Dot product (A·B): " << v1.dot(v2) << "\n";
    std::cout << "Cross product (A×B): " << vec2::cross(v1, v2) << "\n";
    std::cout << "Length of A: " << v1.length() << "\n";
    std::cout << "Distance A to B: " << vec2::distance(v1, v2) << "\n";
    std::cout << "Angle between A and B: " << vec2::angle_between(v1, v2) << " radians\n";
    
    Vec2 normalized = v1.normalized();
    print_vec2(normalized, "A normalized");
    std::cout << "Length of normalized A: " << normalized.length() << "\n";
    
    Vec2 projected = vec2::project(v1, v2);
    print_vec2(projected, "A projected onto B");
    
    Vec2 reflected = vec2::reflect(v1, Vec2{0.0f, 1.0f});
    print_vec2(reflected, "A reflected across Y-axis");
    
    // =================================================================
    // 2. Matrix Operations Demonstration
    // =================================================================
    print_section("Matrix Operations");
    
    f32 angle = 45.0f * DEG_TO_RAD;
    Matrix2 rotation_matrix = Matrix2::rotation(angle);
    
    std::cout << "45-degree rotation matrix:\n";
    std::cout << "[" << rotation_matrix(0, 0) << ", " << rotation_matrix(0, 1) << "]\n";
    std::cout << "[" << rotation_matrix(1, 0) << ", " << rotation_matrix(1, 1) << "]\n";
    
    Vec2 point{1.0f, 0.0f};
    Vec2 rotated_point = rotation_matrix * point;
    
    print_vec2(point, "Original point");
    print_vec2(rotated_point, "After 45° rotation");
    
    Matrix2 scale_matrix = Matrix2::scale(2.0f, 0.5f);
    Vec2 scaled_point = scale_matrix * point;
    print_vec2(scaled_point, "After scaling (2x, 0.5y)");
    
    // =================================================================
    // 3. Transform2D Demonstration
    // =================================================================
    print_section("Transform2D System");
    
    Transform2D transform{Vec2{10.0f, 5.0f}, angle, Vec2{2.0f, 1.5f}};
    
    Vec2 local_point{1.0f, 1.0f};
    Vec2 world_point = transform.transform_point(local_point);
    Vec2 back_to_local = transform.inverse_transform_point(world_point);
    
    print_vec2(local_point, "Local point");
    print_vec2(world_point, "World point");
    print_vec2(back_to_local, "Back to local");
    
    print_vec2(transform.right(), "Transform right vector");
    print_vec2(transform.up(), "Transform up vector");
    
    // =================================================================
    // 4. Geometric Primitives Demonstration
    // =================================================================
    print_section("Geometric Primitives");
    
    // Circle
    Circle circle1{Vec2{0.0f, 0.0f}, 5.0f};
    Circle circle2{Vec2{8.0f, 0.0f}, 3.0f};
    
    std::cout << "Circle 1: center (0, 0), radius 5\n";
    std::cout << "Circle 2: center (8, 0), radius 3\n";
    std::cout << "Circle 1 area: " << circle1.area() << "\n";
    std::cout << "Circle 1 circumference: " << circle1.circumference() << "\n";
    
    Vec2 test_point{3.0f, 3.0f};
    std::cout << "Point (3, 3) in circle 1: " << (circle1.contains(test_point) ? "Yes" : "No") << "\n";
    
    // AABB
    AABB box1 = AABB::from_center_size(Vec2{5.0f, 5.0f}, Vec2{4.0f, 6.0f});
    std::cout << "\nAABB: center (5, 5), size (4, 6)\n";
    std::cout << "AABB area: " << box1.area() << "\n";
    
    Vec2 closest_point = box1.closest_point(Vec2{10.0f, 8.0f});
    print_vec2(closest_point, "Closest point to (10, 8)");
    
    // OBB
    OBB oriented_box{Vec2{0.0f, 0.0f}, Vec2{3.0f, 2.0f}, 30.0f * DEG_TO_RAD};
    std::cout << "\nOBB: center (0, 0), extents (3, 2), rotation 30°\n";
    std::cout << "OBB area: " << oriented_box.area() << "\n";
    
    auto corners = oriented_box.get_corners();
    std::cout << "OBB corners:\n";
    for (size_t i = 0; i < corners.size(); ++i) {
        std::cout << "  Corner " << i << ": (" << corners[i].x << ", " << corners[i].y << ")\n";
    }
    
    // Polygon
    std::vector<Vec2> triangle_vertices = {{0.0f, 0.0f}, {3.0f, 0.0f}, {1.5f, 3.0f}};
    Polygon triangle{triangle_vertices};
    
    std::cout << "\nTriangle vertices: (0,0), (3,0), (1.5,3)\n";
    std::cout << "Triangle area: " << triangle.get_area() << "\n";
    print_vec2(triangle.get_centroid(), "Triangle centroid");
    std::cout << "Is convex: " << (triangle.is_convex() ? "Yes" : "No") << "\n";
    std::cout << "Is counter-clockwise: " << (triangle.is_counter_clockwise() ? "Yes" : "No") << "\n";
    
    // =================================================================
    // 5. Collision Detection Demonstration
    // =================================================================
    print_section("Collision Detection");
    
    // Circle-Circle collision
    auto circle_result = collision::distance_circle_to_circle(circle1, circle2);
    std::cout << "Circle 1 vs Circle 2:\n";
    std::cout << "  Distance: " << circle_result.distance << "\n";
    std::cout << "  Overlapping: " << (circle_result.is_overlapping ? "Yes" : "No") << "\n";
    print_vec2(circle_result.point_a, "  Closest point on Circle 1");
    print_vec2(circle_result.point_b, "  Closest point on Circle 2");
    print_vec2(circle_result.normal, "  Normal vector");
    
    // AABB-AABB collision
    AABB box2 = AABB::from_center_size(Vec2{7.0f, 6.0f}, Vec2{3.0f, 4.0f});
    auto aabb_result = collision::distance_aabb_to_aabb(box1, box2);
    std::cout << "\nAABB 1 vs AABB 2:\n";
    std::cout << "  Distance: " << aabb_result.distance << "\n";
    std::cout << "  Overlapping: " << (aabb_result.is_overlapping ? "Yes" : "No") << "\n";
    
    // Raycast demonstration
    Ray2D ray{Vec2{-2.0f, 0.0f}, Vec2{1.0f, 0.0f}, 15.0f};
    auto raycast_result = collision::raycast_circle(ray, circle1);
    std::cout << "\nRaycast (from (-2,0) along +X axis) vs Circle 1:\n";
    std::cout << "  Hit: " << (raycast_result.hit ? "Yes" : "No") << "\n";
    if (raycast_result.hit) {
        std::cout << "  Distance to hit: " << raycast_result.distance << "\n";
        print_vec2(raycast_result.point, "  Hit point");
        print_vec2(raycast_result.normal, "  Surface normal");
    }
    
    // =================================================================
    // 6. Physics Utilities Demonstration
    // =================================================================
    print_section("Physics Utilities");
    
    // Moment of inertia calculations
    f32 mass = 10.0f;
    f32 circle_inertia = utils::moment_of_inertia_circle(mass, circle1.radius);
    f32 box_inertia = utils::moment_of_inertia_box(mass, 4.0f, 6.0f);
    f32 triangle_inertia = utils::moment_of_inertia_polygon(mass, triangle);
    
    std::cout << "Moment of inertia (mass = " << mass << " kg):\n";
    std::cout << "  Circle (r=" << circle1.radius << "): " << circle_inertia << " kg⋅m²\n";
    std::cout << "  Box (4×6): " << box_inertia << " kg⋅m²\n";
    std::cout << "  Triangle: " << triangle_inertia << " kg⋅m²\n";
    
    // Angle utilities
    f32 angle_degrees = 450.0f;  // More than full rotation
    f32 normalized_rad = utils::normalize_angle(angle_degrees * DEG_TO_RAD);
    std::cout << "\nAngle normalization:\n";
    std::cout << "  " << angle_degrees << "° = " << normalized_rad << " radians\n";
    std::cout << "  Normalized: " << utils::radians_to_degrees(normalized_rad) << "°\n";
    
    // Interpolation functions
    std::cout << "\nInterpolation functions (t = 0.3):\n";
    std::cout << "  Linear: " << 0.3f << "\n";
    std::cout << "  Smooth step: " << utils::smooth_step(0.3f) << "\n";
    std::cout << "  Smoother step: " << utils::smoother_step(0.3f) << "\n";
    std::cout << "  Ease in quad: " << utils::ease_in_quad(0.3f) << "\n";
    std::cout << "  Ease out quad: " << utils::ease_out_quad(0.3f) << "\n";
    
    // Spring force calculation
    auto spring_force = utils::calculate_spring_force(1.2f, 1.0f, 50.0f, 2.0f, 0.5f);
    std::cout << "\nSpring force (length=1.2, rest=1.0, k=50, damping=2, vel=0.5):\n";
    std::cout << "  Spring force: " << spring_force.force << " N\n";
    std::cout << "  Damping force: " << spring_force.damping_force << " N\n";
    
    // =================================================================
    // 7. Educational Debug Features Demonstration
    // =================================================================
    print_section("Educational Debug Features");
    
    // Run self-tests
    std::cout << "Running self-verification tests:\n";
    bool vector_tests_passed = debug::verify_vector_operations();
    bool collision_tests_passed = debug::verify_collision_detection();
    
    std::cout << "  Vector operations: " << (vector_tests_passed ? "PASSED" : "FAILED") << "\n";
    std::cout << "  Collision detection: " << (collision_tests_passed ? "PASSED" : "FAILED") << "\n";
    
    // Debug collision step-by-step
    auto debug_info = debug::debug_collision_detection(circle1, circle2);
    std::cout << "\nStep-by-step collision analysis:\n";
    for (size_t i = 0; i < debug_info.steps.size(); ++i) {
        const auto& step = debug_info.steps[i];
        if (step.significant) {
            std::cout << "  Step " << (i + 1) << ": " << step.description << "\n";
        }
    }
    std::cout << "  Computation time: " << debug_info.computation_time_ms << " ms\n";
    
    // Memory usage analysis
    auto memory_analysis = debug::analyze_memory_usage();
    std::cout << "\nMemory usage analysis:\n";
    std::cout << "  Circle: " << memory_analysis.shape_memory_usage[0] << " bytes\n";
    std::cout << "  AABB: " << memory_analysis.shape_memory_usage[1] << " bytes\n";
    std::cout << "  OBB: " << memory_analysis.shape_memory_usage[2] << " bytes\n";
    std::cout << "  Polygon: " << memory_analysis.shape_memory_usage[3] << " bytes\n";
    std::cout << "  Ray2D: " << memory_analysis.shape_memory_usage[4] << " bytes\n";
    std::cout << "  Recommendations: " << memory_analysis.recommendations << "\n";
    
    // Mathematical explanations
    auto cross_product_explanation = debug::explain_cross_product();
    std::cout << "\nEducational Explanation - " << cross_product_explanation.concept_name << ":\n";
    std::cout << "  Formula: " << cross_product_explanation.formula << "\n";
    std::cout << "  Intuitive: " << cross_product_explanation.intuitive_explanation << "\n";
    std::cout << "  Complexity: " << cross_product_explanation.complexity_analysis << "\n";
    
    // =================================================================
    // 8. Performance Benchmarking
    // =================================================================
    print_section("Performance Benchmarking");
    
    const int num_iterations = 100000;
    
    // Benchmark vector operations
    auto start = std::chrono::high_resolution_clock::now();
    
    Vec2 result = Vec2::zero();
    for (int i = 0; i < num_iterations; ++i) {
        Vec2 a{static_cast<f32>(i), static_cast<f32>(i + 1)};
        Vec2 b{static_cast<f32>(i + 2), static_cast<f32>(i + 3)};
        result += a + b * 2.0f - a.normalized() * b.dot(a);\n    }\n    \n    auto end = std::chrono::high_resolution_clock::now();\n    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);\n    \n    std::cout << \"Vector operations benchmark (\" << num_iterations << \" iterations):\\n\";\n    std::cout << \"  Total time: \" << duration.count() << \" microseconds\\n\";\n    std::cout << \"  Time per operation: \" << (duration.count() / static_cast<double>(num_iterations)) << \" μs\\n\";\n    std::cout << \"  Operations per second: \" << (num_iterations * 1000000.0 / duration.count()) << \"\\n\";\n    print_vec2(result, \"  Final result\");\n    \n    // Benchmark collision detection\n    start = std::chrono::high_resolution_clock::now();\n    \n    int collision_count = 0;\n    for (int i = 0; i < num_iterations / 100; ++i) {\n        Circle c1{Vec2{static_cast<f32>(i % 100), static_cast<f32>((i + 1) % 100)}, 1.0f};\n        Circle c2{Vec2{static_cast<f32>((i + 50) % 100), static_cast<f32>((i + 51) % 100)}, 1.0f};\n        if (collision::intersects_circle_circle(c1, c2)) {\n            collision_count++;\n        }\n    }\n    \n    end = std::chrono::high_resolution_clock::now();\n    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);\n    \n    std::cout << \"\\nCollision detection benchmark (\" << (num_iterations / 100) << \" iterations):\\n\";\n    std::cout << \"  Total time: \" << duration.count() << \" microseconds\\n\";\n    std::cout << \"  Time per collision test: \" << (duration.count() / static_cast<double>(num_iterations / 100)) << \" μs\\n\";\n    std::cout << \"  Collision tests per second: \" << ((num_iterations / 100) * 1000000.0 / duration.count()) << \"\\n\";\n    std::cout << \"  Collisions detected: \" << collision_count << \"\\n\";\n    \n    // =================================================================\n    // Summary\n    // =================================================================\n    print_section(\"Demo Summary\");\n    \n    std::cout << \"This demonstration showcased the comprehensive features of the\\n\";\n    std::cout << \"ECScope Physics Mathematics Foundation, including:\\n\";\n    std::cout << \"\\n\";\n    std::cout << \"✓ Advanced vector mathematics with educational explanations\\n\";\n    std::cout << \"✓ 2D transformation matrices and coordinate system handling\\n\";\n    std::cout << \"✓ Geometric primitives (Circle, AABB, OBB, Polygon, Ray2D)\\n\";\n    std::cout << \"✓ Comprehensive collision detection algorithms\\n\";\n    std::cout << \"✓ Physics utility functions and calculations\\n\";\n    std::cout << \"✓ Educational debugging and visualization features\\n\";\n    std::cout << \"✓ Performance analysis and benchmarking capabilities\\n\";\n    std::cout << \"✓ Memory-efficient implementations with cache-friendly layouts\\n\";\n    std::cout << \"\\n\";\n    std::cout << \"The library is designed for educational purposes while maintaining\\n\";\n    std::cout << \"production-ready performance suitable for real-time 2D physics simulation.\\n\";\n    \n    std::cout << \"\\nDemo completed successfully!\\n\";\n    return 0;\n}"