/**
 * @file advanced.hpp
 * @brief Advanced Rendering Features & Effects
 * 
 * Comprehensive collection of advanced rendering techniques including
 * shadow mapping, SSAO, bloom, tone mapping, and performance optimizations.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#pragma once

#include "renderer.hpp"
#include "deferred_renderer.hpp"
#include <memory>
#include <vector>
#include <array>
#include <functional>
#include <span>

namespace ecscope::rendering {

// =============================================================================
// SHADOW MAPPING SYSTEM
// =============================================================================

/**
 * @brief Shadow mapping techniques
 */
enum class ShadowTechnique : uint8_t {
    Basic,                    ///< Standard shadow mapping
    PCF,                     ///< Percentage Closer Filtering
    PCSS,                    ///< Percentage Closer Soft Shadows
    VSM,                     ///< Variance Shadow Maps
    ESM,                     ///< Exponential Shadow Maps
    CSM,                     ///< Cascaded Shadow Maps (directional lights)
    EVSM                     ///< Exponential Variance Shadow Maps
};

/**
 * @brief Shadow quality settings
 */
struct ShadowConfig {
    ShadowTechnique technique = ShadowTechnique::PCF;
    uint32_t resolution = 2048;
    uint32_t cascade_count = 4;        ///< For CSM
    float cascade_lambda = 0.5f;       ///< Cascade distribution
    float depth_bias = 0.001f;
    float normal_bias = 0.1f;
    uint32_t pcf_samples = 16;         ///< PCF sample count
    float pcss_light_size = 0.1f;      ///< PCSS light world size
    bool enable_soft_shadows = true;
    bool optimize_for_distance = true;
};

/**
 * @brief Professional shadow mapping system
 */
class ShadowMapper {
public:
    ShadowMapper(IRenderer* renderer);
    ~ShadowMapper();

    bool initialize(const ShadowConfig& config);
    void shutdown();
    
    // Shadow map management
    void begin_shadow_pass();
    void render_shadow_map(const Light& light, std::span<const GeometryDrawCall> geometry);
    void end_shadow_pass();
    
    // Shadow application
    void apply_shadows_to_gbuffer(const DeferredRenderer& deferred_renderer);
    TextureHandle get_shadow_atlas() const { return shadow_atlas_; }
    
    // Configuration
    void update_config(const ShadowConfig& config);
    const ShadowConfig& get_config() const { return config_; }
    
    // Statistics
    struct ShadowStats {
        uint32_t active_shadow_maps = 0;
        uint32_t shadow_draw_calls = 0;
        float shadow_pass_time_ms = 0.0f;
        uint64_t shadow_memory_mb = 0;
    };
    
    ShadowStats get_statistics() const { return stats_; }

private:
    IRenderer* renderer_;
    ShadowConfig config_;
    TextureHandle shadow_atlas_;
    std::vector<TextureHandle> cascade_maps_;
    ShaderHandle shadow_shader_;
    ShaderHandle shadow_filter_shader_;
    BufferHandle shadow_uniform_buffer_;
    mutable ShadowStats stats_;
    
    void create_shadow_maps();
    void create_shaders();
    std::array<float, 16> calculate_cascade_projection(const Light& light, uint32_t cascade_index);
};

// =============================================================================
// SCREEN-SPACE AMBIENT OCCLUSION (SSAO)
// =============================================================================

/**
 * @brief SSAO techniques
 */
enum class SSAOTechnique : uint8_t {
    Classic,                 ///< Classic SSAO
    HBAO,                   ///< Horizon-Based Ambient Occlusion  
    GTAO,                   ///< Ground Truth Ambient Occlusion
    ASSAO,                  ///< Adaptive Screen-Space Ambient Occlusion
    RayTracedAO             ///< Hardware ray traced AO (RTX)
};

/**
 * @brief SSAO configuration
 */
struct SSAOConfig {
    SSAOTechnique technique = SSAOTechnique::HBAO;
    uint32_t sample_count = 16;
    float radius = 0.5f;
    float intensity = 1.0f;
    float bias = 0.01f;
    bool enable_blur = true;
    uint32_t blur_passes = 2;
    float blur_sharpness = 8.0f;
    bool temporal_filter = true;
    bool use_half_resolution = false;    ///< Render at half-res for performance
};

/**
 * @brief Professional SSAO implementation
 */
class SSAORenderer {
public:
    SSAORenderer(IRenderer* renderer);
    ~SSAORenderer();

    bool initialize(const SSAOConfig& config);
    void shutdown();
    
    // SSAO rendering
    TextureHandle render_ssao(TextureHandle depth_buffer, TextureHandle normal_buffer,
                             const std::array<float, 16>& view_matrix,
                             const std::array<float, 16>& projection_matrix);
    
