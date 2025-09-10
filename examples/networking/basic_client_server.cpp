/**
 * @file basic_client_server.cpp
 * @brief Basic Client-Server Networking Example
 * 
 * This example demonstrates the fundamental client-server networking capabilities
 * of the ECScope networking system, including:
 * 
 * - Setting up a dedicated server
 * - Connecting clients to the server
 * - Basic message exchange
 * - Connection management and monitoring
 * - ECS entity replication between client and server
 * 
 * Usage:
 * - Run with --server to start a server
 * - Run with --client to connect as a client
 * - Run multiple clients to test multi-client scenarios
 */

#include "../../include/ecscope/networking/network_registry.hpp"
#include "../../include/ecscope/ecs/registry.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <random>

using namespace ecscope;
using namespace ecscope::networking;
using namespace ecscope::ecs;

// Example components for replication
struct Position {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    
    Position() = default;
    Position(float x, float y, float z = 0.0f) : x(x), y(y), z(z) {}
};

struct Velocity {
    float dx = 0.0f;
    float dy = 0.0f;
    float dz = 0.0f;
    
    Velocity() = default;
    Velocity(float dx, float dy, float dz = 0.0f) : dx(dx), dy(dy), dz(dz) {}
};

struct PlayerInfo {
    std::string name;
    int32_t health = 100;
    int32_t score = 0;
    
    PlayerInfo() = default;
    PlayerInfo(const std::string& player_name) : name(player_name) {}
};

// Custom game message
class PlayerActionMessage : public NetworkMessage {
public:
    static constexpr uint16_t MESSAGE_TYPE = 1000;
    
    enum ActionType : uint8_t {
        MOVE = 0,
        ATTACK = 1,
        CHAT = 2
    };
    
    PlayerActionMessage() : NetworkMessage(MESSAGE_TYPE) {}
    PlayerActionMessage(ActionType action, const std::string& data) 
        : NetworkMessage(MESSAGE_TYPE), action_type_(action), action_data_(data) {}
    
    ActionType get_action_type() const { return action_type_; }
    const std::string& get_action_data() const { return action_data_; }
    
    void set_action_type(ActionType type) { action_type_ = type; }
    void set_action_data(const std::string& data) { action_data_ = data; }
    
    size_t get_serialized_size() const override {
        return sizeof(MessageHeader) + sizeof(action_type_) + sizeof(uint32_t) + action_data_.size();
    }

protected:
    void serialize_payload(std::vector<uint8_t>& buffer) const override {
        serialization::write_uint8(buffer, static_cast<uint8_t>(action_type_));
        serialization::write_string(buffer, action_data_);
    }
    
    bool deserialize_payload(const uint8_t* data, size_t size) override {
        const uint8_t* ptr = data;
        size_t remaining = size;
        
        uint8_t action_type_raw;
        if (!serialization::read_uint8(ptr, remaining, action_type_raw)) return false;
        action_type_ = static_cast<ActionType>(action_type_raw);
        
        if (!serialization::read_string(ptr, remaining, action_data_)) return false;
        
        return true;
    }

private:
    ActionType action_type_ = MOVE;
    std::string action_data_;
};

class BasicNetworkingDemo {
public:
    BasicNetworkingDemo(bool is_server, const std::string& server_address = "127.0.0.1", uint16_t port = 7777)
        : is_server_(is_server), server_address_(server_address), port_(port) {
        
        // Create ECS registry
        ecs_registry_ = std::make_shared<Registry>();
        
        // Create network registry configuration
        NetworkRegistryConfig config;
        config.is_server = is_server_;
        config.server_address = NetworkAddress::ipv4(127, 0, 0, 1, port_);
        config.transport_protocol = TransportProtocol::ReliableUDP;
        config.enable_replication = true;
        config.enable_compression = true;
        config.enable_monitoring = true;
        config.replication_interval = std::chrono::milliseconds(33); // ~30 Hz
        
        // Create network registry
        network_registry_ = std::make_unique<NetworkRegistry>(ecs_registry_, config);
    }
    
