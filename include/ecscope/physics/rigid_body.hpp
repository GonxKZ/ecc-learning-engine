#pragma once

#include "physics_math.hpp"
#include <memory>
#include <limits>

namespace ecscope::physics {

enum class BodyType {
    Static,    // Infinite mass, doesn't move
    Kinematic, // Infinite mass, controlled by user
    Dynamic    // Finite mass, affected by forces
};

// Material properties for physics bodies
struct Material {
    Real density = 1.0f;
    Real friction = 0.3f;
    Real restitution = 0.2f;
    Real linear_damping = 0.01f;
    Real angular_damping = 0.01f;
    
    Material() = default;
    Material(Real density, Real friction, Real restitution) 
        : density(density), friction(friction), restitution(restitution) {}
};

// Mass properties for rigid bodies
struct MassProperties {
    Real mass = 1.0f;
    Real inverse_mass = 1.0f;
    Mat3 inertia = Mat3::identity();
    Mat3 inverse_inertia = Mat3::identity();
    Vec3 center_of_mass = Vec3::zero();
    
    MassProperties() = default;
    
    void set_mass(Real m) {
        if (m > PHYSICS_EPSILON) {
            mass = m;
            inverse_mass = 1.0f / m;
        } else {
            mass = 0.0f;
            inverse_mass = 0.0f;
        }
    }
    
    void set_infinite_mass() {
        mass = std::numeric_limits<Real>::max();
        inverse_mass = 0.0f;
        inertia = Mat3::identity();
        inverse_inertia = Mat3();  // Zero matrix
    }
    
    static MassProperties for_box(Real width, Real height, Real depth, Real density) {
        MassProperties props;
        Real volume = width * height * depth;
        props.set_mass(volume * density);
        
        // Inertia tensor for box
        Real w2 = width * width, h2 = height * height, d2 = depth * depth;
        props.inertia = Mat3::identity();
        props.inertia(0, 0) = props.mass * (h2 + d2) / 12.0f;
        props.inertia(1, 1) = props.mass * (w2 + d2) / 12.0f;
        props.inertia(2, 2) = props.mass * (w2 + h2) / 12.0f;
        props.inverse_inertia = props.inertia.inverse();
        
        return props;
    }
    
    static MassProperties for_sphere(Real radius, Real density) {
        MassProperties props;
        Real volume = (4.0f / 3.0f) * PI * radius * radius * radius;
        props.set_mass(volume * density);
        
        // Inertia tensor for sphere
        Real inertia_value = 0.4f * props.mass * radius * radius;
        props.inertia = Mat3::identity();
        props.inertia(0, 0) = props.inertia(1, 1) = props.inertia(2, 2) = inertia_value;
        props.inverse_inertia = props.inertia.inverse();
        
        return props;
    }
    
    static MassProperties for_circle(Real radius, Real density) {
        MassProperties props;
        Real area = PI * radius * radius;
        props.set_mass(area * density);
        
        // Moment of inertia for circle (2D)
        Real inertia_value = 0.5f * props.mass * radius * radius;
        props.inertia = Mat3::identity();
        props.inertia(2, 2) = inertia_value;  // Only z-axis rotation matters in 2D
        props.inverse_inertia = props.inertia.inverse();
        
        return props;
    }
};

// 2D Rigid Body
class RigidBody2D {
public:
    // Identity and type
    uint32_t id = 0;
    BodyType type = BodyType::Dynamic;
    
    // Transform
    Transform2D transform;
    
    // Linear motion
    Vec2 velocity = Vec2::zero();
    Vec2 force = Vec2::zero();
    Real mass = 1.0f;
    Real inverse_mass = 1.0f;
    
    // Angular motion
    Real angular_velocity = 0.0f;
    Real torque = 0.0f;
    Real moment_of_inertia = 1.0f;
    Real inverse_moment_of_inertia = 1.0f;
    
    // Material properties
    Material material;
    
    // State flags
    bool is_sleeping = false;
    bool allow_sleep = true;
    Real sleep_threshold = 0.01f;
    Real sleep_time = 0.0f;
    
    // User data
    void* user_data = nullptr;
    
    RigidBody2D() = default;
    
    explicit RigidBody2D(BodyType body_type) : type(body_type) {
        if (body_type != BodyType::Dynamic) {
            set_infinite_mass();
        }
    }
    
    void set_mass(Real m) {
        if (type == BodyType::Dynamic && m > PHYSICS_EPSILON) {
            mass = m;
            inverse_mass = 1.0f / m;
        } else {
            set_infinite_mass();
        }
    }
    
