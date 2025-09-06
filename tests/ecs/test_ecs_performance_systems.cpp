#include "../framework/ecscope_test_framework.hpp"
#include <ecscope/ecs_performance_benchmarker.hpp>
#include <ecscope/ecs_performance_regression_tester.hpp>
#include <ecscope/ecs_performance_visualizer.hpp>
#include <ecscope/sparse_set.hpp>
#include <ecscope/archetype.hpp>
#include <ecscope/query.hpp>
#include <ecscope/system.hpp>

namespace ECScope::Testing {

using namespace ecscope::performance::ecs;

class ECSPerformanceSystemTest : public PerformanceTestFixture {
protected:
    void SetUp() override {
        PerformanceTestFixture::SetUp();
        
        // Initialize ECS performance benchmarker
        config_ = ECSBenchmarkConfig::create_quick();
        config_.entity_counts = {100, 500, 1000}; // Small counts for fast testing
        config_.iterations = 3;
        config_.enable_stress_testing = false;
        
        benchmarker_ = std::make_unique<ECSPerformanceBenchmarker>(config_);
        
        // Initialize regression tester
        regression_tester_ = std::make_unique<ECSPerformanceRegressionTester>();
        
        // Initialize visualizer
        visualizer_ = std::make_unique<ECSPerformanceVisualizer>();
    }

