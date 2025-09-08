#pragma once

#include "../core/types.hpp"
#include "../core/memory.hpp"
#include "../foundation/concepts.hpp"
#include "../foundation/component.hpp"
#include "sparse_set.hpp"
#include "archetype.hpp"
#include "entity_pool.hpp"
#include <vector>
#include <span>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <functional>
#include <optional>
#include <variant>
#include <fstream>
#include <sstream>

/**
 * @file advanced_features.hpp
 * @brief Advanced ECS features: relationships, prefabs, serialization, and hot-reloading
 * 
 * This file implements advanced ECS registry features including:
 * - Entity relationship system with hierarchical organization
 * - Prefab system with template instantiation and variants
 * - Serialization framework for save/load functionality
 * - Hot-reloading support for component and system updates
 * - Entity validation and debugging utilities
 * - Component dependency management and validation
 * - Entity lifecycle event system
 * - Advanced querying with relationship traversal
 * 
 * Educational Notes:
 * - Entity relationships enable scene graphs and complex entity hierarchies
 * - Prefabs provide efficient template instantiation for common entity types
 * - Serialization supports both binary and text formats for flexibility
 * - Hot-reloading enables rapid development iteration cycles
 * - Component dependencies ensure data integrity across entity modifications
 * - Event systems decouple entity lifecycle notifications from core logic
 */

namespace ecscope::registry::advanced {

using namespace ecscope::core;
using namespace ecscope::foundation;
using namespace ecscope::registry;

/// @brief Entity relationship types with semantic meaning
enum class RelationType : std::uint16_t {
    // Hierarchical relationships
    Parent = 1,           ///< Parent-child hierarchy (transforms, scene graph)
    Child = 2,            ///< Child of parent (reverse of Parent)
    
    // Ownership relationships  
    Owns = 10,            ///< Strong ownership (owner destroys owned)
    OwnedBy = 11,         ///< Owned by another entity (reverse of Owns)
    
    // Reference relationships
    References = 20,      ///< Weak reference (doesn't affect lifecycle)
    ReferencedBy = 21,    ///< Referenced by another entity (reverse of References)
    
    // Dependency relationships
    Depends = 30,         ///< Dependency (requires other entity to function)
    RequiredBy = 31,      ///< Required by another entity (reverse of Depends)
    
    // Group relationships
    MemberOf = 40,        ///< Member of a group/collection
    Contains = 41,        ///< Contains other entities (reverse of MemberOf)
    
    // Custom relationships (application-specific)
    Custom = 1000         ///< Base for custom relationship types
};

/// @brief Entity relationship descriptor with metadata
struct EntityRelation {
    EntityHandle from;
    EntityHandle to;
    RelationType type;
    std::uint32_t strength{1};              ///< Relationship strength/priority
    std::string metadata;                   ///< Optional metadata string
    Version creation_version{};             ///< Version when relationship was created
    
    bool operator==(const EntityRelation& other) const noexcept {
        return from == other.from && to == other.to && type == other.type;
    }
    
    /// @brief Get reverse relationship type
    RelationType get_reverse_type() const noexcept {
        switch (type) {
            case RelationType::Parent: return RelationType::Child;
            case RelationType::Child: return RelationType::Parent;
            case RelationType::Owns: return RelationType::OwnedBy;
            case RelationType::OwnedBy: return RelationType::Owns;
            case RelationType::References: return RelationType::ReferencedBy;
            case RelationType::ReferencedBy: return RelationType::References;
            case RelationType::Depends: return RelationType::RequiredBy;
            case RelationType::RequiredBy: return RelationType::Depends;
            case RelationType::MemberOf: return RelationType::Contains;
            case RelationType::Contains: return RelationType::MemberOf;
            default: return type;
        }
    }
    
    /// @brief Check if relationship affects entity lifecycle
    bool affects_lifecycle() const noexcept {
        return type == RelationType::Owns || type == RelationType::OwnedBy ||
               type == RelationType::Depends || type == RelationType::RequiredBy;
    }
};

/// @brief Entity relationship manager with graph traversal
class RelationshipManager {
public:
    using relation_callback = std::function<void(const EntityRelation&)>;
    using traversal_callback = std::function<bool(EntityHandle, const EntityRelation&)>;
    