    void set_infinite_mass() {
        mass = std::numeric_limits<Real>::max();
        inverse_mass = 0.0f;
        moment_of_inertia = std::numeric_limits<Real>::max();
        inverse_moment_of_inertia = 0.0f;
    }
    
    void set_moment_of_inertia(Real moi) {
        if (type == BodyType::Dynamic && moi > PHYSICS_EPSILON) {
            moment_of_inertia = moi;
            inverse_moment_of_inertia = 1.0f / moi;
        } else {
            moment_of_inertia = std::numeric_limits<Real>::max();
            inverse_moment_of_inertia = 0.0f;
        }
    }
    
    // Apply forces
    void apply_force(const Vec2& f) {
        if (type == BodyType::Dynamic) {
            force += f;
            wake_up();
        }
    }
    
    void apply_force_at_point(const Vec2& f, const Vec2& world_point) {
        if (type == BodyType::Dynamic) {
            force += f;
            Vec2 r = world_point - transform.position;
            torque += r.cross(f);
            wake_up();
        }
    }
    
    void apply_impulse(const Vec2& impulse) {
        if (type == BodyType::Dynamic) {
            velocity += impulse * inverse_mass;
            wake_up();
        }
    }
    
    void apply_impulse_at_point(const Vec2& impulse, const Vec2& world_point) {
        if (type == BodyType::Dynamic) {
            velocity += impulse * inverse_mass;
            Vec2 r = world_point - transform.position;
            angular_velocity += r.cross(impulse) * inverse_moment_of_inertia;
            wake_up();
        }
    }
    
    void apply_torque(Real t) {
        if (type == BodyType::Dynamic) {
            torque += t;
            wake_up();
        }
    }
    
    void apply_angular_impulse(Real impulse) {
        if (type == BodyType::Dynamic) {
            angular_velocity += impulse * inverse_moment_of_inertia;
            wake_up();
        }
    }
    
    // Sleep management
    void wake_up() {
        is_sleeping = false;
        sleep_time = 0.0f;
    }
    
    void put_to_sleep() {
        is_sleeping = true;
        velocity = Vec2::zero();
        angular_velocity = 0.0f;
        force = Vec2::zero();
        torque = 0.0f;
    }
    
    bool can_sleep() const {
        if (!allow_sleep || type != BodyType::Dynamic) return false;
        
        Real kinetic_energy = 0.5f * mass * velocity.length_squared() + 
                             0.5f * moment_of_inertia * angular_velocity * angular_velocity;
        return kinetic_energy < sleep_threshold;
    }
    
    // Integration
    void integrate_forces(Real dt) {
        if (type != BodyType::Dynamic || is_sleeping) return;
        
        // Apply gravity and other constant forces here
        Vec2 acceleration = force * inverse_mass;
        Real angular_acceleration = torque * inverse_moment_of_inertia;
        
        velocity += acceleration * dt;
        angular_velocity += angular_acceleration * dt;
        
        // Apply damping
        velocity *= std::pow(1.0f - material.linear_damping, dt);
        angular_velocity *= std::pow(1.0f - material.angular_damping, dt);
    }
    
    void integrate_velocity(Real dt) {
        if (type == BodyType::Static || is_sleeping) return;
        
        transform.position += velocity * dt;
        transform.rotation += angular_velocity * dt;
        
        // Clear forces
        force = Vec2::zero();
        torque = 0.0f;
    }
    
    // Utility methods
    Vec2 get_velocity_at_point(const Vec2& world_point) const {
        Vec2 r = world_point - transform.position;
        return velocity + Vec2(-r.y * angular_velocity, r.x * angular_velocity);
    }
    
    Real get_kinetic_energy() const {
        return 0.5f * mass * velocity.length_squared() + 
               0.5f * moment_of_inertia * angular_velocity * angular_velocity;
    }
};

// 3D Rigid Body
class RigidBody3D {
public:
    // Identity and type
    uint32_t id = 0;
    BodyType type = BodyType::Dynamic;
    
    // Transform
    Transform3D transform;
    
    // Linear motion
    Vec3 velocity = Vec3::zero();
    Vec3 force = Vec3::zero();
    
    // Angular motion
    Vec3 angular_velocity = Vec3::zero();
    Vec3 torque = Vec3::zero();
    
    // Mass properties
    MassProperties mass_props;
    
    // Material properties
    Material material;
    
    // State flags
    bool is_sleeping = false;
    bool allow_sleep = true;
    Real sleep_threshold = 0.01f;
    Real sleep_time = 0.0f;
    
    // User data
    void* user_data = nullptr;
    
    RigidBody3D() = default;
    
