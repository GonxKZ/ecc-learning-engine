#pragma once

#include "plugin_types.hpp"
#include "plugin_base.hpp"
#include "plugin_api.hpp"
#include "security_context.hpp"
#include "dynamic_loader.hpp"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>

namespace ecscope::plugins {

// Forward declarations
class PluginRegistry;
class PluginSandbox;

// =============================================================================
// Plugin Manager - Central Plugin System Controller
// =============================================================================

/// Central management system for all plugin operations
/// Handles loading, unloading, dependency resolution, security, and lifecycle management
class PluginManager {
public:
    // =============================================================================
    // Configuration Structure
    // =============================================================================

    struct Configuration {
        std::string plugin_directory = "plugins/";      ///< Default plugin search directory
        std::string config_directory = "config/";       ///< Configuration files directory
        std::string temp_directory = "temp/";           ///< Temporary files directory
        
        bool enable_hot_reload = true;                   ///< Enable hot-reloading of plugins
        bool enable_sandboxing = true;                   ///< Enable plugin sandboxing
        bool enable_signature_validation = false;       ///< Enable plugin signature validation
        bool strict_dependency_checking = true;         ///< Strict dependency version checking
        bool parallel_loading = true;                    ///< Load plugins in parallel when possible
        
        uint32_t max_plugins = 256;                     ///< Maximum number of plugins
        uint32_t loading_timeout_ms = 30000;            ///< Plugin loading timeout
        uint32_t shutdown_timeout_ms = 10000;           ///< Plugin shutdown timeout
        
        SecurityLevel default_security_level = SecurityLevel::SANDBOXED;
        
        std::vector<std::string> trusted_publishers;     ///< Trusted plugin publishers
        std::vector<std::string> blocked_plugins;        ///< Blocked plugin names
        std::vector<std::string> search_paths;           ///< Additional search paths
        
        // Resource limits
        uint64_t max_memory_per_plugin = 128 * 1024 * 1024; ///< 128MB default
        uint64_t max_cpu_time_ms = 100;                  ///< 100ms per frame default
        uint64_t max_file_io_per_second = 10 * 1024 * 1024; ///< 10MB/s default
    };

    // =============================================================================
    // Constructor and Lifecycle
    // =============================================================================

    explicit PluginManager(const Configuration& config = Configuration{});
    ~PluginManager();

    /// Initialize the plugin manager
    /// @return PluginError::SUCCESS on success
    PluginError initialize();

    /// Shutdown the plugin manager and all loaded plugins
    /// @param timeout_ms Maximum time to wait for shutdown
    /// @return PluginError::SUCCESS on success
    PluginError shutdown(uint32_t timeout_ms = 0);

    /// Update all active plugins (call every frame)
    /// @param delta_time Time elapsed since last update
    void update(double delta_time);

    /// Render all active plugins that support rendering
    void render();

    // =============================================================================
    // Plugin Loading and Management
    // =============================================================================

    /// Load plugin from file
    /// @param plugin_path Path to plugin file or manifest
    /// @param force_reload Force reload if already loaded
    /// @return PluginError::SUCCESS on success
    PluginError loadPlugin(const std::string& plugin_path, bool force_reload = false);

    /// Load plugin from manifest
    /// @param manifest Plugin manifest
    /// @return PluginError::SUCCESS on success
    PluginError loadPlugin(std::shared_ptr<PluginManifest> manifest);

    /// Unload plugin by name
    /// @param plugin_name Name of plugin to unload
    /// @param force Force unload even if other plugins depend on it
    /// @return PluginError::SUCCESS on success
    PluginError unloadPlugin(const std::string& plugin_name, bool force = false);

    /// Reload plugin (unload and load again)
    /// @param plugin_name Name of plugin to reload
    /// @return PluginError::SUCCESS on success
    PluginError reloadPlugin(const std::string& plugin_name);

    /// Enable plugin (if loaded but disabled)
    /// @param plugin_name Name of plugin to enable
    /// @return PluginError::SUCCESS on success
    PluginError enablePlugin(const std::string& plugin_name);

    /// Disable plugin (keep loaded but stop updates)
    /// @param plugin_name Name of plugin to disable
    /// @return PluginError::SUCCESS on success
    PluginError disablePlugin(const std::string& plugin_name);

    // =============================================================================
    // Plugin Discovery and Registry
    // =============================================================================

    /// Scan directory for plugins
    /// @param directory Directory to scan
    /// @param recursive Scan subdirectories recursively
    /// @return List of discovered plugin manifests
    std::vector<std::shared_ptr<PluginManifest>> discoverPlugins(const std::string& directory, 
                                                                 bool recursive = true);

    /// Refresh plugin registry (rescan all search paths)
    void refreshRegistry();

