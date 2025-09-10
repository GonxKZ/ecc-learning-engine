#pragma once

#include "script_engine.hpp"
#include "../ecs/registry.hpp"
#include "../physics/world.hpp"
#include "../rendering/renderer.hpp"
#include "../audio/audio_system.hpp"

#include <lua.hpp>
#include <unordered_map>
#include <functional>
#include <type_traits>
#include <concepts>

namespace ecscope::scripting {

/**
 * @brief Lua-specific script context with advanced state management
 */
class LuaScriptContext : public ScriptContext {
public:
    LuaScriptContext(const std::string& name);
    ~LuaScriptContext();
    
    // Non-copyable, movable
    LuaScriptContext(const LuaScriptContext&) = delete;
    LuaScriptContext& operator=(const LuaScriptContext&) = delete;
    LuaScriptContext(LuaScriptContext&& other) noexcept;
    LuaScriptContext& operator=(LuaScriptContext&& other) noexcept;
    
    bool is_valid() const { return lua_state != nullptr; }
    
    // Function reference caching for performance
    void cache_function_ref(const std::string& function_name);
    bool call_cached_function(const std::string& function_name, int nargs = 0, int nresults = 0);
    void clear_function_cache();
    
    // Coroutine support
    lua_State* create_coroutine();
    bool resume_coroutine(lua_State* coroutine, int nargs = 0);
    void cleanup_coroutine(lua_State* coroutine);
    
    // State serialization for hot-reload
    std::string serialize_state();
    bool restore_state(const std::string& serialized_state);
    
    lua_State* lua_state{nullptr};
    bool owns_state{true};
    
private:
    std::unordered_map<std::string, int> function_refs_;
    std::unordered_map<lua_State*, int> coroutine_refs_;
    usize memory_limit_{0};
    usize memory_used_{0};
    
    friend class LuaEngine;
};

/**
 * @brief Advanced type marshaling between C++ and Lua
 */
class LuaTypeMarshaller {
public:
    // Push C++ values to Lua stack with type safety
    template<typename T>
    static void push(lua_State* L, T&& value);
    
    // Get C++ values from Lua stack with type checking
    template<typename T>
    static T get(lua_State* L, int index);
    
    // Type checking predicates
    template<typename T>
    static bool is_type(lua_State* L, int index);
    
    // Complex type support
    static void push_vector3(lua_State* L, const math::Vec3& vec);
    static math::Vec3 get_vector3(lua_State* L, int index);
    
    static void push_transform(lua_State* L, const Transform& transform);
    static Transform get_transform(lua_State* L, int index);
    
    static void push_entity(lua_State* L, Entity entity);
    static Entity get_entity(lua_State* L, int index);
    
    // Container support
    template<typename Container>
    static void push_container(lua_State* L, const Container& container);
    
    template<typename Container>
    static Container get_container(lua_State* L, int index);
    
private:
    static void setup_metatable(lua_State* L, const std::string& type_name);
    static bool check_metatable(lua_State* L, int index, const std::string& type_name);
};

/**
 * @brief Comprehensive ECS bindings for Lua
 */
class LuaECSBinder {
public:
    explicit LuaECSBinder(lua_State* L, ecs::Registry* registry);
    
    // Core ECS operations
    void bind_registry_operations();
    void bind_entity_operations();
    void bind_component_operations();
    void bind_system_operations();
    void bind_query_operations();
    
    // Component type binding with reflection
    template<typename Component>
    void bind_component_type();
    
    // System binding with automatic scheduling
    template<typename System>
    void bind_system_type();
    
    // Advanced query binding
    void bind_query_builder();
    void bind_archetype_operations();
    
    // Event system integration
    void bind_event_system();
    
private:
    lua_State* lua_state_;
    ecs::Registry* registry_;
    
    // Lua C function implementations
    static int lua_create_entity(lua_State* L);
    static int lua_destroy_entity(lua_State* L);
    static int lua_clone_entity(lua_State* L);
    static int lua_get_entity_archetype(lua_State* L);
    
    static int lua_add_component(lua_State* L);
    static int lua_remove_component(lua_State* L);
    static int lua_get_component(lua_State* L);
    static int lua_has_component(lua_State* L);
    static int lua_list_components(lua_State* L);
    
