#include "ecscope/gui/plugin_manager_ui.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <fstream>
#include <regex>

#ifdef _WIN32
#include <windows.h>
#elif __linux__ || __APPLE__
#include <dlfcn.h>
#endif

#ifdef ECSCOPE_HAS_IMGUI
#include <imgui.h>
#include <imgui_internal.h>
#endif

namespace ecscope {
namespace gui {

PluginManagerUI::PluginManagerUI()
    : show_window_(true)
    , show_plugin_list_(true)
    , show_plugin_details_(true)
    , show_installation_queue_(false)
    , show_plugin_browser_(false)
    , show_repository_manager_(false)
    , show_config_editor_(false)
    , list_width_(300.0f)
    , details_width_(400.0f)
    , type_filter_(PluginType::Core)
    , state_filter_(PluginState::NotLoaded) {
    
    search_filter_.resize(256);
}

PluginManagerUI::~PluginManagerUI() {
    shutdown();
}

bool PluginManagerUI::initialize(const std::string& plugins_directory) {
#ifdef ECSCOPE_HAS_IMGUI
    plugins_directory_ = plugins_directory;
    
    installer_ = std::make_unique<PluginInstaller>();
    installer_->initialize();
    
    loader_ = std::make_unique<PluginLoader>();
    loader_->initialize();
    
    config_editor_ = std::make_unique<PluginConfigEditor>();
    config_editor_->initialize();
    
    browser_ = std::make_unique<PluginBrowser>();
    browser_->initialize();
    
    // Create plugins directory if it doesn't exist
    std::filesystem::create_directories(plugins_directory_);
    
    // Scan for existing plugins
    scan_plugins_directory();
    
    // Load plugin configurations
    load_plugin_configs();
    
    PluginManagerSystem::instance().register_plugin_manager_ui(this);
    return true;
#else
    return false;
#endif
}

void PluginManagerUI::render() {
#ifdef ECSCOPE_HAS_IMGUI
    if (!show_window_) return;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar;
    
    if (ImGui::Begin("Plugin Manager", &show_window_, window_flags)) {
        render_menu_bar();
        
        // Main layout with tabs
        if (ImGui::BeginTabBar("PluginManagerTabs")) {
            
            if (ImGui::BeginTabItem("Installed Plugins")) {
                ImGui::Columns(2, "PluginColumns", true);
                ImGui::SetColumnWidth(0, list_width_);
                
                if (show_plugin_list_) {
                    render_plugin_list();
                }
                
                ImGui::NextColumn();
                
                if (show_plugin_details_) {
                    render_plugin_details();
                }
                
                if (show_config_editor_) {
                    ImGui::Separator();
                    render_plugin_config_editor();
                }
                
                ImGui::Columns(1);
                ImGui::EndTabItem();
            }
            
            if (show_plugin_browser_ && ImGui::BeginTabItem("Browse Plugins")) {
                render_plugin_browser();
                ImGui::EndTabItem();
            }
            
            if (show_installation_queue_ && ImGui::BeginTabItem("Installation Queue")) {
                render_installation_queue();
                ImGui::EndTabItem();
            }
            
            if (show_repository_manager_ && ImGui::BeginTabItem("Repositories")) {
                render_repository_manager();
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
#endif
}

void PluginManagerUI::render_menu_bar() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Refresh Plugin List")) {
                refresh_plugin_list();
            }
            if (ImGui::MenuItem("Save Configurations")) {
                save_plugin_configs();
            }
            if (ImGui::MenuItem("Load Configurations")) {
                load_plugin_configs();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Install Plugin from File...")) {
                // Open file dialog for plugin installation
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Plugin List", nullptr, &show_plugin_list_);
            ImGui::MenuItem("Plugin Details", nullptr, &show_plugin_details_);
            ImGui::MenuItem("Installation Queue", nullptr, &show_installation_queue_);
            ImGui::MenuItem("Plugin Browser", nullptr, &show_plugin_browser_);
            ImGui::MenuItem("Repository Manager", nullptr, &show_repository_manager_);
            ImGui::MenuItem("Configuration Editor", nullptr, &show_config_editor_);
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Plugins")) {
            if (ImGui::MenuItem("Load All Plugins")) {
                for (auto& plugin : installed_plugins_) {
                    if (plugin.state == PluginState::NotLoaded && plugin.installation_status == InstallationStatus::Installed) {
                        load_plugin(plugin.id);
                    }
                }
            }
            if (ImGui::MenuItem("Unload All Plugins")) {
                for (auto& plugin : installed_plugins_) {
                    if (plugin.state == PluginState::Loaded) {
                        unload_plugin(plugin.id);
                    }
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Check for Updates")) {
                // Check for plugin updates
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Tools")) {
            if (ImGui::MenuItem("Validate All Plugins")) {
                // Validate plugin integrity
            }
            if (ImGui::MenuItem("Clean Plugin Cache")) {
                // Clean plugin cache
            }
            if (ImGui::MenuItem("Export Plugin List")) {
                // Export plugin list
            }
            ImGui::EndMenu();
        }
        
        ImGui::EndMenuBar();
    }
#endif
}

void PluginManagerUI::render_plugin_list() {
#ifdef ECSCOPE_HAS_IMGUI
    // Search and filter controls
    ImGui::Text("Search:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputText("##Search", search_filter_.data(), search_filter_.capacity())) {
        // Filter plugins based on search
    }
    
    ImGui::Text("Filter by Type:");
    const char* type_names[] = {"Core", "Rendering", "Audio", "Physics", "Networking", "Scripting", "Tools", "Custom"};
    int current_type = static_cast<int>(type_filter_);
    if (ImGui::Combo("##TypeFilter", &current_type, type_names, IM_ARRAYSIZE(type_names))) {
        type_filter_ = static_cast<PluginType>(current_type);
    }
    
    ImGui::Separator();
    
    // Plugin list
    if (ImGui::BeginChild("PluginList", ImVec2(0, 0), true)) {
        std::lock_guard<std::mutex> lock(plugins_mutex_);
        
        for (auto& plugin : installed_plugins_) {
            // Apply filters
            if (!search_filter_.empty() && 
                plugin.name.find(search_filter_) == std::string::npos &&
                plugin.description.find(search_filter_) == std::string::npos) {
                continue;
            }
            
            ImGui::PushID(plugin.id.c_str());
            
            // Plugin item
            bool is_selected = (selected_plugin_id_ == plugin.id);
            
            // Status indicator
            ImVec4 status_color;
            switch (plugin.state) {
                case PluginState::Loaded:
                    status_color = ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
                    break;
                case PluginState::Loading:
                    status_color = ImVec4(0.8f, 0.8f, 0.2f, 1.0f);
                    break;
                case PluginState::Failed:
                    status_color = ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
                    break;
                case PluginState::Disabled:
                    status_color = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
                    break;
                default:
                    status_color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
                    break;
            }
            
            ImGui::ColorButton("##Status", status_color, ImGuiColorEditFlags_NoTooltip, ImVec2(12, 12));
            ImGui::SameLine();
            
            // Type color indicator
            ImU32 type_color = plugin_type_colors_[static_cast<int>(plugin.type)];
            ImGui::ColorButton("##Type", ImGui::ColorConvertU32ToFloat4(type_color), 
                             ImGuiColorEditFlags_NoTooltip, ImVec2(12, 12));
            ImGui::SameLine();
            
            if (ImGui::Selectable(plugin.name.c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns)) {
                selected_plugin_id_ = plugin.id;
            }
            
            // Context menu
            if (ImGui::BeginPopupContextItem()) {
                if (plugin.state == PluginState::NotLoaded) {
                    if (ImGui::MenuItem("Load Plugin")) {
                        load_plugin(plugin.id);
                    }
                } else if (plugin.state == PluginState::Loaded) {
                    if (ImGui::MenuItem("Unload Plugin")) {
                        unload_plugin(plugin.id);
                    }
                    if (ImGui::MenuItem("Reload Plugin")) {
                        unload_plugin(plugin.id);
                        load_plugin(plugin.id);
                    }
                }
                
                if (ImGui::MenuItem("Configure")) {
                    edit_plugin_config(plugin.id);
                    show_config_editor_ = true;
                }
                
                if (ImGui::MenuItem("Uninstall")) {
                    uninstall_plugin(plugin.id);
                }
                
                ImGui::EndPopup();
            }
            
            // Tooltip with details
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s v%s\n%s\nAuthor: %s\nState: %s", 
                                plugin.name.c_str(),
                                plugin.version.to_string().c_str(),
                                plugin.description.c_str(),
                                plugin.author.c_str(),
                                plugin.state == PluginState::Loaded ? "Loaded" :
                                plugin.state == PluginState::Failed ? "Failed" : "Not Loaded");
            }
            
            ImGui::PopID();
        }
    }
    ImGui::EndChild();
#endif
}

void PluginManagerUI::render_plugin_details() {
#ifdef ECSCOPE_HAS_IMGUI
    if (selected_plugin_id_.empty()) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No plugin selected");
        return;
    }
    
