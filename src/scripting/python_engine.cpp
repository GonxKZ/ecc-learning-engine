#include "python_engine.hpp"
#include "core/log.hpp"
#include "ecs/registry.hpp"
#include <fstream>
#include <filesystem>
#include <sstream>

namespace ecscope::scripting {

// Static member definitions for PythonECSBinder
PyMethodDef PythonECSBinder::ecs_methods_[] = {
    {"create_entity", py_create_entity, METH_NOARGS, "Create a new entity"},
    {"destroy_entity", py_destroy_entity, METH_VARARGS, "Destroy an entity"},
    {"add_component", reinterpret_cast<PyCFunction>(py_add_component), METH_VARARGS | METH_KEYWORDS, "Add component to entity"},
    {"get_component", py_get_component, METH_VARARGS, "Get component from entity"},
    {"has_component", py_has_component, METH_VARARGS, "Check if entity has component"},
    {"get_entities_with", py_get_entities_with, METH_VARARGS, "Get entities with specific components"},
    {nullptr, nullptr, 0, nullptr} // Sentinel
};

PyModuleDef PythonECSBinder::ecs_module_def_ = {
    PyModuleDef_HEAD_INIT,
    "ecscope_ecs",                    // Module name
    "ECScope ECS integration module", // Module documentation
    -1,                               // Size of per-interpreter state
    ecs_methods_,                     // Method definitions
    nullptr,                          // Slot definitions
    nullptr,                          // Traverse function
    nullptr,                          // Clear function
    nullptr                           // Free function
};

// PythonECSBinder implementation
PythonECSBinder::PythonECSBinder() = default;

PythonECSBinder::~PythonECSBinder() {
    shutdown();
}

bool PythonECSBinder::initialize() {
    if (ecs_module_) {
        return true; // Already initialized
    }
    
    // Create the ECS module
    ecs_module_ = PyModule_Create(&ecs_module_def_);
    if (!ecs_module_) {
        LOG_ERROR("Failed to create ECScope ECS Python module");
        return false;
    }
    
    // Add module to sys.modules
    PyObject* sys_modules = PyImport_GetModuleDict();
    if (PyDict_SetItemString(sys_modules, "ecscope_ecs", ecs_module_) < 0) {
        LOG_ERROR("Failed to add ECS module to sys.modules");
        Py_DECREF(ecs_module_);
        ecs_module_ = nullptr;
        return false;
    }
    
    register_ecs_constants();
    register_component_types();
    
    LOG_INFO("Python ECS binder initialized successfully");
    return true;
}

void PythonECSBinder::shutdown() {
    if (ecs_module_) {
        // The module will be cleaned up by Python interpreter shutdown
        ecs_module_ = nullptr;
    }
    bound_registry_ = nullptr;
}

void PythonECSBinder::bind_ecs_registry(ecs::Registry* registry) {
    bound_registry_ = registry;
    
    if (ecs_module_) {
        // Store registry pointer in module state (simplified approach)
        PyObject* registry_capsule = PyCapsule_New(static_cast<void*>(registry), 
                                                  "ecscope_registry", nullptr);
        if (registry_capsule) {
            PyModule_AddObject(ecs_module_, "_registry", registry_capsule);
        }
    }
    
    LOG_INFO("ECS registry bound to Python environment");
}

void PythonECSBinder::create_educational_examples() {
    if (!ecs_module_) {
        LOG_WARN("Cannot create educational examples - ECS module not initialized");
        return;
    }
    
    // Add educational example functions to the module
    const char* example_code = R"(
# Educational ECS examples in Python
import ecscope_ecs as ecs

def create_example_entities():
    """Create example entities for demonstration."""
    print("Creating example entities...")
    
    # Create player and enemy entities
    player = ecs.create_entity()
    enemy = ecs.create_entity()
    
    print(f"Created player entity: {player}")
    print(f"Created enemy entity: {enemy}")
    
    return player, enemy

def demonstrate_component_operations():
    """Demonstrate component operations."""
    print("Demonstrating component operations...")
    
    player, enemy = create_example_entities()
    
    # Note: Component operations would work with actual bindings
    print("Component operations would be demonstrated here")
    print("with actual ECS component bindings")
    
    return player, enemy

def ecs_performance_example():
    """Demonstrate ECS performance patterns."""
    import time
    
    print("ECS Performance Example")
    print("Note: This demonstrates Python-side patterns")
    
    # Simulate entity processing
    entities = list(range(1000))  # Simulated entity IDs
    
    start_time = time.time()
    
    # Simulate component processing
    for entity_id in entities:
        # This would process actual components in a real scenario
        result = entity_id * 2 + 1
    
    end_time = time.time()
    processing_time = (end_time - start_time) * 1000
    
    print(f"Processed {len(entities)} entities in {processing_time:.2f}ms")
    print(f"Average time per entity: {processing_time / len(entities):.4f}ms")

print("ECS educational examples loaded successfully!")
)";
    
    // Execute the example code in the module's namespace
    PyObject* module_dict = PyModule_GetDict(ecs_module_);
    if (module_dict) {
        PyRun_String(example_code, Py_file_input, module_dict, module_dict);
        if (PyErr_Occurred()) {
            LOG_ERROR("Error executing educational examples: {}", 
                     PythonTypeHelper::get_python_error());
            PyErr_Clear();
        }
    }
}

// Static C API function implementations
PyObject* PythonECSBinder::py_create_entity(PyObject* self, PyObject* args) {
    // Get registry from module
    PyObject* registry_capsule = PyObject_GetAttrString(self, "_registry");
    if (!registry_capsule || !PyCapsule_IsValid(registry_capsule, "ecscope_registry")) {
        PyErr_SetString(PyExc_RuntimeError, "ECS registry not available");
        return nullptr;
    }
    
    auto* registry = static_cast<ecs::Registry*>(PyCapsule_GetPointer(registry_capsule, "ecscope_registry"));
    if (!registry) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid ECS registry");
        return nullptr;
    }
    
    // Create entity
    ecs::Entity entity = registry->create_entity();
    
    return PyLong_FromLong(static_cast<long>(entity));
}

PyObject* PythonECSBinder::py_destroy_entity(PyObject* self, PyObject* args) {
    long entity_id;
    if (!PyArg_ParseTuple(args, "l", &entity_id)) {
        return nullptr;
    }
    
    // Get registry
    PyObject* registry_capsule = PyObject_GetAttrString(self, "_registry");
    if (!registry_capsule || !PyCapsule_IsValid(registry_capsule, "ecscope_registry")) {
        PyErr_SetString(PyExc_RuntimeError, "ECS registry not available");
        return nullptr;
    }
    
    auto* registry = static_cast<ecs::Registry*>(PyCapsule_GetPointer(registry_capsule, "ecscope_registry"));
    if (!registry) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid ECS registry");
        return nullptr;
    }
    
    // Destroy entity
    bool success = registry->destroy_entity(static_cast<ecs::Entity>(entity_id));
    
    return PyBool_FromLong(success ? 1 : 0);
}

