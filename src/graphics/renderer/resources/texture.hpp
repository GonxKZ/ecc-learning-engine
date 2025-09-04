#pragma once

/**
 * @file renderer/resources/texture.hpp
 * @brief Texture Resource Management System for ECScope Educational ECS Engine - Phase 7: Renderizado 2D
 * 
 * This header provides a comprehensive texture management system designed for educational
 * clarity while maintaining professional-grade performance. It includes:
 * 
 * Core Features:
 * - OpenGL 3.3+ texture resource management with RAII
 * - Multiple texture formats and compression support
 * - Automatic mipmap generation and custom mipmap chains
 * - Memory usage tracking and optimization
 * - Asynchronous texture loading and streaming
 * 
 * Educational Features:
 * - Detailed documentation of texture concepts and GPU memory management
 * - Performance metrics and memory usage analysis
 * - Debug visualization tools for texture inspection
 * - Comprehensive error handling and validation
 * - Interactive texture property modification for learning
 * 
 * Advanced Features:
 * - Texture atlasing and sprite sheet support
 * - Dynamic texture updates and streaming
 * - Compressed texture format support (DXT/BC, ETC2, ASTC)
 * - HDR texture support for advanced lighting
 * - Texture arrays and cube maps for special effects
 * 
 * Performance Characteristics:
 * - Efficient GPU memory allocation with custom allocators
 * - Texture binding optimization and state caching
 * - Automatic texture compression and format conversion
 * - Memory-mapped file loading for large textures
 * - Background streaming for seamless LOD transitions
 * 
 * @author ECScope Educational ECS Framework - Phase 7: Renderizado 2D
 * @date 2024
 */

#include "../../core/types.hpp"
#include "../../core/result.hpp"
#include "../../memory/memory_tracker.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <optional>
#include <functional>

namespace ecscope::renderer::resources {

//=============================================================================
// Forward Declarations and Type Definitions
//=============================================================================

class TextureManager;
struct TextureData;

using TextureID = u32;
constexpr TextureID INVALID_TEXTURE_ID = 0;
constexpr TextureID WHITE_TEXTURE_ID = 1;
constexpr TextureID BLACK_TEXTURE_ID = 2;
constexpr TextureID TRANSPARENT_TEXTURE_ID = 3;

//=============================================================================
// Texture Format and Properties
//=============================================================================

/**
 * @brief Texture Format Specification
 * 
 * Defines the pixel format and layout of texture data. Each format has different
 * memory requirements, quality characteristics, and GPU compatibility.
 * 
 * Educational Context:
 * Different formats serve different purposes:
 * - Uncompressed formats: Best quality, highest memory usage
 * - Compressed formats: Lower quality, significantly less memory
 * - HDR formats: Support for high dynamic range lighting
 * - Depth formats: Used for shadow mapping and depth effects
 */
enum class TextureFormat : u8 {
    // Uncompressed formats
    R8 = 0,             ///< 8-bit single channel (grayscale)
    RG8,                ///< 8-bit dual channel  
    RGB8,               ///< 8-bit RGB (24-bit color, no alpha)
    RGBA8,              ///< 8-bit RGBA (32-bit color with alpha) - most common
    
    // Higher precision formats
    R16F,               ///< 16-bit float single channel
    RG16F,              ///< 16-bit float dual channel
    RGB16F,             ///< 16-bit float RGB (HDR)
    RGBA16F,            ///< 16-bit float RGBA (HDR with alpha)
    
    R32F,               ///< 32-bit float single channel
    RG32F,              ///< 32-bit float dual channel
    RGB32F,             ///< 32-bit float RGB (full precision HDR)
    RGBA32F,            ///< 32-bit float RGBA (full precision HDR)
    
    // Compressed formats (GPU-specific compression)
    DXT1,               ///< BC1: RGB compression, 4:1 ratio, no alpha
    DXT3,               ///< BC2: RGBA compression, 4:1 ratio, explicit alpha
    DXT5,               ///< BC3: RGBA compression, 4:1 ratio, interpolated alpha
    BC4,                ///< Single channel compression
    BC5,                ///< Dual channel compression (normal maps)
    BC6H,               ///< HDR RGB compression
    BC7,                ///< High-quality RGBA compression
    
    // Mobile-optimized formats
    ETC2_RGB,           ///< ETC2 RGB compression (mobile GPUs)
    ETC2_RGBA,          ///< ETC2 RGBA compression
    ASTC_4x4,           ///< ASTC 4x4 compression (high quality)
    ASTC_8x8,           ///< ASTC 8x8 compression (balanced)
    