    // Configuration
    void update_config(const SSAOConfig& config);
    const SSAOConfig& get_config() const { return config_; }
    
    // Debug visualization
    void render_debug_overlay(TextureHandle target);

private:
    IRenderer* renderer_;
    SSAOConfig config_;
    
    // Render targets
    TextureHandle ssao_target_;
    TextureHandle ssao_blur_target_;
    TextureHandle noise_texture_;
    
    // Shaders
    ShaderHandle ssao_shader_;
    ShaderHandle blur_shader_;
    
    // Sample kernel
    std::vector<std::array<float, 3>> sample_kernel_;
    BufferHandle kernel_buffer_;
    
    void create_render_targets();
    void create_shaders();
    void generate_sample_kernel();
    void generate_noise_texture();
};

// =============================================================================
// BLOOM & HDR POST-PROCESSING
// =============================================================================

/**
 * @brief Bloom configuration
 */
struct BloomConfig {
    float threshold = 1.0f;          ///< Brightness threshold
    float intensity = 0.1f;          ///< Bloom intensity
    uint32_t iterations = 6;         ///< Blur iterations
    float scatter = 0.7f;           ///< Bloom scatter
    std::array<float, 3> tint = {1.0f, 1.0f, 1.0f}; ///< Bloom tint color
    bool enable_lens_flare = false;
    bool enable_ghost_features = false;
};

/**
 * @brief Tone mapping operators
 */
enum class ToneMappingOperator : uint8_t {
    None,                   ///< No tone mapping (clamp to [0,1])
    Reinhard,               ///< Reinhard tone mapping
    ReinhardExtended,       ///< Extended Reinhard with white point
    Filmic,                 ///< Filmic tone mapping (Uncharted 2)
    ACES,                   ///< ACES Filmic tone mapping
    Exposure,               ///< Simple exposure-based
    AGX,                    ///< AgX tone mapping
    Custom                  ///< User-defined curve
};

/**
 * @brief HDR tone mapping configuration
 */
struct ToneMappingConfig {
    ToneMappingOperator operator_type = ToneMappingOperator::ACES;
    float exposure = 1.0f;
    float gamma = 2.2f;
    float white_point = 11.2f;      ///< For ReinhardExtended
    float shoulder_strength = 0.15f; ///< For Filmic
    float linear_strength = 0.5f;    ///< For Filmic
    float linear_angle = 0.1f;       ///< For Filmic
    float toe_strength = 0.2f;       ///< For Filmic
    float toe_numerator = 0.02f;     ///< For Filmic
    float toe_denominator = 0.3f;    ///< For Filmic
    TextureHandle custom_lut;        ///< For custom tone mapping
};

/**
 * @brief Professional bloom and tone mapping pipeline
 */
class HDRPostProcessor {
public:
    HDRPostProcessor(IRenderer* renderer);
    ~HDRPostProcessor();

    bool initialize(uint32_t width, uint32_t height);
    void shutdown();
    void resize(uint32_t width, uint32_t height);
    
    // Post-processing pipeline
    TextureHandle process_hdr_frame(TextureHandle hdr_input, 
                                   const BloomConfig& bloom_config,
                                   const ToneMappingConfig& tone_config);
    
    // Individual effects
    TextureHandle render_bloom(TextureHandle hdr_input, const BloomConfig& config);
    TextureHandle apply_tone_mapping(TextureHandle hdr_input, const ToneMappingConfig& config);
    
    // Utilities
    static TextureHandle create_custom_lut(IRenderer* renderer, 
                                         std::function<std::array<float, 3>(std::array<float, 3>)> curve,
                                         uint32_t size = 64);

private:
    IRenderer* renderer_;
    uint32_t width_, height_;
    
    // Bloom chain
    std::vector<TextureHandle> bloom_mips_;
    ShaderHandle bloom_downsample_shader_;
    ShaderHandle bloom_upsample_shader_;
    
    // Tone mapping
    ShaderHandle tone_mapping_shader_;
    TextureHandle tone_mapped_target_;
    
    void create_bloom_chain();
    void create_shaders();
};

// =============================================================================
// SCREEN-SPACE REFLECTIONS (SSR)
// =============================================================================

/**
 * @brief SSR quality settings
 */
struct SSRConfig {
    uint32_t max_steps = 64;         ///< Ray marching steps
    float step_size = 1.0f;          ///< Ray step size
    float thickness = 0.1f;          ///< Surface thickness
    uint32_t binary_search_steps = 8; ///< Binary search refinement steps
    float max_roughness = 0.8f;      ///< Max roughness for reflections
    float fade_start = 0.7f;         ///< Screen edge fade start
    float fade_end = 1.0f;           ///< Screen edge fade end
    bool enable_temporal_filter = true;
    bool use_half_resolution = true;
};

/**
 * @brief Screen-space reflections renderer
 */
