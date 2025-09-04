#pragma once

#include "script_engine.hpp"
#include <Python.h>
#include <unordered_map>
#include <memory>

namespace ecscope::scripting {

// Forward declarations
namespace ecs { class Registry; }

/**
 * @brief Python-specific script context with interpreter state
 */
struct PythonScriptContext : ScriptContext {
    PyObject* module{nullptr};          // Python module object
    PyObject* globals{nullptr};         // Global namespace dict
    PyObject* locals{nullptr};          // Local namespace dict
    std::unordered_map<std::string, PyObject*> cached_functions;
    
    PythonScriptContext(const std::string& name) 
        : ScriptContext(name, "Python") {
        // Create new dictionaries for this context
        globals = PyDict_New();
        locals = PyDict_New();
        
        // Import builtins into globals
        if (globals) {
            PyDict_SetItemString(globals, "__builtins__", PyEval_GetBuiltins());
        }
    }
    
    ~PythonScriptContext() {
        // Clean up cached functions
        for (auto& [name, func] : cached_functions) {
            Py_XDECREF(func);
        }
        cached_functions.clear();
        
        // Clean up Python objects
        Py_XDECREF(locals);
        Py_XDECREF(globals);
        Py_XDECREF(module);
    }
    
    // Non-copyable, movable
    PythonScriptContext(const PythonScriptContext&) = delete;
    PythonScriptContext& operator=(const PythonScriptContext&) = delete;
    PythonScriptContext(PythonScriptContext&& other) noexcept
        : ScriptContext(std::move(other))
        , module(other.module)
        , globals(other.globals)
        , locals(other.locals)
        , cached_functions(std::move(other.cached_functions)) {
        other.module = nullptr;
        other.globals = nullptr;
        other.locals = nullptr;
    }
    
    bool is_valid() const {
        return globals != nullptr && locals != nullptr;
    }
    
    PyObject* get_cached_function(const std::string& name) {
        auto it = cached_functions.find(name);
        if (it != cached_functions.end()) {
            return it->second; // Already has a reference
        }
        
        // Try to find function in globals
        PyObject* func = PyDict_GetItemString(globals, name.c_str());
        if (func && PyCallable_Check(func)) {
            Py_INCREF(func); // Add reference for caching
            cached_functions[name] = func;
            return func;
        }
        
        return nullptr;
    }
    
    void invalidate_function_cache() {
        for (auto& [name, func] : cached_functions) {
            Py_XDECREF(func);
        }
        cached_functions.clear();
    }
};

/**
 * @brief Type-safe Python binding utilities
 */
class PythonTypeHelper {
public:
    // Convert C++ values to Python objects
    static PyObject* to_python(bool value) { return PyBool_FromLong(value ? 1 : 0); }
    static PyObject* to_python(int value) { return PyLong_FromLong(value); }
    static PyObject* to_python(long value) { return PyLong_FromLong(value); }
    static PyObject* to_python(float value) { return PyFloat_FromDouble(value); }
    static PyObject* to_python(double value) { return PyFloat_FromDouble(value); }
    static PyObject* to_python(const char* value) { return PyUnicode_FromString(value); }
    static PyObject* to_python(const std::string& value) { return PyUnicode_FromString(value.c_str()); }
    
    template<typename T>
    static PyObject* to_python(T* ptr) {
        if (ptr) {
            return PyLong_FromVoidPtr(static_cast<void*>(ptr));
        } else {
            Py_RETURN_NONE;
        }
    }
    
    // Convert Python objects to C++ values
    static bool from_python_bool(PyObject* obj) {
        return PyObject_IsTrue(obj) == 1;
    }
    
    static int from_python_int(PyObject* obj) {
        if (PyLong_Check(obj)) {
            return static_cast<int>(PyLong_AsLong(obj));
        }
        return 0;
    }
    
    static float from_python_float(PyObject* obj) {
        if (PyFloat_Check(obj)) {
            return static_cast<float>(PyFloat_AsDouble(obj));
        } else if (PyLong_Check(obj)) {
            return static_cast<float>(PyLong_AsDouble(obj));
        }
        return 0.0f;
    }
    
    static double from_python_double(PyObject* obj) {
        if (PyFloat_Check(obj)) {
            return PyFloat_AsDouble(obj);
        } else if (PyLong_Check(obj)) {
            return PyLong_AsDouble(obj);
        }
        return 0.0;
    }
    
