#pragma once

/**
 * @file spatial_audio_engine.hpp
 * @brief Advanced 3D Spatial Audio Engine for ECScope Educational ECS Framework
 * 
 * This comprehensive 3D spatial audio system provides world-class audio positioning,
 * HRTF-based 3D processing, environmental audio effects, and educational features
 * for teaching audio engineering and spatial audio concepts.
 * 
 * Key Features:
 * - HRTF-based 3D audio with head tracking support
 * - Distance-based attenuation and Doppler effects
 * - Environmental audio with reverb, occlusion, and reflection
 * - Multi-channel surround sound support (stereo to 7.1+)
 * - Real-time audio processing with SIMD optimization
 * - Physics integration for realistic audio interactions
 * - Educational DSP demonstrations and visualizations
 * 
 * Educational Value:
 * - Demonstrates psychoacoustics and spatial hearing
 * - Shows HRTF processing and binaural audio concepts
 * - Illustrates environmental audio modeling
 * - Provides real-time audio analysis and visualization
 * - Teaches performance optimization for audio systems
 * - Interactive audio algorithm demonstrations
 * 
 * Performance Characteristics:
 * - Low-latency audio pipeline (sub-10ms)
 * - SIMD-optimized processing for real-time performance
 * - Memory pool management for audio buffers
 * - Efficient spatial partitioning for audio culling
 * - Adaptive quality scaling based on performance
 * 
 * Integration:
 * - Full ECS component integration
 * - Physics engine integration for occlusion/reflection
 * - Scene editor integration for audio source placement
 * - Memory tracking and performance analysis
 * - Educational system integration for learning
 * 
 * @author ECScope Educational ECS Framework - Advanced Audio Systems
 * @date 2025
 */

#include "core/types.hpp"
#include "core/math.hpp"
#include "ecs/component.hpp"
#include "memory/memory_tracker.hpp"
#include "audio_importer.hpp"
#include <vector>
#include <array>
#include <memory>
#include <atomic>
#include <complex>
#include <unordered_map>
#include <functional>
#include <immintrin.h>

namespace ecscope::audio {

//=============================================================================
// Forward Declarations and Type Aliases
//=============================================================================

class SpatialAudioEngine;
class HRTFProcessor;
class AudioEnvironmentProcessor;
class AudioOcclusionProcessor;

// Audio sample formats optimized for processing
using AudioSample = f32;                    // 32-bit float samples for processing
using AudioFrame = std::array<AudioSample, 2>; // Stereo frame
using AudioBuffer = std::vector<AudioSample>;
using StereoBuffer = std::vector<AudioFrame>;

// SIMD-optimized vector types
#if defined(__AVX2__)
using AudioVec8 = __m256;
constexpr usize SIMD_WIDTH = 8;
#elif defined(__SSE2__)
using AudioVec4 = __m128;
constexpr usize SIMD_WIDTH = 4;
#else
constexpr usize SIMD_WIDTH = 1;
#endif

//=============================================================================
// Audio Configuration and Constants
//=============================================================================

/**
 * @brief Global audio configuration constants
 */
namespace config {
    // Sample rates (standard audio rates)
    constexpr u32 SAMPLE_RATE_44K = 44100;
    constexpr u32 SAMPLE_RATE_48K = 48000;
    constexpr u32 SAMPLE_RATE_96K = 96000;
    constexpr u32 SAMPLE_RATE_192K = 192000;
    
    // Buffer sizes (power of 2 for efficiency)
    constexpr usize AUDIO_BUFFER_SIZE = 512;    // Samples per buffer
    constexpr usize MAX_CHANNELS = 8;           // Support up to 7.1 surround
    constexpr usize MAX_AUDIO_SOURCES = 256;    // Maximum simultaneous audio sources
    constexpr usize MAX_LISTENERS = 4;          // Maximum listeners (local multiplayer)
    
    // Spatial audio constants
    constexpr f32 SPEED_OF_SOUND = 343.0f;      // m/s at 20°C
    constexpr f32 MIN_DISTANCE = 0.1f;          // Minimum distance for calculations
    constexpr f32 MAX_DISTANCE = 1000.0f;       // Maximum audible distance
    constexpr f32 DOPPLER_SCALE = 1.0f;         // Doppler effect scaling
    
    // Performance settings
    constexpr f32 TARGET_LATENCY_MS = 10.0f;    // Target latency
    constexpr u32 CONVOLUTION_SIZE = 256;       // HRTF convolution size
    constexpr u32 ENVIRONMENT_TAPS = 32;        // Reverb/environment taps
    
