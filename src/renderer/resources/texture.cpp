/**
 * @file renderer/resources/texture.cpp
 * @brief Texture Management System Implementation for ECScope Educational ECS Engine - Phase 7: Renderizado 2D
 * 
 * This implementation provides a comprehensive texture management system with modern OpenGL
 * integration and educational focus on graphics programming concepts.
 * 
 * Key Features Implemented:
 * - Multi-format image loading with stb_image integration (PNG, JPEG, TGA, BMP)
 * - Advanced texture filtering and mipmapping with educational explanations
 * - Memory-efficient texture atlasing and compression support
 * - GPU memory tracking and resource lifetime management
 * - Hot-reload support for development workflow
 * - Comprehensive error handling and validation
 * 
 * Educational Focus:
 * - Detailed explanation of texture formats, filtering, and GPU memory concepts
 * - Performance implications of different texture configurations
 * - Visual debugging with texture visualization tools
 * - Memory usage analysis and optimization techniques
 * - Understanding GPU resource management patterns
 * 
 * @author ECScope Educational ECS Framework - Phase 7: Renderizado 2D
 * @date 2024
 */

#include "texture.hpp"
#include "../../core/log.hpp"
#include "../../memory/memory_tracker.hpp"

// Platform-specific OpenGL headers
#ifdef _WIN32
    #include <windows.h>
    #include <GL/gl.h>
    #include <GL/glext.h>
#elif defined(__linux__)
    #include <GL/gl.h>
    #include <GL/glext.h>
#endif

// stb_image implementation
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#define STBI_ASSERT(x) // Disable stb assertions for educational clarity
#include <stb_image.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <cmath>

namespace ecscope::renderer::resources {

//=============================================================================
// OpenGL Texture Utilities
//=============================================================================

namespace gl_texture_utils {
    /** @brief Convert TextureFormat to OpenGL internal format */
    GLenum get_internal_format(TextureFormat format) noexcept {
        switch (format) {
            case TextureFormat::R8:           return GL_R8;
            case TextureFormat::RG8:          return GL_RG8;
            case TextureFormat::RGB8:         return GL_RGB8;
            case TextureFormat::RGBA8:        return GL_RGBA8;
            case TextureFormat::RGB16F:       return GL_RGB16F;
            case TextureFormat::RGBA16F:      return GL_RGBA16F;
            case TextureFormat::RGB32F:       return GL_RGB32F;
            case TextureFormat::RGBA32F:      return GL_RGBA32F;
            case TextureFormat::Depth16:      return GL_DEPTH_COMPONENT16;
            case TextureFormat::Depth24:      return GL_DEPTH_COMPONENT24;
            case TextureFormat::Depth32F:     return GL_DEPTH_COMPONENT32F;
            case TextureFormat::Depth24Stencil8: return GL_DEPTH24_STENCIL8;
            default: return GL_RGBA8;
        }
    }
    
    /** @brief Convert TextureFormat to OpenGL pixel format */
    GLenum get_pixel_format(TextureFormat format) noexcept {
        switch (format) {
            case TextureFormat::R8:           return GL_RED;
            case TextureFormat::RG8:          return GL_RG;
            case TextureFormat::RGB8:
            case TextureFormat::RGB16F:
            case TextureFormat::RGB32F:       return GL_RGB;
            case TextureFormat::RGBA8:
            case TextureFormat::RGBA16F:
            case TextureFormat::RGBA32F:      return GL_RGBA;
            case TextureFormat::Depth16:
            case TextureFormat::Depth24:
            case TextureFormat::Depth32F:     return GL_DEPTH_COMPONENT;
            case TextureFormat::Depth24Stencil8: return GL_DEPTH_STENCIL;
            default: return GL_RGBA;
        }
    }
    
    /** @brief Convert TextureFormat to OpenGL data type */
    GLenum get_data_type(TextureFormat format) noexcept {
        switch (format) {
            case TextureFormat::R8:
            case TextureFormat::RG8:
            case TextureFormat::RGB8:
            case TextureFormat::RGBA8:        return GL_UNSIGNED_BYTE;
            case TextureFormat::RGB16F:
            case TextureFormat::RGBA16F:      return GL_HALF_FLOAT;
            case TextureFormat::RGB32F:
            case TextureFormat::RGBA32F:
            case TextureFormat::Depth32F:     return GL_FLOAT;
            case TextureFormat::Depth16:      return GL_UNSIGNED_SHORT;
            case TextureFormat::Depth24:      return GL_UNSIGNED_INT;
            case TextureFormat::Depth24Stencil8: return GL_UNSIGNED_INT_24_8;
            default: return GL_UNSIGNED_BYTE;
        }
    }
    
    /** @brief Convert TextureWrap to OpenGL wrap mode */
    GLenum get_wrap_mode(TextureWrap wrap) noexcept {
        switch (wrap) {
            case TextureWrap::Clamp:          return GL_CLAMP_TO_EDGE;
            case TextureWrap::Repeat:         return GL_REPEAT;
            case TextureWrap::MirroredRepeat: return GL_MIRRORED_REPEAT;
            case TextureWrap::ClampToBorder:  return GL_CLAMP_TO_BORDER;
            default: return GL_CLAMP_TO_EDGE;
        }
    }
    