    std::lock_guard<std::mutex> lock(plugins_mutex_);
    auto it = std::find_if(installed_plugins_.begin(), installed_plugins_.end(),
                          [this](const PluginMetadata& p) { return p.id == selected_plugin_id_; });
    
    if (it == installed_plugins_.end()) {
        ImGui::TextColored(ImVec4(0.8f, 0.3f, 0.3f, 1.0f), "Plugin not found");
        return;
    }
    
    const PluginMetadata& plugin = *it;
    
    if (ImGui::CollapsingHeader("General Information", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Name: %s", plugin.name.c_str());
        ImGui::Text("Version: %s", plugin.version.to_string().c_str());
        ImGui::Text("Author: %s", plugin.author.c_str());
        ImGui::Text("License: %s", plugin.license.c_str());
        
        if (!plugin.website.empty()) {
            ImGui::Text("Website: %s", plugin.website.c_str());
        }
        
        ImGui::Separator();
        ImGui::TextWrapped("Description: %s", plugin.description.c_str());
    }
    
    if (ImGui::CollapsingHeader("Status", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char* state_names[] = {"Not Loaded", "Loading", "Loaded", "Unloading", "Failed", "Disabled", "Update Available"};
        ImGui::Text("State: %s", state_names[static_cast<int>(plugin.state)]);
        
        const char* install_names[] = {"Not Installed", "Installing", "Installed", "Updating", "Uninstalling", "Failed", "Corrupted"};
        ImGui::Text("Installation: %s", install_names[static_cast<int>(plugin.installation_status)]);
        
        const char* type_names[] = {"Core", "Rendering", "Audio", "Physics", "Networking", "Scripting", "Tools", "Custom"};
        ImGui::Text("Type: %s", type_names[static_cast<int>(plugin.type)]);
        
        ImGui::Text("Essential: %s", plugin.is_essential ? "Yes" : "No");
        ImGui::Text("Auto Load: %s", plugin.auto_load ? "Yes" : "No");
        
        if (plugin.state == PluginState::Loaded) {
            ImGui::Text("Load Time: %.2f ms", plugin.average_load_time_ms);
            ImGui::Text("Load Count: %u", plugin.load_count);
        }
    }
    
    if (ImGui::CollapsingHeader("File Information")) {
        ImGui::Text("File Path: %s", plugin.file_path.c_str());
        ImGui::Text("Config Path: %s", plugin.config_path.c_str());
        ImGui::Text("File Size: %.2f KB", plugin.file_size / 1024.0f);
        
        auto install_time = std::chrono::system_clock::to_time_t(plugin.install_date);
        ImGui::Text("Install Date: %s", std::ctime(&install_time));
        
        if (plugin.state == PluginState::Loaded) {
            auto load_time = std::chrono::system_clock::to_time_t(plugin.last_loaded);
            ImGui::Text("Last Loaded: %s", std::ctime(&load_time));
        }
    }
    
    if (!plugin.dependencies.empty() && ImGui::CollapsingHeader("Dependencies")) {
        for (const auto& dep : plugin.dependencies) {
            ImVec4 dep_color = dep.is_satisfied ? ImVec4(0.3f, 0.8f, 0.3f, 1.0f) : ImVec4(0.8f, 0.3f, 0.3f, 1.0f);
            
            ImGui::TextColored(dep_color, "%s %s", dep.is_satisfied ? "✓" : "✗", dep.plugin_name.c_str());
            ImGui::SameLine();
            
            const char* dep_type_names[] = {"Required", "Optional", "Conflicting"};
            ImGui::Text("(%s)", dep_type_names[static_cast<int>(dep.type)]);
            
            if (!dep.description.empty()) {
                ImGui::TextWrapped("    %s", dep.description.c_str());
            }
        }
    }
    
    if (!plugin.dependents.empty() && ImGui::CollapsingHeader("Dependents")) {
        for (const auto& dependent : plugin.dependents) {
            ImGui::Text("• %s", dependent.c_str());
        }
    }
    
    // Action buttons
    ImGui::Separator();
    
    if (plugin.state == PluginState::NotLoaded) {
        if (ImGui::Button("Load Plugin")) {
            load_plugin(plugin.id);
        }
    } else if (plugin.state == PluginState::Loaded) {
        if (ImGui::Button("Unload Plugin")) {
            unload_plugin(plugin.id);
        }
        ImGui::SameLine();
        if (ImGui::Button("Reload Plugin")) {
            unload_plugin(plugin.id);
            load_plugin(plugin.id);
        }
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Configure")) {
        edit_plugin_config(plugin.id);
        show_config_editor_ = true;
    }
    
    if (plugin.available_version.major > 0 && plugin.available_version != plugin.version) {
        ImGui::SameLine();
        if (ImGui::Button("Update Available")) {
            update_plugin(plugin.id);
            show_installation_queue_ = true;
        }
    }
#endif
}

void PluginManagerUI::render_installation_queue() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::CollapsingHeader("Installation Queue", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto active_jobs = installer_->get_active_jobs();
        
        if (active_jobs.empty()) {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No active installation jobs");
        } else {
            for (const auto& job : active_jobs) {
                ImGui::PushID(job.job_id.c_str());
                
                // Status indicator
                ImVec4 status_color;
                const char* status_text;
                switch (job.status) {
                    case InstallationStatus::Installing:
                        status_color = ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
                        status_text = "Installing";
                        break;
                    case InstallationStatus::Updating:
                        status_color = ImVec4(0.8f, 0.8f, 0.2f, 1.0f);
                        status_text = "Updating";
                        break;
                    case InstallationStatus::Uninstalling:
                        status_color = ImVec4(0.8f, 0.4f, 0.2f, 1.0f);
                        status_text = "Uninstalling";
                        break;
                    case InstallationStatus::Failed:
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
                ImGui::Text("%s", job.plugin_name.c_str());
                
                if (job.status == InstallationStatus::Installing || job.status == InstallationStatus::Updating) {
                    ImGui::ProgressBar(job.progress, ImVec2(-1, 0));
                    ImGui::Text("Operation: %s", job.current_operation.c_str());
                    
                    if (job.total_bytes > 0) {
                        float download_progress = static_cast<float>(job.downloaded_bytes) / job.total_bytes;
                        ImGui::Text("Downloaded: %.2f MB / %.2f MB", 
                                  job.downloaded_bytes / (1024.0f * 1024.0f),
                                  job.total_bytes / (1024.0f * 1024.0f));
                    }
                }
                
                if (job.status == InstallationStatus::Failed && !job.error_message.empty()) {
                    ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "Error: %s", job.error_message.c_str());
                }
                
                if (job.status == InstallationStatus::Installing || job.status == InstallationStatus::Updating) {
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Cancel")) {
                        installer_->cancel_installation(job.job_id);
                    }
                }
                
                ImGui::PopID();
                ImGui::Separator();
            }
        }
    }
#endif
}

void PluginManagerUI::render_plugin_browser() {
#ifdef ECSCOPE_HAS_IMGUI
    browser_->render();
#endif
}

void PluginManagerUI::render_repository_manager() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::CollapsingHeader("Plugin Repositories", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto repositories = browser_->get_repositories();
        
        if (ImGui::Button("Add Repository")) {
            // Open add repository dialog
        }
        ImGui::SameLine();
        if (ImGui::Button("Refresh All")) {
            refresh_plugin_repositories();
        }
        
        ImGui::Separator();
        
        if (ImGui::BeginTable("RepositoriesTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("URL");
            ImGui::TableSetupColumn("Status");
            ImGui::TableSetupColumn("Actions");
            ImGui::TableHeadersRow();
            
            for (const auto& repo : repositories) {
                ImGui::TableNextRow();
                
                ImGui::TableNextColumn();
                ImGui::Text("%s", repo.name.c_str());
                
                ImGui::TableNextColumn();
                ImGui::Text("%s", repo.url.c_str());
                
                ImGui::TableNextColumn();
                ImVec4 status_color = repo.is_enabled ? 
                    (repo.is_trusted ? ImVec4(0.2f, 0.8f, 0.2f, 1.0f) : ImVec4(0.8f, 0.8f, 0.2f, 1.0f)) :
                    ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
                
                const char* status_text = repo.is_enabled ? 
                    (repo.is_trusted ? "Trusted" : "Enabled") : "Disabled";
                
                ImGui::TextColored(status_color, "%s", status_text);
                
                ImGui::TableNextColumn();
                ImGui::PushID(repo.id.c_str());
                if (ImGui::SmallButton("Edit")) {
                    // Edit repository
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("Remove")) {
                    browser_->remove_repository(repo.id);
                }
                ImGui::PopID();
            }
            
            ImGui::EndTable();
        }
    }
#endif
}

void PluginManagerUI::render_plugin_config_editor() {
#ifdef ECSCOPE_HAS_IMGUI
    if (selected_plugin_id_.empty()) return;
    
    auto it = std::find_if(installed_plugins_.begin(), installed_plugins_.end(),
                          [this](const PluginMetadata& p) { return p.id == selected_plugin_id_; });
    
    if (it != installed_plugins_.end()) {
        config_editor_->render(*it);
    }
#endif
}

void PluginManagerUI::update(float delta_time) {
    update_installation_jobs();
    update_plugin_states();
    
    if (browser_) {
        browser_->update();
    }
}

void PluginManagerUI::update_installation_jobs() {
    if (installer_) {
        installer_->update_installation_queue();
    }
}

void PluginManagerUI::update_plugin_states() {
    std::lock_guard<std::mutex> lock(plugins_mutex_);
    
    for (auto& plugin : installed_plugins_) {
        // Update plugin state based on loader status
        if (loader_ && loader_->is_plugin_loaded(plugin.id)) {
            if (plugin.state != PluginState::Loaded) {
                plugin.state = PluginState::Loaded;
                plugin.last_loaded = std::chrono::system_clock::now();
                plugin.load_count++;
            }
        }
    }
}

void PluginManagerUI::scan_plugins_directory() {
    if (!std::filesystem::exists(plugins_directory_)) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(plugins_mutex_);
    installed_plugins_.clear();
    
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(plugins_directory_)) {
            if (entry.is_regular_file()) {
                std::string extension = entry.path().extension().string();
                std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
                
                if (extension == ".dll" || extension == ".so" || extension == ".dylib" || extension == ".plugin") {
                    PluginMetadata metadata;
                    load_plugin_metadata(entry.path().string(), metadata);
                    
                    if (!metadata.id.empty()) {
                        installed_plugins_.push_back(metadata);
                    }
                }
            }
        }
        
