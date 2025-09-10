/**
 * @file widgets.hpp
 * @brief Widget System - Complete set of GUI widgets
 * 
 * Professional-grade widget implementations with consistent API,
 * customizable styling, and optimal performance.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#pragma once

#include "core.hpp"
#include <functional>
#include <variant>

namespace ecscope::gui {

// Forward declarations
class Context;

// =============================================================================
// WIDGET FLAGS & OPTIONS
// =============================================================================

/**
 * @brief Widget flags for customizing behavior
 */
enum class WidgetFlags : uint32_t {
    None            = 0,
    NoBackground    = 1 << 0,   // Don't draw background
    NoBorder        = 1 << 1,   // Don't draw border
    NoHover         = 1 << 2,   // Disable hover effects
    NoActive        = 1 << 3,   // Disable active/pressed effects
    Disabled        = 1 << 4,   // Widget is disabled
    ReadOnly        = 1 << 5,   // Widget is read-only
    AlignLeft       = 1 << 6,   // Align content to left
    AlignCenter     = 1 << 7,   // Align content to center
    AlignRight      = 1 << 8,   // Align content to right
    ExpandWidth     = 1 << 9,   // Expand to available width
    ExpandHeight    = 1 << 10,  // Expand to available height
    WordWrap        = 1 << 11,  // Enable word wrapping
    Multiline       = 1 << 12,  // Enable multiline input
    Password        = 1 << 13,  // Password input (show dots)
    Resizable       = 1 << 14,  // Widget can be resized
    Draggable       = 1 << 15,  // Widget can be dragged
    Closable        = 1 << 16,  // Widget can be closed
    Collapsible     = 1 << 17,  // Widget can be collapsed
    Scrollable      = 1 << 18,  // Widget has scrollbars
    Border3D        = 1 << 19,  // Use 3D border style
    Flat            = 1 << 20   // Use flat style
};

