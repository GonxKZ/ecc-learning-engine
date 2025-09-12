/**
 * @file accessibility_visual.hpp
 * @brief Visual Accessibility and High Contrast Support
 * 
 * Comprehensive visual accessibility system providing high contrast modes,
 * color blindness support, customizable font scaling, visual indicators,
 * and other visual accommodations for users with various visual needs.
 * 
 * Features:
 * - High contrast themes (Windows, custom)
 * - Color blindness simulation and correction
 * - Dynamic font scaling and sizing
 * - Enhanced focus indicators and visual cues
 * - Reduced motion support
 * - Visual notification system
 * - Pattern-based alternatives to color coding
 * - Customizable visual preferences
 * 
 * @author ECScope Development Team  
 * @version 1.0.0
 */

#pragma once

#include "accessibility_core.hpp"
#include "gui_theme.hpp"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace ecscope::gui::accessibility {

// =============================================================================
// VISUAL ACCESSIBILITY ENUMERATIONS
// =============================================================================

/**
 * @brief High contrast mode types
 */
enum class HighContrastMode : uint8_t {
    None,
    Standard,      // Standard high contrast (black on white)
    Inverted,      // Inverted high contrast (white on black)
    Custom,        // Custom high contrast colors
    Windows,       // Windows high contrast theme
    Enhanced,      // Enhanced contrast with colors
    Monochrome     // Pure black and white
};

/**
 * @brief Visual indicator types
 */
enum class VisualIndicatorType : uint8_t {
    Focus,
    Hover,
    Active,
    Selected,
    Disabled,
    Error,
    Warning,
    Success,
    Information
};

/**
 * @brief Pattern types for color alternatives
 */
enum class PatternType : uint8_t {
    None,
    Dots,
    Stripes,
    DiagonalLines,
    Grid,
    Checkerboard,
    Crosshatch,
    Solid,
    Dashed,
    Custom
};

/**
 * @brief Animation preference levels
 */
enum class MotionPreference : uint8_t {
    Full,       // All animations enabled
    Reduced,    // Essential animations only
    Minimal,    // Only critical feedback animations
    None        // No animations at all
};

// =============================================================================
// VISUAL ACCESSIBILITY STRUCTURES
// =============================================================================

/**
 * @brief High contrast color scheme
 */
struct HighContrastScheme {
    std::string name;
    Color background = Color(0, 0, 0, 255);         // Black
    Color foreground = Color(255, 255, 255, 255);   // White
    Color accent = Color(0, 120, 215, 255);         // Blue
    Color disabled = Color(128, 128, 128, 255);     // Gray
    Color selection = Color(0, 120, 215, 255);      // Blue
    Color warning = Color(255, 255, 0, 255);        // Yellow
    Color error = Color(255, 0, 0, 255);            // Red
    Color success = Color(0, 255, 0, 255);          // Green
    Color information = Color(0, 255, 255, 255);    // Cyan
    Color border = Color(255, 255, 255, 255);       // White
    
    // Minimum contrast ratios
    float text_contrast_ratio = 7.0f;       // WCAG AAA
    float ui_contrast_ratio = 4.5f;         // WCAG AA
    
    bool validate_contrast() const;
    void adjust_for_minimum_contrast();
};

/**
 * @brief Color blindness simulation parameters
 */
struct ColorBlindnessSimulation {
    ColorBlindnessType type = ColorBlindnessType::None;
    float severity = 1.0f;  // 0.0 = no effect, 1.0 = full effect
    
    // Correction parameters
    bool enable_correction = false;
    float correction_strength = 0.5f;
    
    // Simulation matrices for different types
    static const float protanopia_matrix[9];
    static const float deuteranopia_matrix[9];
    static const float tritanopia_matrix[9];
    static const float protanomaly_matrix[9];
    static const float deuteranomaly_matrix[9];
    static const float tritanomaly_matrix[9];
    static const float achromatopsia_matrix[9];
};

/**
 * @brief Font accessibility settings
 */
struct FontAccessibilitySettings {
    float base_scale = 1.0f;                    // Overall font scaling
    float minimum_size = 12.0f;                 // Minimum readable size
    float maximum_size = 72.0f;                 // Maximum practical size
    float line_height_multiplier = 1.2f;        // Line height adjustment
    float letter_spacing = 0.0f;                // Additional letter spacing
    float word_spacing = 0.0f;                  // Additional word spacing
    
    // Font preferences
    bool prefer_sans_serif = false;             // Sans-serif over serif
    bool prefer_monospace = false;              // Monospace fonts
    bool avoid_thin_fonts = true;               // Avoid thin font weights
    bool prefer_high_contrast_fonts = false;    // Fonts designed for accessibility
    
