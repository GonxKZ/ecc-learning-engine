#pragma once

/**
 * @file renderer/renderer_2d.hpp
 * @brief 2D Renderer System for ECScope Educational ECS Engine - Phase 7: Renderizado 2D
 * 
 * This header provides a comprehensive 2D rendering system designed for educational
 * clarity while maintaining professional-grade performance. It includes:
 * 
 * Core Features:
 * - Modern OpenGL 3.3+ based 2D rendering pipeline
 * - Efficient sprite batching system for optimal draw call reduction
 * - Multiple camera support with viewport management
 * - Advanced material system with custom shader support
 * - Comprehensive debug rendering and performance analysis
 * 
 * Educational Features:
 * - Detailed documentation of 2D rendering concepts and techniques
 * - Performance metrics and bottleneck identification
 * - Visual debugging tools for render pipeline inspection
 * - Memory usage tracking and optimization guidance
 * - Interactive rendering parameter modification for learning
 * 
 * Advanced Features:
 * - Multi-pass rendering support for complex effects
 * - Render-to-texture capabilities for post-processing
 * - Dynamic lighting system for 2D scenes
 * - Particle system integration and rendering
 * - UI rendering with proper depth layering
 * 
 * Performance Characteristics:
 * - Automatic sprite batching with texture atlas support
 * - GPU-optimized vertex buffer management
 * - Frustum culling for large scenes
 * - Multi-threaded render command generation
 * - Memory pool allocation for render commands
 * 
 * ECS Integration:
 * - Seamless integration with ECScope component system
 * - System-based rendering with automatic component queries
 * - Event-driven rendering updates and notifications
 * - Memory tracking integration for resource monitoring
 * 
 * @author ECScope Educational ECS Framework - Phase 7: Renderizado 2D
 * @date 2024
 */

#include "components/render_components.hpp"
#include "resources/texture.hpp"
#include "resources/shader.hpp"
#include "../ecs/registry.hpp"
#include "../ecs/components/transform.hpp"
#include "../memory/memory_tracker.hpp"
#include "../core/types.hpp"
#include "../core/result.hpp"
#include <vector>
#include <memory>
#include <unordered_map>
#include <queue>
#include <functional>

namespace ecscope::renderer {

// Import commonly used types
using namespace components;
using namespace resources;
using namespace ecs::components;

//=============================================================================
// Forward Declarations
//=============================================================================

class Renderer2D;
class BatchRenderer;
class RenderCommand;
struct RenderStatistics;

//=============================================================================
// Render Command System
//=============================================================================

/**
 * @brief Render Command Types
 * 
 * Defines different types of rendering operations that can be queued
 * and executed by the renderer. Each command type has specific data
 * requirements and execution characteristics.
 * 
 * Educational Context:
 * Command-based rendering allows for:
 * - Batching similar operations for efficiency
 * - Multi-threaded render command generation
 * - Easy debugging and profiling of render operations
 * - Flexible render pipeline modification
 */
enum class RenderCommandType : u8 {
    DrawSprite = 0,     ///< Draw a single sprite quad
    DrawBatch,          ///< Draw a batch of sprites with same material
    DrawDebugLine,      ///< Draw debug line
    DrawDebugBox,       ///< Draw debug bounding box
    DrawDebugCircle,    ///< Draw debug circle
    SetCamera,          ///< Set active camera for subsequent draws
    SetMaterial,        ///< Set rendering material/shader
    SetRenderTarget,    ///< Set render target (framebuffer)
    ClearTarget,        ///< Clear render target
    PushDebugGroup,     ///< Begin debug annotation group
    PopDebugGroup       ///< End debug annotation group
};

/**
 * @brief Base Render Command
 * 
 * Base structure for all rendering commands. Contains common data
 * and virtual interface for command execution.
 * 
 * Educational Context:
 * The command pattern allows for flexible rendering pipeline
 * organization and makes it easy to analyze rendering workload.
 */
struct alignas(16) RenderCommand {
    RenderCommandType type;         ///< Command type
    f32 sort_key;                   ///< Sorting key for render order
    u16 camera_id;                  ///< Camera ID for multi-viewport rendering
    u16 flags;                      ///< Command-specific flags
    