    static std::string from_python_string(PyObject* obj) {
        if (PyUnicode_Check(obj)) {
            Py_ssize_t size;
            const char* data = PyUnicode_AsUTF8AndSize(obj, &size);
            if (data) {
                return std::string(data, size);
            }
        }
        return "";
    }
    
    template<typename T>
    static T* from_python_ptr(PyObject* obj) {
        if (PyLong_Check(obj)) {
            return static_cast<T*>(PyLong_AsVoidPtr(obj));
        }
        return nullptr;
    }
    
    // Type checking
    static bool is_bool(PyObject* obj) { return PyBool_Check(obj); }
    static bool is_int(PyObject* obj) { return PyLong_Check(obj); }
    static bool is_float(PyObject* obj) { return PyFloat_Check(obj); }
    static bool is_string(PyObject* obj) { return PyUnicode_Check(obj); }
    static bool is_list(PyObject* obj) { return PyList_Check(obj); }
    static bool is_dict(PyObject* obj) { return PyDict_Check(obj); }
    static bool is_callable(PyObject* obj) { return PyCallable_Check(obj); }
    
    // Error handling
    static std::string get_python_error() {
        if (!PyErr_Occurred()) {
            return "No Python error";
        }
        
        PyObject* type, *value, *traceback;
        PyErr_Fetch(&type, &value, &traceback);
        
        std::string error_msg = "Python error";
        
        if (value) {
            PyObject* str = PyObject_Str(value);
            if (str) {
                error_msg = from_python_string(str);
                Py_DECREF(str);
            }
        }
        
        // Clean up
        Py_XDECREF(type);
        Py_XDECREF(value);
        Py_XDECREF(traceback);
        
        return error_msg;
    }
};

/**
 * @brief ECS integration for Python scripts
 */
class PythonECSBinder {
public:
    explicit PythonECSBinder();
    ~PythonECSBinder();
    
    // Setup ECS integration
    bool initialize();
    void shutdown();
    
    // Bind ECS registry
    void bind_ecs_registry(ecs::Registry* registry);
    
    // Create ECS module for Python
    void create_ecs_module();
    
    // Educational examples
    void create_educational_examples();
    
    // Component binding
    template<typename Component>
    void bind_component_type(const std::string& name);
    
    // Get the ECS module object
    PyObject* get_ecs_module() const { return ecs_module_; }

private:
    PyObject* ecs_module_{nullptr};
    ecs::Registry* bound_registry_{nullptr};
    
    // Python C API functions for ECS
    static PyObject* py_create_entity(PyObject* self, PyObject* args);
    static PyObject* py_destroy_entity(PyObject* self, PyObject* args);
    static PyObject* py_add_component(PyObject* self, PyObject* args, PyObject* kwargs);
    static PyObject* py_get_component(PyObject* self, PyObject* args);
    static PyObject* py_has_component(PyObject* self, PyObject* args);
    static PyObject* py_get_entities_with(PyObject* self, PyObject* args);
    
    // Module method definitions
    static PyMethodDef ecs_methods_[];
    static PyModuleDef ecs_module_def_;
    
    // Utility functions
    void register_ecs_constants();
    void register_component_types();
    
    // Error handling
    void set_python_error(const std::string& message);
};

/**
 * @brief Comprehensive Python scripting engine
 * 
 * Features:
 * - Full Python C API integration
 * - ECS bindings with automatic type conversion
 * - Memory management and garbage collection
 * - Performance monitoring and profiling
 * - Hot-reload support
 * - Educational examples and tutorials
 * - NumPy integration for mathematical operations
 * - Multi-threading support considerations
 */
class PythonEngine : public ScriptEngine {
public:
    PythonEngine();
    ~PythonEngine() override;
    
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
    
    // Python-specific functionality
    void bind_ecs_registry(ecs::Registry* registry);
    void import_module(const std::string& module_name);
    void add_to_sys_path(const std::string& path);
    
    // Global variable management
    void set_global_variable(const std::string& name, PyObject* value);
    PyObject* get_global_variable(const std::string& name);
    
    // Educational features
    void create_tutorial_scripts();
    void demonstrate_numpy_integration();
    void explain_gil_implications() const;
    
