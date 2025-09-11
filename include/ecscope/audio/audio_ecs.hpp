#pragma once

#include "audio_types.hpp"
#include "audio_3d.hpp"
#include "audio_effects.hpp"
#include "../ecs/component.hpp"
#include "../ecs/system.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

namespace ecscope::audio {

// Forward declarations
class Audio3DEngine;
class AudioPipeline;
class AudioStreamManager;

// Audio source component for ECS entities
struct AudioSourceComponent : public ecs::Component<AudioSourceComponent> {
    // Audio source properties
    std::string audio_file_path;
    std::unique_ptr<AudioStream> audio_stream;
    uint32_t voice_id = 0;  // ID in the 3D audio engine
    
    // 3D positioning (can override transform component)
    bool use_transform_position = true;
    Vector3f local_position{0.0f, 0.0f, 0.0f};
    Vector3f velocity{0.0f, 0.0f, 0.0f};
    
    // Audio properties
    float gain = 1.0f;
    float pitch = 1.0f;
    float min_distance = 1.0f;
    float max_distance = 100.0f;
    float rolloff_factor = 1.0f;
    AttenuationModel attenuation_model = AttenuationModel::INVERSE_CLAMPED;
    
    // Directional audio (cone)
    Vector3f direction{0.0f, 0.0f, -1.0f};
    float cone_inner_angle = 360.0f;
    float cone_outer_angle = 360.0f;
    float cone_outer_gain = 0.0f;
    
    // Playback control
    bool auto_play = false;
    bool looping = false;
    bool spatial_audio = true;
    AudioState current_state = AudioState::STOPPED;
    
    // Advanced features
    float doppler_factor = 1.0f;
    bool air_absorption = true;
    bool distance_delay = true;
    float occlusion_factor = 0.0f;
    float obstruction_factor = 0.0f;
    
    // Effects chain
    std::unique_ptr<EffectsChain> effects_chain;
    
    // Serialization support
    void serialize(ecs::ComponentSerializer& serializer) const;
    void deserialize(ecs::ComponentDeserializer& deserializer);
    
    // Component lifecycle
    void on_create();
    void on_destroy();
    void on_enable();
    void on_disable();
};

// Audio listener component (typically one per scene or player)
struct AudioListenerComponent : public ecs::Component<AudioListenerComponent> {
    // Listener properties (can override transform component)
    bool use_transform_position = true;
    Vector3f local_position{0.0f, 0.0f, 0.0f};
    Quaternion local_orientation{1.0f, 0.0f, 0.0f, 0.0f};
    Vector3f velocity{0.0f, 0.0f, 0.0f};
    
    // Audio properties
    float gain = 1.0f;
    bool enabled = true;
    uint32_t listener_id = 0;  // ID in the 3D audio engine
    
    // HRTF parameters
    float head_radius = 0.0875f;
    float ear_distance = 0.165f;
    
    // Multi-listener support
    bool is_primary = true;
    int priority = 0;
    
    // Head tracking for VR/AR
    bool head_tracking_enabled = false;
    Vector3f head_offset{0.0f, 0.0f, 0.0f};
    
    // Serialization support
    void serialize(ecs::ComponentSerializer& serializer) const;
    void deserialize(ecs::ComponentDeserializer& deserializer);
    
    // Component lifecycle
    void on_create();
    void on_destroy();
    void on_enable();
    void on_disable();
};

// Audio zone component for environmental audio
struct AudioZoneComponent : public ecs::Component<AudioZoneComponent> {
    // Zone properties
    enum class ZoneShape {
        SPHERE,
        BOX,
        CYLINDER,
        CUSTOM_MESH
    };
    
    ZoneShape shape = ZoneShape::SPHERE;
    Vector3f size{10.0f, 10.0f, 10.0f};  // Radius for sphere, dimensions for box
    bool inside_triggers_effect = true;
    
    // Environmental audio settings
    EnvironmentalAudio environment_settings;
    
    // Transition properties
    float fade_distance = 1.0f;
    float transition_time = 0.5f;  // seconds
    
    // Zone effects
    std::unique_ptr<EffectsChain> zone_effects;
    std::vector<std::string> ambient_sounds;
    
