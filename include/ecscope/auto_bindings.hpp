#pragma once

#include "script_engine.hpp"
#include "lua_engine.hpp"
#include "python_engine.hpp"
#include "core/types.hpp"
#include "ecs/component.hpp"
#include "ecs/entity.hpp"
#include <string>
#include <typeinfo>
#include <type_traits>
#include <unordered_map>
#include <functional>
#include <memory>
#include <vector>
#include <any>

namespace ecscope::scripting {

// Forward declarations
namespace ecs { 
    class Registry; 
    template<typename T> struct Transform;
    struct Health;
    struct Velocity;
    struct Sprite;
}

/**
 * @brief Type information for automatic binding generation
 */
struct ComponentTypeInfo {
    std::string name;
    std::string cpp_type_name;
    usize size;
    usize alignment;
    std::type_index type_index;
    
    // Field information
    struct FieldInfo {
        std::string name;
        std::string type_name;
        usize offset;
        usize size;
        
        // Type accessors
        std::function<std::any(const void*)> getter;
        std::function<void(void*, const std::any&)> setter;
        
        // Type-specific conversion functions
        std::function<void(lua_State*, const void*)> lua_push;
        std::function<void(const void*, lua_State*, int)> lua_get;
        
        std::function<PyObject*(const void*)> python_to_object;
        std::function<void(void*, PyObject*)> python_from_object;
    };
    
    std::vector<FieldInfo> fields;
    
    // Constructor/destructor functions
    std::function<void*(const std::any&)> constructor;
    std::function<void(void*)> destructor;
    
    // Serialization functions
    std::function<std::string(const void*)> to_string;
    std::function<std::any(const std::string&)> from_string;
    
    ComponentTypeInfo(const std::string& component_name, const std::type_info& type_info)
        : name(component_name)
        , cpp_type_name(type_info.name())
        , size(0)
        , alignment(0)
        , type_index(type_info) {}
};

/**
 * @brief Component reflection registry
 */
class ComponentRegistry {
public:
    static ComponentRegistry& instance();
    
    template<typename Component>
    void register_component(const std::string& name);
    
    template<typename Component>
    void register_component_auto();
    
    const ComponentTypeInfo* get_component_info(const std::string& name) const;
    const ComponentTypeInfo* get_component_info(const std::type_index& type_index) const;
    
    std::vector<std::string> get_registered_components() const;
    
    // Automatic binding generation
    void generate_lua_bindings(LuaEngine* engine);
    void generate_python_bindings(PythonEngine* engine);
    
    // Educational features
    std::string generate_component_documentation() const;
    void explain_binding_process() const;

private:
    std::unordered_map<std::string, std::unique_ptr<ComponentTypeInfo>> components_by_name_;
    std::unordered_map<std::type_index, ComponentTypeInfo*> components_by_type_;
    
    ComponentRegistry() = default;
};

/**
 * @brief Automatic field reflection helper
 */
template<typename T>
class FieldReflector {
public:
    explicit FieldReflector(ComponentTypeInfo& component_info) 
        : component_info_(component_info) {}
    
    template<typename FieldType>
    FieldReflector& field(const std::string& field_name, FieldType T::* member_ptr);
    
    FieldReflector& constructor(std::function<T()> ctor);
    FieldReflector& constructor(std::function<T(const std::any&)> ctor);
    
    FieldReflector& to_string(std::function<std::string(const T&)> func);
    FieldReflector& from_string(std::function<T(const std::string&)> func);

private:
    ComponentTypeInfo& component_info_;
    
    template<typename FieldType>
    void setup_field_accessors(ComponentTypeInfo::FieldInfo& field_info, FieldType T::* member_ptr);
};

/**
 * @brief Type-specific binding generators
 */
class LuaBindingGenerator {
public:
    explicit LuaBindingGenerator(LuaEngine* engine) : engine_(engine) {}
    
    void bind_component_type(const ComponentTypeInfo& component_info);
    void bind_all_components();
    
    // Generate Lua wrapper functions
    void generate_component_constructor(const ComponentTypeInfo& component_info);
    void generate_component_accessors(const ComponentTypeInfo& component_info);
    void generate_component_utilities(const ComponentTypeInfo& component_info);

private:
    LuaEngine* engine_;
    
    // Lua C function generators
    static int lua_component_constructor(lua_State* L);
    static int lua_component_getter(lua_State* L);
    static int lua_component_setter(lua_State* L);
    static int lua_component_to_string(lua_State* L);
    
    void register_component_metatable(const ComponentTypeInfo& component_info);
    void create_component_methods(lua_State* L, const ComponentTypeInfo& component_info);
};

class PythonBindingGenerator {
public:
    explicit PythonBindingGenerator(PythonEngine* engine) : engine_(engine) {}
    
    void bind_component_type(const ComponentTypeInfo& component_info);
    void bind_all_components();
    
