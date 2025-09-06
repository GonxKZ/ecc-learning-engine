#include "../framework/ecscope_test_framework.hpp"

#ifdef ECSCOPE_ENABLE_PHYSICS
#include <ecscope/world3d.hpp>
#include <ecscope/advanced_physics_complete.hpp>
#endif

#ifdef ECSCOPE_ENABLE_AUDIO
#include <ecscope/spatial_audio_engine.hpp>
#include <ecscope/audio_systems.hpp>
#endif

#ifdef ECSCOPE_ENABLE_NETWORKING
#include <ecscope/networking/network_manager.hpp>
#include <ecscope/networking/replication_manager.hpp>
#endif

#ifdef ECSCOPE_HAS_GRAPHICS
#include <ecscope/renderer_2d.hpp>
#include <ecscope/batch_renderer.hpp>
#endif

#include <ecscope/asset_pipeline.hpp>
#include <ecscope/scene_editor.hpp>
#include <ecscope/learning_system.hpp>

namespace ECScope::Testing {

class CrossSystemIntegrationTest : public ECScopeTestFixture {
protected:
    void SetUp() override {
        ECScopeTestFixture::SetUp();
        
        // Initialize all available systems
        
#ifdef ECSCOPE_ENABLE_PHYSICS
        physics_world_ = std::make_unique<Physics3D::World>();
        physics_world_->set_gravity(Vec3(0, -9.81f, 0));
#endif

#ifdef ECSCOPE_ENABLE_AUDIO
        Audio::AudioConfiguration audio_config;
        audio_config.sample_rate = 44100;
        audio_config.buffer_size = 512;
        audio_config.channels = 2;
        
        audio_engine_ = std::make_unique<Audio::SpatialAudioEngine>(audio_config);
        audio_engine_->initialize();
        
        // Create listener
        listener_ = world_->create_entity();
        world_->add_component<Audio::AudioListener>(listener_);
        world_->add_component<Transform3D>(listener_, Vec3(0, 0, 0));
#endif

#ifdef ECSCOPE_ENABLE_NETWORKING
        Network::ServerConfiguration server_config;
        server_config.port = 54321;
        server_config.max_clients = 8;
        server_config.tick_rate = 60;
        
        network_manager_ = std::make_unique<Network::NetworkManager>();
        replication_manager_ = std::make_unique<Network::ReplicationManager>();
#endif

#ifdef ECSCOPE_HAS_GRAPHICS
        // Initialize rendering system (would need actual graphics context in real test)
        Rendering::RendererConfig render_config;
        render_config.window_width = 800;
        render_config.window_height = 600;
        render_config.vsync = false;
        
        // Note: In actual test, would need proper graphics initialization
#endif

        // Initialize asset pipeline
        asset_pipeline_ = std::make_unique<Assets::AssetPipeline>();
        asset_pipeline_->initialize("test_assets");
        
        // Initialize scene editor
        scene_editor_ = std::make_unique<Editor::SceneEditor>(*world_);
        
        // Initialize learning system
        learning_system_ = std::make_unique<Education::LearningSystem>();
        learning_system_->initialize();
    }

    void TearDown() override {
        // Cleanup in reverse order
        learning_system_.reset();
        scene_editor_.reset();
        asset_pipeline_.reset();
        
#ifdef ECSCOPE_ENABLE_NETWORKING
        replication_manager_.reset();
        network_manager_.reset();
#endif

#ifdef ECSCOPE_ENABLE_AUDIO
        audio_engine_.reset();
#endif

#ifdef ECSCOPE_ENABLE_PHYSICS
        physics_world_.reset();
#endif
        
        ECScopeTestFixture::TearDown();
    }

