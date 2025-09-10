/**
 * @file context.hpp
 * @brief GUI Context - Central state and management
 * 
 * The main GUI context that manages the overall state, handles input,
 * coordinates rendering, and provides the main API entry points.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#pragma once

#include "core.hpp"
#include "theme.hpp"
#include "layout.hpp"
#include "text.hpp"
#include "input.hpp"
#include <chrono>
#include <queue>

namespace ecscope::gui {

// =============================================================================
// FRAME DATA & STATE
// =============================================================================

/**
 * @brief Per-frame statistics and metrics
 */
struct FrameData {
    float delta_time = 0.0f;
    uint32_t frame_count = 0;
    
    // Rendering stats
    uint32_t draw_calls = 0;
    uint32_t vertices_rendered = 0;
    uint32_t widgets_rendered = 0;
    
    // Performance metrics
    float cpu_time_ms = 0.0f;
    float layout_time_ms = 0.0f;
    float render_time_ms = 0.0f;
    
    // Memory usage
    size_t vertex_buffer_size = 0;
    size_t index_buffer_size = 0;
    size_t storage_size = 0;
};

/**
 * @brief Current frame state
 */
struct FrameState {
    bool within_frame = false;
    bool layout_dirty = false;
    
    // Current window/container being processed
    Window* current_window = nullptr;
    Layout* current_layout = nullptr;
    
    // Focus and interaction
    ID hovered_id = 0;
    ID active_id = 0;
    ID focused_id = 0;
    ID hot_id = 0;  // Widget that wants to become active
    
    // Mouse interaction
    Vec2 mouse_pos;
    Vec2 mouse_delta;
    std::array<bool, 5> mouse_down = {false, false, false, false, false};
    std::array<bool, 5> mouse_clicked = {false, false, false, false, false};
    std::array<bool, 5> mouse_released = {false, false, false, false, false};
    Vec2 scroll_delta;
    
    // Keyboard interaction
    std::array<bool, 512> keys_down = {};
    std::array<bool, 512> keys_pressed = {};
    std::array<bool, 512> keys_released = {};
    std::string text_input;
    
    // Drag and drop
    bool is_dragging = false;
    ID drag_source_id = 0;
    Vec2 drag_start_pos;
    Vec2 drag_current_pos;
    std::string drag_payload_type;
    const void* drag_payload_data = nullptr;
    size_t drag_payload_size = 0;
    
    // Time tracking
    std::chrono::high_resolution_clock::time_point frame_start_time;
    std::chrono::high_resolution_clock::time_point last_frame_time;
};

/**
 * @brief Tooltip system
 */
struct TooltipSystem {
    bool enabled = true;
    float delay_seconds = 0.5f;
    float fade_in_time = 0.2f;
    
    ID current_id = 0;
    std::string current_text;
    std::chrono::high_resolution_clock::time_point hover_start_time;
    bool is_visible = false;
    float alpha = 0.0f;
};

/**
 * @brief Modal system for dialogs
 */
struct ModalSystem {
    struct Modal {
        ID id;
        std::string title;
        Vec2 size;
        bool closable = true;
        std::function<void()> content_callback;
    };
    
    std::queue<Modal> modal_queue;
    std::optional<Modal> current_modal;
    bool modal_fade_background = true;
    float background_alpha = 0.5f;
};

// =============================================================================
// MAIN GUI CONTEXT
// =============================================================================

/**
 * @brief Main GUI context - the heart of the immediate mode GUI system
 */
class Context {
public:
    Context();
    ~Context();
    
    // Non-copyable, non-movable
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;
    Context(Context&&) = delete;
    Context& operator=(Context&&) = delete;
    
    // =============================================================================
    // INITIALIZATION & LIFECYCLE
    // =============================================================================
    
    /**
     * @brief Initialize the GUI context
     */
    bool initialize(rendering::IRenderer* renderer, int display_width, int display_height);
    
    /**
     * @brief Shutdown and cleanup
     */
    void shutdown();
    
    /**
     * @brief Check if context is initialized
     */
    bool is_initialized() const { return initialized_; }
    
    // =============================================================================
    // FRAME MANAGEMENT
    // =============================================================================
    
    /**
     * @brief Begin a new GUI frame
     */
    void begin_frame();
    
    /**
     * @brief End the current frame and render
     */
    void end_frame();
    
