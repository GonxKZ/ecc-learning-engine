#pragma once

#include "network_types.hpp"
#include <vector>
#include <string>
#include <type_traits>
#include <memory>
#include <unordered_map>
#include <functional>

namespace ecscope::networking {

/**
 * @brief Network Message Header
 * 
 * Fixed-size header for all network messages containing metadata.
 */
struct MessageHeader {
    static constexpr uint32_t MAGIC_NUMBER = 0xEC50C0DE; // ECScope CODE
    static constexpr uint16_t VERSION = 1;
    
    uint32_t magic = MAGIC_NUMBER;
    uint16_t version = VERSION;
    uint16_t message_type = 0;
    uint32_t message_id = 0;
    uint32_t payload_size = 0;
    uint32_t checksum = 0;
    NetworkTimestamp timestamp = 0;
    ClientID sender_id = 0;
    SessionID session_id = 0;
    MessagePriority priority = MessagePriority::Normal;
    Reliability reliability = Reliability::Reliable;
    uint8_t flags = 0;
    uint8_t reserved = 0;
    
    // Header flags
    enum Flags : uint8_t {
        COMPRESSED = 0x01,
        ENCRYPTED = 0x02,
        FRAGMENTED = 0x04,
        ACK_REQUIRED = 0x08,
        IS_ACK = 0x10
    };
    
    bool has_flag(Flags flag) const { return (flags & flag) != 0; }
    void set_flag(Flags flag) { flags |= flag; }
    void clear_flag(Flags flag) { flags &= ~flag; }
    
    // Calculate CRC32 checksum for payload
    static uint32_t calculate_checksum(const void* data, size_t size);
    
    // Validate header integrity
    bool is_valid() const;
};

/**
 * @brief Base Network Message
 * 
 * Base class for all network messages with serialization support.
 */
class NetworkMessage {
public:
    NetworkMessage() = default;
    explicit NetworkMessage(uint16_t message_type);
    virtual ~NetworkMessage() = default;

    // Message metadata
    uint16_t get_message_type() const { return header_.message_type; }
    uint32_t get_message_id() const { return header_.message_id; }
    void set_message_id(uint32_t id) { header_.message_id = id; }
    
    NetworkTimestamp get_timestamp() const { return header_.timestamp; }
    void set_timestamp(NetworkTimestamp timestamp) { header_.timestamp = timestamp; }
    
    ClientID get_sender_id() const { return header_.sender_id; }
    void set_sender_id(ClientID sender_id) { header_.sender_id = sender_id; }
    
    SessionID get_session_id() const { return header_.session_id; }
    void set_session_id(SessionID session_id) { header_.session_id = session_id; }
    
    MessagePriority get_priority() const { return header_.priority; }
    void set_priority(MessagePriority priority) { header_.priority = priority; }
    
    Reliability get_reliability() const { return header_.reliability; }
    void set_reliability(Reliability reliability) { header_.reliability = reliability; }

    // Serialization
    virtual std::vector<uint8_t> serialize() const;
    virtual bool deserialize(const std::vector<uint8_t>& data);
    
    // Size estimation
    virtual size_t get_serialized_size() const;
    
    // Message validation
    virtual bool is_valid() const { return true; }

protected:
    virtual void serialize_payload(std::vector<uint8_t>& buffer) const = 0;
    virtual bool deserialize_payload(const uint8_t* data, size_t size) = 0;

    MessageHeader header_;
};

/**
 * @brief Binary Data Message
 * 
 * Generic message for sending arbitrary binary data.
 */
class BinaryMessage : public NetworkMessage {
public:
    static constexpr uint16_t MESSAGE_TYPE = 1;
    
    BinaryMessage();
    explicit BinaryMessage(std::vector<uint8_t> data);
    
    const std::vector<uint8_t>& get_data() const { return data_; }
    void set_data(std::vector<uint8_t> data) { data_ = std::move(data); }
    
    size_t get_serialized_size() const override;

protected:
    void serialize_payload(std::vector<uint8_t>& buffer) const override;
    bool deserialize_payload(const uint8_t* data, size_t size) override;

private:
    std::vector<uint8_t> data_;
};

/**
 * @brief Text Message
 * 
 * Message for sending text/string data (chat, commands, etc.).
 */
class TextMessage : public NetworkMessage {
public:
    static constexpr uint16_t MESSAGE_TYPE = 2;
    
