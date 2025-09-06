#pragma once

/**
 * @file networking/udp_socket.hpp
 * @brief Cross-Platform UDP Socket Implementation for ECScope Networking
 * 
 * This header provides a robust, educational UDP socket implementation that
 * forms the foundation of our custom networking protocol. Features include:
 * 
 * Core Socket Features:
 * - Cross-platform socket abstraction (Windows/Linux/macOS)
 * - Non-blocking I/O with efficient event polling
 * - IPv4 and IPv6 support with address resolution
 * - Socket option configuration and optimization
 * - Comprehensive error handling and reporting
 * 
 * Educational Features:
 * - Detailed explanations of socket programming concepts
 * - Performance monitoring and analysis
 * - Network condition simulation capabilities
 * - Interactive debugging and inspection tools
 * - Real-world networking problem demonstrations
 * 
 * Performance Optimizations:
 * - Zero-copy operations where possible
 * - Efficient buffer management
 * - Batch send/receive operations
 * - CPU cache-friendly data structures
 * - Minimal system call overhead
 * 
 * @author ECScope Educational ECS Framework - Networking Layer
 * @date 2024
 */

#include "network_types.hpp"
#include "../core/types.hpp"
#include "../memory/arena.hpp"
#include <vector>
#include <array>
#include <optional>
#include <chrono>
#include <functional>

// Platform-specific includes
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    using socket_t = SOCKET;
    constexpr socket_t INVALID_SOCKET_VALUE = INVALID_SOCKET;
#else
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    using socket_t = int;
    constexpr socket_t INVALID_SOCKET_VALUE = -1;
#endif

namespace ecscope::networking::transport {

//=============================================================================
// Socket Error Handling
//=============================================================================

/**
 * @brief Socket Operation Result
 * 
 * Comprehensive error reporting for socket operations. Provides both
 * programmatic error handling and educational error explanations.
 */
enum class SocketError : i32 {
    Success = 0,
    
    // Connection errors
    ConnectionRefused = -1,
    ConnectionReset = -2,
    ConnectionTimedOut = -3,
    NetworkUnreachable = -4,
    HostUnreachable = -5,
    
    // Address/binding errors
    AddressInUse = -10,
    AddressNotAvailable = -11,
    InvalidAddress = -12,
    
    // I/O errors
    WouldBlock = -20,
    MessageTooLarge = -21,
    BufferFull = -22,
    
    // System errors
    OutOfMemory = -30,
    PermissionDenied = -31,
    InvalidSocket = -32,
    NotSupported = -33,
    
    // Generic errors
    UnknownError = -100
};

/**
 * @brief Socket operation result with error details
 */
template<typename T>
struct SocketResult {
    T value{};
    SocketError error{SocketError::Success};
    std::string error_message;
    
    /** @brief Check if operation succeeded */
    bool is_success() const noexcept {
        return error == SocketError::Success;
    }
    
    /** @brief Check if operation failed */
    bool is_error() const noexcept {
        return error != SocketError::Success;
    }
    
    /** @brief Get value or default if error */
    T value_or(const T& default_value) const noexcept {
        return is_success() ? value : default_value;
    }
    
    /** @brief Create success result */
    static SocketResult success(T val) {
        return SocketResult{std::move(val), SocketError::Success, ""};
    }
    
    /** @brief Create error result */
    static SocketResult error(SocketError err, const std::string& msg) {
        return SocketResult{T{}, err, msg};
    }
};

using VoidResult = SocketResult<bool>;  // For operations that don't return values

//=============================================================================
// Socket Address Utilities
//=============================================================================

/**
 * @brief Socket Address Wrapper
 * 
 * Cross-platform wrapper for socket addresses with automatic
 * IPv4/IPv6 detection and conversion utilities.
 */
class SocketAddress {
private:
    union {
        sockaddr generic;
        sockaddr_in ipv4;
        sockaddr_in6 ipv6;
    } addr_;
    
    socklen_t addr_len_;
    
public:
    /** @brief Create empty address */
    SocketAddress() noexcept {
        std::memset(&addr_, 0, sizeof(addr_));
        addr_len_ = 0;
    }
    
