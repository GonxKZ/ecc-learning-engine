#pragma once

#include "test_framework.hpp"
#include "test_runner.hpp"
#include "ecs_testing.hpp"
#include "rendering_testing.hpp"
#include "physics_testing.hpp"
#include "memory_testing.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <regex>

namespace ecscope::testing {

// Test discovery system
class TestDiscovery {
public:
    static void discover_and_register_tests() {
        // Register ECS tests
        register_ecs_tests();
        
        // Register rendering tests
        register_rendering_tests();
        
        // Register physics tests
        register_physics_tests();
        
        // Register memory tests
        register_memory_tests();
        
        // Register integration tests
        register_integration_tests();
        
        // Register performance tests
        register_performance_tests();
    }

private:
    static void register_ecs_tests() {
        auto& registry = TestRegistry::instance();
        
        // Component lifecycle tests
        registry.register_test(std::make_unique<ECSMemoryFragmentationTest>());
        registry.register_test(std::make_unique<ECSConcurrencyTest>());
        
        // Create test suite for ECS
        auto ecs_suite = std::make_unique<TestSuite>("ECS Tests");
        ecs_suite->add_test<ECSMemoryFragmentationTest>();
        ecs_suite->add_test<ECSConcurrencyTest>();
        registry.register_suite(std::move(ecs_suite));
    }
    
    static void register_rendering_tests() {
        auto& registry = TestRegistry::instance();
        
        registry.register_test(std::make_unique<BasicRenderingTest>());
        registry.register_test(std::make_unique<ShaderCompilationTest>());
        registry.register_test(std::make_unique<RenderingPerformanceTest>());
        
        auto rendering_suite = std::make_unique<TestSuite>("Rendering Tests");
        rendering_suite->add_test<BasicRenderingTest>();
        rendering_suite->add_test<ShaderCompilationTest>();
        rendering_suite->add_test<RenderingPerformanceTest>();
        registry.register_suite(std::move(rendering_suite));
    }
    
    static void register_physics_tests() {
        auto& registry = TestRegistry::instance();
        
        registry.register_test(std::make_unique<PhysicsDeterminismTest>());
        registry.register_test(std::make_unique<ConservationLawsTest>());
        registry.register_test(std::make_unique<CollisionAccuracyTest>());
        registry.register_test(std::make_unique<PhysicsPerformanceTest>());
        registry.register_test(std::make_unique<PhysicsStressTest>());
        
        auto physics_suite = std::make_unique<TestSuite>("Physics Tests");
        physics_suite->add_test<PhysicsDeterminismTest>();
        physics_suite->add_test<ConservationLawsTest>();
        physics_suite->add_test<CollisionAccuracyTest>();
        physics_suite->add_test<PhysicsPerformanceTest>();
        physics_suite->add_test<PhysicsStressTest>();
        registry.register_suite(std::move(physics_suite));
    }
    
    static void register_memory_tests() {
        auto& registry = TestRegistry::instance();
        
        registry.register_test(std::make_unique<MemoryLeakTest>());
        registry.register_test(std::make_unique<MemoryFragmentationTest>());
        registry.register_test(std::make_unique<MemoryStressTest>());
        
        auto memory_suite = std::make_unique<TestSuite>("Memory Tests");
        memory_suite->add_test<MemoryLeakTest>();
        memory_suite->add_test<MemoryFragmentationTest>();
        memory_suite->add_test<MemoryStressTest>();
        registry.register_suite(std::move(memory_suite));
    }
    
    static void register_integration_tests() {
        auto& registry = TestRegistry::instance();
        
        // Integration tests would be registered here
        auto integration_suite = std::make_unique<TestSuite>("Integration Tests");
        registry.register_suite(std::move(integration_suite));
    }
    
