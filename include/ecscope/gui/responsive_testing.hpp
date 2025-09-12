/**
 * @file responsive_testing.hpp
 * @brief ECScope Responsive Design Testing Framework
 * 
 * Comprehensive testing framework for responsive design systems, including
 * automated UI testing across different screen sizes, DPI settings, and
 * touch interfaces. Provides visual regression testing and accessibility
 * validation for professional game engine interfaces.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>

#include "responsive_design.hpp"

#ifdef ECSCOPE_HAS_IMGUI
#include <imgui.h>
#endif

namespace ecscope::gui::testing {

// =============================================================================
// ENUMERATIONS & TYPES
// =============================================================================

/**
 * @brief Test result status
 */
enum class TestResult {
    Pass,
    Fail,
    Warning,
    Skip
};

/**
 * @brief Test categories for organization
 */
enum class TestCategory {
    Layout,         // Layout responsiveness tests
    Typography,     // Font scaling and readability tests
    Interaction,    // Touch and mouse interaction tests
    Performance,    // Rendering performance tests
    Accessibility,  // Accessibility compliance tests
    Visual,         // Visual regression tests
    Integration     // Integration tests
};

/**
 * @brief Screen simulation parameters
 */
struct ScreenSimulation {
    int width = 1920;
    int height = 1080;
    float dpi_scale = 1.0f;
    ScreenSize screen_size = ScreenSize::Large;
    TouchMode touch_mode = TouchMode::Disabled;
    std::string name = "Default Screen";
    bool simulate_touch = false;
};

/**
 * @brief Test configuration
 */
struct TestConfig {
    bool enable_visual_regression = true;
    bool enable_performance_testing = true;
    bool enable_accessibility_testing = true;
    bool generate_screenshots = true;
    std::string output_directory = "responsive_test_output";
    float tolerance = 0.01f; // Tolerance for numeric comparisons
    int max_test_duration_ms = 5000; // Maximum test duration
};

/**
 * @brief Individual test case information
 */
struct TestCase {
    std::string name;
    std::string description;
    TestCategory category;
    std::vector<ScreenSimulation> screen_tests;
    std::function<TestResult(const ScreenSimulation&)> test_function;
    bool enabled = true;
    std::vector<std::string> dependencies;
};

/**
 * @brief Test execution result
 */
struct TestExecutionResult {
    std::string test_name;
    TestCategory category;
    ScreenSimulation screen_config;
    TestResult result;
    std::string message;
    std::chrono::milliseconds execution_time;
    std::string screenshot_path;
    std::unordered_map<std::string, float> metrics;
};

/**
 * @brief Test suite summary
 */
struct TestSuiteSummary {
    int total_tests = 0;
    int passed_tests = 0;
    int failed_tests = 0;
    int warning_tests = 0;
    int skipped_tests = 0;
    std::chrono::milliseconds total_execution_time;
    std::vector<TestExecutionResult> results;
};

// =============================================================================
// RESPONSIVE TESTING FRAMEWORK
// =============================================================================

/**
 * @brief Responsive design testing framework
 * 
 * Provides comprehensive testing capabilities for responsive UI systems,
 * including automated testing across screen sizes, DPI scales, and touch
 * interfaces with visual regression and performance testing.
 */
class ResponsiveTestingFramework {
public:
    ResponsiveTestingFramework();
    ~ResponsiveTestingFramework();

    // Non-copyable
    ResponsiveTestingFramework(const ResponsiveTestingFramework&) = delete;
    ResponsiveTestingFramework& operator=(const ResponsiveTestingFramework&) = delete;

    // =============================================================================
    // INITIALIZATION & CONFIGURATION
    // =============================================================================

    /**
     * @brief Initialize the testing framework
     * @param responsive_manager Responsive design manager to test
     * @param config Test configuration
     * @return true on success
     */
    bool initialize(ResponsiveDesignManager* responsive_manager, const TestConfig& config = TestConfig{});

    /**
     * @brief Shutdown the testing framework
     */
    void shutdown();

    /**
     * @brief Set test configuration
     */
    void set_config(const TestConfig& config) { config_ = config; }

    /**
     * @brief Get current test configuration
     */
    const TestConfig& get_config() const { return config_; }

