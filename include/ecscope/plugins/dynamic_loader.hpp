#pragma once

#include "plugin_types.hpp"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <atomic>

namespace ecscope::plugins {

// Forward declarations
class PluginBase;

// =============================================================================
// Platform-specific Dynamic Loading Interface
// =============================================================================

/// Cross-platform dynamic library loading and symbol resolution system
class DynamicLoader {
public:
    // =============================================================================
    // Library Handle
    // =============================================================================

    /// Opaque handle to loaded dynamic library
    using LibraryHandle = void*;

    /// Invalid library handle constant
    static constexpr LibraryHandle INVALID_HANDLE = nullptr;

    // =============================================================================
    // Symbol Information
    // =============================================================================

    struct SymbolInfo {
        std::string name;           ///< Symbol name
        void* address = nullptr;    ///< Symbol address in memory
        size_t size = 0;           ///< Symbol size (if available)
        std::string type;          ///< Symbol type (function, variable, etc.)
        bool is_exported = false;  ///< Whether symbol is exported
    };

    // =============================================================================
    // Library Information
    // =============================================================================

    struct LibraryInfo {
        std::string path;                           ///< Library file path
        std::string name;                          ///< Library name
        Version version;                           ///< Library version
        std::vector<std::string> dependencies;     ///< Required dependencies
        std::vector<SymbolInfo> exported_symbols;  ///< Exported symbols
        uint64_t file_size = 0;                   ///< File size in bytes
        std::string checksum;                      ///< File checksum
        std::chrono::system_clock::time_point load_time; ///< When library was loaded
        bool is_loaded = false;                    ///< Whether library is currently loaded
    };

    // =============================================================================
    // Loading Configuration
    // =============================================================================

    struct LoadingConfig {
        bool lazy_binding = true;           ///< Enable lazy symbol binding
        bool global_symbols = false;       ///< Make symbols globally available
        bool resolve_all_symbols = false;  ///< Resolve all symbols immediately
        bool allow_unresolved = false;     ///< Allow unresolved symbols
        bool deep_binding = false;          ///< Use deep binding for symbol resolution
        
        // Security options
        bool verify_signature = false;     ///< Verify digital signature
        bool check_integrity = true;       ///< Verify file integrity
        bool enforce_aslr = true;          ///< Enforce Address Space Layout Randomization
        bool nx_bit_support = true;        ///< Require NX bit support
        
        // Platform-specific options
        std::unordered_map<std::string, std::string> platform_options;
    };

    // =============================================================================
    // Error Handling
    // =============================================================================

    enum class LoadError : uint32_t {
        SUCCESS = 0,
        FILE_NOT_FOUND,
        ACCESS_DENIED,
        INVALID_FORMAT,
        ARCHITECTURE_MISMATCH,
        DEPENDENCY_NOT_FOUND,
        SYMBOL_NOT_FOUND,
        VERSION_MISMATCH,
        SIGNATURE_INVALID,
        CHECKSUM_MISMATCH,
        MEMORY_ERROR,
        PLATFORM_ERROR,
        SECURITY_VIOLATION,
        ALREADY_LOADED,
        NOT_LOADED,
        INITIALIZATION_FAILED
    };

    struct LoadErrorInfo {
        LoadError error_code;
        std::string error_message;
        std::string system_error;
        int32_t platform_error_code = 0;
        std::vector<std::string> details;
    };

    // =============================================================================
    // Constructor and Lifecycle
    // =============================================================================

    explicit DynamicLoader(const LoadingConfig& config = LoadingConfig{});
    ~DynamicLoader();

    // Non-copyable, moveable
    DynamicLoader(const DynamicLoader&) = delete;
    DynamicLoader& operator=(const DynamicLoader&) = delete;
    DynamicLoader(DynamicLoader&&) noexcept;
    DynamicLoader& operator=(DynamicLoader&&) noexcept;

    // =============================================================================
    // Library Loading and Unloading
    // =============================================================================

    /// Load dynamic library from file
    /// @param library_path Path to library file
    /// @param[out] error_info Error information if loading fails
    /// @return Library handle, or INVALID_HANDLE on failure
    LibraryHandle loadLibrary(const std::string& library_path, LoadErrorInfo* error_info = nullptr);

