#pragma once

#include "../core/asset_types.hpp"
#include <vector>
#include <memory>

namespace ecscope::assets {

// =============================================================================
// Texture Formats and Properties
// =============================================================================

enum class TextureFormat : uint32_t {
    Unknown = 0,
    
    // Uncompressed formats
    R8,                 // 8-bit red
    RG8,                // 8-bit red-green
    RGB8,               // 8-bit RGB
    RGBA8,              // 8-bit RGBA
    R16F,               // 16-bit float red
    RG16F,              // 16-bit float red-green
    RGB16F,             // 16-bit float RGB
    RGBA16F,            // 16-bit float RGBA
    R32F,               // 32-bit float red
    RG32F,              // 32-bit float red-green
    RGB32F,             // 32-bit float RGB
    RGBA32F,            // 32-bit float RGBA
    
    // Compressed formats
    DXT1,               // BC1 - RGB with 1-bit alpha
    DXT3,               // BC2 - RGBA with explicit alpha
    DXT5,               // BC3 - RGBA with interpolated alpha
    BC4,                // Single channel compression
    BC5,                // Two channel compression
    BC6H,               // HDR compression
    BC7,                // High quality RGBA compression
    
    // Mobile compressed formats
    ETC2_RGB,           // ETC2 RGB
    ETC2_RGBA,          // ETC2 RGBA
    ASTC_4x4,           // ASTC 4x4 block
    ASTC_8x8,           // ASTC 8x8 block
    
    // Depth formats
    Depth16,            // 16-bit depth
    Depth24,            // 24-bit depth
    Depth32F,           // 32-bit float depth
    Depth24Stencil8     // 24-bit depth + 8-bit stencil
};

enum class TextureType : uint8_t {
    Texture1D,
    Texture2D,
    Texture3D,
    TextureCube,
    Texture1DArray,
    Texture2DArray,
    TextureCubeArray
};

enum class TextureWrap : uint8_t {
    Repeat,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder
};

enum class TextureFilter : uint8_t {
    Nearest,
    Linear,
    NearestMipmapNearest,
    LinearMipmapNearest,
    NearestMipmapLinear,
    LinearMipmapLinear
};

// =============================================================================
// Texture Data
// =============================================================================

struct TextureData {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 1;
    uint32_t mip_levels = 1;
    uint32_t array_layers = 1;
    TextureFormat format = TextureFormat::Unknown;
    TextureType type = TextureType::Texture2D;
    
    std::vector<uint8_t> data;
    std::vector<size_t> mip_offsets;
    
    // Sampling parameters
    TextureWrap wrap_u = TextureWrap::Repeat;
    TextureWrap wrap_v = TextureWrap::Repeat;
    TextureWrap wrap_w = TextureWrap::Repeat;
    TextureFilter min_filter = TextureFilter::Linear;
    TextureFilter mag_filter = TextureFilter::Linear;
    
    // HDR and tone mapping
    bool is_hdr = false;
    float exposure = 1.0f;
    float gamma = 2.2f;
    
    size_t getPixelSize() const;
    size_t getMipSize(uint32_t mip_level) const;
    size_t getTotalSize() const;
    
    bool isCompressed() const;
    bool hasAlpha() const;
    bool isDepth() const;
};

// =============================================================================
// Texture Processing Options
// =============================================================================

struct TextureProcessingOptions {
    // Mipmap generation
    bool generate_mipmaps = true;
    uint32_t max_mip_levels = 0; // 0 = generate all possible levels
    TextureFilter mip_filter = TextureFilter::Linear;
    
    // Compression
    bool compress = true;
    TextureFormat target_format = TextureFormat::Unknown; // Auto-detect
    float compression_quality = 0.8f; // 0.0 = fastest, 1.0 = best quality
    
    // Resizing
    bool resize = false;
    uint32_t max_width = 0;
    uint32_t max_height = 0;
    bool maintain_aspect_ratio = true;
    
    // Format conversion
    bool convert_to_linear = false;
    bool premultiply_alpha = false;
    bool flip_vertically = false;
    
    // Quality levels
    AssetQuality target_quality = AssetQuality::High;
    
    // Platform-specific options
    bool generate_platform_variants = true;
    std::vector<TextureFormat> preferred_formats;
};

// =============================================================================
// Texture Asset
// =============================================================================

class TextureAsset : public Asset {
public:
    ASSET_TYPE(TextureAsset, 1001)
    
    TextureAsset();
    ~TextureAsset() override;
    
    // Asset interface
    AssetLoadResult load(const std::string& path, const AssetLoadParams& params = {}) override;
    void unload() override;
    bool isLoaded() const override { return texture_data_ != nullptr; }
    
    uint64_t getMemoryUsage() const override;
    
