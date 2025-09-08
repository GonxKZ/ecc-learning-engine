#pragma once

#include "reflection.hpp"
#include "properties.hpp"
#include "validation.hpp"
#include "serialization.hpp"
#include "../foundation/concepts.hpp"
#include "../core/types.hpp"
#include "../core/memory.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <optional>
#include <type_traits>
#include <typeindex>
#include <variant>
#include <any>
#include <mutex>
#include <shared_mutex>

/**
 * @file factory.hpp
 * @brief Component factory system with blueprints and templates
 * 
 * This file implements a comprehensive factory system that provides:
 * - Component blueprints for reusable configurations
 * - Template-based component creation
 * - Factory registration and management
 * - Dynamic component construction from metadata
 * - Blueprint inheritance and composition
 * - Component pools for performance optimization
 * - Parameterized component creation
 * - Runtime factory discovery and introspection
 * 
 * Key Features:
 * - Zero-allocation factory operations when possible
 * - Type-safe component construction
 * - Blueprint serialization and persistence
 * - Factory plugin system for extensibility
 * - Memory pool integration for performance
 * - Validation integration during construction
 * - Thread-safe factory operations
 */

namespace ecscope::components {

/// @brief Component blueprint for reusable configurations
class ComponentBlueprint {
public:
    ComponentBlueprint(std::string name, std::type_index type_index)
        : name_(std::move(name)), type_index_(type_index) {}
    
    /// @brief Get blueprint name
    const std::string& name() const noexcept { return name_; }
    
    /// @brief Get component type
    std::type_index type_index() const noexcept { return type_index_; }
    
    /// @brief Set property value
    ComponentBlueprint& set_property(const std::string& property_name, PropertyValue value) {
        property_values_[property_name] = std::move(value);
        return *this;
    }
    
    /// @brief Get property value
    const PropertyValue* get_property(const std::string& property_name) const {
        auto it = property_values_.find(property_name);
        return it != property_values_.end() ? &it->second : nullptr;
    }
    
    /// @brief Get all property values
    const std::unordered_map<std::string, PropertyValue>& property_values() const noexcept {
        return property_values_;
    }
    
    /// @brief Set description
    ComponentBlueprint& set_description(std::string description) {
        description_ = std::move(description);
        return *this;
    }
    
    /// @brief Get description
    const std::string& description() const noexcept { return description_; }
    
    /// @brief Set category
    ComponentBlueprint& set_category(std::string category) {
        category_ = std::move(category);
        return *this;
    }
    
    /// @brief Get category
    const std::string& category() const noexcept { return category_; }
    
    /// @brief Add tag
    ComponentBlueprint& add_tag(std::string tag) {
        tags_.emplace(std::move(tag));
        return *this;
    }
    
    /// @brief Check if has tag
    bool has_tag(const std::string& tag) const {
        return tags_.contains(tag);
    }
    
    /// @brief Get all tags
    const std::unordered_set<std::string>& tags() const noexcept { return tags_; }
    
    /// @brief Set parent blueprint for inheritance
    ComponentBlueprint& set_parent(std::shared_ptr<ComponentBlueprint> parent) {
        parent_ = std::move(parent);
        return *this;
    }
    
    /// @brief Get parent blueprint
    std::shared_ptr<ComponentBlueprint> parent() const noexcept { return parent_; }
    
    /// @brief Get effective property value (including inheritance)
    PropertyValue get_effective_property(const std::string& property_name) const {
        // Check local properties first
        auto it = property_values_.find(property_name);
        if (it != property_values_.end()) {
            return it->second;
        }
        
        // Check parent blueprint
        if (parent_) {
            return parent_->get_effective_property(property_name);
        }
        
        return PropertyValue{};
    }
    
