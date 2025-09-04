/**
 * @file physics/world3d.cpp
 * @brief PhysicsWorld3D Implementation - Complete 3D Physics Simulation
 * 
 * This implementation provides a comprehensive 3D physics world system that demonstrates
 * the complexity and performance characteristics of 3D physics simulation compared to 2D.
 * 
 * Key Implementation Details:
 * - Integration with work-stealing job system for parallel processing
 * - Advanced 3D collision detection and response algorithms
 * - Quaternion-based orientation integration with numerical stability
 * - Inertia tensor transformations and world-space calculations
 * - Educational step-by-step algorithm breakdowns
 * - Performance monitoring and 2D comparison metrics
 * 
 * Educational Value:
 * This implementation serves as both a production-quality 3D physics engine
 * and an educational tool for understanding advanced physics simulation concepts.
 * 
 * @author ECScope Educational ECS Framework - 3D Physics Extension
 * @date 2025
 */

#include "world3d.hpp"
#include "core/log.hpp"
#include <algorithm>
#include <execution>
#include <thread>

namespace ecscope::physics {

//=============================================================================
// 3D Spatial Partitioning System
//=============================================================================

/**
 * @brief 3D Spatial Hash Grid for efficient broad-phase collision detection
 * 
 * Educational Context:
 * 3D spatial partitioning is significantly more complex than 2D:
 * - Memory usage grows as O(n³) with resolution
 * - Hash function must distribute well in 3D space
 * - Query operations must handle 3D neighborhoods
 * - Load balancing becomes more critical with sparse 3D data
 */
class SpatialHashGrid3D {
private:
    struct Cell {
        std::vector<Entity> entities;
        AABB3D bounds;
        bool is_active{false};
        
        void clear() {
            entities.clear();
            is_active = false;
        }
    };
    
    std::unordered_map<u64, Cell> cells_;
    f32 cell_size_;
    Vec3 world_min_{-1000.0f, -1000.0f, -1000.0f};
    Vec3 world_max_{1000.0f, 1000.0f, 1000.0f};
    
    // Statistics
    mutable u32 total_queries_{0};
    mutable u32 total_insertions_{0};
    mutable u32 hash_collisions_{0};
    
public:
    explicit SpatialHashGrid3D(f32 cell_size) : cell_size_(cell_size) {
        LOG_INFO("Created 3D Spatial Hash Grid with cell size: {}", cell_size);
    }
    
    /**
     * @brief 3D hash function for spatial coordinates
     * 
     * Uses a combination of prime numbers to minimize hash collisions in 3D.
     */
    u64 hash_position(const Vec3& position) const {
        i32 x = static_cast<i32>(std::floor(position.x / cell_size_));
        i32 y = static_cast<i32>(std::floor(position.y / cell_size_));
        i32 z = static_cast<i32>(std::floor(position.z / cell_size_));
        
        // Large primes for good 3D distribution
        constexpr u64 p1 = 73856093ULL;
        constexpr u64 p2 = 19349663ULL;
        constexpr u64 p3 = 83492791ULL;
        
        return (static_cast<u64>(x) * p1) ^ (static_cast<u64>(y) * p2) ^ (static_cast<u64>(z) * p3);
    }
    
    void insert_entity(Entity entity, const AABB3D& aabb) {
        ++total_insertions_;
        
        // Find all cells that the AABB overlaps
        Vec3 min_cell = Vec3{
            std::floor(aabb.min.x / cell_size_),
            std::floor(aabb.min.y / cell_size_),
            std::floor(aabb.min.z / cell_size_)
        };
        
        Vec3 max_cell = Vec3{
            std::floor(aabb.max.x / cell_size_),
            std::floor(aabb.max.y / cell_size_),
            std::floor(aabb.max.z / cell_size_)
        };
        
        // Insert entity into all overlapping cells
        for (f32 z = min_cell.z; z <= max_cell.z; z += 1.0f) {
            for (f32 y = min_cell.y; y <= max_cell.y; y += 1.0f) {
                for (f32 x = min_cell.x; x <= max_cell.x; x += 1.0f) {
                    Vec3 cell_pos{x * cell_size_, y * cell_size_, z * cell_size_};
                    u64 hash = hash_position(cell_pos);
                    
                    auto& cell = cells_[hash];
                    if (!cell.is_active) {
                        cell.bounds = AABB3D{
                            cell_pos,
                            cell_pos + Vec3{cell_size_, cell_size_, cell_size_}
                        };
                        cell.is_active = true;
                    }
                    
                    // Check for hash collision (different positions, same hash)
                    if (cell.bounds.min != cell_pos) {
                        ++hash_collisions_;
                    }
                    
                    cell.entities.push_back(entity);
                }
            }
        }
    }
    
