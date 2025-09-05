#pragma once

/**
 * @file audio_importer.hpp
 * @brief Advanced Audio Import System for ECScope Asset Pipeline
 * 
 * This system provides comprehensive audio import capabilities with support
 * for multiple formats, audio processing, and educational features for
 * teaching audio programming concepts.
 * 
 * Key Features:
 * - Multi-format support (WAV, MP3, OGG, FLAC, M4A, WMA)
 * - Advanced audio processing and effects
 * - Real-time audio analysis and visualization
 * - Educational tools for understanding audio concepts
 * - Integration with audio system and memory management
 * - Performance optimization for games
 * 
 * Educational Value:
 * - Demonstrates digital audio concepts
 * - Shows audio compression trade-offs
 * - Illustrates signal processing techniques
 * - Provides audio analysis and visualization tools
 * - Teaches performance considerations for game audio
 * 
 * @author ECScope Educational ECS Framework - Asset Pipeline
 * @date 2025
 */

#include "asset_pipeline.hpp"
#include "core/types.hpp"
#include <vector>
#include <string>
#include <memory>
#include <complex>
#include <optional>
#include <chrono>

namespace ecscope::assets::importers {

//=============================================================================
// Audio Data Structures
//=============================================================================

/**
 * @brief Audio sample formats
 */
enum class AudioSampleFormat : u8 {
    Unknown = 0,
    UInt8,          // 8-bit unsigned integer
    Int16,          // 16-bit signed integer (most common)
    Int24,          // 24-bit signed integer (packed)
    Int32,          // 32-bit signed integer
    Float32,        // 32-bit floating point
    Float64         // 64-bit floating point (rarely used)
};

/**
 * @brief Raw audio data container
 */
struct AudioData {
    // Format information
    u32 sample_rate{44100};           // Samples per second (Hz)
    u16 channels{2};                  // 1 = mono, 2 = stereo, etc.
    u16 bits_per_sample{16};          // Bit depth
    AudioSampleFormat sample_format{AudioSampleFormat::Int16};
    
    // Raw sample data
    std::vector<u8> sample_data;      // Raw audio samples
    usize frame_count{0};             // Number of audio frames
    f64 duration_seconds{0.0};        // Total duration
    
    // Metadata
    std::string title;
    std::string artist;
    std::string album;
    std::string genre;
    u32 year{0};
    std::string comment;
    
    // Technical information
    bool is_compressed{false};
    f32 compression_ratio{1.0f};
    u32 bitrate{0};                   // Bits per second (for compressed formats)
    std::string codec;                // Codec used (MP3, OGG Vorbis, etc.)
    
    AudioData() = default;
    
    // Utility functions
    usize get_sample_size_bytes() const;
    usize get_frame_size_bytes() const;
    usize calculate_memory_usage() const;
    f64 calculate_duration() const;
    
    // Sample access (template for different sample types)
    template<typename T>
    T get_sample(usize channel, usize frame) const;
    
    template<typename T>
    void set_sample(usize channel, usize frame, T value);
    
    // Channel operations
    bool is_mono() const { return channels == 1; }
    bool is_stereo() const { return channels == 2; }
    AudioData extract_channel(u16 channel_index) const;
    AudioData mix_to_mono() const;
    
    // Format conversion
    bool convert_sample_rate(u32 new_sample_rate);
    bool convert_bit_depth(u16 new_bits_per_sample);
    bool convert_channels(u16 new_channel_count);
    
    // Validation
    bool is_valid() const;
    std::vector<std::string> validate() const;
};

//=============================================================================
// Audio Analysis and Educational Features
//=============================================================================

/**
 * @brief Comprehensive audio analysis for educational purposes
 */
struct AudioAnalysis {
    // Basic properties
    f64 duration_seconds{0.0};
    u32 sample_rate{0};
    u16 channels{0};
    u16 bit_depth{0};
    usize file_size_bytes{0};
    
