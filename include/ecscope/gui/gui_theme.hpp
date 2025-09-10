/**
 * @file gui_theme.hpp
 * @brief Theme and Styling System for GUI Framework
 * 
 * Comprehensive theming system with customizable colors, fonts, spacing,
 * animations, and visual effects for professional GUI appearance.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#pragma once

#include "gui_core.hpp"
#include "gui_text.hpp"
#include <unordered_map>
#include <functional>
#include <variant>

namespace ecscope::gui {

// =============================================================================
// COLOR SYSTEM
// =============================================================================

/**
 * @brief Standard GUI color indices
 */
enum class GuiColor : uint32_t {
    // Window colors
    WindowBackground,
    WindowBorder,
    WindowTitleBar,
    WindowTitleBarActive,
    WindowTitleBarCollapsed,
    ChildBackground,
    PopupBackground,
    MenuBarBackground,
    
    // Frame colors
    FrameBackground,
    FrameBackgroundHovered,
    FrameBackgroundActive,
    FrameBorder,
    FrameBorderShadow,
    
    // Widget colors
    ButtonBackground,
    ButtonBackgroundHovered,
    ButtonBackgroundActive,
    ButtonBorder,
    ButtonText,
    
    CheckboxBackground,
    CheckboxBackgroundHovered,
    CheckboxBackgroundActive,
    CheckboxBorder,
    CheckboxCheck,
    
    SliderGrab,
    SliderGrabActive,
    SliderTrack,
    SliderTrackHovered,
    
    InputBackground,
    InputBackgroundHovered,
    InputBackgroundActive,
    InputBorder,
    InputText,
    InputTextSelected,
    InputCursor,
    
    // Text colors
    Text,
    TextDisabled,
    TextSelectedBackground,
    TextLink,
    TextLinkHovered,
    
    // Header colors
    Header,
    HeaderHovered,
    HeaderActive,
    
    // Selection colors
    SelectionBackground,
    SelectionBackgroundInactive,
    SelectionBorder,
    
    // Scrollbar colors
    ScrollbarBackground,
    ScrollbarGrab,
    ScrollbarGrabHovered,
    ScrollbarGrabActive,
    
    // Tab colors
    Tab,
    TabHovered,
    TabActive,
    TabUnfocused,
    TabUnfocusedActive,
    TabBorder,
    
    // Table colors
    TableHeaderBackground,
    TableBorderStrong,
    TableBorderLight,
    TableRowBackground,
    TableRowBackgroundAlt,
    
    // Drag and drop
    DragDropTarget,
    
    // Navigation
    NavHighlight,
    NavWindowingHighlight,
    NavWindowingDimBg,
    
    // Modal colors
    ModalWindowDimBg,
    
    // Separator
    Separator,
    SeparatorHovered,
    SeparatorActive,
    
    // Resize grip
    ResizeGrip,
    ResizeGripHovered,
    ResizeGripActive,
    
    // Docking
    DockingPreview,
    DockingEmptyBg,
    
    // Plot colors
    PlotLines,
    PlotLinesHovered,
    PlotHistogram,
    PlotHistogramHovered,
    
    // Error/warning/success
    ErrorText,
    WarningText,
    SuccessText,
    InfoText,
    
    // Custom user colors (extend as needed)
    CustomColor0,
    CustomColor1,
    CustomColor2,
    CustomColor3,
    CustomColor4,
    
    COUNT
};

/**
 * @brief Color palette with theme colors
 */
class ColorPalette {
public:
    ColorPalette();
    
    // Color access
    const Color& get_color(GuiColor color_id) const;
    void set_color(GuiColor color_id, const Color& color);
    
    // Bulk operations
    void set_colors(std::initializer_list<std::pair<GuiColor, Color>> colors);
    void reset_to_defaults();
    
    // Color manipulation
    Color lighten(GuiColor color_id, float amount) const;
    Color darken(GuiColor color_id, float amount) const;
    Color with_alpha(GuiColor color_id, float alpha) const;
    
