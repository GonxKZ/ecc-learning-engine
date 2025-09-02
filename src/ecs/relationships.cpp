/**
 * @file relationships.cpp
 * @brief Implementation of the Advanced ECS Component Relationships System
 * 
 * Contains the runtime implementations for relationship management, hierarchy
 * traversal, and relationship queries. This demonstrates advanced graph algorithms
 * and memory-efficient relationship storage patterns.
 */

#include "relationships.hpp"
#include "core/time.hpp"
#include <algorithm>
#include <queue>
#include <stack>
#include <sstream>
#include <iomanip>
#include <fstream>

namespace ecscope::ecs {

// Static member initialization
std::atomic<u32> RelationshipManager::allocator_id_counter_{1};

// RelationshipManager implementation
RelationshipManager::RelationshipManager(usize arena_size, usize node_pool_size)
    : allocator_id_(next_allocator_id())
    , max_cached_queries_(1000)
    , cache_timeout_(10.0)  // 10 seconds
    , enable_validation_(true)
    , enable_change_events_(true)
    , enable_caching_(true)
    , max_hierarchy_depth_(100) {
    
    // Initialize memory allocators
    relationships_arena_ = std::make_unique<memory::ArenaAllocator>(
        arena_size,
        "Relationships_Arena",
        true  // Enable tracking
    );
    
    node_pool_ = std::make_unique<memory::PoolAllocator>(
        sizeof(RelationshipNode),
        node_pool_size,
        "RelationshipNode_Pool",
        true  // Enable tracking
    );
    
    LOG_INFO("RelationshipManager initialized - Arena: {} KB, Node pool: {} nodes",
             arena_size / 1024, node_pool_size);
}

RelationshipManager::~RelationshipManager() {
    LOG_INFO("RelationshipManager destroyed - {} nodes, {} relationships tracked",
             nodes_.size(), stats_.total_relationships);
}

bool RelationshipManager::set_parent(Entity child, Entity parent) {
    if (!child.is_valid() || (!parent.is_valid() && parent != Entity::invalid())) {
        return false;
    }
    
    // Validate constraints
    if (enable_validation_ && !validate_hierarchy_constraints(child, parent)) {
        return false;
    }
    
    std::unique_lock lock(nodes_mutex_);
    
    RelationshipNode* child_node = get_or_create_node(child);
    Entity old_parent = child_node->parent;
    
    // Remove from old parent if exists
    if (old_parent.is_valid()) {
        RelationshipNode* old_parent_node = get_or_create_node(old_parent);
        auto it = std::find(old_parent_node->children.begin(), 
                          old_parent_node->children.end(), child);
        if (it != old_parent_node->children.end()) {
            old_parent_node->children.erase(it);
            old_parent_node->is_dirty = true;
        }
    }
    
    // Set new parent
    child_node->parent = parent;
    child_node->is_dirty = true;
    child_node->last_modified_time = core::get_time_seconds();
    child_node->version++;
    
    // Add to new parent's children
    if (parent.is_valid()) {
        RelationshipNode* parent_node = get_or_create_node(parent);
        
        // Check if already a child
        auto it = std::find(parent_node->children.begin(), 
                          parent_node->children.end(), child);
        if (it == parent_node->children.end()) {
            parent_node->children.push_back(child);
            parent_node->is_dirty = true;
        }
        
        // Update hierarchy level
        child_node->hierarchy_level = parent_node->hierarchy_level + 1;
    } else {
        child_node->hierarchy_level = 0;
    }
    
    // Update hierarchy levels for all descendants
    update_descendant_levels(child);
    
    // Invalidate query cache
    if (enable_caching_) {
        invalidate_query_cache();
    }
    
    // Notify change listeners
    if (enable_change_events_) {
        RelationshipChangeEvent event;
        event.change_type = RelationshipChangeEvent::ChangeType::Hierarchy_Changed;
        event.source_entity = child;
        event.target_entity = parent;
        event.relationship_type = RelationshipType::Hierarchy;
        event.timestamp = core::get_time_seconds();
        event.old_value = old_parent;
        event.new_value = parent;
        
        notify_relationship_change(event);
    }
    
    return true;
}

bool RelationshipManager::remove_parent(Entity child) {
    return set_parent(child, Entity::invalid());
}

Entity RelationshipManager::get_parent(Entity entity) const {
    std::shared_lock lock(nodes_mutex_);
    
    const RelationshipNode* node = get_node(entity);
    return node ? node->parent : Entity::invalid();
}

std::vector<Entity> RelationshipManager::get_children(Entity parent) const {
    std::shared_lock lock(nodes_mutex_);
    
    const RelationshipNode* node = get_node(parent);
    return node ? node->children : std::vector<Entity>{};
}

std::vector<Entity> RelationshipManager::get_siblings(Entity entity) const {
    Entity parent = get_parent(entity);
    if (!parent.is_valid()) {
        return {};
    }
    
    auto siblings = get_children(parent);
    
    // Remove the entity itself from siblings
    auto it = std::find(siblings.begin(), siblings.end(), entity);
    if (it != siblings.end()) {
        siblings.erase(it);
    }
    
    return siblings;
}

std::vector<Entity> RelationshipManager::get_ancestors(Entity entity) const {
    std::vector<Entity> ancestors;
    Entity current = entity;
    
    while (true) {
        Entity parent = get_parent(current);
        if (!parent.is_valid()) {
            break;
        }
        
        ancestors.push_back(parent);
        current = parent;
        
        // Safety check for circular references
        if (ancestors.size() > max_hierarchy_depth_) {
            LOG_ERROR("Circular reference or excessive depth detected for entity {}", 
                     entity.id());
            break;
        }
    }
    
    return ancestors;
}

std::vector<Entity> RelationshipManager::get_descendants(Entity entity) const {
    std::vector<Entity> descendants;
    
    traverse_breadth_first(entity, [&](Entity descendant, u32 depth) {
        if (descendant != entity) {  // Don't include the root entity
            descendants.push_back(descendant);
        }
        return true;  // Continue traversal
    });
    
    return descendants;
}

std::vector<Entity> RelationshipManager::get_root_entities() const {
    std::shared_lock lock(nodes_mutex_);
    std::vector<Entity> roots;
    
    for (const auto& [entity, node] : nodes_) {
        if (!node->parent.is_valid() && !node->children.empty()) {
            roots.push_back(entity);
        }
    }
    
    return roots;
}

bool RelationshipManager::add_reference(Entity from, Entity to) {
    if (!from.is_valid() || !to.is_valid() || from == to) {
        return false;
    }
    
    if (enable_validation_ && !validate_reference_constraints(from, to)) {
        return false;
    }
    
    std::unique_lock lock(nodes_mutex_);
    
    RelationshipNode* from_node = get_or_create_node(from);
    RelationshipNode* to_node = get_or_create_node(to);
    
    // Check if reference already exists
    auto it = std::find(from_node->references.begin(), 
                       from_node->references.end(), to);
    if (it != from_node->references.end()) {
        return true;  // Already exists
    }
    
    // Add reference
    from_node->references.push_back(to);
    to_node->referenced_by.push_back(from);
    
    from_node->is_dirty = true;
    to_node->is_dirty = true;
    from_node->last_modified_time = core::get_time_seconds();
    to_node->last_modified_time = core::get_time_seconds();
    from_node->version++;
    to_node->version++;
    
    // Invalidate query cache
    if (enable_caching_) {
        invalidate_query_cache();
    }
    
    // Notify change listeners
    if (enable_change_events_) {
        RelationshipChangeEvent event;
        event.change_type = RelationshipChangeEvent::ChangeType::Added;
        event.source_entity = from;
        event.target_entity = to;
        event.relationship_type = RelationshipType::Reference;
        event.timestamp = core::get_time_seconds();
        
        notify_relationship_change(event);
    }
    
    return true;
}

bool RelationshipManager::remove_reference(Entity from, Entity to) {
    if (!from.is_valid() || !to.is_valid()) {
        return false;
    }
    
    std::unique_lock lock(nodes_mutex_);
    
    RelationshipNode* from_node = get_or_create_node(from);
    RelationshipNode* to_node = get_or_create_node(to);
    
    // Remove from references
    auto ref_it = std::find(from_node->references.begin(), 
                           from_node->references.end(), to);
    if (ref_it == from_node->references.end()) {
        return false;  // Reference doesn't exist
    }
    
    from_node->references.erase(ref_it);
    
    // Remove from referenced_by
    auto ref_by_it = std::find(to_node->referenced_by.begin(), 
                              to_node->referenced_by.end(), from);
    if (ref_by_it != to_node->referenced_by.end()) {
        to_node->referenced_by.erase(ref_by_it);
    }
    
    from_node->is_dirty = true;
    to_node->is_dirty = true;
    from_node->last_modified_time = core::get_time_seconds();
    to_node->last_modified_time = core::get_time_seconds();
    from_node->version++;
    to_node->version++;
    
    // Invalidate query cache
    if (enable_caching_) {
        invalidate_query_cache();
    }
    
    // Notify change listeners
    if (enable_change_events_) {
        RelationshipChangeEvent event;
        event.change_type = RelationshipChangeEvent::ChangeType::Removed;
        event.source_entity = from;
        event.target_entity = to;
        event.relationship_type = RelationshipType::Reference;
        event.timestamp = core::get_time_seconds();
        
        notify_relationship_change(event);
    }
    
    return true;
}

std::vector<Entity> RelationshipManager::get_references(Entity entity) const {
    std::shared_lock lock(nodes_mutex_);
    
    const RelationshipNode* node = get_node(entity);
    return node ? node->references : std::vector<Entity>{};
}

std::vector<Entity> RelationshipManager::get_referenced_by(Entity entity) const {
    std::shared_lock lock(nodes_mutex_);
    
    const RelationshipNode* node = get_node(entity);
    return node ? node->referenced_by : std::vector<Entity>{};
}

bool RelationshipManager::has_reference(Entity from, Entity to) const {
    auto references = get_references(from);
    return std::find(references.begin(), references.end(), to) != references.end();
}

bool RelationshipManager::set_owner(Entity owned, Entity owner) {
    if (!owned.is_valid() || (!owner.is_valid() && owner != Entity::invalid())) {
        return false;
    }
    
    if (enable_validation_ && !validate_ownership_constraints(owned, owner)) {
        return false;
    }
    
    std::unique_lock lock(nodes_mutex_);
    
    RelationshipNode* owned_node = get_or_create_node(owned);
    Entity old_owner = owned_node->owner;
    
    // Remove from old owner if exists
    if (old_owner.is_valid()) {
        RelationshipNode* old_owner_node = get_or_create_node(old_owner);
        auto it = std::find(old_owner_node->owned_entities.begin(), 
                          old_owner_node->owned_entities.end(), owned);
        if (it != old_owner_node->owned_entities.end()) {
            old_owner_node->owned_entities.erase(it);
            old_owner_node->is_dirty = true;
        }
    }
    
    // Set new owner
    owned_node->owner = owner;
    owned_node->is_dirty = true;
    owned_node->last_modified_time = core::get_time_seconds();
    owned_node->version++;
    
    // Add to new owner's owned entities
    if (owner.is_valid()) {
        RelationshipNode* owner_node = get_or_create_node(owner);
        
        // Check if already owned
        auto it = std::find(owner_node->owned_entities.begin(), 
                          owner_node->owned_entities.end(), owned);
        if (it == owner_node->owned_entities.end()) {
            owner_node->owned_entities.push_back(owned);
            owner_node->is_dirty = true;
        }
    }
    
    // Invalidate query cache
    if (enable_caching_) {
        invalidate_query_cache();
    }
    
    // Notify change listeners
    if (enable_change_events_) {
        RelationshipChangeEvent event;
        event.change_type = RelationshipChangeEvent::ChangeType::Owner_Changed;
        event.source_entity = owned;
        event.target_entity = owner;
        event.relationship_type = RelationshipType::Ownership;
        event.timestamp = core::get_time_seconds();
        event.old_value = old_owner;
        event.new_value = owner;
        
        notify_relationship_change(event);
    }
    
    return true;
}

bool RelationshipManager::remove_owner(Entity owned) {
    return set_owner(owned, Entity::invalid());
}

Entity RelationshipManager::get_owner(Entity entity) const {
    std::shared_lock lock(nodes_mutex_);
    
    const RelationshipNode* node = get_node(entity);
    return node ? node->owner : Entity::invalid();
}

std::vector<Entity> RelationshipManager::get_owned_entities(Entity owner) const {
    std::shared_lock lock(nodes_mutex_);
    
    const RelationshipNode* node = get_node(owner);
    return node ? node->owned_entities : std::vector<Entity>{};
}

std::vector<Entity> RelationshipManager::get_ownership_chain(Entity entity) const {
    std::vector<Entity> chain;
    Entity current = entity;
    
    while (true) {
        Entity owner = get_owner(current);
        if (!owner.is_valid()) {
            break;
        }
        
        chain.push_back(owner);
        current = owner;
        
        // Safety check for circular ownership
        if (chain.size() > max_hierarchy_depth_) {
            LOG_ERROR("Circular ownership or excessive chain detected for entity {}", 
                     entity.id());
            break;
        }
    }
    
    return chain;
}

bool RelationshipManager::add_to_group(Entity entity, Entity group) {
    if (!entity.is_valid() || !group.is_valid() || entity == group) {
        return false;
    }
    
    std::unique_lock lock(nodes_mutex_);
    
    RelationshipNode* entity_node = get_or_create_node(entity);
    RelationshipNode* group_node = get_or_create_node(group);
    
    // Check if already in group
    auto member_it = std::find(entity_node->group_memberships.begin(), 
                              entity_node->group_memberships.end(), group);
    if (member_it != entity_node->group_memberships.end()) {
        return true;  // Already in group
    }
    
    // Add to group
    entity_node->group_memberships.push_back(group);
    group_node->group_members.push_back(entity);
    
    entity_node->is_dirty = true;
    group_node->is_dirty = true;
    entity_node->last_modified_time = core::get_time_seconds();
    group_node->last_modified_time = core::get_time_seconds();
    entity_node->version++;
    group_node->version++;
    
    // Invalidate query cache
    if (enable_caching_) {
        invalidate_query_cache();
    }
    
    // Notify change listeners
    if (enable_change_events_) {
        RelationshipChangeEvent event;
        event.change_type = RelationshipChangeEvent::ChangeType::Added;
        event.source_entity = entity;
        event.target_entity = group;
        event.relationship_type = RelationshipType::Group;
        event.timestamp = core::get_time_seconds();
        
        notify_relationship_change(event);
    }
    
    return true;
}

bool RelationshipManager::remove_from_group(Entity entity, Entity group) {
    if (!entity.is_valid() || !group.is_valid()) {
        return false;
    }
    
    std::unique_lock lock(nodes_mutex_);
    
    RelationshipNode* entity_node = get_or_create_node(entity);
    RelationshipNode* group_node = get_or_create_node(group);
    
    // Remove from entity's memberships
    auto member_it = std::find(entity_node->group_memberships.begin(), 
                              entity_node->group_memberships.end(), group);
    if (member_it == entity_node->group_memberships.end()) {
        return false;  // Not in group
    }
    
    entity_node->group_memberships.erase(member_it);
    
    // Remove from group's members
    auto group_it = std::find(group_node->group_members.begin(), 
                             group_node->group_members.end(), entity);
    if (group_it != group_node->group_members.end()) {
        group_node->group_members.erase(group_it);
    }
    
    entity_node->is_dirty = true;
    group_node->is_dirty = true;
    entity_node->last_modified_time = core::get_time_seconds();
    group_node->last_modified_time = core::get_time_seconds();
    entity_node->version++;
    group_node->version++;
    
    // Invalidate query cache
    if (enable_caching_) {
        invalidate_query_cache();
    }
    
    // Notify change listeners
    if (enable_change_events_) {
        RelationshipChangeEvent event;
        event.change_type = RelationshipChangeEvent::ChangeType::Removed;
        event.source_entity = entity;
        event.target_entity = group;
        event.relationship_type = RelationshipType::Group;
        event.timestamp = core::get_time_seconds();
        
        notify_relationship_change(event);
    }
    
    return true;
}

std::vector<Entity> RelationshipManager::get_group_memberships(Entity entity) const {
    std::shared_lock lock(nodes_mutex_);
    
    const RelationshipNode* node = get_node(entity);
    return node ? node->group_memberships : std::vector<Entity>{};
}

std::vector<Entity> RelationshipManager::get_group_members(Entity group) const {
    std::shared_lock lock(nodes_mutex_);
    
    const RelationshipNode* node = get_node(group);
    return node ? node->group_members : std::vector<Entity>{};
}

bool RelationshipManager::is_member_of_group(Entity entity, Entity group) const {
    auto memberships = get_group_memberships(entity);
    return std::find(memberships.begin(), memberships.end(), group) != memberships.end();
}

RelationshipQueryResult RelationshipManager::query_relationships(const RelationshipQuery& query) const {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Check cache first
    if (enable_caching_) {
        u64 query_hash = hash_query(query);
        std::lock_guard cache_lock(cache_mutex_);
        
        auto cache_it = query_cache_.find(query_hash);
        if (cache_it != query_cache_.end()) {
            auto& cached_result = cache_it->second;
            
            // Check if cache is still valid
            auto end_time = std::chrono::high_resolution_clock::now();
            f64 current_time = std::chrono::duration<f64>(end_time.time_since_epoch()).count();
            
            if (current_time - cached_result.query_time < cache_timeout_) {
                // Update access statistics
                const_cast<RelationshipStats&>(stats_).cache_hits++;
                return cached_result;
            } else {
                // Cache expired, remove it
                query_cache_.erase(cache_it);
            }
        }
        
        const_cast<RelationshipStats&>(stats_).cache_misses++;
    }
    
    // Execute query
    RelationshipQueryResult result;
    std::unordered_set<Entity> visited;
    std::queue<std::pair<Entity, u32>> queue;  // entity, depth
    
    queue.push({query.source_entity, 0});
    if (query.include_source) {
        result.entities.push_back(query.source_entity);
        result.depths.push_back(0);
        result.parents.push_back(Entity::invalid());
    }
    
    while (!queue.empty() && result.entities.size() < 10000) {  // Safety limit
        auto [current_entity, depth] = queue.front();
        queue.pop();
        
        if (depth >= query.max_depth || visited.find(current_entity) != visited.end()) {
            continue;
        }
        
        visited.insert(current_entity);
        result.nodes_visited++;
        
        // Get related entities based on query type and direction
        std::vector<Entity> related_entities;
        
        switch (query.type) {
            case RelationshipType::Hierarchy:
                if (query.direction == RelationshipDirection::Forward) {
                    related_entities = get_children(current_entity);
                } else if (query.direction == RelationshipDirection::Backward) {
                    Entity parent = get_parent(current_entity);
                    if (parent.is_valid()) {
                        related_entities.push_back(parent);
                    }
                }
                break;
                
            case RelationshipType::Reference:
                if (query.direction == RelationshipDirection::Forward) {
                    related_entities = get_references(current_entity);
                } else if (query.direction == RelationshipDirection::Backward) {
                    related_entities = get_referenced_by(current_entity);
                }
                break;
                
            case RelationshipType::Ownership:
                if (query.direction == RelationshipDirection::Forward) {
                    related_entities = get_owned_entities(current_entity);
                } else if (query.direction == RelationshipDirection::Backward) {
                    Entity owner = get_owner(current_entity);
                    if (owner.is_valid()) {
                        related_entities.push_back(owner);
                    }
                }
                break;
                
            case RelationshipType::Group:
                if (query.direction == RelationshipDirection::Forward) {
                    related_entities = get_group_members(current_entity);
                } else if (query.direction == RelationshipDirection::Backward) {
                    related_entities = get_group_memberships(current_entity);
                }
                break;
                
            default:
                break;
        }
        
        result.total_relationships += related_entities.size();
        
        // Apply filters and add to results
        for (Entity related : related_entities) {
            if (visited.find(related) != visited.end()) {
                continue;
            }
            
            // Apply custom filter if provided
            if (query.custom_filter && !query.custom_filter(related)) {
                continue;
            }
            
            // Add to results
            if (depth + 1 <= query.max_depth) {
                if (!query.include_source || related != query.source_entity) {
                    result.entities.push_back(related);
                    result.depths.push_back(depth + 1);
                    result.parents.push_back(current_entity);
                    
                    // Add node information
                    result.nodes[related] = const_cast<RelationshipNode*>(get_node(related));
                }
                
                // Add to queue for further traversal
                queue.push({related, depth + 1});
            }
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    result.query_time = std::chrono::duration<f64>(end_time - start_time).count();
    
    // Update statistics
    const_cast<RelationshipStats&>(stats_).total_query_time += result.query_time;
    const_cast<RelationshipStats&>(stats_).total_queries++;
    
    // Cache result if caching is enabled
    if (enable_caching_ && result.entities.size() <= 1000) {  // Don't cache very large results
        u64 query_hash = hash_query(query);
        std::lock_guard cache_lock(cache_mutex_);
        
        if (query_cache_.size() >= max_cached_queries_) {
            // Remove oldest entry
            auto oldest_it = std::min_element(query_cache_.begin(), query_cache_.end(),
                [](const auto& a, const auto& b) {
                    return a.second.query_time < b.second.query_time;
                });
            if (oldest_it != query_cache_.end()) {
                query_cache_.erase(oldest_it);
            }
        }
        
        query_cache_[query_hash] = result;
    }
    
    return result;
}

void RelationshipManager::traverse_breadth_first(Entity root, 
                                                const std::function<bool(Entity, u32)>& visitor) const {
    breadth_first_impl(root, visitor);
}

void RelationshipManager::traverse_depth_first(Entity root, 
                                              const std::function<bool(Entity, u32)>& visitor) const {
    std::unordered_set<Entity> visited;
    depth_first_impl(root, visitor, 0, visited);
}

ValidationResult RelationshipManager::validate_relationships() const {
    auto start_time = std::chrono::high_resolution_clock::now();
    ValidationResult result;
    
    std::shared_lock lock(nodes_mutex_);
    
    // Check for orphaned entities (invalid parent references)
    for (const auto& [entity, node] : nodes_) {
        if (node->parent.is_valid()) {
            const RelationshipNode* parent_node = get_node(node->parent);
            if (!parent_node) {
                result.orphaned_entities.push_back(entity);
                result.add_error("Entity " + std::to_string(entity.id()) + " has invalid parent reference");
            }
        }
    }
    
    // Check for circular references in hierarchy
    std::unordered_set<Entity> visited;
    std::unordered_set<Entity> recursion_stack;
    
    for (const auto& [entity, node] : nodes_) {
        if (visited.find(entity) == visited.end()) {
            if (check_circular_references_impl(entity, visited, recursion_stack)) {
                result.circular_refs.push_back(entity);
                result.add_error("Circular reference detected involving entity " + std::to_string(entity.id()));
            }
        }
    }
    
    // Check hierarchy depth constraints
    for (const auto& [entity, node] : nodes_) {
        if (node->hierarchy_level > max_hierarchy_depth_) {
            result.constraint_violations.push_back(entity);
            result.add_warning("Entity " + std::to_string(entity.id()) + 
                             " exceeds maximum hierarchy depth (" + 
                             std::to_string(max_hierarchy_depth_) + ")");
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    result.validation_time = std::chrono::duration<f64>(end_time - start_time).count();
    
    return result;
}

bool RelationshipManager::check_circular_references(Entity entity, RelationshipType type) const {
    std::unordered_set<Entity> visited;
    return check_circular_references_recursive(entity, entity, type, visited);
}

const RelationshipStats& RelationshipManager::statistics() const {
    update_statistics();
    return stats_;
}

void RelationshipManager::update_statistics() const {
    auto& mutable_stats = const_cast<RelationshipStats&>(stats_);
    
    std::shared_lock lock(nodes_mutex_);
    
    mutable_stats.total_entities = nodes_.size();
    mutable_stats.total_relationships = 0;
    mutable_stats.root_entities = 0;
    mutable_stats.leaf_entities = 0;
    mutable_stats.intermediate_entities = 0;
    mutable_stats.max_hierarchy_depth = 0;
    mutable_stats.orphaned_entities = 0;
    
    for (const auto& [entity, node] : nodes_) {
        // Count relationships
        mutable_stats.total_relationships += node->relationship_count();
        
        // Classify entities
        bool has_parent = node->parent.is_valid();
        bool has_children = !node->children.empty();
        
        if (!has_parent && has_children) {
            mutable_stats.root_entities++;
        } else if (has_parent && !has_children) {
            mutable_stats.leaf_entities++;
        } else if (has_parent && has_children) {
            mutable_stats.intermediate_entities++;
        }
        
        // Track maximum hierarchy depth
        if (node->hierarchy_level > mutable_stats.max_hierarchy_depth) {
            mutable_stats.max_hierarchy_depth = node->hierarchy_level;
        }
        
        // Check for orphaned entities
        if (has_parent) {
            const RelationshipNode* parent_node = get_node(node->parent);
            if (!parent_node) {
                mutable_stats.orphaned_entities++;
            }
        }
    }
    
    // Calculate memory usage
    mutable_stats.memory_used = relationships_arena_->used_size() + 
                               node_pool_->allocated_size();
    mutable_stats.nodes_allocated = nodes_.size();
    
    mutable_stats.update_averages();
}

usize RelationshipManager::get_memory_usage() const {
    return relationships_arena_->used_size() + 
           node_pool_->allocated_size() + 
           sizeof(*this) +
           nodes_.size() * sizeof(std::pair<Entity, std::unique_ptr<RelationshipNode>>) +
           query_cache_.size() * sizeof(std::pair<u64, RelationshipQueryResult>);
}

void RelationshipManager::compact_memory() {
    // Reset arena allocator (this invalidates all arena-allocated data)
    relationships_arena_->reset();
    
    // Shrink node pool
    node_pool_->shrink_pool();
    
    // Clear expired cache entries
    cleanup_expired_cache_entries();
    
    LOG_INFO("RelationshipManager memory compacted - {} KB freed", 
             (relationships_arena_->total_size() - relationships_arena_->used_size()) / 1024);
}

std::string RelationshipManager::generate_hierarchy_report() const {
    std::ostringstream report;
    
    report << "=== Relationship Manager Hierarchy Report ===\n";
    
    auto stats = statistics();
    report << "Total Entities: " << stats.total_entities << "\n";
    report << "Root Entities: " << stats.root_entities << "\n";
    report << "Leaf Entities: " << stats.leaf_entities << "\n";
    report << "Intermediate Entities: " << stats.intermediate_entities << "\n";
    report << "Max Hierarchy Depth: " << stats.max_hierarchy_depth << "\n";
    report << "Total Relationships: " << stats.total_relationships << "\n";
    report << "Memory Usage: " << (get_memory_usage() / 1024) << " KB\n";
    
    if (stats.orphaned_entities > 0 || stats.circular_references > 0) {
        report << "\n=== Issues ===\n";
        if (stats.orphaned_entities > 0) {
            report << "Orphaned Entities: " << stats.orphaned_entities << "\n";
        }
        if (stats.circular_references > 0) {
            report << "Circular References: " << stats.circular_references << "\n";
        }
    }
    
    report << "\n=== Root Hierarchies ===\n";
    auto roots = get_root_entities();
    for (Entity root : roots) {
        report << "Root Entity " << root.id() << ":\n";
        print_hierarchy_subtree(root, report, 1);
    }
    
    return report.str();
}

void RelationshipManager::print_entity_relationships(Entity entity) const {
    std::shared_lock lock(nodes_mutex_);
    
    const RelationshipNode* node = get_node(entity);
    if (!node) {
        LOG_INFO("Entity {} has no relationships", entity.id());
        return;
    }
    
    LOG_INFO("Entity {} relationships:", entity.id());
    
    if (node->parent.is_valid()) {
        LOG_INFO("  Parent: {}", node->parent.id());
    }
    
    if (!node->children.empty()) {
        std::string children_str;
        for (Entity child : node->children) {
            if (!children_str.empty()) children_str += ", ";
            children_str += std::to_string(child.id());
        }
        LOG_INFO("  Children: [{}]", children_str);
    }
    
    if (!node->references.empty()) {
        std::string refs_str;
        for (Entity ref : node->references) {
            if (!refs_str.empty()) refs_str += ", ";
            refs_str += std::to_string(ref.id());
        }
        LOG_INFO("  References: [{}]", refs_str);
    }
    
    if (!node->referenced_by.empty()) {
        std::string ref_by_str;
        for (Entity ref : node->referenced_by) {
            if (!ref_by_str.empty()) ref_by_str += ", ";
            ref_by_str += std::to_string(ref.id());
        }
        LOG_INFO("  Referenced by: [{}]", ref_by_str);
    }
    
    if (node->owner.is_valid()) {
        LOG_INFO("  Owner: {}", node->owner.id());
    }
    
    if (!node->owned_entities.empty()) {
        std::string owned_str;
        for (Entity owned : node->owned_entities) {
            if (!owned_str.empty()) owned_str += ", ";
            owned_str += std::to_string(owned.id());
        }
        LOG_INFO("  Owned: [{}]", owned_str);
    }
    
    LOG_INFO("  Hierarchy Level: {}", node->hierarchy_level);
    LOG_INFO("  Total Relationships: {}", node->relationship_count());
}

// Private helper methods
RelationshipNode* RelationshipManager::get_or_create_node(Entity entity) {
    auto it = nodes_.find(entity);
    if (it != nodes_.end()) {
        return it->second.get();
    }
    
    // Create new node
    auto node = std::make_unique<RelationshipNode>(entity);
    node->creation_time = core::get_time_seconds();
    node->last_modified_time = node->creation_time;
    
    RelationshipNode* node_ptr = node.get();
    nodes_[entity] = std::move(node);
    
    // Track allocation
    track_node_allocation(node_ptr);
    
    return node_ptr;
}

const RelationshipNode* RelationshipManager::get_node(Entity entity) const {
    auto it = nodes_.find(entity);
    return it != nodes_.end() ? it->second.get() : nullptr;
}

void RelationshipManager::notify_relationship_change(const RelationshipChangeEvent& event) {
    for (const auto& listener : change_listeners_) {
        try {
            listener(event);
        } catch (const std::exception& e) {
            LOG_ERROR("Exception in relationship change listener: {}", e.what());
        }
    }
}

u64 RelationshipManager::hash_query(const RelationshipQuery& query) const {
    // Simple hash combining query parameters
    std::hash<u32> hasher;
    u64 hash = hasher(query.source_entity.id());
    hash ^= hasher(static_cast<u32>(query.type)) << 1;
    hash ^= hasher(static_cast<u32>(query.direction)) << 2;
    hash ^= hasher(static_cast<u32>(query.traversal)) << 3;
    hash ^= hasher(query.max_depth) << 4;
    
    return hash;
}

void RelationshipManager::invalidate_query_cache() {
    std::lock_guard lock(cache_mutex_);
    query_cache_.clear();
}

bool RelationshipManager::validate_hierarchy_constraints(Entity child, Entity parent) const {
    if (child == parent) {
        return false;  // Cannot be parent of itself
    }
    
    if (parent.is_valid()) {
        // Check if this would create a circular reference
        if (check_circular_references_recursive(parent, child, RelationshipType::Hierarchy, {})) {
            return false;
        }
        
        // Check depth constraints
        const RelationshipNode* parent_node = get_node(parent);
        if (parent_node && parent_node->hierarchy_level >= max_hierarchy_depth_ - 1) {
            return false;
        }
    }
    
    return true;
}

bool RelationshipManager::check_circular_references_recursive(Entity current, Entity target,
                                                            RelationshipType type,
                                                            std::unordered_set<Entity>& visited) const {
    if (current == target && !visited.empty()) {
        return true;  // Found cycle
    }
    
    if (visited.find(current) != visited.end()) {
        return false;  // Already visited, no cycle through this path
    }
    
    visited.insert(current);
    
    const RelationshipNode* node = get_node(current);
    if (!node) {
        return false;
    }
    
    // Check relationships based on type
    switch (type) {
        case RelationshipType::Hierarchy:
            if (node->parent.is_valid()) {
                if (check_circular_references_recursive(node->parent, target, type, visited)) {
                    return true;
                }
            }
            break;
            
        case RelationshipType::Reference:
            for (Entity ref : node->references) {
                if (check_circular_references_recursive(ref, target, type, visited)) {
                    return true;
                }
            }
            break;
            
        case RelationshipType::Ownership:
            if (node->owner.is_valid()) {
                if (check_circular_references_recursive(node->owner, target, type, visited)) {
                    return true;
                }
            }
            break;
            
        default:
            break;
    }
    
    visited.erase(current);
    return false;
}

void RelationshipManager::breadth_first_impl(Entity root, 
                                            const std::function<bool(Entity, u32)>& visitor) const {
    std::queue<std::pair<Entity, u32>> queue;
    std::unordered_set<Entity> visited;
    
    queue.push({root, 0});
    
    while (!queue.empty()) {
        auto [entity, depth] = queue.front();
        queue.pop();
        
        if (visited.find(entity) != visited.end()) {
            continue;
        }
        
        visited.insert(entity);
        
        // Visit the entity
        if (!visitor(entity, depth)) {
            break;  // Visitor requested to stop
        }
        
        // Add children to queue
        auto children = get_children(entity);
        for (Entity child : children) {
            if (visited.find(child) == visited.end()) {
                queue.push({child, depth + 1});
            }
        }
    }
}

void RelationshipManager::update_descendant_levels(Entity entity) {
    // Update hierarchy levels for all descendants
    traverse_breadth_first(entity, [this](Entity descendant, u32 depth) {
        std::unique_lock lock(nodes_mutex_);
        RelationshipNode* node = get_or_create_node(descendant);
        
        // Calculate new level based on parent
        if (node->parent.is_valid()) {
            const RelationshipNode* parent_node = get_node(node->parent);
            if (parent_node) {
                node->hierarchy_level = parent_node->hierarchy_level + 1;
            }
        } else {
            node->hierarchy_level = 0;
        }
        
        return true;  // Continue traversal
    });
}

void RelationshipManager::track_node_allocation(RelationshipNode* node) {
    memory::tracker::track_alloc(
        node, sizeof(RelationshipNode), sizeof(RelationshipNode), alignof(RelationshipNode),
        memory::AllocationCategory::ECS_Components, memory::AllocatorType::Pool,
        "RelationshipNode", allocator_id_
    );
}

void RelationshipManager::track_node_deallocation(RelationshipNode* node) {
    memory::tracker::track_dealloc(
        node, memory::AllocatorType::Pool, "RelationshipNode", allocator_id_
    );
}

u32 RelationshipManager::next_allocator_id() {
    return allocator_id_counter_.fetch_add(1, std::memory_order_relaxed);
}

// Utility functions implementation
namespace relationships {
    
    void create_linear_hierarchy(RelationshipManager& manager, 
                                const std::vector<Entity>& entities) {
        for (usize i = 1; i < entities.size(); ++i) {
            manager.set_parent(entities[i], entities[i - 1]);
        }
    }
    
    void create_tree_hierarchy(RelationshipManager& manager,
                              const std::vector<std::pair<Entity, Entity>>& pairs) {
        for (const auto& [child, parent] : pairs) {
            manager.set_parent(child, parent);
        }
    }
    
    std::vector<Entity> get_entities_at_depth(RelationshipManager& manager,
                                            Entity root, u32 target_depth) {
        std::vector<Entity> entities;
        
        manager.traverse_breadth_first(root, [&](Entity entity, u32 depth) {
            if (depth == target_depth) {
                entities.push_back(entity);
            }
            return depth <= target_depth;  // Continue until we reach target depth
        });
        
        return entities;
    }
    
    HierarchyInfo analyze_hierarchy(RelationshipManager& manager, Entity root) {
        HierarchyInfo info;
        info.depth = 0;
        info.node_count = 0;
        info.leaf_count = 0;
        info.balance_factor = 1.0;
        
        manager.traverse_breadth_first(root, [&](Entity entity, u32 depth) {
            info.node_count++;
            
            if (depth > info.depth) {
                info.depth = depth;
            }
            
            // Check if it's a leaf node
            auto children = manager.get_children(entity);
            if (children.empty()) {
                info.leaf_count++;
            }
            
            return true;  // Continue traversal
        });
        
        // Calculate balance factor (simplified)
        if (info.node_count > 1) {
            info.balance_factor = static_cast<f64>(info.leaf_count) / info.node_count;
        }
        
        return info;
    }
    
    std::vector<Entity> flatten_hierarchy(RelationshipManager& manager, Entity root,
                                        TraversalMode mode) {
        std::vector<Entity> flattened;
        
        switch (mode) {
            case TraversalMode::BreadthFirst:
            case TraversalMode::LevelOrder:
                manager.traverse_breadth_first(root, [&](Entity entity, u32 depth) {
                    flattened.push_back(entity);
                    return true;
                });
                break;
                
            case TraversalMode::DepthFirst:
            case TraversalMode::PreOrder:
                manager.traverse_depth_first(root, [&](Entity entity, u32 depth) {
                    flattened.push_back(entity);
                    return true;
                });
                break;
                
            default:
                manager.traverse_breadth_first(root, [&](Entity entity, u32 depth) {
                    flattened.push_back(entity);
                    return true;
                });
                break;
        }
        
        return flattened;
    }
    
    bool is_ancestor_of(RelationshipManager& manager, Entity ancestor, Entity descendant) {
        Entity current = descendant;
        
        while (true) {
            Entity parent = manager.get_parent(current);
            if (!parent.is_valid()) {
                break;
            }
            
            if (parent == ancestor) {
                return true;
            }
            
            current = parent;
        }
        
        return false;
    }
    
    std::optional<u32> get_hierarchy_distance(RelationshipManager& manager,
                                            Entity entity1, Entity entity2) {
        // Find common ancestor and calculate distance
        Entity lca = manager.find_lowest_common_ancestor(entity1, entity2);
        if (!lca.is_valid()) {
            return std::nullopt;  // No common ancestor
        }
        
        // Calculate distance from each entity to LCA
        u32 distance1 = 0;
        Entity current = entity1;
        while (current != lca && current.is_valid()) {
            current = manager.get_parent(current);
            distance1++;
        }
        
        u32 distance2 = 0;
        current = entity2;
        while (current != lca && current.is_valid()) {
            current = manager.get_parent(current);
            distance2++;
        }
        
        return distance1 + distance2;
    }
}

} // namespace ecscope::ecs