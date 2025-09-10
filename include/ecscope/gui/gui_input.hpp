/**
 * @file gui_input.hpp
 * @brief Input Handling System for GUI Framework
 * 
 * Comprehensive input management with focus handling, keyboard navigation,
 * mouse interaction, gamepad support, and input event processing.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#pragma once

#include "gui_core.hpp"
#include <array>
#include <queue>
#include <bitset>
#include <chrono>
#include <functional>

namespace ecscope::gui {

// =============================================================================
// INPUT EVENT SYSTEM
// =============================================================================

/**
 * @brief Input event types
 */
enum class InputEventType : uint8_t {
    MouseMove,
    MouseDown,
    MouseUp,
    MouseWheel,
    KeyDown,
    KeyUp,
    TextInput,
    GamepadButton,
    GamepadAxis,
    Focus,
    WindowResize
};

/**
 * @brief Generic input event structure
 */
struct InputEvent {
    InputEventType type;
    std::chrono::high_resolution_clock::time_point timestamp;
    
    union {
        struct {
            float x, y;
            float delta_x, delta_y;
        } mouse_move;
        
        struct {
            MouseButton button;
            float x, y;
            int click_count;
        } mouse_button;
        
        struct {
            float delta_x, delta_y;
            float x, y;
        } mouse_wheel;
        
        struct {
            Key key;
            KeyMod mods;
            bool repeat;
        } keyboard;
        
        struct {
            uint32_t codepoint;
            KeyMod mods;
        } text_input;
        
        struct {
            uint8_t gamepad_id;
            uint8_t button;
            bool pressed;
        } gamepad_button;
        
        struct {
            uint8_t gamepad_id;
            uint8_t axis;
            float value;
        } gamepad_axis;
        
        struct {
            GuiID widget_id;
            bool gained;
        } focus;
        
        struct {
            int width, height;
        } window_resize;
    };
    
    InputEvent() : timestamp(std::chrono::high_resolution_clock::now()) {}
};

// =============================================================================
// GAMEPAD SUPPORT
// =============================================================================

/**
 * @brief Gamepad button enumeration
 */
enum class GamepadButton : uint8_t {
    A, B, X, Y,
    LeftBumper, RightBumper,
    LeftStick, RightStick,
    Start, Back, Guide,
    DPadUp, DPadDown, DPadLeft, DPadRight,
    Count
};

/**
 * @brief Gamepad axis enumeration
 */
enum class GamepadAxis : uint8_t {
    LeftStickX, LeftStickY,
    RightStickX, RightStickY,
    LeftTrigger, RightTrigger,
    Count
};

/**
 * @brief Gamepad state
 */
struct GamepadState {
    bool connected = false;
    std::array<bool, static_cast<size_t>(GamepadButton::Count)> buttons = {};
    std::array<float, static_cast<size_t>(GamepadAxis::Count)> axes = {};
    float dead_zone = 0.1f;
    std::string name;
    
    bool is_button_pressed(GamepadButton button) const {
        return buttons[static_cast<size_t>(button)];
    }
    
    float get_axis(GamepadAxis axis) const {
        float value = axes[static_cast<size_t>(axis)];
        return std::abs(value) > dead_zone ? value : 0.0f;
    }
};

// =============================================================================
// KEYBOARD NAVIGATION
// =============================================================================

/**
 * @brief Navigation direction for keyboard/gamepad navigation
 */
enum class NavDirection : uint8_t {
    None,
    Up, Down, Left, Right,
    PageUp, PageDown,
    Home, End,
    Enter, Cancel
};

/**
 * @brief Navigation request
 */
struct NavRequest {
    NavDirection direction = NavDirection::None;
    GuiID from_id = 0;
    GuiID to_id = 0;
    bool wrap_around = true;
    bool handled = false;
};

/**
 * @brief Keyboard navigation system
 */
class KeyboardNavigator {
public:
    void update();
    void handle_key_event(Key key, KeyMod mods, bool pressed);
    void register_navigable_widget(GuiID id, const Rect& bounds, bool can_focus = true);
    void unregister_widget(GuiID id);
    
    GuiID get_current_focus() const { return current_focus_; }
    void set_focus(GuiID id);
    void clear_focus() { current_focus_ = 0; }
    
    bool is_navigation_active() const { return navigation_active_; }
    void set_navigation_active(bool active) { navigation_active_ = active; }
    
    // Navigation queries
    GuiID find_next_widget(GuiID current, NavDirection direction) const;
    GuiID find_closest_widget(const Vec2& position) const;
    
private:
    struct NavigableWidget {
        GuiID id;
        Rect bounds;
        bool can_focus;
        float last_access_time;
    };
    
    std::unordered_map<GuiID, NavigableWidget> navigable_widgets_;
    GuiID current_focus_ = 0;
    bool navigation_active_ = false;
    
    // Navigation state
    std::chrono::high_resolution_clock::time_point last_nav_time_;
    float nav_repeat_delay_ = 0.5f;
    float nav_repeat_rate_ = 0.1f;
    Key last_nav_key_ = Key::None;
    bool nav_key_repeat_ = false;
    
