#include "../framework/ecscope_test_framework.hpp"
#include <ecscope/world.hpp>
#include <ecscope/world3d.hpp>
#include <ecscope/physics_system.hpp>
#include <ecscope/spatial_audio_engine.hpp>
#include <ecscope/networking/ecs_networking_system.hpp>
#include <ecscope/renderer_2d.hpp>
#include <ecscope/asset_pipeline.hpp>
#include <ecscope/hot_reload_system.hpp>
#include <ecscope/performance_benchmark.hpp>
#include <ecscope/memory_tracker.hpp>

#include <thread>
#include <atomic>
#include <chrono>

namespace ECScope::Testing {

// =============================================================================
// Cross-System Integration Test Fixture
// =============================================================================

class CrossSystemIntegrationTest : public ECScopeTestFixture {
protected:
    void SetUp() override {
        ECScopeTestFixture::SetUp();
        
        // Initialize all major systems
        physics_world_ = std::make_unique<Physics3D::World>();
        spatial_audio_ = std::make_unique<SpatialAudio::Engine>();
        networking_ = std::make_unique<Networking::ECSNetworkingSystem>();
        asset_pipeline_ = std::make_unique<Assets::Pipeline>();
        hot_reload_ = std::make_unique<HotReload::System>();
        performance_tracker_ = std::make_unique<Performance::Benchmark>();
        
#ifdef ECSCOPE_HAS_GRAPHICS
        renderer_ = std::make_unique<Renderer2D>();
#endif
        
        // Configure systems
        physics_world_->set_gravity({0.0f, -9.81f, 0.0f});
        
        // Create test scenario
        setup_test_scenario();
    }

    void TearDown() override {
        performance_tracker_.reset();
        hot_reload_.reset();
        asset_pipeline_.reset();
        networking_.reset();
        spatial_audio_.reset();
        physics_world_.reset();
#ifdef ECSCOPE_HAS_GRAPHICS
        renderer_.reset();
#endif
        ECScopeTestFixture::TearDown();
    }

    void setup_test_scenario() {
        // Create a game-like scenario with multiple interacting systems
        
        // Create player entity (Physics + Audio + Networking)
        player_entity_ = world_->create_entity();
        world_->add_component<Transform3D>(player_entity_, Vec3{0, 1, 0});
        world_->add_component<RigidBody3D>(player_entity_, Vec3{0, 0, 0});
        world_->add_component<CollisionBox3D>(player_entity_, Vec3{0.5f, 1.0f, 0.5f});
        world_->add_component<SpatialAudio::Listener>(player_entity_);
        world_->add_component<Networking::NetworkComponent>(player_entity_, 
            Networking::NetworkId{1});
        
        // Create physics objects with audio sources
        for (int i = 0; i < 5; ++i) {
            auto entity = world_->create_entity();
            world_->add_component<Transform3D>(entity, 
                Vec3{static_cast<float>(i * 2), 3.0f, 0});
            world_->add_component<RigidBody3D>(entity, Vec3{0, 0, 0});
            world_->add_component<CollisionBox3D>(entity, Vec3{0.5f, 0.5f, 0.5f});
            
            // Add audio source
            SpatialAudio::SourceParams audio_params;
            audio_params.max_distance = 10.0f;
            audio_params.rolloff_factor = 1.0f;
            world_->add_component<SpatialAudio::Source>(entity, audio_params);
            
            // Add networking
            world_->add_component<Networking::NetworkComponent>(entity, 
                Networking::NetworkId{static_cast<uint32_t>(i + 2)});
                
            physics_objects_.push_back(entity);
        }
    }

protected:
    std::unique_ptr<Physics3D::World> physics_world_;
    std::unique_ptr<SpatialAudio::Engine> spatial_audio_;
    std::unique_ptr<Networking::ECSNetworkingSystem> networking_;
    std::unique_ptr<Assets::Pipeline> asset_pipeline_;
    std::unique_ptr<HotReload::System> hot_reload_;
    std::unique_ptr<Performance::Benchmark> performance_tracker_;
    
#ifdef ECSCOPE_HAS_GRAPHICS
    std::unique_ptr<Renderer2D> renderer_;
#endif