    // Predefined palettes
    void apply_dark_theme();
    void apply_light_theme();
    void apply_classic_theme();
    void apply_high_contrast_theme();
    void apply_custom_theme(const std::string& theme_name);
    
    // Save/load
    bool save_to_file(const std::string& filename) const;
    bool load_from_file(const std::string& filename);
    std::string serialize() const;
    bool deserialize(const std::string& data);
    
private:
    std::array<Color, static_cast<size_t>(GuiColor::COUNT)> colors_;
    
    void initialize_default_colors();
};

// =============================================================================
// STYLE VARIABLES
// =============================================================================

/**
 * @brief Style variables for spacing, sizing, and visual properties
 */
enum class GuiStyleVar : uint32_t {
    // Float variables
    Alpha,                    // Global alpha applies to everything
    DisabledAlpha,            // Additional alpha multiplier for disabled items
    WindowRounding,           // Radius of window corners rounding
    WindowBorderSize,         // Thickness of border around windows
    WindowMinSize,            // Minimum window size (stored as Vec2)
    WindowTitleAlign,         // Alignment for title bar text (stored as Vec2)
    WindowMenuButtonPosition, // Position of the collapsing/docking button
    ChildRounding,            // Radius of child window corners rounding
    ChildBorderSize,          // Thickness of border around child windows
    PopupRounding,            // Radius of popup window corners rounding
    PopupBorderSize,          // Thickness of border around popup/tooltip windows
    FramePadding,             // Padding within a framed rectangle (stored as Vec2)
    FrameRounding,            // Radius of frame corners rounding
    FrameBorderSize,          // Thickness of border around frames
    ItemSpacing,              // Horizontal and vertical spacing between widgets/lines (stored as Vec2)
    ItemInnerSpacing,         // Horizontal and vertical spacing between elements of a composed widget (stored as Vec2)
    CellPadding,              // Padding within a table cell (stored as Vec2)
    TouchExtraPadding,        // Expand reactive bounding box for touch-based system (stored as Vec2)
    IndentSpacing,            // Horizontal indentation when e.g. entering a tree node
    ColumnsMinSpacing,        // Minimum horizontal spacing between two columns
    ScrollbarSize,            // Width of the vertical scrollbar, Height of the horizontal scrollbar
    ScrollbarRounding,        // Radius of grab corners rounding for scrollbar
    GrabMinSize,              // Minimum width/height of a grab box for slider/scrollbar
    GrabRounding,             // Radius of grabs corners rounding
    LogSliderDeadzone,        // The size in pixels of the dead-zone around zero on logarithmic sliders
    TabRounding,              // Radius of upper corners of a tab
    TabBorderSize,            // Thickness of border around tabs
    TabMinWidthForCloseButton,// Minimum width for close button to appear on an unselected tab
    ColorButtonPosition,      // Side of the color button in the ColorEdit4 widget
    ButtonTextAlign,          // Alignment of button text (stored as Vec2)
    SelectableTextAlign,      // Alignment of selectable text (stored as Vec2)
    
    // Additional spacing
    WindowPadding,            // Padding within a window (stored as Vec2)
    MenuBarHeight,            // Height of menu bar
    StatusBarHeight,          // Height of status bar
    ToolbarHeight,            // Height of toolbar
    
    // Animation
    AnimationSpeed,           // Speed of animations (0.0 = instant, 1.0 = normal)
    FadeSpeed,                // Speed of fade in/out animations
    
    // Font scaling
    FontGlobalScale,          // Global font scale
    
    COUNT
};

/**
 * @brief Style value variant type
 */
using StyleValue = std::variant<float, Vec2, int>;

/**
 * @brief GUI style configuration
 */
class GuiStyle {
public:
    GuiStyle();
    
    // Style variable access
    const StyleValue& get_var(GuiStyleVar var) const;
    void set_var(GuiStyleVar var, const StyleValue& value);
    
    // Convenience getters
    float get_float(GuiStyleVar var) const;
    Vec2 get_vec2(GuiStyleVar var) const;
    int get_int(GuiStyleVar var) const;
    
