#include "ecscope/gui/asset_pipeline_ui.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <fstream>

#ifdef ECSCOPE_HAS_IMGUI
#include <imgui.h>
#include <imgui_internal.h>
#endif

namespace ecscope {
namespace gui {

AssetPipelineUI::AssetPipelineUI()
    : show_window_(true)
    , show_browser_(true)
    , show_inspector_(true)
    , show_import_queue_(false)
    , show_collections_(true)
    , show_search_panel_(false)
    , browser_width_(400.0f)
    , inspector_width_(300.0f)
    , is_dragging_(false) {
}

AssetPipelineUI::~AssetPipelineUI() {
    shutdown();
}

bool AssetPipelineUI::initialize(const std::string& project_root) {
#ifdef ECSCOPE_HAS_IMGUI
    project_root_ = project_root;
    
    browser_ = std::make_unique<AssetBrowser>();
    browser_->initialize(project_root_);
    
    inspector_ = std::make_unique<AssetInspector>();
    inspector_->initialize();
    
    importer_ = std::make_unique<AssetImporter>();
    importer_->initialize();
    
    preview_generator_ = std::make_unique<AssetPreviewGenerator>();
    preview_generator_->initialize();
    
    // Set up browser callbacks
    browser_->set_selection_callback([this](const std::vector<std::string>& selected) {
        if (!selected.empty()) {
            inspector_->set_selected_asset(selected[0]);
        } else {
            inspector_->clear_selection();
        }
    });
    
    browser_->set_double_click_callback([this](const std::string& asset_path) {
        if (asset_loaded_callback_) {
            asset_loaded_callback_(asset_path);
        }
    });
    
    create_default_collections();
    scan_project_directory();
    
    AssetPipelineManager::instance().register_asset_pipeline_ui(this);
    return true;
#else
    return false;
#endif
}

void AssetPipelineUI::render() {
#ifdef ECSCOPE_HAS_IMGUI
    if (!show_window_) return;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar;
    
    if (ImGui::Begin("Asset Pipeline", &show_window_, window_flags)) {
        render_menu_bar();
        render_toolbar();
        
        // Main layout with splitter
        ImGui::Columns(3, "AssetPipelineColumns", true);
        ImGui::SetColumnWidth(0, browser_width_);
        ImGui::SetColumnWidth(1, ImGui::GetContentRegionAvail().x - inspector_width_);
        ImGui::SetColumnWidth(2, inspector_width_);
        
        // Asset Browser Column
        if (show_browser_) {
            ImGui::Text("Asset Browser");
            ImGui::Separator();
            render_asset_browser();
        }
        
        ImGui::NextColumn();
        
        // Central Content Area
        if (show_collections_) {
            render_collections_panel();
            ImGui::Separator();
        }
        
        if (show_import_queue_) {
            render_import_queue();
            ImGui::Separator();
        }
        
        if (show_search_panel_) {
            render_search_panel();
        }
        
        ImGui::NextColumn();
        
        // Asset Inspector Column
        if (show_inspector_) {
            ImGui::Text("Asset Inspector");
            ImGui::Separator();
            render_asset_inspector();
        }
        
        ImGui::Columns(1);
        
        // Drag and drop handling
        handle_drag_drop();
        render_drag_drop_target();
    }
    ImGui::End();
#endif
}

void AssetPipelineUI::render_menu_bar() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Import Assets...")) {
                // Open file dialog for importing assets
            }
            if (ImGui::MenuItem("Export Selected...")) {
                // Export selected assets
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Refresh Database")) {
                refresh_asset_database();
            }
            if (ImGui::MenuItem("Rebuild Previews")) {
                // Rebuild all previews
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Asset Browser", nullptr, &show_browser_);
            ImGui::MenuItem("Asset Inspector", nullptr, &show_inspector_);
            ImGui::MenuItem("Import Queue", nullptr, &show_import_queue_);
            ImGui::MenuItem("Collections", nullptr, &show_collections_);
            ImGui::MenuItem("Search Panel", nullptr, &show_search_panel_);
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Tools")) {
            if (ImGui::MenuItem("Create Collection...")) {
                // Open create collection dialog
            }
            if (ImGui::MenuItem("Asset Validation")) {
                // Run asset validation
            }
            if (ImGui::MenuItem("Cleanup Unused Assets")) {
                // Clean up unused assets
            }
            if (ImGui::MenuItem("Generate Missing Previews")) {
                // Generate missing previews
            }
            ImGui::EndMenu();
        }
        
