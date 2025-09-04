#pragma once

#include "script_engine.hpp"
#include <lua.hpp>
#include <unordered_map>
#include <functional>
#include <type_traits>

namespace ecscope::scripting {

// Forward declarations for ECS integration
namespace ecs { class Registry; }

/**
 * @brief Lua-specific script context with state management
 */
struct LuaScriptContext : ScriptContext {
    lua_State* lua_state{nullptr};
    bool owns_state{true};
    std::unordered_map<std::string, int> function_refs; // Function reference cache
    
    LuaScriptContext(const std::string& name) 
        : ScriptContext(name, "Lua") {
        // Initialize Lua state
        lua_state = luaL_newstate();
        if (lua_state) {
            luaL_openlibs(lua_state);
        }
    }
    
    ~LuaScriptContext() {
        if (lua_state && owns_state) {
            lua_close(lua_state);
        }
    }
    
    // Non-copyable, movable
    LuaScriptContext(const LuaScriptContext&) = delete;
    LuaScriptContext& operator=(const LuaScriptContext&) = delete;
    LuaScriptContext(LuaScriptContext&& other) noexcept 
        : ScriptContext(std::move(other))
        , lua_state(other.lua_state)
        , owns_state(other.owns_state)
        , function_refs(std::move(other.function_refs)) {
        other.lua_state = nullptr;
        other.owns_state = false;
    }
    
    bool is_valid() const {
        return lua_state != nullptr;
    }
    
    void cache_function_ref(const std::string& function_name) {
        if (!lua_state) return;
        
        lua_getglobal(lua_state, function_name.c_str());
        if (lua_isfunction(lua_state, -1)) {
            function_refs[function_name] = luaL_ref(lua_state, LUA_REGISTRYINDEX);
        } else {
            lua_pop(lua_state, 1); // Pop the non-function value
        }
    }
    
    bool call_cached_function(const std::string& function_name, int nargs = 0, int nresults = 0) {
        if (!lua_state) return false;
        
        auto it = function_refs.find(function_name);
        if (it == function_refs.end()) {
            return false;
        }
        
        lua_rawgeti(lua_state, LUA_REGISTRYINDEX, it->second);
        if (!lua_isfunction(lua_state, -1)) {
            lua_pop(lua_state, 1);
            return false;
        }
        
        // Move function to correct position if there are arguments
        if (nargs > 0) {
            lua_insert(lua_state, -(nargs + 1));
        }
        
        return lua_pcall(lua_state, nargs, nresults, 0) == LUA_OK;
    }
};

/**
 * @brief Type-safe Lua binding utilities
 */
class LuaTypeHelper {
public:
    // Push C++ values to Lua stack
    static void push(lua_State* L, bool value) { lua_pushboolean(L, value); }
    static void push(lua_State* L, int value) { lua_pushinteger(L, value); }
    static void push(lua_State* L, float value) { lua_pushnumber(L, value); }
    static void push(lua_State* L, double value) { lua_pushnumber(L, value); }
    static void push(lua_State* L, const char* value) { lua_pushstring(L, value); }
    static void push(lua_State* L, const std::string& value) { lua_pushstring(L, value.c_str()); }
    
    template<typename T>
    static void push(lua_State* L, T* ptr) {
        if (ptr) {
            lua_pushlightuserdata(L, static_cast<void*>(ptr));
        } else {
            lua_pushnil(L);
        }
    }
    
    // Get C++ values from Lua stack
    static bool get_bool(lua_State* L, int index) { 
        return lua_toboolean(L, index); 
    }
    
    static int get_int(lua_State* L, int index) { 
        return static_cast<int>(luaL_checkinteger(L, index)); 
    }
    
    static float get_float(lua_State* L, int index) { 
        return static_cast<float>(luaL_checknumber(L, index)); 
    }
    
    static double get_double(lua_State* L, int index) { 
        return luaL_checknumber(L, index); 
    }
    
    static std::string get_string(lua_State* L, int index) { 
        size_t len;
        const char* str = luaL_checklstring(L, index, &len);
        return std::string(str, len);
    }
    
    template<typename T>
    static T* get_ptr(lua_State* L, int index) {
        return static_cast<T*>(lua_touserdata(L, index));
    }
    
    // Type checking
    static bool is_bool(lua_State* L, int index) { return lua_isboolean(L, index); }
    static bool is_int(lua_State* L, int index) { return lua_isinteger(L, index); }
    static bool is_number(lua_State* L, int index) { return lua_isnumber(L, index); }
    static bool is_string(lua_State* L, int index) { return lua_isstring(L, index); }
    static bool is_userdata(lua_State* L, int index) { return lua_isuserdata(L, index); }
    static bool is_table(lua_State* L, int index) { return lua_istable(L, index); }
    static bool is_function(lua_State* L, int index) { return lua_isfunction(L, index); }
};

/**
 * @brief Automatic ECS component binding for Lua
 */
class LuaECSBinder {
public:
    explicit LuaECSBinder(lua_State* L) : lua_state_(L) {}
    
