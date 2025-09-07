#pragma once

/**
 * @file plugin/plugin_security.hpp
 * @brief ECScope Plugin Security - Comprehensive Security and Sandboxing System
 * 
 * Advanced security and sandboxing system for plugins providing memory isolation,
 * access control, resource limits, validation, and educational security demonstrations.
 * This system ensures plugins cannot compromise engine stability or security.
 * 
 * Security Features:
 * - Memory isolation and protection
 * - API access control and permission system
 * - Resource usage monitoring and limits
 * - Code signature verification
 * - Execution environment sandboxing
 * - Educational security vulnerability demonstrations
 * 
 * @author ECScope Educational ECS Framework - Plugin System
 * @date 2024
 */

#include "plugin_core.hpp"
#include "core/types.hpp"
#include "core/log.hpp"
#include "memory/arena.hpp"
#include "memory/memory_tracker.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <functional>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <optional>
#include <fstream>

// Platform-specific security includes
#ifdef _WIN32
    #include <windows.h>
    #include <psapi.h>
    #include <tlhelp32.h>
#elif __linux__
    #include <sys/mman.h>
    #include <sys/resource.h>
    #include <unistd.h>
    #include <signal.h>
#elif __APPLE__
    #include <mach/mach.h>
    #include <sys/mman.h>
    #include <sys/resource.h>
#endif

namespace ecscope::plugin {

//=============================================================================
// Security Policy and Configuration
//=============================================================================

/**
 * @brief Security policy level
 */
enum class SecurityPolicyLevel {
    Permissive,     // Allow most operations (development)
    Standard,       // Reasonable restrictions (default)
    Strict,         // High security restrictions
    Paranoid,       // Maximum security (isolated execution)
    Educational     // Designed for security education
};

/**
 * @brief Security violation severity
 */
enum class SecurityViolationSeverity {
    Info,           // Informational (educational)
    Warning,        // Potentially unsafe but allowed
    Error,          // Security violation, operation blocked
    Critical        // Serious security breach, plugin quarantined
};

/**
 * @brief Security violation record
 */
struct SecurityViolation {
    std::string plugin_name;
    std::chrono::system_clock::time_point timestamp;
    SecurityViolationSeverity severity;
    std::string violation_type;
    std::string description;
    std::string stack_trace;
    std::unordered_map<std::string, std::string> context;
    bool was_blocked{false};
    std::string educational_explanation;
    
    SecurityViolation(const std::string& plugin, SecurityViolationSeverity sev, 
                     const std::string& type, const std::string& desc)
        : plugin_name(plugin), timestamp(std::chrono::system_clock::now())
        , severity(sev), violation_type(type), description(desc) {}
};

/**
 * @brief Comprehensive security configuration
 */
struct SecurityConfig {
    SecurityPolicyLevel policy_level{SecurityPolicyLevel::Standard};
    
    // Memory Protection
    bool enable_memory_protection{true};
    bool enable_stack_protection{true};
    bool enable_heap_protection{true};
    bool enable_code_execution_protection{true};
    usize max_memory_per_plugin{128 * MB};
    usize stack_size_limit{8 * MB};
    
    // API Access Control
    bool enable_api_whitelisting{true};
    bool enable_system_call_filtering{true};
    bool enable_file_system_restrictions{true};
    bool enable_network_restrictions{true};
    std::unordered_set<std::string> allowed_api_functions;
    std::unordered_set<std::string> allowed_file_paths;
    std::unordered_set<std::string> allowed_network_hosts;
    
    // Execution Limits
    std::chrono::milliseconds max_execution_time_per_call{100};
    std::chrono::seconds max_total_execution_time{10};
    u32 max_threads_per_plugin{2};
    u32 max_file_handles_per_plugin{10};
    u32 max_network_connections_per_plugin{5};
    
    // Code Verification
    bool require_code_signing{false};
    bool verify_plugin_checksums{true};
    bool enable_runtime_code_validation{true};
    std::string trusted_certificate_path;
    std::unordered_set<std::string> trusted_publishers;
    
