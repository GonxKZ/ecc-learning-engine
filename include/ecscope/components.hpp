#pragma once

/**
 * @file physics/components.hpp
 * @brief Comprehensive Physics Components for ECScope Educational ECS Engine - Phase 5: Física 2D
 * 
 * This header provides a complete set of physics components for 2D simulation with emphasis
 * on educational clarity while maintaining high performance. It includes:
 * 
 * Core Physics Components:
 * - RigidBody2D: Complete rigid body dynamics with mass, velocity, forces
 * - Collider2D: Multi-shape collision detection with material properties
 * - PhysicsMaterial: Material properties for realistic physics simulation
 * - ForceAccumulator: Force and torque accumulation for integration
 * 
 * Advanced Physics Components:
 * - Constraint2D: Base for physics constraints (joints, springs)
 * - Trigger2D: Collision detection without physics response
 * - PhysicsInfo: Debug and performance information
 * - MotionState: Cached physics state for optimization
 * 
 * Educational Philosophy:
 * Each component includes detailed educational documentation explaining the physics
 * concepts, mathematical foundations, and practical applications. Components are
 * designed to be inspectable, debuggable, and easy to understand.
 * 
 * Performance Characteristics:
 * - SoA-friendly data layouts for cache efficiency
 * - Memory-aligned structures for optimal SIMD usage
 * - Integration with custom allocators for minimal overhead
 * - Zero-overhead abstractions where possible
 * - Educational debugging without performance impact in release builds
 * 
 * ECS Integration:
 * - Full compatibility with Component concept
 * - Works seamlessly with existing Transform component
 * - Integrates with memory tracking system
 * - Supports component relationships and hierarchies
 * 
 * @author ECScope Educational ECS Framework - Phase 5: Física 2D
 * @date 2024
 */

#include "../ecs/component.hpp"
#include "../ecs/components/transform.hpp"
#include "../memory/memory_tracker.hpp"
#include "math.hpp"
#include "../core/types.hpp"
#include <array>
#include <vector>
#include <optional>
#include <variant>
#include <bitset>

namespace ecscope::physics::components {

// Import commonly used types
using namespace ecscope::ecs::components;
using namespace ecscope::physics::math;

//=============================================================================
// Physics Component Categories (for memory tracking)
//=============================================================================

namespace categories {
    constexpr memory::AllocationCategory RIGID_BODY = memory::AllocationCategory::Physics_Bodies;
    constexpr memory::AllocationCategory COLLIDER = memory::AllocationCategory::Physics_Collision;
    constexpr memory::AllocationCategory CONSTRAINTS = memory::AllocationCategory::Physics_Bodies;
    constexpr memory::AllocationCategory DEBUG_INFO = memory::AllocationCategory::Debug_Tools;
}

//=============================================================================
// Physics Material Component
//=============================================================================

/**
 * @brief Physics Material Properties Component
 * 
 * Defines the physical properties of objects that determine how they interact
 * during collisions and contact resolution. This component encapsulates the
 * material science aspects of physics simulation.
 * 
 * Educational Context:
 * - Restitution: How "bouncy" a material is (0 = perfectly inelastic, 1 = perfectly elastic)
 * - Friction: Resistance to sliding motion (static and kinetic friction coefficients)
 * - Density: Mass per unit volume, used to calculate mass from volume
 * 
 * Real-world Applications:
 * - Rubber ball: high restitution (~0.8), medium friction
 * - Ice: low restitution (~0.1), very low friction (~0.02)
 * - Cork: medium restitution (~0.5), high friction (~0.68)
 * - Steel: low restitution (~0.2), medium friction (~0.4)
 */
struct alignas(16) PhysicsMaterial {
    //-------------------------------------------------------------------------
    // Core Material Properties
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Coefficient of restitution (bounciness)
     * 
     * Mathematical Definition: e = sqrt(h_bounce / h_drop)
     * Where h_bounce is bounce height and h_drop is drop height
     * 
     * Range: [0.0, 1.0]
     * - 0.0: Perfectly inelastic collision (no bounce)
     * - 1.0: Perfectly elastic collision (perfect bounce)
     * 
     * Educational Note:
     * Real materials never achieve perfect elasticity due to energy loss
     * through heat, sound, and deformation. Super balls can reach ~0.9.
     */
    f32 restitution{0.3f};
    
    /** 
     * @brief Static friction coefficient
     * 
     * Opposes the initiation of sliding motion. Must be overcome for an object
     * at rest to start sliding. Typically higher than kinetic friction.
     * 
     * Mathematical Application: F_static_max = μ_s * N
     * Where N is the normal force
     * 
     * Range: [0.0, 2.0] (can exceed 1.0 for very rough surfaces)
     * Examples:
     * - Ice on ice: ~0.02
     * - Rubber on concrete: ~1.0
     * - Steel on steel: ~0.7
     */
    f32 static_friction{0.6f};
    
    /** 
     * @brief Kinetic friction coefficient
     * 
     * Opposes sliding motion once it has begun. Usually lower than static friction,
     * which is why objects are easier to keep sliding than to start sliding.
     * 
     * Mathematical Application: F_kinetic = μ_k * N
     * 
     * Educational Insight:
     * The difference between static and kinetic friction explains the "stick-slip"
     * phenomenon observed in many mechanical systems.
     */
    f32 kinetic_friction{0.4f};
    
    /** 
     * @brief Material density (kg/m³)
     * 
     * Used to automatically calculate mass from volume when combined with collider shapes.
     * Enables realistic mass distribution without manual mass specification.
     * 
     * Common Material Densities:
     * - Water: 1000 kg/m³
     * - Steel: 7850 kg/m³  
     * - Aluminum: 2700 kg/m³
     * - Wood (pine): 500 kg/m³
     * - Cork: 240 kg/m³
     * - Air: 1.225 kg/m³
     */
    f32 density{1000.0f};
    
    //-------------------------------------------------------------------------
    // Advanced Material Properties
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Surface roughness factor
     * 
     * Affects rolling resistance and detailed collision calculations.
     * Used in advanced physics models for more realistic simulation.
     * 
     * Range: [0.0, 1.0]
     * - 0.0: Perfectly smooth surface
     * - 1.0: Very rough surface
     */
    f32 surface_roughness{0.1f};
    
    /** 
     * @brief Material hardness (Mohs scale normalized)
     * 
     * Affects deformation during collisions and wear simulation.
     * Normalized from Mohs hardness scale (1-10) to (0.1-1.0).
     */
    f32 hardness{0.5f};
    
    /** 
     * @brief Thermal conductivity (for advanced simulations)
     * 
     * Used in thermodynamics simulations where friction generates heat.
     * Higher values mean heat spreads faster through the material.
     */
    f32 thermal_conductivity{0.1f};
    
    /** 
     * @brief Material flags for special behaviors
     */
    union {
        u32 flags;
        struct {
            u32 is_liquid : 1;          ///< Behaves as liquid (affects buoyancy)
            u32 is_granular : 1;        ///< Granular material (sand, gravel)
            u32 is_magnetic : 1;        ///< Affected by magnetic fields
            u32 is_conductive : 1;      ///< Electrically conductive
            u32 is_fragile : 1;         ///< Can break under stress
            u32 is_elastic : 1;         ///< Can deform and return to shape
            u32 generates_sparks : 1;   ///< Generates sparks on collision
            u32 reserved : 25;          ///< Reserved for future use
        };
    } material_flags{0};
    
    //-------------------------------------------------------------------------
    // Constructors and Factory Methods
    //-------------------------------------------------------------------------
    
    constexpr PhysicsMaterial() noexcept = default;
    
    constexpr PhysicsMaterial(f32 rest, f32 static_fric, f32 kinetic_fric, f32 dens) noexcept
        : restitution(rest), static_friction(static_fric), kinetic_friction(kinetic_fric), density(dens) {}
    
    // Predefined material presets for educational purposes
    static constexpr PhysicsMaterial rubber() noexcept {
        PhysicsMaterial mat{0.8f, 1.0f, 0.7f, 920.0f};
        mat.surface_roughness = 0.3f;
        mat.hardness = 0.2f;
        return mat;
    }
    
    static constexpr PhysicsMaterial steel() noexcept {
        PhysicsMaterial mat{0.2f, 0.7f, 0.4f, 7850.0f};
        mat.surface_roughness = 0.05f;
        mat.hardness = 0.8f;
        mat.material_flags.generates_sparks = 1;
        return mat;
    }
    