    /// @brief Get all effective properties (including inherited)
    std::unordered_map<std::string, PropertyValue> get_effective_properties() const {
        std::unordered_map<std::string, PropertyValue> result;
        
        // Start with parent properties
        if (parent_) {
            result = parent_->get_effective_properties();
        }
        
        // Override with local properties
        for (const auto& [name, value] : property_values_) {
            result[name] = value;
        }
        
        return result;
    }
    
    /// @brief Add parameter for blueprint customization
    ComponentBlueprint& add_parameter(const std::string& param_name, PropertyValue default_value, std::string description = "") {
        parameters_[param_name] = BlueprintParameter{std::move(default_value), std::move(description)};
        return *this;
    }
    
    /// @brief Get parameter default value
    const PropertyValue* get_parameter_default(const std::string& param_name) const {
        auto it = parameters_.find(param_name);
        return it != parameters_.end() ? &it->second.default_value : nullptr;
    }
    
    /// @brief Get all parameters
    const std::unordered_map<std::string, BlueprintParameter>& parameters() const noexcept {
        return parameters_;
    }
    
    /// @brief Clone blueprint
    std::shared_ptr<ComponentBlueprint> clone(const std::string& new_name) const {
        auto cloned = std::make_shared<ComponentBlueprint>(new_name, type_index_);
        cloned->property_values_ = property_values_;
        cloned->description_ = description_;
        cloned->category_ = category_;
        cloned->tags_ = tags_;
        cloned->parent_ = parent_;
        cloned->parameters_ = parameters_;
        return cloned;
    }

private:
    struct BlueprintParameter {
        PropertyValue default_value;
        std::string description;
    };
    
    std::string name_;
    std::type_index type_index_;
    std::string description_;
    std::string category_;
    std::unordered_set<std::string> tags_;
    std::unordered_map<std::string, PropertyValue> property_values_;
    std::shared_ptr<ComponentBlueprint> parent_;
    std::unordered_map<std::string, BlueprintParameter> parameters_;
};

/// @brief Factory result information
struct FactoryResult {
    bool success{true};
    std::string error_message{};
    std::vector<ValidationMessage> validation_messages{};
    
    explicit operator bool() const noexcept { return success; }
    
    static FactoryResult success_result() {
        return FactoryResult{true, {}, {}};
    }
    
    static FactoryResult error_result(std::string error) {
        return FactoryResult{false, std::move(error), {}};
    }
    
    FactoryResult& add_validation_messages(const std::vector<ValidationMessage>& messages) {
        validation_messages.insert(validation_messages.end(), messages.begin(), messages.end());
        return *this;
    }
};

/// @brief Abstract component factory interface
class ComponentFactory {
public:
    virtual ~ComponentFactory() = default;
    
    /// @brief Create component instance
    virtual void* create_component() const = 0;
    
    /// @brief Create component with blueprint
    virtual FactoryResult create_component_with_blueprint(void* memory, const ComponentBlueprint& blueprint) const = 0;
    
    /// @brief Create component with parameters
    virtual FactoryResult create_component_with_params(void* memory, const std::unordered_map<std::string, PropertyValue>& params) const = 0;
    
    /// @brief Destroy component instance
    virtual void destroy_component(void* component) const = 0;
    
    /// @brief Get component type
    virtual std::type_index component_type() const = 0;
    
    /// @brief Get component size
    virtual std::size_t component_size() const = 0;
    
    /// @brief Get component alignment
    virtual std::size_t component_alignment() const = 0;
    
    /// @brief Check if factory supports blueprints
    virtual bool supports_blueprints() const { return true; }
    
    /// @brief Check if factory supports parameters
    virtual bool supports_parameters() const { return true; }
    
    /// @brief Get factory name
    virtual std::string name() const = 0;
    
    /// @brief Get factory description
    virtual std::string description() const = 0;
};

/// @brief Typed component factory implementation
template<Component T>
class TypedComponentFactory : public ComponentFactory {
public:
    TypedComponentFactory(std::string name = typeid(T).name(), std::string description = "")
        : name_(std::move(name)), description_(std::move(description)) {}
    
