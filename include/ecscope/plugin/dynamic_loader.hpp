#pragma once

/**
 * @file plugin/dynamic_loader.hpp
 * @brief ECScope Dynamic Library Loader - Cross-Platform Dynamic Module System
 * 
 * Comprehensive dynamic library loading system with cross-platform compatibility,
 * hot-swapping support, symbol resolution, and educational features. Supports
 * Windows DLL, Linux SO, and macOS dylib with unified interface.
 * 
 * Key Features:
 * - Cross-platform dynamic library loading (Windows/Linux/macOS)
 * - Hot-swappable modules with state preservation
 * - Symbol resolution and function pointer management  
 * - Dependency tracking and resolution
 * - Educational demonstrations of dynamic linking
 * - Runtime compilation and loading support
 * - Performance monitoring and optimization
 * 
 * @author ECScope Educational ECS Framework - Plugin System
 * @date 2024
 */

#include "plugin_core.hpp"
#include "core/types.hpp"
#include "core/log.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <functional>
#include <chrono>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <optional>
#include <filesystem>

// Platform-specific includes
#ifdef _WIN32
    #include <windows.h>
    #include <libloaderapi.h>
    #include <processthreadsapi.h>
#elif __linux__
    #include <dlfcn.h>
    #include <link.h>
    #include <unistd.h>
#elif __APPLE__
    #include <dlfcn.h>
    #include <mach-o/dyld.h>
    #include <mach-o/loader.h>
#endif

namespace ecscope::plugin {

//=============================================================================
// Dynamic Library Information and Metadata
//=============================================================================

/**
 * @brief Library loading mode
 */
enum class LoadingMode {
    Lazy,           // Load symbols on first use
    Immediate,      // Load all symbols immediately
    Local,          // Symbols not available to other libraries
    Global,         // Symbols available to other libraries
    DeepBind,       // Prefer symbols from this library (Linux only)
    NodeDelete      // Don't unload on dlclose (Linux only)
};

/**
 * @brief Symbol visibility
 */
enum class SymbolVisibility {
    Default,        // Default visibility
    Hidden,         // Hidden from other modules
    Protected,      // Protected visibility
    Internal        // Internal visibility
};

/**
 * @brief Dynamic library information
 */
struct LibraryInfo {
    std::string file_path;
    std::string name;
    LibraryHandle handle;
    LoadingMode loading_mode{LoadingMode::Immediate};
    std::chrono::system_clock::time_point load_time;
    std::chrono::system_clock::time_point last_modified;
    usize file_size{0};
    std::string file_hash;
    
    // Platform-specific information
    #ifdef _WIN32
        HMODULE windows_handle{nullptr};
        std::string module_filename;
    #else
        void* unix_handle{nullptr};
        Dl_info dl_info{};
    #endif
    
    // Symbol information
    std::unordered_map<std::string, void*> resolved_symbols;
    std::vector<std::string> exported_symbols;
    std::vector<std::string> imported_symbols;
    
    // Dependencies
    std::vector<std::string> dependencies;
    std::unordered_set<std::string> dependents;
    
    // Statistics
    u32 symbol_resolution_count{0};
    f64 total_symbol_resolution_time_ms{0.0};
    std::atomic<u32> reference_count{0};
    
    LibraryInfo() = default;
    LibraryInfo(const std::string& path, LibraryHandle h)
        : file_path(path), handle(h), load_time(std::chrono::system_clock::now()) {
        name = std::filesystem::path(path).stem().string();
        update_file_info();
    }

private:
    void update_file_info();
};

/**
 * @brief Symbol resolution result
 */
struct SymbolInfo {
    std::string name;
    void* address{nullptr};
    SymbolVisibility visibility{SymbolVisibility::Default};
    bool is_function{false};
    bool is_data{false};
    usize size{0};
    std::string demangled_name;
    std::string library_name;
    
    SymbolInfo() = default;
    SymbolInfo(const std::string& n, void* addr) : name(n), address(addr) {}
    
