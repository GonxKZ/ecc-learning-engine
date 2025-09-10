#pragma once

#include "network_types.hpp"
#include "network_message.hpp"
#include "../core/types.hpp"
#include "../ecs/registry.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <functional>
#include <type_traits>

namespace ecscope::networking {

// Forward declaration
class NetworkRegistry;

/**
 * @brief Component Replication Flags
 * 
 * Defines how a component should be replicated across the network.
 */
enum class ReplicationFlags : uint32_t {
    NONE = 0,
    REPLICATED = 1 << 0,        // Component is replicated
    OWNER_ONLY = 1 << 1,        // Only owner can modify
    RELIABLE = 1 << 2,          // Use reliable delivery
    ORDERED = 1 << 3,           // Maintain order
    COMPRESS = 1 << 4,          // Enable compression
    DELTA = 1 << 5,             // Use delta compression
    HIGH_FREQUENCY = 1 << 6,    // Update frequently (position, velocity)
    LOW_FREQUENCY = 1 << 7,     // Update infrequently (name, stats)
    CRITICAL = 1 << 8,          // Critical for gameplay
    COSMETIC = 1 << 9,          // Visual only, can be dropped
    
    // Common combinations
    REPLICATED_RELIABLE = REPLICATED | RELIABLE | ORDERED,
    REPLICATED_UNRELIABLE = REPLICATED,
    REPLICATED_DELTA = REPLICATED | DELTA | COMPRESS,
    POSITION_COMPONENT = REPLICATED | HIGH_FREQUENCY | DELTA | COMPRESS,
    STATIC_COMPONENT = REPLICATED | RELIABLE | ORDERED | LOW_FREQUENCY
};

inline ReplicationFlags operator|(ReplicationFlags a, ReplicationFlags b) {
    return static_cast<ReplicationFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline ReplicationFlags operator&(ReplicationFlags a, ReplicationFlags b) {
    return static_cast<ReplicationFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline bool has_flag(ReplicationFlags flags, ReplicationFlags flag) {
    return (flags & flag) == flag;
}

/**
 * @brief Component Replication Info
 * 
 * Metadata about how a component type should be replicated.
 */
struct ComponentReplicationInfo {
    ComponentTypeId component_type_id = 0;
    ReplicationFlags flags = ReplicationFlags::NONE;
    std::string component_name;
    size_t component_size = 0;
    
    // Serialization functions
    std::function<std::vector<uint8_t>(const void*)> serialize;
    std::function<bool(void*, const uint8_t*, size_t)> deserialize;
    std::function<size_t()> get_serialized_size;
    
    // Delta compression functions (optional)
    std::function<std::vector<uint8_t>(const void*, const void*)> create_delta;
    std::function<bool(void*, const void*, const uint8_t*, size_t)> apply_delta;
    
    // Interest management (optional)
    std::function<bool(EntityId, ClientID)> is_relevant_to_client;
    
    // Update frequency (in ticks)
    uint32_t update_frequency = 1; // Every tick by default
    
    bool is_replicated() const {
        return has_flag(flags, ReplicationFlags::REPLICATED);
    }
    
    bool uses_delta_compression() const {
        return has_flag(flags, ReplicationFlags::DELTA) && create_delta && apply_delta;
    }
    
    bool is_reliable() const {
        return has_flag(flags, ReplicationFlags::RELIABLE);
    }
    
    bool is_ordered() const {
        return has_flag(flags, ReplicationFlags::ORDERED);
    }
    
    bool is_high_frequency() const {
        return has_flag(flags, ReplicationFlags::HIGH_FREQUENCY);
    }
};

/**
 * @brief Entity Replication State
 * 
 * Tracks replication state for a single entity.
 */
struct EntityReplicationState {
    EntityId entity_id = 0;
    NetworkEntityID network_entity_id = 0;
    ClientID owner_id = 0;
    NetworkTick last_update_tick = 0;
    bool is_replicated = false;
    
    // Per-component state
    struct ComponentState {
        ComponentVersion version = 0;
        NetworkTick last_replicated_tick = 0;
        std::vector<uint8_t> last_state; // For delta compression
        bool needs_full_update = true;
    };
    
    std::unordered_map<ComponentTypeId, ComponentState> component_states;
    
    // Interest management - which clients are interested in this entity
    std::unordered_set<ClientID> interested_clients;
};

/**
 * @brief Component Replication Message
 * 
 * Network message containing component data updates.
 */
class ComponentReplicationMessage : public NetworkMessage {
public:
    static constexpr uint16_t MESSAGE_TYPE = 100;
    
    struct ComponentUpdate {
        NetworkEntityID network_entity_id = 0;
        ComponentTypeId component_type_id = 0;
        ComponentVersion version = 0;
        bool is_delta = false;
        std::vector<uint8_t> data;
    };
    
    ComponentReplicationMessage();
    explicit ComponentReplicationMessage(NetworkTick tick);
    
    NetworkTick get_tick() const { return tick_; }
    void set_tick(NetworkTick tick) { tick_ = tick; }
    
    const std::vector<ComponentUpdate>& get_updates() const { return updates_; }
    void add_update(ComponentUpdate update);
    void clear_updates() { updates_.clear(); }
    
    size_t get_serialized_size() const override;
    bool is_valid() const override;

protected:
    void serialize_payload(std::vector<uint8_t>& buffer) const override;
    bool deserialize_payload(const uint8_t* data, size_t size) override;

private:
    NetworkTick tick_ = 0;
    std::vector<ComponentUpdate> updates_;
};

/**
 * @brief Entity Spawn Message
 * 
 * Network message for creating new replicated entities.
 */
class EntitySpawnMessage : public NetworkMessage {
public:
    static constexpr uint16_t MESSAGE_TYPE = 101;
    
    EntitySpawnMessage();
    EntitySpawnMessage(NetworkEntityID network_entity_id, ClientID owner_id);
    
    NetworkEntityID get_network_entity_id() const { return network_entity_id_; }
    void set_network_entity_id(NetworkEntityID id) { network_entity_id_ = id; }
    
    ClientID get_owner_id() const { return owner_id_; }
    void set_owner_id(ClientID owner_id) { owner_id_ = owner_id; }
    
    const std::vector<ComponentReplicationMessage::ComponentUpdate>& get_initial_components() const {
        return initial_components_;
    }
    void add_initial_component(ComponentReplicationMessage::ComponentUpdate component);
    
    size_t get_serialized_size() const override;

protected:
    void serialize_payload(std::vector<uint8_t>& buffer) const override;
    bool deserialize_payload(const uint8_t* data, size_t size) override;

private:
    NetworkEntityID network_entity_id_ = 0;
    ClientID owner_id_ = 0;
    std::vector<ComponentReplicationMessage::ComponentUpdate> initial_components_;
};

/**
 * @brief Entity Despawn Message
 * 
 * Network message for destroying replicated entities.
 */
class EntityDespawnMessage : public NetworkMessage {
public:
    static constexpr uint16_t MESSAGE_TYPE = 102;
    
    EntityDespawnMessage();
    explicit EntityDespawnMessage(NetworkEntityID network_entity_id);
    
    NetworkEntityID get_network_entity_id() const { return network_entity_id_; }
    void set_network_entity_id(NetworkEntityID id) { network_entity_id_ = id; }
    
    size_t get_serialized_size() const override;

protected:
    void serialize_payload(std::vector<uint8_t>& buffer) const override;
    bool deserialize_payload(const uint8_t* data, size_t size) override;

private:
    NetworkEntityID network_entity_id_ = 0;
};

/**
 * @brief Entity Ownership Message
 * 
 * Network message for transferring entity ownership.
 */
class EntityOwnershipMessage : public NetworkMessage {
public:
    static constexpr uint16_t MESSAGE_TYPE = 103;
    
    EntityOwnershipMessage();
    EntityOwnershipMessage(NetworkEntityID network_entity_id, ClientID new_owner_id);
    
    NetworkEntityID get_network_entity_id() const { return network_entity_id_; }
    void set_network_entity_id(NetworkEntityID id) { network_entity_id_ = id; }
    
    ClientID get_new_owner_id() const { return new_owner_id_; }
    void set_new_owner_id(ClientID owner_id) { new_owner_id_ = owner_id; }
    
    size_t get_serialized_size() const override;

protected:
    void serialize_payload(std::vector<uint8_t>& buffer) const override;
    bool deserialize_payload(const uint8_t* data, size_t size) override;

private:
    NetworkEntityID network_entity_id_ = 0;
    ClientID new_owner_id_ = 0;
};

/**
 * @brief Component Replication Registry
 * 
 * Registry for component replication information and serialization functions.
 */
class ComponentReplicationRegistry {
public:
    static ComponentReplicationRegistry& instance();
    
    // Component registration
    template<typename T>
    void register_component(const std::string& name, ReplicationFlags flags,
                           uint32_t update_frequency = 1) {
        ComponentReplicationInfo info;
        info.component_type_id = get_component_type_id<T>();
        info.component_name = name;
        info.component_size = sizeof(T);
        info.flags = flags;
        info.update_frequency = update_frequency;
        
        // Default serialization (requires T to be trivially copyable)
        if constexpr (std::is_trivially_copyable_v<T>) {
            info.serialize = [](const void* component) -> std::vector<uint8_t> {
                const T* comp = static_cast<const T*>(component);
                std::vector<uint8_t> data(sizeof(T));
                std::memcpy(data.data(), comp, sizeof(T));
                return data;
            };
            
            info.deserialize = [](void* component, const uint8_t* data, size_t size) -> bool {
                if (size != sizeof(T)) return false;
                T* comp = static_cast<T*>(component);
                std::memcpy(comp, data, sizeof(T));
                return true;
            };
            
            info.get_serialized_size = []() -> size_t {
                return sizeof(T);
            };
        }
        
        register_component_info(info);
    }
    
    // Component registration with custom serialization
    template<typename T>
    void register_component_with_serialization(
        const std::string& name,
        ReplicationFlags flags,
        std::function<std::vector<uint8_t>(const T&)> serialize_func,
        std::function<bool(T&, const uint8_t*, size_t)> deserialize_func,
        std::function<size_t()> size_func,
        uint32_t update_frequency = 1) {
        
        ComponentReplicationInfo info;
        info.component_type_id = get_component_type_id<T>();
        info.component_name = name;
        info.component_size = sizeof(T);
        info.flags = flags;
        info.update_frequency = update_frequency;
        
        info.serialize = [serialize_func](const void* component) -> std::vector<uint8_t> {
            const T* comp = static_cast<const T*>(component);
            return serialize_func(*comp);
        };
        
        info.deserialize = [deserialize_func](void* component, const uint8_t* data, size_t size) -> bool {
            T* comp = static_cast<T*>(component);
            return deserialize_func(*comp, data, size);
        };
        
        info.get_serialized_size = size_func;
        
        register_component_info(info);
    }
    
    // Delta compression support
    template<typename T>
    void register_delta_compression(
        std::function<std::vector<uint8_t>(const T&, const T&)> create_delta_func,
        std::function<bool(T&, const T&, const uint8_t*, size_t)> apply_delta_func) {
        
        ComponentTypeId type_id = get_component_type_id<T>();
        auto it = replication_info_.find(type_id);
        if (it != replication_info_.end()) {
            it->second.create_delta = [create_delta_func](const void* current, const void* previous) -> std::vector<uint8_t> {
                const T* curr = static_cast<const T*>(current);
                const T* prev = static_cast<const T*>(previous);
                return create_delta_func(*curr, *prev);
            };
            
            it->second.apply_delta = [apply_delta_func](void* target, const void* base, const uint8_t* delta, size_t size) -> bool {
                T* tgt = static_cast<T*>(target);
                const T* base_comp = static_cast<const T*>(base);
                return apply_delta_func(*tgt, *base_comp, delta, size);
            };
        }
    }
    
    // Interest management
    template<typename T>
    void register_interest_filter(std::function<bool(EntityId, ClientID)> filter) {
        ComponentTypeId type_id = get_component_type_id<T>();
        auto it = replication_info_.find(type_id);
        if (it != replication_info_.end()) {
            it->second.is_relevant_to_client = std::move(filter);
        }
    }
    
    // Query functions
    const ComponentReplicationInfo* get_replication_info(ComponentTypeId type_id) const;
    bool is_component_replicated(ComponentTypeId type_id) const;
    std::vector<ComponentTypeId> get_replicated_component_types() const;
    
    // Component type resolution
    template<typename T>
    static ComponentTypeId get_component_type_id() {
        return ecscope::ecs::component_type_id<T>();
    }
    
private:
    ComponentReplicationRegistry() = default;
    void register_component_info(const ComponentReplicationInfo& info);
    
    std::unordered_map<ComponentTypeId, ComponentReplicationInfo> replication_info_;
    mutable std::shared_mutex registry_mutex_;
};

/**
 * @brief Replication Manager
 * 
 * Manages entity and component replication across the network.
 */
class ReplicationManager {
public:
    explicit ReplicationManager(NetworkRegistry* network_registry);
    ~ReplicationManager() = default;
    
    // Entity replication
    NetworkResult<void> register_replicated_entity(EntityId entity_id, ClientID owner_id = 0);
    NetworkResult<void> unregister_replicated_entity(EntityId entity_id);
    bool is_entity_replicated(EntityId entity_id) const;
    
    // Entity ownership
    NetworkResult<void> set_entity_owner(EntityId entity_id, ClientID owner_id);
    ClientID get_entity_owner(EntityId entity_id) const;
    
    // Interest management
    void add_interested_client(EntityId entity_id, ClientID client_id);
    void remove_interested_client(EntityId entity_id, ClientID client_id);
    bool is_client_interested(EntityId entity_id, ClientID client_id) const;
    void update_interest_for_client(ClientID client_id); // Update based on filters
    
    // Replication processing
    void process_replication(NetworkTick current_tick);
    std::vector<std::unique_ptr<NetworkMessage>> generate_replication_messages(
        ClientID target_client, NetworkTick current_tick);
    
    // Message handling
    void handle_component_replication_message(const ComponentReplicationMessage& message, ClientID sender);
    void handle_entity_spawn_message(const EntitySpawnMessage& message, ClientID sender);
    void handle_entity_despawn_message(const EntityDespawnMessage& message, ClientID sender);
    void handle_entity_ownership_message(const EntityOwnershipMessage& message, ClientID sender);
    
    // Statistics and debugging
    struct ReplicationStats {
        uint64_t entities_replicated = 0;
        uint64_t components_updated = 0;
        uint64_t bytes_sent = 0;
        uint64_t bytes_received = 0;
        uint64_t delta_compressions_used = 0;
        uint64_t full_updates_sent = 0;
        double average_compression_ratio = 1.0;
    };
    
    ReplicationStats get_statistics() const;
    void reset_statistics();
    
private:
    NetworkEntityID generate_network_entity_id(EntityId entity_id);
    EntityId get_local_entity_id(NetworkEntityID network_entity_id);
    
    void collect_component_updates_for_entity(EntityId entity_id, ClientID target_client,
                                            NetworkTick current_tick,
                                            std::vector<ComponentReplicationMessage::ComponentUpdate>& updates);
    
    bool should_replicate_component(EntityId entity_id, ComponentTypeId component_type,
                                   ClientID target_client, NetworkTick current_tick);
    
    NetworkRegistry* network_registry_;
    
    // Entity replication state
    std::unordered_map<EntityId, EntityReplicationState> entity_states_;
    std::unordered_map<NetworkEntityID, EntityId> network_to_local_entity_;
    mutable std::shared_mutex entity_states_mutex_;
    
    // Client state
    std::unordered_map<ClientID, NetworkTick> client_last_ack_tick_;
    
    // Statistics
    mutable ReplicationStats statistics_;
    mutable std::mutex statistics_mutex_;
    
    // Network entity ID generation
    std::atomic<uint32_t> next_network_entity_id_{1};
};

} // namespace ecscope::networking