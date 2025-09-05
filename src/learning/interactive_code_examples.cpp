#include "interactive_code_examples.hpp"
#include "core/log.hpp"
#include <imgui.h>
#include <algorithm>
#include <regex>
#include <sstream>

namespace ecscope::learning {

// CodeSyntaxHighlighter Implementation

CodeSyntaxHighlighter::CodeSyntaxHighlighter() {
    // C++ keywords
    cpp_keywords_ = {
        "auto", "bool", "break", "case", "catch", "char", "class", "const", "constexpr",
        "continue", "default", "delete", "do", "double", "else", "enum", "explicit",
        "extern", "false", "float", "for", "friend", "goto", "if", "inline", "int",
        "long", "namespace", "new", "nullptr", "operator", "private", "protected",
        "public", "return", "short", "signed", "sizeof", "static", "struct", "switch",
        "template", "this", "throw", "true", "try", "typedef", "typename", "union",
        "unsigned", "using", "virtual", "void", "volatile", "while"
    };
    
    // ECS-specific types
    ecs_types_ = {
        "Entity", "Registry", "Component", "System", "Archetype", "SparseSet",
        "Transform", "RigidBody", "Velocity", "Position", "Rotation", "Scale",
        "Vec2", "Vec3", "Mat4", "Quaternion"
    };
    
    // ECS-specific functions
    ecs_functions_ = {
        "create_entity", "destroy_entity", "add_component", "remove_component",
        "get_component", "has_component", "for_each", "query", "get_registry",
        "register_system", "execute_system", "update", "render"
    };
}

std::vector<CodeSyntaxHighlighter::HighlightedToken> CodeSyntaxHighlighter::highlight_code(const std::string& code) const {
    std::vector<HighlightedToken> tokens;
    
    // Simple tokenization (could be improved with proper lexer)
    std::regex token_regex(R"((\w+)|([{}();,])|(".*?")|(//.*)|(\/\*.*?\*\/)|(\d+(?:\.\d+)?))");
    std::sregex_iterator iter(code.begin(), code.end(), token_regex);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        const std::smatch& match = *iter;
        std::string token = match.str();
        usize start_pos = static_cast<usize>(match.position());
        usize length = token.length();
        
        const f32* color = DEFAULT_COLOR;
        
        // Determine token type and color
        if (match[3].matched) { // String literal
            color = STRING_COLOR;
        } else if (match[4].matched || match[5].matched) { // Comment
            color = COMMENT_COLOR;
        } else if (match[6].matched) { // Number
            color = NUMBER_COLOR;
        } else if (is_keyword(token)) {
            color = KEYWORD_COLOR;
        } else if (is_ecs_type(token)) {
            color = TYPE_COLOR;
        } else if (is_ecs_function(token)) {
            color = FUNCTION_COLOR;
        }
        
        tokens.emplace_back(token, color, start_pos, length);
    }
    
    return tokens;
}

void CodeSyntaxHighlighter::render_highlighted_code(const std::vector<HighlightedToken>& tokens, f32 wrap_width) {
    for (const auto& token : tokens) {
        ImGui::TextColored(ImVec4(token.color[0], token.color[1], token.color[2], token.color[3]), 
                          "%s", token.text.c_str());
        ImGui::SameLine(0.0f, 0.0f); // No spacing between tokens
    }
    ImGui::NewLine();
}

bool CodeSyntaxHighlighter::is_keyword(const std::string& token) const {
    return std::find(cpp_keywords_.begin(), cpp_keywords_.end(), token) != cpp_keywords_.end();
}

bool CodeSyntaxHighlighter::is_ecs_type(const std::string& token) const {
    return std::find(ecs_types_.begin(), ecs_types_.end(), token) != ecs_types_.end();
}

bool CodeSyntaxHighlighter::is_ecs_function(const std::string& token) const {
    return std::find(ecs_functions_.begin(), ecs_functions_.end(), token) != ecs_functions_.end();
}

bool CodeSyntaxHighlighter::is_number(const std::string& token) const {
    if (token.empty()) return false;
    return std::all_of(token.begin(), token.end(), [](char c) {
        return std::isdigit(c) || c == '.' || c == 'f';
    });
}