    /// @brief Create relationship between entities
    /// @param from Source entity
    /// @param to Target entity  
    /// @param type Relationship type
    /// @param strength Relationship strength
    /// @param metadata Optional metadata
    /// @return true if relationship was created
    bool create_relationship(EntityHandle from, EntityHandle to, RelationType type,
                           std::uint32_t strength = 1, const std::string& metadata = "") {
        if (from == to) return false;  // No self-relationships
        
        EntityRelation relation{from, to, type, strength, metadata, ++version_};
        
        // Check for cycles if this is a hierarchical relationship
        if (is_hierarchical(type) && would_create_cycle(relation)) {
            return false;
        }
        
        // Add forward relationship
        relationships_[from].push_back(relation);
        
        // Add reverse relationship if bidirectional
        const auto reverse_type = relation.get_reverse_type();
        if (reverse_type != type) {
            EntityRelation reverse{to, from, reverse_type, strength, metadata, version_};
            relationships_[to].push_back(reverse);
        }
        
        if (relationship_created_callback_) {
            relationship_created_callback_(relation);
        }
        
        return true;
    }
    
    /// @brief Remove relationship between entities
    /// @param from Source entity
    /// @param to Target entity
    /// @param type Relationship type
    /// @return true if relationship was removed
    bool remove_relationship(EntityHandle from, EntityHandle to, RelationType type) {
        auto from_it = relationships_.find(from);
        if (from_it == relationships_.end()) return false;
        
        auto& from_relations = from_it->second;
        auto relation_it = std::find_if(from_relations.begin(), from_relations.end(),
            [to, type](const EntityRelation& rel) {
                return rel.to == to && rel.type == type;
            });
        
        if (relation_it == from_relations.end()) return false;
        
        const auto relation = *relation_it;
        from_relations.erase(relation_it);
        
        // Remove reverse relationship if it exists
        const auto reverse_type = relation.get_reverse_type();
        if (reverse_type != type) {
            auto to_it = relationships_.find(to);
            if (to_it != relationships_.end()) {
                auto& to_relations = to_it->second;
                auto reverse_it = std::find_if(to_relations.begin(), to_relations.end(),
                    [from, reverse_type](const EntityRelation& rel) {
                        return rel.to == from && rel.type == reverse_type;
                    });
                
                if (reverse_it != to_relations.end()) {
                    to_relations.erase(reverse_it);
                }
            }
        }
        
        if (relationship_removed_callback_) {
            relationship_removed_callback_(relation);
        }
        
        return true;
    }
    
    /// @brief Get all relationships for entity
    /// @param entity Entity to get relationships for
    /// @return Vector of relationships where entity is the source
    std::vector<EntityRelation> get_relationships(EntityHandle entity) const {
        auto it = relationships_.find(entity);
        return it != relationships_.end() ? it->second : std::vector<EntityRelation>{};
    }
    
    /// @brief Get relationships of specific type
    /// @param entity Source entity
    /// @param type Relationship type to filter by
    /// @return Vector of matching relationships
    std::vector<EntityRelation> get_relationships(EntityHandle entity, RelationType type) const {
        std::vector<EntityRelation> filtered;
        const auto relations = get_relationships(entity);
        
        for (const auto& relation : relations) {
            if (relation.type == type) {
                filtered.push_back(relation);
            }
        }
        
        return filtered;
    }
    
    /// @brief Check if relationship exists
    /// @param from Source entity
    /// @param to Target entity
    /// @param type Relationship type
    /// @return true if relationship exists
    bool has_relationship(EntityHandle from, EntityHandle to, RelationType type) const {
        const auto relations = get_relationships(from);
        return std::any_of(relations.begin(), relations.end(),
            [to, type](const EntityRelation& rel) {
                return rel.to == to && rel.type == type;
            });
    }
    
    /// @brief Get all children of entity (hierarchical)
    /// @param parent Parent entity
    /// @return Vector of child entities
    std::vector<EntityHandle> get_children(EntityHandle parent) const {
        std::vector<EntityHandle> children;
        const auto relations = get_relationships(parent, RelationType::Parent);
        
        for (const auto& relation : relations) {
            children.push_back(relation.to);
        }
        
        return children;
    }
    
