/**
 * @file accessibility_motor.hpp
 * @brief Motor Disability Accommodations and Assistive Input Support
 * 
 * Comprehensive motor disability accommodation system providing support for
 * users with various motor impairments through alternative input methods,
 * timing adjustments, gesture accommodations, and assistive technologies.
 * 
 * Features:
 * - Sticky Keys and modifier key assistance
 * - Slow Keys and key repeat filtering
 * - Mouse Keys (numeric keypad mouse control)
 * - Click lock and dwell clicking
 * - Switch access and scanning interfaces
 * - Voice control integration preparation
 * - Eye tracking support preparation
 * - Gesture customization and simplification
 * - Timing and motion accommodations
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#pragma once

#include "accessibility_core.hpp"
#include "gui_input.hpp"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <bitset>

namespace ecscope::gui::accessibility {

// =============================================================================
// MOTOR ACCESSIBILITY ENUMERATIONS
// =============================================================================

/**
 * @brief Input accommodation types
 */
enum class InputAccommodationType : uint8_t {
    None,
    StickyKeys,        // Modifier keys stay pressed
    SlowKeys,          // Ignore quick keypresses
    BounceKeys,        // Ignore repeated keypresses
    FilterKeys,        // Combination of slow/bounce keys
    MouseKeys,         // Numeric keypad mouse control
    ClickLock,         // Lock mouse clicks
    DwellClick,        // Click by dwelling/hovering
    HoverClick,        // Click by hovering with timer
    SwitchAccess,      // Switch-based input
    ScanningInterface, // Row/column scanning
    VoiceControl,      // Voice command input
    EyeTracking,       // Eye gaze input
    HeadTracking,      // Head movement input
    JoystickEmulation, // Joystick/gamepad input
    TouchAdaptation    // Touch interface adaptations
};

/**
 * @brief Switch input types
 */
enum class SwitchType : uint8_t {
    Single,           // One switch (scanning)
    Dual,             // Two switches (select/advance)
    Joystick,         // Joystick switches (4-8 directions)
    Sip_Puff,         // Sip and puff switches
    Eye_Blink,        // Eye blink detection
    Muscle_Twitch,    // EMG muscle sensors
    Custom            // Custom switch configuration
};

/**
 * @brief Dwell click modes
 */
enum class DwellClickMode : uint8_t {
    SingleClick,      // Single left click
    DoubleClick,      // Double left click
    RightClick,       // Right click
    ContextMenu,      // Context menu
    DragStart,        // Start drag operation
    DragEnd,          // End drag operation
    Hover,            // Just hover (no click)
    Custom            // User-defined action
};

/**
 * @brief Scanning patterns
 */
enum class ScanningPattern : uint8_t {
    LinearRow,        // Row by row scanning
    LinearColumn,     // Column by column scanning
    RowColumn,        // Row first, then column
    Group,            // Group-based scanning
    Circular,         // Circular scanning
    Binary,           // Binary tree scanning
    Adaptive,         // Adaptive based on usage
    Custom            // Custom pattern
};

// =============================================================================
// MOTOR ACCOMMODATION STRUCTURES
// =============================================================================

/**
 * @brief Sticky Keys configuration
 */
struct StickyKeysConfig {
    bool enabled = false;
    bool lock_modifier_keys = true;       // Keep modifiers pressed
    bool beep_on_modifier_press = false;  // Audio feedback
    bool visual_feedback = true;          // Visual modifier indicators
    bool turn_off_if_two_pressed = true;  // Disable when two keys pressed
    float modifier_timeout = 5.0f;        // Auto-release timeout (0 = never)
    
    // Which modifiers to make sticky
    bool sticky_shift = true;
    bool sticky_ctrl = true;
    bool sticky_alt = true;
    bool sticky_super = true;
};

/**
 * @brief Slow Keys configuration
 */
struct SlowKeysConfig {
    bool enabled = false;
    float acceptance_delay = 0.5f;        // How long to hold key
    bool beep_on_press = false;           // Audio feedback on press
    bool beep_on_accept = true;           // Audio feedback on acceptance
    bool visual_feedback = true;          // Visual progress indicator
    bool repeat_allowed = false;          // Allow key repeat after acceptance
    float repeat_delay = 1.0f;            // Delay before repeat starts
};

/**
 * @brief Bounce Keys configuration  
 */
