/**
 * @file gui_renderer.hpp
 * @brief GUI Renderer Integration
 * 
 * High-performance GUI rendering system that integrates with the existing
 * Vulkan/OpenGL renderer backends for optimal performance.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#pragma once

#include "gui_core.hpp"
#include "../rendering/renderer.hpp"
#include <memory>
#include <vector>
#include <array>

namespace ecscope::gui {

// =============================================================================
// RENDER DATA STRUCTURES
// =============================================================================

/**
 * @brief Vertex structure for GUI rendering
 */
struct GuiVertex {
    Vec2 position;
    Vec2 uv;
    Color color;
    
    static rendering::VertexLayout get_vertex_layout();
};

/**
 * @brief Render batch for efficient drawing
 */
struct RenderBatch {
    std::vector<GuiVertex> vertices;
    std::vector<uint32_t> indices;
    uint32_t texture_id = 0;
    bool scissor_enabled = false;
    Rect scissor_rect;
    
    void clear() {
        vertices.clear();
        indices.clear();
        texture_id = 0;
        scissor_enabled = false;
        scissor_rect = {};
    }
    
    bool empty() const {
        return vertices.empty() || indices.empty();
    }
    
    size_t vertex_count() const { return vertices.size(); }
    size_t index_count() const { return indices.size(); }
    size_t triangle_count() const { return indices.size() / 3; }
};

/**
 * @brief Render command for the GPU
 */
struct RenderCommand {
    uint32_t vertex_offset = 0;
    uint32_t index_offset = 0;
    uint32_t index_count = 0;
    uint32_t texture_id = 0;
    bool scissor_enabled = false;
    Rect scissor_rect;
};

// =============================================================================
// GUI RENDERER CLASS
// =============================================================================

/**
 * @brief High-performance GUI renderer
 * 
 * Integrates with the existing rendering system to provide efficient
 * GUI rendering with minimal draw calls and state changes.
 */
class GuiRenderer {
public:
    GuiRenderer();
    ~GuiRenderer();
    
    // =============================================================================
    // INITIALIZATION
    // =============================================================================
    
    /**
     * @brief Initialize the GUI renderer
     */
    bool initialize(rendering::IRenderer* renderer, int display_width, int display_height);
    
    /**
     * @brief Shutdown and cleanup
     */
    void shutdown();
    
    /**
     * @brief Handle display size changes
     */
    void set_display_size(int width, int height);
    
    // =============================================================================
    // FRAME MANAGEMENT
    // =============================================================================
    
    /**
     * @brief Begin a new frame
     */
    void begin_frame();
    
    /**
     * @brief End frame and submit for rendering
     */
    void end_frame();
    
    /**
     * @brief Render all GUI elements
     */
    void render();
    
    // =============================================================================
    // DRAWING COMMANDS
    // =============================================================================
    
    /**
     * @brief Process a draw list and add to render queue
     */
    void add_draw_list(const DrawList& draw_list);
    
    /**
     * @brief Add primitive shapes
     */
    void add_rect_filled(const Rect& rect, const Color& color, float rounding = 0.0f);
    void add_rect(const Rect& rect, const Color& color, float thickness = 1.0f, float rounding = 0.0f);
    void add_circle_filled(const Vec2& center, float radius, const Color& color, int segments = 0);
    void add_circle(const Vec2& center, float radius, const Color& color, float thickness = 1.0f, int segments = 0);
    void add_line(const Vec2& p1, const Vec2& p2, const Color& color, float thickness = 1.0f);
    void add_triangle_filled(const Vec2& p1, const Vec2& p2, const Vec2& p3, const Color& color);
    void add_triangle(const Vec2& p1, const Vec2& p2, const Vec2& p3, const Color& color, float thickness = 1.0f);
    
    /**
     * @brief Add textured quad
     */
    void add_image(const Rect& rect, uint32_t texture_id, const Vec2& uv_min = {0, 0}, 
                   const Vec2& uv_max = {1, 1}, const Color& tint = Color::WHITE);
    
