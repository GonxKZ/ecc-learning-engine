#pragma once

#include "plugin_types.hpp"
#include <memory>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <functional>
#include <atomic>
#include <mutex>

namespace ecscope::plugins {

// Forward declarations
class PluginManager;

// =============================================================================
// Security Context - Plugin Sandboxing and Permission Management
// =============================================================================

/// Security context for plugin sandboxing, permission management, and resource control
class SecurityContext {
public:
    // =============================================================================
    // Security Policy Structure
    // =============================================================================

    struct SecurityPolicy {
        SecurityLevel level = SecurityLevel::SANDBOXED;
        PluginCapabilities allowed_capabilities = PluginCapabilities::NONE;
        
        // File system restrictions
        std::vector<std::string> allowed_paths;         ///< Paths plugin can access
        std::vector<std::string> blocked_paths;         ///< Explicitly blocked paths
        bool allow_file_creation = false;              ///< Allow creating new files
        bool allow_file_deletion = false;              ///< Allow deleting files
        bool allow_directory_creation = false;         ///< Allow creating directories
        uint64_t max_file_size = 10 * 1024 * 1024;    ///< Maximum file size (10MB)
        
        // Network restrictions
        std::vector<std::string> allowed_hosts;         ///< Allowed network hosts
        std::vector<uint16_t> allowed_ports;           ///< Allowed network ports
        bool allow_outbound_connections = false;       ///< Allow outbound connections
        bool allow_inbound_connections = false;        ///< Allow inbound connections
        uint64_t max_bandwidth_per_second = 1024 * 1024; ///< Max bandwidth (1MB/s)
        
        // System access restrictions
        bool allow_process_creation = false;           ///< Allow spawning processes
        bool allow_dll_loading = false;                ///< Allow loading additional DLLs
        bool allow_registry_access = false;            ///< Allow Windows registry access
        bool allow_hardware_access = false;            ///< Allow direct hardware access
        bool allow_kernel_calls = false;               ///< Allow kernel system calls
        
        // Resource limits
        std::unordered_map<ResourceType, ResourceQuota> resource_quotas;
        
        // Time restrictions
        std::chrono::milliseconds max_execution_time{100}; ///< Max time per frame
        std::chrono::milliseconds max_blocking_time{10};   ///< Max time for blocking operations
        
        // Memory restrictions
        uint64_t max_heap_size = 64 * 1024 * 1024;     ///< Maximum heap allocation (64MB)
        uint64_t max_stack_size = 1024 * 1024;         ///< Maximum stack size (1MB)
        uint32_t max_thread_count = 4;                 ///< Maximum thread count
        
        // Code execution restrictions
        bool allow_code_generation = false;            ///< Allow JIT compilation
        bool allow_shellcode_execution = false;        ///< Allow shellcode execution
        bool allow_reflection = false;                 ///< Allow reflection/introspection
        
        /// Load policy from JSON file
        static SecurityPolicy loadFromFile(const std::string& path);
        
        /// Save policy to JSON file
        bool saveToFile(const std::string& path) const;
        
        /// Create default policy for security level
        static SecurityPolicy createDefault(SecurityLevel level);
        
        /// Merge another policy into this one (more restrictive wins)
        SecurityPolicy& merge(const SecurityPolicy& other);
        
        /// Validate policy consistency
        std::vector<std::string> validate() const;
    };

    // =============================================================================
    // Permission Record
    // =============================================================================

    struct PermissionRecord {
        PluginCapabilities capability;
        std::string reason;
        std::chrono::system_clock::time_point granted_time;
        std::chrono::system_clock::time_point expires_time;
        std::string granted_by;
        bool temporary = false;
    };

    // =============================================================================
    // Security Violation
    // =============================================================================

    struct SecurityViolation {
        std::string plugin_name;
        std::string violation_type;
        std::string description;
        std::chrono::system_clock::time_point timestamp;
        PluginCapabilities attempted_capability;
        std::string stack_trace;
        uint32_t severity = 0; ///< 0=info, 1=warning, 2=error, 3=critical
    };

    // =============================================================================
    // Constructor and Lifecycle
    // =============================================================================

