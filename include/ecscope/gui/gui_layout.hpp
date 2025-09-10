/**
 * @file gui_layout.hpp
 * @brief Flexible Layout System for GUI Framework
 * 
 * Advanced layout management with containers, splitters, docking, window
 * management, and responsive design capabilities.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#pragma once

#include "gui_core.hpp"
#include <stack>
#include <array>
#include <functional>
#include <optional>

namespace ecscope::gui {

// =============================================================================
// WINDOW SYSTEM
// =============================================================================

/**
 * @brief Window flags
 */
enum class WindowFlags : uint32_t {
    None = 0,
    NoTitleBar = 1 << 0,          // Disable title-bar
    NoResize = 1 << 1,            // Disable user resizing with the lower-right grip
    NoMove = 1 << 2,              // Disable user moving the window
    NoScrollbar = 1 << 3,         // Disable scrollbars (window can still scroll with mouse or programmatically)
    NoScrollWithMouse = 1 << 4,   // Disable user vertically scrolling with mouse wheel
    NoCollapse = 1 << 5,          // Disable user collapsing window by double-clicking on it
    AlwaysAutoResize = 1 << 6,    // Resize every window to its content every frame
    NoBackground = 1 << 7,        // Disable drawing background color and outside border
    NoSavedSettings = 1 << 8,     // Never load/save settings in .ini file
    NoMouseInputs = 1 << 9,       // Disable catching mouse
    MenuBar = 1 << 10,            // Has a menu-bar
    HorizontalScrollbar = 1 << 11,// Allow horizontal scrollbar to appear
    NoFocusOnAppearing = 1 << 12, // Disable taking focus when transitioning from hidden to visible state
    NoBringToFrontOnFocus = 1 << 13, // Disable bringing window to front when taking focus
    AlwaysVerticalScrollbar = 1 << 14, // Always show vertical scrollbar
    AlwaysHorizontalScrollbar = 1 << 15, // Always show horizontal scrollbar
    AlwaysUseWindowPadding = 1 << 16, // Ensure child windows without border use style.WindowPadding
    NoNavInputs = 1 << 18,        // No gamepad/keyboard navigation within the window
    NoNavFocus = 1 << 19,         // No focusing toward this window with gamepad/keyboard navigation
    UnsavedDocument = 1 << 20,    // Display a dot next to the title
    NoDocking = 1 << 21,          // Disable docking of this window
    
    NoNav = NoNavInputs | NoNavFocus,
    NoDecoration = NoTitleBar | NoResize | NoScrollbar | NoCollapse,
    NoInputs = NoMouseInputs | NoNavInputs | NoNavFocus,
    
    // Child window
    ChildWindow = 1 << 24,        // For internal use by BeginChild()
    Tooltip = 1 << 25,            // For internal use by BeginTooltip()
    Popup = 1 << 26,              // For internal use by BeginPopup()
    Modal = 1 << 27,              // For internal use by BeginPopupModal()
    ChildMenu = 1 << 28           // For internal use by BeginMenu()
};