PyObject* PythonECSBinder::py_add_component(PyObject* self, PyObject* args, PyObject* kwargs) {
    // Simplified implementation - would need proper component type handling
    PyErr_SetString(PyExc_NotImplementedError, 
                   "add_component requires specific component type bindings");
    return nullptr;
}

PyObject* PythonECSBinder::py_get_component(PyObject* self, PyObject* args) {
    // Simplified implementation
    PyErr_SetString(PyExc_NotImplementedError, 
                   "get_component requires specific component type bindings");
    return nullptr;
}

PyObject* PythonECSBinder::py_has_component(PyObject* self, PyObject* args) {
    // Simplified implementation
    PyErr_SetString(PyExc_NotImplementedError, 
                   "has_component requires specific component type bindings");
    return nullptr;
}

PyObject* PythonECSBinder::py_get_entities_with(PyObject* self, PyObject* args) {
    // Simplified implementation
    PyErr_SetString(PyExc_NotImplementedError, 
                   "get_entities_with requires query system bindings");
    return nullptr;
}

void PythonECSBinder::register_ecs_constants() {
    if (!ecs_module_) return;
    
    // Add useful constants
    PyModule_AddIntConstant(ecs_module_, "INVALID_ENTITY", 0);
    PyModule_AddStringConstant(ecs_module_, "VERSION", "1.0.0");
    
    // Add component type constants (would be expanded with actual component IDs)
    PyModule_AddIntConstant(ecs_module_, "TRANSFORM_COMPONENT", 1);
    PyModule_AddIntConstant(ecs_module_, "HEALTH_COMPONENT", 2);
    PyModule_AddIntConstant(ecs_module_, "VELOCITY_COMPONENT", 3);
}

void PythonECSBinder::register_component_types() {
    if (!ecs_module_) return;
    
    // This would register specific component types with their binding functions
    // For now, just add placeholder documentation
    PyModule_AddStringConstant(ecs_module_, "__doc__", 
        "ECScope ECS Python Integration\n\n"
        "This module provides Python bindings for the ECScope ECS system.\n"
        "Functions:\n"
        "  create_entity() -> int: Create a new entity\n"
        "  destroy_entity(entity_id) -> bool: Destroy an entity\n"
        "  add_component(entity_id, component_type, **data) -> bool: Add component\n"
        "  get_component(entity_id, component_type) -> dict: Get component data\n"
        "  has_component(entity_id, component_type) -> bool: Check component\n"
        "  get_entities_with(*component_types) -> list: Query entities\n"
    );
}

void PythonECSBinder::set_python_error(const std::string& message) {
    PyErr_SetString(PyExc_RuntimeError, message.c_str());
}

// PythonEngine implementation
PythonEngine::PythonEngine() : ScriptEngine("Python") {
    LOG_INFO("Initializing Python scripting engine");
}

PythonEngine::~PythonEngine() {
    shutdown();
}

bool PythonEngine::initialize() {
    if (initialized_) {
        LOG_WARN("Python engine already initialized");
        return true;
    }
    
    if (!initialize_python_interpreter()) {
        return false;
    }
    
    // Initialize ECS binder
    ecs_binder_ = std::make_unique<PythonECSBinder>();
    if (!ecs_binder_->initialize()) {
        LOG_ERROR("Failed to initialize Python ECS binder");
        shutdown();
        return false;
    }
    
    setup_python_paths();
    
    // Try to initialize NumPy
    initialize_numpy();
    
    initialized_ = true;
    LOG_INFO("Python scripting engine initialized successfully");
    
    return true;
}

void PythonEngine::shutdown() {
    if (!initialized_) {
        return;
    }
    
    // Unload all scripts
    unload_all_scripts();
    
    // Shutdown ECS binder
    if (ecs_binder_) {
        ecs_binder_->shutdown();
        ecs_binder_.reset();
    }
    
    // Finalize Python interpreter
    if (gil_initialized_) {
        if (Py_IsInitialized()) {
            Py_Finalize();
        }
        gil_initialized_ = false;
    }
    
    main_thread_state_ = nullptr;
    initialized_ = false;
    
    LOG_INFO("Python scripting engine shut down");
}

ScriptResult<void> PythonEngine::load_script(const std::string& name, const std::string& source) {
    if (!initialized_) {
        auto error = create_python_error(name, "Python engine not initialized");
        return ScriptResult<void>::error_result(error);
    }
    
    GILLock gil_lock;
    
    start_performance_measurement(name, "compilation");
    
    auto* context = create_python_context(name);
    if (!context || !context->is_valid()) {
        auto error = create_python_error(name, "Failed to create Python context");
        end_performance_measurement(name, "compilation");
        return ScriptResult<void>::error_result(error);
    }
    
    // Store source code
    context->source_code = source;
    
    // Compile the script
    PyObject* code = Py_CompileString(source.c_str(), name.c_str(), Py_file_input);
    if (!code) {
        auto error = handle_python_error(name, "compilation");
        end_performance_measurement(name, "compilation");
        return ScriptResult<void>::error_result(error);
    }
    
    // Execute the code to load definitions
    PyObject* result = PyEval_EvalCode(code, context->globals, context->locals);
    Py_DECREF(code);
    
    if (!result) {
        auto error = handle_python_error(name, "loading");
        end_performance_measurement(name, "compilation");
        return ScriptResult<void>::error_result(error);
    }
    
    Py_DECREF(result);
    
    context->is_compiled = true;
    context->is_loaded = true;
    
    end_performance_measurement(name, "compilation");
    
    LOG_INFO("Loaded Python script: {} ({} bytes)", name, source.size());
    
    return ScriptResult<void>::success_result(context->metrics);
}

ScriptResult<void> PythonEngine::load_script_file(const std::string& name, const std::string& filepath) {
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
        auto* context = get_python_context(name);
        if (context) {
            context->filepath = filepath;
            if (is_hot_reload_enabled()) {
                context->file_state = FileWatchState(filepath);
            }
        }
    }
    
    return result;
}

