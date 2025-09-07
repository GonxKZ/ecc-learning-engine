#pragma once

/**
 * @file plugin/plugin_sdk.hpp
 * @brief ECScope Plugin SDK - Complete Development Kit for Plugin Creation
 * 
 * Comprehensive plugin development SDK providing templates, debugging tools,
 * documentation generation, testing frameworks, and educational resources.
 * This is everything a developer needs to create high-quality plugins for ECScope.
 * 
 * SDK Features:
 * - Plugin project templates and scaffolding
 * - Code generation and boilerplate automation
 * - Comprehensive debugging and profiling tools
 * - Documentation generation and validation
 * - Testing framework integration
 * - Educational tutorials and examples
 * - Best practices enforcement
 * - Performance analysis and optimization
 * 
 * @author ECScope Educational ECS Framework - Plugin System
 * @date 2024
 */

#include "plugin_core.hpp"
#include "plugin_api.hpp"
#include "core/types.hpp"
#include "core/log.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace ecscope::plugin::sdk {

//=============================================================================
// Plugin Template System
//=============================================================================

/**
 * @brief Plugin template type
 */
enum class PluginTemplateType {
    Basic,              // Basic plugin with minimal functionality
    Component,          // Plugin that adds new ECS components
    System,             // Plugin that adds new ECS systems
    Renderer,           // Graphics/rendering plugin
    Physics,            // Physics simulation plugin
    Audio,              // Audio processing plugin
    Tool,               // Development tool plugin
    Educational,        // Educational demonstration plugin
    ComplexSystem,      // Full-featured system plugin
    Custom              // User-defined template
};

/**
 * @brief Template configuration
 */
struct TemplateConfig {
    PluginTemplateType type{PluginTemplateType::Basic};
    std::string plugin_name;
    std::string display_name;
    std::string description;
    std::string author;
    std::string license{"MIT"};
    PluginCategory category{PluginCategory::Custom};
    PluginPriority priority{PluginPriority::Normal};
    
    // Feature flags
    bool include_examples{true};
    bool include_tests{true};
    bool include_documentation{true};
    bool include_cmake{true};
    bool include_git{false};
    bool enable_hot_reload{true};
    bool enable_debugging{true};
    
    // Educational features
    bool is_educational{false};
    std::string difficulty_level{"beginner"};
    std::vector<std::string> learning_objectives;
    
    // Dependencies
    std::vector<std::string> required_plugins;
    std::vector<std::string> optional_plugins;
    
    // Output configuration
    std::string output_directory{"./plugins"};
    bool overwrite_existing{false};
};

/**
 * @brief Code generation parameters
 */
struct CodeGenParams {
    std::unordered_map<std::string, std::string> replacements;
    std::unordered_map<std::string, bool> feature_flags;
    std::vector<std::string> include_files;
    std::vector<std::string> source_files;
    std::string namespace_name;
    std::string class_prefix;
};

/**
 * @brief Plugin template generator
 */
class PluginTemplateGenerator {
private:
    std::unordered_map<PluginTemplateType, std::string> template_paths_;
    std::unordered_map<std::string, std::string> template_cache_;

public:
    /**
     * @brief Generate plugin from template
     */
    bool generate_plugin(const TemplateConfig& config);
    
    /**
     * @brief Get available templates
     */
    std::vector<PluginTemplateType> get_available_templates() const;
    
    /**
     * @brief Get template description
     */
    std::string get_template_description(PluginTemplateType type) const;
    
    /**
     * @brief Generate code from template
     */
    std::string generate_code(const std::string& template_content, 
                             const CodeGenParams& params);
    
    /**
     * @brief Create project structure
     */
    bool create_project_structure(const TemplateConfig& config);
    
    /**
     * @brief Generate CMakeLists.txt
     */
    std::string generate_cmake_file(const TemplateConfig& config);
    
    /**
     * @brief Generate plugin metadata JSON
     */
    std::string generate_metadata_json(const TemplateConfig& config);
    
    /**
     * @brief Generate README.md
     */
    std::string generate_readme(const TemplateConfig& config);
    
