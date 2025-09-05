#pragma once

/**
 * @file advanced_physics_integration.hpp
 * @brief Advanced Physics Systems Integration for ECScope Engine
 * 
 * This header provides the integration layer that connects the advanced physics
 * systems (soft body, fluid, materials, education) with the existing ECS
 * architecture and memory management systems. Designed for maximum performance
 * while maintaining educational value.
 * 
 * Key Features:
 * - Seamless integration with existing physics system
 * - Memory-efficient component layouts and storage
 * - ECS system scheduling and dependencies
 * - Multi-threaded physics processing
 * - Educational performance monitoring
 * - Automatic optimization based on workload
 * 
 * Performance Goals:
 * - 1000+ rigid bodies at 60 FPS (maintained)
 * - 500+ soft body particles at 60 FPS
 * - 10,000+ fluid particles at 60 FPS
 * - Real-time material property updates
 * - Interactive educational features with minimal overhead
 * 
 * Integration Philosophy:
 * - Extend rather than replace existing systems
 * - Maintain backward compatibility
 * - Progressive enhancement approach
 * - Educational features as optional overlays
 * 
 * @author ECScope Educational Physics Engine
 * @date 2024
 */

#include "physics.hpp"
#include "soft_body_physics.hpp" 
#include "fluid_simulation.hpp"
#include "advanced_materials.hpp"
#include "physics_education_tools.hpp"
#include "../ecs/registry.hpp"
#include "../ecs/archetype.hpp"
#include "../memory/arena.hpp"
#include "../memory/pool.hpp"
#include "../work_stealing_job_system.hpp"
#include <memory>
#include <vector>
#include <array>
#include <unordered_map>

namespace ecscope::physics::integration {

// Import necessary types
using namespace ecscope::ecs;
using namespace ecscope::memory;
using namespace ecscope::physics::components;
using namespace ecscope::physics::soft_body;
using namespace ecscope::physics::fluid;
using namespace ecscope::physics::materials;
using namespace ecscope::physics::education;

//=============================================================================
// Advanced Physics Components
//=============================================================================

/**
 * @brief Soft body component for ECS integration
 * 
 * Links entities to soft body particle systems and manages their lifecycle.
 */
struct alignas(32) SoftBodyComponent {
    /** @brief Index into soft body particle system */
    u32 soft_body_id{0};
    
    /** @brief Material properties for this soft body */
    SoftBodyMaterial material{};
    
    /** @brief Particle indices that belong to this entity */
    std::vector<u32> particle_indices;
    
    /** @brief Constraint indices for this soft body */
    std::vector<u32> constraint_indices;
    
    /** @brief Soft body configuration */
    struct {
        u32 resolution_x{10};        ///< Particle resolution in X
        u32 resolution_y{10};        ///< Particle resolution in Y
        f32 rest_distance{0.1f};     ///< Rest distance between particles
        bool generate_bending{true}; ///< Generate bending constraints
        bool generate_diagonal{true}; ///< Generate diagonal constraints
        f32 pressure{0.0f};          ///< Internal pressure (for balloons)
    } config;
    
    /** @brief Current state */
    struct {
        f32 total_mass{0.0f};           ///< Total mass of soft body
        f32 current_volume{0.0f};       ///< Current volume/area
        f32 rest_volume{0.0f};          ///< Rest volume/area
        Vec2 center_of_mass{0.0f, 0.0f}; ///< Current center of mass
        f32 max_stress{0.0f};           ///< Maximum stress in system
        f32 kinetic_energy{0.0f};       ///< Total kinetic energy
        f32 potential_energy{0.0f};     ///< Total potential energy
    } state;
    
    /** @brief Interaction properties */
    struct {
        bool collide_with_rigid_bodies{true};  ///< Interact with rigid bodies
        bool collide_with_fluids{false};       ///< Interact with fluid particles
        bool collide_with_other_soft_bodies{true}; ///< Self and inter soft-body collision
        f32 collision_margin{0.01f};           ///< Collision detection margin
    } interaction;
    
    /** @brief Performance metrics */
    mutable struct {
        u32 active_particles{0};        ///< Currently active particles
        u32 active_constraints{0};      ///< Currently active constraints
        f32 update_time{0.0f};          ///< Last update time in ms
        u32 constraint_iterations{0};   ///< Constraint solver iterations
        f32 convergence_error{0.0f};    ///< Constraint convergence error
    } performance;
    