    static int lua_register_system(lua_State* L);
    static int lua_unregister_system(lua_State* L);
    static int lua_execute_system(lua_State* L);
    static int lua_get_system_dependencies(lua_State* L);
    
    static int lua_create_query(lua_State* L);
    static int lua_execute_query(lua_State* L);
    static int lua_count_entities(lua_State* L);
    static int lua_for_each_entity(lua_State* L);
    
    static int lua_emit_event(lua_State* L);
    static int lua_subscribe_event(lua_State* L);
    static int lua_unsubscribe_event(lua_State* L);
    
    // Utility functions
    void create_component_metatable(const std::string& component_name);
    void register_global_function(const std::string& name, lua_CFunction func);
    
    template<typename T>
    void push_component_to_lua(lua_State* L, const T& component);
    
    template<typename T>
    T get_component_from_lua(lua_State* L, int index);
};

/**
 * @brief Engine system bindings for comprehensive access
 */
class LuaEngineBindings {
public:
    explicit LuaEngineBindings(lua_State* L);
    
    // Rendering system bindings
    void bind_rendering_system(rendering::Renderer* renderer);
    void bind_shader_system();
    void bind_material_system();
    void bind_mesh_system();
    void bind_camera_system();
    void bind_lighting_system();
    
    // Physics system bindings
    void bind_physics_system(physics::World* physics_world);
    void bind_rigidbody_operations();
    void bind_collision_detection();
    void bind_constraint_system();
    
    // Audio system bindings
    void bind_audio_system(audio::AudioSystem* audio_system);
    void bind_sound_operations();
    void bind_music_operations();
    void bind_spatial_audio();
    
    // Asset system bindings
    void bind_asset_system();
    void bind_texture_loading();
    void bind_model_loading();
    void bind_shader_loading();
    void bind_audio_loading();
    
    // GUI system bindings
    void bind_gui_system();
    void bind_widget_operations();
    void bind_layout_system();
    void bind_styling_system();
    
    // Networking bindings
    void bind_networking_system();
    void bind_client_operations();
    void bind_server_operations();
    void bind_packet_handling();
    
private:
    lua_State* lua_state_;
    
    // Rendering function implementations
    static int lua_render_mesh(lua_State* L);
    static int lua_set_camera_transform(lua_State* L);
    static int lua_create_shader(lua_State* L);
    static int lua_bind_texture(lua_State* L);
    static int lua_set_material_properties(lua_State* L);
    
    // Physics function implementations
    static int lua_create_rigidbody(lua_State* L);
    static int lua_apply_force(lua_State* L);
    static int lua_set_velocity(lua_State* L);
    static int lua_raycast(lua_State* L);
    static int lua_create_constraint(lua_State* L);
    
    // Audio function implementations
    static int lua_play_sound(lua_State* L);
    static int lua_stop_sound(lua_State* L);
    static int lua_set_volume(lua_State* L);
    static int lua_set_listener_position(lua_State* L);
    static int lua_create_audio_source(lua_State* L);
};

/**
 * @brief Advanced Lua debugging and profiling support
 */
class LuaDebugger {
public:
    explicit LuaDebugger(lua_State* L);
    ~LuaDebugger();
    
    // Breakpoint management
    void set_breakpoint(const std::string& script_name, int line);
    void remove_breakpoint(const std::string& script_name, int line);
    void clear_all_breakpoints();
    
    // Execution control
    void step_over();
    void step_into();
    void step_out();
    void continue_execution();
    void pause_execution();
    
    // Variable inspection
    std::unordered_map<std::string, std::any> get_local_variables();
    std::unordered_map<std::string, std::any> get_global_variables();
    std::any get_variable_value(const std::string& name);
    void set_variable_value(const std::string& name, const std::any& value);
    
    // Stack trace
    std::vector<std::string> get_stack_trace();
    std::string get_current_function();
    int get_current_line();
    
    // Watch expressions
    void add_watch(const std::string& expression);
    void remove_watch(const std::string& expression);
    std::unordered_map<std::string, std::any> evaluate_watches();
    
    // Performance profiling
    void start_profiling();
    void stop_profiling();
    std::string generate_profile_report();
    
private:
    lua_State* lua_state_;
    bool debugging_enabled_{false};
    bool profiling_enabled_{false};
    
