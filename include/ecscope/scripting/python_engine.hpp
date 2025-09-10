#pragma once

#include "script_engine.hpp"
#include "../ecs/registry.hpp"
#include "../physics/world.hpp"
#include "../rendering/renderer.hpp"
#include "../audio/audio_system.hpp"

#include <Python.h>
#include <unordered_map>
#include <functional>
#include <type_traits>
#include <concepts>

namespace ecscope::scripting {

/**
 * @brief RAII wrapper for Python objects
 */
class PyObjectWrapper {
public:
    PyObjectWrapper() : obj_(nullptr) {}
    explicit PyObjectWrapper(PyObject* obj) : obj_(obj) {}
    
    ~PyObjectWrapper() {
        if (obj_) {
            Py_DECREF(obj_);
        }
    }
    
    // Non-copyable, movable
    PyObjectWrapper(const PyObjectWrapper&) = delete;
    PyObjectWrapper& operator=(const PyObjectWrapper&) = delete;
    
    PyObjectWrapper(PyObjectWrapper&& other) noexcept : obj_(other.obj_) {
        other.obj_ = nullptr;
    }
    
    PyObjectWrapper& operator=(PyObjectWrapper&& other) noexcept {
        if (this != &other) {
            if (obj_) {
                Py_DECREF(obj_);
            }
            obj_ = other.obj_;
            other.obj_ = nullptr;
        }
        return *this;
    }
    
    PyObject* get() const { return obj_; }
    PyObject* release() {
        PyObject* temp = obj_;
        obj_ = nullptr;
        return temp;
    }
    
    void reset(PyObject* obj = nullptr) {
        if (obj_) {
            Py_DECREF(obj_);
        }
        obj_ = obj;
    }
    
    bool is_valid() const { return obj_ != nullptr; }
    operator bool() const { return is_valid(); }
    
    PyObject* operator->() const { return obj_; }
    PyObject& operator*() const { return *obj_; }

private:
    PyObject* obj_;
};

/**
 * @brief Python-specific script context with advanced interpreter management
 */
class PythonScriptContext : public ScriptContext {
public:
    PythonScriptContext(const std::string& name);
    ~PythonScriptContext();
    
    // Non-copyable, movable
    PythonScriptContext(const PythonScriptContext&) = delete;
    PythonScriptContext& operator=(const PythonScriptContext&) = delete;
    PythonScriptContext(PythonScriptContext&& other) noexcept;
    PythonScriptContext& operator=(PythonScriptContext&& other) noexcept;
    
    bool is_valid() const;
    
    // Module and namespace management
    PyObject* get_main_module() const { return main_module_.get(); }
    PyObject* get_main_dict() const { return main_dict_.get(); }
    PyObject* get_builtins_dict() const { return builtins_dict_.get(); }
    
    // Function caching for performance
    void cache_function(const std::string& function_name, PyObject* func);
    PyObject* get_cached_function(const std::string& function_name);
    void clear_function_cache();
    
    // Generator and coroutine support
    PyObject* create_generator(const std::string& generator_func);
    PyObject* next_from_generator(PyObject* generator);
    void cleanup_generator(PyObject* generator);
    
    // Async/await support
    PyObject* create_coroutine(const std::string& coro_func);
    PyObject* await_coroutine(PyObject* coroutine);
    void cleanup_coroutine(PyObject* coroutine);
    
    // State serialization for hot-reload
    std::string serialize_globals();
    bool restore_globals(const std::string& serialized_globals);
    
    // Thread state management
    PyThreadState* get_thread_state() const { return thread_state_; }
    void acquire_gil();
    void release_gil();
    
private:
    PyThreadState* thread_state_{nullptr};
    bool owns_thread_state_{true};
    
    PyObjectWrapper main_module_;
    PyObjectWrapper main_dict_;
    PyObjectWrapper builtins_dict_;
    
    std::unordered_map<std::string, PyObjectWrapper> cached_functions_;
    std::unordered_map<std::string, PyObjectWrapper> cached_modules_;
    
    usize memory_limit_{0};
    usize memory_used_{0};
    
    friend class PythonEngine;
};

/**
 * @brief Advanced type marshaling between C++ and Python
 */
class PythonTypeMarshaller {
public:
    // Push C++ values to Python with type safety
    template<typename T>
    static PyObject* to_python(T&& value);
    
    // Get C++ values from Python with type checking
    template<typename T>
    static T from_python(PyObject* obj);
    
