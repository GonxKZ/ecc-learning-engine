#pragma once

/**
 * @file renderer/batch_renderer.hpp
 * @brief Sprite Batching System for ECScope Educational ECS Engine - Phase 7: Renderizado 2D
 * 
 * This header provides an advanced sprite batching system designed for educational
 * clarity while achieving maximum rendering performance. It includes:
 * 
 * Core Features:
 * - Intelligent sprite batching with texture atlas support
 * - Dynamic vertex buffer management with memory pooling
 * - Multi-threaded batch generation for complex scenes
 * - Comprehensive sorting strategies for optimal rendering order
 * - GPU-optimized vertex layouts and instanced rendering
 * 
 * Educational Features:
 * - Detailed documentation of batching concepts and benefits
 * - Performance analysis and batch efficiency metrics
 * - Visual batch debugging with color-coded rendering
 * - Memory usage tracking and optimization guidance
 * - Interactive batching parameter tuning for learning
 * 
 * Advanced Features:
 * - Automatic texture atlas generation and management
 * - Dynamic batch splitting for memory constraints
 * - Frustum culling integration for large worlds
 * - Z-order preservation with depth sorting
 * - Multi-material batching with shader variants
 * 
 * Performance Characteristics:
 * - Minimizes GPU state changes and draw calls
 * - Optimizes vertex cache utilization
 * - Reduces CPU-GPU synchronization overhead  
 * - Memory-efficient vertex data packing
 * - Background batch preparation for smooth frame rates
 * 
 * @author ECScope Educational ECS Framework - Phase 7: Renderizado 2D
 * @date 2024
 */

#include "components/render_components.hpp"
#include "resources/texture.hpp"
#include "resources/shader.hpp"
#include "../memory/memory_tracker.hpp"
#include "../core/types.hpp"
#include <vector>
#include <array>
#include <memory>
#include <unordered_map>
#include <algorithm>

namespace ecscope::renderer {

// Import commonly used types
using namespace components;
using namespace resources;

//=============================================================================
// Forward Declarations
//=============================================================================

class Renderer2D;
struct SpriteBatch;
struct BatchVertex;

//=============================================================================
// Vertex Data Structures
//=============================================================================

/**
 * @brief Optimized Sprite Vertex Data
 * 
 * Defines the vertex layout for sprite rendering with efficient packing
 * for GPU consumption. Each sprite uses 4 vertices arranged as a quad.
 * 
 * Educational Context:
 * This structure demonstrates:
 * - GPU-friendly vertex layout design
 * - Memory alignment for optimal transfer
 * - Attribute interleaving vs separation trade-offs
 * - Color space and precision considerations
 */
struct alignas(32) BatchVertex {
    //-------------------------------------------------------------------------
    // Position Data (8 bytes)
    //-------------------------------------------------------------------------
    f32 position_x;         ///< World X coordinate  
    f32 position_y;         ///< World Y coordinate
    
    //-------------------------------------------------------------------------
    // Texture Coordinates (8 bytes)
    //-------------------------------------------------------------------------
    f32 texture_u;          ///< Texture U coordinate (0-1)
    f32 texture_v;          ///< Texture V coordinate (0-1)
    
    //-------------------------------------------------------------------------
    // Color Data (4 bytes)
    //-------------------------------------------------------------------------
    u32 color_rgba;         ///< Packed RGBA color (8 bits per channel)
    
    //-------------------------------------------------------------------------
    // Metadata (4 bytes)
    //-------------------------------------------------------------------------
    union {
        u32 metadata;
        struct {
            u16 texture_id : 12;    ///< Texture ID (up to 4096 textures)
            u16 blend_mode : 3;     ///< Blend mode (up to 8 modes)
            u16 reserved : 1;       ///< Reserved for future use
            u16 batch_id : 16;      ///< Batch ID for debugging
        };
    };
    
    //-------------------------------------------------------------------------
    // Construction
    //-------------------------------------------------------------------------
    
    constexpr BatchVertex() noexcept = default;
    
