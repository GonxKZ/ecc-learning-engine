#pragma once

/**
 * @file texture_importer.hpp
 * @brief Advanced Texture Import System for ECScope Asset Pipeline
 * 
 * This system provides comprehensive texture import capabilities with educational
 * features, performance optimization, and integration with the asset pipeline.
 * 
 * Key Features:
 * - Multi-format support (PNG, JPG, TGA, BMP, DDS, KTX, HDR, EXR)
 * - Advanced import settings and preprocessing
 * - Educational analysis and optimization suggestions
 * - Memory-efficient processing with streaming support
 * - Integration with existing texture management system
 * - Real-time preview and validation
 * 
 * Educational Value:
 * - Demonstrates texture format trade-offs
 * - Shows compression techniques and quality impact
 * - Illustrates memory usage optimization strategies
 * - Provides performance analysis tools
 * - Teaches GPU texture format compatibility
 * 
 * @author ECScope Educational ECS Framework - Asset Pipeline
 * @date 2025
 */

#include "asset_pipeline.hpp"
#include "texture.hpp"
#include <vector>
#include <memory>
#include <functional>

namespace ecscope::assets::importers {

//=============================================================================
// Texture Analysis Results
//=============================================================================

/**
 * @brief Comprehensive texture analysis for educational purposes
 */
struct TextureAnalysis {
    // Basic properties
    u32 width{0};
    u32 height{0};
    u32 channels{0};
    usize file_size_bytes{0};
    std::string format_name;
    
    // Content analysis
    struct ColorAnalysis {
        bool has_alpha{false};
        bool has_transparency{false};
        bool is_grayscale{false};
        bool is_hdr{false};
        
        // Color statistics
        struct ChannelStats {
            f32 min_value{1.0f};
            f32 max_value{0.0f};
            f32 average_value{0.5f};
            f32 variance{0.0f};
        } red, green, blue, alpha;
        
        f32 dynamic_range{1.0f};
        f32 contrast_ratio{1.0f};
        
    } color_analysis;
    
    // Performance characteristics
    struct PerformanceInfo {
        bool power_of_two{false};
        bool suitable_for_compression{false};
        f32 estimated_compression_ratio{1.0f};
        usize gpu_memory_estimate_bytes{0};
        f32 upload_time_estimate_ms{0.0f};
        
        // Optimization suggestions
        std::vector<std::string> suggestions;
        
    } performance;
    
    // Educational insights
    struct EducationalInfo {
        std::string complexity_assessment; // "Simple", "Moderate", "Complex"
        std::vector<std::string> learning_points;
        std::string recommended_format;
        std::string use_case_suggestions;
        f32 educational_value_score{0.5f}; // 0.0 = low, 1.0 = high educational value
        
    } educational;
    
    // Quality metrics
    f32 overall_quality_score{1.0f};
    std::string quality_assessment;
    std::vector<std::string> quality_issues;
};

//=============================================================================
// Texture Format Support Detection
//=============================================================================

/**
 * @brief Runtime detection of supported texture formats
 */
class TextureFormatSupport {
private:
    static std::unordered_map<std::string, bool> format_support_cache_;
    static bool cache_initialized_;
    
public:
    static void initialize();
    static bool is_format_supported(const std::string& extension);
    static std::vector<std::string> get_supported_extensions();
    static std::vector<std::string> get_read_formats();
    static std::vector<std::string> get_write_formats();
    
    // Educational information about formats
    struct FormatInfo {
        std::string name;
        std::string description;
        bool supports_alpha;
        bool supports_compression;
        bool supports_hdr;
        bool lossless;
        std::vector<std::string> common_uses;
        f32 typical_compression_ratio;
    };
    
