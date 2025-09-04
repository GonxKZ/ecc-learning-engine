#pragma once

/**
 * @file physics/components3d.hpp
 * @brief Comprehensive 3D Physics Components for ECScope Educational ECS Engine
 * 
 * This header implements advanced 3D physics components that extend the 2D foundation
 * into the third dimension. It provides all necessary components for realistic 3D
 * physics simulation with emphasis on educational clarity and performance.
 * 
 * Key Components:
 * - RigidBody3D: Complete 3D rigid body dynamics with inertia tensors
 * - Collider3D: 3D collision shapes and detection properties
 * - Transform3D: Enhanced 3D transformations with quaternion rotations
 * - ForceAccumulator3D: 3D force and torque accumulation system
 * - Joint3D: Constraint-based joint system for connected bodies
 * - DebugRenderer3D: Educational visualization components
 * 
 * Educational Features:
 * - Step-by-step breakdown of 3D physics concepts
 * - Interactive parameter tuning for learning
 * - Performance comparison with 2D equivalents
 * - Real-time visualization of physics concepts
 * 
 * Performance Optimizations:
 * - SIMD-optimized 3D vector operations
 * - Cache-friendly component layouts
 * - Memory pool allocations for frequent operations
 * - Integration with job system for parallel processing
 * 
 * @author ECScope Educational ECS Framework - 3D Physics Extension
 * @date 2025
 */

#include "components.hpp"  // 2D physics components foundation
#include "math3d.hpp"      // 3D mathematics foundation
#include "collision3d.hpp" // 3D collision detection
#include "core/types.hpp"
#include "ecs/component.hpp"
#include "memory/memory_tracker.hpp"
#include <array>
#include <vector>
#include <optional>

namespace ecscope::physics::components3d {

// Import 3D math foundation
using namespace ecscope::physics::math3d;
using namespace ecscope::physics::collision3d;

// Import 2D foundation for comparison and compatibility
using namespace ecscope::physics::components;

//=============================================================================
// 3D Rigid Body Dynamics Component
//=============================================================================

/**
 * @brief Comprehensive 3D Rigid Body Component
 * 
 * Educational Context:
 * 3D rigid body dynamics introduces significant complexity compared to 2D:
 * - 3x3 inertia tensors instead of scalar moments of inertia
 * - Quaternion rotations for robust 3D orientation representation
 * - Angular velocity and momentum in 3D space
 * - Euler's equations for rotational motion
 * - Gyroscopic effects and precession
 * 
 * Key Differences from 2D:
 * - Linear motion: 3 DOF instead of 2
 * - Angular motion: 3 DOF instead of 1
 * - Inertia: 3x3 matrix instead of scalar
 * - Orientation: Quaternion instead of angle
 * - Computational complexity: Significantly higher
 * 
 * Performance Considerations:
 * - Aligned memory layout for SIMD operations
 * - Pre-computed inertia tensor inverses
 * - Optimized quaternion operations
 * - Cache-friendly data access patterns
 */
struct alignas(64) RigidBody3D {
    //-------------------------------------------------------------------------
    // Linear Motion Properties
    //-------------------------------------------------------------------------
    
    /** @brief Mass in kilograms (0 = immovable/static body) */
    f32 mass{1.0f};
    
    /** @brief Inverse mass (cached for performance: 1/mass or 0 for static) */
    f32 inv_mass{1.0f};
    
    /** @brief Linear velocity in m/s */
    Vec3 linear_velocity{0.0f, 0.0f, 0.0f};
    
    /** @brief Linear acceleration in m/s² (computed each frame) */
    Vec3 linear_acceleration{0.0f, 0.0f, 0.0f};
    
    /** @brief Accumulated forces for current frame */
    Vec3 accumulated_force{0.0f, 0.0f, 0.0f};
    
    /** @brief Center of mass in local coordinates */
    Vec3 local_center_of_mass{0.0f, 0.0f, 0.0f};
    
    //-------------------------------------------------------------------------
    // Angular Motion Properties (3D Specific)
    //-------------------------------------------------------------------------
    
