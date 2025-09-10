/**
 * @file window.hpp
 * @brief Window System - Advanced window management and layout
 * 
 * Comprehensive window system with docking, tabs, viewports,
 * and advanced layout capabilities.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#pragma once

#include "core.hpp"
#include "layout.hpp"
#include <functional>

namespace ecscope::gui {

// Forward declarations
class Context;
class DockSpace;

// =============================================================================
// WINDOW FLAGS & CONFIGURATION
// =============================================================================

/**
 * @brief Window behavior flags
 */
enum class WindowFlags : uint32_t {
    None                     = 0,
    NoTitleBar              = 1 << 0,   // Disable title bar
    NoResize                = 1 << 1,   // Disable manual resize
    NoMove                  = 1 << 2,   // Disable manual moving
    NoScrollbar             = 1 << 3,   // Disable scrollbars
    NoScrollWithMouse       = 1 << 4,   // Disable scrolling with mouse wheel
    NoCollapse              = 1 << 5,   // Disable collapse button
    AlwaysAutoResize        = 1 << 6,   // Resize to content size
    NoBackground            = 1 << 7,   // Disable background drawing
    NoSavedSettings         = 1 << 8,   // Don't save/load window state
    NoMouseInputs           = 1 << 9,   // Disable mouse input
    MenuBar                 = 1 << 10,  // Has menu bar
    HorizontalScrollbar     = 1 << 11,  // Allow horizontal scrollbar
    NoFocusOnAppearing      = 1 << 12,  // Disable focus when appearing
    NoBringToFrontOnFocus   = 1 << 13,  // Disable bringing to front
    AlwaysVerticalScrollbar = 1 << 14,  // Always show vertical scrollbar
    AlwaysHorizontalScrollbar = 1 << 15, // Always show horizontal scrollbar
    AlwaysUseWindowPadding  = 1 << 16,  // Ensure child windows use padding
    NoNavInputs             = 1 << 17,  // Disable gamepad/keyboard navigation
    NoNavFocus              = 1 << 18,  // Disable focus with Tab/Shift+Tab
    UnsavedDocument         = 1 << 19,  // Display a dot next to title
    NoDocking               = 1 << 20,  // Disable docking
    
    // Convenience combinations
    NoNav                   = NoNavInputs | NoNavFocus,
    NoDecoration           = NoTitleBar | NoResize | NoScrollbar | NoCollapse,
    NoInputs               = NoMouseInputs | NoNavInputs | NoNavFocus,
    
    // Child window specific
    ChildWindow            = 1 << 24,   // Internal use
    Tooltip                = 1 << 25,   // Internal use  
    Popup                  = 1 << 26,   // Internal use
    Modal                  = 1 << 27,   // Internal use
    ChildMenu              = 1 << 28    // Internal use
};