    static void register_performance_tests() {
        auto& registry = TestRegistry::instance();
        
        // Performance regression tests
        auto performance_suite = std::make_unique<TestSuite>("Performance Tests");
        registry.register_suite(std::move(performance_suite));
    }
};

// Command line argument parser
class CommandLineParser {
public:
    struct ParsedArgs {
        bool help = false;
        bool list_tests = false;
        bool parallel = true;
        bool verbose = false;
        bool shuffle = false;
        int repeat_count = 1;
        std::string filter_pattern;
        std::vector<std::string> included_tags;
        std::vector<std::string> excluded_tags;
        std::vector<TestCategory> included_categories;
        std::vector<TestCategory> excluded_categories;
        std::string output_format = "console";
        std::string output_file;
        std::string baseline_file;
        bool stop_on_failure = false;
        int timeout_seconds = 300;
        bool enable_memory_tracking = true;
        bool enable_performance_tracking = true;
    };

    static ParsedArgs parse(int argc, char* argv[]) {
        ParsedArgs args;
        
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            
            if (arg == "--help" || arg == "-h") {
                args.help = true;
            } else if (arg == "--list-tests" || arg == "-l") {
                args.list_tests = true;
            } else if (arg == "--no-parallel") {
                args.parallel = false;
            } else if (arg == "--verbose" || arg == "-v") {
                args.verbose = true;
            } else if (arg == "--shuffle") {
                args.shuffle = true;
            } else if (arg == "--stop-on-failure") {
                args.stop_on_failure = true;
            } else if (arg == "--no-memory-tracking") {
                args.enable_memory_tracking = false;
            } else if (arg == "--no-performance-tracking") {
                args.enable_performance_tracking = false;
            } else if (arg.starts_with("--repeat=")) {
                args.repeat_count = std::stoi(arg.substr(9));
            } else if (arg.starts_with("--filter=")) {
                args.filter_pattern = arg.substr(9);
            } else if (arg.starts_with("--include-tag=")) {
                args.included_tags.push_back(arg.substr(14));
            } else if (arg.starts_with("--exclude-tag=")) {
                args.excluded_tags.push_back(arg.substr(14));
            } else if (arg.starts_with("--include-category=")) {
                args.included_categories.push_back(parse_category(arg.substr(19)));
            } else if (arg.starts_with("--exclude-category=")) {
                args.excluded_categories.push_back(parse_category(arg.substr(19)));
            } else if (arg.starts_with("--output-format=")) {
                args.output_format = arg.substr(16);
            } else if (arg.starts_with("--output-file=")) {
                args.output_file = arg.substr(14);
            } else if (arg.starts_with("--baseline-file=")) {
                args.baseline_file = arg.substr(16);
            } else if (arg.starts_with("--timeout=")) {
                args.timeout_seconds = std::stoi(arg.substr(10));
            }
        }
        
        return args;
    }