    static constexpr PhysicsMaterial ice() noexcept {
        PhysicsMaterial mat{0.1f, 0.02f, 0.01f, 917.0f};
        mat.surface_roughness = 0.01f;
        mat.hardness = 0.3f;
        mat.material_flags.is_fragile = 1;
        return mat;
    }
    
    static constexpr PhysicsMaterial wood() noexcept {
        PhysicsMaterial mat{0.4f, 0.6f, 0.4f, 500.0f};
        mat.surface_roughness = 0.2f;
        mat.hardness = 0.4f;
        return mat;
    }
    
    static constexpr PhysicsMaterial cork() noexcept {
        PhysicsMaterial mat{0.5f, 0.68f, 0.5f, 240.0f};
        mat.surface_roughness = 0.4f;
        mat.hardness = 0.1f;
        return mat;
    }
    
    //-------------------------------------------------------------------------
    // Material Combination Functions
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Combine two materials for contact resolution
     * 
     * Uses physics-based combination rules:
     * - Restitution: Minimum of both values (weakest link principle)
     * - Friction: Geometric mean (realistic for contact mechanics)
     */
    static PhysicsMaterial combine(const PhysicsMaterial& a, const PhysicsMaterial& b) noexcept;
    
    //-------------------------------------------------------------------------
    // Validation and Debugging
    //-------------------------------------------------------------------------
    
    /** @brief Validate material properties are within reasonable ranges */
    bool is_valid() const noexcept {
        return restitution >= 0.0f && restitution <= 1.0f &&
               static_friction >= 0.0f && static_friction <= 2.0f &&
               kinetic_friction >= 0.0f && kinetic_friction <= static_friction &&
               density > 0.0f && density < 20000.0f &&  // Reasonable density range
               surface_roughness >= 0.0f && surface_roughness <= 1.0f &&
               hardness >= 0.0f && hardness <= 1.0f;
    }
    
    /** @brief Get material description for educational display */
    const char* get_material_description() const noexcept;
    
    /** @brief Calculate approximate rolling resistance coefficient */
    f32 get_rolling_resistance() const noexcept {
        return surface_roughness * 0.01f + (1.0f - hardness) * 0.005f;
    }
};

//=============================================================================
// Collision Shape Variants
//=============================================================================

/**
 * @brief Shape variants for collision detection
 * 
 * Uses std::variant for type-safe, cache-friendly shape storage.
 * Each shape type includes educational information about its use cases
 * and performance characteristics.
 */
using CollisionShape = std::variant<
    Circle,         ///< Best performance, simple math, good for particles/balls
    AABB,          ///< Very fast broad-phase, good for static geometry
    OBB,           ///< More expensive than AABB, better fit for rotated objects
    Polygon        ///< Most flexible, most expensive, good for complex shapes
>;

//=============================================================================
// Collider Component
//=============================================================================

/**
 * @brief 2D Collision Detection Component
 * 
 * Handles collision shape definition and collision detection properties.
 * Supports multiple collision shapes for complex objects and provides
 * comprehensive educational information about collision detection.
 * 
 * Educational Context:
 * This component demonstrates:
 * - Different collision primitives and their trade-offs
 * - Broad-phase vs narrow-phase collision detection
 * - Material property integration with collision response
 * - Multi-shape collision for complex objects
 * - Trigger vs solid collision semantics
 * 
 * Performance Design:
 * - Uses std::variant for type-safe shape storage
 * - Cache-friendly layout with hot data first
 * - Supports multiple shapes for complex collision geometry
 * - Integrates with spatial partitioning systems
 */
struct alignas(32) Collider2D {
    //-------------------------------------------------------------------------
    // Primary Collision Shape
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Primary collision shape
     * 
     * Most collision objects have a single primary shape. This is stored
     * inline for optimal cache performance.
     * 
     * Shape Selection Guidelines:
     * - Circle: Fastest, use for balls, particles, circular objects
     * - AABB: Very fast, use for axis-aligned boxes, UI elements
     * - OBB: Good balance, use for rotated rectangular objects
     * - Polygon: Most flexible, use for complex shapes (max 16 vertices)
     */
    CollisionShape shape;
    
    /** 
     * @brief Local offset from entity transform
     * 
     * Allows the collision shape to be positioned relative to the entity's
     * transform. Useful for objects where the visual center doesn't match
     * the collision center.
     */
    Vec2 offset{0.0f, 0.0f};
    
    //-------------------------------------------------------------------------
    // Material Properties
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Physics material for this collider
     * 
     * Stored by value for cache efficiency. Contains all material properties
     * needed for realistic collision response.
     */
    PhysicsMaterial material{};
    
    //-------------------------------------------------------------------------
    // Collision Detection Properties
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Collision detection layers (bitmask)
     * 
     * Used for collision filtering. Objects only collide if their layers
     * have overlapping bits. Supports up to 32 different collision layers.
     * 
     * Common Layer Setup:
     * - Bit 0: Player
     * - Bit 1: Enemies
     * - Bit 2: Environment
     * - Bit 3: Triggers
     * - Bit 4: Projectiles
     * 
     * Educational Example:
     * Player bullets (layer 5) might collide with enemies (layer 1)
     * but not with player (layer 0) or other bullets (layer 5).
     */
    u32 collision_layers{0xFFFFFFFF};  // Default: collide with everything
    
    /** 
     * @brief Collision mask (what this collider can collide with)
     * 
     * Separate from layers for fine-grained control. The collision occurs if:
     * (objectA.layers & objectB.mask) != 0 AND (objectB.layers & objectA.mask) != 0
     */
    u32 collision_mask{0xFFFFFFFF};   // Default: collide with everything
    
    /** 
     * @brief Collision behavior flags
     */
    union {
        u32 flags;
        struct {
            u32 is_trigger : 1;         ///< Trigger mode (no collision response)
            u32 is_sensor : 1;          ///< Sensor mode (detect but don't respond)
            u32 is_static : 1;          ///< Static collider (doesn't move)
            u32 is_kinematic : 1;       ///< Kinematic (moves but unaffected by physics)
            u32 generate_events : 1;    ///< Generate collision events
            u32 continuous_collision : 1; ///< Use continuous collision detection
            u32 one_way_collision : 1;  ///< One-way platform collision
            u32 ignore_gravity : 1;     ///< Not affected by gravity
            u32 ghost_mode : 1;         ///< Can pass through solid objects
            u32 high_precision : 1;     ///< Use high-precision collision detection
            u32 reserved : 22;          ///< Reserved for future use
        };
    } collision_flags{0};
    
    //-------------------------------------------------------------------------
    // Multi-Shape Support (Advanced)
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Additional collision shapes for complex objects
     * 
     * Most objects only need the primary shape, but complex objects
     * (like a character with separate head/body/limb collision) can
     * use multiple shapes. Stored separately to keep the primary
     * component cache-friendly.
     */
    std::vector<CollisionShape>* additional_shapes{nullptr};
    
    /** 
     * @brief Offsets for additional shapes
     */
    std::vector<Vec2>* additional_offsets{nullptr};
    
    //-------------------------------------------------------------------------
    // Performance and Debug Information
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Performance metrics for educational analysis
     */
    mutable struct {
        u32 collision_checks_count{0};     ///< Number of collision checks performed
        f32 last_check_duration{0.0f};     ///< Duration of last collision check
        f32 total_check_duration{0.0f};    ///< Total time spent in collision checks
        u32 cache_hits{0};                 ///< Spatial cache hits
        u32 cache_misses{0};               ///< Spatial cache misses
    } performance_info;
    
    /** 
     * @brief Educational debug information
     */
    mutable struct {
        Vec2 last_collision_point{0.0f, 0.0f};  ///< Last collision point
        Vec2 last_collision_normal{0.0f, 0.0f}; ///< Last collision normal
        f32 last_collision_depth{0.0f};         ///< Last penetration depth
        u32 active_contacts{0};                 ///< Currently active contacts
        f32 contact_lifetime{0.0f};             ///< How long contact has lasted
    } debug_info;
    
    //-------------------------------------------------------------------------
    // Constructors
    //-------------------------------------------------------------------------
    
    constexpr Collider2D() noexcept = default;
    
    explicit Collider2D(CollisionShape shape_param, Vec2 offset_param = Vec2::zero()) noexcept
        : shape(shape_param), offset(offset_param) {}
    
    Collider2D(CollisionShape shape_param, const PhysicsMaterial& material_param, 
               Vec2 offset_param = Vec2::zero()) noexcept
        : shape(shape_param), offset(offset_param), material(material_param) {}
    
