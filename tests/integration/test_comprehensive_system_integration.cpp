#include "../framework/ecscope_test_framework.hpp"

// Include all major system headers
#include <ecscope/registry.hpp>
#include <ecscope/world.hpp>
#include <ecscope/system.hpp>

#ifdef ECSCOPE_ENABLE_PHYSICS
#include <ecscope/physics_system.hpp>
#include <ecscope/advanced_physics_complete.hpp>
#endif

#include <ecscope/audio_systems.hpp>
#include <ecscope/spatial_audio_engine.hpp>

#include <ecscope/networking/ecs_networking_system.hpp>

#include <ecscope/ecs_performance_benchmarker.hpp>
#include <ecscope/memory_benchmark_suite.hpp>

#include <ecscope/asset_pipeline.hpp>
#include <ecscope/asset_hot_reload_manager.hpp>

#include <ecscope/educational_system.hpp>
#include <ecscope/tutorial_system.hpp>

#include <thread>
#include <future>
#include <chrono>
#include <random>

namespace ECScope::Testing {

using namespace ecscope::networking;
using namespace ecscope::audio;
using namespace ecscope::performance;

class CrossSystemIntegrationTest : public ECScopeTestFixture {
protected:
    void SetUp() override {
        ECScopeTestFixture::SetUp();
        
        // Initialize comprehensive system integration
        integration_memory_tracker_ = std::make_unique<MemoryTracker>("CrossSystemIntegration");
        integration_memory_tracker_->start_tracking();
        
#ifdef ECSCOPE_ENABLE_PHYSICS
        // Initialize physics system
        physics_system_ = std::make_unique<Physics::PhysicsSystem>();
        world_->add_system(physics_system_.get());
#endif
        
        // Initialize audio systems
        setup_audio_systems();
        
        // Initialize networking system
        setup_networking_systems();
        
        // Initialize asset pipeline
        setup_asset_pipeline();
        
        // Initialize educational systems
        setup_educational_systems();
        
        // Initialize performance monitoring
        performance_benchmarker_ = std::make_unique<ECSPerformanceBenchmarker>();
        memory_benchmarker_ = std::make_unique<memory::MemoryBenchmarkSuite>();
        
        // Create integration test world
        create_integration_test_world();
    }
    
    void TearDown() override {
        // Shutdown systems in reverse order
        shutdown_educational_systems();
        shutdown_asset_pipeline();
        shutdown_networking_systems();
        shutdown_audio_systems();
        
#ifdef ECSCOPE_ENABLE_PHYSICS
        physics_system_.reset();
#endif
        
        performance_benchmarker_.reset();
        memory_benchmarker_.reset();
        
        // Check for memory leaks
        if (integration_memory_tracker_) {
            integration_memory_tracker_->stop_tracking();
            EXPECT_EQ(integration_memory_tracker_->get_allocation_count(),
                     integration_memory_tracker_->get_deallocation_count());
        }
        
        integration_memory_tracker_.reset();
        
        ECScopeTestFixture::TearDown();
    }
    
    void setup_audio_systems() {
        // Create mock audio device for testing
        audio_device_ = std::make_unique<audio::MockAudioDevice>();
        
        // Configure spatial audio engine
        audio::SpatialAudioEngineConfig audio_config;
        audio_config.sample_rate = 44100;
        audio_config.buffer_size = 512;
        audio_config.max_sources = 32;
        audio_config.enable_hrtf = true;
        audio_config.audio_device = audio_device_.get();
        
        spatial_audio_engine_ = std::make_unique<audio::SpatialAudioEngine>(audio_config);
        
        // Create audio systems
        spatial_audio_system_ = std::make_unique<audio::systems::SpatialAudioSystem>(
            integration_memory_tracker_.get());
        audio_listener_system_ = std::make_unique<audio::systems::AudioListenerSystem>(
            integration_memory_tracker_.get());
        
        world_->add_system(spatial_audio_system_.get());
        world_->add_system(audio_listener_system_.get());
    }
    
    void setup_networking_systems() {
        // Create separate registries for client/server simulation
        server_registry_ = std::make_unique<Registry>();
        client_registry_ = std::make_unique<Registry>();
        
        // Configure networking for local testing
        constexpr u16 test_port = 58000;
        
        server_config_ = NetworkConfig::server_default();
        server_config_.server_address = NetworkAddress::local(test_port);
        server_config_.max_clients = 2;
        server_config_.enable_network_visualization = false;
        
        client_config_ = NetworkConfig::client_default();
        client_config_.server_address = NetworkAddress::local(test_port);
        client_config_.enable_network_visualization = false;
        
        // Create networking systems (not started by default)
        server_networking_ = std::make_unique<ECSNetworkingSystem>(*server_registry_, server_config_);
        client_networking_ = std::make_unique<ECSNetworkingSystem>(*client_registry_, client_config_);
    }
    
