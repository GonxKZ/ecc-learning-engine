#pragma once

#include "../core/asset_types.hpp"
#include <vector>
#include <memory>
#include <complex>

namespace ecscope::assets {

// =============================================================================
// Audio Formats and Properties
// =============================================================================

enum class AudioFormat : uint8_t {
    Unknown = 0,
    PCM_U8,         // 8-bit unsigned PCM
    PCM_S16,        // 16-bit signed PCM
    PCM_S24,        // 24-bit signed PCM
    PCM_S32,        // 32-bit signed PCM
    PCM_F32,        // 32-bit float PCM
    PCM_F64,        // 64-bit float PCM
    
    // Compressed formats
    MP3,            // MPEG Audio Layer III
    OGG,            // Ogg Vorbis
    FLAC,           // Free Lossless Audio Codec
    AAC,            // Advanced Audio Codec
    WAV,            // Waveform Audio File Format
    OPUS,           // Opus codec
    
    // Special formats
    ADPCM,          // Adaptive Differential PCM
    IMA_ADPCM       // IMA ADPCM
};

enum class AudioChannelLayout : uint8_t {
    Mono = 1,
    Stereo = 2,
    Surround_5_1 = 6,
    Surround_7_1 = 8
};

// =============================================================================
// Audio Data
// =============================================================================

struct AudioData {
    AudioFormat format = AudioFormat::Unknown;
    uint32_t sample_rate = 44100;
    AudioChannelLayout channels = AudioChannelLayout::Stereo;
    uint32_t bits_per_sample = 16;
    uint64_t frame_count = 0;
    
    std::vector<uint8_t> data;
    
    // Streaming metadata
    uint64_t loop_start = 0;
    uint64_t loop_end = 0;
    bool looping = false;
    
    // Spatial audio properties
    bool is_3d = false;
    float min_distance = 1.0f;
    float max_distance = 100.0f;
    float rolloff_factor = 1.0f;
    
    // Quality metadata
    float dynamic_range = 0.0f;  // dB
    float peak_level = 0.0f;     // dB
    float rms_level = 0.0f;      // dB
    
    size_t getSampleSize() const;
    size_t getFrameSize() const;
    size_t getTotalSize() const;
    double getDuration() const; // in seconds
    
    bool isCompressed() const;
    bool isPCM() const;
};

// =============================================================================
// Audio Processing Options
// =============================================================================

struct AudioProcessingOptions {
    // Format conversion
    AudioFormat target_format = AudioFormat::Unknown; // Auto-detect
    uint32_t target_sample_rate = 0; // 0 = keep original
    AudioChannelLayout target_channels = AudioChannelLayout::Stereo;
    
    // Quality settings
    AssetQuality target_quality = AssetQuality::High;
    float compression_quality = 0.8f; // 0.0 = smallest, 1.0 = best quality
    
    // Processing effects
    bool normalize = false;
    float target_peak_db = -3.0f;
    float target_rms_db = -23.0f; // EBU R128 standard
    
    bool apply_fade_in = false;
    bool apply_fade_out = false;
    float fade_duration = 0.1f; // seconds
    
    bool remove_silence = false;
    float silence_threshold_db = -60.0f;
    
    // Spatial audio processing
    bool process_for_3d = false;
    bool generate_hrtf_data = false;
    
    // Loop processing
    bool auto_detect_loops = false;
    float loop_detection_threshold = 0.95f; // Correlation threshold
    
    // Platform variants
    bool generate_platform_variants = true;
    std::vector<AudioFormat> preferred_formats;
};

// =============================================================================
// Audio Asset
// =============================================================================

class AudioAsset : public Asset {
public:
    ASSET_TYPE(AudioAsset, 1003)
    
    AudioAsset();
    ~AudioAsset() override;
    
    // Asset interface
    AssetLoadResult load(const std::string& path, const AssetLoadParams& params = {}) override;
    void unload() override;
    bool isLoaded() const override { return audio_data_ != nullptr; }
    
    uint64_t getMemoryUsage() const override;
    
    // Streaming support
    bool supportsStreaming() const override { return true; }
    void streamIn(AssetQuality quality) override;
    void streamOut() override;
    
    // Audio data access
    const AudioData* getAudioData() const { return audio_data_.get(); }
    AudioData* getAudioData() { return audio_data_.get(); }
    
