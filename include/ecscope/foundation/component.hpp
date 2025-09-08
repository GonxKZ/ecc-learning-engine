#pragma once

#include "../core/types.hpp"
#include "../core/memory.hpp"
#include "concepts.hpp"
#include <unordered_map>
#include <vector>
#include <string_view>
#include <typeinfo>
#include <memory>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <functional>

/**
 * @file component.hpp
 * @brief Type-safe component framework with reflection
 * 
 * This file implements a comprehensive component system with:
 * - Type-safe component registration and management
 * - Runtime reflection for component types
 * - Component serialization support
 * - Memory-efficient component type tracking
 * - Component dependency management
 * - Component lifecycle hooks
 * 
 * Educational Notes:
 * - Component types are registered at compile-time but tracked at runtime
 * - Each component type gets a unique ID for fast lookups
 * - Type erasure allows storing different component types uniformly
 * - Reflection enables debugging, serialization, and tooling
 * - Component signatures use bitsets for fast set operations
 * - RTTI information is cached to avoid repeated std::type_info calls
 */

namespace ecscope::foundation {

using namespace ecscope::core;

/// @brief Component registry for type management and reflection
class ComponentRegistry {
public:
    /// @brief Component lifecycle callbacks
    struct LifecycleCallbacks {
        std::function<void(void*)> on_construct{};    ///< Called after component construction
        std::function<void(void*)> on_destruct{};     ///< Called before component destruction
        std::function<void(void*, const void*)> on_copy{};     ///< Called during component copy
        std::function<void(void*, void*)> on_move{};           ///< Called during component move
    };
    
    /// @brief Enhanced component type information with reflection
    struct ComponentTypeDesc {
        ComponentTypeInfo type_info{};
        LifecycleCallbacks callbacks{};
        
        // Reflection data
        std::string_view name{};
        std::size_t type_hash{};
        const std::type_info* type_info_ptr{nullptr};
        
        // Serialization support
        std::function<std::size_t(const void*)> serialized_size_func{};
        std::function<std::size_t(const void*, std::span<std::byte>)> serialize_func{};
        std::function<void(void*, std::span<const std::byte>)> deserialize_func{};
        
        // Debug support
        std::function<std::string(const void*)> to_string_func{};
        std::function<bool(const void*, const void*)> equals_func{};
        
        // Component dependencies
        std::vector<ComponentId> required_components{};
        std::vector<ComponentId> incompatible_components{};
        
        constexpr ComponentTypeDesc() = default;
        
        template<Component T>
        static ComponentTypeDesc create(ComponentId id, std::string_view type_name) {
            ComponentTypeDesc desc{};
            desc.type_info = ComponentTypeInfo::create<T>(id, type_name.data());
            desc.name = type_name;
            desc.type_hash = std::hash<std::string_view>{}(type_name);
            desc.type_info_ptr = &typeid(T);
            
            // Setup serialization if supported
            if constexpr (Serializable<T>) {
                desc.serialized_size_func = [](const void* ptr) -> std::size_t {
                    return static_cast<const T*>(ptr)->serialized_size();
                };
                
                desc.serialize_func = [](const void* ptr, std::span<std::byte> buffer) -> std::size_t {
                    return static_cast<const T*>(ptr)->serialize(buffer);
                };
                
                desc.deserialize_func = [](void* ptr, std::span<const std::byte> buffer) {
                    static_cast<T*>(ptr)->deserialize(buffer);
                };
            }
            
            // Setup debug support
            desc.to_string_func = [](const void* ptr) -> std::string {
                if constexpr (requires(const T& t) { t.to_string(); }) {
                    return static_cast<const T*>(ptr)->to_string();
                } else if constexpr (requires(const T& t) { std::to_string(t); }) {
                    return std::to_string(*static_cast<const T*>(ptr));
                } else {
                    return std::string(typeid(T).name());
                }
            };
            
            desc.equals_func = [](const void* a, const void* b) -> bool {
                if constexpr (std::equality_comparable<T>) {
                    return *static_cast<const T*>(a) == *static_cast<const T*>(b);
                } else {
                    return false;
                }
            };
            
            return desc;
        }
        
        bool has_serialization_support() const noexcept {
            return serialize_func && deserialize_func && serialized_size_func;
        }
        
        bool has_debug_support() const noexcept {
            return to_string_func && equals_func;
        }
    };
    
    /// @brief Singleton access
    static ComponentRegistry& instance() {
        static ComponentRegistry registry;
        return registry;
    }
    
    /// @brief Register a component type
    /// @tparam T Component type to register
    /// @param name Human-readable name for the component
    /// @return Component ID for the registered type
    template<Component T>
    ComponentId register_component(std::string_view name = typeid(T).name()) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        const auto type_hash = std::hash<std::string_view>{}(name);
        