    void setup_asset_pipeline() {
        // Initialize asset pipeline with test configuration
        asset_pipeline_config_.asset_root_directory = "test_assets";
        asset_pipeline_config_.enable_hot_reload = true;
        asset_pipeline_config_.enable_compression = false; // Faster for tests
        asset_pipeline_config_.cache_directory = "test_cache";
        
        asset_pipeline_ = std::make_unique<AssetPipeline>(asset_pipeline_config_);
        asset_hot_reload_ = std::make_unique<AssetHotReloadManager>(*asset_pipeline_);
        
        // Create test assets directory if needed
        create_test_assets();
    }
    
    void setup_educational_systems() {
        educational_system_ = std::make_unique<EducationalSystem>();
        tutorial_system_ = std::make_unique<TutorialSystem>();
        
        // Configure educational systems
        educational_system_->enable_performance_tracking(true);
        educational_system_->enable_interactive_tutorials(true);
        
        tutorial_system_->load_tutorial_set("integration_tutorials");
    }
    
    void create_integration_test_world() {
        // Create a complex world with entities that interact across systems
        
        // Physics entities with audio
        for (int i = 0; i < 10; ++i) {
            auto entity = world_->create_entity();
            
            // Transform component
            Vec3 position = {
                static_cast<f32>(i * 5 - 25),  // Spread from -25 to 25
                10.0f,
                static_cast<f32>(i % 3 - 1) * 10.0f
            };
            world_->add_component<Transform>(entity, position);
            
#ifdef ECSCOPE_ENABLE_PHYSICS
            // Physics components
            Physics::RigidBody rigidbody;
            rigidbody.mass = 1.0f + i * 0.1f;
            rigidbody.velocity = {0.0f, 0.0f, 0.0f};
            world_->add_component<Physics::RigidBody>(entity, rigidbody);
            
            Physics::SphereCollider collider;
            collider.radius = 0.5f;
            world_->add_component<Physics::SphereCollider>(entity, collider);
#endif
            
            // Audio components
            audio::AudioSource audio_source;
            audio_source.volume = 0.7f;
            audio_source.type = audio::AudioSourceType::Point;
            audio_source.enable_hrtf = true;
            audio_source.enable_doppler = true;
            audio_source.is_playing = true;
            world_->add_component<audio::AudioSource>(entity, audio_source);
            
            physics_audio_entities_.push_back(entity);
        }
        
        // Audio listener
        audio_listener_entity_ = world_->create_entity();
        world_->add_component<Transform>(audio_listener_entity_, Vec3{0.0f, 5.0f, 0.0f});
        
        audio::AudioListener listener;
        listener.is_active = true;
        listener.gain = 1.0f;
        world_->add_component<audio::AudioListener>(audio_listener_entity_, listener);
        
#ifdef ECSCOPE_ENABLE_PHYSICS
        // Environment entities (walls, floors)
        create_physics_environment();
#endif
        
        // Create networked entities (for networking tests)
        create_networked_entities();
    }
    
#ifdef ECSCOPE_ENABLE_PHYSICS
    void create_physics_environment() {
        // Floor
        auto floor = world_->create_entity();
        world_->add_component<Transform>(floor, Vec3{0.0f, 0.0f, 0.0f});
        
        Physics::RigidBody floor_body;
        floor_body.is_static = true;
        floor_body.mass = std::numeric_limits<f32>::infinity();
        world_->add_component<Physics::RigidBody>(floor, floor_body);
        
        Physics::BoxCollider floor_collider;
        floor_collider.half_extents = {50.0f, 0.5f, 50.0f};
        world_->add_component<Physics::BoxCollider>(floor, floor_collider);
        
        // Walls (for audio occlusion testing)
        for (int i = 0; i < 3; ++i) {
            auto wall = world_->create_entity();
            Vec3 wall_position = {
                static_cast<f32>(i * 20 - 20), 
                3.0f, 
                0.0f
            };
            world_->add_component<Transform>(wall, wall_position);
            
            Physics::RigidBody wall_body;
            wall_body.is_static = true;
            wall_body.mass = std::numeric_limits<f32>::infinity();
            world_->add_component<Physics::RigidBody>(wall, wall_body);
            
            Physics::BoxCollider wall_collider;
            wall_collider.half_extents = {0.5f, 3.0f, 10.0f};
            world_->add_component<Physics::BoxCollider>(wall, wall_collider);
            
            environment_entities_.push_back(wall);
        }
    }
#endif
    
    void create_networked_entities() {
        // Create entities that will be synchronized over network
        for (int i = 0; i < 5; ++i) {
            auto entity = world_->create_entity();
            
            Vec3 position = {
                static_cast<f32>(i * 3),
                2.0f,
                0.0f
            };
            world_->add_component<Transform>(entity, position);
            world_->add_component<TestVelocity>(entity, 1.0f + i * 0.5f, 0.0f, 0.0f);
            world_->add_component<TestHealth>(entity, 100 - i * 10, 100);
            
            networked_entities_.push_back(entity);
        }
    }
    
    void create_test_assets() {
        // Create minimal test assets for asset pipeline testing
        // Note: In a real implementation, these would be actual asset files
        
        test_audio_asset_ = AssetHandle::create("test_audio.wav", AssetType::Audio);
        test_texture_asset_ = AssetHandle::create("test_texture.png", AssetType::Texture);
        test_model_asset_ = AssetHandle::create("test_model.obj", AssetType::Model);
    }
    
