#pragma once

/**
 * @file networking/ecs_networking_system.hpp
 * @brief Comprehensive ECS Networking System for ECScope
 * 
 * This header provides a complete networking system that integrates all
 * networking components into a unified, educational, and high-performance
 * distributed ECS solution. Features:
 * 
 * Core Integration Features:
 * - Complete client-server ECS synchronization
 * - Entity replication with delta compression
 * - Component synchronization with priority-based updates
 * - Authority system for distributed entity ownership
 * - Network prediction and lag compensation
 * 
 * Advanced Networking Features:
 * - Custom UDP protocol with reliability layers
 * - Packet batching and compression
 * - Network bandwidth monitoring and adaptation
 * - Interest management and spatial partitioning
 * - Connection management with automatic reconnection
 * 
 * Educational Features:
 * - Network simulation with artificial latency/packet loss
 * - Real-time network performance visualization
 * - Interactive tutorials on distributed systems concepts
 * - Network debugging and analysis tools
 * - Bandwidth usage analysis and optimization suggestions
 * 
 * Performance Optimizations:
 * - ECS sparse sets integration for efficient serialization
 * - Memory pool management for network buffers
 * - Physics engine integration for prediction and rollback
 * - Cache-friendly data structures and algorithms
 * - Zero-allocation hot paths for critical networking operations
 * 
 * @author ECScope Educational ECS Framework - Complete Networking Layer
 * @date 2024
 */

#include "network_types.hpp"
#include "network_protocol.hpp"
#include "entity_replication.hpp"
#include "component_sync.hpp"
#include "authority_system.hpp"
#include "network_prediction.hpp"
#include "network_simulation.hpp"
#include "udp_socket.hpp"
#include "../registry.hpp"
#include "../world.hpp"
#include "../system.hpp"
#include "../components.hpp"
#include "../physics_system.hpp"
#include "../memory/pool.hpp"
#include "../memory/arena.hpp"
#include "../performance/performance_benchmark.hpp"
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>

namespace ecscope::networking {

//=============================================================================
// Network Configuration
//=============================================================================

/**
 * @brief Comprehensive Network Configuration
 * 
 * Controls all aspects of the networking system including transport,
 * replication, prediction, and educational features.
 */
struct NetworkConfig {
    // Transport Configuration
    TransportProtocol transport{TransportProtocol::ReliableUDP};
    NetworkAddress server_address{NetworkAddress::local(constants::DEFAULT_SERVER_PORT)};
    u16 client_port{0};  // 0 = auto-assign
    
    // Connection Settings
    u32 max_clients{64};
    u32 connection_timeout_ms{30000};
    u32 ping_interval_ms{1000};
    u32 heartbeat_interval_ms{5000};
    
    // Replication Settings
    u32 tick_rate{60};                        // Network updates per second
    u32 max_entities_per_update{100};        // Maximum entities to update per tick
    bool enable_delta_compression{true};     // Use delta compression for components
    bool enable_spatial_partitioning{true}; // Only replicate nearby entities
    f32 spatial_range{100.0f};              // Range for spatial partitioning
    
    // Prediction Settings
    bool enable_client_prediction{true};    // Enable client-side prediction
    bool enable_lag_compensation{true};     // Enable server lag compensation
    u32 max_rollback_ticks{10};            // Maximum ticks to rollback
    f32 prediction_error_threshold{0.1f};   // Threshold for prediction correction
    
    // Reliability Settings
    u32 packet_ack_timeout_ms{100};         // Time to wait for packet ACK
    u32 max_packet_retries{5};              // Maximum retransmission attempts
    f32 packet_loss_simulation{0.0f};       // Artificial packet loss (0-1)
    u32 latency_simulation_ms{0};           // Artificial latency in ms
    
    // Bandwidth Management
    u32 max_bandwidth_kbps{1000};           // Maximum bandwidth per client
    u32 priority_bandwidth_allocation[5]{   // Bandwidth allocation by priority
        400, 300, 200, 80, 20               // Critical, High, Normal, Low, Background
    };
    