    //-------------------------------------------------------------------------
    // Shape Access and Manipulation
    //-------------------------------------------------------------------------
    
    /** @brief Get the primary shape type index */
    usize get_shape_type() const noexcept {
        return shape.index();
    }
    
    /** @brief Get shape type name for debugging */
    const char* get_shape_name() const noexcept;
    
    /** @brief Check if collider has multiple shapes */
    bool has_multiple_shapes() const noexcept {
        return additional_shapes != nullptr && !additional_shapes->empty();
    }
    
    /** @brief Get total number of shapes (primary + additional) */
    usize get_shape_count() const noexcept {
        return 1 + (additional_shapes ? additional_shapes->size() : 0);
    }
    
    /** @brief Add an additional collision shape */
    void add_shape(const CollisionShape& new_shape, Vec2 shape_offset = Vec2::zero());
    
    /** @brief Remove all additional shapes */
    void clear_additional_shapes() noexcept;
    
    //-------------------------------------------------------------------------
    // Collision Testing Interface
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Test if this collider can collide with another
     * 
     * Checks layer masks and collision flags to determine if collision
     * detection should be performed between two colliders.
     */
    bool can_collide_with(const Collider2D& other) const noexcept {
        return (collision_layers & other.collision_mask) != 0 &&
               (other.collision_layers & collision_mask) != 0 &&
               !collision_flags.ghost_mode && !other.collision_flags.ghost_mode;
    }
    
    /** 
     * @brief Get transformed collision shape in world space
     * 
     * Applies entity transform and local offset to get world-space collision shape.
     */
    CollisionShape get_world_shape(const Transform& entity_transform) const noexcept;
    
    /** 
     * @brief Get world-space AABB for broad-phase collision
     * 
     * Returns axis-aligned bounding box in world coordinates for efficient
     * broad-phase collision culling.
     */
    AABB get_world_aabb(const Transform& entity_transform) const noexcept;
    
    //-------------------------------------------------------------------------
    // Educational Utilities
    //-------------------------------------------------------------------------
    
    /** @brief Calculate approximate collision detection cost */
    f32 estimate_collision_cost() const noexcept;
    
    /** @brief Get detailed shape information for educational display */
    struct ShapeInfo {
        const char* type_name;
        f32 area;
        f32 perimeter;
        Vec2 centroid;
        f32 moment_of_inertia;
        u32 complexity_score;  // 1-10 scale
    };
    
    ShapeInfo get_shape_info() const noexcept;
    
    /** @brief Validate collider configuration */
    bool is_valid() const noexcept;
    
    //-------------------------------------------------------------------------
    // Memory Management
    //-------------------------------------------------------------------------
    
    /** @brief Destructor handles additional shapes cleanup */
    ~Collider2D() noexcept;
    
    // Copy and move operations handle additional shapes properly
    Collider2D(const Collider2D& other);
    Collider2D& operator=(const Collider2D& other);
    Collider2D(Collider2D&& other) noexcept;
    Collider2D& operator=(Collider2D&& other) noexcept;
    
private:
    void cleanup_additional_shapes() noexcept;
    void copy_additional_shapes(const Collider2D& other);
    void move_additional_shapes(Collider2D& other) noexcept;
};

//=============================================================================
// Rigid Body Component
//=============================================================================

/**
 * @brief 2D Rigid Body Physics Component
 * 
 * Implements complete 2D rigid body dynamics including linear and angular motion,
 * force accumulation, and physics integration. This component demonstrates
 * fundamental physics concepts in an educational and performant manner.
 * 
 * Educational Context:
 * This component teaches:
 * - Newton's laws of motion in practice
 * - The relationship between force, mass, and acceleration (F = ma)
 * - Angular dynamics and moment of inertia
 * - Integration methods for physics simulation
 * - Damping and energy loss in real systems
 * - The difference between linear and angular quantities
 * 
 * Physics Foundations:
 * Linear Motion:  F = ma, v = ∫a dt, p = ∫v dt
 * Angular Motion: τ = Iα, ω = ∫α dt, θ = ∫ω dt
 * Where: F=force, m=mass, a=acceleration, v=velocity, p=position
 *        τ=torque, I=moment of inertia, α=angular acceleration, ω=angular velocity, θ=angle
 */
struct alignas(32) RigidBody2D {
    //-------------------------------------------------------------------------
    // Mass Properties
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Object mass in kilograms
     * 
     * Fundamental property determining how forces affect motion.
     * - Higher mass = harder to accelerate
     * - Infinite mass (0 or negative) = static object
     * 
     * Educational Note:
     * Mass vs Weight: Mass is intrinsic property (kg), weight is force due to gravity (N).
     * In space, objects have mass but no weight.
     */
    f32 mass{1.0f};
    
    /** 
     * @brief Inverse mass (1/mass) for performance
     * 
     * Stored for efficiency since division is expensive.
     * - 0.0 = infinite mass (static object)
     * - Positive value = dynamic object
     * 
     * Educational Insight:
     * Many physics engines store inverse mass because most calculations
     * need to divide by mass, making multiplication faster.
     */
    f32 inverse_mass{1.0f};
    
    /** 
     * @brief Moment of inertia (rotational mass)
     * 
     * Rotational equivalent of mass. Determines how torques affect angular acceleration.
     * Units: kg⋅m² 
     * 
     * Shape Dependencies:
     * - Circle: I = (1/2) * m * r²
     * - Rectangle: I = (1/12) * m * (w² + h²)
     * - Point mass at distance r: I = m * r²
     * 
     * Educational Context:
     * Objects with mass concentrated far from center (like a wheel) have
     * high moment of inertia and resist rotational changes more.
     */
    f32 moment_of_inertia{1.0f};
    
    /** 
     * @brief Inverse moment of inertia (1/I) for performance
     * 
     * - 0.0 = infinite rotational inertia (no rotation)
     * - Positive value = can rotate
     */
    f32 inverse_moment_of_inertia{1.0f};
    
    //-------------------------------------------------------------------------
    // Linear Motion State
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Current velocity in m/s
     * 
     * Rate of change of position. First derivative of position.
     * Vector quantity with both magnitude (speed) and direction.
     */
    Vec2 velocity{0.0f, 0.0f};
    
    /** 
     * @brief Current acceleration in m/s²
     * 
     * Rate of change of velocity. Second derivative of position.
     * Directly related to net force by Newton's second law: a = F/m
     */
    Vec2 acceleration{0.0f, 0.0f};
    
    /** 
     * @brief Previous position for integration
     * 
     * Used in Verlet integration and for calculating velocity from position changes.
     * Enables more stable physics simulation than Euler integration.
     */
    Vec2 previous_position{0.0f, 0.0f};
    
    //-------------------------------------------------------------------------
    // Angular Motion State  
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Angular velocity in radians per second
     * 
     * Rate of rotation. Positive values = counter-clockwise rotation.
     * Related to linear velocity at a point: v = ω × r
     */
    f32 angular_velocity{0.0f};
    
    /** 
     * @brief Angular acceleration in radians per second²
     * 
     * Rate of change of angular velocity.
     * Related to torque by: α = τ / I
     */
    f32 angular_acceleration{0.0f};
    
    /** 
     * @brief Previous rotation for integration
     * 
     * Used in rotational Verlet integration for stability.
     */
    f32 previous_rotation{0.0f};
    
    //-------------------------------------------------------------------------
    // Physics Behavior Control
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Linear damping coefficient (0.0 - 1.0)
     * 
     * Simulates air resistance, friction, and other velocity-dependent forces.
     * - 0.0 = no damping (objects move forever)
     * - 1.0 = maximum damping (objects stop quickly)
     * 
     * Mathematical Model: F_drag = -damping * velocity
     * 
     * Educational Note:
     * Real-world drag force is usually proportional to v² for high speeds,
     * but linear damping is simpler and sufficient for most games.
     */
    f32 linear_damping{0.01f};
    
    /** 
     * @brief Angular damping coefficient (0.0 - 1.0)
     * 
     * Simulates rotational friction and air resistance affecting rotation.
     * Applied as: τ_drag = -angular_damping * angular_velocity
     */
    f32 angular_damping{0.01f};
    
    /** 
     * @brief Gravity scale multiplier
     * 
     * Multiplier for global gravity affecting this object.
     * - 1.0 = normal gravity
     * - 0.0 = unaffected by gravity (like space objects)
     * - -1.0 = reverse gravity (floats upward)
     * 
     * Educational Use:
     * Demonstrates how gravity affects different objects and enables
     * experimentation with different gravitational effects.
     */
    f32 gravity_scale{1.0f};
    