class SSRRenderer {
public:
    SSRRenderer(IRenderer* renderer);
    ~SSRRenderer();

    bool initialize(const SSRConfig& config);
    void shutdown();
    
    TextureHandle render_ssr(TextureHandle color_buffer, TextureHandle normal_buffer,
                            TextureHandle depth_buffer, TextureHandle roughness_buffer,
                            const std::array<float, 16>& view_matrix,
                            const std::array<float, 16>& projection_matrix,
                            const std::array<float, 16>& prev_view_projection_matrix);
    
    void update_config(const SSRConfig& config);
    const SSRConfig& get_config() const { return config_; }

private:
    IRenderer* renderer_;
    SSRConfig config_;
    
    TextureHandle ssr_target_;
    TextureHandle ssr_history_;
    ShaderHandle ssr_shader_;
    ShaderHandle temporal_filter_shader_;
    
    void create_render_targets();
    void create_shaders();
};

// =============================================================================
// TEMPORAL ANTI-ALIASING (TAA)
// =============================================================================

/**
 * @brief TAA configuration
 */
struct TAAConfig {
    float history_blend = 0.9f;      ///< Temporal history blend factor
    float variance_clipping = 1.0f;   ///< Variance clipping strength
    float motion_blur_scale = 1.0f;   ///< Motion blur intensity
    bool enable_sharpening = true;    ///< Post-TAA sharpening
    float sharpening_strength = 0.1f; ///< Sharpening amount
    uint32_t jitter_pattern = 8;      ///< Sample pattern (8x or 16x)
};

/**
 * @brief Professional temporal anti-aliasing
 */
class TAARenderer {
public:
    TAARenderer(IRenderer* renderer);
    ~TAARenderer();

    bool initialize(const TAAConfig& config);
    void shutdown();
    
    TextureHandle render_taa(TextureHandle current_frame, TextureHandle velocity_buffer,
                            TextureHandle depth_buffer, uint32_t frame_index);
    
    void update_config(const TAAConfig& config);
    const TAAConfig& get_config() const { return config_; }
    
    // Jitter pattern for camera projection
    std::array<float, 2> get_jitter_offset(uint32_t frame_index) const;

private:
    IRenderer* renderer_;
    TAAConfig config_;
    
    TextureHandle history_buffer_;
    TextureHandle taa_target_;
    ShaderHandle taa_shader_;
    ShaderHandle sharpen_shader_;
    
    // Jitter patterns
    std::vector<std::array<float, 2>> jitter_8x_;
    std::vector<std::array<float, 2>> jitter_16x_;
    
    void create_render_targets();
    void create_shaders();
    void initialize_jitter_patterns();
};

// =============================================================================
// PERFORMANCE OPTIMIZATIONS
// =============================================================================

/**
 * @brief Frustum culling implementation
 */
class FrustumCuller {
public:
    struct Frustum {
        std::array<std::array<float, 4>, 6> planes; ///< 6 frustum planes
    };
    
    struct AABB {
        std::array<float, 3> min_point;
        std::array<float, 3> max_point;
    };
    
    static Frustum extract_frustum(const std::array<float, 16>& view_projection_matrix);
    static bool is_aabb_in_frustum(const AABB& aabb, const Frustum& frustum);
    static bool is_sphere_in_frustum(const std::array<float, 3>& center, float radius, const Frustum& frustum);
    
    // Batch culling for performance
    static std::vector<uint32_t> cull_aabbs(std::span<const AABB> aabbs, const Frustum& frustum);
    static std::vector<uint32_t> cull_spheres(std::span<const std::array<float, 4>> spheres, const Frustum& frustum);
};

/**
 * @brief GPU-driven rendering system
 */
class GPUDrivenRenderer {
public:
    GPUDrivenRenderer(IRenderer* renderer);
    ~GPUDrivenRenderer();

    bool initialize(uint32_t max_objects = 100000);
    void shutdown();
    
    // Object management
    uint32_t register_object(const FrustumCuller::AABB& bounds, 
                           BufferHandle vertex_buffer,
                           BufferHandle index_buffer,
                           MaterialHandle material,
                           uint32_t index_count);
    
    void update_object_transform(uint32_t object_id, const std::array<float, 16>& transform);
    void remove_object(uint32_t object_id);
    
    // GPU-driven culling and rendering
    void render_frame(const std::array<float, 16>& view_projection_matrix);
    
    struct GPUStats {
        uint32_t total_objects = 0;
        uint32_t visible_objects = 0;
        uint32_t gpu_culled_objects = 0;
        uint32_t draw_calls = 0;
        float culling_time_ms = 0.0f;
        float rendering_time_ms = 0.0f;
    };
    
    GPUStats get_statistics() const { return stats_; }

private:
    IRenderer* renderer_;
    
