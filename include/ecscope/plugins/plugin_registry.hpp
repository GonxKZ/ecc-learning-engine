#pragma once

#include "plugin_interface.hpp"
#include "plugin_context.hpp"
#include "plugin_loader.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <mutex>
#include <functional>
#include <future>
#include <queue>

namespace ecscope {
    // Forward declarations for engine systems
    namespace ecs { class Registry; class World; }
    namespace rendering { class Renderer; class ResourceManager; }
    namespace assets { class AssetManager; }
    namespace gui { class GuiManager; }
}

namespace ecscope::plugins {

    class PluginSecurity;
    class PluginMessaging;
    
    /**
     * @brief Plugin loading priority and dependency resolution
     */
    struct LoadOrder {
        std::vector<std::string> critical_plugins;
        std::vector<std::string> high_priority_plugins;
        std::vector<std::string> normal_plugins;
        std::vector<std::string> low_priority_plugins;
        std::unordered_map<std::string, std::vector<std::string>> dependencies;
    };
    
    /**
     * @brief Plugin instance information
     */
    struct PluginInstance {
        std::string name;
        std::unique_ptr<IPlugin> plugin;
        std::unique_ptr<PluginContext> context;
        LoadInfo load_info;
        PluginState state{PluginState::Unloaded};
        std::vector<std::string> dependents; // Plugins that depend on this one
        uint64_t load_time{0};
        uint64_t last_update{0};
        bool hot_swappable{false};
        
        PluginInstance(const std::string& plugin_name) : name(plugin_name) {}
    };
    
    /**
     * @brief Central registry for all plugins
     * 
     * Manages plugin lifecycle, dependencies, communication, and provides
     * a unified interface for plugin operations.
     */
    class PluginRegistry {
    public:
        PluginRegistry();
        ~PluginRegistry();
        
        // Initialization and shutdown
        bool initialize();
        void shutdown();
        bool is_initialized() const { return initialized_; }
        
        // Plugin loading and unloading
        bool load_plugin(const std::string& plugin_path);
        bool load_plugin_from_manifest(const std::string& manifest_path);
        bool unload_plugin(const std::string& plugin_name);
        bool reload_plugin(const std::string& plugin_name);
        
        // Batch operations
        std::vector<std::string> load_plugins_from_directory(const std::string& directory);
        bool load_plugins_with_dependencies(const std::vector<std::string>& plugin_paths);
        void unload_all_plugins();
        
        // Plugin management
        bool start_plugin(const std::string& plugin_name);
        bool stop_plugin(const std::string& plugin_name);
        bool pause_plugin(const std::string& plugin_name);
        bool resume_plugin(const std::string& plugin_name);
        
        // Plugin information
        bool is_plugin_loaded(const std::string& plugin_name) const;
        PluginState get_plugin_state(const std::string& plugin_name) const;
        const PluginMetadata* get_plugin_metadata(const std::string& plugin_name) const;
        IPlugin* get_plugin(const std::string& plugin_name);
        const IPlugin* get_plugin(const std::string& plugin_name) const;
        
        // Plugin discovery
        std::vector<std::string> get_loaded_plugins() const;
        std::vector<std::string> get_active_plugins() const;
        std::vector<std::string> find_plugins_by_tag(const std::string& tag) const;
        std::vector<std::string> find_plugins_by_author(const std::string& author) const;
        
        // Dependency management
        bool check_dependencies(const std::string& plugin_name) const;
        std::vector<std::string> get_missing_dependencies(const std::string& plugin_name) const;
        std::vector<std::string> get_plugin_dependents(const std::string& plugin_name) const;
        LoadOrder calculate_load_order(const std::vector<std::string>& plugin_names) const;
        
        // Plugin lifecycle events
        void update_plugins(double delta_time);
        
        // Event system
        void subscribe_to_event(const std::string& event_name, 
                               std::function<void(const std::map<std::string, std::string>&)> callback);
        void unsubscribe_from_event(const std::string& event_name, size_t callback_id);
        void emit_event(const std::string& event_name, 
                       const std::map<std::string, std::string>& params = {});
        
        // Plugin communication
        bool send_message(const std::string& sender, const std::string& recipient, 
                         const std::string& message, const std::map<std::string, std::string>& params = {});
        void broadcast_message(const std::string& sender, const std::string& message, 
                              const std::map<std::string, std::string>& params = {});
        
        // Hot-swapping support
        bool supports_hot_swap(const std::string& plugin_name) const;
        bool hot_swap_plugin(const std::string& plugin_name, const std::string& new_plugin_path);
        
