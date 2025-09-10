/**
 * @file opengl_backend.hpp
 * @brief Professional OpenGL Rendering Backend
 * 
 * High-performance OpenGL 4.5+ implementation with modern features,
 * direct state access, and robust resource management.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#pragma once

#include "renderer.hpp"
#include <GL/glew.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <array>
#include <string>
#include <chrono>

// Forward declaration
struct GLFWwindow;

namespace ecscope::rendering {

// =============================================================================
// OPENGL-SPECIFIC STRUCTURES
// =============================================================================

/**
 * @brief OpenGL buffer resource
 */
struct OpenGLBuffer {
    GLuint buffer_id = 0;
    GLenum target = GL_ARRAY_BUFFER;
    GLenum usage = GL_STATIC_DRAW;
    GLsizeiptr size = 0;
    bool is_mapped = false;
    void* mapped_pointer = nullptr;
    std::string debug_name;
};

/**
 * @brief OpenGL texture resource
 */
struct OpenGLTexture {
    GLuint texture_id = 0;
    GLuint sampler_id = 0;
    GLenum target = GL_TEXTURE_2D;
    GLenum internal_format = GL_RGBA8;
    GLenum format = GL_RGBA;
    GLenum type = GL_UNSIGNED_BYTE;
    GLsizei width = 0;
    GLsizei height = 0;
    GLsizei depth = 1;
    GLint levels = 1;
    GLint layers = 1;
    GLint samples = 1;
    bool is_render_target = false;
    bool is_depth_stencil = false;
    std::string debug_name;
};

/**
 * @brief OpenGL shader resource
 */
struct OpenGLShader {
    GLuint program_id = 0;
    GLuint vertex_shader_id = 0;
    GLuint fragment_shader_id = 0;
    GLuint compute_shader_id = 0;
    
    // Uniform locations cache
    std::unordered_map<std::string, GLint> uniform_locations;
    std::unordered_map<std::string, GLint> uniform_block_indices;
    std::unordered_map<std::string, GLint> storage_block_indices;
    
    bool is_compute_shader = false;
    std::string debug_name;
};

/**
 * @brief OpenGL vertex array object
 */
struct OpenGLVertexArray {
    GLuint vao_id = 0;
    VertexLayout layout;
    std::vector<BufferHandle> vertex_buffers;
    BufferHandle index_buffer;
    bool has_index_buffer = false;
    bool use_32bit_indices = true;
};

/**
 * @brief OpenGL framebuffer object
 */
struct OpenGLFramebuffer {
    GLuint fbo_id = 0;
    std::vector<TextureHandle> color_attachments;
    TextureHandle depth_attachment;
    bool has_depth_attachment = false;
    GLsizei width = 0;
    GLsizei height = 0;
    std::string debug_name;
};

// =============================================================================
// OPENGL RENDERER IMPLEMENTATION
// =============================================================================

/**
 * @brief High-performance OpenGL rendering backend
 * 
 * This implementation provides:
 * - OpenGL 4.5+ Core Profile with DSA (Direct State Access)
 * - Efficient state caching and change detection
 * - Automatic resource lifecycle management
 * - Buffer orphaning for dynamic updates
 * - Comprehensive error checking and debugging
 * - Multi-threaded command submission support
 */
class OpenGLRenderer : public IRenderer {
public:
    OpenGLRenderer();
    ~OpenGLRenderer() override;

    // Disable copy/move
    OpenGLRenderer(const OpenGLRenderer&) = delete;
    OpenGLRenderer& operator=(const OpenGLRenderer&) = delete;
    OpenGLRenderer(OpenGLRenderer&&) = delete;
    OpenGLRenderer& operator=(OpenGLRenderer&&) = delete;

    // =============================================================================
    // IRenderer INTERFACE IMPLEMENTATION
    // =============================================================================

    bool initialize(RenderingAPI api = RenderingAPI::OpenGL) override;
    void shutdown() override;
    RenderingAPI get_api() const override { return RenderingAPI::OpenGL; }
    RendererCaps get_capabilities() const override;

    // Resource management
    BufferHandle create_buffer(const BufferDesc& desc, const void* initial_data = nullptr) override;
    TextureHandle create_texture(const TextureDesc& desc, const void* initial_data = nullptr) override;
    ShaderHandle create_shader(const std::string& vertex_source, 
                              const std::string& fragment_source,
                              const std::string& debug_name = "") override;
    ShaderHandle create_compute_shader(const std::string& compute_source,
                                     const std::string& debug_name = "") override;

    void destroy_buffer(BufferHandle handle) override;
    void destroy_texture(TextureHandle handle) override;
    void destroy_shader(ShaderHandle handle) override;

    // Resource updates
    void update_buffer(BufferHandle handle, size_t offset, size_t size, const void* data) override;
    void update_texture(TextureHandle handle, uint32_t mip_level, uint32_t array_layer,
                       uint32_t x, uint32_t y, uint32_t z,
                       uint32_t width, uint32_t height, uint32_t depth,
                       const void* data) override;
    void generate_mipmaps(TextureHandle handle) override;

