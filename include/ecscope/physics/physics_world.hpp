#pragma once

#include "physics_math.hpp"
#include "rigid_body.hpp"
#include "collision_detection.hpp"
#include "narrow_phase.hpp"
#include "constraints.hpp"
#include "materials.hpp"
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>

namespace ecscope::physics {

// Force generators for physics simulation
class ForceGenerator {
public:
    virtual ~ForceGenerator() = default;
    virtual void apply_force(RigidBody2D& body, Real dt) {}
    virtual void apply_force(RigidBody3D& body, Real dt) {}
    virtual bool is_active() const { return true; }
};

// Gravity force generator
class GravityForce : public ForceGenerator {
private:
    Vec3 gravity_3d;
    Vec2 gravity_2d;
    
public:
    GravityForce(const Vec3& g) : gravity_3d(g), gravity_2d(g.x, g.y) {}
    GravityForce(const Vec2& g) : gravity_2d(g), gravity_3d(g.x, g.y, 0) {}
    
    void apply_force(RigidBody2D& body, Real dt) override {
        if (body.type == BodyType::Dynamic && !body.is_sleeping) {
            body.apply_force(gravity_2d * body.mass);
        }
    }
    
    void apply_force(RigidBody3D& body, Real dt) override {
        if (body.type == BodyType::Dynamic && !body.is_sleeping) {
            body.apply_force(gravity_3d * body.mass_props.mass);
        }
    }
    
    void set_gravity(const Vec3& g) { gravity_3d = g; gravity_2d = Vec2(g.x, g.y); }
    Vec3 get_gravity_3d() const { return gravity_3d; }
    Vec2 get_gravity_2d() const { return gravity_2d; }
};

// Wind force generator
class WindForce : public ForceGenerator {
private:
    Vec3 wind_velocity;
    Real air_density = 1.225f;  // kg/m³ at sea level
    Real drag_coefficient = 0.47f;  // Sphere drag coefficient
    
public:
    WindForce(const Vec3& velocity, Real density = 1.225f, Real drag_coef = 0.47f)
        : wind_velocity(velocity), air_density(density), drag_coefficient(drag_coef) {}
    
    void apply_force(RigidBody3D& body, Real dt) override {
        if (body.type != BodyType::Dynamic || body.is_sleeping) return;
        
        Vec3 relative_velocity = wind_velocity - body.velocity;
        Real velocity_squared = relative_velocity.length_squared();
        
        if (velocity_squared > PHYSICS_EPSILON) {
            Vec3 drag_direction = relative_velocity.normalized();
            
            // Simplified drag force: F = 0.5 * ρ * Cd * A * v²
            // Assume area proportional to mass (rough approximation)
            Real area = std::pow(body.mass_props.mass, 2.0f/3.0f) * 0.1f;  // Rough scaling
            Real drag_magnitude = 0.5f * air_density * drag_coefficient * area * velocity_squared;
            
            body.apply_force(drag_direction * drag_magnitude);
        }
    }
    
    void set_wind_velocity(const Vec3& velocity) { wind_velocity = velocity; }
    Vec3 get_wind_velocity() const { return wind_velocity; }
};

// Buoyancy force generator
class BuoyancyForce : public ForceGenerator {
private:
    Real fluid_density;
    Real fluid_height;
    Vec3 fluid_surface_normal = Vec3::unit_y();
    
public:
    BuoyancyForce(Real density, Real height) : fluid_density(density), fluid_height(height) {}
    
    void apply_force(RigidBody3D& body, Real dt) override {
        if (body.type != BodyType::Dynamic || body.is_sleeping) return;
        
        // Simple approximation: check if body center is below fluid surface
        if (body.transform.position.y < fluid_height) {
            // Calculate submerged volume (simplified as full volume if center is submerged)
            Real volume = body.mass_props.mass / Materials::Water().density;  // Approximate volume
            
            // Buoyant force = fluid_density * volume * gravity
            Vec3 buoyant_force = Vec3(0, fluid_density * volume * 9.81f, 0);
            body.apply_force(buoyant_force);
            
            // Apply fluid damping
            body.velocity *= 0.95f;  // Simple fluid resistance
            body.angular_velocity *= 0.95f;
        }
    }
};

// Physics world configuration
struct PhysicsWorldConfig {
    // Simulation parameters
    Real gravity_scale = 1.0f;
    Vec3 gravity = Vec3(0, -9.81f, 0);
    Real time_scale = 1.0f;
    