    // Signal analysis
    struct SignalAnalysis {
        f32 peak_amplitude{0.0f};         // Maximum amplitude (0.0-1.0)
        f32 rms_amplitude{0.0f};          // Root mean square amplitude
        f32 dynamic_range{0.0f};          // Difference between loudest and quietest parts
        f32 crest_factor{0.0f};           // Peak to RMS ratio
        bool has_clipping{false};         // Audio clipping detected
        f32 dc_offset{0.0f};              // DC bias in the signal
        f32 signal_to_noise_ratio{0.0f};  // SNR estimate
        
        // Frequency analysis
        f32 dominant_frequency{0.0f};     // Most prominent frequency
        f32 spectral_centroid{0.0f};      // "Brightness" of the sound
        f32 spectral_rolloff{0.0f};       // Frequency below which 85% of energy lies
        f32 zero_crossing_rate{0.0f};     // Measure of noisiness
        
        // Stereo analysis (if applicable)
        f32 stereo_width{1.0f};           // Stereo image width
        f32 left_right_balance{0.0f};     // Balance between channels
        bool mono_compatible{true};       // Safe to mix to mono
        
    } signal;
    
    // Content analysis
    struct ContentAnalysis {
        enum class AudioType {
            Unknown, Speech, Music, SoundEffect, Ambient, Noise
        } detected_type{AudioType::Unknown};
        
        bool has_silence{false};
        f32 silence_percentage{0.0f};
        std::vector<std::pair<f64, f64>> silence_regions; // Start, end times
        
        bool has_music{false};
        bool has_speech{false};
        bool has_transients{false};        // Sharp attacks (drums, etc.)
        
        f32 tempo_bpm{0.0f};              // Estimated tempo for music
        f32 rhythm_strength{0.0f};        // How rhythmic the audio is
        
    } content;
    
    // Quality assessment
    struct QualityMetrics {
        f32 overall_quality{1.0f};        // 0.0 = poor, 1.0 = excellent
        std::vector<std::string> quality_issues;
        
        bool suitable_for_games{true};
        bool needs_processing{false};
        bool suitable_for_looping{false};
        bool has_fade_in{false};
        bool has_fade_out{false};
        
        // Compression analysis
        bool shows_compression_artifacts{false};
        f32 estimated_original_bitrate{0.0f};
        std::string recommended_format;
        
    } quality;
    
    // Educational insights
    struct EducationalInfo {
        std::string complexity_level;     // "Simple", "Moderate", "Complex"
        std::vector<std::string> concepts_demonstrated;
        std::vector<std::string> learning_opportunities;
        std::string recommended_exercises;
        f32 educational_value{0.5f};      // 0.0-1.0
        
        // Technical learning points
        std::string nyquist_explanation;
        std::string aliasing_risk;
        std::string compression_trade_offs;
        
    } educational;
    
    // Performance considerations
    struct PerformanceInfo {
        usize memory_usage_estimate{0};
        f32 decode_cost_score{1.0f};      // Computational cost to decode
        f32 streaming_suitability{1.0f}; // How suitable for streaming
        bool suitable_for_mobile{true};
        
        std::vector<std::string> optimization_suggestions;
        
    } performance;
};

/**
 * @brief Real-time audio processing for educational visualization
 */
class AudioAnalyzer {
private:
    // FFT for frequency analysis
    std::vector<std::complex<f32>> fft_buffer_;
    std::vector<f32> window_function_;
    u32 fft_size_{1024};
    
    // Analysis parameters
    f32 hop_size_{0.25f};             // Overlap between analysis windows
    u32 window_type_{1};              // 0=rectangular, 1=hanning, 2=hamming
    
public:
    AudioAnalyzer(u32 fft_size = 1024);
    
    // Comprehensive analysis
    AudioAnalysis analyze_audio_data(const AudioData& audio) const;
    
    // Real-time analysis (for educational visualization)
    struct RealTimeAnalysis {
        std::vector<f32> frequency_spectrum; // Current frequency content
        f32 current_level{0.0f};              // Current audio level
        f32 peak_level{0.0f};                 // Peak level
        std::vector<f32> level_history;       // Recent level history for visualization
        
