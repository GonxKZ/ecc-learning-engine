/**
 * @file spatial_audio_ui.hpp
 * @brief Spatial Audio Controls and Ambisonics Interface
 * 
 * Professional spatial audio interface featuring:
 * - Ambisonics visualization and control interface
 * - Multi-listener setup for split-screen scenarios
 * - Audio occlusion and obstruction controls
 * - Environmental audio presets and reverb zones
 * - Sound cone visualization for directional audio
 * - Audio streaming quality and buffering controls
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

#ifdef ECSCOPE_HAS_IMGUI
#include <imgui.h>
#include <imgui_internal.h>
#endif

#include "../audio/ambisonics.hpp"
#include "../audio/spatial_audio.hpp"
#include "../audio/audio_3d.hpp"
#include "../audio/audio_raytracing.hpp"
#include "../core/math.hpp"

namespace ecscope::gui {

// =============================================================================
// SPATIAL AUDIO TYPES
// =============================================================================

/**
 * @brief Environmental audio preset
 */
struct EnvironmentalPreset {
    std::string name;
    std::string description;
    std::string category; // "Indoor", "Outdoor", "Underwater", "Space", etc.
    
    // Reverb parameters
    float room_size = 0.5f;
    float damping = 0.1f;
    float wet_level = 0.3f;
    float dry_level = 0.7f;
    float pre_delay = 0.02f;
    float decay_time = 1.5f;
    
    // Environmental effects
    float air_absorption = 0.1f;
    float distance_factor = 1.0f;
    float doppler_factor = 1.0f;
    float speed_of_sound = 343.3f; // m/s
    
    // Occlusion/obstruction
    float occlusion_strength = 1.0f;
    float obstruction_strength = 1.0f;
    float transmission_factor = 0.1f;
    
    // Atmospheric effects
    Vector3f wind_direction{0.0f, 0.0f, 0.0f};
    float wind_strength = 0.0f;
    float temperature = 20.0f; // Celsius
    float humidity = 50.0f;    // Percentage
    
    bool is_factory_preset = false;
    bool is_favorite = false;
};

/**
 * @brief Ambisonics channel configuration
 */
struct AmbisonicsConfiguration {
    uint32_t order = 1; // 1st order = 4 channels, 2nd order = 9 channels, etc.
    audio::AmbisonicsProcessor::AmbisonicOrder ambisonics_order;
    
    // Channel mapping
    std::vector<std::string> channel_names;
    std::vector<Vector3f> speaker_positions;
    
    // Decoding parameters
    float max_re_weight = 1.0f;
    float in_phase_weight = 1.0f;
    bool use_dual_band_decoding = false;
    float crossover_frequency = 700.0f;
    
    // Head tracking
    bool head_tracking_enabled = false;
    Vector3f head_orientation{0.0f, 0.0f, 0.0f}; // Euler angles
    Vector3f head_position{0.0f, 0.0f, 0.0f};
    
    // Visualization
    bool show_spherical_harmonics = false;
    bool show_directivity_pattern = true;
    float visualization_scale = 1.0f;
};

/**
 * @brief Sound cone configuration
 */
struct SoundConeConfig {
    float inner_angle = 360.0f; // degrees
    float outer_angle = 360.0f; // degrees
    float outer_gain = 0.0f;    // linear gain
    Vector3f direction{0.0f, 0.0f, -1.0f}; // normalized direction vector
    bool enabled = false;
    
    // Visualization
    ImU32 inner_cone_color = IM_COL32(255, 255, 0, 128);
    ImU32 outer_cone_color = IM_COL32(255, 128, 0, 64);
    bool show_cone_visualization = true;
};

/**
 * @brief Audio streaming configuration
 */
struct AudioStreamingConfig {
    // Buffer settings
    uint32_t buffer_size = 1024;
    uint32_t num_buffers = 4;
    float target_latency_ms = 20.0f;
    
    // Quality settings
    uint32_t sample_rate = 48000;
    uint32_t bit_depth = 24;
    uint32_t channels = 2;
    
    // Streaming options
    bool enable_adaptive_quality = true;
    bool enable_buffer_prediction = true;
    bool enable_dropouts_recovery = true;
    float quality_adaptation_threshold = 10.0f; // ms
    
    // Network streaming (if applicable)
    uint32_t network_buffer_size = 8192;
    float network_timeout_ms = 5000.0f;
    bool enable_compression = true;
    std::string compression_codec = "opus";
    
    // Performance monitoring
    bool monitor_performance = true;
    float underrun_threshold = 5.0f; // percentage
    float overrun_threshold = 90.0f; // percentage
};

/**
 * @brief Multi-listener configuration
 */