    // Educational settings
    constexpr bool ENABLE_ANALYSIS = true;      // Enable educational analysis
    constexpr bool ENABLE_VISUALIZATION = true; // Enable real-time visualization
    constexpr u32 ANALYSIS_WINDOW_SIZE = 1024;  // FFT window size for analysis
}

//=============================================================================
// 3D Math Utilities for Spatial Audio
//=============================================================================

namespace spatial_math {
    /**
     * @brief 3D vector for spatial calculations
     */
    struct Vec3 {
        f32 x, y, z;
        
        constexpr Vec3() noexcept : x(0.0f), y(0.0f), z(0.0f) {}
        constexpr Vec3(f32 x_, f32 y_, f32 z_) noexcept : x(x_), y(y_), z(z_) {}
        
        // Basic vector operations
        Vec3 operator+(const Vec3& other) const noexcept {
            return Vec3{x + other.x, y + other.y, z + other.z};
        }
        
        Vec3 operator-(const Vec3& other) const noexcept {
            return Vec3{x - other.x, y - other.y, z - other.z};
        }
        
        Vec3 operator*(f32 scalar) const noexcept {
            return Vec3{x * scalar, y * scalar, z * scalar};
        }
        
        f32 length() const noexcept {
            return std::sqrt(x * x + y * y + z * z);
        }
        
        f32 length_squared() const noexcept {
            return x * x + y * y + z * z;
        }
        
        Vec3 normalized() const noexcept {
            f32 len = length();
            if (len > 0.0f) {
                f32 inv_len = 1.0f / len;
                return Vec3{x * inv_len, y * inv_len, z * inv_len};
            }
            return Vec3{0.0f, 0.0f, 1.0f}; // Default forward
        }
        
        f32 dot(const Vec3& other) const noexcept {
            return x * other.x + y * other.y + z * other.z;
        }
        
        Vec3 cross(const Vec3& other) const noexcept {
            return Vec3{
                y * other.z - z * other.y,
                z * other.x - x * other.z,
                x * other.y - y * other.x
            };
        }
        
        static constexpr Vec3 zero() noexcept { return Vec3{0.0f, 0.0f, 0.0f}; }
        static constexpr Vec3 forward() noexcept { return Vec3{0.0f, 0.0f, 1.0f}; }
        static constexpr Vec3 up() noexcept { return Vec3{0.0f, 1.0f, 0.0f}; }
        static constexpr Vec3 right() noexcept { return Vec3{1.0f, 0.0f, 0.0f}; }
    };
    
    /**
     * @brief 3D orientation for spatial audio
     */
    struct Orientation {
        Vec3 forward{0.0f, 0.0f, 1.0f};  // Forward direction
        Vec3 up{0.0f, 1.0f, 0.0f};       // Up direction
        
        // Calculate right vector
        Vec3 right() const noexcept {
            return forward.cross(up).normalized();
        }
        
        // Create from Euler angles (in radians)
        static Orientation from_euler(f32 yaw, f32 pitch, f32 roll) noexcept;
        
        // Create from quaternion
        static Orientation from_quaternion(f32 x, f32 y, f32 z, f32 w) noexcept;
    };
    
    /**
     * @brief Transform for 3D positioning
     */
    struct Transform3D {
        Vec3 position{0.0f, 0.0f, 0.0f};
        Orientation orientation;
        Vec3 velocity{0.0f, 0.0f, 0.0f};  // For Doppler calculations
        
        // Convert from 2D transform (assuming Y=up, Z=forward)
        static Transform3D from_2d_transform(const Vec2& pos_2d, f32 rotation_2d, f32 height = 0.0f) noexcept;
        
        // Calculate relative positioning for spatial audio
        struct RelativePosition {
            Vec3 relative_pos;      // Position relative to listener
            f32 distance;           // Distance to listener
            f32 azimuth;           // Horizontal angle (-π to π)
            f32 elevation;         // Vertical angle (-π/2 to π/2)
            f32 doppler_factor;    // Doppler shift factor
        };
        
        RelativePosition get_relative_position(const Transform3D& listener) const noexcept;
    };
    
    /**
     * @brief Distance attenuation models
     */
    namespace attenuation {
        // Linear attenuation: volume decreases linearly with distance
        inline f32 linear(f32 distance, f32 max_distance) noexcept {
            return std::max(0.0f, 1.0f - distance / max_distance);
        }
        
        // Inverse distance attenuation (physically accurate)
        inline f32 inverse(f32 distance, f32 reference_distance = 1.0f) noexcept {
            return reference_distance / std::max(distance, config::MIN_DISTANCE);
        }
        
