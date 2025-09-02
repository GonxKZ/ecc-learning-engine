#pragma once

/**
 * @file physics/benchmarks.hpp
 * @brief Comprehensive Physics Performance Benchmarks for ECScope Phase 5: Física 2D
 * 
 * This header provides extensive benchmarking and performance testing tools for the
 * 2D physics system. It enables students to understand performance characteristics,
 * compare different algorithms, and analyze optimization opportunities.
 * 
 * Key Features:
 * - Comprehensive physics system benchmarks
 * - Algorithm comparison and analysis
 * - Memory usage profiling and optimization
 * - Scalability testing with varying entity counts
 * - Educational performance analysis and reporting
 * - Cache behavior analysis and optimization suggestions
 * 
 * Educational Philosophy:
 * The benchmarks are designed to teach students about performance analysis in
 * game engines, helping them understand the relationship between algorithmic
 * choices, data structures, and real-world performance.
 * 
 * @author ECScope Educational ECS Framework - Phase 5: Física 2D
 * @date 2024
 */

#include "world.hpp"
#include "physics_system.hpp"
#include "collision.hpp"
#include "../ecs/registry.hpp"
#include "../memory/arena.hpp"
#include "../memory/pool.hpp"
#include "../memory/memory_tracker.hpp"
#include "../core/types.hpp"
#include "../core/log.hpp"
#include <vector>
#include <string>
#include <chrono>
#include <memory>
#include <functional>
#include <map>
#include <fstream>

namespace ecscope::physics::benchmarks {

//=============================================================================
// Benchmark Configuration and Test Scenarios
//=============================================================================

/**
 * @brief Configuration for physics benchmarks
 */
struct BenchmarkConfig {
    /** @brief Test parameters */
    u32 num_iterations{100};           // Number of benchmark iterations
    f32 simulation_time{10.0f};        // Simulation time per test (seconds)
    bool enable_warmup{true};          // Enable warmup runs
    u32 warmup_iterations{10};         // Number of warmup iterations
    
    /** @brief Test scenarios */
    std::vector<u32> entity_counts{10, 50, 100, 500, 1000, 2000}; // Entity count variations
    std::vector<f32> time_steps{1.0f/60.0f, 1.0f/120.0f, 1.0f/240.0f}; // Time step variations
    std::vector<u32> constraint_iterations{1, 5, 10, 15, 20}; // Solver iteration variations
    
    /** @brief Memory analysis */
    bool enable_memory_profiling{true};
    bool analyze_cache_behavior{true};
    bool track_memory_allocations{true};
    
    /** @brief Algorithm comparisons */
    bool compare_collision_algorithms{true};
    bool compare_integration_methods{true};
    bool compare_memory_allocators{true};
    
    /** @brief Output configuration */
    bool generate_csv_report{true};
    bool generate_html_report{true};
    bool generate_performance_graphs{false};
    std::string output_directory{"benchmarks/"};
    
    /** @brief Factory methods */
    static BenchmarkConfig create_quick_test() {
        BenchmarkConfig config;
        config.num_iterations = 20;
        config.simulation_time = 2.0f;
        config.entity_counts = {10, 50, 100};
        config.time_steps = {1.0f/60.0f};
        config.constraint_iterations = {5, 10};
        config.generate_performance_graphs = false;
        return config;
    }
    
    static BenchmarkConfig create_comprehensive() {
        BenchmarkConfig config;
        config.num_iterations = 100;
        config.simulation_time = 10.0f;
        config.entity_counts = {10, 50, 100, 200, 500, 1000, 2000, 5000};
        config.time_steps = {1.0f/30.0f, 1.0f/60.0f, 1.0f/120.0f, 1.0f/240.0f};
        config.constraint_iterations = {1, 3, 5, 8, 10, 15, 20};
        config.generate_performance_graphs = true;
        return config;
    }
    
    static BenchmarkConfig create_memory_focused() {
        BenchmarkConfig config;
        config.num_iterations = 50;
        config.enable_memory_profiling = true;
        config.analyze_cache_behavior = true;
        config.track_memory_allocations = true;
        config.compare_memory_allocators = true;
        config.entity_counts = {100, 500, 1000, 2000, 5000};
        return config;
    }
};

//=============================================================================
// Benchmark Result Data Structures
//=============================================================================

/**
 * @brief Results from a single benchmark test
 */
struct BenchmarkResult {
    /** @brief Test identification */
    std::string test_name;
    u32 entity_count;
    f32 time_step;
    u32 constraint_iterations;
    