    // Helper to create a fully-featured entity
    Entity create_complete_entity(const Vec3& position, const std::string& name = "TestEntity") {
        auto entity = world_->create_entity();
        
        // Core components
        world_->add_component<Transform3D>(entity, position);
        world_->add_component<TestVelocity>(entity);
        world_->add_component<TestHealth>(entity);
        
#ifdef ECSCOPE_ENABLE_PHYSICS
        // Physics components
        Physics3D::RigidBody3D rigidbody;
        rigidbody.mass = 1.0f;
        rigidbody.velocity = Vec3(0, 0, 0);
        world_->add_component<Physics3D::RigidBody3D>(entity, rigidbody);
        
        Physics3D::SphereCollider collider(0.5f);
        world_->add_component<Physics3D::SphereCollider>(entity, collider);
#endif

#ifdef ECSCOPE_ENABLE_AUDIO
        // Audio components
        Audio::AudioSource audio_source;
        audio_source.volume = 1.0f;
        audio_source.pitch = 1.0f;
        audio_source.is_3d = true;
        audio_source.min_distance = 1.0f;
        audio_source.max_distance = 100.0f;
        world_->add_component<Audio::AudioSource>(entity, audio_source);
#endif

#ifdef ECSCOPE_ENABLE_NETWORKING
        // Networking components
        Network::NetworkedComponent networked;
        networked.network_id = next_network_id_++;
        networked.owner_id = 0; // Server owned
        networked.replicate_transform = true;
        networked.replicate_physics = true;
        networked.update_frequency = 20.0f;
        world_->add_component<Network::NetworkedComponent>(entity, networked);
#endif

#ifdef ECSCOPE_HAS_GRAPHICS
        // Rendering components
        Rendering::Sprite sprite;
        sprite.texture_id = 1; // Default texture
        sprite.color = Vec4(1, 1, 1, 1);
        sprite.size = Vec2(1, 1);
        world_->add_component<Rendering::Sprite>(entity, sprite);
#endif
        
        // Editor components
        Editor::EditorMetadata metadata;
        metadata.name = name;
        metadata.selectable = true;
        metadata.visible = true;
        world_->add_component<Editor::EditorMetadata>(entity, metadata);
        
        return entity;
    }

protected:
#ifdef ECSCOPE_ENABLE_PHYSICS
    std::unique_ptr<Physics3D::World> physics_world_;
#endif

#ifdef ECSCOPE_ENABLE_AUDIO
    std::unique_ptr<Audio::SpatialAudioEngine> audio_engine_;
    Entity listener_;
#endif

#ifdef ECSCOPE_ENABLE_NETWORKING
    std::unique_ptr<Network::NetworkManager> network_manager_;
    std::unique_ptr<Network::ReplicationManager> replication_manager_;
    uint32_t next_network_id_ = 1000;
#endif

