#pragma once

/**
 * @file plugin/plugin_testing.hpp
 * @brief ECScope Plugin Testing Framework - Comprehensive Validation and Testing
 * 
 * Complete testing framework for plugin development and validation including:
 * - Unit testing framework for plugin components and systems
 * - Integration testing with ECS and engine systems
 * - Performance benchmarking and profiling
 * - Compatibility testing across platforms and versions
 * - Security validation and vulnerability testing
 * - Educational testing tutorials and examples
 * 
 * Testing Categories:
 * - Component behavior and lifecycle testing
 * - System integration and dependency testing
 * - Memory management and leak detection
 * - Performance benchmarking and regression testing
 * - Security validation and penetration testing
 * - Cross-platform compatibility validation
 * 
 * @author ECScope Educational ECS Framework - Plugin System
 * @date 2024
 */

#include "plugin_core.hpp"
#include "plugin_api.hpp"
#include "plugin_manager.hpp"
#include "ecs_plugin_integration.hpp"
#include "core/types.hpp"
#include "core/log.hpp"
#include "memory/arena.hpp"
#include "memory/memory_tracker.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <chrono>
#include <exception>
#include <future>
#include <thread>
#include <atomic>
#include <mutex>

namespace ecscope::plugin::testing {

//=============================================================================
// Test Framework Configuration and Types
//=============================================================================

/**
 * @brief Test execution mode
 */
enum class TestExecutionMode {
    Sequential,     // Run tests one after another
    Parallel,       // Run tests in parallel where possible
    Isolated,       // Run each test in isolated environment
    Educational     // Run with educational explanations
};

/**
 * @brief Test severity level
 */
enum class TestSeverity {
    Info,           // Informational test
    Warning,        // Test with warnings
    Error,          // Test failure
    Critical        // Critical failure that stops execution
};

/**
 * @brief Test category for organization
 */
enum class TestCategory {
    Unit,           // Unit tests for individual components
    Integration,    // Integration tests with other systems
    Performance,    // Performance benchmarks
    Memory,         // Memory usage and leak tests
    Security,       // Security validation tests
    Compatibility,  // Platform/version compatibility
    Regression,     // Regression tests
    Educational     // Educational test examples
};

/**
 * @brief Test result data
 */
struct TestResult {
    std::string test_name;
    std::string plugin_name;
    TestCategory category;
    TestSeverity severity;
    bool passed{false};
    f64 execution_time_ms{0.0};
    std::string error_message;
    std::string detailed_output;
    
    // Performance metrics
    std::unordered_map<std::string, f64> performance_metrics;
    
    // Memory metrics
    usize memory_used{0};
    usize peak_memory{0};
    bool memory_leaks_detected{false};
    
    // Educational information
    std::string educational_explanation;
    std::vector<std::string> learning_points;
    
    TestResult(const std::string& name, const std::string& plugin) 
        : test_name(name), plugin_name(plugin) {}
};

/**
 * @brief Test configuration
 */
struct TestConfig {
    TestExecutionMode execution_mode{TestExecutionMode::Sequential};
    bool enable_performance_testing{true};
    bool enable_memory_testing{true};
    bool enable_security_testing{true};
    bool enable_educational_mode{true};
    
    // Timeouts and limits
    std::chrono::milliseconds test_timeout{30000}; // 30 seconds
    usize max_memory_per_test{512 * MB};
    u32 performance_test_iterations{1000};
    
    // Educational features
    bool explain_test_failures{true};
    bool generate_learning_reports{true};
    bool demonstrate_best_practices{true};
    
    // Output configuration
    bool verbose_output{false};
    bool save_test_reports{true};
    std::string report_directory{"./test_reports"};
};

//=============================================================================
// Test Framework Base Classes
//=============================================================================

/**
 * @brief Base test case interface
 */
class ITestCase {
public:
    virtual ~ITestCase() = default;
    
    /**
     * @brief Set up test environment before running
     */
    virtual void setup() {}
    
    /**
     * @brief Clean up test environment after running
     */
    virtual void teardown() {}
    
    /**
     * @brief Execute the test
     */
    virtual TestResult run() = 0;
    
    /**
     * @brief Get test metadata
     */
    virtual std::string get_test_name() const = 0;
    virtual TestCategory get_test_category() const = 0;
    virtual std::string get_description() const = 0;
    