    // Advanced Python features
    template<typename... Args>
    ScriptResult<PyObject*> call_python_function(const std::string& script_name,
                                                const std::string& function_name,
                                                Args&&... args);
    
    template<typename ReturnType, typename... Args>
    ScriptResult<ReturnType> call_python_function_typed(const std::string& script_name,
                                                       const std::string& function_name,
                                                       Args&&... args);
    
    // NumPy integration
    bool initialize_numpy();
    bool is_numpy_available() const { return numpy_available_; }
    
    // Threading considerations
    void acquire_gil();
    void release_gil();
    
    // Profiling integration
    void enable_profiling(const std::string& script_name);
    void disable_profiling(const std::string& script_name);
    std::string get_profiling_results(const std::string& script_name) const;

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
    bool numpy_available_{false};
    bool gil_initialized_{false};
    
    // Python interpreter state
    PyThreadState* main_thread_state_{nullptr};
    
    // ECS integration
    std::unique_ptr<PythonECSBinder> ecs_binder_;
    ecs::Registry* bound_registry_{nullptr};
    
    // Memory tracking
    std::unordered_map<std::string, usize> script_memory_limits_;
    
    // Profiling state
    std::unordered_map<std::string, bool> profiling_enabled_;
    
    // Internal utility methods
    PythonScriptContext* get_python_context(const std::string& name);
    const PythonScriptContext* get_python_context(const std::string& name) const;
    PythonScriptContext* create_python_context(const std::string& name);
    
    // Error handling
    ScriptError create_python_error(const std::string& script_name, const std::string& message) const;
    ScriptError handle_python_error(const std::string& script_name, const std::string& operation) const;
    
    // Memory management
    void update_memory_statistics(const std::string& script_name);
    usize estimate_python_memory_usage(PythonScriptContext* context) const;
    
    // Initialization helpers
    bool initialize_python_interpreter();
    void setup_python_paths();
    void setup_signal_handlers();
    
    // Educational content generation
    void generate_basic_python_tutorial() const;
    void generate_ecs_python_tutorial() const;
    void generate_numpy_tutorial() const;
    void generate_performance_tutorial() const;
    
    // Template implementations
    PyObject* create_python_tuple_from_args() { return PyTuple_New(0); }
    
    template<typename First, typename... Rest>
    PyObject* create_python_tuple_from_args(First&& first, Rest&&... rest);
    
    template<typename T>
    PyObject* convert_arg_to_python(T&& arg);
    
    // GIL management RAII helper
    class GILLock {
    public:
        GILLock() : state_(PyGILState_Ensure()) {}
        ~GILLock() { PyGILState_Release(state_); }
        
        GILLock(const GILLock&) = delete;
        GILLock& operator=(const GILLock&) = delete;
        
    private:
        PyGILState_STATE state_;
    };
    
