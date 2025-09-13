/**
 * @file dashboard.cpp
 * @brief ECScope Engine Main Dashboard Implementation
 * 
 * Professional dashboard implementation with Dear ImGui integration,
 * featuring docking, themes, navigation, and system monitoring.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#include "../../include/ecscope/gui/dashboard.hpp"

#ifdef ECSCOPE_HAS_IMGUI
#include <imgui.h>
#include <imgui_internal.h>
#endif

#include <algorithm>
#include <chrono>
#include <fstream>
#include <sstream>
#include <cmath>
#include <numeric>

namespace ecscope::gui {

// =============================================================================
// CONSTANTS & STATIC DATA
// =============================================================================

namespace {
    // Color constants for professional theme
    constexpr ImU32 COLOR_PRIMARY = IM_COL32(100, 180, 255, 255);     // Blue
    constexpr ImU32 COLOR_SUCCESS = IM_COL32(76, 175, 80, 255);       // Green
    constexpr ImU32 COLOR_WARNING = IM_COL32(255, 193, 7, 255);       // Amber
    constexpr ImU32 COLOR_ERROR = IM_COL32(244, 67, 54, 255);         // Red
    constexpr ImU32 COLOR_BACKGROUND_DARK = IM_COL32(25, 25, 25, 255);
    constexpr ImU32 COLOR_SURFACE_DARK = IM_COL32(35, 35, 35, 255);
    constexpr ImU32 COLOR_TEXT_PRIMARY = IM_COL32(255, 255, 255, 255);
    constexpr ImU32 COLOR_TEXT_SECONDARY = IM_COL32(160, 160, 160, 255);
    
    // Category colors
    const std::unordered_map<FeatureCategory, ImU32> CATEGORY_COLORS = {
        {FeatureCategory::Core, IM_COL32(100, 180, 255, 255)},
        {FeatureCategory::Rendering, IM_COL32(156, 39, 176, 255)},
        {FeatureCategory::Physics, IM_COL32(255, 152, 0, 255)},
        {FeatureCategory::Audio, IM_COL32(76, 175, 80, 255)},
        {FeatureCategory::Networking, IM_COL32(3, 169, 244, 255)},
        {FeatureCategory::Tools, IM_COL32(96, 125, 139, 255)},
        {FeatureCategory::Debugging, IM_COL32(244, 67, 54, 255)},
        {FeatureCategory::Performance, IM_COL32(255, 193, 7, 255)}
    };
    
    // Font Awesome icons (using Unicode)
    const char* ICON_DASHBOARD = "\uf0e4";
    const char* ICON_COG = "\uf013";
    const char* ICON_SEARCH = "\uf002";
    const char* ICON_STAR = "\uf005";
    const char* ICON_HEART = "\uf004";
    const char* ICON_CUBE = "\uf1b2";
    const char* ICON_ROCKET = "\uf135";
    const char* ICON_CHART = "\uf080";
    const char* ICON_BUG = "\uf188";
    const char* ICON_WRENCH = "\uf0ad";
    const char* ICON_PLAY = "\uf04b";
    const char* ICON_STOP = "\uf04d";
    const char* ICON_REFRESH = "\uf021";
    
} // anonymous namespace

// =============================================================================
// DASHBOARD IMPLEMENTATION
// =============================================================================

Dashboard::Dashboard() 
    : last_update_time_(std::chrono::steady_clock::now()) {
    
    // Initialize panel visibility
    panel_visibility_[PanelType::Welcome] = true;
    panel_visibility_[PanelType::FeatureGallery] = true;
    panel_visibility_[PanelType::SystemStatus] = true;
    panel_visibility_[PanelType::Performance] = false;
    panel_visibility_[PanelType::LogOutput] = false;
    panel_visibility_[PanelType::Properties] = false;
    panel_visibility_[PanelType::Explorer] = false;
    panel_visibility_[PanelType::Viewport] = false;
    panel_visibility_[PanelType::Tools] = false;
    panel_visibility_[PanelType::Settings] = false;
}

Dashboard::~Dashboard() {
    shutdown();
}

bool Dashboard::initialize(rendering::IRenderer* renderer) {
    if (initialized_) {
        return true;
    }

#ifndef ECSCOPE_HAS_IMGUI
    core::Log::error("Dashboard: ImGui not available in build configuration");
    return false;
#endif

    renderer_ = renderer;
    
    // Initialize default features
    initialize_default_features();
    
    // Setup theme
    set_theme(DashboardTheme::Dark);
    
    // Initialize docking (disabled - not available in this ImGui version)
#ifdef ECSCOPE_HAS_IMGUI
    // ImGuiIO& io = ImGui::GetIO();
    // Docking not available in this ImGui version
    // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif
    
    initialized_ = true;
    core::Log::info("Dashboard: Initialized successfully");
    return true;
}

void Dashboard::shutdown() {
    if (!initialized_) {
        return;
    }
    
    // Save configuration before shutdown
    save_config(config_filepath_);
    
    initialized_ = false;
    core::Log::info("Dashboard: Shut down successfully");
}

// =============================================================================
// MAIN RENDER LOOP
// =============================================================================

void Dashboard::render() {
    if (!initialized_) {
        return;
    }

#ifdef ECSCOPE_HAS_IMGUI
    // Update timing
    auto now = std::chrono::steady_clock::now();
    auto delta = std::chrono::duration<float>(now - last_update_time_).count();
    last_update_time_ = now;
    
    // Update dashboard state
    update(delta);
    
    // Render main menu bar
    render_main_menu_bar();
    
    // Note: Dockspace disabled - not available in this ImGui version
    
    // Render panels based on visibility
    if (panel_visibility_[PanelType::Welcome]) {
        render_welcome_panel();
    }
    
    if (panel_visibility_[PanelType::FeatureGallery]) {
        render_feature_gallery_panel();
    }
    
    if (panel_visibility_[PanelType::SystemStatus]) {
        render_system_status_panel();
    }
    
    if (panel_visibility_[PanelType::Performance]) {
        render_performance_panel();
    }
    
    if (panel_visibility_[PanelType::LogOutput]) {
        render_log_output_panel();
    }
    
    if (panel_visibility_[PanelType::Properties]) {
        render_properties_panel();
    }
    
    if (panel_visibility_[PanelType::Explorer]) {
        render_explorer_panel();
    }
    
    if (panel_visibility_[PanelType::Viewport] && renderer_) {
        render_viewport_panel();
    }
    
    if (panel_visibility_[PanelType::Tools]) {
        render_tools_panel();
    }
    
    if (panel_visibility_[PanelType::Settings]) {
        render_settings_panel();
    }
    
    // Show demo windows if requested
    if (show_demo_window_) {
        ImGui::ShowDemoWindow(&show_demo_window_);
    }
    
    if (show_style_editor_) {
        ImGui::Begin("Style Editor", &show_style_editor_);
        ImGui::ShowStyleEditor();
        ImGui::End();
    }
#endif
}

void Dashboard::update(float delta_time) {
    // Update system monitors
    update_system_monitors();
    
    // Update performance metrics history
    if (metrics_history_.size() >= MAX_METRICS_HISTORY) {
        metrics_history_.erase(metrics_history_.begin());
    }
    metrics_history_.push_back(current_metrics_);
}

// =============================================================================
// THEME & STYLING
// =============================================================================

void Dashboard::set_theme(DashboardTheme theme) {
    current_theme_ = theme;
    
#ifdef ECSCOPE_HAS_IMGUI
    switch (theme) {
        case DashboardTheme::Dark:
            setup_professional_dark_theme();
            break;
        case DashboardTheme::Light:
            setup_clean_light_theme();
            break;
        case DashboardTheme::HighContrast:
            setup_high_contrast_theme();
            break;
        case DashboardTheme::Custom:
            // Keep current style
            break;
    }
#endif
}

void Dashboard::apply_custom_style(const ImGuiStyle& style) {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::GetStyle() = style;
    current_theme_ = DashboardTheme::Custom;
#endif
}

// =============================================================================
// FEATURE MANAGEMENT
// =============================================================================

void Dashboard::register_feature(const FeatureInfo& feature) {
    // Check for duplicate IDs
    auto it = std::find_if(features_.begin(), features_.end(),
        [&feature](const FeatureInfo& f) { return f.id == feature.id; });
    
    if (it != features_.end()) {
        core::Log::warning("Dashboard: Feature '{}' already registered, updating", feature.id);
        *it = feature;
    } else {
        features_.push_back(feature);
        core::Log::info("Dashboard: Registered feature '{}'", feature.name);
    }
}

void Dashboard::unregister_feature(const std::string& feature_id) {
    auto it = std::remove_if(features_.begin(), features_.end(),
        [&feature_id](const FeatureInfo& f) { return f.id == feature_id; });
    
    if (it != features_.end()) {
        features_.erase(it, features_.end());
        core::Log::info("Dashboard: Unregistered feature '{}'", feature_id);
    }
}

std::vector<FeatureInfo> Dashboard::get_features_by_category(FeatureCategory category) const {
    std::vector<FeatureInfo> result;
    std::copy_if(features_.begin(), features_.end(), std::back_inserter(result),
        [category](const FeatureInfo& f) { return f.category == category; });
    return result;
}

bool Dashboard::launch_feature(const std::string& feature_id) {
    auto it = std::find_if(features_.begin(), features_.end(),
        [&feature_id](const FeatureInfo& f) { return f.id == feature_id; });
    
    if (it != features_.end() && it->enabled && it->launch_callback) {
        core::Log::info("Dashboard: Launching feature '{}'", it->name);
        it->launch_callback();
        return true;
    }
    
    core::Log::warning("Dashboard: Failed to launch feature '{}'", feature_id);
    return false;
}

void Dashboard::toggle_favorite(const std::string& feature_id) {
    auto it = std::find_if(features_.begin(), features_.end(),
        [&feature_id](const FeatureInfo& f) { return f.id == feature_id; });
    
    if (it != features_.end()) {
        it->favorite = !it->favorite;
    }
}

// =============================================================================
// WORKSPACE MANAGEMENT
// =============================================================================

void Dashboard::apply_workspace_preset(WorkspacePreset preset) {
    current_workspace_ = preset;
    
    // Reset all panels to default visibility
    for (auto& [panel_type, visible] : panel_visibility_) {
        visible = false;
    }
    
    switch (preset) {
        case WorkspacePreset::Overview:
            panel_visibility_[PanelType::Welcome] = true;
            panel_visibility_[PanelType::FeatureGallery] = true;
            panel_visibility_[PanelType::SystemStatus] = true;
            break;
            
        case WorkspacePreset::Development:
            panel_visibility_[PanelType::FeatureGallery] = true;
            panel_visibility_[PanelType::Explorer] = true;
            panel_visibility_[PanelType::Properties] = true;
            panel_visibility_[PanelType::LogOutput] = true;
            break;
            
        case WorkspacePreset::Debugging:
            panel_visibility_[PanelType::SystemStatus] = true;
            panel_visibility_[PanelType::Performance] = true;
            panel_visibility_[PanelType::LogOutput] = true;
            panel_visibility_[PanelType::Tools] = true;
            break;
            
        case WorkspacePreset::Performance:
            panel_visibility_[PanelType::Performance] = true;
            panel_visibility_[PanelType::SystemStatus] = true;
            break;
            
        case WorkspacePreset::ContentCreation:
            panel_visibility_[PanelType::Viewport] = true;
            panel_visibility_[PanelType::Explorer] = true;
            panel_visibility_[PanelType::Properties] = true;
            panel_visibility_[PanelType::Tools] = true;
            break;
            
        case WorkspacePreset::Testing:
            panel_visibility_[PanelType::FeatureGallery] = true;
            panel_visibility_[PanelType::SystemStatus] = true;
            panel_visibility_[PanelType::LogOutput] = true;
            break;
            
        case WorkspacePreset::Custom:
            // Keep current visibility settings
            break;
    }
}

// =============================================================================
// PRIVATE RENDERING METHODS
// =============================================================================

void Dashboard::render_main_menu_bar() {
#ifdef ECSCOPE_HAS_IMGUI
    if (ImGui::BeginMainMenuBar()) {
        // File menu
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Project", "Ctrl+N")) {
                // TODO: Implement new project
            }
            if (ImGui::MenuItem("Open Project", "Ctrl+O")) {
                // TODO: Implement open project
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Save Layout", "Ctrl+S")) {
                save_layout_to_ini();
            }
            if (ImGui::MenuItem("Load Layout", "Ctrl+L")) {
                load_layout_from_ini();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                // TODO: Request application exit
            }
            ImGui::EndMenu();
        }
        
        // View menu
        if (ImGui::BeginMenu("View")) {
            // Workspace presets
            if (ImGui::BeginMenu("Workspace")) {
                if (ImGui::MenuItem("Overview", nullptr, current_workspace_ == WorkspacePreset::Overview)) {
                    apply_workspace_preset(WorkspacePreset::Overview);
                }
                if (ImGui::MenuItem("Development", nullptr, current_workspace_ == WorkspacePreset::Development)) {
                    apply_workspace_preset(WorkspacePreset::Development);
                }
                if (ImGui::MenuItem("Debugging", nullptr, current_workspace_ == WorkspacePreset::Debugging)) {
                    apply_workspace_preset(WorkspacePreset::Debugging);
                }
                if (ImGui::MenuItem("Performance", nullptr, current_workspace_ == WorkspacePreset::Performance)) {
                    apply_workspace_preset(WorkspacePreset::Performance);
                }
                if (ImGui::MenuItem("Content Creation", nullptr, current_workspace_ == WorkspacePreset::ContentCreation)) {
                    apply_workspace_preset(WorkspacePreset::ContentCreation);
                }
                if (ImGui::MenuItem("Testing", nullptr, current_workspace_ == WorkspacePreset::Testing)) {
                    apply_workspace_preset(WorkspacePreset::Testing);
                }
                ImGui::EndMenu();
            }
            
            ImGui::Separator();
            
            // Panel toggles
            if (ImGui::BeginMenu("Panels")) {
                ImGui::MenuItem("Welcome", nullptr, &panel_visibility_[PanelType::Welcome]);
                ImGui::MenuItem("Feature Gallery", nullptr, &panel_visibility_[PanelType::FeatureGallery]);
                ImGui::MenuItem("System Status", nullptr, &panel_visibility_[PanelType::SystemStatus]);
                ImGui::MenuItem("Performance", nullptr, &panel_visibility_[PanelType::Performance]);
                ImGui::MenuItem("Log Output", nullptr, &panel_visibility_[PanelType::LogOutput]);
                ImGui::MenuItem("Properties", nullptr, &panel_visibility_[PanelType::Properties]);
                ImGui::MenuItem("Explorer", nullptr, &panel_visibility_[PanelType::Explorer]);
                if (renderer_) {
                    ImGui::MenuItem("Viewport", nullptr, &panel_visibility_[PanelType::Viewport]);
                }
                ImGui::MenuItem("Tools", nullptr, &panel_visibility_[PanelType::Tools]);
                ImGui::MenuItem("Settings", nullptr, &panel_visibility_[PanelType::Settings]);
                ImGui::EndMenu();
            }
            
            ImGui::Separator();
            
            // Theme selection
            if (ImGui::BeginMenu("Theme")) {
                if (ImGui::MenuItem("Dark", nullptr, current_theme_ == DashboardTheme::Dark)) {
                    set_theme(DashboardTheme::Dark);
                }
                if (ImGui::MenuItem("Light", nullptr, current_theme_ == DashboardTheme::Light)) {
                    set_theme(DashboardTheme::Light);
                }
                if (ImGui::MenuItem("High Contrast", nullptr, current_theme_ == DashboardTheme::HighContrast)) {
                    set_theme(DashboardTheme::HighContrast);
                }
                ImGui::EndMenu();
            }
            
            ImGui::EndMenu();
        }
        
        // Tools menu
        if (ImGui::BeginMenu("Tools")) {
            ImGui::MenuItem("Style Editor", nullptr, &show_style_editor_);
            ImGui::MenuItem("ImGui Demo", nullptr, &show_demo_window_);
            ImGui::Separator();
            if (ImGui::MenuItem("Reset Layout")) {
                setup_default_layout();
            }
            ImGui::EndMenu();
        }
        
        // Help menu
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About ECScope")) {
                // TODO: Show about dialog
            }
            if (ImGui::MenuItem("Documentation")) {
                // TODO: Open documentation
            }
            ImGui::EndMenu();
        }
        
        // Search bar on the right side
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 250);
        render_search_bar();
        
        ImGui::EndMainMenuBar();
    }
#endif
}

void Dashboard::render_main_dockspace() {
    // Dockspace not available in this ImGui version - simplified to regular windows
    // No-op function for now
}

void Dashboard::render_welcome_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Begin("Welcome to ECScope", &panel_visibility_[PanelType::Welcome]);
    
    // Header with logo and title
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]); // Assume first font is larger
    ImGui::Text("ECScope Engine Dashboard");
    ImGui::PopFont();
    
    ImGui::Text("Version 1.0.0");
    ImGui::Separator();
    
    // Quick start section
    ImGui::Text("Quick Start:");
    
    if (ImGui::Button("Create New Project", ImVec2(200, 40))) {
        // TODO: Launch project creation wizard
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Open Existing Project", ImVec2(200, 40))) {
        // TODO: Launch project browser
    }
    
    ImGui::Spacing();
    
    // Recent projects
    ImGui::Text("Recent Projects:");
    ImGui::Separator();
    
    // Placeholder for recent projects list
    for (int i = 0; i < 3; ++i) {
        if (ImGui::Selectable(("Recent Project " + std::to_string(i + 1)).c_str())) {
            // TODO: Open recent project
        }
    }
    
    ImGui::Spacing();
    
    // Feature highlights
    ImGui::Text("Featured Systems:");
    ImGui::Separator();
    
    // Display some key features with status
    const char* featured_systems[] = {
        "ECS Architecture",
        "Modern Rendering",
        "Physics Simulation", 
        "Audio System",
        "Networking"
    };
    
    for (const char* system : featured_systems) {
        draw_status_led(true, ImVec2(10, 10));
        ImGui::SameLine();
        ImGui::Text("%s", system);
    }
    
    ImGui::End();
#endif
}

void Dashboard::render_feature_gallery_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Begin("Feature Gallery", &panel_visibility_[PanelType::FeatureGallery]);
    
    // Category filter tabs
    static FeatureCategory selected_category = FeatureCategory::Core;
    
    if (ImGui::BeginTabBar("CategoryTabs")) {
        for (int cat = 0; cat < 8; ++cat) {
            FeatureCategory category = static_cast<FeatureCategory>(cat);
            std::string label = category_to_string(category);
            
            if (ImGui::BeginTabItem(label.c_str())) {
                selected_category = category;
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }
    
    // Render features for selected category
    render_category_section(selected_category);
    
    ImGui::End();
#endif
}

void Dashboard::render_system_status_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Begin("System Status", &panel_visibility_[PanelType::SystemStatus]);
    
    // Overall system health indicator
    bool overall_healthy = true;
    for (const auto& [name, status] : system_status_) {
        if (!status.healthy) {
            overall_healthy = false;
            break;
        }
    }
    
    // System health header
    draw_status_led(overall_healthy, ImVec2(16, 16));
    ImGui::SameLine();
    ImGui::Text("Overall System Status: %s", overall_healthy ? "Healthy" : "Issues Detected");
    
    ImGui::Separator();
    
    // Individual system status
    for (const auto& [name, status] : system_status_) {
        render_system_health_indicator(status);
    }
    
    // Quick actions
    ImGui::Spacing();
    ImGui::Text("Quick Actions:");
    ImGui::Separator();
    
    if (ImGui::Button("Refresh All Systems")) {
        update_system_monitors();
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Run Diagnostics")) {
        // TODO: Run system diagnostics
    }
    
    ImGui::End();
#endif
}

void Dashboard::render_performance_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Begin("Performance Monitor", &panel_visibility_[PanelType::Performance]);
    
    // Current metrics
    ImGui::Text("Current Performance:");
    ImGui::Separator();
    
    ImGui::Text("Frame Rate: %.1f FPS", current_metrics_.frame_rate);
    ImGui::Text("Frame Time: %.2f ms", current_metrics_.frame_time_ms);
    ImGui::Text("CPU Usage: %.1f%%", current_metrics_.cpu_usage);
    ImGui::Text("Memory: %s", format_memory_size(current_metrics_.memory_usage).c_str());
    
    if (renderer_) {
        ImGui::Text("GPU Memory: %s", format_memory_size(current_metrics_.gpu_memory_usage).c_str());
        ImGui::Text("Draw Calls: %u", current_metrics_.draw_calls);
        ImGui::Text("Vertices: %u", current_metrics_.vertices_rendered);
    }
    
    ImGui::Spacing();
    
    // Performance graphs
    render_performance_graph();
    
    ImGui::Spacing();
    
    // Memory usage chart
    render_memory_usage_chart();
    
    ImGui::End();
#endif
}

void Dashboard::render_log_output_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Begin("Log Output", &panel_visibility_[PanelType::LogOutput]);
    
    // Log level filter
    static bool show_info = true;
    static bool show_warning = true;
    static bool show_error = true;
    static bool show_debug = false;
    
    ImGui::Checkbox("Info", &show_info); ImGui::SameLine();
    ImGui::Checkbox("Warning", &show_warning); ImGui::SameLine();
    ImGui::Checkbox("Error", &show_error); ImGui::SameLine();
    ImGui::Checkbox("Debug", &show_debug);
    
    ImGui::Separator();
    
    // Log output area
    if (ImGui::BeginChild("LogScrollArea")) {
        // TODO: Display actual log entries from the logging system
        ImGui::Text("[INFO] Dashboard initialized successfully");
        ImGui::Text("[INFO] All systems operational");
        ImGui::Text("[WARNING] GPU memory usage high");
        ImGui::Text("[INFO] Feature gallery loaded");
    }
    ImGui::EndChild();
    
    ImGui::End();
#endif
}

void Dashboard::render_properties_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Begin("Properties", &panel_visibility_[PanelType::Properties]);
    
    if (selected_feature_.has_value()) {
        auto it = std::find_if(features_.begin(), features_.end(),
            [this](const FeatureInfo& f) { return f.id == selected_feature_.value(); });
        
        if (it != features_.end()) {
            ImGui::Text("Feature: %s", it->name.c_str());
            ImGui::Text("Category: %s", category_to_string(it->category).c_str());
            ImGui::Text("Version: %s", it->version.c_str());
            ImGui::Separator();
            ImGui::TextWrapped("Description: %s", it->description.c_str());
            
            if (!it->dependencies.empty()) {
                ImGui::Text("Dependencies:");
                for (const auto& dep : it->dependencies) {
                    ImGui::BulletText("%s", dep.c_str());
                }
            }
        }
    } else {
        ImGui::Text("No feature selected");
        ImGui::Text("Click on a feature in the gallery to view its properties.");
    }
    
    ImGui::End();
#endif
}

void Dashboard::render_explorer_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Begin("Project Explorer", &panel_visibility_[PanelType::Explorer]);
    
    // Project tree view
    if (ImGui::TreeNode("Project")) {
        if (ImGui::TreeNode("Assets")) {
            ImGui::BulletText("Models");
            ImGui::BulletText("Textures");
            ImGui::BulletText("Shaders");
            ImGui::BulletText("Audio");
            ImGui::TreePop();
        }
        
        if (ImGui::TreeNode("Scenes")) {
            ImGui::BulletText("Main Scene");
            ImGui::BulletText("Test Scene");
            ImGui::TreePop();
        }
        
        if (ImGui::TreeNode("Scripts")) {
            ImGui::BulletText("GameLogic.cpp");
            ImGui::BulletText("PlayerController.cpp");
            ImGui::TreePop();
        }
        
        ImGui::TreePop();
    }
    
    ImGui::End();
#endif
}

void Dashboard::render_viewport_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Begin("3D Viewport", &panel_visibility_[PanelType::Viewport]);
    
    if (renderer_) {
        // Get available space for the viewport
        ImVec2 available = ImGui::GetContentRegionAvail();
        
        // Render viewport controls
        if (ImGui::Button("Reset Camera")) {
            // TODO: Reset camera to default position
        }
        ImGui::SameLine();
        
        static bool wireframe = false;
        ImGui::Checkbox("Wireframe", &wireframe);
        
        // Reserve space for the 3D view
        ImGui::Text("3D Viewport (%dx%d)", (int)available.x, (int)available.y);
        
        // TODO: Integrate with actual renderer
        // This would display the rendered output as a texture
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
        ImVec2 canvas_size = available;
        
        // Draw placeholder for 3D viewport
        draw_list->AddRectFilled(canvas_pos, 
                               ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
                               IM_COL32(64, 64, 64, 255));
        
        draw_list->AddRect(canvas_pos, 
                          ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
                          IM_COL32(128, 128, 128, 255));
        
        // Center text
        ImVec2 text_size = ImGui::CalcTextSize("3D Viewport");
        ImVec2 text_pos = ImVec2(canvas_pos.x + (canvas_size.x - text_size.x) * 0.5f,
                                canvas_pos.y + (canvas_size.y - text_size.y) * 0.5f);
        
        draw_list->AddText(text_pos, IM_COL32(160, 160, 160, 255), "3D Viewport");
        
    } else {
        ImGui::Text("No renderer available");
        ImGui::Text("Initialize a renderer to display 3D content.");
    }
    
    ImGui::End();
#endif
}

void Dashboard::render_tools_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Begin("Tools", &panel_visibility_[PanelType::Tools]);
    
    // Development tools
    ImGui::Text("Development Tools:");
    ImGui::Separator();
    
    if (ImGui::Button("Asset Importer")) {
        // TODO: Launch asset importer
    }
    
    if (ImGui::Button("Shader Editor")) {
        // TODO: Launch shader editor
    }
    
    if (ImGui::Button("Scene Editor")) {
        // TODO: Launch scene editor
    }
    
    ImGui::Spacing();
    
    // Debugging tools
    ImGui::Text("Debugging Tools:");
    ImGui::Separator();
    
    if (ImGui::Button("Memory Profiler")) {
        // TODO: Launch memory profiler
    }
    
    if (ImGui::Button("Performance Profiler")) {
        // TODO: Launch performance profiler
    }
    
    if (ImGui::Button("GPU Debugger")) {
        // TODO: Launch GPU debugger
    }
    
    ImGui::End();
#endif
}

void Dashboard::render_settings_panel() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::Begin("Settings", &panel_visibility_[PanelType::Settings]);
    
    // Theme settings
    if (ImGui::CollapsingHeader("Appearance")) {
        const char* themes[] = {"Dark", "Light", "High Contrast", "Custom"};
        int current_theme = static_cast<int>(current_theme_);
        
        if (ImGui::Combo("Theme", &current_theme, themes, 4)) {
            set_theme(static_cast<DashboardTheme>(current_theme));
        }
        
        static float ui_scale = 1.0f;
        if (ImGui::SliderFloat("UI Scale", &ui_scale, 0.5f, 2.0f)) {
            // TODO: Apply UI scaling
        }
    }
    
    // Performance settings
    if (ImGui::CollapsingHeader("Performance")) {
        static bool vsync = true;
        ImGui::Checkbox("VSync", &vsync);
        
        static int target_fps = 60;
        ImGui::SliderInt("Target FPS", &target_fps, 30, 144);
        
        static bool show_fps = true;
        ImGui::Checkbox("Show FPS Counter", &show_fps);
    }
    
    // Debug settings
    if (ImGui::CollapsingHeader("Debug")) {
        ImGui::Checkbox("Show Demo Window", &show_demo_window_);
        ImGui::Checkbox("Show Style Editor", &show_style_editor_);
        
        static bool debug_logging = false;
        ImGui::Checkbox("Debug Logging", &debug_logging);
    }
    
    ImGui::End();
#endif
}

// =============================================================================
// PRIVATE UTILITY METHODS
// =============================================================================

void Dashboard::initialize_default_features() {
    features_ = create_default_ecscope_features();
}

void Dashboard::update_system_monitors() {
    auto now = std::chrono::steady_clock::now();
    
    for (const auto& [name, callback] : system_monitors_) {
        SystemStatus status = callback();
        status.last_update = now;
        system_status_[name] = status;
    }
}

void Dashboard::render_search_bar() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui::PushItemWidth(200);
    
    char search_buffer[256];
    strncpy(search_buffer, search_query_.c_str(), sizeof(search_buffer) - 1);
    search_buffer[sizeof(search_buffer) - 1] = '\0';
    
    if (ImGui::InputTextWithHint("##search", "Search features...", search_buffer, sizeof(search_buffer))) {
        search_query_ = search_buffer;
    }
    
    ImGui::PopItemWidth();
#endif
}

void Dashboard::render_category_section(FeatureCategory category) {
#ifdef ECSCOPE_HAS_IMGUI
    auto category_features = get_features_by_category(category);
    
    if (category_features.empty()) {
        ImGui::Text("No features available in this category.");
        return;
    }
    
    // Calculate grid layout
    float window_width = ImGui::GetContentRegionAvail().x;
    const float card_width = 280.0f;
    const float card_spacing = 10.0f;
    int columns = std::max(1, (int)((window_width + card_spacing) / (card_width + card_spacing)));
    
    int column = 0;
    for (const auto& feature : category_features) {
        if (column > 0) {
            ImGui::SameLine();
        }
        
        render_feature_card(feature);
        
        column = (column + 1) % columns;
    }
#endif
}

void Dashboard::render_feature_card(const FeatureInfo& feature) {
#ifdef ECSCOPE_HAS_IMGUI
    const ImVec2 card_size(270, 180);
    const ImU32 card_color = get_category_color(feature.category);
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
    
    // Card background
    draw_list->AddRectFilled(cursor_pos, 
                           ImVec2(cursor_pos.x + card_size.x, cursor_pos.y + card_size.y),
                           IM_COL32(45, 45, 45, 255), 8.0f);
    
    // Card border
    draw_list->AddRect(cursor_pos, 
                      ImVec2(cursor_pos.x + card_size.x, cursor_pos.y + card_size.y),
                      card_color, 8.0f, 0, 2.0f);
    
    // Make card clickable
    ImGui::InvisibleButton(("card_" + feature.id).c_str(), card_size);
    bool is_hovered = ImGui::IsItemHovered();
    bool is_clicked = ImGui::IsItemClicked();
    
    // Hover effect
    if (is_hovered) {
        draw_list->AddRectFilled(cursor_pos, 
                               ImVec2(cursor_pos.x + card_size.x, cursor_pos.y + card_size.y),
                               IM_COL32(55, 55, 55, 255), 8.0f);
    }
    
    // Feature icon
    ImVec2 icon_pos = ImVec2(cursor_pos.x + 15, cursor_pos.y + 15);
    draw_list->AddCircleFilled(ImVec2(icon_pos.x + 20, icon_pos.y + 20), 20, card_color);
    
    // Feature name
    ImVec2 name_pos = ImVec2(cursor_pos.x + 70, cursor_pos.y + 20);
    draw_list->AddText(name_pos, COLOR_TEXT_PRIMARY, feature.name.c_str());
    
    // Feature description
    ImVec2 desc_pos = ImVec2(cursor_pos.x + 15, cursor_pos.y + 60);
    std::string truncated_desc = feature.description;
    if (truncated_desc.length() > 80) {
        truncated_desc = truncated_desc.substr(0, 77) + "...";
    }
    
    draw_list->AddText(desc_pos, COLOR_TEXT_SECONDARY, truncated_desc.c_str());
    
    // Status indicator
    ImVec2 status_pos = ImVec2(cursor_pos.x + card_size.x - 30, cursor_pos.y + 15);
    bool healthy = !feature.status_callback || feature.status_callback();
    draw_list->AddCircleFilled(status_pos, 6, healthy ? COLOR_SUCCESS : COLOR_ERROR);
    
    // Favorite indicator
    if (feature.favorite) {
        ImVec2 fav_pos = ImVec2(cursor_pos.x + card_size.x - 60, cursor_pos.y + 15);
        draw_list->AddText(fav_pos, COLOR_WARNING, ICON_STAR);
    }
    
    // Launch button
    ImVec2 button_pos = ImVec2(cursor_pos.x + card_size.x - 80, cursor_pos.y + card_size.y - 35);
    if (feature.enabled) {
        draw_list->AddRectFilled(button_pos, 
                               ImVec2(button_pos.x + 65, button_pos.y + 25),
                               card_color, 4.0f);
        draw_list->AddText(ImVec2(button_pos.x + 15, button_pos.y + 5), 
                          COLOR_TEXT_PRIMARY, "Launch");
    }
    
    // Handle interactions
    if (is_clicked) {
        selected_feature_ = feature.id;
        if (feature.enabled && feature.launch_callback) {
            feature.launch_callback();
        }
    }
    
    // Context menu
    if (ImGui::BeginPopupContextItem(("context_" + feature.id).c_str())) {
        if (ImGui::MenuItem("Launch", nullptr, false, feature.enabled)) {
            launch_feature(feature.id);
        }
        
        ImGui::Separator();
        
        if (ImGui::MenuItem(feature.favorite ? "Remove from Favorites" : "Add to Favorites")) {
            toggle_favorite(feature.id);
        }
        
        if (ImGui::MenuItem("Properties")) {
            selected_feature_ = feature.id;
            panel_visibility_[PanelType::Properties] = true;
        }
        
        ImGui::EndPopup();
    }
    
    // Tooltip
    if (is_hovered) {
        ImGui::BeginTooltip();
        ImGui::Text("%s", feature.name.c_str());
        ImGui::Separator();
        ImGui::TextWrapped("%s", feature.description.c_str());
        
        if (!feature.version.empty()) {
            ImGui::Text("Version: %s", feature.version.c_str());
        }
        
        if (!feature.dependencies.empty()) {
            ImGui::Text("Dependencies: %s", 
                       std::accumulate(feature.dependencies.begin(), feature.dependencies.end(), std::string(),
                                     [](const std::string& a, const std::string& b) {
                                         return a.empty() ? b : a + ", " + b;
                                     }).c_str());
        }
        
        ImGui::EndTooltip();
    }
#endif
}

void Dashboard::render_system_health_indicator(const SystemStatus& status) {
#ifdef ECSCOPE_HAS_IMGUI
    draw_status_led(status.healthy);
    ImGui::SameLine();
    
    ImGui::Text("%s", status.name.c_str());
    ImGui::SameLine();
    
    if (!status.status_message.empty()) {
        ImGui::TextColored(status.healthy ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 
                          " - %s", status.status_message.c_str());
    }
    
    // Resource usage bars
    if (status.cpu_usage > 0.0f) {
        ImGui::Text("  CPU: %.1f%%", status.cpu_usage);
        ImGui::SameLine();
        ImGui::ProgressBar(status.cpu_usage / 100.0f, ImVec2(100, 0));
    }
    
    if (status.memory_usage > 0) {
        ImGui::Text("  Memory: %s", format_memory_size(status.memory_usage).c_str());
    }
#endif
}

void Dashboard::render_performance_graph() {
#ifdef ECSCOPE_HAS_IMGUI
    if (metrics_history_.empty()) {
        return;
    }
    
    // Extract frame times for graph
    std::vector<float> frame_times;
    frame_times.reserve(metrics_history_.size());
    
    for (const auto& metrics : metrics_history_) {
        frame_times.push_back(metrics.frame_time_ms);
    }
    
    // Draw frame time graph
    ImGui::Text("Frame Time History:");
    draw_sparkline("##FrameTime", frame_times.data(), (int)frame_times.size(), 
                   0.0f, 33.33f, ImVec2(0, 60)); // 0-33ms range (30-inf FPS)
    
    ImGui::SameLine();
    ImGui::Text("%.2f ms", frame_times.back());
#endif
}

void Dashboard::render_memory_usage_chart() {
#ifdef ECSCOPE_HAS_IMGUI
    if (metrics_history_.empty()) {
        return;
    }
    
    // Extract memory usage for chart
    std::vector<float> memory_usage;
    memory_usage.reserve(metrics_history_.size());
    
    for (const auto& metrics : metrics_history_) {
        memory_usage.push_back(static_cast<float>(metrics.memory_usage) / (1024.0f * 1024.0f)); // Convert to MB
    }
    
    ImGui::Text("Memory Usage History:");
    draw_sparkline("##MemoryUsage", memory_usage.data(), (int)memory_usage.size(), 
                   0.0f, FLT_MAX, ImVec2(0, 60));
    
    ImGui::SameLine();
    ImGui::Text("%.1f MB", memory_usage.back());
#endif
}

void Dashboard::create_dockspace_layout() {
    // Docking not available in this ImGui version
    // No-op function
}

const char* Dashboard::get_category_icon(FeatureCategory category) const {
    switch (category) {
        case FeatureCategory::Core: return ICON_CUBE;
        case FeatureCategory::Rendering: return ICON_ROCKET;
        case FeatureCategory::Physics: return ICON_COG;
        case FeatureCategory::Audio: return ICON_HEART;
        case FeatureCategory::Networking: return ICON_CHART;
        case FeatureCategory::Tools: return ICON_WRENCH;
        case FeatureCategory::Debugging: return ICON_BUG;
        case FeatureCategory::Performance: return ICON_CHART;
        default: return ICON_COG;
    }
}

ImU32 Dashboard::get_category_color(FeatureCategory category) const {
    auto it = CATEGORY_COLORS.find(category);
    return (it != CATEGORY_COLORS.end()) ? it->second : COLOR_PRIMARY;
}

void Dashboard::save_layout_to_ini() {
#ifdef ECSCOPE_HAS_IMGUI
    // ImGui automatically saves layout to imgui.ini
    // We could also save our own custom layout data here
    core::Log::info("Dashboard: Layout saved");
#endif
}

bool Dashboard::load_layout_from_ini() {
#ifdef ECSCOPE_HAS_IMGUI
    // ImGui automatically loads layout from imgui.ini
    // We could also load our own custom layout data here
    core::Log::info("Dashboard: Layout loaded");
    return true;
#endif
    return false;
}

// =============================================================================
// IMGUI HELPER FUNCTIONS
// =============================================================================

#ifdef ECSCOPE_HAS_IMGUI

void draw_status_led(bool healthy, const ImVec2& size) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
    
    ImU32 color = healthy ? COLOR_SUCCESS : COLOR_ERROR;
    float radius = std::min(size.x, size.y) * 0.4f;
    ImVec2 center = ImVec2(cursor_pos.x + size.x * 0.5f, cursor_pos.y + size.y * 0.5f);
    
    draw_list->AddCircleFilled(center, radius, color);
    draw_list->AddCircle(center, radius, IM_COL32(255, 255, 255, 100), 0, 1.0f);
    
    ImGui::Dummy(size);
}

void draw_sparkline(const char* label, const float* values, int values_count, 
                   float scale_min, float scale_max, ImVec2 graph_size) {
    
    if (values_count < 2) return;
    
    // Auto-scale if needed
    if (scale_min == FLT_MAX || scale_max == FLT_MAX) {
        scale_min = scale_max = values[0];
        for (int i = 1; i < values_count; ++i) {
            scale_min = std::min(scale_min, values[i]);
            scale_max = std::max(scale_max, values[i]);
        }
        
        // Add some padding
        float range = scale_max - scale_min;
        if (range < 0.01f) range = 1.0f;
        scale_min -= range * 0.1f;
        scale_max += range * 0.1f;
    }
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
    
    if (graph_size.x == 0) graph_size.x = ImGui::CalcItemWidth();
    if (graph_size.y == 0) graph_size.y = 40;
    
    // Background
    draw_list->AddRectFilled(cursor_pos, 
                           ImVec2(cursor_pos.x + graph_size.x, cursor_pos.y + graph_size.y),
                           IM_COL32(30, 30, 30, 255));
    
    // Draw lines
    float scale = graph_size.y / (scale_max - scale_min);
    float step = graph_size.x / (values_count - 1);
    
    for (int i = 0; i < values_count - 1; ++i) {
        float y1 = graph_size.y - (values[i] - scale_min) * scale;
        float y2 = graph_size.y - (values[i + 1] - scale_min) * scale;
        
        ImVec2 p1 = ImVec2(cursor_pos.x + i * step, cursor_pos.y + y1);
        ImVec2 p2 = ImVec2(cursor_pos.x + (i + 1) * step, cursor_pos.y + y2);
        
        draw_list->AddLine(p1, p2, COLOR_PRIMARY, 2.0f);
    }
    
    // Border
    draw_list->AddRect(cursor_pos, 
                      ImVec2(cursor_pos.x + graph_size.x, cursor_pos.y + graph_size.y),
                      IM_COL32(100, 100, 100, 255));
    
    ImGui::Dummy(graph_size);
}

void setup_professional_dark_theme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;
    
    // Window styling
    style.WindowRounding = 8.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;
    
    style.WindowPadding = ImVec2(12, 12);
    style.FramePadding = ImVec2(8, 4);
    style.ItemSpacing = ImVec2(8, 8);
    style.ItemInnerSpacing = ImVec2(6, 4);
    style.IndentSpacing = 20.0f;
    style.ScrollbarSize = 16.0f;
    style.GrabMinSize = 12.0f;
    
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.TabBorderSize = 0.0f;
    
    // Professional dark color scheme
    colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border]                 = ImVec4(0.30f, 0.30f, 0.30f, 0.50f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.60f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.39f, 0.71f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.39f, 0.71f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.59f, 0.81f, 1.00f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.39f, 0.71f, 1.00f, 0.40f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.39f, 0.71f, 1.00f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.59f, 0.81f, 1.00f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.39f, 0.71f, 1.00f, 0.31f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.39f, 0.71f, 1.00f, 0.80f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.39f, 0.71f, 1.00f, 1.00f);
    colors[ImGuiCol_Separator]              = ImVec4(0.39f, 0.71f, 1.00f, 0.50f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.39f, 0.71f, 1.00f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.39f, 0.71f, 1.00f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.39f, 0.71f, 1.00f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.39f, 0.71f, 1.00f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.39f, 0.71f, 1.00f, 0.95f);
    colors[ImGuiCol_Tab]                    = ImVec4(0.18f, 0.35f, 0.58f, 0.86f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.39f, 0.71f, 1.00f, 0.80f);
    colors[ImGuiCol_TabActive]              = ImVec4(0.30f, 0.60f, 1.00f, 1.00f);
    colors[ImGuiCol_TabUnfocused]           = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
    // Docking colors not available in this ImGui version
    // colors[ImGuiCol_DockingPreview]         = ImVec4(0.39f, 0.71f, 1.00f, 0.70f);
    // colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight]           = ImVec4(0.39f, 0.71f, 1.00f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

void setup_clean_light_theme() {
    ImGui::StyleColorsLight();
    
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Adjust for cleaner look
    style.WindowRounding = 6.0f;
    style.FrameRounding = 3.0f;
    style.PopupRounding = 3.0f;
    style.ScrollbarRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 3.0f;
}

void setup_high_contrast_theme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;
    
    // High contrast colors for accessibility
    colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.94f);
    colors[ImGuiCol_Border]                 = ImVec4(1.00f, 1.00f, 1.00f, 0.80f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    
    // Strong borders for better definition
    style.WindowBorderSize = 2.0f;
    style.FrameBorderSize = 2.0f;
    style.PopupBorderSize = 2.0f;
}

#endif // ECSCOPE_HAS_IMGUI

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

std::vector<FeatureInfo> create_default_ecscope_features() {
    std::vector<FeatureInfo> features;
    
    // Core systems
    features.push_back({
        "ecs_core", "ECS Architecture", 
        "Entity-Component-System with advanced scheduling, dependency management, and parallel execution.",
        "", FeatureCategory::Core, true, false,
        []() { /* Launch ECS demo */ },
        []() { return true; }, // Always healthy for demo
        {}, "1.0.0", ""
    });
    
    features.push_back({
        "memory_management", "Memory Management",
        "Advanced memory allocators, pool management, and leak detection with comprehensive tracking.",
        "", FeatureCategory::Core, true, false,
        []() { /* Launch memory profiler */ },
        []() { return true; },
        {}, "1.0.0", ""
    });
    
    // Rendering systems
    features.push_back({
        "modern_rendering", "Modern Rendering",
        "Multi-API rendering engine with Vulkan/OpenGL backends, deferred rendering, and PBR materials.",
        "", FeatureCategory::Rendering, true, true,
        []() { /* Launch rendering demo */ },
        []() { return true; },
        {"vulkan", "opengl"}, "1.0.0", ""
    });
    
    features.push_back({
        "shader_system", "Shader System",
        "Hot-reloadable shader compilation with real-time editing and cross-platform support.",
        "", FeatureCategory::Rendering, true, false,
        []() { /* Launch shader editor */ },
        []() { return true; },
        {"modern_rendering"}, "1.0.0", ""
    });
    
    // Physics systems
    features.push_back({
        "physics_engine", "Physics Engine",
        "High-performance 3D physics simulation with broadphase, narrowphase, and constraint solving.",
        "", FeatureCategory::Physics, true, true,
        []() { /* Launch physics demo */ },
        []() { return true; },
        {}, "1.0.0", ""
    });
    
    // Audio systems
    features.push_back({
        "audio_system", "Audio System",
        "3D spatial audio with real-time effects, streaming, and multi-format support.",
        "", FeatureCategory::Audio, true, false,
        []() { /* Launch audio demo */ },
        []() { return true; },
        {}, "1.0.0", ""
    });
    
    // Networking
    features.push_back({
        "networking", "Networking",
        "High-performance networking with UDP/TCP support, serialization, and prediction.",
        "", FeatureCategory::Networking, true, false,
        []() { /* Launch network demo */ },
        []() { return true; },
        {}, "1.0.0", ""
    });
    
    // Development tools
    features.push_back({
        "asset_pipeline", "Asset Pipeline",
        "Automated asset processing with hot-reloading, compression, and format conversion.",
        "", FeatureCategory::Tools, true, false,
        []() { /* Launch asset browser */ },
        []() { return true; },
        {}, "1.0.0", ""
    });
    
    features.push_back({
        "profiler", "Performance Profiler",
        "Real-time CPU/GPU profiling with detailed timing, memory usage, and bottleneck analysis.",
        "", FeatureCategory::Performance, true, true,
        []() { /* Launch profiler */ },
        []() { return true; },
        {}, "1.0.0", ""
    });
    
    features.push_back({
        "debugger", "Visual Debugger",
        "Integrated debugging tools with breakpoints, variable inspection, and call stack analysis.",
        "", FeatureCategory::Debugging, true, false,
        []() { /* Launch debugger */ },
        []() { return true; },
        {}, "1.0.0", ""
    });
    
    return features;
}