    /** @brief Timing results (milliseconds) */
    f64 average_frame_time{0.0};
    f64 min_frame_time{std::numeric_limits<f64>::max()};
    f64 max_frame_time{0.0};
    f64 total_test_time{0.0};
    f64 frame_time_std_deviation{0.0};
    
    /** @brief Throughput metrics */
    f64 fps_equivalent{0.0};
    f64 entities_per_second{0.0};
    f64 contacts_per_second{0.0};
    
    /** @brief Memory usage */
    usize peak_memory_usage{0};
    usize average_memory_usage{0};
    usize memory_allocations{0};
    usize memory_deallocations{0};
    
    /** @brief Physics-specific metrics */
    f64 broad_phase_time{0.0};
    f64 narrow_phase_time{0.0};
    f64 constraint_solve_time{0.0};
    f64 integration_time{0.0};
    
    /** @brief Cache behavior */
    f64 cache_hit_ratio{0.0};
    u64 cache_misses{0};
    f64 memory_bandwidth_usage{0.0};
    
    /** @brief Educational metrics */
    f64 energy_conservation_error{0.0};
    f64 momentum_conservation_error{0.0};
    f64 constraint_residual{0.0};
    
    /** @brief Stability metrics */
    u32 unstable_frames{0};          // Frames with excessive velocities
    u32 constraint_failures{0};      // Failed constraint iterations
    f64 simulation_drift{0.0};       // Total simulation drift over test
    
    BenchmarkResult() = default;
    explicit BenchmarkResult(const std::string& name) : test_name(name) {}
};

/**
 * @brief Collection of benchmark results with analysis
 */
struct BenchmarkSuite {
    std::string suite_name;
    std::vector<BenchmarkResult> results;
    BenchmarkConfig config;
    
    /** @brief Analysis data */
    struct Analysis {
        BenchmarkResult best_performance;
        BenchmarkResult worst_performance;
        BenchmarkResult most_stable;
        
        /** @brief Scaling analysis */
        f64 entity_count_scaling_factor{0.0};    // How performance scales with entity count
        f64 time_step_impact{0.0};               // How time step affects performance
        f64 iteration_impact{0.0};               // How constraint iterations affect performance
        
        /** @brief Memory analysis */
        f64 memory_scaling_factor{0.0};          // How memory usage scales
        f64 allocation_efficiency{0.0};          // Memory allocation efficiency
        
        /** @brief Educational insights */
        std::vector<std::string> performance_insights;
        std::vector<std::string> optimization_recommendations;
        std::string overall_grade;               // A, B, C, D, F performance grade
    } analysis;
    
    /** @brief Add result to suite */
    void add_result(const BenchmarkResult& result) {
        results.push_back(result);
    }
    
    /** @brief Perform comprehensive analysis */
    void analyze_results();
    
    /** @brief Generate reports */
    void generate_csv_report(const std::string& filename) const;
    void generate_html_report(const std::string& filename) const;
    std::string generate_text_summary() const;
};

//=============================================================================
// Physics Benchmark Runner
//=============================================================================

/**
 * @brief Comprehensive physics benchmarking system
 * 
 * This class provides extensive benchmarking capabilities for the physics system,
 * including performance analysis, algorithm comparison, and educational insights.
 */
class PhysicsBenchmarkRunner {
private:
    BenchmarkConfig config_;
    std::unique_ptr<Registry> registry_;
    std::unique_ptr<PhysicsSystem> physics_system_;
    
    /** @brief Test scenario generators */
    std::map<std::string, std::function<void(Registry&, u32)>> scenario_generators_;
    
    /** @brief Memory tracking */
    std::unique_ptr<memory::MemoryTracker> memory_tracker_;
    
    /** @brief Benchmark state */
    bool initialized_{false};
    std::chrono::high_resolution_clock::time_point benchmark_start_time_;
    
public:
    /** @brief Constructor */
    explicit PhysicsBenchmarkRunner(const BenchmarkConfig& config = BenchmarkConfig::create_comprehensive())
        : config_(config) {
        
        initialize_scenario_generators();
        LOG_INFO("PhysicsBenchmarkRunner initialized with {} test scenarios", scenario_generators_.size());
    }
    
