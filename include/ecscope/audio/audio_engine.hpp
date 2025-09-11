#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <thread>
#include <functional>
#include <chrono>

#include "../math.hpp"
#include "../ecs/entity.hpp"
#include "../ecs/component.hpp"

namespace ecscope::audio {

// Forward declarations
class AudioDevice;
class AudioBuffer;
class AudioSource;
class AudioListener;
class HRTFProcessor;
class AudioEffects;
class AudioMixer;
class AudioStreamer;
class EnvironmentalAudio;

// Audio configuration structures
struct AudioConfig {
    uint32_t sample_rate = 48000;
    uint32_t buffer_size = 1024;
    uint32_t channels = 2;
    uint32_t bit_depth = 32;
    bool enable_hrtf = true;
    bool enable_doppler = true;
    bool enable_occlusion = true;
    bool enable_reverb = true;
    bool enable_streaming = true;
    uint32_t max_sources = 256;
    uint32_t max_listeners = 4;
    float master_volume = 1.0f;
    std::string audio_backend = "auto"; // auto, wasapi, alsa, coreaudio
};

struct AudioStats {
    uint32_t active_sources = 0;
    uint32_t streaming_sources = 0;
    uint32_t processed_samples = 0;
    float cpu_usage = 0.0f;
    float memory_usage = 0.0f;
    float latency_ms = 0.0f;
    uint32_t buffer_underruns = 0;
    uint32_t buffer_overruns = 0;
    std::chrono::high_resolution_clock::time_point last_update;
};

// Audio format enumeration
enum class AudioFormat {
    PCM_S16,
    PCM_S24,
    PCM_S32,
    PCM_F32,
    MP3,
    OGG,
    FLAC,
    WAV
};

// Audio backend interface
class AudioBackend {
public:
    virtual ~AudioBackend() = default;
    
    virtual bool initialize(const AudioConfig& config) = 0;
    virtual void shutdown() = 0;
    virtual bool start() = 0;
    virtual bool stop() = 0;
    virtual bool is_running() const = 0;
    virtual uint32_t get_sample_rate() const = 0;
    virtual uint32_t get_buffer_size() const = 0;
    virtual uint32_t get_channels() const = 0;
    virtual float get_latency() const = 0;
    virtual void set_audio_callback(std::function<void(float*, uint32_t)> callback) = 0;
    virtual std::string get_backend_name() const = 0;
};

// Main audio engine class
class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();
    
    // Core functionality
    bool initialize(const AudioConfig& config = AudioConfig{});
    void shutdown();
    bool is_initialized() const { return m_initialized; }
    
    // Device management
    std::vector<std::string> get_available_devices() const;
    bool set_output_device(const std::string& device_name);
    std::string get_current_device() const;
    
    // Source management
    uint32_t create_source();
    void destroy_source(uint32_t source_id);
    AudioSource* get_source(uint32_t source_id);
    
    // Listener management
    uint32_t create_listener();
    void destroy_listener(uint32_t listener_id);
    AudioListener* get_listener(uint32_t listener_id);
    void set_active_listener(uint32_t listener_id);
    uint32_t get_active_listener() const { return m_active_listener; }
    
    // Buffer management
    uint32_t create_buffer();
    void destroy_buffer(uint32_t buffer_id);
    AudioBuffer* get_buffer(uint32_t buffer_id);
    
    // Audio loading and streaming
    uint32_t load_audio(const std::string& filename);
    uint32_t create_streaming_source(const std::string& filename);
    bool unload_audio(uint32_t buffer_id);
    
    // Global audio settings
    void set_master_volume(float volume);
    float get_master_volume() const { return m_master_volume; }
    void set_doppler_factor(float factor);
    float get_doppler_factor() const { return m_doppler_factor; }
    void set_speed_of_sound(float speed);
    float get_speed_of_sound() const { return m_speed_of_sound; }
    
    // Environmental audio
    void set_environmental_preset(const std::string& preset);
    void set_reverb_parameters(float room_size, float damping, float wet_level);
    void enable_occlusion(bool enable) { m_occlusion_enabled = enable; }
    bool is_occlusion_enabled() const { return m_occlusion_enabled; }
    
    // Performance and debugging
    const AudioStats& get_stats() const { return m_stats; }
    void reset_stats();
    void set_debug_mode(bool enable) { m_debug_mode = enable; }
    bool is_debug_mode() const { return m_debug_mode; }
    
    // HRTF control
    bool set_hrtf_dataset(const std::string& dataset_path);
    void enable_hrtf(bool enable);
    bool is_hrtf_enabled() const;
    
    // Audio processing callbacks
    void set_pre_mix_callback(std::function<void(float*, uint32_t)> callback);
    void set_post_mix_callback(std::function<void(float*, uint32_t)> callback);
    
    // Thread safety
    void lock_audio_thread() { m_audio_mutex.lock(); }
    void unlock_audio_thread() { m_audio_mutex.unlock(); }
    
    // Update function (called from main thread)
    void update(float delta_time);
    
private:
    // Audio processing thread function
    void audio_thread_function();
    void audio_callback(float* output_buffer, uint32_t frame_count);
    
    // Internal management
    void cleanup_finished_sources();
    void update_statistics();
    uint32_t generate_id();
    
    // Configuration and state
    AudioConfig m_config;
    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_shutdown_requested{false};
    
    // Backend
    std::unique_ptr<AudioBackend> m_backend;
    
    // Audio components
    std::unique_ptr<HRTFProcessor> m_hrtf_processor;
    std::unique_ptr<AudioMixer> m_mixer;
    std::unique_ptr<AudioStreamer> m_streamer;
    std::unique_ptr<EnvironmentalAudio> m_environmental_audio;
    
    // Resource management
    std::unordered_map<uint32_t, std::unique_ptr<AudioSource>> m_sources;
    std::unordered_map<uint32_t, std::unique_ptr<AudioListener>> m_listeners;
    std::unordered_map<uint32_t, std::unique_ptr<AudioBuffer>> m_buffers;
    
    // State
    std::atomic<uint32_t> m_next_id{1};
    std::atomic<uint32_t> m_active_listener{0};
    std::atomic<float> m_master_volume{1.0f};
    std::atomic<float> m_doppler_factor{1.0f};
    std::atomic<float> m_speed_of_sound{343.3f};
    std::atomic<bool> m_occlusion_enabled{true};
    std::atomic<bool> m_debug_mode{false};
    
    // Threading
    std::thread m_audio_thread;
    mutable std::mutex m_audio_mutex;
    mutable std::mutex m_resource_mutex;
    
    // Statistics
    mutable AudioStats m_stats;
    mutable std::mutex m_stats_mutex;
    
    // Callbacks
    std::function<void(float*, uint32_t)> m_pre_mix_callback;
    std::function<void(float*, uint32_t)> m_post_mix_callback;
    
    // Cleanup lists (for thread-safe resource cleanup)
    std::vector<uint32_t> m_sources_to_cleanup;
    std::vector<uint32_t> m_listeners_to_cleanup;
    std::vector<uint32_t> m_buffers_to_cleanup;
};

// Global audio engine instance
AudioEngine& get_audio_engine();

} // namespace ecscope::audio