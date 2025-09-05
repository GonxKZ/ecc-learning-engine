#pragma once

/**
 * @file interactive_code_examples.hpp
 * @brief Interactive Code Examples System for Educational Programming
 * 
 * This system provides interactive code examples with syntax highlighting,
 * real-time execution, and educational feedback for learning ECS concepts.
 * 
 * Features:
 * - Syntax highlighting for C++ and ECS-specific code
 * - Real-time code execution and validation
 * - Interactive code completion and hints
 * - Step-by-step code construction tutorials
 * - Error detection and educational explanations
 * 
 * @author ECScope Educational Framework
 * @date 2024
 */

#include "core/types.hpp"
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

namespace ecscope::learning {

/**
 * @brief Interactive code example with execution and validation
 */
struct InteractiveCodeExample {
    std::string id;
    std::string title;
    std::string description;
    std::string initial_code;
    std::string solution_code;
    std::string expected_output;
    
    // Interactive features
    std::vector<std::string> hints;
    std::vector<std::string> completion_suggestions;
    std::function<bool(const std::string&)> validator;
    
    // Execution state
    std::string current_code;
    std::string current_output;
    std::string current_errors;
    bool is_valid{false};
    bool is_completed{false};
    
    InteractiveCodeExample() = default;
    InteractiveCodeExample(const std::string& example_id, const std::string& example_title)
        : id(example_id), title(example_title), current_code(initial_code) {}
};

/**
 * @brief Code syntax highlighting and formatting
 */
class CodeSyntaxHighlighter {
public:
    struct HighlightedToken {
        std::string text;
        const f32* color;
        usize start_pos;
        usize length;
        
        HighlightedToken(const std::string& txt, const f32* col, usize start, usize len)
            : text(txt), color(col), start_pos(start), length(len) {}
    };
    
private:
    // Color constants
    static constexpr f32 KEYWORD_COLOR[4] = {0.5f, 0.8f, 1.0f, 1.0f};    // Light blue
    static constexpr f32 STRING_COLOR[4] = {0.8f, 0.6f, 0.8f, 1.0f};     // Light purple
    static constexpr f32 COMMENT_COLOR[4] = {0.5f, 0.7f, 0.5f, 1.0f};    // Green
    static constexpr f32 NUMBER_COLOR[4] = {1.0f, 0.7f, 0.4f, 1.0f};     // Orange
    static constexpr f32 TYPE_COLOR[4] = {0.4f, 0.8f, 0.4f, 1.0f};       // Light green
    static constexpr f32 FUNCTION_COLOR[4] = {1.0f, 0.8f, 0.4f, 1.0f};   // Yellow
    static constexpr f32 DEFAULT_COLOR[4] = {0.9f, 0.9f, 0.9f, 1.0f};    // Light gray
    
    std::vector<std::string> cpp_keywords_;
    std::vector<std::string> ecs_types_;
    std::vector<std::string> ecs_functions_;
    
public:
    CodeSyntaxHighlighter();
    
    std::vector<HighlightedToken> highlight_code(const std::string& code) const;
    void render_highlighted_code(const std::vector<HighlightedToken>& tokens, f32 wrap_width = 0.0f);
    
private:
    bool is_keyword(const std::string& token) const;
    bool is_ecs_type(const std::string& token) const;
    bool is_ecs_function(const std::string& token) const;
    bool is_number(const std::string& token) const;
    std::vector<std::string> tokenize_code(const std::string& code) const;
};

/**
 * @brief Interactive code execution engine (simplified simulation)
 */
class CodeExecutionEngine {
public:
    struct ExecutionResult {
        bool success{false};
        std::string output;
        std::string error_message;
        f64 execution_time_ms{0.0};
        
        ExecutionResult() = default;
        ExecutionResult(bool ok, const std::string& out = "", const std::string& err = "")
            : success(ok), output(out), error_message(err) {}
    };
    
private:
    // Simulated ECS environment for code execution
    struct SimulatedECS {
        u32 entity_count{0};
        std::vector<std::string> component_types;
        std::vector<std::string> system_names;
        std::unordered_map<std::string, std::string> variables;
        
        void reset() {
            entity_count = 0;
            component_types.clear();
            system_names.clear();
            variables.clear();
        }
    };
    
    SimulatedECS sim_environment_;
    
public:
    CodeExecutionEngine() = default;
    
    ExecutionResult execute_code(const std::string& code);
    ExecutionResult validate_code_syntax(const std::string& code);
    std::vector<std::string> get_completion_suggestions(const std::string& partial_code, usize cursor_pos);
    
    void reset_environment() { sim_environment_.reset(); }
    
private:
    ExecutionResult simulate_ecs_operations(const std::string& code);
    bool parse_entity_creation(const std::string& line, std::string& output);
    bool parse_component_addition(const std::string& line, std::string& output);
    bool parse_system_execution(const std::string& line, std::string& output);
    std::string extract_string_literal(const std::string& code, const std::string& pattern);
};

/**
 * @brief Factory for creating educational code examples
 */
class CodeExampleFactory {
public:
    // Basic ECS examples
    static InteractiveCodeExample create_entity_creation_example();
    static InteractiveCodeExample create_component_addition_example();
    static InteractiveCodeExample create_system_iteration_example();
    static InteractiveCodeExample create_query_example();
    
    // Intermediate examples
    static InteractiveCodeExample create_archetype_example();
    static InteractiveCodeExample create_memory_optimization_example();
    static InteractiveCodeExample create_performance_measurement_example();
    
    // Advanced examples
    static InteractiveCodeExample create_custom_allocator_example();
    static InteractiveCodeExample create_parallel_system_example();
    static InteractiveCodeExample create_sparse_set_example();
    
    // Tutorial-specific examples
    static InteractiveCodeExample create_example_for_tutorial_step(const std::string& tutorial_id, const std::string& step_id);
    
private:
    static std::function<bool(const std::string&)> create_entity_validator();
    static std::function<bool(const std::string&)> create_component_validator();
    static std::function<bool(const std::string&)> create_system_validator();
};

} // namespace ecscope::learning