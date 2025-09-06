#pragma once

/**
 * @file audio_processing_pipeline.hpp  
 * @brief Real-time Audio Processing Pipeline for ECScope Spatial Audio System
 * 
 * This comprehensive audio processing pipeline provides high-performance,
 * low-latency audio processing with SIMD optimization and educational features.
 * Designed for real-time spatial audio applications with extensive learning opportunities.
 * 
 * Key Features:
 * - Sub-10ms latency audio processing pipeline
 * - SIMD-optimized DSP operations (SSE2, AVX2)
 * - Dynamic range compression and limiting
 * - Real-time audio analysis and visualization  
 * - Memory pool management for zero-allocation processing
 * - Adaptive quality scaling for performance
 * - Educational DSP algorithm demonstrations
 * 
 * Educational Value:
 * - Demonstrates real-time audio system architecture
 * - Shows SIMD optimization techniques for audio
 * - Illustrates low-latency processing requirements
 * - Provides insight into professional audio pipelines
 * - Teaches about audio quality vs performance trade-offs
 * - Interactive DSP algorithm visualization
 * 
 * Performance Characteristics:
 * - Lock-free audio processing thread
 * - SIMD-vectorized operations for critical paths
 * - Cache-friendly data structures
 * - Minimal memory allocations during processing
 * - Adaptive quality based on CPU load
 * - Educational analysis without performance impact
 * 
 * @author ECScope Educational ECS Framework - Audio Processing Pipeline
 * @date 2025
 */

#include "spatial_audio_engine.hpp"
#include "audio_components.hpp"
#include "core/types.hpp"
#include "memory/memory_tracker.hpp"
#include "memory/arena.hpp"
#include <vector>
#include <array>
#include <atomic>
#include <memory>
#include <thread>
#include <chrono>
#include <functional>

// Platform-specific SIMD includes
#if defined(__AVX2__)
    #include <immintrin.h>
    #define AUDIO_SIMD_AVX2
#elif defined(__SSE2__)  
    #include <emmintrin.h>
    #include <xmmintrin.h>
    #define AUDIO_SIMD_SSE2
#endif

namespace ecscope::audio::processing {

// Import commonly used types
using namespace ecscope::audio;
using namespace ecscope::audio::components;
using namespace ecscope::memory;

//=============================================================================
// Processing Pipeline Configuration
//=============================================================================

/**
 * @brief Audio processing pipeline configuration
 */
struct PipelineConfig {
    // Core audio parameters
    u32 sample_rate{48000};                 ///< Sample rate (Hz)
    u32 buffer_size{512};                   ///< Buffer size (samples)
    u16 channels{2};                        ///< Number of output channels
    f32 target_latency_ms{10.0f};          ///< Target latency (milliseconds)
    
    // Processing quality
    enum class QualityLevel : u8 {
        Draft = 0,      ///< Draft quality (fastest, educational only)
        Low,            ///< Low quality (reduced precision)
        Medium,         ///< Medium quality (standard processing)
        High,           ///< High quality (enhanced processing) 
        Ultra           ///< Ultra quality (maximum quality)
    } quality_level{QualityLevel::High};
    
    // SIMD optimization settings
    enum class SIMDLevel : u8 {
        Disabled = 0,   ///< Disable SIMD (for comparison/debugging)
        Auto,           ///< Auto-detect best SIMD level
        SSE2,           ///< Force SSE2 only
        AVX2            ///< Force AVX2 only
    } simd_level{SIMDLevel::Auto};
    
    // Memory management
    usize memory_pool_size{64 * 1024 * 1024}; ///< 64MB memory pool
    u32 max_audio_sources{256};             ///< Maximum simultaneous sources
    u32 max_listeners{4};                   ///< Maximum listeners
    bool use_memory_pools{true};            ///< Use memory pools for allocations
    
    // Performance optimization
    bool enable_culling{true};              ///< Enable performance-based culling
    f32 culling_threshold{0.001f};          ///< Volume threshold for culling
    f32 cpu_load_threshold{80.0f};          ///< CPU load threshold for quality reduction
    bool adaptive_quality{true};            ///< Enable adaptive quality scaling
    
    // Educational features
    bool enable_analysis{true};             ///< Enable real-time analysis
    bool enable_visualization{true};        ///< Enable visualization data generation
    u32 analysis_window_size{1024};         ///< FFT window size for analysis
    f32 analysis_update_rate{30.0f};        ///< Analysis update rate (Hz)
    
