#pragma once

/**
 * @file plugin/plugin_manager.hpp
 * @brief ECScope Plugin Manager - Complete Plugin Lifecycle Management
 * 
 * Comprehensive plugin management system providing dynamic loading/unloading,
 * dependency resolution, security enforcement, hot-swapping, and educational
 * features. This is the central orchestrator for the entire plugin ecosystem.
 * 
 * Key Features:
 * - Automatic dependency resolution and loading order
 * - Hot-swappable plugins with state preservation
 * - Comprehensive security and sandboxing
 * - Plugin discovery and automatic updates
 * - Performance monitoring and optimization
 * - Educational visualization and debugging
 * - Cross-platform compatibility (Windows/Linux/macOS)
 * 
 * @author ECScope Educational ECS Framework - Plugin System
 * @date 2024
 */

#include "plugin_core.hpp"
#include "plugin_registry.hpp"
#include "plugin_security.hpp"
#include "core/types.hpp"
#include "core/log.hpp"
#include "memory/arena.hpp"
#include "memory/memory_tracker.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <future>
#include <queue>
#include <filesystem>

namespace ecscope::plugin {

//=============================================================================
// Plugin Manager Configuration
//=============================================================================

/**
 * @brief Configuration for plugin manager behavior
 */
struct PluginManagerConfig {
    // Directory Management
    std::vector<std::string> plugin_directories{
        "./plugins",
        "./plugins/core",
        "./plugins/extensions",
        "./plugins/educational"
    };
    std::string cache_directory{"./cache/plugins"};
    std::string temp_directory{"./temp/plugins"};
    
    // Loading Behavior
    bool auto_discover_plugins{true};
    bool auto_load_compatible_plugins{true};
    bool enable_hot_reload{true};
    bool enable_lazy_loading{false};
    bool enable_parallel_loading{true};
    u32 max_parallel_loads{4};
    
    // Security Settings
    bool enable_security_validation{true};
    bool require_plugin_signatures{false};
    bool enable_sandboxing{true};
    bool allow_untrusted_plugins{false};
    std::string trusted_publishers_file{"trusted_publishers.json"};
    
    // Performance and Resource Management
    usize max_total_plugin_memory{512 * MB};
    u32 max_plugins_loaded{100};
    std::chrono::milliseconds plugin_timeout{10000};
    bool enable_memory_monitoring{true};
    bool enable_performance_profiling{true};
    
    // Update and Maintenance
    bool enable_auto_updates{false};
    std::string update_server_url;
    std::chrono::hours update_check_interval{24};
    bool backup_plugins_before_update{true};
    
    // Educational Features
    bool enable_educational_mode{true};
    bool verbose_logging{true};
    bool track_learning_progress{true};
    bool generate_documentation{true};
    
    // Error Handling
    u32 max_load_retries{3};
    std::chrono::seconds retry_delay{5};
    bool quarantine_failed_plugins{true};
    bool continue_on_load_failure{true};
    
    /**
     * @brief Factory methods for common configurations
     */
    static PluginManagerConfig create_development() {
        PluginManagerConfig config;
        config.enable_educational_mode = true;
        config.verbose_logging = true;
        config.enable_security_validation = false;
        config.enable_hot_reload = true;
        config.enable_parallel_loading = false; // Easier debugging
        return config;
    }
    
    static PluginManagerConfig create_production() {
        PluginManagerConfig config;
        config.enable_educational_mode = false;
        config.verbose_logging = false;
        config.enable_security_validation = true;
        config.require_plugin_signatures = true;
        config.allow_untrusted_plugins = false;
        config.enable_parallel_loading = true;
        return config;
    }
    
    static PluginManagerConfig create_educational() {
        PluginManagerConfig config;
        config.enable_educational_mode = true;
        config.verbose_logging = true;
        config.track_learning_progress = true;
        config.generate_documentation = true;
        config.enable_hot_reload = true;
        return config;
    }
};

//=============================================================================
// Plugin Discovery and Metadata
//=============================================================================

/**
 * @brief Plugin discovery result
 */
struct PluginDiscoveryResult {
    std::string file_path;
    PluginMetadata metadata;
    bool is_valid{false};
    std::string error_message;
    f64 discovery_time_ms{0.0};
};

/**
 * @brief Plugin loading queue entry
 */
struct PluginLoadQueueEntry {
    std::string plugin_name;
    std::string file_path;
    PluginPriority priority;
    std::vector<std::string> dependencies;
    bool is_hot_reload{false};
    std::chrono::system_clock::time_point queued_time;
};

//=============================================================================
// Plugin Manager Statistics and Monitoring
//=============================================================================

/**
 * @brief Comprehensive plugin manager statistics
 */
struct PluginManagerStats {
    // Plugin Counts
    u32 total_plugins_discovered{0};
    u32 plugins_loaded{0};
    u32 plugins_active{0};
    u32 plugins_failed{0};
    u32 plugins_quarantined{0};
    