struct BounceKeysConfig {
    bool enabled = false;
    float ignore_time = 0.1f;             // Time to ignore repeats
    bool beep_on_reject = false;          // Audio feedback on rejection
    bool visual_feedback = true;          // Visual indication of rejection
    bool use_previous_typing_rhythm = false; // Adapt to user's rhythm
};

/**
 * @brief Mouse Keys configuration
 */
struct MouseKeysConfig {
    bool enabled = false;
    float max_speed = 200.0f;             // Maximum pixels per second
    float acceleration_time = 1.0f;       // Time to reach max speed
    float acceleration_curve = 2.0f;      // Acceleration curve (1.0 = linear)
    bool enable_click_lock = false;       // Enable click locking
    bool enable_drag_lock = false;        // Enable drag locking
    
    // Key mappings (numeric keypad)
    Key move_up = Key::Num8;
    Key move_down = Key::Num2;
    Key move_left = Key::Num4;
    Key move_right = Key::Num6;
    Key move_up_left = Key::Num7;
    Key move_up_right = Key::Num9;
    Key move_down_left = Key::Num1;
    Key move_down_right = Key::Num3;
    Key left_click = Key::Num5;
    Key right_click = Key::Minus;
    Key double_click = Key::Plus;
};

/**
 * @brief Dwell Click configuration
 */
struct DwellClickConfig {
    bool enabled = false;
    float dwell_time = 1.0f;              // Time to hover for click
    float movement_tolerance = 5.0f;       // Pixels of allowed movement
    DwellClickMode click_mode = DwellClickMode::SingleClick;
    bool visual_progress = true;          // Show dwell progress
    bool audio_feedback = false;          // Audio countdown/feedback
    bool require_pause_before_dwell = true; // Must stop moving first
    float pause_time = 0.2f;              // Required pause time
    
    // Visual feedback
    Color progress_color = Color(0, 120, 215, 128);
    float progress_radius = 15.0f;
    bool show_crosshair = true;
};

/**
 * @brief Switch Access configuration
 */
struct SwitchAccessConfig {
    bool enabled = false;
    SwitchType switch_type = SwitchType::Single;
    ScanningPattern scanning_pattern = ScanningPattern::RowColumn;
    float scan_speed = 1.0f;              // Scans per second
    float switch_hold_time = 0.1f;        // Minimum hold time
    bool auto_scan = true;                // Automatic scanning vs manual
    bool wrap_around = true;              // Wrap to beginning after end
    
    // Visual feedback
    Color highlight_color = Color(255, 255, 0, 128); // Scanning highlight
    float highlight_thickness = 3.0f;
    bool show_scan_line = true;
    
    // Audio feedback
    bool beep_on_scan = false;
    bool beep_on_select = true;
    float beep_volume = 0.5f;
    
    // Custom switch mappings
    Key primary_switch = Key::Space;      // Primary selection switch
    Key secondary_switch = Key::Enter;    // Secondary switch (if dual)
};

/**
 * @brief Voice Control configuration
 */
struct VoiceControlConfig {
    bool enabled = false;
    std::string language = "en-US";
    float confidence_threshold = 0.7f;
    bool continuous_listening = false;
    std::string wake_word = "computer";
    bool use_system_voice_recognition = true;
    
    // Command mappings
    std::unordered_map<std::string, std::string> voice_commands;
    std::unordered_map<std::string, GuiID> voice_shortcuts;
};

/**
 * @brief Touch Adaptation configuration
 */
struct TouchAdaptationConfig {
    bool enabled = false;
    float minimum_touch_size = 44.0f;     // Minimum touch target size (CSS pixels)
    float touch_timeout = 0.5f;           // Touch hold timeout
    bool prevent_accidental_activation = true;
    float edge_margin = 10.0f;            // Margin from screen edges
    bool simplify_gestures = true;        // Reduce complex gestures
    bool disable_multi_touch = false;     // Disable multi-touch gestures
};

// =============================================================================
// MOTOR ACCESSIBILITY MANAGER
// =============================================================================

/**
 * @brief Motor disability accommodation manager
 */
class MotorAccessibilityManager {
public:
    MotorAccessibilityManager();
    ~MotorAccessibilityManager();

    // =============================================================================
    // INITIALIZATION & LIFECYCLE
    // =============================================================================