    TextMessage();
    explicit TextMessage(std::string text);
    
    const std::string& get_text() const { return text_; }
    void set_text(std::string text) { text_ = std::move(text); }
    
    size_t get_serialized_size() const override;

protected:
    void serialize_payload(std::vector<uint8_t>& buffer) const override;
    bool deserialize_payload(const uint8_t* data, size_t size) override;

private:
    std::string text_;
};

/**
 * @brief Connection Handshake Message
 * 
 * Initial message sent when establishing a connection.
 */
class HandshakeMessage : public NetworkMessage {
public:
    static constexpr uint16_t MESSAGE_TYPE = 10;
    
    HandshakeMessage();
    HandshakeMessage(const std::string& client_version, const std::string& client_name);
    
    const std::string& get_client_version() const { return client_version_; }
    const std::string& get_client_name() const { return client_name_; }
    SessionID get_requested_session_id() const { return requested_session_id_; }
    
    void set_client_version(std::string version) { client_version_ = std::move(version); }
    void set_client_name(std::string name) { client_name_ = std::move(name); }
    void set_requested_session_id(SessionID session_id) { requested_session_id_ = session_id; }
    
    size_t get_serialized_size() const override;
    bool is_valid() const override;

protected:
    void serialize_payload(std::vector<uint8_t>& buffer) const override;
    bool deserialize_payload(const uint8_t* data, size_t size) override;

private:
    std::string client_version_;
    std::string client_name_;
    SessionID requested_session_id_ = 0;
};

/**
 * @brief Connection Acknowledgment Message
 * 
 * Response to handshake message from server.
 */
class HandshakeAckMessage : public NetworkMessage {
public:
    static constexpr uint16_t MESSAGE_TYPE = 11;
    
    HandshakeAckMessage();
    HandshakeAckMessage(bool accepted, ClientID assigned_client_id, SessionID session_id);
    
    bool is_accepted() const { return accepted_; }
    ClientID get_assigned_client_id() const { return assigned_client_id_; }
    const std::string& get_rejection_reason() const { return rejection_reason_; }
    
    void set_accepted(bool accepted) { accepted_ = accepted; }
    void set_assigned_client_id(ClientID client_id) { assigned_client_id_ = client_id; }
    void set_rejection_reason(std::string reason) { rejection_reason_ = std::move(reason); }
    
    size_t get_serialized_size() const override;

protected:
    void serialize_payload(std::vector<uint8_t>& buffer) const override;
    bool deserialize_payload(const uint8_t* data, size_t size) override;

private:
    bool accepted_ = false;
    ClientID assigned_client_id_ = 0;
    std::string rejection_reason_;
};

/**
 * @brief Heartbeat/Ping Message
 * 
 * Keep-alive message for maintaining connections.
 */
class HeartbeatMessage : public NetworkMessage {
public:
    static constexpr uint16_t MESSAGE_TYPE = 20;
    
    HeartbeatMessage();
    explicit HeartbeatMessage(uint64_t ping_id);
    
    uint64_t get_ping_id() const { return ping_id_; }
    void set_ping_id(uint64_t ping_id) { ping_id_ = ping_id; }
    
    size_t get_serialized_size() const override;

protected:
    void serialize_payload(std::vector<uint8_t>& buffer) const override;
    bool deserialize_payload(const uint8_t* data, size_t size) override;

private:
    uint64_t ping_id_ = 0;
};

/**
 * @brief Heartbeat Acknowledgment Message
 * 
 * Response to heartbeat message for RTT calculation.
 */
class HeartbeatAckMessage : public NetworkMessage {
public:
    static constexpr uint16_t MESSAGE_TYPE = 21;
    
    HeartbeatAckMessage();
    explicit HeartbeatAckMessage(uint64_t ping_id);
    
    uint64_t get_ping_id() const { return ping_id_; }
    void set_ping_id(uint64_t ping_id) { ping_id_ = ping_id; }
    
    size_t get_serialized_size() const override;

protected:
    void serialize_payload(std::vector<uint8_t>& buffer) const override;
    bool deserialize_payload(const uint8_t* data, size_t size) override;

private:
    uint64_t ping_id_ = 0;
};

/**
 * @brief Disconnect Message
 * 
 * Graceful disconnection notification.
 */
class DisconnectMessage : public NetworkMessage {
public:
    static constexpr uint16_t MESSAGE_TYPE = 30;
    