        ImGui::EndMenuBar();
    }
#endif
}

void AssetPipelineUI::render_toolbar() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::Button("Import")) {
        // Open import dialog
    }
    ImGui::SameLine();
    if (ImGui::Button("Refresh")) {
        browser_->refresh_directory();
        refresh_asset_database();
    }
    ImGui::SameLine();
    if (ImGui::Button("New Folder")) {
        // Create new folder dialog
    }
    
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 200);
    ImGui::SetNextItemWidth(200);
    if (ImGui::InputText("##Search", search_query_.data(), search_query_.capacity())) {
        // Perform search
        search_results_.clear();
        if (!search_query_.empty()) {
            std::lock_guard<std::mutex> lock(assets_mutex_);
            for (const auto& [id, metadata] : assets_) {
                if (metadata.name.find(search_query_) != std::string::npos) {
                    search_results_.push_back(id);
                }
            }
        }
    }
#endif
}

void AssetPipelineUI::render_asset_browser() {
#ifdef ECSCOPE_HAS_IMGUI
    browser_->render();
#endif
}

void AssetPipelineUI::render_asset_inspector() {
#ifdef ECSCOPE_HAS_IMGUI
    inspector_->render();
#endif
}

void AssetPipelineUI::render_import_queue() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::CollapsingHeader("Import Queue", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto active_imports = importer_->get_active_imports();
        
        if (active_imports.empty()) {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No active imports");
        } else {
            for (const auto& job : active_imports) {
                ImGui::PushID(job.id.c_str());
                
                // Status indicator
                ImVec4 status_color;
                const char* status_text;
                switch (job.status) {
                    case ImportStatus::Processing:
                        status_color = ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
                        status_text = "Processing";
                        break;
                    case ImportStatus::Pending:
                        status_color = ImVec4(0.8f, 0.8f, 0.2f, 1.0f);
                        status_text = "Pending";
                        break;
                    case ImportStatus::Failed:
                        status_color = ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
                        status_text = "Failed";
                        break;
                    default:
                        status_color = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
                        status_text = "Unknown";
                        break;
                }
                
                ImGui::TextColored(status_color, "%s", status_text);
                ImGui::SameLine();
                ImGui::Text("%s", std::filesystem::path(job.source_path).filename().string().c_str());
                
                if (job.status == ImportStatus::Processing) {
                    ImGui::ProgressBar(job.progress, ImVec2(-1, 0));
                }
                
                if (job.status == ImportStatus::Failed && !job.error_message.empty()) {
                    ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "Error: %s", job.error_message.c_str());
                }
                
                if (job.status == ImportStatus::Processing && ImGui::SmallButton("Cancel")) {
                    importer_->cancel_import(job.id);
                }
                
                ImGui::PopID();
                ImGui::Separator();
            }
        }
    }
#endif
}

void AssetPipelineUI::render_collections_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::CollapsingHeader("Collections", ImGuiTreeNodeFlags_DefaultOpen)) {
        std::lock_guard<std::mutex> lock(collections_mutex_);
        
        for (auto& [name, collection] : collections_) {
            ImGui::PushID(name.c_str());
            
            ImGui::ColorButton("##CollectionColor", ImGui::ColorConvertU32ToFloat4(collection.color),
                             ImGuiColorEditFlags_NoTooltip, ImVec2(12, 12));
            ImGui::SameLine();
            
            if (ImGui::TreeNode(name.c_str())) {
                collection.is_expanded = true;
                
                ImGui::Text("Assets: %zu", collection.asset_ids.size());
                if (!collection.description.empty()) {
                    ImGui::TextWrapped("Description: %s", collection.description.c_str());
                }
                
                ImGui::Separator();
                
                for (const auto& asset_id : collection.asset_ids) {
                    auto asset_it = assets_.find(asset_id);
                    if (asset_it != assets_.end()) {
                        ImGui::Selectable(asset_it->second.name.c_str());
                        
                        if (ImGui::BeginPopupContextItem()) {
                            if (ImGui::MenuItem("Remove from Collection")) {
                                remove_from_collection(name, asset_id);
                            }
                            ImGui::EndPopup();
                        }
                    }
                }
                
                ImGui::TreePop();
            } else {
                collection.is_expanded = false;
            }
            
            ImGui::PopID();
        }
        
        if (ImGui::Button("New Collection...")) {
            // Open create collection dialog
        }
    }
