#pragma once

/**
 * @file relationships.hpp
 * @brief Advanced ECS Component Relationships System
 * 
 * This comprehensive relationship system provides sophisticated entity hierarchy
 * management, component references, and relationship queries with educational
 * focus on graph-based entity organization and memory-efficient relationship storage.
 * 
 * Key Educational Features:
 * - Parent-child hierarchies with efficient tree traversal
 * - Component references using stable entity IDs
 * - Relationship-based queries for graph operations
 * - Memory-optimized relationship storage using arena allocators
 * - Relationship change detection and event propagation
 * - Relationship visualization and debugging tools
 * - Educational demonstrations of tree and graph algorithms
 * 
 * Relationship Types:
 * - Hierarchy: Parent-child relationships forming trees
 * - References: Direct entity-to-entity links
 * - Ownership: Strong ownership relationships
 * - Dependencies: Entity dependency chains
 * - Groups: Many-to-many group memberships
 * 
 * Advanced Features:
 * - Efficient tree traversal (breadth-first, depth-first)
 * - Relationship validation and constraint checking
 * - Bulk relationship operations for performance
 * - Relationship serialization and persistence
 * - Relationship-aware component access patterns
 * - Cross-relationship queries and analysis
 * 
 * Performance Optimizations:
 * - Cache-friendly relationship storage
 * - Batch relationship updates
 * - Lazy relationship resolution
 * - Memory pool allocation for relationship nodes
 * - Spatial locality optimization for related entities
 * 
 * @author ECScope Educational ECS Framework
 * @date 2024
 */

#include "core/types.hpp"
#include "core/log.hpp"
#include "entity.hpp"
#include "component.hpp"
#include "memory/arena.hpp"
#include "memory/pool.hpp"
#include "memory/memory_tracker.hpp"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>
#include <functional>
#include <optional>
#include <span>
#include <queue>
#include <stack>
#include <algorithm>

namespace ecscope::ecs {

// Forward declarations
class Registry;
class RelationshipManager;

/**
 * @brief Relationship type enumeration
 */
enum class RelationshipType : u8 {
    Hierarchy = 0,          // Parent-child tree relationships
    Reference,              // Direct entity references
    Ownership,              // Strong ownership relationships
    Dependency,             // Entity dependency relationships
    Group,                  // Group membership relationships
    Spatial,                // Spatial/positional relationships
    Temporal,               // Time-based relationships
    Custom                  // User-defined custom relationships
};

/**
 * @brief Relationship direction for queries
 */
enum class RelationshipDirection : u8 {
    Forward = 0,            // From source to target
    Backward,               // From target to source
    Bidirectional           // Both directions
};

/**
 * @brief Relationship traversal mode
 */
enum class TraversalMode : u8 {
    DepthFirst = 0,         // Depth-first traversal
    BreadthFirst,           // Breadth-first traversal
    PreOrder,               // Pre-order (parent before children)
    PostOrder,              // Post-order (children before parent)
    LevelOrder              // Level-by-level traversal
};

/**
 * @brief Relationship constraint types for validation
 */
enum class RelationshipConstraint : u8 {
    None = 0,               // No constraints
    Unique,                 // Unique relationship (1:1)
    SingleParent,           // Entity can have only one parent
    NoSelfReference,        // Cannot reference itself
    NoCycles,               // No circular relationships
    MaxChildren,            // Maximum number of children
    RequiredComponents      // Required components for relationship
};

/**
 * @brief Relationship node storing entity connections
 */
struct RelationshipNode {
    Entity entity;                          // Entity this node represents
    RelationshipType type;                  // Type of relationship
    
    // Hierarchy relationships
    Entity parent;                          // Parent entity (invalid if root)
    std::vector<Entity> children;           // Child entities
    u32 hierarchy_level;                    // Depth in hierarchy (0 = root)
    
    // Reference relationships
    std::vector<Entity> references;         // Entities this entity references
    std::vector<Entity> referenced_by;      // Entities that reference this entity
    
    // Ownership relationships
    Entity owner;                           // Entity that owns this entity
    std::vector<Entity> owned_entities;     // Entities owned by this entity
    
    // Group relationships
    std::vector<Entity> group_memberships;  // Groups this entity belongs to
    std::vector<Entity> group_members;      // Members if this is a group entity
    
    // Metadata
    f64 creation_time;                      // When relationship was created
    f64 last_modified_time;                 // Last modification time
    u32 version;                            // Version for change detection
    bool is_dirty;                          // Needs update/validation
    
