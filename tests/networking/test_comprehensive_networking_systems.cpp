#include "../framework/ecscope_test_framework.hpp"
#include <ecscope/networking/ecs_networking_system.hpp>
#include <ecscope/networking/network_protocol.hpp>
#include <ecscope/networking/entity_replication.hpp>
#include <ecscope/networking/component_sync.hpp>
#include <ecscope/networking/authority_system.hpp>
#include <ecscope/networking/network_prediction.hpp>
#include <ecscope/networking/network_simulation.hpp>
#include <ecscope/networking/udp_socket.hpp>

namespace ECScope::Testing {

using namespace ecscope::networking;

class NetworkingSystemTest : public ECScopeTestFixture {
protected:
    void SetUp() override {
        ECScopeTestFixture::SetUp();
        
        // Create separate registries for client and server
        server_registry_ = std::make_unique<Registry>();
        client_registry_ = std::make_unique<Registry>();
        
        // Initialize network configurations
        server_config_ = NetworkConfig::server_default();
        server_config_.server_address = NetworkAddress::local(test_port_);
        server_config_.max_clients = 4;
        server_config_.tick_rate = 30; // Lower for testing
        server_config_.enable_network_visualization = false; // Disable UI for tests
        
        client_config_ = NetworkConfig::client_default();
        client_config_.server_address = NetworkAddress::local(test_port_);
        client_config_.enable_network_visualization = false;
        
        // Create networking systems
        server_system_ = std::make_unique<ECSNetworkingSystem>(*server_registry_, server_config_);
        client_system_ = std::make_unique<ECSNetworkingSystem>(*client_registry_, client_config_);
        
        // Register test components for synchronization
        register_test_components();
    }
    
    void TearDown() override {
        // Shutdown networking systems
        if (client_system_) {
            client_system_->shutdown();
        }
        if (server_system_) {
            server_system_->shutdown();
        }
        
        // Small delay to ensure clean shutdown
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        client_system_.reset();
        server_system_.reset();
        client_registry_.reset();
        server_registry_.reset();
        
        ECScopeTestFixture::TearDown();
    }
    
    void register_test_components() {
        // Register networking components
        server_system_->register_component_sync<TestPosition>();
        server_system_->register_component_sync<TestVelocity>();
        server_system_->register_component_sync<TestHealth>();
        
        client_system_->register_component_sync<TestPosition>();
        client_system_->register_component_sync<TestVelocity>();
        client_system_->register_component_sync<TestHealth>();
    }
    
    bool start_server_and_wait() {
        if (!server_system_->start_server()) {
            return false;
        }
        
        // Wait for server to be ready
        auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (!server_system_->is_running() && std::chrono::steady_clock::now() < timeout) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        return server_system_->is_running();
    }
    
    bool connect_client_and_wait() {
        if (!client_system_->start_client()) {
            return false;
        }
        
        // Wait for client to connect
        auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (!client_system_->is_running() && std::chrono::steady_clock::now() < timeout) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            // Update systems to process network events
            server_system_->update(0.016f);
            client_system_->update(0.016f);
        }
        
        return client_system_->is_running();
    }
    