    constexpr BatchVertex(f32 x, f32 y, f32 u, f32 v, Color color, u16 tex_id = 0) noexcept
        : position_x(x), position_y(y)
        , texture_u(u), texture_v(v)
        , color_rgba(color.rgba)
        , metadata(static_cast<u32>(tex_id)) {}
    
    //-------------------------------------------------------------------------
    // Utility Functions
    //-------------------------------------------------------------------------
    
    /** @brief Set color from Color struct */
    void set_color(const Color& color) noexcept {
        color_rgba = color.rgba;
    }
    
    /** @brief Get color as Color struct */
    Color get_color() const noexcept {
        return Color{color_rgba};
    }
    
    /** @brief Set texture ID */
    void set_texture_id(u16 id) noexcept {
        texture_id = id & 0xFFF; // Mask to 12 bits
    }
    
    /** @brief Set blend mode */
    void set_blend_mode(RenderableSprite::BlendMode mode) noexcept {
        blend_mode = static_cast<u16>(mode) & 0x7; // Mask to 3 bits
    }
    
    /** @brief Calculate memory footprint */
    static constexpr usize get_size() noexcept { return sizeof(BatchVertex); }
    
    /** @brief Validate vertex data */
    bool is_valid() const noexcept {
        return texture_u >= 0.0f && texture_u <= 1.0f &&
               texture_v >= 0.0f && texture_v <= 1.0f;
    }
};

// Ensure optimal vertex size for GPU consumption
static_assert(sizeof(BatchVertex) == 24, "BatchVertex should be exactly 24 bytes");
static_assert(alignof(BatchVertex) >= 8, "BatchVertex should be at least 8-byte aligned");

/**
 * @brief Sprite Quad Index Pattern
 * 
 * Defines the index pattern for rendering sprite quads as triangles.
 * Each sprite quad uses 6 indices forming 2 triangles.
 * 
 * Educational Context:
 * Index buffers allow vertices to be reused, reducing memory usage
 * and improving cache performance. The pattern creates counter-clockwise
 * triangles for consistent front-face orientation.
 */
struct QuadIndices {
    // Triangle 1: top-left, bottom-left, top-right (CCW)
    // Triangle 2: top-right, bottom-left, bottom-right (CCW)
    static constexpr std::array<u16, 6> PATTERN = {0, 1, 2, 2, 1, 3};
    
    /** @brief Generate indices for sprite quad at specified vertex offset */
    static void generate_quad_indices(u16* indices, u16 vertex_offset) noexcept {
        for (usize i = 0; i < 6; ++i) {
            indices[i] = vertex_offset + PATTERN[i];
        }
    }
    
    /** @brief Calculate number of indices needed for sprite count */
    static constexpr usize indices_for_sprites(usize sprite_count) noexcept {
        return sprite_count * 6;
    }
    
    /** @brief Calculate number of vertices needed for sprite count */
    static constexpr usize vertices_for_sprites(usize sprite_count) noexcept {
        return sprite_count * 4;
    }
};

//=============================================================================
// Batching Strategies and Sorting
//=============================================================================

/**
 * @brief Sprite Batching Strategy
 * 
 * Defines different strategies for grouping sprites into batches.
 * Each strategy optimizes for different scenarios and performance characteristics.
 * 
 * Educational Context:
 * Different batching strategies demonstrate the trade-offs between:
 * - Draw call reduction vs state change minimization
 * - Memory usage vs performance
 * - Visual quality vs rendering speed
 */
enum class BatchingStrategy : u8 {
    /**
     * @brief Texture-First Batching
     * 
     * Groups sprites by texture first, then by material properties.
     * Minimizes texture binding changes at the cost of more draw calls.
     * 
     * Best for: Scenes with few textures but many materials
     * Performance: Excellent texture cache utilization
     */
    TextureFirst = 0,
    
    /**
     * @brief Material-First Batching  
     * 
     * Groups sprites by material (shader + render state) first.
     * Minimizes expensive render state changes but may increase texture binds.
     * 
     * Best for: Scenes with complex materials and effects
     * Performance: Reduces GPU pipeline stalls from state changes
     */
    MaterialFirst,
    