    // Threading configuration
    enum class ThreadingMode : u8 {
        SingleThread = 0,   ///< Single-threaded processing
        AudioThread,        ///< Dedicated audio thread
        ThreadPool          ///< Thread pool for parallel processing
    } threading_mode{ThreadingMode::AudioThread};
    
    u32 num_worker_threads{0};              ///< Number of worker threads (0 = auto)
    bool realtime_priority{true};           ///< Use real-time thread priority
};

//=============================================================================
// SIMD-Optimized Audio Processing Utilities
//=============================================================================

/**
 * @brief High-performance SIMD audio processing functions
 * 
 * Educational Context:
 * These functions demonstrate how SIMD instructions can dramatically
 * improve audio processing performance. Each function includes both
 * SIMD and scalar versions for educational comparison.
 */
namespace simd_ops {
    
#ifdef AUDIO_SIMD_AVX2
    /**
     * @brief AVX2-optimized audio mixing (8 samples at once)
     * 
     * Educational Note: AVX2 allows processing 8 single-precision
     * floats simultaneously, providing ~8x speedup for compatible operations.
     */
    inline void mix_audio_avx2(const f32* input1, const f32* input2, f32* output, 
                               f32 gain1, f32 gain2, usize count) noexcept {
        const __m256 vgain1 = _mm256_set1_ps(gain1);
        const __m256 vgain2 = _mm256_set1_ps(gain2);
        
        usize simd_count = count & ~7; // Process in groups of 8
        for (usize i = 0; i < simd_count; i += 8) {
            __m256 vin1 = _mm256_loadu_ps(&input1[i]);
            __m256 vin2 = _mm256_loadu_ps(&input2[i]);
            
            vin1 = _mm256_mul_ps(vin1, vgain1);
            vin2 = _mm256_mul_ps(vin2, vgain2);
            
            __m256 vout = _mm256_add_ps(vin1, vin2);
            _mm256_storeu_ps(&output[i], vout);
        }
        
        // Process remaining samples
        for (usize i = simd_count; i < count; ++i) {
            output[i] = input1[i] * gain1 + input2[i] * gain2;
        }
    }
    
    /**
     * @brief AVX2-optimized volume scaling
     */
    inline void apply_volume_avx2(f32* audio, f32 volume, usize count) noexcept {
        const __m256 vvolume = _mm256_set1_ps(volume);
        
        usize simd_count = count & ~7;
        for (usize i = 0; i < simd_count; i += 8) {
            __m256 vaudio = _mm256_loadu_ps(&audio[i]);
            vaudio = _mm256_mul_ps(vaudio, vvolume);
            _mm256_storeu_ps(&audio[i], vaudio);
        }
        
        // Process remaining samples
        for (usize i = simd_count; i < count; ++i) {
            audio[i] *= volume;
        }
    }
    
    /**
     * @brief AVX2-optimized convolution (for HRTF processing)
     */
    void convolve_avx2(const f32* signal, const f32* kernel, f32* output,
                       usize signal_len, usize kernel_len) noexcept;
    
    /**
     * @brief AVX2-optimized RMS level calculation
     */
    inline f32 calculate_rms_avx2(const f32* audio, usize count) noexcept {
        __m256 sum = _mm256_setzero_ps();
        
        usize simd_count = count & ~7;
        for (usize i = 0; i < simd_count; i += 8) {
            __m256 vaudio = _mm256_loadu_ps(&audio[i]);
            vaudio = _mm256_mul_ps(vaudio, vaudio);
            sum = _mm256_add_ps(sum, vaudio);
        }
        
        // Horizontal sum of AVX register
        __m128 sum_high = _mm256_extractf128_ps(sum, 1);
        __m128 sum_low = _mm256_castps256_ps128(sum);
        sum_low = _mm_add_ps(sum_low, sum_high);
        
        sum_low = _mm_hadd_ps(sum_low, sum_low);
        sum_low = _mm_hadd_ps(sum_low, sum_low);
        
        f32 total = _mm_cvtss_f32(sum_low);
        
        // Process remaining samples
        for (usize i = simd_count; i < count; ++i) {
            total += audio[i] * audio[i];
        }
        
        return std::sqrt(total / static_cast<f32>(count));
    }
    
#endif // AUDIO_SIMD_AVX2
    
#ifdef AUDIO_SIMD_SSE2
    /**
     * @brief SSE2-optimized audio mixing (4 samples at once)
     */
    inline void mix_audio_sse2(const f32* input1, const f32* input2, f32* output,
                               f32 gain1, f32 gain2, usize count) noexcept {
        const __m128 vgain1 = _mm_set1_ps(gain1);
        const __m128 vgain2 = _mm_set1_ps(gain2);
        
        usize simd_count = count & ~3; // Process in groups of 4
        for (usize i = 0; i < simd_count; i += 4) {
            __m128 vin1 = _mm_loadu_ps(&input1[i]);
            __m128 vin2 = _mm_loadu_ps(&input2[i]);
            
            vin1 = _mm_mul_ps(vin1, vgain1);
            vin2 = _mm_mul_ps(vin2, vgain2);
            
            __m128 vout = _mm_add_ps(vin1, vin2);
            _mm_storeu_ps(&output[i], vout);
        }
        
        // Process remaining samples
        for (usize i = simd_count; i < count; ++i) {
            output[i] = input1[i] * gain1 + input2[i] * gain2;
        }
    }
    
