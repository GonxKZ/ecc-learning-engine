#pragma once

/**
 * @file plugin/plugin_api.hpp
 * @brief ECScope Plugin API Framework - Complete Plugin Development Interface
 * 
 * Comprehensive plugin API framework providing event systems, service registration,
 * ECS integration, resource management, and educational features. This is the
 * primary interface that plugins use to interact with the ECScope engine.
 * 
 * Key Features:
 * - Complete ECS integration (components, systems, entities)
 * - Event-driven communication system
 * - Service registration and discovery
 * - Resource management and loading
 * - Educational plugin development support
 * - Performance monitoring and profiling
 * - Cross-plugin communication
 * 
 * @author ECScope Educational ECS Framework - Plugin System
 * @date 2024
 */

#include "plugin_core.hpp"
#include "plugin_registry.hpp"
#include "core/types.hpp"
#include "core/log.hpp"
#include "ecs/registry.hpp"
#include "ecs/component.hpp"
#include "ecs/system.hpp"
#include "memory/arena.hpp"
#include "memory/memory_tracker.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <chrono>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <type_traits>
#include <any>

namespace ecscope::plugin {

//=============================================================================
// Forward Declarations
//=============================================================================

class PluginAPI;
class PluginContext;
class ResourceManager;
class EventBus;

//=============================================================================
// Plugin Context and Environment
//=============================================================================

/**
 * @brief Plugin execution context
 * 
 * Contains all the information and resources available to a plugin
 * during its execution. This includes ECS access, event systems,
 * resource management, and educational tools.
 */
class PluginContext {
private:
    std::string plugin_name_;
    PluginMetadata metadata_;
    PluginSecurityContext security_context_;
    
    // Core systems access
    ecs::Registry* ecs_registry_;
    PluginRegistry* plugin_registry_;
    ResourceManager* resource_manager_;
    EventBus* event_bus_;
    
    // Memory management
    std::unique_ptr<memory::ArenaAllocator> plugin_allocator_;
    memory::MemoryTracker::ScopeTracker memory_tracker_;
    
    // Performance tracking
    std::chrono::high_resolution_clock::time_point creation_time_;
    std::atomic<u64> api_calls_count_{0};
    std::atomic<f64> total_execution_time_ms_{0.0};
    
    // Configuration
    std::unordered_map<std::string, std::string> configuration_;
    
    // Educational features
    std::vector<std::string> learning_notes_;
    std::unordered_map<std::string, std::string> code_examples_;

public:
    /**
     * @brief Construct plugin context
     */
    PluginContext(const std::string& plugin_name,
                 const PluginMetadata& metadata,
                 ecs::Registry& ecs_registry,
                 PluginRegistry& plugin_registry);
    
    /**
     * @brief Destructor
     */
    ~PluginContext();
    
    // Non-copyable but movable
    PluginContext(const PluginContext&) = delete;
    PluginContext& operator=(const PluginContext&) = delete;
    PluginContext(PluginContext&&) = default;
    PluginContext& operator=(PluginContext&&) = default;
    
    /**
     * @brief Get plugin name
     */
    const std::string& get_plugin_name() const { return plugin_name_; }
    
    /**
     * @brief Get plugin metadata
     */
    const PluginMetadata& get_metadata() const { return metadata_; }
    
    /**
     * @brief Get ECS registry
     */
    ecs::Registry& get_ecs_registry() { return *ecs_registry_; }
    
    /**
     * @brief Get plugin registry
     */
    PluginRegistry& get_plugin_registry() { return *plugin_registry_; }
    
    /**
     * @brief Get resource manager
     */
    ResourceManager& get_resource_manager() { return *resource_manager_; }
    
    /**
     * @brief Get event bus
     */
    EventBus& get_event_bus() { return *event_bus_; }
    
    /**
     * @brief Get plugin memory allocator
     */
    memory::ArenaAllocator& get_allocator() { return *plugin_allocator_; }
    
    /**
     * @brief Get security context
     */
    const PluginSecurityContext& get_security_context() const { return security_context_; }
    
    /**
     * @brief Set security context
     */
    void set_security_context(const PluginSecurityContext& context) {
        security_context_ = context;
    }
    
    /**
     * @brief Get configuration value
     */
    std::string get_config(const std::string& key, const std::string& default_value = "") const {
        auto it = configuration_.find(key);
        return (it != configuration_.end()) ? it->second : default_value;
    }
    
    /**
     * @brief Set configuration value
     */
    void set_config(const std::string& key, const std::string& value) {
        configuration_[key] = value;
    }
    