#endif
}

void AssetPipelineUI::render_search_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::CollapsingHeader("Search Results", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (search_results_.empty() && !search_query_.empty()) {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No results found for '%s'", search_query_.c_str());
        } else if (!search_results_.empty()) {
            ImGui::Text("Found %zu results:", search_results_.size());
            ImGui::Separator();
            
            std::lock_guard<std::mutex> lock(assets_mutex_);
            for (const auto& asset_id : search_results_) {
                auto asset_it = assets_.find(asset_id);
                if (asset_it != assets_.end()) {
                    const auto& metadata = asset_it->second;
                    
                    ImU32 type_color = asset_type_colors_[static_cast<int>(metadata.type)];
                    ImGui::ColorButton("##TypeColor", ImGui::ColorConvertU32ToFloat4(type_color),
                                     ImGuiColorEditFlags_NoTooltip, ImVec2(12, 12));
                    ImGui::SameLine();
                    
                    if (ImGui::Selectable(metadata.name.c_str())) {
                        inspector_->set_selected_asset(asset_id);
                    }
                }
            }
        }
    }
#endif
}

void AssetPipelineUI::render_drag_drop_target() {
#ifdef ECSCOPE_HAS_IMGUI
    if (is_dragging_ && !drag_drop_files_.empty()) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 window_pos = ImGui::GetWindowPos();
        ImVec2 window_size = ImGui::GetWindowSize();
        
        // Draw overlay
        draw_list->AddRectFilled(window_pos, ImVec2(window_pos.x + window_size.x, window_pos.y + window_size.y),
                               IM_COL32(0, 120, 255, 50));
        draw_list->AddRect(window_pos, ImVec2(window_pos.x + window_size.x, window_pos.y + window_size.y),
                         IM_COL32(0, 120, 255, 200), 0.0f, 0, 3.0f);
        
        // Draw text
        ImVec2 text_pos = ImVec2(window_pos.x + window_size.x * 0.5f - 100, window_pos.y + window_size.y * 0.5f);
        draw_list->AddText(text_pos, IM_COL32(255, 255, 255, 255), "Drop files here to import");
    }
#endif
}

void AssetPipelineUI::handle_drag_drop() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::BeginDragDropTarget()) {
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILES");
        if (payload) {
            // Process dropped files
            std::vector<std::string> dropped_files;
            // Parse payload data for file paths
            // This would be implemented based on the specific drag-drop system used
            
            import_assets(dropped_files);
        }
        ImGui::EndDragDropTarget();
    }
#endif
}

void AssetPipelineUI::update(float delta_time) {
    if (importer_) {
        importer_->update_import_queue();
    }
    
    if (preview_generator_) {
        preview_generator_->update_preview_queue();
    }
    
    update_asset_watchers();
}

void AssetPipelineUI::shutdown() {
    AssetPipelineManager::instance().unregister_asset_pipeline_ui(this);
    
    if (preview_generator_) {
        preview_generator_->shutdown();
    }
    
    if (importer_) {
        importer_->shutdown();
    }
}

void AssetPipelineUI::add_asset(const AssetMetadata& metadata) {
    std::lock_guard<std::mutex> lock(assets_mutex_);
    assets_[metadata.id] = metadata;
}

void AssetPipelineUI::update_asset(const std::string& asset_id, const AssetMetadata& metadata) {
    std::lock_guard<std::mutex> lock(assets_mutex_);
    auto it = assets_.find(asset_id);
    if (it != assets_.end()) {
        it->second = metadata;
    }
}

void AssetPipelineUI::remove_asset(const std::string& asset_id) {
    std::lock_guard<std::mutex> lock(assets_mutex_);
    assets_.erase(asset_id);
}

AssetMetadata* AssetPipelineUI::get_asset(const std::string& asset_id) {
    std::lock_guard<std::mutex> lock(assets_mutex_);
    auto it = assets_.find(asset_id);
    return (it != assets_.end()) ? &it->second : nullptr;
}