    /** @brief Default constructor */
    SoftBodyComponent() = default;
    
    /** @brief Constructor with material */
    explicit SoftBodyComponent(const SoftBodyMaterial& mat) : material(mat) {}
    
    /** @brief Initialize rectangular soft body mesh */
    void initialize_rectangular_mesh(const Vec2& size, const Vec2& center) {
        particle_indices.clear();
        constraint_indices.clear();
        
        // Calculate particle spacing
        Vec2 spacing = {size.x / (config.resolution_x - 1), size.y / (config.resolution_y - 1)};
        Vec2 start_pos = center - size * 0.5f;
        
        // Generate particles
        for (u32 y = 0; y < config.resolution_y; ++y) {
            for (u32 x = 0; x < config.resolution_x; ++x) {
                Vec2 pos = start_pos + Vec2{x * spacing.x, y * spacing.y};
                // Particle creation will be handled by the soft body system
                u32 particle_id = y * config.resolution_x + x;
                particle_indices.push_back(particle_id);
            }
        }
        
        // Calculate total mass and rest volume
        f32 particle_mass = material.density * spacing.x * spacing.y * material.thickness;
        state.total_mass = particle_mass * particle_indices.size();
        state.rest_volume = size.x * size.y * material.thickness;
        state.current_volume = state.rest_volume;
        state.center_of_mass = center;
    }
    
    /** @brief Check if component is valid */
    bool is_valid() const noexcept {
        return material.is_valid() && 
               !particle_indices.empty() &&
               state.total_mass > 0.0f &&
               state.rest_volume > 0.0f;
    }
};

/**
 * @brief Fluid component for ECS integration
 */
struct alignas(32) FluidComponent {
    /** @brief Fluid material properties */
    FluidMaterial material{};
    
    /** @brief Fluid particle indices managed by this component */
    std::vector<u32> particle_indices;
    
    /** @brief Fluid region bounds */
    struct {
        Vec2 min_bounds{-10.0f, -10.0f};  ///< Minimum fluid region
        Vec2 max_bounds{10.0f, 10.0f};    ///< Maximum fluid region
        bool enforce_bounds{true};         ///< Remove particles outside bounds
    } region;
    
    /** @brief Emitter properties (if this is a fluid emitter) */
    struct {
        bool is_emitter{false};           ///< Whether this emits fluid particles
        Vec2 emission_point{0.0f, 0.0f};  ///< Emission location
        Vec2 emission_velocity{0.0f, 0.0f}; ///< Initial particle velocity
        f32 emission_rate{10.0f};         ///< Particles per second
        f32 emission_timer{0.0f};         ///< Internal timer for emission
        u32 max_particles{1000};         ///< Maximum particles to emit
        f32 particle_lifetime{10.0f};    ///< Particle lifetime in seconds
    } emitter;
    
    /** @brief Current fluid state */
    struct {
        u32 active_particles{0};         ///< Currently active particles
        f32 total_mass{0.0f};            ///< Total fluid mass
        f32 total_volume{0.0f};          ///< Total fluid volume
        f32 average_density{0.0f};       ///< Average density
        f32 max_velocity{0.0f};          ///< Maximum particle velocity
        f32 max_pressure{0.0f};          ///< Maximum pressure
        Vec2 center_of_mass{0.0f, 0.0f}; ///< Fluid center of mass
        f32 kinetic_energy{0.0f};        ///< Total kinetic energy
    } state;
    
    /** @brief Interaction properties */
    struct {
        bool interact_with_rigid_bodies{true};  ///< Fluid-solid coupling
        bool interact_with_soft_bodies{true};   ///< Fluid-soft coupling
        f32 coupling_strength{1.0f};            ///< Interaction strength
        f32 surface_tension_threshold{0.1f};    ///< Surface tension activation
    } interaction;
    
    /** @brief Performance metrics */
    mutable struct {
        f32 neighbor_search_time{0.0f};   ///< Neighbor finding time
        f32 density_calculation_time{0.0f}; ///< Density computation time
        f32 force_calculation_time{0.0f};  ///< Force computation time
        f32 integration_time{0.0f};        ///< Integration time
        u32 neighbor_checks{0};            ///< Number of neighbor checks
        f32 cache_hit_ratio{0.0f};         ///< Spatial cache hit ratio
    } performance;
    