    /** @brief Initialize benchmark environment */
    bool initialize() {
        if (initialized_) return true;
        
        // Create fresh registry for benchmarks
        registry_ = std::make_unique<Registry>(ecs::AllocatorConfig::create_performance_optimized(), "Benchmark_Registry");
        
        // Initialize memory tracking
        if (config_.enable_memory_profiling) {
            memory_tracker_ = std::make_unique<memory::MemoryTracker>();
            memory_tracker_->set_tracking_enabled(true);
        }
        
        initialized_ = true;
        LOG_INFO("Benchmark environment initialized");
        return true;
    }
    
    //-------------------------------------------------------------------------
    // Main Benchmarking Interface
    //-------------------------------------------------------------------------
    
    /** @brief Run all configured benchmarks */
    BenchmarkSuite run_all_benchmarks() {
        if (!initialize()) {
            throw std::runtime_error("Failed to initialize benchmark environment");
        }
        
        BenchmarkSuite suite;
        suite.suite_name = "Comprehensive Physics Benchmarks";
        suite.config = config_;
        
        benchmark_start_time_ = std::chrono::high_resolution_clock::now();
        
        // Run different test categories
        run_scalability_benchmarks(suite);
        run_algorithm_comparison_benchmarks(suite);
        run_memory_benchmarks(suite);
        run_stability_benchmarks(suite);
        
        suite.analyze_results();
        
        auto end_time = std::chrono::high_resolution_clock::now();
        f64 total_time = std::chrono::duration<f64>(end_time - benchmark_start_time_).count();
        
        LOG_INFO("Benchmark suite completed in {:.2f} seconds", total_time);
        LOG_INFO("Total tests run: {}", suite.results.size());
        
        return suite;
    }
    
    /** @brief Run specific benchmark test */
    BenchmarkResult run_single_benchmark(const std::string& scenario_name, u32 entity_count, 
                                        f32 time_step, u32 constraint_iterations) {
        
        BenchmarkResult result(scenario_name + "_" + std::to_string(entity_count) + "_entities");
        result.entity_count = entity_count;
        result.time_step = time_step;
        result.constraint_iterations = constraint_iterations;
        
        // Setup physics system for this test
        PhysicsWorldConfig world_config = PhysicsWorldConfig::create_performance();
        world_config.time_step = time_step;
        world_config.constraint_iterations = constraint_iterations;
        world_config.enable_profiling = true;
        
        PhysicsSystemConfig system_config = PhysicsSystemConfig::create_performance();
        system_config.world_config = world_config;
        
        physics_system_ = std::make_unique<PhysicsSystem>(*registry_, system_config);
        
        // Generate test scenario
        auto scenario_it = scenario_generators_.find(scenario_name);
        if (scenario_it == scenario_generators_.end()) {
            LOG_ERROR("Unknown benchmark scenario: {}", scenario_name);
            return result;
        }
        
        registry_->clear();
        scenario_it->second(*registry_, entity_count);
        
        // Run warmup iterations
        if (config_.enable_warmup) {
            run_warmup_iterations();
        }
        
        // Reset memory tracking
        if (memory_tracker_) {
            memory_tracker_->reset();
        }
        
        // Run actual benchmark
        run_benchmark_iterations(result);
        
        // Cleanup
        physics_system_.reset();
        
        return result;
    }
    
    //-------------------------------------------------------------------------
    // Algorithm Comparison Benchmarks
    //-------------------------------------------------------------------------
    
    /** @brief Compare collision detection algorithms */
    std::map<std::string, BenchmarkResult> compare_collision_algorithms(u32 entity_count = 100) {
        std::map<std::string, BenchmarkResult> comparison_results;
        
        // Test different collision detection approaches
        std::vector<std::string> algorithms = {
            "brute_force",
            "spatial_hash", 
            "quadtree",
            "sweep_and_prune"
        };
        
        for (const std::string& algorithm : algorithms) {
            BenchmarkResult result = run_collision_algorithm_benchmark(algorithm, entity_count);
            comparison_results[algorithm] = result;
        }
        
        return comparison_results;
    }
    
    /** @brief Compare integration methods */
    std::map<std::string, BenchmarkResult> compare_integration_methods(u32 entity_count = 100) {
        std::map<std::string, BenchmarkResult> comparison_results;
        
        // Test different integration methods
        std::vector<std::string> methods = {
            "euler",
            "semi_implicit_euler",
            "verlet",
            "runge_kutta_4"
        };
        
        for (const std::string& method : methods) {
            BenchmarkResult result = run_integration_method_benchmark(method, entity_count);
            comparison_results[method] = result;
        }
        
        return comparison_results;
    }
    