    bool initialize(AccessibilityContext* accessibility_context, InputSystem* input_system);
    void shutdown();
    void update(float delta_time);

    // =============================================================================
    // ACCOMMODATION MANAGEMENT
    // =============================================================================

    void enable_accommodation(InputAccommodationType type, bool enable = true);
    bool is_accommodation_enabled(InputAccommodationType type) const;
    std::vector<InputAccommodationType> get_active_accommodations() const;
    
    void set_accommodation_profile(const std::string& profile_name);
    std::string get_current_accommodation_profile() const;
    void save_accommodation_profile(const std::string& profile_name);
    void load_accommodation_profile(const std::string& profile_name);

    // =============================================================================
    // STICKY KEYS
    // =============================================================================

    void configure_sticky_keys(const StickyKeysConfig& config);
    const StickyKeysConfig& get_sticky_keys_config() const;
    
    void set_sticky_key_state(Key key, bool sticky);
    bool is_key_sticky(Key key) const;
    void clear_all_sticky_keys();
    
    void render_sticky_keys_indicator(DrawList* draw_list);

    // =============================================================================
    // SLOW KEYS
    // =============================================================================

    void configure_slow_keys(const SlowKeysConfig& config);
    const SlowKeysConfig& get_slow_keys_config() const;
    
    bool is_key_being_held(Key key) const;
    float get_key_hold_progress(Key key) const;
    
    void render_slow_keys_progress(DrawList* draw_list);

    // =============================================================================
    // BOUNCE KEYS
    // =============================================================================

    void configure_bounce_keys(const BounceKeysConfig& config);
    const BounceKeysConfig& get_bounce_keys_config() const;
    
    bool was_key_bounce_filtered(Key key) const;

    // =============================================================================
    // MOUSE KEYS
    // =============================================================================

    void configure_mouse_keys(const MouseKeysConfig& config);
    const MouseKeysConfig& get_mouse_keys_config() const;
    
    void enable_mouse_keys(bool enable);
    bool are_mouse_keys_enabled() const;
    
    Vec2 get_mouse_keys_velocity() const;
    void render_mouse_keys_indicator(DrawList* draw_list);

    // =============================================================================
    // DWELL CLICKING
    // =============================================================================

    void configure_dwell_click(const DwellClickConfig& config);
    const DwellClickConfig& get_dwell_click_config() const;
    
    void enable_dwell_click(bool enable);
    bool is_dwell_click_enabled() const;
    
    float get_dwell_progress() const;
    Vec2 get_dwell_position() const;
    
    void render_dwell_progress(DrawList* draw_list);

    // =============================================================================
    // SWITCH ACCESS
    // =============================================================================

    void configure_switch_access(const SwitchAccessConfig& config);
    const SwitchAccessConfig& get_switch_access_config() const;
    
    void enable_switch_access(bool enable);
    bool is_switch_access_enabled() const;
    
    GuiID get_current_scan_target() const;
    void advance_scan();
    void select_current_target();
    
    void render_scan_highlight(DrawList* draw_list);
    void render_switch_access_overlay(DrawList* draw_list);

    // =============================================================================
    // VOICE CONTROL
    // =============================================================================

    void configure_voice_control(const VoiceControlConfig& config);
    const VoiceControlConfig& get_voice_control_config() const;
    
    void enable_voice_control(bool enable);
    bool is_voice_control_enabled() const;
    
    void register_voice_command(const std::string& command, std::function<void()> action);
    void unregister_voice_command(const std::string& command);
    
    void start_listening();
    void stop_listening();
    bool is_listening() const;

    // =============================================================================
    // TOUCH ADAPTATIONS
    // =============================================================================

    void configure_touch_adaptation(const TouchAdaptationConfig& config);
    const TouchAdaptationConfig& get_touch_adaptation_config() const;
    
    void enable_touch_adaptation(bool enable);
    bool is_touch_adaptation_enabled() const;
    
    float calculate_accessible_touch_size(float original_size) const;
    bool is_touch_target_accessible(const Rect& bounds) const;

    // =============================================================================
    // INPUT PROCESSING
    // =============================================================================

    bool process_input_event(const InputEvent& event);
    bool should_filter_key_event(Key key, bool pressed) const;
    bool should_filter_mouse_event(const InputEvent& event) const;
    
    void inject_synthetic_input(const InputEvent& event);