    bool initialize() {
        std::cout << "Initializing " << (is_server_ ? "Server" : "Client") << "...\n";
        
        // Initialize network registry
        auto result = network_registry_->initialize();
        if (!result) {
            std::cerr << "Failed to initialize network registry: " << result.error_message << "\n";
            return false;
        }
        
        // Register message factory
        auto& message_factory = MessageFactory::instance();
        message_factory.register_message_type<PlayerActionMessage>();
        
        // Register components for replication
        register_components();
        
        // Set up callbacks
        setup_callbacks();
        
        return true;
    }
    
    bool start() {
        auto result = network_registry_->start();
        if (!result) {
            std::cerr << "Failed to start network registry: " << result.error_message << "\n";
            return false;
        }
        
        if (is_server_) {
            return start_server();
        } else {
            return start_client();
        }
    }
    
    void run() {
        std::cout << "Starting main loop...\n";
        
        auto last_stats_time = std::chrono::steady_clock::now();
        auto last_entity_spawn_time = std::chrono::steady_clock::now();
        
        while (running_) {
            // Update network registry
            network_registry_->update();
            
            // Update game simulation
            update_simulation();
            
            // Print statistics every 5 seconds
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - last_stats_time).count() >= 5) {
                print_statistics();
                last_stats_time = now;
            }
            
            // Server: Spawn entities periodically for demonstration
            if (is_server_ && std::chrono::duration_cast<std::chrono::seconds>(now - last_entity_spawn_time).count() >= 10) {
                spawn_demo_entity();
                last_entity_spawn_time = now;
            }
            
            // Sleep for a short time to prevent busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
        }
        
        // Cleanup
        network_registry_->stop();
        network_registry_->shutdown();
    }
    
    void stop() {
        running_ = false;
    }

