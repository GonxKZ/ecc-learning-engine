/**
 * @file gui_core.hpp
 * @brief Core IMGUI Framework - Immediate Mode GUI System
 * 
 * Professional-grade immediate mode GUI framework with complete widget system,
 * flexible layouts, and high-performance rendering integration.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#pragma once

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <stack>
#include <span>
#include <optional>
#include <chrono>
#include <array>

// Rendering integration
#include "../rendering/renderer.hpp"

namespace ecscope::gui {

// =============================================================================
// FORWARD DECLARATIONS
// =============================================================================

class GuiContext;
class GuiRenderer;
class FontAtlas;
class InputSystem;
class LayoutManager;
class ThemeManager;

// =============================================================================
// CORE TYPES & ENUMERATIONS
// =============================================================================

/**
 * @brief Unique identifier for GUI elements (generated from string hashes)
 */
using GuiID = uint32_t;

/**
 * @brief 2D vector for positions, sizes, etc.
 */
struct Vec2 {
    float x = 0.0f, y = 0.0f;
    
    Vec2() = default;
    Vec2(float x, float y) : x(x), y(y) {}
    
    Vec2 operator+(const Vec2& other) const { return {x + other.x, y + other.y}; }
    Vec2 operator-(const Vec2& other) const { return {x - other.x, y - other.y}; }
    Vec2 operator*(float scale) const { return {x * scale, y * scale}; }
    Vec2 operator/(float scale) const { return {x / scale, y / scale}; }
    
    Vec2& operator+=(const Vec2& other) { x += other.x; y += other.y; return *this; }
    Vec2& operator-=(const Vec2& other) { x -= other.x; y -= other.y; return *this; }
    Vec2& operator*=(float scale) { x *= scale; y *= scale; return *this; }
    Vec2& operator/=(float scale) { x /= scale; y /= scale; return *this; }
    
    float length_squared() const { return x * x + y * y; }
    float length() const { return std::sqrt(length_squared()); }
    Vec2 normalized() const { float len = length(); return len > 0 ? *this / len : Vec2{}; }
};

/**
 * @brief 4-component color (RGBA)
 */
struct Color {
    float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
    
    Color() = default;
    Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}
    Color(uint32_t rgba) {
        r = ((rgba >> 24) & 0xFF) / 255.0f;
        g = ((rgba >> 16) & 0xFF) / 255.0f;
        b = ((rgba >> 8) & 0xFF) / 255.0f;
        a = (rgba & 0xFF) / 255.0f;
    }
    
    uint32_t to_rgba() const {
        return (static_cast<uint32_t>(r * 255) << 24) |
               (static_cast<uint32_t>(g * 255) << 16) |
               (static_cast<uint32_t>(b * 255) << 8) |
               static_cast<uint32_t>(a * 255);
    }
    
    Color with_alpha(float alpha) const { return {r, g, b, alpha}; }
    
    // Common colors
    static const Color WHITE;
    static const Color BLACK;
    static const Color RED;
    static const Color GREEN;
    static const Color BLUE;
    static const Color YELLOW;
    static const Color CYAN;
    static const Color MAGENTA;
    static const Color TRANSPARENT;
};

/**
 * @brief Rectangle with position and size
 */
struct Rect {
    Vec2 min, max;
    
    Rect() = default;
    Rect(const Vec2& min, const Vec2& max) : min(min), max(max) {}
    Rect(float x, float y, float w, float h) : min(x, y), max(x + w, y + h) {}
    
    Vec2 size() const { return max - min; }
    Vec2 center() const { return (min + max) * 0.5f; }
    float width() const { return max.x - min.x; }
    float height() const { return max.y - min.y; }
    
    bool contains(const Vec2& point) const {
        return point.x >= min.x && point.x <= max.x && 
               point.y >= min.y && point.y <= max.y;
    }
    
    bool overlaps(const Rect& other) const {
        return min.x <= other.max.x && max.x >= other.min.x &&
               min.y <= other.max.y && max.y >= other.min.y;
    }
    
