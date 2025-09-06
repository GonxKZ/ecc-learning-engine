#include "../framework/ecscope_test_framework.hpp"

#ifdef ECSCOPE_ENABLE_NETWORKING
#include <ecscope/networking/network_manager.hpp>
#include <ecscope/networking/packet_serialization.hpp>
#include <ecscope/networking/state_synchronization.hpp>
#include <ecscope/networking/client_prediction.hpp>
#include <ecscope/networking/lag_compensation.hpp>
#include <ecscope/networking/authority_system.hpp>
#include <ecscope/networking/replication_manager.hpp>
#include <ecscope/networking/network_protocols.hpp>
#include <ecscope/networking/compression_system.hpp>
#include <ecscope/networking/interpolation_system.hpp>
#endif

#include <thread>
#include <chrono>
#include <future>

namespace ECScope::Testing {

#ifdef ECSCOPE_ENABLE_NETWORKING
class NetworkingSystemTest : public ECScopeTestFixture {
protected:
    void SetUp() override {
        ECScopeTestFixture::SetUp();
        
        // Initialize networking configuration
        server_config_.port = 12345;
        server_config_.max_clients = 16;
        server_config_.tick_rate = 60;
        server_config_.timeout_seconds = 30.0f;
        server_config_.enable_compression = true;
        server_config_.enable_encryption = false; // Disable for testing
        
        client_config_.server_address = "127.0.0.1";
        client_config_.server_port = 12345;
        client_config_.client_prediction = true;
        client_config_.interpolation = true;
        client_config_.lag_compensation = true;
        
        // Initialize networking systems
        network_manager_ = std::make_unique<Network::NetworkManager>();
        serialization_ = std::make_unique<Network::PacketSerialization>();
        state_sync_ = std::make_unique<Network::StateSynchronization>();
        prediction_ = std::make_unique<Network::ClientPrediction>();
        lag_compensation_ = std::make_unique<Network::LagCompensation>();
        authority_ = std::make_unique<Network::AuthoritySystem>();
        replication_ = std::make_unique<Network::ReplicationManager>();
        compression_ = std::make_unique<Network::CompressionSystem>();
        interpolation_ = std::make_unique<Network::InterpolationSystem>();
        
        // Initialize protocol handlers
        tcp_handler_ = std::make_unique<Network::TCPHandler>();
        udp_handler_ = std::make_unique<Network::UDPHandler>();
        reliable_udp_ = std::make_unique<Network::ReliableUDPHandler>();
    }

    void TearDown() override {
        // Shutdown networking systems
        if (server_running_) {
            stop_server();
        }
        
        for (auto& client : test_clients_) {
            if (client && client->is_connected()) {
                client->disconnect();
            }
        }
        test_clients_.clear();
        
        // Clean up systems
        reliable_udp_.reset();
        udp_handler_.reset();
        tcp_handler_.reset();
        interpolation_.reset();
        compression_.reset();
        replication_.reset();
        authority_.reset();
        lag_compensation_.reset();
        prediction_.reset();
        state_sync_.reset();
        serialization_.reset();
        network_manager_.reset();
        
        ECScopeTestFixture::TearDown();
    }

    // Helper to start test server
    bool start_server() {
        if (server_running_) return true;
        
        server_future_ = std::async(std::launch::async, [this]() {
            return network_manager_->start_server(server_config_);
        });
        
        // Wait for server to start
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        server_running_ = true;
        return true;
    }

    // Helper to stop test server
    void stop_server() {
        if (!server_running_) return;
        
        network_manager_->stop_server();
        
        if (server_future_.valid()) {
            server_future_.wait();
        }
        
        server_running_ = false;
    }

    // Helper to create test client
    std::unique_ptr<Network::NetworkClient> create_test_client() {
        auto client = std::make_unique<Network::NetworkClient>();
        if (client->initialize(client_config_)) {
            test_clients_.push_back(std::move(client));
            return std::unique_ptr<Network::NetworkClient>(test_clients_.back().get());
        }
        return nullptr;
    }