    /** @brief Default constructor */
    FluidComponent() = default;
    
    /** @brief Constructor with material */
    explicit FluidComponent(const FluidMaterial& mat) : material(mat) {}
    
    /** @brief Initialize fluid region with particles */
    void initialize_fluid_region(const Vec2& region_size, const Vec2& center, f32 particle_spacing) {
        particle_indices.clear();
        
        // Calculate number of particles
        u32 particles_x = static_cast<u32>(region_size.x / particle_spacing);
        u32 particles_y = static_cast<u32>(region_size.y / particle_spacing);
        
        Vec2 start_pos = center - region_size * 0.5f;
        
        // Generate particles in grid pattern
        for (u32 y = 0; y < particles_y; ++y) {
            for (u32 x = 0; x < particles_x; ++x) {
                Vec2 pos = start_pos + Vec2{x * particle_spacing, y * particle_spacing};
                // Add small random offset to prevent grid artifacts
                pos += Vec2{(rand() / f32(RAND_MAX) - 0.5f) * particle_spacing * 0.1f,
                           (rand() / f32(RAND_MAX) - 0.5f) * particle_spacing * 0.1f};
                
                u32 particle_id = y * particles_x + x;
                particle_indices.push_back(particle_id);
            }
        }
        
        // Update state
        state.active_particles = static_cast<u32>(particle_indices.size());
        state.total_mass = state.active_particles * material.particle_mass;
        state.center_of_mass = center;
        
        // Set region bounds
        region.min_bounds = center - region_size * 0.6f; // Slightly larger for movement
        region.max_bounds = center + region_size * 0.6f;
    }
    
    /** @brief Setup as fluid emitter */
    void setup_emitter(const Vec2& position, const Vec2& velocity, f32 rate, u32 max_count) {
        emitter.is_emitter = true;
        emitter.emission_point = position;
        emitter.emission_velocity = velocity;
        emitter.emission_rate = rate;
        emitter.max_particles = max_count;
    }
    
    /** @brief Check if component is valid */
    bool is_valid() const noexcept {
        return material.is_valid();
    }
};

/**
 * @brief Advanced material component
 */
struct alignas(32) AdvancedMaterialComponent {
    /** @brief Advanced material properties */
    AdvancedMaterial material{};
    
    /** @brief Current material state */
    struct {
        f32 current_temperature{293.15f}; ///< Current temperature in K
        f32 damage_level{0.0f};           ///< Damage accumulation [0-1]
        f32 plastic_strain{0.0f};         ///< Accumulated plastic strain
        u32 fatigue_cycles{0};            ///< Number of fatigue cycles
        f32 max_stress_experienced{0.0f}; ///< Maximum stress seen
        f32 corrosion_depth{0.0f};        ///< Corrosion penetration depth
    } state;
    
    /** @brief Environmental conditions */
    struct {
        f32 ambient_temperature{293.15f}; ///< Ambient temperature
        f32 humidity{0.5f};               ///< Relative humidity [0-1]
        f32 uv_exposure{0.0f};            ///< UV exposure level
        f32 chemical_concentration{0.0f}; ///< Corrosive chemical concentration
        f32 radiation_level{0.0f};        ///< Radiation exposure
    } environment;
    
    /** @brief Failure tracking */
    struct {
        bool has_failed{false};           ///< Material has failed
        f32 time_to_failure{0.0f};        ///< Predicted time to failure
        std::string failure_mode;          ///< Description of failure mode
        f32 reliability{1.0f};            ///< Current reliability [0-1]
    } failure;
    
    /** @brief Default constructor */
    AdvancedMaterialComponent() = default;
    
    /** @brief Constructor with material */
    explicit AdvancedMaterialComponent(const AdvancedMaterial& mat) : material(mat) {}
    