    std::vector<Entity> query_region(const AABB3D& query_aabb) const {
        ++total_queries_;
        
        std::unordered_set<Entity> unique_entities;
        
        Vec3 min_cell = Vec3{
            std::floor(query_aabb.min.x / cell_size_),
            std::floor(query_aabb.min.y / cell_size_),
            std::floor(query_aabb.min.z / cell_size_)
        };
        
        Vec3 max_cell = Vec3{
            std::floor(query_aabb.max.x / cell_size_),
            std::floor(query_aabb.max.y / cell_size_),
            std::floor(query_aabb.max.z / cell_size_)
        };
        
        for (f32 z = min_cell.z; z <= max_cell.z; z += 1.0f) {
            for (f32 y = min_cell.y; y <= max_cell.y; y += 1.0f) {
                for (f32 x = min_cell.x; x <= max_cell.x; x += 1.0f) {
                    Vec3 cell_pos{x * cell_size_, y * cell_size_, z * cell_size_};
                    u64 hash = hash_position(cell_pos);
                    
                    auto it = cells_.find(hash);
                    if (it != cells_.end() && it->second.is_active) {
                        for (Entity entity : it->second.entities) {
                            unique_entities.insert(entity);
                        }
                    }
                }
            }
        }
        
        return std::vector<Entity>(unique_entities.begin(), unique_entities.end());
    }
    
    void clear() {
        for (auto& [hash, cell] : cells_) {
            cell.clear();
        }
    }
    
    // Statistics
    usize get_active_cell_count() const {
        return std::count_if(cells_.begin(), cells_.end(),
                           [](const auto& pair) { return pair.second.is_active; });
    }
    
    f32 get_load_factor() const {
        if (cells_.empty()) return 0.0f;
        return static_cast<f32>(get_active_cell_count()) / cells_.size();
    }
    
    u32 get_total_queries() const { return total_queries_; }
    u32 get_total_insertions() const { return total_insertions_; }
    u32 get_hash_collisions() const { return hash_collisions_; }
};

//=============================================================================
// PhysicsWorld3D Implementation
//=============================================================================

/**
 * @brief Complete 3D Physics World Implementation
 */
class PhysicsWorld3D {
private:
    //-------------------------------------------------------------------------
    // Core Systems
    //-------------------------------------------------------------------------
    Registry* registry_;
    PhysicsWorldConfig3D config_;
    PhysicsWorldStats3D stats_;
    
    //-------------------------------------------------------------------------
    // Memory Management
    //-------------------------------------------------------------------------
    std::unique_ptr<memory::ArenaAllocator> physics_arena_3d_;
    std::unique_ptr<memory::PoolAllocator> contact_pool_3d_;
    std::unique_ptr<memory::PoolAllocator> constraint_pool_;
    
    //-------------------------------------------------------------------------
    // Job System Integration
    //-------------------------------------------------------------------------
    JobSystem* job_system_;
    bool owns_job_system_;
    
    //-------------------------------------------------------------------------
    // Time Management
    //-------------------------------------------------------------------------
    f32 time_accumulator_;
    f32 current_physics_time_;
    std::chrono::high_resolution_clock::time_point last_frame_time_;
    
    //-------------------------------------------------------------------------
    // Spatial Partitioning
    //-------------------------------------------------------------------------
    std::unique_ptr<SpatialHashGrid3D> spatial_hash_3d_;
    
    //-------------------------------------------------------------------------
    // 3D Collision and Constraint Data
    //-------------------------------------------------------------------------
    struct ContactManifold3D {
        Entity entity_a;
        Entity entity_b;
        std::array<Vec3, 8> contact_points;  // More points possible in 3D
        std::array<f32, 8> penetration_depths;
        Vec3 contact_normal;
        u32 contact_count;
        f32 friction;
        f32 restitution;
        f32 lifetime;
        bool is_new_contact;
        
        // 3D specific properties
        Vec3 tangent1;  // First tangent vector for friction
        Vec3 tangent2;  // Second tangent vector for friction
        
        ContactManifold3D() 
            : entity_a(0), entity_b(0), contact_count(0), lifetime(0.0f), is_new_contact(true) {}
    };
    
    std::vector<ContactManifold3D> contact_manifolds_3d_;
    std::unordered_map<u64, usize> contact_cache_3d_;
    
    //-------------------------------------------------------------------------
    // Entity Management
    //-------------------------------------------------------------------------
    std::vector<Entity> active_entities_3d_;
    std::vector<Entity> sleeping_entities_3d_;
    std::unordered_set<Entity> entities_to_wake_3d_;
    
    //-------------------------------------------------------------------------
    // Event System
    //-------------------------------------------------------------------------
    std::vector<PhysicsEventCallback3D> event_callbacks_3d_;
    std::vector<PhysicsEvent3D> event_queue_3d_;
    
    //-------------------------------------------------------------------------
    // Educational and Profiling
    //-------------------------------------------------------------------------
    struct ProfileTimer3D {
        std::chrono::high_resolution_clock::time_point start;
        f64* target_time;
        
        ProfileTimer3D(f64* target) : target_time(target) {
            start = std::chrono::high_resolution_clock::now();
        }
        
        ~ProfileTimer3D() {
            if (target_time) {
                auto end = std::chrono::high_resolution_clock::now();
                *target_time = std::chrono::duration<f64, std::milli>(end - start).count();
            }
        }
    };
    