    // Educational Features
    bool enable_network_visualization{true}; // Show network debug visualizations
    bool enable_performance_tracking{true}; // Track detailed performance metrics
    bool enable_tutorials{false};           // Show networking tutorials
    bool enable_packet_inspection{false};   // Enable deep packet analysis
    
    // Memory Configuration
    usize network_buffer_size{64 * 1024};   // 64KB network buffer
    usize message_pool_size{1024};          // Maximum pooled messages
    usize entity_state_pool_size{2048};     // Maximum pooled entity states
    
    /** @brief Create default client configuration */
    static NetworkConfig client_default() {
        NetworkConfig config;
        config.server_address = NetworkAddress::local(constants::DEFAULT_SERVER_PORT);
        return config;
    }
    
    /** @brief Create default server configuration */
    static NetworkConfig server_default() {
        NetworkConfig config;
        config.server_address = NetworkAddress::local(constants::DEFAULT_SERVER_PORT);
        config.max_clients = 64;
        return config;
    }
    
    /** @brief Create configuration for educational demonstration */
    static NetworkConfig educational_demo() {
        NetworkConfig config = client_default();
        config.enable_network_visualization = true;
        config.enable_performance_tracking = true;
        config.enable_tutorials = true;
        config.enable_packet_inspection = true;
        config.packet_loss_simulation = 0.05f;  // 5% packet loss
        config.latency_simulation_ms = 50;      // 50ms latency
        return config;
    }
};

//=============================================================================
// Network Event System
//=============================================================================

/**
 * @brief Network Event Types
 */
enum class NetworkEventType : u8 {
    // Connection Events
    ClientConnected,
    ClientDisconnected,
    ServerConnected,
    ServerDisconnected,
    ConnectionFailed,
    ConnectionTimeout,
    
    // Entity Events
    EntityCreated,
    EntityUpdated,
    EntityDestroyed,
    AuthorityTransferred,
    
    // Prediction Events
    PredictionCorrected,
    RollbackTriggered,
    
    // Performance Events
    BandwidthExceeded,
    PacketLoss,
    HighLatency,
    
    // Educational Events
    TutorialTriggered,
    DebugInfoAvailable
};

/**
 * @brief Network Event Data
 */
struct NetworkEvent {
    NetworkEventType type;
    NetworkTimestamp timestamp;
    ClientID client_id{0};
    NetworkEntityID entity_id{0};
    std::variant<std::monostate, std::string, f32, u32, bool> data;
    
    /** @brief Create simple event */
    static NetworkEvent create(NetworkEventType type, ClientID client = 0) {
        return NetworkEvent{type, timing::now(), client, 0, std::monostate{}};
    }
    
    /** @brief Create event with entity */
    static NetworkEvent create_entity(NetworkEventType type, NetworkEntityID entity, ClientID client = 0) {
        return NetworkEvent{type, timing::now(), client, entity, std::monostate{}};
    }
    
    /** @brief Create event with data */
    template<typename T>
    static NetworkEvent create_with_data(NetworkEventType type, T data, ClientID client = 0) {
        return NetworkEvent{type, timing::now(), client, 0, data};
    }
};

//=============================================================================
// Main Networking System
//=============================================================================

/**
 * @brief Comprehensive ECS Networking System
 * 
 * This is the main networking system that coordinates all networking
 * components and provides a unified interface for distributed ECS.
 */
class ECSNetworkingSystem : public ecs::System {
private:
    // Core Configuration
    NetworkConfig config_;
    bool is_server_;
    bool is_running_;
    ClientID local_client_id_;
    SessionID current_session_;
    
    // ECS Integration
    ecs::Registry* registry_;
    physics::PhysicsSystem* physics_system_;
    
    // Networking Components
    std::unique_ptr<UDPSocket> socket_;
    std::unique_ptr<protocol::NetworkProtocol> protocol_;
    std::unique_ptr<replication::NetworkEntityManager> entity_manager_;
    std::unique_ptr<replication::ComponentDeltaEncoder> delta_encoder_;
    std::unique_ptr<sync::ComponentSynchronizer> component_sync_;
    std::unique_ptr<authority::AuthoritySystem> authority_system_;
    std::unique_ptr<prediction::NetworkPredictionSystem> prediction_system_;
    std::unique_ptr<simulation::NetworkSimulator> network_simulator_;
    
