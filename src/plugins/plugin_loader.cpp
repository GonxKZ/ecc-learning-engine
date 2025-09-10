#include "ecscope/plugins/plugin_loader.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <chrono>
#include <regex>

#ifdef _WIN32
    #include <shlwapi.h>
    #pragma comment(lib, "shlwapi.lib")
#else
    #include <unistd.h>
    #include <sys/stat.h>
#endif

namespace ecscope::plugins {

    PluginLoader::PluginLoader() {
        // Set default search paths
        #ifdef _WIN32
            add_search_path("./plugins");
            add_search_path("../plugins");
            // Add system plugin directories if they exist
            if (std::filesystem::exists("C:/Program Files/ECScope/plugins")) {
                add_search_path("C:/Program Files/ECScope/plugins");
            }
        #else
            add_search_path("./plugins");
            add_search_path("../plugins");
            add_search_path("/usr/local/lib/ecscope/plugins");
            add_search_path("/usr/lib/ecscope/plugins");
        #endif
        
        // Set default security policy (allow all for now)
        security_policy_ = [](const std::string&) { return true; };
    }
    
    PluginLoader::~PluginLoader() {
        // Unload all libraries
        for (auto& [path, info] : loaded_libraries_) {
            if (info.handle) {
                unload_library_impl(info.handle);
            }
        }
        loaded_libraries_.clear();
        handle_to_path_.clear();
    }
    
    LoadInfo PluginLoader::load_library(const std::string& library_path) {
        LoadInfo info;
        info.plugin_path = normalize_path(library_path);
        
        // Check if already loaded
        if (is_library_loaded(info.plugin_path)) {
            info.result = LoadResult::AlreadyLoaded;
            info.error_message = "Library is already loaded";
            return info;
        }
        
        // Check file exists
        if (!std::filesystem::exists(info.plugin_path)) {
            // Try to find the library using search paths
            std::string found_path = find_library(std::filesystem::path(library_path).stem().string());
            if (found_path.empty()) {
                info.result = LoadResult::FileNotFound;
                info.error_message = "Plugin library not found: " + library_path;
                return info;
            }
            info.plugin_path = found_path;
        }
        
        // Security checks
        if (!check_security_policy(info.plugin_path)) {
            info.result = LoadResult::SecurityViolation;
            info.error_message = "Security policy violation";
            return info;
        }
        
        if (!check_file_permissions(info.plugin_path)) {
            info.result = LoadResult::PermissionDenied;
            info.error_message = "Insufficient file permissions";
            return info;
        }
        
        // Validate library format
        if (!validate_library_format(info.plugin_path)) {
            info.result = LoadResult::InvalidFormat;
            info.error_message = "Invalid library format";
            return info;
        }
        
        // Load the library
        LibraryHandle handle = load_library_impl(info.plugin_path);
        if (!handle) {
            info.result = LoadResult::InitializationFailed;
            info.error_message = "Failed to load library: " + get_library_error();
            return info;
        }
        
        // Validate API version
        if (!validate_api_version(handle)) {
            unload_library_impl(handle);
            info.result = LoadResult::IncompatibleVersion;
            info.error_message = "Incompatible plugin API version";
            return info;
        }
        
        // Get plugin export information
        auto get_export_func = get_function<PluginExport*(*)()>(handle, "get_plugin_export");
        if (!get_export_func) {
            unload_library_impl(handle);
            info.result = LoadResult::MissingSymbols;
            info.error_message = "Missing get_plugin_export symbol";
            return info;
        }
        
        PluginExport* export_info = get_export_func();
        if (!export_info || !export_info->metadata) {
            unload_library_impl(handle);
            info.result = LoadResult::MissingSymbols;
            info.error_message = "Invalid plugin export information";
            return info;
        }
        
        // Validate metadata
        if (!validate_metadata(*export_info->metadata)) {
            unload_library_impl(handle);
            info.result = LoadResult::InvalidFormat;
            info.error_message = "Invalid plugin metadata";
            return info;
        }
        
        // Store information
        info.library_handle = handle;
        info.export_info = export_info;
        info.metadata = *export_info->metadata;
        info.result = LoadResult::Success;
        
        // Add to loaded libraries
        LibraryInfo lib_info;
        lib_info.path = info.plugin_path;
        lib_info.handle = handle;
        lib_info.load_info = info;
        lib_info.load_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        loaded_libraries_[info.plugin_path] = lib_info;
        handle_to_path_[handle] = info.plugin_path;
        
        return info;
    }
    