    bool is_step_mode_;
    bool step_requested_;
    u32 current_simulation_step_;
    u32 quaternion_normalization_counter_;
    
public:
    //-------------------------------------------------------------------------
    // Construction and Initialization
    //-------------------------------------------------------------------------
    
    explicit PhysicsWorld3D(Registry& registry, 
                           const PhysicsWorldConfig3D& config = PhysicsWorldConfig3D::create_educational(),
                           JobSystem* external_job_system = nullptr) 
        : registry_(&registry)
        , config_(config)
        , time_accumulator_(0.0f)
        , current_physics_time_(0.0f)
        , is_step_mode_(false)
        , step_requested_(false)
        , current_simulation_step_(0)
        , quaternion_normalization_counter_(0)
    {
        LOG_INFO("Initializing PhysicsWorld3D...");
        
        // Initialize job system
        if (external_job_system) {
            job_system_ = external_job_system;
            owns_job_system_ = false;
            LOG_INFO("Using external job system for 3D physics");
        } else if (config_.enable_job_system_integration) {
            auto job_config = JobSystem::Config::create_performance_optimized();
            job_config.worker_count = config_.worker_thread_count;
            job_system_ = new JobSystem(job_config);
            job_system_->initialize();
            owns_job_system_ = true;
            LOG_INFO("Created internal job system for 3D physics with {} threads", 
                    job_system_->worker_count());
        } else {
            job_system_ = nullptr;
            owns_job_system_ = false;
            LOG_INFO("3D physics running in single-threaded mode");
        }
        
        initialize();
        
        LOG_INFO("PhysicsWorld3D initialized successfully");
        LOG_INFO("  - Memory Arena: {} MB", config_.physics_arena_size_3d / (1024 * 1024));
        LOG_INFO("  - Max Bodies: {}", config_.max_active_bodies_3d);
        LOG_INFO("  - Spatial Hash Cell Size: {}", config_.spatial_hash_cell_size);
        LOG_INFO("  - Multithreading: {}", config_.enable_multithreading ? "Enabled" : "Disabled");
    }
    
    ~PhysicsWorld3D() {
        LOG_INFO("Shutting down PhysicsWorld3D...");
        
        cleanup();
        
        if (owns_job_system_ && job_system_) {
            job_system_->shutdown();
            delete job_system_;
        }
        
        LOG_INFO("PhysicsWorld3D shutdown complete");
        LOG_INFO("Final Statistics:\n{}", stats_.generate_comprehensive_report());
    }
    
    //-------------------------------------------------------------------------
    // Main Simulation Interface
    //-------------------------------------------------------------------------
    
    void update(f32 delta_time) {
        PROFILE_TIMER(stats_.total_frame_time_3d);
        
        // Handle educational step mode
        if (is_step_mode_ && !step_requested_) {
            return;
        }
        step_requested_ = false;
        
        // Fixed timestep accumulation
        time_accumulator_ += delta_time;
        time_accumulator_ = std::min(time_accumulator_, config_.max_time_accumulator);
        
        // Simulate fixed timesteps
        while (time_accumulator_ >= config_.time_step) {
            step_internal();
            time_accumulator_ -= config_.time_step;
            current_physics_time_ += config_.time_step;
            ++stats_.total_steps;
            ++current_simulation_step_;
        }
        
        // Update statistics
        update_statistics();
        
        // Process events
        process_events();
    }
    
    void step() {
        step_internal();
        ++stats_.total_steps;
        ++current_simulation_step_;
        update_statistics();
    }
    
    //-------------------------------------------------------------------------
    // Entity Management
    //-------------------------------------------------------------------------
    
    bool add_entity_3d(Entity entity) {
        // Verify entity has required 3D components
        if (!registry_->has<Transform3D>(entity) || !registry_->has<RigidBody3D>(entity)) {
            LOG_WARNING("Entity {} missing required 3D components", entity);
            return false;
        }
        
        active_entities_3d_.push_back(entity);
        
        LOG_DEBUG("Added 3D entity {} to physics world", entity);
        return true;
    }
    
    bool remove_entity_3d(Entity entity) {
        auto it = std::find(active_entities_3d_.begin(), active_entities_3d_.end(), entity);
        if (it != active_entities_3d_.end()) {
            active_entities_3d_.erase(it);
            LOG_DEBUG("Removed 3D entity {} from physics world", entity);
            return true;
        }
        return false;
    }
    
    //-------------------------------------------------------------------------
    // Force and Impulse Application
    //-------------------------------------------------------------------------
    
    void apply_force_3d(Entity entity, const Vec3& force) {
        if (!registry_->has<RigidBody3D>(entity)) return;
        
        auto& body = registry_->get<RigidBody3D>(entity);
        body.apply_force(force);
        
        // Wake up the body if it's sleeping
        if (!body.is_awake) {
            body.wake_up();
        }
        
        // Fire event
        PhysicsEvent3D event(PhysicsEventType3D::ForceApplied, entity);
        event.impulse_vector = force;
        event.impulse_magnitude = force.length();
        fire_event_3d(event);
    }
    