    void shutdown_audio_systems() {
        spatial_audio_engine_.reset();
        audio_device_.reset();
        spatial_audio_system_.reset();
        audio_listener_system_.reset();
    }
    
    void shutdown_networking_systems() {
        if (client_networking_) {
            client_networking_->shutdown();
        }
        if (server_networking_) {
            server_networking_->shutdown();
        }
        client_networking_.reset();
        server_networking_.reset();
        client_registry_.reset();
        server_registry_.reset();
    }
    
    void shutdown_asset_pipeline() {
        asset_hot_reload_.reset();
        asset_pipeline_.reset();
    }
    
    void shutdown_educational_systems() {
        tutorial_system_.reset();
        educational_system_.reset();
    }
    
    void update_all_systems(f32 delta_time) {
        world_->update(delta_time);
        
        if (asset_hot_reload_) {
            asset_hot_reload_->update(delta_time);
        }
        
        if (educational_system_) {
            educational_system_->update(delta_time, *world_);
        }
        
        if (tutorial_system_) {
            tutorial_system_->update(delta_time);
        }
    }

protected:
    std::unique_ptr<MemoryTracker> integration_memory_tracker_;
    
    // Physics system
#ifdef ECSCOPE_ENABLE_PHYSICS
    std::unique_ptr<Physics::PhysicsSystem> physics_system_;
#endif
    
    // Audio systems
    std::unique_ptr<audio::MockAudioDevice> audio_device_;
    std::unique_ptr<audio::SpatialAudioEngine> spatial_audio_engine_;
    std::unique_ptr<audio::systems::SpatialAudioSystem> spatial_audio_system_;
    std::unique_ptr<audio::systems::AudioListenerSystem> audio_listener_system_;
    
    // Networking systems
    std::unique_ptr<Registry> server_registry_;
    std::unique_ptr<Registry> client_registry_;
    NetworkConfig server_config_;
    NetworkConfig client_config_;
    std::unique_ptr<ECSNetworkingSystem> server_networking_;
    std::unique_ptr<ECSNetworkingSystem> client_networking_;
    
    // Asset pipeline
    AssetPipelineConfig asset_pipeline_config_;
    std::unique_ptr<AssetPipeline> asset_pipeline_;
    std::unique_ptr<AssetHotReloadManager> asset_hot_reload_;
    AssetHandle test_audio_asset_;
    AssetHandle test_texture_asset_;
    AssetHandle test_model_asset_;
    
    // Educational systems
    std::unique_ptr<EducationalSystem> educational_system_;
    std::unique_ptr<TutorialSystem> tutorial_system_;
    
    // Performance monitoring
    std::unique_ptr<ECSPerformanceBenchmarker> performance_benchmarker_;
    std::unique_ptr<memory::MemoryBenchmarkSuite> memory_benchmarker_;
    
