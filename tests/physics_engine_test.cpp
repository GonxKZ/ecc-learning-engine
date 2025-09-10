#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "ecscope/physics/physics_world.hpp"
#include "ecscope/physics/collision_detection.hpp"
#include "ecscope/physics/constraints.hpp"
#include <chrono>
#include <random>

namespace ecscope::physics::test {

class PhysicsEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        world_config = PhysicsWorldConfig{};
        world_config.gravity = Vec3(0, -9.81f, 0);
        world_config.time_step = 1.0f / 60.0f;
        world_config.velocity_iterations = 8;
        world_config.position_iterations = 3;
        world_config.enable_multithreading = true;
        
        world = std::make_unique<PhysicsWorld>(world_config);
    }
    
    void TearDown() override {
        world.reset();
    }
    
    PhysicsWorldConfig world_config;
    std::unique_ptr<PhysicsWorld> world;
};

// Test basic physics world initialization
TEST_F(PhysicsEngineTest, WorldInitialization) {
    EXPECT_TRUE(world != nullptr);
    EXPECT_EQ(world->get_config().gravity.y, -9.81f);
    EXPECT_EQ(world->get_config().time_step, 1.0f / 60.0f);
    
    PhysicsStats stats = world->get_stats();
    EXPECT_EQ(stats.active_bodies, 0);
    EXPECT_EQ(stats.total_time, 0.0f);
}

// Test sphere collision detection
TEST_F(PhysicsEngineTest, SphereCollisionDetection) {
    // Create two overlapping spheres
    SphereShape sphere_a(1.0f);
    SphereShape sphere_b(1.0f);
    
    Transform3D transform_a(Vec3(0, 0, 0), Quaternion::identity());
    Transform3D transform_b(Vec3(1.5f, 0, 0), Quaternion::identity()); // Overlapping
    
    ContactManifold manifold(1, 2);
    bool collision = test_sphere_sphere_optimized(sphere_a, transform_a, sphere_b, transform_b, manifold);
    
    EXPECT_TRUE(collision);
    EXPECT_FALSE(manifold.contacts.empty());
    EXPECT_GT(manifold.contacts[0].penetration, 0.0f);
    EXPECT_NEAR(manifold.contacts[0].penetration, 0.5f, 0.01f); // Expected penetration
}

// Test sphere separation
TEST_F(PhysicsEngineTest, SphereSeparation) {
    SphereShape sphere_a(1.0f);
    SphereShape sphere_b(1.0f);
    
    Transform3D transform_a(Vec3(0, 0, 0), Quaternion::identity());
    Transform3D transform_b(Vec3(3.0f, 0, 0), Quaternion::identity()); // Separated
    
    ContactManifold manifold(1, 2);
    bool collision = test_sphere_sphere_optimized(sphere_a, transform_a, sphere_b, transform_b, manifold);
    
    EXPECT_FALSE(collision);
    EXPECT_TRUE(manifold.contacts.empty());
}

// Test broad phase performance with many objects
TEST_F(PhysicsEngineTest, BroadPhasePerformance) {
    const size_t object_count = 1000;
    auto broad_phase = create_optimal_broad_phase(object_count, 1000.0f);
    
    // Create random sphere bodies
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<Real> pos_dist(-50.0f, 50.0f);
    
    std::vector<RigidBody3D> bodies;
    std::vector<SphereShape> shapes;
    
    for (size_t i = 0; i < object_count; ++i) {
        RigidBody3D body;
        body.id = static_cast<uint32_t>(i);
        body.transform.position = Vec3(pos_dist(gen), pos_dist(gen), pos_dist(gen));
        body.type = BodyType::Dynamic;
        bodies.push_back(body);
        
        shapes.emplace_back(1.0f); // 1m radius spheres
    }
    
    // Measure broad phase performance
    auto start = std::chrono::high_resolution_clock::now();
    
    broad_phase->clear();
    for (size_t i = 0; i < bodies.size(); ++i) {
        broad_phase->add_body_3d(bodies[i], shapes[i]);
    }
    broad_phase->find_collision_pairs_3d();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    // Should complete in under 1ms for 1000 objects
    EXPECT_LT(duration, 1000);
    
    BroadPhaseStats stats = broad_phase->get_stats();
    EXPECT_EQ(stats.total_objects, object_count);
    EXPECT_GT(stats.total_cells, 0);
    EXPECT_LT(stats.efficiency_ratio, 1.0f); // Should filter out most pairs
    
    std::cout << "Broad phase performance: " << duration << " microseconds for " 
              << object_count << " objects\n";
    std::cout << "Generated " << stats.total_pairs << " collision pairs\n";
    std::cout << "Efficiency ratio: " << stats.efficiency_ratio << "\n";
}

