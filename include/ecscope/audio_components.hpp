#pragma once

/**
 * @file audio_components.hpp
 * @brief ECS Audio Components for ECScope 3D Spatial Audio System
 * 
 * This header defines comprehensive ECS components for 3D spatial audio,
 * integrating with the ECScope educational ECS framework. Components are
 * designed for optimal performance while providing rich educational content.
 * 
 * Key Components:
 * - AudioSource: 3D positioned audio sources with full spatial processing
 * - AudioListener: Audio receivers with head tracking and HRTF processing
 * - AudioEnvironment: Environmental audio effects and acoustic modeling
 * - AudioOccluder: Objects that block or modify audio propagation
 * - AudioMaterial: Surface properties affecting audio reflection/absorption
 * - AudioAnalyzer: Real-time audio analysis for educational purposes
 * 
 * Educational Value:
 * - Demonstrates component-based audio system architecture
 * - Shows real-world audio engine design patterns
 * - Provides insight into performance optimization for audio
 * - Illustrates spatial audio concepts through interactive components
 * - Teaches about audio-physics integration
 * 
 * Performance Features:
 * - Cache-friendly memory layouts
 * - SIMD-optimized processing paths
 * - Efficient spatial partitioning integration
 * - Memory pool compatible structures
 * - Low-overhead component updates
 * 
 * @author ECScope Educational ECS Framework - Spatial Audio Systems
 * @date 2025
 */

#include "spatial_audio_engine.hpp"
#include "ecs/component.hpp"
#include "core/types.hpp"
#include "core/math.hpp"
#include "memory/memory_tracker.hpp"
#include "physics/components.hpp"
#include "assets/importers/audio_importer.hpp"
#include <vector>
#include <array>
#include <string>
#include <memory>
#include <optional>
#include <variant>

namespace ecscope::audio::components {

// Import commonly used types
using namespace ecscope::ecs;
using namespace ecscope::audio;
using namespace ecscope::audio::spatial_math;

//=============================================================================
// Audio Component Categories (for memory tracking)
//=============================================================================

namespace categories {
    constexpr memory::AllocationCategory AUDIO_SOURCES = memory::AllocationCategory::Audio_Processing;
    constexpr memory::AllocationCategory AUDIO_BUFFERS = memory::AllocationCategory::Audio_Buffers;
    constexpr memory::AllocationCategory AUDIO_ANALYSIS = memory::AllocationCategory::Debug_Tools;
    constexpr memory::AllocationCategory AUDIO_ENVIRONMENT = memory::AllocationCategory::Audio_Processing;
}

//=============================================================================
// Audio Source Component
//=============================================================================

/**
 * @brief 3D Spatial Audio Source Component
 * 
 * Represents a positioned audio source in 3D space with full spatial audio
 * processing including HRTF, distance attenuation, Doppler effects, and
 * environmental processing. Integrates seamlessly with the ECS physics system.
 * 
 * Educational Context:
 * - Demonstrates 3D audio positioning and spatialization
 * - Shows integration between audio and physics systems
 * - Illustrates real-time audio processing concepts
 * - Provides examples of performance optimization in audio systems
 * - Teaches about audio asset management and streaming
 * 
 * Performance Design:
 * - Hot data (frequently accessed) stored at beginning of structure
 * - Cold data (rarely accessed) stored at end
 * - SIMD-friendly alignments for processing
 * - Efficient state management for audio pipeline
 */
struct alignas(32) AudioSource {
    //-------------------------------------------------------------------------
    // Core Audio Properties (Hot Data - Accessed Every Frame)
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Audio playback state
     */
    enum class PlaybackState : u8 {
        Stopped = 0,    ///< Audio is stopped
        Playing,        ///< Audio is playing
        Paused,         ///< Audio is paused
        Stopping,       ///< Audio is fading out to stop
        Starting        ///< Audio is fading in from stop
    } playback_state{PlaybackState::Stopped};
    
    /** 
     * @brief Master volume (0.0 = silent, 1.0 = full volume)
     * 
     * Educational Note: Volume in audio is typically logarithmic
     * (measured in dB), but linear scaling is often used in games
     * for simplicity and performance.
     */
    f32 volume{1.0f};
    
    /** 
     * @brief Audio pitch multiplier (0.5 = half speed, 2.0 = double speed)
     * 
     * Affects both playback speed and pitch. Real-time pitch shifting
     * without speed change requires more complex processing.
     */
    f32 pitch{1.0f};
    
    /** 
     * @brief Current playback position in seconds
     * 
     * For educational purposes, shows exact playback position
     * and enables seeking functionality.
     */
    f32 playback_position{0.0f};
    
    /** 
     * @brief Audio duration in seconds (0.0 = unknown/streaming)
     */
    f32 duration{0.0f};
    
    //-------------------------------------------------------------------------
    // 3D Spatial Properties
    //-------------------------------------------------------------------------
    
    /** 
     * @brief 3D position relative to entity transform
     * 
     * Allows audio source to be offset from the visual entity.
     * Useful for complex objects with multiple sound sources.
     */
    Vec3 local_position{0.0f, 0.0f, 0.0f};
    
    /** 
     * @brief Audio source velocity for Doppler calculations
     * 
     * Can be automatically calculated from physics or manually set.
     * Used for realistic Doppler shift effects.
     */
    Vec3 velocity{0.0f, 0.0f, 0.0f};
    
    /** 
     * @brief Distance attenuation model
     */
    enum class AttenuationModel : u8 {
        None = 0,       ///< No distance attenuation
        Linear,         ///< Linear falloff
        Inverse,        ///< Inverse distance (physically accurate)
        Exponential,    ///< Exponential decay
        Logarithmic,    ///< Logarithmic (perceptually linear)
        Custom          ///< Custom curve (see attenuation_curve)
    } attenuation_model{AttenuationModel::Inverse};
    
    /** 
     * @brief Distance attenuation parameters
     */
    struct AttenuationParams {
        f32 min_distance{1.0f};         ///< Distance where attenuation starts
        f32 max_distance{100.0f};       ///< Distance where volume reaches 0
        f32 rolloff_factor{1.0f};       ///< Rolloff steepness
        f32 reference_distance{1.0f};   ///< Reference distance for inverse model
        
        /** Custom attenuation curve (if using Custom model) */
        std::array<f32, 64> custom_curve{};  // 64 points for smooth curve
        bool curve_initialized{false};
    } attenuation{};
    
    /** 
     * @brief Directional audio properties (for directional sources)
     */
    struct DirectionalParams {
        bool is_directional{false};     ///< Whether source is directional
        Vec3 forward_direction{0.0f, 0.0f, 1.0f}; ///< Forward direction
        f32 inner_cone_angle{30.0f};    ///< Inner cone angle (degrees)
        f32 outer_cone_angle{90.0f};    ///< Outer cone angle (degrees) 
        f32 outer_cone_gain{0.5f};      ///< Volume outside outer cone
        f32 cone_transition{1.0f};      ///< Transition smoothness
    } directional{};
    
    //-------------------------------------------------------------------------
    // Audio Asset and Streaming
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Audio asset reference
     */
    struct AudioAsset {
        u32 asset_id{0};                ///< Asset system ID
        std::string asset_path;         ///< Asset file path (for debugging)
        bool is_streaming{false};       ///< Whether audio is streamed from disk
        bool is_compressed{false};      ///< Whether audio data is compressed
        u32 sample_rate{44100};         ///< Sample rate of audio data
        u16 channels{2};                ///< Number of channels (1=mono, 2=stereo, etc.)
        u16 bit_depth{16};              ///< Bit depth of samples
        