    // Occlusion geometry
    bool provides_occlusion = false;
    AcousticMaterial material;
    
    // Serialization support
    void serialize(ecs::ComponentSerializer& serializer) const;
    void deserialize(ecs::ComponentDeserializer& deserializer);
    
    // Zone testing
    bool is_point_inside(const Vector3f& point, const Vector3f& zone_center) const;
    float get_distance_factor(const Vector3f& point, const Vector3f& zone_center) const;
};

// Audio emitter component for procedural/synthesized audio
struct AudioEmitterComponent : public ecs::Component<AudioEmitterComponent> {
    // Synthesis parameters
    enum class EmitterType {
        SINE_WAVE,
        SQUARE_WAVE,
        SAWTOOTH_WAVE,
        TRIANGLE_WAVE,
        WHITE_NOISE,
        PINK_NOISE,
        BROWN_NOISE,
        CUSTOM_GENERATOR
    };
    
    EmitterType emitter_type = EmitterType::SINE_WAVE;
    float frequency = 440.0f;
    float amplitude = 0.5f;
    float phase = 0.0f;
    
    // Modulation
    struct Modulation {
        bool enabled = false;
        float frequency = 1.0f;
        float depth = 0.1f;
        float phase_offset = 0.0f;
    };
    
    Modulation frequency_modulation;
    Modulation amplitude_modulation;
    
    // Custom generator function
    std::function<float(float)> custom_generator;
    
    // Playback control
    bool active = false;
    float duration = -1.0f;  // -1 = infinite
    float fade_in_time = 0.0f;
    float fade_out_time = 0.0f;
    
    // 3D properties (inherits from AudioSourceComponent behavior)
    float gain = 1.0f;
    bool spatial_audio = true;
    
    // Serialization support (excluding function pointers)
    void serialize(ecs::ComponentSerializer& serializer) const;
    void deserialize(ecs::ComponentDeserializer& deserializer);
};

// Audio stream component for streaming large audio files
struct AudioStreamComponent : public ecs::Component<AudioStreamComponent> {
    // Streaming properties
    std::string stream_url;  // File path or network URL
    bool is_network_stream = false;
    size_t buffer_size = 8192;
    size_t num_buffers = 4;
    
    // Streaming state
    bool auto_stream = true;
    bool preload_amount = 2.0f;  // seconds to preload
    AudioState stream_state = AudioState::STOPPED;
    
    // Quality settings
    bool allow_format_conversion = true;
    AudioFormat preferred_format;
    int resampling_quality = 5;
    
    // Network streaming specific
    float network_timeout = 10.0f;  // seconds
    size_t network_buffer_size = 65536;
    bool enable_buffering = true;
    
    // Callbacks
    std::function<void(AudioState)> on_state_change;
    std::function<void(float)> on_buffer_update;  // buffer percentage
    std::function<void(const std::string&)> on_error;
    
    // Serialization support (excluding callbacks)
    void serialize(ecs::ComponentSerializer& serializer) const;
    void deserialize(ecs::ComponentDeserializer& deserializer);
};

// Audio system for managing all audio components
class AudioSystem : public ecs::System {
public:
    AudioSystem();
    ~AudioSystem() override;
    
    // System lifecycle
    void initialize() override;
    void shutdown() override;
    void update(float delta_time) override;
    
    // Component management
    void on_component_added(ecs::Entity entity, AudioSourceComponent& component);
    void on_component_removed(ecs::Entity entity, AudioSourceComponent& component);
    void on_component_added(ecs::Entity entity, AudioListenerComponent& component);
    void on_component_removed(ecs::Entity entity, AudioListenerComponent& component);
    
    // Audio engine access
    Audio3DEngine& get_3d_engine();
    AudioPipeline& get_pipeline();
    AudioStreamManager& get_stream_manager();
    
    // Global audio control
    void set_master_volume(float volume);
    float get_master_volume() const;
    void set_global_pause(bool paused);
    bool is_globally_paused() const;
    
    // Scene management
    void load_audio_scene(const std::string& scene_file);
    void save_audio_scene(const std::string& scene_file);
    void clear_audio_scene();
    
