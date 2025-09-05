#pragma once

/**
 * @file fluid_simulation.hpp
 * @brief Advanced Fluid Simulation System for ECScope - SPH/PBF Implementation
 * 
 * This header implements a comprehensive fluid simulation system using both
 * Smoothed Particle Hydrodynamics (SPH) and Position-Based Fluids (PBF).
 * Designed for educational purposes while maintaining high performance.
 * 
 * Key Features:
 * - SPH (Smoothed Particle Hydrodynamics) for realistic fluid dynamics
 * - PBF (Position-Based Fluids) for stable, game-ready fluid simulation
 * - Fluid-rigid body interaction and coupling
 * - Multiple fluid types (water, oil, honey, etc.)
 * - Surface tension and viscosity effects
 * - Educational visualization of fluid properties
 * - Efficient spatial partitioning and neighbor finding
 * - GPU-friendly data structures and algorithms
 * 
 * Educational Goals:
 * - Demonstrate continuum mechanics and fluid dynamics principles
 * - Show the difference between Lagrangian and Eulerian approaches
 * - Visualize pressure, velocity fields, and fluid flow patterns
 * - Compare SPH vs PBF approaches and their trade-offs
 * - Illustrate computational fluid dynamics (CFD) concepts
 * 
 * Performance Targets:
 * - 10,000+ fluid particles at 60 FPS
 * - Real-time fluid-solid interaction
 * - Efficient memory usage and cache coherency
 * - SIMD-optimized computations
 * - Scalable spatial data structures
 * 
 * @author ECScope Educational Physics Engine
 * @date 2024
 */

#include "physics.hpp"
#include "soft_body_physics.hpp"
#include "../ecs/component.hpp"
#include "../memory/pool.hpp"
#include "../core/simd_math.hpp"
#include <vector>
#include <array>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace ecscope::physics::fluid {

// Import necessary types
using namespace ecscope::physics::math;
using namespace ecscope::ecs;

//=============================================================================
// Fluid Material Properties
//=============================================================================

/**
 * @brief Comprehensive fluid material properties
 * 
 * Defines the physical characteristics that determine fluid behavior.
 * Includes educational information about fluid mechanics concepts.
 */
struct alignas(32) FluidMaterial {
    //-------------------------------------------------------------------------
    // Basic Fluid Properties
    //-------------------------------------------------------------------------
    
    /**
     * @brief Fluid density (kg/m³)
     * 
     * Mass per unit volume. Affects buoyancy, pressure, and inertia.
     * 
     * Educational Context:
     * Density determines how "heavy" the fluid feels and affects:
     * - Hydrostatic pressure: P = ρgh
     * - Buoyancy force: F = ρVg (Archimedes' principle)
     * - Momentum transfer in collisions
     * 
     * Common Values:
     * - Air: 1.225 kg/m³
     * - Water: 1000 kg/m³
     * - Oil: 800-900 kg/m³
     * - Honey: 1420 kg/m³
     * - Mercury: 13534 kg/m³
     */
    f32 rest_density{1000.0f};
    
    /**
     * @brief Dynamic viscosity (Pa·s)
     * 
     * Resistance to shear deformation. Higher values = thicker fluid.
     * 
     * Educational Context:
     * Viscosity determines how "thick" fluid feels:
     * - Low viscosity: Water, alcohol (flows easily)
     * - Medium viscosity: Oil, syrup (flows slowly)
     * - High viscosity: Honey, molasses (very thick)
     * 
     * Shear stress: τ = μ(∂u/∂y) (Newton's law of viscosity)
     * 
     * Common Values:
     * - Water: 1.0 × 10⁻³ Pa·s
     * - Oil: 0.1-1.0 Pa·s  
     * - Honey: 2-10 Pa·s
     * - Pitch: 10⁸ Pa·s
     */
    f32 viscosity{1e-3f}; // Water viscosity
    
