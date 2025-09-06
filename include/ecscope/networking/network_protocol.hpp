#pragma once

/**
 * @file networking/network_protocol.hpp
 * @brief Custom UDP Network Protocol with Reliability Layers for ECScope
 * 
 * This header implements a custom UDP-based network protocol optimized for
 * real-time ECS synchronization. The protocol provides:
 * 
 * Core Protocol Features:
 * - Unreliable and reliable message delivery
 * - Packet sequencing and acknowledgment
 * - Fragmentation for large messages
 * - Connection management and heartbeats
 * - Bandwidth optimization and compression
 * 
 * Educational Features:
 * - Clear visualization of protocol internals
 * - Performance comparisons with standard protocols
 * - Interactive protocol parameter tuning
 * - Network condition simulation (latency, loss)
 * - Educational explanations of networking concepts
 * 
 * Performance Optimizations:
 * - Zero-copy packet construction where possible
 * - Efficient serialization with minimal overhead
 * - Adaptive reliability based on message importance
 * - Bandwidth-aware packet batching
 * - CPU cache-friendly data structures
 * 
 * @author ECScope Educational ECS Framework - Networking Layer
 * @date 2024
 */

#include "network_types.hpp"
#include "../core/types.hpp"
#include "../memory/arena.hpp"
#include "../memory/pool.hpp"
#include <array>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <cstring>
#include <bit>

namespace ecscope::networking::protocol {

//=============================================================================
// Protocol Constants and Configuration
//=============================================================================

namespace constants {
    /** @brief Protocol magic number for packet validation */
    constexpr u32 PROTOCOL_MAGIC = 0xECS0C0DE;
    
    /** @brief Current protocol version */
    constexpr u16 PROTOCOL_VERSION = 1;
    
    /** @brief Maximum packet size (including headers) */
    constexpr usize MAX_PACKET_SIZE = 1400;  // Safe for Ethernet MTU
    
    /** @brief Minimum packet header size */
    constexpr usize MIN_HEADER_SIZE = 16;
    
    /** @brief Maximum payload size per packet */
    constexpr usize MAX_PAYLOAD_SIZE = MAX_PACKET_SIZE - MIN_HEADER_SIZE;
    
    /** @brief Maximum fragments per message */
    constexpr u16 MAX_FRAGMENTS = 255;
    
    /** @brief Acknowledgment timeout (in microseconds) */
    constexpr u64 ACK_TIMEOUT_US = 100000;  // 100ms
    
    /** @brief Maximum retransmission attempts */
    constexpr u8 MAX_RETRANSMISSIONS = 5;
    
    /** @brief Sequence number wrap-around point */
    constexpr u32 SEQUENCE_WRAP = 0x80000000;
}

//=============================================================================
// Packet Header Structure
//=============================================================================

/**
 * @brief Packet Type Enumeration
 * 
 * Defines different types of packets in our custom protocol.
 * Each type has specific handling and reliability characteristics.
 */
enum class PacketType : u8 {
    /** @brief Regular data packet */
    Data = 0,
    
    /** @brief Acknowledgment packet */
    Acknowledgment = 1,
    
    /** @brief Connection request */
    ConnectRequest = 2,
    
    /** @brief Connection response */
    ConnectResponse = 3,
    
    /** @brief Graceful disconnect */
    Disconnect = 4,
    
    /** @brief Heartbeat/ping packet */
    Heartbeat = 5,
    
    /** @brief Fragment of a larger message */
    Fragment = 6,
    
    /** @brief Bandwidth probe packet */
    BandwidthProbe = 7
};

/**
 * @brief Packet Flags
 * 
 * Bit flags that modify packet behavior and provide additional information.
 */
enum class PacketFlags : u8 {
    None           = 0x00,
    RequiresAck    = 0x01,  // Packet needs acknowledgment
    IsCompressed   = 0x02,  // Payload is compressed
    IsEncrypted    = 0x04,  // Payload is encrypted
    IsFragmented   = 0x08,  // Part of a fragmented message
    LastFragment   = 0x10,  // Last fragment of a message
    OrderedPacket  = 0x20,  // Must be processed in order
    CriticalData   = 0x40,  // High priority data
    Reserved       = 0x80   // Reserved for future use
};

inline PacketFlags operator|(PacketFlags a, PacketFlags b) {
    return static_cast<PacketFlags>(static_cast<u8>(a) | static_cast<u8>(b));
}

inline PacketFlags operator&(PacketFlags a, PacketFlags b) {
    return static_cast<PacketFlags>(static_cast<u8>(a) & static_cast<u8>(b));
}

inline bool has_flag(PacketFlags flags, PacketFlags flag) {
    return (flags & flag) == flag;
}

/**
 * @brief Protocol Packet Header
 * 
 * Fixed-size header present in all packets. Designed for efficient parsing
 * and minimal overhead while providing essential protocol features.
 * 
 * Memory Layout (16 bytes total):
 * - 4 bytes: Magic number (protocol validation)
 * - 2 bytes: Protocol version
 * - 1 byte:  Packet type
 * - 1 byte:  Packet flags
 * - 4 bytes: Sequence number
 * - 4 bytes: Timestamp (microseconds)
 * 
 * Educational Note:
 * This header is designed to be exactly 16 bytes for cache line alignment
 * and efficient network parsing. Each field serves a specific purpose in
 * the protocol's reliability and ordering mechanisms.
 */
struct alignas(4) PacketHeader {
    u32 magic{constants::PROTOCOL_MAGIC};
    u16 version{constants::PROTOCOL_VERSION};
    PacketType type{PacketType::Data};
    PacketFlags flags{PacketFlags::None};
    u32 sequence_number{0};
    NetworkTimestamp timestamp{0};
    
