#pragma once

#include "../core/asset_types.hpp"
#include <filesystem>
#include <thread>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <functional>
#include <chrono>

namespace ecscope::assets {

// =============================================================================
// File System Events
// =============================================================================

enum class FileEvent : uint8_t {
    Created,
    Modified,
    Deleted,
    Renamed,
    AttributeChanged
};

struct FileChangeEvent {
    std::filesystem::path path;
    std::filesystem::path old_path; // For rename events
    FileEvent event_type;
    std::chrono::system_clock::time_point timestamp;
    uint64_t file_size = 0;
    
    FileChangeEvent(const std::filesystem::path& p, FileEvent type)
        : path(p), event_type(type), timestamp(std::chrono::system_clock::now()) {}
};

// =============================================================================
// File Watch Filter
// =============================================================================

class FileWatchFilter {
public:
    FileWatchFilter() = default;
    
    // Extension filtering
    void addExtension(const std::string& extension);
    void removeExtension(const std::string& extension);
    void setExtensions(const std::vector<std::string>& extensions);
    
    // Pattern filtering (glob-style)
    void addPattern(const std::string& pattern);
    void removePattern(const std::string& pattern);
    
    // Directory filtering
    void addIgnoredDirectory(const std::string& directory);
    void removeIgnoredDirectory(const std::string& directory);
    
    // Size filtering
    void setMinFileSize(uint64_t size) { min_file_size_ = size; }
    void setMaxFileSize(uint64_t size) { max_file_size_ = size; }
    
    // Check if file passes filter
    bool shouldWatch(const std::filesystem::path& path, uint64_t file_size = 0) const;
    bool shouldIgnore(const std::filesystem::path& path) const;
    
private:
    std::unordered_set<std::string> watched_extensions_;
    std::vector<std::string> watch_patterns_;
    std::unordered_set<std::string> ignored_directories_;
    uint64_t min_file_size_ = 0;
    uint64_t max_file_size_ = UINT64_MAX;
    
    bool matchesPattern(const std::string& path, const std::string& pattern) const;
};

// =============================================================================
// File System Watcher
// =============================================================================

class FileSystemWatcher {
public:
    using EventCallback = std::function<void(const FileChangeEvent&)>;
    
    FileSystemWatcher();
    ~FileSystemWatcher();
    
    // Non-copyable, non-movable
    FileSystemWatcher(const FileSystemWatcher&) = delete;
    FileSystemWatcher& operator=(const FileSystemWatcher&) = delete;
    
    // Watch management
    bool addWatch(const std::filesystem::path& path, bool recursive = true);
    bool removeWatch(const std::filesystem::path& path);
    void clearWatches();
    
    // Callback management
    void setEventCallback(EventCallback callback) { event_callback_ = std::move(callback); }
    
    // Filter management
    void setFilter(std::unique_ptr<FileWatchFilter> filter) { filter_ = std::move(filter); }
    const FileWatchFilter* getFilter() const { return filter_.get(); }
    
    // Control
    void start();
    void stop();
    bool isRunning() const { return running_; }
    
    // Configuration
    void setPollingInterval(std::chrono::milliseconds interval) { polling_interval_ = interval; }
    void setDebounceTime(std::chrono::milliseconds time) { debounce_time_ = time; }
    
    // Statistics
    struct Statistics {
        uint64_t events_processed = 0;
        uint64_t events_filtered = 0;
        uint64_t events_debounced = 0;
        std::chrono::steady_clock::time_point start_time;
    };
    
    Statistics getStatistics() const;
    void resetStatistics();
    
private:
    struct WatchEntry {
        std::filesystem::path path;
        bool recursive;
        std::filesystem::file_time_type last_write_time;
        uint64_t file_size;
    };
    
    struct PendingEvent {
        FileChangeEvent event;
        std::chrono::steady_clock::time_point debounce_time;
    };
    
    void watchThread();
    void processEvents();
    void scanDirectory(const std::filesystem::path& dir, bool recursive);
    void checkFileChanges();
    void debounceEvent(FileChangeEvent event);
    void processDebounceQueue();
    
    // Platform-specific implementation
    bool initializePlatformWatcher();
    void shutdownPlatformWatcher();
    bool addPlatformWatch(const std::filesystem::path& path, bool recursive);
    bool removePlatformWatch(const std::filesystem::path& path);
    
    std::thread watch_thread_;
    std::atomic<bool> running_;
    std::atomic<bool> shutdown_requested_;
    
    EventCallback event_callback_;
    std::unique_ptr<FileWatchFilter> filter_;
    
    // Watched paths and their state
    std::unordered_map<std::string, WatchEntry> watched_paths_;
    mutable std::mutex watched_paths_mutex_;
    
    // File system state tracking
    std::unordered_map<std::string, std::filesystem::file_time_type> file_timestamps_;
    std::unordered_map<std::string, uint64_t> file_sizes_;
    mutable std::mutex file_state_mutex_;
    
    // Event debouncing
    std::vector<PendingEvent> pending_events_;
    std::mutex pending_events_mutex_;
    
    // Configuration
    std::chrono::milliseconds polling_interval_{100};
    std::chrono::milliseconds debounce_time_{50};
    
    // Statistics
    mutable std::mutex stats_mutex_;
    Statistics stats_;
    
    // Platform-specific data
    void* platform_data_ = nullptr;
};

// =============================================================================
// Asset Hot Reload Manager
// =============================================================================

class HotReloadManager {
public:
    using ReloadCallback = std::function<void(AssetID, const std::string&)>;
    
