/**
 * @file rendering_ui.hpp
 * @brief Comprehensive Rendering System UI Interface
 * 
 * Professional-grade rendering pipeline control interface featuring real-time 
 * parameter adjustment, visual debugging tools, scene management, and performance 
 * optimization for the ECScope engine.
 * 
 * Features:
 * - Deferred rendering pipeline control with live preview
 * - PBR material editor with real-time updates
 * - Post-processing stack with HDR, bloom, tone mapping
 * - Shadow mapping controls and visualization
 * - G-Buffer visualization and render pass debugging
 * - GPU profiling and performance monitoring
 * - Scene hierarchy and object management
 * - Shader hot-reload and debugging interface
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#pragma once

#include "gui_core.hpp"
#include "gui_widgets.hpp"
#include "dashboard.hpp"
#include "../rendering/renderer.hpp"
#include "../rendering/deferred_renderer.hpp"
#include "../rendering/materials.hpp"
#include "../rendering/render_graph.hpp"

#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <array>
#include <optional>

namespace ecscope::gui {

// Forward declarations
class RenderingUI;

// =============================================================================
// RENDERING UI TYPES & ENUMERATIONS
// =============================================================================

/**
 * @brief Rendering debug visualization modes
 */
enum class DebugVisualizationMode : uint8_t {
    None,                    ///< No debug visualization
    GBufferAlbedo,          ///< G-Buffer albedo channel
    GBufferNormal,          ///< G-Buffer world normals
    GBufferDepth,           ///< Depth buffer visualization
    GBufferMaterial,        ///< Material properties
    GBufferMotion,          ///< Motion vectors
    LightComplexity,        ///< Light complexity heatmap
    Overdraw,               ///< Pixel overdraw visualization
    ShadowCascades,         ///< Shadow cascade visualization
    SSAO,                   ///< Screen-space ambient occlusion
    SSR,                    ///< Screen-space reflections
    Bloom,                  ///< Bloom effect preview
    Wireframe,              ///< Wireframe rendering mode
    LightBounds,            ///< Light volume bounds
    ClusterVisualization    ///< Tile/cluster debugging
};

/**
 * @brief Performance profiling target
 */
enum class ProfilingTarget : uint8_t {
    Overall,                ///< Overall frame profiling
    GeometryPass,           ///< G-Buffer generation
    ShadowPass,             ///< Shadow map rendering
    LightingPass,           ///< Deferred lighting
    PostProcessing,         ///< Post-processing effects
    GPUMemory,              ///< GPU memory usage
    DrawCalls,              ///< Draw call analysis
    TextureStreaming        ///< Texture streaming performance
};

/**
 * @brief Viewport camera control modes
 */
enum class CameraControlMode : uint8_t {
    Orbit,                  ///< Orbit around target
    Fly,                    ///< Free-fly camera
    FirstPerson,            ///< FPS-style controls
    Inspect                 ///< Object inspection mode
};

/**
 * @brief Shader hot-reload status
 */
enum class ShaderReloadStatus : uint8_t {
    Idle,                   ///< No reload in progress
    Reloading,              ///< Currently reloading
    Success,                ///< Successfully reloaded
    Error                   ///< Reload failed
};

// =============================================================================
// RENDERING CONFIGURATION STRUCTURES
// =============================================================================

/**
 * @brief Live rendering configuration that can be modified in real-time
 */
struct LiveRenderingConfig {
    // Deferred rendering settings
    rendering::DeferredConfig deferred_config;
    
    // Post-processing parameters
    struct PostProcessConfig {
        // HDR and tone mapping
        bool enable_hdr = true;
        float exposure = 1.0f;
        float gamma = 2.2f;
        int tone_mapping_mode = 0; // 0: Reinhard, 1: ACES, 2: Uncharted2
        
        // Bloom settings
        bool enable_bloom = true;
        float bloom_threshold = 1.0f;
        float bloom_intensity = 0.8f;
        float bloom_radius = 1.0f;
        int bloom_iterations = 6;
        
