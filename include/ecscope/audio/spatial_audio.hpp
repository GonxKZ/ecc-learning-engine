#pragma once

#include <memory>
#include <vector>
#include <array>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <functional>

#include "../math.hpp"
#include "audio_components.hpp"

namespace ecscope::audio {

// Forward declarations
class HRTFProcessor;
class AudioSource;
class AudioListener;

// Spatial audio configuration
struct SpatialAudioConfig {
    float doppler_factor = 1.0f;
    float speed_of_sound = 343.3f; // m/s at 20°C
    float air_absorption_coefficient = 0.0001f;
    float room_rolloff_factor = 0.0f;
    bool enable_hrtf = true;
    bool enable_doppler = true;
    bool enable_air_absorption = true;
    bool enable_occlusion = true;
    bool enable_reverb = true;
    uint32_t max_occlusion_rays = 16;
    float occlusion_ray_length = 100.0f;
};

// Distance attenuation models
enum class AttenuationModel {
    None,           // No distance attenuation
    Linear,         // Linear distance attenuation
    Inverse,        // 1/distance attenuation
    InverseSquare,  // 1/distance² attenuation (physically accurate)
    Exponential,    // Exponential decay
    Custom          // User-defined function
};

// Doppler effect processor
class DopplerProcessor {
public:
    DopplerProcessor();
    ~DopplerProcessor();
    
    void initialize(uint32_t sample_rate, float speed_of_sound = 343.3f);
    
    float calculate_doppler_factor(
        const vec3& source_pos, const vec3& source_vel,
        const vec3& listener_pos, const vec3& listener_vel
    ) const;
    
    void process_doppler_effect(
        const float* input_buffer, float* output_buffer,
        uint32_t frame_count, float doppler_factor,
        uint32_t& pitch_shift_state
    );
    
    void set_doppler_factor(float factor) { m_doppler_factor = factor; }
    float get_doppler_factor() const { return m_doppler_factor; }
    
    void set_speed_of_sound(float speed) { m_speed_of_sound = speed; }
    float get_speed_of_sound() const { return m_speed_of_sound; }
    
private:
    uint32_t m_sample_rate = 48000;
    float m_doppler_factor = 1.0f;
    float m_speed_of_sound = 343.3f;
    
    // Pitch shifting implementation
    std::vector<float> m_pitch_shift_buffer;
    std::vector<float> m_overlap_buffer;
    uint32_t m_overlap_size = 512;
};

// Distance attenuation processor
class AttenuationProcessor {
public:
    AttenuationProcessor();
    ~AttenuationProcessor();
    
    void set_model(AttenuationModel model) { m_model = model; }
    AttenuationModel get_model() const { return m_model; }
    
    void set_custom_function(std::function<float(float)> func) { 
        m_custom_function = func; 
        m_model = AttenuationModel::Custom;
    }
    
    float calculate_attenuation(float distance, float min_distance, float max_distance, float rolloff_factor = 1.0f) const;
    
    void apply_distance_attenuation(float* buffer, uint32_t frame_count, float attenuation) const;
    
    // Air absorption simulation
    void apply_air_absorption(float* buffer, uint32_t frame_count, float distance, uint32_t sample_rate) const;
    
private:
    AttenuationModel m_model = AttenuationModel::InverseSquare;
    std::function<float(float)> m_custom_function;
    
    // Air absorption filter coefficients
    mutable std::array<float, 3> m_air_filter_state{0.0f, 0.0f, 0.0f};
};

// Occlusion and obstruction processor
class OcclusionProcessor {
public:
    OcclusionProcessor();
    ~OcclusionProcessor();
    
    // Ray-casting based occlusion
    float calculate_occlusion(
        const vec3& source_pos, const vec3& listener_pos,
        const std::vector<vec3>& obstacle_positions,
        const std::vector<float>& obstacle_radii,
        uint32_t ray_count = 16
    ) const;
    
    // Physics-based occlusion (requires collision detection)
    float calculate_physics_occlusion(
        const vec3& source_pos, const vec3& listener_pos,
        std::function<bool(const vec3&, const vec3&)> ray_cast_func
    ) const;
    
    void apply_occlusion_filter(float* buffer, uint32_t frame_count, float occlusion_factor) const;
    