    // Custom data
    std::unordered_map<std::string, std::any> custom_data;
    
    RelationshipNode() noexcept
        : entity(Entity::invalid())
        , type(RelationshipType::Hierarchy)
        , parent(Entity::invalid())
        , hierarchy_level(0)
        , owner(Entity::invalid())
        , creation_time(0.0)
        , last_modified_time(0.0)
        , version(0)
        , is_dirty(false) {}
    
    explicit RelationshipNode(Entity e, RelationshipType t = RelationshipType::Hierarchy) noexcept
        : entity(e)
        , type(t)
        , parent(Entity::invalid())
        , hierarchy_level(0)
        , owner(Entity::invalid())
        , creation_time(0.0)
        , last_modified_time(0.0)
        , version(0)
        , is_dirty(false) {}
    
    // Check if entity has any relationships
    bool has_relationships() const noexcept {
        return parent.is_valid() || !children.empty() || 
               !references.empty() || !referenced_by.empty() ||
               owner.is_valid() || !owned_entities.empty() ||
               !group_memberships.empty() || !group_members.empty();
    }
    
    // Get total relationship count
    usize relationship_count() const noexcept {
        usize count = 0;
        if (parent.is_valid()) count++;
        count += children.size();
        count += references.size();
        count += referenced_by.size();
        if (owner.is_valid()) count++;
        count += owned_entities.size();
        count += group_memberships.size();
        count += group_members.size();
        return count;
    }
};

/**
 * @brief Relationship query for finding related entities
 */
struct RelationshipQuery {
    Entity source_entity;                   // Starting entity
    RelationshipType type;                  // Type of relationship to query
    RelationshipDirection direction;        // Query direction
    TraversalMode traversal;                // How to traverse relationships
    u32 max_depth;                         // Maximum traversal depth
    bool include_source;                    // Include source entity in results
    
    // Filters
    std::vector<ComponentSignature> required_components;  // Required components
    std::vector<ComponentSignature> forbidden_components; // Forbidden components
    std::function<bool(Entity)> custom_filter;           // Custom filter function
    
    RelationshipQuery(Entity entity, RelationshipType rel_type = RelationshipType::Hierarchy)
        : source_entity(entity)
        , type(rel_type)
        , direction(RelationshipDirection::Forward)
        , traversal(TraversalMode::BreadthFirst)
        , max_depth(std::numeric_limits<u32>::max())
        , include_source(false) {}
};

/**
 * @brief Relationship query results
 */
struct RelationshipQueryResult {
    std::vector<Entity> entities;           // Found entities
    std::vector<u32> depths;               // Depth of each entity
    std::vector<Entity> parents;           // Parent of each entity in traversal
    std::unordered_map<Entity, RelationshipNode*> nodes; // Node information
    
    f64 query_time;                        // Time taken to execute query
    usize nodes_visited;                   // Number of nodes visited
    usize total_relationships;             // Total relationships examined
    
    RelationshipQueryResult() noexcept
        : query_time(0.0), nodes_visited(0), total_relationships(0) {}
    
    bool empty() const noexcept { return entities.empty(); }
    usize size() const noexcept { return entities.size(); }
    
    // Get entities at specific depth
    std::vector<Entity> entities_at_depth(u32 depth) const {
        std::vector<Entity> result;
        for (usize i = 0; i < entities.size(); ++i) {
            if (depths[i] == depth) {
                result.push_back(entities[i]);
            }
        }
        return result;
    }
    
    // Get maximum depth in results
    u32 max_depth() const noexcept {
        return depths.empty() ? 0 : *std::max_element(depths.begin(), depths.end());
    }
};

/**
 * @brief Relationship statistics for analysis and debugging
 */
struct RelationshipStats {
    // Entity counts
    usize total_entities;                   // Total entities with relationships
    usize root_entities;                    // Entities without parents
    usize leaf_entities;                    // Entities without children
    usize intermediate_entities;            // Entities with both parent and children
    
    // Relationship counts by type
    std::array<usize, 8> relationships_by_type; // Counts per relationship type
    usize total_relationships;              // Total relationship count
    
    // Hierarchy statistics
    u32 max_hierarchy_depth;               // Maximum hierarchy depth
    f64 average_hierarchy_depth;           // Average depth of entities
    usize orphaned_entities;               // Entities with invalid parent references
    usize circular_references;             // Circular reference count
    