inline WindowFlags operator|(WindowFlags a, WindowFlags b) {
    return static_cast<WindowFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline WindowFlags operator&(WindowFlags a, WindowFlags b) {
    return static_cast<WindowFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

/**
 * @brief Window condition flags for SetNext* functions
 */
enum class Cond : int {
    None = 0,        // No condition (always set the variable), same as _Always
    Always = 1 << 0, // No condition (always set the variable)
    Once = 1 << 1,   // Set the variable once per runtime session (only the first call will succeed)
    FirstUseEver = 1 << 2, // Set the variable if the object/window has no persistently saved data (no entry in .ini file)
    Appearing = 1 << 3     // Set the variable if the object/window is appearing after being hidden/inactive (or the first time)
};

/**
 * @brief Window functions
 */
bool begin(const std::string& name, bool* p_open = nullptr, WindowFlags flags = WindowFlags::None);
void end();
bool begin_child(const std::string& str_id, const Vec2& size = Vec2{0, 0}, 
                bool border = false, WindowFlags flags = WindowFlags::None);
bool begin_child(GuiID id, const Vec2& size = Vec2{0, 0}, 
                bool border = false, WindowFlags flags = WindowFlags::None);
void end_child();

// Window utilities
bool is_window_appearing();
bool is_window_collapsed();
bool is_window_focused(int flags = 0);
bool is_window_hovered(int flags = 0);
Vec2 get_window_pos();
Vec2 get_window_size();
float get_window_width();
float get_window_height();

// Window manipulation
void set_next_window_pos(const Vec2& pos, Cond cond = Cond::None, const Vec2& pivot = Vec2{0, 0});
void set_next_window_size(const Vec2& size, Cond cond = Cond::None);
void set_next_window_size_constraints(const Vec2& size_min, const Vec2& size_max, 
                                     std::function<void(Vec2&)> custom_callback = nullptr);
void set_next_window_content_size(const Vec2& size);
void set_next_window_collapsed(bool collapsed, Cond cond = Cond::None);
void set_next_window_focus();
void set_next_window_bg_alpha(float alpha);
void set_window_pos(const Vec2& pos, Cond cond = Cond::None);
void set_window_size(const Vec2& size, Cond cond = Cond::None);
void set_window_collapsed(bool collapsed, Cond cond = Cond::None);
void set_window_focus();
void set_window_focus(const std::string& name);

// Content region
Vec2 get_content_region_avail();
Vec2 get_content_region_max();
Vec2 get_window_content_region_min();
Vec2 get_window_content_region_max();

// Scrolling
float get_scroll_x();
float get_scroll_y();
void set_scroll_x(float scroll_x);
void set_scroll_y(float scroll_y);
float get_scroll_max_x();
float get_scroll_max_y();
void set_scroll_here_x(float center_x_ratio = 0.5f);
void set_scroll_here_y(float center_y_ratio = 0.5f);
void set_scroll_from_pos_x(float local_x, float center_x_ratio = 0.5f);
void set_scroll_from_pos_y(float local_y, float center_y_ratio = 0.5f);

// =============================================================================
// LAYOUT CONTAINERS
// =============================================================================

/**
 * @brief Layout direction
 */
enum class LayoutDirection : uint8_t {
    Horizontal,
    Vertical
};

/**
 * @brief Layout alignment
 */
enum class LayoutAlign : uint8_t {
    Start,      // Left/Top
    Center,     // Center
    End,        // Right/Bottom
    Stretch     // Fill available space
};

/**
 * @brief Layout justify (main axis alignment)
 */
enum class LayoutJustify : uint8_t {
    Start,         // Pack to start
    End,           // Pack to end
    Center,        // Pack to center
    SpaceBetween,  // Space between items
    SpaceAround,   // Space around items
    SpaceEvenly    // Even space distribution
};

/**
 * @brief Layout container (Flexbox-like)
 */
class LayoutContainer {
public:
    LayoutContainer(LayoutDirection direction = LayoutDirection::Vertical);
    ~LayoutContainer();
    
    // Container properties
    void set_direction(LayoutDirection direction) { direction_ = direction; }
    void set_wrap(bool wrap) { wrap_ = wrap; }
    void set_justify_content(LayoutJustify justify) { justify_content_ = justify; }
    void set_align_items(LayoutAlign align) { align_items_ = align; }
    void set_align_content(LayoutAlign align) { align_content_ = align; }
    void set_gap(float gap) { gap_ = gap; }
    void set_padding(float padding) { padding_ = {padding, padding, padding, padding}; }
    void set_padding(float horizontal, float vertical) { padding_ = {horizontal, vertical, horizontal, vertical}; }
    void set_padding(float top, float right, float bottom, float left) { padding_ = {top, right, bottom, left}; }
    
    // Item management
    void begin_item(float flex_grow = 0.0f, float flex_shrink = 1.0f, float flex_basis = -1.0f);
    void end_item();
    
    // Manual layout
    void calculate_layout(const Vec2& available_size);
    Vec2 get_computed_size() const { return computed_size_; }
    
private:
    struct LayoutItem {
        float flex_grow = 0.0f;
        float flex_shrink = 1.0f;
        float flex_basis = -1.0f; // -1 = auto
        Vec2 min_size = {0, 0};
        Vec2 max_size = {FLT_MAX, FLT_MAX};
        Vec2 computed_size = {0, 0};
        Vec2 computed_position = {0, 0};
        LayoutAlign align_self = static_cast<LayoutAlign>(-1); // -1 = inherit from container
        
        Rect content_rect = {{0, 0}, {0, 0}};
        GuiID item_id = 0;
        bool visible = true;
    };
    
    LayoutDirection direction_ = LayoutDirection::Vertical;
    bool wrap_ = false;
    LayoutJustify justify_content_ = LayoutJustify::Start;
    LayoutAlign align_items_ = LayoutAlign::Stretch;
    LayoutAlign align_content_ = LayoutAlign::Start;
    float gap_ = 0.0f;
    std::array<float, 4> padding_ = {0, 0, 0, 0}; // top, right, bottom, left
    
    std::vector<LayoutItem> items_;
    Vec2 computed_size_ = {0, 0};
    bool layout_dirty_ = true;
    
    void calculate_main_sizes(const Vec2& available_size);
    void calculate_cross_sizes(const Vec2& available_size);
    void position_items(const Vec2& available_size);
    float get_main_axis_size(const Vec2& size) const;
    float get_cross_axis_size(const Vec2& size) const;
    void set_main_axis_size(Vec2& size, float value) const;
    void set_cross_axis_size(Vec2& size, float value) const;
};

// Helper functions for layout containers
void begin_horizontal_layout(float gap = 0.0f, LayoutJustify justify = LayoutJustify::Start);
void begin_vertical_layout(float gap = 0.0f, LayoutJustify justify = LayoutJustify::Start);
void end_layout();
void layout_item(float flex_grow = 0.0f, float flex_shrink = 1.0f, float flex_basis = -1.0f);

// =============================================================================
// GRID LAYOUT
// =============================================================================

/**
 * @brief Grid layout container
 */
class GridLayout {
public:
    GridLayout(int columns, int rows = -1); // -1 = auto rows
    ~GridLayout();
    
    void set_column_widths(std::span<const float> widths);
    void set_row_heights(std::span<const float> heights);
    void set_gap(float column_gap, float row_gap);
    void set_padding(float padding);
    void set_padding(float horizontal, float vertical);
    void set_padding(float top, float right, float bottom, float left);
    
    // Grid item placement
    void begin_item(int column, int row, int column_span = 1, int row_span = 1);
    void end_item();
    void next_item(); // Auto-advance to next grid cell
    
    // Utilities
    Vec2 get_cell_size(int column, int row) const;
    Vec2 get_cell_position(int column, int row) const;
    void skip_cells(int count = 1); // Skip cells in auto-advance mode
    
private:
    int columns_;
    int rows_;
    std::vector<float> column_widths_;
    std::vector<float> row_heights_;
    float column_gap_ = 0.0f;
    float row_gap_ = 0.0f;
    std::array<float, 4> padding_ = {0, 0, 0, 0};
    
    int current_column_ = 0;
    int current_row_ = 0;
    bool auto_advance_ = true;
    
    Vec2 available_size_ = {0, 0};
    Vec2 computed_size_ = {0, 0};
    
    void calculate_grid_sizes();
    void ensure_grid_capacity(int column, int row);
};

// Helper functions for grid layout
void begin_grid(int columns, int rows = -1, float column_gap = 0.0f, float row_gap = 0.0f);
void end_grid();
void grid_item(int column, int row, int column_span = 1, int row_span = 1);
void grid_next_item();

// =============================================================================
// SPLITTER WIDGETS
// =============================================================================

/**
 * @brief Splitter flags
 */
enum class SplitterFlags : uint32_t {
    None = 0,
    Horizontal = 1 << 0,  // Horizontal splitter (default is vertical)
    NoResize = 1 << 1,    // Disable resizing
    NoBorder = 1 << 2,    // No border around splitter
    NoBackground = 1 << 3 // No background fill
};

inline SplitterFlags operator|(SplitterFlags a, SplitterFlags b) {
    return static_cast<SplitterFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

/**
 * @brief Splitter widget for resizable panels
 */
bool splitter(const std::string& str_id, float* size1, float* size2, float min_size1, float min_size2,
             float splitter_long_axis_size = -1.0f, float splitter_short_axis_size = 4.0f,
             SplitterFlags flags = SplitterFlags::None);

// Helper functions
bool begin_splitter_pane(const std::string& str_id, const Vec2& size, SplitterFlags flags = SplitterFlags::None);
void end_splitter_pane();

// =============================================================================
// TAB SYSTEM
// =============================================================================

/**
 * @brief Tab bar flags
 */
enum class TabBarFlags : uint32_t {
    None = 0,
    Reorderable = 1 << 0,          // Allow manually dragging tabs to re-order them + New tabs are appended at the end of list
    AutoSelectNewTabs = 1 << 1,    // Automatically select new tabs when they appear
    TabListPopupButton = 1 << 2,   // Disable buttons to open the tab list popup
    NoCloseWithMiddleMouseButton = 1 << 3, // Disable behavior of closing tabs with middle mouse button
    NoTabListScrollingButtons = 1 << 4,    // Disable scrolling buttons (apply when fitting policy is ImGuiTabBarFlags_FittingPolicyScroll)
    NoTooltip = 1 << 5,            // Disable tooltips when hovering a tab
    FittingPolicyResizeDown = 1 << 6,      // Resize tabs when they don't fit
    FittingPolicyScroll = 1 << 7,          // Add scroll buttons when tabs don't fit
    FittingPolicyMask = FittingPolicyResizeDown | FittingPolicyScroll,
    FittingPolicyDefault = FittingPolicyResizeDown
};

inline TabBarFlags operator|(TabBarFlags a, TabBarFlags b) {
    return static_cast<TabBarFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

/**
 * @brief Tab item flags
 */
enum class TabItemFlags : uint32_t {
    None = 0,
    UnsavedDocument = 1 << 0,      // Display a dot next to the title + tab is selected when clicking the X + closure is not assumed
    SetSelected = 1 << 1,          // Trigger flag to programmatically make the tab selected when calling BeginTabItem()
    NoCloseWithMiddleMouseButton = 1 << 2, // Disable behavior of closing tabs with middle mouse button
    NoPushId = 1 << 3,             // Don't call PushID(tab->ID)/PopID() on BeginTabItem()/EndTabItem()
    NoTooltip = 1 << 4,            // Disable tooltip for the given tab
    NoReorder = 1 << 5,            // Disable reordering this tab or having another tab cross over this tab
    Leading = 1 << 6,              // Enforce the tab position to the left of the tab bar (after the tab list popup button)
    Trailing = 1 << 7              // Enforce the tab position to the right of the tab bar (before the scrolling buttons)
};

inline TabItemFlags operator|(TabItemFlags a, TabItemFlags b) {
    return static_cast<TabItemFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

/**
 * @brief Tab system functions
 */
bool begin_tab_bar(const std::string& str_id, TabBarFlags flags = TabBarFlags::None);
void end_tab_bar();
bool begin_tab_item(const std::string& label, bool* p_open = nullptr, TabItemFlags flags = TabItemFlags::None);
void end_tab_item();
bool tab_item_button(const std::string& label, TabItemFlags flags = TabItemFlags::None);
void set_tab_item_closed(const std::string& tab_or_docked_window_label);

// =============================================================================
// DOCKING SYSTEM
// =============================================================================

/**
 * @brief Docking flags
 */
enum class DockNodeFlags : uint32_t {
    None = 0,
    KeepAliveOnly = 1 << 0,        // Prevent dockspace from being split/merged/resized
    NoDockingInCentralNode = 1 << 2, // Disable docking in the Central Node
    PassthruCentralNode = 1 << 3,    // Enable passthru dockspace
    NoSplit = 1 << 4,              // Disable splitting the node
    NoResize = 1 << 5,             // Disable resizing
    AutoHideTabBar = 1 << 6        // Tab bar will automatically hide when there's a single window in the dock node
};

inline DockNodeFlags operator|(DockNodeFlags a, DockNodeFlags b) {
    return static_cast<DockNodeFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

/**
 * @brief Docking functions
 */
GuiID dockspace(GuiID id, const Vec2& size = Vec2{0, 0}, DockNodeFlags flags = DockNodeFlags::None);
GuiID dockspace_over_viewport(DockNodeFlags flags = DockNodeFlags::None);
void set_next_window_dock_id(GuiID dock_id, Cond cond = Cond::None);
void set_next_window_class(const std::string& class_name);
GuiID get_window_dock_id();
bool is_window_docked();

// Docking builder API (for programmatic docking setup)
struct DockBuilder {
    static void dock_builder_dock_window(const std::string& window_name, GuiID node_id);
    static GuiID dock_builder_add_node(GuiID node_id = 0, DockNodeFlags flags = DockNodeFlags::None);
    static void dock_builder_remove_node(GuiID node_id);
    static void dock_builder_remove_node_docked_windows(GuiID node_id, bool clear_settings_refs = true);
    static void dock_builder_remove_node_child_nodes(GuiID node_id);
    static void dock_builder_set_node_pos(GuiID node_id, Vec2 pos);
    static void dock_builder_set_node_size(GuiID node_id, Vec2 size);
    static GuiID dock_builder_split_node(GuiID node_id, LayoutDirection split_dir, float size_ratio_for_node_at_dir, 
                                        GuiID* out_id_at_dir, GuiID* out_id_at_opposite_dir);
    static void dock_builder_finish(GuiID node_id);
};

// =============================================================================
// POPUP SYSTEM
// =============================================================================

/**
 * @brief Popup flags
 */
enum class PopupFlags : uint32_t {
    None = 0,
    MouseButtonLeft = 0,        // For BeginPopupContext*(): open on Left Mouse release
    MouseButtonRight = 1,       // For BeginPopupContext*(): open on Right Mouse release  
    MouseButtonMiddle = 2,      // For BeginPopupContext*(): open on Middle Mouse release
    MouseButtonMask = 0x1F,
    MouseButtonDefault = 1,
    NoOpenOverExistingPopup = 1 << 5, // For OpenPopup*(), don't open if there's already a popup open at the same level
    NoOpenOverItems = 1 << 6,         // For BeginPopupContextWindow(): don't return true when hovering items, only when hovering empty space
    AnyPopupId = 1 << 7,              // For IsPopupOpen(): ignore the ImGuiID parameter and test for any popup
    AnyPopupLevel = 1 << 8,           // For IsPopupOpen(): search/test at any level of the popup stack (default test only at current level)
    AnyPopup = AnyPopupId | AnyPopupLevel
};

inline PopupFlags operator|(PopupFlags a, PopupFlags b) {
    return static_cast<PopupFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

/**
 * @brief Popup functions
 */
void open_popup(const std::string& str_id, PopupFlags popup_flags = PopupFlags::MouseButtonDefault);
void open_popup(GuiID id, PopupFlags popup_flags = PopupFlags::MouseButtonDefault);
void open_popup_on_item_click(const std::string& str_id = "", PopupFlags popup_flags = PopupFlags::MouseButtonRight);
void close_current_popup();

bool begin_popup(const std::string& str_id, WindowFlags flags = WindowFlags::None);
bool begin_popup_modal(const std::string& name, bool* p_open = nullptr, WindowFlags flags = WindowFlags::None);
void end_popup();

// Context popups
bool begin_popup_context_item(const std::string& str_id = "", PopupFlags popup_flags = PopupFlags::MouseButtonDefault);
bool begin_popup_context_window(const std::string& str_id = "", PopupFlags popup_flags = PopupFlags::MouseButtonDefault);
bool begin_popup_context_void(const std::string& str_id = "", PopupFlags popup_flags = PopupFlags::MouseButtonDefault);

// Query popup state
bool is_popup_open(const std::string& str_id, PopupFlags flags = PopupFlags::None);
bool is_popup_open(GuiID id, PopupFlags flags = PopupFlags::None);

// =============================================================================
// MENU SYSTEM
// =============================================================================

/**
 * @brief Menu functions
 */
bool begin_main_menu_bar();
void end_main_menu_bar();
bool begin_menu_bar();
void end_menu_bar();
bool begin_menu(const std::string& label, bool enabled = true);
void end_menu();
bool menu_item(const std::string& label, const std::string& shortcut = "", bool selected = false, bool enabled = true);
bool menu_item(const std::string& label, const std::string& shortcut, bool* p_selected, bool enabled = true);

// =============================================================================
// TOOLTIP SYSTEM
// =============================================================================

/**
 * @brief Tooltip functions
 */
void begin_tooltip();
void end_tooltip();
void set_tooltip(const std::string& text);
void set_tooltip_v(const std::string& fmt, ...);

bool begin_item_tooltip();
void set_item_tooltip(const std::string& text);
void set_item_tooltip_v(const std::string& fmt, ...);

// =============================================================================
// TABLE SYSTEM
// =============================================================================

/**
 * @brief Table flags
 */
enum class TableFlags : uint32_t {
    None = 0,
    
    // Features
    Resizable = 1 << 0,   // Enable resizing columns
    Reorderable = 1 << 1, // Enable reordering columns in header row (need calling TableSetupColumn() + TableHeadersRow() to display headers)
    Hideable = 1 << 2,    // Enable hiding/disabling columns in context menu
    Sortable = 1 << 3,    // Enable sorting. Call TableGetSortSpecs() to obtain sort specs. Also see ImGuiTableFlags_SortMulti and ImGuiTableFlags_SortTristate
    NoSavedSettings = 1 << 4, // Disable persisting columns order, width and sort settings in the .ini file
    ContextMenuInBody = 1 << 5, // Right-click on columns body/contents will display table context menu. By default it is available in TableHeadersRow()
    
    // Decorations
    RowBg = 1 << 6,           // Set each RowBg color with ImGuiCol_TableRowBg or ImGuiCol_TableRowBgAlt (equivalent of calling TableSetBgColor)
    BordersInnerH = 1 << 7,   // Draw horizontal borders between rows
    BordersOuterH = 1 << 8,   // Draw horizontal borders at the top and bottom
    BordersInnerV = 1 << 9,   // Draw vertical borders between columns
    BordersOuterV = 1 << 10,  // Draw vertical borders on the left and right sides
    BordersH = BordersInnerH | BordersOuterH, // Draw horizontal borders
    BordersV = BordersInnerV | BordersOuterV, // Draw vertical borders
    BordersInner = BordersInnerV | BordersInnerH, // Draw inner borders
    BordersOuter = BordersOuterV | BordersOuterH, // Draw outer borders
    Borders = BordersInner | BordersOuter, // Draw all borders
    NoBordersInBody = 1 << 11, // Disable vertical borders in columns Body (borders will always appears in Headers)
    NoBordersInBodyUntilResize = 1 << 12, // Disable vertical borders in columns Body until hovered for resize (borders will always appears in Headers)
    
    // Sizing Policy
    SizingFixedFit = 1 << 13,    // Columns default to _WidthFixed or _WidthAuto (if resizable or not resizable), matching contents width
    SizingFixedSame = 2 << 13,   // Columns default to _WidthFixed or _WidthAuto (if resizable or not resizable), matching the maximum contents width of all columns
    SizingStretchProp = 3 << 13, // Columns default to _WidthStretch with default weights proportional to each columns contents widths
    SizingStretchSame = 4 << 13, // Columns default to _WidthStretch with default weights all equal, unless overridden by TableSetupColumn()
    
    // Sizing Extra Options
    NoHostExtendX = 1 << 16,     // Make outer width auto-fit to columns, overriding outer_size.x value. Only available when ScrollX/ScrollY are disabled and Stretch columns are not used
    NoHostExtendY = 1 << 17,     // Make outer height stop exactly at outer_size.y (prevent auto-extending table past the limit)
    NoKeepColumnsVisible = 1 << 18, // Disable keeping column always minimally visible when ScrollX is off and table gets too small
    PreciseWidths = 1 << 19,     // Disable distributing remainder width to stretched columns (width allocation on a 100-wide table with 3 columns: Without this flag: 33,33,34. With this flag: 33,33,33)
    
    // Clipping
    NoClip = 1 << 20,            // Disable clipping rectangle for every individual columns
    
    // Padding
    PadOuterX = 1 << 21,         // Default if BordersOuterV is on. Enable outer-most padding. Generally desirable if you have headers
    NoPadOuterX = 1 << 22,       // Default if BordersOuterV is off. Disable outer-most padding
    NoPadInnerX = 1 << 23,       // Disable inner padding between columns (double inner padding if BordersOuterV is on, single inner padding if BordersOuterV is off)
    
    // Scrolling
    ScrollX = 1 << 24,           // Enable horizontal scrolling. Require 'outer_size' parameter of BeginTable() to specify the container size. Changes default sizing policy. Because this create a child window, ScrollY is currently generally recommended when using ScrollX
    ScrollY = 1 << 25,           // Enable vertical scrolling. Require 'outer_size' parameter of BeginTable() to specify the container size
    
    // Sorting
    SortMulti = 1 << 26,         // Hold shift when clicking headers to sort on multiple column. TableGetSortSpecs() may return specs where (SpecsCount > 1)
    SortTristate = 1 << 27       // Allow no sorting, disable default sorting. TableGetSortSpecs() may return specs where (SpecsCount == 0)
};

inline TableFlags operator|(TableFlags a, TableFlags b) {
    return static_cast<TableFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

/**
 * @brief Table column flags
 */
enum class TableColumnFlags : uint32_t {
    None = 0,
    Disabled = 1 << 0,           // Overriding/master disable flag: hide column, won't show in context menu (unlike calling TableSetColumnEnabled() which manipulates the user accessible state)
    DefaultHide = 1 << 1,        // Default as a hidden/disabled column
    DefaultSort = 1 << 2,        // Default as a sorting column
    WidthStretch = 1 << 3,       // Column will stretch. Preferable with horizontal scrolling disabled (default if table sizing policy is _SizingStretchSame or _SizingStretchProp)
    WidthFixed = 1 << 4,         // Column will not stretch. Preferable with horizontal scrolling enabled (default if table sizing policy is _SizingFixedFit and table is resizable)
    NoResize = 1 << 5,           // Disable manual resizing
    NoReorder = 1 << 6,          // Disable manual reordering this column, this will also prevent other columns from crossing over this column
    NoHide = 1 << 7,             // Disable ability to hide/disable this column
    NoClip = 1 << 8,             // Disable clipping for this column (all NoClip columns will render in a same draw command)
    NoSort = 1 << 9,             // Disable ability to sort on this field (even if ImGuiTableFlags_Sortable is set on the table)
    NoSortAscending = 1 << 10,   // Disable ability to sort in the ascending direction
    NoSortDescending = 1 << 11,  // Disable ability to sort in the descending direction
    NoHeaderLabel = 1 << 12,     // TableHeadersRow() will not submit label for this column. Convenient for some small columns. Name will still appear in context menu
    NoHeaderWidth = 1 << 13,     // Disable header text width contribution to automatic column width
    PreferSortAscending = 1 << 14,  // Make the initial sort direction Ascending when first sorting on this column (default)
    PreferSortDescending = 1 << 15, // Make the initial sort direction Descending when first sorting on this column
    IndentEnable = 1 << 16,      // Use current Indent value when entering cell (default for column 0)
    IndentDisable = 1 << 17,     // Ignore current Indent value when entering cell (default for columns > 0). Indentation changes _within_ the cell will still be honored
    
    // Output status flags, read-only via TableGetColumnFlags()
    IsEnabled = 1 << 24,         // Status: is enabled == not hidden
    IsVisible = 1 << 25,         // Status: is visible == is enabled AND not clipped by scrolling
    IsSorted = 1 << 26,          // Status: is currently part of the sort specs
    IsHovered = 1 << 27,         // Status: is hovered by mouse
    
    WidthMask = WidthStretch | WidthFixed,
    IndentMask = IndentEnable | IndentDisable,
    StatusMask = IsEnabled | IsVisible | IsSorted | IsHovered,
    NoDirectResize = 1 << 30     // [Internal] Disable user resizing this column directly (it may still be resized indirectly from its left edge)
};

inline TableColumnFlags operator|(TableColumnFlags a, TableColumnFlags b) {
    return static_cast<TableColumnFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

/**
 * @brief Table row flags
 */
enum class TableRowFlags : uint32_t {
    None = 0,
    Headers = 1 << 0    // Identify header row (set default background color + width of its contents accounted differently for auto column width)
};

/**
 * @brief Table background target
 */
enum class TableBgTarget : uint8_t {
    None = 0,
    RowBg0 = 1,         // Set row background color 0 (generally used for background, automatically set when ImGuiTableFlags_RowBg is used)
    RowBg1 = 2,         // Set row background color 1 (generally used for selection marking)
    CellBg = 3          // Set cell background color (top-most color)
};

/**
 * @brief Table sort specs
 */
struct TableColumnSortSpecs {
    GuiID column_user_id = 0;    // User id of the column (if specified by a TableSetupColumn() call)
    int16_t column_index = -1;    // Index of the column
    int16_t sort_order = -1;      // Index within parent ImGuiTableSortSpecs (always stored in order starting from 0, tables sorted on a single criteria will always have a 0 here)
    int sort_direction = 0;       // ImGuiSortDirection_Ascending or ImGuiSortDirection_Descending (you can use this or SortSign, whichever is more convenient for your sort function)
    
    bool is_ascending() const { return sort_direction == 1; }
    bool is_descending() const { return sort_direction == -1; }
};

struct TableSortSpecs {
    const TableColumnSortSpecs* specs = nullptr; // Pointer to sort spec array
    int specs_count = 0;                         // Sort spec count. Most often 1. May be > 1 when ImGuiTableFlags_SortMulti is enabled. May be == 0 when ImGuiTableFlags_SortTristate is enabled
    bool specs_dirty = false;                    // Set to true when specs have changed since last time! Use this to sort your data. Your code should clear this to false
};

/**
 * @brief Table functions
 */
bool begin_table(const std::string& str_id, int column, TableFlags flags = TableFlags::None, 
                const Vec2& outer_size = Vec2{0.0f, 0.0f}, float inner_width = 0.0f);
void end_table();
void table_next_row(TableRowFlags row_flags = TableRowFlags::None, float min_row_height = 0.0f);
bool table_next_column();
bool table_set_column_index(int column_n);

// Tables: Headers & Columns declaration
void table_setup_column(const std::string& label, TableColumnFlags flags = TableColumnFlags::None, 
                       float init_width_or_weight = 0.0f, GuiID user_id = 0);
void table_setup_scroll_freeze(int cols, int rows);
void table_headers_row();
void table_header(const std::string& label);

// Tables: Sorting & miscellaneous functions
TableSortSpecs* table_get_sort_specs();
int table_get_column_count();
int table_get_column_index();
int table_get_row_index();
std::string table_get_column_name(int column_n = -1);
TableColumnFlags table_get_column_flags(int column_n = -1);
void table_set_column_enabled(int column_n, bool v);
void table_set_bg_color(TableBgTarget target, uint32_t color, int column_n = -1);

// =============================================================================
// LAYOUT UTILITIES
// =============================================================================

/**
 * @brief Layout state management
 */
struct LayoutState {
    Vec2 cursor_pos = {0, 0};
    Vec2 cursor_max_pos = {0, 0};
    Vec2 cursor_start_pos = {0, 0};
    Vec2 content_region_rect_max = {0, 0};
    float current_line_height = 0.0f;
    float prev_line_height = 0.0f;
    float item_spacing_x = 8.0f;
    float item_spacing_y = 4.0f;
    float frame_padding_x = 8.0f;
    float frame_padding_y = 4.0f;
    bool same_line = false;
    int tree_depth = 0;
    GuiID last_item_id = 0;
    
    void reset() {
        cursor_pos = cursor_max_pos = cursor_start_pos = {0, 0};
        current_line_height = prev_line_height = 0.0f;
        same_line = false;
        tree_depth = 0;
        last_item_id = 0;
    }
};

LayoutState* get_current_layout_state();
void push_layout_state();
void pop_layout_state();

// Layout helpers
void push_item_width(float item_width);
void pop_item_width();
void set_next_item_width(float item_width);
float calc_item_width();
void push_text_wrap_pos(float wrap_local_pos_x = 0.0f);
void pop_text_wrap_pos();

// =============================================================================
// RESPONSIVE DESIGN HELPERS
// =============================================================================

/**
 * @brief Responsive breakpoints
 */
enum class Breakpoint : uint8_t {
    Mobile,    // < 768px
    Tablet,    // >= 768px && < 1024px
    Desktop,   // >= 1024px && < 1440px
    Wide       // >= 1440px
};

/**
 * @brief Get current responsive breakpoint
 */
Breakpoint get_current_breakpoint();

/**
 * @brief Conditional layout based on breakpoint
 */
template<typename Func>
void responsive_layout(Breakpoint min_breakpoint, Func&& func) {
    if (get_current_breakpoint() >= min_breakpoint) {
        func();
    }
}

/**
 * @brief Responsive columns
 */
void begin_responsive_columns(int mobile_cols, int tablet_cols, int desktop_cols, int wide_cols);
void end_responsive_columns();

} // namespace ecscope::gui