ScriptResult<void> PythonEngine::compile_script(const std::string& name) {
    auto* context = get_python_context(name);
    if (!context) {
        auto error = create_python_error(name, "Script not loaded");
        return ScriptResult<void>::error_result(error);
    }
    
    if (context->source_code.empty()) {
        auto error = create_python_error(name, "No source code to compile");
        return ScriptResult<void>::error_result(error);
    }
    
    GILLock gil_lock;
    
    start_performance_measurement(name, "compilation");
    
    // Invalidate function cache since we're recompiling
    context->invalidate_function_cache();
    
    // Clear previous locals
    PyDict_Clear(context->locals);
    
    // Re-compile and execute
    PyObject* code = Py_CompileString(context->source_code.c_str(), name.c_str(), Py_file_input);
    if (!code) {
        auto error = handle_python_error(name, "compilation");
        end_performance_measurement(name, "compilation");
        return ScriptResult<void>::error_result(error);
    }
    
    PyObject* result = PyEval_EvalCode(code, context->globals, context->locals);
    Py_DECREF(code);
    
    if (!result) {
        auto error = handle_python_error(name, "compilation execution");
        end_performance_measurement(name, "compilation");
        return ScriptResult<void>::error_result(error);
    }
    
    Py_DECREF(result);
    context->is_compiled = true;
    
    end_performance_measurement(name, "compilation");
    
    LOG_DEBUG("Recompiled Python script: {}", name);
    return ScriptResult<void>::success_result(context->metrics);
}

