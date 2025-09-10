#pragma once

#include "../core/asset_types.hpp"
#include "../core/asset_handle.hpp"
#include "../loading/asset_loader.hpp"
#include "../hotreload/file_watcher.hpp"
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <future>

namespace ecscope::assets {

// Forward declarations
class AssetDatabase;
class AssetCache;

// =============================================================================
// Asset Dependency Manager
// =============================================================================

class AssetDependencyManager {
public:
    AssetDependencyManager();
    ~AssetDependencyManager();
    
    // Dependency registration
    void addDependency(AssetID dependent, AssetID dependency);
    void removeDependency(AssetID dependent, AssetID dependency);
    void clearDependencies(AssetID asset_id);
    
    // Dependency queries
    std::vector<AssetID> getDependencies(AssetID asset_id) const;
    std::vector<AssetID> getDependents(AssetID asset_id) const;
    std::vector<AssetID> getAllDependencies(AssetID asset_id, bool recursive = true) const;
    std::vector<AssetID> getAllDependents(AssetID asset_id, bool recursive = true) const;
    
    // Dependency validation
    bool hasDependency(AssetID dependent, AssetID dependency) const;
    bool hasCircularDependency(AssetID asset_id) const;
    bool canUnload(AssetID asset_id) const; // Check if any dependents are loaded
    
    // Loading order
    std::vector<AssetID> getLoadOrder(const std::vector<AssetID>& assets) const;
    std::vector<AssetID> getUnloadOrder(const std::vector<AssetID>& assets) const;
    
    // Dependency graph analysis
    struct DependencyStats {
        uint32_t total_dependencies = 0;
        uint32_t max_depth = 0;
        uint32_t circular_dependencies = 0;
        std::vector<std::vector<AssetID>> circular_chains;
    };
    
    DependencyStats analyzeDependencies() const;
    void exportDependencyGraph(const std::string& filename) const; // DOT format
    
private:
    // Dependency graph representation
    std::unordered_map<AssetID, std::unordered_set<AssetID>> dependencies_;  // asset -> its dependencies
    std::unordered_map<AssetID, std::unordered_set<AssetID>> dependents_;    // asset -> what depends on it
    mutable std::shared_mutex dependencies_mutex_;
    
    // Helper methods
    void getAllDependenciesRecursive(AssetID asset_id, std::unordered_set<AssetID>& visited, 
                                    std::vector<AssetID>& result) const;
    bool detectCircularDependencyRecursive(AssetID asset_id, std::unordered_set<AssetID>& visited,
                                          std::unordered_set<AssetID>& recursion_stack) const;
};

// =============================================================================
// Asset Reference Manager
// =============================================================================

class AssetReferenceManager {
public:
    AssetReferenceManager();
    ~AssetReferenceManager();
    
    // Reference counting
    void addReference(AssetID asset_id);
    void removeReference(AssetID asset_id);
    uint32_t getReferenceCount(AssetID asset_id) const;
    
    // Asset lifecycle based on references
    bool canUnload(AssetID asset_id) const { return getReferenceCount(asset_id) == 0; }
    std::vector<AssetID> getUnloadCandidates() const;
    
    // Memory pressure handling
    void setMemoryBudget(uint64_t bytes) { memory_budget_ = bytes; }
    uint64_t getMemoryBudget() const { return memory_budget_; }
    uint64_t getMemoryUsed() const;
    
    std::vector<AssetID> getLeastRecentlyUsed(size_t count) const;
    std::vector<AssetID> getMemoryPressureUnloadCandidates() const;
    
    // Access tracking
    void recordAccess(AssetID asset_id);
    std::chrono::system_clock::time_point getLastAccess(AssetID asset_id) const;
    
private:
    struct ReferenceInfo {
        uint32_t ref_count = 0;
        std::chrono::system_clock::time_point last_access;
        uint64_t memory_usage = 0;
    };
    