    // Convenience setters
    void set_float(GuiStyleVar var, float value);
    void set_vec2(GuiStyleVar var, const Vec2& value);
    void set_int(GuiStyleVar var, int value);
    
    // Bulk operations
    void reset_to_defaults();
    void scale_all_sizes(float scale_factor);
    
    // Predefined styles
    void apply_compact_style();
    void apply_spacious_style();
    void apply_minimal_style();
    void apply_gaming_style();
    void apply_professional_style();
    
    // Save/load
    bool save_to_file(const std::string& filename) const;
    bool load_from_file(const std::string& filename);
    std::string serialize() const;
    bool deserialize(const std::string& data);
    
private:
    std::array<StyleValue, static_cast<size_t>(GuiStyleVar::COUNT)> vars_;
    
    void initialize_default_vars();
};

// =============================================================================
// FONT MANAGEMENT
// =============================================================================

/**
 * @brief Font configuration for themes
 */
struct FontConfig {
    std::string name;
    FontAtlas::FontHandle handle = FontAtlas::INVALID_FONT;
    float size = 13.0f;
    FontWeight weight = FontWeight::Normal;
    FontStyle style = FontStyle::Normal;
    bool is_default = false;
    
    bool is_valid() const { return handle != FontAtlas::INVALID_FONT; }
};

/**
 * @brief Font roles in the UI
 */
enum class FontRole : uint8_t {
    Default,
    Title,
    Heading1,
    Heading2,
    Heading3,
    Subtitle,
    Body,
    Caption,
    Code,
    Icon,
    COUNT
};

/**
 * @brief Font manager for themes
 */
class FontManager {
public:
    FontManager();
    
    bool initialize(FontAtlas* font_atlas);
    void shutdown();
    
    // Font role management
    void set_font_for_role(FontRole role, FontAtlas::FontHandle font);
    FontAtlas::FontHandle get_font_for_role(FontRole role) const;
    const FontConfig& get_font_config(FontRole role) const;
    
    // Font loading
    bool load_font_for_role(FontRole role, const std::string& filename, float size,
                           FontWeight weight = FontWeight::Normal, FontStyle style = FontStyle::Normal);
    bool load_fonts_from_config(const std::string& config_file);
    
    // Default fonts
    void setup_default_fonts();
    void setup_icon_font(const std::string& icon_font_path);
    
    // Font scaling
    void scale_all_fonts(float scale_factor);
    void apply_dpi_scaling(float dpi_scale);
    
    // Current font context
    void push_font(FontRole role);
    void push_font(FontAtlas::FontHandle font);
    void pop_font();
    FontAtlas::FontHandle get_current_font() const;
    
private:
    FontAtlas* font_atlas_ = nullptr;
    std::array<FontConfig, static_cast<size_t>(FontRole::COUNT)> font_configs_;
    std::stack<FontAtlas::FontHandle> font_stack_;
    bool initialized_ = false;
};

// =============================================================================
// ANIMATION SYSTEM
// =============================================================================

/**
 * @brief Animation easing functions
 */
enum class EasingType : uint8_t {
    Linear,
    EaseInQuad, EaseOutQuad, EaseInOutQuad,
    EaseInCubic, EaseOutCubic, EaseInOutCubic,
    EaseInQuart, EaseOutQuart, EaseInOutQuart,
    EaseInQuint, EaseOutQuint, EaseInOutQuint,
    EaseInSine, EaseOutSine, EaseInOutSine,
    EaseInExpo, EaseOutExpo, EaseInOutExpo,
    EaseInCirc, EaseOutCirc, EaseInOutCirc,
    EaseInBack, EaseOutBack, EaseInOutBack,
    EaseInElastic, EaseOutElastic, EaseInOutElastic,
    EaseInBounce, EaseOutBounce, EaseInOutBounce
};

/**
 * @brief Animation interpolation helper
 */
class Animation {
public:
    Animation(float duration = 0.3f, EasingType easing = EasingType::EaseOutQuad);
    
