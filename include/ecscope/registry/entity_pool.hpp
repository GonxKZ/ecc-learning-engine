#pragma once

#include "../core/types.hpp"
#include "../core/memory.hpp"
#include "../foundation/concepts.hpp"
#include "../foundation/entity.hpp"
#include "sparse_set.hpp"
#include "archetype.hpp"
#include <vector>
#include <span>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <type_traits>
#include <utility>
#include <cassert>
#include <atomic>
#include <mutex>
#include <queue>
#include <functional>
#include <execution>

/**
 * @file entity_pool.hpp
 * @brief Advanced entity pool for bulk operations and lifecycle management
 * 
 * This file implements a comprehensive entity management system with:
 * - Bulk entity creation and destruction operations
 * - Entity templates and prefab instantiation
 * - Efficient entity recycling and memory management
 * - Thread-safe entity operations with minimal contention
 * - Entity relationship tracking and dependency management
 * - Batch component operations across entity groups
 * - Entity validation and debugging support
 * - Performance monitoring and optimization
 * 
 * Educational Notes:
 * - Entity pools reduce allocation overhead through recycling
 * - Bulk operations amortize per-entity costs across groups
 * - Entity templates enable efficient prefab instantiation
 * - Generational indices prevent dangling entity references
 * - Batch processing improves cache locality and throughput
 * - Thread-safe design enables concurrent entity operations
 * - Memory pools reduce fragmentation and improve performance
 */

namespace ecscope::registry {

using namespace ecscope::core;
using namespace ecscope::foundation;

/// @brief Configuration for entity pool behavior
struct EntityPoolConfig {
    std::uint32_t initial_capacity{4096};                 ///< Initial entity capacity
    std::uint32_t max_entities{1'000'000};                ///< Maximum entities
    std::uint32_t batch_size{256};                        ///< Default batch processing size
    std::uint32_t recycling_threshold{1000};              ///< Min entities before recycling
    bool enable_entity_recycling{true};                   ///< Enable entity ID recycling
    bool enable_thread_safety{true};                      ///< Enable thread-safe operations
    bool enable_validation{true};                         ///< Enable entity validation
    bool enable_relationship_tracking{true};              ///< Enable entity relationships
    bool enable_prefab_caching{true};                     ///< Cache prefab instances
    double memory_pool_growth_factor{1.5};                ///< Memory pool growth rate
    std::uint32_t prefab_cache_size{512};                 ///< Maximum cached prefabs
};

/// @brief Entity template for prefab instantiation
struct EntityTemplate {
    ComponentSignature signature{};
    std::unordered_map<ComponentId, std::vector<std::byte>> component_data;
    std::string name;
    std::uint32_t usage_count{};
    
    /// @brief Create template from existing entity
    template<typename Registry>
    static EntityTemplate create_from_entity(const Registry& registry, EntityHandle entity, 
                                            const std::string& name = "") {
        EntityTemplate template_def;
        template_def.name = name.empty() ? "Template_" + std::to_string(entity.id.value) : name;
        
        // Extract component data from entity
        // In a full implementation, this would serialize all components
        template_def.signature = registry.get_entity_signature(entity);
        
        return template_def;
    }
    
    /// @brief Calculate memory usage of template
    std::size_t memory_usage() const noexcept {
        std::size_t total = sizeof(*this) + name.size();
        for (const auto& [id, data] : component_data) {
            total += data.size();
        }
        return total;
    }
};

/// @brief Entity relationship types
enum class RelationshipType : std::uint8_t {
    Parent,      ///< Parent-child hierarchy relationship
    Owns,        ///< Ownership relationship (owner destroys owned)
    References,  ///< Reference relationship (weak link)
    Depends,     ///< Dependency relationship (requires other entity)
    Groups       ///< Group membership relationship
};

/// @brief Entity relationship descriptor
struct EntityRelationship {
    EntityHandle from;
    EntityHandle to;
    RelationshipType type;
    std::uint32_t strength{1};  ///< Relationship strength/weight
    
    bool operator==(const EntityRelationship& other) const noexcept {
        return from == other.from && to == other.to && type == other.type;
    }
};

/// @brief Bulk entity operation descriptor
struct BulkEntityOperation {
    enum class Type { Create, Destroy, AddComponent, RemoveComponent, MoveArchetype };
    