    /**
     * @brief Surface tension coefficient (N/m)
     * 
     * Cohesive force between fluid particles at interfaces.
     * Creates droplet formation and capillary effects.
     * 
     * Educational Context:
     * Surface tension causes:
     * - Droplet formation (minimizes surface area)
     * - Capillary action (liquid climbing narrow tubes)
     * - Contact angles with surfaces
     * - Pressure inside curved interfaces: ΔP = γ/R
     * 
     * Common Values:
     * - Water-air: 0.0728 N/m
     * - Oil-air: 0.02-0.05 N/m
     * - Mercury-air: 0.486 N/m
     */
    f32 surface_tension{0.0728f}; // Water surface tension
    
    /**
     * @brief Bulk modulus (Pa)
     * 
     * Resistance to compression. Determines speed of sound in fluid.
     * Higher values = less compressible fluid.
     * 
     * Educational Context:
     * - Speed of sound: c = √(K/ρ)
     * - Compressibility: β = 1/K
     * - Most liquids are nearly incompressible
     * - Gases are highly compressible
     * 
     * Common Values:
     * - Water: 2.2 × 10⁹ Pa
     * - Oil: 1-2 × 10⁹ Pa
     * - Air: 1.4 × 10⁵ Pa
     */
    f32 bulk_modulus{2.2e9f}; // Water bulk modulus
    
    //-------------------------------------------------------------------------
    // SPH Simulation Parameters
    //-------------------------------------------------------------------------
    
    /**
     * @brief SPH smoothing length (kernel radius)
     * 
     * Radius of influence for SPH kernel functions.
     * Determines the "resolution" of the fluid simulation.
     * 
     * Educational Context:
     * - Larger h = smoother but less detailed
     * - Smaller h = more detailed but less stable
     * - Usually h ≈ 1.2-2.0 × particle spacing
     * - Affects computational cost as O(h³) in 3D
     */
    f32 smoothing_length{0.05f};
    
    /**
     * @brief Particle mass
     * 
     * Mass of each fluid particle. Usually calculated from
     * density and particle volume.
     */
    f32 particle_mass{0.001f};
    
    /**
     * @brief Gas constant for pressure calculation
     * 
     * Used in equation of state for pressure computation.
     * P = k(ρ - ρ₀) where k is gas constant, ρ is density, ρ₀ is rest density.
     */
    f32 gas_constant{50.0f};
    
    /**
     * @brief Artificial pressure for particle distribution
     * 
     * Additional pressure term to prevent particle clustering
     * and maintain uniform distribution.
     */
    f32 artificial_pressure{0.1f};
    
    /**
     * @brief Vorticity confinement strength
     * 
     * Artificial force to restore vorticity lost due to
     * numerical dissipation. Adds swirling motion back.
     */
    f32 vorticity_confinement{0.05f};
    
    //-------------------------------------------------------------------------
    // PBF Simulation Parameters
    //-------------------------------------------------------------------------
    
    /**
     * @brief PBF constraint relaxation parameter
     * 
     * Controls how quickly constraints are satisfied.
     * Higher values = faster convergence but less stability.
     */
    f32 constraint_relaxation{0.1f};
    
    /**
     * @brief Number of PBF solver iterations
     * 
     * More iterations = better constraint satisfaction but higher cost.
     * Typically 2-5 iterations for real-time applications.
     */
    u32 solver_iterations{3};
    
    /**
     * @brief PBF artificial pressure radius
     * 
     * Radius for artificial pressure calculation in PBF.
     * Usually smaller than smoothing length.
     */
    f32 artificial_pressure_radius{0.3f};
    
    /**
     * @brief PBF artificial pressure strength
     * 
     * Strength of artificial pressure to prevent clustering.
     */
    f32 artificial_pressure_strength{0.01f};
    
    //-------------------------------------------------------------------------
    // Interaction Properties
    //-------------------------------------------------------------------------
    
    /**
     * @brief Adhesion strength with solid surfaces
     * 
     * How much fluid "sticks" to solid objects.
     * 0 = no adhesion, higher = more sticky.
     */
    f32 adhesion_strength{0.1f};
    
    /**
     * @brief Cohesion strength between fluid particles
     * 
     * Internal attraction between fluid particles.
     * Related to surface tension but for volume interactions.
     */
    f32 cohesion_strength{0.05f};
    