    // Educational debug information
    const char* debug_name{"Unknown"}; ///< Human-readable name for debugging
    u32 entity_id{0};              ///< Source entity ID (if applicable)
    
    constexpr RenderCommand(RenderCommandType cmd_type, f32 key = 0.0f, u16 cam_id = 0) noexcept
        : type(cmd_type), sort_key(key), camera_id(cam_id), flags(0) {}
    
    virtual ~RenderCommand() = default;
    
    /** @brief Execute the render command */
    virtual void execute(Renderer2D& renderer) const = 0;
    
    /** @brief Get estimated GPU cost */
    virtual f32 estimate_gpu_cost() const noexcept { return 1.0f; }
    
    /** @brief Get memory footprint of command */
    virtual usize get_memory_size() const noexcept { return sizeof(RenderCommand); }
};

/**
 * @brief Sprite Rendering Command
 * 
 * Command for rendering a single sprite with full transformation
 * and material properties.
 */
struct SpriteRenderCommand : public RenderCommand {
    // Transform data
    struct {
        f32 x, y;                   ///< World position
        f32 rotation;               ///< Rotation in radians
        f32 scale_x, scale_y;       ///< Scale factors
    } transform;
    
    // Sprite data
    TextureHandle texture;          ///< Texture to render
    UVRect uv_rect;                 ///< UV coordinates
    Color color;                    ///< Color modulation
    
    // Rendering properties
    f32 z_order;                    ///< Depth sorting order
    BlendMode blend_mode;           ///< Blending mode
    
    SpriteRenderCommand(f32 x, f32 y, TextureHandle tex, f32 z = 0.0f) noexcept
        : RenderCommand(RenderCommandType::DrawSprite, z)
        , transform{x, y, 0.0f, 1.0f, 1.0f}
        , texture(tex)
        , uv_rect(UVRect::full_texture())
        , color(Color::white())
        , z_order(z)
        , blend_mode(BlendMode::Alpha) {}
    
    void execute(Renderer2D& renderer) const override;
    f32 estimate_gpu_cost() const noexcept override { return 1.2f; }
    usize get_memory_size() const noexcept override { return sizeof(SpriteRenderCommand); }
};

/**
 * @brief Debug Line Rendering Command
 * 
 * Command for rendering debug lines, wireframes, and geometric primitives.
 */
struct DebugLineCommand : public RenderCommand {
    struct {
        f32 start_x, start_y;       ///< Line start position
        f32 end_x, end_y;           ///< Line end position
    } line;
    
    Color color;                    ///< Line color
    f32 thickness;                  ///< Line thickness in pixels
    
    DebugLineCommand(f32 sx, f32 sy, f32 ex, f32 ey, Color c = Color::white(), f32 thick = 1.0f) noexcept
        : RenderCommand(RenderCommandType::DrawDebugLine, 1000.0f) // Debug renders on top
        , line{sx, sy, ex, ey}
        , color(c)
        , thickness(thick) {}
    
    void execute(Renderer2D& renderer) const override;
    f32 estimate_gpu_cost() const noexcept override { return 0.8f; }
};

/**
 * @brief Camera Set Command
 * 
 * Command to switch active camera for subsequent rendering operations.
 */
struct CameraSetCommand : public RenderCommand {
    u32 camera_entity_id;           ///< Entity ID of camera component
    
    explicit CameraSetCommand(u32 cam_entity) noexcept
        : RenderCommand(RenderCommandType::SetCamera, -1000.0f) // Execute early
        , camera_entity_id(cam_entity) {}
    
    void execute(Renderer2D& renderer) const override;
    f32 estimate_gpu_cost() const noexcept override { return 0.1f; }
};

//=============================================================================
// Render Statistics and Performance Monitoring
//=============================================================================

/**
 * @brief Comprehensive Rendering Statistics
 * 
 * Collects detailed performance metrics for educational analysis
 * and optimization guidance.
 * 
 * Educational Context:
 * These statistics help students understand:
 * - GPU performance characteristics and bottlenecks
 * - The impact of different rendering techniques
 * - Memory usage patterns in graphics programming
 * - Frame timing and optimization opportunities
 */
struct RenderStatistics {
    //-------------------------------------------------------------------------
    // Frame Statistics
    //-------------------------------------------------------------------------
    