ScriptResult<void> PythonEngine::reload_script(const std::string& name) {
    auto* context = get_python_context(name);
    if (!context) {
        auto error = create_python_error(name, "Script not loaded");
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

ScriptResult<void> PythonEngine::execute_script(const std::string& name) {
    auto* context = get_python_context(name);
    if (!context || !context->is_valid()) {
        auto error = create_python_error(name, "Script not loaded or invalid");
        return ScriptResult<void>::error_result(error);
    }
    
    if (!context->is_compiled) {
        auto compile_result = compile_script(name);
        if (!compile_result) {
            return compile_result;
        }
    }
    
    GILLock gil_lock;
    
    start_performance_measurement(name, "execution");
    
    // Look for main function and execute it
    PyObject* main_func = PyDict_GetItemString(context->globals, "main");
    if (main_func && PyCallable_Check(main_func)) {
        PyObject* result = PyObject_CallObject(main_func, nullptr);
        if (!result) {
            auto error = handle_python_error(name, "main function execution");
            end_performance_measurement(name, "execution");
            return ScriptResult<void>::error_result(error);
        }
        Py_DECREF(result);
    } else {
        LOG_DEBUG("No main function found in script: {}, script loaded but not executed", name);
    }
    
    end_performance_measurement(name, "execution");
    update_memory_statistics(name);
    
    return ScriptResult<void>::success_result(context->metrics);
}

usize PythonEngine::get_memory_usage(const std::string& script_name) const {
    const auto* context = get_python_context(script_name);
    if (context && context->is_valid()) {
        return estimate_python_memory_usage(const_cast<PythonScriptContext*>(context));
    }
    return 0;
}

void PythonEngine::collect_garbage() {
    if (!initialized_) {
        return;
    }
    
    GILLock gil_lock;
    
    // Import gc module and run collection
    PyObject* gc_module = PyImport_ImportModule("gc");
    if (gc_module) {
        PyObject* collect_func = PyObject_GetAttrString(gc_module, "collect");
        if (collect_func && PyCallable_Check(collect_func)) {
            PyObject* result = PyObject_CallObject(collect_func, nullptr);
            if (result) {
                int collected = PyLong_AsLong(result);
                LOG_DEBUG("Python garbage collection freed {} objects", collected);
                Py_DECREF(result);
            }
            Py_DECREF(collect_func);
        }
        Py_DECREF(gc_module);
    }
    
    // Update memory statistics for all contexts
    for (const auto& script_name : get_loaded_scripts()) {
        update_memory_statistics(script_name);
    }
}

void PythonEngine::set_memory_limit(const std::string& script_name, usize limit_bytes) {
    script_memory_limits_[script_name] = limit_bytes;
    LOG_DEBUG("Set memory limit for Python script '{}': {} bytes", script_name, limit_bytes);
    
    // Note: Python doesn't have built-in per-script memory limits
    // This would require custom memory tracking or resource limits
}

std::string PythonEngine::get_version_info() const {
    return std::string("Python ") + Py_GetVersion() + " with ECScope integration";
}

std::string PythonEngine::explain_performance_characteristics() const {
    return R"(
Python Performance Characteristics in ECScope:

Strengths:
- Rich standard library and extensive ecosystem
- Excellent for rapid prototyping and development
- Strong support for scientific computing (NumPy, SciPy)
- Dynamic typing enables flexible scripting
- Comprehensive debugging and profiling tools
- Great for AI/ML algorithms and data processing

Weaknesses:
- Slower execution compared to compiled languages
- Global Interpreter Lock (GIL) limits threading
- Higher memory usage due to object overhead
- Dynamic typing can impact performance
- Interpreted nature adds execution overhead

Performance Considerations:
1. Python is generally 10-100x slower than C++ for compute-intensive tasks
2. GIL prevents true multi-threading for CPU-bound tasks
3. Object creation and garbage collection overhead
4. Function call overhead is higher than Lua
5. NumPy operations can be nearly as fast as C++ for numerical tasks

ECS Integration Benefits:
- Flexible component data structures using dictionaries
- Easy iteration and filtering of entities
- Rich debugging capabilities for game logic
- Rapid prototyping of game systems
- Integration with AI/ML libraries for intelligent behavior

Optimization Tips:
1. Use NumPy for numerical computations
2. Minimize Python ↔ C++ boundary crossings
3. Cache frequently-accessed objects
4. Use list comprehensions instead of explicit loops
5. Profile code to identify bottlenecks
6. Consider Cython for performance-critical sections
)";
}

std::vector<std::string> PythonEngine::get_optimization_suggestions(const std::string& script_name) const {
    std::vector<std::string> suggestions;
    
    const auto* context = get_python_context(script_name);
    if (!context) {
        suggestions.push_back("Script not found - cannot analyze");
        return suggestions;
    }
    
    const auto& metrics = context->metrics;
    
    // Memory-based suggestions
    if (metrics.memory_usage_bytes > 50 * 1024 * 1024) { // > 50MB
        suggestions.push_back("High memory usage detected - consider optimizing data structures");
        suggestions.push_back("Use generators instead of lists for large datasets");
        suggestions.push_back("Consider using NumPy arrays for numerical data");
    }
    
    // Performance-based suggestions
    if (metrics.performance_ratio > 100.0) {
        suggestions.push_back("Script is much slower than C++ - consider moving critical code to C++");
        suggestions.push_back("Profile the script to identify performance bottlenecks");
        suggestions.push_back("Consider using Cython for performance-critical functions");
    } else if (metrics.performance_ratio > 20.0) {
        suggestions.push_back("Consider caching expensive computations");
        suggestions.push_back("Use NumPy for numerical operations");
        suggestions.push_back("Minimize object creation in tight loops");
    }
    
    // Execution frequency suggestions
    if (metrics.execution_count > 1000) {
        suggestions.push_back("Frequently executed script - ensure optimal performance");
        suggestions.push_back("Consider pre-compiling frequently used functions");
    }
    
    // Python-specific suggestions
    if (numpy_available_) {
        suggestions.push_back("NumPy is available - use it for mathematical operations");
    } else {
        suggestions.push_back("Consider installing NumPy for better numerical performance");
    }
    
    if (suggestions.empty()) {
        suggestions.push_back("Script performance is acceptable for Python");
        suggestions.push_back("Consider profiling if more optimization is needed");
    }
    
    return suggestions;
}

void PythonEngine::bind_ecs_registry(ecs::Registry* registry) {
    if (!initialized_ || !ecs_binder_) {
        LOG_ERROR("Cannot bind ECS registry - Python engine not properly initialized");
        return;
    }
    
    bound_registry_ = registry;
    ecs_binder_->bind_ecs_registry(registry);
    ecs_binder_->create_educational_examples();
    
    LOG_INFO("ECS registry bound to Python engine");
}

void PythonEngine::import_module(const std::string& module_name) {
    if (!initialized_) {
        LOG_ERROR("Cannot import module - Python engine not initialized");
        return;
    }
    
    GILLock gil_lock;
    
    PyObject* module = PyImport_ImportModule(module_name.c_str());
    if (module) {
        LOG_INFO("Successfully imported Python module: {}", module_name);
        Py_DECREF(module);
    } else {
        LOG_ERROR("Failed to import Python module: {} - {}", 
                 module_name, PythonTypeHelper::get_python_error());
        PyErr_Clear();
    }
}

void PythonEngine::add_to_sys_path(const std::string& path) {
    if (!initialized_) {
        return;
    }
    
    GILLock gil_lock;
    
    PyObject* sys_path = PySys_GetObject("path");
    if (sys_path && PyList_Check(sys_path)) {
        PyObject* path_str = PyUnicode_FromString(path.c_str());
        if (path_str) {
            PyList_Append(sys_path, path_str);
            Py_DECREF(path_str);
            LOG_DEBUG("Added to Python sys.path: {}", path);
        }
    }
}

void PythonEngine::set_global_variable(const std::string& name, PyObject* value) {
    if (!initialized_) {
        return;
    }
    
    GILLock gil_lock;
    
    PyObject* main_module = PyImport_AddModule("__main__");
    if (main_module) {
        PyObject* main_dict = PyModule_GetDict(main_module);
        if (main_dict) {
            Py_INCREF(value);
            PyDict_SetItemString(main_dict, name.c_str(), value);
        }
    }
}

PyObject* PythonEngine::get_global_variable(const std::string& name) {
    if (!initialized_) {
        return nullptr;
    }
    
    GILLock gil_lock;
    
    PyObject* main_module = PyImport_AddModule("__main__");
    if (main_module) {
        PyObject* main_dict = PyModule_GetDict(main_module);
        if (main_dict) {
            PyObject* value = PyDict_GetItemString(main_dict, name.c_str());
            if (value) {
                Py_INCREF(value); // Return new reference
                return value;
            }
        }
    }
    
    return nullptr;
}

void PythonEngine::create_tutorial_scripts() {
    if (!initialized_) {
        LOG_ERROR("Cannot create tutorial scripts - Python engine not initialized");
        return;
    }
    
    generate_basic_python_tutorial();
    generate_ecs_python_tutorial();
    if (numpy_available_) {
        generate_numpy_tutorial();
    }
    generate_performance_tutorial();
    
    LOG_INFO("Created Python tutorial scripts");
}

void PythonEngine::demonstrate_numpy_integration() {
    if (!numpy_available_) {
        LOG_WARN("NumPy not available - cannot demonstrate NumPy integration");
        return;
    }
    
    const char* numpy_demo = R"(
import numpy as np
import time

def numpy_performance_demo():
    """Demonstrate NumPy performance compared to pure Python."""
    
    size = 1000000
    print(f"Performance comparison with {size} elements")
    
    # Pure Python version
    start_time = time.time()
    python_list = list(range(size))
    python_result = [x * 2 + 1 for x in python_list]
    python_time = time.time() - start_time
    
    # NumPy version
    start_time = time.time()
    numpy_array = np.arange(size)
    numpy_result = numpy_array * 2 + 1
    numpy_time = time.time() - start_time
    
    speedup = python_time / numpy_time if numpy_time > 0 else 0
    
    print(f"Pure Python time: {python_time:.4f}s")
    print(f"NumPy time: {numpy_time:.4f}s")
    print(f"NumPy speedup: {speedup:.2f}x")
    
    return speedup

def matrix_operations_demo():
    """Demonstrate matrix operations with NumPy."""
    print("Matrix operations demonstration")
    
    # Create random matrices
    a = np.random.rand(1000, 1000)
    b = np.random.rand(1000, 1000)
    
    start_time = time.time()
    c = np.dot(a, b)  # Matrix multiplication
    end_time = time.time()
    
    print(f"1000x1000 matrix multiplication: {(end_time - start_time):.4f}s")
    print(f"Result shape: {c.shape}")
    print(f"Result mean: {c.mean():.4f}")

if __name__ == "__main__":
    numpy_performance_demo()
    matrix_operations_demo()
)";
    
    auto result = load_script("numpy_demo", numpy_demo);
    if (result) {
        execute_script("numpy_demo");
    } else {
        LOG_ERROR("Failed to load NumPy demonstration script");
    }
}

void PythonEngine::explain_gil_implications() const {
    LOG_INFO(R"(
Python Global Interpreter Lock (GIL) Implications:

What is the GIL?
- A mutex that protects Python object access
- Only one thread can execute Python bytecode at a time
- Prevents true multi-threading for CPU-bound tasks

Impact on ECScope Integration:
1. CPU-bound scripts won't benefit from multiple threads
2. I/O-bound operations can still use threading effectively
3. C++ code can release GIL for parallel execution
4. Consider using multiprocessing for CPU-intensive tasks

Workarounds:
- Use NumPy for numerical computations (releases GIL)
- Implement CPU-intensive logic in C++
- Use asyncio for concurrent I/O operations
- Consider Jython or IronPython for true threading

Best Practices:
- Keep scripts I/O-bound or use C++ for computation
- Use threading for I/O, multiprocessing for CPU work
- Profile to understand actual performance bottlenecks
- Consider the GIL when designing parallel systems
)");
}

