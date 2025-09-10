/**
 * @file opengl_renderer.cpp
 * @brief Professional OpenGL Rendering Backend Implementation
 * 
 * High-performance OpenGL 4.5+ implementation with modern features,
 * direct state access, and robust resource management.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#include "ecscope/rendering/opengl_backend.hpp"
#include <GLFW/glfw3.h>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <cassert>
#include <cstring>

namespace ecscope::rendering {

// =============================================================================
// OPENGL FORMAT CONVERSION UTILITIES
// =============================================================================

GLenum texture_format_to_gl_internal(TextureFormat format) {
    switch (format) {
        case TextureFormat::R8:             return GL_R8;
        case TextureFormat::RG8:            return GL_RG8;
        case TextureFormat::RGB8:           return GL_RGB8;
        case TextureFormat::RGBA8:          return GL_RGBA8;
        case TextureFormat::R16F:           return GL_R16F;
        case TextureFormat::RG16F:          return GL_RG16F;
        case TextureFormat::RGB16F:         return GL_RGB16F;
        case TextureFormat::RGBA16F:        return GL_RGBA16F;
        case TextureFormat::R32F:           return GL_R32F;
        case TextureFormat::RG32F:          return GL_RG32F;
        case TextureFormat::RGB32F:         return GL_RGB32F;
        case TextureFormat::RGBA32F:        return GL_RGBA32F;
        case TextureFormat::SRGB8:          return GL_SRGB8;
        case TextureFormat::SRGBA8:         return GL_SRGB8_ALPHA8;
        case TextureFormat::Depth16:        return GL_DEPTH_COMPONENT16;
        case TextureFormat::Depth24:        return GL_DEPTH_COMPONENT24;
        case TextureFormat::Depth32F:       return GL_DEPTH_COMPONENT32F;
        case TextureFormat::Depth24Stencil8: return GL_DEPTH24_STENCIL8;
        case TextureFormat::Depth32FStencil8: return GL_DEPTH32F_STENCIL8;
        case TextureFormat::BC1_RGB:        return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
        case TextureFormat::BC1_RGBA:       return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        case TextureFormat::BC3_RGBA:       return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        case TextureFormat::BC4_R:          return GL_COMPRESSED_RED_RGTC1;
        case TextureFormat::BC5_RG:         return GL_COMPRESSED_RG_RGTC2;
        case TextureFormat::BC6H_RGB_UF16:  return GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT;
        case TextureFormat::BC7_RGBA:       return GL_COMPRESSED_RGBA_BPTC_UNORM;
        default:                            return GL_RGBA8;
    }
}

std::pair<GLenum, GLenum> texture_format_to_gl_format_type(TextureFormat format) {
    switch (format) {
        case TextureFormat::R8:             return {GL_RED, GL_UNSIGNED_BYTE};
        case TextureFormat::RG8:            return {GL_RG, GL_UNSIGNED_BYTE};
        case TextureFormat::RGB8:           return {GL_RGB, GL_UNSIGNED_BYTE};
        case TextureFormat::RGBA8:          return {GL_RGBA, GL_UNSIGNED_BYTE};
        case TextureFormat::R16F:           return {GL_RED, GL_HALF_FLOAT};
        case TextureFormat::RG16F:          return {GL_RG, GL_HALF_FLOAT};
        case TextureFormat::RGB16F:         return {GL_RGB, GL_HALF_FLOAT};
        case TextureFormat::RGBA16F:        return {GL_RGBA, GL_HALF_FLOAT};
        case TextureFormat::R32F:           return {GL_RED, GL_FLOAT};
        case TextureFormat::RG32F:          return {GL_RG, GL_FLOAT};
        case TextureFormat::RGB32F:         return {GL_RGB, GL_FLOAT};
        case TextureFormat::RGBA32F:        return {GL_RGBA, GL_FLOAT};
        case TextureFormat::SRGB8:          return {GL_RGB, GL_UNSIGNED_BYTE};
        case TextureFormat::SRGBA8:         return {GL_RGBA, GL_UNSIGNED_BYTE};
        case TextureFormat::Depth16:        return {GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT};
        case TextureFormat::Depth24:        return {GL_DEPTH_COMPONENT, GL_UNSIGNED_INT};
        case TextureFormat::Depth32F:       return {GL_DEPTH_COMPONENT, GL_FLOAT};
        case TextureFormat::Depth24Stencil8: return {GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8};
        case TextureFormat::Depth32FStencil8: return {GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV};
        default:                            return {GL_RGBA, GL_UNSIGNED_BYTE};
    }
}

GLenum buffer_usage_to_gl(BufferUsage usage) {
    switch (usage) {
        case BufferUsage::Static:    return GL_STATIC_DRAW;
        case BufferUsage::Dynamic:   return GL_DYNAMIC_DRAW;
        case BufferUsage::Streaming: return GL_STREAM_DRAW;
        case BufferUsage::Staging:   return GL_DYNAMIC_READ;
        default:                     return GL_STATIC_DRAW;
    }
}

GLenum primitive_topology_to_gl(PrimitiveTopology topology) {
    switch (topology) {
        case PrimitiveTopology::TriangleList:  return GL_TRIANGLES;
        case PrimitiveTopology::TriangleStrip: return GL_TRIANGLE_STRIP;
        case PrimitiveTopology::TriangleFan:   return GL_TRIANGLE_FAN;
        case PrimitiveTopology::LineList:      return GL_LINES;
        case PrimitiveTopology::LineStrip:     return GL_LINE_STRIP;
        case PrimitiveTopology::PointList:     return GL_POINTS;
        default:                               return GL_TRIANGLES;
    }
}

GLenum compare_op_to_gl(CompareOp op) {
    switch (op) {
        case CompareOp::Never:        return GL_NEVER;
        case CompareOp::Less:         return GL_LESS;
        case CompareOp::Equal:        return GL_EQUAL;
        case CompareOp::LessEqual:    return GL_LEQUAL;
        case CompareOp::Greater:      return GL_GREATER;
        case CompareOp::NotEqual:     return GL_NOTEQUAL;
        case CompareOp::GreaterEqual: return GL_GEQUAL;
        case CompareOp::Always:       return GL_ALWAYS;
        default:                      return GL_LESS;
    }
}

GLenum vertex_format_to_gl_type(TextureFormat format) {
    switch (format) {
        case TextureFormat::R8:
        case TextureFormat::RG8:
        case TextureFormat::RGB8:
        case TextureFormat::RGBA8:  return GL_UNSIGNED_BYTE;
        case TextureFormat::R16F:
        case TextureFormat::RG16F:
        case TextureFormat::RGB16F:
        case TextureFormat::RGBA16F: return GL_HALF_FLOAT;
        case TextureFormat::R32F:
        case TextureFormat::RG32F:
        case TextureFormat::RGB32F:
        case TextureFormat::RGBA32F: return GL_FLOAT;
        default:                     return GL_FLOAT;
    }
}

GLint vertex_format_to_component_count(TextureFormat format) {
    switch (format) {
        case TextureFormat::R8:
        case TextureFormat::R16F:
        case TextureFormat::R32F:   return 1;
        case TextureFormat::RG8:
        case TextureFormat::RG16F:
        case TextureFormat::RG32F:  return 2;
        case TextureFormat::RGB8:
        case TextureFormat::RGB16F:
        case TextureFormat::RGB32F: return 3;
        case TextureFormat::RGBA8:
        case TextureFormat::RGBA16F:
        case TextureFormat::RGBA32F: return 4;
        default:                     return 3;
    }
}

// =============================================================================
// OPENGL RENDERER IMPLEMENTATION
// =============================================================================

OpenGLRenderer::OpenGLRenderer() {
    // Initialize push constant buffer
    push_constant_buffer_.size = MAX_PUSH_CONSTANT_SIZE;
}

OpenGLRenderer::~OpenGLRenderer() {
    shutdown();
}

bool OpenGLRenderer::initialize(RenderingAPI api) {
    if (context_initialized_) {
        return true;
    }

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return false;
    }

    // Check OpenGL version
    auto [major, minor] = get_opengl_version();
    if (major < 4 || (major == 4 && minor < 5)) {
        std::cerr << "OpenGL 4.5+ required, but only " << major << "." << minor << " is available" << std::endl;
        return false;
    }

    std::cout << "OpenGL " << major << "." << minor << " initialized successfully" << std::endl;

    // Query capabilities
    query_capabilities();

    // Setup debug callback if in debug mode
    #ifndef NDEBUG
    set_debug_output(true);
    #endif

    // Initialize push constant buffer
    glCreateBuffers(1, &push_constant_buffer_.buffer_id);
    glNamedBufferData(push_constant_buffer_.buffer_id, MAX_PUSH_CONSTANT_SIZE, nullptr, GL_DYNAMIC_DRAW);

    // Initialize default state
    current_state_ = GLState{};
    cached_state_ = GLState{};

    // Initialize GPU timer for profiling
    glCreateQueries(GL_TIME_ELAPSED, 1, &gpu_timer_.query_id);

    context_initialized_ = true;
    return true;
}

void OpenGLRenderer::shutdown() {
    if (!context_initialized_) {
        return;
    }

    // Wait for all operations to complete
    wait_idle();

    // Clean up resources
    for (auto& [id, buffer] : buffers_) {
        if (buffer.buffer_id != 0) {
            glDeleteBuffers(1, &buffer.buffer_id);
        }
    }
    buffers_.clear();

    for (auto& [id, texture] : textures_) {
        if (texture.texture_id != 0) {
            glDeleteTextures(1, &texture.texture_id);
        }
        if (texture.sampler_id != 0) {
            glDeleteSamplers(1, &texture.sampler_id);
        }
    }
    textures_.clear();

    for (auto& [id, shader] : shaders_) {
        if (shader.program_id != 0) {
            glDeleteProgram(shader.program_id);
        }
        if (shader.vertex_shader_id != 0) {
            glDeleteShader(shader.vertex_shader_id);
        }
        if (shader.fragment_shader_id != 0) {
            glDeleteShader(shader.fragment_shader_id);
        }
        if (shader.compute_shader_id != 0) {
            glDeleteShader(shader.compute_shader_id);
        }
    }
    shaders_.clear();

    for (auto& [id, vao] : vertex_arrays_) {
        if (vao.vao_id != 0) {
            glDeleteVertexArrays(1, &vao.vao_id);
        }
    }
    vertex_arrays_.clear();

    for (auto& [id, fbo] : framebuffers_) {
        if (fbo.fbo_id != 0) {
            glDeleteFramebuffers(1, &fbo.fbo_id);
        }
    }
    framebuffers_.clear();

    for (auto& [id, fence] : fences_) {
        if (fence != nullptr) {
            glDeleteSync(fence);
        }
    }
    fences_.clear();

    // Clean up push constant buffer
    if (push_constant_buffer_.buffer_id != 0) {
        glDeleteBuffers(1, &push_constant_buffer_.buffer_id);
        push_constant_buffer_.buffer_id = 0;
    }

    // Clean up GPU timer
    if (gpu_timer_.query_id != 0) {
        glDeleteQueries(1, &gpu_timer_.query_id);
        gpu_timer_.query_id = 0;
    }

    context_initialized_ = false;
}

IRenderer::RendererCaps OpenGLRenderer::get_capabilities() const {
    return capabilities_;
}

void OpenGLRenderer::query_capabilities() {
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, reinterpret_cast<GLint*>(&capabilities_.max_texture_size));
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, reinterpret_cast<GLint*>(&capabilities_.max_3d_texture_size));
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, reinterpret_cast<GLint*>(&capabilities_.max_array_texture_layers));
    glGetIntegerv(GL_MAX_SAMPLES, reinterpret_cast<GLint*>(&capabilities_.max_msaa_samples));
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, reinterpret_cast<GLfloat*>(&capabilities_.max_anisotropy));

    // Check for feature support
    capabilities_.supports_compute_shaders = glewIsSupported("GL_ARB_compute_shader");
    capabilities_.supports_tessellation = glewIsSupported("GL_ARB_tessellation_shader");
    capabilities_.supports_geometry_shaders = glewIsSupported("GL_ARB_geometry_shader4");
    capabilities_.supports_bindless_resources = glewIsSupported("GL_ARB_bindless_texture");
    capabilities_.supports_ray_tracing = false; // OpenGL doesn't support ray tracing

    // Get texture unit limits
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_texture_units_);
    glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &max_uniform_buffer_bindings_);
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &max_storage_buffer_bindings_);
}

// =============================================================================
// DEBUG AND ERROR HANDLING
// =============================================================================

void OpenGLRenderer::set_debug_output(bool enable) {
    debug_output_enabled_ = enable;
    
    if (enable && glewIsSupported("GL_KHR_debug")) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(debug_callback, this);
        
        // Filter out notification messages
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
        
        std::cout << "OpenGL debug output enabled" << std::endl;
    } else if (enable) {
        std::cout << "OpenGL debug output not supported" << std::endl;
    }
}

void GLAPIENTRY OpenGLRenderer::debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                             GLsizei length, const GLchar* message, const void* user_param) {
    // Skip certain types of messages
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) {
        return;
    }

    std::string source_str;
    switch (source) {
        case GL_DEBUG_SOURCE_API:             source_str = "API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   source_str = "Window System"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: source_str = "Shader Compiler"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     source_str = "Third Party"; break;
        case GL_DEBUG_SOURCE_APPLICATION:     source_str = "Application"; break;
        case GL_DEBUG_SOURCE_OTHER:           source_str = "Other"; break;
        default:                              source_str = "Unknown"; break;
    }

    std::string type_str;
    switch (type) {
        case GL_DEBUG_TYPE_ERROR:               type_str = "Error"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: type_str = "Deprecated Behavior"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  type_str = "Undefined Behavior"; break;
        case GL_DEBUG_TYPE_PORTABILITY:         type_str = "Portability"; break;
        case GL_DEBUG_TYPE_PERFORMANCE:         type_str = "Performance"; break;
        case GL_DEBUG_TYPE_MARKER:              type_str = "Marker"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          type_str = "Push Group"; break;
        case GL_DEBUG_TYPE_POP_GROUP:           type_str = "Pop Group"; break;
        case GL_DEBUG_TYPE_OTHER:               type_str = "Other"; break;
        default:                                type_str = "Unknown"; break;
    }

    std::string severity_str;
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:         severity_str = "High"; break;
        case GL_DEBUG_SEVERITY_MEDIUM:       severity_str = "Medium"; break;
        case GL_DEBUG_SEVERITY_LOW:          severity_str = "Low"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: severity_str = "Notification"; break;
        default:                             severity_str = "Unknown"; break;
    }

    std::cerr << "[OpenGL " << severity_str << " " << type_str << " from " << source_str 
              << " (ID: " << id << ")]: " << message << std::endl;

    // Break on high severity errors in debug builds
    #ifndef NDEBUG
    if (severity == GL_DEBUG_SEVERITY_HIGH && type == GL_DEBUG_TYPE_ERROR) {
        assert(false && "High severity OpenGL error occurred");
    }
    #endif
}

void OpenGLRenderer::check_gl_error(const std::string& operation) const {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::string error_str;
        switch (error) {
            case GL_INVALID_ENUM:      error_str = "GL_INVALID_ENUM"; break;
            case GL_INVALID_VALUE:     error_str = "GL_INVALID_VALUE"; break;
            case GL_INVALID_OPERATION: error_str = "GL_INVALID_OPERATION"; break;
            case GL_OUT_OF_MEMORY:     error_str = "GL_OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: error_str = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
            default:                   error_str = "Unknown error (" + std::to_string(error) + ")"; break;
        }
        
        std::cerr << "OpenGL error in " << operation << ": " << error_str << std::endl;
        
        #ifndef NDEBUG
        assert(false && "OpenGL error occurred");
        #endif
    }
}

OpenGLRenderer::ContextInfo OpenGLRenderer::get_context_info() const {
    ContextInfo info;
    
    info.vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    info.renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    info.version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    info.glsl_version = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
    
    GLint num_extensions;
    glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);
    
    for (GLint i = 0; i < num_extensions; ++i) {
        const char* extension = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
        if (extension) {
            info.extensions.emplace_back(extension);
        }
    }
    
    return info;
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

bool is_opengl_available() {
    // This would typically check if OpenGL context can be created
    // For now, assume it's available if GLEW can be initialized
    return glewIsSupported("GL_VERSION_4_5");
}

std::pair<int, int> get_opengl_version() {
    GLint major, minor;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    return {major, minor};
}

bool is_extension_supported(const std::string& extension_name) {
    return glewIsSupported(extension_name.c_str());
}

// =============================================================================
// RESOURCE MANAGEMENT - BUFFERS
// =============================================================================

BufferHandle OpenGLRenderer::create_buffer(const BufferDesc& desc, const void* initial_data) {
    if (desc.size == 0) {
        std::cerr << "Cannot create buffer with size 0" << std::endl;
        return BufferHandle{};
    }

    uint64_t id = next_resource_id_.fetch_add(1);
    OpenGLBuffer buffer;
    
    // Create buffer object using DSA
    glCreateBuffers(1, &buffer.buffer_id);
    check_gl_error("glCreateBuffers");
    
    if (buffer.buffer_id == 0) {
        std::cerr << "Failed to create OpenGL buffer" << std::endl;
        return BufferHandle{};
    }
    
    // Set buffer properties
    buffer.size = desc.size;
    buffer.usage = buffer_usage_to_gl(desc.usage);
    buffer.debug_name = desc.debug_name;
    
    // Allocate buffer storage
    GLbitfield flags = 0;
    if (!desc.gpu_only) {
        // Allow CPU access for non-GPU-only buffers
        if (desc.usage == BufferUsage::Dynamic || desc.usage == BufferUsage::Streaming) {
            flags |= GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT;
        } else {
            flags |= GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
        }
    }
    
    glNamedBufferStorage(buffer.buffer_id, desc.size, initial_data, flags);
    check_gl_error("glNamedBufferStorage");
    
    // Set debug label if available and name is provided
    if (!desc.debug_name.empty() && glewIsSupported("GL_KHR_debug")) {
        glObjectLabel(GL_BUFFER, buffer.buffer_id, -1, desc.debug_name.c_str());
    }
    
    buffers_[id] = std::move(buffer);
    return BufferHandle{id};
}

void OpenGLRenderer::destroy_buffer(BufferHandle handle) {
    if (!handle.is_valid()) {
        return;
    }
    
    auto it = buffers_.find(handle.id());
    if (it != buffers_.end()) {
        if (it->second.buffer_id != 0) {
            glDeleteBuffers(1, &it->second.buffer_id);
        }
        buffers_.erase(it);
    }
}

void OpenGLRenderer::update_buffer(BufferHandle handle, size_t offset, size_t size, const void* data) {
    if (!handle.is_valid() || !data) {
        return;
    }
    
    auto it = buffers_.find(handle.id());
    if (it == buffers_.end()) {
        std::cerr << "Invalid buffer handle for update" << std::endl;
        return;
    }
    
    const auto& buffer = it->second;
    if (offset + size > static_cast<size_t>(buffer.size)) {
        std::cerr << "Buffer update out of bounds" << std::endl;
        return;
    }
    
    // Use glNamedBufferSubData for DSA
    glNamedBufferSubData(buffer.buffer_id, offset, size, data);
    check_gl_error("glNamedBufferSubData");
}

// =============================================================================
// RESOURCE MANAGEMENT - TEXTURES
// =============================================================================

TextureHandle OpenGLRenderer::create_texture(const TextureDesc& desc, const void* initial_data) {
    if (desc.width == 0 || desc.height == 0) {
        std::cerr << "Cannot create texture with zero dimensions" << std::endl;
        return TextureHandle{};
    }
    
    uint64_t id = next_resource_id_.fetch_add(1);
    OpenGLTexture texture;
    
    // Determine texture target
    if (desc.array_layers > 1) {
        if (desc.depth > 1) {
            texture.target = GL_TEXTURE_2D_ARRAY; // 3D texture arrays not commonly used
        } else {
            texture.target = desc.samples > 1 ? GL_TEXTURE_2D_MULTISAMPLE_ARRAY : GL_TEXTURE_2D_ARRAY;
        }
    } else if (desc.depth > 1) {
        texture.target = GL_TEXTURE_3D;
    } else if (desc.samples > 1) {
        texture.target = GL_TEXTURE_2D_MULTISAMPLE;
    } else {
        texture.target = GL_TEXTURE_2D;
    }
    
    // Create texture using DSA
    glCreateTextures(texture.target, 1, &texture.texture_id);
    check_gl_error("glCreateTextures");
    
    if (texture.texture_id == 0) {
        std::cerr << "Failed to create OpenGL texture" << std::endl;
        return TextureHandle{};
    }
    
    // Set texture properties
    texture.width = desc.width;
    texture.height = desc.height;
    texture.depth = desc.depth;
    texture.levels = desc.mip_levels;
    texture.layers = desc.array_layers;
    texture.samples = desc.samples;
    texture.internal_format = texture_format_to_gl_internal(desc.format);
    texture.is_render_target = desc.render_target;
    texture.is_depth_stencil = desc.depth_stencil;
    texture.debug_name = desc.debug_name;
    
    auto [format, type] = texture_format_to_gl_format_type(desc.format);
    texture.format = format;
    texture.type = type;
    
    // Allocate texture storage
    try {
        if (texture.target == GL_TEXTURE_2D_MULTISAMPLE) {
            glTextureStorage2DMultisample(texture.texture_id, desc.samples, texture.internal_format, 
                                        desc.width, desc.height, GL_TRUE);
        } else if (texture.target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY) {
            glTextureStorage3DMultisample(texture.texture_id, desc.samples, texture.internal_format,
                                        desc.width, desc.height, desc.array_layers, GL_TRUE);
        } else if (texture.target == GL_TEXTURE_3D) {
            glTextureStorage3D(texture.texture_id, desc.mip_levels, texture.internal_format,
                             desc.width, desc.height, desc.depth);
        } else if (texture.target == GL_TEXTURE_2D_ARRAY) {
            glTextureStorage3D(texture.texture_id, desc.mip_levels, texture.internal_format,
                             desc.width, desc.height, desc.array_layers);
        } else {
            glTextureStorage2D(texture.texture_id, desc.mip_levels, texture.internal_format,
                             desc.width, desc.height);
        }
        check_gl_error("texture storage allocation");
        
        // Upload initial data if provided
        if (initial_data && desc.samples == 1) {
            if (texture.target == GL_TEXTURE_3D) {
                glTextureSubImage3D(texture.texture_id, 0, 0, 0, 0, 
                                  desc.width, desc.height, desc.depth, 
                                  format, type, initial_data);
            } else if (texture.target == GL_TEXTURE_2D_ARRAY) {
                glTextureSubImage3D(texture.texture_id, 0, 0, 0, 0,
                                  desc.width, desc.height, desc.array_layers,
                                  format, type, initial_data);
            } else {
                glTextureSubImage2D(texture.texture_id, 0, 0, 0,
                                  desc.width, desc.height,
                                  format, type, initial_data);
            }
            check_gl_error("texture data upload");
        }
        
    } catch (...) {
        glDeleteTextures(1, &texture.texture_id);
        std::cerr << "Failed to allocate texture storage" << std::endl;
        return TextureHandle{};
    }
    
    // Create default sampler
    glCreateSamplers(1, &texture.sampler_id);
    glSamplerParameteri(texture.sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glSamplerParameteri(texture.sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(texture.sampler_id, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glSamplerParameteri(texture.sampler_id, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glSamplerParameteri(texture.sampler_id, GL_TEXTURE_WRAP_R, GL_REPEAT);
    
    // Set debug label
    if (!desc.debug_name.empty() && glewIsSupported("GL_KHR_debug")) {
        glObjectLabel(GL_TEXTURE, texture.texture_id, -1, desc.debug_name.c_str());
        glObjectLabel(GL_SAMPLER, texture.sampler_id, -1, 
                     (desc.debug_name + "_sampler").c_str());
    }
    
    textures_[id] = std::move(texture);
    return TextureHandle{id};
}

void OpenGLRenderer::destroy_texture(TextureHandle handle) {
    if (!handle.is_valid()) {
        return;
    }
    
    auto it = textures_.find(handle.id());
    if (it != textures_.end()) {
        if (it->second.texture_id != 0) {
            glDeleteTextures(1, &it->second.texture_id);
        }
        if (it->second.sampler_id != 0) {
            glDeleteSamplers(1, &it->second.sampler_id);
        }
        textures_.erase(it);
    }
}

void OpenGLRenderer::update_texture(TextureHandle handle, uint32_t mip_level, uint32_t array_layer,
                                   uint32_t x, uint32_t y, uint32_t z,
                                   uint32_t width, uint32_t height, uint32_t depth,
                                   const void* data) {
    if (!handle.is_valid() || !data) {
        return;
    }
    
    auto it = textures_.find(handle.id());
    if (it == textures_.end()) {
        std::cerr << "Invalid texture handle for update" << std::endl;
        return;
    }
    
    const auto& texture = it->second;
    
    // Validate parameters
    if (mip_level >= static_cast<uint32_t>(texture.levels) ||
        array_layer >= static_cast<uint32_t>(texture.layers) ||
        x + width > static_cast<uint32_t>(texture.width) ||
        y + height > static_cast<uint32_t>(texture.height)) {
        std::cerr << "Texture update parameters out of bounds" << std::endl;
        return;
    }
    
    // Update texture data based on target
    try {
        if (texture.target == GL_TEXTURE_3D) {
            glTextureSubImage3D(texture.texture_id, mip_level, x, y, z,
                              width, height, depth, texture.format, texture.type, data);
        } else if (texture.target == GL_TEXTURE_2D_ARRAY) {
            glTextureSubImage3D(texture.texture_id, mip_level, x, y, array_layer,
                              width, height, 1, texture.format, texture.type, data);
        } else {
            glTextureSubImage2D(texture.texture_id, mip_level, x, y,
                              width, height, texture.format, texture.type, data);
        }
        check_gl_error("texture update");
    } catch (...) {
        std::cerr << "Failed to update texture data" << std::endl;
    }
}

void OpenGLRenderer::generate_mipmaps(TextureHandle handle) {
    if (!handle.is_valid()) {
        return;
    }
    
    auto it = textures_.find(handle.id());
    if (it == textures_.end()) {
        std::cerr << "Invalid texture handle for mipmap generation" << std::endl;
        return;
    }
    
    const auto& texture = it->second;
    if (texture.samples > 1) {
        std::cerr << "Cannot generate mipmaps for multisample textures" << std::endl;
        return;
    }
    
    glGenerateTextureMipmap(texture.texture_id);
    check_gl_error("mipmap generation");
}

// =============================================================================
// RESOURCE MANAGEMENT - SHADERS
// =============================================================================

GLuint OpenGLRenderer::compile_shader(GLenum shader_type, const std::string& source, const std::string& name) {
    GLuint shader = glCreateShader(shader_type);
    if (shader == 0) {
        std::cerr << "Failed to create shader object for " << name << std::endl;
        return 0;
    }
    
    const char* source_ptr = source.c_str();
    glShaderSource(shader, 1, &source_ptr, nullptr);
    glCompileShader(shader);
    
    // Check compilation status
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        GLint log_length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
        
        std::vector<char> log(log_length);
        glGetShaderInfoLog(shader, log_length, nullptr, log.data());
        
        std::string shader_type_name;
        switch (shader_type) {
            case GL_VERTEX_SHADER:   shader_type_name = "vertex"; break;
            case GL_FRAGMENT_SHADER: shader_type_name = "fragment"; break;
            case GL_COMPUTE_SHADER:  shader_type_name = "compute"; break;
            case GL_GEOMETRY_SHADER: shader_type_name = "geometry"; break;
            case GL_TESS_CONTROL_SHADER: shader_type_name = "tessellation control"; break;
            case GL_TESS_EVALUATION_SHADER: shader_type_name = "tessellation evaluation"; break;
            default: shader_type_name = "unknown"; break;
        }
        
        std::cerr << "Failed to compile " << shader_type_name << " shader '" << name << "':\n" 
                  << log.data() << std::endl;
        
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

GLuint OpenGLRenderer::link_program(GLuint vertex_shader, GLuint fragment_shader, const std::string& name) {
    GLuint program = glCreateProgram();
    if (program == 0) {
        std::cerr << "Failed to create shader program for " << name << std::endl;
        return 0;
    }
    
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    
    // Check link status
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        GLint log_length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
        
        std::vector<char> log(log_length);
        glGetProgramInfoLog(program, log_length, nullptr, log.data());
        
        std::cerr << "Failed to link shader program '" << name << "':\n" << log.data() << std::endl;
        
        glDeleteProgram(program);
        return 0;
    }
    
    // Validate program
    glValidateProgram(program);
    GLint validation_status;
    glGetProgramiv(program, GL_VALIDATE_STATUS, &validation_status);
    if (validation_status == GL_FALSE) {
        GLint log_length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
        
        std::vector<char> log(log_length);
        glGetProgramInfoLog(program, log_length, nullptr, log.data());
        
        std::cerr << "Shader program validation failed for '" << name << "':\n" << log.data() << std::endl;
        // Don't fail on validation warnings, just log them
    }
    
    return program;
}

GLuint OpenGLRenderer::link_compute_program(GLuint compute_shader, const std::string& name) {
    GLuint program = glCreateProgram();
    if (program == 0) {
        std::cerr << "Failed to create compute shader program for " << name << std::endl;
        return 0;
    }
    
    glAttachShader(program, compute_shader);
    glLinkProgram(program);
    
    // Check link status
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        GLint log_length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
        
        std::vector<char> log(log_length);
        glGetProgramInfoLog(program, log_length, nullptr, log.data());
        
        std::cerr << "Failed to link compute shader program '" << name << "':\n" << log.data() << std::endl;
        
        glDeleteProgram(program);
        return 0;
    }
    
    return program;
}

void OpenGLRenderer::cache_shader_uniforms(OpenGLShader& shader) {
    GLint uniform_count;
    glGetProgramiv(shader.program_id, GL_ACTIVE_UNIFORMS, &uniform_count);
    
    for (GLint i = 0; i < uniform_count; ++i) {
        char name[256];
        GLsizei length;
        GLint size;
        GLenum type;
        
        glGetActiveUniform(shader.program_id, i, sizeof(name), &length, &size, &type, name);
        GLint location = glGetUniformLocation(shader.program_id, name);
        
        if (location != -1) {
            shader.uniform_locations[name] = location;
        }
    }
    
    // Cache uniform block indices
    GLint uniform_block_count;
    glGetProgramiv(shader.program_id, GL_ACTIVE_UNIFORM_BLOCKS, &uniform_block_count);
    
    for (GLint i = 0; i < uniform_block_count; ++i) {
        char name[256];
        GLsizei length;
        
        glGetActiveUniformBlockName(shader.program_id, i, sizeof(name), &length, name);
        GLuint index = glGetUniformBlockIndex(shader.program_id, name);
        
        if (index != GL_INVALID_INDEX) {
            shader.uniform_block_indices[name] = index;
        }
    }
    
    // Cache storage buffer indices if supported
    if (glewIsSupported("GL_ARB_shader_storage_buffer_object")) {
        GLint storage_block_count;
        glGetProgramInterfaceiv(shader.program_id, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, &storage_block_count);
        
        for (GLint i = 0; i < storage_block_count; ++i) {
            char name[256];
            GLsizei length;
            
            glGetProgramResourceName(shader.program_id, GL_SHADER_STORAGE_BLOCK, i, sizeof(name), &length, name);
            GLuint index = glGetProgramResourceIndex(shader.program_id, GL_SHADER_STORAGE_BLOCK, name);
            
            if (index != GL_INVALID_INDEX) {
                shader.storage_block_indices[name] = index;
            }
        }
    }
}

ShaderHandle OpenGLRenderer::create_shader(const std::string& vertex_source, 
                                          const std::string& fragment_source,
                                          const std::string& debug_name) {
    if (vertex_source.empty() || fragment_source.empty()) {
        std::cerr << "Vertex and fragment shader sources cannot be empty" << std::endl;
        return ShaderHandle{};
    }
    
    uint64_t id = next_resource_id_.fetch_add(1);
    OpenGLShader shader;
    
    // Compile vertex shader
    shader.vertex_shader_id = compile_shader(GL_VERTEX_SHADER, vertex_source, debug_name + "_vertex");
    if (shader.vertex_shader_id == 0) {
        return ShaderHandle{};
    }
    
    // Compile fragment shader
    shader.fragment_shader_id = compile_shader(GL_FRAGMENT_SHADER, fragment_source, debug_name + "_fragment");
    if (shader.fragment_shader_id == 0) {
        glDeleteShader(shader.vertex_shader_id);
        return ShaderHandle{};
    }
    
    // Link program
    shader.program_id = link_program(shader.vertex_shader_id, shader.fragment_shader_id, debug_name);
    if (shader.program_id == 0) {
        glDeleteShader(shader.vertex_shader_id);
        glDeleteShader(shader.fragment_shader_id);
        return ShaderHandle{};
    }
    
    shader.is_compute_shader = false;
    shader.debug_name = debug_name;
    
    // Cache uniform locations and block indices
    cache_shader_uniforms(shader);
    
    // Set debug label
    if (!debug_name.empty() && glewIsSupported("GL_KHR_debug")) {
        glObjectLabel(GL_PROGRAM, shader.program_id, -1, debug_name.c_str());
    }
    
    shaders_[id] = std::move(shader);
    return ShaderHandle{id};
}

ShaderHandle OpenGLRenderer::create_compute_shader(const std::string& compute_source,
                                                  const std::string& debug_name) {
    if (compute_source.empty()) {
        std::cerr << "Compute shader source cannot be empty" << std::endl;
        return ShaderHandle{};
    }
    
    if (!capabilities_.supports_compute_shaders) {
        std::cerr << "Compute shaders are not supported on this system" << std::endl;
        return ShaderHandle{};
    }
    
    uint64_t id = next_resource_id_.fetch_add(1);
    OpenGLShader shader;
    
    // Compile compute shader
    shader.compute_shader_id = compile_shader(GL_COMPUTE_SHADER, compute_source, debug_name);
    if (shader.compute_shader_id == 0) {
        return ShaderHandle{};
    }
    
    // Link program
    shader.program_id = link_compute_program(shader.compute_shader_id, debug_name);
    if (shader.program_id == 0) {
        glDeleteShader(shader.compute_shader_id);
        return ShaderHandle{};
    }
    
    shader.is_compute_shader = true;
    shader.debug_name = debug_name;
    
    // Cache uniform locations and block indices
    cache_shader_uniforms(shader);
    
    // Set debug label
    if (!debug_name.empty() && glewIsSupported("GL_KHR_debug")) {
        glObjectLabel(GL_PROGRAM, shader.program_id, -1, debug_name.c_str());
    }
    
    shaders_[id] = std::move(shader);
    return ShaderHandle{id};
}

void OpenGLRenderer::destroy_shader(ShaderHandle handle) {
    if (!handle.is_valid()) {
        return;
    }
    
    auto it = shaders_.find(handle.id());
    if (it != shaders_.end()) {
        const auto& shader = it->second;
        
        if (shader.program_id != 0) {
            glDeleteProgram(shader.program_id);
        }
        if (shader.vertex_shader_id != 0) {
            glDeleteShader(shader.vertex_shader_id);
        }
        if (shader.fragment_shader_id != 0) {
            glDeleteShader(shader.fragment_shader_id);
        }
        if (shader.compute_shader_id != 0) {
            glDeleteShader(shader.compute_shader_id);
        }
        
        shaders_.erase(it);
    }
}

// =============================================================================
// FRAME MANAGEMENT
// =============================================================================

void OpenGLRenderer::begin_frame() {
    frame_start_time_ = std::chrono::high_resolution_clock::now();
    frame_stats_ = FrameStats{}; // Reset stats
    
    // Start GPU timing
    if (gpu_timer_.query_id != 0 && !gpu_timer_.is_active) {
        glBeginQuery(GL_TIME_ELAPSED, gpu_timer_.query_id);
        gpu_timer_.is_active = true;
    }
    
    check_gl_error("begin_frame");
}

void OpenGLRenderer::end_frame() {
    // End GPU timing
    if (gpu_timer_.is_active) {
        glEndQuery(GL_TIME_ELAPSED);
        gpu_timer_.is_active = false;
        
        // Get the result if available (non-blocking)
        GLint available;
        glGetQueryObjectiv(gpu_timer_.query_id, GL_QUERY_RESULT_AVAILABLE, &available);
        if (available) {
            GLuint64 time_ns;
            glGetQueryObjectui64v(gpu_timer_.query_id, GL_QUERY_RESULT, &time_ns);
            gpu_timer_.last_time_ns = time_ns;
            frame_stats_.gpu_time_ms = static_cast<float>(time_ns) / 1000000.0f;
        } else {
            frame_stats_.gpu_time_ms = static_cast<float>(gpu_timer_.last_time_ns) / 1000000.0f;
        }
    }
    
    // Calculate frame time
    auto frame_end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(frame_end_time - frame_start_time_);
    frame_stats_.frame_time_ms = static_cast<float>(duration.count()) / 1000.0f;
    
    // Swap buffers if we have a window
    if (window_) {
        glfwSwapBuffers(window_);
    }
    
    check_gl_error("end_frame");
}

GLuint OpenGLRenderer::create_vertex_array(const VertexLayout& layout, 
                                          std::span<const BufferHandle> vertex_buffers,
                                          BufferHandle index_buffer) {
    GLuint vao;
    glCreateVertexArrays(1, &vao);
    
    if (vao == 0) {
        std::cerr << "Failed to create vertex array object" << std::endl;
        return 0;
    }
    
    // Bind vertex buffers and set up attributes
    for (size_t i = 0; i < vertex_buffers.size() && i < layout.attributes.size(); ++i) {
        auto buffer_it = buffers_.find(vertex_buffers[i].id());
        if (buffer_it == buffers_.end()) {
            std::cerr << "Invalid vertex buffer handle" << std::endl;
            glDeleteVertexArrays(1, &vao);
            return 0;
        }
        
        // Bind vertex buffer to binding point
        glVertexArrayVertexBuffer(vao, static_cast<GLuint>(i), buffer_it->second.buffer_id, 0, layout.stride);
    }
    
    // Set up vertex attributes
    for (const auto& attr : layout.attributes) {
        GLint components = vertex_format_to_component_count(attr.format);
        GLenum type = vertex_format_to_gl_type(attr.format);
        GLboolean normalized = GL_FALSE;
        
        // Enable the attribute
        glEnableVertexArrayAttrib(vao, attr.location);
        
        // Set attribute format
        if (type == GL_FLOAT || type == GL_HALF_FLOAT) {
            glVertexArrayAttribFormat(vao, attr.location, components, type, normalized, attr.offset);
        } else {
            glVertexArrayAttribIFormat(vao, attr.location, components, type, attr.offset);
        }
        
        // Associate attribute with binding point
        glVertexArrayAttribBinding(vao, attr.location, attr.binding);
    }
    
    // Bind index buffer if provided
    if (index_buffer.is_valid()) {
        auto buffer_it = buffers_.find(index_buffer.id());
        if (buffer_it != buffers_.end()) {
            glVertexArrayElementBuffer(vao, buffer_it->second.buffer_id);
        }
    }
    
    check_gl_error("create_vertex_array");
    return vao;
}

void OpenGLRenderer::set_render_target(TextureHandle color_target, TextureHandle depth_target) {
    uint64_t fbo_id = 0;
    
    // Check if we're setting the default framebuffer (both targets invalid)
    if (!color_target.is_valid() && !depth_target.is_valid()) {
        bind_framebuffer(0);
        current_color_target_ = TextureHandle{};
        current_depth_target_ = TextureHandle{};
        current_framebuffer_id_ = 0;
        return;
    }
    
    // Find or create framebuffer object
    bool found = false;
    for (const auto& [id, fbo] : framebuffers_) {
        if ((fbo.color_attachments.empty() && !color_target.is_valid()) ||
            (!fbo.color_attachments.empty() && fbo.color_attachments[0] == color_target)) {
            if ((!fbo.has_depth_attachment && !depth_target.is_valid()) ||
                (fbo.has_depth_attachment && fbo.depth_attachment == depth_target)) {
                fbo_id = id;
                found = true;
                break;
            }
        }
    }
    
    if (!found) {
        // Create new framebuffer
        fbo_id = next_resource_id_.fetch_add(1);
        OpenGLFramebuffer fbo;
        
        glCreateFramebuffers(1, &fbo.fbo_id);
        if (fbo.fbo_id == 0) {
            std::cerr << "Failed to create framebuffer object" << std::endl;
            return;
        }
        
        // Attach color target
        if (color_target.is_valid()) {
            auto color_it = textures_.find(color_target.id());
            if (color_it != textures_.end()) {
                const auto& texture = color_it->second;
                
                if (texture.target == GL_TEXTURE_2D_MULTISAMPLE) {
                    glNamedFramebufferTexture(fbo.fbo_id, GL_COLOR_ATTACHMENT0, texture.texture_id, 0);
                } else {
                    glNamedFramebufferTexture(fbo.fbo_id, GL_COLOR_ATTACHMENT0, texture.texture_id, 0);
                }
                
                fbo.color_attachments.push_back(color_target);
                fbo.width = texture.width;
                fbo.height = texture.height;
            }
        }
        
        // Attach depth target
        if (depth_target.is_valid()) {
            auto depth_it = textures_.find(depth_target.id());
            if (depth_it != textures_.end()) {
                const auto& texture = depth_it->second;
                
                GLenum attachment = GL_DEPTH_ATTACHMENT;
                if (texture.internal_format == GL_DEPTH24_STENCIL8 || 
                    texture.internal_format == GL_DEPTH32F_STENCIL8) {
                    attachment = GL_DEPTH_STENCIL_ATTACHMENT;
                }
                
                if (texture.target == GL_TEXTURE_2D_MULTISAMPLE) {
                    glNamedFramebufferTexture(fbo.fbo_id, attachment, texture.texture_id, 0);
                } else {
                    glNamedFramebufferTexture(fbo.fbo_id, attachment, texture.texture_id, 0);
                }
                
                fbo.depth_attachment = depth_target;
                fbo.has_depth_attachment = true;
                
                if (fbo.width == 0) {
                    fbo.width = texture.width;
                    fbo.height = texture.height;
                }
            }
        }
        
        // Check framebuffer completeness
        GLenum status = glCheckNamedFramebufferStatus(fbo.fbo_id, GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Framebuffer is not complete: " << std::hex << status << std::dec << std::endl;
            glDeleteFramebuffers(1, &fbo.fbo_id);
            return;
        }
        
        // Set draw buffers
        if (color_target.is_valid()) {
            GLenum draw_buffer = GL_COLOR_ATTACHMENT0;
            glNamedFramebufferDrawBuffers(fbo.fbo_id, 1, &draw_buffer);
        } else {
            glNamedFramebufferDrawBuffer(fbo.fbo_id, GL_NONE);
            glNamedFramebufferReadBuffer(fbo.fbo_id, GL_NONE);
        }
        
        framebuffers_[fbo_id] = std::move(fbo);
    }
    
    // Bind the framebuffer
    const auto& fbo = framebuffers_[fbo_id];
    bind_framebuffer(fbo.fbo_id);
    
    current_color_target_ = color_target;
    current_depth_target_ = depth_target;
    current_framebuffer_id_ = fbo_id;
    
    check_gl_error("set_render_target");
}

void OpenGLRenderer::clear(const std::array<float, 4>& color, float depth, uint8_t stencil) {
    // Update clear values if they've changed
    if (current_state_.clear_color != color) {
        glClearColor(color[0], color[1], color[2], color[3]);
        current_state_.clear_color = color;
    }
    
    if (current_state_.clear_depth != depth) {
        glClearDepth(depth);
        current_state_.clear_depth = depth;
    }
    
    if (current_state_.clear_stencil != stencil) {
        glClearStencil(stencil);
        current_state_.clear_stencil = stencil;
    }
    
    // Determine what to clear
    GLbitfield clear_flags = 0;
    
    if (current_color_target_.is_valid() || current_framebuffer_id_ == 0) {
        clear_flags |= GL_COLOR_BUFFER_BIT;
    }
    
    if (current_depth_target_.is_valid() || current_framebuffer_id_ == 0) {
        clear_flags |= GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
    }
    
    if (clear_flags != 0) {
        glClear(clear_flags);
        check_gl_error("clear");
    }
}

void OpenGLRenderer::set_viewport(const Viewport& viewport) {
    if (current_state_.viewport_x != static_cast<GLint>(viewport.x) ||
        current_state_.viewport_y != static_cast<GLint>(viewport.y) ||
        current_state_.viewport_width != static_cast<GLsizei>(viewport.width) ||
        current_state_.viewport_height != static_cast<GLsizei>(viewport.height)) {
        
        glViewport(static_cast<GLint>(viewport.x), static_cast<GLint>(viewport.y),
                  static_cast<GLsizei>(viewport.width), static_cast<GLsizei>(viewport.height));
        
        current_state_.viewport_x = static_cast<GLint>(viewport.x);
        current_state_.viewport_y = static_cast<GLint>(viewport.y);
        current_state_.viewport_width = static_cast<GLsizei>(viewport.width);
        current_state_.viewport_height = static_cast<GLsizei>(viewport.height);
        
        check_gl_error("set_viewport");
    }
    
    // Set depth range
    glDepthRange(viewport.min_depth, viewport.max_depth);
    check_gl_error("set_depth_range");
}

void OpenGLRenderer::set_scissor(const ScissorRect& scissor) {
    if (current_state_.render_state.scissor_x != scissor.x ||
        current_state_.render_state.scissor_y != scissor.y ||
        current_state_.render_state.scissor_width != static_cast<GLsizei>(scissor.width) ||
        current_state_.render_state.scissor_height != static_cast<GLsizei>(scissor.height)) {
        
        glScissor(scissor.x, scissor.y, static_cast<GLsizei>(scissor.width), static_cast<GLsizei>(scissor.height));
        
        current_state_.render_state.scissor_x = scissor.x;
        current_state_.render_state.scissor_y = scissor.y;
        current_state_.render_state.scissor_width = static_cast<GLsizei>(scissor.width);
        current_state_.render_state.scissor_height = static_cast<GLsizei>(scissor.height);
        
        check_gl_error("set_scissor");
    }
    
    // Enable scissor test if not already enabled
    if (!current_state_.render_state.scissor_test) {
        glEnable(GL_SCISSOR_TEST);
        current_state_.render_state.scissor_test = true;
        check_gl_error("enable_scissor_test");
    }
}

// =============================================================================
// PIPELINE STATE MANAGEMENT
// =============================================================================

void OpenGLRenderer::bind_vertex_array(GLuint vao) {
    if (current_state_.bound_vao != vao) {
        glBindVertexArray(vao);
        current_state_.bound_vao = vao;
        check_gl_error("bind_vertex_array");
    }
}

void OpenGLRenderer::use_program(GLuint program) {
    if (current_state_.bound_program != program) {
        glUseProgram(program);
        current_state_.bound_program = program;
        check_gl_error("use_program");
    }
}

void OpenGLRenderer::bind_texture_unit(GLuint unit, GLuint texture, GLuint sampler) {
    if (unit >= static_cast<GLuint>(max_texture_units_)) {
        std::cerr << "Texture unit " << unit << " exceeds maximum of " << max_texture_units_ << std::endl;
        return;
    }
    
    if (current_state_.bound_textures[unit] != texture) {
        glBindTextureUnit(unit, texture);
        current_state_.bound_textures[unit] = texture;
    }
    
    if (sampler != 0 && current_state_.bound_samplers[unit] != sampler) {
        glBindSampler(unit, sampler);
        current_state_.bound_samplers[unit] = sampler;
    }
    
    check_gl_error("bind_texture_unit");
}

void OpenGLRenderer::bind_uniform_buffer_range(GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) {
    if (index >= static_cast<GLuint>(max_uniform_buffer_bindings_)) {
        std::cerr << "Uniform buffer binding " << index << " exceeds maximum of " << max_uniform_buffer_bindings_ << std::endl;
        return;
    }
    
    if (current_state_.bound_uniform_buffers[index] != buffer) {
        if (size > 0) {
            glBindBufferRange(GL_UNIFORM_BUFFER, index, buffer, offset, size);
        } else {
            glBindBufferBase(GL_UNIFORM_BUFFER, index, buffer);
        }
        current_state_.bound_uniform_buffers[index] = buffer;
        check_gl_error("bind_uniform_buffer_range");
    }
}

void OpenGLRenderer::bind_storage_buffer_range(GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) {
    if (index >= static_cast<GLuint>(max_storage_buffer_bindings_)) {
        std::cerr << "Storage buffer binding " << index << " exceeds maximum of " << max_storage_buffer_bindings_ << std::endl;
        return;
    }
    
    if (current_state_.bound_storage_buffers[index] != buffer) {
        if (size > 0) {
            glBindBufferRange(GL_SHADER_STORAGE_BUFFER, index, buffer, offset, size);
        } else {
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, buffer);
        }
        current_state_.bound_storage_buffers[index] = buffer;
        check_gl_error("bind_storage_buffer_range");
    }
}

void OpenGLRenderer::bind_framebuffer(GLuint fbo) {
    if (current_state_.bound_framebuffer != fbo) {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        current_state_.bound_framebuffer = fbo;
        check_gl_error("bind_framebuffer");
    }
}

void OpenGLRenderer::set_depth_state(bool test_enable, bool write_enable, GLenum func) {
    if (current_state_.render_state.depth_test != test_enable) {
        if (test_enable) {
            glEnable(GL_DEPTH_TEST);
        } else {
            glDisable(GL_DEPTH_TEST);
        }
        current_state_.render_state.depth_test = test_enable;
    }
    
    if (current_state_.render_state.depth_write != write_enable) {
        glDepthMask(write_enable ? GL_TRUE : GL_FALSE);
        current_state_.render_state.depth_write = write_enable;
    }
    
    if (current_state_.render_state.depth_func != func) {
        glDepthFunc(func);
        current_state_.render_state.depth_func = func;
    }
    
    check_gl_error("set_depth_state");
}

void OpenGLRenderer::set_blend_state(BlendMode mode) {
    bool blend_enable = (mode != BlendMode::None);
    GLenum src_factor = GL_ONE;
    GLenum dst_factor = GL_ZERO;
    GLenum blend_eq = GL_FUNC_ADD;
    
    switch (mode) {
        case BlendMode::None:
            blend_enable = false;
            break;
            
        case BlendMode::Alpha:
            src_factor = GL_SRC_ALPHA;
            dst_factor = GL_ONE_MINUS_SRC_ALPHA;
            break;
            
        case BlendMode::Additive:
            src_factor = GL_ONE;
            dst_factor = GL_ONE;
            break;
            
        case BlendMode::Multiply:
            src_factor = GL_DST_COLOR;
            dst_factor = GL_ZERO;
            break;
            
        case BlendMode::Screen:
            src_factor = GL_ONE_MINUS_DST_COLOR;
            dst_factor = GL_ONE;
            break;
            
        case BlendMode::PremultipliedAlpha:
            src_factor = GL_ONE;
            dst_factor = GL_ONE_MINUS_SRC_ALPHA;
            break;
    }
    
    if (current_state_.render_state.blend != blend_enable) {
        if (blend_enable) {
            glEnable(GL_BLEND);
        } else {
            glDisable(GL_BLEND);
        }
        current_state_.render_state.blend = blend_enable;
    }
    
    if (blend_enable && 
        (current_state_.render_state.blend_src != src_factor ||
         current_state_.render_state.blend_dst != dst_factor)) {
        glBlendFunc(src_factor, dst_factor);
        current_state_.render_state.blend_src = src_factor;
        current_state_.render_state.blend_dst = dst_factor;
    }
    
    if (blend_enable && current_state_.render_state.blend_equation != blend_eq) {
        glBlendEquation(blend_eq);
        current_state_.render_state.blend_equation = blend_eq;
    }
    
    check_gl_error("set_blend_state");
}

void OpenGLRenderer::set_cull_state(CullMode mode) {
    bool cull_enable = (mode != CullMode::None);
    GLenum cull_face_mode = GL_BACK;
    
    switch (mode) {
        case CullMode::None:
            cull_enable = false;
            break;
        case CullMode::Front:
            cull_face_mode = GL_FRONT;
            break;
        case CullMode::Back:
            cull_face_mode = GL_BACK;
            break;
    }
    
    if (current_state_.render_state.cull_face != cull_enable) {
        if (cull_enable) {
            glEnable(GL_CULL_FACE);
        } else {
            glDisable(GL_CULL_FACE);
        }
        current_state_.render_state.cull_face = cull_enable;
    }
    
    if (cull_enable && current_state_.render_state.cull_face_mode != cull_face_mode) {
        glCullFace(cull_face_mode);
        current_state_.render_state.cull_face_mode = cull_face_mode;
    }
    
    check_gl_error("set_cull_state");
}

void OpenGLRenderer::set_wireframe_state(bool wireframe) {
    if (current_state_.render_state.wireframe != wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
        current_state_.render_state.wireframe = wireframe;
        check_gl_error("set_wireframe_state");
    }
}

void OpenGLRenderer::set_shader(ShaderHandle handle) {
    if (!handle.is_valid()) {
        use_program(0);
        current_shader_ = ShaderHandle{};
        return;
    }
    
    auto it = shaders_.find(handle.id());
    if (it == shaders_.end()) {
        std::cerr << "Invalid shader handle" << std::endl;
        return;
    }
    
    use_program(it->second.program_id);
    current_shader_ = handle;
}

void OpenGLRenderer::set_render_state(const RenderState& state) {
    set_depth_state(state.depth_test_enable, state.depth_write_enable, 
                   compare_op_to_gl(state.depth_compare_op));
    set_blend_state(state.blend_mode);
    set_cull_state(state.cull_mode);
    set_wireframe_state(state.wireframe);
}

void OpenGLRenderer::set_vertex_buffers(std::span<const BufferHandle> buffers,
                                       std::span<const uint64_t> offsets) {
    bound_vertex_buffers_.clear();
    bound_vertex_buffers_.reserve(buffers.size());
    
    for (const auto& buffer : buffers) {
        bound_vertex_buffers_.push_back(buffer);
    }
}

void OpenGLRenderer::set_index_buffer(BufferHandle buffer, size_t offset, bool use_32bit_indices) {
    bound_index_buffer_ = buffer;
    index_buffer_32bit_ = use_32bit_indices;
}

void OpenGLRenderer::set_vertex_layout(const VertexLayout& layout) {
    // Create or update vertex array object
    GLuint vao = create_vertex_array(layout, bound_vertex_buffers_, bound_index_buffer_);
    
    if (vao != 0) {
        uint64_t vao_id = next_resource_id_.fetch_add(1);
        
        OpenGLVertexArray vao_obj;
        vao_obj.vao_id = vao;
        vao_obj.layout = layout;
        vao_obj.vertex_buffers = bound_vertex_buffers_;
        vao_obj.index_buffer = bound_index_buffer_;
        vao_obj.has_index_buffer = bound_index_buffer_.is_valid();
        vao_obj.use_32bit_indices = index_buffer_32bit_;
        
        vertex_arrays_[vao_id] = std::move(vao_obj);
        
        bind_vertex_array(vao);
        current_vertex_array_id_ = vao_id;
    }
}

// =============================================================================
// RESOURCE BINDING
// =============================================================================

void OpenGLRenderer::bind_texture(uint32_t slot, TextureHandle texture) {
    if (!texture.is_valid()) {
        bind_texture_unit(slot, 0, 0);
        return;
    }
    
    auto it = textures_.find(texture.id());
    if (it == textures_.end()) {
        std::cerr << "Invalid texture handle for binding" << std::endl;
        return;
    }
    
    const auto& tex = it->second;
    bind_texture_unit(slot, tex.texture_id, tex.sampler_id);
}

void OpenGLRenderer::bind_textures(uint32_t first_slot, std::span<const TextureHandle> textures) {
    for (size_t i = 0; i < textures.size(); ++i) {
        bind_texture(first_slot + static_cast<uint32_t>(i), textures[i]);
    }
}

void OpenGLRenderer::bind_uniform_buffer(uint32_t slot, BufferHandle buffer, 
                                       size_t offset, size_t size) {
    if (!buffer.is_valid()) {
        bind_uniform_buffer_range(slot, 0, 0, 0);
        return;
    }
    
    auto it = buffers_.find(buffer.id());
    if (it == buffers_.end()) {
        std::cerr << "Invalid buffer handle for uniform buffer binding" << std::endl;
        return;
    }
    
    const auto& buf = it->second;
    bind_uniform_buffer_range(slot, buf.buffer_id, offset, size);
}

void OpenGLRenderer::bind_storage_buffer(uint32_t slot, BufferHandle buffer,
                                       size_t offset, size_t size) {
    if (!buffer.is_valid()) {
        bind_storage_buffer_range(slot, 0, 0, 0);
        return;
    }
    
    auto it = buffers_.find(buffer.id());
    if (it == buffers_.end()) {
        std::cerr << "Invalid buffer handle for storage buffer binding" << std::endl;
        return;
    }
    
    const auto& buf = it->second;
    bind_storage_buffer_range(slot, buf.buffer_id, offset, size);
}

void OpenGLRenderer::set_push_constants(uint32_t offset, uint32_t size, const void* data) {
    if (!data || size == 0 || offset + size > MAX_PUSH_CONSTANT_SIZE) {
        std::cerr << "Invalid push constant parameters" << std::endl;
        return;
    }
    
    // Update the push constant buffer
    glNamedBufferSubData(push_constant_buffer_.buffer_id, offset, size, data);
    
    // Bind it as uniform buffer at binding point 0 (reserved for push constants)
    bind_uniform_buffer_range(0, push_constant_buffer_.buffer_id, offset, size);
    
    check_gl_error("set_push_constants");
}

// =============================================================================
// DRAW COMMANDS
// =============================================================================

void OpenGLRenderer::draw_indexed(const DrawIndexedCommand& cmd) {
    if (!current_shader_.is_valid()) {
        std::cerr << "No shader bound for draw call" << std::endl;
        return;
    }
    
    if (current_vertex_array_id_ == 0) {
        std::cerr << "No vertex array bound for draw call" << std::endl;
        return;
    }
    
    auto vao_it = vertex_arrays_.find(current_vertex_array_id_);
    if (vao_it == vertex_arrays_.end() || !vao_it->second.has_index_buffer) {
        std::cerr << "No index buffer bound for indexed draw call" << std::endl;
        return;
    }
    
    GLenum index_type = vao_it->second.use_32bit_indices ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
    size_t index_size = vao_it->second.use_32bit_indices ? sizeof(uint32_t) : sizeof(uint16_t);
    
    const void* indices = reinterpret_cast<const void*>(cmd.first_index * index_size);
    
    if (cmd.instance_count > 1) {
        glDrawElementsInstancedBaseVertexBaseInstance(
            GL_TRIANGLES, 
            cmd.index_count,
            index_type,
            indices,
            cmd.instance_count,
            cmd.vertex_offset,
            cmd.first_instance
        );
    } else {
        glDrawElementsBaseVertex(
            GL_TRIANGLES,
            cmd.index_count,
            index_type,
            indices,
            cmd.vertex_offset
        );
    }
    
    // Update stats
    frame_stats_.draw_calls++;
    frame_stats_.vertices_rendered += cmd.index_count * cmd.instance_count;
    
    check_gl_error("draw_indexed");
}

void OpenGLRenderer::draw(const DrawCommand& cmd) {
    if (!current_shader_.is_valid()) {
        std::cerr << "No shader bound for draw call" << std::endl;
        return;
    }
    
    if (current_vertex_array_id_ == 0) {
        std::cerr << "No vertex array bound for draw call" << std::endl;
        return;
    }
    
    if (cmd.instance_count > 1) {
        glDrawArraysInstancedBaseInstance(
            GL_TRIANGLES,
            cmd.first_vertex,
            cmd.vertex_count,
            cmd.instance_count,
            cmd.first_instance
        );
    } else {
        glDrawArrays(
            GL_TRIANGLES,
            cmd.first_vertex,
            cmd.vertex_count
        );
    }
    
    // Update stats
    frame_stats_.draw_calls++;
    frame_stats_.vertices_rendered += cmd.vertex_count * cmd.instance_count;
    
    check_gl_error("draw");
}

void OpenGLRenderer::dispatch(const DispatchCommand& cmd) {
    if (!current_shader_.is_valid()) {
        std::cerr << "No compute shader bound for dispatch" << std::endl;
        return;
    }
    
    auto shader_it = shaders_.find(current_shader_.id());
    if (shader_it == shaders_.end() || !shader_it->second.is_compute_shader) {
        std::cerr << "Current shader is not a compute shader" << std::endl;
        return;
    }
    
    if (!capabilities_.supports_compute_shaders) {
        std::cerr << "Compute shaders not supported on this system" << std::endl;
        return;
    }
    
    glDispatchCompute(cmd.group_count_x, cmd.group_count_y, cmd.group_count_z);
    
    // Add memory barrier to ensure compute shader writes are visible
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    
    check_gl_error("dispatch");
}

// =============================================================================
// DEBUGGING & PROFILING
// =============================================================================

void OpenGLRenderer::push_debug_marker(const std::string& name) {
    debug_marker_stack_.push_back(name);
    
    if (glewIsSupported("GL_KHR_debug")) {
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, name.c_str());
    }
}

void OpenGLRenderer::pop_debug_marker() {
    if (!debug_marker_stack_.empty()) {
        debug_marker_stack_.pop_back();
        
        if (glewIsSupported("GL_KHR_debug")) {
            glPopDebugGroup();
        }
    }
}

void OpenGLRenderer::insert_debug_marker(const std::string& name) {
    if (glewIsSupported("GL_KHR_debug")) {
        glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER, 0,
                           GL_DEBUG_SEVERITY_NOTIFICATION, -1, name.c_str());
    }
}

IRenderer::FrameStats OpenGLRenderer::get_frame_stats() const {
    return frame_stats_;
}

// =============================================================================
// ADVANCED FEATURES
// =============================================================================

void OpenGLRenderer::wait_idle() {
    glFinish();
    check_gl_error("wait_idle");
}

uint64_t OpenGLRenderer::create_fence() {
    uint64_t fence_id = next_fence_id_.fetch_add(1);
    GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    
    if (fence == nullptr) {
        std::cerr << "Failed to create OpenGL fence" << std::endl;
        return 0;
    }
    
    fences_[fence_id] = fence;
    return fence_id;
}

void OpenGLRenderer::wait_for_fence(uint64_t fence_id, uint64_t timeout_ns) {
    auto it = fences_.find(fence_id);
    if (it == fences_.end()) {
        std::cerr << "Invalid fence ID: " << fence_id << std::endl;
        return;
    }
    
    GLsync fence = it->second;
    if (fence == nullptr) {
        return;
    }
    
    GLenum result = glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, timeout_ns);
    
    switch (result) {
        case GL_ALREADY_SIGNALED:
        case GL_CONDITION_SATISFIED:
            // Success
            break;
            
        case GL_TIMEOUT_EXPIRED:
            std::cerr << "Fence wait timeout expired" << std::endl;
            break;
            
        case GL_WAIT_FAILED:
            std::cerr << "Fence wait failed" << std::endl;
            check_gl_error("wait_for_fence");
            break;
    }
}

bool OpenGLRenderer::is_fence_signaled(uint64_t fence_id) {
    auto it = fences_.find(fence_id);
    if (it == fences_.end()) {
        return false;
    }
    
    GLsync fence = it->second;
    if (fence == nullptr) {
        return true;
    }
    
    GLint result;
    GLsizei length;
    glGetSynciv(fence, GL_SYNC_STATUS, sizeof(GLint), &length, &result);
    
    return result == GL_SIGNALED;
}

} // namespace ecscope::rendering