    /**
     * @brief Restitution with solid objects
     * 
     * Bounciness when fluid hits solid surfaces.
     * 0 = perfectly inelastic, 1 = perfectly elastic.
     */
    f32 restitution{0.1f};
    
    /**
     * @brief Friction coefficient with solids
     * 
     * Resistance to sliding along solid surfaces.
     */
    f32 friction{0.3f};
    
    //-------------------------------------------------------------------------
    // Thermal Properties
    //-------------------------------------------------------------------------
    
    /**
     * @brief Specific heat capacity (J/kg·K)
     * 
     * Amount of heat needed to raise temperature of 1kg by 1K.
     */
    f32 specific_heat{4184.0f}; // Water specific heat
    
    /**
     * @brief Thermal conductivity (W/m·K)
     * 
     * Rate of heat transfer through the fluid.
     */
    f32 thermal_conductivity{0.6f}; // Water thermal conductivity
    
    /**
     * @brief Current fluid temperature (K)
     */
    f32 temperature{293.15f}; // Room temperature
    
    //-------------------------------------------------------------------------
    // Simulation Control Flags
    //-------------------------------------------------------------------------
    
    union {
        u32 flags;
        struct {
            u32 enable_surface_tension : 1;   ///< Calculate surface tension forces
            u32 enable_viscosity : 1;         ///< Calculate viscous forces
            u32 enable_vorticity : 1;         ///< Apply vorticity confinement
            u32 enable_thermal : 1;           ///< Simulate thermal effects
            u32 incompressible : 1;           ///< Use incompressible formulation
            u32 use_pbf : 1;                  ///< Use PBF instead of SPH
            u32 enable_surface_detection : 1; ///< Detect and mark surface particles
            u32 enable_foaming : 1;           ///< Allow foam/bubble generation
            u32 enable_evaporation : 1;       ///< Allow particle removal (evaporation)
            u32 enable_two_way_coupling : 1;  ///< Affect rigid bodies
            u32 reserved : 22;
        };
    } simulation_flags{0x3FF}; // Enable most features by default
    
    //-------------------------------------------------------------------------
    // Visual Properties
    //-------------------------------------------------------------------------
    
    /**
     * @brief Fluid color (RGBA)
     */
    struct {
        u8 r, g, b, a;
    } color{100, 150, 255, 200}; // Semi-transparent blue (water-like)
    
    /**
     * @brief Opacity/transparency
     * 
     * 0 = completely transparent, 1 = completely opaque
     */
    f32 opacity{0.8f};
    
    /**
     * @brief Refraction index (for advanced rendering)
     */
    f32 refraction_index{1.33f}; // Water refraction index
    
    //-------------------------------------------------------------------------
    // Factory Methods
    //-------------------------------------------------------------------------
    
    /** @brief Create water-like fluid */
    static FluidMaterial create_water() noexcept {
        FluidMaterial mat;
        mat.rest_density = 1000.0f;
        mat.viscosity = 1e-3f;
        mat.surface_tension = 0.0728f;
        mat.bulk_modulus = 2.2e9f;
        mat.color = {100, 150, 255, 200};
        mat.opacity = 0.8f;
        mat.refraction_index = 1.33f;
        mat.simulation_flags.enable_surface_tension = 1;
        mat.simulation_flags.enable_viscosity = 1;
        mat.simulation_flags.incompressible = 1;
        return mat;
    }
    
    /** @brief Create oil-like fluid */
    static FluidMaterial create_oil() noexcept {
        FluidMaterial mat;
        mat.rest_density = 850.0f;
        mat.viscosity = 0.5f; // Higher viscosity
        mat.surface_tension = 0.035f;
        mat.bulk_modulus = 1.5e9f;
        mat.color = {80, 60, 20, 180};
        mat.opacity = 0.7f;
        mat.adhesion_strength = 0.3f; // Oily, sticks more
        mat.friction = 0.1f; // Slippery
        return mat;
    }
    
