#pragma once

/**
 * @file networking/entity_replication.hpp
 * @brief Entity Replication System with Delta Compression for ECScope ECS
 * 
 * This header implements a comprehensive entity replication system that
 * efficiently synchronizes ECS entities across network clients. Features:
 * 
 * Core Replication Features:
 * - Entity creation, modification, and destruction sync
 * - Component-level delta compression for bandwidth efficiency
 * - Priority-based update scheduling
 * - Selective entity replication with interest management
 * - Version-based conflict resolution
 * 
 * Delta Compression System:
 * - Bit-packed component change detection
 * - Value-based delta encoding for numeric components
 * - String delta compression using edit distance
 * - Hierarchical entity relationship preservation
 * - Compressed transform interpolation
 * 
 * Educational Features:
 * - Visual representation of network entity states
 * - Bandwidth usage analysis per entity/component
 * - Replication algorithm comparison and benchmarking
 * - Interactive exploration of delta compression techniques
 * - Real-time network efficiency monitoring
 * 
 * Performance Optimizations:
 * - Sparse component tracking using bitsets
 * - Memory-efficient delta storage
 * - Batch processing of entity updates
 * - Cache-friendly entity sorting and processing
 * - Zero-allocation hot paths for critical updates
 * 
 * @author ECScope Educational ECS Framework - Networking Layer
 * @date 2024
 */

#include "network_types.hpp"
#include "network_protocol.hpp"
#include "../component.hpp"
#include "../entity.hpp"
#include "../registry.hpp"
#include "../core/types.hpp"
#include "../memory/arena.hpp"
#include "../memory/pool.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <bitset>
#include <optional>
#include <variant>
#include <functional>

namespace ecscope::networking::replication {

//=============================================================================
// Entity Network Identity and Versioning
//=============================================================================

/**
 * @brief Network Entity State
 * 
 * Tracks the network synchronization state of an entity, including
 * version information and replication metadata.
 */
struct NetworkEntityState {
    NetworkEntityID network_id{0};        // Global network entity ID
    ecs::Entity local_id{};               // Local ECS entity ID
    ComponentVersion version{1};          // Current entity version
    NetworkTick last_update_tick{0};      // Last network update tick
    NetworkTimestamp last_sync_time{0};   // Last synchronization timestamp
    ClientID authority{0};                // Client with authority over this entity
    
    /** @brief Bitset tracking which components have changed */
    std::bitset<64> changed_components;
    
    /** @brief Bitset tracking which components are replicated */
    std::bitset<64> replicated_components;
    
    /** @brief Priority level for replication updates */
    MessagePriority update_priority{MessagePriority::Normal};
    
    /** @brief Check if entity has pending changes */
    bool has_changes() const noexcept {
        return changed_components.any();
    }
    
    /** @brief Mark component as changed */
    template<typename T>
    void mark_component_changed() noexcept {
        static_assert(ecs::Component<T>, "T must be a component type");
        u32 component_id = static_cast<u32>(ecs::ComponentTraits<T>::id());
        if (component_id < 64) {
            changed_components.set(component_id);
        }
    }
    
    /** @brief Check if component has changed */
    template<typename T>
    bool has_component_changed() const noexcept {
        static_assert(ecs::Component<T>, "T must be a component type");
        u32 component_id = static_cast<u32>(ecs::ComponentTraits<T>::id());
        return component_id < 64 && changed_components.test(component_id);
    }
    
    /** @brief Clear all change flags */
    void clear_changes() noexcept {
        changed_components.reset();
    }
    
    /** @brief Increment entity version */
    void increment_version() noexcept {
        version++;
        if (version == 0) version = 1;  // Skip 0 (reserved for invalid)
    }
};

/**
 * @brief Network Entity Manager
 * 
 * Manages the mapping between local ECS entities and network entities,
 * handling ID assignment, version tracking, and replication state.
 */
class NetworkEntityManager {
private:
    // Network ID generation
    ClientID local_client_id_{0};
    u32 next_local_entity_index_{1};  // Start at 1 (0 reserved for invalid)
    
    // Bidirectional entity mapping
    std::unordered_map<ecs::Entity, NetworkEntityState> local_to_network_;
    std::unordered_map<NetworkEntityID, ecs::Entity> network_to_local_;
    