    /**
     * @brief Get all configuration
     */
    const std::unordered_map<std::string, std::string>& get_all_config() const {
        return configuration_;
    }
    
    /**
     * @brief Add learning note
     */
    void add_learning_note(const std::string& note) {
        learning_notes_.push_back(note);
    }
    
    /**
     * @brief Get learning notes
     */
    const std::vector<std::string>& get_learning_notes() const {
        return learning_notes_;
    }
    
    /**
     * @brief Add code example
     */
    void add_code_example(const std::string& title, const std::string& code) {
        code_examples_[title] = code;
    }
    
    /**
     * @brief Get code examples
     */
    const std::unordered_map<std::string, std::string>& get_code_examples() const {
        return code_examples_;
    }
    
    /**
     * @brief Track API call
     */
    void track_api_call(f64 execution_time_ms = 0.0) {
        api_calls_count_++;
        total_execution_time_ms_ += execution_time_ms;
    }
    
    /**
     * @brief Get performance statistics
     */
    struct PerformanceStats {
        u64 api_calls_count;
        f64 total_execution_time_ms;
        f64 average_call_time_ms;
        f64 uptime_seconds;
        usize memory_usage;
    };
    
    PerformanceStats get_performance_stats() const;

private:
    void initialize_subsystems();
    void cleanup_subsystems();
};

//=============================================================================
// Resource Management System
//=============================================================================

/**
 * @brief Resource type identifier
 */
enum class ResourceType {
    Texture,
    Mesh,
    Audio,
    Font,
    Shader,
    Script,
    Data,
    Configuration,
    Custom
};

/**
 * @brief Resource metadata
 */
struct ResourceMetadata {
    std::string name;
    std::string file_path;
    ResourceType type{ResourceType::Custom};
    usize size{0};
    std::string checksum;
    std::chrono::system_clock::time_point created_time;
    std::chrono::system_clock::time_point last_accessed;
    std::string providing_plugin;
    bool is_persistent{true};
    std::unordered_map<std::string, std::string> properties;
};

/**
 * @brief Resource handle for safe resource access
 */
template<typename T>
class ResourceHandle {
private:
    std::shared_ptr<T> resource_;
    std::string resource_name_;
    std::atomic<u32> access_count_{0};
    mutable std::mutex access_mutex_;

public:
    ResourceHandle() = default;
    ResourceHandle(std::shared_ptr<T> resource, const std::string& name)
        : resource_(resource), resource_name_(name) {}
    
    /**
     * @brief Get resource (thread-safe)
     */
    T* get() {
        std::lock_guard<std::mutex> lock(access_mutex_);
        access_count_++;
        return resource_.get();
    }
    
    /**
     * @brief Get resource (const)
     */
    const T* get() const {
        std::lock_guard<std::mutex> lock(access_mutex_);
        access_count_++;
        return resource_.get();
    }
    
    /**
     * @brief Check if resource is valid
     */
    bool is_valid() const {
        return resource_ != nullptr;
    }
    
    /**
     * @brief Get resource name
     */
    const std::string& get_name() const { return resource_name_; }
    
    /**
     * @brief Get access count
     */
    u32 get_access_count() const { return access_count_.load(); }
    
    /**
     * @brief Reset resource
     */
    void reset() {
        std::lock_guard<std::mutex> lock(access_mutex_);
        resource_.reset();
    }
    
    // Pointer-like operators
    T& operator*() { return *get(); }
    const T& operator*() const { return *get(); }
    T* operator->() { return get(); }
    const T* operator->() const { return get(); }
    
    explicit operator bool() const { return is_valid(); }
};

/**
 * @brief Resource manager for plugin resources
 */
class ResourceManager {
private:
    std::string plugin_name_;
    std::unordered_map<std::string, std::any> resources_;
    std::unordered_map<std::string, ResourceMetadata> resource_metadata_;
    std::unordered_map<ResourceType, std::vector<std::string>> resources_by_type_;
    
    // Resource loading
    std::unordered_map<ResourceType, std::function<std::any(const std::string&)>> loaders_;
    std::unordered_map<std::string, std::function<std::any(const std::string&)>> custom_loaders_;
    
    mutable std::shared_mutex resources_mutex_;
    
    // Memory tracking
    std::atomic<usize> total_memory_usage_{0};
    memory::MemoryTracker::ScopeTracker memory_tracker_;

public:
    explicit ResourceManager(const std::string& plugin_name);
    ~ResourceManager();
    