    /**
     * @brief Z-Order Preserving
     * 
     * Maintains depth sorting order while opportunistically batching.
     * Ensures correct transparency rendering at some performance cost.
     * 
     * Best for: Scenes with many transparent objects requiring depth sorting
     * Performance: Moderate - balances correctness and performance
     */
    ZOrderPreserving,
    
    /**
     * @brief Spatial Locality
     * 
     * Groups sprites based on spatial proximity in world coordinates.
     * Optimizes for vertex cache utilization and culling efficiency.
     * 
     * Best for: Large worlds with spatial coherence
     * Performance: Excellent vertex cache performance
     */
    SpatialLocality,
    
    /**
     * @brief Adaptive Hybrid
     * 
     * Dynamically chooses strategy based on current scene characteristics.
     * Analyzes frame composition to select optimal batching approach.
     * 
     * Best for: Variable scenes with changing characteristics
     * Performance: Good overall with automatic optimization
     */
    AdaptiveHybrid
};

/**
 * @brief Sprite Sorting Criteria
 * 
 * Defines sorting criteria for sprites within and between batches.
 * Multiple criteria can be combined with different priorities.
 */
struct SortingCriteria {
    /** @brief Primary sorting criteria */
    enum class Primary : u8 {
        None = 0,           ///< No sorting (submission order)
        ZOrder,             ///< Sort by Z-order (depth)
        TextureID,          ///< Sort by texture binding
        MaterialID,         ///< Sort by material properties
        DistanceToCamera,   ///< Sort by distance from camera
        YPosition           ///< Sort by Y coordinate (painter's algorithm)
    };
    
    /** @brief Secondary sorting criteria (for tie-breaking) */
    enum class Secondary : u8 {
        None = 0,
        EntityID,           ///< Sort by entity ID (for determinism)
        TextureID,          ///< Secondary texture sorting
        XPosition,          ///< Sort by X coordinate
        SubmissionOrder     ///< Preserve submission order
    };
    
    Primary primary{Primary::ZOrder};
    Secondary secondary{Secondary::EntityID};
    bool reverse_primary{false};        ///< Reverse primary sort order
    bool reverse_secondary{false};      ///< Reverse secondary sort order
    
    /** @brief Default Z-order sorting (back-to-front for transparency) */
    static constexpr SortingCriteria z_order_back_to_front() noexcept {
        return {Primary::ZOrder, Secondary::EntityID, false, false};
    }
    
    /** @brief Front-to-back sorting (for early Z rejection) */
    static constexpr SortingCriteria z_order_front_to_back() noexcept {
        return {Primary::ZOrder, Secondary::EntityID, true, false};
    }
    
    /** @brief Texture-optimized sorting */
    static constexpr SortingCriteria texture_optimized() noexcept {
        return {Primary::TextureID, Secondary::ZOrder, false, false};
    }
    
    /** @brief Spatial locality sorting */
    static constexpr SortingCriteria spatial_locality() noexcept {
        return {Primary::YPosition, Secondary::XPosition, false, false};
    }
};

//=============================================================================
// Sprite Batch Management
//=============================================================================

/**
 * @brief Individual Sprite Batch
 * 
 * Represents a group of sprites that can be rendered with a single draw call.
 * Contains vertex data, render state, and performance metrics.
 * 
 * Educational Context:
 * A batch demonstrates how multiple sprites can be combined into a single
 * GPU operation, dramatically reducing CPU overhead and improving performance.
 */
class SpriteBatch {
public:
    //-------------------------------------------------------------------------
    // Batch Properties
    //-------------------------------------------------------------------------
    
    /** @brief Maximum sprites per batch */
    static constexpr usize MAX_SPRITES_PER_BATCH = 1000;
    
    /** @brief Maximum vertices per batch */
    static constexpr usize MAX_VERTICES = MAX_SPRITES_PER_BATCH * 4;
    
    /** @brief Maximum indices per batch */
    static constexpr usize MAX_INDICES = MAX_SPRITES_PER_BATCH * 6;
    
