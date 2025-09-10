#pragma once

#include "physics_math.hpp"
#include "rigid_body.hpp"
#include <vector>
#include <memory>
#include <functional>

namespace ecscope::physics {

// Base constraint class
class Constraint {
public:
    enum Type {
        Distance, Pin, Hinge, Slider, Fixed, Contact
    };
    
    Type type;
    uint32_t body_a_id;
    uint32_t body_b_id;
    bool is_active = true;
    Real compliance = 0.0f;  // Constraint softness (0 = rigid, higher = softer)
    
    Constraint(Type constraint_type, uint32_t a_id, uint32_t b_id) 
        : type(constraint_type), body_a_id(a_id), body_b_id(b_id) {}
    
    virtual ~Constraint() = default;
    
    // Solve constraint using Position-Based Dynamics (PBD)
    virtual void solve_position(RigidBody3D& body_a, RigidBody3D& body_b, Real dt) = 0;
    virtual void solve_velocity(RigidBody3D& body_a, RigidBody3D& body_b, Real dt) = 0;
    
    // 2D versions
    virtual void solve_position_2d(RigidBody2D& body_a, RigidBody2D& body_b, Real dt) {}
    virtual void solve_velocity_2d(RigidBody2D& body_a, RigidBody2D& body_b, Real dt) {}
    
    // Prepare constraint for solving (called once per frame)
    virtual void prepare(const RigidBody3D& body_a, const RigidBody3D& body_b, Real dt) {}
    virtual void prepare_2d(const RigidBody2D& body_a, const RigidBody2D& body_b, Real dt) {}
};

// Distance constraint - maintains fixed distance between two points
class DistanceConstraint : public Constraint {
public:
    Vec3 anchor_a;    // Local anchor point on body A
    Vec3 anchor_b;    // Local anchor point on body B
    Real rest_distance;
    Real damping = 0.1f;
    
    // Cached values for solving
    Vec3 world_anchor_a, world_anchor_b;
    Vec3 constraint_direction;
    Real current_distance;
    Real constraint_mass;
    Real bias;
    Real accumulated_impulse = 0.0f;
    
    DistanceConstraint(uint32_t a_id, uint32_t b_id, const Vec3& anchor_point_a, 
                      const Vec3& anchor_point_b, Real distance)
        : Constraint(Distance, a_id, b_id), anchor_a(anchor_point_a), 
          anchor_b(anchor_point_b), rest_distance(distance) {}
    
    void prepare(const RigidBody3D& body_a, const RigidBody3D& body_b, Real dt) override {
        world_anchor_a = body_a.transform.transform_point(anchor_a);
        world_anchor_b = body_b.transform.transform_point(anchor_b);
        
        Vec3 delta = world_anchor_b - world_anchor_a;
        current_distance = delta.length();
        
        if (current_distance > PHYSICS_EPSILON) {
            constraint_direction = delta / current_distance;
        } else {
            constraint_direction = Vec3::unit_x();
        }
        
        // Calculate constraint mass (effective mass in constraint direction)
        Vec3 r_a = world_anchor_a - body_a.transform.position;
        Vec3 r_b = world_anchor_b - body_b.transform.position;
        
        Mat3 inv_inertia_a = body_a.get_world_inverse_inertia();
        Mat3 inv_inertia_b = body_b.get_world_inverse_inertia();
        
        Vec3 u1 = r_a.cross(constraint_direction);
        Vec3 u2 = r_b.cross(constraint_direction);
        
        constraint_mass = body_a.mass_props.inverse_mass + body_b.mass_props.inverse_mass +
                         u1.dot(inv_inertia_a * u1) + u2.dot(inv_inertia_b * u2);
        
        if (constraint_mass > PHYSICS_EPSILON) {
            constraint_mass = 1.0f / constraint_mass;
        } else {
            constraint_mass = 0.0f;
        }
        
        // Bias for position correction (Baumgarte stabilization)
        Real error = current_distance - rest_distance;
        bias = -(0.2f / dt) * error;  // 0.2 is the Baumgarte factor
    }
    