// CodeExecutionEngine Implementation

CodeExecutionEngine::ExecutionResult CodeExecutionEngine::execute_code(const std::string& code) {
    // Reset environment for fresh execution
    sim_environment_.reset();
    
    // Validate syntax first
    auto syntax_result = validate_code_syntax(code);
    if (!syntax_result.success) {
        return syntax_result;
    }
    
    // Simulate ECS operations
    return simulate_ecs_operations(code);
}

CodeExecutionEngine::ExecutionResult CodeExecutionEngine::validate_code_syntax(const std::string& code) {
    // Simple syntax validation (could be more sophisticated)
    
    // Check for basic syntax errors
    size_t open_braces = std::count(code.begin(), code.end(), '{');
    size_t close_braces = std::count(code.begin(), code.end(), '}');
    
    if (open_braces != close_braces) {
        return ExecutionResult(false, "", "Syntax Error: Mismatched braces");
    }
    
    size_t open_parens = std::count(code.begin(), code.end(), '(');
    size_t close_parens = std::count(code.begin(), code.end(), ')');
    
    if (open_parens != close_parens) {
        return ExecutionResult(false, "", "Syntax Error: Mismatched parentheses");
    }
    
    // Check for missing semicolons (simple heuristic)
    std::istringstream iss(code);
    std::string line;
    int line_number = 0;
    
    while (std::getline(iss, line)) {
        line_number++;
        
        // Skip empty lines and comments
        std::string trimmed = line;
        trimmed.erase(0, trimmed.find_first_not_of(" \t"));
        if (trimmed.empty() || trimmed.substr(0, 2) == "//") continue;
        
        // Check if line should end with semicolon
        if (!trimmed.empty() && 
            trimmed.back() != ';' && 
            trimmed.back() != '{' && 
            trimmed.back() != '}' &&
            trimmed.find("if") != 0 &&
            trimmed.find("for") != 0 &&
            trimmed.find("while") != 0) {
            
            return ExecutionResult(false, "", 
                "Syntax Error: Missing semicolon on line " + std::to_string(line_number));
        }
    }
    
    return ExecutionResult(true, "Syntax validation passed");
}

CodeExecutionEngine::ExecutionResult CodeExecutionEngine::simulate_ecs_operations(const std::string& code) {
    std::ostringstream output;
    
    // Parse code line by line and simulate execution
    std::istringstream iss(code);
    std::string line;
    
    while (std::getline(iss, line)) {
        // Skip empty lines and comments
        std::string trimmed = line;
        trimmed.erase(0, trimmed.find_first_not_of(" \t"));
        if (trimmed.empty() || trimmed.substr(0, 2) == "//") continue;
        
        std::string line_output;
        bool executed = false;
        
        // Try to parse different ECS operations
        if (parse_entity_creation(trimmed, line_output)) {
            executed = true;
        } else if (parse_component_addition(trimmed, line_output)) {
            executed = true;
        } else if (parse_system_execution(trimmed, line_output)) {
            executed = true;
        }
        
        if (executed && !line_output.empty()) {
            output << line_output << "\n";
        }
    }
    
    // Add final state summary
    output << "--- Execution Summary ---\n";
    output << "Entities created: " << sim_environment_.entity_count << "\n";
    output << "Component types: " << sim_environment_.component_types.size() << "\n";
    output << "Systems executed: " << sim_environment_.system_names.size() << "\n";
    
    return ExecutionResult(true, output.str());
}

bool CodeExecutionEngine::parse_entity_creation(const std::string& line, std::string& output) {
    // Look for entity creation patterns
    if (line.find("create_entity") != std::string::npos) {
        sim_environment_.entity_count++;
        output = "Created entity " + std::to_string(sim_environment_.entity_count);
        return true;
    }
    
    if (line.find("Entity entity") != std::string::npos || 
        line.find("auto entity") != std::string::npos) {
        sim_environment_.entity_count++;
        output = "Created entity " + std::to_string(sim_environment_.entity_count);
        return true;
    }
    
    return false;
}