    // Performance Metrics
    f64 total_load_time_ms{0.0};
    f64 average_load_time_ms{0.0};
    f64 discovery_time_ms{0.0};
    f64 dependency_resolution_time_ms{0.0};
    
    // Memory Usage
    usize total_plugin_memory_usage{0};
    usize peak_plugin_memory_usage{0};
    usize manager_overhead{0};
    
    // Hot Reload Statistics
    u32 hot_reloads_performed{0};
    u32 hot_reload_failures{0};
    f64 average_hot_reload_time_ms{0.0};
    
    // Security Statistics
    u32 security_violations_detected{0};
    u32 plugins_quarantined_for_security{0};
    u32 signature_verification_failures{0};
    
    // Educational Metrics
    u32 learning_sessions_started{0};
    u32 documentation_requests{0};
    f64 average_plugin_complexity_score{0.0};
    
    void reset() {
        *this = PluginManagerStats{};
    }
    
    /**
     * @brief Calculate derived metrics
     */
    void update_derived_metrics() {
        if (total_plugins_discovered > 0) {
            average_load_time_ms = total_load_time_ms / total_plugins_discovered;
        }
        
        if (hot_reloads_performed > 0) {
            // Hot reload time would be tracked separately
        }
    }
};

//=============================================================================
// Plugin Manager Events
//=============================================================================

/**
 * @brief Plugin manager event types
 */
enum class PluginManagerEventType {
    PluginDiscovered,
    PluginLoaded,
    PluginUnloaded,
    PluginFailed,
    DependencyResolved,
    HotReloadStarted,
    HotReloadCompleted,
    SecurityViolation,
    MemoryLimitExceeded,
    UpdateAvailable,
    ConfigurationChanged
};

/**
 * @brief Plugin manager event data
 */
struct PluginManagerEvent {
    PluginManagerEventType type;
    std::string plugin_name;
    std::chrono::system_clock::time_point timestamp;
    std::string message;
    std::unordered_map<std::string, std::string> details;
    
    PluginManagerEvent(PluginManagerEventType t, const std::string& name, const std::string& msg = "")
        : type(t), plugin_name(name), timestamp(std::chrono::system_clock::now()), message(msg) {}
};

/**
 * @brief Plugin manager event handler
 */
using PluginManagerEventHandler = std::function<void(const PluginManagerEvent&)>;

//=============================================================================
// Main Plugin Manager Class
//=============================================================================

/**
 * @brief Comprehensive Plugin Manager for ECScope
 * 
 * The PluginManager is the central orchestrator for the entire plugin ecosystem.
 * It provides complete lifecycle management, security, performance monitoring,
 * and educational features for all plugins in the system.
 * 
 * Key Responsibilities:
 * - Plugin discovery and metadata validation
 * - Dependency resolution and loading order optimization
 * - Security validation and sandboxing enforcement
 * - Hot-swapping with state preservation
 * - Performance monitoring and resource management
 * - Educational visualization and debugging
 * - Automatic updates and maintenance
 */
class PluginManager {
private:
    // Configuration and State
    PluginManagerConfig config_;
    std::atomic<bool> is_initialized_{false};
    std::atomic<bool> is_shutting_down_{false};
    
    // Plugin Storage and Management
    std::unordered_map<std::string, std::unique_ptr<PluginContainer>> loaded_plugins_;
    std::unordered_map<std::string, PluginDiscoveryResult> discovered_plugins_;
    std::unordered_map<std::string, PluginMetadata> plugin_metadata_cache_;
    
    // Loading and Dependency Management
    std::queue<PluginLoadQueueEntry> load_queue_;
    std::unordered_map<std::string, std::unordered_set<std::string>> dependency_graph_;
    std::unordered_map<std::string, std::vector<std::string>> reverse_dependency_graph_;
    std::vector<std::string> load_order_;
    
