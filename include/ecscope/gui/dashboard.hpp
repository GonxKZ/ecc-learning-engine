/**
 * @file dashboard.hpp
 * @brief ECScope Engine Main Dashboard Interface
 * 
 * Professional main dashboard using Dear ImGui with comprehensive UI/UX design.
 * Features docking system, navigation, feature gallery, and system integration.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>
#include <array>

// ImGui includes (assuming installed)
#ifdef ECSCOPE_HAS_IMGUI
#include <imgui.h>
#include <imgui_internal.h>
#endif

// ECScope includes
#include "../core/log.hpp"
#include "../core/time.hpp"
#include "../rendering/renderer.hpp"

namespace ecscope::gui {

// =============================================================================
// CORE TYPES & ENUMERATIONS
// =============================================================================

/**
 * @brief Dashboard themes for visual customization
 */
enum class DashboardTheme : uint8_t {
    Dark,           ///< Professional dark theme (default)
    Light,          ///< Clean light theme
    HighContrast,   ///< Accessibility-focused theme
    Custom          ///< User-defined theme
};

/**
 * @brief Feature categories for organized navigation
 */
enum class FeatureCategory : uint8_t {
    Core,           ///< Core engine systems
    Rendering,      ///< Rendering and graphics
    Physics,        ///< Physics simulation
    Audio,          ///< Audio systems
    Networking,     ///< Network functionality
    Tools,          ///< Development tools
    Debugging,      ///< Debugging utilities
    Performance     ///< Performance monitoring
};

/**
 * @brief Workspace presets for different development tasks
 */
enum class WorkspacePreset : uint8_t {
    Overview,       ///< General overview layout
    Development,    ///< Code development focused
    Debugging,      ///< Debugging and profiling
    Performance,    ///< Performance analysis
    ContentCreation,///< Asset and content creation
    Testing,        ///< Testing and validation
    Custom          ///< User-defined layout
};

/**
 * @brief Panel types for the docking system
 */
enum class PanelType : uint8_t {
    Welcome,
    FeatureGallery,
    SystemStatus,
    Performance,
    LogOutput,
    Properties,
    Explorer,
    Viewport,
    Tools,
    Settings
};

// =============================================================================
// FEATURE SYSTEM
// =============================================================================

/**
 * @brief Individual feature descriptor
 */
struct FeatureInfo {
    std::string id;
    std::string name;
    std::string description;
    std::string icon; // Font icon or path to icon
    FeatureCategory category;
    bool enabled = true;
    bool favorite = false;
    std::function<void()> launch_callback;
    std::function<bool()> status_callback; // Returns true if system is healthy
    std::vector<std::string> dependencies;
    std::string version;
    std::string documentation_url;
};

/**
 * @brief System status information
 */
struct SystemStatus {
    std::string name;
    bool healthy = true;
    float cpu_usage = 0.0f;
    size_t memory_usage = 0;
    std::string status_message;
    std::chrono::steady_clock::time_point last_update;
};

/**
 * @brief Performance metrics
 */
struct PerformanceMetrics {
    float frame_rate = 0.0f;
    float frame_time_ms = 0.0f;
    float cpu_usage = 0.0f;
    size_t memory_usage = 0;
    size_t gpu_memory_usage = 0;
    uint32_t draw_calls = 0;
    uint32_t vertices_rendered = 0;
    std::chrono::steady_clock::time_point timestamp;
};

// =============================================================================
// MAIN DASHBOARD CLASS
// =============================================================================

/**
 * @brief Main Dashboard Implementation
 * 
 * Professional dashboard interface providing:
 * - Docking system for flexible layout
 * - Feature gallery with visual previews
 * - System status monitoring
 * - Performance metrics
 * - Navigation and search
 * - Workspace management
 */
class Dashboard {
public:
    Dashboard();
    ~Dashboard();

    // =============================================================================
    // INITIALIZATION & LIFECYCLE
    // =============================================================================

    /**
     * @brief Initialize the dashboard system
     * @param renderer Optional renderer instance for 3D viewports
     * @return true on success
     */
    bool initialize(rendering::IRenderer* renderer = nullptr);

    /**
     * @brief Shutdown and cleanup resources
     */
    void shutdown();

