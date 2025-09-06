#include "../framework/ecscope_test_framework.hpp"

#include <ecscope/audio_components.hpp>
#include <ecscope/audio_systems.hpp>
#include <ecscope/audio_processing_pipeline.hpp>

#ifdef ECSCOPE_ENABLE_AUDIO
#include <ecscope/spatial_audio_engine.hpp>
#include <ecscope/dsp_processors.hpp>
#include <ecscope/hrtf_processor.hpp>
#include <ecscope/reverb_engine.hpp>
#include <ecscope/audio_streaming_system.hpp>
#endif

namespace ECScope::Testing {

#ifdef ECSCOPE_ENABLE_AUDIO
class AudioSystemTest : public ECScopeTestFixture {
protected:
    void SetUp() override {
        ECScopeTestFixture::SetUp();
        
        // Initialize audio configuration
        audio_config_.sample_rate = 44100;
        audio_config_.buffer_size = 512;
        audio_config_.channels = 2;
        audio_config_.bit_depth = 16;
        
        // Initialize audio engine
        audio_engine_ = std::make_unique<Audio::SpatialAudioEngine>(audio_config_);
        audio_engine_->initialize();
        
        // Initialize DSP chain
        dsp_chain_ = std::make_unique<Audio::DSPChain>();
        
        // Initialize HRTF processor
        hrtf_processor_ = std::make_unique<Audio::HRTFProcessor>();
        hrtf_processor_->load_database("assets/audio/hrtf/mit_kemar.hrtf");
        
        // Initialize reverb engine
        reverb_engine_ = std::make_unique<Audio::ReverbEngine>();
        
        // Initialize streaming system
        streaming_system_ = std::make_unique<Audio::StreamingSystem>(audio_config_);
        
        // Create test listener
        listener_entity_ = world_->create_entity();
        world_->add_component<Audio::AudioListener>(listener_entity_);
        world_->add_component<Transform3D>(listener_entity_, Vec3(0, 0, 0));
    }

    void TearDown() override {
        // Clean up audio systems
        streaming_system_.reset();
        reverb_engine_.reset();
        hrtf_processor_.reset();
        dsp_chain_.reset();
        audio_engine_.reset();
        
        ECScopeTestFixture::TearDown();
    }

    // Helper to create audio source entity
    Entity create_audio_source(const Vec3& position, const std::string& audio_file = "") {
        auto entity = world_->create_entity();
        world_->add_component<Transform3D>(entity, position);
        
        Audio::AudioSource source;
        if (!audio_file.empty()) {
            source.audio_clip = audio_file;
        }
        source.volume = 1.0f;
        source.pitch = 1.0f;
        source.is_3d = true;
        source.min_distance = 1.0f;
        source.max_distance = 100.0f;
        source.rolloff_factor = 1.0f;
        
        world_->add_component<Audio::AudioSource>(entity, source);
        return entity;
    }

