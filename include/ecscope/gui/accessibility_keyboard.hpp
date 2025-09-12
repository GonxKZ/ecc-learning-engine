/**
 * @file accessibility_keyboard.hpp
 * @brief Advanced Keyboard Navigation and Focus Management
 * 
 * Enhanced keyboard navigation system providing comprehensive keyboard accessibility
 * with advanced focus management, spatial navigation, focus traps, skip links,
 * and accessibility shortcuts for professional development tools.
 * 
 * Features:
 * - Advanced focus management with visual indicators
 * - Spatial navigation (arrow keys, grid navigation)
 * - Focus traps for modals and overlays
 * - Skip links for efficient navigation
 * - Roving tabindex implementation
 * - Accessibility shortcuts and hotkeys
 * - Focus history and breadcrumbs
 * - Keyboard event filtering and processing
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#pragma once

#include "accessibility_core.hpp"
#include "gui_input.hpp"
#include <memory>
#include <stack>
#include <queue>
#include <chrono>
#include <functional>

namespace ecscope::gui::accessibility {

// =============================================================================
// KEYBOARD NAVIGATION ENHANATIONS
// =============================================================================

/**
 * @brief Focus indicator styles
 */
struct FocusIndicatorStyle {
    Color color = Color(0, 120, 215, 255);  // Windows blue
    float thickness = 2.0f;
    float rounding = 4.0f;
    float padding = 2.0f;
    bool animated = true;
    float animation_duration = 0.2f;
    enum class Style { 
        Solid, 
        Dashed, 
        Dotted, 
        Glow, 
        HighContrast 
    } style = Style::Solid;
    
    // High contrast mode
    Color high_contrast_color = Color(255, 255, 255, 255);
    float high_contrast_thickness = 3.0f;
    
    // Animation parameters
    float glow_intensity = 0.5f;
    float pulse_speed = 2.0f;
};

/**
 * @brief Focus trap configuration
 */
struct FocusTrap {
    GuiID container_id = 0;
    std::vector<GuiID> focusable_widgets;
    GuiID initial_focus = 0;
    GuiID return_focus = 0;  // Where to return focus when trap is released
    bool active = false;
    bool cycle_at_ends = true;
    
    // Event handlers
    std::function<void()> on_activate;
    std::function<void()> on_deactivate;
    std::function<bool(Key key)> on_escape_attempt;
};

/**
 * @brief Skip link definition
 */
struct SkipLink {
    std::string label;
    GuiID target_id = 0;
    Key shortcut_key = Key::Unknown;
    KeyMod shortcut_mods = KeyMod::None;
    bool visible = false;
    bool always_visible = false;
    int priority = 0;  // Lower numbers appear first
    
    std::function<void()> on_activate;
};

/**
 * @brief Roving tabindex group
 */
struct RovingTabindexGroup {
    std::vector<GuiID> widgets;
    GuiID current_active = 0;
    FocusPattern navigation_pattern = FocusPattern::Sequential;
    bool wrap_around = true;
    bool skip_disabled = true;
    
    // Grid navigation (if applicable)
    int grid_columns = 0;
    int grid_rows = 0;
};

/**
 * @brief Accessibility shortcut definition
 */
struct AccessibilityShortcut {
    std::string name;
    std::string description;
    Key key = Key::Unknown;
    KeyMod mods = KeyMod::None;
    std::function<void()> action;
    bool enabled = true;
    bool global = false;  // Available everywhere vs context-specific
    std::string context;  // Context where shortcut is available
};

/**
 * @brief Spatial navigation grid
 */
class SpatialNavigationGrid {
public:
    struct GridCell {
        GuiID widget_id = 0;
        Rect bounds;
        bool focusable = true;
        int row = -1;
        int column = -1;
    };

    void clear();
    void add_widget(GuiID widget_id, const Rect& bounds, int row = -1, int column = -1);
    void remove_widget(GuiID widget_id);
    void rebuild_from_widgets(const std::vector<std::pair<GuiID, Rect>>& widgets);
    
    GuiID find_next_widget(GuiID current, NavDirection direction) const;
    GuiID find_closest_widget(const Vec2& position) const;
    
