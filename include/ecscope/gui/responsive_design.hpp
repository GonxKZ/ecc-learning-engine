/**
 * @file responsive_design.hpp
 * @brief ECScope Responsive Design System
 * 
 * Comprehensive responsive design system for ECScope's Dear ImGui-based UI.
 * Provides DPI scaling, adaptive layouts, screen size detection, and touch
 * interface support for professional game engine interfaces.
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

#ifdef ECSCOPE_HAS_IMGUI
#include <imgui.h>
#endif

#ifdef ECSCOPE_HAS_GLFW
struct GLFWwindow;
struct GLFWmonitor;
#endif

namespace ecscope::gui {

// =============================================================================
// ENUMERATIONS & TYPES
// =============================================================================

/**
 * @brief Screen size categories for responsive breakpoints
 */
enum class ScreenSize {
    XSmall,    // < 480px  - Mobile phones (portrait)
    Small,     // < 768px  - Mobile phones (landscape), small tablets
    Medium,    // < 1024px - Tablets, small laptops
    Large,     // < 1440px - Desktop monitors, laptops
    XLarge,    // < 1920px - Large desktop monitors
    XXLarge    // >= 1920px - Ultra-wide and 4K displays
};

/**
 * @brief DPI categories for scaling
 */
enum class DPICategory {
    Standard,  // 96 DPI (1.0x scale)
    High,      // 144 DPI (1.5x scale)
    VeryHigh,  // 192 DPI (2.0x scale)
    Ultra      // 288+ DPI (3.0x+ scale)
};

/**
 * @brief Touch interface mode
 */
enum class TouchMode {
    Disabled,  // Mouse/keyboard only
    Enabled,   // Touch-friendly interfaces
    Auto       // Automatically detect touch capability
};

/**
 * @brief Responsive layout mode
 */
enum class ResponsiveMode {
    Fixed,     // Fixed layout (no responsiveness)
    Fluid,     // Fluid layout (scales with window)
    Adaptive,  // Adaptive layout (discrete breakpoints)
    Hybrid     // Hybrid (combines fluid and adaptive)
};

/**
 * @brief Display information structure
 */
struct DisplayInfo {
    int width = 1920;
    int height = 1080;
    float dpi_scale = 1.0f;
    DPICategory dpi_category = DPICategory::Standard;
    ScreenSize screen_size = ScreenSize::Large;
    bool is_primary = true;
    std::string name = "Primary Display";
};

/**
 * @brief Responsive layout configuration
 */
struct ResponsiveConfig {
    ResponsiveMode mode = ResponsiveMode::Adaptive;
    TouchMode touch_mode = TouchMode::Auto;
    bool auto_dpi_scaling = true;
    bool preserve_aspect_ratio = true;
    float min_ui_scale = 0.75f;
    float max_ui_scale = 3.0f;
    bool smooth_transitions = true;
    float transition_duration = 0.2f;
};

/**
 * @brief Layout constraints for responsive components
 */
struct LayoutConstraints {
    std::optional<float> min_width;
    std::optional<float> max_width;
    std::optional<float> min_height;
    std::optional<float> max_height;
    float preferred_aspect_ratio = 0.0f;
    bool maintain_aspect = false;
};

/**
 * @brief Responsive spacing values
 */
struct ResponsiveSpacing {
    struct SpacingSet {
        float tiny = 2.0f;
        float small = 4.0f;
        float medium = 8.0f;
        float large = 16.0f;
        float xlarge = 24.0f;
        float xxlarge = 32.0f;
        float huge = 48.0f;
    };
    
    SpacingSet xs_screen;
    SpacingSet small_screen;
    SpacingSet medium_screen;
    SpacingSet large_screen;
    SpacingSet xlarge_screen;
    SpacingSet xxlarge_screen;
    
    ResponsiveSpacing();
};

/**
 * @brief Responsive font system
 */
struct ResponsiveFonts {
    struct FontScale {
        float display = 2.0f;      // Hero headlines
        float h1 = 1.75f;          // Page titles
        float h2 = 1.5f;           // Section headers
        float h3 = 1.25f;          // Subsection headers
        float body = 1.0f;         // Body text
        float small = 0.875f;      // Small text
        float tiny = 0.75f;        // Caption text
    };
    
    FontScale mobile;
    FontScale tablet;
    FontScale desktop;
    FontScale large_desktop;
    
