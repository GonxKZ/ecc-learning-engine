#pragma once

/**
 * @file asset_pipeline.hpp
 * @brief Comprehensive Asset Pipeline System for ECScope Educational Platform
 * 
 * This system provides a complete asset management solution with hot-reloading,
 * importers, and educational features designed for teaching game engine concepts.
 * 
 * Key Features:
 * - Multi-format asset importers (textures, models, audio, shaders)
 * - Real-time hot-reloading with dependency tracking
 * - Memory-efficient asset caching and reference counting
 * - Asynchronous loading with loading screens
 * - Educational visualization of asset processing
 * - Integration with existing memory systems and scene editor
 * 
 * Educational Value:
 * - Demonstrates asset pipeline architecture patterns
 * - Shows performance implications of different loading strategies
 * - Illustrates memory management for large assets
 * - Provides real-time metrics and analysis tools
 * - Teaches dependency management and cascade updates
 * 
 * Architecture:
 * - AssetRegistry: Central asset management and reference counting
 * - AssetImporter: Pluggable importers for different file formats
 * - HotReloadManager: File system watching and automatic updates
 * - AssetLoader: Asynchronous loading with progress tracking
 * - DependencyTracker: Dependency analysis and cascade updates
 * - EducationalAnalyzer: Learning tools and performance visualization
 * 
 * @author ECScope Educational ECS Framework - Asset Pipeline
 * @date 2025
 */

#include "core/types.hpp"
#include "core/result.hpp"
#include "core/log.hpp"
#include "hot_reload_system.hpp"
#include "memory/memory_tracker.hpp"
#include "texture.hpp"
#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <shared_mutex>
#include <functional>
#include <chrono>
#include <future>
#include <queue>
#include <thread>
#include <optional>
#include <variant>

namespace ecscope::assets {

//=============================================================================
// Forward Declarations and Core Types
//=============================================================================

class AssetRegistry;
class AssetImporter;
class HotReloadManager;
class AssetLoader;
class DependencyTracker;
class EducationalAnalyzer;

using AssetID = u64;
constexpr AssetID INVALID_ASSET_ID = 0;

/**
 * @brief Supported asset types in the pipeline
 */
enum class AssetType : u8 {
    Unknown = 0,
    Texture,           // Images, textures (PNG, JPG, TGA, DDS, etc.)
    Model,             // 3D models (OBJ, FBX, glTF, COLLADA)
    Audio,             // Sound files (WAV, MP3, OGG, FLAC)
    Shader,            // Shader code (GLSL, HLSL, SPIR-V)
    Animation,         // Animation data (skeletal, morph targets)
    Material,          // Material definitions (JSON, XML)
    Scene,             // Scene descriptions (JSON, XML)
    Font,              // Font files (TTF, OTF)
    Config,            // Configuration files (JSON, XML, INI)
    Script,            // Script files (Lua, Python, JavaScript)
    COUNT
};

/**
 * @brief Asset loading priority levels
 */
enum class LoadPriority : u8 {
    Critical = 0,      // Must be loaded immediately (blocking)
    High,              // Load as soon as possible
    Normal,            // Standard priority
    Low,               // Load when resources are available
    Background         // Load in background when idle
};

/**
 * @brief Asset loading states for tracking
 */
enum class AssetState : u8 {
    Unloaded = 0,      // Asset exists but not loaded
    Loading,           // Currently being loaded
    Loaded,            // Successfully loaded and ready
    Failed,            // Loading failed with errors
    Outdated,          // Asset needs to be reloaded (hot-reload)
    Unloading          // Currently being unloaded
};

//=============================================================================
// Asset Metadata and Properties
//=============================================================================

/**
 * @brief Comprehensive asset metadata
 */
struct AssetMetadata {
    AssetID id{INVALID_ASSET_ID};
    std::filesystem::path source_path;
    std::filesystem::path cache_path;
    AssetType type{AssetType::Unknown};
    
    // File information
    usize file_size_bytes{0};
    std::filesystem::file_time_type last_modified;
    std::string file_hash; // For detecting content changes
    std::string import_settings_hash; // For detecting import setting changes
    
    // Loading information
    AssetState state{AssetState::Unloaded};
    LoadPriority priority{LoadPriority::Normal};
    f64 loading_time_seconds{0.0};
    f64 last_access_time{0.0};
    u32 access_count{0};
    
    // Memory usage
    usize memory_usage_bytes{0};
    usize compressed_size_bytes{0};
    f32 compression_ratio{1.0f};
    
    // Dependencies
    std::vector<AssetID> dependencies; // Assets this asset depends on
    std::vector<AssetID> dependents;   // Assets that depend on this asset
    