    std::unordered_map<std::string, std::set<int>> breakpoints_;
    std::vector<std::string> watch_expressions_;
    
    // Debug hook callback
    static void debug_hook(lua_State* L, lua_Debug* ar);
    void handle_debug_event(lua_Debug* ar);
    
    // Profile data
    std::unordered_map<std::string, usize> function_call_counts_;
    std::unordered_map<std::string, std::chrono::nanoseconds> function_execution_times_;
    std::chrono::steady_clock::time_point profile_start_time_;
};

/**
 * @brief Interactive REPL for Lua development
 */
class LuaREPL {
public:
    explicit LuaREPL(LuaEngine* engine);
    ~LuaREPL() = default;
    
    // REPL operations
    void start();
    void stop();
    bool is_running() const { return running_; }
    
    // Command execution
    std::string execute_command(const std::string& command);
    void execute_file(const std::string& filepath);
    
    // History management
    void add_to_history(const std::string& command);
    std::vector<std::string> get_history() const;
    void clear_history();
    void save_history(const std::string& filepath);
    void load_history(const std::string& filepath);
    
    // Auto-completion
    std::vector<std::string> get_completions(const std::string& partial_input);
    
    // Help system
    std::string get_help(const std::string& topic = "");
    void register_help_topic(const std::string& topic, const std::string& content);
    
private:
    LuaEngine* engine_;
    std::atomic<bool> running_{false};
    std::thread repl_thread_;
    
    std::vector<std::string> command_history_;
    std::unordered_map<std::string, std::string> help_topics_;
    
    void repl_loop();
    std::string format_output(const std::string& result);
    std::string syntax_highlight(const std::string& code);
};

/**
 * @brief Professional Lua scripting engine with advanced features
 * 
 * Features:
 * - Lua 5.4+ with full API bindings
 * - Comprehensive ECS integration
 * - Engine system bindings (rendering, physics, audio, etc.)
 * - Hot-reload with state preservation  
 * - Interactive REPL and debugging
 * - Performance profiling and optimization
 * - Coroutine support for async programming
 * - Memory management and sandboxing
 * - Multi-threading support
 * - Educational examples and documentation
 */
class LuaEngine : public ScriptEngine {
public:
    LuaEngine();
    ~LuaEngine() override;
    
    // ScriptEngine interface
    bool initialize() override;
    void shutdown() override;
    bool is_initialized() const override { return initialized_; }
    ScriptLanguageInfo get_language_info() const override;
    
    // Script management
    ScriptResult<void> load_script(const std::string& name, const std::string& source) override;
    ScriptResult<void> load_script_file(const std::string& name, const std::string& filepath) override;
    ScriptResult<void> reload_script(const std::string& name) override;
    
    // Script compilation
    ScriptResult<void> compile_script(const std::string& name) override;
    ScriptResult<std::vector<u8>> compile_to_bytecode(const std::string& name);
    ScriptResult<void> load_bytecode(const std::string& name, const std::vector<u8>& bytecode);
    
    // Script execution
    ScriptResult<void> execute_script(const std::string& name) override;
    ScriptResult<void> execute_string(const std::string& code, const std::string& context_name = "inline") override;
    
    // Advanced function calls with coroutine support
    template<typename... Args>
    ScriptResult<void> call_lua_function(const std::string& script_name, 
                                        const std::string& function_name,
                                        Args&&... args);
    
    template<typename ReturnType, typename... Args>
    ScriptResult<ReturnType> call_lua_function_with_return(const std::string& script_name,
                                                          const std::string& function_name,
                                                          Args&&... args);
    
    // Coroutine support
    ScriptResult<lua_State*> create_coroutine(const std::string& script_name, const std::string& function_name);
    ScriptResult<void> resume_coroutine(const std::string& script_name, lua_State* coroutine);
    void cleanup_coroutine(const std::string& script_name, lua_State* coroutine);
    
    // Variable management
    ScriptResult<void> set_global_variable(const std::string& script_name, 
                                          const std::string& var_name,
                                          const std::any& value) override;
    ScriptResult<std::any> get_global_variable(const std::string& script_name,
                                              const std::string& var_name) override;
    
    // Memory management
    usize get_memory_usage(const std::string& script_name) const override;
    void collect_garbage() override;
    void set_memory_limit(const std::string& script_name, usize limit_bytes) override;
    void enable_incremental_gc(bool enable);
    void set_gc_parameters(int pause, int stepmul, int stepsize);
    
