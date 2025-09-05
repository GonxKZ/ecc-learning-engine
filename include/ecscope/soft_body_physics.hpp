#pragma once

/**
 * @file soft_body_physics.hpp
 * @brief Advanced Soft Body Physics System for ECScope - Educational Implementation
 * 
 * This header implements a comprehensive soft body physics system using mass-spring
 * networks and Finite Element Methods (FEM). The system is designed for both
 * educational value and high performance, demonstrating advanced physics concepts
 * while maintaining 60+ FPS with hundreds of soft bodies.
 * 
 * Key Features:
 * - Mass-spring networks for cloth and deformable objects
 * - Position-based dynamics for stable simulation
 * - Finite Element Method for advanced materials
 * - Educational visualization of forces and deformations
 * - Integration with existing rigid body physics
 * - Cloth simulation with self-collision
 * - Fluid-soft body interaction
 * 
 * Educational Goals:
 * - Demonstrate continuum mechanics principles
 * - Show the relationship between discrete and continuous models
 * - Visualize stress, strain, and material properties
 * - Compare different integration methods and their stability
 * - Illustrate performance optimization techniques
 * 
 * Performance Targets:
 * - 500+ soft body particles at 60 FPS
 * - Stable simulation with large time steps
 * - Efficient collision detection and response
 * - Memory-efficient particle storage
 * - SIMD-optimized force calculations
 * 
 * @author ECScope Educational Physics Engine
 * @date 2024
 */

#include "physics.hpp"
#include "math3d.hpp"
#include "../ecs/component.hpp"
#include "../memory/pool.hpp"
#include "../core/simd_math.hpp"
#include <vector>
#include <array>
#include <memory>
#include <unordered_map>
#include <functional>

namespace ecscope::physics::soft_body {

// Import necessary types
using namespace ecscope::physics::math;
using namespace ecscope::ecs;

//=============================================================================
// Soft Body Material Properties
//=============================================================================

/**
 * @brief Advanced Material Properties for Soft Bodies
 * 
 * Extends the basic PhysicsMaterial with properties specific to deformable
 * objects. Includes educational information about material science concepts.
 */
struct alignas(32) SoftBodyMaterial {
    //-------------------------------------------------------------------------
    // Mechanical Properties
    //-------------------------------------------------------------------------
    
    /**
     * @brief Young's modulus (elastic modulus) in Pa
     * 
     * Measures stiffness of material - resistance to elastic deformation.
     * Higher values = stiffer material.
     * 
     * Educational Context:
     * E = stress / strain = (F/A) / (ΔL/L)
     * 
     * Typical Values:
     * - Rubber: 1-10 MPa
     * - Muscle tissue: 10-500 kPa  
     * - Skin: 100-200 kPa
     * - Steel: 200 GPa
     * - Diamond: 1220 GPa
     */
    f32 youngs_modulus{1e6f}; // 1 MPa default (rubber-like)
    
    /**
     * @brief Poisson's ratio (dimensionless)
     * 
     * Ratio of transverse strain to axial strain. Describes how material
     * contracts in directions perpendicular to applied load.
     * 
     * Range: -1.0 to 0.5 (most materials: 0.0 to 0.5)
     * - 0.0: No lateral contraction
     * - 0.5: Incompressible (like rubber, liquids)
     * - 0.3: Steel, aluminum
     * 
     * Educational Note:
     * ν = -ε_transverse / ε_axial
     */
    f32 poissons_ratio{0.4f};
    
    /**
     * @brief Shear modulus in Pa
     * 
     * Resistance to shear deformation. Related to Young's modulus:
     * G = E / (2 * (1 + ν))
     * 
     * Educational Context:
     * Shear strain occurs when layers of material slide past each other.
     */
    f32 shear_modulus{3.57e5f}; // Calculated from E and ν
    
    /**
     * @brief Bulk modulus in Pa
     * 
     * Resistance to uniform compression. Related to other moduli:
     * K = E / (3 * (1 - 2ν))
     * 
     * Educational Context:
     * Bulk modulus measures volumetric elasticity - how much volume
     * changes under pressure.
     */
    f32 bulk_modulus{1.67e6f}; // Calculated from E and ν
    
    //-------------------------------------------------------------------------
    // Damping Properties
    //-------------------------------------------------------------------------
    
