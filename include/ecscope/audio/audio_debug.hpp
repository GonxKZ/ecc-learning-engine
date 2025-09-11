#pragma once

#include "audio_types.hpp"
#include "audio_processing.hpp"
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <fstream>
#include <chrono>

namespace ecscope::audio {

// Forward declarations
class Audio3DEngine;
class AudioPipeline;

// Audio debugging levels
enum class AudioDebugLevel {
    TRACE,
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

// Audio logger for debugging
class AudioLogger {
public:
    static AudioLogger& instance();
    
    // Logging functions
    void log(AudioDebugLevel level, const std::string& message);
    void log(AudioDebugLevel level, const char* format, ...);
    
    // Convenience functions
    void trace(const std::string& message);
    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    void critical(const std::string& message);
    
    // Logger configuration
    void set_log_level(AudioDebugLevel level);
    void enable_file_logging(bool enable);
    void set_log_file(const std::string& filepath);
    void enable_console_logging(bool enable);
    void enable_timestamps(bool enable);
    void enable_thread_id(bool enable);
    
    // Log filtering
    void add_filter(const std::string& filter);
    void remove_filter(const std::string& filter);
    void clear_filters();
    
    // Log callbacks
    using LogCallback = std::function<void(AudioDebugLevel, const std::string&)>;
    void set_log_callback(LogCallback callback);
    
    // Log management
    void flush();
    void clear_log();
    std::vector<std::string> get_recent_logs(size_t count = 100) const;

private:
    AudioLogger() = default;
    ~AudioLogger() = default;
    AudioLogger(const AudioLogger&) = delete;
    AudioLogger& operator=(const AudioLogger&) = delete;
    
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

// Audio visualizer for real-time debugging
class AudioVisualizer {
public:
    struct VisualizationConfig {
        bool show_waveform = true;
        bool show_spectrum = true;
        bool show_spectrogram = false;
        bool show_3d_positions = true;
        bool show_hrtf_visualization = false;
        bool show_ray_tracing = false;
        bool show_performance_metrics = true;
        
        // Waveform settings
        float waveform_scale = 1.0f;
        int waveform_buffer_size = 1024;
        
        // Spectrum settings
        int spectrum_fft_size = 2048;
        float spectrum_min_db = -80.0f;
        float spectrum_max_db = 0.0f;
        bool spectrum_log_frequency = true;
        
        // 3D visualization settings
        float position_marker_size = 0.1f;
        float max_distance_visualization = 100.0f;
        bool show_attenuation_spheres = true;
        bool show_listener_orientation = true;
        
        // Performance settings
        int update_rate_hz = 60;
        bool show_cpu_usage = true;
        bool show_memory_usage = true;
        bool show_voice_count = true;
    };
    
    AudioVisualizer();
    ~AudioVisualizer();
    
    // Configuration
    void set_config(const VisualizationConfig& config);
    VisualizationConfig get_config() const;
    void enable_visualization(bool enable);
    bool is_visualization_enabled() const;
    
    // Data input
    void update_audio_buffer(const AudioBuffer& buffer, uint32_t channel = 0);
    void update_stereo_buffer(const StereoBuffer& buffer);
    void update_3d_positions(const std::vector<Vector3f>& source_positions,
                           const Vector3f& listener_position,
                           const Quaternion& listener_orientation);
    void update_performance_metrics(const AudioMetrics& metrics);
    
    // Rendering (platform-specific implementation)
    void render_to_texture(uint32_t texture_id, int width, int height);
    void render_to_screen(int x, int y, int width, int height);
    void render_imgui_window();  // For Dear ImGui integration
    
    // Waveform visualization
    std::vector<float> get_waveform_data() const;
    void set_waveform_time_window(float seconds);
    
    // Spectrum visualization
    std::vector<float> get_spectrum_data() const;
    std::vector<float> get_spectrum_frequencies() const;
    void set_spectrum_smoothing(float smoothing_factor);
    