        // Audio format for educational display
        assets::importers::AudioSampleFormat sample_format{
            assets::importers::AudioSampleFormat::Int16
        };
    } audio_asset{};
    
    /** 
     * @brief Looping configuration
     */
    struct LoopingParams {
        bool is_looping{false};         ///< Whether audio loops
        f32 loop_start{0.0f};           ///< Loop start time (seconds)
        f32 loop_end{0.0f};             ///< Loop end time (seconds, 0 = end of file)
        u32 loop_count{0};              ///< Number of loops played (infinite if is_looping)
        u32 max_loops{0};               ///< Maximum loops (0 = infinite)
        f32 loop_crossfade_time{0.0f};  ///< Crossfade time at loop points
    } looping{};
    
    //-------------------------------------------------------------------------
    // Real-time Processing Configuration
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Spatial processing flags
     */
    union SpatialFlags {
        u32 flags;
        struct {
            u32 use_hrtf : 1;           ///< Use HRTF processing for 3D positioning
            u32 use_distance_attenuation : 1; ///< Apply distance attenuation
            u32 use_doppler : 1;        ///< Apply Doppler effects
            u32 use_environmental_effects : 1; ///< Apply reverb and environmental processing
            u32 use_occlusion : 1;      ///< Apply occlusion/obstruction
            u32 use_air_absorption : 1; ///< Apply air absorption (high-freq loss)
            u32 bypass_processing : 1;  ///< Bypass all spatial processing (2D audio)
            u32 auto_velocity : 1;      ///< Automatically calculate velocity from transform
            u32 lock_to_listener : 1;   ///< Move with listener (UI sounds)
            u32 ignore_pause : 1;       ///< Continue playing when game is paused
            u32 stream_from_disk : 1;   ///< Stream large files from disk
            u32 compress_in_memory : 1; ///< Use compressed audio in memory
            u32 reserved : 20;          ///< Reserved for future use
        };
        
        SpatialFlags() noexcept : flags(0x3F) {} // Enable HRTF, distance, doppler, environment, occlusion, air absorption by default
    } spatial_flags{};
    
    /** 
     * @brief Audio effects configuration
     */
    struct AudioEffects {
        // Volume envelope
        f32 fade_in_time{0.0f};         ///< Fade in duration (seconds)
        f32 fade_out_time{0.0f};        ///< Fade out duration (seconds)
        f32 current_fade_factor{1.0f};  ///< Current fade multiplier
        
        // Low-pass filter (for occlusion, distance)
        f32 low_pass_cutoff{22050.0f};  ///< Low-pass filter cutoff (Hz)
        f32 low_pass_resonance{0.7f};   ///< Filter resonance (Q factor)
        bool low_pass_enabled{false};   ///< Whether low-pass filter is active
        
        // High-pass filter (for proximity effects)
        f32 high_pass_cutoff{80.0f};    ///< High-pass filter cutoff (Hz)
        f32 high_pass_resonance{0.7f};  ///< Filter resonance (Q factor)
        bool high_pass_enabled{false};  ///< Whether high-pass filter is active
        
        // Dynamic range processing
        f32 compressor_threshold{0.8f}; ///< Compressor threshold (0-1)
        f32 compressor_ratio{4.0f};     ///< Compression ratio
        f32 compressor_attack{0.003f};  ///< Attack time (seconds)
        f32 compressor_release{0.1f};   ///< Release time (seconds)
        bool compressor_enabled{false}; ///< Whether compressor is active
        
        // Educational note: Real audio engines often have many more effects
        // This is a representative set for educational purposes
    } effects{};
    
    //-------------------------------------------------------------------------
    // Performance and Debug Information (Cold Data)
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Priority for performance management
     * 
     * Higher priority sounds are processed first and are less likely
     * to be culled during performance optimization.
     */
    enum class Priority : u8 {
        Background = 0, ///< Background ambience, least important
        Low = 1,        ///< Environmental sounds
        Normal = 2,     ///< Standard game sounds
        High = 3,       ///< Important gameplay sounds
        Critical = 4    ///< UI sounds, voice, music - never cull
    } priority{Priority::Normal};
    
    /** 
     * @brief Performance optimization state
     */
    struct PerformanceState {
        bool is_audible{true};          ///< Whether currently audible to any listener
        f32 perceived_loudness{1.0f};   ///< Perceived loudness (0-1) for culling
        f32 last_update_time{0.0f};     ///< Last time source was updated
        u32 frames_since_audible{0};    ///< Frames since last audible (for culling)
        f32 cpu_cost_estimate{1.0f};    ///< Estimated CPU cost multiplier
        
        // Quality scaling for performance
        enum class QualityLevel : u8 {
            Full = 0,    ///< Full quality processing
            High,        ///< High quality (minor optimizations)
            Medium,      ///< Medium quality (some effects disabled)
            Low,         ///< Low quality (basic processing only)
            Minimal      ///< Minimal quality (distance/volume only)
        } quality_level{QualityLevel::Full};
        
    } performance{};
    
    /** 
     * @brief Real-time analysis and educational data
     */
    mutable struct AnalysisData {
        // Audio content analysis
        f32 current_rms_level{0.0f};        ///< Current RMS audio level
        f32 peak_level{0.0f};               ///< Peak audio level
        f32 spectral_centroid{1000.0f};     ///< Spectral centroid (brightness)
        f32 dominant_frequency{440.0f};     ///< Dominant frequency component
        
        // Spatial processing analysis
        f32 distance_to_nearest_listener{10.0f}; ///< Distance to nearest listener
        f32 applied_attenuation{1.0f};      ///< Applied distance attenuation
        f32 applied_doppler_shift{1.0f};    ///< Applied Doppler shift factor
        f32 occlusion_amount{0.0f};         ///< Amount of occlusion (0-1)
        
        // Performance metrics
        f32 processing_time_ms{0.0f};       ///< Processing time per buffer
        u32 buffers_processed{0};           ///< Total buffers processed
        u32 buffer_underruns{0};            ///< Number of buffer underruns
        f32 average_cpu_usage{0.0f};        ///< Average CPU usage percentage
        
        // Educational insights
        std::string current_processing_description; ///< What processing is being applied
        std::vector<std::string> active_effects;    ///< List of active effects
        
    } analysis{};
    
    //-------------------------------------------------------------------------
    // Constructors and Factory Methods
    //-------------------------------------------------------------------------
    
    constexpr AudioSource() noexcept = default;
    
    /** @brief Create audio source with asset */
    explicit AudioSource(u32 asset_id, f32 volume = 1.0f, bool looping = false) noexcept
        : volume(volume) {
        audio_asset.asset_id = asset_id;
        looping.is_looping = looping;
    }
    
    /** @brief Create positioned audio source */
    AudioSource(u32 asset_id, const Vec3& position, f32 volume = 1.0f) noexcept
        : volume(volume), local_position(position) {
        audio_asset.asset_id = asset_id;
    }
    
    //-------------------------------------------------------------------------
    // Playback Control Interface
    //-------------------------------------------------------------------------
    
    /** @brief Start audio playback */
    void play() noexcept {
        playback_state = PlaybackState::Playing;
        if (effects.fade_in_time > 0.0f) {
            playback_state = PlaybackState::Starting;
            effects.current_fade_factor = 0.0f;
        }
    }
    