    // Type checking predicates
    template<typename T>
    static bool is_type(PyObject* obj);
    
    // Complex type support
    static PyObject* vector3_to_python(const math::Vec3& vec);
    static math::Vec3 vector3_from_python(PyObject* obj);
    
    static PyObject* transform_to_python(const Transform& transform);
    static Transform transform_from_python(PyObject* obj);
    
    static PyObject* entity_to_python(Entity entity);
    static Entity entity_from_python(PyObject* obj);
    
    // Container support
    template<typename Container>
    static PyObject* container_to_python(const Container& container);
    
    template<typename Container>
    static Container container_from_python(PyObject* obj);
    
    // NumPy integration for arrays
    static PyObject* array_to_numpy(const void* data, const std::vector<npy_intp>& dims, int type_num);
    static void* numpy_to_array(PyObject* array, std::vector<npy_intp>& dims, int& type_num);
    
private:
    static bool ensure_numpy_available();
    static void register_custom_types();
};

/**
 * @brief Comprehensive ECS bindings for Python
 */
class PythonECSBinder {
public:
    explicit PythonECSBinder(ecs::Registry* registry);
    ~PythonECSBinder();
    
    // Initialize Python ECS module
    bool initialize();
    void shutdown();
    
    // Core ECS operations
    void bind_registry_operations();
    void bind_entity_operations();
    void bind_component_operations();
    void bind_system_operations();
    void bind_query_operations();
    
    // Component type binding with automatic Python class generation
    template<typename Component>
    void bind_component_type(const std::string& name);
    
    // System binding with Python class integration
    template<typename System>
    void bind_system_type(const std::string& name);
    
    // Advanced query binding with Python iterators
    void bind_query_builder();
    void bind_archetype_operations();
    
    // Event system with Python callbacks
    void bind_event_system();
    
    // Get the ECS module
    PyObject* get_ecs_module() const { return ecs_module_.get(); }
    
private:
    ecs::Registry* registry_;
    PyObjectWrapper ecs_module_;
    
    // Python method implementations
    static PyObject* py_create_entity(PyObject* self, PyObject* args);
    static PyObject* py_destroy_entity(PyObject* self, PyObject* args);
    static PyObject* py_clone_entity(PyObject* self, PyObject* args);
    static PyObject* py_get_entity_archetype(PyObject* self, PyObject* args);
    
    static PyObject* py_add_component(PyObject* self, PyObject* args, PyObject* kwargs);
    static PyObject* py_remove_component(PyObject* self, PyObject* args);
    static PyObject* py_get_component(PyObject* self, PyObject* args);
    static PyObject* py_has_component(PyObject* self, PyObject* args);
    static PyObject* py_list_components(PyObject* self, PyObject* args);
    
    static PyObject* py_register_system(PyObject* self, PyObject* args, PyObject* kwargs);
    static PyObject* py_unregister_system(PyObject* self, PyObject* args);
    static PyObject* py_execute_system(PyObject* self, PyObject* args);
    static PyObject* py_get_system_dependencies(PyObject* self, PyObject* args);
    
    static PyObject* py_create_query(PyObject* self, PyObject* args, PyObject* kwargs);
    static PyObject* py_execute_query(PyObject* self, PyObject* args);
    static PyObject* py_count_entities(PyObject* self, PyObject* args);
    static PyObject* py_for_each_entity(PyObject* self, PyObject* args);
    
    static PyObject* py_emit_event(PyObject* self, PyObject* args, PyObject* kwargs);
    static PyObject* py_subscribe_event(PyObject* self, PyObject* args);
    static PyObject* py_unsubscribe_event(PyObject* self, PyObject* args);
    
    // Utility functions
    void create_component_class(const std::string& component_name);
    void register_module_function(const std::string& name, PyCFunction func, const std::string& doc = "");
    
    template<typename T>
    void push_component_to_python(const T& component);
    
    template<typename T>
    T get_component_from_python(PyObject* obj);
    
    // Module method table
    static PyMethodDef ecs_methods_[];
};

/**
 * @brief Engine system bindings for comprehensive Python access
 */
class PythonEngineBindings {
public:
    explicit PythonEngineBindings();
    ~PythonEngineBindings();
    
    bool initialize();
    void shutdown();
    
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
    
    // Networking bindings with asyncio integration
    void bind_networking_system();
    void bind_client_operations();
    void bind_server_operations();
    void bind_packet_handling();
    