    void TearDown() override {
        visualizer_.reset();
        regression_tester_.reset();
        benchmarker_.reset();
        PerformanceTestFixture::TearDown();
    }

protected:
    ECSBenchmarkConfig config_;
    std::unique_ptr<ECSPerformanceBenchmarker> benchmarker_;
    std::unique_ptr<ECSPerformanceRegressionTester> regression_tester_;
    std::unique_ptr<ECSPerformanceVisualizer> visualizer_;
};

// =============================================================================
// ECS Performance Benchmarker Tests
// =============================================================================

TEST_F(ECSPerformanceSystemTest, BenchmarkerInitialization) {
    EXPECT_NE(benchmarker_, nullptr);
    EXPECT_FALSE(benchmarker_->is_running());
    EXPECT_EQ(benchmarker_->get_progress(), 0.0);
    
    // Test configuration
    const auto& retrieved_config = benchmarker_->get_config();
    EXPECT_EQ(retrieved_config.iterations, config_.iterations);
    EXPECT_EQ(retrieved_config.entity_counts, config_.entity_counts);
}

TEST_F(ECSPerformanceSystemTest, StandardTestRegistration) {
    benchmarker_->register_all_standard_tests();
    
    auto available_tests = benchmarker_->get_available_tests();
    EXPECT_GT(available_tests.size(), 5);
    
    // Check for expected standard tests
    bool has_entity_lifecycle = false;
    bool has_component_manipulation = false;
    bool has_query_iteration = false;
    
    for (const auto& test_name : available_tests) {
        if (test_name == "EntityLifecycle") has_entity_lifecycle = true;
        if (test_name == "ComponentManipulation") has_component_manipulation = true;
        if (test_name == "QueryIteration") has_query_iteration = true;
    }
    
    EXPECT_TRUE(has_entity_lifecycle);
    EXPECT_TRUE(has_component_manipulation);
    EXPECT_TRUE(has_query_iteration);
    
    // Test descriptions
    for (const auto& test_name : available_tests) {
        std::string description = benchmarker_->get_test_description(test_name);
        EXPECT_FALSE(description.empty());
    }
}

TEST_F(ECSPerformanceSystemTest, EntityLifecycleBenchmark) {
    EntityLifecycleBenchmark test;
    
    EXPECT_EQ(test.get_name(), "EntityLifecycle");
    EXPECT_EQ(test.get_category(), ECSBenchmarkCategory::Architecture);
    EXPECT_FALSE(test.get_description().empty());
    
    // Test different architectures
    std::vector<ECSArchitectureType> architectures = {
        ECSArchitectureType::SparseSet,
        ECSArchitectureType::Archetype_SoA
    };
    
    for (auto architecture : architectures) {
        EXPECT_TRUE(test.supports_architecture(architecture));
        
        auto result = test.run_benchmark(architecture, 100, config_);
        
        EXPECT_TRUE(result.is_valid);
        EXPECT_EQ(result.test_name, "EntityLifecycle");
        EXPECT_EQ(result.architecture_type, architecture);
        EXPECT_EQ(result.entity_count, 100);
        EXPECT_GT(result.average_time_us, 0.0);
        EXPECT_GT(result.entities_per_second, 0.0);
        EXPECT_FALSE(result.raw_timings.empty());
    }
}

TEST_F(ECSPerformanceSystemTest, ComponentManipulationBenchmark) {
    ComponentManipulationBenchmark test;
    
    auto result = test.run_benchmark(ECSArchitectureType::SparseSet, 500, config_);
    
    EXPECT_TRUE(result.is_valid);
    EXPECT_GT(result.components_per_second, 0.0);
    EXPECT_GE(result.archetype_migrations, 0); // Should have some migrations
    EXPECT_GT(result.structural_change_time, 0.0);
    
    // Component manipulation should be relatively fast
    EXPECT_LT(result.average_time_us, 10000.0); // Less than 10ms for 500 entities
}

TEST_F(ECSPerformanceSystemTest, QueryIterationBenchmark) {
    QueryIterationBenchmark test;
    
    auto result = test.run_benchmark(ECSArchitectureType::SparseSet, 1000, config_);
    
    EXPECT_TRUE(result.is_valid);
    EXPECT_GT(result.query_iteration_time, 0.0);
    EXPECT_GT(result.component_access_time, 0.0);
    EXPECT_GT(result.cache_hit_ratio, 0.0); // Should have some cache locality
    
    // Query iteration should be memory-efficient
    EXPECT_GT(result.memory_efficiency, 0.5); // At least 50% efficient
}

TEST_F(ECSPerformanceSystemTest, RandomAccessBenchmark) {
    RandomAccessBenchmark test;
    
    auto result = test.run_benchmark(ECSArchitectureType::SparseSet, 1000, config_);
    
    EXPECT_TRUE(result.is_valid);
    
    // Random access should show cache misses
    EXPECT_LT(result.cache_hit_ratio, 0.8); // Should have cache misses
    EXPECT_GT(result.cache_miss_penalty, 0.0);
    
    // Should be slower than sequential access
    QueryIterationBenchmark sequential_test;
    auto sequential_result = sequential_test.run_benchmark(ECSArchitectureType::SparseSet, 1000, config_);
    
    EXPECT_GT(result.average_time_us, sequential_result.average_time_us);
}

TEST_F(ECSPerformanceSystemTest, ArchetypeMigrationBenchmark) {
    ArchetypeMigrationBenchmark test;
    
    // Test only on archetype-based architectures
    std::vector<ECSArchitectureType> archetype_architectures = {
        ECSArchitectureType::Archetype_SoA
    };
    
    for (auto architecture : archetype_architectures) {
        EXPECT_TRUE(test.supports_architecture(architecture));
        
        auto result = test.run_benchmark(architecture, 500, config_);
        
        EXPECT_TRUE(result.is_valid);
        EXPECT_GT(result.archetype_migrations, 0);
        EXPECT_GT(result.structural_change_time, 0.0);
        
        // Archetype migrations should create multiple archetypes
        EXPECT_GT(result.archetype_count, 1);
    }
    
    // Should not support non-archetype architectures
    EXPECT_FALSE(test.supports_architecture(ECSArchitectureType::SparseSet));
}

TEST_F(ECSPerformanceSystemTest, SystemUpdateBenchmark) {
    SystemUpdateBenchmark test;
    
    auto result = test.run_benchmark(ECSArchitectureType::SparseSet, 1000, config_);
    
    EXPECT_TRUE(result.is_valid);
    EXPECT_GT(result.operations_per_second, 0.0);
    
    // System updates should be consistent
    EXPECT_LT(result.std_deviation_us / result.average_time_us, 0.3); // CV < 30%
    EXPECT_GT(result.consistency_score, 0.7); // Good consistency
}

TEST_F(ECSPerformanceSystemTest, MultiThreadingBenchmark) {
    if (std::thread::hardware_concurrency() < 2) {
        GTEST_SKIP() << "Multi-threading test requires at least 2 cores";
    }
    
    MultiThreadingBenchmark test;
    
    auto single_thread_config = config_;
    single_thread_config.thread_count = 1;
    
    auto multi_thread_config = config_;
    multi_thread_config.thread_count = std::min(4u, std::thread::hardware_concurrency());
    
    auto single_result = test.run_benchmark(ECSArchitectureType::SparseSet, 1000, single_thread_config);
    auto multi_result = test.run_benchmark(ECSArchitectureType::SparseSet, 1000, multi_thread_config);
    
    EXPECT_TRUE(single_result.is_valid);
    EXPECT_TRUE(multi_result.is_valid);
    
    // Multi-threading should provide some speedup for larger workloads
    // (may not always be true for small test workloads)
    std::cout << "Single-thread: " << single_result.average_time_us << "us, "
              << "Multi-thread: " << multi_result.average_time_us << "us\n";
}

TEST_F(ECSPerformanceSystemTest, MemoryPressureBenchmark) {
    MemoryPressureBenchmark test;
    
    auto result = test.run_benchmark(ECSArchitectureType::SparseSet, 1000, config_);
    
    EXPECT_TRUE(result.is_valid);
    EXPECT_GT(result.peak_memory_usage, 0);
    EXPECT_GT(result.allocation_count, 0);
    
    // Memory pressure should show allocations
    EXPECT_GT(result.fragmentation_ratio, 0.0);
    
    // Should handle large components
    EXPECT_GT(result.peak_memory_usage, 100 * 1024); // At least 100KB for 1000 large components
}

// =============================================================================
// Architecture Comparison Tests
// =============================================================================

TEST_F(ECSPerformanceSystemTest, ArchitectureComparison) {
    benchmarker_->register_all_standard_tests();
    
    std::vector<ECSArchitectureType> architectures = {
        ECSArchitectureType::SparseSet,
        ECSArchitectureType::Archetype_SoA
    };
    
    benchmarker_->run_architecture_comparison(architectures);
    
    // Wait for completion
    while (benchmarker_->is_running()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    auto results = benchmarker_->get_results();
    EXPECT_GT(results.size(), 0);
    
    // Should have results for both architectures
    auto sparse_set_results = benchmarker_->get_results_for_architecture(ECSArchitectureType::SparseSet);
    auto archetype_results = benchmarker_->get_results_for_architecture(ECSArchitectureType::Archetype_SoA);
    
    EXPECT_GT(sparse_set_results.size(), 0);
    EXPECT_GT(archetype_results.size(), 0);
    
    // Analyze results
    benchmarker_->analyze_results();
    auto comparisons = benchmarker_->get_architecture_comparisons();
    
    EXPECT_EQ(comparisons.size(), architectures.size());
    
    for (const auto& comparison : comparisons) {
        EXPECT_GT(comparison.overall_score, 0.0);
        EXPECT_FALSE(comparison.test_scores.empty());
    }
}

TEST_F(ECSPerformanceSystemTest, ScalingAnalysis) {
    benchmarker_->register_all_standard_tests();
    
    std::vector<u32> entity_counts = {100, 500, 1000};
    benchmarker_->run_scaling_analysis(entity_counts);
    
    while (benchmarker_->is_running()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    auto results = benchmarker_->get_results();
    EXPECT_GT(results.size(), 0);
    
    // Check that we have results for all entity counts
    for (u32 count : entity_counts) {
        bool found = false;
        for (const auto& result : results) {
            if (result.entity_count == count) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "Missing results for entity count: " << count;
    }
    
    // Generate scaling analysis
    std::string scaling_report = benchmarker_->generate_scaling_analysis();
    EXPECT_FALSE(scaling_report.empty());
    std::cout << "Scaling Analysis:\n" << scaling_report << "\n";
}

// =============================================================================
// Regression Testing Tests
// =============================================================================

TEST_F(ECSPerformanceSystemTest, RegressionTesterInitialization) {
    EXPECT_NE(regression_tester_, nullptr);
    
    // Configure baseline
    std::vector<ECSBenchmarkResult> baseline_results;
    regression_tester_->set_baseline_results(baseline_results);
    
    // Test configuration
    ECSRegressionTestConfig regression_config;
    regression_config.regression_threshold = 0.05; // 5% threshold
    regression_config.enable_statistical_analysis = true;
    
    regression_tester_->set_config(regression_config);
    
    auto retrieved_config = regression_tester_->get_config();
    EXPECT_EQ(retrieved_config.regression_threshold, 0.05);
    EXPECT_TRUE(retrieved_config.enable_statistical_analysis);
}

TEST_F(ECSPerformanceSystemTest, RegressionDetection) {
    // Create some baseline results
    std::vector<ECSBenchmarkResult> baseline_results;
    
    for (int i = 0; i < 3; ++i) {
        ECSBenchmarkResult result;
        result.test_name = "TestBenchmark" + std::to_string(i);
        result.category = ECSBenchmarkCategory::Architecture;
        result.architecture_type = ECSArchitectureType::SparseSet;
        result.entity_count = 1000;
        result.average_time_us = 1000.0 + i * 100.0; // 1000, 1100, 1200 us
        result.std_deviation_us = 50.0;
        result.entities_per_second = 1000000.0 / result.average_time_us;
        result.is_valid = true;
        
        baseline_results.push_back(result);
    }
    
    regression_tester_->set_baseline_results(baseline_results);
    
    // Create current results with a regression
    std::vector<ECSBenchmarkResult> current_results = baseline_results;
    current_results[1].average_time_us = 1320.0; // 20% slower (regression)
    current_results[1].entities_per_second = 1000000.0 / current_results[1].average_time_us;
    
    auto regression_report = regression_tester_->detect_regressions(current_results);
    
    EXPECT_FALSE(regression_report.regressions.empty());
    EXPECT_EQ(regression_report.regressions.size(), 1);
    EXPECT_EQ(regression_report.regressions[0].test_name, "TestBenchmark1");
    EXPECT_GT(regression_report.regressions[0].performance_change, 0.15); // At least 15% regression
}

TEST_F(ECSPerformanceSystemTest, PerformanceImprovementDetection) {
    // Test improvement detection
    std::vector<ECSBenchmarkResult> baseline_results;
    
    ECSBenchmarkResult baseline;
    baseline.test_name = "ImprovementTest";
    baseline.average_time_us = 2000.0;
    baseline.entities_per_second = 500.0;
    baseline.is_valid = true;
    baseline_results.push_back(baseline);
    
    regression_tester_->set_baseline_results(baseline_results);
    
    std::vector<ECSBenchmarkResult> current_results = baseline_results;
    current_results[0].average_time_us = 1500.0; // 25% faster
    current_results[0].entities_per_second = 666.7;
    
    auto regression_report = regression_tester_->detect_regressions(current_results);
    
    EXPECT_FALSE(regression_report.improvements.empty());
    EXPECT_EQ(regression_report.improvements[0].test_name, "ImprovementTest");
    EXPECT_GT(regression_report.improvements[0].performance_change, 0.2); // At least 20% improvement
}

// =============================================================================
// Visualization Tests
// =============================================================================

TEST_F(ECSPerformanceSystemTest, VisualizationDataGeneration) {
    benchmarker_->register_all_standard_tests();
    benchmarker_->run_scaling_analysis({100, 500, 1000});
    
    while (benchmarker_->is_running()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    auto visualization_data = benchmarker_->generate_visualization_data();
    
    EXPECT_FALSE(visualization_data.scaling_curve.empty());
    EXPECT_FALSE(visualization_data.architecture_performance.empty());
    EXPECT_FALSE(visualization_data.test_breakdown.empty());
    EXPECT_FALSE(visualization_data.interpretation.empty());
    
    // Scaling curve should have entries for each entity count
    EXPECT_EQ(visualization_data.scaling_curve.size(), 3);
    
    for (const auto& [entity_count, performance] : visualization_data.scaling_curve) {
        EXPECT_GT(entity_count, 0);
        EXPECT_GT(performance, 0.0);
    }
}

TEST_F(ECSPerformanceSystemTest, PerformanceVisualizerIntegration) {
    // Generate some test results
    std::vector<ECSBenchmarkResult> results;
    
    ECSBenchmarkResult result;
    result.test_name = "VisualizationTest";
    result.category = ECSBenchmarkCategory::Memory;
    result.architecture_type = ECSArchitectureType::SparseSet;
    result.entity_count = 1000;
    result.average_time_us = 1500.0;
    result.entities_per_second = 666.7;
    result.peak_memory_usage = 1024 * 1024; // 1MB
    result.cache_hit_ratio = 0.85;
    result.is_valid = true;
    
    results.push_back(result);
    
    visualizer_->set_benchmark_results(results);
    
    // Test visualization data extraction
    auto chart_data = visualizer_->generate_performance_chart_data();
    EXPECT_FALSE(chart_data.empty());
    
    auto memory_data = visualizer_->generate_memory_usage_chart();
    EXPECT_FALSE(memory_data.empty());
    
    auto cache_data = visualizer_->generate_cache_behavior_chart();
    EXPECT_FALSE(cache_data.empty());
}

// =============================================================================
// Educational Insights Tests
// =============================================================================

TEST_F(ECSPerformanceSystemTest, EducationalInsights) {
    benchmarker_->register_all_standard_tests();
    benchmarker_->run_architecture_comparison({
        ECSArchitectureType::SparseSet,
        ECSArchitectureType::Archetype_SoA
    });
    
    while (benchmarker_->is_running()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    auto insights = benchmarker_->get_educational_insights();
    EXPECT_FALSE(insights.empty());
    
    // Each insight should be informative
    for (const auto& insight : insights) {
        EXPECT_GT(insight.length(), 20); // At least 20 characters
        std::cout << "Educational Insight: " << insight << "\n";
    }
    
    // Test result explanation
    auto results = benchmarker_->get_results();
    if (!results.empty()) {
        std::string explanation = benchmarker_->explain_result(results[0]);
        EXPECT_FALSE(explanation.empty());
        
        auto optimizations = benchmarker_->suggest_optimizations(results[0]);
        EXPECT_FALSE(optimizations.empty());
        
        std::cout << "Result Explanation: " << explanation << "\n";
        std::cout << "Optimization Suggestions:\n";
        for (const auto& opt : optimizations) {
            std::cout << "  - " << opt << "\n";
        }
    }
}

TEST_F(ECSPerformanceSystemTest, OptimizationRecommendations) {
    benchmarker_->register_all_standard_tests();
    benchmarker_->run_scaling_analysis({100, 1000});
    
    while (benchmarker_->is_running()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    std::string recommendations = benchmarker_->generate_optimization_recommendations();
    EXPECT_FALSE(recommendations.empty());
    EXPECT_GT(recommendations.length(), 100); // Should be substantial
    
    std::cout << "Optimization Recommendations:\n" << recommendations << "\n";
    
    // Should contain common optimization keywords
    bool has_memory = recommendations.find("memory") != std::string::npos ||
                     recommendations.find("cache") != std::string::npos;
    bool has_architecture = recommendations.find("architecture") != std::string::npos ||
                           recommendations.find("archetype") != std::string::npos;
    
    EXPECT_TRUE(has_memory || has_architecture);
}

// =============================================================================
// Export and Reporting Tests
// =============================================================================

TEST_F(ECSPerformanceSystemTest, ResultExport) {
    // Generate some test results
    benchmarker_->register_all_standard_tests();
    benchmarker_->run_scaling_analysis({100, 500});
    
    while (benchmarker_->is_running()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Test CSV export
    std::string csv_filename = "test_ecs_results.csv";
    EXPECT_NO_THROW(benchmarker_->export_results_csv(csv_filename));
    
    // Check if file was created and has content
    std::ifstream csv_file(csv_filename);
    EXPECT_TRUE(csv_file.is_open());
    
    std::string line;
    int line_count = 0;
    while (std::getline(csv_file, line)) {
        line_count++;
        if (line_count == 1) {
            // Header should contain expected columns
            EXPECT_NE(line.find("test_name"), std::string::npos);
            EXPECT_NE(line.find("architecture"), std::string::npos);
            EXPECT_NE(line.find("entity_count"), std::string::npos);
            EXPECT_NE(line.find("average_time_us"), std::string::npos);
        }
    }
    EXPECT_GT(line_count, 1); // Should have header + data
    csv_file.close();
    
    // Test JSON export
    std::string json_filename = "test_ecs_results.json";
    EXPECT_NO_THROW(benchmarker_->export_results_json(json_filename));
    
    std::ifstream json_file(json_filename);
    EXPECT_TRUE(json_file.is_open());
    json_file.close();
    
    // Test comparative report
    std::string report_filename = "test_ecs_report.txt";
    EXPECT_NO_THROW(benchmarker_->export_comparative_report(report_filename));
    
    std::ifstream report_file(report_filename);
    EXPECT_TRUE(report_file.is_open());
    report_file.close();
    
    // Clean up test files
    std::remove(csv_filename.c_str());
    std::remove(json_filename.c_str());
    std::remove(report_filename.c_str());
}

TEST_F(ECSPerformanceSystemTest, ComparativeReport) {
    benchmarker_->register_all_standard_tests();
    benchmarker_->run_architecture_comparison({
        ECSArchitectureType::SparseSet,
        ECSArchitectureType::Archetype_SoA
    });
    
    while (benchmarker_->is_running()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    benchmarker_->analyze_results();
    
    std::string report = benchmarker_->generate_comparative_report();
    EXPECT_FALSE(report.empty());
    EXPECT_GT(report.length(), 500); // Should be substantial
    
    // Report should mention both architectures
    EXPECT_NE(report.find("SparseSet"), std::string::npos);
    EXPECT_NE(report.find("Archetype"), std::string::npos);
    
    std::cout << "Comparative Report (excerpt):\n" 
              << report.substr(0, 500) << "...\n";
}

// =============================================================================
// Utility and Helper Tests
// =============================================================================

TEST_F(ECSPerformanceSystemTest, UtilityFunctions) {
    // Test architecture string conversion
    std::string sparse_set_str = ECSPerformanceBenchmarker::architecture_to_string(
        ECSArchitectureType::SparseSet);
    EXPECT_EQ(sparse_set_str, "SparseSet");
    
    std::string archetype_str = ECSPerformanceBenchmarker::architecture_to_string(
        ECSArchitectureType::Archetype_SoA);
    EXPECT_EQ(archetype_str, "Archetype_SoA");
    
    // Test category string conversion
    std::string memory_str = ECSPerformanceBenchmarker::category_to_string(
        ECSBenchmarkCategory::Memory);
    EXPECT_EQ(memory_str, "Memory");
    
    std::string architecture_str_cat = ECSPerformanceBenchmarker::category_to_string(
        ECSBenchmarkCategory::Architecture);
    EXPECT_EQ(architecture_str_cat, "Architecture");
}

TEST_F(ECSPerformanceSystemTest, BenchmarkResultProcessing) {
    ECSBenchmarkResult result;
    result.test_name = "TestResult";
    result.architecture_type = ECSArchitectureType::SparseSet;
    result.entity_count = 1000;
    result.raw_timings = {1000.0, 1100.0, 900.0, 1050.0, 950.0};
    
    result.calculate_statistics();
    
    EXPECT_NEAR(result.average_time_us, 1000.0, 50.0); // Should be around 1000us
    EXPECT_GT(result.std_deviation_us, 0.0);
    EXPECT_EQ(result.min_time_us, 900.0);
    EXPECT_EQ(result.max_time_us, 1100.0);
    EXPECT_NEAR(result.median_time_us, 1000.0, 100.0);
    
    // Test CSV export
    std::string csv_row = result.to_csv_row();
    EXPECT_FALSE(csv_row.empty());
    EXPECT_NE(csv_row.find("TestResult"), std::string::npos);
    EXPECT_NE(csv_row.find("1000"), std::string::npos);
    
    // Test JSON export
    std::string json_str = result.to_json();
    EXPECT_FALSE(json_str.empty());
    EXPECT_NE(json_str.find("TestResult"), std::string::npos);
    EXPECT_NE(json_str.find("SparseSet"), std::string::npos);
}

// =============================================================================
// Factory and Integration Tests
// =============================================================================

TEST_F(ECSPerformanceSystemTest, BenchmarkSuiteFactory) {
    // Test quick suite creation
    auto quick_suite = ECSBenchmarkSuiteFactory::create_quick_suite();
    EXPECT_NE(quick_suite, nullptr);
    
    auto quick_config = quick_suite->get_config();
    EXPECT_LT(quick_config.entity_counts.size(), 5); // Should have fewer test cases
    EXPECT_LT(quick_config.iterations, 10);
    
    // Test comprehensive suite creation
    auto comprehensive_suite = ECSBenchmarkSuiteFactory::create_comprehensive_suite();
    EXPECT_NE(comprehensive_suite, nullptr);
    
    auto comprehensive_config = comprehensive_suite->get_config();
    EXPECT_GT(comprehensive_config.entity_counts.size(), quick_config.entity_counts.size());
    EXPECT_GT(comprehensive_config.iterations, quick_config.iterations);
    
    // Test educational suite
    auto educational_suite = ECSBenchmarkSuiteFactory::create_educational_suite();
    EXPECT_NE(educational_suite, nullptr);
    
    // Test memory suite
    auto memory_suite = ECSBenchmarkSuiteFactory::create_memory_suite();
    EXPECT_NE(memory_suite, nullptr);
}

// =============================================================================
// Stress and Edge Case Tests
// =============================================================================

TEST_F(ECSPerformanceSystemTest, EmptyEntityBenchmark) {
    EntityLifecycleBenchmark test;
    
    // Test with zero entities (edge case)
    auto result = test.run_benchmark(ECSArchitectureType::SparseSet, 0, config_);
    
    // Should handle gracefully, even if not providing meaningful data
    EXPECT_TRUE(result.is_valid || !result.error_message.empty());
}

TEST_F(ECSPerformanceSystemTest, CancellationHandling) {
    benchmarker_->register_all_standard_tests();
    
    // Start benchmark in separate thread
    std::thread benchmark_thread([this]() {
        benchmarker_->run_scaling_analysis({100, 500, 1000, 5000});
    });
    
    // Wait a bit, then cancel
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    benchmarker_->cancel_benchmarks();
    
    // Should stop relatively quickly
    auto start_time = std::chrono::steady_clock::now();
    benchmark_thread.join();
    auto end_time = std::chrono::steady_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
    EXPECT_LT(duration.count(), 5); // Should cancel within 5 seconds
    
    EXPECT_FALSE(benchmarker_->is_running());
}

TEST_F(ECSPerformanceSystemTest, ConfigurationValidation) {
    // Test invalid configuration handling
    ECSBenchmarkConfig invalid_config;
    invalid_config.entity_counts = {}; // Empty
    invalid_config.iterations = 0;     // Invalid
    
    ECSPerformanceBenchmarker invalid_benchmarker(invalid_config);
    
    // Should handle gracefully or provide useful error messages
    invalid_benchmarker.register_all_standard_tests();
    auto available_tests = invalid_benchmarker.get_available_tests();
    // Should still register tests even with invalid config
    EXPECT_GT(available_tests.size(), 0);
}

} // namespace ECScope::Testing