        // Exponential attenuation (customizable falloff)
        inline f32 exponential(f32 distance, f32 rolloff_factor = 1.0f) noexcept {
            return 1.0f / (1.0f + rolloff_factor * distance);
        }
        
        // Logarithmic attenuation (similar to perceived loudness)
        inline f32 logarithmic(f32 distance, f32 reference_distance = 1.0f) noexcept {
            return reference_distance / (reference_distance + rolloff_factor * distance);
        }
        
        // Custom curve attenuation (user-defined curve)
        f32 custom_curve(f32 distance, const std::vector<f32>& curve_points) noexcept;
        
    private:
        static constexpr f32 rolloff_factor = 1.0f;
    }
    
    /**
     * @brief Doppler effect calculations
     */
    namespace doppler {
        /**
         * @brief Calculate Doppler shift factor
         * 
         * Educational Context:
         * Doppler effect occurs when source or listener are moving.
         * Formula: f' = f * (v + v_r) / (v + v_s)
         * Where: v = speed of sound, v_r = listener velocity, v_s = source velocity
         * 
         * @param source_velocity Source velocity vector
         * @param listener_velocity Listener velocity vector
         * @param relative_direction Normalized direction from source to listener
         * @return Doppler frequency ratio (1.0 = no shift)
         */
        f32 calculate_shift(const Vec3& source_velocity, const Vec3& listener_velocity, 
                           const Vec3& relative_direction) noexcept;
        
        /**
         * @brief Apply Doppler effect to audio buffer
         * 
         * Uses pitch shifting to simulate Doppler effect.
         */
        void apply_doppler_effect(AudioBuffer& buffer, f32 doppler_factor, 
                                 u32 sample_rate) noexcept;
    }
}

//=============================================================================
// HRTF (Head-Related Transfer Function) Processing
//=============================================================================

/**
 * @brief HRTF Database and Processing System
 * 
 * Implements Head-Related Transfer Functions for realistic 3D audio.
 * HRTF filters simulate how sounds are modified by the listener's head,
 * ears, and torso, creating convincing 3D audio over headphones.
 * 
 * Educational Context:
 * - Demonstrates psychoacoustics and binaural hearing
 * - Shows how humans localize sound in 3D space
 * - Illustrates convolution-based audio processing
 * - Teaches about frequency-dependent directional filtering
 */
class HRTFProcessor {
public:
    /**
     * @brief HRTF impulse response for a specific direction
     */
    struct HRTFImpulseResponse {
        std::array<f32, config::CONVOLUTION_SIZE> left_ear;   // Left ear impulse response
        std::array<f32, config::CONVOLUTION_SIZE> right_ear;  // Right ear impulse response
        f32 azimuth;        // Horizontal angle
        f32 elevation;      // Vertical angle
        f32 distance;       // Reference distance for this response
    };
    
    /**
     * @brief HRTF database containing responses for different directions
     */
    struct HRTFDatabase {
        std::vector<HRTFImpulseResponse> responses;
        u32 azimuth_resolution;     // Number of azimuth steps (e.g., 72 = 5° steps)
        u32 elevation_resolution;   // Number of elevation steps (e.g., 37 = 5° steps)
        u32 sample_rate;           // Sample rate for impulse responses
        
        // Metadata
        std::string source;         // "MIT KEMAR", "CIPIC", "Custom", etc.
        std::string description;
        bool is_loaded{false};
        
        // Educational information
        struct EducationalInfo {
            std::string explanation;
            std::vector<std::string> key_concepts;
            f32 educational_value{0.8f};
        } educational;
    };
    
private:
    std::unique_ptr<HRTFDatabase> hrtf_database_;
    
    // Convolution state for real-time processing
    struct ConvolutionState {
        std::array<f32, config::CONVOLUTION_SIZE> left_history;
        std::array<f32, config::CONVOLUTION_SIZE> right_history;
        usize history_index{0};
        f32 current_azimuth{0.0f};
        f32 current_elevation{0.0f};
        f32 interpolation_alpha{0.0f};  // For smooth transitions
    };
    
    std::unordered_map<u32, ConvolutionState> source_states_; // Per audio source
    
    // Performance optimization
    mutable std::atomic<u32> processed_samples_{0};
    mutable std::atomic<f32> processing_time_{0.0f};
    
public:
    HRTFProcessor();
    ~HRTFProcessor() = default;
    
    // HRTF database management
    bool load_hrtf_database(const std::string& database_path);
    bool load_default_hrtf_database();  // Load built-in educational HRTF
    void generate_synthetic_hrtf();     // Generate simplified HRTF for education
    