    explicit SecurityContext(const std::string& plugin_name, const SecurityPolicy& policy);
    ~SecurityContext() = default;

    /// Initialize security context
    /// @return true if initialization successful
    bool initialize();

    /// Cleanup security context
    void cleanup();

    // =============================================================================
    // Permission Management
    // =============================================================================

    /// Check if plugin has permission for capability
    /// @param capability Capability to check
    /// @return true if permission granted
    bool hasPermission(PluginCapabilities capability) const;

    /// Request permission for capability
    /// @param capability Capability to request
    /// @param reason Reason for request
    /// @param temporary Whether permission is temporary
    /// @return true if permission granted
    bool requestPermission(PluginCapabilities capability, const std::string& reason, bool temporary = false);

    /// Grant permission for capability
    /// @param capability Capability to grant
    /// @param reason Reason for grant
    /// @param grantor Who granted the permission
    /// @param expires_in Expiration time (0 for permanent)
    /// @return true if permission granted successfully
    bool grantPermission(PluginCapabilities capability, const std::string& reason, 
                        const std::string& grantor = "system", 
                        std::chrono::milliseconds expires_in = std::chrono::milliseconds{0});

    /// Revoke permission for capability
    /// @param capability Capability to revoke
    /// @return true if permission revoked successfully
    bool revokePermission(PluginCapabilities capability);

    /// Get all granted permissions
    /// @return List of permission records
    std::vector<PermissionRecord> getGrantedPermissions() const;

    /// Check if permission is expired
    /// @param capability Capability to check
    /// @return true if permission exists and is expired
    bool isPermissionExpired(PluginCapabilities capability) const;

    /// Cleanup expired permissions
    void cleanupExpiredPermissions();

    // =============================================================================
    // File System Security
    // =============================================================================

    /// Check if path is allowed for reading
    /// @param path File path to check
    /// @return true if read access allowed
    bool canReadPath(const std::string& path) const;

    /// Check if path is allowed for writing
    /// @param path File path to check
    /// @return true if write access allowed
    bool canWritePath(const std::string& path) const;

    /// Check if path is allowed for execution
    /// @param path File path to check
    /// @return true if execute access allowed
    bool canExecutePath(const std::string& path) const;

    /// Check if file creation is allowed in path
    /// @param path Directory path to check
    /// @return true if file creation allowed
    bool canCreateFile(const std::string& path) const;

    /// Check if file deletion is allowed
    /// @param path File path to check
    /// @return true if file deletion allowed
    bool canDeleteFile(const std::string& path) const;

    /// Get sandbox directory for plugin
    /// @return Path to plugin sandbox directory
    std::string getSandboxDirectory() const;

    /// Resolve path relative to sandbox
    /// @param relative_path Path relative to sandbox
    /// @return Absolute path within sandbox
    std::string resolveSandboxPath(const std::string& relative_path) const;

    // =============================================================================
    // Network Security
    // =============================================================================

    /// Check if connection to host is allowed
    /// @param host Hostname or IP address
    /// @param port Port number
    /// @return true if connection allowed
    bool canConnectToHost(const std::string& host, uint16_t port) const;

    /// Check if binding to port is allowed
    /// @param port Port number
    /// @return true if binding allowed
    bool canBindToPort(uint16_t port) const;

    /// Check if network bandwidth limit is exceeded
    /// @param bytes_to_transfer Number of bytes to transfer
    /// @return true if transfer would exceed limit
    bool wouldExceedBandwidthLimit(uint64_t bytes_to_transfer) const;

    /// Report network usage
    /// @param bytes_transferred Number of bytes transferred
    void reportNetworkUsage(uint64_t bytes_transferred);

    // =============================================================================
    // Resource Monitoring and Quotas
    // =============================================================================

    /// Check if resource usage is within quota
    /// @param type Resource type
    /// @param usage Current usage amount
    /// @return true if within quota
    bool isWithinResourceQuota(ResourceType type, uint64_t usage) const;

    /// Report resource usage
    /// @param type Resource type
    /// @param amount Usage amount to report
    void reportResourceUsage(ResourceType type, uint64_t amount);

    /// Get current resource usage
    /// @param type Resource type
    /// @return Current usage amount
    uint64_t getResourceUsage(ResourceType type) const;