    enum Reason : uint8_t {
        USER_INITIATED = 0,
        SERVER_SHUTDOWN = 1,
        KICKED = 2,
        TIMEOUT = 3,
        ERROR = 4
    };
    
    DisconnectMessage();
    DisconnectMessage(Reason reason, const std::string& message = "");
    
    Reason get_reason() const { return reason_; }
    const std::string& get_message() const { return message_; }
    
    void set_reason(Reason reason) { reason_ = reason; }
    void set_message(std::string message) { message_ = std::move(message); }
    
    size_t get_serialized_size() const override;

protected:
    void serialize_payload(std::vector<uint8_t>& buffer) const override;
    bool deserialize_payload(const uint8_t* data, size_t size) override;

private:
    Reason reason_ = USER_INITIATED;
    std::string message_;
};

/**
 * @brief Message Factory
 * 
 * Factory for creating message instances from message types.
 */
class MessageFactory {
public:
    using CreateFunction = std::function<std::unique_ptr<NetworkMessage>()>;
    
    static MessageFactory& instance();
    
    // Register message types
    template<typename T>
    void register_message_type() {
        static_assert(std::is_base_of_v<NetworkMessage, T>, "T must derive from NetworkMessage");
        register_message_type(T::MESSAGE_TYPE, []() -> std::unique_ptr<NetworkMessage> {
            return std::make_unique<T>();
        });
    }
    
    void register_message_type(uint16_t message_type, CreateFunction create_func);
    void unregister_message_type(uint16_t message_type);
    
    // Create messages
    std::unique_ptr<NetworkMessage> create_message(uint16_t message_type) const;
    std::unique_ptr<NetworkMessage> deserialize_message(const std::vector<uint8_t>& data) const;
    
    // Query message types
    bool is_registered(uint16_t message_type) const;
    std::vector<uint16_t> get_registered_types() const;

private:
    MessageFactory();
    void register_built_in_types();
    
    std::unordered_map<uint16_t, CreateFunction> message_creators_;
    mutable std::mutex factory_mutex_;
};

/**
 * @brief Message Serialization Utilities
 */
namespace serialization {
    
    // Primitive type serialization
    void write_uint8(std::vector<uint8_t>& buffer, uint8_t value);
    void write_uint16(std::vector<uint8_t>& buffer, uint16_t value);
    void write_uint32(std::vector<uint8_t>& buffer, uint32_t value);
    void write_uint64(std::vector<uint8_t>& buffer, uint64_t value);
    void write_string(std::vector<uint8_t>& buffer, const std::string& value);
    void write_bytes(std::vector<uint8_t>& buffer, const void* data, size_t size);
    
    // Primitive type deserialization
    bool read_uint8(const uint8_t*& data, size_t& remaining, uint8_t& value);
    bool read_uint16(const uint8_t*& data, size_t& remaining, uint16_t& value);
    bool read_uint32(const uint8_t*& data, size_t& remaining, uint32_t& value);
    bool read_uint64(const uint8_t*& data, size_t& remaining, uint64_t& value);
    bool read_string(const uint8_t*& data, size_t& remaining, std::string& value);
    bool read_bytes(const uint8_t*& data, size_t& remaining, void* buffer, size_t size);
    
    // Size calculation helpers
    size_t size_of_string(const std::string& value);
}

/**
 * @brief Message Queue
 * 
 * Thread-safe queue for network messages with priority support.
 */
class MessageQueue {
public:
    explicit MessageQueue(size_t max_size = 1000);
    ~MessageQueue() = default;
    
    // Queue operations
    bool enqueue(std::unique_ptr<NetworkMessage> message);
    std::unique_ptr<NetworkMessage> dequeue();
    std::unique_ptr<NetworkMessage> try_dequeue();
    
    // Queue state
    size_t size() const;
    bool empty() const;
    bool full() const;
    void clear();
    
    // Priority queue support
    void set_priority_enabled(bool enabled);
    bool is_priority_enabled() const;

private:
    struct QueueEntry {
        std::unique_ptr<NetworkMessage> message;
        NetworkTimestamp enqueue_time;
        
        bool operator<(const QueueEntry& other) const;
    };
    
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::priority_queue<QueueEntry> priority_queue_;
    std::queue<QueueEntry> fifo_queue_;
    size_t max_size_;
    bool priority_enabled_ = true;
};

} // namespace ecscope::networking