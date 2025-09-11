#pragma once

#include <memory>
#include <vector>
#include <array>
#include <string>
#include <atomic>
#include <mutex>
#include <functional>

#include "../math.hpp"
#include "../ecs/component.hpp"
#include "audio_engine.hpp"

namespace ecscope::audio {

// Audio buffer class for holding audio data
class AudioBuffer {
public:
    AudioBuffer();
    ~AudioBuffer();
    
    bool load_from_file(const std::string& filename);
    bool load_from_memory(const float* data, uint32_t sample_count, uint32_t channels, uint32_t sample_rate);
    void clear();
    
    // Getters
    const float* get_data() const { return m_data.data(); }
    uint32_t get_sample_count() const { return m_sample_count; }
    uint32_t get_channels() const { return m_channels; }
    uint32_t get_sample_rate() const { return m_sample_rate; }
    float get_duration() const { return static_cast<float>(m_sample_count) / (m_sample_rate * m_channels); }
    bool is_loaded() const { return !m_data.empty(); }
    
    // Format info
    AudioFormat get_format() const { return m_format; }
    size_t get_size_in_bytes() const { return m_data.size() * sizeof(float); }
    
private:
    std::vector<float> m_data;
    uint32_t m_sample_count = 0;
    uint32_t m_channels = 0;
    uint32_t m_sample_rate = 0;
    AudioFormat m_format = AudioFormat::PCM_F32;
};

// Audio source state enumeration
enum class AudioSourceState {
    Stopped,
    Playing,
    Paused,
    Streaming
};

// Audio source class for 3D positioned audio
class AudioSource {
public:
    AudioSource(uint32_t id);
    ~AudioSource();
    
    // Playback control
    void play();
    void pause();
    void stop();
    void rewind();
    bool is_playing() const { return m_state == AudioSourceState::Playing; }
    bool is_paused() const { return m_state == AudioSourceState::Paused; }
    bool is_stopped() const { return m_state == AudioSourceState::Stopped; }
    AudioSourceState get_state() const { return m_state; }
    
    // Buffer management
    void set_buffer(AudioBuffer* buffer);
    AudioBuffer* get_buffer() const { return m_buffer; }
    
    // Position and orientation
    void set_position(const vec3& position) { m_position = position; }
    const vec3& get_position() const { return m_position; }
    void set_velocity(const vec3& velocity) { m_velocity = velocity; }
    const vec3& get_velocity() const { return m_velocity; }
    void set_direction(const vec3& direction) { m_direction = normalize(direction); }
    const vec3& get_direction() const { return m_direction; }
    
    // Audio properties
    void set_volume(float volume) { m_volume = std::clamp(volume, 0.0f, 1.0f); }
    float get_volume() const { return m_volume; }
    void set_pitch(float pitch) { m_pitch = std::max(0.1f, pitch); }
    float get_pitch() const { return m_pitch; }
    void set_pan(float pan) { m_pan = std::clamp(pan, -1.0f, 1.0f); }
    float get_pan() const { return m_pan; }
    
    // 3D audio properties
    void set_min_distance(float distance) { m_min_distance = std::max(0.0f, distance); }
    float get_min_distance() const { return m_min_distance; }
    void set_max_distance(float distance) { m_max_distance = std::max(m_min_distance, distance); }
    float get_max_distance() const { return m_max_distance; }
    void set_rolloff_factor(float factor) { m_rolloff_factor = std::max(0.0f, factor); }
    float get_rolloff_factor() const { return m_rolloff_factor; }
    
    // Directional audio (cone)
    void set_cone_inner_angle(float angle) { m_cone_inner_angle = std::clamp(angle, 0.0f, 360.0f); }
    float get_cone_inner_angle() const { return m_cone_inner_angle; }
    void set_cone_outer_angle(float angle) { m_cone_outer_angle = std::clamp(angle, m_cone_inner_angle, 360.0f); }
    float get_cone_outer_angle() const { return m_cone_outer_angle; }
    void set_cone_outer_gain(float gain) { m_cone_outer_gain = std::clamp(gain, 0.0f, 1.0f); }
    float get_cone_outer_gain() const { return m_cone_outer_gain; }
    
    // Looping and playback
    void set_looping(bool loop) { m_looping = loop; }
    bool is_looping() const { return m_looping; }
    void set_playback_position(float seconds);
    float get_playback_position() const;
    
