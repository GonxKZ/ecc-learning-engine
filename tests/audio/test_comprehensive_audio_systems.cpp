#include "../framework/ecscope_test_framework.hpp"
#include <ecscope/audio_systems.hpp>
#include <ecscope/audio_components.hpp>
#include <ecscope/audio_processing_pipeline.hpp>
#include <ecscope/spatial_audio_engine.hpp>
#include <ecscope/audio_education_system.hpp>
#include <ecscope/audio_testing_framework.hpp>

#ifdef ECSCOPE_ENABLE_PHYSICS
#include <ecscope/physics_system.hpp>
#endif

#include <cmath>
#include <random>
#include <algorithm>

namespace ECScope::Testing {

using namespace ecscope::audio;
using namespace ecscope::audio::systems;
using namespace ecscope::audio::components;
using namespace ecscope::audio::processing;

class AudioSystemTest : public ECScopeTestFixture {
protected:
    void SetUp() override {
        ECScopeTestFixture::SetUp();
        
        // Initialize audio memory tracker
        audio_memory_tracker_ = std::make_unique<MemoryTracker>("AudioSystemTest");
        audio_memory_tracker_->start_tracking();
        
        // Initialize audio testing framework
        audio_test_framework_ = std::make_unique<AudioTestingFramework>();
        audio_test_framework_->initialize();
        
        // Create mock audio device for testing
        mock_audio_device_ = audio_test_framework_->create_mock_device();
        ASSERT_NE(mock_audio_device_, nullptr);
        
        // Initialize spatial audio engine
        SpatialAudioEngineConfig audio_config;
        audio_config.sample_rate = 44100;
        audio_config.buffer_size = 512;
        audio_config.max_sources = 64;
        audio_config.enable_hrtf = true;
        audio_config.enable_reverb = true;
        audio_config.audio_device = mock_audio_device_.get();
        
        spatial_audio_engine_ = std::make_unique<SpatialAudioEngine>(audio_config);
        
        // Initialize audio systems
        spatial_audio_system_ = std::make_unique<SpatialAudioSystem>(
            audio_memory_tracker_.get(), memory::AllocationCategory::Audio_Processing);
        
        audio_listener_system_ = std::make_unique<AudioListenerSystem>(
            audio_memory_tracker_.get());
        
        audio_environment_system_ = std::make_unique<AudioEnvironmentSystem>(
            audio_memory_tracker_.get());
        
        audio_analysis_system_ = std::make_unique<AudioAnalysisSystem>(
            audio_memory_tracker_.get());
        
        // Register audio systems with world
        world_->add_system(spatial_audio_system_.get());
        world_->add_system(audio_listener_system_.get());
        world_->add_system(audio_environment_system_.get());
        world_->add_system(audio_analysis_system_.get());
        
        // Create test audio data
        create_test_audio_data();
        
#ifdef ECSCOPE_ENABLE_PHYSICS
        // Initialize physics system for audio integration tests
        physics_system_ = std::make_unique<Physics::PhysicsSystem>();
        world_->add_system(physics_system_.get());
#endif
    }
    
    void TearDown() override {
        // Clean up audio systems
        if (world_) {
            world_->remove_all_systems();
        }
        
        // Clean up audio engine and framework
        spatial_audio_engine_.reset();
        audio_test_framework_.reset();
        mock_audio_device_.reset();
        
        // Clean up systems
        audio_analysis_system_.reset();
        audio_environment_system_.reset();
        audio_listener_system_.reset();
        spatial_audio_system_.reset();
        
#ifdef ECSCOPE_ENABLE_PHYSICS
        physics_system_.reset();
#endif
        
        // Check for audio memory leaks
        if (audio_memory_tracker_) {
            audio_memory_tracker_->stop_tracking();
            EXPECT_EQ(audio_memory_tracker_->get_allocation_count(),
                     audio_memory_tracker_->get_deallocation_count());
        }
        
        audio_memory_tracker_.reset();
        
        ECScopeTestFixture::TearDown();
    }
    