    // Entity pools for efficient allocation
    memory::Pool<NetworkEntityState> entity_state_pool_;
    
    // Statistics
    mutable u64 entities_created_{0};
    mutable u64 entities_destroyed_{0};
    mutable u64 total_updates_sent_{0};
    mutable u64 total_bytes_replicated_{0};
    
public:
    /** @brief Initialize with client ID */
    explicit NetworkEntityManager(ClientID client_id) 
        : local_client_id_(client_id),
          entity_state_pool_(1024) {}  // Start with capacity for 1024 entities
    
    /** @brief Register local entity for network replication */
    NetworkEntityID register_entity(ecs::Entity entity, MessagePriority priority = MessagePriority::Normal) {
        // Generate unique network ID
        NetworkEntityID network_id = generate_network_id();
        
        // Create network state
        NetworkEntityState state;
        state.network_id = network_id;
        state.local_id = entity;
        state.authority = local_client_id_;
        state.update_priority = priority;
        state.last_update_tick = 0;
        state.version = 1;
        
        // Store mappings
        local_to_network_[entity] = state;
        network_to_local_[network_id] = entity;
        
        entities_created_++;
        return network_id;
    }
    
    /** @brief Unregister entity from network replication */
    void unregister_entity(ecs::Entity entity) {
        auto it = local_to_network_.find(entity);
        if (it != local_to_network_.end()) {
            network_to_local_.erase(it->second.network_id);
            local_to_network_.erase(it);
            entities_destroyed_++;
        }
    }
    
    /** @brief Get network state for local entity */
    NetworkEntityState* get_network_state(ecs::Entity entity) {
        auto it = local_to_network_.find(entity);
        return it != local_to_network_.end() ? &it->second : nullptr;
    }
    
    /** @brief Get local entity from network ID */
    std::optional<ecs::Entity> get_local_entity(NetworkEntityID network_id) const {
        auto it = network_to_local_.find(network_id);
        return it != network_to_local_.end() ? 
               std::optional<ecs::Entity>(it->second) : 
               std::nullopt;
    }
    
    /** @brief Check if entity is registered for replication */
    bool is_replicated(ecs::Entity entity) const {
        return local_to_network_.find(entity) != local_to_network_.end();
    }
    
    /** @brief Get all replicated entities */
    std::vector<ecs::Entity> get_replicated_entities() const {
        std::vector<ecs::Entity> entities;
        entities.reserve(local_to_network_.size());
        
        for (const auto& [entity, state] : local_to_network_) {
            entities.push_back(entity);
        }
        
        return entities;
    }
    
    /** @brief Get entities with pending changes */
    std::vector<ecs::Entity> get_entities_with_changes() const {
        std::vector<ecs::Entity> entities;
        
        for (const auto& [entity, state] : local_to_network_) {
            if (state.has_changes()) {
                entities.push_back(entity);
            }
        }
        
        return entities;
    }
    
    /** @brief Update entity authority */
    void set_entity_authority(ecs::Entity entity, ClientID authority) {
        auto* state = get_network_state(entity);
        if (state) {
            state->authority = authority;
        }
    }
    
    /** @brief Check if local client has authority over entity */
    bool has_authority(ecs::Entity entity) const {
        auto it = local_to_network_.find(entity);
        return it != local_to_network_.end() && it->second.authority == local_client_id_;
    }
    
    /** @brief Clear all entities and reset state */
    void clear() {
        local_to_network_.clear();
        network_to_local_.clear();
        next_local_entity_index_ = 1;
        entities_created_ = 0;
        entities_destroyed_ = 0;
        total_updates_sent_ = 0;
        total_bytes_replicated_ = 0;
    }
    
    /** @brief Get replication statistics */
    struct Statistics {
        u64 total_entities;
        u64 entities_created;
        u64 entities_destroyed;
        u64 entities_with_changes;
        u64 total_updates_sent;
        u64 total_bytes_replicated;
        f64 average_bytes_per_update;
    };
    