    // Main processing functions
    void process_spatial_audio(u32 source_id, const AudioBuffer& mono_input,
                              StereoBuffer& stereo_output, f32 azimuth, f32 elevation,
                              f32 distance, u32 sample_rate);
    
    void process_spatial_audio_simd(u32 source_id, const AudioBuffer& mono_input,
                                   StereoBuffer& stereo_output, f32 azimuth, f32 elevation,
                                   f32 distance, u32 sample_rate);
    
    // HRTF interpolation (for smooth movement)
    HRTFImpulseResponse interpolate_hrtf(f32 azimuth, f32 elevation) const;
    void set_interpolation_smoothing(f32 smoothing_factor); // 0.0 = instant, 1.0 = very smooth
    
    // Educational analysis functions
    struct HRTFAnalysis {
        f32 left_right_delay_us;        // Interaural time difference (microseconds)
        f32 left_right_level_db;        // Interaural level difference (dB)
        f32 spectral_centroid_shift;    // How much the frequency content shifts
        f32 localization_accuracy;     // Predicted localization accuracy (0-1)
        std::vector<f32> frequency_response_left;   // Frequency response for left ear
        std::vector<f32> frequency_response_right;  // Frequency response for right ear
        
        // Educational insights
        std::string perceptual_description; // "Sound appears to come from front-left"
        std::string technical_explanation;  // Technical explanation of the processing
    };
    
    HRTFAnalysis analyze_spatial_position(f32 azimuth, f32 elevation) const;
    
    // Performance and debugging
    struct HRTFPerformanceInfo {
        u32 sources_processed{0};
        f32 average_processing_time_ms{0.0f};
        f32 cpu_usage_percent{0.0f};
        usize memory_usage_bytes{0};
        u32 interpolations_per_second{0};
    };
    
    HRTFPerformanceInfo get_performance_info() const;
    void reset_performance_counters();
    
    // Educational utilities
    std::string get_hrtf_explanation() const;
    std::vector<std::string> get_learning_objectives() const;
    void generate_hrtf_visualization_data(std::vector<f32>& azimuth_response,
                                        std::vector<f32>& elevation_response) const;
    
private:
    // HRTF database utilities
    void build_spatial_index();
    std::pair<usize, usize> find_nearest_responses(f32 azimuth, f32 elevation) const;
    
    // Convolution implementation
    void convolve_hrtf(const f32* input, usize input_size,
                      const f32* left_impulse, const f32* right_impulse,
                      f32* left_output, f32* right_output,
                      ConvolutionState& state) const;
    
    void convolve_hrtf_simd(const f32* input, usize input_size,
                           const f32* left_impulse, const f32* right_impulse,
                           f32* left_output, f32* right_output,
                           ConvolutionState& state) const;
    
    // Educational HRTF generation
    void generate_simple_itd_model(f32 azimuth, f32 elevation,
                                  f32& left_delay, f32& right_delay) const;
    void generate_simple_ild_model(f32 azimuth, f32 elevation,
                                  f32& left_gain, f32& right_gain) const;
};

//=============================================================================
// Environmental Audio Processing
//=============================================================================

/**
 * @brief Environmental Audio Effects Processor
 * 
 * Simulates environmental audio effects including reverb, occlusion,
 * obstruction, and atmospheric effects. Provides realistic audio
 * environments and educational insight into acoustic modeling.
 * 
 * Educational Context:
 * - Room acoustics and reverberation
 * - Sound propagation and reflection
 * - Absorption and diffusion
 * - Acoustic material properties
 * - Early reflections and late reverb
 */
class AudioEnvironmentProcessor {
public:
    /**
     * @brief Environmental audio parameters
     */
    struct EnvironmentParameters {
        // Room characteristics
        Vec3 room_dimensions{10.0f, 3.0f, 8.0f};  // Width, height, depth (meters)
        f32 absorption_coefficient{0.3f};          // Wall absorption (0=reflective, 1=absorbing)
        f32 diffusion_coefficient{0.7f};           // Surface diffusion (0=specular, 1=diffuse)
        f32 air_absorption{0.01f};                 // High-frequency air absorption
        
        // Reverb characteristics
        f32 reverb_time{2.0f};                     // RT60 reverb time (seconds)
        f32 early_reflection_delay{0.02f};         // Early reflection delay (seconds)
        f32 late_reverb_delay{0.05f};              // Late reverb delay (seconds)
        f32 reverb_density{0.8f};                  // Reverb echo density (0-1)
        f32 reverb_diffusion{0.7f};                // Reverb diffusion (0-1)
        