    // Performance metrics
    f64 total_query_time;                  // Total time spent in queries
    u64 total_queries;                     // Total queries executed
    f64 average_query_time;                // Average query execution time
    usize cache_hits;                      // Relationship cache hits
    usize cache_misses;                    // Relationship cache misses
    
    // Memory usage
    usize memory_used;                     // Memory used by relationships
    usize nodes_allocated;                 // Number of relationship nodes
    usize average_node_size;               // Average size per node
    
    RelationshipStats() noexcept { reset(); }
    
    void reset() noexcept {
        total_entities = 0;
        root_entities = 0;
        leaf_entities = 0;
        intermediate_entities = 0;
        relationships_by_type.fill(0);
        total_relationships = 0;
        max_hierarchy_depth = 0;
        average_hierarchy_depth = 0.0;
        orphaned_entities = 0;
        circular_references = 0;
        total_query_time = 0.0;
        total_queries = 0;
        average_query_time = 0.0;
        cache_hits = 0;
        cache_misses = 0;
        memory_used = 0;
        nodes_allocated = 0;
        average_node_size = 0;
    }
    
    void update_averages() noexcept {
        if (total_queries > 0) {
            average_query_time = total_query_time / total_queries;
        }
        
        if (nodes_allocated > 0) {
            average_node_size = memory_used / nodes_allocated;
        }
        
        if (total_entities > 0) {
            // Calculate average hierarchy depth
            // This would be computed during relationship validation
        }
    }
};

/**
 * @brief Relationship validation results
 */
struct ValidationResult {
    bool is_valid;                          // Overall validation result
    std::vector<std::string> errors;        // Validation errors
    std::vector<std::string> warnings;      // Validation warnings
    
    // Specific issues
    std::vector<Entity> orphaned_entities;  // Entities with invalid parents
    std::vector<Entity> circular_refs;      // Entities in circular references
    std::vector<Entity> constraint_violations; // Entities violating constraints
    
    f64 validation_time;                    // Time taken for validation
    
    ValidationResult() noexcept : is_valid(true), validation_time(0.0) {}
    
    void add_error(const std::string& error) {
        errors.push_back(error);
        is_valid = false;
    }
    
    void add_warning(const std::string& warning) {
        warnings.push_back(warning);
    }
};

/**
 * @brief Relationship change event for notifications
 */
struct RelationshipChangeEvent {
    enum class ChangeType {
        Added,                              // Relationship added
        Removed,                           // Relationship removed
        Modified,                          // Relationship properties changed
        Hierarchy_Changed,                 // Hierarchy structure changed
        Owner_Changed                      // Ownership changed
    };
    
    ChangeType change_type;
    Entity source_entity;
    Entity target_entity;
    RelationshipType relationship_type;
    f64 timestamp;
    std::any old_value;                    // Previous value (if applicable)
    std::any new_value;                    // New value (if applicable)
};

/**
 * @brief Advanced relationship manager with educational features
 */
class RelationshipManager {
private:
    // Storage
    std::unordered_map<Entity, std::unique_ptr<RelationshipNode>> nodes_;
    mutable std::shared_mutex nodes_mutex_;
    
    // Memory management
    std::unique_ptr<memory::ArenaAllocator> relationships_arena_;
    std::unique_ptr<memory::PoolAllocator> node_pool_;
    u32 allocator_id_;
    
    // Caching
    mutable std::unordered_map<u64, RelationshipQueryResult> query_cache_;
    mutable std::mutex cache_mutex_;
    usize max_cached_queries_;
    f64 cache_timeout_;
    
    // Statistics
    mutable RelationshipStats stats_;
    std::vector<std::function<void(const RelationshipChangeEvent&)>> change_listeners_;
    
    // Configuration
    bool enable_validation_;
    bool enable_change_events_;
    bool enable_caching_;
    usize max_hierarchy_depth_;
    
public:
    explicit RelationshipManager(usize arena_size = 2 * 1024 * 1024,  // 2MB
                                usize node_pool_size = 10000);
    ~RelationshipManager();
    
    // Hierarchy relationships
    bool set_parent(Entity child, Entity parent);
    bool remove_parent(Entity child);
    Entity get_parent(Entity entity) const;
    std::vector<Entity> get_children(Entity parent) const;
    std::vector<Entity> get_siblings(Entity entity) const;
    std::vector<Entity> get_ancestors(Entity entity) const;
    std::vector<Entity> get_descendants(Entity entity) const;
    std::vector<Entity> get_root_entities() const;
    