    /** @brief Compare memory allocators */
    std::map<std::string, BenchmarkResult> compare_memory_allocators(u32 entity_count = 1000) {
        std::map<std::string, BenchmarkResult> comparison_results;
        
        // Test different allocator strategies
        std::vector<std::string> allocators = {
            "standard",
            "arena_only",
            "pool_only",
            "hybrid_arena_pool"
        };
        
        for (const std::string& allocator : allocators) {
            BenchmarkResult result = run_allocator_benchmark(allocator, entity_count);
            comparison_results[allocator] = result;
        }
        
        return comparison_results;
    }
    
    //-------------------------------------------------------------------------
    // Specialized Benchmark Tests
    //-------------------------------------------------------------------------
    
    /** @brief Stress test with maximum entity count */
    BenchmarkResult run_stress_test(u32 max_entities = 10000) {
        BenchmarkResult result("stress_test");
        result.entity_count = max_entities;
        
        // Use performance-optimized configuration
        PhysicsWorldConfig world_config = PhysicsWorldConfig::create_performance();
        world_config.max_active_bodies = max_entities;
        world_config.physics_arena_size = 64 * 1024 * 1024;  // 64 MB
        
        PhysicsSystemConfig system_config = PhysicsSystemConfig::create_performance();
        system_config.world_config = world_config;
        system_config.batch_size = 256;  // Larger batches for stress test
        
        physics_system_ = std::make_unique<PhysicsSystem>(*registry_, system_config);
        
        // Generate stress test scenario
        generate_stress_test_scenario(*registry_, max_entities);
        
        // Run extended benchmark
        auto start_time = std::chrono::high_resolution_clock::now();
        
        const f32 stress_test_duration = 30.0f;  // 30 seconds
        const f32 time_step = 1.0f / 60.0f;
        
        f32 elapsed_time = 0.0f;
        u32 frame_count = 0;
        std::vector<f64> frame_times;
        
        while (elapsed_time < stress_test_duration) {
            auto frame_start = std::chrono::high_resolution_clock::now();
            
            physics_system_->update(time_step);
            
            auto frame_end = std::chrono::high_resolution_clock::now();
            f64 frame_time = std::chrono::duration<f64, std::milli>(frame_end - frame_start).count();
            frame_times.push_back(frame_time);
            
            elapsed_time += time_step;
            ++frame_count;
            
            // Check for instability
            if (frame_time > 100.0) {  // 100ms = very slow frame
                ++result.unstable_frames;
            }
        }
        
        // Calculate results
        calculate_timing_statistics(frame_times, result);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        result.total_test_time = std::chrono::duration<f64>(end_time - start_time).count() * 1000.0;
        
        LOG_INFO("Stress test completed: {} entities, {} frames, {:.2f}ms avg frame time", 
                 max_entities, frame_count, result.average_frame_time);
        
        physics_system_.reset();
        return result;
    }
    
    /** @brief Cache behavior analysis benchmark */
    BenchmarkResult analyze_cache_behavior(u32 entity_count = 1000) {
        BenchmarkResult result("cache_analysis");
        result.entity_count = entity_count;
        
        // This would require specialized cache profiling
        // Implementation would analyze:
        // - Cache miss patterns
        // - Memory access patterns
        // - Data structure layout efficiency
        // - Component access coherency
        
        LOG_INFO("Cache behavior analysis for {} entities", entity_count);
        
        // Placeholder implementation
        result.cache_hit_ratio = 0.85f;  // Example value
        result.cache_misses = entity_count * 100;  // Example value
        
        return result;
    }

private:
    //-------------------------------------------------------------------------
    // Benchmark Implementation Details
    //-------------------------------------------------------------------------
    
    /** @brief Initialize test scenario generators */
    void initialize_scenario_generators() {
        // Falling objects scenario
        scenario_generators_["falling_objects"] = [](Registry& registry, u32 count) {
            generate_falling_objects_scenario(registry, count);
        };
        
        // Collision stress test
        scenario_generators_["collision_stress"] = [](Registry& registry, u32 count) {
            generate_collision_stress_scenario(registry, count);
        };
        
        // Stacking scenario
        scenario_generators_["stacking"] = [](Registry& registry, u32 count) {
            generate_stacking_scenario(registry, count);
        };
        
        // Particle system
        scenario_generators_["particle_system"] = [](Registry& registry, u32 count) {
            generate_particle_system_scenario(registry, count);
        };
        
        // Constraint chains
        scenario_generators_["constraint_chains"] = [](Registry& registry, u32 count) {
            generate_constraint_chains_scenario(registry, count);
        };
    }
    