    /** @brief Pause audio playback */
    void pause() noexcept {
        if (playback_state == PlaybackState::Playing) {
            playback_state = PlaybackState::Paused;
        }
    }
    
    /** @brief Stop audio playback */
    void stop() noexcept {
        if (effects.fade_out_time > 0.0f && playback_state == PlaybackState::Playing) {
            playback_state = PlaybackState::Stopping;
        } else {
            playback_state = PlaybackState::Stopped;
            playback_position = 0.0f;
            effects.current_fade_factor = 1.0f;
        }
    }
    
    /** @brief Resume audio playback */
    void resume() noexcept {
        if (playback_state == PlaybackState::Paused) {
            playback_state = PlaybackState::Playing;
        }
    }
    
    /** @brief Seek to specific position */
    void seek(f32 position_seconds) noexcept {
        playback_position = std::clamp(position_seconds, 0.0f, duration);
    }
    
    /** @brief Check if audio is currently playing */
    bool is_playing() const noexcept {
        return playback_state == PlaybackState::Playing || 
               playback_state == PlaybackState::Starting;
    }
    
    /** @brief Check if audio has finished playing */
    bool has_finished() const noexcept {
        return playback_state == PlaybackState::Stopped && 
               !looping.is_looping && 
               playback_position >= duration;
    }
    
    //-------------------------------------------------------------------------
    // Spatial Audio Configuration
    //-------------------------------------------------------------------------
    
    /** @brief Set distance attenuation model */
    void set_attenuation_model(AttenuationModel model, f32 min_dist, f32 max_dist, f32 rolloff = 1.0f) noexcept {
        attenuation_model = model;
        attenuation.min_distance = min_dist;
        attenuation.max_distance = max_dist;
        attenuation.rolloff_factor = rolloff;
    }
    
    /** @brief Configure directional audio */
    void set_directional(const Vec3& forward_dir, f32 inner_angle, f32 outer_angle, f32 outer_gain = 0.5f) noexcept {
        directional.is_directional = true;
        directional.forward_direction = forward_dir.normalized();
        directional.inner_cone_angle = inner_angle;
        directional.outer_cone_angle = outer_angle;
        directional.outer_cone_gain = outer_gain;
    }
    
    /** @brief Disable directional audio (make omnidirectional) */
    void set_omnidirectional() noexcept {
        directional.is_directional = false;
    }
    
    /** @brief Set custom attenuation curve */
    void set_custom_attenuation_curve(const std::vector<f32>& curve_points) noexcept {
        if (curve_points.size() <= attenuation.custom_curve.size()) {
            attenuation_model = AttenuationModel::Custom;
            std::copy(curve_points.begin(), curve_points.end(), attenuation.custom_curve.begin());
            attenuation.curve_initialized = true;
        }
    }
    
    //-------------------------------------------------------------------------
    // Audio Effects Interface
    //-------------------------------------------------------------------------
    
    /** @brief Configure fade in/out times */
    void set_fade_times(f32 fade_in_seconds, f32 fade_out_seconds) noexcept {
        effects.fade_in_time = fade_in_seconds;
        effects.fade_out_time = fade_out_seconds;
    }
    
    /** @brief Enable/configure low-pass filter */
    void set_low_pass_filter(bool enabled, f32 cutoff_hz = 1000.0f, f32 resonance = 0.7f) noexcept {
        effects.low_pass_enabled = enabled;
        effects.low_pass_cutoff = cutoff_hz;
        effects.low_pass_resonance = resonance;
    }
    
    /** @brief Enable/configure compressor */
    void set_compressor(bool enabled, f32 threshold = 0.8f, f32 ratio = 4.0f) noexcept {
        effects.compressor_enabled = enabled;
        effects.compressor_threshold = threshold;
        effects.compressor_ratio = ratio;
    }
    
    //-------------------------------------------------------------------------
    // Performance and Quality Management
    //-------------------------------------------------------------------------
    
    /** @brief Set audio source priority */
    void set_priority(Priority new_priority) noexcept {
        priority = new_priority;
    }
    
    /** @brief Set quality level for performance optimization */
    void set_quality_level(PerformanceState::QualityLevel level) noexcept {
        performance.quality_level = level;
    }
    
    /** @brief Check if source is currently audible to any listener */
    bool is_audible() const noexcept {
        return performance.is_audible && 
               playback_state != PlaybackState::Stopped && 
               volume > 0.001f;
    }
    
    /** @brief Get estimated CPU cost for this source */
    f32 get_cpu_cost_estimate() const noexcept {
        f32 base_cost = 1.0f;
        
        // Spatial processing adds cost
        if (spatial_flags.use_hrtf) base_cost *= 2.0f;
        if (spatial_flags.use_environmental_effects) base_cost *= 1.5f;
        if (spatial_flags.use_doppler) base_cost *= 1.2f;
        
        // Effects add cost
        if (effects.compressor_enabled) base_cost *= 1.3f;
        if (effects.low_pass_enabled) base_cost *= 1.1f;
        
        // Quality level affects cost
        switch (performance.quality_level) {
            case PerformanceState::QualityLevel::Full: break;
            case PerformanceState::QualityLevel::High: base_cost *= 0.9f; break;
            case PerformanceState::QualityLevel::Medium: base_cost *= 0.7f; break;
            case PerformanceState::QualityLevel::Low: base_cost *= 0.5f; break;
            case PerformanceState::QualityLevel::Minimal: base_cost *= 0.2f; break;
        }
        
        return base_cost * performance.cpu_cost_estimate;
    }
    
    //-------------------------------------------------------------------------
    // Educational Analysis Interface
    //-------------------------------------------------------------------------
    
    /** @brief Get current audio processing description for educational display */
    std::string get_processing_description() const noexcept {
        std::string desc = "Audio Source Processing: ";
        
        if (spatial_flags.bypass_processing) {
            desc += "2D Audio (no spatial processing)";
            return desc;
        }
        
        desc += "3D Spatial Audio with ";
        if (spatial_flags.use_hrtf) desc += "HRTF, ";
        if (spatial_flags.use_distance_attenuation) desc += "Distance Attenuation, ";
        if (spatial_flags.use_doppler) desc += "Doppler Effects, ";
        if (spatial_flags.use_environmental_effects) desc += "Environmental Processing, ";
        if (spatial_flags.use_occlusion) desc += "Occlusion, ";
        
        // Remove trailing comma and space
        if (desc.back() == ' ') desc.pop_back();
        if (desc.back() == ',') desc.pop_back();
        
        return desc;
    }
    
    /** @brief Get spatial audio metrics for educational analysis */
    struct SpatialAudioMetrics {
        f32 distance_attenuation_db;        ///< Applied distance attenuation in dB
        f32 directional_attenuation_db;     ///< Applied directional attenuation in dB
        f32 occlusion_attenuation_db;       ///< Applied occlusion attenuation in dB
        f32 doppler_shift_semitones;        ///< Doppler shift in semitones
        f32 perceived_azimuth_degrees;      ///< Perceived horizontal position
        f32 perceived_elevation_degrees;    ///< Perceived vertical position
        f32 perceived_distance_meters;      ///< Perceived distance
        
        std::string spatial_description;    ///< Human-readable spatial description
        f32 localization_accuracy;          ///< Predicted localization accuracy (0-1)
    };
    
    SpatialAudioMetrics get_spatial_metrics() const noexcept;
    
