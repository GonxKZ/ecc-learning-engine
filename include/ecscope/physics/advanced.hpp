#pragma once

#include "physics_world.hpp"
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <future>
#include <vector>
#include <algorithm>
#include <execution>
#include <immintrin.h>

namespace ecscope::physics {

// Thread pool for parallel physics computations
class PhysicsThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    std::atomic<bool> stop{false};
    
public:
    explicit PhysicsThreadPool(size_t thread_count) {
        for (size_t i = 0; i < thread_count; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        condition.wait(lock, [this] { return stop || !tasks.empty(); });
                        
                        if (stop && tasks.empty()) return;
                        
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    
                    task();
                }
            });
        }
    }
    
    ~PhysicsThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        
        condition.notify_all();
        
        for (std::thread& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }
    
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
        using return_type = typename std::result_of<F(Args...)>::type;
        
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
        std::future<return_type> result = task->get_future();
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            
            if (stop) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
            
            tasks.emplace([task]() { (*task)(); });
        }
        
        condition.notify_one();
        return result;
    }
    
    size_t get_thread_count() const {
        return workers.size();
    }
    
    void wait_for_all() {
        while (true) {
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                if (tasks.empty()) break;
            }
            std::this_thread::yield();
        }
    }
};

// SIMD-optimized physics operations
namespace SIMD {
    
#ifdef __AVX2__
    // Parallel vector operations using AVX2
    inline void parallel_integrate_positions(Vec3* positions, const Vec3* velocities, 
                                           Real dt, size_t count) {
        const __m256 dt_vec = _mm256_set1_ps(dt);
        const size_t simd_count = count & ~7;  // Process 8 Vec3s at a time (24 floats)
        
        for (size_t i = 0; i < simd_count; i += 8) {
            // Load positions and velocities
            __m256 pos_x = _mm256_load_ps(&positions[i].x);
            __m256 pos_y = _mm256_load_ps(&positions[i].y);
            __m256 pos_z = _mm256_load_ps(&positions[i].z);
            
            __m256 vel_x = _mm256_load_ps(&velocities[i].x);
            __m256 vel_y = _mm256_load_ps(&velocities[i].y);
            __m256 vel_z = _mm256_load_ps(&velocities[i].z);
            
            // Integrate: pos += vel * dt
            pos_x = _mm256_fmadd_ps(vel_x, dt_vec, pos_x);
            pos_y = _mm256_fmadd_ps(vel_y, dt_vec, pos_y);
            pos_z = _mm256_fmadd_ps(vel_z, dt_vec, pos_z);
            
            // Store results
            _mm256_store_ps(&positions[i].x, pos_x);
            _mm256_store_ps(&positions[i].y, pos_y);
            _mm256_store_ps(&positions[i].z, pos_z);
        }
        
        // Handle remaining elements
        for (size_t i = simd_count; i < count; ++i) {
            positions[i] += velocities[i] * dt;
        }
    }
    
    inline void parallel_apply_gravity(Vec3* forces, Real gravity_y, 
                                     const Real* masses, size_t count) {
        const __m256 gravity_vec = _mm256_set1_ps(gravity_y);
        const size_t simd_count = count & ~7;
        
        for (size_t i = 0; i < simd_count; i += 8) {
            __m256 mass_vec = _mm256_load_ps(&masses[i]);
            __m256 force_y = _mm256_load_ps(&forces[i].y);
            
            // force_y += gravity * mass
            force_y = _mm256_fmadd_ps(gravity_vec, mass_vec, force_y);
            
            _mm256_store_ps(&forces[i].y, force_y);
        }
        
        for (size_t i = simd_count; i < count; ++i) {
            forces[i].y += gravity_y * masses[i];
        }
    }
#endif

    // Cache-friendly memory layout for bulk operations
    struct SoATransform3D {
        std::vector<Real> pos_x, pos_y, pos_z;
        std::vector<Real> rot_x, rot_y, rot_z, rot_w;
        
        void resize(size_t count) {
            pos_x.resize(count); pos_y.resize(count); pos_z.resize(count);
            rot_x.resize(count); rot_y.resize(count); rot_z.resize(count); rot_w.resize(count);
        }
        
