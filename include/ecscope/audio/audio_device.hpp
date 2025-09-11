#pragma once

#include "audio_types.hpp"
#include <functional>
#include <memory>
#include <vector>
#include <string>
#include <atomic>
#include <mutex>

namespace ecscope::audio {

// Forward declarations
class AudioBuffer;

// Audio device information
struct AudioDeviceInfo {
    std::string name;
    std::string driver;
    uint32_t id;
    bool is_default = false;
    bool supports_input = false;
    bool supports_output = true;
    std::vector<AudioFormat> supported_formats;
    uint32_t min_buffer_size = 64;
    uint32_t max_buffer_size = 8192;
    float min_sample_rate = 8000.0f;
    float max_sample_rate = 192000.0f;
};

// Cross-platform audio device interface
class AudioDevice {
public:
    virtual ~AudioDevice() = default;
    
    // Device management
    virtual bool initialize(const AudioFormat& format) = 0;
    virtual void shutdown() = 0;
    virtual bool is_initialized() const = 0;
    
    // Audio streaming
    virtual bool start() = 0;
    virtual bool stop() = 0;
    virtual bool is_running() const = 0;
    
    // Buffer management
    virtual void set_callback(AudioCallback callback) = 0;
    virtual uint32_t get_buffer_size() const = 0;
    virtual uint32_t get_sample_rate() const = 0;
    virtual uint32_t get_channels() const = 0;
    
    // Latency and timing
    virtual float get_latency_ms() const = 0;
    virtual uint64_t get_stream_time() const = 0;
    virtual uint64_t get_frames_processed() const = 0;
    
    // Device information
    virtual AudioDeviceInfo get_device_info() const = 0;
    virtual AudioError get_last_error() const = 0;
    virtual std::string get_error_string() const = 0;
    
    // Performance monitoring
    virtual AudioMetrics get_metrics() const = 0;
    virtual void reset_metrics() = 0;

protected:
    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_running{false};
    AudioFormat m_format;
    AudioCallback m_callback;
    mutable std::mutex m_mutex;
    AudioError m_last_error = AudioError::NONE;
    AudioMetrics m_metrics;
};

// Platform-specific device implementations
#ifdef _WIN32
class WASAPIDevice : public AudioDevice {
public:
    WASAPIDevice();
    ~WASAPIDevice() override;
    
    bool initialize(const AudioFormat& format) override;
    void shutdown() override;
    bool is_initialized() const override;
    
    bool start() override;
    bool stop() override;
    bool is_running() const override;
    
    void set_callback(AudioCallback callback) override;
    uint32_t get_buffer_size() const override;
    uint32_t get_sample_rate() const override;
    uint32_t get_channels() const override;
    
    float get_latency_ms() const override;
    uint64_t get_stream_time() const override;
    uint64_t get_frames_processed() const override;
    
    AudioDeviceInfo get_device_info() const override;
    AudioError get_last_error() const override;
    std::string get_error_string() const override;
    
    AudioMetrics get_metrics() const override;
    void reset_metrics() override;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};
#endif

#ifdef __linux__
class ALSADevice : public AudioDevice {
public:
    ALSADevice();
    ~ALSADevice() override;
    
    bool initialize(const AudioFormat& format) override;
    void shutdown() override;
    bool is_initialized() const override;
    
    bool start() override;
    bool stop() override;
    bool is_running() const override;
    
    void set_callback(AudioCallback callback) override;
    uint32_t get_buffer_size() const override;
    uint32_t get_sample_rate() const override;
    uint32_t get_channels() const override;
    
    float get_latency_ms() const override;
    uint64_t get_stream_time() const override;
    uint64_t get_frames_processed() const override;
    
    AudioDeviceInfo get_device_info() const override;
    AudioError get_last_error() const override;
    std::string get_error_string() const override;
    
    AudioMetrics get_metrics() const override;
    void reset_metrics() override;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};
#endif

#ifdef __APPLE__
class CoreAudioDevice : public AudioDevice {
public:
    CoreAudioDevice();
    ~CoreAudioDevice() override;
    
    bool initialize(const AudioFormat& format) override;
    void shutdown() override;
    bool is_initialized() const override;
    
    bool start() override;
    bool stop() override;
    bool is_running() const override;
    
    void set_callback(AudioCallback callback) override;
    uint32_t get_buffer_size() const override;
    uint32_t get_sample_rate() const override;
    uint32_t get_channels() const override;
    
    float get_latency_ms() const override;
    uint64_t get_stream_time() const override;
    uint64_t get_frames_processed() const override;
    
    AudioDeviceInfo get_device_info() const override;
    AudioError get_last_error() const override;
    std::string get_error_string() const override;
    
    AudioMetrics get_metrics() const override;
    void reset_metrics() override;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};
#endif

// Audio device factory
class AudioDeviceFactory {
public:
    static std::unique_ptr<AudioDevice> create_device();
    static std::unique_ptr<AudioDevice> create_device(const std::string& device_name);
    static std::vector<AudioDeviceInfo> enumerate_devices();
    static AudioDeviceInfo get_default_device();
    static bool is_device_available(const std::string& device_name);
    
    // Platform detection
    static std::string get_platform_name();
    static std::vector<std::string> get_available_backends();
};

// Cross-platform audio device manager
class AudioDeviceManager {
public:
    static AudioDeviceManager& instance();
    
    // Device enumeration
    std::vector<AudioDeviceInfo> get_available_devices() const;
    AudioDeviceInfo get_default_output_device() const;
    AudioDeviceInfo get_default_input_device() const;
    
    // Device creation and management
    std::unique_ptr<AudioDevice> create_output_device(const AudioFormat& format);
    std::unique_ptr<AudioDevice> create_output_device(
        const std::string& device_name, 
        const AudioFormat& format
    );
    
    // Device monitoring
    void refresh_device_list();
    bool is_device_connected(const std::string& device_name) const;
    void set_device_change_callback(std::function<void()> callback);
    
    // System audio settings
    float get_master_volume() const;
    void set_master_volume(float volume);
    bool is_muted() const;
    void set_muted(bool muted);

private:
    AudioDeviceManager() = default;
    ~AudioDeviceManager() = default;
    AudioDeviceManager(const AudioDeviceManager&) = delete;
    AudioDeviceManager& operator=(const AudioDeviceManager&) = delete;
    
    void initialize();
    void update_device_list();
    
    mutable std::mutex m_mutex;
    std::vector<AudioDeviceInfo> m_devices;
    std::function<void()> m_device_change_callback;
    bool m_initialized = false;
};

} // namespace ecscope::audio