    // Effects and filtering
    void set_low_pass_filter(float cutoff, float resonance = 1.0f);
    void set_high_pass_filter(float cutoff, float resonance = 1.0f);
    void set_band_pass_filter(float center, float width, float resonance = 1.0f);
    void clear_filters();
    void enable_doppler(bool enable) { m_doppler_enabled = enable; }
    bool is_doppler_enabled() const { return m_doppler_enabled; }
    
    // Occlusion and obstruction
    void set_occlusion_factor(float factor) { m_occlusion_factor = std::clamp(factor, 0.0f, 1.0f); }
    float get_occlusion_factor() const { return m_occlusion_factor; }
    void set_obstruction_factor(float factor) { m_obstruction_factor = std::clamp(factor, 0.0f, 1.0f); }
    float get_obstruction_factor() const { return m_obstruction_factor; }
    
    // Advanced features
    void enable_spatialization(bool enable) { m_spatialization_enabled = enable; }
    bool is_spatialization_enabled() const { return m_spatialization_enabled; }
    void set_room_rolloff_factor(float factor) { m_room_rolloff_factor = std::max(0.0f, factor); }
    float get_room_rolloff_factor() const { return m_room_rolloff_factor; }
    
    // Internal audio processing
    void process_audio(float* output_buffer, uint32_t frame_count, uint32_t channels, uint32_t sample_rate);
    uint32_t get_id() const { return m_id; }
    
    // Streaming support
    void set_streaming(bool streaming) { m_is_streaming = streaming; }
    bool is_streaming() const { return m_is_streaming; }
    
private:
    uint32_t m_id;
    AudioBuffer* m_buffer = nullptr;
    std::atomic<AudioSourceState> m_state{AudioSourceState::Stopped};
    
    // Position and movement
    vec3 m_position{0.0f, 0.0f, 0.0f};
    vec3 m_velocity{0.0f, 0.0f, 0.0f};
    vec3 m_direction{0.0f, 0.0f, -1.0f};
    
    // Audio properties
    std::atomic<float> m_volume{1.0f};
    std::atomic<float> m_pitch{1.0f};
    std::atomic<float> m_pan{0.0f};
    
    // 3D audio properties
    std::atomic<float> m_min_distance{1.0f};
    std::atomic<float> m_max_distance{100.0f};
    std::atomic<float> m_rolloff_factor{1.0f};
    
    // Directional audio
    std::atomic<float> m_cone_inner_angle{360.0f};
    std::atomic<float> m_cone_outer_angle{360.0f};
    std::atomic<float> m_cone_outer_gain{1.0f};
    
    // Playback state
    std::atomic<bool> m_looping{false};
    std::atomic<uint32_t> m_playback_position{0};
    
    // Effects and processing
    std::atomic<bool> m_doppler_enabled{true};
    std::atomic<bool> m_spatialization_enabled{true};
    std::atomic<float> m_occlusion_factor{0.0f};
    std::atomic<float> m_obstruction_factor{0.0f};
    std::atomic<float> m_room_rolloff_factor{0.0f};
    
    // Streaming
    std::atomic<bool> m_is_streaming{false};
    
    // Filter parameters
    struct FilterParams {
        bool enabled = false;
        float cutoff = 1000.0f;
        float resonance = 1.0f;
        float bandwidth = 1.0f;
    };
    
    FilterParams m_low_pass_filter;
    FilterParams m_high_pass_filter;
    FilterParams m_band_pass_filter;
    mutable std::mutex m_filter_mutex;
    
    // Internal state
    float m_last_computed_gain = 1.0f;
    float m_last_computed_pitch = 1.0f;
    vec3 m_last_computed_position{0.0f, 0.0f, 0.0f};
};

// Audio listener class for receiving 3D audio
class AudioListener {
public:
    AudioListener(uint32_t id);
    ~AudioListener();
    
    // Position and orientation
    void set_position(const vec3& position) { m_position = position; }
    const vec3& get_position() const { return m_position; }
    void set_velocity(const vec3& velocity) { m_velocity = velocity; }
    const vec3& get_velocity() const { return m_velocity; }
    
    // Orientation (forward and up vectors)
    void set_orientation(const vec3& forward, const vec3& up);
    const vec3& get_forward() const { return m_forward; }
    const vec3& get_up() const { return m_up; }
    const vec3& get_right() const { return m_right; }
    
    // Audio properties
    void set_volume(float volume) { m_volume = std::clamp(volume, 0.0f, 1.0f); }
    float get_volume() const { return m_volume; }
    