    void create_test_audio_data() {
        // Create test audio buffers
        constexpr u32 sample_rate = 44100;
        constexpr u32 duration_ms = 1000; // 1 second
        constexpr u32 sample_count = (sample_rate * duration_ms) / 1000;
        
        // Sine wave test tone (440 Hz - A4)
        test_sine_wave_.resize(sample_count);
        for (u32 i = 0; i < sample_count; ++i) {
            f32 time = static_cast<f32>(i) / sample_rate;
            test_sine_wave_[i] = std::sin(2.0f * Math::PI * 440.0f * time) * 0.5f;
        }
        
        // White noise test signal
        test_white_noise_.resize(sample_count);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<f32> dist(-1.0f, 1.0f);
        
        for (u32 i = 0; i < sample_count; ++i) {
            test_white_noise_[i] = dist(gen) * 0.3f;
        }
        
        // Impulse response for testing reverb
        test_impulse_response_.resize(sample_count / 4); // 250ms impulse response
        for (u32 i = 0; i < test_impulse_response_.size(); ++i) {
            f32 decay = std::exp(-static_cast<f32>(i) / (sample_rate * 0.1f)); // 100ms decay
            test_impulse_response_[i] = decay * (dist(gen) * 0.1f);
        }
    }
    
    Entity create_test_audio_source(const Vec3& position, AudioSourceType type = AudioSourceType::Point) {
        auto entity = world_->create_entity();
        
        // Add transform component
        world_->add_component<Transform>(entity, position);
        
        // Add audio source component
        AudioSource audio_source;
        audio_source.type = type;
        audio_source.volume = 1.0f;
        audio_source.pitch = 1.0f;
        audio_source.is_looping = false;
        audio_source.is_playing = true;
        audio_source.distance_attenuation = AudioDistanceModel::InverseSquare;
        audio_source.max_distance = 100.0f;
        audio_source.reference_distance = 1.0f;
        world_->add_component<AudioSource>(entity, audio_source);
        
        // Add audio buffer with test data
        AudioBuffer audio_buffer;
        audio_buffer.sample_rate = 44100;
        audio_buffer.channel_count = 1;
        audio_buffer.sample_format = AudioSampleFormat::Float32;
        audio_buffer.samples = test_sine_wave_;
        world_->add_component<AudioBuffer>(entity, audio_buffer);
        
        return entity;
    }
    
    Entity create_test_audio_listener(const Vec3& position) {
        auto entity = world_->create_entity();
        
        // Add transform component
        world_->add_component<Transform>(entity, position);
        
        // Add audio listener component
        AudioListener listener;
        listener.is_active = true;
        listener.gain = 1.0f;
        listener.orientation = {0.0f, 0.0f, -1.0f}; // Facing forward
        listener.up_vector = {0.0f, 1.0f, 0.0f}; // Up is positive Y
        world_->add_component<AudioListener>(entity, listener);
        
        return entity;
    }

protected:
    std::unique_ptr<MemoryTracker> audio_memory_tracker_;
    std::unique_ptr<AudioTestingFramework> audio_test_framework_;
    std::unique_ptr<AudioDevice> mock_audio_device_;
    std::unique_ptr<SpatialAudioEngine> spatial_audio_engine_;
    
    // Audio systems
    std::unique_ptr<SpatialAudioSystem> spatial_audio_system_;
    std::unique_ptr<AudioListenerSystem> audio_listener_system_;
    std::unique_ptr<AudioEnvironmentSystem> audio_environment_system_;
    std::unique_ptr<AudioAnalysisSystem> audio_analysis_system_;
    
#ifdef ECSCOPE_ENABLE_PHYSICS
    std::unique_ptr<Physics::PhysicsSystem> physics_system_;
#endif
    