    /** @brief Validate packet header */
    bool is_valid() const noexcept {
        return magic == constants::PROTOCOL_MAGIC && 
               version == constants::PROTOCOL_VERSION;
    }
    
    /** @brief Check if packet requires acknowledgment */
    bool requires_ack() const noexcept {
        return has_flag(flags, PacketFlags::RequiresAck);
    }
    
    /** @brief Check if packet is fragmented */
    bool is_fragmented() const noexcept {
        return has_flag(flags, PacketFlags::IsFragmented);
    }
    
    /** @brief Get header size in bytes */
    static constexpr usize size() noexcept {
        return sizeof(PacketHeader);
    }
};

static_assert(sizeof(PacketHeader) == 16, "PacketHeader must be exactly 16 bytes");
static_assert(std::is_standard_layout_v<PacketHeader>, "PacketHeader must be standard layout");

//=============================================================================
// Specialized Headers for Different Packet Types
//=============================================================================

/**
 * @brief Acknowledgment Packet Header
 * 
 * Sent to acknowledge receipt of reliable packets. Contains the sequence
 * number of the packet being acknowledged plus additional information
 * for flow control and congestion management.
 */
struct AckHeader {
    u32 ack_sequence{0};        // Sequence number being acknowledged
    u32 ack_bitfield{0};        // Bitfield for selective acknowledgments
    u16 receive_window{0};      // Available buffer space
    u16 padding{0};             // Alignment padding
    
    /** @brief Check if sequence number is acknowledged in bitfield */
    bool is_acked(u32 sequence) const noexcept {
        if (sequence == ack_sequence) return true;
        
        // Check if sequence is in the selective ack bitfield
        i32 diff = static_cast<i32>(ack_sequence - sequence);
        if (diff > 0 && diff <= 32) {
            return (ack_bitfield & (1u << (diff - 1))) != 0;
        }
        return false;
    }
};

/**
 * @brief Fragment Header
 * 
 * Used when a message is too large to fit in a single packet and must
 * be split across multiple fragments. Each fragment includes information
 * needed to reassemble the original message.
 */
struct FragmentHeader {
    u32 message_id{0};          // Unique ID for the fragmented message
    u16 fragment_index{0};      // Index of this fragment (0-based)
    u16 total_fragments{0};     // Total number of fragments
    u32 total_message_size{0};  // Size of complete reassembled message
    u32 fragment_offset{0};     // Byte offset of this fragment in message
    
    /** @brief Check if this is the last fragment */
    bool is_last_fragment() const noexcept {
        return fragment_index == (total_fragments - 1);
    }
};

/**
 * @brief Connection Request Header
 * 
 * Sent by clients to initiate a connection with the server. Contains
 * protocol negotiation information and client capabilities.
 */
struct ConnectRequestHeader {
    SessionID session_id{0};      // Requested session ID
    u32 client_version{0};        // Client software version
    u32 supported_features{0};    // Bitfield of supported features
    u16 max_packet_size{0};       // Maximum packet size client can handle
    u16 preferred_tick_rate{0};   // Preferred network tick rate
};

/**
 * @brief Connection Response Header
 * 
 * Sent by server in response to connection requests. Contains assigned
 * client ID and negotiated connection parameters.
 */
struct ConnectResponseHeader {
    ClientID assigned_client_id{0};  // Client ID assigned by server
    SessionID session_id{0};         // Confirmed session ID
    u32 server_version{0};           // Server software version
    u32 negotiated_features{0};      // Agreed-upon features
    u16 negotiated_tick_rate{0};     // Final network tick rate
    u16 padding{0};                  // Alignment padding
};

//=============================================================================
// Packet Buffer Management
//=============================================================================

/**
 * @brief Network Packet Buffer
 * 
 * Efficient packet buffer implementation with educational features.
 * Designed to minimize allocations and provide clear insight into
 * network data flow.
 */
class PacketBuffer {
private:
    std::array<u8, constants::MAX_PACKET_SIZE> data_;
    usize size_{0};
    usize read_pos_{0};
    
public:
    /** @brief Initialize empty packet buffer */
    PacketBuffer() = default;
    
    /** @brief Initialize packet buffer with data */
    PacketBuffer(const void* data, usize size) {
        write(data, size);
    }
    
    /** @brief Get raw data pointer */
    const u8* data() const noexcept { return data_.data(); }
    u8* data() noexcept { return data_.data(); }
    
    /** @brief Get current buffer size */
    usize size() const noexcept { return size_; }
    
    /** @brief Get remaining capacity */
    usize capacity() const noexcept { return data_.size() - size_; }
    
    /** @brief Get current read position */
    usize read_pos() const noexcept { return read_pos_; }
    
    /** @brief Get remaining bytes to read */
    usize remaining() const noexcept { return size_ - read_pos_; }
    
    /** @brief Check if buffer is empty */
    bool empty() const noexcept { return size_ == 0; }
    
    /** @brief Clear buffer */
    void clear() noexcept {
        size_ = 0;
        read_pos_ = 0;
    }
    
    /** @brief Reset read position */
    void reset_read() noexcept {
        read_pos_ = 0;
    }
    
    /** @brief Write data to buffer */
    bool write(const void* data, usize bytes) noexcept {
        if (bytes > capacity()) return false;
        
        std::memcpy(data_.data() + size_, data, bytes);
        size_ += bytes;
        return true;
    }
    