    /**
     * @brief Add text
     */
    void add_text(const Vec2& position, const Color& color, const std::string& text,
                  uint32_t font_texture_id, const std::vector<GuiVertex>& text_vertices);
    
    /**
     * @brief Add gradient
     */
    void add_gradient_rect(const Rect& rect, const Color& top_left, const Color& top_right,
                          const Color& bottom_left, const Color& bottom_right);
    
    // =============================================================================
    // CLIPPING
    // =============================================================================
    
    /**
     * @brief Push clipping rectangle
     */
    void push_clip_rect(const Rect& clip_rect, bool intersect_with_current = true);
    
    /**
     * @brief Pop clipping rectangle
     */
    void pop_clip_rect();
    
    /**
     * @brief Get current clipping rectangle
     */
    const Rect& get_current_clip_rect() const;
    
    // =============================================================================
    // TEXTURE MANAGEMENT
    // =============================================================================
    
    /**
     * @brief Create texture from pixel data
     */
    uint32_t create_texture(int width, int height, const void* pixels, 
                           rendering::TextureFormat format = rendering::TextureFormat::RGBA8);
    
    /**
     * @brief Update texture data
     */
    void update_texture(uint32_t texture_id, int x, int y, int width, int height, 
                       const void* pixels);
    
    /**
     * @brief Destroy texture
     */
    void destroy_texture(uint32_t texture_id);
    
    /**
     * @brief Get white pixel texture (for solid colors)
     */
    uint32_t get_white_texture() const { return white_texture_id_; }
    
    // =============================================================================
    // RENDER STATE
    // =============================================================================
    
    /**
     * @brief Set render state for custom drawing
     */
    void set_render_state(const rendering::RenderState& state);
    
    /**
     * @brief Get current projection matrix
     */
    const std::array<float, 16>& get_projection_matrix() const { return projection_matrix_; }
    
    /**
     * @brief Set custom projection matrix
     */
    void set_projection_matrix(const std::array<float, 16>& matrix);
    
    // =============================================================================
    // PERFORMANCE AND STATISTICS
    // =============================================================================
    
    /**
     * @brief Rendering statistics
     */
    struct RenderStats {
        uint32_t frame_count = 0;
        uint32_t draw_calls = 0;
        uint32_t vertices_rendered = 0;
        uint32_t triangles_rendered = 0;
        uint32_t batches_merged = 0;
        uint32_t texture_switches = 0;
        uint32_t clip_rect_changes = 0;
        float cpu_time_ms = 0.0f;
        float gpu_time_ms = 0.0f;
        size_t vertex_buffer_size = 0;
        size_t index_buffer_size = 0;
        uint32_t active_textures = 0;
    };
    
    const RenderStats& get_render_stats() const { return render_stats_; }
    void reset_render_stats();
    
    /**
     * @brief Enable/disable performance profiling
     */
    void set_profiling_enabled(bool enabled) { profiling_enabled_ = enabled; }
    bool is_profiling_enabled() const { return profiling_enabled_; }
    
    // =============================================================================
    // DEBUG UTILITIES
    // =============================================================================
    
    /**
     * @brief Enable wireframe rendering (debug)
     */
    void set_wireframe_mode(bool enabled) { wireframe_mode_ = enabled; }
    
    /**
     * @brief Render debug information
     */
    void render_debug_info();
    
    /**
     * @brief Draw render batch bounds (debug)
     */
    void set_debug_draw_batches(bool enabled) { debug_draw_batches_ = enabled; }
    
private:
    // Core rendering
    rendering::IRenderer* renderer_ = nullptr;
    bool initialized_ = false;
    
    // Display info
    int display_width_ = 800;
    int display_height_ = 600;
    std::array<float, 16> projection_matrix_;
    
    // GPU resources
    rendering::ShaderHandle gui_shader_;
    rendering::BufferHandle vertex_buffer_;
    rendering::BufferHandle index_buffer_;
    rendering::BufferHandle uniform_buffer_;
    