    /** @brief Create honey-like fluid */
    static FluidMaterial create_honey() noexcept {
        FluidMaterial mat;
        mat.rest_density = 1420.0f;
        mat.viscosity = 5.0f; // Very viscous
        mat.surface_tension = 0.08f;
        mat.bulk_modulus = 3e9f;
        mat.color = {255, 200, 50, 220};
        mat.opacity = 0.9f;
        mat.adhesion_strength = 0.5f; // Very sticky
        mat.cohesion_strength = 0.2f; // Strong internal cohesion
        return mat;
    }
    
    /** @brief Create mercury-like fluid */
    static FluidMaterial create_mercury() noexcept {
        FluidMaterial mat;
        mat.rest_density = 13534.0f; // Very heavy
        mat.viscosity = 1.5e-3f;
        mat.surface_tension = 0.486f; // Very high surface tension
        mat.bulk_modulus = 25e9f;
        mat.color = {200, 200, 220, 255};
        mat.opacity = 1.0f;
        mat.adhesion_strength = 0.01f; // Doesn't stick to most surfaces
        mat.cohesion_strength = 0.8f; // Very cohesive (forms droplets easily)
        return mat;
    }
    
    /** @brief Create gas/air-like fluid */
    static FluidMaterial create_gas() noexcept {
        FluidMaterial mat;
        mat.rest_density = 1.225f; // Air density
        mat.viscosity = 1.8e-5f; // Very low viscosity
        mat.surface_tension = 0.0f; // Gases don't have surface tension
        mat.bulk_modulus = 1.4e5f; // Compressible
        mat.gas_constant = 287.0f; // Ideal gas constant for air
        mat.color = {220, 220, 255, 50}; // Very transparent
        mat.opacity = 0.2f;
        mat.simulation_flags.incompressible = 0; // Gases are compressible
        mat.simulation_flags.enable_surface_tension = 0;
        return mat;
    }
    
    //-------------------------------------------------------------------------
    // Utility Methods
    //-------------------------------------------------------------------------
    
    /** @brief Update derived parameters from fundamental properties */
    void update_derived_parameters() noexcept {
        // Calculate particle mass from density and smoothing length
        f32 volume_per_particle = smoothing_length * smoothing_length * smoothing_length; // Approximate
        particle_mass = rest_density * volume_per_particle;
        
        // Adjust gas constant based on bulk modulus
        gas_constant = bulk_modulus / rest_density;
        
        // Ensure reasonable parameter ranges
        viscosity = std::max(0.0f, viscosity);
        surface_tension = std::max(0.0f, surface_tension);
        smoothing_length = std::max(0.001f, smoothing_length);
    }
    
    /** @brief Check if material properties are physically reasonable */
    bool is_valid() const noexcept {
        return rest_density > 0.0f &&
               viscosity >= 0.0f &&
               surface_tension >= 0.0f &&
               bulk_modulus > 0.0f &&
               smoothing_length > 0.0f &&
               particle_mass > 0.0f &&
               gas_constant > 0.0f;
    }
    
    /** @brief Get fluid type description */
    const char* get_fluid_description() const noexcept {
        if (rest_density > 10000.0f) return "Heavy Liquid (Mercury-like)";
        else if (rest_density > 1200.0f) return "Dense Liquid (Honey-like)";
        else if (rest_density > 500.0f) return "Normal Liquid (Water/Oil-like)";
        else if (rest_density > 10.0f) return "Light Liquid";
        else return "Gas";
    }
    
    /** @brief Calculate Reynolds number for educational purposes */
    f32 calculate_reynolds_number(f32 characteristic_velocity, f32 characteristic_length) const noexcept {
        return (rest_density * characteristic_velocity * characteristic_length) / viscosity;
    }
    
    /** @brief Get flow regime description based on Reynolds number */
    const char* get_flow_regime_description(f32 reynolds_number) const noexcept {
        if (reynolds_number < 1.0f) return "Stokes Flow (Viscous)";
        else if (reynolds_number < 100.0f) return "Laminar Flow";
        else if (reynolds_number < 2000.0f) return "Transitional Flow";
        else return "Turbulent Flow";
    }
};