    /**
     * @brief Structural damping coefficient
     * 
     * Damping applied along spring connections (stretch/compression).
     * Higher values reduce oscillations but may cause energy loss.
     */
    f32 structural_damping{0.1f};
    
    /**
     * @brief Shear damping coefficient
     * 
     * Damping applied to shear deformations.
     * Important for cloth simulation to prevent unrealistic fluttering.
     */
    f32 shear_damping{0.05f};
    
    /**
     * @brief Bend damping coefficient
     * 
     * Damping applied to bending forces.
     * Controls how quickly bending oscillations decay.
     */
    f32 bend_damping{0.02f};
    
    /**
     * @brief Global velocity damping
     * 
     * General velocity damping to simulate air resistance and
     * internal friction. Applied to all particle velocities.
     */
    f32 global_damping{0.01f};
    
    //-------------------------------------------------------------------------
    // Simulation Parameters
    //-------------------------------------------------------------------------
    
    /**
     * @brief Particle density in kg/m³
     * 
     * Used to calculate mass of individual particles based on
     * their associated volume in the mesh.
     */
    f32 density{1000.0f};
    
    /**
     * @brief Thickness for 2D soft bodies
     * 
     * Since we're simulating 3D materials in 2D space, this represents
     * the assumed thickness for volume and mass calculations.
     */
    f32 thickness{0.01f}; // 1cm default thickness
    
    /**
     * @brief Minimum constraint distance
     * 
     * Prevents particles from getting too close and causing
     * numerical instabilities.
     */
    f32 min_distance{0.001f};
    
    /**
     * @brief Maximum stretch ratio before breakage
     * 
     * When springs stretch beyond this ratio of their rest length,
     * they break. Set to 0 for unbreakable springs.
     */
    f32 max_stretch_ratio{2.0f};
    
    //-------------------------------------------------------------------------
    // Material Behavior Flags
    //-------------------------------------------------------------------------
    
    union {
        u32 flags;
        struct {
            u32 enable_plasticity : 1;      ///< Allow permanent deformation
            u32 enable_fracture : 1;        ///< Allow material to break
            u32 enable_self_collision : 1;  ///< Self-collision detection
            u32 incompressible : 1;         ///< Maintain constant volume
            u32 anisotropic : 1;            ///< Different properties in different directions
            u32 viscoelastic : 1;           ///< Time-dependent deformation
            u32 temperature_dependent : 1;  ///< Properties vary with temperature
            u32 reserved : 25;
        };
    } material_flags{0};
    
    //-------------------------------------------------------------------------
    // Advanced Properties
    //-------------------------------------------------------------------------
    
    /**
     * @brief Yield strength for plastic deformation (Pa)
     * 
     * Stress level at which material begins permanent deformation.
     * Only used if enable_plasticity is true.
     */
    f32 yield_strength{1e6f};
    
    /**
     * @brief Fracture stress (Pa)
     * 
     * Stress level at which material breaks completely.
     * Only used if enable_fracture is true.
     */
    f32 fracture_stress{2e6f};
    
    /**
     * @brief Temperature coefficient
     * 
     * How much properties change with temperature.
     * Only used if temperature_dependent is true.
     */
    f32 temperature_coefficient{0.001f};
    
    /**
     * @brief Current temperature (Kelvin)
     */
    f32 current_temperature{293.15f}; // Room temperature
    
    //-------------------------------------------------------------------------
    // Factory Methods
    //-------------------------------------------------------------------------
    
    /** @brief Create cloth-like material */
    static SoftBodyMaterial create_cloth() noexcept {
        SoftBodyMaterial mat;
        mat.youngs_modulus = 5e5f;      // 0.5 MPa
        mat.poissons_ratio = 0.3f;
        mat.structural_damping = 0.15f;
        mat.shear_damping = 0.1f;
        mat.bend_damping = 0.05f;
        mat.density = 300.0f;           // Typical fabric density
        mat.thickness = 0.001f;         // 1mm thick
        mat.material_flags.enable_self_collision = 1;
        mat.update_derived_properties();
        return mat;
    }
    
    /** @brief Create rubber-like material */
    static SoftBodyMaterial create_rubber() noexcept {
        SoftBodyMaterial mat;
        mat.youngs_modulus = 2e6f;      // 2 MPa
        mat.poissons_ratio = 0.49f;     // Nearly incompressible
        mat.structural_damping = 0.08f;
        mat.density = 920.0f;           // Rubber density
        mat.thickness = 0.01f;          // 1cm thick
        mat.material_flags.incompressible = 1;
        mat.update_derived_properties();
        return mat;
    }
    
