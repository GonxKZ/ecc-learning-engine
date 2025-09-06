/**
 * @file 10-comprehensive-performance-benchmarking.cpp
 * @brief Comprehensive ECS Performance Benchmarking Example
 * 
 * This example demonstrates the complete ECS performance benchmarking suite,
 * including architecture comparison, regression testing, visualization, and
 * educational analysis. It showcases all aspects of performance analysis
 * and optimization in ECScope.
 * 
 * Key Features Demonstrated:
 * - Complete benchmarking suite execution
 * - Architecture performance comparison
 * - Regression testing and baseline management
 * - Real-time performance visualization
 * - Educational insights and optimization recommendations
 * - Integration with existing physics and memory systems
 * 
 * Learning Objectives:
 * - Understand comprehensive performance analysis methodology
 * - Learn to interpret performance benchmarking results
 * - See the integration of multiple performance analysis tools
 * - Practice performance optimization decision-making
 * - Experience regression testing and continuous monitoring
 * 
 * @author ECScope Educational ECS Framework
 * @date 2025
 */

#include "ecs_performance_benchmarker.hpp"
#include "ecs_performance_visualizer.hpp"
#include "ecs_performance_regression_tester.hpp"
#include "performance_lab.hpp"
#include "memory_benchmark_suite.hpp"
#include "core/log.hpp"
#include "core/time.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace ecscope;
using namespace ecscope::performance;

//=============================================================================
// Educational Performance Analysis Demo
//=============================================================================

class ComprehensivePerformanceDemo {
private:
    std::unique_ptr<ecs::ECSPerformanceBenchmarker> benchmarker_;
    std::unique_ptr<visualization::ECSPerformanceVisualizer> visualizer_;
    std::unique_ptr<regression::ECSPerformanceRegressionTester> regression_tester_;
    std::unique_ptr<PerformanceLab> performance_lab_;
    
public:
    ComprehensivePerformanceDemo() {
        LOG_INFO("=== ECScope Comprehensive Performance Analysis Demo ===");
        initialize_systems();
    }
    
    void run_complete_analysis() {
        LOG_INFO("\n🚀 Starting Comprehensive Performance Analysis");
        
        // Phase 1: Initial benchmarking
        demonstrate_basic_benchmarking();
        
        // Phase 2: Architecture comparison
        demonstrate_architecture_comparison();
        
        // Phase 3: Memory performance analysis
        demonstrate_memory_performance_analysis();
        
        // Phase 4: Regression testing
        demonstrate_regression_testing();
        
        // Phase 5: Real-time monitoring
        demonstrate_realtime_monitoring();
        
        // Phase 6: Educational insights
        demonstrate_educational_features();
        
        // Phase 7: Integration testing
        demonstrate_system_integration();
        
        // Phase 8: Optimization recommendations
        demonstrate_optimization_recommendations();
        
        LOG_INFO("\n✅ Comprehensive Performance Analysis Complete!");
    }
    
private:
    void initialize_systems() {
        LOG_INFO("Initializing performance analysis systems...");
        
        // Create performance lab
        performance_lab_ = PerformanceLabFactory::create_educational_lab();
        performance_lab_->initialize();
        
        // Create benchmarker with educational configuration
        auto benchmark_config = ecs::ECSBenchmarkConfig::create_comprehensive();
        benchmark_config.generate_comparative_report = true;
        benchmark_config.generate_visualization_data = true;
        benchmarker_ = std::make_unique<ecs::ECSPerformanceBenchmarker>(benchmark_config);
        
        // Create visualizer
        visualizer_ = std::make_unique<visualization::ECSPerformanceVisualizer>();
        visualizer_->initialize();
        visualizer_->set_benchmarker(benchmarker_);
        
        // Create regression tester
        auto regression_config = regression::RegressionTestConfig::create_default();
        regression_tester_ = std::make_unique<regression::ECSPerformanceRegressionTester>(regression_config);
        regression_tester_->initialize();
        
        LOG_INFO("✅ All systems initialized successfully");
    }
    
