#include <gtest/gtest.h>
#include <chrono>
#include <random>
#include <algorithm>

// Include all physics headers
#include "../include/ecscope/physics/physics_math.hpp"
#include "../include/ecscope/physics/rigid_body.hpp"
#include "../include/ecscope/physics/collision_detection.hpp"
#include "../include/ecscope/physics/narrow_phase.hpp"
#include "../include/ecscope/physics/constraints.hpp"
#include "../include/ecscope/physics/materials.hpp"
#include "../include/ecscope/physics/physics_world.hpp"
#include "../include/ecscope/physics/advanced.hpp"

using namespace ecscope::physics;

class PhysicsMathTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup for math tests
    }
};

TEST_F(PhysicsMathTest, Vec2Operations) {
    Vec2 a(3.0f, 4.0f);
    Vec2 b(1.0f, 2.0f);
    
    // Basic operations
    Vec2 sum = a + b;
    EXPECT_FLOAT_EQ(sum.x, 4.0f);
    EXPECT_FLOAT_EQ(sum.y, 6.0f);
    
    Vec2 diff = a - b;
    EXPECT_FLOAT_EQ(diff.x, 2.0f);
    EXPECT_FLOAT_EQ(diff.y, 2.0f);
    
    Vec2 scaled = a * 2.0f;
    EXPECT_FLOAT_EQ(scaled.x, 6.0f);
    EXPECT_FLOAT_EQ(scaled.y, 8.0f);
    
    // Dot product
    Real dot = a.dot(b);
    EXPECT_FLOAT_EQ(dot, 11.0f);
    
    // Cross product (2D)
    Real cross = a.cross(b);
    EXPECT_FLOAT_EQ(cross, 2.0f);
    
    // Length
    EXPECT_FLOAT_EQ(a.length(), 5.0f);
    EXPECT_FLOAT_EQ(a.length_squared(), 25.0f);
    
    // Normalization
    Vec2 normalized = a.normalized();
    EXPECT_FLOAT_EQ(normalized.length(), 1.0f);
    EXPECT_FLOAT_EQ(normalized.x, 0.6f);
    EXPECT_FLOAT_EQ(normalized.y, 0.8f);
    
    // Perpendicular
    Vec2 perp = a.perpendicular();
    EXPECT_FLOAT_EQ(perp.x, -4.0f);
    EXPECT_FLOAT_EQ(perp.y, 3.0f);
    EXPECT_FLOAT_EQ(a.dot(perp), 0.0f);  // Should be orthogonal
}

TEST_F(PhysicsMathTest, Vec3Operations) {
    Vec3 a(1.0f, 2.0f, 3.0f);
    Vec3 b(4.0f, 5.0f, 6.0f);
    
    // Basic operations
    Vec3 sum = a + b;
    EXPECT_FLOAT_EQ(sum.x, 5.0f);
    EXPECT_FLOAT_EQ(sum.y, 7.0f);
    EXPECT_FLOAT_EQ(sum.z, 9.0f);
    
    // Dot product
    Real dot = a.dot(b);
    EXPECT_FLOAT_EQ(dot, 32.0f);
    
    // Cross product
    Vec3 cross = a.cross(b);
    EXPECT_FLOAT_EQ(cross.x, -3.0f);
    EXPECT_FLOAT_EQ(cross.y, 6.0f);
    EXPECT_FLOAT_EQ(cross.z, -3.0f);
    
    // Verify cross product properties
    EXPECT_NEAR(cross.dot(a), 0.0f, 1e-6f);
    EXPECT_NEAR(cross.dot(b), 0.0f, 1e-6f);
    
    // Length
    Real expected_length = std::sqrt(14.0f);
    EXPECT_FLOAT_EQ(a.length(), expected_length);
}

TEST_F(PhysicsMathTest, QuaternionOperations) {
    // Identity quaternion
    Quaternion identity = Quaternion::identity();
    EXPECT_FLOAT_EQ(identity.x, 0.0f);
    EXPECT_FLOAT_EQ(identity.y, 0.0f);
    EXPECT_FLOAT_EQ(identity.z, 0.0f);
    EXPECT_FLOAT_EQ(identity.w, 1.0f);
    
    // Rotation around Y axis by 90 degrees
    Quaternion rot_y = Quaternion::from_axis_angle(Vec3::unit_y(), PI / 2);
    
    // Rotate X axis vector should give Z axis vector
    Vec3 x_rotated = rot_y.rotate_vector(Vec3::unit_x());
    EXPECT_NEAR(x_rotated.x, 0.0f, 1e-6f);
    EXPECT_NEAR(x_rotated.y, 0.0f, 1e-6f);
    EXPECT_NEAR(x_rotated.z, 1.0f, 1e-6f);
    
    // Quaternion multiplication
    Quaternion rot_x = Quaternion::from_axis_angle(Vec3::unit_x(), PI / 4);
    Quaternion combined = rot_y * rot_x;
    
    // Combined rotation should be normalized
    Quaternion normalized = combined.normalized();
    Real length_sq = normalized.x * normalized.x + normalized.y * normalized.y + 
                    normalized.z * normalized.z + normalized.w * normalized.w;
    EXPECT_NEAR(length_sq, 1.0f, 1e-6f);
}