    // Test entities
    std::vector<Entity> physics_audio_entities_;
    std::vector<Entity> environment_entities_;
    std::vector<Entity> networked_entities_;
    Entity audio_listener_entity_;
};

// =============================================================================
// Physics + Audio Integration Tests
// =============================================================================

#ifdef ECSCOPE_ENABLE_PHYSICS
TEST_F(CrossSystemIntegrationTest, PhysicsAudioDopplerIntegration) {
    // Test Doppler effect with moving physics objects
    
    // Get a physics-audio entity and set it in motion
    ASSERT_FALSE(physics_audio_entities_.empty());
    auto moving_entity = physics_audio_entities_[0];
    
    auto& rigidbody = world_->get_component<Physics::RigidBody>(moving_entity);
    rigidbody.velocity = {15.0f, 0.0f, 0.0f}; // 15 m/s to the right
    
    auto& audio_source = world_->get_component<audio::AudioSource>(moving_entity);
    audio_source.enable_doppler = true;
    
    // Run simulation for several frames
    for (int i = 0; i < 30; ++i) {
        update_all_systems(0.033f); // 30 FPS updates
    }
    
    // Check that Doppler effect was applied
    EXPECT_TRUE(audio_source.doppler_data.has_doppler_effect);
    EXPECT_NE(audio_source.doppler_data.pitch_shift, 1.0f);
    
    // Object moving away from listener should have lower pitch
    auto& listener_transform = world_->get_component<Transform>(audio_listener_entity_);
    auto& entity_transform = world_->get_component<Transform>(moving_entity);
    
    Vec3 relative_velocity = rigidbody.velocity;
    Vec3 direction_to_listener = (listener_transform.position - entity_transform.position).normalized();
    f32 radial_velocity = relative_velocity.dot(direction_to_listener);
    
    if (radial_velocity < 0) { // Moving away
        EXPECT_LT(audio_source.doppler_data.pitch_shift, 1.0f);
    } else { // Moving towards
        EXPECT_GT(audio_source.doppler_data.pitch_shift, 1.0f);
    }
}

TEST_F(CrossSystemIntegrationTest, PhysicsAudioCollisionSounds) {
    // Test automatic audio triggering from physics collisions
    
    // Create a falling object
    auto falling_object = world_->create_entity();
    world_->add_component<Transform>(falling_object, Vec3{0.0f, 20.0f, 0.0f});
    
    Physics::RigidBody falling_body;
    falling_body.mass = 2.0f;
    falling_body.velocity = {0.0f, 0.0f, 0.0f};
    world_->add_component<Physics::RigidBody>(falling_object, falling_body);
    
    Physics::SphereCollider falling_collider;
    falling_collider.radius = 0.5f;
    world_->add_component<Physics::SphereCollider>(falling_object, falling_collider);
    
    // Add collision audio trigger
    audio::CollisionAudioTrigger collision_audio;
    collision_audio.impact_volume_threshold = 5.0f; // Moderate impact needed
    collision_audio.audio_asset = test_audio_asset_;
    collision_audio.enable_impact_synthesis = true;
    world_->add_component<audio::CollisionAudioTrigger>(falling_object, collision_audio);
    
    // Simulate until collision with floor
    for (int i = 0; i < 120; ++i) { // ~4 seconds at 30 FPS
        update_all_systems(0.033f);
        
        auto& transform = world_->get_component<Transform>(falling_object);
        if (transform.position.y <= 1.0f) {
            break; // Hit the floor
        }
    }
    
    // Check that collision audio was triggered
    EXPECT_TRUE(collision_audio.collision_detected);
    EXPECT_GT(collision_audio.last_impact_magnitude, collision_audio.impact_volume_threshold);
    
    // An audio source should have been created for the collision sound
    // (Implementation detail - verify collision sound system works)
}

TEST_F(CrossSystemIntegrationTest, PhysicsAudioOcclusionIntegration) {
    // Test audio occlusion with physics-based ray casting
    
    ASSERT_FALSE(physics_audio_entities_.empty());
    ASSERT_FALSE(environment_entities_.empty());
    
    auto audio_source_entity = physics_audio_entities_[0];
    auto wall_entity = environment_entities_[0];
    
    // Position audio source behind wall relative to listener
    auto& source_transform = world_->get_component<Transform>(audio_source_entity);
    auto& wall_transform = world_->get_component<Transform>(wall_entity);
    auto& listener_transform = world_->get_component<Transform>(audio_listener_entity_);
    
    // Place source on opposite side of wall from listener
    source_transform.position = wall_transform.position + Vec3{5.0f, 0.0f, 0.0f};
    listener_transform.position = wall_transform.position - Vec3{5.0f, 0.0f, 0.0f};
    
    auto& audio_source = world_->get_component<audio::AudioSource>(audio_source_entity);
    audio_source.enable_occlusion = true;
    
    // Update systems to calculate occlusion
    for (int i = 0; i < 10; ++i) {
        update_all_systems(0.033f);
    }
    
    // Check that occlusion was detected and applied
    EXPECT_TRUE(audio_source.occlusion_data.is_occluded);
    EXPECT_LT(audio_source.occlusion_data.occlusion_factor, 1.0f);
    EXPECT_GT(audio_source.occlusion_data.occlusion_factor, 0.0f);
    
    // Effective volume should be reduced
    EXPECT_LT(audio_source.effective_volume, audio_source.volume);
}
#endif

// =============================================================================
// ECS + Networking Integration Tests
// =============================================================================

TEST_F(CrossSystemIntegrationTest, ECSNetworkingBasicSynchronization) {
    // Test basic ECS entity synchronization over network
    
    // Start server
    ASSERT_TRUE(server_networking_->start_server());
    
    // Wait for server to be ready
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Create and register entities for networking on server
    for (auto entity : networked_entities_) {
        auto network_id = server_networking_->register_entity(entity);
        EXPECT_NE(network_id, 0);
        
        server_networking_->register_component_sync<Transform>();
        server_networking_->register_component_sync<TestVelocity>();
        server_networking_->register_component_sync<TestHealth>();
    }
    
    // Start client and connect
    ASSERT_TRUE(client_networking_->start_client());
    
    // Wait for connection
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Simulate network updates
    for (int i = 0; i < 30; ++i) {
        // Update entity positions on server
        for (auto entity : networked_entities_) {
            auto& transform = server_registry_->get_component<Transform>(entity);
            auto& velocity = server_registry_->get_component<TestVelocity>(entity);
            
            transform.position.x += velocity.vx * 0.033f;
            
            server_networking_->mark_component_changed<Transform>(entity);
        }
        
        // Update networking systems
        server_networking_->update(0.033f);
        client_networking_->update(0.033f);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }
    
    // Verify network statistics
    auto server_stats = server_networking_->get_network_stats();
    auto client_stats = client_networking_->get_network_stats();
    
    EXPECT_GT(server_stats.packets_sent, 0);
    EXPECT_GT(client_stats.packets_received, 0);
    
    // Clean shutdown
    client_networking_->shutdown();
    server_networking_->shutdown();
}

TEST_F(CrossSystemIntegrationTest, NetworkedPhysicsAudioIntegration) {
    // Test physics and audio synchronization over network
    
#ifdef ECSCOPE_ENABLE_PHYSICS
    // This test would verify that physics-driven audio events 
    // are properly synchronized across network clients
    
    ASSERT_TRUE(server_networking_->start_server());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Register physics-audio entities for networking
    for (auto entity : physics_audio_entities_) {
        auto network_id = server_networking_->register_entity(entity);
        ASSERT_NE(network_id, 0);
        
        server_networking_->register_component_sync<Transform>();
        server_networking_->register_component_sync<audio::AudioSource>();
    }
    
    ASSERT_TRUE(client_networking_->start_client());
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Apply physics forces to networked entities
    for (auto entity : physics_audio_entities_) {
        auto& rigidbody = server_registry_->get_component<Physics::RigidBody>(entity);
        rigidbody.add_force({100.0f, 50.0f, 0.0f});
    }
    
    // Simulate integrated physics, audio, and networking
    for (int i = 0; i < 60; ++i) {
        // Update server physics and audio
        physics_system_->update(0.033f);
        spatial_audio_system_->update(0.033f);
        
        // Mark changed components for networking
        for (auto entity : physics_audio_entities_) {
            server_networking_->mark_component_changed<Transform>(entity);
            server_networking_->mark_component_changed<audio::AudioSource>(entity);
        }
        
        // Update networking
        server_networking_->update(0.033f);
        client_networking_->update(0.033f);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }
    
    // Verify cross-system integration
    auto entity_stats = server_networking_->get_entity_stats();
    EXPECT_GT(entity_stats.components_synchronized, 0);
    
    client_networking_->shutdown();
    server_networking_->shutdown();
#else
    GTEST_SKIP() << "Physics not enabled, skipping physics-networking integration test";
#endif
}

// =============================================================================
// Asset Pipeline Integration Tests
// =============================================================================

TEST_F(CrossSystemIntegrationTest, AssetPipelineHotReloadIntegration) {
    // Test hot-reloading of assets used by various systems
    
    // Load initial assets
    EXPECT_TRUE(asset_pipeline_->load_asset(test_audio_asset_));
    EXPECT_TRUE(asset_pipeline_->load_asset(test_texture_asset_));
    
    auto initial_audio_asset = asset_pipeline_->get_asset<audio::AudioAsset>(test_audio_asset_);
    EXPECT_NE(initial_audio_asset, nullptr);
    
    // Assign asset to an entity
    ASSERT_FALSE(physics_audio_entities_.empty());
    auto entity = physics_audio_entities_[0];
    
    audio::AudioBuffer audio_buffer;
    audio_buffer.asset_handle = test_audio_asset_;
    world_->add_component<audio::AudioBuffer>(entity, audio_buffer);
    
    // Simulate asset modification (trigger hot reload)
    asset_hot_reload_->simulate_asset_change(test_audio_asset_);
    
    // Update systems to process hot reload
    for (int i = 0; i < 10; ++i) {
        update_all_systems(0.033f);
    }
    
    // Verify hot reload occurred
    auto reloaded_audio_asset = asset_pipeline_->get_asset<audio::AudioAsset>(test_audio_asset_);
    EXPECT_NE(reloaded_audio_asset, nullptr);
    
    // Check that entity's audio buffer was updated
    auto& updated_buffer = world_->get_component<audio::AudioBuffer>(entity);
    EXPECT_TRUE(updated_buffer.is_updated_from_asset);
}

TEST_F(CrossSystemIntegrationTest, AssetStreamingWithSystems) {
    // Test streaming asset loading while systems are running
    
    std::vector<AssetHandle> streaming_assets;
    
    // Create many asset handles for streaming test
    for (int i = 0; i < 20; ++i) {
        std::string asset_name = "streaming_audio_" + std::to_string(i) + ".wav";
        auto handle = AssetHandle::create(asset_name, AssetType::Audio);
        streaming_assets.push_back(handle);
    }
    
    // Start streaming load in background
    auto streaming_future = std::async(std::launch::async, [&]() {
        for (auto& handle : streaming_assets) {
            asset_pipeline_->load_asset_async(handle);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });
    
    // Continue updating systems while streaming
    for (int i = 0; i < 100; ++i) {
        update_all_systems(0.033f);
        
        // Check streaming progress
        int loaded_count = 0;
        for (auto& handle : streaming_assets) {
            if (asset_pipeline_->is_asset_loaded(handle)) {
                loaded_count++;
            }
        }
        
        if (loaded_count == streaming_assets.size()) {
            break; // All assets loaded
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }
    
    streaming_future.wait();
    
    // Verify all assets were loaded successfully
    for (auto& handle : streaming_assets) {
        EXPECT_TRUE(asset_pipeline_->is_asset_loaded(handle));
    }
}

// =============================================================================
// Educational System Integration Tests
// =============================================================================

TEST_F(CrossSystemIntegrationTest, EducationalSystemCrossSystemTracking) {
    // Test that educational system tracks concepts across multiple systems
    
    educational_system_->start_learning_session("CrossSystemIntegration");
    
    // Trigger educational events in different systems
    
    // Physics events
#ifdef ECSCOPE_ENABLE_PHYSICS
    for (auto entity : physics_audio_entities_) {
        auto& rigidbody = world_->get_component<Physics::RigidBody>(entity);
        rigidbody.add_force({50.0f, 0.0f, 0.0f});
    }
#endif
    
    // Audio events
    auto& audio_source = world_->get_component<audio::AudioSource>(physics_audio_entities_[0]);
    audio_source.enable_hrtf = true;
    audio_source.enable_analysis = true;
    
    // Simulate systems to generate educational events
    for (int i = 0; i < 50; ++i) {
        update_all_systems(0.033f);
    }
    
    educational_system_->end_learning_session();
    
    // Check that cross-system concepts were tracked
    auto learning_progress = educational_system_->get_learning_progress();
    EXPECT_GT(learning_progress.concepts_encountered.size(), 0);
    
    // Should have concepts from multiple systems
    bool has_physics_concepts = false;
    bool has_audio_concepts = false;
    bool has_integration_concepts = false;
    
    for (const auto& concept : learning_progress.concepts_encountered) {
        if (concept.find("physics") != std::string::npos || 
            concept.find("collision") != std::string::npos) {
            has_physics_concepts = true;
        }
        if (concept.find("audio") != std::string::npos || 
            concept.find("spatial") != std::string::npos ||
            concept.find("HRTF") != std::string::npos) {
            has_audio_concepts = true;
        }
        if (concept.find("integration") != std::string::npos ||
            concept.find("cross-system") != std::string::npos) {
            has_integration_concepts = true;
        }
    }
    
    EXPECT_TRUE(has_audio_concepts);
    // Physics and integration concepts depend on compilation flags
#ifdef ECSCOPE_ENABLE_PHYSICS
    EXPECT_TRUE(has_physics_concepts);
    EXPECT_TRUE(has_integration_concepts);
#endif
}

TEST_F(CrossSystemIntegrationTest, InteractiveTutorialSystemIntegration) {
    // Test interactive tutorials that involve multiple systems
    
    tutorial_system_->start_tutorial("cross_system_integration");
    
    auto current_tutorial = tutorial_system_->get_current_tutorial();
    ASSERT_NE(current_tutorial, nullptr);
    EXPECT_EQ(current_tutorial->title, "Cross-System Integration");
    
    // Tutorial should have multiple steps covering different systems
    EXPECT_GT(current_tutorial->steps.size(), 3);
    
    // Simulate tutorial progression through different system interactions
    for (size_t step = 0; step < current_tutorial->steps.size(); ++step) {
        auto& tutorial_step = current_tutorial->steps[step];
        
        // Execute tutorial step action
        if (tutorial_step.system_type == "physics") {
#ifdef ECSCOPE_ENABLE_PHYSICS
            // Perform physics action
            auto entity = physics_audio_entities_[step % physics_audio_entities_.size()];
            auto& rigidbody = world_->get_component<Physics::RigidBody>(entity);
            rigidbody.add_impulse({10.0f, 5.0f, 0.0f});
#endif
        } else if (tutorial_step.system_type == "audio") {
            // Perform audio action
            auto entity = physics_audio_entities_[step % physics_audio_entities_.size()];
            auto& audio_source = world_->get_component<audio::AudioSource>(entity);
            audio_source.volume = 0.8f;
            audio_source.enable_hrtf = true;
        }
        
        // Update systems
        for (int i = 0; i < 10; ++i) {
            update_all_systems(0.033f);
        }
        
        // Check tutorial step completion
        bool step_completed = tutorial_system_->check_step_completion(tutorial_step);
        EXPECT_TRUE(step_completed) << "Tutorial step " << step << " not completed";
        
        if (step_completed) {
            tutorial_system_->advance_to_next_step();
        }
    }
    
    // Tutorial should be completed
    EXPECT_TRUE(tutorial_system_->is_tutorial_completed());
    
    auto completion_stats = tutorial_system_->get_completion_statistics();
    EXPECT_EQ(completion_stats.completed_steps, current_tutorial->steps.size());
    EXPECT_GT(completion_stats.completion_time_seconds, 0.0);
}

// =============================================================================
// Performance Integration Tests
// =============================================================================

TEST_F(CrossSystemIntegrationTest, CrossSystemPerformanceProfiling) {
    // Test performance profiling of integrated systems
    
    performance_benchmarker_->register_all_standard_tests();
    
    // Configure for cross-system testing
    ECSBenchmarkConfig config;
    config.entity_counts = {50, 100, 200}; // Moderate counts for integration testing
    config.iterations = 5;
    config.test_physics_integration = true;
    config.enable_stress_testing = false;
    
    performance_benchmarker_->set_config(config);
    
    // Run cross-system benchmarks
    performance_benchmarker_->run_all_benchmarks();
    
    // Wait for completion
    auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    while (performance_benchmarker_->is_running() && 
           std::chrono::steady_clock::now() < timeout) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    EXPECT_FALSE(performance_benchmarker_->is_running());
    
    // Analyze results
    auto results = performance_benchmarker_->get_results();
    EXPECT_GT(results.size(), 0);
    
    performance_benchmarker_->analyze_results();
    
    // Generate cross-system performance report
    std::string performance_report = performance_benchmarker_->generate_comparative_report();
    EXPECT_FALSE(performance_report.empty());
    EXPECT_GT(performance_report.length(), 500);
    
    std::cout << "Cross-System Performance Report (excerpt):\n"
              << performance_report.substr(0, 500) << "...\n";
}

TEST_F(CrossSystemIntegrationTest, MemoryIntegrationBenchmarking) {
    // Test memory usage across integrated systems
    
    memory_benchmarker_->configure_for_integration_testing();
    memory_benchmarker_->start_comprehensive_analysis();
    
    // Create high-memory-usage scenario
    std::vector<Entity> temp_entities;
    
    for (int i = 0; i < 100; ++i) {
        auto entity = world_->create_entity();
        world_->add_component<Transform>(entity, Vec3{static_cast<f32>(i), 0, 0});
        
#ifdef ECSCOPE_ENABLE_PHYSICS
        Physics::RigidBody rigidbody;
        rigidbody.mass = 1.0f;
        world_->add_component<Physics::RigidBody>(entity, rigidbody);
        
        Physics::SphereCollider collider;
        collider.radius = 0.5f;
        world_->add_component<Physics::SphereCollider>(entity, collider);
#endif
        
        audio::AudioSource audio_source;
        audio_source.enable_hrtf = true;
        audio_source.enable_analysis = true;
        world_->add_component<audio::AudioSource>(entity, audio_source);
        
        temp_entities.push_back(entity);
    }
    
    // Run systems for memory analysis
    for (int i = 0; i < 60; ++i) {
        update_all_systems(0.033f);
    }
    
    memory_benchmarker_->capture_memory_snapshot("peak_usage");
    
    // Clean up entities
    for (auto entity : temp_entities) {
        world_->destroy_entity(entity);
    }
    
    // Update systems to clean up memory
    for (int i = 0; i < 30; ++i) {
        update_all_systems(0.033f);
    }
    
    memory_benchmarker_->capture_memory_snapshot("after_cleanup");
    memory_benchmarker_->finalize_analysis();
    
    // Check memory usage analysis
    auto memory_report = memory_benchmarker_->generate_integration_report();
    EXPECT_FALSE(memory_report.empty());
    
    auto memory_stats = memory_benchmarker_->get_cross_system_statistics();
    EXPECT_GT(memory_stats.peak_memory_usage, 0);
    EXPECT_GT(memory_stats.allocations_per_system.size(), 1); // Multiple systems
    
    std::cout << "Memory Integration Analysis:\n"
              << "Peak Usage: " << memory_stats.peak_memory_usage << " bytes\n"
              << "Systems Analyzed: " << memory_stats.allocations_per_system.size() << "\n";
}

// =============================================================================
// Stress Testing Integration
// =============================================================================

TEST_F(CrossSystemIntegrationTest, HighLoadIntegrationStressTest) {
    // Test system integration under high load
    
    constexpr int stress_entity_count = 200;
    std::vector<Entity> stress_entities;
    
    // Create many entities with multiple systems
    for (int i = 0; i < stress_entity_count; ++i) {
        auto entity = world_->create_entity();
        
        Vec3 position = {
            static_cast<f32>((i % 20) * 5 - 50),
            static_cast<f32>((i / 20) * 3),
            static_cast<f32>((i % 7) * 8 - 24)
        };
        world_->add_component<Transform>(entity, position);
        
#ifdef ECSCOPE_ENABLE_PHYSICS
        Physics::RigidBody rigidbody;
        rigidbody.mass = 0.5f + (i % 10) * 0.1f;
        rigidbody.velocity = {
            static_cast<f32>((i % 7) - 3),
            0.0f,
            static_cast<f32>((i % 5) - 2)
        };\n        world_->add_component<Physics::RigidBody>(entity, rigidbody);\n        \n        if (i % 3 == 0) { // Only some entities have colliders\n            Physics::SphereCollider collider;\n            collider.radius = 0.3f + (i % 5) * 0.1f;\n            world_->add_component<Physics::SphereCollider>(entity, collider);\n        }\n#endif\n        \n        if (i % 2 == 0) { // Half the entities have audio\n            audio::AudioSource audio_source;\n            audio_source.volume = 0.1f + (i % 10) * 0.05f;\n            audio_source.enable_hrtf = (i % 4 == 0);\n            audio_source.enable_analysis = (i % 8 == 0);\n            world_->add_component<audio::AudioSource>(entity, audio_source);\n        }\n        \n        stress_entities.push_back(entity);\n    }\n    \n    // Stress test: Run for extended period with high entity count\n    auto start_time = std::chrono::high_resolution_clock::now();\n    \n    for (int frame = 0; frame < 300; ++frame) { // 10 seconds at 30 FPS\n        // Add some dynamic behavior\n        if (frame % 30 == 0) { // Every second\n            for (int i = 0; i < 10; ++i) {\n                auto entity = stress_entities[i + (frame / 30) * 10];\n                \n#ifdef ECSCOPE_ENABLE_PHYSICS\n                if (world_->has_component<Physics::RigidBody>(entity)) {\n                    auto& rigidbody = world_->get_component<Physics::RigidBody>(entity);\n                    rigidbody.add_impulse({(i % 3 - 1) * 5.0f, 2.0f, (i % 3 - 1) * 3.0f});\n                }\n#endif\n                \n                if (world_->has_component<audio::AudioSource>(entity)) {\n                    auto& audio_source = world_->get_component<audio::AudioSource>(entity);\n                    audio_source.volume = 0.2f + (frame % 20) * 0.01f;\n                }\n            }\n        }\n        \n        update_all_systems(0.033f);\n    }\n    \n    auto end_time = std::chrono::high_resolution_clock::now();\n    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);\n    \n    // Performance should still be reasonable even with high load\n    EXPECT_LT(duration.count(), 15000); // Should complete in less than 15 seconds\n    \n    double average_frame_time = duration.count() / 300.0;\n    EXPECT_LT(average_frame_time, 50.0); // Less than 50ms per frame on average\n    \n    std::cout << "Stress test completed with " << stress_entity_count << " entities\\n"\n              << "Total time: " << duration.count() << "ms\\n"\n              << "Average frame time: " << average_frame_time << "ms\\n";\n    \n    // Clean up\n    for (auto entity : stress_entities) {\n        world_->destroy_entity(entity);\n    }\n}\n\nTEST_F(CrossSystemIntegrationTest, RealTimeConstraintValidation) {\n    // Test that integrated systems can maintain real-time constraints\n    \n    constexpr int target_fps = 60;\n    constexpr f32 target_frame_time = 1000.0f / target_fps; // 16.67ms\n    constexpr int test_frames = 120; // 2 seconds\n    \n    std::vector<f32> frame_times;\n    frame_times.reserve(test_frames);\n    \n    // Create moderate entity load\n    for (int i = 0; i < 50; ++i) {\n        auto entity = create_test_audio_source({static_cast<f32>(i * 2), 0, 0});\n        \n#ifdef ECSCOPE_ENABLE_PHYSICS\n        Physics::RigidBody rigidbody;\n        rigidbody.mass = 1.0f;\n        rigidbody.velocity = {1.0f, 0.0f, 0.5f};\n        world_->add_component<Physics::RigidBody>(entity, rigidbody);\n        \n        Physics::SphereCollider collider;\n        collider.radius = 0.5f;\n        world_->add_component<Physics::SphereCollider>(entity, collider);\n#endif\n    }\n    \n    // Measure frame times\n    for (int frame = 0; frame < test_frames; ++frame) {\n        auto frame_start = std::chrono::high_resolution_clock::now();\n        \n        update_all_systems(1.0f / target_fps);\n        \n        auto frame_end = std::chrono::high_resolution_clock::now();\n        auto frame_duration = std::chrono::duration_cast<std::chrono::microseconds>(\n            frame_end - frame_start);\n        \n        f32 frame_time_ms = frame_duration.count() / 1000.0f;\n        frame_times.push_back(frame_time_ms);\n    }\n    \n    // Analyze frame time statistics\n    f32 average_frame_time = std::accumulate(frame_times.begin(), frame_times.end(), 0.0f) / frame_times.size();\n    f32 max_frame_time = *std::max_element(frame_times.begin(), frame_times.end());\n    \n    // Calculate 95th percentile\n    std::sort(frame_times.begin(), frame_times.end());\n    f32 percentile_95 = frame_times[static_cast<size_t>(frame_times.size() * 0.95)];\n    \n    // Real-time constraint validation\n    EXPECT_LT(average_frame_time, target_frame_time);\n    EXPECT_LT(percentile_95, target_frame_time * 1.5f); // Allow 50% variance for 95% of frames\n    EXPECT_LT(max_frame_time, target_frame_time * 2.0f); // Max frame time shouldn't exceed 2x target\n    \n    // Count frames that missed the target\n    int missed_frames = std::count_if(frame_times.begin(), frame_times.end(),\n        [target_frame_time](f32 time) { return time > target_frame_time; });\n    \n    f32 missed_frame_percentage = (static_cast<f32>(missed_frames) / test_frames) * 100.0f;\n    \n    // Should maintain target frame rate for majority of frames\n    EXPECT_LT(missed_frame_percentage, 10.0f); // Less than 10% missed frames\n    \n    std::cout << "Real-time performance analysis:\\n"\n              << "Average frame time: " << average_frame_time << "ms (target: " << target_frame_time << "ms)\\n"\n              << "95th percentile: " << percentile_95 << "ms\\n"\n              << "Max frame time: " << max_frame_time << "ms\\n"\n              << "Missed frames: " << missed_frame_percentage << "%\\n";\n}\n\n} // namespace ECScope::Testing"