    // Import settings and results
    std::string importer_name;
    std::string import_settings; // JSON serialized import settings
    std::string import_log;      // Import process log for debugging
    bool import_succeeded{false};
    f32 import_quality_score{1.0f}; // 0.0 = poor, 1.0 = excellent
    
    // Educational metadata
    std::string educational_category; // For learning system organization
    std::vector<std::string> tags;    // Searchable tags
    std::string description;          // Human-readable description
    
    // Performance metrics
    struct PerformanceMetrics {
        f64 average_load_time{0.0};
        f64 total_load_time{0.0};
        u32 load_count{0};
        f64 cache_hit_ratio{1.0f};
        f32 memory_efficiency{1.0f}; // How efficiently memory is used
        
        void record_load_time(f64 time) {
            total_load_time += time;
            ++load_count;
            average_load_time = total_load_time / load_count;
        }
    } performance;
    
    AssetMetadata() {
        last_modified = std::filesystem::file_time_type::min();
        update_access_time();
    }
    
    void update_access_time() {
        using namespace std::chrono;
        last_access_time = duration<f64>(steady_clock::now().time_since_epoch()).count();
        ++access_count;
    }
    
    f64 get_priority_score() const {
        f64 base_score = static_cast<f64>(static_cast<u8>(LoadPriority::Background) - static_cast<u8>(priority));
        f64 recency_score = 1.0 / (1.0 + get_time_since_access());
        f64 usage_score = std::log(1.0 + access_count) / 10.0;
        return base_score * 10.0 + recency_score + usage_score;
    }
    
    f64 get_time_since_access() const {
        using namespace std::chrono;
        f64 current_time = duration<f64>(steady_clock::now().time_since_epoch()).count();
        return current_time - last_access_time;
    }
    
    bool needs_reimport() const {
        if (source_path.empty() || !std::filesystem::exists(source_path)) {
            return false;
        }
        
        try {
            auto current_modified = std::filesystem::last_write_time(source_path);
            return current_modified > last_modified;
        } catch (...) {
            return false;
        }
    }
};

//=============================================================================
// Asset Data Container
//=============================================================================

/**
 * @brief Generic container for asset data with type safety
 */
class AssetData {
private:
    std::variant<
        std::monostate,                          // Empty/unloaded
        std::shared_ptr<renderer::resources::TextureData>, // Texture data
        std::shared_ptr<void>,                   // Generic data pointer
        std::vector<u8>,                         // Raw binary data
        std::string                              // Text data
    > data_;
    
    AssetType type_{AssetType::Unknown};
    usize size_bytes_{0};
    bool is_valid_{false};
    
public:
    AssetData() = default;
    
    template<typename T>
    AssetData(std::shared_ptr<T> data, AssetType type, usize size = 0) 
        : data_(data), type_(type), size_bytes_(size), is_valid_(data != nullptr) {}
    
    AssetData(std::vector<u8> data, AssetType type)
        : data_(std::move(data)), type_(type), size_bytes_(std::get<std::vector<u8>>(data_).size()), is_valid_(true) {}
    
    AssetData(std::string data, AssetType type)
        : data_(std::move(data)), type_(type), size_bytes_(std::get<std::string>(data_).size()), is_valid_(true) {}
    
    // Type queries
    AssetType type() const { return type_; }
    usize size_bytes() const { return size_bytes_; }
    bool is_valid() const { return is_valid_; }
    bool is_empty() const { return std::holds_alternative<std::monostate>(data_); }
    
    // Type-safe accessors
    template<typename T>
    std::shared_ptr<T> get_typed_data() const {
        if (auto ptr = std::get_if<std::shared_ptr<void>>(&data_)) {
            return std::static_pointer_cast<T>(*ptr);
        }
        return nullptr;
    }
    
    std::shared_ptr<renderer::resources::TextureData> get_texture_data() const {
        if (auto ptr = std::get_if<std::shared_ptr<renderer::resources::TextureData>>(&data_)) {
            return *ptr;
        }
        return nullptr;
    }
    
    const std::vector<u8>* get_binary_data() const {
        return std::get_if<std::vector<u8>>(&data_);
    }
    
    const std::string* get_text_data() const {
        return std::get_if<std::string>(&data_);
    }
    
    // Memory usage calculation
    usize calculate_memory_usage() const {
        switch (type_) {
            case AssetType::Texture: {
                if (auto tex_data = get_texture_data()) {
                    return tex_data->get_memory_usage();
                }
                break;
            }
            case AssetType::Model:
            case AssetType::Audio:
                return size_bytes_;
            default:
                break;
        }
        return size_bytes_;
    }
    