    // =============================================================================
    // TEST CASE MANAGEMENT
    // =============================================================================

    /**
     * @brief Register a test case
     * @param test_case Test case to register
     */
    void register_test(const TestCase& test_case);

    /**
     * @brief Remove a test case
     * @param name Test case name
     */
    void unregister_test(const std::string& name);

    /**
     * @brief Enable/disable a test case
     * @param name Test case name
     * @param enabled Enable state
     */
    void set_test_enabled(const std::string& name, bool enabled);

    /**
     * @brief Get all registered test cases
     */
    const std::vector<TestCase>& get_test_cases() const { return test_cases_; }

    /**
     * @brief Get test cases by category
     */
    std::vector<TestCase> get_test_cases_by_category(TestCategory category) const;

    // =============================================================================
    // SCREEN SIMULATIONS
    // =============================================================================

    /**
     * @brief Create standard screen simulation presets
     */
    std::vector<ScreenSimulation> create_standard_screen_simulations();

    /**
     * @brief Add custom screen simulation
     */
    void add_screen_simulation(const ScreenSimulation& simulation);

    /**
     * @brief Apply screen simulation
     * @param simulation Screen parameters to simulate
     */
    bool apply_screen_simulation(const ScreenSimulation& simulation);

    /**
     * @brief Reset to original screen settings
     */
    void reset_screen_simulation();

    // =============================================================================
    // TEST EXECUTION
    // =============================================================================

    /**
     * @brief Run all enabled tests
     * @return Test suite summary
     */
    TestSuiteSummary run_all_tests();

    /**
     * @brief Run tests by category
     * @param category Test category to run
     * @return Test suite summary
     */
    TestSuiteSummary run_tests_by_category(TestCategory category);

    /**
     * @brief Run a specific test
     * @param test_name Test name to run
     * @return Test execution results
     */
    std::vector<TestExecutionResult> run_test(const std::string& test_name);

    /**
     * @brief Run tests matching a pattern
     * @param pattern Test name pattern (supports wildcards)
     * @return Test suite summary
     */
    TestSuiteSummary run_tests_matching(const std::string& pattern);

    // =============================================================================
    // VISUAL REGRESSION TESTING
    // =============================================================================

    /**
     * @brief Capture screenshot for visual testing
     * @param test_name Test name for file naming
     * @param screen_config Screen configuration
     * @return Screenshot file path
     */
    std::string capture_screenshot(const std::string& test_name, const ScreenSimulation& screen_config);

    /**
     * @brief Compare screenshots for visual regression
     * @param reference_path Reference screenshot path
     * @param current_path Current screenshot path
     * @param tolerance Comparison tolerance (0.0-1.0)
     * @return Comparison result and difference percentage
     */
    std::pair<bool, float> compare_screenshots(const std::string& reference_path, 
                                              const std::string& current_path, 
                                              float tolerance = 0.05f);

    /**
     * @brief Generate baseline screenshots
     */
    bool generate_baseline_screenshots();

    /**
     * @brief Update baseline screenshots
     */
    bool update_baseline_screenshots();

    // =============================================================================
    // PERFORMANCE TESTING
    // =============================================================================

    /**
     * @brief Measure rendering performance
     * @param screen_config Screen configuration
     * @param duration_ms Test duration in milliseconds
     * @return Performance metrics
     */
    std::unordered_map<std::string, float> measure_rendering_performance(const ScreenSimulation& screen_config, 
                                                                        int duration_ms = 1000);

    /**
     * @brief Measure layout performance
     * @param screen_config Screen configuration
     * @return Layout timing metrics
     */
    std::unordered_map<std::string, float> measure_layout_performance(const ScreenSimulation& screen_config);

    /**
     * @brief Benchmark screen size transitions
     * @return Transition performance metrics
     */
    std::unordered_map<std::string, float> benchmark_screen_transitions();

    // =============================================================================
    // ACCESSIBILITY TESTING
    // =============================================================================

    /**
     * @brief Test touch target sizes
     * @param screen_config Screen configuration
     * @return Accessibility test result
     */
    TestResult test_touch_target_sizes(const ScreenSimulation& screen_config);

    /**
     * @brief Test color contrast ratios
     * @param screen_config Screen configuration
     * @return Accessibility test result
     */
    TestResult test_color_contrast(const ScreenSimulation& screen_config);

