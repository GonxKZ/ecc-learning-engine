#pragma once

#include "plugin_interface.hpp"
#include "plugin_context.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <typeindex>
#include <any>

// Forward declarations for ECS components
namespace ecscope::ecs {
    class Registry;
    class World;
    class Entity;
    class ComponentBase;
    class SystemBase;
    template<typename T> class Component;
    template<typename... Components> class System;
}

namespace ecscope::plugins {

    /**
     * @brief Plugin component registration and management
     */
    class PluginComponentRegistry {
    public:
        PluginComponentRegistry();
        ~PluginComponentRegistry();
        
        // Component registration
        template<typename ComponentType>
        bool register_component(const std::string& plugin_name, const std::string& component_name = "") {
            std::string name = component_name.empty() ? typeid(ComponentType).name() : component_name;
            
            ComponentInfo info;
            info.plugin_name = plugin_name;
            info.component_name = name;
            info.type_index = std::type_index(typeid(ComponentType));
            info.size = sizeof(ComponentType);
            info.alignment = alignof(ComponentType);
            
            // Component factory
            info.factory = [](const std::string& data) -> std::unique_ptr<ecs::ComponentBase> {
                return std::make_unique<ecs::Component<ComponentType>>();
            };
            
            // Serialization functions
            info.serializer = [](const ecs::ComponentBase* comp) -> std::string {
                // Basic serialization - in a real implementation, use reflection or custom serialization
                return "{}"; // JSON placeholder
            };
            
            info.deserializer = [](ecs::ComponentBase* comp, const std::string& data) -> bool {
                // Basic deserialization - in a real implementation, use reflection or custom deserialization
                return true;
            };
            
            std::lock_guard<std::mutex> lock(components_mutex_);
            registered_components_[info.type_index] = info;
            plugin_components_[plugin_name].push_back(info.type_index);
            
            return true;
        }
        
        bool unregister_component(const std::string& plugin_name, const std::type_index& type);
        void unregister_all_components(const std::string& plugin_name);
        
        // Component queries
        bool is_component_registered(const std::type_index& type) const;
        std::string get_component_plugin(const std::type_index& type) const;
        std::vector<std::type_index> get_plugin_components(const std::string& plugin_name) const;
        
        // Component creation
        std::unique_ptr<ecs::ComponentBase> create_component(const std::type_index& type, const std::string& init_data = "") const;
        
        // Serialization
        std::string serialize_component(const std::type_index& type, const ecs::ComponentBase* component) const;
        bool deserialize_component(const std::type_index& type, ecs::ComponentBase* component, const std::string& data) const;
        
    private:
        struct ComponentInfo {
            std::string plugin_name;
            std::string component_name;
            std::type_index type_index{std::type_index(typeid(void))};
            size_t size{0};
            size_t alignment{0};
            std::function<std::unique_ptr<ecs::ComponentBase>(const std::string&)> factory;
            std::function<std::string(const ecs::ComponentBase*)> serializer;
            std::function<bool(ecs::ComponentBase*, const std::string&)> deserializer;
        };
        
        std::unordered_map<std::type_index, ComponentInfo> registered_components_;
        std::unordered_map<std::string, std::vector<std::type_index>> plugin_components_;
        mutable std::mutex components_mutex_;
    };
    
    /**
     * @brief Plugin system registration and management
     */
    class PluginSystemRegistry {
    public:
        PluginSystemRegistry();
        ~PluginSystemRegistry();
        
        // System registration
        template<typename SystemType>
        bool register_system(const std::string& plugin_name, const std::string& system_name = "") {
            std::string name = system_name.empty() ? typeid(SystemType).name() : system_name;
            
            SystemInfo info;
            info.plugin_name = plugin_name;
            info.system_name = name;
            info.type_index = std::type_index(typeid(SystemType));
            info.priority = 1000; // Default priority
            
            // System factory
            info.factory = []() -> std::unique_ptr<ecs::SystemBase> {
                return std::make_unique<SystemType>();
            };
            
            std::lock_guard<std::mutex> lock(systems_mutex_);
            registered_systems_[info.type_index] = info;
            plugin_systems_[plugin_name].push_back(info.type_index);
            
            return true;
        }
        
        bool unregister_system(const std::string& plugin_name, const std::type_index& type);
        void unregister_all_systems(const std::string& plugin_name);
        
        // System queries
        bool is_system_registered(const std::type_index& type) const;
        std::string get_system_plugin(const std::type_index& type) const;
        std::vector<std::type_index> get_plugin_systems(const std::string& plugin_name) const;
        
