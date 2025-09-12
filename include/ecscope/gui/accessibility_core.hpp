/**
 * @file accessibility_core.hpp
 * @brief ECScope Accessibility Framework - Core System
 * 
 * Comprehensive accessibility framework providing WCAG 2.1 Level AA/AAA compliance,
 * screen reader support, keyboard navigation, high contrast modes, and inclusive
 * design patterns for professional game development tools.
 * 
 * Features:
 * - WCAG 2.1 AA compliance validation
 * - Screen reader compatibility (NVDA, JAWS, VoiceOver)
 * - Advanced keyboard navigation and focus management
 * - High contrast and visual accommodation modes
 * - Motor disability accommodations
 * - Color blindness support
 * - Customizable accessibility preferences
 * - Accessibility testing automation
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#pragma once

#include "gui_core.hpp"
#include "gui_theme.hpp"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>
#include <chrono>
#include <bitset>

namespace ecscope::gui::accessibility {

// =============================================================================
// ACCESSIBILITY ENUMERATIONS
// =============================================================================

/**
 * @brief WCAG 2.1 conformance levels
 */
enum class WCAGLevel : uint8_t {
    A,      // Level A (minimum)
    AA,     // Level AA (standard)
    AAA     // Level AAA (enhanced)
};

/**
 * @brief Screen reader types
 */
enum class ScreenReaderType : uint8_t {
    None,
    NVDA,
    JAWS,
    VoiceOver,
    Orca,
    WindowEyes,
    Dragon,
    Generic
};

/**
 * @brief Accessibility feature flags
 */
enum class AccessibilityFeature : uint32_t {
    None                    = 0,
    ScreenReader           = 1 << 0,
    HighContrast           = 1 << 1,
    ReducedMotion          = 1 << 2,
    LargeText              = 1 << 3,
    KeyboardNavigation     = 1 << 4,
    FocusIndicators        = 1 << 5,
    MotorAssistance        = 1 << 6,
    ColorBlindnessSupport  = 1 << 7,
    AudioDescriptions      = 1 << 8,
    SlowAnimations         = 1 << 9,
    StickyKeys             = 1 << 10,
    MouseKeys              = 1 << 11,
    FilterKeys             = 1 << 12,
    ToggleKeys             = 1 << 13,
    SoundSentry            = 1 << 14,
    VisualNotifications    = 1 << 15
};

/**
 * @brief Color blindness types
 */
enum class ColorBlindnessType : uint8_t {
    None,
    Protanopia,    // Red-blind
    Deuteranopia,  // Green-blind
    Tritanopia,    // Blue-blind
    Achromatopsia, // Complete color blindness
    Protanomaly,   // Red-weak
    Deuteranomaly, // Green-weak
    Tritanomaly    // Blue-weak
};

/**
 * @brief Motor disability accommodation types
 */
enum class MotorAccommodation : uint8_t {
    None,
    StickyKeys,      // Hold modifiers without continuous pressure
    SlowKeys,        // Ignore quick key presses
    BounceKeys,      // Ignore repeated keystrokes
    MouseKeys,       // Use numeric keypad as mouse
    ClickLock,       // Lock mouse clicks
    HoverClick,      // Click by hovering
    DwellClick,      // Click by dwelling
    SwitchAccess,    // Switch-based navigation
    EyeTracking,     // Eye-tracking control
    VoiceControl     // Voice commands
};

/**
 * @brief Focus navigation patterns
 */
enum class FocusPattern : uint8_t {
    Sequential,    // Tab order
    Spatial,       // Arrow keys (up/down/left/right)
    Hierarchical,  // Tree-like navigation
    Grid,          // 2D grid navigation
    Custom         // Application-defined
};

// =============================================================================
// ACCESSIBILITY STRUCTURES
// =============================================================================

/**
 * @brief Color contrast information
 */
struct ContrastInfo {
    float ratio = 0.0f;
    bool passes_aa = false;
    bool passes_aaa = false;
    Color foreground;
    Color background;
    float foreground_luminance = 0.0f;
    float background_luminance = 0.0f;
};

/**
 * @brief ARIA-like role definitions
 */
enum class AccessibilityRole : uint16_t {
    None,
    // Landmark roles
    Application, Banner, Complementary, ContentInfo, Form, 
    Main, Navigation, Region, Search,
    