    //-------------------------------------------------------------------------
    // Construction and Management
    //-------------------------------------------------------------------------
    
    /** @brief Create empty sprite batch */
    SpriteBatch() noexcept;
    
    /** @brief Destructor releases GPU resources */
    ~SpriteBatch() noexcept;
    
    // Move-only semantics (batches manage GPU resources)
    SpriteBatch(const SpriteBatch&) = delete;
    SpriteBatch& operator=(const SpriteBatch&) = delete;
    SpriteBatch(SpriteBatch&& other) noexcept;
    SpriteBatch& operator=(SpriteBatch&& other) noexcept;
    
    //-------------------------------------------------------------------------
    // Batch State and Information
    //-------------------------------------------------------------------------
    
    /** @brief Get number of sprites in batch */
    usize get_sprite_count() const noexcept { return sprite_count_; }
    
    /** @brief Get number of vertices in batch */
    usize get_vertex_count() const noexcept { return sprite_count_ * 4; }
    
    /** @brief Get number of indices in batch */
    usize get_index_count() const noexcept { return sprite_count_ * 6; }
    
    /** @brief Check if batch is full */
    bool is_full() const noexcept { return sprite_count_ >= MAX_SPRITES_PER_BATCH; }
    
    /** @brief Check if batch is empty */
    bool is_empty() const noexcept { return sprite_count_ == 0; }
    
    /** @brief Get primary texture ID for this batch */
    TextureID get_primary_texture() const noexcept { return primary_texture_id_; }
    
    /** @brief Get material hash for batching compatibility */
    u64 get_material_hash() const noexcept { return material_hash_; }
    
    /** @brief Get batch memory usage */
    usize get_memory_usage() const noexcept;
    
    //-------------------------------------------------------------------------
    // Sprite Addition and Building
    //-------------------------------------------------------------------------
    
    /** @brief Check if sprite can be added to this batch */
    bool can_add_sprite(const RenderableSprite& sprite, const Transform& transform) const noexcept;
    
    /** @brief Add sprite to batch */
    bool add_sprite(const RenderableSprite& sprite, const Transform& transform) noexcept;
    
    /** @brief Reserve space for sprites */
    void reserve(usize sprite_count) noexcept;
    
    /** @brief Clear batch contents */
    void clear() noexcept;
    
    /** @brief Finalize batch and prepare for rendering */
    void finalize() noexcept;
    
    /** @brief Check if batch is finalized and ready for rendering */
    bool is_finalized() const noexcept { return is_finalized_; }
    
    //-------------------------------------------------------------------------
    // Rendering Interface
    //-------------------------------------------------------------------------
    
    /** @brief Render the entire batch */
    void render(Renderer2D& renderer) const noexcept;
    
    /** @brief Get OpenGL vertex array object ID */
    u32 get_vao_id() const noexcept { return vao_id_; }
    
    /** @brief Get vertex buffer object ID */
    u32 get_vbo_id() const noexcept { return vbo_id_; }
    
    /** @brief Get index buffer object ID */
    u32 get_ibo_id() const noexcept { return ibo_id_; }
    
    //-------------------------------------------------------------------------
    // Performance and Debug Information
    //-------------------------------------------------------------------------
    
    /** @brief Batch performance statistics */
    struct BatchStats {
        u32 render_count{0};            ///< Number of times batch was rendered
        f32 total_render_time{0.0f};    ///< Total GPU time spent rendering
        f32 average_render_time{0.0f};  ///< Average render time per draw call
        u32 vertex_cache_misses{0};     ///< Estimated vertex cache misses
        f32 fill_rate_impact{0.0f};     ///< Impact on GPU fill rate
        
        // Memory efficiency
        f32 vertex_buffer_utilization{0.0f}; ///< How full the vertex buffer is
        usize memory_overhead{0};       ///< Memory overhead per sprite
        
        // Batching efficiency
        bool was_split{false};          ///< Whether batch was split due to limits
        u32 texture_switches{0};        ///< Number of texture changes in batch
        f32 batching_effectiveness{1.0f}; ///< How effective the batching was
    };
    