    // =============================================================================
    // TIMING AND GESTURE ACCOMMODATIONS
    // =============================================================================

    struct TimingAccommodations {
        float double_click_time = 0.5f;
        float drag_threshold = 5.0f;
        float hover_time = 1.0f;
        float key_repeat_delay = 0.5f;
        float key_repeat_rate = 0.1f;
        bool disable_timeouts = false;
        float timeout_multiplier = 2.0f;
    };

    void set_timing_accommodations(const TimingAccommodations& accommodations);
    const TimingAccommodations& get_timing_accommodations() const;
    
    void simplify_gestures(bool simplify);
    bool are_gestures_simplified() const;
    
    void set_minimum_target_size(float size);
    float get_minimum_target_size() const;

    // =============================================================================
    // ASSISTIVE TECHNOLOGY INTEGRATION
    // =============================================================================

    void register_assistive_device(const std::string& device_name, void* device_handle);
    void unregister_assistive_device(const std::string& device_name);
    
    bool is_assistive_device_connected(const std::string& device_name) const;
    std::vector<std::string> get_connected_assistive_devices() const;
    
    void send_to_assistive_device(const std::string& device_name, const std::string& data);

    // =============================================================================
    // CUSTOMIZATION AND PROFILES
    // =============================================================================

    struct MotorProfile {
        std::string name;
        std::string description;
        StickyKeysConfig sticky_keys;
        SlowKeysConfig slow_keys;
        BounceKeysConfig bounce_keys;
        MouseKeysConfig mouse_keys;
        DwellClickConfig dwell_click;
        SwitchAccessConfig switch_access;
        VoiceControlConfig voice_control;
        TouchAdaptationConfig touch_adaptation;
        TimingAccommodations timing;
        std::unordered_map<std::string, std::string> custom_settings;
    };

    void create_motor_profile(const MotorProfile& profile);
    void apply_motor_profile(const std::string& profile_name);
    void remove_motor_profile(const std::string& profile_name);
    
    std::vector<std::string> get_available_profiles() const;
    const MotorProfile* get_motor_profile(const std::string& profile_name) const;
    
    void save_profiles_to_file(const std::string& filename) const;
    void load_profiles_from_file(const std::string& filename);

    // =============================================================================
    // DEBUGGING & DIAGNOSTICS
    // =============================================================================

    struct MotorAccessibilityStats {
        bool sticky_keys_active = false;
        bool slow_keys_active = false;
        bool bounce_keys_active = false;
        bool mouse_keys_active = false;
        bool dwell_click_active = false;
        bool switch_access_active = false;
        bool voice_control_active = false;
        
        size_t keys_filtered_this_session = 0;
        size_t mouse_events_filtered_this_session = 0;
        size_t synthetic_events_injected = 0;
        size_t dwell_clicks_performed = 0;
        size_t voice_commands_recognized = 0;
        
        std::string current_profile;
        size_t connected_assistive_devices = 0;
    };

    MotorAccessibilityStats get_stats() const;
    void render_debug_overlay(DrawList* draw_list);
    void render_accommodation_status(DrawList* draw_list);

    // =============================================================================
    // EVENT CALLBACKS
    // =============================================================================

    using AccommodationChangeCallback = std::function<void(InputAccommodationType type, bool enabled)>;
    using VoiceCommandCallback = std::function<void(const std::string& command, float confidence)>;
    using SwitchActivationCallback = std::function<void(SwitchType type, GuiID target)>;

    void set_accommodation_change_callback(AccommodationChangeCallback callback);
    void set_voice_command_callback(VoiceCommandCallback callback);
    void set_switch_activation_callback(SwitchActivationCallback callback);

private:
    // Core components
    AccessibilityContext* accessibility_context_ = nullptr;
    InputSystem* input_system_ = nullptr;
    
    // Configuration
    StickyKeysConfig sticky_keys_config_;
    SlowKeysConfig slow_keys_config_;
    BounceKeysConfig bounce_keys_config_;
    MouseKeysConfig mouse_keys_config_;
    DwellClickConfig dwell_click_config_;
    SwitchAccessConfig switch_access_config_;
    VoiceControlConfig voice_control_config_;
    TouchAdaptationConfig touch_adaptation_config_;
    TimingAccommodations timing_accommodations_;
    
    // Active accommodations
    std::bitset<16> active_accommodations_;
    std::string current_profile_name_;
    