bool CodeExecutionEngine::parse_component_addition(const std::string& line, std::string& output) {
    // Look for component addition patterns
    if (line.find("add_component") != std::string::npos) {
        // Extract component type if possible
        std::regex component_regex(R"(add_component<(\w+)>)");
        std::smatch match;
        
        if (std::regex_search(line, match, component_regex)) {
            std::string component_type = match[1].str();
            sim_environment_.component_types.push_back(component_type);
            output = "Added component: " + component_type;
            return true;
        }
        
        output = "Added component to entity";
        return true;
    }
    
    return false;
}

bool CodeExecutionEngine::parse_system_execution(const std::string& line, std::string& output) {
    // Look for system execution patterns
    if (line.find("for_each") != std::string::npos) {
        output = "Executed system query on " + std::to_string(sim_environment_.entity_count) + " entities";
        return true;
    }
    
    if (line.find("execute_system") != std::string::npos ||
        line.find("update") != std::string::npos) {
        sim_environment_.system_names.push_back("update_system");
        output = "Executed update system";
        return true;
    }
    
    return false;
}

std::vector<std::string> CodeExecutionEngine::get_completion_suggestions(const std::string& partial_code, usize cursor_pos) {
    std::vector<std::string> suggestions;
    
    // Extract current token at cursor position
    std::string current_token;
    if (cursor_pos > 0 && cursor_pos <= partial_code.length()) {
        // Simple token extraction (could be improved)
        size_t start = cursor_pos;
        while (start > 0 && (std::isalnum(partial_code[start - 1]) || partial_code[start - 1] == '_')) {
            start--;
        }
        
        current_token = partial_code.substr(start, cursor_pos - start);
    }
    
    // Suggest ECS-related completions based on current token
    std::vector<std::string> all_completions = {
        "create_entity()", "destroy_entity(", "add_component<>(", "remove_component<>(",
        "get_component<>(", "has_component<>(", "for_each<>([](", "Entity", "Registry",
        "Transform", "RigidBody", "Velocity", "Position"
    };
    
    for (const std::string& completion : all_completions) {
        if (current_token.empty() || completion.substr(0, current_token.length()) == current_token) {
            suggestions.push_back(completion);
        }
    }
    
    return suggestions;
}

// CodeExampleFactory Implementation

InteractiveCodeExample CodeExampleFactory::create_entity_creation_example() {
    InteractiveCodeExample example("entity_creation", "Creating Your First Entity");
    
    example.description = "Learn how to create entities in an ECS system";
    example.initial_code = "// Create an entity using the registry\nRegistry& registry = get_registry();\n\n// TODO: Create an entity here\n";
    example.solution_code = "Registry& registry = get_registry();\nEntity entity = registry.create_entity();\nstd::cout << \"Created entity: \" << entity.index << std::endl;";
    example.expected_output = "Created entity: 1\n--- Execution Summary ---\nEntities created: 1";
    
    example.hints = {
        "Use registry.create_entity() to create a new entity",
        "Store the result in an Entity variable",
        "Print the entity's index to see the result"
    };
    
    example.validator = create_entity_validator();
    example.current_code = example.initial_code;
    
    return example;
}

InteractiveCodeExample CodeExampleFactory::create_component_addition_example() {
    InteractiveCodeExample example("component_addition", "Adding Components to Entities");
    
    example.description = "Learn how to add components to entities";
    example.initial_code = "Registry& registry = get_registry();\nEntity entity = registry.create_entity();\n\n// TODO: Add a Transform component to the entity\n";
    example.solution_code = "Registry& registry = get_registry();\nEntity entity = registry.create_entity();\nregistry.add_component<Transform>(entity, Transform{Vec2{0, 0}, 0.0f, Vec2{1, 1}});";
    example.expected_output = "Created entity: 1\nAdded component: Transform\n--- Execution Summary ---\nEntities created: 1\nComponent types: 1";
    
    example.hints = {
        "Use registry.add_component<ComponentType>(entity, component_data)",
        "Transform component takes position, rotation, and scale",
        "Initialize Transform with Vec2{0, 0}, 0.0f, Vec2{1, 1}"
    };
    
    example.validator = create_component_validator();
    example.current_code = example.initial_code;
    
    return example;
}