    // Engine system integration
    void bind_ecs_registry(ecs::Registry* registry);
    void bind_physics_world(physics::World* world);
    void bind_renderer(rendering::Renderer* renderer);
    void bind_audio_system(audio::AudioSystem* audio);
    
    // Development features
    LuaREPL* get_repl();
    LuaDebugger* get_debugger(const std::string& script_name);
    void create_educational_examples();
    void generate_api_documentation();
    
    // Hot-reload with state preservation
    void enable_state_preservation(bool enable) { state_preservation_enabled_ = enable; }
    bool is_state_preservation_enabled() const { return state_preservation_enabled_; }
    
    // Security and sandboxing
    void enable_sandbox_mode(bool enable);
    void set_allowed_modules(const std::string& script_name, const std::vector<std::string>& modules);
    void set_io_restrictions(const std::string& script_name, bool allow_io);
    
    // Multi-threading support
    void enable_multithreading(bool enable);
    ScriptResult<void> execute_script_on_thread(const std::string& name);
    
    // Performance optimization
    void enable_jit_compilation(bool enable);
    void enable_function_caching(bool enable);
    void warm_up_scripts();
    
    // Engine information
    std::string get_version_info() const override;
    std::string explain_performance_characteristics() const override;
    std::vector<std::string> get_optimization_suggestions(const std::string& script_name) const override;
    
protected:
    // Internal function call implementations
    ScriptResult<void> call_function_impl_void(const std::string& script_name,
                                              const std::string& function_name,
                                              const std::vector<std::any>& args) override;
    
    template<typename ReturnType>
    ScriptResult<ReturnType> call_function_impl_typed(const std::string& script_name,
                                                     const std::string& function_name,
                                                     const std::vector<std::any>& args) override;

private:
    bool initialized_{false};
    bool sandbox_mode_enabled_{false};
    bool state_preservation_enabled_{true};
    bool multithreading_enabled_{false};
    bool jit_enabled_{false};
    bool function_caching_enabled_{true};
    
    // Global Lua state for shared bindings
    lua_State* global_lua_state_{nullptr};
    
    // Engine system bindings
    std::unique_ptr<LuaECSBinder> ecs_binder_;
    std::unique_ptr<LuaEngineBindings> engine_bindings_;
    
    // Development tools
    std::unique_ptr<LuaREPL> repl_;
    std::unordered_map<std::string, std::unique_ptr<LuaDebugger>> debuggers_;
    
    // Memory management
    std::unordered_map<std::string, usize> script_memory_usage_;
    std::unordered_map<std::string, usize> script_memory_limits_;
    
    // Threading support
    std::thread_pool script_thread_pool_;
    
    // Internal utility methods
    LuaScriptContext* get_lua_context(const std::string& name);
    const LuaScriptContext* get_lua_context(const std::string& name) const;
    LuaScriptContext* create_lua_context(const std::string& name);
    
    ScriptError create_lua_error(const std::string& script_name, const std::string& message) const;
    ScriptError handle_lua_error(lua_State* L, const std::string& script_name, const std::string& operation) const;
    
    void setup_lua_environment(lua_State* L);
    void setup_sandbox_restrictions(lua_State* L, const std::string& script_name);
    void setup_memory_limits(lua_State* L, usize limit_bytes);
    void update_memory_statistics(const std::string& script_name);
    
    // Lua memory allocation callback
    static void* lua_memory_allocator(void* ud, void* ptr, size_t osize, size_t nsize);
    
    // Educational content generation
    void generate_basic_tutorial();
    void generate_ecs_integration_tutorial();
    void generate_engine_bindings_tutorial();
    void generate_advanced_features_tutorial();
    
    // Template implementations for argument marshaling
    template<typename T>
    void push_argument_to_lua(lua_State* L, T&& arg);
    
    template<typename T>
    T get_return_value_from_lua(lua_State* L, int index);
    
    void push_arguments_to_lua(lua_State* L) {} // Base case
    
    template<typename First, typename... Rest>
    void push_arguments_to_lua(lua_State* L, First&& first, Rest&&... rest);
};

// Register Lua engine with the factory
void register_lua_engine();

} // namespace ecscope::scripting