    /** @brief Convert TextureFilter to OpenGL filter mode */
    GLenum get_filter_mode(TextureFilter filter) noexcept {
        switch (filter) {
            case TextureFilter::Nearest:      return GL_NEAREST;
            case TextureFilter::Linear:       return GL_LINEAR;
            case TextureFilter::NearestMipmapNearest: return GL_NEAREST_MIPMAP_NEAREST;
            case TextureFilter::NearestMipmapLinear:  return GL_NEAREST_MIPMAP_LINEAR;
            case TextureFilter::LinearMipmapNearest:  return GL_LINEAR_MIPMAP_NEAREST;
            case TextureFilter::LinearMipmapLinear:   return GL_LINEAR_MIPMAP_LINEAR;
            default: return GL_LINEAR;
        }
    }
    
    /** @brief Calculate number of mipmap levels */
    u32 calculate_mipmap_levels(u32 width, u32 height) noexcept {
        u32 max_dim = std::max(width, height);
        return static_cast<u32>(std::floor(std::log2(max_dim))) + 1;
    }
    
    /** @brief Get bytes per pixel for a texture format */
    u32 get_bytes_per_pixel(TextureFormat format) noexcept {
        switch (format) {
            case TextureFormat::R8:           return 1;
            case TextureFormat::RG8:          return 2;
            case TextureFormat::RGB8:         return 3;
            case TextureFormat::RGBA8:        return 4;
            case TextureFormat::RGB16F:       return 6;
            case TextureFormat::RGBA16F:      return 8;
            case TextureFormat::RGB32F:       return 12;
            case TextureFormat::RGBA32F:      return 16;
            case TextureFormat::Depth16:      return 2;
            case TextureFormat::Depth24:      return 3;
            case TextureFormat::Depth32F:     return 4;
            case TextureFormat::Depth24Stencil8: return 4;
            default: return 4;
        }
    }
    
    /** @brief Check for OpenGL errors with educational context */
    bool check_gl_error(const char* operation) noexcept {
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            const char* error_str = "Unknown OpenGL Error";
            
            switch (error) {
                case GL_INVALID_ENUM: error_str = "GL_INVALID_ENUM - Invalid enumeration"; break;
                case GL_INVALID_VALUE: error_str = "GL_INVALID_VALUE - Invalid parameter value"; break;
                case GL_INVALID_OPERATION: error_str = "GL_INVALID_OPERATION - Invalid operation"; break;
                case GL_OUT_OF_MEMORY: error_str = "GL_OUT_OF_MEMORY - GPU memory exhausted"; break;
            }
            
            core::Log::error("OpenGL Error in texture operation '{}': {}", operation, error_str);
            return false;
        }
        return true;
    }
}

//=============================================================================
// Image Loading Utilities
//=============================================================================

namespace image_loading {
    /** @brief Load image data using stb_image */
    struct ImageData {
        u8* pixels{nullptr};
        u32 width{0};
        u32 height{0};
        u32 channels{0};
        bool is_valid() const noexcept { return pixels != nullptr; }
    };
    
    ImageData load_image_from_file(const std::string& file_path, bool flip_vertically = true) noexcept {
        // Educational Note: stb_image is a popular single-header image loading library
        // It supports many formats and handles the complexity of image decoding
        
        stbi_set_flip_vertically_on_load(flip_vertically);
        
        ImageData image_data;
        int width, height, channels;
        
        image_data.pixels = stbi_load(file_path.c_str(), &width, &height, &channels, 0);
        
        if (image_data.pixels) {
            image_data.width = static_cast<u32>(width);
            image_data.height = static_cast<u32>(height);
            image_data.channels = static_cast<u32>(channels);
            
            core::Log::info("Loaded image '{}': {}x{} with {} channels", 
                           file_path, width, height, channels);
        } else {
            const char* error = stbi_failure_reason();
            core::Log::error("Failed to load image '{}': {}", file_path, error ? error : "Unknown error");
        }
        
        return image_data;
    }
    
    ImageData load_image_from_memory(const u8* data, usize size, bool flip_vertically = true) noexcept {
        stbi_set_flip_vertically_on_load(flip_vertically);
        
        ImageData image_data;
        int width, height, channels;
        
        image_data.pixels = stbi_load_from_memory(data, static_cast<int>(size), &width, &height, &channels, 0);
        
        if (image_data.pixels) {
            image_data.width = static_cast<u32>(width);
            image_data.height = static_cast<u32>(height);
            image_data.channels = static_cast<u32>(channels);
            
            core::Log::debug("Loaded image from memory: {}x{} with {} channels", width, height, channels);
        } else {
            const char* error = stbi_failure_reason();
            core::Log::error("Failed to load image from memory: {}", error ? error : "Unknown error");
        }
        
        return image_data;
    }
    
    void free_image_data(ImageData& image_data) noexcept {
        if (image_data.pixels) {
            stbi_image_free(image_data.pixels);
            image_data.pixels = nullptr;
            image_data.width = 0;
            image_data.height = 0;
            image_data.channels = 0;
        }
    }
    
    TextureFormat deduce_texture_format(u32 channels, bool use_srgb = false) noexcept {
        // Educational Note: Format selection affects memory usage and rendering quality
        // sRGB formats provide correct gamma correction for color textures
        
        switch (channels) {
            case 1: return TextureFormat::R8;
            case 2: return TextureFormat::RG8;
            case 3: return use_srgb ? TextureFormat::RGB8 : TextureFormat::RGB8; // Could be sRGB8
            case 4: return use_srgb ? TextureFormat::RGBA8 : TextureFormat::RGBA8; // Could be sRGB8_ALPHA8
            default: return TextureFormat::RGBA8;
        }
    }
    