TEST_F(PhysicsMathTest, Matrix3Operations) {
    Mat3 identity = Mat3::identity();
    
    // Identity matrix properties
    Vec3 test_vec(1.0f, 2.0f, 3.0f);
    Vec3 result = identity * test_vec;
    EXPECT_FLOAT_EQ(result.x, test_vec.x);
    EXPECT_FLOAT_EQ(result.y, test_vec.y);
    EXPECT_FLOAT_EQ(result.z, test_vec.z);
    
    // Matrix from quaternion
    Quaternion rot = Quaternion::from_axis_angle(Vec3::unit_z(), PI / 2);
    Mat3 rot_matrix = Mat3::from_quaternion(rot);
    
    Vec3 x_axis = rot_matrix * Vec3::unit_x();
    EXPECT_NEAR(x_axis.x, 0.0f, 1e-6f);
    EXPECT_NEAR(x_axis.y, 1.0f, 1e-6f);
    EXPECT_NEAR(x_axis.z, 0.0f, 1e-6f);
    
    // Matrix inverse
    Mat3 inverse = rot_matrix.inverse();
    Mat3 product = rot_matrix * inverse;
    
    // Should be approximately identity
    EXPECT_NEAR(product(0, 0), 1.0f, 1e-5f);
    EXPECT_NEAR(product(1, 1), 1.0f, 1e-5f);
    EXPECT_NEAR(product(2, 2), 1.0f, 1e-5f);
    EXPECT_NEAR(product(0, 1), 0.0f, 1e-5f);
    EXPECT_NEAR(product(1, 0), 0.0f, 1e-5f);
}

class RigidBodyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup for rigid body tests
    }
};

TEST_F(RigidBodyTest, RigidBody2DBasics) {
    RigidBody2D body(BodyType::Dynamic);
    
    // Initial state
    EXPECT_EQ(body.type, BodyType::Dynamic);
    EXPECT_FALSE(body.is_sleeping);
    EXPECT_GT(body.inverse_mass, 0.0f);
    
    // Set mass
    body.set_mass(5.0f);
    EXPECT_FLOAT_EQ(body.mass, 5.0f);
    EXPECT_FLOAT_EQ(body.inverse_mass, 0.2f);
    
    // Apply force
    Vec2 initial_velocity = body.velocity;
    body.apply_force(Vec2(10.0f, 0.0f));
    EXPECT_GT(body.force.x, 0.0f);
    
    // Apply impulse
    body.apply_impulse(Vec2(5.0f, 0.0f));
    EXPECT_GT(body.velocity.x, initial_velocity.x);
    
    // Static body
    RigidBody2D static_body(BodyType::Static);
    EXPECT_FLOAT_EQ(static_body.inverse_mass, 0.0f);
    
    static_body.apply_force(Vec2(100.0f, 0.0f));
    EXPECT_FLOAT_EQ(static_body.force.x, 0.0f);  // Should not accept forces
}

TEST_F(RigidBodyTest, RigidBody3DBasics) {
    RigidBody3D body(BodyType::Dynamic);
    
    // Set mass properties
    MassProperties props = MassProperties::for_box(2.0f, 2.0f, 2.0f, 1000.0f);
    body.set_mass_properties(props);
    
    EXPECT_GT(body.mass_props.mass, 0.0f);
    EXPECT_GT(body.mass_props.inverse_mass, 0.0f);
    
    // Apply forces and torques
    body.apply_force(Vec3(0.0f, -9.81f, 0.0f) * body.mass_props.mass);
    EXPECT_FLOAT_EQ(body.force.y, -9.81f * body.mass_props.mass);
    
    body.apply_torque(Vec3(1.0f, 0.0f, 0.0f));
    EXPECT_FLOAT_EQ(body.torque.x, 1.0f);
    
    // Integration
    Real dt = 1.0f / 60.0f;
    Vec3 initial_velocity = body.velocity;
    
    body.integrate_forces(dt);
    EXPECT_LT(body.velocity.y, initial_velocity.y);  // Should fall due to gravity
    
    Vec3 initial_position = body.transform.position;
    body.integrate_velocity(dt);
    EXPECT_LT(body.transform.position.y, initial_position.y);  // Should move down
}

TEST_F(RigidBodyTest, MassProperties) {
    // Box mass properties
    MassProperties box_props = MassProperties::for_box(2.0f, 4.0f, 6.0f, 1000.0f);
    Real expected_mass = 2.0f * 4.0f * 6.0f * 1000.0f;
    EXPECT_FLOAT_EQ(box_props.mass, expected_mass);
    EXPECT_GT(box_props.inertia(0, 0), 0.0f);
    EXPECT_GT(box_props.inertia(1, 1), 0.0f);
    EXPECT_GT(box_props.inertia(2, 2), 0.0f);
    
    // Sphere mass properties
    MassProperties sphere_props = MassProperties::for_sphere(2.0f, 1000.0f);
    Real expected_sphere_mass = (4.0f / 3.0f) * PI * 8.0f * 1000.0f;
    EXPECT_NEAR(sphere_props.mass, expected_sphere_mass, 1e-3f);
    
    // Inertia should be same for all axes (sphere)
    EXPECT_FLOAT_EQ(sphere_props.inertia(0, 0), sphere_props.inertia(1, 1));
    EXPECT_FLOAT_EQ(sphere_props.inertia(1, 1), sphere_props.inertia(2, 2));
    
    // Circle mass properties (2D)
    MassProperties circle_props = MassProperties::for_circle(3.0f, 500.0f);
    Real expected_circle_mass = PI * 9.0f * 500.0f;
    EXPECT_NEAR(circle_props.mass, expected_circle_mass, 1e-3f);
}

class CollisionDetectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        broad_phase = std::make_unique<BroadPhaseCollisionDetection>();
    }
    
    std::unique_ptr<BroadPhaseCollisionDetection> broad_phase;
};

TEST_F(CollisionDetectionTest, AABBOverlap) {
    AABB2D aabb1(Vec2(0.0f, 0.0f), Vec2(2.0f, 2.0f));
    AABB2D aabb2(Vec2(1.0f, 1.0f), Vec2(3.0f, 3.0f));
    AABB2D aabb3(Vec2(5.0f, 5.0f), Vec2(7.0f, 7.0f));
    
    // Overlapping boxes
    EXPECT_TRUE(aabb1.overlaps(aabb2));
    EXPECT_TRUE(aabb2.overlaps(aabb1));
    
    // Non-overlapping boxes
    EXPECT_FALSE(aabb1.overlaps(aabb3));
    EXPECT_FALSE(aabb3.overlaps(aabb1));
    
    // Point containment
    EXPECT_TRUE(aabb1.contains(Vec2(1.0f, 1.0f)));
    EXPECT_FALSE(aabb1.contains(Vec2(3.0f, 3.0f)));
    
    // AABB properties
    EXPECT_FLOAT_EQ(aabb1.area(), 4.0f);
    Vec2 center = aabb1.center();
    EXPECT_FLOAT_EQ(center.x, 1.0f);
    EXPECT_FLOAT_EQ(center.y, 1.0f);
}

TEST_F(CollisionDetectionTest, SpatialHashPerformance) {
    SpatialHash<AABB2D> spatial_hash(5.0f);
    
    // Create many random AABBs
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<Real> pos_dist(-100.0f, 100.0f);
    std::uniform_real_distribution<Real> size_dist(0.5f, 2.0f);
    
    const size_t num_objects = 1000;
    std::vector<AABB2D> aabbs;
    
    for (size_t i = 0; i < num_objects; ++i) {
        Vec2 pos(pos_dist(gen), pos_dist(gen));
        Vec2 half_size(size_dist(gen), size_dist(gen));
        
        AABB2D aabb(pos - half_size, pos + half_size);
        aabbs.push_back(aabb);
        spatial_hash.insert(static_cast<uint32_t>(i), aabb);
    }
    
    // Find collision pairs
    auto start_time = std::chrono::high_resolution_clock::now();
    auto pairs = spatial_hash.find_collision_pairs();
    auto end_time = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    // Should complete in reasonable time (less than 10ms for 1000 objects)
    EXPECT_LT(duration.count(), 10000);
    
    // Should find some pairs but not all possible combinations
    size_t max_possible_pairs = num_objects * (num_objects - 1) / 2;
    EXPECT_LT(pairs.size(), max_possible_pairs);
    
    // Memory usage should be reasonable
    size_t memory_usage = spatial_hash.get_memory_usage();
    EXPECT_LT(memory_usage, num_objects * 1000);  // Less than 1KB per object
}