    Statistics get_statistics() const {
        u64 entities_with_changes = 0;
        for (const auto& [entity, state] : local_to_network_) {
            if (state.has_changes()) {
                entities_with_changes++;
            }
        }
        
        return Statistics{
            .total_entities = local_to_network_.size(),
            .entities_created = entities_created_,
            .entities_destroyed = entities_destroyed_,
            .entities_with_changes = entities_with_changes,
            .total_updates_sent = total_updates_sent_,
            .total_bytes_replicated = total_bytes_replicated_,
            .average_bytes_per_update = total_updates_sent_ > 0 ? 
                static_cast<f64>(total_bytes_replicated_) / total_updates_sent_ : 0.0
        };
    }

private:
    /** @brief Generate unique network entity ID */
    NetworkEntityID generate_network_id() {
        // Combine client ID (upper 32 bits) with local index (lower 32 bits)
        NetworkEntityID network_id = (static_cast<u64>(local_client_id_) << 32) | 
                                     next_local_entity_index_++;
        
        // Handle overflow (unlikely but possible)
        if (next_local_entity_index_ == 0) {
            next_local_entity_index_ = 1;  // Skip 0
        }
        
        return network_id;
    }
};

//=============================================================================
// Component Delta Compression
//=============================================================================

/**
 * @brief Component Delta Types
 * 
 * Defines different types of deltas that can be applied to components
 * for efficient network serialization.
 */
enum class DeltaType : u8 {
    /** @brief Full component replacement */
    FullReplace = 0,
    
    /** @brief Numeric value delta (difference from previous) */
    NumericDelta = 1,
    
    /** @brief Bitwise change mask for struct components */
    BitwiseDelta = 2,
    
    /** @brief String edit operations (insert/delete/replace) */
    StringDelta = 3,
    
    /** @brief Vector/array element changes */
    ArrayDelta = 4,
    
    /** @brief Transform-specific optimizations */
    TransformDelta = 5
};

/**
 * @brief Component Delta Data
 * 
 * Compressed representation of component changes for network transmission.
 */
struct ComponentDelta {
    ecs::core::ComponentID component_id{0};
    DeltaType delta_type{DeltaType::FullReplace};
    ComponentVersion from_version{0};
    ComponentVersion to_version{0};
    std::vector<u8> delta_data;
    
    /** @brief Get delta data size */
    usize size() const noexcept {
        return delta_data.size();
    }
    
    /** @brief Check if delta is empty */
    bool empty() const noexcept {
        return delta_data.empty();
    }
};

/**
 * @brief Component Delta Encoder
 * 
 * Encodes component changes into compressed delta representations.
 * Uses different compression strategies based on component type and data.
 */
class ComponentDeltaEncoder {
private:
    /** @brief Component snapshots for delta generation */
    struct ComponentSnapshot {
        std::vector<u8> data;
        ComponentVersion version;
        NetworkTimestamp timestamp;
    };
    
    // Per-component type snapshot storage
    std::unordered_map<ecs::core::ComponentID, 
                      std::unordered_map<NetworkEntityID, ComponentSnapshot>> snapshots_;
    
    // Delta generation functions per component type
    std::unordered_map<ecs::core::ComponentID, 
                      std::function<ComponentDelta(const void*, const void*, usize)>> encoders_;
    
    // Memory allocator for delta data
    memory::Arena delta_arena_;
    
public:
    /** @brief Initialize encoder with arena size */
    explicit ComponentDeltaEncoder(usize arena_size = 1024 * 1024) // 1MB default
        : delta_arena_(arena_size) {}
    
    /** @brief Register delta encoder for component type */
    template<typename T>
    void register_encoder() {
        static_assert(ecs::Component<T>, "T must be a component type");
        
        auto component_id = ecs::ComponentTraits<T>::id();
        encoders_[component_id] = [this](const void* old_data, const void* new_data, usize size) {
            return encode_component_delta<T>(
                static_cast<const T*>(old_data),
                static_cast<const T*>(new_data)
            );
        };
    }
    