    // Special formats
    Depth16,            ///< 16-bit depth buffer
    Depth24,            ///< 24-bit depth buffer  
    Depth32F,           ///< 32-bit float depth buffer
    Depth24_Stencil8    ///< 24-bit depth + 8-bit stencil
};

/**
 * @brief Texture Filtering Modes
 * 
 * Determines how pixels are sampled when the texture is scaled.
 * Critical for both visual quality and performance.
 * 
 * Educational Context:
 * - Nearest: Sharp, pixelated look - good for pixel art, very fast
 * - Linear: Smooth, blurred look - good for realistic graphics, slightly slower
 * - Anisotropic: High-quality filtering for textures viewed at angles, slower
 */
enum class TextureFilter : u8 {
    Nearest = 0,        ///< Nearest neighbor sampling (pixelated)
    Linear,             ///< Bilinear filtering (smooth)
    NearestMipmap,      ///< Nearest with mipmap selection
    LinearMipmap,       ///< Bilinear with mipmap selection
    Anisotropic         ///< Anisotropic filtering (highest quality)
};

/**
 * @brief Texture Wrap Modes
 * 
 * Defines behavior when texture coordinates go outside [0,1] range.
 */
enum class TextureWrap : u8 {
    Repeat = 0,         ///< Tile the texture (u % 1.0)
    MirroredRepeat,     ///< Mirror and tile the texture
    ClampToEdge,        ///< Clamp to edge pixels (most common for sprites)
    ClampToBorder       ///< Clamp to specified border color
};

/**
 * @brief Texture Usage Hints
 * 
 * Provides optimization hints to the texture manager about how
 * the texture will be used.
 */
enum class TextureUsage : u8 {
    Static = 0,         ///< Texture data never changes after creation
    Dynamic,            ///< Texture data may be updated occasionally
    Streaming,          ///< Texture data updated frequently (video, animations)
    RenderTarget,       ///< Used as render target (framebuffer)
    DepthTarget         ///< Used as depth/stencil target
};

/**
 * @brief Comprehensive Texture Properties
 * 
 * Contains all properties that define how a texture behaves during rendering.
 * Designed to be both educational and performance-oriented.
 */
struct TextureProperties {
    //-------------------------------------------------------------------------
    // Core Properties
    //-------------------------------------------------------------------------
    
    TextureFormat format{TextureFormat::RGBA8};
    TextureFilter min_filter{TextureFilter::Linear};
    TextureFilter mag_filter{TextureFilter::Linear};
    TextureWrap wrap_u{TextureWrap::ClampToEdge};
    TextureWrap wrap_v{TextureWrap::ClampToEdge};
    TextureUsage usage{TextureUsage::Static};
    
    //-------------------------------------------------------------------------
    // Advanced Properties
    //-------------------------------------------------------------------------
    
    /** @brief Generate mipmaps automatically */
    bool generate_mipmaps{true};
    
    /** @brief Maximum anisotropic filtering level (1-16) */
    f32 max_anisotropy{1.0f};
    
    /** @brief Border color for ClampToBorder wrap mode */
    struct {
        f32 r{0.0f}, g{0.0f}, b{0.0f}, a{1.0f};
    } border_color;
    
    /** @brief Mipmap bias (-1.0 to 1.0, negative = sharper, positive = blurrier) */
    f32 mipmap_bias{0.0f};
    
    /** @brief Minimum mipmap level to use */
    u32 min_mipmap_level{0};
    
    /** @brief Maximum mipmap level to use */
    u32 max_mipmap_level{1000};
    
    //-------------------------------------------------------------------------
    // Educational and Debug Properties
    //-------------------------------------------------------------------------
    
    /** @brief Human-readable name for debugging */
    std::string debug_name{};
    
    /** @brief Whether to collect detailed usage statistics */
    bool enable_profiling{false};
    
    /** @brief Compression quality hint (0.0-1.0, higher = better quality) */
    f32 compression_quality{0.8f};
    
    //-------------------------------------------------------------------------
    // Utility Functions
    //-------------------------------------------------------------------------
    
    /** @brief Get memory requirements for given dimensions */
    usize calculate_memory_size(u32 width, u32 height, u32 mip_levels = 1) const noexcept;
    
    /** @brief Get human-readable format description */
    const char* get_format_name() const noexcept;
    
    /** @brief Check if format supports transparency */
    bool supports_alpha() const noexcept;
    
    /** @brief Check if format is compressed */
    bool is_compressed() const noexcept;
    
    /** @brief Check if format supports HDR */
    bool is_hdr() const noexcept;
    
    /** @brief Get bytes per pixel (for uncompressed formats) */
    u32 get_bytes_per_pixel() const noexcept;
    
    /** @brief Validate property combination */
    bool is_valid() const noexcept;
    
    // Factory methods for common configurations
    static TextureProperties sprite_default() noexcept {
        TextureProperties props;
        props.format = TextureFormat::RGBA8;
        props.min_filter = TextureFilter::Linear;
        props.mag_filter = TextureFilter::Linear;
        props.wrap_u = TextureWrap::ClampToEdge;
        props.wrap_v = TextureWrap::ClampToEdge;
        props.generate_mipmaps = false; // Sprites usually don't need mipmaps
        return props;
    }
    
    static TextureProperties pixel_art() noexcept {
        TextureProperties props;
        props.format = TextureFormat::RGBA8;
        props.min_filter = TextureFilter::Nearest;
        props.mag_filter = TextureFilter::Nearest;
        props.wrap_u = TextureWrap::ClampToEdge;
        props.wrap_v = TextureWrap::ClampToEdge;
        props.generate_mipmaps = false;
        return props;
    }
    
    static TextureProperties hdr_environment() noexcept {
        TextureProperties props;
        props.format = TextureFormat::RGBA16F;
        props.min_filter = TextureFilter::LinearMipmap;
        props.mag_filter = TextureFilter::Linear;
        props.wrap_u = TextureWrap::Repeat;
        props.wrap_v = TextureWrap::ClampToEdge;
        props.generate_mipmaps = true;
        return props;
    }
    
