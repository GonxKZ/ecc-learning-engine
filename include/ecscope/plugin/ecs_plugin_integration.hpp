#pragma once

/**
 * @file plugin/ecs_plugin_integration.hpp
 * @brief ECScope ECS Plugin Integration - Seamless Plugin System Integration
 * 
 * Complete integration layer between the ECScope ECS system and the plugin framework.
 * This provides seamless interoperability, allowing plugins to extend ECS functionality
 * while maintaining performance and educational features.
 * 
 * Integration Features:
 * - Plugin component registration with ECS registry
 * - Plugin system integration with ECS update cycles
 * - Event bridging between plugin and ECS systems
 * - Memory management coordination
 * - Educational debugging and visualization
 * - Performance monitoring and optimization
 * 
 * @author ECScope Educational ECS Framework - Plugin System
 * @date 2024
 */

#include "plugin_core.hpp"
#include "plugin_api.hpp"
#include "plugin_manager.hpp"
#include "plugin_registry.hpp"
#include "core/types.hpp"
#include "ecs/registry.hpp"
#include "ecs/component.hpp"
#include "ecs/system.hpp"
#include "memory/arena.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <type_traits>

namespace ecscope::plugin {

//=============================================================================
// ECS Plugin Integration Configuration
//=============================================================================

/**
 * @brief Configuration for ECS plugin integration
 */
struct ECSPluginIntegrationConfig {
    // Component Integration
    bool auto_register_plugin_components{true};
    bool enable_component_hot_reload{true};
    bool validate_component_types{true};
    bool track_component_usage{true};
    
    // System Integration  
    bool auto_register_plugin_systems{true};
    bool enable_system_priority_sorting{true};
    bool enable_system_dependency_resolution{true};
    bool monitor_system_performance{true};
    
    // Memory Management
    bool use_shared_memory_pool{true};
    usize shared_pool_size{16 * MB};
    bool enable_memory_isolation{false};
    bool track_memory_per_plugin{true};
    
    // Event Integration
    bool enable_ecs_event_forwarding{true};
    bool enable_plugin_event_broadcasting{true};
    bool validate_event_handlers{true};
    
    // Educational Features
    bool enable_integration_visualization{true};
    bool track_learning_progress{true};
    bool generate_integration_reports{true};
    bool demonstrate_integration_patterns{true};
    
    // Performance and Debugging
    bool enable_performance_profiling{true};
    bool enable_debug_visualization{true};
    u32 max_debug_entities{1000};
    f32 performance_warning_threshold{16.0f}; // milliseconds
};

//=============================================================================
// Plugin Component Bridge
//=============================================================================

/**
 * @brief Bridge for integrating plugin components with ECS registry
 */
class PluginComponentBridge {
private:
    ecs::Registry* ecs_registry_;
    PluginRegistry* plugin_registry_;
    ECSPluginIntegrationConfig config_;
    
    // Component tracking
    std::unordered_map<std::string, std::string> component_to_plugin_;
    std::unordered_map<std::string, std::unordered_set<ecs::Entity>> component_entities_;
    std::unordered_map<std::string, u64> component_usage_stats_;
    
    // Memory management
    std::unique_ptr<memory::ArenaAllocator> shared_component_arena_;
    std::unordered_map<std::string, usize> component_memory_usage_;
    
    mutable std::shared_mutex bridge_mutex_;

public:
    /**
     * @brief Constructor
     */
    PluginComponentBridge(ecs::Registry& ecs_registry, 
                         PluginRegistry& plugin_registry,
                         const ECSPluginIntegrationConfig& config);
    
    /**
     * @brief Register plugin component with ECS
     */
    template<typename ComponentType>
    bool register_plugin_component(const std::string& component_name,
                                 const std::string& plugin_name,
                                 const std::string& description = "");
    
    /**
     * @brief Unregister plugin component
     */
    bool unregister_plugin_component(const std::string& component_name,
                                   const std::string& plugin_name);
    
