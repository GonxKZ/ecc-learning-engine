#pragma once

#include "asset.hpp"
#include "asset_loader.hpp"
#include "asset_registry.hpp"
#include "asset_cache.hpp"
#include "hot_reload.hpp"
#include "../core/thread_pool.hpp"
#include <unordered_map>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>

namespace ecscope::assets {

    // Asset manager configuration
    struct AssetManagerConfig {
        size_t max_memory_mb = 512;           // Maximum memory usage in MB
        size_t worker_threads = 4;            // Number of loading threads
        size_t cache_size_mb = 128;           // Asset cache size in MB
        bool enable_hot_reload = true;        // Enable hot reloading
        bool enable_compression = true;       // Enable asset compression
        bool enable_streaming = true;         // Enable asset streaming
        bool enable_memory_mapping = true;    // Enable memory-mapped files
        std::string asset_root = "assets/";   // Root asset directory
        std::string cache_directory = "cache/"; // Asset cache directory
    };

    // Load request with priority queue comparison
    struct LoadRequest {
        AssetID id;
        std::string path;
        AssetType type;
        LoadPriority priority;
        LoadFlags flags;
        QualityLevel quality;
        std::promise<AssetHandle> promise;
        std::chrono::steady_clock::time_point request_time;

        LoadRequest(AssetID id, const std::string& path, AssetType type, 
                   LoadPriority priority = Priority::NORMAL, 
                   LoadFlags flags = LoadFlags::NONE,
                   QualityLevel quality = QualityLevel::MEDIUM)
            : id(id), path(path), type(type), priority(priority), 
              flags(flags), quality(quality), 
              request_time(std::chrono::steady_clock::now()) {}

        bool operator<(const LoadRequest& other) const {
            if (priority != other.priority) {
                return priority < other.priority; // Higher priority first
            }
            return request_time > other.request_time; // Earlier requests first
        }
    };

    // Asset manager - central hub for all asset operations
    class AssetManager {
    public:
        explicit AssetManager(const AssetManagerConfig& config = AssetManagerConfig{});
        ~AssetManager();

        // Initialization and shutdown
        bool initialize();
        void shutdown();

        // Factory registration
        void register_factory(AssetType type, std::unique_ptr<AssetFactory> factory);
        void unregister_factory(AssetType type);

        // Asset loading - synchronous
        AssetHandle load_asset(const std::string& path, 
                              LoadPriority priority = Priority::NORMAL,
                              LoadFlags flags = LoadFlags::NONE,
                              QualityLevel quality = QualityLevel::MEDIUM);

        AssetHandle load_asset(AssetID id,
                              LoadPriority priority = Priority::NORMAL,
                              LoadFlags flags = LoadFlags::NONE,
                              QualityLevel quality = QualityLevel::MEDIUM);

        // Asset loading - asynchronous
        std::future<AssetHandle> load_asset_async(const std::string& path,
                                                 LoadPriority priority = Priority::NORMAL,
                                                 LoadFlags flags = LoadFlags::ASYNC,
                                                 QualityLevel quality = QualityLevel::MEDIUM);

        std::future<AssetHandle> load_asset_async(AssetID id,
                                                 LoadPriority priority = Priority::NORMAL,
                                                 LoadFlags flags = LoadFlags::ASYNC,
                                                 QualityLevel quality = QualityLevel::MEDIUM);

        // Asset loading with callback
        void load_asset_callback(const std::string& path,
                                std::function<void(AssetHandle)> callback,
                                LoadPriority priority = Priority::NORMAL,
                                LoadFlags flags = LoadFlags::ASYNC,
                                QualityLevel quality = QualityLevel::MEDIUM);

        // Batch loading
        std::vector<AssetHandle> load_assets_batch(const std::vector<std::string>& paths,
                                                  LoadPriority priority = Priority::NORMAL,
                                                  LoadFlags flags = LoadFlags::NONE);

