/**
 * @file webgl_context.cpp
 * @brief Complete WebGL 2.0 context management and rendering pipeline
 * 
 * This file implements a comprehensive WebGL 2.0 rendering system optimized for
 * educational ECS applications. It provides complete shader management, buffer
 * management, texture handling, and advanced rendering features.
 */

#include "webgl_context.hpp"
#include "web_error_handler.hpp"
#include "web_performance_monitor.hpp"

#include <emscripten/html5.h>
#include <emscripten/emscripten.h>
#include <GLES3/gl3.h>
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <array>

namespace ecscope::web::graphics {

/**
 * Complete WebGL 2.0 context implementation with advanced features
 */
class WebGLContext::Impl {
private:
    // Context state
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE webgl_context_ = 0;
    std::string canvas_id_;
    int canvas_width_ = 0;
    int canvas_height_ = 0;
    bool initialized_ = false;
    
    // Rendering state
    struct RenderState {
        GLuint current_program = 0;
        GLuint current_vao = 0;
        GLuint current_framebuffer = 0;
        GLenum current_blend_func_src = GL_SRC_ALPHA;
        GLenum current_blend_func_dst = GL_ONE_MINUS_SRC_ALPHA;
        bool depth_test_enabled = false;
        bool blend_enabled = false;
        bool cull_face_enabled = false;
        std::array<float, 4> clear_color = {0.0f, 0.0f, 0.0f, 1.0f};
    } render_state_;
    
    // Resource management
    std::unordered_map<std::string, GLuint> shaders_;
    std::unordered_map<std::string, GLuint> programs_;
    std::unordered_map<std::string, GLuint> textures_;
    std::unordered_map<std::string, GLuint> buffers_;
    std::unordered_map<std::string, GLuint> vertex_arrays_;
    std::unordered_map<std::string, GLuint> framebuffers_;
    
    // Performance monitoring
    WebPerformanceMonitor performance_monitor_;
    uint64_t draw_call_count_ = 0;
    uint64_t triangle_count_ = 0;
    uint64_t vertex_count_ = 0;
    
    // Error handling
    WebErrorHandler error_handler_;

public:
    bool initialize(const std::string& canvas_id, const WebGLContextAttributes& attributes) {
        try {
            canvas_id_ = canvas_id;
            
            // Get canvas dimensions
            EmscriptenWebGLContextAttributes webgl_attrs;
            emscripten_webgl_init_context_attributes(&webgl_attrs);
            
            // Configure WebGL 2.0 attributes
            webgl_attrs.majorVersion = 2;
            webgl_attrs.minorVersion = 0;
            webgl_attrs.alpha = attributes.alpha;
            webgl_attrs.depth = attributes.depth;
            webgl_attrs.stencil = attributes.stencil;
            webgl_attrs.antialias = attributes.antialias;
            webgl_attrs.premultipliedAlpha = attributes.premultiplied_alpha;
            webgl_attrs.preserveDrawingBuffer = attributes.preserve_drawing_buffer;
            webgl_attrs.powerPreference = EM_WEBGL_POWER_PREFERENCE_HIGH_PERFORMANCE;
            webgl_attrs.failIfMajorPerformanceCaveat = false;
            webgl_attrs.enableExtensionsByDefault = true;
            
            // Create WebGL context
            webgl_context_ = emscripten_webgl_create_context(canvas_id.c_str(), &webgl_attrs);
            if (webgl_context_ <= 0) {
                error_handler_.report_error("Failed to create WebGL 2.0 context", ErrorSeverity::CRITICAL);
                return false;
            }
            
            // Make context current
            EMSCRIPTEN_RESULT result = emscripten_webgl_make_context_current(webgl_context_);
            if (result != EMSCRIPTEN_RESULT_SUCCESS) {
                error_handler_.report_error("Failed to make WebGL context current", ErrorSeverity::CRITICAL);
                return false;
            }
            
            // Get canvas size
            double width, height;
            emscripten_get_canvas_element_size(canvas_id.c_str(), &width, &height);
            canvas_width_ = static_cast<int>(width);
            canvas_height_ = static_cast<int>(height);
            
            // Initialize OpenGL state
            initialize_opengl_state();
            
            // Load and enable extensions
            load_extensions();
            
            // Initialize built-in shaders
            initialize_built_in_shaders();
            
            initialized_ = true;
            
            performance_monitor_.log_event("WebGL Context Initialized", {
                {"canvas_id", canvas_id},
                {"width", std::to_string(canvas_width_)},
                {"height", std::to_string(canvas_height_)},
                {"version", get_webgl_version()}
            });
            
            return true;
            
        } catch (const std::exception& e) {
            error_handler_.report_error("WebGL initialization failed: " + std::string(e.what()), ErrorSeverity::CRITICAL);
            return false;
        }
    }
    
