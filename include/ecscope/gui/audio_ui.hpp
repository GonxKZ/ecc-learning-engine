/**
 * @file audio_ui.hpp
 * @brief Comprehensive Audio System UI with 3D Visualization and Real-time Controls
 * 
 * Professional audio interface for ECScope engine featuring:
 * - Real-time 3D visualization of audio sources and listeners
 * - HRTF processing visualization with head tracking
 * - Audio effects chain interface with live preview
 * - Spatial audio controls and environmental audio presets
 * - Integration with existing audio system and dashboard
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <array>
#include <functional>
#include <atomic>
#include <mutex>
#include <chrono>
#include <optional>
#include <queue>

// ImGui includes
#ifdef ECSCOPE_HAS_IMGUI
#include <imgui.h>
#include <imgui_internal.h>
// Note: ImPlot not available, using custom plotting implementation
#endif

// ECScope includes (audio dependencies commented out for compilation)
// #include "../audio/audio_system.hpp"
// #include "../audio/audio_3d.hpp"
// #include "../audio/spatial_audio.hpp"
// #include "../audio/audio_effects.hpp"
// #include "../audio/hrtf_processor.hpp"
// #include "../audio/ambisonics.hpp"
// #include "../audio/audio_raytracing.hpp"
// #include "../math.hpp"
#include "dashboard.hpp"
// #include "gui_widgets.hpp"

namespace ecscope::gui {

// =============================================================================
// FORWARD DECLARATIONS & MINIMAL TYPES
// =============================================================================

// Minimal Vector3f for compilation without math library
struct Vector3f {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    Vector3f() = default;
    Vector3f(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
};

// Forward declarations for audio system types
namespace audio {
    class AudioSystem;
    struct AudioMetrics {};
}

class Audio3DVisualizer;
class AudioSpectrumAnalyzer;
class AudioWaveformDisplay;
class HRTFVisualizer;
class EffectsChainEditor;
class SpatialAudioController;
class AudioPerformanceMonitor;

// =============================================================================
// CORE TYPES & ENUMERATIONS
// =============================================================================

/**
 * @brief Audio UI display modes
 */
enum class AudioDisplayMode : uint8_t {
    Overview,           ///< General overview with all components
    Sources3D,          ///< 3D audio sources visualization
    Listener,           ///< Listener and HRTF controls
    Effects,            ///< Effects chain editing
    Spatial,            ///< Spatial audio controls
    Performance,        ///< Performance monitoring
    Debugging          ///< Debug and diagnostic tools
};

/**
 * @brief 3D visualization rendering modes
 */
enum class Audio3DRenderMode : uint8_t {
    Wireframe,         ///< Wireframe representation
    Solid,             ///< Solid 3D objects
    Transparent,       ///< Semi-transparent objects
    Heatmap           ///< Audio intensity heatmap
};

/**
 * @brief Audio source visual representation
 */
struct AudioSourceVisual {
    uint32_t source_id = 0;
    Vector3f position{0.0f, 0.0f, 0.0f};
    Vector3f velocity{0.0f, 0.0f, 0.0f};
    Vector3f direction{0.0f, 0.0f, 1.0f};
    
    // Visual properties
    float radius = 1.0f;
    float intensity = 1.0f;
    ImU32 color = IM_COL32(255, 100, 100, 255);
    bool is_playing = false;
    bool is_selected = false;
    bool show_attenuation_sphere = true;
    bool show_cone = false;
    
    // Audio properties for visualization
    float volume = 1.0f;
    float pitch = 1.0f;
    float min_distance = 1.0f;
    float max_distance = 100.0f;
    float cone_inner_angle = 360.0f;
    float cone_outer_angle = 360.0f;
    
    // Animation state
    std::chrono::steady_clock::time_point last_update;
    Vector3f animated_position{0.0f, 0.0f, 0.0f};
};

/**
 * @brief Audio listener visual representation
 */
struct AudioListenerVisual {
    uint32_t listener_id = 0;
    Vector3f position{0.0f, 0.0f, 0.0f};
    Vector3f forward{0.0f, 0.0f, -1.0f};
    Vector3f up{0.0f, 1.0f, 0.0f};
    Vector3f velocity{0.0f, 0.0f, 0.0f};
    
    // Visual properties
    float head_size = 0.5f;
    ImU32 color = IM_COL32(100, 255, 100, 255);
    bool is_active = false;
    bool show_orientation = true;
    bool show_hrtf_pattern = false;
    