private:
    bool is_server_;
    std::string server_address_;
    uint16_t port_;
    std::shared_ptr<Registry> ecs_registry_;
    std::unique_ptr<NetworkRegistry> network_registry_;
    std::atomic<bool> running_{true};
    
    // Game state
    std::vector<EntityId> player_entities_;
    std::random_device random_device_;
    std::mt19937 random_generator_{random_device_()};
    
    void register_components() {
        // Register components for replication with different settings
        network_registry_->register_component_for_replication<Position>(
            "Position", 
            ReplicationFlags::POSITION_COMPONENT,  // High frequency with delta compression
            1  // Update every tick
        );
        
        network_registry_->register_component_for_replication<Velocity>(
            "Velocity",
            ReplicationFlags::REPLICATED_DELTA,  // Delta compression for efficiency
            2  // Update every 2 ticks
        );
        
        network_registry_->register_component_for_replication<PlayerInfo>(
            "PlayerInfo",
            ReplicationFlags::STATIC_COMPONENT,  // Low frequency, reliable delivery
            30 // Update every 30 ticks (once per second at 30 Hz)
        );
    }
    
    void setup_callbacks() {
        // Connection events
        network_registry_->set_connection_event_callback(
            [this](ConnectionId connection_id, ConnectionState state) {
                handle_connection_event(connection_id, state);
            });
        
        // Client authentication (server only)
        if (is_server_) {
            network_registry_->set_client_auth_callback(
                [this](ConnectionId connection_id, const std::string& client_name) {
                    return handle_client_auth(connection_id, client_name);
                });
        }
        
        // Error handling
        network_registry_->set_error_callback(
            [this](ConnectionId connection_id, NetworkError error, const std::string& message) {
                handle_network_error(connection_id, error, message);
            });
        
        // Message handlers
        network_registry_->register_message_handler<TextMessage>(
            [this](ConnectionId connection_id, const TextMessage& message) {
                handle_text_message(connection_id, message);
            });
        
        network_registry_->register_message_handler<PlayerActionMessage>(
            [this](ConnectionId connection_id, const PlayerActionMessage& message) {
                handle_player_action_message(connection_id, message);
            });
    }
    
    bool start_server() {
        std::cout << "Starting server on port " << port_ << "...\n";
        
        auto result = network_registry_->start_server();
        if (!result) {
            std::cerr << "Failed to start server: " << result.error_message << "\n";
            return false;
        }
        
        std::cout << "Server started successfully! Waiting for clients...\n";
        return true;
    }
    
    bool start_client() {
        std::cout << "Connecting to server at " << server_address_ << ":" << port_ << "...\n";
        
        NetworkAddress server_addr = NetworkAddress::ipv4(127, 0, 0, 1, port_);
        auto result = network_registry_->connect_to_server(server_addr);
        if (!result) {
            std::cerr << "Failed to connect to server: " << result.error_message << "\n";
            return false;
        }
        
        std::cout << "Connected to server with connection ID: " << result.value << "\n";
        return true;
    }
    
    void update_simulation() {
        // Simple simulation: Move entities with velocity
        auto position_view = ecs_registry_->view<Position, Velocity>();
        
        for (auto entity : position_view) {
            auto& position = ecs_registry_->get_component<Position>(entity);
            const auto& velocity = ecs_registry_->get_component<Velocity>(entity);
            
            // Update position based on velocity (assuming 60 FPS)
            position.x += velocity.dx / 60.0f;
            position.y += velocity.dy / 60.0f;
            position.z += velocity.dz / 60.0f;
            
            // Simple boundary wrapping
            if (position.x > 100.0f) position.x = -100.0f;
            if (position.x < -100.0f) position.x = 100.0f;
            if (position.y > 100.0f) position.y = -100.0f;
            if (position.y < -100.0f) position.y = 100.0f;
        }
    }
    
    void spawn_demo_entity() {
        if (!is_server_) return;
        
        std::cout << "Spawning demo entity...\n";
        
        // Create a new entity with random properties
        std::uniform_real_distribution<float> pos_dist(-50.0f, 50.0f);
        std::uniform_real_distribution<float> vel_dist(-10.0f, 10.0f);
        std::uniform_int_distribution<int> health_dist(50, 100);
        
        EntityId entity = ecs_registry_->create_entity();
        
        Position pos(pos_dist(random_generator_), pos_dist(random_generator_));
        Velocity vel(vel_dist(random_generator_), vel_dist(random_generator_));
        PlayerInfo info("DemoEntity_" + std::to_string(entity));
        info.health = health_dist(random_generator_);
        
        ecs_registry_->add_component<Position>(entity, pos);
        ecs_registry_->add_component<Velocity>(entity, vel);
        ecs_registry_->add_component<PlayerInfo>(entity, info);
        
        // Register for replication (server owns the entity)
        network_registry_->register_replicated_entity<Position, Velocity, PlayerInfo>(entity, 0);
        
        player_entities_.push_back(entity);
    }
    
    void handle_connection_event(ConnectionId connection_id, ConnectionState state) {
        std::cout << "Connection " << connection_id << " state changed to: ";
        switch (state) {
            case ConnectionState::Connected:
                std::cout << "Connected\n";
                if (is_server_) {
                    // Send welcome message to new client
                    auto welcome_msg = std::make_unique<TextMessage>("Welcome to the ECScope networking demo!");
                    network_registry_->send_message(connection_id, std::move(welcome_msg));
                }
                break;
            case ConnectionState::Disconnected:
                std::cout << "Disconnected\n";
                break;
            case ConnectionState::Connecting:
                std::cout << "Connecting\n";
                break;
            case ConnectionState::Disconnecting:
                std::cout << "Disconnecting\n";
                break;
            default:
                std::cout << "Unknown\n";
                break;
        }
    }
    
    bool handle_client_auth(ConnectionId connection_id, const std::string& client_name) {
        std::cout << "Client authentication request from connection " << connection_id 
                  << " with name: " << client_name << "\n";
        
        // Simple authentication - accept all clients for this demo
        // In a real application, you would validate credentials here
        return true;
    }
    
    void handle_network_error(ConnectionId connection_id, NetworkError error, const std::string& message) {
        std::cerr << "Network error on connection " << connection_id << ": " << message << "\n";
    }
    
    void handle_text_message(ConnectionId connection_id, const TextMessage& message) {
        std::cout << "Received text message from connection " << connection_id 
                  << ": " << message.get_text() << "\n";
        
        // Echo the message back to demonstrate bidirectional communication
        if (is_server_) {
            auto echo_msg = std::make_unique<TextMessage>("Echo: " + message.get_text());
            network_registry_->send_message(connection_id, std::move(echo_msg));
        }
    }
    
    void handle_player_action_message(ConnectionId connection_id, const PlayerActionMessage& message) {
        std::cout << "Received player action from connection " << connection_id 
                  << " - Type: " << static_cast<int>(message.get_action_type())
                  << ", Data: " << message.get_action_data() << "\n";
        
        // Handle different action types
        switch (message.get_action_type()) {
            case PlayerActionMessage::MOVE:
                // Process movement command
                break;
            case PlayerActionMessage::ATTACK:
                // Process attack command
                break;
            case PlayerActionMessage::CHAT:
                // Broadcast chat message to all clients
                if (is_server_) {
                    auto chat_msg = std::make_unique<TextMessage>("Chat: " + message.get_action_data());
                    network_registry_->broadcast_message(std::move(chat_msg));
                }
                break;
        }
    }
    
    void print_statistics() {
        std::cout << "\n=== Network Statistics ===\n";
        
        auto network_stats = network_registry_->get_network_statistics();
        std::cout << "Bytes sent: " << network_stats.bytes_sent << "\n";
        std::cout << "Bytes received: " << network_stats.bytes_received << "\n";
        std::cout << "Packets sent: " << network_stats.packets_sent << "\n";
        std::cout << "Packets received: " << network_stats.packets_received << "\n";
        std::cout << "Round trip time: " << network_stats.round_trip_time << " ms\n";
        
        auto monitor_stats = network_registry_->get_monitoring_metrics();
        std::cout << "Active connections: " << monitor_stats.active_connections << "\n";
        std::cout << "Average quality: " << monitor_stats.average_quality << "\n";
        
        auto replication_stats = network_registry_->get_replication_statistics();
        std::cout << "Entities replicated: " << replication_stats.entities_replicated << "\n";
        std::cout << "Components updated: " << replication_stats.components_updated << "\n";
        std::cout << "Compression ratio: " << replication_stats.average_compression_ratio << "\n";
        
        std::cout << "=========================\n\n";
    }
};

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [--server|--client] [options]\n";
    std::cout << "Options:\n";
    std::cout << "  --server              Start as server\n";
    std::cout << "  --client              Start as client\n";
    std::cout << "  --address <addr>      Server address (default: 127.0.0.1)\n";
    std::cout << "  --port <port>         Server port (default: 7777)\n";
    std::cout << "  --help                Show this help message\n";
}

