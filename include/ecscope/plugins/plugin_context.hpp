#pragma once

#include "plugin_interface.hpp"
#include <memory>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <any>

namespace ecscope {
    // Forward declarations from other systems
    namespace ecs { class Registry; class World; }
    namespace rendering { class Renderer; class ResourceManager; }
    namespace assets { class AssetManager; }
    namespace gui { class GuiManager; }
}

namespace ecscope::plugins {

    class PluginRegistry;
    class PluginSecurity;
    class PluginMessaging;
    
    /**
     * @brief Permission types for plugin security
     */
    enum class Permission {
        ReadFiles,
        WriteFiles,
        NetworkAccess,
        SystemCalls,
        ECCoreAccess,
        RenderingAccess,
        AssetAccess,
        GuiAccess,
        PluginCommunication,
        ScriptExecution
    };
    
    /**
     * @brief Resource quota limits for plugins
     */
    struct ResourceQuota {
        uint64_t max_memory_bytes{100 * 1024 * 1024}; // 100MB
        uint32_t max_cpu_time_ms{1000}; // 1 second
        uint32_t max_file_handles{100};
        uint32_t max_network_connections{10};
        uint32_t max_threads{4};
        
        bool is_within_limits(uint64_t memory, uint32_t cpu_time, 
                             uint32_t files, uint32_t connections, uint32_t threads) const;
    };
    
    /**
     * @brief Plugin execution context and API access point
     * 
     * This class provides plugins with controlled access to engine systems,
     * handles security and sandboxing, and manages resource quotas.
     */
    class PluginContext {
    public:
        PluginContext(const std::string& plugin_name, PluginRegistry* registry);
        ~PluginContext();
        
        // Security and permissions
        bool has_permission(Permission perm) const;
        bool request_permission(Permission perm, const std::string& reason = "");
        void revoke_permission(Permission perm);
        const ResourceQuota& get_resource_quota() const { return quota_; }
        void set_resource_quota(const ResourceQuota& quota) { quota_ = quota; }
        
        // Engine system access (with permission checks)
        ecs::Registry* get_ecs_registry();
        ecs::World* get_ecs_world();
        rendering::Renderer* get_renderer();
        rendering::ResourceManager* get_resource_manager();
        assets::AssetManager* get_asset_manager();
        gui::GuiManager* get_gui_manager();
        
        // Plugin communication
        bool send_message(const std::string& target_plugin, const std::string& message, 
                         const std::map<std::string, std::string>& params = {});
        void subscribe_to_event(const std::string& event_name, 
                               std::function<void(const std::map<std::string, std::string>&)> callback);
        void unsubscribe_from_event(const std::string& event_name);
        void emit_event(const std::string& event_name, 
                       const std::map<std::string, std::string>& params = {});
        
        // Resource management
        template<typename T>
        void store_resource(const std::string& key, T&& resource) {
            std::lock_guard<std::mutex> lock(resources_mutex_);
            resources_[key] = std::make_any<T>(std::forward<T>(resource));
        }
        
        template<typename T>
        T* get_resource(const std::string& key) {
            std::lock_guard<std::mutex> lock(resources_mutex_);
            auto it = resources_.find(key);
            if (it != resources_.end()) {
                try {
                    return std::any_cast<T>(&it->second);
                } catch (const std::bad_any_cast&) {
                    return nullptr;
                }
            }
            return nullptr;
        }
        
        void remove_resource(const std::string& key);
        std::vector<std::string> get_resource_keys() const;
        
        // Configuration
        void set_config(const std::string& key, const std::string& value);
        std::string get_config(const std::string& key, const std::string& default_value = "") const;
        void remove_config(const std::string& key);
        std::map<std::string, std::string> get_all_config() const;
        
        // Logging and debugging
        void log_debug(const std::string& message);
        void log_info(const std::string& message);
        void log_warning(const std::string& message);
        void log_error(const std::string& message);
        
        // Plugin information
        const std::string& get_plugin_name() const { return plugin_name_; }
        std::string get_plugin_directory() const;
        std::string get_plugin_data_directory() const;
        std::string get_plugin_config_directory() const;
        
        // Resource monitoring
        uint64_t get_current_memory_usage() const;
        uint32_t get_current_cpu_time() const;
        uint32_t get_open_file_handles() const;
        uint32_t get_network_connections() const;
        uint32_t get_thread_count() const;
        
        // Sandbox management
        void enter_sandbox();
        void exit_sandbox();
        bool is_sandboxed() const { return sandboxed_; }
        
    private:
        std::string plugin_name_;
        PluginRegistry* registry_;
        std::unordered_map<Permission, bool> permissions_;
        ResourceQuota quota_;
        
        // Resource storage
        std::unordered_map<std::string, std::any> resources_;
        mutable std::mutex resources_mutex_;
        
        // Configuration
        std::unordered_map<std::string, std::string> config_;
        mutable std::mutex config_mutex_;
        
        // Event subscriptions
        std::unordered_map<std::string, std::function<void(const std::map<std::string, std::string>&)>> event_callbacks_;
        mutable std::mutex events_mutex_;
        
        // Sandbox state
        bool sandboxed_{false};
        
        // System pointers (set by registry)
        ecs::Registry* ecs_registry_{nullptr};
        ecs::World* ecs_world_{nullptr};
        rendering::Renderer* renderer_{nullptr};
        rendering::ResourceManager* resource_manager_{nullptr};
        assets::AssetManager* asset_manager_{nullptr};
        gui::GuiManager* gui_manager_{nullptr};
        
        friend class PluginRegistry;
    };
    
    /**
     * @brief RAII wrapper for plugin sandbox context
     */
    class SandboxGuard {
    public:
        explicit SandboxGuard(PluginContext* context) : context_(context) {
            if (context_) {
                context_->enter_sandbox();
            }
        }
        
        ~SandboxGuard() {
            if (context_) {
                context_->exit_sandbox();
            }
        }
        
        SandboxGuard(const SandboxGuard&) = delete;
        SandboxGuard& operator=(const SandboxGuard&) = delete;
        
        SandboxGuard(SandboxGuard&& other) noexcept : context_(other.context_) {
            other.context_ = nullptr;
        }
        
        SandboxGuard& operator=(SandboxGuard&& other) noexcept {
            if (this != &other) {
                if (context_) {
                    context_->exit_sandbox();
                }
                context_ = other.context_;
                other.context_ = nullptr;
            }
            return *this;
        }
        
    private:
        PluginContext* context_;
    };
    
} // namespace ecscope::plugins