    // Performance monitoring
    AudioMetrics get_system_metrics() const;
    size_t get_active_sources() const;
    size_t get_active_listeners() const;

private:
    void update_audio_sources(float delta_time);
    void update_audio_listeners(float delta_time);
    void update_audio_zones(float delta_time);
    void update_audio_emitters(float delta_time);
    void update_audio_streams(float delta_time);
    
    void process_spatial_audio();
    void handle_zone_transitions();
    void manage_voice_priorities();
    
    std::unique_ptr<Audio3DEngine> m_3d_engine;
    std::unique_ptr<AudioPipeline> m_pipeline;
    std::unique_ptr<AudioStreamManager> m_stream_manager;
    
    // Component tracking
    std::unordered_map<ecs::Entity, uint32_t> m_entity_to_voice_id;
    std::unordered_map<ecs::Entity, uint32_t> m_entity_to_listener_id;
    std::unordered_map<uint32_t, ecs::Entity> m_voice_id_to_entity;
    std::unordered_map<uint32_t, ecs::Entity> m_listener_id_to_entity;
    
    // Global state
    float m_master_volume = 1.0f;
    bool m_globally_paused = false;
    uint32_t m_primary_listener_id = 0;
    
    // Performance tracking
    mutable AudioMetrics m_system_metrics;
};

// Audio zone system for managing environmental audio
class AudioZoneSystem : public ecs::System {
public:
    AudioZoneSystem();
    ~AudioZoneSystem() override;
    
    void initialize() override;
    void shutdown() override;
    void update(float delta_time) override;
    
    // Zone management
    void on_component_added(ecs::Entity entity, AudioZoneComponent& component);
    void on_component_removed(ecs::Entity entity, AudioZoneComponent& component);
    
    // Zone queries
    std::vector<ecs::Entity> get_zones_containing_point(const Vector3f& point);
    EnvironmentalAudio calculate_combined_environment(const Vector3f& point);
    float calculate_occlusion_at_point(const Vector3f& source, const Vector3f& listener);
    
    // Ambient sound management
    void start_ambient_sounds_in_zone(ecs::Entity zone_entity);
    void stop_ambient_sounds_in_zone(ecs::Entity zone_entity);
    void update_ambient_sound_volumes(ecs::Entity zone_entity, float volume_factor);

private:
    struct ZoneState {
        ecs::Entity entity;
        bool listener_inside = false;
        float transition_factor = 0.0f;
        std::vector<uint32_t> ambient_voice_ids;
    };
    
    void update_zone_transitions(float delta_time);
    void handle_listener_zone_changes(const Vector3f& listener_pos);
    void interpolate_environmental_settings(const EnvironmentalAudio& from,
                                          const EnvironmentalAudio& to,
                                          float factor, EnvironmentalAudio& result);
    
    std::unordered_map<ecs::Entity, ZoneState> m_zone_states;
    EnvironmentalAudio m_current_environment;
    Vector3f m_last_listener_position{0.0f, 0.0f, 0.0f};
    
    // Reference to audio system for environmental processing
    AudioSystem* m_audio_system = nullptr;
};

// Audio streaming system for managing streamed audio
class AudioStreamingSystem : public ecs::System {
public:
    AudioStreamingSystem();
    ~AudioStreamingSystem() override;
    
    void initialize() override;
    void shutdown() override;
    void update(float delta_time) override;
    
    // Stream management
    void on_component_added(ecs::Entity entity, AudioStreamComponent& component);
    void on_component_removed(ecs::Entity entity, AudioStreamComponent& component);
    
    // Stream control
    void start_stream(ecs::Entity entity);
    void stop_stream(ecs::Entity entity);
    void pause_stream(ecs::Entity entity);
    void resume_stream(ecs::Entity entity);
    
    // Network streaming
    void enable_network_streaming(bool enable);
    void set_network_buffer_size(size_t buffer_size);
    void set_connection_timeout(float timeout_seconds);
    
