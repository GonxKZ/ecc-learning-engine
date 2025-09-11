#pragma once

#include "audio_types.hpp"
#include <memory>
#include <vector>
#include <array>
#include <complex>

namespace ecscope::audio {

// Ambisonics coordinate system
enum class AmbisonicsCoordinate {
    ACN,    // Ambisonic Channel Number
    FuMa    // Traditional B-format
};

// Ambisonics normalization
enum class AmbisonicsNormalization {
    SN3D,   // Schmidt Semi-Normalized
    N3D,    // Fully Normalized
    FuMa    // Traditional B-format normalization
};

// Spherical harmonic coefficients
struct SphericalHarmonic {
    int degree;     // l (order)
    int order;      // m (sub-order)
    float coefficient;
    
    SphericalHarmonic(int l, int m, float coeff = 0.0f) 
        : degree(l), order(m), coefficient(coeff) {}
};

// Ambisonics encoder - converts point sources to ambisonic format
class AmbisonicsEncoder {
public:
    AmbisonicsEncoder(uint32_t ambisonic_order = 1, 
                     AmbisonicsCoordinate coord = AmbisonicsCoordinate::ACN,
                     AmbisonicsNormalization norm = AmbisonicsNormalization::SN3D);
    ~AmbisonicsEncoder();
    
    // Configuration
    void set_ambisonic_order(uint32_t order);
    void set_coordinate_system(AmbisonicsCoordinate coord);
    void set_normalization(AmbisonicsNormalization norm);
    uint32_t get_channel_count() const;
    
    // Encoding operations
    void encode_point_source(const AudioBuffer& mono_input,
                           float azimuth, float elevation, float distance,
                           std::vector<AudioBuffer>& ambisonic_output);
    
    void encode_multiple_sources(const std::vector<AudioBuffer>& mono_inputs,
                               const std::vector<Vector3f>& positions,
                               std::vector<AudioBuffer>& ambisonic_output);
    
    // Real-time encoding
    void encode_source_realtime(const AudioBuffer& input, float azimuth, float elevation,
                              std::vector<float>& coefficients);
    
    // Gain calculation for each harmonic
    std::vector<float> calculate_encoding_gains(float azimuth, float elevation) const;
    
    // Utility functions
    static uint32_t get_channel_count_for_order(uint32_t order);
    static std::string get_channel_name(uint32_t channel, uint32_t order);

private:
    void calculate_spherical_harmonics(float azimuth, float elevation,
                                     std::vector<float>& harmonics) const;
    void apply_distance_compensation(std::vector<float>& gains, float distance) const;
    
    uint32_t m_order;
    AmbisonicsCoordinate m_coordinate;
    AmbisonicsNormalization m_normalization;
    uint32_t m_channel_count;
    
    // Pre-computed tables for performance
    std::vector<std::vector<float>> m_legendre_table;
    std::vector<float> m_normalization_factors;
};

// Ambisonics decoder - converts ambisonic format to speaker array or binaural
class AmbisonicsDecoder {
public:
    enum class DecoderType {
        BASIC,          // Basic ambisonic decoder
        MAX_RE,         // Maximum energy vector decoder
        DUAL_BAND,      // Dual-band decoder
        ALL_ROUND,      // All-round ambisonic decoder
        BINAURAL        // Binaural decoder using HRTFs
    };
    
    struct SpeakerConfiguration {
        Vector3f position;      // Cartesian position
        float azimuth;         // Spherical coordinates
        float elevation;
        float distance = 1.0f;
        float gain = 1.0f;
        std::string name;
    };
    
    AmbisonicsDecoder(uint32_t ambisonic_order = 1);
    ~AmbisonicsDecoder();
    
    // Configuration
    void set_ambisonic_order(uint32_t order);
    void set_decoder_type(DecoderType type);
    void set_speaker_configuration(const std::vector<SpeakerConfiguration>& speakers);
    void load_speaker_preset(const std::string& preset_name);
    
    // Standard speaker configurations
    void setup_stereo_speakers();
    void setup_5_1_speakers();
    void setup_7_1_speakers();
    void setup_binaural_output();
    void setup_custom_array(const std::vector<SpeakerConfiguration>& speakers);
    