    // Render batches
    std::vector<RenderBatch> batches_;
    std::vector<RenderCommand> commands_;
    RenderBatch* current_batch_ = nullptr;
    
    // Clipping stack
    std::vector<Rect> clip_stack_;
    Rect current_clip_rect_;
    
    // Textures
    std::unordered_map<uint32_t, rendering::TextureHandle> texture_map_;
    uint32_t next_texture_id_ = 1;
    uint32_t white_texture_id_ = 0;
    
    // Buffer management
    size_t vertex_buffer_size_ = 0;
    size_t index_buffer_size_ = 0;
    static constexpr size_t INITIAL_VERTEX_BUFFER_SIZE = 65536 * sizeof(GuiVertex);
    static constexpr size_t INITIAL_INDEX_BUFFER_SIZE = 65536 * 6 * sizeof(uint32_t);
    
    // Render state
    rendering::RenderState current_render_state_;
    
    // Statistics
    RenderStats render_stats_;
    bool profiling_enabled_ = false;
    
    // Debug options
    bool wireframe_mode_ = false;
    bool debug_draw_batches_ = false;
    
    // Performance timers
    std::chrono::high_resolution_clock::time_point frame_start_time_;
    
    // =============================================================================
    // INTERNAL METHODS
    // =============================================================================
    
    // Initialization helpers
    bool create_gpu_resources();
    bool create_shaders();
    bool create_buffers();
    bool create_white_texture();
    void create_projection_matrix();
    
    // Cleanup helpers
    void destroy_gpu_resources();
    
    // Batch management
    void start_new_batch(uint32_t texture_id);
    void flush_current_batch();
    void optimize_batches();
    void merge_compatible_batches();
    
    // Vertex generation
    void add_quad_vertices(const Rect& rect, const Color& color, const Vec2& uv_min, const Vec2& uv_max);
    void add_circle_vertices(const Vec2& center, float radius, const Color& color, int segments, bool filled);
    void add_line_vertices(const Vec2& p1, const Vec2& p2, float thickness, const Color& color);
    
    // Buffer management
    void ensure_vertex_buffer_capacity(size_t required_size);
    void ensure_index_buffer_capacity(size_t required_size);
    void upload_vertex_data();
    void upload_index_data();
    
    // Rendering helpers
    void setup_render_state();
    void bind_texture(uint32_t texture_id);
    void set_scissor_rect(const Rect& rect);
    void disable_scissor();
    
    // Utility functions
    int calculate_circle_segments(float radius) const;
    void update_render_stats();
    
    // Shader uniform structure
    struct GuiUniforms {
        std::array<float, 16> projection_matrix;
        float time = 0.0f;
        float _padding[3] = {0, 0, 0};
    };
    
    // Vertex shader source
    static const char* get_vertex_shader_source();
    
    // Fragment shader source
    static const char* get_fragment_shader_source();
};

// =============================================================================
// RENDER BACKEND FACTORY
// =============================================================================

/**
 * @brief Factory for creating GUI renderers
 */
class GuiRendererFactory {
public:
    /**
     * @brief Create a GUI renderer for the given backend
     */
    static std::unique_ptr<GuiRenderer> create(rendering::IRenderer* renderer,
                                              int display_width, int display_height);
    
    /**
     * @brief Check if GUI rendering is supported with the given backend
     */
    static bool is_supported(rendering::RenderingAPI api);
};

// =============================================================================
// RENDER UTILITIES
// =============================================================================

/**
 * @brief Convert DrawCommand to render primitives
 */