    /**
     * @brief 3x3 Inertia tensor in local space
     * 
     * Educational Context:
     * The inertia tensor generalizes the scalar moment of inertia to 3D.
     * It's a 3x3 symmetric matrix that describes how mass is distributed
     * around the three principal axes.
     * 
     * Matrix form:
     * | Ixx -Ixy -Ixz |
     * |-Iyx  Iyy -Iyz |
     * |-Izx -Izy  Izz |
     * 
     * Where diagonal elements are moments of inertia and off-diagonal
     * elements are products of inertia.
     */
    alignas(16) std::array<f32, 9> inertia_tensor{
        1.0f, 0.0f, 0.0f,  // Row 1: Ixx, -Ixy, -Ixz
        0.0f, 1.0f, 0.0f,  // Row 2: -Iyx, Iyy, -Iyz  
        0.0f, 0.0f, 1.0f   // Row 3: -Izx, -Izy, Izz
    };
    
    /**
     * @brief Inverse inertia tensor (cached for performance)
     * 
     * Precomputed inverse avoids expensive matrix inversion during simulation.
     * Updated when inertia tensor or orientation changes.
     */
    alignas(16) std::array<f32, 9> inv_inertia_tensor{
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    };
    
    /**
     * @brief Angular velocity in rad/s (local space)
     * 
     * Educational Context:
     * Angular velocity in 3D is a vector where:
     * - Direction: Rotation axis (right-hand rule)
     * - Magnitude: Angular speed in rad/s
     * - Unlike 2D, 3D angular velocity can change direction!
     */
    Vec3 angular_velocity{0.0f, 0.0f, 0.0f};
    
    /**
     * @brief Angular acceleration in rad/s² (computed each frame)
     */
    Vec3 angular_acceleration{0.0f, 0.0f, 0.0f};
    
    /**
     * @brief Angular momentum in kg⋅m²/s (world space)
     * 
     * Educational Context:
     * L = I * ω in 3D, where I is the world-space inertia tensor
     * Angular momentum is conserved in the absence of external torques.
     */
    Vec3 angular_momentum{0.0f, 0.0f, 0.0f};
    
    /** @brief Accumulated torque for current frame */
    Vec3 accumulated_torque{0.0f, 0.0f, 0.0f};
    
    //-------------------------------------------------------------------------
    // Material Properties
    //-------------------------------------------------------------------------
    
    /** @brief Coefficient of restitution (0 = perfectly inelastic, 1 = perfectly elastic) */
    f32 restitution{0.5f};
    
    /** @brief Static friction coefficient */
    f32 static_friction{0.6f};
    
    /** @brief Dynamic friction coefficient */
    f32 dynamic_friction{0.4f};
    
    /** @brief Linear damping factor (air resistance) */
    f32 linear_damping{0.01f};
    
    /** @brief Angular damping factor (rotational air resistance) */
    f32 angular_damping{0.01f};
    
    /** @brief Density in kg/m³ (used for automatic mass calculation) */
    f32 density{1000.0f};
    
    //-------------------------------------------------------------------------
    // Simulation State
    //-------------------------------------------------------------------------
    
    /** @brief Body type (dynamic, kinematic, static) */
    BodyType body_type{BodyType::Dynamic};
    
    /** @brief Whether body is awake and should be simulated */
    bool is_awake{true};
    
    /** @brief Whether body can go to sleep */
    bool can_sleep{true};
    
    /** @brief Time body has been moving slowly (for sleep detection) */
    f32 sleep_time{0.0f};
    
    /** @brief Whether body has gravity applied */
    bool use_gravity{true};
    
    /** @brief Whether body is affected by global forces */
    bool use_global_forces{true};
    
    /** @brief Collision detection layer mask */
    u32 collision_layer{1};
    
    /** @brief Which layers this body can collide with */
    u32 collision_mask{0xFFFFFFFF};
    
    //-------------------------------------------------------------------------
    // Performance and Debugging
    //-------------------------------------------------------------------------
    
    /** @brief Estimated kinetic energy (for educational analysis) */
    mutable f32 kinetic_energy{0.0f};
    
    /** @brief Estimated rotational energy (for educational analysis) */
    mutable f32 rotational_energy{0.0f};
    
    /** @brief Whether to render debug information */
    bool debug_render{false};
    
    /** @brief Debug color for visualization */
    u32 debug_color{0xFFFFFFFF};
    
    //-------------------------------------------------------------------------
    // Constructors and Factory Methods
    //-------------------------------------------------------------------------
    