    /**
     * @brief SSE2-optimized volume scaling
     */
    inline void apply_volume_sse2(f32* audio, f32 volume, usize count) noexcept {
        const __m128 vvolume = _mm_set1_ps(volume);
        
        usize simd_count = count & ~3;
        for (usize i = 0; i < simd_count; i += 4) {
            __m128 vaudio = _mm_loadu_ps(&audio[i]);
            vaudio = _mm_mul_ps(vaudio, vvolume);
            _mm_storeu_ps(&audio[i], vaudio);
        }
        
        // Process remaining samples
        for (usize i = simd_count; i < count; ++i) {
            audio[i] *= volume;
        }
    }
    
    /**
     * @brief SSE2-optimized convolution
     */
    void convolve_sse2(const f32* signal, const f32* kernel, f32* output,
                       usize signal_len, usize kernel_len) noexcept;
    
    /**
     * @brief SSE2-optimized RMS calculation  
     */
    inline f32 calculate_rms_sse2(const f32* audio, usize count) noexcept {
        __m128 sum = _mm_setzero_ps();
        
        usize simd_count = count & ~3;
        for (usize i = 0; i < simd_count; i += 4) {
            __m128 vaudio = _mm_loadu_ps(&audio[i]);
            vaudio = _mm_mul_ps(vaudio, vaudio);
            sum = _mm_add_ps(sum, vaudio);
        }
        
        // Horizontal sum
        sum = _mm_hadd_ps(sum, sum);
        sum = _mm_hadd_ps(sum, sum);
        f32 total = _mm_cvtss_f32(sum);
        
        // Process remaining samples
        for (usize i = simd_count; i < count; ++i) {
            total += audio[i] * audio[i];
        }
        
        return std::sqrt(total / static_cast<f32>(count));
    }
    
#endif // AUDIO_SIMD_SSE2
    
    // Scalar fallback implementations
    void mix_audio_scalar(const f32* input1, const f32* input2, f32* output,
                          f32 gain1, f32 gain2, usize count) noexcept;
    void apply_volume_scalar(f32* audio, f32 volume, usize count) noexcept;
    void convolve_scalar(const f32* signal, const f32* kernel, f32* output,
                        usize signal_len, usize kernel_len) noexcept;
    f32 calculate_rms_scalar(const f32* audio, usize count) noexcept;
    
    // Adaptive SIMD dispatch functions
    class SIMDDispatcher {
    private:
        PipelineConfig::SIMDLevel detected_level_;
        
    public:
        SIMDDispatcher();
        
        void mix_audio(const f32* input1, const f32* input2, f32* output,
                      f32 gain1, f32 gain2, usize count) const noexcept;
        void apply_volume(f32* audio, f32 volume, usize count) const noexcept;
        void convolve(const f32* signal, const f32* kernel, f32* output,
                     usize signal_len, usize kernel_len) const noexcept;
        f32 calculate_rms(const f32* audio, usize count) const noexcept;
        
        PipelineConfig::SIMDLevel get_simd_level() const noexcept { return detected_level_; }
        const char* get_simd_level_name() const noexcept;
        f32 get_performance_multiplier() const noexcept; // Speed improvement vs scalar
        
        // Educational benchmarking
        struct SIMDBenchmark {
            f32 scalar_time_ms;
            f32 sse2_time_ms;
            f32 avx2_time_ms;
            f32 sse2_speedup;
            f32 avx2_speedup;
            std::string fastest_method;
            std::string educational_summary;
        };
        