// Test GJK algorithm correctness
TEST_F(PhysicsEngineTest, GJKCollisionDetection) {
    // Test GJK with box shapes
    BoxShape3D box_a(Vec3(1, 1, 1));
    BoxShape3D box_b(Vec3(1, 1, 1));
    
    Transform3D transform_a(Vec3(0, 0, 0), Quaternion::identity());
    Transform3D transform_b(Vec3(1.5f, 0, 0), Quaternion::identity()); // Overlapping
    
    Simplex simplex;
    bool collision = GJK::intersects(box_a, transform_a, box_b, transform_b, simplex);
    
    EXPECT_TRUE(collision);
    EXPECT_GT(simplex.size(), 0);
}

// Test EPA contact generation
TEST_F(PhysicsEngineTest, EPAContactGeneration) {
    BoxShape3D box_a(Vec3(1, 1, 1));
    BoxShape3D box_b(Vec3(1, 1, 1));
    
    Transform3D transform_a(Vec3(0, 0, 0), Quaternion::identity());
    Transform3D transform_b(Vec3(1.5f, 0, 0), Quaternion::identity()); // Overlapping
    
    Simplex simplex;
    bool collision = GJK::intersects(box_a, transform_a, box_b, transform_b, simplex);
    ASSERT_TRUE(collision);
    
    ContactManifold manifold = EPA::get_contact_manifold(box_a, transform_a, box_b, transform_b, simplex);
    
    EXPECT_FALSE(manifold.contacts.empty());
    EXPECT_GT(manifold.contacts[0].penetration, 0.0f);
    EXPECT_NEAR(manifold.contacts[0].penetration, 0.5f, 0.1f);
}

// Test constraint solver stability
TEST_F(PhysicsEngineTest, ConstraintSolverStability) {
    // Create a simple distance constraint scenario
    RigidBody3D body_a, body_b;
    body_a.id = 1;
    body_b.id = 2;
    body_a.type = BodyType::Dynamic;
    body_b.type = BodyType::Dynamic;
    body_a.mass = 1.0f;
    body_b.mass = 1.0f;
    body_a.inverse_mass = 1.0f;
    body_b.inverse_mass = 1.0f;
    body_a.inverse_inertia_tensor = Mat3::identity();
    body_b.inverse_inertia_tensor = Mat3::identity();
    
    body_a.transform.position = Vec3(0, 0, 0);
    body_b.transform.position = Vec3(5, 0, 0); // Too far apart
    
    DistanceConstraint constraint;
    constraint.local_anchor_a = Vec3::zero();
    constraint.local_anchor_b = Vec3::zero();
    constraint.rest_length = 2.0f;
    
    // Solve constraint multiple times to test stability
    for (int i = 0; i < 10; ++i) {
        constraint.solve_constraint(body_a, body_b, 1.0f / 60.0f);
    }
    
    // Check that bodies have moved closer to desired distance
    Real final_distance = (body_b.transform.position - body_a.transform.position).length();
    EXPECT_LT(std::abs(final_distance - constraint.rest_length), 0.5f);
}

// Test material property combinations
TEST_F(PhysicsEngineTest, MaterialPropertyCombination) {
    Material mat_a{0.5f, 0.7f, 1.0f}; // friction, restitution, density
    Material mat_b{0.3f, 0.4f, 2.0f};
    
    Material combined = Material::combine(mat_a, mat_b);
    
    // Friction should be geometric mean
    EXPECT_NEAR(combined.friction, std::sqrt(0.5f * 0.3f), 0.01f);
    
    // Restitution should be maximum
    EXPECT_NEAR(combined.restitution, 0.7f, 0.01f);
    
    // Density combination (for this test, just check it's reasonable)
    EXPECT_GT(combined.density, 0.0f);
}