        // System creation and lifecycle
        std::unique_ptr<ecs::SystemBase> create_system(const std::type_index& type) const;
        bool initialize_plugin_systems(const std::string& plugin_name, ecs::Registry* registry);
        void shutdown_plugin_systems(const std::string& plugin_name);
        
        // System priority management
        void set_system_priority(const std::type_index& type, int priority);
        int get_system_priority(const std::type_index& type) const;
        std::vector<std::type_index> get_systems_by_priority() const;
        
    private:
        struct SystemInfo {
            std::string plugin_name;
            std::string system_name;
            std::type_index type_index{std::type_index(typeid(void))};
            int priority{1000};
            std::function<std::unique_ptr<ecs::SystemBase>()> factory;
            std::unique_ptr<ecs::SystemBase> instance{nullptr};
        };
        
        std::unordered_map<std::type_index, SystemInfo> registered_systems_;
        std::unordered_map<std::string, std::vector<std::type_index>> plugin_systems_;
        mutable std::mutex systems_mutex_;
    };
    
    /**
     * @brief ECS plugin integration manager
     */
    class ECSPluginIntegration {
    public:
        ECSPluginIntegration();
        ~ECSPluginIntegration();
        
        // Initialization
        bool initialize(ecs::Registry* registry, ecs::World* world);
        void shutdown();
        bool is_initialized() const { return initialized_; }
        
        // Registry access
        PluginComponentRegistry* get_component_registry() { return component_registry_.get(); }
        PluginSystemRegistry* get_system_registry() { return system_registry_.get(); }
        
        // Plugin integration
        bool integrate_plugin(const std::string& plugin_name, IPlugin* plugin);
        void unintegrate_plugin(const std::string& plugin_name);
        bool is_plugin_integrated(const std::string& plugin_name) const;
        
        // Component management
        template<typename ComponentType>
        bool register_plugin_component(const std::string& plugin_name, const std::string& component_name = "") {
            if (!component_registry_) return false;
            
            bool success = component_registry_->register_component<ComponentType>(plugin_name, component_name);
            if (success) {
                // Register with ECS registry if available
                if (ecs_registry_) {
                    // In a real implementation, register with the actual ECS registry
                    // ecs_registry_->register_component<ComponentType>();
                }
            }
            return success;
        }
        
        // System management
        template<typename SystemType>
        bool register_plugin_system(const std::string& plugin_name, const std::string& system_name = "") {
            if (!system_registry_) return false;
            
            bool success = system_registry_->register_system<SystemType>(plugin_name, system_name);
            if (success) {
                // Initialize system if ECS is available
                if (ecs_registry_) {
                    // In a real implementation, add to the ECS scheduler
                    // auto system = system_registry_->create_system(std::type_index(typeid(SystemType)));
                    // ecs_registry_->add_system(std::move(system));
                }
            }
            return success;
        }
        
        // Entity access for plugins
        bool grant_entity_access(const std::string& plugin_name, uint32_t entity_id);
        void revoke_entity_access(const std::string& plugin_name, uint32_t entity_id);
        bool has_entity_access(const std::string& plugin_name, uint32_t entity_id) const;
        std::vector<uint32_t> get_plugin_entities(const std::string& plugin_name) const;
        
        // Plugin entity creation
        uint32_t create_plugin_entity(const std::string& plugin_name);
        bool destroy_plugin_entity(const std::string& plugin_name, uint32_t entity_id);
        
        // Component access control
        template<typename ComponentType>
        bool grant_component_access(const std::string& plugin_name) {
            std::lock_guard<std::mutex> lock(access_mutex_);
            auto type_index = std::type_index(typeid(ComponentType));
            plugin_component_access_[plugin_name].insert(type_index);
            return true;
        }
        
        template<typename ComponentType>
        void revoke_component_access(const std::string& plugin_name) {
            std::lock_guard<std::mutex> lock(access_mutex_);
            auto type_index = std::type_index(typeid(ComponentType));
            plugin_component_access_[plugin_name].erase(type_index);
        }
        
        template<typename ComponentType>
        bool has_component_access(const std::string& plugin_name) const {
            std::lock_guard<std::mutex> lock(access_mutex_);
            auto plugin_it = plugin_component_access_.find(plugin_name);
            if (plugin_it == plugin_component_access_.end()) return false;
            
            auto type_index = std::type_index(typeid(ComponentType));
            return plugin_it->second.find(type_index) != plugin_it->second.end();
        }
        
        // Event integration
        void on_entity_created(uint32_t entity_id);
        void on_entity_destroyed(uint32_t entity_id);
        void on_component_added(uint32_t entity_id, const std::type_index& component_type);
        void on_component_removed(uint32_t entity_id, const std::type_index& component_type);
        