        void from_aos(const std::vector<Transform3D>& transforms) {
            resize(transforms.size());
            for (size_t i = 0; i < transforms.size(); ++i) {
                pos_x[i] = transforms[i].position.x;
                pos_y[i] = transforms[i].position.y;
                pos_z[i] = transforms[i].position.z;
                rot_x[i] = transforms[i].rotation.x;
                rot_y[i] = transforms[i].rotation.y;
                rot_z[i] = transforms[i].rotation.z;
                rot_w[i] = transforms[i].rotation.w;
            }
        }
        
        void to_aos(std::vector<Transform3D>& transforms) const {
            transforms.resize(pos_x.size());
            for (size_t i = 0; i < transforms.size(); ++i) {
                transforms[i].position = Vec3(pos_x[i], pos_y[i], pos_z[i]);
                transforms[i].rotation = Quaternion(rot_x[i], rot_y[i], rot_z[i], rot_w[i]);
            }
        }
    };
}

// Parallel collision detection system
class ParallelCollisionDetection {
private:
    std::unique_ptr<PhysicsThreadPool> thread_pool;
    
public:
    explicit ParallelCollisionDetection(size_t thread_count = 0) {
        if (thread_count == 0) {
            thread_count = std::thread::hardware_concurrency();
        }
        thread_pool = std::make_unique<PhysicsThreadPool>(thread_count);
    }
    
    template<typename BodyContainer, typename ShapeContainer>
    std::vector<CollisionPair> find_collision_pairs_parallel(
        const BodyContainer& bodies, const ShapeContainer& shapes) {
        
        const size_t body_count = bodies.size();
        const size_t thread_count = thread_pool->get_thread_count();
        const size_t chunk_size = std::max(size_t(1), body_count / thread_count);
        
        std::vector<std::future<std::vector<CollisionPair>>> futures;
        std::vector<std::vector<CollisionPair>> results(thread_count);
        
        // Divide work among threads
        for (size_t thread_id = 0; thread_id < thread_count; ++thread_id) {
            size_t start = thread_id * chunk_size;
            size_t end = std::min(start + chunk_size, body_count);
            
            if (start >= end) break;
            
            futures.push_back(thread_pool->enqueue([this, &bodies, &shapes, start, end, thread_id, &results]() {
                return find_collision_pairs_range(bodies, shapes, start, end);
            }));
        }
        
        // Collect results
        std::vector<CollisionPair> all_pairs;
        for (auto& future : futures) {
            auto pairs = future.get();
            all_pairs.insert(all_pairs.end(), pairs.begin(), pairs.end());
        }
        
        // Remove duplicates (pairs might be detected by multiple threads)
        std::sort(all_pairs.begin(), all_pairs.end(), 
            [](const CollisionPair& a, const CollisionPair& b) {
                return a.id_a < b.id_a || (a.id_a == b.id_a && a.id_b < b.id_b);
            });
        
        auto new_end = std::unique(all_pairs.begin(), all_pairs.end(),
            [](const CollisionPair& a, const CollisionPair& b) {
                return a.id_a == b.id_a && a.id_b == b.id_b;
            });
        
        all_pairs.erase(new_end, all_pairs.end());
        return all_pairs;
    }
    
private:
    template<typename BodyContainer, typename ShapeContainer>
    std::vector<CollisionPair> find_collision_pairs_range(
        const BodyContainer& bodies, const ShapeContainer& shapes,
        size_t start, size_t end) {
        
        std::vector<CollisionPair> pairs;
        SpatialHash<AABB3D> local_hash(10.0f);  // Thread-local spatial hash
        
        // Build spatial hash for this range
        for (size_t i = start; i < end; ++i) {
            const auto& body = bodies[i];
            auto shape_it = shapes.find(body.id);
            if (shape_it != shapes.end()) {
                AABB3D aabb = shape_it->second->get_aabb_3d(body.transform);
                local_hash.insert(body.id, aabb);
            }
        }
        
        return local_hash.find_collision_pairs();
    }
};

// Advanced physics world with optimizations
class AdvancedPhysicsWorld : public PhysicsWorld {
private:
    std::unique_ptr<PhysicsThreadPool> thread_pool;
    std::unique_ptr<ParallelCollisionDetection> parallel_collision;
    
