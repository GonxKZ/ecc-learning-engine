#pragma once

// Core asset system headers
#include "asset_types.hpp"
#include "asset.hpp"
#include "asset_manager.hpp"
#include "asset_registry.hpp"
#include "asset_loader.hpp"
#include "asset_cache.hpp"
#include "asset_database.hpp"
#include "asset_streaming.hpp"
#include "hot_reload.hpp"
#include "asset_tools.hpp"

// Asset processors
#include "processors/asset_processor.hpp"
#include "processors/texture_processor.hpp"
#include "processors/mesh_processor.hpp"
#include "processors/audio_processor.hpp"
#include "processors/shader_processor.hpp"

/**
 * ECScope Asset System
 * 
 * A comprehensive, production-ready asset management system for modern game engines.
 * 
 * Key Features:
 * - Multi-threaded asset loading with priority queues
 * - Streaming asset system with LOD and quality management
 * - Hot-reload system with file system watching
 * - Asset processing pipeline for all major asset types
 * - Multi-level caching system (memory + disk)
 * - Asset database with metadata and dependency tracking
 * - Compression support (LZ4, Zstd)
 * - Asset bundling and packaging tools
 * - Cross-platform file system support
 * - Network-based asset distribution
 * - Predictive asset streaming
 * - Asset validation and optimization tools
 * - Command-line build tools
 * 
 * Supported Asset Types:
 * - Textures (PNG, JPG, TGA, DDS, KTX, HDR, EXR)
 * - 3D Models (OBJ, FBX, GLTF, DAE, 3DS, PLY)
 * - Audio (WAV, MP3, OGG, FLAC, AAC)
 * - Shaders (GLSL, HLSL, SPIR-V, MSL, WGSL)
 * - Materials and configurations
 * - Fonts and UI resources
 * - Animation data
 * - Scene descriptions
 * - Custom binary formats
 * 
 * Threading Model:
 * - Main thread: Asset management and coordination
 * - Worker threads: Asset loading and processing
 * - Background threads: Cache management and hot-reload
 * - Streaming threads: Predictive loading and quality management
 * 
 * Memory Management:
 * - Reference counting for automatic cleanup
 * - Configurable memory budgets
 * - LRU/LFU cache eviction policies
 * - Memory-mapped file I/O for large assets
 * - Compression for reduced memory footprint
 * 
 * Performance Features:
 * - Asset dependency resolution and load ordering
 * - Incremental loading and streaming
 * - Quality-based LOD system
 * - Predictive asset streaming
 * - Multi-level caching strategy
 * - Asset bundling for reduced I/O overhead
 * 
 * Development Tools:
 * - Asset browser and inspector
 * - Real-time performance monitoring
 * - Asset validation and optimization tools
 * - Build system integration
 * - Command-line asset processing tools
 * - Hot-reload for rapid iteration
 * 
 * Usage Example:
 * 
 * ```cpp
 * // Initialize asset system
 * AssetManagerConfig config;
 * config.max_memory_mb = 512;
 * config.worker_threads = 4;
 * config.enable_hot_reload = true;
 * 
 * auto asset_manager = std::make_unique<AssetManager>(config);
 * asset_manager->initialize();
 * set_asset_manager(std::move(asset_manager));
 * 
 * // Load assets
 * auto texture = load_asset<TextureAsset>("textures/diffuse.png");
 * auto model = load_asset<ModelAsset>("models/character.fbx");
 * 
 * // Asynchronous loading
 * auto future = load_asset_async<AudioAsset>("audio/music.ogg");
 * 
 * // Streaming system
 * auto& streaming = get_streaming_system();
 * streaming.request_asset(texture->get_id(), QualityLevel::HIGH, 
 *                        StreamingPriority::VISIBLE, 10.0f);
 * 
 * // Hot reload
 * auto& hot_reload = get_asset_manager().get_hot_reload_system();
 * hot_reload.register_asset_path(texture->get_id(), "textures/diffuse.png");
 * ```
 */

namespace ecscope::assets {