    /**
     * @brief Load resource from file
     */
    template<typename T>
    ResourceHandle<T> load_resource(const std::string& name, 
                                   const std::string& file_path,
                                   ResourceType type = ResourceType::Custom);
    
    /**
     * @brief Create resource in memory
     */
    template<typename T>
    ResourceHandle<T> create_resource(const std::string& name, 
                                     std::unique_ptr<T> resource,
                                     ResourceType type = ResourceType::Custom);
    
    /**
     * @brief Get resource by name
     */
    template<typename T>
    ResourceHandle<T> get_resource(const std::string& name);
    
    /**
     * @brief Check if resource exists
     */
    bool has_resource(const std::string& name) const;
    
    /**
     * @brief Unload resource
     */
    bool unload_resource(const std::string& name);
    
    /**
     * @brief Get resource metadata
     */
    std::optional<ResourceMetadata> get_resource_metadata(const std::string& name) const;
    
    /**
     * @brief Get resources by type
     */
    std::vector<std::string> get_resources_by_type(ResourceType type) const;
    
    /**
     * @brief Get all resource names
     */
    std::vector<std::string> get_all_resource_names() const;
    
    /**
     * @brief Register custom resource loader
     */
    template<typename T>
    void register_resource_loader(ResourceType type, 
                                 std::function<std::unique_ptr<T>(const std::string&)> loader);
    
    /**
     * @brief Register custom resource loader by name
     */
    template<typename T>
    void register_custom_loader(const std::string& loader_name,
                               std::function<std::unique_ptr<T>(const std::string&)> loader);
    
    /**
     * @brief Get memory usage statistics
     */
    struct MemoryStats {
        usize total_memory_usage;
        u32 total_resources;
        std::unordered_map<ResourceType, u32> resources_by_type;
        std::unordered_map<ResourceType, usize> memory_by_type;
    };
    
    MemoryStats get_memory_stats() const;
    
    /**
     * @brief Clear all resources
     */
    void clear();

private:
    void initialize_default_loaders();
    void update_memory_usage(const std::string& name, isize size_delta);
    ResourceType deduce_resource_type(const std::string& file_path) const;
};

//=============================================================================
// Event Bus System
//=============================================================================

/**
 * @brief Event bus for plugin communication
 */
class EventBus {
public:
    /**
     * @brief Event listener function type
     */
    template<typename EventType>
    using EventListener = std::function<void(const EventType&)>;
    
    /**
     * @brief Event filter function type
     */
    template<typename EventType>
    using EventFilter = std::function<bool(const EventType&)>;

private:
    std::string plugin_name_;
    
    // Event handlers storage
    std::unordered_map<std::type_index, std::vector<std::function<void(const std::any&)>>> event_handlers_;
    std::unordered_map<std::type_index, std::vector<std::function<bool(const std::any&)>>> event_filters_;
    
    // Event queue
    std::vector<std::pair<std::type_index, std::any>> event_queue_;
    std::mutex event_queue_mutex_;
    
    // Statistics
    std::atomic<u64> events_published_{0};
    std::atomic<u64> events_handled_{0};
    std::unordered_map<std::type_index, u64> events_by_type_;
    
    mutable std::shared_mutex handlers_mutex_;

public:
    explicit EventBus(const std::string& plugin_name);
    ~EventBus();
    
    /**
     * @brief Subscribe to event type
     */
    template<typename EventType>
    void subscribe(EventListener<EventType> listener);
    
    /**
     * @brief Subscribe with filter
     */
    template<typename EventType>
    void subscribe(EventListener<EventType> listener, EventFilter<EventType> filter);
    
    /**
     * @brief Unsubscribe from event type
     */
    template<typename EventType>
    void unsubscribe();
    
    /**
     * @brief Publish event immediately
     */
    template<typename EventType>
    void publish(const EventType& event);
    
    /**
     * @brief Queue event for later processing
     */
    template<typename EventType>
    void queue_event(const EventType& event);
    
    /**
     * @brief Process queued events
     */
    void process_queued_events();
    
    /**
     * @brief Get event statistics
     */
    struct EventStats {
        u64 events_published;
        u64 events_handled;
        u32 active_subscriptions;
        std::unordered_map<std::string, u64> events_by_type_name;
    };
    
    EventStats get_event_stats() const;
    