    struct FrameStats {
        u32 frame_number{0};            ///< Current frame number
        f32 frame_time_ms{0.0f};        ///< Total frame time
        f32 cpu_time_ms{0.0f};          ///< CPU rendering time
        f32 gpu_time_ms{0.0f};          ///< GPU rendering time (if available)
        f32 present_time_ms{0.0f};      ///< Present/swap time
        
        // Command statistics
        u32 total_commands{0};          ///< Total render commands issued
        u32 draw_commands{0};           ///< Draw commands executed
        u32 state_changes{0};           ///< State change commands
        u32 debug_commands{0};          ///< Debug rendering commands
    } current_frame;
    
    //-------------------------------------------------------------------------
    // GPU Resource Statistics
    //-------------------------------------------------------------------------
    
    struct GPUStats {
        u32 draw_calls{0};              ///< Number of draw calls issued
        u32 vertices_rendered{0};       ///< Total vertices processed
        u32 triangles_rendered{0};      ///< Total triangles rendered
        u32 pixels_shaded{0};           ///< Estimated pixels shaded
        
        // Batching efficiency
        u32 batches_created{0};         ///< Number of sprite batches
        u32 batch_breaks{0};            ///< Times batching was broken
        f32 batching_efficiency{1.0f};  ///< Ratio of batched to individual draws
        
        // Memory usage
        usize vertex_buffer_memory{0};  ///< Vertex buffer memory used
        usize index_buffer_memory{0};   ///< Index buffer memory used
        usize texture_memory{0};        ///< Texture memory used
        usize total_gpu_memory{0};      ///< Total GPU memory used
    } gpu_stats;
    
    //-------------------------------------------------------------------------
    // Performance Analysis
    //-------------------------------------------------------------------------
    
    struct PerformanceStats {
        // Timing breakdown
        f32 culling_time_ms{0.0f};      ///< Time spent on frustum culling
        f32 sorting_time_ms{0.0f};      ///< Time spent sorting render commands
        f32 batching_time_ms{0.0f};     ///< Time spent creating sprite batches
        f32 binding_time_ms{0.0f};      ///< Time spent binding resources
        f32 rendering_time_ms{0.0f};    ///< Time spent in actual draw calls
        
        // Performance ratings
        const char* frame_rate_rating{"Good"};      // "Excellent", "Good", "Fair", "Poor"
        const char* gpu_utilization_rating{"Good"}; // GPU utilization assessment
        const char* memory_efficiency_rating{"Good"}; // Memory usage efficiency
        
        // Bottleneck analysis
        const char* primary_bottleneck{"None"};     // Main performance bottleneck
        const char* optimization_suggestion{"None"}; // Primary optimization advice
        f32 performance_score{100.0f};             // Overall performance score (0-100)
    } performance;
    
    //-------------------------------------------------------------------------
    // Educational Insights
    //-------------------------------------------------------------------------
    
    struct EducationalStats {
        // Rendering technique analysis
        u32 opaque_objects{0};          ///< Objects rendered with opaque materials
        u32 transparent_objects{0};     ///< Objects requiring alpha blending
        u32 ui_elements{0};             ///< UI elements rendered
        u32 debug_primitives{0};        ///< Debug shapes and lines
        
        // Shader usage
        u32 shader_switches{0};         ///< Number of shader program changes
        u32 unique_shaders_used{0};     ///< Number of different shaders used
        u32 default_shader_usage{0};    ///< Draw calls using default shader
        
        // Texture usage
        u32 texture_switches{0};        ///< Number of texture binding changes
        u32 unique_textures_used{0};    ///< Number of different textures used
        u32 texture_cache_hits{0};      ///< Texture cache hits
        u32 texture_cache_misses{0};    ///< Texture cache misses
        
        // Educational recommendations
        std::vector<std::string> optimization_hints; ///< Performance suggestions
        std::vector<std::string> learning_points;    ///< Educational insights
    } educational;
    
    //-------------------------------------------------------------------------
    // Historical Data (for trending analysis)
    //-------------------------------------------------------------------------
    