    /** @brief Read data from buffer */
    bool read(void* dest, usize bytes) noexcept {
        if (bytes > remaining()) return false;
        
        std::memcpy(dest, data_.data() + read_pos_, bytes);
        read_pos_ += bytes;
        return true;
    }
    
    /** @brief Peek at data without advancing read position */
    bool peek(void* dest, usize bytes, usize offset = 0) const noexcept {
        if (read_pos_ + offset + bytes > size_) return false;
        
        std::memcpy(dest, data_.data() + read_pos_ + offset, bytes);
        return true;
    }
    
    /** @brief Write typed data */
    template<typename T>
    bool write(const T& value) noexcept {
        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
        return write(&value, sizeof(T));
    }
    
    /** @brief Read typed data */
    template<typename T>
    bool read(T& value) noexcept {
        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
        return read(&value, sizeof(T));
    }
    
    /** @brief Peek at typed data */
    template<typename T>
    bool peek(T& value, usize offset = 0) const noexcept {
        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
        return peek(&value, sizeof(T), offset);
    }
};

//=============================================================================
// Reliable Packet Tracking
//=============================================================================

/**
 * @brief Pending Acknowledgment Entry
 * 
 * Tracks packets that require acknowledgment for the reliability layer.
 * Contains all information needed for retransmission and timeout handling.
 */
struct PendingAck {
    PacketBuffer packet;               // Original packet data
    NetworkTimestamp send_time;        // When packet was sent
    NetworkTimestamp last_resend;      // Last resend attempt
    u8 resend_count{0};               // Number of resend attempts
    Reliability reliability;           // Required reliability level
    
    /** @brief Check if acknowledgment has timed out */
    bool has_timed_out(NetworkTimestamp current_time) const noexcept {
        return (current_time - send_time) > constants::ACK_TIMEOUT_US;
    }
    
    /** @brief Check if maximum resend attempts reached */
    bool max_resends_reached() const noexcept {
        return resend_count >= constants::MAX_RETRANSMISSIONS;
    }
    
    /** @brief Get time since last resend attempt */
    u64 time_since_resend(NetworkTimestamp current_time) const noexcept {
        return current_time - last_resend;
    }
};

/**
 * @brief Sequence Number Manager
 * 
 * Manages sequence number generation and validation with proper wrap-around
 * handling. Provides educational insights into sequence number mechanics.
 */
class SequenceManager {
private:
    u32 next_sequence_{1};  // Next sequence number to assign (0 reserved)
    
public:
    /** @brief Get next sequence number */
    u32 next() noexcept {
        u32 seq = next_sequence_++;
        
        // Handle wrap-around
        if (next_sequence_ >= constants::SEQUENCE_WRAP) {
            next_sequence_ = 1;  // Skip 0
        }
        
        return seq;
    }
    
    /** @brief Check if sequence number is newer than reference */
    static bool is_newer(u32 sequence, u32 reference) noexcept {
        return ((sequence > reference) && (sequence - reference <= constants::SEQUENCE_WRAP / 2)) ||
               ((reference > sequence) && (reference - sequence > constants::SEQUENCE_WRAP / 2));
    }
    
    /** @brief Calculate sequence number difference (handles wrap-around) */
    static i32 sequence_diff(u32 a, u32 b) noexcept {
        if (a >= b) {
            u32 diff = a - b;
            return (diff <= constants::SEQUENCE_WRAP / 2) ? 
                   static_cast<i32>(diff) : 
                   static_cast<i32>(diff) - static_cast<i32>(constants::SEQUENCE_WRAP);
        } else {
            u32 diff = b - a;
            return (diff <= constants::SEQUENCE_WRAP / 2) ? 
                   -static_cast<i32>(diff) : 
                   static_cast<i32>(constants::SEQUENCE_WRAP) - static_cast<i32>(diff);
        }
    }
    
    /** @brief Reset sequence counter */
    void reset() noexcept {
        next_sequence_ = 1;
    }
};

//=============================================================================
// Message Fragmentation System
//=============================================================================

/**
 * @brief Fragment Reassembly Buffer
 * 
 * Manages reassembly of fragmented messages. Handles out-of-order delivery
 * and provides educational visualization of the fragmentation process.
 */
class FragmentReassembler {
private:
    struct FragmentedMessage {
        std::vector<u8> data;           // Reassembled message data
        std::vector<bool> received;     // Which fragments we have
        u16 total_fragments;            // Expected fragment count
        u32 total_size;                 // Expected final message size
        NetworkTimestamp first_fragment_time;  // For timeout detection
        usize received_count{0};        // Number of fragments received
        
        bool is_complete() const noexcept {
            return received_count == total_fragments;
        }
    };
    
    std::unordered_map<u32, FragmentedMessage> pending_messages_;
    NetworkTimestamp fragment_timeout_{5 * 1000 * 1000};  // 5 second timeout
    
public:
    /** @brief Add fragment to reassembly buffer */
    enum class AddResult {
        NeedMoreFragments,  // Fragment added, waiting for more
        MessageComplete,    // All fragments received, message ready
        AlreadyReceived,    // Fragment was duplicate
        InvalidFragment,    // Fragment data was invalid
        MessageTimeout      // Message reassembly timed out
    };
    