    // Performance monitoring
    size_t get_active_streams() const;
    float get_total_bandwidth_usage() const;
    size_t get_total_buffered_data() const;

private:
    struct StreamState {
        ecs::Entity entity;
        std::unique_ptr<AudioStream> stream;
        AudioState current_state = AudioState::STOPPED;
        float buffer_percentage = 0.0f;
        size_t bytes_streamed = 0;
        float last_update_time = 0.0f;
    };
    
    void update_stream_states(float delta_time);
    void handle_stream_buffering(StreamState& state);
    void process_network_streams();
    
    std::unordered_map<ecs::Entity, StreamState> m_stream_states;
    std::unique_ptr<AudioStreamManager> m_stream_manager;
    
    bool m_network_streaming_enabled = true;
    size_t m_network_buffer_size = 65536;
    float m_connection_timeout = 10.0f;
};

// Audio event system for trigger-based audio
class AudioEventSystem : public ecs::System {
public:
    struct AudioEvent {
        std::string name;
        std::string audio_file;
        float volume = 1.0f;
        float pitch = 1.0f;
        bool spatial = false;
        Vector3f position{0.0f, 0.0f, 0.0f};
        float min_distance = 1.0f;
        float max_distance = 100.0f;
        
        // Randomization
        float volume_variation = 0.0f;    // ±variation
        float pitch_variation = 0.0f;     // ±variation
        std::vector<std::string> random_files;  // Random file selection
        
        // Cooldown
        float cooldown_time = 0.0f;
        float last_played_time = -1.0f;
    };
    
    AudioEventSystem();
    ~AudioEventSystem() override;
    
    void initialize() override;
    void shutdown() override;
    void update(float delta_time) override;
    
    // Event management
    void register_event(const std::string& name, const AudioEvent& event);
    void unregister_event(const std::string& name);
    bool has_event(const std::string& name) const;
    
    // Event triggering
    void trigger_event(const std::string& name);
    void trigger_event_at_position(const std::string& name, const Vector3f& position);
    void trigger_event_on_entity(const std::string& name, ecs::Entity entity);
    
    // Event loading from files
    void load_events_from_file(const std::string& filepath);
    void save_events_to_file(const std::string& filepath);
    
    // Global event control
    void set_global_event_volume(float volume);
    void enable_events(bool enable);
    void clear_all_events();

private:
    void update_event_cooldowns(float delta_time);
    AudioEvent apply_randomization(const AudioEvent& base_event);
    bool can_play_event(const AudioEvent& event, float current_time);
    
    std::unordered_map<std::string, AudioEvent> m_events;
    float m_global_event_volume = 1.0f;
    bool m_events_enabled = true;
    
    AudioSystem* m_audio_system = nullptr;
};

// ECS audio utilities
namespace ecs_audio_utils {
    // Component queries
    std::vector<ecs::Entity> find_entities_with_audio_sources();
    std::vector<ecs::Entity> find_entities_with_audio_listeners();
    std::vector<ecs::Entity> find_nearby_audio_sources(const Vector3f& position, float radius);
    
    // Audio source utilities
    void play_audio_on_entity(ecs::Entity entity);
    void stop_audio_on_entity(ecs::Entity entity);
    void set_audio_volume(ecs::Entity entity, float volume);
    void set_audio_pitch(ecs::Entity entity, float pitch);
    
    // Batch operations
    void play_all_audio_sources();
    void stop_all_audio_sources();
    void pause_all_audio_sources();
    void resume_all_audio_sources();
    
    // Listener utilities
    ecs::Entity find_primary_listener();
    Vector3f get_listener_position(ecs::Entity listener_entity);
    Quaternion get_listener_orientation(ecs::Entity listener_entity);
    
    // Zone utilities
    std::vector<ecs::Entity> find_audio_zones_at_position(const Vector3f& position);
    EnvironmentalAudio get_environmental_audio_at_position(const Vector3f& position);
    
    // Performance utilities
    void cull_distant_audio_sources(float max_distance);
    void optimize_audio_lod(const Vector3f& listener_position);
    void cleanup_finished_audio_sources();
    
    // Serialization helpers
    void save_audio_components_to_json(const std::string& filepath);
    void load_audio_components_from_json(const std::string& filepath);
}

} // namespace ecscope::audio