        // Frequency response
        f32 low_frequency_gain{0.0f};              // Low frequency adjustment (dB)
        f32 mid_frequency_gain{0.0f};              // Mid frequency adjustment (dB)
        f32 high_frequency_gain{-3.0f};           // High frequency adjustment (dB)
        f32 low_frequency_cutoff{200.0f};          // Low frequency cutoff (Hz)
        f32 high_frequency_cutoff{4000.0f};       // High frequency cutoff (Hz)
        
        // Environmental effects
        f32 doppler_scale{1.0f};                   // Doppler effect scaling
        f32 speed_of_sound{343.0f};                // Local speed of sound (m/s)
        Vec3 wind_velocity{0.0f, 0.0f, 0.0f};      // Wind velocity for wind effects
        
        // Educational metadata
        std::string environment_type;               // "Concert Hall", "Forest", "Cave", etc.
        std::string acoustic_description;          // Educational description
        f32 educational_interest{0.5f};            // How educationally interesting (0-1)
    };
    
    /**
     * @brief Reverb processor using feedback delay networks
     */
    class ReverbProcessor {
    private:
        // Feedback delay network (FDN) for high-quality reverb
        struct FeedbackDelayNetwork {
            static constexpr usize NUM_DELAYS = 8;
            std::array<std::vector<f32>, NUM_DELAYS> delay_lines;
            std::array<usize, NUM_DELAYS> delay_lengths;
            std::array<usize, NUM_DELAYS> delay_indices;
            std::array<f32, NUM_DELAYS> feedback_gains;
            std::array<f32, NUM_DELAYS> output_gains;
            
            // Modulation for natural sound
            std::array<f32, NUM_DELAYS> modulation_phases;
            std::array<f32, NUM_DELAYS> modulation_rates;
            std::array<f32, NUM_DELAYS> modulation_depths;
        } fdn_;
        
        // Early reflections using tap delays
        struct EarlyReflections {
            static constexpr usize NUM_TAPS = 16;
            std::vector<f32> delay_line;
            std::array<usize, NUM_TAPS> tap_delays;
            std::array<f32, NUM_TAPS> tap_gains;
            usize write_index{0};
        } early_reflections_;
        
        // Filters for frequency shaping
        struct ReverbFilters {
            // Low-pass filter for high-frequency damping
            struct LowPassFilter {
                f32 coefficient{0.7f};
                f32 state{0.0f};
                
                f32 process(f32 input) noexcept {
                    state = state * coefficient + input * (1.0f - coefficient);
                    return state;
                }
            };
            
            // High-pass filter for rumble removal
            struct HighPassFilter {
                f32 coefficient{0.99f};
                f32 state{0.0f};
                f32 prev_input{0.0f};
                
                f32 process(f32 input) noexcept {
                    state = coefficient * (state + input - prev_input);
                    prev_input = input;
                    return state;
                }
            };
            
            std::array<LowPassFilter, FeedbackDelayNetwork::NUM_DELAYS> low_pass_filters;
            std::array<HighPassFilter, FeedbackDelayNetwork::NUM_DELAYS> high_pass_filters;
        } filters_;
        
    public:
        ReverbProcessor(u32 sample_rate);
        
        void set_parameters(const EnvironmentParameters& params, u32 sample_rate);
        void process_reverb(const AudioBuffer& input, StereoBuffer& output);
        void process_reverb_simd(const AudioBuffer& input, StereoBuffer& output);
        
        // Educational analysis
        struct ReverbAnalysis {
            f32 rt60_measured;              // Measured RT60 time
            f32 early_decay_time;           // EDT (early decay time)
            f32 clarity_c50;                // C50 clarity measure
            f32 definition_d50;             // D50 definition
            std::vector<f32> impulse_response; // Room impulse response
            
            std::string acoustic_quality;   // "Intimate", "Spacious", "Cathedral", etc.
            std::string educational_notes;  // Educational insights
        };
        
        ReverbAnalysis analyze_reverb_characteristics() const;
        
    private:
        void calculate_delay_lengths(const EnvironmentParameters& params, u32 sample_rate);
        void calculate_feedback_matrix();
        f32 estimate_rt60_from_params(const EnvironmentParameters& params) const;
    };
    
    /**
     * @brief Occlusion and obstruction processor
     */
    class OcclusionProcessor {
    private:
        // Low-pass filters for occlusion effect
        struct OcclusionFilter {
            f32 cutoff_frequency{1000.0f};
            f32 coefficient{0.0f};
            f32 state_left{0.0f};
            f32 state_right{0.0f};
            
            void set_cutoff(f32 frequency, u32 sample_rate) noexcept;
            AudioFrame process(const AudioFrame& input) noexcept;
        };
        
        std::unordered_map<u32, OcclusionFilter> source_filters_;
        