    static FormatInfo get_format_info(const std::string& extension);
    static std::string get_format_comparison_table();
};

//=============================================================================
// Advanced Texture Processing
//=============================================================================

/**
 * @brief Advanced texture processing operations
 */
class TextureProcessor {
public:
    // Resize operations
    enum class ResizeFilter {
        NearestNeighbor,  // Fast, pixelated
        Bilinear,         // Good quality/speed balance
        Bicubic,          // High quality, slower
        Lanczos,          // Highest quality, slowest
        Mitchell          // Good for downscaling
    };
    
    static bool resize_texture(
        renderer::resources::TextureData& texture,
        u32 new_width,
        u32 new_height,
        ResizeFilter filter = ResizeFilter::Bilinear
    );
    
    // Format conversion
    static bool convert_format(
        renderer::resources::TextureData& texture,
        renderer::resources::TextureFormat target_format
    );
    
    // Power-of-two operations
    static std::pair<u32, u32> calculate_pot_dimensions(u32 width, u32 height);
    static bool resize_to_power_of_two(
        renderer::resources::TextureData& texture,
        ResizeFilter filter = ResizeFilter::Bilinear
    );
    
    // Compression
    enum class CompressionFormat {
        DXT1_RGB,        // RGB, 4:1 compression
        DXT3_RGBA,       // RGBA with explicit alpha
        DXT5_RGBA,       // RGBA with interpolated alpha
        BC7_RGBA,        // High-quality RGBA compression
        ETC2_RGB,        // Mobile RGB compression
        ETC2_RGBA,       // Mobile RGBA compression
        ASTC_4x4,        // Adaptive compression, high quality
        ASTC_8x8         // Adaptive compression, high compression
    };
    
    static bool compress_texture(
        renderer::resources::TextureData& texture,
        CompressionFormat format,
        f32 quality = 0.8f
    );
    
    // Quality enhancement
    static bool sharpen_texture(renderer::resources::TextureData& texture, f32 amount = 1.0f);
    static bool apply_gamma_correction(renderer::resources::TextureData& texture, f32 gamma = 2.2f);
    static bool normalize_texture(renderer::resources::TextureData& texture);
    
    // Educational processing with step-by-step visualization
    struct ProcessingStep {
        std::string name;
        std::string description;
        f64 processing_time_ms;
        usize memory_usage_before;
        usize memory_usage_after;
        std::function<void()> visualize; // Function to show this step in UI
    };
    
    static std::vector<ProcessingStep> process_with_steps(
        renderer::resources::TextureData& texture,
        const TextureImportSettings& settings
    );
};

//=============================================================================
// Texture Importer Implementation
//=============================================================================

/**
 * @brief Comprehensive texture importer with educational features
 */
class TextureImporter : public AssetImporter {
private:
    // Cache for expensive operations
    mutable std::unordered_map<std::string, TextureAnalysis> analysis_cache_;
    mutable std::shared_mutex cache_mutex_;
    
    // Performance tracking
    mutable std::atomic<u64> total_imports_{0};
    mutable std::atomic<f64> total_import_time_{0.0};
    mutable std::atomic<usize> total_bytes_processed_{0};
    
    // Educational settings
    bool enable_detailed_analysis_{true};
    bool generate_optimization_suggestions_{true};
    bool track_processing_steps_{false};
    
public:
    TextureImporter();
    ~TextureImporter() = default;
    
    // AssetImporter interface
    std::vector<std::string> supported_extensions() const override;
    AssetType asset_type() const override { return AssetType::Texture; }
    bool can_import(const std::filesystem::path& file_path) const override;
    
    ImportResult import_asset(
        const std::filesystem::path& source_path,
        const ImportSettings& settings,
        memory::MemoryTracker* memory_tracker = nullptr
    ) const override;
    
    std::unique_ptr<ImportSettings> create_default_settings() const override;
    
    core::Result<void, const char*> validate_file(const std::filesystem::path& file_path) const override;
    std::string analyze_file(const std::filesystem::path& file_path) const override;
    
    // Educational interface
    std::string get_educational_description() const override;
    std::vector<std::string> get_learning_objectives() const override;
    
