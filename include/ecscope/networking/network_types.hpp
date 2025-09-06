#pragma once

/**
 * @file networking/network_types.hpp
 * @brief Core Network Types and Constants for ECScope Distributed ECS
 * 
 * This header defines fundamental networking types, constants, and configurations
 * for the ECScope educational distributed ECS system. It provides:
 * 
 * Core Network Types:
 * - NetworkID: Unique network identifiers for entities and clients
 * - Timestamps: Network timing and synchronization primitives
 * - Protocol enums: Network protocol and message type definitions
 * - Network addresses and connection info
 * 
 * Educational Features:
 * - Clear documentation of networking concepts
 * - Performance characteristics of different approaches
 * - Real-world networking protocol insights
 * - Debugging and analysis capabilities
 * 
 * Performance Optimizations:
 * - Fixed-size types for predictable serialization
 * - Cache-friendly data layouts
 * - Minimal memory overhead
 * - Zero-copy networking primitives where possible
 * 
 * @author ECScope Educational ECS Framework - Networking Layer
 * @date 2024
 */

#include "../core/types.hpp"
#include "../core/id.hpp"
#include "../memory/memory_tracker.hpp"
#include <chrono>
#include <cstdint>
#include <array>
#include <string>
#include <variant>

namespace ecscope::networking {

//=============================================================================
// Core Network Identifiers
//=============================================================================

/** 
 * @brief Network Entity Identifier
 * 
 * Unique identifier for entities across the network. Unlike local Entity IDs,
 * NetworkEntityID must be globally unique and deterministic across all clients.
 * 
 * Educational Context:
 * - Lower 32 bits: Local entity index (for fast local lookup)
 * - Upper 32 bits: Client/Server ID that created the entity
 * 
 * This design allows for efficient local access while maintaining global uniqueness.
 */
using NetworkEntityID = u64;

/**
 * @brief Client/Peer Identifier  
 * 
 * Unique identifier for each client or server in the distributed system.
 * Assigned by the authoritative server upon connection.
 */
using ClientID = u32;

/**
 * @brief Network Session Identifier
 * 
 * Identifies a specific game session/match. Useful for preventing stale
 * packets from affecting new sessions.
 */
using SessionID = u64;

/**
 * @brief Network Component Version
 * 
 * Version number for component data, used for delta compression and
 * conflict resolution in distributed ownership scenarios.
 */
using ComponentVersion = u32;

//=============================================================================
// Network Timing and Synchronization
//=============================================================================

/**
 * @brief Network Timestamp
 * 
 * High-precision timestamp for network synchronization and lag compensation.
 * Uses microseconds since epoch for maximum precision while maintaining
 * reasonable range (can represent ~584,000 years).
 */
using NetworkTimestamp = u64;

/**
 * @brief Network Tick/Frame Number
 * 
 * Logical time unit for deterministic simulation. All clients advance
 * simulation in discrete ticks for consistent state.
 */
using NetworkTick = u64;

/**
 * @brief Network timing utilities
 */
namespace timing {
    /** @brief Get current network timestamp in microseconds */
    inline NetworkTimestamp now() noexcept {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count();
    }
    
    /** @brief Convert milliseconds to NetworkTimestamp */
    constexpr NetworkTimestamp from_ms(u64 ms) noexcept {
        return ms * 1000;
    }
    
    /** @brief Convert NetworkTimestamp to milliseconds */
    constexpr u64 to_ms(NetworkTimestamp timestamp) noexcept {
        return timestamp / 1000;
    }
    
    /** @brief Calculate time difference in microseconds */
    constexpr i64 diff_us(NetworkTimestamp t1, NetworkTimestamp t2) noexcept {
        return static_cast<i64>(t1) - static_cast<i64>(t2);
    }
    
    /** @brief Calculate time difference in milliseconds */
    constexpr i64 diff_ms(NetworkTimestamp t1, NetworkTimestamp t2) noexcept {
        return diff_us(t1, t2) / 1000;
    }
}

//=============================================================================
// Network Protocol Definitions
//=============================================================================

/**
 * @brief Network Transport Protocol
 * 
 * Defines the underlying transport mechanism used for network communication.
 * Each has different characteristics suitable for different use cases.
 */
enum class TransportProtocol : u8 {
    /** 
     * @brief Custom UDP with reliability layers
     * 
     * Benefits:
     * - Low latency (no connection overhead)
     * - Custom reliability (only for critical data)
     * - Efficient for real-time games
     * - Educational: shows how UDP reliability works
     * 
     * Drawbacks:
     * - Complex implementation
     * - May be blocked by corporate firewalls
     */
    ReliableUDP,
    
