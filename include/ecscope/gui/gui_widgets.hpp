/**
 * @file gui_widgets.hpp
 * @brief Complete Widget System for GUI Framework
 * 
 * Comprehensive collection of immediate-mode GUI widgets including buttons,
 * sliders, inputs, checkboxes, combo boxes, tables, and advanced controls.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#pragma once

#include "gui_core.hpp"
#include "gui_text.hpp"
#include <functional>
#include <variant>
#include <optional>
#include <span>

namespace ecscope::gui {

// =============================================================================
// WIDGET FLAGS AND OPTIONS
// =============================================================================

/**
 * @brief Generic widget flags
 */
enum class WidgetFlags : uint32_t {
    None = 0,
    Disabled = 1 << 0,
    ReadOnly = 1 << 1,
    NoBackground = 1 << 2,
    NoBorder = 1 << 3,
    NoFocus = 1 << 4,
    AlwaysAutoResize = 1 << 5,
    NoScrollbar = 1 << 6,
    NoClipping = 1 << 7,
    AllowKeyboardFocus = 1 << 8,
    AllowOverlap = 1 << 9
};

inline WidgetFlags operator|(WidgetFlags a, WidgetFlags b) {
    return static_cast<WidgetFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline WidgetFlags operator&(WidgetFlags a, WidgetFlags b) {
    return static_cast<WidgetFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline bool has_flag(WidgetFlags flags, WidgetFlags flag) {
    return (flags & flag) != WidgetFlags::None;
}

// =============================================================================
// BASIC WIDGETS
// =============================================================================

/**
 * @brief Button widget
 */
bool button(const std::string& label, const Vec2& size = Vec2{0, 0});
bool button_colored(const std::string& label, const Color& color, const Vec2& size = Vec2{0, 0});
bool button_small(const std::string& label);
bool button_invisible(const std::string& label, const Vec2& size);

// Button variations
bool arrow_button(const std::string& str_id, NavDirection direction);
bool image_button(const std::string& str_id, uint32_t texture_id, const Vec2& size, 
                 const Vec2& uv0 = Vec2{0, 0}, const Vec2& uv1 = Vec2{1, 1},
                 const Color& tint_color = Color::WHITE, const Color& bg_color = Color::TRANSPARENT);

/**
 * @brief Checkbox widget
 */
bool checkbox(const std::string& label, bool* value);
bool checkbox_flags(const std::string& label, uint32_t* flags, uint32_t flags_value);

/**
 * @brief Radio button widget
 */
bool radio_button(const std::string& label, bool active);
bool radio_button(const std::string& label, int* value, int button_value);

// =============================================================================
// TEXT INPUT WIDGETS
// =============================================================================

/**
 * @brief Text input flags
 */
enum class InputTextFlags : uint32_t {
    None = 0,
    CharsDecimal = 1 << 0,        // Allow 0123456789.+-
    CharsHexadecimal = 1 << 1,    // Allow 0123456789ABCDEFabcdef
    CharsUppercase = 1 << 2,      // Turn a..z into A..Z
    CharsNoBlank = 1 << 3,        // Filter out spaces, tabs
    AutoSelectAll = 1 << 4,       // Select entire text when first taking mouse focus
    EnterReturnsTrue = 1 << 5,    // Return 'true' when Enter is pressed
    CallbackCompletion = 1 << 6,  // Call user function on pressing TAB
    CallbackHistory = 1 << 7,     // Call user function on pressing Up/Down arrows
    CallbackAlways = 1 << 8,      // Call user function every time
    CallbackCharFilter = 1 << 9,  // Call user function to filter character
    AllowTabInput = 1 << 10,      // Pressing TAB input a '\t' character
    CtrlEnterForNewLine = 1 << 11, // In multi-line mode, unfocus with Enter, add new line with Ctrl+Enter
    NoHorizontalScroll = 1 << 12, // Disable following the cursor horizontally
    AlwaysOverwrite = 1 << 13,    // Overwrite mode
    ReadOnly = 1 << 14,           // Read-only mode
    Password = 1 << 15,           // Password mode, display all characters as '*'
    NoUndoRedo = 1 << 16,         // Disable undo/redo
    CharsScientific = 1 << 17,    // Allow 0123456789.+-*/eE
    CallbackResize = 1 << 18,     // Callback to resize string size
    CallbackEdit = 1 << 19        // Callback on any edit
};

inline InputTextFlags operator|(InputTextFlags a, InputTextFlags b) {
    return static_cast<InputTextFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

/**
 * @brief Input text callback data
 */
struct InputTextCallbackData {
    InputTextFlags event_flag;
    WidgetFlags flags;
    void* user_data;
    
    // Text buffer
    Codepoint event_char;        // Character input
    Key event_key;               // Key pressed
    std::string* buf;            // Text buffer
    size_t buf_text_len;         // Text length (in bytes)
    bool buf_dirty;              // Set if you modify buf/buf_text_len
    
    // Cursor/selection
    int cursor_pos;              // Current cursor position
    int selection_start;         // Selection start position
    int selection_end;           // Selection end position
    
    // Helper functions
    void delete_chars(int pos, int bytes_count);
    void insert_chars(int pos, const char* text, const char* text_end = nullptr);
    void select_all() { selection_start = 0; selection_end = static_cast<int>(buf_text_len); }
    void clear_selection() { selection_start = selection_end = cursor_pos; }
    bool has_selection() const { return selection_start != selection_end; }
};

using InputTextCallback = std::function<int(InputTextCallbackData*)>;

/**
 * @brief Text input widgets
 */
bool input_text(const std::string& label, std::string& buf, InputTextFlags flags = InputTextFlags::None, 
                InputTextCallback callback = nullptr, void* user_data = nullptr);
bool input_text_multiline(const std::string& label, std::string& buf, const Vec2& size = Vec2{0, 0},
                          InputTextFlags flags = InputTextFlags::None, InputTextCallback callback = nullptr, 
                          void* user_data = nullptr);
bool input_text_with_hint(const std::string& label, const std::string& hint, std::string& buf,
                          InputTextFlags flags = InputTextFlags::None, InputTextCallback callback = nullptr, 
                          void* user_data = nullptr);

// Numeric input widgets
bool input_float(const std::string& label, float* value, float step = 0.0f, float step_fast = 0.0f, 
                const std::string& format = "%.3f", InputTextFlags flags = InputTextFlags::None);
bool input_float2(const std::string& label, float value[2], const std::string& format = "%.3f", 
                 InputTextFlags flags = InputTextFlags::None);
bool input_float3(const std::string& label, float value[3], const std::string& format = "%.3f", 
                 InputTextFlags flags = InputTextFlags::None);
bool input_float4(const std::string& label, float value[4], const std::string& format = "%.3f", 
                 InputTextFlags flags = InputTextFlags::None);

bool input_int(const std::string& label, int* value, int step = 1, int step_fast = 100, 
               InputTextFlags flags = InputTextFlags::None);
bool input_int2(const std::string& label, int value[2], InputTextFlags flags = InputTextFlags::None);
bool input_int3(const std::string& label, int value[3], InputTextFlags flags = InputTextFlags::None);
bool input_int4(const std::string& label, int value[4], InputTextFlags flags = InputTextFlags::None);

bool input_double(const std::string& label, double* value, double step = 0.0, double step_fast = 0.0,
                 const std::string& format = "%.6f", InputTextFlags flags = InputTextFlags::None);

// =============================================================================
// SLIDER WIDGETS
// =============================================================================

/**
 * @brief Slider flags
 */
enum class SliderFlags : uint32_t {
    None = 0,
    AlwaysClamp = 1 << 0,        // Clamp value to min/max bounds when input manually with CTRL+Click
    Logarithmic = 1 << 1,        // Make the widget logarithmic (linear otherwise)
    NoRoundToFormat = 1 << 2,    // Disable rounding underlying value to match precision of display format string
    NoInput = 1 << 3             // Disable CTRL+Click or Enter key allowing to input text directly
};

inline SliderFlags operator|(SliderFlags a, SliderFlags b) {
    return static_cast<SliderFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

/**
 * @brief Slider widgets
 */
bool slider_float(const std::string& label, float* value, float v_min, float v_max, 
                 const std::string& format = "%.3f", SliderFlags flags = SliderFlags::None);
bool slider_float2(const std::string& label, float value[2], float v_min, float v_max, 
                  const std::string& format = "%.3f", SliderFlags flags = SliderFlags::None);
bool slider_float3(const std::string& label, float value[3], float v_min, float v_max, 
                  const std::string& format = "%.3f", SliderFlags flags = SliderFlags::None);
bool slider_float4(const std::string& label, float value[4], float v_min, float v_max, 
                  const std::string& format = "%.3f", SliderFlags flags = SliderFlags::None);

bool slider_int(const std::string& label, int* value, int v_min, int v_max, 
               const std::string& format = "%d", SliderFlags flags = SliderFlags::None);
bool slider_int2(const std::string& label, int value[2], int v_min, int v_max, 
                const std::string& format = "%d", SliderFlags flags = SliderFlags::None);
bool slider_int3(const std::string& label, int value[3], int v_min, int v_max, 
                const std::string& format = "%d", SliderFlags flags = SliderFlags::None);
bool slider_int4(const std::string& label, int value[4], int v_min, int v_max, 
                const std::string& format = "%d", SliderFlags flags = SliderFlags::None);

// Vertical sliders
bool v_slider_float(const std::string& label, const Vec2& size, float* value, float v_min, float v_max,
                   const std::string& format = "%.3f", SliderFlags flags = SliderFlags::None);
bool v_slider_int(const std::string& label, const Vec2& size, int* value, int v_min, int v_max,
                 const std::string& format = "%d", SliderFlags flags = SliderFlags::None);

// =============================================================================
// RANGE WIDGETS (DUAL-SIDED SLIDERS)
// =============================================================================

bool slider_float_range(const std::string& label, float* v_current_min, float* v_current_max,
                       float v_min, float v_max, const std::string& format = "%.3f",
                       const std::string& format_max = nullptr, SliderFlags flags = SliderFlags::None);
bool slider_int_range(const std::string& label, int* v_current_min, int* v_current_max,
                     int v_min, int v_max, const std::string& format = "%d",
                     const std::string& format_max = nullptr, SliderFlags flags = SliderFlags::None);

// =============================================================================
// DRAG WIDGETS
// =============================================================================

/**
 * @brief Drag value widgets (no visual slider)
 */
bool drag_float(const std::string& label, float* value, float v_speed = 1.0f, float v_min = 0.0f, 
               float v_max = 0.0f, const std::string& format = "%.3f", SliderFlags flags = SliderFlags::None);
bool drag_float2(const std::string& label, float value[2], float v_speed = 1.0f, float v_min = 0.0f, 
                float v_max = 0.0f, const std::string& format = "%.3f", SliderFlags flags = SliderFlags::None);
bool drag_float3(const std::string& label, float value[3], float v_speed = 1.0f, float v_min = 0.0f, 
                float v_max = 0.0f, const std::string& format = "%.3f", SliderFlags flags = SliderFlags::None);
bool drag_float4(const std::string& label, float value[4], float v_speed = 1.0f, float v_min = 0.0f, 
                float v_max = 0.0f, const std::string& format = "%.3f", SliderFlags flags = SliderFlags::None);

bool drag_int(const std::string& label, int* value, float v_speed = 1.0f, int v_min = 0, int v_max = 0,
             const std::string& format = "%d", SliderFlags flags = SliderFlags::None);
bool drag_int2(const std::string& label, int value[2], float v_speed = 1.0f, int v_min = 0, int v_max = 0,
              const std::string& format = "%d", SliderFlags flags = SliderFlags::None);
bool drag_int3(const std::string& label, int value[3], float v_speed = 1.0f, int v_min = 0, int v_max = 0,
              const std::string& format = "%d", SliderFlags flags = SliderFlags::None);
bool drag_int4(const std::string& label, int value[4], float v_speed = 1.0f, int v_min = 0, int v_max = 0,
              const std::string& format = "%d", SliderFlags flags = SliderFlags::None);

bool drag_float_range(const std::string& label, float* v_current_min, float* v_current_max,
                     float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f,
                     const std::string& format = "%.3f", const std::string& format_max = nullptr,
                     SliderFlags flags = SliderFlags::None);
bool drag_int_range(const std::string& label, int* v_current_min, int* v_current_max,
                   float v_speed = 1.0f, int v_min = 0, int v_max = 0,
                   const std::string& format = "%d", const std::string& format_max = nullptr,
                   SliderFlags flags = SliderFlags::None);

// =============================================================================
// COMBO BOX WIDGETS
// =============================================================================

/**
 * @brief Combo box flags
 */
enum class ComboFlags : uint32_t {
    None = 0,
    PopupAlignLeft = 1 << 0,      // Align the popup toward the left by default
    HeightSmall = 1 << 1,         // Max ~4 items visible
    HeightRegular = 1 << 2,       // Max ~8 items visible (default)
    HeightLarge = 1 << 3,         // Max ~20 items visible
    HeightLargest = 1 << 4,       // As many fitting items as possible
    NoArrowButton = 1 << 5,       // Display on the preview box without the square arrow button
    NoPreview = 1 << 6,           // Display only a square arrow button
    WidthFitPreview = 1 << 7      // Width dynamically calculated from preview contents
};

inline ComboFlags operator|(ComboFlags a, ComboFlags b) {
    return static_cast<ComboFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

/**
 * @brief Combo box widgets
 */
bool combo(const std::string& label, int* current_item, std::span<const std::string> items, 
          int popup_max_height_in_items = -1);
bool combo(const std::string& label, int* current_item, const std::string& items_separated_by_zeros,
          int popup_max_height_in_items = -1);
bool combo(const std::string& label, int* current_item, 
          std::function<std::string(int)> items_getter, int items_count, int popup_max_height_in_items = -1);

// Advanced combo
bool begin_combo(const std::string& label, const std::string& preview_value, ComboFlags flags = ComboFlags::None);
void end_combo();

// =============================================================================
// LIST BOX WIDGETS
// =============================================================================

bool list_box(const std::string& label, int* current_item, std::span<const std::string> items,
             int height_in_items = -1);
bool list_box(const std::string& label, int* current_item, 
             std::function<std::string(int)> items_getter, int items_count, int height_in_items = -1);

// Advanced listbox
bool begin_list_box(const std::string& label, const Vec2& size = Vec2{0, 0});
void end_list_box();

// =============================================================================
// SELECTABLE WIDGETS
// =============================================================================

/**
 * @brief Selectable flags
 */
enum class SelectableFlags : uint32_t {
    None = 0,
    DontClosePopups = 1 << 0,     // Clicking this will not close parent popup windows
    SpanAllColumns = 1 << 1,      // Selectable frame can span all columns (text will still fit in current column)
    AllowDoubleClick = 1 << 2,    // Generate press events on double clicks too
    Disabled = 1 << 3,            // Cannot be selected, display grayed out text
    AllowOverlap = 1 << 4         // Hit testing to allow subsequent widgets to overlap this one
};

inline SelectableFlags operator|(SelectableFlags a, SelectableFlags b) {
    return static_cast<SelectableFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

bool selectable(const std::string& label, bool selected = false, SelectableFlags flags = SelectableFlags::None, 
               const Vec2& size = Vec2{0, 0});
bool selectable(const std::string& label, bool* p_selected, SelectableFlags flags = SelectableFlags::None, 
               const Vec2& size = Vec2{0, 0});

// =============================================================================
// COLOR WIDGETS
// =============================================================================

/**
 * @brief Color edit flags
 */
enum class ColorEditFlags : uint32_t {
    None = 0,
    NoAlpha = 1 << 1,             // Disable Alpha component
    NoPicker = 1 << 2,            // Disable picker when clicking on colored square
    NoOptions = 1 << 3,           // Disable toggling options menu when right-clicking
    NoSmallPreview = 1 << 4,      // Disable colored square preview next to the inputs
    NoInputs = 1 << 5,            // Disable inputs sliders/text widgets
    NoTooltip = 1 << 6,           // Disable tooltip when hovering the preview
    NoLabel = 1 << 7,             // Disable display of inline text label
    NoSidePreview = 1 << 8,       // Disable bigger color preview on right side of the picker
    NoDragDrop = 1 << 9,          // Disable drag and drop target
    NoBorder = 1 << 10,           // Disable border around the colored square
    
    // User Options
    AlphaBar = 1 << 16,           // Show vertical alpha bar/gradient in picker
    AlphaPreview = 1 << 17,       // Display preview as a transparent color over a checkerboard
    AlphaPreviewHalf = 1 << 18,   // Display half opaque / half checkerboard
    HDR = 1 << 19,                // Currently only disable 0.0f..1.0f limits
    DisplayRGB = 1 << 20,         // Override display mode (RGB/HSV/Hex)
    DisplayHSV = 1 << 21,         // Override display mode (RGB/HSV/Hex)  
    DisplayHex = 1 << 22,         // Override display mode (RGB/HSV/Hex)
    Uint8 = 1 << 23,              // Display values formatted as 0..255
    Float = 1 << 24,              // Display values formatted as 0.0f..1.0f floats
    PickerHueBar = 1 << 25,       // Bar for Hue, rectangle for Sat/Value
    PickerHueWheel = 1 << 26,     // Wheel for Hue, triangle for Sat/Value
    InputRGB = 1 << 27,           // Input and output data in RGB format
    InputHSV = 1 << 28            // Input and output data in HSV format
};

inline ColorEditFlags operator|(ColorEditFlags a, ColorEditFlags b) {
    return static_cast<ColorEditFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

bool color_edit3(const std::string& label, float col[3], ColorEditFlags flags = ColorEditFlags::None);
bool color_edit4(const std::string& label, float col[4], ColorEditFlags flags = ColorEditFlags::None);
bool color_picker3(const std::string& label, float col[3], ColorEditFlags flags = ColorEditFlags::None);
bool color_picker4(const std::string& label, float col[4], ColorEditFlags flags = ColorEditFlags::None);
bool color_button(const std::string& desc_id, const Color& col, ColorEditFlags flags = ColorEditFlags::None, 
                 const Vec2& size = Vec2{0, 0});

// =============================================================================
// TREE WIDGETS
// =============================================================================

/**
 * @brief Tree node flags
 */
enum class TreeNodeFlags : uint32_t {
    None = 0,
    Selected = 1 << 0,            // Draw as selected
    Framed = 1 << 1,              // Draw frame with background
    AllowOverlap = 1 << 2,        // Hit testing to allow subsequent widgets to overlap this one
    NoTreePushOnOpen = 1 << 3,    // Don't do a TreePush() when open
    NoAutoOpenOnLog = 1 << 4,     // Don't automatically and temporarily open node when Logging is active
    DefaultOpen = 1 << 5,         // Default node to be open
    OpenOnDoubleClick = 1 << 6,   // Need double-click to open node
    OpenOnArrow = 1 << 7,         // Only open when clicking on the arrow part
    Leaf = 1 << 8,                // No collapsing, no arrow
    Bullet = 1 << 9,              // Display a bullet instead of arrow
    FramePadding = 1 << 10,       // Use FramePadding (even for an unframed text node)
    SpanAvailWidth = 1 << 11,     // Extend hit box to the right-most edge, even if not framed
    SpanFullWidth = 1 << 12,      // Extend hit box to the left-most and right-most edges
    SpanAllColumns = 1 << 13,     // Frame will span all columns of its container table
    NavLeftJumpsBackHere = 1 << 14, // Left direction may move to this TreeNode from any of its child
    CollapsingHeader = Framed | NoTreePushOnOpen | NoAutoOpenOnLog
};

inline TreeNodeFlags operator|(TreeNodeFlags a, TreeNodeFlags b) {
    return static_cast<TreeNodeFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

bool tree_node(const std::string& label);
bool tree_node(const std::string& str_id, const std::string& text);
bool tree_node_ex(const std::string& label, TreeNodeFlags flags = TreeNodeFlags::None);
bool tree_node_ex(const std::string& str_id, TreeNodeFlags flags, const std::string& text);
void tree_push(const std::string& str_id);
void tree_push(const void* ptr_id = nullptr);
void tree_pop();
float get_tree_node_to_label_spacing();
bool collapsing_header(const std::string& label, TreeNodeFlags flags = TreeNodeFlags::None);
bool collapsing_header(const std::string& label, bool* p_visible, TreeNodeFlags flags = TreeNodeFlags::None);
void set_next_item_open(bool is_open, int cond = 0);

// =============================================================================
// PROGRESS BAR AND LOADING WIDGETS
// =============================================================================

void progress_bar(float fraction, const Vec2& size_arg = Vec2{-1, 0}, const std::string& overlay = "");
void spinner(const std::string& label, float radius = 10.0f, int thickness = 6, const Color& color = Color::WHITE);
void loading_indicator_circle(const std::string& label, float radius = 10.0f, const Color& main_color = Color::WHITE, 
                             const Color& backdrop_color = Color{0.5f, 0.5f, 0.5f, 0.5f}, int circle_count = 8, 
                             float speed = 1.0f);

// =============================================================================
// SEPARATORS AND SPACING
// =============================================================================

void separator();
void separator_text(const std::string& label);
void spacing();
void dummy(const Vec2& size);
void indent(float indent_w = 0.0f);
void unindent(float indent_w = 0.0f);
void new_line();
void same_line(float offset_from_start_x = 0.0f, float spacing = -1.0f);

// =============================================================================
// LAYOUT HELPERS
// =============================================================================

void begin_group();
void end_group();
Vec2 get_cursor_pos();
float get_cursor_pos_x();
float get_cursor_pos_y();
void set_cursor_pos(const Vec2& local_pos);
void set_cursor_pos_x(float local_x);
void set_cursor_pos_y(float local_y);
Vec2 get_cursor_start_pos();
Vec2 get_cursor_screen_pos();
void set_cursor_screen_pos(const Vec2& pos);
void align_text_to_frame_padding();
float get_text_line_height();
float get_text_line_height_with_spacing();
float get_frame_height();
float get_frame_height_with_spacing();

// =============================================================================
// CUSTOM WIDGET BUILDING BLOCKS
// =============================================================================

// Item/widget utilities and querying
bool is_item_hovered(int flags = 0);
bool is_item_active();
bool is_item_focused();
bool is_item_clicked(MouseButton mouse_button = MouseButton::Left);
bool is_item_visible();
bool is_item_edited();
bool is_item_activated();
bool is_item_deactivated();
bool is_item_deactivated_after_edit();
bool is_item_toggled_open();
bool is_any_item_hovered();
bool is_any_item_active();
bool is_any_item_focused();
GuiID get_item_id();
Vec2 get_item_rect_min();
Vec2 get_item_rect_max();
Vec2 get_item_rect_size();
void set_item_allow_overlap();

// Widget construction helpers
void push_clip_rect(const Vec2& clip_rect_min, const Vec2& clip_rect_max, bool intersect_with_current_clip_rect);
void pop_clip_rect();
bool invisible_button(const std::string& str_id, const Vec2& size, int flags = 0);
void push_button_repeat(bool repeat);
void pop_button_repeat();
bool button_behavior(const Rect& bb, GuiID id, bool* out_hovered, bool* out_held, int flags = 0);

// =============================================================================
// WIDGET STATE STORAGE
// =============================================================================

/**
 * @brief Persistent widget state storage
 */
struct WidgetStateStorage {
    std::unordered_map<GuiID, int> int_storage;
    std::unordered_map<GuiID, float> float_storage;
    std::unordered_map<GuiID, void*> ptr_storage;
    
    void clear();
    int* get_int_ref(GuiID key, int default_val = 0);
    bool* get_bool_ref(GuiID key, bool default_val = false);
    float* get_float_ref(GuiID key, float default_val = 0.0f);
    void** get_void_ptr_ref(GuiID key, void* default_val = nullptr);
    
    int get_int(GuiID key, int default_val = 0) const;
    void set_int(GuiID key, int val);
    bool get_bool(GuiID key, bool default_val = false) const;
    void set_bool(GuiID key, bool val);
    float get_float(GuiID key, float default_val = 0.0f) const;
    void set_float(GuiID key, float val);
    void* get_void_ptr(GuiID key) const;
    void set_void_ptr(GuiID key, void* val);
    
    void set_all_int(int val);
};

WidgetStateStorage* get_state_storage();

// =============================================================================
// CUSTOM WIDGET CREATION MACROS AND UTILITIES
// =============================================================================

/**
 * @brief Begin/End pattern for custom widgets
 */
#define GUI_WIDGET_BEGIN(widget_name, ...) \
    bool widget_name##_internal(__VA_ARGS__); \
    bool widget_name(__VA_ARGS__) { \
        GuiContext* ctx = get_current_context(); \
        if (!ctx) return false; \
        return widget_name##_internal(__VA_ARGS__); \
    } \
    bool widget_name##_internal(__VA_ARGS__)

/**
 * @brief Helper for creating unique widget IDs
 */
#define GUI_ID_FROM_STR(str) get_current_context()->get_id(str)
#define GUI_ID_FROM_PTR(ptr) get_current_context()->get_id(std::to_string(reinterpret_cast<uintptr_t>(ptr)))

/**
 * @brief Widget utility functions
 */
Vec2 calc_item_size(Vec2 size, float default_w, float default_h);
float calc_wrap_width_for_pos(const Vec2& pos, float wrap_pos_x);
void push_multi_items_widths(int components, float width_full);
void shrink_widths(std::span<float> widths, float width_excess);

// =============================================================================
// ADVANCED WIDGET HELPERS
// =============================================================================

/**
 * @brief Value formatting helpers
 */
std::string format_value(float value, const std::string& format);
std::string format_value(int value, const std::string& format);
std::string format_value(double value, const std::string& format);

/**
 * @brief Data type variant for generic widgets
 */
using WidgetValue = std::variant<int, float, double, std::string, bool>;

/**
 * @brief Generic widget creation
 */
bool generic_slider(const std::string& label, WidgetValue* value, WidgetValue v_min, WidgetValue v_max,
                   const std::string& format = "", SliderFlags flags = SliderFlags::None);
bool generic_drag(const std::string& label, WidgetValue* value, float v_speed = 1.0f,
                 WidgetValue v_min = {}, WidgetValue v_max = {},
                 const std::string& format = "", SliderFlags flags = SliderFlags::None);
bool generic_input(const std::string& label, WidgetValue* value, InputTextFlags flags = InputTextFlags::None);

} // namespace ecscope::gui