    void shutdown() {
        if (initialized_) {
            // Clean up resources
            cleanup_resources();
            
            // Destroy context
            if (webgl_context_ > 0) {
                emscripten_webgl_destroy_context(webgl_context_);
                webgl_context_ = 0;
            }
            
            initialized_ = false;
            performance_monitor_.log_event("WebGL Context Shutdown");
        }
    }
    
    bool is_initialized() const { return initialized_; }
    
    void begin_frame() {
        if (!initialized_) return;
        
        performance_monitor_.begin_frame();
        draw_call_count_ = 0;
        triangle_count_ = 0;
        vertex_count_ = 0;
        
        // Update canvas size if changed
        update_canvas_size();
        
        // Set viewport
        glViewport(0, 0, canvas_width_, canvas_height_);
        
        // Clear buffers
        glClearColor(render_state_.clear_color[0], render_state_.clear_color[1], 
                    render_state_.clear_color[2], render_state_.clear_color[3]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }
    
    void end_frame() {
        if (!initialized_) return;
        
        // Flush GPU commands
        glFlush();
        
        performance_monitor_.end_frame();
        performance_monitor_.update_gpu_metrics(draw_call_count_, triangle_count_, vertex_count_);
        
        check_gl_errors("End Frame");
    }
    
    void present() {
        // WebGL automatically presents, no explicit swap needed
    }
    
    // Shader management
    GLuint create_shader(const std::string& name, GLenum type, const std::string& source) {
        GLuint shader = glCreateShader(type);
        if (shader == 0) {
            error_handler_.report_error("Failed to create shader: " + name, ErrorSeverity::ERROR);
            return 0;
        }
        
        const char* source_cstr = source.c_str();
        glShaderSource(shader, 1, &source_cstr, nullptr);
        glCompileShader(shader);
        
        GLint compiled;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint log_length;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
            std::vector<char> log(log_length);
            glGetShaderInfoLog(shader, log_length, nullptr, log.data());
            
            error_handler_.report_error("Shader compilation failed: " + name + "\n" + std::string(log.data()), 
                                      ErrorSeverity::ERROR);
            glDeleteShader(shader);
            return 0;
        }
        
        shaders_[name] = shader;
        return shader;
    }
    
    GLuint create_program(const std::string& name, const std::string& vertex_shader, const std::string& fragment_shader) {
        GLuint vs = create_shader(name + "_vs", GL_VERTEX_SHADER, vertex_shader);
        GLuint fs = create_shader(name + "_fs", GL_FRAGMENT_SHADER, fragment_shader);
        
        if (vs == 0 || fs == 0) {
            if (vs) glDeleteShader(vs);
            if (fs) glDeleteShader(fs);
            return 0;
        }
        
        GLuint program = glCreateProgram();
        if (program == 0) {
            error_handler_.report_error("Failed to create program: " + name, ErrorSeverity::ERROR);
            glDeleteShader(vs);
            glDeleteShader(fs);
            return 0;
        }
        
        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);
        
        GLint linked;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked) {
            GLint log_length;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
            std::vector<char> log(log_length);
            glGetProgramInfoLog(program, log_length, nullptr, log.data());
            
            error_handler_.report_error("Program linking failed: " + name + "\n" + std::string(log.data()), 
                                      ErrorSeverity::ERROR);
            glDeleteProgram(program);
            glDeleteShader(vs);
            glDeleteShader(fs);
            return 0;
        }
        
        // Clean up shaders (they're now part of the program)
        glDeleteShader(vs);
        glDeleteShader(fs);
        
