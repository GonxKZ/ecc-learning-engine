#pragma once

#include "network_types.hpp"
#include <memory>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    using socket_t = SOCKET;
    constexpr socket_t INVALID_SOCKET_VALUE = INVALID_SOCKET;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    using socket_t = int;
    constexpr socket_t INVALID_SOCKET_VALUE = -1;
#endif

namespace ecscope::networking {

// Forward declarations
class NetworkBuffer;
class NetworkMessage;

/**
 * @brief Base Network Socket Interface
 * 
 * Abstract base class for all network socket implementations.
 * Provides common interface for TCP and UDP sockets.
 */
class NetworkSocket {
public:
    NetworkSocket() = default;
    virtual ~NetworkSocket() = default;

    // Non-copyable but movable
    NetworkSocket(const NetworkSocket&) = delete;
    NetworkSocket& operator=(const NetworkSocket&) = delete;
    NetworkSocket(NetworkSocket&&) = default;
    NetworkSocket& operator=(NetworkSocket&&) = default;

    // Core socket operations
    virtual NetworkResult<void> bind(const NetworkAddress& address) = 0;
    virtual NetworkResult<void> connect(const NetworkAddress& address) = 0;
    virtual NetworkResult<void> listen(int backlog = 128) = 0;
    virtual NetworkResult<std::unique_ptr<NetworkSocket>> accept() = 0;
    virtual NetworkResult<void> disconnect() = 0;

    // Data operations
    virtual NetworkResult<size_t> send(const void* data, size_t size) = 0;
    virtual NetworkResult<size_t> receive(void* buffer, size_t size) = 0;
    virtual NetworkResult<size_t> send_to(const void* data, size_t size, const NetworkAddress& address) = 0;
    virtual NetworkResult<size_t> receive_from(void* buffer, size_t size, NetworkAddress& address) = 0;

    // Socket state
    virtual bool is_connected() const = 0;
    virtual bool is_listening() const = 0;
    virtual ConnectionState get_state() const = 0;
    virtual NetworkAddress get_local_address() const = 0;
    virtual NetworkAddress get_remote_address() const = 0;

    // Configuration
    virtual NetworkResult<void> set_blocking(bool blocking) = 0;
    virtual NetworkResult<void> set_reuse_address(bool reuse) = 0;
    virtual NetworkResult<void> set_no_delay(bool no_delay) = 0;
    virtual NetworkResult<void> set_receive_buffer_size(size_t size) = 0;
    virtual NetworkResult<void> set_send_buffer_size(size_t size) = 0;

    // Statistics
    virtual NetworkStats get_statistics() const = 0;
    virtual void reset_statistics() = 0;

protected:
    mutable NetworkStats statistics_;
    ConnectionState state_ = ConnectionState::Disconnected;
    socket_t socket_ = INVALID_SOCKET_VALUE;
};

/**
 * @brief TCP Socket Implementation
 * 
 * Reliable, connection-oriented socket implementation using TCP.
 */
class TCPSocket : public NetworkSocket {
public:
    TCPSocket();
    explicit TCPSocket(socket_t existing_socket);
    ~TCPSocket() override;

    // NetworkSocket implementation
    NetworkResult<void> bind(const NetworkAddress& address) override;
    NetworkResult<void> connect(const NetworkAddress& address) override;
    NetworkResult<void> listen(int backlog = 128) override;
    NetworkResult<std::unique_ptr<NetworkSocket>> accept() override;
    NetworkResult<void> disconnect() override;

    NetworkResult<size_t> send(const void* data, size_t size) override;
    NetworkResult<size_t> receive(void* buffer, size_t size) override;
    NetworkResult<size_t> send_to(const void* data, size_t size, const NetworkAddress& address) override;
    NetworkResult<size_t> receive_from(void* buffer, size_t size, NetworkAddress& address) override;

    bool is_connected() const override;
    bool is_listening() const override;
    ConnectionState get_state() const override;
    NetworkAddress get_local_address() const override;
    NetworkAddress get_remote_address() const override;