    static constexpr usize HISTORY_SIZE = 120; // 2 seconds at 60 FPS
    struct HistoryBuffer {
        std::array<f32, HISTORY_SIZE> frame_times{};
        std::array<u32, HISTORY_SIZE> draw_calls{};
        std::array<u32, HISTORY_SIZE> vertices_rendered{};
        std::array<usize, HISTORY_SIZE> memory_usage{};
        
        u32 current_index{0};
        bool buffer_full{false};
        
        void add_sample(f32 frame_time, u32 draws, u32 vertices, usize memory) noexcept {
            frame_times[current_index] = frame_time;
            draw_calls[current_index] = draws;
            vertices_rendered[current_index] = vertices;
            memory_usage[current_index] = memory;
            
            current_index = (current_index + 1) % HISTORY_SIZE;
            if (current_index == 0) buffer_full = true;
        }
        
        f32 get_average_frame_time() const noexcept {
            usize count = buffer_full ? HISTORY_SIZE : current_index;
            if (count == 0) return 0.0f;
            
            f32 sum = 0.0f;
            for (usize i = 0; i < count; ++i) {
                sum += frame_times[i];
            }
            return sum / count;
        }
        
        f32 get_fps() const noexcept {
            f32 avg_time = get_average_frame_time();
            return avg_time > 0.0f ? 1000.0f / avg_time : 0.0f;
        }
    } history;
    
    //-------------------------------------------------------------------------
    // Statistics Management
    //-------------------------------------------------------------------------
    
    /** @brief Reset frame statistics */
    void reset_frame_stats() noexcept {
        current_frame = {};
        gpu_stats = {};
    }
    
    /** @brief Update performance analysis */
    void update_performance_analysis() noexcept;
    
    /** @brief Add performance sample to history */
    void add_to_history() noexcept {
        history.add_sample(current_frame.frame_time_ms, gpu_stats.draw_calls, 
                          gpu_stats.vertices_rendered, gpu_stats.total_gpu_memory);
    }
    
    /** @brief Generate comprehensive performance report */
    std::string generate_performance_report() const noexcept;
    
    /** @brief Get performance grade (A-F) */
    char get_performance_grade() const noexcept {
        if (performance.performance_score >= 90) return 'A';
        if (performance.performance_score >= 80) return 'B';
        if (performance.performance_score >= 70) return 'C';
        if (performance.performance_score >= 60) return 'D';
        return 'F';
    }
};

//=============================================================================
// Renderer Configuration and State
//=============================================================================

/**
 * @brief 2D Renderer Configuration
 * 
 * Contains all configuration options for the 2D renderer including
 * performance settings, debug options, and educational features.
 */
struct Renderer2DConfig {
    //-------------------------------------------------------------------------
    // Core Rendering Settings
    //-------------------------------------------------------------------------
    
    struct RenderingSettings {
        bool enable_vsync{true};            ///< Enable vertical synchronization
        u32 max_sprites_per_batch{1000};    ///< Maximum sprites per batch
        u32 max_vertices_per_buffer{4000};  ///< Vertex buffer size (sprites * 4)
        u32 max_indices_per_buffer{6000};   ///< Index buffer size (sprites * 6)
        
        // Quality settings
        bool enable_multisampling{false};   ///< Enable MSAA
        u32 msaa_samples{4};                ///< MSAA sample count
        bool enable_anisotropic_filtering{true}; ///< Enable anisotropic filtering
        f32 max_anisotropy{16.0f};          ///< Maximum anisotropy level
        
        // Optimization settings
        bool enable_frustum_culling{true};  ///< Enable frustum culling
        bool enable_occlusion_culling{false}; ///< Enable occlusion culling
        bool enable_instancing{true};       ///< Enable instanced rendering
        f32 culling_margin{10.0f};          ///< Culling margin in world units
    } rendering;
    
    //-------------------------------------------------------------------------
    // Memory and Performance Settings
    //-------------------------------------------------------------------------
    
    struct PerformanceSettings {
        usize vertex_buffer_pool_size{1024 * 1024}; ///< Vertex buffer pool size
        usize index_buffer_pool_size{512 * 1024};   ///< Index buffer pool size
        usize command_buffer_size{10000};           ///< Render command buffer size
        