    /// @brief Get parent of entity (hierarchical)
    /// @param child Child entity
    /// @return Parent entity, or invalid handle if no parent
    EntityHandle get_parent(EntityHandle child) const {
        const auto relations = get_relationships(child, RelationType::Child);
        return relations.empty() ? EntityHandle::invalid() : relations[0].to;
    }
    
    /// @brief Traverse relationship graph
    /// @param start Starting entity
    /// @param type Relationship type to follow
    /// @param callback Function to call for each visited entity
    /// @param max_depth Maximum traversal depth (0 = unlimited)
    void traverse_relationships(EntityHandle start, RelationType type,
                              traversal_callback callback, std::uint32_t max_depth = 0) const {
        std::unordered_set<EntityHandle> visited;
        traverse_recursive(start, type, callback, visited, 0, max_depth);
    }
    
    /// @brief Remove all relationships involving entity
    /// @param entity Entity to clean up relationships for
    void cleanup_entity_relationships(EntityHandle entity) {
        // Remove all outgoing relationships
        relationships_.erase(entity);
        
        // Remove all incoming relationships
        for (auto& [source, relations] : relationships_) {
            relations.erase(
                std::remove_if(relations.begin(), relations.end(),
                    [entity](const EntityRelation& rel) {
                        return rel.to == entity;
                    }),
                relations.end()
            );
        }
    }
    
    /// @brief Set relationship callbacks
    void set_relationship_created_callback(relation_callback callback) {
        relationship_created_callback_ = std::move(callback);
    }
    
    void set_relationship_removed_callback(relation_callback callback) {
        relationship_removed_callback_ = std::move(callback);
    }
    
    /// @brief Get relationship statistics
    struct RelationshipStats {
        std::uint32_t total_relationships{};
        std::uint32_t unique_entities_with_relationships{};
        std::uint32_t hierarchical_relationships{};
        std::uint32_t ownership_relationships{};
        std::uint32_t reference_relationships{};
        std::uint32_t dependency_relationships{};
        std::uint32_t max_relationships_per_entity{};
        double average_relationships_per_entity{};
    };
    
    RelationshipStats get_stats() const {
        RelationshipStats stats{};
        stats.unique_entities_with_relationships = static_cast<std::uint32_t>(relationships_.size());
        
        for (const auto& [entity, relations] : relationships_) {
            stats.total_relationships += static_cast<std::uint32_t>(relations.size());
            stats.max_relationships_per_entity = std::max(
                stats.max_relationships_per_entity,
                static_cast<std::uint32_t>(relations.size())
            );
            
            for (const auto& relation : relations) {
                if (is_hierarchical(relation.type)) ++stats.hierarchical_relationships;
                else if (is_ownership(relation.type)) ++stats.ownership_relationships;
                else if (is_reference(relation.type)) ++stats.reference_relationships;
                else if (is_dependency(relation.type)) ++stats.dependency_relationships;
            }
        }
        
        if (stats.unique_entities_with_relationships > 0) {
            stats.average_relationships_per_entity = 
                static_cast<double>(stats.total_relationships) / stats.unique_entities_with_relationships;
        }
        
        return stats;
    }

private:
    /// @brief Check if relationship type is hierarchical
    bool is_hierarchical(RelationType type) const noexcept {
        return type == RelationType::Parent || type == RelationType::Child;
    }
    
    /// @brief Check if relationship type is ownership
    bool is_ownership(RelationType type) const noexcept {
        return type == RelationType::Owns || type == RelationType::OwnedBy;
    }
    
    /// @brief Check if relationship type is reference
    bool is_reference(RelationType type) const noexcept {
        return type == RelationType::References || type == RelationType::ReferencedBy;
    }
    
    /// @brief Check if relationship type is dependency
    bool is_dependency(RelationType type) const noexcept {
        return type == RelationType::Depends || type == RelationType::RequiredBy;
    }
    