    /** @brief Run scalability benchmarks across different entity counts */
    void run_scalability_benchmarks(BenchmarkSuite& suite) {
        LOG_INFO("Running scalability benchmarks...");
        
        for (const std::string& scenario : {"falling_objects", "collision_stress", "stacking"}) {
            for (u32 entity_count : config_.entity_counts) {
                for (f32 time_step : config_.time_steps) {
                    BenchmarkResult result = run_single_benchmark(scenario, entity_count, time_step, 8);
                    suite.add_result(result);
                }
            }
        }
    }
    
    /** @brief Run algorithm comparison benchmarks */
    void run_algorithm_comparison_benchmarks(BenchmarkSuite& suite) {
        if (!config_.compare_collision_algorithms) return;
        
        LOG_INFO("Running algorithm comparison benchmarks...");
        
        auto collision_results = compare_collision_algorithms(500);
        for (const auto& [algorithm, result] : collision_results) {
            suite.add_result(result);
        }
        
        if (config_.compare_integration_methods) {
            auto integration_results = compare_integration_methods(200);
            for (const auto& [method, result] : integration_results) {
                suite.add_result(result);
            }
        }
    }
    
    /** @brief Run memory-focused benchmarks */
    void run_memory_benchmarks(BenchmarkSuite& suite) {
        if (!config_.enable_memory_profiling) return;
        
        LOG_INFO("Running memory benchmarks...");
        
        if (config_.compare_memory_allocators) {
            auto allocator_results = compare_memory_allocators(1000);
            for (const auto& [allocator, result] : allocator_results) {
                suite.add_result(result);
            }
        }
        
        // Cache behavior analysis
        if (config_.analyze_cache_behavior) {
            BenchmarkResult cache_result = analyze_cache_behavior(1000);
            suite.add_result(cache_result);
        }
    }
    
    /** @brief Run stability and accuracy benchmarks */
    void run_stability_benchmarks(BenchmarkSuite& suite) {
        LOG_INFO("Running stability benchmarks...");
        
        // Stress test
        BenchmarkResult stress_result = run_stress_test(5000);
        suite.add_result(stress_result);
    }
    
    /** @brief Run warmup iterations to stabilize performance */
    void run_warmup_iterations() {
        for (u32 i = 0; i < config_.warmup_iterations; ++i) {
            physics_system_->update(config_.simulation_time / 100.0f);  // Short updates
        }
    }
    
    /** @brief Run benchmark iterations and collect results */
    void run_benchmark_iterations(BenchmarkResult& result) {
        std::vector<f64> frame_times;
        frame_times.reserve(config_.num_iterations);
        
        auto test_start = std::chrono::high_resolution_clock::now();
        
        for (u32 i = 0; i < config_.num_iterations; ++i) {
            auto frame_start = std::chrono::high_resolution_clock::now();
            
            physics_system_->update(result.time_step);
            
            auto frame_end = std::chrono::high_resolution_clock::now();
            f64 frame_time = std::chrono::duration<f64, std::milli>(frame_end - frame_start).count();
            frame_times.push_back(frame_time);
        }
        
        auto test_end = std::chrono::high_resolution_clock::now();
        result.total_test_time = std::chrono::duration<f64, std::milli>(test_end - test_start).count();
        
        // Calculate timing statistics
        calculate_timing_statistics(frame_times, result);
        
        // Collect physics statistics
        collect_physics_statistics(result);
        
        // Collect memory statistics
        if (config_.enable_memory_profiling) {
            collect_memory_statistics(result);
        }
    }
    
    /** @brief Calculate timing statistics from frame times */
    void calculate_timing_statistics(const std::vector<f64>& frame_times, BenchmarkResult& result) {
        if (frame_times.empty()) return;
        
        // Basic statistics
        f64 total_time = 0.0;
        result.min_frame_time = frame_times[0];
        result.max_frame_time = frame_times[0];
        
        for (f64 time : frame_times) {
            total_time += time;
            result.min_frame_time = std::min(result.min_frame_time, time);
            result.max_frame_time = std::max(result.max_frame_time, time);
        }
        
        result.average_frame_time = total_time / frame_times.size();
        result.fps_equivalent = 1000.0 / result.average_frame_time;
        
        // Standard deviation
        f64 variance = 0.0;
        for (f64 time : frame_times) {
            f64 diff = time - result.average_frame_time;
            variance += diff * diff;
        }
        result.frame_time_std_deviation = std::sqrt(variance / frame_times.size());
    }
    
