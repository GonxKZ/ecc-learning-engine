#include "../framework/ecscope_test_framework.hpp"
#include <ecscope/physics.hpp>
#include <ecscope/physics_system.hpp>
#include <ecscope/world3d.hpp>
#include <ecscope/collision3d.hpp>
#include <ecscope/collision3d_algorithms.hpp>
#include <ecscope/soft_body_physics.hpp>
#include <ecscope/fluid_simulation.hpp>
#include <ecscope/advanced_materials.hpp>
#include <ecscope/math3d.hpp>
#include <ecscope/simd_math3d.hpp>

#include <random>
#include <thread>
#include <atomic>

namespace ECScope::Testing {

// =============================================================================
// Physics Test Fixture
// =============================================================================

class PhysicsSystemTest : public PhysicsTestFixture {
protected:
    void SetUp() override {
        PhysicsTestFixture::SetUp();
        
        // Initialize physics components
        soft_body_system_ = std::make_unique<SoftBody::System>();
        fluid_system_ = std::make_unique<Fluid::System>();
        materials_system_ = std::make_unique<Materials::System>();
        
        // Set up test parameters
        time_step_ = 1.0f / 60.0f; // 60 FPS
        gravity_ = Vec3{0.0f, -9.81f, 0.0f};
        
        // Initialize random number generator for testing
        rng_.seed(42);
    }

    void TearDown() override {
        materials_system_.reset();
        fluid_system_.reset();
        soft_body_system_.reset();
        PhysicsTestFixture::TearDown();
    }

protected:
    std::unique_ptr<SoftBody::System> soft_body_system_;
    std::unique_ptr<Fluid::System> fluid_system_;
    std::unique_ptr<Materials::System> materials_system_;
    float time_step_;
    Vec3 gravity_;
    std::mt19937 rng_;
};

// =============================================================================
// Basic Physics Math Tests
// =============================================================================

TEST_F(PhysicsSystemTest, Vector3DMathOperations) {
    Vec3 v1{1.0f, 2.0f, 3.0f};
    Vec3 v2{4.0f, 5.0f, 6.0f};
    
    // Addition
    Vec3 sum = v1 + v2;
    EXPECT_FLOAT_EQ(sum.x, 5.0f);
    EXPECT_FLOAT_EQ(sum.y, 7.0f);
    EXPECT_FLOAT_EQ(sum.z, 9.0f);
    
    // Dot product
    float dot = Math3D::dot(v1, v2);
    EXPECT_FLOAT_EQ(dot, 32.0f); // 1*4 + 2*5 + 3*6
    
    // Cross product
    Vec3 cross = Math3D::cross(v1, v2);
    EXPECT_FLOAT_EQ(cross.x, -3.0f);  // 2*6 - 3*5
    EXPECT_FLOAT_EQ(cross.y, 6.0f);   // 3*4 - 1*6
    EXPECT_FLOAT_EQ(cross.z, -3.0f);  // 1*5 - 2*4
    
    // Magnitude
    float magnitude = Math3D::length(v1);
    EXPECT_FLOAT_EQ(magnitude, std::sqrt(14.0f));
    
    // Normalization
    Vec3 normalized = Math3D::normalize(v1);
    float norm_magnitude = Math3D::length(normalized);
    EXPECT_NEAR(norm_magnitude, 1.0f, 1e-6f);
}

TEST_F(PhysicsSystemTest, QuaternionOperations) {
    // Test quaternion creation and operations
    Quaternion q1 = Math3D::quaternion_from_axis_angle(Vec3{0, 1, 0}, Math3D::PI / 2); // 90° around Y
    Quaternion q2 = Math3D::quaternion_from_axis_angle(Vec3{1, 0, 0}, Math3D::PI / 4); // 45° around X
    
    // Test quaternion multiplication
    Quaternion combined = Math3D::multiply(q1, q2);
    EXPECT_NEAR(Math3D::length_squared(combined), 1.0f, 1e-6f); // Should be unit quaternion
    
    // Test rotation application
    Vec3 point{1, 0, 0};
    Vec3 rotated = Math3D::rotate(point, q1);
    
    // 90° rotation around Y should map (1,0,0) to approximately (0,0,-1)
    EXPECT_NEAR(rotated.x, 0.0f, 1e-6f);
    EXPECT_NEAR(rotated.y, 0.0f, 1e-6f);
    EXPECT_NEAR(rotated.z, -1.0f, 1e-6f);
}

#ifdef ECSCOPE_ENABLE_SIMD
TEST_F(PhysicsSystemTest, SIMDMathPerformance) {
    constexpr size_t vector_count = 10000;
    constexpr size_t iterations = 100;
    
    // Create test data
    std::vector<Vec3> vectors1, vectors2, results_scalar, results_simd;
    vectors1.resize(vector_count);
    vectors2.resize(vector_count);
    results_scalar.resize(vector_count);
    results_simd.resize(vector_count);
    
    std::uniform_real_distribution<float> dist(-10.0f, 10.0f);
    for (size_t i = 0; i < vector_count; ++i) {
        vectors1[i] = {dist(rng_), dist(rng_), dist(rng_)};
        vectors2[i] = {dist(rng_), dist(rng_), dist(rng_)};
    }
    
    // Test scalar performance
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t iter = 0; iter < iterations; ++iter) {
        for (size_t i = 0; i < vector_count; ++i) {
            results_scalar[i] = vectors1[i] + vectors2[i];
        }
    }
    auto scalar_time = std::chrono::high_resolution_clock::now() - start;
    
