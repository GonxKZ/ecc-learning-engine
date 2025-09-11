#pragma once

#include <cstdint>
#include <complex>
#include <vector>
#include <array>
#include <memory>
#include <string>

namespace ecscope::audio {

// Audio sample types
using AudioSample = float;
using AudioBuffer = std::vector<AudioSample>;
using StereoBuffer = std::array<AudioBuffer, 2>;
using ComplexSample = std::complex<AudioSample>;
using ComplexBuffer = std::vector<ComplexSample>;

// Audio format specifications
struct AudioFormat {
    uint32_t sample_rate = 44100;
    uint16_t channels = 2;
    uint16_t bits_per_sample = 32;
    uint32_t buffer_size = 1024;
    
    bool operator==(const AudioFormat& other) const noexcept {
        return sample_rate == other.sample_rate &&
               channels == other.channels &&
               bits_per_sample == other.bits_per_sample &&
               buffer_size == other.buffer_size;
    }
};

// 3D spatial audio types
struct Vector3f {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    
    Vector3f() = default;
    Vector3f(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    
    Vector3f operator+(const Vector3f& other) const noexcept {
        return {x + other.x, y + other.y, z + other.z};
    }
    
    Vector3f operator-(const Vector3f& other) const noexcept {
        return {x - other.x, y - other.y, z - other.z};
    }
    
    Vector3f operator*(float scalar) const noexcept {
        return {x * scalar, y * scalar, z * scalar};
    }
    
    float magnitude() const noexcept;
    float distance_to(const Vector3f& other) const noexcept;
    Vector3f normalized() const noexcept;
    float dot(const Vector3f& other) const noexcept;
    Vector3f cross(const Vector3f& other) const noexcept;
};

struct Quaternion {
    float w = 1.0f, x = 0.0f, y = 0.0f, z = 0.0f;
    
    Quaternion() = default;
    Quaternion(float w_, float x_, float y_, float z_) : w(w_), x(x_), y(y_), z(z_) {}
    
    static Quaternion from_euler(float pitch, float yaw, float roll);
    Vector3f to_forward() const noexcept;
    Vector3f to_up() const noexcept;
    Vector3f to_right() const noexcept;
    Quaternion normalized() const noexcept;
};

// Listener configuration for 3D audio
struct AudioListener {
    Vector3f position{0.0f, 0.0f, 0.0f};
    Quaternion orientation{1.0f, 0.0f, 0.0f, 0.0f};
    Vector3f velocity{0.0f, 0.0f, 0.0f};
    float gain = 1.0f;
    bool enabled = true;
    
    // HRTF parameters
    float head_radius = 0.0875f;  // Average human head radius in meters
    float ear_distance = 0.165f;  // Distance between ears
};

// Sound source configuration
struct AudioSource {
    Vector3f position{0.0f, 0.0f, 0.0f};
    Vector3f velocity{0.0f, 0.0f, 0.0f};
    Vector3f direction{0.0f, 0.0f, -1.0f};  // Forward direction
    
    float gain = 1.0f;
    float pitch = 1.0f;
    float min_distance = 1.0f;
    float max_distance = 100.0f;
    float rolloff_factor = 1.0f;
    float cone_inner_angle = 360.0f;  // In degrees
    float cone_outer_angle = 360.0f;
    float cone_outer_gain = 0.0f;
    
    bool looping = false;
    bool relative = false;  // Position relative to listener
    bool enabled = true;
};

// Distance attenuation models
enum class AttenuationModel {
    INVERSE,
    INVERSE_CLAMPED,
    LINEAR,
    LINEAR_CLAMPED,
    EXPONENTIAL,
    EXPONENTIAL_CLAMPED
};

// Audio file formats
enum class AudioFileFormat {
    UNKNOWN,
    WAV,
    MP3,
    OGG,
    FLAC,
    AAC,
    M4A
};

// Audio processing states
enum class AudioState {
    STOPPED,
    PLAYING,
    PAUSED,
    BUFFERING,
    ERROR
};

// HRTF data structure
struct HRTFData {
    uint32_t sample_rate;
    uint32_t length;
    uint32_t azimuth_count;
    uint32_t elevation_count;
    std::vector<AudioBuffer> left_responses;   // HRIR for left ear
    std::vector<AudioBuffer> right_responses;  // HRIR for right ear
    std::vector<float> delays;                 // ITD values
    
    // Spherical coordinate mapping
    std::vector<float> azimuths;   // Azimuth angles in degrees
    std::vector<float> elevations; // Elevation angles in degrees
};

// Environmental audio parameters
struct EnvironmentalAudio {
    float room_size = 10.0f;
    float damping = 0.5f;
    float wet_gain = 0.3f;
    float dry_gain = 0.7f;
    float width = 1.0f;
    float freezemode = 0.0f;
    
    // Material properties for surfaces
    struct MaterialProperties {
        float absorption = 0.1f;    // 0 = reflective, 1 = absorptive
        float scattering = 0.1f;    // Surface roughness
        float transmission = 0.0f;   // Sound transmission through material
    };
    
    std::vector<MaterialProperties> materials;
};

// Ambisonics configuration
struct AmbisonicsConfig {
    uint32_t order = 1;  // First-order ambisonics (4 channels)
    uint32_t channels() const { return (order + 1) * (order + 1); }
    
    enum class Format {
        ACN_SN3D,  // Ambisonic Channel Number with SN3D normalization
        ACN_N3D,   // Ambisonic Channel Number with N3D normalization
        FuMa       // Traditional B-format
    } format = Format::ACN_SN3D;
};

// Audio analysis data
struct AudioAnalysis {
    std::vector<float> spectrum;        // Frequency spectrum
    std::vector<float> mel_spectrum;    // Mel-scale spectrum
    std::vector<float> mfcc;           // Mel-frequency cepstral coefficients
    float rms_level = 0.0f;            // RMS level
    float peak_level = 0.0f;           // Peak level
    float zero_crossing_rate = 0.0f;   // Zero crossing rate
    float spectral_centroid = 0.0f;    // Spectral centroid
    float spectral_rolloff = 0.0f;     // Spectral rolloff
    float spectral_flux = 0.0f;        // Spectral flux
};

// Error codes
enum class AudioError {
    NONE,
    DEVICE_NOT_FOUND,
    FORMAT_NOT_SUPPORTED,
    BUFFER_UNDERRUN,
    BUFFER_OVERRUN,
    FILE_NOT_FOUND,
    DECODE_ERROR,
    INITIALIZATION_FAILED,
    MEMORY_ERROR,
    THREAD_ERROR,
    INVALID_PARAMETER
};

// Audio callback function type
using AudioCallback = std::function<void(AudioBuffer& buffer, const AudioFormat& format)>;

// Performance metrics
struct AudioMetrics {
    float cpu_usage = 0.0f;           // CPU usage percentage
    uint64_t samples_processed = 0;    // Total samples processed
    uint32_t buffer_underruns = 0;     // Number of buffer underruns
    uint32_t buffer_overruns = 0;      // Number of buffer overruns
    float latency_ms = 0.0f;          // Current latency in milliseconds
    uint32_t active_voices = 0;        // Number of active audio voices
    size_t memory_usage = 0;           // Memory usage in bytes
};

} // namespace ecscope::audio