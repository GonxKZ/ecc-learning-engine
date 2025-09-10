#include "ecscope/plugins/plugin_context.hpp"
#include "ecscope/plugins/plugin_registry.hpp"
#include <iostream>
#include <filesystem>
#include <chrono>

// Forward declarations for engine systems (we'll assume these exist)
namespace ecscope {
    namespace ecs { class Registry; class World; }
    namespace rendering { class Renderer; class ResourceManager; }
    namespace assets { class AssetManager; }
    namespace gui { class GuiManager; }
}

namespace ecscope::plugins {

    bool ResourceQuota::is_within_limits(uint64_t memory, uint32_t cpu_time, 
                                        uint32_t files, uint32_t connections, uint32_t threads) const {
        return memory <= max_memory_bytes &&
               cpu_time <= max_cpu_time_ms &&
               files <= max_file_handles &&
               connections <= max_network_connections &&
               threads <= max_threads;
    }
    
    PluginContext::PluginContext(const std::string& plugin_name, PluginRegistry* registry)
        : plugin_name_(plugin_name), registry_(registry) {
        
        // Set default permissions (none by default, must be explicitly granted)
        permissions_[Permission::ReadFiles] = false;
        permissions_[Permission::WriteFiles] = false;
        permissions_[Permission::NetworkAccess] = false;
        permissions_[Permission::SystemCalls] = false;
        permissions_[Permission::ECCoreAccess] = false;
        permissions_[Permission::RenderingAccess] = false;
        permissions_[Permission::AssetAccess] = false;
        permissions_[Permission::GuiAccess] = false;
        permissions_[Permission::PluginCommunication] = true; // Allow by default
        permissions_[Permission::ScriptExecution] = false;
        
        // Set default resource quota
        quota_ = ResourceQuota{};
    }
    
    PluginContext::~PluginContext() {
        // Clean up resources
        {
            std::lock_guard<std::mutex> lock(resources_mutex_);
            resources_.clear();
        }
        
        {
            std::lock_guard<std::mutex> lock(config_mutex_);
            config_.clear();
        }
        
        {
            std::lock_guard<std::mutex> lock(events_mutex_);
            event_callbacks_.clear();
        }
    }
    
    bool PluginContext::has_permission(Permission perm) const {
        auto it = permissions_.find(perm);
        return it != permissions_.end() && it->second;
    }
    
    bool PluginContext::request_permission(Permission perm, const std::string& reason) {
        // For now, just grant the permission if requested
        // In a full implementation, this would involve user consent or policy checks
        permissions_[perm] = true;
        
        log_info("Permission granted: " + std::to_string(static_cast<int>(perm)) + 
                " Reason: " + reason);
        
        return true;
    }
    
    void PluginContext::revoke_permission(Permission perm) {
        permissions_[perm] = false;
        log_warning("Permission revoked: " + std::to_string(static_cast<int>(perm)));
    }
    
    ecs::Registry* PluginContext::get_ecs_registry() {
        if (!has_permission(Permission::ECCoreAccess)) {
            log_error("Plugin does not have permission to access ECS registry");
            return nullptr;
        }
        return ecs_registry_;
    }
    
    ecs::World* PluginContext::get_ecs_world() {
        if (!has_permission(Permission::ECCoreAccess)) {
            log_error("Plugin does not have permission to access ECS world");
            return nullptr;
        }
        return ecs_world_;
    }
    
    rendering::Renderer* PluginContext::get_renderer() {
        if (!has_permission(Permission::RenderingAccess)) {
            log_error("Plugin does not have permission to access renderer");
            return nullptr;
        }
        return renderer_;
    }
    
    rendering::ResourceManager* PluginContext::get_resource_manager() {
        if (!has_permission(Permission::RenderingAccess)) {
            log_error("Plugin does not have permission to access resource manager");
            return nullptr;
        }
        return resource_manager_;
    }
    
    assets::AssetManager* PluginContext::get_asset_manager() {
        if (!has_permission(Permission::AssetAccess)) {
            log_error("Plugin does not have permission to access asset manager");
            return nullptr;
        }
        return asset_manager_;
    }
    
    gui::GuiManager* PluginContext::get_gui_manager() {
        if (!has_permission(Permission::GuiAccess)) {
            log_error("Plugin does not have permission to access GUI manager");
            return nullptr;
        }
        return gui_manager_;
    }
    
    bool PluginContext::send_message(const std::string& target_plugin, const std::string& message, 
                                    const std::map<std::string, std::string>& params) {
        if (!has_permission(Permission::PluginCommunication)) {
            log_error("Plugin does not have permission for inter-plugin communication");
            return false;
        }
        
        if (!registry_) {
            log_error("Plugin registry not available for message sending");
            return false;
        }
        
        return registry_->send_message(plugin_name_, target_plugin, message, params);
    }
    