        // Check dependencies for all plugins
        for (auto& plugin : installed_plugins_) {
            check_plugin_dependencies(plugin);
        }
        
        // Resolve dependency conflicts
        resolve_dependency_conflicts();
        
    } catch (const std::exception& e) {
        // Handle filesystem errors
    }
}

void PluginManagerUI::load_plugin_metadata(const std::string& plugin_path, PluginMetadata& metadata) {
    metadata.file_path = plugin_path;
    metadata.id = std::filesystem::path(plugin_path).stem().string();
    metadata.name = metadata.id; // Default name
    metadata.description = "Plugin loaded from " + plugin_path;
    metadata.author = "Unknown";
    metadata.license = "Unknown";
    metadata.version = {1, 0, 0, ""};
    metadata.type = PluginType::Custom;
    metadata.state = PluginState::NotLoaded;
    metadata.installation_status = InstallationStatus::Installed;
    
    try {
        metadata.file_size = std::filesystem::file_size(plugin_path);
        auto file_time = std::filesystem::last_write_time(plugin_path);
        metadata.install_date = std::chrono::system_clock::from_time_t(
            std::chrono::system_clock::to_time_t(
                std::chrono::file_clock::to_sys(file_time)));
    } catch (const std::exception&) {
        metadata.file_size = 0;
        metadata.install_date = std::chrono::system_clock::now();
    }
    
    metadata.load_count = 0;
    metadata.average_load_time_ms = 0.0f;
    metadata.is_essential = false;
    metadata.auto_load = false;
    
    // Try to load metadata from companion .json file
    std::string metadata_path = plugin_path + ".json";
    if (std::filesystem::exists(metadata_path)) {
        std::ifstream file(metadata_path);
        if (file.is_open()) {
            // Parse JSON metadata (simplified implementation)
            std::string line;
            while (std::getline(file, line)) {
                if (line.find("\"name\"") != std::string::npos) {
                    size_t start = line.find("\"", line.find(":") + 1) + 1;
                    size_t end = line.find("\"", start);
                    if (start != std::string::npos && end != std::string::npos) {
                        metadata.name = line.substr(start, end - start);
                    }
                }
                // Similar parsing for other fields...
            }
        }
    }
}