    bool is_valid() const { return address != nullptr; }
};

//=============================================================================
// Hot Reload Support
//=============================================================================

/**
 * @brief Hot reload configuration
 */
struct HotReloadConfig {
    bool enable_file_watching{true};
    bool enable_automatic_reload{false};
    bool preserve_state_on_reload{true};
    bool validate_symbols_after_reload{true};
    std::chrono::milliseconds file_check_interval{1000};
    std::chrono::milliseconds reload_debounce_time{500};
    std::string state_backup_directory{"./hot_reload_backups"};
    
    // Educational features
    bool log_reload_process{true};
    bool demonstrate_hot_reload_challenges{false};
    bool track_reload_performance{true};
};

/**
 * @brief Hot reload state
 */
struct HotReloadState {
    bool is_watching{false};
    std::chrono::system_clock::time_point last_check_time;
    std::filesystem::file_time_type last_modification_time;
    std::string state_backup_file;
    u32 reload_count{0};
    f64 total_reload_time_ms{0.0};
    std::vector<std::string> failed_reload_attempts;
};

//=============================================================================
// Runtime Compilation Support
//=============================================================================

/**
 * @brief Runtime compilation configuration
 */
struct CompilationConfig {
    std::string compiler_path;
    std::vector<std::string> include_directories;
    std::vector<std::string> library_directories;
    std::vector<std::string> linked_libraries;
    std::vector<std::string> compiler_flags;
    std::string output_directory{"./compiled_plugins"};
    bool enable_optimization{true};
    bool enable_debug_info{true};
    bool enable_warnings{true};
    
    // Educational features
    bool show_compilation_process{true};
    bool explain_compiler_flags{false};
    bool demonstrate_linking_process{false};
};

/**
 * @brief Compilation result
 */
struct CompilationResult {
    bool success{false};
    std::string output_file;
    std::string compiler_output;
    std::string error_output;
    f64 compilation_time_ms{0.0};
    std::vector<std::string> generated_files;
    
    explicit operator bool() const { return success; }
};

//=============================================================================
// Dynamic Loader Class
//=============================================================================

/**
 * @brief Cross-platform dynamic library loader with hot reload support
 * 
 * The DynamicLoader provides a unified interface for loading, managing, and
 * hot-reloading dynamic libraries across Windows, Linux, and macOS platforms.
 * It handles symbol resolution, dependency management, and provides educational
 * insights into dynamic linking processes.
 */
class DynamicLoader {
private:
    // Library management
    std::unordered_map<std::string, std::unique_ptr<LibraryInfo>> loaded_libraries_;
    std::unordered_map<LibraryHandle, std::string> handle_to_path_;
    std::unordered_map<std::string, std::string> path_to_canonical_;
    
    // Symbol cache
    std::unordered_map<std::string, SymbolInfo> symbol_cache_;
    std::unordered_map<std::string, std::chrono::system_clock::time_point> symbol_access_times_;
    
    // Hot reload support
    HotReloadConfig hot_reload_config_;
    std::unordered_map<std::string, HotReloadState> hot_reload_states_;
    std::thread file_watcher_thread_;
    std::atomic<bool> should_stop_watching_{false};
    
    // Runtime compilation
    CompilationConfig compilation_config_;
    std::unordered_map<std::string, std::string> source_to_binary_map_;
    
    // Threading and synchronization
    mutable std::shared_mutex loader_mutex_;
    mutable std::mutex symbol_cache_mutex_;
    mutable std::mutex hot_reload_mutex_;
    
    // Educational and monitoring
    std::atomic<u32> total_libraries_loaded_{0};
    std::atomic<u32> total_symbols_resolved_{0};
    std::atomic<u32> hot_reloads_performed_{0};
    std::chrono::high_resolution_clock::time_point creation_time_;
    
    // Platform-specific state
    #ifdef _WIN32
        HMODULE kernel32_handle_{nullptr};
    #endif

public:
    /**
     * @brief Construct dynamic loader with configuration
     */
    explicit DynamicLoader(const HotReloadConfig& hot_reload_config = {},
                          const CompilationConfig& compilation_config = {});
    
    /**
     * @brief Destructor with cleanup
     */
    ~DynamicLoader();
    