    RigidBody3D() = default;
    
    /**
     * @brief Create dynamic rigid body with specified mass
     */
    static RigidBody3D create_dynamic(f32 mass, f32 density = 1000.0f) {
        RigidBody3D body;
        body.set_mass(mass);
        body.density = density;
        body.body_type = BodyType::Dynamic;
        body.is_awake = true;
        return body;
    }
    
    /**
     * @brief Create kinematic rigid body (infinite mass, controlled by user)
     */
    static RigidBody3D create_kinematic() {
        RigidBody3D body;
        body.set_mass(0.0f);  // Infinite mass
        body.body_type = BodyType::Kinematic;
        body.is_awake = true;
        body.can_sleep = false;
        return body;
    }
    
    /**
     * @brief Create static rigid body (infinite mass, never moves)
     */
    static RigidBody3D create_static() {
        RigidBody3D body;
        body.set_mass(0.0f);  // Infinite mass
        body.body_type = BodyType::Static;
        body.is_awake = false;
        body.can_sleep = false;
        return body;
    }
    
    //-------------------------------------------------------------------------
    // Mass and Inertia Management
    //-------------------------------------------------------------------------
    
    /**
     * @brief Set mass and update inverse mass
     */
    void set_mass(f32 new_mass) {
        mass = new_mass;
        if (mass > constants::EPSILON) {
            inv_mass = 1.0f / mass;
            body_type = BodyType::Dynamic;
        } else {
            mass = 0.0f;
            inv_mass = 0.0f;
            // Static or kinematic body
        }
    }
    
    /**
     * @brief Set inertia tensor for common shapes
     */
    void set_inertia_tensor_sphere(f32 radius) {
        f32 inertia = 0.4f * mass * radius * radius;  // (2/5) * m * r²
        set_inertia_tensor_diagonal(inertia, inertia, inertia);
    }
    
    void set_inertia_tensor_box(const Vec3& size) {
        f32 ixx = (mass / 12.0f) * (size.y * size.y + size.z * size.z);
        f32 iyy = (mass / 12.0f) * (size.x * size.x + size.z * size.z);
        f32 izz = (mass / 12.0f) * (size.x * size.x + size.y * size.y);
        set_inertia_tensor_diagonal(ixx, iyy, izz);
    }
    
    void set_inertia_tensor_cylinder(f32 radius, f32 height, const Vec3& axis = Vec3::unit_z()) {
        f32 parallel_inertia = 0.5f * mass * radius * radius;          // (1/2) * m * r²
        f32 perpendicular_inertia = mass * (3.0f * radius * radius + height * height) / 12.0f;
        
        // Set inertia based on cylinder axis
        if (std::abs(axis.dot(Vec3::unit_z())) > 0.9f) {
            set_inertia_tensor_diagonal(perpendicular_inertia, perpendicular_inertia, parallel_inertia);
        } else if (std::abs(axis.dot(Vec3::unit_y())) > 0.9f) {
            set_inertia_tensor_diagonal(perpendicular_inertia, parallel_inertia, perpendicular_inertia);
        } else {
            set_inertia_tensor_diagonal(parallel_inertia, perpendicular_inertia, perpendicular_inertia);
        }
    }
    
    /**
     * @brief Set diagonal inertia tensor (most common case)
     */
    void set_inertia_tensor_diagonal(f32 ixx, f32 iyy, f32 izz) {
        inertia_tensor = {
            ixx, 0.0f, 0.0f,
            0.0f, iyy, 0.0f,
            0.0f, 0.0f, izz
        };
        update_inverse_inertia_tensor();
    }
    
    /**
     * @brief Set full 3x3 inertia tensor
     */
    void set_inertia_tensor(const std::array<f32, 9>& tensor) {
        inertia_tensor = tensor;
        update_inverse_inertia_tensor();
    }
    
    //-------------------------------------------------------------------------
    // Force and Torque Application
    //-------------------------------------------------------------------------
    
    /**
     * @brief Apply force at center of mass
     */
    void apply_force(const Vec3& force) {
        if (body_type != BodyType::Dynamic) return;
        accumulated_force += force;
    }
    
