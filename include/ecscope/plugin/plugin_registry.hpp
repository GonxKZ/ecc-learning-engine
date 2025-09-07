#pragma once

/**
 * @file plugin/plugin_registry.hpp
 * @brief ECScope Plugin Registry - Plugin Discovery and Service Registration
 * 
 * Comprehensive plugin registry system providing service discovery, component
 * registration, system registration, and plugin capability advertising. This
 * serves as the central directory for all plugin-provided functionality.
 * 
 * Key Features:
 * - Service discovery and registration
 * - ECS component and system registration from plugins
 * - Plugin capability advertising and querying
 * - Inter-plugin communication facilitation
 * - Educational plugin cataloging and organization
 * 
 * @author ECScope Educational ECS Framework - Plugin System
 * @date 2024
 */

#include "plugin_core.hpp"
#include "core/types.hpp"
#include "core/log.hpp"
#include "ecs/registry.hpp"
#include "ecs/component.hpp"
#include "ecs/system.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <functional>
#include <typeindex>
#include <any>
#include <mutex>
#include <shared_mutex>

namespace ecscope::plugin {

//=============================================================================
// Service Registration and Discovery
//=============================================================================

/**
 * @brief Service interface type
 */
enum class ServiceType {
    Singleton,      // Single instance service
    Factory,        // Factory for creating instances
    Prototype,      // New instance per request
    Component,      // ECS component type
    System,         // ECS system type
    Resource,       // Shared resource
    Custom          // Plugin-defined type
};

/**
 * @brief Service metadata
 */
struct ServiceMetadata {
    std::string name;
    std::string description;
    ServiceType type;
    std::string providing_plugin;
    PluginVersion version;
    std::vector<std::string> interfaces; // Interface names this service implements
    std::unordered_map<std::string, std::string> properties;
    bool is_educational{false};
    std::string learning_purpose;
    
    ServiceMetadata() = default;
    ServiceMetadata(const std::string& n, ServiceType t, const std::string& plugin)
        : name(n), type(t), providing_plugin(plugin) {}
};

/**
 * @brief Service factory function
 */
using ServiceFactory = std::function<std::any()>;

/**
 * @brief Service instance container
 */
struct ServiceInstance {
    std::any instance;
    ServiceMetadata metadata;
    std::chrono::system_clock::time_point created_time;
    std::atomic<u64> reference_count{0};
    std::string plugin_owner;
    
    ServiceInstance(std::any inst, const ServiceMetadata& meta, const std::string& owner)
        : instance(std::move(inst))
        , metadata(meta)
        , created_time(std::chrono::system_clock::now())
        , plugin_owner(owner) {}
};

//=============================================================================
// ECS Integration Structures
//=============================================================================

/**
 * @brief ECS component registration info
 */
struct ComponentRegistration {
    std::string name;
    std::string description;
    std::type_index type_index;
    usize size;
    std::string providing_plugin;
    std::function<void*(void*)> constructor;
    std::function<void(void*)> destructor;
    std::function<void*(const void*)> copy_constructor;
    std::function<void(void*, const void*)> copy_assignment;
    std::function<std::string(const void*)> to_string;
    bool is_educational{false};
    std::vector<std::string> learning_tags;
    
    ComponentRegistration(const std::string& n, std::type_index ti, usize s, const std::string& plugin)
        : name(n), type_index(ti), size(s), providing_plugin(plugin) {}
};

/**
 * @brief ECS system registration info
 */
struct SystemRegistration {
    std::string name;
    std::string description;
    std::string providing_plugin;
    PluginPriority priority{PluginPriority::Normal};
    std::vector<std::type_index> required_components;
    std::vector<std::type_index> optional_components;
    std::function<void(ecs::Registry&, f64)> update_function;
    std::function<void(ecs::Registry&)> initialize_function;
    std::function<void(ecs::Registry&)> shutdown_function;
    bool is_educational{false};
    std::string educational_purpose;
    
