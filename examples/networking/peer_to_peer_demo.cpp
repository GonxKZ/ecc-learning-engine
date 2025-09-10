/**
 * @file peer_to_peer_demo.cpp
 * @brief Peer-to-Peer Networking Example
 * 
 * This example demonstrates P2P networking capabilities where multiple
 * peers can connect directly to each other without a central server:
 * 
 * - Direct peer-to-peer connections
 * - Distributed entity ownership
 * - Peer discovery and connection management
 * - Conflict resolution without central authority
 * - Real-time collaborative simulation
 * 
 * Usage:
 * - Run with --port <port> to specify listening port
 * - Run with --connect <address>:<port> to connect to another peer
 * - Each peer can accept connections and connect to others
 */

#include "../../include/ecscope/networking/network_registry.hpp"
#include "../../include/ecscope/ecs/registry.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <random>
#include <sstream>

using namespace ecscope;
using namespace ecscope::networking;
using namespace ecscope::ecs;

// Shared game components
struct Transform {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    float rotation = 0.0f;
    float scale = 1.0f;
    
    Transform() = default;
    Transform(float x, float y, float z = 0.0f, float rot = 0.0f, float s = 1.0f) 
        : x(x), y(y), z(z), rotation(rot), scale(s) {}
};

struct RigidBody {
    float velocity_x = 0.0f, velocity_y = 0.0f, velocity_z = 0.0f;
    float angular_velocity = 0.0f;
    float mass = 1.0f;
    float friction = 0.9f;
    
    RigidBody() = default;
    RigidBody(float mass, float friction = 0.9f) : mass(mass), friction(friction) {}
};

struct PeerInfo {
    std::string peer_name;
    uint32_t peer_id;
    NetworkTimestamp join_time;
    int32_t entities_owned = 0;
    
    PeerInfo() = default;
    PeerInfo(const std::string& name, uint32_t id) 
        : peer_name(name), peer_id(id), join_time(timing::now()) {}
};

// P2P specific messages
class PeerDiscoveryMessage : public NetworkMessage {
public:
    static constexpr uint16_t MESSAGE_TYPE = 2000;
    
    PeerDiscoveryMessage() : NetworkMessage(MESSAGE_TYPE) {}
    PeerDiscoveryMessage(const std::string& peer_name, uint16_t listening_port)
        : NetworkMessage(MESSAGE_TYPE), peer_name_(peer_name), listening_port_(listening_port) {}
    
    const std::string& get_peer_name() const { return peer_name_; }
    uint16_t get_listening_port() const { return listening_port_; }
    
    size_t get_serialized_size() const override {
        return sizeof(MessageHeader) + serialization::size_of_string(peer_name_) + sizeof(listening_port_);
    }

protected:
    void serialize_payload(std::vector<uint8_t>& buffer) const override {
        serialization::write_string(buffer, peer_name_);
        serialization::write_uint16(buffer, listening_port_);
    }
    
    bool deserialize_payload(const uint8_t* data, size_t size) override {
        const uint8_t* ptr = data;
        size_t remaining = size;
        
        if (!serialization::read_string(ptr, remaining, peer_name_)) return false;
        if (!serialization::read_uint16(ptr, remaining, listening_port_)) return false;
        
        return true;
    }

private:
    std::string peer_name_;
    uint16_t listening_port_ = 0;
};

class EntityOwnershipRequestMessage : public NetworkMessage {
public:
    static constexpr uint16_t MESSAGE_TYPE = 2001;
    
    EntityOwnershipRequestMessage() : NetworkMessage(MESSAGE_TYPE) {}
    EntityOwnershipRequestMessage(NetworkEntityID entity_id, ClientID requester_id)
        : NetworkMessage(MESSAGE_TYPE), entity_id_(entity_id), requester_id_(requester_id) {}
    
    NetworkEntityID get_entity_id() const { return entity_id_; }
    ClientID get_requester_id() const { return requester_id_; }
    
    size_t get_serialized_size() const override {
        return sizeof(MessageHeader) + sizeof(entity_id_) + sizeof(requester_id_);
    }

protected:
    void serialize_payload(std::vector<uint8_t>& buffer) const override {
        serialization::write_uint64(buffer, entity_id_);
        serialization::write_uint32(buffer, requester_id_);
    }
    
