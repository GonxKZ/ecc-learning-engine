#pragma once

#include "plugin_types.hpp"
#include <memory>
#include <string>
#include <functional>

namespace ecscope {
    // Forward declarations for engine integration
    namespace ecs {
        class Registry;
        class System;
    }
    
    namespace rendering {
        class Renderer;
    }
    
    namespace physics {
        class World;
    }
    
    namespace assets {
        class AssetManager;
    }
}

namespace ecscope::plugins {

// Forward declarations
class PluginAPI;
class SecurityContext;

// =============================================================================
// Plugin Base Interface
// =============================================================================

/// Base class for all plugins - provides lifecycle management and core functionality
class PluginBase {
public:
    virtual ~PluginBase() = default;

    // =============================================================================
    // Lifecycle Management
    // =============================================================================

    /// Initialize the plugin with the provided API and security context
    /// This is called after the plugin is loaded but before it's marked as running
    /// @param api Plugin API interface for engine communication
    /// @param security Security context for sandboxing and permissions
    /// @return PluginError::SUCCESS on success, error code on failure
    virtual PluginError initialize(std::shared_ptr<PluginAPI> api, 
                                   std::shared_ptr<SecurityContext> security) = 0;

    /// Shutdown the plugin and cleanup all resources
    /// This is called when the plugin is being unloaded
    /// @param timeout_ms Maximum time allowed for shutdown in milliseconds
    /// @return PluginError::SUCCESS on success, error code on failure
    virtual PluginError shutdown(uint32_t timeout_ms = 5000) = 0;

    /// Update the plugin - called each frame if the plugin is active
    /// @param delta_time Time elapsed since last update in seconds
    virtual void update(double delta_time) {}

    /// Render the plugin - called during rendering phase if applicable
    /// @param renderer Rendering interface
    virtual void render() {}

    // =============================================================================
    // Plugin Information
    // =============================================================================

    /// Get the plugin manifest
    virtual const PluginManifest& getManifest() const = 0;

    /// Get the current plugin state
    virtual PluginState getState() const = 0;

    /// Get plugin statistics
    virtual const PluginStats& getStats() const = 0;

    /// Get plugin-specific configuration
    virtual std::string getConfigValue(const std::string& key) const { return ""; }

    /// Set plugin-specific configuration
    virtual void setConfigValue(const std::string& key, const std::string& value) {}

    // =============================================================================
    // Event and Message Handling
    // =============================================================================

    /// Handle incoming plugin event
    /// @param event The plugin event to handle
    virtual void onPluginEvent(const PluginEvent& event) {}

    /// Handle incoming plugin message
    /// @param message The plugin message to handle
    virtual void onPluginMessage(const PluginMessage& message) {}

    /// Handle system events (window close, resize, etc.)
    /// @param event_type System event type
    /// @param data Event-specific data
    virtual void onSystemEvent(const std::string& event_type, 
                              const std::unordered_map<std::string, std::string>& data) {}

    // =============================================================================
    // Engine Integration Points
    // =============================================================================

    /// Called when ECS registry becomes available
    /// Override this to register custom components and systems
    /// @param registry The ECS registry
    virtual void onECSRegistryAvailable(std::shared_ptr<ecs::Registry> registry) {}

    /// Called when rendering system becomes available
    /// Override this to register custom renderers or render passes
    /// @param renderer The rendering system
    virtual void onRenderingSystemAvailable(std::shared_ptr<rendering::Renderer> renderer) {}

    /// Called when physics world becomes available
    /// Override this to register custom physics components or systems
    /// @param world The physics world
    virtual void onPhysicsWorldAvailable(std::shared_ptr<physics::World> world) {}

    /// Called when asset manager becomes available
    /// Override this to register custom asset loaders or processors
    /// @param asset_manager The asset management system
    virtual void onAssetManagerAvailable(std::shared_ptr<assets::AssetManager> asset_manager) {}

    // =============================================================================
    // Security and Sandboxing
    // =============================================================================

    /// Request permission for a specific capability
    /// @param capability The capability to request
    /// @param reason Human-readable reason for the request
    /// @return true if permission granted, false if denied
    virtual bool requestPermission(PluginCapabilities capability, const std::string& reason);

    /// Check if plugin has a specific capability
    /// @param capability The capability to check
    /// @return true if plugin has the capability, false otherwise
    virtual bool hasCapability(PluginCapabilities capability) const;

    /// Get current security context
    /// @return The security context, or nullptr if not available
    virtual std::shared_ptr<SecurityContext> getSecurityContext() const;

    // =============================================================================
    // Resource Management
    // =============================================================================

    /// Get current resource usage
    /// @param type The resource type to query
    /// @return Current usage amount, or 0 if not tracked
    virtual uint64_t getResourceUsage(ResourceType type) const;

    /// Get resource quota limit
    /// @param type The resource type to query
    /// @return Quota limit, or 0 if no limit set
    virtual uint64_t getResourceQuota(ResourceType type) const;

    /// Check if resource usage is within quota
    /// @param type The resource type to check
    /// @return true if within quota, false if exceeded
    virtual bool isWithinQuota(ResourceType type) const;

    // =============================================================================
    // Debugging and Diagnostics
    // =============================================================================

    /// Get debug information as key-value pairs
    /// Override this to provide plugin-specific debug info
    /// @return Debug information map
    virtual std::unordered_map<std::string, std::string> getDebugInfo() const {
        return {};
    }

    /// Enable or disable debug mode
    /// @param enabled true to enable debug mode, false to disable
    virtual void setDebugMode(bool enabled) {
        debug_mode_enabled_ = enabled;
    }

    /// Check if debug mode is enabled
    /// @return true if debug mode is enabled
    virtual bool isDebugModeEnabled() const {
        return debug_mode_enabled_;
    }

