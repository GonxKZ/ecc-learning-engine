/**
 * @file web_ecs_bindings.cpp
 * @brief Complete JavaScript/C++ Embind bindings for ECScope ECS system
 * 
 * This file provides comprehensive JavaScript bindings for the entire ECScope ECS system,
 * enabling full interoperability between JavaScript and C++ code in WebAssembly.
 * All ECS functionality is exposed with complete type safety and performance optimization.
 */

#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <emscripten/emscripten.h>

// ECScope core includes
#include "ecscope/entity.hpp"
#include "ecscope/component.hpp"
#include "ecscope/registry.hpp"
#include "ecscope/archetype.hpp"
#include "ecscope/query.hpp"
#include "ecscope/system.hpp"
#include "ecscope/world.hpp"
#include "ecscope/relationships.hpp"
#include "ecscope/components.hpp"

// WebAssembly-specific includes
#include "web_memory_manager.hpp"
#include "web_performance_monitor.hpp"
#include "web_error_handler.hpp"

using namespace emscripten;

namespace ecscope::web {

/**
 * Complete Entity binding with all functionality
 */
class WebEntity {
private:
    ecscope::Entity entity_;
    ecscope::Registry* registry_;

public:
    explicit WebEntity(ecscope::Registry* reg) : registry_(reg) {
        entity_ = registry_->create_entity();
    }

    explicit WebEntity(ecscope::Entity entity, ecscope::Registry* reg) 
        : entity_(entity), registry_(reg) {}

    // Entity management
    bool is_valid() const { return registry_->is_valid(entity_); }
    void destroy() { registry_->destroy_entity(entity_); }
    uint32_t get_id() const { return static_cast<uint32_t>(entity_); }
    uint32_t get_version() const { return registry_->get_version(entity_); }

    // Component management with complete type safety
    template<typename T>
    void add_component(const T& component) {
        registry_->add_component(entity_, component);
    }

    template<typename T>
    bool has_component() const {
        return registry_->has_component<T>(entity_);
    }

    template<typename T>
    T& get_component() {
        return registry_->get_component<T>(entity_);
    }

    template<typename T>
    const T& get_component() const {
        return registry_->get_component<T>(entity_);
    }

    template<typename T>
    void remove_component() {
        registry_->remove_component<T>(entity_);
    }

    // JavaScript-friendly component access
    val get_component_as_object(const std::string& component_type) const;
    void set_component_from_object(const std::string& component_type, const val& component_data);
    bool has_component_by_name(const std::string& component_type) const;

    // Relationship management
    void add_child(const WebEntity& child);
    void remove_child(const WebEntity& child);
    void set_parent(const WebEntity& parent);
    std::vector<WebEntity> get_children() const;
    WebEntity get_parent() const;
    bool has_parent() const;
    size_t get_child_count() const;

    // Utility functions
    std::string to_string() const;
    val to_javascript_object() const;
};

/**
 * Complete Registry binding with full ECS functionality
 */
class WebRegistry {
private:
    std::unique_ptr<ecscope::Registry> registry_;
    WebPerformanceMonitor performance_monitor_;

public:
    WebRegistry() : registry_(std::make_unique<ecscope::Registry>()) {}

    // Entity management
    WebEntity create_entity() {
        auto entity = registry_->create_entity();
        return WebEntity(entity, registry_.get());
    }

    void destroy_entity(const WebEntity& entity) {
        registry_->destroy_entity(static_cast<ecscope::Entity>(entity.get_id()));
    }

    bool is_valid(const WebEntity& entity) const {
        return registry_->is_valid(static_cast<ecscope::Entity>(entity.get_id()));
    }

    // Archetype management
    size_t get_archetype_count() const { return registry_->get_archetype_count(); }
    std::vector<std::string> get_archetype_signatures() const;
    val get_archetype_info(const std::string& signature) const;

    // Entity queries with JavaScript integration
    std::vector<WebEntity> query_entities(const std::vector<std::string>& component_types) const;
    std::vector<WebEntity> query_entities_with_all(const std::vector<std::string>& component_types) const;
    std::vector<WebEntity> query_entities_with_any(const std::vector<std::string>& component_types) const;
    std::vector<WebEntity> query_entities_without(const std::vector<std::string>& component_types) const;