// Performance stress test
TEST_F(PhysicsEngineTest, PerformanceStressTest) {
    const size_t body_count = 5000;
    
    // Create many dynamic bodies
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<Real> pos_dist(-25.0f, 25.0f);
    std::uniform_real_distribution<Real> vel_dist(-5.0f, 5.0f);
    
    std::vector<uint32_t> body_ids;
    for (size_t i = 0; i < body_count; ++i) {
        SphereShape shape(0.5f);
        Material material{0.5f, 0.3f, 1.0f};
        
        Vec3 position(pos_dist(gen), pos_dist(gen) + 50.0f, pos_dist(gen)); // Start above ground
        Vec3 velocity(vel_dist(gen), 0, vel_dist(gen));
        
        uint32_t body_id = world->create_dynamic_body_3d(position, Quaternion::identity(), shape, material);
        world->set_body_velocity_3d(body_id, velocity);
        body_ids.push_back(body_id);
    }
    
    // Run simulation for several steps
    const int simulation_steps = 100;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int step = 0; step < simulation_steps; ++step) {
        world->step(world_config.time_step);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    PhysicsStats stats = world->get_stats();
    Real average_step_time = duration / static_cast<Real>(simulation_steps);
    
    // Should maintain at least 30 FPS (33.33ms per frame) even with 5000 bodies
    EXPECT_LT(average_step_time, 33.0f);
    
    std::cout << "Stress test performance:\n";
    std::cout << "Bodies: " << body_count << "\n";
    std::cout << "Steps: " << simulation_steps << "\n";
    std::cout << "Total time: " << duration << "ms\n";
    std::cout << "Average step time: " << average_step_time << "ms\n";
    std::cout << "Active bodies: " << stats.active_bodies << "\n";
    std::cout << "Sleeping bodies: " << stats.sleeping_bodies << "\n";
    std::cout << "Average contacts: " << stats.collision_pairs << "\n";
}

// Energy conservation test
TEST_F(PhysicsEngineTest, EnergyConservation) {
    // Create a pendulum system to test energy conservation
    SphereShape shape(0.5f);
    Material material{0.1f, 0.95f, 1.0f}; // Low friction, high restitution
    
    // Create pendulum bob
    Vec3 initial_position(0, -5, 0);
    Vec3 initial_velocity(5, 0, 0); // Give it initial sideways velocity
    
    uint32_t bob_id = world->create_dynamic_body_3d(initial_position, Quaternion::identity(), shape, material);
    world->set_body_velocity_3d(bob_id, initial_velocity);
    
    // Create fixed anchor point
    uint32_t anchor_id = world->create_static_body_3d(Vec3(0, 0, 0), Quaternion::identity(), shape, material);
    
    // Add distance constraint to create pendulum
    DistanceConstraint constraint;
    constraint.local_anchor_a = Vec3::zero();
    constraint.local_anchor_b = Vec3::zero();
    constraint.rest_length = 5.0f;
    world->add_distance_constraint(anchor_id, bob_id, constraint);
    
    // Calculate initial energy
    Real initial_kinetic = 0.5f * 1.0f * initial_velocity.length_squared(); // m = 1kg
    Real initial_potential = 1.0f * 9.81f * (initial_position.y + 5.0f); // Relative to anchor
    Real initial_total_energy = initial_kinetic + initial_potential;
    
    // Simulate for several seconds
    const int steps = 300; // 5 seconds at 60fps
    for (int i = 0; i < steps; ++i) {
        world->step(world_config.time_step);
    }
    
    // Check final energy
    Vec3 final_velocity = world->get_body_velocity_3d(bob_id);
    Vec3 final_position = world->get_body_transform_3d(bob_id).position;
    
    Real final_kinetic = 0.5f * 1.0f * final_velocity.length_squared();
    Real final_potential = 1.0f * 9.81f * (final_position.y + 5.0f);
    Real final_total_energy = final_kinetic + final_potential;
    
    // Energy should be conserved within reasonable bounds (10% loss is acceptable due to numerical damping)
    Real energy_loss_ratio = std::abs(final_total_energy - initial_total_energy) / initial_total_energy;
    EXPECT_LT(energy_loss_ratio, 0.1f);
    
    std::cout << "Energy conservation test:\n";
    std::cout << "Initial energy: " << initial_total_energy << "J\n";
    std::cout << "Final energy: " << final_total_energy << "J\n";
    std::cout << "Energy loss ratio: " << (energy_loss_ratio * 100) << "%\n";
}