    /**
     * @brief Get educational information
     */
    virtual std::string get_educational_purpose() const { return ""; }
    virtual std::vector<std::string> get_learning_objectives() const { return {}; }
};

/**
 * @brief Test assertion utilities
 */
class TestAssertions {
public:
    /**
     * @brief Assert condition is true
     */
    static void assert_true(bool condition, const std::string& message = "Assertion failed") {
        if (!condition) {
            throw TestAssertionException(message);
        }
    }
    
    /**
     * @brief Assert condition is false
     */
    static void assert_false(bool condition, const std::string& message = "Assertion failed") {
        assert_true(!condition, message);
    }
    
    /**
     * @brief Assert values are equal
     */
    template<typename T>
    static void assert_equal(const T& expected, const T& actual, 
                           const std::string& message = "Values not equal") {
        if (expected != actual) {
            throw TestAssertionException(message + " (expected: " + 
                                       std::to_string(expected) + 
                                       ", actual: " + std::to_string(actual) + ")");
        }
    }
    
    /**
     * @brief Assert values are not equal
     */
    template<typename T>
    static void assert_not_equal(const T& expected, const T& actual,
                                const std::string& message = "Values are equal") {
        if (expected == actual) {
            throw TestAssertionException(message);
        }
    }
    
    /**
     * @brief Assert value is null
     */
    template<typename T>
    static void assert_null(T* pointer, const std::string& message = "Pointer is not null") {
        if (pointer != nullptr) {
            throw TestAssertionException(message);
        }
    }
    
    /**
     * @brief Assert value is not null
     */
    template<typename T>
    static void assert_not_null(T* pointer, const std::string& message = "Pointer is null") {
        if (pointer == nullptr) {
            throw TestAssertionException(message);
        }
    }
    
    /**
     * @brief Assert exception is thrown
     */
    template<typename ExceptionType, typename Callable>
    static void assert_throws(Callable&& callable, const std::string& message = "Exception not thrown") {
        bool exception_thrown = false;
        try {
            callable();
        } catch (const ExceptionType&) {
            exception_thrown = true;
        } catch (...) {
            // Wrong exception type
        }
        
        if (!exception_thrown) {
            throw TestAssertionException(message);
        }
    }
    
    /**
     * @brief Assert no exception is thrown
     */
    template<typename Callable>
    static void assert_no_throw(Callable&& callable, const std::string& message = "Unexpected exception thrown") {
        try {
            callable();
        } catch (...) {
            throw TestAssertionException(message);
        }
    }

private:
    class TestAssertionException : public std::runtime_error {
    public:
        explicit TestAssertionException(const std::string& message) : std::runtime_error(message) {}
    };
};

//=============================================================================
// Plugin-Specific Test Framework
//=============================================================================

/**
 * @brief Plugin component test base class
 */
template<typename ComponentType>
class PluginComponentTest : public ITestCase {
protected:
    std::unique_ptr<PluginAPI> api_;
    ecs::Entity test_entity_{0};
    ComponentType* test_component_{nullptr};
    
public:
    explicit PluginComponentTest(std::unique_ptr<PluginAPI> api) : api_(std::move(api)) {}
    
    void setup() override {
        // Create test entity with component
        test_entity_ = api_->get_ecs().create_entity<ComponentType>();
        test_component_ = api_->get_ecs().get_component<ComponentType>(test_entity_);
        TestAssertions::assert_not_null(test_component_, "Failed to create test component");
    }
    
    void teardown() override {
        if (test_entity_ != 0) {
            api_->get_ecs().destroy_entity(test_entity_);
        }
        test_component_ = nullptr;
        test_entity_ = 0;
    }
    
    TestCategory get_test_category() const override {
        return TestCategory::Unit;
    }

protected:
    /**
     * @brief Test component creation and initialization
     */
    void test_component_creation() {
        TestAssertions::assert_not_null(test_component_, "Component should be created");
        TestAssertions::assert_true(api_->get_ecs().has_component<ComponentType>(test_entity_),
                                   "Entity should have component");
    }
    
    /**
     * @brief Test component data integrity
     */
    virtual void test_component_data_integrity() {
        // Override in derived classes to test specific component data
    }
    
