#include "../framework/ecscope_test_framework.hpp"
#include <ecscope/ecs_performance_benchmarker.hpp>
#include <ecscope/ecs_performance_regression_tester.hpp>
#include <ecscope/ecs_performance_visualizer.hpp>
#include <ecscope/memory_benchmark_suite.hpp>
#include <ecscope/bandwidth_analyzer.hpp>

#include <fstream>
#include <filesystem>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <cmath>

namespace ECScope::Testing {

// =============================================================================
// Performance Regression Test Fixture
// =============================================================================

class PerformanceRegressionTest : public PerformanceTestFixture {
protected:
    void SetUp() override {
        PerformanceTestFixture::SetUp();
        
        // Initialize performance testing systems
        benchmarker_ = std::make_unique<Performance::Benchmarker>();
        regression_tester_ = std::make_unique<Performance::RegressionTester>();
        visualizer_ = std::make_unique<Performance::Visualizer>();
        memory_suite_ = std::make_unique<Memory::BenchmarkSuite>();
        bandwidth_analyzer_ = std::make_unique<Cache::BandwidthAnalyzer>();
        
        // Set up performance baselines directory
        baseline_dir_ = "test_baselines";
        std::filesystem::create_directories(baseline_dir_);
        
        // Configure regression detection thresholds
        performance_threshold_ = 0.15f; // 15% regression threshold
        memory_threshold_ = 0.20f;      // 20% memory regression threshold
        
        // Set up test parameters
        warmup_iterations_ = 10;
        benchmark_iterations_ = 100;
        large_scale_entities_ = 100000;
        
        // Initialize random seed for reproducible tests
        rng_.seed(42);
    }

    void TearDown() override {
        bandwidth_analyzer_.reset();
        memory_suite_.reset();
        visualizer_.reset();
        regression_tester_.reset();
        benchmarker_.reset();
        PerformanceTestFixture::TearDown();
    }

    // Helper to save performance baselines
    void save_baseline(const std::string& test_name, const Performance::BenchmarkResult& result) {
        std::string baseline_file = baseline_dir_ + "/" + test_name + "_baseline.json";
        std::ofstream file(baseline_file);
        
        if (file.is_open()) {
            file << "{\n";
            file << "  \"test_name\": \"" << test_name << "\",\n";
            file << "  \"average_time_ns\": " << result.average_time_ns << ",\n";
            file << "  \"min_time_ns\": " << result.min_time_ns << ",\n";
            file << "  \"max_time_ns\": " << result.max_time_ns << ",\n";
            file << "  \"std_deviation_ns\": " << result.std_deviation_ns << ",\n";
            file << "  \"iterations\": " << result.iterations << ",\n";
            file << "  \"memory_usage_bytes\": " << result.memory_usage_bytes << ",\n";
            file << "  \"cache_misses\": " << result.cache_misses << ",\n";
            file << "  \"timestamp\": \"" << get_current_timestamp() << "\"\n";
            file << "}\n";
        }
    }

    // Helper to load performance baseline
    Performance::BenchmarkResult load_baseline(const std::string& test_name) {
        std::string baseline_file = baseline_dir_ + "/" + test_name + "_baseline.json";
        Performance::BenchmarkResult baseline;
        
        if (std::filesystem::exists(baseline_file)) {
            // Simple JSON parsing for baseline data
            std::ifstream file(baseline_file);
            std::string line;
            while (std::getline(file, line)) {
                if (line.find("average_time_ns") != std::string::npos) {
                    baseline.average_time_ns = extract_number_from_json_line(line);
                }
                // Add more fields as needed...
            }
        }
        
        return baseline;
    }

    std::string get_current_timestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        return std::to_string(time_t);
    }

    double extract_number_from_json_line(const std::string& line) {
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            size_t start = colon_pos + 1;
            size_t end = line.find(',', start);
            if (end == std::string::npos) end = line.find('}', start);
            
            std::string number_str = line.substr(start, end - start);
            // Remove whitespace
            number_str.erase(std::remove_if(number_str.begin(), number_str.end(), ::isspace), number_str.end());
            
            return std::stod(number_str);
        }
        return 0.0;
    }