    /** @brief Create jelly/gel-like material */
    static SoftBodyMaterial create_jelly() noexcept {
        SoftBodyMaterial mat;
        mat.youngs_modulus = 1e4f;      // 10 kPa
        mat.poissons_ratio = 0.45f;
        mat.structural_damping = 0.2f;
        mat.global_damping = 0.05f;
        mat.density = 1050.0f;          // Similar to water
        mat.material_flags.incompressible = 1;
        mat.material_flags.viscoelastic = 1;
        mat.update_derived_properties();
        return mat;
    }
    
    /** @brief Create muscle tissue-like material */
    static SoftBodyMaterial create_muscle_tissue() noexcept {
        SoftBodyMaterial mat;
        mat.youngs_modulus = 5e4f;      // 50 kPa
        mat.poissons_ratio = 0.45f;
        mat.structural_damping = 0.12f;
        mat.density = 1060.0f;          // Muscle density
        mat.material_flags.anisotropic = 1; // Different properties along/across fibers
        mat.material_flags.viscoelastic = 1;
        mat.update_derived_properties();
        return mat;
    }
    
    //-------------------------------------------------------------------------
    // Utility Methods
    //-------------------------------------------------------------------------
    
    /** @brief Update derived properties from fundamental ones */
    void update_derived_properties() noexcept {
        // Ensure Poisson's ratio is in valid range
        poissons_ratio = std::clamp(poissons_ratio, -0.99f, 0.49f);
        
        // Calculate derived moduli
        shear_modulus = youngs_modulus / (2.0f * (1.0f + poissons_ratio));
        
        // Avoid division by zero for incompressible materials
        if (poissons_ratio < 0.49f) {
            bulk_modulus = youngs_modulus / (3.0f * (1.0f - 2.0f * poissons_ratio));
        } else {
            bulk_modulus = 1e12f; // Very large for incompressible
        }
    }
    
    /** @brief Check if material properties are valid */
    bool is_valid() const noexcept {
        return youngs_modulus > 0.0f &&
               poissons_ratio >= -1.0f && poissons_ratio <= 0.5f &&
               shear_modulus > 0.0f &&
               bulk_modulus > 0.0f &&
               structural_damping >= 0.0f && structural_damping <= 1.0f &&
               density > 0.0f &&
               thickness > 0.0f;
    }
    
    /** @brief Get material description */
    const char* get_material_description() const noexcept {
        if (youngs_modulus < 1e4f) return "Very Soft (Jelly)";
        else if (youngs_modulus < 1e5f) return "Soft (Tissue)";
        else if (youngs_modulus < 1e6f) return "Medium (Rubber)";
        else if (youngs_modulus < 1e7f) return "Stiff (Plastic)";
        else return "Very Stiff (Metal)";
    }
};

//=============================================================================
// Soft Body Particle System
//=============================================================================

/**
 * @brief Individual particle in soft body simulation
 * 
 * Represents a point mass in the soft body mesh. Contains position,
 * velocity, forces, and additional properties for advanced simulation.
 */
struct alignas(16) SoftBodyParticle {
    //-------------------------------------------------------------------------
    // Kinematic State
    //-------------------------------------------------------------------------
    
    /** @brief Current world position */
    Vec2 position{0.0f, 0.0f};
    
    /** @brief Previous position (for Verlet integration) */
    Vec2 previous_position{0.0f, 0.0f};
    
    /** @brief Current velocity */
    Vec2 velocity{0.0f, 0.0f};
    
    /** @brief Accumulated forces for this frame */
    Vec2 force{0.0f, 0.0f};
    
    //-------------------------------------------------------------------------
    // Physical Properties
    //-------------------------------------------------------------------------
    
    /** @brief Particle mass */
    f32 mass{1.0f};
    
    /** @brief Inverse mass (0 = infinite mass/pinned) */
    f32 inverse_mass{1.0f};
    
    /** @brief Local rest position in soft body */
    Vec2 rest_position{0.0f, 0.0f};
    
    /** @brief Associated area/volume for this particle */
    f32 associated_volume{1.0f};
    