    // HRTF visualization
    std::vector<float> hrtf_pattern_left;
    std::vector<float> hrtf_pattern_right;
};

/**
 * @brief Reverb zone visual representation
 */
struct ReverbZoneVisual {
    uint32_t zone_id = 0;
    Vector3f center{0.0f, 0.0f, 0.0f};
    Vector3f size{10.0f, 10.0f, 10.0f};
    float reverb_level = 0.5f;
    float damping = 0.1f;
    ImU32 color = IM_COL32(100, 100, 255, 100);
    bool is_selected = false;
};

/**
 * @brief Audio ray tracing visualization data
 */
struct AudioRayVisual {
    Vector3f start;
    Vector3f end;
    Vector3f reflection_point;
    int bounce_count = 0;
    float intensity = 1.0f;
    ImU32 color = IM_COL32(255, 255, 0, 200);
    bool is_occluded = false;
};

/**
 * @brief Audio spectrum analysis data
 */
struct AudioSpectrumData {
    std::vector<float> frequencies;
    std::vector<float> magnitudes;
    std::vector<float> phases;
    std::chrono::steady_clock::time_point timestamp;
    float sample_rate = 48000.0f;
    uint32_t fft_size = 2048;
};

/**
 * @brief Audio waveform data
 */
struct AudioWaveformData {
    std::vector<float> samples_left;
    std::vector<float> samples_right;
    std::chrono::steady_clock::time_point timestamp;
    float sample_rate = 48000.0f;
    float duration_seconds = 1.0f;
};

/**
 * @brief Audio effects visualization data
 */
struct EffectVisualizationData {
    std::string effect_name;
    std::unordered_map<std::string, float> parameters;
    AudioSpectrumData input_spectrum;
    AudioSpectrumData output_spectrum;
    bool is_enabled = true;
    bool is_bypassed = false;
};

// =============================================================================
// MAIN AUDIO UI CLASS
// =============================================================================

/**
 * @brief Comprehensive Audio System UI
 * 
 * Professional audio interface providing:
 * - Real-time 3D visualization of audio sources and listeners
 * - HRTF processing visualization with head tracking
 * - Sound propagation and attenuation visualization
 * - Audio effects chain editing with real-time preview
 * - Spatial audio controls and environmental presets
 * - Performance monitoring and debugging tools
 */
class AudioSystemUI {
public:
    AudioSystemUI();
    ~AudioSystemUI();

    // =============================================================================
    // INITIALIZATION & LIFECYCLE
    // =============================================================================

    /**
     * @brief Initialize the audio UI system
     * @param audio_system Reference to the main audio system
     * @param dashboard Reference to the main dashboard
     * @return true on success
     */
    bool initialize(audio::AudioSystem* audio_system, Dashboard* dashboard);

    /**
     * @brief Shutdown and cleanup resources
     */
    void shutdown();

    /**
     * @brief Check if UI is properly initialized
     */
    bool is_initialized() const { return initialized_; }

    // =============================================================================
    // MAIN RENDER LOOP
    // =============================================================================

    /**
     * @brief Render the main audio UI
     * Called once per frame from the main application loop
     */
    void render();

    /**
     * @brief Update audio UI state and visualizations
     * @param delta_time Time since last update
     */
    void update(float delta_time);

    // =============================================================================
    // DISPLAY MODE MANAGEMENT
    // =============================================================================

    /**
     * @brief Set the current display mode
     */
    void set_display_mode(AudioDisplayMode mode);

    /**
     * @brief Get current display mode
     */
    AudioDisplayMode get_display_mode() const { return current_mode_; }

    // =============================================================================
    // 3D VISUALIZATION CONTROLS
    // =============================================================================

    /**
     * @brief Set 3D rendering mode
     */
    void set_3d_render_mode(Audio3DRenderMode mode);

    /**
     * @brief Enable/disable specific visualization elements
     */
    void enable_source_visualization(bool enable) { show_sources_ = enable; }
    void enable_listener_visualization(bool enable) { show_listeners_ = enable; }
    void enable_reverb_zones(bool enable) { show_reverb_zones_ = enable; }
    void enable_audio_rays(bool enable) { show_audio_rays_ = enable; }
    void enable_doppler_visualization(bool enable) { show_doppler_ = enable; }

    // =============================================================================
    // AUDIO SOURCE MANAGEMENT
    // =============================================================================