    std::vector<GuiID> get_row_widgets(int row) const;
    std::vector<GuiID> get_column_widgets(int column) const;
    
    int get_widget_row(GuiID widget_id) const;
    int get_widget_column(GuiID widget_id) const;
    
    void set_grid_dimensions(int rows, int columns);
    std::pair<int, int> get_grid_dimensions() const;

private:
    std::vector<GridCell> cells_;
    std::unordered_map<GuiID, size_t> widget_to_cell_;
    int rows_ = 0;
    int columns_ = 0;
    
    void auto_assign_grid_positions();
    float calculate_spatial_distance(const Rect& from, const Rect& to, NavDirection direction) const;
    bool is_in_direction(const Rect& from, const Rect& to, NavDirection direction) const;
};

// =============================================================================
// ADVANCED KEYBOARD NAVIGATOR
// =============================================================================

/**
 * @brief Advanced keyboard navigation system
 */
class AdvancedKeyboardNavigator {
public:
    AdvancedKeyboardNavigator();
    ~AdvancedKeyboardNavigator();

    // =============================================================================
    // INITIALIZATION & LIFECYCLE
    // =============================================================================

    bool initialize(AccessibilityContext* accessibility_context);
    void shutdown();
    void update(float delta_time);

    // =============================================================================
    // WIDGET REGISTRATION
    // =============================================================================

    void register_widget(GuiID widget_id, const Rect& bounds, bool focusable = true, int tab_index = 0);
    void unregister_widget(GuiID widget_id);
    void update_widget_bounds(GuiID widget_id, const Rect& bounds);
    void set_widget_focusable(GuiID widget_id, bool focusable);

    // =============================================================================
    // FOCUS MANAGEMENT
    // =============================================================================

    void set_focus(GuiID widget_id, bool show_indicator = true);
    GuiID get_current_focus() const;
    void clear_focus();
    
    bool move_focus_next(bool wrap = true);
    bool move_focus_previous(bool wrap = true);
    bool move_focus_up();
    bool move_focus_down();
    bool move_focus_left();
    bool move_focus_right();
    bool move_focus_to_parent();
    bool move_focus_to_first_child();
    
    void set_focus_pattern(FocusPattern pattern);
    FocusPattern get_focus_pattern() const;

    // =============================================================================
    // FOCUS INDICATORS
    // =============================================================================

    void set_focus_indicator_style(const FocusIndicatorStyle& style);
    const FocusIndicatorStyle& get_focus_indicator_style() const;
    
    void show_focus_indicator(GuiID widget_id, bool animated = true);
    void hide_focus_indicator();
    void render_focus_indicator(DrawList* draw_list);
    
    bool is_focus_indicator_visible() const;

    // =============================================================================
    // FOCUS TRAPS
    // =============================================================================

    void create_focus_trap(const FocusTrap& trap);
    void activate_focus_trap(GuiID container_id);
    void deactivate_focus_trap(GuiID container_id);
    void release_all_focus_traps();
    
    bool is_focus_trapped() const;
    GuiID get_active_focus_trap() const;

    // =============================================================================
    // SKIP LINKS
    // =============================================================================

    void add_skip_link(const SkipLink& skip_link);
    void remove_skip_link(GuiID target_id);
    void clear_skip_links();
    
    void show_skip_links();
    void hide_skip_links();
    bool are_skip_links_visible() const;
    
    void render_skip_links(DrawList* draw_list);
    bool process_skip_link_activation(Key key, KeyMod mods);

    // =============================================================================
    // ROVING TABINDEX
    // =============================================================================

    void create_roving_tabindex_group(const std::string& group_name, const RovingTabindexGroup& group);
    void remove_roving_tabindex_group(const std::string& group_name);
    
    void add_widget_to_roving_group(const std::string& group_name, GuiID widget_id);
    void remove_widget_from_roving_group(const std::string& group_name, GuiID widget_id);
    
    void set_active_widget_in_group(const std::string& group_name, GuiID widget_id);
    GuiID get_active_widget_in_group(const std::string& group_name) const;

    // =============================================================================
    // SPATIAL NAVIGATION
    // =============================================================================

    void enable_spatial_navigation(bool enable = true);
    bool is_spatial_navigation_enabled() const;
    