    // Decoding operations
    void decode_to_speakers(const std::vector<AudioBuffer>& ambisonic_input,
                          std::vector<AudioBuffer>& speaker_output);
    
    void decode_to_binaural(const std::vector<AudioBuffer>& ambisonic_input,
                          StereoBuffer& binaural_output,
                          const AudioListener& listener);
    
    // Real-time decoding
    void decode_frame_realtime(const std::vector<float>& ambisonic_frame,
                             std::vector<float>& speaker_frame);
    
    // Decoder matrix access
    const std::vector<std::vector<float>>& get_decoder_matrix() const;
    void set_custom_decoder_matrix(const std::vector<std::vector<float>>& matrix);
    
    // Performance optimization
    void enable_near_field_compensation(bool enable);
    void set_crossover_frequency(float frequency);  // For dual-band decoder

private:
    void calculate_basic_decoder_matrix();
    void calculate_max_re_decoder_matrix();
    void calculate_dual_band_decoder_matrix();
    void calculate_binaural_decoder_matrix();
    
    uint32_t m_order;
    uint32_t m_channel_count;
    DecoderType m_decoder_type;
    std::vector<SpeakerConfiguration> m_speakers;
    std::vector<std::vector<float>> m_decoder_matrix;
    
    // Binaural decoding
    std::unique_ptr<class HRTFProcessor> m_hrtf_processor;
    bool m_binaural_mode = false;
    
    // Dual-band processing
    float m_crossover_frequency = 400.0f;
    bool m_near_field_compensation = false;
};

// Ambisonics rotator for head tracking and scene rotation
class AmbisonicsRotator {
public:
    AmbisonicsRotator(uint32_t ambisonic_order = 1);
    ~AmbisonicsRotator();
    
    // Rotation operations
    void set_rotation(const Quaternion& rotation);
    void set_rotation_euler(float yaw, float pitch, float roll);
    void rotate_ambisonic_field(const std::vector<AudioBuffer>& input,
                              std::vector<AudioBuffer>& output);
    
    // Real-time rotation
    void rotate_frame_realtime(const std::vector<float>& input_frame,
                             std::vector<float>& output_frame);
    
    // Rotation matrix access
    const std::vector<std::vector<float>>& get_rotation_matrix() const;
    void set_custom_rotation_matrix(const std::vector<std::vector<float>>& matrix);
    
    // Smooth rotation for head tracking
    void enable_smooth_rotation(bool enable);
    void set_smoothing_factor(float factor);
    void update_rotation_smoothly(const Quaternion& target_rotation, float delta_time);

private:
    void calculate_rotation_matrix(const Quaternion& rotation);
    void calculate_wigner_d_matrices(float alpha, float beta, float gamma);
    
    uint32_t m_order;
    uint32_t m_channel_count;
    Quaternion m_current_rotation{1, 0, 0, 0};
    Quaternion m_target_rotation{1, 0, 0, 0};
    std::vector<std::vector<float>> m_rotation_matrix;
    
    // Smooth rotation
    bool m_smooth_rotation = true;
    float m_smoothing_factor = 0.1f;
    
    // Wigner D-matrix tables
    std::vector<std::vector<std::vector<float>>> m_wigner_d_matrices;
};

// Ambisonics transformer for format conversion
class AmbisonicsTransformer {
public:
    AmbisonicsTransformer();
    ~AmbisonicsTransformer();
    
    // Format conversion
    void convert_coordinate_system(const std::vector<AudioBuffer>& input,
                                 std::vector<AudioBuffer>& output,
                                 AmbisonicsCoordinate from_coord,
                                 AmbisonicsCoordinate to_coord);
    
    void convert_normalization(const std::vector<AudioBuffer>& input,
                             std::vector<AudioBuffer>& output,
                             AmbisonicsNormalization from_norm,
                             AmbisonicsNormalization to_norm);
    
    // Order conversion
    void change_ambisonic_order(const std::vector<AudioBuffer>& input,
                              std::vector<AudioBuffer>& output,
                              uint32_t from_order, uint32_t to_order);
    
    // Mixed-order ambisonics support
    void create_mixed_order_stream(const std::vector<AudioBuffer>& input,
                                 std::vector<AudioBuffer>& output,
                                 const std::vector<uint32_t>& channel_orders);
    