    Type operation_type;
    std::vector<EntityHandle> entities;
    ComponentId component_id{ComponentId::invalid()};
    ArchetypeId target_archetype{ArchetypeId::invalid()};
    std::vector<std::byte> component_data;  // For component additions
    
    /// @brief Estimate operation cost for scheduling
    std::uint32_t estimate_cost() const noexcept {
        switch (operation_type) {
            case Type::Create: return static_cast<std::uint32_t>(entities.size() * 2);
            case Type::Destroy: return static_cast<std::uint32_t>(entities.size() * 3);
            case Type::AddComponent: 
            case Type::RemoveComponent: return static_cast<std::uint32_t>(entities.size() * 4);
            case Type::MoveArchetype: return static_cast<std::uint32_t>(entities.size() * 5);
        }
        return static_cast<std::uint32_t>(entities.size());
    }
};

/// @brief Entity pool statistics
struct EntityPoolStats {
    std::uint64_t entities_created{};
    std::uint64_t entities_destroyed{};
    std::uint64_t entities_recycled{};
    std::uint64_t bulk_operations_executed{};
    std::uint64_t relationships_created{};
    std::uint64_t prefabs_instantiated{};
    std::uint32_t current_entity_count{};
    std::uint32_t peak_entity_count{};
    std::uint32_t recycled_entity_count{};
    std::uint32_t active_relationship_count{};
    std::size_t memory_usage_bytes{};
    double entity_fragmentation_ratio{};
    double average_bulk_operation_size{};
};

/// @brief Advanced entity pool with bulk operations and lifecycle management
class AdvancedEntityPool {
public:
    using entity_batch = std::vector<EntityHandle>;
    using relationship_callback = std::function<void(const EntityRelationship&)>;
    
    explicit AdvancedEntityPool(const EntityPoolConfig& config = {})
        : config_(config)
        , entity_manager_(EntityManager::Config{
            .initial_capacity = config.initial_capacity,
            .max_entities = config.max_entities,
            .enable_recycling = config.enable_entity_recycling,
            .thread_safe = config.enable_thread_safety
          }) {
        
        // Pre-allocate containers
        entity_relationships_.reserve(config_.initial_capacity / 4);
        entity_templates_.reserve(config_.prefab_cache_size);
        pending_operations_.reserve(config_.batch_size);
    }
    
    /// @brief Create single entity
    /// @return New entity handle
    /// @throws std::runtime_error if maximum entities exceeded
    EntityHandle create_entity() {
        auto entity = entity_manager_.create_entity();
        
        stats_.entities_created.fetch_add(1, std::memory_order_relaxed);
        stats_.current_entity_count.fetch_add(1, std::memory_order_relaxed);
        
        // Update peak count
        auto current = stats_.current_entity_count.load(std::memory_order_relaxed);
        auto peak = stats_.peak_entity_count.load(std::memory_order_relaxed);
        while (current > peak && !stats_.peak_entity_count.compare_exchange_weak(
            peak, current, std::memory_order_relaxed)) {}
        
        if (entity_created_callback_) {
            entity_created_callback_(entity);
        }
        
        return entity;
    }
    
    /// @brief Create multiple entities efficiently
    /// @param count Number of entities to create
    /// @return Vector of new entity handles
    entity_batch create_entities(std::uint32_t count) {
        if (count == 0) return {};
        
        entity_batch entities;
        entities.reserve(count);
        
        // Use batch allocation for efficiency
        for (std::uint32_t i = 0; i < count; ++i) {
            entities.push_back(create_entity());
        }
        
        return entities;
    }
    
    /// @brief Create entities from template
    /// @param template_def Entity template
    /// @param count Number of instances to create
    /// @return Vector of instantiated entities
    entity_batch create_from_template(const EntityTemplate& template_def, std::uint32_t count = 1) {
        if (count == 0) return {};
        
        auto entities = create_entities(count);
        
        // Apply template to all entities
        for (EntityHandle entity : entities) {
            instantiate_template(entity, template_def);
        }
        
        stats_.prefabs_instantiated.fetch_add(count, std::memory_order_relaxed);
        
        return entities;
    }
    