        // Physics integration for automatic occlusion detection
        struct PhysicsIntegration {
            bool use_physics_occlusion{true};
            f32 occlusion_raycast_resolution{1.0f};  // Meters between raycasts
            f32 obstruction_threshold{0.5f};         // Fraction of rays blocked for obstruction
            u32 raycast_samples{8};                  // Number of rays to cast
        } physics_integration_;
        
    public:
        // Manual occlusion control
        void set_occlusion(u32 source_id, f32 occlusion_amount); // 0.0 = clear, 1.0 = fully occluded
        void set_obstruction(u32 source_id, f32 obstruction_amount); // 0.0 = clear, 1.0 = fully obstructed
        
        // Automatic occlusion using physics
        void update_physics_occlusion(u32 source_id, const spatial_math::Vec3& source_pos,
                                    const spatial_math::Vec3& listener_pos);
        
        void process_occlusion(u32 source_id, StereoBuffer& audio_buffer);
        
        // Educational analysis
        struct OcclusionAnalysis {
            f32 occlusion_amount;           // Current occlusion (0-1)
            f32 obstruction_amount;         // Current obstruction (0-1)
            f32 effective_cutoff_frequency; // Effective filter cutoff
            f32 volume_reduction_db;        // Volume reduction in dB
            
            std::string occlusion_type;     // "Partial", "Full", "Obstruction"
            std::string perceptual_effect;  // How it sounds to the listener
        };
        
        OcclusionAnalysis get_occlusion_analysis(u32 source_id) const;
    };
    
private:
    std::unique_ptr<ReverbProcessor> reverb_processor_;
    std::unique_ptr<OcclusionProcessor> occlusion_processor_;
    
    EnvironmentParameters current_environment_;
    u32 sample_rate_;
    
    // Performance tracking
    mutable std::atomic<u32> processed_buffers_{0};
    mutable std::atomic<f32> processing_time_ms_{0.0f};
    
public:
    AudioEnvironmentProcessor(u32 sample_rate);
    ~AudioEnvironmentProcessor();
    
    // Environment management
    void set_environment(const EnvironmentParameters& params);
    const EnvironmentParameters& get_environment() const { return current_environment_; }
    
    // Built-in environment presets for education
    static EnvironmentParameters create_concert_hall();
    static EnvironmentParameters create_small_room();
    static EnvironmentParameters create_cathedral();
    static EnvironmentParameters create_forest();
    static EnvironmentParameters create_cave();
    static EnvironmentParameters create_underwater();
    
    // Main processing function
    void process_environment_audio(u32 source_id, const spatial_math::Vec3& source_pos,
                                 const spatial_math::Vec3& listener_pos,
                                 const AudioBuffer& input, StereoBuffer& output);
    
    // Occlusion control
    void set_source_occlusion(u32 source_id, f32 occlusion);
    void enable_physics_occlusion(bool enable) { occlusion_processor_->physics_integration_.use_physics_occlusion = enable; }
    
    // Educational features
    struct EnvironmentAnalysis {
        f32 reverb_time_measured;           // Measured RT60
        f32 room_volume_estimate;           // Estimated room volume
        f32 surface_area_estimate;          // Estimated surface area
        f32 acoustic_quality_score;         // Overall acoustic quality (0-1)
        
        std::string environment_classification; // Type of acoustic space
        std::string educational_insights;       // Key learning points
        std::vector<std::string> improvement_suggestions; // How to improve acoustics
    };
    
    EnvironmentAnalysis analyze_environment() const;
    std::string get_environment_tutorial() const;
    
    // Performance monitoring
    struct EnvironmentPerformanceInfo {
        f32 reverb_cpu_percent{0.0f};
        f32 occlusion_cpu_percent{0.0f};
        f32 total_cpu_percent{0.0f};
        usize memory_usage_bytes{0};
        u32 active_sources{0};
    };
    
    EnvironmentPerformanceInfo get_performance_info() const;
    
private:
    void calculate_room_acoustics(const EnvironmentParameters& params);
    f32 estimate_reverb_time(const EnvironmentParameters& params) const;
    void setup_early_reflections(const EnvironmentParameters& params);
};

//=============================================================================
// Performance Optimization and SIMD Processing
//=============================================================================

/**
 * @brief SIMD-optimized audio processing utilities
 * 
 * Provides high-performance audio processing functions using
 * SIMD instructions for real-time performance. Educational
 * examples of optimization techniques for audio systems.
 */
namespace simd_audio {
    // Vector operations for audio processing
    #if defined(__AVX2__)
    