void PluginManagerUI::check_plugin_dependencies(PluginMetadata& plugin) {
    // Check if all dependencies are satisfied
    for (auto& dep : plugin.dependencies) {
        dep.is_satisfied = false;
        
        for (const auto& other_plugin : installed_plugins_) {
            if (other_plugin.id == dep.plugin_id || other_plugin.name == dep.plugin_name) {
                // Check version requirements
                if (other_plugin.version == dep.min_version || 
                    (dep.max_version.major > 0 && other_plugin.version < dep.max_version)) {
                    dep.is_satisfied = true;
                    break;
                }
            }
        }
    }
}

void PluginManagerUI::resolve_dependency_conflicts() {
    // Resolve conflicting dependencies
    for (auto& plugin : installed_plugins_) {
        for (const auto& dep : plugin.dependencies) {
            if (dep.type == DependencyType::Conflicting && dep.is_satisfied) {
                // Handle conflicting dependency
                plugin.state = PluginState::Failed;
            }
        }
    }
}

void PluginManagerUI::refresh_plugin_list() {
    scan_plugins_directory();
}

bool PluginManagerUI::load_plugin(const std::string& plugin_id) {
    if (loader_) {
        bool success = loader_->load_plugin(plugin_id);
        if (success && plugin_loaded_callback_) {
            plugin_loaded_callback_(plugin_id, true);
        }
        return success;
    }
    return false;
}