    /**
     * @brief Standard TCP connections
     * 
     * Benefits:
     * - Guaranteed delivery and ordering
     * - Built-in congestion control
     * - Works through most firewalls
     * 
     * Drawbacks:
     * - Higher latency due to head-of-line blocking
     * - Connection overhead
     * - Less suitable for real-time gaming
     */
    TCP,
    
    /**
     * @brief WebSocket (for web clients)
     * 
     * Benefits:
     * - Works in browsers
     * - Good firewall compatibility
     * - Built-in framing
     * 
     * Drawbacks:
     * - Additional HTTP overhead
     * - Limited to web environments
     */
    WebSocket,
    
    /**
     * @brief Local simulation (no network)
     * 
     * For testing and single-player modes without network overhead.
     */
    LocalOnly
};

/**
 * @brief Message Reliability Level
 * 
 * Defines how important it is that a message reaches its destination.
 * This allows the networking system to optimize bandwidth and latency
 * based on message importance.
 */
enum class Reliability : u8 {
    /** 
     * @brief Unreliable, unordered delivery
     * 
     * Use for: Position updates, visual effects, non-critical data
     * that's frequently updated. If lost, newer data will replace it.
     * 
     * Characteristics:
     * - Minimal bandwidth overhead
     * - Lowest latency
     * - May be lost or arrive out of order
     */
    Unreliable,
    
    /**
     * @brief Unreliable but ordered delivery
     * 
     * Use for: Animation states, temporary UI updates where order matters
     * but occasional loss is acceptable.
     * 
     * Characteristics:
     * - Low bandwidth overhead
     * - Low latency
     * - May be lost but order is maintained
     */
    UnreliableOrdered,
    
    /**
     * @brief Reliable, unordered delivery
     * 
     * Use for: Chat messages, item pickups, events where delivery is
     * critical but order doesn't matter.
     * 
     * Characteristics:
     * - Guaranteed delivery
     * - May arrive out of order
     * - Higher bandwidth due to ACKs
     */
    Reliable,
    
    /**
     * @brief Reliable, ordered delivery
     * 
     * Use for: Game state changes, player actions, critical events where
     * both delivery and order are essential.
     * 
     * Characteristics:
     * - Guaranteed delivery and order
     * - Highest bandwidth overhead
     * - Potential for head-of-line blocking
     */
    ReliableOrdered
};

/**
 * @brief Network Message Priority
 * 
 * Defines the priority of messages for bandwidth management and
 * packet batching optimizations.
 */
enum class MessagePriority : u8 {
    /** @brief Critical system messages (disconnect, handshake) */
    Critical = 0,
    
    /** @brief Important game state changes */
    High = 1,
    
    /** @brief Regular gameplay messages */
    Normal = 2,
    
    /** @brief Non-essential updates (cosmetic effects) */
    Low = 3,
    
    /** @brief Background data (statistics, telemetry) */
    Background = 4
};

//=============================================================================
// Network Address and Connection Info
//=============================================================================

/**
 * @brief Network Address
 * 
 * Represents an address for network communication. Can be IPv4, IPv6,
 * or local addresses depending on the transport protocol.
 */
struct NetworkAddress {
    /** @brief Address family/type */
    enum Type : u8 {
        IPv4,
        IPv6,
        Local,      // For local/loopback connections
        WebSocket   // For WebSocket URLs
    };
    
    Type type{IPv4};
    u16 port{0};
    
    /** @brief Address data (IPv4: 4 bytes, IPv6: 16 bytes, others: string) */
    std::variant<
        std::array<u8, 4>,   // IPv4
        std::array<u8, 16>,  // IPv6
        std::string          // Local/WebSocket
    > address_data;
    
    /** @brief Create IPv4 address */
    static NetworkAddress ipv4(u8 a, u8 b, u8 c, u8 d, u16 port_num) {
        NetworkAddress addr;
        addr.type = IPv4;
        addr.port = port_num;
        addr.address_data = std::array<u8, 4>{a, b, c, d};
        return addr;
    }
    
    /** @brief Create local/loopback address */
    static NetworkAddress local(u16 port_num) {
        return ipv4(127, 0, 0, 1, port_num);
    }
    
    /** @brief Create WebSocket address */
    static NetworkAddress websocket(const std::string& url) {
        NetworkAddress addr;
        addr.type = WebSocket;
        addr.port = 0;  // Port embedded in URL
        addr.address_data = url;
        return addr;
    }
    