    //-------------------------------------------------------------------------
    // Simulation Control
    //-------------------------------------------------------------------------
    
    /** @brief Particle flags */
    union {
        u16 flags;
        struct {
            u16 is_pinned : 1;          ///< Fixed in space (infinite mass)
            u16 is_surface : 1;         ///< On surface (for collision/rendering)
            u16 is_internal : 1;        ///< Internal particle (optimization)
            u16 enable_collision : 1;   ///< Participates in collision detection
            u16 is_torn : 1;           ///< Particle has been torn/fractured
            u16 is_plastically_deformed : 1; ///< Has permanent deformation
            u16 reserved : 10;
        };
    } particle_flags{0};
    
    /** @brief Particle ID for tracking */
    u16 particle_id{0};
    
    //-------------------------------------------------------------------------
    // Advanced Properties
    //-------------------------------------------------------------------------
    
    /** @brief Current stress tensor (for visualization) */
    struct {
        f32 xx, xy, yx, yy; // 2x2 stress tensor components
    } stress{0.0f, 0.0f, 0.0f, 0.0f};
    
    /** @brief Current temperature (for thermal effects) */
    f32 temperature{293.15f};
    
    /** @brief Plastic strain magnitude */
    f32 plastic_strain{0.0f};
    
    /** @brief Damage variable (0 = intact, 1 = completely damaged) */
    f32 damage{0.0f};
    
    //-------------------------------------------------------------------------
    // Educational/Debug Information
    //-------------------------------------------------------------------------
    
    /** @brief Visualization color (for stress/strain display) */
    struct {
        u8 r, g, b, a;
    } debug_color{255, 255, 255, 255};
    
    /** @brief Educational metrics */
    mutable struct {
        f32 kinetic_energy{0.0f};       ///< Current kinetic energy
        f32 potential_energy{0.0f};     ///< Current potential energy
        f32 strain_energy{0.0f};        ///< Elastic strain energy
        f32 max_stress_magnitude{0.0f}; ///< Maximum stress experienced
        u32 constraint_count{0};        ///< Number of constraints attached
    } metrics;
    
    //-------------------------------------------------------------------------
    // Methods
    //-------------------------------------------------------------------------
    
    /** @brief Default constructor */
    constexpr SoftBodyParticle() noexcept = default;
    
    /** @brief Constructor with position and mass */
    constexpr SoftBodyParticle(Vec2 pos, f32 m) noexcept 
        : position(pos), previous_position(pos), rest_position(pos), mass(m) {
        inverse_mass = (m > 0.0f) ? 1.0f / m : 0.0f;
    }
    
    /** @brief Pin particle (make static) */
    void pin() noexcept {
        particle_flags.is_pinned = 1;
        mass = 0.0f;
        inverse_mass = 0.0f;
        velocity = Vec2::zero();
        force = Vec2::zero();
    }
    
    /** @brief Unpin particle (make dynamic) */
    void unpin(f32 new_mass = 1.0f) noexcept {
        particle_flags.is_pinned = 0;
        mass = new_mass;
        inverse_mass = (new_mass > 0.0f) ? 1.0f / new_mass : 0.0f;
    }
    
    /** @brief Apply force to particle */
    void apply_force(const Vec2& f) noexcept {
        if (!particle_flags.is_pinned) {
            force += f;
        }
    }
    
    /** @brief Apply impulse to particle */
    void apply_impulse(const Vec2& impulse) noexcept {
        if (!particle_flags.is_pinned) {
            velocity += impulse * inverse_mass;
        }
    }
    
    /** @brief Integrate motion using Verlet integration */
    void integrate_verlet(f32 dt) noexcept {
        if (particle_flags.is_pinned) return;
        
        Vec2 acceleration = force * inverse_mass;
        Vec2 new_position = position * 2.0f - previous_position + acceleration * (dt * dt);
        
        // Update velocity from position change
        velocity = (new_position - position) / dt;
        
        // Update positions
        previous_position = position;
        position = new_position;
        
        // Clear forces for next frame
        force = Vec2::zero();
        
        // Update metrics
        metrics.kinetic_energy = 0.5f * mass * velocity.length_squared();
    }
    