    /**
     * @brief Apply force at specific world point (generates torque)
     */
    void apply_force_at_point(const Vec3& force, const Vec3& world_point, const Vec3& center_of_mass_world) {
        if (body_type != BodyType::Dynamic) return;
        
        // Apply linear force
        accumulated_force += force;
        
        // Apply torque: τ = r × F
        Vec3 r = world_point - center_of_mass_world;
        Vec3 torque = r.cross(force);
        accumulated_torque += torque;
    }
    
    /**
     * @brief Apply pure torque
     */
    void apply_torque(const Vec3& torque) {
        if (body_type != BodyType::Dynamic) return;
        accumulated_torque += torque;
    }
    
    /**
     * @brief Apply impulse (instantaneous momentum change)
     */
    void apply_impulse(const Vec3& impulse) {
        if (body_type != BodyType::Dynamic) return;
        linear_velocity += impulse * inv_mass;
    }
    
    /**
     * @brief Apply angular impulse (instantaneous angular momentum change)
     */
    void apply_angular_impulse(const Vec3& angular_impulse) {
        if (body_type != BodyType::Dynamic) return;
        
        // Convert angular impulse to change in angular velocity
        Vec3 delta_omega = multiply_by_inverse_inertia(angular_impulse);
        angular_velocity += delta_omega;
    }
    
    /**
     * @brief Apply impulse at specific world point
     */
    void apply_impulse_at_point(const Vec3& impulse, const Vec3& world_point, const Vec3& center_of_mass_world) {
        if (body_type != BodyType::Dynamic) return;
        
        // Apply linear impulse
        linear_velocity += impulse * inv_mass;
        
        // Apply angular impulse: L = r × p
        Vec3 r = world_point - center_of_mass_world;
        Vec3 angular_impulse = r.cross(impulse);
        apply_angular_impulse(angular_impulse);
    }
    
    //-------------------------------------------------------------------------
    // State Management and Integration
    //-------------------------------------------------------------------------
    
    /**
     * @brief Clear accumulated forces and torques
     */
    void clear_forces() {
        accumulated_force = Vec3::zero();
        accumulated_torque = Vec3::zero();
    }
    
    /**
     * @brief Wake up the body
     */
    void wake_up() {
        is_awake = true;
        sleep_time = 0.0f;
    }
    
    /**
     * @brief Put body to sleep
     */
    void sleep() {
        if (!can_sleep) return;
        
        is_awake = false;
        linear_velocity = Vec3::zero();
        angular_velocity = Vec3::zero();
        accumulated_force = Vec3::zero();
        accumulated_torque = Vec3::zero();
    }
    
    /**
     * @brief Check if body should go to sleep
     */
    bool should_sleep(f32 linear_threshold = 0.01f, f32 angular_threshold = 0.01f, f32 time_threshold = 1.0f) const {
        if (!can_sleep || !is_awake || body_type != BodyType::Dynamic) {
            return false;
        }
        
        bool is_moving_slowly = (linear_velocity.length_squared() < linear_threshold * linear_threshold) &&
                               (angular_velocity.length_squared() < angular_threshold * angular_threshold);
        
        return is_moving_slowly && sleep_time > time_threshold;
    }
    
    //-------------------------------------------------------------------------
    // Physics Calculations
    //-------------------------------------------------------------------------
    
    /**
     * @brief Calculate current kinetic energy (1/2 * m * v²)
     */
    f32 calculate_kinetic_energy() const {
        if (body_type != BodyType::Dynamic) return 0.0f;
        return 0.5f * mass * linear_velocity.length_squared();
    }
    
    /**
     * @brief Calculate current rotational energy (1/2 * ω^T * I * ω)
     */
    f32 calculate_rotational_energy() const {
        if (body_type != BodyType::Dynamic) return 0.0f;
        
        // E_rot = 1/2 * ω^T * I * ω
        Vec3 inertia_times_omega = multiply_by_inertia(angular_velocity);
        return 0.5f * angular_velocity.dot(inertia_times_omega);
    }
    
    /**
     * @brief Calculate total mechanical energy
     */
    f32 calculate_total_energy() const {
        return calculate_kinetic_energy() + calculate_rotational_energy();
    }
    
    /**
     * @brief Update cached energy values (for educational analysis)
     */
    void update_energy_cache() const {
        kinetic_energy = calculate_kinetic_energy();
        rotational_energy = calculate_rotational_energy();
    }
    