    // Solver settings
    int velocity_iterations = 8;
    int position_iterations = 3;
    Real linear_slop = 0.005f;       // Linear position tolerance
    Real angular_slop = 0.0174533f;  // Angular position tolerance (1 degree)
    
    // Sleep settings
    bool allow_sleep = true;
    Real sleep_threshold = 0.01f;
    Real sleep_time_threshold = 0.5f;
    
    // Performance settings
    size_t max_contacts_per_body = 64;
    size_t max_joint_iterations = 20;
    Real broad_phase_margin = 0.1f;
    
    // Threading
    bool use_multithreading = true;
    size_t worker_thread_count = 0;  // 0 = auto-detect
    
    // Debug settings
    bool enable_debug_draw = false;
    bool enable_profiling = false;
    bool enable_continuous_collision = true;
    
    PhysicsWorldConfig() = default;
};

// Physics statistics for performance monitoring
struct PhysicsStats {
    // Timing information
    Real total_time = 0.0f;
    Real broad_phase_time = 0.0f;
    Real narrow_phase_time = 0.0f;
    Real constraint_solving_time = 0.0f;
    Real integration_time = 0.0f;
    
    // Object counts
    size_t active_bodies = 0;
    size_t sleeping_bodies = 0;
    size_t total_shapes = 0;
    size_t collision_pairs = 0;
    size_t active_contacts = 0;
    size_t active_constraints = 0;
    
    // Performance metrics
    Real fps = 0.0f;
    size_t memory_usage_bytes = 0;
    Real efficiency_ratio = 0.0f;  // Ratio of actual pairs to max possible pairs
    
    void reset() {
        total_time = broad_phase_time = narrow_phase_time = 0.0f;
        constraint_solving_time = integration_time = 0.0f;
        active_bodies = sleeping_bodies = total_shapes = 0;
        collision_pairs = active_contacts = active_constraints = 0;
        fps = memory_usage_bytes = efficiency_ratio = 0.0f;
    }
};

// Main physics world class
class PhysicsWorld {
private:
    // Configuration
    PhysicsWorldConfig config;
    bool is_2d_mode = false;
    
    // Time management
    Real accumulated_time = 0.0f;
    Real fixed_time_step = 1.0f / 60.0f;  // 60 FPS physics
    uint64_t step_count = 0;
    
    // Bodies and shapes
    std::vector<RigidBody2D> bodies_2d;
    std::vector<RigidBody3D> bodies_3d;
    std::unordered_map<uint32_t, std::unique_ptr<Shape>> shapes;
    std::unordered_map<uint32_t, std::string> body_materials;  // body_id -> material_name
    
    // Collision detection
    std::unique_ptr<BroadPhaseCollisionDetection> broad_phase;
    std::vector<ContactManifold> contact_manifolds;
    
    // Constraint solving
    ConstraintSolver constraint_solver;
    std::vector<std::unique_ptr<ContactConstraint>> contact_constraints;
    
    // Force generators
    std::vector<std::unique_ptr<ForceGenerator>> force_generators;
    std::unique_ptr<GravityForce> gravity_generator;
    
    // Threading
    std::vector<std::thread> worker_threads;
    std::atomic<bool> threads_active{false};
    std::mutex world_mutex;
    
    // Statistics and profiling
    PhysicsStats stats;
    std::chrono::high_resolution_clock::time_point last_frame_time;
    
    // Event callbacks
    std::function<void(uint32_t, uint32_t, const ContactManifold&)> collision_callback;
    std::function<void(uint32_t, uint32_t)> trigger_callback;
    
    uint32_t next_body_id = 1;
    
public:
    explicit PhysicsWorld(bool is_2d = false) : is_2d_mode(is_2d) {
        initialize();
    }
    
    ~PhysicsWorld() {
        shutdown();
    }
    
    // Initialization and configuration
    void initialize() {
        broad_phase = std::make_unique<BroadPhaseCollisionDetection>();
        gravity_generator = std::make_unique<GravityForce>(config.gravity * config.gravity_scale);
        force_generators.push_back(std::unique_ptr<ForceGenerator>(gravity_generator.get()));
        
        last_frame_time = std::chrono::high_resolution_clock::now();
        
        // Initialize worker threads if multithreading is enabled
        if (config.use_multithreading) {
            size_t thread_count = config.worker_thread_count;
            if (thread_count == 0) {
                thread_count = std::thread::hardware_concurrency();
                thread_count = std::max(1u, thread_count - 1);  // Leave one core for main thread
            }
            
            threads_active = true;
            worker_threads.reserve(thread_count);
            for (size_t i = 0; i < thread_count; ++i) {
                worker_threads.emplace_back([this]() {
                    worker_thread_function();
                });
            }
        }
    }
    