    void solve_position(RigidBody3D& body_a, RigidBody3D& body_b, Real dt) override {
        if (constraint_mass <= PHYSICS_EPSILON) return;
        
        world_anchor_a = body_a.transform.transform_point(anchor_a);
        world_anchor_b = body_b.transform.transform_point(anchor_b);
        
        Vec3 delta = world_anchor_b - world_anchor_a;
        Real current_dist = delta.length();
        
        if (current_dist <= PHYSICS_EPSILON) return;
        
        Vec3 direction = delta / current_dist;
        Real constraint_error = current_dist - rest_distance;
        
        // Position correction
        Real correction_magnitude = -constraint_error * 0.8f;  // Position correction factor
        Vec3 correction = direction * correction_magnitude;
        
        Vec3 r_a = world_anchor_a - body_a.transform.position;
        Vec3 r_b = world_anchor_b - body_b.transform.position;
        
        // Apply position corrections
        body_a.transform.position -= correction * body_a.mass_props.inverse_mass * 0.5f;
        body_b.transform.position += correction * body_b.mass_props.inverse_mass * 0.5f;
        
        // Apply rotation corrections
        Mat3 inv_inertia_a = body_a.get_world_inverse_inertia();
        Mat3 inv_inertia_b = body_b.get_world_inverse_inertia();
        
        Vec3 angular_correction_a = inv_inertia_a * (r_a.cross(correction * -0.5f));
        Vec3 angular_correction_b = inv_inertia_b * (r_b.cross(correction * 0.5f));
        
        // Convert angular corrections to quaternions (small angle approximation)
        if (angular_correction_a.length_squared() > PHYSICS_EPSILON) {
            Real angle_a = angular_correction_a.length();
            Vec3 axis_a = angular_correction_a / angle_a;
            Quaternion rot_delta_a = Quaternion::from_axis_angle(axis_a, angle_a);
            body_a.transform.rotation = (rot_delta_a * body_a.transform.rotation).normalized();
        }
        
        if (angular_correction_b.length_squared() > PHYSICS_EPSILON) {
            Real angle_b = angular_correction_b.length();
            Vec3 axis_b = angular_correction_b / angle_b;
            Quaternion rot_delta_b = Quaternion::from_axis_angle(axis_b, angle_b);
            body_b.transform.rotation = (rot_delta_b * body_b.transform.rotation).normalized();
        }
    }
    
    void solve_velocity(RigidBody3D& body_a, RigidBody3D& body_b, Real dt) override {
        if (constraint_mass <= PHYSICS_EPSILON) return;
        
        Vec3 r_a = world_anchor_a - body_a.transform.position;
        Vec3 r_b = world_anchor_b - body_b.transform.position;
        
        // Calculate relative velocity at constraint point
        Vec3 va = body_a.velocity + body_a.angular_velocity.cross(r_a);
        Vec3 vb = body_b.velocity + body_b.angular_velocity.cross(r_b);
        Vec3 relative_velocity = vb - va;
        
        Real velocity_along_normal = relative_velocity.dot(constraint_direction);
        
        // Apply damping
        Real damping_factor = velocity_along_normal * damping;
        
        // Calculate impulse
        Real impulse_magnitude = -(velocity_along_normal + bias + damping_factor) * constraint_mass;
        Vec3 impulse = constraint_direction * impulse_magnitude;
        
        accumulated_impulse += impulse_magnitude;
        
        // Apply impulses
        body_a.velocity -= impulse * body_a.mass_props.inverse_mass;
        body_b.velocity += impulse * body_b.mass_props.inverse_mass;
        
        Mat3 inv_inertia_a = body_a.get_world_inverse_inertia();
        Mat3 inv_inertia_b = body_b.get_world_inverse_inertia();
        
        body_a.angular_velocity -= inv_inertia_a * r_a.cross(impulse);
        body_b.angular_velocity += inv_inertia_b * r_b.cross(impulse);
    }
};

// Pin constraint - constrains a point on body A to a fixed world position
class PinConstraint : public Constraint {
public:
    Vec3 anchor;          // Local anchor point on body
    Vec3 world_position;  // Fixed world position
    
    // Cached values
    Vec3 world_anchor;
    Mat3 constraint_mass_matrix;
    Vec3 bias;
    Vec3 accumulated_impulse = Vec3::zero();
    