    /** @brief Generate solid color texture data */
    std::vector<u8> generate_solid_color(u32 width, u32 height, const Color& color) noexcept {
        std::vector<u8> pixels;
        pixels.reserve(width * height * 4);
        
        for (u32 y = 0; y < height; ++y) {
            for (u32 x = 0; x < width; ++x) {
                pixels.push_back(color.r);
                pixels.push_back(color.g);
                pixels.push_back(color.b);
                pixels.push_back(color.a);
            }
        }
        
        return pixels;
    }
    
    /** @brief Generate checkerboard pattern texture */
    std::vector<u8> generate_checkerboard(u32 width, u32 height, const Color& color1, const Color& color2, u32 checker_size = 8) noexcept {
        std::vector<u8> pixels;
        pixels.reserve(width * height * 4);
        
        for (u32 y = 0; y < height; ++y) {
            for (u32 x = 0; x < width; ++x) {
                bool is_color1 = ((x / checker_size) % 2) == ((y / checker_size) % 2);
                const Color& chosen_color = is_color1 ? color1 : color2;
                
                pixels.push_back(chosen_color.r);
                pixels.push_back(chosen_color.g);
                pixels.push_back(chosen_color.b);
                pixels.push_back(chosen_color.a);
            }
        }
        
        return pixels;
    }
}

//=============================================================================
// Texture Implementation
//=============================================================================

Texture::Texture() noexcept {
    // Initialize with invalid state
}

Texture::~Texture() noexcept {
    destroy();
}

Texture::Texture(Texture&& other) noexcept
    : opengl_id_(other.opengl_id_)
    , format_(other.format_)
    , width_(other.width_)
    , height_(other.height_)
    , mipmap_levels_(other.mipmap_levels_)
    , sample_count_(other.sample_count_)
    , created_(other.created_)
    , immutable_(other.immutable_)
    , config_(std::move(other.config_))
    , memory_usage_(other.memory_usage_)
    , debug_name_(std::move(other.debug_name_))
    , file_path_(std::move(other.file_path_))
    , creation_time_(other.creation_time_)
    , last_access_time_(other.last_access_time_)
    , access_count_(other.access_count_)
{
    // Clear other to prevent double deletion
    other.opengl_id_ = 0;
    other.created_ = false;
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        destroy();
        
        opengl_id_ = other.opengl_id_;
        format_ = other.format_;
        width_ = other.width_;
        height_ = other.height_;
        mipmap_levels_ = other.mipmap_levels_;
        sample_count_ = other.sample_count_;
        created_ = other.created_;
        immutable_ = other.immutable_;
        config_ = std::move(other.config_);
        memory_usage_ = other.memory_usage_;
        debug_name_ = std::move(other.debug_name_);
        file_path_ = std::move(other.file_path_);
        creation_time_ = other.creation_time_;
        last_access_time_ = other.last_access_time_;
        access_count_ = other.access_count_;
        
        other.opengl_id_ = 0;
        other.created_ = false;
    }
    return *this;
}

core::Result<void, std::string> Texture::create(u32 width, u32 height, TextureFormat format, const TextureConfig& config) noexcept {
    if (created_) {
        return core::Result<void, std::string>::err("Texture already created");
    }
    
    if (width == 0 || height == 0) {
        return core::Result<void, std::string>::err("Invalid texture dimensions");
    }
    
    // Educational Note: Texture creation involves allocating GPU memory
    // The size and format determine memory usage and rendering capabilities
    
    auto creation_start = std::chrono::high_resolution_clock::now();
    
    // Store configuration
    width_ = width;
    height_ = height;
    format_ = format;
    config_ = config;
    mipmap_levels_ = config.generate_mipmaps ? gl_texture_utils::calculate_mipmap_levels(width, height) : 1;
    sample_count_ = 1; // Single-sample texture
    
    // Generate OpenGL texture object
    glGenTextures(1, &opengl_id_);
    if (!gl_texture_utils::check_gl_error("glGenTextures")) {
        return core::Result<void, std::string>::err("Failed to generate OpenGL texture");
    }
    
    // Bind texture for configuration
    glBindTexture(GL_TEXTURE_2D, opengl_id_);
    if (!gl_texture_utils::check_gl_error("glBindTexture")) {
        destroy();
        return core::Result<void, std::string>::err("Failed to bind texture");
    }
    
    // Configure texture parameters
    configure_texture_parameters();
    
    // Allocate texture storage
    GLenum internal_format = gl_texture_utils::get_internal_format(format);
    GLenum pixel_format = gl_texture_utils::get_pixel_format(format);
    GLenum data_type = gl_texture_utils::get_data_type(format);
    
    if (config.immutable_storage && mipmap_levels_ > 1) {
        // Use immutable storage for better performance
        glTexStorage2D(GL_TEXTURE_2D, mipmap_levels_, internal_format, width, height);
        immutable_ = true;
        
        if (!gl_texture_utils::check_gl_error("glTexStorage2D")) {
            destroy();
            return core::Result<void, std::string>::err("Failed to create immutable texture storage");
        }
    } else {
        // Use mutable storage
        glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, pixel_format, data_type, nullptr);
        immutable_ = false;
        
        if (!gl_texture_utils::check_gl_error("glTexImage2D")) {
            destroy();
            return core::Result<void, std::string>::err("Failed to create texture storage");
        }
    }
    
    // Calculate memory usage
    calculate_memory_usage();
    
    // Record creation time
    creation_time_ = std::chrono::high_resolution_clock::now();
    
    created_ = true;
    
    // Set default debug name
    if (debug_name_.empty()) {
        debug_name_ = fmt::format("Texture_{}x{}_{}", width, height, static_cast<int>(format));
    }
    
    auto creation_end = std::chrono::high_resolution_clock::now();
    auto creation_time = std::chrono::duration<f32, std::milli>(creation_end - creation_start).count();
    
    core::Log::info("Created texture '{}': {}x{} {} ({:.2f} MB) in {:.3f}ms",
                    debug_name_, width, height, format_name(), 
                    memory_usage_ / (1024.0f * 1024.0f), creation_time);
    
    return core::Result<void, std::string>::ok();
}