        // Screen-space effects
        bool enable_ssao = true;
        float ssao_radius = 0.5f;
        float ssao_intensity = 1.0f;
        int ssao_samples = 16;
        
        bool enable_ssr = true;
        float ssr_max_distance = 50.0f;
        float ssr_fade_distance = 10.0f;
        int ssr_max_steps = 64;
        float ssr_thickness = 0.1f;
        
        // Temporal anti-aliasing
        bool enable_taa = true;
        float taa_feedback = 0.9f;
        bool taa_sharpening = true;
        float taa_sharpening_amount = 0.5f;
        
        // Motion blur
        bool enable_motion_blur = false;
        float motion_blur_strength = 1.0f;
        int motion_blur_samples = 8;
    } post_process;
    
    // Shadow settings
    struct ShadowConfig {
        bool enable_shadows = true;
        int cascade_count = 4;
        std::array<float, 8> cascade_distances = {1.0f, 5.0f, 20.0f, 100.0f, 0.0f, 0.0f, 0.0f, 0.0f};
        float cascade_lambda = 0.5f;
        int shadow_resolution = 2048;
        float shadow_bias = 0.001f;
        float shadow_normal_bias = 0.01f;
        bool enable_pcf = true;
        int pcf_samples = 4;
        bool enable_contact_shadows = false;
        float contact_shadow_length = 0.1f;
    } shadows;
    
    // Environment settings
    struct EnvironmentConfig {
        float sky_intensity = 1.0f;
        std::array<float, 3> ambient_color = {0.1f, 0.1f, 0.15f};
        float ambient_intensity = 0.3f;
        bool enable_ibl = true;
        float ibl_intensity = 1.0f;
        bool rotate_environment = false;
        float rotation_speed = 0.1f;
    } environment;
    
    // Quality settings
    struct QualityConfig {
        int msaa_samples = 1;
        bool enable_temporal_upsampling = false;
        float render_scale = 1.0f;
        int max_lights_per_tile = 1024;
        bool use_compute_shading = true;
        bool enable_gpu_culling = true;
        bool enable_early_z = true;
    } quality;
};

/**
 * @brief Real-time performance metrics
 */
struct RenderingPerformanceMetrics {
    // Frame timing
    float frame_time_ms = 0.0f;
    float gpu_time_ms = 0.0f;
    float cpu_time_ms = 0.0f;
    
    // Pass-specific timing
    float geometry_pass_ms = 0.0f;
    float shadow_pass_ms = 0.0f;
    float lighting_pass_ms = 0.0f;
    float post_process_ms = 0.0f;
    
    // Resource usage
    uint64_t gpu_memory_used = 0;
    uint64_t gpu_memory_total = 0;
    uint64_t texture_memory_used = 0;
    uint64_t buffer_memory_used = 0;
    
    // Rendering statistics
    uint32_t draw_calls = 0;
    uint32_t vertices_rendered = 0;
    uint32_t triangles_rendered = 0;
    uint32_t lights_rendered = 0;
    uint32_t shadow_maps_updated = 0;
    
    // Quality metrics
    float pixel_overdraw = 0.0f;
    float shader_complexity = 0.0f;
    uint32_t texture_switches = 0;
    uint32_t render_target_switches = 0;
    
    std::chrono::steady_clock::time_point timestamp;
};

/**
 * @brief Scene object representation for UI
 */
struct SceneObject {
    uint32_t id = 0;
    std::string name;
    bool visible = true;
    bool cast_shadows = true;
    std::array<float, 16> transform = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    
    // Rendering properties
    rendering::MaterialProperties material;
    rendering::BufferHandle vertex_buffer;
    rendering::BufferHandle index_buffer;
    uint32_t index_count = 0;
    
    // Hierarchy
    std::vector<uint32_t> children;
    uint32_t parent_id = 0;
    
    // LOD information
    int lod_level = 0;
    float lod_distance = 0.0f;
    std::vector<rendering::BufferHandle> lod_vertex_buffers;
    std::vector<rendering::BufferHandle> lod_index_buffers;
    std::vector<uint32_t> lod_index_counts;
};

