#pragma once

#include "ecscope/web/web_types.hpp"
#include <array>
#include <bitset>
#include <vector>

namespace ecscope::web {

/**
 * @brief Web input system for handling keyboard, mouse, touch, and gamepad input
 * 
 * This class provides comprehensive input handling for web browsers,
 * including support for multiple input devices and gesture recognition.
 */
class WebInput {
public:
    /**
     * @brief Mouse button enumeration
     */
    enum class MouseButton {
        Left = 0,
        Middle = 1,
        Right = 2,
        Back = 3,
        Forward = 4
    };
    
    /**
     * @brief Key codes for common keys
     */
    enum class KeyCode {
        // Letters
        A = 65, B = 66, C = 67, D = 68, E = 69, F = 70, G = 71, H = 72, I = 73, J = 74,
        K = 75, L = 76, M = 77, N = 78, O = 79, P = 80, Q = 81, R = 82, S = 83, T = 84,
        U = 85, V = 86, W = 87, X = 88, Y = 89, Z = 90,
        
        // Numbers
        Num0 = 48, Num1 = 49, Num2 = 50, Num3 = 51, Num4 = 52,
        Num5 = 53, Num6 = 54, Num7 = 55, Num8 = 56, Num9 = 57,
        
        // Function keys
        F1 = 112, F2 = 113, F3 = 114, F4 = 115, F5 = 116, F6 = 117,
        F7 = 118, F8 = 119, F9 = 120, F10 = 121, F11 = 122, F12 = 123,
        
        // Arrow keys
        Left = 37, Up = 38, Right = 39, Down = 40,
        
        // Special keys
        Space = 32, Enter = 13, Escape = 27, Tab = 9, Backspace = 8, Delete = 46,
        Shift = 16, Ctrl = 17, Alt = 18, Meta = 91,
        
        // Numpad
        Numpad0 = 96, Numpad1 = 97, Numpad2 = 98, Numpad3 = 99, Numpad4 = 100,
        Numpad5 = 101, Numpad6 = 102, Numpad7 = 103, Numpad8 = 104, Numpad9 = 105,
        NumpadMultiply = 106, NumpadAdd = 107, NumpadSubtract = 109,
        NumpadDecimal = 110, NumpadDivide = 111
    };
    
    /**
     * @brief Gamepad state
     */
    struct GamepadState {
        bool connected = false;
        std::string id;
        std::array<float, 16> buttons{};  // Button values (0.0 to 1.0)
        std::array<float, 4> axes{};      // Axis values (-1.0 to 1.0)
        std::uint32_t timestamp = 0;
    };
    
    /**
     * @brief Touch point information
     */
    struct TouchPoint {
        std::uint32_t identifier;
        float x, y;
        float radius_x, radius_y;
        float rotation_angle;
        float force;
        bool active;
    };
    
    /**
     * @brief Gesture information
     */
    struct Gesture {
        enum Type {
            None,
            Tap,
            DoubleTap,
            LongPress,
            Swipe,
            Pinch,
            Rotate,
            Pan
        };
        
        Type type = None;
        float x, y;           // Center position
        float delta_x, delta_y; // Movement delta
        float scale = 1.0f;   // Pinch scale
        float rotation = 0.0f; // Rotation angle
        float velocity_x = 0.0f, velocity_y = 0.0f; // Gesture velocity
        std::uint32_t duration = 0; // Gesture duration in ms
    };
    
    /**
     * @brief Construct a new WebInput system
     * @param canvas_id Target canvas element ID
     */
    explicit WebInput(const std::string& canvas_id);
    
    /**
     * @brief Destructor
     */
    ~WebInput();
    
    // Non-copyable, movable
    WebInput(const WebInput&) = delete;
    WebInput& operator=(const WebInput&) = delete;
    WebInput(WebInput&&) noexcept = default;
    WebInput& operator=(WebInput&&) noexcept = default;
    
    /**
     * @brief Initialize the input system
     * @return true if initialization succeeded
     */
    bool initialize();
    
