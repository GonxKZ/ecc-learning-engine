#pragma once

#include "network_types.hpp"
#include "network_socket.hpp"
#include "network_message.hpp"
#include <memory>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <functional>
#include <chrono>

namespace ecscope::networking {

/**
 * @brief Network Connection
 * 
 * Represents a single network connection with message handling,
 * heartbeat management, and connection state tracking.
 */
class Connection {
public:
    using MessageCallback = std::function<void(Connection&, std::unique_ptr<NetworkMessage>)>;
    using StateChangeCallback = std::function<void(Connection&, ConnectionState, ConnectionState)>;
    using ErrorCallback = std::function<void(Connection&, NetworkError, const std::string&)>;

    struct Config {
        std::chrono::milliseconds heartbeat_interval{1000};
        std::chrono::milliseconds heartbeat_timeout{5000};
        size_t max_send_queue_size = 1000;
        size_t max_receive_queue_size = 1000;
        bool enable_heartbeat = true;
        bool enable_message_batching = true;
        size_t max_batch_size = 10;
    };

    Connection(std::unique_ptr<NetworkSocket> socket, ConnectionId id, const Config& config = {});
    ~Connection();

    // Non-copyable but movable
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;
    Connection(Connection&&) = default;
    Connection& operator=(Connection&&) = default;

    // Connection management
    NetworkResult<void> start();
    NetworkResult<void> disconnect();
    void update(); // Process messages and heartbeats

    // Message sending
    NetworkResult<void> send_message(std::unique_ptr<NetworkMessage> message);
    NetworkResult<void> send_message_immediate(std::unique_ptr<NetworkMessage> message);
    
    // Connection info
    ConnectionId get_id() const { return connection_id_; }
    ConnectionState get_state() const { return state_.load(); }
    NetworkAddress get_local_address() const;
    NetworkAddress get_remote_address() const;
    ClientID get_client_id() const { return client_id_; }
    void set_client_id(ClientID client_id) { client_id_ = client_id; }

    // Callbacks
    void set_message_callback(MessageCallback callback);
    void set_state_change_callback(StateChangeCallback callback);
    void set_error_callback(ErrorCallback callback);

    // Statistics
    NetworkStats get_statistics() const;
    void reset_statistics();

    // Heartbeat management
    void send_heartbeat();
    NetworkTimestamp get_last_heartbeat_time() const { return last_heartbeat_time_.load(); }
    NetworkTimestamp get_last_activity_time() const { return last_activity_time_.load(); }

private:
    void worker_thread_function();
    void process_incoming_messages();
    void process_outgoing_messages();
    void process_heartbeat();
    void handle_received_message(std::unique_ptr<NetworkMessage> message);
    void change_state(ConnectionState new_state);
    void report_error(NetworkError error, const std::string& message);

    // Socket and identification
    std::unique_ptr<NetworkSocket> socket_;
    ConnectionId connection_id_;
    ClientID client_id_ = 0;

    // State management
    std::atomic<ConnectionState> state_{ConnectionState::Disconnected};
    Config config_;

    // Message queues
    MessageQueue send_queue_;
    MessageQueue receive_queue_;

    // Heartbeat management
    std::atomic<NetworkTimestamp> last_heartbeat_time_{0};
    std::atomic<NetworkTimestamp> last_activity_time_{0};
    std::atomic<uint64_t> next_ping_id_{1};
    std::unordered_map<uint64_t, NetworkTimestamp> pending_pings_;
    mutable std::mutex ping_mutex_;

    // Threading
    std::thread worker_thread_;
    std::atomic<bool> should_stop_{false};
    std::condition_variable worker_cv_;
    std::mutex worker_mutex_;

    // Callbacks
    MessageCallback message_callback_;
    StateChangeCallback state_change_callback_;
    ErrorCallback error_callback_;
    std::mutex callback_mutex_;

    // Statistics
    mutable NetworkStats statistics_;
    mutable std::mutex statistics_mutex_;
};

/**
 * @brief Connection Pool
 * 
 * Manages multiple connections with efficient event processing.
 */
class ConnectionPool {
public:
    using ConnectionCallback = std::function<void(std::shared_ptr<Connection>)>;

    explicit ConnectionPool(size_t max_connections = 1000);
    ~ConnectionPool();

    // Connection management
    ConnectionId add_connection(std::unique_ptr<NetworkSocket> socket);
    std::shared_ptr<Connection> get_connection(ConnectionId id);
    bool remove_connection(ConnectionId id);
    void clear_connections();

    // Bulk operations
    void update_all_connections();
    void send_to_all(std::unique_ptr<NetworkMessage> message);
    void send_to_many(const std::vector<ConnectionId>& connection_ids, 
                      std::unique_ptr<NetworkMessage> message);

    // Connection queries
    size_t get_connection_count() const;
    std::vector<ConnectionId> get_all_connection_ids() const;
    std::vector<std::shared_ptr<Connection>> get_all_connections() const;
    std::vector<std::shared_ptr<Connection>> get_connections_by_state(ConnectionState state) const;

