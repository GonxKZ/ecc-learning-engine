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

enum class AssetType {
    Unknown,
    Texture,
    Model,
    Audio,
    Script,
    Shader,
    Material,
    Animation,
    Font,
    Video,
    Data,
    Scene
};

enum class AssetStatus {
    NotLoaded,
    Loading,
    Loaded,
    Failed,
    Modified,
    Outdated
};

enum class ImportStatus {
    Pending,
    Processing,
    Completed,
    Failed,
    Cancelled
};

struct AssetMetadata {
    std::string id;
    std::string name;
    std::string path;
    std::string source_path;
    AssetType type;
    AssetStatus status;
    size_t file_size;
    std::chrono::system_clock::time_point created_time;
    std::chrono::system_clock::time_point modified_time;
    std::chrono::system_clock::time_point last_accessed;
    
    // Type-specific metadata
    std::unordered_map<std::string, std::string> properties;
    
    // Dependencies
    std::vector<std::string> dependencies;
    std::vector<std::string> dependents;
    
    // Preview information
    bool has_preview;
    std::string preview_path;
    uint32_t preview_texture_id;
    
    // Import settings
    std::unordered_map<std::string, std::string> import_settings;
};

struct ImportJob {
    std::string id;
    std::string source_path;
    std::string target_path;
    AssetType type;
    ImportStatus status;
    float progress;
    std::string error_message;
    std::chrono::steady_clock::time_point start_time;
    std::unordered_map<std::string, std::string> settings;
};

struct AssetCollection {
    std::string name;
    std::string description;
    std::vector<std::string> asset_ids;
    ImU32 color;
    bool is_expanded;
};

class AssetPreviewGenerator {
public:
    AssetPreviewGenerator() = default;
    ~AssetPreviewGenerator() = default;

    void initialize();
    void shutdown();
    
    bool generate_preview(const std::string& asset_path, AssetType type, const std::string& output_path);
    bool has_preview_support(AssetType type) const;
    void update_preview_queue();

private:
    struct PreviewRequest {
        std::string asset_path;
        std::string output_path;
        AssetType type;
        std::chrono::steady_clock::time_point request_time;
    };

    std::vector<PreviewRequest> preview_queue_;
    std::mutex queue_mutex_;
    bool processing_previews_;

    bool generate_texture_preview(const std::string& input_path, const std::string& output_path);
    bool generate_model_preview(const std::string& input_path, const std::string& output_path);
    bool generate_audio_preview(const std::string& input_path, const std::string& output_path);
};

class AssetImporter {
public:
    AssetImporter() = default;
    ~AssetImporter() = default;

    void initialize();
    void shutdown();
    
    std::string import_asset(const std::string& source_path, const std::string& target_directory,
                           const std::unordered_map<std::string, std::string>& settings = {});
    void cancel_import(const std::string& job_id);
    ImportJob get_import_status(const std::string& job_id);
    std::vector<ImportJob> get_active_imports();
    
    void update_import_queue();

private:
    std::unordered_map<std::string, ImportJob> import_jobs_;
    std::mutex jobs_mutex_;
    uint32_t next_job_id_;

    AssetType detect_asset_type(const std::string& file_path);
    bool import_texture(const ImportJob& job);
    bool import_model(const ImportJob& job);
    bool import_audio(const ImportJob& job);
    bool import_script(const ImportJob& job);
};

class AssetBrowser {
public:
    AssetBrowser() = default;
    ~AssetBrowser() = default;

    void initialize(const std::string& root_directory);
    void render();
    void update();
    
    void set_current_directory(const std::string& path);
    std::string get_current_directory() const { return current_directory_; }
    
    void refresh_directory();
    void create_folder(const std::string& name);
    void delete_asset(const std::string& asset_id);
    void rename_asset(const std::string& asset_id, const std::string& new_name);
    
    void set_selection_callback(std::function<void(const std::vector<std::string>&)> callback);
    void set_double_click_callback(std::function<void(const std::string&)> callback);

private:
    std::string root_directory_;
    std::string current_directory_;
    std::vector<std::filesystem::path> directory_contents_;
    std::vector<std::string> selected_items_;
    
    std::function<void(const std::vector<std::string>&)> selection_callback_;
    std::function<void(const std::string&)> double_click_callback_;
    
    float thumbnail_size_;
    bool show_hidden_files_;
    int sort_mode_;
    bool ascending_sort_;
    
    void render_breadcrumbs();
    void render_toolbar();
    void render_directory_tree();
    void render_file_grid();
    void render_context_menu();
    
    ImVec2 calculate_grid_size();
    void sort_directory_contents();
    bool is_selected(const std::filesystem::path& path) const;
};

class AssetInspector {
public:
    AssetInspector() = default;
    ~AssetInspector() = default;

