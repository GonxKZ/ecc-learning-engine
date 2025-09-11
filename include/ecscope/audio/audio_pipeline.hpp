#pragma once

#include "audio_types.hpp"
#include "audio_device.hpp"
#include <memory>
#include <vector>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>
#include <queue>

namespace ecscope::audio {

// Forward declarations
class AudioDecoder;
class AudioEncoder;
class AudioEffect;
class AudioMixer;

// Audio stream interface
class AudioStream {
public:
    virtual ~AudioStream() = default;
    
    // Stream control
    virtual bool open() = 0;
    virtual void close() = 0;
    virtual bool is_open() const = 0;
    
    // Data access
    virtual size_t read(AudioBuffer& buffer, size_t samples) = 0;
    virtual size_t write(const AudioBuffer& buffer) = 0;
    virtual bool seek(uint64_t sample_position) = 0;
    virtual uint64_t tell() const = 0;
    
    // Stream information
    virtual AudioFormat get_format() const = 0;
    virtual uint64_t get_length_samples() const = 0;
    virtual float get_length_seconds() const = 0;
    virtual bool is_seekable() const = 0;
    virtual bool supports_streaming() const = 0;
    
    // State management
    virtual AudioState get_state() const = 0;
    virtual AudioError get_last_error() const = 0;
};

// File-based audio stream
class FileAudioStream : public AudioStream {
public:
    explicit FileAudioStream(const std::string& filepath);
    ~FileAudioStream() override;
    
    bool open() override;
    void close() override;
    bool is_open() const override;
    
    size_t read(AudioBuffer& buffer, size_t samples) override;
    size_t write(const AudioBuffer& buffer) override;
    bool seek(uint64_t sample_position) override;
    uint64_t tell() const override;
    
    AudioFormat get_format() const override;
    uint64_t get_length_samples() const override;
    float get_length_seconds() const override;
    bool is_seekable() const override;
    bool supports_streaming() const override;
    
    AudioState get_state() const override;
    AudioError get_last_error() const override;
    
    // File-specific methods
    std::string get_filepath() const;
    AudioFileFormat get_file_format() const;
    bool set_decode_format(const AudioFormat& format);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

// Memory-based audio stream
class MemoryAudioStream : public AudioStream {
public:
    explicit MemoryAudioStream(const AudioBuffer& data, const AudioFormat& format);
    explicit MemoryAudioStream(AudioBuffer&& data, const AudioFormat& format);
    ~MemoryAudioStream() override;
    
    bool open() override;
    void close() override;
    bool is_open() const override;
    
    size_t read(AudioBuffer& buffer, size_t samples) override;
    size_t write(const AudioBuffer& buffer) override;
    bool seek(uint64_t sample_position) override;
    uint64_t tell() const override;
    
    AudioFormat get_format() const override;
    uint64_t get_length_samples() const override;
    float get_length_seconds() const override;
    bool is_seekable() const override;
    bool supports_streaming() const override;
    
    AudioState get_state() const override;
    AudioError get_last_error() const override;

private:
    AudioBuffer m_data;
    AudioFormat m_format;
    uint64_t m_position = 0;
    bool m_open = false;
    AudioError m_last_error = AudioError::NONE;
};

// Audio decoder interface
class AudioDecoder {
public:
    virtual ~AudioDecoder() = default;
    
    // Decoder capabilities
    virtual std::vector<AudioFileFormat> get_supported_formats() const = 0;
    virtual bool can_decode(const std::string& filepath) const = 0;
    virtual bool can_decode(AudioFileFormat format) const = 0;
    
    // Decoding operations
    virtual bool open(const std::string& filepath) = 0;
    virtual void close() = 0;
    virtual bool is_open() const = 0;
    
    virtual size_t decode(AudioBuffer& buffer, size_t max_samples) = 0;
    virtual bool seek(uint64_t sample_position) = 0;
    virtual uint64_t tell() const = 0;
    