    PinConstraint(uint32_t body_id, const Vec3& anchor_point, const Vec3& world_pos)
        : Constraint(Pin, body_id, 0), anchor(anchor_point), world_position(world_pos) {}
    
    void prepare(const RigidBody3D& body_a, const RigidBody3D& body_b, Real dt) override {
        world_anchor = body_a.transform.transform_point(anchor);
        
        Vec3 r = world_anchor - body_a.transform.position;
        Mat3 r_cross = Mat3();  // Skew symmetric matrix for cross product
        r_cross(0, 1) = -r.z; r_cross(0, 2) = r.y;
        r_cross(1, 0) = r.z;  r_cross(1, 2) = -r.x;
        r_cross(2, 0) = -r.y; r_cross(2, 1) = r.x;
        
        Mat3 inv_inertia = body_a.get_world_inverse_inertia();
        Mat3 identity = Mat3::identity();
        
        constraint_mass_matrix = identity * body_a.mass_props.inverse_mass + 
                                r_cross * inv_inertia * r_cross;
        constraint_mass_matrix = constraint_mass_matrix.inverse();
        
        // Position error for bias
        Vec3 position_error = world_anchor - world_position;
        bias = position_error * (-0.2f / dt);  // Baumgarte stabilization
    }
    
    void solve_position(RigidBody3D& body_a, RigidBody3D& body_b, Real dt) override {
        world_anchor = body_a.transform.transform_point(anchor);
        Vec3 position_error = world_anchor - world_position;
        
        if (position_error.length_squared() <= PHYSICS_EPSILON) return;
        
        Vec3 correction = position_error * -0.8f;  // Position correction factor
        Vec3 r = world_anchor - body_a.transform.position;
        
        // Apply linear correction
        body_a.transform.position += correction * body_a.mass_props.inverse_mass;
        
        // Apply angular correction
        Mat3 inv_inertia = body_a.get_world_inverse_inertia();
        Vec3 angular_correction = inv_inertia * r.cross(correction);
        
        if (angular_correction.length_squared() > PHYSICS_EPSILON) {
            Real angle = angular_correction.length();
            Vec3 axis = angular_correction / angle;
            Quaternion rot_delta = Quaternion::from_axis_angle(axis, angle);
            body_a.transform.rotation = (rot_delta * body_a.transform.rotation).normalized();
        }
    }
    
    void solve_velocity(RigidBody3D& body_a, RigidBody3D& body_b, Real dt) override {
        Vec3 r = world_anchor - body_a.transform.position;
        Vec3 velocity = body_a.velocity + body_a.angular_velocity.cross(r);
        
        Vec3 constraint_velocity = velocity + bias;
        Vec3 impulse = constraint_mass_matrix * (constraint_velocity * -1);
        
        accumulated_impulse += impulse;
        
        // Apply impulse
        body_a.velocity += impulse * body_a.mass_props.inverse_mass;
        Mat3 inv_inertia = body_a.get_world_inverse_inertia();
        body_a.angular_velocity += inv_inertia * r.cross(impulse);
    }
};

// Hinge constraint - allows rotation around one axis
class HingeConstraint : public Constraint {
public:
    Vec3 anchor_a, anchor_b;    // Local anchor points
    Vec3 axis_a, axis_b;        // Local hinge axes
    Real lower_limit = -PI;     // Lower rotation limit
    Real upper_limit = PI;      // Upper rotation limit
    bool enable_limits = false;
    bool enable_motor = false;
    Real motor_speed = 0.0f;
    Real max_motor_torque = 1000.0f;
    
    // Cached values
    Vec3 world_anchor_a, world_anchor_b;
    Vec3 world_axis_a, world_axis_b;
    Mat3 constraint_mass_matrix;
    Vec3 bias = Vec3::zero();
    Vec3 accumulated_impulse = Vec3::zero();
    Real accumulated_motor_impulse = 0.0f;
    
    HingeConstraint(uint32_t a_id, uint32_t b_id, const Vec3& anchor_point_a, 
                   const Vec3& anchor_point_b, const Vec3& hinge_axis_a, const Vec3& hinge_axis_b)
        : Constraint(Hinge, a_id, b_id), anchor_a(anchor_point_a), anchor_b(anchor_point_b),
          axis_a(hinge_axis_a), axis_b(hinge_axis_b) {}
    
