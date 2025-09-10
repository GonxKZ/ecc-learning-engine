#pragma once

#include "plugin_types.hpp"
#include <memory>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>

namespace ecscope {
    // Forward declarations for engine integration
    namespace ecs {
        class Registry;
        class Entity;
    }
    
    namespace rendering {
        class Renderer;
        class Texture;
        class Shader;
    }
    
    namespace physics {
        class World;
    }
    
    namespace assets {
        class AssetManager;
        class Asset;
    }
    
    namespace gui {
        class GuiContext;
    }
    
    namespace audio {
        class AudioSystem;
    }
    
    namespace networking {
        class NetworkManager;
    }
}

namespace ecscope::plugins {

// Forward declarations
class PluginManager;
class SecurityContext;

// =============================================================================
// Plugin API Interface
// =============================================================================

/// Comprehensive API interface provided to plugins for engine interaction
/// This class acts as a controlled gateway between plugins and the core engine
class PluginAPI {
public:
    explicit PluginAPI(std::shared_ptr<PluginManager> manager);
    virtual ~PluginAPI() = default;

    // =============================================================================
    // Core System Access
    // =============================================================================

    /// Get the plugin manager instance
    /// @return Shared pointer to the plugin manager
    std::shared_ptr<PluginManager> getPluginManager() const { return manager_; }

    /// Get current frame time in seconds
    /// @return Time elapsed since last frame
    double getDeltaTime() const;

    /// Get total elapsed time since engine start in seconds
    /// @return Total elapsed time
    double getTotalTime() const;

    /// Get current frame number
    /// @return Current frame number
    uint64_t getFrameNumber() const;

    // =============================================================================
    // Logging and Debugging
    // =============================================================================

    /// Log a message with specified level
    /// @param level Log level (INFO, WARN, ERROR, DEBUG)
    /// @param plugin_name Name of the plugin logging the message
    /// @param message The message to log
    void log(const std::string& level, const std::string& plugin_name, const std::string& message) const;

    /// Log info message
    void logInfo(const std::string& plugin_name, const std::string& message) const;
    
    /// Log warning message
    void logWarning(const std::string& plugin_name, const std::string& message) const;
    
    /// Log error message
    void logError(const std::string& plugin_name, const std::string& message) const;
    
    /// Log debug message (only shown in debug mode)
    void logDebug(const std::string& plugin_name, const std::string& message) const;

    /// Assert with message (debug builds only)
    void assertCondition(bool condition, const std::string& plugin_name, const std::string& message) const;

    // =============================================================================
    // ECS System Access
    // =============================================================================

    /// Get the ECS registry
    /// @return Shared pointer to the ECS registry, or nullptr if not available
    std::shared_ptr<ecs::Registry> getECSRegistry() const;

    /// Create a new entity
    /// @return New entity ID, or invalid entity if creation failed
    ecs::Entity createEntity() const;

    /// Destroy an entity
    /// @param entity The entity to destroy
    /// @return true if entity was destroyed successfully
    bool destroyEntity(ecs::Entity entity) const;

    /// Check if entity exists
    /// @param entity The entity to check
    /// @return true if entity exists
    bool entityExists(ecs::Entity entity) const;

    // Component access would be provided through template functions
    // but since we're focusing on the interface, we'll provide generic access

    /// Get component data as byte array
    /// @param entity The entity
    /// @param component_type Component type identifier
    /// @param[out] data Buffer to store component data
    /// @param[in,out] size Size of buffer / actual data size
    /// @return true if component retrieved successfully
    bool getComponentData(ecs::Entity entity, const std::string& component_type, 
                         void* data, size_t* size) const;

    /// Set component data from byte array
    /// @param entity The entity
    /// @param component_type Component type identifier
    /// @param data Component data buffer
    /// @param size Size of data buffer
    /// @return true if component set successfully
    bool setComponentData(ecs::Entity entity, const std::string& component_type, 
                         const void* data, size_t size) const;

    /// Remove component from entity
    /// @param entity The entity
    /// @param component_type Component type identifier
    /// @return true if component removed successfully
    bool removeComponent(ecs::Entity entity, const std::string& component_type) const;

    /// Check if entity has component
    /// @param entity The entity
    /// @param component_type Component type identifier
    /// @return true if entity has the component
    bool hasComponent(ecs::Entity entity, const std::string& component_type) const;

    // =============================================================================
    // Rendering System Access
    // =============================================================================