    /// Get available plugins (discovered but not necessarily loaded)
    /// @return List of available plugin manifests
    std::vector<std::shared_ptr<PluginManifest>> getAvailablePlugins() const;

    /// Get loaded plugins
    /// @return List of loaded plugin names
    std::vector<std::string> getLoadedPlugins() const;

    /// Check if plugin is loaded
    /// @param plugin_name Plugin name to check
    /// @return true if plugin is loaded
    bool isPluginLoaded(const std::string& plugin_name) const;

    /// Get plugin by name
    /// @param plugin_name Plugin name
    /// @return Shared pointer to plugin, or nullptr if not found
    std::shared_ptr<PluginBase> getPlugin(const std::string& plugin_name) const;

    /// Get plugin manifest by name
    /// @param plugin_name Plugin name
    /// @return Shared pointer to manifest, or nullptr if not found
    std::shared_ptr<PluginManifest> getPluginManifest(const std::string& plugin_name) const;

    // =============================================================================
    // Dependency Management
    // =============================================================================

    /// Resolve plugin dependencies
    /// @param plugin_name Plugin to resolve dependencies for
    /// @return List of plugins in load order, empty if unresolvable
    std::vector<std::string> resolveDependencies(const std::string& plugin_name);

    /// Check for dependency conflicts
    /// @param plugin_name Plugin to check
    /// @return List of conflicting plugin names
    std::vector<std::string> checkConflicts(const std::string& plugin_name);

    /// Load plugin with all dependencies
    /// @param plugin_name Plugin name
    /// @return PluginError::SUCCESS on success
    PluginError loadPluginWithDependencies(const std::string& plugin_name);

    /// Get plugins that depend on the specified plugin
    /// @param plugin_name Plugin name
    /// @return List of dependent plugin names
    std::vector<std::string> getDependents(const std::string& plugin_name) const;

    // =============================================================================
    // Hot Reloading System
    // =============================================================================

    /// Enable hot-reloading for a plugin
    /// @param plugin_name Plugin name
    /// @return true if hot-reload enabled successfully
    bool enableHotReload(const std::string& plugin_name);

    /// Disable hot-reloading for a plugin
    /// @param plugin_name Plugin name
    /// @return true if hot-reload disabled successfully
    bool disableHotReload(const std::string& plugin_name);

    /// Check if hot-reloading is enabled for a plugin
    /// @param plugin_name Plugin name
    /// @return true if hot-reload is enabled
    bool isHotReloadEnabled(const std::string& plugin_name) const;

    /// Force check for plugin file changes
    void checkForChanges();

    // =============================================================================
    // Security and Sandboxing
    // =============================================================================

    /// Set security level for a plugin
    /// @param plugin_name Plugin name
    /// @param level Security level
    /// @return true if security level set successfully
    bool setPluginSecurityLevel(const std::string& plugin_name, SecurityLevel level);

    /// Get security level for a plugin
    /// @param plugin_name Plugin name
    /// @return Security level, or ISOLATED if plugin not found
    SecurityLevel getPluginSecurityLevel(const std::string& plugin_name) const;

    /// Grant permission to a plugin
    /// @param plugin_name Plugin name
    /// @param capability Capability to grant
    /// @param reason Reason for granting permission
    /// @return true if permission granted
    bool grantPermission(const std::string& plugin_name, PluginCapabilities capability, 
                        const std::string& reason = "");

    /// Revoke permission from a plugin
    /// @param plugin_name Plugin name
    /// @param capability Capability to revoke
    /// @return true if permission revoked
    bool revokePermission(const std::string& plugin_name, PluginCapabilities capability);

    /// Check if plugin has permission
    /// @param plugin_name Plugin name
    /// @param capability Capability to check
    /// @return true if plugin has permission
    bool hasPermission(const std::string& plugin_name, PluginCapabilities capability) const;

    // =============================================================================
    // Plugin Communication
    // =============================================================================

    /// Send message between plugins
    /// @param from Sender plugin name
    /// @param to Target plugin name (empty for broadcast)
    /// @param type Message type
    /// @param data Message data
    /// @return true if message sent successfully
    bool sendMessage(const std::string& from, const std::string& to, 
                    const std::string& type, const std::vector<uint8_t>& data);

    /// Register message handler
    /// @param plugin_name Plugin name
    /// @param message_type Message type to handle
    /// @param handler Handler function
    /// @return true if handler registered successfully
    bool registerMessageHandler(const std::string& plugin_name, const std::string& message_type,
                               std::function<void(const PluginMessage&)> handler);

    /// Unregister message handler
    /// @param plugin_name Plugin name
    /// @param message_type Message type
    /// @return true if handler unregistered successfully
    bool unregisterMessageHandler(const std::string& plugin_name, const std::string& message_type);

    /// Broadcast event to all interested plugins
    /// @param event Plugin event
    /// @return Number of plugins that received the event
    uint32_t broadcastEvent(const PluginEvent& event);

