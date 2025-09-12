#pragma once

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <mutex>
#include <filesystem>

#ifdef ECSCOPE_HAS_IMGUI
#include <imgui.h>
#endif

namespace ecscope {
namespace gui {

enum class PluginState {
    NotLoaded,
    Loading,
    Loaded,
    Unloading,
    Failed,
    Disabled,
    UpdateAvailable
};

enum class PluginType {
    Core,
    Rendering,
    Audio,
    Physics,
    Networking,
    Scripting,
    Tools,
    Custom
};

enum class InstallationStatus {
    NotInstalled,
    Installing,
    Installed,
    Updating,
    Uninstalling,
    Failed,
    Corrupted
};

enum class DependencyType {
    Required,
    Optional,
    Conflicting
};

struct PluginVersion {
    int major;
    int minor;
    int patch;
    std::string pre_release;
    
    std::string to_string() const {
        std::string version = std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
        if (!pre_release.empty()) {
            version += "-" + pre_release;
        }
        return version;
    }
    
    bool operator<(const PluginVersion& other) const {
        if (major != other.major) return major < other.major;
        if (minor != other.minor) return minor < other.minor;
        if (patch != other.patch) return patch < other.patch;
        return pre_release < other.pre_release;
    }
    
    bool operator==(const PluginVersion& other) const {
        return major == other.major && minor == other.minor && 
               patch == other.patch && pre_release == other.pre_release;
    }
};

struct PluginDependency {
    std::string plugin_id;
    std::string plugin_name;
    PluginVersion min_version;
    PluginVersion max_version;
    DependencyType type;
    bool is_satisfied;
    std::string description;
};

struct PluginMetadata {
    std::string id;
    std::string name;
    std::string description;
    std::string author;
    std::string website;
    std::string license;
    PluginVersion version;
    PluginType type;
    PluginState state;
    InstallationStatus installation_status;
    
    // File information
    std::string file_path;
    std::string config_path;
    size_t file_size;
    std::chrono::system_clock::time_point install_date;
    std::chrono::system_clock::time_point last_loaded;
    
    // Dependencies
    std::vector<PluginDependency> dependencies;
    std::vector<std::string> dependents;
    
    // Configuration
    std::unordered_map<std::string, std::string> settings;
    std::unordered_map<std::string, std::string> default_settings;
    
    // Statistics
    uint32_t load_count;
    float average_load_time_ms;
    bool is_essential;
    bool auto_load;
    
    // Update information
    PluginVersion available_version;
    std::string update_url;
    std::string changelog;
};

struct PluginRepository {
    std::string id;
    std::string name;
    std::string url;
    std::string description;
    bool is_enabled;
    bool is_trusted;
    std::chrono::system_clock::time_point last_updated;
    std::vector<PluginMetadata> available_plugins;
};

struct InstallationJob {
    std::string job_id;
    std::string plugin_id;
    std::string plugin_name;
    InstallationStatus status;
    float progress;
    std::string current_operation;
    std::string error_message;
    std::chrono::steady_clock::time_point start_time;
    size_t total_bytes;
    size_t downloaded_bytes;
};

class PluginInstaller {
public:
    PluginInstaller() = default;
    ~PluginInstaller() = default;

    void initialize();
    void shutdown();
    
    std::string install_plugin(const std::string& plugin_id, const std::string& source_url);
    std::string update_plugin(const std::string& plugin_id);
    std::string uninstall_plugin(const std::string& plugin_id);
    
    void cancel_installation(const std::string& job_id);
    InstallationJob get_installation_status(const std::string& job_id);
    std::vector<InstallationJob> get_active_jobs();
    
    void update_installation_queue();

private:
    std::unordered_map<std::string, InstallationJob> installation_jobs_;
    std::mutex jobs_mutex_;
    uint32_t next_job_id_;

    bool download_plugin(const std::string& url, const std::string& output_path, InstallationJob& job);
    bool verify_plugin_integrity(const std::string& file_path);
    bool extract_plugin(const std::string& archive_path, const std::string& destination);
    bool install_plugin_files(const std::string& source_dir, const std::string& plugin_id);
};

class PluginLoader {
public:
    PluginLoader() = default;
    ~PluginLoader() = default;

    void initialize();
    void shutdown();
    
    bool load_plugin(const std::string& plugin_id);
    bool unload_plugin(const std::string& plugin_id);
    bool reload_plugin(const std::string& plugin_id);
    