    /**
     * @brief Check if dashboard is properly initialized
     */
    bool is_initialized() const { return initialized_; }

    // =============================================================================
    // MAIN RENDER LOOP
    // =============================================================================

    /**
     * @brief Render the main dashboard UI
     * Called once per frame from the main application loop
     */
    void render();

    /**
     * @brief Update dashboard state and metrics
     * @param delta_time Time since last update
     */
    void update(float delta_time);

    // =============================================================================
    // THEME & STYLING
    // =============================================================================

    /**
     * @brief Set the dashboard theme
     */
    void set_theme(DashboardTheme theme);

    /**
     * @brief Get current theme
     */
    DashboardTheme get_theme() const { return current_theme_; }

    /**
     * @brief Apply custom styling
     */
    void apply_custom_style(const ImGuiStyle& style);

    // =============================================================================
    // FEATURE MANAGEMENT
    // =============================================================================

    /**
     * @brief Register a new feature with the dashboard
     */
    void register_feature(const FeatureInfo& feature);

    /**
     * @brief Unregister a feature
     */
    void unregister_feature(const std::string& feature_id);

    /**
     * @brief Get all registered features
     */
    const std::vector<FeatureInfo>& get_features() const { return features_; }

    /**
     * @brief Get features by category
     */
    std::vector<FeatureInfo> get_features_by_category(FeatureCategory category) const;

    /**
     * @brief Launch a feature by ID
     */
    bool launch_feature(const std::string& feature_id);

    /**
     * @brief Add/remove feature from favorites
     */
    void toggle_favorite(const std::string& feature_id);

    // =============================================================================
    // WORKSPACE MANAGEMENT
    // =============================================================================

    /**
     * @brief Apply a workspace preset
     */
    void apply_workspace_preset(WorkspacePreset preset);

    /**
     * @brief Save current layout as custom workspace
     */
    void save_custom_workspace(const std::string& name);

    /**
     * @brief Load a custom workspace
     */
    bool load_custom_workspace(const std::string& name);

    /**
     * @brief Get list of available workspaces
     */
    std::vector<std::string> get_available_workspaces() const;

    // =============================================================================
    // SYSTEM MONITORING
    // =============================================================================

    /**
     * @brief Register a system for status monitoring
     */
    void register_system_monitor(const std::string& system_name,
                               std::function<SystemStatus()> status_callback);

    /**
     * @brief Update performance metrics
     */
    void update_performance_metrics(const PerformanceMetrics& metrics);

    /**
     * @brief Get current system status
     */
    const std::unordered_map<std::string, SystemStatus>& get_system_status() const {
        return system_status_;
    }

    // =============================================================================
    // NAVIGATION & SEARCH
    // =============================================================================

    /**
     * @brief Search features by name or description
     */
    std::vector<FeatureInfo> search_features(const std::string& query) const;

    /**
     * @brief Navigate to a specific panel
     */
    void navigate_to_panel(PanelType panel);

    /**
     * @brief Show/hide specific panels
     */
    void show_panel(PanelType panel, bool show = true);
    bool is_panel_visible(PanelType panel) const;

    // =============================================================================
    // CONFIGURATION
    // =============================================================================

    /**
     * @brief Save dashboard configuration to file
     */
    bool save_config(const std::string& filepath) const;

    /**
     * @brief Load dashboard configuration from file
     */
    bool load_config(const std::string& filepath);

    /**
     * @brief Reset to default configuration
     */
    void reset_to_defaults();

private:
    // =============================================================================
    // PRIVATE RENDERING METHODS
    // =============================================================================

    void render_main_menu_bar();
    void render_main_dockspace();
    void render_welcome_panel();
    void render_feature_gallery_panel();
    void render_system_status_panel();
    void render_performance_panel();
    void render_log_output_panel();
    void render_properties_panel();
    void render_explorer_panel();
    void render_viewport_panel();
    void render_tools_panel();
    void render_settings_panel();

    // Feature gallery rendering
    void render_feature_card(const FeatureInfo& feature);
    void render_category_section(FeatureCategory category);
    
    // System status widgets
    void render_system_health_indicator(const SystemStatus& status);
    void render_performance_graph();
    void render_memory_usage_chart();
    