    /**
     * @brief Generate example usage code
     */
    std::string generate_example_code(const TemplateConfig& config);

private:
    void initialize_templates();
    std::string load_template(const std::string& template_path);
    bool write_file(const std::string& file_path, const std::string& content);
    std::string process_template_variables(const std::string& content, 
                                          const CodeGenParams& params);
};

//=============================================================================
// Debugging and Profiling Tools
//=============================================================================

/**
 * @brief Plugin debugger interface
 */
class PluginDebugger {
private:
    std::string plugin_name_;
    bool is_debugging_{false};
    
    // Breakpoint system
    std::unordered_set<std::string> breakpoints_;
    std::unordered_map<std::string, std::function<void()>> breakpoint_handlers_;
    
    // Variable watching
    std::unordered_map<std::string, std::any> watched_variables_;
    std::unordered_map<std::string, std::function<std::string()>> variable_getters_;
    
    // Execution tracing
    std::vector<std::string> execution_trace_;
    bool enable_tracing_{false};
    
    // Performance profiling
    std::unordered_map<std::string, f64> function_timings_;
    std::unordered_map<std::string, u32> function_call_counts_;

public:
    explicit PluginDebugger(const std::string& plugin_name);
    
    /**
     * @brief Start debugging session
     */
    void start_debugging();
    
    /**
     * @brief Stop debugging session
     */
    void stop_debugging();
    
    /**
     * @brief Set breakpoint
     */
    void set_breakpoint(const std::string& location, 
                       std::function<void()> handler = nullptr);
    
    /**
     * @brief Remove breakpoint
     */
    void remove_breakpoint(const std::string& location);
    
    /**
     * @brief Check if breakpoint should trigger
     */
    bool check_breakpoint(const std::string& location);
    
    /**
     * @brief Watch variable
     */
    template<typename T>
    void watch_variable(const std::string& name, const T* variable_ptr);
    
    /**
     * @brief Watch variable with custom getter
     */
    void watch_variable_custom(const std::string& name, 
                              std::function<std::string()> getter);
    
    /**
     * @brief Get watched variable value
     */
    std::string get_variable_value(const std::string& name);
    
    /**
     * @brief Start execution tracing
     */
    void start_tracing();
    
    /**
     * @brief Stop execution tracing
     */
    void stop_tracing();
    
    /**
     * @brief Add trace entry
     */
    void trace(const std::string& message);
    
    /**
     * @brief Get execution trace
     */
    const std::vector<std::string>& get_trace() const { return execution_trace_; }
    
    /**
     * @brief Profile function execution
     */
    class FunctionProfiler {
    private:
        std::chrono::high_resolution_clock::time_point start_time_;
        std::string function_name_;
        PluginDebugger& debugger_;
        
    public:
        FunctionProfiler(const std::string& name, PluginDebugger& debugger)
            : start_time_(std::chrono::high_resolution_clock::now())
            , function_name_(name), debugger_(debugger) {}
        
        ~FunctionProfiler() {
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<f64, std::milli>(end_time - start_time_).count();
            debugger_.record_function_timing(function_name_, duration);
        }
    };
    
    FunctionProfiler profile_function(const std::string& function_name) {
        return FunctionProfiler(function_name, *this);
    }
    
    /**
     * @brief Get profiling results
     */
    struct ProfilingResults {
        std::unordered_map<std::string, f64> average_timings;
        std::unordered_map<std::string, u32> call_counts;
        f64 total_execution_time;
        std::string hottest_function;
    };
    
    ProfilingResults get_profiling_results() const;
    
    /**
     * @brief Generate debug report
     */
    std::string generate_debug_report() const;

private:
    void record_function_timing(const std::string& function_name, f64 timing_ms);
};

/**
 * @brief Debug utility macros for plugins
 */
#define PLUGIN_DEBUG_BREAK(debugger, location) \
    do { \
        if ((debugger).check_breakpoint(location)) { \
            LOG_DEBUG("Breakpoint hit at: {}", location); \
        } \
    } while(0)

#define PLUGIN_DEBUG_TRACE(debugger, message) \
    do { \
        (debugger).trace(message); \
    } while(0)