    static void print_help() {
        std::cout << "ECScope Test Runner\n";
        std::cout << "==================\n\n";
        std::cout << "Usage: test_runner [options]\n\n";
        std::cout << "Options:\n";
        std::cout << "  --help, -h               Show this help message\n";
        std::cout << "  --list-tests, -l         List all available tests\n";
        std::cout << "  --verbose, -v            Enable verbose output\n";
        std::cout << "  --no-parallel            Disable parallel test execution\n";
        std::cout << "  --shuffle                Shuffle test execution order\n";
        std::cout << "  --stop-on-failure        Stop execution on first failure\n";
        std::cout << "  --repeat=N               Repeat tests N times\n";
        std::cout << "  --filter=PATTERN         Only run tests matching regex pattern\n";
        std::cout << "  --include-tag=TAG        Include tests with specific tag\n";
        std::cout << "  --exclude-tag=TAG        Exclude tests with specific tag\n";
        std::cout << "  --include-category=CAT   Include specific test category\n";
        std::cout << "  --exclude-category=CAT   Exclude specific test category\n";
        std::cout << "  --output-format=FORMAT   Output format (console|xml|json|html)\n";
        std::cout << "  --output-file=FILE       Output file for reports\n";
        std::cout << "  --baseline-file=FILE     Performance baseline file\n";
        std::cout << "  --timeout=SECONDS        Global test timeout\n";
        std::cout << "  --no-memory-tracking     Disable memory leak detection\n";
        std::cout << "  --no-performance-tracking Disable performance monitoring\n\n";
        std::cout << "Test Categories:\n";
        std::cout << "  unit, integration, performance, memory, stress, regression\n";
        std::cout << "  rendering, physics, audio, networking, asset, ecs, multithreaded\n\n";
        std::cout << "Examples:\n";
        std::cout << "  test_runner --filter=\"Physics.*\" --verbose\n";
        std::cout << "  test_runner --include-category=performance --output-format=xml\n";
        std::cout << "  test_runner --exclude-tag=slow --parallel\n";
    }

private:
    static TestCategory parse_category(const std::string& category_str) {
        if (category_str == "unit") return TestCategory::UNIT;
        if (category_str == "integration") return TestCategory::INTEGRATION;
        if (category_str == "performance") return TestCategory::PERFORMANCE;
        if (category_str == "memory") return TestCategory::MEMORY;
        if (category_str == "stress") return TestCategory::STRESS;
        if (category_str == "regression") return TestCategory::REGRESSION;
        if (category_str == "rendering") return TestCategory::RENDERING;
        if (category_str == "physics") return TestCategory::PHYSICS;
        if (category_str == "audio") return TestCategory::AUDIO;
        if (category_str == "networking") return TestCategory::NETWORKING;
        if (category_str == "asset") return TestCategory::ASSET;
        if (category_str == "ecs") return TestCategory::ECS;
        if (category_str == "multithreaded") return TestCategory::MULTITHREADED;
        return TestCategory::UNIT; // Default
    }
};

// Test listing functionality
class TestLister {
public:
    static void list_all_tests() {
        auto& registry = TestRegistry::instance();
        
        std::cout << "Available Tests:\n";
        std::cout << "================\n\n";
        
        // List individual tests
        const auto& tests = registry.tests();
        if (!tests.empty()) {
            std::cout << "Individual Tests:\n";
            for (const auto& test : tests) {
                print_test_info(test.get());
            }
            std::cout << "\n";
        }
        
        // List test suites
        const auto& suites = registry.suites();
        if (!suites.empty()) {
            std::cout << "Test Suites:\n";
            for (const auto& suite : suites) {
                std::cout << "  " << suite->name() << " (" << suite->tests().size() << " tests)\n";
                for (const auto& test : suite->tests()) {
                    std::cout << "    ";
                    print_test_info(test.get(), false);
                }
                std::cout << "\n";
            }
        }
        
        std::cout << "Total: " << tests.size() << " individual tests, " 
                  << suites.size() << " test suites\n";
    }

private:
    static void print_test_info(TestCase* test, bool include_category = true) {
        const auto& context = test->context();
        std::cout << context.name;
        
        if (include_category) {
            std::cout << " [" << category_to_string(context.category) << "]";
        }
        
        if (!context.tags.empty()) {
            std::cout << " (tags: ";
            for (size_t i = 0; i < context.tags.size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << context.tags[i];
            }
            std::cout << ")";
        }
        
        std::cout << "\n";
    }
    
    static std::string category_to_string(TestCategory category) {
        switch (category) {
            case TestCategory::UNIT: return "Unit";
            case TestCategory::INTEGRATION: return "Integration";
            case TestCategory::PERFORMANCE: return "Performance";
            case TestCategory::MEMORY: return "Memory";
            case TestCategory::STRESS: return "Stress";
            case TestCategory::REGRESSION: return "Regression";
            case TestCategory::RENDERING: return "Rendering";
            case TestCategory::PHYSICS: return "Physics";
            case TestCategory::AUDIO: return "Audio";
            case TestCategory::NETWORKING: return "Networking";
            case TestCategory::ASSET: return "Asset";
            case TestCategory::ECS: return "ECS";
            case TestCategory::MULTITHREADED: return "Multithreaded";
            default: return "Unknown";
        }
    }
};

