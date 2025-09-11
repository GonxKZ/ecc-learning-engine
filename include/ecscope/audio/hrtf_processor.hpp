#pragma once

#include "audio_types.hpp"
#include <memory>
#include <vector>
#include <array>
#include <complex>
#include <unordered_map>
#include <mutex>

namespace ecscope::audio {

// HRTF interpolation methods
enum class HRTFInterpolation {
    NEAREST,
    LINEAR,
    CUBIC,
    SPHERICAL_LINEAR
};

// HRTF database interface
class HRTFDatabase {
public:
    virtual ~HRTFDatabase() = default;
    
    // Database loading
    virtual bool load_from_file(const std::string& filepath) = 0;
    virtual bool load_from_sofa(const std::string& filepath) = 0;  // SOFA format
    virtual bool load_default_database() = 0;
    
    // HRTF data access
    virtual bool get_hrtf_data(float azimuth, float elevation, 
                              AudioBuffer& left_hrir, AudioBuffer& right_hrir,
                              float& left_delay, float& right_delay) const = 0;
    
    virtual bool get_interpolated_hrtf(float azimuth, float elevation,
                                     HRTFInterpolation method,
                                     AudioBuffer& left_hrir, AudioBuffer& right_hrir,
                                     float& left_delay, float& right_delay) const = 0;
    
    // Database information
    virtual uint32_t get_sample_rate() const = 0;
    virtual uint32_t get_hrir_length() const = 0;
    virtual uint32_t get_azimuth_count() const = 0;
    virtual uint32_t get_elevation_count() const = 0;
    virtual std::vector<float> get_available_azimuths() const = 0;
    virtual std::vector<float> get_available_elevations() const = 0;
    
    // Validation
    virtual bool is_valid() const = 0;
    virtual std::string get_metadata(const std::string& key) const = 0;
};

// Standard HRTF database implementation
class StandardHRTFDatabase : public HRTFDatabase {
public:
    StandardHRTFDatabase();
    ~StandardHRTFDatabase() override;
    
    bool load_from_file(const std::string& filepath) override;
    bool load_from_sofa(const std::string& filepath) override;
    bool load_default_database() override;
    
    bool get_hrtf_data(float azimuth, float elevation,
                      AudioBuffer& left_hrir, AudioBuffer& right_hrir,
                      float& left_delay, float& right_delay) const override;
    
    bool get_interpolated_hrtf(float azimuth, float elevation,
                             HRTFInterpolation method,
                             AudioBuffer& left_hrir, AudioBuffer& right_hrir,
                             float& left_delay, float& right_delay) const override;
    
    uint32_t get_sample_rate() const override;
    uint32_t get_hrir_length() const override;
    uint32_t get_azimuth_count() const override;
    uint32_t get_elevation_count() const override;
    std::vector<float> get_available_azimuths() const override;
    std::vector<float> get_available_elevations() const override;
    
    bool is_valid() const override;
    std::string get_metadata(const std::string& key) const override;

private:
    void normalize_angles(float& azimuth, float& elevation) const;
    size_t find_hrtf_index(float azimuth, float elevation) const;
    void interpolate_linear(float azimuth, float elevation,
                           AudioBuffer& left_hrir, AudioBuffer& right_hrir,
                           float& left_delay, float& right_delay) const;
    void interpolate_cubic(float azimuth, float elevation,
                          AudioBuffer& left_hrir, AudioBuffer& right_hrir,
                          float& left_delay, float& right_delay) const;
    
    HRTFData m_hrtf_data;
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, std::string> m_metadata;
    bool m_loaded = false;
};

// Real-time HRTF convolution processor
class HRTFConvolver {
public:
    HRTFConvolver(uint32_t buffer_size, uint32_t sample_rate);
    ~HRTFConvolver();
    
    // Configuration
    void set_buffer_size(uint32_t buffer_size);
    void set_sample_rate(uint32_t sample_rate);
    void set_hrir(const AudioBuffer& left_hrir, const AudioBuffer& right_hrir);
    void set_delays(float left_delay, float right_delay);
    
    // Real-time processing
    void process(const AudioBuffer& input, StereoBuffer& output);
    void process_interleaved(const AudioBuffer& input, AudioBuffer& output);
    
    // Convolution methods
    enum class ConvolutionMethod {
        TIME_DOMAIN,
        FREQUENCY_DOMAIN,
        OVERLAP_ADD,
        OVERLAP_SAVE,
        PARTITIONED_CONVOLUTION
    };
    void set_convolution_method(ConvolutionMethod method);
    
    // Performance optimization
    void enable_simd(bool enable);
    void set_thread_count(uint32_t threads);
    
    // State management
    void reset();
    void flush_buffers();

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

// HRTF processor with caching and optimization
class HRTFProcessor {
public:
    HRTFProcessor(uint32_t buffer_size, uint32_t sample_rate);
    ~HRTFProcessor();
    