        // Threading settings
        bool enable_multithreaded_rendering{false}; ///< Enable multi-threaded rendering
        u32 render_thread_count{2};                ///< Number of render threads
        
        // Caching settings
        bool enable_state_caching{true};           ///< Cache OpenGL state changes
        bool enable_uniform_caching{true};         ///< Cache uniform updates
        u32 texture_bind_cache_size{16};           ///< Number of texture slots to cache
    } performance;
    
    //-------------------------------------------------------------------------
    // Debug and Educational Settings
    //-------------------------------------------------------------------------
    
    struct DebugSettings {
        bool enable_debug_rendering{false};    ///< Enable debug primitives
        bool enable_wireframe_mode{false};     ///< Render in wireframe mode
        bool show_bounding_boxes{false};       ///< Show sprite bounding boxes
        bool show_batch_colors{false};         ///< Color-code sprite batches
        bool show_overdraw{false};             ///< Visualize overdraw areas
        
        // Performance visualization
        bool show_performance_overlay{false};  ///< Show performance metrics overlay
        bool collect_gpu_timings{false};       ///< Collect detailed GPU timings
        bool log_render_commands{false};       ///< Log all render commands
        u32 max_debug_lines{10000};           ///< Maximum debug lines per frame
        
        // Educational features
        bool enable_step_through_mode{false};  ///< Allow stepping through render commands
        bool highlight_expensive_operations{false}; ///< Highlight costly operations
        bool show_memory_usage{false};         ///< Show memory usage information
    } debug;
    
    //-------------------------------------------------------------------------
    // Default Configurations
    //-------------------------------------------------------------------------
    
    static Renderer2DConfig performance_focused() noexcept {
        Renderer2DConfig config;
        config.rendering.max_sprites_per_batch = 2000;
        config.rendering.enable_frustum_culling = true;
        config.performance.enable_multithreaded_rendering = true;
        config.performance.enable_state_caching = true;
        config.debug.collect_gpu_timings = false;
        return config;
    }
    
    static Renderer2DConfig educational_mode() noexcept {
        Renderer2DConfig config;
        config.rendering.max_sprites_per_batch = 500; // Smaller batches for clearer analysis
        config.debug.enable_debug_rendering = true;
        config.debug.show_performance_overlay = true;
        config.debug.collect_gpu_timings = true;
        config.debug.show_memory_usage = true;
        return config;
    }
    
    static Renderer2DConfig debug_mode() noexcept {
        Renderer2DConfig config = educational_mode();
        config.debug.show_bounding_boxes = true;
        config.debug.show_batch_colors = true;
        config.debug.log_render_commands = true;
        config.debug.enable_step_through_mode = true;
        return config;
    }
};

//=============================================================================
// Main 2D Renderer Class
//=============================================================================

/**
 * @brief Main 2D Rendering System
 * 
 * The core 2D renderer that coordinates all rendering operations, manages
 * render commands, and provides comprehensive educational features.
 * 
 * Educational Context:
 * This renderer demonstrates:
 * - Command-based rendering architecture
 * - Efficient sprite batching techniques
 * - Multi-camera rendering systems
 * - Performance monitoring and optimization
 * - Debug visualization and analysis tools
 * 
 * Design Philosophy:
 * - Educational clarity without sacrificing performance
 * - Comprehensive debug and analysis tools
 * - Modular architecture for easy extension
 * - Integration with ECS systems and memory tracking
 */
class Renderer2D {
public:
    //-------------------------------------------------------------------------
    // Construction and Initialization
    //-------------------------------------------------------------------------
    
    /** @brief Create 2D renderer with configuration */
    explicit Renderer2D(const Renderer2DConfig& config = Renderer2DConfig{}) noexcept;
    
    /** @brief Destructor cleans up all resources */
    ~Renderer2D() noexcept;
    
    // Non-copyable, moveable
    Renderer2D(const Renderer2D&) = delete;
    Renderer2D& operator=(const Renderer2D&) = delete;
    Renderer2D(Renderer2D&&) noexcept;
    Renderer2D& operator=(Renderer2D&&) noexcept;
    