std::vector<AssetMetadata> AssetPipelineUI::get_all_assets() const {
    std::lock_guard<std::mutex> lock(assets_mutex_);
    std::vector<AssetMetadata> result;
    result.reserve(assets_.size());
    
    for (const auto& [id, metadata] : assets_) {
        result.push_back(metadata);
    }
    
    return result;
}

void AssetPipelineUI::create_collection(const std::string& name, const std::string& description) {
    std::lock_guard<std::mutex> lock(collections_mutex_);
    
    AssetCollection collection;
    collection.name = name;
    collection.description = description;
    collection.color = IM_COL32(100 + (name.size() * 30) % 155, 
                               100 + (name.size() * 50) % 155,
                               100 + (name.size() * 70) % 155, 255);
    collection.is_expanded = false;
    
    collections_[name] = collection;
}

void AssetPipelineUI::add_to_collection(const std::string& collection_name, const std::string& asset_id) {
    std::lock_guard<std::mutex> lock(collections_mutex_);
    auto it = collections_.find(collection_name);
    if (it != collections_.end()) {
        auto& asset_ids = it->second.asset_ids;
        if (std::find(asset_ids.begin(), asset_ids.end(), asset_id) == asset_ids.end()) {
            asset_ids.push_back(asset_id);
        }
    }
}

void AssetPipelineUI::remove_from_collection(const std::string& collection_name, const std::string& asset_id) {
    std::lock_guard<std::mutex> lock(collections_mutex_);
    auto it = collections_.find(collection_name);
    if (it != collections_.end()) {
        auto& asset_ids = it->second.asset_ids;
        asset_ids.erase(std::remove(asset_ids.begin(), asset_ids.end(), asset_id), asset_ids.end());
    }
}

void AssetPipelineUI::import_assets(const std::vector<std::string>& source_paths) {
    if (!importer_) return;
    
    for (const auto& source_path : source_paths) {
        std::string job_id = importer_->import_asset(source_path, browser_->get_current_directory());
        if (!job_id.empty()) {
            show_import_queue_ = true;
        }
    }
}

void AssetPipelineUI::create_default_collections() {
    create_collection("Textures", "Image assets and texture files");
    create_collection("Models", "3D models and mesh files");
    create_collection("Audio", "Sound effects and music files");
    create_collection("Scripts", "Code and scripting files");
    create_collection("Materials", "Material definitions and shaders");
}

void AssetPipelineUI::scan_project_directory() {
    if (project_root_.empty() || !std::filesystem::exists(project_root_)) return;
    
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(project_root_)) {
            if (entry.is_regular_file()) {
                AssetMetadata metadata;
                metadata.id = entry.path().string();
                metadata.name = entry.path().filename().string();
                metadata.path = entry.path().string();
                metadata.source_path = entry.path().string();
                metadata.type = detect_asset_type_from_extension(entry.path().extension().string());
                metadata.status = AssetStatus::NotLoaded;
                metadata.file_size = std::filesystem::file_size(entry);
                metadata.created_time = std::chrono::system_clock::now();
                metadata.modified_time = std::chrono::system_clock::from_time_t(
                    std::chrono::system_clock::to_time_t(
                        std::chrono::file_clock::to_sys(entry.last_write_time())));
                metadata.has_preview = false;
                
                add_asset(metadata);
            }
        }
    } catch (const std::exception& e) {
        // Handle filesystem errors
    }
}

AssetType AssetPipelineUI::detect_asset_type_from_extension(const std::string& extension) {
    std::string lower_ext = extension;
    std::transform(lower_ext.begin(), lower_ext.end(), lower_ext.begin(), ::tolower);
    
    if (lower_ext == ".png" || lower_ext == ".jpg" || lower_ext == ".jpeg" || 
        lower_ext == ".bmp" || lower_ext == ".tga" || lower_ext == ".dds") {
        return AssetType::Texture;
    } else if (lower_ext == ".obj" || lower_ext == ".fbx" || lower_ext == ".dae" ||
               lower_ext == ".gltf" || lower_ext == ".glb") {
        return AssetType::Model;
    } else if (lower_ext == ".wav" || lower_ext == ".mp3" || lower_ext == ".ogg" ||
               lower_ext == ".flac") {
        return AssetType::Audio;
    } else if (lower_ext == ".cpp" || lower_ext == ".h" || lower_ext == ".hpp" ||
               lower_ext == ".js" || lower_ext == ".lua" || lower_ext == ".py") {
        return AssetType::Script;
    } else if (lower_ext == ".glsl" || lower_ext == ".hlsl" || lower_ext == ".shader") {
        return AssetType::Shader;
    } else if (lower_ext == ".mat" || lower_ext == ".material") {
        return AssetType::Material;
    } else if (lower_ext == ".anim" || lower_ext == ".animation") {
        return AssetType::Animation;
    } else if (lower_ext == ".ttf" || lower_ext == ".otf") {
        return AssetType::Font;
    } else if (lower_ext == ".mp4" || lower_ext == ".avi" || lower_ext == ".mov") {
        return AssetType::Video;
    } else if (lower_ext == ".json" || lower_ext == ".xml" || lower_ext == ".yaml") {
        return AssetType::Data;
    } else if (lower_ext == ".scene") {
        return AssetType::Scene;
    }
    
    return AssetType::Unknown;
}