    //-------------------------------------------------------------------------
    // Physics Constraints and Limits
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Maximum linear velocity (m/s)
     * 
     * Prevents unrealistic velocities that can break physics simulation.
     * Set to 0 for unlimited velocity.
     */
    f32 max_velocity{100.0f};
    
    /** 
     * @brief Maximum angular velocity (rad/s)
     * 
     * Prevents unrealistic rotation speeds. Approximately 30 rad/s = 1800°/s.
     */
    f32 max_angular_velocity{50.0f};
    
    /** 
     * @brief Minimum velocity threshold for sleep
     * 
     * Objects with velocity below this threshold can be put to sleep
     * for performance optimization.
     */
    f32 sleep_threshold{0.01f};
    
    //-------------------------------------------------------------------------
    // Physics State Flags
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Physics behavior flags
     */
    union {
        u32 flags;
        struct {
            u32 is_sleeping : 1;            ///< Object is sleeping (not simulated)
            u32 is_static : 1;              ///< Static object (infinite mass, doesn't move)
            u32 is_kinematic : 1;           ///< Kinematic object (moves but unaffected by forces)
            u32 freeze_rotation : 1;        ///< Prevent rotation (useful for characters)
            u32 freeze_position_x : 1;      ///< Lock X position
            u32 freeze_position_y : 1;      ///< Lock Y position  
            u32 ignore_gravity : 1;         ///< Not affected by gravity
            u32 high_precision : 1;         ///< Use high-precision integration
            u32 continuous_collision : 1;   ///< Use continuous collision detection
            u32 auto_sleep : 1;             ///< Automatically sleep when slow
            u32 wake_on_collision : 1;      ///< Wake up when touched by other objects
            u32 reserved : 21;              ///< Reserved for future use
        };
    } physics_flags{0};
    
    //-------------------------------------------------------------------------
    // Performance and Debug Information
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Integration method used
     * 
     * Different integration methods have different stability and performance characteristics.
     * Educational opportunity to compare Euler, RK4, Verlet, etc.
     */
    enum class IntegrationMethod : u8 {
        Euler,          ///< Simple, fast, less stable
        RungeKutta4,    ///< More accurate, slower, very stable
        Verlet,         ///< Good stability, position-based
        LeapFrog        ///< Energy-conserving, good for orbital mechanics
    } integration_method{IntegrationMethod::Verlet};
    
    /** 
     * @brief Sleep timer for performance optimization
     * 
     * Tracks how long the object has been below sleep threshold.
     */
    f32 sleep_timer{0.0f};
    
    /** 
     * @brief Performance metrics for educational analysis
     */
    mutable struct {
        u32 integration_steps{0};           ///< Number of integration steps performed
        f32 total_integration_time{0.0f};   ///< Total time spent integrating
        f32 kinetic_energy{0.0f};           ///< Current kinetic energy
        f32 potential_energy{0.0f};         ///< Current potential energy
        f32 total_energy{0.0f};             ///< Total mechanical energy
    } performance_info;
    
    //-------------------------------------------------------------------------
    // Constructors
    //-------------------------------------------------------------------------
    
    constexpr RigidBody2D() noexcept {
        update_derived_values();
    }
    
    explicit constexpr RigidBody2D(f32 mass_param) noexcept : mass(mass_param) {
        update_derived_values();
    }
    
    constexpr RigidBody2D(f32 mass_param, f32 moment_param) noexcept 
        : mass(mass_param), moment_of_inertia(moment_param) {
        update_derived_values();
    }
    
    //-------------------------------------------------------------------------
    // Mass and Inertia Management
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Set mass and update inverse mass
     * 
     * Handles special cases like infinite mass (static objects).
     */
    void set_mass(f32 new_mass) noexcept {
        mass = new_mass;
        inverse_mass = (new_mass > constants::EPSILON) ? 1.0f / new_mass : 0.0f;
        
        // Static objects are also kinematic
        if (inverse_mass == 0.0f) {
            physics_flags.is_static = 1;
            physics_flags.is_kinematic = 1;
        }
    }
    
    /** 
     * @brief Set moment of inertia and update inverse
     */
    void set_moment_of_inertia(f32 new_moment) noexcept {
        moment_of_inertia = new_moment;
        inverse_moment_of_inertia = (new_moment > constants::EPSILON) ? 1.0f / new_moment : 0.0f;
    }
    
    /** 
     * @brief Calculate moment of inertia from shape and mass
     * 
     * Automatically calculates appropriate moment of inertia based on
     * collision shape and current mass.
     */
    void calculate_moment_of_inertia_from_shape(const CollisionShape& shape) noexcept;
    
    /** 
     * @brief Make object static (infinite mass)
     */
    void make_static() noexcept {
        set_mass(0.0f);
        velocity = Vec2::zero();
        angular_velocity = 0.0f;
        physics_flags.is_static = 1;
        physics_flags.is_kinematic = 1;
    }
    
    /** 
     * @brief Make object kinematic (moves but unaffected by forces)
     */
    void make_kinematic() noexcept {
        physics_flags.is_kinematic = 1;
        physics_flags.is_static = 0;
    }
    
    /** 
     * @brief Make object dynamic (normal physics)
     */
    void make_dynamic(f32 new_mass = 1.0f) noexcept {
        set_mass(new_mass);
        physics_flags.is_static = 0;
        physics_flags.is_kinematic = 0;
    }
    
    //-------------------------------------------------------------------------
    // Motion Control
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Set velocity with optional max velocity clamping
     */
    void set_velocity(const Vec2& new_velocity) noexcept {
        velocity = new_velocity;
        if (max_velocity > 0.0f) {
            velocity = vec2::clamp_magnitude(velocity, max_velocity);
        }
        wake_up();
    }
    
    /** 
     * @brief Set angular velocity with clamping
     */
    void set_angular_velocity(f32 new_angular_velocity) noexcept {
        angular_velocity = new_angular_velocity;
        if (max_angular_velocity > 0.0f) {
            angular_velocity = std::clamp(angular_velocity, -max_angular_velocity, max_angular_velocity);
        }
        wake_up();
    }
    
    /** 
     * @brief Add to current velocity
     */
    void add_velocity(const Vec2& delta_velocity) noexcept {
        set_velocity(velocity + delta_velocity);
    }
    
    /** 
     * @brief Add to angular velocity
     */
    void add_angular_velocity(f32 delta_angular_velocity) noexcept {
        set_angular_velocity(angular_velocity + delta_angular_velocity);
    }
    
    /** 
     * @brief Stop all motion
     */
    void stop() noexcept {
        velocity = Vec2::zero();
        angular_velocity = 0.0f;
        acceleration = Vec2::zero();
        angular_acceleration = 0.0f;
        wake_up();
    }
    
    //-------------------------------------------------------------------------
    // Energy Calculations (Educational)
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Calculate current kinetic energy
     * 
     * KE = (1/2) * m * v² + (1/2) * I * ω²
     * Demonstrates energy conservation principles.
     */
    f32 calculate_kinetic_energy() const noexcept {
        f32 linear_ke = 0.5f * mass * velocity.length_squared();
        f32 angular_ke = 0.5f * moment_of_inertia * angular_velocity * angular_velocity;
        return linear_ke + angular_ke;
    }
    
    /** 
     * @brief Calculate potential energy (relative to reference height)
     * 
     * PE = m * g * h
     * Where g is gravity and h is height above reference.
     */
    f32 calculate_potential_energy(f32 reference_height, f32 gravity) const noexcept {
        // Note: Needs current position which is in Transform component
        return mass * gravity * reference_height;  // Simplified for component-only calculation
    }
    
    /** 
     * @brief Get current momentum
     * 
     * Linear momentum: p = m * v
     * Angular momentum: L = I * ω
     */
    Vec2 get_linear_momentum() const noexcept {
        return velocity * mass;
    }
    
    f32 get_angular_momentum() const noexcept {
        return angular_velocity * moment_of_inertia;
    }
    
    //-------------------------------------------------------------------------
    // Sleep System
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Check if object should be sleeping
     * 
     * Objects with very low velocity can be put to sleep to save computation.
     * This is a major performance optimization in physics engines.
     */
    bool should_be_sleeping() const noexcept {
        if (!physics_flags.auto_sleep || physics_flags.is_kinematic) {
            return false;
        }
        
        f32 velocity_sq = velocity.length_squared();
        f32 angular_vel_sq = angular_velocity * angular_velocity;
        f32 threshold_sq = sleep_threshold * sleep_threshold;
        
        return velocity_sq < threshold_sq && angular_vel_sq < threshold_sq;
    }
    