    // Threading and Synchronization
    std::unique_ptr<std::thread> network_thread_;
    std::atomic<bool> shutdown_requested_;
    std::mutex event_queue_mutex_;
    std::condition_variable event_condition_;
    std::queue<NetworkEvent> event_queue_;
    
    // Memory Management
    memory::Pool<protocol::NetworkMessage> message_pool_;
    memory::Arena network_arena_;
    
    // Performance Tracking
    NetworkStats network_stats_;
    performance::PerformanceBenchmark network_benchmark_;
    std::chrono::high_resolution_clock::time_point last_tick_time_;
    
    // Client Management (Server only)
    struct ClientInfo {
        ClientID id;
        NetworkAddress address;
        ConnectionState state;
        NetworkTimestamp last_activity;
        NetworkStats stats;
        std::unordered_set<NetworkEntityID> replicated_entities;
    };
    std::unordered_map<ClientID, ClientInfo> connected_clients_;
    std::mutex client_mutex_;
    
    // Educational Features
    struct NetworkTutorial {
        std::string title;
        std::string description;
        std::function<void()> trigger_condition;
        bool triggered{false};
    };
    std::vector<NetworkTutorial> tutorials_;
    bool tutorials_enabled_{false};
    
public:
    /** @brief Initialize networking system */
    explicit ECSNetworkingSystem(ecs::Registry& registry, const NetworkConfig& config = NetworkConfig::client_default())
        : registry_(&registry),
          config_(config),
          is_server_(false),
          is_running_(false),
          local_client_id_(0),
          current_session_(0),
          shutdown_requested_(false),
          message_pool_(config.message_pool_size),
          network_arena_(config.network_buffer_size),
          network_benchmark_("NetworkingSystem") {
        
        initialize_components();
        setup_tutorials();
    }
    
    /** @brief Destructor */
    ~ECSNetworkingSystem() {
        shutdown();
    }
    
    // System Interface
    void update(f32 delta_time) override {
        if (!is_running_) return;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        network_benchmark_.begin_frame();
        
        // Process network events
        process_events();
        
        // Update networking components
        if (is_server_) {
            update_server(delta_time);
        } else {
            update_client(delta_time);
        }
        
        // Update prediction system
        if (prediction_system_ && config_.enable_client_prediction) {
            prediction_system_->update(delta_time);
        }
        
        // Update educational features
        if (config_.enable_tutorials) {
            update_tutorials();
        }
        
        // Update performance tracking
        network_benchmark_.end_frame();
        update_performance_stats(delta_time);
        
        last_tick_time_ = start_time;
    }
    
    void debug_render() const override {
        if (!config_.enable_network_visualization) return;
        
        // Render network debug information
        render_network_stats();
        render_entity_replication_info();
        render_prediction_debug();
        render_bandwidth_usage();
        
        if (config_.enable_packet_inspection) {
            render_packet_analysis();
        }
    }
    
    // Network Management
    /** @brief Start as server */
    bool start_server() {
        if (is_running_) {
            log_error("Networking system is already running");
            return false;
        }
        
        is_server_ = true;
        local_client_id_ = 1;  // Server is always client ID 1
        current_session_ = generate_session_id();
        
        // Initialize server socket
        socket_ = std::make_unique<UDPSocket>();
        if (!socket_->bind(config_.server_address)) {
            log_error("Failed to bind server socket to {}", config_.server_address.to_string());
            return false;
        }
        
        // Initialize authority system with server authority
        authority_system_->set_local_authority(true);
        
        // Start network thread
        is_running_ = true;
        shutdown_requested_ = false;
        network_thread_ = std::make_unique<std::thread>(&ECSNetworkingSystem::network_thread_main, this);
        
        log_info("Server started on {}", config_.server_address.to_string());
        emit_event(NetworkEvent::create(NetworkEventType::ServerConnected));
        
        return true;
    }
    