    NavDirection key_to_nav_direction(Key key) const;
    void cleanup_stale_widgets();
};

// =============================================================================
// INPUT SYSTEM CORE
// =============================================================================

/**
 * @brief Main input system managing all input sources
 */
class InputSystem {
public:
    InputSystem();
    ~InputSystem();
    
    bool initialize();
    void shutdown();
    void update(float delta_time);
    
    // =============================================================================
    // EVENT PROCESSING
    // =============================================================================
    
    void process_events();
    void add_event(const InputEvent& event);
    void clear_events();
    
    // Event callbacks
    using EventCallback = std::function<bool(const InputEvent&)>;
    void set_event_callback(InputEventType type, EventCallback callback);
    void remove_event_callback(InputEventType type);
    
    // =============================================================================
    // MOUSE INPUT
    // =============================================================================
    
    Vec2 get_mouse_position() const { return mouse_state_.position; }
    Vec2 get_mouse_delta() const { return mouse_state_.delta; }
    float get_mouse_wheel_delta() const { return mouse_state_.wheel_delta; }
    
    bool is_mouse_button_down(MouseButton button) const;
    bool is_mouse_button_pressed(MouseButton button) const;
    bool is_mouse_button_released(MouseButton button) const;
    bool is_mouse_button_double_clicked(MouseButton button) const;
    
    Vec2 get_mouse_drag_start(MouseButton button) const;
    Vec2 get_mouse_drag_delta(MouseButton button) const;
    float get_mouse_drag_distance(MouseButton button) const;
    bool is_mouse_dragging(MouseButton button, float threshold = 5.0f) const;
    
    // Mouse capture
    void capture_mouse(GuiID widget_id);
    void release_mouse_capture();
    GuiID get_mouse_capture() const { return mouse_capture_id_; }
    
    // =============================================================================
    // KEYBOARD INPUT
    // =============================================================================
    
    bool is_key_down(Key key) const;
    bool is_key_pressed(Key key) const;
    bool is_key_released(Key key) const;
    bool is_key_repeated(Key key) const;
    
    KeyMod get_key_mods() const { return keyboard_state_.mods; }
    bool has_key_mod(KeyMod mod) const;
    
    // Text input
    std::string get_input_characters() const { return text_input_buffer_; }
    void clear_input_characters() { text_input_buffer_.clear(); }
    void add_input_character(uint32_t codepoint);
    
    // Keyboard shortcuts
    struct Shortcut {
        Key key;
        KeyMod mods;
        std::function<void()> callback;
        bool enabled = true;
    };
    
    void register_shortcut(const std::string& name, Key key, KeyMod mods, std::function<void()> callback);
    void unregister_shortcut(const std::string& name);
    void enable_shortcut(const std::string& name, bool enabled);
    
    // =============================================================================
    // GAMEPAD INPUT
    // =============================================================================
    
    size_t get_gamepad_count() const;
    const GamepadState& get_gamepad_state(size_t index) const;
    bool is_gamepad_connected(size_t index) const;
    
    void update_gamepad_state(size_t index, const GamepadState& state);
    
    // =============================================================================
    // FOCUS MANAGEMENT
    // =============================================================================
    
    KeyboardNavigator& get_navigator() { return navigator_; }
    const KeyboardNavigator& get_navigator() const { return navigator_; }
    
    void set_focus(GuiID widget_id);
    GuiID get_focused_widget() const { return focused_widget_id_; }
    void clear_focus();
    
    // Focus events
    using FocusCallback = std::function<void(GuiID widget_id, bool gained)>;
    void set_focus_callback(FocusCallback callback) { focus_callback_ = callback; }
    
    // =============================================================================
    // INPUT FILTERING AND BLOCKING
    // =============================================================================
    
    void block_input(bool block) { input_blocked_ = block; }
    bool is_input_blocked() const { return input_blocked_; }
    
    void set_modal_widget(GuiID widget_id) { modal_widget_id_ = widget_id; }
    GuiID get_modal_widget() const { return modal_widget_id_; }
    void clear_modal() { modal_widget_id_ = 0; }
    
    // =============================================================================
    // PLATFORM INTEGRATION
    // =============================================================================
    
    void set_platform_window(void* window) { platform_window_ = window; }
    void* get_platform_window() const { return platform_window_; }
    
    // Cursor management
    enum class CursorType {
        Arrow, TextBeam, ResizeHorizontal, ResizeVertical,
        ResizeDiagonalLeft, ResizeDiagonalRight, Hand, NotAllowed
    };
    
    void set_cursor(CursorType cursor);
    CursorType get_cursor() const { return current_cursor_; }
    
    // Clipboard
    void set_clipboard_text(const std::string& text);
    std::string get_clipboard_text() const;
    
    // =============================================================================
    // DEBUGGING AND DIAGNOSTICS
    // =============================================================================
    