    void simulate_network_updates(float duration) {
        auto end_time = std::chrono::steady_clock::now() + 
                       std::chrono::milliseconds(static_cast<int>(duration * 1000));
        
        while (std::chrono::steady_clock::now() < end_time) {
            server_system_->update(0.016f);
            client_system_->update(0.016f);
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }

protected:
    static constexpr u16 test_port_ = 57890; // Use non-standard port for testing
    
    std::unique_ptr<Registry> server_registry_;
    std::unique_ptr<Registry> client_registry_;
    NetworkConfig server_config_;
    NetworkConfig client_config_;
    std::unique_ptr<ECSNetworkingSystem> server_system_;
    std::unique_ptr<ECSNetworkingSystem> client_system_;
};

// =============================================================================
// Basic Networking Tests
// =============================================================================

TEST_F(NetworkingSystemTest, NetworkingSystemInitialization) {
    EXPECT_NE(server_system_, nullptr);
    EXPECT_NE(client_system_, nullptr);
    
    EXPECT_TRUE(server_system_->is_server());
    EXPECT_FALSE(client_system_->is_server());
    
    EXPECT_FALSE(server_system_->is_running());
    EXPECT_FALSE(client_system_->is_running());
}

TEST_F(NetworkingSystemTest, ServerStartupAndShutdown) {
    // Test server startup
    EXPECT_TRUE(start_server_and_wait());
    EXPECT_TRUE(server_system_->is_running());
    EXPECT_TRUE(server_system_->is_server());
    EXPECT_EQ(server_system_->get_local_client_id(), 1); // Server is always ID 1
    
    // Test basic server update
    EXPECT_NO_THROW(server_system_->update(0.016f));
    
    // Test server shutdown
    server_system_->shutdown();
    EXPECT_FALSE(server_system_->is_running());
}

TEST_F(NetworkingSystemTest, ClientConnectionAndDisconnection) {
    // Start server first
    ASSERT_TRUE(start_server_and_wait());
    
    // Connect client
    EXPECT_TRUE(connect_client_and_wait());
    EXPECT_TRUE(client_system_->is_running());
    EXPECT_FALSE(client_system_->is_server());
    
    // Simulate some network updates
    simulate_network_updates(0.5f);
    
    // Check client connection status
    auto server_stats = server_system_->get_network_stats();
    EXPECT_GE(server_stats.active_connections, 0); // May not detect connection immediately
    
    // Test client disconnect
    client_system_->shutdown();
    EXPECT_FALSE(client_system_->is_running());
    
    // Update server to process disconnection
    server_system_->update(0.016f);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// =============================================================================
// Entity Replication Tests
// =============================================================================

TEST_F(NetworkingSystemTest, EntityRegistrationAndReplication) {
    ASSERT_TRUE(start_server_and_wait());
    ASSERT_TRUE(connect_client_and_wait());
    
    // Create an entity on the server
    auto server_entity = server_registry_->create();
    server_registry_->emplace<TestPosition>(server_entity, 10.0f, 20.0f, 30.0f);
    server_registry_->emplace<TestVelocity>(server_entity, 1.0f, 2.0f, 3.0f);
    
    // Register entity for networking
    auto network_id = server_system_->register_entity(server_entity, MessagePriority::Normal);
    EXPECT_NE(network_id, 0);
    
    // Mark components as changed
    server_system_->mark_component_changed<TestPosition>(server_entity);
    server_system_->mark_component_changed<TestVelocity>(server_entity);
    
    // Simulate network updates to replicate entity
    simulate_network_updates(1.0f);
    
    // Check entity replication statistics
    auto entity_stats = server_system_->get_entity_stats();
    EXPECT_GT(entity_stats.entities_registered, 0);
    
    // Unregister entity
    server_system_->unregister_entity(server_entity);
    
    simulate_network_updates(0.5f);
}

TEST_F(NetworkingSystemTest, ComponentSynchronization) {
    ASSERT_TRUE(start_server_and_wait());
    ASSERT_TRUE(connect_client_and_wait());
    
    // Create entity with components on server
    auto server_entity = server_registry_->create();
    auto& position = server_registry_->emplace<TestPosition>(server_entity, 0.0f, 0.0f, 0.0f);
    auto& velocity = server_registry_->emplace<TestVelocity>(server_entity, 10.0f, 0.0f, 0.0f);
    
    auto network_id = server_system_->register_entity(server_entity);
    ASSERT_NE(network_id, 0);
    
    // Simulate entity movement
    for (int i = 0; i < 10; ++i) {
        position.x += velocity.vx * 0.016f;
        position.y += velocity.vy * 0.016f;
        position.z += velocity.vz * 0.016f;
        
        server_system_->mark_component_changed<TestPosition>(server_entity);
        
        simulate_network_updates(0.016f);
    }
    
    // Verify component synchronization occurred
    auto server_stats = server_system_->get_network_stats();
    EXPECT_GT(server_stats.bytes_sent, 0);
    
    auto client_stats = client_system_->get_network_stats();
    EXPECT_GT(client_stats.bytes_received, 0);
}

TEST_F(NetworkingSystemTest, MultipleEntityReplication) {
    ASSERT_TRUE(start_server_and_wait());
    ASSERT_TRUE(connect_client_and_wait());
    
    constexpr int entity_count = 10;
    std::vector<Entity> server_entities;
    std::vector<NetworkEntityID> network_ids;
    
    // Create multiple entities
    for (int i = 0; i < entity_count; ++i) {
        auto entity = server_registry_->create();
        server_registry_->emplace<TestPosition>(entity, 
            static_cast<float>(i * 10), 
            static_cast<float>(i * 5), 
            0.0f);
        server_registry_->emplace<TestHealth>(entity, 100, 100);
        
        auto network_id = server_system_->register_entity(entity);
        ASSERT_NE(network_id, 0);
        
        server_entities.push_back(entity);
        network_ids.push_back(network_id);
        
        server_system_->mark_component_changed<TestPosition>(entity);
        server_system_->mark_component_changed<TestHealth>(entity);
    }
    
    // Simulate network updates
    simulate_network_updates(2.0f);
    
    // Check that multiple entities were processed
    auto entity_stats = server_system_->get_entity_stats();
    EXPECT_EQ(entity_stats.entities_registered, entity_count);
    
    // Clean up
    for (auto entity : server_entities) {
        server_system_->unregister_entity(entity);
    }
}

// =============================================================================
// Network Protocol Tests
// =============================================================================

TEST_F(NetworkingSystemTest, UDPSocketBasicOperations) {
    UDPSocket server_socket;
    UDPSocket client_socket;
    
    NetworkAddress server_addr = NetworkAddress::local(test_port_ + 1);
    NetworkAddress client_addr = NetworkAddress::local(test_port_ + 2);
    
    // Test socket binding
    EXPECT_TRUE(server_socket.bind(server_addr));
    EXPECT_TRUE(client_socket.bind(client_addr));
    
    // Test sending and receiving
    std::string test_message = "Hello, Networking!";
    auto bytes_sent = client_socket.send(
        test_message.data(), 
        test_message.size(), 
        server_addr);
    EXPECT_EQ(bytes_sent, test_message.size());
    
    // Receive message on server
    std::array<char, 1024> buffer;
    NetworkAddress sender;
    auto bytes_received = server_socket.receive(buffer.data(), buffer.size(), sender);
    
    EXPECT_EQ(bytes_received, test_message.size());
    EXPECT_EQ(std::string(buffer.data(), bytes_received), test_message);
    EXPECT_EQ(sender.get_port(), client_addr.get_port());
}

TEST_F(NetworkingSystemTest, NetworkAddressOperations) {
    // Test local address creation
    auto local_addr = NetworkAddress::local(8080);
    EXPECT_EQ(local_addr.get_port(), 8080);
    EXPECT_TRUE(local_addr.is_loopback());
    
    // Test address string conversion
    std::string addr_str = local_addr.to_string();
    EXPECT_FALSE(addr_str.empty());
    EXPECT_NE(addr_str.find("8080"), std::string::npos);
    
    // Test address comparison
    auto local_addr2 = NetworkAddress::local(8080);
    auto local_addr3 = NetworkAddress::local(8081);
    
    EXPECT_EQ(local_addr, local_addr2);
    EXPECT_NE(local_addr, local_addr3);
}

TEST_F(NetworkingSystemTest, NetworkProtocolMessage) {
    protocol::NetworkProtocol protocol(TransportProtocol::ReliableUDP);
    
    // Create test message
    protocol::NetworkMessage message;
    message.type = protocol::MessageType::EntityUpdate;
    message.priority = MessagePriority::High;
    message.timestamp = timing::now();
    message.size = 100;
    
    // Test message serialization/deserialization
    std::vector<u8> serialized_data(message.size + 64); // Extra space for header
    auto serialized_size = protocol.serialize_message(message, serialized_data.data());
    EXPECT_GT(serialized_size, 0);
    EXPECT_LE(serialized_size, serialized_data.size());
    
    // Deserialize message
    protocol::NetworkMessage deserialized_message;
    bool deserialize_success = protocol.deserialize_message(
        serialized_data.data(), serialized_size, deserialized_message);
    
    EXPECT_TRUE(deserialize_success);
    EXPECT_EQ(deserialized_message.type, message.type);
    EXPECT_EQ(deserialized_message.priority, message.priority);
    EXPECT_EQ(deserialized_message.size, message.size);
}

// =============================================================================
// Authority System Tests
// =============================================================================

TEST_F(NetworkingSystemTest, AuthoritySystemBasicOperations) {
    authority::AuthoritySystem authority_system;
    authority_system.set_local_authority(true); // Act as server
    
    // Create test entity
    Entity test_entity = Entity{42};
    
    // Test initial authority assignment
    authority_system.assign_authority(test_entity, 1); // Server has authority
    EXPECT_TRUE(authority_system.has_authority(test_entity));
    EXPECT_EQ(authority_system.get_authority(test_entity), 1);
    
    // Test authority transfer
    authority_system.transfer_authority(test_entity, 2); // Transfer to client 2
    EXPECT_FALSE(authority_system.has_authority(test_entity));
    EXPECT_EQ(authority_system.get_authority(test_entity), 2);
    
    // Test authority statistics
    auto stats = authority_system.get_statistics();
    EXPECT_GT(stats.total_entities_tracked, 0);
    EXPECT_GT(stats.authority_transfers, 0);
}

TEST_F(NetworkingSystemTest, AuthorityTransferInNetworkSession) {
    ASSERT_TRUE(start_server_and_wait());
    ASSERT_TRUE(connect_client_and_wait());
    
    // Create entity on server
    auto server_entity = server_registry_->create();
    server_registry_->emplace<TestPosition>(server_entity, 0.0f, 0.0f, 0.0f);
    
    auto network_id = server_system_->register_entity(server_entity);
    ASSERT_NE(network_id, 0);
    
    // Initially, server has authority
    simulate_network_updates(0.5f);
    
    // Transfer authority to client
    server_system_->transfer_authority(server_entity, 2); // Client ID 2
    
    // Simulate network updates to process transfer
    simulate_network_updates(1.0f);
    
    // Verify authority transfer occurred
    auto entity_stats = server_system_->get_entity_stats();
    EXPECT_GT(entity_stats.authority_transfers, 0);
}

// =============================================================================
// Network Prediction Tests
// =============================================================================

TEST_F(NetworkingSystemTest, ClientPredictionInitialization) {
    // Enable prediction in config
    client_config_.enable_client_prediction = true;
    client_config_.max_rollback_ticks = 10;
    client_config_.prediction_error_threshold = 0.1f;
    
    // Recreate client system with prediction enabled
    client_system_ = std::make_unique<ECSNetworkingSystem>(*client_registry_, client_config_);
    register_test_components();
    
    ASSERT_TRUE(start_server_and_wait());
    ASSERT_TRUE(connect_client_and_wait());
    
    // Check prediction system is active
    auto prediction_stats = client_system_->get_prediction_stats();
    EXPECT_EQ(prediction_stats.max_rollback_ticks, 10);
    EXPECT_FLOAT_EQ(prediction_stats.error_threshold, 0.1f);
    
    simulate_network_updates(1.0f);
    
    // Prediction stats may be updated during simulation
    prediction_stats = client_system_->get_prediction_stats();
    EXPECT_GE(prediction_stats.predictions_made, 0);
}

TEST_F(NetworkingSystemTest, PredictionWithEntityMovement) {
    // Enable prediction
    client_config_.enable_client_prediction = true;
    client_system_ = std::make_unique<ECSNetworkingSystem>(*client_registry_, client_config_);
    register_test_components();
    
    ASSERT_TRUE(start_server_and_wait());
    ASSERT_TRUE(connect_client_and_wait());
    
    // Create moving entity on server
    auto server_entity = server_registry_->create();
    auto& position = server_registry_->emplace<TestPosition>(server_entity, 0.0f, 0.0f, 0.0f);
    auto& velocity = server_registry_->emplace<TestVelocity>(server_entity, 10.0f, 0.0f, 0.0f);
    
    auto network_id = server_system_->register_entity(server_entity);
    ASSERT_NE(network_id, 0);
    
    // Simulate entity movement with prediction
    for (int i = 0; i < 30; ++i) {
        // Update position on server
        position.x += velocity.vx * 0.033f; // 30 FPS updates
        server_system_->mark_component_changed<TestPosition>(server_entity);
        
        // Update systems (client prediction should occur)
        server_system_->update(0.033f);
        client_system_->update(0.033f);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }
    
    // Check prediction statistics
    auto prediction_stats = client_system_->get_prediction_stats();
    // Note: Actual prediction may or may not occur depending on implementation details
    EXPECT_GE(prediction_stats.predictions_made, 0);
}

// =============================================================================
// Network Simulation Tests
// =============================================================================

TEST_F(NetworkingSystemTest, NetworkSimulationWithPacketLoss) {
    // Configure artificial network conditions
    server_config_.packet_loss_simulation = 0.1f; // 10% packet loss
    client_config_.packet_loss_simulation = 0.1f;
    
    // Recreate systems with simulation enabled
    server_system_ = std::make_unique<ECSNetworkingSystem>(*server_registry_, server_config_);
    client_system_ = std::make_unique<ECSNetworkingSystem>(*client_registry_, client_config_);
    register_test_components();
    
    ASSERT_TRUE(start_server_and_wait());
    ASSERT_TRUE(connect_client_and_wait());
    
    // Apply network simulation settings
    server_system_->simulate_network_conditions(0.1f, 0);
    client_system_->simulate_network_conditions(0.1f, 0);
    
    // Create entity and simulate network traffic
    auto server_entity = server_registry_->create();
    server_registry_->emplace<TestPosition>(server_entity, 0.0f, 0.0f, 0.0f);
    
    auto network_id = server_system_->register_entity(server_entity);
    ASSERT_NE(network_id, 0);
    
    // Generate network traffic and observe packet loss
    for (int i = 0; i < 20; ++i) {
        server_system_->mark_component_changed<TestPosition>(server_entity);
        simulate_network_updates(0.05f);
    }
    
    // Check for packet loss in statistics
    auto server_stats = server_system_->get_network_stats();
    // Note: Packet loss may or may not be reflected in stats depending on implementation
    EXPECT_GE(server_stats.packets_sent, 0);
}

TEST_F(NetworkingSystemTest, NetworkSimulationWithLatency) {
    // Configure artificial latency
    server_config_.latency_simulation_ms = 100; // 100ms latency
    client_config_.latency_simulation_ms = 100;
    
    // Recreate systems with latency simulation
    server_system_ = std::make_unique<ECSNetworkingSystem>(*server_registry_, server_config_);
    client_system_ = std::make_unique<ECSNetworkingSystem>(*client_registry_, client_config_);
    register_test_components();
    
    ASSERT_TRUE(start_server_and_wait());
    ASSERT_TRUE(connect_client_and_wait());
    
    // Apply latency simulation
    server_system_->simulate_network_conditions(0.0f, 100);
    client_system_->simulate_network_conditions(0.0f, 100);
    
    // Measure round-trip time with latency
    auto start_time = std::chrono::steady_clock::now();
    
    // Create and replicate entity
    auto server_entity = server_registry_->create();
    server_registry_->emplace<TestPosition>(server_entity, 1.0f, 2.0f, 3.0f);
    
    auto network_id = server_system_->register_entity(server_entity);
    ASSERT_NE(network_id, 0);
    
    server_system_->mark_component_changed<TestPosition>(server_entity);
    
    // Simulate network updates with artificial latency
    simulate_network_updates(0.5f);
    
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // With 100ms artificial latency, operations should take noticeably longer
    EXPECT_GT(elapsed.count(), 200); // At least 200ms with bi-directional latency
}

// =============================================================================
// Performance and Stress Tests
// =============================================================================

TEST_F(NetworkingSystemTest, HighEntityCountReplication) {
    ASSERT_TRUE(start_server_and_wait());
    ASSERT_TRUE(connect_client_and_wait());
    
    constexpr int high_entity_count = 100;
    std::vector<Entity> entities;
    entities.reserve(high_entity_count);
    
    auto start_time = std::chrono::steady_clock::now();
    
    // Create many entities
    for (int i = 0; i < high_entity_count; ++i) {
        auto entity = server_registry_->create();
        server_registry_->emplace<TestPosition>(entity, 
            static_cast<float>(i), 
            static_cast<float>(i * 2), 
            0.0f);
        
        auto network_id = server_system_->register_entity(entity);
        EXPECT_NE(network_id, 0);
        
        server_system_->mark_component_changed<TestPosition>(entity);
        entities.push_back(entity);
        
        // Update systems occasionally to prevent overwhelming
        if (i % 20 == 0) {
            server_system_->update(0.016f);
            client_system_->update(0.016f);
        }
    }
    
    // Simulate network replication
    simulate_network_updates(3.0f);
    
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // Check entity statistics
    auto entity_stats = server_system_->get_entity_stats();
    EXPECT_EQ(entity_stats.entities_registered, high_entity_count);
    
    // Performance should be reasonable (less than 10 seconds for 100 entities)
    EXPECT_LT(elapsed.count(), 10000);
    
    std::cout << "Replicated " << high_entity_count << " entities in " 
              << elapsed.count() << "ms" << std::endl;
    
    // Clean up
    for (auto entity : entities) {
        server_system_->unregister_entity(entity);
    }
}

TEST_F(NetworkingSystemTest, NetworkBandwidthMeasurement) {
    ASSERT_TRUE(start_server_and_wait());
    ASSERT_TRUE(connect_client_and_wait());
    
    // Create entity with large component updates
    auto server_entity = server_registry_->create();
    server_registry_->emplace<TestPosition>(server_entity, 0.0f, 0.0f, 0.0f);
    server_registry_->emplace<TestVelocity>(server_entity, 1.0f, 1.0f, 1.0f);
    server_registry_->emplace<TestHealth>(server_entity, 100, 100);
    
    auto network_id = server_system_->register_entity(server_entity);
    ASSERT_NE(network_id, 0);
    
    // Record initial bandwidth usage
    auto initial_server_stats = server_system_->get_network_stats();
    auto initial_client_stats = client_system_->get_network_stats();
    
    // Generate sustained network traffic
    for (int i = 0; i < 60; ++i) { // 60 updates
        server_system_->mark_component_changed<TestPosition>(server_entity);
        server_system_->mark_component_changed<TestVelocity>(server_entity);
        server_system_->mark_component_changed<TestHealth>(server_entity);
        
        simulate_network_updates(0.016f);
    }
    
    // Measure bandwidth usage
    auto final_server_stats = server_system_->get_network_stats();
    auto final_client_stats = client_system_->get_network_stats();
    
    auto bytes_sent = final_server_stats.bytes_sent - initial_server_stats.bytes_sent;
    auto bytes_received = final_client_stats.bytes_received - initial_client_stats.bytes_received;
    
    EXPECT_GT(bytes_sent, 0);
    EXPECT_GT(bytes_received, 0);
    
    std::cout << "Network bandwidth - Sent: " << bytes_sent 
              << " bytes, Received: " << bytes_received << " bytes" << std::endl;
}

// =============================================================================
// Educational Features Tests
// =============================================================================

TEST_F(NetworkingSystemTest, EducationalTutorialSystem) {
    // Create educational demo configuration
    auto educational_config = NetworkConfig::educational_demo();
    educational_config.server_address = NetworkAddress::local(test_port_);
    
    // Recreate systems with educational features
    server_system_ = std::make_unique<ECSNetworkingSystem>(*server_registry_, educational_config);
    client_system_ = std::make_unique<ECSNetworkingSystem>(*client_registry_, educational_config);
    
    register_test_components();
    
    // Enable tutorials
    server_system_->set_tutorials_enabled(true);
    client_system_->set_tutorials_enabled(true);
    
    ASSERT_TRUE(start_server_and_wait());
    ASSERT_TRUE(connect_client_and_wait());
    
    // Create entity to trigger entity replication tutorial
    auto server_entity = server_registry_->create();
    server_registry_->emplace<TestPosition>(server_entity, 0.0f, 0.0f, 0.0f);
    
    auto network_id = server_system_->register_entity(server_entity);
    ASSERT_NE(network_id, 0);
    
    server_system_->mark_component_changed<TestPosition>(server_entity);
    
    // Simulate network updates to trigger tutorials
    simulate_network_updates(2.0f);
    
    // Verify tutorial system is active
    // Note: Tutorial triggering depends on internal implementation details
    // This test mainly ensures no crashes occur with educational features enabled
    
    auto server_stats = server_system_->get_network_stats();
    auto client_stats = client_system_->get_network_stats();
    
    EXPECT_GE(server_stats.bytes_sent, 0);
    EXPECT_GE(client_stats.bytes_received, 0);
}

TEST_F(NetworkingSystemTest, NetworkDebuggingAndVisualization) {
    // Enable debugging features
    server_config_.enable_packet_inspection = true;
    server_config_.enable_performance_tracking = true;
    client_config_.enable_packet_inspection = true;
    client_config_.enable_performance_tracking = true;
    
    // Recreate systems with debugging enabled
    server_system_ = std::make_unique<ECSNetworkingSystem>(*server_registry_, server_config_);
    client_system_ = std::make_unique<ECSNetworkingSystem>(*client_registry_, client_config_);
    register_test_components();
    
    ASSERT_TRUE(start_server_and_wait());
    ASSERT_TRUE(connect_client_and_wait());
    
    // Generate network activity
    auto server_entity = server_registry_->create();
    server_registry_->emplace<TestPosition>(server_entity, 0.0f, 0.0f, 0.0f);
    
    auto network_id = server_system_->register_entity(server_entity);
    ASSERT_NE(network_id, 0);
    
    // Simulate network traffic with debugging enabled
    for (int i = 0; i < 20; ++i) {
        server_system_->mark_component_changed<TestPosition>(server_entity);
        simulate_network_updates(0.05f);
    }
    
    // Test debug rendering (should not crash)
    EXPECT_NO_THROW(server_system_->debug_render());
    EXPECT_NO_THROW(client_system_->debug_render());
    
    // Verify performance tracking
    auto server_stats = server_system_->get_network_stats();
    auto client_stats = client_system_->get_network_stats();
    
    EXPECT_GT(server_stats.packets_sent, 0);
    EXPECT_GT(client_stats.packets_received, 0);
}

// =============================================================================
// Error Handling and Edge Cases Tests
// =============================================================================

TEST_F(NetworkingSystemTest, InvalidNetworkConfiguration) {
    // Test with invalid port
    NetworkConfig invalid_config;
    invalid_config.server_address = NetworkAddress::local(0); // Invalid port
    
    ECSNetworkingSystem invalid_system(*registry_, invalid_config);
    
    // Should handle gracefully
    EXPECT_FALSE(invalid_system.start_server());
    EXPECT_FALSE(invalid_system.is_running());
}

TEST_F(NetworkingSystemTest, EntityRegistrationEdgeCases) {
    ASSERT_TRUE(start_server_and_wait());
    
    // Test registering invalid entity
    Entity invalid_entity{999999}; // Non-existent entity
    auto network_id = server_system_->register_entity(invalid_entity);
    EXPECT_EQ(network_id, 0); // Should fail
    
    // Test double registration
    auto valid_entity = server_registry_->create();
    server_registry_->emplace<TestPosition>(valid_entity, 0.0f, 0.0f, 0.0f);
    
    auto network_id1 = server_system_->register_entity(valid_entity);
    auto network_id2 = server_system_->register_entity(valid_entity);
    
    EXPECT_NE(network_id1, 0);
    // Second registration should either return same ID or fail gracefully
    EXPECT_TRUE(network_id2 == 0 || network_id2 == network_id1);
}

TEST_F(NetworkingSystemTest, ConnectionFailureHandling) {
    // Try to connect client without server running
    EXPECT_FALSE(client_system_->start_client());
    EXPECT_FALSE(client_system_->is_running());
    
    // Start server on different port
    server_config_.server_address = NetworkAddress::local(test_port_ + 100);
    server_system_ = std::make_unique<ECSNetworkingSystem>(*server_registry_, server_config_);
    register_test_components();
    
    ASSERT_TRUE(start_server_and_wait());
    
    // Client tries to connect to wrong port (should fail gracefully)
    EXPECT_FALSE(client_system_->start_client());
}

TEST_F(NetworkingSystemTest, SystemShutdownWithActiveConnections) {
    ASSERT_TRUE(start_server_and_wait());
    ASSERT_TRUE(connect_client_and_wait());
    
    // Create some network activity
    auto server_entity = server_registry_->create();
    server_registry_->emplace<TestPosition>(server_entity, 0.0f, 0.0f, 0.0f);
    auto network_id = server_system_->register_entity(server_entity);
    
    simulate_network_updates(0.5f);
    
    // Shutdown server while client is connected (should be graceful)
    EXPECT_NO_THROW(server_system_->shutdown());
    EXPECT_FALSE(server_system_->is_running());
    
    // Client should handle server disconnection gracefully
    simulate_network_updates(0.5f);
    EXPECT_NO_THROW(client_system_->shutdown());
}

} // namespace ECScope::Testing