    Entity player_entity_;
    std::vector<Entity> physics_objects_;
};

// =============================================================================
// ECS + Physics Integration Tests
// =============================================================================

TEST_F(CrossSystemIntegrationTest, ECSPhysicsIntegration) {
    // Test that ECS and Physics systems work together correctly
    
    // Record initial positions
    std::vector<Vec3> initial_positions;
    for (auto entity : physics_objects_) {
        auto& transform = world_->get_component<Transform3D>(entity);
        initial_positions.push_back(transform.position);
    }
    
    // Apply forces and simulate
    for (auto entity : physics_objects_) {
        auto& rigidbody = world_->get_component<RigidBody3D>(entity);
        rigidbody.velocity = Vec3{0, -5, 0}; // Fall downward
    }
    
    // Run simulation
    float dt = 1.0f / 60.0f;
    for (int step = 0; step < 60; ++step) { // 1 second
        physics_world_->step(dt);
    }
    
    // Verify objects have fallen
    for (size_t i = 0; i < physics_objects_.size(); ++i) {
        auto& transform = world_->get_component<Transform3D>(physics_objects_[i]);
        EXPECT_LT(transform.position.y, initial_positions[i].y) 
            << "Object " << i << " should have fallen";
    }
    
    // Verify ECS queries still work after physics updates
    size_t physics_entity_count = 0;
    world_->each<Transform3D, RigidBody3D>([&](Entity entity, Transform3D& transform, RigidBody3D& rb) {
        physics_entity_count++;
        EXPECT_TRUE(world_->is_valid(entity));
    });
    
    EXPECT_EQ(physics_entity_count, physics_objects_.size() + 1); // +1 for player
}

TEST_F(CrossSystemIntegrationTest, PhysicsAudioIntegration) {
    // Test physics events triggering audio
    
    struct AudioEvent {
        Entity entity;
        Vec3 position;
        float intensity;
    };
    
    std::vector<AudioEvent> collision_events;
    
    // Set up collision detection callback
    physics_world_->set_collision_callback([&](Entity a, Entity b, const Vec3& point) {
        // Create audio event at collision point
        AudioEvent event;
        event.entity = a; // Use first entity as audio source
        event.position = point;
        event.intensity = 1.0f;
        collision_events.push_back(event);
        
        // Play collision sound
        if (world_->has_component<SpatialAudio::Source>(a)) {
            auto& audio_source = world_->get_component<SpatialAudio::Source>(a);
            spatial_audio_->play_sound(audio_source, "collision_sound");
        }
    });
    
    // Create collision scenario
    auto& player_rb = world_->get_component<RigidBody3D>(player_entity_);
    player_rb.velocity = Vec3{5, 0, 0}; // Move toward objects
    
    // Simulate until collision
    float dt = 1.0f / 60.0f;
    for (int step = 0; step < 300; ++step) { // 5 seconds max
        physics_world_->step(dt);
        
        if (!collision_events.empty()) {
            break; // Collision detected
        }
    }
    
    // Verify collision events were generated
    EXPECT_GT(collision_events.size(), 0) << "Should have detected collisions";
    
    // Verify audio system received the events
    auto audio_events = spatial_audio_->get_recent_events();
    EXPECT_GT(audio_events.size(), 0) << "Audio system should have received collision events";
}

// =============================================================================
// Audio + Networking Integration Tests
// =============================================================================

TEST_F(CrossSystemIntegrationTest, AudioNetworkingSynchronization) {
    // Test that audio events are properly networked
    
    // Set up networking for audio sources
    for (auto entity : physics_objects_) {
        networking_->mark_for_replication(entity);
        networking_->replicate_component<SpatialAudio::Source>(entity);
    }
    
    // Create audio event
    auto audio_entity = physics_objects_[0];
    auto& audio_source = world_->get_component<SpatialAudio::Source>(audio_entity);
    
    // Play sound
    spatial_audio_->play_sound(audio_source, "test_sound", 1.0f);
    
    // Generate network packet
    auto network_data = networking_->create_update_packet(*world_);
    EXPECT_GT(network_data.size(), 0) << "Should generate network data for audio events";
    
    // Simulate receiving the packet on another client
    std::unique_ptr<World> client_world = std::make_unique<World>();
    
    // Apply network update
    bool success = networking_->apply_update_packet(*client_world, network_data);
    EXPECT_TRUE(success) << "Should successfully apply audio network update";
    
    // Verify audio source was replicated
    if (client_world->is_valid(audio_entity)) {
        EXPECT_TRUE(client_world->has_component<SpatialAudio::Source>(audio_entity));
    }
}

TEST_F(CrossSystemIntegrationTest, NetworkedPhysicsAudioConsistency) {
    // Test that networked physics objects maintain audio consistency
    
    struct NetworkedEntity {
        Entity local_entity;
        Entity remote_entity;
        Vec3 last_known_position;
    };
    
    std::vector<NetworkedEntity> networked_entities;
    
    // Set up local and "remote" entities
    for (size_t i = 0; i < 3; ++i) {
        NetworkedEntity net_ent;
        net_ent.local_entity = physics_objects_[i];
        
        // Create corresponding remote entity
        net_ent.remote_entity = world_->create_entity();
        
        // Copy components
        auto& local_transform = world_->get_component<Transform3D>(net_ent.local_entity);
        world_->add_component<Transform3D>(net_ent.remote_entity, local_transform);
        
        auto& local_audio = world_->get_component<SpatialAudio::Source>(net_ent.local_entity);
        world_->add_component<SpatialAudio::Source>(net_ent.remote_entity, local_audio);
        
        net_ent.last_known_position = local_transform.position;
        networked_entities.push_back(net_ent);
    }
    
    // Simulate local physics updates
    float dt = 1.0f / 60.0f;
    for (int step = 0; step < 30; ++step) {
        // Update local physics
        physics_world_->step(dt);
        
        // Simulate network updates (every 4 frames = 15 Hz)
        if (step % 4 == 0) {
            for (auto& net_ent : networked_entities) {
                // Get current local position
                auto& local_transform = world_->get_component<Transform3D>(net_ent.local_entity);
                
                // Update remote entity (with network delay simulation)
                auto& remote_transform = world_->get_component<Transform3D>(net_ent.remote_entity);
                remote_transform.position = net_ent.last_known_position;
                
                // Update audio system with new position
                spatial_audio_->update_source_position(
                    world_->get_component<SpatialAudio::Source>(net_ent.remote_entity),
                    remote_transform.position
                );
                
                net_ent.last_known_position = local_transform.position;
            }
        }
    }
    
    // Verify that audio positions roughly match physics positions
    for (const auto& net_ent : networked_entities) {
        auto& local_pos = world_->get_component<Transform3D>(net_ent.local_entity).position;
        auto& remote_pos = world_->get_component<Transform3D>(net_ent.remote_entity).position;
        
        float distance = Math3D::length(local_pos - remote_pos);
        EXPECT_LT(distance, 2.0f) << "Networked entities should stay reasonably close";
    }
}

// =============================================================================
// Asset Pipeline Integration Tests
// =============================================================================

TEST_F(CrossSystemIntegrationTest, AssetPipelineHotReload) {
    // Test hot reloading of assets used by multiple systems
    
    // Register assets with different systems
    Assets::AssetHandle physics_material = asset_pipeline_->load_asset("test_material.json");
    Assets::AssetHandle audio_clip = asset_pipeline_->load_asset("test_sound.wav");
    Assets::AssetHandle shader = asset_pipeline_->load_asset("test_shader.glsl");
    
    EXPECT_NE(physics_material, Assets::INVALID_HANDLE);
    EXPECT_NE(audio_clip, Assets::INVALID_HANDLE);
    
    // Set up hot reload watching
    hot_reload_->watch_asset(physics_material);
    hot_reload_->watch_asset(audio_clip);
    
    if (shader != Assets::INVALID_HANDLE) {
        hot_reload_->watch_asset(shader);
    }
    
    // Simulate asset changes
    std::atomic<int> reload_count{0};
    
    hot_reload_->set_reload_callback([&](Assets::AssetHandle handle) {
        reload_count++;
        
        // Update systems that use this asset
        if (handle == physics_material) {
            // Reload physics materials
            for (auto entity : physics_objects_) {
                if (world_->has_component<Materials::Component>(entity)) {
                    // Refresh material properties
                    auto& material = world_->get_component<Materials::Component>(entity);
                    asset_pipeline_->reload_asset_data(handle, &material.properties);
                }
            }
        } else if (handle == audio_clip) {
            // Reload audio data
            spatial_audio_->reload_audio_clip(handle);
        }
    });
    
    // Simulate file system changes
    hot_reload_->simulate_file_change("test_material.json");
    hot_reload_->simulate_file_change("test_sound.wav");
    
    // Process hot reload events
    hot_reload_->process_events();
    
    EXPECT_EQ(reload_count.load(), 2) << "Should have reloaded 2 assets";
}

TEST_F(CrossSystemIntegrationTest, AssetDependencyChain) {
    // Test complex asset dependency chains across systems
    
    // Create dependency chain: Scene -> Physics Materials -> Audio Events -> Shaders
    auto scene_asset = asset_pipeline_->load_asset("test_scene.json");
    EXPECT_NE(scene_asset, Assets::INVALID_HANDLE);
    
    // Load scene should trigger loading of dependent assets
    auto dependencies = asset_pipeline_->get_dependencies(scene_asset);
    EXPECT_GT(dependencies.size(), 0) << "Scene should have dependencies";
    
    // Verify all systems received their required assets
    bool physics_materials_loaded = false;
    bool audio_clips_loaded = false;
    bool shaders_loaded = false;
    
    for (auto dep : dependencies) {
        std::string asset_type = asset_pipeline_->get_asset_type(dep);
        
        if (asset_type == "material") {
            physics_materials_loaded = true;
        } else if (asset_type == "audio") {
            audio_clips_loaded = true;
        } else if (asset_type == "shader") {
            shaders_loaded = true;
        }
    }
    
    EXPECT_TRUE(physics_materials_loaded) << "Physics materials should be loaded";
    EXPECT_TRUE(audio_clips_loaded) << "Audio clips should be loaded";
    
#ifdef ECSCOPE_HAS_GRAPHICS
    EXPECT_TRUE(shaders_loaded) << "Shaders should be loaded";
#endif
}

// =============================================================================
// Performance Integration Tests
// =============================================================================

TEST_F(CrossSystemIntegrationTest, MultiSystemPerformance) {
    // Test performance when all systems are running together
    
    performance_tracker_->begin_frame();
    
    // Create a complex scenario
    constexpr size_t entity_count = 500;
    std::vector<Entity> test_entities;
    
    for (size_t i = 0; i < entity_count; ++i) {
        auto entity = world_->create_entity();
        
        // Add components for multiple systems
        world_->add_component<Transform3D>(entity, 
            Vec3{static_cast<float>(i % 50), static_cast<float>(i / 50), 0});
        world_->add_component<RigidBody3D>(entity, Vec3{0, -1, 0});
        world_->add_component<CollisionSphere3D>(entity, 0.5f);
        
        // Every 10th entity gets audio
        if (i % 10 == 0) {
            SpatialAudio::SourceParams params;
            params.max_distance = 5.0f;
            world_->add_component<SpatialAudio::Source>(entity, params);
        }
        
        // Every 5th entity is networked
        if (i % 5 == 0) {
            world_->add_component<Networking::NetworkComponent>(entity, 
                Networking::NetworkId{static_cast<uint32_t>(i + 100)});
        }
        
        test_entities.push_back(entity);
    }
    
    // Measure integrated system performance
    auto start_time = std::chrono::high_resolution_clock::now();
    
    constexpr int simulation_frames = 60; // 1 second at 60 FPS
    
    for (int frame = 0; frame < simulation_frames; ++frame) {
        performance_tracker_->begin_system("Physics");
        physics_world_->step(1.0f / 60.0f);
        performance_tracker_->end_system("Physics");
        
        performance_tracker_->begin_system("Audio");
        spatial_audio_->update(*world_);
        performance_tracker_->end_system("Audio");
        
        performance_tracker_->begin_system("Networking");
        networking_->update(*world_, frame);
        performance_tracker_->end_system("Networking");
        
#ifdef ECSCOPE_HAS_GRAPHICS
        performance_tracker_->begin_system("Rendering");
        renderer_->render(*world_);
        performance_tracker_->end_system("Rendering");
#endif
        
        performance_tracker_->end_frame();
        performance_tracker_->begin_frame();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // Calculate performance metrics
    double fps = static_cast<double>(simulation_frames * 1000) / duration.count();
    auto system_stats = performance_tracker_->get_system_statistics();
    
    std::cout << "Multi-system integration performance:\n";
    std::cout << "  Overall FPS: " << fps << "\n";
    std::cout << "  Total entities: " << entity_count << "\n";
    
    for (const auto& [system_name, stats] : system_stats) {
        std::cout << "  " << system_name << " avg: " << stats.average_time_ms << "ms\n";
    }
    
    // Performance requirements
    EXPECT_GT(fps, 30.0) << "Should maintain at least 30 FPS";
    EXPECT_LT(system_stats["Physics"].average_time_ms, 10.0) << "Physics should take < 10ms";
    EXPECT_LT(system_stats["Audio"].average_time_ms, 5.0) << "Audio should take < 5ms";
    EXPECT_LT(system_stats["Networking"].average_time_ms, 3.0) << "Networking should take < 3ms";
}

TEST_F(CrossSystemIntegrationTest, MemoryConsistencyAcrossSystems) {
    // Test memory consistency when multiple systems modify the same data
    
    auto test_entity = physics_objects_[0];
    
    // Multiple systems will modify this entity concurrently
    std::atomic<bool> systems_running{true};
    std::atomic<int> modification_count{0};
    
    // Physics system thread
    std::thread physics_thread([&]() {
        while (systems_running.load()) {
            auto& transform = world_->get_component<Transform3D>(test_entity);
            transform.position.x += 0.001f;
            modification_count++;
            std::this_thread::sleep_for(std::chrono::microseconds(1000)); // 1ms
        }
    });
    
    // Audio system thread
    std::thread audio_thread([&]() {
        while (systems_running.load()) {
            if (world_->has_component<SpatialAudio::Source>(test_entity)) {
                auto& transform = world_->get_component<Transform3D>(test_entity);
                auto& audio_source = world_->get_component<SpatialAudio::Source>(test_entity);
                
                // Audio system reads position for 3D audio processing
                Vec3 position = transform.position;
                spatial_audio_->update_source_position(audio_source, position);
                modification_count++;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(2000)); // 2ms
        }
    });
    
    // Networking system thread
    std::thread network_thread([&]() {
        while (systems_running.load()) {
            // Networking system reads position for replication
            auto& transform = world_->get_component<Transform3D>(test_entity);
            Vec3 position = transform.position;
            
            // Simulate network serialization
            uint8_t buffer[sizeof(Vec3)];
            std::memcpy(buffer, &position, sizeof(Vec3));
            modification_count++;
            
            std::this_thread::sleep_for(std::chrono::microseconds(1500)); // 1.5ms
        }
    });
    
    // Run for a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    systems_running = false;
    
    // Wait for threads to finish
    physics_thread.join();
    audio_thread.join();
    network_thread.join();
    
    // Verify no corruption occurred
    auto& final_transform = world_->get_component<Transform3D>(test_entity);
    
    // Position should have increased (physics was adding to x)
    EXPECT_GT(final_transform.position.x, 0.0f);
    
    // No NaN values should exist
    EXPECT_FALSE(std::isnan(final_transform.position.x));
    EXPECT_FALSE(std::isnan(final_transform.position.y));
    EXPECT_FALSE(std::isnan(final_transform.position.z));
    
    std::cout << "Memory consistency test: " << modification_count.load() 
              << " concurrent modifications completed successfully\n";
    
    EXPECT_GT(modification_count.load(), 100) << "Should have performed many concurrent operations";
}

// =============================================================================
// Educational System Integration Tests
// =============================================================================

TEST_F(CrossSystemIntegrationTest, EducationalVisualizationIntegration) {
    // Test that educational visualizations work with all systems
    
    struct VisualizationData {
        std::vector<Vec3> entity_positions;
        std::vector<Vec3> physics_forces;
        std::vector<float> audio_levels;
        std::vector<uint32_t> network_updates;
    };
    
    VisualizationData viz_data;
    
    // Collect data from all systems
    world_->each<Transform3D>([&](Entity entity, Transform3D& transform) {
        viz_data.entity_positions.push_back(transform.position);
        
        // Get physics data if available
        if (world_->has_component<RigidBody3D>(entity)) {
            auto& rb = world_->get_component<RigidBody3D>(entity);
            Vec3 force = rb.velocity; // Use velocity as proxy for force
            viz_data.physics_forces.push_back(force);
        }
        
        // Get audio data if available
        if (world_->has_component<SpatialAudio::Source>(entity)) {
            float audio_level = spatial_audio_->get_source_level(
                world_->get_component<SpatialAudio::Source>(entity));
            viz_data.audio_levels.push_back(audio_level);
        }
        
        // Get network data if available
        if (world_->has_component<Networking::NetworkComponent>(entity)) {
            auto& net_comp = world_->get_component<Networking::NetworkComponent>(entity);
            viz_data.network_updates.push_back(net_comp.network_id.id);
        }
    });
    
    // Verify we collected comprehensive data
    EXPECT_GT(viz_data.entity_positions.size(), 0) << "Should have entity positions";
    EXPECT_GT(viz_data.physics_forces.size(), 0) << "Should have physics forces";
    EXPECT_GT(viz_data.audio_levels.size(), 0) << "Should have audio levels";
    EXPECT_GT(viz_data.network_updates.size(), 0) << "Should have network updates";
    
    // Test that visualization data is coherent
    for (const auto& pos : viz_data.entity_positions) {
        EXPECT_FALSE(std::isnan(pos.x)) << "Position data should be valid";
        EXPECT_FALSE(std::isnan(pos.y)) << "Position data should be valid";
        EXPECT_FALSE(std::isnan(pos.z)) << "Position data should be valid";
    }
    
    for (const auto& force : viz_data.physics_forces) {
        EXPECT_FALSE(std::isnan(force.x)) << "Force data should be valid";
        EXPECT_FALSE(std::isnan(force.y)) << "Force data should be valid";
        EXPECT_FALSE(std::isnan(force.z)) << "Force data should be valid";
    }
    
    for (float level : viz_data.audio_levels) {
        EXPECT_GE(level, 0.0f) << "Audio levels should be non-negative";
        EXPECT_FALSE(std::isnan(level)) << "Audio levels should be valid";
    }
}

} // namespace ECScope::Testing