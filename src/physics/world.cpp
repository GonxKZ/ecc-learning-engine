#include "world.hpp"
#include "../core/log.hpp"
#include <algorithm>
#include <execution>
#include <unordered_set>
#include <set>

namespace ecscope::physics {

//=============================================================================
// Spatial Hash Grid Implementation (Broad-Phase Collision Detection)
//=============================================================================

/**
 * @brief Spatial hash grid for efficient broad-phase collision detection
 * 
 * This implementation demonstrates spatial partitioning concepts and provides
 * educational insights into how modern physics engines optimize collision detection.
 */
struct PhysicsWorld2D::SpatialHashGrid {
    f32 cell_size;
    std::unordered_map<u64, std::vector<Entity>> grid;
    mutable std::vector<std::pair<Entity, Entity>> potential_pairs;
    
    explicit SpatialHashGrid(f32 size) : cell_size(size) {
        potential_pairs.reserve(1000);  // Pre-allocate for common case
    }
    
    /** @brief Convert world position to grid cell coordinates */
    std::pair<i32, i32> world_to_cell(const Vec2& world_pos) const {
        return {
            static_cast<i32>(std::floor(world_pos.x / cell_size)),
            static_cast<i32>(std::floor(world_pos.y / cell_size))
        };
    }
    
    /** @brief Convert cell coordinates to hash key */
    static u64 cell_to_key(i32 x, i32 y) {
        return (static_cast<u64>(x) << 32) | static_cast<u32>(y);
    }
    
    /** @brief Clear all entities from grid */
    void clear() {
        grid.clear();
        potential_pairs.clear();
    }
    
    /** @brief Insert entity into spatial grid based on its AABB */
    void insert(Entity entity, const AABB& aabb) {
        auto [min_x, min_y] = world_to_cell(aabb.min);
        auto [max_x, max_y] = world_to_cell(aabb.max);
        
        // Insert entity into all cells it overlaps
        for (i32 x = min_x; x <= max_x; ++x) {
            for (i32 y = min_y; y <= max_y; ++y) {
                u64 key = cell_to_key(x, y);
                grid[key].push_back(entity);
            }
        }
    }
    
    /** @brief Generate all potential collision pairs */
    const std::vector<std::pair<Entity, Entity>>& get_potential_pairs() const {
        potential_pairs.clear();
        std::unordered_set<u64> processed_pairs;
        
        // Iterate through all cells and generate pairs
        for (const auto& [key, entities] : grid) {
            if (entities.size() < 2) continue;
            
            // Generate pairs within this cell
            for (usize i = 0; i < entities.size(); ++i) {
                for (usize j = i + 1; j < entities.size(); ++j) {
                    Entity a = entities[i];
                    Entity b = entities[j];
                    
                    // Ensure consistent ordering and avoid duplicates
                    if (a > b) std::swap(a, b);
                    u64 pair_key = (static_cast<u64>(a) << 32) | b;
                    
                    if (processed_pairs.find(pair_key) == processed_pairs.end()) {
                        processed_pairs.insert(pair_key);
                        potential_pairs.emplace_back(a, b);
                    }
                }
            }
        }
        
        return potential_pairs;
    }
    
    /** @brief Get statistics for educational analysis */
    struct Stats {
        u32 total_cells;
        u32 occupied_cells;
        u32 total_entities;
        f32 average_entities_per_cell;
        u32 max_entities_per_cell;
        f32 occupancy_ratio;
    };
    