        // Check if already registered
        auto it = type_hash_to_id_.find(type_hash);
        if (it != type_hash_to_id_.end()) {
            return it->second;
        }
        
        // Assign new ID
        const auto id = ComponentId{next_id_++};
        
        // Create type descriptor
        auto desc = ComponentTypeDesc::create<T>(id, name);
        
        // Store mappings
        components_[id] = std::move(desc);
        type_hash_to_id_[type_hash] = id;
        type_info_to_id_[&typeid(T)] = id;
        
        // Update signature tracking
        if (id.value >= component_signature_bits_) {
            throw std::runtime_error("Too many component types registered");
        }
        
        return id;
    }
    
    /// @brief Get component ID for a type
    /// @tparam T Component type
    /// @return Component ID, or invalid ID if not registered
    template<Component T>
    ComponentId get_component_id() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = type_info_to_id_.find(&typeid(T));
        return it != type_info_to_id_.end() ? it->second : ComponentId::invalid();
    }
    
    /// @brief Get component type descriptor
    /// @param id Component ID
    /// @return Component type descriptor, or nullptr if not found
    const ComponentTypeDesc* get_component_desc(ComponentId id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = components_.find(id);
        return it != components_.end() ? &it->second : nullptr;
    }
    
    /// @brief Find component ID by name
    /// @param name Component type name
    /// @return Component ID, or invalid ID if not found
    ComponentId find_component_by_name(std::string_view name) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        const auto type_hash = std::hash<std::string_view>{}(name);
        auto it = type_hash_to_id_.find(type_hash);
        return it != type_hash_to_id_.end() ? it->second : ComponentId::invalid();
    }
    
    /// @brief Get all registered component IDs
    /// @return Vector of all registered component IDs
    std::vector<ComponentId> get_all_component_ids() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::vector<ComponentId> ids;
        ids.reserve(components_.size());
        
        for (const auto& [id, _] : components_) {
            ids.push_back(id);
        }
        
        return ids;
    }
    
    /// @brief Get number of registered components
    /// @return Component count
    std::size_t component_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return components_.size();
    }
    
    /// @brief Set lifecycle callbacks for a component type
    /// @param id Component ID
    /// @param callbacks Lifecycle callback functions
    void set_lifecycle_callbacks(ComponentId id, LifecycleCallbacks callbacks) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = components_.find(id);
        if (it != components_.end()) {
            it->second.callbacks = std::move(callbacks);
        }
    }
    
    /// @brief Set component dependencies
    /// @param id Component ID
    /// @param required Required component IDs
    /// @param incompatible Incompatible component IDs
    void set_component_dependencies(ComponentId id, 
                                  std::vector<ComponentId> required,
                                  std::vector<ComponentId> incompatible = {}) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = components_.find(id);
        if (it != components_.end()) {
            it->second.required_components = std::move(required);
            it->second.incompatible_components = std::move(incompatible);
        }
    }
    
    /// @brief Check if component dependencies are satisfied
    /// @param id Component ID to check
    /// @param signature Current component signature
    /// @return true if dependencies are satisfied
    bool check_dependencies(ComponentId id, ComponentSignature signature) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = components_.find(id);
        if (it == components_.end()) {
            return false;
        }
        
        const auto& desc = it->second;
        
        // Check required components
        for (ComponentId required : desc.required_components) {
            if (!has_component_in_signature(signature, required)) {
                return false;
            }
        }
        
        // Check incompatible components
        for (ComponentId incompatible : desc.incompatible_components) {
            if (has_component_in_signature(signature, incompatible)) {
                return false;
            }
        }
        
        return true;
    }
    
    /// @brief Component signature utilities
    class SignatureBuilder {
    public:
        explicit SignatureBuilder(const ComponentRegistry& registry) : registry_(registry) {}
        
        /// @brief Add component to signature
        template<Component T>
        SignatureBuilder& with() {
            const auto id = registry_.get_component_id<T>();
            if (id.is_valid()) {
                signature_ |= (1ULL << id.value);
            }
            return *this;
        }
        
        /// @brief Add component by ID to signature
        SignatureBuilder& with(ComponentId id) {
            if (id.is_valid() && id.value < 64) {
                signature_ |= (1ULL << id.value);
            }
            return *this;
        }
        
        /// @brief Remove component from signature
        template<Component T>
        SignatureBuilder& without() {
            const auto id = registry_.get_component_id<T>();
            if (id.is_valid()) {
                signature_ &= ~(1ULL << id.value);
            }
            return *this;
        }
        
        /// @brief Remove component by ID from signature
        SignatureBuilder& without(ComponentId id) {
            if (id.is_valid() && id.value < 64) {
                signature_ &= ~(1ULL << id.value);
            }
            return *this;
        }
        
        /// @brief Build final signature
        ComponentSignature build() const noexcept { return signature_; }
        
        /// @brief Reset builder
        void reset() noexcept { signature_ = 0; }

    private:
        const ComponentRegistry& registry_;
        ComponentSignature signature_{0};
    };
    
    /// @brief Create signature builder
    SignatureBuilder create_signature_builder() const { return SignatureBuilder(*this); }
    
    /// @brief Component signature operations
    static bool has_component_in_signature(ComponentSignature signature, ComponentId id) noexcept {
        return id.is_valid() && id.value < 64 && (signature & (1ULL << id.value)) != 0;
    }
    
    static ComponentSignature add_component_to_signature(ComponentSignature signature, ComponentId id) noexcept {
        if (id.is_valid() && id.value < 64) {
            return signature | (1ULL << id.value);
        }
        return signature;
    }
    
    static ComponentSignature remove_component_from_signature(ComponentSignature signature, ComponentId id) noexcept {
        if (id.is_valid() && id.value < 64) {
            return signature & ~(1ULL << id.value);
        }
        return signature;
    }
    
    static bool signature_matches(ComponentSignature signature, ComponentSignature required, ComponentSignature excluded = 0) noexcept {
        return (signature & required) == required && (signature & excluded) == 0;
    }
    
    static std::uint32_t count_components_in_signature(ComponentSignature signature) noexcept {
        return std::popcount(signature);
    }