    std::unordered_map<AssetID, ReferenceInfo> references_;
    mutable std::shared_mutex references_mutex_;
    std::atomic<uint64_t> memory_budget_{1024 * 1024 * 1024}; // 1GB default
};

// =============================================================================
// Asset Manager - Central hub for all asset operations
// =============================================================================

class AssetManager {
public:
    AssetManager();
    ~AssetManager();
    
    // Non-copyable, non-movable
    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;
    
    // Initialization and shutdown
    bool initialize(const std::string& asset_root_path = "assets/");
    void shutdown();
    bool isInitialized() const { return initialized_; }
    
    // Asset creation and registration
    template<typename T>
    AssetHandle<T> createAsset(const std::string& path, const AssetLoadParams& params = {});
    
    template<typename T>
    AssetHandle<T> getAsset(AssetID asset_id);
    
    template<typename T>
    AssetHandle<T> getAssetByPath(const std::string& path);
    
    // Asset loading
    template<typename T>
    std::future<AssetHandle<T>> loadAssetAsync(const std::string& path, const AssetLoadParams& params = {});
    
    template<typename T>
    AssetHandle<T> loadAssetSync(const std::string& path, const AssetLoadParams& params = {});
    
    // Asset management
    void unloadAsset(AssetID asset_id);
    void unloadAllAssets();
    void reloadAsset(AssetID asset_id);
    
    // Asset queries
    bool isAssetLoaded(AssetID asset_id) const;
    AssetState getAssetState(AssetID asset_id) const;
    const AssetMetadata* getAssetMetadata(AssetID asset_id) const;
    
    std::vector<AssetID> getLoadedAssets() const;
    std::vector<AssetID> getAssetsByType(AssetTypeID type_id) const;
    std::vector<AssetID> findAssets(const std::string& pattern) const;
    
    // Dependency management
    void addDependency(AssetID dependent, AssetID dependency);
    void removeDependency(AssetID dependent, AssetID dependency);
    std::vector<AssetID> getDependencies(AssetID asset_id) const;
    
    // Memory management
    void setMemoryBudget(uint64_t bytes);
    uint64_t getMemoryBudget() const;
    uint64_t getMemoryUsed() const;
    void freeUnusedMemory();
    void handleMemoryPressure();
    
    // Hot reload
    void enableHotReload();
    void disableHotReload();
    bool isHotReloadEnabled() const;
    
    // Asset database
    void saveAssetDatabase(const std::string& filename = "asset_database.json");
    bool loadAssetDatabase(const std::string& filename = "asset_database.json");
    
    // Statistics and monitoring
    AssetStats getStatistics() const;
    void resetStatistics();
    
    // Asset validation
    bool validateAsset(AssetID asset_id) const;
    std::vector<AssetID> getInvalidAssets() const;
    void repairAssets();
    
    // Batch operations
    struct BatchLoadRequest {
        std::string path;
        AssetTypeID type_id;
        AssetLoadParams params;
    };
    
    std::vector<std::future<AssetID>> loadAssetsAsync(const std::vector<BatchLoadRequest>& requests);
    void preloadAssets(const std::vector<std::string>& paths);
    
    // Asset streaming
    void setStreamingEnabled(bool enabled);
    bool isStreamingEnabled() const;
    void updateStreaming(float delta_time);
    
    // Platform integration
    void setPlatform(const std::string& platform) { platform_ = platform; }
    const std::string& getPlatform() const { return platform_; }
    
    // Event callbacks
    using AssetLoadedCallback = std::function<void(AssetID, const std::string&)>;
    using AssetUnloadedCallback = std::function<void(AssetID, const std::string&)>;
    using AssetFailedCallback = std::function<void(AssetID, const std::string&, const std::string&)>;
    