    static TextureProperties compressed_diffuse() noexcept {
        TextureProperties props;
        props.format = TextureFormat::DXT5; // RGBA compression
        props.min_filter = TextureFilter::LinearMipmap;
        props.mag_filter = TextureFilter::Linear;
        props.generate_mipmaps = true;
        props.compression_quality = 0.85f;
        return props;
    }
};

//=============================================================================
// Texture Data Storage and Management
//=============================================================================

/**
 * @brief Raw Texture Data Container
 * 
 * Holds pixel data and metadata for a texture. Supports multiple mipmap levels
 * and various pixel formats. Designed for efficient memory management and
 * educational inspection.
 */
struct TextureData {
    //-------------------------------------------------------------------------
    // Texture Metadata
    //-------------------------------------------------------------------------
    
    u32 width{0};                           ///< Texture width in pixels
    u32 height{0};                          ///< Texture height in pixels
    u32 channels{4};                        ///< Number of color channels (1-4)
    TextureFormat format{TextureFormat::RGBA8}; ///< Pixel format
    
    //-------------------------------------------------------------------------
    // Pixel Data Storage
    //-------------------------------------------------------------------------
    
    /** 
     * @brief Main texture data
     * 
     * Raw pixel data in the specified format. Memory layout depends on format:
     * - RGB8: [R,G,B,R,G,B...] 
     * - RGBA8: [R,G,B,A,R,G,B,A...]
     * - Compressed: GPU-specific compressed blocks
     */
    std::vector<u8> pixel_data{};
    
    /** 
     * @brief Mipmap level data
     * 
     * Additional mipmap levels for improved rendering quality at distance.
     * Each mipmap is half the resolution of the previous level.
     * 
     * Educational Note:
     * Mipmaps solve the "texture aliasing" problem when textures are
     * viewed at a distance. Without mipmaps, distant textures flicker
     * and shimmer due to undersampling.
     */
    std::vector<std::vector<u8>> mipmap_data{};
    
    //-------------------------------------------------------------------------
    // Creation and Loading Information
    //-------------------------------------------------------------------------
    
    std::string source_file{};              ///< Original file path (if loaded from file)
    u64 file_timestamp{0};                  ///< File modification time for hot reloading
    usize uncompressed_size{0};             ///< Original file size in bytes
    usize memory_usage{0};                  ///< Current memory usage in bytes
    
    //-------------------------------------------------------------------------
    // Educational Debug Information
    //-------------------------------------------------------------------------
    
    struct DebugInfo {
        // Color analysis
        struct {
            u8 min_r{255}, min_g{255}, min_b{255}, min_a{255};
            u8 max_r{0}, max_g{0}, max_b{0}, max_a{0};
            f32 avg_r{0}, avg_g{0}, avg_b{0}, avg_a{0};
        } color_stats;
        
        // Texture characteristics
        bool has_transparency{false};
        bool is_grayscale{false};
        bool has_alpha_channel{false};
        f32 compression_ratio{1.0f};        ///< Original size / compressed size
        
        // Performance hints
        bool suitable_for_compression{false};
        bool power_of_two{false};           ///< Width and height are powers of 2
        f32 memory_efficiency{1.0f};        ///< How efficiently memory is used
        
        void analyze_texture_data(const TextureData& data) noexcept;
    } debug_info;
    
    //-------------------------------------------------------------------------
    // Constructor and Factory Methods
    //-------------------------------------------------------------------------
    
    TextureData() noexcept = default;
    
    /** @brief Create texture data with specified dimensions and format */
    TextureData(u32 w, u32 h, TextureFormat fmt, u32 ch = 4) noexcept 
        : width(w), height(h), channels(ch), format(fmt) {
        resize_for_format();
        update_memory_usage();
    }
    
    /** @brief Create solid color texture */
    static TextureData create_solid_color(u32 width, u32 height, u8 r, u8 g, u8 b, u8 a = 255) noexcept;
    
    /** @brief Create checkerboard pattern (useful for debugging) */
    static TextureData create_checkerboard(u32 width, u32 height, u8 color1_r, u8 color1_g, u8 color1_b,
                                         u8 color2_r, u8 color2_g, u8 color2_b, u32 checker_size = 8) noexcept;
    
    /** @brief Create noise texture for testing */
    static TextureData create_noise(u32 width, u32 height, u32 seed = 12345) noexcept;
    
    /** @brief Create gradient texture */
    static TextureData create_gradient(u32 width, u32 height, u8 start_r, u8 start_g, u8 start_b,
                                     u8 end_r, u8 end_g, u8 end_b, bool horizontal = true) noexcept;
    
    //-------------------------------------------------------------------------
    // Data Manipulation
    //-------------------------------------------------------------------------
    
    /** @brief Resize texture data (reallocates pixel array) */
    void resize(u32 new_width, u32 new_height) noexcept;
    
    /** @brief Generate mipmap chain */
    void generate_mipmaps() noexcept;
    
    /** @brief Convert to different format */
    core::Result<void, const char*> convert_format(TextureFormat new_format) noexcept;
    
    /** @brief Apply gamma correction */
    void apply_gamma(f32 gamma) noexcept;
    