private:
    ComponentRegistry() = default;
    
    mutable std::mutex mutex_;
    
    /// @brief Component type storage
    std::unordered_map<ComponentId, ComponentTypeDesc> components_;
    
    /// @brief Type hash to ID mapping
    std::unordered_map<std::size_t, ComponentId> type_hash_to_id_;
    
    /// @brief Type info to ID mapping
    std::unordered_map<const std::type_info*, ComponentId> type_info_to_id_;
    
    /// @brief Next component ID to assign
    std::uint16_t next_id_{0};
    
    /// @brief Maximum component signature bits supported
    static constexpr std::size_t component_signature_bits_ = 64;
};

/// @brief Global component registration helper
template<Component T>
class ComponentRegistrar {
public:
    explicit ComponentRegistrar(std::string_view name = typeid(T).name()) {
        component_id_ = ComponentRegistry::instance().register_component<T>(name);
    }
    
    ComponentId id() const noexcept { return component_id_; }
    
    static ComponentId get_id() {
        static ComponentRegistrar<T> registrar;
        return registrar.id();
    }

private:
    ComponentId component_id_{ComponentId::invalid()};
};

/// @brief Convenience macro for component registration
#define ECSCOPE_REGISTER_COMPONENT(Type, Name) \
    namespace { \
        static const auto Type##_registrar = ::ecscope::foundation::ComponentRegistrar<Type>(Name); \
    }

/// @brief Component type utilities
namespace component_utils {

/// @brief Get component ID for a type (with automatic registration)
template<Component T>
ComponentId get_component_id() {
    return ComponentRegistrar<T>::get_id();
}

/// @brief Check if type is registered as component
template<typename T>
bool is_registered() {
    return ComponentRegistry::instance().get_component_id<T>().is_valid();
}

/// @brief Get component name by ID
inline std::string_view get_component_name(ComponentId id) {
    const auto* desc = ComponentRegistry::instance().get_component_desc(id);
    return desc ? desc->name : "Unknown";
}

/// @brief Create component signature for types
template<Component... Components>
ComponentSignature create_signature() {
    ComponentSignature signature = 0;
    ((signature = ComponentRegistry::add_component_to_signature(signature, get_component_id<Components>())), ...);
    return signature;
}

/// @brief Component tuple for multiple component operations
template<Component... Components>
struct ComponentTuple {
    std::tuple<Components...> components;
    
    static constexpr std::size_t size() { return sizeof...(Components); }
    
    static ComponentSignature signature() {
        return create_signature<Components...>();
    }
    
    template<std::size_t Index>
    auto& get() { return std::get<Index>(components); }
    
    template<std::size_t Index>
    const auto& get() const { return std::get<Index>(components); }
    
    template<Component T>
    T& get() { return std::get<T>(components); }
    
    template<Component T>
    const T& get() const { return std::get<T>(components); }
};

/// @brief Create component tuple
template<Component... Components>
ComponentTuple<Components...> make_component_tuple(Components&&... components) {
    return ComponentTuple<Components...>{{std::forward<Components>(components)...}};
}

}  // namespace component_utils

}  // namespace ecscope::foundation