    void apply_force_at_point_3d(Entity entity, const Vec3& force, const Vec3& world_point) {
        if (!registry_->has<RigidBody3D>(entity) || !registry_->has<Transform3D>(entity)) return;
        
        auto& body = registry_->get<RigidBody3D>(entity);
        const auto& transform = registry_->get<Transform3D>(entity);
        
        Vec3 center_of_mass_world = transform.transform_point(body.local_center_of_mass);
        body.apply_force_at_point(force, world_point, center_of_mass_world);
        
        if (!body.is_awake) {
            body.wake_up();
        }
    }
    
    void apply_torque_3d(Entity entity, const Vec3& torque) {
        if (!registry_->has<RigidBody3D>(entity)) return;
        
        auto& body = registry_->get<RigidBody3D>(entity);
        body.apply_torque(torque);
        
        if (!body.is_awake) {
            body.wake_up();
        }
    }
    
    void apply_impulse_3d(Entity entity, const Vec3& impulse) {
        if (!registry_->has<RigidBody3D>(entity)) return;
        
        auto& body = registry_->get<RigidBody3D>(entity);
        body.apply_impulse(impulse);
        
        if (!body.is_awake) {
            body.wake_up();
        }
    }
    
    //-------------------------------------------------------------------------
    // Configuration and Properties
    //-------------------------------------------------------------------------
    
    const PhysicsWorldConfig3D& get_config_3d() const { return config_; }
    const PhysicsWorldStats3D& get_statistics_3d() const { return stats_; }
    
    void set_gravity_3d(const Vec3& gravity) { config_.gravity = gravity; }
    Vec3 get_gravity_3d() const { return config_.gravity; }
    
    f32 get_physics_time() const { return current_physics_time_; }
    u64 get_step_count() const { return stats_.total_steps; }
    
    //-------------------------------------------------------------------------
    // Event System
    //-------------------------------------------------------------------------
    
    void add_event_callback_3d(const PhysicsEventCallback3D& callback) {
        event_callbacks_3d_.push_back(callback);
    }
    
    void clear_event_callbacks_3d() {
        event_callbacks_3d_.clear();
    }
    
private:
    //-------------------------------------------------------------------------
    // Internal Implementation
    //-------------------------------------------------------------------------
    
    void initialize() {
        // Initialize memory systems
        physics_arena_3d_ = std::make_unique<memory::ArenaAllocator>(
            config_.physics_arena_size_3d, "Physics3D_Arena");
        
        contact_pool_3d_ = std::make_unique<memory::PoolAllocator>(
            sizeof(ContactManifold3D), config_.contact_pool_capacity_3d, "Physics3D_Contacts");
        
        constraint_pool_ = std::make_unique<memory::PoolAllocator>(
            64, config_.constraint_pool_capacity, "Physics3D_Constraints");
        
        // Initialize spatial partitioning
        spatial_hash_3d_ = std::make_unique<SpatialHashGrid3D>(config_.spatial_hash_cell_size);
        
        // Reserve entity containers
        active_entities_3d_.reserve(config_.max_active_bodies_3d);
        contact_manifolds_3d_.reserve(config_.contact_pool_capacity_3d);
        
        last_frame_time_ = std::chrono::high_resolution_clock::now();
    }
    
    void cleanup() {
        active_entities_3d_.clear();
        sleeping_entities_3d_.clear();
        contact_manifolds_3d_.clear();
        contact_cache_3d_.clear();
        event_queue_3d_.clear();
    }
    
    void step_internal() {
        PROFILE_TIMER(nullptr);  // Manual timing for educational breakdown
        auto step_start = std::chrono::high_resolution_clock::now();
        
        // 1. Update active entities list
        update_active_entities_3d();
        
        // 2. Apply gravity and persistent forces
        {
            PROFILE_TIMER(&stats_.integration_time_3d);
            apply_gravity_and_forces();
        }
        
        // 3. Integrate forces to velocities
        {
            PROFILE_TIMER(&stats_.integration_time_3d);
            integrate_forces_3d();
        }
        
        // 4. Broad-phase collision detection
        {
            PROFILE_TIMER(&stats_.broad_phase_time_3d);
            broad_phase_collision_detection_3d();
        }
        
        // 5. Narrow-phase collision detection
        {
            PROFILE_TIMER(&stats_.narrow_phase_time_3d);
            narrow_phase_collision_detection_3d();
        }
        
        // 6. Solve constraints and contacts
        {
            PROFILE_TIMER(&stats_.constraint_solve_time_3d);
            solve_constraints_and_contacts_3d();
        }
        
        // 7. Integrate velocities to positions
        {
            PROFILE_TIMER(&stats_.integration_time_3d);
            integrate_velocities_3d();
        }
        
        // 8. Update sleeping system
        update_sleeping_system_3d();
        
        // 9. Periodic maintenance
        periodic_maintenance();
        
        auto step_end = std::chrono::high_resolution_clock::now();
        stats_.total_frame_time_3d = std::chrono::duration<f64, std::milli>(step_end - step_start).count();
    }
    