        SIMDBenchmark benchmark_performance(usize buffer_size, usize iterations = 1000) const;
    };
}

//=============================================================================
// Real-time Audio Buffer Management
//=============================================================================

/**
 * @brief Lock-free audio buffer for real-time processing
 * 
 * Educational Context:
 * Real-time audio requires lock-free data structures to avoid
 * priority inversion and timing issues. This ring buffer provides
 * guaranteed constant-time operations.
 */
template<typename T, usize Capacity>
class LockFreeRingBuffer {
private:
    alignas(64) std::array<T, Capacity> buffer_;
    alignas(64) std::atomic<usize> write_pos_{0};
    alignas(64) std::atomic<usize> read_pos_{0};
    
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");
    static constexpr usize MASK = Capacity - 1;
    
public:
    constexpr LockFreeRingBuffer() noexcept = default;
    
    /** @brief Write data to buffer (returns false if buffer full) */
    bool write(const T* data, usize count) noexcept {
        const usize current_write = write_pos_.load(std::memory_order_relaxed);
        const usize current_read = read_pos_.load(std::memory_order_acquire);
        
        const usize available = Capacity - ((current_write - current_read) & MASK);
        if (count > available) return false;
        
        // Copy data in two parts if wrapping
        const usize first_chunk = std::min(count, Capacity - (current_write & MASK));
        std::memcpy(&buffer_[current_write & MASK], data, first_chunk * sizeof(T));
        
        if (count > first_chunk) {
            std::memcpy(&buffer_[0], &data[first_chunk], (count - first_chunk) * sizeof(T));
        }
        
        write_pos_.store((current_write + count) & MASK, std::memory_order_release);
        return true;
    }
    
    /** @brief Read data from buffer (returns actual count read) */
    usize read(T* data, usize max_count) noexcept {
        const usize current_write = write_pos_.load(std::memory_order_acquire);
        const usize current_read = read_pos_.load(std::memory_order_relaxed);
        
        const usize available = (current_write - current_read) & MASK;
        const usize count = std::min(max_count, available);
        
        if (count == 0) return 0;
        
        // Copy data in two parts if wrapping
        const usize first_chunk = std::min(count, Capacity - (current_read & MASK));
        std::memcpy(data, &buffer_[current_read & MASK], first_chunk * sizeof(T));
        
        if (count > first_chunk) {
            std::memcpy(&data[first_chunk], &buffer_[0], (count - first_chunk) * sizeof(T));
        }
        
        read_pos_.store((current_read + count) & MASK, std::memory_order_release);
        return count;
    }
    
    /** @brief Get available data count */
    usize available_read() const noexcept {
        return (write_pos_.load(std::memory_order_acquire) - 
                read_pos_.load(std::memory_order_acquire)) & MASK;
    }
    
    /** @brief Get available write space */
    usize available_write() const noexcept {
        return Capacity - available_read();
    }
    
    /** @brief Check if buffer is empty */
    bool empty() const noexcept {
        return available_read() == 0;
    }
    
    /** @brief Check if buffer is full */
    bool full() const noexcept {
        return available_write() == 0;
    }
    
    /** @brief Clear buffer */
    void clear() noexcept {
        read_pos_.store(write_pos_.load(std::memory_order_relaxed), std::memory_order_relaxed);
    }
};

/**
 * @brief Memory-pool-backed audio buffer
 * 
 * Educational Context:
 * Audio processing requires predictable memory allocation patterns.
 * Memory pools eliminate allocation overhead and fragmentation.
 */
class AudioBufferPool {
private:
    std::unique_ptr<Arena> arena_;
    struct BufferHeader {
        usize size;
        usize alignment;
        BufferHeader* next;
    };
    
    BufferHeader* free_list_{nullptr};
    std::atomic<usize> allocated_bytes_{0};
    std::atomic<u32> allocation_count_{0};
    
public:
    explicit AudioBufferPool(usize pool_size);
    ~AudioBufferPool();
    
    /** @brief Allocate aligned audio buffer */
    f32* allocate_buffer(usize sample_count, usize alignment = 32);
    
    /** @brief Deallocate audio buffer */
    void deallocate_buffer(f32* buffer);
    
    /** @brief Allocate stereo buffer */
    AudioFrame* allocate_stereo_buffer(usize frame_count, usize alignment = 32);
    
    /** @brief Deallocate stereo buffer */
    void deallocate_stereo_buffer(AudioFrame* buffer);
    
    /** @brief Get pool statistics */
    struct PoolStats {
        usize total_size;
        usize allocated_bytes;
        usize available_bytes;
        u32 allocation_count;
        f32 fragmentation_ratio;
    };
    
    PoolStats get_stats() const;
    
