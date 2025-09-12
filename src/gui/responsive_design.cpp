/**
 * @file responsive_design.cpp
 * @brief ECScope Responsive Design System Implementation
 * 
 * Implementation of the comprehensive responsive design system for ECScope's
 * Dear ImGui-based UI, providing DPI scaling, adaptive layouts, and touch support.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#include "../../include/ecscope/gui/responsive_design.hpp"
#include "../../include/ecscope/core/log.hpp"

#ifdef ECSCOPE_HAS_IMGUI
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#endif

#ifdef ECSCOPE_HAS_GLFW
#include <GLFW/glfw3.h>
#endif

#include <algorithm>
#include <cmath>
#include <chrono>

namespace ecscope::gui {

// =============================================================================
// STATIC GLOBAL INSTANCE
// =============================================================================

namespace {
    static std::unique_ptr<ResponsiveDesignManager> g_responsive_manager = nullptr;
}

// =============================================================================
// RESPONSIVE SPACING IMPLEMENTATION
// =============================================================================

ResponsiveSpacing::ResponsiveSpacing() {
    // XS Screen (Mobile Portrait) - Tight spacing
    xs_screen.tiny = 1.0f;
    xs_screen.small = 2.0f;
    xs_screen.medium = 4.0f;
    xs_screen.large = 8.0f;
    xs_screen.xlarge = 12.0f;
    xs_screen.xxlarge = 16.0f;
    xs_screen.huge = 24.0f;
    
    // Small Screen (Mobile Landscape) - Compact spacing
    small_screen.tiny = 2.0f;
    small_screen.small = 3.0f;
    small_screen.medium = 6.0f;
    small_screen.large = 10.0f;
    small_screen.xlarge = 14.0f;
    small_screen.xxlarge = 20.0f;
    small_screen.huge = 28.0f;
    
    // Medium Screen (Tablet) - Standard spacing
    medium_screen.tiny = 2.0f;
    medium_screen.small = 4.0f;
    medium_screen.medium = 8.0f;
    medium_screen.large = 16.0f;
    medium_screen.xlarge = 24.0f;
    medium_screen.xxlarge = 32.0f;
    medium_screen.huge = 48.0f;
    
    // Large Screen (Desktop) - Comfortable spacing
    large_screen.tiny = 3.0f;
    large_screen.small = 6.0f;
    large_screen.medium = 12.0f;
    large_screen.large = 20.0f;
    large_screen.xlarge = 28.0f;
    large_screen.xxlarge = 36.0f;
    large_screen.huge = 56.0f;
    
    // XLarge Screen (Large Desktop) - Generous spacing
    xlarge_screen.tiny = 4.0f;
    xlarge_screen.small = 8.0f;
    xlarge_screen.medium = 16.0f;
    xlarge_screen.large = 24.0f;
    xlarge_screen.xlarge = 32.0f;
    xlarge_screen.xxlarge = 48.0f;
    xlarge_screen.huge = 64.0f;
    
    // XXLarge Screen (4K+) - Maximum spacing
    xxlarge_screen.tiny = 6.0f;
    xxlarge_screen.small = 12.0f;
    xxlarge_screen.medium = 20.0f;
    xxlarge_screen.large = 28.0f;
    xxlarge_screen.xlarge = 40.0f;
    xxlarge_screen.xxlarge = 56.0f;
    xxlarge_screen.huge = 80.0f;
}

// =============================================================================
// RESPONSIVE FONTS IMPLEMENTATION
// =============================================================================

ResponsiveFonts::ResponsiveFonts() {
    // Mobile font scales - More conservative to fit content
    mobile.display = 1.5f;
    mobile.h1 = 1.375f;
    mobile.h2 = 1.25f;
    mobile.h3 = 1.125f;
    mobile.body = 1.0f;
    mobile.small = 0.875f;
    mobile.tiny = 0.75f;
    
    // Tablet font scales - Balanced approach
    tablet.display = 1.75f;
    tablet.h1 = 1.5f;
    tablet.h2 = 1.375f;
    tablet.h3 = 1.25f;
    tablet.body = 1.0f;
    tablet.small = 0.875f;
    tablet.tiny = 0.75f;
    
    // Desktop font scales - Standard scales
    desktop.display = 2.0f;
    desktop.h1 = 1.75f;
    desktop.h2 = 1.5f;
    desktop.h3 = 1.25f;
    desktop.body = 1.0f;
    desktop.small = 0.875f;
    desktop.tiny = 0.75f;
    
    // Large desktop font scales - Enhanced readability
    large_desktop.display = 2.25f;
    large_desktop.h1 = 2.0f;
    large_desktop.h2 = 1.625f;
    large_desktop.h3 = 1.375f;
    large_desktop.body = 1.0f;
    large_desktop.small = 0.875f;
    large_desktop.tiny = 0.75f;
}

// =============================================================================
// RESPONSIVE DESIGN MANAGER IMPLEMENTATION
// =============================================================================

ResponsiveDesignManager::ResponsiveDesignManager() {
    spacing_ = std::make_unique<ResponsiveSpacing>();
    core::Log::info("ResponsiveDesignManager: Created");
}

ResponsiveDesignManager::~ResponsiveDesignManager() {
    shutdown();
    core::Log::info("ResponsiveDesignManager: Destroyed");
}

bool ResponsiveDesignManager::initialize(GLFWwindow* window, const ResponsiveConfig& config) {
    if (initialized_) {
        core::Log::warning("ResponsiveDesignManager: Already initialized");
        return true;
    }

    core::Log::info("ResponsiveDesignManager: Initializing responsive design system...");
    
    config_ = config;
#ifdef ECSCOPE_HAS_GLFW
    window_ = window;
#endif

    // Detect displays and set up initial state
    detect_displays();
    update_display_info();
    update_screen_size();
    update_dpi_scaling();
    update_touch_detection();

    // Setup responsive fonts and spacing
    setup_responsive_fonts();
    setup_responsive_spacing();

#ifdef ECSCOPE_HAS_GLFW
    // Set up monitor callback for display changes
    glfwSetMonitorCallback(monitor_callback);
#endif

    initialized_ = true;
    core::Log::info("ResponsiveDesignManager: Initialized successfully with {}x{} at {:.1f}x DPI scale", 
                   primary_display_.width, primary_display_.height, current_dpi_scale_);
    
    return true;
}

void ResponsiveDesignManager::shutdown() {
    if (!initialized_) return;

    core::Log::info("ResponsiveDesignManager: Shutting down...");

    // Clear callbacks
    screen_size_callbacks_.clear();
    dpi_scale_callbacks_.clear();

    // Clear font resources
#ifdef ECSCOPE_HAS_IMGUI
    responsive_fonts_.clear();
#endif

    // Reset state
    initialized_ = false;
    layout_state_ = {};
    
#ifdef ECSCOPE_HAS_GLFW
    window_ = nullptr;
#endif

    core::Log::info("ResponsiveDesignManager: Shutdown complete");
}

void ResponsiveDesignManager::update(float delta_time) {
    if (!initialized_) return;

    // Update transition timer
    if (in_transition_) {
        transition_timer_ += delta_time;
        if (transition_timer_ >= config_.transition_duration) {
            in_transition_ = false;
            transition_timer_ = 0.0f;
        }
    }

    // Check for display changes
    update_display_info();
    
    // Check for screen size changes
    ScreenSize old_screen_size = current_screen_size_;
    update_screen_size();
    if (old_screen_size != current_screen_size_) {
        notify_screen_size_change(old_screen_size, current_screen_size_);
        if (config_.smooth_transitions) {
            in_transition_ = true;
            transition_timer_ = 0.0f;
        }
    }

    // Check for DPI scale changes
    float old_dpi_scale = current_dpi_scale_;
    update_dpi_scaling();
    if (std::abs(old_dpi_scale - current_dpi_scale_) > 0.01f) {
        notify_dpi_scale_change(old_dpi_scale, current_dpi_scale_);
        update_font_scaling();
    }

    // Update touch detection
    update_touch_detection();
}

// =============================================================================
// DISPLAY DETECTION & DPI SCALING
// =============================================================================

void ResponsiveDesignManager::detect_displays() {
    displays_.clear();

#ifdef ECSCOPE_HAS_GLFW
    int monitor_count;
    GLFWmonitor** monitors = glfwGetMonitors(&monitor_count);
    
    GLFWmonitor* primary_monitor = glfwGetPrimaryMonitor();
    
    for (int i = 0; i < monitor_count; ++i) {
        GLFWmonitor* monitor = monitors[i];
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        
        DisplayInfo info;
        info.width = mode->width;
        info.height = mode->height;
        info.is_primary = (monitor == primary_monitor);
        info.name = glfwGetMonitorName(monitor);
        
        // Get DPI scale
        float xscale, yscale;
        glfwGetMonitorContentScale(monitor, &xscale, &yscale);
        info.dpi_scale = std::max(xscale, yscale);
        info.dpi_category = calculate_dpi_category(info.dpi_scale);
        info.screen_size = calculate_screen_size(info.width, info.height);
        
        displays_.push_back(info);
        
        if (info.is_primary) {
            primary_display_ = info;
        }
    }
#else
    // Fallback for non-GLFW builds
    DisplayInfo fallback;
    fallback.width = 1920;
    fallback.height = 1080;
    fallback.dpi_scale = 1.0f;
    fallback.dpi_category = DPICategory::Standard;
    fallback.screen_size = ScreenSize::Large;
    fallback.is_primary = true;
    fallback.name = "Default Display";
    
    displays_.push_back(fallback);
    primary_display_ = fallback;
#endif
}

void ResponsiveDesignManager::set_user_ui_scale(float scale) {
    user_ui_scale_ = std::clamp(scale, config_.min_ui_scale, config_.max_ui_scale);
    effective_ui_scale_ = calculate_effective_scale();
    
    core::Log::info("ResponsiveDesignManager: User UI scale set to {:.2f}, effective scale: {:.2f}", 
                   user_ui_scale_, effective_ui_scale_);
    
    update_font_scaling();
}

// =============================================================================
// RESPONSIVE BREAKPOINTS
// =============================================================================

bool ResponsiveDesignManager::is_screen_at_least(ScreenSize size) const {
    return static_cast<int>(current_screen_size_) >= static_cast<int>(size);
}

bool ResponsiveDesignManager::is_screen_at_most(ScreenSize size) const {
    return static_cast<int>(current_screen_size_) <= static_cast<int>(size);
}

// =============================================================================
// FONT MANAGEMENT
// =============================================================================

float ResponsiveDesignManager::get_font_size(const std::string& style) const {
    const ResponsiveFonts::FontScale* scale = nullptr;
    
    switch (current_screen_size_) {
        case ScreenSize::XSmall:
        case ScreenSize::Small:
            scale = &font_config_.mobile;
            break;
        case ScreenSize::Medium:
            scale = &font_config_.tablet;
            break;
        case ScreenSize::Large:
            scale = &font_config_.desktop;
            break;
        case ScreenSize::XLarge:
        case ScreenSize::XXLarge:
            scale = &font_config_.large_desktop;
            break;
    }
    
    float multiplier = 1.0f;
    if (style == "display") multiplier = scale->display;
    else if (style == "h1") multiplier = scale->h1;
    else if (style == "h2") multiplier = scale->h2;
    else if (style == "h3") multiplier = scale->h3;
    else if (style == "body") multiplier = scale->body;
    else if (style == "small") multiplier = scale->small;
    else if (style == "tiny") multiplier = scale->tiny;
    
    return font_config_.base_size * multiplier * effective_ui_scale_;
}

bool ResponsiveDesignManager::load_responsive_fonts() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGuiIO& io = ImGui::GetIO();
    
    // Clear existing fonts
    io.Fonts->Clear();
    responsive_fonts_.clear();
    
    std::vector<std::string> font_styles = {"display", "h1", "h2", "h3", "body", "small", "tiny"};
    std::vector<ScreenSize> screen_sizes = {ScreenSize::XSmall, ScreenSize::Small, ScreenSize::Medium, 
                                          ScreenSize::Large, ScreenSize::XLarge, ScreenSize::XXLarge};
    
    for (const std::string& style : font_styles) {
        std::vector<ImFont*> style_fonts;
        
        for (ScreenSize screen_size : screen_sizes) {
            // Calculate font size for this screen size
            ScreenSize old_size = current_screen_size_;
            current_screen_size_ = screen_size; // Temporarily set for calculation
            float font_size = get_font_size(style);
            current_screen_size_ = old_size; // Restore
            
            // Configure font
            ImFontConfig font_config;
            font_config.SizePixels = font_size;
            if (font_config_.use_oversampling) {
                font_config.OversampleH = 2;
                font_config.OversampleV = 2;
            }
            
            // Load default font with calculated size
            ImFont* font = io.Fonts->AddFontDefault(&font_config);
            style_fonts.push_back(font);
        }
        
        responsive_fonts_[style] = style_fonts;
    }
    
    // Build font atlas
    io.Fonts->Build();
    
    core::Log::info("ResponsiveDesignManager: Loaded {} font styles for {} screen sizes", 
                   font_styles.size(), screen_sizes.size());
    return true;
#else
    core::Log::warning("ResponsiveDesignManager: ImGui not available, cannot load fonts");
    return false;
#endif
}

void ResponsiveDesignManager::update_font_scaling() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGuiIO& io = ImGui::GetIO();
    
    // Update global font scale
    io.FontGlobalScale = 1.0f; // We handle scaling in font sizes directly
    
    // Apply style scaling
    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiStyle default_style;
    style = default_style;
    style.ScaleAllSizes(effective_ui_scale_);
#endif
}

#ifdef ECSCOPE_HAS_IMGUI
ImFont* ResponsiveDesignManager::get_font(ScreenSize screen_size, const std::string& style) const {
    auto style_it = responsive_fonts_.find(style);
    if (style_it == responsive_fonts_.end()) {
        return nullptr;
    }
    
    size_t size_index = static_cast<size_t>(screen_size);
    if (size_index >= style_it->second.size()) {
        return nullptr;
    }
    
    return style_it->second[size_index];
}
#endif

// =============================================================================
// SPACING & LAYOUT
// =============================================================================

float ResponsiveDesignManager::get_spacing(const std::string& size) const {
    const ResponsiveSpacing::SpacingSet* spacing_set = nullptr;
    
    switch (current_screen_size_) {
        case ScreenSize::XSmall:
            spacing_set = &spacing_->xs_screen;
            break;
        case ScreenSize::Small:
            spacing_set = &spacing_->small_screen;
            break;
        case ScreenSize::Medium:
            spacing_set = &spacing_->medium_screen;
            break;
        case ScreenSize::Large:
            spacing_set = &spacing_->large_screen;
            break;
        case ScreenSize::XLarge:
            spacing_set = &spacing_->xlarge_screen;
            break;
        case ScreenSize::XXLarge:
            spacing_set = &spacing_->xxlarge_screen;
            break;
    }
    
    float spacing_value = spacing_set->medium; // Default
    if (size == "tiny") spacing_value = spacing_set->tiny;
    else if (size == "small") spacing_value = spacing_set->small;
    else if (size == "medium") spacing_value = spacing_set->medium;
    else if (size == "large") spacing_value = spacing_set->large;
    else if (size == "xlarge") spacing_value = spacing_set->xlarge;
    else if (size == "xxlarge") spacing_value = spacing_set->xxlarge;
    else if (size == "huge") spacing_value = spacing_set->huge;
    
    return spacing_value * effective_ui_scale_;
}

#ifdef ECSCOPE_HAS_IMGUI
ImVec2 ResponsiveDesignManager::get_spacing_vec2(const std::string& horizontal, const std::string& vertical) const {
    return ImVec2(get_spacing(horizontal), get_spacing(vertical));
}

ImVec2 ResponsiveDesignManager::apply_constraints(const ImVec2& size, const LayoutConstraints& constraints) const {
    ImVec2 result = size;
    
    if (constraints.min_width.has_value()) {
        result.x = std::max(result.x, constraints.min_width.value() * effective_ui_scale_);
    }
    if (constraints.max_width.has_value()) {
        result.x = std::min(result.x, constraints.max_width.value() * effective_ui_scale_);
    }
    if (constraints.min_height.has_value()) {
        result.y = std::max(result.y, constraints.min_height.value() * effective_ui_scale_);
    }
    if (constraints.max_height.has_value()) {
        result.y = std::min(result.y, constraints.max_height.value() * effective_ui_scale_);
    }
    
    // Apply aspect ratio constraints
    if (constraints.maintain_aspect && constraints.preferred_aspect_ratio > 0.0f) {
        float current_aspect = result.x / result.y;
        float target_aspect = constraints.preferred_aspect_ratio;
        
        if (current_aspect > target_aspect) {
            // Too wide, constrain width
            result.x = result.y * target_aspect;
        } else {
            // Too tall, constrain height
            result.y = result.x / target_aspect;
        }
    }
    
    return result;
}

ImVec2 ResponsiveDesignManager::calculate_adaptive_window_size(const ImVec2& content_size, const LayoutConstraints& constraints) const {
    ImVec2 window_size = content_size;
    
    // Add padding and decorations
    ImGuiStyle& style = ImGui::GetStyle();
    window_size.x += style.WindowPadding.x * 2.0f;
    window_size.y += style.WindowPadding.y * 2.0f + style.FramePadding.y; // Title bar
    
    // Apply responsive scaling
    window_size = scale(window_size);
    
    // Apply constraints
    window_size = apply_constraints(window_size, constraints);
    
    // Ensure minimum viable size
    float min_size = get_spacing("huge");
    window_size.x = std::max(window_size.x, min_size * 4.0f);
    window_size.y = std::max(window_size.y, min_size * 2.0f);
    
    return window_size;
}
#endif

// =============================================================================
// TOUCH INTERFACE
// =============================================================================

#ifdef ECSCOPE_HAS_IMGUI
ImVec2 ResponsiveDesignManager::get_touch_button_size() const {
    // Minimum 44pt (iOS) / 48dp (Android) touch target
    float touch_target_size = 44.0f * effective_ui_scale_;
    
    // Adjust based on screen size
    switch (current_screen_size_) {
        case ScreenSize::XSmall:
        case ScreenSize::Small:
            touch_target_size *= 0.9f; // Slightly smaller for phones
            break;
        case ScreenSize::Medium:
            touch_target_size *= 1.1f; // Larger for tablets
            break;
        default:
            // Desktop sizes use standard touch target
            break;
    }
    
    return ImVec2(touch_target_size, touch_target_size);
}
#endif

float ResponsiveDesignManager::get_touch_spacing() const {
    // Touch-friendly spacing should be at least 8pt
    float base_spacing = get_spacing("medium");
    return std::max(base_spacing, 8.0f * effective_ui_scale_);
}

void ResponsiveDesignManager::set_touch_mode(TouchMode mode) {
    if (current_touch_mode_ != mode) {
        TouchMode old_mode = current_touch_mode_;
        current_touch_mode_ = mode;
        
        core::Log::info("ResponsiveDesignManager: Touch mode changed from {} to {}", 
                       static_cast<int>(old_mode), static_cast<int>(mode));
    }
}

// =============================================================================
// RESPONSIVE COMPONENTS
// =============================================================================

#ifdef ECSCOPE_HAS_IMGUI
bool ResponsiveDesignManager::begin_responsive_window(const char* name, bool* p_open, ImGuiWindowFlags flags) {
    layout_state_.in_responsive_window = true;
    
    // Apply responsive window flags
    if (is_screen_at_most(ScreenSize::Small)) {
        // On small screens, windows should be more constrained
        flags |= ImGuiWindowFlags_NoResize;
    }
    
    if (is_touch_enabled()) {
        // Touch-friendly window behavior
        ImGuiStyle& style = ImGui::GetStyle();
        style.TouchExtraPadding = scale(ImVec2(4.0f, 4.0f));
    }
    
    return ImGui::Begin(name, p_open, flags);
}

void ResponsiveDesignManager::end_responsive_window() {
    if (layout_state_.in_responsive_window) {
        ImGui::End();
        layout_state_.in_responsive_window = false;
    }
}

bool ResponsiveDesignManager::responsive_button(const char* label, const ImVec2& size_hint) {
    ImVec2 button_size = size_hint;
    
    if (size_hint.x <= 0.0f || size_hint.y <= 0.0f) {
        // Auto-calculate responsive button size
        if (is_touch_enabled()) {
            button_size = get_touch_button_size();
        } else {
            // Standard button size with responsive scaling
            button_size = scale(ImVec2(120.0f, 0.0f)); // Let ImGui calculate height
        }
    } else {
        button_size = scale(size_hint);
    }
    
    return ImGui::Button(label, button_size);
}

bool ResponsiveDesignManager::responsive_selectable(const char* label, bool selected, ImGuiSelectableFlags flags) {
    ImVec2 size(0, 0); // Auto-size by default
    
    if (is_touch_enabled()) {
        // Ensure touch-friendly height
        ImVec2 touch_size = get_touch_button_size();
        size.y = touch_size.y;
    }
    
    return ImGui::Selectable(label, selected, flags, size);
}
#endif

void ResponsiveDesignManager::begin_responsive_group() {
#ifdef ECSCOPE_HAS_IMGUI
    layout_state_.in_responsive_group = true;
    
    // Apply responsive group styling
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, get_spacing_vec2("medium", "small"));
    ImGui::BeginGroup();
#endif
}

void ResponsiveDesignManager::end_responsive_group() {
#ifdef ECSCOPE_HAS_IMGUI
    if (layout_state_.in_responsive_group) {
        ImGui::EndGroup();
        ImGui::PopStyleVar();
        layout_state_.in_responsive_group = false;
    }
#endif
}

// =============================================================================
// ADAPTIVE LAYOUTS
// =============================================================================

void ResponsiveDesignManager::begin_adaptive_columns(int base_columns, int max_columns) {
#ifdef ECSCOPE_HAS_IMGUI
    layout_state_.in_adaptive_columns = true;
    
    float available_width = ImGui::GetContentRegionAvail().x;
    int adaptive_columns = calculate_adaptive_columns(base_columns, max_columns, available_width);
    layout_state_.current_columns = adaptive_columns;
    
    ImGui::Columns(adaptive_columns, nullptr, false);
#endif
}

void ResponsiveDesignManager::end_adaptive_columns() {
#ifdef ECSCOPE_HAS_IMGUI
    if (layout_state_.in_adaptive_columns) {
        ImGui::Columns(1);
        layout_state_.in_adaptive_columns = false;
        layout_state_.current_columns = 1;
    }
#endif
}

int ResponsiveDesignManager::calculate_adaptive_columns(int base_columns, int max_columns, float available_width) const {
    // Minimum column width for readability
    float min_column_width = 200.0f * effective_ui_scale_;
    
    // Adjust based on screen size
    switch (current_screen_size_) {
        case ScreenSize::XSmall:
            min_column_width = 150.0f * effective_ui_scale_;
            break;
        case ScreenSize::Small:
            min_column_width = 180.0f * effective_ui_scale_;
            break;
        case ScreenSize::Medium:
            min_column_width = 220.0f * effective_ui_scale_;
            break;
        default:
            break;
    }
    
    int max_possible_columns = static_cast<int>(available_width / min_column_width);
    
    if (max_columns < 0) {
        max_columns = base_columns * 2; // Default max
    }
    
    return std::clamp(max_possible_columns, 1, std::min(base_columns, max_columns));
}

void ResponsiveDesignManager::begin_responsive_flex(bool horizontal) {
    layout_state_.in_responsive_flex = true;
    layout_state_.flex_items.clear();
    layout_state_.flex_total_grow = 0.0f;
    
    // Store flex direction for later use
    // In ImGui, we'll simulate flex layout using Child windows
}

void ResponsiveDesignManager::responsive_flex_item(float flex_grow) {
    layout_state_.flex_items.push_back(flex_grow);
    layout_state_.flex_total_grow += flex_grow;
}

void ResponsiveDesignManager::end_responsive_flex() {
    layout_state_.in_responsive_flex = false;
    layout_state_.flex_items.clear();
    layout_state_.flex_total_grow = 0.0f;
}

// =============================================================================
// PRIVATE METHODS
// =============================================================================

void ResponsiveDesignManager::update_display_info() {
#ifdef ECSCOPE_HAS_GLFW
    if (!window_) return;
    
    // Get current window size
    int width, height;
    glfwGetWindowSize(window_, &width, &height);
    
    // Update primary display info if window size changed significantly
    if (std::abs(primary_display_.width - width) > 50 || 
        std::abs(primary_display_.height - height) > 50) {
        
        primary_display_.width = width;
        primary_display_.height = height;
        
        // Get DPI scale from window's monitor
        GLFWmonitor* monitor = glfwGetWindowMonitor(window_);
        if (!monitor) {
            monitor = glfwGetPrimaryMonitor(); // Fallback to primary
        }
        
        if (monitor) {
            float xscale, yscale;
            glfwGetMonitorContentScale(monitor, &xscale, &yscale);
            primary_display_.dpi_scale = std::max(xscale, yscale);
            primary_display_.dpi_category = calculate_dpi_category(primary_display_.dpi_scale);
            primary_display_.screen_size = calculate_screen_size(width, height);
        }
    }
#endif
}

void ResponsiveDesignManager::update_screen_size() {
    previous_screen_size_ = current_screen_size_;
    current_screen_size_ = calculate_screen_size(primary_display_.width, primary_display_.height);
}

void ResponsiveDesignManager::update_dpi_scaling() {
    previous_dpi_scale_ = current_dpi_scale_;
    
    if (config_.auto_dpi_scaling) {
        current_dpi_scale_ = primary_display_.dpi_scale;
    } else {
        current_dpi_scale_ = 1.0f; // Manual scaling only
    }
    
    effective_ui_scale_ = calculate_effective_scale();
}

void ResponsiveDesignManager::update_touch_detection() {
    if (config_.touch_mode == TouchMode::Auto) {
        // Auto-detect touch based on screen size and DPI
        // Mobile-sized screens are likely touch-enabled
        bool likely_touch = (current_screen_size_ <= ScreenSize::Medium) && 
                           (current_dpi_scale_ >= 1.5f);
        
        TouchMode detected_mode = likely_touch ? TouchMode::Enabled : TouchMode::Disabled;
        if (detected_mode != current_touch_mode_) {
            set_touch_mode(detected_mode);
        }
    } else {
        current_touch_mode_ = config_.touch_mode;
    }
}

void ResponsiveDesignManager::notify_screen_size_change(ScreenSize old_size, ScreenSize new_size) {
    core::Log::info("ResponsiveDesignManager: Screen size changed from {} to {}", 
                   static_cast<int>(old_size), static_cast<int>(new_size));
    
    for (auto& callback : screen_size_callbacks_) {
        callback(old_size, new_size);
    }
}

void ResponsiveDesignManager::notify_dpi_scale_change(float old_scale, float new_scale) {
    core::Log::info("ResponsiveDesignManager: DPI scale changed from {:.2f} to {:.2f}", old_scale, new_scale);
    
    for (auto& callback : dpi_scale_callbacks_) {
        callback(old_scale, new_scale);
    }
}

ScreenSize ResponsiveDesignManager::calculate_screen_size(int width, int height) const {
    // Use the larger dimension for screen size classification
    int size = std::max(width, height);
    
    if (size < 480) return ScreenSize::XSmall;
    if (size < 768) return ScreenSize::Small;
    if (size < 1024) return ScreenSize::Medium;
    if (size < 1440) return ScreenSize::Large;
    if (size < 1920) return ScreenSize::XLarge;
    return ScreenSize::XXLarge;
}

DPICategory ResponsiveDesignManager::calculate_dpi_category(float dpi_scale) const {
    if (dpi_scale >= 3.0f) return DPICategory::Ultra;
    if (dpi_scale >= 2.0f) return DPICategory::VeryHigh;
    if (dpi_scale >= 1.5f) return DPICategory::High;
    return DPICategory::Standard;
}

float ResponsiveDesignManager::calculate_effective_scale() const {
    return current_dpi_scale_ * user_ui_scale_;
}

void ResponsiveDesignManager::setup_responsive_fonts() {
    load_responsive_fonts();
}

void ResponsiveDesignManager::setup_responsive_spacing() {
    // Spacing is already initialized in constructor
}

#ifdef ECSCOPE_HAS_GLFW
void ResponsiveDesignManager::monitor_callback(GLFWmonitor* monitor, int event) {
    if (g_responsive_manager) {
        if (event == GLFW_CONNECTED) {
            core::Log::info("ResponsiveDesignManager: Monitor connected: {}", glfwGetMonitorName(monitor));
        } else if (event == GLFW_DISCONNECTED) {
            core::Log::info("ResponsiveDesignManager: Monitor disconnected");
        }
        
        // Re-detect displays on monitor changes
        g_responsive_manager->detect_displays();
    }
}
#endif

void ResponsiveDesignManager::add_screen_size_callback(std::function<void(ScreenSize, ScreenSize)> callback) {
    screen_size_callbacks_.push_back(std::move(callback));
}

void ResponsiveDesignManager::add_dpi_scale_callback(std::function<void(float, float)> callback) {
    dpi_scale_callbacks_.push_back(std::move(callback));
}

void ResponsiveDesignManager::set_config(const ResponsiveConfig& config) {
    config_ = config;
    
    // Re-apply configuration changes
    update_dpi_scaling();
    update_touch_detection();
    
    core::Log::info("ResponsiveDesignManager: Configuration updated");
}

// =============================================================================
// RESPONSIVE WIDGET IMPLEMENTATION
// =============================================================================

ResponsiveWidget::ResponsiveWidget(ResponsiveDesignManager* manager) 
    : manager_(manager) {
    if (!manager_) {
        manager_ = GetResponsiveDesignManager();
    }
}

#ifdef ECSCOPE_HAS_IMGUI
bool ResponsiveWidget::adaptive_layout(const std::vector<std::function<void()>>& widgets, int min_columns, int max_columns) {
    if (!manager_ || widgets.empty()) return false;
    
    manager_->begin_adaptive_columns(min_columns, max_columns);
    
    for (size_t i = 0; i < widgets.size(); ++i) {
        widgets[i]();
        if (i < widgets.size() - 1) {
            ImGui::NextColumn();
        }
    }
    
    manager_->end_adaptive_columns();
    return true;
}

void ResponsiveWidget::responsive_text(const char* text, bool centered) {
    if (centered) {
        float window_width = ImGui::GetWindowSize().x;
        float text_width = ImGui::CalcTextSize(text).x;
        ImGui::SetCursorPosX((window_width - text_width) * 0.5f);
    }
    
    ImGui::TextWrapped("%s", text);
}

bool ResponsiveWidget::responsive_input_text(const char* label, char* buf, size_t buf_size) {
    if (manager_ && manager_->is_touch_enabled()) {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, manager_->get_spacing_vec2("medium", "medium"));
    }
    
    bool result = ImGui::InputText(label, buf, buf_size);
    
    if (manager_ && manager_->is_touch_enabled()) {
        ImGui::PopStyleVar();
    }
    
    return result;
}

bool ResponsiveWidget::responsive_combo(const char* label, int* current_item, const char* const items[], int items_count) {
    if (manager_ && manager_->is_touch_enabled()) {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, manager_->get_spacing_vec2("medium", "medium"));
    }
    
    bool result = ImGui::Combo(label, current_item, items, items_count);
    
    if (manager_ && manager_->is_touch_enabled()) {
        ImGui::PopStyleVar();
    }
    
    return result;
}

void ResponsiveWidget::responsive_separator() {
    if (manager_) {
        ImGui::Dummy(ImVec2(0.0f, manager_->get_spacing("small")));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0.0f, manager_->get_spacing("small")));
    } else {
        ImGui::Separator();
    }
}

void ResponsiveWidget::responsive_spacing() {
    if (manager_) {
        ImGui::Dummy(ImVec2(0.0f, manager_->get_spacing()));
    } else {
        ImGui::Spacing();
    }
}
#endif

// =============================================================================
// SCOPED RESPONSIVE LAYOUT IMPLEMENTATION
// =============================================================================

ScopedResponsiveLayout::ScopedResponsiveLayout(ResponsiveDesignManager* manager, Type type, const std::string& params) 
    : manager_(manager), type_(type), valid_(false) {
    
    if (!manager_) return;
    
    switch (type_) {
        case Type::Window:
            // Window parameters would be parsed from params
            valid_ = true; // Placeholder
            break;
        case Type::Group:
            manager_->begin_responsive_group();
            valid_ = true;
            break;
        case Type::Columns:
            // Parse column count from params
            manager_->begin_adaptive_columns(2, 4); // Default values
            valid_ = true;
            break;
        case Type::Flex:
            manager_->begin_responsive_flex(true); // Default horizontal
            valid_ = true;
            break;
    }
}

ScopedResponsiveLayout::~ScopedResponsiveLayout() {
    if (!manager_ || !valid_) return;
    
    switch (type_) {
        case Type::Window:
            manager_->end_responsive_window();
            break;
        case Type::Group:
            manager_->end_responsive_group();
            break;
        case Type::Columns:
            manager_->end_adaptive_columns();
            break;
        case Type::Flex:
            manager_->end_responsive_flex();
            break;
    }
}

// =============================================================================
// RESPONSIVE STYLE PRESETS IMPLEMENTATION
// =============================================================================

void ResponsiveStylePresets::apply_dashboard_mobile_style() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Mobile-optimized dashboard style
    style.WindowPadding = ImVec2(8.0f, 8.0f);
    style.FramePadding = ImVec2(6.0f, 6.0f);
    style.ItemSpacing = ImVec2(6.0f, 4.0f);
    style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
    style.TouchExtraPadding = ImVec2(4.0f, 4.0f);
    
    // Larger touch targets
    style.FrameRounding = 4.0f;
    style.GrabMinSize = 16.0f;
    style.ScrollbarSize = 20.0f;
#endif
}

void ResponsiveStylePresets::apply_dashboard_tablet_style() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Tablet-optimized dashboard style
    style.WindowPadding = ImVec2(12.0f, 12.0f);
    style.FramePadding = ImVec2(8.0f, 6.0f);
    style.ItemSpacing = ImVec2(8.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
    style.TouchExtraPadding = ImVec2(2.0f, 2.0f);
    
    style.FrameRounding = 6.0f;
    style.GrabMinSize = 14.0f;
    style.ScrollbarSize = 18.0f;
#endif
}

void ResponsiveStylePresets::apply_dashboard_desktop_style() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Desktop-optimized dashboard style
    style.WindowPadding = ImVec2(16.0f, 16.0f);
    style.FramePadding = ImVec2(8.0f, 4.0f);
    style.ItemSpacing = ImVec2(8.0f, 4.0f);
    style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
    style.TouchExtraPadding = ImVec2(0.0f, 0.0f);
    
    style.FrameRounding = 3.0f;
    style.GrabMinSize = 12.0f;
    style.ScrollbarSize = 16.0f;
#endif
}

void ResponsiveStylePresets::apply_touch_friendly_style() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Touch-friendly adjustments
    style.TouchExtraPadding = ImVec2(6.0f, 6.0f);
    style.FramePadding.x = std::max(style.FramePadding.x, 10.0f);
    style.FramePadding.y = std::max(style.FramePadding.y, 8.0f);
    style.ItemSpacing.y = std::max(style.ItemSpacing.y, 8.0f);
    
    // Larger interactive elements
    style.GrabMinSize = std::max(style.GrabMinSize, 20.0f);
    style.ScrollbarSize = std::max(style.ScrollbarSize, 24.0f);
    style.FrameRounding = std::max(style.FrameRounding, 6.0f);
#endif
}

// =============================================================================
// GLOBAL ACCESS IMPLEMENTATION
// =============================================================================

ResponsiveDesignManager* GetResponsiveDesignManager() {
    return g_responsive_manager.get();
}

bool InitializeGlobalResponsiveDesign(GLFWwindow* window, const ResponsiveConfig& config) {
    if (g_responsive_manager) {
        core::Log::warning("Global responsive design manager already initialized");
        return true;
    }
    
    g_responsive_manager = std::make_unique<ResponsiveDesignManager>();
    return g_responsive_manager->initialize(window, config);
}

void ShutdownGlobalResponsiveDesign() {
    if (g_responsive_manager) {
        g_responsive_manager->shutdown();
        g_responsive_manager.reset();
    }
}

} // namespace ecscope::gui