bool PluginManagerUI::unload_plugin(const std::string& plugin_id) {
    if (loader_) {
        bool success = loader_->unload_plugin(plugin_id);
        if (success && plugin_loaded_callback_) {
            plugin_loaded_callback_(plugin_id, false);
        }
        return success;
    }
    return false;
}

void PluginManagerUI::enable_plugin(const std::string& plugin_id, bool enabled) {
    if (loader_) {
        loader_->enable_plugin(plugin_id, enabled);
    }
}

void PluginManagerUI::install_plugin(const std::string& plugin_id, const std::string& source_url) {
    if (installer_) {
        std::string job_id = installer_->install_plugin(plugin_id, source_url);
        if (!job_id.empty()) {
            show_installation_queue_ = true;
        }
    }
}

void PluginManagerUI::update_plugin(const std::string& plugin_id) {
    if (installer_) {
        std::string job_id = installer_->update_plugin(plugin_id);
        if (!job_id.empty()) {
            show_installation_queue_ = true;
        }
    }
}

void PluginManagerUI::uninstall_plugin(const std::string& plugin_id) {
    if (installer_) {
        std::string job_id = installer_->uninstall_plugin(plugin_id);
        if (!job_id.empty()) {
            show_installation_queue_ = true;
        }
    }
}

void PluginManagerUI::edit_plugin_config(const std::string& plugin_id) {
    if (config_editor_) {
        config_editor_->load_plugin_config(plugin_id);
    }
}