    // 3D audio visualization
    void render_3d_audio_scene(const Audio3DEngine& engine);
    void set_3d_camera_position(const Vector3f& position, const Quaternion& orientation);
    
    // Screenshot and recording
    void take_screenshot(const std::string& filepath);
    void start_recording(const std::string& filepath, float duration = 10.0f);
    void stop_recording();
    bool is_recording() const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

// Audio performance monitor
class AudioPerformanceMonitor {
public:
    struct PerformanceSnapshot {
        float timestamp = 0.0f;
        float cpu_usage = 0.0f;
        float memory_usage_mb = 0.0f;
        uint32_t active_voices = 0;
        uint32_t buffer_underruns = 0;
        uint32_t buffer_overruns = 0;
        float latency_ms = 0.0f;
        float processing_time_ms = 0.0f;
        
        // 3D audio specific
        uint32_t hrtf_convolutions = 0;
        uint32_t ray_tracing_rays = 0;
        float spatial_processing_time_ms = 0.0f;
        
        // Thread statistics
        std::vector<float> thread_usage;
        float job_queue_length = 0.0f;
    };
    
    AudioPerformanceMonitor();
    ~AudioPerformanceMonitor();
    
    // Monitoring control
    void start_monitoring();
    void stop_monitoring();
    void reset_statistics();
    bool is_monitoring() const;
    
    // Data collection
    void update(const AudioMetrics& metrics);
    void record_frame_time(float frame_time_ms);
    void record_processing_time(const std::string& section, float time_ms);
    
    // Data access
    std::vector<PerformanceSnapshot> get_history(float duration_seconds = 10.0f) const;
    PerformanceSnapshot get_current_snapshot() const;
    PerformanceSnapshot get_average_snapshot(float duration_seconds = 5.0f) const;
    PerformanceSnapshot get_peak_snapshot(float duration_seconds = 10.0f) const;
    
    // Alerts and thresholds
    void set_cpu_threshold(float threshold_percent);
    void set_memory_threshold(float threshold_mb);
    void set_latency_threshold(float threshold_ms);
    void set_alert_callback(std::function<void(const std::string&)> callback);
    
    // Export and analysis
    void export_to_csv(const std::string& filepath, float duration_seconds = 60.0f);
    void export_to_json(const std::string& filepath, float duration_seconds = 60.0f);
    
    // Real-time performance display
    void render_performance_overlay();
    std::string generate_performance_report() const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

// Audio analyzer for frequency and waveform analysis
class AudioAnalyzer {
public:
    struct AnalysisResult {
        // Time domain analysis
        float rms_level = 0.0f;
        float peak_level = 0.0f;
        float crest_factor = 0.0f;
        float zero_crossing_rate = 0.0f;
        
        // Frequency domain analysis
        std::vector<float> magnitude_spectrum;
        std::vector<float> phase_spectrum;
        std::vector<float> frequency_bins;
        float spectral_centroid = 0.0f;
        float spectral_rolloff = 0.0f;
        float spectral_flux = 0.0f;
        float spectral_flatness = 0.0f;
        
        // Perceptual analysis
        std::vector<float> mel_spectrum;
        std::vector<float> mfcc;  // Mel-frequency cepstral coefficients
        float loudness_lufs = 0.0f;  // LUFS loudness measurement
        
        // 3D audio analysis
        float stereo_width = 0.0f;
        float left_right_correlation = 0.0f;
        Vector3f estimated_source_position{0.0f, 0.0f, 0.0f};
        
        float analysis_timestamp = 0.0f;
    };
    
    AudioAnalyzer(uint32_t sample_rate = 44100, uint32_t analysis_window = 2048);
    ~AudioAnalyzer();
    
    // Configuration
    void set_sample_rate(uint32_t sample_rate);
    void set_analysis_window(uint32_t window_size);
    void set_overlap_factor(float overlap);  // 0.0 to 1.0
    void set_window_function(enum WindowFunction { 
        RECTANGULAR, HANNING, HAMMING, BLACKMAN, KAISER 
    } window);
    
