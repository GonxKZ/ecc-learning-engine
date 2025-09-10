#pragma once

#include "plugin_interface.hpp"
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <map>

#ifdef _WIN32
    #include <windows.h>
    using LibraryHandle = HMODULE;
#else
    #include <dlfcn.h>
    using LibraryHandle = void*;
#endif

namespace ecscope::plugins {

    /**
     * @brief Plugin loading result
     */
    enum class LoadResult {
        Success,
        FileNotFound,
        InvalidFormat,
        IncompatibleVersion,
        MissingSymbols,
        InitializationFailed,
        SecurityViolation,
        DependencyMissing,
        AlreadyLoaded,
        PermissionDenied
    };
    
    /**
     * @brief Plugin loading information
     */
    struct LoadInfo {
        LoadResult result{LoadResult::Success};
        std::string error_message;
        std::string plugin_path;
        PluginMetadata metadata;
        LibraryHandle library_handle{nullptr};
        PluginExport* export_info{nullptr};
        
        bool is_success() const { return result == LoadResult::Success; }
    };
    
    /**
     * @brief Platform-specific dynamic library loader
     * 
     * Handles cross-platform loading of plugin libraries with proper
     * symbol resolution, version checking, and security validation.
     */
    class PluginLoader {
    public:
        PluginLoader();
        ~PluginLoader();
        
        // Library loading and unloading
        LoadInfo load_library(const std::string& library_path);
        bool unload_library(const std::string& library_path);
        bool unload_library(LibraryHandle handle);
        
        // Plugin creation and management
        std::unique_ptr<IPlugin> create_plugin(const LoadInfo& load_info);
        bool validate_plugin(const LoadInfo& load_info);
        
        // Hot-swapping support
        bool supports_hot_swap(const std::string& library_path);
        LoadInfo hot_swap_library(const std::string& library_path);
        
        // Symbol resolution
        template<typename FuncType>
        FuncType get_function(LibraryHandle handle, const std::string& symbol_name) {
            return reinterpret_cast<FuncType>(get_symbol(handle, symbol_name));
        }
        
        // Library information
        std::vector<std::string> get_loaded_libraries() const;
        bool is_library_loaded(const std::string& library_path) const;
        LoadInfo get_load_info(const std::string& library_path) const;
        
        // Error handling
        std::string get_last_error() const;
        void clear_error();
        
        // Security and validation
        bool verify_signature(const std::string& library_path);
        bool check_security_policy(const std::string& library_path);
        void set_security_policy(std::function<bool(const std::string&)> policy);
        
        // Library search paths
        void add_search_path(const std::string& path);
        void remove_search_path(const std::string& path);
        std::vector<std::string> get_search_paths() const;
        std::string find_library(const std::string& library_name);
        
    private:
        struct LibraryInfo {
            std::string path;
            LibraryHandle handle;
            LoadInfo load_info;
            uint64_t load_time;
            bool hot_swappable{false};
        };
        
        std::map<std::string, LibraryInfo> loaded_libraries_;
        std::map<LibraryHandle, std::string> handle_to_path_;
        std::vector<std::string> search_paths_;
        std::function<bool(const std::string&)> security_policy_;
        std::string last_error_;
        
        // Platform-specific implementations
        LibraryHandle load_library_impl(const std::string& path);
        bool unload_library_impl(LibraryHandle handle);
        void* get_symbol(LibraryHandle handle, const std::string& symbol_name);
        std::string get_library_error() const;
        
        // Validation helpers
        bool validate_library_format(const std::string& path);
        bool validate_api_version(LibraryHandle handle);
        bool validate_metadata(const PluginMetadata& metadata);
        
        // Security helpers
        bool check_file_permissions(const std::string& path);
        bool scan_for_malicious_code(const std::string& path);
        
        // Path helpers
        std::string normalize_path(const std::string& path);
        std::string get_library_extension();
        std::vector<std::string> get_library_variants(const std::string& base_name);
    };
    
    /**
     * @brief Plugin discovery system
     * 
     * Automatically discovers and catalogs available plugins in search directories.
     */
    class PluginDiscovery {
    public:
        struct PluginCandidate {
            std::string path;
            std::string name;
            PluginMetadata metadata;
            bool valid{false};
            std::string error_message;
        };
        
        PluginDiscovery(PluginLoader* loader);
        
        // Discovery operations
        std::vector<PluginCandidate> discover_plugins(const std::string& directory);
        std::vector<PluginCandidate> discover_plugins_recursive(const std::string& directory);
        void scan_for_plugins();
        
        // Plugin catalog management
        void add_plugin_directory(const std::string& directory);
        void remove_plugin_directory(const std::string& directory);
        std::vector<std::string> get_plugin_directories() const;
        
        // Plugin information
        std::vector<PluginCandidate> get_available_plugins() const;
        PluginCandidate find_plugin(const std::string& name) const;
        std::vector<PluginCandidate> find_plugins_by_tag(const std::string& tag) const;
        
        // Manifest file support
        bool load_manifest(const std::string& manifest_path, PluginMetadata& metadata);
        bool save_manifest(const std::string& manifest_path, const PluginMetadata& metadata);
        std::string get_default_manifest_name() const { return "plugin.json"; }
        
        // Filtering and sorting
        void set_filter(std::function<bool(const PluginCandidate&)> filter);
        void clear_filter();
        void sort_by_priority();
        void sort_by_name();
        void sort_by_version();
        
    private:
        PluginLoader* loader_;
        std::vector<std::string> plugin_directories_;
        std::vector<PluginCandidate> available_plugins_;
        std::function<bool(const PluginCandidate&)> filter_;
        
        // Discovery helpers
        bool is_plugin_file(const std::string& path);
        PluginCandidate analyze_plugin_file(const std::string& path);
        std::vector<std::string> find_manifest_files(const std::string& directory);
        
        // Manifest parsing
        bool parse_json_manifest(const std::string& content, PluginMetadata& metadata);
        std::string serialize_json_manifest(const PluginMetadata& metadata);
        
        // File system helpers
        std::vector<std::string> list_files(const std::string& directory, const std::string& extension = "");
        bool is_directory(const std::string& path);
        bool file_exists(const std::string& path);
        std::string read_file(const std::string& path);
        bool write_file(const std::string& path, const std::string& content);
    };
    
} // namespace ecscope::plugins