/**
 * @file webgl_context.hpp
 * @brief Complete WebGL 2.0 context management and rendering pipeline header
 */

#pragma once

#include <string>
#include <vector>
#include <memory>

#ifdef EMSCRIPTEN
#include <GLES3/gl3.h>
#else
#include <GL/gl.h>
#endif

namespace ecscope::web::graphics {

/**
 * WebGL context creation attributes
 */
struct WebGLContextAttributes {
    bool alpha = true;
    bool depth = true;
    bool stencil = false;
    bool antialias = true;
    bool premultiplied_alpha = true;
    bool preserve_drawing_buffer = false;
    bool desynchronized = false;
    bool fail_if_major_performance_caveat = false;
};

/**
 * WebGL rendering statistics
 */
struct WebGLStats {
    uint64_t draw_calls = 0;
    uint64_t triangles = 0;
    uint64_t vertices = 0;
    size_t textures = 0;
    size_t buffers = 0;
    size_t programs = 0;
    size_t vertex_arrays = 0;
    size_t framebuffers = 0;
};

/**
 * Complete WebGL 2.0 context wrapper with advanced features
 */
class WebGLContext {
public:
    WebGLContext();
    ~WebGLContext();

    // Context management
    bool initialize(const std::string& canvas_id, const WebGLContextAttributes& attributes = {});
    void shutdown();
    bool is_initialized() const;

    // Frame management
    void begin_frame();
    void end_frame();
    void present();

    // Shader management
    GLuint create_shader(const std::string& name, GLenum type, const std::string& source);
    GLuint create_program(const std::string& name, const std::string& vertex_shader, const std::string& fragment_shader);
    void use_program(const std::string& name);
    GLuint get_program(const std::string& name) const;

    // Texture management
    GLuint create_texture(const std::string& name, int width, int height, GLenum format = GL_RGBA, GLenum type = GL_UNSIGNED_BYTE, const void* data = nullptr);
    void bind_texture(const std::string& name, int unit = 0);

    // Buffer management
    GLuint create_buffer(const std::string& name, GLenum target, GLsizeiptr size, const void* data, GLenum usage = GL_STATIC_DRAW);
    void bind_buffer(const std::string& name, GLenum target);

    // Vertex array management
    GLuint create_vertex_array(const std::string& name);
    void bind_vertex_array(const std::string& name);

    // Framebuffer management
    GLuint create_framebuffer(const std::string& name);
    void bind_framebuffer(const std::string& name, GLenum target = GL_FRAMEBUFFER);

    // Rendering commands
    void draw_arrays(GLenum mode, GLint first, GLsizei count);
    void draw_elements(GLenum mode, GLsizei count, GLenum type, const void* indices);

    // State management
    void set_clear_color(float r, float g, float b, float a);
    void set_blend_mode(GLenum src, GLenum dst);
    void enable_depth_test(bool enable);
    void enable_blend(bool enable);
    void enable_cull_face(bool enable);

    // Information queries
    int get_canvas_width() const;
    int get_canvas_height() const;
    std::string get_webgl_version() const;
    std::vector<std::string> get_extensions() const;
    WebGLStats get_stats() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ecscope::web::graphics