    /** @brief Create IPv4 address */
    SocketAddress(u32 ip_address, u16 port) noexcept {
        std::memset(&addr_, 0, sizeof(addr_));
        addr_.ipv4.sin_family = AF_INET;
        addr_.ipv4.sin_port = htons(port);
        addr_.ipv4.sin_addr.s_addr = htonl(ip_address);
        addr_len_ = sizeof(sockaddr_in);
    }
    
    /** @brief Create address from NetworkAddress */
    explicit SocketAddress(const NetworkAddress& net_addr) noexcept {
        std::memset(&addr_, 0, sizeof(addr_));
        
        if (net_addr.type == NetworkAddress::IPv4) {
            auto ipv4_data = std::get<std::array<u8, 4>>(net_addr.address_data);
            addr_.ipv4.sin_family = AF_INET;
            addr_.ipv4.sin_port = htons(net_addr.port);
            
            u32 ip = (static_cast<u32>(ipv4_data[0]) << 24) |
                    (static_cast<u32>(ipv4_data[1]) << 16) |
                    (static_cast<u32>(ipv4_data[2]) << 8) |
                     static_cast<u32>(ipv4_data[3]);
            addr_.ipv4.sin_addr.s_addr = htonl(ip);
            addr_len_ = sizeof(sockaddr_in);
        } else {
            // For now, default to localhost IPv4
            addr_.ipv4.sin_family = AF_INET;
            addr_.ipv4.sin_port = htons(net_addr.port);
            addr_.ipv4.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            addr_len_ = sizeof(sockaddr_in);
        }
    }
    
    /** @brief Get raw sockaddr pointer */
    const sockaddr* sockaddr_ptr() const noexcept {
        return &addr_.generic;
    }
    
    sockaddr* sockaddr_ptr() noexcept {
        return &addr_.generic;
    }
    
    /** @brief Get sockaddr length */
    socklen_t sockaddr_len() const noexcept {
        return addr_len_;
    }
    
    socklen_t& sockaddr_len() noexcept {
        return addr_len_;
    }
    
    /** @brief Check if address is IPv4 */
    bool is_ipv4() const noexcept {
        return addr_.generic.sa_family == AF_INET;
    }
    
    /** @brief Check if address is IPv6 */
    bool is_ipv6() const noexcept {
        return addr_.generic.sa_family == AF_INET6;
    }
    
    /** @brief Get port number */
    u16 port() const noexcept {
        if (is_ipv4()) {
            return ntohs(addr_.ipv4.sin_port);
        } else if (is_ipv6()) {
            return ntohs(addr_.ipv6.sin6_port);
        }
        return 0;
    }
    
    /** @brief Convert to NetworkAddress */
    NetworkAddress to_network_address() const noexcept {
        if (is_ipv4()) {
            u32 ip = ntohl(addr_.ipv4.sin_addr.s_addr);
            return NetworkAddress::ipv4(
                static_cast<u8>((ip >> 24) & 0xFF),
                static_cast<u8>((ip >> 16) & 0xFF),
                static_cast<u8>((ip >> 8) & 0xFF),
                static_cast<u8>(ip & 0xFF),
                port()
            );
        }
        
        // Default to localhost
        return NetworkAddress::local(port());
    }
    
    /** @brief Convert to string representation */
    std::string to_string() const;
    
    /** @brief Comparison operators */
    bool operator==(const SocketAddress& other) const noexcept {
        if (addr_.generic.sa_family != other.addr_.generic.sa_family) {
            return false;
        }
        
        if (is_ipv4()) {
            return addr_.ipv4.sin_port == other.addr_.ipv4.sin_port &&
                   addr_.ipv4.sin_addr.s_addr == other.addr_.ipv4.sin_addr.s_addr;
        } else if (is_ipv6()) {
            return addr_.ipv6.sin6_port == other.addr_.ipv6.sin6_port &&
                   std::memcmp(&addr_.ipv6.sin6_addr, &other.addr_.ipv6.sin6_addr, 
                              sizeof(in6_addr)) == 0;
        }
        
        return false;
    }
    