    // Asset system initialization and shutdown
    bool initialize_asset_system(const AssetManagerConfig& config = AssetManagerConfig{});
    void shutdown_asset_system();
    bool is_asset_system_initialized();

    // Convenience functions for common operations
    template<typename T>
    TypedAssetHandle<T> load_asset(const std::string& path,
                                   LoadPriority priority = Priority::NORMAL,
                                   LoadFlags flags = LoadFlags::NONE,
                                   QualityLevel quality = QualityLevel::MEDIUM) {
        return TypedAssetHandle<T>(get_asset_manager().load_asset(path, priority, flags, quality));
    }

    template<typename T>
    std::future<TypedAssetHandle<T>> load_asset_async(const std::string& path,
                                                      LoadPriority priority = Priority::NORMAL,
                                                      LoadFlags flags = LoadFlags::ASYNC,
                                                      QualityLevel quality = QualityLevel::MEDIUM) {
        auto future = get_asset_manager().load_asset_async(path, priority, flags, quality);
        return std::async(std::launch::deferred, [future = std::move(future)]() mutable {
            return TypedAssetHandle<T>(future.get());
        });
    }

    // Batch loading utilities
    template<typename T>
    std::vector<TypedAssetHandle<T>> load_assets_batch(const std::vector<std::string>& paths,
                                                       LoadPriority priority = Priority::NORMAL,
                                                       LoadFlags flags = LoadFlags::NONE) {
        auto handles = get_asset_manager().load_assets_batch(paths, priority, flags);
        std::vector<TypedAssetHandle<T>> typed_handles;
        typed_handles.reserve(handles.size());
        for (auto& handle : handles) {
            typed_handles.emplace_back(std::move(handle));
        }
        return typed_handles;
    }

    // Asset system statistics
    struct AssetSystemStatistics {
        LoadStatistics load_stats;
        StreamingStatistics streaming_stats;
        CacheStatistics cache_stats;
        size_t total_assets_loaded = 0;
        size_t total_memory_used = 0;
        size_t total_cache_hits = 0;
        size_t total_cache_misses = 0;
        float average_load_time_ms = 0.0f;
    };

    AssetSystemStatistics get_asset_system_statistics();
    void reset_asset_system_statistics();

    // System configuration updates
    void set_asset_quality_level(QualityLevel quality);
    QualityLevel get_asset_quality_level();

    void set_streaming_enabled(bool enabled);
    bool is_streaming_enabled();

    void set_hot_reload_enabled(bool enabled);
    bool is_hot_reload_enabled();

    // Memory management utilities
    void collect_garbage();
    void free_unused_assets();
    size_t get_total_memory_usage();
    
    // Debug utilities
    void dump_asset_system_info();
    void dump_memory_usage();
    void dump_cache_statistics();
    void dump_streaming_state();

} // namespace ecscope::assets

// Convenience macros for common operations
#define ECSCOPE_LOAD_TEXTURE(path) ecscope::assets::load_asset<ecscope::assets::TextureAsset>(path)
#define ECSCOPE_LOAD_MODEL(path) ecscope::assets::load_asset<ecscope::assets::ModelAsset>(path)
#define ECSCOPE_LOAD_AUDIO(path) ecscope::assets::load_asset<ecscope::assets::AudioAsset>(path)
#define ECSCOPE_LOAD_SHADER(path) ecscope::assets::load_asset<ecscope::assets::ShaderAsset>(path)

#define ECSCOPE_LOAD_TEXTURE_ASYNC(path) ecscope::assets::load_asset_async<ecscope::assets::TextureAsset>(path)
#define ECSCOPE_LOAD_MODEL_ASYNC(path) ecscope::assets::load_asset_async<ecscope::assets::ModelAsset>(path)
#define ECSCOPE_LOAD_AUDIO_ASYNC(path) ecscope::assets::load_asset_async<ecscope::assets::AudioAsset>(path)
#define ECSCOPE_LOAD_SHADER_ASYNC(path) ecscope::assets::load_asset_async<ecscope::assets::ShaderAsset>(path)