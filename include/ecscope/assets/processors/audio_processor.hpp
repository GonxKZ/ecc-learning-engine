#pragma once

#include "asset_processor.hpp"
#include <complex>
#include <array>

namespace ecscope::assets::processors {

    // Audio formats
    enum class AudioFormat {
        UNKNOWN = 0,
        PCM_U8,
        PCM_S16,
        PCM_S24,
        PCM_S32,
        PCM_F32,
        PCM_F64,
        ADPCM,
        MP3,
        OGG_VORBIS,
        FLAC,
        AAC,
        OPUS,
        COUNT
    };

    // Audio channel layouts
    enum class ChannelLayout {
        MONO = 1,
        STEREO = 2,
        SURROUND_2_1 = 3,
        SURROUND_4_0 = 4,
        SURROUND_4_1 = 5,
        SURROUND_5_1 = 6,
        SURROUND_7_1 = 8
    };

    // Audio processing settings
    struct AudioProcessingSettings {
        // Format conversion
        AudioFormat target_format = AudioFormat::PCM_S16;
        int target_sample_rate = 44100;
        ChannelLayout target_channels = ChannelLayout::STEREO;
        
        // Quality settings
        int bitrate = 128000; // For compressed formats (bits/sec)
        float quality = 0.7f; // 0.0 to 1.0 for quality-based encoding
        
        // Processing options
        bool normalize_audio = true;
        float normalize_peak = 0.95f; // Peak normalization level
        bool apply_fade_in = false;
        bool apply_fade_out = false;
        float fade_duration = 0.1f; // seconds
        
        // Compression settings
        bool enable_compression = true;
        float compression_ratio = 4.0f; // For dynamic range compression
        float compression_threshold = -12.0f; // dB
        float compression_attack = 0.003f; // seconds
        float compression_release = 0.1f; // seconds
        
        // EQ settings
        bool apply_eq = false;
        float low_gain = 0.0f;   // dB
        float mid_gain = 0.0f;   // dB  
        float high_gain = 0.0f;  // dB
        float low_freq = 200.0f; // Hz
        float high_freq = 2000.0f; // Hz
    };

    // Audio metadata
    struct AudioMetadata {
        AudioFormat format = AudioFormat::UNKNOWN;
        int sample_rate = 0;
        int channels = 0;
        int bits_per_sample = 0;
        int bitrate = 0;
        float duration = 0.0f; // seconds
        size_t frame_count = 0;
        bool is_compressed = false;
        
        // Audio analysis
        float peak_amplitude = 0.0f;
        float rms_amplitude = 0.0f;
        float dynamic_range = 0.0f; // dB
        float silence_ratio = 0.0f; // 0.0 to 1.0
        
        // Frequency analysis
        float dominant_frequency = 0.0f; // Hz
        float frequency_centroid = 0.0f; // Hz
        std::vector<float> spectrum; // FFT magnitude spectrum
        
        // Musical properties (if detected)
        float tempo = 0.0f; // BPM
        std::string key; // Musical key
        bool is_music = false;
        bool is_speech = false;
        bool has_beats = false;
    };

    // Audio processor
    class AudioProcessor : public BaseAssetProcessor {
    public:
        AudioProcessor();
        
        // AssetProcessor implementation
        std::vector<std::string> get_supported_extensions() const override;
        bool can_process(const std::string& file_path, const AssetMetadata& metadata) const override;
        bool supports_streaming() const override { return true; }
        
        ProcessingResult process(const std::vector<uint8_t>& input_data,
                               const AssetMetadata& input_metadata,
                               const ProcessingOptions& options) override;
        
        bool validate_input(const std::vector<uint8_t>& input_data,
                           const AssetMetadata& metadata) const override;
        
        AssetMetadata extract_metadata(const std::vector<uint8_t>& data,
                                     const std::string& file_path) const override;
        
        size_t estimate_output_size(size_t input_size,
                                   const ProcessingOptions& options) const override;
        
        // Audio-specific operations
        ProcessingResult load_audio(const std::vector<uint8_t>& data,
                                  const std::string& file_path) const;
        
        ProcessingResult convert_format(const std::vector<float>& pcm_data,
                                      const AudioMetadata& metadata,
                                      const AudioProcessingSettings& settings) const;
        
        ProcessingResult resample_audio(const std::vector<float>& pcm_data,
                                      int source_rate, int target_rate,
                                      int channels) const;
        