    // Audio properties
    double getDuration() const { return audio_data_ ? audio_data_->getDuration() : 0.0; }
    uint32_t getSampleRate() const { return audio_data_ ? audio_data_->sample_rate : 0; }
    AudioChannelLayout getChannels() const { return audio_data_ ? audio_data_->channels : AudioChannelLayout::Mono; }
    
    // Playback control
    void setLooping(bool looping) { if (audio_data_) audio_data_->looping = looping; }
    bool isLooping() const { return audio_data_ ? audio_data_->looping : false; }
    
private:
    std::unique_ptr<AudioData> audio_data_;
    AssetQuality current_quality_ = AssetQuality::High;
};

// =============================================================================
// Audio Processor
// =============================================================================

class AudioProcessor {
public:
    AudioProcessor();
    ~AudioProcessor();
    
    // Main processing interface
    std::unique_ptr<AudioData> processAudio(const std::string& input_path,
                                           const AudioProcessingOptions& options = {});
    
    std::unique_ptr<AudioData> processAudioFromMemory(const uint8_t* data, size_t size,
                                                     const AudioProcessingOptions& options = {});
    
    // Individual processing steps
    std::unique_ptr<AudioData> loadFromFile(const std::string& path);
    std::unique_ptr<AudioData> loadFromMemory(const uint8_t* data, size_t size);
    
    bool convertFormat(AudioData& audio, AudioFormat new_format);
    bool resample(AudioData& audio, uint32_t new_sample_rate);
    bool convertChannels(AudioData& audio, AudioChannelLayout new_layout);
    
    // Audio effects
    bool normalize(AudioData& audio, float target_peak_db = -3.0f, float target_rms_db = -23.0f);
    bool applyFade(AudioData& audio, float fade_in_duration, float fade_out_duration);
    bool removeSilence(AudioData& audio, float threshold_db = -60.0f);
    
    // Analysis
    float calculatePeakLevel(const AudioData& audio);
    float calculateRMSLevel(const AudioData& audio);
    float calculateDynamicRange(const AudioData& audio);
    
    // Loop detection and processing
    std::pair<uint64_t, uint64_t> detectLoopPoints(const AudioData& audio, float threshold = 0.95f);
    bool validateLoop(const AudioData& audio, uint64_t start, uint64_t end);
    
    // Spatial audio processing
    bool processFor3D(AudioData& audio, float min_distance = 1.0f, float max_distance = 100.0f);
    
    // Compression
    bool compressAudio(AudioData& audio, AudioFormat compression_format, float quality = 0.8f);
    
    // Quality level processing
    std::unique_ptr<AudioData> processForQuality(const AudioData& source, AssetQuality quality);
    
    // Platform variants
    std::vector<std::unique_ptr<AudioData>> generatePlatformVariants(const AudioData& source);
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// =============================================================================
// Audio Codec Interface
// =============================================================================

class AudioCodec {
public:
    virtual ~AudioCodec() = default;
    
    virtual bool canDecode(AudioFormat format) const = 0;
    virtual bool canEncode(AudioFormat format) const = 0;
    
    virtual std::unique_ptr<AudioData> decode(const uint8_t* data, size_t size) = 0;
    virtual std::vector<uint8_t> encode(const AudioData& audio, float quality = 0.8f) = 0;
    
    virtual AudioFormat getFormat() const = 0;
    virtual std::string getName() const = 0;
    virtual std::vector<std::string> getFileExtensions() const = 0;
};

// =============================================================================
// Specific Audio Codecs
// =============================================================================

class WAVCodec : public AudioCodec {
public:
    bool canDecode(AudioFormat format) const override;
    bool canEncode(AudioFormat format) const override;
    
    std::unique_ptr<AudioData> decode(const uint8_t* data, size_t size) override;
    std::vector<uint8_t> encode(const AudioData& audio, float quality = 0.8f) override;
    
    AudioFormat getFormat() const override { return AudioFormat::WAV; }
    std::string getName() const override { return "WAV Codec"; }
    std::vector<std::string> getFileExtensions() const override { return {".wav"}; }
};

class OGGCodec : public AudioCodec {
public:
    bool canDecode(AudioFormat format) const override;
    bool canEncode(AudioFormat format) const override;
    