    NetworkResult<void> set_blocking(bool blocking) override;
    NetworkResult<void> set_reuse_address(bool reuse) override;
    NetworkResult<void> set_no_delay(bool no_delay) override;
    NetworkResult<void> set_receive_buffer_size(size_t size) override;
    NetworkResult<void> set_send_buffer_size(size_t size) override;

    NetworkStats get_statistics() const override;
    void reset_statistics() override;

private:
    void initialize_socket();
    void cleanup_socket();
    NetworkAddress socket_address_to_network_address(const sockaddr_in& addr) const;
    sockaddr_in network_address_to_socket_address(const NetworkAddress& addr) const;

    NetworkAddress local_address_;
    NetworkAddress remote_address_;
    bool is_listening_ = false;
    mutable std::mutex statistics_mutex_;
};

/**
 * @brief UDP Socket Implementation
 * 
 * Unreliable, connectionless socket implementation using UDP.
 */
class UDPSocket : public NetworkSocket {
public:
    UDPSocket();
    ~UDPSocket() override;

    // NetworkSocket implementation
    NetworkResult<void> bind(const NetworkAddress& address) override;
    NetworkResult<void> connect(const NetworkAddress& address) override;
    NetworkResult<void> listen(int backlog = 128) override;
    NetworkResult<std::unique_ptr<NetworkSocket>> accept() override;
    NetworkResult<void> disconnect() override;

    NetworkResult<size_t> send(const void* data, size_t size) override;
    NetworkResult<size_t> receive(void* buffer, size_t size) override;
    NetworkResult<size_t> send_to(const void* data, size_t size, const NetworkAddress& address) override;
    NetworkResult<size_t> receive_from(void* buffer, size_t size, NetworkAddress& address) override;

    bool is_connected() const override;
    bool is_listening() const override;
    ConnectionState get_state() const override;
    NetworkAddress get_local_address() const override;
    NetworkAddress get_remote_address() const override;

    NetworkResult<void> set_blocking(bool blocking) override;
    NetworkResult<void> set_reuse_address(bool reuse) override;
    NetworkResult<void> set_no_delay(bool no_delay) override;
    NetworkResult<void> set_receive_buffer_size(size_t size) override;
    NetworkResult<void> set_send_buffer_size(size_t size) override;

    NetworkStats get_statistics() const override;
    void reset_statistics() override;

private:
    void initialize_socket();
    void cleanup_socket();
    NetworkAddress socket_address_to_network_address(const sockaddr_in& addr) const;
    sockaddr_in network_address_to_socket_address(const NetworkAddress& addr) const;

    NetworkAddress local_address_;
    NetworkAddress remote_address_;
    bool has_remote_address_ = false;
    mutable std::mutex statistics_mutex_;
};

/**
 * @brief Reliable UDP Socket Implementation
 * 
 * UDP socket with custom reliability layer for ordered, guaranteed delivery.
 * Implements packet acknowledgment, retransmission, and ordering.
 */
class ReliableUDPSocket : public NetworkSocket {
public:
    struct ReliableConfig {
        std::chrono::milliseconds ack_timeout{100};
        std::chrono::milliseconds retransmit_timeout{500};
        uint32_t max_retransmits = 5;
        size_t window_size = 64;  // Sliding window size
        bool enable_ordering = true;
        bool enable_congestion_control = true;
    };

    explicit ReliableUDPSocket(const ReliableConfig& config = {});
    ~ReliableUDPSocket() override;

    // NetworkSocket implementation
    NetworkResult<void> bind(const NetworkAddress& address) override;
    NetworkResult<void> connect(const NetworkAddress& address) override;
    NetworkResult<void> listen(int backlog = 128) override;
    NetworkResult<std::unique_ptr<NetworkSocket>> accept() override;
    NetworkResult<void> disconnect() override;

    NetworkResult<size_t> send(const void* data, size_t size) override;
    NetworkResult<size_t> receive(void* buffer, size_t size) override;
    NetworkResult<size_t> send_to(const void* data, size_t size, const NetworkAddress& address) override;
    NetworkResult<size_t> receive_from(void* buffer, size_t size, NetworkAddress& address) override;

    bool is_connected() const override;
    bool is_listening() const override;
    ConnectionState get_state() const override;
    NetworkAddress get_local_address() const override;
    NetworkAddress get_remote_address() const override;