    /**
     * @brief Handle input event
     */
    void handle_event(const Event& event);
    
    /**
     * @brief Set display size (call on window resize)
     */
    void set_display_size(int width, int height);
    
    // =============================================================================
    // RENDERING INTEGRATION
    // =============================================================================
    
    /**
     * @brief Get the renderer instance
     */
    rendering::IRenderer* get_renderer() const { return renderer_; }
    
    /**
     * @brief Render all draw data
     */
    void render();
    
    // =============================================================================
    // THEME & STYLING
    // =============================================================================
    
    /**
     * @brief Get the current theme
     */
    Theme& get_theme() { return *theme_; }
    const Theme& get_theme() const { return *theme_; }
    
    /**
     * @brief Set a new theme
     */
    void set_theme(std::unique_ptr<Theme> theme);
    
    // =============================================================================
    // TEXT & FONTS
    // =============================================================================
    
    /**
     * @brief Get the font atlas
     */
    FontAtlas& get_font_atlas() { return *font_atlas_; }
    const FontAtlas& get_font_atlas() const { return *font_atlas_; }
    
    /**
     * @brief Calculate text size
     */
    Vec2 calculate_text_size(const std::string& text, float font_size = 0.0f) const;
    
    // =============================================================================
    // ID MANAGEMENT
    // =============================================================================
    
    /**
     * @brief Get the ID stack
     */
    IDStack& get_id_stack() { return id_stack_; }
    const IDStack& get_id_stack() const { return id_stack_; }
    
    // =============================================================================
    // STORAGE ACCESS
    // =============================================================================
    
    /**
     * @brief Get persistent storage
     */
    Storage& get_storage() { return storage_; }
    const Storage& get_storage() const { return storage_; }
    
    // =============================================================================
    // STATE ACCESS
    // =============================================================================
    
    /**
     * @brief Get current frame state
     */
    FrameState& get_frame_state() { return frame_state_; }
    const FrameState& get_frame_state() const { return frame_state_; }
    
    /**
     * @brief Get frame data/statistics
     */
    const FrameData& get_frame_data() const { return frame_data_; }
    
    // =============================================================================
    // WIDGET INTERACTION
    // =============================================================================
    
    /**
     * @brief Register a widget for interaction
     */
    WidgetState register_widget(ID id, const Rect& bounds, bool can_focus = false);
    
    /**
     * @brief Set active widget (for mouse capture)
     */
    void set_active_id(ID id);
    
    /**
     * @brief Set focused widget (for keyboard input)
     */
    void set_focused_id(ID id);
    
    /**
     * @brief Check if widget is hovered
     */
    bool is_hovered(ID id) const { return frame_state_.hovered_id == id; }
    
    /**
     * @brief Check if widget is active
     */
    bool is_active(ID id) const { return frame_state_.active_id == id; }
    
    /**
     * @brief Check if widget is focused
     */
    bool is_focused(ID id) const { return frame_state_.focused_id == id; }
    
    // =============================================================================
    // DRAWING
    // =============================================================================
    
    /**
     * @brief Get the main draw list
     */
    DrawList& get_draw_list() { return *draw_list_; }
    
    /**
     * @brief Get window-specific draw list
     */
    DrawList& get_window_draw_list();
    
    // =============================================================================
    // TOOLTIPS
    // =============================================================================
    
    /**
     * @brief Set tooltip for hovered widget
     */
    void set_tooltip(const std::string& text);
    
    /**
     * @brief Check if tooltip should be shown
     */
    bool should_show_tooltip() const;
    
    /**
     * @brief Get tooltip system
     */
    TooltipSystem& get_tooltip_system() { return tooltip_system_; }
    
    // =============================================================================
    // MODALS
    // =============================================================================
    
    /**
     * @brief Open a modal dialog
     */
    void open_modal(const std::string& title, const Vec2& size, std::function<void()> content_callback, bool closable = true);
    
    /**
     * @brief Close current modal
     */
    void close_modal();
    
    /**
     * @brief Check if a modal is open
     */
    bool has_modal() const { return modal_system_.current_modal.has_value(); }
    
    // =============================================================================
    // DRAG AND DROP
    // =============================================================================
    
    /**
     * @brief Begin drag operation
     */
    bool begin_drag(ID source_id, const std::string& payload_type, const void* data, size_t size);
    
