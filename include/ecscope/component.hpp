#pragma once

#include "core/types.hpp"
#include "core/id.hpp"
#include <type_traits>
#include <concepts>
#include <typeinfo>

namespace ecscope::ecs {

// Component concept - defines what qualifies as a component
template<typename T>
concept Component = std::is_standard_layout_v<T> &&     // Memory layout predictable
                    std::is_trivially_copyable_v<T> &&  // Can memcpy safely 
                    std::is_destructible_v<T> &&        // Can destroy cleanly
                    !std::is_pointer_v<T> &&            // No raw pointers
                    !std::is_reference_v<T>;            // No references

// Component metadata trait
template<Component T>
struct ComponentTraits {
    using type = T;
    static constexpr usize size = sizeof(T);
    static constexpr usize alignment = alignof(T);
    static constexpr bool is_trivial = std::is_trivial_v<T>;
    static constexpr bool is_pod = std::is_standard_layout_v<T> && std::is_trivial_v<T>;
    
    // Get unique component ID for this type
    static core::ComponentID id() noexcept {
        return core::component_id<T>();
    }
    
    // Component type name (for debugging)
    static constexpr const char* name() noexcept {
        return typeid(T).name(); // Note: implementation-specific, but useful for debug
    }
};

// Base class marker (optional - components don't need to inherit from this)
struct ComponentBase {
    // Empty base - just a marker for documentation
    // Components are pure data - no virtual functions or behavior
};

// Utility to get component ID for any component type
template<Component T>
constexpr core::ComponentID component_id() noexcept {
    return ComponentTraits<T>::id();
}

// Utility to get component size
template<Component T>
constexpr usize component_size() noexcept {
    return ComponentTraits<T>::size;
}

// Utility to get component alignment
template<Component T>
constexpr usize component_alignment() noexcept {
    return ComponentTraits<T>::alignment;
}

// Type-erased component info (for archetype storage)
struct ComponentInfo {
    core::ComponentID id;
    usize size;
    usize alignment;
    const char* name;
    
    template<Component T>
    static ComponentInfo create() noexcept {
        return ComponentInfo{
            .id = component_id<T>(),
            .size = component_size<T>(),
            .alignment = component_alignment<T>(),
            .name = ComponentTraits<T>::name()
        };
    }
    
    bool operator==(const ComponentInfo& other) const noexcept {
        return id == other.id;
    }
    
    bool operator<(const ComponentInfo& other) const noexcept {
        return id < other.id;
    }
};

} // namespace ecscope::ecs