#pragma once

#include "audio_types.hpp"
#include "hrtf_processor.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <mutex>

namespace ecscope::audio {

// Forward declarations
class AudioPipeline;
class EnvironmentalProcessor;
class OcclusionProcessor;

// 3D audio voice - represents a single 3D positioned sound
class Audio3DVoice {
public:
    Audio3DVoice(uint32_t voice_id, std::unique_ptr<AudioStream> stream);
    ~Audio3DVoice();
    
    // Voice control
    void play();
    void pause();
    void stop();
    void set_looping(bool looping);
    AudioState get_state() const;
    
    // 3D positioning
    void set_position(const Vector3f& position);
    void set_velocity(const Vector3f& velocity);
    void set_orientation(const Vector3f& direction);
    Vector3f get_position() const;
    Vector3f get_velocity() const;
    Vector3f get_orientation() const;
    
    // Audio properties
    void set_gain(float gain);
    void set_pitch(float pitch);
    void set_min_distance(float distance);
    void set_max_distance(float distance);
    void set_rolloff_factor(float rolloff);
    void set_attenuation_model(AttenuationModel model);
    
    // Directional audio (cone)
    void set_cone_angles(float inner_angle, float outer_angle);
    void set_cone_outer_gain(float gain);
    bool is_directional() const;
    
    // Advanced features
    void set_doppler_factor(float factor);
    void set_air_absorption(bool enable);
    void set_distance_delay(bool enable);
    void set_occlusion_factor(float factor);
    void set_obstruction_factor(float factor);
    
    // Voice information
    uint32_t get_id() const;
    float get_current_distance() const;
    float get_effective_gain() const;
    bool is_audible() const;
    
    // Internal processing
    void update_3d_parameters(const AudioListener& listener, float delta_time);
    size_t process_audio(AudioBuffer& output, size_t samples);

private:
    uint32_t m_id;
    std::unique_ptr<AudioStream> m_stream;
    AudioSource m_source;
    AudioState m_state = AudioState::STOPPED;
    
    // 3D parameters
    float m_current_distance = 0.0f;
    float m_effective_gain = 1.0f;
    float m_doppler_pitch = 1.0f;
    float m_occlusion_factor = 0.0f;
    float m_obstruction_factor = 0.0f;
    bool m_air_absorption_enabled = true;
    bool m_distance_delay_enabled = true;
    
    // Distance attenuation
    AttenuationModel m_attenuation_model = AttenuationModel::INVERSE_CLAMPED;
    
    mutable std::mutex m_mutex;
};

// 3D audio engine - main interface for 3D spatial audio
class Audio3DEngine {
public:
    Audio3DEngine();
    ~Audio3DEngine();
    
    // Engine initialization
    bool initialize(const AudioFormat& format);
    void shutdown();
    bool is_initialized() const;
    
    // Listener management
    void set_listener(const AudioListener& listener);
    AudioListener get_listener() const;
    void update_listener(const Vector3f& position, const Quaternion& orientation, 
                        const Vector3f& velocity = {0, 0, 0});
    
    // Multi-listener support (split-screen)
    uint32_t add_listener(const AudioListener& listener);
    void remove_listener(uint32_t listener_id);
    void set_active_listener(uint32_t listener_id);
    std::vector<uint32_t> get_listener_ids() const;
    
    // Voice management
    uint32_t create_voice(std::unique_ptr<AudioStream> stream);
    uint32_t create_voice_from_file(const std::string& filepath);
    void destroy_voice(uint32_t voice_id);
    Audio3DVoice* get_voice(uint32_t voice_id);
    std::vector<uint32_t> get_active_voices() const;
    
    // Global 3D settings
    void set_doppler_factor(float factor);
    void set_speed_of_sound(float speed);
    void set_distance_model(AttenuationModel model);
    void enable_air_absorption(bool enable);
    void set_air_absorption_coefficients(const std::vector<float>& coefficients);
    
    // HRTF configuration
    bool load_hrtf_database(const std::string& filepath);
    void set_hrtf_interpolation(HRTFInterpolation method);
    void enable_hrtf_processing(bool enable);
    bool is_hrtf_enabled() const;
    
    // Environmental audio
    void set_environmental_settings(const EnvironmentalAudio& settings);
    EnvironmentalAudio get_environmental_settings() const;
    void enable_environmental_processing(bool enable);
    
    // Occlusion and obstruction
    void enable_occlusion_processing(bool enable);
    void set_occlusion_geometry(const std::vector<Vector3f>& geometry);
    void update_occlusion_for_voice(uint32_t voice_id, float occlusion_factor, 
                                   float obstruction_factor);
    
    // Real-time processing
    void update(float delta_time);
    size_t process_audio(AudioBuffer& output, size_t samples);
    size_t process_stereo_audio(StereoBuffer& output, size_t samples);
    
    // Performance optimization
    void set_max_audible_distance(float distance);
    void set_max_concurrent_voices(uint32_t max_voices);
    void enable_voice_culling(bool enable);
    void set_lod_distances(const std::vector<float>& distances);
    
