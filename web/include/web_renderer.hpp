#pragma once

#include "ecscope/web/web_types.hpp"
#include <vector>
#include <memory>

#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#endif

namespace ecscope::web {

/**
 * @brief WebGL/WebGPU renderer for browser deployment
 * 
 * This class provides a high-performance rendering backend specifically
 * optimized for web browsers, supporting both WebGL 2.0 and WebGPU APIs.
 */
class WebRenderer {
public:
    /**
     * @brief Rendering backend type
     */
    enum class Backend {
        WebGL2,
        WebGPU,
        Auto  // Automatically select best available
    };
    
    /**
     * @brief Render target information
     */
    struct RenderTarget {
        std::string canvas_id;
        int width;
        int height;
        float device_pixel_ratio;
        bool is_offscreen;
    };
    
    /**
     * @brief Construct a new WebRenderer
     * @param target Render target configuration
     * @param backend Preferred rendering backend
     */
    WebRenderer(const RenderTarget& target, Backend backend = Backend::Auto);
    
    /**
     * @brief Destructor
     */
    ~WebRenderer();
    
    // Non-copyable, movable
    WebRenderer(const WebRenderer&) = delete;
    WebRenderer& operator=(const WebRenderer&) = delete;
    WebRenderer(WebRenderer&&) noexcept = default;
    WebRenderer& operator=(WebRenderer&&) noexcept = default;
    
    /**
     * @brief Initialize the renderer
     * @return true if initialization succeeded
     */
    bool initialize();
    
    /**
     * @brief Shutdown the renderer
     */
    void shutdown();
    
    /**
     * @brief Begin frame rendering
     */
    void begin_frame();
    
    /**
     * @brief End frame rendering and present
     */
    void end_frame();
    
    /**
     * @brief Resize render target
     * @param width New width
     * @param height New height
     */
    void resize(int width, int height);
    
    /**
     * @brief Set viewport
     * @param x X coordinate
     * @param y Y coordinate
     * @param width Viewport width
     * @param height Viewport height
     */
    void set_viewport(int x, int y, int width, int height);
    
    /**
     * @brief Clear render target
     * @param r Red component
     * @param g Green component
     * @param b Blue component
     * @param a Alpha component
     */
    void clear(float r, float g, float b, float a = 1.0f);
    
    /**
     * @brief Get current backend
     * @return Active rendering backend
     */
    Backend get_backend() const noexcept { return active_backend_; }
    
    /**
     * @brief Get render target info
     * @return Current render target
     */
    const RenderTarget& get_target() const noexcept { return target_; }
    
    /**
     * @brief Check if renderer is initialized
     * @return true if initialized
     */
    bool is_initialized() const noexcept { return initialized_; }
    
    /**
     * @brief Get WebGL context (if using WebGL backend)
     * @return WebGL context handle
     */
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE get_webgl_context() const;
    
    /**
     * @brief Create shader program
     * @param vertex_source Vertex shader source
     * @param fragment_source Fragment shader source
     * @return Shader program ID
     */
    std::uint32_t create_shader_program(const std::string& vertex_source, const std::string& fragment_source);
    
    /**
     * @brief Delete shader program
     * @param program Program ID
     */
    void delete_shader_program(std::uint32_t program);
    
    /**
     * @brief Create vertex buffer
     * @param data Buffer data
     * @param size Data size in bytes
     * @param usage Buffer usage pattern
     * @return Buffer ID
     */
    std::uint32_t create_vertex_buffer(const void* data, std::size_t size, GLenum usage = GL_STATIC_DRAW);
    
    /**
     * @brief Create index buffer
     * @param data Buffer data
     * @param size Data size in bytes
     * @param usage Buffer usage pattern
     * @return Buffer ID
     */
    std::uint32_t create_index_buffer(const void* data, std::size_t size, GLenum usage = GL_STATIC_DRAW);
    
    /**
     * @brief Update buffer data
     * @param buffer Buffer ID
     * @param offset Offset in bytes
     * @param data New data
     * @param size Data size in bytes
     */
    void update_buffer(std::uint32_t buffer, std::size_t offset, const void* data, std::size_t size);
    
    /**
     * @brief Delete buffer
     * @param buffer Buffer ID
     */
    void delete_buffer(std::uint32_t buffer);
    
    /**
     * @brief Create texture
     * @param width Texture width
     * @param height Texture height
     * @param format Internal format
     * @param type Data type
     * @param data Texture data (can be nullptr)
     * @return Texture ID
     */
    std::uint32_t create_texture(int width, int height, GLenum format, GLenum type, const void* data = nullptr);
    