    /**
     * @brief Test keyboard navigation
     * @param screen_config Screen configuration
     * @return Accessibility test result
     */
    TestResult test_keyboard_navigation(const ScreenSimulation& screen_config);

    /**
     * @brief Test screen reader compatibility
     * @param screen_config Screen configuration
     * @return Accessibility test result
     */
    TestResult test_screen_reader_compatibility(const ScreenSimulation& screen_config);

    // =============================================================================
    // REPORT GENERATION
    // =============================================================================

    /**
     * @brief Generate HTML test report
     * @param summary Test suite summary
     * @param output_path Output file path
     * @return true on success
     */
    bool generate_html_report(const TestSuiteSummary& summary, const std::string& output_path);

    /**
     * @brief Generate JSON test report
     * @param summary Test suite summary
     * @param output_path Output file path
     * @return true on success
     */
    bool generate_json_report(const TestSuiteSummary& summary, const std::string& output_path);

    /**
     * @brief Generate CSV test report
     * @param summary Test suite summary
     * @param output_path Output file path
     * @return true on success
     */
    bool generate_csv_report(const TestSuiteSummary& summary, const std::string& output_path);

    // =============================================================================
    // UTILITY FUNCTIONS
    // =============================================================================

    /**
     * @brief Get test result as string
     */
    static std::string test_result_to_string(TestResult result);

    /**
     * @brief Get test category as string
     */
    static std::string test_category_to_string(TestCategory category);

    /**
     * @brief Check if test name matches pattern
     */
    static bool matches_pattern(const std::string& name, const std::string& pattern);

private:
    // =============================================================================
    // PRIVATE METHODS
    // =============================================================================

    TestExecutionResult execute_test_case(const TestCase& test_case, const ScreenSimulation& screen_config);
    bool setup_output_directory();
    std::string generate_screenshot_path(const std::string& test_name, const ScreenSimulation& screen_config);
    bool check_dependencies(const std::vector<std::string>& dependencies);
    void log_test_result(const TestExecutionResult& result);

    // Built-in test implementations
    void register_built_in_tests();
    TestResult test_layout_responsiveness(const ScreenSimulation& screen_config);
    TestResult test_font_scaling(const ScreenSimulation& screen_config);
    TestResult test_spacing_consistency(const ScreenSimulation& screen_config);
    TestResult test_touch_interactions(const ScreenSimulation& screen_config);
    TestResult test_dpi_scaling_accuracy(const ScreenSimulation& screen_config);

    // =============================================================================
    // MEMBER VARIABLES
    // =============================================================================

    bool initialized_ = false;
    TestConfig config_;
    ResponsiveDesignManager* responsive_manager_ = nullptr;

    // Test management
    std::vector<TestCase> test_cases_;
    std::vector<ScreenSimulation> screen_simulations_;

    // Original settings for restoration
    ScreenSimulation original_screen_config_;
    bool has_original_config_ = false;

    // Performance tracking
    std::chrono::steady_clock::time_point test_start_time_;
    
    // Results tracking
    std::vector<TestExecutionResult> current_results_;
};

// =============================================================================
// AUTOMATED TEST BUILDERS
// =============================================================================

/**
 * @brief Builder for layout responsiveness tests
 */
class LayoutTestBuilder {
public:
    LayoutTestBuilder& test_name(const std::string& name) { name_ = name; return *this; }
    LayoutTestBuilder& description(const std::string& desc) { description_ = desc; return *this; }
    LayoutTestBuilder& test_breakpoints(bool enable = true) { test_breakpoints_ = enable; return *this; }
    LayoutTestBuilder& test_scaling(bool enable = true) { test_scaling_ = enable; return *this; }
    LayoutTestBuilder& test_overflow(bool enable = true) { test_overflow_ = enable; return *this; }
    LayoutTestBuilder& min_width(float width) { min_width_ = width; return *this; }
    LayoutTestBuilder& max_width(float width) { max_width_ = width; return *this; }
    
    TestCase build();

private:
    std::string name_ = "Layout Test";
    std::string description_ = "Automated layout responsiveness test";
    bool test_breakpoints_ = true;
    bool test_scaling_ = true;
    bool test_overflow_ = false;
    float min_width_ = 0.0f;
    float max_width_ = 0.0f;
};