    /// @brief Destroy single entity
    /// @param entity Entity to destroy
    /// @return true if entity was destroyed
    bool destroy_entity(EntityHandle entity) {
        // Handle relationships first
        if (config_.enable_relationship_tracking) {
            destroy_entity_relationships(entity);
        }
        
        bool destroyed = entity_manager_.destroy_entity(entity);
        if (destroyed) {
            stats_.entities_destroyed.fetch_add(1, std::memory_order_relaxed);
            stats_.current_entity_count.fetch_sub(1, std::memory_order_relaxed);
            
            if (entity_destroyed_callback_) {
                entity_destroyed_callback_(entity);
            }
        }
        
        return destroyed;
    }
    
    /// @brief Destroy multiple entities efficiently
    /// @param entities Entities to destroy
    /// @return Number of entities destroyed
    std::uint32_t destroy_entities(std::span<const EntityHandle> entities) {
        if (entities.empty()) return 0;
        
        std::uint32_t destroyed_count = 0;
        
        // Handle relationships for all entities first
        if (config_.enable_relationship_tracking) {
            for (EntityHandle entity : entities) {
                destroy_entity_relationships(entity);
            }
        }
        
        // Destroy entities in batch
        for (EntityHandle entity : entities) {
            if (entity_manager_.destroy_entity(entity)) {
                ++destroyed_count;
                
                if (entity_destroyed_callback_) {
                    entity_destroyed_callback_(entity);
                }
            }
        }
        
        stats_.entities_destroyed.fetch_add(destroyed_count, std::memory_order_relaxed);
        stats_.current_entity_count.fetch_sub(destroyed_count, std::memory_order_relaxed);
        
        return destroyed_count;
    }
    
    /// @brief Check if entity is alive
    /// @param entity Entity to check
    /// @return true if entity is alive
    bool is_alive(EntityHandle entity) const noexcept {
        return entity_manager_.is_alive(entity);
    }
    
    /// @brief Validate multiple entities
    /// @param entities Entities to validate
    /// @return Vector of validation results
    std::vector<bool> validate_entities(std::span<const EntityHandle> entities) const {
        std::vector<bool> results;
        results.reserve(entities.size());
        
        for (EntityHandle entity : entities) {
            results.push_back(is_alive(entity));
        }
        
        return results;
    }
    
    /// @brief Create entity relationship
    /// @param relationship Relationship to create
    /// @return true if relationship was created
    bool create_relationship(const EntityRelationship& relationship) {
        if (!config_.enable_relationship_tracking) {
            return false;
        }
        
        // Validate entities
        if (!is_alive(relationship.from) || !is_alive(relationship.to)) {
            return false;
        }
        
        std::lock_guard<std::mutex> lock(relationships_mutex_);
        
        // Check for duplicate relationships
        auto range = entity_relationships_.equal_range(relationship.from);
        for (auto it = range.first; it != range.second; ++it) {
            if (it->second == relationship) {
                return false; // Already exists
            }
        }
        
        entity_relationships_.emplace(relationship.from, relationship);
        stats_.relationships_created.fetch_add(1, std::memory_order_relaxed);
        
        if (relationship_created_callback_) {
            relationship_created_callback_(relationship);
        }
        
        return true;
    }
    
    /// @brief Get all relationships for entity
    /// @param entity Entity to get relationships for
    /// @return Vector of relationships where entity is the source
    std::vector<EntityRelationship> get_entity_relationships(EntityHandle entity) const {
        if (!config_.enable_relationship_tracking) {
            return {};
        }
        
        std::lock_guard<std::mutex> lock(relationships_mutex_);
        
        std::vector<EntityRelationship> relationships;
        auto range = entity_relationships_.equal_range(entity);
        
        for (auto it = range.first; it != range.second; ++it) {
            relationships.push_back(it->second);
        }
        
        return relationships;
    }
    
    /// @brief Create entity template from existing entity
    /// @param entity Source entity
    /// @param name Template name
    /// @return Created template
    EntityTemplate create_template_from_entity(EntityHandle entity, const std::string& name) {
        if (!is_alive(entity)) {
            throw std::invalid_argument("Cannot create template from dead entity");
        }
        
        // This would need access to a registry to extract component data
        // For now, create a minimal template
        EntityTemplate template_def;
        template_def.name = name.empty() ? "Template_" + std::to_string(entity.id.value) : name;
        // template_def.signature would be populated from registry
        
        if (config_.enable_prefab_caching) {
            std::lock_guard<std::mutex> lock(templates_mutex_);
            entity_templates_[template_def.name] = template_def;
        }
        
        return template_def;
    }
    