std::string category_to_string(FeatureCategory category) {
    switch (category) {
        case FeatureCategory::Core: return "Core";
        case FeatureCategory::Rendering: return "Rendering";
        case FeatureCategory::Physics: return "Physics";
        case FeatureCategory::Audio: return "Audio";
        case FeatureCategory::Networking: return "Networking";
        case FeatureCategory::Tools: return "Tools";
        case FeatureCategory::Debugging: return "Debugging";
        case FeatureCategory::Performance: return "Performance";
        default: return "Unknown";
    }
}

std::string workspace_to_string(WorkspacePreset preset) {
    switch (preset) {
        case WorkspacePreset::Overview: return "Overview";
        case WorkspacePreset::Development: return "Development";
        case WorkspacePreset::Debugging: return "Debugging";
        case WorkspacePreset::Performance: return "Performance";
        case WorkspacePreset::ContentCreation: return "Content Creation";
        case WorkspacePreset::Testing: return "Testing";
        case WorkspacePreset::Custom: return "Custom";
        default: return "Unknown";
    }
}

std::string format_memory_size(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    double size = static_cast<double>(bytes);
    int unit = 0;
    
    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
        ++unit;
    }
    
    std::ostringstream ss;
    ss.precision(1);
    ss << std::fixed << size << " " << units[unit];
    return ss.str();
}