    /** @brief Validate audio source configuration */
    bool is_valid() const noexcept {
        return volume >= 0.0f && volume <= 2.0f && // Allow slight overdrive
               pitch > 0.0f && pitch <= 4.0f &&
               playback_position >= 0.0f &&
               attenuation.min_distance >= 0.0f &&
               attenuation.max_distance > attenuation.min_distance &&
               attenuation.rolloff_factor >= 0.0f &&
               effects.fade_in_time >= 0.0f &&
               effects.fade_out_time >= 0.0f;
    }
};

//=============================================================================
// Audio Listener Component
//=============================================================================

/**
 * @brief 3D Spatial Audio Listener Component
 * 
 * Represents a listener (typically the player or camera) that receives
 * spatial audio. Handles HRTF processing, head tracking, and provides
 * educational insight into human spatial hearing.
 * 
 * Educational Context:
 * - Demonstrates human auditory system modeling
 * - Shows binaural audio processing concepts
 * - Illustrates head tracking and spatial audio
 * - Provides examples of psychoacoustic modeling
 * - Teaches about audio rendering pipelines
 */
struct alignas(32) AudioListener {
    //-------------------------------------------------------------------------
    // Listener Spatial Properties
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Listener orientation for HRTF processing
     * 
     * The listener's head orientation affects spatial audio processing.
     * In VR applications, this would be the head orientation.
     * In traditional games, this is typically the camera orientation.
     */
    Orientation head_orientation;
    
    /** 
     * @brief Listener velocity for Doppler calculations
     */
    Vec3 velocity{0.0f, 0.0f, 0.0f};
    
    /** 
     * @brief Local position offset from entity transform
     * 
     * Allows listener to be offset from entity center.
     * Useful for characters where ears aren't at the center.
     */
    Vec3 local_position{0.0f, 0.0f, 0.0f};
    
    //-------------------------------------------------------------------------
    // HRTF and Binaural Processing Configuration
    //-------------------------------------------------------------------------
    
    /** 
     * @brief HRTF processing configuration
     */
    struct HRTFConfig {
        bool enabled{true};                 ///< Enable HRTF processing
        std::string hrtf_profile{"default"}; ///< HRTF profile name
        f32 head_circumference_cm{56.0f};   ///< Head circumference for ITD calculation
        f32 interaural_distance_cm{17.0f}; ///< Distance between ears
        f32 hrtf_volume_scale{1.0f};        ///< HRTF processing volume scaling
        f32 crossfeed_amount{0.0f};         ///< Crossfeed for speaker playback (0-1)
        
        // Advanced HRTF parameters
        f32 distance_variation{0.1f};       ///< How much HRTF varies with distance
        f32 room_correction{0.0f};          ///< Room acoustic correction factor
        bool personalized_hrtf{false};     ///< Whether using personalized HRTF
        
    } hrtf_config{};
    
    /** 
     * @brief Binaural rendering quality settings
     */
    enum class BinauralQuality : u8 {
        Low = 0,        ///< Basic ITD/ILD processing only
        Medium,         ///< Simple HRTF with reduced accuracy
        High,           ///< Full HRTF with interpolation
        Ultra           ///< High-resolution HRTF with advanced features
    } binaural_quality{BinauralQuality::High};
    
    //-------------------------------------------------------------------------
    // Audio Output Configuration
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Audio output configuration
     */
    struct OutputConfig {
        enum class OutputMode : u8 {
            Stereo = 0,     ///< Stereo headphones/speakers
            Surround_5_1,   ///< 5.1 surround sound
            Surround_7_1,   ///< 7.1 surround sound
            Binaural,       ///< Binaural processing (headphones)
            Speakers_Near,  ///< Near-field monitors
            Speakers_Far    ///< Far-field speakers
        } output_mode{OutputMode::Binaural};
        
        f32 master_volume{1.0f};            ///< Master output volume
        f32 speaker_distance_m{2.0f};       ///< Distance to speakers (for speaker modes)
        f32 speaker_angle_degrees{30.0f};   ///< Speaker angle from center
        
        // Channel mapping for surround sound
        std::array<f32, 8> channel_gains{1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
        
        // Dynamic range processing
        bool enable_limiter{true};          ///< Enable output limiter
        f32 limiter_threshold{0.95f};       ///< Limiter threshold
        bool enable_loudness_compensation{false}; ///< Enable loudness compensation
        
    } output_config{};
    
    //-------------------------------------------------------------------------
    // Environmental Audio Reception
    //-------------------------------------------------------------------------
    
    /** 
     * @brief How this listener receives environmental audio
     */
    struct EnvironmentalReception {
        bool process_environment{true};     ///< Process environmental effects
        f32 reverb_sensitivity{1.0f};       ///< Sensitivity to reverb (0-2)
        f32 occlusion_sensitivity{1.0f};    ///< Sensitivity to occlusion (0-2)
        f32 distance_sensitivity{1.0f};     ///< Sensitivity to distance effects (0-2)
        f32 doppler_sensitivity{1.0f};      ///< Sensitivity to Doppler effects (0-2)
        
        // Frequency response shaping
        f32 low_frequency_gain{0.0f};       ///< Low frequency adjustment (dB)
        f32 mid_frequency_gain{0.0f};       ///< Mid frequency adjustment (dB)  
        f32 high_frequency_gain{0.0f};      ///< High frequency adjustment (dB)
        
        // Advanced environmental processing
        bool model_head_shadow{true};       ///< Model head shadowing effects
        bool model_pinna_effects{true};     ///< Model outer ear effects
        f32 room_tone_sensitivity{0.5f};    ///< Sensitivity to room tone/ambience
        
    } environmental{};
    
    //-------------------------------------------------------------------------
    // Performance and Quality Management
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Listener priority (for multiple listeners)
     * 
     * In local multiplayer scenarios, higher priority listeners
     * get more processing resources and higher quality.
     */
    enum class ListenerPriority : u8 {
        Background = 0, ///< Background listener (minimal processing)
        Low = 1,        ///< Low priority listener
        Normal = 2,     ///< Standard priority
        High = 3,       ///< High priority listener
        Primary = 4     ///< Primary listener (full quality)
    } priority{ListenerPriority::Primary};
    
    /** 
     * @brief Active audio sources being processed for this listener
     * 
     * Performance optimization: track which sources are audible
     * to this listener to avoid unnecessary processing.
     */
    struct ActiveSources {
        static constexpr usize MAX_ACTIVE_SOURCES = 64;
        std::array<u32, MAX_ACTIVE_SOURCES> source_entities;
        std::array<f32, MAX_ACTIVE_SOURCES> source_volumes;  // Perceived volumes
        std::array<f32, MAX_ACTIVE_SOURCES> source_distances; // Distances to sources
        u8 active_count{0};
        
        // Performance culling parameters
        f32 audibility_threshold{0.001f};   ///< Minimum volume to be considered audible
        f32 max_audible_distance{200.0f};   ///< Maximum distance for audibility
        u32 max_simultaneous_sources{32};   ///< Maximum sources to process simultaneously
        
    } active_sources{};
    