    void demonstrate_basic_benchmarking() {
        LOG_INFO("\n📊 Phase 1: Basic Performance Benchmarking");
        LOG_INFO("===========================================");
        
        std::cout << R"(
🎯 EDUCATIONAL CONTEXT: Basic Performance Benchmarking

In this phase, we'll run fundamental ECS performance tests to establish
baseline performance characteristics. This is the foundation of all
performance analysis work.

Key Concepts:
• Entity lifecycle performance (creation/destruction)
• Component manipulation overhead
• Query iteration efficiency
• Memory access patterns

We'll test multiple architectures to see their trade-offs:
)" << std::endl;
        
        // Run basic benchmarks
        LOG_INFO("Running basic benchmarking suite...");
        
        benchmarker_->run_specific_benchmarks({
            "EntityLifecycle",
            "ComponentManipulation", 
            "QueryIteration",
            "RandomAccess"
        });
        
        auto results = benchmarker_->get_results();
        LOG_INFO("Completed {} benchmark tests", results.size());
        
        // Show basic results summary
        display_benchmark_summary(results);
        
        std::cout << R"(
💡 Key Takeaways:
• Different architectures excel at different operations
• Entity count significantly impacts performance scaling
• Memory access patterns are crucial for performance
• Consistency matters as much as raw speed
)" << std::endl;
    }
    
    void demonstrate_architecture_comparison() {
        LOG_INFO("\n🏗️ Phase 2: Architecture Comparison Analysis");
        LOG_INFO("=============================================");
        
        std::cout << R"(
🎯 EDUCATIONAL CONTEXT: ECS Architecture Trade-offs

Different ECS architectures have different performance characteristics:

• Archetype (SoA): Excellent for iteration, moderate for random access
• Sparse Set: Good for random access, moderate for iteration
• Component Arrays: Simple but limited scalability
• Hybrid: Attempts to balance trade-offs

Let's compare them across different scenarios:
)" << std::endl;
        
        // Run architecture comparison
        std::vector<ecs::ECSArchitectureType> architectures = {
            ecs::ECSArchitectureType::Archetype_SoA,
            ecs::ECSArchitectureType::SparseSet
        };
        
        benchmarker_->run_architecture_comparison(architectures);
        
        // Generate and display comparison
        auto comparison_report = benchmarker_->generate_comparative_report();
        std::cout << "\n" << comparison_report << std::endl;
        
        // Show educational insights
        auto insights = benchmarker_->get_educational_insights();
        std::cout << "\n🧠 Educational Insights:\n";
        for (const auto& insight : insights) {
            std::cout << "• " << insight << "\n";
        }
        
        std::cout << R"(
💡 Architecture Selection Guidelines:
• Choose Archetype (SoA) for systems that iterate over many entities
• Choose Sparse Set for systems with frequent component additions/removals
• Consider hybrid approaches for complex scenarios
• Memory usage patterns matter as much as raw performance
)" << std::endl;
    }
    
    void demonstrate_memory_performance_analysis() {
        LOG_INFO("\n🧠 Phase 3: Memory Performance Analysis");
        LOG_INFO("=====================================");
        
        std::cout << R"(
🎯 EDUCATIONAL CONTEXT: Memory Performance in ECS

Memory performance is crucial for ECS systems because:
• Cache locality affects iteration speed
• Memory allocation patterns impact scalability
• Fragmentation can degrade performance over time
• NUMA effects matter on multi-socket systems

Let's analyze memory behavior across different patterns:
)" << std::endl;
        
        // Create memory benchmark suite
        memory::benchmark::BenchmarkConfig memory_config;
        memory_config.entity_counts = {1000, 5000, 10000, 25000};
        memory_config.enable_cache_tests = true;
        memory_config.enable_numa_tests = true;
        
        memory::benchmark::MemoryBenchmarkSuite memory_suite(memory_config);
        
        LOG_INFO("Running memory performance analysis...");
        memory_suite.run_all_benchmarks();
        
        auto memory_analysis = memory_suite.generate_comparative_analysis();
        std::cout << "\n" << memory_analysis << std::endl;
        
        std::cout << R"(