    void shutdown() {
        if (config.use_multithreading) {
            threads_active = false;
            for (auto& thread : worker_threads) {
                if (thread.joinable()) {
                    thread.join();
                }
            }
            worker_threads.clear();
        }
        
        clear();
    }
    
    void set_config(const PhysicsWorldConfig& new_config) {
        config = new_config;
        constraint_solver.set_iterations(config.position_iterations, config.velocity_iterations);
        gravity_generator->set_gravity(config.gravity * config.gravity_scale);
    }
    
    const PhysicsWorldConfig& get_config() const { return config; }
    
    // Body management
    uint32_t create_body_2d(const Transform2D& transform, BodyType type = BodyType::Dynamic) {
        if (!is_2d_mode) {
            // Auto-switch to 2D mode if not set
            is_2d_mode = true;
        }
        
        uint32_t id = next_body_id++;
        bodies_2d.emplace_back(type);
        bodies_2d.back().id = id;
        bodies_2d.back().transform = transform;
        
        return id;
    }
    
    uint32_t create_body_3d(const Transform3D& transform, BodyType type = BodyType::Dynamic) {
        if (is_2d_mode) {
            is_2d_mode = false;  // Switch to 3D mode
            bodies_2d.clear();   // Clear 2D bodies (or migrate them)
        }
        
        uint32_t id = next_body_id++;
        bodies_3d.emplace_back(type);
        bodies_3d.back().id = id;
        bodies_3d.back().transform = transform;
        
        return id;
    }
    
    bool remove_body(uint32_t body_id) {
        if (is_2d_mode) {
            auto it = std::find_if(bodies_2d.begin(), bodies_2d.end(),
                [body_id](const RigidBody2D& body) { return body.id == body_id; });
            if (it != bodies_2d.end()) {
                bodies_2d.erase(it);
                shapes.erase(body_id);
                body_materials.erase(body_id);
                return true;
            }
        } else {
            auto it = std::find_if(bodies_3d.begin(), bodies_3d.end(),
                [body_id](const RigidBody3D& body) { return body.id == body_id; });
            if (it != bodies_3d.end()) {
                bodies_3d.erase(it);
                shapes.erase(body_id);
                body_materials.erase(body_id);
                return true;
            }
        }
        return false;
    }
    
    RigidBody2D* get_body_2d(uint32_t body_id) {
        auto it = std::find_if(bodies_2d.begin(), bodies_2d.end(),
            [body_id](const RigidBody2D& body) { return body.id == body_id; });
        return it != bodies_2d.end() ? &(*it) : nullptr;
    }
    
    RigidBody3D* get_body_3d(uint32_t body_id) {
        auto it = std::find_if(bodies_3d.begin(), bodies_3d.end(),
            [body_id](const RigidBody3D& body) { return body.id == body_id; });
        return it != bodies_3d.end() ? &(*it) : nullptr;
    }
    
    void set_body_shape(uint32_t body_id, std::unique_ptr<Shape> shape) {
        shapes[body_id] = std::move(shape);
    }
    
    void set_body_material(uint32_t body_id, const std::string& material_name) {
        body_materials[body_id] = material_name;
        
        // Apply material properties to body
        const PhysicsMaterial* material = get_material(material_name);
        if (material) {
            if (is_2d_mode) {
                RigidBody2D* body = get_body_2d(body_id);
                if (body) {
                    body->material = *material;
                }
            } else {
                RigidBody3D* body = get_body_3d(body_id);
                if (body) {
                    body->material = *material;
                }
            }
        }
    }
    
    Shape* get_body_shape(uint32_t body_id) {
        auto it = shapes.find(body_id);
        return it != shapes.end() ? it->second.get() : nullptr;
    }
    
    // Force generators
    void add_force_generator(std::unique_ptr<ForceGenerator> generator) {
        force_generators.push_back(std::move(generator));
    }
    
    void set_gravity(const Vec3& gravity) {
        config.gravity = gravity;
        gravity_generator->set_gravity(gravity * config.gravity_scale);
    }
    
    Vec3 get_gravity() const { return config.gravity * config.gravity_scale; }
    