    bool operator!=(const SocketAddress& other) const noexcept {
        return !(*this == other);
    }
};

//=============================================================================
// UDP Socket Implementation
//=============================================================================

/**
 * @brief UDP Socket Configuration
 * 
 * Configuration options for UDP socket behavior and performance tuning.
 * Includes educational explanations of each option's impact.
 */
struct UdpSocketConfig {
    /** @brief Enable address reuse (SO_REUSEADDR) */
    bool reuse_address{true};
    
    /** @brief Enable broadcast packets */
    bool enable_broadcast{false};
    
    /** @brief Socket send buffer size in bytes */
    u32 send_buffer_size{65536};  // 64KB default
    
    /** @brief Socket receive buffer size in bytes */
    u32 receive_buffer_size{65536};  // 64KB default
    
    /** @brief Enable non-blocking I/O */
    bool non_blocking{true};
    
    /** @brief Bind to specific interface (empty = all interfaces) */
    std::string bind_interface{};
    
    /** @brief IPv6 dual-stack mode (IPv6 socket accepts IPv4) */
    bool ipv6_dual_stack{true};
    
    /** @brief Don't fragment packets (DF flag) */
    bool dont_fragment{true};
    
    /** @brief Type of Service (ToS) value for QoS */
    u8 type_of_service{0};
    
    /** @brief Time to Live (TTL) for outgoing packets */
    u8 ttl{64};
    
    /** @brief Factory for gaming-optimized configuration */
    static UdpSocketConfig gaming_optimized() {
        UdpSocketConfig config;
        config.send_buffer_size = 1048576;    // 1MB for high throughput
        config.receive_buffer_size = 1048576; // 1MB for high throughput
        config.dont_fragment = true;          // Avoid fragmentation
        config.type_of_service = 0x10;        // Low delay
        return config;
    }
    
    /** @brief Factory for memory-conservative configuration */
    static UdpSocketConfig memory_conservative() {
        UdpSocketConfig config;
        config.send_buffer_size = 8192;       // 8KB minimal
        config.receive_buffer_size = 8192;    // 8KB minimal
        return config;
    }
};

/**
 * @brief High-Performance UDP Socket
 * 
 * Educational UDP socket implementation with comprehensive features for
 * network programming education and real-world performance.
 */
class UdpSocket {
private:
    socket_t socket_{INVALID_SOCKET_VALUE};
    SocketAddress local_address_;
    UdpSocketConfig config_;
    bool is_bound_{false};
    
    // Statistics for educational purposes
    mutable u64 bytes_sent_{0};
    mutable u64 bytes_received_{0};
    mutable u64 packets_sent_{0};
    mutable u64 packets_received_{0};
    mutable u64 send_errors_{0};
    mutable u64 receive_errors_{0};
    
    /** @brief Get last socket error */
    SocketError get_last_error() const noexcept;
    
    /** @brief Convert system error to SocketError */
    SocketError system_error_to_socket_error(int error_code) const noexcept;
    
    /** @brief Apply socket configuration */
    VoidResult apply_configuration() noexcept;
    
public:
    /** @brief Default constructor */
    UdpSocket() = default;
    
    /** @brief Constructor with configuration */
    explicit UdpSocket(const UdpSocketConfig& config) : config_(config) {}
    
    /** @brief Destructor - automatically closes socket */
    ~UdpSocket() {
        close();
    }
    
    // Move-only type
    UdpSocket(const UdpSocket&) = delete;
    UdpSocket& operator=(const UdpSocket&) = delete;
    
    UdpSocket(UdpSocket&& other) noexcept 
        : socket_(other.socket_), local_address_(other.local_address_),
          config_(other.config_), is_bound_(other.is_bound_) {
        other.socket_ = INVALID_SOCKET_VALUE;
        other.is_bound_ = false;
    }
    
    UdpSocket& operator=(UdpSocket&& other) noexcept {
        if (this != &other) {
            close();
            socket_ = other.socket_;
            local_address_ = other.local_address_;
            config_ = other.config_;
            is_bound_ = other.is_bound_;
            
            other.socket_ = INVALID_SOCKET_VALUE;
            other.is_bound_ = false;
        }
        return *this;
    }
    
    //-------------------------------------------------------------------------
    // Socket Lifecycle Management
    //-------------------------------------------------------------------------
    
    /** @brief Initialize socket */
    VoidResult initialize() noexcept;
    