    /** 
     * @brief Put object to sleep
     * 
     * Sleeping objects are not integrated, saving significant CPU time.
     */
    void put_to_sleep() noexcept {
        if (physics_flags.auto_sleep) {
            physics_flags.is_sleeping = 1;
            velocity = Vec2::zero();
            angular_velocity = 0.0f;
            acceleration = Vec2::zero();
            angular_acceleration = 0.0f;
        }
    }
    
    /** 
     * @brief Wake up sleeping object
     * 
     * Called when object is affected by forces or collisions.
     */
    void wake_up() noexcept {
        physics_flags.is_sleeping = 0;
        sleep_timer = 0.0f;
    }
    
    //-------------------------------------------------------------------------
    // Validation and Utilities
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Validate rigid body state
     * 
     * Checks for NaN values, reasonable ranges, and consistency.
     */
    bool is_valid() const noexcept {
        return mass >= 0.0f && moment_of_inertia >= 0.0f &&
               !std::isnan(velocity.x) && !std::isnan(velocity.y) &&
               !std::isnan(angular_velocity) &&
               linear_damping >= 0.0f && linear_damping <= 1.0f &&
               angular_damping >= 0.0f && angular_damping <= 1.0f;
    }
    
    /** 
     * @brief Get detailed physics information for educational display
     */
    struct PhysicsInfo {
        f32 speed;                  ///< Current speed (magnitude of velocity)
        f32 kinetic_energy;         ///< Current kinetic energy
        f32 linear_momentum_mag;    ///< Magnitude of linear momentum
        f32 angular_momentum_mag;   ///< Magnitude of angular momentum
        const char* integration_method_name; ///< Integration method being used
        bool is_moving;             ///< Whether object is currently moving
        bool is_rotating;           ///< Whether object is currently rotating
    };
    
    PhysicsInfo get_physics_info() const noexcept;
    
    /** 
     * @brief Get object type description
     */
    const char* get_body_type_description() const noexcept {
        if (physics_flags.is_static) return "Static Body";
        if (physics_flags.is_kinematic) return "Kinematic Body";
        return "Dynamic Body";
    }
    
private:
    /** 
     * @brief Update derived values when mass/inertia changes
     */
    constexpr void update_derived_values() noexcept {
        inverse_mass = (mass > constants::EPSILON) ? 1.0f / mass : 0.0f;
        inverse_moment_of_inertia = (moment_of_inertia > constants::EPSILON) ? 1.0f / moment_of_inertia : 0.0f;
    }
};

//=============================================================================
// Force Accumulator Component
//=============================================================================

/**
 * @brief Force and Torque Accumulation Component
 * 
 * Accumulates forces and torques applied to rigid bodies during a physics step.
 * This component demonstrates the principle of force superposition and provides
 * educational insight into how multiple forces combine to produce motion.
 * 
 * Educational Context:
 * - Force superposition: Net force = sum of all individual forces
 * - Torque accumulation: Net torque = sum of all individual torques
 * - Force application points and their effect on torque
 * - Different force types (constant, impulse, continuous)
 * 
 * Design Philosophy:
 * Forces are accumulated during the frame and applied during integration.
 * This separation allows for clear educational demonstration of force
 * combination and provides opportunities for force visualization.
 */
struct alignas(32) ForceAccumulator {
    //-------------------------------------------------------------------------
    // Force Accumulation
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Accumulated linear force (Newtons)
     * 
     * Sum of all forces applied to the object during current physics step.
     * Cleared after each integration step.
     * 
     * Educational Note:
     * This demonstrates Newton's principle of force superposition:
     * multiple forces acting on an object combine vectorially.
     */
    Vec2 accumulated_force{0.0f, 0.0f};
    
    /** 
     * @brief Accumulated torque (Newton-meters)
     * 
     * Sum of all torques applied to the object during current physics step.
     * Positive torque = counter-clockwise rotation.
     * 
     * Educational Context:
     * Torque can come from:
     * 1. Direct torque application (motors, springs)
     * 2. Forces applied away from center of mass
     * 3. Friction forces creating rotational resistance
     */
    f32 accumulated_torque{0.0f};
    
    //-------------------------------------------------------------------------
    // Force Application History (Educational)
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Individual force record for educational analysis
     */
    struct ForceRecord {
        Vec2 force;                 ///< Force vector
        Vec2 application_point;     ///< Where force was applied (local coordinates)
        f32 torque_contribution;    ///< Torque generated by this force
        const char* source_name;    ///< What generated this force (for debugging)
        f32 application_time;       ///< When force was applied
        
        enum class ForceType : u8 {
            Unknown = 0,
            Gravity,            ///< Gravitational force
            Spring,             ///< Spring force
            Damping,           ///< Damping/drag force
            Contact,           ///< Contact/collision force
            User,              ///< User-applied force
            Motor,             ///< Motor/actuator force
            Friction,          ///< Friction force
            Magnetic,          ///< Magnetic force
            Wind               ///< Wind/fluid force
        } type{ForceType::Unknown};
        
        ForceRecord() noexcept = default;
        ForceRecord(Vec2 f, Vec2 app_point, f32 torque, const char* source, ForceType t) noexcept
            : force(f), application_point(app_point), torque_contribution(torque), 
              source_name(source), type(t) {}
    };
    
    /** 
     * @brief History of forces applied this frame
     * 
     * Used for educational analysis and debugging. Can be disabled
     * in release builds for performance.
     */
    static constexpr usize MAX_FORCE_RECORDS = 32;
    std::array<ForceRecord, MAX_FORCE_RECORDS> force_history;
    u8 force_count{0};
    
    //-------------------------------------------------------------------------
    // Impulse Accumulation (for collision response)
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Accumulated linear impulse (Newton-seconds)
     * 
     * Impulses are instantaneous changes in momentum, typically from collisions.
     * Applied directly to velocity rather than through force integration.
     * 
     * Educational Context:
     * Impulse-momentum theorem: Impulse = change in momentum
     * J = Δp = m * Δv, therefore Δv = J / m
     */
    Vec2 accumulated_impulse{0.0f, 0.0f};
    
    /** 
     * @brief Accumulated angular impulse (Newton-meter-seconds)
     * 
     * Rotational equivalent of linear impulse.
     * Changes angular momentum directly: ΔL = I * Δω
     */
    f32 accumulated_angular_impulse{0.0f};
    
    //-------------------------------------------------------------------------
    // Persistent Forces (continuous effects)
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Persistent force generators
     * 
     * Some forces (like gravity, springs, dampers) are continuously applied.
     * This structure manages their state and parameters.
     */
    struct PersistentForce {
        Vec2 force_per_second;      ///< Force applied per second
        f32 torque_per_second;      ///< Torque applied per second
        f32 duration;               ///< How long to apply (-1 = infinite)
        f32 remaining_time;         ///< Time remaining
        bool is_active;             ///< Whether currently active
        ForceRecord::ForceType type; ///< Type of persistent force
        const char* name;           ///< Name for debugging
        
        PersistentForce() noexcept = default;
        PersistentForce(Vec2 force, f32 torque, f32 dur, ForceRecord::ForceType t, const char* n) noexcept
            : force_per_second(force), torque_per_second(torque), duration(dur), 
              remaining_time(dur), is_active(true), type(t), name(n) {}
    };
    
    /** 
     * @brief Active persistent forces
     */
    static constexpr usize MAX_PERSISTENT_FORCES = 8;
    std::array<PersistentForce, MAX_PERSISTENT_FORCES> persistent_forces;
    u8 persistent_force_count{0};
    
    //-------------------------------------------------------------------------
    // Force Application Interface
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Apply force at center of mass (no torque)
     * 
     * Most common force application. Creates pure linear acceleration
     * without any rotational effect.
     */
    void apply_force(const Vec2& force, const char* source = "Unknown") noexcept {
        apply_force_at_point(force, Vec2::zero(), source);
    }
    
    /** 
     * @brief Apply force at specific point (generates torque)
     * 
     * When force is applied away from center of mass, it creates both
     * linear acceleration and torque (rotational acceleration).
     * 
     * Torque calculation: τ = r × F (cross product)
     * In 2D: τ = r.x * F.y - r.y * F.x
     * 
     * @param force Force vector in world coordinates
     * @param application_point Point of application relative to center of mass
     * @param source Human-readable source description
     */
    void apply_force_at_point(const Vec2& force, const Vec2& application_point, 
                             const char* source = "Unknown") noexcept;
    