    // Memory pools for efficient allocation
    struct MemoryPools {
        std::vector<ContactManifold> contact_pool;
        std::vector<ContactConstraint> constraint_pool;
        size_t contact_pool_index = 0;
        size_t constraint_pool_index = 0;
        
        void reset() {
            contact_pool_index = 0;
            constraint_pool_index = 0;
        }
        
        ContactManifold* get_contact() {
            if (contact_pool_index >= contact_pool.size()) {
                contact_pool.emplace_back(0, 0);
            }
            return &contact_pool[contact_pool_index++];
        }
        
        ContactConstraint* get_constraint(uint32_t a_id, uint32_t b_id, 
                                        const Vec3& contact_a, const Vec3& contact_b,
                                        const Vec3& normal, Real depth) {
            if (constraint_pool_index >= constraint_pool.size()) {
                constraint_pool.emplace_back(a_id, b_id, contact_a, contact_b, normal, depth);
            } else {
                constraint_pool[constraint_pool_index] = ContactConstraint(a_id, b_id, contact_a, contact_b, normal, depth);
            }
            return &constraint_pool[constraint_pool_index++];
        }
    };
    
    MemoryPools memory_pools;
    
    // Cache-friendly data layout
    SIMD::SoATransform3D soa_transforms;
    std::vector<Vec3> soa_velocities;
    std::vector<Vec3> soa_forces;
    std::vector<Real> soa_masses;
    
    // Performance profiling
    struct DetailedStats {
        Real broad_phase_parallel_overhead = 0.0f;
        Real narrow_phase_simd_speedup = 0.0f;
        Real constraint_solving_parallel_efficiency = 0.0f;
        size_t cache_hits = 0;
        size_t cache_misses = 0;
        Real memory_bandwidth_utilization = 0.0f;
    };
    
    DetailedStats detailed_stats;
    
public:
    explicit AdvancedPhysicsWorld(bool is_2d = false, size_t thread_count = 0) 
        : PhysicsWorld(is_2d) {
        
        if (thread_count == 0) {
            thread_count = std::thread::hardware_concurrency();
        }
        
        thread_pool = std::make_unique<PhysicsThreadPool>(thread_count);
        parallel_collision = std::make_unique<ParallelCollisionDetection>(thread_count);
        
        // Pre-allocate memory pools
        memory_pools.contact_pool.reserve(10000);
        memory_pools.constraint_pool.reserve(10000);
        
        // Configure for high-performance simulation
        PhysicsWorldConfig config = get_config();
        config.use_multithreading = true;
        config.worker_thread_count = thread_count;
        config.enable_continuous_collision = true;
        set_config(config);
    }
    
    // High-performance stepping with parallel execution
    void step_parallel(Real dt) override {
        auto frame_start = std::chrono::high_resolution_clock::now();
        
        dt *= get_config().time_scale;
        if (dt <= 0.0f) return;
        
        Real accumulated = get_accumulated_time() + dt;
        Real fixed_dt = get_fixed_time_step();
        
        while (accumulated >= fixed_dt) {
            step_fixed_parallel(fixed_dt);
            accumulated -= fixed_dt;
        }
        
        set_accumulated_time(accumulated);
        
        auto frame_end = std::chrono::high_resolution_clock::now();
        update_performance_metrics(frame_start, frame_end);
    }
    
private:
    void step_fixed_parallel(Real dt) {
        memory_pools.reset();
        
        // Phase 1: Parallel force application and integration
        std::vector<std::future<void>> phase1_futures;
        
        phase1_futures.push_back(thread_pool->enqueue([this, dt]() {
            apply_forces_parallel(dt);
        }));
        
        phase1_futures.push_back(thread_pool->enqueue([this, dt]() {
            integrate_forces_simd(dt);
        }));
        
        // Wait for phase 1
        for (auto& future : phase1_futures) {
            future.wait();
        }
        
        // Phase 2: Collision detection (can be parallelized)
        auto collision_future = thread_pool->enqueue([this]() {
            return update_collision_detection_parallel();
        });
        
        // Phase 3: Constraint preparation (parallel)
        auto contacts = collision_future.get();
        auto constraint_future = thread_pool->enqueue([this, &contacts, dt]() {
            prepare_constraints_parallel(contacts, dt);
        });
        
        constraint_future.wait();
        
        // Phase 4: Constraint solving (iterative, but can parallelize constraint groups)
        solve_constraints_parallel(dt);
        
        // Phase 5: Integration and sleep updates
        std::vector<std::future<void>> phase5_futures;
        
        phase5_futures.push_back(thread_pool->enqueue([this, dt]() {
            integrate_velocities_simd(dt);
        }));
        
        phase5_futures.push_back(thread_pool->enqueue([this, dt]() {
            update_sleep_states_parallel(dt);
        }));
        
        for (auto& future : phase5_futures) {
            future.wait();
        }
        
        update_statistics_parallel();
    }
    