    /**
     * @brief Shutdown the input system
     */
    void shutdown();
    
    /**
     * @brief Update input system (call each frame)
     * @param delta_time Time since last update
     */
    void update(double delta_time);
    
    /**
     * @brief Set input event callback
     * @param callback Event callback function
     */
    void set_input_callback(InputCallback callback);
    
    // Keyboard input
    
    /**
     * @brief Check if key is currently pressed
     * @param key_code Key code to check
     * @return true if key is pressed
     */
    bool is_key_down(KeyCode key_code) const;
    
    /**
     * @brief Check if key was just pressed this frame
     * @param key_code Key code to check
     * @return true if key was just pressed
     */
    bool is_key_pressed(KeyCode key_code) const;
    
    /**
     * @brief Check if key was just released this frame
     * @param key_code Key code to check
     * @return true if key was just released
     */
    bool is_key_released(KeyCode key_code) const;
    
    /**
     * @brief Get typed characters this frame
     * @return String of typed characters
     */
    const std::string& get_typed_text() const;
    
    // Mouse input
    
    /**
     * @brief Check if mouse button is currently pressed
     * @param button Mouse button to check
     * @return true if button is pressed
     */
    bool is_mouse_button_down(MouseButton button) const;
    
    /**
     * @brief Check if mouse button was just pressed this frame
     * @param button Mouse button to check
     * @return true if button was just pressed
     */
    bool is_mouse_button_pressed(MouseButton button) const;
    
    /**
     * @brief Check if mouse button was just released this frame
     * @param button Mouse button to check
     * @return true if button was just released
     */
    bool is_mouse_button_released(MouseButton button) const;
    
    /**
     * @brief Get current mouse position
     * @return Mouse position (x, y)
     */
    std::pair<float, float> get_mouse_position() const;
    
    /**
     * @brief Get mouse movement delta
     * @return Mouse delta (dx, dy)
     */
    std::pair<float, float> get_mouse_delta() const;
    
    /**
     * @brief Get mouse wheel delta
     * @return Wheel delta (dx, dy)
     */
    std::pair<float, float> get_mouse_wheel_delta() const;
    
    /**
     * @brief Set mouse cursor visibility
     * @param visible Whether cursor should be visible
     */
    void set_cursor_visible(bool visible);
    
    /**
     * @brief Lock mouse pointer
     * @return true if pointer lock succeeded
     */
    bool lock_pointer();
    
    /**
     * @brief Unlock mouse pointer
     */
    void unlock_pointer();
    
    /**
     * @brief Check if pointer is locked
     * @return true if pointer is locked
     */
    bool is_pointer_locked() const;
    
    // Touch input
    
    /**
     * @brief Get current touch points
     * @return Vector of active touch points
     */
    const std::vector<TouchPoint>& get_touch_points() const;
    
    /**
     * @brief Get touch point by ID
     * @param identifier Touch point identifier
     * @return Touch point (nullptr if not found)
     */
    const TouchPoint* get_touch_point(std::uint32_t identifier) const;
    
    /**
     * @brief Get current gesture
     * @return Current gesture information
     */
    const Gesture& get_current_gesture() const;
    
    /**
     * @brief Enable/disable gesture recognition
     * @param enable Whether to enable gesture recognition
     */
    void set_gesture_recognition(bool enable);
    
    // Gamepad input
    
    /**
     * @brief Get gamepad state
     * @param index Gamepad index (0-3)
     * @return Gamepad state
     */
    const GamepadState& get_gamepad_state(std::uint32_t index) const;
    
    /**
     * @brief Check if gamepad button is pressed
     * @param gamepad_index Gamepad index
     * @param button_index Button index
     * @return true if button is pressed
     */
    bool is_gamepad_button_down(std::uint32_t gamepad_index, std::uint32_t button_index) const;
    
    /**
     * @brief Get gamepad axis value
     * @param gamepad_index Gamepad index
     * @param axis_index Axis index
     * @return Axis value (-1.0 to 1.0)
     */
    float get_gamepad_axis(std::uint32_t gamepad_index, std::uint32_t axis_index) const;
    
