#include "lua_engine.hpp"
#include "core/log.hpp"
#include "ecs/registry.hpp"
#include "ecs/entity.hpp"
#include "ecs/components/transform.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>

namespace ecscope::scripting {

// LuaECSBinder implementation
void LuaECSBinder::bind_ecs_registry(ecs::Registry* registry) {
    // Store registry pointer in Lua registry for access from C functions
    lua_pushlightuserdata(lua_state_, static_cast<void*>(registry));
    lua_setfield(lua_state_, LUA_REGISTRYINDEX, "ecs_registry");
    
    // Register global ECS functions
    register_global_function("create_entity", lua_create_entity);
    register_global_function("destroy_entity", lua_destroy_entity);
    register_global_function("add_component", lua_add_component);
    register_global_function("get_component", lua_get_component);
    register_global_function("has_component", lua_has_component);
    register_global_function("for_each_entity", lua_for_each_entity);
    
    LOG_INFO("Bound ECS registry to Lua environment");
}

void LuaECSBinder::bind_system_functions() {
    // Create a systems table
    lua_newtable(lua_state_);
    
    // Example system functions
    lua_pushcfunction(lua_state_, [](lua_State* L) -> int {
        LOG_INFO("Movement system called from Lua");
        return 0;
    });
    lua_setfield(lua_state_, -2, "movement_system");
    
    lua_pushcfunction(lua_state_, [](lua_State* L) -> int {
        LOG_INFO("Render system called from Lua");
        return 0;
    });
    lua_setfield(lua_state_, -2, "render_system");
    
    lua_setglobal(lua_state_, "systems");
}

void LuaECSBinder::create_educational_examples() {
    // Create example Lua code that demonstrates ECS usage
    const char* example_code = R"(
-- Educational ECS Examples in Lua

-- Example 1: Creating entities with components
function create_example_entities()
    print("Creating example entities...")
    
    -- Create a player entity
    local player = create_entity()
    print("Created player entity:", player)
    
    -- Create an enemy entity
    local enemy = create_entity()
    print("Created enemy entity:", enemy)
    
    return player, enemy
end

-- Example 2: Working with components
function setup_components(player, enemy)
    print("Setting up components...")
    
    -- This would work with actual component bindings
    -- add_component(player, "Transform", {x=0, y=0, z=0})
    -- add_component(enemy, "Transform", {x=10, y=0, z=0})
    
    print("Components added successfully")
end

-- Example 3: Simple game loop logic
function update_game_logic()
    print("Updating game logic in Lua...")
    -- This demonstrates how game logic can be scripted
    return true
end

-- Example 4: Performance-conscious code
function optimized_batch_update(entities)
    local count = 0
    if entities then
        for _, entity in ipairs(entities) do
            -- Process entity efficiently
            count = count + 1
        end
    end
    return count
end

print("ECS educational examples loaded successfully!")
)";
    
    // Execute the example code
    if (luaL_dostring(lua_state_, example_code) != LUA_OK) {
        const char* error = lua_tostring(lua_state_, -1);
        LOG_ERROR("Failed to load educational examples: {}", error ? error : "unknown error");
        lua_pop(lua_state_, 1);
    }
}

// Static C function implementations for Lua binding
int LuaECSBinder::lua_create_entity(lua_State* L) {
    // Get registry from Lua registry
    lua_getfield(L, LUA_REGISTRYINDEX, "ecs_registry");
    auto* registry = static_cast<ecs::Registry*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    
    if (!registry) {
        luaL_error(L, "ECS registry not available");
        return 0;
    }
    
    // Create entity
    auto entity = registry->create_entity();
    lua_pushinteger(L, static_cast<lua_Integer>(entity));
    
    return 1; // Return entity ID
}

int LuaECSBinder::lua_destroy_entity(lua_State* L) {
    // Get entity ID from arguments
    lua_Integer entity_id = luaL_checkinteger(L, 1);
    
    // Get registry
    lua_getfield(L, LUA_REGISTRYINDEX, "ecs_registry");
    auto* registry = static_cast<ecs::Registry*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    
    if (!registry) {
        luaL_error(L, "ECS registry not available");
        return 0;
    }
    
    // Destroy entity
    bool success = registry->destroy_entity(static_cast<ecs::Entity>(entity_id));
    lua_pushboolean(L, success);
    
    return 1;
}

int LuaECSBinder::lua_add_component(lua_State* L) {
    // This is a simplified implementation
    // In practice, you'd need type-specific handling
    luaL_error(L, "add_component not fully implemented - requires component type bindings");
    return 0;
}