#define PLUGIN_PROFILE_FUNCTION(debugger, function_name) \
    auto _profile = (debugger).profile_function(function_name)

//=============================================================================
// Documentation Generation
//=============================================================================

/**
 * @brief Documentation format type
 */
enum class DocumentationFormat {
    Markdown,
    HTML,
    LaTeX,
    JSON,
    XML
};

/**
 * @brief Documentation section type
 */
enum class DocumentationSection {
    Overview,
    Installation,
    Usage,
    API,
    Examples,
    Configuration,
    Troubleshooting,
    ChangeLog,
    License
};

/**
 * @brief Documentation generator
 */
class DocumentationGenerator {
private:
    PluginMetadata metadata_;
    std::unordered_map<DocumentationSection, std::string> sections_;
    std::vector<std::string> code_examples_;
    std::vector<std::string> api_references_;

public:
    explicit DocumentationGenerator(const PluginMetadata& metadata);
    
    /**
     * @brief Add documentation section
     */
    void add_section(DocumentationSection section, const std::string& content);
    
    /**
     * @brief Add code example
     */
    void add_code_example(const std::string& title, const std::string& code, 
                         const std::string& language = "cpp");
    
    /**
     * @brief Add API reference
     */
    void add_api_reference(const std::string& function_signature, 
                          const std::string& description,
                          const std::vector<std::string>& parameters = {},
                          const std::string& return_description = "");
    
    /**
     * @brief Generate documentation in specified format
     */
    std::string generate_documentation(DocumentationFormat format);
    
    /**
     * @brief Generate API documentation
     */
    std::string generate_api_documentation(DocumentationFormat format);
    
    /**
     * @brief Generate tutorial documentation
     */
    std::string generate_tutorial(const std::string& tutorial_title,
                                 const std::vector<std::string>& steps);
    
    /**
     * @brief Extract documentation from source code comments
     */
    bool extract_from_source_files(const std::vector<std::string>& file_paths);
    
    /**
     * @brief Validate documentation completeness
     */
    struct ValidationResult {
        bool is_complete;
        std::vector<std::string> missing_sections;
        std::vector<std::string> warnings;
        f32 completeness_score;
    };
    
    ValidationResult validate_documentation();
    
    /**
     * @brief Generate documentation template
     */
    std::string generate_template(DocumentationFormat format);

private:
    std::string generate_markdown();
    std::string generate_html();
    std::string generate_latex();
    std::string generate_json();
    std::string generate_xml();
    
    std::string format_code_block(const std::string& code, const std::string& language);
    std::string format_api_reference(const std::string& signature, 
                                    const std::string& description);
    std::string parse_source_comments(const std::string& file_content);
};

//=============================================================================
// Testing Framework Integration
//=============================================================================

/**
 * @brief Test type classification
 */
enum class TestType {
    Unit,           // Unit tests for individual functions/classes
    Integration,    // Integration tests for plugin interactions
    Performance,    // Performance and benchmark tests
    Compatibility,  // Compatibility tests with engine versions
    Educational,    // Educational test examples
    Regression      // Regression tests for bug fixes
};

/**
 * @brief Test result
 */
struct TestResult {
    std::string test_name;
    TestType type;
    bool passed{false};
    f64 execution_time_ms{0.0};
    std::string error_message;
    std::string output;
    std::unordered_map<std::string, std::string> metrics;
};

/**
 * @brief Plugin test framework
 */
class PluginTestFramework {
private:
    std::string plugin_name_;
    std::vector<std::function<TestResult()>> test_functions_;
    std::unordered_map<std::string, TestType> test_types_;
    std::vector<TestResult> test_results_;

public:
    explicit PluginTestFramework(const std::string& plugin_name);
    
    /**
     * @brief Register test function
     */
    void register_test(const std::string& test_name, TestType type,
                      std::function<TestResult()> test_function);
    
    /**
     * @brief Run all tests
     */
    std::vector<TestResult> run_all_tests();
    
    /**
     * @brief Run tests of specific type
     */
    std::vector<TestResult> run_tests_by_type(TestType type);
    
