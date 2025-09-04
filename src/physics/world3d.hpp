#pragma once

/**
 * @file physics/world3d.hpp
 * @brief PhysicsWorld3D - Comprehensive 3D Physics Simulation Coordinator for ECScope ECS Engine
 * 
 * This header implements a complete 3D physics world system that extends the 2D foundation
 * while introducing the additional complexity and computational challenges of 3D physics:
 * 
 * Key Features:
 * - Complete 3D rigid body dynamics with inertia tensors
 * - Advanced 3D collision detection (SAT, GJK/EPA, continuous)
 * - 3D constraint system for joints, springs, and motors
 * - Integration with work-stealing job system for parallel processing
 * - Educational visualization and debugging tools
 * - Performance comparison and analysis with 2D equivalent
 * 
 * 3D Specific Enhancements:
 * - Quaternion-based orientation and integration
 * - 3x3 inertia tensor mathematics and world-space transformations
 * - Complex 3D contact manifold generation and clipping
 * - Parallel broad-phase with 3D spatial partitioning
 * - Advanced constraint solving for 3D joint systems
 * 
 * Educational Philosophy:
 * The 3D physics world provides comprehensive learning opportunities by:
 * - Demonstrating the complexity increase from 2D to 3D
 * - Showing real-world physics engine architecture
 * - Providing step-by-step algorithm breakdowns
 * - Enabling performance analysis and optimization learning
 * 
 * Performance Considerations:
 * - Work-stealing job system integration for parallel processing
 * - SIMD-optimized 3D vector and matrix operations
 * - Advanced spatial partitioning for efficient collision detection
 * - Memory-efficient contact manifold management
 * - Sleeping system for inactive 3D objects
 * 
 * @author ECScope Educational ECS Framework - 3D Physics Extension
 * @date 2025
 */

#include "world.hpp"          // 2D physics world foundation
#include "components3d.hpp"   // 3D physics components
#include "math3d.hpp"         // 3D mathematics
#include "collision3d.hpp"    // 3D collision detection
#include "../ecs/registry.hpp"
#include "../job_system/work_stealing_job_system.hpp"
#include "../memory/memory_tracker.hpp"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <chrono>
#include <functional>
#include <span>
#include <optional>

namespace ecscope::physics {

// Import 3D foundation
using namespace ecscope::physics::math3d;
using namespace ecscope::physics::components3d;
using namespace ecscope::physics::collision3d;

// Import job system
using namespace ecscope::job_system;

//=============================================================================
// 3D Physics World Configuration
//=============================================================================

/**
 * @brief Comprehensive 3D Physics World Configuration
 * 
 * Extends the 2D configuration with 3D-specific parameters and complexity considerations.
 */
struct PhysicsWorldConfig3D {
    //-------------------------------------------------------------------------
    // Basic 3D Simulation Parameters
    //-------------------------------------------------------------------------
    
    /** @brief 3D gravity vector in m/s² */
    Vec3 gravity{0.0f, -9.81f, 0.0f};
    
    /** @brief Physics time step in seconds */
    f32 time_step{1.0f / 60.0f};
    
    /** @brief Maximum time accumulation for fixed timestep */
    f32 max_time_accumulator{0.25f};
    
    /** @brief Constraint solver iterations (3D typically needs more) */
    u32 constraint_iterations{10};
    
    /** @brief Velocity solver iterations */
    u32 velocity_iterations{8};
    
    /** @brief Position solver iterations */
    u32 position_iterations{4};
    
    //-------------------------------------------------------------------------
    // 3D Collision Detection Parameters
    //-------------------------------------------------------------------------
    
    /** @brief 3D spatial hash cell size */
    f32 spatial_hash_cell_size{10.0f};
    
    /** @brief Initial 3D spatial hash capacity */
    u32 spatial_hash_initial_capacity{2048};
    
    /** @brief Enable continuous collision detection for fast objects */
    bool enable_continuous_collision{true};
    
    /** @brief CCD velocity threshold */
    f32 ccd_velocity_threshold{20.0f};
    
    /** @brief Maximum contact points per 3D manifold */
    u32 max_contact_points_3d{8};
    