💡 Memory Optimization Insights:
• Sequential memory access is 3-10x faster than random access
• Cache-friendly data layouts dramatically improve performance
• Memory pooling reduces allocation overhead
• NUMA-aware allocation can improve multi-threaded performance
)" << std::endl;
    }
    
    void demonstrate_regression_testing() {
        LOG_INFO("\n🔄 Phase 4: Performance Regression Testing");
        LOG_INFO("==========================================");
        
        std::cout << R"(
🎯 EDUCATIONAL CONTEXT: Performance Regression Testing

Regression testing ensures that:
• Performance optimizations don't break existing functionality
• Performance degradations are detected early
• Performance trends are tracked over time
• Statistical significance is properly evaluated

This is crucial for maintaining system quality over time.
)" << std::endl;
        
        // Establish baseline if needed
        if (!regression_tester_->has_valid_baselines()) {
            LOG_INFO("Establishing performance baselines...");
            regression_tester_->establish_baseline();
            
            std::cout << R"(
📊 Baseline Established!

A performance baseline captures the "normal" performance characteristics
of your system. It includes:
• Statistical measures (mean, std dev, percentiles)
• Sample size for statistical validity
• Platform and configuration metadata
• Confidence intervals for comparison
)" << std::endl;
        }
        
        // Run regression tests
        LOG_INFO("Running regression analysis...");
        auto regression_results = regression_tester_->run_regression_tests();
        
        LOG_INFO("Regression test results: {} tests analyzed", regression_results.size());
        
        // Display regression analysis
        display_regression_results(regression_results);
        
        // Generate trend analysis
        auto trend_report = regression_tester_->generate_trend_analysis_report();
        std::cout << "\n" << trend_report << std::endl;
        
        std::cout << R"(
💡 Regression Testing Best Practices:
• Establish baselines with sufficient sample sizes (>10 samples)
• Use statistical significance testing, not just raw comparisons
• Consider both performance degradation AND improvement
• Track trends over time, not just point-in-time comparisons
• Automate regression testing in your CI/CD pipeline
)" << std::endl;
    }
    
    void demonstrate_realtime_monitoring() {
        LOG_INFO("\n⏱️ Phase 5: Real-time Performance Monitoring");
        LOG_INFO("============================================");
        
        std::cout << R"(
🎯 EDUCATIONAL CONTEXT: Real-time Performance Monitoring

Real-time monitoring helps you:
• Identify performance bottlenecks as they occur
• Understand system behavior under different loads
• Detect performance anomalies and spikes
• Validate optimizations in real-time

Let's simulate a real-time workload and monitor its performance:
)" << std::endl;
        
        // Start real-time monitoring
        visualizer_->start_realtime_monitoring();
        
        // Simulate varying workload
        simulate_varying_workload();
        
        // Get real-time insights
        auto current_insights = visualizer_->get_current_insights();
        std::cout << "\n🔍 Real-time Performance Insights:\n";
        for (const auto& insight : current_insights) {
            std::cout << "• " << insight << "\n";
        }
        
        // Check for bottlenecks
        auto bottlenecks = visualizer_->get_identified_bottlenecks();
        if (!bottlenecks.empty()) {
            std::cout << "\n⚠️ Performance Bottlenecks Detected:\n";
            for (const auto& bottleneck : bottlenecks) {
                std::cout << "• " << bottleneck.name << ": " << bottleneck.description << "\n";
                std::cout << "  Impact: " << bottleneck.impact_factor << "x slowdown\n";
                std::cout << "  Solutions: ";
                for (const auto& solution : bottleneck.solutions) {
                    std::cout << solution << "; ";
                }
                std::cout << "\n";
            }
        }
        
        visualizer_->stop_realtime_monitoring();
        
        std::cout << R"(