    bool PluginLoader::unload_library(const std::string& library_path) {
        std::string normalized_path = normalize_path(library_path);
        auto it = loaded_libraries_.find(normalized_path);
        if (it == loaded_libraries_.end()) {
            return false;
        }
        
        LibraryHandle handle = it->second.handle;
        
        // Call cleanup function if available
        if (it->second.load_info.export_info && it->second.load_info.export_info->cleanup) {
            it->second.load_info.export_info->cleanup();
        }
        
        // Unload the library
        bool success = unload_library_impl(handle);
        
        // Remove from tracking
        handle_to_path_.erase(handle);
        loaded_libraries_.erase(it);
        
        return success;
    }
    
    bool PluginLoader::unload_library(LibraryHandle handle) {
        auto it = handle_to_path_.find(handle);
        if (it == handle_to_path_.end()) {
            return false;
        }
        
        return unload_library(it->second);
    }
    
    std::unique_ptr<IPlugin> PluginLoader::create_plugin(const LoadInfo& load_info) {
        if (!load_info.is_success() || !load_info.export_info || !load_info.export_info->factory) {
            return nullptr;
        }
        
        try {
            return load_info.export_info->factory();
        } catch (const std::exception& e) {
            last_error_ = "Plugin factory exception: " + std::string(e.what());
            return nullptr;
        } catch (...) {
            last_error_ = "Unknown plugin factory exception";
            return nullptr;
        }
    }
    
    bool PluginLoader::validate_plugin(const LoadInfo& load_info) {
        if (!load_info.is_success()) {
            return false;
        }
        
        // Create a temporary plugin instance to validate it
        auto plugin = create_plugin(load_info);
        if (!plugin) {
            return false;
        }
        
        // Check if metadata matches
        const auto& plugin_metadata = plugin->get_metadata();
        if (plugin_metadata.name != load_info.metadata.name ||
            plugin_metadata.version.to_string() != load_info.metadata.version.to_string()) {
            return false;
        }
        
        return plugin_metadata.is_valid();
    }
    
    bool PluginLoader::supports_hot_swap(const std::string& library_path) {
        // Hot-swapping requires the library to support it
        // For now, assume all libraries support it
        return true;
    }
    
    LoadInfo PluginLoader::hot_swap_library(const std::string& library_path) {
        // First, unload the existing library
        if (is_library_loaded(library_path)) {
            unload_library(library_path);
        }
        
        // Then load the new version
        return load_library(library_path);
    }
    
    std::vector<std::string> PluginLoader::get_loaded_libraries() const {
        std::vector<std::string> libraries;
        libraries.reserve(loaded_libraries_.size());
        
        for (const auto& [path, info] : loaded_libraries_) {
            libraries.push_back(path);
        }
        
        return libraries;
    }
    
    bool PluginLoader::is_library_loaded(const std::string& library_path) const {
        return loaded_libraries_.find(normalize_path(library_path)) != loaded_libraries_.end();
    }
    
    LoadInfo PluginLoader::get_load_info(const std::string& library_path) const {
        auto it = loaded_libraries_.find(normalize_path(library_path));
        if (it != loaded_libraries_.end()) {
            return it->second.load_info;
        }
        
        LoadInfo info;
        info.result = LoadResult::FileNotFound;
        return info;
    }
    
    std::string PluginLoader::get_last_error() const {
        return last_error_;
    }
    
    void PluginLoader::clear_error() {
        last_error_.clear();
    }
    
    bool PluginLoader::verify_signature(const std::string& library_path) {
        // TODO: Implement digital signature verification
        // For now, return true (no verification)
        return true;
    }
    
    bool PluginLoader::check_security_policy(const std::string& library_path) {
        return security_policy_ ? security_policy_(library_path) : true;
    }
    
    void PluginLoader::set_security_policy(std::function<bool(const std::string&)> policy) {
        security_policy_ = std::move(policy);
    }
    
    void PluginLoader::add_search_path(const std::string& path) {
        std::string normalized = normalize_path(path);
        if (std::find(search_paths_.begin(), search_paths_.end(), normalized) == search_paths_.end()) {
            search_paths_.push_back(normalized);
        }
    }
    
