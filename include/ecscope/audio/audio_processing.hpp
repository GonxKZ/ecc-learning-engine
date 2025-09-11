#pragma once

#include "audio_types.hpp"
#include <memory>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <queue>
#include <future>
#include <immintrin.h>  // For AVX/SSE intrinsics

namespace ecscope::audio {

// SIMD-optimized audio operations
class AudioSIMD {
public:
    // Detection of available SIMD instruction sets
    struct SIMDCapabilities {
        bool sse = false;
        bool sse2 = false;
        bool sse3 = false;
        bool ssse3 = false;
        bool sse41 = false;
        bool sse42 = false;
        bool avx = false;
        bool avx2 = false;
        bool fma3 = false;
        bool avx512 = false;
        
        static SIMDCapabilities detect();
    };
    
    static SIMDCapabilities get_capabilities();
    
    // Basic SIMD operations
    static void add_arrays_simd(const float* a, const float* b, float* result, size_t count);
    static void multiply_arrays_simd(const float* a, const float* b, float* result, size_t count);
    static void multiply_scalar_simd(const float* input, float scalar, float* output, size_t count);
    static void copy_array_simd(const float* source, float* dest, size_t count);
    static void zero_array_simd(float* array, size_t count);
    
    // Audio-specific SIMD operations
    static void interleave_stereo_simd(const float* left, const float* right, 
                                     float* interleaved, size_t samples);
    static void deinterleave_stereo_simd(const float* interleaved, 
                                       float* left, float* right, size_t samples);
    
    static void apply_gain_simd(float* audio, float gain, size_t samples);
    static void apply_gain_ramp_simd(float* audio, float start_gain, 
                                   float end_gain, size_t samples);
    
    // DSP operations
    static void fir_filter_simd(const float* input, const float* coefficients,
                              float* output, size_t input_len, size_t filter_len);
    
    static void convolution_simd(const float* signal, const float* kernel,
                               float* output, size_t signal_len, size_t kernel_len);
    
    static void fft_real_simd(const float* input, float* real_output, 
                            float* imag_output, size_t length);
    
    static void ifft_real_simd(const float* real_input, const float* imag_input,
                             float* output, size_t length);
    
    // Vector operations for 3D audio
    static void calculate_distances_simd(const Vector3f* positions, 
                                       const Vector3f& listener_pos,
                                       float* distances, size_t count);
    
    static void apply_distance_attenuation_simd(float* gains, const float* distances,
                                              float min_distance, float max_distance,
                                              float rolloff, size_t count);

private:
    static SIMDCapabilities s_capabilities;
    static bool s_capabilities_detected;
};

// Audio thread pool for parallel processing
class AudioThreadPool {
public:
    AudioThreadPool(size_t num_threads = 0);  // 0 = auto-detect
    ~AudioThreadPool();
    
    // Task submission
    template<typename F>
    auto submit(F&& task) -> std::future<decltype(task())>;
    
    void submit_no_return(std::function<void()> task);
    
    // Parallel audio processing utilities
    void parallel_process(const AudioBuffer& input, AudioBuffer& output,
                         std::function<float(float)> processor);
    
    void parallel_process_stereo(const StereoBuffer& input, StereoBuffer& output,
                               std::function<void(float&, float&)> processor);
    
    void parallel_mix(const std::vector<AudioBuffer>& inputs, AudioBuffer& output);
    
    void parallel_convolve(const AudioBuffer& signal, const AudioBuffer& kernel,
                          AudioBuffer& output);
    
    // Thread pool management
    void resize(size_t num_threads);
    size_t get_thread_count() const;
    void shutdown();
    bool is_running() const;
    
    // Performance monitoring
    size_t get_active_tasks() const;
    size_t get_completed_tasks() const;
    float get_cpu_usage() const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

// Lock-free ring buffer for audio streaming
template<typename T>
class LockFreeRingBuffer {
public:
    explicit LockFreeRingBuffer(size_t capacity);
    ~LockFreeRingBuffer();
    
    // Thread-safe operations
    bool push(const T& item);
    bool push(T&& item);
    bool pop(T& item);
    
    // Bulk operations
    size_t push_bulk(const T* items, size_t count);
    size_t pop_bulk(T* items, size_t count);
    
    // State queries
    size_t size() const;
    size_t capacity() const;
    bool empty() const;
    bool full() const;
    float load_factor() const;
    
    // Reset
    void clear();

private:
    std::unique_ptr<T[]> m_buffer;
    size_t m_capacity;
    std::atomic<size_t> m_head{0};
    std::atomic<size_t> m_tail{0};
    