    // Helper to create networked entity with common components
    Entity create_networked_entity(const Vec3& position = Vec3(0, 0, 0), 
                                  Network::NetworkID network_id = 0) {
        auto entity = world_->create_entity();
        
        world_->add_component<Transform3D>(entity, position);
        world_->add_component<TestVelocity>(entity);
        
        Network::NetworkedComponent networked;
        networked.network_id = network_id;
        networked.owner_id = 0; // Server owned by default
        networked.replicate_transform = true;
        networked.replicate_velocity = true;
        networked.update_frequency = 20.0f; // 20 Hz
        
        world_->add_component<Network::NetworkedComponent>(entity, networked);
        
        return entity;
    }

protected:
    Network::ServerConfiguration server_config_;
    Network::ClientConfiguration client_config_;
    
    std::unique_ptr<Network::NetworkManager> network_manager_;
    std::unique_ptr<Network::PacketSerialization> serialization_;
    std::unique_ptr<Network::StateSynchronization> state_sync_;
    std::unique_ptr<Network::ClientPrediction> prediction_;
    std::unique_ptr<Network::LagCompensation> lag_compensation_;
    std::unique_ptr<Network::AuthoritySystem> authority_;
    std::unique_ptr<Network::ReplicationManager> replication_;
    std::unique_ptr<Network::CompressionSystem> compression_;
    std::unique_ptr<Network::InterpolationSystem> interpolation_;
    
    std::unique_ptr<Network::TCPHandler> tcp_handler_;
    std::unique_ptr<Network::UDPHandler> udp_handler_;
    std::unique_ptr<Network::ReliableUDPHandler> reliable_udp_;
    