    //-------------------------------------------------------------------------
    // Real-time Analysis and Educational Data
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Real-time audio analysis for educational purposes
     */
    mutable struct ListenerAnalysis {
        // Spatial audio metrics
        f32 average_source_distance{10.0f}; ///< Average distance to active sources
        f32 spatial_audio_complexity{0.5f}; ///< Complexity of spatial scene (0-1)
        u32 sources_using_hrtf{0};          ///< Number of sources using HRTF
        u32 sources_using_environmental{0}; ///< Number of sources using environmental effects
        
        // Perceptual audio metrics  
        f32 perceived_loudness{0.5f};       ///< Overall perceived loudness
        f32 spatial_accuracy_estimate{0.8f}; ///< Estimated spatial accuracy
        f32 immersion_factor{0.7f};         ///< Estimated immersion factor (0-1)
        
        // Performance metrics
        f32 processing_load_percent{25.0f}; ///< CPU usage for this listener
        f32 memory_usage_mb{8.0f};          ///< Memory usage for this listener
        u32 processed_samples_per_second{0}; ///< Audio throughput
        
        // Educational insights
        std::string current_processing_summary; ///< Summary of current processing
        std::vector<std::string> active_effects; ///< List of active audio effects
        std::string spatial_scene_description;   ///< Description of spatial audio scene
        
    } analysis{};
    
    /** 
     * @brief Head tracking integration (for VR/AR)
     */
    struct HeadTracking {
        bool enabled{false};                ///< Enable head tracking
        f32 tracking_smoothing{0.1f};       ///< Smoothing factor for tracking
        f32 position_scale{1.0f};           ///< Scale factor for position tracking
        f32 rotation_scale{1.0f};           ///< Scale factor for rotation tracking
        
        // Tracking state
        Vec3 tracked_position{0.0f, 0.0f, 0.0f}; ///< Current tracked head position
        Orientation tracked_orientation;     ///< Current tracked head orientation
        f32 tracking_confidence{1.0f};      ///< Tracking confidence (0-1)
        
        // Calibration
        Vec3 calibration_offset{0.0f, 0.0f, 0.0f}; ///< Calibration position offset
        Orientation calibration_orientation; ///< Calibration orientation offset
        
    } head_tracking{};
    
    //-------------------------------------------------------------------------
    // Constructors and Configuration
    //-------------------------------------------------------------------------
    
    constexpr AudioListener() noexcept = default;
    
    /** @brief Create listener with specific HRTF profile */
    explicit AudioListener(const std::string& hrtf_profile) noexcept {
        hrtf_config.hrtf_profile = hrtf_profile;
    }
    
    /** @brief Create listener with output configuration */
    AudioListener(OutputConfig::OutputMode output_mode, f32 master_volume = 1.0f) noexcept {
        output_config.output_mode = output_mode;
        output_config.master_volume = master_volume;
    }
    
    //-------------------------------------------------------------------------
    // Configuration Interface
    //-------------------------------------------------------------------------
    
    /** @brief Set HRTF profile and parameters */
    void set_hrtf_config(const std::string& profile, f32 head_size_cm = 56.0f, 
                        f32 ear_distance_cm = 17.0f) noexcept {
        hrtf_config.hrtf_profile = profile;
        hrtf_config.head_circumference_cm = head_size_cm;
        hrtf_config.interaural_distance_cm = ear_distance_cm;
    }
    
    /** @brief Configure audio output mode */
    void set_output_mode(OutputConfig::OutputMode mode, f32 master_vol = 1.0f) noexcept {
        output_config.output_mode = mode;
        output_config.master_volume = master_vol;
    }
    
    /** @brief Enable/configure head tracking */
    void enable_head_tracking(bool enable, f32 smoothing = 0.1f) noexcept {
        head_tracking.enabled = enable;
        head_tracking.tracking_smoothing = smoothing;
    }
    
    /** @brief Set environmental audio sensitivity */
    void set_environmental_sensitivity(f32 reverb, f32 occlusion, f32 distance, f32 doppler) noexcept {
        environmental.reverb_sensitivity = reverb;
        environmental.occlusion_sensitivity = occlusion;
        environmental.distance_sensitivity = distance;
        environmental.doppler_sensitivity = doppler;
    }
    
    //-------------------------------------------------------------------------
    // Active Source Management
    //-------------------------------------------------------------------------
    
    /** @brief Add active audio source */
    void add_active_source(u32 source_entity, f32 volume, f32 distance) noexcept {
        if (active_sources.active_count < active_sources.MAX_ACTIVE_SOURCES) {
            usize idx = active_sources.active_count++;
            active_sources.source_entities[idx] = source_entity;
            active_sources.source_volumes[idx] = volume;
            active_sources.source_distances[idx] = distance;
        }
    }
    
    /** @brief Remove active audio source */
    void remove_active_source(u32 source_entity) noexcept {
        for (usize i = 0; i < active_sources.active_count; ++i) {
            if (active_sources.source_entities[i] == source_entity) {
                // Move last element to this position
                if (i < active_sources.active_count - 1) {
                    active_sources.source_entities[i] = active_sources.source_entities[active_sources.active_count - 1];
                    active_sources.source_volumes[i] = active_sources.source_volumes[active_sources.active_count - 1];
                    active_sources.source_distances[i] = active_sources.source_distances[active_sources.active_count - 1];
                }
                active_sources.active_count--;
                break;
            }
        }
    }
    
    /** @brief Clear all active sources */
    void clear_active_sources() noexcept {
        active_sources.active_count = 0;
    }
    
    /** @brief Get list of active source entities */
    std::span<const u32> get_active_source_entities() const noexcept {
        return std::span<const u32>(active_sources.source_entities.data(), active_sources.active_count);
    }
    
    //-------------------------------------------------------------------------
    // Educational Analysis Interface
    //-------------------------------------------------------------------------
    
    /** @brief Get detailed listening analysis for educational display */
    struct ListeningAnalysis {
        f32 total_perceived_loudness;       ///< Total loudness from all sources
        f32 spatial_scene_complexity;       ///< Complexity of 3D audio scene
        f32 localization_accuracy;          ///< Estimated localization accuracy
        f32 immersion_score;                ///< Audio immersion score (0-1)
        
        // Processing breakdown
        u32 sources_processed;              ///< Number of sources processed
        u32 sources_with_hrtf;              ///< Sources using HRTF
        u32 sources_with_environmental;     ///< Sources using environmental effects
        f32 cpu_usage_percent;              ///< CPU usage for this listener
        
        // Educational insights
        std::string processing_description; ///< Current processing description
        std::string spatial_audio_quality;  ///< Quality assessment
        std::vector<std::string> improvement_suggestions; ///< Suggestions for better audio
    };
    
    ListeningAnalysis get_listening_analysis() const noexcept;
    
    /** @brief Get HRTF processing information for education */
    struct HRTFProcessingInfo {
        bool is_processing_hrtf;            ///< Whether HRTF is currently active
        f32 head_radius_used_mm;            ///< Head radius used for calculations
        f32 average_itd_microseconds;       ///< Average ITD being applied
        f32 average_ild_db;                 ///< Average ILD being applied
        std::string hrtf_profile_used;      ///< HRTF profile currently in use
        f32 processing_quality_percent;     ///< Quality level of HRTF processing
        
        std::string educational_explanation; ///< Explanation of HRTF processing
    };
    
    HRTFProcessingInfo get_hrtf_processing_info() const noexcept;
    