        // Spectral features for visualization
        f32 spectral_centroid{0.0f};
        f32 spectral_bandwidth{0.0f};
        std::vector<f32> mel_frequency_cepstral_coefficients; // MFCCs for advanced analysis
    };
    
    RealTimeAnalysis analyze_buffer(
        const f32* samples,
        usize sample_count,
        u32 sample_rate,
        u16 channels = 1
    );
    
    // Educational visualizations
    std::vector<f32> generate_waveform_data(const AudioData& audio, u32 width = 800) const;
    std::vector<f32> generate_spectrum_data(const AudioData& audio, u32 bins = 512) const;
    std::vector<std::vector<f32>> generate_spectrogram_data(
        const AudioData& audio,
        u32 time_bins = 100,
        u32 freq_bins = 512
    ) const;
    
    // Audio feature extraction for machine learning
    struct AudioFeatures {
        // Temporal features
        f32 zero_crossing_rate{0.0f};
        f32 energy{0.0f};
        f32 entropy{0.0f};
        
        // Spectral features
        f32 spectral_centroid{0.0f};
        f32 spectral_rolloff{0.0f};
        f32 spectral_flux{0.0f};
        std::vector<f32> mfccs;           // Mel-frequency cepstral coefficients
        std::vector<f32> chroma_features; // Pitch class profiles
        
        // Rhythm features
        f32 tempo{0.0f};
        std::vector<f32> onset_times;
        f32 rhythmic_regularity{0.0f};
    };
    
    AudioFeatures extract_features(const AudioData& audio) const;
    
private:
    void compute_fft(std::vector<std::complex<f32>>& data) const;
    void apply_window_function(std::vector<f32>& data, u32 window_type) const;
    f32 compute_spectral_centroid(const std::vector<f32>& spectrum, f32 sample_rate) const;
    std::vector<f32> compute_mel_filterbank(const std::vector<f32>& spectrum, u32 sample_rate) const;
};

//=============================================================================
// Audio Processing Effects
//=============================================================================

/**
 * @brief Audio processing effects for educational demonstration
 */
class AudioProcessor {
public:
    // Basic effects
    static void apply_gain(AudioData& audio, f32 gain_db);
    static void apply_fade_in(AudioData& audio, f64 fade_time_seconds);
    static void apply_fade_out(AudioData& audio, f64 fade_time_seconds);
    static void normalize_audio(AudioData& audio, f32 target_db = -3.0f);
    
    // Filtering
    enum class FilterType {
        LowPass, HighPass, BandPass, BandReject, Notch
    };
    
    static void apply_filter(
        AudioData& audio,
        FilterType type,
        f32 cutoff_frequency,
        f32 q_factor = 1.0f
    );
    
    // Dynamic range processing
    static void apply_compressor(
        AudioData& audio,
        f32 threshold_db,
        f32 ratio,
        f32 attack_ms,
        f32 release_ms
    );
    
    static void apply_limiter(AudioData& audio, f32 threshold_db);
    static void apply_noise_gate(AudioData& audio, f32 threshold_db, f32 ratio = 10.0f);
    
    // Spatial effects
    static AudioData create_stereo_delay(const AudioData& mono_audio, f32 delay_ms);
    static void apply_panning(AudioData& stereo_audio, f32 pan); // -1.0 = left, 1.0 = right
    
    // Educational processing with visualization
    struct ProcessingStep {
        std::string effect_name;
        std::string description;
        std::string parameters;
        f64 processing_time_ms;
        f32 quality_impact;
        AudioData before_data; // For A/B comparison
        AudioData after_data;
        
        std::function<void()> show_visualization; // Display effect results
    };
    
    static std::vector<ProcessingStep> process_with_steps(
        AudioData& audio,
        const AudioImportSettings& settings
    );
    
private:
    // DSP utility functions
    static f32 db_to_linear(f32 db) { return std::pow(10.0f, db / 20.0f); }
    static f32 linear_to_db(f32 linear) { return 20.0f * std::log10(std::max(linear, 1e-6f)); }
    