    // Constraints
    void add_constraint(std::unique_ptr<Constraint> constraint) {
        constraint_solver.add_constraint(std::move(constraint));
    }
    
    // Simulation stepping
    void step(Real dt) {
        auto frame_start = std::chrono::high_resolution_clock::now();
        
        // Apply time scale
        dt *= config.time_scale;
        
        if (dt <= 0.0f) return;
        
        accumulated_time += dt;
        
        // Fixed time step simulation for determinism
        while (accumulated_time >= fixed_time_step) {
            step_fixed(fixed_time_step);
            accumulated_time -= fixed_time_step;
            step_count++;
        }
        
        // Update statistics
        auto frame_end = std::chrono::high_resolution_clock::now();
        stats.total_time = std::chrono::duration<Real>(frame_end - frame_start).count();
        stats.fps = 1.0f / stats.total_time;
        
        last_frame_time = frame_end;
    }
    
    void step_fixed(Real dt) {
        stats.reset();
        
        auto step_start = std::chrono::high_resolution_clock::now();
        
        // 1. Apply forces
        apply_forces(dt);
        
        // 2. Broad phase collision detection
        auto broad_start = std::chrono::high_resolution_clock::now();
        update_broad_phase();
        auto broad_end = std::chrono::high_resolution_clock::now();
        stats.broad_phase_time = std::chrono::duration<Real>(broad_end - broad_start).count();
        
        // 3. Narrow phase collision detection
        auto narrow_start = std::chrono::high_resolution_clock::now();
        update_narrow_phase();
        auto narrow_end = std::chrono::high_resolution_clock::now();
        stats.narrow_phase_time = std::chrono::duration<Real>(narrow_end - narrow_start).count();
        
        // 4. Integrate forces (velocity integration)
        integrate_forces(dt);
        
        // 5. Solve constraints
        auto constraint_start = std::chrono::high_resolution_clock::now();
        solve_constraints(dt);
        auto constraint_end = std::chrono::high_resolution_clock::now();
        stats.constraint_solving_time = std::chrono::duration<Real>(constraint_end - constraint_start).count();
        
        // 6. Integrate velocities (position integration)
        auto integration_start = std::chrono::high_resolution_clock::now();
        integrate_velocities(dt);
        auto integration_end = std::chrono::high_resolution_clock::now();
        stats.integration_time = std::chrono::duration<Real>(integration_end - integration_start).count();
        
        // 7. Update sleep states
        update_sleep_states(dt);
        
        // 8. Update statistics
        update_statistics();
        
        auto step_end = std::chrono::high_resolution_clock::now();
        stats.total_time = std::chrono::duration<Real>(step_end - step_start).count();
    }
    
    // Query functions
    std::vector<uint32_t> query_aabb_2d(const AABB2D& aabb) const {
        std::vector<uint32_t> results;
        // Implementation would use broad phase spatial structure
        return results;
    }
    
    std::vector<uint32_t> query_aabb_3d(const AABB3D& aabb) const {
        std::vector<uint32_t> results;
        // Implementation would use broad phase spatial structure
        return results;
    }
    
    bool raycast_2d(const Vec2& origin, const Vec2& direction, Real max_distance,
                   uint32_t& hit_body, Vec2& hit_point, Vec2& hit_normal) const {
        // Raycast implementation
        return false;  // Placeholder
    }
    
    bool raycast_3d(const Vec3& origin, const Vec3& direction, Real max_distance,
                   uint32_t& hit_body, Vec3& hit_point, Vec3& hit_normal) const {
        // Raycast implementation
        return false;  // Placeholder
    }
    
    // Event callbacks
    void set_collision_callback(std::function<void(uint32_t, uint32_t, const ContactManifold&)> callback) {
        collision_callback = callback;
    }
    
    void set_trigger_callback(std::function<void(uint32_t, uint32_t)> callback) {
        trigger_callback = callback;
    }
    
    // Statistics and debugging
    const PhysicsStats& get_stats() const { return stats; }
    
    void clear() {
        bodies_2d.clear();
        bodies_3d.clear();
        shapes.clear();
        body_materials.clear();
        contact_manifolds.clear();
        contact_constraints.clear();
        constraint_solver.clear_constraints();
        force_generators.clear();
        force_generators.push_back(std::unique_ptr<ForceGenerator>(gravity_generator.get()));
        
        accumulated_time = 0.0f;
        step_count = 0;
        next_body_id = 1;
        
        if (broad_phase) {
            broad_phase->clear();
        }
    }
    