//=============================================================================
// Fluid Particle System
//=============================================================================

/**
 * @brief Individual fluid particle for SPH/PBF simulation
 * 
 * Represents a small volume of fluid with associated properties.
 * Contains both SPH and PBF specific data for maximum flexibility.
 */
struct alignas(32) FluidParticle {
    //-------------------------------------------------------------------------
    // Kinematic State
    //-------------------------------------------------------------------------
    
    /** @brief Current world position */
    Vec2 position{0.0f, 0.0f};
    
    /** @brief Previous position (for Verlet integration) */
    Vec2 previous_position{0.0f, 0.0f};
    
    /** @brief Current velocity */
    Vec2 velocity{0.0f, 0.0f};
    
    /** @brief Predicted position (for PBF) */
    Vec2 predicted_position{0.0f, 0.0f};
    
    //-------------------------------------------------------------------------
    // SPH Properties
    //-------------------------------------------------------------------------
    
    /** @brief Current density */
    f32 density{1000.0f};
    
    /** @brief Current pressure */
    f32 pressure{0.0f};
    
    /** @brief Accumulated force */
    Vec2 force{0.0f, 0.0f};
    
    /** @brief Particle mass */
    f32 mass{0.001f};
    
    //-------------------------------------------------------------------------
    // PBF Properties
    //-------------------------------------------------------------------------
    
    /** @brief Lambda value for PBF constraint */
    f32 lambda{0.0f};
    
    /** @brief Position correction delta */
    Vec2 position_delta{0.0f, 0.0f};
    
    /** @brief Number of neighbor particles */
    u32 neighbor_count{0};
    
    //-------------------------------------------------------------------------
    // Physical Properties
    //-------------------------------------------------------------------------
    
    /** @brief Current temperature */
    f32 temperature{293.15f};
    
    /** @brief Vorticity (curl of velocity field) */
    f32 vorticity{0.0f};
    
    /** @brief Color field (for surface detection) */
    f32 color_field{0.0f};
    
    /** @brief Surface normal (for surface particles) */
    Vec2 surface_normal{0.0f, 0.0f};
    
    //-------------------------------------------------------------------------
    // Simulation Control
    //-------------------------------------------------------------------------
    
    /** @brief Particle flags */
    union {
        u16 flags;
        struct {
            u16 is_surface : 1;          ///< Particle is on fluid surface
            u16 is_boundary : 1;         ///< Boundary/wall particle
            u16 is_active : 1;           ///< Particle is active in simulation
            u16 near_surface : 1;        ///< Close to surface (for optimization)
            u16 in_contact : 1;          ///< In contact with solid object
            u16 is_foam : 1;             ///< Part of foam/bubble
            u16 mark_for_removal : 1;    ///< Scheduled for removal (evaporation)
            u16 high_pressure : 1;       ///< Pressure above threshold
            u16 reserved : 8;
        };
    } particle_flags{0x04}; // Active by default
    
    /** @brief Unique particle ID */
    u32 particle_id{0};
    
    /** @brief Fluid material index */
    u16 material_id{0};
    
    /** @brief Grid cell index (for spatial hashing) */
    u32 grid_cell{0};
    
    //-------------------------------------------------------------------------
    // Neighbor Information
    //-------------------------------------------------------------------------
    
    /** @brief Maximum neighbors for fixed arrays */
    static constexpr u32 MAX_NEIGHBORS = 64;
    
    /** @brief Neighbor particle indices */
    std::array<u32, MAX_NEIGHBORS> neighbors;
    
    /** @brief Distances to neighbors (for kernel evaluation) */
    std::array<f32, MAX_NEIGHBORS> neighbor_distances;
    
    /** @brief Kernel values for neighbors (cached for performance) */
    std::array<f32, MAX_NEIGHBORS> neighbor_kernels;
    
    //-------------------------------------------------------------------------
    // Rendering/Debug Information
    //-------------------------------------------------------------------------
    
    /** @brief Particle color for visualization */
    struct {
        u8 r, g, b, a;
    } render_color{100, 150, 255, 200};
    
    /** @brief Particle size for rendering */
    f32 render_size{1.0f};
    