    bool deserialize_payload(const uint8_t* data, size_t size) override {
        const uint8_t* ptr = data;
        size_t remaining = size;
        
        if (!serialization::read_uint64(ptr, remaining, entity_id_)) return false;
        if (!serialization::read_uint32(ptr, remaining, requester_id_)) return false;
        
        return true;
    }

private:
    NetworkEntityID entity_id_ = 0;
    ClientID requester_id_ = 0;
};

class PeerToPeerDemo {
public:
    PeerToPeerDemo(uint16_t listen_port, const std::string& peer_name)
        : listen_port_(listen_port), peer_name_(peer_name) {
        
        // Generate unique peer ID based on name and port
        std::hash<std::string> hasher;
        peer_id_ = static_cast<uint32_t>(hasher(peer_name + std::to_string(listen_port)));
        
        // Create ECS registry
        ecs_registry_ = std::make_shared<Registry>();
        
        // Configure network registry for P2P
        NetworkRegistryConfig config;
        config.is_server = true; // P2P peers act as both server and client
        config.server_address = NetworkAddress::ipv4(0, 0, 0, 0, listen_port); // Listen on all interfaces
        config.transport_protocol = TransportProtocol::ReliableUDP;
        config.enable_replication = true;
        config.enable_compression = true;
        config.enable_monitoring = true;
        config.enable_interest_management = true;
        config.interest_radius = 50.0; // Only replicate nearby entities
        config.replication_interval = std::chrono::milliseconds(50); // 20 Hz
        
        network_registry_ = std::make_unique<NetworkRegistry>(ecs_registry_, config);
        
        std::cout << "Initialized P2P peer '" << peer_name_ << "' (ID: " << peer_id_ 
                  << ") listening on port " << listen_port_ << "\n";
    }
    
    bool initialize() {
        auto result = network_registry_->initialize();
        if (!result) {
            std::cerr << "Failed to initialize network registry: " << result.error_message << "\n";
            return false;
        }
        
        // Register message types
        auto& message_factory = MessageFactory::instance();
        message_factory.register_message_type<PeerDiscoveryMessage>();
        message_factory.register_message_type<EntityOwnershipRequestMessage>();
        
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
        
        // Start listening for connections
        result = network_registry_->start_server();
        if (!result) {
            std::cerr << "Failed to start P2P server: " << result.error_message << "\n";
            return false;
        }
        
        std::cout << "P2P peer started successfully. Ready to accept connections and connect to others.\n";
        return true;
    }
    
    bool connect_to_peer(const std::string& address, uint16_t port) {
        std::cout << "Attempting to connect to peer at " << address << ":" << port << "\n";
        
        NetworkAddress peer_address = NetworkAddress::ipv4(
            std::stoi(address.substr(0, address.find('.'))),
            std::stoi(address.substr(address.find('.') + 1, address.find('.', address.find('.') + 1) - address.find('.') - 1)),
            std::stoi(address.substr(address.find('.', address.find('.') + 1) + 1, address.rfind('.') - address.find('.', address.find('.') + 1) - 1)),
            std::stoi(address.substr(address.rfind('.') + 1)),
            port
        );
        
        auto result = network_registry_->connect_to_server(peer_address);
        if (!result) {
            std::cerr << "Failed to connect to peer: " << result.error_message << "\n";
            return false;
        }
        
        std::cout << "Connected to peer with connection ID: " << result.value << "\n";
        
        // Send discovery message to introduce ourselves
        auto discovery_msg = std::make_unique<PeerDiscoveryMessage>(peer_name_, listen_port_);
        network_registry_->send_message(result.value, std::move(discovery_msg));
        
        return true;
    }
    
    void run() {
        std::cout << "Starting P2P simulation loop...\n";
        std::cout << "Commands:\n";
        std::cout << "  spawn - Create a new entity\n";
        std::cout << "  connect <ip:port> - Connect to another peer\n";
        std::cout << "  peers - List connected peers\n";
        std::cout << "  entities - List all entities\n";
        std::cout << "  stats - Show network statistics\n";
        std::cout << "  quit - Exit the application\n\n";
        
        auto last_update_time = std::chrono::steady_clock::now();
        auto last_stats_time = std::chrono::steady_clock::now();
        
        // Start input thread for interactive commands
        std::thread input_thread(&PeerToPeerDemo::input_thread_function, this);
        
        while (running_) {
            // Update network registry
            network_registry_->update();
            
            // Update simulation
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update_time).count() >= 50) {
                update_simulation();
                last_update_time = now;
            }
            
