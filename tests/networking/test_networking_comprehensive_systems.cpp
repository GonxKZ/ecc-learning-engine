#include "../framework/ecscope_test_framework.hpp"
#include <ecscope/networking/ecs_networking_system.hpp>
#include <ecscope/networking/network_protocol.hpp>
#include <ecscope/networking/entity_replication.hpp>
#include <ecscope/networking/network_prediction.hpp>
#include <ecscope/networking/component_sync.hpp>
#include <ecscope/networking/authority_system.hpp>
#include <ecscope/networking/udp_socket.hpp>
#include <ecscope/networking/network_simulation.hpp>

#include <thread>
#include <atomic>
#include <chrono>
#include <random>

namespace ECScope::Testing {

// =============================================================================
// Networking Test Fixture
// =============================================================================

class NetworkingSystemTest : public ECScopeTestFixture {
protected:
    void SetUp() override {
        ECScopeTestFixture::SetUp();
        
        // Initialize networking systems
        networking_system_ = std::make_unique<Networking::ECSNetworkingSystem>();
        protocol_ = std::make_unique<Networking::Protocol>();
        replication_ = std::make_unique<Networking::EntityReplication>();
        prediction_ = std::make_unique<Networking::NetworkPrediction>();
        sync_system_ = std::make_unique<Networking::ComponentSync>();
        authority_ = std::make_unique<Networking::AuthoritySystem>();
        
        // Set up test parameters
        server_port_ = 7777;
        client_port_ = 7778;
        tick_rate_ = 60; // 60 Hz
        max_clients_ = 32;
        
        // Initialize network simulation parameters
        packet_loss_rate_ = 0.0f;
        latency_ms_ = 50;
        jitter_ms_ = 10;
        
        // Create test entity data
        create_test_entities();
    }

    void TearDown() override {
        authority_.reset();
        sync_system_.reset();
        prediction_.reset();
        replication_.reset();
        protocol_.reset();
        networking_system_.reset();
        ECScopeTestFixture::TearDown();
    }

    void create_test_entities() {
        // Create networked entities for testing
        for (int i = 0; i < 10; ++i) {
            auto entity = world_->create_entity();
            world_->add_component<TestPosition>(entity, 
                static_cast<float>(i), static_cast<float>(i * 2), 0.0f);
            world_->add_component<TestVelocity>(entity, 1.0f, 0.5f, 0.0f);
            
            // Add networking components
            Networking::NetworkId net_id{static_cast<uint32_t>(i + 1)};
            world_->add_component<Networking::NetworkComponent>(entity, net_id);
            
            test_entities_.push_back(entity);
        }
    }

protected:
    std::unique_ptr<Networking::ECSNetworkingSystem> networking_system_;
    std::unique_ptr<Networking::Protocol> protocol_;
    std::unique_ptr<Networking::EntityReplication> replication_;
    std::unique_ptr<Networking::NetworkPrediction> prediction_;
    std::unique_ptr<Networking::ComponentSync> sync_system_;
    std::unique_ptr<Networking::AuthoritySystem> authority_;
    
    uint16_t server_port_;
    uint16_t client_port_;
    uint32_t tick_rate_;
    uint32_t max_clients_;
    
    float packet_loss_rate_;
    uint32_t latency_ms_;
    uint32_t jitter_ms_;
    