    // Reference relationships
    bool add_reference(Entity from, Entity to);
    bool remove_reference(Entity from, Entity to);
    std::vector<Entity> get_references(Entity entity) const;
    std::vector<Entity> get_referenced_by(Entity entity) const;
    bool has_reference(Entity from, Entity to) const;
    
    // Ownership relationships
    bool set_owner(Entity owned, Entity owner);
    bool remove_owner(Entity owned);
    Entity get_owner(Entity entity) const;
    std::vector<Entity> get_owned_entities(Entity owner) const;
    std::vector<Entity> get_ownership_chain(Entity entity) const;
    
    // Group relationships
    bool add_to_group(Entity entity, Entity group);
    bool remove_from_group(Entity entity, Entity group);
    std::vector<Entity> get_group_memberships(Entity entity) const;
    std::vector<Entity> get_group_members(Entity group) const;
    bool is_member_of_group(Entity entity, Entity group) const;
    
    // Complex queries
    RelationshipQueryResult query_relationships(const RelationshipQuery& query) const;
    std::vector<Entity> find_entities_with_relationship(RelationshipType type,
                                                       RelationshipDirection direction = RelationshipDirection::Forward) const;
    std::vector<Entity> find_path_between(Entity from, Entity to, RelationshipType type = RelationshipType::Hierarchy) const;
    std::vector<Entity> find_common_ancestors(Entity entity1, Entity entity2) const;
    Entity find_lowest_common_ancestor(Entity entity1, Entity entity2) const;
    
    // Traversal utilities
    void traverse_depth_first(Entity root, const std::function<bool(Entity, u32)>& visitor) const;
    void traverse_breadth_first(Entity root, const std::function<bool(Entity, u32)>& visitor) const;
    void traverse_pre_order(Entity root, const std::function<bool(Entity, u32)>& visitor) const;
    void traverse_post_order(Entity root, const std::function<bool(Entity, u32)>& visitor) const;
    
    // Bulk operations
    void set_parents_bulk(const std::vector<std::pair<Entity, Entity>>& parent_child_pairs);
    void add_references_bulk(const std::vector<std::pair<Entity, Entity>>& reference_pairs);
    void remove_entity_relationships(Entity entity);
    void clear_all_relationships();
    
    // Validation and integrity
    ValidationResult validate_relationships() const;
    ValidationResult validate_entity_relationships(Entity entity) const;
    bool check_circular_references(Entity entity, RelationshipType type = RelationshipType::Hierarchy) const;
    void repair_broken_relationships();
    
    // Information and statistics
    const RelationshipStats& statistics() const;
    void update_statistics() const;
    bool has_relationships(Entity entity) const;
    usize get_relationship_count(Entity entity) const;
    u32 get_hierarchy_depth(Entity entity) const;
    usize get_subtree_size(Entity root) const;
    
    // Configuration
    void set_enable_validation(bool enable) { enable_validation_ = enable; }
    void set_enable_change_events(bool enable) { enable_change_events_ = enable; }
    void set_enable_caching(bool enable) { enable_caching_ = enable; }
    void set_max_hierarchy_depth(usize max_depth) { max_hierarchy_depth_ = max_depth; }
    void set_cache_settings(usize max_cached, f64 timeout) { 
        max_cached_queries_ = max_cached; 
        cache_timeout_ = timeout; 
    }
    
    // Event system
    void add_change_listener(const std::function<void(const RelationshipChangeEvent&)>& listener);
    void remove_all_change_listeners() { change_listeners_.clear(); }
    
    // Memory management
    usize get_memory_usage() const;
    void compact_memory();
    void optimize_storage();
    
    // Debugging and visualization
    std::string generate_hierarchy_report() const;
    std::string generate_relationship_graph_dot() const;  // Graphviz DOT format
    void print_entity_relationships(Entity entity) const;
    void print_hierarchy_tree(Entity root = Entity::invalid()) const;
    
    // Serialization support
    struct SerializedRelationships {
        std::vector<Entity> entities;
        std::vector<Entity> parents;
        std::vector<std::vector<Entity>> references;
        std::vector<Entity> owners;
        std::vector<std::vector<Entity>> group_memberships;
    };
    