    /** @brief Generate delta for component change */
    template<typename T>
    ComponentDelta generate_delta(NetworkEntityID entity_id, 
                                 const T& current_component,
                                 ComponentVersion version) {
        static_assert(ecs::Component<T>, "T must be a component type");
        
        auto component_id = ecs::ComponentTraits<T>::id();
        
        // Get previous snapshot
        auto& entity_snapshots = snapshots_[component_id];
        auto snapshot_it = entity_snapshots.find(entity_id);
        
        ComponentDelta delta;
        delta.component_id = component_id;
        delta.to_version = version;
        
        if (snapshot_it == entity_snapshots.end()) {
            // No previous snapshot - send full component
            delta.delta_type = DeltaType::FullReplace;
            delta.from_version = 0;
            delta.delta_data.resize(sizeof(T));
            std::memcpy(delta.delta_data.data(), &current_component, sizeof(T));
        } else {
            // Generate delta from previous snapshot
            const T* previous_component = reinterpret_cast<const T*>(snapshot_it->second.data.data());
            delta = encode_component_delta<T>(previous_component, &current_component);
            delta.from_version = snapshot_it->second.version;
        }
        
        // Update snapshot
        ComponentSnapshot& snapshot = entity_snapshots[entity_id];
        snapshot.data.resize(sizeof(T));
        std::memcpy(snapshot.data.data(), &current_component, sizeof(T));
        snapshot.version = version;
        snapshot.timestamp = timing::now();
        
        return delta;
    }
    