    /** 
     * @brief Apply pure torque (no linear force)
     * 
     * Directly applies rotational force without linear motion.
     * Useful for motors, springs, and rotational constraints.
     */
    void apply_torque(f32 torque, const char* source = "Unknown") noexcept;
    
    /** 
     * @brief Apply impulse (instantaneous momentum change)
     * 
     * Used for collision response and instantaneous effects.
     * Applied directly to velocity during integration.
     */
    void apply_impulse(const Vec2& impulse, const char* source = "Collision") noexcept;
    
    /** 
     * @brief Apply angular impulse (instantaneous angular momentum change)
     */
    void apply_angular_impulse(f32 impulse, const char* source = "Collision") noexcept;
    
    /** 
     * @brief Apply impulse at point (generates both linear and angular impulse)
     */
    void apply_impulse_at_point(const Vec2& impulse, const Vec2& application_point, 
                               const char* source = "Collision") noexcept;
    
    //-------------------------------------------------------------------------
    // Persistent Force Management
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Add persistent force (applied continuously)
     * 
     * Returns index of the persistent force for later management.
     */
    u8 add_persistent_force(const Vec2& force_per_second, f32 torque_per_second = 0.0f,
                           f32 duration = -1.0f, ForceRecord::ForceType type = ForceRecord::ForceType::Unknown,
                           const char* name = "Persistent") noexcept;
    
    /** 
     * @brief Remove persistent force by index
     */
    void remove_persistent_force(u8 index) noexcept;
    
    /** 
     * @brief Update persistent forces (call once per frame)
     * 
     * Updates duration counters and applies forces.
     */
    void update_persistent_forces(f32 delta_time) noexcept;
    
    /** 
     * @brief Clear all persistent forces
     */
    void clear_persistent_forces() noexcept {
        persistent_force_count = 0;
        for (auto& force : persistent_forces) {
            force.is_active = false;
        }
    }
    
    //-------------------------------------------------------------------------
    // Integration and Cleanup
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Get net force and torque for integration
     * 
     * Returns the accumulated forces and torques, ready for physics integration.
     */
    void get_net_forces(Vec2& out_force, f32& out_torque) const noexcept {
        out_force = accumulated_force;
        out_torque = accumulated_torque;
    }
    
    /** 
     * @brief Get accumulated impulses
     * 
     * Returns impulses to be applied directly to velocity.
     */
    void get_impulses(Vec2& out_impulse, f32& out_angular_impulse) const noexcept {
        out_impulse = accumulated_impulse;
        out_angular_impulse = accumulated_angular_impulse;
    }
    
    /** 
     * @brief Clear accumulated forces (call after integration)
     * 
     * Resets force accumulation for next physics step.
     * Persistent forces are not cleared.
     */
    void clear_accumulated_forces() noexcept {
        accumulated_force = Vec2::zero();
        accumulated_torque = 0.0f;
        accumulated_impulse = Vec2::zero();
        accumulated_angular_impulse = 0.0f;
        force_count = 0;
    }
    
    //-------------------------------------------------------------------------
    // Educational Analysis
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Get force analysis for educational purposes
     */
    struct ForceAnalysis {
        Vec2 net_force;             ///< Total accumulated force
        f32 net_torque;             ///< Total accumulated torque
        f32 force_magnitude;        ///< Magnitude of net force
        u32 force_contributors;     ///< Number of forces contributing
        Vec2 center_of_pressure;    ///< Effective point of force application
        f32 largest_force_mag;      ///< Magnitude of largest individual force
        ForceRecord::ForceType dominant_force_type; ///< Type of strongest force
    };
    
    ForceAnalysis get_force_analysis() const noexcept;
    
    /** 
     * @brief Get force breakdown by type
     */
    std::array<Vec2, static_cast<usize>(ForceRecord::ForceType::Wind) + 1> get_force_breakdown_by_type() const noexcept;
    
    /** 
     * @brief Calculate work done by forces this frame
     * 
     * Work = Force · displacement
     * Educational opportunity to demonstrate energy concepts.
     */
    f32 calculate_work_done(const Vec2& displacement, f32 angular_displacement) const noexcept;
    
    /** 
     * @brief Estimate power output (work per time)
     * 
     * Power = Force · velocity
     */
    f32 calculate_power_output(const Vec2& velocity, f32 angular_velocity) const noexcept;
    
    //-------------------------------------------------------------------------
    // Validation and Debugging
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Check if any forces are accumulated
     */
    bool has_forces() const noexcept {
        return (accumulated_force.x != 0.0f || accumulated_force.y != 0.0f) ||
               std::abs(accumulated_torque) > constants::EPSILON ||
               (accumulated_impulse.x != 0.0f || accumulated_impulse.y != 0.0f) ||
               std::abs(accumulated_angular_impulse) > constants::EPSILON ||
               persistent_force_count > 0;
    }
    
    /** 
     * @brief Validate force accumulator state
     */
    bool is_valid() const noexcept {
        return !std::isnan(accumulated_force.x) && !std::isnan(accumulated_force.y) &&
               !std::isnan(accumulated_torque) && !std::isnan(accumulated_impulse.x) &&
               !std::isnan(accumulated_impulse.y) && !std::isnan(accumulated_angular_impulse) &&
               force_count <= MAX_FORCE_RECORDS &&
               persistent_force_count <= MAX_PERSISTENT_FORCES;
    }
    
    /** 
     * @brief Get force history for debugging
     */
    std::span<const ForceRecord> get_force_history() const noexcept {
        return std::span<const ForceRecord>(force_history.data(), force_count);
    }
    
    /** 
     * @brief Get active persistent forces
     */
    std::span<const PersistentForce> get_persistent_forces() const noexcept {
        return std::span<const PersistentForce>(persistent_forces.data(), persistent_force_count);
    }
    
private:
    /** 
     * @brief Add force to history for educational analysis
     */
    void record_force(const Vec2& force, const Vec2& application_point, 
                     f32 torque_contribution, const char* source,
                     ForceRecord::ForceType type) noexcept;
};

//=============================================================================
// Advanced Physics Components
//=============================================================================

/**
 * @brief Base Constraint Component for Physics Joints and Springs
 * 
 * Base class for all physics constraints (joints, springs, motors, etc.).
 * Demonstrates constraint-based physics and provides educational insight
 * into how complex mechanical systems are constructed.
 * 
 * Educational Context:
 * Constraints are mathematical relationships that limit the motion of objects.
 * They're fundamental to robotics, mechanical engineering, and game physics.
 */
struct alignas(16) Constraint2D {
    /** 
     * @brief Constraint types for educational categorization
     */
    enum class Type : u8 {
        Unknown = 0,
        Distance,       ///< Fixed distance between two points
        Revolute,       ///< Rotational joint (hinge)
        Prismatic,      ///< Sliding joint
        Weld,          ///< Fixed joint (no relative motion)
        Spring,        ///< Spring connection
        Motor,         ///< Motorized joint
        Rope,          ///< Rope/cable constraint
        Pulley,        ///< Pulley system
        Gear          ///< Gear connection
    } constraint_type{Type::Unknown};
    
    /** 
     * @brief Entity IDs of connected objects
     * 
     * Most constraints connect two rigid bodies. Some constraints
     * (like anchors) connect one body to a world position.
     */
    u32 entity_a{0};    // First connected entity
    u32 entity_b{0};    // Second connected entity (0 = world anchor)
    
    /** 
     * @brief Local attachment points
     * 
     * Points where constraint attaches to each object,
     * in local coordinates relative to object center.
     */
    Vec2 local_anchor_a{0.0f, 0.0f};
    Vec2 local_anchor_b{0.0f, 0.0f};
    
    /** 
     * @brief Constraint parameters
     * 
     * Different constraint types use these parameters differently:
     * - Distance: target_distance, damping_ratio
     * - Spring: spring_constant, rest_length
     * - Motor: target_velocity, max_force
     */
    f32 target_value{0.0f};      // Target distance/angle/velocity
    f32 spring_constant{100.0f}; // Spring stiffness (N/m)
    f32 damping_ratio{0.1f};     // Damping coefficient (0-1)
    f32 max_force{1000.0f};      // Maximum constraint force
    