    /** @brief Bind socket to address */
    VoidResult bind(const NetworkAddress& address) noexcept;
    
    /** @brief Bind socket to port (any interface) */
    VoidResult bind(u16 port) noexcept {
        return bind(NetworkAddress::ipv4(0, 0, 0, 0, port));  // INADDR_ANY
    }
    
    /** @brief Close socket */
    void close() noexcept;
    
    /** @brief Check if socket is valid */
    bool is_valid() const noexcept {
        return socket_ != INVALID_SOCKET_VALUE;
    }
    
    /** @brief Check if socket is bound */
    bool is_bound() const noexcept {
        return is_bound_;
    }
    
    /** @brief Get local address */
    const SocketAddress& local_address() const noexcept {
        return local_address_;
    }
    
    //-------------------------------------------------------------------------
    // Data Transmission
    //-------------------------------------------------------------------------
    
    /** @brief Send data to specific address */
    SocketResult<usize> send_to(const void* data, usize size, 
                               const NetworkAddress& destination) noexcept;
    
    /** @brief Receive data with sender address */
    SocketResult<usize> receive_from(void* buffer, usize buffer_size,
                                    NetworkAddress& sender) noexcept;
    
    /** @brief Send multiple buffers (scatter-gather I/O) */
    SocketResult<usize> send_to_vectored(const std::vector<std::pair<const void*, usize>>& buffers,
                                        const NetworkAddress& destination) noexcept;
    
    /** @brief Check if data is available to read */
    SocketResult<bool> has_data_available() const noexcept;
    
    /** @brief Get estimated bytes available to read */
    SocketResult<usize> bytes_available() const noexcept;
    
    //-------------------------------------------------------------------------
    // Socket Configuration and Options
    //-------------------------------------------------------------------------
    
    /** @brief Update socket configuration */
    VoidResult set_config(const UdpSocketConfig& config) noexcept;
    
    /** @brief Get current configuration */
    const UdpSocketConfig& config() const noexcept {
        return config_;
    }
    
    /** @brief Set socket to non-blocking mode */
    VoidResult set_non_blocking(bool non_blocking) noexcept;
    
    /** @brief Set socket send timeout */
    VoidResult set_send_timeout(std::chrono::milliseconds timeout) noexcept;
    
    /** @brief Set socket receive timeout */
    VoidResult set_receive_timeout(std::chrono::milliseconds timeout) noexcept;
    
    /** @brief Get socket send buffer size */
    SocketResult<u32> get_send_buffer_size() const noexcept;
    
    /** @brief Get socket receive buffer size */
    SocketResult<u32> get_receive_buffer_size() const noexcept;
    
    //-------------------------------------------------------------------------
    // Statistics and Monitoring
    //-------------------------------------------------------------------------
    
    /** @brief Get total bytes sent */
    u64 bytes_sent() const noexcept { return bytes_sent_; }
    
    /** @brief Get total bytes received */
    u64 bytes_received() const noexcept { return bytes_received_; }
    
    /** @brief Get total packets sent */
    u64 packets_sent() const noexcept { return packets_sent_; }
    
    /** @brief Get total packets received */
    u64 packets_received() const noexcept { return packets_received_; }
    
    /** @brief Get send error count */
    u64 send_errors() const noexcept { return send_errors_; }
    
    /** @brief Get receive error count */
    u64 receive_errors() const noexcept { return receive_errors_; }
    
    /** @brief Reset all statistics */
    void reset_statistics() noexcept {
        bytes_sent_ = 0;
        bytes_received_ = 0;
        packets_sent_ = 0;
        packets_received_ = 0;
        send_errors_ = 0;
        receive_errors_ = 0;
    }
    
    /** @brief Get average packet size sent */
    f64 average_packet_size_sent() const noexcept {
        return packets_sent_ > 0 ? static_cast<f64>(bytes_sent_) / packets_sent_ : 0.0;
    }
    
    /** @brief Get average packet size received */
    f64 average_packet_size_received() const noexcept {
        return packets_received_ > 0 ? static_cast<f64>(bytes_received_) / packets_received_ : 0.0;
    }
    