    // Analysis operations
    AnalysisResult analyze_buffer(const AudioBuffer& buffer);
    AnalysisResult analyze_stereo_buffer(const StereoBuffer& buffer);
    void analyze_continuous(const AudioBuffer& buffer);  // For real-time analysis
    
    // Advanced analysis
    void enable_pitch_detection(bool enable);
    float detect_fundamental_frequency(const AudioBuffer& buffer);
    std::vector<float> detect_harmonics(const AudioBuffer& buffer, float fundamental);
    
    void enable_onset_detection(bool enable);
    std::vector<float> detect_onsets(const AudioBuffer& buffer);
    
    void enable_beat_tracking(bool enable);
    std::vector<float> track_beats(const AudioBuffer& buffer);
    
    // Quality analysis
    float calculate_thd(const AudioBuffer& buffer, float fundamental);  // Total Harmonic Distortion
    float calculate_snr(const AudioBuffer& signal, const AudioBuffer& noise);
    float calculate_dynamic_range(const AudioBuffer& buffer);
    
    // Real-time monitoring
    void start_realtime_analysis();
    void stop_realtime_analysis();
    AnalysisResult get_realtime_result() const;
    
    // History and trends
    std::vector<AnalysisResult> get_analysis_history(float duration_seconds = 10.0f) const;
    void export_analysis_data(const std::string& filepath, float duration_seconds = 60.0f);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

// Audio unit tester for automated testing
class AudioUnitTester {
public:
    struct TestResult {
        std::string test_name;
        bool passed = false;
        std::string error_message;
        float execution_time_ms = 0.0f;
        std::vector<std::string> warnings;
    };
    
    struct TestSuite {
        std::string name;
        std::vector<std::function<TestResult()>> tests;
        bool parallel_execution = false;
    };
    
    AudioUnitTester();
    ~AudioUnitTester();
    
    // Test registration
    void register_test(const std::string& suite_name, const std::string& test_name,
                      std::function<TestResult()> test_function);
    void register_test_suite(const TestSuite& suite);
    
    // Test execution
    std::vector<TestResult> run_all_tests();
    std::vector<TestResult> run_test_suite(const std::string& suite_name);
    TestResult run_single_test(const std::string& suite_name, const std::string& test_name);
    
    // Built-in audio tests
    void register_builtin_tests();
    TestResult test_audio_device_initialization();
    TestResult test_audio_buffer_operations();
    TestResult test_3d_audio_positioning();
    TestResult test_hrtf_processing();
    TestResult test_effects_chain();
    TestResult test_file_loading();
    TestResult test_streaming_performance();
    TestResult test_memory_leaks();
    
    // Test utilities
    AudioBuffer generate_sine_wave(float frequency, float duration, uint32_t sample_rate = 44100);
    AudioBuffer generate_white_noise(float duration, uint32_t sample_rate = 44100);
    AudioBuffer generate_impulse(float duration, uint32_t sample_rate = 44100);
    
    bool compare_buffers(const AudioBuffer& a, const AudioBuffer& b, float tolerance = 0.001f);
    float calculate_buffer_difference(const AudioBuffer& a, const AudioBuffer& b);
    
    // Reporting
    void generate_test_report(const std::vector<TestResult>& results, const std::string& filepath);
    void print_test_summary(const std::vector<TestResult>& results);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

// Audio debugger interface for interactive debugging
class AudioDebugger {
public:
    AudioDebugger();
    ~AudioDebugger();
    
    // Debugger control
    void attach_to_audio_system(Audio3DEngine* engine, AudioPipeline* pipeline);
    void detach();
    bool is_attached() const;
    
    // Breakpoints and stepping
    void set_breakpoint(const std::string& component, const std::string& condition);
    void remove_breakpoint(const std::string& component);
    void clear_all_breakpoints();
    
    void pause_audio_processing();
    void resume_audio_processing();
    void step_single_frame();
    bool is_paused() const;
    
