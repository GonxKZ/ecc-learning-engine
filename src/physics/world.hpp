#pragma once

/**
 * @file physics/world.hpp
 * @brief PhysicsWorld2D - Core Physics Simulation Coordinator for ECScope Phase 5: Física 2D
 * 
 * This header implements the central physics world system that orchestrates all physics
 * simulation in the ECS. The PhysicsWorld2D acts as the main coordinator between:
 * 
 * - Physics Components (RigidBody2D, Collider2D, ForceAccumulator, etc.)
 * - ECS Registry and entity management
 * - Memory management systems (Arena/Pool allocators)
 * - Educational debugging and visualization tools
 * - Performance monitoring and optimization
 * 
 * Architecture Overview:
 * - Data-oriented physics world as ECS system coordinator
 * - Spatial hashing for broad-phase collision detection
 * - Sequential impulse solver with educational visualization
 * - Semi-implicit Euler integration for stability + performance balance
 * - Integration with existing ECS Registry and memory management
 * 
 * Educational Features:
 * - Step-by-step physics pipeline visualization
 * - Real-time performance metrics and comparisons
 * - Interactive physics parameter tuning
 * - Educational explanations of physics concepts
 * - Memory usage analysis and optimization insights
 * 
 * Performance Optimizations:
 * - Spatial partitioning for efficient collision detection
 * - Memory-optimized data structures using custom allocators
 * - Cache-friendly component access patterns
 * - SIMD-optimized vector operations where beneficial
 * - Sleeping system for inactive objects
 * 
 * @author ECScope Educational ECS Framework - Phase 5: Física 2D
 * @date 2024
 */

#include "components.hpp"
#include "math.hpp"
#include "../ecs/registry.hpp"
#include "../memory/arena.hpp"
#include "../memory/pool.hpp"
#include "../memory/memory_tracker.hpp"
#include "../core/types.hpp"
#include "../core/log.hpp"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <chrono>
#include <functional>
#include <span>
#include <optional>

namespace ecscope::physics {

// Import commonly used types
using namespace ecscope::physics::math;
using namespace ecscope::physics::components;
using namespace ecscope::ecs;

//=============================================================================
// Physics World Configuration
//=============================================================================

/**
 * @brief Configuration for PhysicsWorld2D simulation parameters
 * 
 * This structure contains all the tunable parameters for the physics simulation.
 * Each parameter includes educational documentation explaining its purpose and
 * typical values for different types of simulations.
 */
struct PhysicsWorldConfig {
    //-------------------------------------------------------------------------
    // Simulation Parameters
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Gravity vector in m/s²
     * 
     * Educational Context:
     * Earth gravity: (0, -9.81) m/s²
     * Moon gravity: (0, -1.62) m/s²
     * Zero gravity: (0, 0) m/s²
     * Custom gravity: any direction and magnitude
     */
    Vec2 gravity{0.0f, -9.81f};
    
    /** 
     * @brief Physics time step in seconds
     * 
     * Fixed time step ensures consistent simulation regardless of frame rate.
     * Common values:
     * - 1/60 = 0.0166s (60 FPS equivalent)
     * - 1/120 = 0.0083s (120 FPS, more accurate)
     * - 1/240 = 0.0041s (240 FPS, very accurate but expensive)
     */
    f32 time_step{1.0f / 60.0f};
    
    /** 
     * @brief Maximum time accumulation for fixed timestep
     * 
     * Prevents "spiral of death" when simulation can't keep up with real time.
     */
    f32 max_time_accumulator{0.25f};
    
    /** 
     * @brief Number of constraint solver iterations
     * 
     * More iterations = more accurate constraint resolution but higher CPU cost.
     * Educational range: 1-20 iterations
     * - 1-4: Fast but less accurate
     * - 6-10: Good balance for most simulations  
     * - 15-20: High accuracy for complex constraint systems
     */
    u32 constraint_iterations{8};
    
    /** 
     * @brief Number of velocity iterations for constraint solving
     * 
     * Separate from position iterations for better convergence.
     */
    u32 velocity_iterations{6};
    
    //-------------------------------------------------------------------------
    // Collision Detection Parameters
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Spatial hash cell size for broad-phase collision detection
     * 
     * Should be roughly 2-4x the size of typical objects in your simulation.
     * Larger cells = fewer cells to check but more objects per cell
     * Smaller cells = more cells but fewer objects per cell
     */
    f32 spatial_hash_cell_size{10.0f};
    
    /** 
     * @brief Initial capacity for spatial hash grid
     * 
     * Pre-allocate space to avoid dynamic allocations during simulation.
     */
    u32 spatial_hash_initial_capacity{1024};
    
