#pragma once

#include "network_types.hpp"
#include "connection.hpp"
#include "ecs_replication.hpp"
#include "state_synchronization.hpp"
#include "compression.hpp"
#include "encryption.hpp"
#include "network_monitor.hpp"
#include "../ecs/registry.hpp"
#include <memory>
#include <unordered_set>
#include <functional>

namespace ecscope::networking {

/**
 * @brief Network Registry Configuration
 */
struct NetworkRegistryConfig {
    // Core networking
    bool is_server = false;
    NetworkAddress server_address;
    TransportProtocol transport_protocol = TransportProtocol::ReliableUDP;
    Connection::Config connection_config;
    
    // ECS Replication
    bool enable_replication = true;
    std::chrono::milliseconds replication_interval{16}; // 60 Hz
    size_t max_entities_per_update = 100;
    bool enable_interest_management = true;
    double interest_radius = 100.0; // For spatial interest management
    
    // State Synchronization
    StateSynchronizationManager::SyncConfig sync_config;
    
    // Compression
    bool enable_compression = true;
    CompressionAlgorithm compression_algorithm = CompressionAlgorithm::LZ4;
    CompressionLevel compression_level = CompressionLevel::FAST;
    
    // Encryption
    bool enable_encryption = false;
    EncryptionAlgorithm encryption_algorithm = EncryptionAlgorithm::AES_256_GCM;
    KeyExchangeMethod key_exchange_method = KeyExchangeMethod::ECDH_X25519;
    
    // Monitoring
    bool enable_monitoring = true;
    NetworkMonitorManager::GlobalConfig monitor_config;
    bool enable_profiling = false;
    NetworkProfiler::ProfileConfig profiler_config;
    
    // Performance
    size_t network_thread_count = 1;
    size_t max_concurrent_connections = 100;
    bool enable_multithreaded_replication = true;
};

/**
 * @brief Main Network Registry
 * 
 * Central hub for all networking functionality, integrating with the ECS registry.
 * This is the primary interface for networked applications.
 */
class NetworkRegistry {
public:
    explicit NetworkRegistry(std::shared_ptr<ecs::Registry> ecs_registry,
                           const NetworkRegistryConfig& config = {});
    ~NetworkRegistry();
    
    // Non-copyable but movable
    NetworkRegistry(const NetworkRegistry&) = delete;
    NetworkRegistry& operator=(const NetworkRegistry&) = delete;
    NetworkRegistry(NetworkRegistry&&) = default;
    NetworkRegistry& operator=(NetworkRegistry&&) = default;
    
    // Core lifecycle
    NetworkResult<void> initialize();
    NetworkResult<void> start();
    NetworkResult<void> stop();
    NetworkResult<void> shutdown();
    void update(); // Call every frame
    
    // Server operations
    NetworkResult<void> start_server();
    NetworkResult<void> stop_server();
    bool is_server() const { return config_.is_server; }
    bool is_server_running() const;
    
    // Client operations
    NetworkResult<ConnectionId> connect_to_server(const NetworkAddress& address);
    NetworkResult<void> disconnect_from_server();
    bool is_connected_to_server() const;
    ConnectionId get_server_connection() const;
    
    // Connection management
    std::vector<ConnectionId> get_all_connections() const;
    size_t get_connection_count() const;
    std::shared_ptr<Connection> get_connection(ConnectionId connection_id) const;
    NetworkResult<void> disconnect_client(ConnectionId connection_id);
    
    // Entity replication registration
    template<typename... Components>
    void register_replicated_entity(EntityId entity_id, ClientID owner_id = 0) {
        // Register the entity for replication
        if (replication_manager_) {
            replication_manager_->register_replicated_entity(entity_id, owner_id);
            
            // Register each component type for replication if not already registered
            auto& comp_registry = ComponentReplicationRegistry::instance();
            (register_component_for_replication<Components>(), ...);
        }
    }
    
    template<typename ComponentType>
    void register_component_for_replication(const std::string& name = "", 
                                          ReplicationFlags flags = ReplicationFlags::REPLICATED_RELIABLE,
                                          uint32_t update_frequency = 1) {
        auto& comp_registry = ComponentReplicationRegistry::instance();
        
        std::string component_name = name.empty() ? typeid(ComponentType).name() : name;
        comp_registry.register_component<ComponentType>(component_name, flags, update_frequency);
    }
    
