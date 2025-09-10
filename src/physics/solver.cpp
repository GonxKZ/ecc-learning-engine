#include "ecscope/physics/constraints.hpp"
#include <algorithm>
#include <execution>
#include <unordered_set>

namespace ecscope::physics {

// Advanced constraint solver implementation with warm starting
void ConstraintSolver::solve_velocity_constraints(std::vector<ContactManifold>& manifolds,
                                                 std::vector<RigidBody3D*>& bodies,
                                                 Real time_step,
                                                 uint32_t iterations) {
    
    // Build body lookup for faster access
    std::unordered_map<uint32_t, RigidBody3D*> body_lookup;
    for (auto* body : bodies) {
        body_lookup[body->id] = body;
    }
    
    // Sequential impulse method
    for (uint32_t iter = 0; iter < iterations; ++iter) {
        for (auto& manifold : manifolds) {
            RigidBody3D* body_a = body_lookup[manifold.body_a_id];
            RigidBody3D* body_b = body_lookup[manifold.body_b_id];
            
            if (!body_a || !body_b) continue;
            
            // Skip if both bodies are static or kinematic
            if ((body_a->type == BodyType::Static || body_a->type == BodyType::Kinematic) &&
                (body_b->type == BodyType::Static || body_b->type == BodyType::Kinematic)) {
                continue;
            }
            
            solve_manifold_constraints(*body_a, *body_b, manifold, time_step);
        }
    }
}

void ConstraintSolver::solve_manifold_constraints(RigidBody3D& body_a, RigidBody3D& body_b,
                                                 ContactManifold& manifold, Real time_step) {
    for (auto& contact : manifold.contacts) {
        // Calculate relative contact positions
        Vec3 r_a = contact.world_position_a - body_a.transform.position;
        Vec3 r_b = contact.world_position_b - body_b.transform.position;
        
        // Calculate relative velocity at contact point
        Vec3 v_a = body_a.velocity + body_a.angular_velocity.cross(r_a);
        Vec3 v_b = body_b.velocity + body_b.angular_velocity.cross(r_b);
        Vec3 relative_velocity = v_a - v_b;
        
        // Solve normal constraint
        solve_normal_constraint(body_a, body_b, contact, manifold.normal, 
                               manifold.restitution, r_a, r_b, relative_velocity);
        
        // Solve friction constraints
        solve_friction_constraint(body_a, body_b, contact, manifold.normal,
                                 manifold.friction, r_a, r_b, relative_velocity);
    }
}

void ConstraintSolver::solve_normal_constraint(RigidBody3D& body_a, RigidBody3D& body_b,
                                              ContactPoint& contact, const Vec3& normal,
                                              Real restitution, const Vec3& r_a, const Vec3& r_b,
                                              const Vec3& relative_velocity) {
    
    Real relative_velocity_normal = relative_velocity.dot(normal);
    
    // Don't resolve if velocities are separating
    if (relative_velocity_normal > 0) return;
    
    // Calculate effective mass for the constraint
    Vec3 r_a_cross_n = r_a.cross(normal);
    Vec3 r_b_cross_n = r_b.cross(normal);
    
    Real inv_mass_sum = body_a.inverse_mass + body_b.inverse_mass;
    Real inv_inertia_sum = r_a_cross_n.dot(body_a.inverse_inertia_tensor * r_a_cross_n) +
                          r_b_cross_n.dot(body_b.inverse_inertia_tensor * r_b_cross_n);
    
    Real effective_mass = inv_mass_sum + inv_inertia_sum;
    if (effective_mass < PHYSICS_EPSILON) return;
    
    // Calculate impulse magnitude with restitution
    Real impulse_magnitude = -(1.0f + restitution) * relative_velocity_normal / effective_mass;
    
    // Clamp to prevent negative separation
    Real old_impulse = contact.normal_impulse;
    contact.normal_impulse = std::max(0.0f, old_impulse + impulse_magnitude);
    Real delta_impulse = contact.normal_impulse - old_impulse;
    
    // Apply impulse
    Vec3 impulse = normal * delta_impulse;
    
    if (body_a.type == BodyType::Dynamic) {
        body_a.velocity += impulse * body_a.inverse_mass;
        body_a.angular_velocity += body_a.inverse_inertia_tensor * r_a.cross(impulse);
    }
    
    if (body_b.type == BodyType::Dynamic) {
        body_b.velocity -= impulse * body_b.inverse_mass;
        body_b.angular_velocity -= body_b.inverse_inertia_tensor * r_b.cross(impulse);
    }
}

void ConstraintSolver::solve_friction_constraint(RigidBody3D& body_a, RigidBody3D& body_b,
                                                ContactPoint& contact, const Vec3& normal,
                                                Real friction, const Vec3& r_a, const Vec3& r_b,
                                                const Vec3& relative_velocity) {
    
    // Calculate tangent velocity
    Vec3 tangent_velocity = relative_velocity - normal * relative_velocity.dot(normal);
    Real tangent_speed = tangent_velocity.length();
    
    if (tangent_speed < PHYSICS_EPSILON) return;
    
    Vec3 tangent = tangent_velocity / tangent_speed;
    
    // Calculate effective mass for friction constraint
    Vec3 r_a_cross_t = r_a.cross(tangent);
    Vec3 r_b_cross_t = r_b.cross(tangent);
    
    Real inv_mass_sum = body_a.inverse_mass + body_b.inverse_mass;
    Real inv_inertia_sum = r_a_cross_t.dot(body_a.inverse_inertia_tensor * r_a_cross_t) +
                          r_b_cross_t.dot(body_b.inverse_inertia_tensor * r_b_cross_t);
    
    Real effective_mass = inv_mass_sum + inv_inertia_sum;
    if (effective_mass < PHYSICS_EPSILON) return;
    
    // Calculate friction impulse
    Real friction_impulse = -tangent_speed / effective_mass;
    
    // Coulomb friction constraint
    Real max_friction = friction * contact.normal_impulse;
    Real old_tangent_impulse = contact.tangent_impulse;
    contact.tangent_impulse = std::clamp(old_tangent_impulse + friction_impulse,
                                       -max_friction, max_friction);
    Real delta_friction_impulse = contact.tangent_impulse - old_tangent_impulse;
    
    // Apply friction impulse
    Vec3 friction_force = tangent * delta_friction_impulse;
    
    if (body_a.type == BodyType::Dynamic) {
        body_a.velocity += friction_force * body_a.inverse_mass;
        body_a.angular_velocity += body_a.inverse_inertia_tensor * r_a.cross(friction_force);
    }
    
    if (body_b.type == BodyType::Dynamic) {
        body_b.velocity -= friction_force * body_b.inverse_mass;
        body_b.angular_velocity -= body_b.inverse_inertia_tensor * r_b.cross(friction_force);
    }
}

void ConstraintSolver::solve_position_constraints(std::vector<ContactManifold>& manifolds,
                                                 std::vector<RigidBody3D*>& bodies,
                                                 Real time_step,
                                                 uint32_t iterations) {
    
    std::unordered_map<uint32_t, RigidBody3D*> body_lookup;
    for (auto* body : bodies) {
        body_lookup[body->id] = body;
    }
    
    const Real position_correction_factor = 0.2f; // Baumgarte stabilization factor
    const Real position_slop = 0.005f; // 5mm slop tolerance
    
    for (uint32_t iter = 0; iter < iterations; ++iter) {
        for (auto& manifold : manifolds) {
            RigidBody3D* body_a = body_lookup[manifold.body_a_id];
            RigidBody3D* body_b = body_lookup[manifold.body_b_id];
            
            if (!body_a || !body_b) continue;
            
            for (auto& contact : manifold.contacts) {
                if (contact.penetration <= position_slop) continue;
                
                Vec3 r_a = contact.world_position_a - body_a->transform.position;
                Vec3 r_b = contact.world_position_b - body_b->transform.position;
                
                Vec3 normal = manifold.normal;
                
                // Calculate effective mass for position constraint
                Vec3 r_a_cross_n = r_a.cross(normal);
                Vec3 r_b_cross_n = r_b.cross(normal);
                
                Real inv_mass_sum = body_a->inverse_mass + body_b->inverse_mass;
                Real inv_inertia_sum = r_a_cross_n.dot(body_a->inverse_inertia_tensor * r_a_cross_n) +
                                      r_b_cross_n.dot(body_b->inverse_inertia_tensor * r_b_cross_n);
                
                Real effective_mass = inv_mass_sum + inv_inertia_sum;
                if (effective_mass < PHYSICS_EPSILON) continue;
                
                // Calculate position correction impulse
                Real correction = -position_correction_factor * 
                                 std::max(0.0f, contact.penetration - position_slop) / effective_mass;
                
                Vec3 correction_impulse = normal * correction;
                
                if (body_a->type == BodyType::Dynamic) {
                    body_a->transform.position += correction_impulse * body_a->inverse_mass;
                    
                    // Apply angular position correction
                    Vec3 angular_correction = body_a->inverse_inertia_tensor * r_a.cross(correction_impulse);
                    Quaternion rotation_correction = Quaternion::from_axis_angle(angular_correction, angular_correction.length());
                    body_a->transform.rotation = body_a->transform.rotation * rotation_correction;
                    body_a->transform.rotation = body_a->transform.rotation.normalized();
                }
                
                if (body_b->type == BodyType::Dynamic) {
                    body_b->transform.position -= correction_impulse * body_b->inverse_mass;
                    
                    Vec3 angular_correction = body_b->inverse_inertia_tensor * r_b.cross(correction_impulse);
                    Quaternion rotation_correction = Quaternion::from_axis_angle(angular_correction, -angular_correction.length());
                    body_b->transform.rotation = body_b->transform.rotation * rotation_correction;
                    body_b->transform.rotation = body_b->transform.rotation.normalized();
                }
            }
        }
    }
}

// Distance constraint implementation
void DistanceConstraint::solve_constraint(RigidBody3D& body_a, RigidBody3D& body_b, Real time_step) {
    Vec3 r_a = local_anchor_a;
    Vec3 r_b = local_anchor_b;
    
    // Transform to world space
    Vec3 world_anchor_a = body_a.transform.position + body_a.transform.rotation.rotate_vector(r_a);
    Vec3 world_anchor_b = body_b.transform.position + body_b.transform.rotation.rotate_vector(r_b);
    
    Vec3 separation = world_anchor_b - world_anchor_a;
    Real current_distance = separation.length();
    
    if (current_distance < PHYSICS_EPSILON) return;
    
    Vec3 direction = separation / current_distance;
    Real constraint_error = current_distance - rest_length;
    
    // Calculate effective mass
    Vec3 r_a_cross_dir = r_a.cross(direction);
    Vec3 r_b_cross_dir = r_b.cross(direction);
    
    Real inv_mass_sum = body_a.inverse_mass + body_b.inverse_mass;
    Real inv_inertia_sum = r_a_cross_dir.dot(body_a.inverse_inertia_tensor * r_a_cross_dir) +
                          r_b_cross_dir.dot(body_b.inverse_inertia_tensor * r_b_cross_dir);
    
    Real effective_mass = inv_mass_sum + inv_inertia_sum;
    if (effective_mass < PHYSICS_EPSILON) return;
    
    // Calculate relative velocity along constraint direction
    Vec3 v_a = body_a.velocity + body_a.angular_velocity.cross(r_a);
    Vec3 v_b = body_b.velocity + body_b.angular_velocity.cross(r_b);
    Real relative_velocity = (v_b - v_a).dot(direction);
    
    // Calculate impulse with damping
    Real bias = -0.1f * constraint_error / time_step; // Position correction
    Real impulse_magnitude = -(relative_velocity + bias) / effective_mass;
    
    Vec3 impulse = direction * impulse_magnitude;
    
    if (body_a.type == BodyType::Dynamic) {
        body_a.velocity -= impulse * body_a.inverse_mass;
        body_a.angular_velocity -= body_a.inverse_inertia_tensor * r_a.cross(impulse);
    }
    
    if (body_b.type == BodyType::Dynamic) {
        body_b.velocity += impulse * body_b.inverse_mass;
        body_b.angular_velocity += body_b.inverse_inertia_tensor * r_b.cross(impulse);
    }
}

// Hinge joint constraint implementation  
void HingeConstraint::solve_constraint(RigidBody3D& body_a, RigidBody3D& body_b, Real time_step) {
    // Transform anchors and axes to world space
    Vec3 world_anchor_a = body_a.transform.position + body_a.transform.rotation.rotate_vector(local_anchor_a);
    Vec3 world_anchor_b = body_b.transform.position + body_b.transform.rotation.rotate_vector(local_anchor_b);
    Vec3 world_axis_a = body_a.transform.rotation.rotate_vector(local_axis_a);
    Vec3 world_axis_b = body_b.transform.rotation.rotate_vector(local_axis_b);
    
    // Position constraint: anchors should coincide
    Vec3 position_error = world_anchor_b - world_anchor_a;
    
    if (position_error.length() > PHYSICS_EPSILON) {
        solve_position_constraint(body_a, body_b, position_error, local_anchor_a, local_anchor_b, time_step);
    }
    
    // Orientation constraint: axes should be aligned
    Vec3 axis_error = world_axis_a.cross(world_axis_b);
    
    if (axis_error.length() > PHYSICS_EPSILON) {
        solve_orientation_constraint(body_a, body_b, axis_error, time_step);
    }
    
    // Angular limits
    if (enable_limits) {
        solve_angular_limits(body_a, body_b, world_axis_a, world_axis_b, time_step);
    }
}

void HingeConstraint::solve_position_constraint(RigidBody3D& body_a, RigidBody3D& body_b,
                                               const Vec3& position_error, const Vec3& r_a, const Vec3& r_b,
                                               Real time_step) {
    for (int i = 0; i < 3; ++i) {
        Vec3 axis = (i == 0) ? Vec3::unit_x() : (i == 1) ? Vec3::unit_y() : Vec3::unit_z();
        Real error = position_error.dot(axis);
        
        if (std::abs(error) < PHYSICS_EPSILON) continue;
        
        // Calculate effective mass for this axis
        Vec3 r_a_cross_axis = r_a.cross(axis);
        Vec3 r_b_cross_axis = r_b.cross(axis);
        
        Real inv_mass_sum = body_a.inverse_mass + body_b.inverse_mass;
        Real inv_inertia_sum = r_a_cross_axis.dot(body_a.inverse_inertia_tensor * r_a_cross_axis) +
                              r_b_cross_axis.dot(body_b.inverse_inertia_tensor * r_b_cross_axis);
        
        Real effective_mass = inv_mass_sum + inv_inertia_sum;
        if (effective_mass < PHYSICS_EPSILON) continue;
        
        // Calculate relative velocity
        Vec3 v_a = body_a.velocity + body_a.angular_velocity.cross(r_a);
        Vec3 v_b = body_b.velocity + body_b.angular_velocity.cross(r_b);
        Real relative_velocity = (v_b - v_a).dot(axis);
        
        // Calculate impulse
        Real bias = -0.2f * error / time_step;
        Real impulse_magnitude = -(relative_velocity + bias) / effective_mass;
        
        Vec3 impulse = axis * impulse_magnitude;
        
        if (body_a.type == BodyType::Dynamic) {
            body_a.velocity -= impulse * body_a.inverse_mass;
            body_a.angular_velocity -= body_a.inverse_inertia_tensor * r_a.cross(impulse);
        }
        
        if (body_b.type == BodyType::Dynamic) {
            body_b.velocity += impulse * body_b.inverse_mass;
            body_b.angular_velocity += body_b.inverse_inertia_tensor * r_b.cross(impulse);
        }
    }
}

void HingeConstraint::solve_orientation_constraint(RigidBody3D& body_a, RigidBody3D& body_b,
                                                  const Vec3& axis_error, Real time_step) {
    // Constrain rotation to only allow rotation around hinge axis
    Vec3 world_hinge_axis = body_a.transform.rotation.rotate_vector(local_axis_a);
    
    // Project error onto plane perpendicular to hinge axis
    Vec3 constrained_error = axis_error - world_hinge_axis * axis_error.dot(world_hinge_axis);
    
    if (constrained_error.length() < PHYSICS_EPSILON) return;
    
    Vec3 constraint_axis = constrained_error.normalized();
    Real error_magnitude = constrained_error.length();
    
    // Calculate effective angular mass
    Real inv_inertia_sum = constraint_axis.dot(body_a.inverse_inertia_tensor * constraint_axis) +
                          constraint_axis.dot(body_b.inverse_inertia_tensor * constraint_axis);
    
    if (inv_inertia_sum < PHYSICS_EPSILON) return;
    
    // Calculate relative angular velocity
    Real relative_angular_velocity = (body_b.angular_velocity - body_a.angular_velocity).dot(constraint_axis);
    
    // Calculate impulse
    Real bias = -0.2f * error_magnitude / time_step;
    Real impulse_magnitude = -(relative_angular_velocity + bias) / inv_inertia_sum;
    
    Vec3 angular_impulse = constraint_axis * impulse_magnitude;
    
    if (body_a.type == BodyType::Dynamic) {
        body_a.angular_velocity -= body_a.inverse_inertia_tensor * angular_impulse;
    }
    
    if (body_b.type == BodyType::Dynamic) {
        body_b.angular_velocity += body_b.inverse_inertia_tensor * angular_impulse;
    }
}

} // namespace ecscope::physics