    SystemRegistration(const std::string& n, const std::string& plugin)
        : name(n), providing_plugin(plugin) {}
};

//=============================================================================
// Plugin Capability System
//=============================================================================

/**
 * @brief Plugin capability description
 */
struct PluginCapability {
    std::string name;
    std::string description;
    std::string category;
    std::vector<std::string> features;
    std::unordered_map<std::string, std::string> parameters;
    std::function<bool(const std::unordered_map<std::string, std::string>&)> is_available;
    std::string providing_plugin;
    bool is_core_capability{false};
    
    PluginCapability(const std::string& n, const std::string& plugin)
        : name(n), providing_plugin(plugin) {}
};

//=============================================================================
// Plugin Communication System
//=============================================================================

/**
 * @brief Message for inter-plugin communication
 */
struct PluginMessage {
    std::string sender;
    std::string receiver; // Empty for broadcast
    std::string message_type;
    std::unordered_map<std::string, std::any> payload;
    std::chrono::system_clock::time_point timestamp;
    u64 message_id;
    
    PluginMessage(const std::string& from, const std::string& to, const std::string& type)
        : sender(from), receiver(to), message_type(type)
        , timestamp(std::chrono::system_clock::now())
        , message_id(generate_message_id()) {}

private:
    static std::atomic<u64> message_id_counter_;
    static u64 generate_message_id() { return ++message_id_counter_; }
};

/**
 * @brief Plugin message handler
 */
using PluginMessageHandler = std::function<void(const PluginMessage&)>;

//=============================================================================
// Main Plugin Registry Class
//=============================================================================

/**
 * @brief Central Plugin Registry for Service Discovery and ECS Integration
 * 
 * The PluginRegistry serves as the central directory and communication hub
 * for all plugin-provided functionality. It handles service registration,
 * ECS component/system integration, capability advertising, and inter-plugin
 * communication.
 * 
 * Key Responsibilities:
 * - Service registration and discovery
 * - ECS component and system registration from plugins
 * - Plugin capability advertising and querying
 * - Inter-plugin message routing
 * - Educational resource organization
 * - Performance and usage monitoring
 */
class PluginRegistry {
private:
    // Service Management
    std::unordered_map<std::string, std::unique_ptr<ServiceInstance>> services_;
    std::unordered_map<std::string, ServiceFactory> service_factories_;
    std::unordered_map<std::string, ServiceMetadata> service_metadata_;
    
    // ECS Integration
    std::unordered_map<std::type_index, std::unique_ptr<ComponentRegistration>> component_registrations_;
    std::unordered_map<std::string, std::unique_ptr<SystemRegistration>> system_registrations_;
    std::unordered_map<std::string, std::type_index> component_name_to_type_;
    
    // Plugin Capabilities
    std::unordered_map<std::string, std::unique_ptr<PluginCapability>> capabilities_;
    std::unordered_map<std::string, std::vector<std::string>> plugin_to_capabilities_;
    std::unordered_map<std::string, std::vector<std::string>> category_to_capabilities_;
    
    // Communication System
    std::unordered_map<std::string, std::vector<PluginMessageHandler>> message_handlers_;
    std::vector<PluginMessage> message_queue_;
    std::atomic<u64> messages_sent_{0};
    std::atomic<u64> messages_processed_{0};
    
    // Educational Organization
    std::unordered_map<std::string, std::vector<std::string>> learning_paths_;
    std::unordered_map<std::string, std::vector<std::string>> difficulty_levels_;
    std::unordered_map<std::string, std::vector<std::string>> concept_maps_;
    
    // Threading and Synchronization
    mutable std::shared_mutex registry_mutex_;
    mutable std::mutex message_queue_mutex_;
    std::thread message_processor_thread_;
    std::atomic<bool> should_process_messages_{true};
    
    // Performance Tracking
    std::unordered_map<std::string, u64> service_access_counts_;
    std::unordered_map<std::string, f64> service_average_access_time_;
    std::chrono::high_resolution_clock::time_point creation_time_;

public:
    /**
     * @brief Construct plugin registry
     */
    PluginRegistry();
    
    /**
     * @brief Destructor
     */
    ~PluginRegistry();
    
    // Non-copyable but movable
    PluginRegistry(const PluginRegistry&) = delete;
    PluginRegistry& operator=(const PluginRegistry&) = delete;
    PluginRegistry(PluginRegistry&&) = default;
    PluginRegistry& operator=(PluginRegistry&&) = default;
    