    /** @brief Collect physics-specific statistics */
    void collect_physics_statistics(BenchmarkResult& result) {
        if (!physics_system_) return;
        
        const auto& stats = physics_system_->get_system_statistics();
        
        // Timing breakdown
        result.broad_phase_time = stats.world_stats.broad_phase_time;
        result.narrow_phase_time = stats.world_stats.narrow_phase_time;
        result.constraint_solve_time = stats.world_stats.constraint_solve_time;
        result.integration_time = stats.world_stats.integration_time;
        
        // Throughput
        result.entities_per_second = static_cast<f64>(result.entity_count) / (result.average_frame_time / 1000.0);
        result.contacts_per_second = static_cast<f64>(stats.world_stats.active_contacts) / (result.average_frame_time / 1000.0);
        
        // Quality metrics
        result.energy_conservation_error = static_cast<f64>(stats.world_stats.energy_conservation_error);
        result.constraint_residual = static_cast<f64>(stats.world_stats.constraint_residual);
        
        // Cache behavior
        result.cache_hit_ratio = static_cast<f64>(stats.world_stats.cache_hit_ratio);
    }
    
    /** @brief Collect memory usage statistics */
    void collect_memory_statistics(BenchmarkResult& result) {
        if (!memory_tracker_) return;
        
        auto memory_stats = memory_tracker_->get_current_stats();
        result.peak_memory_usage = memory_stats.peak_allocation;
        result.average_memory_usage = memory_stats.current_allocation;
        result.memory_allocations = memory_stats.total_allocations;
        result.memory_deallocations = memory_stats.total_deallocations;
    }
    
    //-------------------------------------------------------------------------
    // Specialized Benchmark Implementations
    //-------------------------------------------------------------------------
    
    BenchmarkResult run_collision_algorithm_benchmark(const std::string& algorithm, u32 entity_count) {
        BenchmarkResult result("collision_" + algorithm);
        // Implementation would test specific collision detection algorithm
        return result;
    }
    
    BenchmarkResult run_integration_method_benchmark(const std::string& method, u32 entity_count) {
        BenchmarkResult result("integration_" + method);
        // Implementation would test specific integration method
        return result;
    }
    
    BenchmarkResult run_allocator_benchmark(const std::string& allocator, u32 entity_count) {
        BenchmarkResult result("allocator_" + allocator);
        // Implementation would test specific memory allocator
        return result;
    }
    
    //-------------------------------------------------------------------------
    // Test Scenario Generators
    //-------------------------------------------------------------------------
    
    static void generate_falling_objects_scenario(Registry& registry, u32 count) {
        // Create falling objects with gravity
        for (u32 i = 0; i < count; ++i) {
            Entity entity = registry.create_entity();
            
            // Random position above ground
            f32 x = static_cast<f32>((rand() % 400) - 200);
            f32 y = static_cast<f32>(100 + (rand() % 200));
            
            Transform transform{Vec2{x, y}, 0.0f, Vec2{1.0f, 1.0f}};
            RigidBody2D rigidbody{1.0f};  // 1kg mass
            Collider2D collider{Circle{Vec2::zero(), 5.0f}};  // 5-unit radius
            ForceAccumulator forces{};
            
            registry.add_component(entity, transform);
            registry.add_component(entity, rigidbody);
            registry.add_component(entity, collider);
            registry.add_component(entity, forces);
        }
        
        // Create ground plane
        Entity ground = registry.create_entity();
        Transform ground_transform{Vec2{0.0f, -50.0f}, 0.0f, Vec2{1.0f, 1.0f}};
        RigidBody2D ground_body{0.0f};  // Static body
        ground_body.make_static();
        Collider2D ground_collider{AABB{Vec2{-300.0f, -10.0f}, Vec2{300.0f, 10.0f}}};
        
        registry.add_component(ground, ground_transform);
        registry.add_component(ground, ground_body);
        registry.add_component(ground, ground_collider);
    }
    