    /** @brief Integrate motion using improved Euler */
    void integrate_euler(f32 dt) noexcept {
        if (particle_flags.is_pinned) return;
        
        Vec2 acceleration = force * inverse_mass;
        velocity += acceleration * dt;
        position += velocity * dt;
        force = Vec2::zero();
        
        metrics.kinetic_energy = 0.5f * mass * velocity.length_squared();
    }
    
    /** @brief Calculate distance from rest position (deformation) */
    f32 get_deformation_magnitude() const noexcept {
        return (position - rest_position).length();
    }
    
    /** @brief Update stress visualization color */
    void update_stress_color(f32 max_stress) noexcept {
        f32 stress_magnitude = std::sqrt(stress.xx * stress.xx + stress.yy * stress.yy + 
                                       2.0f * stress.xy * stress.xy);
        f32 normalized_stress = std::clamp(stress_magnitude / max_stress, 0.0f, 1.0f);
        
        // Blue (low stress) -> Green -> Yellow -> Red (high stress)
        if (normalized_stress < 0.5f) {
            debug_color.b = 255;
            debug_color.g = static_cast<u8>(255 * normalized_stress * 2.0f);
            debug_color.r = 0;
        } else {
            debug_color.b = 0;
            debug_color.g = static_cast<u8>(255 * (1.0f - (normalized_stress - 0.5f) * 2.0f));
            debug_color.r = static_cast<u8>(255 * (normalized_stress - 0.5f) * 2.0f);
        }
        debug_color.a = 255;
    }
    
    /** @brief Check if particle is valid */
    bool is_valid() const noexcept {
        return !std::isnan(position.x) && !std::isnan(position.y) &&
               !std::isnan(velocity.x) && !std::isnan(velocity.y) &&
               mass >= 0.0f && associated_volume > 0.0f;
    }
};

//=============================================================================
// Soft Body Constraints
//=============================================================================

/**
 * @brief Base class for soft body constraints
 * 
 * Constraints maintain relationships between particles, such as distance,
 * angle, or volume preservation. This is the foundation for mass-spring
 * networks and finite element methods.
 */
class SoftBodyConstraint {
public:
    /**
     * @brief Constraint types for different physics behaviors
     */
    enum class Type : u8 {
        Distance,       ///< Maintain distance between two particles
        Bend,          ///< Maintain angle between three particles
        Volume,        ///< Maintain area/volume of triangle/tetrahedron
        Collision,     ///< Handle particle-object collision
        SelfCollision, ///< Handle particle-particle collision
        Attachment,    ///< Attach particle to rigid body
        Tear,          ///< Constraint that can break under stress
        Plasticity     ///< Constraint with permanent deformation
    };
    
protected:
    Type constraint_type_;
    bool is_active_{true};
    f32 stiffness_{1.0f};
    f32 damping_{0.1f};
    u32 iteration_count_{0};
    f32 accumulated_impulse_{0.0f};
    
public:
    explicit SoftBodyConstraint(Type type) : constraint_type_(type) {}
    virtual ~SoftBodyConstraint() = default;
    
    /** @brief Apply constraint to particles */
    virtual void solve_constraint(std::span<SoftBodyParticle> particles, f32 dt) = 0;
    
    /** @brief Get constraint type */
    Type get_type() const noexcept { return constraint_type_; }
    
    /** @brief Check if constraint is active */
    bool is_active() const noexcept { return is_active_; }
    
    /** @brief Deactivate constraint */
    void deactivate() noexcept { is_active_ = false; }
    
    /** @brief Set constraint stiffness */
    void set_stiffness(f32 k) noexcept { stiffness_ = std::max(0.0f, k); }
    
    /** @brief Set constraint damping */
    void set_damping(f32 d) noexcept { damping_ = std::clamp(d, 0.0f, 1.0f); }
    
    /** @brief Get constraint statistics */
    virtual f32 get_constraint_error() const noexcept { return 0.0f; }
    virtual f32 get_constraint_force() const noexcept { return accumulated_impulse_; }
};

/**
 * @brief Distance constraint between two particles
 * 
 * Maintains a specific distance between two particles. This is the fundamental
 * building block of mass-spring systems and cloth simulation.
 */
class DistanceConstraint : public SoftBodyConstraint {
private:
    u32 particle_a_;
    u32 particle_b_;
    f32 rest_distance_;
    f32 max_stretch_ratio_;
    f32 current_distance_;
    
public:
    DistanceConstraint(u32 a, u32 b, f32 rest_dist, f32 max_stretch = 2.0f) 
        : SoftBodyConstraint(Type::Distance), particle_a_(a), particle_b_(b),
          rest_distance_(rest_dist), max_stretch_ratio_(max_stretch) {}
    