bool PythonEngine::initialize_numpy() {
    if (!initialized_) {
        return false;
    }
    
    GILLock gil_lock;
    
    // Try to import NumPy
    PyObject* numpy_module = PyImport_ImportModule("numpy");
    if (numpy_module) {
        numpy_available_ = true;
        Py_DECREF(numpy_module);
        LOG_INFO("NumPy integration available");
        return true;
    } else {
        PyErr_Clear();
        numpy_available_ = false;
        LOG_INFO("NumPy not available - numerical operations will use pure Python");
        return false;
    }
}

void PythonEngine::acquire_gil() {
    if (gil_initialized_) {
        PyGILState_Ensure();
    }
}

void PythonEngine::release_gil() {
    if (gil_initialized_) {
        PyGILState_Release(PyGILState_Ensure()); // This is a bit hacky
    }
}

void PythonEngine::enable_profiling(const std::string& script_name) {
    profiling_enabled_[script_name] = true;
    LOG_DEBUG("Enabled profiling for Python script: {}", script_name);
}

void PythonEngine::disable_profiling(const std::string& script_name) {
    profiling_enabled_[script_name] = false;
    LOG_DEBUG("Disabled profiling for Python script: {}", script_name);
}

std::string PythonEngine::get_profiling_results(const std::string& script_name) const {
    auto it = profiling_enabled_.find(script_name);
    if (it == profiling_enabled_.end() || !it->second) {
        return "Profiling not enabled for script: " + script_name;
    }
    
    // This would integrate with Python's cProfile or other profiling tools
    return "Profiling results for " + script_name + " (implementation needed)";
}

// Protected method implementations
ScriptResult<void> PythonEngine::call_function_impl_void(const std::string& script_name,
                                                        const std::string& function_name,
                                                        const std::vector<std::any>& args) {
    auto* context = get_python_context(script_name);
    if (!context || !context->is_valid()) {
        auto error = create_python_error(script_name, "Script not loaded");
        return ScriptResult<void>::error_result(error);
    }
    
    GILLock gil_lock;
    
    // Get function
    PyObject* func = PyDict_GetItemString(context->globals, function_name.c_str());
    if (!func || !PyCallable_Check(func)) {
        auto error = create_python_error(script_name, "Function not found: " + function_name);
        return ScriptResult<void>::error_result(error);
    }
    
    // Create arguments tuple (simplified - would need proper any handling)
    PyObject* args_tuple = PyTuple_New(args.size());
    for (size_t i = 0; i < args.size(); ++i) {
        // This would need proper type conversion from std::any
        PyTuple_SetItem(args_tuple, i, Py_None);
        Py_INCREF(Py_None);
    }
    
    // Call function
    PyObject* result = PyObject_CallObject(func, args_tuple);
    Py_DECREF(args_tuple);
    
    if (!result) {
        auto error = handle_python_error(script_name, "function call");
        return ScriptResult<void>::error_result(error);
    }
    
    Py_DECREF(result);
    return ScriptResult<void>::success_result(context->metrics);
}

// Private utility method implementations
PythonScriptContext* PythonEngine::get_python_context(const std::string& name) {
    auto* context = get_script_context(name);
    return static_cast<PythonScriptContext*>(context);
}

const PythonScriptContext* PythonEngine::get_python_context(const std::string& name) const {
    auto* context = get_script_context(name);
    return static_cast<const PythonScriptContext*>(context);
}

PythonScriptContext* PythonEngine::create_python_context(const std::string& name) {
    // Remove existing context if it exists
    remove_script_context(name);
    
    auto python_context = std::make_unique<PythonScriptContext>(name);
    PythonScriptContext* context_ptr = python_context.get();
    
    // Transfer ownership to base class storage
    script_contexts_[name] = std::unique_ptr<ScriptContext>(
        static_cast<ScriptContext*>(python_context.release()));
    
    return context_ptr;
}

ScriptError PythonEngine::create_python_error(const std::string& script_name, const std::string& message) const {
    return ScriptError{
        ScriptError::Type::RuntimeError,
        message,
        script_name,
        0, 0,
        "",
        "Check Python script syntax and ECScope integration"
    };
}

ScriptError PythonEngine::handle_python_error(const std::string& script_name, const std::string& operation) const {
    std::string error_message = PythonTypeHelper::get_python_error();
    
    return ScriptError{
        ScriptError::Type::RuntimeError,
        error_message,
        script_name,
        0, 0, // Line number extraction would need traceback parsing
        "",
        "Python error during " + operation + " - check script syntax and logic"
    };
}

void PythonEngine::update_memory_statistics(const std::string& script_name) {
    auto* context = get_python_context(script_name);
    if (!context || !context->is_valid()) {
        return;
    }
    
    usize memory_usage = estimate_python_memory_usage(context);
    context->metrics.memory_usage_bytes = memory_usage;
    context->metrics.peak_memory_usage_bytes = std::max(
        context->metrics.peak_memory_usage_bytes,
        memory_usage
    );
}

usize PythonEngine::estimate_python_memory_usage(PythonScriptContext* context) const {
    if (!context) {
        return 0;
    }
    
    // This is a simplified estimation
    // Real implementation would traverse Python objects
    usize estimated_size = 0;
    
    std::unordered_set<PyObject*> visited;
    
    if (context->globals) {
        estimated_size += walk_object_tree(context->globals, visited);
    }
    
    if (context->locals) {
        estimated_size += walk_object_tree(context->locals, visited);
    }
    
    return estimated_size;
}

bool PythonEngine::initialize_python_interpreter() {
    if (Py_IsInitialized()) {
        LOG_WARN("Python interpreter already initialized");
        return true;
    }
    
    // Initialize Python
    Py_Initialize();
    
    if (!Py_IsInitialized()) {
        LOG_ERROR("Failed to initialize Python interpreter");
        return false;
    }
    
    // Initialize threading support
    if (!PyEval_ThreadsInitialized()) {
        PyEval_InitThreads();
    }
    
    // Save main thread state
    main_thread_state_ = PyEval_SaveThread();
    gil_initialized_ = true;
    
    LOG_INFO("Python interpreter initialized successfully");
    return true;
}

void PythonEngine::setup_python_paths() {
    // Add script directories to Python path
    add_to_sys_path("scripts/");
    add_to_sys_path("scripts/tutorials/");
    add_to_sys_path("scripts/examples/");
}

