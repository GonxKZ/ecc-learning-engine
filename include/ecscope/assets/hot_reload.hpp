#pragma once

#include "asset.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <chrono>

namespace ecscope::assets {

    // Forward declaration
    class AssetRegistry;

    // File change types
    enum class FileChangeType {
        CREATED,
        MODIFIED,
        DELETED,
        MOVED
    };

    // File change event
    struct FileChangeEvent {
        std::string path;
        FileChangeType type;
        std::chrono::system_clock::time_point timestamp;
        std::string old_path; // For MOVED events
        
        FileChangeEvent() : type(FileChangeType::MODIFIED), 
                           timestamp(std::chrono::system_clock::now()) {}
        
        FileChangeEvent(const std::string& file_path, FileChangeType change_type)
            : path(file_path), type(change_type), 
              timestamp(std::chrono::system_clock::now()) {}
    };

    // File system watcher interface
    class FileSystemWatcher {
    public:
        using ChangeCallback = std::function<void(const FileChangeEvent&)>;
        
        virtual ~FileSystemWatcher() = default;
        
        // Watch management
        virtual bool add_watch(const std::string& path, bool recursive = true) = 0;
        virtual bool remove_watch(const std::string& path) = 0;
        virtual void clear_watches() = 0;
        
        // Callback registration
        virtual void set_change_callback(ChangeCallback callback) = 0;
        
        // Watcher control
        virtual bool start() = 0;
        virtual void stop() = 0;
        virtual bool is_running() const = 0;
        
        // Configuration
        virtual void set_debounce_time(std::chrono::milliseconds time) = 0;
        virtual void add_ignore_pattern(const std::string& pattern) = 0;
        virtual void remove_ignore_pattern(const std::string& pattern) = 0;
    };

    // Platform-specific file system watchers
    #ifdef _WIN32
    class Win32FileSystemWatcher : public FileSystemWatcher {
    public:
        Win32FileSystemWatcher();
        ~Win32FileSystemWatcher();
        
        bool add_watch(const std::string& path, bool recursive = true) override;
        bool remove_watch(const std::string& path) override;
        void clear_watches() override;
        void set_change_callback(ChangeCallback callback) override;
        bool start() override;
        void stop() override;
        bool is_running() const override;
        void set_debounce_time(std::chrono::milliseconds time) override;
        void add_ignore_pattern(const std::string& pattern) override;
        void remove_ignore_pattern(const std::string& pattern) override;
        
    private:
        struct WatchData;
        std::vector<std::unique_ptr<WatchData>> m_watches;
        ChangeCallback m_callback;
        std::thread m_worker_thread;
        std::atomic<bool> m_running;
        std::chrono::milliseconds m_debounce_time;
        std::vector<std::string> m_ignore_patterns;
        mutable std::mutex m_mutex;
        
        void worker_thread_func();
        bool should_ignore_file(const std::string& path) const;
    };
    #endif

    #ifdef __linux__
    class InotifyFileSystemWatcher : public FileSystemWatcher {
    public:
        InotifyFileSystemWatcher();
        ~InotifyFileSystemWatcher();
        
        bool add_watch(const std::string& path, bool recursive = true) override;
        bool remove_watch(const std::string& path) override;
        void clear_watches() override;
        void set_change_callback(ChangeCallback callback) override;
        bool start() override;
        void stop() override;
        bool is_running() const override;
        void set_debounce_time(std::chrono::milliseconds time) override;
        void add_ignore_pattern(const std::string& pattern) override;
        void remove_ignore_pattern(const std::string& pattern) override;
        
    private:
        int m_inotify_fd;
        std::unordered_map<int, std::string> m_watch_descriptors;
        std::unordered_map<std::string, int> m_path_to_wd;
        ChangeCallback m_callback;
        std::thread m_worker_thread;
        std::atomic<bool> m_running;
        std::chrono::milliseconds m_debounce_time;
        std::vector<std::string> m_ignore_patterns;
        mutable std::mutex m_mutex;
        