    size_t next_index(size_t index) const;
};

// Real-time audio processor with multi-threading
class AudioProcessor {
public:
    struct ProcessingConfig {
        uint32_t sample_rate = 44100;
        uint32_t buffer_size = 1024;
        uint32_t num_channels = 2;
        uint32_t thread_count = 0;  // 0 = auto-detect
        bool enable_simd = true;
        bool enable_prefetch = true;
        int cpu_affinity_mask = -1;  // -1 = no affinity
        enum Priority { LOW, NORMAL, HIGH, REALTIME } priority = HIGH;
    };
    
    AudioProcessor();
    ~AudioProcessor();
    
    // Configuration
    void initialize(const ProcessingConfig& config);
    void shutdown();
    bool is_initialized() const;
    
    // Processing callback registration
    using ProcessingCallback = std::function<void(AudioBuffer&, const AudioFormat&)>;
    void set_processing_callback(ProcessingCallback callback);
    
    // Real-time processing control
    void start_processing();
    void stop_processing();
    void pause_processing();
    void resume_processing();
    bool is_processing() const;
    
    // Buffer management
    void push_input_buffer(const AudioBuffer& buffer);
    bool pop_output_buffer(AudioBuffer& buffer);
    size_t get_input_latency_samples() const;
    size_t get_output_latency_samples() const;
    
    // Performance monitoring
    AudioMetrics get_processing_metrics() const;
    float get_cpu_usage() const;
    float get_memory_usage_mb() const;
    uint32_t get_buffer_underruns() const;
    uint32_t get_buffer_overruns() const;
    
    // Thread management
    void set_thread_priority(std::thread::native_handle_type handle, int priority);
    void set_thread_affinity(std::thread::native_handle_type handle, int core_mask);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

// Optimized DSP operations
class AudioDSP {
public:
    // Filter implementations
    class BiquadFilter {
    public:
        enum FilterType {
            LOW_PASS,
            HIGH_PASS,
            BAND_PASS,
            NOTCH,
            ALL_PASS,
            PEAKING_EQ,
            LOW_SHELF,
            HIGH_SHELF
        };
        
        BiquadFilter();
        void set_parameters(FilterType type, float frequency, float q, 
                          float gain_db, float sample_rate);
        
        float process_sample(float input);
        void process_buffer(const float* input, float* output, size_t samples);
        void process_buffer_simd(const float* input, float* output, size_t samples);
        void reset();
        
    private:
        float m_a0, m_a1, m_a2, m_b1, m_b2;
        float m_x1 = 0.0f, m_x2 = 0.0f, m_y1 = 0.0f, m_y2 = 0.0f;
    };
    
    // Fast Fourier Transform
    class FFT {
    public:
        explicit FFT(size_t size);
        ~FFT();
        
        void forward(const float* input, float* real_output, float* imag_output);
        void inverse(const float* real_input, const float* imag_input, float* output);
        void forward_simd(const float* input, float* real_output, float* imag_output);
        void inverse_simd(const float* real_input, const float* imag_input, float* output);
        
        size_t get_size() const;
        bool is_power_of_two(size_t n) const;
        
    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };
    
    // Overlap-Add convolution
    class OverlapAddConvolver {
    public:
        OverlapAddConvolver(size_t block_size, const AudioBuffer& kernel);
        ~OverlapAddConvolver();
        
        void process(const float* input, float* output, size_t samples);
        void process_simd(const float* input, float* output, size_t samples);
        void set_kernel(const AudioBuffer& kernel);
        void reset();
        
    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };
    
    // Resampler for sample rate conversion
    class Resampler {
    public:
        Resampler(float input_rate, float output_rate, int quality = 5);
        ~Resampler();
        
        size_t process(const float* input, size_t input_samples, 
                      float* output, size_t output_capacity);
        
        void reset();
        void set_rates(float input_rate, float output_rate);
        float get_latency() const;
        
    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };
    
    // Static utility functions
    static float db_to_linear(float db);
    static float linear_to_db(float linear);
    static float cents_to_ratio(float cents);
    static float ratio_to_cents(float ratio);
    
    // Window functions
    static void generate_hanning_window(float* window, size_t size);
    static void generate_hamming_window(float* window, size_t size);
    static void generate_blackman_window(float* window, size_t size);
    static void generate_kaiser_window(float* window, size_t size, float beta);
    
    // Analysis functions
    static float calculate_rms(const float* buffer, size_t samples);
    static float calculate_peak(const float* buffer, size_t samples);
    static void calculate_spectrum(const float* input, float* magnitude, 
                                 float* phase, size_t fft_size);
    
