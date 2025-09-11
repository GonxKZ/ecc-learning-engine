#pragma once

#include "ecscope/web/web_types.hpp"
#include <vector>
#include <memory>
#include <unordered_map>

namespace ecscope::web {

/**
 * @brief Web Audio API integration for browser audio
 * 
 * This class provides a comprehensive audio system using the Web Audio API,
 * supporting spatial audio, effects processing, and real-time audio synthesis.
 */
class WebAudio {
public:
    /**
     * @brief Audio node types
     */
    enum class NodeType {
        Source,
        Gain,
        Filter,
        Delay,
        Reverb,
        Compressor,
        Analyzer,
        Panner,
        Destination
    };
    
    /**
     * @brief Audio buffer format
     */
    struct AudioBuffer {
        std::uint32_t id;
        std::uint32_t sample_rate;
        std::uint32_t channels;
        std::uint32_t length;
        std::vector<float> data;
        double duration;
    };
    
    /**
     * @brief Audio source configuration
     */
    struct AudioSource {
        std::uint32_t id;
        std::uint32_t buffer_id;
        bool looping;
        float volume;
        float pitch;
        float pan;
        bool spatial;
        float x, y, z;  // 3D position
        bool playing;
    };
    
    /**
     * @brief 3D audio listener configuration
     */
    struct AudioListener {
        float x, y, z;  // Position
        float forward_x, forward_y, forward_z;  // Forward vector
        float up_x, up_y, up_z;  // Up vector
        float velocity_x, velocity_y, velocity_z;  // Velocity for Doppler
    };
    
    /**
     * @brief Audio effect parameters
     */
    struct EffectParameters {
        // Filter
        float frequency = 1000.0f;
        float q = 1.0f;
        float gain = 0.0f;
        
        // Delay
        float delay_time = 0.3f;
        float feedback = 0.3f;
        float mix = 0.5f;
        
        // Reverb
        float room_size = 0.5f;
        float decay_time = 1.5f;
        float damping = 0.5f;
        float wet_mix = 0.3f;
        
        // Compressor
        float threshold = -24.0f;
        float ratio = 12.0f;
        float attack = 0.003f;
        float release = 0.25f;
        
        // Distortion
        float drive = 5.0f;
        float output_gain = 0.5f;
    };
    
    /**
     * @brief Construct a new WebAudio system
     * @param config Audio configuration
     */
    explicit WebAudio(const WebAudioConfig& config);
    
    /**
     * @brief Destructor
     */
    ~WebAudio();
    
    // Non-copyable, movable
    WebAudio(const WebAudio&) = delete;
    WebAudio& operator=(const WebAudio&) = delete;
    WebAudio(WebAudio&&) noexcept = default;
    WebAudio& operator=(WebAudio&&) noexcept = default;
    
    /**
     * @brief Initialize the audio system
     * @return true if initialization succeeded
     */
    bool initialize();
    
    /**
     * @brief Shutdown the audio system
     */
    void shutdown();
    
    /**
     * @brief Update audio system (called each frame)
     * @param delta_time Time since last update
     */
    void update(double delta_time);
    
    /**
     * @brief Resume audio context (required after user gesture)
     * @return true if context resumed successfully
     */
    bool resume_context();
    
    /**
     * @brief Suspend audio context
     * @return true if context suspended successfully
     */
    bool suspend_context();
    
    /**
     * @brief Check if audio context is running
     * @return true if context is running
     */
    bool is_context_running() const;
    
    /**
     * @brief Load audio buffer from URL
     * @param url Audio file URL
     * @param callback Completion callback
     * @return Buffer ID (0 if failed)
     */
    std::uint32_t load_audio_buffer(const std::string& url, 
                                   std::function<void(std::uint32_t, bool)> callback);
    
    /**
     * @brief Create audio buffer from raw data
     * @param sample_rate Sample rate in Hz
     * @param channels Number of channels
     * @param data Audio samples (interleaved)
     * @return Buffer ID (0 if failed)
     */
    std::uint32_t create_audio_buffer(std::uint32_t sample_rate, std::uint32_t channels, 
                                     const std::vector<float>& data);
    
    /**
     * @brief Delete audio buffer
     * @param buffer_id Buffer ID
     */
    void delete_audio_buffer(std::uint32_t buffer_id);
    
    /**
     * @brief Create audio source
     * @param buffer_id Buffer to play
     * @return Source ID (0 if failed)
     */
    std::uint32_t create_audio_source(std::uint32_t buffer_id);
    
    /**
     * @brief Delete audio source
     * @param source_id Source ID
     */
    void delete_audio_source(std::uint32_t source_id);
    
    /**
     * @brief Play audio source
     * @param source_id Source ID
     * @param start_time When to start (0 = immediately)
     */
    void play_source(std::uint32_t source_id, double start_time = 0.0);
    
    /**
     * @brief Stop audio source
     * @param source_id Source ID
     * @param stop_time When to stop (0 = immediately)
     */
    void stop_source(std::uint32_t source_id, double stop_time = 0.0);
    
    /**
     * @brief Pause audio source
     * @param source_id Source ID
     */
    void pause_source(std::uint32_t source_id);
    
    /**
     * @brief Resume audio source
     * @param source_id Source ID
     */
    void resume_source(std::uint32_t source_id);
    
    /**
     * @brief Set source volume
     * @param source_id Source ID
     * @param volume Volume (0.0 to 1.0)
     */
    void set_source_volume(std::uint32_t source_id, float volume);
    