    /**
     * @brief Run single test
     */
    TestResult run_test(const std::string& test_name);
    
    /**
     * @brief Generate test report
     */
    std::string generate_test_report(const std::vector<TestResult>& results);
    
    /**
     * @brief Create test template
     */
    std::string create_test_template(const std::string& test_name, TestType type);
    
    /**
     * @brief Validate test coverage
     */
    struct CoverageReport {
        f32 line_coverage;
        f32 function_coverage;
        f32 branch_coverage;
        std::vector<std::string> uncovered_functions;
        std::vector<std::string> uncovered_lines;
    };
    
    CoverageReport analyze_test_coverage();
    
    /**
     * @brief Generate performance benchmark
     */
    TestResult benchmark_function(const std::string& function_name,
                                 std::function<void()> function,
                                 u32 iterations = 1000);

private:
    TestResult execute_test_safely(std::function<TestResult()> test_function);
    void setup_test_environment();
    void cleanup_test_environment();
};

//=============================================================================
// Code Quality and Analysis Tools
//=============================================================================

/**
 * @brief Code quality analyzer
 */
class CodeQualityAnalyzer {
private:
    std::string plugin_directory_;
    
public:
    explicit CodeQualityAnalyzer(const std::string& plugin_directory);
    
    /**
     * @brief Code quality metrics
     */
    struct QualityMetrics {
        u32 lines_of_code;
        u32 lines_of_comments;
        u32 cyclomatic_complexity;
        f32 comment_ratio;
        u32 number_of_functions;
        u32 number_of_classes;
        std::vector<std::string> potential_issues;
        std::vector<std::string> style_violations;
        f32 overall_score; // 0-100
    };
    
    /**
     * @brief Analyze code quality
     */
    QualityMetrics analyze_code_quality();
    
    /**
     * @brief Check coding standards compliance
     */
    std::vector<std::string> check_coding_standards();
    
    /**
     * @brief Detect potential security issues
     */
    std::vector<std::string> detect_security_issues();
    
    /**
     * @brief Suggest performance improvements
     */
    std::vector<std::string> suggest_performance_improvements();
    
    /**
     * @brief Generate quality report
     */
    std::string generate_quality_report(const QualityMetrics& metrics);

private:
    std::vector<std::string> get_source_files();
    QualityMetrics analyze_file(const std::string& file_path);
    u32 calculate_cyclomatic_complexity(const std::string& content);
    std::vector<std::string> check_file_standards(const std::string& content);
};

//=============================================================================
// Educational Resources and Tutorials
//=============================================================================

/**
 * @brief Tutorial step
 */
struct TutorialStep {
    std::string title;
    std::string description;
    std::string code_example;
    std::vector<std::string> key_points;
    std::string expected_output;
    std::vector<std::string> common_mistakes;
};

/**
 * @brief Educational tutorial generator
 */
class TutorialGenerator {
private:
    std::vector<TutorialStep> tutorial_steps_;
    std::string tutorial_title_;
    std::string difficulty_level_;

public:
    /**
     * @brief Create tutorial
     */
    TutorialGenerator(const std::string& title, const std::string& difficulty);
    
    /**
     * @brief Add tutorial step
     */
    void add_step(const TutorialStep& step);
    
    /**
     * @brief Generate complete tutorial
     */
    std::string generate_tutorial();
    
    /**
     * @brief Generate interactive tutorial
     */
    std::string generate_interactive_tutorial();
    
    /**
     * @brief Create beginner plugin tutorial
     */
    static TutorialGenerator create_beginner_tutorial();
    
    /**
     * @brief Create intermediate plugin tutorial
     */
    static TutorialGenerator create_intermediate_tutorial();
    
    /**
     * @brief Create advanced plugin tutorial
     */
    static TutorialGenerator create_advanced_tutorial();
};

//=============================================================================
// Build System Integration
//=============================================================================

/**
 * @brief Build system configuration
 */