        programs_[name] = program;
        return program;
    }
    
    void use_program(const std::string& name) {
        auto it = programs_.find(name);
        if (it != programs_.end() && it->second != render_state_.current_program) {
            glUseProgram(it->second);
            render_state_.current_program = it->second;
        }
    }
    
    GLuint get_program(const std::string& name) const {
        auto it = programs_.find(name);
        return (it != programs_.end()) ? it->second : 0;
    }
    
    // Texture management
    GLuint create_texture(const std::string& name, int width, int height, GLenum format = GL_RGBA, GLenum type = GL_UNSIGNED_BYTE, const void* data = nullptr) {
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, type, data);
        
        // Set default parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        textures_[name] = texture;
        return texture;
    }
    
    void bind_texture(const std::string& name, int unit = 0) {
        auto it = textures_.find(name);
        if (it != textures_.end()) {
            glActiveTexture(GL_TEXTURE0 + unit);
            glBindTexture(GL_TEXTURE_2D, it->second);
        }
    }
    
    // Buffer management
    GLuint create_buffer(const std::string& name, GLenum target, GLsizeiptr size, const void* data, GLenum usage = GL_STATIC_DRAW) {
        GLuint buffer;
        glGenBuffers(1, &buffer);
        glBindBuffer(target, buffer);
        glBufferData(target, size, data, usage);
        
        buffers_[name] = buffer;
        return buffer;
    }
    
    void bind_buffer(const std::string& name, GLenum target) {
        auto it = buffers_.find(name);
        if (it != buffers_.end()) {
            glBindBuffer(target, it->second);
        }
    }
    
    // Vertex array management
    GLuint create_vertex_array(const std::string& name) {
        GLuint vao;
        glGenVertexArrays(1, &vao);
        vertex_arrays_[name] = vao;
        return vao;
    }
    
    void bind_vertex_array(const std::string& name) {
        auto it = vertex_arrays_.find(name);
        if (it != vertex_arrays_.end() && it->second != render_state_.current_vao) {
            glBindVertexArray(it->second);
            render_state_.current_vao = it->second;
        }
    }
    
    // Framebuffer management
    GLuint create_framebuffer(const std::string& name) {
        GLuint fbo;
        glGenFramebuffers(1, &fbo);
        framebuffers_[name] = fbo;
        return fbo;
    }
    
    void bind_framebuffer(const std::string& name, GLenum target = GL_FRAMEBUFFER) {
        auto it = framebuffers_.find(name);
        GLuint fbo = (it != framebuffers_.end()) ? it->second : 0;
        if (fbo != render_state_.current_framebuffer) {
            glBindFramebuffer(target, fbo);
            render_state_.current_framebuffer = fbo;
        }
    }
    
    // Rendering commands
    void draw_arrays(GLenum mode, GLint first, GLsizei count) {
        glDrawArrays(mode, first, count);
        draw_call_count_++;
        vertex_count_ += count;
        if (mode == GL_TRIANGLES) triangle_count_ += count / 3;
    }
    
    void draw_elements(GLenum mode, GLsizei count, GLenum type, const void* indices) {
        glDrawElements(mode, count, type, indices);
        draw_call_count_++;
        vertex_count_ += count;
        if (mode == GL_TRIANGLES) triangle_count_ += count / 3;
    }
    
    // State management
    void set_clear_color(float r, float g, float b, float a) {
        render_state_.clear_color = {r, g, b, a};
    }
    
    void set_blend_mode(GLenum src, GLenum dst) {
        if (src != render_state_.current_blend_func_src || dst != render_state_.current_blend_func_dst) {
            glBlendFunc(src, dst);
            render_state_.current_blend_func_src = src;
            render_state_.current_blend_func_dst = dst;
        }
    }
    
    void enable_depth_test(bool enable) {
        if (enable != render_state_.depth_test_enabled) {
            if (enable) glEnable(GL_DEPTH_TEST);
            else glDisable(GL_DEPTH_TEST);
            render_state_.depth_test_enabled = enable;
        }
    }
    
    void enable_blend(bool enable) {
        if (enable != render_state_.blend_enabled) {
            if (enable) glEnable(GL_BLEND);
            else glDisable(GL_BLEND);
            render_state_.blend_enabled = enable;
        }
    }
    
    void enable_cull_face(bool enable) {
        if (enable != render_state_.cull_face_enabled) {
            if (enable) glEnable(GL_CULL_FACE);
            else glDisable(GL_CULL_FACE);
            render_state_.cull_face_enabled = enable;
        }
    }
    
    // Utility functions
    int get_canvas_width() const { return canvas_width_; }
    int get_canvas_height() const { return canvas_height_; }
    
    std::string get_webgl_version() const {
        const GLubyte* version = glGetString(GL_VERSION);
        return version ? std::string(reinterpret_cast<const char*>(version)) : "Unknown";
    }
    
    std::vector<std::string> get_extensions() const {
        std::vector<std::string> extensions;
        GLint num_extensions;
        glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);
        for (GLint i = 0; i < num_extensions; i++) {
            const GLubyte* ext = glGetStringi(GL_EXTENSIONS, i);
            if (ext) {
                extensions.emplace_back(reinterpret_cast<const char*>(ext));
            }
        }
        return extensions;
    }
    
    WebGLStats get_stats() const {
        WebGLStats stats;
        stats.draw_calls = draw_call_count_;
        stats.triangles = triangle_count_;
        stats.vertices = vertex_count_;
        stats.textures = textures_.size();
        stats.buffers = buffers_.size();
        stats.programs = programs_.size();
        stats.vertex_arrays = vertex_arrays_.size();
        stats.framebuffers = framebuffers_.size();
        return stats;
    }