void PluginManagerUI::save_plugin_configs() {
    // Save all plugin configurations
    std::string config_file = plugins_directory_ + "/plugins_config.json";
    std::ofstream file(config_file);
    if (file.is_open()) {
        file << "{\n";
        // Write plugin configurations in JSON format
        file << "}\n";
    }
}

void PluginManagerUI::load_plugin_configs() {
    // Load plugin configurations
    std::string config_file = plugins_directory_ + "/plugins_config.json";
    if (std::filesystem::exists(config_file)) {
        std::ifstream file(config_file);
        if (file.is_open()) {
            // Parse JSON configuration
        }
    }
}

void PluginManagerUI::add_plugin_repository(const std::string& name, const std::string& url) {
    if (browser_) {
        browser_->add_repository(name, url);
    }
}

void PluginManagerUI::refresh_plugin_repositories() {
    if (browser_) {
        browser_->refresh_repositories();
    }
}

void PluginManagerUI::shutdown() {
    PluginManagerSystem::instance().unregister_plugin_manager_ui(this);
    
    if (installer_) {
        installer_->shutdown();
    }
    
    if (loader_) {
        loader_->shutdown();
    }
}

// Placeholder implementations for component classes
void PluginInstaller::initialize() { next_job_id_ = 1; }
void PluginInstaller::shutdown() {}
std::string PluginInstaller::install_plugin(const std::string&, const std::string&) { return ""; }
std::string PluginInstaller::update_plugin(const std::string&) { return ""; }
std::string PluginInstaller::uninstall_plugin(const std::string&) { return ""; }
void PluginInstaller::cancel_installation(const std::string&) {}
InstallationJob PluginInstaller::get_installation_status(const std::string&) { return {}; }
std::vector<InstallationJob> PluginInstaller::get_active_jobs() { return {}; }
void PluginInstaller::update_installation_queue() {}
bool PluginInstaller::download_plugin(const std::string&, const std::string&, InstallationJob&) { return false; }
bool PluginInstaller::verify_plugin_integrity(const std::string&) { return false; }
bool PluginInstaller::extract_plugin(const std::string&, const std::string&) { return false; }
bool PluginInstaller::install_plugin_files(const std::string&, const std::string&) { return false; }