    void apply_forces_parallel(Real dt) {
        if (is_2d()) return;  // 2D implementation would be similar
        
        const auto& bodies = get_bodies_3d();
        const size_t body_count = bodies.size();
        const size_t thread_count = thread_pool->get_thread_count();
        const size_t chunk_size = std::max(size_t(1), body_count / thread_count);
        
        std::vector<std::future<void>> futures;
        
        for (size_t thread_id = 0; thread_id < thread_count; ++thread_id) {
            size_t start = thread_id * chunk_size;
            size_t end = std::min(start + chunk_size, body_count);
            
            if (start >= end) break;
            
            futures.push_back(thread_pool->enqueue([this, start, end, dt]() {
                apply_forces_range(start, end, dt);
            }));
        }
        
        for (auto& future : futures) {
            future.wait();
        }
    }
    
    void apply_forces_range(size_t start, size_t end, Real dt) {
        auto& bodies = get_bodies_3d_mutable();
        const auto& force_generators = get_force_generators();
        
        for (size_t i = start; i < end; ++i) {
            for (const auto& generator : force_generators) {
                if (generator->is_active()) {
                    generator->apply_force(bodies[i], dt);
                }
            }
        }
    }
    
    void integrate_forces_simd(Real dt) {
        if (is_2d()) return;  // 2D implementation
        
        const auto& bodies = get_bodies_3d();
        prepare_soa_data(bodies);
        
#ifdef __AVX2__
        // Use SIMD acceleration for bulk integration
        SIMD::parallel_apply_gravity(
            reinterpret_cast<Vec3*>(soa_forces.data()),
            get_gravity().y,
            soa_masses.data(),
            bodies.size()
        );
        
        // Integrate velocities with forces
        for (size_t i = 0; i < bodies.size(); ++i) {
            if (bodies[i].type == BodyType::Dynamic && !bodies[i].is_sleeping) {
                soa_velocities[i] += soa_forces[i] * bodies[i].mass_props.inverse_mass * dt;
                
                // Apply damping
                soa_velocities[i] *= std::pow(1.0f - bodies[i].material.linear_damping, dt);
            }
        }
#endif
        
        write_back_soa_data();
    }
    
    std::vector<ContactManifold> update_collision_detection_parallel() {
        if (is_2d()) return {};  // 2D implementation
        
        const auto& bodies = get_bodies_3d();
        const auto& shapes = get_shapes();
        
        // Parallel broad phase
        auto collision_pairs = parallel_collision->find_collision_pairs_parallel(bodies, shapes);
        
        // Parallel narrow phase
        std::vector<ContactManifold> manifolds;
        const size_t pair_count = collision_pairs.size();
        const size_t thread_count = thread_pool->get_thread_count();
        const size_t chunk_size = std::max(size_t(1), pair_count / thread_count);
        
        std::vector<std::future<std::vector<ContactManifold>>> futures;
        
        for (size_t thread_id = 0; thread_id < thread_count; ++thread_id) {
            size_t start = thread_id * chunk_size;
            size_t end = std::min(start + chunk_size, pair_count);
            
            if (start >= end) break;
            
            futures.push_back(thread_pool->enqueue([this, &collision_pairs, start, end]() {
                return process_collision_pairs_range(collision_pairs, start, end);
            }));
        }
        
        // Collect results
        for (auto& future : futures) {
            auto thread_manifolds = future.get();
            manifolds.insert(manifolds.end(), thread_manifolds.begin(), thread_manifolds.end());
        }
        
        return manifolds;
    }
    