    // GPU buffers
    BufferHandle object_data_buffer_;     ///< Object transforms and bounds
    BufferHandle draw_commands_buffer_;   ///< Indirect draw commands
    BufferHandle visibility_buffer_;     ///< Per-object visibility flags
    
    // Compute shaders
    ShaderHandle culling_shader_;
    ShaderHandle draw_generation_shader_;
    
    uint32_t max_objects_;
    uint32_t object_count_ = 0;
    mutable GPUStats stats_;
    
    void create_buffers();
    void create_shaders();
};

/**
 * @brief Batch renderer for efficient small object rendering
 */
class BatchRenderer {
public:
    BatchRenderer(IRenderer* renderer);
    ~BatchRenderer();

    bool initialize(uint32_t max_vertices = 65536, uint32_t max_indices = 196608);
    void shutdown();
    
    // Batching interface
    void begin_batch();
    void submit_quad(const std::array<std::array<float, 3>, 4>& positions,
                    const std::array<std::array<float, 2>, 4>& uvs,
                    const std::array<float, 4>& color,
                    TextureHandle texture = TextureHandle{});
    
    void submit_sprite(const std::array<float, 3>& position,
                      const std::array<float, 2>& size,
                      float rotation,
                      const std::array<float, 4>& color,
                      TextureHandle texture = TextureHandle{});
    
    void end_batch();
    void flush();
    
    struct BatchStats {
        uint32_t quads_rendered = 0;
        uint32_t draw_calls = 0;
        uint32_t texture_switches = 0;
        float batch_time_ms = 0.0f;
    };
    
    BatchStats get_statistics() const { return stats_; }

private:
    IRenderer* renderer_;
    
    // Vertex format
    struct BatchVertex {
        std::array<float, 3> position;
        std::array<float, 2> uv;
        std::array<float, 4> color;
        float texture_index;
    };
    
    // Batching data
    std::vector<BatchVertex> vertices_;
    std::vector<uint32_t> indices_;
    std::vector<TextureHandle> textures_;
    
    BufferHandle vertex_buffer_;
    BufferHandle index_buffer_;
    ShaderHandle batch_shader_;
    
    uint32_t max_vertices_;
    uint32_t max_indices_;
    uint32_t vertex_count_ = 0;
    uint32_t index_count_ = 0;
    
    mutable BatchStats stats_;
    
    void create_buffers();
    void create_shaders();
    void flush_internal();
};

// =============================================================================
// RENDER GRAPH SYSTEM
// =============================================================================

/**
 * @brief Render pass dependency graph for automatic optimization
 */
class RenderGraph {
public:
    struct PassDesc {
        std::string name;
        std::vector<TextureHandle> inputs;
        std::vector<TextureHandle> outputs;
        std::function<void()> execute;
        bool can_be_culled = true;
    };
    
    RenderGraph(IRenderer* renderer);
    ~RenderGraph();
    
    // Graph construction
    uint32_t add_pass(const PassDesc& desc);
    void add_dependency(uint32_t from_pass, uint32_t to_pass);
    
    // Execution
    void execute(TextureHandle final_target);
    void compile();
    void reset();
    
    // Optimization
    void enable_pass_culling(bool enable) { pass_culling_enabled_ = enable; }
    void enable_resource_aliasing(bool enable) { resource_aliasing_enabled_ = enable; }
    
    // Debug
    void export_graphviz(const std::string& filename) const;

private:
    IRenderer* renderer_;
    std::vector<PassDesc> passes_;
    std::vector<std::vector<uint32_t>> dependencies_;
    std::vector<uint32_t> execution_order_;
    
    bool pass_culling_enabled_ = true;
    bool resource_aliasing_enabled_ = true;
    bool compiled_ = false;
    
    void topological_sort();
    void cull_unused_passes(TextureHandle final_target);
    void optimize_resource_usage();
};

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

/**
 * @brief Calculate LOD level based on distance
 */
uint32_t calculate_lod_level(float distance, float lod_bias = 1.0f, uint32_t max_lod = 4);

/**
 * @brief Generate halton sequence for sampling
 */
std::vector<std::array<float, 2>> generate_halton_sequence(uint32_t count, uint32_t base1 = 2, uint32_t base2 = 3);

/**
 * @brief Create blue noise texture for dithering
 */
TextureHandle create_blue_noise_texture(IRenderer* renderer, uint32_t size = 64);

/**
 * @brief Compute histogram of image for auto-exposure
 */
std::array<uint32_t, 256> compute_luminance_histogram(IRenderer* renderer, TextureHandle hdr_texture);

/**
 * @brief Calculate auto-exposure value
 */
float calculate_auto_exposure(const std::array<uint32_t, 256>& histogram, 
                            float min_ev = -8.0f, float max_ev = 8.0f);

} // namespace ecscope::rendering