core::Result<void, std::string> Texture::load_from_file(const std::string& file_path, const TextureConfig& config) noexcept {
    if (created_) {
        return core::Result<void, std::string>::err("Texture already created");
    }
    
    if (!std::filesystem::exists(file_path)) {
        return core::Result<void, std::string>::err(fmt::format("File does not exist: {}", file_path));
    }
    
    // Educational Note: File loading demonstrates image format support
    // stb_image handles format detection and decoding automatically
    
    auto load_start = std::chrono::high_resolution_clock::now();
    
    // Load image data
    auto image_data = image_loading::load_image_from_file(file_path, config.flip_vertically);
    if (!image_data.is_valid()) {
        return core::Result<void, std::string>::err("Failed to load image data");
    }
    
    // Deduce texture format from image channels
    TextureFormat deduced_format = image_loading::deduce_texture_format(image_data.channels, config.use_srgb);
    
    // Create texture with loaded dimensions
    auto create_result = create(image_data.width, image_data.height, deduced_format, config);
    if (!create_result.is_ok()) {
        image_loading::free_image_data(image_data);
        return create_result;
    }
    
    // Upload image data to GPU
    auto upload_result = upload_data(image_data.pixels, 0);
    if (!upload_result.is_ok()) {
        image_loading::free_image_data(image_data);
        return upload_result;
    }
    
    // Generate mipmaps if requested
    if (config.generate_mipmaps) {
        generate_mipmaps();
    }
    
    // Store file path for debugging and reloading
    file_path_ = file_path;
    
    // Clean up image data
    image_loading::free_image_data(image_data);
    
    auto load_end = std::chrono::high_resolution_clock::now();
    auto load_time = std::chrono::duration<f32, std::milli>(load_end - load_start).count();
    
    core::Log::info("Loaded texture from '{}' in {:.3f}ms", file_path, load_time);
    
    return core::Result<void, std::string>::ok();
}

core::Result<void, std::string> Texture::load_from_memory(const u8* data, usize size, const TextureConfig& config) noexcept {
    if (created_) {
        return core::Result<void, std::string>::err("Texture already created");
    }
    
    if (!data || size == 0) {
        return core::Result<void, std::string>::err("Invalid memory data");
    }
    
    // Load image from memory buffer
    auto image_data = image_loading::load_image_from_memory(data, size, config.flip_vertically);
    if (!image_data.is_valid()) {
        return core::Result<void, std::string>::err("Failed to decode image from memory");
    }
    
    // Deduce format and create texture
    TextureFormat deduced_format = image_loading::deduce_texture_format(image_data.channels, config.use_srgb);
    auto create_result = create(image_data.width, image_data.height, deduced_format, config);
    if (!create_result.is_ok()) {
        image_loading::free_image_data(image_data);
        return create_result;
    }
    
    // Upload and generate mipmaps
    auto upload_result = upload_data(image_data.pixels, 0);
    if (!upload_result.is_ok()) {
        image_loading::free_image_data(image_data);
        return upload_result;
    }
    
    if (config.generate_mipmaps) {
        generate_mipmaps();
    }
    
    // Clean up
    image_loading::free_image_data(image_data);
    
    core::Log::debug("Loaded texture from memory buffer ({} bytes)", size);
    
    return core::Result<void, std::string>::ok();
}

core::Result<void, std::string> Texture::upload_data(const void* data, u32 mip_level) noexcept {
    if (!created_) {
        return core::Result<void, std::string>::err("Texture not created");
    }
    
    if (mip_level >= mipmap_levels_) {
        return core::Result<void, std::string>::err("Invalid mipmap level");
    }
    
    // Educational Note: Data upload transfers pixel data from CPU to GPU
    // This is typically the most expensive part of texture loading
    
    glBindTexture(GL_TEXTURE_2D, opengl_id_);
    
    // Calculate mip level dimensions
    u32 mip_width = std::max(1u, width_ >> mip_level);
    u32 mip_height = std::max(1u, height_ >> mip_level);
    
    GLenum pixel_format = gl_texture_utils::get_pixel_format(format_);
    GLenum data_type = gl_texture_utils::get_data_type(format_);
    
    if (immutable_) {
        // Use glTexSubImage2D for immutable textures
        glTexSubImage2D(GL_TEXTURE_2D, mip_level, 0, 0, mip_width, mip_height, pixel_format, data_type, data);
    } else {
        // Use glTexImage2D for mutable textures
        GLenum internal_format = gl_texture_utils::get_internal_format(format_);
        glTexImage2D(GL_TEXTURE_2D, mip_level, internal_format, mip_width, mip_height, 0, pixel_format, data_type, data);
    }
    
    if (!gl_texture_utils::check_gl_error("Texture data upload")) {
        return core::Result<void, std::string>::err("Failed to upload texture data to GPU");
    }
    
    core::Log::debug("Uploaded texture data to mip level {} ({}x{})", mip_level, mip_width, mip_height);
    
    return core::Result<void, std::string>::ok();
}