    // Security and Sandboxing
    std::unique_ptr<PluginSecurityManager> security_manager_;
    std::unordered_set<std::string> trusted_plugins_;
    std::unordered_set<std::string> quarantined_plugins_;
    
    // Threading and Synchronization
    mutable std::shared_mutex plugins_mutex_;
    mutable std::shared_mutex load_queue_mutex_;
    std::thread discovery_thread_;
    std::thread update_thread_;
    std::vector<std::thread> loading_threads_;
    std::atomic<bool> should_stop_threads_{false};
    
    // Performance and Monitoring
    PluginManagerStats stats_;
    std::unique_ptr<memory::ArenaAllocator> manager_arena_;
    memory::MemoryTracker::ScopeTracker memory_tracker_;
    
    // Event System
    std::vector<PluginManagerEventHandler> event_handlers_;
    mutable std::mutex event_handlers_mutex_;
    
    // Hot Reload Support
    std::unordered_map<std::string, std::filesystem::file_time_type> plugin_file_times_;
    std::unordered_map<std::string, std::string> plugin_state_backups_;
    std::thread file_watcher_thread_;
    
    // Update System
    std::unordered_map<std::string, PluginVersion> available_updates_;
    std::thread update_checker_thread_;

public:
    /**
     * @brief Construct plugin manager with configuration
     */
    explicit PluginManager(const PluginManagerConfig& config = PluginManagerConfig::create_educational());
    
    /**
     * @brief Destructor with comprehensive cleanup
     */
    ~PluginManager();
    
    // Non-copyable but movable
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;
    PluginManager(PluginManager&&) = default;
    PluginManager& operator=(PluginManager&&) = default;
    
    //-------------------------------------------------------------------------
    // Initialization and Configuration
    //-------------------------------------------------------------------------
    
    /**
     * @brief Initialize plugin manager
     */
    bool initialize();
    
    /**
     * @brief Shutdown plugin manager
     */
    void shutdown();
    
    /**
     * @brief Update configuration
     */
    void set_config(const PluginManagerConfig& config);
    
    /**
     * @brief Get current configuration
     */
    const PluginManagerConfig& get_config() const { return config_; }
    
    /**
     * @brief Check if manager is initialized
     */
    bool is_initialized() const { return is_initialized_.load(); }
    
    //-------------------------------------------------------------------------
    // Plugin Discovery and Loading
    //-------------------------------------------------------------------------
    
    /**
     * @brief Discover all plugins in configured directories
     */
    std::vector<PluginDiscoveryResult> discover_plugins();
    
    /**
     * @brief Discover plugins in specific directory
     */
    std::vector<PluginDiscoveryResult> discover_plugins_in_directory(const std::string& directory);
    
    /**
     * @brief Load plugin by name
     */
    PluginLoadResult load_plugin(const std::string& plugin_name);
    
    /**
     * @brief Load plugin from file
     */
    PluginLoadResult load_plugin_from_file(const std::string& file_path);
    
    /**
     * @brief Load all discovered plugins
     */
    std::vector<PluginLoadResult> load_all_plugins();
    
    /**
     * @brief Load plugins with specific category
     */
    std::vector<PluginLoadResult> load_plugins_by_category(PluginCategory category);
    
    /**
     * @brief Unload plugin by name
     */
    bool unload_plugin(const std::string& plugin_name);
    
    /**
     * @brief Unload all plugins
     */
    void unload_all_plugins();
    
    //-------------------------------------------------------------------------
    // Plugin Management and Control
    //-------------------------------------------------------------------------
    
    /**
     * @brief Get loaded plugin by name
     */
    PluginContainer* get_plugin(const std::string& plugin_name);
    
    /**
     * @brief Check if plugin is loaded
     */
    bool is_plugin_loaded(const std::string& plugin_name) const;
    
    /**
     * @brief Get all loaded plugin names
     */
    std::vector<std::string> get_loaded_plugin_names() const;
    
    /**
     * @brief Get plugins by category
     */
    std::vector<std::string> get_plugins_by_category(PluginCategory category) const;
    
    /**
     * @brief Get plugin metadata
     */
    std::optional<PluginMetadata> get_plugin_metadata(const std::string& plugin_name) const;
    
