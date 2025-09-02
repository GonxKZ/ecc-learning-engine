/**
 * @file physics/components.cpp
 * @brief Implementation of Comprehensive Physics Components for ECScope Educational ECS Engine
 * 
 * This implementation provides complete functionality for all physics components defined in
 * components.hpp. Each function is implemented with educational clarity while maintaining
 * high performance through modern C++ techniques.
 * 
 * Implementation Philosophy:
 * - Clear, readable code with extensive educational comments
 * - Optimal performance through careful algorithm selection and memory access patterns
 * - Robust error handling and validation
 * - Integration with memory tracking and debugging systems
 * - Educational insights embedded throughout the implementation
 * 
 * Performance Optimizations:
 * - SIMD utilization where beneficial
 * - Cache-friendly memory access patterns
 * - Branch prediction optimization
 * - Minimal dynamic allocation
 * - Compile-time constant evaluation
 * 
 * Educational Features:
 * - Step-by-step physics calculations with explanations
 * - Performance metrics and analysis
 * - Debugging information and visualization data
 * - Comprehensive validation and error reporting
 * 
 * @author ECScope Educational ECS Framework - Phase 5: Física 2D
 * @date 2024
 */

#include "components.hpp"
#include "math.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <chrono>
#include <cstring>

namespace ecscope::physics::components {

//=============================================================================
// Physics Material Implementation
//=============================================================================

PhysicsMaterial PhysicsMaterial::combine(const PhysicsMaterial& a, const PhysicsMaterial& b) noexcept {
    PhysicsMaterial result;
    
    // Educational Note: Material combination uses physics-based rules
    // rather than simple averaging for realistic behavior
    
    // Restitution: Use minimum (weakest link principle)
    // In real collisions, the less bouncy material dominates
    result.restitution = std::min(a.restitution, b.restitution);
    
    // Friction: Use geometric mean for realistic contact mechanics
    // This approximates the complex tribological interactions
    result.static_friction = std::sqrt(a.static_friction * b.static_friction);
    result.kinetic_friction = std::sqrt(a.kinetic_friction * b.kinetic_friction);
    
    // Density: Average of both materials (for contact calculations)
    result.density = (a.density + b.density) * 0.5f;
    
    // Surface properties: Combine using weighted averages
    result.surface_roughness = (a.surface_roughness + b.surface_roughness) * 0.5f;
    result.hardness = (a.hardness + b.hardness) * 0.5f;
    result.thermal_conductivity = (a.thermal_conductivity + b.thermal_conductivity) * 0.5f;
    
    // Material flags: Combine using logical OR for active properties
    result.material_flags.flags = a.material_flags.flags | b.material_flags.flags;
    
    return result;
}

const char* PhysicsMaterial::get_material_description() const noexcept {
    // Educational feature: Provide material identification based on properties
    if (restitution > 0.7f && density < 1000.0f) {
        return "Rubber-like (High bounce, low density)";
    } else if (density > 7000.0f && hardness > 0.7f) {
        return "Metallic (High density and hardness)";
    } else if (static_friction < 0.1f) {
        return "Slippery (Very low friction)";
    } else if (surface_roughness > 0.5f) {
        return "Rough surface (High friction)";
    } else if (material_flags.is_liquid) {
        return "Fluid (Liquid behavior)";
    } else if (material_flags.is_fragile) {
        return "Brittle (Breaks under stress)";
    } else {
        return "Generic material";
    }
}

//=============================================================================
// Collider2D Implementation
//=============================================================================

Collider2D::~Collider2D() noexcept {
    // Now trivially destructible - no cleanup needed for fixed arrays
}

Collider2D& Collider2D::operator=(Collider2D&& other) noexcept {
    if (this != &other) {
        shape = std::move(other.shape);
        offset = other.offset;
        material = other.material;
        collision_layers = other.collision_layers;
        collision_mask = other.collision_mask;
        collision_flags = other.collision_flags;
        performance_info = other.performance_info;
        debug_info = other.debug_info;
        // Copy the additional shapes arrays
        additional_shapes = other.additional_shapes;
        additional_offsets = other.additional_offsets;
    }
    return *this;
}

const char* Collider2D::get_shape_name() const noexcept {
    // Educational feature: Human-readable shape type names
    switch (shape.index()) {
        case 0: return "Circle";
        case 1: return "AABB";
        case 2: return "OBB";
        case 3: return "Polygon";
        default: return "Unknown";
    }
}

CollisionShape Collider2D::get_world_shape(const Transform& entity_transform) const noexcept {
    // Educational Note: This function demonstrates how local shapes are transformed
    // to world space using the entity's transform plus the collider's local offset
    
    // Create Transform2D for more efficient transformations
    Transform2D world_transform(entity_transform.position + offset, 
                               entity_transform.rotation, 
                               entity_transform.scale);
    
    // Transform shape based on its type
    return std::visit([&](const auto& local_shape) -> CollisionShape {
        using ShapeType = std::decay_t<decltype(local_shape)>;
        
        if constexpr (std::is_same_v<ShapeType, Circle>) {
            return local_shape.transformed(world_transform);
        } else if constexpr (std::is_same_v<ShapeType, AABB>) {
            // AABB needs special handling for rotation
            if (std::abs(entity_transform.rotation) < constants::EPSILON) {
                // No rotation - simple translation and scaling
                Vec2 size = local_shape.size();
                Vec2 world_size = Vec2{size.x * entity_transform.scale.x, 
                                     size.y * entity_transform.scale.y};
                Vec2 world_center = entity_transform.position + offset;
                return AABB::from_center_size(world_center, world_size);
            } else {
                // Rotation present - convert to OBB for accurate representation
                OBB world_obb = OBB::from_aabb(local_shape, entity_transform.rotation);
                world_obb.center = entity_transform.position + offset;
                world_obb.half_extents = Vec2{world_obb.half_extents.x * entity_transform.scale.x,
                                            world_obb.half_extents.y * entity_transform.scale.y};
                return world_obb;
            }
        } else if constexpr (std::is_same_v<ShapeType, OBB>) {
            OBB world_obb = local_shape;
            world_obb.center = world_transform.transform_point(local_shape.center);
            world_obb.rotation += entity_transform.rotation;
            world_obb.half_extents = Vec2{local_shape.half_extents.x * entity_transform.scale.x,
                                        local_shape.half_extents.y * entity_transform.scale.y};
            return world_obb;
        } else if constexpr (std::is_same_v<ShapeType, Polygon>) {
            return local_shape.transformed(world_transform);
        }
        
        // Fallback (should not reach here)
        return local_shape;
    }, shape);
}

AABB Collider2D::get_world_aabb(const Transform& entity_transform) const noexcept {
    // Educational Note: AABB calculation is crucial for broad-phase collision detection
    // We need to handle each shape type appropriately to get tight bounds
    
    CollisionShape world_shape = get_world_shape(entity_transform);
    
    return std::visit([](const auto& shape) -> AABB {
        using ShapeType = std::decay_t<decltype(shape)>;
        
        if constexpr (std::is_same_v<ShapeType, Circle>) {
            return shape.get_aabb();
        } else if constexpr (std::is_same_v<ShapeType, AABB>) {
            return shape;
        } else if constexpr (std::is_same_v<ShapeType, OBB>) {
            return shape.get_aabb();
        } else if constexpr (std::is_same_v<ShapeType, Polygon>) {
            return shape.get_aabb();
        }
        
        // Fallback
        return AABB{};
    }, world_shape);
}

f32 Collider2D::estimate_collision_cost() const noexcept {
    // Educational Note: This provides insight into collision detection performance
    // Different shape types have vastly different computational costs
    
    f32 base_cost = std::visit([](const auto& shape) -> f32 {
        using ShapeType = std::decay_t<decltype(shape)>;
        
        if constexpr (std::is_same_v<ShapeType, Circle>) {
            return 1.0f;  // Baseline: circle-circle collision is cheapest
        } else if constexpr (std::is_same_v<ShapeType, AABB>) {
            return 1.2f;  // Slightly more expensive due to more checks
        } else if constexpr (std::is_same_v<ShapeType, OBB>) {
            return 2.5f;  // Requires axis projection calculations
        } else if constexpr (std::is_same_v<ShapeType, Polygon>) {
            return 5.0f + shape.vertex_count * 0.5f;  // SAT algorithm complexity
        }
        return 1.0f;
    }, shape);
    
    // Additional shapes multiply the cost
    if (has_multiple_shapes()) {
        base_cost *= (1.0f + get_shape_count() * 0.3f);
    }
    
    // High-precision mode increases cost
    if (collision_flags.high_precision) {
        base_cost *= 1.5f;
    }
    
    // Continuous collision detection is much more expensive
    if (collision_flags.continuous_collision) {
        base_cost *= 3.0f;
    }
    
    return base_cost;
}

Collider2D::ShapeInfo Collider2D::get_shape_info() const noexcept {
    ShapeInfo info;
    
    info.type_name = get_shape_name();
    
    // Calculate shape properties for educational display
    std::visit([&info](const auto& shape) {
        using ShapeType = std::decay_t<decltype(shape)>;
        
        if constexpr (std::is_same_v<ShapeType, Circle>) {
            info.area = shape.area();
            info.perimeter = shape.circumference();
            info.centroid = shape.center;
            info.moment_of_inertia = 0.5f * info.area * shape.radius * shape.radius;  // For unit mass
            info.complexity_score = 1;
        } else if constexpr (std::is_same_v<ShapeType, AABB>) {
            info.area = shape.area();
            info.perimeter = shape.perimeter();
            info.centroid = shape.center();
            // Moment of inertia for rectangle: (1/12) * mass * (width² + height²)
            f32 w = shape.width();
            f32 h = shape.height();
            info.moment_of_inertia = (w * w + h * h) / 12.0f;  // For unit mass
            info.complexity_score = 2;
        } else if constexpr (std::is_same_v<ShapeType, OBB>) {
            info.area = shape.area();
            info.perimeter = 2.0f * (shape.half_extents.x + shape.half_extents.y) * 2.0f;
            info.centroid = shape.center;
            f32 w = shape.half_extents.x * 2.0f;
            f32 h = shape.half_extents.y * 2.0f;
            info.moment_of_inertia = (w * w + h * h) / 12.0f;  // For unit mass
            info.complexity_score = 4;
        } else if constexpr (std::is_same_v<ShapeType, Polygon>) {
            info.area = shape.get_area();
            // Approximate perimeter calculation
            f32 perimeter = 0.0f;
            for (u32 i = 0; i < shape.vertex_count; ++i) {
                Vec2 edge = shape.get_edge(i);
                perimeter += edge.length();
            }
            info.perimeter = perimeter;
            info.centroid = shape.get_centroid();
            info.moment_of_inertia = math::utils::moment_of_inertia_polygon(1.0f, shape);
            info.complexity_score = 5 + shape.vertex_count / 2;
        }
    }, shape);
    
    return info;
}

bool Collider2D::is_valid() const noexcept {
    // Educational Note: Validation is crucial for robust physics simulation
    // Invalid components can cause crashes or undefined behavior
    
    if (!material.is_valid()) {
        return false;
    }
    
    // Validate the primary shape
    bool shape_valid = std::visit([](const auto& shape) -> bool {
        using ShapeType = std::decay_t<decltype(shape)>;
        
        if constexpr (std::is_same_v<ShapeType, Circle>) {
            return shape.radius > 0.0f && !std::isnan(shape.center.x) && !std::isnan(shape.center.y);
        } else if constexpr (std::is_same_v<ShapeType, AABB>) {
            return shape.is_valid();
        } else if constexpr (std::is_same_v<ShapeType, OBB>) {
            return shape.half_extents.x > 0.0f && shape.half_extents.y > 0.0f &&
                   !std::isnan(shape.center.x) && !std::isnan(shape.center.y) &&
                   !std::isnan(shape.rotation);
        } else if constexpr (std::is_same_v<ShapeType, Polygon>) {
            return shape.vertex_count >= 3 && shape.vertex_count <= Polygon::MAX_VERTICES;
        }
        return false;
    }, shape);
    
    if (!shape_valid) {
        return false;
    }
    
    // Validate offset
    if (std::isnan(offset.x) || std::isnan(offset.y)) {
        return false;
    }
    
    // Note: Additional shapes validation would go here if needed
    
    return true;
}

//=============================================================================
// RigidBody2D Implementation
//=============================================================================

void RigidBody2D::calculate_moment_of_inertia_from_shape(const CollisionShape& shape) noexcept {
    // Educational Note: Moment of inertia calculation is fundamental to rotational dynamics
    // Different shapes have different formulas based on their mass distribution
    
    f32 calculated_inertia = std::visit([this](const auto& collision_shape) -> f32 {
        using ShapeType = std::decay_t<decltype(collision_shape)>;
        
        if constexpr (std::is_same_v<ShapeType, Circle>) {
            // Circle: I = (1/2) * m * r²
            return math::utils::moment_of_inertia_circle(mass, collision_shape.radius);
        } else if constexpr (std::is_same_v<ShapeType, AABB>) {
            // Rectangle: I = (1/12) * m * (w² + h²)
            return math::utils::moment_of_inertia_box(mass, collision_shape.width(), collision_shape.height());
        } else if constexpr (std::is_same_v<ShapeType, OBB>) {
            // OBB is essentially a rotated rectangle
            f32 width = collision_shape.half_extents.x * 2.0f;
            f32 height = collision_shape.half_extents.y * 2.0f;
            return math::utils::moment_of_inertia_box(mass, width, height);
        } else if constexpr (std::is_same_v<ShapeType, Polygon>) {
            // Complex polygon calculation
            return math::utils::moment_of_inertia_polygon(mass, collision_shape);
        }
        
        // Fallback for unknown shape types
        return mass;
    }, shape);
    
    set_moment_of_inertia(calculated_inertia);
}

RigidBody2D::PhysicsInfo RigidBody2D::get_physics_info() const noexcept {
    PhysicsInfo info;
    
    info.speed = velocity.length();
    info.kinetic_energy = calculate_kinetic_energy();
    info.linear_momentum_mag = get_linear_momentum().length();
    info.angular_momentum_mag = std::abs(get_angular_momentum());
    
    // Integration method name
    switch (integration_method) {
        case IntegrationMethod::Euler:
            info.integration_method_name = "Euler (Simple)";
            break;
        case IntegrationMethod::RungeKutta4:
            info.integration_method_name = "Runge-Kutta 4th Order";
            break;
        case IntegrationMethod::Verlet:
            info.integration_method_name = "Verlet (Stable)";
            break;
        case IntegrationMethod::LeapFrog:
            info.integration_method_name = "Leap-Frog";
            break;
        default:
            info.integration_method_name = "Unknown";
            break;
    }
    
    // Movement state
    info.is_moving = info.speed > sleep_threshold;
    info.is_rotating = std::abs(angular_velocity) > sleep_threshold;
    
    return info;
}

//=============================================================================
// ForceAccumulator Implementation  
//=============================================================================

void ForceAccumulator::apply_force_at_point(const Vec2& force, const Vec2& application_point, 
                                           const char* source) noexcept {
    // Educational Note: Applying force at a point away from center of mass creates both
    // linear acceleration and torque (rotational acceleration)
    
    // Accumulate linear force
    accumulated_force += force;
    
    // Calculate torque contribution: τ = r × F (cross product in 2D)
    f32 torque_contribution = vec2::cross(application_point, force);
    accumulated_torque += torque_contribution;
    
    // Record for educational analysis
    record_force(force, application_point, torque_contribution, source, 
                ForceRecord::ForceType::Unknown);
}

void ForceAccumulator::apply_torque(f32 torque, const char* source) noexcept {
    accumulated_torque += torque;
    
    // Record pure torque application
    record_force(Vec2::zero(), Vec2::zero(), torque, source, 
                ForceRecord::ForceType::Motor);
}

void ForceAccumulator::apply_impulse(const Vec2& impulse, const char* source) noexcept {
    // Educational Note: Impulses are instantaneous momentum changes
    // Unlike forces, they're applied directly to velocity
    accumulated_impulse += impulse;
    
    // Record impulse for educational tracking
    if (force_count < MAX_FORCE_RECORDS) {
        ForceRecord& record = force_history[force_count++];
        record.force = impulse;
        record.application_point = Vec2::zero();
        record.torque_contribution = 0.0f;
        record.source_name = source;
        record.type = ForceRecord::ForceType::Contact;
    }
}

void ForceAccumulator::apply_angular_impulse(f32 impulse, const char* source) noexcept {
    accumulated_angular_impulse += impulse;
    
    // Record for analysis
    if (force_count < MAX_FORCE_RECORDS) {
        ForceRecord& record = force_history[force_count++];
        record.force = Vec2::zero();
        record.application_point = Vec2::zero();
        record.torque_contribution = impulse;
        record.source_name = source;
        record.type = ForceRecord::ForceType::Contact;
    }
}

void ForceAccumulator::apply_impulse_at_point(const Vec2& impulse, const Vec2& application_point, 
                                             const char* source) noexcept {
    // Linear impulse
    accumulated_impulse += impulse;
    
    // Angular impulse from point application
    f32 angular_impulse_contrib = vec2::cross(application_point, impulse);
    accumulated_angular_impulse += angular_impulse_contrib;
    
    // Record for educational analysis
    record_force(impulse, application_point, angular_impulse_contrib, source,
                ForceRecord::ForceType::Contact);
}

u8 ForceAccumulator::add_persistent_force(const Vec2& force_per_second, f32 torque_per_second,
                                         f32 duration, ForceRecord::ForceType type,
                                         const char* name) noexcept {
    if (persistent_force_count >= MAX_PERSISTENT_FORCES) {
        // No more slots available
        return MAX_PERSISTENT_FORCES;  // Invalid index
    }
    
    u8 index = persistent_force_count++;
    PersistentForce& persistent = persistent_forces[index];
    
    persistent.force_per_second = force_per_second;
    persistent.torque_per_second = torque_per_second;
    persistent.duration = duration;
    persistent.remaining_time = duration;
    persistent.is_active = true;
    persistent.type = type;
    persistent.name = name;
    
    return index;
}

void ForceAccumulator::remove_persistent_force(u8 index) noexcept {
    if (index >= persistent_force_count) {
        return;  // Invalid index
    }
    
    // Mark as inactive (actual removal is done during update)
    persistent_forces[index].is_active = false;
}

void ForceAccumulator::update_persistent_forces(f32 delta_time) noexcept {
    // Educational Note: This function demonstrates how continuous forces
    // are integrated over time in a physics simulation
    
    u8 active_count = 0;
    
    for (u8 i = 0; i < persistent_force_count; ++i) {
        PersistentForce& force = persistent_forces[i];
        
        if (!force.is_active) {
            continue;  // Skip inactive forces
        }
        
        // Apply force for this time step
        Vec2 force_this_frame = force.force_per_second * delta_time;
        f32 torque_this_frame = force.torque_per_second * delta_time;
        
        accumulated_force += force_this_frame;
        accumulated_torque += torque_this_frame;
        
        // Record for educational analysis
        record_force(force_this_frame, Vec2::zero(), torque_this_frame,
                    force.name, force.type);
        
        // Update duration
        if (force.duration > 0.0f) {  // -1 means infinite duration
            force.remaining_time -= delta_time;
            if (force.remaining_time <= 0.0f) {
                force.is_active = false;  // Force has expired
                continue;
            }
        }
        
        // Compact array by moving active forces to front
        if (active_count != i) {
            persistent_forces[active_count] = force;
        }
        active_count++;
    }
    
    persistent_force_count = active_count;
}

ForceAccumulator::ForceAnalysis ForceAccumulator::get_force_analysis() const noexcept {
    ForceAnalysis analysis;
    
    analysis.net_force = accumulated_force;
    analysis.net_torque = accumulated_torque;
    analysis.force_magnitude = accumulated_force.length();
    analysis.force_contributors = force_count;
    
    // Find dominant force type and calculate center of pressure
    if (force_count > 0) {
        std::array<f32, static_cast<usize>(ForceRecord::ForceType::Wind) + 1> force_magnitudes = {};
        Vec2 weighted_position_sum = Vec2::zero();
        f32 total_force_magnitude = 0.0f;
        f32 largest_magnitude = 0.0f;
        
        for (u8 i = 0; i < force_count; ++i) {
            const ForceRecord& record = force_history[i];
            f32 magnitude = record.force.length();
            
            // Track force type magnitudes
            usize type_index = static_cast<usize>(record.type);
            force_magnitudes[type_index] += magnitude;
            
            // Calculate center of pressure (weighted average of application points)
            if (magnitude > constants::EPSILON) {
                weighted_position_sum += record.application_point * magnitude;
                total_force_magnitude += magnitude;
                
                if (magnitude > largest_magnitude) {
                    largest_magnitude = magnitude;
                    analysis.largest_force_mag = magnitude;
                    analysis.dominant_force_type = record.type;
                }
            }
        }
        
        if (total_force_magnitude > constants::EPSILON) {
            analysis.center_of_pressure = weighted_position_sum / total_force_magnitude;
        }
    }
    
    return analysis;
}

std::array<Vec2, static_cast<usize>(ForceAccumulator::ForceRecord::ForceType::Wind) + 1> 
ForceAccumulator::get_force_breakdown_by_type() const noexcept {
    std::array<Vec2, static_cast<usize>(ForceRecord::ForceType::Wind) + 1> breakdown = {};
    
    for (u8 i = 0; i < force_count; ++i) {
        const ForceRecord& record = force_history[i];
        usize type_index = static_cast<usize>(record.type);
        breakdown[type_index] += record.force;
    }
    
    return breakdown;
}

f32 ForceAccumulator::calculate_work_done(const Vec2& displacement, f32 angular_displacement) const noexcept {
    // Educational Note: Work = Force · displacement (dot product)
    // This demonstrates the relationship between force, motion, and energy
    
    f32 linear_work = accumulated_force.dot(displacement);
    f32 rotational_work = accumulated_torque * angular_displacement;
    
    return linear_work + rotational_work;
}

f32 ForceAccumulator::calculate_power_output(const Vec2& velocity, f32 angular_velocity) const noexcept {
    // Educational Note: Power = Force · velocity (instantaneous rate of work)
    // This shows how power relates to the rate of energy transfer
    
    f32 linear_power = accumulated_force.dot(velocity);
    f32 rotational_power = accumulated_torque * angular_velocity;
    
    return linear_power + rotational_power;
}

void ForceAccumulator::record_force(const Vec2& force, const Vec2& application_point,
                                   f32 torque_contribution, const char* source,
                                   ForceRecord::ForceType type) noexcept {
    if (force_count >= MAX_FORCE_RECORDS) {
        return;  // History buffer is full
    }
    
    ForceRecord& record = force_history[force_count++];
    record.force = force;
    record.application_point = application_point;
    record.torque_contribution = torque_contribution;
    record.source_name = source;
    record.type = type;
    record.application_time = 0.0f;  // Would need time system integration for precise timing
}

//=============================================================================
// Constraint2D Implementation
//=============================================================================

Constraint2D Constraint2D::create_distance(u32 entity_a, u32 entity_b, 
                                          Vec2 anchor_a, Vec2 anchor_b, 
                                          f32 distance) noexcept {
    Constraint2D constraint;
    constraint.constraint_type = Type::Distance;
    constraint.entity_a = entity_a;
    constraint.entity_b = entity_b;
    constraint.local_anchor_a = anchor_a;
    constraint.local_anchor_b = anchor_b;
    constraint.target_value = distance;
    constraint.constraint_flags.is_active = 1;
    
    return constraint;
}

Constraint2D Constraint2D::create_spring(u32 entity_a, u32 entity_b,
                                        Vec2 anchor_a, Vec2 anchor_b,
                                        f32 rest_length, f32 spring_k, f32 damping) noexcept {
    Constraint2D constraint;
    constraint.constraint_type = Type::Spring;
    constraint.entity_a = entity_a;
    constraint.entity_b = entity_b;
    constraint.local_anchor_a = anchor_a;
    constraint.local_anchor_b = anchor_b;
    constraint.target_value = rest_length;
    constraint.spring_constant = spring_k;
    constraint.damping_ratio = damping;
    constraint.constraint_flags.is_active = 1;
    
    return constraint;
}

Constraint2D Constraint2D::create_revolute(u32 entity_a, u32 entity_b,
                                          Vec2 anchor_a, Vec2 anchor_b) noexcept {
    Constraint2D constraint;
    constraint.constraint_type = Type::Revolute;
    constraint.entity_a = entity_a;
    constraint.entity_b = entity_b;
    constraint.local_anchor_a = anchor_a;
    constraint.local_anchor_b = anchor_b;
    constraint.constraint_flags.is_active = 1;
    
    return constraint;
}

Constraint2D Constraint2D::create_motor(u32 entity_a, u32 entity_b,
                                       Vec2 anchor_a, Vec2 anchor_b,
                                       f32 target_speed, f32 max_torque) noexcept {
    Constraint2D constraint;
    constraint.constraint_type = Type::Motor;
    constraint.entity_a = entity_a;
    constraint.entity_b = entity_b;
    constraint.local_anchor_a = anchor_a;
    constraint.local_anchor_b = anchor_b;
    constraint.target_value = target_speed;
    constraint.max_force = max_torque;
    constraint.constraint_flags.is_active = 1;
    constraint.constraint_flags.motor_enabled = 1;
    
    return constraint;
}

const char* Constraint2D::get_type_name() const noexcept {
    switch (constraint_type) {
        case Type::Distance: return "Distance Joint";
        case Type::Revolute: return "Revolute Joint (Hinge)";
        case Type::Prismatic: return "Prismatic Joint (Slider)";
        case Type::Weld: return "Weld Joint (Fixed)";
        case Type::Spring: return "Spring Connection";
        case Type::Motor: return "Motor Joint";
        case Type::Rope: return "Rope Constraint";
        case Type::Pulley: return "Pulley System";
        case Type::Gear: return "Gear Connection";
        default: return "Unknown Constraint";
    }
}

bool Constraint2D::should_break() const noexcept {
    // Educational Note: Constraint breaking simulates real-world material limits
    // Objects can only withstand so much force before breaking
    
    if (constraint_flags.break_on_force) {
        f32 force_magnitude = solver_state.constraint_force.length();
        if (force_magnitude > max_force) {
            return true;
        }
    }
    
    if (constraint_flags.break_on_impulse) {
        if (std::abs(solver_state.constraint_impulse) > max_force * 0.1f) {  // Impulse threshold
            return true;
        }
    }
    
    return false;
}

//=============================================================================
// Trigger2D Implementation
//=============================================================================

bool Trigger2D::is_detecting(u32 entity_id) const noexcept {
    for (u8 i = 0; i < detected_count; ++i) {
        if (detected_entities[i] == entity_id) {
            return true;
        }
    }
    return false;
}

void Trigger2D::add_detected(u32 entity_id) noexcept {
    // Avoid duplicates
    if (is_detecting(entity_id)) {
        return;
    }
    
    if (detected_count < MAX_DETECTED) {
        detected_entities[detected_count++] = entity_id;
        statistics.total_entries++;
        statistics.current_occupants = detected_count;
    }
}

void Trigger2D::remove_detected(u32 entity_id) noexcept {
    for (u8 i = 0; i < detected_count; ++i) {
        if (detected_entities[i] == entity_id) {
            // Remove by swapping with last element
            detected_entities[i] = detected_entities[detected_count - 1];
            detected_count--;
            statistics.total_exits++;
            statistics.current_occupants = detected_count;
            break;
        }
    }
}

//=============================================================================
// PhysicsInfo Implementation
//=============================================================================

void PhysicsInfo::update_frame_metrics(f32 delta_time) noexcept {
    // Update simulation metrics
    simulation.total_simulation_time += delta_time;
    simulation.total_integration_steps += simulation.integration_steps_per_frame;
    
    // Update performance metrics (running averages)
    performance.average_frame_time = performance.average_frame_time * 0.9f + 
                                   simulation.last_frame_physics_time * 0.1f;
    
    if (simulation.last_frame_physics_time > performance.worst_frame_time) {
        performance.worst_frame_time = simulation.last_frame_physics_time;
    }
    
    if (simulation.last_frame_physics_time < performance.best_frame_time) {
        performance.best_frame_time = simulation.last_frame_physics_time;
    }
    
    // Estimate CPU usage (rough calculation)
    f32 target_frame_time = 1.0f / 60.0f;  // 60 FPS target
    performance.cpu_usage_percent = (simulation.last_frame_physics_time / target_frame_time) * 100.0f;
}

void PhysicsInfo::record_operation_time(const char* operation, f32 time) noexcept {
    // Educational feature: Track time spent in different physics subsystems
    if (strcmp(operation, "integration") == 0) {
        performance.integration_time = time;
    } else if (strcmp(operation, "collision") == 0) {
        performance.collision_time = time;
    } else if (strcmp(operation, "constraints") == 0) {
        performance.constraint_time = time;
    } else if (strcmp(operation, "broadphase") == 0) {
        performance.broadphase_time = time;
    } else if (strcmp(operation, "narrowphase") == 0) {
        performance.narrowphase_time = time;
    }
}

PhysicsInfo::PerformanceReport PhysicsInfo::get_performance_report() const noexcept {
    PerformanceReport report;
    
    // Calculate FPS equivalent
    if (performance.average_frame_time > 0.0f) {
        report.fps_equivalent = 1.0f / performance.average_frame_time;
    } else {
        report.fps_equivalent = 999.0f;  // Essentially infinite
    }
    
    report.cpu_percentage = performance.cpu_usage_percent;
    
    // Performance rating based on frame time
    if (performance.average_frame_time < 0.008f) {  // <8ms (120+ FPS)
        report.performance_rating = "Excellent";
    } else if (performance.average_frame_time < 0.016f) {  // <16ms (60+ FPS)
        report.performance_rating = "Good";
    } else if (performance.average_frame_time < 0.033f) {  // <33ms (30+ FPS)
        report.performance_rating = "Fair";
    } else {
        report.performance_rating = "Poor";
    }
    
    // Identify bottleneck
    f32 max_time = std::max({performance.integration_time, performance.collision_time, 
                            performance.constraint_time, performance.broadphase_time, 
                            performance.narrowphase_time});
    
    if (max_time == performance.collision_time) {
        report.bottleneck = "Collision Detection";
        report.optimization_advice = "Consider spatial partitioning, simpler collision shapes, or reducing object count";
    } else if (max_time == performance.constraint_time) {
        report.bottleneck = "Constraint Solving";
        report.optimization_advice = "Reduce constraint count or solver iterations";
    } else if (max_time == performance.integration_time) {
        report.bottleneck = "Physics Integration";
        report.optimization_advice = "Consider simpler integration method or fewer active bodies";
    } else if (max_time == performance.narrowphase_time) {
        report.bottleneck = "Narrow-phase Collision";
        report.optimization_advice = "Use simpler collision shapes or improve broad-phase filtering";
    } else {
        report.bottleneck = "Broad-phase Collision";
        report.optimization_advice = "Optimize spatial data structures or reduce active object count";
    }
    
    return report;
}

//=============================================================================
// MotionState Implementation
//=============================================================================

void MotionState::update_transform_cache(const Transform& transform) noexcept {
    transform_cache.world_position = transform.position;
    transform_cache.world_rotation = transform.rotation;
    transform_cache.world_scale = transform.scale;
    transform_cache.rotation_matrix = Matrix2::rotation(transform.rotation);
    transform_cache.is_dirty = false;
    
    metrics.cache_hits++;
}

void MotionState::update_motion_cache(const RigidBody2D& rigidbody) noexcept {
    motion_cache.velocity = rigidbody.velocity;
    motion_cache.angular_velocity = rigidbody.angular_velocity;
    motion_cache.speed = rigidbody.velocity.length();
    
    if (motion_cache.speed > constants::EPSILON) {
        motion_cache.velocity_direction = rigidbody.velocity / motion_cache.speed;
    } else {
        motion_cache.velocity_direction = Vec2::zero();
    }
    
    motion_cache.is_moving = motion_cache.speed > rigidbody.sleep_threshold;
    motion_cache.is_rotating = std::abs(rigidbody.angular_velocity) > rigidbody.sleep_threshold;
    motion_cache.last_update_time = 0.0f;  // Would need time system integration
    
    metrics.cache_hits++;
}

void MotionState::update_collision_cache(const Transform& transform, const Collider2D& collider) noexcept {
    collision_cache.world_aabb = collider.get_world_aabb(transform);
    collision_cache.aabb_center = collision_cache.world_aabb.center();
    collision_cache.aabb_extents = collision_cache.world_aabb.half_size();
    collision_cache.has_moved = false;
    
    metrics.cache_hits++;
}

const AABB& MotionState::get_world_aabb(const Transform& transform, const Collider2D& collider) noexcept {
    if (collision_cache.has_moved || 
        has_moved_significantly(transform.position, transform.rotation)) {
        update_collision_cache(transform, collider);
    } else {
        metrics.cache_hits++;
    }
    
    return collision_cache.world_aabb;
}

const Matrix2& MotionState::get_rotation_matrix(const Transform& transform) noexcept {
    if (transform_cache.is_dirty ||
        std::abs(transform.rotation - transform_cache.world_rotation) > constants::ANGULAR_SLOP) {
        update_transform_cache(transform);
    } else {
        metrics.cache_hits++;
    }
    
    return transform_cache.rotation_matrix;
}

bool MotionState::has_moved_significantly(const Vec2& new_position, f32 new_rotation) const noexcept {
    f32 position_delta = vec2::distance(new_position, transform_cache.world_position);
    f32 rotation_delta = std::abs(math::utils::angle_difference(new_rotation, transform_cache.world_rotation));
    
    return position_delta > collision_cache.movement_threshold ||
           rotation_delta > constants::ANGULAR_SLOP;
}

//=============================================================================
// Utility Functions Implementation
//=============================================================================

namespace utils {

f32 calculate_mass_from_shape_and_material(const CollisionShape& shape, const PhysicsMaterial& material) noexcept {
    // Educational Note: Mass calculation from volume and density
    // This demonstrates how real-world physics properties are related
    
    f32 volume = std::visit([](const auto& collision_shape) -> f32 {
        using ShapeType = std::decay_t<decltype(collision_shape)>;
        
        if constexpr (std::is_same_v<ShapeType, Circle>) {
            return collision_shape.area();
        } else if constexpr (std::is_same_v<ShapeType, AABB>) {
            return collision_shape.area();
        } else if constexpr (std::is_same_v<ShapeType, OBB>) {
            return collision_shape.area();
        } else if constexpr (std::is_same_v<ShapeType, Polygon>) {
            return collision_shape.get_area();
        }
        return 1.0f;  // Default area
    }, shape);
    
    return volume * material.density;
}

f32 calculate_moment_of_inertia_from_shape(const CollisionShape& shape, f32 mass) noexcept {
    return std::visit([mass](const auto& collision_shape) -> f32 {
        using ShapeType = std::decay_t<decltype(collision_shape)>;
        
        if constexpr (std::is_same_v<ShapeType, Circle>) {
            return math::utils::moment_of_inertia_circle(mass, collision_shape.radius);
        } else if constexpr (std::is_same_v<ShapeType, AABB>) {
            return math::utils::moment_of_inertia_box(mass, collision_shape.width(), collision_shape.height());
        } else if constexpr (std::is_same_v<ShapeType, OBB>) {
            f32 width = collision_shape.half_extents.x * 2.0f;
            f32 height = collision_shape.half_extents.y * 2.0f;
            return math::utils::moment_of_inertia_box(mass, width, height);
        } else if constexpr (std::is_same_v<ShapeType, Polygon>) {
            return math::utils::moment_of_inertia_polygon(mass, collision_shape);
        }
        return mass;  // Fallback
    }, shape);
}

PhysicsComponents create_physics_entity(const PhysicsEntityDesc& desc) noexcept {
    PhysicsComponents components;
    
    // Create rigid body
    if (desc.is_static) {
        components.rigidbody.make_static();
    } else if (desc.is_kinematic) {
        components.rigidbody.make_kinematic();
        components.rigidbody.set_mass(desc.mass);
    } else {
        components.rigidbody.set_mass(desc.mass);
    }
    
    // Calculate moment of inertia from shape
    components.rigidbody.calculate_moment_of_inertia_from_shape(desc.shape);
    
    // Create collider
    components.collider = Collider2D(desc.shape, desc.material);
    if (desc.is_trigger) {
        components.collider.collision_flags.is_trigger = 1;
    }
    
    // Initialize force accumulator
    components.forces = ForceAccumulator{};
    
    // Add debug info for educational builds
    #ifdef ECSCOPE_EDUCATIONAL_BUILD
    components.debug_info = PhysicsInfo{};
    components.motion_cache = MotionState{};
    #endif
    
    return components;
}

bool validate_physics_components(const RigidBody2D* rigidbody, 
                               const Collider2D* collider,
                               const ForceAccumulator* forces) noexcept {
    // Educational Note: Component validation ensures consistent physics state
    // Invalid combinations can lead to simulation instability or crashes
    
    if (!rigidbody || !collider) {
        return false;
    }
    
    if (!rigidbody->is_valid()) {
        return false;
    }
    
    if (!collider->is_valid()) {
        return false;
    }
    
    if (forces && !forces->is_valid()) {
        return false;
    }
    
    // Check for consistent static/kinematic flags
    if (rigidbody->physics_flags.is_static && rigidbody->inverse_mass > 0.0f) {
        return false;
    }
    
    // Check material-mass consistency
    if (rigidbody->mass <= 0.0f && !rigidbody->physics_flags.is_static) {
        return false;
    }
    
    return true;
}

} // namespace utils

} // namespace ecscope::physics::components