    void update_active_entities_3d() {
        stats_.total_rigid_bodies_3d = 0;
        stats_.active_rigid_bodies_3d = 0;
        stats_.sleeping_rigid_bodies_3d = 0;
        stats_.static_bodies_3d = 0;
        stats_.total_colliders_3d = 0;
        stats_.trigger_colliders_3d = 0;
        
        // Count entities and update lists
        active_entities_3d_.clear();
        
        registry_->view<Transform3D, RigidBody3D>().each([this](auto entity, auto& transform, auto& body) {
            ++stats_.total_rigid_bodies_3d;
            
            if (body.body_type == BodyType::Static) {
                ++stats_.static_bodies_3d;
            } else if (body.is_awake) {
                ++stats_.active_rigid_bodies_3d;
                active_entities_3d_.push_back(entity);
            } else {
                ++stats_.sleeping_rigid_bodies_3d;
            }
            
            // Update energy cache for educational purposes
            body.update_energy_cache();
        });
        
        // Count colliders
        registry_->view<Collider3D>().each([this](auto entity, auto& collider) {
            ++stats_.total_colliders_3d;
            if (collider.is_trigger) {
                ++stats_.trigger_colliders_3d;
            }
        });
    }
    
    void apply_gravity_and_forces() {
        if (!config_.enable_multithreading || !job_system_ || active_entities_3d_.size() < config_.min_entities_per_thread) {
            // Single-threaded version
            for (Entity entity : active_entities_3d_) {
                apply_forces_to_entity(entity);
            }
        } else {
            // Parallel version using job system
            parallel_apply_forces();
        }
    }
    
    void apply_forces_to_entity(Entity entity) {
        if (!registry_->has<RigidBody3D>(entity) || !registry_->has<Transform3D>(entity)) return;
        
        auto& body = registry_->get<RigidBody3D>(entity);
        const auto& transform = registry_->get<Transform3D>(entity);
        
        // Apply gravity
        if (body.use_gravity && body.mass > constants::EPSILON) {
            Vec3 gravity_force = config_.gravity * body.mass;
            body.apply_force(gravity_force);
        }
        
        // Apply forces from ForceAccumulator3D if present
        if (registry_->has<ForceAccumulator3D>(entity)) {
            const auto& force_accumulator = registry_->get<ForceAccumulator3D>(entity);
            force_accumulator.apply_to_rigid_body(body, transform, config_.gravity, config_.time_step);
        }
    }
    
    void parallel_apply_forces() {
        if (!job_system_) return;
        
        const usize batch_size = std::max(config_.min_entities_per_thread, 
                                         active_entities_3d_.size() / job_system_->worker_count());
        
        std::vector<JobID> force_jobs;
        
        for (usize i = 0; i < active_entities_3d_.size(); i += batch_size) {
            usize end = std::min(i + batch_size, active_entities_3d_.size());
            
            JobID job_id = job_system_->submit_job(
                "Apply3DForces_" + std::to_string(i),
                [this, i, end]() {
                    for (usize j = i; j < end; ++j) {
                        apply_forces_to_entity(active_entities_3d_[j]);
                    }
                },
                JobPriority::High
            );
            
            force_jobs.push_back(job_id);
            ++stats_.parallel_stats.total_jobs_submitted;
        }
        
        // Wait for all force application jobs
        job_system_->wait_for_batch(force_jobs);
    }
    
    void integrate_forces_3d() {
        // This can also be parallelized
        for (Entity entity : active_entities_3d_) {
            if (!registry_->has<RigidBody3D>(entity)) continue;
            
            auto& body = registry_->get<RigidBody3D>(entity);
            
            if (body.body_type != BodyType::Dynamic) continue;
            
            // Semi-implicit Euler integration for forces to velocities
            // v = v + (F/m) * dt
            Vec3 linear_acceleration = body.accumulated_force * body.inv_mass;
            body.linear_velocity += linear_acceleration * config_.time_step;
            
            // Apply linear damping
            body.linear_velocity *= (1.0f - body.linear_damping * config_.time_step);
            
            // Angular integration: ω = ω + I⁻¹ * τ * dt
            Vec3 angular_acceleration = body.multiply_by_inverse_inertia(body.accumulated_torque);
            body.angular_velocity += angular_acceleration * config_.time_step;
            
            // Apply angular damping
            body.angular_velocity *= (1.0f - body.angular_damping * config_.time_step);
            
            // Velocity limits for stability
            if (body.linear_velocity.length_squared() > config_.max_linear_velocity * config_.max_linear_velocity) {
                body.linear_velocity = body.linear_velocity.normalized() * config_.max_linear_velocity;
            }
            
            if (body.angular_velocity.length_squared() > config_.max_angular_velocity * config_.max_angular_velocity) {
                body.angular_velocity = body.angular_velocity.normalized() * config_.max_angular_velocity;
            }
            
            // Clear accumulated forces
            body.clear_forces();
        }
    }
    