void AssetPipelineUI::refresh_asset_database() {
    {
        std::lock_guard<std::mutex> lock(assets_mutex_);
        assets_.clear();
    }
    scan_project_directory();
}

void AssetPipelineUI::update_asset_watchers() {
    // This would implement file system watching to detect changes
    // and update asset metadata accordingly
}

// AssetBrowser implementation
void AssetBrowser::initialize(const std::string& root_directory) {
    root_directory_ = root_directory;
    current_directory_ = root_directory;
    thumbnail_size_ = 64.0f;
    show_hidden_files_ = false;
    sort_mode_ = 0; // By name
    ascending_sort_ = true;
    
    refresh_directory();
}

void AssetBrowser::render() {
#ifdef ECSCOPE_HAS_IMGUI
    render_breadcrumbs();
    render_toolbar();
    
    ImGui::Separator();
    
    ImGui::Columns(2, "BrowserColumns", true);
    ImGui::SetColumnWidth(0, 200);
    
    render_directory_tree();
    
    ImGui::NextColumn();
    
    render_file_grid();
    
    ImGui::Columns(1);
    
    render_context_menu();
#endif
}

void AssetBrowser::render_breadcrumbs() {
#ifdef ECSCOPE_HAS_IMGUI
    std::filesystem::path current_path(current_directory_);
    std::vector<std::filesystem::path> path_parts;
    
    for (auto part = current_path; part != part.root_path(); part = part.parent_path()) {
        path_parts.push_back(part);
    }
    std::reverse(path_parts.begin(), path_parts.end());
    
    for (size_t i = 0; i < path_parts.size(); ++i) {
        if (i > 0) {
            ImGui::SameLine();
            ImGui::Text(" > ");
            ImGui::SameLine();
        }
        
        if (ImGui::Button(path_parts[i].filename().string().c_str())) {
            set_current_directory(path_parts[i].string());
        }
    }
#endif
}

void AssetBrowser::render_toolbar() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::Button("Up")) {
        std::filesystem::path parent = std::filesystem::path(current_directory_).parent_path();
        if (parent != current_directory_ && parent.string().find(root_directory_) == 0) {
            set_current_directory(parent.string());
        }
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Refresh")) {
        refresh_directory();
    }
    
    ImGui::SameLine();
    ImGui::SliderFloat("Size", &thumbnail_size_, 32.0f, 128.0f, "%.0f");
    
    ImGui::SameLine();
    ImGui::Checkbox("Hidden", &show_hidden_files_);
#endif
}

void AssetBrowser::render_directory_tree() {
#ifdef ECSCOPE_HAS_IMGUI
    // This would implement a tree view of the directory structure
    ImGui::Text("Directory Tree");
    // Simplified implementation - would show actual tree structure
#endif
}