💡 Real-time Monitoring Insights:
• Performance can vary significantly with workload patterns
• Bottlenecks may only appear under specific conditions
• Real-time feedback enables immediate optimization
• Continuous monitoring catches regressions early
)" << std::endl;
    }
    
    void demonstrate_educational_features() {
        LOG_INFO("\n🎓 Phase 6: Educational Performance Analysis");
        LOG_INFO("===========================================");
        
        std::cout << R"(
🎯 EDUCATIONAL CONTEXT: Learning from Performance Data

Performance analysis is not just about numbers - it's about understanding
the underlying systems and making informed optimization decisions.

Let's explore educational features that help you learn:
)" << std::endl;
        
        // Get educational content
        auto explanation = visualizer_->get_educational_content("cache_locality");
        std::cout << "\n📚 Understanding Cache Locality:\n" << explanation << "\n";
        
        explanation = visualizer_->get_educational_content("ecs_architectures");
        std::cout << "\n🏗️ ECS Architecture Trade-offs:\n" << explanation << "\n";
        
        // Interactive queries
        std::cout << "\n❓ Interactive Performance Questions:\n";
        auto queries = visualizer_->get_available_queries();
        for (size_t i = 0; i < std::min(queries.size(), 3uz); ++i) {
            std::cout << "Q: " << queries[i] << "\n";
            auto answer = visualizer_->answer_query(queries[i]);
            std::cout << "A: " << answer << "\n\n";
        }
        
        std::cout << R"(
💡 Educational Value of Performance Analysis:
• Understand the 'why' behind performance characteristics
• Learn to make informed optimization decisions
• Develop intuition for performance trade-offs
• Build skills in performance debugging methodology
)" << std::endl;
    }
    
    void demonstrate_system_integration() {
        LOG_INFO("\n🔗 Phase 7: System Integration Performance");
        LOG_INFO("========================================");
        
        std::cout << R"(
🎯 EDUCATIONAL CONTEXT: Real-world Integration Performance

ECS systems rarely exist in isolation. They integrate with:
• Physics systems (collision detection, simulation)
• Rendering systems (culling, batching, drawing)
• Audio systems (3D positioning, streaming)
• Networking systems (synchronization, prediction)

Integration performance is often where bottlenecks hide:
)" << std::endl;
        
        // Run integration benchmarks
        if (benchmarker_->get_config().test_physics_integration) {
            LOG_INFO("Testing physics integration performance...");
            benchmarker_->run_specific_benchmarks({"PhysicsIntegration"});
        }
        
        if (benchmarker_->get_config().test_rendering_integration) {
            LOG_INFO("Testing rendering integration performance...");
            benchmarker_->run_specific_benchmarks({"RenderingIntegration"});
        }
        
        // Multi-threading analysis
        if (benchmarker_->get_config().test_multi_threading) {
            LOG_INFO("Testing multi-threading scalability...");
            benchmarker_->run_specific_benchmarks({"MultiThreading"});
        }
        
        auto integration_results = benchmarker_->get_results_for_test("PhysicsIntegration");
        if (!integration_results.empty()) {
            display_integration_analysis(integration_results);
        }
        
        std::cout << R"(