struct MultiListenerConfig {
    struct ListenerSetup {
        uint32_t listener_id;
        std::string name;
        Vector3f position{0.0f, 0.0f, 0.0f};
        Vector3f forward{0.0f, 0.0f, -1.0f};
        Vector3f up{0.0f, 1.0f, 0.0f};
        Vector3f velocity{0.0f, 0.0f, 0.0f};
        bool is_active = true;
        float volume_multiplier = 1.0f;
        ImU32 visualization_color = IM_COL32(100, 255, 100, 255);
    };
    
    std::vector<ListenerSetup> listeners;
    uint32_t active_listener_id = 0;
    
    // Split-screen configuration
    bool enable_split_screen = false;
    std::vector<ImVec2> viewport_sizes;
    std::vector<ImVec2> viewport_positions;
    
    // Audio mixing
    bool enable_listener_mixing = false;
    float crossfade_time = 0.5f; // seconds
    
    // Performance
    bool enable_lod_per_listener = true;
    std::vector<float> lod_distances;
};

// =============================================================================
// SPATIAL AUDIO CONTROLLER CLASS
// =============================================================================

/**
 * @brief Comprehensive Spatial Audio Controller
 * 
 * Provides interface for:
 * - Ambisonics encoding/decoding controls
 * - Environmental audio presets
 * - Multi-listener management
 * - Audio ray tracing visualization
 * - Streaming quality controls
 * - Performance optimization
 */
class SpatialAudioController {
public:
    SpatialAudioController();
    ~SpatialAudioController();

    // =============================================================================
    // INITIALIZATION & LIFECYCLE
    // =============================================================================

    /**
     * @brief Initialize spatial audio controller
     * @param audio_system Reference to main audio system
     * @return true on success
     */
    bool initialize(audio::AudioSystem* audio_system);

    /**
     * @brief Shutdown controller
     */
    void shutdown();

    /**
     * @brief Check if controller is initialized
     */
    bool is_initialized() const { return initialized_; }

    // =============================================================================
    // MAIN INTERFACE
    // =============================================================================

    /**
     * @brief Render spatial audio controls
     */
    void render();

    /**
     * @brief Update controller state
     * @param delta_time Time since last update
     */
    void update(float delta_time);

    // =============================================================================
    // AMBISONICS CONTROL
    // =============================================================================

    /**
     * @brief Enable/disable ambisonics processing
     */
    void enable_ambisonics(bool enable);

    /**
     * @brief Set ambisonics order
     */
    void set_ambisonics_order(uint32_t order);

    /**
     * @brief Configure ambisonics settings
     */
    void set_ambisonics_config(const AmbisonicsConfiguration& config);

    /**
     * @brief Get current ambisonics configuration
     */
    const AmbisonicsConfiguration& get_ambisonics_config() const { return ambisonics_config_; }

    /**
     * @brief Update head tracking data
     */
    void update_head_tracking(const Vector3f& position, const Vector3f& orientation);

    // =============================================================================
    // ENVIRONMENTAL PRESETS
    // =============================================================================

    /**
     * @brief Apply environmental preset
     */
    void apply_environmental_preset(const std::string& preset_name);

    /**
     * @brief Create custom environmental preset
     */
    bool create_environmental_preset(const std::string& name, const EnvironmentalPreset& preset);

    /**
     * @brief Remove environmental preset
     */
    void remove_environmental_preset(const std::string& preset_name);

    /**
     * @brief Get all available environmental presets
     */
    std::vector<std::string> get_environmental_presets() const;

    /**
     * @brief Import environmental presets from file
     */
    bool import_environmental_presets(const std::string& filepath);

    /**
     * @brief Export environmental presets to file
     */
    bool export_environmental_presets(const std::string& filepath) const;

    // =============================================================================
    // MULTI-LISTENER MANAGEMENT
    // =============================================================================

    /**
     * @brief Configure multi-listener setup
     */
    void set_multi_listener_config(const MultiListenerConfig& config);

    /**
     * @brief Add listener to multi-listener setup
     */
    uint32_t add_listener(const Vector3f& position, const Vector3f& orientation);

    /**
     * @brief Remove listener from setup
     */
    void remove_listener(uint32_t listener_id);

    /**
     * @brief Set active listener
     */
    void set_active_listener(uint32_t listener_id);

    /**
     * @brief Enable/disable split-screen mode
     */
    void enable_split_screen(bool enable);

    // =============================================================================
    // AUDIO RAY TRACING
    // =============================================================================

    /**
     * @brief Enable/disable audio ray tracing
     */
    void enable_ray_tracing(bool enable);

    /**
     * @brief Set ray tracing quality (1-10)
     */
    void set_ray_tracing_quality(int quality);

    /**
     * @brief Configure ray tracing parameters
     */
    void set_ray_tracing_params(uint32_t max_rays, float max_distance, uint32_t max_bounces);