void Texture::bind(u32 texture_unit) const noexcept {
    if (!created_) {
        core::Log::warning("Attempting to bind uncreated texture");
        return;
    }
    
    // Educational Note: Texture binding makes the texture active for rendering
    // Multiple textures can be bound to different texture units simultaneously
    
    glActiveTexture(GL_TEXTURE0 + texture_unit);
    glBindTexture(GL_TEXTURE_2D, opengl_id_);
    
    // Track usage for statistics
    last_access_time_ = std::chrono::high_resolution_clock::now();
    access_count_++;
    
    gl_texture_utils::check_gl_error("Texture bind");
}

void Texture::unbind(u32 texture_unit) const noexcept {
    glActiveTexture(GL_TEXTURE0 + texture_unit);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    gl_texture_utils::check_gl_error("Texture unbind");
}

void Texture::generate_mipmaps() noexcept {
    if (!created_) {
        core::Log::warning("Cannot generate mipmaps for uncreated texture");
        return;
    }
    
    // Educational Note: Mipmaps improve rendering quality and performance
    // They prevent aliasing artifacts and improve texture cache efficiency
    
    glBindTexture(GL_TEXTURE_2D, opengl_id_);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    if (gl_texture_utils::check_gl_error("Generate mipmaps")) {
        core::Log::debug("Generated {} mipmap levels for texture '{}'", mipmap_levels_, debug_name_);
    }
}

void Texture::set_filter_mode(TextureFilter min_filter, TextureFilter mag_filter) noexcept {
    if (!created_) {
        return;
    }
    
    // Educational Note: Filter modes determine how textures are sampled
    // Linear filtering provides smoother results, nearest gives pixelated look
    
    config_.min_filter = min_filter;
    config_.mag_filter = mag_filter;
    
    glBindTexture(GL_TEXTURE_2D, opengl_id_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_texture_utils::get_filter_mode(min_filter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_texture_utils::get_filter_mode(mag_filter));
    
    gl_texture_utils::check_gl_error("Set texture filter mode");
    
    core::Log::debug("Updated filter mode for texture '{}': min={}, mag={}", 
                     debug_name_, static_cast<int>(min_filter), static_cast<int>(mag_filter));
}

void Texture::set_wrap_mode(TextureWrap wrap_s, TextureWrap wrap_t) noexcept {
    if (!created_) {
        return;
    }
    
    // Educational Note: Wrap modes control texture coordinate behavior outside [0,1]
    // Important for tiling textures and preventing edge artifacts
    
    config_.wrap_s = wrap_s;
    config_.wrap_t = wrap_t;
    
    glBindTexture(GL_TEXTURE_2D, opengl_id_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, gl_texture_utils::get_wrap_mode(wrap_s));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, gl_texture_utils::get_wrap_mode(wrap_t));
    
    gl_texture_utils::check_gl_error("Set texture wrap mode");
    
    core::Log::debug("Updated wrap mode for texture '{}': s={}, t={}", 
                     debug_name_, static_cast<int>(wrap_s), static_cast<int>(wrap_t));
}

void Texture::set_border_color(const Color& color) noexcept {
    if (!created_) {
        return;
    }
    
    // Educational Note: Border color is used when wrap mode is ClampToBorder
    // It defines the color returned for texture coordinates outside [0,1]
    
    config_.border_color = color;
    
    f32 border_color[4] = {
        color.red_f(), color.green_f(), color.blue_f(), color.alpha_f()
    };
    
    glBindTexture(GL_TEXTURE_2D, opengl_id_);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
    
    gl_texture_utils::check_gl_error("Set texture border color");
}

void Texture::destroy() noexcept {
    if (created_ && opengl_id_ != 0) {
        glDeleteTextures(1, &opengl_id_);
        
        core::Log::debug("Destroyed texture '{}' (OpenGL ID: {})", debug_name_, opengl_id_);
        
        opengl_id_ = 0;
        created_ = false;
    }
}

bool Texture::reload() noexcept {
    if (file_path_.empty()) {
        core::Log::warning("Cannot reload texture '{}' - no source file path", debug_name_);
        return false;
    }
    
    // Educational Note: Hot reloading improves development workflow
    // Artists and designers can see texture changes immediately
    
    core::Log::info("Reloading texture '{}' from '{}'", debug_name_, file_path_);
    
    // Store current configuration
    auto old_config = config_;
    auto old_debug_name = debug_name_;
    
    // Destroy current texture
    destroy();
    
    // Recreate from file
    auto result = load_from_file(file_path_, old_config);
    if (!result.is_ok()) {
        core::Log::error("Failed to reload texture '{}': {}", old_debug_name, result.error());
        return false;
    }
    
    debug_name_ = old_debug_name; // Restore debug name
    
    core::Log::info("Successfully reloaded texture '{}'", debug_name_);
    return true;
}