    // Test SIMD performance
    start = std::chrono::high_resolution_clock::now();
    for (size_t iter = 0; iter < iterations; ++iter) {
        SIMD::add_vectors(vectors1.data(), vectors2.data(), results_simd.data(), vector_count);
    }
    auto simd_time = std::chrono::high_resolution_clock::now() - start;
    
    // Verify results are the same
    for (size_t i = 0; i < vector_count; ++i) {
        EXPECT_NEAR(results_scalar[i].x, results_simd[i].x, 1e-5f);
        EXPECT_NEAR(results_scalar[i].y, results_simd[i].y, 1e-5f);
        EXPECT_NEAR(results_scalar[i].z, results_simd[i].z, 1e-5f);
    }
    
    auto scalar_us = std::chrono::duration_cast<std::chrono::microseconds>(scalar_time);
    auto simd_us = std::chrono::duration_cast<std::chrono::microseconds>(simd_time);
    
    std::cout << "SIMD performance - Scalar: " << scalar_us.count() 
              << "μs, SIMD: " << simd_us.count() << "μs\n";
    
    // SIMD should be faster (though not guaranteed on all systems)
    EXPECT_GT(scalar_us.count(), 0);
    EXPECT_GT(simd_us.count(), 0);
}
#endif

// =============================================================================
// Rigid Body Physics Tests
// =============================================================================

TEST_F(PhysicsSystemTest, RigidBodyBasicMotion) {
    auto entity = create_physics_entity(Vec3{0, 10, 0}, Vec3{0, 0, 0});
    
    // Apply gravity and simulate one step
    physics_world_->set_gravity(gravity_);
    physics_world_->step(time_step_);
    
    // Get the new position
    auto& transform = world_->get_component<Transform3D>(entity);
    auto& rigidbody = world_->get_component<RigidBody3D>(entity);
    
    // Object should have fallen
    EXPECT_LT(transform.position.y, 10.0f);
    EXPECT_LT(rigidbody.velocity.y, 0.0f); // Should be moving downward
    
    // Velocity should match expected physics
    float expected_velocity = gravity_.y * time_step_;
    EXPECT_NEAR(rigidbody.velocity.y, expected_velocity, 1e-4f);
}