    /** 
     * @brief Enable continuous collision detection (CCD)
     * 
     * Prevents fast objects from tunneling through thin obstacles.
     * More expensive but necessary for high-speed objects.
     */
    bool enable_continuous_collision{false};
    
    /** 
     * @brief Velocity threshold for CCD activation
     * 
     * Only objects moving faster than this will use CCD.
     */
    f32 ccd_velocity_threshold{20.0f};
    
    //-------------------------------------------------------------------------
    // Performance Optimization Parameters
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Enable sleeping system for inactive objects
     * 
     * Objects below velocity threshold for sufficient time go to sleep,
     * significantly reducing CPU usage for large numbers of objects.
     */
    bool enable_sleeping{true};
    
    /** 
     * @brief Linear velocity threshold for sleeping
     */
    f32 sleep_linear_velocity_threshold{0.01f};
    
    /** 
     * @brief Angular velocity threshold for sleeping
     */
    f32 sleep_angular_velocity_threshold{0.01f};
    
    /** 
     * @brief Time objects must be slow before sleeping (seconds)
     */
    f32 sleep_time_threshold{1.0f};
    
    /** 
     * @brief Maximum number of active rigid bodies to simulate
     * 
     * Performance limit - objects beyond this may be skipped or simplified.
     */
    u32 max_active_bodies{2000};
    
    /** 
     * @brief Enable multithreading for physics simulation
     * 
     * Educational note: Demonstrates challenges of parallel physics simulation.
     */
    bool enable_multithreading{false};
    
    /** 
     * @brief Number of worker threads for physics (0 = auto-detect)
     */
    u32 worker_thread_count{0};
    
    //-------------------------------------------------------------------------
    // Educational and Debugging Parameters
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Enable detailed performance profiling
     * 
     * Tracks timing for each physics subsystem for educational analysis.
     */
    bool enable_profiling{true};
    
    /** 
     * @brief Enable step-by-step physics visualization
     * 
     * Allows students to see each stage of the physics pipeline.
     */
    bool enable_step_visualization{false};
    
    /** 
     * @brief Enable collision shape debug rendering
     */
    bool debug_render_collision_shapes{false};
    
    /** 
     * @brief Enable contact point debug rendering
     */
    bool debug_render_contact_points{false};
    
    /** 
     * @brief Enable force vector debug rendering
     */
    bool debug_render_forces{false};
    
    /** 
     * @brief Enable velocity vector debug rendering
     */
    bool debug_render_velocities{false};
    
    /** 
     * @brief Enable spatial partitioning debug rendering
     */
    bool debug_render_spatial_hash{false};
    
    /** 
     * @brief Maximum number of debug elements to render (performance limit)
     */
    u32 max_debug_elements{1000};
    
    //-------------------------------------------------------------------------
    // Memory Management Configuration
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Arena allocator size for physics data (bytes)
     * 
     * Used for collision detection working memory, contact manifolds, etc.
     */
    usize physics_arena_size{8 * 1024 * 1024};  // 8 MB
    
    /** 
     * @brief Pool allocator capacity for contact points
     */
    u32 contact_pool_capacity{10000};
    
    /** 
     * @brief Pool allocator capacity for collision pairs
     */
    u32 collision_pair_pool_capacity{5000};
    
    /** 
     * @brief Enable memory usage tracking and analysis
     */
    bool enable_memory_tracking{true};
    
    //-------------------------------------------------------------------------
    // Factory Methods for Common Configurations
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Configuration optimized for educational demonstrations
     */
    static PhysicsWorldConfig create_educational() {
        PhysicsWorldConfig config;
        config.time_step = 1.0f / 60.0f;
        config.constraint_iterations = 6;
        config.enable_profiling = true;
        config.enable_step_visualization = true;
        config.debug_render_collision_shapes = true;
        config.debug_render_contact_points = true;
        config.debug_render_forces = true;
        config.spatial_hash_cell_size = 8.0f;
        config.max_active_bodies = 500;
        return config;
    }
    
    /** 
     * @brief Configuration optimized for high performance
     */
    static PhysicsWorldConfig create_performance() {
        PhysicsWorldConfig config;
        config.time_step = 1.0f / 120.0f;
        config.constraint_iterations = 10;
        config.velocity_iterations = 8;
        config.enable_profiling = false;
        config.enable_step_visualization = false;
        config.enable_sleeping = true;
        config.enable_multithreading = true;
        config.max_active_bodies = 5000;
        config.physics_arena_size = 16 * 1024 * 1024;  // 16 MB
        return config;
    }
    