    /** @brief Clean up old snapshots */
    void cleanup_old_snapshots(NetworkTimestamp cutoff_time) {
        for (auto& [component_id, entity_snapshots] : snapshots_) {
            auto it = entity_snapshots.begin();
            while (it != entity_snapshots.end()) {
                if (it->second.timestamp < cutoff_time) {
                    it = entity_snapshots.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }
    
    /** @brief Get memory usage statistics */
    struct MemoryStats {
        usize snapshot_memory_used;
        usize total_snapshots;
        usize arena_memory_used;
        usize arena_memory_available;
    };
    
    MemoryStats get_memory_stats() const {
        usize snapshot_memory = 0;
        usize total_snapshots = 0;
        
        for (const auto& [component_id, entity_snapshots] : snapshots_) {
            for (const auto& [entity_id, snapshot] : entity_snapshots) {
                snapshot_memory += snapshot.data.size();
                total_snapshots++;
            }
        }
        
        return MemoryStats{
            .snapshot_memory_used = snapshot_memory,
            .total_snapshots = total_snapshots,
            .arena_memory_used = delta_arena_.used(),
            .arena_memory_available = delta_arena_.available()
        };
    }

private:
    /** @brief Encode delta for generic component */
    template<typename T>
    ComponentDelta encode_component_delta(const T* old_component, const T* new_component) {
        ComponentDelta delta;
        delta.component_id = ecs::ComponentTraits<T>::id();
        
        // For now, use simple strategies based on type characteristics
        if constexpr (std::is_arithmetic_v<T>) {
            return encode_numeric_delta(*old_component, *new_component);
        } else if constexpr (std::is_same_v<T, std::string>) {
            return encode_string_delta(*old_component, *new_component);
        } else {
            return encode_bitwise_delta(old_component, new_component, sizeof(T));
        }
    }
    
    /** @brief Encode numeric value delta */
    template<typename T>
    ComponentDelta encode_numeric_delta(T old_value, T new_value) {
        static_assert(std::is_arithmetic_v<T>, "T must be arithmetic type");
        
        ComponentDelta delta;
        delta.delta_type = DeltaType::NumericDelta;
        
        // Calculate difference
        if constexpr (std::is_floating_point_v<T>) {
            // For floating point, check if difference is significant
            T diff = new_value - old_value;
            constexpr T epsilon = static_cast<T>(1e-6);
            
            if (std::abs(diff) < epsilon) {
                // No significant change
                return delta;
            }
            
            delta.delta_data.resize(sizeof(T));
            std::memcpy(delta.delta_data.data(), &diff, sizeof(T));
        } else {
            // For integers, store the difference
            auto diff = new_value - old_value;
            
            if (diff == 0) {
                // No change
                return delta;
            }
            
            delta.delta_data.resize(sizeof(diff));
            std::memcpy(delta.delta_data.data(), &diff, sizeof(diff));
        }
        
        return delta;
    }
    
    /** @brief Encode string delta using edit distance */
    ComponentDelta encode_string_delta(const std::string& old_string, const std::string& new_string) {
        ComponentDelta delta;
        delta.delta_type = DeltaType::StringDelta;
        
        if (old_string == new_string) {
            // No change
            return delta;
        }
        
        // For now, use simple approach - store full string if different
        // TODO: Implement proper edit distance algorithm
        delta.delta_type = DeltaType::FullReplace;
        delta.delta_data.resize(sizeof(u32) + new_string.size());
        
        u32 string_length = static_cast<u32>(new_string.size());
        std::memcpy(delta.delta_data.data(), &string_length, sizeof(u32));
        std::memcpy(delta.delta_data.data() + sizeof(u32), new_string.data(), new_string.size());
        
        return delta;
    }
    
    /** @brief Encode bitwise delta for struct types */
    ComponentDelta encode_bitwise_delta(const void* old_data, const void* new_data, usize size) {
        ComponentDelta delta;
        delta.delta_type = DeltaType::BitwiseDelta;
        
        // Find changed bytes
        std::vector<u8> change_mask((size + 7) / 8, 0);  // Bit mask for changed bytes
        std::vector<u8> changed_bytes;
        
        const u8* old_bytes = static_cast<const u8*>(old_data);
        const u8* new_bytes = static_cast<const u8*>(new_data);
        
        bool has_changes = false;
        for (usize i = 0; i < size; ++i) {
            if (old_bytes[i] != new_bytes[i]) {
                change_mask[i / 8] |= (1 << (i % 8));
                changed_bytes.push_back(new_bytes[i]);
                has_changes = true;
            }
        }
        
        if (!has_changes) {
            // No changes
            return delta;
        }
        
        // Store change mask followed by changed bytes
        delta.delta_data.reserve(change_mask.size() + changed_bytes.size());
        delta.delta_data.insert(delta.delta_data.end(), change_mask.begin(), change_mask.end());
        delta.delta_data.insert(delta.delta_data.end(), changed_bytes.begin(), changed_bytes.end());
        
        return delta;
    }
};

//=============================================================================
// Entity Replication Messages
//=============================================================================

/**
 * @brief Entity Replication Message Types
 */
enum class ReplicationMessageType : u8 {
    /** @brief Create new entity */
    EntityCreate = 0,
    
    /** @brief Update existing entity components */
    EntityUpdate = 1,
    
    /** @brief Destroy entity */
    EntityDestroy = 2,
    
    /** @brief Transfer entity authority */
    AuthorityTransfer = 3,
    
    /** @brief Request entity state synchronization */
    StateRequest = 4,
    
    /** @brief Full entity state response */
    StateResponse = 5
};

/**
 * @brief Entity Create Message
 * 
 * Sent when a new entity is created and needs to be replicated
 * to other clients.
 */
struct EntityCreateMessage {
    NetworkEntityID network_id;
    ComponentVersion version;
    ClientID authority;
    MessagePriority priority;
    NetworkTick creation_tick;
    
    // Component data follows this header
    u16 component_count;
    // Array of: [component_id: u32][component_size: u32][component_data: varies]
    
    /** @brief Calculate message header size */
    static constexpr usize header_size() {
        return sizeof(EntityCreateMessage);
    }
};

/**
 * @brief Entity Update Message
 * 
 * Sent when entity components change and need to be synchronized.
 * Uses delta compression for bandwidth efficiency.
 */
struct EntityUpdateMessage {
    NetworkEntityID network_id;
    ComponentVersion from_version;
    ComponentVersion to_version;
    NetworkTick update_tick;
    
    // Delta data follows this header
    u16 delta_count;
    // Array of ComponentDelta structures
    
    /** @brief Calculate message header size */
    static constexpr usize header_size() {
        return sizeof(EntityUpdateMessage);
    }
};

/**
 * @brief Entity Destroy Message
 * 
 * Sent when an entity is destroyed and should be removed from all clients.
 */
struct EntityDestroyMessage {
    NetworkEntityID network_id;
    ComponentVersion final_version;
    ClientID authority;
    NetworkTick destruction_tick;
    
    /** @brief Get message size */
    static constexpr usize size() {
        return sizeof(EntityDestroyMessage);
    }
};

} // namespace ecscope::networking::replication