    // Entity ownership
    NetworkResult<void> set_entity_owner(EntityId entity_id, ClientID owner_id);
    ClientID get_entity_owner(EntityId entity_id) const;
    
    // Interest management
    void add_interested_client(EntityId entity_id, ClientID client_id);
    void remove_interested_client(EntityId entity_id, ClientID client_id);
    void update_spatial_interest(ClientID client_id, float x, float y, float z = 0.0f);
    
    // Messaging
    template<typename MessageType>
    NetworkResult<void> send_message(ConnectionId connection_id, std::unique_ptr<MessageType> message) {
        static_assert(std::is_base_of_v<NetworkMessage, MessageType>, "MessageType must derive from NetworkMessage");
        return send_message_impl(connection_id, std::move(message));
    }
    
    template<typename MessageType>
    NetworkResult<void> broadcast_message(std::unique_ptr<MessageType> message) {
        static_assert(std::is_base_of_v<NetworkMessage, MessageType>, "MessageType must derive from NetworkMessage");
        return broadcast_message_impl(std::move(message));
    }
    
    // Message handling callbacks
    template<typename MessageType>
    void register_message_handler(std::function<void(ConnectionId, const MessageType&)> handler) {
        static_assert(std::is_base_of_v<NetworkMessage, MessageType>, "MessageType must derive from NetworkMessage");
        register_message_handler_impl(MessageType::MESSAGE_TYPE, 
            [handler](ConnectionId conn_id, const NetworkMessage& msg) {
                const auto& typed_msg = static_cast<const MessageType&>(msg);
                handler(conn_id, typed_msg);
            });
    }
    
    // Event callbacks
    using ConnectionEventCallback = std::function<void(ConnectionId, ConnectionState)>;
    using ClientAuthCallback = std::function<bool(ConnectionId, const std::string& client_name)>; // Return false to reject
    using ErrorCallback = std::function<void(ConnectionId, NetworkError, const std::string&)>;
    
    void set_connection_event_callback(ConnectionEventCallback callback);
    void set_client_auth_callback(ClientAuthCallback callback);
    void set_error_callback(ErrorCallback callback);
    
    // Statistics and monitoring
    NetworkStats get_network_statistics() const;
    NetworkMonitorManager::AggregatedMetrics get_monitoring_metrics() const;
    ReplicationManager::ReplicationStats get_replication_statistics() const;
    
    // Performance profiling
    void start_profiling();
    void stop_profiling();
    std::string generate_performance_report() const;
    
    // Configuration
    void set_config(const NetworkRegistryConfig& config);
    const NetworkRegistryConfig& get_config() const { return config_; }
    
    // Debug and diagnostics
    std::vector<debug_tools::ConnectionDiagnostics> diagnose_all_connections() const;
    debug_tools::ConnectionDiagnostics diagnose_connection(ConnectionId connection_id) const;
    void enable_network_tracing(bool enable);
    void set_simulation_config(const debug_tools::NetworkSimulator::SimulationConfig& sim_config);
    
    // Advanced features
    void enable_delta_compression(bool enable);
    void enable_encryption(bool enable);
    void rotate_encryption_keys();
    void set_compression_algorithm(CompressionAlgorithm algorithm, CompressionLevel level);

private:
    // Core components
    std::shared_ptr<ecs::Registry> ecs_registry_;
    NetworkRegistryConfig config_;
    
    // Network management
    std::unique_ptr<ConnectionManager> connection_manager_;
    std::unique_ptr<ReplicationManager> replication_manager_;
    std::unique_ptr<StateSynchronizationManager> sync_manager_;
    
    // Compression and encryption
    std::unique_ptr<AdaptiveCompressionManager> compression_manager_;
    std::unique_ptr<SecureNetworkProtocol> security_protocol_;
    
    // Monitoring and profiling
    std::unique_ptr<NetworkMonitorManager> monitor_manager_;
    std::unique_ptr<NetworkProfiler> profiler_;
    std::unique_ptr<debug_tools::NetworkSimulator> network_simulator_;
    
    // State
    std::atomic<bool> initialized_{false};
    std::atomic<bool> running_{false};
    NetworkTick current_tick_ = 0;
    std::chrono::steady_clock::time_point last_replication_time_;
    
    // Client state (for client mode)
    ConnectionId server_connection_id_ = INVALID_CONNECTION_ID;
    ClientID local_client_id_ = 0;
    SessionID current_session_id_ = 0;
    