    /** @brief Contact manifold lifetime for persistence */
    f32 contact_manifold_lifetime{0.1f};
    
    /** @brief Contact point tolerance for merging */
    f32 contact_point_tolerance{0.01f};
    
    //-------------------------------------------------------------------------
    // 3D Specific Physics Parameters
    //-------------------------------------------------------------------------
    
    /** @brief Angular velocity damping factor */
    f32 default_angular_damping{0.05f};
    
    /** @brief Linear velocity damping factor */
    f32 default_linear_damping{0.01f};
    
    /** @brief Maximum angular velocity (rad/s) to prevent instability */
    f32 max_angular_velocity{50.0f};
    
    /** @brief Maximum linear velocity (m/s) */
    f32 max_linear_velocity{1000.0f};
    
    /** @brief Quaternion normalization frequency (every N steps) */
    u32 quaternion_normalization_frequency{10};
    
    /** @brief Inertia tensor update frequency */
    u32 inertia_tensor_update_frequency{1};
    
    //-------------------------------------------------------------------------
    // Performance and Parallelization
    //-------------------------------------------------------------------------
    
    /** @brief Enable multithreading for 3D physics */
    bool enable_multithreading{true};
    
    /** @brief Number of worker threads (0 = auto-detect) */
    u32 worker_thread_count{0};
    
    /** @brief Enable work-stealing job system integration */
    bool enable_job_system_integration{true};
    
    /** @brief Enable parallel broad-phase collision detection */
    bool enable_parallel_broadphase{true};
    
    /** @brief Enable parallel narrow-phase collision detection */
    bool enable_parallel_narrowphase{true};
    
    /** @brief Enable parallel constraint solving */
    bool enable_parallel_constraints{true};
    
    /** @brief Enable parallel integration */
    bool enable_parallel_integration{true};
    
    /** @brief Minimum entities per thread for parallel processing */
    u32 min_entities_per_thread{50};
    
    /** @brief Maximum number of active 3D bodies */
    u32 max_active_bodies_3d{5000};
    
    //-------------------------------------------------------------------------
    // Advanced 3D Features
    //-------------------------------------------------------------------------
    
    /** @brief Enable 3D joint system */
    bool enable_joint_system{true};
    
    /** @brief Enable soft body dynamics */
    bool enable_soft_bodies{false};
    
    /** @brief Enable fluid simulation */
    bool enable_fluid_simulation{false};
    
    /** @brief Enable cloth simulation */
    bool enable_cloth_simulation{false};
    
    /** @brief Enable particle systems */
    bool enable_particle_systems{true};
    
    //-------------------------------------------------------------------------
    // Sleeping and Optimization
    //-------------------------------------------------------------------------
    
    /** @brief Enable 3D sleeping system */
    bool enable_sleeping{true};
    
    /** @brief Linear velocity threshold for sleeping */
    f32 sleep_linear_velocity_threshold{0.01f};
    
    /** @brief Angular velocity threshold for sleeping */
    f32 sleep_angular_velocity_threshold{0.01f};
    
    /** @brief Time threshold before sleeping */
    f32 sleep_time_threshold{1.0f};
    
    /** @brief Enable island-based solving */
    bool enable_island_solving{true};
    
    /** @brief Minimum island size for parallel processing */
    u32 min_island_size_parallel{10};
    
    //-------------------------------------------------------------------------
    // Educational and Debugging
    //-------------------------------------------------------------------------
    
    /** @brief Enable comprehensive 3D profiling */
    bool enable_profiling{true};
    
    /** @brief Enable step-by-step 3D visualization */
    bool enable_step_visualization{false};
    
    /** @brief Enable 3D collision shape debug rendering */
    bool debug_render_collision_shapes_3d{false};
    
    /** @brief Enable 3D contact point visualization */
    bool debug_render_contact_points_3d{false};
    
    /** @brief Enable 3D force vector visualization */
    bool debug_render_forces_3d{false};
    
    /** @brief Enable velocity visualization */
    bool debug_render_velocities_3d{false};
    
    /** @brief Enable angular velocity visualization */
    bool debug_render_angular_velocities{false};
    