protected:
    std::unique_ptr<Performance::Benchmarker> benchmarker_;
    std::unique_ptr<Performance::RegressionTester> regression_tester_;
    std::unique_ptr<Performance::Visualizer> visualizer_;
    std::unique_ptr<Memory::BenchmarkSuite> memory_suite_;
    std::unique_ptr<Cache::BandwidthAnalyzer> bandwidth_analyzer_;
    
    std::string baseline_dir_;
    float performance_threshold_;
    float memory_threshold_;
    
    int warmup_iterations_;
    int benchmark_iterations_;
    size_t large_scale_entities_;
    
    std::mt19937 rng_;
};

// =============================================================================
// ECS Performance Regression Tests
// =============================================================================

TEST_F(PerformanceRegressionTest, EntityCreationPerformanceRegression) {
    const std::string test_name = "entity_creation";
    
    // Benchmark current performance
    auto benchmark_func = [&]() {
        constexpr size_t entity_count = 10000;
        std::vector<Entity> entities;
        entities.reserve(entity_count);
        
        for (size_t i = 0; i < entity_count; ++i) {
            entities.push_back(world_->create_entity());
        }
        
        // Clean up
        for (auto entity : entities) {
            world_->destroy_entity(entity);
        }
    };
    
    auto result = benchmarker_->benchmark(test_name, benchmark_func, benchmark_iterations_);
    
    // Load baseline and compare
    auto baseline = load_baseline(test_name);
    
    if (baseline.average_time_ns > 0) {
        float regression_ratio = static_cast<float>(result.average_time_ns) / baseline.average_time_ns;
        
        std::cout << test_name << " performance:\n";
        std::cout << "  Current: " << result.average_time_ns << " ns avg\n";
        std::cout << "  Baseline: " << baseline.average_time_ns << " ns avg\n";
        std::cout << "  Ratio: " << regression_ratio << "x\n";
        
        EXPECT_LT(regression_ratio, 1.0f + performance_threshold_) 
            << "Performance regression detected: " << ((regression_ratio - 1.0f) * 100.0f) << "% slower";
        
        if (regression_ratio < 0.95f) {
            std::cout << "Performance improvement detected: " << ((1.0f - regression_ratio) * 100.0f) << "% faster\n";
        }
    } else {
        std::cout << "No baseline found for " << test_name << ". Creating new baseline.\n";
    }
    
    // Save current result as new baseline
    save_baseline(test_name, result);
    
    // Validate basic performance requirements
    EXPECT_LT(result.average_time_ns, 100000000) << "Entity creation should be < 100ms for 10k entities";
}

TEST_F(PerformanceRegressionTest, ComponentQueryPerformanceRegression) {
    const std::string test_name = "component_query";
    
    // Set up test entities
    constexpr size_t entity_count = 50000;
    auto entities = factory_->create_many(entity_count, true);
    
    auto benchmark_func = [&]() {
        size_t processed = 0;
        world_->each<TestPosition, TestVelocity>([&](Entity entity, TestPosition& pos, TestVelocity& vel) {
            // Simulate typical component processing
            pos.x += vel.vx * 0.016f;
            pos.y += vel.vy * 0.016f;
            pos.z += vel.vz * 0.016f;
            processed++;
        });
        
        // Verify we processed all entities
        volatile size_t result = processed; // Prevent optimization
        (void)result;
    };
    
    auto result = benchmarker_->benchmark(test_name, benchmark_func, benchmark_iterations_);
    
    // Performance regression analysis
    auto baseline = load_baseline(test_name);
    
    if (baseline.average_time_ns > 0) {
        float regression_ratio = static_cast<float>(result.average_time_ns) / baseline.average_time_ns;
        
        std::cout << test_name << " performance:\n";
        std::cout << "  Current: " << result.average_time_ns << " ns avg (" 
                  << (result.average_time_ns / entity_count) << " ns/entity)\n";
        std::cout << "  Baseline: " << baseline.average_time_ns << " ns avg (" 
                  << (baseline.average_time_ns / entity_count) << " ns/entity)\n";
        std::cout << "  Regression ratio: " << regression_ratio << "x\n";
        
        EXPECT_LT(regression_ratio, 1.0f + performance_threshold_) 
            << "Query performance regression: " << ((regression_ratio - 1.0f) * 100.0f) << "% slower";
    }
    
    save_baseline(test_name, result);
    
    // Clean up
    for (auto entity : entities) {
        world_->destroy_entity(entity);
    }
    
    // Performance requirements
    double ns_per_entity = static_cast<double>(result.average_time_ns) / entity_count;
    EXPECT_LT(ns_per_entity, 100.0) << "Query should process entities in < 100ns each";
}