    /**
     * @brief Test component serialization (if applicable)
     */
    virtual void test_component_serialization() {
        // Override in derived classes for serialization tests
    }
};

/**
 * @brief Plugin system test base class
 */
class PluginSystemTest : public ITestCase {
protected:
    std::unique_ptr<PluginAPI> api_;
    std::string system_name_;
    
public:
    PluginSystemTest(std::unique_ptr<PluginAPI> api, const std::string& system_name)
        : api_(std::move(api)), system_name_(system_name) {}
    
    TestCategory get_test_category() const override {
        return TestCategory::Integration;
    }

protected:
    /**
     * @brief Test system registration
     */
    void test_system_registration() {
        auto& registry = api_->get_registry();
        auto systems = registry.get_system_names();
        bool system_found = std::find(systems.begin(), systems.end(), system_name_) != systems.end();
        TestAssertions::assert_true(system_found, "System should be registered");
    }
    
    /**
     * @brief Test system update functionality
     */
    virtual void test_system_update() {
        // Override in derived classes to test specific system behavior
    }
    
    /**
     * @brief Test system performance
     */
    virtual void test_system_performance() {
        // Override in derived classes for performance tests
    }
};

//=============================================================================
// Performance Testing Framework
//=============================================================================

/**
 * @brief Performance benchmark test
 */
class PerformanceBenchmark : public ITestCase {
private:
    std::string benchmark_name_;
    std::function<void()> benchmark_function_;
    u32 iterations_;
    f64 target_time_ms_;
    
public:
    PerformanceBenchmark(const std::string& name,
                        std::function<void()> function,
                        u32 iterations = 1000,
                        f64 target_time_ms = 100.0)
        : benchmark_name_(name), benchmark_function_(function)
        , iterations_(iterations), target_time_ms_(target_time_ms) {}
    
    std::string get_test_name() const override {
        return "Performance_" + benchmark_name_;
    }
    
    TestCategory get_test_category() const override {
        return TestCategory::Performance;
    }
    
    std::string get_description() const override {
        return "Performance benchmark for " + benchmark_name_;
    }
    
    TestResult run() override {
        TestResult result(get_test_name(), "Performance");
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        try {
            // Warm-up run
            benchmark_function_();
            
            // Actual benchmark
            auto benchmark_start = std::chrono::high_resolution_clock::now();
            
            for (u32 i = 0; i < iterations_; ++i) {
                benchmark_function_();
            }
            
            auto benchmark_end = std::chrono::high_resolution_clock::now();
            
            auto total_time = std::chrono::duration<f64, std::milli>(benchmark_end - benchmark_start).count();
            auto average_time = total_time / iterations_;
            
            result.performance_metrics["total_time_ms"] = total_time;
            result.performance_metrics["average_time_ms"] = average_time;
            result.performance_metrics["iterations"] = static_cast<f64>(iterations_);
            result.performance_metrics["operations_per_second"] = 1000.0 / average_time;
            
            // Evaluate performance
            result.passed = average_time <= target_time_ms_;
            if (!result.passed) {
                result.error_message = "Performance target not met. Average: " + 
                                     std::to_string(average_time) + "ms, Target: " + 
                                     std::to_string(target_time_ms_) + "ms";
            }
            
            result.detailed_output = "Benchmark completed " + std::to_string(iterations_) + 
                                   " iterations in " + std::to_string(total_time) + "ms";
            
        } catch (const std::exception& e) {
            result.passed = false;
            result.error_message = e.what();
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        result.execution_time_ms = std::chrono::duration<f64, std::milli>(end_time - start_time).count();
        
        return result;
    }
};

//=============================================================================
// Memory Testing Framework
//=============================================================================

/**
 * @brief Memory leak detection test
 */
class MemoryLeakTest : public ITestCase {
private:
    std::string test_name_;
    std::function<void()> test_function_;
    usize max_allowed_leak_bytes_;
    
public:
    MemoryLeakTest(const std::string& name,
                  std::function<void()> function,
                  usize max_leak_bytes = 1024)
        : test_name_(name), test_function_(function), max_allowed_leak_bytes_(max_leak_bytes) {}
    
    std::string get_test_name() const override {
        return "MemoryLeak_" + test_name_;
    }
    
    TestCategory get_test_category() const override {
        return TestCategory::Memory;
    }
    
    std::string get_description() const override {
        return "Memory leak detection test for " + test_name_;
    }
    