    /** @brief Convert to string representation */
    std::string to_string() const;
};

/**
 * @brief Connection State
 * 
 * Tracks the current state of a network connection for both
 * clients and servers.
 */
enum class ConnectionState : u8 {
    /** @brief Not connected */
    Disconnected,
    
    /** @brief Attempting to connect */
    Connecting,
    
    /** @brief Connected and authenticated */
    Connected,
    
    /** @brief Connection is being closed gracefully */
    Disconnecting,
    
    /** @brief Connection failed or was forcibly closed */
    Failed,
    
    /** @brief Connection timed out */
    TimedOut
};

/**
 * @brief Network Connection Statistics
 * 
 * Educational statistics about network performance and characteristics.
 * Useful for understanding network behavior and optimizing protocols.
 */
struct NetworkStats {
    // Latency measurements (in microseconds)
    u32 ping_min{0};
    u32 ping_max{0};
    u32 ping_average{0};
    u32 ping_current{0};
    
    // Jitter (variation in latency)
    u32 jitter_average{0};
    u32 jitter_max{0};
    
    // Packet statistics
    u64 packets_sent{0};
    u64 packets_received{0};
    u64 packets_lost{0};
    u64 packets_duplicate{0};
    u64 packets_out_of_order{0};
    
    // Bandwidth usage (bytes per second)
    u32 bytes_sent_per_sec{0};
    u32 bytes_received_per_sec{0};
    
    // Reliability layer statistics
    u32 acks_sent{0};
    u32 acks_received{0};
    u32 retransmissions{0};
    
    // Connection quality metrics
    f32 packet_loss_percentage{0.0f};
    f32 connection_quality{1.0f};  // 0.0 = terrible, 1.0 = perfect
    
    /** @brief Calculate packet loss percentage */
    void update_packet_loss() {
        if (packets_sent > 0) {
            packet_loss_percentage = static_cast<f32>(packets_lost) / 
                                   static_cast<f32>(packets_sent) * 100.0f;
        }
    }
    
    /** @brief Calculate overall connection quality */
    void update_connection_quality() {
        f32 loss_factor = 1.0f - (packet_loss_percentage / 100.0f);
        f32 latency_factor = std::max(0.0f, 1.0f - (ping_average / 200000.0f)); // 200ms baseline
        f32 jitter_factor = std::max(0.0f, 1.0f - (jitter_average / 50000.0f));  // 50ms baseline
        
        connection_quality = (loss_factor * 0.5f) + (latency_factor * 0.3f) + (jitter_factor * 0.2f);
        connection_quality = std::clamp(connection_quality, 0.0f, 1.0f);
    }
    
    /** @brief Reset all statistics */
    void reset() {
        *this = NetworkStats{};
    }
};

//=============================================================================
// Memory Management for Networking
//=============================================================================

namespace memory {
    /** @brief Memory categories for network allocations */
    constexpr ecscope::memory::AllocationCategory NETWORK_BUFFERS = 
        ecscope::memory::AllocationCategory::Asset_Loaders;  // Reuse existing category
    constexpr ecscope::memory::AllocationCategory NETWORK_MESSAGES = 
        ecscope::memory::AllocationCategory::Debug_Tools;    // Reuse existing category
    constexpr ecscope::memory::AllocationCategory NETWORK_CONNECTIONS = 
        ecscope::memory::AllocationCategory::Debug_Tools;    // Reuse existing category
}

//=============================================================================
// Configuration Constants
//=============================================================================

namespace constants {
    /** @brief Maximum message size in bytes (64KB) */
    constexpr usize MAX_MESSAGE_SIZE = 65536;
    
    /** @brief Maximum number of concurrent connections */
    constexpr u32 MAX_CONNECTIONS = 1000;
    
    /** @brief Default server listen port */
    constexpr u16 DEFAULT_SERVER_PORT = 7777;
    
    /** @brief Connection timeout in microseconds (30 seconds) */
    constexpr u64 CONNECTION_TIMEOUT_US = 30 * 1000 * 1000;
    
    /** @brief Ping interval in microseconds (1 second) */
    constexpr u64 PING_INTERVAL_US = 1000 * 1000;
    
    /** @brief Maximum packet buffer size */
    constexpr usize PACKET_BUFFER_SIZE = 8192;
    
    /** @brief Network tick rate (60 Hz) */
    constexpr u32 NETWORK_TICK_RATE = 60;
    
    /** @brief Network tick interval in microseconds */
    constexpr u64 NETWORK_TICK_INTERVAL_US = 1000000 / NETWORK_TICK_RATE;
}

} // namespace ecscope::networking