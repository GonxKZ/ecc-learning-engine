/**
 * @file deferred_renderer.hpp
 * @brief Professional Deferred Rendering Pipeline
 * 
 * Complete deferred shading implementation with G-buffer layout,
 * lighting passes, and advanced post-processing effects.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#pragma once

#include "renderer.hpp"
#include <memory>
#include <vector>
#include <array>
#include <functional>

namespace ecscope::rendering {

// =============================================================================
// DEFERRED RENDERING STRUCTURES
// =============================================================================

/**
 * @brief G-buffer layout specification
 */
enum class GBufferTarget : uint8_t {
    Albedo,          ///< RGB: Albedo/Diffuse, A: Metallic
    Normal,          ///< RGB: World-space normal (packed), A: Roughness
    MotionVector,    ///< RG: Motion vectors, B: Depth derivative, A: Reserved
    MaterialProperties, ///< R: AO, G: Emission, B: Subsurface, A: Reserved
    Depth            ///< Single channel depth buffer
};

/**
 * @brief Light types for deferred lighting
 */
enum class LightType : uint8_t {
    Directional,     ///< Sun/Moon directional light
    Point,          ///< Point light with radius
    Spot,           ///< Spot light with cone
    Area            ///< Area light (rectangular)
};

/**
 * @brief Light data structure
 */
struct Light {
    LightType type = LightType::Point;
    std::array<float, 3> position = {0.0f, 0.0f, 0.0f};
    std::array<float, 3> direction = {0.0f, -1.0f, 0.0f};
    std::array<float, 3> color = {1.0f, 1.0f, 1.0f};
    float intensity = 1.0f;
    float range = 10.0f;
    float inner_cone_angle = 30.0f; ///< For spot lights (degrees)
    float outer_cone_angle = 45.0f; ///< For spot lights (degrees)
    std::array<float, 2> area_size = {1.0f, 1.0f}; ///< For area lights
    bool cast_shadows = true;
    uint32_t shadow_map_size = 1024;
    
    // Shadow cascade data (for directional lights)
    uint32_t cascade_count = 4;
    std::array<float, 8> cascade_distances = {};
};

/**
 * @brief Deferred rendering configuration
 */
struct DeferredConfig {
    // G-buffer format configuration
    TextureFormat albedo_format = TextureFormat::RGBA8;
    TextureFormat normal_format = TextureFormat::RGBA16F;
    TextureFormat motion_format = TextureFormat::RG16F;
    TextureFormat material_format = TextureFormat::RGBA8;
    TextureFormat depth_format = TextureFormat::Depth24Stencil8;
    
    // Resolution and quality
    uint32_t width = 1920;
    uint32_t height = 1080;
    uint32_t msaa_samples = 1;
    
    // Features
    bool enable_motion_vectors = true;
    bool enable_temporal_effects = true;
    bool enable_screen_space_reflections = true;
    bool enable_volumetric_lighting = false;
    
    // Performance settings
    uint32_t max_lights_per_tile = 1024;
    uint32_t tile_size = 16; ///< Tile size for tiled deferred shading
    bool use_compute_shading = true; ///< Use compute shaders for lighting
    
    // Debug options
    bool visualize_overdraw = false;
    bool visualize_light_complexity = false;
    bool visualize_g_buffer = false;
};

/**
 * @brief Material properties for PBR rendering
 */
struct MaterialProperties {
    std::array<float, 3> albedo = {0.5f, 0.5f, 0.5f};
    float metallic = 0.0f;
    float roughness = 0.5f;
    float normal_intensity = 1.0f;
    float emission_intensity = 0.0f;
    std::array<float, 3> emission_color = {1.0f, 1.0f, 1.0f};
    float subsurface_scattering = 0.0f;
    float ambient_occlusion = 1.0f;
    
    // Texture handles
    TextureHandle albedo_texture;
    TextureHandle normal_texture;
    TextureHandle metallic_roughness_texture;
    TextureHandle emission_texture;
    TextureHandle occlusion_texture;
    TextureHandle height_texture;
};