std::string format_time_ms(float milliseconds) {
    std::ostringstream ss;
    ss.precision(2);
    ss << std::fixed << milliseconds << " ms";
    return ss.str();
}

// Missing method implementations
void Dashboard::register_system_monitor(const std::string& name, std::function<SystemStatus()> monitor) {
    // Mock implementation for demo purposes
    static std::unordered_map<std::string, std::function<SystemStatus()>> monitors;
    monitors[name] = monitor;
}

void Dashboard::update_performance_metrics(const PerformanceMetrics& metrics) {
    // Mock implementation - store metrics for display
    static PerformanceMetrics current_metrics;
    current_metrics = metrics;
}

bool Dashboard::save_config(const std::string& config_path) const {
    // Mock implementation - would normally save dashboard configuration
    // For demo purposes, just log the operation and return success
    return true;
}

void Dashboard::setup_default_layout() {
    // Mock implementation - would set up default ImGui docking layout
    // For demo purposes, just ensure basic panels are visible
    panel_visibility_[PanelType::Welcome] = true;
    panel_visibility_[PanelType::FeatureGallery] = true;
    panel_visibility_[PanelType::Properties] = false;
}

void Dashboard::navigate_to_panel(PanelType panel) {
    // Mock implementation - navigate to specific panel
    for (auto& [type, visible] : panel_visibility_) {
        visible = (type == panel);
    }
}

void Dashboard::show_panel(PanelType panel, bool show) {
    // Mock implementation - show/hide specific panel
    panel_visibility_[panel] = show;
}

} // namespace ecscope::gui