        void worker_thread_func();
        bool should_ignore_file(const std::string& path) const;
        void add_recursive_watches(const std::string& path);
    };
    #endif

    #ifdef __APPLE__
    class FSEventsFileSystemWatcher : public FileSystemWatcher {
    public:
        FSEventsFileSystemWatcher();
        ~FSEventsFileSystemWatcher();
        
        bool add_watch(const std::string& path, bool recursive = true) override;
        bool remove_watch(const std::string& path) override;
        void clear_watches() override;
        void set_change_callback(ChangeCallback callback) override;
        bool start() override;
        void stop() override;
        bool is_running() const override;
        void set_debounce_time(std::chrono::milliseconds time) override;
        void add_ignore_pattern(const std::string& pattern) override;
        void remove_ignore_pattern(const std::string& pattern) override;
        
    private:
        struct FSEventsData;
        std::unique_ptr<FSEventsData> m_data;
        ChangeCallback m_callback;
        std::atomic<bool> m_running;
        std::chrono::milliseconds m_debounce_time;
        std::vector<std::string> m_ignore_patterns;
        mutable std::mutex m_mutex;
        
        bool should_ignore_file(const std::string& path) const;
    };
    #endif

    // Cross-platform file system watcher factory
    std::unique_ptr<FileSystemWatcher> create_file_system_watcher();

    // Hot reload system
    class HotReloadSystem {
    public:
        explicit HotReloadSystem(AssetRegistry* registry);
        ~HotReloadSystem();
        
        // System control
        bool initialize(const std::string& watch_directory);
        void shutdown();
        bool is_enabled() const { return m_enabled.load(); }
        void set_enabled(bool enabled);
        
        // Watch management
        bool add_watch_path(const std::string& path, bool recursive = true);
        bool remove_watch_path(const std::string& path);
        void clear_watch_paths();
        std::vector<std::string> get_watch_paths() const;
        
        // Asset tracking
        void register_asset_path(AssetID id, const std::string& path);
        void unregister_asset_path(AssetID id);
        void register_dependency(const std::string& path, const std::vector<std::string>& dependencies);
        
        // Reload callbacks
        using ReloadCallback = std::function<void(AssetID, const std::string&)>;
        void set_reload_callback(ReloadCallback callback) { m_reload_callback = callback; }
        
        // Manual reload
        void force_reload(AssetID id);
        void force_reload(const std::string& path);
        void force_reload_all();
        
        // Configuration
        void set_debounce_time(std::chrono::milliseconds time);
        void set_batch_reload_enabled(bool enabled) { m_batch_reload_enabled = enabled; }
        void set_batch_time(std::chrono::milliseconds time) { m_batch_time = time; }
        
        // Ignore patterns
        void add_ignore_pattern(const std::string& pattern);
        void remove_ignore_pattern(const std::string& pattern);
        void clear_ignore_patterns();
        
        // Statistics
        struct HotReloadStats {
            std::atomic<uint64_t> files_watched{0};
            std::atomic<uint64_t> reload_events{0};
            std::atomic<uint64_t> successful_reloads{0};
            std::atomic<uint64_t> failed_reloads{0};
            std::atomic<uint64_t> ignored_events{0};
            
            void reset() {
                files_watched = 0;
                reload_events = 0;
                successful_reloads = 0;
                failed_reloads = 0;
                ignored_events = 0;
            }
        };
        
        const HotReloadStats& get_statistics() const { return m_stats; }
        void reset_statistics() { m_stats.reset(); }
        
        // Version management
        void create_backup(AssetID id);
        bool restore_backup(AssetID id);
        void clear_backups();
        
        // Debugging
        void dump_watched_files() const;
        void dump_asset_mappings() const;
        
    private:
        AssetRegistry* m_registry;
        std::unique_ptr<FileSystemWatcher> m_watcher;
        
        // Configuration
        std::atomic<bool> m_enabled{false};
        std::atomic<bool> m_batch_reload_enabled{true};
        std::chrono::milliseconds m_batch_time{100};
        