    /** @brief Reset pool (free all allocations) */
    void reset();
};

//=============================================================================
// Dynamic Range Processing
//=============================================================================

/**
 * @brief Real-time dynamic range compressor
 * 
 * Educational Context:
 * Dynamic range compression is essential for maintaining
 * consistent audio levels. This implementation demonstrates
 * professional-grade audio processing techniques.
 */
class DynamicRangeProcessor {
public:
    /** @brief Compressor parameters */
    struct CompressorParams {
        f32 threshold{0.7f};            ///< Threshold (0-1)
        f32 ratio{4.0f};                ///< Compression ratio (1-inf)
        f32 attack_ms{5.0f};            ///< Attack time (milliseconds)
        f32 release_ms{50.0f};          ///< Release time (milliseconds)
        f32 knee_width_db{2.0f};        ///< Knee width (dB)
        f32 makeup_gain_db{0.0f};       ///< Makeup gain (dB)
        bool enabled{false};            ///< Enable compression
        
        // Educational parameters
        enum class Algorithm : u8 {
            FeedForward = 0,    ///< Feed-forward topology
            FeedBack,           ///< Feed-back topology
            Multiband           ///< Multi-band compression
        } algorithm{Algorithm::FeedForward};
    };
    
    /** @brief Limiter parameters */
    struct LimiterParams {
        f32 threshold{0.95f};           ///< Limiting threshold (0-1)
        f32 release_ms{10.0f};          ///< Release time (milliseconds)
        f32 lookahead_ms{1.0f};         ///< Lookahead time (milliseconds)
        bool enabled{true};             ///< Enable limiting
        
        enum class Type : u8 {
            Peak = 0,           ///< Peak limiter
            RMS,                ///< RMS limiter
            Hybrid              ///< Hybrid peak/RMS limiter
        } limiter_type{Type::Peak};
    };
    
    /** @brief Gate/expander parameters */
    struct GateParams {
        f32 threshold{0.01f};           ///< Gate threshold (0-1)
        f32 ratio{10.0f};               ///< Expansion ratio
        f32 attack_ms{1.0f};            ///< Attack time (milliseconds)
        f32 release_ms{100.0f};         ///< Release time (milliseconds)
        bool enabled{false};            ///< Enable gating
    };
    
private:
    // Processing state
    struct ProcessorState {
        f32 envelope_follower{0.0f};    ///< Current envelope level
        f32 gain_reduction{1.0f};       ///< Current gain reduction
        f32 peak_detector{0.0f};        ///< Peak detection state
        f32 rms_detector{0.0f};         ///< RMS detection state
        
        // Attack/release filters
        f32 attack_coeff{0.0f};
        f32 release_coeff{0.0f};
        
        // Lookahead delay line (for limiter)
        std::array<f32, 256> lookahead_buffer{};
        usize lookahead_index{0};
        
    } state_;
    
    CompressorParams compressor_params_;
    LimiterParams limiter_params_;
    GateParams gate_params_;
    
    u32 sample_rate_;
    simd_ops::SIMDDispatcher simd_dispatcher_;
    
public:
    explicit DynamicRangeProcessor(u32 sample_rate);
    
    /** @brief Set compressor parameters */
    void set_compressor_params(const CompressorParams& params);
    
    /** @brief Set limiter parameters */
    void set_limiter_params(const LimiterParams& params);
    
    /** @brief Set gate parameters */
    void set_gate_params(const GateParams& params);
    
    /** @brief Process audio buffer */
    void process_buffer(f32* audio, usize sample_count);
    void process_stereo_buffer(AudioFrame* audio, usize frame_count);
    
    /** @brief Process with SIMD optimization */
    void process_buffer_simd(f32* audio, usize sample_count);
    void process_stereo_buffer_simd(AudioFrame* audio, usize frame_count);
    
    /** @brief Get current processing metrics */
    struct ProcessingMetrics {
        f32 input_level_db;             ///< Input level (dB)
        f32 output_level_db;            ///< Output level (dB)
        f32 gain_reduction_db;          ///< Current gain reduction (dB)
        f32 envelope_level;             ///< Current envelope level
        bool compressor_active;         ///< Whether compressor is active
        bool limiter_active;            ///< Whether limiter is active
        bool gate_active;               ///< Whether gate is active
        
        // Educational information
        std::string processing_description; ///< What processing is being applied
        f32 dynamic_range_db;           ///< Current dynamic range
        f32 crest_factor_db;            ///< Crest factor (peak to RMS ratio)
    };
    
    ProcessingMetrics get_processing_metrics() const;
    
    /** @brief Generate processing visualization data */
    struct VisualizationData {
        std::vector<f32> input_waveform;
        std::vector<f32> output_waveform;
        std::vector<f32> gain_reduction_curve;
        std::vector<f32> compression_curve; // Input vs output levels
        