    /** @brief Enable inertia tensor visualization */
    bool debug_render_inertia_tensors{false};
    
    /** @brief Enable constraint visualization */
    bool debug_render_constraints{false};
    
    /** @brief Enable 3D spatial partitioning visualization */
    bool debug_render_spatial_hash_3d{false};
    
    /** @brief Maximum debug elements to render */
    u32 max_debug_elements_3d{2000};
    
    /** @brief Enable performance comparison with 2D */
    bool enable_2d_3d_comparison{true};
    
    //-------------------------------------------------------------------------
    // Memory Management
    //-------------------------------------------------------------------------
    
    /** @brief 3D physics arena size (larger than 2D) */
    usize physics_arena_size_3d{32 * 1024 * 1024};  // 32 MB
    
    /** @brief 3D contact pool capacity */
    u32 contact_pool_capacity_3d{20000};
    
    /** @brief 3D collision pair pool capacity */
    u32 collision_pair_pool_capacity_3d{10000};
    
    /** @brief 3D constraint pool capacity */
    u32 constraint_pool_capacity{5000};
    
    /** @brief Enable memory tracking for 3D components */
    bool enable_memory_tracking_3d{true};
    
    //-------------------------------------------------------------------------
    // Factory Methods for Different 3D Scenarios
    //-------------------------------------------------------------------------
    
    /** @brief Educational configuration optimized for learning */
    static PhysicsWorldConfig3D create_educational() {
        PhysicsWorldConfig3D config;
        config.time_step = 1.0f / 60.0f;
        config.constraint_iterations = 8;
        config.enable_profiling = true;
        config.enable_step_visualization = true;
        config.debug_render_collision_shapes_3d = true;
        config.debug_render_contact_points_3d = true;
        config.debug_render_forces_3d = true;
        config.debug_render_angular_velocities = true;
        config.enable_2d_3d_comparison = true;
        config.max_active_bodies_3d = 1000;
        config.enable_multithreading = false;  // Simpler for learning
        return config;
    }
    
    /** @brief High-performance 3D configuration */
    static PhysicsWorldConfig3D create_performance() {
        PhysicsWorldConfig3D config;
        config.time_step = 1.0f / 120.0f;
        config.constraint_iterations = 12;
        config.velocity_iterations = 10;
        config.enable_profiling = false;
        config.enable_step_visualization = false;
        config.enable_multithreading = true;
        config.enable_job_system_integration = true;
        config.enable_parallel_broadphase = true;
        config.enable_parallel_narrowphase = true;
        config.enable_parallel_constraints = true;
        config.enable_sleeping = true;
        config.max_active_bodies_3d = 10000;
        config.physics_arena_size_3d = 64 * 1024 * 1024;  // 64 MB
        return config;
    }
    
    /** @brief High-accuracy 3D simulation */
    static PhysicsWorldConfig3D create_high_accuracy() {
        PhysicsWorldConfig3D config;
        config.time_step = 1.0f / 240.0f;
        config.constraint_iterations = 20;
        config.velocity_iterations = 15;
        config.position_iterations = 8;
        config.enable_continuous_collision = true;
        config.ccd_velocity_threshold = 5.0f;
        config.max_contact_points_3d = 12;
        config.quaternion_normalization_frequency = 5;
        config.sleep_linear_velocity_threshold = 0.001f;
        config.sleep_angular_velocity_threshold = 0.001f;
        return config;
    }
    
    /** @brief Game development optimized configuration */
    static PhysicsWorldConfig3D create_game_optimized() {
        PhysicsWorldConfig3D config;
        config.time_step = 1.0f / 60.0f;
        config.constraint_iterations = 6;
        config.velocity_iterations = 6;
        config.enable_multithreading = true;
        config.enable_sleeping = true;
        config.enable_continuous_collision = false;  // Often not needed in games
        config.max_active_bodies_3d = 3000;
        config.enable_profiling = false;
        config.spatial_hash_cell_size = 5.0f;  // Smaller for tighter spatial partitioning
        return config;
    }
};

//=============================================================================
// 3D Physics World Statistics
//=============================================================================

/**
 * @brief Comprehensive 3D Physics Statistics with 2D Comparison
 */
struct PhysicsWorldStats3D {
    //-------------------------------------------------------------------------
    // Basic Simulation State
    //-------------------------------------------------------------------------
    f32 current_time{0.0f};
    u64 total_steps{0};
    f32 time_accumulator{0.0f};
    