    /**
     * @brief Clear all event handlers
     */
    void clear();

private:
    std::string get_type_name(const std::type_index& type_index) const;
};

//=============================================================================
// ECS Integration Helper
//=============================================================================

/**
 * @brief ECS integration helper for plugins
 */
class ECSIntegration {
private:
    PluginContext& context_;
    std::vector<std::string> registered_components_;
    std::vector<std::string> registered_systems_;

public:
    explicit ECSIntegration(PluginContext& context) : context_(context) {}
    
    /**
     * @brief Register component type
     */
    template<typename ComponentType>
    bool register_component(const std::string& name, 
                           const std::string& description = "",
                           bool is_educational = false);
    
    /**
     * @brief Register system
     */
    template<typename SystemType>
    bool register_system(const std::string& name,
                        const std::string& description = "",
                        PluginPriority priority = PluginPriority::Normal);
    
    /**
     * @brief Register system with custom update function
     */
    bool register_system_function(const std::string& name,
                                 std::function<void(ecs::Registry&, f64)> update_func,
                                 const std::string& description = "",
                                 PluginPriority priority = PluginPriority::Normal);
    
    /**
     * @brief Create entity with components
     */
    template<typename... Components>
    ecs::Entity create_entity(Components&&... components);
    
    /**
     * @brief Destroy entity
     */
    bool destroy_entity(ecs::Entity entity);
    
    /**
     * @brief Add component to entity
     */
    template<typename ComponentType>
    bool add_component(ecs::Entity entity, const ComponentType& component);
    
    /**
     * @brief Remove component from entity
     */
    template<typename ComponentType>
    bool remove_component(ecs::Entity entity);
    
    /**
     * @brief Get component from entity
     */
    template<typename ComponentType>
    ComponentType* get_component(ecs::Entity entity);
    
    /**
     * @brief Check if entity has component
     */
    template<typename ComponentType>
    bool has_component(ecs::Entity entity);
    
    /**
     * @brief Query entities with components
     */
    template<typename... Components>
    std::vector<ecs::Entity> query_entities();
    
    /**
     * @brief Iterate over entities with components
     */
    template<typename... Components>
    void for_each(std::function<void(ecs::Entity, Components&...)> func);
    
    /**
     * @brief Get registered components
     */
    const std::vector<std::string>& get_registered_components() const {
        return registered_components_;
    }
    
    /**
     * @brief Get registered systems
     */
    const std::vector<std::string>& get_registered_systems() const {
        return registered_systems_;
    }
    
    /**
     * @brief Cleanup all registrations
     */
    void cleanup();
};

//=============================================================================
// Main Plugin API Class
//=============================================================================

/**
 * @brief Main Plugin API interface
 * 
 * This is the primary interface that plugins use to interact with the
 * ECScope engine. It provides access to all subsystems and utilities
 * needed for plugin development.
 */
class PluginAPI {
private:
    std::unique_ptr<PluginContext> context_;
    std::unique_ptr<ECSIntegration> ecs_integration_;
    
    // API call tracking
    mutable std::atomic<u64> total_api_calls_{0};
    mutable std::chrono::high_resolution_clock::time_point last_call_time_;

public:
    /**
     * @brief Construct plugin API
     */
    explicit PluginAPI(std::unique_ptr<PluginContext> context);
    
    /**
     * @brief Destructor
     */
    ~PluginAPI();
    
    // Non-copyable but movable
    PluginAPI(const PluginAPI&) = delete;
    PluginAPI& operator=(const PluginAPI&) = delete;
    PluginAPI(PluginAPI&&) = default;
    PluginAPI& operator=(PluginAPI&&) = default;
    
    /**
     * @brief Get plugin context
     */
    PluginContext& get_context() { return *context_; }
    const PluginContext& get_context() const { return *context_; }
    
    /**
     * @brief Get ECS integration helper
     */
    ECSIntegration& get_ecs() { return *ecs_integration_; }
    
    /**
     * @brief Get resource manager
     */
    ResourceManager& get_resources() { return context_->get_resource_manager(); }
    
    /**
     * @brief Get event bus
     */
    EventBus& get_events() { return context_->get_event_bus(); }
    
    /**
     * @brief Get plugin registry
     */
    PluginRegistry& get_registry() { return context_->get_plugin_registry(); }
    
    //-------------------------------------------------------------------------
    // Logging and Debug Utilities
    //-------------------------------------------------------------------------
    
    /**
     * @brief Log message with plugin context
     */
    template<typename... Args>
    void log_info(const std::string& format, Args&&... args);
    
    template<typename... Args>
    void log_warn(const std::string& format, Args&&... args);
    