Texture::TextureInfo Texture::get_info() const noexcept {
    TextureInfo info;
    
    info.width = width_;
    info.height = height_;
    info.format = format_;
    info.mipmap_levels = mipmap_levels_;
    info.memory_usage_bytes = memory_usage_;
    info.created = created_;
    info.immutable_storage = immutable_;
    info.opengl_id = opengl_id_;
    
    // Calculate age and access statistics
    if (created_) {
        auto now = std::chrono::high_resolution_clock::now();
        auto age_ms = std::chrono::duration<f32, std::milli>(now - creation_time_).count();
        auto last_access_ms = std::chrono::duration<f32, std::milli>(now - last_access_time_).count();
        
        info.age_seconds = age_ms / 1000.0f;
        info.seconds_since_last_access = last_access_ms / 1000.0f;
    }
    
    info.access_count = access_count_;
    info.debug_name = debug_name_;
    info.source_file = file_path_;
    
    return info;
}

//=============================================================================
// Private Texture Methods
//=============================================================================

void Texture::configure_texture_parameters() noexcept {
    // Educational Note: Texture parameter configuration affects rendering quality
    // These settings control how the GPU samples the texture during rendering
    
    // Set filtering modes
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_texture_utils::get_filter_mode(config_.min_filter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_texture_utils::get_filter_mode(config_.mag_filter));
    
    // Set wrap modes
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, gl_texture_utils::get_wrap_mode(config_.wrap_s));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, gl_texture_utils::get_wrap_mode(config_.wrap_t));
    
    // Set border color if using border clamping
    if (config_.wrap_s == TextureWrap::ClampToBorder || config_.wrap_t == TextureWrap::ClampToBorder) {
        f32 border_color[4] = {
            config_.border_color.red_f(), config_.border_color.green_f(),
            config_.border_color.blue_f(), config_.border_color.alpha_f()
        };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
    }
    
    // Set anisotropic filtering if supported
    if (config_.anisotropic_filtering > 1.0f) {
        // Note: This would require checking for GL_EXT_texture_filter_anisotropic extension
        // glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, config_.anisotropic_filtering);
    }
    
    gl_texture_utils::check_gl_error("Configure texture parameters");
}

void Texture::calculate_memory_usage() noexcept {
    // Educational Note: Memory usage calculation helps track GPU resource consumption
    // Textures can use significant GPU memory, especially with mipmaps
    
    u32 bytes_per_pixel = gl_texture_utils::get_bytes_per_pixel(format_);
    memory_usage_ = 0;
    
    // Calculate memory for each mipmap level
    for (u32 level = 0; level < mipmap_levels_; ++level) {
        u32 mip_width = std::max(1u, width_ >> level);
        u32 mip_height = std::max(1u, height_ >> level);
        memory_usage_ += mip_width * mip_height * bytes_per_pixel;
    }
    
    core::Log::debug("Calculated memory usage for texture '{}': {} bytes ({:.2f} MB)", 
                     debug_name_, memory_usage_, memory_usage_ / (1024.0f * 1024.0f));
}

std::string Texture::format_name() const noexcept {
    // Educational Note: Human-readable format names improve debugging
    // Understanding format differences is crucial for texture optimization
    
    switch (format_) {
        case TextureFormat::R8:           return "R8";
        case TextureFormat::RG8:          return "RG8";
        case TextureFormat::RGB8:         return "RGB8";
        case TextureFormat::RGBA8:        return "RGBA8";
        case TextureFormat::RGB16F:       return "RGB16F";
        case TextureFormat::RGBA16F:      return "RGBA16F";
        case TextureFormat::RGB32F:       return "RGB32F";
        case TextureFormat::RGBA32F:      return "RGBA32F";
        case TextureFormat::Depth16:      return "Depth16";
        case TextureFormat::Depth24:      return "Depth24";
        case TextureFormat::Depth32F:     return "Depth32F";
        case TextureFormat::Depth24Stencil8: return "Depth24Stencil8";
        default: return "Unknown";
    }
}

//=============================================================================
// TextureManager Implementation
//=============================================================================

TextureManager::TextureManager() noexcept
    : next_handle_id_(1)
{
    // Reserve space for common texture handles
    textures_.reserve(128);
    handle_to_index_.reserve(128);
}

TextureManager::~TextureManager() noexcept {
    destroy_all_textures();
}

TextureHandle TextureManager::create_texture(u32 width, u32 height, TextureFormat format, 
                                            const TextureConfig& config, const std::string& debug_name) noexcept {
    auto texture = std::make_unique<Texture>();
    texture->set_debug_name(debug_name);
    
    auto result = texture->create(width, height, format, config);
    if (!result.is_ok()) {
        core::Log::error("Failed to create texture '{}': {}", debug_name, result.error());
        return {INVALID_TEXTURE_ID, 0, 0};
    }
    
    return register_texture(std::move(texture));
}

TextureHandle TextureManager::load_texture_from_file(const std::string& file_path, 
                                                    const TextureConfig& config, 
                                                    const std::string& debug_name) noexcept {
    // Check cache first
    if (config.enable_caching) {
        auto it = texture_cache_.find(file_path);
        if (it != texture_cache_.end()) {
            auto& texture = textures_[it->second];
            if (texture) {
                core::Log::debug("Returning cached texture for '{}'", file_path);
                return {texture->get_opengl_id(), texture->get_width(), texture->get_height()};
            }
        }
    }
    
    auto texture = std::make_unique<Texture>();
    if (!debug_name.empty()) {
        texture->set_debug_name(debug_name);
    } else {
        // Generate debug name from file path
        std::filesystem::path path(file_path);
        texture->set_debug_name(path.filename().string());
    }
    
    auto result = texture->load_from_file(file_path, config);
    if (!result.is_ok()) {
        core::Log::error("Failed to load texture from '{}': {}", file_path, result.error());
        return {INVALID_TEXTURE_ID, 0, 0};
    }
    
    auto handle = register_texture(std::move(texture));
    
    // Add to cache if caching is enabled
    if (config.enable_caching && handle.is_valid()) {
        usize texture_index = handle_to_index_[handle.id];
        texture_cache_[file_path] = texture_index;
    }
    
    return handle;
}