    // Advanced features
    void enable_ambisonics(bool enable, uint32_t order = 1);
    void enable_audio_ray_tracing(bool enable);
    void set_ray_tracing_quality(int quality);
    
    // Debugging and visualization
    void enable_debug_visualization(bool enable);
    std::vector<Vector3f> get_debug_ray_paths(uint32_t voice_id) const;
    AudioMetrics get_3d_metrics() const;
    
    // Error handling
    AudioError get_last_error() const;
    std::string get_error_string() const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

// Doppler effect processor
class DopplerProcessor {
public:
    DopplerProcessor(uint32_t sample_rate);
    ~DopplerProcessor();
    
    // Configuration
    void set_doppler_factor(float factor);
    void set_speed_of_sound(float speed);
    void enable_velocity_smoothing(bool enable);
    
    // Processing
    float calculate_doppler_pitch(const Vector3f& source_pos, const Vector3f& source_vel,
                                 const Vector3f& listener_pos, const Vector3f& listener_vel);
    
    void process_doppler_shift(AudioBuffer& buffer, float pitch_shift);
    void update_pitch_smoothly(float target_pitch, float delta_time);

private:
    uint32_t m_sample_rate;
    float m_doppler_factor = 1.0f;
    float m_speed_of_sound = 343.3f;  // m/s at 20°C
    float m_current_pitch = 1.0f;
    float m_target_pitch = 1.0f;
    bool m_velocity_smoothing = true;
    
    // Pitch shifting implementation
    class PitchShifter;
    std::unique_ptr<PitchShifter> m_pitch_shifter;
};

// Distance attenuation processor
class DistanceProcessor {
public:
    // Calculate distance-based gain
    static float calculate_distance_gain(float distance, float min_distance,
                                       float max_distance, float rolloff_factor,
                                       AttenuationModel model);
    
    // Calculate cone attenuation
    static float calculate_cone_gain(const Vector3f& source_direction,
                                   const Vector3f& to_listener,
                                   float inner_angle, float outer_angle,
                                   float outer_gain);
    
    // Calculate air absorption
    static float calculate_air_absorption(float distance, float frequency,
                                        const std::vector<float>& coefficients);
    
    // Distance delay calculation
    static float calculate_distance_delay(float distance, float speed_of_sound);
};

// 3D audio utilities
namespace audio3d_utils {
    // Coordinate system conversions
    Vector3f world_to_listener_space(const Vector3f& world_pos, 
                                    const AudioListener& listener);
    Vector3f listener_to_world_space(const Vector3f& listener_pos,
                                    const AudioListener& listener);
    
    // Geometric calculations
    float calculate_distance(const Vector3f& pos1, const Vector3f& pos2);
    Vector3f calculate_direction(const Vector3f& from, const Vector3f& to);
    float calculate_angle_between(const Vector3f& vec1, const Vector3f& vec2);
    
    // Audio positioning helpers
    bool is_behind_listener(const Vector3f& source_pos, const AudioListener& listener);
    float calculate_lateral_angle(const Vector3f& source_pos, const AudioListener& listener);
    float calculate_elevation_angle(const Vector3f& source_pos, const AudioListener& listener);
    
    // Performance helpers
    bool is_voice_audible(const Audio3DVoice& voice, const AudioListener& listener,
                         float max_distance);
    float calculate_voice_priority(const Audio3DVoice& voice, const AudioListener& listener);
    
    // Validation
    bool validate_3d_parameters(const AudioSource& source);
    bool validate_listener_parameters(const AudioListener& listener);
}

// 3D audio scene manager
class Audio3DScene {
public:
    struct SceneObject {
        uint32_t id;
        Vector3f position;
        Vector3f velocity;
        Vector3f size;  // Bounding box
        float reflectance = 0.1f;
        float absorption = 0.1f;
        bool is_occluder = true;
    };
    
    Audio3DScene();
    ~Audio3DScene();
    
    // Scene object management
    uint32_t add_scene_object(const SceneObject& object);
    void remove_scene_object(uint32_t object_id);
    void update_scene_object(uint32_t object_id, const SceneObject& object);
    SceneObject* get_scene_object(uint32_t object_id);
    
    // Spatial queries
    std::vector<uint32_t> query_objects_in_radius(const Vector3f& center, float radius) const;
    std::vector<uint32_t> query_objects_in_cone(const Vector3f& apex, 
                                               const Vector3f& direction,
                                               float angle, float range) const;
    
    // Occlusion testing
    float calculate_occlusion(const Vector3f& source, const Vector3f& listener) const;
    float calculate_obstruction(const Vector3f& source, const Vector3f& listener) const;
    std::vector<Vector3f> trace_audio_ray(const Vector3f& source, 
                                         const Vector3f& listener) const;
    
    // Environmental calculations
    float calculate_reverb_time(const Vector3f& position) const;
    float calculate_room_size_estimate(const Vector3f& position) const;
    
    // Scene optimization
    void build_spatial_acceleration_structure();
    void update_dynamic_objects();
    void set_lod_distances(const std::vector<float>& distances);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace ecscope::audio