TEST_F(CollisionDetectionTest, BroadPhaseEfficiency) {
    // Test broad phase with realistic scenario
    std::vector<RigidBody2D> bodies;
    std::vector<std::unique_ptr<Shape>> shapes;
    
    // Create a grid of objects
    const int grid_size = 20;
    const Real spacing = 5.0f;
    
    for (int x = 0; x < grid_size; ++x) {
        for (int y = 0; y < grid_size; ++y) {
            RigidBody2D body(BodyType::Dynamic);
            body.id = static_cast<uint32_t>(x * grid_size + y);
            body.transform.position = Vec2(x * spacing, y * spacing);
            
            bodies.push_back(body);
            shapes.push_back(std::make_unique<CircleShape>(1.0f));
        }
    }
    
    // Clear and populate broad phase
    broad_phase->clear();
    for (size_t i = 0; i < bodies.size(); ++i) {
        broad_phase->add_body_2d(bodies[i], *shapes[i]);
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    const auto& pairs = broad_phase->find_collision_pairs_2d();
    auto end_time = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    // Should complete quickly
    EXPECT_LT(duration.count(), 5000);  // Less than 5ms
    
    // Check efficiency ratio
    auto stats = broad_phase->get_stats();
    EXPECT_LT(stats.efficiency_ratio, 0.1f);  // Should filter out most pairs
    EXPECT_GT(stats.total_pairs, 0);  // Should find some nearby pairs
}

class ShapeTest : public ::testing::Test {
protected:
    void SetUp() override {
        circle = std::make_unique<CircleShape>(2.0f);
        box_2d = std::make_unique<BoxShape2D>(Vec2(1.5f, 2.5f));
        sphere = std::make_unique<SphereShape>(3.0f);
        box_3d = std::make_unique<BoxShape3D>(Vec3(2.0f, 3.0f, 4.0f));
    }
    
    std::unique_ptr<CircleShape> circle;
    std::unique_ptr<BoxShape2D> box_2d;
    std::unique_ptr<SphereShape> sphere;
    std::unique_ptr<BoxShape3D> box_3d;
};

TEST_F(ShapeTest, CircleShapeProperties) {
    Transform2D transform(Vec2(5.0f, 10.0f), PI / 4);
    
    // AABB should contain the circle
    AABB2D aabb = circle->get_aabb_2d(transform);
    EXPECT_FLOAT_EQ(aabb.min.x, 3.0f);
    EXPECT_FLOAT_EQ(aabb.max.x, 7.0f);
    EXPECT_FLOAT_EQ(aabb.min.y, 8.0f);
    EXPECT_FLOAT_EQ(aabb.max.y, 12.0f);
    
    // Support point should be on circle perimeter
    Vec2 support = circle->get_support_point_2d(Vec2(1.0f, 0.0f), transform);
    EXPECT_FLOAT_EQ(support.x, 7.0f);  // center.x + radius
    EXPECT_FLOAT_EQ(support.y, 10.0f); // center.y
    
    // Mass factor should be circle area
    Real expected_area = PI * circle->radius * circle->radius;
    EXPECT_NEAR(circle->get_mass_factor(), expected_area, 1e-3f);
}

TEST_F(ShapeTest, BoxShape2DProperties) {
    Transform2D transform(Vec2(0.0f, 0.0f), PI / 4);  // 45 degree rotation
    
    // AABB of rotated box should be larger
    AABB2D aabb = box_2d->get_aabb_2d(transform);
    
    // For 45-degree rotation, the diagonal becomes the new extent
    Real diagonal = std::sqrt(box_2d->half_extents.x * box_2d->half_extents.x + 
                             box_2d->half_extents.y * box_2d->half_extents.y);
    
    EXPECT_NEAR(aabb.max.x, diagonal, 1e-3f);
    EXPECT_NEAR(aabb.max.y, diagonal, 1e-3f);
    EXPECT_NEAR(aabb.min.x, -diagonal, 1e-3f);
    EXPECT_NEAR(aabb.min.y, -diagonal, 1e-3f);
    
    // Mass factor should be box area
    Real expected_area = 4.0f * box_2d->half_extents.x * box_2d->half_extents.y;
    EXPECT_FLOAT_EQ(box_2d->get_mass_factor(), expected_area);
}

TEST_F(ShapeTest, SphereShapeProperties) {
    Transform3D transform(Vec3(1.0f, 2.0f, 3.0f), Quaternion::identity());
    
    // AABB should be centered and sized correctly
    AABB3D aabb = sphere->get_aabb_3d(transform);
    EXPECT_FLOAT_EQ(aabb.center().x, 1.0f);
    EXPECT_FLOAT_EQ(aabb.center().y, 2.0f);
    EXPECT_FLOAT_EQ(aabb.center().z, 3.0f);
    
    Vec3 extents = aabb.extents();
    EXPECT_FLOAT_EQ(extents.x, sphere->radius);
    EXPECT_FLOAT_EQ(extents.y, sphere->radius);
    EXPECT_FLOAT_EQ(extents.z, sphere->radius);
    
    // Support point in any direction should be at surface
    Vec3 support = sphere->get_support_point_3d(Vec3(1.0f, 0.0f, 0.0f), transform);
    Real distance = (support - transform.position).length();
    EXPECT_NEAR(distance, sphere->radius, 1e-6f);
    
    // Mass factor should be sphere volume
    Real expected_volume = (4.0f / 3.0f) * PI * sphere->radius * sphere->radius * sphere->radius;
    EXPECT_NEAR(sphere->get_mass_factor(), expected_volume, 1e-3f);
}

class NarrowPhaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup for narrow phase tests
    }
};

TEST_F(NarrowPhaseTest, GJKCircleCircle) {
    // Two overlapping circles
    CircleShape circle_a(1.0f);
    CircleShape circle_b(1.0f);
    
    Transform2D transform_a(Vec2(0.0f, 0.0f), 0.0f);
    Transform2D transform_b(Vec2(1.5f, 0.0f), 0.0f);  // Overlapping
    
    bool intersects = GJK::intersects_2d(circle_a, transform_a, circle_b, transform_b);
    EXPECT_TRUE(intersects);
    
    // Non-overlapping circles
    Transform2D transform_c(Vec2(3.0f, 0.0f), 0.0f);  // Not overlapping
    
    bool not_intersects = GJK::intersects_2d(circle_a, transform_a, circle_b, transform_c);
    EXPECT_FALSE(not_intersects);
}

