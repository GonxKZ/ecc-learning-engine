#pragma once

#include "../core/types.hpp"
#include "../core/memory.hpp"
#include "concepts.hpp"
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <memory>
#include <algorithm>
#include <bit>

/**
 * @file entity.hpp
 * @brief Generational entity ID system with thread safety
 * 
 * This file implements a high-performance entity management system with:
 * - Generational indices to detect stale entity references
 * - Thread-safe entity creation and destruction
 * - Entity recycling to minimize memory usage
 * - Fast entity validation and lookup
 * - Support for millions of entities
 * 
 * Educational Notes:
 * - Generational indices solve the "dangling entity" problem
 * - Each entity slot has an ID + generation counter
 * - When an entity is destroyed, the generation increments
 * - Old entity handles become invalid automatically
 * - Free list maintains destroyed entity slots for reuse
 * - Lock-free operations where possible for performance
 * - Memory layout optimized for cache efficiency
 */

namespace ecscope::foundation {

using namespace ecscope::core;

/// @brief Entity metadata stored in the entity manager
struct EntityMetadata {
    Generation generation{1};  ///< Current generation (0 = never existed)
    bool alive{false};          ///< Whether entity is currently active
    
    constexpr EntityMetadata() = default;
    constexpr EntityMetadata(Generation gen, bool is_alive) 
        : generation(gen), alive(is_alive) {}
    
    constexpr bool is_valid() const noexcept {
        return generation > 0;
    }
};

/// @brief Thread-safe entity ID manager
class EntityManager {
public:
    /// @brief Configuration for entity manager
    struct Config {
        std::uint32_t initial_capacity{1024};      ///< Initial entity capacity
        std::uint32_t max_entities{1'000'000};     ///< Maximum number of entities
        bool enable_recycling{true};               ///< Enable entity ID recycling
        bool thread_safe{true};                    ///< Enable thread safety
        std::uint32_t free_list_batch_size{64};    ///< Batch size for free list operations
    };
    
    explicit EntityManager(const Config& config = {});
    
    /// @brief Non-copyable but movable
    EntityManager(const EntityManager&) = delete;
    EntityManager& operator=(const EntityManager&) = delete;
    EntityManager(EntityManager&&) = default;
    EntityManager& operator=(EntityManager&&) = default;
    
    /// @brief Create a new entity
    /// @return Handle to the created entity
    /// @throws std::runtime_error if maximum entities exceeded
    EntityHandle create_entity();
    
    /// @brief Destroy an entity
    /// @param handle Entity to destroy
    /// @return true if entity was destroyed, false if already dead/invalid
    bool destroy_entity(EntityHandle handle) noexcept;
    
    /// @brief Check if an entity handle is valid and alive
    /// @param handle Entity handle to check
    /// @return true if entity exists and is alive
    bool is_alive(EntityHandle handle) const noexcept;
    
    /// @brief Get entity metadata
    /// @param handle Entity handle
    /// @return Entity metadata, or default if invalid
    EntityMetadata get_metadata(EntityHandle handle) const noexcept;
    
    /// @brief Get current entity count
    /// @return Number of alive entities
    std::uint32_t entity_count() const noexcept;
    
    /// @brief Get total capacity
    /// @return Maximum number of entities that can be stored
    std::uint32_t capacity() const noexcept;
    
    /// @brief Get number of recycled entity slots
    /// @return Number of destroyed entities available for recycling
    std::uint32_t recycled_count() const noexcept;
    
    /// @brief Get entity utilization ratio
    /// @return Ratio of alive entities to total capacity
    double utilization() const noexcept;
    
    /// @brief Compact entity storage by removing gaps
    /// @note This invalidates all existing entity handles!
    void compact();
    
    /// @brief Clear all entities
    void clear() noexcept;
    
    /// @brief Reserve capacity for entities
    /// @param capacity New capacity
    void reserve(std::uint32_t capacity);
    
    /// @brief Get all alive entity handles
    /// @return Vector of all currently alive entities
    std::vector<EntityHandle> get_alive_entities() const;
    
    /// @brief Iterate over all alive entities
    /// @tparam Func Function type (EntityHandle) -> void
    /// @param func Function to call for each alive entity
    template<typename Func>
    void for_each_alive_entity(Func&& func) const;
    
    /// @brief Entity iterator for range-based loops
    class EntityIterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = EntityHandle;
        using pointer = const EntityHandle*;
        using reference = const EntityHandle&;
        
        EntityIterator(const EntityManager* manager, std::uint32_t index);
        
        reference operator*() const noexcept { return current_handle_; }
        pointer operator->() const noexcept { return &current_handle_; }
        
        EntityIterator& operator++();
        EntityIterator operator++(int);
        
        bool operator==(const EntityIterator& other) const noexcept;
        bool operator!=(const EntityIterator& other) const noexcept;

    private:
        void find_next_alive();
        
        const EntityManager* manager_;
        std::uint32_t current_index_;
        EntityHandle current_handle_;
    };
    
    /// @brief Begin iterator for alive entities
    EntityIterator begin() const { return EntityIterator(this, 0); }
    
    /// @brief End iterator for alive entities
    EntityIterator end() const { return EntityIterator(this, static_cast<std::uint32_t>(entities_.size())); }
    