    void PluginLoader::remove_search_path(const std::string& path) {
        std::string normalized = normalize_path(path);
        search_paths_.erase(
            std::remove(search_paths_.begin(), search_paths_.end(), normalized),
            search_paths_.end()
        );
    }
    
    std::vector<std::string> PluginLoader::get_search_paths() const {
        return search_paths_;
    }
    
    std::string PluginLoader::find_library(const std::string& library_name) {
        std::vector<std::string> variants = get_library_variants(library_name);
        
        for (const std::string& search_path : search_paths_) {
            if (!std::filesystem::exists(search_path)) {
                continue;
            }
            
            for (const std::string& variant : variants) {
                std::filesystem::path full_path = std::filesystem::path(search_path) / variant;
                if (std::filesystem::exists(full_path)) {
                    return full_path.string();
                }
            }
        }
        
        return "";
    }
    
    // Platform-specific implementations
    
    LibraryHandle PluginLoader::load_library_impl(const std::string& path) {
        clear_error();
        
        #ifdef _WIN32
            return LoadLibraryA(path.c_str());
        #else
            return dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL);
        #endif
    }
    
    bool PluginLoader::unload_library_impl(LibraryHandle handle) {
        if (!handle) {
            return false;
        }
        
        clear_error();
        
        #ifdef _WIN32
            return FreeLibrary(handle) != 0;
        #else
            return dlclose(handle) == 0;
        #endif
    }
    
    void* PluginLoader::get_symbol(LibraryHandle handle, const std::string& symbol_name) {
        if (!handle) {
            return nullptr;
        }
        
        clear_error();
        
        #ifdef _WIN32
            return GetProcAddress(handle, symbol_name.c_str());
        #else
            return dlsym(handle, symbol_name.c_str());
        #endif
    }
    
    std::string PluginLoader::get_library_error() const {
        #ifdef _WIN32
            DWORD error = GetLastError();
            if (error == 0) return "";
            
            LPSTR messageBuffer = nullptr;
            size_t size = FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
            
            std::string message(messageBuffer, size);
            LocalFree(messageBuffer);
            return message;
        #else
            const char* error = dlerror();
            return error ? error : "";
        #endif
    }
    
    bool PluginLoader::validate_library_format(const std::string& path) {
        // Basic file extension check
        std::string extension = std::filesystem::path(path).extension().string();
        std::string expected_ext = get_library_extension();
        
        return extension == expected_ext;
    }
    
    bool PluginLoader::validate_api_version(LibraryHandle handle) {
        auto get_version_func = get_function<int(*)()>(handle, "get_plugin_api_version");
        if (!get_version_func) {
            return false; // Assume old version if no function
        }
        
        int plugin_api_version = get_version_func();
        return plugin_api_version == PLUGIN_API_VERSION;
    }
    
    bool PluginLoader::validate_metadata(const PluginMetadata& metadata) {
        return metadata.is_valid();
    }
    
    bool PluginLoader::check_file_permissions(const std::string& path) {
        #ifdef _WIN32
            // Check if file is readable
            DWORD attributes = GetFileAttributesA(path.c_str());
            return (attributes != INVALID_FILE_ATTRIBUTES) && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
        #else
            return access(path.c_str(), R_OK) == 0;
        #endif
    }
    
    bool PluginLoader::scan_for_malicious_code(const std::string& path) {
        // TODO: Implement malware scanning
        // For now, return true (no scanning)
        return true;
    }
    
    std::string PluginLoader::normalize_path(const std::string& path) {
        return std::filesystem::absolute(path).string();
    }
    
    std::string PluginLoader::get_library_extension() {
        #ifdef _WIN32
            return ".dll";
        #elif defined(__APPLE__)
            return ".dylib";
        #else
            return ".so";
        #endif
    }
    
    std::vector<std::string> PluginLoader::get_library_variants(const std::string& base_name) {
        std::vector<std::string> variants;
        std::string ext = get_library_extension();
        
        // Try various naming conventions
        variants.push_back(base_name + ext);
        variants.push_back("lib" + base_name + ext);
        variants.push_back(base_name + "_plugin" + ext);
        variants.push_back("lib" + base_name + "_plugin" + ext);
        
        return variants;
    }

} // namespace ecscope::plugins