    // Threading
    std::vector<std::thread> network_threads_;
    std::atomic<bool> should_stop_threads_{false};
    
    // Message handling
    using MessageHandler = std::function<void(ConnectionId, const NetworkMessage&)>;
    std::unordered_map<uint16_t, MessageHandler> message_handlers_;
    std::shared_mutex message_handlers_mutex_;
    
    // Callbacks
    ConnectionEventCallback connection_event_callback_;
    ClientAuthCallback client_auth_callback_;
    ErrorCallback error_callback_;
    std::mutex callbacks_mutex_;
    
    // Spatial interest management
    struct ClientSpatialInfo {
        ClientID client_id;
        float x, y, z;
        NetworkTimestamp last_update;
        std::unordered_set<EntityId> interested_entities;
    };
    std::unordered_map<ClientID, ClientSpatialInfo> client_spatial_info_;
    std::shared_mutex spatial_info_mutex_;
    
    // Implementation methods
    NetworkResult<void> send_message_impl(ConnectionId connection_id, std::unique_ptr<NetworkMessage> message);
    NetworkResult<void> broadcast_message_impl(std::unique_ptr<NetworkMessage> message);
    void register_message_handler_impl(uint16_t message_type, MessageHandler handler);
    
    // Internal event handlers
    void handle_connection_established(ConnectionId connection_id);
    void handle_connection_lost(ConnectionId connection_id);
    void handle_message_received(ConnectionId connection_id, std::unique_ptr<NetworkMessage> message);
    void handle_network_error(ConnectionId connection_id, NetworkError error, const std::string& message);
    
    // Built-in message handlers
    void handle_handshake_message(ConnectionId connection_id, const HandshakeMessage& message);
    void handle_handshake_ack_message(ConnectionId connection_id, const HandshakeAckMessage& message);
    void handle_heartbeat_message(ConnectionId connection_id, const HeartbeatMessage& message);
    void handle_disconnect_message(ConnectionId connection_id, const DisconnectMessage& message);
    
    // Replication processing
    void process_replication();
    void update_spatial_interest_management();
    void calculate_entity_interest(const ClientSpatialInfo& client_info);
    
    // Threading functions
    void network_thread_function(int thread_id);
    void replication_thread_function();
    
    // Initialization helpers
    void initialize_message_factory();
    void initialize_component_replication();
    void setup_built_in_message_handlers();
    void create_network_components();
};

/**
 * @brief Convenience wrapper for easy networking setup
 */
class SimpleNetworkRegistry {
public:
    static std::unique_ptr<NetworkRegistry> create_server(
        std::shared_ptr<ecs::Registry> ecs_registry,
        uint16_t port = constants::DEFAULT_SERVER_PORT,
        TransportProtocol protocol = TransportProtocol::ReliableUDP);
    
    static std::unique_ptr<NetworkRegistry> create_client(
        std::shared_ptr<ecs::Registry> ecs_registry,
        TransportProtocol protocol = TransportProtocol::ReliableUDP);
    
    static std::unique_ptr<NetworkRegistry> create_peer_to_peer(
        std::shared_ptr<ecs::Registry> ecs_registry,
        uint16_t port = 0,
        TransportProtocol protocol = TransportProtocol::ReliableUDP);
};

/**
 * @brief Network Entity Factory
 * 
 * Helper for creating networked entities with proper replication setup.
 */
class NetworkEntityFactory {
public:
    explicit NetworkEntityFactory(NetworkRegistry* network_registry);
    
    template<typename... Components>
    EntityId create_replicated_entity(ClientID owner_id = 0) {
        EntityId entity_id = network_registry_->ecs_registry_->create_entity();
        
        // Add components
        (network_registry_->ecs_registry_->add_component<Components>(entity_id, Components{}), ...);
        
        // Register for replication
        network_registry_->register_replicated_entity<Components...>(entity_id, owner_id);
        
        return entity_id;
    }
    
    template<typename... Components>
    EntityId create_local_entity() {
        EntityId entity_id = network_registry_->ecs_registry_->create_entity();
        
        // Add components (not replicated)
        (network_registry_->ecs_registry_->add_component<Components>(entity_id, Components{}), ...);
        
        return entity_id;
    }
    
    void destroy_entity(EntityId entity_id);
    void transfer_ownership(EntityId entity_id, ClientID new_owner_id);

private:
    NetworkRegistry* network_registry_;
};

} // namespace ecscope::networking