TEST_F(PerformanceRegressionTest, ArchetypeTransitionPerformanceRegression) {
    const std::string test_name = "archetype_transition";
    
    // Create entities for archetype transition testing
    constexpr size_t entity_count = 5000;
    auto entities = factory_->create_many(entity_count, false); // Position only
    
    auto benchmark_func = [&]() {
        // Add velocity components (triggers archetype transition)
        for (auto entity : entities) {
            world_->add_component<TestVelocity>(entity, TestVelocity{1, 1, 1});
        }
        
        // Remove position components (another archetype transition)
        for (auto entity : entities) {
            world_->remove_component<TestPosition>(entity);
        }
        
        // Add position back
        for (auto entity : entities) {
            world_->add_component<TestPosition>(entity, TestPosition{0, 0, 0});
        }
    };
    
    auto result = benchmarker_->benchmark(test_name, benchmark_func, 10); // Fewer iterations for complex test
    
    // Regression analysis
    auto baseline = load_baseline(test_name);
    
    if (baseline.average_time_ns > 0) {
        float regression_ratio = static_cast<float>(result.average_time_ns) / baseline.average_time_ns;
        
        std::cout << test_name << " performance:\n";
        std::cout << "  Current: " << (result.average_time_ns / 1000000.0) << " ms avg\n";
        std::cout << "  Baseline: " << (baseline.average_time_ns / 1000000.0) << " ms avg\n";
        std::cout << "  Regression ratio: " << regression_ratio << "x\n";
        
        EXPECT_LT(regression_ratio, 1.0f + performance_threshold_) 
            << "Archetype transition regression: " << ((regression_ratio - 1.0f) * 100.0f) << "% slower";
    }
    
    save_baseline(test_name, result);
    
    // Clean up
    for (auto entity : entities) {
        world_->destroy_entity(entity);
    }
}

// =============================================================================
// Memory Performance Regression Tests
// =============================================================================

TEST_F(PerformanceRegressionTest, MemoryAllocationPerformanceRegression) {
    const std::string test_name = "memory_allocation";
    
    auto benchmark_func = [&]() {
        constexpr size_t allocation_count = 10000;
        constexpr size_t allocation_size = 1024;
        
        std::vector<void*> allocations;
        allocations.reserve(allocation_count);
        
        // Allocate
        for (size_t i = 0; i < allocation_count; ++i) {
            void* ptr = std::malloc(allocation_size);
            if (ptr) {
                allocations.push_back(ptr);
            }
        }
        
        // Deallocate
        for (void* ptr : allocations) {
            std::free(ptr);
        }
    };
    
    // Get memory usage before benchmark
    size_t memory_before = memory_tracker_->get_current_usage();
    
    auto result = benchmarker_->benchmark(test_name, benchmark_func, benchmark_iterations_);
    
    // Get memory usage after benchmark
    size_t memory_after = memory_tracker_->get_current_usage();
    result.memory_usage_bytes = memory_after - memory_before;
    
    // Regression analysis
    auto baseline = load_baseline(test_name);
    
    if (baseline.average_time_ns > 0) {
        float perf_regression = static_cast<float>(result.average_time_ns) / baseline.average_time_ns;
        float memory_regression = static_cast<float>(result.memory_usage_bytes) / baseline.memory_usage_bytes;
        
        std::cout << test_name << " performance:\n";
        std::cout << "  Time - Current: " << result.average_time_ns << " ns, Baseline: " << baseline.average_time_ns << " ns\n";
        std::cout << "  Memory - Current: " << result.memory_usage_bytes << " bytes, Baseline: " << baseline.memory_usage_bytes << " bytes\n";
        std::cout << "  Performance ratio: " << perf_regression << "x\n";
        std::cout << "  Memory ratio: " << memory_regression << "x\n";
        
        EXPECT_LT(perf_regression, 1.0f + performance_threshold_) << "Allocation performance regression";
        EXPECT_LT(memory_regression, 1.0f + memory_threshold_) << "Memory usage regression";
    }
    
    save_baseline(test_name, result);
}

