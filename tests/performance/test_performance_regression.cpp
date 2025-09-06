#include "../framework/ecscope_test_framework.hpp"
#include <ecscope/ecs_performance_benchmarker.hpp>
#include <ecscope/ecs_performance_regression_tester.hpp>

#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <numeric>

namespace ECScope::Testing {

class PerformanceRegressionTest : public PerformanceTestFixture {
protected:
    void SetUp() override {
        PerformanceTestFixture::SetUp();
        
        // Initialize performance benchmarker
        benchmarker_ = std::make_unique<Performance::ECSPerformanceBenchmarker>(*world_);
        
        // Initialize regression tester
        regression_tester_ = std::make_unique<Performance::ECSPerformanceRegressionTester>();
        regression_tester_->set_baseline_directory("test_baselines");
        regression_tester_->set_regression_threshold(0.15f); // 15% regression threshold
        
        // Create baseline directory if it doesn't exist
        std::filesystem::create_directories("test_baselines");
        
        // Set up test configuration
        Performance::BenchmarkConfiguration config;
        config.entity_counts = {1000, 5000, 10000, 25000};
        config.iterations_per_test = 100;
        config.warmup_iterations = 10;
        config.enable_memory_tracking = true;
        config.enable_cache_analysis = true;
        
        benchmarker_->configure(config);
    }

    void TearDown() override {
        regression_tester_.reset();
        benchmarker_.reset();
        
        PerformanceTestFixture::TearDown();
    }

    // Helper to create performance baseline
    void create_baseline(const std::string& test_name) {
        auto results = benchmarker_->run_comprehensive_benchmark();
        regression_tester_->save_baseline(test_name, results);
    }
    
    // Helper to load baseline results
    Performance::BenchmarkResults load_baseline(const std::string& test_name) {
        return regression_tester_->load_baseline(test_name);
    }

    // Generate performance report
    void generate_performance_report(const std::string& test_name, 
                                   const Performance::BenchmarkResults& baseline,
                                   const Performance::BenchmarkResults& current) {
        std::ofstream report("performance_report_" + test_name + ".html");
        
        report << "<!DOCTYPE html>\n<html>\n<head>\n";
        report << "<title>ECScope Performance Report - " << test_name << "</title>\n";
        report << "<style>\n";
        report << "body { font-family: Arial, sans-serif; margin: 20px; }\n";
        report << "table { border-collapse: collapse; width: 100%; }\n";
        report << "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n";
        report << "th { background-color: #f2f2f2; }\n";
        report << ".regression { background-color: #ffcccc; }\n";
        report << ".improvement { background-color: #ccffcc; }\n";
        report << ".neutral { background-color: #ffffff; }\n";
        report << "</style>\n</head>\n<body>\n";
        
        report << "<h1>ECScope Performance Report: " << test_name << "</h1>\n";
        report << "<h2>Test Configuration</h2>\n";
        report << "<p>Timestamp: " << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) << "</p>\n";
        
        // Summary table
        report << "<h2>Performance Summary</h2>\n";
        report << "<table>\n<tr><th>Metric</th><th>Baseline</th><th>Current</th><th>Change (%)</th><th>Status</th></tr>\n";
        
        for (const auto& [metric, baseline_value] : baseline.metrics) {
            auto current_it = current.metrics.find(metric);
            if (current_it != current.metrics.end()) {
                double current_value = current_it->second;
                double change_percent = ((current_value - baseline_value) / baseline_value) * 100.0;
                
                std::string status_class = "neutral";
                std::string status_text = "OK";
                
                if (std::abs(change_percent) > 15.0) {
                    if (change_percent > 0) {
                        status_class = "regression";
                        status_text = "REGRESSION";
                    } else {
                        status_class = "improvement";
                        status_text = "IMPROVEMENT";
                    }
                }
                
                report << "<tr class=\"" << status_class << "\">";
                report << "<td>" << metric << "</td>";
                report << "<td>" << baseline_value << "</td>";
                report << "<td>" << current_value << "</td>";
                report << "<td>" << std::fixed << std::setprecision(2) << change_percent << "%</td>";
                report << "<td>" << status_text << "</td>";
                report << "</tr>\n";
            }
        }
        