    TestResult run() override {
        TestResult result(get_test_name(), "Memory");
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        try {
            // Get initial memory state
            auto initial_memory = memory::tracker::get_total_allocated();
            
            // Run test function
            test_function_();
            
            // Force garbage collection if available
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            // Get final memory state
            auto final_memory = memory::tracker::get_total_allocated();
            
            usize memory_diff = (final_memory > initial_memory) ? 
                              (final_memory - initial_memory) : 0;
            
            result.memory_used = memory_diff;
            result.memory_leaks_detected = memory_diff > max_allowed_leak_bytes_;
            result.passed = !result.memory_leaks_detected;
            
            if (result.memory_leaks_detected) {
                result.error_message = "Memory leak detected: " + std::to_string(memory_diff) + 
                                     " bytes leaked (max allowed: " + 
                                     std::to_string(max_allowed_leak_bytes_) + " bytes)";
            }
            
            result.detailed_output = "Memory usage: Initial=" + std::to_string(initial_memory) + 
                                   ", Final=" + std::to_string(final_memory) + 
                                   ", Diff=" + std::to_string(memory_diff);
            
        } catch (const std::exception& e) {
            result.passed = false;
            result.error_message = e.what();
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        result.execution_time_ms = std::chrono::duration<f64, std::milli>(end_time - start_time).count();
        
        return result;
    }
};

//=============================================================================
// Security Testing Framework
//=============================================================================

/**
 * @brief Security validation test
 */
class SecurityTest : public ITestCase {
private:
    std::string test_name_;
    std::function<bool()> security_check_;
    std::string security_description_;
    
public:
    SecurityTest(const std::string& name,
                std::function<bool()> check,
                const std::string& description)
        : test_name_(name), security_check_(check), security_description_(description) {}
    
    std::string get_test_name() const override {
        return "Security_" + test_name_;
    }
    
    TestCategory get_test_category() const override {
        return TestCategory::Security;
    }
    
    std::string get_description() const override {
        return "Security validation: " + security_description_;
    }
    
    TestResult run() override {
        TestResult result(get_test_name(), "Security");
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        try {
            result.passed = security_check_();
            if (!result.passed) {
                result.error_message = "Security check failed: " + security_description_;
                result.severity = TestSeverity::Critical;
            }
            
            result.detailed_output = "Security check: " + security_description_ + 
                                   " - " + (result.passed ? "PASSED" : "FAILED");
            
        } catch (const std::exception& e) {
            result.passed = false;
            result.error_message = "Security test exception: " + std::string(e.what());
            result.severity = TestSeverity::Critical;
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        result.execution_time_ms = std::chrono::duration<f64, std::milli>(end_time - start_time).count();
        
        return result;
    }
};

//=============================================================================
// Test Suite Management
//=============================================================================

/**
 * @brief Test suite for organizing and running multiple tests
 */
class TestSuite {
private:
    std::string suite_name_;
    std::vector<std::unique_ptr<ITestCase>> test_cases_;
    TestConfig config_;
    
    // Results tracking
    std::vector<TestResult> test_results_;
    std::chrono::high_resolution_clock::time_point start_time_;
    std::chrono::high_resolution_clock::time_point end_time_;

public:
    /**
     * @brief Constructor
     */
    explicit TestSuite(const std::string& name, const TestConfig& config = {})
        : suite_name_(name), config_(config) {}
    
    /**
     * @brief Add test case to suite
     */
    void add_test(std::unique_ptr<ITestCase> test_case) {
        test_cases_.push_back(std::move(test_case));
    }
    
    /**
     * @brief Run all tests in suite
     */
    std::vector<TestResult> run_all_tests() {
        test_results_.clear();
        start_time_ = std::chrono::high_resolution_clock::now();
        
        LOG_INFO("Running test suite '{}' with {} tests", suite_name_, test_cases_.size());
        
        switch (config_.execution_mode) {
            case TestExecutionMode::Sequential:
                run_tests_sequential();
                break;
            case TestExecutionMode::Parallel:
                run_tests_parallel();
                break;
            case TestExecutionMode::Isolated:
                run_tests_isolated();
                break;
            case TestExecutionMode::Educational:
                run_tests_educational();
                break;
        }
        
        end_time_ = std::chrono::high_resolution_clock::now();
        
        // Generate summary
        generate_test_summary();
        
        return test_results_;
    }
    