void PluginLoader::initialize() {}
void PluginLoader::shutdown() {}
bool PluginLoader::load_plugin(const std::string& plugin_id) {
    std::lock_guard<std::mutex> lock(loader_mutex_);
    if (loaded_plugins_.find(plugin_id) != loaded_plugins_.end()) {
        return true; // Already loaded
    }
    
    // Mock loading
    LoadedPlugin plugin;
    plugin.handle = reinterpret_cast<void*>(0x12345678); // Mock handle
    plugin.load_time = std::chrono::steady_clock::now();
    plugin.load_duration_ms = 50.0f; // Mock load time
    
    loaded_plugins_[plugin_id] = plugin;
    return true;
}

bool PluginLoader::unload_plugin(const std::string& plugin_id) {
    std::lock_guard<std::mutex> lock(loader_mutex_);
    auto it = loaded_plugins_.find(plugin_id);
    if (it != loaded_plugins_.end()) {
        loaded_plugins_.erase(it);
        return true;
    }
    return false;
}

bool PluginLoader::reload_plugin(const std::string& plugin_id) {
    unload_plugin(plugin_id);
    return load_plugin(plugin_id);
}

void PluginLoader::enable_plugin(const std::string&, bool) {}
bool PluginLoader::is_plugin_loaded(const std::string& plugin_id) const {
    std::lock_guard<std::mutex> lock(loader_mutex_);
    return loaded_plugins_.find(plugin_id) != loaded_plugins_.end();
}

std::vector<std::string> PluginLoader::get_loaded_plugins() const {
    std::lock_guard<std::mutex> lock(loader_mutex_);
    std::vector<std::string> result;
    result.reserve(loaded_plugins_.size());
    for (const auto& [id, plugin] : loaded_plugins_) {
        result.push_back(id);
    }
    return result;
}

float PluginLoader::get_plugin_load_time(const std::string& plugin_id) const {
    std::lock_guard<std::mutex> lock(loader_mutex_);
    auto it = loaded_plugins_.find(plugin_id);
    return (it != loaded_plugins_.end()) ? it->second.load_duration_ms : 0.0f;
}

void* PluginLoader::load_dynamic_library(const std::string&) { return nullptr; }
void PluginLoader::unload_dynamic_library(void*) {}
bool PluginLoader::validate_plugin_interface(void*) { return false; }

void PluginConfigEditor::initialize() {}
void PluginConfigEditor::render(PluginMetadata& plugin) {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::CollapsingHeader("Plugin Configuration", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (plugin.settings.empty()) {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No configuration options available");
        } else {
            for (auto& [key, value] : plugin.settings) {
                ImGui::PushID(key.c_str());
                render_config_item(key, value);
                ImGui::PopID();
            }
            
            ImGui::Separator();
            
            if (ImGui::Button("Save Configuration")) {
                save_plugin_config(plugin.id);
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset to Defaults")) {
                reset_to_defaults(plugin.id);
            }
        }
    }
#endif
}

void PluginConfigEditor::load_plugin_config(const std::string&) {}
void PluginConfigEditor::save_plugin_config(const std::string&) {}
void PluginConfigEditor::reset_to_defaults(const std::string&) {}
void PluginConfigEditor::set_config_value(const std::string&, const std::string&, const std::string&) {}
std::string PluginConfigEditor::get_config_value(const std::string&, const std::string&) const { return ""; }
void PluginConfigEditor::render_config_section(const std::string&, std::unordered_map<std::string, std::string>&) {}
void PluginConfigEditor::render_config_item(const std::string& key, std::string& value) {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Text("%s:", key.c_str());
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1);
    
    char buffer[256];
    std::strncpy(buffer, value.c_str(), sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    if (ImGui::InputText(("##" + key).c_str(), buffer, sizeof(buffer))) {
        value = buffer;
    }
#endif
}
bool PluginConfigEditor::is_boolean_setting(const std::string&) const { return false; }
bool PluginConfigEditor::is_numeric_setting(const std::string&) const { return false; }
bool PluginConfigEditor::is_file_path_setting(const std::string&) const { return false; }