💡 Integration Performance Lessons:
• Integration overhead can dominate performance
• Data transformation costs between systems add up
• Cache coherency becomes critical in integrated systems
• Threading models must be carefully coordinated
• Profiling integrated systems reveals hidden bottlenecks
)" << std::endl;
    }
    
    void demonstrate_optimization_recommendations() {
        LOG_INFO("\n🎯 Phase 8: Optimization Recommendations");
        LOG_INFO("=======================================");
        
        std::cout << R"(
🎯 EDUCATIONAL CONTEXT: From Analysis to Action

The ultimate goal of performance analysis is actionable optimization.
Good performance tools don't just show you problems - they suggest solutions.

Let's see what optimizations our analysis suggests:
)" << std::endl;
        
        // Get optimization recommendations
        auto recommendations = visualizer_->get_optimization_recommendations();
        
        std::cout << "\n🔧 Optimization Recommendations:\n";
        for (const auto& rec : recommendations) {
            std::cout << "\n📈 " << rec.title << "\n";
            std::cout << "   " << rec.description << "\n";
            std::cout << "   Expected Improvement: " << rec.expected_improvement << "%\n";
            std::cout << "   Implementation Effort: " << (rec.implementation_effort * 100) << "%\n";
            
            if (!rec.steps.empty()) {
                std::cout << "   Implementation Steps:\n";
                for (const auto& step : rec.steps) {
                    std::cout << "   • " << step << "\n";
                }
            }
        }
        
        // Performance improvement estimates
        auto scaling_analysis = benchmarker_->generate_scaling_analysis();
        std::cout << "\n" << scaling_analysis << std::endl;
        
        // Overall health score
        f64 health_score = regression_tester_->calculate_overall_health_score();
        std::cout << "\n🏥 Overall System Health Score: " << (health_score * 100) << "/100\n";
        
        if (health_score > 0.8) {
            std::cout << "✅ Excellent performance - system is well optimized\n";
        } else if (health_score > 0.6) {
            std::cout << "⚠️ Good performance - some optimization opportunities exist\n";
        } else {
            std::cout << "🔧 Performance needs attention - significant optimizations recommended\n";
        }
        
        std::cout << R"(