    std::vector<Entity> test_entities_;
};

// =============================================================================
// Basic Networking Protocol Tests
// =============================================================================

TEST_F(NetworkingSystemTest, ProtocolPacketSerialization) {
    // Create test packet
    Networking::Packet packet;
    packet.header.type = Networking::PacketType::EntityUpdate;
    packet.header.sequence = 12345;
    packet.header.timestamp = 67890;
    packet.header.size = 128;
    
    // Add test data
    std::string test_data = "Hello, Network!";
    packet.data.resize(test_data.size());
    std::memcpy(packet.data.data(), test_data.c_str(), test_data.size());
    
    // Serialize packet
    std::vector<uint8_t> serialized;
    bool serialize_success = protocol_->serialize_packet(packet, serialized);
    EXPECT_TRUE(serialize_success);
    EXPECT_GT(serialized.size(), 0);
    
    // Deserialize packet
    Networking::Packet deserialized;
    bool deserialize_success = protocol_->deserialize_packet(serialized, deserialized);
    EXPECT_TRUE(deserialize_success);
    
    // Verify packet contents
    EXPECT_EQ(deserialized.header.type, packet.header.type);
    EXPECT_EQ(deserialized.header.sequence, packet.header.sequence);
    EXPECT_EQ(deserialized.header.timestamp, packet.header.timestamp);
    EXPECT_EQ(deserialized.data.size(), packet.data.size());
    
    std::string recovered_data(deserialized.data.begin(), deserialized.data.end());
    EXPECT_EQ(recovered_data, test_data);
}

TEST_F(NetworkingSystemTest, ProtocolMessageTypes) {
    // Test different message types
    std::vector<Networking::PacketType> types = {
        Networking::PacketType::Connect,
        Networking::PacketType::Disconnect,
        Networking::PacketType::EntityUpdate,
        Networking::PacketType::ComponentSync,
        Networking::PacketType::Input,
        Networking::PacketType::Heartbeat
    };
    
    for (auto type : types) {
        Networking::Packet packet;
        packet.header.type = type;
        packet.header.sequence = 1;
        packet.header.timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        
        std::vector<uint8_t> serialized;
        EXPECT_TRUE(protocol_->serialize_packet(packet, serialized));
        
        Networking::Packet deserialized;
        EXPECT_TRUE(protocol_->deserialize_packet(serialized, deserialized));
        EXPECT_EQ(deserialized.header.type, type);
    }
}

TEST_F(NetworkingSystemTest, ProtocolSequenceNumbers) {
    // Test sequence number handling
    constexpr uint32_t packet_count = 1000;
    std::vector<uint32_t> sent_sequences;
    std::vector<uint32_t> received_sequences;
    
    for (uint32_t i = 0; i < packet_count; ++i) {
        Networking::Packet packet;
        packet.header.type = Networking::PacketType::EntityUpdate;
        packet.header.sequence = i;
        packet.header.timestamp = i * 1000; // Mock timestamp
        
        sent_sequences.push_back(i);
        
        std::vector<uint8_t> serialized;
        EXPECT_TRUE(protocol_->serialize_packet(packet, serialized));
        
        Networking::Packet received;
        EXPECT_TRUE(protocol_->deserialize_packet(serialized, received));
        received_sequences.push_back(received.header.sequence);
    }
    
    // Verify all sequences were preserved
    EXPECT_EQ(sent_sequences.size(), received_sequences.size());
    for (size_t i = 0; i < sent_sequences.size(); ++i) {
        EXPECT_EQ(sent_sequences[i], received_sequences[i]);
    }
}

// =============================================================================
// Entity Replication Tests
// =============================================================================

TEST_F(NetworkingSystemTest, EntityReplicationBasics) {
    auto entity = test_entities_[0];
    
    // Mark entity for replication
    replication_->mark_for_replication(entity);
    EXPECT_TRUE(replication_->is_replicated(entity));
    
    // Test replication data generation
    auto replication_data = replication_->create_replication_data(entity, *world_);
    EXPECT_GT(replication_data.size(), 0);
    
    // Verify that entity data is included
    bool has_position_data = false;
    bool has_velocity_data = false;
    
    // Parse replication data (simplified check)
    if (replication_data.size() >= sizeof(TestPosition)) {
        has_position_data = true;
    }
    if (replication_data.size() >= sizeof(TestPosition) + sizeof(TestVelocity)) {
        has_velocity_data = true;
    }
    
    EXPECT_TRUE(has_position_data);
    EXPECT_TRUE(has_velocity_data);
}

TEST_F(NetworkingSystemTest, EntityReplicationDelta) {
    auto entity = test_entities_[0];
    replication_->mark_for_replication(entity);
    
    // Create initial snapshot
    auto initial_data = replication_->create_replication_data(entity, *world_);
    replication_->store_snapshot(entity, 1, initial_data);
    
    // Modify entity
    auto& pos = world_->get_component<TestPosition>(entity);
    pos.x = 100.0f;
    pos.y = 200.0f;
    
    // Create delta
    auto delta_data = replication_->create_delta_data(entity, *world_, 1);
    
    // Delta should be different from initial
    EXPECT_NE(delta_data.size(), initial_data.size());
    EXPECT_GT(delta_data.size(), 0);
}

TEST_F(NetworkingSystemTest, EntityReplicationPriority) {
    // Create multiple entities with different priorities
    std::vector<Entity> priority_entities;
    std::vector<float> priorities = {1.0f, 0.5f, 0.1f, 0.01f};
    
    for (size_t i = 0; i < priorities.size(); ++i) {
        auto entity = world_->create_entity();
        world_->add_component<TestPosition>(entity, static_cast<float>(i), 0, 0);
        world_->add_component<Networking::NetworkComponent>(entity, 
            Networking::NetworkId{static_cast<uint32_t>(i + 100)});
        
        replication_->mark_for_replication(entity);
        replication_->set_replication_priority(entity, priorities[i]);
        
        priority_entities.push_back(entity);
    }
    
    // Get prioritized update list
    auto prioritized_updates = replication_->get_prioritized_updates(1000); // 1KB budget
    
    // Higher priority entities should come first
    EXPECT_GT(prioritized_updates.size(), 0);
    
    // Verify ordering (this is a simplified test - actual implementation may be more complex)
    if (prioritized_updates.size() >= 2) {
        float first_priority = replication_->get_replication_priority(prioritized_updates[0]);
        float second_priority = replication_->get_replication_priority(prioritized_updates[1]);
        EXPECT_GE(first_priority, second_priority);
    }
}

// =============================================================================
// Network Prediction Tests
// =============================================================================

TEST_F(NetworkingSystemTest, ClientSidePrediction) {
    auto entity = test_entities_[0];
    
    // Set up prediction
    prediction_->enable_prediction_for_entity(entity);
    
    // Record current state
    auto& pos = world_->get_component<TestPosition>(entity);
    auto& vel = world_->get_component<TestVelocity>(entity);
    
    Vec3 initial_pos = {pos.x, pos.y, pos.z};
    Vec3 velocity = {vel.vx, vel.vy, vel.vz};
    
    // Run prediction for several frames
    float dt = 1.0f / 60.0f; // 60 FPS
    int prediction_frames = 5;
    
    for (int frame = 0; frame < prediction_frames; ++frame) {
        prediction_->predict_entity(entity, dt);
    }
    
    // Check predicted position
    Vec3 predicted_pos = {pos.x, pos.y, pos.z};
    Vec3 expected_pos = {
        initial_pos.x + velocity.x * dt * prediction_frames,
        initial_pos.y + velocity.y * dt * prediction_frames,
        initial_pos.z + velocity.z * dt * prediction_frames
    };
    
    EXPECT_NEAR(predicted_pos.x, expected_pos.x, 1e-4f);
    EXPECT_NEAR(predicted_pos.y, expected_pos.y, 1e-4f);
    EXPECT_NEAR(predicted_pos.z, expected_pos.z, 1e-4f);
}

TEST_F(NetworkingSystemTest, ServerReconciliation) {
    auto entity = test_entities_[0];
    prediction_->enable_prediction_for_entity(entity);
    
    // Store initial state
    auto& pos = world_->get_component<TestPosition>(entity);
    TestPosition initial_state = pos;
    
    // Run client prediction
    float dt = 1.0f / 60.0f;
    for (int i = 0; i < 10; ++i) {
        prediction_->predict_entity(entity, dt);
    }
    
    TestPosition predicted_state = pos;
    
    // Simulate server correction (slightly different)
    TestPosition server_state = initial_state;
    server_state.x = initial_state.x + 5.0f; // Server has different result
    server_state.y = initial_state.y + 2.5f;
    
    // Apply server reconciliation
    prediction_->apply_server_correction(entity, server_state, 10); // Frame 10
    
    // Position should be corrected toward server state
    TestPosition corrected_state = world_->get_component<TestPosition>(entity);
    
    // Should be closer to server state than original prediction
    float pred_distance = std::sqrt(
        (predicted_state.x - server_state.x) * (predicted_state.x - server_state.x) +
        (predicted_state.y - server_state.y) * (predicted_state.y - server_state.y)
    );
    
    float corrected_distance = std::sqrt(
        (corrected_state.x - server_state.x) * (corrected_state.x - server_state.x) +
        (corrected_state.y - server_state.y) * (corrected_state.y - server_state.y)
    );
    
    EXPECT_LT(corrected_distance, pred_distance);
}

// =============================================================================
// Component Synchronization Tests
// =============================================================================

TEST_F(NetworkingSystemTest, ComponentSyncConfiguration) {
    // Register components for synchronization
    sync_system_->register_component<TestPosition>();
    sync_system_->register_component<TestVelocity>();
    
    EXPECT_TRUE(sync_system_->is_component_registered<TestPosition>());
    EXPECT_TRUE(sync_system_->is_component_registered<TestVelocity>());
    EXPECT_FALSE(sync_system_->is_component_registered<TestHealth>());
}

TEST_F(NetworkingSystemTest, ComponentSyncFrequency) {
    auto entity = test_entities_[0];
    
    // Set different sync frequencies for different components
    sync_system_->set_sync_frequency<TestPosition>(20); // 20 Hz
    sync_system_->set_sync_frequency<TestVelocity>(10); // 10 Hz
    
    // Simulate time progression and check sync intervals
    uint32_t current_frame = 0;
    uint32_t position_syncs = 0;
    uint32_t velocity_syncs = 0;
    
    for (int frame = 0; frame < 60; ++frame) { // 1 second at 60 FPS
        current_frame++;
        
        if (sync_system_->should_sync_component<TestPosition>(entity, current_frame)) {
            position_syncs++;
        }
        
        if (sync_system_->should_sync_component<TestVelocity>(entity, current_frame)) {
            velocity_syncs++;
        }
    }
    
    // Position should sync ~20 times, velocity ~10 times
    EXPECT_NEAR(position_syncs, 20, 2); // Allow some tolerance
    EXPECT_NEAR(velocity_syncs, 10, 1);
    EXPECT_GT(position_syncs, velocity_syncs); // Position syncs more frequently
}

TEST_F(NetworkingSystemTest, ComponentDeltaCompression) {
    auto entity = test_entities_[0];
    sync_system_->register_component<TestPosition>();
    
    // Create initial state
    auto& pos = world_->get_component<TestPosition>(entity);
    TestPosition initial_state = pos;
    
    // Create baseline
    auto baseline_data = sync_system_->create_baseline<TestPosition>(entity);
    EXPECT_GT(baseline_data.size(), 0);
    
    // Modify component slightly
    pos.x += 0.1f;
    pos.y += 0.05f;
    
    // Create delta
    auto delta_data = sync_system_->create_delta<TestPosition>(entity, baseline_data);
    
    // Delta should be smaller than full component data
    EXPECT_GT(delta_data.size(), 0);
    EXPECT_LT(delta_data.size(), sizeof(TestPosition)); // Should be compressed
}

// =============================================================================
// Authority System Tests
// =============================================================================

TEST_F(NetworkingSystemTest, EntityAuthorityBasics) {
    auto entity = test_entities_[0];
    uint32_t client_id = 1;
    
    // Assign authority
    authority_->assign_authority(entity, client_id);
    EXPECT_TRUE(authority_->has_authority(entity, client_id));
    EXPECT_FALSE(authority_->has_authority(entity, client_id + 1));
    
    // Check authority owner
    EXPECT_EQ(authority_->get_authority_owner(entity), client_id);
}

TEST_F(NetworkingSystemTest, AuthorityTransfer) {
    auto entity = test_entities_[0];
    uint32_t client1 = 1;
    uint32_t client2 = 2;
    
    // Initial authority assignment
    authority_->assign_authority(entity, client1);
    EXPECT_TRUE(authority_->has_authority(entity, client1));
    
    // Transfer authority
    authority_->transfer_authority(entity, client1, client2);
    EXPECT_FALSE(authority_->has_authority(entity, client1));
    EXPECT_TRUE(authority_->has_authority(entity, client2));
    EXPECT_EQ(authority_->get_authority_owner(entity), client2);
}

TEST_F(NetworkingSystemTest, AuthorityZones) {
    // Create entities in different zones
    auto entity1 = world_->create_entity();
    auto entity2 = world_->create_entity();
    
    world_->add_component<TestPosition>(entity1, 0, 0, 0);    // Zone A
    world_->add_component<TestPosition>(entity2, 100, 0, 0);  // Zone B
    
    // Assign zone-based authority
    uint32_t client1 = 1;
    uint32_t client2 = 2;
    
    Networking::AuthorityZone zone_a = {-50, -50, 50, 50};  // Covers entity1
    Networking::AuthorityZone zone_b = {50, -50, 150, 50};  // Covers entity2
    
    authority_->define_authority_zone(client1, zone_a);
    authority_->define_authority_zone(client2, zone_b);
    
    // Update authority based on zones
    authority_->update_zone_authority(*world_);
    
    EXPECT_TRUE(authority_->has_authority(entity1, client1));
    EXPECT_TRUE(authority_->has_authority(entity2, client2));
    EXPECT_FALSE(authority_->has_authority(entity1, client2));
    EXPECT_FALSE(authority_->has_authority(entity2, client1));
}

// =============================================================================
// Network Simulation Tests
// =============================================================================

TEST_F(NetworkingSystemTest, PacketLossSimulation) {
    auto simulator = std::make_unique<Networking::NetworkSimulator>();
    
    // Configure packet loss
    Networking::NetworkConditions conditions;
    conditions.packet_loss_rate = 0.1f; // 10% packet loss
    conditions.latency_ms = 50;
    conditions.jitter_ms = 5;
    
    simulator->set_conditions(conditions);
    
    // Send many packets and count losses
    constexpr int packet_count = 1000;
    int packets_delivered = 0;
    
    for (int i = 0; i < packet_count; ++i) {
        Networking::Packet packet;
        packet.header.sequence = i;
        packet.header.type = Networking::PacketType::EntityUpdate;
        
        if (simulator->should_deliver_packet(packet)) {
            packets_delivered++;
        }
    }
    
    // Should lose approximately 10% of packets
    float actual_loss_rate = 1.0f - (static_cast<float>(packets_delivered) / packet_count);
    EXPECT_NEAR(actual_loss_rate, 0.1f, 0.05f); // Allow 5% tolerance
}

TEST_F(NetworkingSystemTest, LatencySimulation) {
    auto simulator = std::make_unique<Networking::NetworkSimulator>();
    
    Networking::NetworkConditions conditions;
    conditions.packet_loss_rate = 0.0f; // No packet loss for this test
    conditions.latency_ms = 100;
    conditions.jitter_ms = 20;
    
    simulator->set_conditions(conditions);
    
    std::vector<uint32_t> delivery_times;
    
    for (int i = 0; i < 100; ++i) {
        Networking::Packet packet;
        packet.header.sequence = i;
        packet.header.timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        
        uint32_t delivery_delay = simulator->calculate_delivery_delay(packet);
        delivery_times.push_back(delivery_delay);
    }
    
    // Calculate average delay
    uint32_t total_delay = std::accumulate(delivery_times.begin(), delivery_times.end(), 0u);
    float average_delay = static_cast<float>(total_delay) / delivery_times.size();
    
    // Should be approximately 100ms with jitter
    EXPECT_NEAR(average_delay, 100.0f, 30.0f); // Allow for jitter
    
    // Check that there's actual variation (jitter)
    bool has_variation = false;
    for (size_t i = 1; i < delivery_times.size(); ++i) {
        if (delivery_times[i] != delivery_times[i-1]) {
            has_variation = true;
            break;
        }
    }
    EXPECT_TRUE(has_variation) << "Jitter should create variation in delivery times";
}

// =============================================================================
// Performance and Stress Tests
// =============================================================================

TEST_F(NetworkingSystemTest, NetworkingSystemPerformance) {
    constexpr size_t entity_count = 1000;
    constexpr size_t simulation_frames = 60; // 1 second at 60 FPS
    
    // Create many networked entities
    std::vector<Entity> entities;
    for (size_t i = 0; i < entity_count; ++i) {
        auto entity = world_->create_entity();
        world_->add_component<TestPosition>(entity, 
            static_cast<float>(i % 100), static_cast<float>(i / 100), 0);
        world_->add_component<TestVelocity>(entity, 1.0f, 0.5f, 0);
        world_->add_component<Networking::NetworkComponent>(entity, 
            Networking::NetworkId{static_cast<uint32_t>(i + 1)});
        
        replication_->mark_for_replication(entity);
        entities.push_back(entity);
    }
    
    // Measure networking system performance
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t frame = 0; frame < simulation_frames; ++frame) {
        // Simulate entity updates
        for (auto entity : entities) {
            auto& pos = world_->get_component<TestPosition>(entity);
            auto& vel = world_->get_component<TestVelocity>(entity);
            
            pos.x += vel.vx * (1.0f / 60.0f);
            pos.y += vel.vy * (1.0f / 60.0f);
        }
        
        // Process networking updates
        networking_system_->update(*world_, frame);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    double us_per_entity_per_frame = static_cast<double>(duration.count()) / 
                                    (entity_count * simulation_frames);
    
    std::cout << "Networking performance: " << us_per_entity_per_frame 
              << " μs per entity per frame\n";
    
    // Should maintain reasonable performance
    EXPECT_LT(us_per_entity_per_frame, 50.0); // Less than 50μs per entity per frame
}

TEST_F(NetworkingSystemTest, BandwidthUsageTest) {
    constexpr size_t entity_count = 100;
    constexpr size_t test_duration_frames = 60;
    
    // Create entities
    for (size_t i = 0; i < entity_count; ++i) {
        auto entity = world_->create_entity();
        world_->add_component<TestPosition>(entity, 
            static_cast<float>(i), static_cast<float>(i), 0);
        world_->add_component<Networking::NetworkComponent>(entity, 
            Networking::NetworkId{static_cast<uint32_t>(i + 1)});
        replication_->mark_for_replication(entity);
    }
    
    size_t total_bytes_sent = 0;
    
    for (size_t frame = 0; frame < test_duration_frames; ++frame) {
        // Get network updates for this frame
        auto updates = replication_->get_prioritized_updates(1024 * 10); // 10KB budget
        
        for (auto entity : updates) {
            auto data = replication_->create_replication_data(entity, *world_);
            total_bytes_sent += data.size();
        }
    }
    
    // Calculate bandwidth usage
    double bytes_per_second = static_cast<double>(total_bytes_sent) / 
                             (test_duration_frames / 60.0);
    double kbps = (bytes_per_second * 8) / 1024.0; // Convert to kilobits per second
    
    std::cout << "Bandwidth usage: " << kbps << " kbps for " << entity_count 
              << " entities\n";
    
    // Should maintain reasonable bandwidth usage
    EXPECT_LT(kbps, 1000.0); // Less than 1 Mbps for 100 entities
}

} // namespace ECScope::Testing