            // Auto-print stats every 30 seconds
            if (std::chrono::duration_cast<std::chrono::seconds>(now - last_stats_time).count() >= 30) {
                print_statistics();
                last_stats_time = now;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        input_thread.join();
        
        // Cleanup
        network_registry_->stop();
        network_registry_->shutdown();
    }
    
    void stop() {
        running_ = false;
    }

private:
    uint16_t listen_port_;
    std::string peer_name_;
    uint32_t peer_id_;
    std::shared_ptr<Registry> ecs_registry_;
    std::unique_ptr<NetworkRegistry> network_registry_;
    std::atomic<bool> running_{true};
    
    // P2P state
    std::unordered_map<ConnectionId, std::string> connected_peers_;
    std::vector<EntityId> owned_entities_;
    std::random_device random_device_;
    std::mt19937 random_generator_{random_device_()};
    
    void register_components() {
        network_registry_->register_component_for_replication<Transform>(
            "Transform",
            ReplicationFlags::POSITION_COMPONENT,
            1
        );
        
        network_registry_->register_component_for_replication<RigidBody>(
            "RigidBody",
            ReplicationFlags::REPLICATED_DELTA,
            2
        );
        
        network_registry_->register_component_for_replication<PeerInfo>(
            "PeerInfo",
            ReplicationFlags::STATIC_COMPONENT,
            60
        );
    }
    
    void setup_callbacks() {
        network_registry_->set_connection_event_callback(
            [this](ConnectionId connection_id, ConnectionState state) {
                handle_connection_event(connection_id, state);
            });
        
        network_registry_->set_client_auth_callback(
            [this](ConnectionId connection_id, const std::string& client_name) {
                return handle_peer_auth(connection_id, client_name);
            });
        
        network_registry_->set_error_callback(
            [this](ConnectionId connection_id, NetworkError error, const std::string& message) {
                handle_network_error(connection_id, error, message);
            });
        
        // Message handlers
        network_registry_->register_message_handler<PeerDiscoveryMessage>(
            [this](ConnectionId connection_id, const PeerDiscoveryMessage& message) {
                handle_peer_discovery(connection_id, message);
            });
        
        network_registry_->register_message_handler<EntityOwnershipRequestMessage>(
            [this](ConnectionId connection_id, const EntityOwnershipRequestMessage& message) {
                handle_ownership_request(connection_id, message);
            });
        
        network_registry_->register_message_handler<TextMessage>(
            [this](ConnectionId connection_id, const TextMessage& message) {
                handle_text_message(connection_id, message);
            });
    }
    
    void update_simulation() {
        // Physics simulation for owned entities
        auto physics_view = ecs_registry_->view<Transform, RigidBody>();
        
        for (auto entity : physics_view) {
            // Only update entities we own
            ClientID owner = network_registry_->get_entity_owner(entity);
            if (owner != peer_id_) continue;
            
            auto& transform = ecs_registry_->get_component<Transform>(entity);
            auto& rigidbody = ecs_registry_->get_component<RigidBody>(entity);
            
            // Simple physics: apply velocity and friction
            transform.x += rigidbody.velocity_x * 0.02f; // Assuming 50 Hz update
            transform.y += rigidbody.velocity_y * 0.02f;
            transform.z += rigidbody.velocity_z * 0.02f;
            transform.rotation += rigidbody.angular_velocity * 0.02f;
            
            // Apply friction
            rigidbody.velocity_x *= rigidbody.friction;
            rigidbody.velocity_y *= rigidbody.friction;
            rigidbody.velocity_z *= rigidbody.friction;
            rigidbody.angular_velocity *= rigidbody.friction;
            
            // Boundary wrapping
            if (transform.x > 100.0f) transform.x = -100.0f;
            if (transform.x < -100.0f) transform.x = 100.0f;
            if (transform.y > 100.0f) transform.y = -100.0f;
            if (transform.y < -100.0f) transform.y = 100.0f;
        }
    }
    
    void handle_connection_event(ConnectionId connection_id, ConnectionState state) {
        std::string peer_name = connected_peers_.count(connection_id) ? 
                               connected_peers_[connection_id] : "Unknown";
        
        std::cout << "Peer '" << peer_name << "' (connection " << connection_id 
                  << ") state: " << static_cast<int>(state) << "\n";
        
        if (state == ConnectionState::Disconnected) {
            connected_peers_.erase(connection_id);
        }
    }
    