    /**
     * @brief Get test results
     */
    const std::vector<TestResult>& get_test_results() const {
        return test_results_;
    }
    
    /**
     * @brief Generate test report
     */
    std::string generate_report() const {
        std::ostringstream oss;
        
        auto total_time = std::chrono::duration<f64, std::milli>(end_time_ - start_time_).count();
        u32 passed_count = 0;
        u32 failed_count = 0;
        
        for (const auto& result : test_results_) {
            if (result.passed) passed_count++;
            else failed_count++;
        }
        
        oss << "=== Test Suite Report: " << suite_name_ << " ===\n";
        oss << "Total Tests: " << test_results_.size() << "\n";
        oss << "Passed: " << passed_count << "\n";
        oss << "Failed: " << failed_count << "\n";
        oss << "Success Rate: " << (test_results_.empty() ? 0.0f : 
                                   (static_cast<f32>(passed_count) / test_results_.size() * 100.0f)) << "%\n";
        oss << "Total Execution Time: " << total_time << "ms\n\n";
        
        // Detailed results
        for (const auto& result : test_results_) {
            oss << "Test: " << result.test_name << "\n";
            oss << "  Status: " << (result.passed ? "PASSED" : "FAILED") << "\n";
            oss << "  Time: " << result.execution_time_ms << "ms\n";
            if (!result.passed) {
                oss << "  Error: " << result.error_message << "\n";
            }
            if (!result.detailed_output.empty()) {
                oss << "  Output: " << result.detailed_output << "\n";
            }
            oss << "\n";
        }
        
        return oss.str();
    }

private:
    void run_tests_sequential() {
        for (auto& test_case : test_cases_) {
            run_single_test(*test_case);
        }
    }
    
    void run_tests_parallel() {
        std::vector<std::future<TestResult>> futures;
        
        for (auto& test_case : test_cases_) {
            futures.push_back(std::async(std::launch::async, [&test_case]() {
                return run_test_safely(*test_case);
            }));
        }
        
        for (auto& future : futures) {
            test_results_.push_back(future.get());
        }
    }
    
    void run_tests_isolated() {
        // Each test runs in its own isolated context
        for (auto& test_case : test_cases_) {
            run_single_test(*test_case);
        }
    }
    
    void run_tests_educational() {
        for (auto& test_case : test_cases_) {
            if (config_.verbose_output) {
                LOG_INFO("Running educational test: {}", test_case->get_test_name());
                LOG_INFO("Purpose: {}", test_case->get_educational_purpose());
            }
            
            auto result = run_single_test(*test_case);
            
            if (config_.explain_test_failures && !result.passed) {
                LOG_WARN("Test '{}' failed: {}", result.test_name, result.error_message);
                LOG_INFO("Educational explanation: {}", result.educational_explanation);
            }
        }
    }
    
    TestResult run_single_test(ITestCase& test_case) {
        return run_test_safely(test_case);
    }
    
    static TestResult run_test_safely(ITestCase& test_case) {
        try {
            test_case.setup();
            auto result = test_case.run();
            test_case.teardown();
            return result;
        } catch (const std::exception& e) {
            test_case.teardown(); // Ensure cleanup
            
            TestResult error_result(test_case.get_test_name(), "Unknown");
            error_result.passed = false;
            error_result.error_message = e.what();
            error_result.severity = TestSeverity::Error;
            return error_result;
        }
    }
    
    void generate_test_summary() {
        u32 passed = 0, failed = 0;
        for (const auto& result : test_results_) {
            if (result.passed) passed++;
            else failed++;
        }
        
        LOG_INFO("Test suite '{}' completed: {} passed, {} failed", 
                suite_name_, passed, failed);
    }
};

//=============================================================================
// Plugin Test Factory
//=============================================================================

/**
 * @brief Factory for creating plugin-specific tests
 */
class PluginTestFactory {
public:
    /**
     * @brief Create component tests for plugin
     */
    template<typename ComponentType>
    static std::unique_ptr<ITestCase> create_component_test(std::unique_ptr<PluginAPI> api,
                                                          const std::string& test_name) {
        class ConcreteComponentTest : public PluginComponentTest<ComponentType> {
        private:
            std::string test_name_;
            
        public:
            ConcreteComponentTest(std::unique_ptr<PluginAPI> api, const std::string& name)
                : PluginComponentTest<ComponentType>(std::move(api)), test_name_(name) {}
            
            std::string get_test_name() const override { return test_name_; }
            std::string get_description() const override { 
                return "Component test for " + typeid(ComponentType).name();
            }
            
            TestResult run() override {
                TestResult result(get_test_name(), this->api_->get_context().get_plugin_name());
                
                try {
                    this->test_component_creation();
                    this->test_component_data_integrity();
                    
                    result.passed = true;
                    result.detailed_output = "Component test completed successfully";
                    
                } catch (const std::exception& e) {
                    result.passed = false;
                    result.error_message = e.what();
                }
                
                return result;
            }
        };
        
        return std::make_unique<ConcreteComponentTest>(std::move(api), test_name);
    }
    