    /** 
     * @brief Constraint state flags
     */
    union {
        u32 flags;
        struct {
            u32 is_active : 1;          ///< Whether constraint is active
            u32 break_on_force : 1;     ///< Break if force exceeds limit
            u32 break_on_impulse : 1;   ///< Break if impulse exceeds limit
            u32 motor_enabled : 1;      ///< Motor functionality active
            u32 limits_enabled : 1;     ///< Joint limits active
            u32 collision_disabled : 1; ///< Disable collision between connected bodies
            u32 visualize_debug : 1;    ///< Show debug visualization
            u32 reserved : 25;
        };
    } constraint_flags{0};
    
    /** 
     * @brief Current constraint state (updated by solver)
     */
    mutable struct {
        Vec2 constraint_force{0.0f, 0.0f}; ///< Force being applied by constraint
        f32 constraint_impulse{0.0f};      ///< Impulse applied this step
        f32 current_error{0.0f};           ///< Current constraint violation
        f32 accumulated_impulse{0.0f};     ///< Total impulse over time
        bool is_broken{false};             ///< Whether constraint has broken
    } solver_state;
    
    /** 
     * @brief Performance metrics
     */
    mutable struct {
        u32 solver_iterations{0};          ///< Iterations needed to solve
        f32 solve_time{0.0f};              ///< Time spent solving constraint
        f32 constraint_energy{0.0f};       ///< Energy stored in constraint
    } performance_info;
    
    //-------------------------------------------------------------------------
    // Factory Methods for Different Constraint Types
    //-------------------------------------------------------------------------
    
    /** @brief Create distance constraint (maintains fixed distance) */
    static Constraint2D create_distance(u32 entity_a, u32 entity_b, 
                                       Vec2 anchor_a, Vec2 anchor_b, 
                                       f32 distance) noexcept;
    
    /** @brief Create spring constraint (Hooke's law spring) */
    static Constraint2D create_spring(u32 entity_a, u32 entity_b,
                                     Vec2 anchor_a, Vec2 anchor_b,
                                     f32 rest_length, f32 spring_k, f32 damping) noexcept;
    
    /** @brief Create revolute joint (rotation only) */
    static Constraint2D create_revolute(u32 entity_a, u32 entity_b,
                                       Vec2 anchor_a, Vec2 anchor_b) noexcept;
    
    /** @brief Create motor joint (applies rotational force) */
    static Constraint2D create_motor(u32 entity_a, u32 entity_b,
                                   Vec2 anchor_a, Vec2 anchor_b,
                                   f32 target_speed, f32 max_torque) noexcept;
    
    //-------------------------------------------------------------------------
    // Constraint Interface
    //-------------------------------------------------------------------------
    
    /** @brief Get constraint type name for debugging */
    const char* get_type_name() const noexcept;
    
    /** @brief Check if constraint should break under current forces */
    bool should_break() const noexcept;
    
    /** @brief Break the constraint */
    void break_constraint() noexcept {
        constraint_flags.is_active = 0;
        solver_state.is_broken = true;
    }
    
    /** @brief Check if constraint is currently active */
    bool is_active() const noexcept {
        return constraint_flags.is_active && !solver_state.is_broken;
    }
    
    /** @brief Get current constraint violation (for educational display) */
    f32 get_constraint_error() const noexcept {
        return solver_state.current_error;
    }
};

/**
 * @brief Trigger Component for Non-Physical Collision Detection
 * 
 * Triggers detect collisions but don't respond physically. Used for
 * gameplay events, sensors, and area detection. Demonstrates the
 * distinction between collision detection and collision response.
 */
struct alignas(16) Trigger2D {
    /** 
     * @brief Trigger shape for detection
     * 
     * Same shape types as Collider2D but optimized for detection only.
     */
    CollisionShape trigger_shape;
    
    /** 
     * @brief Local offset from entity transform
     */
    Vec2 offset{0.0f, 0.0f};
    
    /** 
     * @brief What layers this trigger detects
     */
    u32 detection_layers{0xFFFFFFFF};
    
    /** 
     * @brief Trigger behavior flags
     */
    union {
        u32 flags;
        struct {
            u32 detect_entry : 1;      ///< Trigger on object entering
            u32 detect_exit : 1;       ///< Trigger on object exiting  
            u32 detect_stay : 1;       ///< Trigger while object inside
            u32 one_shot : 1;          ///< Trigger only once then disable
            u32 visualize_bounds : 1;  ///< Show trigger bounds in debug
            u32 reserved : 27;
        };
    } trigger_flags{0x07}; // Entry, exit, and stay by default
    
    /** 
     * @brief Currently detected objects
     */
    static constexpr usize MAX_DETECTED = 32;
    std::array<u32, MAX_DETECTED> detected_entities;
    u8 detected_count{0};
    
    /** 
     * @brief Trigger statistics for educational analysis
     */
    mutable struct {
        u32 total_entries{0};       ///< Total objects that entered
        u32 total_exits{0};         ///< Total objects that exited
        u32 current_occupants{0};   ///< Current objects inside
        f32 last_trigger_time{0.0f}; ///< When last triggered
        f32 average_occupancy{0.0f}; ///< Average number of objects inside
    } statistics;
    
    //-------------------------------------------------------------------------
    // Trigger Interface
    //-------------------------------------------------------------------------
    
    /** @brief Check if entity is currently detected */
    bool is_detecting(u32 entity_id) const noexcept;
    
    /** @brief Add entity to detected list */
    void add_detected(u32 entity_id) noexcept;
    
    /** @brief Remove entity from detected list */
    void remove_detected(u32 entity_id) noexcept;
    
    /** @brief Clear all detected entities */
    void clear_detected() noexcept {
        detected_count = 0;
        detected_entities.fill(0);
    }
    
    /** @brief Get all currently detected entities */
    std::span<const u32> get_detected_entities() const noexcept {
        return std::span<const u32>(detected_entities.data(), detected_count);
    }
};

/**
 * @brief Physics Debug and Performance Information Component
 * 
 * Provides comprehensive debug information and performance metrics
 * for educational analysis. Can be enabled/disabled for performance.
 */
struct alignas(32) PhysicsInfo {
    /** 
     * @brief Physics simulation metrics
     */
    struct SimulationMetrics {
        f32 physics_time_step{0.016f};      ///< Current physics timestep
        u32 integration_steps_per_frame{1}; ///< Integration steps per frame
        f32 total_simulation_time{0.0f};    ///< Total time simulating
        u32 total_integration_steps{0};     ///< Total integration steps performed
        
        // Per-frame metrics
        f32 last_frame_physics_time{0.0f};  ///< Physics time last frame
        u32 active_bodies{0};               ///< Currently active rigid bodies
        u32 sleeping_bodies{0};             ///< Currently sleeping bodies
        u32 collision_checks{0};            ///< Collision checks per frame
        u32 contacts_generated{0};          ///< Contacts generated per frame
        u32 constraints_solved{0};          ///< Constraints solved per frame
    } simulation;
    
    /** 
     * @brief Memory usage information
     */
    struct MemoryMetrics {
        usize rigid_body_memory{0};         ///< Memory used by rigid bodies
        usize collider_memory{0};           ///< Memory used by colliders
        usize constraint_memory{0};         ///< Memory used by constraints
        usize total_physics_memory{0};      ///< Total physics memory usage
        u32 memory_allocations{0};          ///< Number of physics allocations
        u32 memory_deallocations{0};        ///< Number of physics deallocations
    } memory;
    
    /** 
     * @brief Performance analysis
     */
    struct PerformanceMetrics {
        f32 average_frame_time{0.016f};     ///< Average physics frame time
        f32 worst_frame_time{0.0f};         ///< Worst physics frame time
        f32 best_frame_time{1.0f};          ///< Best physics frame time
        f32 cpu_usage_percent{0.0f};        ///< Estimated CPU usage
        
        // Breakdown by subsystem
        f32 integration_time{0.0f};         ///< Time spent integrating
        f32 collision_time{0.0f};           ///< Time spent on collision detection
        f32 constraint_time{0.0f};          ///< Time spent solving constraints
        f32 broadphase_time{0.0f};          ///< Time spent in broad phase
        f32 narrowphase_time{0.0f};         ///< Time spent in narrow phase
    } performance;
    
    /** 
     * @brief Educational statistics
     */
    struct EducationalMetrics {
        f32 total_kinetic_energy{0.0f};     ///< Total KE in system
        f32 total_potential_energy{0.0f};   ///< Total PE in system  
        f32 energy_conservation_error{0.0f}; ///< Energy conservation violation
        f32 momentum_conservation_error{0.0f}; ///< Momentum conservation violation
        