TEST_F(NarrowPhaseTest, GJKBoxBox) {
    // Two overlapping boxes
    BoxShape2D box_a(Vec2(1.0f, 1.0f));
    BoxShape2D box_b(Vec2(1.0f, 1.0f));
    
    Transform2D transform_a(Vec2(0.0f, 0.0f), 0.0f);
    Transform2D transform_b(Vec2(1.5f, 0.0f), 0.0f);  // Overlapping
    
    bool intersects = GJK::intersects_2d(box_a, transform_a, box_b, transform_b);
    EXPECT_TRUE(intersects);
    
    // Non-overlapping boxes
    Transform2D transform_c(Vec2(3.0f, 0.0f), 0.0f);
    
    bool not_intersects = GJK::intersects_2d(box_a, transform_a, box_b, transform_c);
    EXPECT_FALSE(not_intersects);
}

TEST_F(NarrowPhaseTest, CollisionManifoldGeneration) {
    // Create two colliding rigid bodies
    RigidBody2D body_a(BodyType::Dynamic);
    RigidBody2D body_b(BodyType::Dynamic);
    
    body_a.id = 1;
    body_b.id = 2;
    body_a.transform.position = Vec2(0.0f, 0.0f);
    body_b.transform.position = Vec2(1.5f, 0.0f);
    
    CircleShape shape_a(1.0f);
    CircleShape shape_b(1.0f);
    
    auto collision_info = NarrowPhaseCollisionDetection::test_collision_2d(
        body_a, shape_a, body_b, shape_b);
    
    EXPECT_TRUE(collision_info.is_colliding);
    EXPECT_EQ(collision_info.manifold.body_a_id, 1);
    EXPECT_EQ(collision_info.manifold.body_b_id, 2);
    EXPECT_FALSE(collision_info.manifold.contacts.empty());
    
    if (!collision_info.manifold.contacts.empty()) {
        const auto& contact = collision_info.manifold.contacts[0];
        EXPECT_GT(contact.penetration, 0.0f);
        EXPECT_GT(contact.normal.length_squared(), 0.0f);
    }
}

class MaterialTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager = &get_material_manager();
    }
    
    MaterialManager* manager;
};

TEST_F(MaterialTest, PredefinedMaterials) {
    // Test predefined materials exist
    const PhysicsMaterial* steel = manager->get_material("Steel");
    ASSERT_NE(steel, nullptr);
    EXPECT_GT(steel->density, 1000.0f);  // Steel should be dense
    EXPECT_GT(steel->metallic, 0.5f);    // Should be metallic
    
    const PhysicsMaterial* rubber = manager->get_material("Rubber");
    ASSERT_NE(rubber, nullptr);
    EXPECT_GT(rubber->restitution, 0.5f);  // Rubber should be bouncy
    
    const PhysicsMaterial* ice = manager->get_material("Ice");
    ASSERT_NE(ice, nullptr);
    EXPECT_LT(ice->friction, 0.1f);       // Ice should be slippery
    
    const PhysicsMaterial* sensor = manager->get_material("Sensor");
    ASSERT_NE(sensor, nullptr);
    EXPECT_TRUE(sensor->is_sensor);       // Should be a sensor material
}

TEST_F(MaterialTest, MaterialCombination) {
    // Test material property combination
    auto combined = manager->get_combined_properties("Steel", "Rubber");
    
    const PhysicsMaterial* steel = manager->get_material("Steel");
    const PhysicsMaterial* rubber = manager->get_material("Rubber");
    
    // Friction should be geometric mean
    Real expected_friction = std::sqrt(steel->friction * rubber->friction);
    EXPECT_NEAR(combined.friction, expected_friction, 1e-3f);
    
    // Restitution should be maximum
    Real expected_restitution = std::max(steel->restitution, rubber->restitution);
    EXPECT_FLOAT_EQ(combined.restitution, expected_restitution);
    
    // Test sensor combination
    auto sensor_combined = manager->get_combined_properties("Steel", "Sensor");
    EXPECT_TRUE(sensor_combined.is_sensor);  // Either material being sensor should make it sensor
}

TEST_F(MaterialTest, CustomMaterialCreation) {
    // Create custom material using builder
    auto custom_material = MaterialManager::create("CustomTest")
        .density(2500.0f)
        .friction(0.8f)
        .restitution(0.3f)
        .color(Vec3(1.0f, 0.0f, 0.0f))
        .roughness(0.7f)
        .build();
    
    uint32_t material_id = manager->register_material(std::move(custom_material));
    EXPECT_GT(material_id, 0);
    
    const PhysicsMaterial* retrieved = manager->get_material("CustomTest");
    ASSERT_NE(retrieved, nullptr);
    
    EXPECT_FLOAT_EQ(retrieved->density, 2500.0f);
    EXPECT_FLOAT_EQ(retrieved->friction, 0.8f);
    EXPECT_FLOAT_EQ(retrieved->restitution, 0.3f);
    EXPECT_FLOAT_EQ(retrieved->color.x, 1.0f);
    EXPECT_FLOAT_EQ(retrieved->roughness, 0.7f);
}