    /// Get resource quota limit
    /// @param type Resource type
    /// @return Quota limit, or 0 if no limit
    uint64_t getResourceLimit(ResourceType type) const;

    /// Check if any resource quota is exceeded
    /// @return true if any quota is exceeded
    bool hasResourceViolations() const;

    /// Get list of violated resource quotas
    /// @return List of resource types that exceeded quotas
    std::vector<ResourceType> getResourceViolations() const;

    // =============================================================================
    // Code Execution Security
    // =============================================================================

    /// Check if dynamic code generation is allowed
    /// @return true if code generation allowed
    bool canGenerateCode() const;

    /// Check if additional DLL loading is allowed
    /// @param dll_path Path to DLL to load
    /// @return true if DLL loading allowed
    bool canLoadDLL(const std::string& dll_path) const;

    /// Check if process creation is allowed
    /// @param executable_path Path to executable
    /// @return true if process creation allowed
    bool canCreateProcess(const std::string& executable_path) const;

    /// Check if system call is allowed
    /// @param syscall_name System call name
    /// @return true if system call allowed
    bool canMakeSystemCall(const std::string& syscall_name) const;

    // =============================================================================
    // Security Violations and Logging
    // =============================================================================

    /// Report security violation
    /// @param violation_type Type of violation
    /// @param description Description of violation
    /// @param attempted_capability Capability that was attempted
    /// @param severity Severity level (0-3)
    void reportViolation(const std::string& violation_type, const std::string& description,
                        PluginCapabilities attempted_capability = PluginCapabilities::NONE,
                        uint32_t severity = 2);

    /// Get all security violations
    /// @return List of security violations
    std::vector<SecurityViolation> getViolations() const;

    /// Get violation count for specific type
    /// @param violation_type Violation type
    /// @return Number of violations of this type
    uint32_t getViolationCount(const std::string& violation_type) const;

    /// Clear all violation records
    void clearViolations();

    /// Check if plugin should be blocked due to violations
    /// @return true if plugin should be blocked
    bool shouldBlockPlugin() const;

    // =============================================================================
    // Security Context Information
    // =============================================================================

    /// Get plugin name
    /// @return Plugin name
    const std::string& getPluginName() const { return plugin_name_; }

    /// Get security policy
    /// @return Security policy
    const SecurityPolicy& getPolicy() const { return policy_; }

    /// Get security level
    /// @return Security level
    SecurityLevel getSecurityLevel() const { return policy_.level; }

    /// Update security policy
    /// @param new_policy New security policy
    /// @return true if policy updated successfully
    bool updatePolicy(const SecurityPolicy& new_policy);

    /// Export security state to JSON
    /// @return JSON string representing security state
    std::string exportState() const;

    /// Import security state from JSON
    /// @param json_state JSON string representing security state
    /// @return true if state imported successfully
    bool importState(const std::string& json_state);

    // =============================================================================
    // Security Event Callbacks
    // =============================================================================

    /// Set permission request callback
    /// @param callback Function to call when permission is requested
    void setPermissionRequestCallback(std::function<bool(const std::string&, PluginCapabilities, const std::string&)> callback);

    /// Set violation callback
    /// @param callback Function to call when violation occurs
    void setViolationCallback(std::function<void(const SecurityViolation&)> callback);

    /// Set resource quota warning callback
    /// @param callback Function to call when resource usage reaches warning threshold
    void setResourceWarningCallback(std::function<void(ResourceType, uint64_t, uint64_t)> callback);

    // =============================================================================
    // Advanced Security Features
    // =============================================================================

    /// Enable security monitoring
    /// @param enabled true to enable monitoring
    void setSecurityMonitoring(bool enabled);

    /// Check if security monitoring is enabled
    /// @return true if monitoring enabled
    bool isSecurityMonitoringEnabled() const;

    /// Set execution time limit
    /// @param limit Maximum execution time per frame
    void setExecutionTimeLimit(std::chrono::milliseconds limit);

    /// Check if execution time limit would be exceeded
    /// @param duration Execution duration to check
    /// @return true if limit would be exceeded
    bool wouldExceedExecutionTimeLimit(std::chrono::milliseconds duration) const;