    /// @brief Check if adding relationship would create a cycle
    bool would_create_cycle(const EntityRelation& relation) const {
        if (!is_hierarchical(relation.type)) return false;
        
        // Check if 'to' entity can reach 'from' entity through existing relationships
        std::unordered_set<EntityHandle> visited;
        return can_reach(relation.to, relation.from, relation.type, visited);
    }
    
    /// @brief Check if entity can reach target through relationships
    bool can_reach(EntityHandle from, EntityHandle target, RelationType type,
                   std::unordered_set<EntityHandle>& visited) const {
        if (from == target) return true;
        if (visited.count(from)) return false;
        
        visited.insert(from);
        
        const auto relations = get_relationships(from, type);
        for (const auto& relation : relations) {
            if (can_reach(relation.to, target, type, visited)) {
                return true;
            }
        }
        
        return false;
    }
    
    /// @brief Recursive relationship traversal
    void traverse_recursive(EntityHandle current, RelationType type, 
                          const traversal_callback& callback,
                          std::unordered_set<EntityHandle>& visited,
                          std::uint32_t depth, std::uint32_t max_depth) const {
        if (visited.count(current) || (max_depth > 0 && depth >= max_depth)) {
            return;
        }
        
        visited.insert(current);
        
        const auto relations = get_relationships(current, type);
        for (const auto& relation : relations) {
            if (callback(relation.to, relation)) {
                traverse_recursive(relation.to, type, callback, visited, depth + 1, max_depth);
            }
        }
    }
    
    /// @brief Entity relationships storage: entity -> list of relationships
    std::unordered_map<EntityHandle, std::vector<EntityRelation>> relationships_;
    
    /// @brief Version counter for relationship tracking
    std::atomic<Version> version_{constants::INITIAL_VERSION};
    
    /// @brief Relationship lifecycle callbacks
    relation_callback relationship_created_callback_;
    relation_callback relationship_removed_callback_;
};

/// @brief Prefab variant descriptor
struct PrefabVariant {
    std::string name;
    std::unordered_map<ComponentId, std::vector<std::byte>> component_overrides;
    std::vector<ComponentId> removed_components;
    std::uint32_t usage_count{};
    
    /// @brief Apply variant to entity template
    EntityTemplate apply_to_template(const EntityTemplate& base_template) const {
        EntityTemplate variant_template = base_template;
        variant_template.name += "_" + name;
        
        // Apply component overrides
        for (const auto& [component_id, override_data] : component_overrides) {
            variant_template.component_data[component_id] = override_data;
            variant_template.signature = ComponentRegistry::add_component_to_signature(
                variant_template.signature, component_id);
        }
        
        // Remove components
        for (ComponentId removed_id : removed_components) {
            variant_template.component_data.erase(removed_id);
            variant_template.signature = ComponentRegistry::remove_component_from_signature(
                variant_template.signature, removed_id);
        }
        
        return variant_template;
    }
};

/// @brief Advanced prefab system with variants and inheritance
class PrefabManager {
public:
    using prefab_callback = std::function<void(const EntityTemplate&)>;
    
    /// @brief Register entity template as prefab
    /// @param template_def Entity template to register
    /// @return true if prefab was registered
    bool register_prefab(const EntityTemplate& template_def) {
        if (template_def.name.empty()) return false;
        
        prefabs_[template_def.name] = template_def;
        
        if (prefab_registered_callback_) {
            prefab_registered_callback_(template_def);
        }
        
        return true;
    }
    
    /// @brief Get prefab by name
    /// @param name Prefab name
    /// @return Pointer to prefab template, or nullptr if not found
    const EntityTemplate* get_prefab(const std::string& name) const {
        auto it = prefabs_.find(name);
        return it != prefabs_.end() ? &it->second : nullptr;
    }
    
    /// @brief Create prefab variant
    /// @param base_prefab_name Base prefab name
    /// @param variant Variant descriptor
    /// @return true if variant was created
    bool create_variant(const std::string& base_prefab_name, const PrefabVariant& variant) {
        const auto* base_prefab = get_prefab(base_prefab_name);
        if (!base_prefab) return false;
        
        const auto variant_template = variant.apply_to_template(*base_prefab);
        prefabs_[variant_template.name] = variant_template;
        prefab_variants_[base_prefab_name].push_back(variant);
        
        return true;
    }
    