    void solve_constraint(std::span<SoftBodyParticle> particles, f32 dt) override {
        if (!is_active_ || particle_a_ >= particles.size() || particle_b_ >= particles.size()) {
            return;
        }
        
        auto& p1 = particles[particle_a_];
        auto& p2 = particles[particle_b_];
        
        if (p1.particle_flags.is_pinned && p2.particle_flags.is_pinned) {
            return;
        }
        
        Vec2 delta = p2.position - p1.position;
        current_distance_ = delta.length();
        
        // Check for constraint breaking
        if (max_stretch_ratio_ > 0.0f && current_distance_ > rest_distance_ * max_stretch_ratio_) {
            is_active_ = false;
            return;
        }
        
        if (current_distance_ < 1e-6f) {
            return; // Avoid division by zero
        }
        
        f32 constraint_error = current_distance_ - rest_distance_;
        Vec2 constraint_direction = delta / current_distance_;
        
        // Calculate relative velocity along constraint direction
        Vec2 relative_velocity = p2.velocity - p1.velocity;
        f32 velocity_error = vec2::dot(relative_velocity, constraint_direction);
        
        // Combined constraint force (position + velocity correction)
        f32 constraint_magnitude = -(stiffness_ * constraint_error + damping_ * velocity_error);
        constraint_magnitude /= (p1.inverse_mass + p2.inverse_mass);
        
        Vec2 constraint_impulse = constraint_direction * constraint_magnitude * dt;
        accumulated_impulse_ = std::abs(constraint_magnitude);
        
        // Apply position corrections
        if (!p1.particle_flags.is_pinned) {
            Vec2 correction = constraint_impulse * p1.inverse_mass * 0.5f;
            p1.position -= correction;
            p1.velocity -= constraint_impulse * p1.inverse_mass / dt;
        }
        
        if (!p2.particle_flags.is_pinned) {
            Vec2 correction = constraint_impulse * p2.inverse_mass * 0.5f;
            p2.position += correction;
            p2.velocity += constraint_impulse * p2.inverse_mass / dt;
        }
        
        iteration_count_++;
    }
    
    f32 get_constraint_error() const noexcept override {
        return std::abs(current_distance_ - rest_distance_);
    }
    
    /** @brief Get the two particles involved in this constraint */
    std::pair<u32, u32> get_particles() const noexcept {
        return {particle_a_, particle_b_};
    }
    
    /** @brief Get rest distance */
    f32 get_rest_distance() const noexcept { return rest_distance_; }
    
    /** @brief Get current distance */
    f32 get_current_distance() const noexcept { return current_distance_; }
    
    /** @brief Get stretch ratio */
    f32 get_stretch_ratio() const noexcept {
        return (rest_distance_ > 0.0f) ? current_distance_ / rest_distance_ : 1.0f;
    }
};

/**
 * @brief Bend constraint between three particles
 * 
 * Maintains the angle between three connected particles. Essential for
 * cloth simulation to prevent unrealistic folding.
 */
class BendConstraint : public SoftBodyConstraint {
private:
    u32 particle_a_, particle_b_, particle_c_;
    f32 rest_angle_;
    f32 current_angle_;
    
public:
    BendConstraint(u32 a, u32 b, u32 c, f32 rest_ang = 0.0f)
        : SoftBodyConstraint(Type::Bend), particle_a_(a), particle_b_(b), 
          particle_c_(c), rest_angle_(rest_ang) {}
    
