#pragma once

// Core audio system includes
#include "audio_types.hpp"
#include "audio_device.hpp"
#include "audio_pipeline.hpp"
#include "audio_3d.hpp"

// Advanced audio processing
#include "hrtf_processor.hpp"
#include "audio_effects.hpp"
#include "ambisonics.hpp"
#include "audio_raytracing.hpp"

// Performance and threading
#include "audio_processing.hpp"

// ECS integration
#include "audio_ecs.hpp"

// Development and debugging
#include "audio_debug.hpp"

#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace ecscope::audio {

// Main audio system configuration
struct AudioSystemConfig {
    // Core audio settings
    AudioFormat format{44100, 2, 32, 1024};
    std::string device_name;  // Empty = default device
    
    // 3D audio settings
    bool enable_3d_audio = true;
    bool enable_hrtf = true;
    std::string hrtf_database_path;
    HRTFInterpolation hrtf_interpolation = HRTFInterpolation::LINEAR;
    
    // Advanced features
    bool enable_ambisonics = false;
    uint32_t ambisonics_order = 1;
    bool enable_ray_tracing = false;
    int ray_tracing_quality = 5;  // 1-10 scale
    
    // Performance settings
    uint32_t thread_count = 0;  // 0 = auto-detect
    bool enable_simd = true;
    bool enable_job_system = true;
    int cpu_affinity_mask = -1;  // -1 = no affinity
    
    // Memory settings
    size_t memory_pool_size = 64 * 1024 * 1024;  // 64MB
    std::vector<size_t> buffer_pool_sizes = {256, 512, 1024, 2048, 4096, 8192};
    
    // Development settings
    bool enable_debugging = false;
    bool enable_profiling = false;
    bool enable_visualization = false;
    AudioDebugLevel log_level = AudioDebugLevel::WARNING;
    std::string log_file_path;
    
    // ECS integration
    bool enable_ecs_integration = true;
    bool auto_register_systems = true;
};

// Main audio system class - central management interface
class AudioSystem {
public:
    AudioSystem();
    ~AudioSystem();
    
    // System lifecycle
    bool initialize(const AudioSystemConfig& config);
    void shutdown();
    bool is_initialized() const;
    
    // Configuration management
    void set_config(const AudioSystemConfig& config);
    AudioSystemConfig get_config() const;
    void reload_config();
    void save_config(const std::string& filepath) const;
    void load_config(const std::string& filepath);
    
    // Core system access
    Audio3DEngine& get_3d_engine();
    const Audio3DEngine& get_3d_engine() const;
    
    AudioPipeline& get_pipeline();
    const AudioPipeline& get_pipeline() const;
    
    AudioDeviceManager& get_device_manager();
    const AudioDeviceManager& get_device_manager() const;
    
    // Advanced system access
    HRTFProcessor* get_hrtf_processor();
    AmbisonicsProcessor* get_ambisonics_processor();
    RayTracingAudioProcessor* get_raytracing_processor();
    
    // Performance system access
    AudioThreadPool& get_thread_pool();
    AudioJobSystem& get_job_system();
    AudioMemoryManager& get_memory_manager();
    
    // Development tools access
    AudioLogger& get_logger();
    AudioVisualizer* get_visualizer();
    AudioPerformanceMonitor* get_performance_monitor();
    AudioAnalyzer* get_analyzer();
    AudioDebugger* get_debugger();
    
    // ECS system access
    ecs::AudioSystem* get_ecs_audio_system();
    ecs::AudioZoneSystem* get_ecs_zone_system();
    ecs::AudioStreamingSystem* get_ecs_streaming_system();
    ecs::AudioEventSystem* get_ecs_event_system();
    
    // Global audio control
    void set_master_volume(float volume);
    float get_master_volume() const;
    void set_global_pause(bool paused);
    bool is_globally_paused() const;
    void set_global_time_scale(float scale);
    float get_global_time_scale() const;
    
    // Global effects
    void add_global_effect(std::unique_ptr<AudioEffect> effect);
    void remove_global_effect(size_t index);
    void clear_global_effects();
    EffectsChain& get_global_effects_chain();
    
    // System update (call once per frame)
    void update(float delta_time);
    
    // Quick audio playback utilities
    uint32_t play_sound(const std::string& filepath);
    uint32_t play_sound_3d(const std::string& filepath, const Vector3f& position);
    uint32_t play_sound_with_effects(const std::string& filepath, 
                                   const std::vector<std::unique_ptr<AudioEffect>>& effects);
    void stop_sound(uint32_t sound_id);
    void stop_all_sounds();
    
    // Audio resource management
    void preload_audio_file(const std::string& filepath);
    void unload_audio_file(const std::string& filepath);
    void preload_audio_directory(const std::string& directory_path);
    void clear_audio_cache();
    size_t get_audio_cache_size() const;
    
    // Scene management
    void load_audio_scene(const std::string& scene_filepath);
    void save_audio_scene(const std::string& scene_filepath);
    void clear_audio_scene();
    