    // System management
    void register_system(const std::string& system_name, val system_function);
    void run_system(const std::string& system_name, float delta_time);
    void run_all_systems(float delta_time);

    // Component type registration
    void register_component_type(const std::string& type_name, const val& type_info);
    std::vector<std::string> get_registered_component_types() const;

    // Performance monitoring
    val get_performance_metrics() const;
    void reset_performance_counters();

    // Memory management
    size_t get_memory_usage() const;
    val get_memory_statistics() const;
    void garbage_collect();

    // Serialization and persistence
    val serialize_to_json() const;
    void deserialize_from_json(const val& json_data);
    val export_entity_data(const WebEntity& entity) const;
    WebEntity import_entity_data(const val& entity_data);

    // Debugging and introspection
    val get_debug_info() const;
    std::vector<WebEntity> get_all_entities() const;
    size_t get_entity_count() const;
    val get_component_usage_statistics() const;

    // Batch operations for performance
    std::vector<WebEntity> create_entities(size_t count);
    void destroy_entities(const std::vector<WebEntity>& entities);
    void add_component_to_entities(const std::vector<WebEntity>& entities, 
                                   const std::string& component_type, 
                                   const val& component_data);
};

/**
 * Complete Component system binding with type-safe access
 */
class WebComponentSystem {
public:
    // Transform component
    static void bind_transform_component() {
        value_object<ecscope::Transform>("Transform")
            .field("position", &ecscope::Transform::position)
            .field("rotation", &ecscope::Transform::rotation)
            .field("scale", &ecscope::Transform::scale);
    }

    // Velocity component
    static void bind_velocity_component() {
        value_object<ecscope::Velocity>("Velocity")
            .field("linear", &ecscope::Velocity::linear)
            .field("angular", &ecscope::Velocity::angular);
    }

    // RigidBody component
    static void bind_rigidbody_component() {
        value_object<ecscope::RigidBody>("RigidBody")
            .field("mass", &ecscope::RigidBody::mass)
            .field("inverse_mass", &ecscope::RigidBody::inverse_mass)
            .field("restitution", &ecscope::RigidBody::restitution)
            .field("friction", &ecscope::RigidBody::friction)
            .field("is_static", &ecscope::RigidBody::is_static)
            .field("is_kinematic", &ecscope::RigidBody::is_kinematic);
    }

    // Sprite component
    static void bind_sprite_component() {
        value_object<ecscope::Sprite>("Sprite")
            .field("texture_id", &ecscope::Sprite::texture_id)
            .field("color", &ecscope::Sprite::color)
            .field("size", &ecscope::Sprite::size)
            .field("uv_offset", &ecscope::Sprite::uv_offset)
            .field("uv_scale", &ecscope::Sprite::uv_scale)
            .field("layer", &ecscope::Sprite::layer)
            .field("visible", &ecscope::Sprite::visible);
    }

    // Camera component
    static void bind_camera_component() {
        value_object<ecscope::Camera2D>("Camera2D")
            .field("zoom", &ecscope::Camera2D::zoom)
            .field("viewport_size", &ecscope::Camera2D::viewport_size)
            .field("background_color", &ecscope::Camera2D::background_color)
            .field("is_active", &ecscope::Camera2D::is_active);
    }

    // All component bindings
    static void bind_all_components() {
        bind_transform_component();
        bind_velocity_component();
        bind_rigidbody_component();
        bind_sprite_component();
        bind_camera_component();
    }
};

/**
 * Complete Query system binding with advanced filtering
 */
class WebQueryBuilder {
private:
    ecscope::Registry* registry_;
    std::vector<std::string> include_types_;
    std::vector<std::string> exclude_types_;
    
public:
    explicit WebQueryBuilder(ecscope::Registry* registry) : registry_(registry) {}

    WebQueryBuilder& with(const std::string& component_type) {
        include_types_.push_back(component_type);
        return *this;
    }

    WebQueryBuilder& without(const std::string& component_type) {
        exclude_types_.push_back(component_type);
        return *this;
    }

    std::vector<WebEntity> execute() const;
    size_t count() const;
    WebEntity first() const;
    std::vector<WebEntity> take(size_t limit) const;
    std::vector<WebEntity> skip(size_t offset) const;