    void PluginContext::subscribe_to_event(const std::string& event_name, 
                                          std::function<void(const std::map<std::string, std::string>&)> callback) {
        std::lock_guard<std::mutex> lock(events_mutex_);
        event_callbacks_[event_name] = callback;
        
        if (registry_) {
            registry_->subscribe_to_event(event_name, [this, callback](const std::map<std::string, std::string>& params) {
                callback(params);
            });
        }
        
        log_debug("Subscribed to event: " + event_name);
    }
    
    void PluginContext::unsubscribe_from_event(const std::string& event_name) {
        std::lock_guard<std::mutex> lock(events_mutex_);
        event_callbacks_.erase(event_name);
        log_debug("Unsubscribed from event: " + event_name);
    }
    
    void PluginContext::emit_event(const std::string& event_name, 
                                  const std::map<std::string, std::string>& params) {
        if (registry_) {
            std::map<std::string, std::string> extended_params = params;
            extended_params["source_plugin"] = plugin_name_;
            registry_->emit_event(event_name, extended_params);
        }
        log_debug("Emitted event: " + event_name);
    }
    
    void PluginContext::remove_resource(const std::string& key) {
        std::lock_guard<std::mutex> lock(resources_mutex_);
        resources_.erase(key);
    }
    
    std::vector<std::string> PluginContext::get_resource_keys() const {
        std::lock_guard<std::mutex> lock(resources_mutex_);
        std::vector<std::string> keys;
        keys.reserve(resources_.size());
        
        for (const auto& [key, value] : resources_) {
            keys.push_back(key);
        }
        
        return keys;
    }
    
    void PluginContext::set_config(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(config_mutex_);
        config_[key] = value;
    }
    
    std::string PluginContext::get_config(const std::string& key, const std::string& default_value) const {
        std::lock_guard<std::mutex> lock(config_mutex_);
        auto it = config_.find(key);
        return (it != config_.end()) ? it->second : default_value;
    }
    
    void PluginContext::remove_config(const std::string& key) {
        std::lock_guard<std::mutex> lock(config_mutex_);
        config_.erase(key);
    }
    
    std::map<std::string, std::string> PluginContext::get_all_config() const {
        std::lock_guard<std::mutex> lock(config_mutex_);
        return config_;
    }
    
    void PluginContext::log_debug(const std::string& message) {
        std::cout << "[DEBUG][" << plugin_name_ << "] " << message << std::endl;
    }
    
    void PluginContext::log_info(const std::string& message) {
        std::cout << "[INFO][" << plugin_name_ << "] " << message << std::endl;
    }
    
    void PluginContext::log_warning(const std::string& message) {
        std::cout << "[WARNING][" << plugin_name_ << "] " << message << std::endl;
    }
    
    void PluginContext::log_error(const std::string& message) {
        std::cerr << "[ERROR][" << plugin_name_ << "] " << message << std::endl;
    }
    
    std::string PluginContext::get_plugin_directory() const {
        if (registry_) {
            return registry_->get_plugin_directory() + "/" + plugin_name_;
        }
        return "./plugins/" + plugin_name_;
    }
    
    std::string PluginContext::get_plugin_data_directory() const {
        std::string data_dir = get_plugin_directory() + "/data";
        std::filesystem::create_directories(data_dir);
        return data_dir;
    }
    
    std::string PluginContext::get_plugin_config_directory() const {
        std::string config_dir = get_plugin_directory() + "/config";
        std::filesystem::create_directories(config_dir);
        return config_dir;
    }
    
    uint64_t PluginContext::get_current_memory_usage() const {
        // In a real implementation, this would track actual memory usage
        // For now, return a placeholder
        return 0;
    }
    
    uint32_t PluginContext::get_current_cpu_time() const {
        // In a real implementation, this would track actual CPU time
        // For now, return a placeholder
        return 0;
    }
    
    uint32_t PluginContext::get_open_file_handles() const {
        // In a real implementation, this would count open file handles
        // For now, return a placeholder
        return 0;
    }
    
    uint32_t PluginContext::get_network_connections() const {
        // In a real implementation, this would count network connections
        // For now, return a placeholder
        return 0;
    }
    
    uint32_t PluginContext::get_thread_count() const {
        // In a real implementation, this would count plugin threads
        // For now, return a placeholder
        return 1;
    }
    
    void PluginContext::enter_sandbox() {
        if (!sandboxed_) {
            sandboxed_ = true;
            log_debug("Entered sandbox mode");
        }
    }
    
    void PluginContext::exit_sandbox() {
        if (sandboxed_) {
            sandboxed_ = false;
            log_debug("Exited sandbox mode");
        }
    }
    
} // namespace ecscope::plugins