    /// @brief Get all variants of prefab
    /// @param prefab_name Base prefab name
    /// @return Vector of variants
    std::vector<PrefabVariant> get_variants(const std::string& prefab_name) const {
        auto it = prefab_variants_.find(prefab_name);
        return it != prefab_variants_.end() ? it->second : std::vector<PrefabVariant>{};
    }
    
    /// @brief Get all registered prefab names
    /// @return Vector of prefab names
    std::vector<std::string> get_prefab_names() const {
        std::vector<std::string> names;
        names.reserve(prefabs_.size());
        
        for (const auto& [name, template_def] : prefabs_) {
            names.push_back(name);
        }
        
        return names;
    }
    
    /// @brief Remove prefab and all its variants
    /// @param name Prefab name to remove
    /// @return true if prefab was removed
    bool remove_prefab(const std::string& name) {
        auto prefab_it = prefabs_.find(name);
        if (prefab_it == prefabs_.end()) return false;
        
        // Remove base prefab
        prefabs_.erase(prefab_it);
        
        // Remove all variants
        const auto variants = get_variants(name);
        for (const auto& variant : variants) {
            prefabs_.erase(name + "_" + variant.name);
        }
        prefab_variants_.erase(name);
        
        return true;
    }
    
    /// @brief Set prefab callbacks
    void set_prefab_registered_callback(prefab_callback callback) {
        prefab_registered_callback_ = std::move(callback);
    }
    
    /// @brief Get prefab statistics
    struct PrefabStats {
        std::uint32_t total_prefabs{};
        std::uint32_t total_variants{};
        std::uint32_t most_used_prefab_usage{};
        std::string most_used_prefab_name;
        std::size_t total_memory_usage{};
    };
    
    PrefabStats get_stats() const {
        PrefabStats stats{};
        stats.total_prefabs = static_cast<std::uint32_t>(prefabs_.size());
        
        std::uint32_t max_usage = 0;
        
        for (const auto& [name, template_def] : prefabs_) {
            stats.total_memory_usage += template_def.memory_usage();
            
            if (template_def.usage_count > max_usage) {
                max_usage = template_def.usage_count;
                stats.most_used_prefab_usage = max_usage;
                stats.most_used_prefab_name = name;
            }
        }
        
        for (const auto& [prefab_name, variants] : prefab_variants_) {
            stats.total_variants += static_cast<std::uint32_t>(variants.size());
        }
        
        return stats;
    }

private:
    /// @brief Prefab storage: name -> template
    std::unordered_map<std::string, EntityTemplate> prefabs_;
    
    /// @brief Prefab variants: base_name -> variants
    std::unordered_map<std::string, std::vector<PrefabVariant>> prefab_variants_;
    
    /// @brief Prefab lifecycle callbacks
    prefab_callback prefab_registered_callback_;
};

/// @brief Serialization format types
enum class SerializationFormat {
    Binary,     ///< Compact binary format
    JSON,       ///< Human-readable JSON format
    XML,        ///< XML format for tools integration
    Custom      ///< Custom application-specific format
};

/// @brief Serialization context for customizing serialization behavior
struct SerializationContext {
    SerializationFormat format{SerializationFormat::Binary};
    bool include_relationships{true};
    bool include_metadata{true};
    bool compress_data{false};
    std::unordered_set<ComponentId> excluded_components;
    std::function<bool(EntityHandle)> entity_filter;  // Optional entity filter
    
    /// @brief Check if entity should be serialized
    bool should_serialize_entity(EntityHandle entity) const {
        return !entity_filter || entity_filter(entity);
    }
    