    /** @brief Initialize renderer with OpenGL context */
    core::Result<void, std::string> initialize() noexcept;
    
    /** @brief Shutdown renderer and release resources */
    void shutdown() noexcept;
    
    /** @brief Check if renderer is initialized */
    bool is_initialized() const noexcept { return initialized_; }
    
    //-------------------------------------------------------------------------
    // Resource Management
    //-------------------------------------------------------------------------
    
    /** @brief Get texture manager */
    TextureManager& get_texture_manager() noexcept { return *texture_manager_; }
    const TextureManager& get_texture_manager() const noexcept { return *texture_manager_; }
    
    /** @brief Get shader manager */
    ShaderManager& get_shader_manager() noexcept { return *shader_manager_; }
    const ShaderManager& get_shader_manager() const noexcept { return *shader_manager_; }
    
    /** @brief Set custom resource managers */
    void set_texture_manager(std::unique_ptr<TextureManager> manager) noexcept {
        texture_manager_ = std::move(manager);
    }
    
    void set_shader_manager(std::unique_ptr<ShaderManager> manager) noexcept {
        shader_manager_ = std::move(manager);
    }
    
    //-------------------------------------------------------------------------
    // Frame Management
    //-------------------------------------------------------------------------
    
    /** @brief Begin new frame */
    void begin_frame() noexcept;
    
    /** @brief End frame and execute all render commands */
    void end_frame() noexcept;
    
    /** @brief Check if frame is active */
    bool is_frame_active() const noexcept { return frame_active_; }
    
    /** @brief Get current frame number */
    u32 get_frame_number() const noexcept { return frame_number_; }
    
    //-------------------------------------------------------------------------
    // Render Command Interface
    //-------------------------------------------------------------------------
    
    /** @brief Submit render command for execution */
    void submit_command(std::unique_ptr<RenderCommand> command) noexcept;
    
    /** @brief Submit sprite for rendering */
    void draw_sprite(const Transform& transform, const RenderableSprite& sprite) noexcept;
    
    /** @brief Draw debug line */
    void draw_debug_line(f32 start_x, f32 start_y, f32 end_x, f32 end_y, 
                        Color color = Color::white(), f32 thickness = 1.0f) noexcept;
    
    /** @brief Draw debug bounding box */
    void draw_debug_box(f32 x, f32 y, f32 width, f32 height, 
                       Color color = Color::cyan(), f32 thickness = 1.0f) noexcept;
    
    /** @brief Draw debug circle */
    void draw_debug_circle(f32 center_x, f32 center_y, f32 radius, 
                          Color color = Color::yellow(), u32 segments = 32) noexcept;
    
    //-------------------------------------------------------------------------
    // Camera Management
    //-------------------------------------------------------------------------
    
    /** @brief Set active camera for rendering */
    void set_active_camera(const Camera2D& camera) noexcept;
    
    /** @brief Get current active camera */
    const Camera2D* get_active_camera() const noexcept { return active_camera_; }
    
    /** @brief Begin camera-specific rendering */
    void begin_camera(const Camera2D& camera) noexcept;
    
    /** @brief End camera-specific rendering */
    void end_camera() noexcept;
    
    /** @brief Support for multiple cameras per frame */
    void render_with_camera(const Camera2D& camera, std::function<void()> render_func) noexcept;
    
    //-------------------------------------------------------------------------
    // Material and Shader Management
    //-------------------------------------------------------------------------
    
    /** @brief Set active material for subsequent draws */
    void set_material(const Material& material) noexcept;
    
    /** @brief Get current active material */
    const Material* get_active_material() const noexcept { return active_material_; }
    
    /** @brief Bind shader program */
    void bind_shader(ShaderID shader_id) noexcept;
    
    /** @brief Bind texture to specified slot */
    void bind_texture(TextureID texture_id, u32 slot = 0) noexcept;
    
    //-------------------------------------------------------------------------
    // ECS System Integration
    //-------------------------------------------------------------------------
    
    /** @brief Render all entities with renderable components */
    void render_entities(ecs::Registry& registry) noexcept;
    
    /** @brief Render entities with specific camera */
    void render_entities_with_camera(ecs::Registry& registry, const Camera2D& camera) noexcept;
    