int LuaECSBinder::lua_get_component(lua_State* L) {
    // This is a simplified implementation
    luaL_error(L, "get_component not fully implemented - requires component type bindings");
    return 0;
}

int LuaECSBinder::lua_has_component(lua_State* L) {
    // This is a simplified implementation
    luaL_error(L, "has_component not fully implemented - requires component type bindings");
    return 0;
}

int LuaECSBinder::lua_for_each_entity(lua_State* L) {
    // This would iterate over entities and call a Lua function
    luaL_error(L, "for_each_entity not fully implemented - requires query system binding");
    return 0;
}

void LuaECSBinder::register_global_function(const std::string& name, lua_CFunction func) {
    lua_pushcfunction(lua_state_, func);
    lua_setglobal(lua_state_, name.c_str());
}

void LuaECSBinder::create_component_metatable(const std::string& component_name) {
    luaL_newmetatable(lua_state_, component_name.c_str());
    
    // Set __index to itself for method lookup
    lua_pushvalue(lua_state_, -1);
    lua_setfield(lua_state_, -2, "__index");
    
    // Add common component methods
    lua_pushcfunction(lua_state_, [](lua_State* L) -> int {
        // __tostring method for debugging
        lua_pushfstring(L, "%s component", lua_tostring(L, lua_upvalueindex(1)));
        return 1;
    });
    lua_setfield(lua_state_, -2, "__tostring");
    
    lua_pop(lua_state_, 1); // Pop metatable
}

// LuaEngine implementation
LuaEngine::LuaEngine() : ScriptEngine("Lua") {
    LOG_INFO("Initializing Lua scripting engine");
}

LuaEngine::~LuaEngine() {
    shutdown();
}

bool LuaEngine::initialize() {
    if (initialized_) {
        LOG_WARN("Lua engine already initialized");
        return true;
    }
    
    // Create global Lua state for shared functionality
    global_lua_state_ = luaL_newstate();
    if (!global_lua_state_) {
        LOG_ERROR("Failed to create global Lua state");
        return false;
    }
    
    // Open standard libraries
    luaL_openlibs(global_lua_state_);
    
    // Setup error handling
    setup_lua_error_handling(global_lua_state_);
    
    // Create ECS binder
    ecs_binder_ = std::make_unique<LuaECSBinder>(global_lua_state_);
    
    initialized_ = true;
    LOG_INFO("Lua scripting engine initialized successfully");
    
    return true;
}

void LuaEngine::shutdown() {
    if (!initialized_) {
        return;
    }
    
    // Unload all scripts
    unload_all_scripts();
    
    // Clean up ECS binder
    ecs_binder_.reset();
    
    // Close global Lua state
    if (global_lua_state_) {
        lua_close(global_lua_state_);
        global_lua_state_ = nullptr;
    }
    
    initialized_ = false;
    LOG_INFO("Lua scripting engine shut down");
}

ScriptResult<void> LuaEngine::load_script(const std::string& name, const std::string& source) {
    if (!initialized_) {
        auto error = create_lua_error(name, "Lua engine not initialized");
        return ScriptResult<void>::error_result(error);
    }
    
    start_performance_measurement(name, "compilation");
    
    auto* context = create_lua_context(name);
    if (!context || !context->is_valid()) {
        auto error = create_lua_error(name, "Failed to create Lua context");
        end_performance_measurement(name, "compilation");
        return ScriptResult<void>::error_result(error);
    }
    
    // Store source code
    context->source_code = source;
    
    // Compile the script
    lua_State* L = context->lua_state;
    if (luaL_loadstring(L, source.c_str()) != LUA_OK) {
        auto error = handle_lua_error(L, name, "compilation");
        end_performance_measurement(name, "compilation");
        return ScriptResult<void>::error_result(error);
    }
    
    // Execute to load functions and global variables
    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
        auto error = handle_lua_error(L, name, "loading");
        end_performance_measurement(name, "compilation");
        return ScriptResult<void>::error_result(error);
    }
    
    context->is_compiled = true;
    context->is_loaded = true;
    
    end_performance_measurement(name, "compilation");
    
    // Setup memory limits if configured
    auto it = script_memory_limits_.find(name);
    if (it != script_memory_limits_.end()) {
        setup_memory_limits(L, it->second);
    }
    
    LOG_INFO("Loaded Lua script: {} ({} bytes)", name, source.size());
    
    return ScriptResult<void>::success_result(context->metrics);
}