    /** @brief Validate listener configuration */
    bool is_valid() const noexcept {
        return output_config.master_volume >= 0.0f && output_config.master_volume <= 2.0f &&
               hrtf_config.head_circumference_cm > 0.0f && hrtf_config.head_circumference_cm < 100.0f &&
               hrtf_config.interaural_distance_cm > 0.0f && hrtf_config.interaural_distance_cm < 50.0f &&
               active_sources.audibility_threshold >= 0.0f &&
               active_sources.max_audible_distance > 0.0f;
    }
};

//=============================================================================
// Audio Environment Component  
//=============================================================================

/**
 * @brief Environmental Audio Component
 * 
 * Defines acoustic properties of an environment or region that affect
 * all audio sources within it. Provides realistic environmental audio
 * effects and educational insight into room acoustics.
 * 
 * Educational Context:
 * - Demonstrates room acoustics and reverberation
 * - Shows acoustic material modeling
 * - Illustrates sound propagation in different environments
 * - Teaches about environmental audio design
 * - Provides examples of acoustic modeling techniques
 */
struct alignas(32) AudioEnvironment {
    //-------------------------------------------------------------------------
    // Environment Identification and Bounds
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Environment type for educational classification
     */
    enum class EnvironmentType : u8 {
        Generic = 0,        ///< Generic environment with custom parameters
        SmallRoom,          ///< Small enclosed room
        LargeRoom,          ///< Large enclosed room
        ConcertHall,        ///< Concert hall or auditorium
        Cathedral,          ///< Cathedral or large stone building
        Forest,             ///< Outdoor forest environment
        Cave,               ///< Underground cave
        Underwater,         ///< Underwater environment
        Space,              ///< Outer space (no air)
        Urban,              ///< Urban outdoor environment
        Vehicle,            ///< Inside vehicle
        Tunnel,             ///< Tunnel or corridor
        Custom              ///< Custom environment with user-defined properties
    } environment_type{EnvironmentType::Generic};
    
    /** 
     * @brief Environment bounds (optional - for region-based environments)
     * 
     * If specified, this environment only affects audio sources within
     * the defined region. If not specified, affects all sources globally.
     */
    struct EnvironmentBounds {
        bool use_bounds{false};             ///< Whether to use spatial bounds
        Vec3 center{0.0f, 0.0f, 0.0f};      ///< Center of environment region
        Vec3 extents{10.0f, 10.0f, 10.0f};  ///< Half-extents of environment region
        f32 falloff_distance{2.0f};         ///< Distance over which effect fades
        
        enum class BoundsShape : u8 {
            Box = 0,        ///< Rectangular bounds
            Sphere,         ///< Spherical bounds  
            Cylinder,       ///< Cylindrical bounds
            Custom          ///< Custom shape (requires additional data)
        } shape{BoundsShape::Box};
        
    } bounds{};
    
    //-------------------------------------------------------------------------
    // Acoustic Properties
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Room acoustic parameters
     * 
     * Based on standard room acoustics measurements and modeling.
     * Educational opportunity to learn about RT60, early reflections, etc.
     */
    struct AcousticProperties {
        // Size and geometry
        Vec3 room_dimensions{10.0f, 3.0f, 8.0f};    ///< Room size (width, height, depth)
        f32 room_volume{240.0f};                    ///< Room volume (cubic meters)
        f32 surface_area{236.0f};                   ///< Total surface area (square meters)
        
        // Reverberation characteristics  
        f32 reverb_time_rt60{1.5f};                 ///< RT60 reverberation time (seconds)
        f32 early_decay_time{1.2f};                 ///< Early decay time (seconds)
        f32 clarity_c50{2.0f};                      ///< C50 clarity measure (dB)
        f32 definition_d50{0.6f};                   ///< D50 definition (0-1)
        
        // Material properties
        f32 absorption_coefficient{0.25f};          ///< Average absorption coefficient (0-1)
        f32 diffusion_coefficient{0.7f};            ///< Average diffusion coefficient (0-1)
        f32 scattering_coefficient{0.5f};           ///< Scattering coefficient (0-1)
        
        // Frequency-dependent properties
        struct FrequencyResponse {
            f32 low_frequency_rt60{2.0f};           ///< RT60 at 125 Hz
            f32 mid_frequency_rt60{1.5f};           ///< RT60 at 1000 Hz  
            f32 high_frequency_rt60{1.0f};          ///< RT60 at 4000 Hz
            
            f32 low_frequency_absorption{0.1f};     ///< Low-freq absorption
            f32 mid_frequency_absorption{0.25f};    ///< Mid-freq absorption
            f32 high_frequency_absorption{0.4f};    ///< High-freq absorption
        } frequency_response{};
        
        // Air properties
        f32 air_absorption_coefficient{0.001f};     ///< Air absorption (per meter)
        f32 temperature_celsius{20.0f};             ///< Air temperature
        f32 humidity_percent{50.0f};                ///< Relative humidity
        f32 atmospheric_pressure_kpa{101.325f};     ///< Atmospheric pressure
        
        // Advanced acoustic properties
        f32 background_noise_level{-40.0f};         ///< Background noise level (dB SPL)
        f32 acoustic_coupling{0.1f};                ///< Coupling to adjacent spaces
        Vec3 primary_reflection_direction{0.0f, -1.0f, 0.0f}; ///< Primary reflection direction
        
    } acoustic_properties{};
    
    //-------------------------------------------------------------------------
    // Environmental Effects Configuration
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Reverb processing configuration
     */
    struct ReverbConfig {
        bool enabled{true};                         ///< Enable reverb processing
        f32 reverb_gain{0.3f};                      ///< Reverb mix level (0-1)
        f32 pre_delay{0.02f};                       ///< Pre-delay time (seconds)
        f32 room_size{0.7f};                        ///< Room size parameter (0-1)
        f32 damping{0.5f};                          ///< High-frequency damping (0-1)
        f32 diffusion{0.8f};                        ///< Diffusion amount (0-1)
        f32 density{1.0f};                          ///< Echo density (0-1)
        
        // Advanced reverb parameters
        f32 modulation_rate{0.1f};                  ///< Modulation rate (Hz)
        f32 modulation_depth{0.02f};                ///< Modulation depth (0-1)
        f32 stereo_width{1.0f};                     ///< Stereo width (0-2)
        
        // Early reflections
        f32 early_reflections_gain{0.5f};           ///< Early reflections level
        f32 early_reflections_delay{0.01f};         ///< Early reflections delay
        
    } reverb_config{};
    
    /** 
     * @brief Environmental filters and effects
     */
    struct EnvironmentalEffects {
        // Distance-based effects
        f32 air_absorption_scale{1.0f};             ///< Scale factor for air absorption
        f32 doppler_scale{1.0f};                    ///< Scale factor for Doppler effects
        f32 distance_delay_scale{1.0f};             ///< Scale factor for distance delays
        
        // Atmospheric effects
        Vec3 wind_velocity{0.0f, 0.0f, 0.0f};       ///< Wind velocity (m/s)
        f32 wind_turbulence{0.0f};                  ///< Wind turbulence amount (0-1)
        f32 atmospheric_distortion{0.0f};           ///< Atmospheric distortion (0-1)
        
        // Occlusion and obstruction
        bool enable_occlusion_calculation{true};    ///< Enable automatic occlusion
        f32 occlusion_intensity{1.0f};              ///< Occlusion effect intensity
        f32 obstruction_intensity{1.0f};            ///< Obstruction effect intensity
        
        // Special environmental effects
        f32 echo_intensity{0.0f};                   ///< Echo intensity (for canyons, etc.)
        f32 echo_delay{0.5f};                       ///< Echo delay time (seconds)
        f32 underwater_effect{0.0f};                ///< Underwater distortion (0-1)
        f32 pressure_effect{0.0f};                  ///< Pressure/altitude effects (0-1)
        
    } environmental_effects{};
    