    void enable_plugin(const std::string& plugin_id, bool enabled);
    bool is_plugin_loaded(const std::string& plugin_id) const;
    
    std::vector<std::string> get_loaded_plugins() const;
    float get_plugin_load_time(const std::string& plugin_id) const;

private:
    struct LoadedPlugin {
        void* handle;
        std::chrono::steady_clock::time_point load_time;
        float load_duration_ms;
    };

    std::unordered_map<std::string, LoadedPlugin> loaded_plugins_;
    std::mutex loader_mutex_;

    void* load_dynamic_library(const std::string& file_path);
    void unload_dynamic_library(void* handle);
    bool validate_plugin_interface(void* handle);
};

class PluginConfigEditor {
public:
    PluginConfigEditor() = default;
    ~PluginConfigEditor() = default;

    void initialize();
    void render(PluginMetadata& plugin);
    
    void load_plugin_config(const std::string& plugin_id);
    void save_plugin_config(const std::string& plugin_id);
    void reset_to_defaults(const std::string& plugin_id);
    
    void set_config_value(const std::string& plugin_id, const std::string& key, const std::string& value);
    std::string get_config_value(const std::string& plugin_id, const std::string& key) const;

private:
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> plugin_configs_;
    std::string selected_plugin_id_;

    void render_config_section(const std::string& section_name, 
                              std::unordered_map<std::string, std::string>& config);
    void render_config_item(const std::string& key, std::string& value);
    
    bool is_boolean_setting(const std::string& key) const;
    bool is_numeric_setting(const std::string& key) const;
    bool is_file_path_setting(const std::string& key) const;
};

class PluginBrowser {
public:
    PluginBrowser() = default;
    ~PluginBrowser() = default;

    void initialize();
    void render();
    void update();
    
    void add_repository(const std::string& name, const std::string& url);
    void remove_repository(const std::string& id);
    void refresh_repositories();
    
    std::vector<PluginRepository> get_repositories() const;
    std::vector<PluginMetadata> search_plugins(const std::string& query) const;
    std::vector<PluginMetadata> filter_plugins(PluginType type) const;

private:
    std::vector<PluginRepository> repositories_;
    std::vector<PluginMetadata> available_plugins_;
    std::string search_query_;
    PluginType filter_type_;
    
    std::mutex browser_mutex_;

    void render_repository_list();
    void render_plugin_grid();
    void render_plugin_details(const PluginMetadata& plugin);
    void render_search_filters();
    
    bool fetch_repository_data(PluginRepository& repo);
    void parse_plugin_manifest(const std::string& manifest_data, std::vector<PluginMetadata>& plugins);
};

class PluginManagerUI {
public:
    PluginManagerUI();
    ~PluginManagerUI();

    bool initialize(const std::string& plugins_directory);
    void render();
    void update(float delta_time);
    void shutdown();

    // Plugin management
    void refresh_plugin_list();
    bool load_plugin(const std::string& plugin_id);
    bool unload_plugin(const std::string& plugin_id);
    void enable_plugin(const std::string& plugin_id, bool enabled);
    
    // Installation management
    void install_plugin(const std::string& plugin_id, const std::string& source_url = "");
    void update_plugin(const std::string& plugin_id);
    void uninstall_plugin(const std::string& plugin_id);
    
    // Configuration
    void edit_plugin_config(const std::string& plugin_id);
    void save_plugin_configs();
    void load_plugin_configs();
    
    // Repository management
    void add_plugin_repository(const std::string& name, const std::string& url);
    void refresh_plugin_repositories();
    
    // Callbacks
    void set_plugin_loaded_callback(std::function<void(const std::string&, bool)> callback);
    void set_plugin_installed_callback(std::function<void(const std::string&, bool)> callback);
    void set_plugin_error_callback(std::function<void(const std::string&, const std::string&)> callback);

    bool is_window_open() const { return show_window_; }
    void set_window_open(bool open) { show_window_ = open; }

private:
    void render_menu_bar();
    void render_plugin_list();
    void render_plugin_details();
    void render_installation_queue();
    void render_plugin_browser();
    void render_repository_manager();
    void render_plugin_config_editor();
    
    void scan_plugins_directory();
    void load_plugin_metadata(const std::string& plugin_path, PluginMetadata& metadata);
    void check_plugin_dependencies(PluginMetadata& plugin);
    void resolve_dependency_conflicts();
    