inline WindowFlags operator|(WindowFlags a, WindowFlags b) {
    return static_cast<WindowFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline WindowFlags operator&(WindowFlags a, WindowFlags b) {
    return static_cast<WindowFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline bool has_flag(WindowFlags flags, WindowFlags flag) {
    return (flags & flag) == flag;
}

/**
 * @brief Window condition for size/position setting
 */
enum class WindowCondition : uint8_t {
    Always,        // Set every frame
    Once,          // Set only once
    FirstUseEver,  // Set if no existing data
    Appearing      // Set if appearing this frame
};

// =============================================================================
// WINDOW STRUCTURE
// =============================================================================

/**
 * @brief Window state and data
 */
struct WindowData {
    std::string name;
    ID id;
    
    // Position and size
    Vec2 position;
    Vec2 size;
    Vec2 size_full;  // Size including decorations
    Vec2 size_contents;  // Content area size
    Vec2 size_contents_ideal;  // Ideal content size
    
    // Display state
    bool is_collapsed = false;
    bool want_collapse_toggle = false;
    bool skip_items = false;  // Set when collapsed
    bool appearing = false;
    bool hidden = false;
    bool has_close_button = false;
    
    // Layout
    Vec2 cursor_pos;
    Vec2 cursor_max_pos;
    Vec2 cursor_start_pos;
    Vec2 scroll;
    Vec2 scroll_max;
    Vec2 scroll_target;
    Vec2 scroll_target_center_ratio;
    bool scroll_target_edge_snap_dist_x = 0.0f;
    bool scroll_target_edge_snap_dist_y = 0.0f;
    bool scroll_request_x = false;
    bool scroll_request_y = false;
    
    // Focus and interaction
    bool was_active = false;
    bool write_accessed = false;
    bool focus_id_set_this_frame = false;
    ID focus_id_next_frame = 0;
    ID focus_id_desired = 0;
    
    // Docking
    ID dock_id = 0;
    bool docking_allowed = true;
    
    // Drawing
    DrawList* draw_list = nullptr;
    DrawList* draw_list_inst = nullptr;  // Instance-specific draw list
    
    // Style
    float title_bar_height = 0.0f;
    float menu_bar_height = 0.0f;
    
    WindowData(const std::string& name);
    ~WindowData();
};

// =============================================================================
// WINDOW CLASS
// =============================================================================

/**
 * @brief Main window class
 */
class Window {
public:
    Window(const std::string& name, WindowFlags flags = WindowFlags::None);
    ~Window();
    
    // Non-copyable, movable
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) noexcept;
    Window& operator=(Window&&) noexcept;
    
    // =============================================================================
    // WINDOW LIFECYCLE
    // =============================================================================
    
    /**
     * @brief Begin window rendering
     * @return false if window is collapsed/hidden
     */
    bool begin();
    
    /**
     * @brief End window rendering
     */
    void end();
    
    /**
     * @brief Check if window is open
     */
    bool is_open() const { return is_open_; }
    
    /**
     * @brief Close the window
     */
    void close() { is_open_ = false; }
    
    // =============================================================================
    // WINDOW PROPERTIES
    // =============================================================================
    
    /**
     * @brief Get window name
     */
    const std::string& get_name() const { return data_.name; }
    
    /**
     * @brief Get window ID
     */
    ID get_id() const { return data_.id; }
    
    /**
     * @brief Get window flags
     */
    WindowFlags get_flags() const { return flags_; }
    
    /**
     * @brief Set window flags
     */
    void set_flags(WindowFlags flags) { flags_ = flags; }
    
    // =============================================================================
    // POSITION & SIZE
    // =============================================================================
    
    /**
     * @brief Set window position
     */
    void set_position(const Vec2& pos, WindowCondition condition = WindowCondition::Always);
    
    /**
     * @brief Set window size
     */
    void set_size(const Vec2& size, WindowCondition condition = WindowCondition::Always);
    
    /**
     * @brief Set size constraints
     */
    void set_size_constraints(const Vec2& min_size, const Vec2& max_size);
    
    /**
     * @brief Get window position
     */
    Vec2 get_position() const { return data_.position; }
    
    /**
     * @brief Get window size
     */
    Vec2 get_size() const { return data_.size; }
    
    /**
     * @brief Get content region size
     */
    Vec2 get_content_region_size() const { return data_.size_contents; }
    
    /**
     * @brief Get content region available size
     */
    Vec2 get_content_region_avail() const;
    
    // =============================================================================
    // WINDOW STATE
    // =============================================================================
    
    /**
     * @brief Check if window is collapsed
     */
    bool is_collapsed() const { return data_.is_collapsed; }
    
    /**
     * @brief Check if window is focused
     */
    bool is_focused() const;
    
    /**
     * @brief Check if window is hovered
     */
    bool is_hovered() const;
    
    /**
     * @brief Focus this window
     */
    void focus();
    
    /**
     * @brief Bring window to front
     */
    void bring_to_front();
    
    // =============================================================================
    // SCROLLING
    // =============================================================================
    
    /**
     * @brief Get scroll position
     */
    Vec2 get_scroll() const { return data_.scroll; }
    
    /**
     * @brief Set scroll position
     */
    void set_scroll(const Vec2& scroll);
    
    /**
     * @brief Set scroll X
     */
    void set_scroll_x(float scroll_x);
    
    /**
     * @brief Set scroll Y
     */
    void set_scroll_y(float scroll_y);
    
    /**
     * @brief Get maximum scroll position
     */
    Vec2 get_scroll_max() const { return data_.scroll_max; }
    
    /**
     * @brief Scroll to make item visible
     */
    void scroll_to_item(const Rect& item_rect);
    
    /**
     * @brief Scroll to top
     */
    void scroll_to_top() { set_scroll_y(0.0f); }
    
    /**
     * @brief Scroll to bottom
     */
    void scroll_to_bottom() { set_scroll_y(data_.scroll_max.y); }
    
    // =============================================================================
    // LAYOUT & DRAWING
    // =============================================================================
    
    /**
     * @brief Get window draw list
     */
    DrawList& get_draw_list();
    
    /**
     * @brief Get current cursor position (relative to window)
     */
    Vec2 get_cursor_pos() const { return data_.cursor_pos; }
    
    /**
     * @brief Set cursor position
     */
    void set_cursor_pos(const Vec2& pos);
    
    /**
     * @brief Get cursor start position
     */
    Vec2 get_cursor_start_pos() const { return data_.cursor_start_pos; }
    
    /**
     * @brief Get cursor screen position
     */
    Vec2 get_cursor_screen_pos() const;
    
    /**
     * @brief Set cursor screen position
     */
    void set_cursor_screen_pos(const Vec2& pos);
    
    // =============================================================================
    // DOCKING
    // =============================================================================
    
    /**
     * @brief Check if window is docked
     */
    bool is_docked() const { return data_.dock_id != 0; }
    
    /**
     * @brief Get dock ID
     */
    ID get_dock_id() const { return data_.dock_id; }
    
    /**
     * @brief Set docking allowed
     */
    void set_docking_allowed(bool allowed) { data_.docking_allowed = allowed; }
    
    /**
     * @brief Check if docking is allowed
     */
    bool is_docking_allowed() const { return data_.docking_allowed; }
    
    // =============================================================================
    // MENU BAR
    // =============================================================================
    
    /**
     * @brief Begin menu bar
     * @return true if menu bar is visible
     */
    bool begin_menu_bar();
    
    /**
     * @brief End menu bar
     */
    void end_menu_bar();
    
    /**
     * @brief Get menu bar height
     */
    float get_menu_bar_height() const { return data_.menu_bar_height; }
    
    // =============================================================================
    // CHILD WINDOWS
    // =============================================================================
    
    /**
     * @brief Begin child window
     * @return false if child is collapsed/hidden
     */
    static bool begin_child(const std::string& str_id, const Vec2& size = {0, 0}, 
                           bool border = false, WindowFlags extra_flags = WindowFlags::None);
    
    /**
     * @brief Begin child window with ID
     */
    static bool begin_child(ID id, const Vec2& size = {0, 0}, 
                           bool border = false, WindowFlags extra_flags = WindowFlags::None);
    
    /**
     * @brief End child window
     */
    static void end_child();
    
    // =============================================================================
    // POPUPS
    // =============================================================================
    
    /**
     * @brief Begin popup window
     */
    static bool begin_popup(const std::string& str_id, WindowFlags flags = WindowFlags::None);
    
    /**
     * @brief Begin popup context item
     */
    static bool begin_popup_context_item(const std::string& str_id = "", MouseButton button = MouseButton::Right);
    
    /**
     * @brief Begin popup context window
     */
    static bool begin_popup_context_window(const std::string& str_id = "", MouseButton button = MouseButton::Right);
    
    /**
     * @brief Begin popup context void (background)
     */
    static bool begin_popup_context_void(const std::string& str_id = "", MouseButton button = MouseButton::Right);
    
    /**
     * @brief Begin modal popup
     */
    static bool begin_popup_modal(const std::string& name, bool* open = nullptr, WindowFlags flags = WindowFlags::None);
    
    /**
     * @brief End popup
     */
    static void end_popup();
    
    /**
     * @brief Open popup
     */
    static void open_popup(const std::string& str_id, WindowFlags flags = WindowFlags::None);
    
    /**
     * @brief Open popup on item click
     */
    static void open_popup_on_item_click(const std::string& str_id = "", MouseButton button = MouseButton::Right);
    
    /**
     * @brief Close current popup
     */
    static void close_current_popup();
    
    /**
     * @brief Check if popup is open
     */
    static bool is_popup_open(const std::string& str_id);
    
private:
    WindowData data_;
    WindowFlags flags_;
    bool is_open_ = true;
    bool in_begin_ = false;
    
    // Size constraints
    Vec2 size_constraint_min_ = {-1, -1};
    Vec2 size_constraint_max_ = {-1, -1};
    
    // Condition tracking
    struct ConditionTracker {
        bool position_set_once = false;
        bool size_set_once = false;
        bool appearing_set = false;
    } condition_tracker_;
    
    // Internal methods
    void update_appearing_state();
    void update_skip_items();
    void update_window_parent_and_root_links();
    Rect get_viewport_rect() const;
    void calc_auto_resize_size();
    void apply_size_constraints();
    void render_window_decorations();
    void render_title_bar();
    void render_resize_grips();
    bool handle_window_interactions();
    void update_scroll();
    void clip_content_area();
    
    friend class Context;
};

// =============================================================================
// CONVENIENCE FUNCTIONS
// =============================================================================

/**
 * @brief Begin a simple window
 * @return false if window is collapsed/hidden
 */
bool begin(const std::string& name, bool* open = nullptr, WindowFlags flags = WindowFlags::None);

/**
 * @brief End current window
 */
void end();

/**
 * @brief Set next window position
 */
void set_next_window_pos(const Vec2& pos, WindowCondition condition = WindowCondition::Always, const Vec2& pivot = {0, 0});

/**
 * @brief Set next window size
 */
void set_next_window_size(const Vec2& size, WindowCondition condition = WindowCondition::Always);

/**
 * @brief Set next window size constraints
 */
void set_next_window_size_constraints(const Vec2& min_size, const Vec2& max_size);

/**
 * @brief Set next window collapsed state
 */
void set_next_window_collapsed(bool collapsed, WindowCondition condition = WindowCondition::Always);

/**
 * @brief Set next window focus
 */
void set_next_window_focus();

/**
 * @brief Set next window background alpha
 */
void set_next_window_bg_alpha(float alpha);

/**
 * @brief Get window position
 */
Vec2 get_window_pos();

/**
 * @brief Get window size
 */
Vec2 get_window_size();

/**
 * @brief Get window width
 */
float get_window_width();

/**
 * @brief Get window height
 */
float get_window_height();

/**
 * @brief Check if window is collapsed
 */
bool is_window_collapsed();

/**
 * @brief Check if window is focused
 */
bool is_window_focused(WindowFlags flags = WindowFlags::None);

/**
 * @brief Check if window is hovered
 */
bool is_window_hovered(WindowFlags flags = WindowFlags::None);

/**
 * @brief Get window draw list
 */
DrawList* get_window_draw_list();

} // namespace ecscope::gui