    /** @brief Get performance statistics */
    const BatchStats& get_stats() const noexcept { return stats_; }
    
    /** @brief Reset performance counters */
    void reset_stats() noexcept { stats_ = {}; }
    
    /** @brief Get debug information for educational analysis */
    struct DebugInfo {
        std::string description;        ///< Human-readable batch description
        Color debug_tint;               ///< Color for batch visualization
        f32 complexity_score;           ///< Rendering complexity (1.0 = simple sprite)
        const char* optimization_hints[4]; ///< Performance suggestions
        u32 hint_count{0};
        
        // Sprite composition
        u32 opaque_sprites{0};          ///< Number of opaque sprites
        u32 transparent_sprites{0};     ///< Number of transparent sprites
        u32 unique_textures{0};         ///< Number of different textures used
        f32 average_sprite_size{0.0f};  ///< Average sprite size in pixels
    };
    
    /** @brief Get debug information */
    DebugInfo get_debug_info() const noexcept;
    
    /** @brief Validate batch state */
    bool validate() const noexcept;

private:
    //-------------------------------------------------------------------------
    // Internal State
    //-------------------------------------------------------------------------
    
    // Batch composition
    usize sprite_count_{0};
    TextureID primary_texture_id_{INVALID_TEXTURE_ID};
    u64 material_hash_{0};
    bool is_finalized_{false};
    
    // Vertex data (CPU-side buffer)
    std::vector<BatchVertex> vertices_;
    std::vector<u16> indices_;
    
    // GPU resources
    u32 vao_id_{0};                     ///< Vertex Array Object
    u32 vbo_id_{0};                     ///< Vertex Buffer Object
    u32 ibo_id_{0};                     ///< Index Buffer Object
    bool gpu_resources_created_{false};
    
    // Performance tracking
    mutable BatchStats stats_{};
    
    // Render state caching
    struct RenderState {
        RenderableSprite::BlendMode blend_mode{RenderableSprite::BlendMode::Alpha};
        bool depth_test_enabled{false};
        bool depth_write_enabled{false};
        f32 z_order_min{0.0f};          ///< Minimum Z-order in batch
        f32 z_order_max{0.0f};          ///< Maximum Z-order in batch
    } render_state_;
    
    //-------------------------------------------------------------------------
    // Internal Methods
    //-------------------------------------------------------------------------
    
    void create_gpu_resources() noexcept;
    void destroy_gpu_resources() noexcept;
    void upload_vertex_data() const noexcept;
    void generate_indices() noexcept;
    void update_render_state(const RenderableSprite& sprite) noexcept;
    void calculate_material_hash(const RenderableSprite& sprite) noexcept;
    void record_render() const noexcept;
    
    friend class BatchRenderer;
};

//=============================================================================
// Main Batch Renderer
//=============================================================================

/**
 * @brief Advanced Sprite Batch Renderer
 * 
 * Manages the entire sprite batching pipeline including batch creation,
 * optimization, sorting, and rendering. Provides comprehensive educational
 * features for understanding modern 2D rendering techniques.
 * 
 * Educational Context:
 * This system demonstrates:
 * - Modern GPU-optimized rendering architecture
 * - The importance of minimizing state changes
 * - Trade-offs between memory usage and performance
 * - Dynamic resource management and optimization
 * - Performance monitoring and analysis techniques
 */
class BatchRenderer {
public:
    //-------------------------------------------------------------------------
    // Configuration
    //-------------------------------------------------------------------------
    
    /** @brief Batch renderer configuration */
    struct Config {
        BatchingStrategy strategy{BatchingStrategy::AdaptiveHybrid};
        SortingCriteria sorting{SortingCriteria::z_order_back_to_front()};
        
        // Performance settings
        usize max_batches_per_frame{100};       ///< Maximum batches per frame
        usize max_sprites_per_batch{1000};      ///< Sprites per batch limit
        bool enable_dynamic_batching{true};     ///< Allow dynamic batch resizing
        bool enable_frustum_culling{true};      ///< Cull sprites outside view
        
