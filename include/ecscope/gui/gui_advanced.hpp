/**
 * @file gui_advanced.hpp
 * @brief Advanced GUI Features
 * 
 * Advanced GUI functionality including drag-and-drop, tooltips, context menus,
 * modal dialogs, notifications, and other sophisticated UI interactions.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#pragma once

#include "gui_core.hpp"
#include "gui_widgets.hpp"
#include "gui_layout.hpp"
#include <functional>
#include <queue>
#include <unordered_set>

namespace ecscope::gui {

// =============================================================================
// DRAG AND DROP SYSTEM
// =============================================================================

/**
 * @brief Drag and drop payload types
 */
enum class DragDropDataType : uint32_t {
    Custom = 0,
    Text,
    File,
    Image,
    Color,
    Float,
    Int,
    Vec2,
    Vec3,
    Vec4,
    COUNT
};

/**
 * @brief Drag and drop flags
 */
enum class DragDropFlags : uint32_t {
    None = 0,
    
    // BeginDragDropSource() flags
    SourceNoPreviewTooltip = 1 << 0,      // Disable preview tooltip
    SourceNoDisableHover = 1 << 1,        // By default, a successful call to BeginDragDropSource opens a tooltip
    SourceNoHoldToOpenOthers = 1 << 2,    // Disable the behavior that allows to open tree nodes and collapsing header by holding over them while dragging a source item
    SourceAllowNullID = 1 << 3,           // Allow items with no identifier to be used as drag source, by manufacturing a temporary identifier based on their window-relative position
    SourceExtern = 1 << 4,                // External source (from outside of dear imgui), won't attempt to read current item/window info. Will always return true
    SourceAutoExpirePayload = 1 << 5,     // Automatically expire the payload if the source cease to be submitted (otherwise payloads are persisting while being dragged)
    
    // AcceptDragDropPayload() flags
    AcceptBeforeDelivery = 1 << 10,       // AcceptDragDropPayload() will return true even before the mouse button is released
    AcceptNoDrawDefaultRect = 1 << 11,    // Do not draw the default highlight rectangle when hovering over target
    AcceptNoPreviewTooltip = 1 << 12,     // Request hiding the BeginDragDropSource tooltip from the BeginDragDropTarget site
    AcceptPeekOnly = AcceptBeforeDelivery | AcceptNoDrawDefaultRect // For peeking ahead and inspecting the payload before delivery
};