    // Generate Python wrapper classes
    void generate_component_class(const ComponentTypeInfo& component_info);
    void generate_component_methods(const ComponentTypeInfo& component_info);

private:
    PythonEngine* engine_;
    
    // Python C API function generators
    static PyObject* python_component_new(PyTypeObject* type, PyObject* args, PyObject* kwds);
    static void python_component_dealloc(PyObject* obj);
    static PyObject* python_component_getattro(PyObject* obj, PyObject* name);
    static int python_component_setattro(PyObject* obj, PyObject* name, PyObject* value);
    static PyObject* python_component_str(PyObject* obj);
    
    PyTypeObject* create_component_type(const ComponentTypeInfo& component_info);
    void register_component_methods(PyTypeObject* type, const ComponentTypeInfo& component_info);
};

/**
 * @brief Educational binding analyzer
 */
class BindingAnalyzer {
public:
    static void analyze_component_suitability(const ComponentTypeInfo& component_info);
    static std::string explain_performance_implications(const ComponentTypeInfo& component_info);
    static std::vector<std::string> suggest_optimizations(const ComponentTypeInfo& component_info);
    static void compare_binding_approaches(const ComponentTypeInfo& component_info);
    
private:
    static std::string analyze_field_complexity(const ComponentTypeInfo::FieldInfo& field);
    static f64 estimate_binding_overhead(const ComponentTypeInfo& component_info);
    static std::string suggest_caching_strategy(const ComponentTypeInfo& component_info);
};

/**
 * @brief Comprehensive scripting integration manager
 */
class ScriptIntegrationManager {
public:
    static ScriptIntegrationManager& instance();
    
    // Engine management
    void initialize_all_engines();
    void shutdown_all_engines();
    
    void register_lua_engine(std::unique_ptr<LuaEngine> engine);
    void register_python_engine(std::unique_ptr<PythonEngine> engine);
    
    LuaEngine* get_lua_engine() { return lua_engine_.get(); }
    PythonEngine* get_python_engine() { return python_engine_.get(); }
    
    // ECS integration
    void bind_ecs_registry(ecs::Registry* registry);
    
    // Automatic binding generation
    void generate_all_bindings();
    void regenerate_bindings_for_component(const std::string& component_name);
    
    // Hot-reload management
    void enable_hot_reload(const HotReloadConfig& config);
    void disable_hot_reload();
    
    // Educational features
    void create_all_tutorial_scripts();
    void run_performance_comparisons();
    std::string generate_integration_report() const;
    
    // Script execution
    template<typename... Args>
    ScriptResult<void> execute_in_all_engines(const std::string& script_name, Args&&... args);
    
    // Performance monitoring
    void benchmark_script_performance(const std::string& script_name, usize iterations = 1000);
    std::string compare_engine_performance() const;

private:
    std::unique_ptr<LuaEngine> lua_engine_;
    std::unique_ptr<PythonEngine> python_engine_;
    ecs::Registry* bound_registry_{nullptr};
    
    std::unique_ptr<LuaBindingGenerator> lua_binding_generator_;
    std::unique_ptr<PythonBindingGenerator> python_binding_generator_;
    
    ScriptIntegrationManager() = default;
    