    /**
     * @brief Create entity with plugin components
     */
    template<typename... Components>
    ecs::Entity create_entity_with_plugin_components(const std::string& plugin_name,
                                                    Components&&... components);
    
    /**
     * @brief Add plugin component to existing entity
     */
    template<typename ComponentType>
    bool add_plugin_component(ecs::Entity entity,
                            const std::string& plugin_name,
                            const ComponentType& component);
    
    /**
     * @brief Remove plugin component from entity
     */
    template<typename ComponentType>
    bool remove_plugin_component(ecs::Entity entity, const std::string& plugin_name);
    
    /**
     * @brief Get component usage statistics
     */
    struct ComponentUsageStats {
        u64 total_instances;
        u64 active_instances;
        u64 created_this_session;
        u64 destroyed_this_session;
        usize memory_usage;
        f64 average_access_time_ms;
        std::string providing_plugin;
    };
    
    std::unordered_map<std::string, ComponentUsageStats> get_component_usage_stats() const;
    
    /**
     * @brief Handle plugin unloading
     */
    void handle_plugin_unloading(const std::string& plugin_name);
    
    /**
     * @brief Validate component integrity
     */
    bool validate_components() const;
    
    /**
     * @brief Generate component integration report
     */
    std::string generate_component_report() const;

private:
    void initialize_shared_memory();
    void cleanup_plugin_components(const std::string& plugin_name);
    void update_usage_statistics(const std::string& component_name);
};

//=============================================================================
// Plugin System Bridge
//=============================================================================

/**
 * @brief Bridge for integrating plugin systems with ECS update cycle
 */
class PluginSystemBridge {
private:
    ecs::Registry* ecs_registry_;
    PluginRegistry* plugin_registry_;
    ECSPluginIntegrationConfig config_;
    
    // System management
    struct PluginSystemInfo {
        std::string system_name;
        std::string plugin_name;
        PluginPriority priority;
        std::function<void(ecs::Registry&, f64)> update_function;
        std::function<void(ecs::Registry&)> init_function;
        std::function<void(ecs::Registry&)> shutdown_function;
        std::vector<std::string> dependencies;
        bool is_active{true};
        bool is_educational{false};
        
        // Performance tracking
        f64 total_execution_time{0.0};
        u64 update_count{0};
        f64 average_execution_time{0.0};
        f64 max_execution_time{0.0};
    };
    
    std::unordered_map<std::string, std::unique_ptr<PluginSystemInfo>> plugin_systems_;
    std::vector<std::string> system_execution_order_;
    std::unordered_map<std::string, std::vector<std::string>> system_dependencies_;
    
    // Performance monitoring
    f64 total_system_time_{0.0};
    u64 total_updates_{0};
    std::chrono::high_resolution_clock::time_point last_update_time_;
    
    mutable std::shared_mutex systems_mutex_;

public:
    /**
     * @brief Constructor
     */
    PluginSystemBridge(ecs::Registry& ecs_registry,
                      PluginRegistry& plugin_registry, 
                      const ECSPluginIntegrationConfig& config);
    
    /**
     * @brief Register plugin system
     */
    bool register_plugin_system(const std::string& system_name,
                               const std::string& plugin_name,
                               std::function<void(ecs::Registry&, f64)> update_func,
                               PluginPriority priority = PluginPriority::Normal,
                               const std::vector<std::string>& dependencies = {});
    
    /**
     * @brief Register plugin system with full lifecycle
     */
    bool register_plugin_system_full(const std::string& system_name,
                                    const std::string& plugin_name,
                                    std::function<void(ecs::Registry&, f64)> update_func,
                                    std::function<void(ecs::Registry&)> init_func,
                                    std::function<void(ecs::Registry&)> shutdown_func,
                                    PluginPriority priority = PluginPriority::Normal,
                                    const std::vector<std::string>& dependencies = {});
    
    /**
     * @brief Unregister plugin system
     */
    bool unregister_plugin_system(const std::string& system_name, const std::string& plugin_name);
    
