/**
 * @file accessibility_testing.hpp
 * @brief Accessibility Testing and Validation Framework
 * 
 * Comprehensive testing framework for validating WCAG 2.1 compliance,
 * automated accessibility testing, user testing simulation, and 
 * accessibility audit reporting for professional development tools.
 * 
 * Features:
 * - WCAG 2.1 Level AA/AAA compliance validation
 * - Automated accessibility testing suite
 * - Screen reader simulation and testing
 * - Keyboard navigation testing
 * - Color contrast analysis and validation
 * - Focus management testing
 * - Live accessibility monitoring
 * - Accessibility audit reports
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#pragma once

#include "accessibility_core.hpp"
#include "accessibility_keyboard.hpp"
#include "accessibility_screen_reader.hpp"
#include "accessibility_visual.hpp"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <fstream>

namespace ecscope::gui::accessibility {

// =============================================================================
// TESTING ENUMERATIONS
// =============================================================================

/**
 * @brief Accessibility test severity levels
 */
enum class TestSeverity : uint8_t {
    Info,       // Informational
    Minor,      // Minor issue, doesn't block accessibility
    Major,      // Major issue, significantly impacts accessibility
    Critical,   // Critical issue, blocks accessibility
    Blocker     // Absolute blocker, prevents use
};

/**
 * @brief Test categories
 */
enum class TestCategory : uint8_t {
    General,
    KeyboardNavigation,
    ScreenReader,
    ColorContrast,
    FocusManagement,
    TextAlternatives,
    FormLabeling,
    HeadingStructure,
    LiveRegions,
    TimingAndMotion,
    ErrorHandling,
    UserInterface,
    Documentation
};

/**
 * @brief WCAG success criteria levels
 */
enum class WCAGSuccessCriteria : uint16_t {
    // Level A
    SC_1_1_1_NonTextContent = 111,
    SC_1_2_1_AudioOnlyVideoOnly = 121,
    SC_1_3_1_InfoAndRelationships = 131,
    SC_1_3_2_MeaningfulSequence = 132,
    SC_1_3_3_SensoryCharacteristics = 133,
    SC_1_4_1_UseOfColor = 141,
    SC_1_4_2_AudioControl = 142,
    SC_2_1_1_Keyboard = 211,
    SC_2_1_2_NoKeyboardTrap = 212,
    SC_2_1_4_CharacterKeyShortcuts = 214,
    SC_2_2_1_TimingAdjustable = 221,
    SC_2_2_2_PauseStopHide = 222,
    SC_2_3_1_ThreeFlashesOrBelowThreshold = 231,
    SC_2_4_1_BypassBlocks = 241,
    SC_2_4_2_PageTitled = 242,
    SC_2_4_3_FocusOrder = 243,
    SC_2_4_4_LinkPurpose = 244,
    SC_2_5_1_PointerGestures = 251,
    SC_2_5_2_PointerCancellation = 252,
    SC_2_5_3_LabelInName = 253,
    SC_2_5_4_MotionActuation = 254,
    SC_3_1_1_LanguageOfPage = 311,
    SC_3_2_1_OnFocus = 321,
    SC_3_2_2_OnInput = 322,
    SC_3_3_1_ErrorIdentification = 331,
    SC_3_3_2_LabelsOrInstructions = 332,
    SC_4_1_1_Parsing = 411,
    SC_4_1_2_NameRoleValue = 412,
    