    // Process 8 samples at once with AVX2
    inline void multiply_add_avx2(const f32* input, const f32* coeffs,
                                 f32* output, usize count) noexcept {
        for (usize i = 0; i < count; i += 8) {
            __m256 in = _mm256_loadu_ps(&input[i]);
            __m256 coeff = _mm256_loadu_ps(&coeffs[i]);
            __m256 out = _mm256_loadu_ps(&output[i]);
            out = _mm256_fmadd_ps(in, coeff, out);
            _mm256_storeu_ps(&output[i], out);
        }
    }
    
    // Convolution with AVX2
    void convolve_avx2(const f32* signal, const f32* kernel,
                       f32* output, usize signal_len, usize kernel_len) noexcept;
    
    #elif defined(__SSE2__)
    
    // Process 4 samples at once with SSE2
    inline void multiply_add_sse2(const f32* input, const f32* coeffs,
                                 f32* output, usize count) noexcept {
        for (usize i = 0; i < count; i += 4) {
            __m128 in = _mm_loadu_ps(&input[i]);
            __m128 coeff = _mm_loadu_ps(&coeffs[i]);
            __m128 out = _mm_loadu_ps(&output[i]);
            out = _mm_add_ps(_mm_mul_ps(in, coeff), out);
            _mm_storeu_ps(&output[i], out);
        }
    }
    
    // Convolution with SSE2
    void convolve_sse2(const f32* signal, const f32* kernel,
                       f32* output, usize signal_len, usize kernel_len) noexcept;
    
    #endif
    
    // Generic fallback implementations
    void multiply_add_scalar(const f32* input, const f32* coeffs,
                            f32* output, usize count) noexcept;
    void convolve_scalar(const f32* signal, const f32* kernel,
                        f32* output, usize signal_len, usize kernel_len) noexcept;
    
    // Adaptive SIMD dispatch
    class SIMDProcessor {
    private:
        enum class SIMDLevel {
            Scalar,
            SSE2,
            AVX2
        } simd_level_;
        
    public:
        SIMDProcessor();
        
        void multiply_add(const f32* input, const f32* coeffs,
                         f32* output, usize count) noexcept;
        void convolve(const f32* signal, const f32* kernel,
                     f32* output, usize signal_len, usize kernel_len) noexcept;
        
        const char* get_simd_level_name() const noexcept;
        f32 get_performance_multiplier() const noexcept; // Speedup vs scalar
    };
    
    // Performance benchmarking for education
    struct SIMDPerformanceBenchmark {
        f32 scalar_time_ms;
        f32 sse2_time_ms;
        f32 avx2_time_ms;
        f32 sse2_speedup;
        f32 avx2_speedup;
        
        std::string fastest_method;
        std::string performance_summary;
    };
    
    SIMDPerformanceBenchmark benchmark_simd_performance(usize buffer_size, usize iterations = 1000);
}

//=============================================================================
// Educational Audio Analysis and Visualization
//=============================================================================

/**
 * @brief Real-time audio analysis for educational purposes
 * 
 * Provides comprehensive audio analysis including frequency domain analysis,
 * spatial audio metrics, and real-time visualization data for teaching
 * audio engineering concepts.
 */
class AudioAnalysisEngine {
public:
    /**
     * @brief Real-time frequency domain analysis
     */
    struct FrequencyAnalysis {
        std::vector<f32> magnitude_spectrum;    // Magnitude spectrum (linear)
        std::vector<f32> magnitude_db;          // Magnitude spectrum (dB)
        std::vector<f32> phase_spectrum;        // Phase spectrum
        std::vector<f32> frequencies;           // Frequency bins (Hz)
        
        // Spectral features
        f32 spectral_centroid;                  // "Brightness" of the sound
        f32 spectral_rolloff;                   // 85% energy rolloff frequency
        f32 spectral_flux;                      // Rate of spectral change
        f32 spectral_flatness;                  // Measure of noisiness
        
        // Educational metrics
        f32 fundamental_frequency;              // Detected pitch
        std::vector<f32> harmonic_frequencies;  // Detected harmonics
        f32 harmonic_to_noise_ratio;           // HNR
        f32 total_harmonic_distortion;         // THD
    };
    
    /**
     * @brief Spatial audio analysis
     */
    struct SpatialAnalysis {
        // Binaural analysis
        f32 interaural_time_difference_us;      // ITD in microseconds
        f32 interaural_level_difference_db;     // ILD in decibels
        f32 interaural_correlation;             // Correlation between ears
        
        // Perceived spatial position
        f32 perceived_azimuth;                  // Perceived horizontal angle
        f32 perceived_elevation;                // Perceived vertical angle
        f32 perceived_distance;                 // Perceived distance
        f32 localization_confidence;           // Confidence in localization
        