    /**
     * @brief Update all plugin systems
     */
    void update_plugin_systems(f64 delta_time);
    
    /**
     * @brief Initialize all plugin systems
     */
    void initialize_plugin_systems();
    
    /**
     * @brief Shutdown all plugin systems
     */
    void shutdown_plugin_systems();
    
    /**
     * @brief Get system execution order
     */
    std::vector<std::string> get_system_execution_order() const;
    
    /**
     * @brief Set system active state
     */
    bool set_system_active(const std::string& system_name, bool active);
    
    /**
     * @brief Get system performance metrics
     */
    struct SystemPerformanceMetrics {
        f64 average_execution_time_ms;
        f64 max_execution_time_ms;
        f64 total_execution_time_ms;
        u64 update_count;
        f64 performance_score; // 0-100
        bool performance_warning;
        std::string plugin_name;
    };
    
    std::unordered_map<std::string, SystemPerformanceMetrics> get_system_performance() const;
    
    /**
     * @brief Handle plugin unloading
     */
    void handle_plugin_unloading(const std::string& plugin_name);
    
    /**
     * @brief Generate system integration report
     */
    std::string generate_system_report() const;

private:
    void calculate_execution_order();
    void resolve_system_dependencies();
    bool validate_system_dependencies() const;
    void update_performance_metrics(const std::string& system_name, f64 execution_time);
};

//=============================================================================
// Plugin Event Bridge
//=============================================================================

/**
 * @brief Bridge for event communication between plugins and ECS
 */
class PluginEventBridge {
private:
    ecs::Registry* ecs_registry_;
    PluginRegistry* plugin_registry_;
    PluginManager* plugin_manager_;
    ECSPluginIntegrationConfig config_;
    
    // Event routing
    std::unordered_map<std::string, std::vector<std::string>> event_subscriptions_;
    std::queue<PluginEvent> event_queue_;
    std::atomic<u64> events_processed_{0};
    std::atomic<u64> events_forwarded_{0};
    
    mutable std::mutex event_queue_mutex_;

public:
    /**
     * @brief Constructor
     */
    PluginEventBridge(ecs::Registry& ecs_registry,
                     PluginRegistry& plugin_registry,
                     PluginManager& plugin_manager,
                     const ECSPluginIntegrationConfig& config);
    
    /**
     * @brief Forward ECS events to plugins
     */
    void forward_ecs_event(const PluginEvent& event);
    
    /**
     * @brief Forward plugin events to ECS
     */
    void forward_plugin_event(const std::string& sender_plugin, const PluginEvent& event);
    
    /**
     * @brief Subscribe plugin to ECS events
     */
    bool subscribe_plugin_to_ecs_events(const std::string& plugin_name, 
                                       PluginEventType event_type);
    
    /**
     * @brief Unsubscribe plugin from ECS events
     */
    bool unsubscribe_plugin_from_ecs_events(const std::string& plugin_name,
                                           PluginEventType event_type);
    
    /**
     * @brief Process event queue
     */
    void process_events();
    
    /**
     * @brief Get event statistics
     */
    struct EventBridgeStats {
        u64 events_processed;
        u64 events_forwarded;
        u64 events_queued;
        std::unordered_map<std::string, u64> events_by_plugin;
        std::unordered_map<PluginEventType, u64> events_by_type;
    };
    
    EventBridgeStats get_event_stats() const;
    
    /**
     * @brief Handle plugin unloading
     */
    void handle_plugin_unloading(const std::string& plugin_name);

private:
    void route_event_to_subscribers(const PluginEvent& event);
    std::string event_type_to_string(PluginEventType type) const;
};

//=============================================================================
// Main ECS Plugin Integration Manager
//=============================================================================

/**
 * @brief Main integration manager coordinating all plugin-ECS interactions
 */
class ECSPluginIntegrationManager {
private:
    ecs::Registry* ecs_registry_;
    PluginManager* plugin_manager_;
    ECSPluginIntegrationConfig config_;
    