    SpatialNavigationGrid* get_spatial_grid() { return &spatial_grid_; }
    const SpatialNavigationGrid* get_spatial_grid() const { return &spatial_grid_; }
    
    void rebuild_spatial_grid();
    void set_spatial_grid_dimensions(int rows, int columns);

    // =============================================================================
    // ACCESSIBILITY SHORTCUTS
    // =============================================================================

    void register_accessibility_shortcut(const AccessibilityShortcut& shortcut);
    void unregister_accessibility_shortcut(const std::string& name);
    void enable_accessibility_shortcut(const std::string& name, bool enabled);
    
    void set_current_context(const std::string& context);
    std::string get_current_context() const;
    
    std::vector<AccessibilityShortcut> get_available_shortcuts() const;
    std::vector<AccessibilityShortcut> get_context_shortcuts(const std::string& context) const;

    // =============================================================================
    // KEYBOARD EVENT PROCESSING
    // =============================================================================

    bool process_keyboard_event(const InputEvent& event);
    bool handle_navigation_key(Key key, KeyMod mods, bool pressed);
    bool handle_action_key(Key key, KeyMod mods, bool pressed);
    
    void set_key_repeat_settings(float delay, float rate);
    void set_navigation_sounds_enabled(bool enabled);

    // =============================================================================
    // FOCUS HISTORY
    // =============================================================================

    void enable_focus_history(bool enable = true, size_t max_history = 50);
    std::vector<GuiID> get_focus_history() const;
    void clear_focus_history();
    
    bool return_to_previous_focus();
    void create_focus_breadcrumb(const std::string& label, GuiID widget_id);

    // =============================================================================
    // DEBUGGING & DIAGNOSTICS
    // =============================================================================

    struct NavigationStats {
        size_t registered_widgets = 0;
        size_t focusable_widgets = 0;
        size_t active_focus_traps = 0;
        size_t skip_links = 0;
        size_t roving_groups = 0;
        size_t accessibility_shortcuts = 0;
        GuiID current_focus = 0;
        std::string current_context;
        bool spatial_navigation_enabled = false;
        bool focus_history_enabled = false;
    };

    NavigationStats get_navigation_stats() const;
    void render_debug_overlay(DrawList* draw_list);
    void print_navigation_tree() const;

    // =============================================================================
    // EVENT CALLBACKS
    // =============================================================================

    using FocusChangeCallback = std::function<void(GuiID old_focus, GuiID new_focus)>;
    using NavigationCallback = std::function<bool(GuiID from, GuiID to, NavDirection direction)>;
    using ShortcutCallback = std::function<bool(const AccessibilityShortcut& shortcut)>;

    void set_focus_change_callback(FocusChangeCallback callback);
    void set_navigation_callback(NavigationCallback callback);
    void set_shortcut_callback(ShortcutCallback callback);

private:
    // Core components
    AccessibilityContext* accessibility_context_ = nullptr;
    
    // Focus management
    GuiID current_focus_ = 0;
    GuiID previous_focus_ = 0;
    FocusPattern focus_pattern_ = FocusPattern::Sequential;
    
    // Focus indicators
    FocusIndicatorStyle focus_indicator_style_;
    GuiID focus_indicator_widget_ = 0;
    bool focus_indicator_visible_ = false;
    std::chrono::steady_clock::time_point focus_indicator_start_time_;
    
    // Focus traps
    std::unordered_map<GuiID, FocusTrap> focus_traps_;
    std::stack<GuiID> active_focus_trap_stack_;
    
    // Skip links
    std::vector<SkipLink> skip_links_;
    bool skip_links_visible_ = false;
    
    // Roving tabindex
    std::unordered_map<std::string, RovingTabindexGroup> roving_groups_;
    
    // Spatial navigation
    bool spatial_navigation_enabled_ = false;
    SpatialNavigationGrid spatial_grid_;
    
    // Accessibility shortcuts
    std::unordered_map<std::string, AccessibilityShortcut> accessibility_shortcuts_;
    std::string current_context_ = "default";
    