    /** @brief Flip texture vertically (useful for OpenGL coordinate system) */
    void flip_vertical() noexcept;
    
    /** @brief Flip texture horizontally */
    void flip_horizontal() noexcept;
    
    /** @brief Premultiply alpha channel */
    void premultiply_alpha() noexcept;
    
    //-------------------------------------------------------------------------
    // Pixel Access (for educational inspection)
    //-------------------------------------------------------------------------
    
    /** @brief Get pixel color at coordinates (RGBA) */
    struct Color {
        u8 r, g, b, a;
    };
    
    Color get_pixel(u32 x, u32 y) const noexcept;
    
    /** @brief Set pixel color at coordinates */
    void set_pixel(u32 x, u32 y, u8 r, u8 g, u8 b, u8 a = 255) noexcept;
    
    /** @brief Get pixel data pointer (raw access) */
    const u8* get_pixel_data() const noexcept { return pixel_data.data(); }
    u8* get_pixel_data() noexcept { return pixel_data.data(); }
    
    /** @brief Get mipmap level data */
    const u8* get_mipmap_data(u32 level) const noexcept {
        return (level < mipmap_data.size()) ? mipmap_data[level].data() : nullptr;
    }
    
    //-------------------------------------------------------------------------
    // Information and Validation
    //-------------------------------------------------------------------------
    
    /** @brief Get total memory usage including mipmaps */
    usize get_memory_usage() const noexcept { return memory_usage; }
    
    /** @brief Get number of mipmap levels */
    u32 get_mipmap_count() const noexcept { return static_cast<u32>(mipmap_data.size()); }
    
    /** @brief Check if texture data is valid */
    bool is_valid() const noexcept {
        return width > 0 && height > 0 && channels > 0 && channels <= 4 &&
               !pixel_data.empty() && pixel_data.size() >= calculate_required_size();
    }
    
    /** @brief Get texture aspect ratio */
    f32 get_aspect_ratio() const noexcept {
        return height > 0 ? static_cast<f32>(width) / height : 1.0f;
    }
    
    /** @brief Check if dimensions are power of 2 */
    bool is_power_of_two() const noexcept {
        return (width & (width - 1)) == 0 && (height & (height - 1)) == 0;
    }
    
    /** @brief Get detailed information for educational display */
    struct TextureInfo {
        u32 width, height, channels;
        const char* format_name;
        usize memory_mb;
        u32 mipmap_levels;
        bool has_transparency;
        bool is_compressed;
        f32 compression_ratio;
        bool power_of_two;
    };
    
    TextureInfo get_texture_info() const noexcept;
    
private:
    void resize_for_format() noexcept;
    void update_memory_usage() noexcept;
    usize calculate_required_size() const noexcept;
};

//=============================================================================
// GPU Texture Resource (OpenGL Implementation)
//=============================================================================

/**
 * @brief GPU Texture Resource with OpenGL Backend
 * 
 * Represents an actual texture resource on the GPU. Provides RAII management
 * of OpenGL texture objects with comprehensive educational features.
 * 
 * Educational Context:
 * This class demonstrates:
 * - OpenGL texture object lifecycle management
 * - GPU memory allocation and deallocation
 * - Texture binding and state management
 * - Performance monitoring and optimization
 */
class Texture {
public:
    //-------------------------------------------------------------------------
    // Construction and Destruction
    //-------------------------------------------------------------------------
    
    /** @brief Default constructor creates invalid texture */
    Texture() noexcept = default;
    
    /** @brief Create texture from texture data */
    explicit Texture(const TextureData& data, const TextureProperties& properties = TextureProperties{}) noexcept;
    
    /** @brief Create empty texture with specified properties */
    Texture(u32 width, u32 height, const TextureProperties& properties) noexcept;
    
    /** @brief Destructor automatically releases GPU resources */
    ~Texture() noexcept;
    
    // Move-only semantics (textures can't be copied, only moved)
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;
    
    //-------------------------------------------------------------------------
    // Resource Management
    //-------------------------------------------------------------------------
    
    /** @brief Check if texture is valid (has GPU resource) */
    bool is_valid() const noexcept { return gl_texture_id_ != 0; }
    
    /** @brief Get OpenGL texture ID */
    u32 get_gl_id() const noexcept { return gl_texture_id_; }
    
    /** @brief Get texture dimensions */
    u32 get_width() const noexcept { return width_; }
    u32 get_height() const noexcept { return height_; }
    
    /** @brief Get texture format */
    TextureFormat get_format() const noexcept { return properties_.format; }
    
    /** @brief Get texture properties */
    const TextureProperties& get_properties() const noexcept { return properties_; }
    
    /** @brief Get memory usage in bytes */
    usize get_memory_usage() const noexcept { return memory_usage_; }
    
    //-------------------------------------------------------------------------
    // GPU Operations
    //-------------------------------------------------------------------------
    
    /** @brief Bind texture to specified texture unit */
    void bind(u32 texture_unit = 0) const noexcept;
    
    /** @brief Unbind from current texture unit */
    void unbind() const noexcept;
    
    /** @brief Update texture data (for dynamic textures) */
    core::Result<void, const char*> update_data(const TextureData& new_data) noexcept;
    
    /** @brief Update partial texture data */
    core::Result<void, const char*> update_sub_data(u32 x, u32 y, u32 width, u32 height, 
                                                   const u8* pixel_data) noexcept;
    