    void clear() {
        data_ = std::monostate{};
        type_ = AssetType::Unknown;
        size_bytes_ = 0;
        is_valid_ = false;
    }
};

//=============================================================================
// Asset Import Settings
//=============================================================================

/**
 * @brief Base class for asset import settings
 */
struct ImportSettings {
    virtual ~ImportSettings() = default;
    virtual std::string serialize() const = 0;
    virtual bool deserialize(const std::string& data) = 0;
    virtual std::string calculate_hash() const = 0;
};

/**
 * @brief Texture import settings
 */
struct TextureImportSettings : public ImportSettings {
    renderer::resources::TextureFormat target_format{renderer::resources::TextureFormat::RGBA8};
    bool generate_mipmaps{true};
    bool flip_vertically{false};
    f32 compression_quality{0.8f};
    u32 max_size{4096}; // Maximum texture dimension
    bool power_of_two{false}; // Force power-of-two dimensions
    
    // Advanced settings
    bool premultiply_alpha{false};
    f32 gamma_correction{1.0f};
    bool srgb_color_space{true};
    
    std::string serialize() const override;
    bool deserialize(const std::string& data) override;
    std::string calculate_hash() const override;
};

/**
 * @brief Model import settings
 */
struct ModelImportSettings : public ImportSettings {
    f32 scale_factor{1.0f};
    bool generate_normals{false};
    bool generate_tangents{false};
    bool optimize_meshes{true};
    bool merge_vertices{true};
    f32 smoothing_angle{45.0f}; // For normal generation
    
    // Animation settings
    bool import_animations{true};
    f32 animation_sample_rate{30.0f};
    
    std::string serialize() const override;
    bool deserialize(const std::string& data) override;
    std::string calculate_hash() const override;
};

/**
 * @brief Audio import settings
 */
struct AudioImportSettings : public ImportSettings {
    u32 sample_rate{44100};
    u16 bit_depth{16};
    u8 channels{2}; // 1 = mono, 2 = stereo
    bool compress{false};
    f32 quality{0.9f}; // For compressed formats
    bool normalize{false};
    
    std::string serialize() const override;
    bool deserialize(const std::string& data) override;
    std::string calculate_hash() const override;
};

/**
 * @brief Shader import settings
 */
struct ShaderImportSettings : public ImportSettings {
    enum class ShaderStage : u8 {
        Vertex, Fragment, Geometry, Compute, TessControl, TessEvaluation
    } stage{ShaderStage::Vertex};
    
    std::vector<std::string> defines; // Preprocessor definitions
    std::vector<std::string> include_paths; // Include search paths
    bool optimize{true};
    bool debug_info{false};
    std::string target_version{"330"}; // GLSL version
    
    std::string serialize() const override;
    bool deserialize(const std::string& data) override;
    std::string calculate_hash() const override;
};

//=============================================================================
// Asset Import Result
//=============================================================================

/**
 * @brief Result of asset import operation with detailed information
 */
struct ImportResult {
    bool success{false};
    std::string error_message;
    std::vector<std::string> warnings;
    
    AssetData imported_data;
    AssetMetadata metadata;
    
    // Import statistics
    f64 import_time_seconds{0.0};
    usize original_size_bytes{0};
    usize processed_size_bytes{0};
    f32 quality_score{1.0f}; // 0.0 = poor, 1.0 = excellent
    
    // Educational information
    std::vector<std::string> processing_steps; // Steps taken during import
    std::string optimization_suggestions;      // Performance improvement suggestions
    
    ImportResult() = default;
    
    static ImportResult success_result(AssetData data, AssetMetadata meta) {
        ImportResult result;
        result.success = true;
        result.imported_data = std::move(data);
        result.metadata = std::move(meta);
        return result;
    }
    
    static ImportResult failure_result(const std::string& error) {
        ImportResult result;
        result.success = false;
        result.error_message = error;
        return result;
    }
    
    void add_warning(const std::string& warning) {
        warnings.push_back(warning);
    }
    
    void add_processing_step(const std::string& step) {
        processing_steps.push_back(step);
    }
    
    f32 get_compression_ratio() const {
        if (original_size_bytes > 0) {
            return static_cast<f32>(processed_size_bytes) / original_size_bytes;
        }
        return 1.0f;
    }
};

//=============================================================================
// Asset Importer Interface
//=============================================================================

/**
 * @brief Base interface for asset importers
 */
class AssetImporter {
public:
    virtual ~AssetImporter() = default;
    
    // Core import functionality
    virtual std::vector<std::string> supported_extensions() const = 0;
    virtual AssetType asset_type() const = 0;
    virtual bool can_import(const std::filesystem::path& file_path) const = 0;
    
    virtual ImportResult import_asset(
        const std::filesystem::path& source_path,
        const ImportSettings& settings,
        memory::MemoryTracker* memory_tracker = nullptr
    ) const = 0;
    
    virtual std::unique_ptr<ImportSettings> create_default_settings() const = 0;
    