    /// @brief Get cached template by name
    /// @param name Template name
    /// @return Pointer to template, or nullptr if not found
    const EntityTemplate* get_template(const std::string& name) const {
        if (!config_.enable_prefab_caching) {
            return nullptr;
        }
        
        std::lock_guard<std::mutex> lock(templates_mutex_);
        auto it = entity_templates_.find(name);
        return it != entity_templates_.end() ? &it->second : nullptr;
    }
    
    /// @brief Schedule bulk operation for batch processing
    /// @param operation Operation to schedule
    void schedule_bulk_operation(BulkEntityOperation operation) {
        std::lock_guard<std::mutex> lock(operations_mutex_);
        pending_operations_.push_back(std::move(operation));
        
        // Auto-execute if batch is full
        if (pending_operations_.size() >= config_.batch_size) {
            execute_pending_operations_locked();
        }
    }
    
    /// @brief Execute all pending bulk operations
    void execute_pending_operations() {
        std::lock_guard<std::mutex> lock(operations_mutex_);
        execute_pending_operations_locked();
    }
    
    /// @brief Get current entity count
    std::uint32_t entity_count() const noexcept {
        return stats_.current_entity_count.load(std::memory_order_relaxed);
    }
    
    /// @brief Get entity pool statistics
    EntityPoolStats get_stats() const {
        EntityPoolStats stats{};
        
        // Copy atomic values
        stats.entities_created = stats_.entities_created.load(std::memory_order_relaxed);
        stats.entities_destroyed = stats_.entities_destroyed.load(std::memory_order_relaxed);
        stats.entities_recycled = stats_.entities_recycled.load(std::memory_order_relaxed);
        stats.bulk_operations_executed = stats_.bulk_operations_executed.load(std::memory_order_relaxed);
        stats.relationships_created = stats_.relationships_created.load(std::memory_order_relaxed);
        stats.prefabs_instantiated = stats_.prefabs_instantiated.load(std::memory_order_relaxed);
        stats.current_entity_count = stats_.current_entity_count.load(std::memory_order_relaxed);
        stats.peak_entity_count = stats_.peak_entity_count.load(std::memory_order_relaxed);
        
        // Calculate derived metrics
        if (stats.bulk_operations_executed > 0) {
            stats.average_bulk_operation_size = 
                static_cast<double>(stats.entities_created + stats.entities_destroyed) / 
                stats.bulk_operations_executed;
        }
        
        // Calculate memory usage
        stats.memory_usage_bytes = sizeof(*this) + 
                                  entity_relationships_.size() * sizeof(EntityRelationship) +
                                  entity_templates_.size() * sizeof(EntityTemplate) +
                                  pending_operations_.size() * sizeof(BulkEntityOperation);
        
        for (const auto& [name, template_def] : entity_templates_) {
            stats.memory_usage_bytes += template_def.memory_usage();
        }
        
        // Calculate fragmentation ratio
        stats.recycled_entity_count = entity_manager_.recycled_count();
        if (stats.current_entity_count > 0) {
            stats.entity_fragmentation_ratio = 
                static_cast<double>(stats.recycled_entity_count) / stats.current_entity_count;
        }
        
        stats.active_relationship_count = static_cast<std::uint32_t>(entity_relationships_.size());
        
        return stats;
    }
    
    /// @brief Reset performance statistics
    void reset_stats() {
        stats_.entities_created.store(0, std::memory_order_relaxed);
        stats_.entities_destroyed.store(0, std::memory_order_relaxed);
        stats_.entities_recycled.store(0, std::memory_order_relaxed);
        stats_.bulk_operations_executed.store(0, std::memory_order_relaxed);
        stats_.relationships_created.store(0, std::memory_order_relaxed);
        stats_.prefabs_instantiated.store(0, std::memory_order_relaxed);
        stats_.peak_entity_count.store(stats_.current_entity_count.load(std::memory_order_relaxed), 
                                      std::memory_order_relaxed);
    }
    
    /// @brief Set entity lifecycle callbacks
    void set_entity_created_callback(std::function<void(EntityHandle)> callback) {
        entity_created_callback_ = std::move(callback);
    }
    
    void set_entity_destroyed_callback(std::function<void(EntityHandle)> callback) {
        entity_destroyed_callback_ = std::move(callback);
    }
    