        std::string algorithm_explanation;
        std::vector<std::string> processing_stages;
    };
    
    VisualizationData generate_visualization_data(const f32* input_buffer, usize count) const;
    
private:
    void update_coefficients();
    f32 calculate_gain_reduction(f32 input_level) const;
    f32 apply_knee_function(f32 input_db, f32 threshold_db, f32 knee_width_db) const;
};

//=============================================================================
// Real-time Audio Analysis
//=============================================================================

/**
 * @brief Real-time audio analyzer with educational features
 * 
 * Educational Context:
 * Real-time audio analysis provides immediate feedback about
 * audio characteristics, enabling interactive learning about
 * signal processing concepts.
 */
class RealtimeAudioAnalyzer {
public:
    /** @brief Analysis configuration */
    struct AnalysisConfig {
        u32 fft_size{1024};             ///< FFT size for frequency analysis
        f32 overlap_factor{0.75f};      ///< Window overlap (0-1)
        f32 update_rate_hz{30.0f};      ///< Analysis update rate
        bool enable_spectrogram{true};  ///< Enable spectrogram generation
        bool enable_phase_analysis{false}; ///< Enable phase analysis (CPU intensive)
        
        enum class WindowFunction : u8 {
            Rectangular = 0,
            Hann,
            Hamming,
            Blackman,
            Kaiser
        } window_function{WindowFunction::Hann};
        
        // Educational features
        bool calculate_psychoacoustic_metrics{true}; ///< Calculate psychoacoustic measures
        bool detect_musical_features{true};          ///< Detect pitch, harmony, etc.
        bool analyze_spatial_properties{true};       ///< Analyze stereo/spatial properties
    };
    
    /** @brief Real-time analysis results */
    struct AnalysisResults {
        // Time domain analysis
        f32 rms_level_db{-60.0f};          ///< RMS level (dB)
        f32 peak_level_db{-60.0f};         ///< Peak level (dB)
        f32 crest_factor_db{0.0f};         ///< Crest factor (dB)
        f32 dynamic_range_db{0.0f};        ///< Dynamic range (dB)
        f32 zero_crossing_rate{0.0f};      ///< Zero crossing rate
        
        // Frequency domain analysis
        std::vector<f32> magnitude_spectrum; ///< Magnitude spectrum (dB)
        std::vector<f32> phase_spectrum;     ///< Phase spectrum (radians)
        std::vector<f32> frequency_bins;     ///< Frequency bin centers (Hz)
        f32 spectral_centroid_hz{1000.0f};  ///< Spectral centroid (Hz)
        f32 spectral_rolloff_hz{5000.0f};   ///< Spectral rolloff (Hz)
        f32 spectral_flux{0.0f};            ///< Spectral flux
        f32 spectral_flatness{0.0f};        ///< Spectral flatness (0-1)
        
        // Musical analysis
        f32 fundamental_frequency_hz{0.0f}; ///< Detected pitch (Hz)
        f32 pitch_confidence{0.0f};         ///< Pitch detection confidence (0-1)
        std::vector<f32> harmonic_frequencies; ///< Detected harmonics (Hz)
        f32 harmonic_to_noise_ratio_db{0.0f}; ///< Harmonic-to-noise ratio (dB)
        f32 inharmonicity{0.0f};            ///< Inharmonicity measure
        
        // Psychoacoustic analysis
        f32 loudness_phons{0.0f};           ///< Perceived loudness (phons)
        f32 sharpness_acum{0.0f};           ///< Sharpness (acum)
        f32 roughness_asper{0.0f};          ///< Roughness (asper)
        f32 fluctuation_strength{0.0f};     ///< Fluctuation strength
        
        // Spatial analysis (for stereo input)
        f32 stereo_width{1.0f};             ///< Stereo width (0-2)
        f32 left_right_correlation{0.0f};   ///< L-R correlation (-1 to 1)
        f32 mid_side_balance{0.0f};         ///< M-S balance
        f32 phase_coherence{1.0f};          ///< Phase coherence (0-1)
        
        // Spectrogram data (for visualization)
        std::vector<std::vector<f32>> spectrogram; ///< 2D spectrogram data
        
        // Educational insights
        struct EducationalInsights {
            std::string audio_classification;   ///< "Speech", "Music", "Noise", etc.
            std::string spectral_description;   ///< Description of frequency content
            std::string dynamic_description;    ///< Description of dynamics
            std::vector<std::string> detected_features; ///< Detected audio features
            f32 signal_complexity{0.5f};        ///< Signal complexity score (0-1)
            std::string educational_summary;    ///< Summary for educational display
        } educational;
    };

private:
    AnalysisConfig config_;
    u32 sample_rate_;
    