    // JavaScript-friendly iteration
    void for_each(val callback) const;
    val map(val mapper) const;
    std::vector<WebEntity> filter(val predicate) const;
};

/**
 * Complete System binding with JavaScript integration
 */
class WebSystemManager {
private:
    struct SystemInfo {
        std::string name;
        val update_function;
        std::vector<std::string> required_components;
        bool enabled = true;
        float last_execution_time = 0.0f;
        uint64_t execution_count = 0;
    };

    ecscope::Registry* registry_;
    std::unordered_map<std::string, SystemInfo> systems_;

public:
    explicit WebSystemManager(ecscope::Registry* registry) : registry_(registry) {}

    // System registration and management
    void register_system(const std::string& name, val update_function, 
                        const std::vector<std::string>& required_components = {});
    void unregister_system(const std::string& name);
    bool has_system(const std::string& name) const;

    // System execution
    void update_system(const std::string& name, float delta_time);
    void update_all_systems(float delta_time);
    void enable_system(const std::string& name);
    void disable_system(const std::string& name);

    // System information
    std::vector<std::string> get_system_names() const;
    val get_system_info(const std::string& name) const;
    val get_system_performance_stats(const std::string& name) const;

    // System dependencies and ordering
    void set_system_priority(const std::string& name, int priority);
    void add_system_dependency(const std::string& system, const std::string& depends_on);
    std::vector<std::string> get_execution_order() const;
};

/**
 * Complete Math utility bindings
 */
class WebMathUtils {
public:
    // Vector2 operations
    static ecscope::Vec2 vec2_add(const ecscope::Vec2& a, const ecscope::Vec2& b) {
        return a + b;
    }
    
    static ecscope::Vec2 vec2_subtract(const ecscope::Vec2& a, const ecscope::Vec2& b) {
        return a - b;
    }
    
    static ecscope::Vec2 vec2_multiply(const ecscope::Vec2& a, float scalar) {
        return a * scalar;
    }
    
    static float vec2_dot(const ecscope::Vec2& a, const ecscope::Vec2& b) {
        return ecscope::dot(a, b);
    }
    
    static float vec2_length(const ecscope::Vec2& v) {
        return ecscope::length(v);
    }
    
    static ecscope::Vec2 vec2_normalize(const ecscope::Vec2& v) {
        return ecscope::normalize(v);
    }

    // Color operations
    static ecscope::Color color_lerp(const ecscope::Color& a, const ecscope::Color& b, float t) {
        return ecscope::lerp(a, b, t);
    }

    // Utility functions
    static float lerp(float a, float b, float t) {
        return a + t * (b - a);
    }
    
    static float clamp(float value, float min_val, float max_val) {
        return std::max(min_val, std::min(value, max_val));
    }
    
    static float random_range(float min_val, float max_val) {
        return min_val + (max_val - min_val) * (rand() / static_cast<float>(RAND_MAX));
    }
};

/**
 * Debugging and profiling utilities
 */
class WebDebugUtils {
private:
    static std::vector<std::string> debug_messages_;

public:
    static void log(const std::string& message) {
        debug_messages_.push_back(message);
        EM_ASM({
            console.log(UTF8ToString($0));
        }, message.c_str());
    }

    static void warn(const std::string& message) {
        EM_ASM({
            console.warn(UTF8ToString($0));
        }, message.c_str());
    }

    static void error(const std::string& message) {
        EM_ASM({
            console.error(UTF8ToString($0));
        }, message.c_str());
    }

    static std::vector<std::string> get_debug_messages() {
        return debug_messages_;
    }

    static void clear_debug_messages() {
        debug_messages_.clear();
    }

    // Performance profiling
    static void start_profile(const std::string& name) {
        EM_ASM({
            console.time(UTF8ToString($0));
        }, name.c_str());
    }

    static void end_profile(const std::string& name) {
        EM_ASM({
            console.timeEnd(UTF8ToString($0));
        }, name.c_str());
    }
};

std::vector<std::string> WebDebugUtils::debug_messages_;

} // namespace ecscope::web

