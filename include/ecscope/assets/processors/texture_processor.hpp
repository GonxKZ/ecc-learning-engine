#pragma once

#include "asset_processor.hpp"
#include <array>

namespace ecscope::assets::processors {

    // Texture formats
    enum class TextureFormat {
        UNKNOWN = 0,
        R8,
        RG8,
        RGB8,
        RGBA8,
        R16F,
        RG16F,
        RGB16F,
        RGBA16F,
        R32F,
        RG32F,
        RGB32F,
        RGBA32F,
        BC1,    // DXT1
        BC3,    // DXT5
        BC4,    // ATI1
        BC5,    // ATI2
        BC6H,   // BPTC float
        BC7,    // BPTC
        ETC2_RGB,
        ETC2_RGBA,
        ASTC_4x4,
        ASTC_8x8,
        COUNT
    };

    // Texture compression settings
    struct TextureCompressionSettings {
        TextureFormat target_format = TextureFormat::BC7;
        int quality = 95;  // 0-100
        bool use_alpha = true;
        bool generate_mipmaps = true;
        int max_mipmap_levels = -1; // -1 = all levels
        bool use_srgb = true;
        
        // Advanced settings
        float alpha_threshold = 0.5f;
        bool use_perceptual_metrics = true;
        int compression_threads = 0; // 0 = auto
    };

    // Texture resize settings
    struct TextureResizeSettings {
        enum class Filter {
            POINT,
            LINEAR,
            CUBIC,
            LANCZOS
        };
        
        int max_width = 2048;
        int max_height = 2048;
        bool maintain_aspect_ratio = true;
        bool power_of_two = false;
        Filter filter = Filter::LANCZOS;
        bool resize_only_if_larger = true;
    };

    // Texture metadata
    struct TextureMetadata {
        int width = 0;
        int height = 0;
        int channels = 0;
        int bit_depth = 8;
        TextureFormat format = TextureFormat::UNKNOWN;
        bool has_alpha = false;
        bool is_srgb = false;
        int mipmap_levels = 1;
        size_t compressed_size = 0;
        
        // Color analysis
        std::array<float, 4> average_color = {0, 0, 0, 0};
        std::array<float, 4> dominant_color = {0, 0, 0, 0};
        float brightness = 0.0f;
        float contrast = 0.0f;
    };

    // Texture processor
    class TextureProcessor : public BaseAssetProcessor {
    public:
        TextureProcessor();
        
        // AssetProcessor implementation
        std::vector<std::string> get_supported_extensions() const override;
        bool can_process(const std::string& file_path, const AssetMetadata& metadata) const override;
        bool supports_streaming() const override { return true; }
        
        ProcessingResult process(const std::vector<uint8_t>& input_data,
                               const AssetMetadata& input_metadata,
                               const ProcessingOptions& options) override;
        
        bool validate_input(const std::vector<uint8_t>& input_data,
                           const AssetMetadata& metadata) const override;
        
        AssetMetadata extract_metadata(const std::vector<uint8_t>& data,
                                     const std::string& file_path) const override;
        
        size_t estimate_output_size(size_t input_size,
                                   const ProcessingOptions& options) const override;
        
        // Texture-specific operations
        ProcessingResult load_texture(const std::vector<uint8_t>& data,
                                    const std::string& file_path) const;
        
        ProcessingResult compress_texture(const std::vector<uint8_t>& raw_data,
                                        const TextureMetadata& metadata,
                                        const TextureCompressionSettings& settings) const;
        
        ProcessingResult resize_texture(const std::vector<uint8_t>& raw_data,
                                      const TextureMetadata& metadata,
                                      const TextureResizeSettings& settings) const;
        
        ProcessingResult generate_mipmaps(const std::vector<uint8_t>& raw_data,
                                        const TextureMetadata& metadata) const;
        
        ProcessingResult convert_format(const std::vector<uint8_t>& raw_data,
                                      const TextureMetadata& metadata,
                                      TextureFormat target_format) const;
        
        // Texture analysis
        TextureMetadata analyze_texture(const std::vector<uint8_t>& raw_data,
                                      int width, int height, int channels) const;
        
        bool is_normal_map(const std::vector<uint8_t>& raw_data,
                          int width, int height, int channels) const;
        
        bool has_transparency(const std::vector<uint8_t>& raw_data,
                             int width, int height, int channels) const;
        
        // Format utilities
        static const char* format_to_string(TextureFormat format);
        static TextureFormat string_to_format(const std::string& format_str);
        static bool is_compressed_format(TextureFormat format);
        static bool supports_alpha(TextureFormat format);
        static size_t get_format_block_size(TextureFormat format);
        static size_t calculate_texture_size(int width, int height, TextureFormat format, int mip_levels = 1);
        
        // Compression utilities
        TextureFormat select_optimal_format(const TextureMetadata& metadata,
                                           const ProcessingOptions& options) const;
        
        int calculate_optimal_mipmap_levels(int width, int height) const;
        