int main(int argc, char* argv[]) {
    bool is_server = false;
    bool is_client = false;
    std::string server_address = "127.0.0.1";
    uint16_t port = 7777;
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--server") {
            is_server = true;
        } else if (arg == "--client") {
            is_client = true;
        } else if (arg == "--address" && i + 1 < argc) {
            server_address = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            port = static_cast<uint16_t>(std::stoi(argv[++i]));
        } else if (arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            print_usage(argv[0]);
            return 1;
        }
    }
    
    if (!is_server && !is_client) {
        std::cerr << "Must specify either --server or --client\n";
        print_usage(argv[0]);
        return 1;
    }
    
    if (is_server && is_client) {
        std::cerr << "Cannot specify both --server and --client\n";
        print_usage(argv[0]);
        return 1;
    }
    
    try {
        BasicNetworkingDemo demo(is_server, server_address, port);
        
        if (!demo.initialize()) {
            std::cerr << "Failed to initialize demo\n";
            return 1;
        }
        
        if (!demo.start()) {
            std::cerr << "Failed to start demo\n";
            return 1;
        }
        
        // Set up signal handling to gracefully shut down
        std::signal(SIGINT, [](int) {
            std::cout << "\nShutting down...\n";
            std::exit(0);
        });
        
        demo.run();
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}