    void* create_component() const override {
        if constexpr (std::is_default_constructible_v<T>) {
            return new T{};
        } else {
            throw std::runtime_error("Component type " + name_ + " is not default constructible");
        }
    }
    
    FactoryResult create_component_with_blueprint(void* memory, const ComponentBlueprint& blueprint) const override {
        if (blueprint.type_index() != typeid(T)) {
            return FactoryResult::error_result("Blueprint type mismatch");
        }
        
        // Construct component
        T* component = nullptr;
        if (memory) {
            component = new(memory) T{};
        } else {
            component = new T{};
        }
        
        // Apply blueprint properties
        auto effective_properties = blueprint.get_effective_properties();
        auto& registry = ReflectionRegistry::instance();
        const auto* type_info = registry.get_type_info<T>();
        
        if (!type_info) {
            if (!memory) delete component;
            return FactoryResult::error_result("Type not registered in reflection system");
        }
        
        FactoryResult result = FactoryResult::success_result();
        
        for (const auto& [property_name, value] : effective_properties) {
            const auto* property = type_info->get_property(property_name);
            if (!property) {
                result.validation_messages.emplace_back(
                    ValidationSeverity::Warning, "PROPERTY_NOT_FOUND",
                    "Property '" + property_name + "' not found in type", property_name
                );
                continue;
            }
            
            // Validate property value
            auto& validation_manager = ValidationManager::instance();
            auto validation_result = validation_manager.validate_property<T>(property_name, value, ValidationContext::Creation);
            
            if (!validation_result) {
                result.add_validation_messages(validation_result.messages);
                continue;
            }
            
            // Set property value
            auto set_result = property->set_value(component, value);
            if (!set_result) {
                result.validation_messages.emplace_back(
                    ValidationSeverity::Error, "PROPERTY_SET_FAILED",
                    "Failed to set property '" + property_name + "': " + set_result.error_message,
                    property_name
                );
            }
        }
        
        // Validate entire component
        auto& validation_manager = ValidationManager::instance();
        auto component_validation = validation_manager.validate_component(*component, ValidationContext::Creation);
        result.add_validation_messages(component_validation.messages);
        
        if (!component_validation) {
            if (!memory) delete component;
            result.success = false;
            result.error_message = "Component validation failed";
        }
        
        return result;
    }
    
    FactoryResult create_component_with_params(void* memory, const std::unordered_map<std::string, PropertyValue>& params) const override {
        // Create temporary blueprint from parameters
        ComponentBlueprint temp_blueprint("temp", typeid(T));
        for (const auto& [name, value] : params) {
            temp_blueprint.set_property(name, value);
        }
        
        return create_component_with_blueprint(memory, temp_blueprint);
    }
    
    void destroy_component(void* component) const override {
        if (component) {
            static_cast<T*>(component)->~T();
        }
    }
    
    std::type_index component_type() const override { return typeid(T); }
    std::size_t component_size() const override { return sizeof(T); }
    std::size_t component_alignment() const override { return alignof(T); }
    std::string name() const override { return name_; }
    std::string description() const override { return description_; }

private:
    std::string name_;
    std::string description_;
};

/// @brief Component pool for performance optimization
template<Component T>
class ComponentPool {
public:
    explicit ComponentPool(std::size_t initial_capacity = 64)
        : pool_memory_(initial_capacity * sizeof(T)) {
        
        // Initialize free list
        char* memory_ptr = pool_memory_.data();
        for (std::size_t i = 0; i < initial_capacity; ++i) {
            free_components_.push(reinterpret_cast<T*>(memory_ptr + i * sizeof(T)));
        }
    }
    
    ~ComponentPool() {
        // Destroy any remaining constructed components
        for (T* component : constructed_components_) {
            component->~T();
        }
    }
    