    // Navigation helpers
    void render_search_bar();
    void render_breadcrumb_navigation();
    void render_quick_actions();

    // =============================================================================
    // PRIVATE UTILITY METHODS
    // =============================================================================

    void initialize_default_features();
    void setup_theme_colors(DashboardTheme theme);
    void setup_default_layout();
    void update_system_monitors();
    
    // Icon helpers
    const char* get_category_icon(FeatureCategory category) const;
    const char* get_feature_icon(const FeatureInfo& feature) const;
    ImU32 get_category_color(FeatureCategory category) const;
    
    // Layout helpers
    void create_dockspace_layout();
    void save_layout_to_ini();
    bool load_layout_from_ini();

    // =============================================================================
    // MEMBER VARIABLES
    // =============================================================================

    // Core state
    bool initialized_ = false;
    DashboardTheme current_theme_ = DashboardTheme::Dark;
    WorkspacePreset current_workspace_ = WorkspacePreset::Overview;
    
    // Rendering
    rendering::IRenderer* renderer_ = nullptr;
    
    // Features and systems
    std::vector<FeatureInfo> features_;
    std::unordered_map<std::string, SystemStatus> system_status_;
    std::unordered_map<std::string, std::function<SystemStatus()>> system_monitors_;
    
    // Performance tracking
    PerformanceMetrics current_metrics_;
    std::vector<PerformanceMetrics> metrics_history_;
    static constexpr size_t MAX_METRICS_HISTORY = 300; // 5 seconds at 60fps
    
    // UI state
    std::unordered_map<PanelType, bool> panel_visibility_;
    std::string search_query_;
    std::vector<std::string> navigation_breadcrumbs_;
    std::optional<std::string> selected_feature_;
    
    // Layout management
    std::unordered_map<std::string, std::string> saved_workspaces_;
    bool dockspace_initialized_ = false;
    ImGuiID main_dockspace_id_ = 0;
    
    // Configuration
    std::string config_filepath_ = "ecscope_dashboard.ini";
    bool show_demo_window_ = false;
    bool show_style_editor_ = false;
    
    // Timing
    std::chrono::steady_clock::time_point last_update_time_;
    float update_interval_ = 1.0f / 60.0f; // 60 FPS target
};

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

/**
 * @brief Create default ECScope feature set
 */
std::vector<FeatureInfo> create_default_ecscope_features();

/**
 * @brief Convert feature category to display string
 */
std::string category_to_string(FeatureCategory category);

/**
 * @brief Convert workspace preset to display string
 */
std::string workspace_to_string(WorkspacePreset preset);

/**
 * @brief Helper to format memory size
 */
std::string format_memory_size(size_t bytes);

/**
 * @brief Helper to format performance time
 */
std::string format_time_ms(float milliseconds);

// =============================================================================
// IMGUI HELPER FUNCTIONS
// =============================================================================

#ifdef ECSCOPE_HAS_IMGUI

/**
 * @brief Draw a status indicator LED
 */
void draw_status_led(bool healthy, const ImVec2& size = ImVec2(12, 12));

/**
 * @brief Draw a progress ring
 */
void draw_progress_ring(float progress, const ImVec2& center, float radius, 
                       ImU32 color = IM_COL32(100, 180, 255, 255));

/**
 * @brief Draw a sparkline chart
 */
void draw_sparkline(const char* label, const float* values, int values_count, 
                   float scale_min = FLT_MAX, float scale_max = FLT_MAX, 
                   ImVec2 graph_size = ImVec2(0, 35));

/**
 * @brief Draw feature card with preview
 */
void draw_feature_card(const FeatureInfo& feature, const ImVec2& card_size = ImVec2(280, 200));

/**
 * @brief Draw system health widget
 */
void draw_system_health_widget(const SystemStatus& status);

/**
 * @brief Setup professional dark theme
 */
void setup_professional_dark_theme();

/**
 * @brief Setup clean light theme
 */
void setup_clean_light_theme();

/**
 * @brief Setup high contrast theme
 */
void setup_high_contrast_theme();

#endif // ECSCOPE_HAS_IMGUI

} // namespace ecscope::gui