    /** 
     * @brief Configuration for accuracy-focused simulation
     */
    static PhysicsWorldConfig create_high_accuracy() {
        PhysicsWorldConfig config;
        config.time_step = 1.0f / 240.0f;
        config.constraint_iterations = 15;
        config.velocity_iterations = 12;
        config.enable_continuous_collision = true;
        config.ccd_velocity_threshold = 10.0f;
        config.sleep_linear_velocity_threshold = 0.001f;
        config.sleep_angular_velocity_threshold = 0.001f;
        return config;
    }
    
    /** 
     * @brief Minimal configuration for debugging
     */
    static PhysicsWorldConfig create_minimal() {
        PhysicsWorldConfig config;
        config.time_step = 1.0f / 30.0f;
        config.constraint_iterations = 3;
        config.velocity_iterations = 3;
        config.max_active_bodies = 100;
        config.spatial_hash_cell_size = 20.0f;
        config.physics_arena_size = 1024 * 1024;  // 1 MB
        return config;
    }
};

//=============================================================================
// Physics World Statistics and Profiling
//=============================================================================

/**
 * @brief Comprehensive physics simulation statistics
 * 
 * Provides detailed metrics for educational analysis and performance optimization.
 */
struct PhysicsWorldStats {
    //-------------------------------------------------------------------------
    // Simulation State
    //-------------------------------------------------------------------------
    f32 current_time{0.0f};              // Current simulation time
    u64 total_steps{0};                  // Total physics steps simulated
    f32 time_accumulator{0.0f};          // Accumulated time for fixed timestep
    
    //-------------------------------------------------------------------------
    // Entity Counts
    //-------------------------------------------------------------------------
    u32 total_rigid_bodies{0};           // Total rigid bodies in world
    u32 active_rigid_bodies{0};          // Currently active (not sleeping)
    u32 sleeping_rigid_bodies{0};        // Currently sleeping
    u32 static_bodies{0};                // Static/kinematic bodies
    u32 total_colliders{0};              // Total colliders in world
    u32 trigger_colliders{0};            // Trigger-only colliders
    
    //-------------------------------------------------------------------------
    // Collision Detection Statistics
    //-------------------------------------------------------------------------
    u32 broad_phase_pairs{0};            // Pairs found in broad phase
    u32 narrow_phase_tests{0};           // Narrow phase tests performed
    u32 active_contacts{0};              // Currently active contact points
    u32 new_contacts{0};                 // New contacts this frame
    u32 persistent_contacts{0};          // Contacts from previous frame
    u32 contact_cache_hits{0};           // Contact cache hits
    u32 contact_cache_misses{0};         // Contact cache misses
    
    //-------------------------------------------------------------------------
    // Spatial Partitioning Statistics
    //-------------------------------------------------------------------------
    u32 spatial_hash_cells_used{0};      // Number of active spatial hash cells
    u32 spatial_hash_total_cells{0};     // Total spatial hash grid size
    f32 spatial_hash_occupancy{0.0f};    // Percentage of cells occupied
    f32 average_objects_per_cell{0.0f};  // Average objects per occupied cell
    u32 max_objects_per_cell{0};         // Maximum objects in any cell
    
    //-------------------------------------------------------------------------
    // Constraint Solving Statistics
    //-------------------------------------------------------------------------
    u32 constraints_solved{0};           // Total constraints solved this step
    u32 constraint_islands{0};           // Number of constraint islands
    f32 average_iterations_per_island{0.0f}; // Average solver iterations per island
    u32 max_iterations_used{0};          // Maximum iterations used by any island
    f32 constraint_residual{0.0f};       // Final constraint violation residual
    
    //-------------------------------------------------------------------------
    // Performance Timing (in milliseconds)
    //-------------------------------------------------------------------------
    f64 total_frame_time{0.0};           // Total physics frame time
    f64 broad_phase_time{0.0};           // Time spent in broad phase
    f64 narrow_phase_time{0.0};          // Time spent in narrow phase
    f64 constraint_solve_time{0.0};      // Time spent solving constraints
    f64 integration_time{0.0};           // Time spent integrating motion
    f64 sleeping_update_time{0.0};       // Time spent updating sleeping state
    
    //-------------------------------------------------------------------------
    // Memory Usage Statistics
    //-------------------------------------------------------------------------
    usize total_physics_memory{0};       // Total memory used by physics
    usize arena_memory_used{0};          // Arena allocator memory used
    usize arena_memory_peak{0};          // Peak arena memory usage
    usize pool_memory_used{0};           // Pool allocator memory used
    usize contact_pool_usage{0};         // Contact point pool usage
    usize collision_pair_pool_usage{0}; // Collision pair pool usage
    