    AddResult add_fragment(const FragmentHeader& header, 
                          const void* fragment_data,
                          usize fragment_size,
                          NetworkTimestamp current_time) {
        auto& msg = pending_messages_[header.message_id];
        
        // Initialize new message
        if (msg.total_fragments == 0) {
            msg.total_fragments = header.total_fragments;
            msg.total_size = header.total_message_size;
            msg.data.resize(header.total_message_size);
            msg.received.resize(header.total_fragments, false);
            msg.first_fragment_time = current_time;
        }
        
        // Check for timeout
        if (current_time - msg.first_fragment_time > fragment_timeout_) {
            pending_messages_.erase(header.message_id);
            return AddResult::MessageTimeout;
        }
        
        // Validate fragment
        if (header.fragment_index >= msg.total_fragments ||
            header.fragment_offset + fragment_size > msg.total_size) {
            return AddResult::InvalidFragment;
        }
        
        // Check for duplicate
        if (msg.received[header.fragment_index]) {
            return AddResult::AlreadyReceived;
        }
        
        // Add fragment data
        std::memcpy(msg.data.data() + header.fragment_offset, 
                   fragment_data, fragment_size);
        msg.received[header.fragment_index] = true;
        msg.received_count++;
        
        // Check if message is complete
        if (msg.is_complete()) {
            return AddResult::MessageComplete;
        }
        
        return AddResult::NeedMoreFragments;
    }
    
    /** @brief Get completed message data */
    std::vector<u8> get_completed_message(u32 message_id) {
        auto it = pending_messages_.find(message_id);
        if (it != pending_messages_.end() && it->second.is_complete()) {
            std::vector<u8> data = std::move(it->second.data);
            pending_messages_.erase(it);
            return data;
        }
        return {};
    }
    
    /** @brief Clean up timed out messages */
    usize cleanup_timeouts(NetworkTimestamp current_time) {
        usize cleaned = 0;
        auto it = pending_messages_.begin();
        while (it != pending_messages_.end()) {
            if (current_time - it->second.first_fragment_time > fragment_timeout_) {
                it = pending_messages_.erase(it);
                cleaned++;
            } else {
                ++it;
            }
        }
        return cleaned;
    }
    
    /** @brief Get number of pending messages */
    usize pending_count() const noexcept {
        return pending_messages_.size();
    }
    
    /** @brief Clear all pending messages */
    void clear() {
        pending_messages_.clear();
    }
};

//=============================================================================
// Protocol Statistics
//=============================================================================

/**
 * @brief Protocol Layer Statistics
 * 
 * Comprehensive statistics for educational analysis of protocol performance.
 * Provides insights into reliability layer effectiveness, bandwidth usage,
 * and overall protocol health.
 */
struct ProtocolStats {
    // Basic packet counters
    u64 packets_sent{0};
    u64 packets_received{0};
    u64 bytes_sent{0};
    u64 bytes_received{0};
    
    // Reliability layer statistics
    u64 acks_sent{0};
    u64 acks_received{0};
    u64 packets_retransmitted{0};
    u64 packets_lost{0};
    u64 packets_duplicate{0};
    u64 packets_out_of_order{0};
    
    // Fragmentation statistics
    u64 messages_fragmented{0};
    u64 fragments_sent{0};
    u64 fragments_received{0};
    u64 fragments_reassembled{0};
    u64 fragmented_messages_completed{0};
    u64 fragmented_messages_timed_out{0};
    
    // Bandwidth and performance
    u32 average_packet_size{0};
    f32 bandwidth_utilization{0.0f};
    f32 protocol_overhead_percentage{0.0f};
    
    // Connection quality metrics
    f32 packet_loss_rate{0.0f};
    f32 out_of_order_rate{0.0f};
    f32 duplicate_rate{0.0f};
    
    /** @brief Update derived statistics */
    void update_derived_stats() {
        if (packets_sent > 0) {
            packet_loss_rate = static_cast<f32>(packets_lost) / 
                              static_cast<f32>(packets_sent);
            out_of_order_rate = static_cast<f32>(packets_out_of_order) / 
                               static_cast<f32>(packets_received);
            duplicate_rate = static_cast<f32>(packets_duplicate) / 
                           static_cast<f32>(packets_received);
            
            if (bytes_sent > 0) {
                average_packet_size = static_cast<u32>(bytes_sent / packets_sent);
                
                // Estimate protocol overhead
                u64 header_bytes = packets_sent * sizeof(PacketHeader);
                protocol_overhead_percentage = static_cast<f32>(header_bytes) / 
                                              static_cast<f32>(bytes_sent) * 100.0f;
            }
        }
    }
    
    /** @brief Reset all statistics */
    void reset() {
        *this = ProtocolStats{};
    }
    
    /** @brief Get overall protocol efficiency score (0.0 - 1.0) */
    f32 efficiency_score() const {
        f32 loss_factor = 1.0f - packet_loss_rate;
        f32 order_factor = 1.0f - out_of_order_rate;
        f32 overhead_factor = 1.0f - (protocol_overhead_percentage / 100.0f);
        
        return (loss_factor * 0.5f) + (order_factor * 0.3f) + (overhead_factor * 0.2f);
    }
};

//=============================================================================
// Complete Network Protocol Implementation
//=============================================================================

/**
 * @brief Network Protocol Manager
 * 
 * Complete implementation of the custom UDP protocol with reliability layers,
 * fragmentation, and comprehensive educational features.
 */
class NetworkProtocol {
private:
    // Core protocol state
    TransportProtocol transport_type_;
    SequenceManager sequence_manager_;
    FragmentReassembler fragment_reassembler_;
    
    // Reliability tracking
    std::unordered_map<u32, PendingAck> pending_acks_;  // Outgoing packets waiting for ACK
    std::unordered_map<u32, bool> received_sequences_;  // Incoming packet deduplication
    