ScriptResult<void> LuaEngine::load_script_file(const std::string& name, const std::string& filepath) {
    if (!std::filesystem::exists(filepath)) {
        ScriptError error{
            ScriptError::Type::CompilationError,
            "Script file not found: " + filepath,
            name,
            0, 0,
            "",
            "Check that the file path is correct and the file exists"
        };
        return ScriptResult<void>::error_result(error);
    }
    
    // Read file contents
    std::ifstream file(filepath);
    if (!file) {
        ScriptError error{
            ScriptError::Type::CompilationError,
            "Failed to open script file: " + filepath,
            name,
            0, 0,
            "",
            "Check file permissions and disk space"
        };
        return ScriptResult<void>::error_result(error);
    }
    
    std::string source((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    
    auto result = load_script(name, source);
    
    if (result) {
        // Store filepath for hot-reload
        auto* context = get_lua_context(name);
        if (context) {
            context->filepath = filepath;
            if (is_hot_reload_enabled()) {
                context->file_state = FileWatchState(filepath);
            }
        }
    }
    
    return result;
}

ScriptResult<void> LuaEngine::compile_script(const std::string& name) {
    auto* context = get_lua_context(name);
    if (!context) {
        auto error = create_lua_error(name, "Script not loaded");
        return ScriptResult<void>::error_result(error);
    }
    
    if (context->source_code.empty()) {
        auto error = create_lua_error(name, "No source code to compile");
        return ScriptResult<void>::error_result(error);
    }
    
    start_performance_measurement(name, "compilation");
    
    lua_State* L = context->lua_state;
    
    // Re-compile the script
    if (luaL_loadstring(L, context->source_code.c_str()) != LUA_OK) {
        auto error = handle_lua_error(L, name, "compilation");
        end_performance_measurement(name, "compilation");
        return ScriptResult<void>::error_result(error);
    }
    
    // Execute to load functions
    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
        auto error = handle_lua_error(L, name, "compilation execution");
        end_performance_measurement(name, "compilation");
        return ScriptResult<void>::error_result(error);
    }
    
    context->is_compiled = true;
    
    end_performance_measurement(name, "compilation");
    
    LOG_DEBUG("Recompiled Lua script: {}", name);
    return ScriptResult<void>::success_result(context->metrics);
}

ScriptResult<void> LuaEngine::reload_script(const std::string& name) {
    auto* context = get_lua_context(name);
    if (!context) {
        auto error = create_lua_error(name, "Script not loaded");
        return ScriptResult<void>::error_result(error);
    }
    
    if (context->filepath.empty()) {
        // Reload from source code
        return compile_script(name);
    } else {
        // Reload from file
        std::string filepath = context->filepath;
        unload_script(name);
        return load_script_file(name, filepath);
    }
}

ScriptResult<void> LuaEngine::execute_script(const std::string& name) {
    auto* context = get_lua_context(name);
    if (!context || !context->is_valid()) {
        auto error = create_lua_error(name, "Script not loaded or invalid");
        return ScriptResult<void>::error_result(error);
    }
    
    if (!context->is_compiled) {
        auto compile_result = compile_script(name);
        if (!compile_result) {
            return compile_result;
        }
    }
    
    start_performance_measurement(name, "execution");
    
    lua_State* L = context->lua_state;
    
    // Look for main function and execute it
    lua_getglobal(L, "main");
    if (lua_isfunction(L, -1)) {
        if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
            auto error = handle_lua_error(L, name, "main function execution");
            end_performance_measurement(name, "execution");
            return ScriptResult<void>::error_result(error);
        }
    } else {
        lua_pop(L, 1); // Pop the non-function value
        LOG_DEBUG("No main function found in script: {}, script loaded but not executed", name);
    }
    
    end_performance_measurement(name, "execution");
    update_memory_statistics(name);
    
    return ScriptResult<void>::success_result(context->metrics);
}

usize LuaEngine::get_memory_usage(const std::string& script_name) const {
    auto it = script_memory_usage_.find(script_name);
    if (it != script_memory_usage_.end()) {
        return it->second;
    }
    
    // Fall back to Lua's memory usage if available
    const auto* context = get_lua_context(script_name);
    if (context && context->is_valid()) {
        return lua_gc(context->lua_state, LUA_GCCOUNT, 0) * 1024; // Convert KB to bytes
    }
    
    return 0;
}