/**
 * @brief Light object for scene management
 */
struct SceneLight {
    uint32_t id = 0;
    std::string name;
    bool enabled = true;
    rendering::Light light_data;
    
    // UI-specific properties
    bool show_debug_visualization = false;
    Color debug_color = {1.0f, 1.0f, 0.0f, 1.0f};
    
    // Animation properties
    bool animated = false;
    std::array<float, 3> animation_center = {0.0f, 0.0f, 0.0f};
    float animation_radius = 5.0f;
    float animation_speed = 1.0f;
};

/**
 * @brief Shader program information for hot-reload system
 */
struct ShaderProgram {
    std::string name;
    std::string vertex_path;
    std::string fragment_path;
    std::string compute_path;
    rendering::ShaderHandle handle;
    ShaderReloadStatus reload_status = ShaderReloadStatus::Idle;
    std::chrono::steady_clock::time_point last_modified;
    std::string error_message;
};

// =============================================================================
// RENDERING UI MAIN CLASS
// =============================================================================

/**
 * @brief Comprehensive Rendering System UI
 * 
 * This class provides a complete interface for controlling and debugging
 * the rendering pipeline in real-time. It integrates with the existing
 * Dashboard system and provides specialized panels for rendering control.
 */
class RenderingUI {
public:
    RenderingUI();
    ~RenderingUI();

    // =============================================================================
    // INITIALIZATION & LIFECYCLE
    // =============================================================================

    /**
     * @brief Initialize the rendering UI system
     * @param renderer The renderer instance to control
     * @param deferred_renderer The deferred renderer for advanced features
     * @param dashboard Optional dashboard integration
     * @return true on success
     */
    bool initialize(rendering::IRenderer* renderer,
                   rendering::DeferredRenderer* deferred_renderer = nullptr,
                   Dashboard* dashboard = nullptr);

    /**
     * @brief Shutdown and cleanup resources
     */
    void shutdown();

    /**
     * @brief Check if properly initialized
     */
    bool is_initialized() const { return initialized_; }

    // =============================================================================
    // MAIN RENDERING INTERFACE
    // =============================================================================

    /**
     * @brief Render the complete UI system
     * Call this once per frame from the main render loop
     */
    void render();

    /**
     * @brief Update UI state and metrics
     * @param delta_time Time since last update
     */
    void update(float delta_time);

    // =============================================================================
    // CONFIGURATION MANAGEMENT
    // =============================================================================

    /**
     * @brief Get current rendering configuration
     */
    LiveRenderingConfig& get_config() { return config_; }
    const LiveRenderingConfig& get_config() const { return config_; }

    /**
     * @brief Apply configuration changes to renderer
     */
    void apply_config_changes();

    /**
     * @brief Load configuration from file
     */
    bool load_config(const std::string& filepath);

    /**
     * @brief Save configuration to file
     */
    bool save_config(const std::string& filepath) const;

    /**
     * @brief Reset to default configuration
     */
    void reset_to_defaults();

    // =============================================================================
    // SCENE MANAGEMENT
    // =============================================================================

    /**
     * @brief Add scene object to management system
     */
    uint32_t add_scene_object(const SceneObject& object);

    /**
     * @brief Remove scene object
     */
    void remove_scene_object(uint32_t object_id);

    /**
     * @brief Get scene object by ID
     */
    SceneObject* get_scene_object(uint32_t object_id);

    /**
     * @brief Add light to scene
     */
    uint32_t add_scene_light(const SceneLight& light);

    /**
     * @brief Remove scene light
     */
    void remove_scene_light(uint32_t light_id);

    /**
     * @brief Get scene light by ID
     */
    SceneLight* get_scene_light(uint32_t light_id);

    /**
     * @brief Clear entire scene
     */
    void clear_scene();

    // =============================================================================
    // SHADER MANAGEMENT
    // =============================================================================

    /**
     * @brief Register shader for hot-reload monitoring
     */
    void register_shader(const ShaderProgram& shader);

    /**
     * @brief Reload specific shader
     */
    bool reload_shader(const std::string& shader_name);