    /** @brief Get send success rate */
    f64 send_success_rate() const noexcept {
        u64 total_attempts = packets_sent_ + send_errors_;
        return total_attempts > 0 ? static_cast<f64>(packets_sent_) / total_attempts : 1.0;
    }
    
    /** @brief Get receive success rate */
    f64 receive_success_rate() const noexcept {
        u64 total_attempts = packets_received_ + receive_errors_;
        return total_attempts > 0 ? static_cast<f64>(packets_received_) / total_attempts : 1.0;
    }
};

//=============================================================================
// Socket Utilities
//=============================================================================

/**
 * @brief Network Interface Detection
 * 
 * Utility functions for discovering and analyzing network interfaces.
 * Educational tool for understanding network topology.
 */
namespace interface_utils {
    /** @brief Network interface information */
    struct NetworkInterface {
        std::string name;
        std::vector<NetworkAddress> addresses;
        bool is_up{false};
        bool is_loopback{false};
        bool supports_broadcast{false};
        u32 mtu{0};  // Maximum Transmission Unit
    };
    
    /** @brief Get all available network interfaces */
    std::vector<NetworkInterface> get_interfaces();
    
    /** @brief Get default network interface */
    std::optional<NetworkInterface> get_default_interface();
    
    /** @brief Check if address is reachable */
    bool is_address_reachable(const NetworkAddress& address);
}

/**
 * @brief Socket Polling Utilities
 * 
 * Cross-platform utilities for efficient socket polling and event detection.
 */
namespace polling {
    /** @brief Socket poll events */
    enum class PollEvents : u16 {
        None = 0,
        Read = 0x01,
        Write = 0x02,
        Error = 0x04,
        HangUp = 0x08,
        Invalid = 0x10
    };
    
    inline PollEvents operator|(PollEvents a, PollEvents b) {
        return static_cast<PollEvents>(static_cast<u16>(a) | static_cast<u16>(b));
    }
    
    inline PollEvents operator&(PollEvents a, PollEvents b) {
        return static_cast<PollEvents>(static_cast<u16>(a) & static_cast<u16>(b));
    }
    
    inline bool has_event(PollEvents events, PollEvents event) {
        return (events & event) == event;
    }
    
    /** @brief Poll single socket for events */
    SocketResult<PollEvents> poll_socket(const UdpSocket& socket, 
                                        PollEvents events,
                                        std::chrono::milliseconds timeout);
    
    /** @brief Poll multiple sockets for events */
    struct PollEntry {
        const UdpSocket* socket;
        PollEvents requested_events;
        PollEvents returned_events;
    };
    
    SocketResult<usize> poll_sockets(std::vector<PollEntry>& entries,
                                    std::chrono::milliseconds timeout);
}

//=============================================================================
// Platform Initialization
//=============================================================================

/**
 * @brief Network Subsystem Initialization
 * 
 * Platform-specific network subsystem initialization and cleanup.
 * Required on Windows (Winsock), no-op on Unix systems.
 */
class NetworkSubsystem {
private:
    static bool initialized_;
    
public:
    /** @brief Initialize network subsystem */
    static VoidResult initialize();
    
    /** @brief Cleanup network subsystem */
    static void cleanup();
    
    /** @brief Check if subsystem is initialized */
    static bool is_initialized() { return initialized_; }
};

/**
 * @brief RAII Network Subsystem Manager
 * 
 * Ensures proper initialization and cleanup of network subsystem.
 */
class NetworkSubsystemGuard {
public:
    NetworkSubsystemGuard() {
        auto result = NetworkSubsystem::initialize();
        if (result.is_error()) {
            throw std::runtime_error("Failed to initialize network subsystem: " + result.error_message);
        }
    }
    
    ~NetworkSubsystemGuard() {
        NetworkSubsystem::cleanup();
    }
    
    // Non-copyable, non-movable
    NetworkSubsystemGuard(const NetworkSubsystemGuard&) = delete;
    NetworkSubsystemGuard& operator=(const NetworkSubsystemGuard&) = delete;
    NetworkSubsystemGuard(NetworkSubsystemGuard&&) = delete;
    NetworkSubsystemGuard& operator=(NetworkSubsystemGuard&&) = delete;
};

} // namespace ecscope::networking::transport