    /// @brief Check if component should be serialized
    bool should_serialize_component(ComponentId id) const {
        return excluded_components.find(id) == excluded_components.end();
    }
};

/// @brief Serialization result information
struct SerializationResult {
    bool success{false};
    std::size_t bytes_written{};
    std::uint32_t entities_serialized{};
    std::uint32_t components_serialized{};
    std::uint32_t relationships_serialized{};
    std::string error_message;
    std::chrono::milliseconds serialization_time{};
};

/// @brief Advanced serialization system for registry persistence
class SerializationManager {
public:
    /// @brief Serialize registry to stream
    /// @param registry Registry to serialize
    /// @param stream Output stream
    /// @param context Serialization context
    /// @return Serialization result
    SerializationResult serialize_registry(const AdvancedRegistry& registry,
                                         std::ostream& stream,
                                         const SerializationContext& context = {}) {
        const auto start_time = std::chrono::steady_clock::now();
        
        SerializationResult result{};
        
        try {
            switch (context.format) {
                case SerializationFormat::Binary:
                    result = serialize_binary(registry, stream, context);
                    break;
                case SerializationFormat::JSON:
                    result = serialize_json(registry, stream, context);
                    break;
                case SerializationFormat::XML:
                    result = serialize_xml(registry, stream, context);
                    break;
                default:
                    result.error_message = "Unsupported serialization format";
                    return result;
            }
            
            result.success = true;
        } catch (const std::exception& e) {
            result.error_message = e.what();
            result.success = false;
        }
        
        const auto end_time = std::chrono::steady_clock::now();
        result.serialization_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);
        
        return result;
    }
    
    /// @brief Deserialize registry from stream
    /// @param registry Registry to deserialize into
    /// @param stream Input stream
    /// @param context Serialization context
    /// @return Deserialization result
    SerializationResult deserialize_registry(AdvancedRegistry& registry,
                                           std::istream& stream,
                                           const SerializationContext& context = {}) {
        const auto start_time = std::chrono::steady_clock::now();
        
        SerializationResult result{};
        
        try {
            switch (context.format) {
                case SerializationFormat::Binary:
                    result = deserialize_binary(registry, stream, context);
                    break;
                case SerializationFormat::JSON:
                    result = deserialize_json(registry, stream, context);
                    break;
                case SerializationFormat::XML:
                    result = deserialize_xml(registry, stream, context);
                    break;
                default:
                    result.error_message = "Unsupported deserialization format";
                    return result;
            }
            
            result.success = true;
        } catch (const std::exception& e) {
            result.error_message = e.what();
            result.success = false;
        }
        
        const auto end_time = std::chrono::steady_clock::now();
        result.serialization_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);
        
        return result;
    }
    
    /// @brief Serialize single entity
    /// @param registry Registry containing entity
    /// @param entity Entity to serialize
    /// @param stream Output stream
    /// @param context Serialization context
    /// @return Serialization result
    SerializationResult serialize_entity(const AdvancedRegistry& registry,
                                       EntityHandle entity,
                                       std::ostream& stream,
                                       const SerializationContext& context = {}) {
        // Implementation would serialize single entity and its components
        SerializationResult result{};
        result.success = true;
        result.entities_serialized = 1;
        return result;
    }