    // State inspection
    std::string get_audio_state_dump() const;
    std::vector<std::string> get_active_voices() const;
    std::vector<std::string> get_loaded_audio_files() const;
    AudioMetrics get_current_metrics() const;
    
    // Real-time modification
    void modify_voice_parameter(uint32_t voice_id, const std::string& parameter, float value);
    void modify_global_parameter(const std::string& parameter, float value);
    void inject_audio_buffer(const AudioBuffer& buffer);
    
    // Audio capture and playback
    void start_audio_capture(const std::string& filepath);
    void stop_audio_capture();
    void playback_captured_audio(const std::string& filepath);
    
    // Script debugging
    void execute_debug_script(const std::string& script_code);
    void load_debug_script(const std::string& filepath);
    std::string get_script_output() const;
    
    // Interactive console
    void start_debug_console();
    void stop_debug_console();
    void process_console_command(const std::string& command);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

// Audio development utilities
namespace audio_debug_utils {
    // File format validation
    bool validate_audio_file(const std::string& filepath);
    std::string get_file_format_info(const std::string& filepath);
    bool repair_audio_file_headers(const std::string& filepath);
    
    // Audio quality assessment
    float calculate_audio_quality_score(const AudioBuffer& buffer);
    std::vector<std::string> detect_audio_artifacts(const AudioBuffer& buffer);
    bool check_for_clipping(const AudioBuffer& buffer, float threshold = 0.99f);
    bool check_for_silence(const AudioBuffer& buffer, float threshold = 0.001f);
    
    // Performance benchmarking
    float benchmark_3d_audio_performance(Audio3DEngine& engine, int num_sources = 100);
    float benchmark_effects_performance(const std::vector<std::unique_ptr<AudioEffect>>& effects);
    float benchmark_file_loading_speed(const std::string& filepath);
    
    // Memory debugging
    void track_audio_memory_allocations(bool enable);
    size_t get_total_audio_memory_usage();
    std::vector<std::string> get_memory_leak_report();
    void dump_audio_memory_statistics(const std::string& filepath);
    
    // Thread debugging
    void monitor_audio_thread_performance(bool enable);
    std::string get_audio_thread_report();
    void detect_audio_thread_contention();
    
    // Validation utilities
    bool validate_3d_audio_setup(const Audio3DEngine& engine);
    bool validate_hrtf_database(const std::string& filepath);
    bool validate_impulse_response(const AudioBuffer& ir);
    
    // Debug output
    void dump_audio_system_state(const std::string& filepath);
    void generate_audio_debug_report(const std::string& filepath);
    void save_audio_configuration(const std::string& filepath);
    void load_audio_configuration(const std::string& filepath);
}

// Debug macros for convenient logging
#define AUDIO_LOG_TRACE(msg) ecscope::audio::AudioLogger::instance().trace(msg)
#define AUDIO_LOG_DEBUG(msg) ecscope::audio::AudioLogger::instance().debug(msg)
#define AUDIO_LOG_INFO(msg) ecscope::audio::AudioLogger::instance().info(msg)
#define AUDIO_LOG_WARNING(msg) ecscope::audio::AudioLogger::instance().warning(msg)
#define AUDIO_LOG_ERROR(msg) ecscope::audio::AudioLogger::instance().error(msg)
#define AUDIO_LOG_CRITICAL(msg) ecscope::audio::AudioLogger::instance().critical(msg)

#ifdef AUDIO_DEBUG_BUILD
    #define AUDIO_ASSERT(condition, msg) \
        do { \
            if (!(condition)) { \
                AUDIO_LOG_CRITICAL("Assertion failed: " #condition " - " msg); \
                std::terminate(); \
            } \
        } while(0)
    
    #define AUDIO_DEBUG_ONLY(code) code
#else
    #define AUDIO_ASSERT(condition, msg) ((void)0)
    #define AUDIO_DEBUG_ONLY(code) ((void)0)
#endif

} // namespace ecscope::audio