    PyObject* get_engine_module() const { return engine_module_.get(); }
    
private:
    PyObjectWrapper engine_module_;
    
    // Python method implementations for rendering
    static PyObject* py_render_mesh(PyObject* self, PyObject* args, PyObject* kwargs);
    static PyObject* py_set_camera_transform(PyObject* self, PyObject* args);
    static PyObject* py_create_shader(PyObject* self, PyObject* args, PyObject* kwargs);
    static PyObject* py_bind_texture(PyObject* self, PyObject* args);
    static PyObject* py_set_material_properties(PyObject* self, PyObject* args, PyObject* kwargs);
    
    // Physics method implementations
    static PyObject* py_create_rigidbody(PyObject* self, PyObject* args, PyObject* kwargs);
    static PyObject* py_apply_force(PyObject* self, PyObject* args);
    static PyObject* py_set_velocity(PyObject* self, PyObject* args);
    static PyObject* py_raycast(PyObject* self, PyObject* args, PyObject* kwargs);
    static PyObject* py_create_constraint(PyObject* self, PyObject* args, PyObject* kwargs);
    
    // Audio method implementations
    static PyObject* py_play_sound(PyObject* self, PyObject* args, PyObject* kwargs);
    static PyObject* py_stop_sound(PyObject* self, PyObject* args);
    static PyObject* py_set_volume(PyObject* self, PyObject* args);
    static PyObject* py_set_listener_position(PyObject* self, PyObject* args);
    static PyObject* py_create_audio_source(PyObject* self, PyObject* args, PyObject* kwargs);
    
    static PyMethodDef engine_methods_[];
};

/**
 * @brief Advanced Python debugging and profiling support
 */
class PythonDebugger {
public:
    explicit PythonDebugger(PythonScriptContext* context);
    ~PythonDebugger();
    
    // Breakpoint management
    void set_breakpoint(const std::string& filename, int line);
    void remove_breakpoint(const std::string& filename, int line);
    void clear_all_breakpoints();
    
    // Execution control
    void step_over();
    void step_into();
    void step_out();
    void continue_execution();
    void pause_execution();
    
    // Variable inspection using Python's inspect module
    std::unordered_map<std::string, std::any> get_local_variables();
    std::unordered_map<std::string, std::any> get_global_variables();
    std::any get_variable_value(const std::string& name);
    void set_variable_value(const std::string& name, const std::any& value);
    
    // Stack trace using traceback module
    std::vector<std::string> get_stack_trace();
    std::string get_current_function();
    int get_current_line();
    std::string get_current_filename();
    
    // Watch expressions
    void add_watch(const std::string& expression);
    void remove_watch(const std::string& expression);
    std::unordered_map<std::string, std::any> evaluate_watches();
    
    // Performance profiling using cProfile
    void start_profiling();
    void stop_profiling();
    std::string generate_profile_report();
    
    // Memory profiling using tracemalloc
    void start_memory_tracing();
    void stop_memory_tracing();
    std::string generate_memory_report();
    
private:
    PythonScriptContext* context_;
    bool debugging_enabled_{false};
    bool profiling_enabled_{false};
    bool memory_tracing_enabled_{false};
    
    std::unordered_map<std::string, std::set<int>> breakpoints_;
    std::vector<std::string> watch_expressions_;
    
    PyObjectWrapper trace_function_;
    PyObjectWrapper profile_object_;
    
    // Trace callback
    static int trace_callback(PyObject* obj, PyFrameObject* frame, int what, PyObject* arg);
    int handle_trace_event(PyFrameObject* frame, int what, PyObject* arg);
    
    void setup_debugging();
    void cleanup_debugging();
};

/**
 * @brief Interactive Python REPL with IPython-like features
 */
class PythonREPL {
public:
    explicit PythonREPL(PythonEngine* engine);
    ~PythonREPL();
    
    // REPL operations
    void start();
    void stop();
    bool is_running() const { return running_; }
    
    // Command execution
    std::string execute_command(const std::string& command);
    void execute_file(const std::string& filepath);
    
    // Multi-line input support
    bool is_complete_statement(const std::string& code);
    void set_continuation_prompt(const std::string& prompt);
    
    // History management with persistence
    void add_to_history(const std::string& command);
    std::vector<std::string> get_history() const;
    void clear_history();
    void save_history(const std::string& filepath);
    void load_history(const std::string& filepath);
    