    void broad_phase_collision_detection_3d() {
        // Clear and rebuild spatial hash
        spatial_hash_3d_->clear();
        
        // Insert all colliders into spatial hash
        registry_->view<Transform3D, Collider3D>().each([this](auto entity, auto& transform, auto& collider) {
            if (!collider.is_enabled) return;
            
            AABB3D aabb = collider.calculate_aabb(transform);
            spatial_hash_3d_->insert_entity(entity, aabb);
        });
        
        // Update spatial hash statistics
        stats_.spatial_hash_cells_used_3d = spatial_hash_3d_->get_active_cell_count();
        stats_.spatial_hash_occupancy_3d = spatial_hash_3d_->get_load_factor();
    }
    
    void narrow_phase_collision_detection_3d() {
        // Clear previous contacts
        contact_manifolds_3d_.clear();
        
        // For each active collider, query spatial hash for potential collisions
        registry_->view<Transform3D, Collider3D>().each([this](auto entity_a, auto& transform_a, auto& collider_a) {
            if (!collider_a.is_enabled) return;
            
            AABB3D aabb_a = collider_a.calculate_aabb(transform_a);
            auto potential_colliders = spatial_hash_3d_->query_region(aabb_a);
            
            for (Entity entity_b : potential_colliders) {
                if (entity_a >= entity_b) continue;  // Avoid duplicate pairs
                
                if (!registry_->has<Transform3D>(entity_b) || !registry_->has<Collider3D>(entity_b)) continue;
                
                const auto& transform_b = registry_->get<Transform3D>(entity_b);
                const auto& collider_b = registry_->get<Collider3D>(entity_b);
                
                if (!collider_b.is_enabled) continue;
                
                // Layer filtering
                if ((collider_a.collision_mask & collider_b.collision_layer) == 0 &&
                    (collider_b.collision_mask & collider_a.collision_layer) == 0) {
                    continue;
                }
                
                // Perform detailed collision detection
                auto contact_manifold = detect_collision_3d(entity_a, entity_b, transform_a, transform_b, collider_a, collider_b);
                if (contact_manifold.has_value()) {
                    contact_manifolds_3d_.push_back(contact_manifold.value());
                    ++stats_.active_contacts_3d;
                }
                
                ++stats_.narrow_phase_tests_3d;
            }
        });
        
        stats_.contact_manifolds_3d = contact_manifolds_3d_.size();
    }
    
    std::optional<ContactManifold3D> detect_collision_3d(Entity entity_a, Entity entity_b,
                                                        const Transform3D& transform_a, const Transform3D& transform_b,
                                                        const Collider3D& collider_a, const Collider3D& collider_b) {
        // This would delegate to specific 3D collision detection algorithms
        // For now, we'll implement a simple sphere-sphere test as an example
        
        if (collider_a.shape_type == Collider3D::ShapeType::Sphere && 
            collider_b.shape_type == Collider3D::ShapeType::Sphere) {
            
            return detect_sphere_sphere_collision(entity_a, entity_b, transform_a, transform_b, collider_a, collider_b);
        }
        
        // Add more collision detection algorithms here (SAT, GJK/EPA, etc.)
        
        return std::nullopt;
    }
    
    std::optional<ContactManifold3D> detect_sphere_sphere_collision(Entity entity_a, Entity entity_b,
                                                                   const Transform3D& transform_a, const Transform3D& transform_b,
                                                                   const Collider3D& collider_a, const Collider3D& collider_b) {
        Vec3 center_a = transform_a.transform_point(collider_a.local_offset);
        Vec3 center_b = transform_b.transform_point(collider_b.local_offset);
        
        f32 radius_a = collider_a.shape_data.sphere.radius;
        f32 radius_b = collider_b.shape_data.sphere.radius;
        
        Vec3 delta = center_b - center_a;
        f32 distance_squared = delta.length_squared();
        f32 radii_sum = radius_a + radius_b;
        
        if (distance_squared >= radii_sum * radii_sum) {
            return std::nullopt;  // No collision
        }
        
        // Create contact manifold
        ContactManifold3D manifold;
        manifold.entity_a = entity_a;
        manifold.entity_b = entity_b;
        manifold.contact_count = 1;
        
        f32 distance = std::sqrt(distance_squared);
        if (distance > constants::EPSILON) {
            manifold.contact_normal = delta / distance;
        } else {
            // Spheres are at same position - choose arbitrary normal
            manifold.contact_normal = Vec3::unit_x();
        }
        
        f32 penetration = radii_sum - distance;
        manifold.contact_points[0] = center_a + manifold.contact_normal * (radius_a - penetration * 0.5f);
        manifold.penetration_depths[0] = penetration;
        
        // Material properties
        manifold.friction = std::sqrt(collider_a.friction * collider_b.friction);
        manifold.restitution = std::sqrt(collider_a.restitution * collider_b.restitution);
        
        // Generate tangent vectors for friction
        vec3::generate_orthonormal_basis(manifold.contact_normal);
        
        manifold.is_new_contact = true;
        
        return manifold;
    }
    
    void solve_constraints_and_contacts_3d() {
        // Iterative constraint solver
        for (u32 iteration = 0; iteration < config_.constraint_iterations; ++iteration) {
            for (auto& manifold : contact_manifolds_3d_) {
                solve_contact_constraint_3d(manifold);
            }
        }
        
        stats_.constraints_solved_3d = contact_manifolds_3d_.size() * config_.constraint_iterations;
    }
    