    /** @brief Update renderer systems (call once per frame) */
    void update(f32 delta_time) noexcept;
    
    //-------------------------------------------------------------------------
    // Performance and Statistics
    //-------------------------------------------------------------------------
    
    /** @brief Get rendering statistics */
    const RenderStatistics& get_statistics() const noexcept { return statistics_; }
    
    /** @brief Reset performance counters */
    void reset_statistics() noexcept;
    
    /** @brief Get configuration */
    const Renderer2DConfig& get_config() const noexcept { return config_; }
    
    /** @brief Update configuration */
    void update_config(const Renderer2DConfig& new_config) noexcept;
    
    //-------------------------------------------------------------------------
    // Debug and Educational Features
    //-------------------------------------------------------------------------
    
    /** @brief Enable/disable debug rendering */
    void set_debug_rendering_enabled(bool enabled) noexcept {
        config_.debug.enable_debug_rendering = enabled;
    }
    
    /** @brief Get debug rendering state */
    bool is_debug_rendering_enabled() const noexcept {
        return config_.debug.enable_debug_rendering;
    }
    
    /** @brief Generate comprehensive render report */
    std::string generate_render_report() const noexcept;
    
    /** @brief Get render command history for analysis */
    const std::vector<std::unique_ptr<RenderCommand>>& get_command_history() const noexcept {
        return command_history_;
    }
    
    /** @brief Step through render commands (debug mode) */
    void step_to_next_command() noexcept;
    
    /** @brief Enable/disable step-through mode */
    void set_step_through_mode(bool enabled) noexcept {
        config_.debug.enable_step_through_mode = enabled;
    }
    
    //-------------------------------------------------------------------------
    // System Integration
    //-------------------------------------------------------------------------
    
    /** @brief Handle window resize */
    void handle_window_resize(u32 new_width, u32 new_height) noexcept;
    
    /** @brief Handle GPU context lost/restored */
    void handle_context_lost() noexcept;
    void handle_context_restored() noexcept;
    
    /** @brief Get memory usage information */
    struct MemoryUsage {
        usize vertex_buffers{0};
        usize index_buffers{0};
        usize textures{0};
        usize shaders{0};
        usize render_commands{0};
        usize total{0};
    };
    
    MemoryUsage get_memory_usage() const noexcept;

private:
    //-------------------------------------------------------------------------
    // Internal State
    //-------------------------------------------------------------------------
    
    Renderer2DConfig config_;
    bool initialized_{false};
    bool frame_active_{false};
    u32 frame_number_{0};
    
    // Resource managers
    std::unique_ptr<TextureManager> texture_manager_;
    std::unique_ptr<ShaderManager> shader_manager_;
    std::unique_ptr<BatchRenderer> batch_renderer_;
    
    // Rendering state
    const Camera2D* active_camera_{nullptr};
    const Material* active_material_{nullptr};
    ShaderID active_shader_{INVALID_SHADER_ID};
    std::array<TextureID, 16> bound_textures_{}; // Support up to 16 texture units
    
    // Command system
    std::vector<std::unique_ptr<RenderCommand>> render_commands_;
    std::vector<std::unique_ptr<RenderCommand>> command_history_; // For debug analysis
    memory::ArenaAllocator command_allocator_;
    
    // Performance tracking
    RenderStatistics statistics_;
    
    // Debug and step-through mode
    usize current_command_index_{0};
    bool step_mode_active_{false};
    
    //-------------------------------------------------------------------------
    // Internal Methods
    //-------------------------------------------------------------------------
    
    void sort_render_commands() noexcept;
    void execute_render_commands() noexcept;
    void execute_command(const RenderCommand& command) noexcept;
    void update_statistics() noexcept;
    void setup_camera_matrices(const Camera2D& camera) noexcept;
    void setup_default_render_state() noexcept;
    
    // Resource creation and management
    void create_default_resources() noexcept;
    void cleanup_resources() noexcept;
    
    // Performance monitoring
    void begin_gpu_timing(const char* label) noexcept;
    void end_gpu_timing() noexcept;
    void record_draw_call(u32 vertices, u32 indices) noexcept;
    