    /** @brief Generate mipmaps */
    void generate_mipmaps() noexcept;
    
    /** @brief Update texture properties (sampling, wrapping) */
    void update_properties(const TextureProperties& new_properties) noexcept;
    
    //-------------------------------------------------------------------------
    // Performance Monitoring
    //-------------------------------------------------------------------------
    
    /** @brief Performance and usage statistics */
    struct TextureStats {
        u32 bind_count{0};                  ///< Number of times texture was bound
        u32 update_count{0};                ///< Number of data updates
        f32 total_bind_time{0.0f};          ///< Total time spent binding
        f32 total_update_time{0.0f};        ///< Total time spent updating
        u32 memory_allocations{0};          ///< GPU memory allocation count
        usize peak_memory_usage{0};         ///< Peak memory usage
        f32 cache_hit_ratio{1.0f};          ///< Texture cache hit ratio
        u64 last_access_frame{0};           ///< Frame when last accessed
    };
    
    /** @brief Get performance statistics */
    const TextureStats& get_stats() const noexcept { return stats_; }
    
    /** @brief Reset performance counters */
    void reset_stats() noexcept { stats_ = {}; }
    
    //-------------------------------------------------------------------------
    // Educational Features
    //-------------------------------------------------------------------------
    
    /** @brief Debug information for educational analysis */
    struct DebugInfo {
        std::string name;                   ///< Human-readable name
        const char* gl_format_name;         ///< OpenGL format name
        const char* gl_type_name;           ///< OpenGL data type name
        u32 gl_internal_format;             ///< OpenGL internal format
        u32 gl_format;                      ///< OpenGL format
        u32 gl_type;                        ///< OpenGL data type
        bool has_mipmaps;                   ///< Whether mipmaps are present
        u32 mipmap_levels;                  ///< Number of mipmap levels
        f32 memory_efficiency;              ///< Memory usage efficiency (0-1)
        const char* optimization_hints[8];  ///< Performance optimization suggestions
        u32 hint_count{0};
    };
    
    /** @brief Get debug information */
    DebugInfo get_debug_info() const noexcept;
    
    /** @brief Set debug name for GPU debugging tools */
    void set_debug_name(const std::string& name) noexcept;
    
    /** @brief Validate texture state */
    bool validate() const noexcept;
    
    //-------------------------------------------------------------------------
    // Utility Functions
    //-------------------------------------------------------------------------
    
    /** @brief Read texture data back from GPU (slow, for debugging) */
    core::Result<TextureData, const char*> read_back_data() const noexcept;
    
    /** @brief Calculate theoretical memory usage */
    static usize calculate_memory_usage(u32 width, u32 height, TextureFormat format, bool mipmaps) noexcept;
    
    /** @brief Check if format is supported by current GPU */
    static bool is_format_supported(TextureFormat format) noexcept;
    
    /** @brief Get maximum supported texture size */
    static u32 get_max_texture_size() noexcept;
    
    /** @brief Get maximum supported anisotropy */
    static f32 get_max_anisotropy() noexcept;
    
private:
    //-------------------------------------------------------------------------
    // Internal State
    //-------------------------------------------------------------------------
    
    u32 gl_texture_id_{0};                  ///< OpenGL texture object ID
    u32 width_{0};                          ///< Texture width
    u32 height_{0};                         ///< Texture height
    TextureProperties properties_{};        ///< Texture properties
    usize memory_usage_{0};                 ///< Current memory usage
    mutable TextureStats stats_{};          ///< Performance statistics
    std::string debug_name_{};              ///< Debug name
    
    //-------------------------------------------------------------------------
    // Internal Methods
    //-------------------------------------------------------------------------
    
    void create_gl_texture() noexcept;
    void destroy_gl_texture() noexcept;
    void upload_texture_data(const TextureData& data) noexcept;
    void apply_texture_parameters() noexcept;
    void update_memory_usage() noexcept;
    void record_bind() const noexcept;
    
    // OpenGL utility functions
    static u32 format_to_gl_internal_format(TextureFormat format) noexcept;
    static u32 format_to_gl_format(TextureFormat format) noexcept;
    static u32 format_to_gl_type(TextureFormat format) noexcept;
    static u32 filter_to_gl_filter(TextureFilter filter) noexcept;
    static u32 wrap_to_gl_wrap(TextureWrap wrap) noexcept;
};

//=============================================================================
// Texture Loading and File Format Support
//=============================================================================

/**
 * @brief Texture Loading System
 * 
 * Handles loading textures from various file formats with comprehensive
 * educational features and performance optimization.
 */
class TextureLoader {
public:
    //-------------------------------------------------------------------------
    // Supported File Formats
    //-------------------------------------------------------------------------
    
    enum class FileFormat : u8 {
        Unknown = 0,
        PNG,                ///< Portable Network Graphics (lossless, supports alpha)
        JPEG,               ///< Joint Photographic Experts Group (lossy, no alpha)
        BMP,                ///< Bitmap (uncompressed, simple format)
        TGA,                ///< Targa (uncompressed, supports alpha)
        DDS,                ///< DirectDraw Surface (compressed formats, mipmaps)
        KTX,                ///< Khronos Texture (OpenGL native format)
        HDR,                ///< High Dynamic Range (32-bit float)
        EXR                 ///< Extended Dynamic Range (advanced HDR)
    };
    