    /**
     * @brief Reload all shaders
     */
    void reload_all_shaders();

    /**
     * @brief Get shader reload status
     */
    ShaderReloadStatus get_shader_status(const std::string& shader_name) const;

    // =============================================================================
    // DEBUGGING & VISUALIZATION
    // =============================================================================

    /**
     * @brief Set debug visualization mode
     */
    void set_debug_mode(DebugVisualizationMode mode);

    /**
     * @brief Get current debug mode
     */
    DebugVisualizationMode get_debug_mode() const { return debug_mode_; }

    /**
     * @brief Capture frame for detailed analysis
     */
    void capture_frame();

    /**
     * @brief Get current performance metrics
     */
    const RenderingPerformanceMetrics& get_metrics() const { return current_metrics_; }

    /**
     * @brief Get performance history
     */
    const std::vector<RenderingPerformanceMetrics>& get_metrics_history() const { return metrics_history_; }

    // =============================================================================
    // CAMERA CONTROL
    // =============================================================================

    /**
     * @brief Set camera control mode
     */
    void set_camera_mode(CameraControlMode mode);

    /**
     * @brief Get camera matrices for current viewport
     */
    std::pair<std::array<float, 16>, std::array<float, 16>> get_camera_matrices() const;

    /**
     * @brief Focus camera on scene object
     */
    void focus_camera_on_object(uint32_t object_id);

    /**
     * @brief Reset camera to default position
     */
    void reset_camera();

private:
    // =============================================================================
    // UI PANEL RENDERING METHODS
    // =============================================================================

    void render_main_control_panel();
    void render_pipeline_control_panel();
    void render_material_editor_panel();
    void render_lighting_control_panel();
    void render_post_processing_panel();
    void render_debug_visualization_panel();
    void render_performance_profiler_panel();
    void render_scene_hierarchy_panel();
    void render_viewport_panel();
    void render_shader_editor_panel();
    void render_render_graph_panel();
    void render_gpu_memory_panel();

    // Pipeline control widgets
    void render_deferred_config_controls();
    void render_shadow_config_controls();
    void render_quality_settings_controls();

    // Material editor widgets
    void render_pbr_material_editor(rendering::MaterialProperties& material);
    void render_texture_slot_editor(const std::string& label, rendering::TextureHandle& texture);
    void render_material_preview(const rendering::MaterialProperties& material);

    // Lighting controls
    void render_light_editor(SceneLight& light);
    void render_environment_lighting_controls();
    void render_light_animation_controls(SceneLight& light);

    // Post-processing controls
    void render_hdr_tone_mapping_controls();
    void render_bloom_controls();
    void render_ssao_controls();
    void render_ssr_controls();
    void render_taa_controls();

    // Debug visualization
    void render_gbuffer_visualization();
    void render_performance_overlay();
    void render_light_debug_visualization();

    // Scene management
    void render_scene_object_editor(SceneObject& object);
    void render_transform_editor(std::array<float, 16>& transform);
    void render_hierarchy_tree();

    // Performance profiling
    void render_frame_time_graph();
    void render_gpu_profiler();
    void render_memory_usage_charts();
    void render_draw_call_analysis();

    // Shader editor
    void render_shader_list();
    void render_shader_reload_controls();
    void render_shader_error_display();

    // =============================================================================
    // UTILITY METHODS
    // =============================================================================

    void update_performance_metrics();
    void collect_gpu_stats();
    void monitor_shader_files();
    void handle_viewport_input();
    void update_camera_controls();
    void animate_scene_lights(float delta_time);
    
    // Configuration helpers
    void apply_deferred_config();
    void apply_post_process_config();
    void apply_shadow_config();
    void apply_quality_config();

    // Debug helpers
    void render_debug_overlay();
    void capture_gbuffer_textures();
    rendering::TextureHandle get_debug_texture() const;

    // Scene helpers
    void update_scene_objects();
    void cull_scene_objects();
    void submit_scene_to_renderer();

    // =============================================================================
    // MEMBER VARIABLES
    // =============================================================================