void PythonEngine::setup_signal_handlers() {
    // Setup signal handlers for graceful shutdown
    // This would integrate with Python's signal module
}

void PythonEngine::generate_basic_python_tutorial() const {
    const char* tutorial_content = R"(
#!/usr/bin/env python3
"""ECScope Python Tutorial: Basics

This tutorial demonstrates basic Python scripting integration with ECScope.
"""

import sys
import time

# Global variables
player_health = 100
player_name = "Hero"
player_level = 1

def heal_player(amount):
    """Heal the player by the specified amount."""
    global player_health
    player_health += amount
    print(f"Player healed by {amount} - New health: {player_health}")

def damage_player(amount):
    """Damage the player by the specified amount."""
    global player_health
    player_health -= amount
    if player_health <= 0:
        player_health = 0
        print("Player has died!")
    else:
        print(f"Player damaged by {amount} - Health remaining: {player_health}")

def level_up():
    """Level up the player."""
    global player_level
    player_level += 1
    print(f"Level up! Now level {player_level}")

class PlayerStats:
    """Player statistics class."""
    
    def __init__(self):
        self.strength = 10
        self.agility = 8
        self.intelligence = 12
        
    def display(self):
        print(f"Stats - STR: {self.strength}, AGI: {self.agility}, INT: {self.intelligence}")
        
    def increase_stats(self, str_boost=2, agi_boost=1, int_boost=1):
        self.strength += str_boost
        self.agility += agi_boost
        self.intelligence += int_boost

def main():
    """Main tutorial function."""
    print("=== ECScope Python Tutorial: Basics ===")
    print(f"Player: {player_name}, Health: {player_health}, Level: {player_level}")
    
    # Demonstrate function calls
    heal_player(25)
    damage_player(50)
    level_up()
    
    # Demonstrate class usage
    stats = PlayerStats()
    stats.display()
    stats.increase_stats()
    stats.display()
    
    print("Tutorial completed!")

if __name__ == "__main__":
    main()
)";
    
    std::filesystem::create_directories("scripts/tutorials");
    std::ofstream file("scripts/tutorials/python_basics.py");
    file << tutorial_content;
    file.close();
    
    LOG_INFO("Generated basic Python tutorial script");
}

void PythonEngine::generate_ecs_python_tutorial() const {
    const char* ecs_tutorial = R"(
#!/usr/bin/env python3
"""ECScope Python Tutorial: ECS Integration

This tutorial demonstrates ECS integration patterns with Python.
"""

try:
    import ecscope_ecs as ecs
    ECS_AVAILABLE = True
except ImportError:
    print("Warning: ECScope ECS module not available")
    ECS_AVAILABLE = False

def create_game_entities():
    """Create game entities using ECS."""
    if not ECS_AVAILABLE:
        print("ECS not available - simulating entity creation")
        return [1, 2, 3]  # Mock entity IDs
    
    print("Creating game entities...")
    
    # Create player
    player = ecs.create_entity()
    print(f"Created player entity: {player}")
    
    # Create enemies
    enemies = []
    for i in range(3):
        enemy = ecs.create_entity()
        enemies.append(enemy)
        print(f"Created enemy entity: {enemy}")
    
    return [player] + enemies

def simulate_component_system():
    """Simulate a component system."""
    print("Simulating component system processing...")
    
    # Mock entity data
    entities = [
        {"id": 1, "transform": {"x": 0, "y": 0}, "velocity": {"x": 1, "y": 0}},
        {"id": 2, "transform": {"x": 10, "y": 5}, "velocity": {"x": -0.5, "y": 0.5}},
        {"id": 3, "transform": {"x": -5, "y": 3}, "velocity": {"x": 0.2, "y": -0.3}}
    ]
    
    # Movement system simulation
    for entity in entities:
        transform = entity["transform"]
        velocity = entity["velocity"]
        
        # Update position
        transform["x"] += velocity["x"]
        transform["y"] += velocity["y"]
        
        print(f"Entity {entity['id']} moved to ({transform['x']:.1f}, {transform['y']:.1f})")

def performance_comparison():
    """Compare Python performance patterns."""
    import time
    
    print("Python Performance Comparison")
    
    # Test different approaches
    size = 100000
    
    # List comprehension approach
    start_time = time.time()
    result1 = [x * 2 + 1 for x in range(size)]
    list_comp_time = time.time() - start_time
    
    # Traditional loop approach
    start_time = time.time()
    result2 = []
    for x in range(size):
        result2.append(x * 2 + 1)
    loop_time = time.time() - start_time
    
    # Map approach
    start_time = time.time()
    result3 = list(map(lambda x: x * 2 + 1, range(size)))
    map_time = time.time() - start_time
    
    print(f"List comprehension: {list_comp_time:.4f}s")
    print(f"Traditional loop: {loop_time:.4f}s")
    print(f"Map function: {map_time:.4f}s")
    print(f"Best approach: {'List comprehension' if list_comp_time < min(loop_time, map_time) else 'Map' if map_time < loop_time else 'Loop'}")

def demonstrate_python_features():
    """Demonstrate useful Python features for game development."""
    print("Python Features for Game Development")
    
    # Dictionaries for flexible data
    game_config = {
        "player_speed": 5.0,
        "enemy_count": 10,
        "world_size": (1024, 768),
        "debug_mode": True
    }
    
    print("Game configuration:")
    for key, value in game_config.items():
        print(f"  {key}: {value}")
    
    # List operations
    entities = list(range(1, 11))
    active_entities = [e for e in entities if e % 2 == 0]  # Even entities
    print(f"Active entities: {active_entities}")
    
    # Exception handling
    try:
        risky_operation = 10 / 0
    except ZeroDivisionError as e:
        print(f"Handled error: {e}")

def main():
    """Main ECS tutorial function."""
    print("=== ECScope Python Tutorial: ECS Integration ===")
    
    entities = create_game_entities()
    simulate_component_system()
    performance_comparison()
    demonstrate_python_features()
    
    print("ECS integration tutorial completed!")

if __name__ == "__main__":
    main()
)";
    
    std::filesystem::create_directories("scripts/tutorials");
    std::ofstream file("scripts/tutorials/ecs_python_integration.py");
    file << ecs_tutorial;
    file.close();
    
    LOG_INFO("Generated ECS Python integration tutorial script");
}