    // State tracking
    struct KeyState {
        bool sticky = false;
        std::chrono::steady_clock::time_point press_time;
        std::chrono::steady_clock::time_point last_bounce_time;
        bool being_held = false;
        bool accepted = false;
    };
    std::unordered_map<Key, KeyState> key_states_;
    
    // Mouse keys state
    Vec2 mouse_keys_velocity_;
    bool mouse_keys_active_ = false;
    
    // Dwell click state
    bool dwell_active_ = false;
    Vec2 dwell_position_;
    std::chrono::steady_clock::time_point dwell_start_time_;
    std::chrono::steady_clock::time_point last_movement_time_;
    
    // Switch access state
    bool switch_access_active_ = false;
    GuiID current_scan_target_ = 0;
    std::vector<GuiID> scan_order_;
    size_t scan_index_ = 0;
    std::chrono::steady_clock::time_point last_scan_advance_;
    
    // Voice control state
    bool voice_control_active_ = false;
    bool listening_ = false;
    std::unordered_map<std::string, std::function<void()>> voice_commands_;
    
    // Assistive devices
    std::unordered_map<std::string, void*> assistive_devices_;
    
    // Profiles
    std::unordered_map<std::string, MotorProfile> motor_profiles_;
    
    // Statistics
    mutable MotorAccessibilityStats stats_;
    size_t keys_filtered_this_session_ = 0;
    size_t mouse_events_filtered_this_session_ = 0;
    size_t synthetic_events_injected_ = 0;
    
    // Callbacks
    AccommodationChangeCallback accommodation_change_callback_;
    VoiceCommandCallback voice_command_callback_;
    SwitchActivationCallback switch_activation_callback_;
    
    // Helper methods
    void update_sticky_keys(float delta_time);
    void update_slow_keys(float delta_time);
    void update_bounce_keys(float delta_time);
    void update_mouse_keys(float delta_time);
    void update_dwell_click(float delta_time);
    void update_switch_access(float delta_time);
    void update_voice_control(float delta_time);
    
    bool process_sticky_key_event(Key key, bool pressed);
    bool process_slow_key_event(Key key, bool pressed);
    bool process_bounce_key_event(Key key, bool pressed);
    bool process_mouse_keys_event(Key key, bool pressed);
    
    void build_scan_order();
    void advance_scan_target();
    void highlight_scan_target(GuiID target_id, DrawList* draw_list);
    
    void play_accommodation_sound(const std::string& sound_name);
    void create_default_profiles();
    
    bool initialized_ = false;
};

// =============================================================================
// MOTOR ACCESSIBILITY UTILITIES
// =============================================================================

/**
 * @brief Utilities for motor accessibility
 */
namespace motor_utils {
    /**
     * @brief Create standard motor accommodation profiles
     */
    MotorAccessibilityManager::MotorProfile create_mild_motor_impairment_profile();
    MotorAccessibilityManager::MotorProfile create_severe_motor_impairment_profile();
    MotorAccessibilityManager::MotorProfile create_switch_access_profile();
    MotorAccessibilityManager::MotorProfile create_voice_control_profile();
    
    /**
     * @brief Calculate accessible target sizes
     */
    float calculate_minimum_touch_target_size(float base_size, InputAccommodationType accommodation);
    Rect expand_for_accessibility(const Rect& original, float expansion_factor);
    
    /**
     * @brief Timing calculations
     */
    float calculate_accessible_timing(float base_timing, const MotorAccessibilityManager::TimingAccommodations& accommodations);
    bool is_gesture_accessible(const std::string& gesture_type, InputAccommodationType accommodation);
    
    /**
     * @brief Voice command utilities
     */
    std::vector<std::string> get_standard_voice_commands();
    bool is_voice_command_valid(const std::string& command);
    std::string normalize_voice_command(const std::string& raw_command);
}

// =============================================================================
// GLOBAL MOTOR ACCESSIBILITY MANAGER
// =============================================================================

/**
 * @brief Get the global motor accessibility manager
 */
MotorAccessibilityManager* get_motor_accessibility_manager();

/**
 * @brief Initialize global motor accessibility support
 */
bool initialize_motor_accessibility();

/**
 * @brief Shutdown global motor accessibility support
 */
void shutdown_motor_accessibility();

} // namespace ecscope::gui::accessibility