    float base_size = 16.0f;       // Base font size in pixels
    bool use_oversampling = true;  // Use font oversampling for clarity
    
    ResponsiveFonts();
};

// =============================================================================
// RESPONSIVE DESIGN MANAGER
// =============================================================================

/**
 * @brief Central responsive design system manager
 * 
 * Manages DPI scaling, responsive breakpoints, font scaling, adaptive layouts,
 * and touch interfaces for the ECScope GUI system.
 */
class ResponsiveDesignManager {
public:
    ResponsiveDesignManager();
    ~ResponsiveDesignManager();

    // Non-copyable
    ResponsiveDesignManager(const ResponsiveDesignManager&) = delete;
    ResponsiveDesignManager& operator=(const ResponsiveDesignManager&) = delete;

    // =============================================================================
    // INITIALIZATION & LIFECYCLE
    // =============================================================================

    /**
     * @brief Initialize the responsive design system
     * @param window GLFW window handle for display detection
     * @param config Responsive configuration
     * @return true on success
     */
    bool initialize(GLFWwindow* window = nullptr, const ResponsiveConfig& config = ResponsiveConfig{});

    /**
     * @brief Shutdown the responsive design system
     */
    void shutdown();

    /**
     * @brief Update responsive system (call each frame)
     * @param delta_time Time since last frame
     */
    void update(float delta_time);

    // =============================================================================
    // DISPLAY DETECTION & DPI SCALING
    // =============================================================================

    /**
     * @brief Detect all connected displays
     */
    void detect_displays();

    /**
     * @brief Get primary display information
     */
    const DisplayInfo& get_primary_display() const { return primary_display_; }

    /**
     * @brief Get all display information
     */
    const std::vector<DisplayInfo>& get_all_displays() const { return displays_; }

    /**
     * @brief Get current DPI scale factor
     */
    float get_dpi_scale() const { return current_dpi_scale_; }

    /**
     * @brief Get current screen size category
     */
    ScreenSize get_screen_size() const { return current_screen_size_; }

    /**
     * @brief Get effective UI scale (combines DPI scale and user preferences)
     */
    float get_effective_ui_scale() const { return effective_ui_scale_; }

    /**
     * @brief Set user UI scale multiplier
     */
    void set_user_ui_scale(float scale);

    // =============================================================================
    // RESPONSIVE BREAKPOINTS
    // =============================================================================

    /**
     * @brief Check if current screen matches size category
     */
    bool is_screen_size(ScreenSize size) const { return current_screen_size_ == size; }

    /**
     * @brief Check if screen is at least the specified size
     */
    bool is_screen_at_least(ScreenSize size) const;

    /**
     * @brief Check if screen is at most the specified size
     */
    bool is_screen_at_most(ScreenSize size) const;

    /**
     * @brief Get responsive breakpoint widths
     */
    static constexpr float get_breakpoint_width(ScreenSize size);

    // =============================================================================
    // FONT MANAGEMENT
    // =============================================================================

    /**
     * @brief Get scaled font size for current screen/DPI
     */
    float get_font_size(const std::string& style = "body") const;

    /**
     * @brief Load responsive font atlas
     */
    bool load_responsive_fonts();

    /**
     * @brief Update font scaling
     */
    void update_font_scaling();

    /**
     * @brief Get font for specific screen size
     */
#ifdef ECSCOPE_HAS_IMGUI
    ImFont* get_font(ScreenSize screen_size, const std::string& style = "body") const;
#endif

    // =============================================================================
    // SPACING & LAYOUT
    // =============================================================================

    /**
     * @brief Get responsive spacing value
     */
    float get_spacing(const std::string& size = "medium") const;

    /**
     * @brief Get scaled ImVec2 spacing
     */
#ifdef ECSCOPE_HAS_IMGUI
    ImVec2 get_spacing_vec2(const std::string& horizontal = "medium", const std::string& vertical = "medium") const;
#endif

    /**
     * @brief Apply responsive constraints to size
     */
#ifdef ECSCOPE_HAS_IMGUI
    ImVec2 apply_constraints(const ImVec2& size, const LayoutConstraints& constraints) const;
#endif

    /**
     * @brief Calculate adaptive window size
     */
#ifdef ECSCOPE_HAS_IMGUI
    ImVec2 calculate_adaptive_window_size(const ImVec2& content_size, const LayoutConstraints& constraints = {}) const;
#endif

    // =============================================================================
    // TOUCH INTERFACE
    // =============================================================================