void PythonEngine::generate_numpy_tutorial() const {
    const char* numpy_tutorial = R"(
#!/usr/bin/env python3
"""ECScope Python Tutorial: NumPy Integration

This tutorial demonstrates NumPy integration for high-performance numerical operations.
"""

import numpy as np
import time

def numpy_basics():
    """Demonstrate basic NumPy operations."""
    print("=== NumPy Basics ===")
    
    # Create arrays
    vec3 = np.array([1.0, 2.0, 3.0])
    matrix = np.array([[1, 2], [3, 4]])
    
    print(f"3D Vector: {vec3}")
    print(f"2x2 Matrix:\n{matrix}")
    
    # Vector operations
    vec3_normalized = vec3 / np.linalg.norm(vec3)
    print(f"Normalized vector: {vec3_normalized}")
    
    # Matrix operations
    determinant = np.linalg.det(matrix)
    print(f"Matrix determinant: {determinant}")

def performance_demonstration():
    """Demonstrate NumPy performance benefits."""
    print("\n=== Performance Comparison ===")
    
    size = 1000000
    
    # Pure Python approach
    start_time = time.time()
    python_list = list(range(size))
    python_result = [x * 2.5 + 1.0 for x in python_list]
    python_sum = sum(python_result)
    python_time = time.time() - start_time
    
    # NumPy approach
    start_time = time.time()
    numpy_array = np.arange(size, dtype=np.float32)
    numpy_result = numpy_array * 2.5 + 1.0
    numpy_sum = np.sum(numpy_result)
    numpy_time = time.time() - start_time
    
    speedup = python_time / numpy_time if numpy_time > 0 else float('inf')
    
    print(f"Array size: {size:,}")
    print(f"Pure Python: {python_time:.4f}s (sum: {python_sum:,.0f})")
    print(f"NumPy:       {numpy_time:.4f}s (sum: {numpy_sum:,.0f})")
    print(f"Speedup:     {speedup:.2f}x")

def game_math_with_numpy():
    """Demonstrate game-related math operations with NumPy."""
    print("\n=== Game Math with NumPy ===")
    
    # Transform matrices
    def create_rotation_matrix(angle_rad):
        cos_a, sin_a = np.cos(angle_rad), np.sin(angle_rad)
        return np.array([[cos_a, -sin_a], [sin_a, cos_a]])
    
    def create_translation_matrix(tx, ty):
        return np.array([[1, 0, tx], [0, 1, ty], [0, 0, 1]])
    
    # Rotate a point
    point = np.array([10.0, 0.0])
    rotation_45 = create_rotation_matrix(np.pi / 4)  # 45 degrees
    rotated_point = rotation_45 @ point
    
    print(f"Original point: {point}")
    print(f"Rotated point (45°): {rotated_point}")
    
    # Batch process multiple entities
    positions = np.random.rand(1000, 2) * 100  # 1000 random positions
    velocities = np.random.rand(1000, 2) * 10   # Random velocities
    
    # Update all positions at once
    dt = 1.0/60.0  # 60 FPS
    new_positions = positions + velocities * dt
    
    print(f"Updated {len(positions)} entity positions in batch")
    print(f"Average position: {np.mean(new_positions, axis=0)}")

def collision_detection_example():
    """Demonstrate collision detection with NumPy."""
    print("\n=== Collision Detection Example ===")
    
    # Circle-circle collision detection for multiple objects
    def check_circle_collisions(positions, radii):
        n = len(positions)
        collisions = []
        
        # Vectorized distance calculation
        pos_expanded = positions[:, np.newaxis, :]  # Shape: (n, 1, 2)
        distances = np.linalg.norm(pos_expanded - positions, axis=2)  # Shape: (n, n)
        radii_sum = radii[:, np.newaxis] + radii  # Shape: (n, n)
        
        # Find collisions (excluding self-collision)
        collision_mask = (distances < radii_sum) & (distances > 0)
        collision_indices = np.where(collision_mask)
        
        return list(zip(collision_indices[0], collision_indices[1]))
    
    # Test with random circles
    num_circles = 100
    positions = np.random.rand(num_circles, 2) * 50
    radii = np.random.rand(num_circles) * 2 + 1
    
    start_time = time.time()
    collisions = check_circle_collisions(positions, radii)
    collision_time = time.time() - start_time
    
    print(f"Checked {num_circles} circles for collisions in {collision_time:.4f}s")
    print(f"Found {len(collisions)} collision pairs")

def main():
    """Main NumPy tutorial function."""
    print("=== ECScope Python Tutorial: NumPy Integration ===")
    
    numpy_basics()
    performance_demonstration()
    game_math_with_numpy()
    collision_detection_example()
    
    print("\nNumPy integration tutorial completed!")

if __name__ == "__main__":
    main()
)";
    
    std::filesystem::create_directories("scripts/tutorials");
    std::ofstream file("scripts/tutorials/numpy_integration.py");
    file << numpy_tutorial;
    file.close();
    
    LOG_INFO("Generated NumPy integration tutorial script");
}