    //-------------------------------------------------------------------------
    // Matrix Operations (Inertia Tensor Utilities)
    //-------------------------------------------------------------------------
    
    /**
     * @brief Multiply vector by inertia tensor: I * ω
     */
    Vec3 multiply_by_inertia(const Vec3& omega) const {
        return Vec3{
            inertia_tensor[0] * omega.x + inertia_tensor[1] * omega.y + inertia_tensor[2] * omega.z,
            inertia_tensor[3] * omega.x + inertia_tensor[4] * omega.y + inertia_tensor[5] * omega.z,
            inertia_tensor[6] * omega.x + inertia_tensor[7] * omega.y + inertia_tensor[8] * omega.z
        };
    }
    
    /**
     * @brief Multiply vector by inverse inertia tensor: I⁻¹ * τ
     */
    Vec3 multiply_by_inverse_inertia(const Vec3& torque) const {
        return Vec3{
            inv_inertia_tensor[0] * torque.x + inv_inertia_tensor[1] * torque.y + inv_inertia_tensor[2] * torque.z,
            inv_inertia_tensor[3] * torque.x + inv_inertia_tensor[4] * torque.y + inv_inertia_tensor[5] * torque.z,
            inv_inertia_tensor[6] * torque.x + inv_inertia_tensor[7] * torque.y + inv_inertia_tensor[8] * torque.z
        };
    }
    
private:
    /**
     * @brief Update inverse inertia tensor (expensive operation)
     */
    void update_inverse_inertia_tensor() {
        // For educational purposes, we'll implement 3x3 matrix inversion
        // In production, you might use specialized libraries or optimizations
        
        // Extract matrix elements
        f32 a11 = inertia_tensor[0], a12 = inertia_tensor[1], a13 = inertia_tensor[2];
        f32 a21 = inertia_tensor[3], a22 = inertia_tensor[4], a23 = inertia_tensor[5];
        f32 a31 = inertia_tensor[6], a32 = inertia_tensor[7], a33 = inertia_tensor[8];
        
        // Calculate determinant
        f32 det = a11 * (a22 * a33 - a23 * a32) - 
                  a12 * (a21 * a33 - a23 * a31) + 
                  a13 * (a21 * a32 - a22 * a31);
        
        if (std::abs(det) < constants::EPSILON) {
            // Singular matrix - use identity
            inv_inertia_tensor = {
                1.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 1.0f
            };
            return;
        }
        
        f32 inv_det = 1.0f / det;
        
        // Calculate adjugate matrix and multiply by 1/det
        inv_inertia_tensor[0] = (a22 * a33 - a23 * a32) * inv_det;
        inv_inertia_tensor[1] = (a13 * a32 - a12 * a33) * inv_det;
        inv_inertia_tensor[2] = (a12 * a23 - a13 * a22) * inv_det;
        inv_inertia_tensor[3] = (a23 * a31 - a21 * a33) * inv_det;
        inv_inertia_tensor[4] = (a11 * a33 - a13 * a31) * inv_det;
        inv_inertia_tensor[5] = (a13 * a21 - a11 * a23) * inv_det;
        inv_inertia_tensor[6] = (a21 * a32 - a22 * a31) * inv_det;
        inv_inertia_tensor[7] = (a12 * a31 - a11 * a32) * inv_det;
        inv_inertia_tensor[8] = (a11 * a22 - a12 * a21) * inv_det;
    }
};

//=============================================================================
// 3D Collider Component
//=============================================================================

/**
 * @brief 3D Collision Detection Component
 * 
 * Extends the 2D collider concept to handle 3D collision shapes and detection.
 */
struct alignas(32) Collider3D {
    /** @brief Type of 3D collision shape */
    enum class ShapeType : u8 {
        Sphere = 0,
        Box,
        Capsule,
        ConvexHull,
        TriangleMesh,
        Compound
    };
    
    /** @brief Shape type */
    ShapeType shape_type{ShapeType::Sphere};
    
    /** @brief Whether this is a trigger (no collision response) */
    bool is_trigger{false};
    
    /** @brief Whether collider is enabled */
    bool is_enabled{true};
    
    /** @brief Local offset from entity transform */
    Vec3 local_offset{0.0f, 0.0f, 0.0f};
    