    // Bind ECS registry and basic operations
    void bind_ecs_registry(ecs::Registry* registry);
    
    // Bind common component types
    template<typename Component>
    void bind_component(const std::string& component_name);
    
    // Bind system functions
    void bind_system_functions();
    
    // Educational binding examples
    void create_educational_examples();

private:
    lua_State* lua_state_;
    
    // Internal registry binding functions
    static int lua_create_entity(lua_State* L);
    static int lua_destroy_entity(lua_State* L);
    static int lua_add_component(lua_State* L);
    static int lua_get_component(lua_State* L);
    static int lua_has_component(lua_State* L);
    static int lua_for_each_entity(lua_State* L);
    
    // Utility functions
    void register_global_function(const std::string& name, lua_CFunction func);
    void create_component_metatable(const std::string& component_name);
    
    template<typename T>
    void push_component_to_lua(lua_State* L, const T& component);
    
    template<typename T>
    T get_component_from_lua(lua_State* L, int index);
};

/**
 * @brief Comprehensive Lua scripting engine with educational focus
 * 
 * Features:
 * - Full ECS integration with automatic bindings
 * - Performance monitoring and comparison
 * - Memory-safe script execution with limits
 * - Hot-reload support with validation
 * - Educational examples and tutorials
 * - Type-safe C++ <-> Lua interop
 */
class LuaEngine : public ScriptEngine {
public:
    LuaEngine();
    ~LuaEngine() override;
    
    // ScriptEngine interface implementation
    bool initialize() override;
    void shutdown() override;
    bool is_initialized() const override { return initialized_; }
    
    // Script loading and compilation
    ScriptResult<void> load_script(const std::string& name, const std::string& source) override;
    ScriptResult<void> load_script_file(const std::string& name, const std::string& filepath) override;
    ScriptResult<void> compile_script(const std::string& name) override;
    ScriptResult<void> reload_script(const std::string& name) override;
    
    // Script execution
    ScriptResult<void> execute_script(const std::string& name) override;
    
    // Memory management
    usize get_memory_usage(const std::string& script_name) const override;
    void collect_garbage() override;
    void set_memory_limit(const std::string& script_name, usize limit_bytes) override;
    
    // Engine information
    std::string get_version_info() const override;
    std::string explain_performance_characteristics() const override;
    std::vector<std::string> get_optimization_suggestions(const std::string& script_name) const override;
    
    // Lua-specific functionality
    void bind_ecs_registry(ecs::Registry* registry);
    void bind_global_function(const std::string& name, lua_CFunction func);
    void set_global_variable(const std::string& name, const std::string& value);
    void set_global_variable(const std::string& name, double value);
    void set_global_variable(const std::string& name, bool value);
    
    // Educational features
    void create_tutorial_scripts();
    void demonstrate_performance_patterns();
    void explain_lua_integration_benefits() const;
    
    // Advanced Lua integration
    template<typename... Args>
    ScriptResult<void> call_lua_function(const std::string& script_name, 
                                        const std::string& function_name,
                                        Args&&... args);
    
    template<typename ReturnType, typename... Args>
    ScriptResult<ReturnType> call_lua_function_with_return(const std::string& script_name,
                                                          const std::string& function_name,
                                                          Args&&... args);
    
    // Job system integration
    void enable_parallel_execution(bool enable) { parallel_execution_enabled_ = enable; }
    bool is_parallel_execution_enabled() const { return parallel_execution_enabled_; }
    
protected:
    // ScriptEngine protected interface
    ScriptResult<void> call_function_impl_void(const std::string& script_name,
                                              const std::string& function_name,
                                              const std::vector<std::any>& args) override;
    
    template<typename ReturnType>
    ScriptResult<ReturnType> call_function_impl_typed(const std::string& script_name,
                                                     const std::string& function_name,
                                                     const std::vector<std::any>& args) override;

private:
    bool initialized_{false};
    bool parallel_execution_enabled_{false};
    
    // Global Lua state for shared bindings
    lua_State* global_lua_state_{nullptr};
    std::unique_ptr<LuaECSBinder> ecs_binder_;
    
    // ECS integration
    ecs::Registry* bound_registry_{nullptr};
    
    // Lua-specific memory tracking
    std::unordered_map<std::string, usize> script_memory_usage_;
    std::unordered_map<std::string, usize> script_memory_limits_;
    
    // Performance monitoring
    mutable std::mutex performance_mutex_;
    std::unordered_map<std::string, std::vector<f64>> function_call_times_;
    
    // Internal utility methods
    LuaScriptContext* get_lua_context(const std::string& name);
    const LuaScriptContext* get_lua_context(const std::string& name) const;
    LuaScriptContext* create_lua_context(const std::string& name);
    
    ScriptError create_lua_error(const std::string& script_name, const std::string& message) const;
    ScriptError handle_lua_error(lua_State* L, const std::string& script_name, const std::string& operation) const;
    
    void setup_lua_error_handling(lua_State* L);
    void setup_memory_limits(lua_State* L, usize limit_bytes);
    void update_memory_statistics(const std::string& script_name);
    