    // Focus history
    bool focus_history_enabled_ = false;
    std::queue<GuiID> focus_history_;
    size_t max_focus_history_ = 50;
    struct FocusBreadcrumb {
        std::string label;
        GuiID widget_id;
        std::chrono::steady_clock::time_point timestamp;
    };
    std::vector<FocusBreadcrumb> focus_breadcrumbs_;
    
    // Widget registry
    struct NavigableWidget {
        GuiID id = 0;
        Rect bounds;
        bool focusable = true;
        int tab_index = 0;
        std::chrono::steady_clock::time_point last_updated;
    };
    std::unordered_map<GuiID, NavigableWidget> widgets_;
    
    // Key repeat handling
    struct KeyRepeatState {
        Key key = Key::Unknown;
        KeyMod mods = KeyMod::None;
        std::chrono::steady_clock::time_point first_press;
        std::chrono::steady_clock::time_point last_repeat;
        float delay = 0.5f;
        float rate = 0.1f;
        bool repeating = false;
    } key_repeat_;
    
    // Audio feedback
    bool navigation_sounds_enabled_ = false;
    
    // Callbacks
    FocusChangeCallback focus_change_callback_;
    NavigationCallback navigation_callback_;
    ShortcutCallback shortcut_callback_;
    
    // Helper methods
    std::vector<GuiID> get_tab_order() const;
    std::vector<GuiID> get_focusable_widgets_in_trap(GuiID trap_id) const;
    bool is_widget_in_focus_trap(GuiID widget_id, GuiID trap_id) const;
    GuiID find_next_focusable_in_trap(GuiID current_focus, GuiID trap_id, bool forward) const;
    
    void update_focus_indicator_animation(float delta_time);
    void play_navigation_sound(const std::string& sound_name);
    
    bool initialized_ = false;
};

// =============================================================================
// KEYBOARD NAVIGATION UTILITIES
// =============================================================================

/**
 * @brief RAII helper for focus traps
 */
class ScopedFocusTrap {
public:
    ScopedFocusTrap(AdvancedKeyboardNavigator* navigator, const FocusTrap& trap);
    ~ScopedFocusTrap();
    
    ScopedFocusTrap(const ScopedFocusTrap&) = delete;
    ScopedFocusTrap& operator=(const ScopedFocusTrap&) = delete;

private:
    AdvancedKeyboardNavigator* navigator_;
    GuiID trap_id_;
};

/**
 * @brief RAII helper for skip links
 */
class ScopedSkipLinks {
public:
    ScopedSkipLinks(AdvancedKeyboardNavigator* navigator);
    ~ScopedSkipLinks();
    
    void add_skip_link(const SkipLink& skip_link);

private:
    AdvancedKeyboardNavigator* navigator_;
    std::vector<GuiID> added_skip_links_;
};

/**
 * @brief Keyboard navigation helper functions
 */
namespace keyboard_utils {
    /**
     * @brief Create standard skip links for a typical interface
     */
    std::vector<SkipLink> create_standard_skip_links();
    
    /**
     * @brief Create accessibility shortcuts for common actions
     */
    std::vector<AccessibilityShortcut> create_standard_shortcuts();
    
    /**
     * @brief Get recommended tab order for widgets
     */
    std::vector<GuiID> calculate_recommended_tab_order(const std::vector<std::pair<GuiID, Rect>>& widgets);
    
    /**
     * @brief Calculate optimal focus indicator style based on theme
     */
    FocusIndicatorStyle calculate_optimal_focus_style(const Theme& theme, bool high_contrast);
    
    /**
     * @brief Check if key combination is accessible
     */
    bool is_key_combination_accessible(Key key, KeyMod mods);
    
    /**
     * @brief Get human-readable description of key combination
     */
    std::string describe_key_combination(Key key, KeyMod mods);
}

// =============================================================================
// GLOBAL KEYBOARD NAVIGATION MANAGER
// =============================================================================

/**
 * @brief Get the global keyboard navigator
 */
AdvancedKeyboardNavigator* get_keyboard_navigator();

/**
 * @brief Initialize global keyboard navigation
 */
bool initialize_keyboard_navigation();

/**
 * @brief Shutdown global keyboard navigation
 */
void shutdown_keyboard_navigation();

} // namespace ecscope::gui::accessibility