    //-------------------------------------------------------------------------
    // Loading Options
    //-------------------------------------------------------------------------
    
    struct LoadingOptions {
        bool flip_vertically{false};        ///< Flip texture for OpenGL coordinate system
        bool generate_mipmaps{true};         ///< Generate mipmap chain
        bool premultiply_alpha{false};       ///< Premultiply alpha channel
        f32 gamma_correction{1.0f};          ///< Apply gamma correction (1.0 = no correction)
        TextureFormat target_format{TextureFormat::RGBA8}; ///< Convert to this format
        bool enable_compression{false};      ///< Enable automatic compression
        f32 compression_quality{0.8f};       ///< Compression quality (0-1)
        
        // Educational features
        bool collect_statistics{true};       ///< Collect loading statistics
        bool validate_data{true};            ///< Validate loaded data
    };
    
    //-------------------------------------------------------------------------
    // Loading Results
    //-------------------------------------------------------------------------
    
    struct LoadingResult {
        TextureData data;                   ///< Loaded texture data
        FileFormat detected_format;         ///< Detected file format
        usize file_size_bytes;              ///< Original file size
        f32 loading_time_ms;                ///< Time spent loading
        std::string error_message;          ///< Error description (if failed)
        
        // Educational statistics
        struct {
            bool required_conversion;        ///< Whether format conversion was needed
            f32 compression_ratio;           ///< Original/final size ratio
            const char* optimization_hints[4]; ///< Performance suggestions
            u32 hint_count{0};
        } statistics;
    };
    
    //-------------------------------------------------------------------------
    // Loading Interface
    //-------------------------------------------------------------------------
    
    /** @brief Load texture from file */
    static core::Result<LoadingResult, const char*> load_from_file(
        const std::string& file_path, 
        const LoadingOptions& options = LoadingOptions{}) noexcept;
    
    /** @brief Load texture from memory buffer */
    static core::Result<LoadingResult, const char*> load_from_memory(
        const u8* data, 
        usize size, 
        FileFormat format,
        const LoadingOptions& options = LoadingOptions{}) noexcept;
    
    /** @brief Detect file format from extension */
    static FileFormat detect_format_from_extension(const std::string& file_path) noexcept;
    
    /** @brief Detect file format from file header */
    static FileFormat detect_format_from_header(const u8* data, usize size) noexcept;
    
    /** @brief Check if file format is supported */
    static bool is_format_supported(FileFormat format) noexcept;
    
    /** @brief Get supported file extensions */
    static std::vector<std::string> get_supported_extensions() noexcept;
    
    //-------------------------------------------------------------------------
    // Asynchronous Loading
    //-------------------------------------------------------------------------
    
    using LoadingCallback = std::function<void(core::Result<LoadingResult, const char*>)>;
    
    /** @brief Load texture asynchronously */
    static void load_async(const std::string& file_path,
                          const LoadingOptions& options,
                          LoadingCallback callback) noexcept;
    
    /** @brief Cancel pending async loads */
    static void cancel_async_loads() noexcept;
    
    /** @brief Get number of pending async loads */
    static u32 get_pending_load_count() noexcept;
    
    //-------------------------------------------------------------------------
    // Educational Features
    //-------------------------------------------------------------------------
    
    /** @brief Get detailed format information */
    struct FormatInfo {
        const char* name;
        const char* description;
        bool supports_alpha;
        bool supports_compression;
        bool supports_hdr;
        bool supports_mipmaps;
        const char* common_use_cases;
        f32 typical_compression_ratio;
    };
    
    static FormatInfo get_format_info(FileFormat format) noexcept;
    
    /** @brief Analyze texture file without loading full data */
    struct FileAnalysis {
        u32 width, height;
        u32 channels;
        FileFormat format;
        usize file_size;
        bool has_mipmaps;
        bool has_alpha;
        const char* recommendations[4];
        u32 recommendation_count;
    };
    
    static core::Result<FileAnalysis, const char*> analyze_file(const std::string& file_path) noexcept;

private:
    // Internal loading implementations for different formats
    static core::Result<TextureData, const char*> load_png(const std::string& path, const LoadingOptions& options) noexcept;
    static core::Result<TextureData, const char*> load_jpeg(const std::string& path, const LoadingOptions& options) noexcept;
    static core::Result<TextureData, const char*> load_bmp(const std::string& path, const LoadingOptions& options) noexcept;
    static core::Result<TextureData, const char*> load_tga(const std::string& path, const LoadingOptions& options) noexcept;
    static core::Result<TextureData, const char*> load_dds(const std::string& path, const LoadingOptions& options) noexcept;
    static core::Result<TextureData, const char*> load_hdr(const std::string& path, const LoadingOptions& options) noexcept;
};

//=============================================================================
// Texture Manager - Central Resource Management
//=============================================================================

/**
 * @brief Centralized Texture Resource Manager
 * 
 * Manages all texture resources in the system with caching, memory management,
 * and comprehensive educational features. Provides the main interface for
 * texture creation, loading, and lifetime management.
 * 
 * Educational Context:
 * This manager demonstrates:
 * - Resource pooling and caching strategies
 * - Memory management for GPU resources
 * - Asynchronous resource loading and streaming
 * - Performance monitoring and optimization
 * - Debug tools and educational visualization
 */
class TextureManager {
public:
    //-------------------------------------------------------------------------
    // Manager Configuration
    //-------------------------------------------------------------------------
    