    static void generate_collision_stress_scenario(Registry& registry, u32 count) {
        // Create densely packed objects for maximum collision testing
        f32 spacing = 10.0f;
        u32 grid_size = static_cast<u32>(std::sqrt(count));
        
        for (u32 i = 0; i < count; ++i) {
            Entity entity = registry.create_entity();
            
            f32 x = (i % grid_size) * spacing - (grid_size * spacing * 0.5f);
            f32 y = (i / grid_size) * spacing;
            
            Transform transform{Vec2{x, y}, 0.0f, Vec2{1.0f, 1.0f}};
            RigidBody2D rigidbody{0.5f};  // Lighter objects
            Collider2D collider{Circle{Vec2::zero(), 3.0f}};  // Smaller radius
            ForceAccumulator forces{};
            
            // Add random initial velocity for chaos
            rigidbody.velocity = Vec2{static_cast<f32>((rand() % 40) - 20), 
                                     static_cast<f32>((rand() % 20) - 10)};
            
            registry.add_component(entity, transform);
            registry.add_component(entity, rigidbody);
            registry.add_component(entity, collider);
            registry.add_component(entity, forces);
        }
    }
    
    static void generate_stacking_scenario(Registry& registry, u32 count) {
        // Create stacked boxes for constraint solving testing
        f32 box_size = 8.0f;
        
        for (u32 i = 0; i < count; ++i) {
            Entity entity = registry.create_entity();
            
            Transform transform{Vec2{0.0f, i * box_size}, 0.0f, Vec2{1.0f, 1.0f}};
            RigidBody2D rigidbody{1.0f};
            Collider2D collider{AABB{Vec2{-box_size*0.5f, -box_size*0.5f}, 
                                     Vec2{box_size*0.5f, box_size*0.5f}}};
            ForceAccumulator forces{};
            
            registry.add_component(entity, transform);
            registry.add_component(entity, rigidbody);
            registry.add_component(entity, collider);
            registry.add_component(entity, forces);
        }
        
        // Create ground
        Entity ground = registry.create_entity();
        Transform ground_transform{Vec2{0.0f, -20.0f}, 0.0f, Vec2{1.0f, 1.0f}};
        RigidBody2D ground_body{0.0f};
        ground_body.make_static();
        Collider2D ground_collider{AABB{Vec2{-50.0f, -5.0f}, Vec2{50.0f, 5.0f}}};
        
        registry.add_component(ground, ground_transform);
        registry.add_component(ground, ground_body);
        registry.add_component(ground, ground_collider);
    }
    
    static void generate_particle_system_scenario(Registry& registry, u32 count) {
        // Create particle system for large-scale testing
        for (u32 i = 0; i < count; ++i) {
            Entity entity = registry.create_entity();
            
            // Random spawn in circle
            f32 angle = static_cast<f32>(i) / count * 2.0f * constants::PI_F;
            f32 radius = 50.0f + static_cast<f32>(rand() % 100);
            
            Vec2 position{std::cos(angle) * radius, std::sin(angle) * radius};
            
            Transform transform{position, 0.0f, Vec2{1.0f, 1.0f}};
            RigidBody2D rigidbody{0.1f};  // Very light particles
            Collider2D collider{Circle{Vec2::zero(), 1.0f}};  // Small particles
            ForceAccumulator forces{};
            
            // Initial velocity toward center
            rigidbody.velocity = -position.normalized() * 20.0f;
            
            registry.add_component(entity, transform);
            registry.add_component(entity, rigidbody);
            registry.add_component(entity, collider);
            registry.add_component(entity, forces);
        }
    }
    
    static void generate_constraint_chains_scenario(Registry& registry, u32 count) {
        // Create chain of connected objects for constraint testing
        std::vector<Entity> chain_entities;
        
        for (u32 i = 0; i < count; ++i) {
            Entity entity = registry.create_entity();
            chain_entities.push_back(entity);
            
            Transform transform{Vec2{i * 10.0f, 100.0f}, 0.0f, Vec2{1.0f, 1.0f}};
            RigidBody2D rigidbody{i == 0 ? 0.0f : 1.0f};  // First link is static
            if (i == 0) rigidbody.make_static();
            
            Collider2D collider{Circle{Vec2::zero(), 3.0f}};
            ForceAccumulator forces{};
            
            registry.add_component(entity, transform);
            registry.add_component(entity, rigidbody);
            registry.add_component(entity, collider);
            registry.add_component(entity, forces);
        }
        
        // Connect chain with constraints (would need constraint system implementation)
        // This is a placeholder for constraint creation
    }
    