    /** @brief Debug/educational information */
    struct {
        f32 kinetic_energy{0.0f};        ///< Current kinetic energy
        f32 pressure_contribution{0.0f}; ///< Contribution to surrounding pressure
        Vec2 pressure_gradient{0.0f, 0.0f}; ///< Local pressure gradient
        f32 divergence{0.0f};            ///< Velocity field divergence
        f32 strain_rate{0.0f};           ///< Local strain rate magnitude
    } debug_info;
    
    //-------------------------------------------------------------------------
    // Methods
    //-------------------------------------------------------------------------
    
    /** @brief Default constructor */
    constexpr FluidParticle() noexcept {
        neighbors.fill(0);
        neighbor_distances.fill(0.0f);
        neighbor_kernels.fill(0.0f);
    }
    
    /** @brief Constructor with position */
    explicit FluidParticle(Vec2 pos) noexcept 
        : position(pos), previous_position(pos), predicted_position(pos) {
        neighbors.fill(0);
        neighbor_distances.fill(0.0f);
        neighbor_kernels.fill(0.0f);
    }
    
    /** @brief Add neighbor particle */
    bool add_neighbor(u32 neighbor_id, f32 distance, f32 kernel_value) noexcept {
        if (neighbor_count >= MAX_NEIGHBORS) return false;
        
        neighbors[neighbor_count] = neighbor_id;
        neighbor_distances[neighbor_count] = distance;
        neighbor_kernels[neighbor_count] = kernel_value;
        neighbor_count++;
        return true;
    }
    
    /** @brief Clear neighbor list */
    void clear_neighbors() noexcept {
        neighbor_count = 0;
        // Don't need to clear arrays for performance
    }
    
    /** @brief Apply force to particle */
    void apply_force(const Vec2& f) noexcept {
        if (particle_flags.is_active && !particle_flags.is_boundary) {
            force += f;
        }
    }
    
    /** @brief Integrate using Verlet method */
    void integrate_verlet(f32 dt) noexcept {
        if (!particle_flags.is_active || particle_flags.is_boundary) return;
        
        Vec2 acceleration = force / mass;
        Vec2 new_position = position * 2.0f - previous_position + acceleration * (dt * dt);
        
        // Update velocity from position difference
        velocity = (new_position - position) / dt;
        
        // Update positions
        previous_position = position;
        position = new_position;
        
        // Clear forces
        force = Vec2::zero();
        
        // Update debug info
        debug_info.kinetic_energy = 0.5f * mass * velocity.length_squared();
    }
    
    /** @brief Integrate using leap-frog method (for SPH) */
    void integrate_leapfrog(f32 dt) noexcept {
        if (!particle_flags.is_active || particle_flags.is_boundary) return;
        
        Vec2 acceleration = force / mass;
        velocity += acceleration * dt;
        position += velocity * dt;
        force = Vec2::zero();
        
        debug_info.kinetic_energy = 0.5f * mass * velocity.length_squared();
    }
    
    /** @brief Update predicted position (for PBF) */
    void predict_position(f32 dt, const Vec2& external_force = Vec2::zero()) noexcept {
        if (!particle_flags.is_active || particle_flags.is_boundary) return;
        
        Vec2 acceleration = (force + external_force) / mass;
        velocity += acceleration * dt;
        predicted_position = position + velocity * dt;
    }
    
    /** @brief Apply position correction (for PBF) */
    void apply_position_correction() noexcept {
        if (!particle_flags.is_active || particle_flags.is_boundary) return;
        
        predicted_position += position_delta;
        position_delta = Vec2::zero();
    }
    
    /** @brief Finalize PBF step */
    void finalize_pbf_step(f32 dt) noexcept {
        if (!particle_flags.is_active || particle_flags.is_boundary) return;
        
        velocity = (predicted_position - position) / dt;
        position = predicted_position;
        force = Vec2::zero();
    }
    