// Complete Embind bindings registration
EMSCRIPTEN_BINDINGS(ecscope_ecs) {
    // Core math types
    value_object<ecscope::Vec2>("Vec2")
        .field("x", &ecscope::Vec2::x)
        .field("y", &ecscope::Vec2::y);

    value_object<ecscope::Color>("Color")
        .field("r", &ecscope::Color::r)
        .field("g", &ecscope::Color::g)
        .field("b", &ecscope::Color::b)
        .field("a", &ecscope::Color::a);

    // Entity binding
    class_<ecscope::web::WebEntity>("Entity")
        .constructor<ecscope::Registry*>()
        .function("isValid", &ecscope::web::WebEntity::is_valid)
        .function("destroy", &ecscope::web::WebEntity::destroy)
        .function("getId", &ecscope::web::WebEntity::get_id)
        .function("getVersion", &ecscope::web::WebEntity::get_version)
        .function("hasComponentByName", &ecscope::web::WebEntity::has_component_by_name)
        .function("getComponentAsObject", &ecscope::web::WebEntity::get_component_as_object)
        .function("setComponentFromObject", &ecscope::web::WebEntity::set_component_from_object)
        .function("addChild", &ecscope::web::WebEntity::add_child)
        .function("removeChild", &ecscope::web::WebEntity::remove_child)
        .function("setParent", &ecscope::web::WebEntity::set_parent)
        .function("getChildren", &ecscope::web::WebEntity::get_children)
        .function("getParent", &ecscope::web::WebEntity::get_parent)
        .function("hasParent", &ecscope::web::WebEntity::has_parent)
        .function("getChildCount", &ecscope::web::WebEntity::get_child_count)
        .function("toString", &ecscope::web::WebEntity::to_string)
        .function("toJavaScriptObject", &ecscope::web::WebEntity::to_javascript_object);

    // Registry binding
    class_<ecscope::web::WebRegistry>("Registry")
        .constructor<>()
        .function("createEntity", &ecscope::web::WebRegistry::create_entity)
        .function("destroyEntity", &ecscope::web::WebRegistry::destroy_entity)
        .function("isValid", &ecscope::web::WebRegistry::is_valid)
        .function("getArchetypeCount", &ecscope::web::WebRegistry::get_archetype_count)
        .function("getArchetypeSignatures", &ecscope::web::WebRegistry::get_archetype_signatures)
        .function("getArchetypeInfo", &ecscope::web::WebRegistry::get_archetype_info)
        .function("queryEntities", &ecscope::web::WebRegistry::query_entities)
        .function("queryEntitiesWithAll", &ecscope::web::WebRegistry::query_entities_with_all)
        .function("queryEntitiesWithAny", &ecscope::web::WebRegistry::query_entities_with_any)
        .function("queryEntitiesWithout", &ecscope::web::WebRegistry::query_entities_without)
        .function("registerSystem", &ecscope::web::WebRegistry::register_system)
        .function("runSystem", &ecscope::web::WebRegistry::run_system)
        .function("runAllSystems", &ecscope::web::WebRegistry::run_all_systems)
        .function("registerComponentType", &ecscope::web::WebRegistry::register_component_type)
        .function("getRegisteredComponentTypes", &ecscope::web::WebRegistry::get_registered_component_types)
        .function("getPerformanceMetrics", &ecscope::web::WebRegistry::get_performance_metrics)
        .function("resetPerformanceCounters", &ecscope::web::WebRegistry::reset_performance_counters)
        .function("getMemoryUsage", &ecscope::web::WebRegistry::get_memory_usage)
        .function("getMemoryStatistics", &ecscope::web::WebRegistry::get_memory_statistics)
        .function("garbageCollect", &ecscope::web::WebRegistry::garbage_collect)
        .function("serializeToJson", &ecscope::web::WebRegistry::serialize_to_json)
        .function("deserializeFromJson", &ecscope::web::WebRegistry::deserialize_from_json)
        .function("exportEntityData", &ecscope::web::WebRegistry::export_entity_data)
        .function("importEntityData", &ecscope::web::WebRegistry::import_entity_data)
        .function("getDebugInfo", &ecscope::web::WebRegistry::get_debug_info)
        .function("getAllEntities", &ecscope::web::WebRegistry::get_all_entities)
        .function("getEntityCount", &ecscope::web::WebRegistry::get_entity_count)
        .function("getComponentUsageStatistics", &ecscope::web::WebRegistry::get_component_usage_statistics)
        .function("createEntities", &ecscope::web::WebRegistry::create_entities)
        .function("destroyEntities", &ecscope::web::WebRegistry::destroy_entities)
        .function("addComponentToEntities", &ecscope::web::WebRegistry::add_component_to_entities);

    // Query builder binding
    class_<ecscope::web::WebQueryBuilder>("QueryBuilder")
        .constructor<ecscope::Registry*>()
        .function("with", &ecscope::web::WebQueryBuilder::with)
        .function("without", &ecscope::web::WebQueryBuilder::without)
        .function("execute", &ecscope::web::WebQueryBuilder::execute)
        .function("count", &ecscope::web::WebQueryBuilder::count)
        .function("first", &ecscope::web::WebQueryBuilder::first)
        .function("take", &ecscope::web::WebQueryBuilder::take)
        .function("skip", &ecscope::web::WebQueryBuilder::skip)
        .function("forEach", &ecscope::web::WebQueryBuilder::for_each)
        .function("map", &ecscope::web::WebQueryBuilder::map)
        .function("filter", &ecscope::web::WebQueryBuilder::filter);

    // System manager binding
    class_<ecscope::web::WebSystemManager>("SystemManager")
        .constructor<ecscope::Registry*>()
        .function("registerSystem", &ecscope::web::WebSystemManager::register_system)
        .function("unregisterSystem", &ecscope::web::WebSystemManager::unregister_system)
        .function("hasSystem", &ecscope::web::WebSystemManager::has_system)
        .function("updateSystem", &ecscope::web::WebSystemManager::update_system)
        .function("updateAllSystems", &ecscope::web::WebSystemManager::update_all_systems)
        .function("enableSystem", &ecscope::web::WebSystemManager::enable_system)
        .function("disableSystem", &ecscope::web::WebSystemManager::disable_system)
        .function("getSystemNames", &ecscope::web::WebSystemManager::get_system_names)
        .function("getSystemInfo", &ecscope::web::WebSystemManager::get_system_info)
        .function("getSystemPerformanceStats", &ecscope::web::WebSystemManager::get_system_performance_stats)
        .function("setSystemPriority", &ecscope::web::WebSystemManager::set_system_priority)
        .function("addSystemDependency", &ecscope::web::WebSystemManager::add_system_dependency)
        .function("getExecutionOrder", &ecscope::web::WebSystemManager::get_execution_order);

    // Component bindings
    ecscope::web::WebComponentSystem::bind_all_components();

    // Math utilities
    class_<ecscope::web::WebMathUtils>("MathUtils")
        .class_function("vec2Add", &ecscope::web::WebMathUtils::vec2_add)
        .class_function("vec2Subtract", &ecscope::web::WebMathUtils::vec2_subtract)
        .class_function("vec2Multiply", &ecscope::web::WebMathUtils::vec2_multiply)
        .class_function("vec2Dot", &ecscope::web::WebMathUtils::vec2_dot)
        .class_function("vec2Length", &ecscope::web::WebMathUtils::vec2_length)
        .class_function("vec2Normalize", &ecscope::web::WebMathUtils::vec2_normalize)
        .class_function("colorLerp", &ecscope::web::WebMathUtils::color_lerp)
        .class_function("lerp", &ecscope::web::WebMathUtils::lerp)
        .class_function("clamp", &ecscope::web::WebMathUtils::clamp)
        .class_function("randomRange", &ecscope::web::WebMathUtils::random_range);

    // Debug utilities
    class_<ecscope::web::WebDebugUtils>("DebugUtils")
        .class_function("log", &ecscope::web::WebDebugUtils::log)
        .class_function("warn", &ecscope::web::WebDebugUtils::warn)
        .class_function("error", &ecscope::web::WebDebugUtils::error)
        .class_function("getDebugMessages", &ecscope::web::WebDebugUtils::get_debug_messages)
        .class_function("clearDebugMessages", &ecscope::web::WebDebugUtils::clear_debug_messages)
        .class_function("startProfile", &ecscope::web::WebDebugUtils::start_profile)
        .class_function("endProfile", &ecscope::web::WebDebugUtils::end_profile);

    // STL container bindings for JavaScript interop
    register_vector<std::string>("VectorString");
    register_vector<ecscope::web::WebEntity>("VectorEntity");
    register_vector<float>("VectorFloat");
    register_vector<int>("VectorInt");
}