/**
 * @brief Environment lighting configuration
 */
struct EnvironmentLighting {
    TextureHandle skybox_texture;          ///< HDR skybox cubemap
    TextureHandle irradiance_map;          ///< Diffuse irradiance cubemap
    TextureHandle prefiltered_map;         ///< Specular prefiltered cubemap
    TextureHandle brdf_lut;                ///< BRDF integration lookup table
    
    float intensity = 1.0f;
    std::array<float, 3> ambient_color = {0.03f, 0.03f, 0.03f};
    bool rotate_environment = false;
    float rotation_speed = 0.1f;
};

// =============================================================================
// DEFERRED RENDERER CLASS
// =============================================================================

/**
 * @brief Professional deferred rendering pipeline
 * 
 * Features:
 * - Tiled/Clustered deferred shading for scalable lighting
 * - PBR (Physically Based Rendering) materials
 * - Shadow mapping with cascaded shadow maps
 * - Screen-space effects (SSR, SSAO, etc.)
 * - Temporal anti-aliasing (TAA)
 * - HDR pipeline with tone mapping
 * - Multi-threaded command generation
 */
class DeferredRenderer {
public:
    DeferredRenderer(IRenderer* renderer);
    ~DeferredRenderer();

    // Disable copy/move
    DeferredRenderer(const DeferredRenderer&) = delete;
    DeferredRenderer& operator=(const DeferredRenderer&) = delete;
    DeferredRenderer(DeferredRenderer&&) = delete;
    DeferredRenderer& operator=(DeferredRenderer&&) = delete;

    // =============================================================================
    // INITIALIZATION & CONFIGURATION
    // =============================================================================

    /**
     * @brief Initialize the deferred rendering pipeline
     */
    bool initialize(const DeferredConfig& config);

    /**
     * @brief Shutdown and cleanup resources
     */
    void shutdown();

    /**
     * @brief Resize render targets (call on window resize)
     */
    void resize(uint32_t width, uint32_t height);

    /**
     * @brief Update configuration at runtime
     */
    void update_config(const DeferredConfig& config);

    /**
     * @brief Get current configuration
     */
    const DeferredConfig& get_config() const { return config_; }

    // =============================================================================
    // FRAME RENDERING
    // =============================================================================

    /**
     * @brief Begin a new frame
     */
    void begin_frame();

    /**
     * @brief End the current frame
     */
    void end_frame();

    /**
     * @brief Set camera matrices
     */
    void set_camera(const std::array<float, 16>& view_matrix,
                    const std::array<float, 16>& projection_matrix,
                    const std::array<float, 16>& prev_view_projection = {});

    /**
     * @brief Submit geometry for rendering
     */
    void submit_geometry(BufferHandle vertex_buffer,
                        BufferHandle index_buffer,
                        const MaterialProperties& material,
                        const std::array<float, 16>& model_matrix,
                        uint32_t index_count = 0,
                        uint32_t index_offset = 0);

    /**
     * @brief Submit a light for rendering
     */
    void submit_light(const Light& light);

    /**
     * @brief Set environment lighting
     */
    void set_environment(const EnvironmentLighting& environment);

    // =============================================================================
    // RENDER PASSES
    // =============================================================================

    /**
     * @brief Execute geometry pass (G-buffer generation)
     */
    void geometry_pass();

    /**
     * @brief Execute shadow pass for all shadow-casting lights
     */
    void shadow_pass();

    /**
     * @brief Execute lighting pass (deferred shading)
     */
    void lighting_pass();

    /**
     * @brief Execute post-processing effects
     */
    void post_process_pass();

    /**
     * @brief Execute final composition and tone mapping
     */
    void composition_pass();

    // =============================================================================
    // ADVANCED FEATURES
    // =============================================================================

    /**
     * @brief Screen-space ambient occlusion pass
     */
    void ssao_pass();

    /**
     * @brief Screen-space reflections pass
     */
    void ssr_pass();