    // Auto-completion using Python's rlcompleter
    std::vector<std::string> get_completions(const std::string& partial_input);
    
    // Magic commands (IPython-style)
    void register_magic_command(const std::string& command, std::function<std::string(const std::string&)> handler);
    std::string execute_magic_command(const std::string& command, const std::string& args);
    
    // Help system using Python's help()
    std::string get_help(const std::string& topic = "");
    void register_help_topic(const std::string& topic, const std::string& content);
    
    // Code introspection
    std::string inspect_object(const std::string& object_name);
    std::string get_source_code(const std::string& object_name);
    std::string get_docstring(const std::string& object_name);
    
private:
    PythonEngine* engine_;
    std::atomic<bool> running_{false};
    std::thread repl_thread_;
    
    std::vector<std::string> command_history_;
    std::unordered_map<std::string, std::string> help_topics_;
    std::unordered_map<std::string, std::function<std::string(const std::string&)>> magic_commands_;
    
    PyObjectWrapper completer_;
    PyObjectWrapper code_module_;
    
    std::string primary_prompt_{">>> "};
    std::string continuation_prompt_{"... "};
    
    void repl_loop();
    std::string format_output(PyObject* result);
    std::string syntax_highlight(const std::string& code);
    void setup_builtin_magic_commands();
    
    // Built-in magic commands
    std::string magic_help(const std::string& args);
    std::string magic_history(const std::string& args);
    std::string magic_reset(const std::string& args);
    std::string magic_time(const std::string& args);
    std::string magic_memory(const std::string& args);
};

/**
 * @brief Professional Python scripting engine with advanced features
 * 
 * Features:
 * - Python 3.11+ with full C API bindings
 * - Comprehensive ECS integration with Pythonic APIs
 * - Engine system bindings with async support
 * - Hot-reload with state preservation
 * - Interactive REPL with IPython-like features
 * - Advanced debugging with pdb integration
 * - Performance profiling and memory tracing
 * - NumPy and SciPy integration for scientific computing
 * - Async/await and asyncio support
 * - Multi-threading with GIL management
 * - Package management and virtual environment support
 * - Educational examples and comprehensive documentation
 */
class PythonEngine : public ScriptEngine {
public:
    PythonEngine();
    ~PythonEngine() override;
    
    // ScriptEngine interface
    bool initialize() override;
    void shutdown() override;
    bool is_initialized() const override { return initialized_; }
    ScriptLanguageInfo get_language_info() const override;
    
    // Script management
    ScriptResult<void> load_script(const std::string& name, const std::string& source) override;
    ScriptResult<void> load_script_file(const std::string& name, const std::string& filepath) override;
    ScriptResult<void> reload_script(const std::string& name) override;
    
    // Script compilation to bytecode
    ScriptResult<void> compile_script(const std::string& name) override;
    ScriptResult<std::vector<u8>> compile_to_bytecode(const std::string& name);
    ScriptResult<void> load_bytecode(const std::string& name, const std::vector<u8>& bytecode);
    
    // Script execution with async support
    ScriptResult<void> execute_script(const std::string& name) override;
    ScriptResult<void> execute_string(const std::string& code, const std::string& context_name = "inline") override;
    ScriptResult<void> execute_async(const std::string& name);
    
    // Advanced function calls with keyword arguments
    template<typename... Args>
    ScriptResult<void> call_python_function(const std::string& script_name, 
                                           const std::string& function_name,
                                           Args&&... args);
    
    template<typename ReturnType, typename... Args>
    ScriptResult<ReturnType> call_python_function_with_return(const std::string& script_name,
                                                             const std::string& function_name,
                                                             Args&&... args);
    
    // Keyword argument support
    ScriptResult<void> call_function_with_kwargs(const std::string& script_name,
                                                 const std::string& function_name,
                                                 const std::vector<std::any>& args,
                                                 const std::unordered_map<std::string, std::any>& kwargs);
    
    // Generator and coroutine support
    ScriptResult<PyObject*> create_generator(const std::string& script_name, const std::string& function_name);
    ScriptResult<std::any> next_from_generator(const std::string& script_name, PyObject* generator);
    
    ScriptResult<PyObject*> create_coroutine(const std::string& script_name, const std::string& function_name);
    ScriptResult<std::any> await_coroutine(const std::string& script_name, PyObject* coroutine);
    
