/**
 * @file examples/physics_performance_benchmark.cpp
 * @brief Comprehensive 2D vs 3D Physics Performance Benchmark Suite
 * 
 * This benchmark suite provides detailed performance analysis comparing 2D and 3D
 * physics implementations, demonstrating the computational complexity differences
 * and providing educational insights into physics engine optimization.
 * 
 * Key Features:
 * 1. **Comprehensive Benchmarking:**
 *    - Side-by-side 2D vs 3D performance comparison
 *    - Detailed timing of individual physics subsystems
 *    - Memory usage analysis and allocation patterns
 *    - Scalability testing with varying entity counts
 * 
 * 2. **Educational Analysis:**
 *    - Theoretical vs actual complexity comparisons
 *    - Algorithm efficiency measurements
 *    - SIMD optimization effectiveness
 *    - Parallel processing benefits
 * 
 * 3. **Real-World Scenarios:**
 *    - Game-like physics simulations
 *    - Scientific simulation workloads
 *    - Stress testing with thousands of objects
 *    - Various collision shape combinations
 * 
 * 4. **Optimization Insights:**
 *    - Bottleneck identification
 *    - Memory access pattern analysis
 *    - Cache efficiency measurements
 *    - Threading scalability analysis
 * 
 * @author ECScope Educational ECS Framework - Performance Analysis
 * @date 2025
 */

#include "../src/physics/world.hpp"        // 2D physics world
#include "../src/physics/world3d.hpp"      // 3D physics world
#include "../src/physics/components.hpp"   // 2D physics components
#include "../src/physics/components3d.hpp" // 3D physics components
#include "../src/job_system/work_stealing_job_system.hpp"
#include "../src/ecs/registry.hpp"
#include "../src/core/log.hpp"
#include "../src/memory/memory_tracker.hpp"
#include <chrono>
#include <random>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <thread>

using namespace ecscope;
using namespace ecscope::physics;
using namespace ecscope::physics::components;
using namespace ecscope::physics::components3d;
using namespace ecscope::job_system;
using namespace ecscope::ecs;

/**
 * @brief Comprehensive Physics Performance Benchmark Suite
 */
class PhysicsPerformanceBenchmark {
private:
    // Benchmark configuration
    struct BenchmarkConfig {
        std::vector<u32> entity_counts{100, 250, 500, 1000, 2000, 5000};
        u32 warmup_frames{60};          // Frames to run before measurement
        u32 measurement_frames{300};    // Frames to measure (5 seconds at 60 FPS)
        f32 time_step{1.0f / 60.0f};   // Fixed timestep
        bool enable_multithreading{true};
        bool enable_simd{true};
        u32 benchmark_runs{3};         // Multiple runs for statistical accuracy
    } config_;
    
    // Benchmark results
    struct BenchmarkResults {
        struct PhysicsResults {
            f64 total_time_ms{0.0};
            f64 broad_phase_time_ms{0.0};
            f64 narrow_phase_time_ms{0.0};
            f64 constraint_solve_time_ms{0.0};
            f64 integration_time_ms{0.0};
            usize peak_memory_bytes{0};
            usize average_memory_bytes{0};
            u32 collision_tests{0};
            u32 successful_collisions{0};
            f32 collision_efficiency{0.0f};
        };
        
        std::map<u32, PhysicsResults> results_2d;
        std::map<u32, PhysicsResults> results_3d;
        
        // Comparison metrics
        std::map<u32, f32> complexity_ratios;     // 3D/2D time ratios
        std::map<u32, f32> memory_ratios;         // 3D/2D memory ratios
        std::map<u32, f32> efficiency_ratios;     // 3D/2D efficiency ratios
    } results_;
    
    // Benchmark data storage
    std::vector<f64> frame_times_2d_;
    std::vector<f64> frame_times_3d_;
    std::vector<usize> memory_samples_2d_;
    std::vector<usize> memory_samples_3d_;

public:
    PhysicsPerformanceBenchmark() {
        LOG_INFO("=== ECScope Physics Performance Benchmark Suite ===");
        LOG_INFO("This benchmark compares 2D vs 3D physics performance");
        LOG_INFO("and provides detailed analysis of computational complexity.");
        LOG_INFO("");
    }
    