    /// Start execution time tracking
    void startExecutionTracking();

    /// Stop execution time tracking
    /// @return Execution time since start
    std::chrono::milliseconds stopExecutionTracking();

    /// Get total execution time
    /// @return Total execution time
    std::chrono::milliseconds getTotalExecutionTime() const;

private:
    // =============================================================================
    // Member Variables
    // =============================================================================

    std::string plugin_name_;
    SecurityPolicy policy_;
    
    mutable std::shared_mutex permissions_mutex_;
    std::unordered_map<PluginCapabilities, PermissionRecord> granted_permissions_;
    
    mutable std::mutex violations_mutex_;
    std::vector<SecurityViolation> violations_;
    
    mutable std::mutex resource_mutex_;
    std::unordered_map<ResourceType, uint64_t> resource_usage_;
    std::unordered_map<ResourceType, std::chrono::system_clock::time_point> resource_reset_times_;
    uint64_t network_usage_this_second_ = 0;
    std::chrono::system_clock::time_point network_usage_reset_time_;
    
    // Execution tracking
    std::chrono::system_clock::time_point execution_start_time_;
    std::chrono::milliseconds total_execution_time_{0};
    bool execution_tracking_active_ = false;
    
    // Callbacks
    std::function<bool(const std::string&, PluginCapabilities, const std::string&)> permission_request_callback_;
    std::function<void(const SecurityViolation&)> violation_callback_;
    std::function<void(ResourceType, uint64_t, uint64_t)> resource_warning_callback_;
    
    // State
    std::atomic<bool> security_monitoring_enabled_{true};
    std::atomic<uint32_t> violation_count_{0};
    std::string sandbox_directory_;

    // =============================================================================
    // Internal Helper Methods
    // =============================================================================

    /// Check if path is within allowed paths
    bool isPathAllowed(const std::string& path, const std::vector<std::string>& allowed_paths) const;
    
    /// Check if path is explicitly blocked
    bool isPathBlocked(const std::string& path) const;
    
    /// Normalize path for comparison
    std::string normalizePath(const std::string& path) const;
    
    /// Reset resource usage counters if time window expired
    void resetResourceCountersIfNeeded(ResourceType type);
    
    /// Check violation threshold
    bool hasExceededViolationThreshold() const;
    
    /// Create sandbox directory structure
    bool createSandboxDirectory();
    
    /// Cleanup sandbox directory
    void cleanupSandboxDirectory();
    
    /// Validate policy against current capabilities
    bool validatePolicyConsistency() const;
};

// =============================================================================
// Security Context Factory
// =============================================================================

/// Factory for creating security contexts with predefined policies
class SecurityContextFactory {
public:
    /// Create security context with default policy for security level
    /// @param plugin_name Plugin name
    /// @param level Security level
    /// @return Unique pointer to security context
    static std::unique_ptr<SecurityContext> create(const std::string& plugin_name, SecurityLevel level);
    
    /// Create security context with custom policy
    /// @param plugin_name Plugin name
    /// @param policy Custom security policy
    /// @return Unique pointer to security context
    static std::unique_ptr<SecurityContext> create(const std::string& plugin_name, 
                                                   const SecurityContext::SecurityPolicy& policy);
    
    /// Create security context from policy file
    /// @param plugin_name Plugin name
    /// @param policy_file Path to policy file
    /// @return Unique pointer to security context
    static std::unique_ptr<SecurityContext> createFromFile(const std::string& plugin_name, 
                                                           const std::string& policy_file);

    /// Register custom security policy template
    /// @param name Template name
    /// @param policy Policy template
    static void registerPolicyTemplate(const std::string& name, const SecurityContext::SecurityPolicy& policy);
    
    /// Create security context from template
    /// @param plugin_name Plugin name
    /// @param template_name Template name
    /// @return Unique pointer to security context, or nullptr if template not found
    static std::unique_ptr<SecurityContext> createFromTemplate(const std::string& plugin_name, 
                                                               const std::string& template_name);

private:
    static std::unordered_map<std::string, SecurityContext::SecurityPolicy> policy_templates_;
    static std::mutex templates_mutex_;
};

} // namespace ecscope::plugins