    // Advanced texture analysis
    TextureAnalysis analyze_texture(const std::filesystem::path& file_path) const;
    TextureAnalysis analyze_texture_data(const renderer::resources::TextureData& data) const;
    
    // Import with detailed feedback
    ImportResult import_with_analysis(
        const std::filesystem::path& source_path,
        const TextureImportSettings& settings,
        bool generate_analysis = true,
        memory::MemoryTracker* memory_tracker = nullptr
    ) const;
    
    // Batch operations
    struct BatchImportResult {
        std::vector<ImportResult> results;
        f64 total_time_seconds{0.0};
        usize total_memory_used{0};
        std::string summary_report;
    };
    
    BatchImportResult import_batch(
        const std::vector<std::filesystem::path>& files,
        const TextureImportSettings& settings,
        memory::MemoryTracker* memory_tracker = nullptr
    ) const;
    
    // Preview and validation
    core::Result<renderer::resources::TextureData, const char*> generate_preview(
        const std::filesystem::path& file_path,
        u32 max_dimension = 512
    ) const;
    
    bool validate_import_settings(
        const std::filesystem::path& file_path,
        const TextureImportSettings& settings,
        std::vector<std::string>& warnings
    ) const;
    
    // Performance monitoring
    struct ImporterStatistics {
        u64 total_imports{0};
        f64 average_import_time{0.0};
        f64 total_import_time{0.0};
        usize total_bytes_processed{0};
        f64 average_throughput_mbps{0.0};
        
        // Format statistics
        std::unordered_map<std::string, u32> format_counts;
        std::unordered_map<std::string, f64> format_times;
        
        // Quality metrics
        f32 average_quality_score{0.0f};
        u32 high_quality_imports{0};
        u32 failed_imports{0};
        f64 success_rate{1.0};
    };
    
    ImporterStatistics get_statistics() const;
    void reset_statistics();
    
    // Configuration
    void enable_detailed_analysis(bool enable) { enable_detailed_analysis_ = enable; }
    void enable_optimization_suggestions(bool enable) { generate_optimization_suggestions_ = enable; }
    void enable_processing_steps_tracking(bool enable) { track_processing_steps_ = enable; }
    
    // Educational utilities
    std::string generate_format_comparison(const std::vector<std::filesystem::path>& files) const;
    std::string generate_compression_analysis(const std::filesystem::path& file_path) const;
    std::string generate_optimization_guide(const std::filesystem::path& file_path) const;
    
private:
    // Internal import implementations
    ImportResult import_stb_supported(
        const std::filesystem::path& source_path,
        const TextureImportSettings& settings,
        memory::MemoryTracker* memory_tracker
    ) const;
    
    ImportResult import_dds(
        const std::filesystem::path& source_path,
        const TextureImportSettings& settings,
        memory::MemoryTracker* memory_tracker
    ) const;
    
    ImportResult import_hdr(
        const std::filesystem::path& source_path,
        const TextureImportSettings& settings,
        memory::MemoryTracker* memory_tracker
    ) const;
    
    // Analysis helpers
    void analyze_color_content(
        const renderer::resources::TextureData& data,
        TextureAnalysis::ColorAnalysis& analysis
    ) const;
    
    void analyze_performance_characteristics(
        const renderer::resources::TextureData& data,
        const std::filesystem::path& source_path,
        TextureAnalysis::PerformanceInfo& performance
    ) const;
    
    void generate_educational_insights(
        const renderer::resources::TextureData& data,
        const std::filesystem::path& source_path,
        TextureAnalysis::EducationalInfo& educational
    ) const;
    
    // Processing pipeline
    ImportResult process_texture_data(
        renderer::resources::TextureData texture_data,
        const TextureImportSettings& settings,
        const std::filesystem::path& source_path,
        memory::MemoryTracker* memory_tracker
    ) const;
    