    Stats get_stats() const {
        Stats stats{};
        stats.occupied_cells = static_cast<u32>(grid.size());
        stats.max_entities_per_cell = 0;
        
        u32 total_entity_instances = 0;
        for (const auto& [key, entities] : grid) {
            total_entity_instances += static_cast<u32>(entities.size());
            stats.max_entities_per_cell = std::max(stats.max_entities_per_cell, 
                                                  static_cast<u32>(entities.size()));
        }
        
        stats.average_entities_per_cell = stats.occupied_cells > 0 ? 
            static_cast<f32>(total_entity_instances) / stats.occupied_cells : 0.0f;
        
        return stats;
    }
};

//=============================================================================
// PhysicsWorld2D Implementation
//=============================================================================

PhysicsWorld2D::PhysicsWorld2D(Registry& registry, const PhysicsWorldConfig& config)
    : registry_(&registry)
    , config_(config)
    , time_accumulator_(0.0f)
    , current_physics_time_(0.0f)
    , is_step_mode_(false)
    , step_requested_(false)
    , current_simulation_step_(0)
{
    initialize();
    
    LOG_INFO("PhysicsWorld2D created with configuration:");
    LOG_INFO("  - Time step: {}s ({} FPS equivalent)", config_.time_step, 1.0f / config_.time_step);
    LOG_INFO("  - Constraint iterations: {}", config_.constraint_iterations);
    LOG_INFO("  - Spatial hash cell size: {}", config_.spatial_hash_cell_size);
    LOG_INFO("  - Max active bodies: {}", config_.max_active_bodies);
    LOG_INFO("  - Physics arena size: {} MB", config_.physics_arena_size / (1024 * 1024));
    
    if (config_.enable_profiling) {
        LOG_INFO("  - Profiling: ENABLED");
    }
    
    if (config_.enable_step_visualization) {
        LOG_INFO("  - Step visualization: ENABLED");
        enable_step_mode(true);
    }
}

PhysicsWorld2D::~PhysicsWorld2D() {
    if (config_.enable_profiling && stats_.total_steps > 0) {
        LOG_INFO("PhysicsWorld2D destroyed - Final Statistics:");
        LOG_INFO("  - Total simulation time: {:.2f}s", current_physics_time_);
        LOG_INFO("  - Total steps: {}", stats_.total_steps);
        LOG_INFO("  - Average frame time: {:.3f}ms", stats_.total_frame_time);
        LOG_INFO("  - Performance score: {:.1f}/100", stats_.performance_score);
        
        if (stats_.total_kinetic_energy > 0.0f || stats_.total_potential_energy > 0.0f) {
            LOG_INFO("  - Final energy: {:.3f}J (KE: {:.3f}, PE: {:.3f})", 
                    stats_.total_kinetic_energy + stats_.total_potential_energy,
                    stats_.total_kinetic_energy, stats_.total_potential_energy);
        }
        
        if (stats_.arena_memory_peak > 0) {
            LOG_INFO("  - Peak memory usage: {} KB", stats_.arena_memory_peak / 1024);
        }
    }
    
    cleanup();
}

void PhysicsWorld2D::initialize() {
    // Initialize custom allocators for optimal physics performance
    physics_arena_ = std::make_unique<memory::ArenaAllocator>(
        config_.physics_arena_size, "PhysicsWorld_Arena", config_.enable_memory_tracking);
    
    contact_pool_ = std::make_unique<memory::PoolAllocator>(
        sizeof(ContactManifold), config_.contact_pool_capacity, "ContactManifold_Pool");
    
    collision_pair_pool_ = std::make_unique<memory::PoolAllocator>(
        sizeof(std::pair<Entity, Entity>), config_.collision_pair_pool_capacity, "CollisionPair_Pool");
    
    // Initialize spatial hash grid for broad-phase collision detection
    spatial_hash_ = std::make_unique<SpatialHashGrid>(config_.spatial_hash_cell_size);
    
    // Pre-allocate vectors for common operations
    contact_manifolds_.reserve(config_.contact_pool_capacity);
    active_entities_.reserve(config_.max_active_bodies);
    sleeping_entities_.reserve(config_.max_active_bodies);
    event_queue_.reserve(1000);
    
    // Initialize timing
    last_frame_time_ = std::chrono::high_resolution_clock::now();
    
    // Reset statistics
    stats_.reset();
    
    LOG_INFO("PhysicsWorld2D initialized successfully");
    
    // Register with memory tracker for educational analysis
    if (config_.enable_memory_tracking) {
        memory::tracker::register_category("Physics_Simulation", 
                                          "Memory used by physics world simulation");
        memory::tracker::track_allocation(this, sizeof(*this), "Physics_Simulation");
    }
}

void PhysicsWorld2D::cleanup() {
    // Clear all data structures
    contact_manifolds_.clear();
    active_entities_.clear();
    sleeping_entities_.clear();
    entities_to_wake_.clear();
    event_queue_.clear();
    contact_cache_.clear();
    
    // Reset allocators
    if (physics_arena_) {
        physics_arena_->clear();
    }
    if (contact_pool_) {
        contact_pool_->reset();
    }
    if (collision_pair_pool_) {
        collision_pair_pool_->reset();
    }
    
    // Unregister from memory tracker
    if (config_.enable_memory_tracking) {
        memory::tracker::track_deallocation(this, sizeof(*this), "Physics_Simulation");
    }
    
    LOG_INFO("PhysicsWorld2D cleaned up");
}

void PhysicsWorld2D::update(f32 delta_time) {
    // Handle educational step mode
    if (is_step_mode_ && !step_requested_) {
        return;  // Wait for step request
    }
    step_requested_ = false;
    
    // Clamp delta time to prevent spiral of death
    delta_time = std::min(delta_time, config_.max_time_accumulator);
    
    // Fixed timestep accumulation
    time_accumulator_ += delta_time;
    
    // Perform physics steps while we have accumulated time
    while (time_accumulator_ >= config_.time_step) {
        step();
        time_accumulator_ -= config_.time_step;
        current_physics_time_ += config_.time_step;
    }
}

void PhysicsWorld2D::step() {
    if (config_.enable_profiling) {
        auto step_start = std::chrono::high_resolution_clock::now();
        
        step_internal();
        
        auto step_end = std::chrono::high_resolution_clock::now();
        stats_.total_frame_time = std::chrono::duration<f64, std::milli>(step_end - step_start).count();
    } else {
        step_internal();
    }
    
    ++stats_.total_steps;
    ++current_simulation_step_;
}

void PhysicsWorld2D::step_internal() {
    // Clear per-frame statistics
    stats_.broad_phase_pairs = 0;
    stats_.narrow_phase_tests = 0;
    stats_.new_contacts = 0;
    stats_.persistent_contacts = 0;
    
    // 1. Update active entity list and wake sleeping entities
    {
        ProfileTimer timer(config_.enable_profiling ? &stats_.sleeping_update_time : nullptr);
        update_active_entities();
    }
    
    // 2. Apply gravity to all dynamic bodies
    {
        ProfileTimer timer(config_.enable_profiling ? &stats_.integration_time : nullptr);
        apply_gravity();
    }
    
    // 3. Apply accumulated forces and integrate to update velocities
    {
        ProfileTimer timer(config_.enable_profiling ? &stats_.integration_time : nullptr);
        integrate_forces();
    }
    
    // 4. Broad-phase collision detection using spatial hashing
    {
        ProfileTimer timer(config_.enable_profiling ? &stats_.broad_phase_time : nullptr);
        broad_phase_collision_detection();
    }
    
    // 5. Narrow-phase collision detection and contact generation
    {
        ProfileTimer timer(config_.enable_profiling ? &stats_.narrow_phase_time : nullptr);
        narrow_phase_collision_detection();
    }
    
    // 6. Solve contact and constraint equations
    {
        ProfileTimer timer(config_.enable_profiling ? &stats_.constraint_solve_time : nullptr);
        solve_contacts();
    }
    
    // 7. Integrate velocities to update positions and rotations
    {
        ProfileTimer timer(config_.enable_profiling ? &stats_.integration_time : nullptr);
        integrate_velocities();
    }
    
    // 8. Update sleeping system for performance optimization
    {
        ProfileTimer timer(config_.enable_profiling ? &stats_.sleeping_update_time : nullptr);
        update_sleeping_system();
    }
    
    // 9. Process collision events and callbacks
    process_collision_events();
    
    // 10. Update comprehensive statistics for educational analysis
    if (config_.enable_profiling) {
        update_statistics();
    }
    
    // 11. Clear frame-specific data
    event_queue_.clear();
    entities_to_wake_.clear();
}

void PhysicsWorld2D::update_active_entities() {
    active_entities_.clear();
    
    // Query ECS for all entities with physics components
    registry_->for_each<Transform, RigidBody2D>([this](Entity entity, const Transform& transform, const RigidBody2D& rigidbody) {
        // Skip static bodies for most operations
        if (!rigidbody.physics_flags.is_static) {
            // Wake entities that were requested to wake
            if (entities_to_wake_.find(entity) != entities_to_wake_.end()) {
                auto* rb = registry_->get_component<RigidBody2D>(entity);
                if (rb) {
                    rb->wake_up();
                }
            }
            
            // Add active (non-sleeping) entities to simulation
            if (!rigidbody.physics_flags.is_sleeping) {
                active_entities_.push_back(entity);
            }
        }
    });
    
    // Update statistics
    stats_.active_rigid_bodies = static_cast<u32>(active_entities_.size());
    
    // Count total and sleeping bodies
    u32 total_bodies = 0;
    u32 sleeping_bodies = 0;
    u32 static_bodies = 0;
    
    registry_->for_each<RigidBody2D>([&](Entity entity, const RigidBody2D& rigidbody) {
        ++total_bodies;
        if (rigidbody.physics_flags.is_static) {
            ++static_bodies;
        } else if (rigidbody.physics_flags.is_sleeping) {
            ++sleeping_bodies;
        }
    });
    
    stats_.total_rigid_bodies = total_bodies;
    stats_.sleeping_rigid_bodies = sleeping_bodies;
    stats_.static_bodies = static_bodies;
    
    if (config_.enable_profiling && stats_.total_steps % 60 == 0) {  // Log every second at 60fps
        LOG_DEBUG("Active entities: {} / {} (sleeping: {}, static: {})", 
                 stats_.active_rigid_bodies, stats_.total_rigid_bodies, 
                 sleeping_bodies, static_bodies);
    }
}

void PhysicsWorld2D::apply_gravity() {
    if (config_.gravity.x == 0.0f && config_.gravity.y == 0.0f) {
        return;  // No gravity to apply
    }
    
    for (Entity entity : active_entities_) {
        auto* rigidbody = registry_->get_component<RigidBody2D>(entity);
        auto* forces = registry_->get_component<ForceAccumulator>(entity);
        
        if (rigidbody && forces && !rigidbody->physics_flags.ignore_gravity) {
            // Apply gravitational force: F = m * g * gravity_scale
            Vec2 gravity_force = config_.gravity * rigidbody->mass * rigidbody->gravity_scale;
            forces->apply_force(gravity_force, "Gravity");
            
            // Track for educational energy calculations
            if (config_.enable_profiling) {
                auto* transform = registry_->get_component<Transform>(entity);
                if (transform) {
                    // Calculate potential energy: PE = m * g * h (relative to zero level)
                    f32 potential_energy = rigidbody->mass * std::abs(config_.gravity.y) * 
                                          std::max(0.0f, transform->position.y);
                    stats_.total_potential_energy += potential_energy;
                }
            }
        }
    }
}

void PhysicsWorld2D::integrate_forces() {
    for (Entity entity : active_entities_) {
        auto* rigidbody = registry_->get_component<RigidBody2D>(entity);
        auto* forces = registry_->get_component<ForceAccumulator>(entity);
        
        if (!rigidbody || !forces) continue;
        if (rigidbody->physics_flags.is_kinematic) continue;
        
        // Get accumulated forces and torques
        Vec2 net_force;
        f32 net_torque;
        forces->get_net_forces(net_force, net_torque);
        
        // Apply damping (velocity-dependent forces)
        Vec2 damping_force = rigidbody->velocity * (-rigidbody->linear_damping * rigidbody->mass);
        f32 damping_torque = rigidbody->angular_velocity * (-rigidbody->angular_damping * rigidbody->moment_of_inertia);
        
        net_force += damping_force;
        net_torque += damping_torque;
        
        // Integration: a = F / m, α = τ / I
        if (rigidbody->inverse_mass > 0.0f) {
            rigidbody->acceleration = net_force * rigidbody->inverse_mass;
            
            // Semi-implicit Euler integration: v_new = v_old + a * dt
            rigidbody->velocity += rigidbody->acceleration * config_.time_step;
            
            // Apply velocity limits for stability
            if (rigidbody->max_velocity > 0.0f) {
                rigidbody->velocity = vec2::clamp_magnitude(rigidbody->velocity, rigidbody->max_velocity);
            }
        }
        
        if (rigidbody->inverse_moment_of_inertia > 0.0f && !rigidbody->physics_flags.freeze_rotation) {
            rigidbody->angular_acceleration = net_torque * rigidbody->inverse_moment_of_inertia;
            
            // Integrate angular velocity
            rigidbody->angular_velocity += rigidbody->angular_acceleration * config_.time_step;
            
            // Apply angular velocity limits
            if (rigidbody->max_angular_velocity > 0.0f) {
                rigidbody->angular_velocity = std::clamp(rigidbody->angular_velocity, 
                                                       -rigidbody->max_angular_velocity, 
                                                        rigidbody->max_angular_velocity);
            }
        }
        
        // Handle impulses (direct velocity changes)
        Vec2 impulse;
        f32 angular_impulse;
        forces->get_impulses(impulse, angular_impulse);
        
        if (rigidbody->inverse_mass > 0.0f) {
            rigidbody->velocity += impulse * rigidbody->inverse_mass;
        }
        
        if (rigidbody->inverse_moment_of_inertia > 0.0f && !rigidbody->physics_flags.freeze_rotation) {
            rigidbody->angular_velocity += angular_impulse * rigidbody->inverse_moment_of_inertia;
        }
        
        // Clear accumulated forces for next frame
        forces->clear_accumulated_forces();
        
        // Calculate kinetic energy for educational metrics
        if (config_.enable_profiling) {
            stats_.total_kinetic_energy += rigidbody->calculate_kinetic_energy();
        }
    }
}

void PhysicsWorld2D::broad_phase_collision_detection() {
    // Clear spatial hash grid
    spatial_hash_->clear();
    
    // Insert all entities with colliders into spatial hash
    u32 total_colliders = 0;
    
    for (Entity entity : active_entities_) {
        auto* transform = registry_->get_component<Transform>(entity);
        auto* collider = registry_->get_component<Collider2D>(entity);
        
        if (transform && collider) {
            // Get world-space AABB for this collider
            AABB world_aabb = collider->get_world_aabb(*transform);
            
            // Insert into spatial hash grid
            spatial_hash_->insert(entity, world_aabb);
            ++total_colliders;
        }
    }
    
    // Also include static/kinematic bodies for collision detection
    registry_->for_each<Transform, Collider2D, RigidBody2D>([&](Entity entity, const Transform& transform, 
                                                              const Collider2D& collider, const RigidBody2D& rigidbody) {
        if (rigidbody.physics_flags.is_static || rigidbody.physics_flags.is_kinematic) {
            AABB world_aabb = collider.get_world_aabb(transform);
            spatial_hash_->insert(entity, world_aabb);
            ++total_colliders;
        }
    });
    
    // Update statistics
    stats_.total_colliders = total_colliders;
    
    auto spatial_stats = spatial_hash_->get_stats();
    stats_.spatial_hash_cells_used = spatial_stats.occupied_cells;
    stats_.spatial_hash_occupancy = spatial_stats.occupancy_ratio;
    stats_.average_objects_per_cell = spatial_stats.average_entities_per_cell;
    stats_.max_objects_per_cell = spatial_stats.max_entities_per_cell;
    
    // Generate potential collision pairs
    const auto& pairs = spatial_hash_->get_potential_pairs();
    stats_.broad_phase_pairs = static_cast<u32>(pairs.size());
    
    if (config_.enable_profiling && stats_.total_steps % 120 == 0) {  // Log every 2 seconds
        LOG_DEBUG("Broad phase: {} colliders in {} cells, {} potential pairs", 
                 total_colliders, spatial_stats.occupied_cells, pairs.size());
    }
}

void PhysicsWorld2D::narrow_phase_collision_detection() {
    // Clear previous frame's contacts but preserve cache for coherency
    contact_manifolds_.clear();
    
    // Get potential pairs from broad phase
    const auto& potential_pairs = spatial_hash_->get_potential_pairs();
    
    for (const auto& [entity_a, entity_b] : potential_pairs) {
        ++stats_.narrow_phase_tests;
        
        // Get components for both entities
        auto* transform_a = registry_->get_component<Transform>(entity_a);
        auto* collider_a = registry_->get_component<Collider2D>(entity_a);
        auto* transform_b = registry_->get_component<Transform>(entity_b);
        auto* collider_b = registry_->get_component<Collider2D>(entity_b);
        
        if (!transform_a || !collider_a || !transform_b || !collider_b) continue;
        
        // Check collision layer compatibility
        if (!collider_a->can_collide_with(*collider_b)) continue;
        
        // Check contact cache for coherency optimization
        u64 contact_key = get_contact_key(entity_a, entity_b);
        bool found_in_cache = contact_cache_.find(contact_key) != contact_cache_.end();
        
        if (found_in_cache) {
            ++stats_.contact_cache_hits;
        } else {
            ++stats_.contact_cache_misses;
        }
        
        // Perform narrow-phase collision detection
        auto contact_manifold = create_contact_manifold(entity_a, entity_b);
        
        if (contact_manifold.has_value()) {
            contact_manifolds_.push_back(contact_manifold.value());
            
            // Update contact cache
            contact_cache_[contact_key] = contact_manifolds_.size() - 1;
            
            // Count contact types
            if (contact_manifold->is_new_contact) {
                ++stats_.new_contacts;
                
                // Fire collision enter event
                PhysicsEvent event(PhysicsEventType::CollisionEnter, entity_a, entity_b);
                event.contact_point = contact_manifold->contact_points[0];
                event.contact_normal = contact_manifold->contact_normal;
                event.timestamp = current_physics_time_;
                fire_event(event);
            } else {
                ++stats_.persistent_contacts;
                
                // Fire collision stay event
                PhysicsEvent event(PhysicsEventType::CollisionStay, entity_a, entity_b);
                event.contact_point = contact_manifold->contact_points[0];
                event.contact_normal = contact_manifold->contact_normal;
                event.timestamp = current_physics_time_;
                fire_event(event);
            }
        }
    }
    
    // Update statistics
    stats_.active_contacts = static_cast<u32>(contact_manifolds_.size());
    
    if (config_.enable_profiling && stats_.total_steps % 60 == 0) {
        LOG_DEBUG("Narrow phase: {} tests, {} active contacts ({} new, {} persistent)", 
                 stats_.narrow_phase_tests, stats_.active_contacts, 
                 stats_.new_contacts, stats_.persistent_contacts);
    }
}

void PhysicsWorld2D::solve_contacts() {
    if (contact_manifolds_.empty()) return;
    
    // Sequential impulse method with warm starting
    for (u32 iteration = 0; iteration < config_.constraint_iterations; ++iteration) {
        for (auto& manifold : contact_manifolds_) {
            // Get rigid bodies for both entities
            auto* rigidbody_a = registry_->get_component<RigidBody2D>(manifold.entity_a);
            auto* rigidbody_b = registry_->get_component<RigidBody2D>(manifold.entity_b);
            auto* transform_a = registry_->get_component<Transform>(manifold.entity_a);
            auto* transform_b = registry_->get_component<Transform>(manifold.entity_b);
            
            if (!rigidbody_a || !rigidbody_b || !transform_a || !transform_b) continue;
            
            // Skip if both bodies are static/kinematic
            if ((rigidbody_a->physics_flags.is_static || rigidbody_a->physics_flags.is_kinematic) &&
                (rigidbody_b->physics_flags.is_static || rigidbody_b->physics_flags.is_kinematic)) {
                continue;
            }
            
            // Solve each contact point in the manifold
            for (u32 i = 0; i < manifold.contact_count; ++i) {
                Vec2 contact_point = manifold.contact_points[i];
                Vec2 normal = manifold.contact_normal;
                f32 penetration = manifold.penetration_depths[i];
                
                // Calculate relative contact positions
                Vec2 ra = contact_point - transform_a->position;
                Vec2 rb = contact_point - transform_b->position;
                
                // Calculate relative velocity at contact point
                Vec2 va = rigidbody_a->velocity + vec2::cross(rigidbody_a->angular_velocity, ra);
                Vec2 vb = rigidbody_b->velocity + vec2::cross(rigidbody_b->angular_velocity, rb);
                Vec2 relative_velocity = va - vb;
                
                // Calculate relative velocity along normal
                f32 velocity_along_normal = relative_velocity.dot(normal);
                
                // Don't resolve if velocities are separating
                if (velocity_along_normal > 0) continue;
                
                // Calculate restitution and effective mass
                f32 restitution = manifold.restitution;
                
                // Calculate effective mass for impulse calculation
                f32 ra_cross_n = vec2::cross(ra, normal);
                f32 rb_cross_n = vec2::cross(rb, normal);
                
                f32 inv_mass_sum = rigidbody_a->inverse_mass + rigidbody_b->inverse_mass;
                f32 inv_inertia_sum = ra_cross_n * ra_cross_n * rigidbody_a->inverse_moment_of_inertia +
                                     rb_cross_n * rb_cross_n * rigidbody_b->inverse_moment_of_inertia;
                
                f32 effective_mass = inv_mass_sum + inv_inertia_sum;
                if (effective_mass < constants::EPSILON) continue;
                
                // Calculate impulse magnitude
                f32 impulse_magnitude = -(1.0f + restitution) * velocity_along_normal / effective_mass;
                
                // Apply normal impulse
                Vec2 impulse = normal * impulse_magnitude;
                
                if (!rigidbody_a->physics_flags.is_static && !rigidbody_a->physics_flags.is_kinematic) {
                    rigidbody_a->velocity += impulse * rigidbody_a->inverse_mass;
                    rigidbody_a->angular_velocity += vec2::cross(ra, impulse) * rigidbody_a->inverse_moment_of_inertia;
                    rigidbody_a->wake_up();
                }
                
                if (!rigidbody_b->physics_flags.is_static && !rigidbody_b->physics_flags.is_kinematic) {
                    rigidbody_b->velocity -= impulse * rigidbody_b->inverse_mass;
                    rigidbody_b->angular_velocity -= vec2::cross(rb, impulse) * rigidbody_b->inverse_moment_of_inertia;
                    rigidbody_b->wake_up();
                }
                
                // Apply friction impulse
                Vec2 tangent = relative_velocity - normal * velocity_along_normal;
                if (tangent.length_squared() > constants::EPSILON * constants::EPSILON) {
                    tangent = tangent.normalized();
                    
                    f32 friction_coefficient = std::sqrt(manifold.friction * manifold.friction);
                    f32 tangent_velocity = relative_velocity.dot(tangent);
                    
                    f32 ra_cross_t = vec2::cross(ra, tangent);
                    f32 rb_cross_t = vec2::cross(rb, tangent);
                    
                    f32 friction_effective_mass = inv_mass_sum + 
                                                ra_cross_t * ra_cross_t * rigidbody_a->inverse_moment_of_inertia +
                                                rb_cross_t * rb_cross_t * rigidbody_b->inverse_moment_of_inertia;
                    
                    if (friction_effective_mass > constants::EPSILON) {
                        f32 friction_impulse = -tangent_velocity / friction_effective_mass;
                        f32 max_friction = friction_coefficient * impulse_magnitude;
                        friction_impulse = std::clamp(friction_impulse, -max_friction, max_friction);
                        
                        Vec2 friction_force = tangent * friction_impulse;
                        
                        if (!rigidbody_a->physics_flags.is_static && !rigidbody_a->physics_flags.is_kinematic) {
                            rigidbody_a->velocity += friction_force * rigidbody_a->inverse_mass;
                            rigidbody_a->angular_velocity += vec2::cross(ra, friction_force) * rigidbody_a->inverse_moment_of_inertia;
                        }
                        
                        if (!rigidbody_b->physics_flags.is_static && !rigidbody_b->physics_flags.is_kinematic) {
                            rigidbody_b->velocity -= friction_force * rigidbody_b->inverse_mass;
                            rigidbody_b->angular_velocity -= vec2::cross(rb, friction_force) * rigidbody_b->inverse_moment_of_inertia;
                        }
                    }
                }
                
                // Position correction to prevent sinking (Baumgarte stabilization)
                if (penetration > constants::LINEAR_SLOP) {
                    f32 correction_percentage = 0.8f;  // Educational: try different values
                    f32 slop = constants::LINEAR_SLOP;
                    f32 correction_magnitude = correction_percentage * (penetration - slop) / effective_mass;
                    Vec2 correction = normal * correction_magnitude;
                    
                    if (!rigidbody_a->physics_flags.is_static && !rigidbody_a->physics_flags.is_kinematic) {
                        transform_a->position += correction * rigidbody_a->inverse_mass;
                    }
                    
                    if (!rigidbody_b->physics_flags.is_static && !rigidbody_b->physics_flags.is_kinematic) {
                        transform_b->position -= correction * rigidbody_b->inverse_mass;
                    }
                }
            }
            
            // Update manifold lifetime
            manifold.lifetime += config_.time_step;
        }
    }
    
    stats_.constraints_solved = static_cast<u32>(contact_manifolds_.size());
}

void PhysicsWorld2D::integrate_velocities() {
    for (Entity entity : active_entities_) {
        auto* rigidbody = registry_->get_component<RigidBody2D>(entity);
        auto* transform = registry_->get_component<Transform>(entity);
        
        if (!rigidbody || !transform) continue;
        if (rigidbody->physics_flags.is_kinematic || rigidbody->physics_flags.is_static) continue;
        
        // Store previous position for Verlet integration if needed
        rigidbody->previous_position = transform->position;
        rigidbody->previous_rotation = transform->rotation;
        
        // Semi-implicit Euler integration: x_new = x_old + v * dt
        if (!rigidbody->physics_flags.freeze_position_x) {
            transform->position.x += rigidbody->velocity.x * config_.time_step;
        }
        if (!rigidbody->physics_flags.freeze_position_y) {
            transform->position.y += rigidbody->velocity.y * config_.time_step;
        }
        
        if (!rigidbody->physics_flags.freeze_rotation) {
            transform->rotation += rigidbody->angular_velocity * config_.time_step;
            transform->rotation = utils::normalize_angle(transform->rotation);
        }
        
        // Update any motion state cache if present
        auto* motion_state = registry_->get_component<MotionState>(entity);
        if (motion_state) {
            motion_state->invalidate_all();
        }
    }
}

void PhysicsWorld2D::update_sleeping_system() {
    if (!config_.enable_sleeping) return;
    
    for (Entity entity : active_entities_) {
        auto* rigidbody = registry_->get_component<RigidBody2D>(entity);
        if (!rigidbody || rigidbody->physics_flags.is_static || rigidbody->physics_flags.is_kinematic) continue;
        
        // Check if body should go to sleep
        if (rigidbody->should_be_sleeping()) {
            rigidbody->sleep_timer += config_.time_step;
            
            if (rigidbody->sleep_timer >= config_.sleep_time_threshold) {
                rigidbody->put_to_sleep();
                
                // Fire sleep event
                PhysicsEvent event(PhysicsEventType::BodySleep, entity);
                event.timestamp = current_physics_time_;
                fire_event(event);
            }
        } else {
            rigidbody->sleep_timer = 0.0f;
        }
    }
}

void PhysicsWorld2D::process_collision_events() {
    // Process all queued events
    for (const auto& event : event_queue_) {
        for (const auto& callback : event_callbacks_) {
            callback(event);
        }
    }
}

void PhysicsWorld2D::update_statistics() {
    stats_.current_time = current_physics_time_;
    stats_.update_derived_stats();
    
    // Update memory statistics
    if (physics_arena_) {
        stats_.arena_memory_used = physics_arena_->used_size();
        stats_.arena_memory_peak = std::max(stats_.arena_memory_peak, stats_.arena_memory_used);
        stats_.total_physics_memory += stats_.arena_memory_used;
    }
    
    if (contact_pool_) {
        stats_.contact_pool_usage = contact_pool_->allocated_count();
        stats_.pool_memory_used += contact_pool_->allocated_count() * contact_pool_->block_size();
    }
    
    if (collision_pair_pool_) {
        stats_.collision_pair_pool_usage = collision_pair_pool_->allocated_count();
        stats_.pool_memory_used += collision_pair_pool_->allocated_count() * collision_pair_pool_->block_size();
    }
    
    // Educational: Calculate conservation violations
    if (stats_.total_steps > 1) {
        // Simple energy drift detection (would need more sophisticated tracking in real simulation)
        static f32 last_total_energy = 0.0f;
        f32 current_total_energy = stats_.total_kinetic_energy + stats_.total_potential_energy;
        stats_.energy_conservation_error = std::abs(current_total_energy - last_total_energy);
        last_total_energy = current_total_energy;
        
        // Reset per-frame energy totals
        stats_.total_kinetic_energy = 0.0f;
        stats_.total_potential_energy = 0.0f;
    }
}

u64 PhysicsWorld2D::get_contact_key(Entity a, Entity b) {
    if (a > b) std::swap(a, b);
    return (static_cast<u64>(a) << 32) | b;
}

void PhysicsWorld2D::fire_event(const PhysicsEvent& event) {
    event_queue_.push_back(event);
}

std::optional<PhysicsWorld2D::ContactManifold> PhysicsWorld2D::create_contact_manifold(Entity a, Entity b) {
    // Get required components
    auto* transform_a = registry_->get_component<Transform>(a);
    auto* collider_a = registry_->get_component<Collider2D>(a);
    auto* transform_b = registry_->get_component<Transform>(b);
    auto* collider_b = registry_->get_component<Collider2D>(b);
    
    if (!transform_a || !collider_a || !transform_b || !collider_b) {
        return std::nullopt;
    }
    
    // Get world-space collision shapes
    CollisionShape shape_a = collider_a->get_world_shape(*transform_a);
    CollisionShape shape_b = collider_b->get_world_shape(*transform_b);
    
    // Perform collision detection based on shape types
    // This is a simplified implementation - a full physics engine would have
    // specialized algorithms for each shape pair combination
    
    collision::DistanceResult result;
    bool collision_detected = false;
    
    // Circle-Circle collision (most common case)
    if (std::holds_alternative<Circle>(shape_a) && std::holds_alternative<Circle>(shape_b)) {
        const Circle& circle_a = std::get<Circle>(shape_a);
        const Circle& circle_b = std::get<Circle>(shape_b);
        
        result = collision::distance_circle_to_circle(circle_a, circle_b);
        collision_detected = result.is_overlapping;
    }
    // AABB-AABB collision
    else if (std::holds_alternative<AABB>(shape_a) && std::holds_alternative<AABB>(shape_b)) {
        const AABB& aabb_a = std::get<AABB>(shape_a);
        const AABB& aabb_b = std::get<AABB>(shape_b);
        
        result = collision::distance_aabb_to_aabb(aabb_a, aabb_b);
        collision_detected = result.is_overlapping;
    }
    // Circle-AABB collision
    else if (std::holds_alternative<Circle>(shape_a) && std::holds_alternative<AABB>(shape_b)) {
        const Circle& circle = std::get<Circle>(shape_a);
        const AABB& aabb = std::get<AABB>(shape_b);
        
        result = collision::distance_circle_to_aabb(circle, aabb);
        collision_detected = result.is_overlapping;
    }
    else if (std::holds_alternative<AABB>(shape_a) && std::holds_alternative<Circle>(shape_b)) {
        const AABB& aabb = std::get<AABB>(shape_a);
        const Circle& circle = std::get<Circle>(shape_b);
        
        result = collision::distance_circle_to_aabb(circle, aabb);
        collision_detected = result.is_overlapping;
        // Flip normal since we swapped the shapes
        result.normal = -result.normal;
    }
    
    if (!collision_detected) {
        return std::nullopt;
    }
    
    // Create contact manifold
    ContactManifold manifold;
    manifold.entity_a = a;
    manifold.entity_b = b;
    manifold.contact_normal = result.normal;
    manifold.contact_count = 1;  // Simplified: single contact point
    manifold.contact_points[0] = result.point_a;  // Contact point on surface A
    manifold.penetration_depths[0] = -result.distance;  // Negative distance = penetration
    
    // Calculate combined material properties
    PhysicsMaterial combined_material = PhysicsMaterial::combine(collider_a->material, collider_b->material);
    manifold.friction = combined_material.static_friction;
    manifold.restitution = combined_material.restitution;
    
    // Check if this is a new contact
    u64 contact_key = get_contact_key(a, b);
    manifold.is_new_contact = contact_cache_.find(contact_key) == contact_cache_.end();
    
    return manifold;
}

// Additional utility functions and other method implementations...

bool PhysicsWorld2D::add_entity(Entity entity) {
    // Validate that entity has required components
    if (!registry_->has_component<Transform>(entity) || 
        !registry_->has_component<RigidBody2D>(entity)) {
        LOG_WARN("Cannot add entity {} to physics world: missing required components (Transform, RigidBody2D)", entity);
        return false;
    }
    
    // Entity is automatically included in physics simulation through ECS queries
    LOG_DEBUG("Entity {} added to physics world", entity);
    return true;
}

bool PhysicsWorld2D::remove_entity(Entity entity) {
    // Remove from contact cache
    std::vector<u64> keys_to_remove;
    for (const auto& [key, index] : contact_cache_) {
        Entity a = static_cast<Entity>(key >> 32);
        Entity b = static_cast<Entity>(key & 0xFFFFFFFF);
        if (a == entity || b == entity) {
            keys_to_remove.push_back(key);
        }
    }
    
    for (u64 key : keys_to_remove) {
        contact_cache_.erase(key);
    }
    
    // Remove from active entities list
    active_entities_.erase(std::remove(active_entities_.begin(), active_entities_.end(), entity), active_entities_.end());
    sleeping_entities_.erase(std::remove(sleeping_entities_.begin(), sleeping_entities_.end(), entity), sleeping_entities_.end());
    entities_to_wake_.erase(entity);
    
    LOG_DEBUG("Entity {} removed from physics world", entity);
    return true;
}

void PhysicsWorld2D::apply_force(Entity entity, const Vec2& force) {
    auto* forces = registry_->get_component<ForceAccumulator>(entity);
    if (forces) {
        forces->apply_force(force, "External");
    }
}

void PhysicsWorld2D::apply_force_at_point(Entity entity, const Vec2& force, const Vec2& world_point) {
    auto* forces = registry_->get_component<ForceAccumulator>(entity);
    auto* transform = registry_->get_component<Transform>(entity);
    
    if (forces && transform) {
        Vec2 local_point = world_point - transform->position;
        forces->apply_force_at_point(force, local_point, "External");
    }
}

void PhysicsWorld2D::wake_entity(Entity entity) {
    entities_to_wake_.insert(entity);
}

std::string PhysicsWorld2D::generate_performance_report() const {
    return stats_.generate_report();
}

} // namespace ecscope::physics