    void prepare(const RigidBody3D& body_a, const RigidBody3D& body_b, Real dt) override {
        world_anchor_a = body_a.transform.transform_point(anchor_a);
        world_anchor_b = body_b.transform.transform_point(anchor_b);
        world_axis_a = body_a.transform.transform_vector(axis_a);
        world_axis_b = body_b.transform.transform_vector(axis_b);
        
        // Calculate constraint mass matrix for position constraints
        Vec3 r_a = world_anchor_a - body_a.transform.position;
        Vec3 r_b = world_anchor_b - body_b.transform.position;
        
        // This is a simplified version - full implementation would handle all 5 DOF constraints
        // (3 for position, 2 for rotation perpendicular to hinge axis)
        
        // Position error for bias
        Vec3 position_error = world_anchor_b - world_anchor_a;
        bias = position_error * (-0.2f / dt);
    }
    
    void solve_position(RigidBody3D& body_a, RigidBody3D& body_b, Real dt) override {
        world_anchor_a = body_a.transform.transform_point(anchor_a);
        world_anchor_b = body_b.transform.transform_point(anchor_b);
        
        Vec3 position_error = world_anchor_b - world_anchor_a;
        
        if (position_error.length_squared() <= PHYSICS_EPSILON) return;
        
        // Solve position constraint
        Vec3 correction = position_error * -0.5f;
        
        body_a.transform.position += correction * body_a.mass_props.inverse_mass / 
                                   (body_a.mass_props.inverse_mass + body_b.mass_props.inverse_mass);
        body_b.transform.position -= correction * body_b.mass_props.inverse_mass / 
                                   (body_a.mass_props.inverse_mass + body_b.mass_props.inverse_mass);
        
        // Solve rotation constraints (align axes perpendicular to hinge)
        world_axis_a = body_a.transform.transform_vector(axis_a);
        world_axis_b = body_b.transform.transform_vector(axis_b);
        
        // Find two vectors perpendicular to the hinge axis
        Vec3 perp1_a = world_axis_a.cross(Vec3::unit_x());
        if (perp1_a.length_squared() < 0.1f) {
            perp1_a = world_axis_a.cross(Vec3::unit_y());
        }
        perp1_a = perp1_a.normalized();
        Vec3 perp2_a = world_axis_a.cross(perp1_a).normalized();
        
        // Project body B's perpendicular vectors onto body A's perpendicular plane
        Vec3 perp1_b = world_axis_b.cross(Vec3::unit_x());
        if (perp1_b.length_squared() < 0.1f) {
            perp1_b = world_axis_b.cross(Vec3::unit_y());
        }
        perp1_b = perp1_b.normalized();
        
        // Calculate rotation correction to align perpendicular components
        Real dot_error = perp1_a.dot(perp1_b);
        if (std::abs(dot_error) < 0.99f) {  // Not already aligned
            Vec3 rotation_axis = perp1_a.cross(perp1_b).normalized();
            Real rotation_angle = std::acos(clamp(dot_error, -1.0f, 1.0f)) * 0.5f;
            
            if (rotation_angle > PHYSICS_EPSILON) {
                Quaternion correction_a = Quaternion::from_axis_angle(rotation_axis, rotation_angle * 0.5f);
                Quaternion correction_b = Quaternion::from_axis_angle(rotation_axis, -rotation_angle * 0.5f);
                
                body_a.transform.rotation = (correction_a * body_a.transform.rotation).normalized();
                body_b.transform.rotation = (correction_b * body_b.transform.rotation).normalized();
            }
        }
    }
    