    // Validation helpers
    bool validate_texture_dimensions(u32 width, u32 height) const;
    bool validate_format_support(renderer::resources::TextureFormat format) const;
    f32 calculate_quality_score(const renderer::resources::TextureData& data) const;
    
    // Cache management
    void cache_analysis(const std::string& file_path, const TextureAnalysis& analysis) const;
    std::optional<TextureAnalysis> get_cached_analysis(const std::string& file_path) const;
    void clear_analysis_cache() const;
    
    // Utility functions
    std::string get_format_extension(const std::filesystem::path& file_path) const;
    bool is_power_of_two(u32 value) const;
    u32 next_power_of_two(u32 value) const;
    
    // Educational content generation
    std::string generate_processing_report(
        const std::vector<TextureProcessor::ProcessingStep>& steps
    ) const;
    
    std::string generate_memory_usage_analysis(
        const renderer::resources::TextureData& original,
        const renderer::resources::TextureData& processed
    ) const;
};

//=============================================================================
// Specialized Texture Importers
//=============================================================================

/**
 * @brief Specialized importer for compressed textures (DDS, KTX)
 */
class CompressedTextureImporter : public AssetImporter {
public:
    CompressedTextureImporter() = default;
    
    std::vector<std::string> supported_extensions() const override;
    AssetType asset_type() const override { return AssetType::Texture; }
    bool can_import(const std::filesystem::path& file_path) const override;
    
    ImportResult import_asset(
        const std::filesystem::path& source_path,
        const ImportSettings& settings,
        memory::MemoryTracker* memory_tracker = nullptr
    ) const override;
    
    std::unique_ptr<ImportSettings> create_default_settings() const override;
    core::Result<void, const char*> validate_file(const std::filesystem::path& file_path) const override;
    std::string analyze_file(const std::filesystem::path& file_path) const override;
    
    std::string get_educational_description() const override;
    std::vector<std::string> get_learning_objectives() const override;
};

/**
 * @brief Specialized importer for HDR textures
 */
class HDRTextureImporter : public AssetImporter {
public:
    HDRTextureImporter() = default;
    
    std::vector<std::string> supported_extensions() const override;
    AssetType asset_type() const override { return AssetType::Texture; }
    bool can_import(const std::filesystem::path& file_path) const override;
    
    ImportResult import_asset(
        const std::filesystem::path& source_path,
        const ImportSettings& settings,
        memory::MemoryTracker* memory_tracker = nullptr
    ) const override;
    
    std::unique_ptr<ImportSettings> create_default_settings() const override;
    core::Result<void, const char*> validate_file(const std::filesystem::path& file_path) const override;
    std::string analyze_file(const std::filesystem::path& file_path) const override;
    
    std::string get_educational_description() const override;
    std::vector<std::string> get_learning_objectives() const override;
    
    // HDR-specific functionality
    struct HDRAnalysis {
        f32 dynamic_range{1.0f};
        f32 peak_luminance{1.0f};
        f32 average_luminance{1.0f};
        bool needs_tone_mapping{false};
        std::string tone_mapping_suggestions;
    };
    
    HDRAnalysis analyze_hdr_content(const std::filesystem::path& file_path) const;
};

//=============================================================================
// Texture Import Factory
//=============================================================================

/**
 * @brief Factory for creating appropriate texture importers
 */
class TextureImporterFactory {
public:
    static std::unique_ptr<AssetImporter> create_importer(const std::filesystem::path& file_path);
    static std::unique_ptr<AssetImporter> create_importer(const std::string& extension);
    
    // Registry of available importers
    static void register_importer(
        const std::string& extension,
        std::function<std::unique_ptr<AssetImporter>()> factory
    );
    
    static std::vector<std::string> get_all_supported_extensions();
    static bool is_extension_supported(const std::string& extension);
    
private:
    static std::unordered_map<std::string, std::function<std::unique_ptr<AssetImporter>()>> importer_registry_;
    static void initialize_default_importers();
    static std::once_flag registry_initialized_;
};

} // namespace ecscope::assets::importers