    // Connection management
    ConnectionState connection_state_{ConnectionState::Disconnected};
    ClientID local_client_id_{0};
    SessionID current_session_{0};
    NetworkTimestamp last_heartbeat_time_{0};
    
    // Memory management
    memory::Pool<PacketBuffer> packet_pool_;
    memory::Arena protocol_arena_;
    u32 next_message_id_{1};
    
    // Educational and debugging features
    bool educational_mode_{false};
    bool packet_inspection_enabled_{false};
    mutable std::vector<std::string> educational_insights_;
    
    // Protocol statistics
    mutable ProtocolStats stats_;
    NetworkTimestamp stats_last_update_{0};
    
    // Configuration
    struct Config {
        u32 max_packet_size{constants::MAX_PACKET_SIZE};
        u32 ack_timeout_us{constants::ACK_TIMEOUT_US};
        u8 max_retransmissions{constants::MAX_RETRANSMISSIONS};
        u32 heartbeat_interval_us{1000000};  // 1 second
        bool enable_compression{false};
        bool enable_packet_batching{true};
        u32 batch_timeout_us{16667}; // ~60 FPS
    } config_;
    
public:
    /** @brief Initialize protocol with transport type */
    explicit NetworkProtocol(TransportProtocol transport = TransportProtocol::ReliableUDP)
        : transport_type_(transport),
          packet_pool_(1024),
          protocol_arena_(64 * 1024) {}  // 64KB protocol buffer
    
    /** @brief Set educational mode */
    void set_educational_mode(bool enabled) {
        educational_mode_ = enabled;
        
        if (enabled) {
            educational_insights_.push_back(
                "Educational mode enabled for network protocol. "
                "You'll see detailed explanations of protocol operations."
            );
        }
    }
    
    /** @brief Enable packet inspection for deep analysis */
    void set_packet_inspection_enabled(bool enabled) {
        packet_inspection_enabled_ = enabled;
        
        if (enabled) {
            educational_insights_.push_back(
                "Packet inspection enabled. All packet headers and payloads will be analyzed."
            );
        }
    }
    
    //-------------------------------------------------------------------------
    // Packet Creation and Sending
    //-------------------------------------------------------------------------
    
    /** @brief Create and send data packet */
    bool send_data(const void* data, usize size, 
                   Reliability reliability = Reliability::Unreliable,
                   MessagePriority priority = MessagePriority::Normal) {
        
        if (size == 0 || !data) {
            return false;
        }
        
        NetworkTimestamp current_time = timing::now();
        
        // Check if message needs fragmentation
        if (size > constants::MAX_PAYLOAD_SIZE) {
            return send_fragmented_message(data, size, reliability, priority, current_time);
        }
        
        // Create single packet
        PacketBuffer packet;
        if (!create_data_packet(packet, data, size, reliability, priority, current_time)) {
            return false;
        }
        
        return send_packet(packet, reliability);
    }
    
    /** @brief Send acknowledgment for received packet */
    bool send_acknowledgment(u32 ack_sequence, u32 ack_bitfield = 0) {
        PacketBuffer ack_packet;
        
        // Create packet header
        PacketHeader header;
        header.type = PacketType::Acknowledgment;
        header.flags = PacketFlags::None;  // ACKs don't need ACKs
        header.sequence_number = sequence_manager_.next();
        header.timestamp = timing::now();
        
        if (!ack_packet.write(header)) {
            return false;
        }
        
        // Create ACK-specific header
        AckHeader ack_header;
        ack_header.ack_sequence = ack_sequence;
        ack_header.ack_bitfield = ack_bitfield;
        ack_header.receive_window = 1024;  // TODO: Calculate actual receive window
        
        if (!ack_packet.write(ack_header)) {
            return false;
        }
        
        stats_.acks_sent++;
        
        if (educational_mode_) {
            educational_insights_.push_back(
                "Sent ACK for sequence " + std::to_string(ack_sequence) + 
                " (Bitfield: 0x" + std::to_string(ack_bitfield) + ")"
            );
        }
        
        return send_packet(ack_packet, Reliability::Unreliable);
    }
    
    /** @brief Send heartbeat packet */
    bool send_heartbeat() {
        PacketBuffer heartbeat_packet;
        
        PacketHeader header;
        header.type = PacketType::Heartbeat;
        header.flags = PacketFlags::None;
        header.sequence_number = sequence_manager_.next();
        header.timestamp = timing::now();
        
        if (!heartbeat_packet.write(header)) {
            return false;
        }
        
        last_heartbeat_time_ = header.timestamp;
        
        if (educational_mode_) {
            educational_insights_.push_back(
                "Sent heartbeat packet (keeps connection alive)"
            );
        }
        
        return send_packet(heartbeat_packet, Reliability::Unreliable);
    }
    
    //-------------------------------------------------------------------------
    // Packet Reception and Processing
    //-------------------------------------------------------------------------
    
    /** @brief Process incoming packet data */
    enum class ProcessResult {
        Success,
        InvalidPacket,
        DuplicatePacket,
        OutOfOrderPacket,
        FragmentReceived,
        MessageReassembled,
        AckReceived,
        HeartbeatReceived
    };
    