    struct Config {
        usize max_memory_mb{512};           ///< Maximum texture memory (MB)
        u32 max_texture_count{1000};        ///< Maximum number of textures
        bool enable_compression{true};       ///< Enable automatic compression
        f32 default_compression_quality{0.8f}; ///< Default compression quality
        bool enable_mipmaps{true};           ///< Generate mipmaps by default
        bool enable_async_loading{true};     ///< Enable background loading
        u32 async_thread_count{2};          ///< Number of loading threads
        
        // Cache settings
        bool enable_texture_cache{true};     ///< Enable texture caching
        usize cache_size_mb{128};           ///< Cache size limit
        f32 cache_eviction_ratio{0.25f};    ///< Portion to evict when full
        
        // Educational settings
        bool collect_statistics{true};       ///< Collect usage statistics
        bool enable_hot_reload{false};       ///< Enable file change detection
        bool validate_all_operations{false}; ///< Validate all operations (slow)
    };
    
    //-------------------------------------------------------------------------
    // Construction and Initialization
    //-------------------------------------------------------------------------
    
    /** @brief Create texture manager with configuration */
    explicit TextureManager(const Config& config = Config{}) noexcept;
    
    /** @brief Destructor cleans up all resources */
    ~TextureManager() noexcept;
    
    // Non-copyable, moveable
    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;
    TextureManager(TextureManager&&) noexcept = default;
    TextureManager& operator=(TextureManager&&) noexcept = default;
    
    //-------------------------------------------------------------------------
    // Texture Creation and Loading
    //-------------------------------------------------------------------------
    
    /** @brief Create texture from texture data */
    TextureID create_texture(const TextureData& data, const TextureProperties& properties = TextureProperties{}) noexcept;
    
    /** @brief Create empty texture */
    TextureID create_empty_texture(u32 width, u32 height, const TextureProperties& properties = TextureProperties{}) noexcept;
    
    /** @brief Load texture from file */
    core::Result<TextureID, const char*> load_texture(const std::string& file_path, 
                                                     const TextureLoader::LoadingOptions& options = {}) noexcept;
    
    /** @brief Load texture asynchronously */
    void load_texture_async(const std::string& file_path,
                          const TextureLoader::LoadingOptions& options,
                          std::function<void(core::Result<TextureID, const char*>)> callback) noexcept;
    
    /** @brief Create default system textures */
    void create_default_textures() noexcept;
    
    //-------------------------------------------------------------------------
    // Texture Access and Management
    //-------------------------------------------------------------------------
    
    /** @brief Get texture by ID */
    Texture* get_texture(TextureID id) noexcept;
    const Texture* get_texture(TextureID id) const noexcept;
    
    /** @brief Check if texture exists */
    bool has_texture(TextureID id) const noexcept;
    
    /** @brief Remove texture and free resources */
    void remove_texture(TextureID id) noexcept;
    
    /** @brief Remove all textures */
    void clear_all_textures() noexcept;
    
    /** @brief Get texture ID by name (if named) */
    TextureID find_texture(const std::string& name) const noexcept;
    
    /** @brief Set texture debug name */
    void set_texture_name(TextureID id, const std::string& name) noexcept;
    
    //-------------------------------------------------------------------------
    // Memory Management
    //-------------------------------------------------------------------------
    
    /** @brief Get current memory usage */
    usize get_memory_usage() const noexcept { return current_memory_usage_; }
    
    /** @brief Get maximum memory usage */
    usize get_max_memory() const noexcept { return config_.max_memory_mb * 1024 * 1024; }
    
    /** @brief Get memory usage percentage */
    f32 get_memory_usage_percent() const noexcept {
        return static_cast<f32>(current_memory_usage_) / get_max_memory() * 100.0f;
    }
    
    /** @brief Force garbage collection */
    usize garbage_collect() noexcept;
    
    /** @brief Compress textures to save memory */
    usize compress_textures(f32 quality = 0.8f) noexcept;
    
    /** @brief Defragment memory (reorganize allocations) */
    void defragment_memory() noexcept;
    
    //-------------------------------------------------------------------------
    // Performance Monitoring and Statistics
    //-------------------------------------------------------------------------
    
    /** @brief Comprehensive manager statistics */
    struct Statistics {
        // Resource counts
        u32 total_textures{0};
        u32 loaded_textures{0};
        u32 compressed_textures{0};
        u32 mipmapped_textures{0};
        
        // Memory statistics
        usize total_memory_bytes{0};
        usize compressed_memory_bytes{0};
        usize cache_memory_bytes{0};
        f32 memory_fragmentation{0.0f};
        f32 average_compression_ratio{1.0f};
        
        // Performance metrics
        u32 cache_hits{0};
        u32 cache_misses{0};
        f32 cache_hit_ratio{0.0f};
        u32 async_loads_completed{0};
        u32 async_loads_pending{0};
        f32 average_load_time_ms{0.0f};
        