    Rect clamp(const Rect& bounds) const {
        return {
            {std::max(min.x, bounds.min.x), std::max(min.y, bounds.min.y)},
            {std::min(max.x, bounds.max.x), std::min(max.y, bounds.max.y)}
        };
    }
    
    Rect expand(float padding) const {
        return {min - Vec2{padding, padding}, max + Vec2{padding, padding}};
    }
    
    Rect shrink(float padding) const {
        return expand(-padding);
    }
};

/**
 * @brief Widget state flags
 */
enum class WidgetState : uint32_t {
    None = 0,
    Hovered = 1 << 0,
    Active = 1 << 1,
    Focused = 1 << 2,
    Disabled = 1 << 3,
    Visible = 1 << 4,
    Clicked = 1 << 5,
    DoubleClicked = 1 << 6,
    RightClicked = 1 << 7
};

inline WidgetState operator|(WidgetState a, WidgetState b) {
    return static_cast<WidgetState>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline WidgetState operator&(WidgetState a, WidgetState b) {
    return static_cast<WidgetState>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline bool has_state(WidgetState flags, WidgetState state) {
    return (flags & state) != WidgetState::None;
}

/**
 * @brief Mouse button enumeration
 */
enum class MouseButton : uint8_t {
    Left = 0,
    Right = 1,
    Middle = 2,
    Count = 3
};

/**
 * @brief Keyboard key codes
 */
enum class Key : uint32_t {
    None = 0,
    Tab = 9,
    Enter = 13,
    Escape = 27,
    Space = 32,
    Backspace = 8,
    Delete = 127,
    Left = 256,
    Right = 257,
    Up = 258,
    Down = 259,
    Home = 260,
    End = 261,
    PageUp = 262,
    PageDown = 263,
    A = 65, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    F1 = 290, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    LeftShift = 340,
    RightShift = 344,
    LeftCtrl = 341,
    RightCtrl = 345,
    LeftAlt = 342,
    RightAlt = 346
};

/**
 * @brief Key modifier flags
 */
enum class KeyMod : uint8_t {
    None = 0,
    Ctrl = 1 << 0,
    Shift = 1 << 1,
    Alt = 1 << 2,
    Super = 1 << 3
};

inline KeyMod operator|(KeyMod a, KeyMod b) {
    return static_cast<KeyMod>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline KeyMod operator&(KeyMod a, KeyMod b) {
    return static_cast<KeyMod>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

/**
 * @brief Text input event
 */
struct TextInput {
    std::string text;
    GuiID target_id = 0;
};

/**
 * @brief Drawing command types
 */
enum class DrawCommandType : uint8_t {
    Rectangle,
    Circle,
    Text,
    Line,
    Triangle,
    Texture,
    Gradient
};

/**
 * @brief Single drawing command
 */
struct DrawCommand {
    DrawCommandType type;
    Rect bounds;
    Color color;
    Color secondary_color = Color::TRANSPARENT; // For gradients
    float thickness = 1.0f;
    float rounding = 0.0f;
    std::string text;
    uint32_t texture_id = 0;
    Vec2 uv_min = {0.0f, 0.0f};
    Vec2 uv_max = {1.0f, 1.0f};
    
    // Clipping rectangle
    Rect clip_rect = {{-10000, -10000}, {10000, 10000}};
};

/**
 * @brief Draw list for collecting drawing commands
 */
class DrawList {
public:
    std::vector<DrawCommand> commands;
    
    void clear() { commands.clear(); }
    
    void add_rect(const Rect& rect, const Color& color, float rounding = 0.0f, float thickness = 0.0f);
    void add_rect_filled(const Rect& rect, const Color& color, float rounding = 0.0f);
    void add_circle(const Vec2& center, float radius, const Color& color, int segments = 0, float thickness = 1.0f);
    void add_circle_filled(const Vec2& center, float radius, const Color& color, int segments = 0);
    void add_line(const Vec2& p1, const Vec2& p2, const Color& color, float thickness = 1.0f);
    void add_text(const Vec2& pos, const Color& color, const std::string& text);
    void add_texture(const Rect& rect, uint32_t texture_id, const Vec2& uv_min = {0, 0}, const Vec2& uv_max = {1, 1}, const Color& tint = Color::WHITE);
    void add_gradient(const Rect& rect, const Color& top_left, const Color& top_right, const Color& bottom_left, const Color& bottom_right);
    
    void push_clip_rect(const Rect& rect);
    void pop_clip_rect();
    
private:
    std::stack<Rect> clip_stack_;
    Rect current_clip_ = {{-10000, -10000}, {10000, 10000}};
};

// =============================================================================
// GUI CONTEXT - MAIN STATE MANAGEMENT
// =============================================================================

/**
 * @brief Main GUI context managing all state
 * 
 * This is the heart of the IMGUI system. It maintains all the temporary state
 * needed for immediate mode rendering while providing a clean API.
 */
class GuiContext {
public:
    GuiContext();
    ~GuiContext();
    
    // =============================================================================
    // LIFECYCLE MANAGEMENT
    // =============================================================================
    
    /**
     * @brief Initialize the GUI system
     */
    bool initialize(rendering::IRenderer* renderer, int display_width, int display_height);
    
    /**
     * @brief Shutdown and cleanup
     */
    void shutdown();
    
    /**
     * @brief Begin a new GUI frame
     */
    void new_frame(float delta_time);
    
    /**
     * @brief End the current frame and prepare for rendering
     */
    void end_frame();
    
    /**
     * @brief Render all GUI elements
     */
    void render();
    
    // =============================================================================
    // INPUT HANDLING
    // =============================================================================
    
    void set_display_size(int width, int height);
    void set_mouse_pos(float x, float y);
    void set_mouse_button(MouseButton button, bool pressed);
    void set_mouse_wheel(float wheel_delta);
    void add_input_character(uint32_t character);
    void set_key(Key key, bool pressed);
    void set_key_mods(KeyMod mods);
    
    // =============================================================================
    // ID STACK MANAGEMENT
    // =============================================================================
    
    void push_id(const std::string& id);
    void push_id(int id);
    void push_id(const void* ptr);
    void pop_id();
    GuiID get_id(const std::string& label);
    
    // =============================================================================
    // LAYOUT AND POSITIONING
    // =============================================================================
    
    Vec2 get_cursor_pos() const;
    void set_cursor_pos(const Vec2& pos);
    Vec2 get_content_region_avail() const;
    Vec2 get_window_size() const;
    Vec2 get_window_pos() const;
    
    // =============================================================================
    // DRAWING PRIMITIVES
    // =============================================================================
    
    DrawList& get_draw_list();
    DrawList& get_overlay_draw_list();
    
    // =============================================================================
    // STATE QUERIES
    // =============================================================================
    
    bool is_item_hovered() const;
    bool is_item_active() const;
    bool is_item_clicked(MouseButton button = MouseButton::Left) const;
    bool is_item_double_clicked(MouseButton button = MouseButton::Left) const;
    Vec2 get_mouse_pos() const;
    Vec2 get_mouse_drag_delta(MouseButton button = MouseButton::Left) const;
    bool is_mouse_down(MouseButton button) const;
    bool is_mouse_clicked(MouseButton button) const;
    bool is_mouse_released(MouseButton button) const;
    bool is_mouse_double_clicked(MouseButton button) const;
    
    // =============================================================================
    // INTERNAL STATE ACCESS (for widget implementations)
    // =============================================================================
    
    struct FrameData {
        float delta_time = 0.0f;
        uint64_t frame_count = 0;
        Vec2 display_size = {800, 600};
        Vec2 mouse_pos = {0, 0};
        Vec2 mouse_pos_prev = {0, 0};
        Vec2 mouse_delta = {0, 0};
        float mouse_wheel = 0.0f;
        std::array<bool, 3> mouse_down = {false, false, false};
        std::array<bool, 3> mouse_clicked = {false, false, false};
        std::array<bool, 3> mouse_released = {false, false, false};
        std::array<bool, 3> mouse_double_clicked = {false, false, false};
        std::array<Vec2, 3> mouse_drag_start = {};
        std::array<float, 3> mouse_drag_time = {};
        KeyMod key_mods = KeyMod::None;
        std::unordered_map<Key, bool> keys_down;
        std::string input_characters;
        
        // Timing
        std::chrono::high_resolution_clock::time_point frame_start_time;
        std::chrono::high_resolution_clock::time_point last_frame_time;
    };
    
    FrameData& get_frame_data() { return frame_data_; }
    const FrameData& get_frame_data() const { return frame_data_; }
    
    // Widget state tracking
    struct WidgetData {
        GuiID id = 0;
        WidgetState state = WidgetState::None;
        Rect bounds;
        float last_access_time = 0.0f;
    };
    
    WidgetData* get_widget_data(GuiID id);
    void set_last_item_data(GuiID id, const Rect& bounds, WidgetState state);
    
    // Focus management
    GuiID get_active_id() const { return active_id_; }
    void set_active_id(GuiID id) { active_id_ = id; }
    GuiID get_hovered_id() const { return hovered_id_; }
    void set_hovered_id(GuiID id) { hovered_id_ = id; }
    GuiID get_focused_id() const { return focused_id_; }
    void set_focused_id(GuiID id) { focused_id_ = id; }
    
private:
    // Core systems
    std::unique_ptr<GuiRenderer> renderer_;
    std::unique_ptr<FontAtlas> font_atlas_;
    std::unique_ptr<InputSystem> input_system_;
    std::unique_ptr<LayoutManager> layout_manager_;
    std::unique_ptr<ThemeManager> theme_manager_;
    
    // Frame data
    FrameData frame_data_;
    
    // ID management
    std::vector<GuiID> id_stack_;
    uint32_t current_id_base_ = 0;
    
    // Widget state
    std::unordered_map<GuiID, WidgetData> widget_data_;
    GuiID active_id_ = 0;        // Currently active widget (mouse pressed)
    GuiID hovered_id_ = 0;       // Currently hovered widget
    GuiID focused_id_ = 0;       // Currently focused widget (keyboard input)
    
    // Last item state
    GuiID last_item_id_ = 0;
    Rect last_item_bounds_;
    WidgetState last_item_state_ = WidgetState::None;
    
    // Drawing
    DrawList main_draw_list_;
    DrawList overlay_draw_list_;
    
    // Initialization state
    bool initialized_ = false;
    
    // Helper functions
    GuiID hash_string(const std::string& str, GuiID seed = 0) const;
    void update_mouse_state();
    void update_keyboard_state();
    void cleanup_unused_widget_data();
};

// =============================================================================
// GLOBAL CONTEXT ACCESS
// =============================================================================

/**
 * @brief Get the current GUI context
 */
GuiContext* get_current_context();

/**
 * @brief Set the current GUI context
 */
void set_current_context(GuiContext* context);

/**
 * @brief Create a new GUI context
 */
std::unique_ptr<GuiContext> create_context();

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

/**
 * @brief Calculate text size with current font
 */
Vec2 calc_text_size(const std::string& text);

/**
 * @brief Get the current frame's delta time
 */
float get_delta_time();

/**
 * @brief Get the current frame count
 */
uint64_t get_frame_count();

/**
 * @brief Convert screen coordinates to GUI coordinates
 */
Vec2 screen_to_gui(const Vec2& screen_pos);

/**
 * @brief Convert GUI coordinates to screen coordinates
 */
Vec2 gui_to_screen(const Vec2& gui_pos);

} // namespace ecscope::gui