    /** @brief Update material state based on current conditions */
    void update_material_state(f32 delta_time, f32 applied_stress) {
        // Update temperature effects
        if (material.material_flags.is_temperature_dependent) {
            // Simple thermal diffusion (needs proper heat transfer for accuracy)
            f32 temp_diff = environment.ambient_temperature - state.current_temperature;
            state.current_temperature += temp_diff * 0.1f * delta_time; // Simplified
        }
        
        // Update damage based on stress
        if (applied_stress > material.get_yield_strength(state.current_temperature)) {
            f32 stress_ratio = applied_stress / material.get_yield_strength(state.current_temperature);
            f32 damage_increment = material.damage_model.damage_rate * 
                                  (stress_ratio - material.damage_model.damage_threshold) * delta_time;
            state.damage_level += damage_increment;
            state.damage_level = std::clamp(state.damage_level, 0.0f, 1.0f);
        }
        
        // Update plastic strain
        if (applied_stress > material.get_yield_strength(state.current_temperature) && 
            !material.material_flags.is_brittle) {
            f32 yield_stress = material.get_yield_strength(state.current_temperature);
            f32 plastic_stress = applied_stress - yield_stress;
            f32 E = material.get_youngs_modulus(Vec2{1.0f, 0.0f}, state.current_temperature);
            state.plastic_strain += plastic_stress / E * delta_time;
        }
        
        // Update maximum stress
        state.max_stress_experienced = std::max(state.max_stress_experienced, applied_stress);
        
        // Check for failure
        if (state.damage_level >= material.damage_model.critical_damage) {
            failure.has_failed = true;
            failure.failure_mode = material.material_flags.is_brittle ? "Brittle Fracture" : "Ductile Failure";
        }
        
        // Update reliability based on damage
        failure.reliability = 1.0f - state.damage_level;
        
        // Update material properties based on current state
        material.update_properties(state.current_temperature, state.damage_level);
    }
    
    /** @brief Get current effective properties */
    f32 get_effective_youngs_modulus(const Vec2& direction) const noexcept {
        f32 base_modulus = material.get_youngs_modulus(direction, state.current_temperature);
        f32 damage_factor = 1.0f - state.damage_level;
        return base_modulus * damage_factor;
    }
    
    /** @brief Check if material has failed */
    bool has_failed() const noexcept {
        return failure.has_failed || state.damage_level >= 1.0f;
    }
    
    /** @brief Predict remaining life */
    f32 predict_remaining_life(f32 current_stress_level) const noexcept {
        if (has_failed()) return 0.0f;
        
        f32 remaining_damage = material.damage_model.critical_damage - state.damage_level;
        if (remaining_damage <= 0.0f) return 0.0f;
        
        f32 damage_rate = material.damage_model.damage_rate * 
                         (current_stress_level / material.get_yield_strength(state.current_temperature));
        
        return (damage_rate > 0.0f) ? remaining_damage / damage_rate : 1e6f; // Very long life
    }
    
    /** @brief Check if component is valid */
    bool is_valid() const noexcept {
        return material.is_valid() && 
               state.damage_level >= 0.0f && state.damage_level <= 1.0f &&
               state.current_temperature > 0.0f;
    }
};

/**
 * @brief Educational debug component for enhanced learning
 */
struct alignas(16) PhysicsEducationComponent {
    /** @brief What educational features are enabled for this entity */
    union {
        u32 features;
        struct {
            u32 show_force_vectors : 1;        ///< Show force visualization
            u32 show_velocity_vectors : 1;     ///< Show velocity visualization
            u32 show_acceleration_vectors : 1; ///< Show acceleration visualization
            u32 show_particle_trails : 1;      ///< Show motion trails
            u32 show_stress_visualization : 1; ///< Show stress/strain
            u32 show_energy_display : 1;       ///< Show energy values
            u32 show_material_properties : 1;  ///< Show material info
            u32 show_performance_metrics : 1;  ///< Show performance data
            u32 interactive_parameters : 1;    ///< Allow parameter tweaking
            u32 step_by_step_analysis : 1;     ///< Enable algorithm stepping
            u32 reserved : 22;
        };
    } education_flags{0xFF}; // Most features enabled by default
    
    /** @brief Educational annotations for this entity */
    std::vector<std::string> annotations;
    
    /** @brief Custom visualization colors */
    struct {
        colors::Color force_color{colors::Color::red()};
        colors::Color velocity_color{colors::Color::blue()};
        colors::Color acceleration_color{colors::Color::green()};
        colors::Color trail_color{colors::Color::white()};
    } visualization_colors;
    