    // Database management
    bool load_hrtf_database(const std::string& filepath);
    bool load_default_database();
    void set_hrtf_database(std::unique_ptr<HRTFDatabase> database);
    
    // Processing configuration
    void set_interpolation_method(HRTFInterpolation method);
    void set_convolution_method(ConvolutionMethod method);
    void enable_caching(bool enable, size_t max_cache_size = 1000);
    void enable_distance_modeling(bool enable);
    void enable_head_shadow(bool enable);
    
    // Real-time processing
    void process_source(const AudioSource& source, const AudioListener& listener,
                       const AudioBuffer& input, StereoBuffer& output);
    
    void process_multiple_sources(
        const std::vector<AudioSource>& sources,
        const AudioListener& listener,
        const std::vector<AudioBuffer>& inputs,
        StereoBuffer& output
    );
    
    // Batch processing for multiple listeners (split-screen)
    void process_multi_listener(
        const std::vector<AudioSource>& sources,
        const std::vector<AudioListener>& listeners,
        const std::vector<AudioBuffer>& inputs,
        std::vector<StereoBuffer>& outputs
    );
    
    // Performance monitoring
    void enable_profiling(bool enable);
    float get_cpu_usage() const;
    size_t get_cache_hit_ratio() const;
    AudioMetrics get_processing_metrics() const;
    
    // Advanced features
    void set_crossfade_time(float time_ms);
    void enable_distance_delay(bool enable);
    void set_air_absorption_coefficients(const std::vector<float>& coefficients);
    
private:
    struct CacheEntry {
        AudioBuffer left_hrir;
        AudioBuffer right_hrir;
        float left_delay;
        float right_delay;
        uint64_t last_used;
    };
    
    Vector3f calculate_relative_position(const AudioSource& source, 
                                        const AudioListener& listener) const;
    void calculate_spherical_coordinates(const Vector3f& position,
                                       float& azimuth, float& elevation,
                                       float& distance) const;
    float calculate_distance_attenuation(float distance, 
                                       const AudioSource& source) const;
    void apply_air_absorption(AudioBuffer& buffer, float distance) const;
    void apply_head_shadow(AudioBuffer& left, AudioBuffer& right,
                          float azimuth, float frequency) const;
    
    std::string create_cache_key(float azimuth, float elevation) const;
    bool get_cached_hrtf(const std::string& key, CacheEntry& entry);
    void cache_hrtf(const std::string& key, const CacheEntry& entry);
    void cleanup_cache();
    
    std::unique_ptr<HRTFDatabase> m_database;
    std::unique_ptr<HRTFConvolver> m_left_convolver;
    std::unique_ptr<HRTFConvolver> m_right_convolver;
    
    uint32_t m_buffer_size;
    uint32_t m_sample_rate;
    HRTFInterpolation m_interpolation = HRTFInterpolation::LINEAR;
    ConvolutionMethod m_convolution = ConvolutionMethod::OVERLAP_ADD;
    
    // Caching
    bool m_caching_enabled = true;
    size_t m_max_cache_size = 1000;
    std::unordered_map<std::string, CacheEntry> m_hrtf_cache;
    mutable std::mutex m_cache_mutex;
    
    // Audio processing options
    bool m_distance_modeling = true;
    bool m_head_shadow = true;
    bool m_distance_delay = true;
    float m_crossfade_time = 10.0f;  // milliseconds
    std::vector<float> m_air_absorption_coeffs;
    
    // Performance tracking
    bool m_profiling_enabled = false;
    mutable AudioMetrics m_metrics;
    std::atomic<uint64_t> m_cache_hits{0};
    std::atomic<uint64_t> m_cache_misses{0};
};

// HRTF utilities
namespace hrtf_utils {
    // Coordinate conversions
    void cartesian_to_spherical(const Vector3f& cartesian,
                               float& azimuth, float& elevation, float& radius);
    Vector3f spherical_to_cartesian(float azimuth, float elevation, float radius);
    
    // Angle utilities
    float normalize_azimuth(float azimuth);
    float normalize_elevation(float elevation);
    float angle_difference(float angle1, float angle2);
    
    // HRIR processing
    void normalize_hrir(AudioBuffer& hrir);
    void apply_minimum_phase(AudioBuffer& hrir);
    void extract_itd(const AudioBuffer& left_hrir, const AudioBuffer& right_hrir,
                    float& left_delay, float& right_delay, uint32_t sample_rate);
    
    // Quality assessment
    float calculate_hrir_quality(const AudioBuffer& hrir);
    bool validate_hrtf_database(const HRTFData& data);
    
    // File I/O utilities
    bool load_wav_hrir(const std::string& filepath, AudioBuffer& left, AudioBuffer& right);
    bool save_wav_hrir(const std::string& filepath, 
                      const AudioBuffer& left, const AudioBuffer& right,
                      uint32_t sample_rate);
}

} // namespace ecscope::audio