    size_t get_body_count() const {
        return is_2d_mode ? bodies_2d.size() : bodies_3d.size();
    }
    
    bool is_2d() const { return is_2d_mode; }
    uint64_t get_step_count() const { return step_count; }
    Real get_fixed_time_step() const { return fixed_time_step; }
    void set_fixed_time_step(Real dt) { fixed_time_step = std::max(dt, 1e-6f); }
    
private:
    void apply_forces(Real dt) {
        if (is_2d_mode) {
            for (auto& body : bodies_2d) {
                for (auto& generator : force_generators) {
                    if (generator->is_active()) {
                        generator->apply_force(body, dt);
                    }
                }
            }
        } else {
            for (auto& body : bodies_3d) {
                for (auto& generator : force_generators) {
                    if (generator->is_active()) {
                        generator->apply_force(body, dt);
                    }
                }
            }
        }
    }
    
    void update_broad_phase() {
        broad_phase->clear();
        
        if (is_2d_mode) {
            for (const auto& body : bodies_2d) {
                Shape* shape = get_body_shape(body.id);
                if (shape) {
                    broad_phase->add_body_2d(body, *shape);
                }
            }
            broad_phase->find_collision_pairs_2d();
        } else {
            for (const auto& body : bodies_3d) {
                Shape* shape = get_body_shape(body.id);
                if (shape) {
                    broad_phase->add_body_3d(body, *shape);
                }
            }
            broad_phase->find_collision_pairs_3d();
        }
        
        stats.collision_pairs = broad_phase->get_stats().total_pairs;
        stats.efficiency_ratio = broad_phase->get_stats().efficiency_ratio;
    }
    
    void update_narrow_phase() {
        contact_manifolds.clear();
        contact_constraints.clear();
        
        const auto& collision_pairs = is_2d_mode ? 
            broad_phase->find_collision_pairs_2d() : broad_phase->find_collision_pairs_3d();
        
        for (const auto& pair : collision_pairs) {
            if (is_2d_mode) {
                RigidBody2D* body_a = get_body_2d(pair.id_a);
                RigidBody2D* body_b = get_body_2d(pair.id_b);
                Shape* shape_a = get_body_shape(pair.id_a);
                Shape* shape_b = get_body_shape(pair.id_b);
                
                if (body_a && body_b && shape_a && shape_b) {
                    auto collision_info = NarrowPhaseCollisionDetection::test_collision_2d(
                        *body_a, *shape_a, *body_b, *shape_b);
                    
                    if (collision_info.is_colliding) {
                        collision_info.manifold.body_a_id = pair.id_a;
                        collision_info.manifold.body_b_id = pair.id_b;
                        contact_manifolds.push_back(collision_info.manifold);
                        
                        // Generate contact constraints
                        generate_contact_constraints(collision_info.manifold);
                        
                        // Call collision callback
                        if (collision_callback) {
                            collision_callback(pair.id_a, pair.id_b, collision_info.manifold);
                        }
                    }
                }
            } else {
                RigidBody3D* body_a = get_body_3d(pair.id_a);
                RigidBody3D* body_b = get_body_3d(pair.id_b);
                Shape* shape_a = get_body_shape(pair.id_a);
                Shape* shape_b = get_body_shape(pair.id_b);
                
                if (body_a && body_b && shape_a && shape_b) {
                    auto collision_info = NarrowPhaseCollisionDetection::test_collision_3d(
                        *body_a, *shape_a, *body_b, *shape_b);
                    
                    if (collision_info.is_colliding) {
                        collision_info.manifold.body_a_id = pair.id_a;
                        collision_info.manifold.body_b_id = pair.id_b;
                        contact_manifolds.push_back(collision_info.manifold);
                        
                        // Generate contact constraints
                        generate_contact_constraints(collision_info.manifold);
                        
                        // Call collision callback
                        if (collision_callback) {
                            collision_callback(pair.id_a, pair.id_b, collision_info.manifold);
                        }
                    }
                }
            }
        }
        
        stats.active_contacts = contact_manifolds.size();
    }
    
    void generate_contact_constraints(const ContactManifold& manifold) {
        for (const auto& contact : manifold.contacts) {
            auto constraint = std::make_unique<ContactConstraint>(
                manifold.body_a_id, manifold.body_b_id,
                contact.position_a, contact.position_b,
                contact.normal, contact.penetration
            );
            
            constraint->friction_coefficient = manifold.friction;
            constraint->restitution_coefficient = manifold.restitution;
            
            contact_constraints.push_back(std::move(constraint));
        }
    }
    