    //-------------------------------------------------------------------------
    // Service Registration and Discovery
    //-------------------------------------------------------------------------
    
    /**
     * @brief Register a singleton service
     */
    template<typename T>
    bool register_singleton_service(const std::string& name, 
                                   std::unique_ptr<T> instance, 
                                   const std::string& plugin_name,
                                   const ServiceMetadata& metadata = {});
    
    /**
     * @brief Register a service factory
     */
    template<typename T>
    bool register_factory_service(const std::string& name, 
                                 std::function<std::unique_ptr<T>()> factory, 
                                 const std::string& plugin_name,
                                 const ServiceMetadata& metadata = {});
    
    /**
     * @brief Get service by name and type
     */
    template<typename T>
    T* get_service(const std::string& name);
    
    /**
     * @brief Create service instance from factory
     */
    template<typename T>
    std::unique_ptr<T> create_service(const std::string& name);
    
    /**
     * @brief Check if service exists
     */
    bool has_service(const std::string& name) const;
    
    /**
     * @brief Unregister service
     */
    bool unregister_service(const std::string& name);
    
    /**
     * @brief Get all service names
     */
    std::vector<std::string> get_service_names() const;
    
    /**
     * @brief Get services by type
     */
    std::vector<std::string> get_services_by_type(ServiceType type) const;
    
    /**
     * @brief Get services by plugin
     */
    std::vector<std::string> get_services_by_plugin(const std::string& plugin_name) const;
    
    /**
     * @brief Get service metadata
     */
    std::optional<ServiceMetadata> get_service_metadata(const std::string& name) const;
    
    //-------------------------------------------------------------------------
    // ECS Component Registration
    //-------------------------------------------------------------------------
    
    /**
     * @brief Register ECS component type
     */
    template<typename T>
    bool register_component(const std::string& name, 
                          const std::string& plugin_name,
                          const std::string& description = "",
                          bool is_educational = false);
    
    /**
     * @brief Unregister ECS component type
     */
    bool unregister_component(const std::string& name);
    
    /**
     * @brief Get component registration
     */
    const ComponentRegistration* get_component_registration(const std::string& name) const;
    
    /**
     * @brief Get component registration by type
     */
    template<typename T>
    const ComponentRegistration* get_component_registration() const;
    
    /**
     * @brief Get all registered component names
     */
    std::vector<std::string> get_component_names() const;
    
    /**
     * @brief Get components by plugin
     */
    std::vector<std::string> get_components_by_plugin(const std::string& plugin_name) const;
    
    /**
     * @brief Get educational components
     */
    std::vector<std::string> get_educational_components() const;
    
    //-------------------------------------------------------------------------
    // ECS System Registration
    //-------------------------------------------------------------------------
    
    /**
     * @brief Register ECS system
     */
    template<typename SystemType>
    bool register_system(const std::string& name,
                        const std::string& plugin_name,
                        const std::string& description = "",
                        PluginPriority priority = PluginPriority::Normal);
    
    /**
     * @brief Register ECS system with custom functions
     */
    bool register_system_functions(const std::string& name,
                                  const std::string& plugin_name,
                                  std::function<void(ecs::Registry&, f64)> update_func,
                                  std::function<void(ecs::Registry&)> init_func = nullptr,
                                  std::function<void(ecs::Registry&)> shutdown_func = nullptr,
                                  const std::string& description = "",
                                  PluginPriority priority = PluginPriority::Normal);
    
    /**
     * @brief Unregister system
     */
    bool unregister_system(const std::string& name);
    
    /**
     * @brief Get system registration
     */
    const SystemRegistration* get_system_registration(const std::string& name) const;
    
    /**
     * @brief Get all registered system names
     */
    std::vector<std::string> get_system_names() const;
    
    /**
     * @brief Get systems by plugin
     */
    std::vector<std::string> get_systems_by_plugin(const std::string& plugin_name) const;
    
    /**
     * @brief Get systems by priority
     */
    std::vector<std::string> get_systems_by_priority(PluginPriority priority) const;
    
    /**
     * @brief Get educational systems
     */
    std::vector<std::string> get_educational_systems() const;
    
