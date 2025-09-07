#pragma once

/**
 * @file plugin/plugin_core.hpp
 * @brief ECScope Plugin System - Core Infrastructure
 * 
 * Production-ready plugin system providing dynamic loading, hot-swapping, versioning,
 * security, and comprehensive educational features. This is the foundation for the
 * complete plugin architecture.
 * 
 * Architecture Overview:
 * - Dynamic library loading (Windows DLL, Linux SO, macOS dylib)
 * - Comprehensive versioning and dependency management
 * - Memory isolation and security sandboxing
 * - Hot-swappable plugins with state preservation
 * - Event-driven communication between plugins
 * - Complete ECS integration with component/system plugins
 * 
 * Educational Features:
 * - Step-by-step plugin lifecycle visualization
 * - Security vulnerability demonstrations
 * - Performance impact analysis
 * - Memory usage tracking per plugin
 * - Plugin development tutorials
 * 
 * Security Features:
 * - API access control and permission system
 * - Memory usage limits per plugin
 * - Execution time limits and monitoring
 * - Plugin validation and signature verification
 * - Sandboxed execution environment
 * 
 * @author ECScope Educational ECS Framework - Plugin System
 * @date 2024
 */

#include "core/types.hpp"
#include "core/log.hpp"
#include "memory/arena.hpp"
#include "memory/pool.hpp"
#include "memory/memory_tracker.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <optional>
#include <variant>

// Cross-platform library loading
#ifdef _WIN32
    #include <windows.h>
    #define PLUGIN_EXPORT __declspec(dllexport)
    #define PLUGIN_IMPORT __declspec(dllimport)
    using LibraryHandle = HMODULE;
#elif __linux__ || __APPLE__
    #include <dlfcn.h>
    #define PLUGIN_EXPORT __attribute__((visibility("default")))
    #define PLUGIN_IMPORT
    using LibraryHandle = void*;
#endif

namespace ecscope::plugin {

//=============================================================================
// Plugin Version and Compatibility Management
//=============================================================================

/**
 * @brief Semantic versioning for plugins
 */
struct PluginVersion {
    u32 major{1};
    u32 minor{0};
    u32 patch{0};
    std::string pre_release;
    std::string build_metadata;
    
    PluginVersion() = default;
    PluginVersion(u32 maj, u32 min, u32 pat) : major(maj), minor(min), patch(pat) {}
    
    /**
     * @brief Parse version from string (e.g., "1.2.3-alpha+build.1")
     */
    static PluginVersion parse(const std::string& version_string);
    
    /**
     * @brief Convert to string representation
     */
    std::string to_string() const;
    
    /**
     * @brief Check compatibility with another version
     */
    bool is_compatible_with(const PluginVersion& other) const;
    
    /**
     * @brief Comparison operators
     */
    bool operator==(const PluginVersion& other) const;
    bool operator!=(const PluginVersion& other) const;
    bool operator<(const PluginVersion& other) const;
    bool operator<=(const PluginVersion& other) const;
    bool operator>(const PluginVersion& other) const;
    bool operator>=(const PluginVersion& other) const;
};

/**
 * @brief Plugin dependency specification
 */
struct PluginDependency {
    std::string plugin_name;
    PluginVersion min_version;
    PluginVersion max_version;
    bool is_optional{false};
    std::string reason; // Educational: why this dependency exists
    
    /**
     * @brief Check if a given version satisfies this dependency
     */
    bool is_satisfied_by(const PluginVersion& version) const;
};

//=============================================================================
// Plugin Metadata and Information
//=============================================================================

/**
 * @brief Plugin category for organization
 */
enum class PluginCategory {
    Core,           // Core engine functionality
    ECS,            // ECS components and systems
    Graphics,       // Rendering and graphics
    Physics,        // Physics simulation
    Audio,          // Audio processing
    Input,          // Input handling
    Network,        // Networking
    AI,             // Artificial intelligence
    Tools,          // Development tools
    Educational,    // Educational examples
    Custom          // User-defined plugins
};

/**
 * @brief Plugin execution priority
 */
enum class PluginPriority {
    Critical = 0,   // Must load first (e.g., memory management)
    High = 100,     // Important systems (e.g., core ECS)
    Normal = 200,   // Standard plugins
    Low = 300,      // Optional features
    Background = 400 // Background tasks
};

/**
 * @brief Comprehensive plugin metadata
 */
struct PluginMetadata {
    // Basic Information
    std::string name;
    std::string display_name;
    std::string description;
    PluginVersion version;
    std::string author;
    std::string license;
    std::string homepage;
    
    // Classification
    PluginCategory category{PluginCategory::Custom};
    PluginPriority priority{PluginPriority::Normal};
    std::vector<std::string> tags;
    
