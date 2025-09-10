#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <chrono>

namespace ecscope::plugins {

// Forward declarations
class PluginBase;
class PluginManager;
class SecurityContext;

// =============================================================================
// Core Plugin Types and Constants
// =============================================================================

/// Plugin API version - increment when breaking changes occur
constexpr uint32_t PLUGIN_API_VERSION = 1;

/// Maximum plugin name length
constexpr size_t MAX_PLUGIN_NAME_LENGTH = 64;

/// Maximum plugin version string length
constexpr size_t MAX_VERSION_STRING_LENGTH = 32;

/// Plugin loading priority levels
enum class PluginPriority : uint32_t {
    CRITICAL = 0,    ///< System-critical plugins (loaded first)
    HIGH = 100,      ///< High priority plugins
    NORMAL = 1000,   ///< Normal priority plugins
    LOW = 2000,      ///< Low priority plugins
    BACKGROUND = 3000 ///< Background plugins (loaded last)
};

/// Plugin lifecycle states
enum class PluginState : uint32_t {
    UNLOADED = 0,    ///< Plugin not loaded
    LOADING,         ///< Plugin is being loaded
    LOADED,          ///< Plugin loaded but not initialized
    INITIALIZING,    ///< Plugin is being initialized
    RUNNING,         ///< Plugin is active and running
    PAUSED,          ///< Plugin is paused
    SHUTTING_DOWN,   ///< Plugin is being shut down
    ERROR,           ///< Plugin encountered an error
    UNLOADING        ///< Plugin is being unloaded
};

/// Plugin capabilities flags
enum class PluginCapabilities : uint32_t {
    NONE = 0,
    ECS_COMPONENTS = 1 << 0,    ///< Plugin provides ECS components
    ECS_SYSTEMS = 1 << 1,       ///< Plugin provides ECS systems
    RENDERING = 1 << 2,         ///< Plugin provides rendering functionality
    PHYSICS = 1 << 3,           ///< Plugin provides physics functionality
    AUDIO = 1 << 4,             ///< Plugin provides audio functionality
    NETWORKING = 1 << 5,        ///< Plugin provides networking functionality
    SCRIPTING = 1 << 6,         ///< Plugin provides scripting functionality
    GUI = 1 << 7,               ///< Plugin provides GUI functionality
    ASSET_LOADING = 1 << 8,     ///< Plugin provides asset loading
    FILE_SYSTEM = 1 << 9,       ///< Plugin accesses file system
    NETWORK_ACCESS = 1 << 10,   ///< Plugin requires network access
    HARDWARE_ACCESS = 1 << 11,  ///< Plugin requires hardware access
    PRIVILEGED = 1 << 12        ///< Plugin requires elevated privileges
};

/// Enable bitwise operations for capabilities
inline PluginCapabilities operator|(PluginCapabilities a, PluginCapabilities b) {
    return static_cast<PluginCapabilities>(
        static_cast<uint32_t>(a) | static_cast<uint32_t>(b)
    );
}

inline PluginCapabilities operator&(PluginCapabilities a, PluginCapabilities b) {
    return static_cast<PluginCapabilities>(
        static_cast<uint32_t>(a) & static_cast<uint32_t>(b)
    );
}

inline bool hasCapability(PluginCapabilities caps, PluginCapabilities check) {
    return (caps & check) == check;
}

/// Plugin security levels
enum class SecurityLevel : uint32_t {
    UNRESTRICTED = 0,   ///< No restrictions (dangerous)
    TRUSTED,            ///< Trusted plugins with minimal restrictions
    SANDBOXED,          ///< Sandboxed with limited access
    ISOLATED            ///< Fully isolated execution
};

/// Resource quota types
enum class ResourceType : uint32_t {
    CPU_TIME,           ///< CPU execution time
    MEMORY_USAGE,       ///< Memory consumption
    FILE_IO,            ///< File I/O operations
    NETWORK_IO,         ///< Network I/O operations
    GPU_MEMORY,         ///< GPU memory usage
    RENDER_CALLS        ///< Rendering API calls
};

/// Plugin error codes
enum class PluginError : uint32_t {
    SUCCESS = 0,
    LOAD_FAILED,            ///< Failed to load plugin library
    SYMBOL_NOT_FOUND,       ///< Required symbol not found
    VERSION_MISMATCH,       ///< API version incompatible
    DEPENDENCY_MISSING,     ///< Required dependency missing
    SECURITY_VIOLATION,     ///< Security policy violation
    QUOTA_EXCEEDED,         ///< Resource quota exceeded
    INITIALIZATION_FAILED,  ///< Plugin initialization failed
    SHUTDOWN_TIMEOUT,       ///< Plugin shutdown timed out
    INVALID_MANIFEST,       ///< Plugin manifest invalid
    SIGNATURE_INVALID,      ///< Plugin signature invalid
    ALREADY_LOADED,         ///< Plugin already loaded
    NOT_FOUND,             ///< Plugin not found
    PERMISSION_DENIED      ///< Insufficient permissions
};

// =============================================================================
// Plugin Metadata Structures
// =============================================================================

/// Semantic version structure
struct Version {
    uint16_t major = 0;
    uint16_t minor = 0;
    uint16_t patch = 0;
    std::string prerelease;
    std::string build;

