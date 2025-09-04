#pragma once

#include "core/types.hpp"
#include "core/result.hpp"
#include <string>

#ifdef ECSCOPE_HAS_GRAPHICS
#include <SDL2/SDL.h>
#endif

namespace ecscope::renderer {

// Window creation parameters
struct WindowConfig {
    std::string title{"ECScope - ECS Engine"};
    u32 width{1280};
    u32 height{720};
    bool fullscreen{false};
    bool resizable{true};
    bool vsync{true};
    bool high_dpi{true};
};

// Window events
enum class WindowEvent {
    None,
    Close,
    Resize,
    Minimize,
    Maximize,
    Focus,
    Unfocus
};

class Window {
private:
#ifdef ECSCOPE_HAS_GRAPHICS
    SDL_Window* window_{nullptr};
    SDL_GLContext gl_context_{nullptr};
#endif
    WindowConfig config_;
    bool is_open_{false};
    
public:
    explicit Window(const WindowConfig& config = WindowConfig{});
    ~Window();
    
    // Non-copyable but movable
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&& other) noexcept;
    Window& operator=(Window&& other) noexcept;
    
    // Window lifecycle
    core::CoreResult<void> create();
    void destroy();
    bool is_open() const noexcept { return is_open_; }
    
    // Frame operations
    void clear(f32 r = 0.1f, f32 g = 0.1f, f32 b = 0.1f, f32 a = 1.0f);
    void swap_buffers();
    
    // Event handling
    WindowEvent poll_event();
    
    // Properties
    u32 width() const noexcept { return config_.width; }
    u32 height() const noexcept { return config_.height; }
    f32 aspect_ratio() const noexcept { return static_cast<f32>(config_.width) / static_cast<f32>(config_.height); }
    const std::string& title() const noexcept { return config_.title; }
    
    // Window state
    void set_title(const std::string& title);
    void set_size(u32 width, u32 height);
    void set_vsync(bool enabled);
    
#ifdef ECSCOPE_HAS_GRAPHICS
    // Platform specific
    SDL_Window* native_handle() const noexcept { return window_; }
    SDL_GLContext gl_context() const noexcept { return gl_context_; }
#endif
    
    // Statistics for UI
    struct Stats {
        f64 last_frame_time{0.0};
        f64 average_frame_time{0.0};
        u32 frame_count{0};
        bool vsync_enabled{false};
    };
    
    const Stats& stats() const noexcept { return stats_; }
    void update_stats(f64 frame_time);
    
private:
    Stats stats_;
    
    void update_config_from_window();
};

// Global window management
Window& get_main_window();
void set_main_window(std::unique_ptr<Window> window);

} // namespace ecscope::renderer