    // HRTF and spatialization
    void enable_hrtf(bool enable) { m_hrtf_enabled = enable; }
    bool is_hrtf_enabled() const { return m_hrtf_enabled; }
    void set_head_radius(float radius) { m_head_radius = std::max(0.05f, radius); }
    float get_head_radius() const { return m_head_radius; }
    
    // Doppler settings
    void enable_doppler(bool enable) { m_doppler_enabled = enable; }
    bool is_doppler_enabled() const { return m_doppler_enabled; }
    
    // Environmental audio
    void set_environmental_filtering(float factor) { m_env_filtering = std::clamp(factor, 0.0f, 1.0f); }
    float get_environmental_filtering() const { return m_env_filtering; }
    
    uint32_t get_id() const { return m_id; }
    
private:
    uint32_t m_id;
    
    // Position and movement
    vec3 m_position{0.0f, 0.0f, 0.0f};
    vec3 m_velocity{0.0f, 0.0f, 0.0f};
    
    // Orientation
    vec3 m_forward{0.0f, 0.0f, -1.0f};
    vec3 m_up{0.0f, 1.0f, 0.0f};
    vec3 m_right{1.0f, 0.0f, 0.0f};
    
    // Properties
    std::atomic<float> m_volume{1.0f};
    std::atomic<float> m_head_radius{0.0875f}; // Average human head radius
    std::atomic<bool> m_hrtf_enabled{true};
    std::atomic<bool> m_doppler_enabled{true};
    std::atomic<float> m_env_filtering{0.0f};
};

// ECS Components for audio integration

struct AudioSourceComponent : public Component {
    uint32_t source_id = 0;
    std::string audio_file;
    bool auto_play = false;
    bool auto_destroy_on_finish = false;
    float fade_in_time = 0.0f;
    float fade_out_time = 0.0f;
    
    AudioSourceComponent() = default;
    AudioSourceComponent(const std::string& file, bool play = false)
        : audio_file(file), auto_play(play) {}
};

struct AudioListenerComponent : public Component {
    uint32_t listener_id = 0;
    bool is_active = false;
    
    AudioListenerComponent() = default;
    AudioListenerComponent(bool active) : is_active(active) {}
};

struct AudioEmitterComponent : public Component {
    std::vector<uint32_t> source_ids;
    float emission_radius = 1.0f;
    float emission_strength = 1.0f;
    bool omnidirectional = true;
    
    AudioEmitterComponent() = default;
    AudioEmitterComponent(float radius, float strength = 1.0f)
        : emission_radius(radius), emission_strength(strength) {}
};

struct AudioOccluderComponent : public Component {
    float occlusion_strength = 1.0f;
    float transmission_loss = 0.8f;
    bool affect_all_sources = true;
    std::vector<uint32_t> affected_sources;
    
    AudioOccluderComponent() = default;
    AudioOccluderComponent(float strength, float loss = 0.8f)
        : occlusion_strength(strength), transmission_loss(loss) {}
};

struct AudioReverbZoneComponent : public Component {
    std::string reverb_preset = "default";
    float room_size = 0.5f;
    float damping = 0.5f;
    float wet_level = 0.3f;
    float dry_level = 0.7f;
    float reverb_time = 1.0f;
    vec3 zone_size{10.0f, 10.0f, 10.0f};
    bool use_physics_bounds = false;
    
    AudioReverbZoneComponent() = default;
    AudioReverbZoneComponent(const std::string& preset, const vec3& size)
        : reverb_preset(preset), zone_size(size) {}
};

struct AudioTriggerComponent : public Component {
    std::string trigger_sound;
    float trigger_volume = 1.0f;
    bool trigger_once = false;
    bool has_triggered = false;
    float trigger_radius = 1.0f;
    std::function<bool(Entity)> trigger_condition;
    
    AudioTriggerComponent() = default;
    AudioTriggerComponent(const std::string& sound, float radius = 1.0f)
        : trigger_sound(sound), trigger_radius(radius) {}
};

struct AudioStreamingComponent : public Component {
    std::string stream_url;
    uint32_t buffer_size = 4096;
    uint32_t buffer_count = 4;
    bool auto_reconnect = true;
    float reconnect_delay = 5.0f;
    bool is_connected = false;
    
    AudioStreamingComponent() = default;
    AudioStreamingComponent(const std::string& url) : stream_url(url) {}
};

} // namespace ecscope::audio