    /// @brief Acquire component from pool
    T* acquire() {
        std::unique_lock lock(mutex_);
        
        if (free_components_.empty()) {
            expand_pool();
        }
        
        T* component = free_components_.top();
        free_components_.pop();
        
        // Construct component
        new(component) T{};
        constructed_components_.insert(component);
        
        return component;
    }
    
    /// @brief Release component back to pool
    void release(T* component) {
        if (!component) return;
        
        std::unique_lock lock(mutex_);
        
        auto it = constructed_components_.find(component);
        if (it != constructed_components_.end()) {
            component->~T();
            constructed_components_.erase(it);
            free_components_.push(component);
        }
    }
    
    /// @brief Get pool statistics
    struct PoolStats {
        std::size_t total_capacity{0};
        std::size_t active_components{0};
        std::size_t available_components{0};
    };
    
    PoolStats get_stats() const {
        std::shared_lock lock(mutex_);
        
        PoolStats stats;
        stats.total_capacity = pool_memory_.size() / sizeof(T);
        stats.active_components = constructed_components_.size();
        stats.available_components = free_components_.size();
        
        return stats;
    }

private:
    mutable std::shared_mutex mutex_;
    std::vector<char> pool_memory_;
    std::stack<T*> free_components_;
    std::unordered_set<T*> constructed_components_;
    
    void expand_pool() {
        // Double the pool size
        std::size_t old_capacity = pool_memory_.size() / sizeof(T);
        std::size_t new_capacity = old_capacity * 2;
        
        pool_memory_.resize(new_capacity * sizeof(T));
        
        // Add new components to free list
        char* memory_ptr = pool_memory_.data();
        for (std::size_t i = old_capacity; i < new_capacity; ++i) {
            free_components_.push(reinterpret_cast<T*>(memory_ptr + i * sizeof(T)));
        }
    }
};

/// @brief Factory registry for component creation management
class FactoryRegistry {
public:
    /// @brief Singleton access
    static FactoryRegistry& instance() {
        static FactoryRegistry registry;
        return registry;
    }
    
    /// @brief Register component factory
    template<Component T>
    void register_factory(std::unique_ptr<ComponentFactory> factory) {
        std::unique_lock lock(mutex_);
        const auto type_index = std::type_index(typeid(T));
        factories_[type_index] = std::move(factory);
    }
    
    /// @brief Register typed factory
    template<Component T>
    void register_typed_factory(const std::string& name = typeid(T).name(), const std::string& description = "") {
        register_factory<T>(std::make_unique<TypedComponentFactory<T>>(name, description));
    }
    
    /// @brief Get factory by type
    template<Component T>
    const ComponentFactory* get_factory() const {
        std::shared_lock lock(mutex_);
        
        const auto type_index = std::type_index(typeid(T));
        auto it = factories_.find(type_index);
        return it != factories_.end() ? it->second.get() : nullptr;
    }
    
    /// @brief Get factory by type index
    const ComponentFactory* get_factory(std::type_index type_index) const {
        std::shared_lock lock(mutex_);
        
        auto it = factories_.find(type_index);
        return it != factories_.end() ? it->second.get() : nullptr;
    }
    
    /// @brief Create component by type
    template<Component T>
    T* create_component() const {
        const auto* factory = get_factory<T>();
        if (!factory) {
            return nullptr;
        }
        
        return static_cast<T*>(factory->create_component());
    }
    
    /// @brief Create component with blueprint
    template<Component T>
    std::pair<T*, FactoryResult> create_component_with_blueprint(const ComponentBlueprint& blueprint) const {
        const auto* factory = get_factory<T>();
        if (!factory) {
            return {nullptr, FactoryResult::error_result("Factory not found for type")};
        }
        
        T* component = nullptr;
        if constexpr (std::is_default_constructible_v<T>) {
            component = new T{};
        } else {
            return {nullptr, FactoryResult::error_result("Type is not default constructible")};
        }
        
        auto result = factory->create_component_with_blueprint(component, blueprint);
        if (!result) {
            delete component;
            return {nullptr, result};
        }
        
        return {component, result};
    }
    