    void start(float from_value, float to_value);
    void restart();
    void stop();
    void set_reversed(bool reversed);
    
    float update(float delta_time);
    float get_current_value() const;
    bool is_animating() const;
    bool is_finished() const;
    
    void set_duration(float duration) { duration_ = duration; }
    void set_easing(EasingType easing) { easing_ = easing; }
    void set_loop(bool loop) { loop_ = loop; }
    void set_yoyo(bool yoyo) { yoyo_ = yoyo; }
    
private:
    float duration_;
    float elapsed_time_;
    float from_value_;
    float to_value_;
    float current_value_;
    EasingType easing_;
    bool animating_;
    bool reversed_;
    bool loop_;
    bool yoyo_;
    
    float apply_easing(float t) const;
};

/**
 * @brief Color animation helper
 */
class ColorAnimation {
public:
    ColorAnimation(float duration = 0.3f, EasingType easing = EasingType::EaseOutQuad);
    
    void start(const Color& from_color, const Color& to_color);
    Color update(float delta_time);
    bool is_animating() const { return animation_.is_animating(); }
    
private:
    Animation animation_;
    Color from_color_;
    Color to_color_;
};

/**
 * @brief Vec2 animation helper
 */
class Vec2Animation {
public:
    Vec2Animation(float duration = 0.3f, EasingType easing = EasingType::EaseOutQuad);
    
    void start(const Vec2& from_vec, const Vec2& to_vec);
    Vec2 update(float delta_time);
    bool is_animating() const { return x_animation_.is_animating() || y_animation_.is_animating(); }
    
private:
    Animation x_animation_;
    Animation y_animation_;
    Vec2 from_vec_;
    Vec2 to_vec_;
};

// =============================================================================
// THEME SYSTEM
// =============================================================================

/**
 * @brief Complete theme configuration
 */
struct Theme {
    std::string name;
    std::string author;
    std::string description;
    std::string version;
    
    ColorPalette colors;
    GuiStyle style;
    std::unordered_map<FontRole, std::string> font_paths;
    std::unordered_map<FontRole, float> font_sizes;
    
    // Theme metadata
    std::unordered_map<std::string, std::string> metadata;
    
    bool is_dark_theme() const;
    void set_dark_theme(bool dark);
};

/**
 * @brief Theme manager
 */
class ThemeManager {
public:
    ThemeManager();
    ~ThemeManager();
    
    bool initialize(FontAtlas* font_atlas);
    void shutdown();
    
    // Theme management
    bool load_theme(const std::string& theme_file);
    bool load_theme_from_string(const std::string& theme_data);
    bool save_theme(const std::string& theme_file, const Theme& theme) const;
    
    void register_theme(const std::string& name, const Theme& theme);
    void unregister_theme(const std::string& name);
    
    const Theme* get_theme(const std::string& name) const;
    std::vector<std::string> get_theme_names() const;
    
    // Current theme
    bool apply_theme(const std::string& name);
    const Theme& get_current_theme() const { return current_theme_; }
    const std::string& get_current_theme_name() const { return current_theme_name_; }
    
    // Built-in themes
    void register_builtin_themes();
    Theme create_dark_theme() const;
    Theme create_light_theme() const;
    Theme create_high_contrast_theme() const;
    Theme create_classic_theme() const;
    Theme create_modern_theme() const;
    
    // Theme customization
    void push_color(GuiColor color_id, const Color& color);
    void pop_color();
    void push_style_var(GuiStyleVar var, const StyleValue& value);
    void pop_style_var();
    void push_font(FontRole role);
    void pop_font();
    
    // Live theme editing
    bool begin_theme_editor(const std::string& theme_name);
    void end_theme_editor();
    bool is_theme_editor_open() const { return theme_editor_open_; }
    
    // Animation support
    void enable_animations(bool enable) { animations_enabled_ = enable; }
    bool are_animations_enabled() const { return animations_enabled_; }
    void set_animation_speed(float speed) { animation_speed_ = speed; }
    float get_animation_speed() const { return animation_speed_; }
    