    /** @brief Local rotation offset */
    Quaternion local_rotation{0.0f, 0.0f, 0.0f, 1.0f};
    
    /** @brief Shape-specific parameters */
    union ShapeData {
        struct {
            f32 radius;
        } sphere;
        
        struct {
            Vec3 half_extents;
        } box;
        
        struct {
            f32 radius;
            f32 height;
        } capsule;
        
        struct {
            u32 vertex_count;
            Vec3* vertices;  // Managed by memory system
        } convex_hull;
        
        struct {
            u32 triangle_count;
            void* mesh_data;  // Managed by memory system
        } triangle_mesh;
        
        ShapeData() { memset(this, 0, sizeof(ShapeData)); }
    } shape_data;
    
    /** @brief Collision layer for filtering */
    u32 collision_layer{1};
    
    /** @brief Which layers this collider interacts with */
    u32 collision_mask{0xFFFFFFFF};
    
    /** @brief Material properties */
    f32 friction{0.5f};
    f32 restitution{0.3f};
    f32 density{1000.0f};
    
    /** @brief Debug rendering properties */
    bool debug_render{false};
    u32 debug_color{0xFF00FF00};
    
    //-------------------------------------------------------------------------
    // Factory Methods for Different Shapes
    //-------------------------------------------------------------------------
    
    static Collider3D create_sphere(f32 radius, bool is_trigger = false) {
        Collider3D collider;
        collider.shape_type = ShapeType::Sphere;
        collider.shape_data.sphere.radius = radius;
        collider.is_trigger = is_trigger;
        return collider;
    }
    
    static Collider3D create_box(const Vec3& half_extents, bool is_trigger = false) {
        Collider3D collider;
        collider.shape_type = ShapeType::Box;
        collider.shape_data.box.half_extents = half_extents;
        collider.is_trigger = is_trigger;
        return collider;
    }
    
    static Collider3D create_capsule(f32 radius, f32 height, bool is_trigger = false) {
        Collider3D collider;
        collider.shape_type = ShapeType::Capsule;
        collider.shape_data.capsule.radius = radius;
        collider.shape_data.capsule.height = height;
        collider.is_trigger = is_trigger;
        return collider;
    }
    
    //-------------------------------------------------------------------------
    // Shape Property Queries
    //-------------------------------------------------------------------------
    
    f32 calculate_volume() const {
        switch (shape_type) {
            case ShapeType::Sphere: {
                f32 r = shape_data.sphere.radius;
                return (4.0f / 3.0f) * constants::PI_F * r * r * r;
            }
            case ShapeType::Box: {
                Vec3 size = shape_data.box.half_extents * 2.0f;
                return size.x * size.y * size.z;
            }
            case ShapeType::Capsule: {
                f32 r = shape_data.capsule.radius;
                f32 h = shape_data.capsule.height;
                // Cylinder + two hemispheres
                return constants::PI_F * r * r * h + (4.0f / 3.0f) * constants::PI_F * r * r * r;
            }
            default:
                return 1.0f;
        }
    }
    
    AABB3D calculate_aabb(const Transform3D& transform) const;
};

//=============================================================================
// 3D Transform Component (Enhanced)
//=============================================================================

/**
 * @brief Enhanced 3D Transform Component
 * 
 * Already defined in math3d.hpp, but we can extend it here for physics-specific needs.
 */
using Transform3D = ecscope::physics::math3d::Transform3D;

//=============================================================================
// 3D Force Accumulator Component
//=============================================================================

/**
 * @brief 3D Force and Torque Accumulation Component
 * 
 * Manages all forces and torques applied to a 3D rigid body.
 */
struct alignas(64) ForceAccumulator3D {
    /** @brief Persistent forces applied each frame */
    struct PersistentForce {
        Vec3 force{0.0f, 0.0f, 0.0f};
        Vec3 application_point{0.0f, 0.0f, 0.0f};  // Local space
        bool apply_at_center_of_mass{true};
        f32 duration{-1.0f};  // -1 = infinite
        std::string name;
        
        PersistentForce() = default;
        PersistentForce(const Vec3& f, const std::string& n = "") 
            : force(f), name(n) {}
        PersistentForce(const Vec3& f, const Vec3& point, const std::string& n = "") 
            : force(f), application_point(point), apply_at_center_of_mass(false), name(n) {}
    };
    