    void initialize();
    void render();
    void set_selected_asset(const std::string& asset_id);
    void clear_selection();

private:
    std::string selected_asset_id_;
    AssetMetadata current_metadata_;
    
    void render_general_properties();
    void render_type_specific_properties();
    void render_dependencies();
    void render_preview();
    void render_import_settings();
    
    void render_texture_properties();
    void render_model_properties();
    void render_audio_properties();
    void render_script_properties();
};

class AssetPipelineUI {
public:
    AssetPipelineUI();
    ~AssetPipelineUI();

    bool initialize(const std::string& project_root);
    void render();
    void update(float delta_time);
    void shutdown();

    // Asset management
    void add_asset(const AssetMetadata& metadata);
    void update_asset(const std::string& asset_id, const AssetMetadata& metadata);
    void remove_asset(const std::string& asset_id);
    AssetMetadata* get_asset(const std::string& asset_id);
    std::vector<AssetMetadata> get_all_assets() const;
    
    // Collections
    void create_collection(const std::string& name, const std::string& description);
    void add_to_collection(const std::string& collection_name, const std::string& asset_id);
    void remove_from_collection(const std::string& collection_name, const std::string& asset_id);
    
    // Import/Export
    void import_assets(const std::vector<std::string>& source_paths);
    void export_assets(const std::vector<std::string>& asset_ids, const std::string& target_path);
    
    // Callbacks
    void set_asset_loaded_callback(std::function<void(const std::string&)> callback);
    void set_asset_modified_callback(std::function<void(const std::string&)> callback);
    void set_import_completed_callback(std::function<void(const std::string&, bool)> callback);

    bool is_window_open() const { return show_window_; }
    void set_window_open(bool open) { show_window_ = open; }

private:
    void render_menu_bar();
    void render_toolbar();
    void render_asset_browser();
    void render_asset_inspector();
    void render_import_queue();
    void render_collections_panel();
    void render_search_panel();
    void render_drag_drop_target();
    
    void handle_drag_drop();
    void handle_file_operations();
    void update_asset_watchers();
    void refresh_asset_database();
    
    void create_default_collections();
    void scan_project_directory();
    AssetType detect_asset_type_from_extension(const std::string& extension);
    
    std::string project_root_;
    std::unordered_map<std::string, AssetMetadata> assets_;
    std::unordered_map<std::string, AssetCollection> collections_;
    
    std::unique_ptr<AssetBrowser> browser_;
    std::unique_ptr<AssetInspector> inspector_;
    std::unique_ptr<AssetImporter> importer_;
    std::unique_ptr<AssetPreviewGenerator> preview_generator_;
    
    std::function<void(const std::string&)> asset_loaded_callback_;
    std::function<void(const std::string&)> asset_modified_callback_;
    std::function<void(const std::string&, bool)> import_completed_callback_;

    bool show_window_;
    bool show_browser_;
    bool show_inspector_;
    bool show_import_queue_;
    bool show_collections_;
    bool show_search_panel_;
    
    std::string search_query_;
    std::vector<std::string> search_results_;
    
    // UI state
    float browser_width_;
    float inspector_width_;
    bool is_dragging_;
    std::vector<std::string> drag_drop_files_;
    
    std::mutex assets_mutex_;
    std::mutex collections_mutex_;
    
    // Asset type colors
    ImU32 asset_type_colors_[12] = {
        IM_COL32(128, 128, 128, 255), // Unknown
        IM_COL32(255, 100, 100, 255), // Texture
        IM_COL32(100, 255, 100, 255), // Model
        IM_COL32(100, 100, 255, 255), // Audio
        IM_COL32(255, 255, 100, 255), // Script
        IM_COL32(255, 150, 100, 255), // Shader
        IM_COL32(150, 255, 150, 255), // Material
        IM_COL32(150, 150, 255, 255), // Animation
        IM_COL32(255, 200, 150, 255), // Font
        IM_COL32(200, 150, 255, 255), // Video
        IM_COL32(150, 255, 200, 255), // Data
        IM_COL32(255, 150, 200, 255)  // Scene
    };
};

class AssetPipelineManager {
public:
    static AssetPipelineManager& instance() {
        static AssetPipelineManager instance;
        return instance;
    }

    void initialize(const std::string& project_root);
    void shutdown();
    void update(float delta_time);

    void register_asset_pipeline_ui(AssetPipelineUI* ui);
    void unregister_asset_pipeline_ui(AssetPipelineUI* ui);

    void notify_asset_changed(const AssetMetadata& metadata);
    void notify_asset_imported(const std::string& asset_id, bool success);

private:
    AssetPipelineManager() = default;
    ~AssetPipelineManager() = default;

    std::vector<AssetPipelineUI*> registered_uis_;
    std::mutex ui_mutex_;
    std::string project_root_;
};

}} // namespace ecscope::gui