        // Educational insights
        const char* memory_health{"Good"};      // "Excellent", "Good", "Fair", "Poor"
        const char* performance_rating{"A"};    // Letter grade A-F
        const char* optimization_suggestions[8];
        u32 suggestion_count{0};
        
        void add_suggestion(const char* suggestion) noexcept {
            if (suggestion_count < 8) {
                optimization_suggestions[suggestion_count++] = suggestion;
            }
        }
    };
    
    /** @brief Get comprehensive statistics */
    Statistics get_statistics() const noexcept;
    
    /** @brief Reset performance counters */
    void reset_statistics() noexcept;
    
    /** @brief Update statistics (call once per frame) */
    void update_statistics() noexcept;
    
    //-------------------------------------------------------------------------
    // Educational and Debug Features
    //-------------------------------------------------------------------------
    
    /** @brief Get list of all texture IDs */
    std::vector<TextureID> get_all_texture_ids() const noexcept;
    
    /** @brief Get texture information for UI display */
    struct TextureDisplayInfo {
        TextureID id;
        std::string name;
        u32 width, height;
        const char* format_name;
        usize memory_kb;
        u32 bind_count;
        bool is_compressed;
        bool has_mipmaps;
        f32 usage_frequency; // How often texture is used
    };
    
    std::vector<TextureDisplayInfo> get_texture_list() const noexcept;
    
    /** @brief Generate memory usage report */
    std::string generate_memory_report() const noexcept;
    
    /** @brief Generate performance report */
    std::string generate_performance_report() const noexcept;
    
    /** @brief Validate all textures */
    bool validate_all_textures() const noexcept;
    
    /** @brief Enable/disable hot reloading */
    void set_hot_reload_enabled(bool enabled) noexcept;
    
    /** @brief Check for file changes and reload if needed */
    void update_hot_reload() noexcept;
    
    //-------------------------------------------------------------------------
    // System Integration
    //-------------------------------------------------------------------------
    
    /** @brief Update manager state (call once per frame) */
    void update() noexcept;
    
    /** @brief Handle GPU context lost/restored */
    void handle_context_lost() noexcept;
    void handle_context_restored() noexcept;
    
    /** @brief Optimize for low memory conditions */
    void optimize_for_low_memory() noexcept;
    
    /** @brief Preload commonly used textures */
    void preload_common_textures() noexcept;

private:
    //-------------------------------------------------------------------------
    // Internal Data Structures
    //-------------------------------------------------------------------------
    
    struct TextureEntry {
        std::unique_ptr<Texture> texture;
        std::string name;
        std::string file_path;
        u64 last_access_time;
        u32 reference_count;
        bool is_system_texture;
        
        TextureEntry() = default;
        TextureEntry(std::unique_ptr<Texture> tex, std::string n = {}) 
            : texture(std::move(tex)), name(std::move(n)), last_access_time(0), 
              reference_count(1), is_system_texture(false) {}
    };
    
    Config config_;
    std::unordered_map<TextureID, TextureEntry> textures_;
    std::unordered_map<std::string, TextureID> name_to_id_;
    
    TextureID next_texture_id_{4}; // Start after system textures
    usize current_memory_usage_{0};
    mutable Statistics cached_stats_{};
    bool stats_dirty_{true};
    
    // Hot reload system
    std::unordered_map<std::string, u64> file_timestamps_;
    
    //-------------------------------------------------------------------------
    // Internal Methods
    //-------------------------------------------------------------------------
    
    TextureID generate_texture_id() noexcept { return next_texture_id_++; }
    void update_memory_usage() noexcept;
    void evict_textures_if_needed() noexcept;
    void update_cached_statistics() const noexcept;
    bool should_compress_texture(const TextureData& data) const noexcept;
    void create_system_texture(TextureID id, const TextureData& data, const std::string& name) noexcept;
};

//=============================================================================
// Utility Functions and Integration Helpers
//=============================================================================

namespace utils {
    /** @brief Convert TextureID to handle for components */
    inline components::TextureHandle texture_id_to_handle(TextureID id, u16 width = 0, u16 height = 0) noexcept {
        return {id, width, height};
    }
    
    /** @brief Convert handle back to TextureID */
    inline TextureID handle_to_texture_id(const components::TextureHandle& handle) noexcept {
        return handle.id;
    }
    
    /** @brief Calculate mipmap level count for dimensions */
    inline u32 calculate_mipmap_levels(u32 width, u32 height) noexcept {
        return static_cast<u32>(std::floor(std::log2(std::max(width, height)))) + 1;
    }
    
    /** @brief Check if dimensions are valid for textures */
    inline bool are_dimensions_valid(u32 width, u32 height) noexcept {
        return width > 0 && height > 0 && width <= 16384 && height <= 16384; // Common max sizes
    }
    
    /** @brief Get next power of 2 */
    inline u32 next_power_of_two(u32 value) noexcept {
        if (value == 0) return 1;
        --value;
        value |= value >> 1;
        value |= value >> 2;
        value |= value >> 4;
        value |= value >> 8;
        value |= value >> 16;
        return ++value;
    }
    
    /** @brief Estimate memory usage for texture configuration */
    inline usize estimate_memory_usage(u32 width, u32 height, TextureFormat format, bool mipmaps) noexcept {
        return Texture::calculate_memory_usage(width, height, format, mipmaps);
    }
}

} // namespace ecscope::renderer::resources