    // Frame management
    void begin_frame() override;
    void end_frame() override;
    void set_render_target(TextureHandle color_target = TextureHandle{},
                          TextureHandle depth_target = TextureHandle{}) override;
    void clear(const std::array<float, 4>& color = {0.0f, 0.0f, 0.0f, 1.0f},
              float depth = 1.0f, uint8_t stencil = 0) override;
    void set_viewport(const Viewport& viewport) override;
    void set_scissor(const ScissorRect& scissor) override;

    // Pipeline state management
    void set_shader(ShaderHandle handle) override;
    void set_render_state(const RenderState& state) override;
    void set_vertex_buffers(std::span<const BufferHandle> buffers,
                           std::span<const uint64_t> offsets = {}) override;
    void set_index_buffer(BufferHandle buffer, size_t offset = 0, bool use_32bit_indices = true) override;
    void set_vertex_layout(const VertexLayout& layout) override;

    // Resource binding
    void bind_texture(uint32_t slot, TextureHandle texture) override;
    void bind_textures(uint32_t first_slot, std::span<const TextureHandle> textures) override;
    void bind_uniform_buffer(uint32_t slot, BufferHandle buffer, 
                            size_t offset = 0, size_t size = 0) override;
    void bind_storage_buffer(uint32_t slot, BufferHandle buffer,
                            size_t offset = 0, size_t size = 0) override;
    void set_push_constants(uint32_t offset, uint32_t size, const void* data) override;

    // Draw commands
    void draw_indexed(const DrawIndexedCommand& cmd) override;
    void draw(const DrawCommand& cmd) override;
    void dispatch(const DispatchCommand& cmd) override;

    // Debugging & profiling
    void push_debug_marker(const std::string& name) override;
    void pop_debug_marker() override;
    void insert_debug_marker(const std::string& name) override;
    FrameStats get_frame_stats() const override;

    // Advanced features
    void wait_idle() override;
    uint64_t create_fence() override;
    void wait_for_fence(uint64_t fence_id, uint64_t timeout_ns = UINT64_MAX) override;
    bool is_fence_signaled(uint64_t fence_id) override;

    // =============================================================================
    // OPENGL-SPECIFIC METHODS
    // =============================================================================

    /**
     * @brief Set window context for rendering
     */
    void set_window(GLFWwindow* window) { window_ = window; }

    /**
     * @brief Get OpenGL context information
     */
    struct ContextInfo {
        std::string vendor;
        std::string renderer;
        std::string version;
        std::string glsl_version;
        std::vector<std::string> extensions;
    };
    
    ContextInfo get_context_info() const;

    /**
     * @brief Enable/disable debug output
     */
    void set_debug_output(bool enable);

private:
    // =============================================================================
    // INTERNAL STRUCTURES
    // =============================================================================

    /**
     * @brief Cached OpenGL state to minimize state changes
     */
    struct GLState {
        // Vertex array state
        GLuint bound_vao = 0;
        
        // Shader state
        GLuint bound_program = 0;
        
        // Texture state
        std::array<GLuint, 32> bound_textures = {};
        std::array<GLuint, 32> bound_samplers = {};
        
        // Buffer state
        std::array<GLuint, 16> bound_uniform_buffers = {};
        std::array<GLuint, 16> bound_storage_buffers = {};
        GLuint bound_array_buffer = 0;
        GLuint bound_element_buffer = 0;
        
        // Framebuffer state
        GLuint bound_framebuffer = 0;
        
        // Render state
        struct {
            bool depth_test = true;
            bool depth_write = true;
            GLenum depth_func = GL_LESS;
            
            bool blend = false;
            GLenum blend_src = GL_ONE;
            GLenum blend_dst = GL_ZERO;
            GLenum blend_equation = GL_FUNC_ADD;
            
            GLenum cull_face_mode = GL_BACK;
            bool cull_face = true;
            bool wireframe = false;
            
            bool scissor_test = false;
            GLint scissor_x = 0;
            GLint scissor_y = 0;
            GLsizei scissor_width = 0;
            GLsizei scissor_height = 0;
        } render_state;
        
        // Viewport state
        GLint viewport_x = 0;
        GLint viewport_y = 0;
        GLsizei viewport_width = 0;
        GLsizei viewport_height = 0;
        
        // Clear state
        std::array<GLfloat, 4> clear_color = {0.0f, 0.0f, 0.0f, 1.0f};
        GLfloat clear_depth = 1.0f;
        GLint clear_stencil = 0;
    };

    /**
     * @brief GPU timer for performance profiling
     */
    struct GPUTimer {
        GLuint query_id = 0;
        bool is_active = false;
        uint64_t last_time_ns = 0;
    };

    // =============================================================================
    // INITIALIZATION & SETUP
    // =============================================================================