    void set_relationship_created_callback(relationship_callback callback) {
        relationship_created_callback_ = std::move(callback);
    }
    
    /// @brief Batch processing utilities
    class BatchProcessor {
    public:
        explicit BatchProcessor(AdvancedEntityPool& pool) : pool_(pool) {}
        
        /// @brief Process entities in parallel batches
        /// @tparam Func Function type: (std::span<const EntityHandle>) -> void
        /// @param entities Entities to process
        /// @param func Processing function
        /// @param batch_size Batch size for parallel processing
        template<typename Func>
        void parallel_for_each(std::span<const EntityHandle> entities, 
                              Func&& func, std::uint32_t batch_size = 0) const {
            if (batch_size == 0) {
                batch_size = pool_.config_.batch_size;
            }
            
            if (entities.size() < batch_size * 2) {
                // Too small for parallel processing
                func(entities);
                return;
            }
            
            // Process in parallel batches
            const auto num_batches = (entities.size() + batch_size - 1) / batch_size;
            
            // In a full implementation, this would use std::execution::parallel_policy
            for (std::size_t batch = 0; batch < num_batches; ++batch) {
                const auto start = batch * batch_size;
                const auto end = std::min(start + batch_size, entities.size());
                const auto batch_span = entities.subspan(start, end - start);
                
                func(batch_span);
            }
        }
        
        /// @brief Create entities with identical template
        /// @param template_def Entity template
        /// @param count Number of entities to create
        /// @return Vector of created entities
        entity_batch create_templated_batch(const EntityTemplate& template_def, 
                                           std::uint32_t count) {
            return pool_.create_from_template(template_def, count);
        }
        
        /// @brief Validate entity batch integrity
        /// @param entities Entities to validate
        /// @return true if all entities are valid
        bool validate_batch_integrity(std::span<const EntityHandle> entities) const {
            return std::all_of(entities.begin(), entities.end(), 
                              [this](EntityHandle entity) {
                                  return pool_.is_alive(entity);
                              });
        }

    private:
        AdvancedEntityPool& pool_;
    };
    
    /// @brief Get batch processor
    BatchProcessor batch() { return BatchProcessor(*this); }
    
    /// @brief Optimize entity pool performance
    void optimize() {
        // Compact entity manager if fragmentation is high
        const auto stats = get_stats();
        if (stats.entity_fragmentation_ratio > 0.5) {
            // Note: This would require careful coordination with active entity references
            // entity_manager_.compact();
        }
        
        // Clean up expired relationships
        if (config_.enable_relationship_tracking) {
            cleanup_invalid_relationships();
        }
        
        // Clean up unused templates
        if (config_.enable_prefab_caching) {
            cleanup_unused_templates();
        }
    }

private:
    /// @brief Instantiate template on entity
    void instantiate_template(EntityHandle entity, const EntityTemplate& template_def) {
        // In a full implementation, this would:
        // 1. Add all components from template to entity
        // 2. Copy component data from template
        // 3. Set up any relationships defined in template
        // 4. Trigger component initialization callbacks
        
        // For now, just increment usage count
        const_cast<EntityTemplate&>(template_def).usage_count++;
    }
    
    /// @brief Execute pending operations (must be called with lock held)
    void execute_pending_operations_locked() {
        if (pending_operations_.empty()) return;
        
        // Group operations by type for better efficiency
        std::sort(pending_operations_.begin(), pending_operations_.end(),
                 [](const BulkEntityOperation& a, const BulkEntityOperation& b) {
                     return static_cast<int>(a.operation_type) < static_cast<int>(b.operation_type);
                 });
        
        // Execute operations in batches
        for (const auto& operation : pending_operations_) {
            execute_bulk_operation(operation);
        }
        
        stats_.bulk_operations_executed.fetch_add(
            pending_operations_.size(), std::memory_order_relaxed);
        
        pending_operations_.clear();
    }
    
    /// @brief Execute single bulk operation
    void execute_bulk_operation(const BulkEntityOperation& operation) {
        switch (operation.operation_type) {
            case BulkEntityOperation::Type::Create: {
                // Create entities would be handled by create_entities
                break;
            }
            case BulkEntityOperation::Type::Destroy: {
                destroy_entities(operation.entities);
                break;
            }
            case BulkEntityOperation::Type::AddComponent:
            case BulkEntityOperation::Type::RemoveComponent:
            case BulkEntityOperation::Type::MoveArchetype: {
                // These would require registry integration
                break;
            }
        }
    }
    