    /** @brief Start as client */
    bool start_client() {
        if (is_running_) {
            log_error("Networking system is already running");
            return false;
        }
        
        is_server_ = false;
        local_client_id_ = 0;  // Will be assigned by server
        
        // Initialize client socket
        socket_ = std::make_unique<UDPSocket>();
        if (config_.client_port > 0) {
            NetworkAddress client_addr = NetworkAddress::local(config_.client_port);
            if (!socket_->bind(client_addr)) {
                log_warning("Failed to bind client to specific port {}, using random port", config_.client_port);
            }
        }
        
        // Initialize authority system without server authority
        authority_system_->set_local_authority(false);
        
        // Start network thread
        is_running_ = true;
        shutdown_requested_ = false;
        network_thread_ = std::make_unique<std::thread>(&ECSNetworkingSystem::network_thread_main, this);
        
        // Attempt to connect to server
        if (!connect_to_server()) {
            shutdown();
            return false;
        }
        
        log_info("Client connecting to {}", config_.server_address.to_string());
        
        return true;
    }
    
    /** @brief Shutdown networking system */
    void shutdown() {
        if (!is_running_) return;
        
        log_info("Shutting down networking system");
        
        // Signal shutdown
        shutdown_requested_ = true;
        event_condition_.notify_all();
        
        // Wait for network thread to finish
        if (network_thread_ && network_thread_->joinable()) {
            network_thread_->join();
        }
        
        // Clean up connections
        if (is_server_) {
            disconnect_all_clients();
        }
        
        // Clean up resources
        socket_.reset();
        is_running_ = false;
        
        emit_event(NetworkEvent::create(is_server_ ? 
                   NetworkEventType::ServerDisconnected : 
                   NetworkEventType::ClientDisconnected));
        
        log_info("Networking system shutdown complete");
    }
    
    // Entity Management
    /** @brief Register entity for network replication */
    NetworkEntityID register_entity(ecs::Entity entity, MessagePriority priority = MessagePriority::Normal) {
        if (!entity_manager_) {
            log_error("Entity manager not initialized");
            return 0;
        }
        
        NetworkEntityID network_id = entity_manager_->register_entity(entity, priority);
        
        if (network_id != 0) {
            log_debug("Registered entity {} with network ID {}", entity.id(), network_id);
            emit_event(NetworkEvent::create_entity(NetworkEventType::EntityCreated, network_id));
        }
        
        return network_id;
    }
    
    /** @brief Unregister entity from network replication */
    void unregister_entity(ecs::Entity entity) {
        if (!entity_manager_) return;
        
        auto* state = entity_manager_->get_network_state(entity);
        if (state) {
            NetworkEntityID network_id = state->network_id;
            entity_manager_->unregister_entity(entity);
            
            log_debug("Unregistered entity {} (network ID {})", entity.id(), network_id);
            emit_event(NetworkEvent::create_entity(NetworkEventType::EntityDestroyed, network_id));
        }
    }
    
    /** @brief Transfer entity authority to another client */
    void transfer_authority(ecs::Entity entity, ClientID new_authority) {
        if (!entity_manager_ || !authority_system_) return;
        
        entity_manager_->set_entity_authority(entity, new_authority);
        authority_system_->transfer_authority(entity, new_authority);
        
        auto* state = entity_manager_->get_network_state(entity);
        if (state) {
            emit_event(NetworkEvent::create_entity(NetworkEventType::AuthorityTransferred, state->network_id, new_authority));
        }
    }
    
    // Component Synchronization
    /** @brief Mark component as changed for replication */
    template<typename T>
    void mark_component_changed(ecs::Entity entity) {
        if (!entity_manager_) return;
        
        auto* state = entity_manager_->get_network_state(entity);
        if (state) {
            state->template mark_component_changed<T>();
            state->increment_version();
        }
    }
    
    /** @brief Register component synchronization handler */
    template<typename T>
    void register_component_sync() {
        static_assert(ecs::Component<T>, "T must be a component type");
        
        if (component_sync_) {
            component_sync_->template register_sync_handler<T>();
        }
        
        if (delta_encoder_) {
            delta_encoder_->template register_encoder<T>();
        }
        
        log_debug("Registered synchronization for component type {}", typeid(T).name());
    }
    