    //-------------------------------------------------------------------------
    // Educational Metrics
    //-------------------------------------------------------------------------
    f32 total_kinetic_energy{0.0f};      // Total kinetic energy in system
    f32 total_potential_energy{0.0f};    // Total potential energy in system
    f32 energy_conservation_error{0.0f}; // Energy conservation violation
    Vec2 total_momentum{0.0f, 0.0f};     // Total linear momentum
    f32 total_angular_momentum{0.0f};    // Total angular momentum
    Vec2 momentum_conservation_error{0.0f, 0.0f}; // Momentum conservation violation
    
    //-------------------------------------------------------------------------
    // Performance Metrics
    //-------------------------------------------------------------------------
    f32 fps_equivalent{0.0f};            // What FPS this physics time represents
    f32 cpu_usage_percentage{0.0f};      // Estimated CPU usage percentage
    f32 memory_efficiency{0.0f};         // Memory usage efficiency (0-1)
    f32 cache_hit_ratio{0.0f};          // Cache hit ratio (0-1)
    f32 performance_score{0.0f};         // Overall performance score (0-100)
    
    //-------------------------------------------------------------------------
    // Utility Methods
    //-------------------------------------------------------------------------
    
    /** @brief Reset all statistics */
    void reset() {
        *this = PhysicsWorldStats{};
    }
    
    /** @brief Calculate derived statistics */
    void update_derived_stats() {
        // Spatial hash metrics
        if (spatial_hash_total_cells > 0) {
            spatial_hash_occupancy = static_cast<f32>(spatial_hash_cells_used) / spatial_hash_total_cells;
        }
        
        if (spatial_hash_cells_used > 0) {
            average_objects_per_cell = static_cast<f32>(total_colliders) / spatial_hash_cells_used;
        }
        
        // Performance metrics
        if (total_frame_time > 0.0) {
            fps_equivalent = 1000.0f / static_cast<f32>(total_frame_time);
        }
        
        // Cache efficiency
        u32 total_cache_ops = contact_cache_hits + contact_cache_misses;
        if (total_cache_ops > 0) {
            cache_hit_ratio = static_cast<f32>(contact_cache_hits) / total_cache_ops;
        }
        
        // Memory efficiency (simple heuristic)
        if (arena_memory_peak > 0) {
            memory_efficiency = static_cast<f32>(arena_memory_used) / arena_memory_peak;
        }
        
        // Overall performance score (0-100)
        performance_score = std::min(100.0f, 
            cache_hit_ratio * 30.0f + 
            memory_efficiency * 20.0f + 
            (fps_equivalent > 60.0f ? 30.0f : fps_equivalent * 0.5f) +
            (constraint_residual < 0.001f ? 20.0f : 20.0f * (1.0f - std::min(1.0f, constraint_residual * 1000.0f)))
        );
    }
    
    /** @brief Generate comprehensive report */
    std::string generate_report() const {
        std::ostringstream oss;
        
        oss << "=== Physics World Statistics ===\n";
        oss << "Simulation Time: " << current_time << "s (Steps: " << total_steps << ")\n";
        oss << "Active Bodies: " << active_rigid_bodies << "/" << total_rigid_bodies;
        oss << " (Sleeping: " << sleeping_rigid_bodies << ")\n";
        oss << "Contacts: " << active_contacts << " (" << new_contacts << " new, " 
            << persistent_contacts << " persistent)\n";
        
        oss << "\n--- Performance ---\n";
        oss << "Frame Time: " << total_frame_time << "ms (FPS equivalent: " << fps_equivalent << ")\n";
        oss << "  - Broad Phase: " << broad_phase_time << "ms\n";
        oss << "  - Narrow Phase: " << narrow_phase_time << "ms\n";
        oss << "  - Constraint Solving: " << constraint_solve_time << "ms\n";
        oss << "  - Integration: " << integration_time << "ms\n";
        oss << "Performance Score: " << performance_score << "/100\n";
        
        oss << "\n--- Memory Usage ---\n";
        oss << "Total Physics Memory: " << (total_physics_memory / 1024) << " KB\n";
        oss << "Arena Usage: " << (arena_memory_used / 1024) << " KB / " 
            << (arena_memory_peak / 1024) << " KB (Efficiency: " 
            << (memory_efficiency * 100.0f) << "%)\n";
        oss << "Cache Hit Ratio: " << (cache_hit_ratio * 100.0f) << "%\n";
        
        oss << "\n--- Educational Metrics ---\n";
        oss << "Total Energy: " << (total_kinetic_energy + total_potential_energy) << " J";
        oss << " (KE: " << total_kinetic_energy << ", PE: " << total_potential_energy << ")\n";
        oss << "Energy Conservation Error: " << energy_conservation_error << " J\n";
        oss << "Momentum: (" << total_momentum.x << ", " << total_momentum.y << ")";
        oss << " Conservation Error: (" << momentum_conservation_error.x << ", " 
            << momentum_conservation_error.y << ")\n";
        
        return oss.str();
    }
};

//=============================================================================
// Physics World Events and Callbacks
//=============================================================================

/**
 * @brief Event types for physics simulation callbacks
 */
enum class PhysicsEventType {
    CollisionEnter,     // Two objects start colliding
    CollisionStay,      // Two objects continue colliding
    CollisionExit,      // Two objects stop colliding
    TriggerEnter,       // Object enters trigger
    TriggerStay,        // Object stays in trigger
    TriggerExit,        // Object exits trigger
    BodySleep,          // Body goes to sleep
    BodyWake,           // Body wakes up
    ConstraintBreak,    // Constraint breaks
    ForceApplied        // Force is applied to body
};

/**
 * @brief Physics event data
 */
struct PhysicsEvent {
    PhysicsEventType type;
    Entity entity_a{0};
    Entity entity_b{0};
    Vec2 contact_point{0.0f, 0.0f};
    Vec2 contact_normal{0.0f, 0.0f};
    f32 impulse_magnitude{0.0f};
    f32 timestamp{0.0f};
    void* user_data{nullptr};
    
