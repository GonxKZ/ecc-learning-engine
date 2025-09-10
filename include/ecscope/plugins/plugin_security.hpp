#pragma once

#include "plugin_context.hpp"
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <mutex>
#include <chrono>
#include <functional>

namespace ecscope::plugins {

    /**
     * @brief Security policy for plugin access control
     */
    class SecurityPolicy {
    public:
        SecurityPolicy() = default;
        virtual ~SecurityPolicy() = default;
        
        // Permission checks
        virtual bool can_access_files(const std::string& plugin_name, const std::string& path) const = 0;
        virtual bool can_access_network(const std::string& plugin_name, const std::string& host, uint16_t port) const = 0;
        virtual bool can_execute_system_calls(const std::string& plugin_name) const = 0;
        virtual bool can_access_engine_component(const std::string& plugin_name, const std::string& component) const = 0;
        virtual bool can_communicate_with_plugin(const std::string& sender, const std::string& recipient) const = 0;
        
        // Resource limits
        virtual uint64_t get_memory_limit(const std::string& plugin_name) const = 0;
        virtual uint32_t get_cpu_time_limit(const std::string& plugin_name) const = 0;
        virtual uint32_t get_file_handle_limit(const std::string& plugin_name) const = 0;
        virtual uint32_t get_network_connection_limit(const std::string& plugin_name) const = 0;
        virtual uint32_t get_thread_limit(const std::string& plugin_name) const = 0;
        
        // Security events
        virtual void on_security_violation(const std::string& plugin_name, const std::string& violation) = 0;
        virtual void on_resource_limit_exceeded(const std::string& plugin_name, const std::string& resource) = 0;
    };
    
    /**
     * @brief Default security policy implementation
     */
    class DefaultSecurityPolicy : public SecurityPolicy {
    public:
        DefaultSecurityPolicy();
        
        // Permission checks
        bool can_access_files(const std::string& plugin_name, const std::string& path) const override;
        bool can_access_network(const std::string& plugin_name, const std::string& host, uint16_t port) const override;
        bool can_execute_system_calls(const std::string& plugin_name) const override;
        bool can_access_engine_component(const std::string& plugin_name, const std::string& component) const override;
        bool can_communicate_with_plugin(const std::string& sender, const std::string& recipient) const override;
        
        // Resource limits
        uint64_t get_memory_limit(const std::string& plugin_name) const override;
        uint32_t get_cpu_time_limit(const std::string& plugin_name) const override;
        uint32_t get_file_handle_limit(const std::string& plugin_name) const override;
        uint32_t get_network_connection_limit(const std::string& plugin_name) const override;
        uint32_t get_thread_limit(const std::string& plugin_name) const override;
        
        // Security events
        void on_security_violation(const std::string& plugin_name, const std::string& violation) override;
        void on_resource_limit_exceeded(const std::string& plugin_name, const std::string& resource) override;
        
        // Configuration
        void set_plugin_permissions(const std::string& plugin_name, const std::unordered_set<Permission>& permissions);
        void set_plugin_resource_limits(const std::string& plugin_name, const ResourceQuota& limits);
        void add_allowed_path(const std::string& plugin_name, const std::string& path);
        void add_blocked_host(const std::string& host);
        void set_default_sandbox_mode(bool enabled) { default_sandbox_enabled_ = enabled; }
        
    private:
        std::unordered_map<std::string, std::unordered_set<Permission>> plugin_permissions_;
        std::unordered_map<std::string, ResourceQuota> plugin_limits_;
        std::unordered_map<std::string, std::vector<std::string>> allowed_paths_;
        std::unordered_set<std::string> blocked_hosts_;
        bool default_sandbox_enabled_{true};
        mutable std::shared_mutex policy_mutex_;
    };
    