        ProcessingResult convert_channels(const std::vector<float>& pcm_data,
                                        ChannelLayout source_layout,
                                        ChannelLayout target_layout,
                                        int sample_rate) const;
        
        ProcessingResult normalize_audio(const std::vector<float>& pcm_data,
                                       float target_peak = 0.95f) const;
        
        ProcessingResult apply_fade(const std::vector<float>& pcm_data,
                                  bool fade_in, bool fade_out,
                                  float fade_duration, int sample_rate) const;
        
        ProcessingResult compress_audio(const std::vector<float>& pcm_data,
                                      const AudioMetadata& metadata,
                                      AudioFormat target_format,
                                      float quality = 0.7f) const;
        
        // Audio analysis
        AudioMetadata analyze_audio(const std::vector<float>& pcm_data,
                                  int sample_rate, int channels) const;
        
        std::vector<float> compute_spectrum(const std::vector<float>& pcm_data,
                                          int sample_rate, int fft_size = 1024) const;
        
        float detect_tempo(const std::vector<float>& pcm_data,
                          int sample_rate) const;
        
        std::string detect_key(const std::vector<float>& pcm_data,
                              int sample_rate) const;
        
        bool detect_speech(const std::vector<float>& pcm_data,
                          int sample_rate) const;
        
        std::vector<float> detect_onsets(const std::vector<float>& pcm_data,
                                       int sample_rate) const;
        
        // Spatial audio processing
        ProcessingResult apply_3d_positioning(const std::vector<float>& mono_data,
                                             float x, float y, float z,
                                             float listener_x, float listener_y, float listener_z,
                                             int sample_rate) const;
        
        ProcessingResult apply_reverb(const std::vector<float>& pcm_data,
                                    float room_size, float damping, float wet_level,
                                    int sample_rate) const;
        
        ProcessingResult apply_hrtf(const std::vector<float>& mono_data,
                                  float azimuth, float elevation,
                                  int sample_rate) const;
        
        // Format utilities
        static const char* format_to_string(AudioFormat format);
        static AudioFormat string_to_format(const std::string& format_str);
        static bool is_compressed_format(AudioFormat format);
        static int get_format_byte_depth(AudioFormat format);
        static size_t calculate_audio_size(int sample_rate, int channels, 
                                         AudioFormat format, float duration);
        
        // Channel utilities
        static const char* channel_layout_to_string(ChannelLayout layout);
        static ChannelLayout string_to_channel_layout(const std::string& layout_str);
        static int get_channel_count(ChannelLayout layout);
        
    private:
        // Format loaders
        ProcessingResult load_wav(const std::vector<uint8_t>& data) const;
        ProcessingResult load_mp3(const std::vector<uint8_t>& data) const;
        ProcessingResult load_ogg(const std::vector<uint8_t>& data) const;
        ProcessingResult load_flac(const std::vector<uint8_t>& data) const;
        ProcessingResult load_aac(const std::vector<uint8_t>& data) const;
        
        // Format encoders
        std::vector<uint8_t> encode_wav(const std::vector<float>& pcm_data,
                                       int sample_rate, int channels,
                                       AudioFormat format) const;
        std::vector<uint8_t> encode_mp3(const std::vector<float>& pcm_data,
                                       int sample_rate, int channels,
                                       int bitrate) const;
        std::vector<uint8_t> encode_ogg(const std::vector<float>& pcm_data,
                                       int sample_rate, int channels,
                                       float quality) const;
        std::vector<uint8_t> encode_flac(const std::vector<float>& pcm_data,
                                        int sample_rate, int channels,
                                        int compression_level = 5) const;
        
        // DSP implementations
        std::vector<float> resample_impl(const std::vector<float>& input,
                                       int source_rate, int target_rate,
                                       int channels) const;
        
        std::vector<float> apply_lowpass_filter(const std::vector<float>& input,
                                              float cutoff_freq, int sample_rate) const;
        
        std::vector<float> apply_highpass_filter(const std::vector<float>& input,
                                               float cutoff_freq, int sample_rate) const;
        
        std::vector<float> apply_bandpass_filter(const std::vector<float>& input,
                                               float low_freq, float high_freq,
                                               int sample_rate) const;
        