    // Educational Features
    bool enable_security_education{true};
    bool demonstrate_vulnerabilities{false};
    bool log_security_decisions{true};
    bool generate_security_reports{true};
    
    // Monitoring and Logging
    bool enable_behavior_monitoring{true};
    bool enable_performance_monitoring{true};
    bool enable_resource_tracking{true};
    u32 max_violation_records{1000};
    std::chrono::hours violation_record_retention{24 * 7}; // 1 week
    
    /**
     * @brief Factory methods for common configurations
     */
    static SecurityConfig create_development() {
        SecurityConfig config;
        config.policy_level = SecurityPolicyLevel::Permissive;
        config.enable_memory_protection = false;
        config.enable_api_whitelisting = false;
        config.require_code_signing = false;
        config.enable_security_education = true;
        config.demonstrate_vulnerabilities = true;
        return config;
    }
    
    static SecurityConfig create_production() {
        SecurityConfig config;
        config.policy_level = SecurityPolicyLevel::Strict;
        config.enable_memory_protection = true;
        config.enable_api_whitelisting = true;
        config.require_code_signing = true;
        config.enable_security_education = false;
        config.demonstrate_vulnerabilities = false;
        return config;
    }
    
    static SecurityConfig create_educational() {
        SecurityConfig config;
        config.policy_level = SecurityPolicyLevel::Educational;
        config.enable_security_education = true;
        config.demonstrate_vulnerabilities = true;
        config.log_security_decisions = true;
        config.generate_security_reports = true;
        return config;
    }
    
    static SecurityConfig create_paranoid() {
        SecurityConfig config;
        config.policy_level = SecurityPolicyLevel::Paranoid;
        config.enable_memory_protection = true;
        config.enable_stack_protection = true;
        config.enable_heap_protection = true;
        config.enable_code_execution_protection = true;
        config.enable_api_whitelisting = true;
        config.enable_system_call_filtering = true;
        config.enable_file_system_restrictions = true;
        config.enable_network_restrictions = true;
        config.require_code_signing = true;
        config.enable_runtime_code_validation = true;
        config.max_memory_per_plugin = 32 * MB;
        config.max_execution_time_per_call = std::chrono::milliseconds(50);
        config.max_threads_per_plugin = 1;
        return config;
    }
};

//=============================================================================
// Memory Protection and Isolation
//=============================================================================

/**
 * @brief Memory protection manager for plugins
 */
class MemoryProtectionManager {
private:
    SecurityConfig config_;
    std::unordered_map<std::string, std::unique_ptr<memory::ArenaAllocator>> plugin_arenas_;
    std::unordered_map<std::string, usize> plugin_memory_usage_;
    std::unordered_map<std::string, std::chrono::system_clock::time_point> last_memory_check_;
    mutable std::shared_mutex protection_mutex_;
    
    // Platform-specific memory protection state
    std::unordered_map<std::string, void*> protected_memory_regions_;

public:
    explicit MemoryProtectionManager(const SecurityConfig& config);
    ~MemoryProtectionManager();
    
    /**
     * @brief Create protected memory arena for plugin
     */
    bool create_plugin_arena(const std::string& plugin_name, usize size);
    
    /**
     * @brief Destroy plugin memory arena
     */
    void destroy_plugin_arena(const std::string& plugin_name);
    
    /**
     * @brief Get plugin memory allocator
     */
    memory::ArenaAllocator* get_plugin_allocator(const std::string& plugin_name);
    
    /**
     * @brief Check memory usage limits
     */
    bool check_memory_limits(const std::string& plugin_name);
    
    /**
     * @brief Get current memory usage
     */
    usize get_plugin_memory_usage(const std::string& plugin_name) const;
    
    /**
     * @brief Track memory allocation
     */
    void track_allocation(const std::string& plugin_name, usize size);
    
    /**
     * @brief Track memory deallocation
     */
    void track_deallocation(const std::string& plugin_name, usize size);
    
    /**
     * @brief Protect memory region from unauthorized access
     */
    bool protect_memory_region(void* address, usize size, bool read_only = true);
    
    /**
     * @brief Unprotect memory region
     */
    bool unprotect_memory_region(void* address, usize size);
    