    /** @brief Educational metrics specific to this entity */
    struct {
        f32 total_work_done{0.0f};          ///< Total work done on entity
        f32 energy_dissipated{0.0f};        ///< Energy lost to damping/friction
        f32 efficiency{1.0f};               ///< Energy efficiency ratio
        u32 collision_count{0};             ///< Number of collisions
        f32 average_velocity{0.0f};         ///< Time-averaged velocity
        f32 maximum_acceleration{0.0f};     ///< Peak acceleration experienced
    } educational_metrics;
    
    /** @brief Add educational annotation */
    void add_annotation(std::string text) {
        annotations.push_back(std::move(text));
    }
    
    /** @brief Clear all annotations */
    void clear_annotations() {
        annotations.clear();
    }
    
    /** @brief Update educational metrics */
    void update_metrics(const Vec2& velocity, const Vec2& acceleration, f32 delta_time) {
        educational_metrics.average_velocity = 
            0.9f * educational_metrics.average_velocity + 0.1f * velocity.length();
        educational_metrics.maximum_acceleration = 
            std::max(educational_metrics.maximum_acceleration, acceleration.length());
    }
};

//=============================================================================
// Static Component Validation
//=============================================================================

// Ensure all components satisfy ECS requirements
static_assert(Component<SoftBodyComponent>, "SoftBodyComponent must satisfy Component concept");
static_assert(Component<FluidComponent>, "FluidComponent must satisfy Component concept");
static_assert(Component<AdvancedMaterialComponent>, "AdvancedMaterialComponent must satisfy Component concept");
static_assert(Component<PhysicsEducationComponent>, "PhysicsEducationComponent must satisfy Component concept");

// Verify alignment for performance
static_assert(alignof(SoftBodyComponent) >= 32, "SoftBodyComponent should be 32-byte aligned");
static_assert(alignof(FluidComponent) >= 32, "FluidComponent should be 32-byte aligned");
static_assert(alignof(AdvancedMaterialComponent) >= 32, "AdvancedMaterialComponent should be 32-byte aligned");

//=============================================================================
// Integrated Physics System
//=============================================================================

/**
 * @brief Master physics system that integrates all advanced physics features
 * 
 * This system orchestrates rigid bodies, soft bodies, fluids, materials,
 * and educational features while maintaining high performance and
 * educational value.
 */
class IntegratedPhysicsSystem {
public:
    /** @brief System configuration */
    struct Configuration {
        // Performance settings
        bool enable_multi_threading{true};
        u32 thread_count{0}; // 0 = auto-detect
        u32 max_rigid_bodies{1000};
        u32 max_soft_body_particles{500};
        u32 max_fluid_particles{10000};
        
        // Integration settings
        f32 fixed_time_step{1.0f / 60.0f};
        u32 max_substeps{4};
        bool enable_adaptive_timestep{true};
        
        // Feature enables
        bool enable_soft_bodies{true};
        bool enable_fluids{true};
        bool enable_advanced_materials{true};
        bool enable_educational_features{true};
        
        // Memory settings
        usize soft_body_pool_size{1024 * 1024};     // 1MB
        usize fluid_pool_size{4 * 1024 * 1024};     // 4MB
        usize material_pool_size{512 * 1024};       // 512KB
        usize education_pool_size{256 * 1024};      // 256KB
        
        // Educational settings
        bool enable_real_time_visualization{true};
        bool enable_interactive_parameters{true};
        bool enable_performance_monitoring{true};
        u32 visualization_grid_resolution{64};
        
        static Configuration create_performance_focused() {
            Configuration config;
            config.enable_educational_features = false;
            config.enable_real_time_visualization = false;
            config.enable_interactive_parameters = false;
            config.thread_count = std::thread::hardware_concurrency();
            return config;
        }
        
        static Configuration create_educational_focused() {
            Configuration config;
            config.enable_educational_features = true;
            config.enable_real_time_visualization = true;
            config.enable_interactive_parameters = true;
            config.enable_performance_monitoring = true;
            config.max_rigid_bodies = 100; // Fewer for better visualization
            config.max_soft_body_particles = 100;
            config.max_fluid_particles = 1000;
            return config;
        }
    };

private:
    // Core systems
    Registry* registry_;
    Configuration config_;
    std::unique_ptr<PhysicsSystem> base_physics_system_;
    