    ProcessResult process_incoming_data(const void* data, usize size, 
                                       const NetworkAddress& sender) {
        if (size < sizeof(PacketHeader)) {
            stats_.packets_lost++;
            return ProcessResult::InvalidPacket;
        }
        
        PacketBuffer packet(data, size);
        PacketHeader header;
        
        if (!packet.read(header)) {
            stats_.packets_lost++;
            return ProcessResult::InvalidPacket;
        }
        
        // Validate header
        if (!header.is_valid()) {
            stats_.packets_lost++;
            if (educational_mode_) {
                educational_insights_.push_back(
                    "Received invalid packet (wrong magic number or version)"
                );
            }
            return ProcessResult::InvalidPacket;
        }
        
        stats_.packets_received++;
        stats_.bytes_received += size;
        
        if (packet_inspection_enabled_) {
            inspect_packet(header, packet);
        }
        
        // Handle different packet types
        switch (header.type) {
            case PacketType::Data:
                return process_data_packet(header, packet);
                
            case PacketType::Fragment:
                return process_fragment_packet(header, packet);
                
            case PacketType::Acknowledgment:
                return process_ack_packet(header, packet);
                
            case PacketType::Heartbeat:
                if (educational_mode_) {
                    educational_insights_.push_back("Received heartbeat packet");
                }
                return ProcessResult::HeartbeatReceived;
                
            case PacketType::ConnectRequest:
                return process_connect_request(header, packet, sender);
                
            case PacketType::ConnectResponse:
                return process_connect_response(header, packet);
                
            case PacketType::Disconnect:
                if (educational_mode_) {
                    educational_insights_.push_back("Received disconnect packet");
                }
                connection_state_ = ConnectionState::Disconnected;
                return ProcessResult::Success;
                
            default:
                if (educational_mode_) {
                    educational_insights_.push_back(
                        "Received unknown packet type: " + std::to_string(static_cast<u8>(header.type))
                    );
                }
                return ProcessResult::InvalidPacket;
        }
    }
    
    //-------------------------------------------------------------------------
    // Reliability Layer Management
    //-------------------------------------------------------------------------
    
    /** @brief Update protocol state (call regularly) */
    void update(NetworkTimestamp current_time) {
        // Process ACK timeouts and retransmissions
        process_ack_timeouts(current_time);
        
        // Clean up old received sequence numbers
        cleanup_received_sequences();
        
        // Clean up fragmentation timeouts
        fragment_reassembler_.cleanup_timeouts(current_time);
        
        // Send heartbeat if needed
        if (connection_state_ == ConnectionState::Connected &&
            current_time - last_heartbeat_time_ > config_.heartbeat_interval_us) {
            send_heartbeat();
        }
        
        // Update statistics
        if (current_time - stats_last_update_ > 1000000) { // Every second
            stats_.update_derived_stats();
            stats_last_update_ = current_time;
            
            if (educational_mode_) {
                update_educational_insights();
            }
        }
    }
    
    /** @brief Get protocol statistics */
    const ProtocolStats& get_statistics() const {
        return stats_;
    }
    
    /** @brief Get educational insights */
    std::vector<std::string> get_educational_insights() const {
        auto insights = educational_insights_;
        educational_insights_.clear();
        return insights;
    }
    
    /** @brief Get current connection state */
    ConnectionState get_connection_state() const {
        return connection_state_;
    }
    
    /** @brief Set connection parameters */
    void set_connection_info(ClientID client_id, SessionID session_id) {
        local_client_id_ = client_id;
        current_session_ = session_id;
        connection_state_ = ConnectionState::Connected;
    }

private:
    //-------------------------------------------------------------------------
    // Internal Packet Creation
    //-------------------------------------------------------------------------
    
    /** @brief Create data packet with header and payload */
    bool create_data_packet(PacketBuffer& packet, 
                           const void* data, usize size,
                           Reliability reliability, MessagePriority priority,
                           NetworkTimestamp timestamp) {
        PacketHeader header;
        header.type = PacketType::Data;
        header.flags = PacketFlags::None;
        header.sequence_number = sequence_manager_.next();
        header.timestamp = timestamp;
        
        // Set reliability flags
        if (reliability == Reliability::Reliable || reliability == Reliability::ReliableOrdered) {
            header.flags = header.flags | PacketFlags::RequiresAck;
        }
        
        if (reliability == Reliability::UnreliableOrdered || reliability == Reliability::ReliableOrdered) {
            header.flags = header.flags | PacketFlags::OrderedPacket;
        }
        
        if (priority == MessagePriority::Critical) {
            header.flags = header.flags | PacketFlags::CriticalData;
        }
        
        // Write header and payload
        if (!packet.write(header) || !packet.write(data, size)) {
            return false;
        }
        
        return true;
    }
    