    // =============================================================================
    // Resource Monitoring
    // =============================================================================

    /// Get plugin statistics
    /// @param plugin_name Plugin name
    /// @return Plugin statistics, or empty stats if plugin not found
    PluginStats getPluginStats(const std::string& plugin_name) const;

    /// Get resource usage for plugin
    /// @param plugin_name Plugin name
    /// @param type Resource type
    /// @return Current resource usage
    uint64_t getPluginResourceUsage(const std::string& plugin_name, ResourceType type) const;

    /// Set resource quota for plugin
    /// @param plugin_name Plugin name
    /// @param quota Resource quota specification
    /// @return true if quota set successfully
    bool setPluginResourceQuota(const std::string& plugin_name, const ResourceQuota& quota);

    /// Check if plugin is within resource quotas
    /// @param plugin_name Plugin name
    /// @return true if within all quotas
    bool isPluginWithinQuotas(const std::string& plugin_name) const;

    /// Get plugins that have exceeded quotas
    /// @return List of plugin names that exceeded quotas
    std::vector<std::string> getQuotaViolators() const;

    // =============================================================================
    // Configuration Management
    // =============================================================================

    /// Get configuration value for plugin
    /// @param plugin_name Plugin name
    /// @param key Configuration key
    /// @return Configuration value, or empty string if not found
    std::string getPluginConfig(const std::string& plugin_name, const std::string& key) const;

    /// Set configuration value for plugin
    /// @param plugin_name Plugin name
    /// @param key Configuration key
    /// @param value Configuration value
    /// @return true if configuration set successfully
    bool setPluginConfig(const std::string& plugin_name, const std::string& key, 
                        const std::string& value);

    /// Load plugin configuration from file
    /// @param plugin_name Plugin name
    /// @param config_file Configuration file path
    /// @return true if configuration loaded successfully
    bool loadPluginConfig(const std::string& plugin_name, const std::string& config_file);

    /// Save plugin configuration to file
    /// @param plugin_name Plugin name
    /// @param config_file Configuration file path
    /// @return true if configuration saved successfully
    bool savePluginConfig(const std::string& plugin_name, const std::string& config_file);

    // =============================================================================
    // Event Handling
    // =============================================================================

    /// Set event handler for plugin events
    /// @param handler Event handler function
    void setEventHandler(std::function<void(const PluginEvent&)> handler);

    /// Set error handler for plugin errors
    /// @param handler Error handler function
    void setErrorHandler(std::function<void(const std::string&, PluginError, const std::string&)> handler);

    /// Set security violation handler
    /// @param handler Security violation handler function
    void setSecurityViolationHandler(std::function<void(const std::string&, const std::string&)> handler);

    // =============================================================================
    // System Integration
    // =============================================================================

    /// Set ECS registry for plugin access
    /// @param registry ECS registry
    void setECSRegistry(std::shared_ptr<ecs::Registry> registry);

    /// Set renderer for plugin access
    /// @param renderer Rendering system
    void setRenderer(std::shared_ptr<rendering::Renderer> renderer);

    /// Set physics world for plugin access
    /// @param world Physics world
    void setPhysicsWorld(std::shared_ptr<physics::World> world);

    /// Set asset manager for plugin access
    /// @param manager Asset manager
    void setAssetManager(std::shared_ptr<assets::AssetManager> manager);

    /// Set GUI context for plugin access
    /// @param context GUI context
    void setGuiContext(std::shared_ptr<gui::GuiContext> context);

    /// Set audio system for plugin access
    /// @param audio Audio system
    void setAudioSystem(std::shared_ptr<audio::AudioSystem> audio);

    /// Set network manager for plugin access
    /// @param network Network manager
    void setNetworkManager(std::shared_ptr<networking::NetworkManager> network);

    // =============================================================================
    // Debugging and Diagnostics
    // =============================================================================

    /// Get comprehensive system status
    /// @return System status information
    std::unordered_map<std::string, std::string> getSystemStatus() const;

    /// Get plugin debug information
    /// @param plugin_name Plugin name
    /// @return Debug information map
    std::unordered_map<std::string, std::string> getPluginDebugInfo(const std::string& plugin_name) const;

    /// Enable debug mode for all plugins
    /// @param enabled true to enable debug mode
    void setDebugMode(bool enabled);

    /// Check if debug mode is enabled
    /// @return true if debug mode is enabled
    bool isDebugModeEnabled() const;

    /// Export plugin system state to JSON
    /// @return JSON string representing system state
    std::string exportSystemState() const;

    /// Import plugin system state from JSON
    /// @param json_state JSON string representing system state
    /// @return true if state imported successfully
    bool importSystemState(const std::string& json_state);

private:
    // =============================================================================
    // Internal Plugin Information
    // =============================================================================

