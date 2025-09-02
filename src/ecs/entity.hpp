#pragma once

#include "core/types.hpp"
#include "core/id.hpp"

namespace ecscope::ecs {

// Entity is just an ID - pure data, no behavior
using Entity = core::EntityID;

// Entity validation helpers
constexpr bool is_valid_entity(Entity entity) noexcept {
    return entity.is_valid();
}

constexpr Entity null_entity() noexcept {
    return core::NULL_ENTITY;
}

// Entity creation shortcut (will be used by Registry)
inline Entity create_entity() noexcept {
    return core::entity_id_generator().create();
}

// Entity recycling shortcut
inline Entity recycle_entity(u32 index, u32 old_generation) noexcept {
    return core::entity_id_generator().recycle(index, old_generation);
}

} // namespace ecscope::ecs