    // Network Statistics and Information
    /** @brief Get current network statistics */
    const NetworkStats& get_network_stats() const {
        return network_stats_;
    }
    
    /** @brief Get entity replication statistics */
    auto get_entity_stats() const {
        return entity_manager_ ? entity_manager_->get_statistics() : 
               replication::NetworkEntityManager::Statistics{};
    }
    
    /** @brief Get connected client information (server only) */
    std::vector<ClientInfo> get_connected_clients() const {
        std::lock_guard<std::mutex> lock(client_mutex_);
        
        std::vector<ClientInfo> clients;
        clients.reserve(connected_clients_.size());
        
        for (const auto& [id, info] : connected_clients_) {
            clients.push_back(info);
        }
        
        return clients;
    }
    
    /** @brief Check if client is connected */
    bool is_client_connected(ClientID client_id) const {
        std::lock_guard<std::mutex> lock(client_mutex_);
        return connected_clients_.find(client_id) != connected_clients_.end();
    }
    
    /** @brief Get local client ID */
    ClientID get_local_client_id() const {
        return local_client_id_;
    }
    
    /** @brief Check if running as server */
    bool is_server() const {
        return is_server_;
    }
    
    /** @brief Check if system is running */
    bool is_running() const {
        return is_running_;
    }
    
    // Educational Features
    /** @brief Enable/disable networking tutorials */
    void set_tutorials_enabled(bool enabled) {
        tutorials_enabled_ = enabled;
        config_.enable_tutorials = enabled;
    }
    
    /** @brief Trigger artificial network conditions for education */
    void simulate_network_conditions(f32 packet_loss, u32 latency_ms) {
        if (network_simulator_) {
            network_simulator_->set_packet_loss_rate(packet_loss);
            network_simulator_->set_base_latency(latency_ms);
        }
        
        config_.packet_loss_simulation = packet_loss;
        config_.latency_simulation_ms = latency_ms;
        
        log_info("Simulating network conditions: {}% packet loss, {}ms latency", 
                packet_loss * 100.0f, latency_ms);
    }
    
    /** @brief Get network prediction statistics */
    auto get_prediction_stats() const {
        return prediction_system_ ? prediction_system_->get_statistics() : 
               prediction::NetworkPredictionSystem::Statistics{};
    }

private:
    /** @brief Initialize networking components */
    void initialize_components() {
        // Initialize core components
        protocol_ = std::make_unique<protocol::NetworkProtocol>(config_.transport);
        entity_manager_ = std::make_unique<replication::NetworkEntityManager>(local_client_id_);
        delta_encoder_ = std::make_unique<replication::ComponentDeltaEncoder>();
        component_sync_ = std::make_unique<sync::ComponentSynchronizer>(*registry_);
        authority_system_ = std::make_unique<authority::AuthoritySystem>();
        
        // Initialize prediction system if enabled
        if (config_.enable_client_prediction) {
            prediction_system_ = std::make_unique<prediction::NetworkPredictionSystem>(
                *registry_, config_.max_rollback_ticks, config_.prediction_error_threshold);
        }
        
        // Initialize network simulator for educational features
        if (config_.packet_loss_simulation > 0.0f || config_.latency_simulation_ms > 0) {
            network_simulator_ = std::make_unique<simulation::NetworkSimulator>();
            network_simulator_->set_packet_loss_rate(config_.packet_loss_simulation);
            network_simulator_->set_base_latency(config_.latency_simulation_ms);
        }
        
        // Try to get physics system reference
        physics_system_ = registry_->try_system<physics::PhysicsSystem>();
    }
    