    // Lua memory allocation callback
    static void* lua_memory_allocator(void* ud, void* ptr, size_t osize, size_t nsize);
    
    // Educational content generation
    void generate_basic_tutorial_script() const;
    void generate_ecs_integration_tutorial() const;
    void generate_performance_comparison_script() const;
    
    // Template implementations for function calls
    template<typename T>
    void push_argument_to_lua(lua_State* L, T&& arg);
    
    template<typename T>
    T get_return_value_from_lua(lua_State* L, int index);
    
    // Argument pushing specializations
    void push_arguments_to_lua(lua_State* L) {} // Base case
    
    template<typename First, typename... Rest>
    void push_arguments_to_lua(lua_State* L, First&& first, Rest&&... rest);
};

// Template implementation for LuaECSBinder
template<typename Component>
void LuaECSBinder::bind_component(const std::string& component_name) {
    create_component_metatable(component_name);
    
    // Register component-specific functions
    std::string add_func_name = "add_" + component_name;
    std::string get_func_name = "get_" + component_name;
    std::string has_func_name = "has_" + component_name;
    
    // These would be implemented with proper component type handling
    register_global_function(add_func_name, lua_add_component);
    register_global_function(get_func_name, lua_get_component);
    register_global_function(has_func_name, lua_has_component);
}

// Template implementations for LuaEngine
template<typename... Args>
ScriptResult<void> LuaEngine::call_lua_function(const std::string& script_name, 
                                               const std::string& function_name,
                                               Args&&... args) {
    auto* context = get_lua_context(script_name);
    if (!context || !context->is_valid()) {
        auto error = create_lua_error(script_name, "Script not loaded or invalid Lua state");
        return ScriptResult<void>::error_result(error);
    }
    
    lua_State* L = context->lua_state;
    
    start_performance_measurement(script_name, "function_call");
    
    // Try cached function first
    bool use_cache = context->function_refs.find(function_name) != context->function_refs.end();
    
    if (!use_cache) {
        lua_getglobal(L, function_name.c_str());
        if (!lua_isfunction(L, -1)) {
            lua_pop(L, 1);
            auto error = create_lua_error(script_name, "Function not found: " + function_name);
            end_performance_measurement(script_name, "function_call");
            return ScriptResult<void>::error_result(error);
        }
    }
    
    // Push arguments
    push_arguments_to_lua(L, std::forward<Args>(args)...);
    
    // Call function
    bool success;
    if (use_cache) {
        success = context->call_cached_function(function_name, sizeof...(args), 0);
    } else {
        success = (lua_pcall(L, sizeof...(args), 0, 0) == LUA_OK);
        // Cache the function for future calls
        context->cache_function_ref(function_name);
    }
    
    end_performance_measurement(script_name, "function_call");
    
    if (!success) {
        auto error = handle_lua_error(L, script_name, "function call: " + function_name);
        return ScriptResult<void>::error_result(error);
    }
    
    // Update memory statistics
    update_memory_statistics(script_name);
    
    return ScriptResult<void>::success_result(context->metrics);
}

template<typename ReturnType, typename... Args>
ScriptResult<ReturnType> LuaEngine::call_lua_function_with_return(const std::string& script_name,
                                                                 const std::string& function_name,
                                                                 Args&&... args) {
    auto* context = get_lua_context(script_name);
    if (!context || !context->is_valid()) {
        auto error = create_lua_error(script_name, "Script not loaded or invalid Lua state");
        return ScriptResult<ReturnType>::error_result(error);
    }
    
    lua_State* L = context->lua_state;
    
    start_performance_measurement(script_name, "function_call_with_return");
    
    // Get function
    lua_getglobal(L, function_name.c_str());
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);
        auto error = create_lua_error(script_name, "Function not found: " + function_name);
        end_performance_measurement(script_name, "function_call_with_return");
        return ScriptResult<ReturnType>::error_result(error);
    }
    
    // Push arguments
    push_arguments_to_lua(L, std::forward<Args>(args)...);
    
    // Call function
    if (lua_pcall(L, sizeof...(args), 1, 0) != LUA_OK) {
        auto error = handle_lua_error(L, script_name, "function call with return: " + function_name);
        end_performance_measurement(script_name, "function_call_with_return");
        return ScriptResult<ReturnType>::error_result(error);
    }
    
    // Get return value
    ReturnType result = get_return_value_from_lua<ReturnType>(L, -1);
    lua_pop(L, 1); // Remove return value from stack
    
    end_performance_measurement(script_name, "function_call_with_return");
    
    // Update memory statistics
    update_memory_statistics(script_name);
    
    return ScriptResult<ReturnType>::success_result(std::move(result), context->metrics);
}

template<typename First, typename... Rest>
void LuaEngine::push_arguments_to_lua(lua_State* L, First&& first, Rest&&... rest) {
    push_argument_to_lua(L, std::forward<First>(first));
    push_arguments_to_lua(L, std::forward<Rest>(rest)...);
}

} // namespace ecscope::scripting