    // Variable management
    ScriptResult<void> set_global_variable(const std::string& script_name, 
                                          const std::string& var_name,
                                          const std::any& value) override;
    ScriptResult<std::any> get_global_variable(const std::string& script_name,
                                              const std::string& var_name) override;
    
    // Module management
    ScriptResult<void> import_module(const std::string& script_name, const std::string& module_name);
    ScriptResult<void> install_package(const std::string& package_name);
    std::vector<std::string> list_installed_packages();
    
    // Memory management with Python's garbage collector
    usize get_memory_usage(const std::string& script_name) const override;
    void collect_garbage() override;
    void set_memory_limit(const std::string& script_name, usize limit_bytes) override;
    void enable_memory_tracing(bool enable);
    std::string generate_memory_report();
    
    // Engine system integration
    void bind_ecs_registry(ecs::Registry* registry);
    void bind_physics_world(physics::World* world);
    void bind_renderer(rendering::Renderer* renderer);
    void bind_audio_system(audio::AudioSystem* audio);
    
    // Development features
    PythonREPL* get_repl();
    PythonDebugger* get_debugger(const std::string& script_name);
    void create_educational_examples();
    void generate_api_documentation();
    
    // Hot-reload with state preservation
    void enable_state_preservation(bool enable) { state_preservation_enabled_ = enable; }
    bool is_state_preservation_enabled() const { return state_preservation_enabled_; }
    
    // Virtual environment support
    void create_virtual_environment(const std::string& env_name);
    void activate_virtual_environment(const std::string& env_name);
    void deactivate_virtual_environment();
    std::vector<std::string> list_virtual_environments();
    
    // Multi-threading with GIL management
    void enable_multithreading(bool enable);
    void acquire_gil();
    void release_gil();
    
    // Performance optimization
    void enable_bytecode_caching(bool enable);
    void enable_function_caching(bool enable);
    void warm_up_scripts();
    
    // Jupyter notebook integration
    void enable_jupyter_support(bool enable);
    std::string execute_jupyter_cell(const std::string& cell_content);
    
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
    bool state_preservation_enabled_{true};
    bool multithreading_enabled_{false};
    bool bytecode_caching_enabled_{true};
    bool function_caching_enabled_{true};
    bool memory_tracing_enabled_{false};
    bool jupyter_support_enabled_{false};
    
    // Python interpreter state
    PyThreadState* main_thread_state_{nullptr};
    
    // Engine system bindings
    std::unique_ptr<PythonECSBinder> ecs_binder_;
    std::unique_ptr<PythonEngineBindings> engine_bindings_;
    
    // Development tools
    std::unique_ptr<PythonREPL> repl_;
    std::unordered_map<std::string, std::unique_ptr<PythonDebugger>> debuggers_;
    
    // Memory management
    std::unordered_map<std::string, usize> script_memory_usage_;
    std::unordered_map<std::string, usize> script_memory_limits_;
    
    // Virtual environment management
    std::string current_venv_;
    std::unordered_map<std::string, std::string> virtual_environments_;
    
    // Threading support
    std::thread_pool script_thread_pool_;
    
    // Internal utility methods
    PythonScriptContext* get_python_context(const std::string& name);
    const PythonScriptContext* get_python_context(const std::string& name) const;
    PythonScriptContext* create_python_context(const std::string& name);
    
    ScriptError create_python_error(const std::string& script_name, const std::string& message) const;
    ScriptError handle_python_error(const std::string& script_name, const std::string& operation) const;
    
    void setup_python_environment();
    void setup_import_paths();
    void setup_memory_limits(const std::string& script_name, usize limit_bytes);
    void update_memory_statistics(const std::string& script_name);
    
    // Educational content generation
    void generate_basic_tutorial();
    void generate_ecs_integration_tutorial();
    void generate_engine_bindings_tutorial();
    void generate_async_programming_tutorial();
    void generate_scientific_computing_tutorial();
    
    // Template implementations for argument marshaling
    template<typename T>
    PyObject* convert_to_python(T&& arg);
    
    template<typename T>
    T convert_from_python(PyObject* obj);
    
    PyObject* build_args_tuple() { return PyTuple_New(0); } // Base case
    
    template<typename First, typename... Rest>
    PyObject* build_args_tuple(First&& first, Rest&&... rest);
};

// Register Python engine with the factory
void register_python_engine();

} // namespace ecscope::scripting