💡 Optimization Strategy Guidelines:
• Prioritize optimizations by impact vs. effort ratio
• Focus on bottlenecks that affect your specific use cases
• Measure before and after optimization to validate improvements
• Consider maintainability and code complexity in optimization decisions
• Use profiling to guide optimization rather than guessing
)" << std::endl;
    }
    
    void simulate_varying_workload() {
        LOG_INFO("Simulating varying workload for real-time analysis...");
        
        // Simulate different entity counts and update patterns
        std::vector<u32> workload_sizes = {100, 500, 1000, 2000, 1000, 500};
        
        for (u32 entity_count : workload_sizes) {
            // Create temporary registry for simulation
            auto test_config = ecs::ECSBenchmarkConfig::create_quick();
            auto registry = std::make_unique<ecs::Registry>(
                ecs::AllocatorConfig::create_performance_optimized(), "SimulationRegistry");
            
            // Populate with entities
            for (u32 i = 0; i < entity_count; ++i) {
                Entity entity = registry->create_entity();
                registry->add_component(entity, ecs::BenchmarkPosition{});
                registry->add_component(entity, ecs::BenchmarkVelocity{});
            }
            
            // Simulate update loop
            auto start = std::chrono::high_resolution_clock::now();
            
            registry->view<ecs::BenchmarkPosition, ecs::BenchmarkVelocity>().each(
                [](Entity, ecs::BenchmarkPosition& pos, ecs::BenchmarkVelocity& vel) {
                    pos.x += vel.x * 0.016f;
                    pos.y += vel.y * 0.016f;
                    pos.z += vel.z * 0.016f;
                });
            
            auto end = std::chrono::high_resolution_clock::now();
            f64 update_time = std::chrono::duration<f64, std::milli>(end - start).count();
            
            // Add data to real-time monitor
            visualization::RealTimePerformanceData data;
            data.timestamp = core::time::get_time();
            data.frame_time_ms = update_time;
            data.ecs_update_time_ms = update_time;
            data.entity_count = entity_count;
            data.memory_usage_bytes = entity_count * (sizeof(ecs::BenchmarkPosition) + sizeof(ecs::BenchmarkVelocity));
            data.cache_hit_ratio = 0.85; // Simulated
            
            visualizer_->add_realtime_data(data);
            
            // Brief pause to simulate frame timing
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        
        LOG_INFO("Workload simulation complete");
    }
    
    void display_benchmark_summary(const std::vector<ecs::ECSBenchmarkResult>& results) {
        std::cout << "\n📊 Benchmark Results Summary:\n";
        std::cout << "==============================\n";
        
        // Group by test name
        std::unordered_map<std::string, std::vector<const ecs::ECSBenchmarkResult*>> by_test;
        for (const auto& result : results) {
            if (result.is_valid) {
                by_test[result.test_name].push_back(&result);
            }
        }
        
        for (const auto& [test_name, test_results] : by_test) {
            std::cout << "\n🔬 " << test_name << ":";
            for (const auto* result : test_results) {
                std::cout << "\n   " << benchmarker_->architecture_to_string(result->architecture_type)
                         << " (" << result->entity_count << " entities): "
                         << std::fixed << std::setprecision(2) << result->entities_per_second 
                         << " entities/sec";
            }
        }
        
        std::cout << std::endl;
    }
    
    void display_regression_results(const std::vector<regression::RegressionTestResult>& results) {
        std::cout << "\n📈 Regression Analysis Results:\n";
        std::cout << "===============================\n";
        
        u32 passed = 0, warnings = 0, regressions = 0, improvements = 0;
        
        for (const auto& result : results) {
            switch (result.status) {
                case regression::RegressionTestResult::Status::Pass:
                    passed++;
                    break;
                case regression::RegressionTestResult::Status::Warning:
                    warnings++;
                    std::cout << "⚠️ " << result.test_name << ": " 
                             << result.performance_change_percent << "% change\n";
                    break;
                case regression::RegressionTestResult::Status::Regression:
                    regressions++;
                    std::cout << "🔴 " << result.test_name << ": " 
                             << result.performance_change_percent << "% REGRESSION\n";
                    break;
                case regression::RegressionTestResult::Status::Improvement:
                    improvements++;
                    std::cout << "✅ " << result.test_name << ": " 
                             << result.performance_change_percent << "% improvement\n";
                    break;
                default:
                    break;
            }
        }
        
        std::cout << "\nSummary: " << passed << " passed, " << warnings << " warnings, "
                 << regressions << " regressions, " << improvements << " improvements\n";
    }
    
    void display_integration_analysis(const std::vector<ecs::ECSBenchmarkResult>& results) {
        std::cout << "\n🔗 Integration Performance Analysis:\n";
        std::cout << "==================================\n";
        
        for (const auto& result : results) {
            if (result.is_valid) {
                std::cout << "\n⚙️ Physics Integration (" << result.entity_count << " entities):\n";
                std::cout << "   Performance: " << std::fixed << std::setprecision(2) 
                         << result.entities_per_second << " entities/sec\n";
                std::cout << "   Memory Usage: " << (result.peak_memory_usage / 1024) << " KB\n";
                std::cout << "   Consistency: " << (result.consistency_score * 100) << "%\n";
            }
        }
    }
};

//=============================================================================
// Main Demonstration Function
//=============================================================================

int main() {
    try {
        LOG_INFO("Starting ECScope Comprehensive Performance Benchmarking Demo");
        
        ComprehensivePerformanceDemo demo;
        demo.run_complete_analysis();
        
        std::cout << R"(
🎉 Congratulations!

You've completed a comprehensive performance analysis using ECScope's
educational performance benchmarking suite. You've learned:

• How to benchmark ECS architectures systematically
• How to compare performance across different approaches
• How to identify and analyze performance bottlenecks
• How to set up regression testing for continuous quality
• How to interpret performance data and make optimization decisions
• How integration affects overall system performance

🚀 Next Steps:
• Apply these techniques to your own ECS implementations
• Set up continuous performance monitoring in your projects
• Experiment with different optimization strategies
• Share your performance insights with the community

Remember: Performance optimization is an iterative process.
Measure, analyze, optimize, and repeat!
)" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Demo failed with exception: {}", e.what());
        return 1;
    }
}