    struct InputStats {
        size_t events_processed_this_frame = 0;
        size_t total_events_processed = 0;
        float average_processing_time_ms = 0.0f;
        size_t active_shortcuts = 0;
        size_t navigable_widgets = 0;
        bool navigation_active = false;
        GuiID focused_widget = 0;
        GuiID mouse_capture = 0;
        GuiID modal_widget = 0;
    };
    
    InputStats get_stats() const;
    void print_debug_info() const;
    
private:
    // Mouse state
    struct MouseState {
        Vec2 position = {0, 0};
        Vec2 previous_position = {0, 0};
        Vec2 delta = {0, 0};
        float wheel_delta = 0.0f;
        
        struct ButtonState {
            bool down = false;
            bool pressed = false;
            bool released = false;
            bool double_clicked = false;
            Vec2 press_position = {0, 0};
            std::chrono::high_resolution_clock::time_point press_time;
            std::chrono::high_resolution_clock::time_point last_click_time;
            int click_count = 0;
        };
        
        std::array<ButtonState, 3> buttons;
        
        void update() {
            delta = position - previous_position;
            previous_position = position;
            wheel_delta = 0.0f;
            
            for (auto& button : buttons) {
                button.pressed = false;
                button.released = false;
                button.double_clicked = false;
            }
        }
    };
    
    // Keyboard state
    struct KeyboardState {
        std::bitset<512> keys_down;
        std::bitset<512> keys_pressed;
        std::bitset<512> keys_released;
        std::bitset<512> keys_repeated;
        KeyMod mods = KeyMod::None;
        
        void update() {
            keys_pressed.reset();
            keys_released.reset();
            keys_repeated.reset();
        }
    };
    
    // Core state
    MouseState mouse_state_;
    KeyboardState keyboard_state_;
    std::array<GamepadState, 4> gamepad_states_;
    KeyboardNavigator navigator_;
    
    // Event system
    std::queue<InputEvent> event_queue_;
    std::array<EventCallback, static_cast<size_t>(InputEventType::WindowResize) + 1> event_callbacks_;
    
    // Focus and capture
    GuiID focused_widget_id_ = 0;
    GuiID mouse_capture_id_ = 0;
    GuiID modal_widget_id_ = 0;
    FocusCallback focus_callback_;
    
    // Text input
    std::string text_input_buffer_;
    
    // Shortcuts
    std::unordered_map<std::string, Shortcut> shortcuts_;
    
    // Platform
    void* platform_window_ = nullptr;
    CursorType current_cursor_ = CursorType::Arrow;
    
    // State flags
    bool input_blocked_ = false;
    bool initialized_ = false;
    
    // Timing and statistics
    std::chrono::high_resolution_clock::time_point frame_start_time_;
    InputStats stats_;
    
    // Constants
    static constexpr float DOUBLE_CLICK_TIME = 0.5f; // seconds
    static constexpr float DOUBLE_CLICK_DISTANCE = 5.0f; // pixels
    static constexpr float KEY_REPEAT_DELAY = 0.5f; // seconds
    static constexpr float KEY_REPEAT_RATE = 0.05f; // seconds
    
    // Helper methods
    void update_mouse_buttons();
    void update_keyboard_repeat();
    void update_gamepads();
    void process_shortcuts();
    void handle_mouse_event(const InputEvent& event);
    void handle_keyboard_event(const InputEvent& event);
    void handle_text_input_event(const InputEvent& event);
    void handle_gamepad_event(const InputEvent& event);
    void handle_focus_event(const InputEvent& event);
    
    bool is_event_blocked_by_modal(const InputEvent& event) const;
    void convert_unicode_to_utf8(uint32_t codepoint, std::string& output);
};

// =============================================================================
// INPUT UTILITIES
// =============================================================================

/**
 * @brief RAII helper for input capture
 */
class InputCapture {
public:
    InputCapture(InputSystem& input, GuiID widget_id);
    ~InputCapture();
    
    InputCapture(const InputCapture&) = delete;
    InputCapture& operator=(const InputCapture&) = delete;
    
private:
    InputSystem& input_;
    GuiID widget_id_;
    bool captured_;
};

/**
 * @brief RAII helper for modal input blocking
 */
class ModalScope {
public:
    ModalScope(InputSystem& input, GuiID widget_id);
    ~ModalScope();
    
    ModalScope(const ModalScope&) = delete;
    ModalScope& operator=(const ModalScope&) = delete;
    
private:
    InputSystem& input_;
    GuiID previous_modal_;
};

/**
 * @brief Key combination helper
 */
struct KeyCombination {
    Key key;
    KeyMod mods;
    
    KeyCombination(Key k, KeyMod m = KeyMod::None) : key(k), mods(m) {}
    
    bool matches(Key pressed_key, KeyMod pressed_mods) const {
        return key == pressed_key && mods == pressed_mods;
    }
    
    std::string to_string() const;
    static KeyCombination from_string(const std::string& str);
};

// Helper functions
std::string key_to_string(Key key);
Key string_to_key(const std::string& str);
std::string keymod_to_string(KeyMod mods);
KeyMod string_to_keymod(const std::string& str);

} // namespace ecscope::gui