    std::future<bool> server_future_;
    std::vector<std::unique_ptr<Network::NetworkClient>> test_clients_;
    bool server_running_ = false;
};

// =============================================================================
// Basic Networking Tests
// =============================================================================

TEST_F(NetworkingSystemTest, NetworkManagerInitialization) {
    EXPECT_TRUE(network_manager_->initialize());
    EXPECT_TRUE(network_manager_->is_initialized());
    
    network_manager_->shutdown();
    EXPECT_FALSE(network_manager_->is_initialized());
}

TEST_F(NetworkingSystemTest, ServerStartStop) {
    EXPECT_TRUE(start_server());
    EXPECT_TRUE(network_manager_->is_server_running());
    
    stop_server();
    EXPECT_FALSE(network_manager_->is_server_running());
}

TEST_F(NetworkingSystemTest, ClientConnection) {
    EXPECT_TRUE(start_server());
    
    auto client = create_test_client();
    EXPECT_NE(client, nullptr);
    
    // Connect to server
    bool connected = client->connect();
    EXPECT_TRUE(connected);
    
    // Wait for connection to be established
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_TRUE(client->is_connected());
    EXPECT_GT(client->get_client_id(), 0);
    
    // Verify server sees the client
    auto connected_clients = network_manager_->get_connected_clients();
    EXPECT_EQ(connected_clients.size(), 1);
    EXPECT_EQ(connected_clients[0], client->get_client_id());
}

TEST_F(NetworkingSystemTest, MultipleClientConnections) {
    EXPECT_TRUE(start_server());
    
    constexpr int client_count = 5;
    std::vector<Network::NetworkClient*> clients;
    
    // Connect multiple clients
    for (int i = 0; i < client_count; ++i) {
        auto client = create_test_client();
        EXPECT_NE(client, nullptr);
        EXPECT_TRUE(client->connect());
        clients.push_back(client.get());
    }
    
    // Wait for all connections
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Verify all clients are connected
    for (auto client : clients) {
        EXPECT_TRUE(client->is_connected());
    }
    
    // Verify server sees all clients
    auto connected_clients = network_manager_->get_connected_clients();
    EXPECT_EQ(connected_clients.size(), client_count);
}

// =============================================================================
// Packet Serialization Tests
// =============================================================================

TEST_F(NetworkingSystemTest, BasicPacketSerialization) {
    Network::Packet packet;
    packet.type = Network::PacketType::StateUpdate;
    packet.sequence_number = 12345;
    packet.timestamp = 1000.0;
    
    // Serialize
    auto serialized_data = serialization_->serialize_packet(packet);
    EXPECT_GT(serialized_data.size(), 0);
    
    // Deserialize
    Network::Packet deserialized_packet;
    bool success = serialization_->deserialize_packet(serialized_data, deserialized_packet);
    
    EXPECT_TRUE(success);
    EXPECT_EQ(deserialized_packet.type, packet.type);
    EXPECT_EQ(deserialized_packet.sequence_number, packet.sequence_number);
    EXPECT_DOUBLE_EQ(deserialized_packet.timestamp, packet.timestamp);
}

TEST_F(NetworkingSystemTest, ComponentSerialization) {
    // Create entity with components
    auto entity = create_networked_entity(Vec3(1.5f, 2.5f, 3.5f));
    
    auto& transform = world_->get_component<Transform3D>(entity);
    auto& velocity = world_->get_component<TestVelocity>(entity);
    velocity.vx = 10.0f;
    velocity.vy = 20.0f;
    velocity.vz = 30.0f;
    
    // Serialize entity state
    Network::EntityState entity_state;
    entity_state.entity_id = entity;
    entity_state.network_id = 100;
    
    auto serialized = serialization_->serialize_entity_state(entity_state, *world_);
    EXPECT_GT(serialized.size(), 0);
    
    // Deserialize into new world
    World test_world;
    auto deserialized_entity = test_world.create_entity();
    
    bool success = serialization_->deserialize_entity_state(
        serialized, deserialized_entity, test_world);
    
    EXPECT_TRUE(success);
    
    // Verify components were properly deserialized
    EXPECT_TRUE(test_world.has_component<Transform3D>(deserialized_entity));
    EXPECT_TRUE(test_world.has_component<TestVelocity>(deserialized_entity));
    
    auto& deserialized_transform = test_world.get_component<Transform3D>(deserialized_entity);
    auto& deserialized_velocity = test_world.get_component<TestVelocity>(deserialized_entity);
    
    EXPECT_FLOAT_EQ(deserialized_transform.position.x, transform.position.x);
    EXPECT_FLOAT_EQ(deserialized_transform.position.y, transform.position.y);
    EXPECT_FLOAT_EQ(deserialized_transform.position.z, transform.position.z);
    
    EXPECT_FLOAT_EQ(deserialized_velocity.vx, velocity.vx);
    EXPECT_FLOAT_EQ(deserialized_velocity.vy, velocity.vy);
    EXPECT_FLOAT_EQ(deserialized_velocity.vz, velocity.vz);
}

TEST_F(NetworkingSystemTest, CompressionSerialization) {
    // Create large packet with redundant data
    Network::Packet large_packet;
    large_packet.type = Network::PacketType::StateUpdate;
    large_packet.data.resize(1024, 0x42); // Fill with same byte
    
    // Serialize without compression
    auto uncompressed = serialization_->serialize_packet(large_packet);
    
    // Serialize with compression
    compression_->set_compression_level(6); // Medium compression
    auto compressed = compression_->compress_packet_data(uncompressed);
    
    EXPECT_LT(compressed.size(), uncompressed.size()); // Should be smaller
    
    // Decompress and verify
    auto decompressed = compression_->decompress_packet_data(compressed);
    EXPECT_EQ(decompressed.size(), uncompressed.size());
    EXPECT_EQ(decompressed, uncompressed);
}

// =============================================================================
// State Synchronization Tests
// =============================================================================

TEST_F(NetworkingSystemTest, StateSynchronizationBasics) {
    // Create entities in server world
    auto entity1 = create_networked_entity(Vec3(1, 2, 3), 1001);
    auto entity2 = create_networked_entity(Vec3(4, 5, 6), 1002);
    
    // Initialize state synchronization
    state_sync_->initialize(*world_);
    state_sync_->set_update_frequency(20.0f); // 20 Hz
    
    // Create snapshot
    auto snapshot = state_sync_->create_snapshot(1.0); // timestamp = 1.0
    EXPECT_NE(snapshot, nullptr);
    EXPECT_EQ(snapshot->entity_count, 2);
    
    // Apply snapshot to client world
    World client_world;
    state_sync_->apply_snapshot(*snapshot, client_world);
    
    // Verify entities were created and synchronized
    client_world.each<Network::NetworkedComponent>([&](Entity entity, Network::NetworkedComponent& networked) {
        EXPECT_TRUE(client_world.has_component<Transform3D>(entity));
        
        if (networked.network_id == 1001) {
            auto& transform = client_world.get_component<Transform3D>(entity);
            EXPECT_FLOAT_EQ(transform.position.x, 1.0f);
            EXPECT_FLOAT_EQ(transform.position.y, 2.0f);
            EXPECT_FLOAT_EQ(transform.position.z, 3.0f);
        }
    });
}

TEST_F(NetworkingSystemTest, DeltaCompression) {
    // Create entity
    auto entity = create_networked_entity(Vec3(0, 0, 0), 2001);
    
    state_sync_->initialize(*world_);
    
    // Create baseline snapshot
    auto baseline = state_sync_->create_snapshot(1.0);
    
    // Modify entity state
    auto& transform = world_->get_component<Transform3D>(entity);
    transform.position = Vec3(10, 20, 30);
    
    // Create delta snapshot
    auto delta = state_sync_->create_delta_snapshot(2.0, baseline.get());
    
    EXPECT_NE(delta, nullptr);
    EXPECT_LE(delta->compressed_size, baseline->compressed_size); // Delta should be smaller
    
    // Apply delta to client world
    World client_world;
    state_sync_->apply_snapshot(*baseline, client_world);
    state_sync_->apply_delta_snapshot(*delta, client_world);
    
    // Verify delta was applied correctly
    client_world.each<Network::NetworkedComponent>([&](Entity client_entity, Network::NetworkedComponent& networked) {
        if (networked.network_id == 2001) {
            auto& client_transform = client_world.get_component<Transform3D>(client_entity);
            EXPECT_FLOAT_EQ(client_transform.position.x, 10.0f);
            EXPECT_FLOAT_EQ(client_transform.position.y, 20.0f);
            EXPECT_FLOAT_EQ(client_transform.position.z, 30.0f);
        }
    });
}

// =============================================================================
// Client Prediction Tests
// =============================================================================

TEST_F(NetworkingSystemTest, ClientPredictionBasics) {
    // Create player entity
    auto player_entity = create_networked_entity(Vec3(0, 0, 0), 3001);
    
    // Initialize client prediction
    prediction_->initialize(*world_);
    prediction_->set_prediction_time(0.1f); // 100ms prediction
    
    // Simulate client input
    Network::InputCommand input;
    input.sequence_number = 1;
    input.timestamp = 1.0;
    input.move_direction = Vec3(1, 0, 0);
    input.move_speed = 5.0f;
    
    // Apply prediction
    prediction_->apply_input(player_entity, input);
    
    // Entity should have moved based on prediction
    auto& transform = world_->get_component<Transform3D>(player_entity);
    EXPECT_GT(transform.position.x, 0.0f);
    
    // Store predicted state
    auto predicted_position = transform.position;
    
    // Simulate server correction (slightly different position)
    Vec3 authoritative_position = predicted_position;
    authoritative_position.x -= 0.1f; // Small correction
    
    Network::ServerCorrection correction;
    correction.entity_id = player_entity;
    correction.sequence_number = 1;
    correction.authoritative_position = authoritative_position;
    correction.timestamp = 1.1;
    
    prediction_->apply_server_correction(correction);
    
    // Position should be corrected
    EXPECT_FLOAT_EQ(transform.position.x, authoritative_position.x);
}

TEST_F(NetworkingSystemTest, PredictionRollback) {
    auto player_entity = create_networked_entity(Vec3(0, 0, 0), 3002);
    
    prediction_->initialize(*world_);
    prediction_->set_max_rollback_frames(10);
    
    // Apply series of inputs
    std::vector<Network::InputCommand> inputs;
    for (int i = 0; i < 5; ++i) {
        Network::InputCommand input;
        input.sequence_number = i + 1;
        input.timestamp = 1.0 + i * 0.1;
        input.move_direction = Vec3(1, 0, 0);
        input.move_speed = 1.0f;
        
        prediction_->apply_input(player_entity, input);
        inputs.push_back(input);
    }
    
    auto& transform = world_->get_component<Transform3D>(player_entity);
    Vec3 predicted_final = transform.position;
    
    // Server correction for earlier frame
    Network::ServerCorrection correction;
    correction.entity_id = player_entity;
    correction.sequence_number = 2;
    correction.authoritative_position = Vec3(1.5f, 0, 0); // Different from predicted
    correction.timestamp = 1.1;
    
    prediction_->apply_server_correction(correction);
    
    // System should rollback and re-predict from correction point
    Vec3 corrected_final = transform.position;
    EXPECT_NE(corrected_final.x, predicted_final.x);
    EXPECT_GT(corrected_final.x, 1.5f); // Should include re-predicted moves after correction
}

// =============================================================================
// Lag Compensation Tests
// =============================================================================

TEST_F(NetworkingSystemTest, LagCompensationBasics) {
    // Create entities
    auto shooter = create_networked_entity(Vec3(0, 0, 0), 4001);
    auto target = create_networked_entity(Vec3(5, 0, 0), 4002);
    
    lag_compensation_->initialize(*world_);
    lag_compensation_->set_compensation_time(0.2f); // 200ms compensation
    
    // Record states over time
    for (int i = 0; i < 20; ++i) {
        double timestamp = 1.0 + i * 0.01; // 10ms intervals
        
        // Move target
        auto& target_transform = world_->get_component<Transform3D>(target);
        target_transform.position.x += 0.1f;
        
        lag_compensation_->record_state(timestamp);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    // Simulate shot with lag compensation
    Network::HitscanQuery query;
    query.origin = Vec3(0, 0, 0);
    query.direction = Vec3(1, 0, 0);
    query.range = 100.0f;
    query.client_timestamp = 1.1; // 100ms ago
    query.client_ping = 0.1; // 100ms ping
    
    auto hit_result = lag_compensation_->perform_hitscan(query);
    
    // Should hit target at compensated position
    EXPECT_TRUE(hit_result.hit);
    EXPECT_EQ(hit_result.entity_hit, target);
    
    // Hit position should be at earlier target location
    EXPECT_LT(hit_result.hit_position.x, 6.0f); // Less than current position
}

TEST_F(NetworkingSystemTest, RewindSystem) {
    auto moving_entity = create_networked_entity(Vec3(0, 0, 0), 4003);
    
    lag_compensation_->initialize(*world_);
    
    // Record movement over time
    std::vector<Vec3> positions;
    for (int i = 0; i < 10; ++i) {
        double timestamp = 1.0 + i * 0.1;
        
        auto& transform = world_->get_component<Transform3D>(moving_entity);
        transform.position.x = static_cast<float>(i);
        positions.push_back(transform.position);
        
        lag_compensation_->record_state(timestamp);
    }
    
    Vec3 current_position = world_->get_component<Transform3D>(moving_entity).position;
    
    // Rewind to earlier time
    double rewind_time = 1.5; // 500ms ago
    lag_compensation_->rewind_to_time(rewind_time);
    
    // Position should be rewound
    auto& rewound_transform = world_->get_component<Transform3D>(moving_entity);
    EXPECT_LT(rewound_transform.position.x, current_position.x);
    EXPECT_NEAR(rewound_transform.position.x, 5.0f, 0.5f); // Should be around position 5
    
    // Restore current state
    lag_compensation_->restore_current_state();
    
    // Should be back to current position
    EXPECT_FLOAT_EQ(rewound_transform.position.x, current_position.x);
}

// =============================================================================
// Protocol Tests
// =============================================================================

TEST_F(NetworkingSystemTest, TCPReliableTransmission) {
    EXPECT_TRUE(start_server());
    
    auto client = create_test_client();
    EXPECT_TRUE(client->connect());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Send large reliable message
    std::vector<uint8_t> large_message(10000, 0xAB);
    
    bool sent = tcp_handler_->send_reliable(client->get_client_id(), large_message);
    EXPECT_TRUE(sent);
    
    // Wait for transmission
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Verify message was received intact
    auto received_messages = client->get_received_messages();
    EXPECT_GT(received_messages.size(), 0);
    
    bool found_message = false;
    for (const auto& msg : received_messages) {
        if (msg.size() == large_message.size() && 
            std::equal(msg.begin(), msg.end(), large_message.begin())) {
            found_message = true;
            break;
        }
    }
    
    EXPECT_TRUE(found_message);
}

TEST_F(NetworkingSystemTest, UDPUnreliableTransmission) {
    EXPECT_TRUE(start_server());
    
    auto client = create_test_client();
    EXPECT_TRUE(client->connect());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Send many small unreliable messages
    constexpr int message_count = 100;
    int messages_sent = 0;
    
    for (int i = 0; i < message_count; ++i) {
        std::vector<uint8_t> message = {static_cast<uint8_t>(i), 0xFF, 0xEE};
        
        if (udp_handler_->send_unreliable(client->get_client_id(), message)) {
            messages_sent++;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Some messages may be lost (that's expected with UDP)
    auto received_messages = client->get_received_unreliable_messages();
    
    // Should receive most messages, but some loss is acceptable
    EXPECT_GT(received_messages.size(), message_count * 0.8f); // At least 80%
    EXPECT_LE(received_messages.size(), messages_sent); // Can't receive more than sent
}

TEST_F(NetworkingSystemTest, ReliableUDPHybrid) {
    EXPECT_TRUE(start_server());
    
    auto client = create_test_client();
    EXPECT_TRUE(client->connect());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Send reliable UDP messages
    constexpr int message_count = 50;
    std::vector<uint32_t> message_ids;
    
    for (int i = 0; i < message_count; ++i) {
        std::vector<uint8_t> message = {static_cast<uint8_t>(i), 0xCC, 0xDD};
        
        auto message_id = reliable_udp_->send_reliable_udp(client->get_client_id(), message);
        if (message_id != Network::INVALID_MESSAGE_ID) {
            message_ids.push_back(message_id);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Wait for all messages to be acknowledged
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Check acknowledgment status
    int acknowledged_count = 0;
    for (auto msg_id : message_ids) {
        if (reliable_udp_->is_message_acknowledged(msg_id)) {
            acknowledged_count++;
        }
    }
    
    // All messages should be acknowledged (reliable delivery)
    EXPECT_EQ(acknowledged_count, message_ids.size());
}

// =============================================================================
// Performance Tests
// =============================================================================

TEST_F(NetworkingSystemTest, SerializationPerformance) {
    constexpr int entity_count = 1000;
    
    // Create many entities
    std::vector<Entity> entities;
    for (int i = 0; i < entity_count; ++i) {
        auto entity = create_networked_entity(
            Vec3(static_cast<float>(i), static_cast<float>(i * 2), static_cast<float>(i * 3)),
            5000 + i
        );
        entities.push_back(entity);
    }
    
    state_sync_->initialize(*world_);
    
    // Benchmark snapshot creation
    benchmark("SnapshotCreation", [&]() {
        auto snapshot = state_sync_->create_snapshot(2.0);
    }, 100);
    
    auto baseline_snapshot = state_sync_->create_snapshot(2.0);
    
    // Modify some entities
    for (int i = 0; i < entity_count; i += 10) {
        auto& transform = world_->get_component<Transform3D>(entities[i]);
        transform.position.x += 1.0f;
    }
    
    // Benchmark delta snapshot creation
    benchmark("DeltaSnapshotCreation", [&]() {
        auto delta = state_sync_->create_delta_snapshot(3.0, baseline_snapshot.get());
    }, 100);
}

TEST_F(NetworkingSystemTest, NetworkThroughputTest) {
    EXPECT_TRUE(start_server());
    
    auto client = create_test_client();
    EXPECT_TRUE(client->connect());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    constexpr int message_count = 1000;
    constexpr size_t message_size = 1024; // 1KB messages
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Send many messages
    for (int i = 0; i < message_count; ++i) {
        std::vector<uint8_t> message(message_size, static_cast<uint8_t>(i % 256));
        udp_handler_->send_unreliable(client->get_client_id(), message);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    double throughput_mbps = (message_count * message_size * 8.0) / (duration.count()); // Megabits per second
    
    std::cout << "Network throughput: " << throughput_mbps << " Mbps\n";
    
    // Should achieve reasonable throughput
    EXPECT_GT(throughput_mbps, 10.0); // At least 10 Mbps
}

TEST_F(NetworkingSystemTest, LatencyMeasurement) {
    EXPECT_TRUE(start_server());
    
    auto client = create_test_client();
    EXPECT_TRUE(client->connect());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    constexpr int ping_count = 100;
    std::vector<double> latencies;
    
    for (int i = 0; i < ping_count; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        
        // Send ping
        std::vector<uint8_t> ping_message = {0xFF, 0xFE, static_cast<uint8_t>(i)};
        tcp_handler_->send_reliable(client->get_client_id(), ping_message);
        
        // Wait for response
        bool received_response = false;
        auto timeout = start + std::chrono::milliseconds(1000);
        
        while (std::chrono::high_resolution_clock::now() < timeout && !received_response) {
            auto responses = client->get_received_messages();
            for (const auto& response : responses) {
                if (response.size() >= 3 && response[0] == 0xFF && response[1] == 0xFE && response[2] == i) {
                    auto end = std::chrono::high_resolution_clock::now();
                    auto latency = std::chrono::duration<double, std::milli>(end - start).count();
                    latencies.push_back(latency);
                    received_response = true;
                    break;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        if (received_response) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    
    EXPECT_GT(latencies.size(), ping_count * 0.8); // At least 80% success rate
    
    if (!latencies.empty()) {
        double avg_latency = std::accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
        double min_latency = *std::min_element(latencies.begin(), latencies.end());
        double max_latency = *std::max_element(latencies.begin(), latencies.end());
        
        std::cout << "Latency - Avg: " << avg_latency << "ms, Min: " << min_latency << "ms, Max: " << max_latency << "ms\n";
        
        // Loopback should have very low latency
        EXPECT_LT(avg_latency, 10.0); // Less than 10ms average
        EXPECT_LT(min_latency, 5.0);  // Less than 5ms minimum
    }
}

} // namespace ECScope::Testing

#else
// Dummy test when networking is not enabled
namespace ECScope::Testing {

class NetworkingSystemTest : public ECScopeTestFixture {};

TEST_F(NetworkingSystemTest, NetworkingNotEnabled) {
    GTEST_SKIP() << "Networking system not enabled in build configuration";
}

} // namespace ECScope::Testing

#endif // ECSCOPE_ENABLE_NETWORKING