    PhysicsEvent(PhysicsEventType t, Entity a, Entity b = 0) 
        : type(t), entity_a(a), entity_b(b) {}
};

/**
 * @brief Callback function type for physics events
 */
using PhysicsEventCallback = std::function<void(const PhysicsEvent&)>;

//=============================================================================
// PhysicsWorld2D - Main Physics Simulation Coordinator
//=============================================================================

/**
 * @brief Main Physics World System for ECScope Educational ECS Engine
 * 
 * The PhysicsWorld2D serves as the central coordinator for all physics simulation
 * in the ECS engine. It orchestrates the interaction between physics components,
 * collision detection, constraint solving, and integration while providing
 * comprehensive educational insights and performance optimization.
 * 
 * Key Responsibilities:
 * - Coordinate physics components across ECS entities
 * - Manage spatial partitioning for efficient collision detection
 * - Execute physics simulation pipeline (forces -> integration -> collision -> response)
 * - Provide educational debugging and visualization
 * - Monitor and optimize performance with custom memory allocators
 * - Generate detailed statistics for learning and analysis
 * 
 * Architecture:
 * - ECS System: Operates on entities with physics components
 * - Memory Efficient: Uses arena/pool allocators for optimal performance
 * - Educational: Extensive documentation and debugging features
 * - Configurable: Highly parameterized for different simulation needs
 * - Observable: Comprehensive statistics and event system
 */
class PhysicsWorld2D {
private:
    //-------------------------------------------------------------------------
    // Core Systems and Configuration
    //-------------------------------------------------------------------------
    Registry* registry_;                         // ECS registry reference
    PhysicsWorldConfig config_;                  // Configuration parameters
    PhysicsWorldStats stats_;                    // Performance and debug statistics
    
    //-------------------------------------------------------------------------
    // Memory Management
    //-------------------------------------------------------------------------
    std::unique_ptr<memory::ArenaAllocator> physics_arena_;    // Physics working memory
    std::unique_ptr<memory::PoolAllocator> contact_pool_;      // Contact point pool
    std::unique_ptr<memory::PoolAllocator> collision_pair_pool_; // Collision pair pool
    
    //-------------------------------------------------------------------------
    // Time Management
    //-------------------------------------------------------------------------
    f32 time_accumulator_;                       // Fixed timestep accumulator
    f32 current_physics_time_;                   // Current simulation time
    std::chrono::high_resolution_clock::time_point last_frame_time_;
    
    //-------------------------------------------------------------------------
    // Spatial Partitioning (Broad-Phase Collision Detection)
    //-------------------------------------------------------------------------
    struct SpatialHashGrid;
    std::unique_ptr<SpatialHashGrid> spatial_hash_;  // Spatial partitioning system
    
    //-------------------------------------------------------------------------
    // Collision Detection Data Structures
    //-------------------------------------------------------------------------
    struct ContactManifold {
        Entity entity_a;
        Entity entity_b;
        std::array<Vec2, constants::MAX_CONTACT_POINTS> contact_points;
        std::array<f32, constants::MAX_CONTACT_POINTS> penetration_depths;
        Vec2 contact_normal;
        u32 contact_count;
        f32 friction;
        f32 restitution;
        f32 lifetime;                            // How long contact has existed
        bool is_new_contact;
        
        ContactManifold() : entity_a(0), entity_b(0), contact_count(0), lifetime(0.0f), is_new_contact(true) {}
    };
    