    /**
     * @brief Check if touch mode is enabled
     */
    bool is_touch_enabled() const { return current_touch_mode_ == TouchMode::Enabled; }

    /**
     * @brief Get touch-friendly button size
     */
#ifdef ECSCOPE_HAS_IMGUI
    ImVec2 get_touch_button_size() const;
#endif

    /**
     * @brief Get touch-friendly spacing
     */
    float get_touch_spacing() const;

    /**
     * @brief Set touch mode
     */
    void set_touch_mode(TouchMode mode);

    // =============================================================================
    // RESPONSIVE COMPONENTS
    // =============================================================================

    /**
     * @brief Begin responsive window
     */
#ifdef ECSCOPE_HAS_IMGUI
    bool begin_responsive_window(const char* name, bool* p_open = nullptr, ImGuiWindowFlags flags = 0);
#endif

    /**
     * @brief End responsive window
     */
    void end_responsive_window();

    /**
     * @brief Create responsive button
     */
#ifdef ECSCOPE_HAS_IMGUI
    bool responsive_button(const char* label, const ImVec2& size_hint = ImVec2(0, 0));
#endif

    /**
     * @brief Create responsive selectable
     */
#ifdef ECSCOPE_HAS_IMGUI
    bool responsive_selectable(const char* label, bool selected = false, ImGuiSelectableFlags flags = 0);
#endif

    /**
     * @brief Begin responsive group
     */
    void begin_responsive_group();

    /**
     * @brief End responsive group
     */
    void end_responsive_group();

    // =============================================================================
    // ADAPTIVE LAYOUTS
    // =============================================================================

    /**
     * @brief Begin adaptive columns layout
     */
    void begin_adaptive_columns(int base_columns, int max_columns = -1);

    /**
     * @brief End adaptive columns layout
     */
    void end_adaptive_columns();

    /**
     * @brief Calculate adaptive column count
     */
    int calculate_adaptive_columns(int base_columns, int max_columns, float available_width) const;

    /**
     * @brief Begin responsive flex layout
     */
    void begin_responsive_flex(bool horizontal = true);

    /**
     * @brief Add responsive flex item
     */
    void responsive_flex_item(float flex_grow = 1.0f);

    /**
     * @brief End responsive flex layout
     */
    void end_responsive_flex();

    // =============================================================================
    // UTILITY FUNCTIONS
    // =============================================================================

    /**
     * @brief Scale value by current effective scale
     */
    float scale(float value) const { return value * effective_ui_scale_; }

    /**
     * @brief Scale ImVec2 by current effective scale
     */
#ifdef ECSCOPE_HAS_IMGUI
    ImVec2 scale(const ImVec2& vec) const { return ImVec2(vec.x * effective_ui_scale_, vec.y * effective_ui_scale_); }
#endif

    /**
     * @brief Get current responsive configuration
     */
    const ResponsiveConfig& get_config() const { return config_; }

    /**
     * @brief Update responsive configuration
     */
    void set_config(const ResponsiveConfig& config);

    /**
     * @brief Register screen size change callback
     */
    void add_screen_size_callback(std::function<void(ScreenSize, ScreenSize)> callback);

    /**
     * @brief Register DPI scale change callback
     */
    void add_dpi_scale_callback(std::function<void(float, float)> callback);

private:
    // =============================================================================
    // PRIVATE METHODS
    // =============================================================================

    void update_display_info();
    void update_screen_size();
    void update_dpi_scaling();
    void update_touch_detection();
    void notify_screen_size_change(ScreenSize old_size, ScreenSize new_size);
    void notify_dpi_scale_change(float old_scale, float new_scale);

    ScreenSize calculate_screen_size(int width, int height) const;
    DPICategory calculate_dpi_category(float dpi_scale) const;
    float calculate_effective_scale() const;

    void setup_responsive_fonts();
    void setup_responsive_spacing();

#ifdef ECSCOPE_HAS_GLFW
    static void monitor_callback(GLFWmonitor* monitor, int event);
#endif

    // =============================================================================
    // MEMBER VARIABLES
    // =============================================================================

    // Core state
    bool initialized_ = false;
    ResponsiveConfig config_;

    // Window and display information
#ifdef ECSCOPE_HAS_GLFW
    GLFWwindow* window_ = nullptr;
#endif
    std::vector<DisplayInfo> displays_;
    DisplayInfo primary_display_;