    /// Get the rendering system
    /// @return Shared pointer to the renderer, or nullptr if not available
    std::shared_ptr<rendering::Renderer> getRenderer() const;

    /// Load texture from file
    /// @param path Path to texture file
    /// @return Shared pointer to texture, or nullptr on failure
    std::shared_ptr<rendering::Texture> loadTexture(const std::string& path) const;

    /// Create shader from source
    /// @param vertex_source Vertex shader source code
    /// @param fragment_source Fragment shader source code
    /// @return Shared pointer to shader, or nullptr on failure
    std::shared_ptr<rendering::Shader> createShader(const std::string& vertex_source,
                                                    const std::string& fragment_source) const;

    /// Get current screen width
    /// @return Screen width in pixels
    uint32_t getScreenWidth() const;

    /// Get current screen height
    /// @return Screen height in pixels
    uint32_t getScreenHeight() const;

    /// Get current viewport dimensions
    /// @param[out] x Viewport X offset
    /// @param[out] y Viewport Y offset
    /// @param[out] width Viewport width
    /// @param[out] height Viewport height
    void getViewport(int32_t* x, int32_t* y, uint32_t* width, uint32_t* height) const;

    // =============================================================================
    // Physics System Access
    // =============================================================================

    /// Get the physics world
    /// @return Shared pointer to the physics world, or nullptr if not available
    std::shared_ptr<physics::World> getPhysicsWorld() const;

    /// Perform raycast in physics world
    /// @param start_x Ray start X coordinate
    /// @param start_y Ray start Y coordinate
    /// @param end_x Ray end X coordinate
    /// @param end_y Ray end Y coordinate
    /// @return Entity ID of first hit, or invalid entity if no hit
    ecs::Entity raycast(float start_x, float start_y, float end_x, float end_y) const;

    /// Get gravity vector
    /// @param[out] x Gravity X component
    /// @param[out] y Gravity Y component
    void getGravity(float* x, float* y) const;

    /// Set gravity vector
    /// @param x Gravity X component
    /// @param y Gravity Y component
    void setGravity(float x, float y) const;

    // =============================================================================
    // Asset Management
    // =============================================================================

    /// Get the asset manager
    /// @return Shared pointer to the asset manager, or nullptr if not available
    std::shared_ptr<assets::AssetManager> getAssetManager() const;

    /// Load asset by path
    /// @param path Asset file path
    /// @return Shared pointer to asset, or nullptr on failure
    std::shared_ptr<assets::Asset> loadAsset(const std::string& path) const;

    /// Get asset by ID
    /// @param id Asset ID
    /// @return Shared pointer to asset, or nullptr if not found
    std::shared_ptr<assets::Asset> getAsset(const std::string& id) const;

    /// Unload asset
    /// @param id Asset ID
    /// @return true if asset was unloaded successfully
    bool unloadAsset(const std::string& id) const;

    /// Check if asset is loaded
    /// @param id Asset ID
    /// @return true if asset is loaded
    bool isAssetLoaded(const std::string& id) const;

    // =============================================================================
    // GUI System Access
    // =============================================================================

    /// Get the GUI context
    /// @return Shared pointer to the GUI context, or nullptr if not available
    std::shared_ptr<gui::GuiContext> getGuiContext() const;

    /// Begin GUI window
    /// @param name Window name
    /// @param open Window open state (can be modified)
    /// @return true if window is visible
    bool beginGuiWindow(const std::string& name, bool* open = nullptr) const;

    /// End GUI window
    void endGuiWindow() const;

    /// Create GUI button
    /// @param label Button label
    /// @return true if button was clicked
    bool guiButton(const std::string& label) const;

    /// Create GUI text input
    /// @param label Input label
    /// @param[in,out] text Text buffer
    /// @param buffer_size Size of text buffer
    /// @return true if text was modified
    bool guiInputText(const std::string& label, char* text, size_t buffer_size) const;

    /// Display GUI text
    /// @param text Text to display
    void guiText(const std::string& text) const;

    // =============================================================================
    // Audio System Access
    // =============================================================================

    /// Get the audio system
    /// @return Shared pointer to the audio system, or nullptr if not available
    std::shared_ptr<audio::AudioSystem> getAudioSystem() const;

    /// Play sound effect
    /// @param path Sound file path
    /// @param volume Volume (0.0 to 1.0)
    /// @return Sound handle for control, or 0 on failure
    uint32_t playSoundEffect(const std::string& path, float volume = 1.0f) const;

    /// Stop sound
    /// @param handle Sound handle returned by playSoundEffect
    void stopSound(uint32_t handle) const;