    // Dependencies
    std::vector<PluginDependency> dependencies;
    std::vector<std::string> conflicts; // Plugins that can't coexist
    
    // Compatibility
    PluginVersion min_engine_version;
    PluginVersion max_engine_version;
    std::vector<std::string> supported_platforms;
    
    // Security and Permissions
    std::vector<std::string> required_permissions;
    bool requires_network_access{false};
    bool requires_file_system_access{false};
    bool requires_system_calls{false};
    
    // Educational Information
    std::string educational_purpose;
    std::vector<std::string> learning_objectives;
    std::string difficulty_level; // "beginner", "intermediate", "advanced"
    std::vector<std::string> related_concepts;
    
    // Resource Requirements
    usize max_memory_usage{64 * MB}; // Maximum memory usage
    f64 max_cpu_usage{0.1}; // Maximum CPU usage (0-1)
    std::chrono::milliseconds max_load_time{5000}; // Maximum load time
    
    // Build and Distribution
    std::string build_date;
    std::string build_hash;
    std::string distribution_url;
    std::string checksum;
    
    /**
     * @brief Validate metadata completeness and consistency
     */
    bool validate() const;
    
    /**
     * @brief Load metadata from JSON file
     */
    static std::optional<PluginMetadata> from_json(const std::string& json_content);
    
    /**
     * @brief Convert metadata to JSON string
     */
    std::string to_json() const;
};

//=============================================================================
// Plugin State and Lifecycle Management
//=============================================================================

/**
 * @brief Plugin lifecycle states
 */
enum class PluginState {
    Unknown,        // State not determined
    Discovered,     // Plugin discovered but not loaded
    Loading,        // Currently being loaded
    Loaded,         // Successfully loaded
    Initializing,   // Being initialized
    Active,         // Fully active and running
    Paused,         // Temporarily paused
    Stopping,       // Being stopped
    Stopped,        // Stopped but still loaded
    Unloading,      // Being unloaded
    Unloaded,       // Successfully unloaded
    Failed,         // Failed to load or run
    Crashed,        // Crashed during execution
    Quarantined     // Isolated due to security issues
};

/**
 * @brief Plugin state transition information
 */
struct PluginStateTransition {
    PluginState from_state;
    PluginState to_state;
    std::chrono::system_clock::time_point timestamp;
    std::string reason;
    std::string error_message;
    f64 transition_time_ms{0.0}; // Time taken for transition
};

/**
 * @brief Plugin performance and resource usage statistics
 */
struct PluginStats {
    // Timing Information
    std::chrono::system_clock::time_point load_time;
    std::chrono::system_clock::time_point last_activity;
    f64 total_cpu_time_ms{0.0};
    f64 average_frame_time_ms{0.0};
    
    // Memory Usage
    usize current_memory_usage{0};
    usize peak_memory_usage{0};
    usize total_allocations{0};
    usize current_allocations{0};
    
    // Plugin Operations
    u64 total_function_calls{0};
    u64 total_events_handled{0};
    u64 total_errors{0};
    u64 total_warnings{0};
    
    // Performance Metrics
    f64 load_time_ms{0.0};
    f64 initialization_time_ms{0.0};
    f64 average_update_time_ms{0.0};
    u32 performance_score{100}; // 0-100 score
    
    void reset() {
        *this = PluginStats{};
    }
};

//=============================================================================
// Plugin Security and Sandboxing
//=============================================================================

/**
 * @brief Security permission types
 */
enum class PluginPermission {
    FileSystemRead,     // Read files
    FileSystemWrite,    // Write files
    NetworkAccess,      // Network operations
    SystemCalls,        // Execute system commands
    ECSAccess,          // Access ECS registry
    MemoryManagement,   // Custom memory allocation
    ThreadCreation,     // Create threads
    DynamicLoading,     // Load other libraries
    ConfigurationAccess, // Access engine configuration
    DebugAccess         // Access debugging features
};

/**
 * @brief Security context for plugin execution
 */
struct PluginSecurityContext {
    std::unordered_map<PluginPermission, bool> permissions;
    usize memory_limit{64 * MB};
    u32 thread_limit{4};
    std::chrono::milliseconds execution_timeout{1000};
    std::vector<std::string> allowed_file_paths;
    std::vector<std::string> allowed_network_hosts;
    bool enable_memory_protection{true};
    bool enable_stack_protection{true};
    
    /**
     * @brief Check if plugin has specific permission
     */
    bool has_permission(PluginPermission permission) const;
    
    /**
     * @brief Grant permission to plugin
     */
    void grant_permission(PluginPermission permission);
    