    template<typename... Args>
    void log_error(const std::string& format, Args&&... args);
    
    template<typename... Args>
    void log_debug(const std::string& format, Args&&... args);
    
    //-------------------------------------------------------------------------
    // Service Management
    //-------------------------------------------------------------------------
    
    /**
     * @brief Register singleton service
     */
    template<typename ServiceType>
    bool register_service(const std::string& name, std::unique_ptr<ServiceType> service);
    
    /**
     * @brief Register factory service
     */
    template<typename ServiceType>
    bool register_factory(const std::string& name, 
                         std::function<std::unique_ptr<ServiceType>()> factory);
    
    /**
     * @brief Get service by name
     */
    template<typename ServiceType>
    ServiceType* get_service(const std::string& name);
    
    /**
     * @brief Create service instance from factory
     */
    template<typename ServiceType>
    std::unique_ptr<ServiceType> create_service(const std::string& name);
    
    //-------------------------------------------------------------------------
    // Configuration Management
    //-------------------------------------------------------------------------
    
    /**
     * @brief Get configuration value
     */
    std::string get_config(const std::string& key, const std::string& default_value = "");
    
    /**
     * @brief Set configuration value
     */
    void set_config(const std::string& key, const std::string& value);
    
    /**
     * @brief Get typed configuration value
     */
    template<typename T>
    T get_config_as(const std::string& key, const T& default_value = T{});
    
    /**
     * @brief Set typed configuration value
     */
    template<typename T>
    void set_config_as(const std::string& key, const T& value);
    
    //-------------------------------------------------------------------------
    // Performance and Profiling
    //-------------------------------------------------------------------------
    
    /**
     * @brief Start performance timer
     */
    class PerformanceTimer {
    private:
        std::chrono::high_resolution_clock::time_point start_time_;
        std::string operation_name_;
        PluginAPI& api_;
        
    public:
        PerformanceTimer(const std::string& name, PluginAPI& api) 
            : start_time_(std::chrono::high_resolution_clock::now())
            , operation_name_(name), api_(api) {}
        
        ~PerformanceTimer() {
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<f64, std::milli>(end_time - start_time_).count();
            api_.record_performance_metric(operation_name_, duration);
        }
    };
    
    PerformanceTimer start_timer(const std::string& operation_name) {
        return PerformanceTimer(operation_name, *this);
    }
    
    /**
     * @brief Record performance metric
     */
    void record_performance_metric(const std::string& metric_name, f64 value);
    
    /**
     * @brief Get performance metrics
     */
    std::unordered_map<std::string, f64> get_performance_metrics() const;
    
    //-------------------------------------------------------------------------
    // Educational Features
    //-------------------------------------------------------------------------
    
    /**
     * @brief Add educational note
     */
    void add_learning_note(const std::string& note) {
        context_->add_learning_note(note);
    }
    
    /**
     * @brief Add code example
     */
    void add_code_example(const std::string& title, const std::string& code) {
        context_->add_code_example(title, code);
    }
    
    /**
     * @brief Explain concept
     */
    void explain_concept(const std::string& concept, const std::string& explanation);
    
    /**
     * @brief Demonstrate feature
     */
    void demonstrate_feature(const std::string& feature_name, 
                           std::function<void()> demonstration);
    
    /**
     * @brief Get learning progress
     */
    f32 get_learning_progress(const std::string& topic) const;
    
    //-------------------------------------------------------------------------
    // Utility Functions
    //-------------------------------------------------------------------------
    
    /**
     * @brief Get current time in milliseconds
     */
    f64 get_time_ms() const;
    
    /**
     * @brief Get frame delta time
     */
    f64 get_delta_time() const;
    
    /**
     * @brief Check if plugin has permission
     */
    bool has_permission(PluginPermission permission) const;
    
    /**
     * @brief Get plugin metadata
     */
    const PluginMetadata& get_metadata() const {
        return context_->get_metadata();
    }
    
    /**
     * @brief Get API usage statistics
     */
    struct APIStats {
        u64 total_api_calls;
        f64 average_call_time_ms;
        std::unordered_map<std::string, u64> calls_by_function;
        std::unordered_map<std::string, f64> performance_metrics;
    };
    
    APIStats get_api_stats() const;

private:
    void initialize_api();
    void cleanup_api();
    void track_api_call(const std::string& function_name) const;
    