// Test sleeping system efficiency
TEST_F(PhysicsEngineTest, SleepingSystemEfficiency) {
    const size_t body_count = 100;
    
    // Create many bodies that should go to sleep
    std::vector<uint32_t> body_ids;
    for (size_t i = 0; i < body_count; ++i) {
        SphereShape shape(0.5f);
        Material material{0.8f, 0.2f, 1.0f}; // High friction, low restitution
        
        Vec3 position(0, static_cast<Real>(i) * 1.1f, 0); // Stack them vertically
        uint32_t body_id = world->create_dynamic_body_3d(position, Quaternion::identity(), shape, material);
        body_ids.push_back(body_id);
    }
    
    // Let them settle
    const int settling_steps = 600; // 10 seconds
    for (int i = 0; i < settling_steps; ++i) {
        world->step(world_config.time_step);
    }
    
    PhysicsStats stats = world->get_stats();
    
    // Most bodies should be sleeping by now
    Real sleeping_ratio = static_cast<Real>(stats.sleeping_bodies) / static_cast<Real>(stats.total_bodies);
    EXPECT_GT(sleeping_ratio, 0.8f); // At least 80% should be sleeping
    
    std::cout << "Sleeping system test:\n";
    std::cout << "Total bodies: " << stats.total_bodies << "\n";
    std::cout << "Active bodies: " << stats.active_bodies << "\n";
    std::cout << "Sleeping bodies: " << stats.sleeping_bodies << "\n";
    std::cout << "Sleeping ratio: " << (sleeping_ratio * 100) << "%\n";
}

// Benchmark different algorithms
TEST_F(PhysicsEngineTest, AlgorithmBenchmarks) {
    const size_t iterations = 1000;
    
    // Benchmark sphere-sphere collision detection
    SphereShape sphere_a(1.0f);
    SphereShape sphere_b(1.0f);
    Transform3D transform_a(Vec3(0, 0, 0), Quaternion::identity());
    Transform3D transform_b(Vec3(1.5f, 0, 0), Quaternion::identity());
    
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        ContactManifold manifold(1, 2);
        test_sphere_sphere_optimized(sphere_a, transform_a, sphere_b, transform_b, manifold);
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto sphere_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    Real sphere_avg_ns = static_cast<Real>(sphere_duration) / iterations;
    
    // Benchmark GJK collision detection
    BoxShape3D box_a(Vec3(1, 1, 1));
    BoxShape3D box_b(Vec3(1, 1, 1));
    
    start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        Simplex simplex;
        GJK::intersects(box_a, transform_a, box_b, transform_b, simplex);
    }
    end = std::chrono::high_resolution_clock::now();
    
    auto gjk_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    Real gjk_avg_ns = static_cast<Real>(gjk_duration) / iterations;
    
    std::cout << "Algorithm benchmarks (" << iterations << " iterations):\n";
    std::cout << "Sphere-sphere collision: " << sphere_avg_ns << " ns/test\n";
    std::cout << "GJK collision: " << gjk_avg_ns << " ns/test\n";
    std::cout << "GJK/Sphere ratio: " << (gjk_avg_ns / sphere_avg_ns) << "x\n";
    
    // GJK should be reasonably fast (less than 10x slower than sphere-sphere)
    EXPECT_LT(gjk_avg_ns / sphere_avg_ns, 10.0f);
}

} // namespace ecscope::physics::test

// Performance benchmark main function
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    std::cout << "=== ECScope Physics Engine Test Suite ===\n";
    std::cout << "Testing high-performance 2D/3D physics engine\n";
    std::cout << "Target: 10,000+ bodies at 60fps\n\n";
    
    int result = RUN_ALL_TESTS();
    
    std::cout << "\n=== Test Suite Complete ===\n";
    return result;
}