    /// Unload dynamic library
    /// @param handle Library handle to unload
    /// @param[out] error_info Error information if unloading fails
    /// @return true if library unloaded successfully
    bool unloadLibrary(LibraryHandle handle, LoadErrorInfo* error_info = nullptr);

    /// Check if library is loaded
    /// @param handle Library handle to check
    /// @return true if library is loaded
    bool isLibraryLoaded(LibraryHandle handle) const;

    /// Get library information
    /// @param handle Library handle
    /// @return Library information, or empty struct if handle invalid
    LibraryInfo getLibraryInfo(LibraryHandle handle) const;

    /// Reload library (unload and load again)
    /// @param handle Current library handle
    /// @param[out] error_info Error information if reloading fails
    /// @return New library handle, or INVALID_HANDLE on failure
    LibraryHandle reloadLibrary(LibraryHandle handle, LoadErrorInfo* error_info = nullptr);

    // =============================================================================
    // Symbol Resolution
    // =============================================================================

    /// Get symbol address from library
    /// @param handle Library handle
    /// @param symbol_name Symbol name to resolve
    /// @param[out] error_info Error information if resolution fails
    /// @return Symbol address, or nullptr if not found
    void* getSymbol(LibraryHandle handle, const std::string& symbol_name, 
                   LoadErrorInfo* error_info = nullptr);

    /// Get typed symbol from library
    /// @tparam T Symbol type
    /// @param handle Library handle
    /// @param symbol_name Symbol name to resolve
    /// @param[out] error_info Error information if resolution fails
    /// @return Typed symbol pointer, or nullptr if not found
    template<typename T>
    T* getTypedSymbol(LibraryHandle handle, const std::string& symbol_name,
                     LoadErrorInfo* error_info = nullptr) {
        void* symbol = getSymbol(handle, symbol_name, error_info);
        return static_cast<T*>(symbol);
    }

    /// Check if symbol exists in library
    /// @param handle Library handle
    /// @param symbol_name Symbol name to check
    /// @return true if symbol exists
    bool hasSymbol(LibraryHandle handle, const std::string& symbol_name);

    /// Get all exported symbols from library
    /// @param handle Library handle
    /// @return List of exported symbols
    std::vector<SymbolInfo> getExportedSymbols(LibraryHandle handle) const;

    /// Resolve multiple symbols at once
    /// @param handle Library handle
    /// @param symbol_names List of symbol names to resolve
    /// @return Map of symbol names to addresses
    std::unordered_map<std::string, void*> resolveSymbols(LibraryHandle handle,
                                                          const std::vector<std::string>& symbol_names);

    // =============================================================================
    // Plugin-Specific Loading
    // =============================================================================

    /// Load plugin library and validate plugin interface
    /// @param library_path Path to plugin library
    /// @param[out] error_info Error information if loading fails
    /// @return Library handle, or INVALID_HANDLE on failure
    LibraryHandle loadPlugin(const std::string& library_path, LoadErrorInfo* error_info = nullptr);

    /// Create plugin instance from loaded library
    /// @param handle Library handle
    /// @param[out] error_info Error information if creation fails
    /// @return Plugin instance, or nullptr on failure
    std::unique_ptr<PluginBase> createPluginInstance(LibraryHandle handle, LoadErrorInfo* error_info = nullptr);

    /// Destroy plugin instance
    /// @param handle Library handle
    /// @param plugin Plugin instance to destroy
    /// @param[out] error_info Error information if destruction fails
    /// @return true if plugin destroyed successfully
    bool destroyPluginInstance(LibraryHandle handle, std::unique_ptr<PluginBase> plugin, 
                              LoadErrorInfo* error_info = nullptr);

    /// Get plugin manifest from library
    /// @param handle Library handle
    /// @param[out] error_info Error information if retrieval fails
    /// @return Plugin manifest, or nullptr on failure
    std::unique_ptr<PluginManifest> getPluginManifest(LibraryHandle handle, LoadErrorInfo* error_info = nullptr);

    /// Validate plugin API version
    /// @param handle Library handle
    /// @param expected_version Expected API version
    /// @param[out] error_info Error information if validation fails
    /// @return true if API version is compatible
    bool validatePluginAPI(LibraryHandle handle, uint32_t expected_version, LoadErrorInfo* error_info = nullptr);

    // =============================================================================
    // Dependency Management
    // =============================================================================