TEST_F(MaterialTest, MaterialInterpolation) {
    PhysicsMaterial steel = Materials::Steel();
    PhysicsMaterial rubber = Materials::Rubber();
    
    // Test interpolation at midpoint
    PhysicsMaterial interpolated = MaterialInterpolator::lerp(steel, rubber, 0.5f);
    
    Real expected_density = (steel.density + rubber.density) * 0.5f;
    EXPECT_FLOAT_EQ(interpolated.density, expected_density);
    
    Real expected_friction = (steel.friction + rubber.friction) * 0.5f;
    EXPECT_FLOAT_EQ(interpolated.friction, expected_friction);
    
    // Test interpolation at extremes
    PhysicsMaterial at_zero = MaterialInterpolator::lerp(steel, rubber, 0.0f);
    EXPECT_FLOAT_EQ(at_zero.density, steel.density);
    
    PhysicsMaterial at_one = MaterialInterpolator::lerp(steel, rubber, 1.0f);
    EXPECT_FLOAT_EQ(at_one.density, rubber.density);
}

class ConstraintTest : public ::testing::Test {
protected:
    void SetUp() override {
        body_a = std::make_unique<RigidBody3D>(BodyType::Dynamic);
        body_b = std::make_unique<RigidBody3D>(BodyType::Dynamic);
        
        body_a->id = 1;
        body_b->id = 2;
        body_a->transform.position = Vec3(0.0f, 0.0f, 0.0f);
        body_b->transform.position = Vec3(2.0f, 0.0f, 0.0f);
        
        // Set up reasonable mass properties
        body_a->set_mass_properties(MassProperties::for_box(1.0f, 1.0f, 1.0f, 1000.0f));
        body_b->set_mass_properties(MassProperties::for_box(1.0f, 1.0f, 1.0f, 1000.0f));
    }
    
    std::unique_ptr<RigidBody3D> body_a;
    std::unique_ptr<RigidBody3D> body_b;
};

TEST_F(ConstraintTest, DistanceConstraint) {
    // Create distance constraint
    Vec3 anchor_a(0.5f, 0.0f, 0.0f);  // Right side of body A
    Vec3 anchor_b(-0.5f, 0.0f, 0.0f); // Left side of body B
    Real rest_distance = 1.0f;
    
    DistanceConstraint constraint(body_a->id, body_b->id, anchor_a, anchor_b, rest_distance);
    
    Real dt = 1.0f / 60.0f;
    
    // Prepare constraint
    constraint.prepare(*body_a, *body_b, dt);
    
    // The current distance should be 1.0 (2.0 - 0.5 - 0.5)
    EXPECT_NEAR(constraint.current_distance, 1.0f, 1e-3f);
    
    // Move bodies apart
    body_b->transform.position.x = 5.0f;  // Now distance is 4.0
    
    // Solve position constraint (should pull bodies together)
    Vec3 pos_a_before = body_a->transform.position;
    Vec3 pos_b_before = body_b->transform.position;
    
    constraint.solve_position(*body_a, *body_b, dt);
    
    // Bodies should move towards each other
    EXPECT_GT(body_a->transform.position.x, pos_a_before.x);
    EXPECT_LT(body_b->transform.position.x, pos_b_before.x);
}

TEST_F(ConstraintTest, PinConstraint) {
    // Pin body A to world origin
    Vec3 anchor(0.0f, 0.0f, 0.0f);
    Vec3 world_position(0.0f, 0.0f, 0.0f);
    
    PinConstraint constraint(body_a->id, anchor, world_position);
    
    // Move body away from pin position
    body_a->transform.position = Vec3(2.0f, 3.0f, 4.0f);
    
    Real dt = 1.0f / 60.0f;
    constraint.prepare(*body_a, *body_b, dt);  // body_b is dummy for pin constraint
    
    // Solve position constraint
    constraint.solve_position(*body_a, *body_b, dt);
    
    // Body should be pulled back towards origin
    Real distance_from_origin = body_a->transform.position.length();
    EXPECT_LT(distance_from_origin, 5.0f);  // Should be closer than original position
}

TEST_F(ConstraintTest, ConstraintSolver) {
    ConstraintSolver solver;
    
    // Create multiple constraints
    auto distance_constraint = std::make_unique<DistanceConstraint>(
        body_a->id, body_b->id, Vec3::zero(), Vec3::zero(), 2.0f);
    
    auto pin_constraint = std::make_unique<PinConstraint>(
        body_a->id, Vec3::zero(), Vec3::zero());
    
    solver.add_constraint(std::move(distance_constraint));
    solver.add_constraint(std::move(pin_constraint));
    
    EXPECT_EQ(solver.get_constraint_count(), 2);
    
    // Create body vector for solving
    std::vector<RigidBody3D> bodies = {*body_a, *body_b};
    
    // Move bodies to violate constraints
    bodies[0].transform.position = Vec3(1.0f, 1.0f, 0.0f);
    bodies[1].transform.position = Vec3(5.0f, 2.0f, 0.0f);
    
    Real dt = 1.0f / 60.0f;
    
    // Solve constraints
    solver.solve_constraints(bodies, dt);
    
    // Constraints should be better satisfied
    Real body_a_distance_from_origin = bodies[0].transform.position.length();
    EXPECT_LT(body_a_distance_from_origin, 1.5f);  // Pin constraint should pull A to origin
    
    Real distance_between = (bodies[1].transform.position - bodies[0].transform.position).length();
    EXPECT_GT(distance_between, 1.5f);  // Distance constraint should prevent them from being too close
}