    /**
     * @brief Revoke permission from plugin
     */
    void revoke_permission(PluginPermission permission);
};

//=============================================================================
// Plugin Event System
//=============================================================================

/**
 * @brief Plugin event types
 */
enum class PluginEventType {
    // Lifecycle Events
    BeforeLoad,
    AfterLoad,
    BeforeUnload,
    AfterUnload,
    StateChanged,
    
    // Runtime Events
    Update,
    Render,
    ComponentAdded,
    ComponentRemoved,
    EntityCreated,
    EntityDestroyed,
    
    // System Events
    EngineStartup,
    EngineShutdown,
    ConfigurationChanged,
    ErrorOccurred,
    
    // Custom Events
    Custom
};

/**
 * @brief Base plugin event data
 */
struct PluginEvent {
    PluginEventType type;
    std::string plugin_name;
    std::chrono::system_clock::time_point timestamp;
    std::unordered_map<std::string, std::variant<std::string, i64, f64, bool>> data;
    
    PluginEvent(PluginEventType t, const std::string& name) 
        : type(t), plugin_name(name), timestamp(std::chrono::system_clock::now()) {}
    
    /**
     * @brief Set event data
     */
    template<typename T>
    void set_data(const std::string& key, const T& value) {
        if constexpr (std::is_same_v<T, std::string> || 
                      std::is_same_v<T, i64> || 
                      std::is_same_v<T, f64> || 
                      std::is_same_v<T, bool>) {
            data[key] = value;
        }
    }
    
    /**
     * @brief Get event data
     */
    template<typename T>
    std::optional<T> get_data(const std::string& key) const {
        auto it = data.find(key);
        if (it != data.end()) {
            if (auto* value = std::get_if<T>(&it->second)) {
                return *value;
            }
        }
        return std::nullopt;
    }
};

/**
 * @brief Plugin event handler function type
 */
using PluginEventHandler = std::function<void(const PluginEvent&)>;

//=============================================================================
// Core Plugin Interface
//=============================================================================

/**
 * @brief Abstract base interface for all plugins
 * 
 * This is the core interface that all plugins must implement. It provides
 * the basic lifecycle management and communication interface.
 */
class IPlugin {
public:
    virtual ~IPlugin() = default;
    
    /**
     * @brief Get plugin metadata
     */
    virtual const PluginMetadata& get_metadata() const = 0;
    
    /**
     * @brief Initialize plugin
     */
    virtual bool initialize() = 0;
    
    /**
     * @brief Shutdown plugin
     */
    virtual void shutdown() = 0;
    
    /**
     * @brief Update plugin (called every frame)
     */
    virtual void update(f64 delta_time) = 0;
    
    /**
     * @brief Handle plugin events
     */
    virtual void handle_event(const PluginEvent& event) = 0;
    
    /**
     * @brief Get plugin configuration
     */
    virtual std::unordered_map<std::string, std::string> get_config() const = 0;
    
    /**
     * @brief Set plugin configuration
     */
    virtual void set_config(const std::unordered_map<std::string, std::string>& config) = 0;
    
    /**
     * @brief Validate plugin state
     */
    virtual bool validate() const = 0;
    
    /**
     * @brief Get plugin statistics
     */
    virtual PluginStats get_stats() const = 0;
    
    /**
     * @brief Educational: Explain plugin functionality
     */
    virtual std::string explain_functionality() const = 0;
    
    /**
     * @brief Educational: Get learning resources
     */
    virtual std::vector<std::string> get_learning_resources() const = 0;
};

//=============================================================================
// Plugin Loading and Management
//=============================================================================

/**
 * @brief Plugin loading error types
 */
enum class PluginLoadError {
    FileNotFound,
    InvalidFormat,
    IncompatibleVersion,
    MissingDependencies,
    SecurityViolation,
    InitializationFailed,
    MemoryError,
    PermissionDenied,
    Timeout,
    AlreadyLoaded,
    Unknown
};

/**
 * @brief Plugin loading result
 */
struct PluginLoadResult {
    bool success{false};
    PluginLoadError error_code{PluginLoadError::Unknown};
    std::string error_message;
    f64 load_time_ms{0.0};
    PluginMetadata metadata;
    
    explicit operator bool() const { return success; }
};

/**
 * @brief Plugin entry point function types
 */
extern "C" {
    typedef IPlugin* (*CreatePluginFunc)();
    typedef void (*DestroyPluginFunc)(IPlugin*);
    typedef const char* (*GetPluginInfoFunc)(); // Returns JSON metadata
    typedef u32 (*GetPluginVersionFunc)();
    typedef bool (*ValidatePluginFunc)();
}

//=============================================================================
// Plugin Container and Management
//=============================================================================

/**
 * @brief Plugin container holding loaded plugin instance
 */
class PluginContainer {
private:
    std::string plugin_name_;
    std::string file_path_;
    LibraryHandle library_handle_;
    std::unique_ptr<IPlugin> plugin_instance_;
    PluginMetadata metadata_;
    PluginSecurityContext security_context_;
    std::atomic<PluginState> state_{PluginState::Unknown};
    PluginStats stats_;
    std::vector<PluginStateTransition> state_history_;
    