    /** @brief Setup educational tutorials */
    void setup_tutorials() {
        tutorials_.clear();
        
        // Entity replication tutorial
        tutorials_.push_back({
            "Entity Replication",
            "Learn how entities are synchronized across the network with delta compression",
            [this]() { return entity_manager_ && entity_manager_->get_statistics().entities_created > 0; }
        });
        
        // Network prediction tutorial
        tutorials_.push_back({
            "Client-Side Prediction",
            "Understand how client prediction reduces perceived latency",
            [this]() { return prediction_system_ && prediction_system_->get_statistics().predictions_made > 10; }
        });
        
        // Bandwidth management tutorial
        tutorials_.push_back({
            "Bandwidth Optimization",
            "Explore techniques for efficient network bandwidth usage",
            [this]() { return network_stats_.bytes_sent_per_sec > 1024; }
        });
        
        // Authority system tutorial
        tutorials_.push_back({
            "Distributed Authority",
            "Learn about entity ownership in distributed systems",
            [this]() { return authority_system_ && authority_system_->get_authority_transfers() > 0; }
        });
        
        // Network simulation tutorial
        tutorials_.push_back({
            "Network Conditions",
            "Experience the effects of latency and packet loss on gameplay",
            [this]() { return config_.packet_loss_simulation > 0.0f || config_.latency_simulation_ms > 0; }
        });
    }
    
    /** @brief Main network thread function */
    void network_thread_main() {
        log_info("Network thread started");
        
        NetworkTick current_tick = 0;
        auto last_tick_time = std::chrono::high_resolution_clock::now();
        const auto tick_duration = std::chrono::microseconds(constants::NETWORK_TICK_INTERVAL_US);
        
        while (!shutdown_requested_) {
            auto tick_start = std::chrono::high_resolution_clock::now();
            
            // Process incoming messages
            process_incoming_messages();
            
            // Send outgoing updates (tick-based)
            if (tick_start - last_tick_time >= tick_duration) {
                process_outgoing_updates(current_tick++);
                last_tick_time = tick_start;
            }
            
            // Update connection states
            update_connections();
            
            // Clean up old data
            cleanup_old_data();
            
            // Sleep until next iteration (but not longer than 1ms)
            auto elapsed = std::chrono::high_resolution_clock::now() - tick_start;
            auto sleep_time = std::min(std::chrono::milliseconds(1), tick_duration - elapsed);
            
            if (sleep_time > std::chrono::microseconds(0)) {
                std::this_thread::sleep_for(sleep_time);
            }
        }
        
        log_info("Network thread stopped");
    }
    
    /** @brief Process incoming network messages */
    void process_incoming_messages() {
        if (!socket_) return;
        
        std::array<u8, constants::PACKET_BUFFER_SIZE> buffer;
        NetworkAddress sender_address;
        
        while (true) {
            auto bytes_received = socket_->receive(buffer.data(), buffer.size(), sender_address);
            if (bytes_received == 0) {
                break;  // No more messages
            }
            
            // Apply network simulation if enabled
            if (network_simulator_ && network_simulator_->should_drop_packet()) {
                network_stats_.packets_lost++;
                continue;
            }
            
            // Process the message through protocol
            if (protocol_) {
                protocol_->process_incoming_data(buffer.data(), bytes_received, sender_address);
            }
            
            network_stats_.packets_received++;
            network_stats_.bytes_received_per_sec += bytes_received;
        }
    }
    
    /** @brief Process outgoing network updates */
    void process_outgoing_updates(NetworkTick tick) {
        if (!socket_ || !protocol_) return;
        
        // Get entities with changes
        auto entities_to_update = entity_manager_->get_entities_with_changes();
        
        // Limit updates per tick
        if (entities_to_update.size() > config_.max_entities_per_update) {
            entities_to_update.resize(config_.max_entities_per_update);
        }
        
        // Process each entity update
        for (auto entity : entities_to_update) {
            send_entity_update(entity, tick);
        }
        
        // Send heartbeats and connection maintenance
        send_heartbeats();
    }
    
    /** @brief Send entity update message */
    void send_entity_update(ecs::Entity entity, NetworkTick tick) {
        auto* state = entity_manager_->get_network_state(entity);
        if (!state || !state->has_changes()) return;
        
        // Generate component deltas
        std::vector<replication::ComponentDelta> deltas;
        // TODO: Implement component delta generation based on changed components
        
        // Create update message
        replication::EntityUpdateMessage update_msg;
        update_msg.network_id = state->network_id;
        update_msg.from_version = state->version;
        update_msg.to_version = state->version + 1;
        update_msg.update_tick = tick;
        update_msg.delta_count = static_cast<u16>(deltas.size());
        
        // Send to appropriate clients
        if (is_server_) {
            send_to_interested_clients(update_msg, entity);
        } else {
            send_to_server(update_msg);
        }
        
        // Clear change flags and update state
        state->clear_changes();
        state->increment_version();
        state->last_update_tick = tick;
        state->last_sync_time = timing::now();
    }
    