    void solve_velocity(RigidBody3D& body_a, RigidBody3D& body_b, Real dt) override {
        // Solve velocity constraints for position
        Vec3 r_a = world_anchor_a - body_a.transform.position;
        Vec3 r_b = world_anchor_b - body_b.transform.position;
        
        Vec3 va = body_a.velocity + body_a.angular_velocity.cross(r_a);
        Vec3 vb = body_b.velocity + body_b.angular_velocity.cross(r_b);
        Vec3 relative_velocity = vb - va;
        
        // This is a simplified velocity solve - full implementation would solve all constraints
        Vec3 impulse = relative_velocity * -0.1f;  // Simple damping
        
        body_a.velocity += impulse * body_a.mass_props.inverse_mass;
        body_b.velocity -= impulse * body_b.mass_props.inverse_mass;
        
        // Motor constraint
        if (enable_motor) {
            Vec3 relative_angular_velocity = body_b.angular_velocity - body_a.angular_velocity;
            Real current_angular_speed = relative_angular_velocity.dot(world_axis_a);
            Real motor_error = motor_speed - current_angular_speed;
            
            Real motor_impulse = motor_error * 0.1f;  // Simple proportional control
            motor_impulse = clamp(motor_impulse, -max_motor_torque * dt, max_motor_torque * dt);
            
            Vec3 motor_impulse_vector = world_axis_a * motor_impulse;
            
            Mat3 inv_inertia_a = body_a.get_world_inverse_inertia();
            Mat3 inv_inertia_b = body_b.get_world_inverse_inertia();
            
            body_a.angular_velocity -= inv_inertia_a * motor_impulse_vector;
            body_b.angular_velocity += inv_inertia_b * motor_impulse_vector;
        }
    }
};

// Contact constraint - used by collision resolution system
class ContactConstraint : public Constraint {
public:
    Vec3 contact_point_a, contact_point_b;
    Vec3 normal;
    Real penetration;
    Real friction_coefficient;
    Real restitution_coefficient;
    
    // Cached values
    Real normal_mass;
    Real tangent_mass_1, tangent_mass_2;
    Vec3 tangent_1, tangent_2;
    Real velocity_bias;
    Real accumulated_normal_impulse = 0.0f;
    Real accumulated_tangent_impulse_1 = 0.0f;
    Real accumulated_tangent_impulse_2 = 0.0f;
    
    ContactConstraint(uint32_t a_id, uint32_t b_id, const Vec3& contact_a, 
                     const Vec3& contact_b, const Vec3& contact_normal, Real depth)
        : Constraint(Contact, a_id, b_id), contact_point_a(contact_a), 
          contact_point_b(contact_b), normal(contact_normal), penetration(depth) {}
    
    void prepare(const RigidBody3D& body_a, const RigidBody3D& body_b, Real dt) override {
        Vec3 r_a = contact_point_a - body_a.transform.position;
        Vec3 r_b = contact_point_b - body_b.transform.position;
        
        // Calculate effective mass for normal constraint
        Mat3 inv_inertia_a = body_a.get_world_inverse_inertia();
        Mat3 inv_inertia_b = body_b.get_world_inverse_inertia();
        
        Vec3 rn_a = r_a.cross(normal);
        Vec3 rn_b = r_b.cross(normal);
        
        normal_mass = body_a.mass_props.inverse_mass + body_b.mass_props.inverse_mass +
                     rn_a.dot(inv_inertia_a * rn_a) + rn_b.dot(inv_inertia_b * rn_b);
        
        if (normal_mass > PHYSICS_EPSILON) {
            normal_mass = 1.0f / normal_mass;
        }
        
        // Generate tangent vectors
        if (std::abs(normal.x) >= 0.57735f) {
            tangent_1 = Vec3(normal.y, -normal.x, 0.0f).normalized();
        } else {
            tangent_1 = Vec3(0.0f, normal.z, -normal.y).normalized();
        }
        tangent_2 = normal.cross(tangent_1);
        
        // Calculate tangent masses
        Vec3 rt1_a = r_a.cross(tangent_1);
        Vec3 rt1_b = r_b.cross(tangent_1);
        tangent_mass_1 = body_a.mass_props.inverse_mass + body_b.mass_props.inverse_mass +
                        rt1_a.dot(inv_inertia_a * rt1_a) + rt1_b.dot(inv_inertia_b * rt1_b);
        
        Vec3 rt2_a = r_a.cross(tangent_2);
        Vec3 rt2_b = r_b.cross(tangent_2);
        tangent_mass_2 = body_a.mass_props.inverse_mass + body_b.mass_props.inverse_mass +
                        rt2_a.dot(inv_inertia_a * rt2_a) + rt2_b.dot(inv_inertia_b * rt2_b);
        
        if (tangent_mass_1 > PHYSICS_EPSILON) tangent_mass_1 = 1.0f / tangent_mass_1;
        if (tangent_mass_2 > PHYSICS_EPSILON) tangent_mass_2 = 1.0f / tangent_mass_2;
        
        // Velocity bias for restitution
        Vec3 va = body_a.velocity + body_a.angular_velocity.cross(r_a);
        Vec3 vb = body_b.velocity + body_b.angular_velocity.cross(r_b);
        Real relative_velocity = (vb - va).dot(normal);
        
        if (relative_velocity < -1.0f) {  // Only apply restitution for significant velocities
            velocity_bias = -restitution_coefficient * relative_velocity;
        } else {
            velocity_bias = 0.0f;
        }
        
        // Add position bias for penetration correction
        velocity_bias += (0.2f / dt) * std::max(0.0f, penetration - 0.01f);  // Slop = 0.01
    }
    