    /**
     * @brief Update plugin (frame-based)
     */
    void update_plugins(f64 delta_time);
    
    /**
     * @brief Send event to all plugins
     */
    void broadcast_event(const PluginEvent& event);
    
    /**
     * @brief Send event to specific plugin
     */
    void send_event_to_plugin(const std::string& plugin_name, const PluginEvent& event);
    
    //-------------------------------------------------------------------------
    // Hot Reload and Dynamic Updates
    //-------------------------------------------------------------------------
    
    /**
     * @brief Enable hot reload for specific plugin
     */
    bool enable_hot_reload(const std::string& plugin_name);
    
    /**
     * @brief Disable hot reload for specific plugin
     */
    void disable_hot_reload(const std::string& plugin_name);
    
    /**
     * @brief Perform hot reload of plugin
     */
    bool hot_reload_plugin(const std::string& plugin_name);
    
    /**
     * @brief Check for plugin file changes
     */
    std::vector<std::string> check_for_plugin_changes();
    
    /**
     * @brief Backup plugin state for hot reload
     */
    bool backup_plugin_state(const std::string& plugin_name);
    
    /**
     * @brief Restore plugin state after hot reload
     */
    bool restore_plugin_state(const std::string& plugin_name);
    
    //-------------------------------------------------------------------------
    // Dependency Management
    //-------------------------------------------------------------------------
    
    /**
     * @brief Resolve plugin dependencies
     */
    std::vector<std::string> resolve_dependencies(const std::string& plugin_name);
    
    /**
     * @brief Get plugin load order considering dependencies
     */
    std::vector<std::string> get_load_order();
    
    /**
     * @brief Check for circular dependencies
     */
    bool has_circular_dependencies() const;
    
    /**
     * @brief Get dependency graph
     */
    std::unordered_map<std::string, std::vector<std::string>> get_dependency_graph() const;
    
    /**
     * @brief Validate all plugin dependencies
     */
    bool validate_all_dependencies() const;
    
    //-------------------------------------------------------------------------
    // Security and Sandboxing
    //-------------------------------------------------------------------------
    
    /**
     * @brief Set security context for plugin
     */
    bool set_plugin_security_context(const std::string& plugin_name, 
                                   const PluginSecurityContext& context);
    
    /**
     * @brief Get plugin security context
     */
    std::optional<PluginSecurityContext> get_plugin_security_context(const std::string& plugin_name) const;
    
    /**
     * @brief Quarantine plugin due to security violation
     */
    void quarantine_plugin(const std::string& plugin_name, const std::string& reason);
    
    /**
     * @brief Remove plugin from quarantine
     */
    bool remove_from_quarantine(const std::string& plugin_name);
    
    /**
     * @brief Get quarantined plugins
     */
    std::vector<std::string> get_quarantined_plugins() const;
    
    /**
     * @brief Validate plugin signature
     */
    bool validate_plugin_signature(const std::string& plugin_name) const;
    
    //-------------------------------------------------------------------------
    // Performance and Resource Management
    //-------------------------------------------------------------------------
    
    /**
     * @brief Get plugin manager statistics
     */
    const PluginManagerStats& get_statistics() const { return stats_; }
    
    /**
     * @brief Get memory usage by plugin
     */
    usize get_plugin_memory_usage(const std::string& plugin_name) const;
    
    /**
     * @brief Get total memory usage of all plugins
     */
    usize get_total_plugin_memory_usage() const;
    
    /**
     * @brief Check if memory limit is exceeded
     */
    bool is_memory_limit_exceeded() const;
    
    /**
     * @brief Get plugin performance metrics
     */
    std::unordered_map<std::string, PluginStats> get_all_plugin_stats() const;
    
    /**
     * @brief Generate performance report
     */
    std::string generate_performance_report() const;
    
    //-------------------------------------------------------------------------
    // Update System
    //-------------------------------------------------------------------------
    
    /**
     * @brief Check for plugin updates
     */
    std::unordered_map<std::string, PluginVersion> check_for_updates();
    
    /**
     * @brief Update specific plugin
     */
    bool update_plugin(const std::string& plugin_name);
    
    /**
     * @brief Update all plugins
     */
    std::vector<std::pair<std::string, bool>> update_all_plugins();
    