inline DragDropFlags operator|(DragDropFlags a, DragDropFlags b) {
    return static_cast<DragDropFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

/**
 * @brief Drag and drop payload
 */
struct DragDropPayload {
    void* data = nullptr;                    // Data payload
    int data_size = 0;                      // Data size
    DragDropDataType data_type = DragDropDataType::Custom; // Data type
    std::string type_name;                  // Custom type name (for Custom type)
    
    GuiID source_id = 0;                    // Source widget ID
    GuiID source_parent_id = 0;             // Source parent widget ID
    int data_frame_count = 0;               // Data timestamp (helps for some delivery/delivery handling)
    std::string preview;                     // Preview string (for tooltip)
    bool delivery = false;                   // Set when AcceptDragDropPayload() was called and mouse was released over the target item
    
    template<typename T>
    bool is_data_type() const {
        return sizeof(T) == data_size && data != nullptr;
    }
    
    template<typename T>
    T* get_data() const {
        return is_data_type<T>() ? static_cast<T*>(data) : nullptr;
    }
    
    void clear() {
        data = nullptr;
        data_size = 0;
        data_type = DragDropDataType::Custom;
        type_name.clear();
        source_id = 0;
        source_parent_id = 0;
        data_frame_count = 0;
        preview.clear();
        delivery = false;
    }
};

/**
 * @brief Drag and drop functions
 */
bool begin_drag_drop_source(DragDropFlags flags = DragDropFlags::None);
bool set_drag_drop_payload(const std::string& type, const void* data, size_t size, 
                          DragDropFlags flags = DragDropFlags::None);
bool set_drag_drop_payload(DragDropDataType type, const void* data, size_t size,
                          DragDropFlags flags = DragDropFlags::None);
void end_drag_drop_source();

bool begin_drag_drop_target();
const DragDropPayload* accept_drag_drop_payload(const std::string& type, DragDropFlags flags = DragDropFlags::None);
const DragDropPayload* accept_drag_drop_payload(DragDropDataType type, DragDropFlags flags = DragDropFlags::None);
void end_drag_drop_target();

const DragDropPayload* get_drag_drop_payload();
bool is_drag_drop_active();

// Convenience functions for common types
bool drag_drop_text(const std::string& text, const std::string& preview = "");
bool drag_drop_file(const std::string& filepath, const std::string& preview = "");
bool drag_drop_color(const Color& color, const std::string& preview = "");
bool drag_drop_float(float value, const std::string& preview = "");
bool drag_drop_int(int value, const std::string& preview = "");
bool drag_drop_vec2(const Vec2& value, const std::string& preview = "");

std::string* accept_text_drop();
std::string* accept_file_drop();
Color* accept_color_drop();
float* accept_float_drop();
int* accept_int_drop();
Vec2* accept_vec2_drop();

// =============================================================================
// ADVANCED TOOLTIP SYSTEM
// =============================================================================

/**
 * @brief Tooltip flags
 */
enum class TooltipFlags : uint32_t {
    None = 0,
    NoWrap = 1 << 0,           // Disable text wrapping
    AlwaysAutoResize = 1 << 1,  // Always auto-resize to content
    NoDelay = 1 << 2,          // Show immediately, no delay
    NoFade = 1 << 3,           // Disable fade in/out animation
    FollowMouse = 1 << 4,      // Follow mouse cursor
    NoBackground = 1 << 5,     // Transparent background
    NoBorder = 1 << 6,         // No border
    RichText = 1 << 7          // Enable rich text formatting
};

inline TooltipFlags operator|(TooltipFlags a, TooltipFlags b) {
    return static_cast<TooltipFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

/**
 * @brief Advanced tooltip functions
 */
void set_tooltip_ex(const std::string& text, TooltipFlags flags = TooltipFlags::None);
void set_item_tooltip_ex(const std::string& text, TooltipFlags flags = TooltipFlags::None);
bool begin_tooltip_ex(TooltipFlags flags = TooltipFlags::None);
void end_tooltip_ex();

// Rich tooltips with custom content
bool begin_rich_tooltip();
void end_rich_tooltip();
void tooltip_text(const std::string& text);
void tooltip_colored_text(const Color& color, const std::string& text);
void tooltip_separator();
void tooltip_image(uint32_t texture_id, const Vec2& size);
void tooltip_progress_bar(float progress, const std::string& overlay = "");

// Tooltip management
void set_tooltip_delay(float delay_seconds);
void set_tooltip_fade_speed(float fade_speed);
void disable_tooltip_for_next_item();

// =============================================================================
// CONTEXT MENU SYSTEM
// =============================================================================

/**
 * @brief Context menu item types
 */
enum class ContextMenuItemType : uint8_t {
    Regular,
    Separator,
    Submenu,
    Checkbox,
    Radio
};

/**
 * @brief Context menu item
 */
struct ContextMenuItem {
    ContextMenuItemType type = ContextMenuItemType::Regular;
    std::string label;
    std::string shortcut;
    std::function<void()> action;
    bool enabled = true;
    bool checked = false;
    uint32_t icon_texture_id = 0;
    
    // For submenus
    std::vector<ContextMenuItem> submenu_items;
    
    ContextMenuItem() = default;
    ContextMenuItem(const std::string& label, std::function<void()> action = nullptr)
        : label(label), action(action) {}
};

/**
 * @brief Context menu builder
 */
class ContextMenuBuilder {
public:
    ContextMenuBuilder& add_item(const std::string& label, std::function<void()> action = nullptr,
                                const std::string& shortcut = "", bool enabled = true);
    ContextMenuBuilder& add_separator();
    ContextMenuBuilder& add_checkbox(const std::string& label, bool* checked, 
                                    std::function<void(bool)> action = nullptr,
                                    const std::string& shortcut = "", bool enabled = true);
    ContextMenuBuilder& add_submenu(const std::string& label, 
                                   std::function<void(ContextMenuBuilder&)> submenu_builder);
    ContextMenuBuilder& add_icon_item(const std::string& label, uint32_t icon_texture_id,
                                     std::function<void()> action = nullptr,
                                     const std::string& shortcut = "", bool enabled = true);
    
    const std::vector<ContextMenuItem>& get_items() const { return items_; }
    void clear() { items_.clear(); }
    
private:
    std::vector<ContextMenuItem> items_;
};

/**
 * @brief Context menu functions
 */
bool begin_context_menu(const std::string& str_id, const std::vector<ContextMenuItem>& items);
bool begin_context_menu(const std::string& str_id, std::function<void(ContextMenuBuilder&)> menu_builder);
void end_context_menu();

void show_context_menu(const Vec2& position, const std::vector<ContextMenuItem>& items);
void show_context_menu(const Vec2& position, std::function<void(ContextMenuBuilder&)> menu_builder);

// Quick context menu for common scenarios
void show_text_edit_context_menu(); // Cut, Copy, Paste, Select All
void show_file_context_menu(const std::string& filepath);
void show_color_context_menu(Color& color);

// =============================================================================
// MODAL DIALOG SYSTEM
// =============================================================================

/**
 * @brief Modal dialog types
 */
enum class ModalType : uint8_t {
    Info,
    Warning,
    Error,
    Question,
    Custom
};

/**
 * @brief Modal dialog buttons
 */
enum class ModalButtons : uint32_t {
    None = 0,
    OK = 1 << 0,
    Cancel = 1 << 1,
    Yes = 1 << 2,
    No = 1 << 3,
    Apply = 1 << 4,
    Close = 1 << 5,
    Retry = 1 << 6,
    Ignore = 1 << 7,
    
    OKCancel = OK | Cancel,
    YesNo = Yes | No,
    YesNoCancel = Yes | No | Cancel,
    RetryCancel = Retry | Cancel
};

inline ModalButtons operator|(ModalButtons a, ModalButtons b) {
    return static_cast<ModalButtons>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

/**
 * @brief Modal dialog result
 */
enum class ModalResult : uint8_t {
    None,
    OK,
    Cancel,
    Yes,
    No,
    Apply,
    Close,
    Retry,
    Ignore
};

/**
 * @brief Modal dialog configuration
 */
struct ModalConfig {
    std::string title;
    std::string message;
    ModalType type = ModalType::Info;
    ModalButtons buttons = ModalButtons::OK;
    Vec2 size = {400, 200};
    bool resizable = false;
    bool closable = true;
    std::function<void(ModalResult)> callback;
    
    // Custom content callback
    std::function<void()> custom_content;
    
    // Icon
    uint32_t icon_texture_id = 0;
};

/**
 * @brief Modal dialog functions
 */
void open_modal(const std::string& modal_id, const ModalConfig& config);
bool begin_modal_ex(const std::string& modal_id, ModalConfig& config, ModalResult& result);
void end_modal_ex();
void close_modal(const std::string& modal_id);
bool is_modal_open(const std::string& modal_id);

// Convenience functions for common modals
ModalResult show_message_box(const std::string& title, const std::string& message, 
                            ModalType type = ModalType::Info, ModalButtons buttons = ModalButtons::OK);
ModalResult show_confirmation_dialog(const std::string& title, const std::string& message);
ModalResult show_error_dialog(const std::string& title, const std::string& error_message);

// File dialogs
std::string show_open_file_dialog(const std::string& title = "Open File",
                                 const std::vector<std::string>& filters = {},
                                 const std::string& initial_dir = "");
std::string show_save_file_dialog(const std::string& title = "Save File",
                                 const std::vector<std::string>& filters = {},
                                 const std::string& initial_dir = "",
                                 const std::string& default_name = "");
std::string show_folder_browser_dialog(const std::string& title = "Select Folder",
                                      const std::string& initial_dir = "");

// Custom modal dialogs
class ModalDialog {
public:
    ModalDialog(const std::string& id, const std::string& title);
    virtual ~ModalDialog() = default;
    
    void open();
    void close();
    bool is_open() const { return open_; }
    
    virtual void render() = 0;
    virtual void on_close() {}
    
protected:
    std::string id_;
    std::string title_;
    bool open_ = false;
    ModalResult result_ = ModalResult::None;
};

// =============================================================================
// NOTIFICATION SYSTEM
// =============================================================================

/**
 * @brief Notification types
 */
enum class NotificationType : uint8_t {
    Info,
    Success,
    Warning,
    Error
};

/**
 * @brief Notification position
 */
enum class NotificationPosition : uint8_t {
    TopLeft,
    TopCenter,
    TopRight,
    CenterLeft,
    Center,
    CenterRight,
    BottomLeft,
    BottomCenter,
    BottomRight
};

/**
 * @brief Notification configuration
 */
struct NotificationConfig {
    std::string title;
    std::string message;
    NotificationType type = NotificationType::Info;
    float duration = 3.0f; // 0 = persistent
    bool dismissible = true;
    bool show_progress = false;
    uint32_t icon_texture_id = 0;
    std::function<void()> on_click;
    std::function<void()> on_dismiss;
};

/**
 * @brief Notification functions
 */
void show_notification(const NotificationConfig& config);
void show_info_notification(const std::string& title, const std::string& message, float duration = 3.0f);
void show_success_notification(const std::string& title, const std::string& message, float duration = 3.0f);
void show_warning_notification(const std::string& title, const std::string& message, float duration = 5.0f);
void show_error_notification(const std::string& title, const std::string& message, float duration = 0.0f);

void set_notification_position(NotificationPosition position);
void set_max_notifications(int max_count);
void clear_all_notifications();

// Progress notifications
void show_progress_notification(const std::string& title, float progress, const std::string& status = "");
void update_progress_notification(const std::string& title, float progress, const std::string& status = "");
void close_progress_notification(const std::string& title);

// =============================================================================
// ADVANCED INPUT DIALOGS
// =============================================================================

/**
 * @brief Input dialog types
 */
enum class InputDialogType : uint8_t {
    Text,
    Password,
    Integer,
    Float,
    Vec2,
    Vec3,
    Color
};

/**
 * @brief Input dialog result
 */
struct InputDialogResult {
    bool confirmed = false;
    std::string text_value;
    int int_value = 0;
    float float_value = 0.0f;
    Vec2 vec2_value = {0, 0};
    struct { float x, y, z; } vec3_value = {0, 0, 0};
    Color color_value = Color::WHITE;
};

/**
 * @brief Input dialog functions
 */
bool show_text_input_dialog(const std::string& title, const std::string& prompt,
                           std::string& value, const std::string& default_value = "");
bool show_password_input_dialog(const std::string& title, const std::string& prompt,
                               std::string& value);
bool show_number_input_dialog(const std::string& title, const std::string& prompt,
                             float& value, float min_value = 0.0f, float max_value = 100.0f);
bool show_integer_input_dialog(const std::string& title, const std::string& prompt,
                              int& value, int min_value = 0, int max_value = 100);
bool show_color_picker_dialog(const std::string& title, Color& color);

// Generic input dialog
bool show_input_dialog(const std::string& title, const std::string& prompt,
                      InputDialogType type, InputDialogResult& result);

// =============================================================================
// WIZARD SYSTEM
// =============================================================================

/**
 * @brief Wizard page interface
 */
class WizardPage {
public:
    virtual ~WizardPage() = default;
    
    virtual std::string get_title() const = 0;
    virtual std::string get_description() const { return ""; }
    virtual void render() = 0;
    virtual bool can_proceed() const { return true; }
    virtual bool can_go_back() const { return true; }
    virtual void on_enter() {}
    virtual void on_leave() {}
    virtual void on_finish() {}
};

/**
 * @brief Wizard dialog
 */
class WizardDialog {
public:
    WizardDialog(const std::string& title, const Vec2& size = {600, 400});
    
    void add_page(std::unique_ptr<WizardPage> page);
    void open();
    void close();
    bool is_open() const { return open_; }
    
    void render();
    
    // Navigation
    void next_page();
    void previous_page();
    void go_to_page(int page_index);
    int get_current_page() const { return current_page_; }
    int get_page_count() const { return static_cast<int>(pages_.size()); }
    
    // Callbacks
    std::function<void()> on_finish;
    std::function<void()> on_cancel;
    
private:
    std::string title_;
    Vec2 size_;
    bool open_ = false;
    int current_page_ = 0;
    std::vector<std::unique_ptr<WizardPage>> pages_;
    
    void render_navigation_buttons();
};

// =============================================================================
// PROGRESS DIALOG
// =============================================================================

/**
 * @brief Progress dialog for long operations
 */
class ProgressDialog {
public:
    ProgressDialog(const std::string& title, const std::string& initial_status = "");
    
    void open();
    void close();
    bool is_open() const { return open_; }
    
    void set_progress(float progress); // 0.0 to 1.0
    void set_status(const std::string& status);
    void set_cancellable(bool cancellable);
    bool was_cancelled() const { return cancelled_; }
    
    void render();
    
    // Indeterminate progress
    void set_indeterminate(bool indeterminate);
    
private:
    std::string title_;
    std::string status_;
    float progress_ = 0.0f;
    bool open_ = false;
    bool cancellable_ = false;
    bool cancelled_ = false;
    bool indeterminate_ = false;
};

// =============================================================================
// ADVANCED WIDGET HELPERS
// =============================================================================

/**
 * @brief Help marker (? icon with tooltip)
 */
void help_marker(const std::string& description);

/**
 * @brief Status indicator
 */
void status_indicator(const std::string& status, const Color& color);

/**
 * @brief Loading spinner
 */
void loading_spinner(const std::string& label, float radius = 8.0f, float thickness = 2.0f);

/**
 * @brief Animated progress bar
 */
void animated_progress_bar(float progress, const Vec2& size, const std::string& overlay = "");

/**
 * @brief Collapsible group
 */
bool collapsible_group(const std::string& label, bool default_open = false);

/**
 * @brief Property grid
 */
class PropertyGrid {
public:
    PropertyGrid(const std::string& label);
    
    void begin();
    void end();
    
    bool property(const std::string& name, std::string& value);
    bool property(const std::string& name, int& value, int min_val = 0, int max_val = 100);
    bool property(const std::string& name, float& value, float min_val = 0.0f, float max_val = 1.0f);
    bool property(const std::string& name, bool& value);
    bool property(const std::string& name, Color& value);
    bool property(const std::string& name, Vec2& value);
    
    void separator();
    void group(const std::string& name);
    
private:
    std::string label_;
    bool open_ = false;
};

// =============================================================================
// KEYBOARD SHORTCUTS
// =============================================================================

/**
 * @brief Shortcut manager
 */
class ShortcutManager {
public:
    static ShortcutManager& instance();
    
    void register_shortcut(const std::string& id, Key key, KeyMod mods, 
                          std::function<void()> callback, const std::string& description = "");
    void unregister_shortcut(const std::string& id);
    void clear_shortcuts();
    
    bool process_shortcuts();
    
    // Global shortcuts (work even when GUI doesn't have focus)
    void register_global_shortcut(const std::string& id, Key key, KeyMod mods,
                                 std::function<void()> callback, const std::string& description = "");
    
    // Context-sensitive shortcuts
    void push_shortcut_context(const std::string& context);
    void pop_shortcut_context();
    void register_context_shortcut(const std::string& context, const std::string& id,
                                  Key key, KeyMod mods, std::function<void()> callback,
                                  const std::string& description = "");
    
    // Shortcut help
    void show_shortcut_help_window(bool* p_open);
    std::vector<std::string> get_shortcuts_for_context(const std::string& context) const;
    
private:
    struct Shortcut {
        std::string id;
        Key key;
        KeyMod mods;
        std::function<void()> callback;
        std::string description;
        std::string context;
        bool global = false;
    };
    
    std::vector<Shortcut> shortcuts_;
    std::stack<std::string> context_stack_;
    
    bool matches_shortcut(const Shortcut& shortcut, Key key, KeyMod mods) const;
};

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

/**
 * @brief Center window on screen
 */
void center_next_window();

/**
 * @brief Fade in/out animation helper
 */
float get_fade_alpha(bool visible, float fade_speed = 5.0f);

/**
 * @brief Pulsing animation helper
 */
float get_pulse_alpha(float speed = 2.0f, float min_alpha = 0.3f, float max_alpha = 1.0f);

/**
 * @brief Get smooth step interpolation
 */
float smooth_step(float t);
float smoother_step(float t);

/**
 * @brief Screen space utilities
 */
Vec2 get_mouse_pos_on_opening_current_popup();
Vec2 get_item_screen_pos();
Vec2 get_item_screen_size();
bool is_rect_visible(const Rect& rect);

} // namespace ecscope::gui