    SerializedRelationships serialize() const;
    void deserialize(const SerializedRelationships& data);
    
private:
    // Internal utilities
    RelationshipNode* get_or_create_node(Entity entity);
    const RelationshipNode* get_node(Entity entity) const;
    void notify_relationship_change(const RelationshipChangeEvent& event);
    u64 hash_query(const RelationshipQuery& query) const;
    void invalidate_query_cache();
    void cleanup_expired_cache_entries() const;
    
    // Validation helpers
    bool validate_hierarchy_constraints(Entity child, Entity parent) const;
    bool validate_reference_constraints(Entity from, Entity to) const;
    bool validate_ownership_constraints(Entity owned, Entity owner) const;
    
    // Traversal implementation
    void depth_first_impl(Entity entity, const std::function<bool(Entity, u32)>& visitor,
                         u32 depth, std::unordered_set<Entity>& visited) const;
    void breadth_first_impl(Entity root, const std::function<bool(Entity, u32)>& visitor) const;
    void pre_order_impl(Entity entity, const std::function<bool(Entity, u32)>& visitor,
                       u32 depth, std::unordered_set<Entity>& visited) const;
    void post_order_impl(Entity entity, const std::function<bool(Entity, u32)>& visitor,
                        u32 depth, std::unordered_set<Entity>& visited) const;
    
    // Memory tracking
    void track_node_allocation(RelationshipNode* node);
    void track_node_deallocation(RelationshipNode* node);
    
    static u32 next_allocator_id();
    static std::atomic<u32> allocator_id_counter_;
};

/**
 * @brief RAII helper for relationship transactions
 */
class RelationshipTransaction {
private:
    RelationshipManager& manager_;
    bool committed_;
    std::vector<std::function<void()>> rollback_operations_;
    
public:
    explicit RelationshipTransaction(RelationshipManager& manager)
        : manager_(manager), committed_(false) {}
    
    ~RelationshipTransaction() {
        if (!committed_) {
            rollback();
        }
    }
    
    // Relationship operations (with rollback support)
    bool set_parent(Entity child, Entity parent);
    bool add_reference(Entity from, Entity to);
    bool set_owner(Entity owned, Entity owner);
    bool add_to_group(Entity entity, Entity group);
    
    void commit() { committed_ = true; }
    void rollback();
    
private:
    void add_rollback_operation(std::function<void()> operation);
};

// Utility functions for common relationship patterns
namespace relationships {
    
    /**
     * @brief Create a simple hierarchy from a list of entities
     */
    void create_linear_hierarchy(RelationshipManager& manager, 
                                const std::vector<Entity>& entities);
    
    /**
     * @brief Create a tree hierarchy from parent-child pairs
     */
    void create_tree_hierarchy(RelationshipManager& manager,
                              const std::vector<std::pair<Entity, Entity>>& pairs);
    
    /**
     * @brief Find all entities at a specific depth in hierarchy
     */
    std::vector<Entity> get_entities_at_depth(RelationshipManager& manager,
                                            Entity root, u32 depth);
    
    /**
     * @brief Get hierarchy statistics for analysis
     */
    struct HierarchyInfo {
        u32 depth;
        usize node_count;
        usize leaf_count;
        f64 balance_factor;  // How balanced the tree is
    };
    
    HierarchyInfo analyze_hierarchy(RelationshipManager& manager, Entity root);
    
    /**
     * @brief Flatten hierarchy into a linear list
     */
    std::vector<Entity> flatten_hierarchy(RelationshipManager& manager, Entity root,
                                        TraversalMode mode = TraversalMode::BreadthFirst);
    
    /**
     * @brief Check if entity is ancestor of another
     */
    bool is_ancestor_of(RelationshipManager& manager, Entity ancestor, Entity descendant);
    
    /**
     * @brief Get distance between two entities in hierarchy
     */
    std::optional<u32> get_hierarchy_distance(RelationshipManager& manager,
                                            Entity entity1, Entity entity2);
}

// Component for storing relationship metadata on entities
struct RelationshipComponent : public ComponentBase {
    Entity primary_parent;              // Primary parent for hierarchy
    u32 hierarchy_level;               // Level in hierarchy
    u32 child_count;                   // Number of children
    u32 reference_count;               // Number of references
    bool is_group;                     // Whether entity is a group
    f64 last_relationship_change;      // Last modification time
    
    RelationshipComponent() noexcept
        : primary_parent(Entity::invalid())
        , hierarchy_level(0)
        , child_count(0)
        , reference_count(0)
        , is_group(false)
        , last_relationship_change(0.0) {}
};

} // namespace ecscope::ecs