    // Stream information
    virtual AudioFormat get_format() const = 0;
    virtual uint64_t get_length_samples() const = 0;
    virtual float get_length_seconds() const = 0;
    virtual std::string get_metadata(const std::string& key) const = 0;
    
    // Error handling
    virtual AudioError get_last_error() const = 0;
    virtual std::string get_error_string() const = 0;
};

// Multi-format audio decoder factory
class AudioDecoderFactory {
public:
    static std::unique_ptr<AudioDecoder> create_decoder(AudioFileFormat format);
    static std::unique_ptr<AudioDecoder> create_decoder(const std::string& filepath);
    static std::vector<AudioFileFormat> get_supported_formats();
    static bool is_format_supported(AudioFileFormat format);
    
    // Register custom decoders
    static void register_decoder(AudioFileFormat format, 
                               std::function<std::unique_ptr<AudioDecoder>()> factory);
};

// Specific decoder implementations
class WAVDecoder : public AudioDecoder {
public:
    std::vector<AudioFileFormat> get_supported_formats() const override;
    bool can_decode(const std::string& filepath) const override;
    bool can_decode(AudioFileFormat format) const override;
    
    bool open(const std::string& filepath) override;
    void close() override;
    bool is_open() const override;
    
    size_t decode(AudioBuffer& buffer, size_t max_samples) override;
    bool seek(uint64_t sample_position) override;
    uint64_t tell() const override;
    
    AudioFormat get_format() const override;
    uint64_t get_length_samples() const override;
    float get_length_seconds() const override;
    std::string get_metadata(const std::string& key) const override;
    
    AudioError get_last_error() const override;
    std::string get_error_string() const override;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

class MP3Decoder : public AudioDecoder {
public:
    std::vector<AudioFileFormat> get_supported_formats() const override;
    bool can_decode(const std::string& filepath) const override;
    bool can_decode(AudioFileFormat format) const override;
    
    bool open(const std::string& filepath) override;
    void close() override;
    bool is_open() const override;
    
    size_t decode(AudioBuffer& buffer, size_t max_samples) override;
    bool seek(uint64_t sample_position) override;
    uint64_t tell() const override;
    
    AudioFormat get_format() const override;
    uint64_t get_length_samples() const override;
    float get_length_seconds() const override;
    std::string get_metadata(const std::string& key) const override;
    
    AudioError get_last_error() const override;
    std::string get_error_string() const override;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

class OGGDecoder : public AudioDecoder {
public:
    std::vector<AudioFileFormat> get_supported_formats() const override;
    bool can_decode(const std::string& filepath) const override;
    bool can_decode(AudioFileFormat format) const override;
    
    bool open(const std::string& filepath) override;
    void close() override;
    bool is_open() const override;
    
    size_t decode(AudioBuffer& buffer, size_t max_samples) override;
    bool seek(uint64_t sample_position) override;
    uint64_t tell() const override;
    
    AudioFormat get_format() const override;
    uint64_t get_length_samples() const override;
    float get_length_seconds() const override;
    std::string get_metadata(const std::string& key) const override;
    
    AudioError get_last_error() const override;
    std::string get_error_string() const override;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

class FLACDecoder : public AudioDecoder {
public:
    std::vector<AudioFileFormat> get_supported_formats() const override;
    bool can_decode(const std::string& filepath) const override;
    bool can_decode(AudioFileFormat format) const override;
    
    bool open(const std::string& filepath) override;
    void close() override;
    bool is_open() const override;
    
    size_t decode(AudioBuffer& buffer, size_t max_samples) override;
    bool seek(uint64_t sample_position) override;
    uint64_t tell() const override;
    
    AudioFormat get_format() const override;
    uint64_t get_length_samples() const override;
    float get_length_seconds() const override;
    std::string get_metadata(const std::string& key) const override;
    