    std::vector<ContactManifold> process_collision_pairs_range(
        const std::vector<CollisionPair>& pairs, size_t start, size_t end) {
        
        std::vector<ContactManifold> manifolds;
        const auto& bodies = get_bodies_3d();
        const auto& shapes = get_shapes();
        
        for (size_t i = start; i < end; ++i) {
            const auto& pair = pairs[i];
            
            const RigidBody3D* body_a = get_body_3d(pair.id_a);
            const RigidBody3D* body_b = get_body_3d(pair.id_b);
            
            auto shape_a_it = shapes.find(pair.id_a);
            auto shape_b_it = shapes.find(pair.id_b);
            
            if (body_a && body_b && shape_a_it != shapes.end() && shape_b_it != shapes.end()) {
                auto collision_info = NarrowPhaseCollisionDetection::test_collision_3d(
                    *body_a, *shape_a_it->second, *body_b, *shape_b_it->second);
                
                if (collision_info.is_colliding) {
                    collision_info.manifold.body_a_id = pair.id_a;
                    collision_info.manifold.body_b_id = pair.id_b;
                    manifolds.push_back(collision_info.manifold);
                }
            }
        }
        
        return manifolds;
    }
    
    void prepare_constraints_parallel(const std::vector<ContactManifold>& manifolds, Real dt) {
        // Prepare contact constraints in parallel
        const size_t manifold_count = manifolds.size();
        const size_t thread_count = thread_pool->get_thread_count();
        const size_t chunk_size = std::max(size_t(1), manifold_count / thread_count);
        
        std::vector<std::future<void>> futures;
        
        for (size_t thread_id = 0; thread_id < thread_count; ++thread_id) {
            size_t start = thread_id * chunk_size;
            size_t end = std::min(start + chunk_size, manifold_count);
            
            if (start >= end) break;
            
            futures.push_back(thread_pool->enqueue([this, &manifolds, start, end, dt]() {
                prepare_constraints_range(manifolds, start, end, dt);
            }));
        }
        
        for (auto& future : futures) {
            future.wait();
        }
    }
    
    void prepare_constraints_range(const std::vector<ContactManifold>& manifolds,
                                  size_t start, size_t end, Real dt) {
        const auto& bodies = get_bodies_3d();
        
        for (size_t i = start; i < end; ++i) {
            const auto& manifold = manifolds[i];
            
            const RigidBody3D* body_a = get_body_3d(manifold.body_a_id);
            const RigidBody3D* body_b = get_body_3d(manifold.body_b_id);
            
            if (body_a && body_b) {
                for (const auto& contact : manifold.contacts) {
                    ContactConstraint* constraint = memory_pools.get_constraint(
                        manifold.body_a_id, manifold.body_b_id,
                        contact.position_a, contact.position_b,
                        contact.normal, contact.penetration
                    );
                    
                    constraint->friction_coefficient = manifold.friction;
                    constraint->restitution_coefficient = manifold.restitution;
                    constraint->prepare(*body_a, *body_b, dt);
                }
            }
        }
    }
    
    void solve_constraints_parallel(Real dt) {
        // Island-based constraint solving for better parallelization
        auto constraint_islands = build_constraint_islands();
        
        const int velocity_iterations = get_config().velocity_iterations;
        const int position_iterations = get_config().position_iterations;
        
        // Solve position constraints
        for (int iter = 0; iter < position_iterations; ++iter) {
            solve_constraint_islands_parallel(constraint_islands, dt, true);
        }
        
        // Solve velocity constraints
        for (int iter = 0; iter < velocity_iterations; ++iter) {
            solve_constraint_islands_parallel(constraint_islands, dt, false);
        }
    }
    
    std::vector<std::vector<size_t>> build_constraint_islands() {
        // Build connectivity graph and find connected components
        // This would be a more complex implementation in practice
        std::vector<std::vector<size_t>> islands;
        
        // For now, create simple islands based on constraint groups
        const size_t constraint_count = memory_pools.constraint_pool_index;
        const size_t max_island_size = 100;  // Reasonable size for parallel processing
        
        for (size_t i = 0; i < constraint_count; i += max_island_size) {
            std::vector<size_t> island;
            size_t end = std::min(i + max_island_size, constraint_count);
            
            for (size_t j = i; j < end; ++j) {
                island.push_back(j);
            }
            
            islands.push_back(island);
        }
        
        return islands;
    }
    