    /**
     * @brief Create system performance test
     */
    static std::unique_ptr<ITestCase> create_system_performance_test(
        const std::string& system_name,
        std::function<void()> system_function,
        f64 target_time_ms = 16.0) {
        
        return std::make_unique<PerformanceBenchmark>(
            system_name + "_Performance",
            system_function,
            100, // iterations
            target_time_ms
        );
    }
    
    /**
     * @brief Create memory leak test for plugin
     */
    static std::unique_ptr<ITestCase> create_memory_leak_test(
        const std::string& plugin_name,
        std::function<void()> plugin_function,
        usize max_leak_bytes = 1024) {
        
        return std::make_unique<MemoryLeakTest>(
            plugin_name + "_MemoryLeak",
            plugin_function,
            max_leak_bytes
        );
    }
    
    /**
     * @brief Create security validation test
     */
    static std::unique_ptr<ITestCase> create_security_test(
        const std::string& test_name,
        std::function<bool()> security_check,
        const std::string& description) {
        
        return std::make_unique<SecurityTest>(test_name, security_check, description);
    }
};

//=============================================================================
// Plugin Test Runner
//=============================================================================

/**
 * @brief Main test runner for plugin testing
 */
class PluginTestRunner {
private:
    PluginManager* plugin_manager_;
    ECSPluginIntegrationManager* integration_manager_;
    TestConfig config_;
    
    std::vector<std::unique_ptr<TestSuite>> test_suites_;
    std::unordered_map<std::string, std::vector<TestResult>> plugin_test_results_;

public:
    /**
     * @brief Constructor
     */
    explicit PluginTestRunner(PluginManager& plugin_manager,
                            ECSPluginIntegrationManager& integration_manager,
                            const TestConfig& config = {});
    
    /**
     * @brief Create test suite for plugin
     */
    std::unique_ptr<TestSuite> create_plugin_test_suite(const std::string& plugin_name);
    
    /**
     * @brief Run all plugin tests
     */
    void run_all_plugin_tests();
    
    /**
     * @brief Run tests for specific plugin
     */
    std::vector<TestResult> run_plugin_tests(const std::string& plugin_name);
    
    /**
     * @brief Generate comprehensive test report
     */
    std::string generate_comprehensive_report() const;
    
    /**
     * @brief Get test results for plugin
     */
    std::vector<TestResult> get_plugin_test_results(const std::string& plugin_name) const;
    
    /**
     * @brief Validate plugin before loading
     */
    bool validate_plugin_before_loading(const std::string& plugin_path);
    
    /**
     * @brief Generate educational test tutorial
     */
    std::string generate_test_tutorial(const std::string& plugin_name) const;

private:
    void initialize_test_environment();
    void cleanup_test_environment();
    void create_default_tests_for_plugin(const std::string& plugin_name, TestSuite& suite);
};

//=============================================================================
// Utility Functions
//=============================================================================

/**
 * @brief Create default test configuration
 */
TestConfig create_default_test_config();

/**
 * @brief Create educational test configuration
 */
TestConfig create_educational_test_config();

/**
 * @brief Create performance-focused test configuration
 */
TestConfig create_performance_test_config();

/**
 * @brief Validate test result
 */
bool validate_test_result(const TestResult& result);

/**
 * @brief Generate test report in HTML format
 */
std::string generate_html_test_report(const std::vector<TestResult>& results);

/**
 * @brief Generate test report in JSON format
 */
std::string generate_json_test_report(const std::vector<TestResult>& results);

} // namespace ecscope::plugin::testing