    /// @brief Register blueprint
    void register_blueprint(std::shared_ptr<ComponentBlueprint> blueprint) {
        std::unique_lock lock(mutex_);
        blueprints_[blueprint->name()] = std::move(blueprint);
    }
    
    /// @brief Get blueprint by name
    std::shared_ptr<ComponentBlueprint> get_blueprint(const std::string& name) const {
        std::shared_lock lock(mutex_);
        
        auto it = blueprints_.find(name);
        return it != blueprints_.end() ? it->second : nullptr;
    }
    
    /// @brief Get all blueprints
    std::vector<std::shared_ptr<ComponentBlueprint>> get_all_blueprints() const {
        std::shared_lock lock(mutex_);
        
        std::vector<std::shared_ptr<ComponentBlueprint>> result;
        result.reserve(blueprints_.size());
        
        for (const auto& [name, blueprint] : blueprints_) {
            result.push_back(blueprint);
        }
        
        return result;
    }
    
    /// @brief Get blueprints by category
    std::vector<std::shared_ptr<ComponentBlueprint>> get_blueprints_by_category(const std::string& category) const {
        std::shared_lock lock(mutex_);
        
        std::vector<std::shared_ptr<ComponentBlueprint>> result;
        for (const auto& [name, blueprint] : blueprints_) {
            if (blueprint->category() == category) {
                result.push_back(blueprint);
            }
        }
        
        return result;
    }
    
    /// @brief Get blueprints with tag
    std::vector<std::shared_ptr<ComponentBlueprint>> get_blueprints_with_tag(const std::string& tag) const {
        std::shared_lock lock(mutex_);
        
        std::vector<std::shared_ptr<ComponentBlueprint>> result;
        for (const auto& [name, blueprint] : blueprints_) {
            if (blueprint->has_tag(tag)) {
                result.push_back(blueprint);
            }
        }
        
        return result;
    }
    
    /// @brief Get all registered factory types
    std::vector<std::type_index> get_registered_types() const {
        std::shared_lock lock(mutex_);
        
        std::vector<std::type_index> result;
        result.reserve(factories_.size());
        
        for (const auto& [type_index, factory] : factories_) {
            result.push_back(type_index);
        }
        
        return result;
    }
    
    /// @brief Get factory count
    std::size_t factory_count() const {
        std::shared_lock lock(mutex_);
        return factories_.size();
    }
    
    /// @brief Get blueprint count
    std::size_t blueprint_count() const {
        std::shared_lock lock(mutex_);
        return blueprints_.size();
    }
    
    /// @brief Clear all factories and blueprints
    void clear() {
        std::unique_lock lock(mutex_);
        factories_.clear();
        blueprints_.clear();
    }

private:
    FactoryRegistry() = default;
    
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::type_index, std::unique_ptr<ComponentFactory>> factories_;
    std::unordered_map<std::string, std::shared_ptr<ComponentBlueprint>> blueprints_;
};

/// @brief Blueprint builder for fluent API
template<Component T>
class BlueprintBuilder {
public:
    BlueprintBuilder(const std::string& name) {
        blueprint_ = std::make_shared<ComponentBlueprint>(name, typeid(T));
    }
    
    /// @brief Set description
    BlueprintBuilder& description(const std::string& desc) {
        if (blueprint_) blueprint_->set_description(desc);
        return *this;
    }
    
    /// @brief Set category
    BlueprintBuilder& category(const std::string& cat) {
        if (blueprint_) blueprint_->set_category(cat);
        return *this;
    }
    
    /// @brief Add tag
    BlueprintBuilder& tag(const std::string& tag_name) {
        if (blueprint_) blueprint_->add_tag(tag_name);
        return *this;
    }
    
    /// @brief Set property value
    template<typename ValueType>
    BlueprintBuilder& property(const std::string& property_name, ValueType&& value) {
        if (blueprint_) {
            blueprint_->set_property(property_name, PropertyValue(std::forward<ValueType>(value)));
        }
        return *this;
    }
    