private:
    void initialize_opengl_state() {
        // Set initial OpenGL state
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
        
        // Update render state
        render_state_.blend_enabled = true;
        render_state_.depth_test_enabled = true;
        render_state_.cull_face_enabled = true;
        render_state_.current_blend_func_src = GL_SRC_ALPHA;
        render_state_.current_blend_func_dst = GL_ONE_MINUS_SRC_ALPHA;
    }
    
    void load_extensions() {
        // WebGL 2.0 has most extensions built-in, but we can check for specific ones
        auto extensions = get_extensions();
        performance_monitor_.log_event("Extensions Loaded", {
            {"count", std::to_string(extensions.size())}
        });
    }
    
    void initialize_built_in_shaders() {
        // Basic 2D sprite shader
        const std::string sprite_vs = R"(
            #version 300 es
            precision highp float;
            
            layout(location = 0) in vec2 a_position;
            layout(location = 1) in vec2 a_texcoord;
            layout(location = 2) in vec4 a_color;
            
            uniform mat4 u_projection;
            uniform mat4 u_view;
            uniform mat4 u_model;
            
            out vec2 v_texcoord;
            out vec4 v_color;
            
            void main() {
                gl_Position = u_projection * u_view * u_model * vec4(a_position, 0.0, 1.0);
                v_texcoord = a_texcoord;
                v_color = a_color;
            }
        )";
        
        const std::string sprite_fs = R"(
            #version 300 es
            precision highp float;
            
            in vec2 v_texcoord;
            in vec4 v_color;
            
            uniform sampler2D u_texture;
            
            out vec4 fragColor;
            
            void main() {
                vec4 texColor = texture(u_texture, v_texcoord);
                fragColor = texColor * v_color;
            }
        )";
        
        create_program("sprite", sprite_vs, sprite_fs);
        
        // Basic primitive shader
        const std::string primitive_vs = R"(
            #version 300 es
            precision highp float;
            
            layout(location = 0) in vec2 a_position;
            layout(location = 1) in vec4 a_color;
            
            uniform mat4 u_projection;
            uniform mat4 u_view;
            
            out vec4 v_color;
            
            void main() {
                gl_Position = u_projection * u_view * vec4(a_position, 0.0, 1.0);
                v_color = a_color;
            }
        )";
        
        const std::string primitive_fs = R"(
            #version 300 es
            precision highp float;
            
            in vec4 v_color;
            
            out vec4 fragColor;
            
            void main() {
                fragColor = v_color;
            }
        )";
        
        create_program("primitive", primitive_vs, primitive_fs);
    }
    
    void update_canvas_size() {
        double width, height;
        emscripten_get_canvas_element_size(canvas_id_.c_str(), &width, &height);
        int new_width = static_cast<int>(width);
        int new_height = static_cast<int>(height);
        
        if (new_width != canvas_width_ || new_height != canvas_height_) {
            canvas_width_ = new_width;
            canvas_height_ = new_height;
            
            performance_monitor_.log_event("Canvas Resized", {
                {"width", std::to_string(canvas_width_)},
                {"height", std::to_string(canvas_height_)}
            });
        }
    }
    
    void cleanup_resources() {
        // Delete textures
        for (auto& pair : textures_) {
            glDeleteTextures(1, &pair.second);
        }
        textures_.clear();
        
        // Delete buffers
        for (auto& pair : buffers_) {
            glDeleteBuffers(1, &pair.second);
        }
        buffers_.clear();
        
        // Delete vertex arrays
        for (auto& pair : vertex_arrays_) {
            glDeleteVertexArrays(1, &pair.second);
        }
        vertex_arrays_.clear();
        
        // Delete framebuffers
        for (auto& pair : framebuffers_) {
            glDeleteFramebuffers(1, &pair.second);
        }
        framebuffers_.clear();
        
        // Delete programs
        for (auto& pair : programs_) {
            glDeleteProgram(pair.second);
        }
        programs_.clear();
        
        // Delete shaders
        for (auto& pair : shaders_) {
            glDeleteShader(pair.second);
        }
        shaders_.clear();
    }
    
    void check_gl_errors(const std::string& operation) {
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            std::string error_msg = "OpenGL error in " + operation + ": ";
            switch (error) {
                case GL_INVALID_ENUM: error_msg += "GL_INVALID_ENUM"; break;
                case GL_INVALID_VALUE: error_msg += "GL_INVALID_VALUE"; break;
                case GL_INVALID_OPERATION: error_msg += "GL_INVALID_OPERATION"; break;
                case GL_OUT_OF_MEMORY: error_msg += "GL_OUT_OF_MEMORY"; break;
                default: error_msg += "Unknown error " + std::to_string(error); break;
            }
            error_handler_.report_error(error_msg, ErrorSeverity::ERROR);
        }
    }
};