        std::future<std::vector<AssetHandle>> load_assets_batch_async(
            const std::vector<std::string>& paths,
            LoadPriority priority = Priority::NORMAL,
            LoadFlags flags = LoadFlags::ASYNC);

        // Asset management
        void unload_asset(AssetID id);
        void unload_asset(const std::string& path);
        void reload_asset(AssetID id);
        void reload_asset(const std::string& path);

        // Asset queries
        AssetHandle get_asset(AssetID id);
        AssetHandle get_asset(const std::string& path);
        bool is_asset_loaded(AssetID id) const;
        bool is_asset_loaded(const std::string& path) const;

        // Asset discovery
        std::vector<AssetID> find_assets_by_type(AssetType type) const;
        std::vector<AssetID> find_assets_by_pattern(const std::string& pattern) const;
        std::vector<AssetID> get_all_assets() const;

        // Memory management
        void collect_garbage();
        void free_unused_assets();
        size_t get_memory_usage() const;
        size_t get_asset_count() const;

        // Streaming
        void set_streaming_quality(AssetID id, QualityLevel quality);
        void preload_assets(const std::vector<AssetID>& assets);
        void prefetch_assets(const std::vector<AssetID>& assets);

        // Configuration
        const AssetManagerConfig& get_config() const { return m_config; }
        void update_config(const AssetManagerConfig& config);

        // Statistics
        const LoadStatistics& get_load_statistics() const { return m_statistics; }
        void reset_statistics() { m_statistics.reset(); }

        // Debug and profiling
        void dump_asset_info() const;
        void dump_memory_usage() const;
        std::vector<AssetMetadata> get_asset_metadata() const;

        // Hot reload
        void enable_hot_reload(bool enable);
        bool is_hot_reload_enabled() const;
        void force_reload_all();

        // Thread safety
        std::mutex& get_mutex() { return m_mutex; }

    private:
        AssetManagerConfig m_config;
        
        // Core subsystems
        std::unique_ptr<AssetRegistry> m_registry;
        std::unique_ptr<AssetCache> m_cache;
        std::unique_ptr<AssetLoader> m_loader;
        std::unique_ptr<HotReloadSystem> m_hot_reload;
        
        // Threading
        std::unique_ptr<core::ThreadPool> m_thread_pool;
        std::priority_queue<LoadRequest> m_load_queue;
        std::mutex m_queue_mutex;
        std::condition_variable m_queue_cv;
        std::atomic<bool> m_running;

        // Asset factories
        std::unordered_map<AssetType, std::unique_ptr<AssetFactory>> m_factories;

        // Statistics
        mutable LoadStatistics m_statistics;

        // Thread safety
        mutable std::mutex m_mutex;
        mutable std::shared_mutex m_factories_mutex;

        // Internal methods
        void process_load_queue();
        AssetHandle load_asset_internal(const LoadRequest& request);
        std::shared_ptr<Asset> create_asset(AssetType type, AssetID id, const std::string& path);
        AssetType detect_asset_type(const std::string& path) const;
        void update_statistics(const LoadRequest& request, bool success, 
                              std::chrono::milliseconds load_time);
        
        // Worker threads
        std::vector<std::thread> m_worker_threads;
        void worker_thread_func();
    };

    // Global asset manager instance
    AssetManager& get_asset_manager();
    void set_asset_manager(std::unique_ptr<AssetManager> manager);

    // Convenience functions
    template<typename T>
    TypedAssetHandle<T> load_asset(const std::string& path) {
        return TypedAssetHandle<T>(get_asset_manager().load_asset(path));
    }

    template<typename T>
    std::future<TypedAssetHandle<T>> load_asset_async(const std::string& path) {
        auto future = get_asset_manager().load_asset_async(path);
        return std::async(std::launch::deferred, [future = std::move(future)]() mutable {
            return TypedAssetHandle<T>(future.get());
        });
    }

} // namespace ecscope::assets