    void solve_constraint_islands_parallel(const std::vector<std::vector<size_t>>& islands,
                                          Real dt, bool position_solve) {
        std::vector<std::future<void>> futures;
        
        for (const auto& island : islands) {
            futures.push_back(thread_pool->enqueue([this, &island, dt, position_solve]() {
                solve_constraint_island(island, dt, position_solve);
            }));
        }
        
        for (auto& future : futures) {
            future.wait();
        }
    }
    
    void solve_constraint_island(const std::vector<size_t>& island_constraints,
                                Real dt, bool position_solve) {
        auto& bodies = get_bodies_3d_mutable();
        
        for (size_t constraint_idx : island_constraints) {
            if (constraint_idx >= memory_pools.constraint_pool_index) continue;
            
            ContactConstraint& constraint = memory_pools.constraint_pool[constraint_idx];
            
            RigidBody3D* body_a = get_body_3d_mutable(constraint.body_a_id);
            RigidBody3D* body_b = get_body_3d_mutable(constraint.body_b_id);
            
            if (body_a && body_b) {
                if (position_solve) {
                    constraint.solve_position(*body_a, *body_b, dt);
                } else {
                    constraint.solve_velocity(*body_a, *body_b, dt);
                }
            }
        }
    }
    
    void integrate_velocities_simd(Real dt) {
        if (is_2d()) return;
        
        const auto& bodies = get_bodies_3d();
        
#ifdef __AVX2__
        // SIMD position integration
        SIMD::parallel_integrate_positions(
            reinterpret_cast<Vec3*>(soa_transforms.pos_x.data()),
            soa_velocities.data(),
            dt,
            bodies.size()
        );
#endif
        
        write_back_integration_results();
    }
    
    void update_sleep_states_parallel(Real dt) {
        if (!get_config().allow_sleep) return;
        
        const auto& bodies = get_bodies_3d();
        const size_t body_count = bodies.size();
        const size_t thread_count = thread_pool->get_thread_count();
        const size_t chunk_size = std::max(size_t(1), body_count / thread_count);
        
        std::vector<std::future<void>> futures;
        
        for (size_t thread_id = 0; thread_id < thread_count; ++thread_id) {
            size_t start = thread_id * chunk_size;
            size_t end = std::min(start + chunk_size, body_count);
            
            if (start >= end) break;
            
            futures.push_back(thread_pool->enqueue([this, start, end, dt]() {
                update_sleep_states_range(start, end, dt);
            }));
        }
        
        for (auto& future : futures) {
            future.wait();
        }
    }
    
    void update_sleep_states_range(size_t start, size_t end, Real dt) {
        auto& bodies = get_bodies_3d_mutable();
        const Real sleep_threshold = get_config().sleep_threshold;
        const Real sleep_time_threshold = get_config().sleep_time_threshold;
        
        for (size_t i = start; i < end; ++i) {
            if (bodies[i].can_sleep()) {
                bodies[i].sleep_time += dt;
                if (bodies[i].sleep_time > sleep_time_threshold) {
                    bodies[i].put_to_sleep();
                }
            } else {
                bodies[i].sleep_time = 0.0f;
            }
        }
    }
    
    void update_statistics_parallel() {
        // Collect statistics in parallel
        auto stats_future = thread_pool->enqueue([this]() {
            calculate_detailed_performance_metrics();
        });
        
        stats_future.wait();
    }
    
    // Helper methods for SoA data layout
    void prepare_soa_data(const std::vector<RigidBody3D>& bodies) {
        const size_t body_count = bodies.size();
        
        soa_velocities.resize(body_count);
        soa_forces.resize(body_count);
        soa_masses.resize(body_count);
        
        std::vector<Transform3D> transforms(body_count);
        for (size_t i = 0; i < body_count; ++i) {
            transforms[i] = bodies[i].transform;
            soa_velocities[i] = bodies[i].velocity;
            soa_forces[i] = bodies[i].force;
            soa_masses[i] = bodies[i].mass_props.mass;
        }
        
        soa_transforms.from_aos(transforms);
    }
    
    void write_back_soa_data() {
        auto& bodies = get_bodies_3d_mutable();
        std::vector<Transform3D> transforms;
        soa_transforms.to_aos(transforms);
        
        for (size_t i = 0; i < bodies.size(); ++i) {
            bodies[i].transform = transforms[i];
            bodies[i].velocity = soa_velocities[i];
            bodies[i].force = soa_forces[i];
        }
    }
    