    struct PluginInfo {
        std::shared_ptr<PluginManifest> manifest;
        std::shared_ptr<PluginBase> instance;
        std::shared_ptr<SecurityContext> security;
        std::shared_ptr<DynamicLoader> loader;
        
        PluginState state = PluginState::UNLOADED;
        PluginStats stats;
        std::chrono::system_clock::time_point load_time;
        std::string file_path;
        
        std::unordered_map<std::string, std::string> config;
        std::unordered_map<ResourceType, ResourceQuota> quotas;
        std::unordered_map<std::string, std::function<void(const PluginMessage&)>> message_handlers;
        
        bool enabled = true;
        bool hot_reload_enabled = false;
        std::chrono::system_clock::time_point last_modified;
    };

    // =============================================================================
    // Member Variables
    // =============================================================================

    Configuration config_;
    std::unique_ptr<PluginRegistry> registry_;
    
    mutable std::shared_mutex plugins_mutex_;
    std::unordered_map<std::string, std::unique_ptr<PluginInfo>> loaded_plugins_;
    std::unordered_map<std::string, std::string> plugin_paths_;
    
    // System references
    std::weak_ptr<ecs::Registry> ecs_registry_;
    std::weak_ptr<rendering::Renderer> renderer_;
    std::weak_ptr<physics::World> physics_world_;
    std::weak_ptr<assets::AssetManager> asset_manager_;
    std::weak_ptr<gui::GuiContext> gui_context_;
    std::weak_ptr<audio::AudioSystem> audio_system_;
    std::weak_ptr<networking::NetworkManager> network_manager_;
    
    // Event handlers
    std::function<void(const PluginEvent&)> event_handler_;
    std::function<void(const std::string&, PluginError, const std::string&)> error_handler_;
    std::function<void(const std::string&, const std::string&)> security_violation_handler_;
    
    // Hot-reload system
    std::unique_ptr<std::thread> hot_reload_thread_;
    std::atomic<bool> hot_reload_active_{false};
    std::condition_variable hot_reload_cv_;
    std::mutex hot_reload_mutex_;
    
    // State management
    std::atomic<bool> initialized_{false};
    std::atomic<bool> shutting_down_{false};
    std::atomic<bool> debug_mode_{false};
    
    // Statistics
    mutable std::shared_mutex stats_mutex_;
    uint64_t total_plugins_loaded_ = 0;
    uint64_t total_plugins_failed_ = 0;
    uint64_t total_messages_sent_ = 0;
    uint64_t total_events_broadcast_ = 0;
    std::chrono::system_clock::time_point start_time_;

    // =============================================================================
    // Internal Helper Methods
    // =============================================================================

    /// Load plugin implementation
    PluginError loadPluginImpl(const std::string& plugin_name, std::shared_ptr<PluginManifest> manifest, 
                              const std::string& file_path, bool force_reload);
    
    /// Unload plugin implementation
    PluginError unloadPluginImpl(const std::string& plugin_name, bool force);
    
    /// Initialize plugin after loading
    PluginError initializePlugin(const std::string& plugin_name);
    
    /// Create plugin API instance
    std::shared_ptr<PluginAPI> createPluginAPI(const std::string& plugin_name);
    
    /// Create security context for plugin
    std::shared_ptr<SecurityContext> createSecurityContext(const std::string& plugin_name);
    
    /// Validate plugin manifest
    std::vector<std::string> validateManifest(const PluginManifest& manifest);
    
    /// Check plugin dependencies
    bool checkDependencies(const PluginManifest& manifest, std::vector<std::string>& missing);
    
    /// Update plugin resource usage
    void updatePluginResourceUsage(const std::string& plugin_name);
    
    /// Check plugin resource quotas
    bool checkPluginQuotas(const std::string& plugin_name);
    
    /// Hot-reload worker thread
    void hotReloadWorker();
    
    /// Handle plugin state transition
    void handleStateTransition(const std::string& plugin_name, PluginState old_state, PluginState new_state);
    
    /// Fire plugin event
    void firePluginEvent(PluginEventType type, const std::string& plugin_name, const std::string& message = "");
    
    /// Fire error event
    void fireErrorEvent(const std::string& plugin_name, PluginError error, const std::string& message);
    
    /// Cleanup plugin resources
    void cleanupPlugin(const std::string& plugin_name);
    
    /// Get dependency load order
    std::vector<std::string> getDependencyLoadOrder(const std::vector<std::string>& plugins);
    
    /// Validate plugin file integrity
    bool validatePluginFile(const std::string& file_path, const std::string& expected_checksum);
    
    /// Create plugin sandbox directory
    std::string createPluginSandbox(const std::string& plugin_name);
    
    /// Cleanup plugin sandbox
    void cleanupPluginSandbox(const std::string& plugin_name);
};

} // namespace ecscope::plugins