    std::unique_ptr<AudioData> decode(const uint8_t* data, size_t size) override;
    std::vector<uint8_t> encode(const AudioData& audio, float quality = 0.8f) override;
    
    AudioFormat getFormat() const override { return AudioFormat::OGG; }
    std::string getName() const override { return "OGG Vorbis Codec"; }
    std::vector<std::string> getFileExtensions() const override { return {".ogg"}; }
};

class FLACCodec : public AudioCodec {
public:
    bool canDecode(AudioFormat format) const override;
    bool canEncode(AudioFormat format) const override;
    
    std::unique_ptr<AudioData> decode(const uint8_t* data, size_t size) override;
    std::vector<uint8_t> encode(const AudioData& audio, float quality = 0.8f) override;
    
    AudioFormat getFormat() const override { return AudioFormat::FLAC; }
    std::string getName() const override { return "FLAC Codec"; }
    std::vector<std::string> getFileExtensions() const override { return {".flac"}; }
};

// =============================================================================
// Audio Analysis Tools
// =============================================================================

class AudioAnalyzer {
public:
    struct FrequencySpectrum {
        std::vector<float> frequencies;
        std::vector<float> magnitudes;
        float sample_rate;
    };
    
    struct AudioFeatures {
        float zero_crossing_rate;
        float spectral_centroid;
        float spectral_rolloff;
        float mfcc[13]; // Mel-frequency cepstral coefficients
        float tempo; // BPM
        bool is_speech;
        bool is_music;
    };
    
    static FrequencySpectrum computeFFT(const AudioData& audio, size_t window_size = 1024);
    static AudioFeatures extractFeatures(const AudioData& audio);
    
    static float computeCorrelation(const AudioData& audio1, const AudioData& audio2);
    static std::vector<float> computeMelSpectrum(const FrequencySpectrum& spectrum, int num_mels = 13);
    
    // Beat detection
    static std::vector<double> detectBeats(const AudioData& audio);
    static float estimateTempo(const AudioData& audio);
    
    // Quality assessment
    static float computeSNR(const AudioData& audio); // Signal-to-noise ratio
    static float computeTHD(const AudioData& audio); // Total harmonic distortion
};

// =============================================================================
// Spatial Audio Processing
// =============================================================================

class SpatialAudioProcessor {
public:
    struct HRTFData {
        std::vector<float> left_impulse_response;
        std::vector<float> right_impulse_response;
        float azimuth; // degrees
        float elevation; // degrees
        float distance; // meters
    };
    
    // HRTF processing
    static std::vector<HRTFData> generateHRTFDatabase();
    static std::unique_ptr<AudioData> applyHRTF(const AudioData& source, const HRTFData& hrtf);
    
    // Spatial positioning
    struct Position3D {
        float x, y, z;
    };
    
    static std::unique_ptr<AudioData> processForPosition(const AudioData& source, 
                                                        const Position3D& sound_pos,
                                                        const Position3D& listener_pos,
                                                        const Position3D& listener_forward);
    
    // Room simulation
    struct RoomParameters {
        float width, height, depth;
        float absorption; // 0.0 = reflective, 1.0 = absorptive
        float reverb_time; // seconds
    };
    
    static std::unique_ptr<AudioData> applyRoomSimulation(const AudioData& source, 
                                                         const RoomParameters& room);
};

// =============================================================================
// Audio Registry
// =============================================================================

class AudioRegistry {
public:
    static AudioRegistry& instance();
    
    void registerCodec(std::unique_ptr<AudioCodec> codec);
    AudioCodec* getCodecByFormat(AudioFormat format) const;
    AudioCodec* getCodecByExtension(const std::string& extension) const;
    std::vector<AudioCodec*> getCodecs() const;
    
    std::vector<AudioFormat> getSupportedFormats() const;
    std::vector<std::string> getSupportedExtensions() const;
    bool supportsFormat(AudioFormat format) const;
    bool supportsExtension(const std::string& extension) const;
    
private:
    std::vector<std::unique_ptr<AudioCodec>> codecs_;
    std::unordered_map<AudioFormat, AudioCodec*> format_to_codec_;
    std::unordered_map<std::string, AudioCodec*> extension_to_codec_;
};

} // namespace ecscope::assets