// Main test runner entry point
class TestMain {
public:
    static int run(int argc, char* argv[]) {
        try {
            // Parse command line arguments
            auto args = CommandLineParser::parse(argc, argv);
            
            if (args.help) {
                CommandLineParser::print_help();
                return 0;
            }
            
            // Discover and register all tests
            TestDiscovery::discover_and_register_tests();
            
            if (args.list_tests) {
                TestLister::list_all_tests();
                return 0;
            }
            
            // Configure test runner
            TestRunnerConfig config;
            config.parallel_execution = args.parallel;
            config.shuffle_tests = args.shuffle;
            config.repeat_count = args.repeat_count;
            config.included_tags = args.included_tags;
            config.excluded_tags = args.excluded_tags;
            config.included_categories = args.included_categories;
            config.excluded_categories = args.excluded_categories;
            config.filter_pattern = args.filter_pattern;
            config.stop_on_failure = args.stop_on_failure;
            config.verbose_output = args.verbose;
            config.timeout_seconds = args.timeout_seconds;
            config.output_format = args.output_format;
            config.output_file = args.output_file;
            
            // Initialize test runner
            TestRunner runner(config);
            
            if (!args.baseline_file.empty()) {
                runner.set_regression_baseline_file(args.baseline_file);
            }
            
            // Enable tracking systems
            if (args.enable_memory_tracking) {
                DetailedMemoryTracker::instance().enable_tracking();
            }
            
            // Run tests
            std::cout << "ECScope Engine Test Suite\n";
            std::cout << "========================\n\n";
            
            auto start_time = std::chrono::steady_clock::now();
            auto stats = runner.run_all_tests();
            auto end_time = std::chrono::steady_clock::now();
            
            auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            
            // Print final summary
            std::cout << "\n" << std::string(60, '=') << "\n";
            std::cout << "Final Test Results\n";
            std::cout << std::string(60, '=') << "\n";
            std::cout << "Tests run: " << stats.total_tests << "\n";
            std::cout << "Passed: " << stats.passed_tests << "\n";
            std::cout << "Failed: " << stats.failed_tests << "\n";
            std::cout << "Errors: " << stats.error_tests << "\n";
            std::cout << "Skipped: " << stats.skipped_tests << "\n";
            std::cout << "Success rate: " << std::fixed << std::setprecision(1) << stats.pass_rate() << "%\n";
            std::cout << "Total time: " << total_duration.count() << "ms\n";
            
            // Memory tracking summary
            if (args.enable_memory_tracking) {
                auto memory_stats = DetailedMemoryTracker::instance().get_statistics();
                std::cout << "\nMemory Summary:\n";
                std::cout << "  Peak usage: " << memory_stats.peak_usage << " bytes\n";
                std::cout << "  Total allocations: " << memory_stats.allocation_count << "\n";
                std::cout << "  Memory leaks: " << memory_stats.leaked_allocations << "\n";
                
                if (memory_stats.leaked_allocations > 0) {
                    std::cout << "  WARNING: Memory leaks detected!\n";
                }
            }
            
            std::cout << std::string(60, '=') << "\n";
            
            // Return appropriate exit code
            if (stats.failed_tests > 0 || stats.error_tests > 0) {
                return 1; // Test failures
            }
            
            return 0; // Success
            
        } catch (const std::exception& e) {
            std::cerr << "Error running tests: " << e.what() << std::endl;
            return 2; // Runtime error
        } catch (...) {
            std::cerr << "Unknown error running tests" << std::endl;
            return 2; // Unknown error
        }
    }
};

} // namespace ecscope::testing

// Macro to define main function for test executable
#define ECSCOPE_TEST_MAIN() \
    int main(int argc, char* argv[]) { \
        return ecscope::testing::TestMain::run(argc, argv); \
    }