    // Non-copyable but movable
    DynamicLoader(const DynamicLoader&) = delete;
    DynamicLoader& operator=(const DynamicLoader&) = delete;
    DynamicLoader(DynamicLoader&&) = default;
    DynamicLoader& operator=(DynamicLoader&&) = default;
    
    //-------------------------------------------------------------------------
    // Library Loading and Management
    //-------------------------------------------------------------------------
    
    /**
     * @brief Load dynamic library
     */
    std::optional<LibraryHandle> load_library(const std::string& file_path,
                                             LoadingMode mode = LoadingMode::Immediate);
    
    /**
     * @brief Unload dynamic library
     */
    bool unload_library(LibraryHandle handle);
    
    /**
     * @brief Unload library by path
     */
    bool unload_library(const std::string& file_path);
    
    /**
     * @brief Check if library is loaded
     */
    bool is_library_loaded(const std::string& file_path) const;
    
    /**
     * @brief Get library handle by path
     */
    std::optional<LibraryHandle> get_library_handle(const std::string& file_path) const;
    
    /**
     * @brief Get library information
     */
    const LibraryInfo* get_library_info(LibraryHandle handle) const;
    
    /**
     * @brief Get library information by path
     */
    const LibraryInfo* get_library_info(const std::string& file_path) const;
    
    /**
     * @brief Get all loaded libraries
     */
    std::vector<std::string> get_loaded_libraries() const;
    
    /**
     * @brief Reload library
     */
    bool reload_library(LibraryHandle handle);
    
    /**
     * @brief Reload library by path
     */
    bool reload_library(const std::string& file_path);
    
    //-------------------------------------------------------------------------
    // Symbol Resolution
    //-------------------------------------------------------------------------
    
    /**
     * @brief Resolve symbol in specific library
     */
    SymbolInfo resolve_symbol(LibraryHandle handle, const std::string& symbol_name);
    
    /**
     * @brief Resolve symbol in any loaded library
     */
    SymbolInfo resolve_symbol(const std::string& symbol_name);
    
    /**
     * @brief Get function pointer
     */
    template<typename FunctionType>
    FunctionType get_function(LibraryHandle handle, const std::string& function_name);
    
    /**
     * @brief Get function pointer from any library
     */
    template<typename FunctionType>
    FunctionType get_function(const std::string& function_name);
    
    /**
     * @brief Get data pointer
     */
    template<typename DataType>
    DataType* get_data(LibraryHandle handle, const std::string& data_name);
    
    /**
     * @brief Get data pointer from any library
     */
    template<typename DataType>
    DataType* get_data(const std::string& data_name);
    
    /**
     * @brief Check if symbol exists
     */
    bool has_symbol(LibraryHandle handle, const std::string& symbol_name);
    
    /**
     * @brief Check if symbol exists in any library
     */
    bool has_symbol(const std::string& symbol_name);
    
    /**
     * @brief Get all symbols in library
     */
    std::vector<std::string> get_library_symbols(LibraryHandle handle);
    
    /**
     * @brief Clear symbol cache
     */
    void clear_symbol_cache();
    
    //-------------------------------------------------------------------------
    // Dependency Management
    //-------------------------------------------------------------------------
    
    /**
     * @brief Analyze library dependencies
     */
    std::vector<std::string> analyze_dependencies(const std::string& file_path);
    
    /**
     * @brief Load library with dependencies
     */
    bool load_library_with_dependencies(const std::string& file_path, 
                                       LoadingMode mode = LoadingMode::Immediate);
    
    /**
     * @brief Get dependency graph
     */
    std::unordered_map<std::string, std::vector<std::string>> get_dependency_graph() const;
    
    /**
     * @brief Check for circular dependencies
     */
    bool has_circular_dependencies() const;
    
    /**
     * @brief Get load order for dependencies
     */
    std::vector<std::string> get_dependency_load_order(const std::string& file_path) const;
    
    //-------------------------------------------------------------------------
    // Hot Reload Support
    //-------------------------------------------------------------------------
    
    /**
     * @brief Enable hot reload for library
     */
    bool enable_hot_reload(const std::string& file_path);
    