    /// @brief Get entity manager statistics
    struct Stats {
        std::uint32_t alive_count{};
        std::uint32_t total_capacity{};
        std::uint32_t recycled_count{};
        std::uint32_t peak_usage{};
        std::uint64_t total_created{};
        std::uint64_t total_destroyed{};
        double utilization{};
        std::uint32_t generation_overflow_count{};
    };
    
    Stats get_stats() const noexcept;
    
    /// @brief Reset statistics counters
    void reset_stats() noexcept;

private:
    /// @brief Find next available entity slot
    EntityId find_free_slot();
    
    /// @brief Grow entity storage capacity
    void grow_capacity(std::uint32_t new_capacity);
    
    /// @brief Validate entity ID is within bounds
    bool is_valid_id(EntityId id) const noexcept;
    
    Config config_;
    
    /// @brief Entity metadata storage
    std::vector<EntityMetadata> entities_;
    
    /// @brief Free entity ID list for recycling
    std::queue<EntityId> free_list_;
    
    /// @brief Thread safety
    mutable std::mutex mutex_;
    
    /// @brief Statistics (atomic for thread-safe access)
    std::atomic<std::uint32_t> alive_count_{0};
    std::atomic<std::uint32_t> peak_usage_{0};
    std::atomic<std::uint64_t> total_created_{0};
    std::atomic<std::uint64_t> total_destroyed_{0};
    std::atomic<std::uint32_t> generation_overflow_count_{0};
    
    /// @brief Next entity ID to allocate (for non-recycled entities)
    std::atomic<std::uint32_t> next_id_{0};
};

/// @brief Entity handle validation utilities
namespace entity_utils {

/// @brief Check if two entity handles refer to the same entity
constexpr bool are_same_entity(EntityHandle a, EntityHandle b) noexcept {
    return a.id == b.id && a.generation == b.generation;
}

/// @brief Check if entity handle is definitely invalid
constexpr bool is_definitely_invalid(EntityHandle handle) noexcept {
    return !handle.id.is_valid() || handle.generation == 0;
}

/// @brief Create an invalid entity handle
constexpr EntityHandle make_invalid() noexcept {
    return EntityHandle::invalid();
}

/// @brief Generate hash for entity handle (for unordered containers)
struct EntityHandleHash {
    std::size_t operator()(EntityHandle handle) const noexcept {
        return std::hash<EntityHandle>{}(handle);
    }
};

/// @brief Entity handle equality comparison
struct EntityHandleEqual {
    bool operator()(EntityHandle a, EntityHandle b) const noexcept {
        return are_same_entity(a, b);
    }
};

/// @brief Entity batch operations
class EntityBatch {
public:
    explicit EntityBatch(EntityManager& manager) : manager_(manager) {}
    
    /// @brief Create multiple entities at once
    std::vector<EntityHandle> create_entities(std::uint32_t count) {
        std::vector<EntityHandle> entities;
        entities.reserve(count);
        
        for (std::uint32_t i = 0; i < count; ++i) {
            entities.push_back(manager_.create_entity());
        }
        
        return entities;
    }
    
    /// @brief Destroy multiple entities at once
    std::uint32_t destroy_entities(std::span<const EntityHandle> entities) noexcept {
        std::uint32_t destroyed_count = 0;
        
        for (EntityHandle entity : entities) {
            if (manager_.destroy_entity(entity)) {
                ++destroyed_count;
            }
        }
        
        return destroyed_count;
    }
    
    /// @brief Check multiple entities for validity
    std::vector<bool> check_entities_alive(std::span<const EntityHandle> entities) const {
        std::vector<bool> results;
        results.reserve(entities.size());
        
        for (EntityHandle entity : entities) {
            results.push_back(manager_.is_alive(entity));
        }
        
        return results;
    }

private:
    EntityManager& manager_;
};

}  // namespace entity_utils

/// @brief Implementation details
namespace detail {

/// @brief Entity slot allocation strategy
enum class AllocationStrategy {
    Linear,      ///< Allocate IDs linearly (no recycling)
    Recycled,    ///< Reuse destroyed entity IDs
    Hybrid       ///< Use recycling with linear fallback
};

/// @brief Entity generation overflow handler
class GenerationOverflowHandler {
public:
    /// @brief Handle generation overflow for an entity slot
    /// @param id Entity ID that overflowed
    /// @param current_generation Current generation value
    /// @return New generation value to use
    static Generation handle_overflow(EntityId id, Generation current_generation) noexcept;
    
    /// @brief Check if generation is approaching overflow
    /// @param generation Current generation value
    /// @return true if close to overflow
    static bool is_near_overflow(Generation generation) noexcept;
    
    /// @brief Get maximum safe generation value
    static constexpr Generation max_safe_generation() noexcept {
        return std::numeric_limits<Generation>::max() - 1000;
    }
};

}  // namespace detail

// Template implementation
template<typename Func>
void EntityManager::for_each_alive_entity(Func&& func) const {
    if (config_.thread_safe) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        const auto size = static_cast<std::uint32_t>(entities_.size());
        for (std::uint32_t i = 0; i < size; ++i) {
            const auto& metadata = entities_[i];
            if (metadata.alive) {
                func(EntityHandle{EntityId{i}, metadata.generation});
            }
        }
    } else {
        const auto size = static_cast<std::uint32_t>(entities_.size());
        for (std::uint32_t i = 0; i < size; ++i) {
            const auto& metadata = entities_[i];
            if (metadata.alive) {
                func(EntityHandle{EntityId{i}, metadata.generation});
            }
        }
    }
}

}  // namespace ecscope::foundation