    // Debug utilities
    void log_render_command(const RenderCommand& command) noexcept;
    void validate_render_state() const noexcept;
    Color get_batch_debug_color(u32 batch_id) const noexcept;
    
    friend class BatchRenderer;
    friend struct SpriteRenderCommand;
    friend struct DebugLineCommand;
    friend struct CameraSetCommand;
};

//=============================================================================
// 2D Rendering System (ECS System Implementation)
//=============================================================================

/**
 * @brief ECS Rendering System
 * 
 * Integrates the 2D renderer with the ECScope ECS architecture.
 * Automatically processes entities with rendering components and
 * submits appropriate render commands.
 */
class RenderingSystem {
public:
    /** @brief Create rendering system with renderer */
    explicit RenderingSystem(std::shared_ptr<Renderer2D> renderer) noexcept;
    
    /** @brief Process all renderable entities */
    void update(ecs::Registry& registry, f32 delta_time) noexcept;
    
    /** @brief Get renderer instance */
    Renderer2D& get_renderer() noexcept { return *renderer_; }
    const Renderer2D& get_renderer() const noexcept { return *renderer_; }
    
    /** @brief Enable/disable automatic camera handling */
    void set_auto_camera_handling(bool enabled) noexcept { auto_camera_handling_ = enabled; }
    
    /** @brief Set default camera for entities without camera */
    void set_default_camera(const Camera2D& camera) noexcept { default_camera_ = camera; }

private:
    std::shared_ptr<Renderer2D> renderer_;
    bool auto_camera_handling_{true};
    Camera2D default_camera_;
    
    void process_cameras(ecs::Registry& registry) noexcept;
    void process_sprites(ecs::Registry& registry) noexcept;
    void process_debug_rendering(ecs::Registry& registry) noexcept;
    void update_render_info_components(ecs::Registry& registry) noexcept;
};

//=============================================================================
// Utility Functions and Helpers
//=============================================================================

namespace utils {
    /** @brief Calculate sprite world bounds */
    struct Bounds2D {
        f32 min_x, min_y, max_x, max_y;
        f32 width() const noexcept { return max_x - min_x; }
        f32 height() const noexcept { return max_y - min_y; }
        bool intersects(const Bounds2D& other) const noexcept {
            return !(max_x < other.min_x || min_x > other.max_x ||
                     max_y < other.min_y || min_y > other.max_y);
        }
    };
    
    /** @brief Calculate sprite bounds in world space */
    Bounds2D calculate_sprite_bounds(const Transform& transform, 
                                    const RenderableSprite& sprite) noexcept;
    
    /** @brief Check if sprite is visible by camera */
    bool is_sprite_visible(const Transform& transform, const RenderableSprite& sprite,
                          const Camera2D& camera) noexcept;
    
    /** @brief Calculate sort key for depth-based rendering */
    f32 calculate_sort_key(f32 z_order, f32 y_position = 0.0f) noexcept;
    
    /** @brief Estimate GPU memory usage for sprite */
    usize estimate_sprite_memory_usage(const RenderableSprite& sprite) noexcept;
    
    /** @brief Convert world coordinates to screen coordinates */
    struct Point2D { f32 x, y; };
    Point2D world_to_screen(f32 world_x, f32 world_y, const Camera2D& camera) noexcept;
    
    /** @brief Convert screen coordinates to world coordinates */
    Point2D screen_to_world(f32 screen_x, f32 screen_y, const Camera2D& camera) noexcept;
    
    /** @brief Create orthographic projection matrix */
    void create_orthographic_matrix(f32* matrix, f32 left, f32 right, f32 bottom, f32 top, 
                                   f32 near_plane = -1.0f, f32 far_plane = 1.0f) noexcept;
    
    /** @brief Create 2D transformation matrix */
    void create_transform_matrix(f32* matrix, f32 x, f32 y, f32 rotation, 
                               f32 scale_x, f32 scale_y) noexcept;
    
    /** @brief Multiply 4x4 matrices */
    void multiply_matrices(const f32* a, const f32* b, f32* result) noexcept;
    
    /** @brief Check if two render commands can be batched together */
    bool can_batch_commands(const RenderCommand& a, const RenderCommand& b) noexcept;
}

} // namespace ecscope::renderer