    /**
     * @brief Check if address is in plugin's allocated memory
     */
    bool is_valid_plugin_memory(const std::string& plugin_name, void* address) const;
    
    /**
     * @brief Generate memory usage report
     */
    std::string generate_memory_report() const;

private:
    void setup_memory_protection();
    void cleanup_memory_protection();
    bool validate_memory_access(const std::string& plugin_name, void* address, usize size);
};

//=============================================================================
// API Access Control System
//=============================================================================

/**
 * @brief API access control manager
 */
class APIAccessController {
private:
    SecurityConfig config_;
    std::unordered_map<std::string, std::unordered_set<std::string>> plugin_api_permissions_;
    std::unordered_map<std::string, u64> api_call_counts_;
    std::unordered_map<std::string, std::chrono::system_clock::time_point> last_api_call_;
    mutable std::shared_mutex access_mutex_;
    
    // Educational tracking
    std::unordered_map<std::string, std::vector<std::string>> blocked_api_calls_;
    std::unordered_map<std::string, std::string> api_educational_descriptions_;

public:
    explicit APIAccessController(const SecurityConfig& config);
    
    /**
     * @brief Grant API permission to plugin
     */
    bool grant_api_permission(const std::string& plugin_name, const std::string& api_function);
    
    /**
     * @brief Revoke API permission from plugin
     */
    void revoke_api_permission(const std::string& plugin_name, const std::string& api_function);
    
    /**
     * @brief Check if plugin has API permission
     */
    bool has_api_permission(const std::string& plugin_name, const std::string& api_function) const;
    
    /**
     * @brief Validate API call
     */
    bool validate_api_call(const std::string& plugin_name, const std::string& api_function);
    
    /**
     * @brief Track API call
     */
    void track_api_call(const std::string& plugin_name, const std::string& api_function);
    
    /**
     * @brief Get API call statistics
     */
    std::unordered_map<std::string, u64> get_api_call_stats(const std::string& plugin_name) const;
    
    /**
     * @brief Set API educational description
     */
    void set_api_description(const std::string& api_function, const std::string& description);
    
    /**
     * @brief Get blocked API calls for educational purposes
     */
    std::vector<std::string> get_blocked_api_calls(const std::string& plugin_name) const;
    
    /**
     * @brief Generate API access report
     */
    std::string generate_api_access_report() const;

private:
    bool is_api_function_whitelisted(const std::string& api_function) const;
    void initialize_default_permissions();
};

//=============================================================================
// Execution Environment Sandbox
//=============================================================================

/**
 * @brief Plugin execution sandbox
 */
class PluginSandbox {
private:
    SecurityConfig config_;
    std::string plugin_name_;
    
    // Execution monitoring
    std::chrono::system_clock::time_point start_time_;
    std::chrono::milliseconds total_execution_time_{0};
    std::atomic<bool> is_executing_{false};
    std::atomic<u32> active_threads_{0};
    
    // Resource tracking
    std::atomic<u32> open_file_handles_{0};
    std::atomic<u32> network_connections_{0};
    std::unordered_set<std::thread::id> plugin_threads_;
    
    // Platform-specific sandbox data
    #ifdef _WIN32
        HANDLE job_object_{nullptr};
    #elif __linux__
        pid_t sandbox_process_{-1};
    #endif
    
    mutable std::mutex sandbox_mutex_;

public:
    explicit PluginSandbox(const std::string& plugin_name, const SecurityConfig& config);
    ~PluginSandbox();
    
    /**
     * @brief Initialize sandbox environment
     */
    bool initialize();
    
    /**
     * @brief Shutdown sandbox
     */
    void shutdown();
    
    /**
     * @brief Begin execution monitoring
     */
    bool begin_execution();
    
    /**
     * @brief End execution monitoring
     */
    void end_execution();
    
    /**
     * @brief Check execution time limits
     */
    bool check_execution_limits() const;
    
    /**
     * @brief Register thread as belonging to plugin
     */
    bool register_thread(std::thread::id thread_id);
    
    /**
     * @brief Unregister plugin thread
     */
    void unregister_thread(std::thread::id thread_id);
    