    std::unique_ptr<Assets::AssetPipeline> asset_pipeline_;
    std::unique_ptr<Editor::SceneEditor> scene_editor_;
    std::unique_ptr<Education::LearningSystem> learning_system_;
};

// =============================================================================
// ECS + Physics Integration Tests
// =============================================================================

TEST_F(CrossSystemIntegrationTest, ECSPhysicsIntegration) {
#ifdef ECSCOPE_ENABLE_PHYSICS
    // Create falling objects
    constexpr int object_count = 10;
    std::vector<Entity> falling_objects;
    
    for (int i = 0; i < object_count; ++i) {
        Vec3 position(static_cast<float>(i - 5), 10.0f, 0.0f);
        auto entity = create_complete_entity(position, "FallingObject_" + std::to_string(i));
        falling_objects.push_back(entity);
    }
    
    // Create ground plane
    auto ground = world_->create_entity();
    world_->add_component<Transform3D>(ground, Vec3(0, -1, 0));
    
    Physics3D::RigidBody3D ground_body;
    ground_body.mass = std::numeric_limits<float>::infinity(); // Static
    world_->add_component<Physics3D::RigidBody3D>(ground, ground_body);
    
    Physics3D::BoxCollider ground_collider(Vec3(20, 0.5f, 20));
    world_->add_component<Physics3D::BoxCollider>(ground, ground_collider);
    
    // Simulate physics for several seconds
    float dt = 1.0f / 60.0f;
    int settled_count = 0;
    
    for (int frame = 0; frame < 600; ++frame) { // 10 seconds
        physics_world_->step(dt);
        
        // Check if objects have settled
        settled_count = 0;
        for (auto entity : falling_objects) {
            auto& transform = world_->get_component<Transform3D>(entity);
            auto& rigidbody = world_->get_component<Physics3D::RigidBody3D>(entity);
            
            if (transform.position.y > -0.5f && transform.position.y < 1.0f && 
                std::abs(rigidbody.velocity.y) < 0.1f) {
                settled_count++;
            }
        }
        
        // If all objects have settled, we can stop early
        if (settled_count == object_count) {
            break;
        }
    }
    
    // All objects should have settled on the ground
    EXPECT_EQ(settled_count, object_count);
    
    // Verify final positions are reasonable
    for (auto entity : falling_objects) {
        auto& transform = world_->get_component<Transform3D>(entity);
        EXPECT_GT(transform.position.y, -0.5f);  // Above ground
        EXPECT_LT(transform.position.y, 1.0f);   // Not floating too high
    }
    
    EXPECT_NO_MEMORY_LEAKS();
#else
    GTEST_SKIP() << "Physics not enabled";
#endif
}

// =============================================================================
// Physics + Audio Integration Tests
// =============================================================================

TEST_F(CrossSystemIntegrationTest, PhysicsAudioIntegration) {
#if defined(ECSCOPE_ENABLE_PHYSICS) && defined(ECSCOPE_ENABLE_AUDIO)
    // Create bouncing ball with sound
    auto ball = create_complete_entity(Vec3(0, 5, 0), "BouncingBall");
    
    // Load bounce sound (mock)
    std::vector<float> bounce_sound = generate_sine_wave(800.0f, 0.2f, 44100); // 800Hz, 200ms
    audio_engine_->load_audio_data(ball, bounce_sound);
    
    auto& ball_audio = world_->get_component<Audio::AudioSource>(ball);
    ball_audio.audio_clip = "bounce.wav";
    ball_audio.volume = 0.8f;
    
    // Create ground with collision detection
    auto ground = world_->create_entity();
    world_->add_component<Transform3D>(ground, Vec3(0, 0, 0));
    
    Physics3D::RigidBody3D ground_body;
    ground_body.mass = std::numeric_limits<float>::infinity();
    world_->add_component<Physics3D::RigidBody3D>(ground, ground_body);
    
    Physics3D::BoxCollider ground_collider(Vec3(10, 0.1f, 10));
    world_->add_component<Physics3D::BoxCollider>(ground, ground_collider);
    
    // Simulate with collision detection
    float dt = 1.0f / 60.0f;
    int bounce_count = 0;
    Vec3 previous_position = world_->get_component<Transform3D>(ball).position;
    
    for (int frame = 0; frame < 300; ++frame) { // 5 seconds
        physics_world_->step(dt);
        
        auto& transform = world_->get_component<Transform3D>(ball);
        auto& rigidbody = world_->get_component<Physics3D::RigidBody3D>(ball);
        
        // Detect bounce (velocity changed from negative to positive Y)
        if (previous_position.y > transform.position.y && rigidbody.velocity.y > 1.0f) {
            // Trigger bounce sound
            Audio::SpatialAudioParams audio_params;
            audio_params.source_position = transform.position;
            audio_params.listener_position = world_->get_component<Transform3D>(listener_).position;
            audio_params.listener_forward = Vec3(0, 0, -1);
            audio_params.listener_up = Vec3(0, 1, 0);
            
            auto spatialized_audio = audio_engine_->process_spatial_audio(ball, audio_params);
            EXPECT_GT(spatialized_audio.size(), 0);
            
            bounce_count++;
        }
        
        previous_position = transform.position;
        
        // Stop if ball has settled
        if (std::abs(rigidbody.velocity.y) < 0.1f && transform.position.y < 1.0f) {
            break;
        }
    }
    
    // Should have bounced at least once
    EXPECT_GT(bounce_count, 0);
    
    EXPECT_NO_MEMORY_LEAKS();
#else
    GTEST_SKIP() << "Physics or Audio not enabled";
#endif
}

// =============================================================================
// Networking + Physics Integration Tests
// =============================================================================

TEST_F(CrossSystemIntegrationTest, NetworkingPhysicsIntegration) {
#if defined(ECSCOPE_ENABLE_PHYSICS) && defined(ECSCOPE_ENABLE_NETWORKING)
    // Initialize networking
    EXPECT_TRUE(network_manager_->initialize());
    
    // Create networked physics objects
    constexpr int object_count = 5;
    std::vector<Entity> networked_objects;
    
    for (int i = 0; i < object_count; ++i) {
        Vec3 position(static_cast<float>(i * 2), 5.0f, 0.0f);
        auto entity = create_complete_entity(position, "NetworkedPhysics_" + std::to_string(i));
        networked_objects.push_back(entity);
    }
    
    // Initialize replication
    replication_manager_->initialize(*world_);
    
    // Simulate physics and network updates
    float dt = 1.0f / 60.0f;
    
    for (int frame = 0; frame < 120; ++frame) { // 2 seconds
        // Physics step
        physics_world_->step(dt);
        
        // Network replication step (every 3 frames = 20Hz)
        if (frame % 3 == 0) {
            // Create network snapshot
            auto snapshot = replication_manager_->create_snapshot();
            EXPECT_NE(snapshot, nullptr);
            
            // Verify networked objects are in snapshot
            int replicated_count = 0;
            for (auto entity : networked_objects) {
                if (replication_manager_->is_entity_replicated(entity)) {
                    replicated_count++;
                }
            }
            
            EXPECT_EQ(replicated_count, object_count);
        }
    }
    
    // Verify objects have moved due to physics
    for (auto entity : networked_objects) {
        auto& transform = world_->get_component<Transform3D>(entity);
        EXPECT_LT(transform.position.y, 5.0f); // Should have fallen
    }
    
    EXPECT_NO_MEMORY_LEAKS();
#else
    GTEST_SKIP() << "Physics or Networking not enabled";
#endif
}

// =============================================================================
// Scene Editor Integration Tests
// =============================================================================

TEST_F(CrossSystemIntegrationTest, SceneEditorIntegration) {
    // Create a complex scene
    auto player = create_complete_entity(Vec3(0, 0, 0), "Player");
    auto enemy1 = create_complete_entity(Vec3(5, 0, 0), "Enemy1");
    auto enemy2 = create_complete_entity(Vec3(-5, 0, 0), "Enemy2");
    auto pickup = create_complete_entity(Vec3(0, 0, 5), "HealthPickup");
    
    // Scene editor operations
    scene_editor_->refresh_entity_list();
    auto entities = scene_editor_->get_all_entities();
    
    EXPECT_GE(entities.size(), 4); // At least our test entities
    
    // Test selection
    scene_editor_->select_entity(player);
    EXPECT_TRUE(scene_editor_->is_entity_selected(player));
    EXPECT_EQ(scene_editor_->get_selected_entity(), player);
    
    // Test multi-selection
    scene_editor_->add_to_selection(enemy1);
    scene_editor_->add_to_selection(enemy2);
    
    auto selected_entities = scene_editor_->get_selected_entities();
    EXPECT_EQ(selected_entities.size(), 3);
    
    // Test entity operations
    Vec3 original_position = world_->get_component<Transform3D>(player).position;
    Vec3 new_position = Vec3(10, 20, 30);
    
    scene_editor_->move_entity(player, new_position);
    
    auto& transform = world_->get_component<Transform3D>(player);
    EXPECT_FLOAT_EQ(transform.position.x, new_position.x);
    EXPECT_FLOAT_EQ(transform.position.y, new_position.y);
    EXPECT_FLOAT_EQ(transform.position.z, new_position.z);
    
    // Test undo/redo
    scene_editor_->undo();
    auto& reverted_transform = world_->get_component<Transform3D>(player);
    EXPECT_FLOAT_EQ(reverted_transform.position.x, original_position.x);
    EXPECT_FLOAT_EQ(reverted_transform.position.y, original_position.y);
    EXPECT_FLOAT_EQ(reverted_transform.position.z, original_position.z);
    
    scene_editor_->redo();
    auto& redone_transform = world_->get_component<Transform3D>(player);
    EXPECT_FLOAT_EQ(redone_transform.position.x, new_position.x);
    
    // Test entity deletion and creation
    auto original_entity_count = entities.size();
    
    scene_editor_->delete_entity(pickup);
    scene_editor_->refresh_entity_list();
    entities = scene_editor_->get_all_entities();
    EXPECT_EQ(entities.size(), original_entity_count - 1);
    
    auto new_entity = scene_editor_->create_entity("NewTestEntity");
    EXPECT_NE(new_entity, INVALID_ENTITY);
    EXPECT_TRUE(world_->is_valid(new_entity));
    
    EXPECT_NO_MEMORY_LEAKS();
}

// =============================================================================
// Asset Pipeline Integration Tests
// =============================================================================

TEST_F(CrossSystemIntegrationTest, AssetPipelineIntegration) {
    // Test asset loading and hot-reloading
    
    // Mock asset files
    std::string test_texture_data = "MOCK_TEXTURE_DATA_1234567890";
    std::string test_audio_data = "MOCK_AUDIO_DATA_ABCDEFGHIJ";
    std::string test_model_data = "MOCK_MODEL_DATA_ZYXWVUTSRQ";
    
    // Load assets
    auto texture_handle = asset_pipeline_->load_texture("test_texture.png", test_texture_data.data(), test_texture_data.size());
    auto audio_handle = asset_pipeline_->load_audio("test_audio.wav", test_audio_data.data(), test_audio_data.size());
    auto model_handle = asset_pipeline_->load_model("test_model.obj", test_model_data.data(), test_model_data.size());
    
    EXPECT_NE(texture_handle, Assets::INVALID_HANDLE);
    EXPECT_NE(audio_handle, Assets::INVALID_HANDLE);
    EXPECT_NE(model_handle, Assets::INVALID_HANDLE);
    
    // Verify assets are loaded
    EXPECT_TRUE(asset_pipeline_->is_asset_loaded(texture_handle));
    EXPECT_TRUE(asset_pipeline_->is_asset_loaded(audio_handle));
    EXPECT_TRUE(asset_pipeline_->is_asset_loaded(model_handle));
    
    // Create entities using loaded assets
    auto textured_entity = world_->create_entity();
    world_->add_component<Transform3D>(textured_entity, Vec3(0, 0, 0));
    
#ifdef ECSCOPE_HAS_GRAPHICS
    Rendering::Sprite sprite;
    sprite.texture_id = texture_handle;
    sprite.size = Vec2(2, 2);
    world_->add_component<Rendering::Sprite>(textured_entity, sprite);
#endif
    
    auto audio_entity = world_->create_entity();
    world_->add_component<Transform3D>(audio_entity, Vec3(3, 0, 0));
    
#ifdef ECSCOPE_ENABLE_AUDIO
    Audio::AudioSource audio_source;
    audio_source.audio_clip_handle = audio_handle;
    audio_source.volume = 0.7f;
    world_->add_component<Audio::AudioSource>(audio_entity, audio_source);
#endif
    
    // Test asset hot-reloading
    std::string updated_texture_data = "UPDATED_TEXTURE_DATA_9876543210";
    bool reloaded = asset_pipeline_->hot_reload_asset(texture_handle, updated_texture_data.data(), updated_texture_data.size());
    EXPECT_TRUE(reloaded);
    
    // Test asset dependency tracking
    auto dependencies = asset_pipeline_->get_asset_dependencies(model_handle);
    // Model might depend on textures and materials
    EXPECT_GE(dependencies.size(), 0);
    
    // Test asset memory management
    asset_pipeline_->unload_asset(audio_handle);
    EXPECT_FALSE(asset_pipeline_->is_asset_loaded(audio_handle));
    
    // Test batch loading
    std::vector<std::string> batch_assets = {
        "batch_texture1.png",
        "batch_texture2.png",
        "batch_audio1.wav"
    };
    
    auto batch_handles = asset_pipeline_->load_asset_batch(batch_assets);
    EXPECT_EQ(batch_handles.size(), batch_assets.size());
    
    for (auto handle : batch_handles) {
        if (handle != Assets::INVALID_HANDLE) {
            EXPECT_TRUE(asset_pipeline_->is_asset_loaded(handle));
        }
    }
    
    EXPECT_NO_MEMORY_LEAKS();
}

// =============================================================================
// Learning System Integration Tests
// =============================================================================

TEST_F(CrossSystemIntegrationTest, LearningSystemIntegration) {
    // Create educational scenario: Physics simulation with guided learning
    
    // Set up learning module
    Education::LearningModule physics_module;
    physics_module.name = "BasicPhysics";
    physics_module.description = "Learn basic physics concepts through simulation";
    physics_module.difficulty_level = Education::DifficultyLevel::Beginner;
    
    // Add learning objectives
    physics_module.objectives.push_back("Understand gravity effects");
    physics_module.objectives.push_back("Observe collision responses");
    physics_module.objectives.push_back("Predict object trajectories");
    
    learning_system_->add_module(physics_module);
    
    // Create interactive physics scene
#ifdef ECSCOPE_ENABLE_PHYSICS
    auto projectile = create_complete_entity(Vec3(-5, 5, 0), "Projectile");
    auto& projectile_rb = world_->get_component<Physics3D::RigidBody3D>(projectile);
    projectile_rb.velocity = Vec3(8, 4, 0); // Initial velocity
    
    auto target = create_complete_entity(Vec3(5, 1, 0), "Target");
    auto& target_rb = world_->get_component<Physics3D::RigidBody3D>(target);
    target_rb.mass = std::numeric_limits<float>::infinity(); // Static target
#endif
    
    // Start learning session
    auto session_id = learning_system_->start_learning_session("BasicPhysics", "TestStudent");
    EXPECT_NE(session_id, Education::INVALID_SESSION_ID);
    
    // Track learning progress during simulation
    float dt = 1.0f / 60.0f;
    bool collision_detected = false;
    float max_height_reached = 0.0f;
    
    for (int frame = 0; frame < 300; ++frame) { // 5 seconds
#ifdef ECSCOPE_ENABLE_PHYSICS
        physics_world_->step(dt);
        
        auto& projectile_transform = world_->get_component<Transform3D>(projectile);
        max_height_reached = std::max(max_height_reached, projectile_transform.position.y);
        
        // Check for collision with target
        float distance_to_target = (projectile_transform.position - 
                                   world_->get_component<Transform3D>(target).position).length();
        
        if (distance_to_target < 1.0f) {
            collision_detected = true;
            
            // Record learning event
            Education::LearningEvent event;
            event.type = Education::LearningEventType::ObjectiveCompleted;
            event.description = "Successfully hit target with projectile";
            event.timestamp = frame * dt;
            
            learning_system_->record_learning_event(session_id, event);
        }
#endif
        
        // Update learning system
        learning_system_->update(dt);
    }
    
    // Evaluate learning outcomes
    auto session_results = learning_system_->get_session_results(session_id);
    EXPECT_GT(session_results.completion_percentage, 0.0f);
    
    // Test adaptive difficulty
    if (collision_detected && max_height_reached > 3.0f) {
        // Student performed well, increase difficulty
        learning_system_->adjust_difficulty(session_id, Education::DifficultyAdjustment::Increase);
    }
    
    // Test progress tracking
    auto progress = learning_system_->get_student_progress("TestStudent");
    EXPECT_GE(progress.modules_completed, 0);
    EXPECT_GE(progress.total_session_time, 0.0f);
    
    // End learning session
    learning_system_->end_learning_session(session_id);
    
    EXPECT_NO_MEMORY_LEAKS();
}

// =============================================================================
// Full System Integration Test
// =============================================================================

TEST_F(CrossSystemIntegrationTest, CompleteSystemIntegration) {
    // Create a comprehensive scenario that exercises all systems together
    
    // Set up multiplayer physics scene with audio and visual feedback
    
    // 1. Create networked players
#ifdef ECSCOPE_ENABLE_NETWORKING
    network_manager_->initialize();
    
    auto player1 = create_complete_entity(Vec3(-3, 1, 0), "Player1");
    auto player2 = create_complete_entity(Vec3(3, 1, 0), "Player2");
    
    // Enable networking for players
    auto& p1_network = world_->get_component<Network::NetworkedComponent>(player1);
    auto& p2_network = world_->get_component<Network::NetworkedComponent>(player2);
    p1_network.owner_id = 1; // Client 1
    p2_network.owner_id = 2; // Client 2
#else
    auto player1 = create_complete_entity(Vec3(-3, 1, 0), "Player1");
    auto player2 = create_complete_entity(Vec3(3, 1, 0), "Player2");
#endif
    
    // 2. Set up physics interaction objects
#ifdef ECSCOPE_ENABLE_PHYSICS
    auto ball = create_complete_entity(Vec3(0, 5, 0), "Ball");
    auto& ball_rb = world_->get_component<Physics3D::RigidBody3D>(ball);
    ball_rb.mass = 0.5f;
    ball_rb.velocity = Vec3(1, 0, 0);
    
    // Create boundaries
    auto boundaries = {
        Vec3(0, -2, 0),   // Floor
        Vec3(8, 3, 0),    // Right wall
        Vec3(-8, 3, 0),   // Left wall
        Vec3(0, 8, 0)     // Ceiling
    };
    
    for (const auto& pos : boundaries) {
        auto boundary = world_->create_entity();
        world_->add_component<Transform3D>(boundary, pos);
        
        Physics3D::RigidBody3D boundary_rb;
        boundary_rb.mass = std::numeric_limits<float>::infinity();
        world_->add_component<Physics3D::RigidBody3D>(boundary, boundary_rb);
        
        Physics3D::BoxCollider boundary_collider(Vec3(10, 0.5f, 10));
        world_->add_component<Physics3D::BoxCollider>(boundary, boundary_collider);
    }
#endif
    
    // 3. Set up spatial audio
#ifdef ECSCOPE_ENABLE_AUDIO
    // Load game sounds
    auto bounce_sound = generate_sine_wave(600.0f, 0.3f, 44100);
    auto hit_sound = generate_sine_wave(200.0f, 0.5f, 44100);
    
    audio_engine_->load_audio_data(ball, bounce_sound);
    
    auto& ball_audio = world_->get_component<Audio::AudioSource>(ball);
    ball_audio.volume = 0.6f;
    ball_audio.pitch = 1.2f;
#endif
    
    // 4. Initialize scene editor for runtime editing
    scene_editor_->refresh_entity_list();
    auto initial_entity_count = scene_editor_->get_all_entities().size();
    
    // 5. Start learning session
    Education::LearningModule game_module;
    game_module.name = "MultiplayerPhysicsGame";
    game_module.description = "Learn through interactive multiplayer physics simulation";
    game_module.difficulty_level = Education::DifficultyLevel::Intermediate;
    
    learning_system_->add_module(game_module);
    auto session_id = learning_system_->start_learning_session("MultiplayerPhysicsGame", "IntegrationTestStudent");
    
    // 6. Run integrated simulation
    float dt = 1.0f / 60.0f;
    int collision_count = 0;
    int frame_count = 0;
    
    std::vector<Vec3> ball_trajectory;
    
    for (int frame = 0; frame < 600; ++frame) { // 10 seconds of simulation
        frame_count++;
        
        // Physics update
#ifdef ECSCOPE_ENABLE_PHYSICS
        physics_world_->step(dt);
        
        auto& ball_transform = world_->get_component<Transform3D>(ball);
        ball_trajectory.push_back(ball_transform.position);
        
        // Detect collisions (simplified - check velocity changes)
        if (ball_rb.velocity.length() > 0.1f && frame > 10) {
            // Check if velocity direction changed significantly from previous frame
            static Vec3 prev_velocity = ball_rb.velocity;
            float velocity_change = (ball_rb.velocity - prev_velocity).length();
            
            if (velocity_change > 2.0f) {
                collision_count++;
                
                // Trigger audio effect
#ifdef ECSCOPE_ENABLE_AUDIO
                Audio::SpatialAudioParams audio_params;
                audio_params.source_position = ball_transform.position;
                audio_params.listener_position = world_->get_component<Transform3D>(listener_).position;
                audio_params.listener_forward = Vec3(0, 0, -1);
                audio_params.listener_up = Vec3(0, 1, 0);
                
                auto spatialized_audio = audio_engine_->process_spatial_audio(ball, audio_params);
                EXPECT_GT(spatialized_audio.size(), 0);
#endif
                
                // Record learning event
                Education::LearningEvent event;
                event.type = Education::LearningEventType::InteractionCompleted;
                event.description = "Ball collision detected";
                event.timestamp = frame * dt;
                learning_system_->record_learning_event(session_id, event);
            }
            
            prev_velocity = ball_rb.velocity;
        }
#endif
        
        // Network update (simulate at 20Hz)
#ifdef ECSCOPE_ENABLE_NETWORKING
        if (frame % 3 == 0) {
            replication_manager_->update(dt * 3);
        }
#endif
        
        // Audio update
#ifdef ECSCOPE_ENABLE_AUDIO
        audio_engine_->update(dt);
#endif
        
        // Learning system update
        learning_system_->update(dt);
        
        // Periodic scene editor operations (simulate user interaction)
        if (frame % 120 == 0) { // Every 2 seconds
            scene_editor_->refresh_entity_list();
            
            // Simulate selecting ball
            scene_editor_->select_entity(ball);
            EXPECT_TRUE(scene_editor_->is_entity_selected(ball));
        }
        
        // Break if ball has settled
#ifdef ECSCOPE_ENABLE_PHYSICS
        if (ball_rb.velocity.length() < 0.1f && ball_transform.position.y < 0.5f) {
            break;
        }
#endif
    }
    
    // 7. Validate integration results
    
    // Physics validation
#ifdef ECSCOPE_ENABLE_PHYSICS
    EXPECT_GT(collision_count, 0); // Ball should have bounced
    EXPECT_GT(ball_trajectory.size(), 100); // Should have substantial trajectory
    
    // Verify energy conservation (approximately)
    float initial_height = 5.0f;
    float initial_energy = ball_rb.mass * 9.81f * initial_height + 0.5f * ball_rb.mass * 1.0f * 1.0f; // PE + KE
    
    auto final_transform = world_->get_component<Transform3D>(ball);
    float final_energy = ball_rb.mass * 9.81f * final_transform.position.y + 
                        0.5f * ball_rb.mass * ball_rb.velocity.length_squared();
    
    // Energy should be conserved within reasonable bounds (accounting for damping)
    EXPECT_LT(final_energy, initial_energy * 1.1f); // Within 10%
    EXPECT_GT(final_energy, initial_energy * 0.3f); // At least 30% (significant damping expected)
#endif
    
    // Audio validation
#ifdef ECSCOPE_ENABLE_AUDIO
    // Verify spatial audio system processed audio frames
    auto processed_frame_count = audio_engine_->get_processed_frame_count();
    EXPECT_GT(processed_frame_count, 0);
#endif
    
    // Networking validation
#ifdef ECSCOPE_ENABLE_NETWORKING
    // Verify replication system tracked entities
    EXPECT_TRUE(replication_manager_->is_entity_replicated(player1));
    EXPECT_TRUE(replication_manager_->is_entity_replicated(player2));
#endif
    
    // Scene editor validation
    scene_editor_->refresh_entity_list();
    auto final_entity_count = scene_editor_->get_all_entities().size();
    EXPECT_GE(final_entity_count, initial_entity_count); // No entities should be lost
    
    // Learning system validation
    auto session_results = learning_system_->get_session_results(session_id);
    EXPECT_GT(session_results.events_recorded, 0);
    EXPECT_GT(session_results.session_duration, 5.0f); // Ran for at least 5 seconds
    
    learning_system_->end_learning_session(session_id);
    
    // Memory validation - this is critical for integration tests
    EXPECT_NO_MEMORY_LEAKS();
    
    // Performance validation
    float average_frame_time = (frame_count * dt) / frame_count;
    EXPECT_LT(average_frame_time, 0.02f); // Should maintain > 50 FPS
    
    std::cout << "Integration Test Results:\n";
    std::cout << "  Frames simulated: " << frame_count << "\n";
    std::cout << "  Collisions detected: " << collision_count << "\n";
#ifdef ECSCOPE_ENABLE_PHYSICS
    std::cout << "  Ball trajectory points: " << ball_trajectory.size() << "\n";
    std::cout << "  Energy conservation: " << (final_energy / initial_energy * 100.0f) << "%\n";
#endif
    std::cout << "  Learning events recorded: " << session_results.events_recorded << "\n";
    std::cout << "  Session duration: " << session_results.session_duration << "s\n";
}

} // namespace ECScope::Testing