    void update_installation_jobs();
    void update_plugin_states();

    std::string plugins_directory_;
    std::vector<PluginMetadata> installed_plugins_;
    std::string selected_plugin_id_;
    
    std::unique_ptr<PluginInstaller> installer_;
    std::unique_ptr<PluginLoader> loader_;
    std::unique_ptr<PluginConfigEditor> config_editor_;
    std::unique_ptr<PluginBrowser> browser_;
    
    std::function<void(const std::string&, bool)> plugin_loaded_callback_;
    std::function<void(const std::string&, bool)> plugin_installed_callback_;
    std::function<void(const std::string&, const std::string&)> plugin_error_callback_;

    bool show_window_;
    bool show_plugin_list_;
    bool show_plugin_details_;
    bool show_installation_queue_;
    bool show_plugin_browser_;
    bool show_repository_manager_;
    bool show_config_editor_;
    
    // UI State
    float list_width_;
    float details_width_;
    std::string search_filter_;
    PluginType type_filter_;
    PluginState state_filter_;
    
    std::mutex plugins_mutex_;
    
    // Plugin type colors
    ImU32 plugin_type_colors_[8] = {
        IM_COL32(255, 100, 100, 255), // Core
        IM_COL32(100, 255, 100, 255), // Rendering
        IM_COL32(100, 100, 255, 255), // Audio
        IM_COL32(255, 255, 100, 255), // Physics
        IM_COL32(255, 150, 100, 255), // Networking
        IM_COL32(150, 255, 150, 255), // Scripting
        IM_COL32(150, 150, 255, 255), // Tools
        IM_COL32(200, 200, 200, 255)  // Custom
    };
};

class PluginManagerSystem {
public:
    static PluginManagerSystem& instance() {
        static PluginManagerSystem instance;
        return instance;
    }

    void initialize(const std::string& plugins_directory);
    void shutdown();
    void update(float delta_time);

    void register_plugin_manager_ui(PluginManagerUI* ui);
    void unregister_plugin_manager_ui(PluginManagerUI* ui);

    void notify_plugin_state_changed(const std::string& plugin_id, PluginState state);
    void notify_installation_progress(const std::string& job_id, float progress);

    // Plugin interface for external plugins
    bool register_plugin_interface(const std::string& interface_name, void* interface_ptr);
    void* get_plugin_interface(const std::string& interface_name);
    
    void register_plugin_hook(const std::string& hook_name, std::function<void()> callback);
    void trigger_plugin_hook(const std::string& hook_name);

private:
    PluginManagerSystem() = default;
    ~PluginManagerSystem() = default;

    std::vector<PluginManagerUI*> registered_uis_;
    std::unordered_map<std::string, void*> plugin_interfaces_;
    std::unordered_map<std::string, std::vector<std::function<void()>>> plugin_hooks_;
    std::mutex ui_mutex_;
    std::mutex interfaces_mutex_;
    std::string plugins_directory_;
};

// Plugin API structures for external plugins
struct PluginAPI {
    // Core functions every plugin must implement
    const char* (*get_plugin_name)();
    const char* (*get_plugin_version)();
    const char* (*get_plugin_description)();
    
    // Lifecycle functions
    bool (*initialize)(void* engine_interface);
    void (*shutdown)();
    void (*update)(float delta_time);
    
    // Configuration
    const char* (*get_config_schema)();
    void (*set_config)(const char* key, const char* value);
    const char* (*get_config)(const char* key);
    
    // Dependencies
    int (*get_dependency_count)();
    const char* (*get_dependency)(int index);
    
    // Optional GUI integration
    void (*render_gui)();
    void (*render_menu_items)();
};

// Convenience macros for plugin development
#define ECSCOPE_PLUGIN_EXPORT extern "C" __declspec(dllexport)
#define ECSCOPE_PLUGIN_API_VERSION 1

#define ECSCOPE_DECLARE_PLUGIN(name, version, desc) \
    ECSCOPE_PLUGIN_EXPORT const char* get_plugin_name() { return name; } \
    ECSCOPE_PLUGIN_EXPORT const char* get_plugin_version() { return version; } \
    ECSCOPE_PLUGIN_EXPORT const char* get_plugin_description() { return desc; }

}} // namespace ecscope::gui