    //-------------------------------------------------------------------------
    // Plugin Capability System
    //-------------------------------------------------------------------------
    
    /**
     * @brief Register plugin capability
     */
    bool register_capability(const std::string& name,
                           const std::string& plugin_name,
                           const PluginCapability& capability);
    
    /**
     * @brief Unregister capability
     */
    bool unregister_capability(const std::string& name);
    
    /**
     * @brief Check if capability exists
     */
    bool has_capability(const std::string& name) const;
    
    /**
     * @brief Check if capability is available
     */
    bool is_capability_available(const std::string& name, 
                               const std::unordered_map<std::string, std::string>& params = {}) const;
    
    /**
     * @brief Get capability
     */
    const PluginCapability* get_capability(const std::string& name) const;
    
    /**
     * @brief Get all capability names
     */
    std::vector<std::string> get_capability_names() const;
    
    /**
     * @brief Get capabilities by category
     */
    std::vector<std::string> get_capabilities_by_category(const std::string& category) const;
    
    /**
     * @brief Get capabilities by plugin
     */
    std::vector<std::string> get_capabilities_by_plugin(const std::string& plugin_name) const;
    
    /**
     * @brief Find capabilities by feature
     */
    std::vector<std::string> find_capabilities_with_feature(const std::string& feature) const;
    
    //-------------------------------------------------------------------------
    // Plugin Communication
    //-------------------------------------------------------------------------
    
    /**
     * @brief Register message handler
     */
    bool register_message_handler(const std::string& plugin_name,
                                const std::string& message_type,
                                const PluginMessageHandler& handler);
    
    /**
     * @brief Unregister message handler
     */
    bool unregister_message_handler(const std::string& plugin_name, const std::string& message_type);
    
    /**
     * @brief Send message to specific plugin
     */
    bool send_message(const std::string& sender,
                     const std::string& receiver,
                     const std::string& message_type,
                     const std::unordered_map<std::string, std::any>& payload = {});
    
    /**
     * @brief Broadcast message to all plugins
     */
    void broadcast_message(const std::string& sender,
                          const std::string& message_type,
                          const std::unordered_map<std::string, std::any>& payload = {});
    
    /**
     * @brief Get message statistics
     */
    struct MessageStats {
        u64 messages_sent;
        u64 messages_processed;
        u64 messages_queued;
        f64 average_processing_time_ms;
        std::unordered_map<std::string, u64> messages_by_type;
    };
    
    MessageStats get_message_stats() const;
    
    //-------------------------------------------------------------------------
    // Educational Organization
    //-------------------------------------------------------------------------
    
    /**
     * @brief Register learning path
     */
    bool register_learning_path(const std::string& path_name,
                               const std::vector<std::string>& components_or_services,
                               const std::string& description = "");
    
    /**
     * @brief Get learning path
     */
    std::vector<std::string> get_learning_path(const std::string& path_name) const;
    
    /**
     * @brief Get all learning paths
     */
    std::vector<std::string> get_learning_path_names() const;
    
    /**
     * @brief Organize services by difficulty
     */
    void organize_by_difficulty(const std::string& difficulty_level,
                               const std::vector<std::string>& services);
    
    /**
     * @brief Get services by difficulty level
     */
    std::vector<std::string> get_services_by_difficulty(const std::string& level) const;
    
    /**
     * @brief Create concept map
     */
    bool create_concept_map(const std::string& concept,
                           const std::vector<std::string>& related_services);
    
    /**
     * @brief Get concept map
     */
    std::vector<std::string> get_concept_map(const std::string& concept) const;
    
    //-------------------------------------------------------------------------
    // Plugin Lifecycle Integration
    //-------------------------------------------------------------------------
    
    /**
     * @brief Handle plugin loading
     */
    void on_plugin_loaded(const std::string& plugin_name);
    
    /**
     * @brief Handle plugin unloading
     */
    void on_plugin_unloading(const std::string& plugin_name);
    
    /**
     * @brief Clean up plugin registrations
     */
    void cleanup_plugin_registrations(const std::string& plugin_name);
    
    /**
     * @brief Get plugin registration summary
     */
    struct PluginRegistrationSummary {
        std::string plugin_name;
        std::vector<std::string> services;
        std::vector<std::string> components;
        std::vector<std::string> systems;
        std::vector<std::string> capabilities;
        u32 total_registrations;
    };
    