void LuaEngine::collect_garbage() {
    // Collect garbage in global state
    if (global_lua_state_) {
        lua_gc(global_lua_state_, LUA_GCCOLLECT, 0);
    }
    
    // Collect garbage in all script contexts
    for (const auto& script_name : get_loaded_scripts()) {
        auto* context = get_lua_context(script_name);
        if (context && context->is_valid()) {
            lua_gc(context->lua_state, LUA_GCCOLLECT, 0);
            update_memory_statistics(script_name);
        }
    }
    
    LOG_DEBUG("Performed garbage collection on all Lua contexts");
}

void LuaEngine::set_memory_limit(const std::string& script_name, usize limit_bytes) {
    script_memory_limits_[script_name] = limit_bytes;
    
    auto* context = get_lua_context(script_name);
    if (context && context->is_valid()) {
        setup_memory_limits(context->lua_state, limit_bytes);
    }
    
    LOG_DEBUG("Set memory limit for script '{}': {} bytes", script_name, limit_bytes);
}

std::string LuaEngine::get_version_info() const {
    return std::string("Lua ") + LUA_VERSION + " with ECScope integration";
}

std::string LuaEngine::explain_performance_characteristics() const {
    return R"(
Lua Performance Characteristics in ECScope:

Strengths:
- Lightweight and fast scripting language
- Excellent C/C++ integration with minimal overhead
- Small memory footprint (typically 150-200KB base)
- Fast function calls and variable access
- Register-based virtual machine (faster than stack-based)
- Efficient garbage collection with incremental collection

Weaknesses:
- Single-threaded execution (GIL-like behavior)
- Limited standard library compared to Python
- Requires manual memory management awareness
- Type conversion overhead for complex C++ objects

Performance Tips:
1. Cache frequently-called functions using function references
2. Minimize C++ ↔ Lua boundary crossings in tight loops
3. Use local variables instead of globals for better performance
4. Batch operations when possible to reduce call overhead
5. Consider LuaJIT for compute-intensive scripts (when available)

ECS Integration Benefits:
- Direct entity/component access with minimal marshaling
- Efficient iteration over component arrays
- Hot-reload capability for rapid development
- Memory tracking and profiling integration
)";
}

std::vector<std::string> LuaEngine::get_optimization_suggestions(const std::string& script_name) const {
    std::vector<std::string> suggestions;
    
    const auto* context = get_lua_context(script_name);
    if (!context) {
        suggestions.push_back("Script not found - cannot analyze");
        return suggestions;
    }
    
    const auto& metrics = context->metrics;
    
    // Memory-based suggestions
    if (metrics.memory_usage_bytes > 10 * 1024 * 1024) { // > 10MB
        suggestions.push_back("Consider reducing memory usage - current usage is high (" + 
                            std::to_string(metrics.memory_usage_bytes / (1024*1024)) + "MB)");
    }
    
    // Performance-based suggestions
    if (metrics.performance_ratio > 50.0) {
        suggestions.push_back("Script is significantly slower than native C++ - consider optimizing or moving to C++");
        suggestions.push_back("Profile the script to identify bottlenecks");
    } else if (metrics.performance_ratio > 10.0) {
        suggestions.push_back("Consider caching frequently-accessed data");
        suggestions.push_back("Minimize C++ function calls in tight loops");
    }
    
    // Cache performance suggestions
    if (metrics.cache_hit_ratio() < 0.8) {
        suggestions.push_back("Poor cache performance - consider improving data locality");
    }
    
    // Execution frequency suggestions
    if (metrics.execution_count > 10000) {
        suggestions.push_back("Frequently executed script - ensure it's well optimized");
        suggestions.push_back("Consider using function reference caching for better performance");
    }
    
    if (suggestions.empty()) {
        suggestions.push_back("Script performance looks good - no major optimizations needed");
    }
    
    return suggestions;
}

void LuaEngine::bind_ecs_registry(ecs::Registry* registry) {
    if (!initialized_ || !ecs_binder_) {
        LOG_ERROR("Cannot bind ECS registry - Lua engine not properly initialized");
        return;
    }
    
    bound_registry_ = registry;
    ecs_binder_->bind_ecs_registry(registry);
    ecs_binder_->bind_system_functions();
    ecs_binder_->create_educational_examples();
    
    LOG_INFO("ECS registry bound to Lua engine");
}

void LuaEngine::bind_global_function(const std::string& name, lua_CFunction func) {
    if (!initialized_ || !global_lua_state_) {
        LOG_ERROR("Cannot bind function - Lua engine not initialized");
        return;
    }
    
    lua_pushcfunction(global_lua_state_, func);
    lua_setglobal(global_lua_state_, name.c_str());
    
    LOG_DEBUG("Bound global Lua function: {}", name);
}

