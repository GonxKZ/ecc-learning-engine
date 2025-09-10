#pragma once

#include <cstdint>
#include <string>
#include <chrono>
#include <unordered_map>
#include <vector>
#include <memory>
#include <atomic>

namespace ecscope::assets {

    // Forward declarations
    class Asset;
    class AssetHandle;
    class AssetManager;

    // Asset type definitions
    using AssetID = uint64_t;
    using AssetTypeID = uint32_t;
    using AssetVersion = uint32_t;
    using LoadPriority = int32_t;

    // Invalid asset ID constant
    constexpr AssetID INVALID_ASSET_ID = 0;

    // Asset loading priorities
    namespace Priority {
        constexpr LoadPriority CRITICAL = 100;
        constexpr LoadPriority HIGH = 75;
        constexpr LoadPriority NORMAL = 50;
        constexpr LoadPriority LOW = 25;
        constexpr LoadPriority BACKGROUND = 0;
    }

    // Asset states
    enum class AssetState : uint8_t {
        UNLOADED = 0,
        QUEUED,
        LOADING,
        LOADED,
        ERROR,
        STALE,      // Needs reload due to file change
        STREAMING   // Being streamed incrementally
    };

    // Asset types
    enum class AssetType : AssetTypeID {
        UNKNOWN = 0,
        TEXTURE,
        MESH,
        MATERIAL,
        SHADER,
        AUDIO,
        ANIMATION,
        FONT,
        SCENE,
        SCRIPT,
        CONFIG,
        BINARY,
        COUNT
    };

    // Asset loading flags
    enum class LoadFlags : uint32_t {
        NONE = 0,
        ASYNC = 1 << 0,         // Load asynchronously
        STREAMING = 1 << 1,     // Enable streaming
        COMPRESSED = 1 << 2,    // Asset is compressed
        CACHEABLE = 1 << 3,     // Can be cached to disk
        HOT_RELOAD = 1 << 4,    // Enable hot reloading
        PRELOAD = 1 << 5,       // Load at startup
        PERSISTENT = 1 << 6,    // Never unload
        HIGH_PRIORITY = 1 << 7, // High priority loading
        NETWORK = 1 << 8,       // Can be loaded over network
        MEMORY_MAPPED = 1 << 9  // Use memory mapping
    };

    // Asset quality levels for LOD
    enum class QualityLevel : uint8_t {
        LOW = 0,
        MEDIUM,
        HIGH,
        ULTRA,
        COUNT
    };

    // Asset metadata structure
    struct AssetMetadata {
        AssetID id;
        AssetType type;
        std::string path;
        std::string name;
        AssetVersion version;
        size_t size_bytes;
        std::chrono::system_clock::time_point last_modified;
        LoadFlags flags;
        QualityLevel quality;
        std::vector<AssetID> dependencies;
        std::unordered_map<std::string, std::string> custom_properties;

        AssetMetadata() : id(INVALID_ASSET_ID), type(AssetType::UNKNOWN), 
                         version(0), size_bytes(0), flags(LoadFlags::NONE),
                         quality(QualityLevel::MEDIUM) {}
    };

    // Asset load request
    struct AssetLoadRequest {
        AssetID id;
        std::string path;
        AssetType type;
        LoadPriority priority;
        LoadFlags flags;
        QualityLevel quality;
        std::function<void(std::shared_ptr<Asset>)> callback;

        AssetLoadRequest() : id(INVALID_ASSET_ID), type(AssetType::UNKNOWN),
                           priority(Priority::NORMAL), flags(LoadFlags::NONE),
                           quality(QualityLevel::MEDIUM) {}
    };

    // Asset streaming info
    struct StreamingInfo {
        size_t bytes_loaded;
        size_t total_bytes;
        float progress;
        bool is_complete;
        QualityLevel current_quality;
        QualityLevel target_quality;

        StreamingInfo() : bytes_loaded(0), total_bytes(0), progress(0.0f),
                         is_complete(false), current_quality(QualityLevel::LOW),
                         target_quality(QualityLevel::MEDIUM) {}
    };

    // Asset load statistics
    struct LoadStatistics {
        std::atomic<uint64_t> total_requests{0};
        std::atomic<uint64_t> successful_loads{0};
        std::atomic<uint64_t> failed_loads{0};
        std::atomic<uint64_t> cache_hits{0};
        std::atomic<uint64_t> cache_misses{0};
        std::atomic<uint64_t> bytes_loaded{0};
        std::atomic<uint64_t> load_time_ms{0};

        void reset() {
            total_requests = 0;
            successful_loads = 0;
            failed_loads = 0;
            cache_hits = 0;
            cache_misses = 0;
            bytes_loaded = 0;
            load_time_ms = 0;
        }
    };

    // Utility functions
    inline LoadFlags operator|(LoadFlags a, LoadFlags b) {
        return static_cast<LoadFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline LoadFlags operator&(LoadFlags a, LoadFlags b) {
        return static_cast<LoadFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    inline bool has_flag(LoadFlags flags, LoadFlags flag) {
        return (flags & flag) == flag;
    }

    // Asset type utilities
    const char* asset_type_to_string(AssetType type);
    AssetType string_to_asset_type(const std::string& str);
    const char* asset_state_to_string(AssetState state);

    // Asset ID generation
    AssetID generate_asset_id();
    AssetID path_to_asset_id(const std::string& path);

} // namespace ecscope::assets