void AssetBrowser::render_file_grid() {
#ifdef ECSCOPE_HAS_IMGUI
    ImVec2 grid_size = calculate_grid_size();
    int columns = static_cast<int>(grid_size.x);
    
    if (columns > 0) {
        ImGui::Columns(columns, "FileGrid", false);
        
        for (const auto& path : directory_contents_) {
            if (!show_hidden_files_ && path.filename().string()[0] == '.') {
                continue;
            }
            
            ImGui::PushID(path.string().c_str());
            
            bool is_directory = std::filesystem::is_directory(path);
            bool selected = is_selected(path);
            
            // Draw thumbnail/icon
            ImVec2 thumbnail_pos = ImGui::GetCursorPos();
            if (ImGui::Selectable("##thumbnail", selected, 0, ImVec2(thumbnail_size_, thumbnail_size_))) {
                // Handle selection
                selected_items_.clear();
                selected_items_.push_back(path.string());
                
                if (selection_callback_) {
                    selection_callback_(selected_items_);
                }
            }
            
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                if (is_directory) {
                    set_current_directory(path.string());
                } else if (double_click_callback_) {
                    double_click_callback_(path.string());
                }
            }
            
            // Draw file name below thumbnail
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
            ImGui::TextWrapped("%s", path.filename().string().c_str());
            
            ImGui::NextColumn();
            ImGui::PopID();
        }
        
        ImGui::Columns(1);
    }
#endif
}

void AssetBrowser::render_context_menu() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::BeginPopupContextWindow("BrowserContextMenu")) {
        if (ImGui::MenuItem("New Folder")) {
            // Create new folder
        }
        
        if (ImGui::MenuItem("Import Assets...")) {
            // Import assets
        }
        
        if (!selected_items_.empty()) {
            ImGui::Separator();
            if (ImGui::MenuItem("Delete Selected")) {
                // Delete selected items
            }
            if (ImGui::MenuItem("Rename")) {
                // Rename selected item
            }
        }
        
        ImGui::EndPopup();
    }
#endif
}

ImVec2 AssetBrowser::calculate_grid_size() {
    ImVec2 available = ImGui::GetContentRegionAvail();
    float item_width = thumbnail_size_ + 10.0f;
    int columns = static_cast<int>(available.x / item_width);
    return ImVec2(static_cast<float>(std::max(1, columns)), 0);
}

void AssetBrowser::set_current_directory(const std::string& path) {
    if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
        current_directory_ = path;
        refresh_directory();
    }
}

void AssetBrowser::refresh_directory() {
    directory_contents_.clear();
    selected_items_.clear();
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(current_directory_)) {
            directory_contents_.push_back(entry.path());
        }
        sort_directory_contents();
    } catch (const std::exception&) {
        // Handle filesystem errors
    }
}

void AssetBrowser::sort_directory_contents() {
    std::sort(directory_contents_.begin(), directory_contents_.end(),
              [this](const std::filesystem::path& a, const std::filesystem::path& b) {
                  bool a_is_dir = std::filesystem::is_directory(a);
                  bool b_is_dir = std::filesystem::is_directory(b);
                  
                  // Directories first
                  if (a_is_dir != b_is_dir) {
                      return a_is_dir;
                  }
                  
                  // Then sort by name
                  std::string a_name = a.filename().string();
                  std::string b_name = b.filename().string();
                  
                  if (ascending_sort_) {
                      return a_name < b_name;
                  } else {
                      return a_name > b_name;
                  }
              });
}

bool AssetBrowser::is_selected(const std::filesystem::path& path) const {
    return std::find(selected_items_.begin(), selected_items_.end(), path.string()) != selected_items_.end();
}

// AssetInspector implementation
void AssetInspector::initialize() {
}

void AssetInspector::render() {
#ifdef ECSCOPE_HAS_IMGUI
    if (selected_asset_id_.empty()) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No asset selected");
        return;
    }
    
    render_general_properties();
    render_preview();
    render_type_specific_properties();
    render_dependencies();
    render_import_settings();
#endif
}

void AssetInspector::render_general_properties() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Name: %s", current_metadata_.name.c_str());
        ImGui::Text("Type: %s", "Asset Type"); // Would show actual type
        ImGui::Text("Size: %zu bytes", current_metadata_.file_size);
        ImGui::Text("Status: %s", "Loaded"); // Would show actual status
    }
#endif
}

void AssetInspector::render_preview() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::CollapsingHeader("Preview", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (current_metadata_.has_preview && current_metadata_.preview_texture_id != 0) {
            ImGui::Image(reinterpret_cast<void*>(current_metadata_.preview_texture_id), ImVec2(128, 128));
        } else {
            ImGui::Text("No preview available");
        }
    }
#endif
}

void AssetInspector::render_type_specific_properties() {
#ifdef ECSCOPE_HAS_IMGUI
    switch (current_metadata_.type) {
        case AssetType::Texture:
            render_texture_properties();
            break;
        case AssetType::Model:
            render_model_properties();
            break;
        case AssetType::Audio:
            render_audio_properties();
            break;
        case AssetType::Script:
            render_script_properties();
            break;
        default:
            break;
    }
#endif
}