    // Level AA
    SC_1_2_4_Captions = 124,
    SC_1_2_5_AudioDescription = 125,
    SC_1_4_3_ContrastMinimum = 143,
    SC_1_4_4_ResizeText = 144,
    SC_1_4_5_ImagesOfText = 145,
    SC_1_4_10_Reflow = 1410,
    SC_1_4_11_NonTextContrast = 1411,
    SC_1_4_12_TextSpacing = 1412,
    SC_1_4_13_ContentOnHoverOrFocus = 1413,
    SC_2_4_5_MultipleWays = 245,
    SC_2_4_6_HeadingsAndLabels = 246,
    SC_2_4_7_FocusVisible = 247,
    SC_2_4_11_FocusNotObscured = 2411,
    SC_3_1_2_LanguageOfParts = 312,
    SC_3_2_3_ConsistentNavigation = 323,
    SC_3_2_4_ConsistentIdentification = 324,
    SC_3_2_6_ConsistentHelp = 326,
    SC_3_3_3_ErrorSuggestion = 333,
    SC_3_3_4_ErrorPrevention = 334,
    SC_3_3_7_RedundantEntry = 337,
    SC_4_1_3_StatusMessages = 413,
    
    // Level AAA
    SC_1_2_6_SignLanguage = 126,
    SC_1_2_7_ExtendedAudioDescription = 127,
    SC_1_2_8_MediaAlternative = 128,
    SC_1_2_9_AudioOnly = 129,
    SC_1_4_6_ContrastEnhanced = 146,
    SC_1_4_7_LowOrNoBackgroundAudio = 147,
    SC_1_4_8_VisualPresentation = 148,
    SC_1_4_9_ImagesOfTextNoException = 149,
    SC_2_1_3_KeyboardNoException = 213,
    SC_2_2_3_NoTiming = 223,
    SC_2_2_4_Interruptions = 224,
    SC_2_2_5_ReAuthentication = 225,
    SC_2_2_6_Timeouts = 226,
    SC_2_3_2_ThreeFlashes = 232,
    SC_2_3_3_AnimationFromInteractions = 233,
    SC_2_4_8_Location = 248,
    SC_2_4_9_LinkPurpose = 249,
    SC_2_4_10_SectionHeadings = 2410,
    SC_2_5_5_TargetSize = 255,
    SC_2_5_6_ConcurrentInputMechanisms = 256,
    SC_3_1_3_UnusualWords = 313,
    SC_3_1_4_Abbreviations = 314,
    SC_3_1_5_ReadingLevel = 315,
    SC_3_1_6_Pronunciation = 316,
    SC_3_2_5_ChangeOnRequest = 325,
    SC_3_3_5_Help = 335,
    SC_3_3_6_ErrorPrevention = 336
};

// =============================================================================
// TEST RESULT STRUCTURES
// =============================================================================

/**
 * @brief Individual test result
 */
struct AccessibilityTestResult {
    std::string test_name;
    std::string test_id;
    TestCategory category = TestCategory::General;
    TestSeverity severity = TestSeverity::Info;
    WCAGSuccessCriteria wcag_criteria = WCAGSuccessCriteria::SC_1_1_1_NonTextContent;
    WCAGLevel wcag_level = WCAGLevel::A;
    
    bool passed = false;
    std::string description;
    std::string failure_reason;
    std::string recommendation;
    std::string help_url;
    
    // Context information
    GuiID widget_id = 0;
    std::string widget_type;
    std::string context_path;
    Rect widget_bounds;
    
    // Evidence and debugging
    std::vector<std::string> evidence;
    std::unordered_map<std::string, std::string> metadata;
    
    std::chrono::steady_clock::time_point timestamp;
    
    AccessibilityTestResult() : timestamp(std::chrono::steady_clock::now()) {}
};

/**
 * @brief Test suite results
 */
struct AccessibilityTestSuiteResult {
    std::string suite_name;
    std::string suite_version;
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point end_time;
    
    std::vector<AccessibilityTestResult> test_results;
    
    // Summary statistics
    size_t total_tests = 0;
    size_t passed_tests = 0;
    size_t failed_tests = 0;
    size_t skipped_tests = 0;
    
    size_t blocker_count = 0;
    size_t critical_count = 0;
    size_t major_count = 0;
    size_t minor_count = 0;
    size_t info_count = 0;
    
    // WCAG compliance
    bool wcag_a_compliant = false;
    bool wcag_aa_compliant = false;
    bool wcag_aaa_compliant = false;
    