void PythonEngine::generate_performance_tutorial() const {
    const char* performance_tutorial = R"(
#!/usr/bin/env python3
"""ECScope Python Tutorial: Performance Analysis

This tutorial analyzes Python performance characteristics and optimization techniques.
"""

import time
import sys
from typing import List, Dict, Any

def benchmark_function(name: str, func, iterations: int = 1000):
    """Benchmark a function with multiple iterations."""
    print(f"\nBenchmarking: {name} ({iterations:,} iterations)")
    
    start_time = time.time()
    for _ in range(iterations):
        result = func()
    end_time = time.time()
    
    total_time = (end_time - start_time) * 1000  # Convert to ms
    avg_time = total_time / iterations
    
    print(f"Total time: {total_time:.4f}ms")
    print(f"Average time: {avg_time:.6f}ms per iteration")
    print(f"Operations per second: {1000 / avg_time:.0f}")
    
    return avg_time

def test_data_structures():
    """Test performance of different data structures."""
    print("=== Data Structure Performance ===")
    
    size = 10000
    
    # List operations
    def list_operations():
        data = list(range(size))
        data.append(size)
        data.pop()
        return len(data)
    
    # Dictionary operations
    def dict_operations():
        data = {i: i*2 for i in range(size)}
        data[size] = size * 2
        del data[size]
        return len(data)
    
    # Set operations
    def set_operations():
        data = set(range(size))
        data.add(size)
        data.discard(size)
        return len(data)
    
    list_time = benchmark_function("List Operations", list_operations, 100)
    dict_time = benchmark_function("Dict Operations", dict_operations, 100)
    set_time = benchmark_function("Set Operations", set_operations, 100)
    
    fastest = min(list_time, dict_time, set_time)
    print(f"\nFastest: {'List' if fastest == list_time else 'Dict' if fastest == dict_time else 'Set'}")

def test_loop_patterns():
    """Test different loop patterns."""
    print("\n=== Loop Pattern Performance ===")
    
    data = list(range(10000))
    
    # Traditional for loop
    def traditional_loop():
        result = []
        for item in data:
            result.append(item * 2)
        return result
    
    # List comprehension
    def list_comprehension():
        return [item * 2 for item in data]
    
    # Map function
    def map_function():
        return list(map(lambda x: x * 2, data))
    
    # NumPy (if available)
    try:
        import numpy as np
        numpy_data = np.array(data)
        
        def numpy_operation():
            return numpy_data * 2
        
        numpy_available = True
    except ImportError:
        numpy_available = False
    
    trad_time = benchmark_function("Traditional Loop", traditional_loop, 1000)
    comp_time = benchmark_function("List Comprehension", list_comprehension, 1000)
    map_time = benchmark_function("Map Function", map_function, 1000)
    
    if numpy_available:
        numpy_time = benchmark_function("NumPy Operation", numpy_operation, 1000)
        fastest_time = min(trad_time, comp_time, map_time, numpy_time)
    else:
        fastest_time = min(trad_time, comp_time, map_time)
    
    print(f"Performance ranking:")
    results = [("Traditional", trad_time), ("Comprehension", comp_time), ("Map", map_time)]
    if numpy_available:
        results.append(("NumPy", numpy_time))
    
    results.sort(key=lambda x: x[1])
    for i, (name, time_val) in enumerate(results, 1):
        speedup = results[-1][1] / time_val
        print(f"{i}. {name}: {speedup:.2f}x faster than slowest")

def test_memory_patterns():
    """Test memory usage patterns."""
    print("\n=== Memory Usage Patterns ===")
    
    def memory_efficient_generator():
        """Generator that yields values on demand."""
        for i in range(1000000):
            yield i * 2
    
    def memory_intensive_list():
        """List that creates all values at once."""
        return [i * 2 for i in range(1000000)]
    
    # Test generator
    start_time = time.time()
    gen_data = memory_efficient_generator()
    first_100 = [next(gen_data) for _ in range(100)]
    gen_time = time.time() - start_time
    
    # Test list
    start_time = time.time()
    list_data = memory_intensive_list()
    first_100_list = list_data[:100]
    list_time = time.time() - start_time
    
    print(f"Generator (first 100): {gen_time:.6f}s")
    print(f"Full list (first 100): {list_time:.6f}s")
    print(f"Generator memory: ~constant")
    print(f"List memory: ~{sys.getsizeof(list_data) // (1024*1024)}MB")

def test_function_call_overhead():
    """Test function call overhead."""
    print("\n=== Function Call Overhead ===")
    
    def simple_add(a, b):
        return a + b
    
    class Calculator:
        def add(self, a, b):
            return a + b
    
    calc = Calculator()
    
    # Direct operation
    def direct_operation():
        return 5 + 3
    
    # Function call
    def function_call():
        return simple_add(5, 3)
    
    # Method call
    def method_call():
        return calc.add(5, 3)
    
    direct_time = benchmark_function("Direct Operation", direct_operation, 100000)
    func_time = benchmark_function("Function Call", function_call, 100000)
    method_time = benchmark_function("Method Call", method_call, 100000)
    
    print(f"Function overhead: {func_time / direct_time:.2f}x slower")
    print(f"Method overhead: {method_time / direct_time:.2f}x slower")

def optimization_recommendations():
    """Provide optimization recommendations."""
    print("\n=== Optimization Recommendations ===")
    
    recommendations = [
        "1. Use list comprehensions instead of traditional loops",
        "2. Prefer generators for large datasets to save memory",
        "3. Use NumPy for numerical computations when possible",
        "4. Cache frequently-called function results",
        "5. Use built-in functions (they're implemented in C)",
        "6. Minimize object creation in tight loops",
        "7. Use local variables instead of global ones",
        "8. Consider using sets for membership testing",
        "9. Profile your code to identify actual bottlenecks",
        "10. Move CPU-intensive code to C++ when necessary"
    ]
    
    for recommendation in recommendations:
        print(recommendation)

def main():
    """Main performance tutorial function."""
    print("=== ECScope Python Tutorial: Performance Analysis ===")
    
    test_data_structures()
    test_loop_patterns()
    test_memory_patterns()
    test_function_call_overhead()
    optimization_recommendations()
    
    print("\nPerformance analysis tutorial completed!")
    print("Use these insights to optimize your ECScope Python scripts!")

if __name__ == "__main__":
    main()
)";
    
    std::filesystem::create_directories("scripts/tutorials");
    std::ofstream file("scripts/tutorials/performance_analysis.py");
    file << performance_tutorial;
    file.close();
    
    LOG_INFO("Generated performance analysis tutorial script");
}

usize PythonEngine::estimate_object_size(PyObject* obj) const {
    if (!obj) {
        return 0;
    }
    
    // Basic size estimation based on object type
    if (PyLong_Check(obj)) {
        return sizeof(PyLongObject);
    } else if (PyFloat_Check(obj)) {
        return sizeof(PyFloatObject);
    } else if (PyUnicode_Check(obj)) {
        Py_ssize_t size = PyUnicode_GET_LENGTH(obj);
        return sizeof(PyUnicodeObject) + size * sizeof(Py_UCS4);
    } else if (PyList_Check(obj)) {
        Py_ssize_t size = PyList_GET_SIZE(obj);
        return sizeof(PyListObject) + size * sizeof(PyObject*);
    } else if (PyDict_Check(obj)) {
        Py_ssize_t size = PyDict_Size(obj);
        return sizeof(PyDictObject) + size * sizeof(PyDictKeyEntry);
    } else {
        // Default estimate for unknown objects
        return sizeof(PyObject);
    }
}

usize PythonEngine::walk_object_tree(PyObject* obj, std::unordered_set<PyObject*>& visited) const {
    if (!obj || visited.count(obj)) {
        return 0;
    }
    
    visited.insert(obj);
    usize total_size = estimate_object_size(obj);
    
    // Recursively walk containers (simplified)
    if (PyDict_Check(obj)) {
        PyObject* key, *value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(obj, &pos, &key, &value)) {
            total_size += walk_object_tree(key, visited);
            total_size += walk_object_tree(value, visited);
        }
    } else if (PyList_Check(obj)) {
        Py_ssize_t size = PyList_GET_SIZE(obj);
        for (Py_ssize_t i = 0; i < size; ++i) {
            PyObject* item = PyList_GET_ITEM(obj, i);
            total_size += walk_object_tree(item, visited);
        }
    }
    
    return total_size;
}

} // namespace ecscope::scripting