    /// Construct from version string (e.g., "1.2.3-beta+build123")
    explicit Version(const std::string& version_string = "0.0.0");

    /// Convert to string representation
    std::string toString() const;

    /// Compare versions (returns -1, 0, 1 for less, equal, greater)
    int compare(const Version& other) const;

    bool operator==(const Version& other) const { return compare(other) == 0; }
    bool operator!=(const Version& other) const { return compare(other) != 0; }
    bool operator<(const Version& other) const { return compare(other) < 0; }
    bool operator<=(const Version& other) const { return compare(other) <= 0; }
    bool operator>(const Version& other) const { return compare(other) > 0; }
    bool operator>=(const Version& other) const { return compare(other) >= 0; }
};

/// Plugin dependency specification
struct Dependency {
    std::string name;           ///< Plugin name
    Version min_version;        ///< Minimum required version
    Version max_version;        ///< Maximum compatible version
    bool optional = false;      ///< Whether dependency is optional
    
    /// Check if a version satisfies this dependency
    bool isSatisfiedBy(const Version& version) const;
};

/// Resource quota specification
struct ResourceQuota {
    ResourceType type;
    uint64_t limit;         ///< Maximum allowed value
    uint64_t warning;       ///< Warning threshold
    std::chrono::milliseconds duration{1000}; ///< Time window for quota
};

/// Plugin author information
struct AuthorInfo {
    std::string name;
    std::string email;
    std::string organization;
    std::string website;
};

/// Plugin manifest - comprehensive metadata
struct PluginManifest {
    // Basic information
    std::string name;
    std::string display_name;
    std::string description;
    Version version;
    std::vector<AuthorInfo> authors;
    
    // Technical specifications
    uint32_t api_version = PLUGIN_API_VERSION;
    std::string entry_point;        ///< Main plugin file
    PluginCapabilities capabilities = PluginCapabilities::NONE;
    PluginPriority priority = PluginPriority::NORMAL;
    SecurityLevel security_level = SecurityLevel::SANDBOXED;
    
    // Dependencies
    std::vector<Dependency> dependencies;
    std::vector<std::string> conflicts;    ///< Plugins that conflict with this one
    
    // Resource management
    std::vector<ResourceQuota> quotas;
    
    // Platform compatibility
    std::vector<std::string> supported_platforms;
    std::vector<std::string> required_features;
    
    // Asset information
    std::vector<std::string> asset_directories;
    std::vector<std::string> config_files;
    
    // Licensing and verification
    std::string license;
    std::string license_url;
    std::string signature;          ///< Digital signature for verification
    std::string checksum;           ///< File checksum for integrity
    
    // URLs and metadata
    std::string homepage;
    std::string repository;
    std::string documentation;
    std::vector<std::string> tags;
    
    /// Load manifest from JSON file
    static std::unique_ptr<PluginManifest> loadFromFile(const std::string& path);
    
    /// Save manifest to JSON file
    bool saveToFile(const std::string& path) const;
    