    AudioError get_last_error() const override;
    std::string get_error_string() const override;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

// Audio streaming manager
class AudioStreamManager {
public:
    AudioStreamManager(size_t buffer_size = 8192, size_t num_buffers = 4);
    ~AudioStreamManager();
    
    // Stream management
    std::unique_ptr<AudioStream> create_file_stream(const std::string& filepath);
    std::unique_ptr<AudioStream> create_memory_stream(const AudioBuffer& data, 
                                                     const AudioFormat& format);
    
    // Streaming control
    void enable_streaming(bool enable);
    void set_buffer_size(size_t buffer_size);
    void set_buffer_count(size_t num_buffers);
    void set_prefetch_amount(float seconds);
    
    // Format conversion
    void set_output_format(const AudioFormat& format);
    void enable_resampling(bool enable);
    void set_resampling_quality(int quality);  // 0-10, higher = better quality
    
    // Performance monitoring
    AudioMetrics get_streaming_metrics() const;
    float get_buffer_usage() const;
    size_t get_active_streams() const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

// Audio mixer for combining multiple sources
class AudioMixer {
public:
    struct MixerChannel {
        std::unique_ptr<AudioStream> stream;
        float gain = 1.0f;
        float pan = 0.0f;  // -1.0 (left) to 1.0 (right)
        bool muted = false;
        bool solo = false;
        uint32_t priority = 0;
        
        // Effects chain
        std::vector<std::unique_ptr<AudioEffect>> effects;
    };
    
    AudioMixer(const AudioFormat& output_format, size_t max_channels = 64);
    ~AudioMixer();
    
    // Channel management
    uint32_t add_channel(std::unique_ptr<AudioStream> stream);
    void remove_channel(uint32_t channel_id);
    void clear_channels();
    MixerChannel* get_channel(uint32_t channel_id);
    
    // Mixing control
    void set_master_gain(float gain);
    float get_master_gain() const;
    void set_master_mute(bool muted);
    bool is_master_muted() const;
    
    // Real-time mixing
    size_t mix(AudioBuffer& output, size_t samples);
    size_t mix_stereo(StereoBuffer& output, size_t samples);
    size_t mix_interleaved(AudioBuffer& output, size_t samples);
    
    // Advanced mixing features
    void enable_automatic_gain_control(bool enable);
    void set_limiter_threshold(float threshold_db);
    void enable_duck_on_priority(bool enable);
    void set_crossfade_time(float time_ms);
    
    // Performance monitoring
    AudioMetrics get_mixer_metrics() const;
    size_t get_active_channels() const;
    float get_output_level() const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

// Audio pipeline - complete audio processing chain
class AudioPipeline {
public:
    AudioPipeline();
    ~AudioPipeline();
    
    // Pipeline configuration
    bool initialize(const AudioFormat& format);
    void shutdown();
    bool is_initialized() const;
    
    // Device management
    bool set_output_device(std::unique_ptr<AudioDevice> device);
    AudioDevice* get_output_device();
    
    // Stream management
    uint32_t add_stream(std::unique_ptr<AudioStream> stream);
    void remove_stream(uint32_t stream_id);
    void clear_streams();
    
    // Mixer access
    AudioMixer& get_mixer();
    
    // Real-time processing
    void start();
    void stop();
    void pause();
    void resume();
    AudioState get_state() const;
    
    // Global audio settings
    void set_master_volume(float volume);
    float get_master_volume() const;
    void set_global_pitch(float pitch);
    float get_global_pitch() const;
    
    // Effects processing
    void add_global_effect(std::unique_ptr<AudioEffect> effect);
    void remove_global_effect(size_t index);
    void clear_global_effects();
    
    // Performance monitoring
    AudioMetrics get_pipeline_metrics() const;
    float get_cpu_usage() const;
    float get_memory_usage_mb() const;
    
    // Error handling
    AudioError get_last_error() const;
    std::string get_error_string() const;
    void set_error_callback(std::function<void(AudioError, const std::string&)> callback);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace ecscope::audio