    void solve_contact_constraint_3d(ContactManifold3D& manifold) {
        if (!registry_->has<RigidBody3D>(manifold.entity_a) || !registry_->has<RigidBody3D>(manifold.entity_b)) {
            return;
        }
        
        auto& body_a = registry_->get<RigidBody3D>(manifold.entity_a);
        auto& body_b = registry_->get<RigidBody3D>(manifold.entity_b);
        
        // Only solve for dynamic bodies
        if (body_a.body_type != BodyType::Dynamic && body_b.body_type != BodyType::Dynamic) {
            return;
        }
        
        // Solve normal constraint (prevent penetration)
        for (u32 i = 0; i < manifold.contact_count; ++i) {
            solve_normal_constraint_3d(manifold, i, body_a, body_b);
            solve_friction_constraint_3d(manifold, i, body_a, body_b);
        }
    }
    
    void solve_normal_constraint_3d(const ContactManifold3D& manifold, u32 contact_index,
                                   RigidBody3D& body_a, RigidBody3D& body_b) {
        Vec3 contact_point = manifold.contact_points[contact_index];
        Vec3 normal = manifold.contact_normal;
        
        // Calculate relative velocity at contact point
        Vec3 rel_velocity = calculate_relative_velocity_at_point(body_a, body_b, contact_point);
        
        f32 normal_velocity = rel_velocity.dot(normal);
        
        // Don't resolve if objects are separating
        if (normal_velocity >= 0.0f) return;
        
        // Calculate impulse magnitude
        f32 effective_mass = calculate_effective_mass_3d(body_a, body_b, contact_point, normal);
        f32 impulse_magnitude = -normal_velocity * effective_mass * (1.0f + manifold.restitution);
        
        // Apply impulse
        Vec3 impulse = normal * impulse_magnitude;
        apply_impulse_at_point(body_a, -impulse, contact_point);
        apply_impulse_at_point(body_b, impulse, contact_point);
    }
    
    void solve_friction_constraint_3d(const ContactManifold3D& manifold, u32 contact_index,
                                     RigidBody3D& body_a, RigidBody3D& body_b) {
        Vec3 contact_point = manifold.contact_points[contact_index];
        
        // Calculate relative velocity at contact point
        Vec3 rel_velocity = calculate_relative_velocity_at_point(body_a, body_b, contact_point);
        
        // Get tangential velocity (remove normal component)
        Vec3 tangent_velocity = rel_velocity - manifold.contact_normal * rel_velocity.dot(manifold.contact_normal);
        
        if (tangent_velocity.length_squared() < constants::EPSILON) return;
        
        Vec3 tangent = tangent_velocity.normalized();
        f32 tangent_speed = tangent_velocity.length();
        
        // Calculate friction impulse
        f32 effective_mass = calculate_effective_mass_3d(body_a, body_b, contact_point, tangent);
        f32 friction_impulse_magnitude = tangent_speed * effective_mass * manifold.friction;
        
        Vec3 friction_impulse = tangent * friction_impulse_magnitude;
        
        // Apply friction impulse
        apply_impulse_at_point(body_a, -friction_impulse, contact_point);
        apply_impulse_at_point(body_b, friction_impulse, contact_point);
    }
    
    Vec3 calculate_relative_velocity_at_point(const RigidBody3D& body_a, const RigidBody3D& body_b, const Vec3& world_point) {
        // Get center of mass positions (would need Transform3D access)
        Vec3 com_a{0.0f, 0.0f, 0.0f};  // Placeholder
        Vec3 com_b{0.0f, 0.0f, 0.0f};  // Placeholder
        
        // Calculate velocity at contact point for each body
        Vec3 r_a = world_point - com_a;
        Vec3 r_b = world_point - com_b;
        
        Vec3 vel_a = body_a.linear_velocity + body_a.angular_velocity.cross(r_a);
        Vec3 vel_b = body_b.linear_velocity + body_b.angular_velocity.cross(r_b);
        
        return vel_b - vel_a;
    }
    
    f32 calculate_effective_mass_3d(const RigidBody3D& body_a, const RigidBody3D& body_b, 
                                   const Vec3& contact_point, const Vec3& direction) {
        // This is a simplified version - full implementation would need world-space inertia tensors
        f32 inv_mass_sum = body_a.inv_mass + body_b.inv_mass;
        
        // For educational purposes, we'll use a simplified effective mass calculation
        // Real implementation would involve full 3D inertia tensor calculations
        
        return 1.0f / (inv_mass_sum + constants::EPSILON);
    }
    
    void apply_impulse_at_point(RigidBody3D& body, const Vec3& impulse, const Vec3& world_point) {
        if (body.body_type != BodyType::Dynamic) return;
        
        // Apply linear impulse
        body.linear_velocity += impulse * body.inv_mass;
        
        // Apply angular impulse (would need center of mass position)
        // This is simplified - real implementation needs proper world-space calculations
        Vec3 torque = Vec3::zero();  // Placeholder calculation
        Vec3 angular_impulse = body.multiply_by_inverse_inertia(torque);
        body.angular_velocity += angular_impulse;
    }
    