    float compliance_score = 0.0f;  // 0-100%
    
    std::string generate_summary() const;
    std::string generate_detailed_report() const;
    void save_to_file(const std::string& filename) const;
    bool load_from_file(const std::string& filename);
};

// =============================================================================
// ACCESSIBILITY TESTING FRAMEWORK
// =============================================================================

/**
 * @brief Main accessibility testing framework
 */
class AccessibilityTestFramework {
public:
    AccessibilityTestFramework();
    ~AccessibilityTestFramework();

    // =============================================================================
    // INITIALIZATION & LIFECYCLE
    // =============================================================================

    bool initialize(AccessibilityContext* accessibility_context,
                   AdvancedKeyboardNavigator* keyboard_navigator,
                   ScreenReaderManager* screen_reader_manager,
                   VisualAccessibilityManager* visual_manager);
    void shutdown();
    void update(float delta_time);

    // =============================================================================
    // TEST SUITE MANAGEMENT
    // =============================================================================

    void register_test_suite(const std::string& suite_name);
    void unregister_test_suite(const std::string& suite_name);
    std::vector<std::string> get_available_test_suites() const;
    
    AccessibilityTestSuiteResult run_test_suite(const std::string& suite_name);
    AccessibilityTestSuiteResult run_all_test_suites();
    AccessibilityTestSuiteResult run_wcag_compliance_test(WCAGLevel target_level = WCAGLevel::AA);
    
    void set_test_timeout(float timeout_seconds);
    float get_test_timeout() const;

    // =============================================================================
    // INDIVIDUAL TESTS
    // =============================================================================

    // Keyboard navigation tests
    AccessibilityTestResult test_keyboard_accessibility();
    AccessibilityTestResult test_focus_order();
    AccessibilityTestResult test_focus_traps();
    AccessibilityTestResult test_skip_links();
    AccessibilityTestResult test_keyboard_shortcuts();
    AccessibilityTestResult test_no_keyboard_traps();
    
    // Screen reader tests
    AccessibilityTestResult test_screen_reader_compatibility();
    AccessibilityTestResult test_accessible_names();
    AccessibilityTestResult test_accessible_descriptions();
    AccessibilityTestResult test_aria_roles();
    AccessibilityTestResult test_aria_states();
    AccessibilityTestResult test_live_regions();
    AccessibilityTestResult test_heading_structure();
    
    // Visual accessibility tests
    AccessibilityTestResult test_color_contrast();
    AccessibilityTestResult test_color_independence();
    AccessibilityTestResult test_text_scaling();
    AccessibilityTestResult test_high_contrast_compatibility();
    AccessibilityTestResult test_reduced_motion_support();
    AccessibilityTestResult test_focus_indicators();
    
    // Form and interaction tests
    AccessibilityTestResult test_form_labels();
    AccessibilityTestResult test_error_identification();
    AccessibilityTestResult test_error_suggestions();
    AccessibilityTestResult test_required_field_indication();
    AccessibilityTestResult test_input_validation();
    
    // Content and structure tests
    AccessibilityTestResult test_content_structure();
    AccessibilityTestResult test_meaningful_sequence();
    AccessibilityTestResult test_language_identification();
    AccessibilityTestResult test_page_titles();
    AccessibilityTestResult test_link_purposes();
    
    // Timing and media tests
    AccessibilityTestResult test_timing_adjustable();
    AccessibilityTestResult test_auto_updating_content();
    AccessibilityTestResult test_moving_content_control();
    AccessibilityTestResult test_seizure_triggers();

    // =============================================================================
    // AUTOMATED TESTING
    // =============================================================================

    void enable_continuous_monitoring(bool enable);
    bool is_continuous_monitoring_enabled() const;
    
    void set_monitoring_interval(float seconds);
    float get_monitoring_interval() const;
    
    void add_test_trigger(const std::string& trigger_name, std::function<bool()> condition);
    void remove_test_trigger(const std::string& trigger_name);
    