    /**
     * @brief End drag operation
     */
    void end_drag();
    
    /**
     * @brief Accept drop payload
     */
    const void* accept_drag_payload(const std::string& payload_type, size_t* out_size = nullptr) const;
    
    /**
     * @brief Check if currently dragging
     */
    bool is_dragging() const { return frame_state_.is_dragging; }
    
    // =============================================================================
    // INPUT QUERIES
    // =============================================================================
    
    /**
     * @brief Get mouse position
     */
    Vec2 get_mouse_pos() const { return frame_state_.mouse_pos; }
    
    /**
     * @brief Get mouse delta this frame
     */
    Vec2 get_mouse_delta() const { return frame_state_.mouse_delta; }
    
    /**
     * @brief Check if mouse button is down
     */
    bool is_mouse_down(MouseButton button) const;
    
    /**
     * @brief Check if mouse button was clicked this frame
     */
    bool is_mouse_clicked(MouseButton button) const;
    
    /**
     * @brief Check if mouse button was released this frame
     */
    bool is_mouse_released(MouseButton button) const;
    
    /**
     * @brief Get scroll delta
     */
    Vec2 get_scroll_delta() const { return frame_state_.scroll_delta; }
    
    /**
     * @brief Check if key is down
     */
    bool is_key_down(Key key) const;
    
    /**
     * @brief Check if key was pressed this frame
     */
    bool is_key_pressed(Key key) const;
    
    /**
     * @brief Check if key was released this frame
     */
    bool is_key_released(Key key) const;
    
    /**
     * @brief Get text input this frame
     */
    const std::string& get_text_input() const { return frame_state_.text_input; }
    
    // =============================================================================
    // LAYOUT SYSTEM
    // =============================================================================
    
    /**
     * @brief Get current layout
     */
    Layout* get_current_layout() const { return frame_state_.current_layout; }
    
    /**
     * @brief Set current layout
     */
    void set_current_layout(Layout* layout) { frame_state_.current_layout = layout; }
    
    // =============================================================================
    // DEBUGGING & PROFILING
    // =============================================================================
    
    /**
     * @brief Enable/disable debug visualization
     */
    void set_debug_enabled(bool enabled) { debug_enabled_ = enabled; }
    
    /**
     * @brief Check if debug mode is enabled
     */
    bool is_debug_enabled() const { return debug_enabled_; }
    
    /**
     * @brief Show debug overlay
     */
    void show_debug_overlay();
    
private:
    // Core systems
    rendering::IRenderer* renderer_ = nullptr;
    std::unique_ptr<Theme> theme_;
    std::unique_ptr<FontAtlas> font_atlas_;
    std::unique_ptr<InputManager> input_manager_;
    std::unique_ptr<DrawList> draw_list_;
    
    // State management
    IDStack id_stack_;
    Storage storage_;
    FrameState frame_state_;
    FrameData frame_data_;
    
    // Special systems
    TooltipSystem tooltip_system_;
    ModalSystem modal_system_;
    
    // Display info
    Vec2 display_size_;
    Vec2 display_scale_ = {1.0f, 1.0f};
    
    // Flags
    bool initialized_ = false;
    bool debug_enabled_ = false;
    
    // Rendering resources
    rendering::BufferHandle vertex_buffer_;
    rendering::BufferHandle index_buffer_;
    rendering::ShaderHandle gui_shader_;
    rendering::TextureHandle font_texture_;
    
    // Internal methods
    void update_input();
    void update_tooltips();
    void render_modals();
    void render_debug();
    void setup_render_state();
    void create_render_resources();
    void update_buffers();
    
    // Event processing
    void process_mouse_event(const Event& event);
    void process_keyboard_event(const Event& event);
    void process_text_input_event(const Event& event);
    
    // Interaction processing
    void update_hover_state();
    void update_active_state();
    void update_focus_state();
};

// =============================================================================
// GLOBAL CONTEXT ACCESS
// =============================================================================

/**
 * @brief Get the current GUI context
 * @note You must call set_current_context() first
 */
Context* get_current_context();

/**
 * @brief Set the current GUI context
 */
void set_current_context(Context* context);

/**
 * @brief RAII helper for context switching
 */
class ScopedContext {
public:
    explicit ScopedContext(Context* context);
    ~ScopedContext();
    
private:
    Context* previous_context_;
};

} // namespace ecscope::gui