    /** @brief Update surface detection */
    void update_surface_detection(f32 surface_threshold = 0.1f) noexcept {
        // Surface particles have lower color field values
        particle_flags.is_surface = (color_field < surface_threshold);
        particle_flags.near_surface = (color_field < surface_threshold * 2.0f);
        
        // Update render properties for surface particles
        if (particle_flags.is_surface) {
            render_size *= 1.2f; // Slightly larger for visibility
        }
    }
    
    /** @brief Update particle color based on properties */
    void update_visualization_color(const FluidMaterial& material) noexcept {
        // Base color from material
        render_color.r = material.color.r;
        render_color.g = material.color.g;
        render_color.b = material.color.b;
        render_color.a = material.color.a;
        
        // Modify based on pressure (blue = low, red = high)
        if (pressure > 0.0f) {
            f32 pressure_factor = std::clamp(pressure / 1000.0f, 0.0f, 1.0f);
            render_color.r = static_cast<u8>(std::lerp(render_color.r, 255, pressure_factor));
            render_color.g = static_cast<u8>(std::lerp(render_color.g, 100, pressure_factor));
            render_color.b = static_cast<u8>(std::lerp(render_color.b, 100, pressure_factor));
        }
        
        // Make surface particles more opaque
        if (particle_flags.is_surface) {
            render_color.a = std::min(255, static_cast<int>(render_color.a * 1.5f));
        }
    }
    
    /** @brief Check if particle is valid */
    bool is_valid() const noexcept {
        return !std::isnan(position.x) && !std::isnan(position.y) &&
               !std::isnan(velocity.x) && !std::isnan(velocity.y) &&
               mass > 0.0f && density > 0.0f &&
               neighbor_count <= MAX_NEIGHBORS;
    }
    
    /** @brief Get current speed */
    f32 get_speed() const noexcept {
        return velocity.length();
    }
    
    /** @brief Get distance to another particle */
    f32 distance_to(const FluidParticle& other) const noexcept {
        return (position - other.position).length();
    }
    
    /** @brief Check if particle is moving significantly */
    bool is_moving(f32 threshold = 0.01f) const noexcept {
        return velocity.length_squared() > threshold * threshold;
    }
};

//=============================================================================
// SPH Kernel Functions
//=============================================================================

/**
 * @brief SPH kernel functions for spatial averaging
 * 
 * Kernels determine how particle properties are averaged over space.
 * Different kernels have different properties (smoothness, support, etc.).
 */
namespace kernels {
    
    /**
     * @brief Cubic spline kernel (most common)
     * 
     * Good balance of smoothness and computational efficiency.
     * Compact support with radius h.
     */
    struct CubicSpline {
        static constexpr f32 normalization_2d = 10.0f / (7.0f * math::PI); // 2D normalization
        
        static f32 kernel(f32 r, f32 h) noexcept {
            if (h <= 0.0f) return 0.0f;
            
            f32 q = r / h;
            if (q >= 2.0f) return 0.0f;
            
            f32 norm = normalization_2d / (h * h);
            
            if (q < 1.0f) {
                return norm * (1.0f - 1.5f * q * q + 0.75f * q * q * q);
            } else {
                f32 temp = 2.0f - q;
                return norm * 0.25f * temp * temp * temp;
            }
        }
        
        static Vec2 gradient(const Vec2& r_vec, f32 h) noexcept {
            f32 r = r_vec.length();
            if (r <= 1e-6f || h <= 0.0f) return Vec2::zero();
            
            f32 q = r / h;
            if (q >= 2.0f) return Vec2::zero();
            
            f32 norm = normalization_2d / (h * h * h);
            f32 grad_magnitude;
            
            if (q < 1.0f) {
                grad_magnitude = norm * (-3.0f * q + 2.25f * q * q);
            } else {
                f32 temp = 2.0f - q;
                grad_magnitude = norm * (-0.75f * temp * temp);
            }
            
            return r_vec * (grad_magnitude / r);
        }
        
        static f32 laplacian(f32 r, f32 h) noexcept {
            if (h <= 0.0f) return 0.0f;
            
            f32 q = r / h;
            if (q >= 2.0f) return 0.0f;
            
            f32 norm = normalization_2d / (h * h * h * h);
            
            if (q < 1.0f) {
                return norm * (-3.0f + 4.5f * q);
            } else {
                return norm * (-1.5f * (2.0f - q));
            }
        }
    };
    