    void set_max_rays(uint32_t count) { m_max_rays = count; }
    uint32_t get_max_rays() const { return m_max_rays; }
    
    void set_ray_length(float length) { m_ray_length = length; }
    float get_ray_length() const { return m_ray_length; }
    
private:
    uint32_t m_max_rays = 16;
    float m_ray_length = 100.0f;
    
    // Low-pass filter for occlusion
    mutable std::array<float, 3> m_occlusion_filter_state{0.0f, 0.0f, 0.0f};
    
    // Ray generation for occlusion testing
    std::vector<vec3> generate_ray_directions(uint32_t count) const;
    bool test_ray_occlusion(const vec3& start, const vec3& end, const vec3& obstacle_pos, float obstacle_radius) const;
};

// Directional audio processor (cone effects)
class DirectionalProcessor {
public:
    DirectionalProcessor();
    ~DirectionalProcessor();
    
    float calculate_directional_gain(
        const vec3& source_pos, const vec3& source_direction,
        const vec3& listener_pos, float inner_angle, float outer_angle, float outer_gain
    ) const;
    
    void apply_directional_filter(float* buffer, uint32_t frame_count, float directional_factor) const;
    
private:
    // Directional filtering state
    mutable std::array<float, 3> m_directional_filter_state{0.0f, 0.0f, 0.0f};
};

// Main spatial audio processor
class SpatialAudioProcessor {
public:
    SpatialAudioProcessor();
    ~SpatialAudioProcessor();
    
    // Initialization
    bool initialize(const SpatialAudioConfig& config, uint32_t sample_rate, uint32_t buffer_size);
    void shutdown();
    bool is_initialized() const { return m_initialized; }
    
    // Configuration
    void set_config(const SpatialAudioConfig& config) { m_config = config; }
    const SpatialAudioConfig& get_config() const { return m_config; }
    
    // Main processing function
    void process_spatial_audio(
        AudioSource* source, AudioListener* listener,
        const float* input_buffer, float* left_output, float* right_output,
        uint32_t frame_count, float delta_time
    );
    
    // Batch processing for multiple sources
    void process_multiple_sources(
        const std::vector<AudioSource*>& sources,
        AudioListener* listener,
        const std::vector<const float*>& input_buffers,
        float* left_output, float* right_output,
        uint32_t frame_count, float delta_time
    );
    
    // HRTF integration
    void set_hrtf_processor(std::shared_ptr<HRTFProcessor> hrtf) { m_hrtf_processor = hrtf; }
    std::shared_ptr<HRTFProcessor> get_hrtf_processor() const { return m_hrtf_processor; }
    
    // Obstacle management for occlusion
    void add_obstacle(const vec3& position, float radius);
    void remove_obstacle(size_t index);
    void clear_obstacles();
    void set_physics_ray_cast_function(std::function<bool(const vec3&, const vec3&)> func);
    
    // Environmental effects
    void set_environmental_factor(float factor) { m_environmental_factor = factor; }
    float get_environmental_factor() const { return m_environmental_factor; }
    
    // Performance optimization
    void enable_lod_system(bool enable) { m_lod_enabled = enable; }
    bool is_lod_enabled() const { return m_lod_enabled; }
    
    void set_lod_distances(float near_distance, float far_distance);
    void get_lod_distances(float& near_distance, float& far_distance) const;
    
    // Statistics
    struct SpatialStats {
        uint32_t sources_processed = 0;
        uint32_t hrtf_processed = 0;
        uint32_t doppler_processed = 0;
        uint32_t occlusion_processed = 0;
        float processing_time_ms = 0.0f;
        float cpu_usage_percent = 0.0f;
    };
    
    const SpatialStats& get_stats() const { return m_stats; }
    void reset_stats();
    
private:
    // Configuration and state
    SpatialAudioConfig m_config;
    uint32_t m_sample_rate = 48000;
    uint32_t m_buffer_size = 1024;
    bool m_initialized = false;
    
    // Processors
    std::unique_ptr<DopplerProcessor> m_doppler_processor;
    std::unique_ptr<AttenuationProcessor> m_attenuation_processor;
    std::unique_ptr<OcclusionProcessor> m_occlusion_processor;
    std::unique_ptr<DirectionalProcessor> m_directional_processor;
    std::shared_ptr<HRTFProcessor> m_hrtf_processor;
    