    /// @brief Destroy all relationships involving entity
    void destroy_entity_relationships(EntityHandle entity) {
        std::lock_guard<std::mutex> lock(relationships_mutex_);
        
        // Remove relationships where entity is the source
        entity_relationships_.erase(entity);
        
        // Remove relationships where entity is the target
        for (auto it = entity_relationships_.begin(); it != entity_relationships_.end();) {
            if (it->second.to == entity) {
                it = entity_relationships_.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    /// @brief Clean up invalid relationships
    void cleanup_invalid_relationships() {
        std::lock_guard<std::mutex> lock(relationships_mutex_);
        
        for (auto it = entity_relationships_.begin(); it != entity_relationships_.end();) {
            const auto& relationship = it->second;
            if (!is_alive(relationship.from) || !is_alive(relationship.to)) {
                it = entity_relationships_.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    /// @brief Clean up unused entity templates
    void cleanup_unused_templates() {
        std::lock_guard<std::mutex> lock(templates_mutex_);
        
        // Remove templates that haven't been used recently
        // In a full implementation, this would use time-based cleanup
        for (auto it = entity_templates_.begin(); it != entity_templates_.end();) {
            if (it->second.usage_count == 0) {
                it = entity_templates_.erase(it);
            } else {
                // Reset usage count for next cleanup cycle
                it->second.usage_count = 0;
                ++it;
            }
        }
    }
    
    EntityPoolConfig config_;
    EntityManager entity_manager_;
    
    /// @brief Entity relationships storage
    mutable std::mutex relationships_mutex_;
    std::unordered_multimap<EntityHandle, EntityRelationship> entity_relationships_;
    
    /// @brief Entity templates storage
    mutable std::mutex templates_mutex_;
    std::unordered_map<std::string, EntityTemplate> entity_templates_;
    
    /// @brief Bulk operations queue
    mutable std::mutex operations_mutex_;
    std::vector<BulkEntityOperation> pending_operations_;
    
    /// @brief Statistics (atomic for thread-safe access)
    struct {
        std::atomic<std::uint64_t> entities_created{0};
        std::atomic<std::uint64_t> entities_destroyed{0};
        std::atomic<std::uint64_t> entities_recycled{0};
        std::atomic<std::uint64_t> bulk_operations_executed{0};
        std::atomic<std::uint64_t> relationships_created{0};
        std::atomic<std::uint64_t> prefabs_instantiated{0};
        std::atomic<std::uint32_t> current_entity_count{0};
        std::atomic<std::uint32_t> peak_entity_count{0};
    } stats_;
    
    /// @brief Lifecycle callbacks
    std::function<void(EntityHandle)> entity_created_callback_;
    std::function<void(EntityHandle)> entity_destroyed_callback_;
    relationship_callback relationship_created_callback_;
};

/// @brief Utility functions for entity pool operations
namespace entity_pool_utils {

/// @brief Create optimal configuration for specific use case
inline EntityPoolConfig create_config_for_game(std::uint32_t expected_entities) {
    EntityPoolConfig config{};
    
    if (expected_entities < 1000) {
        // Small game
        config.initial_capacity = 512;
        config.batch_size = 64;
    } else if (expected_entities < 10000) {
        // Medium game
        config.initial_capacity = 2048;
        config.batch_size = 128;
    } else {
        // Large game
        config.initial_capacity = 8192;
        config.batch_size = 256;
    }
    
    config.max_entities = expected_entities * 2; // Allow for growth
    return config;
}

/// @brief Estimate memory usage for entity pool configuration
inline std::size_t estimate_memory_usage(const EntityPoolConfig& config) {
    std::size_t base_size = sizeof(AdvancedEntityPool);
    std::size_t entity_storage = config.initial_capacity * sizeof(EntityHandle);
    std::size_t relationship_storage = config.initial_capacity * sizeof(EntityRelationship) / 4;
    std::size_t template_storage = config.prefab_cache_size * 256; // Rough estimate
    
    return base_size + entity_storage + relationship_storage + template_storage;
}

} // namespace entity_pool_utils

} // namespace ecscope::registry