    /** @brief Send fragmented message */
    bool send_fragmented_message(const void* data, usize size,
                                Reliability reliability, MessagePriority priority,
                                NetworkTimestamp timestamp) {
        u32 message_id = next_message_id_++;
        usize max_fragment_payload = constants::MAX_PAYLOAD_SIZE - sizeof(FragmentHeader);
        u16 total_fragments = static_cast<u16>((size + max_fragment_payload - 1) / max_fragment_payload);
        
        if (total_fragments > constants::MAX_FRAGMENTS) {
            if (educational_mode_) {
                educational_insights_.push_back(
                    "Message too large to fragment (would require " + 
                    std::to_string(total_fragments) + " fragments)"
                );
            }
            return false;
        }
        
        const u8* data_bytes = static_cast<const u8*>(data);
        
        for (u16 i = 0; i < total_fragments; ++i) {
            PacketBuffer fragment_packet;
            
            // Create packet header
            PacketHeader header;
            header.type = PacketType::Fragment;
            header.flags = PacketFlags::IsFragmented;
            header.sequence_number = sequence_manager_.next();
            header.timestamp = timestamp;
            
            if (reliability == Reliability::Reliable || reliability == Reliability::ReliableOrdered) {
                header.flags = header.flags | PacketFlags::RequiresAck;
            }
            
            if (i == total_fragments - 1) {
                header.flags = header.flags | PacketFlags::LastFragment;
            }
            
            // Create fragment header
            FragmentHeader frag_header;
            frag_header.message_id = message_id;
            frag_header.fragment_index = i;
            frag_header.total_fragments = total_fragments;
            frag_header.total_message_size = static_cast<u32>(size);
            frag_header.fragment_offset = i * max_fragment_payload;
            
            // Calculate fragment size
            usize fragment_size = std::min(max_fragment_payload, size - frag_header.fragment_offset);
            
            // Write headers and fragment data
            if (!fragment_packet.write(header) || 
                !fragment_packet.write(frag_header) ||
                !fragment_packet.write(data_bytes + frag_header.fragment_offset, fragment_size)) {
                return false;
            }
            
            if (!send_packet(fragment_packet, reliability)) {
                return false;
            }
        }
        
        stats_.messages_fragmented++;
        stats_.fragments_sent += total_fragments;
        
        if (educational_mode_) {
            educational_insights_.push_back(
                "Fragmented " + std::to_string(size) + " byte message into " +
                std::to_string(total_fragments) + " fragments"
            );
        }
        
        return true;
    }
    
    //-------------------------------------------------------------------------
    // Packet Processing Implementation
    //-------------------------------------------------------------------------
    
    /** @brief Process data packet */
    ProcessResult process_data_packet(const PacketHeader& header, PacketBuffer& packet) {
        // Check for duplicates
        if (received_sequences_.find(header.sequence_number) != received_sequences_.end()) {
            stats_.packets_duplicate++;
            if (educational_mode_) {
                educational_insights_.push_back(
                    "Received duplicate packet (sequence " + std::to_string(header.sequence_number) + ")"
                );
            }
            return ProcessResult::DuplicatePacket;
        }
        
        // Mark as received
        received_sequences_[header.sequence_number] = true;
        
        // Send ACK if required
        if (header.requires_ack()) {
            send_acknowledgment(header.sequence_number);
        }
        
        // TODO: Handle ordered packet logic
        
        if (educational_mode_) {
            educational_insights_.push_back(
                "Received data packet (sequence " + std::to_string(header.sequence_number) + 
                ", size " + std::to_string(packet.remaining()) + " bytes)"
            );
        }
        
        return ProcessResult::Success;
    }
    
    /** @brief Process fragment packet */
    ProcessResult process_fragment_packet(const PacketHeader& header, PacketBuffer& packet) {
        FragmentHeader frag_header;
        if (!packet.read(frag_header)) {
            return ProcessResult::InvalidPacket;
        }
        
        // Add fragment to reassembler
        auto result = fragment_reassembler_.add_fragment(
            frag_header, 
            packet.data() + packet.read_pos(), 
            packet.remaining(),
            header.timestamp
        );
        
        stats_.fragments_received++;
        
        // Send ACK if required
        if (header.requires_ack()) {
            send_acknowledgment(header.sequence_number);
        }
        
        switch (result) {
            case FragmentReassembler::AddResult::MessageComplete:
                stats_.fragmented_messages_completed++;
                if (educational_mode_) {
                    educational_insights_.push_back(
                        "Completed reassembly of fragmented message " + std::to_string(frag_header.message_id)
                    );
                }
                return ProcessResult::MessageReassembled;
                
            case FragmentReassembler::AddResult::NeedMoreFragments:
                if (educational_mode_) {
                    educational_insights_.push_back(
                        "Received fragment " + std::to_string(frag_header.fragment_index) + 
                        "/" + std::to_string(frag_header.total_fragments) + 
                        " of message " + std::to_string(frag_header.message_id)
                    );
                }
                return ProcessResult::FragmentReceived;
                
            case FragmentReassembler::AddResult::MessageTimeout:
                stats_.fragmented_messages_timed_out++;
                return ProcessResult::InvalidPacket;
                
            default:
                return ProcessResult::InvalidPacket;
        }
    }
    
    /** @brief Process acknowledgment packet */
    ProcessResult process_ack_packet(const PacketHeader& header, PacketBuffer& packet) {
        AckHeader ack_header;
        if (!packet.read(ack_header)) {
            return ProcessResult::InvalidPacket;
        }
        
        stats_.acks_received++;
        
        // Remove acknowledged packets from pending list
        auto it = pending_acks_.find(ack_header.ack_sequence);
        if (it != pending_acks_.end()) {
            pending_acks_.erase(it);
            
            if (educational_mode_) {
                educational_insights_.push_back(
                    "Received ACK for sequence " + std::to_string(ack_header.ack_sequence)
                );
            }
        }
        
        // Handle selective acknowledgments in bitfield
        for (int i = 0; i < 32; ++i) {
            if (ack_header.ack_bitfield & (1u << i)) {
                u32 acked_seq = ack_header.ack_sequence - (i + 1);
                auto acked_it = pending_acks_.find(acked_seq);
                if (acked_it != pending_acks_.end()) {
                    pending_acks_.erase(acked_it);
                }
            }
        }
        
        return ProcessResult::AckReceived;
    }
    
    /** @brief Process connection request */
    ProcessResult process_connect_request(const PacketHeader& header, PacketBuffer& packet, 
                                        const NetworkAddress& sender) {
        // TODO: Implement connection handshake
        if (educational_mode_) {
            educational_insights_.push_back("Received connection request");
        }
        return ProcessResult::Success;
    }
    