    // Validation and analysis
    virtual core::Result<void, const char*> validate_file(const std::filesystem::path& file_path) const = 0;
    virtual std::string analyze_file(const std::filesystem::path& file_path) const = 0;
    
    // Educational features
    virtual std::string get_educational_description() const = 0;
    virtual std::vector<std::string> get_learning_objectives() const = 0;
    
protected:
    // Helper functions for importers
    static std::string calculate_file_hash(const std::filesystem::path& file_path);
    static usize get_file_size(const std::filesystem::path& file_path);
    static bool is_file_readable(const std::filesystem::path& file_path);
};

//=============================================================================
// Asset Registry - Central Asset Management
//=============================================================================

/**
 * @brief Central registry for all assets with reference counting and caching
 */
class AssetRegistry {
private:
    struct AssetEntry {
        AssetMetadata metadata;
        AssetData data;
        std::atomic<u32> reference_count{0};
        mutable std::shared_mutex data_mutex; // Protects the data field
        
        AssetEntry() = default;
        AssetEntry(AssetMetadata meta) : metadata(std::move(meta)) {}
        
        // Non-copyable, movable
        AssetEntry(const AssetEntry&) = delete;
        AssetEntry& operator=(const AssetEntry&) = delete;
        AssetEntry(AssetEntry&&) = default;
        AssetEntry& operator=(AssetEntry&&) = default;
    };
    
    // Core storage
    std::unordered_map<AssetID, std::unique_ptr<AssetEntry>> assets_;
    std::unordered_map<std::string, AssetID> path_to_id_;
    mutable std::shared_mutex registry_mutex_;
    
    // ID generation
    std::atomic<AssetID> next_asset_id_{1};
    
    // Memory management
    memory::MemoryTracker* memory_tracker_{nullptr};
    std::atomic<usize> total_memory_usage_{0};
    usize memory_limit_bytes_{0}; // 0 = no limit
    
    // Statistics
    mutable std::atomic<u64> cache_hits_{0};
    mutable std::atomic<u64> cache_misses_{0};
    mutable std::atomic<u64> total_loads_{0};
    
public:
    explicit AssetRegistry(memory::MemoryTracker* tracker = nullptr) 
        : memory_tracker_(tracker) {}
    
    ~AssetRegistry() = default;
    
    // Non-copyable
    AssetRegistry(const AssetRegistry&) = delete;
    AssetRegistry& operator=(const AssetRegistry&) = delete;
    
    // Asset registration and management
    AssetID register_asset(const std::filesystem::path& source_path, AssetType type);
    bool unregister_asset(AssetID id);
    bool has_asset(AssetID id) const;
    AssetID find_asset_by_path(const std::filesystem::path& path) const;
    
    // Asset data management
    bool set_asset_data(AssetID id, AssetData data);
    AssetData get_asset_data(AssetID id) const;
    bool clear_asset_data(AssetID id);
    
    // Metadata management
    std::optional<AssetMetadata> get_metadata(AssetID id) const;
    bool update_metadata(AssetID id, const AssetMetadata& metadata);
    
    // Reference counting
    u32 add_reference(AssetID id);
    u32 remove_reference(AssetID id);
    u32 get_reference_count(AssetID id) const;
    
    // State management
    AssetState get_asset_state(AssetID id) const;
    bool set_asset_state(AssetID id, AssetState state);
    
    // Collection operations
    std::vector<AssetID> get_all_assets() const;
    std::vector<AssetID> get_assets_by_type(AssetType type) const;
    std::vector<AssetID> get_loaded_assets() const;
    std::vector<AssetID> get_outdated_assets() const;
    
    // Memory management
    void set_memory_limit(usize limit_bytes) { memory_limit_bytes_ = limit_bytes; }
    usize get_memory_usage() const { return total_memory_usage_.load(); }
    usize get_memory_limit() const { return memory_limit_bytes_; }
    bool is_over_memory_limit() const;
    usize garbage_collect(); // Returns bytes freed
    
    // Statistics
    struct RegistryStatistics {
        u32 total_assets{0};
        u32 loaded_assets{0};
        u32 referenced_assets{0};
        usize total_memory_usage{0};
        usize memory_limit{0};
        f32 memory_usage_percentage{0.0f};
        u64 cache_hits{0};
        u64 cache_misses{0};
        f64 cache_hit_ratio{0.0};
        u64 total_loads{0};
        f64 average_load_time{0.0};
    };
    
    RegistryStatistics get_statistics() const;
    void reset_statistics();
    
    // Utility functions
    std::string generate_memory_report() const;
    bool validate_all_assets() const;
    void cleanup_unused_assets();
    
private:
    AssetID generate_asset_id() { return next_asset_id_.fetch_add(1); }
    void update_memory_usage();
    void evict_least_recently_used_assets(usize target_bytes);
};

} // namespace ecscope::assets