    //-------------------------------------------------------------------------
    // 3D Entity Counts
    //-------------------------------------------------------------------------
    u32 total_rigid_bodies_3d{0};
    u32 active_rigid_bodies_3d{0};
    u32 sleeping_rigid_bodies_3d{0};
    u32 static_bodies_3d{0};
    u32 total_colliders_3d{0};
    u32 trigger_colliders_3d{0};
    
    //-------------------------------------------------------------------------
    // 3D Collision Detection Statistics
    //-------------------------------------------------------------------------
    u32 broad_phase_pairs_3d{0};
    u32 narrow_phase_tests_3d{0};
    u32 active_contacts_3d{0};
    u32 new_contacts_3d{0};
    u32 persistent_contacts_3d{0};
    u32 contact_manifolds_3d{0};
    
    // 3D specific collision metrics
    u32 sat_tests_performed{0};
    u32 gjk_tests_performed{0};
    u32 epa_tests_performed{0};
    u32 ccd_tests_performed{0};
    
    //-------------------------------------------------------------------------
    // 3D Spatial Partitioning
    //-------------------------------------------------------------------------
    u32 spatial_hash_cells_used_3d{0};
    u32 spatial_hash_total_cells_3d{0};
    f32 spatial_hash_occupancy_3d{0.0f};
    f32 average_objects_per_cell_3d{0.0f};
    u32 max_objects_per_cell_3d{0};
    
    //-------------------------------------------------------------------------
    // 3D Constraint Solving
    //-------------------------------------------------------------------------
    u32 constraints_solved_3d{0};
    u32 constraint_islands_3d{0};
    f32 average_iterations_per_island_3d{0.0f};
    u32 max_iterations_used_3d{0};
    f32 constraint_residual_3d{0.0f};
    
    // 3D specific constraint types
    u32 joint_constraints{0};
    u32 contact_constraints{0};
    u32 friction_constraints{0};
    
    //-------------------------------------------------------------------------
    // Performance Timing (3D vs 2D comparison)
    //-------------------------------------------------------------------------
    f64 total_frame_time_3d{0.0};
    f64 broad_phase_time_3d{0.0};
    f64 narrow_phase_time_3d{0.0};
    f64 constraint_solve_time_3d{0.0};
    f64 integration_time_3d{0.0};
    f64 quaternion_normalization_time{0.0};
    f64 inertia_tensor_update_time{0.0};
    
    // Parallel processing times
    f64 job_system_overhead_time{0.0};
    f64 parallel_sync_time{0.0};
    f64 thread_idle_time{0.0};
    
    //-------------------------------------------------------------------------
    // 3D Specific Physics Metrics
    //-------------------------------------------------------------------------
    f32 total_linear_energy{0.0f};
    f32 total_rotational_energy{0.0f};
    f32 total_potential_energy_3d{0.0f};
    Vec3 total_linear_momentum_3d{0.0f, 0.0f, 0.0f};
    Vec3 total_angular_momentum_3d{0.0f, 0.0f, 0.0f};
    
    // Conservation errors
    f32 energy_conservation_error_3d{0.0f};
    Vec3 momentum_conservation_error_3d{0.0f, 0.0f, 0.0f};
    Vec3 angular_momentum_conservation_error{0.0f, 0.0f, 0.0f};
    
    //-------------------------------------------------------------------------
    // Memory Usage (3D typically uses more memory)
    //-------------------------------------------------------------------------
    usize total_physics_memory_3d{0};
    usize arena_memory_used_3d{0};
    usize arena_memory_peak_3d{0};
    usize contact_pool_usage_3d{0};
    usize constraint_pool_usage{0};
    usize job_system_memory_usage{0};
    