    /**
     * @brief Update texture data
     * @param texture Texture ID
     * @param x X offset
     * @param y Y offset
     * @param width Update width
     * @param height Update height
     * @param format Data format
     * @param type Data type
     * @param data New data
     */
    void update_texture(std::uint32_t texture, int x, int y, int width, int height, 
                       GLenum format, GLenum type, const void* data);
    
    /**
     * @brief Delete texture
     * @param texture Texture ID
     */
    void delete_texture(std::uint32_t texture);
    
    /**
     * @brief Create framebuffer
     * @param width Framebuffer width
     * @param height Framebuffer height
     * @param color_attachments Number of color attachments
     * @param has_depth Whether to include depth buffer
     * @param has_stencil Whether to include stencil buffer
     * @return Framebuffer ID
     */
    std::uint32_t create_framebuffer(int width, int height, int color_attachments = 1, 
                                    bool has_depth = true, bool has_stencil = false);
    
    /**
     * @brief Bind framebuffer
     * @param framebuffer Framebuffer ID (0 for default)
     */
    void bind_framebuffer(std::uint32_t framebuffer);
    
    /**
     * @brief Delete framebuffer
     * @param framebuffer Framebuffer ID
     */
    void delete_framebuffer(std::uint32_t framebuffer);
    
    /**
     * @brief Draw indexed primitives
     * @param mode Primitive mode
     * @param count Index count
     * @param type Index type
     * @param offset Index offset
     */
    void draw_indexed(GLenum mode, std::uint32_t count, GLenum type, std::size_t offset = 0);
    
    /**
     * @brief Draw arrays
     * @param mode Primitive mode
     * @param first First vertex
     * @param count Vertex count
     */
    void draw_arrays(GLenum mode, std::uint32_t first, std::uint32_t count);
    
    /**
     * @brief Enable/disable feature
     * @param feature OpenGL feature
     * @param enable Whether to enable
     */
    void set_feature(GLenum feature, bool enable);
    
    /**
     * @brief Set blend mode
     * @param src_factor Source blend factor
     * @param dst_factor Destination blend factor
     */
    void set_blend_mode(GLenum src_factor, GLenum dst_factor);
    
    /**
     * @brief Set depth testing
     * @param enable Whether to enable depth testing
     * @param func Depth comparison function
     */
    void set_depth_test(bool enable, GLenum func = GL_LESS);
    
    /**
     * @brief Set culling
     * @param enable Whether to enable culling
     * @param face Face to cull
     */
    void set_culling(bool enable, GLenum face = GL_BACK);
    
    /**
     * @brief Get rendering statistics
     * @return Current frame statistics
     */
    struct RenderStats {
        std::uint32_t draw_calls;
        std::uint32_t triangles;
        std::uint32_t vertices;
        std::uint32_t texture_switches;
        std::uint32_t shader_switches;
        double frame_time_ms;
    };
    
    RenderStats get_render_stats() const;
    
    /**
     * @brief Reset rendering statistics
     */
    void reset_render_stats();
    
    /**
     * @brief Handle context lost event
     */
    void handle_context_lost();
    
    /**
     * @brief Handle context restored event
     */
    void handle_context_restored();
    
private:
    // Configuration
    RenderTarget target_;
    Backend preferred_backend_;
    Backend active_backend_;
    
    // State
    bool initialized_ = false;
    bool context_lost_ = false;
    
    // WebGL context
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE webgl_context_ = 0;
    
    // Current state
    std::uint32_t current_program_ = 0;
    std::uint32_t current_framebuffer_ = 0;
    std::uint32_t current_texture_unit_ = 0;
    
    // Resource tracking
    std::vector<std::uint32_t> shader_programs_;
    std::vector<std::uint32_t> buffers_;
    std::vector<std::uint32_t> textures_;
    std::vector<std::uint32_t> framebuffers_;
    
    // Statistics
    mutable RenderStats render_stats_;
    std::chrono::steady_clock::time_point frame_start_time_;
    
    // Internal methods
    bool initialize_webgl();
    bool initialize_webgpu();
    void cleanup_resources();
    std::uint32_t compile_shader(const std::string& source, GLenum type);
    void check_gl_error(const std::string& operation) const;
    
    // Static callbacks
    static EM_BOOL context_lost_callback(int event_type, const void* reserved, void* user_data);
    static EM_BOOL context_restored_callback(int event_type, const void* reserved, void* user_data);
};

} // namespace ecscope::web