        // Spatial audio quality metrics
        f32 envelopment_factor;                 // How enveloping the sound feels
        f32 source_width;                       // Apparent source width
        f32 listener_envelopment;              // LEV measure
        f32 apparent_source_width;             // ASW measure
        
        // Educational insights
        std::string spatial_description;        // Human-readable spatial description
        std::string localization_cues;         // Which cues are being used
    };
    
    /**
     * @brief Real-time waveform and visualization data
     */
    struct VisualizationData {
        // Time domain
        std::vector<f32> waveform_left;         // Left channel waveform
        std::vector<f32> waveform_right;        // Right channel waveform
        std::vector<f32> envelope;              // Audio envelope
        
        // Frequency domain
        std::vector<f32> spectrum_bins;         // Frequency bins for display
        std::vector<f32> spectrum_magnitudes;   // Magnitudes for each bin
        
        // Spectrogram data
        std::vector<std::vector<f32>> spectrogram; // 2D spectrogram data
        
        // Spatial visualization
        struct SpatialVisualization {
            f32 source_x, source_y;            // 2D position for display
            f32 listener_x, listener_y;        // Listener position
            f32 distance_circle_radius;        // Distance visualization
            f32 direction_arrow_angle;         // Direction arrow
            std::vector<f32> hrtf_response;    // HRTF frequency response
        } spatial;
        
        // Performance visualization
        f32 cpu_usage;                          // CPU usage for audio
        f32 latency_ms;                        // Current audio latency
        u32 buffer_underruns;                  // Buffer underrun count
    };

private:
    // FFT processor for frequency analysis
    std::unique_ptr<class FFTProcessor> fft_processor_;
    
    // Analysis parameters
    u32 fft_size_{1024};
    f32 overlap_factor_{0.75f};
    u32 sample_rate_;
    
    // Ring buffers for continuous analysis
    std::vector<f32> input_buffer_;
    usize buffer_write_pos_{0};
    
    // Analysis state
    std::vector<f32> window_function_;
    std::vector<std::complex<f32>> fft_buffer_;
    std::vector<f32> prev_magnitude_;  // For spectral flux calculation
    
    // Performance tracking
    mutable std::atomic<u32> analyses_performed_{0};
    mutable std::atomic<f32> analysis_time_ms_{0.0f};
    
public:
    AudioAnalysisEngine(u32 sample_rate, u32 fft_size = 1024);
    ~AudioAnalysisEngine();
    
    // Analysis functions
    FrequencyAnalysis analyze_frequency_content(const AudioBuffer& mono_audio);
    SpatialAnalysis analyze_spatial_content(const StereoBuffer& stereo_audio);
    VisualizationData generate_visualization_data(const StereoBuffer& stereo_audio,
                                                 const spatial_math::Transform3D& source_transform,
                                                 const spatial_math::Transform3D& listener_transform);
    
    // Real-time analysis (call every audio buffer)
    void feed_audio_data(const StereoBuffer& stereo_audio);
    FrequencyAnalysis get_latest_frequency_analysis() const;
    SpatialAnalysis get_latest_spatial_analysis() const;
    VisualizationData get_latest_visualization_data() const;
    
    // Educational features
    struct AudioConcept {
        std::string name;
        std::string description;
        f32 current_value;
        f32 min_value, max_value;
        std::string units;
        std::string educational_significance;
    };
    
    std::vector<AudioConcept> get_educational_concepts() const;
    std::string explain_current_analysis() const;
    
    // Configuration
    void set_fft_size(u32 size);
    void set_overlap_factor(f32 overlap); // 0.0 = no overlap, 0.75 = 75% overlap
    void set_window_function(const std::string& window_type); // "hann", "hamming", "blackman"
    
    // Performance monitoring
    struct AnalysisPerformanceInfo {
        f32 average_analysis_time_ms;
        f32 peak_analysis_time_ms;
        f32 cpu_usage_percent;
        u32 analyses_per_second;
        usize memory_usage_bytes;
    };
    
    AnalysisPerformanceInfo get_performance_info() const;
    
private:
    void initialize_fft();
    void calculate_window_function(const std::string& window_type);
    f32 calculate_spectral_centroid(const std::vector<f32>& magnitude,
                                   const std::vector<f32>& frequencies) const;
    f32 detect_fundamental_frequency(const std::vector<f32>& magnitude,
                                    const std::vector<f32>& frequencies) const;
    void calculate_spatial_metrics(const StereoBuffer& stereo_audio,
                                  f32& itd_us, f32& ild_db, f32& correlation) const;
};

} // namespace ecscope::audio