    // Function pointers
    CreatePluginFunc create_plugin_func_;
    DestroyPluginFunc destroy_plugin_func_;
    
    // Threading and synchronization
    mutable std::shared_mutex mutex_;
    std::atomic<bool> is_update_running_{false};
    
    // Memory tracking
    std::unique_ptr<memory::ArenaAllocator> plugin_memory_;
    memory::MemoryTracker::ScopeTracker memory_tracker_;

public:
    /**
     * @brief Construct empty plugin container
     */
    PluginContainer();
    
    /**
     * @brief Construct and load plugin from file
     */
    explicit PluginContainer(const std::string& file_path);
    
    /**
     * @brief Destructor
     */
    ~PluginContainer();
    
    // Non-copyable but movable
    PluginContainer(const PluginContainer&) = delete;
    PluginContainer& operator=(const PluginContainer&) = delete;
    PluginContainer(PluginContainer&&) = default;
    PluginContainer& operator=(PluginContainer&&) = default;
    
    /**
     * @brief Load plugin from file
     */
    PluginLoadResult load_from_file(const std::string& file_path);
    
    /**
     * @brief Unload plugin
     */
    bool unload();
    
    /**
     * @brief Initialize plugin
     */
    bool initialize();
    
    /**
     * @brief Shutdown plugin
     */
    void shutdown();
    
    /**
     * @brief Update plugin
     */
    void update(f64 delta_time);
    
    /**
     * @brief Send event to plugin
     */
    void handle_event(const PluginEvent& event);
    
    /**
     * @brief Get plugin state
     */
    PluginState get_state() const { return state_.load(); }
    
    /**
     * @brief Get plugin metadata
     */
    const PluginMetadata& get_metadata() const { return metadata_; }
    
    /**
     * @brief Get plugin statistics
     */
    PluginStats get_stats() const;
    
    /**
     * @brief Get state transition history
     */
    std::vector<PluginStateTransition> get_state_history() const;
    
    /**
     * @brief Get security context
     */
    const PluginSecurityContext& get_security_context() const { return security_context_; }
    
    /**
     * @brief Update security context
     */
    void set_security_context(const PluginSecurityContext& context);
    
    /**
     * @brief Check if plugin is active
     */
    bool is_active() const { return state_.load() == PluginState::Active; }
    
    /**
     * @brief Check if plugin is loaded
     */
    bool is_loaded() const { 
        auto state = state_.load();
        return state != PluginState::Unknown && 
               state != PluginState::Unloaded && 
               state != PluginState::Failed;
    }
    
    /**
     * @brief Get plugin instance (for advanced operations)
     */
    IPlugin* get_plugin_instance() const { return plugin_instance_.get(); }
    
    /**
     * @brief Get plugin name
     */
    const std::string& get_name() const { return plugin_name_; }
    
    /**
     * @brief Get plugin file path
     */
    const std::string& get_file_path() const { return file_path_; }

private:
    /**
     * @brief Change plugin state with validation
     */
    bool change_state(PluginState new_state, const std::string& reason = "");
    
    /**
     * @brief Validate state transition
     */
    bool is_valid_state_transition(PluginState from, PluginState to) const;
    
    /**
     * @brief Load library and symbols
     */
    bool load_library_symbols();
    
    /**
     * @brief Validate plugin security
     */
    bool validate_security() const;
    
    /**
     * @brief Setup memory protection
     */
    void setup_memory_protection();
    
    /**
     * @brief Update performance statistics
     */
    void update_stats();
};

//=============================================================================
// Utility Functions
//=============================================================================

/**
 * @brief Get plugin state name as string
 */
const char* plugin_state_to_string(PluginState state);

/**
 * @brief Get plugin category name as string
 */
const char* plugin_category_to_string(PluginCategory category);

/**
 * @brief Get plugin permission name as string
 */
const char* plugin_permission_to_string(PluginPermission permission);

/**
 * @brief Get system information for plugin compatibility
 */
struct SystemInfo {
    std::string platform;
    std::string architecture;
    PluginVersion engine_version;
    std::vector<std::string> supported_features;
};

SystemInfo get_system_info();

/**
 * @brief Check plugin file signature (for security)
 */
bool verify_plugin_signature(const std::string& file_path, const std::string& expected_hash);

/**
 * @brief Generate plugin template code
 */
std::string generate_plugin_template(const PluginMetadata& metadata);

} // namespace ecscope::plugin