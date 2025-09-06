#include "../framework/ecscope_test_framework.hpp"
#include <ecscope/spatial_audio_engine.hpp>
#include <ecscope/audio_systems.hpp>
#include <ecscope/audio_components.hpp>
#include <ecscope/audio_processing_pipeline.hpp>
#include <ecscope/audio_importer.hpp>
#include <ecscope/audio_education_system.hpp>

#include <random>
#include <thread>
#include <atomic>
#include <complex>
#include <cmath>

namespace ECScope::Testing {

// =============================================================================
// Audio System Test Fixture
// =============================================================================

class AudioSystemTest : public ECScopeTestFixture {
protected:
    void SetUp() override {
        ECScopeTestFixture::SetUp();
        
        // Initialize audio systems
        spatial_engine_ = std::make_unique<SpatialAudio::Engine>();
        audio_processor_ = std::make_unique<Audio::ProcessingPipeline>();
        importer_ = std::make_unique<Audio::Importer>();
        education_system_ = std::make_unique<Audio::EducationSystem>();
        
        // Set up test parameters
        sample_rate_ = 48000;
        buffer_size_ = 512;
        channels_ = 2;
        bit_depth_ = 24;
        
        // Initialize test data
        test_frequency_ = 440.0f; // A4 note
        test_duration_ = 1.0f; // 1 second
        
        // Create test audio data
        create_test_audio_data();
    }

    void TearDown() override {
        education_system_.reset();
        importer_.reset();
        audio_processor_.reset();
        spatial_engine_.reset();
        ECScopeTestFixture::TearDown();
    }

    void create_test_audio_data() {
        size_t sample_count = static_cast<size_t>(sample_rate_ * test_duration_);
        test_audio_data_.resize(sample_count);
        
        // Generate sine wave test data
        for (size_t i = 0; i < sample_count; ++i) {
            float t = static_cast<float>(i) / sample_rate_;
            test_audio_data_[i] = std::sin(2.0f * M_PI * test_frequency_ * t);
        }
    }

protected:
    std::unique_ptr<SpatialAudio::Engine> spatial_engine_;
    std::unique_ptr<Audio::ProcessingPipeline> audio_processor_;
    std::unique_ptr<Audio::Importer> importer_;
    std::unique_ptr<Audio::EducationSystem> education_system_;
    
    uint32_t sample_rate_;
    uint32_t buffer_size_;
    uint32_t channels_;
    uint32_t bit_depth_;
    