    // Dyslexia-friendly options
    bool dyslexia_friendly = false;             // Use dyslexia-friendly fonts
    bool increase_character_spacing = false;    // Wider character spacing
    bool avoid_italics = false;                 // Avoid italic text
    bool highlight_capitals = false;            // Different styling for capitals
};

/**
 * @brief Visual feedback settings
 */
struct VisualFeedbackSettings {
    // Focus indicators
    float focus_thickness = 2.0f;
    Color focus_color = Color(0, 120, 215, 255);
    bool animated_focus = true;
    bool high_contrast_focus = false;
    
    // Hover effects
    bool enable_hover_effects = true;
    float hover_brightness_change = 0.1f;
    bool animated_hover = true;
    
    // Selection highlighting
    Color selection_background = Color(0, 120, 215, 77);
    Color selection_foreground = Color(255, 255, 255, 255);
    bool high_contrast_selection = false;
    
    // Status indicators
    bool use_patterns_for_status = false;
    bool use_shapes_for_status = true;
    bool use_animations_for_status = true;
    
    // Visual notifications
    bool flash_on_error = true;
    bool border_on_warning = true;
    bool glow_on_success = false;
    float notification_duration = 3.0f;
};

/**
 * @brief Motion and animation settings
 */
struct MotionSettings {
    MotionPreference preference = MotionPreference::Full;
    
    // Animation timing
    float animation_speed_multiplier = 1.0f;    // Speed adjustment
    float max_animation_duration = 2.0f;        // Maximum duration
    float min_animation_duration = 0.1f;        // Minimum duration
    
    // Specific animation types
    bool enable_fade_animations = true;
    bool enable_slide_animations = true;
    bool enable_scale_animations = true;
    bool enable_rotate_animations = false;
    bool enable_bounce_animations = false;
    
    // Motion triggers
    bool reduce_parallax = true;
    bool reduce_zoom_animations = true;
    bool reduce_auto_scroll = true;
    bool disable_video_autoplay = true;
};

/**
 * @brief Pattern configuration for visual alternatives
 */
struct PatternConfig {
    PatternType type = PatternType::None;
    float density = 0.5f;           // Pattern density (0.0 - 1.0)
    float thickness = 1.0f;         // Line/dot thickness
    Color primary_color = Color(255, 255, 255, 255);
    Color secondary_color = Color(0, 0, 0, 255);
    float rotation = 0.0f;          // Pattern rotation in degrees
    Vec2 scale = Vec2(1.0f, 1.0f);  // Pattern scaling
};

// =============================================================================
// VISUAL ACCESSIBILITY MANAGER
// =============================================================================

/**
 * @brief Visual accessibility features manager
 */
class VisualAccessibilityManager {
public:
    VisualAccessibilityManager();
    ~VisualAccessibilityManager();

    // =============================================================================
    // INITIALIZATION & LIFECYCLE
    // =============================================================================

    bool initialize(AccessibilityContext* accessibility_context, ThemeManager* theme_manager);
    void shutdown();
    void update(float delta_time);

    // =============================================================================
    // HIGH CONTRAST SUPPORT
    // =============================================================================

    void enable_high_contrast(HighContrastMode mode = HighContrastMode::Standard);
    void disable_high_contrast();
    bool is_high_contrast_enabled() const;
    HighContrastMode get_high_contrast_mode() const;
    
    void register_high_contrast_scheme(const std::string& name, const HighContrastScheme& scheme);
    void apply_high_contrast_scheme(const std::string& name);
    std::vector<std::string> get_available_schemes() const;
    const HighContrastScheme& get_current_scheme() const;
    
    void create_windows_high_contrast_scheme();
    void create_custom_high_contrast_scheme(const HighContrastScheme& scheme);

    // =============================================================================
    // COLOR BLINDNESS SUPPORT
    // =============================================================================

    void set_color_blindness_type(ColorBlindnessType type, float severity = 1.0f);
    ColorBlindnessType get_color_blindness_type() const;
    float get_color_blindness_severity() const;
    
    void enable_color_blindness_correction(bool enable, float strength = 0.5f);
    bool is_color_blindness_correction_enabled() const;
    
    Color simulate_color_blindness(const Color& original) const;
    Color correct_color_blindness(const Color& original) const;
    std::vector<Color> generate_accessible_color_palette(size_t count) const;

    // =============================================================================
    // FONT ACCESSIBILITY
    // =============================================================================

    void set_font_accessibility_settings(const FontAccessibilitySettings& settings);
    const FontAccessibilitySettings& get_font_accessibility_settings() const;
    
    void set_font_scale(float scale);
    float get_font_scale() const;
    float calculate_accessible_font_size(float base_size) const;
    
    void enable_dyslexia_friendly_fonts(bool enable);
    bool are_dyslexia_friendly_fonts_enabled() const;
    