    void setAssetLoadedCallback(AssetLoadedCallback callback) { asset_loaded_callback_ = std::move(callback); }
    void setAssetUnloadedCallback(AssetUnloadedCallback callback) { asset_unloaded_callback_ = std::move(callback); }
    void setAssetFailedCallback(AssetFailedCallback callback) { asset_failed_callback_ = std::move(callback); }
    
private:
    struct AssetEntry {
        std::shared_ptr<Asset> asset;
        AssetMetadata metadata;
        std::atomic<AssetState> state{AssetState::NotLoaded};
        std::mutex state_mutex;
    };
    
    // Core systems
    std::unique_ptr<AssetLoader> loader_;
    std::unique_ptr<AssetDependencyManager> dependency_manager_;
    std::unique_ptr<AssetReferenceManager> reference_manager_;
    std::unique_ptr<HotReloadManager> hot_reload_manager_;
    std::unique_ptr<AssetDatabase> database_;
    std::unique_ptr<AssetCache> cache_;
    
    // Asset storage
    std::unordered_map<AssetID, std::unique_ptr<AssetEntry>> assets_;
    std::unordered_map<std::string, AssetID> path_to_id_;
    mutable std::shared_mutex assets_mutex_;
    
    // ID generation
    std::atomic<AssetID> next_asset_id_{1};
    
    // Configuration
    std::string asset_root_path_;
    std::string platform_;
    std::atomic<bool> initialized_{false};
    std::atomic<bool> streaming_enabled_{true};
    
    // Callbacks
    AssetLoadedCallback asset_loaded_callback_;
    AssetUnloadedCallback asset_unloaded_callback_;
    AssetFailedCallback asset_failed_callback_;
    
    // Helper methods
    AssetID generateAssetID();
    AssetID registerAssetPath(const std::string& path);
    void onAssetLoaded(AssetID asset_id);
    void onAssetUnloaded(AssetID asset_id);
    void onAssetFailed(AssetID asset_id, const std::string& error);
    void onHotReload(AssetID asset_id, const std::string& path);
    
    std::string normalizePath(const std::string& path) const;
    std::string resolveAssetPath(const std::string& path) const;
    AssetTypeID detectAssetType(const std::string& path) const;
    
    // Template method implementations
    template<typename T>
    AssetHandle<T> createAssetInternal(const std::string& path, const AssetLoadParams& params);
    
    template<typename T>
    std::shared_ptr<T> getAssetInternal(AssetID asset_id);
};

// =============================================================================
// Asset Database - Persistent storage for asset metadata
// =============================================================================

class AssetDatabase {
public:
    AssetDatabase();
    ~AssetDatabase();
    
    // Database operations
    bool open(const std::string& database_path);
    void close();
    bool isOpen() const { return is_open_; }
    
    // Asset metadata operations
    bool insertAsset(const AssetMetadata& metadata);
    bool updateAsset(const AssetMetadata& metadata);
    bool deleteAsset(AssetID asset_id);
    
    std::unique_ptr<AssetMetadata> getAssetMetadata(AssetID asset_id);
    std::unique_ptr<AssetMetadata> getAssetMetadataByPath(const std::string& path);
    
    // Bulk operations
    std::vector<AssetMetadata> getAllAssets();
    std::vector<AssetMetadata> getAssetsByType(AssetTypeID type_id);
    std::vector<AssetMetadata> findAssets(const std::string& pattern);
    
    // Dependency operations
    bool insertDependency(AssetID dependent, AssetID dependency);
    bool deleteDependency(AssetID dependent, AssetID dependency);
    std::vector<AssetID> getDependencies(AssetID asset_id);
    std::vector<AssetID> getDependents(AssetID asset_id);
    
    // Statistics
    uint64_t getAssetCount() const;
    uint64_t getTotalSize() const;
    
    // Maintenance
    void vacuum(); // Optimize database
    void backup(const std::string& backup_path);
    bool restore(const std::string& backup_path);
    
private:
    void* database_; // SQLite database handle
    std::atomic<bool> is_open_{false};
    mutable std::mutex database_mutex_;
    