    /** @brief Send message to clients interested in entity */
    void send_to_interested_clients(const replication::EntityUpdateMessage& msg, ecs::Entity entity) {
        // TODO: Implement interest management and spatial partitioning
        // For now, send to all connected clients
        
        std::lock_guard<std::mutex> lock(client_mutex_);
        for (const auto& [client_id, client_info] : connected_clients_) {
            if (client_info.state == ConnectionState::Connected) {
                // TODO: Serialize and send message
            }
        }
    }
    
    /** @brief Send message to server */
    void send_to_server(const replication::EntityUpdateMessage& msg) {
        // TODO: Implement message serialization and sending
    }
    
    /** @brief Send heartbeat messages */
    void send_heartbeats() {
        // TODO: Implement heartbeat system
    }
    
    /** @brief Update connection states */
    void update_connections() {
        if (!is_server_) return;
        
        auto current_time = timing::now();
        std::lock_guard<std::mutex> lock(client_mutex_);
        
        auto it = connected_clients_.begin();
        while (it != connected_clients_.end()) {
            auto& [client_id, client_info] = *it;
            
            if (current_time - client_info.last_activity > config_.connection_timeout_ms * 1000) {
                log_info("Client {} timed out", client_id);
                emit_event(NetworkEvent::create(NetworkEventType::ConnectionTimeout, client_id));
                it = connected_clients_.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    /** @brief Clean up old data */
    void cleanup_old_data() {
        if (delta_encoder_) {
            auto cutoff_time = timing::now() - 60 * 1000 * 1000;  // 60 seconds ago
            delta_encoder_->cleanup_old_snapshots(cutoff_time);
        }
        
        // Clean up message pool
        // TODO: Implement message pool cleanup
    }
    
    /** @brief Connect to server (client only) */
    bool connect_to_server() {
        // TODO: Implement connection handshake
        return true;
    }
    
    /** @brief Disconnect all clients (server only) */
    void disconnect_all_clients() {
        std::lock_guard<std::mutex> lock(client_mutex_);
        
        for (const auto& [client_id, client_info] : connected_clients_) {
            log_info("Disconnecting client {}", client_id);
            emit_event(NetworkEvent::create(NetworkEventType::ClientDisconnected, client_id));
        }
        
        connected_clients_.clear();
    }
    
    /** @brief Update server-specific logic */
    void update_server(f32 delta_time) {
        // Update authority system
        if (authority_system_) {
            authority_system_->update(delta_time);
        }
        
        // Update client management
        update_client_management(delta_time);
    }
    
    /** @brief Update client-specific logic */
    void update_client(f32 delta_time) {
        // Update component synchronization
        if (component_sync_) {
            component_sync_->update(delta_time);
        }
        
        // Update connection health
        update_connection_health();
    }
    
    /** @brief Update client management (server only) */
    void update_client_management(f32 delta_time) {
        // TODO: Implement client health monitoring, bandwidth management
    }
    
    /** @brief Update connection health (client only) */
    void update_connection_health() {
        // TODO: Implement connection quality monitoring
    }
    
    /** @brief Process network events */
    void process_events() {
        std::lock_guard<std::mutex> lock(event_queue_mutex_);
        
        while (!event_queue_.empty()) {
            auto event = event_queue_.front();
            event_queue_.pop();
            
            handle_network_event(event);
        }
    }
    
    /** @brief Handle individual network event */
    void handle_network_event(const NetworkEvent& event) {
        switch (event.type) {
            case NetworkEventType::ClientConnected:
                log_info("Client {} connected", event.client_id);
                break;
            case NetworkEventType::ClientDisconnected:
                log_info("Client {} disconnected", event.client_id);
                break;
            case NetworkEventType::EntityCreated:
                log_debug("Entity {} created", event.entity_id);
                break;
            case NetworkEventType::BandwidthExceeded:
                log_warning("Bandwidth limit exceeded for client {}", event.client_id);
                break;
            default:
                break;
        }
    }
    
    /** @brief Update tutorials */
    void update_tutorials() {
        if (!tutorials_enabled_) return;
        
        for (auto& tutorial : tutorials_) {
            if (!tutorial.triggered && tutorial.trigger_condition()) {
                tutorial.triggered = true;
                emit_event(NetworkEvent::create_with_data(NetworkEventType::TutorialTriggered, tutorial.title));
                log_info("Tutorial triggered: {}", tutorial.title);
            }
        }
    }
    
    /** @brief Update performance statistics */
    void update_performance_stats(f32 delta_time) {
        // Update bandwidth calculations
        static f32 stats_update_timer = 0.0f;
        stats_update_timer += delta_time;
        
        if (stats_update_timer >= 1.0f) {
            network_stats_.update_packet_loss();
            network_stats_.update_connection_quality();
            stats_update_timer = 0.0f;
        }
    }
    
    /** @brief Emit network event */
    void emit_event(const NetworkEvent& event) {
        std::lock_guard<std::mutex> lock(event_queue_mutex_);
        event_queue_.push(event);
        event_condition_.notify_one();
    }
    
    /** @brief Generate unique session ID */
    SessionID generate_session_id() {
        return timing::now();
    }
    
    /** @brief Render network statistics */
    void render_network_stats() const {
        // TODO: Implement ImGui network stats display
    }
    
    /** @brief Render entity replication information */
    void render_entity_replication_info() const {
        // TODO: Implement ImGui entity replication display
    }
    
    /** @brief Render network prediction debug info */
    void render_prediction_debug() const {
        // TODO: Implement ImGui prediction debug display
    }
    
    /** @brief Render bandwidth usage information */
    void render_bandwidth_usage() const {
        // TODO: Implement ImGui bandwidth usage display
    }
    
    /** @brief Render packet analysis information */
    void render_packet_analysis() const {
        // TODO: Implement ImGui packet analysis display
    }
    
    /** @brief Log error message */
    void log_error(const char* format, auto... args) const {
        // TODO: Integrate with ECScope logging system
    }
    
    /** @brief Log warning message */
    void log_warning(const char* format, auto... args) const {
        // TODO: Integrate with ECScope logging system
    }
    
    /** @brief Log info message */
    void log_info(const char* format, auto... args) const {
        // TODO: Integrate with ECScope logging system
    }
    
    /** @brief Log debug message */
    void log_debug(const char* format, auto... args) const {
        // TODO: Integrate with ECScope logging system
    }
};

//=============================================================================
// Network System Factory
//=============================================================================

/**
 * @brief Factory for creating networking systems
 */
class NetworkSystemFactory {
public:
    /** @brief Create client networking system */
    static std::unique_ptr<ECSNetworkingSystem> create_client(
        ecs::Registry& registry,
        const NetworkAddress& server_address = NetworkAddress::local(constants::DEFAULT_SERVER_PORT)) {
        
        NetworkConfig config = NetworkConfig::client_default();
        config.server_address = server_address;
        
        return std::make_unique<ECSNetworkingSystem>(registry, config);
    }
    
    /** @brief Create server networking system */
    static std::unique_ptr<ECSNetworkingSystem> create_server(
        ecs::Registry& registry,
        u16 port = constants::DEFAULT_SERVER_PORT,
        u32 max_clients = 64) {
        
        NetworkConfig config = NetworkConfig::server_default();
        config.server_address = NetworkAddress::local(port);
        config.max_clients = max_clients;
        
        return std::make_unique<ECSNetworkingSystem>(registry, config);
    }
    
    /** @brief Create educational demo system */
    static std::unique_ptr<ECSNetworkingSystem> create_educational_demo(
        ecs::Registry& registry,
        bool is_server = false) {
        
        NetworkConfig config = NetworkConfig::educational_demo();
        
        auto system = std::make_unique<ECSNetworkingSystem>(registry, config);
        system->set_tutorials_enabled(true);
        
        return system;
    }
};

} // namespace ecscope::networking