class PhysicsWorldTest : public ::testing::Test {
protected:
    void SetUp() override {
        world_2d = std::make_unique<PhysicsWorld>(true);   // 2D world
        world_3d = std::make_unique<PhysicsWorld>(false);  // 3D world
        
        // Configure for testing
        PhysicsWorldConfig config;
        config.velocity_iterations = 4;
        config.position_iterations = 2;
        config.allow_sleep = false;  // Disable sleep for consistent testing
        
        world_2d->set_config(config);
        world_3d->set_config(config);
    }
    
    std::unique_ptr<PhysicsWorld> world_2d;
    std::unique_ptr<PhysicsWorld> world_3d;
};

TEST_F(PhysicsWorldTest, BasicWorld2D) {
    EXPECT_TRUE(world_2d->is_2d());
    EXPECT_EQ(world_2d->get_body_count(), 0);
    
    // Create a body
    Transform2D transform(Vec2(0.0f, 10.0f), 0.0f);
    uint32_t body_id = world_2d->create_body_2d(transform, BodyType::Dynamic);
    
    EXPECT_GT(body_id, 0);
    EXPECT_EQ(world_2d->get_body_count(), 1);
    
    // Set shape and material
    world_2d->set_body_shape(body_id, std::make_unique<CircleShape>(1.0f));
    world_2d->set_body_material(body_id, "Steel");
    
    RigidBody2D* body = world_2d->get_body_2d(body_id);
    ASSERT_NE(body, nullptr);
    EXPECT_EQ(body->id, body_id);
    EXPECT_FLOAT_EQ(body->transform.position.y, 10.0f);
}

TEST_F(PhysicsWorldTest, BasicWorld3D) {
    EXPECT_FALSE(world_3d->is_2d());
    
    // Create a body
    Transform3D transform(Vec3(0.0f, 10.0f, 0.0f), Quaternion::identity());
    uint32_t body_id = world_3d->create_body_3d(transform, BodyType::Dynamic);
    
    world_3d->set_body_shape(body_id, std::make_unique<SphereShape>(1.0f));
    world_3d->set_body_material(body_id, "Rubber");
    
    RigidBody3D* body = world_3d->get_body_3d(body_id);
    ASSERT_NE(body, nullptr);
    EXPECT_FLOAT_EQ(body->transform.position.y, 10.0f);
    
    // Test gravity effect
    Vec3 initial_position = body->transform.position;
    
    // Step simulation
    Real dt = 1.0f / 60.0f;
    for (int i = 0; i < 60; ++i) {  // 1 second of simulation
        world_3d->step(dt);
    }
    
    // Body should have fallen due to gravity
    EXPECT_LT(body->transform.position.y, initial_position.y);
    EXPECT_LT(body->velocity.y, 0.0f);  // Should be moving downward
}

TEST_F(PhysicsWorldTest, CollisionDetection) {
    // Create two bodies that will collide
    Transform3D transform_a(Vec3(0.0f, 0.0f, 0.0f), Quaternion::identity());
    Transform3D transform_b(Vec3(1.8f, 0.0f, 0.0f), Quaternion::identity());
    
    uint32_t body_a_id = world_3d->create_body_3d(transform_a, BodyType::Dynamic);
    uint32_t body_b_id = world_3d->create_body_3d(transform_b, BodyType::Dynamic);
    
    world_3d->set_body_shape(body_a_id, std::make_unique<SphereShape>(1.0f));
    world_3d->set_body_shape(body_b_id, std::make_unique<SphereShape>(1.0f));
    world_3d->set_body_material(body_a_id, "Rubber");
    world_3d->set_body_material(body_b_id, "Rubber");
    
    // Set up collision callback
    bool collision_detected = false;
    world_3d->set_collision_callback([&collision_detected](uint32_t a, uint32_t b, const ContactManifold& manifold) {
        collision_detected = true;
    });
    
    // Push bodies towards each other
    RigidBody3D* body_a = world_3d->get_body_3d(body_a_id);
    RigidBody3D* body_b = world_3d->get_body_3d(body_b_id);
    
    body_a->velocity = Vec3(2.0f, 0.0f, 0.0f);
    body_b->velocity = Vec3(-2.0f, 0.0f, 0.0f);
    
    // Simulate until collision
    Real dt = 1.0f / 60.0f;
    for (int i = 0; i < 30; ++i) {
        world_3d->step(dt);
        if (collision_detected) break;
    }
    
    EXPECT_TRUE(collision_detected);
    
    // Bodies should bounce apart (due to rubber restitution)
    EXPECT_LT(body_a->velocity.x, 1.0f);  // Should have slowed down or reversed
    EXPECT_GT(body_b->velocity.x, -1.0f); // Should have slowed down or reversed
}