    std::vector<AccessibilityTestResult> get_recent_issues() const;
    void clear_recent_issues();

    // =============================================================================
    // SIMULATION TESTING
    // =============================================================================

    /**
     * @brief Simulate screen reader navigation
     */
    AccessibilityTestResult simulate_screen_reader_navigation();
    
    /**
     * @brief Simulate keyboard-only navigation
     */
    AccessibilityTestResult simulate_keyboard_only_navigation();
    
    /**
     * @brief Simulate color blindness
     */
    AccessibilityTestResult simulate_color_blindness(ColorBlindnessType type);
    
    /**
     * @brief Simulate motor impairments
     */
    AccessibilityTestResult simulate_motor_impairments();
    
    /**
     * @brief Simulate low vision conditions
     */
    AccessibilityTestResult simulate_low_vision();

    // =============================================================================
    // VALIDATION UTILITIES
    // =============================================================================

    bool validate_widget_accessibility(GuiID widget_id);
    std::vector<std::string> get_widget_accessibility_issues(GuiID widget_id);
    std::string suggest_widget_improvements(GuiID widget_id);
    
    ContrastInfo validate_color_contrast(const Color& foreground, const Color& background);
    bool validate_focus_order(const std::vector<GuiID>& focus_order);
    bool validate_heading_hierarchy();
    bool validate_form_structure(GuiID form_id);

    // =============================================================================
    // REPORTING AND DOCUMENTATION
    // =============================================================================

    void generate_accessibility_report(const std::string& filename, 
                                      const AccessibilityTestSuiteResult& results);
    void generate_wcag_checklist(const std::string& filename, WCAGLevel target_level);
    void generate_remediation_guide(const std::string& filename, 
                                   const std::vector<AccessibilityTestResult>& failed_tests);
    
    std::string export_test_results_json(const AccessibilityTestSuiteResult& results);
    std::string export_test_results_xml(const AccessibilityTestSuiteResult& results);
    std::string export_test_results_csv(const AccessibilityTestSuiteResult& results);

    // =============================================================================
    // TEST CONFIGURATION
    // =============================================================================

    struct TestConfiguration {
        WCAGLevel target_wcag_level = WCAGLevel::AA;
        bool test_keyboard_navigation = true;
        bool test_screen_reader_support = true;
        bool test_color_contrast = true;
        bool test_focus_management = true;
        bool test_form_accessibility = true;
        bool test_content_structure = true;
        bool test_timing_and_motion = true;
        
        // Test sensitivity settings
        float contrast_tolerance = 0.1f;
        bool strict_wcag_interpretation = false;
        bool include_informational_messages = true;
        
        // Performance settings
        float max_test_duration = 30.0f;
        bool parallel_testing = true;
        size_t max_concurrent_tests = 4;
    };

    void set_test_configuration(const TestConfiguration& config);
    const TestConfiguration& get_test_configuration() const;

    // =============================================================================
    // DEBUGGING AND DIAGNOSTICS
    // =============================================================================

    struct TestFrameworkStats {
        size_t total_tests_run = 0;
        size_t total_issues_found = 0;
        float average_test_duration = 0.0f;
        std::chrono::steady_clock::time_point last_test_run;
        bool continuous_monitoring_active = false;
        size_t widgets_tested = 0;
        size_t wcag_violations_found = 0;
    };

    TestFrameworkStats get_stats() const;
    void render_debug_overlay(DrawList* draw_list);
    void print_test_summary() const;

    // =============================================================================
    // EVENT CALLBACKS
    // =============================================================================

    using TestCompletionCallback = std::function<void(const AccessibilityTestResult& result)>;
    using SuiteCompletionCallback = std::function<void(const AccessibilityTestSuiteResult& results)>;
    using IssueDetectedCallback = std::function<void(const AccessibilityTestResult& issue)>;