    // FFT processing
    std::unique_ptr<class FFTProcessor> fft_processor_;
    std::vector<f32> window_function_;
    std::vector<f32> input_buffer_;
    std::vector<std::complex<f32>> fft_buffer_;
    std::vector<f32> prev_magnitude_;  // For spectral flux
    usize buffer_position_{0};
    
    // Analysis state
    std::atomic<bool> analysis_ready_{false};
    AnalysisResults current_results_;
    mutable std::mutex results_mutex_;
    
    // Performance tracking
    std::atomic<u32> analyses_performed_{0};
    std::atomic<f32> analysis_time_ms_{0.0f};
    
    // Educational features
    std::vector<std::string> detected_audio_events_;
    std::chrono::high_resolution_clock::time_point last_analysis_time_;
    
public:
    explicit RealtimeAudioAnalyzer(u32 sample_rate, const AnalysisConfig& config = {});
    ~RealtimeAudioAnalyzer();
    
    /** @brief Process audio buffer and update analysis */
    void process_buffer(const f32* mono_audio, usize sample_count);
    void process_stereo_buffer(const AudioFrame* stereo_audio, usize frame_count);
    
    /** @brief Get latest analysis results (thread-safe) */
    AnalysisResults get_analysis_results() const;
    
    /** @brief Check if new analysis is available */
    bool has_new_analysis() const noexcept {
        return analysis_ready_.load(std::memory_order_acquire);
    }
    
    /** @brief Configure analysis parameters */
    void set_config(const AnalysisConfig& config);
    
    /** @brief Generate educational explanation of current analysis */
    std::string generate_educational_explanation() const;
    
    /** @brief Get detected audio events (transients, onsets, etc.) */
    std::vector<std::string> get_detected_audio_events() const;
    
    /** @brief Performance monitoring */
    struct PerformanceInfo {
        f32 average_analysis_time_ms;
        f32 peak_analysis_time_ms;
        f32 cpu_usage_percent;
        u32 analyses_per_second;
        usize memory_usage_bytes;
        bool realtime_safe;             ///< Whether analysis can run in real-time
    };
    
    PerformanceInfo get_performance_info() const;
    
    /** @brief Benchmark analysis performance */
    struct BenchmarkResults {
        f32 fft_time_ms;
        f32 analysis_time_ms;
        f32 total_time_ms;
        f32 max_realtime_buffer_size;   ///< Max buffer size for real-time
        std::string performance_rating; ///< "Excellent", "Good", "Fair", "Poor"
    };
    
    BenchmarkResults benchmark_performance(usize buffer_size, usize iterations = 100) const;
    
private:
    void initialize_fft();
    void calculate_window_function();
    void perform_fft_analysis(const f32* audio, usize count);
    void calculate_spectral_features();
    void calculate_musical_features();
    void calculate_psychoacoustic_features();
    void calculate_spatial_features(const AudioFrame* stereo_audio, usize frame_count);
    void update_educational_insights();
    
    // Utility functions
    f32 hz_to_mel(f32 hz) const;
    f32 mel_to_hz(f32 mel) const;
    f32 hz_to_bark(f32 hz) const;
    f32 calculate_loudness_sones(const std::vector<f32>& bark_spectrum) const;
    void detect_audio_events(const f32* audio, usize count);
};

//=============================================================================
// Audio Processing Pipeline Class
//=============================================================================

/**
 * @brief Main audio processing pipeline coordinator
 * 
 * Educational Context:
 * This class demonstrates how to structure a professional
 * audio processing pipeline with real-time constraints,
 * memory management, and educational features.
 */
class AudioProcessingPipeline {
public:
    /** @brief Pipeline processing statistics */
    struct ProcessingStats {
        std::atomic<u64> buffers_processed{0};
        std::atomic<f64> total_processing_time_ms{0.0};
        std::atomic<f32> cpu_usage_percent{0.0f};
        std::atomic<u32> buffer_underruns{0};
        std::atomic<u32> buffer_overruns{0};
        
        // Real-time performance metrics
        f32 average_buffer_time_ms{0.0f};
        f32 worst_buffer_time_ms{0.0f};
        f32 best_buffer_time_ms{100.0f};
        f32 target_buffer_time_ms{10.0f};
        