    // Memory usage estimation helpers
    usize estimate_object_size(PyObject* obj) const;
    usize walk_object_tree(PyObject* obj, std::unordered_set<PyObject*>& visited) const;
};

// Template implementations
template<typename... Args>
ScriptResult<PyObject*> PythonEngine::call_python_function(const std::string& script_name,
                                                          const std::string& function_name,
                                                          Args&&... args) {
    GILLock gil_lock; // Ensure GIL is held
    
    auto* context = get_python_context(script_name);
    if (!context || !context->is_valid()) {
        auto error = create_python_error(script_name, "Script not loaded or invalid");
        return ScriptResult<PyObject*>::error_result(error);
    }
    
    start_performance_measurement(script_name, "python_function_call");
    
    // Get function from cache or globals
    PyObject* func = context->get_cached_function(function_name);
    if (!func) {
        auto error = create_python_error(script_name, "Function not found: " + function_name);
        end_performance_measurement(script_name, "python_function_call");
        return ScriptResult<PyObject*>::error_result(error);
    }
    
    // Create argument tuple
    PyObject* args_tuple = create_python_tuple_from_args(std::forward<Args>(args)...);
    if (!args_tuple) {
        auto error = handle_python_error(script_name, "argument conversion");
        end_performance_measurement(script_name, "python_function_call");
        return ScriptResult<PyObject*>::error_result(error);
    }
    
    // Call the function
    PyObject* result = PyObject_CallObject(func, args_tuple);
    Py_DECREF(args_tuple);
    
    end_performance_measurement(script_name, "python_function_call");
    
    if (!result) {
        auto error = handle_python_error(script_name, "function call: " + function_name);
        return ScriptResult<PyObject*>::error_result(error);
    }
    
    update_memory_statistics(script_name);
    
    return ScriptResult<PyObject*>::success_result(result, context->metrics);
}

template<typename ReturnType, typename... Args>
ScriptResult<ReturnType> PythonEngine::call_python_function_typed(const std::string& script_name,
                                                                 const std::string& function_name,
                                                                 Args&&... args) {
    auto result = call_python_function(script_name, function_name, std::forward<Args>(args)...);
    
    if (!result) {
        return ScriptResult<ReturnType>::error_result(result.error.value(), result.metrics);
    }
    
    PyObject* py_result = result.value();
    
    // Convert Python result to C++ type
    ReturnType cpp_result;
    if constexpr (std::is_same_v<ReturnType, bool>) {
        cpp_result = PythonTypeHelper::from_python_bool(py_result);
    } else if constexpr (std::is_same_v<ReturnType, int>) {
        cpp_result = PythonTypeHelper::from_python_int(py_result);
    } else if constexpr (std::is_same_v<ReturnType, float>) {
        cpp_result = PythonTypeHelper::from_python_float(py_result);
    } else if constexpr (std::is_same_v<ReturnType, double>) {
        cpp_result = PythonTypeHelper::from_python_double(py_result);
    } else if constexpr (std::is_same_v<ReturnType, std::string>) {
        cpp_result = PythonTypeHelper::from_python_string(py_result);
    } else {
        // For other types, you'd need custom conversion logic
        static_assert(std::is_same_v<ReturnType, void>, "Unsupported return type");
    }
    
    Py_DECREF(py_result); // Clean up Python object
    
    return ScriptResult<ReturnType>::success_result(std::move(cpp_result), result.metrics);
}

template<typename First, typename... Rest>
PyObject* PythonEngine::create_python_tuple_from_args(First&& first, Rest&&... rest) {
    constexpr size_t total_args = sizeof...(rest) + 1;
    PyObject* tuple = PyTuple_New(total_args);
    
    if (!tuple) {
        return nullptr;
    }
    
    // Set first argument
    PyObject* first_obj = convert_arg_to_python(std::forward<First>(first));
    if (!first_obj) {
        Py_DECREF(tuple);
        return nullptr;
    }
    PyTuple_SET_ITEM(tuple, 0, first_obj);
    
    // Set remaining arguments
    if constexpr (sizeof...(rest) > 0) {
        PyObject* rest_tuple = create_python_tuple_from_args(std::forward<Rest>(rest)...);
        if (!rest_tuple) {
            Py_DECREF(tuple);
            return nullptr;
        }
        
        for (Py_ssize_t i = 0; i < PyTuple_Size(rest_tuple); ++i) {
            PyObject* item = PyTuple_GetItem(rest_tuple, i);
            Py_INCREF(item); // PyTuple_SET_ITEM steals reference
            PyTuple_SET_ITEM(tuple, i + 1, item);
        }
        
        Py_DECREF(rest_tuple);
    }
    
    return tuple;
}

template<typename T>
PyObject* PythonEngine::convert_arg_to_python(T&& arg) {
    if constexpr (std::is_same_v<std::decay_t<T>, bool>) {
        return PythonTypeHelper::to_python(arg);
    } else if constexpr (std::is_integral_v<std::decay_t<T>>) {
        return PythonTypeHelper::to_python(static_cast<int>(arg));
    } else if constexpr (std::is_floating_point_v<std::decay_t<T>>) {
        return PythonTypeHelper::to_python(static_cast<double>(arg));
    } else if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
        return PythonTypeHelper::to_python(arg);
    } else if constexpr (std::is_same_v<std::decay_t<T>, const char*>) {
        return PythonTypeHelper::to_python(arg);
    } else if constexpr (std::is_pointer_v<std::decay_t<T>>) {
        return PythonTypeHelper::to_python(arg);
    } else {
        // For custom types, you'd need specific conversion logic
        static_assert(std::is_same_v<T, void>, "Unsupported argument type");
        Py_RETURN_NONE;
    }
}

} // namespace ecscope::scripting