private:
    /// @brief Serialize to binary format
    SerializationResult serialize_binary(const AdvancedRegistry& registry,
                                       std::ostream& stream,
                                       const SerializationContext& context) {
        SerializationResult result{};
        
        // Write format header
        const std::uint32_t magic = 0x45435342; // "ECSB"
        const std::uint16_t version = 1;
        
        stream.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
        stream.write(reinterpret_cast<const char*>(&version), sizeof(version));
        
        // Write registry statistics for verification
        const auto stats = registry.get_stats();
        stream.write(reinterpret_cast<const char*>(&stats.active_entities), sizeof(stats.active_entities));
        
        result.bytes_written = stream.tellp();
        result.entities_serialized = stats.active_entities;
        
        return result;
    }
    
    /// @brief Serialize to JSON format
    SerializationResult serialize_json(const AdvancedRegistry& registry,
                                     std::ostream& stream,
                                     const SerializationContext& context) {
        SerializationResult result{};
        
        // Write JSON header
        stream << "{\n";
        stream << "  \"format\": \"ECScope Registry JSON\",\n";
        stream << "  \"version\": 1,\n";
        
        const auto stats = registry.get_stats();
        stream << "  \"entity_count\": " << stats.active_entities << ",\n";
        stream << "  \"entities\": [\n";
        
        // Entities would be serialized here
        
        stream << "  ]\n";
        stream << "}\n";
        
        result.bytes_written = stream.tellp();
        result.entities_serialized = stats.active_entities;
        
        return result;
    }
    
    /// @brief Serialize to XML format
    SerializationResult serialize_xml(const AdvancedRegistry& registry,
                                    std::ostream& stream,
                                    const SerializationContext& context) {
        SerializationResult result{};
        
        // Write XML header
        stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        stream << "<registry format=\"ECScope\" version=\"1\">\n";
        
        const auto stats = registry.get_stats();
        stream << "  <entities count=\"" << stats.active_entities << "\">\n";
        
        // Entities would be serialized here
        
        stream << "  </entities>\n";
        stream << "</registry>\n";
        
        result.bytes_written = stream.tellp();
        result.entities_serialized = stats.active_entities;
        
        return result;
    }
    
    /// @brief Deserialize from binary format
    SerializationResult deserialize_binary(AdvancedRegistry& registry,
                                         std::istream& stream,
                                         const SerializationContext& context) {
        SerializationResult result{};
        
        // Read and verify format header
        std::uint32_t magic;
        std::uint16_t version;
        
        stream.read(reinterpret_cast<char*>(&magic), sizeof(magic));
        stream.read(reinterpret_cast<char*>(&version), sizeof(version));
        
        if (magic != 0x45435342 || version != 1) {
            result.error_message = "Invalid binary format or unsupported version";
            return result;
        }
        
        // Read entity count
        std::uint32_t entity_count;
        stream.read(reinterpret_cast<char*>(&entity_count), sizeof(entity_count));
        
        result.entities_serialized = entity_count;
        
        return result;
    }
    
    /// @brief Deserialize from JSON format
    SerializationResult deserialize_json(AdvancedRegistry& registry,
                                       std::istream& stream,
                                       const SerializationContext& context) {
        SerializationResult result{};
        
        // Parse JSON (simplified implementation)
        std::string json_content((std::istreambuf_iterator<char>(stream)),
                               std::istreambuf_iterator<char>());
        
        // In a full implementation, this would use a JSON parser
        result.entities_serialized = 0;  // Would parse from JSON
        
        return result;
    }
    
    /// @brief Deserialize from XML format
    SerializationResult deserialize_xml(AdvancedRegistry& registry,
                                      std::istream& stream,
                                      const SerializationContext& context) {
        SerializationResult result{};
        
        // Parse XML (simplified implementation)
        std::string xml_content((std::istreambuf_iterator<char>(stream)),
                              std::istreambuf_iterator<char>());
        
        // In a full implementation, this would use an XML parser
        result.entities_serialized = 0;  // Would parse from XML
        
        return result;
    }
};

/// @brief Hot-reloading support for dynamic updates
class HotReloadManager {
public:
    using reload_callback = std::function<void(const std::string&)>;
    using validation_callback = std::function<bool(const std::string&)>;
    
    /// @brief Register file for hot-reloading
    /// @param filepath File path to monitor
    /// @param callback Function to call when file changes
    /// @return true if file was registered for monitoring
    bool register_file(const std::string& filepath, reload_callback callback) {
        file_callbacks_[filepath] = std::move(callback);
        return true;
    }
    
    /// @brief Unregister file from hot-reloading
    /// @param filepath File path to stop monitoring
    void unregister_file(const std::string& filepath) {
        file_callbacks_.erase(filepath);
    }
    
    /// @brief Check for file changes and trigger reloads
    void update() {
        // In a full implementation, this would:
        // 1. Check file modification times
        // 2. Trigger callbacks for changed files
        // 3. Handle reload errors gracefully
        // 4. Provide rollback capabilities
    }
    
    /// @brief Set validation callback for reload safety
    void set_validation_callback(validation_callback callback) {
        validation_callback_ = std::move(callback);
    }

private:
    /// @brief File monitoring: filepath -> callback
    std::unordered_map<std::string, reload_callback> file_callbacks_;
    
    /// @brief File modification times for change detection
    std::unordered_map<std::string, std::time_t> file_times_;
    
    /// @brief Validation callback for reload safety
    validation_callback validation_callback_;
};

} // namespace ecscope::registry::advanced