    /**
     * @brief Select an audio source for editing
     */
    void select_audio_source(uint32_t source_id);

    /**
     * @brief Get currently selected source
     */
    uint32_t get_selected_source() const { return selected_source_id_; }

    /**
     * @brief Register audio source for visualization
     */
    void register_audio_source(uint32_t source_id, const AudioSourceVisual& visual);

    /**
     * @brief Update audio source visual properties
     */
    void update_source_visual(uint32_t source_id, const AudioSourceVisual& visual);

    /**
     * @brief Remove audio source from visualization
     */
    void unregister_audio_source(uint32_t source_id);

    // =============================================================================
    // LISTENER MANAGEMENT
    // =============================================================================

    /**
     * @brief Register audio listener for visualization
     */
    void register_audio_listener(uint32_t listener_id, const AudioListenerVisual& visual);

    /**
     * @brief Update listener visual properties
     */
    void update_listener_visual(uint32_t listener_id, const AudioListenerVisual& visual);

    /**
     * @brief Set active listener
     */
    void set_active_listener(uint32_t listener_id);

    // =============================================================================
    // EFFECTS CHAIN MANAGEMENT
    // =============================================================================

    /**
     * @brief Get effects chain editor
     */
    EffectsChainEditor* get_effects_editor() { return effects_editor_.get(); }

    /**
     * @brief Add effect visualization data
     */
    void add_effect_visualization(const EffectVisualizationData& data);

    // =============================================================================
    // PERFORMANCE MONITORING
    // =============================================================================

    /**
     * @brief Update audio performance metrics
     */
    void update_performance_metrics(const audio::AudioMetrics& metrics);

    /**
     * @brief Enable/disable performance monitoring
     */
    void enable_performance_monitoring(bool enable) { performance_monitoring_enabled_ = enable; }

    // =============================================================================
    // VISUALIZATION DATA
    // =============================================================================

    /**
     * @brief Update spectrum analysis data
     */
    void update_spectrum_data(uint32_t source_id, const AudioSpectrumData& data);

    /**
     * @brief Update waveform data
     */
    void update_waveform_data(uint32_t source_id, const AudioWaveformData& data);

    /**
     * @brief Add audio ray visualization
     */
    void add_audio_ray(const AudioRayVisual& ray);

    /**
     * @brief Clear all audio rays
     */
    void clear_audio_rays();

    // =============================================================================
    // CONFIGURATION
    // =============================================================================

    /**
     * @brief Save UI configuration
     */
    bool save_config(const std::string& filepath) const;

    /**
     * @brief Load UI configuration
     */
    bool load_config(const std::string& filepath);

    /**
     * @brief Reset to default settings
     */
    void reset_to_defaults();

private:
    // =============================================================================
    // PRIVATE RENDERING METHODS
    // =============================================================================

    void render_main_controls();
    void render_3d_viewport();
    void render_source_inspector();
    void render_listener_controls();
    void render_effects_panel();
    void render_spatial_controls();
    void render_performance_panel();
    void render_debug_panel();

    // 3D visualization rendering
    void render_3d_sources();
    void render_3d_listeners();
    void render_reverb_zones();
    void render_audio_rays();
    void render_hrtf_visualization();
    void render_doppler_effects();
    void render_attenuation_spheres();
    void render_coordinate_system();

    // 2D visualization rendering  
    void render_spectrum_analyzer();
    void render_waveform_display();
    void render_level_meters();
    void render_effects_chain();

    // Control panels
    void render_source_properties_panel();
    void render_3d_positioning_controls();
    void render_audio_clip_loader();
    void render_distance_attenuation_editor();
    void render_environmental_presets();
    void render_ambisonics_controls();
    void render_streaming_controls();

    // =============================================================================
    // PRIVATE UTILITY METHODS
    // =============================================================================

    void update_3d_visualizations();
    void update_audio_analysis();
    void calculate_audio_rays();
    void process_hrtf_visualization();
    void handle_3d_viewport_input();
    
    // Math utilities for 3D rendering
    Vector3f world_to_screen(const Vector3f& world_pos) const;
    Vector3f screen_to_world(const ImVec2& screen_pos) const;
    bool is_point_in_view_frustum(const Vector3f& point) const;
    