InteractiveCodeExample CodeExampleFactory::create_system_iteration_example() {
    InteractiveCodeExample example("system_iteration", "Iterating Over Entities with Components");
    
    example.description = "Learn how to iterate over entities that have specific components";
    example.initial_code = "Registry& registry = get_registry();\n\n// Entities are already created with Transform components\n\n// TODO: Iterate over all entities with Transform components\n";
    example.solution_code = "Registry& registry = get_registry();\nregistry.for_each<Transform>([](Entity entity, Transform& transform) {\n    // Process each entity with Transform\n    transform.position.x += 1.0f;\n});";
    example.expected_output = "Executed system query on 5 entities\n--- Execution Summary ---\nEntities created: 5\nComponent types: 1";
    
    example.hints = {
        "Use registry.for_each<ComponentType>() to iterate",
        "Pass a lambda function that takes Entity and Component references",
        "Modify the component data inside the lambda"
    };
    
    example.validator = create_system_validator();
    example.current_code = example.initial_code;
    
    return example;
}

InteractiveCodeExample CodeExampleFactory::create_query_example() {
    InteractiveCodeExample example("query_example", "Querying Multiple Component Types");
    
    example.description = "Learn how to query entities with multiple component types";
    example.initial_code = "Registry& registry = get_registry();\n\n// TODO: Query entities with both Transform and RigidBody components\n";
    example.solution_code = "Registry& registry = get_registry();\nregistry.for_each<Transform, RigidBody>([](Entity entity, Transform& transform, RigidBody& body) {\n    // Update physics\n    transform.position = transform.position + body.velocity;\n});";
    example.expected_output = "Executed system query on 3 entities\n--- Execution Summary ---\nEntities created: 3\nComponent types: 2";
    
    example.hints = {
        "Use registry.for_each<Component1, Component2>() for multiple components",
        "Lambda function should accept all specified component types",
        "Components are passed by reference so you can modify them"
    };
    
    example.current_code = example.initial_code;
    
    return example;
}

InteractiveCodeExample CodeExampleFactory::create_example_for_tutorial_step(const std::string& tutorial_id, const std::string& step_id) {
    // Create contextual examples based on tutorial step
    if (tutorial_id == "ecs_basics") {
        if (step_id == "entity_creation") {
            return create_entity_creation_example();
        } else if (step_id == "component_addition") {
            return create_component_addition_example();
        } else if (step_id == "system_iteration") {
            return create_system_iteration_example();
        }
    }
    
    // Default example
    return create_entity_creation_example();
}

// Validator implementations

std::function<bool(const std::string&)> CodeExampleFactory::create_entity_validator() {
    return [](const std::string& code) -> bool {
        // Check if code contains entity creation
        return code.find("create_entity") != std::string::npos;
    };
}

std::function<bool(const std::string&)> CodeExampleFactory::create_component_validator() {
    return [](const std::string& code) -> bool {
        // Check if code contains component addition
        return code.find("add_component") != std::string::npos && 
               code.find("Transform") != std::string::npos;
    };
}

std::function<bool(const std::string&)> CodeExampleFactory::create_system_validator() {
    return [](const std::string& code) -> bool {
        // Check if code contains system iteration
        return code.find("for_each") != std::string::npos;
    };
}

// Stub implementations for advanced examples
InteractiveCodeExample CodeExampleFactory::create_archetype_example() {
    return create_entity_creation_example(); // Placeholder
}

InteractiveCodeExample CodeExampleFactory::create_memory_optimization_example() {
    return create_entity_creation_example(); // Placeholder
}

InteractiveCodeExample CodeExampleFactory::create_performance_measurement_example() {
    return create_entity_creation_example(); // Placeholder
}

InteractiveCodeExample CodeExampleFactory::create_custom_allocator_example() {
    return create_entity_creation_example(); // Placeholder
}

InteractiveCodeExample CodeExampleFactory::create_parallel_system_example() {
    return create_entity_creation_example(); // Placeholder
}

InteractiveCodeExample CodeExampleFactory::create_sparse_set_example() {
    return create_entity_creation_example(); // Placeholder
}

} // namespace ecscope::learning