    /**
     * @brief Set gamepad vibration
     * @param gamepad_index Gamepad index
     * @param low_frequency Low frequency vibration (0.0 to 1.0)
     * @param high_frequency High frequency vibration (0.0 to 1.0)
     * @param duration Duration in milliseconds
     */
    void set_gamepad_vibration(std::uint32_t gamepad_index, float low_frequency, 
                              float high_frequency, std::uint32_t duration);
    
    // State management
    
    /**
     * @brief Check if input system is initialized
     * @return true if initialized
     */
    bool is_initialized() const noexcept { return initialized_; }
    
    /**
     * @brief Clear all input state
     */
    void clear_state();
    
    /**
     * @brief Set input focus
     * @param focused Whether input has focus
     */
    void set_focus(bool focused);
    
    /**
     * @brief Check if input has focus
     * @return true if input has focus
     */
    bool has_focus() const noexcept { return has_focus_; }
    
private:
    // Configuration
    std::string canvas_id_;
    
    // State
    bool initialized_ = false;
    bool has_focus_ = true;
    bool gesture_recognition_enabled_ = true;
    bool pointer_locked_ = false;
    
    // Keyboard state
    std::bitset<256> keys_current_;
    std::bitset<256> keys_previous_;
    std::string typed_text_;
    
    // Mouse state
    std::bitset<8> mouse_buttons_current_;
    std::bitset<8> mouse_buttons_previous_;
    float mouse_x_ = 0.0f, mouse_y_ = 0.0f;
    float mouse_delta_x_ = 0.0f, mouse_delta_y_ = 0.0f;
    float wheel_delta_x_ = 0.0f, wheel_delta_y_ = 0.0f;
    
    // Touch state
    std::vector<TouchPoint> touch_points_;
    std::unordered_map<std::uint32_t, TouchPoint> active_touches_;
    
    // Gesture recognition
    Gesture current_gesture_;
    std::chrono::steady_clock::time_point gesture_start_time_;
    float initial_touch_distance_ = 0.0f;
    float initial_touch_angle_ = 0.0f;
    
    // Gamepad state
    std::array<GamepadState, 4> gamepads_;
    
    // Callbacks
    InputCallback input_callback_;
    
    // Internal methods
    void register_event_listeners();
    void unregister_event_listeners();
    void process_gesture_recognition();
    void update_gamepad_state();
    float calculate_touch_distance(const TouchPoint& p1, const TouchPoint& p2) const;
    float calculate_touch_angle(const TouchPoint& p1, const TouchPoint& p2) const;
    
    // Static event handlers
    static EM_BOOL keyboard_callback(int event_type, const EmscriptenKeyboardEvent* event, void* user_data);
    static EM_BOOL mouse_callback(int event_type, const EmscriptenMouseEvent* event, void* user_data);
    static EM_BOOL wheel_callback(int event_type, const EmscriptenWheelEvent* event, void* user_data);
    static EM_BOOL touch_callback(int event_type, const EmscriptenTouchEvent* event, void* user_data);
    static EM_BOOL pointer_lock_callback(int event_type, const EmscriptenPointerlockChangeEvent* event, void* user_data);
    static EM_BOOL gamepad_callback(int event_type, const EmscriptenGamepadEvent* event, void* user_data);
    static EM_BOOL focus_callback(int event_type, const EmscriptenFocusEvent* event, void* user_data);
    
    // Helper methods
    void handle_keyboard_event(int event_type, const EmscriptenKeyboardEvent* event);
    void handle_mouse_event(int event_type, const EmscriptenMouseEvent* event);
    void handle_wheel_event(const EmscriptenWheelEvent* event);
    void handle_touch_event(int event_type, const EmscriptenTouchEvent* event);
    void handle_gamepad_event(int event_type, const EmscriptenGamepadEvent* event);
    void handle_focus_event(int event_type, const EmscriptenFocusEvent* event);
};

} // namespace ecscope::web