    //-------------------------------------------------------------------------
    // Performance Comparison Metrics
    //-------------------------------------------------------------------------
    struct ComparisonWith2D {
        f64 computational_complexity_ratio{1.0};  // 3D/2D complexity
        f64 memory_usage_ratio{1.0};              // 3D/2D memory
        f64 performance_ratio{1.0};               // 3D/2D performance
        u32 entity_count_ratio{1};                // 3D/2D entity handling
    } comparison_2d;
    
    //-------------------------------------------------------------------------
    // Parallel Processing Statistics
    //-------------------------------------------------------------------------
    struct ParallelStats {
        u32 total_jobs_submitted{0};
        u32 jobs_completed{0};
        u32 jobs_stolen{0};
        f32 load_balance_efficiency{0.0f};        // 0-1, 1 = perfect
        f32 parallel_efficiency{0.0f};            // Speedup/Thread count
        f64 average_job_duration_us{0.0};
        u32 worker_thread_count{0};
        std::vector<f32> per_thread_utilization;  // Per-thread usage %
    } parallel_stats;
    
    //-------------------------------------------------------------------------
    // Educational Metrics
    //-------------------------------------------------------------------------
    struct EducationalMetrics {
        // Algorithm usage statistics
        std::unordered_map<std::string, u32> algorithm_usage_count;
        std::unordered_map<std::string, f64> algorithm_timing;
        
        // Learning progression tracking
        f32 complexity_understanding_score{0.0f};
        u32 student_interaction_count{0};
        
        // Performance learning metrics
        f32 optimization_effectiveness{0.0f};
        std::vector<std::string> optimization_suggestions;
    } educational_metrics;
    
    //-------------------------------------------------------------------------
    // Utility Methods
    //-------------------------------------------------------------------------
    
    void reset() {
        *this = PhysicsWorldStats3D{};
    }
    
    void update_derived_stats() {
        // Update spatial hash metrics
        if (spatial_hash_total_cells_3d > 0) {
            spatial_hash_occupancy_3d = static_cast<f32>(spatial_hash_cells_used_3d) / spatial_hash_total_cells_3d;
        }
        
        if (spatial_hash_cells_used_3d > 0) {
            average_objects_per_cell_3d = static_cast<f32>(total_colliders_3d) / spatial_hash_cells_used_3d;
        }
        
        // Update island metrics
        if (constraint_islands_3d > 0) {
            average_iterations_per_island_3d = static_cast<f32>(constraints_solved_3d) / constraint_islands_3d;
        }
        
        // Update parallel efficiency
        if (parallel_stats.worker_thread_count > 0 && total_frame_time_3d > 0.0) {
            // Theoretical maximum speedup vs actual
            f32 theoretical_speedup = static_cast<f32>(parallel_stats.worker_thread_count);
            f32 actual_speedup = 1.0f; // Would need baseline single-threaded timing
            parallel_stats.parallel_efficiency = actual_speedup / theoretical_speedup;
        }
        
        // Update comparison metrics with 2D (would need 2D world reference)
        // This would typically be calculated by comparing with a 2D equivalent simulation
        comparison_2d.computational_complexity_ratio = estimate_3d_complexity_ratio();
        comparison_2d.memory_usage_ratio = estimate_3d_memory_ratio();
    }
    