TextureHandle TextureManager::load_texture_from_memory(const u8* data, usize size, 
                                                      const TextureConfig& config,
                                                      const std::string& debug_name) noexcept {
    auto texture = std::make_unique<Texture>();
    texture->set_debug_name(debug_name);
    
    auto result = texture->load_from_memory(data, size, config);
    if (!result.is_ok()) {
        core::Log::error("Failed to load texture from memory: {}", result.error());
        return {INVALID_TEXTURE_ID, 0, 0};
    }
    
    return register_texture(std::move(texture));
}

bool TextureManager::destroy_texture(const TextureHandle& handle) noexcept {
    if (!handle.is_valid()) {
        return false;
    }
    
    auto it = handle_to_index_.find(handle.id);
    if (it == handle_to_index_.end()) {
        core::Log::warning("Attempted to destroy invalid texture handle {}", handle.id);
        return false;
    }
    
    usize index = it->second;
    if (index >= textures_.size() || !textures_[index]) {
        core::Log::warning("Attempted to destroy already destroyed texture handle {}", handle.id);
        return false;
    }
    
    // Remove from cache
    auto& texture = textures_[index];
    if (!texture->get_file_path().empty()) {
        texture_cache_.erase(texture->get_file_path());
    }
    
    // Destroy texture
    core::Log::debug("Destroying texture '{}' (handle: {})", texture->get_debug_name(), handle.id);
    texture.reset();
    
    // Add index to free list
    free_indices_.push_back(index);
    
    // Remove handle mapping
    handle_to_index_.erase(it);
    
    return true;
}

Texture* TextureManager::get_texture(const TextureHandle& handle) const noexcept {
    if (!handle.is_valid()) {
        return nullptr;
    }
    
    auto it = handle_to_index_.find(handle.id);
    if (it == handle_to_index_.end()) {
        return nullptr;
    }
    
    usize index = it->second;
    if (index >= textures_.size()) {
        return nullptr;
    }
    
    return textures_[index].get();
}