TEST_F(PerformanceRegressionTest, CachePerformanceRegression) {
    const std::string test_name = "cache_performance";
    
    auto benchmark_func = [&]() {
        constexpr size_t data_size = 1024 * 1024; // 1MB
        constexpr size_t iterations = 1000;
        
        // Sequential access pattern (cache-friendly)
        std::vector<int> data(data_size / sizeof(int));
        
        for (size_t iter = 0; iter < iterations; ++iter) {
            volatile int sum = 0;
            for (size_t i = 0; i < data.size(); ++i) {
                sum += data[i];
            }
            (void)sum; // Prevent optimization
        }
    };
    
    // Measure cache performance
    auto result = benchmarker_->benchmark_with_cache_analysis(test_name, benchmark_func, 10);
    
    // Regression analysis with cache metrics
    auto baseline = load_baseline(test_name);
    
    if (baseline.average_time_ns > 0 && baseline.cache_misses > 0) {
        float perf_regression = static_cast<float>(result.average_time_ns) / baseline.average_time_ns;
        float cache_regression = static_cast<float>(result.cache_misses) / baseline.cache_misses;
        
        std::cout << test_name << " performance:\n";
        std::cout << "  Time regression: " << perf_regression << "x\n";
        std::cout << "  Cache miss regression: " << cache_regression << "x\n";
        
        EXPECT_LT(perf_regression, 1.0f + performance_threshold_) << "Cache performance regression";
        EXPECT_LT(cache_regression, 1.0f + 0.25f) << "Cache miss count regression"; // 25% threshold for cache
    }
    
    save_baseline(test_name, result);
}

// =============================================================================
// Large Scale Performance Tests
// =============================================================================

TEST_F(PerformanceRegressionTest, LargeScaleEntityPerformance) {
    const std::string test_name = "large_scale_entities";
    
    std::cout << "Running large scale test with " << large_scale_entities_ << " entities...\n";
    
    auto benchmark_func = [&]() {
        // Create large number of entities
        std::vector<Entity> entities;
        entities.reserve(large_scale_entities_);
        
        for (size_t i = 0; i < large_scale_entities_; ++i) {
            auto entity = world_->create_entity();
            world_->add_component<TestPosition>(entity, 
                static_cast<float>(i % 1000), 
                static_cast<float>(i / 1000), 0);
            
            if (i % 2 == 0) {
                world_->add_component<TestVelocity>(entity, 1, 1, 1);
            }
            if (i % 3 == 0) {
                world_->add_component<TestHealth>(entity, 100, 100);
            }
            
            entities.push_back(entity);
        }
        
        // Perform queries on large dataset
        size_t position_count = 0;
        world_->each<TestPosition>([&](Entity, TestPosition&) {
            position_count++;
        });
        
        size_t moving_count = 0;
        world_->each<TestPosition, TestVelocity>([&](Entity, TestPosition& pos, TestVelocity& vel) {
            pos.x += vel.vx * 0.016f;
            moving_count++;
        });
        
        size_t health_count = 0;
        world_->each<TestHealth>([&](Entity, TestHealth&) {
            health_count++;
        });
        
        // Verify counts
        EXPECT_EQ(position_count, large_scale_entities_);
        EXPECT_EQ(moving_count, large_scale_entities_ / 2);
        EXPECT_EQ(health_count, large_scale_entities_ / 3);
        
        // Clean up
        for (auto entity : entities) {
            world_->destroy_entity(entity);
        }
    };
    
    auto result = benchmarker_->benchmark(test_name, benchmark_func, 3); // Fewer iterations for large test
    
    // Regression analysis
    auto baseline = load_baseline(test_name);
    
    if (baseline.average_time_ns > 0) {
        float regression_ratio = static_cast<float>(result.average_time_ns) / baseline.average_time_ns;
        
        std::cout << test_name << " performance:\n";
        std::cout << "  Current: " << (result.average_time_ns / 1000000.0) << " ms avg\n";
        std::cout << "  Baseline: " << (baseline.average_time_ns / 1000000.0) << " ms avg\n";
        std::cout << "  Regression ratio: " << regression_ratio << "x\n";
        
        EXPECT_LT(regression_ratio, 1.0f + performance_threshold_) 
            << "Large scale performance regression";
    }
    
    save_baseline(test_name, result);
    
    // Performance requirements for large scale
    double seconds = result.average_time_ns / 1000000000.0;
    EXPECT_LT(seconds, 5.0) << "Large scale test should complete in < 5 seconds";
}

// =============================================================================
// Automated Regression Detection Tests
// =============================================================================