    PluginRegistrationSummary get_plugin_registrations(const std::string& plugin_name) const;
    
    /**
     * @brief Get all plugin registration summaries
     */
    std::vector<PluginRegistrationSummary> get_all_plugin_registrations() const;
    
    //-------------------------------------------------------------------------
    // Performance and Statistics
    //-------------------------------------------------------------------------
    
    /**
     * @brief Get registry statistics
     */
    struct RegistryStats {
        u32 total_services;
        u32 total_components;
        u32 total_systems;
        u32 total_capabilities;
        u32 total_plugins_registered;
        u64 total_service_accesses;
        f64 average_service_access_time_ms;
        usize memory_usage;
    };
    
    RegistryStats get_statistics() const;
    
    /**
     * @brief Get most accessed services
     */
    std::vector<std::pair<std::string, u64>> get_most_accessed_services(u32 count = 10) const;
    
    /**
     * @brief Generate registry report
     */
    std::string generate_registry_report() const;
    
    /**
     * @brief Clear all registrations
     */
    void clear();

private:
    //-------------------------------------------------------------------------
    // Internal Implementation
    //-------------------------------------------------------------------------
    
    /**
     * @brief Message processing thread worker
     */
    void message_processor_worker();
    
    /**
     * @brief Process single message
     */
    void process_message(const PluginMessage& message);
    
    /**
     * @brief Update service access statistics
     */
    void update_service_access_stats(const std::string& service_name, f64 access_time_ms);
    
    /**
     * @brief Validate service registration
     */
    bool validate_service_registration(const std::string& name, const std::string& plugin_name) const;
    
    /**
     * @brief Generate unique service ID
     */
    std::string generate_service_id(const std::string& name, const std::string& plugin_name) const;
};

//=============================================================================
// Template Implementations
//=============================================================================

template<typename T>
bool PluginRegistry::register_singleton_service(const std::string& name, 
                                               std::unique_ptr<T> instance, 
                                               const std::string& plugin_name,
                                               const ServiceMetadata& metadata) {
    if (!validate_service_registration(name, plugin_name)) {
        return false;
    }
    
    std::unique_lock<std::shared_mutex> lock(registry_mutex_);
    
    ServiceMetadata final_metadata = metadata;
    if (final_metadata.name.empty()) {
        final_metadata.name = name;
    }
    final_metadata.type = ServiceType::Singleton;
    final_metadata.providing_plugin = plugin_name;
    
    auto service_instance = std::make_unique<ServiceInstance>(
        std::any(std::move(instance)), final_metadata, plugin_name);
    
    services_[name] = std::move(service_instance);
    service_metadata_[name] = final_metadata;
    
    LOG_INFO("Registered singleton service '{}' from plugin '{}'", name, plugin_name);
    return true;
}

template<typename T>
bool PluginRegistry::register_factory_service(const std::string& name, 
                                             std::function<std::unique_ptr<T>()> factory, 
                                             const std::string& plugin_name,
                                             const ServiceMetadata& metadata) {
    if (!validate_service_registration(name, plugin_name)) {
        return false;
    }
    
    std::unique_lock<std::shared_mutex> lock(registry_mutex_);
    
    ServiceMetadata final_metadata = metadata;
    if (final_metadata.name.empty()) {
        final_metadata.name = name;
    }
    final_metadata.type = ServiceType::Factory;
    final_metadata.providing_plugin = plugin_name;
    
    // Wrap the typed factory in a generic one
    service_factories_[name] = [factory]() -> std::any {
        return std::any(factory());
    };
    
    service_metadata_[name] = final_metadata;
    
    LOG_INFO("Registered factory service '{}' from plugin '{}'", name, plugin_name);
    return true;
}

template<typename T>
T* PluginRegistry::get_service(const std::string& name) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::shared_lock<std::shared_mutex> lock(registry_mutex_);
    