    HotReloadManager();
    ~HotReloadManager();
    
    // Non-copyable, non-movable
    HotReloadManager(const HotReloadManager&) = delete;
    HotReloadManager& operator=(const HotReloadManager&) = delete;
    
    // Asset path registration
    void registerAsset(AssetID asset_id, const std::filesystem::path& path);
    void unregisterAsset(AssetID asset_id);
    void unregisterPath(const std::filesystem::path& path);
    
    // Watch directory setup
    void addWatchDirectory(const std::filesystem::path& directory, bool recursive = true);
    void removeWatchDirectory(const std::filesystem::path& directory);
    
    // Reload callbacks
    void setReloadCallback(ReloadCallback callback) { reload_callback_ = std::move(callback); }
    
    // Control
    void enable() { enabled_ = true; startWatching(); }
    void disable() { enabled_ = false; stopWatching(); }
    bool isEnabled() const { return enabled_; }
    
    // Manual reload triggering
    void triggerReload(AssetID asset_id);
    void triggerReloadAll();
    
    // Configuration
    struct Config {
        bool auto_reload = true;
        std::chrono::milliseconds reload_delay{100};
        bool reload_dependencies = true;
        bool validate_before_reload = true;
        uint32_t max_retry_attempts = 3;
        std::chrono::milliseconds retry_delay{1000};
    };
    
    void setConfig(const Config& config) { config_ = config; }
    const Config& getConfig() const { return config_; }
    
    // File filters
    void setFileFilter(std::unique_ptr<FileWatchFilter> filter);
    
    // Statistics and monitoring
    struct Statistics {
        uint64_t files_watched = 0;
        uint64_t assets_reloaded = 0;
        uint64_t reload_failures = 0;
        uint64_t dependency_reloads = 0;
        std::chrono::steady_clock::time_point last_reload;
    };
    
    Statistics getStatistics() const;
    void resetStatistics();
    
    // Network hot-reload support for remote assets
    void enableNetworkReload(uint16_t port = 8080);
    void disableNetworkReload();
    bool isNetworkReloadEnabled() const { return network_enabled_; }
    
private:
    struct AssetEntry {
        AssetID asset_id;
        std::filesystem::path path;
        std::chrono::system_clock::time_point last_reload;
        uint32_t reload_count = 0;
        bool pending_reload = false;
    };
    
    struct ReloadTask {
        AssetID asset_id;
        std::filesystem::path path;
        std::chrono::steady_clock::time_point scheduled_time;
        uint32_t retry_count = 0;
    };
    
    void startWatching();
    void stopWatching();
    void onFileChanged(const FileChangeEvent& event);
    void processReloadQueue();
    void executeReload(const ReloadTask& task);
    
    bool validateAssetFile(const std::filesystem::path& path);
    std::vector<AssetID> getDependentAssets(AssetID asset_id);
    
    // Network reload support
    void networkThread();
    void handleNetworkReload(const std::string& message);
    
    std::unique_ptr<FileSystemWatcher> watcher_;
    ReloadCallback reload_callback_;
    
    // Asset tracking
    std::unordered_map<AssetID, AssetEntry> tracked_assets_;
    std::unordered_map<std::string, AssetID> path_to_asset_;
    mutable std::mutex assets_mutex_;
    
    // Reload queue
    std::vector<ReloadTask> reload_queue_;
    std::mutex reload_queue_mutex_;
    std::condition_variable reload_condition_;
    std::thread reload_thread_;
    
    // Configuration and state
    Config config_;
    std::atomic<bool> enabled_;
    std::atomic<bool> shutdown_requested_;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    Statistics stats_;
    
    // Network hot-reload
    std::atomic<bool> network_enabled_;
    std::thread network_thread_;
    uint16_t network_port_ = 8080;
    void* network_socket_ = nullptr; // Platform-specific socket
};

// =============================================================================
// Asset Version Manager
// =============================================================================

class AssetVersionManager {
public:
    struct VersionInfo {
        AssetVersion version;
        std::chrono::system_clock::time_point timestamp;
        std::string checksum;
        uint64_t file_size;
        std::string path;
    };
    
    AssetVersionManager();
    ~AssetVersionManager();
    
    // Version tracking
    AssetVersion getCurrentVersion(AssetID asset_id) const;
    AssetVersion getLatestVersion(AssetID asset_id) const;
    std::vector<VersionInfo> getVersionHistory(AssetID asset_id) const;
    
    // Version management
    AssetVersion addVersion(AssetID asset_id, const std::filesystem::path& path);
    bool rollbackToVersion(AssetID asset_id, AssetVersion version);
    void pruneOldVersions(AssetID asset_id, uint32_t max_versions = 10);
    
    // Version validation
    bool isVersionValid(AssetID asset_id, AssetVersion version) const;
    bool hasNewerVersion(AssetID asset_id, AssetVersion version) const;
    
    // Checksum management
    std::string calculateChecksum(const std::filesystem::path& path) const;
    bool validateChecksum(AssetID asset_id, AssetVersion version) const;
    
private:
    struct AssetVersions {
        std::vector<VersionInfo> versions;
        AssetVersion next_version = 1;
    };
    
    std::unordered_map<AssetID, AssetVersions> asset_versions_;
    mutable std::mutex versions_mutex_;
    
    std::string computeFileHash(const std::filesystem::path& path) const;
};

} // namespace ecscope::assets