    /**
     * @brief Set source pitch
     * @param source_id Source ID
     * @param pitch Pitch multiplier
     */
    void set_source_pitch(std::uint32_t source_id, float pitch);
    
    /**
     * @brief Set source looping
     * @param source_id Source ID
     * @param looping Whether to loop
     */
    void set_source_looping(std::uint32_t source_id, bool looping);
    
    /**
     * @brief Set source 3D position
     * @param source_id Source ID
     * @param x X coordinate
     * @param y Y coordinate
     * @param z Z coordinate
     */
    void set_source_position(std::uint32_t source_id, float x, float y, float z);
    
    /**
     * @brief Set source velocity (for Doppler effect)
     * @param source_id Source ID
     * @param vx X velocity
     * @param vy Y velocity
     * @param vz Z velocity
     */
    void set_source_velocity(std::uint32_t source_id, float vx, float vy, float vz);
    
    /**
     * @brief Set listener position and orientation
     * @param listener Listener configuration
     */
    void set_listener(const AudioListener& listener);
    
    /**
     * @brief Set master volume
     * @param volume Master volume (0.0 to 1.0)
     */
    void set_master_volume(float volume);
    
    /**
     * @brief Get master volume
     * @return Current master volume
     */
    float get_master_volume() const;
    
    /**
     * @brief Create audio effect node
     * @param type Effect type
     * @param params Effect parameters
     * @return Effect node ID (0 if failed)
     */
    std::uint32_t create_effect_node(NodeType type, const EffectParameters& params);
    
    /**
     * @brief Update effect parameters
     * @param node_id Effect node ID
     * @param params New parameters
     */
    void update_effect_parameters(std::uint32_t node_id, const EffectParameters& params);
    
    /**
     * @brief Connect audio nodes
     * @param source_id Source node ID
     * @param destination_id Destination node ID
     */
    void connect_nodes(std::uint32_t source_id, std::uint32_t destination_id);
    
    /**
     * @brief Disconnect audio nodes
     * @param source_id Source node ID
     * @param destination_id Destination node ID (0 = disconnect all)
     */
    void disconnect_nodes(std::uint32_t source_id, std::uint32_t destination_id = 0);
    
    /**
     * @brief Delete effect node
     * @param node_id Effect node ID
     */
    void delete_effect_node(std::uint32_t node_id);
    
    /**
     * @brief Get audio analysis data
     * @param analyzer_id Analyzer node ID
     * @param frequency_data Output frequency data
     * @param time_data Output time domain data
     */
    void get_analysis_data(std::uint32_t analyzer_id, 
                          std::vector<float>& frequency_data,
                          std::vector<float>& time_data);
    
    /**
     * @brief Start audio recording
     * @param callback Data callback for recorded samples
     * @return true if recording started
     */
    bool start_recording(std::function<void(const std::vector<float>&)> callback);
    
    /**
     * @brief Stop audio recording
     */
    void stop_recording();
    
    /**
     * @brief Check if recording is active
     * @return true if recording
     */
    bool is_recording() const;
    
    /**
     * @brief Generate tone
     * @param frequency Frequency in Hz
     * @param duration Duration in seconds
     * @param volume Volume (0.0 to 1.0)
     * @param wave_type Wave type ("sine", "square", "sawtooth", "triangle")
     * @return Generated audio buffer ID
     */
    std::uint32_t generate_tone(float frequency, float duration, float volume, 
                               const std::string& wave_type = "sine");
    
    /**
     * @brief Get current audio time
     * @return Current time in seconds
     */
    double get_current_time() const;
    
    /**
     * @brief Get sample rate
     * @return Audio context sample rate
     */
    std::uint32_t get_sample_rate() const;
    
    /**
     * @brief Check if audio system is initialized
     * @return true if initialized
     */
    bool is_initialized() const noexcept { return initialized_; }
    
private:
    // Configuration
    WebAudioConfig config_;
    
    // State
    bool initialized_ = false;
    bool context_suspended_ = false;
    bool recording_ = false;
    float master_volume_ = 1.0f;
    
    // JavaScript Audio Context handle
    JSValue audio_context_;
    JSValue destination_node_;
    JSValue master_gain_node_;
    
    // Resource tracking
    std::uint32_t next_buffer_id_ = 1;
    std::uint32_t next_source_id_ = 1;
    std::uint32_t next_node_id_ = 1;
    
    std::unordered_map<std::uint32_t, AudioBuffer> audio_buffers_;
    std::unordered_map<std::uint32_t, AudioSource> audio_sources_;
    std::unordered_map<std::uint32_t, JSValue> effect_nodes_;
    
    // Audio processing
    AudioListener listener_;
    std::function<void(const std::vector<float>&)> recording_callback_;
    JSValue microphone_source_;
    JSValue recorder_processor_;
    
    // Internal methods
    bool create_audio_context();
    void setup_master_gain();
    JSValue create_buffer_source(std::uint32_t buffer_id);
    JSValue create_web_audio_buffer(std::uint32_t sample_rate, std::uint32_t channels, 
                                   const std::vector<float>& data);
    void cleanup_resources();
    
    // Static callbacks
    static void audio_process_callback(int input_count, const float** inputs,
                                     int output_count, float** outputs,
                                     int frame_count, void* user_data);
};

} // namespace ecscope::web