class DrawCommandProcessor {
public:
    static void process_command(const DrawCommand& cmd, GuiRenderer& renderer);
    
private:
    static void process_rectangle(const DrawCommand& cmd, GuiRenderer& renderer);
    static void process_circle(const DrawCommand& cmd, GuiRenderer& renderer);
    static void process_text(const DrawCommand& cmd, GuiRenderer& renderer);
    static void process_line(const DrawCommand& cmd, GuiRenderer& renderer);
    static void process_triangle(const DrawCommand& cmd, GuiRenderer& renderer);
    static void process_texture(const DrawCommand& cmd, GuiRenderer& renderer);
    static void process_gradient(const DrawCommand& cmd, GuiRenderer& renderer);
};

/**
 * @brief Render optimization utilities
 */
namespace render_optimization {
    /**
     * @brief Optimize draw list for rendering
     */
    void optimize_draw_list(DrawList& draw_list);
    
    /**
     * @brief Merge compatible draw commands
     */
    void merge_draw_commands(std::vector<DrawCommand>& commands);
    
    /**
     * @brief Sort draw commands for optimal rendering
     */
    void sort_by_texture(std::vector<DrawCommand>& commands);
    void sort_by_depth(std::vector<DrawCommand>& commands);
    
    /**
     * @brief Cull draw commands outside viewport
     */
    void cull_outside_viewport(std::vector<DrawCommand>& commands, const Rect& viewport);
} // namespace render_optimization

// =============================================================================
// IMMEDIATE MODE HELPERS
// =============================================================================

/**
 * @brief Get the current GUI renderer
 */
GuiRenderer* get_gui_renderer();

/**
 * @brief Direct rendering functions (use current renderer)
 */
void render_rect_filled(const Rect& rect, const Color& color, float rounding = 0.0f);
void render_rect(const Rect& rect, const Color& color, float thickness = 1.0f, float rounding = 0.0f);
void render_circle_filled(const Vec2& center, float radius, const Color& color, int segments = 0);
void render_circle(const Vec2& center, float radius, const Color& color, float thickness = 1.0f, int segments = 0);
void render_line(const Vec2& p1, const Vec2& p2, const Color& color, float thickness = 1.0f);
void render_image(const Rect& rect, uint32_t texture_id, const Vec2& uv_min = {0, 0}, 
                 const Vec2& uv_max = {1, 1}, const Color& tint = Color::WHITE);

// =============================================================================
// PLATFORM-SPECIFIC INTEGRATION
// =============================================================================

#ifdef GUI_ENABLE_VULKAN_INTEGRATION
/**
 * @brief Vulkan-specific GUI renderer
 */
class VulkanGuiRenderer : public GuiRenderer {
public:
    bool initialize_vulkan_resources(VkDevice device, VkRenderPass render_pass);
    void set_command_buffer(VkCommandBuffer cmd_buffer);
    
private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkRenderPass render_pass_ = VK_NULL_HANDLE;
    VkCommandBuffer command_buffer_ = VK_NULL_HANDLE;
    
    // Vulkan-specific resources
    VkPipeline graphics_pipeline_ = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
    VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
    VkDescriptorSet descriptor_set_ = VK_NULL_HANDLE;
};
#endif

#ifdef GUI_ENABLE_OPENGL_INTEGRATION
/**
 * @brief OpenGL-specific GUI renderer
 */
class OpenGLGuiRenderer : public GuiRenderer {
public:
    bool initialize_opengl_resources();
    void setup_opengl_state();
    void restore_opengl_state();
    
private:
    // OpenGL-specific resources
    uint32_t vao_ = 0;
    uint32_t vbo_ = 0;
    uint32_t ebo_ = 0;
    uint32_t program_ = 0;
    
    // Saved OpenGL state
    struct OpenGLState {
        bool blend_enabled;
        bool cull_face_enabled;
        bool depth_test_enabled;
        bool scissor_test_enabled;
        int blend_src_rgb, blend_dst_rgb, blend_src_alpha, blend_dst_alpha;
        int blend_equation_rgb, blend_equation_alpha;
        int viewport[4];
        int scissor_box[4];
        float clear_color[4];
    } saved_state_;
    
    void save_opengl_state();
    void restore_opengl_state_internal();
};
#endif

} // namespace ecscope::gui