    /// Set master volume
    /// @param volume Volume (0.0 to 1.0)
    void setMasterVolume(float volume) const;

    // =============================================================================
    // Networking System Access
    // =============================================================================

    /// Get the network manager
    /// @return Shared pointer to the network manager, or nullptr if not available
    std::shared_ptr<networking::NetworkManager> getNetworkManager() const;

    /// Send network message
    /// @param target Target client/server ID
    /// @param data Message data
    /// @param size Size of message data
    /// @return true if message was sent successfully
    bool sendNetworkMessage(uint32_t target, const void* data, size_t size) const;

    /// Broadcast network message
    /// @param data Message data
    /// @param size Size of message data
    /// @return true if message was broadcast successfully
    bool broadcastNetworkMessage(const void* data, size_t size) const;

    /// Check if connected to network
    /// @return true if connected
    bool isNetworkConnected() const;

    // =============================================================================
    // File System Access (Sandboxed)
    // =============================================================================

    /// Read file contents (within plugin sandbox)
    /// @param path File path relative to plugin directory
    /// @param[out] data Buffer to store file contents
    /// @param[in,out] size Buffer size / actual file size
    /// @return true if file read successfully
    bool readFile(const std::string& path, void* data, size_t* size) const;

    /// Write file contents (within plugin sandbox)
    /// @param path File path relative to plugin directory
    /// @param data Data to write
    /// @param size Size of data
    /// @return true if file written successfully
    bool writeFile(const std::string& path, const void* data, size_t size) const;

    /// Check if file exists (within plugin sandbox)
    /// @param path File path relative to plugin directory
    /// @return true if file exists
    bool fileExists(const std::string& path) const;

    /// List files in directory (within plugin sandbox)
    /// @param path Directory path relative to plugin directory
    /// @return List of file names, empty on error
    std::vector<std::string> listFiles(const std::string& path) const;

    /// Create directory (within plugin sandbox)
    /// @param path Directory path relative to plugin directory
    /// @return true if directory created successfully
    bool createDirectory(const std::string& path) const;

    // =============================================================================
    // Configuration and Settings
    // =============================================================================

    /// Get engine configuration value
    /// @param key Configuration key
    /// @return Configuration value, or empty string if not found
    std::string getConfigValue(const std::string& key) const;

    /// Get plugin-specific configuration value
    /// @param plugin_name Plugin name
    /// @param key Configuration key
    /// @return Configuration value, or empty string if not found
    std::string getPluginConfigValue(const std::string& plugin_name, const std::string& key) const;

    /// Set plugin-specific configuration value
    /// @param plugin_name Plugin name
    /// @param key Configuration key
    /// @param value Configuration value
    /// @return true if value set successfully
    bool setPluginConfigValue(const std::string& plugin_name, const std::string& key, 
                             const std::string& value) const;

    // =============================================================================
    // Plugin Communication
    // =============================================================================

    /// Send message to another plugin
    /// @param plugin_name Target plugin name
    /// @param message_type Message type identifier
    /// @param data Message data
    /// @param size Size of message data
    /// @return true if message sent successfully
    bool sendPluginMessage(const std::string& plugin_name, const std::string& message_type,
                          const void* data, size_t size) const;

    /// Broadcast message to all plugins
    /// @param message_type Message type identifier
    /// @param data Message data
    /// @param size Size of message data
    /// @return true if message broadcast successfully
    bool broadcastPluginMessage(const std::string& message_type, const void* data, size_t size) const;

    /// Register message handler
    /// @param message_type Message type to handle
    /// @param handler Function to call when message is received
    /// @return true if handler registered successfully
    bool registerMessageHandler(const std::string& message_type,
                               std::function<void(const PluginMessage&)> handler) const;

    /// Unregister message handler
    /// @param message_type Message type to stop handling
    /// @return true if handler unregistered successfully
    bool unregisterMessageHandler(const std::string& message_type) const;

    // =============================================================================
    // Event System
    // =============================================================================

    /// Subscribe to engine event
    /// @param event_type Event type to subscribe to
    /// @param handler Function to call when event occurs
    /// @return true if subscription successful
    bool subscribeToEvent(const std::string& event_type,
                         std::function<void(const PluginEvent&)> handler) const;

    /// Unsubscribe from engine event
    /// @param event_type Event type to unsubscribe from
    /// @return true if unsubscription successful
    bool unsubscribeFromEvent(const std::string& event_type) const;

