#include "../framework/ecscope_test_framework.hpp"

#ifdef ECSCOPE_ENABLE_PHYSICS
#include <ecscope/math.hpp>
#include <ecscope/math3d.hpp>
#include <ecscope/collision.hpp>
#include <ecscope/collision3d.hpp>
#include <ecscope/world3d.hpp>
#include <ecscope/advanced_physics_complete.hpp>

#ifdef ECSCOPE_ENABLE_SIMD
#include <ecscope/simd_math.hpp>
#include <ecscope/simd_math3d.hpp>
#endif

namespace ECScope::Testing {

class PhysicsSystemTest : public PhysicsTestFixture {
protected:
    void SetUp() override {
        PhysicsTestFixture::SetUp();
        
        // Initialize material database
        material_db_ = std::make_unique<Physics3D::MaterialDatabase>();
        setup_default_materials();
        
        // Initialize soft body system
        soft_body_system_ = std::make_unique<Physics3D::SoftBodySystem>();
        
        // Initialize fluid system
        fluid_system_ = std::make_unique<Physics3D::FluidSystem>();
        
        // Initialize constraint solver
        constraint_solver_ = std::make_unique<Physics3D::ConstraintSolver>();
    }

    void setup_default_materials() {
        // Steel
        Physics3D::MaterialProperties steel;
        steel.density = 7850.0f;        // kg/m³
        steel.elasticity = 200e9f;      // Pa
        steel.friction = 0.7f;
        steel.restitution = 0.3f;
        steel.thermal_conductivity = 50.0f;
        material_db_->add_material("steel", steel);
        
        // Rubber
        Physics3D::MaterialProperties rubber;
        rubber.density = 1200.0f;
        rubber.elasticity = 1e6f;
        rubber.friction = 0.9f;
        rubber.restitution = 0.8f;
        rubber.thermal_conductivity = 0.16f;
        material_db_->add_material("rubber", rubber);
        
        // Water
        Physics3D::MaterialProperties water;
        water.density = 1000.0f;
        water.viscosity = 1e-3f;
        water.surface_tension = 0.072f;
        water.thermal_conductivity = 0.6f;
        material_db_->add_material("water", water);
    }

protected:
    std::unique_ptr<Physics3D::MaterialDatabase> material_db_;
    std::unique_ptr<Physics3D::SoftBodySystem> soft_body_system_;
    std::unique_ptr<Physics3D::FluidSystem> fluid_system_;
    std::unique_ptr<Physics3D::ConstraintSolver> constraint_solver_;
};

// =============================================================================
// Basic Math Tests
// =============================================================================

TEST_F(PhysicsSystemTest, BasicMathOperations) {
    Vec3 v1(1.0f, 2.0f, 3.0f);
    Vec3 v2(4.0f, 5.0f, 6.0f);
    
    // Vector addition
    Vec3 sum = v1 + v2;
    EXPECT_FLOAT_EQ(sum.x, 5.0f);
    EXPECT_FLOAT_EQ(sum.y, 7.0f);
    EXPECT_FLOAT_EQ(sum.z, 9.0f);
    
    // Dot product
    float dot = v1.dot(v2);
    EXPECT_FLOAT_EQ(dot, 32.0f); // 1*4 + 2*5 + 3*6 = 32
    
    // Cross product
    Vec3 cross = v1.cross(v2);
    EXPECT_FLOAT_EQ(cross.x, -3.0f);
    EXPECT_FLOAT_EQ(cross.y, 6.0f);
    EXPECT_FLOAT_EQ(cross.z, -3.0f);
    
    // Length and normalization
    Vec3 v3(3.0f, 4.0f, 0.0f);
    EXPECT_FLOAT_EQ(v3.length(), 5.0f);
    
    Vec3 normalized = v3.normalized();
    EXPECT_FLOAT_EQ(normalized.length(), 1.0f);
    EXPECT_FLOAT_EQ(normalized.x, 0.6f);
    EXPECT_FLOAT_EQ(normalized.y, 0.8f);
}

TEST_F(PhysicsSystemTest, QuaternionOperations) {
    // Identity quaternion
    Quat identity;
    EXPECT_FLOAT_EQ(identity.w, 1.0f);
    EXPECT_FLOAT_EQ(identity.x, 0.0f);
    EXPECT_FLOAT_EQ(identity.y, 0.0f);
    EXPECT_FLOAT_EQ(identity.z, 0.0f);
    
    // Rotation around Y axis by 90 degrees
    Quat rot_y = Quat::from_axis_angle(Vec3(0, 1, 0), Math::PI / 2.0f);
    
    // Rotate vector (1, 0, 0) should become (0, 0, -1)
    Vec3 x_axis(1, 0, 0);
    Vec3 rotated = rot_y.rotate(x_axis);
    
    EXPECT_NEAR(rotated.x, 0.0f, 1e-6f);
    EXPECT_NEAR(rotated.y, 0.0f, 1e-6f);
    EXPECT_NEAR(rotated.z, -1.0f, 1e-6f);
    
    // Quaternion multiplication
    Quat rot_x = Quat::from_axis_angle(Vec3(1, 0, 0), Math::PI / 4.0f);
    Quat combined = rot_y * rot_x;
    
    // Combined rotation should be normalized
    EXPECT_NEAR(combined.length(), 1.0f, 1e-6f);
}

#ifdef ECSCOPE_ENABLE_SIMD
TEST_F(PhysicsSystemTest, SIMDMathOperations) {
    // Test SIMD vector operations
    alignas(16) float data1[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    alignas(16) float data2[4] = {5.0f, 6.0f, 7.0f, 8.0f};
    alignas(16) float result[4];
    
    SIMD::Vec4 v1 = SIMD::load(data1);
    SIMD::Vec4 v2 = SIMD::load(data2);
    
    // SIMD addition
    SIMD::Vec4 sum = SIMD::add(v1, v2);
    SIMD::store(result, sum);
    
    EXPECT_FLOAT_EQ(result[0], 6.0f);
    EXPECT_FLOAT_EQ(result[1], 8.0f);
    EXPECT_FLOAT_EQ(result[2], 10.0f);
    EXPECT_FLOAT_EQ(result[3], 12.0f);
    
    // SIMD dot product
    float dot_result = SIMD::dot3(v1, v2);
    EXPECT_FLOAT_EQ(dot_result, 38.0f); // 1*5 + 2*6 + 3*7 = 38
    
    // SIMD matrix multiplication
    alignas(16) float mat[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        1, 2, 3, 1
    };
    
    SIMD::Mat4 transform = SIMD::load_matrix(mat);
    SIMD::Vec4 point = SIMD::set(1.0f, 1.0f, 1.0f, 1.0f);
    SIMD::Vec4 transformed = SIMD::multiply(transform, point);
    
    SIMD::store(result, transformed);
    EXPECT_FLOAT_EQ(result[0], 2.0f); // 1 + 1*1
    EXPECT_FLOAT_EQ(result[1], 3.0f); // 1 + 2*1
    EXPECT_FLOAT_EQ(result[2], 4.0f); // 1 + 3*1
}
#endif

// =============================================================================
// Collision Detection Tests
// =============================================================================

TEST_F(PhysicsSystemTest, SphereCollisionDetection) {
    Physics3D::Sphere sphere1(Vec3(0, 0, 0), 1.0f);
    Physics3D::Sphere sphere2(Vec3(1.5f, 0, 0), 1.0f);
    Physics3D::Sphere sphere3(Vec3(3.0f, 0, 0), 1.0f);
    
    Physics3D::CollisionInfo collision;
    
    // Overlapping spheres
    EXPECT_TRUE(Physics3D::test_sphere_sphere(sphere1, sphere2, collision));
    EXPECT_GT(collision.penetration_depth, 0.0f);
    EXPECT_FLOAT_EQ(collision.normal.x, 1.0f);
    
    // Non-overlapping spheres
    EXPECT_FALSE(Physics3D::test_sphere_sphere(sphere1, sphere3, collision));
    
    // Touching spheres (edge case)
    Physics3D::Sphere sphere4(Vec3(2.0f, 0, 0), 1.0f);
    EXPECT_TRUE(Physics3D::test_sphere_sphere(sphere1, sphere4, collision));
    EXPECT_NEAR(collision.penetration_depth, 0.0f, 1e-6f);
}

TEST_F(PhysicsSystemTest, AABBCollisionDetection) {
    Physics3D::AABB aabb1(Vec3(-1, -1, -1), Vec3(1, 1, 1));
    Physics3D::AABB aabb2(Vec3(0.5f, 0.5f, 0.5f), Vec3(2, 2, 2));
    Physics3D::AABB aabb3(Vec3(2, 2, 2), Vec3(3, 3, 3));
    
    Physics3D::CollisionInfo collision;
    
    // Overlapping AABBs
    EXPECT_TRUE(Physics3D::test_aabb_aabb(aabb1, aabb2, collision));
    EXPECT_GT(collision.penetration_depth, 0.0f);
    
    // Non-overlapping AABBs
    EXPECT_FALSE(Physics3D::test_aabb_aabb(aabb1, aabb3, collision));
    
    // Test point inside AABB
    Vec3 inside_point(0.5f, 0.5f, 0.5f);
    EXPECT_TRUE(aabb1.contains(inside_point));
    
    Vec3 outside_point(2.0f, 2.0f, 2.0f);
    EXPECT_FALSE(aabb1.contains(outside_point));
}

TEST_F(PhysicsSystemTest, RaycastingTests) {
    Physics3D::Ray ray(Vec3(0, 0, -5), Vec3(0, 0, 1));
    Physics3D::Sphere sphere(Vec3(0, 0, 0), 1.0f);
    
    Physics3D::RaycastHit hit;
    
    // Ray hits sphere
    EXPECT_TRUE(Physics3D::raycast_sphere(ray, sphere, hit));
    EXPECT_FLOAT_EQ(hit.distance, 4.0f); // 5 - 1 = 4
    EXPECT_FLOAT_EQ(hit.point.z, -1.0f);
    EXPECT_FLOAT_EQ(hit.normal.z, -1.0f);
    
    // Ray misses sphere
    Physics3D::Ray miss_ray(Vec3(0, 2, -5), Vec3(0, 0, 1));
    EXPECT_FALSE(Physics3D::raycast_sphere(miss_ray, sphere, hit));
    
    // Ray starts inside sphere
    Physics3D::Ray inside_ray(Vec3(0, 0, 0), Vec3(0, 0, 1));
    EXPECT_TRUE(Physics3D::raycast_sphere(inside_ray, sphere, hit));
    EXPECT_FLOAT_EQ(hit.distance, 1.0f);
}

// =============================================================================
// Rigid Body Dynamics Tests
// =============================================================================

TEST_F(PhysicsSystemTest, RigidBodyBasicPhysics) {
    auto entity = create_physics_entity(Vec3(0, 10, 0), Vec3(0, 0, 0));
    
    auto& transform = world_->get_component<Transform3D>(entity);
    auto& rigidbody = world_->get_component<RigidBody3D>(entity);
    
    // Set mass and apply gravity
    rigidbody.mass = 1.0f;
    rigidbody.apply_force(Vec3(0, -9.81f, 0)); // Gravity
    
    // Simulate for 1 second at 60 FPS
    float dt = 1.0f / 60.0f;
    for (int i = 0; i < 60; ++i) {
        physics_world_->step(dt);
    }
    
    // After 1 second of free fall, should have fallen about 4.9 meters
    EXPECT_LT(transform.position.y, 6.0f);
    EXPECT_GT(transform.position.y, 4.0f);
    
    // Velocity should be approximately -9.81 m/s
    EXPECT_LT(rigidbody.velocity.y, -8.0f);
    EXPECT_GT(rigidbody.velocity.y, -11.0f);
}

TEST_F(PhysicsSystemTest, CollisionResponse) {
    // Create two entities that will collide
    auto entity1 = create_physics_entity(Vec3(-2, 0, 0), Vec3(5, 0, 0));
    auto entity2 = create_physics_entity(Vec3(2, 0, 0), Vec3(-3, 0, 0));
    
    auto& rb1 = world_->get_component<RigidBody3D>(entity1);
    auto& rb2 = world_->get_component<RigidBody3D>(entity2);
    
    rb1.mass = 2.0f;
    rb2.mass = 1.0f;
    
    // Add sphere colliders
    world_->add_component<Physics3D::SphereCollider>(entity1, 0.5f);
    world_->add_component<Physics3D::SphereCollider>(entity2, 0.5f);
    
    // Run simulation until collision
    float dt = 1.0f / 60.0f;
    for (int i = 0; i < 120; ++i) { // 2 seconds
        physics_world_->step(dt);
        
        // Check if collision occurred and velocities changed
        if (std::abs(rb1.velocity.x) < 4.0f || std::abs(rb2.velocity.x) < 2.0f) {
            break; // Collision likely occurred
        }
    }
    
    // After collision, momentum should be conserved (approximately)
    float total_momentum = rb1.mass * rb1.velocity.x + rb2.mass * rb2.velocity.x;
    float initial_momentum = 2.0f * 5.0f + 1.0f * (-3.0f); // 10 - 3 = 7
    
    EXPECT_NEAR(total_momentum, initial_momentum, 0.5f);
}

// =============================================================================
// Material System Tests
// =============================================================================

TEST_F(PhysicsSystemTest, MaterialProperties) {
    auto steel = material_db_->get_material("steel");
    EXPECT_NE(steel, nullptr);
    EXPECT_FLOAT_EQ(steel->density, 7850.0f);
    EXPECT_FLOAT_EQ(steel->friction, 0.7f);
    
    auto rubber = material_db_->get_material("rubber");
    EXPECT_NE(rubber, nullptr);
    EXPECT_FLOAT_EQ(rubber->restitution, 0.8f);
    
    // Test material interaction
    auto interaction = material_db_->get_interaction(*steel, *rubber);
    EXPECT_GT(interaction.friction, 0.0f);
    EXPECT_GT(interaction.restitution, 0.0f);
    
    // Combined friction should be geometric mean
    float expected_friction = std::sqrt(steel->friction * rubber->friction);
    EXPECT_NEAR(interaction.friction, expected_friction, 1e-6f);
}

TEST_F(PhysicsSystemTest, MaterialBehaviorSimulation) {
    // Create a bouncing ball with rubber material
    auto ball_entity = create_physics_entity(Vec3(0, 5, 0), Vec3(0, 0, 0));
    auto ground_entity = create_physics_entity(Vec3(0, -1, 0), Vec3(0, 0, 0));
    
    auto& ball_rb = world_->get_component<RigidBody3D>(ball_entity);
    auto& ground_rb = world_->get_component<RigidBody3D>(ground_entity);
    
    ball_rb.mass = 0.1f; // 100g ball
    ground_rb.mass = std::numeric_limits<float>::infinity(); // Immovable ground
    
    // Add colliders with materials
    world_->add_component<Physics3D::SphereCollider>(ball_entity, 0.1f);
    world_->add_component<Physics3D::BoxCollider>(ground_entity, Vec3(10, 0.1f, 10));
    
    world_->add_component<Physics3D::MaterialComponent>(ball_entity, "rubber");
    world_->add_component<Physics3D::MaterialComponent>(ground_entity, "steel");
    
    // Simulate bouncing
    float dt = 1.0f / 120.0f; // High frequency for accurate collision detection
    std::vector<float> bounce_heights;
    
    for (int i = 0; i < 1200; ++i) { // 10 seconds
        physics_world_->step(dt);
        
        auto& transform = world_->get_component<Transform3D>(ball_entity);
        
        // Record local maxima (bounce peaks)
        if (i > 10 && ball_rb.velocity.y > 0 && 
            i % 10 == 0 && transform.position.y > 0.2f) {
            bounce_heights.push_back(transform.position.y);
        }
    }
    
    // Should have multiple bounces with decreasing height
    EXPECT_GT(bounce_heights.size(), 3);
    
    if (bounce_heights.size() >= 2) {
        EXPECT_LT(bounce_heights[1], bounce_heights[0]); // Energy loss due to restitution < 1
    }
}

// =============================================================================
// Soft Body System Tests
// =============================================================================

TEST_F(PhysicsSystemTest, SoftBodyCreation) {
    // Create a simple soft body (cloth-like grid)
    Physics3D::SoftBodyDefinition cloth_def;
    cloth_def.width = 5;
    cloth_def.height = 5;
    cloth_def.spacing = 0.2f;
    cloth_def.mass_per_particle = 0.01f;
    cloth_def.stiffness = 0.8f;
    cloth_def.damping = 0.1f;
    
    auto cloth = soft_body_system_->create_cloth(cloth_def);
    EXPECT_NE(cloth, nullptr);
    
    // Verify particle count
    size_t expected_particles = cloth_def.width * cloth_def.height;
    EXPECT_EQ(cloth->get_particle_count(), expected_particles);
    
    // Verify constraint count (structural + shear + bend constraints)
    size_t expected_constraints = 
        2 * (cloth_def.width - 1) * cloth_def.height +     // Horizontal
        2 * cloth_def.width * (cloth_def.height - 1) +     // Vertical
        2 * (cloth_def.width - 1) * (cloth_def.height - 1) + // Shear
        (cloth_def.width - 2) * cloth_def.height +         // Bend horizontal
        cloth_def.width * (cloth_def.height - 2);          // Bend vertical
    
    EXPECT_GE(cloth->get_constraint_count(), expected_constraints * 0.8f); // Allow some variation
}

TEST_F(PhysicsSystemTest, SoftBodySimulation) {
    // Create a simple soft body rope
    Physics3D::SoftBodyDefinition rope_def;
    rope_def.width = 10;
    rope_def.height = 1;
    rope_def.spacing = 0.1f;
    rope_def.mass_per_particle = 0.01f;
    rope_def.stiffness = 0.9f;
    rope_def.damping = 0.05f;
    
    auto rope = soft_body_system_->create_rope(rope_def);
    
    // Pin first particle (fixed attachment)
    rope->pin_particle(0, Vec3(0, 5, 0));
    
    // Apply gravity and simulate
    float dt = 1.0f / 60.0f;
    Vec3 gravity(0, -9.81f, 0);
    
    for (int i = 0; i < 300; ++i) { // 5 seconds
        rope->apply_global_force(gravity);
        soft_body_system_->step(dt);
    }
    
    // Last particle should have settled below the first
    Vec3 first_pos = rope->get_particle_position(0);
    Vec3 last_pos = rope->get_particle_position(rope_def.width - 1);
    
    EXPECT_LT(last_pos.y, first_pos.y);
    EXPECT_GT(last_pos.y, first_pos.y - 2.0f); // Shouldn't fall too far due to constraints
}

// =============================================================================
// Fluid Simulation Tests
// =============================================================================

TEST_F(PhysicsSystemTest, FluidBasicProperties) {
    Physics3D::FluidDefinition water_def;
    water_def.particle_count = 1000;
    water_def.particle_radius = 0.02f;
    water_def.rest_density = 1000.0f;
    water_def.viscosity = 1e-3f;
    water_def.surface_tension = 0.072f;
    water_def.pressure_stiffness = 200.0f;
    
    auto fluid = fluid_system_->create_fluid(water_def);
    EXPECT_NE(fluid, nullptr);
    EXPECT_EQ(fluid->get_particle_count(), water_def.particle_count);
    
    // Initialize particles in a box
    Vec3 box_min(-1, 0, -1);
    Vec3 box_max(1, 2, 1);
    fluid->initialize_particles_in_box(box_min, box_max);
    
    // Verify all particles are within bounds
    for (size_t i = 0; i < fluid->get_particle_count(); ++i) {
        Vec3 pos = fluid->get_particle_position(i);
        EXPECT_GE(pos.x, box_min.x);
        EXPECT_LE(pos.x, box_max.x);
        EXPECT_GE(pos.y, box_min.y);
        EXPECT_LE(pos.y, box_max.y);
        EXPECT_GE(pos.z, box_min.z);
        EXPECT_LE(pos.z, box_max.z);
    }
}

TEST_F(PhysicsSystemTest, FluidSimulation) {
    // Create a small fluid simulation
    Physics3D::FluidDefinition fluid_def;
    fluid_def.particle_count = 500;
    fluid_def.particle_radius = 0.03f;
    fluid_def.rest_density = 1000.0f;
    fluid_def.viscosity = 5e-3f; // Slightly viscous
    fluid_def.surface_tension = 0.072f;
    fluid_def.pressure_stiffness = 100.0f;
    
    auto fluid = fluid_system_->create_fluid(fluid_def);
    
    // Initialize fluid in a box above ground
    fluid->initialize_particles_in_box(Vec3(-0.5f, 1, -0.5f), Vec3(0.5f, 2, 0.5f));
    
    // Add ground plane
    fluid_system_->add_boundary_plane(Vec3(0, 1, 0), Vec3(0, 0, 0)); // y = 0 plane
    
    // Simulate fluid falling and spreading
    float dt = 1.0f / 120.0f; // Small timestep for stability
    
    for (int i = 0; i < 600; ++i) { // 5 seconds
        fluid_system_->step(dt);
    }
    
    // Check that fluid has settled on the ground
    float lowest_y = std::numeric_limits<float>::max();
    float highest_y = std::numeric_limits<float>::lowest();
    
    for (size_t i = 0; i < fluid->get_particle_count(); ++i) {
        Vec3 pos = fluid->get_particle_position(i);
        lowest_y = std::min(lowest_y, pos.y);
        highest_y = std::max(highest_y, pos.y);
    }
    
    // Fluid should be resting on ground (y ≈ particle_radius)
    EXPECT_NEAR(lowest_y, fluid_def.particle_radius, 0.1f);
    
    // Fluid should have spread out (height should be less than initial)
    EXPECT_LT(highest_y - lowest_y, 1.0f); // Original height was 1.0, should be compressed
}

// =============================================================================
// Constraint System Tests
// =============================================================================

TEST_F(PhysicsSystemTest, DistanceConstraints) {
    auto entity1 = create_physics_entity(Vec3(0, 5, 0), Vec3(0, 0, 0));
    auto entity2 = create_physics_entity(Vec3(1, 5, 0), Vec3(0, 0, 0));
    
    auto& rb1 = world_->get_component<RigidBody3D>(entity1);
    auto& rb2 = world_->get_component<RigidBody3D>(entity2);
    
    rb1.mass = 1.0f;
    rb2.mass = 1.0f;
    
    // Create distance constraint (rope/rod)
    float constraint_distance = 1.0f;
    auto constraint = constraint_solver_->add_distance_constraint(
        entity1, entity2, constraint_distance, 0.8f // 80% stiffness
    );
    
    // Apply force to first entity
    rb1.apply_force(Vec3(10, 0, 0));
    
    // Simulate
    float dt = 1.0f / 60.0f;
    for (int i = 0; i < 120; ++i) { // 2 seconds
        physics_world_->step(dt);
        constraint_solver_->solve_constraints(dt);
    }
    
    // Entities should maintain approximately the constraint distance
    auto& transform1 = world_->get_component<Transform3D>(entity1);
    auto& transform2 = world_->get_component<Transform3D>(entity2);
    
    float actual_distance = (transform1.position - transform2.position).length();
    EXPECT_NEAR(actual_distance, constraint_distance, 0.2f);
}

TEST_F(PhysicsSystemTest, HingeConstraints) {
    auto entity1 = create_physics_entity(Vec3(0, 5, 0), Vec3(0, 0, 0));
    auto entity2 = create_physics_entity(Vec3(1, 5, 0), Vec3(0, 0, 0));
    
    auto& rb1 = world_->get_component<RigidBody3D>(entity1);
    auto& rb2 = world_->get_component<RigidBody3D>(entity2);
    
    rb1.mass = std::numeric_limits<float>::infinity(); // Fixed
    rb2.mass = 1.0f;
    
    // Create hinge constraint (door-like rotation around Y axis)
    Vec3 hinge_axis(0, 1, 0);
    auto constraint = constraint_solver_->add_hinge_constraint(
        entity1, entity2, Vec3(0.5f, 5, 0), hinge_axis
    );
    
    // Apply torque around hinge
    rb2.apply_torque(Vec3(0, 0, 5));
    
    // Simulate
    float dt = 1.0f / 60.0f;
    for (int i = 0; i < 180; ++i) { // 3 seconds
        physics_world_->step(dt);
        constraint_solver_->solve_constraints(dt);
    }
    
    // Second entity should have rotated around the hinge
    auto& transform2 = world_->get_component<Transform3D>(entity2);
    EXPECT_NE(transform2.rotation, Quat()); // Should have rotated from identity
    
    // Should maintain distance from hinge point
    Vec3 hinge_point(0.5f, 5, 0);
    float distance_to_hinge = (transform2.position - hinge_point).length();
    EXPECT_NEAR(distance_to_hinge, 0.5f, 0.1f);
}

// =============================================================================
// Performance Tests
// =============================================================================

TEST_F(PhysicsSystemTest, MathPerformance) {
    constexpr int iterations = 100000;
    std::vector<Vec3> vectors(iterations);
    
    // Initialize random vectors
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-10.0f, 10.0f);
    
    for (auto& v : vectors) {
        v = Vec3(dist(gen), dist(gen), dist(gen));
    }
    
    // Benchmark vector operations
    benchmark("VectorDotProduct", [&]() {
        volatile float sum = 0.0f;
        for (int i = 0; i < iterations - 1; ++i) {
            sum += vectors[i].dot(vectors[i + 1]);
        }
    });
    
    benchmark("VectorCrossProduct", [&]() {
        std::vector<Vec3> results(iterations - 1);
        for (int i = 0; i < iterations - 1; ++i) {
            results[i] = vectors[i].cross(vectors[i + 1]);
        }
    });
    
    benchmark("VectorNormalization", [&]() {
        std::vector<Vec3> results(iterations);
        for (int i = 0; i < iterations; ++i) {
            results[i] = vectors[i].normalized();
        }
    });
}

TEST_F(PhysicsSystemTest, CollisionPerformance) {
    constexpr int sphere_count = 1000;
    std::vector<Physics3D::Sphere> spheres;
    spheres.reserve(sphere_count);
    
    // Create random spheres
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> pos_dist(-10.0f, 10.0f);
    std::uniform_real_distribution<float> radius_dist(0.1f, 1.0f);
    
    for (int i = 0; i < sphere_count; ++i) {
        Vec3 pos(pos_dist(gen), pos_dist(gen), pos_dist(gen));
        float radius = radius_dist(gen);
        spheres.emplace_back(pos, radius);
    }
    
    // Benchmark collision detection
    benchmark("SphereCollisionDetection", [&]() {
        int collision_count = 0;
        Physics3D::CollisionInfo collision;
        
        for (int i = 0; i < sphere_count; ++i) {
            for (int j = i + 1; j < sphere_count; ++j) {
                if (Physics3D::test_sphere_sphere(spheres[i], spheres[j], collision)) {
                    collision_count++;
                }
            }
        }
        
        // Prevent optimization
        volatile int result = collision_count;
    });
}

TEST_F(PhysicsSystemTest, RigidBodySimulationPerformance) {
    constexpr int entity_count = 1000;
    std::vector<Entity> entities;
    entities.reserve(entity_count);
    
    // Create many rigid bodies
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> pos_dist(-10.0f, 10.0f);
    std::uniform_real_distribution<float> vel_dist(-5.0f, 5.0f);
    
    for (int i = 0; i < entity_count; ++i) {
        Vec3 pos(pos_dist(gen), pos_dist(gen), pos_dist(gen));
        Vec3 vel(vel_dist(gen), vel_dist(gen), vel_dist(gen));
        
        auto entity = create_physics_entity(pos, vel);
        auto& rb = world_->get_component<RigidBody3D>(entity);
        rb.mass = 1.0f;
        
        world_->add_component<Physics3D::SphereCollider>(entity, 0.5f);
        entities.push_back(entity);
    }
    
    // Benchmark physics simulation
    float dt = 1.0f / 60.0f;
    
    benchmark("RigidBodySimulation", [&]() {
        physics_world_->step(dt);
    }, 60); // 60 physics steps (1 second of simulation)
}

} // namespace ECScope::Testing

#endif // ECSCOPE_ENABLE_PHYSICS