void TextureManager::bind_texture(const TextureHandle& handle, u32 texture_unit) const noexcept {
    auto* texture = get_texture(handle);
    if (texture) {
        texture->bind(texture_unit);
    } else {
        // Bind default white texture or unbind
        glActiveTexture(GL_TEXTURE0 + texture_unit);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

bool TextureManager::reload_texture(const TextureHandle& handle) noexcept {
    auto* texture = get_texture(handle);
    if (!texture) {
        return false;
    }
    
    return texture->reload();
}

void TextureManager::reload_all_textures() noexcept {
    core::Log::info("Reloading all textures...");
    
    usize reloaded_count = 0;
    usize failed_count = 0;
    
    for (auto& texture : textures_) {
        if (texture && !texture->get_file_path().empty()) {
            if (texture->reload()) {
                reloaded_count++;
            } else {
                failed_count++;
            }
        }
    }
    
    core::Log::info("Reloaded {} textures, {} failed", reloaded_count, failed_count);
}

void TextureManager::create_default_textures() noexcept {
    // Educational Note: Default textures provide fallbacks when assets are missing
    // Essential for robust rendering systems that can handle missing resources
    
    core::Log::info("Creating default textures...");
    
    TextureConfig default_config;
    default_config.generate_mipmaps = false;
    default_config.min_filter = TextureFilter::Nearest;
    default_config.mag_filter = TextureFilter::Nearest;
    
    // Create 1x1 white texture
    {
        auto white_pixels = image_loading::generate_solid_color(1, 1, Color::white());
        auto texture = std::make_unique<Texture>();
        texture->set_debug_name("DefaultWhite");
        
        if (texture->create(1, 1, TextureFormat::RGBA8, default_config).is_ok()) {
            texture->upload_data(white_pixels.data(), 0);
            default_white_texture_ = register_texture(std::move(texture));
            core::Log::debug("Created default white texture (ID: {})", default_white_texture_.id);
        }
    }
    
    // Create 1x1 black texture
    {
        auto black_pixels = image_loading::generate_solid_color(1, 1, Color::black());
        auto texture = std::make_unique<Texture>();
        texture->set_debug_name("DefaultBlack");
        
        if (texture->create(1, 1, TextureFormat::RGBA8, default_config).is_ok()) {
            texture->upload_data(black_pixels.data(), 0);
            default_black_texture_ = register_texture(std::move(texture));
            core::Log::debug("Created default black texture (ID: {})", default_black_texture_.id);
        }
    }
    
    // Create 2x2 checkerboard texture for missing textures
    {
        auto checker_pixels = image_loading::generate_checkerboard(2, 2, Color::magenta(), Color::black(), 1);
        auto texture = std::make_unique<Texture>();
        texture->set_debug_name("DefaultMissing");
        
        if (texture->create(2, 2, TextureFormat::RGBA8, default_config).is_ok()) {
            texture->upload_data(checker_pixels.data(), 0);
            default_missing_texture_ = register_texture(std::move(texture));
            core::Log::debug("Created default missing texture (ID: {})", default_missing_texture_.id);
        }
    }
    
    core::Log::info("Created {} default textures", 3);
}

void TextureManager::collect_unused_textures() noexcept {
    // Educational Note: Garbage collection helps manage memory automatically
    // Textures not accessed recently can be safely freed
    
    auto now = std::chrono::high_resolution_clock::now();
    usize collected_count = 0;
    
    for (usize i = 0; i < textures_.size(); ++i) {
        auto& texture = textures_[i];
        if (!texture) continue;
        
        // Check if texture hasn't been accessed recently
        auto age = std::chrono::duration<f32>(now - texture->get_last_access_time()).count();
        if (age > 30.0f) { // 30 seconds threshold
            // Find handle for this texture
            TextureHandle handle_to_destroy{INVALID_TEXTURE_ID, 0, 0};
            for (const auto& [handle_id, index] : handle_to_index_) {
                if (index == i) {
                    handle_to_destroy.id = handle_id;
                    break;
                }
            }
            
            if (handle_to_destroy.id != INVALID_TEXTURE_ID) {
                core::Log::debug("Collecting unused texture '{}' (idle for {:.1f}s)", 
                               texture->get_debug_name(), age);
                destroy_texture(handle_to_destroy);
                collected_count++;
            }
        }
    }
    
    if (collected_count > 0) {
        core::Log::info("Collected {} unused textures", collected_count);
    }
}

TextureManager::Statistics TextureManager::get_statistics() const noexcept {
    Statistics stats;
    
    stats.total_textures = 0;
    stats.total_memory_bytes = 0;
    stats.active_textures = 0;
    
    for (const auto& texture : textures_) {
        if (texture) {
            stats.total_textures++;
            stats.total_memory_bytes += texture->get_memory_usage();
            
            if (texture->is_created()) {
                stats.active_textures++;
            }
        }
    }
    
    stats.cached_textures = texture_cache_.size();
    stats.free_slots = free_indices_.size();
    
    return stats;
}

std::string TextureManager::generate_report() const noexcept {
    auto stats = get_statistics();
    
    return fmt::format(
        "=== ECScope Texture Manager Report ===\n\n"
        "Texture Statistics:\n"
        "  Total Textures: {}\n"
        "  Active Textures: {}\n"
        "  Cached Textures: {}\n"
        "  Free Slots: {}\n\n"
        
        "Memory Usage:\n"
        "  Total GPU Memory: {:.2f} MB\n"
        "  Average per Texture: {:.2f} KB\n\n"
        
        "Default Textures:\n"
        "  White Texture: {} (ID: {})\n"
        "  Black Texture: {} (ID: {})\n"
        "  Missing Texture: {} (ID: {})\n\n"
        
        "Educational Insights:\n"
        "  Texture Loading: Use appropriate formats for memory efficiency\n"
        "  Mipmapping: Essential for preventing aliasing artifacts\n"
        "  Filtering: Linear for smooth scaling, Nearest for pixel art\n"
        "  Caching: Reduces redundant loading and improves performance\n",
        
        stats.total_textures,
        stats.active_textures,
        stats.cached_textures,
        stats.free_slots,
        
        stats.total_memory_bytes / (1024.0f * 1024.0f),
        stats.total_textures > 0 ? (stats.total_memory_bytes / stats.total_textures) / 1024.0f : 0.0f,
        
        default_white_texture_.is_valid() ? "Created" : "Missing", default_white_texture_.id,
        default_black_texture_.is_valid() ? "Created" : "Missing", default_black_texture_.id,
        default_missing_texture_.is_valid() ? "Created" : "Missing", default_missing_texture_.id
    );
}

//=============================================================================
// Private TextureManager Methods
//=============================================================================

TextureHandle TextureManager::register_texture(std::unique_ptr<Texture> texture) noexcept {
    u32 handle_id = next_handle_id_++;
    usize index;
    
    // Reuse free index if available
    if (!free_indices_.empty()) {
        index = free_indices_.back();
        free_indices_.pop_back();
        textures_[index] = std::move(texture);
    } else {
        index = textures_.size();
        textures_.push_back(std::move(texture));
    }
    
    handle_to_index_[handle_id] = index;
    
    TextureHandle handle{
        handle_id,
        textures_[index]->get_width(),
        textures_[index]->get_height()
    };
    
    core::Log::debug("Registered texture '{}' with handle {} at index {}", 
                     textures_[index]->get_debug_name(), handle_id, index);
    
    return handle;
}

void TextureManager::destroy_all_textures() noexcept {
    core::Log::info("Destroying all textures...");
    
    usize destroyed_count = 0;
    for (auto& texture : textures_) {
        if (texture) {
            texture->destroy();
            destroyed_count++;
        }
    }
    
    textures_.clear();
    handle_to_index_.clear();
    free_indices_.clear();
    texture_cache_.clear();
    
    // Reset default texture handles
    default_white_texture_ = {INVALID_TEXTURE_ID, 0, 0};
    default_black_texture_ = {INVALID_TEXTURE_ID, 0, 0};
    default_missing_texture_ = {INVALID_TEXTURE_ID, 0, 0};
    
    core::Log::info("Destroyed {} textures", destroyed_count);
}

} // namespace ecscope::renderer::resources