    // Visualization helpers
    void draw_3d_sphere(const Vector3f& center, float radius, ImU32 color, bool wireframe = false);
    void draw_3d_cone(const Vector3f& apex, const Vector3f& direction, float angle, float height, ImU32 color);
    void draw_3d_line(const Vector3f& start, const Vector3f& end, ImU32 color, float thickness = 1.0f);
    void draw_3d_arrow(const Vector3f& start, const Vector3f& end, ImU32 color, float thickness = 1.0f);
    void draw_hrtf_pattern(const Vector3f& listener_pos, const std::vector<float>& pattern, ImU32 color);
    
    // Audio analysis helpers
    void perform_fft_analysis(const std::vector<float>& samples, AudioSpectrumData& output);
    void calculate_audio_levels(const std::vector<float>& samples, float& rms, float& peak);
    void analyze_doppler_effect(uint32_t source_id);

    // =============================================================================
    // MEMBER VARIABLES
    // =============================================================================

    // Core state
    bool initialized_ = false;
    AudioDisplayMode current_mode_ = AudioDisplayMode::Overview;
    Audio3DRenderMode render_mode_ = Audio3DRenderMode::Solid;
    
    // System references
    audio::AudioSystem* audio_system_ = nullptr;
    Dashboard* dashboard_ = nullptr;
    
    // Visualization components
    std::unique_ptr<Audio3DVisualizer> visualizer_3d_;
    std::unique_ptr<AudioSpectrumAnalyzer> spectrum_analyzer_;
    std::unique_ptr<AudioWaveformDisplay> waveform_display_;
    std::unique_ptr<HRTFVisualizer> hrtf_visualizer_;
    std::unique_ptr<EffectsChainEditor> effects_editor_;
    std::unique_ptr<SpatialAudioController> spatial_controller_;
    std::unique_ptr<AudioPerformanceMonitor> performance_monitor_;
    
    // Visual data storage
    std::unordered_map<uint32_t, AudioSourceVisual> source_visuals_;
    std::unordered_map<uint32_t, AudioListenerVisual> listener_visuals_;
    std::unordered_map<uint32_t, ReverbZoneVisual> reverb_zones_;
    std::vector<AudioRayVisual> audio_rays_;
    
    // Analysis data storage
    std::unordered_map<uint32_t, AudioSpectrumData> spectrum_data_;
    std::unordered_map<uint32_t, AudioWaveformData> waveform_data_;
    std::vector<EffectVisualizationData> effect_visualizations_;
    
    // UI state
    uint32_t selected_source_id_ = 0;
    uint32_t active_listener_id_ = 0;
    bool show_sources_ = true;
    bool show_listeners_ = true;
    bool show_reverb_zones_ = true;
    bool show_audio_rays_ = false;
    bool show_doppler_ = true;
    bool performance_monitoring_enabled_ = true;
    
    // 3D viewport state
    Vector3f camera_position_{0.0f, 5.0f, 10.0f};
    Vector3f camera_target_{0.0f, 0.0f, 0.0f};
    Vector3f camera_up_{0.0f, 1.0f, 0.0f};
    float camera_fov_ = 45.0f;
    float camera_near_ = 0.1f;
    float camera_far_ = 1000.0f;
    
    // Interaction state
    bool dragging_3d_object_ = false;
    uint32_t dragged_object_id_ = 0;
    ImVec2 last_mouse_pos_{0.0f, 0.0f};
    
    // Animation state
    std::chrono::steady_clock::time_point last_update_time_;
    float animation_time_ = 0.0f;
    
    // Configuration
    std::string config_filepath_ = "ecscope_audio_ui.ini";
    
    // Thread safety
    mutable std::mutex data_mutex_;
    std::atomic<bool> update_pending_{false};
};

// =============================================================================
// SPECIALIZED VISUALIZATION COMPONENTS
// =============================================================================

/**
 * @brief 3D Audio Visualizer Component
 */
class Audio3DVisualizer {
public:
    Audio3DVisualizer();
    ~Audio3DVisualizer();
    
    bool initialize();
    void render(const std::unordered_map<uint32_t, AudioSourceVisual>& sources,
                const std::unordered_map<uint32_t, AudioListenerVisual>& listeners,
                const Vector3f& camera_pos, const Vector3f& camera_target);
    void set_render_mode(Audio3DRenderMode mode) { render_mode_ = mode; }
    void enable_grid(bool enable) { show_grid_ = enable; }
    void set_grid_size(float size) { grid_size_ = size; }

private:
    Audio3DRenderMode render_mode_ = Audio3DRenderMode::Solid;
    bool show_grid_ = true;
    float grid_size_ = 10.0f;
    ImDrawList* draw_list_ = nullptr;
};