    // Filter implementations
    struct BiquadFilter {
        f32 b0, b1, b2, a1, a2;
        f32 x1{0.0f}, x2{0.0f}, y1{0.0f}, y2{0.0f};
        
        f32 process(f32 input);
        void set_coefficients(FilterType type, f32 frequency, f32 sample_rate, f32 q);
    };
};

//=============================================================================
// Audio Import Settings
//=============================================================================

/**
 * @brief Extended audio import settings
 */
struct AudioImportSettings : public ImportSettings {
    // Target format
    u32 target_sample_rate{44100};
    u16 target_channels{2};
    u16 target_bit_depth{16};
    AudioSampleFormat target_sample_format{AudioSampleFormat::Int16};
    
    // Quality settings
    bool high_quality_resampling{true};
    f32 compression_quality{0.9f};      // For lossy formats
    bool preserve_original_quality{false}; // Don't re-encode if possible
    
    // Processing options
    bool normalize_audio{false};
    f32 target_loudness_lufs{-23.0f};   // EBU R128 standard
    bool remove_dc_offset{true};
    bool apply_fade_in{false};
    bool apply_fade_out{false};
    f64 fade_duration_seconds{0.1};
    
    // Looping settings
    bool detect_loop_points{false};
    bool create_seamless_loop{false};
    f64 loop_start_seconds{0.0};
    f64 loop_end_seconds{0.0};
    
    // Educational settings
    bool generate_waveform_preview{true};
    bool calculate_audio_features{true};
    bool create_analysis_data{true};
    
    // Performance optimization
    bool optimize_for_streaming{false};
    bool create_compressed_version{false};
    std::string preferred_codec{"auto"}; // auto, wav, ogg, mp3
    
    std::string serialize() const override;
    bool deserialize(const std::string& data) override;
    std::string calculate_hash() const override;
};

//=============================================================================
// Audio Format Support
//=============================================================================

/**
 * @brief Audio format support detection and information
 */
class AudioFormatSupport {
private:
    static std::unordered_map<std::string, bool> format_support_cache_;
    static bool cache_initialized_;
    
public:
    static void initialize();
    static bool is_format_supported(const std::string& extension);
    static std::vector<std::string> get_supported_extensions();
    
    // Format information for educational purposes
    struct FormatInfo {
        std::string name;
        std::string description;
        bool is_compressed;
        bool is_lossless;
        std::vector<u32> supported_sample_rates;
        std::vector<u16> supported_bit_depths;
        f32 typical_compression_ratio;
        std::string quality_assessment;
        std::vector<std::string> common_uses;
    };
    
    static FormatInfo get_format_info(const std::string& extension);
    static std::string get_format_comparison_table();
};

//=============================================================================
// Audio Importer Implementation
//=============================================================================

/**
 * @brief Main audio importer with educational features
 */
class AudioImporter : public AssetImporter {
private:
    mutable std::unique_ptr<AudioAnalyzer> analyzer_;
    mutable std::unique_ptr<AudioProcessor> processor_;
    
    // Performance tracking
    mutable std::atomic<u64> total_imports_{0};
    mutable std::atomic<f64> total_import_time_{0.0};
    mutable std::atomic<usize> total_samples_processed_{0};
    
public:
    AudioImporter();
    ~AudioImporter() = default;
    
    // AssetImporter interface
    std::vector<std::string> supported_extensions() const override;
    AssetType asset_type() const override { return AssetType::Audio; }
    bool can_import(const std::filesystem::path& file_path) const override;
    
    ImportResult import_asset(
        const std::filesystem::path& source_path,
        const ImportSettings& settings,
        memory::MemoryTracker* memory_tracker = nullptr
    ) const override;
    
    std::unique_ptr<ImportSettings> create_default_settings() const override;
    core::Result<void, const char*> validate_file(const std::filesystem::path& file_path) const override;
    std::string analyze_file(const std::filesystem::path& file_path) const override;
    
    // Educational interface
    std::string get_educational_description() const override;
    std::vector<std::string> get_learning_objectives() const override;
    