    std::vector<ContactManifold> contact_manifolds_;  // All active contact manifolds
    std::unordered_map<u64, usize> contact_cache_;    // Contact caching for coherency
    
    //-------------------------------------------------------------------------
    // Force Application and Integration
    //-------------------------------------------------------------------------
    std::vector<Entity> active_entities_;            // Entities requiring physics simulation
    std::vector<Entity> sleeping_entities_;          // Entities currently sleeping
    std::unordered_set<Entity> entities_to_wake_;    // Entities to wake this frame
    
    //-------------------------------------------------------------------------
    // Event System
    //-------------------------------------------------------------------------
    std::vector<PhysicsEventCallback> event_callbacks_;
    std::vector<PhysicsEvent> event_queue_;
    
    //-------------------------------------------------------------------------
    // Profiling and Educational Tools
    //-------------------------------------------------------------------------
    struct ProfileTimer {
        std::chrono::high_resolution_clock::time_point start;
        f64* target_time;
        
        ProfileTimer(f64* target) : target_time(target) {
            start = std::chrono::high_resolution_clock::now();
        }
        
        ~ProfileTimer() {
            auto end = std::chrono::high_resolution_clock::now();
            *target_time = std::chrono::duration<f64, std::milli>(end - start).count();
        }
    };
    
    bool is_step_mode_;                          // Whether in educational step mode
    bool step_requested_;                        // Whether next step is requested
    u32 current_simulation_step_;                // Current step number for visualization
    
public:
    //-------------------------------------------------------------------------
    // Construction and Configuration
    //-------------------------------------------------------------------------
    
    /**
     * @brief Create physics world with specified configuration
     * 
     * @param registry ECS registry to operate on
     * @param config Physics simulation configuration
     */
    explicit PhysicsWorld2D(Registry& registry, 
                           const PhysicsWorldConfig& config = PhysicsWorldConfig::create_educational());
    
    /**
     * @brief Destructor with cleanup and final statistics
     */
    ~PhysicsWorld2D();
    
    // Non-copyable but movable
    PhysicsWorld2D(const PhysicsWorld2D&) = delete;
    PhysicsWorld2D& operator=(const PhysicsWorld2D&) = delete;
    PhysicsWorld2D(PhysicsWorld2D&&) = default;
    PhysicsWorld2D& operator=(PhysicsWorld2D&&) = default;
    
    //-------------------------------------------------------------------------
    // Main Simulation Interface
    //-------------------------------------------------------------------------
    
    /**
     * @brief Update physics simulation for one frame
     * 
     * This is the main entry point for physics simulation. It handles
     * fixed timestep accumulation and calls the internal step function.
     * 
     * @param delta_time Time elapsed since last frame (seconds)
     */
    void update(f32 delta_time);
    
    /**
     * @brief Perform one physics simulation step
     * 
     * Educational Method: This can be called directly for step-by-step
     * physics analysis and debugging.
     */
    void step();
    
    /**
     * @brief Reset simulation to initial state
     */
    void reset();
    
    /**
     * @brief Pause/resume simulation
     */
    void set_paused(bool paused);
    bool is_paused() const { return is_step_mode_ && !step_requested_; }
    
    /**
     * @brief Enable educational step-by-step mode
     */
    void enable_step_mode(bool enabled) {
        is_step_mode_ = enabled;
        step_requested_ = false;
    }
    
    /**
     * @brief Request next step in step mode
     */
    void request_step() {
        if (is_step_mode_) {
            step_requested_ = true;
        }
    }
    
    //-------------------------------------------------------------------------
    // Entity Management
    //-------------------------------------------------------------------------
    
    /**
     * @brief Add entity to physics simulation
     * 
     * Entities must have at minimum a Transform and RigidBody2D component.
     * Additional components like Collider2D and ForceAccumulator will be
     * automatically detected and used.
     * 
     * @param entity Entity to add to physics simulation
     * @return true if entity was successfully added
     */
    bool add_entity(Entity entity);
    
    /**
     * @brief Remove entity from physics simulation
     * 
     * @param entity Entity to remove from physics simulation
     * @return true if entity was successfully removed
     */
    bool remove_entity(Entity entity);
    
    /**
     * @brief Check if entity is managed by physics world
     */
    bool contains_entity(Entity entity) const;
    
    /**
     * @brief Get all entities managed by physics world
     */
    std::vector<Entity> get_all_physics_entities() const;
    
    /**
     * @brief Get active (non-sleeping) physics entities
     */
    std::vector<Entity> get_active_physics_entities() const;
    
    /**
     * @brief Wake up sleeping entity
     */
    void wake_entity(Entity entity);
    
    /**
     * @brief Force entity to sleep
     */
    void sleep_entity(Entity entity);
    