    // Integration bridges
    std::unique_ptr<PluginComponentBridge> component_bridge_;
    std::unique_ptr<PluginSystemBridge> system_bridge_;
    std::unique_ptr<PluginEventBridge> event_bridge_;
    
    // Shared plugin registry
    std::unique_ptr<PluginRegistry> plugin_registry_;
    
    // Educational features
    std::vector<std::string> integration_tutorials_;
    std::unordered_map<std::string, std::string> best_practices_;
    
    // Performance monitoring
    std::atomic<bool> is_initialized_{false};
    std::chrono::high_resolution_clock::time_point creation_time_;
    std::atomic<u64> total_integrations_{0};

public:
    /**
     * @brief Constructor
     */
    explicit ECSPluginIntegrationManager(ecs::Registry& ecs_registry,
                                       PluginManager& plugin_manager,
                                       const ECSPluginIntegrationConfig& config = {});
    
    /**
     * @brief Destructor
     */
    ~ECSPluginIntegrationManager();
    
    /**
     * @brief Initialize integration manager
     */
    bool initialize();
    
    /**
     * @brief Shutdown integration manager
     */
    void shutdown();
    
    /**
     * @brief Update integration (called per frame)
     */
    void update(f64 delta_time);
    
    /**
     * @brief Get component bridge
     */
    PluginComponentBridge& get_component_bridge() { return *component_bridge_; }
    
    /**
     * @brief Get system bridge
     */
    PluginSystemBridge& get_system_bridge() { return *system_bridge_; }
    
    /**
     * @brief Get event bridge
     */
    PluginEventBridge& get_event_bridge() { return *event_bridge_; }
    
    /**
     * @brief Get plugin registry
     */
    PluginRegistry& get_plugin_registry() { return *plugin_registry_; }
    
    /**
     * @brief Handle plugin loaded event
     */
    void on_plugin_loaded(const std::string& plugin_name);
    
    /**
     * @brief Handle plugin unloading event
     */
    void on_plugin_unloading(const std::string& plugin_name);
    
    /**
     * @brief Create plugin API context
     */
    std::unique_ptr<PluginAPI> create_plugin_api_context(const std::string& plugin_name,
                                                        const PluginMetadata& metadata);
    
    /**
     * @brief Validate integration state
     */
    bool validate_integration() const;
    
    /**
     * @brief Get integration statistics
     */
    struct IntegrationStats {
        u64 total_plugin_components;
        u64 total_plugin_systems;
        u64 total_events_bridged;
        u64 active_plugins;
        f64 average_system_time_ms;
        usize total_plugin_memory_usage;
        f32 integration_efficiency_score;
    };
    
    IntegrationStats get_integration_stats() const;
    
    /**
     * @brief Generate comprehensive integration report
     */
    std::string generate_integration_report() const;
    
    /**
     * @brief Get educational integration tutorials
     */
    std::vector<std::string> get_integration_tutorials() const;
    
    /**
     * @brief Get integration best practices
     */
    std::unordered_map<std::string, std::string> get_best_practices() const;
    
    /**
     * @brief Update configuration
     */
    void update_configuration(const ECSPluginIntegrationConfig& config);
    