        report << "</table>\n";
        report << "</body>\n</html>\n";
        report.close();
    }

protected:
    std::unique_ptr<Performance::ECSPerformanceBenchmarker> benchmarker_;
    std::unique_ptr<Performance::ECSPerformanceRegressionTester> regression_tester_;
};

// =============================================================================
// ECS Performance Benchmarks
// =============================================================================

TEST_F(PerformanceRegressionTest, ECSEntityCreationPerformance) {
    const std::string test_name = "ECSEntityCreation";
    
    // Check if baseline exists, create if not
    if (!regression_tester_->baseline_exists(test_name)) {
        std::cout << "Creating baseline for " << test_name << std::endl;
        create_baseline(test_name);
        return; // Skip regression test on baseline creation
    }
    
    auto baseline = load_baseline(test_name);
    
    // Run current benchmark
    Performance::BenchmarkResults current_results;
    
    // Entity creation benchmark
    std::vector<int> entity_counts = {1000, 5000, 10000, 25000, 50000};
    
    for (int count : entity_counts) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        std::vector<Entity> entities;
        entities.reserve(count);
        
        for (int i = 0; i < count; ++i) {
            entities.push_back(world_->create_entity());
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(end_time - start_time);
        
        std::string metric_name = "EntityCreation_" + std::to_string(count) + "_ms";
        current_results.metrics[metric_name] = duration.count();
        
        // Calculate entities per second
        double entities_per_second = count / (duration.count() / 1000.0);
        current_results.metrics["EntityCreation_" + std::to_string(count) + "_eps"] = entities_per_second;
        
        // Clean up for next iteration
        for (auto entity : entities) {
            world_->destroy_entity(entity);
        }
    }
    
    // Check for regressions
    auto regression_results = regression_tester_->detect_regressions(baseline, current_results);
    
    // Generate detailed report
    generate_performance_report(test_name, baseline, current_results);
    
    // Assert no significant regressions
    for (const auto& regression : regression_results.regressions) {
        if (regression.severity == Performance::RegressionSeverity::Critical) {
            FAIL() << "Critical performance regression detected in " << regression.metric_name
                   << ": " << regression.percentage_change << "% slower";
        }
    }
    
    // Log performance summary
    std::cout << "Performance Summary for " << test_name << ":\n";
    for (const auto& regression : regression_results.regressions) {
        std::cout << "  " << regression.metric_name << ": " 
                  << regression.percentage_change << "% change\n";
    }
    
    for (const auto& improvement : regression_results.improvements) {
        std::cout << "  " << improvement.metric_name << ": " 
                  << improvement.percentage_change << "% faster (improvement)\n";
    }
}

TEST_F(PerformanceRegressionTest, ECSComponentOperationsPerformance) {
    const std::string test_name = "ECSComponentOperations";
    
    if (!regression_tester_->baseline_exists(test_name)) {
        create_baseline(test_name);
        return;
    }
    
    auto baseline = load_baseline(test_name);
    Performance::BenchmarkResults current_results;
    
    // Create test entities
    constexpr int entity_count = 10000;
    std::vector<Entity> entities;
    entities.reserve(entity_count);
    
    for (int i = 0; i < entity_count; ++i) {
        entities.push_back(world_->create_entity());
    }
    
    // Component addition benchmark
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (auto entity : entities) {
            world_->add_component<TestPosition>(entity, TestPosition{1.0f, 2.0f, 3.0f});
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(end_time - start_time);
        
        current_results.metrics["ComponentAddition_ms"] = duration.count();
        current_results.metrics["ComponentAddition_ops_per_ms"] = entity_count / duration.count();
    }
    
    // Component access benchmark
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        volatile float sum = 0.0f;
        for (auto entity : entities) {
            auto& pos = world_->get_component<TestPosition>(entity);
            sum += pos.x + pos.y + pos.z;
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(end_time - start_time);
        
        current_results.metrics["ComponentAccess_ms"] = duration.count();
        current_results.metrics["ComponentAccess_ops_per_ms"] = entity_count / duration.count();
    }
    
    // Component modification benchmark
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (auto entity : entities) {
            auto& pos = world_->get_component<TestPosition>(entity);
            pos.x += 1.0f;
            pos.y += 2.0f;
            pos.z += 3.0f;
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(end_time - start_time);
        
        current_results.metrics["ComponentModification_ms"] = duration.count();
        current_results.metrics["ComponentModification_ops_per_ms"] = entity_count / duration.count();
    }
    
    // Component removal benchmark
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (auto entity : entities) {
            world_->remove_component<TestPosition>(entity);
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(end_time - start_time);
        
        current_results.metrics["ComponentRemoval_ms"] = duration.count();
        current_results.metrics["ComponentRemoval_ops_per_ms"] = entity_count / duration.count();
    }
    
    // Check for regressions
    auto regression_results = regression_tester_->detect_regressions(baseline, current_results);
    generate_performance_report(test_name, baseline, current_results);
    
    // Assert no critical regressions
    for (const auto& regression : regression_results.regressions) {
        if (regression.severity == Performance::RegressionSeverity::Critical) {
            FAIL() << "Critical performance regression in " << regression.metric_name;
        }
    }
}