    //-------------------------------------------------------------------------
    // Force and Impulse Application
    //-------------------------------------------------------------------------
    
    /**
     * @brief Apply force to entity at center of mass
     */
    void apply_force(Entity entity, const Vec2& force);
    
    /**
     * @brief Apply force to entity at specific point
     */
    void apply_force_at_point(Entity entity, const Vec2& force, const Vec2& world_point);
    
    /**
     * @brief Apply torque to entity
     */
    void apply_torque(Entity entity, f32 torque);
    
    /**
     * @brief Apply impulse to entity (instantaneous momentum change)
     */
    void apply_impulse(Entity entity, const Vec2& impulse);
    
    /**
     * @brief Apply impulse at specific point
     */
    void apply_impulse_at_point(Entity entity, const Vec2& impulse, const Vec2& world_point);
    
    /**
     * @brief Apply angular impulse to entity
     */
    void apply_angular_impulse(Entity entity, f32 angular_impulse);
    
    //-------------------------------------------------------------------------
    // Configuration and Properties
    //-------------------------------------------------------------------------
    
    /**
     * @brief Get current configuration
     */
    const PhysicsWorldConfig& get_config() const { return config_; }
    
    /**
     * @brief Update configuration (some changes require reset)
     */
    void set_config(const PhysicsWorldConfig& new_config);
    
    /**
     * @brief Set world gravity
     */
    void set_gravity(const Vec2& gravity) { config_.gravity = gravity; }
    
    /**
     * @brief Get world gravity
     */
    Vec2 get_gravity() const { return config_.gravity; }
    
    /**
     * @brief Set time step (requires reset for consistency)
     */
    void set_time_step(f32 time_step);
    
    /**
     * @brief Get current physics time
     */
    f32 get_physics_time() const { return current_physics_time_; }
    
    /**
     * @brief Get simulation step count
     */
    u64 get_step_count() const { return stats_.total_steps; }
    
    //-------------------------------------------------------------------------
    // Collision Detection and Queries
    //-------------------------------------------------------------------------
    
    /**
     * @brief Raycast into physics world
     * 
     * @param ray Ray to cast
     * @param layer_mask Collision layers to test against
     * @return Raycast result with hit information
     */
    collision::RaycastResult raycast(const Ray2D& ray, u32 layer_mask = 0xFFFFFFFF);
    
    /**
     * @brief Find all entities overlapping with given shape
     * 
     * @param shape Shape to test for overlap
     * @param layer_mask Collision layers to test against
     * @return Vector of entities overlapping the shape
     */
    std::vector<Entity> overlap_shape(const CollisionShape& shape, u32 layer_mask = 0xFFFFFFFF);
    
    /**
     * @brief Find all entities within radius of point
     */
    std::vector<Entity> overlap_circle(const Vec2& center, f32 radius, u32 layer_mask = 0xFFFFFFFF);
    
    /**
     * @brief Find all entities within AABB
     */
    std::vector<Entity> overlap_aabb(const AABB& aabb, u32 layer_mask = 0xFFFFFFFF);
    
    /**
     * @brief Get closest entity to point
     */
    std::optional<Entity> get_closest_entity(const Vec2& point, f32 max_distance = 1000.0f);
    
    //-------------------------------------------------------------------------
    // Statistics and Profiling
    //-------------------------------------------------------------------------
    
    /**
     * @brief Get current simulation statistics
     */
    const PhysicsWorldStats& get_statistics() const { return stats_; }
    
    /**
     * @brief Reset performance statistics
     */
    void reset_statistics() { stats_.reset(); }
    
    /**
     * @brief Generate comprehensive performance report
     */
    std::string generate_performance_report() const;
    
    /**
     * @brief Get memory usage breakdown
     */
    struct MemoryUsageReport {
        usize total_memory;
        usize arena_used;
        usize arena_peak;
        usize pool_used;
        usize ecs_overhead;
        std::vector<std::pair<std::string, usize>> breakdown;
    };
    
    MemoryUsageReport get_memory_usage() const;
    
    //-------------------------------------------------------------------------
    // Event System
    //-------------------------------------------------------------------------
    
    /**
     * @brief Add event callback for physics events
     */
    void add_event_callback(const PhysicsEventCallback& callback) {
        event_callbacks_.push_back(callback);
    }
    
    /**
     * @brief Remove all event callbacks
     */
    void clear_event_callbacks() {
        event_callbacks_.clear();
    }
    
    //-------------------------------------------------------------------------
    // Educational and Debugging Interface
    //-------------------------------------------------------------------------
    
    /**
     * @brief Get detailed breakdown of current simulation step
     */
    struct SimulationStepBreakdown {
        std::string step_name;
        std::vector<std::string> sub_operations;
        std::vector<f64> operation_times;
        std::vector<Entity> entities_processed;
        std::vector<std::string> educational_notes;
    };
    