inline WidgetFlags operator|(WidgetFlags a, WidgetFlags b) {
    return static_cast<WidgetFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline WidgetFlags operator&(WidgetFlags a, WidgetFlags b) {
    return static_cast<WidgetFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline bool has_flag(WidgetFlags flags, WidgetFlags flag) {
    return (flags & flag) == flag;
}

/**
 * @brief Slider behavior options
 */
enum class SliderBehavior : uint8_t {
    Linear,         // Linear value mapping
    Logarithmic,    // Logarithmic scale
    Power,          // Power curve (with custom exponent)
    Stepped         // Discrete steps
};

// =============================================================================
// BASIC WIDGETS
// =============================================================================

/**
 * @brief Text label widget
 */
bool text(const std::string& label, WidgetFlags flags = WidgetFlags::None);

/**
 * @brief Colored text label
 */
bool colored_text(const std::string& label, const Vec4& color, WidgetFlags flags = WidgetFlags::None);

/**
 * @brief Button widget
 * @return true if clicked
 */
bool button(const std::string& label, const Vec2& size = {0, 0}, WidgetFlags flags = WidgetFlags::None);

/**
 * @brief Image button widget
 * @return true if clicked
 */
bool image_button(rendering::TextureHandle texture, const Vec2& size, 
                  const Vec2& uv0 = {0, 0}, const Vec2& uv1 = {1, 1},
                  const Vec4& tint_color = {1, 1, 1, 1}, WidgetFlags flags = WidgetFlags::None);

/**
 * @brief Checkbox widget
 * @return true if value changed
 */
bool checkbox(const std::string& label, bool* value, WidgetFlags flags = WidgetFlags::None);

/**
 * @brief Radio button widget
 * @return true if selected
 */
bool radio_button(const std::string& label, bool active, WidgetFlags flags = WidgetFlags::None);

/**
 * @brief Radio button group
 * @return true if selection changed
 */
bool radio_button(const std::string& label, int* value, int option_value, WidgetFlags flags = WidgetFlags::None);

// =============================================================================
// INPUT WIDGETS
// =============================================================================

/**
 * @brief Text input widget
 * @return true if text was modified
 */
bool input_text(const std::string& label, std::string& text, size_t max_length = 256, WidgetFlags flags = WidgetFlags::None);

/**
 * @brief Multiline text input widget
 * @return true if text was modified
 */
bool input_text_multiline(const std::string& label, std::string& text, const Vec2& size = {0, 0}, WidgetFlags flags = WidgetFlags::None);

/**
 * @brief Integer input widget
 * @return true if value was modified
 */
bool input_int(const std::string& label, int* value, int step = 1, int step_fast = 100, WidgetFlags flags = WidgetFlags::None);

/**
 * @brief Float input widget
 * @return true if value was modified
 */
bool input_float(const std::string& label, float* value, float step = 0.0f, float step_fast = 0.0f, 
                const char* format = "%.3f", WidgetFlags flags = WidgetFlags::None);

/**
 * @brief Vec2 input widget
 * @return true if value was modified
 */
bool input_float2(const std::string& label, float value[2], const char* format = "%.3f", WidgetFlags flags = WidgetFlags::None);

/**
 * @brief Vec3 input widget
 * @return true if value was modified
 */
bool input_float3(const std::string& label, float value[3], const char* format = "%.3f", WidgetFlags flags = WidgetFlags::None);

/**
 * @brief Vec4 input widget
 * @return true if value was modified
 */
bool input_float4(const std::string& label, float value[4], const char* format = "%.3f", WidgetFlags flags = WidgetFlags::None);

// =============================================================================
// SLIDERS
// =============================================================================

/**
 * @brief Integer slider widget
 * @return true if value was modified
 */
bool slider_int(const std::string& label, int* value, int min_value, int max_value, 
                const char* format = "%d", WidgetFlags flags = WidgetFlags::None);

/**
 * @brief Float slider widget
 * @return true if value was modified
 */
bool slider_float(const std::string& label, float* value, float min_value, float max_value,
                  const char* format = "%.3f", float power = 1.0f, WidgetFlags flags = WidgetFlags::None);

/**
 * @brief Vec2 slider widget
 * @return true if value was modified
 */
bool slider_float2(const std::string& label, float value[2], float min_value, float max_value,
                   const char* format = "%.3f", float power = 1.0f, WidgetFlags flags = WidgetFlags::None);

/**
 * @brief Vec3 slider widget
 * @return true if value was modified
 */
bool slider_float3(const std::string& label, float value[3], float min_value, float max_value,
                   const char* format = "%.3f", float power = 1.0f, WidgetFlags flags = WidgetFlags::None);

/**
 * @brief Vec4 slider widget
 * @return true if value was modified
 */
bool slider_float4(const std::string& label, float value[4], float min_value, float max_value,
                   const char* format = "%.3f", float power = 1.0f, WidgetFlags flags = WidgetFlags::None);

/**
 * @brief Vertical slider widget
 * @return true if value was modified
 */
bool v_slider_float(const std::string& label, const Vec2& size, float* value, float min_value, float max_value,
                    const char* format = "%.3f", float power = 1.0f, WidgetFlags flags = WidgetFlags::None);

/**
 * @brief Slider with custom behavior
 * @return true if value was modified
 */
bool slider_behavior_float(const std::string& label, float* value, float min_value, float max_value,
                          SliderBehavior behavior, float power = 1.0f, int steps = 0,
                          const char* format = "%.3f", WidgetFlags flags = WidgetFlags::None);

// =============================================================================
// COLOR WIDGETS
// =============================================================================

/**
 * @brief Color edit widget (RGB)
 * @return true if color was modified
 */
bool color_edit3(const std::string& label, float color[3], WidgetFlags flags = WidgetFlags::None);

/**
 * @brief Color edit widget (RGBA)
 * @return true if color was modified
 */
bool color_edit4(const std::string& label, float color[4], WidgetFlags flags = WidgetFlags::None);

/**
 * @brief Color picker widget (RGB)
 * @return true if color was modified
 */
bool color_picker3(const std::string& label, float color[3], WidgetFlags flags = WidgetFlags::None);

/**
 * @brief Color picker widget (RGBA)
 * @return true if color was modified
 */
bool color_picker4(const std::string& label, float color[4], WidgetFlags flags = WidgetFlags::None);

/**
 * @brief Color button (shows color swatch)
 * @return true if clicked
 */
bool color_button(const std::string& desc_id, const Vec4& color, const Vec2& size = {0, 0}, WidgetFlags flags = WidgetFlags::None);

// =============================================================================
// SELECTION WIDGETS
// =============================================================================

/**
 * @brief Combo box widget
 * @return true if selection changed
 */
bool combo(const std::string& label, int* current_item, const std::vector<std::string>& items, int popup_max_height_in_items = -1, WidgetFlags flags = WidgetFlags::None);

/**
 * @brief Combo box widget with callback for items
 * @return true if selection changed
 */
bool combo(const std::string& label, int* current_item, std::function<const char*(int)> items_getter, int items_count, int popup_max_height_in_items = -1, WidgetFlags flags = WidgetFlags::None);

/**
 * @brief Listbox widget
 * @return true if selection changed
 */
bool listbox(const std::string& label, int* current_item, const std::vector<std::string>& items, int height_in_items = -1, WidgetFlags flags = WidgetFlags::None);

/**
 * @brief Multi-selection listbox widget
 * @return true if selection changed
 */
bool listbox_multi(const std::string& label, std::vector<bool>& selected, const std::vector<std::string>& items, int height_in_items = -1, WidgetFlags flags = WidgetFlags::None);

// =============================================================================
// TREE & HIERARCHY WIDGETS
// =============================================================================

/**
 * @brief Tree node widget (collapsible header)
 * @return true if opened
 */
bool tree_node(const std::string& label, WidgetFlags flags = WidgetFlags::None);

/**
 * @brief Tree node with ID
 * @return true if opened
 */
bool tree_node(const std::string& str_id, const std::string& label, WidgetFlags flags = WidgetFlags::None);

/**
 * @brief Tree node with pointer ID
 * @return true if opened
 */
bool tree_node(const void* ptr_id, const std::string& label, WidgetFlags flags = WidgetFlags::None);

/**
 * @brief End tree node (call if tree_node returned true)
 */
void tree_pop();

/**
 * @brief Push tree node without rendering (for custom tree nodes)
 */
void tree_push(const std::string& str_id);

/**
 * @brief Push tree node with pointer ID
 */
void tree_push(const void* ptr_id);

/**
 * @brief Selectable item widget
 * @return true if clicked
 */
bool selectable(const std::string& label, bool selected = false, const Vec2& size = {0, 0}, WidgetFlags flags = WidgetFlags::None);

/**
 * @brief Selectable item with state
 * @return true if clicked
 */
bool selectable(const std::string& label, bool* selected, const Vec2& size = {0, 0}, WidgetFlags flags = WidgetFlags::None);

// =============================================================================
// PROGRESS & STATUS WIDGETS
// =============================================================================

/**
 * @brief Progress bar widget
 */
void progress_bar(float fraction, const Vec2& size = {-1, 0}, const std::string& overlay = "", WidgetFlags flags = WidgetFlags::None);

/**
 * @brief Bullet point widget
 */
void bullet();

/**
 * @brief Bullet text widget
 */
void bullet_text(const std::string& text);

// =============================================================================
// ADVANCED WIDGETS
// =============================================================================

/**
 * @brief Plot lines widget
 */
void plot_lines(const std::string& label, const std::vector<float>& values, int values_offset = 0,
                const std::string& overlay_text = "", float scale_min = FLT_MAX, float scale_max = FLT_MAX,
                const Vec2& graph_size = {0, 0});

/**
 * @brief Plot histogram widget
 */
void plot_histogram(const std::string& label, const std::vector<float>& values, int values_offset = 0,
                    const std::string& overlay_text = "", float scale_min = FLT_MAX, float scale_max = FLT_MAX,
                    const Vec2& graph_size = {0, 0});

/**
 * @brief Image widget
 */
void image(rendering::TextureHandle texture, const Vec2& size, 
           const Vec2& uv0 = {0, 0}, const Vec2& uv1 = {1, 1},
           const Vec4& tint_color = {1, 1, 1, 1}, const Vec4& border_color = {0, 0, 0, 0});

// =============================================================================
// DRAG & DROP WIDGETS
// =============================================================================

/**
 * @brief Drag float widget
 * @return true if value was modified
 */
bool drag_float(const std::string& label, float* value, float speed = 1.0f, float min_value = 0.0f, float max_value = 0.0f,
                const char* format = "%.3f", float power = 1.0f, WidgetFlags flags = WidgetFlags::None);

/**
 * @brief Drag int widget
 * @return true if value was modified
 */
bool drag_int(const std::string& label, int* value, float speed = 1.0f, int min_value = 0, int max_value = 0,
              const char* format = "%d", WidgetFlags flags = WidgetFlags::None);

/**
 * @brief Drag float range widget
 * @return true if value was modified
 */
bool drag_float_range2(const std::string& label, float* current_min, float* current_max,
                       float speed = 1.0f, float min_value = 0.0f, float max_value = 0.0f,
                       const char* format = "%.3f", const char* format_max = nullptr, float power = 1.0f,
                       WidgetFlags flags = WidgetFlags::None);

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

/**
 * @brief Add spacing between widgets
 */
void spacing();

/**
 * @brief Add a separator line
 */
void separator();

/**
 * @brief Start a new line
 */
void new_line();

/**
 * @brief Add horizontal spacing
 */
void same_line(float offset_from_start_x = 0.0f, float spacing = -1.0f);

/**
 * @brief Add a dummy widget (invisible spacer)
 */
void dummy(const Vec2& size);

/**
 * @brief Indent following widgets
 */
void indent(float indent_w = 0.0f);

/**
 * @brief Unindent following widgets
 */
void unindent(float indent_w = 0.0f);

/**
 * @brief Group widgets together
 */
void begin_group();
void end_group();

/**
 * @brief Push/pop widget width
 */
void push_item_width(float item_width);
void pop_item_width();
void set_next_item_width(float item_width);

/**
 * @brief Calculate item width
 */
float calc_item_width();

/**
 * @brief Get available content width
 */
float get_content_region_avail_width();

/**
 * @brief Get cursor position
 */
Vec2 get_cursor_pos();
void set_cursor_pos(const Vec2& pos);

/**
 * @brief Get/set cursor screen position
 */
Vec2 get_cursor_screen_pos();
void set_cursor_screen_pos(const Vec2& pos);

// =============================================================================
// WIDGET HELPER FUNCTIONS
// =============================================================================

/**
 * @brief Calculate text size for widget sizing
 */
Vec2 calc_text_size(const std::string& text, bool hide_text_after_double_hash = false, float wrap_width = -1.0f);

/**
 * @brief Check if item is hovered
 */
bool is_item_hovered(WidgetFlags flags = WidgetFlags::None);

/**
 * @brief Check if item is active
 */
bool is_item_active();

/**
 * @brief Check if item is focused
 */
bool is_item_focused();

/**
 * @brief Check if item is clicked
 */
bool is_item_clicked(MouseButton button = MouseButton::Left);

/**
 * @brief Check if item is visible
 */
bool is_item_visible();

/**
 * @brief Check if item was edited
 */
bool is_item_edited();

/**
 * @brief Check if item was activated
 */
bool is_item_activated();

/**
 * @brief Check if item was deactivated
 */
bool is_item_deactivated();

/**
 * @brief Get last item rectangle
 */
Rect get_item_rect();

/**
 * @brief Get last item size
 */
Vec2 get_item_rect_size();

/**
 * @brief Set tooltip for last item
 */
void set_item_tooltip(const std::string& text);

} // namespace ecscope::gui