    std::string generate_comprehensive_report() const {
        std::ostringstream oss;
        
        oss << "=== 3D Physics World Statistics ===\n";
        oss << "Simulation Time: " << current_time << "s (Steps: " << total_steps << ")\n";
        oss << "Active 3D Bodies: " << active_rigid_bodies_3d << "/" << total_rigid_bodies_3d;
        oss << " (Sleeping: " << sleeping_rigid_bodies_3d << ")\n";
        
        oss << "\n--- 3D Collision Detection ---\n";
        oss << "Contacts: " << active_contacts_3d << " (" << new_contacts_3d << " new)\n";
        oss << "Manifolds: " << contact_manifolds_3d << "\n";
        oss << "SAT Tests: " << sat_tests_performed << "\n";
        oss << "GJK Tests: " << gjk_tests_performed << "\n";
        oss << "EPA Tests: " << epa_tests_performed << "\n";
        
        oss << "\n--- 3D Performance ---\n";
        oss << "Total Frame Time: " << total_frame_time_3d << "ms\n";
        oss << "  - Broad Phase: " << broad_phase_time_3d << "ms\n";
        oss << "  - Narrow Phase: " << narrow_phase_time_3d << "ms\n";
        oss << "  - Constraint Solving: " << constraint_solve_time_3d << "ms\n";
        oss << "  - Integration: " << integration_time_3d << "ms\n";
        oss << "  - Quaternion Normalization: " << quaternion_normalization_time << "ms\n";
        
        oss << "\n--- Parallel Processing ---\n";
        oss << "Worker Threads: " << parallel_stats.worker_thread_count << "\n";
        oss << "Jobs Submitted: " << parallel_stats.total_jobs_submitted << "\n";
        oss << "Jobs Stolen: " << parallel_stats.jobs_stolen << "\n";
        oss << "Parallel Efficiency: " << (parallel_stats.parallel_efficiency * 100.0f) << "%\n";
        
        oss << "\n--- 3D vs 2D Comparison ---\n";
        oss << "Computational Complexity: " << comparison_2d.computational_complexity_ratio << "x\n";
        oss << "Memory Usage: " << comparison_2d.memory_usage_ratio << "x\n";
        oss << "Performance Ratio: " << comparison_2d.performance_ratio << "x\n";
        
        oss << "\n--- 3D Energy Conservation ---\n";
        oss << "Linear Energy: " << total_linear_energy << " J\n";
        oss << "Rotational Energy: " << total_rotational_energy << " J\n";
        oss << "Total Energy: " << (total_linear_energy + total_rotational_energy + total_potential_energy_3d) << " J\n";
        oss << "Energy Error: " << energy_conservation_error_3d << " J\n";
        
        return oss.str();
    }
    
private:
    f32 estimate_3d_complexity_ratio() const {
        // Rough estimate based on theoretical complexity increase
        // 3D collision detection is typically O(n log n) vs O(n log n) but with higher constants
        // 3D constraint solving has more complex matrix operations
        return 3.5f;  // Empirical estimate
    }
    
    f32 estimate_3d_memory_ratio() const {
        // 3D uses more memory due to:
        // - Vec3 vs Vec2 (1.5x base vectors)
        // - Quaternions vs floats (4x vs 1x for orientation)
        // - 3x3 inertia tensors vs scalar moments
        // - More complex contact manifolds
        return 2.5f;  // Empirical estimate
    }
};

//=============================================================================
// 3D Physics World Events
//=============================================================================

/**
 * @brief Extended physics events for 3D simulation
 */
enum class PhysicsEventType3D {
    // Basic collision events
    CollisionEnter3D,
    CollisionStay3D,
    CollisionExit3D,
    TriggerEnter3D,
    TriggerStay3D,
    TriggerExit3D,
    
    // 3D specific events
    BodySleep3D,
    BodyWake3D,
    JointBreak,
    JointCreate,
    AngularVelocityLimit,
    LinearVelocityLimit,
    InertiaUpdate,
    
    // Performance events
    PerformanceThreshold,
    MemoryThreshold,
    ParallelizationEvent
};

/**
 * @brief 3D Physics event data
 */
struct PhysicsEvent3D {
    PhysicsEventType3D type;
    Entity entity_a{0};
    Entity entity_b{0};
    Vec3 contact_point{0.0f, 0.0f, 0.0f};
    Vec3 contact_normal{0.0f, 0.0f, 0.0f};
    Vec3 impulse_vector{0.0f, 0.0f, 0.0f};
    f32 impulse_magnitude{0.0f};
    f32 timestamp{0.0f};
    void* user_data{nullptr};
    
    // 3D specific data
    Quaternion relative_rotation{0.0f, 0.0f, 0.0f, 1.0f};
    Vec3 angular_impulse{0.0f, 0.0f, 0.0f};
    
    PhysicsEvent3D(PhysicsEventType3D t, Entity a, Entity b = 0) 
        : type(t), entity_a(a), entity_b(b) {}
};

using PhysicsEventCallback3D = std::function<void(const PhysicsEvent3D&)>;

} // namespace ecscope::physics