    private:
        // Internal processing methods
        ProcessingResult load_png(const std::vector<uint8_t>& data) const;
        ProcessingResult load_jpg(const std::vector<uint8_t>& data) const;
        ProcessingResult load_tga(const std::vector<uint8_t>& data) const;
        ProcessingResult load_bmp(const std::vector<uint8_t>& data) const;
        ProcessingResult load_hdr(const std::vector<uint8_t>& data) const;
        ProcessingResult load_dds(const std::vector<uint8_t>& data) const;
        ProcessingResult load_ktx(const std::vector<uint8_t>& data) const;
        
        // Compression implementations
        std::vector<uint8_t> compress_bc1(const std::vector<uint8_t>& rgba_data,
                                         int width, int height, int quality) const;
        std::vector<uint8_t> compress_bc3(const std::vector<uint8_t>& rgba_data,
                                         int width, int height, int quality) const;
        std::vector<uint8_t> compress_bc7(const std::vector<uint8_t>& rgba_data,
                                         int width, int height, int quality) const;
        std::vector<uint8_t> compress_etc2(const std::vector<uint8_t>& rgba_data,
                                          int width, int height, bool alpha) const;
        std::vector<uint8_t> compress_astc(const std::vector<uint8_t>& rgba_data,
                                          int width, int height, int block_size) const;
        
        // Image manipulation
        std::vector<uint8_t> resize_image(const std::vector<uint8_t>& input,
                                         int src_width, int src_height,
                                         int dst_width, int dst_height,
                                         int channels,
                                         TextureResizeSettings::Filter filter) const;
        
        std::vector<uint8_t> generate_mipmap_level(const std::vector<uint8_t>& input,
                                                  int width, int height, int channels) const;
        
        std::vector<uint8_t> convert_to_normal_map(const std::vector<uint8_t>& height_map,
                                                  int width, int height, float strength = 1.0f) const;
        
        // Color space conversions
        std::vector<uint8_t> convert_srgb_to_linear(const std::vector<uint8_t>& srgb_data) const;
        std::vector<uint8_t> convert_linear_to_srgb(const std::vector<uint8_t>& linear_data) const;
        
        // Validation helpers
        bool is_valid_image_size(int width, int height) const;
        bool is_supported_format(const std::string& file_path) const;
        
        // Threading support
        class CompressionThreadPool;
        std::unique_ptr<CompressionThreadPool> m_thread_pool;
    };

    // DDS (DirectDraw Surface) utilities
    namespace dds {
        struct DDSHeader {
            uint32_t magic;
            uint32_t size;
            uint32_t flags;
            uint32_t height;
            uint32_t width;
            uint32_t pitch_or_linear_size;
            uint32_t depth;
            uint32_t mip_map_count;
            uint32_t reserved1[11];
            struct {
                uint32_t size;
                uint32_t flags;
                uint32_t four_cc;
                uint32_t rgb_bit_count;
                uint32_t r_bit_mask;
                uint32_t g_bit_mask;
                uint32_t b_bit_mask;
                uint32_t a_bit_mask;
            } pixel_format;
            uint32_t caps;
            uint32_t caps2;
            uint32_t caps3;
            uint32_t caps4;
            uint32_t reserved2;
        };
        
        bool parse_dds_header(const std::vector<uint8_t>& data, DDSHeader& header);
        TextureFormat dds_format_to_texture_format(uint32_t four_cc, const DDSHeader& header);
        std::vector<uint8_t> create_dds_file(const std::vector<uint8_t>& texture_data,
                                            int width, int height, TextureFormat format,
                                            int mip_levels = 1);
    }

    // KTX (Khronos Texture) utilities
    namespace ktx {
        struct KTXHeader {
            uint8_t identifier[12];
            uint32_t endianness;
            uint32_t gl_type;
            uint32_t gl_type_size;
            uint32_t gl_format;
            uint32_t gl_internal_format;
            uint32_t gl_base_internal_format;
            uint32_t pixel_width;
            uint32_t pixel_height;
            uint32_t pixel_depth;
            uint32_t number_of_array_elements;
            uint32_t number_of_faces;
            uint32_t number_of_mipmap_levels;
            uint32_t bytes_of_key_value_data;
        };
        
        bool parse_ktx_header(const std::vector<uint8_t>& data, KTXHeader& header);
        TextureFormat gl_format_to_texture_format(uint32_t gl_internal_format);
        std::vector<uint8_t> create_ktx_file(const std::vector<uint8_t>& texture_data,
                                            int width, int height, TextureFormat format,
                                            int mip_levels = 1);
    }

    // Texture streaming support
    class StreamingTextureProcessor {
    public:
        StreamingTextureProcessor();
        ~StreamingTextureProcessor();
        
        // Streaming operations
        ProcessingResult start_streaming_load(const std::vector<uint8_t>& data,
                                             const std::string& file_path);
        
        ProcessingResult continue_streaming_load(float target_progress = 1.0f);
        bool is_streaming_complete() const;
        float get_streaming_progress() const;
        
        // Quality-based streaming
        ProcessingResult load_quality_level(QualityLevel quality);
        QualityLevel get_current_quality_level() const;
        
        void cancel_streaming();
        void reset();
        
    private:
        struct StreamingState;
        std::unique_ptr<StreamingState> m_state;
    };

} // namespace ecscope::assets::processors