    //-------------------------------------------------------------------------
    // Performance and Quality Settings
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Environment processing priority
     */
    enum class ProcessingPriority : u8 {
        Background = 0, ///< Background processing, low quality
        Low = 1,        ///< Low priority, reduced quality
        Normal = 2,     ///< Standard processing priority
        High = 3,       ///< High priority, enhanced quality
        Critical = 4    ///< Critical environments, maximum quality
    } processing_priority{ProcessingPriority::Normal};
    
    /** 
     * @brief Environment processing state
     */
    struct ProcessingState {
        bool is_active{true};                       ///< Whether environment is active
        f32 blend_factor{1.0f};                     ///< Blend with other environments (0-1)
        f32 last_update_time{0.0f};                 ///< Last update timestamp
        u32 active_sources_count{0};                ///< Number of sources in this environment
        
        // Quality scaling
        enum class QualityLevel : u8 {
            Minimal = 0,    ///< Minimal processing (basic reverb only)
            Low,            ///< Low quality (reduced parameters)
            Medium,         ///< Medium quality (standard parameters)
            High,           ///< High quality (enhanced parameters)
            Ultra           ///< Ultra quality (maximum parameters)
        } quality_level{QualityLevel::High};
        
        // Performance optimization
        f32 cpu_usage_estimate{5.0f};               ///< Estimated CPU usage (percentage)
        f32 memory_usage_estimate{2.0f};            ///< Estimated memory usage (MB)
        bool enable_culling{true};                  ///< Enable performance culling
        f32 culling_distance{100.0f};               ///< Distance-based culling
        
    } processing_state{};
    
    //-------------------------------------------------------------------------
    // Educational and Analysis Data
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Educational information about this environment
     */
    struct EducationalInfo {
        std::string environment_name;               ///< Human-readable environment name
        std::string acoustic_description;           ///< Acoustic characteristics description
        std::vector<std::string> key_concepts;      ///< Key acoustic concepts demonstrated
        f32 educational_value{0.7f};                ///< Educational value rating (0-1)
        
        // Real-world examples
        std::string real_world_examples;            ///< Real-world equivalent environments
        std::string design_considerations;          ///< Audio design considerations
        std::vector<std::string> improvement_tips;  ///< Tips for improving acoustics
        
        // Technical details
        std::string reverb_algorithm_info;          ///< Information about reverb algorithm
        std::string acoustic_modeling_info;         ///< Information about acoustic modeling
        
    } educational_info{};
    
    /** 
     * @brief Real-time environment analysis
     */
    mutable struct EnvironmentAnalysis {
        // Acoustic measurements
        f32 measured_rt60{0.0f};                    ///< Currently measured RT60
        f32 measured_clarity{0.0f};                 ///< Currently measured clarity
        f32 measured_definition{0.0f};              ///< Currently measured definition
        f32 background_noise_measured{-50.0f};     ///< Measured background noise
        
        // Source interaction analysis
        u32 sources_affected{0};                    ///< Number of sources affected
        f32 average_source_distance{10.0f};        ///< Average distance to sources
        f32 reverb_contribution_db{-20.0f};        ///< Reverb contribution level
        f32 environmental_complexity{0.5f};        ///< Environment complexity score
        
        // Performance analysis
        f32 processing_load_percent{5.0f};          ///< Current CPU usage
        f32 memory_usage_mb{2.0f};                  ///< Current memory usage
        u32 convolution_operations{0};              ///< Convolution operations per second
        
        // Educational insights
        std::string current_acoustic_state;         ///< Current acoustic state description
        std::string dominant_acoustic_feature;      ///< Most prominent acoustic feature
        std::vector<std::string> active_processing; ///< Currently active processing
        
    } analysis{};
    
    //-------------------------------------------------------------------------
    // Constructors and Factory Methods
    //-------------------------------------------------------------------------
    
    constexpr AudioEnvironment() noexcept = default;
    
    /** @brief Create environment of specific type */
    explicit AudioEnvironment(EnvironmentType type) noexcept : environment_type(type) {
        initialize_preset_parameters(type);
    }
    
    /** @brief Create environment with bounds */
    AudioEnvironment(EnvironmentType type, const Vec3& center, const Vec3& extents) noexcept 
        : environment_type(type) {
        bounds.use_bounds = true;
        bounds.center = center;
        bounds.extents = extents;
        initialize_preset_parameters(type);
    }
    
    //-------------------------------------------------------------------------
    // Environment Configuration Interface
    //-------------------------------------------------------------------------
    
    /** @brief Set environment type and initialize parameters */
    void set_environment_type(EnvironmentType type) noexcept {
        environment_type = type;
        initialize_preset_parameters(type);
    }
    
    /** @brief Configure environment bounds */
    void set_bounds(const Vec3& center, const Vec3& extents, f32 falloff = 2.0f) noexcept {
        bounds.use_bounds = true;
        bounds.center = center;
        bounds.extents = extents;
        bounds.falloff_distance = falloff;
    }
    
    /** @brief Remove environment bounds (make global) */
    void clear_bounds() noexcept {
        bounds.use_bounds = false;
    }
    
    /** @brief Set room dimensions and recalculate acoustic properties */
    void set_room_dimensions(const Vec3& dimensions) noexcept {
        acoustic_properties.room_dimensions = dimensions;
        acoustic_properties.room_volume = dimensions.x * dimensions.y * dimensions.z;
        acoustic_properties.surface_area = 2.0f * (dimensions.x * dimensions.y + 
                                                   dimensions.y * dimensions.z + 
                                                   dimensions.z * dimensions.x);
        recalculate_acoustic_properties();
    }
    
    /** @brief Configure reverb parameters */
    void set_reverb_config(f32 rt60, f32 gain, f32 pre_delay, f32 room_size) noexcept {
        acoustic_properties.reverb_time_rt60 = rt60;
        reverb_config.reverb_gain = gain;
        reverb_config.pre_delay = pre_delay;
        reverb_config.room_size = room_size;
    }
    
    /** @brief Set material properties for surfaces */
    void set_material_properties(f32 absorption, f32 diffusion, f32 scattering) noexcept {
        acoustic_properties.absorption_coefficient = absorption;
        acoustic_properties.diffusion_coefficient = diffusion;
        acoustic_properties.scattering_coefficient = scattering;
        recalculate_acoustic_properties();
    }
    
    //-------------------------------------------------------------------------
    // Spatial Testing and Interaction
    //-------------------------------------------------------------------------
    
    /** @brief Test if a position is within this environment */
    bool contains_point(const Vec3& world_position) const noexcept {
        if (!bounds.use_bounds) return true; // Global environment
        
        Vec3 relative_pos = world_position - bounds.center;
        
        switch (bounds.shape) {
            case EnvironmentBounds::BoundsShape::Box:
                return std::abs(relative_pos.x) <= bounds.extents.x &&
                       std::abs(relative_pos.y) <= bounds.extents.y &&
                       std::abs(relative_pos.z) <= bounds.extents.z;
                       
            case EnvironmentBounds::BoundsShape::Sphere: {
                f32 radius = bounds.extents.x; // Use X extent as radius
                return relative_pos.length_squared() <= radius * radius;
            }
            
            case EnvironmentBounds::BoundsShape::Cylinder: {
                f32 radius = bounds.extents.x; // Use X extent as radius
                f32 height = bounds.extents.y; // Use Y extent as height
                f32 horizontal_dist_sq = relative_pos.x * relative_pos.x + relative_pos.z * relative_pos.z;
                return horizontal_dist_sq <= radius * radius && std::abs(relative_pos.y) <= height;
            }
            
            default:
                return true;
        }
    }
    