    std::vector<SimulationStepBreakdown> get_step_breakdown() const;
    
    /**
     * @brief Get visualization data for educational rendering
     */
    struct VisualizationData {
        struct RenderShape {
            Entity entity;
            CollisionShape shape;
            Transform world_transform;
            u32 color;
            bool is_sleeping;
            bool is_trigger;
        };
        
        struct RenderContact {
            Vec2 point;
            Vec2 normal;
            f32 depth;
            Entity entity_a;
            Entity entity_b;
        };
        
        struct RenderForce {
            Entity entity;
            Vec2 application_point;
            Vec2 force_vector;
            std::string force_type;
            f32 magnitude;
        };
        
        std::vector<RenderShape> collision_shapes;
        std::vector<RenderContact> contact_points;
        std::vector<RenderForce> force_vectors;
        std::vector<std::pair<Vec2, Vec2>> velocity_vectors;
        std::vector<std::pair<AABB, u32>> spatial_hash_cells;
    };
    
    VisualizationData get_visualization_data() const;
    
    /**
     * @brief Enable/disable various debug visualizations
     */
    void set_debug_render_collision_shapes(bool enable) { config_.debug_render_collision_shapes = enable; }
    void set_debug_render_contact_points(bool enable) { config_.debug_render_contact_points = enable; }
    void set_debug_render_forces(bool enable) { config_.debug_render_forces = enable; }
    void set_debug_render_velocities(bool enable) { config_.debug_render_velocities = enable; }
    void set_debug_render_spatial_hash(bool enable) { config_.debug_render_spatial_hash = enable; }
    
    //-------------------------------------------------------------------------
    // Advanced Features
    //-------------------------------------------------------------------------
    
    /**
     * @brief Create physics island for connected entities
     * 
     * Educational feature to demonstrate constraint island solving.
     */
    struct PhysicsIsland {
        std::vector<Entity> entities;
        std::vector<ContactManifold*> contacts;
        Vec2 total_momentum;
        f32 total_energy;
        bool is_sleeping;
    };
    
    std::vector<PhysicsIsland> get_physics_islands() const;
    
    /**
     * @brief Manually solve specific island (for educational analysis)
     */
    void solve_island(const PhysicsIsland& island, u32 iterations = 0);
    
    /**
     * @brief Export simulation state for analysis
     */
    struct SimulationSnapshot {
        f32 timestamp;
        std::vector<std::pair<Entity, RigidBody2D>> rigid_bodies;
        std::vector<std::pair<Entity, Transform>> transforms;
        std::vector<ContactManifold> contacts;
        PhysicsWorldStats stats;
    };
    
    SimulationSnapshot export_snapshot() const;
    
    /**
     * @brief Import simulation state
     */
    void import_snapshot(const SimulationSnapshot& snapshot);

private:
    //-------------------------------------------------------------------------
    // Internal Simulation Pipeline
    //-------------------------------------------------------------------------
    
    /** @brief Initialize physics world systems */
    void initialize();
    
    /** @brief Cleanup physics world resources */
    void cleanup();
    
    /** @brief Core simulation step implementation */
    void step_internal();
    
    /** @brief Update active entity list */
    void update_active_entities();
    
    /** @brief Apply gravity to all dynamic bodies */
    void apply_gravity();
    
    /** @brief Integrate forces to update velocities */
    void integrate_forces();
    
    /** @brief Perform broad-phase collision detection */
    void broad_phase_collision_detection();
    
    /** @brief Perform narrow-phase collision detection */
    void narrow_phase_collision_detection();
    
    /** @brief Solve contact constraints */
    void solve_contacts();
    
    /** @brief Integrate velocities to update positions */
    void integrate_velocities();
    
    /** @brief Update sleeping system */
    void update_sleeping_system();
    
    /** @brief Process collision events */
    void process_collision_events();
    
    /** @brief Update performance statistics */
    void update_statistics();
    
    //-------------------------------------------------------------------------
    // Helper Methods
    //-------------------------------------------------------------------------
    
    /** @brief Get contact cache key for two entities */
    static u64 get_contact_key(Entity a, Entity b);
    
    /** @brief Fire physics event */
    void fire_event(const PhysicsEvent& event);
    
    /** @brief Create contact manifold between two entities */
    std::optional<ContactManifold> create_contact_manifold(Entity a, Entity b);
    
    /** @brief Update existing contact manifold */
    void update_contact_manifold(ContactManifold& manifold);
    
    /** @brief Validate physics world state */
    bool validate_world_state() const;
};

} // namespace ecscope::physics