    /**
     * @brief Get ray tracing performance metrics
     */
    float get_ray_tracing_performance() const;

    // =============================================================================
    // STREAMING CONTROLS
    // =============================================================================

    /**
     * @brief Configure audio streaming settings
     */
    void set_streaming_config(const AudioStreamingConfig& config);

    /**
     * @brief Get streaming performance metrics
     */
    struct StreamingMetrics {
        float buffer_fill_percentage = 0.0f;
        uint32_t buffer_underruns = 0;
        uint32_t buffer_overruns = 0;
        float current_latency_ms = 0.0f;
        float average_latency_ms = 0.0f;
        float current_quality_factor = 1.0f;
    };
    StreamingMetrics get_streaming_metrics() const;

    // =============================================================================
    // OCCLUSION & OBSTRUCTION
    // =============================================================================

    /**
     * @brief Configure occlusion processing
     */
    void configure_occlusion(bool enable, float strength, uint32_t ray_count);

    /**
     * @brief Add occlusion geometry
     */
    void add_occlusion_geometry(const std::vector<Vector3f>& geometry);

    /**
     * @brief Clear occlusion geometry
     */
    void clear_occlusion_geometry();

    // =============================================================================
    // SOUND CONE VISUALIZATION
    // =============================================================================

    /**
     * @brief Configure sound cone for source
     */
    void configure_sound_cone(uint32_t source_id, const SoundConeConfig& config);

    /**
     * @brief Enable/disable cone visualization
     */
    void enable_cone_visualization(bool enable) { show_sound_cones_ = enable; }

private:
    // =============================================================================
    // PRIVATE RENDERING METHODS
    // =============================================================================

    void render_main_controls();
    void render_ambisonics_panel();
    void render_environmental_panel();
    void render_multi_listener_panel();
    void render_ray_tracing_panel();
    void render_streaming_panel();
    void render_occlusion_panel();
    void render_cone_editor_panel();

    // Ambisonics visualization
    void render_ambisonics_sphere();
    void render_spherical_harmonics();
    void render_directivity_pattern();
    void render_ambisonics_meters();

    // Environmental controls
    void render_preset_selector();
    void render_reverb_controls();
    void render_atmospheric_controls();
    void render_environmental_visualizer();

    // Multi-listener interface
    void render_listener_list();
    void render_listener_properties();
    void render_split_screen_config();
    void render_listener_visualization();

    // Ray tracing visualization
    void render_ray_tracing_settings();
    void render_acoustic_analysis();
    void render_impulse_response();
    void render_ray_path_visualization();

    // Streaming interface
    void render_buffer_status();
    void render_quality_controls();
    void render_performance_metrics();
    void render_network_settings();

    // =============================================================================
    // PRIVATE UTILITY METHODS
    // =============================================================================

    void initialize_factory_presets();
    void update_ambisonics_processing();
    void update_environmental_effects();
    void update_ray_tracing();
    void update_streaming_quality();
    
    // Ambisonics utilities
    void calculate_spherical_harmonics();
    void update_speaker_configuration();
    Vector3f spherical_to_cartesian(float azimuth, float elevation, float radius = 1.0f);
    
    // Environmental utilities
    void apply_preset_to_audio_system(const EnvironmentalPreset& preset);
    void interpolate_presets(const EnvironmentalPreset& preset_a, 
                           const EnvironmentalPreset& preset_b, float factor);
    
    // Visualization utilities
    void draw_ambisonics_sphere(const Vector3f& center, float radius);
    void draw_directivity_balloon(const Vector3f& center, const std::vector<float>& pattern);
    void draw_sound_cone(const Vector3f& position, const SoundConeConfig& config);
    void draw_ray_paths(const std::vector<Vector3f>& path_points);
    
    // Performance optimization
    void optimize_processing_for_quality();
    void adjust_buffer_sizes_for_latency();
    bool should_reduce_quality() const;

    // =============================================================================
    // MEMBER VARIABLES
    // =============================================================================

    // Core state
    bool initialized_ = false;
    audio::AudioSystem* audio_system_ = nullptr;
    
    // Configuration objects
    AmbisonicsConfiguration ambisonics_config_;
    MultiListenerConfig multi_listener_config_;
    AudioStreamingConfig streaming_config_;
    
    // Environmental presets
    std::unordered_map<std::string, EnvironmentalPreset> environmental_presets_;
    std::string current_preset_name_ = "Default";
    std::string preset_search_filter_;
    
    // Sound cone configurations
    std::unordered_map<uint32_t, SoundConeConfig> sound_cone_configs_;
    bool show_sound_cones_ = true;
    
    // UI state
    enum class PanelMode {
        Overview,
        Ambisonics,
        Environmental,
        MultiListener,
        RayTracing,
        Streaming,
        Occlusion
    } current_panel_ = PanelMode::Overview;
    