    auto it = services_.find(name);
    if (it != services_.end()) {
        try {
            T* service = std::any_cast<std::unique_ptr<T>&>(it->second->instance).get();
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<f64, std::milli>(end_time - start_time).count();
            
            // Update statistics (need to unlock first to avoid deadlock)
            lock.unlock();
            update_service_access_stats(name, duration);
            
            return service;
        } catch (const std::bad_any_cast& e) {
            LOG_ERROR("Failed to cast service '{}' to requested type: {}", name, e.what());
            return nullptr;
        }
    }
    
    return nullptr;
}

template<typename T>
std::unique_ptr<T> PluginRegistry::create_service(const std::string& name) {
    std::shared_lock<std::shared_mutex> lock(registry_mutex_);
    
    auto it = service_factories_.find(name);
    if (it != service_factories_.end()) {
        try {
            std::any created = it->second();
            return std::any_cast<std::unique_ptr<T>>(created);
        } catch (const std::bad_any_cast& e) {
            LOG_ERROR("Failed to cast created service '{}' to requested type: {}", name, e.what());
            return nullptr;
        }
    }
    
    return nullptr;
}

template<typename T>
bool PluginRegistry::register_component(const std::string& name, 
                                       const std::string& plugin_name,
                                       const std::string& description,
                                       bool is_educational) {
    std::unique_lock<std::shared_mutex> lock(registry_mutex_);
    
    std::type_index type_index = std::type_index(typeid(T));
    
    // Check if already registered
    if (component_registrations_.find(type_index) != component_registrations_.end()) {
        LOG_WARN("Component type '{}' already registered", name);
        return false;
    }
    
    auto registration = std::make_unique<ComponentRegistration>(name, type_index, sizeof(T), plugin_name);
    registration->description = description;
    registration->is_educational = is_educational;
    
    // Set up component operations
    registration->constructor = [](void* ptr) -> void* {
        return new(ptr) T();
    };
    
    registration->destructor = [](void* ptr) {
        static_cast<T*>(ptr)->~T();
    };
    
    registration->copy_constructor = [](const void* src) -> void* {
        return new T(*static_cast<const T*>(src));
    };
    
    registration->copy_assignment = [](void* dst, const void* src) {
        *static_cast<T*>(dst) = *static_cast<const T*>(src);
    };
    
    registration->to_string = [](const void* ptr) -> std::string {
        // Default to_string implementation
        return typeid(T).name();
    };
    
    component_registrations_[type_index] = std::move(registration);
    component_name_to_type_[name] = type_index;
    
    LOG_INFO("Registered component '{}' from plugin '{}'", name, plugin_name);
    return true;
}

template<typename T>
const ComponentRegistration* PluginRegistry::get_component_registration() const {
    std::shared_lock<std::shared_mutex> lock(registry_mutex_);
    
    std::type_index type_index = std::type_index(typeid(T));
    auto it = component_registrations_.find(type_index);
    return (it != component_registrations_.end()) ? it->second.get() : nullptr;
}

template<typename SystemType>
bool PluginRegistry::register_system(const std::string& name,
                                    const std::string& plugin_name,
                                    const std::string& description,
                                    PluginPriority priority) {
    std::unique_lock<std::shared_mutex> lock(registry_mutex_);
    
    // Check if already registered
    if (system_registrations_.find(name) != system_registrations_.end()) {
        LOG_WARN("System '{}' already registered", name);
        return false;
    }
    
    auto registration = std::make_unique<SystemRegistration>(name, plugin_name);
    registration->description = description;
    registration->priority = priority;
    
    // System must implement these methods
    static_assert(std::is_base_of_v<ecs::System, SystemType>, 
                  "SystemType must inherit from ecs::System");
    
    // Create system functions
    registration->update_function = [](ecs::Registry& registry, f64 delta_time) {
        // This would be implemented by the specific system
        // For now, we create a placeholder
    };
    
    system_registrations_[name] = std::move(registration);
    
    LOG_INFO("Registered system '{}' from plugin '{}'", name, plugin_name);
    return true;
}

//=============================================================================
// Global Plugin Registry Instance
//=============================================================================

/**
 * @brief Get global plugin registry instance
 */
PluginRegistry& get_plugin_registry();

/**
 * @brief Set global plugin registry instance
 */
void set_plugin_registry(std::unique_ptr<PluginRegistry> registry);

} // namespace ecscope::plugin