    /// Validate manifest completeness and consistency
    std::vector<std::string> validate() const;
};

// =============================================================================
// Plugin Events and Messages
// =============================================================================

/// Plugin event types
enum class PluginEventType : uint32_t {
    LOADED,             ///< Plugin was loaded
    UNLOADED,           ///< Plugin was unloaded
    INITIALIZED,        ///< Plugin was initialized
    SHUTDOWN,           ///< Plugin was shut down
    ERROR_OCCURRED,     ///< Plugin encountered an error
    DEPENDENCY_LOADED,  ///< A dependency was loaded
    SECURITY_VIOLATION, ///< Security policy violation
    QUOTA_WARNING,      ///< Resource quota warning
    QUOTA_EXCEEDED,     ///< Resource quota exceeded
    HOT_RELOAD,         ///< Plugin was hot-reloaded
    PAUSED,            ///< Plugin was paused
    RESUMED            ///< Plugin was resumed
};

/// Plugin event data
struct PluginEvent {
    PluginEventType type;
    std::string plugin_name;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
    std::unordered_map<std::string, std::string> data;
    
    PluginEvent(PluginEventType t, const std::string& name, const std::string& msg = "")
        : type(t), plugin_name(name), message(msg), timestamp(std::chrono::system_clock::now()) {}
};

/// Plugin message for inter-plugin communication
struct PluginMessage {
    std::string from;           ///< Sender plugin name
    std::string to;             ///< Target plugin name (empty for broadcast)
    std::string type;           ///< Message type identifier
    std::vector<uint8_t> data;  ///< Message payload
    std::chrono::system_clock::time_point timestamp;
    uint64_t id;                ///< Unique message ID
    
    PluginMessage(const std::string& sender, const std::string& target, const std::string& msg_type)
        : from(sender), to(target), type(msg_type), timestamp(std::chrono::system_clock::now()), id(generateId()) {}
        
private:
    static uint64_t generateId();
};

// =============================================================================
// Plugin Interface Function Signatures
// =============================================================================

/// Plugin entry point function signature
using PluginEntryPoint = PluginBase* (*)();

/// Plugin cleanup function signature  
using PluginCleanupPoint = void (*)(PluginBase*);

/// Plugin info query function signature
using PluginInfoQuery = const PluginManifest* (*)();

/// Plugin API query function signature
using PluginAPIQuery = uint32_t (*)();

// =============================================================================
// Plugin Statistics and Monitoring
// =============================================================================

/// Plugin performance statistics
struct PluginStats {
    // Timing information
    std::chrono::microseconds initialization_time{0};
    std::chrono::microseconds update_time{0};
    std::chrono::microseconds shutdown_time{0};
    std::chrono::system_clock::time_point load_time;
    
    // Resource usage
    uint64_t memory_usage = 0;          ///< Current memory usage in bytes
    uint64_t peak_memory_usage = 0;     ///< Peak memory usage in bytes
    uint64_t cpu_time = 0;              ///< Total CPU time in microseconds
    uint64_t file_io_bytes = 0;         ///< Total file I/O bytes
    uint64_t network_io_bytes = 0;      ///< Total network I/O bytes
    
    // API usage
    uint64_t api_calls = 0;             ///< Total API calls made
    uint64_t events_sent = 0;           ///< Events sent to other plugins
    uint64_t events_received = 0;       ///< Events received from other plugins
    uint64_t messages_sent = 0;         ///< Messages sent to other plugins
    uint64_t messages_received = 0;     ///< Messages received from other plugins
    
    // Error tracking
    uint32_t error_count = 0;           ///< Number of errors encountered
    uint32_t warning_count = 0;         ///< Number of warnings generated
    std::string last_error;             ///< Last error message
    std::chrono::system_clock::time_point last_error_time;
    
    /// Reset all statistics
    void reset();
    
    /// Get formatted statistics string
    std::string toString() const;
};

// =============================================================================
// Utility Functions
// =============================================================================

/// Convert plugin state to string
const char* pluginStateToString(PluginState state);

/// Convert plugin error to string
const char* pluginErrorToString(PluginError error);

/// Convert security level to string
const char* securityLevelToString(SecurityLevel level);

/// Convert capabilities to string list
std::vector<std::string> capabilitiesToStrings(PluginCapabilities caps);

/// Get current platform identifier
std::string getCurrentPlatform();

/// Check if current platform is supported
bool isPlatformSupported(const std::vector<std::string>& supported_platforms);

} // namespace ecscope::plugins