    // Callbacks
    void set_new_connection_callback(ConnectionCallback callback);
    void set_connection_lost_callback(ConnectionCallback callback);

    // Statistics
    struct PoolStats {
        size_t total_connections = 0;
        size_t active_connections = 0;
        size_t connecting_connections = 0;
        size_t disconnected_connections = 0;
        uint64_t total_messages_sent = 0;
        uint64_t total_messages_received = 0;
        uint64_t total_bytes_sent = 0;
        uint64_t total_bytes_received = 0;
    };

    PoolStats get_pool_statistics() const;

private:
    void cleanup_disconnected_connections();
    ConnectionId generate_connection_id();

    // Connection storage
    std::unordered_map<ConnectionId, std::shared_ptr<Connection>> connections_;
    mutable std::shared_mutex connections_mutex_;
    size_t max_connections_;
    std::atomic<ConnectionId> next_connection_id_{1};

    // Callbacks
    ConnectionCallback new_connection_callback_;
    ConnectionCallback connection_lost_callback_;
    std::mutex callback_mutex_;

    // Cleanup thread
    std::thread cleanup_thread_;
    std::atomic<bool> should_stop_cleanup_{false};
    std::condition_variable cleanup_cv_;
    std::mutex cleanup_mutex_;
};

/**
 * @brief Connection Manager
 * 
 * High-level interface for managing all network connections,
 * including server listening and client connections.
 */
class ConnectionManager {
public:
    struct ServerConfig {
        NetworkAddress bind_address;
        TransportProtocol protocol = TransportProtocol::ReliableUDP;
        size_t max_connections = 100;
        bool auto_accept_connections = true;
        Connection::Config connection_config;
    };

    struct ClientConfig {
        NetworkAddress server_address;
        TransportProtocol protocol = TransportProtocol::ReliableUDP;
        std::chrono::milliseconds connect_timeout{5000};
        Connection::Config connection_config;
    };

    ConnectionManager();
    ~ConnectionManager();

    // Server operations
    NetworkResult<void> start_server(const ServerConfig& config);
    NetworkResult<void> stop_server();
    bool is_server_running() const { return server_running_.load(); }

    // Client operations
    NetworkResult<ConnectionId> connect_to_server(const ClientConfig& config);
    NetworkResult<void> disconnect_from_server();
    ConnectionId get_server_connection_id() const { return server_connection_id_; }

    // Connection access
    std::shared_ptr<Connection> get_connection(ConnectionId id);
    std::vector<ConnectionId> get_all_connection_ids() const;
    size_t get_connection_count() const;

    // Message handling
    NetworkResult<void> send_message(ConnectionId connection_id, std::unique_ptr<NetworkMessage> message);
    NetworkResult<void> broadcast_message(std::unique_ptr<NetworkMessage> message);
    NetworkResult<void> send_to_many(const std::vector<ConnectionId>& connection_ids, 
                                    std::unique_ptr<NetworkMessage> message);

    // Event handling
    using MessageCallback = std::function<void(ConnectionId, std::unique_ptr<NetworkMessage>)>;
    using ConnectionEventCallback = std::function<void(ConnectionId, ConnectionState)>;
    using ErrorCallback = std::function<void(ConnectionId, NetworkError, const std::string&)>;
    using NewConnectionCallback = std::function<bool(ConnectionId, const NetworkAddress&)>;

    void set_message_callback(MessageCallback callback);
    void set_connection_event_callback(ConnectionEventCallback callback);
    void set_error_callback(ErrorCallback callback);
    void set_new_connection_callback(NewConnectionCallback callback); // Return false to reject

    // Update and processing
    void update();

    // Statistics
    ConnectionPool::PoolStats get_statistics() const;

private:
    void server_accept_thread_function();
    void handle_new_connection(std::shared_ptr<Connection> connection);
    void handle_connection_state_change(Connection& connection, ConnectionState old_state, ConnectionState new_state);
    void handle_connection_message(Connection& connection, std::unique_ptr<NetworkMessage> message);
    void handle_connection_error(Connection& connection, NetworkError error, const std::string& message);

    // Server components
    std::unique_ptr<NetworkSocket> server_socket_;
    ServerConfig server_config_;
    std::atomic<bool> server_running_{false};
    std::thread server_accept_thread_;
    std::atomic<bool> should_stop_server_{false};

    // Client components
    ConnectionId server_connection_id_ = INVALID_CONNECTION_ID;

    // Connection management
    std::unique_ptr<ConnectionPool> connection_pool_;

    // Callbacks
    MessageCallback message_callback_;
    ConnectionEventCallback connection_event_callback_;
    ErrorCallback error_callback_;
    NewConnectionCallback new_connection_callback_;
    std::mutex callback_mutex_;

    // Threading
    std::mutex manager_mutex_;
};

} // namespace ecscope::networking