TEST_F(PerformanceRegressionTest, ECSQueryPerformance) {
    const std::string test_name = "ECSQuery";
    
    if (!regression_tester_->baseline_exists(test_name)) {
        create_baseline(test_name);
        return;
    }
    
    auto baseline = load_baseline(test_name);
    Performance::BenchmarkResults current_results;
    
    // Create diverse entity set
    constexpr int entity_count = 50000;
    EntityFactory factory(*world_);
    
    // Create entities with different component combinations
    std::vector<Entity> positioned_entities;
    std::vector<Entity> moving_entities;
    std::vector<Entity> full_entities;
    
    for (int i = 0; i < entity_count / 3; ++i) {
        positioned_entities.push_back(factory.create_positioned());
        moving_entities.push_back(factory.create_moving());
        full_entities.push_back(factory.create_full_entity());
    }
    
    // Single component query benchmark
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        size_t count = 0;
        world_->each<TestPosition>([&count](Entity entity, TestPosition& pos) {
            count++;
            pos.x += 0.001f; // Minimal work to prevent optimization
        });
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(end_time - start_time);
        
        current_results.metrics["SingleComponentQuery_ms"] = duration.count();
        current_results.metrics["SingleComponentQuery_entities_per_ms"] = count / duration.count();
        
        EXPECT_EQ(count, entity_count); // Should find all entities with TestPosition
    }
    
    // Multi-component query benchmark
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        size_t count = 0;
        world_->each<TestPosition, TestVelocity>([&count](Entity entity, TestPosition& pos, TestVelocity& vel) {
            count++;
            pos.x += vel.vx * 0.001f; // Simple movement simulation
            pos.y += vel.vy * 0.001f;
            pos.z += vel.vz * 0.001f;
        });
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(end_time - start_time);
        
        current_results.metrics["MultiComponentQuery_ms"] = duration.count();
        current_results.metrics["MultiComponentQuery_entities_per_ms"] = count / duration.count();
    }
    
    // Complex query with filtering
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        size_t count = 0;
        world_->each<TestPosition, TestVelocity, TestHealth>([&count](Entity entity, TestPosition& pos, TestVelocity& vel, TestHealth& health) {
            if (health.health > 50) { // Filter condition
                count++;
                pos.x += vel.vx * 0.001f;
                pos.y += vel.vy * 0.001f;
                pos.z += vel.vz * 0.001f;
                health.health -= 1; // Damage over time
            }
        });
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(end_time - start_time);
        
        current_results.metrics["ComplexQuery_ms"] = duration.count();
        current_results.metrics["ComplexQuery_entities_per_ms"] = count / duration.count();
    }
    
    // Check for regressions
    auto regression_results = regression_tester_->detect_regressions(baseline, current_results);
    generate_performance_report(test_name, baseline, current_results);
    
    // Log query performance metrics
    std::cout << "Query Performance Results:\n";
    for (const auto& [metric, value] : current_results.metrics) {
        std::cout << "  " << metric << ": " << value << "\n";
    }
    
    // Assert no critical regressions
    for (const auto& regression : regression_results.regressions) {
        if (regression.severity == Performance::RegressionSeverity::Critical) {
            FAIL() << "Critical query performance regression: " << regression.metric_name;
        }
    }
}