    // Advanced audio analysis
    AudioAnalysis analyze_audio_file(const std::filesystem::path& file_path) const;
    AudioAnalysis analyze_audio_data(const AudioData& data) const;
    
    // Import with detailed analysis
    ImportResult import_with_analysis(
        const std::filesystem::path& source_path,
        const AudioImportSettings& settings,
        bool generate_analysis = true,
        memory::MemoryTracker* memory_tracker = nullptr
    ) const;
    
    // Preview generation
    core::Result<AudioData, const char*> generate_preview(
        const std::filesystem::path& file_path,
        f64 duration_seconds = 10.0,
        f64 start_seconds = 0.0
    ) const;
    
    // Educational utilities
    std::string generate_audio_tutorial(const std::filesystem::path& file_path) const;
    std::string generate_compression_comparison(
        const std::filesystem::path& file_path,
        const std::vector<std::string>& codecs
    ) const;
    
    // Performance monitoring
    struct AudioImporterStatistics {
        u64 total_imports{0};
        f64 average_import_time{0.0};
        usize total_samples_processed{0};
        f64 average_processing_speed{0.0}; // Samples per second
        
        std::unordered_map<std::string, u32> format_counts;
        std::unordered_map<u32, u32> sample_rate_distribution;
        std::unordered_map<u16, u32> bit_depth_distribution;
        
        f32 average_quality_score{0.0f};
        u32 failed_imports{0};
        f64 success_rate{1.0};
    };
    
    AudioImporterStatistics get_statistics() const;
    void reset_statistics();
    
private:
    // Format-specific import functions
    core::Result<AudioData, const char*> import_wav(const std::filesystem::path& file_path) const;
    core::Result<AudioData, const char*> import_mp3(const std::filesystem::path& file_path) const;
    core::Result<AudioData, const char*> import_ogg(const std::filesystem::path& file_path) const;
    core::Result<AudioData, const char*> import_flac(const std::filesystem::path& file_path) const;
    
    // Processing pipeline
    ImportResult process_audio_data(
        AudioData audio_data,
        const AudioImportSettings& settings,
        const std::filesystem::path& source_path,
        memory::MemoryTracker* memory_tracker
    ) const;
    
    // Validation helpers
    bool validate_audio_data(const AudioData& data, std::vector<std::string>& issues) const;
    f32 calculate_audio_quality_score(const AudioData& data) const;
    
    // Educational content generation
    std::string generate_signal_processing_explanation(const AudioAnalysis& analysis) const;
    std::string generate_codec_comparison(const std::filesystem::path& file_path) const;
};

//=============================================================================
// Educational Audio Examples
//=============================================================================

/**
 * @brief Generate educational audio examples for teaching concepts
 */
class AudioEducationGenerator {
public:
    // Generate test signals
    static AudioData generate_sine_wave(f32 frequency, f64 duration, u32 sample_rate = 44100);
    static AudioData generate_white_noise(f64 duration, u32 sample_rate = 44100);
    static AudioData generate_pink_noise(f64 duration, u32 sample_rate = 44100);
    static AudioData generate_chirp(f32 start_freq, f32 end_freq, f64 duration, u32 sample_rate = 44100);
    
    // Generate complex examples
    static AudioData generate_chord(const std::vector<f32>& frequencies, f64 duration, u32 sample_rate = 44100);
    static AudioData generate_amplitude_modulation_example(f32 carrier, f32 modulator, f64 duration);
    static AudioData generate_frequency_modulation_example(f32 carrier, f32 modulator, f64 duration);
    
    // Educational demonstrations
    static AudioData demonstrate_aliasing(f32 frequency, u32 sample_rate);
    static AudioData demonstrate_quantization_noise(u16 bit_depth, f64 duration);
    static AudioData demonstrate_compression_artifacts(const AudioData& original, f32 compression_ratio);
    
    // Create exercise materials
    static std::vector<AudioData> create_hearing_test_suite();
    static std::vector<AudioData> create_frequency_training_set();
    static std::vector<AudioData> create_dynamic_range_examples();
};

} // namespace ecscope::assets::importers