    // Streaming support
    bool supportsStreaming() const override { return true; }
    void streamIn(AssetQuality quality) override;
    void streamOut() override;
    
    // Texture data access
    const TextureData* getTextureData() const { return texture_data_.get(); }
    TextureData* getTextureData() { return texture_data_.get(); }
    
    // Texture properties
    uint32_t getWidth() const { return texture_data_ ? texture_data_->width : 0; }
    uint32_t getHeight() const { return texture_data_ ? texture_data_->height : 0; }
    TextureFormat getFormat() const { return texture_data_ ? texture_data_->format : TextureFormat::Unknown; }
    
    // GPU resource management (platform-specific)
    uint32_t getGPUHandle() const { return gpu_handle_; }
    void setGPUHandle(uint32_t handle) { gpu_handle_ = handle; }
    
private:
    std::unique_ptr<TextureData> texture_data_;
    uint32_t gpu_handle_ = 0;
    AssetQuality current_quality_ = AssetQuality::High;
};

// =============================================================================
// Texture Processor
// =============================================================================

class TextureProcessor {
public:
    TextureProcessor();
    ~TextureProcessor();
    
    // Main processing interface
    std::unique_ptr<TextureData> processTexture(const std::string& input_path, 
                                               const TextureProcessingOptions& options = {});
    
    std::unique_ptr<TextureData> processTextureFromMemory(const uint8_t* data, size_t size,
                                                         const TextureProcessingOptions& options = {});
    
    // Individual processing steps
    std::unique_ptr<TextureData> loadFromFile(const std::string& path);
    std::unique_ptr<TextureData> loadFromMemory(const uint8_t* data, size_t size);
    
    bool generateMipmaps(TextureData& texture, TextureFilter filter = TextureFilter::Linear);
    bool resizeTexture(TextureData& texture, uint32_t new_width, uint32_t new_height);
    bool convertFormat(TextureData& texture, TextureFormat new_format);
    bool compressTexture(TextureData& texture, TextureFormat compression_format, float quality = 0.8f);
    
    // Format utilities
    static bool isFormatSupported(TextureFormat format);
    static std::vector<TextureFormat> getSupportedFormats();
    static TextureFormat getBestCompressionFormat(const TextureData& texture);
    static size_t getFormatPixelSize(TextureFormat format);
    static bool isFormatCompressed(TextureFormat format);
    
    // Quality level processing
    std::unique_ptr<TextureData> processForQuality(const TextureData& source, AssetQuality quality);
    
    // Platform-specific optimization
    std::vector<std::unique_ptr<TextureData>> generatePlatformVariants(const TextureData& source);
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// =============================================================================
// Texture Compression Utilities
// =============================================================================

class TextureCompressor {
public:
    virtual ~TextureCompressor() = default;
    
    virtual bool canCompress(TextureFormat input_format, TextureFormat output_format) const = 0;
    virtual bool compress(const TextureData& input, TextureData& output, float quality = 0.8f) = 0;
    virtual TextureFormat getOutputFormat() const = 0;
    virtual std::string getName() const = 0;
};

class BC1Compressor : public TextureCompressor {
public:
    bool canCompress(TextureFormat input_format, TextureFormat output_format) const override;
    bool compress(const TextureData& input, TextureData& output, float quality = 0.8f) override;
    TextureFormat getOutputFormat() const override { return TextureFormat::DXT1; }
    std::string getName() const override { return "BC1/DXT1"; }
};

class BC3Compressor : public TextureCompressor {
public:
    bool canCompress(TextureFormat input_format, TextureFormat output_format) const override;
    bool compress(const TextureData& input, TextureData& output, float quality = 0.8f) override;
    TextureFormat getOutputFormat() const override { return TextureFormat::DXT5; }
    std::string getName() const override { return "BC3/DXT5"; }
};

class BC7Compressor : public TextureCompressor {
public:
    bool canCompress(TextureFormat input_format, TextureFormat output_format) const override;
    bool compress(const TextureData& input, TextureData& output, float quality = 0.8f) override;
    TextureFormat getOutputFormat() const override { return TextureFormat::BC7; }
    std::string getName() const override { return "BC7"; }
};

// =============================================================================
// Texture Registry
// =============================================================================

class TextureRegistry {
public:
    static TextureRegistry& instance();
    
    void registerCompressor(std::unique_ptr<TextureCompressor> compressor);
    TextureCompressor* getCompressor(TextureFormat output_format) const;
    std::vector<TextureCompressor*> getCompressors() const;
    
    // Platform detection
    std::vector<TextureFormat> getSupportedFormats() const;
    bool supportsFormat(TextureFormat format) const;
    
private:
    std::vector<std::unique_ptr<TextureCompressor>> compressors_;
    std::unordered_map<TextureFormat, TextureCompressor*> format_to_compressor_;
};

} // namespace ecscope::assets