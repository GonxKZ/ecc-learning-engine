#pragma once

#include "../core/types.hpp"
#include <concepts>
#include <type_traits>
#include <span>
#include <ranges>

/**
 * @file concepts.hpp
 * @brief C++20 concepts and constraints for the ECS framework
 * 
 * This file defines concepts that enforce type safety and correct usage
 * throughout the ECS system. Concepts provide better error messages
 * and clearer interfaces than traditional SFINAE.
 * 
 * Educational Notes:
 * - Concepts replace complex SFINAE with readable constraints
 * - They provide better compiler error messages
 * - Enable concept-based function overloading
 * - Support concept refinement and composition
 * - Help with API design and documentation
 */

namespace ecscope::foundation {

using namespace ecscope::core;

/// @brief Concept for valid ECS component types
template<typename T>
concept Component = requires {
    // Must be regular type (copyable, movable, equality comparable)
    requires std::regular<T>;
    
    // Must be trivially relocatable for efficient storage
    // Note: This is not standard yet, but many implementations provide it
    requires std::is_nothrow_move_constructible_v<T> || std::is_trivially_copyable_v<T>;
    
    // Must have reasonable size (prevent huge components)
    requires sizeof(T) <= 4096;  // 4KB max component size
    
    // Must not be polymorphic (no virtual functions)
    requires !std::is_polymorphic_v<T>;
    
    // Must not be a reference or pointer type
    requires !std::is_reference_v<T> && !std::is_pointer_v<T>;
};

/// @brief Concept for tag components (empty or stateless)
template<typename T>
concept TagComponent = Component<T> && (sizeof(T) <= 1 || std::is_empty_v<T>);

/// @brief Concept for data components (non-empty with meaningful data)
template<typename T>
concept DataComponent = Component<T> && sizeof(T) > 1 && !std::is_empty_v<T>;

/// @brief Concept for SIMD-friendly components
template<typename T>
concept SimdComponent = Component<T> && requires {
    // Must be trivially copyable for vectorization
    requires std::is_trivially_copyable_v<T>;
    
    // Must have appropriate alignment for SIMD
    requires alignof(T) >= 4;
    
    // Size should be multiple of 4 bytes for better vectorization
    requires (sizeof(T) % 4) == 0;
};

/// @brief Concept for system types
template<typename T>
concept System = requires(T system) {
    // Must be move constructible
    requires std::is_move_constructible_v<T>;
    
    // Must not be copyable (systems should be unique)
    requires !std::is_copy_constructible_v<T>;
    
    // Must have an update method (exact signature varies)
    requires requires { system.update(); } || 
             requires(float dt) { system.update(dt); };
};

/// @brief Concept for query-compatible systems
template<typename T>
concept QuerySystem = System<T> && requires(T system) {
    // Must define component requirements through queries
    typename T::Query;
    
    // Must have process method for entities
    requires requires(EntityHandle entity) { 
        system.process(entity); 
    } || requires(EntityHandle entity, float dt) { 
        system.process(entity, dt); 
    };
};

/// @brief Concept for allocator types
template<typename T>
concept Allocator = requires(T allocator) {
    // Must support allocation and deallocation
    requires requires(std::size_t size) {
        { allocator.allocate(size) } -> std::same_as<void*>;
    };
    
    requires requires(void* ptr) {
        allocator.deallocate(ptr);
    };
    
    // Must be movable
    requires std::is_move_constructible_v<T>;
};

/// @brief Concept for storage types
template<typename T>
concept Storage = requires(T storage) {
    // Must support entity operations
    typename T::value_type;
    typename T::size_type;
    
    requires requires(EntityHandle entity, typename T::value_type component) {
        { storage.contains(entity) } -> std::convertible_to<bool>;
        storage.insert(entity, std::move(component));
        storage.remove(entity);
    };
    
    requires requires(EntityHandle entity) {
        { storage.get(entity) } -> std::same_as<typename T::value_type&>;
        { storage.get(entity) } -> std::same_as<const typename T::value_type&>;
    };
    
    requires requires() {
        { storage.size() } -> std::convertible_to<std::size_t>;
        { storage.empty() } -> std::convertible_to<bool>;
    };
};

/// @brief Concept for iterable storage
template<typename T>
concept IterableStorage = Storage<T> && requires(T storage) {
    typename T::iterator;
    typename T::const_iterator;
    
    { storage.begin() } -> std::same_as<typename T::iterator>;
    { storage.end() } -> std::same_as<typename T::iterator>;
    { storage.cbegin() } -> std::same_as<typename T::const_iterator>;
    { storage.cend() } -> std::same_as<typename T::const_iterator>;
    
    // Iterator requirements
    requires std::input_iterator<typename T::iterator>;
    requires std::input_iterator<typename T::const_iterator>;
};

/// @brief Concept for archetype storage
template<typename T>
concept ArchetypeStorage = requires(T archetype) {
    // Must support component type queries
    requires requires(ComponentId id) {
        { archetype.has_component(id) } -> std::convertible_to<bool>;
    };
    
    // Must support entity iteration
    requires requires() {
        { archetype.entity_count() } -> std::convertible_to<std::size_t>;
    };
    
    // Must support component data access
    requires requires(ComponentId id) {
        archetype.get_component_data(id);
    };
};

/// @brief Concept for event types
template<typename T>
concept Event = requires {
    // Must be regular type
    requires std::regular<T>;
    
    // Must be reasonably sized
    requires sizeof(T) <= 1024;  // 1KB max event size
    
    // Must be movable (for event queue optimization)
    requires std::is_nothrow_move_constructible_v<T>;
};

/// @brief Concept for resource types (shared data not tied to entities)
template<typename T>
concept Resource = requires {
    // Must be movable but not necessarily copyable
    requires std::is_move_constructible_v<T>;
    
    // Must be default constructible or have factory
    requires std::is_default_constructible_v<T> || requires() {
        { T::create() } -> std::same_as<T>;
    };
};

/// @brief Concept for thread-safe types
template<typename T>
concept ThreadSafe = requires {
    // Type should explicitly declare thread safety
    requires T::is_thread_safe;
    
    // Or provide synchronization primitives
    requires requires(T obj) {
        obj.lock();
        obj.unlock();
    } || requires(T obj) {
        typename T::mutex_type;
    };
};

/// @brief Concept for lockfree types
template<typename T>
concept LockFree = requires {
    // Type should explicitly declare lock-free property
    requires T::is_lock_free;
    
    // Should use atomic operations
    requires std::is_trivially_copyable_v<T> || requires {
        typename T::atomic_type;
    };
};

/// @brief Concept for serializable types
template<typename T>
concept Serializable = requires(T obj) {
    // Must support serialization interface
    requires requires(std::span<std::byte> buffer) {
        { obj.serialize(buffer) } -> std::convertible_to<std::size_t>;
    };
    
    requires requires(std::span<const std::byte> buffer) {
        obj.deserialize(buffer);
    };
    
    // Must provide size hint
    requires requires() {
        { T::serialized_size() } -> std::convertible_to<std::size_t>;
    } || requires() {
        { obj.serialized_size() } -> std::convertible_to<std::size_t>;
    };
};

/// @brief Concept for reflectable types (with metadata)
template<typename T>
concept Reflectable = requires {
    // Must provide type information
    requires requires() {
        { T::type_name() } -> std::convertible_to<const char*>;
        { T::type_id() } -> std::convertible_to<std::size_t>;
    };
    
    // Must provide field/member information
    requires requires() {
        T::field_count();
    };
};

/// @brief Concept for hashable types
template<typename T>
concept Hashable = requires(const T& obj) {
    { std::hash<T>{}(obj) } -> std::convertible_to<std::size_t>;
};

/// @brief Concept for comparable types
template<typename T>
concept Comparable = requires(const T& a, const T& b) {
    { a == b } -> std::convertible_to<bool>;
    { a != b } -> std::convertible_to<bool>;
    { a < b } -> std::convertible_to<bool>;
    { a <= b } -> std::convertible_to<bool>;
    { a > b } -> std::convertible_to<bool>;
    { a >= b } -> std::convertible_to<bool>;
};

/// @brief Concept for query filters
template<typename T>
concept QueryFilter = requires(T filter, EntityHandle entity) {
    { filter(entity) } -> std::convertible_to<bool>;
    
    // Should be copyable for query composition
    requires std::is_copy_constructible_v<T>;
};

/// @brief Concept for entity query types
template<typename T>
concept EntityQuery = requires(T query) {
    // Must specify required components
    typename T::Required;
    
    // May specify excluded components
    // typename T::Excluded; // Optional
    
    // Must be iterable
    requires std::ranges::input_range<T>;
    
    // Iterator should yield entity handles
    requires std::same_as<typename T::value_type, EntityHandle> ||
             std::convertible_to<typename T::value_type, EntityHandle>;
};

/// @brief Concept for component bundles (multiple components together)
template<typename T>
concept ComponentBundle = requires {
    // Must define the component types in the bundle
    typename T::Components;
    
    // Must be constructible
    requires std::is_constructible_v<T>;
    
    // Must support component extraction
    requires requires(T bundle) {
        bundle.extract_components();
    };
};

/// @brief Concept for entity builders
template<typename T>
concept EntityBuilder = requires(T builder) {
    // Must support component addition
    requires requires() {
        builder.template with<int>(42);  // Example component
    };
    
    // Must support entity creation
    requires requires() {
        { builder.build() } -> std::convertible_to<EntityHandle>;
    };
};

/// @brief Concept for world/registry types
template<typename T>
concept World = requires(T world) {
    // Must support entity management
    requires requires() {
        { world.create_entity() } -> std::convertible_to<EntityHandle>;
    };
    
    requires requires(EntityHandle entity) {
        { world.is_valid(entity) } -> std::convertible_to<bool>;
        world.destroy_entity(entity);
    };
    
    // Must support component operations
    requires requires(EntityHandle entity) {
        world.template add_component<int>(entity, 42);
        { world.template has_component<int>(entity) } -> std::convertible_to<bool>;
        world.template remove_component<int>(entity);
    };
    
    // Must support system management
    requires requires() {
        world.template register_system<int>();  // Example system type
    };
};

/// @brief Concept validation helpers
namespace validation {

/// @brief Compile-time check if type satisfies concept
template<typename T, template<typename> class Concept>
constexpr bool satisfies_concept = requires { requires Concept<T>; };

/// @brief Get concept satisfaction status at compile time
template<typename T>
constexpr auto get_concept_status() noexcept {
    struct ConceptStatus {
        bool is_component = Component<T>;
        bool is_tag_component = TagComponent<T>;
        bool is_data_component = DataComponent<T>;
        bool is_simd_component = SimdComponent<T>;
        bool is_event = Event<T>;
        bool is_resource = Resource<T>;
        bool is_serializable = Serializable<T>;
        bool is_reflectable = Reflectable<T>;
        bool is_hashable = Hashable<T>;
        bool is_comparable = Comparable<T>;
    };
    
    return ConceptStatus{};
}

}  // namespace validation

}  // namespace ecscope::foundation