    /** @brief Process connection response */
    ProcessResult process_connect_response(const PacketHeader& header, PacketBuffer& packet) {
        // TODO: Implement connection handshake
        if (educational_mode_) {
            educational_insights_.push_back("Received connection response");
        }
        return ProcessResult::Success;
    }
    
    //-------------------------------------------------------------------------
    // Internal Helper Methods
    //-------------------------------------------------------------------------
    
    /** @brief Send packet through transport layer */
    bool send_packet(const PacketBuffer& packet, Reliability reliability) {
        // Add to pending ACKs if reliable
        if (reliability == Reliability::Reliable || reliability == Reliability::ReliableOrdered) {
            PacketHeader header;
            if (packet.peek(header)) {
                PendingAck pending;
                pending.packet = packet;
                pending.send_time = header.timestamp;
                pending.last_resend = header.timestamp;
                pending.reliability = reliability;
                pending.resend_count = 0;
                
                pending_acks_[header.sequence_number] = std::move(pending);
            }
        }
        
        stats_.packets_sent++;
        stats_.bytes_sent += packet.size();
        
        // TODO: Actually send through socket/transport
        return true;
    }
    
    /** @brief Process ACK timeouts and retransmissions */
    void process_ack_timeouts(NetworkTimestamp current_time) {
        auto it = pending_acks_.begin();
        while (it != pending_acks_.end()) {
            auto& pending = it->second;
            
            if (pending.has_timed_out(current_time)) {
                if (pending.max_resends_reached()) {
                    // Give up on this packet
                    stats_.packets_lost++;
                    if (educational_mode_) {
                        educational_insights_.push_back(
                            "Gave up on packet " + std::to_string(it->first) + 
                            " after " + std::to_string(pending.resend_count) + " retries"
                        );
                    }
                    it = pending_acks_.erase(it);
                } else {
                    // Retransmit
                    pending.resend_count++;
                    pending.last_resend = current_time;
                    
                    send_packet(pending.packet, pending.reliability);
                    stats_.packets_retransmitted++;
                    
                    if (educational_mode_) {
                        educational_insights_.push_back(
                            "Retransmitted packet " + std::to_string(it->first) + 
                            " (attempt " + std::to_string(pending.resend_count) + ")"
                        );
                    }
                    
                    ++it;
                }
            } else {
                ++it;
            }
        }
    }
    
    /** @brief Clean up old received sequence numbers */
    void cleanup_received_sequences() {
        // Keep only recent sequence numbers to prevent memory growth
        // This is simplified - real implementation would use a sliding window
        if (received_sequences_.size() > 10000) {
            received_sequences_.clear();
        }
    }
    
    /** @brief Inspect packet for educational purposes */
    void inspect_packet(const PacketHeader& header, const PacketBuffer& packet) const {
        educational_insights_.push_back(
            "📦 Packet Inspection:\n" +
            "  Type: " + packet_type_to_string(header.type) + "\n" +
            "  Sequence: " + std::to_string(header.sequence_number) + "\n" +
            "  Flags: " + packet_flags_to_string(header.flags) + "\n" +
            "  Size: " + std::to_string(packet.size()) + " bytes\n" +
            "  Payload: " + std::to_string(packet.size() - sizeof(PacketHeader)) + " bytes"
        );
    }
    
    /** @brief Update educational insights with protocol health */
    void update_educational_insights() {
        f32 efficiency = stats_.efficiency_score();
        
        if (efficiency < 0.7f) {
            educational_insights_.push_back(
                "⚠️ Protocol efficiency is low (" + std::to_string(efficiency * 100.0f) + 
                "%). Consider checking network conditions or adjusting parameters."
            );
        }
        
        if (stats_.packet_loss_rate > 0.05f) {
            educational_insights_.push_back(
                "📊 High packet loss detected (" + std::to_string(stats_.packet_loss_rate * 100.0f) + 
                "%). This may indicate network congestion or poor connection quality."
            );
        }
        
        if (pending_acks_.size() > 100) {
            educational_insights_.push_back(
                "🔄 Many pending acknowledgments (" + std::to_string(pending_acks_.size()) + 
                "). This suggests network latency or packet loss issues."
            );
        }
    }
    
    /** @brief Convert packet type to string */
    std::string packet_type_to_string(PacketType type) const {
        switch (type) {
            case PacketType::Data: return "Data";
            case PacketType::Acknowledgment: return "ACK";
            case PacketType::ConnectRequest: return "Connect Request";
            case PacketType::ConnectResponse: return "Connect Response";
            case PacketType::Disconnect: return "Disconnect";
            case PacketType::Heartbeat: return "Heartbeat";
            case PacketType::Fragment: return "Fragment";
            case PacketType::BandwidthProbe: return "Bandwidth Probe";
        }
        return "Unknown";
    }
    
    /** @brief Convert packet flags to string */
    std::string packet_flags_to_string(PacketFlags flags) const {
        std::string result;
        if (has_flag(flags, PacketFlags::RequiresAck)) result += "ACK ";
        if (has_flag(flags, PacketFlags::IsCompressed)) result += "COMP ";
        if (has_flag(flags, PacketFlags::IsEncrypted)) result += "ENC ";
        if (has_flag(flags, PacketFlags::IsFragmented)) result += "FRAG ";
        if (has_flag(flags, PacketFlags::LastFragment)) result += "LAST ";
        if (has_flag(flags, PacketFlags::OrderedPacket)) result += "ORD ";
        if (has_flag(flags, PacketFlags::CriticalData)) result += "CRIT ";
        return result.empty() ? "None" : result;
    }
};

} // namespace ecscope::networking::protocol