        // Configuration
        void set_plugin_directory(const std::string& directory);
        std::string get_plugin_directory() const { return plugin_directory_; }
        void add_plugin_search_path(const std::string& path);
        void set_max_concurrent_loads(size_t max_loads) { max_concurrent_loads_ = max_loads; }
        
        // Security and sandboxing
        void set_security_enabled(bool enabled) { security_enabled_ = enabled; }
        bool is_security_enabled() const { return security_enabled_; }
        void set_default_permissions(const std::vector<Permission>& permissions);
        void grant_permission(const std::string& plugin_name, Permission permission);
        void revoke_permission(const std::string& plugin_name, Permission permission);
        
        // Engine system integration
        void set_ecs_registry(ecs::Registry* registry) { ecs_registry_ = registry; }
        void set_ecs_world(ecs::World* world) { ecs_world_ = world; }
        void set_renderer(rendering::Renderer* renderer) { renderer_ = renderer; }
        void set_resource_manager(rendering::ResourceManager* resource_manager) { resource_manager_ = resource_manager; }
        void set_asset_manager(assets::AssetManager* asset_manager) { asset_manager_ = asset_manager; }
        void set_gui_manager(gui::GuiManager* gui_manager) { gui_manager_ = gui_manager; }
        
        // Statistics and monitoring
        struct Statistics {
            size_t total_plugins{0};
            size_t active_plugins{0};
            size_t failed_plugins{0};
            uint64_t total_memory_usage{0};
            uint64_t total_load_time{0};
            uint64_t last_update_time{0};
        };
        
        Statistics get_statistics() const;
        std::vector<std::string> get_failed_plugins() const;
        std::string get_plugin_status_report() const;
        
        // Error handling
        std::vector<std::string> get_errors() const;
        void clear_errors();
        std::string get_last_error() const;
        
    private:
        // Core components
        std::unique_ptr<PluginLoader> loader_;
        std::unique_ptr<PluginDiscovery> discovery_;
        std::unique_ptr<PluginSecurity> security_;
        std::unique_ptr<PluginMessaging> messaging_;
        
        // Plugin storage
        std::unordered_map<std::string, std::unique_ptr<PluginInstance>> plugins_;
        mutable std::shared_mutex plugins_mutex_;
        
        // Event system
        std::unordered_map<std::string, std::vector<std::pair<size_t, std::function<void(const std::map<std::string, std::string>&)>>>> event_callbacks_;
        std::atomic<size_t> next_callback_id_{0};
        mutable std::mutex events_mutex_;
        
        // Configuration
        std::string plugin_directory_;
        std::vector<Permission> default_permissions_;
        size_t max_concurrent_loads_{4};
        bool security_enabled_{true};
        bool initialized_{false};
        
        // Engine system pointers
        ecs::Registry* ecs_registry_{nullptr};
        ecs::World* ecs_world_{nullptr};
        rendering::Renderer* renderer_{nullptr};
        rendering::ResourceManager* resource_manager_{nullptr};
        assets::AssetManager* asset_manager_{nullptr};
        gui::GuiManager* gui_manager_{nullptr};
        
        // Error tracking
        std::vector<std::string> errors_;
        mutable std::mutex errors_mutex_;
        
        // Statistics
        mutable Statistics stats_;
        
        // Private methods
        bool load_plugin_impl(const std::string& plugin_path, const std::string& manifest_path = "");
        bool unload_plugin_impl(const std::string& plugin_name);
        void setup_plugin_context(PluginInstance* instance);
        bool initialize_plugin(PluginInstance* instance);
        void shutdown_plugin(PluginInstance* instance);
        
        // Dependency resolution
        std::vector<std::string> resolve_dependencies(const std::vector<std::string>& plugin_names) const;
        bool has_circular_dependency(const std::string& plugin_name, 
                                    std::unordered_set<std::string>& visited,
                                    std::unordered_set<std::string>& recursion_stack) const;
        
        // Error handling
        void add_error(const std::string& error);
        void log_plugin_error(const std::string& plugin_name, const std::string& error);
        
        // Event system helpers
        void emit_plugin_event(const std::string& event_type, const std::string& plugin_name, 
                              const std::map<std::string, std::string>& extra_params = {});
        
        // Threading support
        std::queue<std::future<bool>> loading_tasks_;
        void process_loading_tasks();
    };
    
} // namespace ecscope::plugins