    float test_frequency_;
    float test_duration_;
    std::vector<float> test_audio_data_;
};

// =============================================================================
// Spatial Audio Engine Tests
// =============================================================================

TEST_F(AudioSystemTest, SpatialAudioSourceCreation) {
    auto entity = world_->create_entity();
    
    // Add 3D position
    world_->add_component<Transform3D>(entity, Vec3{1.0f, 2.0f, 3.0f});
    
    // Create spatial audio source
    SpatialAudio::SourceParams params;
    params.max_distance = 10.0f;
    params.rolloff_factor = 1.0f;
    params.cone_inner_angle = 30.0f;
    params.cone_outer_angle = 90.0f;
    params.cone_outer_gain = 0.3f;
    
    world_->add_component<SpatialAudio::Source>(entity, params);
    
    EXPECT_TRUE(world_->has_component<SpatialAudio::Source>(entity));
    auto& source = world_->get_component<SpatialAudio::Source>(entity);
    
    EXPECT_FLOAT_EQ(source.params.max_distance, 10.0f);
    EXPECT_FLOAT_EQ(source.params.rolloff_factor, 1.0f);
}

TEST_F(AudioSystemTest, SpatialAudioDistanceAttenuation) {
    auto listener_entity = world_->create_entity();
    auto source_entity = world_->create_entity();
    
    // Set up listener at origin
    world_->add_component<Transform3D>(listener_entity, Vec3{0, 0, 0});
    world_->add_component<SpatialAudio::Listener>(listener_entity);
    
    // Set up source at various distances
    SpatialAudio::SourceParams params;
    params.max_distance = 10.0f;
    params.rolloff_factor = 1.0f;
    params.reference_distance = 1.0f;
    
    world_->add_component<SpatialAudio::Source>(source_entity, params);
    
    // Test attenuation at different distances
    std::vector<float> distances = {1.0f, 2.0f, 5.0f, 10.0f};
    std::vector<float> attenuations;
    
    for (float distance : distances) {
        world_->add_component<Transform3D>(source_entity, Vec3{distance, 0, 0});
        
        float attenuation = spatial_engine_->calculate_distance_attenuation(
            world_->get_component<Transform3D>(listener_entity).position,
            world_->get_component<Transform3D>(source_entity).position,
            params
        );
        attenuations.push_back(attenuation);
    }
    
    // Attenuation should decrease with distance
    for (size_t i = 1; i < attenuations.size(); ++i) {
        EXPECT_LT(attenuations[i], attenuations[i-1]) 
            << "Attenuation should decrease with distance";
    }
    
    // At reference distance, attenuation should be 1.0
    EXPECT_NEAR(attenuations[0], 1.0f, 1e-4f);
}

// =============================================================================
// HRTF Processing Tests
// =============================================================================

TEST_F(AudioSystemTest, HRTFInitialization) {
    // Initialize HRTF system
    auto hrtf_processor = spatial_engine_->create_hrtf_processor();
    ASSERT_NE(hrtf_processor, nullptr);
    
    // Test basic HRTF parameters
    auto params = hrtf_processor->get_parameters();
    EXPECT_GT(params.sample_rate, 0);
    EXPECT_GT(params.filter_length, 0);
    EXPECT_GT(params.azimuth_resolution, 0);
    EXPECT_GT(params.elevation_resolution, 0);
}

TEST_F(AudioSystemTest, HRTFSpatialProcessing) {
    auto hrtf_processor = spatial_engine_->create_hrtf_processor();
    ASSERT_NE(hrtf_processor, nullptr);
    
    // Test HRTF processing for different directions
    std::vector<std::pair<float, float>> directions = {
        {0.0f, 0.0f},    // Front
        {90.0f, 0.0f},   // Right
        {-90.0f, 0.0f},  // Left
        {180.0f, 0.0f},  // Back
        {0.0f, 45.0f},   // Front-up
        {0.0f, -45.0f}   // Front-down
    };
    
    for (auto [azimuth, elevation] : directions) {
        std::vector<float> input_buffer = test_audio_data_;
        std::vector<float> left_output(input_buffer.size());
        std::vector<float> right_output(input_buffer.size());
        
        hrtf_processor->process_spatial(
            input_buffer.data(),
            left_output.data(),
            right_output.data(),
            input_buffer.size(),
            azimuth,
            elevation
        );
        
        // Verify outputs are not silent
        bool left_has_signal = std::any_of(left_output.begin(), left_output.end(),
            [](float sample) { return std::abs(sample) > 1e-6f; });
        bool right_has_signal = std::any_of(right_output.begin(), right_output.end(),
            [](float sample) { return std::abs(sample) > 1e-6f; });
            
        EXPECT_TRUE(left_has_signal) << "Left channel should have signal for azimuth " 
                                     << azimuth << ", elevation " << elevation;
        EXPECT_TRUE(right_has_signal) << "Right channel should have signal for azimuth " 
                                      << azimuth << ", elevation " << elevation;
    }
}

// =============================================================================
// DSP Processing Tests
// =============================================================================

TEST_F(AudioSystemTest, BasicDSPFilters) {
    // Test low-pass filter
    auto lowpass = audio_processor_->create_lowpass_filter(sample_rate_, 1000.0f, 0.707f);
    ASSERT_NE(lowpass, nullptr);
    
    std::vector<float> input = test_audio_data_;
    std::vector<float> output(input.size());
    
    lowpass->process(input.data(), output.data(), input.size());
    
    // Output should not be silent
    bool has_output = std::any_of(output.begin(), output.end(),
        [](float sample) { return std::abs(sample) > 1e-6f; });
    EXPECT_TRUE(has_output);
    
    // Test high-pass filter
    auto highpass = audio_processor_->create_highpass_filter(sample_rate_, 1000.0f, 0.707f);
    ASSERT_NE(highpass, nullptr);
    
    highpass->process(input.data(), output.data(), input.size());
    
    has_output = std::any_of(output.begin(), output.end(),
        [](float sample) { return std::abs(sample) > 1e-6f; });
    EXPECT_TRUE(has_output);
}

TEST_F(AudioSystemTest, ReverbProcessor) {
    auto reverb = audio_processor_->create_reverb_processor();
    ASSERT_NE(reverb, nullptr);
    
    // Set reverb parameters
    Audio::ReverbParams params;
    params.room_size = 0.8f;
    params.damping = 0.5f;
    params.wet_level = 0.3f;
    params.dry_level = 0.7f;
    params.width = 1.0f;
    params.freeze_mode = false;
    
    reverb->set_parameters(params);
    
    // Process audio through reverb
    std::vector<float> input = test_audio_data_;
    std::vector<float> output(input.size());
    
    reverb->process(input.data(), output.data(), input.size());
    
    // Reverb should modify the signal
    float input_energy = 0.0f;
    float output_energy = 0.0f;
    
    for (size_t i = 0; i < input.size(); ++i) {
        input_energy += input[i] * input[i];
        output_energy += output[i] * output[i];
    }
    
    // With wet level > 0, output energy should generally be different
    EXPECT_NE(input_energy, output_energy);
}

// =============================================================================
// Audio Streaming Tests
// =============================================================================

TEST_F(AudioSystemTest, AudioBufferManagement) {
    constexpr size_t buffer_count = 4;
    auto buffer_manager = audio_processor_->create_buffer_manager(buffer_size_, buffer_count);
    ASSERT_NE(buffer_manager, nullptr);
    
    // Test buffer acquisition and release
    std::vector<Audio::Buffer*> acquired_buffers;
    
    for (size_t i = 0; i < buffer_count; ++i) {
        auto buffer = buffer_manager->acquire_buffer();
        ASSERT_NE(buffer, nullptr) << "Should acquire buffer " << i;
        acquired_buffers.push_back(buffer);
    }
    
    // All buffers should be acquired
    auto null_buffer = buffer_manager->acquire_buffer();
    EXPECT_EQ(null_buffer, nullptr) << "Should not acquire buffer when all are used";
    
    // Release buffers
    for (auto buffer : acquired_buffers) {
        buffer_manager->release_buffer(buffer);
    }
    
    // Should be able to acquire again
    auto reacquired_buffer = buffer_manager->acquire_buffer();
    EXPECT_NE(reacquired_buffer, nullptr) << "Should reacquire buffer after release";
    buffer_manager->release_buffer(reacquired_buffer);
}

// =============================================================================
// Performance Tests
// =============================================================================

TEST_F(AudioSystemTest, AudioProcessingPerformance) {
    constexpr size_t chunk_count = 1000;
    constexpr size_t chunk_size = 512;
    
    auto processor = audio_processor_->create_basic_processor();
    ASSERT_NE(processor, nullptr);
    
    std::vector<float> input_chunk(chunk_size);
    std::vector<float> output_chunk(chunk_size);
    
    // Fill input with test data
    for (size_t i = 0; i < chunk_size; ++i) {
        float t = static_cast<float>(i) / sample_rate_;
        input_chunk[i] = std::sin(2.0f * M_PI * test_frequency_ * t);
    }
    
    // Measure processing performance
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < chunk_count; ++i) {
        processor->process(input_chunk.data(), output_chunk.data(), chunk_size);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Calculate real-time performance
    double total_audio_time = (chunk_count * chunk_size) / static_cast<double>(sample_rate_);
    double processing_time = duration.count() / 1000000.0; // Convert to seconds
    double real_time_factor = total_audio_time / processing_time;
    
    std::cout << "Audio processing performance: " << real_time_factor 
              << "x real-time (higher is better)\n";
    
    // Should be able to process faster than real-time
    EXPECT_GT(real_time_factor, 1.0) << "Should process faster than real-time";
}

} // namespace ECScope::Testing