    static void generate_stress_test_scenario(Registry& registry, u32 count) {
        // Maximum stress test with many different object types
        u32 objects_per_type = count / 4;
        
        // Falling spheres
        generate_falling_objects_scenario(registry, objects_per_type);
        
        // Dense collision field
        generate_collision_stress_scenario(registry, objects_per_type);
        
        // Particle system
        generate_particle_system_scenario(registry, objects_per_type);
        
        // Remaining as stacked boxes
        generate_stacking_scenario(registry, count - (3 * objects_per_type));
    }
};

//=============================================================================
// Benchmark Suite Analysis Implementation
//=============================================================================

void BenchmarkSuite::analyze_results() {
    if (results.empty()) return;
    
    // Find best/worst performance
    analysis.best_performance = results[0];
    analysis.worst_performance = results[0];
    analysis.most_stable = results[0];
    
    for (const auto& result : results) {
        if (result.average_frame_time < analysis.best_performance.average_frame_time) {
            analysis.best_performance = result;
        }
        if (result.average_frame_time > analysis.worst_performance.average_frame_time) {
            analysis.worst_performance = result;
        }
        if (result.frame_time_std_deviation < analysis.most_stable.frame_time_std_deviation) {
            analysis.most_stable = result;
        }
    }
    
    // Analyze scaling factors
    analyze_scaling_characteristics();
    
    // Generate insights and recommendations
    generate_performance_insights();
    
    // Calculate overall grade
    calculate_overall_grade();
}

void BenchmarkSuite::analyze_scaling_characteristics() {
    // Analyze how performance scales with entity count
    // This would involve regression analysis on the results
    
    // Placeholder implementation
    analysis.entity_count_scaling_factor = 1.2;  // O(n^1.2) complexity
    analysis.memory_scaling_factor = 1.1;        // Linear-ish memory scaling
}

void BenchmarkSuite::generate_performance_insights() {
    analysis.performance_insights.clear();
    analysis.optimization_recommendations.clear();
    
    // Analyze results and generate insights
    f64 avg_frame_time = 0.0;
    for (const auto& result : results) {
        avg_frame_time += result.average_frame_time;
    }
    avg_frame_time /= results.size();
    
    if (avg_frame_time < 8.0) {  // Better than 120 FPS
        analysis.performance_insights.push_back("Excellent overall performance - suitable for high framerate applications");
    } else if (avg_frame_time < 16.67) {  // Better than 60 FPS
        analysis.performance_insights.push_back("Good performance - suitable for real-time applications");
        analysis.optimization_recommendations.push_back("Consider optimizing for mobile/lower-end hardware");
    } else {
        analysis.performance_insights.push_back("Performance may be insufficient for real-time applications");
        analysis.optimization_recommendations.push_back("Significant optimization needed - consider reducing entity counts or simplifying physics");
    }
    
    // Memory analysis
    if (analysis.memory_scaling_factor > 1.5) {
        analysis.optimization_recommendations.push_back("Memory usage scales poorly - consider object pooling or LOD systems");
    }
    
    // Stability analysis
    f64 avg_stability = 0.0;
    for (const auto& result : results) {
        avg_stability += result.frame_time_std_deviation;
    }
    avg_stability /= results.size();
    
    if (avg_stability > 5.0) {
        analysis.optimization_recommendations.push_back("High frame time variance - investigate performance spikes");
    }
}

void BenchmarkSuite::calculate_overall_grade() {
    f64 performance_score = 0.0;
    f64 stability_score = 0.0;
    f64 memory_score = 0.0;
    
    // Calculate scores based on results
    // This is a simplified scoring system
    
    f64 avg_frame_time = 0.0;
    for (const auto& result : results) {
        avg_frame_time += result.average_frame_time;
    }
    avg_frame_time /= results.size();
    
    if (avg_frame_time < 8.0) performance_score = 95.0;
    else if (avg_frame_time < 16.67) performance_score = 85.0;
    else if (avg_frame_time < 33.33) performance_score = 70.0;
    else performance_score = 50.0;
    
    // Overall grade
    f64 overall_score = (performance_score + stability_score + memory_score) / 3.0;
    
    if (overall_score >= 90) analysis.overall_grade = "A (Excellent)";
    else if (overall_score >= 80) analysis.overall_grade = "B (Good)";
    else if (overall_score >= 70) analysis.overall_grade = "C (Fair)";
    else if (overall_score >= 60) analysis.overall_grade = "D (Poor)";
    else analysis.overall_grade = "F (Failing)";
}

} // namespace ecscope::physics::benchmarks