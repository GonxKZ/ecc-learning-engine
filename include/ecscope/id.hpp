#pragma once

#include "types.hpp"
#include <atomic>
#include <limits>

namespace ecscope::core {

// Entity ID with generational index for detecting dangling references
struct EntityID {
    u32 index;      // Index into entity array
    u32 generation; // Generation counter to detect stale references
    
    constexpr EntityID() noexcept : index(INVALID_ID), generation(0) {}
    constexpr EntityID(u32 idx, u32 gen) noexcept : index(idx), generation(gen) {}
    
    // Validity check
    constexpr bool is_valid() const noexcept {
        return index != INVALID_ID;
    }
    
    // Comparison operators
    constexpr bool operator==(const EntityID& other) const noexcept {
        return index == other.index && generation == other.generation;
    }
    
    constexpr bool operator!=(const EntityID& other) const noexcept {
        return !(*this == other);
    }
    
    constexpr bool operator<(const EntityID& other) const noexcept {
        if (index != other.index) return index < other.index;
        return generation < other.generation;
    }
    
    // For use in hash containers
    struct Hash {
        constexpr usize operator()(const EntityID& id) const noexcept {
            // Combine index and generation using FNV-1a style hash
            usize hash = 2166136261u;
            hash ^= id.index;
            hash *= 16777619u;
            hash ^= id.generation;
            hash *= 16777619u;
            return hash;
        }
    };
    
    // String representation for debugging
    constexpr u64 as_u64() const noexcept {
        return (static_cast<u64>(generation) << 32) | index;
    }
    
    static constexpr EntityID from_u64(u64 value) noexcept {
        return EntityID(
            static_cast<u32>(value & 0xFFFFFFFF),
            static_cast<u32>(value >> 32)
        );
    }
};

// Invalid/null entity constant
constexpr EntityID NULL_ENTITY{};

// Component type ID - unique identifier for each component type
class ComponentID {
private:
    static std::atomic<u32> next_id_;
    u32 id_;
    
public:
    explicit ComponentID(u32 id) noexcept : id_(id) {}
    
    constexpr u32 value() const noexcept { return id_; }
    
    constexpr bool operator==(const ComponentID& other) const noexcept {
        return id_ == other.id_;
    }
    
    constexpr bool operator!=(const ComponentID& other) const noexcept {
        return id_ != other.id_;
    }
    
    constexpr bool operator<(const ComponentID& other) const noexcept {
        return id_ < other.id_;
    }
    
    // Generate next unique component ID (thread-safe)
    static ComponentID next() noexcept {
        return ComponentID(next_id_.fetch_add(1, std::memory_order_relaxed));
    }
    
    // For use in hash containers
    struct Hash {
        constexpr usize operator()(const ComponentID& id) const noexcept {
            return static_cast<usize>(id.value());
        }
    };
};

// Type-safe component ID generator
template<typename T>
ComponentID component_id() noexcept {
    static const ComponentID id = ComponentID::next();
    return id;
}

// Entity generation manager - thread-safe entity ID generator
class EntityIDGenerator {
private:
    std::atomic<u32> next_index_{0};
    
public:
    // Generate new entity ID
    EntityID create() noexcept {
        u32 index = next_index_.fetch_add(1, std::memory_order_relaxed);
        return EntityID(index, 1); // Start with generation 1 (0 reserved for null)
    }
    
    // Create new generation for recycled index (for entity recycling)
    EntityID recycle(u32 index, u32 old_generation) noexcept {
        u32 new_generation = old_generation + 1;
        // Handle overflow (though unlikely in practice)
        if (new_generation == 0) {
            new_generation = 1;
        }
        return EntityID(index, new_generation);
    }
    
    // Reset generator (mainly for testing)
    void reset() noexcept {
        next_index_.store(0, std::memory_order_relaxed);
    }
};

// Global entity ID generator
inline EntityIDGenerator& entity_id_generator() noexcept {
    static EntityIDGenerator generator;
    return generator;
}

} // namespace ecscope::core

// Hash specialization for EntityID in std namespace
namespace std {
    template<>
    struct hash<ecscope::core::EntityID> {
        usize operator()(const ecscope::core::EntityID& id) const noexcept {
            return ecscope::core::EntityID::Hash{}(id);
        }
    };
    
    template<>
    struct hash<ecscope::core::ComponentID> {
        usize operator()(const ecscope::core::ComponentID& id) const noexcept {
            return ecscope::core::ComponentID::Hash{}(id);
        }
    };
}