    // Widget roles
    Alert, AlertDialog, Button, Checkbox, Dialog, GridCell,
    Link, Log, Marquee, MenuItem, MenuItemCheckbox, MenuItemRadio,
    Option, ProgressBar, Radio, ScrollBar, Slider, SpinButton,
    Status, Tab, TabPanel, TextBox, Timer, ToolTip, TreeItem,
    
    // Composite roles
    ComboBox, Grid, ListBox, Menu, MenuBar, RadioGroup,
    TabList, Tree, TreeGrid,
    
    // Document structure roles
    Article, ColumnHeader, Definition, Directory, Document,
    Group, Heading, Img, List, ListItem, Math, Note,
    Presentation, Row, RowGroup, RowHeader, Separator,
    Table, Term,
    
    // Live region roles
    LiveRegion, Log, Status, Alert, Timer
};

/**
 * @brief ARIA-like states and properties
 */
struct AccessibilityState {
    // States
    bool busy = false;
    bool checked = false;
    bool disabled = false;
    bool expanded = false;
    bool grabbed = false;
    bool hidden = false;
    bool invalid = false;
    bool pressed = false;
    bool selected = false;
    
    // Properties
    std::string label;
    std::string description;
    std::string help_text;
    std::string value_text;
    std::optional<int> level;           // For headings
    std::optional<int> position_in_set; // Position in list/group
    std::optional<int> set_size;        // Size of containing set
    float value_min = 0.0f;
    float value_max = 100.0f;
    float value_now = 0.0f;
    
    // Relationships
    GuiID controls_id = 0;        // aria-controls
    GuiID described_by_id = 0;    // aria-describedby
    GuiID labelled_by_id = 0;     // aria-labelledby
    GuiID owns_id = 0;            // aria-owns
    GuiID flows_to_id = 0;        // aria-flowsto
};

/**
 * @brief Keyboard navigation state
 */
struct NavigationState {
    GuiID current_focus = 0;
    GuiID previous_focus = 0;
    std::vector<GuiID> focus_history;
    FocusPattern pattern = FocusPattern::Sequential;
    bool wrap_around = true;
    bool skip_disabled = true;
    std::chrono::steady_clock::time_point last_navigation;
};

/**
 * @brief Accessibility preferences
 */
struct AccessibilityPreferences {
    // General
    WCAGLevel target_level = WCAGLevel::AA;
    std::bitset<16> enabled_features;
    ScreenReaderType screen_reader = ScreenReaderType::None;
    
    // Visual
    bool high_contrast = false;
    bool reduced_motion = false;
    float font_scale = 1.0f;
    float ui_scale = 1.0f;
    ColorBlindnessType color_blindness = ColorBlindnessType::None;
    float minimum_contrast_ratio = 4.5f;  // WCAG AA standard
    
    // Motor
    MotorAccommodation motor_accommodation = MotorAccommodation::None;
    float key_repeat_delay = 0.5f;
    float key_repeat_rate = 0.1f;
    float double_click_time = 0.5f;
    float click_tolerance = 5.0f;
    bool sticky_keys = false;
    bool slow_keys = false;
    bool bounce_keys = false;
    
    // Audio
    bool audio_descriptions = false;
    bool sound_notifications = false;
    float sound_volume = 0.7f;
    
    // Focus and navigation
    bool enhanced_focus_indicators = true;
    float focus_indicator_thickness = 2.0f;
    Color focus_indicator_color = Color(0, 120, 215, 255); // Windows blue
    bool focus_follows_mouse = false;
    float focus_animation_duration = 0.2f;
    
    // Timeout adjustments
    float ui_timeout_multiplier = 1.0f;
    bool disable_timeouts = false;
    
    // Custom user settings
    std::unordered_map<std::string, std::string> custom_settings;
};

// =============================================================================
// ACCESSIBILITY CONTEXT
// =============================================================================

/**
 * @brief Widget accessibility information
 */
struct WidgetAccessibilityInfo {
    AccessibilityRole role = AccessibilityRole::None;
    AccessibilityState state;
    GuiID widget_id = 0;
    Rect bounds;
    bool focusable = false;
    bool keyboard_accessible = true;
    int tab_index = 0;
    std::string keyboard_shortcut;
    
    // Navigation relationships
    GuiID parent_id = 0;
    std::vector<GuiID> child_ids;
    GuiID next_sibling_id = 0;
    GuiID previous_sibling_id = 0;
    