    void solve_constraint(std::span<SoftBodyParticle> particles, f32 dt) override {
        if (!is_active_ || particle_a_ >= particles.size() || 
            particle_b_ >= particles.size() || particle_c_ >= particles.size()) {
            return;
        }
        
        auto& pa = particles[particle_a_];
        auto& pb = particles[particle_b_];
        auto& pc = particles[particle_c_];
        
        // Calculate current angle
        Vec2 v1 = pa.position - pb.position;
        Vec2 v2 = pc.position - pb.position;
        
        f32 len1 = v1.length();
        f32 len2 = v2.length();
        
        if (len1 < 1e-6f || len2 < 1e-6f) return;
        
        v1 /= len1;
        v2 /= len2;
        
        f32 cos_angle = std::clamp(vec2::dot(v1, v2), -1.0f, 1.0f);
        current_angle_ = std::acos(cos_angle);
        
        f32 angle_error = current_angle_ - rest_angle_;
        
        // Calculate constraint forces (simplified bending model)
        f32 constraint_magnitude = -stiffness_ * angle_error;
        constraint_magnitude /= (pa.inverse_mass + pb.inverse_mass + pc.inverse_mass);
        
        // Apply torque-like forces
        Vec2 force_direction = vec2::perpendicular(v1 + v2).normalized();
        Vec2 force = force_direction * constraint_magnitude * dt;
        
        if (!pa.particle_flags.is_pinned) {
            pa.apply_force(force * pa.inverse_mass);
        }
        
        if (!pc.particle_flags.is_pinned) {
            pc.apply_force(-force * pc.inverse_mass);
        }
        
        accumulated_impulse_ = std::abs(constraint_magnitude);
        iteration_count_++;
    }
    
    f32 get_constraint_error() const noexcept override {
        return std::abs(current_angle_ - rest_angle_);
    }
    
    /** @brief Get the three particles involved */
    std::tuple<u32, u32, u32> get_particles() const noexcept {
        return {particle_a_, particle_b_, particle_c_};
    }
};

/**
 * @brief Volume/Area preservation constraint
 * 
 * Maintains the area of a triangle formed by three particles.
 * Essential for incompressible materials and volume conservation.
 */
class VolumeConstraint : public SoftBodyConstraint {
private:
    u32 particle_a_, particle_b_, particle_c_;
    f32 rest_area_;
    f32 current_area_;
    
public:
    VolumeConstraint(u32 a, u32 b, u32 c, f32 rest_area)
        : SoftBodyConstraint(Type::Volume), particle_a_(a), particle_b_(b), 
          particle_c_(c), rest_area_(rest_area) {}
    
    void solve_constraint(std::span<SoftBodyParticle> particles, f32 dt) override {
        if (!is_active_ || particle_a_ >= particles.size() || 
            particle_b_ >= particles.size() || particle_c_ >= particles.size()) {
            return;
        }
        
        auto& pa = particles[particle_a_];
        auto& pb = particles[particle_b_];
        auto& pc = particles[particle_c_];
        
        // Calculate current area using cross product
        Vec2 v1 = pb.position - pa.position;
        Vec2 v2 = pc.position - pa.position;
        current_area_ = std::abs(v1.x * v2.y - v1.y * v2.x) * 0.5f;
        
        f32 area_error = current_area_ - rest_area_;
        
        // Apply area-preserving forces
        if (std::abs(area_error) > 1e-6f) {
            f32 constraint_magnitude = -stiffness_ * area_error;
            constraint_magnitude /= (pa.inverse_mass + pb.inverse_mass + pc.inverse_mass);
            
            // Calculate gradients (partial derivatives of area w.r.t. positions)
            Vec2 grad_a = Vec2{pb.position.y - pc.position.y, pc.position.x - pb.position.x} * 0.5f;
            Vec2 grad_b = Vec2{pc.position.y - pa.position.y, pa.position.x - pc.position.x} * 0.5f;
            Vec2 grad_c = Vec2{pa.position.y - pb.position.y, pb.position.x - pa.position.x} * 0.5f;
            
            // Apply forces along gradients
            if (!pa.particle_flags.is_pinned) {
                pa.apply_force(grad_a * constraint_magnitude * pa.inverse_mass * dt);
            }
            
            if (!pb.particle_flags.is_pinned) {
                pb.apply_force(grad_b * constraint_magnitude * pb.inverse_mass * dt);
            }
            
            if (!pc.particle_flags.is_pinned) {
                pc.apply_force(grad_c * constraint_magnitude * pc.inverse_mass * dt);
            }
        }
        
        accumulated_impulse_ = std::abs(current_area_ - rest_area_);
        iteration_count_++;
    }
    
    f32 get_constraint_error() const noexcept override {
        return std::abs(current_area_ - rest_area_);
    }
    
    /** @brief Get current compression ratio */
    f32 get_compression_ratio() const noexcept {
        return (rest_area_ > 0.0f) ? current_area_ / rest_area_ : 1.0f;
    }
};

} // namespace ecscope::physics::soft_body