    bool create_context();
    bool load_extensions();
    void setup_debug_callback();
    void query_capabilities();

    // =============================================================================
    // RESOURCE HELPERS
    // =============================================================================

    GLuint compile_shader(GLenum shader_type, const std::string& source, const std::string& name);
    GLuint link_program(GLuint vertex_shader, GLuint fragment_shader, const std::string& name);
    GLuint link_compute_program(GLuint compute_shader, const std::string& name);
    void cache_shader_uniforms(OpenGLShader& shader);

    GLenum texture_format_to_gl_internal(TextureFormat format);
    std::pair<GLenum, GLenum> texture_format_to_gl_format_type(TextureFormat format);
    GLenum buffer_usage_to_gl(BufferUsage usage);
    GLenum primitive_topology_to_gl(PrimitiveTopology topology);

    GLuint create_vertex_array(const VertexLayout& layout, 
                              std::span<const BufferHandle> vertex_buffers,
                              BufferHandle index_buffer = BufferHandle{});

    // =============================================================================
    // STATE MANAGEMENT
    // =============================================================================

    void bind_vertex_array(GLuint vao);
    void use_program(GLuint program);
    void bind_texture_unit(GLuint unit, GLuint texture, GLuint sampler = 0);
    void bind_uniform_buffer_range(GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
    void bind_storage_buffer_range(GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
    void bind_framebuffer(GLuint fbo);

    void set_depth_state(bool test_enable, bool write_enable, GLenum func);
    void set_blend_state(BlendMode mode);
    void set_cull_state(CullMode mode);
    void set_wireframe_state(bool wireframe);

    // =============================================================================
    // ERROR HANDLING & DEBUG
    // =============================================================================

    void check_gl_error(const std::string& operation) const;
    static void GLAPIENTRY debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                         GLsizei length, const GLchar* message, const void* user_param);

    // =============================================================================
    // MEMBER VARIABLES
    // =============================================================================

    // Context and window
    GLFWwindow* window_ = nullptr;
    bool context_initialized_ = false;
    
    // Resource storage
    std::unordered_map<uint64_t, OpenGLBuffer> buffers_;
    std::unordered_map<uint64_t, OpenGLTexture> textures_;
    std::unordered_map<uint64_t, OpenGLShader> shaders_;
    std::unordered_map<uint64_t, OpenGLVertexArray> vertex_arrays_;
    std::unordered_map<uint64_t, OpenGLFramebuffer> framebuffers_;
    std::unordered_map<uint64_t, GLsync> fences_;
    
    std::atomic<uint64_t> next_resource_id_{1};
    std::atomic<uint64_t> next_fence_id_{1};
    
    // Current state cache
    GLState current_state_;
    GLState cached_state_;
    
    // Current bindings
    ShaderHandle current_shader_;
    uint64_t current_vertex_array_id_ = 0;
    std::vector<BufferHandle> bound_vertex_buffers_;
    BufferHandle bound_index_buffer_;
    bool index_buffer_32bit_ = true;
    
    // Render targets
    uint64_t current_framebuffer_id_ = 0;
    TextureHandle current_color_target_;
    TextureHandle current_depth_target_;
    
    // Capabilities and limits
    RendererCaps capabilities_;
    GLint max_texture_units_ = 0;
    GLint max_uniform_buffer_bindings_ = 0;
    GLint max_storage_buffer_bindings_ = 0;
    
    // Performance monitoring
    mutable FrameStats frame_stats_;
    std::chrono::high_resolution_clock::time_point frame_start_time_;
    GPUTimer gpu_timer_;
    
    // Debug and validation
    bool debug_output_enabled_ = false;
    mutable std::vector<std::string> debug_marker_stack_;
    
    // Push constants emulation (uniform buffer)
    struct PushConstantBuffer {
        GLuint buffer_id = 0;
        size_t size = 0;
        size_t offset = 0;
    };
    PushConstantBuffer push_constant_buffer_;
    static constexpr size_t MAX_PUSH_CONSTANT_SIZE = 256; // bytes
};

// =============================================================================
// HELPER FUNCTIONS
// =============================================================================

/**
 * @brief Check if OpenGL 4.5+ is available
 */
bool is_opengl_available();

/**
 * @brief Get OpenGL version as major.minor
 */
std::pair<int, int> get_opengl_version();

/**
 * @brief Check if a specific OpenGL extension is supported
 */
bool is_extension_supported(const std::string& extension_name);

/**
 * @brief Utility macro for OpenGL error checking
 */
#ifdef DEBUG
    #define GL_CHECK(call) \
        do { \
            call; \
            GLenum error = glGetError(); \
            if (error != GL_NO_ERROR) { \
                throw std::runtime_error(std::string("OpenGL error: ") + std::to_string(error) + " in " + #call); \
            } \
        } while(0)
#else
    #define GL_CHECK(call) call
#endif

} // namespace ecscope::rendering