void PluginBrowser::initialize() { filter_type_ = PluginType::Core; search_query_.resize(256); }
void PluginBrowser::render() {
#ifdef ECSCOPE_HAS_IMGUI
    render_search_filters();
    ImGui::Separator();
    
    ImGui::Columns(2, "BrowserColumns", true);
    render_repository_list();
    
    ImGui::NextColumn();
    render_plugin_grid();
    
    ImGui::Columns(1);
#endif
}
void PluginBrowser::update() {}
void PluginBrowser::add_repository(const std::string& name, const std::string& url) {
    PluginRepository repo;
    repo.id = "repo_" + std::to_string(repositories_.size());
    repo.name = name;
    repo.url = url;
    repo.description = "Plugin repository";
    repo.is_enabled = true;
    repo.is_trusted = false;
    repo.last_updated = std::chrono::system_clock::now();
    
    repositories_.push_back(repo);
}
void PluginBrowser::remove_repository(const std::string& id) {
    repositories_.erase(
        std::remove_if(repositories_.begin(), repositories_.end(),
                      [&id](const PluginRepository& repo) { return repo.id == id; }),
        repositories_.end());
}
void PluginBrowser::refresh_repositories() {}
std::vector<PluginRepository> PluginBrowser::get_repositories() const { return repositories_; }
std::vector<PluginMetadata> PluginBrowser::search_plugins(const std::string&) const { return {}; }
std::vector<PluginMetadata> PluginBrowser::filter_plugins(PluginType) const { return {}; }
void PluginBrowser::render_repository_list() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Text("Repositories:");
    for (const auto& repo : repositories_) {
        ImVec4 color = repo.is_enabled ? ImVec4(0.8f, 0.8f, 0.8f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
        ImGui::TextColored(color, "• %s", repo.name.c_str());
    }
#endif
}
void PluginBrowser::render_plugin_grid() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Text("Available Plugins:");
    if (available_plugins_.empty()) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No plugins available");
    } else {
        for (const auto& plugin : available_plugins_) {
            if (ImGui::Selectable(plugin.name.c_str())) {
                // Select plugin for details
            }
        }
    }
#endif
}
void PluginBrowser::render_plugin_details(const PluginMetadata&) {}
void PluginBrowser::render_search_filters() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Text("Search:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200);
    ImGui::InputText("##Search", search_query_.data(), search_query_.capacity());
    
    ImGui::SameLine();
    const char* type_names[] = {"Core", "Rendering", "Audio", "Physics", "Networking", "Scripting", "Tools", "Custom"};
    int current_type = static_cast<int>(filter_type_);
    ImGui::SetNextItemWidth(120);
    if (ImGui::Combo("##TypeFilter", &current_type, type_names, IM_ARRAYSIZE(type_names))) {
        filter_type_ = static_cast<PluginType>(current_type);
    }
#endif
}
bool PluginBrowser::fetch_repository_data(PluginRepository&) { return false; }
void PluginBrowser::parse_plugin_manifest(const std::string&, std::vector<PluginMetadata>&) {}

// PluginManagerSystem implementation
void PluginManagerSystem::initialize(const std::string& plugins_directory) {
    plugins_directory_ = plugins_directory;
}

void PluginManagerSystem::shutdown() {
    std::lock_guard<std::mutex> lock(ui_mutex_);
    registered_uis_.clear();
    
    std::lock_guard<std::mutex> lock2(interfaces_mutex_);
    plugin_interfaces_.clear();
    plugin_hooks_.clear();
}

void PluginManagerSystem::register_plugin_manager_ui(PluginManagerUI* ui) {
    std::lock_guard<std::mutex> lock(ui_mutex_);
    registered_uis_.push_back(ui);
}

void PluginManagerSystem::unregister_plugin_manager_ui(PluginManagerUI* ui) {
    std::lock_guard<std::mutex> lock(ui_mutex_);
    registered_uis_.erase(
        std::remove(registered_uis_.begin(), registered_uis_.end(), ui),
        registered_uis_.end());
}

void PluginManagerSystem::notify_plugin_state_changed(const std::string&, PluginState) {}
void PluginManagerSystem::notify_installation_progress(const std::string&, float) {}

bool PluginManagerSystem::register_plugin_interface(const std::string& interface_name, void* interface_ptr) {
    std::lock_guard<std::mutex> lock(interfaces_mutex_);
    plugin_interfaces_[interface_name] = interface_ptr;
    return true;
}

void* PluginManagerSystem::get_plugin_interface(const std::string& interface_name) {
    std::lock_guard<std::mutex> lock(interfaces_mutex_);
    auto it = plugin_interfaces_.find(interface_name);
    return (it != plugin_interfaces_.end()) ? it->second : nullptr;
}

void PluginManagerSystem::register_plugin_hook(const std::string& hook_name, std::function<void()> callback) {
    std::lock_guard<std::mutex> lock(interfaces_mutex_);
    plugin_hooks_[hook_name].push_back(callback);
}

void PluginManagerSystem::trigger_plugin_hook(const std::string& hook_name) {
    std::lock_guard<std::mutex> lock(interfaces_mutex_);
    auto it = plugin_hooks_.find(hook_name);
    if (it != plugin_hooks_.end()) {
        for (const auto& callback : it->second) {
            callback();
        }
    }
}

void PluginManagerSystem::update(float delta_time) {
    // Update system state
}

}} // namespace ecscope::gui