    /**
     * @brief Quintic kernel (higher order, smoother)
     * 
     * More computationally expensive but smoother gradients.
     * Good for high-quality simulations.
     */
    struct Quintic {
        static constexpr f32 normalization_2d = 7.0f / (478.0f * math::PI);
        
        static f32 kernel(f32 r, f32 h) noexcept {
            if (h <= 0.0f) return 0.0f;
            
            f32 q = r / h;
            if (q >= 3.0f) return 0.0f;
            
            f32 norm = normalization_2d / (h * h);
            f32 q2 = q * q;
            f32 result = 0.0f;
            
            if (q <= 1.0f) {
                result = std::pow(3.0f - q, 5) - 6.0f * std::pow(2.0f - q, 5) + 15.0f * std::pow(1.0f - q, 5);
            } else if (q <= 2.0f) {
                result = std::pow(3.0f - q, 5) - 6.0f * std::pow(2.0f - q, 5);
            } else {
                result = std::pow(3.0f - q, 5);
            }
            
            return norm * result;
        }
    };
    
    /**
     * @brief Poly6 kernel (good for density calculations)
     * 
     * Optimized for density and color field calculations.
     * Always positive, good for pressure computations.
     */
    struct Poly6 {
        static constexpr f32 normalization_2d = 4.0f / (math::PI);
        
        static f32 kernel(f32 r, f32 h) noexcept {
            if (h <= 0.0f || r >= h) return 0.0f;
            
            f32 norm = normalization_2d / (h * h * h * h * h * h * h * h);
            f32 diff = h * h - r * r;
            return norm * diff * diff * diff;
        }
        
        static Vec2 gradient(const Vec2& r_vec, f32 h) noexcept {
            f32 r = r_vec.length();
            if (r <= 1e-6f || h <= 0.0f || r >= h) return Vec2::zero();
            
            f32 norm = -6.0f * normalization_2d / (h * h * h * h * h * h * h * h);
            f32 diff = h * h - r * r;
            return r_vec * (norm * diff * diff);
        }
    };
    
    /**
     * @brief Spiky kernel (good for pressure forces)
     * 
     * Has good gradient properties for pressure force calculations.
     * Prevents particle clustering near origin.
     */
    struct Spiky {
        static constexpr f32 normalization_2d = 10.0f / (math::PI);
        
        static f32 kernel(f32 r, f32 h) noexcept {
            if (h <= 0.0f || r >= h) return 0.0f;
            
            f32 norm = normalization_2d / (h * h * h * h * h);
            f32 diff = h - r;
            return norm * diff * diff * diff;
        }
        
        static Vec2 gradient(const Vec2& r_vec, f32 h) noexcept {
            f32 r = r_vec.length();
            if (r <= 1e-6f || h <= 0.0f || r >= h) return Vec2::zero();
            
            f32 norm = -3.0f * normalization_2d / (h * h * h * h * h);
            f32 diff = h - r;
            return r_vec * (norm * diff * diff / r);
        }
    };
    
    /**
     * @brief Viscosity kernel (for viscous forces)
     * 
     * Designed specifically for viscosity calculations.
     * Has good laplacian properties for diffusion.
     */
    struct Viscosity {
        static constexpr f32 normalization_2d = 10.0f / (3.0f * math::PI);
        
        static f32 kernel(f32 r, f32 h) noexcept {
            if (h <= 0.0f || r >= h) return 0.0f;
            
            f32 norm = normalization_2d / (h * h * h);
            f32 q = r / h;
            return norm * (-0.5f * q * q * q + q * q - 0.5f * h / r + 1.0f);
        }
        
        static f32 laplacian(f32 r, f32 h) noexcept {
            if (h <= 0.0f || r >= h) return 0.0f;
            
            f32 norm = normalization_2d / (h * h * h * h * h);
            return norm * (h - r);
        }
    };
    
} // namespace kernels

} // namespace ecscope::physics::fluid