    /**
     * @brief Temporal anti-aliasing pass
     */
    void taa_pass();

    /**
     * @brief Motion blur pass
     */
    void motion_blur_pass();

    /**
     * @brief Volumetric lighting pass
     */
    void volumetric_lighting_pass();

    /**
     * @brief Bloom effect pass
     */
    void bloom_pass();

    /**
     * @brief God rays pass
     */
    void god_rays_pass();

    // =============================================================================
    // DEBUGGING & VISUALIZATION
    // =============================================================================

    /**
     * @brief Render G-buffer visualization
     */
    void render_g_buffer_debug();

    /**
     * @brief Render light complexity heatmap
     */
    void render_light_complexity();

    /**
     * @brief Render overdraw visualization
     */
    void render_overdraw_debug();

    /**
     * @brief Get frame statistics
     */
    struct DeferredStats {
        uint32_t geometry_draw_calls = 0;
        uint32_t light_count = 0;
        uint32_t shadow_map_updates = 0;
        float geometry_pass_time_ms = 0.0f;
        float shadow_pass_time_ms = 0.0f;
        float lighting_pass_time_ms = 0.0f;
        float post_process_time_ms = 0.0f;
        uint64_t g_buffer_memory_mb = 0;
        uint64_t shadow_memory_mb = 0;
    };

    DeferredStats get_statistics() const { return stats_; }

    // =============================================================================
    // RESOURCE ACCESS
    // =============================================================================

    /**
     * @brief Get G-buffer texture handle
     */
    TextureHandle get_g_buffer_texture(GBufferTarget target) const;

    /**
     * @brief Get final HDR render target
     */
    TextureHandle get_hdr_target() const { return hdr_target_; }

    /**
     * @brief Get final LDR render target (after tone mapping)
     */
    TextureHandle get_ldr_target() const { return ldr_target_; }

    /**
     * @brief Get depth buffer
     */
    TextureHandle get_depth_buffer() const;

private:
    // =============================================================================
    // INTERNAL STRUCTURES
    // =============================================================================

    struct GeometryDrawCall {
        BufferHandle vertex_buffer;
        BufferHandle index_buffer;
        MaterialProperties material;
        std::array<float, 16> model_matrix;
        std::array<float, 16> prev_model_matrix;
        uint32_t index_count;
        uint32_t index_offset;
    };

    struct ShadowMap {
        TextureHandle depth_texture;
        std::array<float, 16> light_view_matrix;
        std::array<float, 16> light_projection_matrix;
        uint32_t resolution;
        bool needs_update;
    };

    struct TileData {
        uint32_t light_count;
        std::array<uint32_t, 1024> light_indices; // Max lights per tile
    };

    // =============================================================================
    // RESOURCE CREATION
    // =============================================================================

    bool create_g_buffer();
    bool create_shadow_maps();
    bool create_post_process_targets();
    bool create_shaders();
    bool create_samplers();

    void destroy_resources();

    // =============================================================================
    // SHADER MANAGEMENT
    // =============================================================================

    bool load_geometry_shaders();
    bool load_lighting_shaders();
    bool load_post_process_shaders();
    bool load_debug_shaders();

    // =============================================================================
    // LIGHTING CALCULATIONS
    // =============================================================================

    void cull_lights();
    void update_light_tiles();
    void update_shadow_maps();

    std::array<float, 16> calculate_light_view_matrix(const Light& light, uint32_t cascade_index = 0);
    std::array<float, 16> calculate_light_projection_matrix(const Light& light, uint32_t cascade_index = 0);
    
    void render_tiled_lighting_compute();
    void render_fullscreen_lighting();
    void render_fullscreen_quad();

    // =============================================================================
    // POST-PROCESSING HELPERS
    // =============================================================================

    void downsample_texture(TextureHandle source, TextureHandle destination);
    void upsample_texture(TextureHandle source, TextureHandle destination);
    void gaussian_blur(TextureHandle source, TextureHandle temp, TextureHandle destination, float sigma);