// =============================================================================
// Memory Performance Tests
// =============================================================================

TEST_F(PerformanceRegressionTest, MemoryAllocationPerformance) {
    const std::string test_name = "MemoryAllocation";
    
    if (!regression_tester_->baseline_exists(test_name)) {
        create_baseline(test_name);
        return;
    }
    
    auto baseline = load_baseline(test_name);
    Performance::BenchmarkResults current_results;
    
    // Arena allocator benchmark
    {
        constexpr size_t arena_size = 16 * 1024 * 1024; // 16MB
        constexpr size_t allocation_count = 100000;
        
        Arena arena(arena_size);
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < allocation_count; ++i) {
            void* ptr = arena.allocate(64);
            EXPECT_NE(ptr, nullptr);
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(end_time - start_time);
        
        current_results.metrics["ArenaAllocation_ms"] = duration.count();
        current_results.metrics["ArenaAllocation_allocs_per_ms"] = allocation_count / duration.count();
    }
    
    // Pool allocator benchmark
    {
        constexpr size_t block_count = 10000;
        constexpr size_t iterations = 1000;
        
        Pool pool(128, block_count);
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (size_t iter = 0; iter < iterations; ++iter) {
            std::vector<void*> ptrs;
            ptrs.reserve(block_count);
            
            // Allocate all blocks
            for (size_t i = 0; i < block_count; ++i) {
                void* ptr = pool.allocate();
                if (ptr) ptrs.push_back(ptr);
            }
            
            // Deallocate all blocks
            for (void* ptr : ptrs) {
                pool.deallocate(ptr);
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(end_time - start_time);
        
        current_results.metrics["PoolAllocation_ms"] = duration.count();
        current_results.metrics["PoolAllocation_cycles_per_ms"] = iterations / duration.count();
    }
    
    // Standard allocator comparison
    {
        constexpr size_t allocation_count = 10000;
        constexpr size_t allocation_size = 128;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        std::vector<void*> ptrs;
        ptrs.reserve(allocation_count);
        
        for (size_t i = 0; i < allocation_count; ++i) {
            ptrs.push_back(std::malloc(allocation_size));
        }
        
        for (void* ptr : ptrs) {
            std::free(ptr);
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(end_time - start_time);
        
        current_results.metrics["StandardAllocation_ms"] = duration.count();
        current_results.metrics["StandardAllocation_allocs_per_ms"] = allocation_count / duration.count();
    }
    
    // Memory bandwidth test
    {
        constexpr size_t buffer_size = 32 * 1024 * 1024; // 32MB
        constexpr size_t iterations = 10;
        
        std::vector<uint8_t> source(buffer_size, 0xAA);
        std::vector<uint8_t> destination(buffer_size);
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (size_t iter = 0; iter < iterations; ++iter) {
            std::memcpy(destination.data(), source.data(), buffer_size);
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(end_time - start_time);
        
        double bandwidth_mb_per_sec = (buffer_size * iterations) / (duration.count() / 1000.0) / (1024 * 1024);
        current_results.metrics["MemoryBandwidth_MB_per_sec"] = bandwidth_mb_per_sec;
    }
    
    // Check for regressions
    auto regression_results = regression_tester_->detect_regressions(baseline, current_results);
    generate_performance_report(test_name, baseline, current_results);
    
    // Assert no critical regressions
    for (const auto& regression : regression_results.regressions) {
        if (regression.severity == Performance::RegressionSeverity::Critical) {
            FAIL() << "Critical memory performance regression: " << regression.metric_name;
        }
    }
}

// =============================================================================
// Physics Performance Tests (if enabled)
// =============================================================================

#ifdef ECSCOPE_ENABLE_PHYSICS
TEST_F(PerformanceRegressionTest, PhysicsPerformance) {
    const std::string test_name = "Physics";
    
    if (!regression_tester_->baseline_exists(test_name)) {
        create_baseline(test_name);
        return;
    }
    
    auto baseline = load_baseline(test_name);
    Performance::BenchmarkResults current_results;
    
    Physics3D::World physics_world;
    physics_world.set_gravity(Vec3(0, -9.81f, 0));
    
    // Collision detection benchmark
    {
        constexpr int sphere_count = 1000;
        std::vector<Physics3D::Sphere> spheres;
        
        // Create random spheres
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> pos_dist(-20.0f, 20.0f);
        std::uniform_real_distribution<float> radius_dist(0.1f, 2.0f);
        
        for (int i = 0; i < sphere_count; ++i) {
            Vec3 pos(pos_dist(gen), pos_dist(gen), pos_dist(gen));
            spheres.emplace_back(pos, radius_dist(gen));
        }
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        int collision_count = 0;
        Physics3D::CollisionInfo collision;
        
        for (int i = 0; i < sphere_count; ++i) {
            for (int j = i + 1; j < sphere_count; ++j) {
                if (Physics3D::test_sphere_sphere(spheres[i], spheres[j], collision)) {
                    collision_count++;
                }
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(end_time - start_time);
        
        current_results.metrics["CollisionDetection_ms"] = duration.count();
        current_results.metrics["CollisionTests_per_ms"] = (sphere_count * (sphere_count - 1) / 2) / duration.count();
        current_results.metrics["CollisionsFound"] = collision_count;
    }
    
    // Rigid body simulation benchmark
    {
        constexpr int body_count = 500;
        constexpr int simulation_steps = 60; // 1 second at 60 FPS
        
        // Create falling rigid bodies
        for (int i = 0; i < body_count; ++i) {
            auto entity = world_->create_entity();
            
            Vec3 position(
                (i % 20) * 2.0f - 20.0f,  // Grid X
                20.0f + (i / 400) * 2.0f, // Height layers
                ((i / 20) % 20) * 2.0f - 20.0f // Grid Z
            );
            
            world_->add_component<Transform3D>(entity, position);
            
            Physics3D::RigidBody3D rigidbody;
            rigidbody.mass = 1.0f;
            rigidbody.velocity = Vec3(0, 0, 0);
            world_->add_component<Physics3D::RigidBody3D>(entity, rigidbody);
            
            world_->add_component<Physics3D::SphereCollider>(entity, 0.5f);
        }
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        float dt = 1.0f / 60.0f;
        for (int step = 0; step < simulation_steps; ++step) {
            physics_world.step(dt);
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(end_time - start_time);
        
        current_results.metrics["PhysicsSimulation_ms"] = duration.count();
        current_results.metrics["SimulationSteps_per_ms"] = simulation_steps / duration.count();
        current_results.metrics["Bodies_simulated"] = body_count;
    }
    
    // Check for regressions
    auto regression_results = regression_tester_->detect_regressions(baseline, current_results);
    generate_performance_report(test_name, baseline, current_results);
    
    // Assert no critical regressions
    for (const auto& regression : regression_results.regressions) {
        if (regression.severity == Performance::RegressionSeverity::Critical) {
            FAIL() << "Critical physics performance regression: " << regression.metric_name;
        }
    }
}
#endif

// =============================================================================
// Stress Tests with Performance Monitoring
// =============================================================================

TEST_F(PerformanceRegressionTest, StressTestWithMonitoring) {
    const std::string test_name = "StressTest";
    
    if (!regression_tester_->baseline_exists(test_name)) {
        create_baseline(test_name);
        return;
    }
    
    auto baseline = load_baseline(test_name);
    Performance::BenchmarkResults current_results;
    
    // Massive entity stress test
    constexpr int massive_entity_count = 100000;
    constexpr int stress_duration_frames = 3600; // 60 seconds at 60 FPS
    
    EntityFactory factory(*world_);
    std::vector<Entity> entities;
    entities.reserve(massive_entity_count);
    
    // Entity creation phase
    auto creation_start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < massive_entity_count; ++i) {
        entities.push_back(factory.create_full_entity());
    }
    
    auto creation_end = std::chrono::high_resolution_clock::now();
    auto creation_duration = std::chrono::duration<double, std::milli>(creation_end - creation_start);
    
    current_results.metrics["MassiveEntityCreation_ms"] = creation_duration.count();
    
    // Sustained operation phase
    std::vector<double> frame_times;
    frame_times.reserve(stress_duration_frames);
    
    double total_query_time = 0.0;
    int total_entities_processed = 0;
    
    for (int frame = 0; frame < stress_duration_frames; ++frame) {
        auto frame_start = std::chrono::high_resolution_clock::now();
        
        // Simulate game loop operations
        int entities_processed = 0;
        
        // Movement update
        world_->each<TestPosition, TestVelocity>([&](Entity entity, TestPosition& pos, TestVelocity& vel) {
            pos.x += vel.vx * 0.016f; // 60 FPS timestep
            pos.y += vel.vy * 0.016f;
            pos.z += vel.vz * 0.016f;
            entities_processed++;
        });
        
        // Health update
        world_->each<TestHealth>([&](Entity entity, TestHealth& health) {
            if (health.health > 0) {
                health.health = std::min(health.health + 1, health.max_health); // Regeneration
            }
            entities_processed++;
        });
        
        auto frame_end = std::chrono::high_resolution_clock::now();
        auto frame_duration = std::chrono::duration<double, std::milli>(frame_end - frame_start);
        
        frame_times.push_back(frame_duration.count());
        total_query_time += frame_duration.count();
        total_entities_processed += entities_processed;
        
        // Sample frame times every 60 frames (1 second)
        if (frame % 60 == 59) {
            double avg_frame_time = std::accumulate(frame_times.end() - 60, frame_times.end(), 0.0) / 60.0;
            std::string metric_name = "AvgFrameTime_" + std::to_string(frame / 60) + "s_ms";
            current_results.metrics[metric_name] = avg_frame_time;
        }
    }
    
    // Calculate overall statistics
    double min_frame_time = *std::min_element(frame_times.begin(), frame_times.end());
    double max_frame_time = *std::max_element(frame_times.begin(), frame_times.end());
    double avg_frame_time = total_query_time / stress_duration_frames;
    
    current_results.metrics["StressTest_MinFrameTime_ms"] = min_frame_time;
    current_results.metrics["StressTest_MaxFrameTime_ms"] = max_frame_time;
    current_results.metrics["StressTest_AvgFrameTime_ms"] = avg_frame_time;
    current_results.metrics["StressTest_TotalEntitiesProcessed"] = total_entities_processed;
    current_results.metrics["StressTest_EntitiesPerFrame"] = total_entities_processed / static_cast<double>(stress_duration_frames);
    
    // Calculate frame rate statistics
    current_results.metrics["StressTest_MinFPS"] = 1000.0 / max_frame_time;
    current_results.metrics["StressTest_MaxFPS"] = 1000.0 / min_frame_time;
    current_results.metrics["StressTest_AvgFPS"] = 1000.0 / avg_frame_time;
    
    // Memory usage at end of test
    size_t final_memory_usage = memory_tracker_->get_current_usage();
    current_results.metrics["StressTest_FinalMemoryUsage_bytes"] = static_cast<double>(final_memory_usage);
    current_results.metrics["StressTest_MemoryPerEntity_bytes"] = static_cast<double>(final_memory_usage) / massive_entity_count;
    
    // Check for regressions
    auto regression_results = regression_tester_->detect_regressions(baseline, current_results);
    generate_performance_report(test_name, baseline, current_results);
    
    // Log stress test results
    std::cout << "Stress Test Results:\n";
    std::cout << "  Entities: " << massive_entity_count << "\n";
    std::cout << "  Avg Frame Time: " << avg_frame_time << " ms\n";
    std::cout << "  Avg FPS: " << (1000.0 / avg_frame_time) << "\n";
    std::cout << "  Memory per Entity: " << (final_memory_usage / massive_entity_count) << " bytes\n";
    
    // Assert performance requirements
    EXPECT_LT(avg_frame_time, 20.0); // Must maintain >50 FPS average
    EXPECT_LT(max_frame_time, 50.0); // No frame should take >50ms
    
    // Assert no critical regressions
    for (const auto& regression : regression_results.regressions) {
        if (regression.severity == Performance::RegressionSeverity::Critical) {
            FAIL() << "Critical stress test regression: " << regression.metric_name;
        }
    }
}

// =============================================================================
// Performance Trend Analysis
// =============================================================================

TEST_F(PerformanceRegressionTest, PerformanceTrendAnalysis) {
    // This test analyzes performance trends over multiple runs
    const std::string test_name = "TrendAnalysis";
    
    // Run a simplified benchmark multiple times to establish trend
    constexpr int trend_samples = 5;
    constexpr int entities_per_sample = 10000;
    
    std::vector<Performance::BenchmarkResults> trend_results;
    
    for (int sample = 0; sample < trend_samples; ++sample) {
        Performance::BenchmarkResults sample_results;
        
        // Create entities for this sample
        EntityFactory factory(*world_);
        std::vector<Entity> entities;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < entities_per_sample; ++i) {
            entities.push_back(factory.create_moving());
        }
        
        // Run queries
        size_t query_count = 0;
        world_->each<TestPosition, TestVelocity>([&](Entity entity, TestPosition& pos, TestVelocity& vel) {
            pos.x += vel.vx * 0.016f;
            pos.y += vel.vy * 0.016f;
            pos.z += vel.vz * 0.016f;
            query_count++;
        });
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(end_time - start_time);
        
        sample_results.metrics["SampleTime_ms"] = duration.count();
        sample_results.metrics["EntitiesProcessed"] = static_cast<double>(query_count);
        sample_results.metrics["ProcessingRate_entities_per_ms"] = query_count / duration.count();
        
        trend_results.push_back(sample_results);
        
        // Clean up for next sample
        for (auto entity : entities) {
            world_->destroy_entity(entity);
        }
    }
    
    // Analyze trends
    std::vector<double> processing_rates;
    for (const auto& result : trend_results) {
        auto rate_it = result.metrics.find("ProcessingRate_entities_per_ms");
        if (rate_it != result.metrics.end()) {
            processing_rates.push_back(rate_it->second);
        }
    }
    
    // Calculate trend statistics
    double mean_rate = std::accumulate(processing_rates.begin(), processing_rates.end(), 0.0) / processing_rates.size();
    
    double variance = 0.0;
    for (double rate : processing_rates) {
        variance += (rate - mean_rate) * (rate - mean_rate);
    }
    variance /= processing_rates.size();
    double std_dev = std::sqrt(variance);
    
    double coefficient_of_variation = std_dev / mean_rate;
    
    // Log trend analysis
    std::cout << "Performance Trend Analysis:\n";
    std::cout << "  Mean Processing Rate: " << mean_rate << " entities/ms\n";
    std::cout << "  Standard Deviation: " << std_dev << "\n";
    std::cout << "  Coefficient of Variation: " << coefficient_of_variation << "\n";
    
    // Performance should be stable (low variance)
    EXPECT_LT(coefficient_of_variation, 0.1); // Less than 10% variation
    
    // Calculate linear trend
    double sum_x = 0.0, sum_y = 0.0, sum_xy = 0.0, sum_xx = 0.0;
    for (size_t i = 0; i < processing_rates.size(); ++i) {
        double x = static_cast<double>(i);
        double y = processing_rates[i];
        sum_x += x;
        sum_y += y;
        sum_xy += x * y;
        sum_xx += x * x;
    }
    
    double n = static_cast<double>(processing_rates.size());
    double slope = (n * sum_xy - sum_x * sum_y) / (n * sum_xx - sum_x * sum_x);
    double intercept = (sum_y - slope * sum_x) / n;
    
    std::cout << "  Trend Slope: " << slope << " entities/ms per sample\n";
    std::cout << "  Trend Intercept: " << intercept << " entities/ms\n";
    
    // Performance should not degrade significantly over samples
    // (slope should not be significantly negative)
    EXPECT_GT(slope, -mean_rate * 0.05); // Less than 5% degradation per sample
}

} // namespace ECScope::Testing