        // Force analysis
        Vec2 total_applied_force{0.0f, 0.0f}; ///< Total force applied this frame
        f32 total_applied_torque{0.0f};      ///< Total torque applied this frame
        u32 force_applications{0};           ///< Number of force applications
        u32 impulse_applications{0};         ///< Number of impulse applications
    } educational;
    
    //-------------------------------------------------------------------------
    // Update and Analysis Interface
    //-------------------------------------------------------------------------
    
    /** @brief Update metrics for current frame */
    void update_frame_metrics(f32 delta_time) noexcept;
    
    /** @brief Record physics operation timing */
    void record_operation_time(const char* operation, f32 time) noexcept;
    
    /** @brief Add to energy totals */
    void add_energy(f32 kinetic, f32 potential) noexcept {
        educational.total_kinetic_energy += kinetic;
        educational.total_potential_energy += potential;
    }
    
    /** @brief Record force application */
    void record_force_application(const Vec2& force, f32 torque) noexcept {
        educational.total_applied_force += force;
        educational.total_applied_torque += torque;
        educational.force_applications++;
    }
    
    /** @brief Get comprehensive performance report */
    struct PerformanceReport {
        f32 fps_equivalent;                 ///< What FPS this physics cost represents
        f32 cpu_percentage;                 ///< CPU usage percentage
        const char* performance_rating;     ///< "Excellent", "Good", "Fair", "Poor"
        const char* bottleneck;            ///< Main performance bottleneck
        const char* optimization_advice;    ///< Advice for optimization
    };
    
    PerformanceReport get_performance_report() const noexcept;
    
    /** @brief Reset all metrics */
    void reset() noexcept {
        simulation = {};
        memory = {};
        performance = {};
        educational = {};
    }
};

/**
 * @brief Motion State Caching Component for Performance Optimization
 * 
 * Caches frequently accessed motion data to improve performance in
 * systems that need to query object states repeatedly. Demonstrates
 * cache optimization techniques in physics engines.
 */
struct alignas(32) MotionState {
    /** 
     * @brief Cached transform state
     */
    struct TransformCache {
        Vec2 world_position{0.0f, 0.0f};    ///< Cached world position
        f32 world_rotation{0.0f};           ///< Cached world rotation
        Vec2 world_scale{1.0f, 1.0f};       ///< Cached world scale
        Matrix2 rotation_matrix;            ///< Cached rotation matrix
        bool is_dirty{true};                ///< Whether cache needs update
    } transform_cache;
    
    /** 
     * @brief Cached motion state
     */
    struct MotionCache {
        Vec2 velocity{0.0f, 0.0f};          ///< Cached velocity
        f32 angular_velocity{0.0f};         ///< Cached angular velocity
        f32 speed{0.0f};                    ///< Cached speed (velocity magnitude)
        Vec2 velocity_direction{0.0f, 0.0f}; ///< Cached normalized velocity
        bool is_moving{false};              ///< Whether object is moving
        bool is_rotating{false};            ///< Whether object is rotating
        f32 last_update_time{0.0f};         ///< When cache was last updated
    } motion_cache;
    
    /** 
     * @brief Cached collision state
     */
    struct CollisionCache {
        AABB world_aabb;                    ///< Cached world-space AABB
        Vec2 aabb_center{0.0f, 0.0f};       ///< Cached AABB center
        Vec2 aabb_extents{0.0f, 0.0f};      ///< Cached AABB half-extents
        bool has_moved{false};              ///< Whether object moved since last frame
        f32 movement_threshold{0.01f};      ///< Movement threshold for cache invalidation
    } collision_cache;
    
    /** 
     * @brief Performance tracking
     */
    struct CacheMetrics {
        u32 cache_hits{0};                  ///< Number of cache hits
        u32 cache_misses{0};                ///< Number of cache misses
        f32 hit_ratio{0.0f};                ///< Cache hit ratio
        f32 time_saved{0.0f};               ///< Estimated time saved by caching
    } metrics;
    
    //-------------------------------------------------------------------------
    // Cache Management Interface
    //-------------------------------------------------------------------------
    
    /** @brief Mark all caches as dirty */
    void invalidate_all() noexcept {
        transform_cache.is_dirty = true;
        collision_cache.has_moved = true;
    }
    
    /** @brief Update cached transform data */
    void update_transform_cache(const Transform& transform) noexcept;
    
    /** @brief Update cached motion data */
    void update_motion_cache(const RigidBody2D& rigidbody) noexcept;
    
    /** @brief Update cached collision data */
    void update_collision_cache(const Transform& transform, const Collider2D& collider) noexcept;
    
    /** @brief Get cached world AABB (updates if dirty) */
    const AABB& get_world_aabb(const Transform& transform, const Collider2D& collider) noexcept;
    
    /** @brief Get cached rotation matrix (updates if dirty) */
    const Matrix2& get_rotation_matrix(const Transform& transform) noexcept;
    
    /** @brief Check if object has moved significantly */
    bool has_moved_significantly(const Vec2& new_position, f32 new_rotation) const noexcept;
    
    /** @brief Get cache performance metrics */
    f32 get_cache_efficiency() const noexcept {
        u32 total = metrics.cache_hits + metrics.cache_misses;
        return total > 0 ? static_cast<f32>(metrics.cache_hits) / total : 0.0f;
    }
};

//=============================================================================
// Component Validation and Static Assertions
//=============================================================================

// Ensure all components satisfy the ECS Component concept
static_assert(ecs::Component<PhysicsMaterial>, "PhysicsMaterial must satisfy Component concept");
// TODO: Make Collider2D trivially copyable for ECS compatibility
// static_assert(ecs::Component<Collider2D>, "Collider2D must satisfy Component concept");
static_assert(ecs::Component<RigidBody2D>, "RigidBody2D must satisfy Component concept");
static_assert(ecs::Component<ForceAccumulator>, "ForceAccumulator must satisfy Component concept");
static_assert(ecs::Component<Constraint2D>, "Constraint2D must satisfy Component concept");
static_assert(ecs::Component<Trigger2D>, "Trigger2D must satisfy Component concept");
static_assert(ecs::Component<PhysicsInfo>, "PhysicsInfo must satisfy Component concept");
static_assert(ecs::Component<MotionState>, "MotionState must satisfy Component concept");

// Verify memory alignment for optimal performance
static_assert(alignof(PhysicsMaterial) >= 16, "PhysicsMaterial should be 16-byte aligned");
static_assert(alignof(Collider2D) >= 32, "Collider2D should be 32-byte aligned");
static_assert(alignof(RigidBody2D) >= 32, "RigidBody2D should be 32-byte aligned");
static_assert(alignof(ForceAccumulator) >= 32, "ForceAccumulator should be 32-byte aligned");

// Verify reasonable component sizes for cache efficiency
static_assert(sizeof(PhysicsMaterial) <= 64, "PhysicsMaterial should fit in 64 bytes");
static_assert(sizeof(RigidBody2D) <= 256, "RigidBody2D should fit in 256 bytes");

//=============================================================================
// Utility Functions and Component Relationships
//=============================================================================

namespace utils {
    /**
     * @brief Calculate mass from collider shape and material
     */
    f32 calculate_mass_from_shape_and_material(const CollisionShape& shape, const PhysicsMaterial& material) noexcept;
    
    /**
     * @brief Calculate moment of inertia from shape and mass
     */
    f32 calculate_moment_of_inertia_from_shape(const CollisionShape& shape, f32 mass) noexcept;
    
    /**
     * @brief Create a complete physics entity with reasonable defaults
     */
    struct PhysicsEntityDesc {
        CollisionShape shape;
        PhysicsMaterial material = PhysicsMaterial{};
        f32 mass = 1.0f;
        bool is_static = false;
        bool is_kinematic = false;
        bool is_trigger = false;
    };
    
    /**
     * @brief Helper to create physics components for an entity
     */
    struct PhysicsComponents {
        RigidBody2D rigidbody;
        Collider2D collider;
        ForceAccumulator forces;
        std::optional<PhysicsInfo> debug_info;
        std::optional<MotionState> motion_cache;
    };
    
    PhysicsComponents create_physics_entity(const PhysicsEntityDesc& desc) noexcept;
    
    /**
     * @brief Validate physics component consistency
     */
    bool validate_physics_components(const RigidBody2D* rigidbody, 
                                   const Collider2D* collider,
                                   const ForceAccumulator* forces) noexcept;
}

} // namespace ecscope::physics::components