    /// Get library dependencies
    /// @param library_path Path to library file
    /// @return List of dependency names
    std::vector<std::string> getLibraryDependencies(const std::string& library_path) const;

    /// Check if all dependencies are available
    /// @param library_path Path to library file
    /// @param[out] missing_dependencies List of missing dependencies
    /// @return true if all dependencies are available
    bool checkDependencies(const std::string& library_path, 
                          std::vector<std::string>* missing_dependencies = nullptr) const;

    /// Preload dependencies
    /// @param dependencies List of dependency paths
    /// @param[out] failed_dependencies List of dependencies that failed to load
    /// @return Number of successfully loaded dependencies
    uint32_t preloadDependencies(const std::vector<std::string>& dependencies,
                                std::vector<std::string>* failed_dependencies = nullptr);

    // =============================================================================
    // File Validation and Security
    // =============================================================================

    /// Verify file integrity using checksum
    /// @param file_path Path to file
    /// @param expected_checksum Expected checksum
    /// @return true if checksum matches
    bool verifyFileIntegrity(const std::string& file_path, const std::string& expected_checksum) const;

    /// Verify digital signature (if supported on platform)
    /// @param file_path Path to file
    /// @param[out] error_info Error information if verification fails
    /// @return true if signature is valid
    bool verifyDigitalSignature(const std::string& file_path, LoadErrorInfo* error_info = nullptr) const;

    /// Check if file format is compatible with current platform
    /// @param file_path Path to file
    /// @return true if format is compatible
    bool isFileFormatCompatible(const std::string& file_path) const;

    /// Get file architecture information
    /// @param file_path Path to file
    /// @return Architecture string (e.g., "x64", "x86", "arm64")
    std::string getFileArchitecture(const std::string& file_path) const;

    /// Calculate file checksum
    /// @param file_path Path to file
    /// @param algorithm Checksum algorithm (e.g., "SHA256", "MD5")
    /// @return Checksum string, or empty string on error
    std::string calculateChecksum(const std::string& file_path, const std::string& algorithm = "SHA256") const;

    // =============================================================================
    // Platform Information and Capabilities
    // =============================================================================

    /// Get current platform identifier
    /// @return Platform string (e.g., "windows", "linux", "macos")
    static std::string getCurrentPlatform();

    /// Get current architecture
    /// @return Architecture string (e.g., "x64", "x86", "arm64")
    static std::string getCurrentArchitecture();

    /// Get supported library extensions for current platform
    /// @return List of extensions (e.g., {".dll", ".so", ".dylib"})
    static std::vector<std::string> getSupportedExtensions();

    /// Check if hot-reloading is supported on current platform
    /// @return true if hot-reloading is supported
    static bool isHotReloadSupported();

    /// Check if code signing is supported on current platform
    /// @return true if code signing is supported
    static bool isCodeSigningSupported();

    // =============================================================================
    // Hot-Reloading Support
    // =============================================================================

    /// Enable hot-reloading for library
    /// @param handle Library handle
    /// @return true if hot-reloading enabled successfully
    bool enableHotReload(LibraryHandle handle);

    /// Disable hot-reloading for library
    /// @param handle Library handle
    /// @return true if hot-reloading disabled successfully
    bool disableHotReload(LibraryHandle handle);

    /// Check if library file has been modified
    /// @param handle Library handle
    /// @return true if file has been modified since last load
    bool hasFileChanged(LibraryHandle handle) const;

    /// Set file change callback
    /// @param callback Function to call when monitored files change
    void setFileChangeCallback(std::function<void(LibraryHandle, const std::string&)> callback);

    // =============================================================================
    // Error Handling and Diagnostics
    // =============================================================================

    /// Get last error information
    /// @return Last error information
    LoadErrorInfo getLastError() const;

    /// Clear error state
    void clearError();

    /// Convert error code to string
    /// @param error Error code
    /// @return Human-readable error description
    static std::string errorToString(LoadError error);

    /// Get detailed system error message
    /// @return System-specific error message
    std::string getSystemErrorMessage() const;

    // =============================================================================
    // Statistics and Monitoring
    // =============================================================================

    /// Get loader statistics
    /// @return Map of statistics
    std::unordered_map<std::string, uint64_t> getStatistics() const;

    /// Reset statistics
    void resetStatistics();