    // Core state
    bool initialized_ = false;
    rendering::IRenderer* renderer_ = nullptr;
    rendering::DeferredRenderer* deferred_renderer_ = nullptr;
    Dashboard* dashboard_ = nullptr;

    // Configuration
    LiveRenderingConfig config_;
    bool config_dirty_ = false;

    // Performance monitoring
    RenderingPerformanceMetrics current_metrics_;
    std::vector<RenderingPerformanceMetrics> metrics_history_;
    static constexpr size_t MAX_METRICS_HISTORY = 300; // 5 seconds at 60fps
    std::chrono::steady_clock::time_point last_metrics_update_;

    // Scene management
    std::unordered_map<uint32_t, SceneObject> scene_objects_;
    std::unordered_map<uint32_t, SceneLight> scene_lights_;
    uint32_t next_object_id_ = 1;
    uint32_t next_light_id_ = 1;
    uint32_t selected_object_id_ = 0;
    uint32_t selected_light_id_ = 0;

    // Shader management
    std::unordered_map<std::string, ShaderProgram> shaders_;
    bool shader_hot_reload_enabled_ = true;
    std::chrono::steady_clock::time_point last_shader_check_;

    // Camera control
    CameraControlMode camera_mode_ = CameraControlMode::Orbit;
    struct CameraState {
        std::array<float, 3> position = {0.0f, 5.0f, 10.0f};
        std::array<float, 3> target = {0.0f, 0.0f, 0.0f};
        std::array<float, 3> up = {0.0f, 1.0f, 0.0f};
        float fov = 45.0f;
        float near_plane = 0.1f;
        float far_plane = 1000.0f;
        float orbit_distance = 15.0f;
        float orbit_phi = 0.0f;    // Horizontal angle
        float orbit_theta = 0.3f;  // Vertical angle
    } camera_;

    // Viewport state
    Vec2 viewport_size_ = {1920, 1080};
    bool viewport_focused_ = false;
    Vec2 last_mouse_pos_ = {0, 0};
    bool mouse_dragging_ = false;

    // Debug state
    DebugVisualizationMode debug_mode_ = DebugVisualizationMode::None;
    bool show_performance_overlay_ = true;
    bool show_debug_wireframe_ = false;
    bool capture_next_frame_ = false;

    // UI state
    bool show_pipeline_panel_ = true;
    bool show_material_editor_ = true;
    bool show_lighting_panel_ = true;
    bool show_post_process_panel_ = true;
    bool show_debug_panel_ = true;
    bool show_profiler_panel_ = true;
    bool show_scene_hierarchy_ = true;
    bool show_viewport_ = true;
    bool show_shader_editor_ = true;
    bool show_render_graph_panel_ = false;
    bool show_gpu_memory_panel_ = false;

    // Animation
    float animation_time_ = 0.0f;

    // Preset management
    std::vector<std::string> config_presets_;
    std::string current_preset_name_;
};

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

/**
 * @brief Create default rendering configuration
 */
LiveRenderingConfig create_default_rendering_config();

/**
 * @brief Convert debug visualization mode to string
 */
std::string debug_mode_to_string(DebugVisualizationMode mode);

/**
 * @brief Get icon for profiling target
 */
const char* get_profiling_target_icon(ProfilingTarget target);

/**
 * @brief Format memory size for display
 */
std::string format_memory_size(uint64_t bytes);

/**
 * @brief Format GPU timing for display
 */
std::string format_gpu_time(float milliseconds);

/**
 * @brief Create material preview sphere mesh
 */
std::pair<rendering::BufferHandle, rendering::BufferHandle> 
create_preview_sphere_mesh(rendering::IRenderer* renderer, uint32_t& index_count);

/**
 * @brief Register default rendering UI features with dashboard
 */
void register_rendering_ui_features(Dashboard* dashboard, RenderingUI* rendering_ui);

/**
 * @brief Validate rendering configuration for current hardware
 */
bool validate_rendering_config(const LiveRenderingConfig& config, rendering::IRenderer* renderer);

} // namespace ecscope::gui