    // Event system
    void register_audio_event(const std::string& name, const std::string& filepath);
    void trigger_audio_event(const std::string& name);
    void trigger_audio_event_3d(const std::string& name, const Vector3f& position);
    
    // Performance monitoring
    AudioMetrics get_system_metrics() const;
    std::string generate_performance_report() const;
    void reset_performance_counters();
    
    // Error handling
    AudioError get_last_error() const;
    std::string get_error_string() const;
    void set_error_callback(std::function<void(AudioError, const std::string&)> callback);
    
    // System information
    static std::string get_version();
    static std::vector<std::string> get_supported_formats();
    static std::vector<std::string> get_available_effects();
    static AudioSystemConfig get_recommended_config();
    
    // Factory methods
    static std::unique_ptr<AudioSystem> create();
    static std::unique_ptr<AudioSystem> create_with_config(const AudioSystemConfig& config);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
    
    // Initialization helpers
    bool initialize_core_systems();
    bool initialize_3d_audio();
    bool initialize_advanced_features();
    bool initialize_performance_systems();
    bool initialize_ecs_integration();
    bool initialize_debug_tools();
    
    void shutdown_in_reverse_order();
};

// Global audio system instance (singleton pattern)
class GlobalAudioSystem {
public:
    static bool initialize(const AudioSystemConfig& config);
    static void shutdown();
    static AudioSystem& instance();
    static bool is_initialized();

private:
    static std::unique_ptr<AudioSystem> s_instance;
    static bool s_initialized;
};

// Convenience macros for global access
#define AUDIO_SYSTEM() ecscope::audio::GlobalAudioSystem::instance()
#define AUDIO_3D() ecscope::audio::GlobalAudioSystem::instance().get_3d_engine()
#define AUDIO_PIPELINE() ecscope::audio::GlobalAudioSystem::instance().get_pipeline()
#define AUDIO_LOGGER() ecscope::audio::GlobalAudioSystem::instance().get_logger()

// Audio system factory for different configurations
class AudioSystemFactory {
public:
    // Preset configurations
    static AudioSystemConfig create_gaming_config();
    static AudioSystemConfig create_vr_config();
    static AudioSystemConfig create_music_production_config();
    static AudioSystemConfig create_minimal_config();
    static AudioSystemConfig create_development_config();
    
    // Platform-specific configurations
    static AudioSystemConfig create_windows_config();
    static AudioSystemConfig create_linux_config();
    static AudioSystemConfig create_macos_config();
    static AudioSystemConfig create_mobile_config();
    
    // Hardware-specific optimizations
    static AudioSystemConfig optimize_for_hardware();
    static AudioSystemConfig optimize_for_low_latency();
    static AudioSystemConfig optimize_for_quality();
    static AudioSystemConfig optimize_for_performance();
    
    // Configuration validation
    static bool validate_config(const AudioSystemConfig& config);
    static std::vector<std::string> get_config_warnings(const AudioSystemConfig& config);
    static AudioSystemConfig fix_config_issues(const AudioSystemConfig& config);
};

// Audio system utilities
namespace audio_system_utils {
    // System information
    std::string get_system_audio_info();
    std::vector<AudioDeviceInfo> get_all_audio_devices();
    bool is_audio_device_available(const std::string& device_name);
    
    // Capability testing
    bool test_3d_audio_support();
    bool test_hrtf_support();
    bool test_ambisonics_support();
    bool test_ray_tracing_support();
    SIMDCapabilities get_simd_capabilities();
    
    // Performance benchmarking
    float benchmark_system_performance();
    float benchmark_3d_audio_performance();
    float benchmark_effects_performance();
    std::string generate_benchmark_report();
    
    // Configuration helpers
    AudioSystemConfig detect_optimal_config();
    void apply_platform_optimizations(AudioSystemConfig& config);
    void apply_hardware_optimizations(AudioSystemConfig& config);
    
    // Migration utilities
    bool migrate_config_from_version(AudioSystemConfig& config, const std::string& from_version);
    void backup_audio_settings(const std::string& backup_path);
    bool restore_audio_settings(const std::string& backup_path);
    
    // Diagnostics
    std::vector<std::string> run_audio_diagnostics();
    bool repair_audio_system();
    void generate_diagnostic_report(const std::string& filepath);
    
    // Resource management
    void cleanup_temporary_audio_files();
    void optimize_audio_cache();
    size_t calculate_optimal_buffer_sizes(uint32_t sample_rate, uint32_t target_latency_ms);
}

} // namespace ecscope::audio

// Global initialization and cleanup macros
#define ECSCOPE_AUDIO_INIT(config) ecscope::audio::GlobalAudioSystem::initialize(config)
#define ECSCOPE_AUDIO_SHUTDOWN() ecscope::audio::GlobalAudioSystem::shutdown()

// Convenience include for all audio functionality
#ifdef ECSCOPE_AUDIO_INCLUDE_ALL
    #include "audio_system.hpp"
    using namespace ecscope::audio;
#endif