    // Live region properties
    enum class LiveRegionPoliteness { Off, Polite, Assertive } live_politeness = LiveRegionPoliteness::Off;
    bool live_atomic = false;
    bool live_relevant_additions = true;
    bool live_relevant_removals = true;
    bool live_relevant_text = true;
    
    // Validation
    std::vector<std::string> validation_errors;
    std::chrono::steady_clock::time_point last_updated;
};

/**
 * @brief Central accessibility context
 */
class AccessibilityContext {
public:
    AccessibilityContext();
    ~AccessibilityContext();

    // =============================================================================
    // INITIALIZATION & LIFECYCLE
    // =============================================================================

    bool initialize();
    void shutdown();
    void update(float delta_time);

    // =============================================================================
    // WIDGET REGISTRATION & MANAGEMENT
    // =============================================================================

    void register_widget(GuiID widget_id, const WidgetAccessibilityInfo& info);
    void unregister_widget(GuiID widget_id);
    void update_widget_info(GuiID widget_id, const WidgetAccessibilityInfo& info);
    
    const WidgetAccessibilityInfo* get_widget_info(GuiID widget_id) const;
    std::vector<GuiID> get_all_widgets() const;
    std::vector<GuiID> get_focusable_widgets() const;

    // =============================================================================
    // ACCESSIBILITY TREE
    // =============================================================================

    void build_accessibility_tree();
    void set_parent_child_relationship(GuiID parent_id, GuiID child_id);
    void remove_parent_child_relationship(GuiID parent_id, GuiID child_id);
    
    std::vector<GuiID> get_children(GuiID widget_id) const;
    GuiID get_parent(GuiID widget_id) const;
    std::vector<GuiID> get_siblings(GuiID widget_id) const;

    // =============================================================================
    // FOCUS MANAGEMENT
    // =============================================================================

    void set_focus(GuiID widget_id, bool notify_screen_reader = true);
    GuiID get_current_focus() const;
    void clear_focus();
    
    bool move_focus_next();
    bool move_focus_previous();
    bool move_focus_to_parent();
    bool move_focus_to_first_child();
    
    void set_focus_pattern(FocusPattern pattern);
    FocusPattern get_focus_pattern() const;

    // =============================================================================
    // SCREEN READER SUPPORT
    // =============================================================================

    void announce_to_screen_reader(const std::string& message, bool interrupt = false);
    void announce_focus_change(GuiID widget_id);
    void announce_state_change(GuiID widget_id, const std::string& change);
    void announce_value_change(GuiID widget_id, const std::string& old_value, const std::string& new_value);
    
    void set_screen_reader_type(ScreenReaderType type);
    ScreenReaderType get_screen_reader_type() const;
    bool is_screen_reader_active() const;

    // =============================================================================
    // LIVE REGIONS
    // =============================================================================

    void create_live_region(GuiID region_id, WidgetAccessibilityInfo::LiveRegionPoliteness politeness);
    void update_live_region(GuiID region_id, const std::string& content);
    void remove_live_region(GuiID region_id);

    // =============================================================================
    // PREFERENCES & CONFIGURATION
    // =============================================================================

    void set_preferences(const AccessibilityPreferences& prefs);
    const AccessibilityPreferences& get_preferences() const;
    void load_preferences_from_file(const std::string& filename);
    void save_preferences_to_file(const std::string& filename) const;
    
    bool is_feature_enabled(AccessibilityFeature feature) const;
    void enable_feature(AccessibilityFeature feature, bool enable = true);

    // =============================================================================
    // VALIDATION & TESTING
    // =============================================================================

    struct ValidationResult {
        bool passes_wcag_aa = true;
        bool passes_wcag_aaa = true;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        std::vector<std::string> suggestions;
    };

    ValidationResult validate_accessibility();
    ValidationResult validate_widget(GuiID widget_id);
    ValidationResult validate_color_contrast(const Color& foreground, const Color& background);
    ValidationResult validate_focus_order();
    ValidationResult validate_keyboard_navigation();

    // =============================================================================
    // UTILITY FUNCTIONS
    // =============================================================================

    std::string get_accessible_name(GuiID widget_id) const;
    std::string get_accessible_description(GuiID widget_id) const;
    std::string get_role_name(AccessibilityRole role) const;
    std::string get_state_description(const AccessibilityState& state) const;
    