    // Real-time conversion
    void convert_frame_format(const std::vector<float>& input_frame,
                            std::vector<float>& output_frame,
                            AmbisonicsCoordinate from_coord,
                            AmbisonicsCoordinate to_coord);

private:
    void calculate_conversion_matrix(AmbisonicsCoordinate from_coord,
                                   AmbisonicsCoordinate to_coord,
                                   uint32_t order);
    
    std::vector<std::vector<float>> m_conversion_matrix;
    std::vector<float> m_normalization_factors;
};

// Ambisonics processor for complete workflow
class AmbisonicsProcessor {
public:
    AmbisonicsProcessor(uint32_t ambisonic_order = 1, uint32_t sample_rate = 44100);
    ~AmbisonicsProcessor();
    
    // Configuration
    void initialize(uint32_t sample_rate, uint32_t buffer_size);
    void set_ambisonic_order(uint32_t order);
    void set_coordinate_system(AmbisonicsCoordinate coord);
    void set_normalization(AmbisonicsNormalization norm);
    
    // Complete processing chain
    void process_3d_sources_to_speakers(
        const std::vector<AudioBuffer>& source_inputs,
        const std::vector<Vector3f>& source_positions,
        const std::vector<AmbisonicsDecoder::SpeakerConfiguration>& speakers,
        const AudioListener& listener,
        std::vector<AudioBuffer>& speaker_outputs
    );
    
    void process_3d_sources_to_binaural(
        const std::vector<AudioBuffer>& source_inputs,
        const std::vector<Vector3f>& source_positions,
        const AudioListener& listener,
        StereoBuffer& binaural_output
    );
    
    // Advanced features
    void enable_head_tracking(bool enable);
    void update_head_rotation(const Quaternion& head_rotation);
    void enable_room_compensation(bool enable);
    void set_room_parameters(float room_size, float absorption);
    
    // Performance monitoring
    AudioMetrics get_ambisonics_metrics() const;
    float get_encoding_cpu_usage() const;
    float get_decoding_cpu_usage() const;
    
    // Component access
    AmbisonicsEncoder& get_encoder();
    AmbisonicsDecoder& get_decoder();
    AmbisonicsRotator& get_rotator();
    AmbisonicsTransformer& get_transformer();

private:
    std::unique_ptr<AmbisonicsEncoder> m_encoder;
    std::unique_ptr<AmbisonicsDecoder> m_decoder;
    std::unique_ptr<AmbisonicsRotator> m_rotator;
    std::unique_ptr<AmbisonicsTransformer> m_transformer;
    
    uint32_t m_order;
    uint32_t m_sample_rate;
    uint32_t m_buffer_size;
    bool m_head_tracking_enabled = false;
    
    // Temporary buffers for processing
    std::vector<AudioBuffer> m_temp_ambisonic_buffers;
    
    // Performance tracking
    mutable AudioMetrics m_metrics;
};

// Ambisonics utilities
namespace ambisonics_utils {
    // Mathematical functions
    float factorial(int n);
    float double_factorial(int n);
    float associated_legendre(int l, int m, float x);
    std::complex<float> spherical_harmonic(int l, int m, float theta, float phi);
    
    // Coordinate conversions
    void cartesian_to_spherical(const Vector3f& cartesian, 
                               float& azimuth, float& elevation, float& radius);
    Vector3f spherical_to_cartesian(float azimuth, float elevation, float radius);
    
    // Ambisonic channel utilities
    int acn_index(int degree, int order);
    void acn_to_degree_order(int acn, int& degree, int& order);
    std::string get_channel_description(int degree, int order);
    
    // Format conversion helpers
    float sn3d_to_n3d_factor(int degree);
    float n3d_to_sn3d_factor(int degree);
    float fuma_to_acn_gain(int channel);
    
    // Validation functions
    bool validate_ambisonic_order(uint32_t order);
    bool validate_channel_count(uint32_t channel_count, uint32_t order);
    bool validate_speaker_configuration(
        const std::vector<AmbisonicsDecoder::SpeakerConfiguration>& speakers);
    
    // Quality assessment
    float calculate_decoder_quality(const std::vector<std::vector<float>>& decoder_matrix);
    float calculate_localization_accuracy(
        const std::vector<AmbisonicsDecoder::SpeakerConfiguration>& speakers);
}

} // namespace ecscope::audio