TEST_F(PhysicsWorldTest, PerformanceStats) {
    // Create many bodies for performance testing
    const size_t num_bodies = 100;
    std::vector<uint32_t> body_ids;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<Real> pos_dist(-10.0f, 10.0f);
    
    for (size_t i = 0; i < num_bodies; ++i) {
        Vec3 position(pos_dist(gen), pos_dist(gen) + 20.0f, pos_dist(gen));
        Transform3D transform(position, Quaternion::identity());
        
        uint32_t body_id = world_3d->create_body_3d(transform, BodyType::Dynamic);
        world_3d->set_body_shape(body_id, std::make_unique<SphereShape>(0.5f));
        world_3d->set_body_material(body_id, "Steel");
        
        body_ids.push_back(body_id);
    }
    
    EXPECT_EQ(world_3d->get_body_count(), num_bodies);
    
    // Run simulation for several steps
    Real dt = 1.0f / 60.0f;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 60; ++i) {
        world_3d->step(dt);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // Get performance statistics
    const PhysicsStats& stats = world_3d->get_stats();
    
    EXPECT_GT(stats.active_bodies, 0);
    EXPECT_GE(stats.total_shapes, num_bodies);
    EXPECT_GT(stats.fps, 0.0f);
    EXPECT_GT(stats.memory_usage_bytes, 0);
    
    // Should complete in reasonable time (less than 2 seconds for 100 bodies)
    EXPECT_LT(duration.count(), 2000);
    
    // Check that broad phase is efficient
    if (stats.collision_pairs > 0) {
        EXPECT_LT(stats.efficiency_ratio, 0.5f);  // Should filter out many potential pairs
    }
}

class BenchmarkTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup for benchmarks
    }
    
    void benchmark_physics_step(size_t body_count, const std::string& test_name) {
        PhysicsWorld world(false);  // 3D world
        
        // Disable sleep for consistent benchmarking
        PhysicsWorldConfig config = world.get_config();
        config.allow_sleep = false;
        world.set_config(config);
        
        // Create bodies in a grid
        std::vector<uint32_t> body_ids;
        const int grid_size = static_cast<int>(std::sqrt(body_count));
        const Real spacing = 2.0f;
        
        for (int x = 0; x < grid_size; ++x) {
            for (int y = 0; y < grid_size; ++y) {
                for (int z = 0; z < (body_count / (grid_size * grid_size)) + 1 && body_ids.size() < body_count; ++z) {
                    Vec3 position(x * spacing, y * spacing + 10.0f, z * spacing);
                    Transform3D transform(position, Quaternion::identity());
                    
                    uint32_t body_id = world.create_body_3d(transform, BodyType::Dynamic);
                    world.set_body_shape(body_id, std::make_unique<SphereShape>(0.5f));
                    world.set_body_material(body_id, "Steel");
                    
                    body_ids.push_back(body_id);
                }
            }
        }
        
        // Warm up
        Real dt = 1.0f / 60.0f;
        for (int i = 0; i < 10; ++i) {
            world.step(dt);
        }
        
        // Benchmark
        const int num_steps = 100;
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < num_steps; ++i) {
            world.step(dt);
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        Real avg_step_time = duration.count() / Real(num_steps);
        Real fps = 1e6f / avg_step_time;  // Convert microseconds to FPS
        
        std::cout << test_name << " with " << body_count << " bodies:\n";
        std::cout << "  Average step time: " << avg_step_time << " microseconds\n";
        std::cout << "  Equivalent FPS: " << fps << "\n";
        std::cout << "  Total simulation time: " << duration.count() / 1000.0f << " ms\n";
        
        const PhysicsStats& stats = world.get_stats();
        std::cout << "  Collision pairs: " << stats.collision_pairs << "\n";
        std::cout << "  Active contacts: " << stats.active_contacts << "\n";
        std::cout << "  Memory usage: " << stats.memory_usage_bytes / 1024 << " KB\n";
        std::cout << "  Broad phase efficiency: " << (stats.efficiency_ratio * 100) << "%\n\n";
        
        // Performance expectations (these are rough guidelines)
        if (body_count <= 100) {
            EXPECT_GT(fps, 60.0f);  // Should maintain 60 FPS with 100 bodies
        } else if (body_count <= 1000) {
            EXPECT_GT(fps, 10.0f);  // Should maintain 10 FPS with 1000 bodies
        }
        
        EXPECT_LT(stats.efficiency_ratio, 0.1f);  // Broad phase should be very efficient
    }
};

TEST_F(BenchmarkTest, SmallScale) {
    benchmark_physics_step(10, "Small Scale");
}

TEST_F(BenchmarkTest, MediumScale) {
    benchmark_physics_step(100, "Medium Scale");
}

TEST_F(BenchmarkTest, LargeScale) {
    benchmark_physics_step(1000, "Large Scale");
}

TEST_F(BenchmarkTest, DISABLED_ExtremeScale) {
    // Disabled by default as it's very expensive
    benchmark_physics_step(10000, "Extreme Scale");
}

// Test runner
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    std::cout << "=== ECScope Physics Engine Test Suite ===\n\n";
    std::cout << "Testing professional-grade 2D/3D physics engine with:\n";
    std::cout << "- Mathematical foundations (Vec2/3, Quaternion, Matrix)\n";
    std::cout << "- Rigid body dynamics with mass properties\n";
    std::cout << "- Broad phase collision detection (spatial hashing)\n";
    std::cout << "- Narrow phase collision detection (GJK/EPA)\n";
    std::cout << "- Constraint solving (distance, pin, hinge, contact)\n";
    std::cout << "- Advanced materials system\n";
    std::cout << "- Deterministic physics world simulation\n";
    std::cout << "- Performance benchmarks\n\n";
    
    return RUN_ALL_TESTS();
}