    void setup_common_bindings();
    void register_built_in_components();
};

// Template implementations

template<typename Component>
void ComponentRegistry::register_component(const std::string& name) {
    static_assert(ecs::Component<Component>, "Type must be a valid ECS component");
    
    auto component_info = std::make_unique<ComponentTypeInfo>(name, typeid(Component));
    component_info->size = sizeof(Component);
    component_info->alignment = alignof(Component);
    
    // Setup basic constructor/destructor
    component_info->constructor = [](const std::any& args) -> void* {
        return new Component();
    };
    
    component_info->destructor = [](void* ptr) {
        delete static_cast<Component*>(ptr);
    };
    
    // Setup basic serialization
    component_info->to_string = [](const void* ptr) -> std::string {
        const Component* component = static_cast<const Component*>(ptr);
        return "Component: " + std::string(typeid(Component).name());
    };
    
    ComponentTypeInfo* info_ptr = component_info.get();
    components_by_name_[name] = std::move(component_info);
    components_by_type_[std::type_index(typeid(Component))] = info_ptr;
    
    LOG_INFO("Registered component type: {} ({})", name, typeid(Component).name());
}

template<typename Component>
void ComponentRegistry::register_component_auto() {
    std::string name = typeid(Component).name();
    // Remove namespace prefixes for cleaner names
    size_t last_colon = name.find_last_of(':');
    if (last_colon != std::string::npos) {
        name = name.substr(last_colon + 1);
    }
    
    register_component<Component>(name);
}

template<typename T>
template<typename FieldType>
FieldReflector<T>& FieldReflector<T>::field(const std::string& field_name, FieldType T::* member_ptr) {
    ComponentTypeInfo::FieldInfo field_info;
    field_info.name = field_name;
    field_info.type_name = typeid(FieldType).name();
    field_info.offset = offsetof(T, *member_ptr);
    field_info.size = sizeof(FieldType);
    
    setup_field_accessors(field_info, member_ptr);
    
    component_info_.fields.push_back(std::move(field_info));
    return *this;
}

template<typename T>
FieldReflector<T>& FieldReflector<T>::constructor(std::function<T()> ctor) {
    component_info_.constructor = [ctor](const std::any&) -> void* {
        return new T(ctor());
    };
    return *this;
}

template<typename T>
FieldReflector<T>& FieldReflector<T>::constructor(std::function<T(const std::any&)> ctor) {
    component_info_.constructor = [ctor](const std::any& args) -> void* {
        return new T(ctor(args));
    };
    return *this;
}

template<typename T>
FieldReflector<T>& FieldReflector<T>::to_string(std::function<std::string(const T&)> func) {
    component_info_.to_string = [func](const void* ptr) -> std::string {
        const T* component = static_cast<const T*>(ptr);
        return func(*component);
    };
    return *this;
}

template<typename T>
FieldReflector<T>& FieldReflector<T>::from_string(std::function<T(const std::string&)> func) {
    component_info_.from_string = [func](const std::string& str) -> std::any {
        return std::any(func(str));
    };
    return *this;
}

template<typename T>
template<typename FieldType>
void FieldReflector<T>::setup_field_accessors(ComponentTypeInfo::FieldInfo& field_info, FieldType T::* member_ptr) {
    // Generic getter
    field_info.getter = [member_ptr](const void* ptr) -> std::any {
        const T* component = static_cast<const T*>(ptr);
        return std::any(component->*member_ptr);
    };
    
    // Generic setter
    field_info.setter = [member_ptr](void* ptr, const std::any& value) {
        T* component = static_cast<T*>(ptr);
        component->*member_ptr = std::any_cast<FieldType>(value);
    };
    
    // Lua-specific accessors
    field_info.lua_push = [member_ptr](lua_State* L, const void* ptr) {
        const T* component = static_cast<const T*>(ptr);
        const FieldType& value = component->*member_ptr;
        LuaTypeHelper::push(L, value);
    };
    
    field_info.lua_get = [member_ptr](const void* ptr, lua_State* L, int index) {
        // This would need type-specific implementation
        // For now, placeholder
    };
    
    // Python-specific accessors
    field_info.python_to_object = [member_ptr](const void* ptr) -> PyObject* {
        const T* component = static_cast<const T*>(ptr);
        const FieldType& value = component->*member_ptr;
        return PythonTypeHelper::to_python(value);
    };
    
    field_info.python_from_object = [member_ptr](void* ptr, PyObject* obj) {
        T* component = static_cast<T*>(ptr);
        if constexpr (std::is_same_v<FieldType, bool>) {
            component->*member_ptr = PythonTypeHelper::from_python_bool(obj);
        } else if constexpr (std::is_same_v<FieldType, int>) {
            component->*member_ptr = PythonTypeHelper::from_python_int(obj);
        } else if constexpr (std::is_same_v<FieldType, float>) {
            component->*member_ptr = PythonTypeHelper::from_python_float(obj);
        } else if constexpr (std::is_same_v<FieldType, double>) {
            component->*member_ptr = PythonTypeHelper::from_python_double(obj);
        } else if constexpr (std::is_same_v<FieldType, std::string>) {
            component->*member_ptr = PythonTypeHelper::from_python_string(obj);
        }
        // Add more type conversions as needed
    };
}

template<typename... Args>
ScriptResult<void> ScriptIntegrationManager::execute_in_all_engines(const std::string& script_name, Args&&... args) {
    ScriptResult<void> lua_result{true};
    ScriptResult<void> python_result{true};
    
    if (lua_engine_) {
        lua_result = lua_engine_->execute_script(script_name);
    }
    
    if (python_engine_) {
        python_result = python_engine_->execute_script(script_name);
    }
    
    // Return combined result
    if (!lua_result && !python_result) {
        // Both failed - return Lua error as primary
        return lua_result;
    } else if (!lua_result) {
        return lua_result;
    } else if (!python_result) {
        return python_result;
    } else {
        return ScriptResult<void>::success_result();
    }
}

/**
 * @brief Helper macro for easy component registration with reflection
 */
#define REGISTER_COMPONENT_WITH_FIELDS(ComponentType, component_name) \
    do { \
        ecscope::scripting::ComponentRegistry::instance().register_component<ComponentType>(component_name); \
        auto* info = ecscope::scripting::ComponentRegistry::instance().get_component_info(component_name); \
        if (info) { \
            ecscope::scripting::FieldReflector<ComponentType> reflector(*const_cast<ecscope::scripting::ComponentTypeInfo*>(info));

#define REFLECT_FIELD(field_name, member_ptr) \
            reflector.field(field_name, member_ptr);

#define END_COMPONENT_REGISTRATION() \
        } \
    } while(0)

} // namespace ecscope::scripting