    /// Get list of all loaded libraries
    /// @return List of library handles
    std::vector<LibraryHandle> getLoadedLibraries() const;

    /// Get memory usage of loaded libraries
    /// @return Total memory usage in bytes
    uint64_t getMemoryUsage() const;

    // =============================================================================
    // Advanced Features
    // =============================================================================

    /// Set custom symbol resolver
    /// @param resolver Function to resolve symbols
    void setSymbolResolver(std::function<void*(const std::string&, const std::string&)> resolver);

    /// Set library search paths
    /// @param paths List of search paths
    void setSearchPaths(const std::vector<std::string>& paths);

    /// Add library search path
    /// @param path Search path to add
    void addSearchPath(const std::string& path);

    /// Remove library search path
    /// @param path Search path to remove
    void removeSearchPath(const std::string& path);

    /// Get current search paths
    /// @return List of search paths
    std::vector<std::string> getSearchPaths() const;

    /// Find library file in search paths
    /// @param library_name Library name (without extension)
    /// @return Full path to library, or empty string if not found
    std::string findLibrary(const std::string& library_name) const;

private:
    // =============================================================================
    // Platform-specific Implementation
    // =============================================================================

    struct PlatformImpl;
    std::unique_ptr<PlatformImpl> pimpl_;

    // =============================================================================
    // Member Variables
    // =============================================================================

    LoadingConfig config_;
    mutable std::shared_mutex libraries_mutex_;
    std::unordered_map<LibraryHandle, LibraryInfo> loaded_libraries_;
    std::vector<std::string> search_paths_;
    
    // Error handling
    mutable std::mutex error_mutex_;
    LoadErrorInfo last_error_;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    uint64_t libraries_loaded_count_ = 0;
    uint64_t libraries_failed_count_ = 0;
    uint64_t symbols_resolved_count_ = 0;
    uint64_t hot_reloads_count_ = 0;
    std::chrono::system_clock::time_point start_time_;
    
    // Callbacks
    std::function<void*(const std::string&, const std::string&)> symbol_resolver_;
    std::function<void(LibraryHandle, const std::string&)> file_change_callback_;
    
    // Hot-reload monitoring
    std::unordered_set<LibraryHandle> hot_reload_libraries_;

    // =============================================================================
    // Internal Helper Methods
    // =============================================================================

    /// Set last error
    void setLastError(LoadError error, const std::string& message, 
                     const std::string& system_error = "", int32_t platform_code = 0);
    
    /// Validate library handle
    bool isValidHandle(LibraryHandle handle) const;
    
    /// Update library statistics
    void updateStatistics(const std::string& operation, bool success);
    
    /// Cleanup library resources
    void cleanupLibrary(LibraryHandle handle);
    
    /// Platform-specific library loading
    LibraryHandle platformLoadLibrary(const std::string& path, const LoadingConfig& config, LoadErrorInfo* error);
    
    /// Platform-specific library unloading
    bool platformUnloadLibrary(LibraryHandle handle, LoadErrorInfo* error);
    
    /// Platform-specific symbol resolution
    void* platformGetSymbol(LibraryHandle handle, const std::string& name, LoadErrorInfo* error);
    
    /// Platform-specific dependency analysis
    std::vector<std::string> platformGetDependencies(const std::string& path) const;
    
    /// Platform-specific file validation
    bool platformValidateFile(const std::string& path) const;
};

// =============================================================================
// Dynamic Loader Factory
// =============================================================================

/// Factory for creating dynamic loaders with different configurations
class DynamicLoaderFactory {
public:
    /// Create default dynamic loader
    /// @return Unique pointer to dynamic loader
    static std::unique_ptr<DynamicLoader> createDefault();
    
    /// Create secure dynamic loader with strict validation
    /// @return Unique pointer to dynamic loader
    static std::unique_ptr<DynamicLoader> createSecure();
    
    /// Create performance-optimized dynamic loader
    /// @return Unique pointer to dynamic loader
    static std::unique_ptr<DynamicLoader> createPerformance();
    
    /// Create dynamic loader with custom configuration
    /// @param config Custom loading configuration
    /// @return Unique pointer to dynamic loader
    static std::unique_ptr<DynamicLoader> createCustom(const DynamicLoader::LoadingConfig& config);
};

} // namespace ecscope::plugins