    // Advanced physics data
    std::vector<SoftBodyParticle> soft_body_particles_;
    std::vector<std::unique_ptr<SoftBodyConstraint>> soft_body_constraints_;
    std::vector<FluidParticle> fluid_particles_;
    MaterialDatabase material_database_;
    
    // Educational tools
    std::unique_ptr<PhysicsEducationManager> education_manager_;
    
    // Memory management
    std::unique_ptr<Pool<SoftBodyParticle>> soft_body_particle_pool_;
    std::unique_ptr<Pool<FluidParticle>> fluid_particle_pool_;
    std::unique_ptr<Arena> constraint_arena_;
    
    // Performance tracking
    struct PerformanceData {
        f64 total_frame_time{0.0};
        f64 rigid_body_time{0.0};
        f64 soft_body_time{0.0};
        f64 fluid_time{0.0};
        f64 material_update_time{0.0};
        f64 visualization_time{0.0};
        u32 frame_count{0};
        
        void reset() {
            total_frame_time = rigid_body_time = soft_body_time = 0.0;
            fluid_time = material_update_time = visualization_time = 0.0;
            frame_count = 0;
        }
        
        f64 get_average_frame_time() const {
            return frame_count > 0 ? total_frame_time / frame_count : 0.0;
        }
    } performance_data_;
    
    // Threading
    std::unique_ptr<WorkStealingJobSystem> job_system_;
    
public:
    /** @brief Constructor */
    explicit IntegratedPhysicsSystem(Registry& registry, const Configuration& config = Configuration{});
    
    /** @brief Destructor */
    ~IntegratedPhysicsSystem();
    
    // Disable copy/move for now
    IntegratedPhysicsSystem(const IntegratedPhysicsSystem&) = delete;
    IntegratedPhysicsSystem& operator=(const IntegratedPhysicsSystem&) = delete;
    
    //-------------------------------------------------------------------------
    // System Lifecycle
    //-------------------------------------------------------------------------
    
    /** @brief Initialize all physics systems */
    bool initialize();
    
    /** @brief Main update loop */
    void update(f32 delta_time);
    
    /** @brief Cleanup and shutdown */
    void shutdown();
    
    //-------------------------------------------------------------------------
    // Entity Management
    //-------------------------------------------------------------------------
    
    /** @brief Create soft body entity */
    Entity create_soft_body(const SoftBodyMaterial& material, const Vec2& position, const Vec2& size);
    
    /** @brief Create fluid region */
    Entity create_fluid_region(const FluidMaterial& material, const Vec2& position, 
                              const Vec2& size, f32 particle_spacing = 0.1f);
    
    /** @brief Create fluid emitter */
    Entity create_fluid_emitter(const FluidMaterial& material, const Vec2& position,
                                const Vec2& velocity, f32 emission_rate = 10.0f);
    
    /** @brief Add advanced material to existing entity */
    void add_advanced_material(Entity entity, const AdvancedMaterial& material);
    
    /** @brief Add educational features to entity */
    void add_educational_features(Entity entity, u32 feature_flags = 0xFF);
    
    //-------------------------------------------------------------------------
    // System Access
    //-------------------------------------------------------------------------
    
    /** @brief Get base physics system */
    PhysicsSystem* get_base_physics_system() { return base_physics_system_.get(); }
    
    /** @brief Get education manager */
    PhysicsEducationManager* get_education_manager() { return education_manager_.get(); }
    
    /** @brief Get material database */
    MaterialDatabase* get_material_database() { return &material_database_; }
    
    /** @brief Get configuration */
    const Configuration& get_configuration() const noexcept { return config_; }
    
    //-------------------------------------------------------------------------
    // Performance and Statistics
    //-------------------------------------------------------------------------
    
    /** @brief Get performance statistics */
    const PerformanceData& get_performance_data() const noexcept { return performance_data_; }
    
    /** @brief Generate comprehensive performance report */
    std::string generate_performance_report() const;
    