    /// Log a debug message (only shown when debug mode is enabled)
    /// @param level Log level (INFO, WARN, ERROR)
    /// @param message The message to log
    virtual void logDebug(const std::string& level, const std::string& message) const;

    // =============================================================================
    // Plugin Communication
    // =============================================================================

    /// Send message to another plugin
    /// @param target Target plugin name (empty for broadcast)
    /// @param type Message type identifier
    /// @param data Message data
    /// @return true if message was sent successfully
    virtual bool sendMessage(const std::string& target, const std::string& type, 
                           const std::vector<uint8_t>& data);

    /// Send message to another plugin (string variant)
    /// @param target Target plugin name (empty for broadcast)
    /// @param type Message type identifier
    /// @param data Message data as string
    /// @return true if message was sent successfully
    virtual bool sendMessage(const std::string& target, const std::string& type, 
                           const std::string& data);

    /// Broadcast event to all interested plugins
    /// @param event_type Event type identifier
    /// @param data Event data
    /// @return true if event was broadcast successfully
    virtual bool broadcastEvent(const std::string& event_type, 
                              const std::unordered_map<std::string, std::string>& data);

protected:
    // =============================================================================
    // Protected Helper Methods
    // =============================================================================

    /// Update plugin statistics
    /// @param stats Statistics structure to update
    virtual void updateStats(PluginStats& stats) const;

    /// Validate plugin state transition
    /// @param from Current state
    /// @param to Target state
    /// @return true if transition is valid
    virtual bool isValidStateTransition(PluginState from, PluginState to) const;

    /// Called when plugin state changes
    /// @param old_state Previous state
    /// @param new_state Current state
    virtual void onStateChanged(PluginState old_state, PluginState new_state) {}

    // Member variables accessible by derived classes
    std::shared_ptr<PluginAPI> api_;
    std::shared_ptr<SecurityContext> security_;
    bool debug_mode_enabled_ = false;
};

// =============================================================================
// Plugin Registration Macros
// =============================================================================

/// Macro to simplify plugin registration
/// Use this in your plugin's main source file
#define ECSCOPE_REGISTER_PLUGIN(PluginClass, ManifestInstance) \
    extern "C" { \
        ECSCOPE_PLUGIN_EXPORT ecscope::plugins::PluginBase* ecscope_plugin_create() { \
            return new PluginClass(); \
        } \
        \
        ECSCOPE_PLUGIN_EXPORT void ecscope_plugin_destroy(ecscope::plugins::PluginBase* plugin) { \
            delete plugin; \
        } \
        \
        ECSCOPE_PLUGIN_EXPORT const ecscope::plugins::PluginManifest* ecscope_plugin_get_manifest() { \
            static const auto manifest = ManifestInstance; \
            return &manifest; \
        } \
        \
        ECSCOPE_PLUGIN_EXPORT uint32_t ecscope_plugin_get_api_version() { \
            return ecscope::plugins::PLUGIN_API_VERSION; \
        } \
    }

/// Platform-specific export macro
#ifdef _WIN32
    #define ECSCOPE_PLUGIN_EXPORT __declspec(dllexport)
#else
    #define ECSCOPE_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

// =============================================================================
// Plugin Helper Base Classes
// =============================================================================

/// Base class for ECS-focused plugins
class ECSPlugin : public PluginBase {
public:
    /// Called when ECS registry becomes available
    void onECSRegistryAvailable(std::shared_ptr<ecs::Registry> registry) override;

protected:
    /// Register custom components - override this
    virtual void registerComponents(std::shared_ptr<ecs::Registry> registry) {}
    
    /// Register custom systems - override this
    virtual void registerSystems(std::shared_ptr<ecs::Registry> registry) {}

    std::shared_ptr<ecs::Registry> ecs_registry_;
};

/// Base class for rendering-focused plugins
class RenderingPlugin : public PluginBase {
public:
    /// Called when rendering system becomes available
    void onRenderingSystemAvailable(std::shared_ptr<rendering::Renderer> renderer) override;
    
    /// Render the plugin
    void render() override;

protected:
    /// Initialize rendering resources - override this
    virtual void initializeRendering(std::shared_ptr<rendering::Renderer> renderer) {}
    
    /// Render plugin content - override this
    virtual void renderContent(std::shared_ptr<rendering::Renderer> renderer) {}
    
    /// Cleanup rendering resources - override this
    virtual void cleanupRendering(std::shared_ptr<rendering::Renderer> renderer) {}

    std::shared_ptr<rendering::Renderer> renderer_;
};

/// Base class for physics-focused plugins
class PhysicsPlugin : public PluginBase {
public:
    /// Called when physics world becomes available
    void onPhysicsWorldAvailable(std::shared_ptr<physics::World> world) override;

protected:
    /// Initialize physics components - override this
    virtual void initializePhysics(std::shared_ptr<physics::World> world) {}
    
    /// Update physics - override this
    virtual void updatePhysics(std::shared_ptr<physics::World> world, double delta_time) {}

    std::shared_ptr<physics::World> physics_world_;
};

/// Base class for asset-focused plugins
class AssetPlugin : public PluginBase {
public:
    /// Called when asset manager becomes available
    void onAssetManagerAvailable(std::shared_ptr<assets::AssetManager> asset_manager) override;

protected:
    /// Register asset loaders - override this
    virtual void registerAssetLoaders(std::shared_ptr<assets::AssetManager> asset_manager) {}
    
    /// Register asset processors - override this
    virtual void registerAssetProcessors(std::shared_ptr<assets::AssetManager> asset_manager) {}

    std::shared_ptr<assets::AssetManager> asset_manager_;
};

} // namespace ecscope::plugins