    explicit RigidBody3D(BodyType body_type) : type(body_type) {
        if (body_type != BodyType::Dynamic) {
            mass_props.set_infinite_mass();
        }
    }
    
    void set_mass_properties(const MassProperties& props) {
        if (type == BodyType::Dynamic) {
            mass_props = props;
        } else {
            mass_props.set_infinite_mass();
        }
    }
    
    // Apply forces
    void apply_force(const Vec3& f) {
        if (type == BodyType::Dynamic) {
            force += f;
            wake_up();
        }
    }
    
    void apply_force_at_point(const Vec3& f, const Vec3& world_point) {
        if (type == BodyType::Dynamic) {
            force += f;
            Vec3 r = world_point - transform.position;
            torque += r.cross(f);
            wake_up();
        }
    }
    
    void apply_impulse(const Vec3& impulse) {
        if (type == BodyType::Dynamic) {
            velocity += impulse * mass_props.inverse_mass;
            wake_up();
        }
    }
    
    void apply_impulse_at_point(const Vec3& impulse, const Vec3& world_point) {
        if (type == BodyType::Dynamic) {
            velocity += impulse * mass_props.inverse_mass;
            Vec3 r = world_point - transform.position;
            Mat3 world_inv_inertia = get_world_inverse_inertia();
            angular_velocity += world_inv_inertia * r.cross(impulse);
            wake_up();
        }
    }
    
    void apply_torque(const Vec3& t) {
        if (type == BodyType::Dynamic) {
            torque += t;
            wake_up();
        }
    }
    
    void apply_angular_impulse(const Vec3& impulse) {
        if (type == BodyType::Dynamic) {
            Mat3 world_inv_inertia = get_world_inverse_inertia();
            angular_velocity += world_inv_inertia * impulse;
            wake_up();
        }
    }
    
    // Sleep management
    void wake_up() {
        is_sleeping = false;
        sleep_time = 0.0f;
    }
    
    void put_to_sleep() {
        is_sleeping = true;
        velocity = Vec3::zero();
        angular_velocity = Vec3::zero();
        force = Vec3::zero();
        torque = Vec3::zero();
    }
    
    bool can_sleep() const {
        if (!allow_sleep || type != BodyType::Dynamic) return false;
        
        Real linear_ke = 0.5f * mass_props.mass * velocity.length_squared();
        Real angular_ke = 0.5f * angular_velocity.dot(get_world_inertia() * angular_velocity);
        return (linear_ke + angular_ke) < sleep_threshold;
    }
    
    // Integration
    void integrate_forces(Real dt) {
        if (type != BodyType::Dynamic || is_sleeping) return;
        
        Vec3 acceleration = force * mass_props.inverse_mass;
        Mat3 world_inv_inertia = get_world_inverse_inertia();
        Vec3 angular_acceleration = world_inv_inertia * torque;
        
        velocity += acceleration * dt;
        angular_velocity += angular_acceleration * dt;
        
        // Apply damping
        velocity *= std::pow(1.0f - material.linear_damping, dt);
        angular_velocity *= std::pow(1.0f - material.angular_damping, dt);
    }
    
    void integrate_velocity(Real dt) {
        if (type == BodyType::Static || is_sleeping) return;
        
        transform.position += velocity * dt;
        
        // Update rotation using angular velocity
        if (angular_velocity.length_squared() > PHYSICS_EPSILON) {
            Real angle = angular_velocity.length() * dt;
            Vec3 axis = angular_velocity.normalized();
            Quaternion rotation_delta = Quaternion::from_axis_angle(axis, angle);
            transform.rotation = (rotation_delta * transform.rotation).normalized();
        }
        
        // Clear forces
        force = Vec3::zero();
        torque = Vec3::zero();
    }
    
    // Utility methods
    Mat3 get_world_inertia() const {
        Mat3 rotation_matrix = transform.rotation_matrix();
        return rotation_matrix * mass_props.inertia * rotation_matrix.transposed();
    }
    
    Mat3 get_world_inverse_inertia() const {
        Mat3 rotation_matrix = transform.rotation_matrix();
        return rotation_matrix * mass_props.inverse_inertia * rotation_matrix.transposed();
    }
    
    Vec3 get_velocity_at_point(const Vec3& world_point) const {
        Vec3 r = world_point - transform.position;
        return velocity + angular_velocity.cross(r);
    }
    
    Real get_kinetic_energy() const {
        Real linear_ke = 0.5f * mass_props.mass * velocity.length_squared();
        Real angular_ke = 0.5f * angular_velocity.dot(get_world_inertia() * angular_velocity);
        return linear_ke + angular_ke;
    }
};

} // namespace ecscope::physics