    /**
     * @brief Resource monitor for tracking plugin resource usage
     */
    class ResourceMonitor {
    public:
        struct ResourceUsage {
            uint64_t memory_bytes{0};
            uint32_t cpu_time_ms{0};
            uint32_t file_handles{0};
            uint32_t network_connections{0};
            uint32_t thread_count{0};
            std::chrono::steady_clock::time_point last_update;
        };
        
        ResourceMonitor();
        ~ResourceMonitor();
        
        // Monitoring control
        void start_monitoring(const std::string& plugin_name);
        void stop_monitoring(const std::string& plugin_name);
        bool is_monitoring(const std::string& plugin_name) const;
        
        // Resource tracking
        void update_memory_usage(const std::string& plugin_name, uint64_t bytes);
        void update_cpu_time(const std::string& plugin_name, uint32_t ms);
        void increment_file_handles(const std::string& plugin_name);
        void decrement_file_handles(const std::string& plugin_name);
        void increment_network_connections(const std::string& plugin_name);
        void decrement_network_connections(const std::string& plugin_name);
        void set_thread_count(const std::string& plugin_name, uint32_t count);
        
        // Resource queries
        ResourceUsage get_usage(const std::string& plugin_name) const;
        std::vector<std::string> get_monitored_plugins() const;
        bool is_within_limits(const std::string& plugin_name, const ResourceQuota& limits) const;
        std::vector<std::string> get_over_limit_plugins(const ResourceQuota& limits) const;
        
        // Callbacks for limit violations
        void set_memory_limit_callback(std::function<void(const std::string&, uint64_t, uint64_t)> callback);
        void set_cpu_limit_callback(std::function<void(const std::string&, uint32_t, uint32_t)> callback);
        
    private:
        std::unordered_map<std::string, ResourceUsage> usage_map_;
        mutable std::shared_mutex usage_mutex_;
        
        // Limit violation callbacks
        std::function<void(const std::string&, uint64_t, uint64_t)> memory_limit_callback_;
        std::function<void(const std::string&, uint32_t, uint32_t)> cpu_limit_callback_;
        
        void check_limits(const std::string& plugin_name);
    };
    
    /**
     * @brief Sandbox manager for plugin isolation
     */
    class SandboxManager {
    public:
        SandboxManager();
        ~SandboxManager();
        
        // Sandbox operations
        bool create_sandbox(const std::string& plugin_name);
        bool destroy_sandbox(const std::string& plugin_name);
        bool enter_sandbox(const std::string& plugin_name);
        bool exit_sandbox(const std::string& plugin_name);
        bool is_in_sandbox(const std::string& plugin_name) const;
        
        // Sandbox configuration
        void set_sandbox_directory(const std::string& plugin_name, const std::string& directory);
        void add_allowed_path(const std::string& plugin_name, const std::string& path);
        void remove_allowed_path(const std::string& plugin_name, const std::string& path);
        void set_network_access(const std::string& plugin_name, bool allowed);
        void set_system_call_filter(const std::string& plugin_name, const std::vector<std::string>& allowed_calls);
        
        // Sandbox queries
        std::string get_sandbox_directory(const std::string& plugin_name) const;
        std::vector<std::string> get_allowed_paths(const std::string& plugin_name) const;
        bool has_network_access(const std::string& plugin_name) const;
        std::vector<std::string> get_allowed_system_calls(const std::string& plugin_name) const;
        
        // Security enforcement
        bool check_file_access(const std::string& plugin_name, const std::string& path) const;
        bool check_network_access(const std::string& plugin_name, const std::string& host, uint16_t port) const;
        bool check_system_call(const std::string& plugin_name, const std::string& call) const;
        
    private:
        struct SandboxConfig {
            std::string directory;
            std::vector<std::string> allowed_paths;
            bool network_access{false};
            std::vector<std::string> allowed_system_calls;
            bool active{false};
        };
        
        std::unordered_map<std::string, SandboxConfig> sandbox_configs_;
        mutable std::shared_mutex sandbox_mutex_;
        