    void solve_position(RigidBody3D& body_a, RigidBody3D& body_b, Real dt) override {
        if (penetration <= 0.01f) return;  // No penetration to resolve
        
        Real correction_magnitude = penetration * 0.8f;  // Position correction factor
        Vec3 correction = normal * correction_magnitude;
        
        Real total_inverse_mass = body_a.mass_props.inverse_mass + body_b.mass_props.inverse_mass;
        if (total_inverse_mass > PHYSICS_EPSILON) {
            Vec3 correction_a = correction * (body_a.mass_props.inverse_mass / total_inverse_mass);
            Vec3 correction_b = correction * (body_b.mass_props.inverse_mass / total_inverse_mass);
            
            body_a.transform.position -= correction_a;
            body_b.transform.position += correction_b;
        }
    }
    
    void solve_velocity(RigidBody3D& body_a, RigidBody3D& body_b, Real dt) override {
        Vec3 r_a = contact_point_a - body_a.transform.position;
        Vec3 r_b = contact_point_b - body_b.transform.position;
        
        // Solve normal constraint
        Vec3 va = body_a.velocity + body_a.angular_velocity.cross(r_a);
        Vec3 vb = body_b.velocity + body_b.angular_velocity.cross(r_b);
        Real relative_normal_velocity = (vb - va).dot(normal);
        
        Real impulse_magnitude = -(relative_normal_velocity + velocity_bias) * normal_mass;
        
        // Clamp accumulated impulse to be non-negative (no pulling)
        Real new_impulse = std::max(accumulated_normal_impulse + impulse_magnitude, 0.0f);
        impulse_magnitude = new_impulse - accumulated_normal_impulse;
        accumulated_normal_impulse = new_impulse;
        
        Vec3 impulse = normal * impulse_magnitude;
        
        // Apply normal impulse
        body_a.velocity -= impulse * body_a.mass_props.inverse_mass;
        body_b.velocity += impulse * body_b.mass_props.inverse_mass;
        
        Mat3 inv_inertia_a = body_a.get_world_inverse_inertia();
        Mat3 inv_inertia_b = body_b.get_world_inverse_inertia();
        
        body_a.angular_velocity -= inv_inertia_a * r_a.cross(impulse);
        body_b.angular_velocity += inv_inertia_b * r_b.cross(impulse);
        
        // Solve friction constraints
        va = body_a.velocity + body_a.angular_velocity.cross(r_a);
        vb = body_b.velocity + body_b.angular_velocity.cross(r_b);
        Vec3 relative_velocity = vb - va;
        
        Real tangent_velocity_1 = relative_velocity.dot(tangent_1);
        Real tangent_impulse_1 = -tangent_velocity_1 * tangent_mass_1;
        
        Real max_friction = friction_coefficient * accumulated_normal_impulse;
        Real new_tangent_impulse_1 = clamp(accumulated_tangent_impulse_1 + tangent_impulse_1, 
                                          -max_friction, max_friction);
        tangent_impulse_1 = new_tangent_impulse_1 - accumulated_tangent_impulse_1;
        accumulated_tangent_impulse_1 = new_tangent_impulse_1;
        
        Vec3 friction_impulse_1 = tangent_1 * tangent_impulse_1;
        
        // Apply tangent impulse 1
        body_a.velocity -= friction_impulse_1 * body_a.mass_props.inverse_mass;
        body_b.velocity += friction_impulse_1 * body_b.mass_props.inverse_mass;
        body_a.angular_velocity -= inv_inertia_a * r_a.cross(friction_impulse_1);
        body_b.angular_velocity += inv_inertia_b * r_b.cross(friction_impulse_1);
        
        // Similar for tangent_2
        Real tangent_velocity_2 = relative_velocity.dot(tangent_2);
        Real tangent_impulse_2 = -tangent_velocity_2 * tangent_mass_2;
        
        Real new_tangent_impulse_2 = clamp(accumulated_tangent_impulse_2 + tangent_impulse_2,
                                          -max_friction, max_friction);
        tangent_impulse_2 = new_tangent_impulse_2 - accumulated_tangent_impulse_2;
        accumulated_tangent_impulse_2 = new_tangent_impulse_2;
        
        Vec3 friction_impulse_2 = tangent_2 * tangent_impulse_2;
        
        body_a.velocity -= friction_impulse_2 * body_a.mass_props.inverse_mass;
        body_b.velocity += friction_impulse_2 * body_b.mass_props.inverse_mass;
        body_a.angular_velocity -= inv_inertia_a * r_a.cross(friction_impulse_2);
        body_b.angular_velocity += inv_inertia_b * r_b.cross(friction_impulse_2);
    }
};

// Constraint solver using iterative methods
class ConstraintSolver {
private:
    std::vector<std::unique_ptr<Constraint>> constraints;
    int position_iterations = 4;
    int velocity_iterations = 8;
    
public:
    void add_constraint(std::unique_ptr<Constraint> constraint) {
        constraints.push_back(std::move(constraint));
    }
    