    std::vector<std::string> get_accessible_fonts() const;
    void load_accessibility_fonts();

    // =============================================================================
    // VISUAL FEEDBACK ENHANCEMENT
    // =============================================================================

    void set_visual_feedback_settings(const VisualFeedbackSettings& settings);
    const VisualFeedbackSettings& get_visual_feedback_settings() const;
    
    void enhance_focus_indicators(bool enhance);
    void set_focus_indicator_style(const Color& color, float thickness, bool animated);
    void render_enhanced_focus_indicator(DrawList* draw_list, const Rect& bounds);
    
    void enhance_hover_effects(bool enhance);
    void render_enhanced_hover_effect(DrawList* draw_list, const Rect& bounds, float intensity);
    
    void enhance_selection_highlighting(bool enhance);
    void render_enhanced_selection(DrawList* draw_list, const Rect& bounds);

    // =============================================================================
    // MOTION AND ANIMATION CONTROL
    // =============================================================================

    void set_motion_preferences(const MotionSettings& settings);
    const MotionSettings& get_motion_preferences() const;
    
    void set_motion_preference_level(MotionPreference preference);
    MotionPreference get_motion_preference_level() const;
    
    bool should_animate(const std::string& animation_type) const;
    float get_adjusted_animation_duration(float original_duration) const;
    float get_adjusted_animation_speed(float original_speed) const;
    
    void disable_problematic_animations();
    void enable_essential_animations_only();

    // =============================================================================
    // PATTERN-BASED VISUAL ALTERNATIVES
    // =============================================================================

    void enable_pattern_alternatives(bool enable);
    bool are_pattern_alternatives_enabled() const;
    
    void set_pattern_for_color(const Color& color, const PatternConfig& pattern);
    PatternConfig get_pattern_for_color(const Color& color) const;
    
    void render_pattern(DrawList* draw_list, const Rect& bounds, const PatternConfig& pattern);
    void render_patterned_rect(DrawList* draw_list, const Rect& bounds, const Color& color);
    
    std::vector<PatternConfig> generate_distinguishable_patterns(size_t count) const;

    // =============================================================================
    // VISUAL NOTIFICATIONS
    // =============================================================================

    void show_visual_notification(const std::string& message, VisualIndicatorType type, 
                                 float duration = 3.0f, GuiID widget_id = 0);
    void flash_screen(const Color& color, float duration = 0.2f);
    void highlight_widget(GuiID widget_id, const Color& color, float duration = 1.0f);
    void draw_attention_to_widget(GuiID widget_id, const std::string& reason = "");
    
    void enable_visual_error_indication(bool enable);
    void enable_visual_warning_indication(bool enable);
    void enable_visual_success_indication(bool enable);

    // =============================================================================
    // CONTRAST ANALYSIS AND VALIDATION
    // =============================================================================

    ContrastInfo analyze_contrast(const Color& foreground, const Color& background) const;
    bool meets_wcag_aa_contrast(const Color& foreground, const Color& background) const;
    bool meets_wcag_aaa_contrast(const Color& foreground, const Color& background) const;
    
    Color adjust_for_minimum_contrast(const Color& foreground, const Color& background, 
                                     float minimum_ratio = 4.5f) const;
    std::vector<Color> suggest_accessible_colors(const Color& base_color, bool is_background = false) const;
    
    void validate_theme_accessibility();
    std::vector<std::string> get_contrast_issues() const;

    // =============================================================================
    // VISUAL DEBUGGING TOOLS
    // =============================================================================

    void enable_contrast_checker_overlay(bool enable);
    void enable_color_blindness_simulator(bool enable);
    void enable_focus_indicator_overlay(bool enable);
    
    void render_accessibility_overlay(DrawList* draw_list);
    void render_contrast_information(DrawList* draw_list, const Vec2& position);
    void render_color_blindness_preview(DrawList* draw_list, const Rect& preview_area);

    // =============================================================================
    // SYSTEM INTEGRATION
    // =============================================================================

    void detect_system_accessibility_settings();
    void apply_system_high_contrast_theme();
    void sync_with_system_preferences();
    
    bool is_system_high_contrast_enabled() const;
    MotionPreference get_system_motion_preference() const;
    float get_system_text_scale() const;

    // =============================================================================
    // DEBUGGING & DIAGNOSTICS
    // =============================================================================

    struct VisualAccessibilityStats {
        bool high_contrast_enabled = false;
        HighContrastMode high_contrast_mode = HighContrastMode::None;
        ColorBlindnessType color_blindness_type = ColorBlindnessType::None;
        float font_scale = 1.0f;
        MotionPreference motion_preference = MotionPreference::Full;
        bool pattern_alternatives_enabled = false;
        size_t contrast_violations = 0;
        size_t accessible_colors_generated = 0;
        bool dyslexia_fonts_enabled = false;
    };