        // Memory management
        usize vertex_buffer_pool_size{64};      ///< Number of vertex buffers to pool
        bool enable_buffer_streaming{true};     ///< Stream vertex data for large batches
        f32 buffer_growth_factor{1.5f};         ///< Buffer growth multiplier
        
        // Educational features
        bool collect_detailed_stats{true};      ///< Collect comprehensive statistics
        bool enable_batch_visualization{false}; ///< Color-code batches for debugging
        bool log_batching_decisions{false};     ///< Log batching strategy decisions
        u32 performance_analysis_frequency{60}; ///< Frames between analysis updates
        
        /** @brief Performance-focused configuration */
        static Config performance_mode() noexcept {
            Config config;
            config.strategy = BatchingStrategy::MaterialFirst;
            config.max_sprites_per_batch = 2000;
            config.enable_buffer_streaming = true;
            config.collect_detailed_stats = false;
            return config;
        }
        
        /** @brief Educational configuration with comprehensive analysis */
        static Config educational_mode() noexcept {
            Config config;
            config.strategy = BatchingStrategy::AdaptiveHybrid;
            config.max_sprites_per_batch = 500; // Smaller for clearer analysis
            config.collect_detailed_stats = true;
            config.enable_batch_visualization = true;
            config.log_batching_decisions = true;
            return config;
        }
    };
    
    //-------------------------------------------------------------------------
    // Construction and Initialization
    //-------------------------------------------------------------------------
    
    /** @brief Create batch renderer with configuration */
    explicit BatchRenderer(const Config& config = Config{}) noexcept;
    
    /** @brief Destructor cleans up all resources */
    ~BatchRenderer() noexcept;
    
    // Move-only semantics
    BatchRenderer(const BatchRenderer&) = delete;
    BatchRenderer& operator=(const BatchRenderer&) = delete;
    BatchRenderer(BatchRenderer&& other) noexcept;
    BatchRenderer& operator=(BatchRenderer&& other) noexcept;
    
    /** @brief Initialize renderer resources */
    bool initialize() noexcept;
    
    /** @brief Shutdown and cleanup */
    void shutdown() noexcept;
    
    /** @brief Check if initialized */
    bool is_initialized() const noexcept { return initialized_; }
    
    //-------------------------------------------------------------------------
    // Frame Management
    //-------------------------------------------------------------------------
    
    /** @brief Begin new batching frame */
    void begin_frame() noexcept;
    
    /** @brief End frame and prepare batches for rendering */
    void end_frame() noexcept;
    
    /** @brief Check if frame is active */
    bool is_frame_active() const noexcept { return frame_active_; }
    
    //-------------------------------------------------------------------------
    // Sprite Submission Interface
    //-------------------------------------------------------------------------
    
    /** @brief Submit sprite for batching */
    void submit_sprite(const RenderableSprite& sprite, const Transform& transform) noexcept;
    
    /** @brief Submit multiple sprites */
    void submit_sprites(const std::vector<std::pair<RenderableSprite, Transform>>& sprites) noexcept;
    
    /** @brief Get number of submitted sprites this frame */
    usize get_submitted_sprite_count() const noexcept { return submitted_sprites_.size(); }
    
    //-------------------------------------------------------------------------
    // Batch Generation and Optimization
    //-------------------------------------------------------------------------
    
    /** @brief Generate batches from submitted sprites */
    void generate_batches() noexcept;
    
    /** @brief Optimize existing batches */
    void optimize_batches() noexcept;
    
    /** @brief Sort batches for optimal rendering order */
    void sort_batches() noexcept;
    
    /** @brief Get generated batches */
    const std::vector<std::unique_ptr<SpriteBatch>>& get_batches() const noexcept { return batches_; }
    
    /** @brief Get number of batches */
    usize get_batch_count() const noexcept { return batches_.size(); }
    
    //-------------------------------------------------------------------------
    // Rendering Interface
    //-------------------------------------------------------------------------
    
    /** @brief Render all batches */
    void render_all(Renderer2D& renderer) noexcept;
    
    /** @brief Render specific batch */
    void render_batch(usize batch_index, Renderer2D& renderer) noexcept;
    