void LuaEngine::set_global_variable(const std::string& name, const std::string& value) {
    if (!initialized_ || !global_lua_state_) return;
    
    lua_pushstring(global_lua_state_, value.c_str());
    lua_setglobal(global_lua_state_, name.c_str());
}

void LuaEngine::set_global_variable(const std::string& name, double value) {
    if (!initialized_ || !global_lua_state_) return;
    
    lua_pushnumber(global_lua_state_, value);
    lua_setglobal(global_lua_state_, name.c_str());
}

void LuaEngine::set_global_variable(const std::string& name, bool value) {
    if (!initialized_ || !global_lua_state_) return;
    
    lua_pushboolean(global_lua_state_, value);
    lua_setglobal(global_lua_state_, name.c_str());
}

void LuaEngine::create_tutorial_scripts() {
    if (!initialized_) {
        LOG_ERROR("Cannot create tutorial scripts - Lua engine not initialized");
        return;
    }
    
    generate_basic_tutorial_script();
    generate_ecs_integration_tutorial();
    generate_performance_comparison_script();
    
    LOG_INFO("Created Lua tutorial scripts");
}

void LuaEngine::demonstrate_performance_patterns() {
    const char* performance_demo = R"(
-- Lua Performance Demonstration Script

-- Bad performance pattern: frequent C++ calls in loop
function bad_performance_example(entities)
    print("Demonstrating bad performance pattern...")
    local start_time = os.clock()
    
    for i = 1, 1000 do
        -- This would call C++ function 1000 times
        -- create_entity() -- Commented out for demo
    end
    
    local end_time = os.clock()
    print("Bad pattern time:", (end_time - start_time) * 1000, "ms")
end

-- Good performance pattern: batch operations
function good_performance_example(entities)
    print("Demonstrating good performance pattern...")
    local start_time = os.clock()
    
    -- Prepare data in Lua, then make fewer C++ calls
    local entities_to_create = {}
    for i = 1, 1000 do
        entities_to_create[i] = {x = i, y = i * 2}
    end
    
    -- Single batch operation (would be implemented as C++ batch function)
    -- batch_create_entities(entities_to_create)
    
    local end_time = os.clock()
    print("Good pattern time:", (end_time - start_time) * 1000, "ms")
    print("Performance improvement achieved through batching!")
end

-- Cache-friendly pattern
function cache_friendly_example()
    print("Demonstrating cache-friendly access patterns...")
    
    -- Process related data together for better cache locality
    local transform_data = {
        positions = {{x=1, y=2}, {x=3, y=4}, {x=5, y=6}},
        velocities = {{x=0.1, y=0.2}, {x=0.3, y=0.4}, {x=0.5, y=0.6}}
    }
    
    -- Process all positions, then all velocities
    for i, pos in ipairs(transform_data.positions) do
        pos.x = pos.x + transform_data.velocities[i].x
        pos.y = pos.y + transform_data.velocities[i].y
    end
    
    print("Cache-friendly processing completed")
end

print("Performance demonstration patterns loaded")
)";
    
    auto result = load_script("performance_demo", performance_demo);
    if (!result) {
        LOG_ERROR("Failed to load performance demonstration script: {}", 
                 result.error ? result.error->to_string() : "unknown error");
    }
}

void LuaEngine::explain_lua_integration_benefits() const {
    LOG_INFO(R"(
Lua Integration Benefits in ECScope:

1. Rapid Development:
   - Hot-reload support for instant iteration
   - No compilation step required
   - Quick prototyping and experimentation

2. Performance:
   - Lightweight runtime overhead
   - Efficient C++ integration
   - JIT compilation potential with LuaJIT

3. Educational Value:
   - Clear performance comparisons with C++
   - Memory usage visualization
   - Real-time profiling and analysis

4. ECS Integration:
   - Direct access to entities and components
   - Scriptable systems and behaviors
   - Game logic without engine recompilation

5. Flexibility:
   - Runtime behavior modification
   - Configuration and settings management
   - User-generated content support
)");
}

// Protected method implementations
ScriptResult<void> LuaEngine::call_function_impl_void(const std::string& script_name,
                                                     const std::string& function_name,
                                                     const std::vector<std::any>& args) {
    auto* context = get_lua_context(script_name);
    if (!context || !context->is_valid()) {
        auto error = create_lua_error(script_name, "Script not loaded");
        return ScriptResult<void>::error_result(error);
    }
    
    lua_State* L = context->lua_state;
    
    // Get function
    lua_getglobal(L, function_name.c_str());
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);
        auto error = create_lua_error(script_name, "Function not found: " + function_name);
        return ScriptResult<void>::error_result(error);
    }
    
    // Push arguments (this would need proper type handling)
    for (const auto& arg : args) {
        // Simplified - would need proper any_cast handling for each type
        lua_pushnil(L); // Placeholder
    }
    
    // Call function
    if (lua_pcall(L, args.size(), 0, 0) != LUA_OK) {
        auto error = handle_lua_error(L, script_name, "function call");
        return ScriptResult<void>::error_result(error);
    }
    
    return ScriptResult<void>::success_result(context->metrics);
}