    /**
     * @brief Get available updates
     */
    std::unordered_map<std::string, PluginVersion> get_available_updates() const;
    
    /**
     * @brief Download plugin update
     */
    bool download_plugin_update(const std::string& plugin_name, const PluginVersion& version);
    
    //-------------------------------------------------------------------------
    // Event System
    //-------------------------------------------------------------------------
    
    /**
     * @brief Add event handler
     */
    void add_event_handler(const PluginManagerEventHandler& handler);
    
    /**
     * @brief Remove all event handlers
     */
    void clear_event_handlers();
    
    /**
     * @brief Fire plugin manager event
     */
    void fire_event(const PluginManagerEvent& event);
    
    //-------------------------------------------------------------------------
    // Educational Features
    //-------------------------------------------------------------------------
    
    /**
     * @brief Generate plugin documentation
     */
    std::string generate_plugin_documentation(const std::string& plugin_name) const;
    
    /**
     * @brief Get learning resources for plugin
     */
    std::vector<std::string> get_plugin_learning_resources(const std::string& plugin_name) const;
    
    /**
     * @brief Generate plugin architecture diagram
     */
    std::string generate_architecture_diagram() const;
    
    /**
     * @brief Get plugin complexity analysis
     */
    struct PluginComplexityAnalysis {
        std::string plugin_name;
        u32 complexity_score; // 0-100
        std::vector<std::string> complexity_factors;
        std::string difficulty_level;
        std::vector<std::string> recommendations;
    };
    
    PluginComplexityAnalysis analyze_plugin_complexity(const std::string& plugin_name) const;
    
    /**
     * @brief Start educational learning session
     */
    void start_learning_session(const std::string& topic);
    
    /**
     * @brief Get educational progress
     */
    std::unordered_map<std::string, f32> get_learning_progress() const;

private:
    //-------------------------------------------------------------------------
    // Internal Implementation
    //-------------------------------------------------------------------------
    
    /**
     * @brief Initialize directory structure
     */
    void initialize_directories();
    
    /**
     * @brief Initialize security manager
     */
    void initialize_security_manager();
    
    /**
     * @brief Initialize worker threads
     */
    void initialize_threads();
    
    /**
     * @brief Shutdown worker threads
     */
    void shutdown_threads();
    
    /**
     * @brief Background plugin discovery thread
     */
    void discovery_thread_worker();
    
    /**
     * @brief Background update checker thread
     */
    void update_thread_worker();
    
    /**
     * @brief File watcher thread for hot reload
     */
    void file_watcher_thread_worker();
    
    /**
     * @brief Plugin loading thread worker
     */
    void loading_thread_worker();
    
    /**
     * @brief Process load queue
     */
    void process_load_queue();
    
    /**
     * @brief Resolve single plugin dependencies
     */
    bool resolve_plugin_dependencies(const std::string& plugin_name, 
                                   std::unordered_set<std::string>& resolved,
                                   std::unordered_set<std::string>& visiting);
    
    /**
     * @brief Build dependency graph
     */
    void build_dependency_graph();
    
    /**
     * @brief Calculate load order from dependencies
     */
    void calculate_load_order();
    
    /**
     * @brief Update statistics
     */
    void update_statistics();
    
    /**
     * @brief Handle plugin crash
     */
    void handle_plugin_crash(const std::string& plugin_name, const std::exception& e);
    
    /**
     * @brief Cleanup failed plugin
     */
    void cleanup_failed_plugin(const std::string& plugin_name);
    
    /**
     * @brief Validate plugin file
     */
    bool validate_plugin_file(const std::string& file_path, PluginMetadata& metadata);
    
    /**
     * @brief Get file modification time
     */
    std::filesystem::file_time_type get_file_modification_time(const std::string& file_path);
};

//=============================================================================
// Global Plugin Manager Instance
//=============================================================================

/**
 * @brief Get global plugin manager instance
 */
PluginManager& get_plugin_manager();

/**
 * @brief Set global plugin manager instance
 */
void set_plugin_manager(std::unique_ptr<PluginManager> manager);

/**
 * @brief Initialize global plugin manager with config
 */
bool initialize_plugin_system(const PluginManagerConfig& config = PluginManagerConfig::create_educational());

/**
 * @brief Shutdown global plugin manager
 */
void shutdown_plugin_system();

} // namespace ecscope::plugin