// WebGLContext implementation
WebGLContext::WebGLContext() : impl_(std::make_unique<Impl>()) {}

WebGLContext::~WebGLContext() = default;

bool WebGLContext::initialize(const std::string& canvas_id, const WebGLContextAttributes& attributes) {
    return impl_->initialize(canvas_id, attributes);
}

void WebGLContext::shutdown() {
    impl_->shutdown();
}

bool WebGLContext::is_initialized() const {
    return impl_->is_initialized();
}

void WebGLContext::begin_frame() {
    impl_->begin_frame();
}

void WebGLContext::end_frame() {
    impl_->end_frame();
}

void WebGLContext::present() {
    impl_->present();
}

GLuint WebGLContext::create_shader(const std::string& name, GLenum type, const std::string& source) {
    return impl_->create_shader(name, type, source);
}

GLuint WebGLContext::create_program(const std::string& name, const std::string& vertex_shader, const std::string& fragment_shader) {
    return impl_->create_program(name, vertex_shader, fragment_shader);
}

void WebGLContext::use_program(const std::string& name) {
    impl_->use_program(name);
}

GLuint WebGLContext::get_program(const std::string& name) const {
    return impl_->get_program(name);
}

GLuint WebGLContext::create_texture(const std::string& name, int width, int height, GLenum format, GLenum type, const void* data) {
    return impl_->create_texture(name, width, height, format, type, data);
}

void WebGLContext::bind_texture(const std::string& name, int unit) {
    impl_->bind_texture(name, unit);
}

GLuint WebGLContext::create_buffer(const std::string& name, GLenum target, GLsizeiptr size, const void* data, GLenum usage) {
    return impl_->create_buffer(name, target, size, data, usage);
}

void WebGLContext::bind_buffer(const std::string& name, GLenum target) {
    impl_->bind_buffer(name, target);
}

GLuint WebGLContext::create_vertex_array(const std::string& name) {
    return impl_->create_vertex_array(name);
}

void WebGLContext::bind_vertex_array(const std::string& name) {
    impl_->bind_vertex_array(name);
}

GLuint WebGLContext::create_framebuffer(const std::string& name) {
    return impl_->create_framebuffer(name);
}

void WebGLContext::bind_framebuffer(const std::string& name, GLenum target) {
    impl_->bind_framebuffer(name, target);
}

void WebGLContext::draw_arrays(GLenum mode, GLint first, GLsizei count) {
    impl_->draw_arrays(mode, first, count);
}

void WebGLContext::draw_elements(GLenum mode, GLsizei count, GLenum type, const void* indices) {
    impl_->draw_elements(mode, count, type, indices);
}

void WebGLContext::set_clear_color(float r, float g, float b, float a) {
    impl_->set_clear_color(r, g, b, a);
}

void WebGLContext::set_blend_mode(GLenum src, GLenum dst) {
    impl_->set_blend_mode(src, dst);
}

void WebGLContext::enable_depth_test(bool enable) {
    impl_->enable_depth_test(enable);
}

void WebGLContext::enable_blend(bool enable) {
    impl_->enable_blend(enable);
}

void WebGLContext::enable_cull_face(bool enable) {
    impl_->enable_cull_face(enable);
}

int WebGLContext::get_canvas_width() const {
    return impl_->get_canvas_width();
}

int WebGLContext::get_canvas_height() const {
    return impl_->get_canvas_height();
}

std::string WebGLContext::get_webgl_version() const {
    return impl_->get_webgl_version();
}

std::vector<std::string> WebGLContext::get_extensions() const {
    return impl_->get_extensions();
}

WebGLStats WebGLContext::get_stats() const {
    return impl_->get_stats();
}

} // namespace ecscope::web::graphics