    VisualAccessibilityStats get_stats() const;
    void render_debug_panel(DrawList* draw_list);
    std::string generate_visual_accessibility_report() const;

private:
    // Core components
    AccessibilityContext* accessibility_context_ = nullptr;
    ThemeManager* theme_manager_ = nullptr;
    
    // High contrast
    bool high_contrast_enabled_ = false;
    HighContrastMode high_contrast_mode_ = HighContrastMode::None;
    std::unordered_map<std::string, HighContrastScheme> high_contrast_schemes_;
    std::string current_scheme_name_;
    HighContrastScheme current_scheme_;
    
    // Color blindness
    ColorBlindnessSimulation color_blindness_settings_;
    
    // Font accessibility
    FontAccessibilitySettings font_settings_;
    std::vector<std::string> accessible_font_names_;
    
    // Visual feedback
    VisualFeedbackSettings visual_feedback_;
    
    // Motion settings
    MotionSettings motion_settings_;
    
    // Pattern alternatives
    bool pattern_alternatives_enabled_ = false;
    std::unordered_map<uint32_t, PatternConfig> color_patterns_;  // Color hash -> pattern
    
    // Visual notifications
    struct VisualNotification {
        std::string message;
        VisualIndicatorType type;
        std::chrono::steady_clock::time_point end_time;
        GuiID widget_id;
        Color color;
        bool active;
    };
    std::vector<VisualNotification> active_notifications_;
    
    // Debugging overlays
    bool contrast_checker_overlay_ = false;
    bool color_blindness_simulator_overlay_ = false;
    bool focus_indicator_overlay_ = false;
    
    // Statistics
    mutable VisualAccessibilityStats stats_;
    std::vector<std::string> contrast_issues_;
    
    // Helper methods
    void apply_high_contrast_to_theme();
    void restore_original_theme();
    Color apply_color_blindness_matrix(const Color& color, const float matrix[9]) const;
    PatternConfig generate_pattern_for_color(const Color& color) const;
    float calculate_luminance(const Color& color) const;
    float calculate_contrast_ratio(float luminance1, float luminance2) const;
    void update_visual_notifications(float delta_time);
    void render_pattern_internal(DrawList* draw_list, const Rect& bounds, const PatternConfig& pattern);
    void detect_windows_high_contrast();
    void detect_system_reduced_motion();
    
    bool initialized_ = false;
};

// =============================================================================
// VISUAL ACCESSIBILITY UTILITIES
// =============================================================================

/**
 * @brief Utilities for visual accessibility
 */
namespace visual_utils {
    /**
     * @brief Create standard high contrast schemes
     */
    HighContrastScheme create_standard_high_contrast();
    HighContrastScheme create_inverted_high_contrast();
    HighContrastScheme create_windows_high_contrast();
    
    /**
     * @brief Generate accessible color palettes
     */
    std::vector<Color> generate_wcag_compliant_palette(const Color& base_color, size_t count);
    std::vector<Color> generate_color_blind_friendly_palette(size_t count, ColorBlindnessType type);
    
    /**
     * @brief Pattern generation utilities
     */
    void render_dot_pattern(DrawList* draw_list, const Rect& bounds, const PatternConfig& config);
    void render_stripe_pattern(DrawList* draw_list, const Rect& bounds, const PatternConfig& config);
    void render_diagonal_pattern(DrawList* draw_list, const Rect& bounds, const PatternConfig& config);
    void render_grid_pattern(DrawList* draw_list, const Rect& bounds, const PatternConfig& config);
    void render_checkerboard_pattern(DrawList* draw_list, const Rect& bounds, const PatternConfig& config);
    void render_crosshatch_pattern(DrawList* draw_list, const Rect& bounds, const PatternConfig& config);
    
    /**
     * @brief Font accessibility utilities
     */
    std::vector<std::string> get_dyslexia_friendly_fonts();
    std::vector<std::string> get_high_readability_fonts();
    float calculate_optimal_line_height(float font_size);
    
    /**
     * @brief Motion utilities
     */
    bool should_reduce_motion(MotionPreference preference, const std::string& animation_type);
    float calculate_accessible_animation_duration(float original, MotionPreference preference);
}

// =============================================================================
// GLOBAL VISUAL ACCESSIBILITY MANAGER
// =============================================================================

/**
 * @brief Get the global visual accessibility manager
 */
VisualAccessibilityManager* get_visual_accessibility_manager();

/**
 * @brief Initialize global visual accessibility support
 */
bool initialize_visual_accessibility();

/**
 * @brief Shutdown global visual accessibility support
 */
void shutdown_visual_accessibility();

} // namespace ecscope::gui::accessibility