    bool handle_peer_auth(ConnectionId connection_id, const std::string& client_name) {
        std::cout << "Peer authentication request from: " << client_name << "\n";
        // Accept all peers for this demo
        return true;
    }
    
    void handle_network_error(ConnectionId connection_id, NetworkError error, const std::string& message) {
        std::cerr << "Network error on connection " << connection_id << ": " << message << "\n";
    }
    
    void handle_peer_discovery(ConnectionId connection_id, const PeerDiscoveryMessage& message) {
        std::cout << "Discovered peer: " << message.get_peer_name() 
                  << " (listening on port " << message.get_listening_port() << ")\n";
        
        connected_peers_[connection_id] = message.get_peer_name();
    }
    
    void handle_ownership_request(ConnectionId connection_id, const EntityOwnershipRequestMessage& message) {
        std::cout << "Ownership request for entity " << message.get_entity_id() 
                  << " from peer " << message.get_requester_id() << "\n";
        
        // Simple ownership transfer logic - accept if we own the entity
        EntityId local_entity = 0; // Convert from NetworkEntityID
        ClientID current_owner = network_registry_->get_entity_owner(local_entity);
        
        if (current_owner == peer_id_) {
            network_registry_->set_entity_owner(local_entity, message.get_requester_id());
            std::cout << "Transferred ownership of entity " << message.get_entity_id() << "\n";
        }
    }
    
    void handle_text_message(ConnectionId connection_id, const TextMessage& message) {
        std::string peer_name = connected_peers_.count(connection_id) ? 
                               connected_peers_[connection_id] : "Unknown";
        std::cout << "[" << peer_name << "]: " << message.get_text() << "\n";
    }
    
    void input_thread_function() {
        std::string input;
        while (running_) {
            std::getline(std::cin, input);
            if (input.empty()) continue;
            
            std::istringstream iss(input);
            std::string command;
            iss >> command;
            
            if (command == "spawn") {
                spawn_entity();
            } else if (command == "connect") {
                std::string address;
                iss >> address;
                if (!address.empty()) {
                    size_t colon_pos = address.find(':');
                    if (colon_pos != std::string::npos) {
                        std::string ip = address.substr(0, colon_pos);
                        uint16_t port = static_cast<uint16_t>(std::stoi(address.substr(colon_pos + 1)));
                        connect_to_peer(ip, port);
                    } else {
                        std::cout << "Invalid address format. Use ip:port\n";
                    }
                } else {
                    std::cout << "Usage: connect <ip:port>\n";
                }
            } else if (command == "peers") {
                list_peers();
            } else if (command == "entities") {
                list_entities();
            } else if (command == "stats") {
                print_statistics();
            } else if (command == "quit" || command == "exit") {
                stop();
                break;
            } else if (command == "help") {
                std::cout << "Commands: spawn, connect <ip:port>, peers, entities, stats, quit\n";
            } else {
                std::cout << "Unknown command: " << command << "\n";
            }
        }
    }
    
    void spawn_entity() {
        std::uniform_real_distribution<float> pos_dist(-50.0f, 50.0f);
        std::uniform_real_distribution<float> vel_dist(-20.0f, 20.0f);
        std::uniform_real_distribution<float> mass_dist(0.5f, 3.0f);
        
        EntityId entity = ecs_registry_->create_entity();
        
        Transform transform(pos_dist(random_generator_), pos_dist(random_generator_));
        RigidBody rigidbody(mass_dist(random_generator_));
        rigidbody.velocity_x = vel_dist(random_generator_);
        rigidbody.velocity_y = vel_dist(random_generator_);
        
        PeerInfo peer_info(peer_name_, peer_id_);
        peer_info.entities_owned = static_cast<int32_t>(owned_entities_.size() + 1);
        
        ecs_registry_->add_component<Transform>(entity, transform);
        ecs_registry_->add_component<RigidBody>(entity, rigidbody);
        ecs_registry_->add_component<PeerInfo>(entity, peer_info);
        
        // Register for replication with this peer as owner
        network_registry_->register_replicated_entity<Transform, RigidBody, PeerInfo>(entity, peer_id_);
        
        owned_entities_.push_back(entity);
        
        std::cout << "Spawned entity " << entity << " at (" << transform.x 
                  << ", " << transform.y << ")\n";
    }
    