        // Platform-specific sandbox implementation
        bool create_sandbox_impl(const std::string& plugin_name, const SandboxConfig& config);
        bool destroy_sandbox_impl(const std::string& plugin_name);
        bool enter_sandbox_impl(const std::string& plugin_name);
        bool exit_sandbox_impl(const std::string& plugin_name);
    };
    
    /**
     * @brief Main security manager coordinating all security components
     */
    class PluginSecurity {
    public:
        PluginSecurity();
        ~PluginSecurity();
        
        // Initialization
        bool initialize();
        void shutdown();
        bool is_initialized() const { return initialized_; }
        
        // Security policy
        void set_security_policy(std::unique_ptr<SecurityPolicy> policy);
        SecurityPolicy* get_security_policy() const { return policy_.get(); }
        
        // Plugin registration
        void register_plugin(const std::string& plugin_name, const PluginMetadata& metadata);
        void unregister_plugin(const std::string& plugin_name);
        bool is_plugin_registered(const std::string& plugin_name) const;
        
        // Permission management
        bool has_permission(const std::string& plugin_name, Permission permission) const;
        void grant_permission(const std::string& plugin_name, Permission permission);
        void revoke_permission(const std::string& plugin_name, Permission permission);
        void set_permissions(const std::string& plugin_name, const std::unordered_set<Permission>& permissions);
        std::unordered_set<Permission> get_permissions(const std::string& plugin_name) const;
        
        // Resource management
        void set_resource_limits(const std::string& plugin_name, const ResourceQuota& limits);
        ResourceQuota get_resource_limits(const std::string& plugin_name) const;
        ResourceMonitor::ResourceUsage get_resource_usage(const std::string& plugin_name) const;
        bool is_within_resource_limits(const std::string& plugin_name) const;
        
        // Sandbox management
        bool enable_sandbox(const std::string& plugin_name);
        bool disable_sandbox(const std::string& plugin_name);
        bool is_sandboxed(const std::string& plugin_name) const;
        bool enter_sandbox(const std::string& plugin_name);
        bool exit_sandbox(const std::string& plugin_name);
        
        // Access control
        bool check_file_access(const std::string& plugin_name, const std::string& path) const;
        bool check_network_access(const std::string& plugin_name, const std::string& host, uint16_t port) const;
        bool check_engine_access(const std::string& plugin_name, const std::string& component) const;
        bool check_plugin_communication(const std::string& sender, const std::string& recipient) const;
        
        // Security events
        void report_security_violation(const std::string& plugin_name, const std::string& violation);
        void report_resource_violation(const std::string& plugin_name, const std::string& resource, const std::string& details);
        
        // Monitoring
        void start_monitoring(const std::string& plugin_name);
        void stop_monitoring(const std::string& plugin_name);
        std::vector<std::string> get_security_violations(const std::string& plugin_name = "") const;
        void clear_security_violations(const std::string& plugin_name = "");
        
        // Configuration
        void set_global_sandbox_enabled(bool enabled) { global_sandbox_enabled_ = enabled; }
        bool is_global_sandbox_enabled() const { return global_sandbox_enabled_; }
        void set_strict_mode(bool enabled) { strict_mode_ = enabled; }
        bool is_strict_mode() const { return strict_mode_; }
        
    private:
        std::unique_ptr<SecurityPolicy> policy_;
        std::unique_ptr<ResourceMonitor> resource_monitor_;
        std::unique_ptr<SandboxManager> sandbox_manager_;
        
        // Plugin registration
        std::unordered_set<std::string> registered_plugins_;
        mutable std::shared_mutex plugins_mutex_;
        
        // Security violations log
        std::unordered_map<std::string, std::vector<std::string>> security_violations_;
        mutable std::mutex violations_mutex_;
        
        // Configuration
        bool initialized_{false};
        bool global_sandbox_enabled_{true};
        bool strict_mode_{false};
        
        // Internal helpers
        void setup_default_policy();
        void setup_resource_monitoring();
        void setup_sandbox_management();
    };
    
} // namespace ecscope::plugins