struct BuildConfig {
    std::string build_system{"cmake"};  // cmake, make, ninja
    std::string compiler{"gcc"};         // gcc, clang, msvc
    std::string build_type{"Release"};   // Debug, Release, RelWithDebInfo
    std::vector<std::string> compile_flags;
    std::vector<std::string> link_flags;
    std::vector<std::string> dependencies;
    std::string output_directory{"build"};
    bool enable_testing{true};
    bool enable_documentation{true};
};

/**
 * @brief Build system generator
 */
class BuildSystemGenerator {
private:
    BuildConfig config_;
    std::string project_directory_;

public:
    explicit BuildSystemGenerator(const std::string& project_dir, 
                                 const BuildConfig& config);
    
    /**
     * @brief Generate CMakeLists.txt
     */
    std::string generate_cmake_file();
    
    /**
     * @brief Generate Makefile
     */
    std::string generate_makefile();
    
    /**
     * @brief Generate build script
     */
    std::string generate_build_script();
    
    /**
     * @brief Generate CI/CD configuration
     */
    std::string generate_ci_config(const std::string& ci_system = "github");
    
    /**
     * @brief Validate build configuration
     */
    std::vector<std::string> validate_build_config();

private:
    std::string get_compiler_flags();
    std::string get_linker_flags();
    std::string get_dependencies_cmake();
};

//=============================================================================
// Main SDK Interface
//=============================================================================

/**
 * @brief Main Plugin SDK interface
 */
class PluginSDK {
private:
    std::unique_ptr<PluginTemplateGenerator> template_generator_;
    std::unique_ptr<PluginDebugger> debugger_;
    std::unique_ptr<DocumentationGenerator> doc_generator_;
    std::unique_ptr<PluginTestFramework> test_framework_;
    std::unique_ptr<CodeQualityAnalyzer> quality_analyzer_;
    
    std::string sdk_directory_;
    std::string current_plugin_;

public:
    /**
     * @brief Initialize Plugin SDK
     */
    explicit PluginSDK(const std::string& sdk_directory);
    
    /**
     * @brief Destructor
     */
    ~PluginSDK();
    
    /**
     * @brief Create new plugin project
     */
    bool create_plugin_project(const TemplateConfig& config);
    
    /**
     * @brief Open existing plugin project
     */
    bool open_plugin_project(const std::string& project_path);
    
    /**
     * @brief Build plugin project
     */
    bool build_plugin(const BuildConfig& config = {});
    
    /**
     * @brief Test plugin project
     */
    std::vector<TestResult> test_plugin();
    
    /**
     * @brief Debug plugin
     */
    PluginDebugger& get_debugger() { return *debugger_; }
    
    /**
     * @brief Generate documentation
     */
    std::string generate_documentation(DocumentationFormat format = DocumentationFormat::Markdown);
    
    /**
     * @brief Analyze code quality
     */
    CodeQualityAnalyzer::QualityMetrics analyze_quality();
    
    /**
     * @brief Get available tutorials
     */
    std::vector<std::string> get_available_tutorials();
    
    /**
     * @brief Generate tutorial
     */
    std::string generate_tutorial(const std::string& tutorial_name);
    
    /**
     * @brief Package plugin for distribution
     */
    bool package_plugin(const std::string& output_path);
    
    /**
     * @brief Validate plugin
     */
    std::vector<std::string> validate_plugin();
    
    /**
     * @brief Get SDK version
     */
    std::string get_sdk_version() const;
    
    /**
     * @brief Get help information
     */
    std::string get_help(const std::string& topic = "") const;

private:
    void initialize_sdk();
    void cleanup_sdk();
    bool validate_project_structure(const std::string& project_path);
};

//=============================================================================
// Utility Functions
//=============================================================================

/**
 * @brief Get default template configuration for type
 */
TemplateConfig get_default_template_config(PluginTemplateType type);

/**
 * @brief Validate plugin name
 */
bool validate_plugin_name(const std::string& name);

/**
 * @brief Generate unique plugin ID
 */
std::string generate_plugin_id(const std::string& name, const std::string& author);

/**
 * @brief Get SDK installation directory
 */
std::string get_sdk_installation_directory();

/**
 * @brief Check SDK installation
 */
bool check_sdk_installation();

} // namespace ecscope::plugin::sdk