    ContrastInfo calculate_contrast_info(const Color& foreground, const Color& background) const;
    Color adjust_for_color_blindness(const Color& original) const;
    bool should_reduce_motion() const;

    // =============================================================================
    // EVENT CALLBACKS
    // =============================================================================

    using FocusChangeCallback = std::function<void(GuiID old_focus, GuiID new_focus)>;
    using StateChangeCallback = std::function<void(GuiID widget_id, const AccessibilityState& old_state, const AccessibilityState& new_state)>;
    using AnnouncementCallback = std::function<void(const std::string& message, bool interrupt)>;

    void set_focus_change_callback(FocusChangeCallback callback);
    void set_state_change_callback(StateChangeCallback callback);
    void set_announcement_callback(AnnouncementCallback callback);

private:
    // Widget storage
    std::unordered_map<GuiID, WidgetAccessibilityInfo> widgets_;
    
    // Navigation state
    NavigationState navigation_state_;
    
    // Preferences
    AccessibilityPreferences preferences_;
    
    // Screen reader integration
    ScreenReaderType screen_reader_type_ = ScreenReaderType::None;
    bool screen_reader_active_ = false;
    
    // Live regions
    std::unordered_map<GuiID, WidgetAccessibilityInfo::LiveRegionPoliteness> live_regions_;
    
    // Callbacks
    FocusChangeCallback focus_change_callback_;
    StateChangeCallback state_change_callback_;
    AnnouncementCallback announcement_callback_;
    
    // Validation cache
    mutable std::unordered_map<GuiID, ValidationResult> validation_cache_;
    mutable std::chrono::steady_clock::time_point last_validation_time_;
    
    // Helper methods
    void detect_screen_reader();
    void apply_accessibility_theme();
    std::string generate_accessible_name(const WidgetAccessibilityInfo& info) const;
    std::string generate_accessible_description(const WidgetAccessibilityInfo& info) const;
    GuiID find_next_focusable_widget(GuiID current, bool forward) const;
    GuiID find_spatially_next_widget(GuiID current, int direction) const;
    void invalidate_validation_cache();
    bool initialized_ = false;
};

// =============================================================================
// GLOBAL ACCESSIBILITY MANAGER
// =============================================================================

/**
 * @brief Get the global accessibility context
 */
AccessibilityContext* get_accessibility_context();

/**
 * @brief Initialize global accessibility system
 */
bool initialize_accessibility();

/**
 * @brief Shutdown global accessibility system
 */
void shutdown_accessibility();

/**
 * @brief Check if accessibility features are enabled
 */
bool is_accessibility_enabled();

// =============================================================================
// CONVENIENCE MACROS FOR WCAG COMPLIANCE
// =============================================================================

#define ACCESSIBILITY_LABEL(widget_id, label) \
    do { \
        auto* ctx = get_accessibility_context(); \
        if (ctx) { \
            auto info = ctx->get_widget_info(widget_id); \
            if (info) { \
                auto updated_info = *info; \
                updated_info.state.label = label; \
                ctx->update_widget_info(widget_id, updated_info); \
            } \
        } \
    } while(0)

#define ACCESSIBILITY_DESCRIPTION(widget_id, description) \
    do { \
        auto* ctx = get_accessibility_context(); \
        if (ctx) { \
            auto info = ctx->get_widget_info(widget_id); \
            if (info) { \
                auto updated_info = *info; \
                updated_info.state.description = description; \
                ctx->update_widget_info(widget_id, updated_info); \
            } \
        } \
    } while(0)

#define ACCESSIBILITY_ROLE(widget_id, role) \
    do { \
        auto* ctx = get_accessibility_context(); \
        if (ctx) { \
            auto info = ctx->get_widget_info(widget_id); \
            if (info) { \
                auto updated_info = *info; \
                updated_info.role = role; \
                ctx->update_widget_info(widget_id, updated_info); \
            } \
        } \
    } while(0)

#define ACCESSIBILITY_ANNOUNCE(message) \
    do { \
        auto* ctx = get_accessibility_context(); \
        if (ctx && ctx->is_screen_reader_active()) { \
            ctx->announce_to_screen_reader(message); \
        } \
    } while(0)

} // namespace ecscope::gui::accessibility