    /** @brief Get environment influence factor at position (0-1) */
    f32 get_influence_factor(const Vec3& world_position) const noexcept {
        if (!bounds.use_bounds) return 1.0f; // Global environment
        
        if (!contains_point(world_position)) {
            // Calculate distance-based falloff
            Vec3 relative_pos = world_position - bounds.center;
            f32 distance_outside = 0.0f;
            
            switch (bounds.shape) {
                case EnvironmentBounds::BoundsShape::Box: {
                    Vec3 closest_point = {
                        std::clamp(relative_pos.x, -bounds.extents.x, bounds.extents.x),
                        std::clamp(relative_pos.y, -bounds.extents.y, bounds.extents.y),
                        std::clamp(relative_pos.z, -bounds.extents.z, bounds.extents.z)
                    };
                    distance_outside = (relative_pos - closest_point).length();
                    break;
                }
                
                case EnvironmentBounds::BoundsShape::Sphere: {
                    f32 radius = bounds.extents.x;
                    distance_outside = relative_pos.length() - radius;
                    break;
                }
                
                default:
                    distance_outside = 0.0f; // Fallback
            }
            
            if (distance_outside > bounds.falloff_distance) return 0.0f;
            return 1.0f - (distance_outside / bounds.falloff_distance);
        }
        
        return 1.0f; // Inside bounds
    }
    
    //-------------------------------------------------------------------------
    // Educational Analysis Interface  
    //-------------------------------------------------------------------------
    
    /** @brief Get acoustic analysis for educational display */
    struct AcousticAnalysis {
        f32 reverb_time_calculated;                ///< Calculated RT60 based on parameters
        f32 room_volume_calculated;                ///< Calculated room volume
        f32 absorption_area_calculated;            ///< Calculated total absorption area
        f32 critical_distance;                     ///< Critical distance for this room
        
        std::string acoustic_classification;       ///< Acoustic classification of environment
        std::string reverberation_quality;         ///< Quality assessment of reverberation
        std::string speech_intelligibility;        ///< Assessment of speech intelligibility
        std::string music_suitability;             ///< Suitability for music reproduction
        
        std::vector<std::string> acoustic_issues;  ///< Identified acoustic problems
        std::vector<std::string> improvement_suggestions; ///< Suggestions for improvement
        
        f32 educational_interest_score;            ///< How interesting for education (0-1)
    };
    
    AcousticAnalysis get_acoustic_analysis() const noexcept;
    
    /** @brief Get environment processing information */
    struct ProcessingInfo {
        bool is_currently_active;                  ///< Whether actively processing
        u32 sources_being_processed;               ///< Number of sources being processed
        f32 cpu_usage_percent;                     ///< Current CPU usage
        f32 reverb_processing_cost;                ///< Cost of reverb processing
        f32 environmental_effects_cost;            ///< Cost of environmental effects
        
        std::string processing_quality_level;      ///< Current quality level
        std::string optimization_status;           ///< Performance optimization status
        std::vector<std::string> active_effects;   ///< List of active effects
    };
    
    ProcessingInfo get_processing_info() const noexcept;
    
    /** @brief Validate environment configuration */
    bool is_valid() const noexcept {
        return acoustic_properties.reverb_time_rt60 > 0.0f &&
               acoustic_properties.room_volume > 0.0f &&
               acoustic_properties.surface_area > 0.0f &&
               acoustic_properties.absorption_coefficient >= 0.0f && 
               acoustic_properties.absorption_coefficient <= 1.0f &&
               reverb_config.reverb_gain >= 0.0f && 
               reverb_config.reverb_gain <= 1.0f &&
               reverb_config.pre_delay >= 0.0f &&
               (bounds.use_bounds ? bounds.extents.x > 0.0f && bounds.extents.y > 0.0f && bounds.extents.z > 0.0f : true);
    }
    
private:
    /** @brief Initialize parameters based on environment type preset */
    void initialize_preset_parameters(EnvironmentType type) noexcept;
    
    /** @brief Recalculate acoustic properties based on current parameters */
    void recalculate_acoustic_properties() noexcept;
};

//=============================================================================
// Component Validation and Static Assertions
//=============================================================================

// Ensure all audio components satisfy the ECS Component concept
static_assert(ecs::Component<AudioSource>, "AudioSource must satisfy Component concept");
static_assert(ecs::Component<AudioListener>, "AudioListener must satisfy Component concept");
static_assert(ecs::Component<AudioEnvironment>, "AudioEnvironment must satisfy Component concept");

// Verify memory alignment for optimal performance
static_assert(alignof(AudioSource) >= 32, "AudioSource should be 32-byte aligned");
static_assert(alignof(AudioListener) >= 32, "AudioListener should be 32-byte aligned");  
static_assert(alignof(AudioEnvironment) >= 32, "AudioEnvironment should be 32-byte aligned");

// Verify reasonable component sizes for cache efficiency
static_assert(sizeof(AudioSource) <= 1024, "AudioSource should fit in reasonable cache size");
static_assert(sizeof(AudioListener) <= 1024, "AudioListener should fit in reasonable cache size");
static_assert(sizeof(AudioEnvironment) <= 2048, "AudioEnvironment should fit in reasonable cache size");

//=============================================================================
// Utility Functions and Component Relationships
//=============================================================================

namespace utils {
    /**
     * @brief Calculate 3D distance between audio source and listener
     */
    f32 calculate_3d_distance(const AudioSource& source, const spatial_math::Transform3D& source_transform,
                             const AudioListener& listener, const spatial_math::Transform3D& listener_transform) noexcept;
    
    /**
     * @brief Calculate relative positioning for spatial audio
     */
    spatial_math::Transform3D::RelativePosition calculate_relative_position(
        const AudioSource& source, const spatial_math::Transform3D& source_transform,
        const AudioListener& listener, const spatial_math::Transform3D& listener_transform) noexcept;
    
    /**
     * @brief Helper to create a complete spatial audio entity
     */
    struct SpatialAudioEntityDesc {
        u32 audio_asset_id;
        Vec3 position{0.0f, 0.0f, 0.0f};
        f32 volume{1.0f};
        bool is_looping{false};
        AudioSource::AttenuationModel attenuation_model{AudioSource::AttenuationModel::Inverse};
        f32 min_distance{1.0f};
        f32 max_distance{100.0f};
        bool enable_hrtf{true};
        bool enable_environmental_effects{true};
    };
    
    /**
     * @brief Create audio source component with reasonable defaults
     */
    AudioSource create_spatial_audio_source(const SpatialAudioEntityDesc& desc) noexcept;
    
    /**
     * @brief Create audio listener component for player/camera
     */
    AudioListener create_player_listener(AudioListener::OutputConfig::OutputMode output_mode = AudioListener::OutputConfig::OutputMode::Binaural) noexcept;
    
    /**
     * @brief Create environment component for room/area
     */
    AudioEnvironment create_room_environment(AudioEnvironment::EnvironmentType type,
                                           const Vec3& center, const Vec3& extents) noexcept;
    
    /**
     * @brief Validate spatial audio component compatibility
     */
    bool validate_spatial_audio_components(const AudioSource* source,
                                         const AudioListener* listener,
                                         const AudioEnvironment* environment) noexcept;
    
    /**
     * @brief Calculate estimated CPU cost for spatial audio processing
     */
    f32 estimate_spatial_audio_cpu_cost(const AudioSource& source, const AudioListener& listener,
                                       const AudioEnvironment* environment = nullptr) noexcept;
}

} // namespace ecscope::audio::components