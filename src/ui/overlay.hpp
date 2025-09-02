#pragma once

#include "core/types.hpp"
#include "core/result.hpp"
#include "renderer/window.hpp"
#include <memory>
#include <vector>

// Forward declaration to avoid ImGui dependency in header
struct ImGuiContext;
struct ImGuiIO;

namespace ecscope::ui {

// Base class for UI panels
class Panel {
public:
    Panel(const std::string& name, bool visible = true) 
        : name_(name), visible_(visible) {}
    
    virtual ~Panel() = default;
    
    // Panel interface
    virtual void render() = 0;
    virtual void update(f64 delta_time) { (void)delta_time; }
    
    // Panel state
    const std::string& name() const noexcept { return name_; }
    bool is_visible() const noexcept { return visible_; }
    void set_visible(bool visible) noexcept { visible_ = visible; }
    void toggle_visible() noexcept { visible_ = !visible_; }
    
protected:
    std::string name_;
    bool visible_;
};

// UI Overlay manager
class UIOverlay {
private:
#ifdef ECSCOPE_HAS_GRAPHICS
    ImGuiContext* imgui_context_{nullptr};
    ImGuiIO* imgui_io_{nullptr};
#endif
    
    std::vector<std::unique_ptr<Panel>> panels_;
    bool show_demo_window_{false};
    bool show_metrics_window_{false};
    bool initialized_{false};
    
    // UI state
    f32 ui_scale_{1.0f};
    bool dark_theme_{true};
    
public:
    UIOverlay() = default;
    ~UIOverlay();
    
    // Non-copyable but movable
    UIOverlay(const UIOverlay&) = delete;
    UIOverlay& operator=(const UIOverlay&) = delete;
    UIOverlay(UIOverlay&&) = default;
    UIOverlay& operator=(UIOverlay&&) = default;
    
    // Lifecycle
    core::CoreResult<void> initialize(renderer::Window& window);
    void shutdown();
    bool is_initialized() const noexcept { return initialized_; }
    
    // Frame operations
    void begin_frame();
    void end_frame();
    void render();
    void update(f64 delta_time);
    
    // Panel management
    template<typename T, typename... Args>
    T* add_panel(Args&&... args) {
        static_assert(std::is_base_of_v<Panel, T>, "T must derive from Panel");
        auto panel = std::make_unique<T>(std::forward<Args>(args)...);
        T* panel_ptr = panel.get();
        panels_.push_back(std::move(panel));
        return panel_ptr;
    }
    
    void remove_panel(const std::string& name);
    Panel* get_panel(const std::string& name);
    const Panel* get_panel(const std::string& name) const;
    
    // UI settings
    void set_ui_scale(f32 scale);
    f32 ui_scale() const noexcept { return ui_scale_; }
    void set_dark_theme(bool dark);
    bool dark_theme() const noexcept { return dark_theme_; }
    
    // Debug windows
    void show_demo_window(bool show = true) { show_demo_window_ = show; }
    void show_metrics_window(bool show = true) { show_metrics_window_ = show; }
    
    // Event handling
    void handle_window_event(renderer::WindowEvent event);
    
    // Statistics
    struct Stats {
        f64 render_time{0.0};
        u32 active_panels{0};
        u32 total_panels{0};
        bool capturing_mouse{false};
        bool capturing_keyboard{false};
    };
    
    Stats get_stats() const;
    
private:
    void setup_style();
    void render_main_menu_bar();
    void render_debug_windows();
};

// Global UI overlay
UIOverlay& get_ui_overlay();
void set_ui_overlay(std::unique_ptr<UIOverlay> overlay);

// Utility functions for ImGui
namespace imgui_utils {
    void help_marker(const char* desc);
    bool color_button(const char* desc_id, const float* col, f32 size = 0.0f);
    void progress_bar_animated(f32 fraction, const char* overlay = nullptr);
    
    // Memory size formatting
    std::string format_bytes(usize bytes);
    std::string format_number(u64 number);
    
    // Performance helpers
    void plot_frame_times(const f32* values, usize count, f32 scale_min = 0.0f, f32 scale_max = 33.33f);
    void memory_usage_pie_chart(const char* label, usize used, usize total);
}

} // namespace ecscope::ui