    /// Fire engine event
    /// @param event_type Event type
    /// @param data Event data
    /// @return true if event fired successfully
    bool fireEvent(const std::string& event_type,
                   const std::unordered_map<std::string, std::string>& data) const;

    // =============================================================================
    // Security and Permissions
    // =============================================================================

    /// Request permission for a capability
    /// @param capability Capability to request
    /// @param reason Human-readable reason for request
    /// @return true if permission granted
    bool requestPermission(PluginCapabilities capability, const std::string& reason) const;

    /// Check if plugin has permission for a capability
    /// @param capability Capability to check
    /// @return true if permission granted
    bool hasPermission(PluginCapabilities capability) const;

    /// Get current security context
    /// @return Shared pointer to security context
    std::shared_ptr<SecurityContext> getSecurityContext() const;

    // =============================================================================
    // Resource Monitoring
    // =============================================================================

    /// Get current resource usage
    /// @param type Resource type
    /// @return Current usage amount
    uint64_t getResourceUsage(ResourceType type) const;

    /// Get resource quota limit
    /// @param type Resource type
    /// @return Quota limit
    uint64_t getResourceLimit(ResourceType type) const;

    /// Report resource usage (for tracking)
    /// @param type Resource type
    /// @param amount Usage amount to report
    void reportResourceUsage(ResourceType type, uint64_t amount) const;

    // =============================================================================
    // Memory Management
    // =============================================================================

    /// Allocate memory through engine's allocator
    /// @param size Size in bytes
    /// @param alignment Alignment requirement
    /// @return Pointer to allocated memory, or nullptr on failure
    void* allocateMemory(size_t size, size_t alignment = sizeof(void*)) const;

    /// Free memory allocated through engine's allocator
    /// @param ptr Pointer to memory to free
    void freeMemory(void* ptr) const;

    /// Get memory usage statistics
    /// @param[out] total_allocated Total allocated bytes
    /// @param[out] total_freed Total freed bytes
    /// @param[out] current_usage Current memory usage
    void getMemoryStats(uint64_t* total_allocated, uint64_t* total_freed, uint64_t* current_usage) const;

    // =============================================================================
    // Threading and Concurrency
    // =============================================================================

    /// Execute function on main thread
    /// @param func Function to execute
    /// @return true if function queued successfully
    bool executeOnMainThread(std::function<void()> func) const;

    /// Execute function on background thread
    /// @param func Function to execute
    /// @return true if function queued successfully
    bool executeOnBackgroundThread(std::function<void()> func) const;

    /// Get current thread ID
    /// @return Thread ID
    uint64_t getCurrentThreadId() const;

    /// Check if current thread is main thread
    /// @return true if on main thread
    bool isMainThread() const;

protected:
    std::shared_ptr<PluginManager> manager_;

    // System references (set by plugin manager)
    std::weak_ptr<ecs::Registry> ecs_registry_;
    std::weak_ptr<rendering::Renderer> renderer_;
    std::weak_ptr<physics::World> physics_world_;
    std::weak_ptr<assets::AssetManager> asset_manager_;
    std::weak_ptr<gui::GuiContext> gui_context_;
    std::weak_ptr<audio::AudioSystem> audio_system_;
    std::weak_ptr<networking::NetworkManager> network_manager_;

    friend class PluginManager;

    /// Set system references (called by plugin manager)
    void setSystemReferences(
        std::weak_ptr<ecs::Registry> ecs,
        std::weak_ptr<rendering::Renderer> renderer,
        std::weak_ptr<physics::World> physics,
        std::weak_ptr<assets::AssetManager> assets,
        std::weak_ptr<gui::GuiContext> gui,
        std::weak_ptr<audio::AudioSystem> audio,
        std::weak_ptr<networking::NetworkManager> network
    );
};

// =============================================================================
// Plugin API Factory
// =============================================================================

/// Factory for creating plugin API instances
class PluginAPIFactory {
public:
    /// Create a plugin API instance
    /// @param manager Plugin manager instance
    /// @return Shared pointer to plugin API
    static std::shared_ptr<PluginAPI> create(std::shared_ptr<PluginManager> manager);

    /// Create a restricted plugin API instance (for sandboxing)
    /// @param manager Plugin manager instance
    /// @param capabilities Allowed capabilities
    /// @return Shared pointer to restricted plugin API
    static std::shared_ptr<PluginAPI> createRestricted(std::shared_ptr<PluginManager> manager,
                                                       PluginCapabilities capabilities);
};

} // namespace ecscope::plugins