TEST_F(PerformanceRegressionTest, AutomatedRegressionDetection) {
    // This test demonstrates the automated regression detection system
    
    struct TestScenario {
        std::string name;
        std::function<void()> test_func;
        float expected_baseline_ns;
    };
    
    std::vector<TestScenario> scenarios = {
        {
            "simple_loop",
            []() {
                volatile int sum = 0;
                for (int i = 0; i < 10000; ++i) {
                    sum += i;
                }
            },
            1000000.0f // 1ms expected
        },
        {
            "memory_access",
            []() {
                std::vector<int> data(1000);
                volatile int sum = 0;
                for (int val : data) {
                    sum += val;
                }
            },
            500000.0f // 0.5ms expected
        }
    };
    
    for (const auto& scenario : scenarios) {
        auto result = benchmarker_->benchmark(scenario.name, scenario.test_func, 100);
        
        // Check against expected baseline
        float ratio = result.average_time_ns / scenario.expected_baseline_ns;
        
        std::cout << "Automated test '" << scenario.name << "':\n";
        std::cout << "  Expected: " << scenario.expected_baseline_ns << " ns\n";
        std::cout << "  Actual: " << result.average_time_ns << " ns\n";
        std::cout << "  Ratio: " << ratio << "x\n";
        
        // Allow for some variance in performance
        EXPECT_LT(ratio, 3.0f) << "Performance is significantly worse than expected";
        EXPECT_GT(ratio, 0.1f) << "Performance is suspiciously better (possible measurement error)";
        
        // Flag potential regressions
        if (ratio > 1.5f) {
            std::cout << "WARNING: Potential performance regression detected for " << scenario.name << "\n";
        }
        
        if (ratio < 0.8f) {
            std::cout << "INFO: Performance improvement detected for " << scenario.name << "\n";
        }
    }
}

// =============================================================================
// Performance Report Generation
// =============================================================================

TEST_F(PerformanceRegressionTest, GeneratePerformanceReport) {
    // Generate comprehensive performance report
    
    std::string report_file = "performance_report.json";
    std::ofstream report(report_file);
    
    if (report.is_open()) {
        report << "{\n";
        report << "  \"test_suite\": \"ECScope Performance Regression Tests\",\n";
        report << "  \"timestamp\": \"" << get_current_timestamp() << "\",\n";
        report << "  \"system_info\": {\n";
        report << "    \"build_type\": \"" << CMAKE_BUILD_TYPE << "\",\n";
        report << "    \"compiler\": \"" << CMAKE_CXX_COMPILER_ID << "\",\n";
        report << "    \"platform\": \"" << CMAKE_SYSTEM_NAME << "\"\n";
        report << "  },\n";
        report << "  \"performance_thresholds\": {\n";
        report << "    \"performance_regression\": " << performance_threshold_ << ",\n";
        report << "    \"memory_regression\": " << memory_threshold_ << "\n";
        report << "  },\n";
        report << "  \"test_results\": [\n";
        
        // Run a subset of tests for reporting
        std::vector<std::string> test_names = {
            "entity_creation_report",
            "component_query_report",
            "memory_allocation_report"
        };
        
        for (size_t i = 0; i < test_names.size(); ++i) {
            const auto& test_name = test_names[i];
            
            // Run a quick benchmark
            auto quick_test = [&]() {
                auto entities = factory_->create_many(1000, true);
                
                world_->each<TestPosition, TestVelocity>([](Entity, TestPosition& pos, TestVelocity& vel) {
                    pos.x += vel.vx * 0.016f;
                });
                
                for (auto entity : entities) {
                    world_->destroy_entity(entity);
                }
            };
            
            auto result = benchmarker_->benchmark(test_name, quick_test, 10);
            
            report << "    {\n";
            report << "      \"test_name\": \"" << test_name << "\",\n";
            report << "      \"average_time_ns\": " << result.average_time_ns << ",\n";
            report << "      \"min_time_ns\": " << result.min_time_ns << ",\n";
            report << "      \"max_time_ns\": " << result.max_time_ns << ",\n";
            report << "      \"std_deviation_ns\": " << result.std_deviation_ns << ",\n";
            report << "      \"iterations\": " << result.iterations << "\n";
            report << "    }";
            
            if (i < test_names.size() - 1) {
                report << ",";
            }
            report << "\n";
        }
        
        report << "  ]\n";
        report << "}\n";
        
        std::cout << "Performance report generated: " << report_file << "\n";
    }
    
    EXPECT_TRUE(std::filesystem::exists(report_file)) << "Performance report should be generated";
}

} // namespace ECScope::Testing