        // Advanced DSP
        std::vector<std::complex<float>> compute_fft(const std::vector<float>& input,
                                                   int fft_size) const;
        std::vector<float> compute_ifft(const std::vector<std::complex<float>>& input) const;
        
        std::vector<float> apply_spectral_filter(const std::vector<float>& input,
                                               const std::function<float(float)>& filter_func,
                                               int sample_rate) const;
        
        // Channel conversion implementations
        std::vector<float> mono_to_stereo(const std::vector<float>& mono) const;
        std::vector<float> stereo_to_mono(const std::vector<float>& stereo) const;
        std::vector<float> stereo_to_surround(const std::vector<float>& stereo,
                                             ChannelLayout target_layout) const;
        
        // Dynamic range processing
        std::vector<float> apply_compressor(const std::vector<float>& input,
                                          float threshold, float ratio,
                                          float attack, float release,
                                          int sample_rate) const;
        
        std::vector<float> apply_limiter(const std::vector<float>& input,
                                       float threshold, int sample_rate) const;
        
        std::vector<float> apply_gate(const std::vector<float>& input,
                                    float threshold, float ratio,
                                    int sample_rate) const;
        
        // Analysis implementations
        float calculate_rms(const std::vector<float>& samples) const;
        float calculate_peak(const std::vector<float>& samples) const;
        float calculate_dynamic_range(const std::vector<float>& samples) const;
        float calculate_silence_ratio(const std::vector<float>& samples,
                                     float threshold = 0.01f) const;
        
        std::vector<float> compute_mel_spectrogram(const std::vector<float>& pcm_data,
                                                 int sample_rate, int n_mels = 128) const;
        
        std::vector<float> compute_mfcc(const std::vector<float>& pcm_data,
                                      int sample_rate, int n_coeffs = 13) const;
        
        // Tempo detection
        std::vector<float> onset_detection_function(const std::vector<float>& pcm_data,
                                                   int sample_rate) const;
        float autocorrelation_tempo(const std::vector<float>& odf,
                                   int sample_rate) const;
        
        // Key detection
        std::vector<float> chromagram(const std::vector<float>& pcm_data,
                                    int sample_rate) const;
        std::string classify_key(const std::vector<float>& chroma) const;
        
        // HRTF processing
        struct HRTFData;
        std::unique_ptr<HRTFData> m_hrtf_data;
        void load_hrtf_database();
        
        // Threading support
        class AudioProcessingThreadPool;
        std::unique_ptr<AudioProcessingThreadPool> m_thread_pool;
    };

    // Audio streaming support
    class StreamingAudioProcessor {
    public:
        StreamingAudioProcessor();
        ~StreamingAudioProcessor();
        
        // Streaming operations
        ProcessingResult start_streaming_decode(const std::vector<uint8_t>& data,
                                              const std::string& file_path);
        
        ProcessingResult read_samples(float* buffer, size_t sample_count);
        bool is_end_of_stream() const;
        void seek(float time_seconds);
        float get_position() const;
        float get_duration() const;
        
        void reset();
        
    private:
        struct StreamingState;
        std::unique_ptr<StreamingState> m_state;
    };

    // Audio utilities
    namespace audio_utils {
        // Sample conversion
        float int16_to_float(int16_t sample);
        int16_t float_to_int16(float sample);
        float int24_to_float(int32_t sample); // 24-bit stored in 32-bit
        int32_t float_to_int24(float sample);
        
        // Decibel conversions
        float linear_to_db(float linear);
        float db_to_linear(float db);
        
        // Window functions for FFT
        std::vector<float> hann_window(int size);
        std::vector<float> hamming_window(int size);
        std::vector<float> blackman_window(int size);
        
        // Pitch/frequency utilities
        float frequency_to_mel(float frequency);
        float mel_to_frequency(float mel);
        float frequency_to_bark(float frequency);
        float bark_to_frequency(float bark);
        
        // Musical utilities
        float note_to_frequency(const std::string& note); // e.g., "A4" -> 440 Hz
        std::string frequency_to_note(float frequency);
        int frequency_to_midi_note(float frequency);
        float midi_note_to_frequency(int midi_note);
        
        // Interpolation
        float linear_interpolate(float a, float b, float t);
        float cosine_interpolate(float a, float b, float t);
        float cubic_interpolate(float a, float b, float c, float d, float t);
    }

} // namespace ecscope::assets::processors