    /**
     * @brief Get current configuration
     */
    const ECSPluginIntegrationConfig& get_configuration() const { return config_; }

private:
    void initialize_bridges();
    void initialize_educational_content();
    void cleanup_all_integrations();
    void setup_plugin_event_handlers();
    void update_integration_statistics();
};

//=============================================================================
// Integration Utility Functions
//=============================================================================

/**
 * @brief Create default integration configuration
 */
ECSPluginIntegrationConfig create_default_integration_config();

/**
 * @brief Create educational integration configuration
 */
ECSPluginIntegrationConfig create_educational_integration_config();

/**
 * @brief Create performance-focused integration configuration
 */
ECSPluginIntegrationConfig create_performance_integration_config();

/**
 * @brief Validate plugin-ECS compatibility
 */
bool validate_plugin_ecs_compatibility(const PluginMetadata& metadata, 
                                      const ecs::Registry& registry);

/**
 * @brief Get integration memory usage breakdown
 */
struct IntegrationMemoryBreakdown {
    usize component_bridge_memory;
    usize system_bridge_memory;
    usize event_bridge_memory;
    usize plugin_registry_memory;
    usize shared_allocator_memory;
    usize total_memory;
};

IntegrationMemoryBreakdown get_integration_memory_breakdown(
    const ECSPluginIntegrationManager& manager);

//=============================================================================
// Template Implementations
//=============================================================================

template<typename ComponentType>
bool PluginComponentBridge::register_plugin_component(const std::string& component_name,
                                                     const std::string& plugin_name,
                                                     const std::string& description) {
    static_assert(std::is_base_of_v<ecs::ComponentBase, ComponentType>,
                  "ComponentType must derive from ecs::ComponentBase");
    
    std::unique_lock<std::shared_mutex> lock(bridge_mutex_);
    
    // Check if component already registered by another plugin
    auto existing_it = component_to_plugin_.find(component_name);
    if (existing_it != component_to_plugin_.end() && existing_it->second != plugin_name) {
        LOG_ERROR("Component '{}' already registered by plugin '{}'", 
                 component_name, existing_it->second);
        return false;
    }
    
    // Register with plugin registry
    if (!plugin_registry_->register_component<ComponentType>(component_name, plugin_name, 
                                                           description, config_.track_component_usage)) {
        LOG_ERROR("Failed to register component '{}' with plugin registry", component_name);
        return false;
    }
    
    // Track the registration
    component_to_plugin_[component_name] = plugin_name;
    component_entities_[component_name] = {};
    component_usage_stats_[component_name] = 0;
    
    // Initialize memory tracking
    if (config_.track_memory_per_plugin) {
        component_memory_usage_[component_name] = 0;
    }
    
    LOG_INFO("Successfully registered plugin component '{}' from plugin '{}'", 
             component_name, plugin_name);
    return true;
}

template<typename... Components>
ecs::Entity PluginComponentBridge::create_entity_with_plugin_components(
    const std::string& plugin_name, Components&&... components) {
    
    // Create entity using ECS registry
    ecs::Entity entity = ecs_registry_->create_entity(std::forward<Components>(components)...);
    
    if (entity != 0) {
        std::unique_lock<std::shared_mutex> lock(bridge_mutex_);
        
        // Track component usage for each component type
        ((component_entities_[typeid(Components).name()].insert(entity),
          update_usage_statistics(typeid(Components).name())), ...);
        
        LOG_DEBUG("Created entity {} with plugin components from '{}'", entity, plugin_name);
    }
    
    return entity;
}

template<typename ComponentType>
bool PluginComponentBridge::add_plugin_component(ecs::Entity entity,
                                                const std::string& plugin_name,
                                                const ComponentType& component) {
    
    if (!ecs_registry_->add_component(entity, component)) {
        return false;
    }
    
    std::unique_lock<std::shared_mutex> lock(bridge_mutex_);
    
    std::string component_name = typeid(ComponentType).name();
    component_entities_[component_name].insert(entity);
    update_usage_statistics(component_name);
    
    return true;
}

//=============================================================================
// Global Integration Manager
//=============================================================================

/**
 * @brief Get global ECS plugin integration manager
 */
ECSPluginIntegrationManager& get_ecs_plugin_integration_manager();

/**
 * @brief Initialize global ECS plugin integration
 */
bool initialize_ecs_plugin_integration(ecs::Registry& ecs_registry,
                                     PluginManager& plugin_manager,
                                     const ECSPluginIntegrationConfig& config = {});

/**
 * @brief Shutdown global ECS plugin integration
 */
void shutdown_ecs_plugin_integration();

} // namespace ecscope::plugin