        // Query integration
        struct PluginQuery {
            std::string plugin_name;
            std::vector<std::type_index> required_components;
            std::vector<std::type_index> excluded_components;
            std::function<void(const std::vector<uint32_t>&)> callback;
        };
        
        uint64_t register_plugin_query(const std::string& plugin_name, const PluginQuery& query);
        void unregister_plugin_query(uint64_t query_id);
        void execute_plugin_queries();
        
        // Statistics
        struct Statistics {
            size_t total_plugin_components{0};
            size_t total_plugin_systems{0};
            size_t total_plugin_entities{0};
            size_t total_plugin_queries{0};
            size_t integrated_plugins{0};
        };
        
        Statistics get_statistics() const;
        std::string generate_integration_report() const;
        
    private:
        std::unique_ptr<PluginComponentRegistry> component_registry_;
        std::unique_ptr<PluginSystemRegistry> system_registry_;
        
        // ECS system references
        ecs::Registry* ecs_registry_{nullptr};
        ecs::World* ecs_world_{nullptr};
        
        // Plugin integration tracking
        std::unordered_map<std::string, IPlugin*> integrated_plugins_;
        std::unordered_map<std::string, std::vector<uint32_t>> plugin_entities_;
        std::unordered_map<std::string, std::unordered_set<std::type_index>> plugin_component_access_;
        
        // Query system
        std::unordered_map<uint64_t, PluginQuery> plugin_queries_;
        std::atomic<uint64_t> next_query_id_{1};
        
        // Synchronization
        mutable std::mutex access_mutex_;
        mutable std::mutex entities_mutex_;
        mutable std::mutex queries_mutex_;
        
        bool initialized_{false};
        
        // Internal helpers
        void cleanup_plugin_data(const std::string& plugin_name);
        void notify_plugins_entity_event(const std::string& event_type, uint32_t entity_id, const std::type_index& component_type = std::type_index(typeid(void)));
    };
    
    /**
     * @brief Helper class for plugin ECS integration
     */
    class PluginECSHelper {
    public:
        PluginECSHelper(const std::string& plugin_name, ECSPluginIntegration* integration, PluginContext* context);
        
        // Entity operations
        uint32_t create_entity();
        bool destroy_entity(uint32_t entity_id);
        bool is_valid_entity(uint32_t entity_id) const;
        std::vector<uint32_t> get_owned_entities() const;
        
        // Component operations
        template<typename ComponentType>
        bool add_component(uint32_t entity_id, ComponentType&& component) {
            if (!has_component_access<ComponentType>()) return false;
            if (!has_entity_access(entity_id)) return false;
            
            // In a real implementation, add to ECS registry
            return true;
        }
        
        template<typename ComponentType>
        bool remove_component(uint32_t entity_id) {
            if (!has_component_access<ComponentType>()) return false;
            if (!has_entity_access(entity_id)) return false;
            
            // In a real implementation, remove from ECS registry
            return true;
        }
        
        template<typename ComponentType>
        ComponentType* get_component(uint32_t entity_id) {
            if (!has_component_access<ComponentType>()) return nullptr;
            if (!has_entity_access(entity_id)) return nullptr;
            
            // In a real implementation, get from ECS registry
            return nullptr;
        }
        
        template<typename ComponentType>
        bool has_component(uint32_t entity_id) const {
            if (!has_component_access<ComponentType>()) return false;
            if (!has_entity_access(entity_id)) return false;
            
            // In a real implementation, check ECS registry
            return false;
        }
        
        // Query operations
        template<typename... ComponentTypes>
        std::vector<uint32_t> query_entities() {
            // Check access for all component types
            bool has_access = (has_component_access<ComponentTypes>() && ...);
            if (!has_access) return {};
            
            // In a real implementation, query ECS registry
            return {};
        }
        
        // System registration
        template<typename SystemType>
        bool register_system(const std::string& system_name = "") {
            return integration_->register_plugin_system<SystemType>(plugin_name_, system_name);
        }
        
        // Component registration
        template<typename ComponentType>
        bool register_component(const std::string& component_name = "") {
            return integration_->register_plugin_component<ComponentType>(plugin_name_, component_name);
        }
        
    private:
        std::string plugin_name_;
        ECSPluginIntegration* integration_;
        PluginContext* context_;
        
        template<typename ComponentType>
        bool has_component_access() const {
            return integration_->has_component_access<ComponentType>(plugin_name_);
        }
        
        bool has_entity_access(uint32_t entity_id) const {
            return integration_->has_entity_access(plugin_name_, entity_id);
        }
    };
    
} // namespace ecscope::plugins