    /// @brief Set parent blueprint
    BlueprintBuilder& inherits(std::shared_ptr<ComponentBlueprint> parent) {
        if (blueprint_) blueprint_->set_parent(std::move(parent));
        return *this;
    }
    
    /// @brief Add parameter
    template<typename ValueType>
    BlueprintBuilder& parameter(const std::string& param_name, ValueType&& default_value, const std::string& description = "") {
        if (blueprint_) {
            blueprint_->add_parameter(param_name, PropertyValue(std::forward<ValueType>(default_value)), description);
        }
        return *this;
    }
    
    /// @brief Register the blueprint
    std::shared_ptr<ComponentBlueprint> register_blueprint() {
        if (blueprint_) {
            auto& registry = FactoryRegistry::instance();
            registry.register_blueprint(blueprint_);
        }
        return blueprint_;
    }
    
    /// @brief Get the blueprint without registering
    std::shared_ptr<ComponentBlueprint> build() {
        return blueprint_;
    }

private:
    std::shared_ptr<ComponentBlueprint> blueprint_;
};

/// @brief Helper function to create blueprint builder
template<Component T>
BlueprintBuilder<T> blueprint(const std::string& name) {
    return BlueprintBuilder<T>(name);
}

/// @brief Component creation helper functions
namespace factory {

/// @brief Create component with factory
template<Component T>
T* create() {
    return FactoryRegistry::instance().create_component<T>();
}

/// @brief Create component with blueprint
template<Component T>
std::pair<T*, FactoryResult> create_with_blueprint(const std::string& blueprint_name) {
    auto& registry = FactoryRegistry::instance();
    auto blueprint = registry.get_blueprint(blueprint_name);
    if (!blueprint) {
        return {nullptr, FactoryResult::error_result("Blueprint not found: " + blueprint_name)};
    }
    
    return registry.create_component_with_blueprint<T>(*blueprint);
}

/// @brief Create component with parameters
template<Component T>
std::pair<T*, FactoryResult> create_with_params(const std::unordered_map<std::string, PropertyValue>& params) {
    const auto* factory = FactoryRegistry::instance().get_factory<T>();
    if (!factory) {
        return {nullptr, FactoryResult::error_result("Factory not found for type")};
    }
    
    T* component = nullptr;
    if constexpr (std::is_default_constructible_v<T>) {
        component = new T{};
    } else {
        return {nullptr, FactoryResult::error_result("Type is not default constructible")};
    }
    
    auto result = factory->create_component_with_params(component, params);
    if (!result) {
        delete component;
        return {nullptr, result};
    }
    
    return {component, result};
}

/// @brief Destroy component
template<Component T>
void destroy(T* component) {
    const auto* factory = FactoryRegistry::instance().get_factory<T>();
    if (factory) {
        factory->destroy_component(component);
    } else {
        delete component;
    }
}

}  // namespace factory

/// @brief Convenience macros for factory registration
#define ECSCOPE_REGISTER_FACTORY(Type) \
    namespace { \
        struct Type##_FactoryRegistrar { \
            Type##_FactoryRegistrar() { \
                ::ecscope::components::FactoryRegistry::instance().register_typed_factory<Type>(#Type); \
            } \
        }; \
        static Type##_FactoryRegistrar Type##_factory_registrar_instance; \
    }

#define ECSCOPE_REGISTER_BLUEPRINT(Type, Name, ...) \
    namespace { \
        struct Type##_BlueprintRegistrar { \
            Type##_BlueprintRegistrar() { \
                register_blueprint(); \
            } \
            void register_blueprint(); \
        }; \
        static Type##_BlueprintRegistrar Type##_blueprint_registrar_instance; \
    } \
    void Type##_BlueprintRegistrar::register_blueprint()

}  // namespace ecscope::components