void AssetInspector::render_texture_properties() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::CollapsingHeader("Texture Properties")) {
        ImGui::Text("Texture-specific properties would go here");
    }
#endif
}

void AssetInspector::render_model_properties() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::CollapsingHeader("Model Properties")) {
        ImGui::Text("Model-specific properties would go here");
    }
#endif
}

void AssetInspector::render_audio_properties() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::CollapsingHeader("Audio Properties")) {
        ImGui::Text("Audio-specific properties would go here");
    }
#endif
}

void AssetInspector::render_script_properties() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::CollapsingHeader("Script Properties")) {
        ImGui::Text("Script-specific properties would go here");
    }
#endif
}

void AssetInspector::render_dependencies() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::CollapsingHeader("Dependencies")) {
        ImGui::Text("Asset dependencies would be listed here");
    }
#endif
}

void AssetInspector::render_import_settings() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::CollapsingHeader("Import Settings")) {
        ImGui::Text("Import settings would go here");
    }
#endif
}

void AssetInspector::set_selected_asset(const std::string& asset_id) {
    selected_asset_id_ = asset_id;
    // Would load actual metadata here
}

void AssetInspector::clear_selection() {
    selected_asset_id_.clear();
}

// Placeholder implementations for other classes
void AssetPreviewGenerator::initialize() {}
void AssetPreviewGenerator::shutdown() {}
bool AssetPreviewGenerator::generate_preview(const std::string&, AssetType, const std::string&) { return false; }
bool AssetPreviewGenerator::has_preview_support(AssetType) const { return false; }
void AssetPreviewGenerator::update_preview_queue() {}
bool AssetPreviewGenerator::generate_texture_preview(const std::string&, const std::string&) { return false; }
bool AssetPreviewGenerator::generate_model_preview(const std::string&, const std::string&) { return false; }
bool AssetPreviewGenerator::generate_audio_preview(const std::string&, const std::string&) { return false; }

void AssetImporter::initialize() { next_job_id_ = 1; }
void AssetImporter::shutdown() {}
std::string AssetImporter::import_asset(const std::string&, const std::string&, const std::unordered_map<std::string, std::string>&) { return ""; }
void AssetImporter::cancel_import(const std::string&) {}
ImportJob AssetImporter::get_import_status(const std::string&) { return {}; }
std::vector<ImportJob> AssetImporter::get_active_imports() { return {}; }
void AssetImporter::update_import_queue() {}
AssetType AssetImporter::detect_asset_type(const std::string&) { return AssetType::Unknown; }
bool AssetImporter::import_texture(const ImportJob&) { return false; }
bool AssetImporter::import_model(const ImportJob&) { return false; }
bool AssetImporter::import_audio(const ImportJob&) { return false; }
bool AssetImporter::import_script(const ImportJob&) { return false; }

// AssetPipelineManager implementation
void AssetPipelineManager::initialize(const std::string& project_root) {
    project_root_ = project_root;
}

void AssetPipelineManager::shutdown() {
    std::lock_guard<std::mutex> lock(ui_mutex_);
    registered_uis_.clear();
}

void AssetPipelineManager::register_asset_pipeline_ui(AssetPipelineUI* ui) {
    std::lock_guard<std::mutex> lock(ui_mutex_);
    registered_uis_.push_back(ui);
}

void AssetPipelineManager::unregister_asset_pipeline_ui(AssetPipelineUI* ui) {
    std::lock_guard<std::mutex> lock(ui_mutex_);
    registered_uis_.erase(
        std::remove(registered_uis_.begin(), registered_uis_.end(), ui),
        registered_uis_.end());
}

void AssetPipelineManager::notify_asset_changed(const AssetMetadata& metadata) {
    std::lock_guard<std::mutex> lock(ui_mutex_);
    for (auto* ui : registered_uis_) {
        ui->update_asset(metadata.id, metadata);
    }
}

void AssetPipelineManager::notify_asset_imported(const std::string& asset_id, bool success) {
    // Notify UIs about import completion
}

void AssetPipelineManager::update(float delta_time) {
    // Update manager state
}

}} // namespace ecscope::gui