    // Helper to generate test audio data
    std::vector<float> generate_sine_wave(float frequency, float duration, int sample_rate = 44100) {
        int sample_count = static_cast<int>(duration * sample_rate);
        std::vector<float> samples(sample_count);
        
        for (int i = 0; i < sample_count; ++i) {
            float t = static_cast<float>(i) / sample_rate;
            samples[i] = std::sin(2.0f * Math::PI * frequency * t);
        }
        
        return samples;
    }

protected:
    Audio::AudioConfiguration audio_config_;
    std::unique_ptr<Audio::SpatialAudioEngine> audio_engine_;
    std::unique_ptr<Audio::DSPChain> dsp_chain_;
    std::unique_ptr<Audio::HRTFProcessor> hrtf_processor_;
    std::unique_ptr<Audio::ReverbEngine> reverb_engine_;
    std::unique_ptr<Audio::StreamingSystem> streaming_system_;
    Entity listener_entity_;
};

// =============================================================================
// Basic Audio System Tests
// =============================================================================

TEST_F(AudioSystemTest, AudioEngineInitialization) {
    EXPECT_TRUE(audio_engine_->is_initialized());
    EXPECT_EQ(audio_engine_->get_sample_rate(), audio_config_.sample_rate);
    EXPECT_EQ(audio_engine_->get_buffer_size(), audio_config_.buffer_size);
    EXPECT_EQ(audio_engine_->get_channel_count(), audio_config_.channels);
}

TEST_F(AudioSystemTest, AudioSourceCreation) {
    auto source_entity = create_audio_source(Vec3(1, 0, 0));
    
    EXPECT_TRUE(world_->has_component<Audio::AudioSource>(source_entity));
    EXPECT_TRUE(world_->has_component<Transform3D>(source_entity));
    
    auto& source = world_->get_component<Audio::AudioSource>(source_entity);
    EXPECT_FLOAT_EQ(source.volume, 1.0f);
    EXPECT_FLOAT_EQ(source.pitch, 1.0f);
    EXPECT_TRUE(source.is_3d);
}

TEST_F(AudioSystemTest, AudioListenerFunctionality) {
    auto& listener = world_->get_component<Audio::AudioListener>(listener_entity_);
    auto& listener_transform = world_->get_component<Transform3D>(listener_entity_);
    
    // Test listener orientation
    listener.forward = Vec3(0, 0, -1);
    listener.up = Vec3(0, 1, 0);
    
    // Move listener
    listener_transform.position = Vec3(5, 2, 3);
    
    audio_engine_->update_listener(listener_entity_);
    
    // Verify listener was updated in engine
    auto engine_listener_pos = audio_engine_->get_listener_position();
    EXPECT_FLOAT_EQ(engine_listener_pos.x, 5.0f);
    EXPECT_FLOAT_EQ(engine_listener_pos.y, 2.0f);
    EXPECT_FLOAT_EQ(engine_listener_pos.z, 3.0f);
}

// =============================================================================
// 3D Spatial Audio Tests
// =============================================================================

TEST_F(AudioSystemTest, SpatialAudioPositioning) {
    // Create audio source to the right of listener
    auto source_entity = create_audio_source(Vec3(5, 0, 0));
    
    // Generate test audio
    auto test_audio = generate_sine_wave(440.0f, 1.0f); // A4 note, 1 second
    audio_engine_->load_audio_data(source_entity, test_audio);
    
    // Process spatial audio
    Audio::SpatialAudioParams params;
    params.source_position = Vec3(5, 0, 0);
    params.listener_position = Vec3(0, 0, 0);
    params.listener_forward = Vec3(0, 0, -1);
    params.listener_up = Vec3(0, 1, 0);
    
    auto spatialized_audio = audio_engine_->process_spatial_audio(source_entity, params);
    
    EXPECT_EQ(spatialized_audio.size(), audio_config_.buffer_size * 2); // Stereo output
    
    // Right channel should be louder than left for source on the right
    float left_energy = 0.0f;
    float right_energy = 0.0f;
    
    for (size_t i = 0; i < spatialized_audio.size(); i += 2) {
        left_energy += spatialized_audio[i] * spatialized_audio[i];
        right_energy += spatialized_audio[i + 1] * spatialized_audio[i + 1];
    }
    
    EXPECT_GT(right_energy, left_energy);
}

TEST_F(AudioSystemTest, DistanceAttenuation) {
    auto source_entity = create_audio_source(Vec3(0, 0, 0));
    auto& source = world_->get_component<Audio::AudioSource>(source_entity);
    
    source.min_distance = 1.0f;
    source.max_distance = 10.0f;
    source.rolloff_factor = 1.0f;
    
    // Test attenuation at different distances
    std::vector<float> test_distances = {0.5f, 1.0f, 2.0f, 5.0f, 10.0f, 20.0f};
    std::vector<float> attenuations;
    
    for (float distance : test_distances) {
        float attenuation = audio_engine_->calculate_distance_attenuation(
            source, distance);
        attenuations.push_back(attenuation);
    }
    
    // Attenuation should decrease with distance
    for (size_t i = 1; i < attenuations.size(); ++i) {
        EXPECT_LE(attenuations[i], attenuations[i-1]);
    }
    
    // At min distance, attenuation should be 1.0
    EXPECT_FLOAT_EQ(attenuations[1], 1.0f); // distance = 1.0 = min_distance
    
    // Beyond max distance, attenuation should be very low
    EXPECT_LT(attenuations.back(), 0.1f);
}

TEST_F(AudioSystemTest, DopplerEffect) {
    auto source_entity = create_audio_source(Vec3(0, 0, -10));
    auto& transform = world_->get_component<Transform3D>(source_entity);
    
    // Source moving towards listener at 10 m/s
    Vec3 velocity(0, 0, 10);
    world_->add_component<Audio::Velocity>(source_entity, velocity);
    
    // Calculate Doppler shift
    float original_frequency = 440.0f;
    float doppler_frequency = audio_engine_->calculate_doppler_shift(
        source_entity, listener_entity_, original_frequency);
    
    // Frequency should be higher (blue-shifted) when source approaches
    EXPECT_GT(doppler_frequency, original_frequency);
    
    // Reverse velocity (source moving away)
    auto& vel_component = world_->get_component<Audio::Velocity>(source_entity);
    vel_component.velocity = Vec3(0, 0, -10);
    
    doppler_frequency = audio_engine_->calculate_doppler_shift(
        source_entity, listener_entity_, original_frequency);
    
    // Frequency should be lower (red-shifted) when source recedes
    EXPECT_LT(doppler_frequency, original_frequency);
}

// =============================================================================
// DSP Processing Tests
// =============================================================================

TEST_F(AudioSystemTest, BasicDSPEffects) {
    auto test_audio = generate_sine_wave(440.0f, 1.0f);
    
    // Test low-pass filter
    auto low_pass = std::make_unique<Audio::LowPassFilter>(audio_config_.sample_rate, 1000.0f);
    dsp_chain_->add_processor(std::move(low_pass));
    
    auto filtered_audio = dsp_chain_->process(test_audio);
    EXPECT_EQ(filtered_audio.size(), test_audio.size());
    
    // High frequencies should be attenuated
    auto spectrum_original = Audio::FFTAnalyzer::compute_spectrum(test_audio);
    auto spectrum_filtered = Audio::FFTAnalyzer::compute_spectrum(filtered_audio);
    
    // Energy at 440 Hz should be preserved (below cutoff)
    int freq_440_bin = static_cast<int>(440.0f * test_audio.size() / audio_config_.sample_rate);
    EXPECT_GT(spectrum_filtered[freq_440_bin], spectrum_original[freq_440_bin] * 0.8f);
    
    // Energy at high frequencies should be reduced
    int high_freq_bin = static_cast<int>(5000.0f * test_audio.size() / audio_config_.sample_rate);
    if (high_freq_bin < spectrum_filtered.size()) {
        EXPECT_LT(spectrum_filtered[high_freq_bin], spectrum_original[high_freq_bin] * 0.5f);
    }
}

TEST_F(AudioSystemTest, ReverbProcessing) {
    // Configure reverb parameters
    Audio::ReverbParameters reverb_params;
    reverb_params.room_size = 0.8f;
    reverb_params.damping = 0.5f;
    reverb_params.wet_level = 0.3f;
    reverb_params.dry_level = 0.7f;
    reverb_params.pre_delay = 0.03f; // 30ms pre-delay
    
    reverb_engine_->set_parameters(reverb_params);
    
    // Process dry signal
    auto dry_audio = generate_sine_wave(440.0f, 0.5f);
    auto reverb_audio = reverb_engine_->process(dry_audio);
    
    EXPECT_EQ(reverb_audio.size(), dry_audio.size());
    
    // Reverb should add energy (higher RMS)
    float dry_rms = Audio::AudioAnalyzer::calculate_rms(dry_audio);
    float reverb_rms = Audio::AudioAnalyzer::calculate_rms(reverb_audio);
    
    // With wet level < dry level, reverb RMS might be similar or slightly higher
    EXPECT_GE(reverb_rms, dry_rms * 0.8f);
    
    // Test impulse response characteristics
    std::vector<float> impulse(audio_config_.buffer_size, 0.0f);
    impulse[0] = 1.0f; // Unit impulse
    
    auto impulse_response = reverb_engine_->process(impulse);
    
    // Impulse response should have exponential decay characteristic
    float early_energy = 0.0f;
    float late_energy = 0.0f;
    
    for (size_t i = 0; i < impulse_response.size() / 2; ++i) {
        early_energy += impulse_response[i] * impulse_response[i];
    }
    
    for (size_t i = impulse_response.size() / 2; i < impulse_response.size(); ++i) {
        late_energy += impulse_response[i] * impulse_response[i];
    }
    
    // Early reflections should have more energy than late reflections
    EXPECT_GT(early_energy, late_energy);
}

TEST_F(AudioSystemTest, EqualizerProcessing) {
    // Create parametric EQ
    auto eq = std::make_unique<Audio::ParametricEqualizer>(audio_config_.sample_rate);
    
    // Add frequency bands
    eq->add_band(Audio::EQBand{100.0f, 2.0f, 1.5f, Audio::EQBand::Type::HighPass});
    eq->add_band(Audio::EQBand{1000.0f, 1.0f, 0.5f, Audio::EQBand::Type::Peak});
    eq->add_band(Audio::EQBand{5000.0f, 2.0f, 2.0f, Audio::EQBand::Type::LowPass});
    
    dsp_chain_->clear();
    dsp_chain_->add_processor(std::move(eq));
    
    // Create test signal with multiple frequencies
    auto test_signal = generate_sine_wave(50.0f, 1.0f);   // Should be attenuated (below HPF)
    auto mid_signal = generate_sine_wave(1000.0f, 1.0f);  // Should be attenuated (peak cut)
    auto high_signal = generate_sine_wave(10000.0f, 1.0f); // Should be attenuated (above LPF)
    
    // Mix signals
    std::vector<float> mixed_signal(test_signal.size());
    for (size_t i = 0; i < mixed_signal.size(); ++i) {
        mixed_signal[i] = (test_signal[i] + mid_signal[i] + high_signal[i]) / 3.0f;
    }
    
    auto eq_signal = dsp_chain_->process(mixed_signal);
    
    // Verify frequency response
    auto original_spectrum = Audio::FFTAnalyzer::compute_spectrum(mixed_signal);
    auto eq_spectrum = Audio::FFTAnalyzer::compute_spectrum(eq_signal);
    
    // 1000 Hz should be attenuated (gain = 0.5 = -6dB)
    int freq_1000_bin = static_cast<int>(1000.0f * eq_signal.size() / audio_config_.sample_rate);
    if (freq_1000_bin < eq_spectrum.size()) {
        EXPECT_LT(eq_spectrum[freq_1000_bin], original_spectrum[freq_1000_bin] * 0.8f);
    }
}

// =============================================================================
// HRTF Processing Tests
// =============================================================================

TEST_F(AudioSystemTest, HRTFProcessing) {
    if (!hrtf_processor_->is_database_loaded()) {
        GTEST_SKIP() << "HRTF database not available";
    }
    
    // Test HRTF processing for different angles
    auto mono_audio = generate_sine_wave(1000.0f, 0.1f); // 100ms tone
    
    // Test source at 90 degrees (right side)
    float azimuth = 90.0f;   // degrees
    float elevation = 0.0f;  // degrees
    
    auto hrtf_output = hrtf_processor_->process(mono_audio, azimuth, elevation);
    EXPECT_EQ(hrtf_output.size(), mono_audio.size() * 2); // Stereo output
    
    // For 90-degree azimuth, right channel should have different characteristics than left
    std::vector<float> left_channel, right_channel;
    for (size_t i = 0; i < hrtf_output.size(); i += 2) {
        left_channel.push_back(hrtf_output[i]);
        right_channel.push_back(hrtf_output[i + 1]);
    }
    
    // Calculate cross-correlation to measure time delay
    auto correlation = Audio::SignalProcessor::cross_correlation(left_channel, right_channel);
    int max_correlation_index = std::max_element(correlation.begin(), correlation.end()) - correlation.begin();
    
    // There should be an interaural time difference (ITD)
    EXPECT_NE(max_correlation_index, correlation.size() / 2); // Not perfectly aligned
    
    // Test frequency response differences (ILD - Interaural Level Difference)
    auto left_spectrum = Audio::FFTAnalyzer::compute_spectrum(left_channel);
    auto right_spectrum = Audio::FFTAnalyzer::compute_spectrum(right_channel);
    
    float left_energy = std::accumulate(left_spectrum.begin(), left_spectrum.end(), 0.0f);
    float right_energy = std::accumulate(right_spectrum.begin(), right_spectrum.end(), 0.0f);
    
    // For 90-degree source, right channel should generally have more energy
    EXPECT_GT(right_energy, left_energy * 0.8f);
}

TEST_F(AudioSystemTest, HRTFInterpolation) {
    if (!hrtf_processor_->is_database_loaded()) {
        GTEST_SKIP() << "HRTF database not available";
    }
    
    auto test_audio = generate_sine_wave(1000.0f, 0.1f);
    
    // Test HRTF at exact database position
    float azimuth_exact = 45.0f;
    auto hrtf_exact = hrtf_processor_->process(test_audio, azimuth_exact, 0.0f);
    
    // Test HRTF at intermediate position (requires interpolation)
    float azimuth_interp = 47.5f; // Halfway between 45 and 50 degrees
    auto hrtf_interp = hrtf_processor_->process(test_audio, azimuth_interp, 0.0f);
    
    EXPECT_EQ(hrtf_exact.size(), hrtf_interp.size());
    
    // Interpolated result should be different from exact position
    bool is_different = false;
    for (size_t i = 0; i < hrtf_exact.size() && !is_different; ++i) {
        if (std::abs(hrtf_exact[i] - hrtf_interp[i]) > 1e-6f) {
            is_different = true;
        }
    }
    
    EXPECT_TRUE(is_different);
    
    // Test smooth transition as azimuth changes
    std::vector<float> azimuths = {40.0f, 42.5f, 45.0f, 47.5f, 50.0f};
    std::vector<std::vector<float>> hrtf_results;
    
    for (float az : azimuths) {
        hrtf_results.push_back(hrtf_processor_->process(test_audio, az, 0.0f));
    }
    
    // Calculate smoothness metric (difference between adjacent results)
    for (size_t i = 1; i < hrtf_results.size(); ++i) {
        float diff = 0.0f;
        for (size_t j = 0; j < hrtf_results[i].size(); ++j) {
            diff += std::abs(hrtf_results[i][j] - hrtf_results[i-1][j]);
        }
        diff /= hrtf_results[i].size();
        
        // Difference should be reasonable (not too abrupt)
        EXPECT_LT(diff, 0.1f);
    }
}

// =============================================================================
// Audio Streaming Tests
// =============================================================================

TEST_F(AudioSystemTest, AudioStreamingBasics) {
    // Create audio stream
    auto stream_id = streaming_system_->create_stream("test_stream");
    EXPECT_NE(stream_id, Audio::INVALID_STREAM_ID);
    
    EXPECT_TRUE(streaming_system_->is_stream_valid(stream_id));
    
    // Test stream configuration
    Audio::StreamConfiguration stream_config;
    stream_config.sample_rate = 44100;
    stream_config.channels = 2;
    stream_config.buffer_count = 4;
    stream_config.buffer_size = 1024;
    
    EXPECT_TRUE(streaming_system_->configure_stream(stream_id, stream_config));
    
    // Clean up
    streaming_system_->destroy_stream(stream_id);
    EXPECT_FALSE(streaming_system_->is_stream_valid(stream_id));
}

TEST_F(AudioSystemTest, AudioStreamingBuffering) {
    auto stream_id = streaming_system_->create_stream("buffering_test");
    
    Audio::StreamConfiguration config;
    config.sample_rate = 44100;
    config.channels = 2;
    config.buffer_count = 3;
    config.buffer_size = 512;
    
    streaming_system_->configure_stream(stream_id, config);
    
    // Generate test audio data
    auto audio_data = generate_sine_wave(440.0f, 2.0f, config.sample_rate);
    
    // Convert mono to stereo
    std::vector<float> stereo_data;
    stereo_data.reserve(audio_data.size() * 2);
    for (float sample : audio_data) {
        stereo_data.push_back(sample);
        stereo_data.push_back(sample);
    }
    
    // Queue audio data
    size_t samples_per_buffer = config.buffer_size * config.channels;
    size_t offset = 0;
    
    while (offset < stereo_data.size()) {
        size_t samples_to_queue = std::min(samples_per_buffer, stereo_data.size() - offset);
        
        bool queued = streaming_system_->queue_audio_data(
            stream_id,
            stereo_data.data() + offset,
            samples_to_queue
        );
        
        EXPECT_TRUE(queued);
        offset += samples_to_queue;
    }
    
    // Start playback
    EXPECT_TRUE(streaming_system_->start_stream(stream_id));
    EXPECT_TRUE(streaming_system_->is_stream_playing(stream_id));
    
    // Simulate playback for a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Stop and clean up
    streaming_system_->stop_stream(stream_id);
    streaming_system_->destroy_stream(stream_id);
}

// =============================================================================
// Performance Tests
// =============================================================================

TEST_F(AudioSystemTest, SpatialAudioPerformance) {
    constexpr int source_count = 100;
    constexpr int buffer_size = 512;
    
    // Create many audio sources
    std::vector<Entity> sources;
    for (int i = 0; i < source_count; ++i) {
        float angle = (2.0f * Math::PI * i) / source_count;
        Vec3 position(std::cos(angle) * 10.0f, 0, std::sin(angle) * 10.0f);
        
        auto source = create_audio_source(position);
        auto test_audio = generate_sine_wave(440.0f + i * 10.0f, 0.1f);
        audio_engine_->load_audio_data(source, test_audio);
        sources.push_back(source);
    }
    
    // Benchmark spatial audio processing
    benchmark("SpatialAudioProcessing", [&]() {
        for (Entity source : sources) {
            Audio::SpatialAudioParams params;
            auto& transform = world_->get_component<Transform3D>(source);
            params.source_position = transform.position;
            params.listener_position = Vec3(0, 0, 0);
            params.listener_forward = Vec3(0, 0, -1);
            params.listener_up = Vec3(0, 1, 0);
            
            auto spatialized = audio_engine_->process_spatial_audio(source, params);
        }
    }, 100);
}

TEST_F(AudioSystemTest, DSPProcessingPerformance) {
    // Create complex DSP chain
    dsp_chain_->clear();
    
    dsp_chain_->add_processor(std::make_unique<Audio::HighPassFilter>(audio_config_.sample_rate, 80.0f));
    dsp_chain_->add_processor(std::make_unique<Audio::ParametricEqualizer>(audio_config_.sample_rate));
    dsp_chain_->add_processor(std::make_unique<Audio::Compressor>(audio_config_.sample_rate));
    dsp_chain_->add_processor(std::make_unique<Audio::LowPassFilter>(audio_config_.sample_rate, 15000.0f));
    
    // Generate test audio
    auto test_audio = generate_sine_wave(1000.0f, 1.0f);
    
    // Benchmark DSP processing
    benchmark("ComplexDSPChain", [&]() {
        auto processed = dsp_chain_->process(test_audio);
    }, 1000);
}

TEST_F(AudioSystemTest, HRTFPerformance) {
    if (!hrtf_processor_->is_database_loaded()) {
        GTEST_SKIP() << "HRTF database not available";
    }
    
    auto test_audio = generate_sine_wave(1000.0f, 0.1f);
    
    // Benchmark HRTF processing
    benchmark("HRTFProcessing", [&]() {
        float azimuth = (rand() % 360) - 180.0f;   // Random azimuth
        float elevation = (rand() % 180) - 90.0f;  // Random elevation
        
        auto hrtf_output = hrtf_processor_->process(test_audio, azimuth, elevation);
    }, 1000);
}

// =============================================================================
// Integration Tests
// =============================================================================

TEST_F(AudioSystemTest, FullAudioPipelineIntegration) {
    // Create a complete audio scene
    auto source1 = create_audio_source(Vec3(-5, 0, 0));
    auto source2 = create_audio_source(Vec3(5, 0, 0));
    auto source3 = create_audio_source(Vec3(0, 0, -5));
    
    // Generate different audio for each source
    auto audio1 = generate_sine_wave(440.0f, 1.0f);  // A4
    auto audio2 = generate_sine_wave(554.37f, 1.0f); // C#5
    auto audio3 = generate_sine_wave(659.25f, 1.0f); // E5
    
    audio_engine_->load_audio_data(source1, audio1);
    audio_engine_->load_audio_data(source2, audio2);
    audio_engine_->load_audio_data(source3, audio3);
    
    // Configure audio sources
    auto& src1 = world_->get_component<Audio::AudioSource>(source1);
    auto& src2 = world_->get_component<Audio::AudioSource>(source2);
    auto& src3 = world_->get_component<Audio::AudioSource>(source3);
    
    src1.volume = 0.7f;
    src2.volume = 0.5f;
    src3.volume = 0.8f;
    
    // Process complete audio frame
    auto final_output = audio_engine_->process_frame();
    
    EXPECT_EQ(final_output.size(), audio_config_.buffer_size * 2); // Stereo output
    
    // Verify output is not silent
    float total_energy = 0.0f;
    for (float sample : final_output) {
        total_energy += sample * sample;
    }
    
    EXPECT_GT(total_energy, 0.0f);
    
    // Move listener and reprocess
    auto& listener_transform = world_->get_component<Transform3D>(listener_entity_);
    listener_transform.position = Vec3(2, 0, 0);
    
    audio_engine_->update_listener(listener_entity_);
    auto moved_output = audio_engine_->process_frame();
    
    // Output should be different after listener movement
    bool is_different = false;
    for (size_t i = 0; i < final_output.size() && !is_different; ++i) {
        if (std::abs(final_output[i] - moved_output[i]) > 1e-6f) {
            is_different = true;
        }
    }
    
    EXPECT_TRUE(is_different);
}

} // namespace ECScope::Testing

#else
// Dummy test when audio is not enabled
namespace ECScope::Testing {

class AudioSystemTest : public ECScopeTestFixture {};

TEST_F(AudioSystemTest, AudioNotEnabled) {
    GTEST_SKIP() << "Audio system not enabled in build configuration";
}

} // namespace ECScope::Testing

#endif // ECSCOPE_ENABLE_AUDIO