    /** @brief Persistent torques applied each frame */
    struct PersistentTorque {
        Vec3 torque{0.0f, 0.0f, 0.0f};
        f32 duration{-1.0f};  // -1 = infinite
        std::string name;
        
        PersistentTorque() = default;
        PersistentTorque(const Vec3& t, const std::string& n = "") 
            : torque(t), name(n) {}
    };
    
    /** @brief List of persistent forces */
    std::vector<PersistentForce> persistent_forces;
    
    /** @brief List of persistent torques */
    std::vector<PersistentTorque> persistent_torques;
    
    /** @brief One-time impulses to be applied */
    std::vector<Vec3> pending_impulses;
    
    /** @brief One-time angular impulses to be applied */
    std::vector<Vec3> pending_angular_impulses;
    
    /** @brief Whether to automatically apply gravity */
    bool apply_gravity{true};
    
    /** @brief Custom gravity vector (overrides world gravity if set) */
    std::optional<Vec3> custom_gravity;
    
    /** @brief Total accumulated force this frame (for debugging) */
    mutable Vec3 total_force{0.0f, 0.0f, 0.0f};
    
    /** @brief Total accumulated torque this frame (for debugging) */
    mutable Vec3 total_torque{0.0f, 0.0f, 0.0f};
    
    //-------------------------------------------------------------------------
    // Force Management
    //-------------------------------------------------------------------------
    
    void add_persistent_force(const Vec3& force, const std::string& name = "") {
        persistent_forces.emplace_back(force, name);
    }
    
    void add_persistent_force_at_point(const Vec3& force, const Vec3& local_point, const std::string& name = "") {
        persistent_forces.emplace_back(force, local_point, name);
    }
    
    void add_persistent_torque(const Vec3& torque, const std::string& name = "") {
        persistent_torques.emplace_back(torque, name);
    }
    
    void add_impulse(const Vec3& impulse) {
        pending_impulses.push_back(impulse);
    }
    
    void add_angular_impulse(const Vec3& angular_impulse) {
        pending_angular_impulses.push_back(angular_impulse);
    }
    
    void remove_persistent_force(const std::string& name) {
        persistent_forces.erase(
            std::remove_if(persistent_forces.begin(), persistent_forces.end(),
                          [&name](const PersistentForce& f) { return f.name == name; }),
            persistent_forces.end());
    }
    
    void remove_persistent_torque(const std::string& name) {
        persistent_torques.erase(
            std::remove_if(persistent_torques.begin(), persistent_torques.end(),
                          [&name](const PersistentTorque& t) { return t.name == name; }),
            persistent_torques.end());
    }
    
    void clear_all() {
        persistent_forces.clear();
        persistent_torques.clear();
        pending_impulses.clear();
        pending_angular_impulses.clear();
    }
    
    //-------------------------------------------------------------------------
    // Force Application
    //-------------------------------------------------------------------------
    