    bool createTables();
    bool prepareStatements();
    void finalizeStatements();
    
    void* insert_asset_stmt_ = nullptr;
    void* update_asset_stmt_ = nullptr;
    void* delete_asset_stmt_ = nullptr;
    void* select_asset_stmt_ = nullptr;
    void* select_asset_by_path_stmt_ = nullptr;
};

// =============================================================================
// Asset Cache - Memory and disk caching system
// =============================================================================

class AssetCache {
public:
    AssetCache();
    ~AssetCache();
    
    // Cache configuration
    void setMemoryCacheSize(uint64_t bytes) { memory_cache_size_ = bytes; }
    void setDiskCacheSize(uint64_t bytes) { disk_cache_size_ = bytes; }
    void setDiskCachePath(const std::string& path) { disk_cache_path_ = path; }
    
    // Cache operations
    bool put(AssetID asset_id, const std::vector<uint8_t>& data);
    std::unique_ptr<std::vector<uint8_t>> get(AssetID asset_id);
    bool contains(AssetID asset_id) const;
    void remove(AssetID asset_id);
    void clear();
    
    // Cache statistics
    struct CacheStats {
        uint64_t hits = 0;
        uint64_t misses = 0;
        uint64_t memory_used = 0;
        uint64_t disk_used = 0;
        uint32_t entries = 0;
    };
    
    CacheStats getStatistics() const;
    void resetStatistics();
    
    // Cache maintenance
    void evictLRU();
    void validateCache();
    void compactDiskCache();
    
private:
    struct CacheEntry {
        std::vector<uint8_t> data;
        std::chrono::steady_clock::time_point last_access;
        uint64_t access_count = 0;
        bool in_memory = false;
        bool on_disk = false;
    };
    
    std::unordered_map<AssetID, CacheEntry> cache_entries_;
    mutable std::shared_mutex cache_mutex_;
    
    uint64_t memory_cache_size_ = 256 * 1024 * 1024; // 256MB default
    uint64_t disk_cache_size_ = 2 * 1024 * 1024 * 1024; // 2GB default
    std::string disk_cache_path_ = "cache/";
    
    mutable std::mutex stats_mutex_;
    CacheStats stats_;
    
    bool putToDisk(AssetID asset_id, const std::vector<uint8_t>& data);
    std::unique_ptr<std::vector<uint8_t>> getFromDisk(AssetID asset_id);
    std::string getDiskPath(AssetID asset_id) const;
};

// =============================================================================
// Template implementations
// =============================================================================

template<typename T>
AssetHandle<T> AssetManager::createAsset(const std::string& path, const AssetLoadParams& params) {
    return createAssetInternal<T>(path, params);
}

template<typename T>
AssetHandle<T> AssetManager::getAsset(AssetID asset_id) {
    return AssetHandle<T>(asset_id, this);
}

template<typename T>
AssetHandle<T> AssetManager::getAssetByPath(const std::string& path) {
    std::shared_lock lock(assets_mutex_);
    auto it = path_to_id_.find(normalizePath(path));
    if (it != path_to_id_.end()) {
        return AssetHandle<T>(it->second, this);
    }
    return AssetHandle<T>();
}

template<typename T>
std::future<AssetHandle<T>> AssetManager::loadAssetAsync(const std::string& path, const AssetLoadParams& params) {
    auto promise = std::make_shared<std::promise<AssetHandle<T>>>();
    auto future = promise->get_future();
    
    // Start async loading
    std::thread([this, path, params, promise]() {
        try {
            auto handle = loadAssetSync<T>(path, params);
            promise->set_value(handle);
        } catch (...) {
            promise->set_exception(std::current_exception());
        }
    }).detach();
    
    return future;
}

template<typename T>
AssetHandle<T> AssetManager::loadAssetSync(const std::string& path, const AssetLoadParams& params) {
    auto handle = createAsset<T>(path, params);
    handle.load(params);
    return handle;
}

} // namespace ecscope::assets