    /** @brief Get estimated GPU cost for all batches */
    f32 get_estimated_gpu_cost() const noexcept;
    
    //-------------------------------------------------------------------------
    // Performance Monitoring and Statistics
    //-------------------------------------------------------------------------
    
    /** @brief Comprehensive batching statistics */
    struct BatchingStatistics {
        // Frame statistics
        u32 frame_number{0};
        usize sprites_submitted{0};
        usize batches_generated{0};
        f32 batching_efficiency{0.0f};          ///< Sprites per batch ratio
        
        // Performance metrics
        f32 batch_generation_time_ms{0.0f};     ///< Time spent creating batches
        f32 sorting_time_ms{0.0f};              ///< Time spent sorting
        f32 optimization_time_ms{0.0f};         ///< Time spent optimizing
        f32 total_batching_time_ms{0.0f};       ///< Total batching overhead
        
        // Memory usage
        usize vertex_buffer_memory{0};          ///< Memory used by vertex buffers
        usize index_buffer_memory{0};           ///< Memory used by index buffers
        usize batch_metadata_memory{0};         ///< Memory used by batch management
        usize total_batching_memory{0};         ///< Total memory used by batching
        
        // Strategy analysis
        BatchingStrategy active_strategy;       ///< Currently active strategy
        const char* strategy_effectiveness{"Unknown"}; ///< How well strategy is working
        u32 strategy_switches{0};              ///< Number of strategy changes this frame
        
        // Quality metrics
        f32 texture_coherence{1.0f};           ///< How well textures are grouped
        f32 depth_coherence{1.0f};             ///< How well depth is preserved
        f32 spatial_coherence{1.0f};           ///< How well spatial locality is maintained
        u32 batch_breaks{0};                   ///< Number of forced batch breaks
        
        // Educational insights
        std::vector<std::string> performance_insights; ///< Performance analysis
        std::vector<std::string> optimization_suggestions; ///< Improvement suggestions
        const char* bottleneck_analysis{"None"}; ///< Primary performance bottleneck
    };
    
    /** @brief Get comprehensive statistics */
    const BatchingStatistics& get_statistics() const noexcept { return statistics_; }
    
    /** @brief Reset performance counters */
    void reset_statistics() noexcept;
    
    /** @brief Update configuration */
    void update_config(const Config& new_config) noexcept;
    
    /** @brief Get current configuration */
    const Config& get_config() const noexcept { return config_; }
    
    //-------------------------------------------------------------------------
    // Educational and Debug Features
    //-------------------------------------------------------------------------
    
    /** @brief Enable/disable batch visualization */
    void set_batch_visualization_enabled(bool enabled) noexcept {
        config_.enable_batch_visualization = enabled;
    }
    
    /** @brief Get batch debug colors */
    std::vector<Color> get_batch_debug_colors() const noexcept;
    
    /** @brief Generate detailed batching report */
    std::string generate_batching_report() const noexcept;
    
    /** @brief Analyze batching effectiveness */
    std::string analyze_batching_effectiveness() const noexcept;
    
    /** @brief Get memory usage breakdown */
    struct MemoryBreakdown {
        usize vertex_data{0};
        usize index_data{0};
        usize batch_metadata{0};
        usize gpu_buffers{0};
        usize total{0};
        
        f32 efficiency() const noexcept {
            return total > 0 ? static_cast<f32>(vertex_data + index_data) / total : 0.0f;
        }
    };
    
    MemoryBreakdown get_memory_breakdown() const noexcept;
    
    //-------------------------------------------------------------------------
    // Advanced Features
    //-------------------------------------------------------------------------
    
    /** @brief Force specific batching strategy */
    void set_batching_strategy(BatchingStrategy strategy) noexcept;
    
    /** @brief Enable adaptive strategy selection */
    void enable_adaptive_strategy(bool enabled = true) noexcept;
    
    /** @brief Set custom sorting criteria */
    void set_sorting_criteria(const SortingCriteria& criteria) noexcept;
    