    // Color utilities
    static Color blend_colors(const Color& a, const Color& b, float t);
    static Color adjust_brightness(const Color& color, float amount);
    static Color adjust_saturation(const Color& color, float amount);
    static Color adjust_hue(const Color& color, float degrees);
    static Color to_grayscale(const Color& color);
    static float get_luminance(const Color& color);
    static Color get_contrasting_color(const Color& background);
    
    // Accessibility
    void apply_accessibility_settings(bool high_contrast, bool reduce_motion, float font_scale);
    bool check_color_contrast(const Color& foreground, const Color& background) const;
    
private:
    std::unordered_map<std::string, Theme> themes_;
    Theme current_theme_;
    std::string current_theme_name_;
    FontManager font_manager_;
    
    // Style stacks
    std::stack<Color> color_stack_;
    std::stack<std::pair<GuiColor, Color>> pushed_colors_;
    std::stack<StyleValue> style_var_stack_;
    std::stack<std::pair<GuiStyleVar, StyleValue>> pushed_style_vars_;
    
    // Animation state
    bool animations_enabled_ = true;
    float animation_speed_ = 1.0f;
    
    // Theme editor
    bool theme_editor_open_ = false;
    std::string editing_theme_name_;
    
    bool initialized_ = false;
    
    // Serialization helpers
    std::string serialize_color_palette(const ColorPalette& palette) const;
    bool deserialize_color_palette(const std::string& data, ColorPalette& palette);
    std::string serialize_style(const GuiStyle& style) const;
    bool deserialize_style(const std::string& data, GuiStyle& style);
};

// =============================================================================
// GLOBAL THEME ACCESS
// =============================================================================

/**
 * @brief Get the current theme manager
 */
ThemeManager* get_theme_manager();

/**
 * @brief Get current color from active theme
 */
const Color& get_color(GuiColor color_id);

/**
 * @brief Get current style variable from active theme
 */
const StyleValue& get_style_var(GuiStyleVar var);
float get_style_float(GuiStyleVar var);
Vec2 get_style_vec2(GuiStyleVar var);
int get_style_int(GuiStyleVar var);

/**
 * @brief Theme manipulation helpers
 */
void push_color(GuiColor color_id, const Color& color);
void pop_color();
void push_style_var(GuiStyleVar var, const StyleValue& value);
void push_style_var(GuiStyleVar var, float value);
void push_style_var(GuiStyleVar var, const Vec2& value);
void pop_style_var();

/**
 * @brief RAII helpers for temporary theme changes
 */
class ColorScope {
public:
    ColorScope(GuiColor color_id, const Color& color);
    ~ColorScope();
    
private:
    int count_;
};

class StyleVarScope {
public:
    StyleVarScope(GuiStyleVar var, const StyleValue& value);
    StyleVarScope(GuiStyleVar var, float value);
    StyleVarScope(GuiStyleVar var, const Vec2& value);
    ~StyleVarScope();
    
private:
    int count_;
};

class FontScope {
public:
    FontScope(FontRole role);
    FontScope(FontAtlas::FontHandle font);
    ~FontScope();
    
private:
    bool pushed_;
};

// =============================================================================
// THEME UTILITIES
// =============================================================================

/**
 * @brief Get theme-aware colors for common UI patterns
 */
namespace theme_colors {
    Color button_normal();
    Color button_hovered();
    Color button_active();
    Color button_disabled();
    
    Color input_normal();
    Color input_focused();
    Color input_error();
    
    Color text_primary();
    Color text_secondary();
    Color text_disabled();
    Color text_link();
    
    Color success();
    Color warning();
    Color error();
    Color info();
} // namespace theme_colors

/**
 * @brief Get theme-aware measurements
 */
namespace theme_metrics {
    float button_height();
    float input_height();
    float menu_height();
    float title_bar_height();
    Vec2 button_padding();
    Vec2 frame_padding();
    Vec2 item_spacing();
    float rounding();
    float border_size();
} // namespace theme_metrics

} // namespace ecscope::gui