    bool show_ambisonics_sphere_ = true;
    bool show_spherical_harmonics_ = false;
    bool show_directivity_pattern_ = true;
    bool show_environmental_visualizer_ = true;
    bool show_ray_paths_ = false;
    
    // Processing state
    bool ambisonics_enabled_ = false;
    bool ray_tracing_enabled_ = false;
    int ray_tracing_quality_ = 5;
    bool occlusion_enabled_ = true;
    
    // Performance monitoring
    std::vector<float> processing_time_history_;
    std::vector<float> quality_history_;
    std::vector<float> latency_history_;
    static constexpr size_t MAX_HISTORY_SIZE = 300;
    
    // Ambisonics visualization data
    std::vector<std::vector<float>> spherical_harmonics_data_;
    std::vector<float> directivity_pattern_;
    std::vector<ImVec2> ambisonics_sphere_points_;
    
    // Ray tracing data
    std::vector<std::vector<Vector3f>> ray_paths_;
    std::vector<float> impulse_response_;
    std::vector<float> acoustic_analysis_data_;
    
    // Threading and synchronization
    mutable std::mutex config_mutex_;
    mutable std::mutex preset_mutex_;
    mutable std::mutex visualization_mutex_;
    
    // Configuration file paths
    std::string presets_file_path_ = "environmental_presets.json";
    std::string config_file_path_ = "spatial_audio_config.json";
};

// =============================================================================
// SPECIALIZED VISUALIZATION WIDGETS
// =============================================================================

/**
 * @brief Spherical Harmonics Visualizer
 */
class SphericalHarmonicsVisualizer {
public:
    SphericalHarmonicsVisualizer();
    ~SphericalHarmonicsVisualizer();
    
    void render(const std::vector<std::vector<float>>& harmonics_data, 
               uint32_t order, const ImVec2& size = ImVec2(300, 300));
    void set_visualization_mode(bool wireframe) { wireframe_mode_ = wireframe; }
    
private:
    bool wireframe_mode_ = false;
    std::vector<ImVec2> sphere_vertices_;
    std::vector<std::array<int, 3>> sphere_triangles_;
    void generate_sphere_mesh();
};

/**
 * @brief Directivity Pattern Editor
 */
class DirectivityPatternEditor {
public:
    DirectivityPatternEditor();
    ~DirectivityPatternEditor();
    
    void render(std::vector<float>& pattern_data, const ImVec2& size = ImVec2(250, 250));
    void set_edit_mode(bool enabled) { edit_mode_enabled_ = enabled; }
    
private:
    bool edit_mode_enabled_ = false;
    int selected_point_ = -1;
    bool dragging_ = false;
};

/**
 * @brief Environmental Reverb Visualizer
 */
class EnvironmentalReverbVisualizer {
public:
    EnvironmentalReverbVisualizer();
    ~EnvironmentalReverbVisualizer();
    
    void render(const EnvironmentalPreset& preset, const ImVec2& size = ImVec2(400, 200));
    void update_impulse_response(const std::vector<float>& impulse_data);
    
private:
    std::vector<float> current_impulse_response_;
    float visualization_time_ = 0.0f;
    bool show_frequency_response_ = true;
};

/**
 * @brief Multi-Listener Split Screen Controller
 */
class SplitScreenController {
public:
    SplitScreenController();
    ~SplitScreenController();
    
    void render(MultiListenerConfig& config, const ImVec2& available_size);
    void handle_viewport_resizing();
    
private:
    bool resizing_viewport_ = false;
    int resizing_listener_id_ = -1;
    ImVec2 resize_start_pos_{0.0f, 0.0f};
};

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

/**
 * @brief Create default environmental presets
 */
std::vector<EnvironmentalPreset> create_factory_environmental_presets();

/**
 * @brief Calculate ambisonics channel count for given order
 */
uint32_t calculate_ambisonics_channel_count(uint32_t order);

/**
 * @brief Convert spherical coordinates to cartesian
 */
Vector3f spherical_to_cartesian(float azimuth_deg, float elevation_deg, float radius = 1.0f);

/**
 * @brief Convert cartesian coordinates to spherical
 */
void cartesian_to_spherical(const Vector3f& cartesian, float& azimuth_deg, 
                           float& elevation_deg, float& radius);

/**
 * @brief Calculate speaker configuration for ambisonics order
 */
std::vector<Vector3f> calculate_optimal_speaker_positions(uint32_t ambisonics_order);

/**
 * @brief Format latency for display
 */
std::string format_latency_display(float latency_ms);

/**
 * @brief Format buffer status for display
 */
std::string format_buffer_status(float fill_percentage);

} // namespace ecscope::gui