    void list_peers() {
        std::cout << "Connected peers:\n";
        for (const auto& [connection_id, peer_name] : connected_peers_) {
            std::cout << "  - " << peer_name << " (connection " << connection_id << ")\n";
        }
        if (connected_peers_.empty()) {
            std::cout << "  No peers connected.\n";
        }
    }
    
    void list_entities() {
        auto entity_view = ecs_registry_->view<Transform, PeerInfo>();
        
        std::cout << "All entities in simulation:\n";
        for (auto entity : entity_view) {
            const auto& transform = ecs_registry_->get_component<Transform>(entity);
            const auto& peer_info = ecs_registry_->get_component<PeerInfo>(entity);
            ClientID owner = network_registry_->get_entity_owner(entity);
            
            std::cout << "  Entity " << entity << " - Owner: " << peer_info.peer_name 
                      << " (" << owner << ") - Position: (" << transform.x 
                      << ", " << transform.y << ")\n";
        }
    }
    
    void print_statistics() {
        std::cout << "\n=== P2P Network Statistics ===\n";
        
        auto network_stats = network_registry_->get_network_statistics();
        std::cout << "Network Traffic:\n";
        std::cout << "  Bytes sent: " << network_stats.bytes_sent << "\n";
        std::cout << "  Bytes received: " << network_stats.bytes_received << "\n";
        std::cout << "  Packets sent: " << network_stats.packets_sent << "\n";
        std::cout << "  Packets received: " << network_stats.packets_received << "\n";
        std::cout << "  Packet loss: " << network_stats.packet_loss_rate * 100 << "%\n";
        
        auto monitor_stats = network_registry_->get_monitoring_metrics();
        std::cout << "Connection Status:\n";
        std::cout << "  Active connections: " << monitor_stats.active_connections << "\n";
        std::cout << "  Average quality: " << monitor_stats.average_quality << "\n";
        
        auto replication_stats = network_registry_->get_replication_statistics();
        std::cout << "Replication:\n";
        std::cout << "  Entities replicated: " << replication_stats.entities_replicated << "\n";
        std::cout << "  Components updated: " << replication_stats.components_updated << "\n";
        std::cout << "  Delta compressions: " << replication_stats.delta_compressions_used << "\n";
        
        std::cout << "Game State:\n";
        std::cout << "  Owned entities: " << owned_entities_.size() << "\n";
        std::cout << "  Total entities: " << ecs_registry_->get_entity_count() << "\n";
        
        std::cout << "==============================\n\n";
    }
};

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " --port <port> [options]\n";
    std::cout << "Options:\n";
    std::cout << "  --port <port>         Port to listen on\n";
    std::cout << "  --name <name>         Peer name (default: random)\n";
    std::cout << "  --connect <ip:port>   Auto-connect to another peer on startup\n";
    std::cout << "  --help                Show this help message\n";
}

int main(int argc, char* argv[]) {
    uint16_t listen_port = 0;
    std::string peer_name = "Peer_" + std::to_string(std::time(nullptr) % 10000);
    std::string auto_connect;
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--port" && i + 1 < argc) {
            listen_port = static_cast<uint16_t>(std::stoi(argv[++i]));
        } else if (arg == "--name" && i + 1 < argc) {
            peer_name = argv[++i];
        } else if (arg == "--connect" && i + 1 < argc) {
            auto_connect = argv[++i];
        } else if (arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            print_usage(argv[0]);
            return 1;
        }
    }
    
    if (listen_port == 0) {
        std::cerr << "Must specify --port\n";
        print_usage(argv[0]);
        return 1;
    }
    
    try {
        PeerToPeerDemo demo(listen_port, peer_name);
        
        if (!demo.initialize()) {
            std::cerr << "Failed to initialize P2P demo\n";
            return 1;
        }
        
        if (!demo.start()) {
            std::cerr << "Failed to start P2P demo\n";
            return 1;
        }
        
        // Auto-connect if specified
        if (!auto_connect.empty()) {
            size_t colon_pos = auto_connect.find(':');
            if (colon_pos != std::string::npos) {
                std::string ip = auto_connect.substr(0, colon_pos);
                uint16_t port = static_cast<uint16_t>(std::stoi(auto_connect.substr(colon_pos + 1)));
                demo.connect_to_peer(ip, port);
            }
        }
        
        // Set up signal handling
        std::signal(SIGINT, [](int) {
            std::cout << "\nShutting down P2P demo...\n";
            std::exit(0);
        });
        
        demo.run();
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}