    NetworkResult<void> set_blocking(bool blocking) override;
    NetworkResult<void> set_reuse_address(bool reuse) override;
    NetworkResult<void> set_no_delay(bool no_delay) override;
    NetworkResult<void> set_receive_buffer_size(size_t size) override;
    NetworkResult<void> set_send_buffer_size(size_t size) override;

    NetworkStats get_statistics() const override;
    void reset_statistics() override;

    // Reliable UDP specific methods
    void set_reliability_config(const ReliableConfig& config);
    ReliableConfig get_reliability_config() const;
    void update(); // Process acknowledgments and retransmissions

private:
    struct PendingPacket {
        uint32_t sequence_number;
        NetworkTimestamp send_time;
        NetworkTimestamp last_retransmit_time;
        uint32_t retransmit_count;
        std::vector<uint8_t> data;
        NetworkAddress destination;
    };

    struct ReceivedPacket {
        uint32_t sequence_number;
        NetworkTimestamp receive_time;
        std::vector<uint8_t> data;
        NetworkAddress source;
    };

    void process_incoming_packets();
    void process_acknowledgments();
    void process_retransmissions();
    void send_acknowledgment(uint32_t sequence_number, const NetworkAddress& address);
    uint32_t get_next_sequence_number();

    std::unique_ptr<UDPSocket> udp_socket_;
    ReliableConfig config_;
    
    // Reliability state
    std::atomic<uint32_t> next_sequence_number_{1};
    std::atomic<uint32_t> next_expected_sequence_{1};
    
    // Pending packets (waiting for ACK)
    std::map<uint32_t, PendingPacket> pending_packets_;
    std::mutex pending_packets_mutex_;
    
    // Received packets buffer
    std::map<uint32_t, ReceivedPacket> received_packets_;
    std::queue<ReceivedPacket> ordered_packets_;
    std::mutex receive_buffer_mutex_;
    
    // Worker thread for processing
    std::thread worker_thread_;
    std::atomic<bool> should_stop_{false};
    std::condition_variable worker_cv_;
    std::mutex worker_mutex_;
};

/**
 * @brief Socket Factory
 * 
 * Factory class for creating different types of network sockets.
 */
class SocketFactory {
public:
    static std::unique_ptr<NetworkSocket> create_tcp_socket();
    static std::unique_ptr<NetworkSocket> create_udp_socket();
    static std::unique_ptr<NetworkSocket> create_reliable_udp_socket(
        const ReliableUDPSocket::ReliableConfig& config = {});
    
    static std::unique_ptr<NetworkSocket> create_socket(TransportProtocol protocol);
};

/**
 * @brief Network Socket Manager
 * 
 * Manages multiple sockets and provides event-driven I/O using select/epoll.
 */
class SocketManager {
public:
    struct SocketEvent {
        NetworkSocket* socket;
        bool can_read = false;
        bool can_write = false;
        bool has_error = false;
    };

    SocketManager();
    ~SocketManager();

    // Socket management
    void add_socket(std::shared_ptr<NetworkSocket> socket);
    void remove_socket(NetworkSocket* socket);
    void clear_sockets();

    // Event processing
    std::vector<SocketEvent> poll(std::chrono::milliseconds timeout = std::chrono::milliseconds(0));
    void run_event_loop(std::function<void(const std::vector<SocketEvent>&)> event_handler);
    void stop_event_loop();

private:
    struct SocketEntry {
        std::shared_ptr<NetworkSocket> socket;
        socket_t native_handle;
    };

    std::vector<SocketEntry> sockets_;
    std::mutex sockets_mutex_;
    std::atomic<bool> should_stop_event_loop_{false};

#ifdef _WIN32
    // Windows implementation using select
    void poll_sockets_select(std::vector<SocketEvent>& events, std::chrono::milliseconds timeout);
#else
    // Linux/Unix implementation using epoll
    int epoll_fd_ = -1;
    void poll_sockets_epoll(std::vector<SocketEvent>& events, std::chrono::milliseconds timeout);
#endif
};

} // namespace ecscope::networking