    // Performance metrics storage
    mutable std::mutex metrics_mutex_;
    mutable std::unordered_map<std::string, f64> performance_metrics_;
    mutable std::unordered_map<std::string, u64> function_call_counts_;
};

//=============================================================================
// Template Implementations
//=============================================================================

template<typename T>
ResourceHandle<T> ResourceManager::load_resource(const std::string& name, 
                                                 const std::string& file_path,
                                                 ResourceType type) {
    std::unique_lock<std::shared_mutex> lock(resources_mutex_);
    
    // Check if resource already exists
    auto it = resources_.find(name);
    if (it != resources_.end()) {
        try {
            auto shared_ptr = std::any_cast<std::shared_ptr<T>>(it->second);
            return ResourceHandle<T>(shared_ptr, name);
        } catch (const std::bad_any_cast&) {
            // Resource exists but wrong type
            return ResourceHandle<T>();
        }
    }
    
    // Load the resource
    auto loader_it = loaders_.find(type);
    if (loader_it != loaders_.end()) {
        try {
            auto resource_any = loader_it->second(file_path);
            auto shared_ptr = std::any_cast<std::shared_ptr<T>>(resource_any);
            
            // Store resource and metadata
            resources_[name] = resource_any;
            
            ResourceMetadata metadata;
            metadata.name = name;
            metadata.file_path = file_path;
            metadata.type = type;
            metadata.created_time = std::chrono::system_clock::now();
            metadata.providing_plugin = plugin_name_;
            
            resource_metadata_[name] = metadata;
            resources_by_type_[type].push_back(name);
            
            return ResourceHandle<T>(shared_ptr, name);
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to load resource '{}': {}", name, e.what());
            return ResourceHandle<T>();
        }
    }
    
    return ResourceHandle<T>();
}

template<typename T>
ResourceHandle<T> ResourceManager::create_resource(const std::string& name, 
                                                   std::unique_ptr<T> resource,
                                                   ResourceType type) {
    std::unique_lock<std::shared_mutex> lock(resources_mutex_);
    
    auto shared_ptr = std::shared_ptr<T>(resource.release());
    resources_[name] = std::any(shared_ptr);
    
    ResourceMetadata metadata;
    metadata.name = name;
    metadata.type = type;
    metadata.created_time = std::chrono::system_clock::now();
    metadata.providing_plugin = plugin_name_;
    metadata.size = sizeof(T);
    
    resource_metadata_[name] = metadata;
    resources_by_type_[type].push_back(name);
    
    return ResourceHandle<T>(shared_ptr, name);
}

template<typename T>
ResourceHandle<T> ResourceManager::get_resource(const std::string& name) {
    std::shared_lock<std::shared_mutex> lock(resources_mutex_);
    
    auto it = resources_.find(name);
    if (it != resources_.end()) {
        try {
            auto shared_ptr = std::any_cast<std::shared_ptr<T>>(it->second);
            return ResourceHandle<T>(shared_ptr, name);
        } catch (const std::bad_any_cast&) {
            return ResourceHandle<T>();
        }
    }
    
    return ResourceHandle<T>();
}

template<typename EventType>
void EventBus::subscribe(EventListener<EventType> listener) {
    std::unique_lock<std::shared_mutex> lock(handlers_mutex_);
    
    std::type_index type_idx = std::type_index(typeid(EventType));
    
    auto wrapper = [listener](const std::any& event_any) {
        try {
            const EventType& event = std::any_cast<const EventType&>(event_any);
            listener(event);
        } catch (const std::bad_any_cast& e) {
            LOG_ERROR("Bad any_cast in event handler: {}", e.what());
        }
    };
    
    event_handlers_[type_idx].push_back(wrapper);
}

template<typename EventType>
void EventBus::publish(const EventType& event) {
    std::shared_lock<std::shared_mutex> lock(handlers_mutex_);
    
    std::type_index type_idx = std::type_index(typeid(EventType));
    
    auto handlers_it = event_handlers_.find(type_idx);
    if (handlers_it != event_handlers_.end()) {
        std::any event_any = event;
        
        // Apply filters first
        auto filters_it = event_filters_.find(type_idx);
        if (filters_it != event_filters_.end()) {
            for (const auto& filter : filters_it->second) {
                if (!filter(event_any)) {
                    return; // Event filtered out
                }
            }
        }
        
        // Call handlers
        for (const auto& handler : handlers_it->second) {
            handler(event_any);
        }
        
        events_handled_ += handlers_it->second.size();
    }
    
    events_published_++;
    events_by_type_[type_idx]++;
}

// More template implementations would continue here...

} // namespace ecscope::plugin