        // Memory statistics
        usize peak_memory_usage{0};
        usize current_memory_usage{0};
        u32 memory_allocations{0};
        u32 memory_pool_efficiency_percent{100};
        
        // Educational statistics
        u32 simd_operations_per_second{0};
        f32 simd_performance_gain{1.0f};
        std::string current_quality_level;
        std::vector<std::string> active_optimizations;
    };

private:
    PipelineConfig config_;
    std::unique_ptr<AudioBufferPool> buffer_pool_;
    std::unique_ptr<simd_ops::SIMDDispatcher> simd_dispatcher_;
    std::unique_ptr<DynamicRangeProcessor> dynamics_processor_;
    std::unique_ptr<RealtimeAudioAnalyzer> analyzer_;
    std::unique_ptr<HRTFProcessor> hrtf_processor_;
    std::unique_ptr<AudioEnvironmentProcessor> environment_processor_;
    
    // Threading and synchronization
    std::unique_ptr<std::thread> audio_thread_;
    std::atomic<bool> should_stop_{false};
    std::atomic<bool> is_processing_{false};
    
    // Audio I/O buffers
    using AudioRingBuffer = LockFreeRingBuffer<f32, 8192>;
    std::unique_ptr<AudioRingBuffer> input_buffer_;
    std::unique_ptr<AudioRingBuffer> output_buffer_;
    
    // Performance monitoring
    ProcessingStats processing_stats_;
    std::chrono::high_resolution_clock::time_point last_stats_update_;
    
    // Educational features
    mutable std::mutex educational_data_mutex_;
    std::string current_processing_description_;
    std::vector<std::string> processing_pipeline_stages_;
    
public:
    explicit AudioProcessingPipeline(const PipelineConfig& config);
    ~AudioProcessingPipeline();
    
    // Pipeline control
    bool initialize();
    bool start_processing();
    void stop_processing();
    bool is_running() const noexcept { return is_processing_.load(std::memory_order_acquire); }
    
    // Audio I/O
    bool submit_audio_input(const f32* audio, usize sample_count);
    usize get_audio_output(f32* audio, usize max_samples);
    bool submit_stereo_input(const AudioFrame* audio, usize frame_count);
    usize get_stereo_output(AudioFrame* audio, usize max_frames);
    
    // Configuration management
    void update_config(const PipelineConfig& new_config);
    const PipelineConfig& get_config() const noexcept { return config_; }
    
    // Component access
    DynamicRangeProcessor* get_dynamics_processor() { return dynamics_processor_.get(); }
    RealtimeAudioAnalyzer* get_analyzer() { return analyzer_.get(); }
    HRTFProcessor* get_hrtf_processor() { return hrtf_processor_.get(); }
    AudioEnvironmentProcessor* get_environment_processor() { return environment_processor_.get(); }
    
    // Performance monitoring
    ProcessingStats get_processing_stats() const { return processing_stats_; }
    void reset_stats();
    
    // Educational features
    std::string get_processing_description() const;
    std::vector<std::string> get_pipeline_stages() const;
    
    /** @brief Get detailed educational information about current processing */
    struct EducationalInfo {
        std::string pipeline_overview;
        std::vector<std::string> processing_stages;
        std::string simd_optimization_info;
        std::string memory_management_info;
        std::string realtime_considerations;
        f32 educational_complexity_score; // 0-1, how complex for learning
        std::vector<std::string> key_concepts;
        std::vector<std::string> optimization_techniques;
    };
    
    EducationalInfo get_educational_info() const;
    
    // Performance analysis and optimization
    struct OptimizationReport {
        std::vector<std::string> performance_bottlenecks;
        std::vector<std::string> optimization_suggestions;
        f32 current_efficiency_score; // 0-1
        f32 potential_improvement_percent;
        std::string recommended_config_changes;
    };
    
    OptimizationReport analyze_performance() const;
    
    // Adaptive quality management
    void enable_adaptive_quality(bool enable) { config_.adaptive_quality = enable; }
    PipelineConfig::QualityLevel get_current_quality_level() const;
    void set_quality_level(PipelineConfig::QualityLevel level);
    
private:
    // Main processing thread function
    void processing_thread_main();
    
    // Pipeline stages
    void process_audio_buffer(f32* audio, usize sample_count);
    void process_stereo_buffer(AudioFrame* audio, usize frame_count);
    
    // Performance management
    void update_performance_stats();
    void adaptive_quality_adjustment();
    
    // Memory management
    bool initialize_memory_pools();
    void cleanup_memory_pools();
    
    // Educational data generation
    void update_educational_data();
};

} // namespace ecscope::audio::processing