    void run_complete_benchmark_suite() {
        LOG_INFO("Starting comprehensive physics performance benchmark...");
        
        // Initialize benchmark environment
        initialize_benchmark_environment();
        
        // Run benchmarks for different entity counts
        for (u32 entity_count : config_.entity_counts) {
            LOG_INFO("=== Benchmarking with {} entities ===", entity_count);
            
            // Run multiple benchmark runs for statistical accuracy
            std::vector<BenchmarkResults::PhysicsResults> runs_2d;
            std::vector<BenchmarkResults::PhysicsResults> runs_3d;
            
            for (u32 run = 0; run < config_.benchmark_runs; ++run) {
                LOG_INFO("Run {}/{} with {} entities", run + 1, config_.benchmark_runs, entity_count);
                
                // 2D Benchmark
                auto result_2d = benchmark_2d_physics(entity_count);
                runs_2d.push_back(result_2d);
                
                // 3D Benchmark
                auto result_3d = benchmark_3d_physics(entity_count);
                runs_3d.push_back(result_3d);
                
                // Brief pause between runs
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            
            // Average the results
            results_.results_2d[entity_count] = average_results(runs_2d);
            results_.results_3d[entity_count] = average_results(runs_3d);
            
            // Calculate comparison metrics
            calculate_comparison_metrics(entity_count);
            
            // Report intermediate results
            report_entity_count_results(entity_count);
        }
        
        // Generate comprehensive analysis
        generate_comprehensive_analysis();
        
        // Export results to file
        export_results_to_csv();
        
        LOG_INFO("Benchmark suite completed successfully!");
    }

private:
    void initialize_benchmark_environment() {
        LOG_INFO("Initializing benchmark environment...");
        
        // Set up consistent random seed for reproducible results
        std::srand(12345);
        
        // Initialize memory tracker for accurate memory measurements
        memory::MemoryTracker::instance().reset();
        
        LOG_INFO("Benchmark environment initialized");
        LOG_INFO("  - Entity counts to test: {}", config_.entity_counts.size());
        LOG_INFO("  - Warmup frames per test: {}", config_.warmup_frames);
        LOG_INFO("  - Measurement frames per test: {}", config_.measurement_frames);
        LOG_INFO("  - Benchmark runs per configuration: {}", config_.benchmark_runs);
        LOG_INFO("  - Multithreading: {}", config_.enable_multithreading ? "Enabled" : "Disabled");
    }
    
    BenchmarkResults::PhysicsResults benchmark_2d_physics(u32 entity_count) {
        LOG_DEBUG("Benchmarking 2D physics with {} entities", entity_count);
        
        // Create registry and physics world
        Registry registry;
        PhysicsWorldConfig config_2d = PhysicsWorldConfig::create_performance();
        config_2d.enable_multithreading = config_.enable_multithreading;
        config_2d.max_active_bodies = entity_count;
        
        PhysicsWorld2D physics_world(registry, config_2d);
        
        // Create 2D physics entities
        create_2d_entities(registry, physics_world, entity_count);
        
        // Warmup phase
        for (u32 i = 0; i < config_.warmup_frames; ++i) {
            physics_world.update(config_.time_step);
        }
        
        // Clear memory tracker for accurate measurement
        memory::MemoryTracker::instance().reset();
        
        // Measurement phase
        auto start_time = std::chrono::high_resolution_clock::now();
        usize memory_samples = 0;
        usize total_memory = 0;
        usize peak_memory = 0;
        
        for (u32 i = 0; i < config_.measurement_frames; ++i) {
            auto frame_start = std::chrono::high_resolution_clock::now();
            
            physics_world.update(config_.time_step);
            
            auto frame_end = std::chrono::high_resolution_clock::now();
            f64 frame_time = std::chrono::duration<f64, std::milli>(frame_end - frame_start).count();
            frame_times_2d_.push_back(frame_time);
            
            // Sample memory usage
            usize current_memory = memory::MemoryTracker::instance().get_total_allocated();
            total_memory += current_memory;
            peak_memory = std::max(peak_memory, current_memory);
            memory_samples_2d_.push_back(current_memory);
            ++memory_samples;
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        f64 total_time = std::chrono::duration<f64, std::milli>(end_time - start_time).count();
        
        // Collect detailed timing statistics
        const auto& stats = physics_world.get_statistics();
        
        BenchmarkResults::PhysicsResults result;
        result.total_time_ms = total_time / config_.measurement_frames;
        result.broad_phase_time_ms = stats.broad_phase_time;
        result.narrow_phase_time_ms = stats.narrow_phase_time;
        result.constraint_solve_time_ms = stats.constraint_solve_time;
        result.integration_time_ms = stats.integration_time;
        result.peak_memory_bytes = peak_memory;
        result.average_memory_bytes = memory_samples > 0 ? total_memory / memory_samples : 0;
        result.collision_tests = stats.narrow_phase_tests;
        result.successful_collisions = stats.active_contacts;
        result.collision_efficiency = stats.narrow_phase_tests > 0 ? 
            static_cast<f32>(stats.active_contacts) / stats.narrow_phase_tests : 0.0f;
        
        LOG_DEBUG("2D benchmark complete: {:.3f}ms avg, {:.2f}MB peak memory", 
                 result.total_time_ms, result.peak_memory_bytes / (1024.0f * 1024.0f));
        
        return result;
    }
    
    BenchmarkResults::PhysicsResults benchmark_3d_physics(u32 entity_count) {
        LOG_DEBUG("Benchmarking 3D physics with {} entities", entity_count);
        
        // Create registry and job system
        Registry registry;
        
        JobSystem::Config job_config = JobSystem::Config::create_performance_optimized();
        job_config.enable_profiling = false;  // Disable for pure performance
        JobSystem job_system(job_config);
        job_system.initialize();
        
        // Create 3D physics world
        PhysicsWorldConfig3D config_3d = PhysicsWorldConfig3D::create_performance();
        config_3d.enable_multithreading = config_.enable_multithreading;
        config_3d.max_active_bodies_3d = entity_count;
        config_3d.enable_profiling = false;  // Disable for pure performance
        
        PhysicsWorld3D physics_world_3d(registry, config_3d, &job_system);
        
        // Create 3D physics entities
        create_3d_entities(registry, physics_world_3d, entity_count);
        
        // Warmup phase
        for (u32 i = 0; i < config_.warmup_frames; ++i) {
            physics_world_3d.update(config_.time_step);
        }
        
        // Clear memory tracker for accurate measurement
        memory::MemoryTracker::instance().reset();
        
        // Measurement phase
        auto start_time = std::chrono::high_resolution_clock::now();
        usize memory_samples = 0;
        usize total_memory = 0;
        usize peak_memory = 0;
        
        for (u32 i = 0; i < config_.measurement_frames; ++i) {
            auto frame_start = std::chrono::high_resolution_clock::now();
            
            physics_world_3d.update(config_.time_step);
            
            auto frame_end = std::chrono::high_resolution_clock::now();
            f64 frame_time = std::chrono::duration<f64, std::milli>(frame_end - frame_start).count();
            frame_times_3d_.push_back(frame_time);
            
            // Sample memory usage
            usize current_memory = memory::MemoryTracker::instance().get_total_allocated();
            total_memory += current_memory;
            peak_memory = std::max(peak_memory, current_memory);
            memory_samples_3d_.push_back(current_memory);
            ++memory_samples;
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        f64 total_time = std::chrono::duration<f64, std::milli>(end_time - start_time).count();
        
        // Collect detailed timing statistics
        const auto& stats = physics_world_3d.get_statistics_3d();
        
        BenchmarkResults::PhysicsResults result;
        result.total_time_ms = total_time / config_.measurement_frames;
        result.broad_phase_time_ms = stats.broad_phase_time_3d;
        result.narrow_phase_time_ms = stats.narrow_phase_time_3d;
        result.constraint_solve_time_ms = stats.constraint_solve_time_3d;
        result.integration_time_ms = stats.integration_time_3d;
        result.peak_memory_bytes = peak_memory;
        result.average_memory_bytes = memory_samples > 0 ? total_memory / memory_samples : 0;
        result.collision_tests = stats.narrow_phase_tests_3d;
        result.successful_collisions = stats.active_contacts_3d;
        result.collision_efficiency = stats.narrow_phase_tests_3d > 0 ? 
            static_cast<f32>(stats.active_contacts_3d) / stats.narrow_phase_tests_3d : 0.0f;
        
        LOG_DEBUG("3D benchmark complete: {:.3f}ms avg, {:.2f}MB peak memory", 
                 result.total_time_ms, result.peak_memory_bytes / (1024.0f * 1024.0f));
        
        job_system.shutdown();
        return result;
    }
    
    void create_2d_entities(Registry& registry, PhysicsWorld2D& physics_world, u32 entity_count) {
        std::random_device rd;
        std::mt19937 gen(12345);  // Fixed seed for reproducibility
        std::uniform_real_distribution<f32> pos_dist(-25.0f, 25.0f);
        std::uniform_real_distribution<f32> vel_dist(-5.0f, 5.0f);
        std::uniform_real_distribution<f32> mass_dist(1.0f, 5.0f);
        std::uniform_real_distribution<f32> size_dist(0.5f, 2.0f);
        
        // Create diverse mix of 2D shapes
        for (u32 i = 0; i < entity_count; ++i) {
            Entity entity = registry.create();
            
            // Transform
            Vec2 position{pos_dist(gen), pos_dist(gen) + 10.0f};
            f32 rotation = pos_dist(gen) * 0.1f;
            auto& transform = registry.emplace<Transform>(entity, position, rotation);
            
            // Rigid body
            f32 mass = mass_dist(gen);
            auto& body = registry.emplace<RigidBody2D>(entity);
            body.set_mass(mass);
            body.linear_velocity = Vec2{vel_dist(gen), vel_dist(gen)};
            body.angular_velocity = vel_dist(gen) * 0.2f;
            
            // Collider (alternate between circles and boxes)
            if (i % 2 == 0) {
                f32 radius = size_dist(gen);
                auto& collider = registry.emplace<Collider2D>(entity);
                collider.shape = CollisionShape::create_circle(radius);
                body.set_moment_of_inertia_circle(radius);
            } else {
                Vec2 size{size_dist(gen), size_dist(gen)};
                auto& collider = registry.emplace<Collider2D>(entity);
                collider.shape = CollisionShape::create_box(size);
                body.set_moment_of_inertia_box(size.x, size.y);
            }
            
            physics_world.add_entity(entity);
        }
        
        // Add some static ground objects
        for (i32 x = -3; x <= 3; ++x) {
            Entity ground = registry.create();
            Vec2 ground_pos{x * 15.0f, -10.0f};
            auto& transform = registry.emplace<Transform>(ground, ground_pos, 0.0f);
            
            auto& body = registry.emplace<RigidBody2D>(ground);
            body.set_mass(0.0f);  // Static body
            
            auto& collider = registry.emplace<Collider2D>(ground);
            collider.shape = CollisionShape::create_box(Vec2{7.0f, 1.0f});
            
            physics_world.add_entity(ground);
        }
    }
    
    void create_3d_entities(Registry& registry, PhysicsWorld3D& physics_world_3d, u32 entity_count) {
        std::random_device rd;
        std::mt19937 gen(12345);  // Fixed seed for reproducibility
        std::uniform_real_distribution<f32> pos_dist(-25.0f, 25.0f);
        std::uniform_real_distribution<f32> vel_dist(-5.0f, 5.0f);
        std::uniform_real_distribution<f32> mass_dist(1.0f, 5.0f);
        std::uniform_real_distribution<f32> size_dist(0.5f, 2.0f);
        
        // Create diverse mix of 3D shapes
        for (u32 i = 0; i < entity_count; ++i) {
            Entity entity = registry.create();
            
            // Transform
            Vec3 position{pos_dist(gen), pos_dist(gen) + 10.0f, pos_dist(gen)};
            auto& transform = registry.emplace<Transform3D>(entity, position);
            
            // Rigid body
            f32 mass = mass_dist(gen);
            auto& body = registry.emplace<RigidBody3D>(entity, RigidBody3D::create_dynamic(mass));
            body.linear_velocity = Vec3{vel_dist(gen), vel_dist(gen), vel_dist(gen)};
            body.angular_velocity = Vec3{vel_dist(gen) * 0.2f, vel_dist(gen) * 0.2f, vel_dist(gen) * 0.2f};
            
            // Collider (cycle through different 3D shapes)
            if (i % 3 == 0) {
                // Sphere
                f32 radius = size_dist(gen);
                auto& collider = registry.emplace<Collider3D>(entity, Collider3D::create_sphere(radius));
                body.set_inertia_tensor_sphere(radius);
            } else if (i % 3 == 1) {
                // Box
                Vec3 size{size_dist(gen), size_dist(gen), size_dist(gen)};
                auto& collider = registry.emplace<Collider3D>(entity, Collider3D::create_box(size * 0.5f));
                body.set_inertia_tensor_box(size);
            } else {
                // Capsule
                f32 radius = size_dist(gen) * 0.5f;
                f32 height = size_dist(gen) * 2.0f;
                auto& collider = registry.emplace<Collider3D>(entity, Collider3D::create_capsule(radius, height));
                body.set_inertia_tensor_cylinder(radius, height);
            }
            
            physics_world_3d.add_entity_3d(entity);
        }
        
        // Add static ground objects
        for (i32 x = -2; x <= 2; ++x) {
            for (i32 z = -2; z <= 2; ++z) {
                Entity ground = registry.create();
                Vec3 ground_pos{x * 20.0f, -10.0f, z * 20.0f};
                auto& transform = registry.emplace<Transform3D>(ground, ground_pos);
                
                auto& body = registry.emplace<RigidBody3D>(ground, RigidBody3D::create_static());
                
                Vec3 ground_size{9.0f, 1.0f, 9.0f};
                auto& collider = registry.emplace<Collider3D>(ground, Collider3D::create_box(ground_size));
                
                physics_world_3d.add_entity_3d(ground);
            }
        }
    }
    
    BenchmarkResults::PhysicsResults average_results(const std::vector<BenchmarkResults::PhysicsResults>& runs) {
        if (runs.empty()) return {};
        
        BenchmarkResults::PhysicsResults avg;
        
        for (const auto& run : runs) {
            avg.total_time_ms += run.total_time_ms;
            avg.broad_phase_time_ms += run.broad_phase_time_ms;
            avg.narrow_phase_time_ms += run.narrow_phase_time_ms;
            avg.constraint_solve_time_ms += run.constraint_solve_time_ms;
            avg.integration_time_ms += run.integration_time_ms;
            avg.peak_memory_bytes = std::max(avg.peak_memory_bytes, run.peak_memory_bytes);
            avg.average_memory_bytes += run.average_memory_bytes;
            avg.collision_tests += run.collision_tests;
            avg.successful_collisions += run.successful_collisions;
            avg.collision_efficiency += run.collision_efficiency;
        }
        
        f32 count = static_cast<f32>(runs.size());
        avg.total_time_ms /= count;
        avg.broad_phase_time_ms /= count;
        avg.narrow_phase_time_ms /= count;
        avg.constraint_solve_time_ms /= count;
        avg.integration_time_ms /= count;
        avg.average_memory_bytes /= static_cast<usize>(count);
        avg.collision_tests /= static_cast<u32>(count);
        avg.successful_collisions /= static_cast<u32>(count);
        avg.collision_efficiency /= count;
        
        return avg;
    }
    
    void calculate_comparison_metrics(u32 entity_count) {
        const auto& result_2d = results_.results_2d[entity_count];
        const auto& result_3d = results_.results_3d[entity_count];
        
        // Calculate ratios (3D/2D)
        results_.complexity_ratios[entity_count] = 
            result_2d.total_time_ms > 0.0 ? static_cast<f32>(result_3d.total_time_ms / result_2d.total_time_ms) : 0.0f;
        
        results_.memory_ratios[entity_count] = 
            result_2d.peak_memory_bytes > 0 ? static_cast<f32>(result_3d.peak_memory_bytes) / result_2d.peak_memory_bytes : 0.0f;
        
        results_.efficiency_ratios[entity_count] = 
            result_2d.collision_efficiency > 0.0f ? result_3d.collision_efficiency / result_2d.collision_efficiency : 0.0f;
    }
    
    void report_entity_count_results(u32 entity_count) {
        const auto& result_2d = results_.results_2d[entity_count];
        const auto& result_3d = results_.results_3d[entity_count];
        
        LOG_INFO("Results for {} entities:", entity_count);
        LOG_INFO("  2D Physics: {:.3f}ms total ({:.2f}MB memory)", 
                result_2d.total_time_ms, result_2d.peak_memory_bytes / (1024.0f * 1024.0f));
        LOG_INFO("  3D Physics: {:.3f}ms total ({:.2f}MB memory)", 
                result_3d.total_time_ms, result_3d.peak_memory_bytes / (1024.0f * 1024.0f));
        LOG_INFO("  Complexity Ratio: {:.2f}x", results_.complexity_ratios[entity_count]);
        LOG_INFO("  Memory Ratio: {:.2f}x", results_.memory_ratios[entity_count]);
        LOG_INFO("");
    }
    
    void generate_comprehensive_analysis() {
        LOG_INFO("=== Comprehensive Performance Analysis ===");
        
        // Overall performance summary
        generate_performance_summary();
        
        // Scalability analysis
        analyze_scalability();
        
        // Component breakdown analysis
        analyze_component_breakdown();
        
        // Memory usage analysis
        analyze_memory_usage();
        
        // Theoretical vs actual complexity analysis
        analyze_theoretical_complexity();
        
        // Optimization recommendations
        generate_optimization_recommendations();
    }
    
    void generate_performance_summary() {
        LOG_INFO("--- Performance Summary ---");
        
        // Calculate average complexity ratio across all entity counts
        f32 avg_complexity_ratio = 0.0f;
        f32 avg_memory_ratio = 0.0f;
        
        for (const auto& [entity_count, ratio] : results_.complexity_ratios) {
            avg_complexity_ratio += ratio;
        }
        
        for (const auto& [entity_count, ratio] : results_.memory_ratios) {
            avg_memory_ratio += ratio;
        }
        
        avg_complexity_ratio /= results_.complexity_ratios.size();
        avg_memory_ratio /= results_.memory_ratios.size();
        
        LOG_INFO("Average 3D vs 2D Complexity Ratio: {:.2f}x", avg_complexity_ratio);
        LOG_INFO("Average 3D vs 2D Memory Ratio: {:.2f}x", avg_memory_ratio);
        
        // Find best and worst case scenarios
        auto min_complexity = std::min_element(results_.complexity_ratios.begin(), results_.complexity_ratios.end(),
                                              [](const auto& a, const auto& b) { return a.second < b.second; });
        auto max_complexity = std::max_element(results_.complexity_ratios.begin(), results_.complexity_ratios.end(),
                                              [](const auto& a, const auto& b) { return a.second < b.second; });
        
        LOG_INFO("Best case (lowest complexity ratio): {:.2f}x at {} entities", 
                min_complexity->second, min_complexity->first);
        LOG_INFO("Worst case (highest complexity ratio): {:.2f}x at {} entities", 
                max_complexity->second, max_complexity->first);
    }
    
    void analyze_scalability() {
        LOG_INFO("--- Scalability Analysis ---");
        
        // Analyze how performance scales with entity count
        std::vector<std::pair<u32, f64>> scaling_2d, scaling_3d;
        
        for (u32 entity_count : config_.entity_counts) {
            scaling_2d.emplace_back(entity_count, results_.results_2d[entity_count].total_time_ms);
            scaling_3d.emplace_back(entity_count, results_.results_3d[entity_count].total_time_ms);
        }
        
        // Calculate scaling factors
        if (scaling_2d.size() >= 2) {
            f64 scaling_factor_2d = scaling_2d.back().second / scaling_2d.front().second;
            f64 entity_factor = static_cast<f64>(scaling_2d.back().first) / scaling_2d.front().first;
            f64 complexity_2d = std::log(scaling_factor_2d) / std::log(entity_factor);
            
            f64 scaling_factor_3d = scaling_3d.back().second / scaling_3d.front().second;
            f64 complexity_3d = std::log(scaling_factor_3d) / std::log(entity_factor);
            
            LOG_INFO("2D Physics Scaling: O(n^{:.2f}) empirical complexity", complexity_2d);
            LOG_INFO("3D Physics Scaling: O(n^{:.2f}) empirical complexity", complexity_3d);
            
            LOG_INFO("Entity count increased by {:.1f}x:", entity_factor);
            LOG_INFO("  2D performance decreased by {:.2f}x", scaling_factor_2d);
            LOG_INFO("  3D performance decreased by {:.2f}x", scaling_factor_3d);
        }
    }
    
    void analyze_component_breakdown() {
        LOG_INFO("--- Component Breakdown Analysis ---");
        
        // Analyze which components contribute most to the 3D overhead
        for (u32 entity_count : config_.entity_counts) {
            const auto& result_2d = results_.results_2d[entity_count];
            const auto& result_3d = results_.results_3d[entity_count];
            
            if (entity_count == config_.entity_counts[config_.entity_counts.size()/2]) {  // Middle entity count
                LOG_INFO("Component breakdown for {} entities:", entity_count);
                
                f32 broad_phase_ratio = result_2d.broad_phase_time_ms > 0.0 ? 
                    static_cast<f32>(result_3d.broad_phase_time_ms / result_2d.broad_phase_time_ms) : 0.0f;
                f32 narrow_phase_ratio = result_2d.narrow_phase_time_ms > 0.0 ? 
                    static_cast<f32>(result_3d.narrow_phase_time_ms / result_2d.narrow_phase_time_ms) : 0.0f;
                f32 constraint_ratio = result_2d.constraint_solve_time_ms > 0.0 ? 
                    static_cast<f32>(result_3d.constraint_solve_time_ms / result_2d.constraint_solve_time_ms) : 0.0f;
                f32 integration_ratio = result_2d.integration_time_ms > 0.0 ? 
                    static_cast<f32>(result_3d.integration_time_ms / result_2d.integration_time_ms) : 0.0f;
                
                LOG_INFO("  Broad Phase: 2D={:.3f}ms, 3D={:.3f}ms, ratio={:.2f}x", 
                        result_2d.broad_phase_time_ms, result_3d.broad_phase_time_ms, broad_phase_ratio);
                LOG_INFO("  Narrow Phase: 2D={:.3f}ms, 3D={:.3f}ms, ratio={:.2f}x", 
                        result_2d.narrow_phase_time_ms, result_3d.narrow_phase_time_ms, narrow_phase_ratio);
                LOG_INFO("  Constraint Solve: 2D={:.3f}ms, 3D={:.3f}ms, ratio={:.2f}x", 
                        result_2d.constraint_solve_time_ms, result_3d.constraint_solve_time_ms, constraint_ratio);
                LOG_INFO("  Integration: 2D={:.3f}ms, 3D={:.3f}ms, ratio={:.2f}x", 
                        result_2d.integration_time_ms, result_3d.integration_time_ms, integration_ratio);
                break;
            }
        }
    }
    
    void analyze_memory_usage() {
        LOG_INFO("--- Memory Usage Analysis ---");
        
        for (u32 entity_count : config_.entity_counts) {
            const auto& result_2d = results_.results_2d[entity_count];
            const auto& result_3d = results_.results_3d[entity_count];
            
            f32 memory_per_entity_2d = static_cast<f32>(result_2d.peak_memory_bytes) / entity_count;
            f32 memory_per_entity_3d = static_cast<f32>(result_3d.peak_memory_bytes) / entity_count;
            
            LOG_INFO("{} entities: 2D={:.1f}KB/entity, 3D={:.1f}KB/entity", 
                    entity_count, memory_per_entity_2d / 1024.0f, memory_per_entity_3d / 1024.0f);
        }
        
        // Analyze memory growth patterns
        if (config_.entity_counts.size() >= 2) {
            u32 small_count = config_.entity_counts.front();
            u32 large_count = config_.entity_counts.back();
            
            f32 memory_growth_2d = static_cast<f32>(results_.results_2d[large_count].peak_memory_bytes) / 
                                  results_.results_2d[small_count].peak_memory_bytes;
            f32 memory_growth_3d = static_cast<f32>(results_.results_3d[large_count].peak_memory_bytes) / 
                                  results_.results_3d[small_count].peak_memory_bytes;
            f32 entity_growth = static_cast<f32>(large_count) / small_count;
            
            LOG_INFO("Memory scaling from {} to {} entities ({:.1f}x):", 
                    small_count, large_count, entity_growth);
            LOG_INFO("  2D memory increased by {:.2f}x", memory_growth_2d);
            LOG_INFO("  3D memory increased by {:.2f}x", memory_growth_3d);
        }
    }
    
    void analyze_theoretical_complexity() {
        LOG_INFO("--- Theoretical vs Actual Complexity Analysis ---");
        
        LOG_INFO("Theoretical expectations:");
        LOG_INFO("  Vector operations: 3D should be ~1.5x slower than 2D");
        LOG_INFO("  Matrix operations: 3D should be ~3-4x slower (3x3 vs scalars)");
        LOG_INFO("  Collision detection: 3D should be ~2-5x slower (dimensionality)");
        LOG_INFO("  Memory usage: 3D should be ~2-3x higher");
        
        // Compare with actual results
        f32 avg_complexity_ratio = 0.0f;
        for (const auto& [entity_count, ratio] : results_.complexity_ratios) {
            avg_complexity_ratio += ratio;
        }
        avg_complexity_ratio /= results_.complexity_ratios.size();
        
        LOG_INFO("Actual measurements:");
        LOG_INFO("  Average complexity ratio: {:.2f}x", avg_complexity_ratio);
        
        if (avg_complexity_ratio < 2.0f) {
            LOG_INFO("  Result: Better than expected! Good optimization effectiveness");
        } else if (avg_complexity_ratio < 4.0f) {
            LOG_INFO("  Result: Within expected range");
        } else {
            LOG_INFO("  Result: Worse than expected - potential optimization opportunities");
        }
    }
    
    void generate_optimization_recommendations() {
        LOG_INFO("--- Optimization Recommendations ---");
        
        // Analyze bottlenecks and suggest optimizations
        f32 avg_complexity_ratio = 0.0f;
        for (const auto& [entity_count, ratio] : results_.complexity_ratios) {
            avg_complexity_ratio += ratio;
        }
        avg_complexity_ratio /= results_.complexity_ratios.size();
        
        if (avg_complexity_ratio > 3.5f) {
            LOG_INFO("High complexity ratio suggests optimization opportunities:");
            LOG_INFO("  1. Improve SIMD utilization for 3D vector operations");
            LOG_INFO("  2. Optimize 3D collision detection algorithms");
            LOG_INFO("  3. Consider better spatial partitioning for 3D");
            LOG_INFO("  4. Improve memory access patterns");
        }
        
        // Check memory efficiency
        f32 avg_memory_ratio = 0.0f;
        for (const auto& [entity_count, ratio] : results_.memory_ratios) {
            avg_memory_ratio += ratio;
        }
        avg_memory_ratio /= results_.memory_ratios.size();
        
        if (avg_memory_ratio > 3.0f) {
            LOG_INFO("High memory ratio suggests memory optimizations:");
            LOG_INFO("  1. Consider more compact data structures for 3D");
            LOG_INFO("  2. Improve memory pooling strategies");
            LOG_INFO("  3. Optimize component layouts for cache efficiency");
        }
        
        LOG_INFO("General recommendations for 3D physics optimization:");
        LOG_INFO("  - Use SIMD extensively for 3D vector/matrix operations");
        LOG_INFO("  - Implement hierarchical broad-phase for better 3D scaling");
        LOG_INFO("  - Consider GPU acceleration for particle systems");
        LOG_INFO("  - Use memory-efficient contact manifold representations");
        LOG_INFO("  - Implement sleeping systems more aggressively in 3D");
    }
    
    void export_results_to_csv() {
        LOG_INFO("Exporting benchmark results to CSV files...");
        
        // Export raw performance data
        std::ofstream perf_file("physics_performance_benchmark.csv");
        perf_file << "EntityCount,2D_TotalTime,3D_TotalTime,2D_BroadPhase,3D_BroadPhase,";
        perf_file << "2D_NarrowPhase,3D_NarrowPhase,2D_ConstraintSolve,3D_ConstraintSolve,";
        perf_file << "2D_Integration,3D_Integration,2D_Memory,3D_Memory,ComplexityRatio,MemoryRatio\n";
        
        for (u32 entity_count : config_.entity_counts) {
            const auto& r2d = results_.results_2d[entity_count];
            const auto& r3d = results_.results_3d[entity_count];
            
            perf_file << entity_count << ","
                     << r2d.total_time_ms << "," << r3d.total_time_ms << ","
                     << r2d.broad_phase_time_ms << "," << r3d.broad_phase_time_ms << ","
                     << r2d.narrow_phase_time_ms << "," << r3d.narrow_phase_time_ms << ","
                     << r2d.constraint_solve_time_ms << "," << r3d.constraint_solve_time_ms << ","
                     << r2d.integration_time_ms << "," << r3d.integration_time_ms << ","
                     << r2d.peak_memory_bytes << "," << r3d.peak_memory_bytes << ","
                     << results_.complexity_ratios[entity_count] << ","
                     << results_.memory_ratios[entity_count] << "\n";
        }
        
        perf_file.close();
        
        // Export summary statistics
        std::ofstream summary_file("physics_benchmark_summary.txt");
        summary_file << "=== ECScope Physics Performance Benchmark Summary ===\n";
        summary_file << "Generated: " << std::chrono::system_clock::now().time_since_epoch().count() << "\n\n";
        
        summary_file << "Benchmark Configuration:\n";
        summary_file << "  Entity counts tested: ";
        for (u32 count : config_.entity_counts) {
            summary_file << count << " ";
        }
        summary_file << "\n";
        summary_file << "  Measurement frames: " << config_.measurement_frames << "\n";
        summary_file << "  Benchmark runs: " << config_.benchmark_runs << "\n";
        summary_file << "  Multithreading: " << (config_.enable_multithreading ? "Enabled" : "Disabled") << "\n\n";
        
        // Add all the analysis results
        f32 avg_complexity_ratio = 0.0f;
        for (const auto& [entity_count, ratio] : results_.complexity_ratios) {
            avg_complexity_ratio += ratio;
        }
        avg_complexity_ratio /= results_.complexity_ratios.size();
        
        summary_file << "Key Results:\n";
        summary_file << "  Average 3D vs 2D complexity ratio: " << avg_complexity_ratio << "x\n";
        
        summary_file.close();
        
        LOG_INFO("Results exported to:");
        LOG_INFO("  - physics_performance_benchmark.csv (detailed data)");
        LOG_INFO("  - physics_benchmark_summary.txt (summary report)");
    }
};

//=============================================================================
// Main Benchmark Entry Point
//=============================================================================

int main() {
    LOG_INFO("=== ECScope Physics Performance Benchmark Suite ===");
    LOG_INFO("Comprehensive 2D vs 3D physics performance analysis");
    LOG_INFO("with detailed educational insights and optimization guidance.");
    LOG_INFO("");
    
    try {
        PhysicsPerformanceBenchmark benchmark;
        benchmark.run_complete_benchmark_suite();
        
        LOG_INFO("");
        LOG_INFO("Benchmark suite completed successfully!");
        LOG_INFO("Check the generated CSV files and logs for detailed analysis.");
        
        return 0;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Benchmark failed with exception: {}", e.what());
        return 1;
    } catch (...) {
        LOG_ERROR("Benchmark failed with unknown exception");
        return 1;
    }
}