        // Asset tracking
        std::unordered_map<std::string, AssetID> m_path_to_asset;
        std::unordered_map<AssetID, std::string> m_asset_to_path;
        std::unordered_map<std::string, std::vector<std::string>> m_dependencies;
        std::unordered_map<std::string, std::chrono::system_clock::time_point> m_last_modified;
        
        // Batched reload system
        std::unordered_set<std::string> m_pending_reloads;
        std::thread m_batch_thread;
        std::mutex m_batch_mutex;
        std::condition_variable m_batch_cv;
        std::atomic<bool> m_batch_running{false};
        
        // Callbacks
        ReloadCallback m_reload_callback;
        
        // Statistics
        mutable HotReloadStats m_stats;
        
        // Backup system
        std::unordered_map<AssetID, std::vector<uint8_t>> m_backups;
        
        // Thread safety
        mutable std::shared_mutex m_mutex;
        
        // Internal methods
        void on_file_changed(const FileChangeEvent& event);
        void process_file_change(const std::string& path, FileChangeType type);
        void schedule_reload(const std::string& path);
        void batch_thread_func();
        bool should_ignore_file(const std::string& path) const;
        void reload_asset_internal(const std::string& path);
        void reload_dependencies(const std::string& path);
        std::string normalize_path(const std::string& path) const;
    };

    // Hot reload configuration
    struct HotReloadConfig {
        bool enabled = true;
        std::chrono::milliseconds debounce_time{100};
        bool batch_reload_enabled = true;
        std::chrono::milliseconds batch_time{100};
        bool enable_backups = true;
        std::vector<std::string> ignore_patterns = {
            "*.tmp", "*.temp", "*~", ".DS_Store", "Thumbs.db"
        };
        std::vector<std::string> watch_extensions = {
            ".png", ".jpg", ".jpeg", ".bmp", ".tga", ".dds",  // Textures
            ".obj", ".fbx", ".gltf", ".glb", ".dae",          // Models
            ".wav", ".mp3", ".ogg", ".flac",                  // Audio
            ".glsl", ".hlsl", ".spv",                         // Shaders
            ".json", ".xml", ".yaml", ".ini"                  // Config
        };
    };

    // Network-based hot reload for distributed development
    class NetworkHotReload {
    public:
        NetworkHotReload();
        ~NetworkHotReload();
        
        // Server mode (asset server)
        bool start_server(uint16_t port = 8080);
        void stop_server();
        bool is_server_running() const;
        
        // Client mode (game instances)
        bool connect_to_server(const std::string& host, uint16_t port = 8080);
        void disconnect_from_server();
        bool is_connected_to_server() const;
        
        // Asset distribution
        void broadcast_asset_change(AssetID id, const std::vector<uint8_t>& data);
        void request_asset_update(AssetID id);
        
        // Callbacks
        using NetworkReloadCallback = std::function<void(AssetID, const std::vector<uint8_t>&)>;
        void set_network_reload_callback(NetworkReloadCallback callback);
        
        // Statistics
        struct NetworkStats {
            std::atomic<uint64_t> bytes_sent{0};
            std::atomic<uint64_t> bytes_received{0};
            std::atomic<uint64_t> assets_distributed{0};
            std::atomic<uint64_t> connection_count{0};
        };
        
        const NetworkStats& get_network_statistics() const { return m_network_stats; }
        
    private:
        struct NetworkData;
        std::unique_ptr<NetworkData> m_data;
        NetworkStats m_network_stats;
        NetworkReloadCallback m_network_callback;
        
        void handle_client_connection();
        void handle_server_messages();
        void send_asset_update(AssetID id, const std::vector<uint8_t>& data);
    };

    // Hot reload factory
    std::unique_ptr<HotReloadSystem> create_hot_reload_system(AssetRegistry* registry, 
                                                             const HotReloadConfig& config = HotReloadConfig{});

} // namespace ecscope::assets