    // Obstacle data for occlusion
    std::vector<vec3> m_obstacle_positions;
    std::vector<float> m_obstacle_radii;
    mutable std::mutex m_obstacle_mutex;
    
    // Physics integration
    std::function<bool(const vec3&, const vec3&)> m_physics_ray_cast;
    
    // Environmental effects
    std::atomic<float> m_environmental_factor{0.0f};
    
    // LOD system
    bool m_lod_enabled = true;
    float m_lod_near_distance = 10.0f;
    float m_lod_far_distance = 100.0f;
    
    // Temporary buffers for processing
    std::vector<float> m_temp_buffer_left;
    std::vector<float> m_temp_buffer_right;
    std::vector<float> m_temp_mono_buffer;
    
    // Statistics
    mutable SpatialStats m_stats;
    mutable std::mutex m_stats_mutex;
    std::chrono::high_resolution_clock::time_point m_last_stats_update;
    
    // Internal processing helpers
    void apply_distance_effects(AudioSource* source, const vec3& listener_pos, 
                              float* buffer, uint32_t frame_count);
    void apply_directional_effects(AudioSource* source, const vec3& listener_pos,
                                 float* buffer, uint32_t frame_count);
    void apply_occlusion_effects(AudioSource* source, const vec3& listener_pos,
                               float* buffer, uint32_t frame_count);
    void apply_doppler_effects(AudioSource* source, AudioListener* listener,
                             float* buffer, uint32_t frame_count, float delta_time);
    
    float calculate_lod_quality(float distance) const;
    bool should_process_effect(float distance, float effect_threshold) const;
    
    // Utility functions
    void mix_to_stereo(const float* mono_input, float* left_output, float* right_output,
                      uint32_t frame_count, float left_gain, float right_gain);
    void apply_panning(const float* input, float* left_output, float* right_output,
                      uint32_t frame_count, float pan_value);
    
    float calculate_stereo_pan(const vec3& source_pos, const vec3& listener_pos,
                              const vec3& listener_right) const;
};

// Ambisonics processor for 360-degree audio
class AmbisonicsProcessor {
public:
    enum class AmbisonicOrder {
        First = 1,   // 4 channels (W, X, Y, Z)
        Second = 2,  // 9 channels
        Third = 3    // 16 channels
    };
    
    AmbisonicsProcessor();
    ~AmbisonicsProcessor();
    
    bool initialize(AmbisonicOrder order, uint32_t sample_rate);
    void shutdown();
    
    // Encoding: convert positioned mono sources to ambisonic format
    void encode_source(const float* input, float* ambisonic_output,
                      uint32_t frame_count, const vec3& source_position,
                      const vec3& listener_position, const vec3& listener_orientation);
    
    // Decoding: convert ambisonic format to stereo/multichannel
    void decode_to_stereo(const float* ambisonic_input, float* left_output, float* right_output,
                         uint32_t frame_count, const vec3& listener_orientation);
    
    void decode_to_multichannel(const float* ambisonic_input, float** channel_outputs,
                               uint32_t channel_count, uint32_t frame_count,
                               const std::vector<vec3>& speaker_positions);
    
    // Rotation for head tracking
    void rotate_soundfield(float* ambisonic_buffer, uint32_t frame_count,
                          const vec3& rotation_euler);
    
    uint32_t get_channel_count() const;
    AmbisonicOrder get_order() const { return m_order; }
    
private:
    AmbisonicOrder m_order = AmbisonicOrder::First;
    uint32_t m_sample_rate = 48000;
    bool m_initialized = false;
    
    // Spherical harmonic coefficients
    std::vector<std::vector<float>> m_encoding_matrix;
    std::vector<std::vector<float>> m_decoding_matrix;
    
    // Rotation matrices for soundfield rotation
    std::array<std::array<float, 16>, 16> m_rotation_matrix;
    
    void calculate_encoding_coefficients();
    void calculate_decoding_coefficients(const std::vector<vec3>& speaker_positions);
    void update_rotation_matrix(const vec3& rotation);
    
    float spherical_harmonic(int l, int m, float theta, float phi) const;
    float associated_legendre(int l, int m, float x) const;
};

} // namespace ecscope::audio