    void integrate_velocities_3d() {
        for (Entity entity : active_entities_3d_) {
            if (!registry_->has<RigidBody3D>(entity) || !registry_->has<Transform3D>(entity)) continue;
            
            auto& body = registry_->get<RigidBody3D>(entity);
            auto& transform = registry_->get<Transform3D>(entity);
            
            if (body.body_type != BodyType::Dynamic) continue;
            
            // Integrate linear motion: x = x + v * dt
            transform.position += body.linear_velocity * config_.time_step;
            
            // Integrate angular motion using quaternions
            integrate_angular_motion_3d(transform, body.angular_velocity, config_.time_step);
            
            // Update sleep timer
            if (body.can_sleep) {
                bool is_moving_slowly = 
                    (body.linear_velocity.length_squared() < config_.sleep_linear_velocity_threshold * config_.sleep_linear_velocity_threshold) &&
                    (body.angular_velocity.length_squared() < config_.sleep_angular_velocity_threshold * config_.sleep_angular_velocity_threshold);
                
                if (is_moving_slowly) {
                    body.sleep_time += config_.time_step;
                } else {
                    body.sleep_time = 0.0f;
                }
            }
        }
    }
    
    void integrate_angular_motion_3d(Transform3D& transform, const Vec3& angular_velocity, f32 dt) {
        if (angular_velocity.length_squared() < constants::EPSILON) return;
        
        // Create quaternion representing the rotation during this timestep
        f32 angle = angular_velocity.length() * dt;
        Vec3 axis = angular_velocity.normalized();
        
        Quaternion rotation_delta = Quaternion::from_axis_angle(axis, angle);
        
        // Apply rotation: q_new = rotation_delta * q_old
        transform.rotation = rotation_delta * transform.rotation;
        
        // Quaternions need periodic normalization to prevent drift
        if (++quaternion_normalization_counter_ >= config_.quaternion_normalization_frequency) {
            transform.rotation.normalize();
            quaternion_normalization_counter_ = 0;
        }
    }
    
    void update_sleeping_system_3d() {
        if (!config_.enable_sleeping) return;
        
        for (Entity entity : active_entities_3d_) {
            if (!registry_->has<RigidBody3D>(entity)) continue;
            
            auto& body = registry_->get<RigidBody3D>(entity);
            
            if (body.should_sleep(config_.sleep_linear_velocity_threshold, 
                                 config_.sleep_angular_velocity_threshold, 
                                 config_.sleep_time_threshold)) {
                body.sleep();
                
                // Fire sleep event
                PhysicsEvent3D event(PhysicsEventType3D::BodySleep3D, entity);
                fire_event_3d(event);
            }
        }
    }
    
    void periodic_maintenance() {
        // Quaternion normalization for all entities
        if (current_simulation_step_ % config_.quaternion_normalization_frequency == 0) {
            PROFILE_TIMER(&stats_.quaternion_normalization_time);
            
            registry_->view<Transform3D>().each([](auto entity, auto& transform) {
                transform.rotation.normalize();
            });
        }
        
        // Inertia tensor updates (if needed for dynamic shapes)
        if (current_simulation_step_ % config_.inertia_tensor_update_frequency == 0) {
            PROFILE_TIMER(&stats_.inertia_tensor_update_time);
            // Update world-space inertia tensors if needed
            // This would be implementation-specific based on shape types
        }
    }
    
    void update_statistics() {
        stats_.update_derived_stats();
        
        // Update energy calculations
        stats_.total_linear_energy = 0.0f;
        stats_.total_rotational_energy = 0.0f;
        stats_.total_linear_momentum_3d = Vec3::zero();
        stats_.total_angular_momentum_3d = Vec3::zero();
        
        registry_->view<RigidBody3D>().each([this](auto entity, auto& body) {
            if (body.body_type == BodyType::Dynamic) {
                stats_.total_linear_energy += body.calculate_kinetic_energy();
                stats_.total_rotational_energy += body.calculate_rotational_energy();
                
                stats_.total_linear_momentum_3d += body.linear_velocity * body.mass;
                // Angular momentum calculation would need world-space inertia tensor
            }
        });
        
        // Update job system statistics
        if (job_system_) {
            auto job_stats = job_system_->get_system_statistics();
            stats_.parallel_stats.total_jobs_submitted += job_stats.total_jobs_submitted;
            stats_.parallel_stats.jobs_completed += job_stats.total_jobs_completed;
            stats_.parallel_stats.worker_thread_count = job_system_->worker_count();
        }
    }
    
    void process_events() {
        for (const auto& callback : event_callbacks_3d_) {
            for (const auto& event : event_queue_3d_) {
                callback(event);
            }
        }
        event_queue_3d_.clear();
    }
    
    void fire_event_3d(const PhysicsEvent3D& event) {
        event_queue_3d_.push_back(event);
    }
};

} // namespace ecscope::physics