    /**
     * @brief Track file handle opening
     */
    bool track_file_handle_open();
    
    /**
     * @brief Track file handle closing
     */
    void track_file_handle_close();
    
    /**
     * @brief Track network connection opening
     */
    bool track_network_connection_open();
    
    /**
     * @brief Track network connection closing
     */
    void track_network_connection_close();
    
    /**
     * @brief Check resource limits
     */
    bool check_resource_limits() const;
    
    /**
     * @brief Force stop plugin execution
     */
    void force_stop();
    
    /**
     * @brief Get execution statistics
     */
    struct ExecutionStats {
        std::chrono::milliseconds total_execution_time;
        u32 active_threads;
        u32 open_file_handles;
        u32 network_connections;
        bool is_within_limits;
    };
    
    ExecutionStats get_execution_stats() const;

private:
    void setup_platform_sandbox();
    void cleanup_platform_sandbox();
    bool enforce_resource_limits();
};

//=============================================================================
// Code Signature and Verification
//=============================================================================

/**
 * @brief Code signature verifier
 */
class CodeSignatureVerifier {
private:
    SecurityConfig config_;
    std::unordered_map<std::string, std::string> plugin_signatures_;
    std::unordered_set<std::string> trusted_signatures_;
    
public:
    explicit CodeSignatureVerifier(const SecurityConfig& config);
    
    /**
     * @brief Verify plugin code signature
     */
    bool verify_plugin_signature(const std::string& plugin_path);
    
    /**
     * @brief Calculate plugin checksum
     */
    std::string calculate_plugin_checksum(const std::string& plugin_path);
    
    /**
     * @brief Add trusted signature
     */
    void add_trusted_signature(const std::string& signature);
    
    /**
     * @brief Remove trusted signature
     */
    void remove_trusted_signature(const std::string& signature);
    
    /**
     * @brief Verify plugin publisher
     */
    bool verify_plugin_publisher(const std::string& plugin_path, const std::string& expected_publisher);
    
    /**
     * @brief Get plugin signature info
     */
    struct SignatureInfo {
        std::string checksum;
        std::string publisher;
        std::chrono::system_clock::time_point signing_time;
        bool is_valid;
        bool is_trusted;
    };
    
    SignatureInfo get_plugin_signature_info(const std::string& plugin_path);

private:
    bool validate_certificate_chain(const std::string& plugin_path);
    std::string extract_publisher_info(const std::string& plugin_path);
};

//=============================================================================
// Main Security Manager
//=============================================================================

/**
 * @brief Comprehensive plugin security manager
 */
class PluginSecurityManager {
private:
    SecurityConfig config_;
    std::unique_ptr<MemoryProtectionManager> memory_manager_;
    std::unique_ptr<APIAccessController> api_controller_;
    std::unique_ptr<CodeSignatureVerifier> signature_verifier_;
    
    // Security violation tracking
    std::vector<SecurityViolation> violation_history_;
    std::unordered_map<std::string, std::unique_ptr<PluginSandbox>> plugin_sandboxes_;
    std::unordered_map<std::string, PluginSecurityContext> plugin_contexts_;
    
    // Educational security features
    std::unordered_map<std::string, std::vector<std::string>> security_lessons_;
    std::unordered_map<std::string, std::string> vulnerability_demonstrations_;
    
    mutable std::shared_mutex security_mutex_;
    std::atomic<bool> is_initialized_{false};

public:
    explicit PluginSecurityManager(const SecurityConfig& config);
    ~PluginSecurityManager();
    
    /**
     * @brief Initialize security manager
     */
    bool initialize();
    
    /**
     * @brief Shutdown security manager
     */
    void shutdown();
    
    /**
     * @brief Create security context for plugin
     */
    bool create_plugin_security_context(const std::string& plugin_name, 
                                       const PluginSecurityContext& context);
    
    /**
     * @brief Destroy plugin security context
     */
    void destroy_plugin_security_context(const std::string& plugin_name);
    
    /**
     * @brief Validate plugin before loading
     */
    bool validate_plugin_security(const std::string& plugin_name, const std::string& plugin_path);
    