    // Current state
    ScreenSize current_screen_size_ = ScreenSize::Large;
    ScreenSize previous_screen_size_ = ScreenSize::Large;
    float current_dpi_scale_ = 1.0f;
    float previous_dpi_scale_ = 1.0f;
    float user_ui_scale_ = 1.0f;
    float effective_ui_scale_ = 1.0f;
    TouchMode current_touch_mode_ = TouchMode::Auto;

    // Font management
    ResponsiveFonts font_config_;
#ifdef ECSCOPE_HAS_IMGUI
    std::unordered_map<std::string, std::vector<ImFont*>> responsive_fonts_; // [style][screen_size]
#endif

    // Spacing management
    std::unique_ptr<ResponsiveSpacing> spacing_;

    // Layout state
    struct LayoutState {
        bool in_responsive_window = false;
        bool in_responsive_group = false;
        bool in_adaptive_columns = false;
        bool in_responsive_flex = false;
        int current_columns = 1;
        float flex_total_grow = 0.0f;
        std::vector<float> flex_items;
    } layout_state_;

    // Callbacks
    std::vector<std::function<void(ScreenSize, ScreenSize)>> screen_size_callbacks_;
    std::vector<std::function<void(float, float)>> dpi_scale_callbacks_;

    // Animation state
    float transition_timer_ = 0.0f;
    bool in_transition_ = false;
};

// =============================================================================
// RESPONSIVE WIDGETS & UTILITIES
// =============================================================================

/**
 * @brief Responsive widget helper class
 */
class ResponsiveWidget {
public:
    ResponsiveWidget(ResponsiveDesignManager* manager = nullptr);
    
    // Layout helpers
#ifdef ECSCOPE_HAS_IMGUI
    bool adaptive_layout(const std::vector<std::function<void()>>& widgets, int min_columns = 1, int max_columns = 4);
    void responsive_text(const char* text, bool centered = false);
    bool responsive_input_text(const char* label, char* buf, size_t buf_size);
    bool responsive_combo(const char* label, int* current_item, const char* const items[], int items_count);
    void responsive_separator();
    void responsive_spacing();
#endif
    
protected:
    ResponsiveDesignManager* manager_;
};

/**
 * @brief RAII responsive layout helper
 */
class ScopedResponsiveLayout {
public:
    enum class Type { Window, Group, Columns, Flex };
    
    ScopedResponsiveLayout(ResponsiveDesignManager* manager, Type type, const std::string& params = "");
    ~ScopedResponsiveLayout();

private:
    ResponsiveDesignManager* manager_;
    Type type_;
    bool valid_;
};

// =============================================================================
// RESPONSIVE STYLE PRESETS
// =============================================================================

/**
 * @brief Professional responsive style presets
 */
class ResponsiveStylePresets {
public:
    // Dashboard layouts
    static void apply_dashboard_mobile_style();
    static void apply_dashboard_tablet_style();
    static void apply_dashboard_desktop_style();
    
    // Inspector layouts
    static void apply_inspector_compact_style();
    static void apply_inspector_expanded_style();
    
    // Touch-friendly styles
    static void apply_touch_friendly_style();
    static void apply_touch_gestures_style();
    
    // Accessibility styles
    static void apply_high_contrast_responsive_style();
    static void apply_large_text_responsive_style();
};

// =============================================================================
// GLOBAL ACCESS
// =============================================================================

/**
 * @brief Get the global responsive design manager
 */
ResponsiveDesignManager* GetResponsiveDesignManager();

/**
 * @brief Initialize global responsive design manager
 */
bool InitializeGlobalResponsiveDesign(GLFWwindow* window = nullptr, const ResponsiveConfig& config = ResponsiveConfig{});

/**
 * @brief Shutdown global responsive design manager
 */
void ShutdownGlobalResponsiveDesign();

// =============================================================================
// INLINE IMPLEMENTATIONS
// =============================================================================

constexpr float ResponsiveDesignManager::get_breakpoint_width(ScreenSize size) {
    switch (size) {
        case ScreenSize::XSmall:  return 480.0f;
        case ScreenSize::Small:   return 768.0f;
        case ScreenSize::Medium:  return 1024.0f;
        case ScreenSize::Large:   return 1440.0f;
        case ScreenSize::XLarge:  return 1920.0f;
        case ScreenSize::XXLarge: return 2560.0f;
        default: return 1920.0f;
    }
}

} // namespace ecscope::gui