    // =============================================================================
    // MEMBER VARIABLES
    // =============================================================================

    IRenderer* renderer_;
    DeferredConfig config_;
    bool initialized_ = false;

    // G-Buffer targets
    std::array<TextureHandle, 5> g_buffer_targets_; // Albedo, Normal, Motion, Material, Depth
    
    // Render targets
    TextureHandle hdr_target_;           // High dynamic range accumulation
    TextureHandle ldr_target_;           // Low dynamic range final output
    TextureHandle prev_frame_target_;    // Previous frame for temporal effects
    TextureHandle velocity_buffer_;      // Motion vectors
    
    // Post-processing targets
    TextureHandle ssao_target_;          // Screen-space ambient occlusion
    TextureHandle ssr_target_;           // Screen-space reflections
    TextureHandle bloom_targets_[6];     // Bloom mip chain
    TextureHandle temp_targets_[4];      // Temporary render targets
    
    // Shadow mapping
    std::vector<ShadowMap> directional_shadow_maps_;
    std::vector<ShadowMap> point_shadow_maps_;
    std::vector<ShadowMap> spot_shadow_maps_;
    TextureHandle shadow_atlas_;         // Packed shadow maps
    
    // Shaders
    ShaderHandle geometry_shader_;
    ShaderHandle lighting_shader_;
    ShaderHandle shadow_shader_;
    ShaderHandle ssao_shader_;
    ShaderHandle ssr_shader_;
    ShaderHandle taa_shader_;
    ShaderHandle motion_blur_shader_;
    ShaderHandle bloom_downsample_shader_;
    ShaderHandle bloom_upsample_shader_;
    ShaderHandle tone_mapping_shader_;
    ShaderHandle debug_visualization_shader_;
    
    // Uniform buffers
    BufferHandle camera_uniform_buffer_;
    BufferHandle lighting_uniform_buffer_;
    BufferHandle material_uniform_buffer_;
    BufferHandle tile_data_buffer_;
    
    // Current frame data
    std::vector<GeometryDrawCall> geometry_draw_calls_;
    std::vector<Light> lights_;
    EnvironmentLighting environment_;
    
    // Camera matrices
    std::array<float, 16> view_matrix_;
    std::array<float, 16> projection_matrix_;
    std::array<float, 16> view_projection_matrix_;
    std::array<float, 16> prev_view_projection_matrix_;
    std::array<float, 16> inv_view_matrix_;
    std::array<float, 16> inv_projection_matrix_;
    
    // Tile-based lighting data
    std::vector<std::vector<TileData>> light_tiles_;
    uint32_t tiles_x_ = 0;
    uint32_t tiles_y_ = 0;
    
    // Statistics and profiling
    mutable DeferredStats stats_;
    std::vector<uint64_t> pass_timers_;
    
    // Temporal data
    uint32_t frame_index_ = 0;
    std::array<float, 2> jitter_offset_ = {0.0f, 0.0f};
    
    // Debug state
    GBufferTarget debug_g_buffer_target_ = GBufferTarget::Albedo;
    bool debug_mode_ = false;
};

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

/**
 * @brief Calculate optimal G-buffer formats for target hardware
 */
DeferredConfig optimize_g_buffer_format(IRenderer* renderer, uint32_t width, uint32_t height);

/**
 * @brief Pre-compute environment lighting textures
 */
bool precompute_environment_lighting(IRenderer* renderer, 
                                   TextureHandle hdr_environment,
                                   EnvironmentLighting& out_lighting);

/**
 * @brief Create BRDF integration lookup table
 */
TextureHandle create_brdf_lut(IRenderer* renderer, uint32_t size = 512);

/**
 * @brief Utility to pack normal vector into RGB format
 */
std::array<uint8_t, 3> pack_normal(const std::array<float, 3>& normal);

/**
 * @brief Utility to unpack normal vector from RGB format
 */
std::array<float, 3> unpack_normal(const std::array<uint8_t, 3>& packed_normal);

} // namespace ecscope::rendering