    // Test audio data
    std::vector<f32> test_sine_wave_;
    std::vector<f32> test_white_noise_;
    std::vector<f32> test_impulse_response_;
};

// =============================================================================
// Audio System Base Tests
// =============================================================================

TEST_F(AudioSystemTest, AudioSystemBaseInitialization) {
    EXPECT_NE(spatial_audio_system_, nullptr);
    EXPECT_NE(audio_listener_system_, nullptr);
    EXPECT_NE(audio_environment_system_, nullptr);
    EXPECT_NE(audio_analysis_system_, nullptr);
    
    // Test educational interface
    EXPECT_FALSE(spatial_audio_system_->get_system_description().empty());
    EXPECT_FALSE(spatial_audio_system_->get_key_concepts().empty());
    EXPECT_GT(spatial_audio_system_->get_educational_value_score(), 0.0f);
    
    // Test performance metrics initialization
    auto performance = spatial_audio_system_->get_performance_metrics();
    EXPECT_EQ(performance.average_update_time_ms, 0.0);
    EXPECT_EQ(performance.updates_per_second, 0.0f);
    EXPECT_EQ(performance.average_entities_processed, 0.0f);
}

TEST_F(AudioSystemTest, AudioSystemPerformanceTracking) {
    // Create some test entities
    auto source1 = create_test_audio_source({0.0f, 0.0f, 0.0f});
    auto source2 = create_test_audio_source({10.0f, 0.0f, 0.0f});
    auto listener = create_test_audio_listener({5.0f, 0.0f, 0.0f});
    
    // Reset performance counters
    spatial_audio_system_->reset_performance_counters();
    
    // Run several update cycles
    for (int i = 0; i < 10; ++i) {
        spatial_audio_system_->update(0.016f);
    }
    
    // Check performance metrics
    auto performance = spatial_audio_system_->get_performance_metrics();
    EXPECT_GT(performance.average_update_time_ms, 0.0);
    EXPECT_GT(performance.updates_per_second, 0.0f);
    
    // Performance should be reasonable (less than 1ms per update for simple scene)
    EXPECT_LT(performance.average_update_time_ms, 1.0);
}

// =============================================================================
// Spatial Audio System Tests
// =============================================================================

TEST_F(AudioSystemTest, SpatialAudioBasicFunctionality) {
    // Create audio source and listener
    auto source = create_test_audio_source({0.0f, 0.0f, 0.0f});
    auto listener = create_test_audio_listener({5.0f, 0.0f, 0.0f});
    
    // Update spatial audio system
    spatial_audio_system_->update(0.016f);
    
    // Check that audio source was processed
    auto& audio_source = world_->get_component<AudioSource>(source);
    EXPECT_TRUE(audio_source.is_playing);
    
    // Volume should be attenuated based on distance
    EXPECT_LT(audio_source.effective_volume, 1.0f);
    EXPECT_GT(audio_source.effective_volume, 0.0f);
}

TEST_F(AudioSystemTest, DistanceAttenuationCalculation) {
    auto listener = create_test_audio_listener({0.0f, 0.0f, 0.0f});
    
    // Create sources at different distances
    auto near_source = create_test_audio_source({1.0f, 0.0f, 0.0f});
    auto far_source = create_test_audio_source({10.0f, 0.0f, 0.0f});
    
    spatial_audio_system_->update(0.016f);
    
    auto& near_audio = world_->get_component<AudioSource>(near_source);
    auto& far_audio = world_->get_component<AudioSource>(far_source);
    
    // Near source should be louder than far source
    EXPECT_GT(near_audio.effective_volume, far_audio.effective_volume);
    
    // Both should be audible but attenuated
    EXPECT_GT(near_audio.effective_volume, 0.0f);
    EXPECT_GT(far_audio.effective_volume, 0.0f);
    EXPECT_LT(near_audio.effective_volume, 1.0f);
    EXPECT_LT(far_audio.effective_volume, 1.0f);
}

TEST_F(AudioSystemTest, DirectionalAudioSources) {
    auto listener = create_test_audio_listener({0.0f, 0.0f, 5.0f});
    
    // Create directional audio source
    auto source = create_test_audio_source({0.0f, 0.0f, 0.0f}, AudioSourceType::Directional);
    auto& audio_source = world_->get_component<AudioSource>(source);
    audio_source.cone_inner_angle = Math::PI * 0.25f; // 45 degrees
    audio_source.cone_outer_angle = Math::PI * 0.5f;  // 90 degrees
    audio_source.cone_outer_gain = 0.5f;
    
    // Set source direction (facing positive Z)
    audio_source.direction = {0.0f, 0.0f, 1.0f};
    
    spatial_audio_system_->update(0.016f);
    
    // Listener is in front of the source, should receive full volume
    EXPECT_GT(audio_source.effective_volume, 0.5f);
    
    // Move listener to the side (outside cone)
    auto& listener_transform = world_->get_component<Transform>(listener);
    listener_transform.position = {5.0f, 0.0f, 0.0f};
    
    spatial_audio_system_->update(0.016f);
    
    // Volume should be reduced due to directional cone
    EXPECT_LT(audio_source.effective_volume, 0.5f);
}

TEST_F(AudioSystemTest, AudioSourceCulling) {
    auto listener = create_test_audio_listener({0.0f, 0.0f, 0.0f});
    
    // Create sources at various distances
    std::vector<Entity> sources;
    for (int i = 1; i <= 10; ++i) {
        auto source = create_test_audio_source({static_cast<f32>(i * 20), 0.0f, 0.0f});
        sources.push_back(source);
    }
    
    spatial_audio_system_->update(0.016f);
    
    // Check that distant sources are culled (not processed or have zero volume)
    int audible_sources = 0;
    for (auto source : sources) {
        auto& audio_source = world_->get_component<AudioSource>(source);
        if (audio_source.effective_volume > 0.001f) {
            audible_sources++;
        }
    }
    
    // Not all sources should be audible due to culling
    EXPECT_LT(audible_sources, sources.size());
    EXPECT_GT(audible_sources, 0);
}

// =============================================================================
// HRTF Processing Tests
// =============================================================================

TEST_F(AudioSystemTest, HRTFProcessing) {
    auto listener = create_test_audio_listener({0.0f, 0.0f, 0.0f});
    
    // Create audio source to the right of listener
    auto source = create_test_audio_source({10.0f, 0.0f, 0.0f});
    
    // Enable HRTF processing
    auto& audio_source = world_->get_component<AudioSource>(source);
    audio_source.enable_hrtf = true;
    
    spatial_audio_system_->update(0.016f);
    
    // Check that HRTF data was computed
    EXPECT_TRUE(audio_source.hrtf_data.left_delay >= 0.0f);
    EXPECT_TRUE(audio_source.hrtf_data.right_delay >= 0.0f);
    EXPECT_TRUE(audio_source.hrtf_data.left_gain >= 0.0f);
    EXPECT_TRUE(audio_source.hrtf_data.right_gain >= 0.0f);
    
    // For a source to the right, right channel should be stronger
    EXPECT_GT(audio_source.hrtf_data.right_gain, audio_source.hrtf_data.left_gain);
}

TEST_F(AudioSystemTest, HRTFSpatialAccuracy) {
    auto listener = create_test_audio_listener({0.0f, 0.0f, 0.0f});
    
    // Test sources at different angles around the listener
    struct TestPosition {
        Vec3 position;
        std::string description;
        bool expect_right_stronger;
    };
    
    std::vector<TestPosition> test_positions = {
        {{5.0f, 0.0f, 0.0f}, "Right", true},
        {{-5.0f, 0.0f, 0.0f}, "Left", false},
        {{0.0f, 0.0f, 5.0f}, "Front", false}, // Should be roughly equal
        {{0.0f, 0.0f, -5.0f}, "Back", false}  // Should be roughly equal
    };
    
    for (const auto& test_pos : test_positions) {
        auto source = create_test_audio_source(test_pos.position);
        auto& audio_source = world_->get_component<AudioSource>(source);
        audio_source.enable_hrtf = true;
        
        spatial_audio_system_->update(0.016f);
        
        if (test_pos.expect_right_stronger) {
            EXPECT_GT(audio_source.hrtf_data.right_gain, audio_source.hrtf_data.left_gain)
                << "Failed for position: " << test_pos.description;
        } else if (test_pos.description == "Left") {
            EXPECT_GT(audio_source.hrtf_data.left_gain, audio_source.hrtf_data.right_gain)
                << "Failed for position: " << test_pos.description;
        }
        
        // Clean up for next iteration
        world_->destroy_entity(source);
    }
}

// =============================================================================
// Audio Environment System Tests
// =============================================================================

TEST_F(AudioSystemTest, AudioEnvironmentBasicFunctionality) {
    // Create environment zone
    auto environment_entity = world_->create_entity();
    world_->add_component<Transform>(environment_entity, Vec3{0.0f, 0.0f, 0.0f});
    
    AudioEnvironment environment;
    environment.reverb_preset = ReverbPreset::Hall;
    environment.ambient_volume = 0.3f;
    environment.zone_radius = 50.0f;
    environment.enable_reverb = true;
    environment.enable_occlusion = true;
    world_->add_component<AudioEnvironment>(environment_entity, environment);
    
    // Create listener and source within environment
    auto listener = create_test_audio_listener({0.0f, 0.0f, 0.0f});
    auto source = create_test_audio_source({10.0f, 0.0f, 0.0f});
    
    audio_environment_system_->update(0.016f);
    
    // Check that environment affects audio source
    auto& audio_source = world_->get_component<AudioSource>(source);
    EXPECT_TRUE(audio_source.environment_data.in_reverb_zone);
    EXPECT_GT(audio_source.environment_data.reverb_strength, 0.0f);
    EXPECT_EQ(audio_source.environment_data.reverb_preset, ReverbPreset::Hall);
}

TEST_F(AudioSystemTest, ReverbProcessing) {
    // Create reverb environment
    auto environment_entity = world_->create_entity();
    world_->add_component<Transform>(environment_entity, Vec3{0.0f, 0.0f, 0.0f});
    
    AudioEnvironment environment;
    environment.reverb_preset = ReverbPreset::Cathedral;
    environment.zone_radius = 100.0f;
    environment.enable_reverb = true;
    environment.reverb_decay_time = 2.0f;
    environment.reverb_wet_gain = 0.4f;
    environment.reverb_dry_gain = 0.6f;
    world_->add_component<AudioEnvironment>(environment_entity, environment);
    
    auto listener = create_test_audio_listener({0.0f, 0.0f, 0.0f});
    auto source = create_test_audio_source({20.0f, 0.0f, 0.0f});
    
    audio_environment_system_->update(0.016f);
    
    auto& audio_source = world_->get_component<AudioSource>(source);
    
    // Check reverb parameters are applied
    EXPECT_TRUE(audio_source.environment_data.in_reverb_zone);
    EXPECT_FLOAT_EQ(audio_source.environment_data.reverb_decay_time, 2.0f);
    EXPECT_FLOAT_EQ(audio_source.environment_data.reverb_wet_gain, 0.4f);
    EXPECT_FLOAT_EQ(audio_source.environment_data.reverb_dry_gain, 0.6f);
}

TEST_F(AudioSystemTest, AmbientAudioProcessing) {
    // Create environment with ambient audio
    auto environment_entity = world_->create_entity();
    world_->add_component<Transform>(environment_entity, Vec3{0.0f, 0.0f, 0.0f});
    
    AudioEnvironment environment;
    environment.ambient_volume = 0.5f;
    environment.zone_radius = 30.0f;
    environment.enable_ambient = true;
    world_->add_component<AudioEnvironment>(environment_entity, environment);
    
    // Add ambient audio buffer
    AudioBuffer ambient_buffer;
    ambient_buffer.samples = test_white_noise_;
    ambient_buffer.sample_rate = 44100;
    ambient_buffer.channel_count = 2;
    ambient_buffer.is_ambient = true;
    world_->add_component<AudioBuffer>(environment_entity, ambient_buffer);
    
    auto listener = create_test_audio_listener({0.0f, 0.0f, 0.0f});
    
    audio_environment_system_->update(0.016f);
    
    // Check that ambient audio is processed
    auto& environment_comp = world_->get_component<AudioEnvironment>(environment_entity);
    EXPECT_TRUE(environment_comp.ambient_active);
    EXPECT_FLOAT_EQ(environment_comp.current_ambient_volume, 0.5f);
}

// =============================================================================
// Audio Analysis System Tests
// =============================================================================

TEST_F(AudioSystemTest, AudioAnalysisSystemInitialization) {
    // Test that analysis system initializes correctly
    auto performance = audio_analysis_system_->get_performance_metrics();
    EXPECT_GE(performance.educational_value_score, 0.0f);
    
    // Check educational content
    auto concepts = audio_analysis_system_->get_key_concepts();
    EXPECT_FALSE(concepts.empty());
    
    auto description = audio_analysis_system_->get_system_description();
    EXPECT_FALSE(description.empty());
    
    // Generate educational summary
    auto summary = audio_analysis_system_->generate_educational_summary();
    EXPECT_FALSE(summary.empty());
    EXPECT_GT(summary.length(), 50); // Should be substantial
}

TEST_F(AudioSystemTest, RealTimeAudioAnalysis) {
    // Create audio source with analyzable content
    auto source = create_test_audio_source({0.0f, 0.0f, 0.0f});
    auto listener = create_test_audio_listener({5.0f, 0.0f, 0.0f});
    
    // Enable analysis for the audio source
    auto& audio_source = world_->get_component<AudioSource>(source);
    audio_source.enable_analysis = true;
    
    // Run analysis for several frames
    for (int i = 0; i < 10; ++i) {
        spatial_audio_system_->update(0.016f);
        audio_analysis_system_->update(0.016f);
    }
    
    // Check that analysis data was generated
    EXPECT_TRUE(audio_source.analysis_data.has_analysis);
    EXPECT_GT(audio_source.analysis_data.rms_level, 0.0f);
    EXPECT_GE(audio_source.analysis_data.peak_level, 0.0f);
    EXPECT_GE(audio_source.analysis_data.frequency_centroid, 0.0f);
    
    // For a sine wave at 440Hz, centroid should be around that frequency
    EXPECT_GT(audio_source.analysis_data.frequency_centroid, 300.0f);
    EXPECT_LT(audio_source.analysis_data.frequency_centroid, 600.0f);
}

TEST_F(AudioSystemTest, FrequencyAnalysis) {
    auto source = create_test_audio_source({0.0f, 0.0f, 0.0f});
    auto listener = create_test_audio_listener({1.0f, 0.0f, 0.0f});
    
    auto& audio_source = world_->get_component<AudioSource>(source);
    audio_source.enable_analysis = true;
    audio_source.enable_fft_analysis = true;
    
    // Update systems to perform analysis
    for (int i = 0; i < 20; ++i) {
        spatial_audio_system_->update(0.016f);
        audio_analysis_system_->update(0.016f);
    }
    
    // Check FFT analysis results
    EXPECT_TRUE(audio_source.analysis_data.has_fft_data);
    EXPECT_FALSE(audio_source.analysis_data.frequency_spectrum.empty());
    
    // For a 440Hz sine wave, there should be a peak around that frequency
    // Find the bin corresponding to 440Hz
    u32 sample_rate = 44100;
    u32 fft_size = audio_source.analysis_data.frequency_spectrum.size() * 2;
    f32 bin_frequency = static_cast<f32>(sample_rate) / fft_size;
    u32 expected_bin = static_cast<u32>(440.0f / bin_frequency);
    
    if (expected_bin < audio_source.analysis_data.frequency_spectrum.size()) {
        f32 peak_magnitude = audio_source.analysis_data.frequency_spectrum[expected_bin];
        EXPECT_GT(peak_magnitude, 0.1f); // Should have significant magnitude at 440Hz
    }
}

// =============================================================================
// Audio Memory System Tests
// =============================================================================

TEST_F(AudioSystemTest, AudioMemoryManagement) {
    // Test audio buffer memory management
    constexpr int buffer_count = 10;
    std::vector<Entity> audio_entities;
    
    for (int i = 0; i < buffer_count; ++i) {
        auto entity = create_test_audio_source({static_cast<f32>(i), 0.0f, 0.0f});
        audio_entities.push_back(entity);
    }
    
    // Record initial memory usage
    auto initial_stats = audio_memory_tracker_->get_stats();
    
    // Update systems to allocate audio processing memory
    for (int i = 0; i < 5; ++i) {
        spatial_audio_system_->update(0.016f);
        audio_analysis_system_->update(0.016f);
    }
    
    auto mid_stats = audio_memory_tracker_->get_stats();
    EXPECT_GT(mid_stats.current_memory_usage, initial_stats.current_memory_usage);
    
    // Clean up entities
    for (auto entity : audio_entities) {
        world_->destroy_entity(entity);
    }
    
    // Update systems to clean up memory
    for (int i = 0; i < 3; ++i) {
        spatial_audio_system_->update(0.016f);
        audio_analysis_system_->update(0.016f);
    }
    
    // Memory usage should decrease after cleanup
    auto final_stats = audio_memory_tracker_->get_stats();
    // Note: Exact cleanup behavior depends on implementation
    EXPECT_LE(final_stats.current_memory_usage, mid_stats.current_memory_usage);
}

TEST_F(AudioSystemTest, AudioBufferPooling) {
    // Test that audio buffers are properly pooled and reused
    AudioBufferPool buffer_pool(1024, 16); // 16 buffers of 1024 samples each
    
    std::vector<AudioBufferPool::BufferHandle> allocated_buffers;
    
    // Allocate buffers up to pool capacity
    for (int i = 0; i < 16; ++i) {
        auto buffer = buffer_pool.acquire_buffer();
        EXPECT_NE(buffer, nullptr);
        allocated_buffers.push_back(buffer);
    }
    
    // Pool should be exhausted
    auto buffer = buffer_pool.acquire_buffer();
    EXPECT_EQ(buffer, nullptr);
    
    // Release half the buffers
    for (int i = 0; i < 8; ++i) {
        buffer_pool.release_buffer(allocated_buffers[i]);
    }
    allocated_buffers.erase(allocated_buffers.begin(), allocated_buffers.begin() + 8);
    
    // Should be able to allocate again
    for (int i = 0; i < 8; ++i) {
        buffer = buffer_pool.acquire_buffer();
        EXPECT_NE(buffer, nullptr);
        allocated_buffers.push_back(buffer);
    }
    
    // Clean up remaining buffers
    for (auto handle : allocated_buffers) {
        buffer_pool.release_buffer(handle);
    }
}

// =============================================================================
// Physics Integration Tests
// =============================================================================

#ifdef ECSCOPE_ENABLE_PHYSICS
TEST_F(AudioSystemTest, AudioPhysicsIntegrationBasic) {
    // Create audio source with physics body
    auto source_entity = create_test_audio_source({0.0f, 0.0f, 0.0f});
    
    // Add physics components for Doppler effect
    Physics::RigidBody physics_body;
    physics_body.velocity = {10.0f, 0.0f, 0.0f}; // Moving right at 10 m/s
    physics_body.mass = 1.0f;
    world_->add_component<Physics::RigidBody>(source_entity, physics_body);
    
    auto listener = create_test_audio_listener({50.0f, 0.0f, 0.0f});
    
    // Update physics and audio systems
    for (int i = 0; i < 10; ++i) {
        physics_system_->update(0.016f);
        spatial_audio_system_->update(0.016f);
    }
    
    // Check that Doppler effect is applied
    auto& audio_source = world_->get_component<AudioSource>(source_entity);
    EXPECT_TRUE(audio_source.doppler_data.has_doppler_effect);
    EXPECT_NE(audio_source.doppler_data.pitch_shift, 1.0f); // Should have pitch shift
    
    // Source moving towards listener should have higher pitch
    EXPECT_GT(audio_source.doppler_data.pitch_shift, 1.0f);
}

TEST_F(AudioSystemTest, AudioOcclusionWithPhysics) {
    // Create audio source and listener with obstacle between them
    auto source = create_test_audio_source({-10.0f, 0.0f, 0.0f});
    auto listener = create_test_audio_listener({10.0f, 0.0f, 0.0f});
    
    // Create obstacle (wall) between source and listener
    auto obstacle = world_->create_entity();
    world_->add_component<Transform>(obstacle, Vec3{0.0f, 0.0f, 0.0f});
    
    Physics::BoxCollider wall_collider;
    wall_collider.half_extents = {1.0f, 5.0f, 5.0f}; // Thin, tall, wide wall
    world_->add_component<Physics::BoxCollider>(obstacle, wall_collider);
    
    Physics::RigidBody wall_body;
    wall_body.is_static = true;
    wall_body.mass = std::numeric_limits<f32>::infinity();
    world_->add_component<Physics::RigidBody>(obstacle, wall_body);
    
    // Enable occlusion processing
    auto& audio_source = world_->get_component<AudioSource>(source);
    audio_source.enable_occlusion = true;
    
    // Update systems
    for (int i = 0; i < 5; ++i) {
        physics_system_->update(0.016f);
        spatial_audio_system_->update(0.016f);
    }
    
    // Check that occlusion is detected and applied
    EXPECT_TRUE(audio_source.occlusion_data.is_occluded);
    EXPECT_LT(audio_source.occlusion_data.occlusion_factor, 1.0f);
    EXPECT_GT(audio_source.occlusion_data.occlusion_factor, 0.0f);
    
    // Effective volume should be reduced due to occlusion
    EXPECT_LT(audio_source.effective_volume, audio_source.volume);
}
#endif

// =============================================================================
// Performance Tests
// =============================================================================

TEST_F(AudioSystemTest, AudioSystemScalability) {
    auto listener = create_test_audio_listener({0.0f, 0.0f, 0.0f});
    
    // Create many audio sources
    constexpr int source_count = 100;
    std::vector<Entity> sources;
    
    for (int i = 0; i < source_count; ++i) {
        f32 angle = (2.0f * Math::PI * i) / source_count;
        f32 radius = 20.0f + (i % 10) * 5.0f; // Varying distances
        
        Vec3 position = {
            radius * std::cos(angle),
            (i % 5) - 2.0f, // Varying heights
            radius * std::sin(angle)
        };
        
        auto source = create_test_audio_source(position);
        sources.push_back(source);
    }
    
    // Measure performance with many sources
    spatial_audio_system_->reset_performance_counters();
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Run for several frames
    for (int i = 0; i < 60; ++i) {
        spatial_audio_system_->update(0.016f);
        audio_analysis_system_->update(0.016f);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    // Check performance metrics
    auto performance = spatial_audio_system_->get_performance_metrics();
    EXPECT_GT(performance.average_entities_processed, 0.0f);
    EXPECT_LT(performance.average_update_time_ms, 10.0); // Should be under 10ms per frame
    
    // Total time for 60 frames should be reasonable
    EXPECT_LT(duration.count(), 1000000); // Less than 1 second for 60 frames
    
    std::cout << "Processed " << source_count << " audio sources in " 
              << duration.count() << "us (avg: " << performance.average_update_time_ms 
              << "ms per frame)" << std::endl;
}

TEST_F(AudioSystemTest, AudioMemoryEfficiency) {
    // Test memory efficiency with many short-lived audio sources
    auto listener = create_test_audio_listener({0.0f, 0.0f, 0.0f});
    
    // Record baseline memory usage
    auto baseline_stats = audio_memory_tracker_->get_stats();
    
    // Create and destroy many audio sources in cycles
    for (int cycle = 0; cycle < 10; ++cycle) {
        std::vector<Entity> temp_sources;
        
        // Create sources
        for (int i = 0; i < 20; ++i) {
            auto source = create_test_audio_source({
                static_cast<f32>(i), 
                0.0f, 
                static_cast<f32>(cycle)
            });
            temp_sources.push_back(source);
        }
        
        // Update systems
        spatial_audio_system_->update(0.016f);
        audio_analysis_system_->update(0.016f);
        
        // Destroy sources
        for (auto source : temp_sources) {
            world_->destroy_entity(source);
        }
        
        // Update systems to clean up
        spatial_audio_system_->update(0.016f);
        audio_analysis_system_->update(0.016f);
    }
    
    // Final memory usage should not be significantly higher than baseline
    auto final_stats = audio_memory_tracker_->get_stats();
    
    // Allow for some memory growth, but not excessive
    usize memory_growth = final_stats.current_memory_usage - baseline_stats.current_memory_usage;
    usize reasonable_growth_limit = 1024 * 1024; // 1MB
    
    EXPECT_LT(memory_growth, reasonable_growth_limit)
        << "Memory usage grew by " << memory_growth << " bytes";
}

// =============================================================================
// Educational Content Tests
// =============================================================================

TEST_F(AudioSystemTest, AudioEducationSystemIntegration) {
    // Create educational audio tutorial system
    AudioEducationSystem education_system;
    education_system.initialize();
    
    // Test basic educational content
    auto tutorials = education_system.get_available_tutorials();
    EXPECT_FALSE(tutorials.empty());
    
    // Check for expected tutorials
    bool has_spatial_audio_tutorial = false;
    bool has_hrtf_tutorial = false;
    bool has_doppler_tutorial = false;
    
    for (const auto& tutorial : tutorials) {
        if (tutorial.title.find("Spatial Audio") != std::string::npos) {
            has_spatial_audio_tutorial = true;
        }
        if (tutorial.title.find("HRTF") != std::string::npos) {
            has_hrtf_tutorial = true;
        }
        if (tutorial.title.find("Doppler") != std::string::npos) {
            has_doppler_tutorial = true;
        }
    }
    
    EXPECT_TRUE(has_spatial_audio_tutorial);
    EXPECT_TRUE(has_hrtf_tutorial);
    EXPECT_TRUE(has_doppler_tutorial);
    
    // Test tutorial triggering
    auto source = create_test_audio_source({10.0f, 0.0f, 0.0f});
    auto listener = create_test_audio_listener({0.0f, 0.0f, 0.0f});
    
    // Enable HRTF to trigger tutorial
    auto& audio_source = world_->get_component<AudioSource>(source);
    audio_source.enable_hrtf = true;
    
    spatial_audio_system_->update(0.016f);
    education_system.update(0.016f, *world_);
    
    // Check if HRTF tutorial was triggered
    auto triggered_tutorials = education_system.get_triggered_tutorials();
    bool hrtf_tutorial_triggered = std::any_of(triggered_tutorials.begin(), triggered_tutorials.end(),
        [](const auto& tutorial) { return tutorial.title.find("HRTF") != std::string::npos; });
    
    EXPECT_TRUE(hrtf_tutorial_triggered);
}

TEST_F(AudioSystemTest, AudioSystemEducationalMetrics) {
    // Test educational value scoring for different scenarios
    auto source = create_test_audio_source({5.0f, 0.0f, 0.0f});
    auto listener = create_test_audio_listener({0.0f, 0.0f, 0.0f});
    
    // Simple scenario - basic spatial audio
    spatial_audio_system_->update(0.016f);
    f32 basic_score = spatial_audio_system_->get_educational_value_score();
    
    // Complex scenario - add HRTF, analysis, and directional audio
    auto& audio_source = world_->get_component<AudioSource>(source);
    audio_source.enable_hrtf = true;
    audio_source.enable_analysis = true;
    audio_source.type = AudioSourceType::Directional;
    
    spatial_audio_system_->update(0.016f);
    audio_analysis_system_->update(0.016f);
    f32 complex_score = spatial_audio_system_->get_educational_value_score();
    
    // Complex scenario should have higher educational value
    EXPECT_GE(complex_score, basic_score);
    
    // Generate educational summary
    std::string summary = spatial_audio_system_->generate_educational_summary();
    EXPECT_FALSE(summary.empty());
    EXPECT_GT(summary.length(), 100); // Should be comprehensive
    
    // Summary should mention key concepts
    EXPECT_NE(summary.find("spatial"), std::string::npos);
    EXPECT_NE(summary.find("audio"), std::string::npos);
}

} // namespace ECScope::Testing