    void remove_constraint(size_t index) {
        if (index < constraints.size()) {
            constraints.erase(constraints.begin() + index);
        }
    }
    
    void clear_constraints() {
        constraints.clear();
    }
    
    void set_iterations(int pos_iters, int vel_iters) {
        position_iterations = pos_iters;
        velocity_iterations = vel_iters;
    }
    
    // Solve constraints for 3D bodies
    void solve_constraints(std::vector<RigidBody3D>& bodies, Real dt) {
        // Create body lookup map
        std::unordered_map<uint32_t, RigidBody3D*> body_map;
        for (auto& body : bodies) {
            body_map[body.id] = &body;
        }
        
        // Prepare all constraints
        for (auto& constraint : constraints) {
            if (!constraint->is_active) continue;
            
            auto* body_a = body_map[constraint->body_a_id];
            auto* body_b = constraint->body_b_id != 0 ? body_map[constraint->body_b_id] : nullptr;
            
            if (body_a && (body_b || constraint->body_b_id == 0)) {
                RigidBody3D dummy_body;
                constraint->prepare(*body_a, body_b ? *body_b : dummy_body, dt);
            }
        }
        
        // Solve position constraints
        for (int iter = 0; iter < position_iterations; ++iter) {
            for (auto& constraint : constraints) {
                if (!constraint->is_active) continue;
                
                auto* body_a = body_map[constraint->body_a_id];
                auto* body_b = constraint->body_b_id != 0 ? body_map[constraint->body_b_id] : nullptr;
                
                if (body_a && (body_b || constraint->body_b_id == 0)) {
                    RigidBody3D dummy_body;
                    constraint->solve_position(*body_a, body_b ? *body_b : dummy_body, dt);
                }
            }
        }
        
        // Solve velocity constraints
        for (int iter = 0; iter < velocity_iterations; ++iter) {
            for (auto& constraint : constraints) {
                if (!constraint->is_active) continue;
                
                auto* body_a = body_map[constraint->body_a_id];
                auto* body_b = constraint->body_b_id != 0 ? body_map[constraint->body_b_id] : nullptr;
                
                if (body_a && (body_b || constraint->body_b_id == 0)) {
                    RigidBody3D dummy_body;
                    constraint->solve_velocity(*body_a, body_b ? *body_b : dummy_body, dt);
                }
            }
        }
    }
    
    size_t get_constraint_count() const { return constraints.size(); }
    
    Constraint* get_constraint(size_t index) {
        return index < constraints.size() ? constraints[index].get() : nullptr;
    }
};

} // namespace ecscope::physics