    /**
     * @brief Check plugin permission
     */
    bool check_plugin_permission(const std::string& plugin_name, PluginPermission permission);
    
    /**
     * @brief Validate API call
     */
    bool validate_api_call(const std::string& plugin_name, const std::string& api_function);
    
    /**
     * @brief Track memory allocation
     */
    bool track_memory_allocation(const std::string& plugin_name, usize size);
    
    /**
     * @brief Track memory deallocation
     */
    void track_memory_deallocation(const std::string& plugin_name, usize size);
    
    /**
     * @brief Begin plugin execution
     */
    bool begin_plugin_execution(const std::string& plugin_name);
    
    /**
     * @brief End plugin execution
     */
    void end_plugin_execution(const std::string& plugin_name);
    
    /**
     * @brief Report security violation
     */
    void report_security_violation(const SecurityViolation& violation);
    
    /**
     * @brief Get plugin security context
     */
    std::optional<PluginSecurityContext> get_plugin_security_context(const std::string& plugin_name) const;
    
    /**
     * @brief Get security violation history
     */
    std::vector<SecurityViolation> get_violation_history(const std::string& plugin_name = "") const;
    
    /**
     * @brief Get security statistics
     */
    struct SecurityStats {
        u32 total_violations;
        u32 critical_violations;
        u32 plugins_quarantined;
        u32 api_calls_blocked;
        u32 memory_limit_violations;
        std::unordered_map<std::string, u32> violations_by_plugin;
        std::unordered_map<std::string, u32> violations_by_type;
    };
    
    SecurityStats get_security_statistics() const;
    
    /**
     * @brief Generate security report
     */
    std::string generate_security_report() const;
    
    /**
     * @brief Update security configuration
     */
    void update_configuration(const SecurityConfig& config);
    
    /**
     * @brief Get current configuration
     */
    const SecurityConfig& get_configuration() const { return config_; }
    
    //-------------------------------------------------------------------------
    // Educational Security Features
    //-------------------------------------------------------------------------
    
    /**
     * @brief Add security lesson for plugin
     */
    void add_security_lesson(const std::string& plugin_name, const std::string& lesson);
    
    /**
     * @brief Get security lessons for plugin
     */
    std::vector<std::string> get_security_lessons(const std::string& plugin_name) const;
    
    /**
     * @brief Demonstrate security vulnerability
     */
    void demonstrate_vulnerability(const std::string& vulnerability_type, 
                                 const std::string& description,
                                 bool actually_execute = false);
    
    /**
     * @brief Get vulnerability demonstrations
     */
    std::unordered_map<std::string, std::string> get_vulnerability_demonstrations() const;
    
    /**
     * @brief Generate security education report
     */
    std::string generate_security_education_report() const;

private:
    void initialize_security_lessons();
    void initialize_vulnerability_demonstrations();
    void cleanup_violation_history();
    bool should_quarantine_plugin(const std::string& plugin_name) const;
    void quarantine_plugin(const std::string& plugin_name, const std::string& reason);
};

//=============================================================================
// Security Utilities and Helpers
//=============================================================================

/**
 * @brief Calculate file hash for integrity checking
 */
std::string calculate_file_hash(const std::string& file_path);

/**
 * @brief Check if file has been tampered with
 */
bool verify_file_integrity(const std::string& file_path, const std::string& expected_hash);

/**
 * @brief Generate secure random string
 */
std::string generate_secure_random_string(usize length);

/**
 * @brief Encrypt plugin data
 */
std::vector<u8> encrypt_plugin_data(const std::vector<u8>& data, const std::string& key);

/**
 * @brief Decrypt plugin data
 */
std::vector<u8> decrypt_plugin_data(const std::vector<u8>& encrypted_data, const std::string& key);

/**
 * @brief Get system security information
 */
struct SystemSecurityInfo {
    bool has_address_space_randomization;
    bool has_data_execution_prevention;
    bool has_stack_protection;
    bool has_heap_protection;
    std::string security_features;
};

SystemSecurityInfo get_system_security_info();

} // namespace ecscope::plugin