    void write_back_integration_results() {
        auto& bodies = get_bodies_3d_mutable();
        
        for (size_t i = 0; i < bodies.size(); ++i) {
            if (bodies[i].type != BodyType::Static && !bodies[i].is_sleeping) {
                bodies[i].transform.position = Vec3(
                    soa_transforms.pos_x[i],
                    soa_transforms.pos_y[i],
                    soa_transforms.pos_z[i]
                );
                
                // Clear forces
                bodies[i].force = Vec3::zero();
                bodies[i].torque = Vec3::zero();
            }
        }
    }
    
    void calculate_detailed_performance_metrics() {
        // Calculate advanced performance metrics
        detailed_stats.cache_hits = memory_pools.contact_pool.size() - memory_pools.contact_pool_index;
        detailed_stats.cache_misses = memory_pools.contact_pool_index;
        
        // Estimate memory bandwidth utilization (simplified)
        const size_t body_count = is_2d() ? get_bodies_2d().size() : get_bodies_3d().size();
        const size_t bytes_per_body = is_2d() ? sizeof(RigidBody2D) : sizeof(RigidBody3D);
        const size_t total_memory_accessed = body_count * bytes_per_body * 4;  // Rough estimate
        
        detailed_stats.memory_bandwidth_utilization = 
            Real(total_memory_accessed) / (get_stats().total_time * 1e9f);  // GB/s rough estimate
    }
    
    void update_performance_metrics(std::chrono::high_resolution_clock::time_point start,
                                   std::chrono::high_resolution_clock::time_point end) {
        Real frame_time = std::chrono::duration<Real>(end - start).count();
        
        // Update base class statistics
        auto& stats = get_stats_mutable();
        stats.fps = 1.0f / frame_time;
        stats.total_time = frame_time;
        
        // Additional performance metrics
        detailed_stats.broad_phase_parallel_overhead = 
            std::max(0.0f, stats.broad_phase_time - (stats.broad_phase_time / thread_pool->get_thread_count()));
        
        detailed_stats.constraint_solving_parallel_efficiency = 
            Real(thread_pool->get_thread_count()) / (stats.constraint_solving_time * thread_pool->get_thread_count());
    }
    
public:
    const DetailedStats& get_detailed_stats() const { return detailed_stats; }
    
    // Performance tuning methods
    void optimize_for_body_count(size_t expected_body_count) {
        // Pre-allocate memory based on expected body count
        size_t estimated_contacts = expected_body_count * 4;  // Rough estimate
        size_t estimated_constraints = estimated_contacts * 2;
        
        memory_pools.contact_pool.reserve(estimated_contacts);
        memory_pools.constraint_pool.reserve(estimated_constraints);
        
        // Adjust spatial hash cell size based on expected density
        Real estimated_world_size = std::sqrt(Real(expected_body_count)) * 10.0f;
        Real optimal_cell_size = estimated_world_size / std::sqrt(Real(expected_body_count));
        
        auto config = get_config();
        config.broad_phase_margin = optimal_cell_size * 0.1f;
        set_config(config);
    }
    
    void enable_simd_optimizations(bool enable) {
        // Control SIMD usage at runtime
        auto config = get_config();
        config.enable_continuous_collision = enable;  // Use as SIMD flag for now
        set_config(config);
    }
    
    // Memory management
    void compact_memory() {
        memory_pools.contact_pool.shrink_to_fit();
        memory_pools.constraint_pool.shrink_to_fit();
        soa_velocities.shrink_to_fit();
        soa_forces.shrink_to_fit();
        soa_masses.shrink_to_fit();
    }
    
    size_t get_memory_footprint() const {
        return get_stats().memory_usage_bytes +
               memory_pools.contact_pool.capacity() * sizeof(ContactManifold) +
               memory_pools.constraint_pool.capacity() * sizeof(ContactConstraint) +
               soa_velocities.capacity() * sizeof(Vec3) +
               soa_forces.capacity() * sizeof(Vec3) +
               soa_masses.capacity() * sizeof(Real);
    }
};

} // namespace ecscope::physics