    /** @brief Get memory usage statistics */
    struct MemoryUsage {
        usize total_physics_memory{0};
        usize rigid_body_memory{0};
        usize soft_body_memory{0};
        usize fluid_memory{0};
        usize material_memory{0};
        usize education_memory{0};
        f32 memory_utilization{0.0f}; // 0-100%
    };
    
    MemoryUsage get_memory_usage() const;
    
    /** @brief Get current entity counts */
    struct EntityCounts {
        u32 rigid_bodies{0};
        u32 soft_bodies{0};
        u32 fluid_regions{0};
        u32 fluid_emitters{0};
        u32 advanced_materials{0};
        u32 educational_entities{0};
        u32 total_particles{0};
        u32 total_constraints{0};
    };
    
    EntityCounts get_entity_counts() const;
    
    /** @brief Check if system is meeting performance targets */
    bool is_meeting_performance_targets() const noexcept {
        f64 target_frame_time = 1000.0 / 60.0; // 60 FPS = ~16.67ms
        return performance_data_.get_average_frame_time() <= target_frame_time;
    }
    
    //-------------------------------------------------------------------------
    // Educational Features
    //-------------------------------------------------------------------------
    
    /** @brief Enable/disable educational mode */
    void set_educational_mode(bool enabled);
    
    /** @brief Start algorithm stepping for educational analysis */
    void start_algorithm_stepping(const std::string& algorithm_name);
    
    /** @brief Add educational parameter group */
    ParameterGroup* add_parameter_group(const std::string& group_name);
    
    /** @brief Generate educational report */
    std::string generate_educational_report() const;
    
private:
    //-------------------------------------------------------------------------
    // Internal Update Methods
    //-------------------------------------------------------------------------
    
    /** @brief Update rigid body physics */
    void update_rigid_bodies(f32 delta_time);
    
    /** @brief Update soft body physics */
    void update_soft_bodies(f32 delta_time);
    
    /** @brief Update fluid simulation */
    void update_fluids(f32 delta_time);
    
    /** @brief Update material properties */
    void update_materials(f32 delta_time);
    
    /** @brief Update educational visualizations */
    void update_educational_features(f32 delta_time);
    
    /** @brief Handle interactions between different physics systems */
    void update_cross_system_interactions(f32 delta_time);
    
    //-------------------------------------------------------------------------
    // Memory Management
    //-------------------------------------------------------------------------
    
    /** @brief Initialize memory pools */
    bool initialize_memory_pools();
    
    /** @brief Cleanup memory pools */
    void cleanup_memory_pools();
    
    //-------------------------------------------------------------------------
    // Threading Support
    //-------------------------------------------------------------------------
    
    /** @brief Initialize job system */
    bool initialize_job_system();
    
    /** @brief Cleanup job system */
    void cleanup_job_system();
    
    /** @brief Distribute work across threads */
    void schedule_parallel_work(f32 delta_time);
    
    //-------------------------------------------------------------------------
    // Utility Methods
    //-------------------------------------------------------------------------
    
    /** @brief Update performance metrics */
    void update_performance_metrics(f64 frame_time);
    
    /** @brief Validate system state */
    bool validate_system_state() const;
    
    /** @brief Handle system errors */
    void handle_system_error(const std::string& error_message);
};

//=============================================================================
// Utility Functions
//=============================================================================

namespace utils {
    /**
     * @brief Create a complete physics scene with mixed content
     */
    void create_mixed_physics_scene(IntegratedPhysicsSystem& physics_system, Registry& registry);
    
    /**
     * @brief Create educational demonstration scene
     */
    void create_educational_demo_scene(IntegratedPhysicsSystem& physics_system, Registry& registry);
    
    /**
     * @brief Create performance benchmark scene
     */
    void create_benchmark_scene(IntegratedPhysicsSystem& physics_system, Registry& registry,
                               u32 rigid_body_count, u32 soft_body_count, u32 fluid_particle_count);
    
    /**
     * @brief Analyze physics system performance and suggest optimizations
     */
    struct OptimizationSuggestions {
        std::vector<std::string> suggestions;
        f32 expected_performance_gain{0.0f}; // Percentage
        bool requires_quality_trade_off{false};
    };
    
    OptimizationSuggestions analyze_performance_and_suggest_optimizations(
        const IntegratedPhysicsSystem& physics_system);
}

} // namespace ecscope::physics::integration