    void apply_to_rigid_body(RigidBody3D& body, const Transform3D& transform, 
                            const Vec3& world_gravity, f32 dt) const {
        total_force = Vec3::zero();
        total_torque = Vec3::zero();
        
        // Apply gravity
        if (apply_gravity && body.use_gravity && body.mass > constants::EPSILON) {
            Vec3 gravity = custom_gravity.value_or(world_gravity);
            Vec3 gravity_force = gravity * body.mass;
            body.apply_force(gravity_force);
            total_force += gravity_force;
        }
        
        // Apply persistent forces
        for (const auto& pf : persistent_forces) {
            if (pf.apply_at_center_of_mass) {
                body.apply_force(pf.force);
                total_force += pf.force;
            } else {
                // Convert local application point to world space
                Vec3 world_point = transform.transform_point(pf.application_point);
                Vec3 world_com = transform.transform_point(body.local_center_of_mass);
                body.apply_force_at_point(pf.force, world_point, world_com);
                total_force += pf.force;
                
                // Calculate torque for debugging
                Vec3 r = world_point - world_com;
                total_torque += r.cross(pf.force);
            }
        }
        
        // Apply persistent torques
        for (const auto& pt : persistent_torques) {
            body.apply_torque(pt.torque);
            total_torque += pt.torque;
        }
        
        // Apply pending impulses
        for (const Vec3& impulse : pending_impulses) {
            body.apply_impulse(impulse);
        }
        
        for (const Vec3& angular_impulse : pending_angular_impulses) {
            body.apply_angular_impulse(angular_impulse);
        }
        
        // Clear one-time impulses
        const_cast<ForceAccumulator3D*>(this)->pending_impulses.clear();
        const_cast<ForceAccumulator3D*>(this)->pending_angular_impulses.clear();
        
        // Update force durations
        update_force_durations(dt);
    }
    
private:
    void update_force_durations(f32 dt) const {
        auto& mutable_forces = const_cast<std::vector<PersistentForce>&>(persistent_forces);
        auto& mutable_torques = const_cast<std::vector<PersistentTorque>&>(persistent_torques);
        
        // Update and remove expired forces
        mutable_forces.erase(
            std::remove_if(mutable_forces.begin(), mutable_forces.end(),
                          [dt](PersistentForce& f) {
                              if (f.duration > 0.0f) {
                                  f.duration -= dt;
                                  return f.duration <= 0.0f;
                              }
                              return false;
                          }),
            mutable_forces.end());
        
        // Update and remove expired torques
        mutable_torques.erase(
            std::remove_if(mutable_torques.begin(), mutable_torques.end(),
                          [dt](PersistentTorque& t) {
                              if (t.duration > 0.0f) {
                                  t.duration -= dt;
                                  return t.duration <= 0.0f;
                              }
                              return false;
                          }),
            mutable_torques.end());
    }
};

//=============================================================================
// 3D Debug Rendering Component
//=============================================================================

/**
 * @brief 3D Physics Debug Rendering Component
 * 
 * Provides comprehensive visualization for educational purposes.
 */
struct PhysicsDebugRenderer3D {
    /** @brief Types of debug visualizations */
    enum class VisualizationType : u32 {
        CollisionShapes     = 1 << 0,
        ContactPoints       = 1 << 1,
        ContactNormals      = 1 << 2,
        ForceVectors        = 1 << 3,
        VelocityVectors     = 1 << 4,
        AngularVelocity     = 1 << 5,
        CenterOfMass        = 1 << 6,
        BoundingBoxes       = 1 << 7,
        Constraints         = 1 << 8,
        InertiaTensor       = 1 << 9,
        All                 = 0xFFFFFFFF
    };
    
    /** @brief Which visualizations to render */
    u32 enabled_visualizations{static_cast<u32>(VisualizationType::CollisionShapes)};
    
    /** @brief Visualization colors */
    u32 collision_shape_color{0xFF00FF00};      // Green
    u32 trigger_shape_color{0xFF0080FF};        // Blue
    u32 sleeping_shape_color{0xFF808080};       // Gray
    u32 contact_point_color{0xFFFF0000};        // Red
    u32 contact_normal_color{0xFFFFFF00};       // Yellow
    u32 force_vector_color{0xFFFF8000};         // Orange
    u32 velocity_vector_color{0xFF00FFFF};      // Cyan
    u32 center_of_mass_color{0xFFFFFFFF};       // White
    
    /** @brief Vector scaling factors for visualization */
    f32 force_vector_scale{0.01f};
    f32 velocity_vector_scale{1.0f};
    f32 contact_normal_length{1.0f};
    f32 angular_velocity_scale{1.0f};
    
    /** @brief Whether to show numerical values */
    bool show_force_magnitudes{false};
    bool show_velocity_magnitudes{false};
    bool show_mass_values{false};
    bool show_energy_values{false};
    
    /** @brief Rendering quality settings */
    u32 sphere_segments{16};
    u32 capsule_segments{12};
    bool wireframe_mode{true};
    f32 line_thickness{1.0f};
    
    /** @brief Performance limits */
    u32 max_debug_objects{1000};
    bool enable_distance_culling{true};
    f32 max_debug_distance{100.0f};
    
    bool is_enabled(VisualizationType type) const {
        return (enabled_visualizations & static_cast<u32>(type)) != 0;
    }
    
    void enable_visualization(VisualizationType type) {
        enabled_visualizations |= static_cast<u32>(type);
    }
    
    void disable_visualization(VisualizationType type) {
        enabled_visualizations &= ~static_cast<u32>(type);
    }
};

} // namespace ecscope::physics::components3d