    /**
     * @brief Disable hot reload for library
     */
    void disable_hot_reload(const std::string& file_path);
    
    /**
     * @brief Check for file changes
     */
    std::vector<std::string> check_for_changes();
    
    /**
     * @brief Perform hot reload
     */
    bool perform_hot_reload(const std::string& file_path);
    
    /**
     * @brief Get hot reload statistics
     */
    struct HotReloadStats {
        u32 total_reloads;
        u32 successful_reloads;
        u32 failed_reloads;
        f64 average_reload_time_ms;
        std::unordered_map<std::string, u32> reloads_by_library;
    };
    
    HotReloadStats get_hot_reload_stats() const;
    
    /**
     * @brief Set hot reload configuration
     */
    void set_hot_reload_config(const HotReloadConfig& config);
    
    /**
     * @brief Get hot reload configuration
     */
    const HotReloadConfig& get_hot_reload_config() const { return hot_reload_config_; }
    
    //-------------------------------------------------------------------------
    // Runtime Compilation
    //-------------------------------------------------------------------------
    
    /**
     * @brief Compile source code to library
     */
    CompilationResult compile_to_library(const std::string& source_file,
                                        const std::string& output_name = "");
    
    /**
     * @brief Compile and load library
     */
    std::optional<LibraryHandle> compile_and_load(const std::string& source_file,
                                                 const std::string& output_name = "");
    
    /**
     * @brief Set compilation configuration
     */
    void set_compilation_config(const CompilationConfig& config);
    
    /**
     * @brief Get compilation configuration
     */
    const CompilationConfig& get_compilation_config() const { return compilation_config_; }
    
    /**
     * @brief Get compiler version
     */
    std::string get_compiler_version() const;
    
    /**
     * @brief Check if compiler is available
     */
    bool is_compiler_available() const;
    
    //-------------------------------------------------------------------------
    // Educational Features
    //-------------------------------------------------------------------------
    
    /**
     * @brief Explain dynamic linking process
     */
    std::string explain_dynamic_linking(const std::string& file_path) const;
    
    /**
     * @brief Demonstrate symbol resolution
     */
    void demonstrate_symbol_resolution(const std::string& symbol_name) const;
    
    /**
     * @brief Show library dependencies visually
     */
    std::string visualize_dependencies(const std::string& file_path) const;
    
    /**
     * @brief Generate learning report
     */
    std::string generate_learning_report() const;
    
    /**
     * @brief Get platform-specific information
     */
    struct PlatformInfo {
        std::string platform_name;
        std::string library_extension;
        std::string library_prefix;
        std::string path_separator;
        std::vector<std::string> library_search_paths;
        std::vector<std::string> supported_loading_modes;
    };
    
    PlatformInfo get_platform_info() const;
    
    //-------------------------------------------------------------------------
    // Performance and Statistics
    //-------------------------------------------------------------------------
    
    /**
     * @brief Get loader statistics
     */
    struct LoaderStats {
        u32 total_libraries_loaded;
        u32 total_symbols_resolved;
        u32 hot_reloads_performed;
        f64 average_load_time_ms;
        f64 average_symbol_resolution_time_ms;
        usize total_memory_usage;
        std::unordered_map<std::string, u32> symbols_by_library;
    };
    
    LoaderStats get_statistics() const;
    
    /**
     * @brief Generate performance report
     */
    std::string generate_performance_report() const;
    
    /**
     * @brief Get most accessed symbols
     */
    std::vector<std::pair<std::string, u32>> get_most_accessed_symbols(u32 count = 10) const;
    
    /**
     * @brief Clear all cached data
     */
    void clear_caches();

private:
    //-------------------------------------------------------------------------
    // Platform-Specific Implementation
    //-------------------------------------------------------------------------
    
    /**
     * @brief Platform-specific library loading
     */
    LibraryHandle platform_load_library(const std::string& file_path, LoadingMode mode);
    
    /**
     * @brief Platform-specific library unloading
     */
    bool platform_unload_library(LibraryHandle handle);
    