    void set_test_completion_callback(TestCompletionCallback callback);
    void set_suite_completion_callback(SuiteCompletionCallback callback);
    void set_issue_detected_callback(IssueDetectedCallback callback);

private:
    // Core components
    AccessibilityContext* accessibility_context_ = nullptr;
    AdvancedKeyboardNavigator* keyboard_navigator_ = nullptr;
    ScreenReaderManager* screen_reader_manager_ = nullptr;
    VisualAccessibilityManager* visual_manager_ = nullptr;
    
    // Test management
    std::unordered_map<std::string, std::vector<std::function<AccessibilityTestResult()>>> test_suites_;
    TestConfiguration test_configuration_;
    float test_timeout_ = 10.0f;
    
    // Continuous monitoring
    bool continuous_monitoring_enabled_ = false;
    float monitoring_interval_ = 5.0f;
    std::chrono::steady_clock::time_point last_monitoring_check_;
    
    // Test triggers
    std::unordered_map<std::string, std::function<bool()>> test_triggers_;
    
    // Recent issues
    std::vector<AccessibilityTestResult> recent_issues_;
    size_t max_recent_issues_ = 100;
    
    // Statistics
    mutable TestFrameworkStats stats_;
    
    // Callbacks
    TestCompletionCallback test_completion_callback_;
    SuiteCompletionCallback suite_completion_callback_;
    IssueDetectedCallback issue_detected_callback_;
    
    // Helper methods
    void register_built_in_tests();
    void update_continuous_monitoring(float delta_time);
    void check_test_triggers();
    
    AccessibilityTestResult create_test_result(const std::string& test_name, 
                                              TestCategory category,
                                              WCAGSuccessCriteria criteria,
                                              WCAGLevel level);
    
    void update_test_statistics();
    std::string wcag_criteria_to_string(WCAGSuccessCriteria criteria) const;
    TestSeverity calculate_test_severity(WCAGLevel level, bool passed) const;
    
    // Specific test implementations
    AccessibilityTestResult test_widget_keyboard_access(GuiID widget_id);
    AccessibilityTestResult test_widget_screen_reader_support(GuiID widget_id);
    AccessibilityTestResult test_widget_visual_accessibility(GuiID widget_id);
    AccessibilityTestResult test_widget_focus_management(GuiID widget_id);
    
    bool initialized_ = false;
};

// =============================================================================
// ACCESSIBILITY TESTING UTILITIES
// =============================================================================

/**
 * @brief Utilities for accessibility testing
 */
namespace testing_utils {
    /**
     * @brief Calculate WCAG compliance score
     */
    float calculate_wcag_compliance_score(const AccessibilityTestSuiteResult& results, WCAGLevel target_level);
    
    /**
     * @brief Generate test recommendations
     */
    std::vector<std::string> generate_recommendations(const std::vector<AccessibilityTestResult>& failed_tests);
    
    /**
     * @brief Create test data for simulation
     */
    std::vector<std::pair<GuiID, std::string>> create_test_navigation_path();
    
    /**
     * @brief Validate test results
     */
    bool validate_test_results(const AccessibilityTestSuiteResult& results);
    
    /**
     * @brief Create standard test configuration
     */
    AccessibilityTestFramework::TestConfiguration create_standard_test_config(WCAGLevel target_level);
}

// =============================================================================
// GLOBAL ACCESSIBILITY TESTING
// =============================================================================

/**
 * @brief Get the global accessibility test framework
 */
AccessibilityTestFramework* get_accessibility_test_framework();

/**
 * @brief Initialize global accessibility testing
 */
bool initialize_accessibility_testing();

/**
 * @brief Shutdown global accessibility testing
 */
void shutdown_accessibility_testing();

/**
 * @brief Run quick accessibility validation
 */
AccessibilityTestSuiteResult run_quick_accessibility_check();

/**
 * @brief Run full accessibility audit
 */
AccessibilityTestSuiteResult run_full_accessibility_audit(WCAGLevel target_level = WCAGLevel::AA);

} // namespace ecscope::gui::accessibility