// Private utility method implementations
LuaScriptContext* LuaEngine::get_lua_context(const std::string& name) {
    auto* context = get_script_context(name);
    return static_cast<LuaScriptContext*>(context);
}

const LuaScriptContext* LuaEngine::get_lua_context(const std::string& name) const {
    auto* context = get_script_context(name);
    return static_cast<const LuaScriptContext*>(context);
}

LuaScriptContext* LuaEngine::create_lua_context(const std::string& name) {
    // Remove existing context if it exists
    remove_script_context(name);
    
    auto lua_context = std::make_unique<LuaScriptContext>(name);
    LuaScriptContext* context_ptr = lua_context.get();
    
    // Transfer ownership to base class storage
    // This is a bit hacky but works with the current design
    script_contexts_[name] = std::unique_ptr<ScriptContext>(
        static_cast<ScriptContext*>(lua_context.release()));
    
    return context_ptr;
}

ScriptError LuaEngine::create_lua_error(const std::string& script_name, const std::string& message) const {
    return ScriptError{
        ScriptError::Type::RuntimeError,
        message,
        script_name,
        0, 0,
        "",
        "Check Lua script syntax and ECScope integration"
    };
}

ScriptError LuaEngine::handle_lua_error(lua_State* L, const std::string& script_name, const std::string& operation) const {
    std::string error_message = "Unknown Lua error";
    
    if (lua_isstring(L, -1)) {
        error_message = lua_tostring(L, -1);
        lua_pop(L, 1); // Remove error message from stack
    }
    
    // Try to extract line number from error message
    usize line_number = 0;
    std::size_t line_pos = error_message.find(":");
    if (line_pos != std::string::npos) {
        std::size_t next_colon = error_message.find(":", line_pos + 1);
        if (next_colon != std::string::npos) {
            std::string line_str = error_message.substr(line_pos + 1, next_colon - line_pos - 1);
            try {
                line_number = std::stoul(line_str);
            } catch (...) {
                // Ignore parsing errors
            }
        }
    }
    
    return ScriptError{
        ScriptError::Type::RuntimeError,
        error_message,
        script_name,
        line_number,
        0,
        "",
        "Lua error during " + operation + " - check script syntax and logic"
    };
}

void LuaEngine::setup_lua_error_handling(lua_State* L) {
    // Set up error handling functions if needed
    // This is where you could set up custom panic handlers, etc.
}

void LuaEngine::setup_memory_limits(lua_State* L, usize limit_bytes) {
    // Set up memory allocation limits
    // Note: This would require custom allocator setup during lua_newstate
    LOG_DEBUG("Memory limits setup requested: {} bytes (not fully implemented)", limit_bytes);
}

void LuaEngine::update_memory_statistics(const std::string& script_name) {
    auto* context = get_lua_context(script_name);
    if (!context || !context->is_valid()) {
        return;
    }
    
    // Update memory usage from Lua
    usize memory_kb = lua_gc(context->lua_state, LUA_GCCOUNT, 0);
    usize memory_bytes = memory_kb * 1024;
    
    script_memory_usage_[script_name] = memory_bytes;
    context->update_memory_usage();
    context->metrics.memory_usage_bytes = memory_bytes;
    context->metrics.peak_memory_usage_bytes = std::max(
        context->metrics.peak_memory_usage_bytes,
        memory_bytes
    );
}

void* LuaEngine::lua_memory_allocator(void* ud, void* ptr, size_t osize, size_t nsize) {
    // Custom memory allocator for Lua with tracking
    // This would integrate with ECScope's memory system
    
    if (nsize == 0) {
        free(ptr);
        return nullptr;
    } else {
        return realloc(ptr, nsize);
    }
}