/**
 * @brief Builder for typography tests
 */
class TypographyTestBuilder {
public:
    TypographyTestBuilder& test_name(const std::string& name) { name_ = name; return *this; }
    TypographyTestBuilder& description(const std::string& desc) { description_ = desc; return *this; }
    TypographyTestBuilder& test_scaling(bool enable = true) { test_scaling_ = enable; return *this; }
    TypographyTestBuilder& test_readability(bool enable = true) { test_readability_ = enable; return *this; }
    TypographyTestBuilder& test_line_height(bool enable = true) { test_line_height_ = enable; return *this; }
    TypographyTestBuilder& font_styles(const std::vector<std::string>& styles) { font_styles_ = styles; return *this; }
    
    TestCase build();

private:
    std::string name_ = "Typography Test";
    std::string description_ = "Automated typography scaling test";
    bool test_scaling_ = true;
    bool test_readability_ = true;
    bool test_line_height_ = true;
    std::vector<std::string> font_styles_ = {"body", "h1", "h2", "h3", "small"};
};

/**
 * @brief Builder for interaction tests
 */
class InteractionTestBuilder {
public:
    InteractionTestBuilder& test_name(const std::string& name) { name_ = name; return *this; }
    InteractionTestBuilder& description(const std::string& desc) { description_ = desc; return *this; }
    InteractionTestBuilder& test_touch_targets(bool enable = true) { test_touch_targets_ = enable; return *this; }
    InteractionTestBuilder& test_hover_states(bool enable = true) { test_hover_states_ = enable; return *this; }
    InteractionTestBuilder& test_focus_indicators(bool enable = true) { test_focus_indicators_ = enable; return *this; }
    InteractionTestBuilder& min_touch_size(float size) { min_touch_size_ = size; return *this; }
    
    TestCase build();

private:
    std::string name_ = "Interaction Test";
    std::string description_ = "Automated interaction responsiveness test";
    bool test_touch_targets_ = true;
    bool test_hover_states_ = true;
    bool test_focus_indicators_ = true;
    float min_touch_size_ = 44.0f; // iOS/Android minimum
};

// =============================================================================
// TEST MACROS & HELPERS
// =============================================================================

/**
 * @brief Test assertion macros
 */
#define RESPONSIVE_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            return TestResult::Fail; \
        } \
    } while(0)

#define RESPONSIVE_EXPECT(condition, message) \
    do { \
        if (!(condition)) { \
            /* Log warning but continue */ \
        } \
    } while(0)

#define RESPONSIVE_SKIP(message) \
    return TestResult::Skip

/**
 * @brief Utility functions for common test operations
 */
namespace test_utils {
    /**
     * @brief Check if two floating point values are approximately equal
     */
    bool approximately_equal(float a, float b, float tolerance = 0.01f);

    /**
     * @brief Check if value is within expected range
     */
    bool in_range(float value, float min_val, float max_val);

    /**
     * @brief Calculate percentage difference
     */
    float percentage_difference(float a, float b);

    /**
     * @brief Measure execution time of a function
     */
    template<typename Func>
    std::chrono::milliseconds measure_execution_time(Func&& func);

    /**
     * @brief Generate random test data
     */
    std::vector<std::string> generate_test_text(int count = 10);

    /**
     * @brief Create test UI elements for measurement
     */
#ifdef ECSCOPE_HAS_IMGUI
    void create_test_buttons(int count = 5);
    void create_test_text_elements();
    void create_test_input_elements();
    ImVec2 measure_element_size(std::function<void()> render_func);
#endif
}

// =============================================================================
// GLOBAL ACCESS
// =============================================================================

/**
 * @brief Get the global responsive testing framework
 */
ResponsiveTestingFramework* GetResponsiveTestingFramework();

/**
 * @brief Initialize global responsive testing framework
 */
bool InitializeGlobalResponsiveTesting(ResponsiveDesignManager* responsive_manager, 
                                      const TestConfig& config = TestConfig{});

/**
 * @brief Shutdown global responsive testing framework
 */
void ShutdownGlobalResponsiveTesting();

} // namespace ecscope::gui::testing