    void integrate_forces(Real dt) {
        if (is_2d_mode) {
            for (auto& body : bodies_2d) {
                body.integrate_forces(dt);
            }
        } else {
            for (auto& body : bodies_3d) {
                body.integrate_forces(dt);
            }
        }
    }
    
    void solve_constraints(Real dt) {
        // Add contact constraints to solver temporarily
        for (auto& contact_constraint : contact_constraints) {
            constraint_solver.add_constraint(std::move(contact_constraint));
        }
        
        // Solve all constraints
        if (is_2d_mode) {
            // Convert 2D bodies to 3D for constraint solving (simplified)
            std::vector<RigidBody3D> temp_bodies_3d;
            for (const auto& body_2d : bodies_2d) {
                RigidBody3D body_3d;
                body_3d.id = body_2d.id;
                body_3d.transform.position = Vec3(body_2d.transform.position.x, body_2d.transform.position.y, 0);
                body_3d.velocity = Vec3(body_2d.velocity.x, body_2d.velocity.y, 0);
                body_3d.angular_velocity = Vec3(0, 0, body_2d.angular_velocity);
                body_3d.mass_props.set_mass(body_2d.mass);
                body_3d.material = body_2d.material;
                temp_bodies_3d.push_back(body_3d);
            }
            
            constraint_solver.solve_constraints(temp_bodies_3d, dt);
            
            // Convert back to 2D
            for (size_t i = 0; i < bodies_2d.size(); ++i) {
                bodies_2d[i].transform.position = Vec2(temp_bodies_3d[i].transform.position.x, temp_bodies_3d[i].transform.position.y);
                bodies_2d[i].velocity = Vec2(temp_bodies_3d[i].velocity.x, temp_bodies_3d[i].velocity.y);
                bodies_2d[i].angular_velocity = temp_bodies_3d[i].angular_velocity.z;
            }
        } else {
            constraint_solver.solve_constraints(bodies_3d, dt);
        }
        
        stats.active_constraints = constraint_solver.get_constraint_count();
    }
    
    void integrate_velocities(Real dt) {
        if (is_2d_mode) {
            for (auto& body : bodies_2d) {
                body.integrate_velocity(dt);
            }
        } else {
            for (auto& body : bodies_3d) {
                body.integrate_velocity(dt);
            }
        }
    }
    
    void update_sleep_states(Real dt) {
        if (!config.allow_sleep) return;
        
        if (is_2d_mode) {
            for (auto& body : bodies_2d) {
                if (body.can_sleep()) {
                    body.sleep_time += dt;
                    if (body.sleep_time > config.sleep_time_threshold) {
                        body.put_to_sleep();
                    }
                } else {
                    body.sleep_time = 0.0f;
                }
            }
        } else {
            for (auto& body : bodies_3d) {
                if (body.can_sleep()) {
                    body.sleep_time += dt;
                    if (body.sleep_time > config.sleep_time_threshold) {
                        body.put_to_sleep();
                    }
                } else {
                    body.sleep_time = 0.0f;
                }
            }
        }
    }
    
    void update_statistics() {
        stats.total_shapes = shapes.size();
        
        if (is_2d_mode) {
            stats.active_bodies = 0;
            stats.sleeping_bodies = 0;
            for (const auto& body : bodies_2d) {
                if (body.is_sleeping) {
                    stats.sleeping_bodies++;
                } else {
                    stats.active_bodies++;
                }
            }
        } else {
            stats.active_bodies = 0;
            stats.sleeping_bodies = 0;
            for (const auto& body : bodies_3d) {
                if (body.is_sleeping) {
                    stats.sleeping_bodies++;
                } else {
                    stats.active_bodies++;
                }
            }
        }
        
        // Calculate memory usage (rough estimate)
        stats.memory_usage_bytes = sizeof(PhysicsWorld) +
                                  bodies_2d.size() * sizeof(RigidBody2D) +
                                  bodies_3d.size() * sizeof(RigidBody3D) +
                                  shapes.size() * 256 +  // Rough estimate for shapes
                                  contact_manifolds.size() * sizeof(ContactManifold) +
                                  broad_phase->get_memory_usage();
    }
    
    void worker_thread_function() {
        while (threads_active) {
            // Thread work would be implemented here for parallel processing
            // This is a placeholder for future multithreading implementation
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
};

} // namespace ecscope::physics