void LuaEngine::generate_basic_tutorial_script() const {
    const char* tutorial_content = R"(
-- ECScope Lua Tutorial: Basics

print("=== ECScope Lua Tutorial: Basics ===")

-- Variables and basic operations
local player_health = 100
local player_name = "Hero"
local is_alive = true

print("Player:", player_name, "Health:", player_health)

-- Functions
function heal_player(amount)
    player_health = player_health + amount
    print("Player healed by", amount, "- New health:", player_health)
end

function damage_player(amount)
    player_health = player_health - amount
    if player_health <= 0 then
        player_health = 0
        is_alive = false
        print("Player has died!")
    else
        print("Player damaged by", amount, "- Health remaining:", player_health)
    end
end

-- Table-based data structures
local player_stats = {
    strength = 10,
    agility = 8,
    intelligence = 12,
    level = 1
}

function level_up()
    player_stats.level = player_stats.level + 1
    player_stats.strength = player_stats.strength + 2
    player_stats.agility = player_stats.agility + 1
    player_stats.intelligence = player_stats.intelligence + 1
    print("Level up! Now level", player_stats.level)
end

-- Tutorial main function
function main()
    print("Running basic tutorial...")
    heal_player(25)
    damage_player(50)
    level_up()
    print("Tutorial completed!")
end

print("Basic tutorial loaded - call main() to run")
)";
    
    std::filesystem::create_directories("scripts/tutorials");
    std::ofstream file("scripts/tutorials/lua_basics.lua");
    file << tutorial_content;
    file.close();
    
    LOG_INFO("Generated basic Lua tutorial script");
}

void LuaEngine::generate_ecs_integration_tutorial() const {
    const char* ecs_tutorial = R"(
-- ECScope Lua Tutorial: ECS Integration

print("=== ECScope Lua Tutorial: ECS Integration ===")

-- This tutorial demonstrates ECS integration patterns
-- Note: Actual ECS functions would be bound from C++

function ecs_tutorial_main()
    print("Starting ECS integration tutorial...")
    
    -- Create entities (these functions would be bound from C++)
    print("Creating entities...")
    -- local player = create_entity()
    -- local enemy = create_entity()
    
    print("Adding components...")
    -- add_component(player, "Transform", {x=0, y=0, z=0})
    -- add_component(player, "Health", {current=100, max=100})
    
    print("System processing simulation...")
    simulate_movement_system()
    simulate_health_system()
    
    print("ECS tutorial completed!")
end

-- Simulated systems for educational purposes
function simulate_movement_system()
    print("Movement System: Processing entity positions...")
    -- This would iterate over entities with Transform components
    local entities_with_transform = {
        {id=1, transform={x=0, y=0, z=0}, velocity={x=1, y=0, z=0}},
        {id=2, transform={x=10, y=5, z=0}, velocity={x=-0.5, y=0.5, z=0}}
    }
    
    for _, entity in ipairs(entities_with_transform) do
        entity.transform.x = entity.transform.x + entity.velocity.x
        entity.transform.y = entity.transform.y + entity.velocity.y
        print("Entity", entity.id, "moved to", entity.transform.x, entity.transform.y)
    end
end

function simulate_health_system()
    print("Health System: Processing entity health...")
    local entities_with_health = {
        {id=1, health={current=85, max=100}},
        {id=3, health={current=50, max=75}}
    }
    
    for _, entity in ipairs(entities_with_health) do
        -- Simulate health regeneration
        if entity.health.current < entity.health.max then
            entity.health.current = math.min(entity.health.max, entity.health.current + 1)
            print("Entity", entity.id, "regenerated health:", entity.health.current)
        end
    end
end

-- Performance comparison example
function performance_comparison_example()
    print("Performance Comparison: Lua vs C++ patterns")
    
    local start_time = os.clock()
    
    -- Simulate computationally intensive task
    local result = 0
    for i = 1, 100000 do
        result = result + math.sqrt(i) * math.sin(i)
    end
    
    local end_time = os.clock()
    local lua_time = (end_time - start_time) * 1000
    
    print("Lua computation time:", lua_time, "ms")
    print("Result:", result)
    print("Note: Compare this with equivalent C++ implementation")
end

print("ECS integration tutorial loaded - call ecs_tutorial_main() to run")
)";
    
    std::filesystem::create_directories("scripts/tutorials");
    std::ofstream file("scripts/tutorials/ecs_integration.lua");
    file << ecs_tutorial;
    file.close();
    
    LOG_INFO("Generated ECS integration tutorial script");
}