    /**
     * @brief Platform-specific symbol resolution
     */
    void* platform_resolve_symbol(LibraryHandle handle, const std::string& symbol_name);
    
    /**
     * @brief Platform-specific error handling
     */
    std::string get_platform_error() const;
    
    /**
     * @brief Get canonical file path
     */
    std::string get_canonical_path(const std::string& file_path) const;
    
    /**
     * @brief File watcher thread worker
     */
    void file_watcher_worker();
    
    /**
     * @brief Update library file information
     */
    void update_library_file_info(LibraryInfo& info);
    
    /**
     * @brief Validate library file
     */
    bool validate_library_file(const std::string& file_path) const;
    
    /**
     * @brief Extract library symbols
     */
    std::vector<std::string> extract_library_symbols(LibraryHandle handle);
    
    /**
     * @brief Perform dependency resolution
     */
    bool resolve_dependencies(LibraryInfo& library_info);
    
    /**
     * @brief Initialize platform-specific components
     */
    void initialize_platform_components();
    
    /**
     * @brief Cleanup platform-specific resources
     */
    void cleanup_platform_resources();
    
    /**
     * @brief Update symbol statistics
     */
    void update_symbol_statistics(const std::string& symbol_name, f64 resolution_time_ms);
    
    /**
     * @brief Build compiler command
     */
    std::string build_compiler_command(const std::string& source_file,
                                      const std::string& output_file) const;
    
    /**
     * @brief Execute compilation command
     */
    CompilationResult execute_compilation(const std::string& command) const;
    
    /**
     * @brief Validate compiled library
     */
    bool validate_compiled_library(const std::string& library_path) const;
    
    /**
     * @brief Perform topological sort for dependency resolution
     */
    std::vector<std::string> topological_sort_dependencies(
        const std::unordered_map<std::string, std::vector<std::string>>& dependencies) const;
};

//=============================================================================
// Template Implementations
//=============================================================================

template<typename FunctionType>
FunctionType DynamicLoader::get_function(LibraryHandle handle, const std::string& function_name) {
    static_assert(std::is_function_v<std::remove_pointer_t<FunctionType>>, 
                  "FunctionType must be a function pointer type");
    
    SymbolInfo symbol = resolve_symbol(handle, function_name);
    if (!symbol.is_valid()) {
        return nullptr;
    }
    
    return reinterpret_cast<FunctionType>(symbol.address);
}

template<typename FunctionType>
FunctionType DynamicLoader::get_function(const std::string& function_name) {
    static_assert(std::is_function_v<std::remove_pointer_t<FunctionType>>, 
                  "FunctionType must be a function pointer type");
    
    SymbolInfo symbol = resolve_symbol(function_name);
    if (!symbol.is_valid()) {
        return nullptr;
    }
    
    return reinterpret_cast<FunctionType>(symbol.address);
}

template<typename DataType>
DataType* DynamicLoader::get_data(LibraryHandle handle, const std::string& data_name) {
    SymbolInfo symbol = resolve_symbol(handle, data_name);
    if (!symbol.is_valid()) {
        return nullptr;
    }
    
    return reinterpret_cast<DataType*>(symbol.address);
}

template<typename DataType>
DataType* DynamicLoader::get_data(const std::string& data_name) {
    SymbolInfo symbol = resolve_symbol(data_name);
    if (!symbol.is_valid()) {
        return nullptr;
    }
    
    return reinterpret_cast<DataType*>(symbol.address);
}

//=============================================================================
// Utility Functions
//=============================================================================

/**
 * @brief Get platform-specific library extension
 */
std::string get_library_extension();

/**
 * @brief Get platform-specific library prefix
 */
std::string get_library_prefix();

/**
 * @brief Build library filename from name
 */
std::string build_library_filename(const std::string& library_name);

/**
 * @brief Find library in system paths
 */
std::optional<std::string> find_library_in_system_paths(const std::string& library_name);

/**
 * @brief Get system library search paths
 */
std::vector<std::string> get_system_library_paths();

/**
 * @brief Convert loading mode to platform flags
 */
i32 loading_mode_to_platform_flags(LoadingMode mode);

} // namespace ecscope::plugin