    /** @brief Preload resources for known sprite configurations */
    void preload_batch_resources(usize expected_sprite_count) noexcept;

private:
    //-------------------------------------------------------------------------
    // Internal Data Structures
    //-------------------------------------------------------------------------
    
    /** @brief Submitted sprite with metadata */
    struct SubmittedSprite {
        RenderableSprite sprite;
        Transform transform;
        f32 sort_key;                   ///< Calculated sort key
        f32 distance_to_camera;         ///< Distance from camera (for culling)
        u32 submission_order;           ///< Order of submission
        bool is_visible;                ///< Whether sprite passed culling
        
        SubmittedSprite(const RenderableSprite& s, const Transform& t, u32 order) noexcept
            : sprite(s), transform(t), sort_key(s.z_order), distance_to_camera(0.0f), 
              submission_order(order), is_visible(true) {}
    };
    
    Config config_;
    bool initialized_{false};
    bool frame_active_{false};
    u32 frame_number_{0};
    
    // Sprite submission
    std::vector<SubmittedSprite> submitted_sprites_;
    memory::ArenaAllocator sprite_allocator_;
    
    // Batch management
    std::vector<std::unique_ptr<SpriteBatch>> batches_;
    std::vector<std::unique_ptr<SpriteBatch>> batch_pool_; // Reusable batches
    memory::PoolAllocator batch_allocator_;
    
    // Performance tracking
    mutable BatchingStatistics statistics_;
    
    // Strategy management
    BatchingStrategy current_strategy_;
    std::array<f32, 5> strategy_effectiveness_; // Effectiveness scores for each strategy
    
    //-------------------------------------------------------------------------
    // Internal Methods
    //-------------------------------------------------------------------------
    
    // Sprite processing
    void calculate_sort_keys() noexcept;
    void perform_frustum_culling() noexcept;
    void sort_submitted_sprites() noexcept;
    
    // Batch generation strategies
    void generate_batches_texture_first() noexcept;
    void generate_batches_material_first() noexcept;
    void generate_batches_z_order_preserving() noexcept;
    void generate_batches_spatial_locality() noexcept;
    void generate_batches_adaptive_hybrid() noexcept;
    
    // Batch optimization
    void merge_compatible_batches() noexcept;
    void split_oversized_batches() noexcept;
    void reorder_batches_for_performance() noexcept;
    
    // Performance analysis
    void update_statistics() noexcept;
    void analyze_strategy_effectiveness() noexcept;
    void update_adaptive_strategy() noexcept;
    
    // Resource management
    std::unique_ptr<SpriteBatch> acquire_batch() noexcept;
    void release_batch(std::unique_ptr<SpriteBatch> batch) noexcept;
    void cleanup_resources() noexcept;
    
    // Utility functions
    bool can_merge_batches(const SpriteBatch& a, const SpriteBatch& b) const noexcept;
    f32 calculate_batch_sort_key(const SpriteBatch& batch) const noexcept;
    Color get_batch_visualization_color(usize batch_index) const noexcept;
    void log_batching_decision(const char* decision, const char* reason) const noexcept;
};

//=============================================================================
// Utility Functions and Helpers
//=============================================================================

namespace batching_utils {
    /** @brief Calculate optimal batch size for sprite count */
    usize calculate_optimal_batch_size(usize sprite_count, usize memory_limit) noexcept;
    
    /** @brief Estimate batching overhead */
    f32 estimate_batching_overhead(usize sprite_count, usize batch_count) noexcept;
    
    /** @brief Calculate texture coherence score */
    f32 calculate_texture_coherence(const std::vector<TextureID>& texture_sequence) noexcept;
    
    /** @brief Analyze sprite distribution for optimal strategy */
    BatchingStrategy analyze_optimal_strategy(const std::vector<RenderableSprite>& sprites) noexcept;
    
    /** @brief Generate batch debug name */
    std::string generate_batch_debug_name(const SpriteBatch& batch) noexcept;
    
    /** @brief Calculate vertex cache utilization */
    f32 calculate_vertex_cache_utilization(const std::vector<u16>& indices) noexcept;
}

} // namespace ecscope::renderer