    // Time-domain effects
    static void apply_fade_in(float* buffer, size_t samples, size_t fade_samples);
    static void apply_fade_out(float* buffer, size_t samples, size_t fade_samples);
    static void apply_crossfade(const float* buffer_a, const float* buffer_b,
                              float* output, size_t samples);
};

// Audio memory management optimized for real-time processing
class AudioMemoryManager {
public:
    static AudioMemoryManager& instance();
    
    // Aligned memory allocation for SIMD
    void* allocate_aligned(size_t size, size_t alignment = 32);
    void deallocate_aligned(void* ptr);
    
    // Pool allocators for common audio buffer sizes
    AudioBuffer allocate_buffer(size_t samples);
    void deallocate_buffer(AudioBuffer& buffer);
    
    StereoBuffer allocate_stereo_buffer(size_t samples);
    void deallocate_stereo_buffer(StereoBuffer& buffer);
    
    // Memory pool configuration
    void configure_pools(const std::vector<size_t>& buffer_sizes, 
                        size_t buffers_per_pool = 32);
    
    // Memory usage monitoring
    size_t get_total_allocated() const;
    size_t get_peak_usage() const;
    size_t get_current_usage() const;
    float get_fragmentation_ratio() const;
    
    // Cleanup
    void cleanup_pools();
    void force_garbage_collection();

private:
    AudioMemoryManager() = default;
    ~AudioMemoryManager() = default;
    AudioMemoryManager(const AudioMemoryManager&) = delete;
    AudioMemoryManager& operator=(const AudioMemoryManager&) = delete;
    
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

// Job system for audio tasks
class AudioJobSystem {
public:
    struct JobHandle {
        uint64_t id = 0;
        std::atomic<bool> completed{false};
        std::function<void()> on_complete;
    };
    
    static AudioJobSystem& instance();
    
    // Job submission
    JobHandle submit_job(std::function<void()> job, int priority = 0);
    JobHandle submit_dependent_job(std::function<void()> job, 
                                 const std::vector<JobHandle*>& dependencies);
    
    // Job management
    void wait_for_job(const JobHandle& handle);
    void wait_for_all_jobs();
    bool is_job_complete(const JobHandle& handle) const;
    
    // Parallel patterns
    void parallel_for(size_t start, size_t end, size_t grain_size,
                     std::function<void(size_t)> task);
    
    void parallel_transform(const float* input, float* output, size_t count,
                          std::function<float(float)> transform);
    
    // System control
    void initialize(size_t num_threads = 0);
    void shutdown();
    size_t get_worker_count() const;
    
    // Performance monitoring
    size_t get_pending_jobs() const;
    size_t get_completed_jobs() const;
    float get_average_job_time_ms() const;

private:
    AudioJobSystem() = default;
    ~AudioJobSystem() = default;
    AudioJobSystem(const AudioJobSystem&) = delete;
    AudioJobSystem& operator=(const AudioJobSystem&) = delete;
    
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

// Performance profiler for audio processing
class AudioProfiler {
public:
    static AudioProfiler& instance();
    
    // Profiling markers
    void begin_frame();
    void end_frame();
    void begin_section(const std::string& name);
    void end_section(const std::string& name);
    
    // Scoped profiler RAII helper
    class ScopedProfiler {
    public:
        explicit ScopedProfiler(const std::string& name);
        ~ScopedProfiler();
    private:
        std::string m_name;
    };
    
    // Performance data access
    struct ProfileData {
        std::string name;
        float average_time_ms = 0.0f;
        float min_time_ms = 0.0f;
        float max_time_ms = 0.0f;
        uint64_t call_count = 0;
        float cpu_percentage = 0.0f;
    };
    
    std::vector<ProfileData> get_profile_data() const;
    ProfileData get_section_data(const std::string& name) const;
    
    // Configuration
    void enable_profiling(bool enable);
    void set_history_size(size_t frames);
    void reset_statistics();
    
    // Export/Import
    void save_profile_data(const std::string& filepath) const;
    void load_profile_data(const std::string& filepath);

private:
    AudioProfiler() = default;
    ~AudioProfiler() = default;
    AudioProfiler(const AudioProfiler&) = delete;
    AudioProfiler& operator=(const AudioProfiler&) = delete;
    
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

// Macros for easy profiling
#define AUDIO_PROFILE_FRAME() \
    ecscope::audio::AudioProfiler::instance().begin_frame(); \
    auto _frame_end = std::make_unique<std::function<void()>>([]{ \
        ecscope::audio::AudioProfiler::instance().end_frame(); \
    });

#define AUDIO_PROFILE_SECTION(name) \
    ecscope::audio::AudioProfiler::ScopedProfiler _prof(name)

} // namespace ecscope::audio