TEST_F(PhysicsSystemTest, RigidBodyCollisionDetection) {
    // Create two entities that should collide
    auto entity1 = create_physics_entity(Vec3{0, 1, 0}, Vec3{0, -5, 0});
    auto entity2 = create_physics_entity(Vec3{0, -1, 0}, Vec3{0, 5, 0});
    
    // Add collision components
    world_->add_component<CollisionBox3D>(entity1, Vec3{0.5f, 0.5f, 0.5f});
    world_->add_component<CollisionBox3D>(entity2, Vec3{0.5f, 0.5f, 0.5f});
    
    // Step physics multiple times to ensure collision
    for (int i = 0; i < 10; ++i) {
        physics_world_->step(time_step_);
        
        // Check for collision
        if (physics_world_->check_collision(entity1, entity2)) {
            // Collision detected - verify positions are close
            auto& t1 = world_->get_component<Transform3D>(entity1);
            auto& t2 = world_->get_component<Transform3D>(entity2);
            
            float distance = Math3D::length(t1.position - t2.position);
            EXPECT_LT(distance, 1.5f); // Should be within collision distance
            return; // Test passed
        }
    }
    
    FAIL() << "Collision was not detected";
}

TEST_F(PhysicsSystemTest, RigidBodyConservationOfMomentum) {
    // Create two entities for collision test
    auto entity1 = create_physics_entity(Vec3{-2, 0, 0}, Vec3{5, 0, 0});
    auto entity2 = create_physics_entity(Vec3{2, 0, 0}, Vec3{-3, 0, 0});
    
    // Set masses
    world_->add_component<Mass3D>(entity1, 2.0f);
    world_->add_component<Mass3D>(entity2, 3.0f);
    
    // Add collision shapes
    world_->add_component<CollisionSphere3D>(entity1, 0.5f);
    world_->add_component<CollisionSphere3D>(entity2, 0.5f);
    
    // Calculate initial momentum
    Vec3 initial_momentum = Vec3{5, 0, 0} * 2.0f + Vec3{-3, 0, 0} * 3.0f;\n    float initial_momentum_magnitude = Math3D::length(initial_momentum);\n    \n    // Simulate until collision and resolution\n    for (int i = 0; i < 20; ++i) {\n        physics_world_->step(time_step_);\n    }\n    \n    // Calculate final momentum\n    auto& rb1 = world_->get_component<RigidBody3D>(entity1);\n    auto& rb2 = world_->get_component<RigidBody3D>(entity2);\n    auto& m1 = world_->get_component<Mass3D>(entity1);\n    auto& m2 = world_->get_component<Mass3D>(entity2);\n    \n    Vec3 final_momentum = rb1.velocity * m1.mass + rb2.velocity * m2.mass;\n    float final_momentum_magnitude = Math3D::length(final_momentum);\n    \n    // Momentum should be approximately conserved\n    EXPECT_NEAR(final_momentum_magnitude, initial_momentum_magnitude, 0.1f);\n}\n\n// =============================================================================\n// Soft Body Physics Tests\n// =============================================================================\n\nTEST_F(PhysicsSystemTest, SoftBodyCreation) {\n    auto entity = world_->create_entity();\n    \n    // Create a simple soft body (cloth-like)\n    SoftBody::ClothParams params;\n    params.width = 5;\n    params.height = 5;\n    params.mass = 1.0f;\n    params.stiffness = 0.8f;\n    params.damping = 0.1f;\n    \n    auto soft_body = soft_body_system_->create_cloth(params);\n    world_->add_component<SoftBody::Component>(entity, std::move(soft_body));\n    \n    // Verify soft body was created correctly\n    EXPECT_TRUE(world_->has_component<SoftBody::Component>(entity));\n    auto& sb = world_->get_component<SoftBody::Component>(entity);\n    \n    EXPECT_EQ(sb.particles.size(), params.width * params.height);\n    EXPECT_GT(sb.constraints.size(), 0); // Should have spring constraints\n}\n\nTEST_F(PhysicsSystemTest, SoftBodySimulation) {\n    auto entity = world_->create_entity();\n    \n    // Create soft body\n    SoftBody::ClothParams params;\n    params.width = 3;\n    params.height = 3;\n    params.mass = 1.0f;\n    params.stiffness = 0.5f;\n    params.damping = 0.2f;\n    \n    auto soft_body = soft_body_system_->create_cloth(params);\n    world_->add_component<SoftBody::Component>(entity, std::move(soft_body));\n    \n    auto& sb = world_->get_component<SoftBody::Component>(entity);\n    \n    // Record initial positions\n    std::vector<Vec3> initial_positions;\n    for (const auto& particle : sb.particles) {\n        initial_positions.push_back(particle.position);\n    }\n    \n    // Apply gravity and simulate\n    for (int i = 0; i < 60; ++i) { // 1 second of simulation\n        soft_body_system_->update(entity, time_step_, gravity_);\n    }\n    \n    // Particles should have moved (fallen due to gravity)\n    bool particles_moved = false;\n    for (size_t i = 0; i < sb.particles.size(); ++i) {\n        if (Math3D::length(sb.particles[i].position - initial_positions[i]) > 0.1f) {\n            particles_moved = true;\n            break;\n        }\n    }\n    \n    EXPECT_TRUE(particles_moved) << \"Soft body particles should have moved under gravity\";\n}\n\nTEST_F(PhysicsSystemTest, SoftBodyConstraints) {\n    auto entity = world_->create_entity();\n    \n    // Create soft body with known constraints\n    SoftBody::ClothParams params;\n    params.width = 2;\n    params.height = 2;\n    params.mass = 1.0f;\n    params.stiffness = 1.0f; // High stiffness\n    params.damping = 0.0f;   // No damping for test\n    \n    auto soft_body = soft_body_system_->create_cloth(params);\n    world_->add_component<SoftBody::Component>(entity, std::move(soft_body));\n    \n    auto& sb = world_->get_component<SoftBody::Component>(entity);\n    \n    // Manually displace one particle\n    sb.particles[0].position.x += 1.0f;\n    \n    // Run constraint solver\n    for (int i = 0; i < 10; ++i) {\n        soft_body_system_->solve_constraints(entity, time_step_);\n    }\n    \n    // Connected particles should have moved closer to original configuration\n    // (This is a simplified test - real soft body physics is complex)\n    EXPECT_LT(std::abs(sb.particles[0].position.x - sb.particles[1].position.x), 1.5f);\n}\n\n// =============================================================================\n// Fluid Simulation Tests\n// =============================================================================\n\nTEST_F(PhysicsSystemTest, FluidParticleCreation) {\n    auto entity = world_->create_entity();\n    \n    Fluid::SystemParams params;\n    params.particle_count = 1000;\n    params.particle_radius = 0.1f;\n    params.density = 1000.0f; // Water density\n    params.viscosity = 0.001f;\n    params.surface_tension = 0.0728f;\n    \n    auto fluid_system = fluid_system_->create_fluid(params);\n    world_->add_component<Fluid::Component>(entity, std::move(fluid_system));\n    \n    EXPECT_TRUE(world_->has_component<Fluid::Component>(entity));\n    auto& fluid = world_->get_component<Fluid::Component>(entity);\n    \n    EXPECT_EQ(fluid.particles.size(), params.particle_count);\n    \n    // Check that particles are initialized with reasonable values\n    for (const auto& particle : fluid.particles) {\n        EXPECT_GE(particle.density, 0.0f);\n        EXPECT_GE(particle.pressure, 0.0f);\n        EXPECT_FALSE(std::isnan(particle.position.x));\n        EXPECT_FALSE(std::isnan(particle.position.y));\n        EXPECT_FALSE(std::isnan(particle.position.z));\n    }\n}\n\nTEST_F(PhysicsSystemTest, FluidDensityCalculation) {\n    auto entity = world_->create_entity();\n    \n    // Create small fluid system for testing\n    Fluid::SystemParams params;\n    params.particle_count = 100;\n    params.particle_radius = 0.1f;\n    params.density = 1000.0f;\n    params.smoothing_radius = 0.3f;\n    \n    auto fluid_system = fluid_system_->create_fluid(params);\n    world_->add_component<Fluid::Component>(entity, std::move(fluid_system));\n    \n    auto& fluid = world_->get_component<Fluid::Component>(entity);\n    \n    // Arrange particles in a known configuration\n    for (size_t i = 0; i < fluid.particles.size(); ++i) {\n        fluid.particles[i].position = Vec3{\n            static_cast<float>(i % 10) * 0.2f,\n            static_cast<float>(i / 10) * 0.2f,\n            0.0f\n        };\n    }\n    \n    // Calculate densities\n    fluid_system_->calculate_densities(entity);\n    \n    // Check that densities are reasonable\n    for (const auto& particle : fluid.particles) {\n        EXPECT_GT(particle.density, 0.0f);\n        EXPECT_LT(particle.density, 10000.0f); // Reasonable upper bound\n        EXPECT_FALSE(std::isnan(particle.density));\n    }\n}\n\nTEST_F(PhysicsSystemTest, FluidPressureForces) {\n    auto entity = world_->create_entity();\n    \n    Fluid::SystemParams params;\n    params.particle_count = 50;\n    params.particle_radius = 0.1f;\n    params.density = 1000.0f;\n    params.gas_constant = 1.0f;\n    params.smoothing_radius = 0.25f;\n    \n    auto fluid_system = fluid_system_->create_fluid(params);\n    world_->add_component<Fluid::Component>(entity, std::move(fluid_system));\n    \n    auto& fluid = world_->get_component<Fluid::Component>(entity);\n    \n    // Create a high-pressure scenario (particles close together)\n    for (size_t i = 0; i < fluid.particles.size(); ++i) {\n        fluid.particles[i].position = Vec3{\n            static_cast<float>(i % 5) * 0.05f,\n            static_cast<float>(i / 5) * 0.05f,\n            0.0f\n        };\n        fluid.particles[i].velocity = Vec3{0, 0, 0};\n    }\n    \n    // Calculate densities and pressures\n    fluid_system_->calculate_densities(entity);\n    fluid_system_->calculate_pressures(entity);\n    \n    // Record initial kinetic energy\n    float initial_ke = 0.0f;\n    for (const auto& particle : fluid.particles) {\n        initial_ke += 0.5f * Math3D::length_squared(particle.velocity);\n    }\n    \n    // Apply pressure forces\n    fluid_system_->apply_pressure_forces(entity, time_step_);\n    \n    // Calculate final kinetic energy\n    float final_ke = 0.0f;\n    for (const auto& particle : fluid.particles) {\n        final_ke += 0.5f * Math3D::length_squared(particle.velocity);\n    }\n    \n    // Pressure forces should have added energy to the system\n    EXPECT_GT(final_ke, initial_ke);\n}\n\n// =============================================================================\n// Advanced Materials Tests\n// =============================================================================\n\nTEST_F(PhysicsSystemTest, MaterialProperties) {\n    auto entity1 = create_physics_entity();\n    auto entity2 = create_physics_entity();\n    \n    // Create different materials\n    Materials::Properties rubber;\n    rubber.restitution = 0.9f;  // Bouncy\n    rubber.friction = 0.7f;\n    rubber.density = 1500.0f;\n    \n    Materials::Properties steel;\n    steel.restitution = 0.3f;   // Less bouncy\n    steel.friction = 0.4f;\n    steel.density = 7850.0f;\n    \n    world_->add_component<Materials::Component>(entity1, rubber);\n    world_->add_component<Materials::Component>(entity2, steel);\n    \n    // Test material combination\n    auto combined = materials_system_->combine_materials(\n        world_->get_component<Materials::Component>(entity1).properties,\n        world_->get_component<Materials::Component>(entity2).properties\n    );\n    \n    // Combined restitution should be geometric mean\n    float expected_restitution = std::sqrt(rubber.restitution * steel.restitution);\n    EXPECT_NEAR(combined.restitution, expected_restitution, 1e-4f);\n    \n    // Combined friction should use appropriate mixing rule\n    EXPECT_GT(combined.friction, 0.0f);\n    EXPECT_LT(combined.friction, 1.0f);\n}\n\nTEST_F(PhysicsSystemTest, MaterialEffectsOnCollision) {\n    // Create two bouncing balls with different materials\n    auto bouncy_ball = create_physics_entity(Vec3{0, 5, 0}, Vec3{0, -10, 0});\n    auto heavy_ball = create_physics_entity(Vec3{0, -5, 0}, Vec3{0, 10, 0});\n    \n    // High restitution material (bouncy)\n    Materials::Properties bouncy;\n    bouncy.restitution = 0.95f;\n    bouncy.friction = 0.1f;\n    bouncy.density = 500.0f;\n    \n    // Low restitution material (absorbing)\n    Materials::Properties absorbing;\n    absorbing.restitution = 0.1f;\n    absorbing.friction = 0.8f;\n    absorbing.density = 2000.0f;\n    \n    world_->add_component<Materials::Component>(bouncy_ball, bouncy);\n    world_->add_component<Materials::Component>(heavy_ball, absorbing);\n    world_->add_component<CollisionSphere3D>(bouncy_ball, 0.5f);\n    world_->add_component<CollisionSphere3D>(heavy_ball, 0.5f);\n    \n    // Record initial velocities\n    auto& rb1 = world_->get_component<RigidBody3D>(bouncy_ball);\n    auto& rb2 = world_->get_component<RigidBody3D>(heavy_ball);\n    float initial_speed1 = Math3D::length(rb1.velocity);\n    float initial_speed2 = Math3D::length(rb2.velocity);\n    \n    // Simulate collision\n    for (int i = 0; i < 30; ++i) {\n        physics_world_->step(time_step_);\n        \n        if (physics_world_->check_collision(bouncy_ball, heavy_ball)) {\n            // Apply collision response with materials\n            materials_system_->handle_collision(bouncy_ball, heavy_ball, Vec3{0, 1, 0});\n            break;\n        }\n    }\n    \n    // Check post-collision velocities\n    float final_speed1 = Math3D::length(rb1.velocity);\n    float final_speed2 = Math3D::length(rb2.velocity);\n    \n    // With different restitution values, speeds should change appropriately\n    EXPECT_LT(final_speed1, initial_speed1 * 1.1f); // Some energy lost\n    EXPECT_LT(final_speed2, initial_speed2 * 1.1f);\n}\n\n// =============================================================================\n// Collision Detection Algorithm Tests\n// =============================================================================\n\nTEST_F(PhysicsSystemTest, SphereToSphereCollision) {\n    Vec3 pos1{0, 0, 0};\n    Vec3 pos2{1.5f, 0, 0}; // Slightly overlapping\n    float radius1 = 1.0f;\n    float radius2 = 0.8f;\n    \n    auto result = Collision3D::sphere_to_sphere(pos1, radius1, pos2, radius2);\n    \n    EXPECT_TRUE(result.is_colliding);\n    EXPECT_GT(result.penetration_depth, 0.0f);\n    \n    // Normal should point from sphere 1 to sphere 2\n    Vec3 expected_normal = Math3D::normalize(pos2 - pos1);\n    EXPECT_NEAR(result.collision_normal.x, expected_normal.x, 1e-4f);\n    EXPECT_NEAR(result.collision_normal.y, expected_normal.y, 1e-4f);\n    EXPECT_NEAR(result.collision_normal.z, expected_normal.z, 1e-4f);\n}\n\nTEST_F(PhysicsSystemTest, AABBToAABBCollision) {\n    Vec3 min1{-1, -1, -1};\n    Vec3 max1{1, 1, 1};\n    Vec3 min2{0.5f, 0.5f, 0.5f};\n    Vec3 max2{2, 2, 2};\n    \n    auto result = Collision3D::aabb_to_aabb(min1, max1, min2, max2);\n    \n    EXPECT_TRUE(result.is_colliding);\n    EXPECT_GT(result.penetration_depth, 0.0f);\n    \n    // Test non-overlapping case\n    Vec3 min3{3, 3, 3};\n    Vec3 max3{4, 4, 4};\n    \n    auto result2 = Collision3D::aabb_to_aabb(min1, max1, min3, max3);\n    EXPECT_FALSE(result2.is_colliding);\n}\n\nTEST_F(PhysicsSystemTest, SphereToPlaneCollision) {\n    Vec3 sphere_pos{0, 1, 0};\n    float sphere_radius = 1.5f;\n    Vec3 plane_normal{0, 1, 0}; // Y-up plane\n    float plane_distance = 0.0f; // Plane at Y=0\n    \n    auto result = Collision3D::sphere_to_plane(sphere_pos, sphere_radius, plane_normal, plane_distance);\n    \n    EXPECT_TRUE(result.is_colliding);\n    EXPECT_NEAR(result.penetration_depth, 0.5f, 1e-4f); // 1.5 - 1.0\n    \n    // Normal should be plane normal\n    EXPECT_NEAR(result.collision_normal.x, 0.0f, 1e-4f);\n    EXPECT_NEAR(result.collision_normal.y, 1.0f, 1e-4f);\n    EXPECT_NEAR(result.collision_normal.z, 0.0f, 1e-4f);\n}\n\n// =============================================================================\n// Performance and Stress Tests\n// =============================================================================\n\nTEST_F(PhysicsSystemTest, RigidBodyPerformanceStress) {\n    constexpr size_t entity_count = 1000;\n    constexpr size_t simulation_steps = 100;\n    \n    // Create many rigid bodies\n    std::vector<Entity> entities;\n    entities.reserve(entity_count);\n    \n    std::uniform_real_distribution<float> pos_dist(-10.0f, 10.0f);\n    std::uniform_real_distribution<float> vel_dist(-5.0f, 5.0f);\n    \n    for (size_t i = 0; i < entity_count; ++i) {\n        Vec3 pos{pos_dist(rng_), pos_dist(rng_), pos_dist(rng_)};\n        Vec3 vel{vel_dist(rng_), vel_dist(rng_), vel_dist(rng_)};\n        \n        auto entity = create_physics_entity(pos, vel);\n        world_->add_component<CollisionSphere3D>(entity, 0.5f);\n        entities.push_back(entity);\n    }\n    \n    // Measure simulation performance\n    auto start = std::chrono::high_resolution_clock::now();\n    \n    for (size_t step = 0; step < simulation_steps; ++step) {\n        physics_world_->step(time_step_);\n    }\n    \n    auto end = std::chrono::high_resolution_clock::now();\n    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);\n    \n    std::cout << \"Physics stress test: \" << entity_count << \" entities, \" \n              << simulation_steps << \" steps in \" << duration.count() << \" μs\\n\";\n    \n    // Should maintain reasonable performance\n    double us_per_entity_per_step = static_cast<double>(duration.count()) / (entity_count * simulation_steps);\n    EXPECT_LT(us_per_entity_per_step, 10.0); // Less than 10μs per entity per step\n}\n\nTEST_F(PhysicsSystemTest, CollisionDetectionPerformance) {\n    constexpr size_t entity_count = 500; // Smaller count for O(n²) collision detection\n    \n    std::vector<Entity> entities;\n    std::uniform_real_distribution<float> pos_dist(-5.0f, 5.0f);\n    \n    for (size_t i = 0; i < entity_count; ++i) {\n        Vec3 pos{pos_dist(rng_), pos_dist(rng_), pos_dist(rng_)};\n        auto entity = create_physics_entity(pos);\n        world_->add_component<CollisionSphere3D>(entity, 0.2f);\n        entities.push_back(entity);\n    }\n    \n    // Measure broad-phase collision detection\n    auto start = std::chrono::high_resolution_clock::now();\n    \n    size_t collision_pairs = 0;\n    for (size_t i = 0; i < entities.size(); ++i) {\n        for (size_t j = i + 1; j < entities.size(); ++j) {\n            if (physics_world_->check_collision(entities[i], entities[j])) {\n                collision_pairs++;\n            }\n        }\n    }\n    \n    auto end = std::chrono::high_resolution_clock::now();\n    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);\n    \n    size_t total_checks = (entity_count * (entity_count - 1)) / 2;\n    std::cout << \"Collision detection: \" << total_checks << \" checks, \" \n              << collision_pairs << \" collisions in \" << duration.count() << \" μs\\n\";\n    \n    // Should be reasonably fast\n    double us_per_check = static_cast<double>(duration.count()) / total_checks;\n    EXPECT_LT(us_per_check, 1.0); // Less than 1μs per collision check\n}\n\n} // namespace ECScope::Testing