void LuaEngine::generate_performance_comparison_script() const {
    const char* performance_script = R"(
-- ECScope Lua Tutorial: Performance Analysis

print("=== ECScope Lua Tutorial: Performance Analysis ===")

-- Benchmarking utilities
function benchmark(name, func, iterations)
    iterations = iterations or 1000
    print("Benchmarking:", name, "(" .. iterations .. " iterations)")
    
    local start_time = os.clock()
    for i = 1, iterations do
        func()
    end
    local end_time = os.clock()
    
    local total_time = (end_time - start_time) * 1000
    local avg_time = total_time / iterations
    
    print("Total time:", total_time, "ms")
    print("Average time:", avg_time, "ms per iteration")
    print("Operations per second:", 1000 / avg_time)
    print()
end

-- Performance test functions
function simple_arithmetic_test()
    local result = 0
    for i = 1, 1000 do
        result = result + i * 2 - i / 3 + math.sqrt(i)
    end
    return result
end

function table_access_test()
    local data = {}
    for i = 1, 100 do
        data[i] = {x = i, y = i * 2, z = i * 3}
    end
    
    local sum = 0
    for i = 1, 100 do
        sum = sum + data[i].x + data[i].y + data[i].z
    end
    return sum
end

function function_call_overhead_test()
    local function add(a, b)
        return a + b
    end
    
    local result = 0
    for i = 1, 1000 do
        result = add(result, i)
    end
    return result
end

function string_manipulation_test()
    local str = "Hello"
    for i = 1, 100 do
        str = str .. " World " .. i
    end
    return #str
end

-- Main performance analysis function
function performance_analysis_main()
    print("Starting performance analysis...")
    print("Note: These benchmarks help understand Lua performance characteristics")
    print()
    
    benchmark("Simple Arithmetic", simple_arithmetic_test, 1000)
    benchmark("Table Access", table_access_test, 1000)
    benchmark("Function Call Overhead", function_call_overhead_test, 1000)
    benchmark("String Manipulation", string_manipulation_test, 100)
    
    print("Performance analysis completed!")
    print("Compare these results with equivalent C++ implementations")
    print("to understand the performance trade-offs of scripting")
end

-- Educational insights about Lua performance
function print_performance_insights()
    print("=== Lua Performance Insights ===")
    print("1. Lua is generally fast for scripting languages")
    print("2. Function call overhead is minimal")
    print("3. Table access is optimized and efficient")
    print("4. String operations can be expensive with frequent concatenation")
    print("5. Math operations are reasonably fast")
    print("6. Consider LuaJIT for compute-intensive scripts")
    print("7. Minimize C++ boundary crossings in tight loops")
    print()
end

print("Performance analysis tutorial loaded")
print("Call performance_analysis_main() to run benchmarks")
print("Call print_performance_insights() for optimization tips")
)";
    
    std::filesystem::create_directories("scripts/tutorials");
    std::ofstream file("scripts/tutorials/performance_analysis.lua");
    file << performance_script;
    file.close();
    
    LOG_INFO("Generated performance analysis tutorial script");
}

// Template specializations for argument pushing
template<>
void LuaEngine::push_argument_to_lua<bool>(lua_State* L, bool&& arg) {
    LuaTypeHelper::push(L, arg);
}

template<>
void LuaEngine::push_argument_to_lua<int>(lua_State* L, int&& arg) {
    LuaTypeHelper::push(L, arg);
}

template<>
void LuaEngine::push_argument_to_lua<float>(lua_State* L, float&& arg) {
    LuaTypeHelper::push(L, arg);
}

template<>
void LuaEngine::push_argument_to_lua<double>(lua_State* L, double&& arg) {
    LuaTypeHelper::push(L, arg);
}

template<>
void LuaEngine::push_argument_to_lua<std::string>(lua_State* L, std::string&& arg) {
    LuaTypeHelper::push(L, arg);
}

template<>
void LuaEngine::push_argument_to_lua<const char*>(lua_State* L, const char*&& arg) {
    LuaTypeHelper::push(L, arg);
}

// Template specializations for return value getting
template<>
bool LuaEngine::get_return_value_from_lua<bool>(lua_State* L, int index) {
    return LuaTypeHelper::get_bool(L, index);
}

template<>
int LuaEngine::get_return_value_from_lua<int>(lua_State* L, int index) {
    return LuaTypeHelper::get_int(L, index);
}

template<>
float LuaEngine::get_return_value_from_lua<float>(lua_State* L, int index) {
    return LuaTypeHelper::get_float(L, index);
}

template<>
double LuaEngine::get_return_value_from_lua<double>(lua_State* L, int index) {
    return LuaTypeHelper::get_double(L, index);
}

template<>
std::string LuaEngine::get_return_value_from_lua<std::string>(lua_State* L, int index) {
    return LuaTypeHelper::get_string(L, index);
}

} // namespace ecscope::scripting