/**
 * @brief Audio Spectrum Analyzer Component
 */
class AudioSpectrumAnalyzer {
public:
    AudioSpectrumAnalyzer();
    ~AudioSpectrumAnalyzer();
    
    void render(const AudioSpectrumData& data);
    void set_frequency_range(float min_freq, float max_freq);
    void set_magnitude_range(float min_db, float max_db);
    void enable_logarithmic_frequency(bool enable) { log_frequency_ = enable; }

private:
    float min_frequency_ = 20.0f;
    float max_frequency_ = 20000.0f;
    float min_magnitude_db_ = -60.0f;
    float max_magnitude_db_ = 0.0f;
    bool log_frequency_ = true;
    std::vector<float> smoothed_magnitudes_;
    float smoothing_factor_ = 0.3f;
};

/**
 * @brief Audio Waveform Display Component
 */
class AudioWaveformDisplay {
public:
    AudioWaveformDisplay();
    ~AudioWaveformDisplay();
    
    void render(const AudioWaveformData& data);
    void set_time_range(float duration) { time_range_ = duration; }
    void enable_stereo_display(bool enable) { stereo_display_ = enable; }

private:
    float time_range_ = 1.0f;  // seconds
    bool stereo_display_ = true;
    ImVec2 display_size_{400.0f, 200.0f};
};

/**
 * @brief HRTF Visualization Component
 */
class HRTFVisualizer {
public:
    HRTFVisualizer();
    ~HRTFVisualizer();
    
    void render(const AudioListenerVisual& listener);
    void update_hrtf_data(const std::vector<float>& left_pattern, 
                         const std::vector<float>& right_pattern);

private:
    std::vector<float> hrtf_left_;
    std::vector<float> hrtf_right_;
    float visualization_scale_ = 1.0f;
};

/**
 * @brief Effects Chain Editor Component
 */
class EffectsChainEditor {
public:
    EffectsChainEditor();
    ~EffectsChainEditor();
    
    void render();
    void add_effect_slot(const std::string& effect_name);
    void remove_effect_slot(size_t index);
    void reorder_effects(size_t from_index, size_t to_index);

private:
    std::vector<std::string> effect_chain_;
    std::unordered_map<std::string, std::unordered_map<std::string, float>> effect_parameters_;
    size_t selected_effect_ = 0;
};

/**
 * @brief Spatial Audio Controller Component
 */
class SpatialAudioController {
public:
    SpatialAudioController();
    ~SpatialAudioController();
    
    void render();
    void set_audio_system(audio::AudioSystem* system) { audio_system_ = system; }

private:
    audio::AudioSystem* audio_system_ = nullptr;
    bool ambisonics_enabled_ = false;
    uint32_t ambisonics_order_ = 1;
    bool ray_tracing_enabled_ = false;
    int ray_tracing_quality_ = 5;
};

/**
 * @brief Audio Performance Monitor Component
 */
class AudioPerformanceMonitor {
public:
    AudioPerformanceMonitor();
    ~AudioPerformanceMonitor();
    
    void render();
    void update_metrics(const audio::AudioMetrics& metrics);

private:
    std::queue<float> cpu_usage_history_;
    std::queue<float> latency_history_;
    std::queue<uint32_t> voice_count_history_;
    static constexpr size_t MAX_HISTORY_SIZE = 300;  // 5 seconds at 60fps
};

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

/**
 * @brief Convert frequency to display position
 */
float frequency_to_x_position(float frequency, float min_freq, float max_freq, 
                             float display_width, bool logarithmic = true);

/**
 * @brief Convert magnitude to display position
 */
float magnitude_to_y_position(float magnitude_db, float min_db, float max_db, float display_height);

/**
 * @brief Format frequency for display
 */
std::string format_frequency(float frequency);

/**
 * @brief Format audio time for display
 */
std::string format_audio_time(float seconds);

/**
 * @brief Format decibel value for display
 */
std::string format_decibels(float db);

/**
 * @brief Create color from audio intensity
 */
ImU32 intensity_to_color(float intensity, float max_intensity = 1.0f);

/**
 * @brief Create color gradient for audio visualization
 */
ImU32 audio_gradient_color(float value, float min_val, float max_val);

} // namespace ecscope::gui