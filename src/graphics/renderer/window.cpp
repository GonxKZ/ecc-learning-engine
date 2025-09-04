#include "window.hpp"
#include "core/log.hpp"
#include <memory>

#ifdef ECSCOPE_HAS_GRAPHICS
#include <GL/gl.h>
#endif

namespace ecscope::renderer {

// Global window instance
static std::unique_ptr<Window> g_main_window = nullptr;

Window::Window(const WindowConfig& config)
    : config_(config) {
}

Window::~Window() {
    destroy();
}

Window::Window(Window&& other) noexcept 
    : config_(other.config_)
    , is_open_(other.is_open_)
    , stats_(other.stats_) {
#ifdef ECSCOPE_HAS_GRAPHICS
    window_ = other.window_;
    gl_context_ = other.gl_context_;
    other.window_ = nullptr;
    other.gl_context_ = nullptr;
#endif
    other.is_open_ = false;
}

Window& Window::operator=(Window&& other) noexcept {
    if (this != &other) {
        destroy();
        
        config_ = other.config_;
        is_open_ = other.is_open_;
        stats_ = other.stats_;
        
#ifdef ECSCOPE_HAS_GRAPHICS
        window_ = other.window_;
        gl_context_ = other.gl_context_;
        other.window_ = nullptr;
        other.gl_context_ = nullptr;
#endif
        other.is_open_ = false;
    }
    return *this;
}

core::CoreResult<void> Window::create() {
#ifdef ECSCOPE_HAS_GRAPHICS
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        LOG_ERROR("Failed to initialize SDL: " + std::string(SDL_GetError()));
        return core::CoreResult<void>::Err(core::CoreError::InitializationFailed);
    }
    
    // Set OpenGL attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    
    // Multi-sampling for anti-aliasing
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    
    // Create window flags
    Uint32 window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
    
    if (config_.resizable) {
        window_flags |= SDL_WINDOW_RESIZABLE;
    }
    
    if (config_.high_dpi) {
        window_flags |= SDL_WINDOW_ALLOW_HIGHDPI;
    }
    
    if (config_.fullscreen) {
        window_flags |= SDL_WINDOW_FULLSCREEN;
    }
    
    // Create window
    window_ = SDL_CreateWindow(
        config_.title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        static_cast<int>(config_.width),
        static_cast<int>(config_.height),
        window_flags
    );
    
    if (!window_) {
        LOG_ERROR("Failed to create SDL window: " + std::string(SDL_GetError()));
        SDL_Quit();
        return core::CoreResult<void>::Err(core::CoreError::InitializationFailed);
    }
    
    // Create OpenGL context
    gl_context_ = SDL_GL_CreateContext(window_);
    if (!gl_context_) {
        LOG_ERROR("Failed to create OpenGL context: " + std::string(SDL_GetError()));
        SDL_DestroyWindow(window_);
        SDL_Quit();
        return core::CoreResult<void>::Err(core::CoreError::InitializationFailed);
    }
    
    // Set VSync
    if (SDL_GL_SetSwapInterval(config_.vsync ? 1 : 0) < 0) {
        LOG_WARN("Warning: Unable to set VSync: " + std::string(SDL_GetError()));
    }
    
    // Update actual window size (might differ due to DPI scaling)
    update_config_from_window();
    
    is_open_ = true;
    stats_.vsync_enabled = config_.vsync;
    
    LOG_INFO("Window created successfully: " + std::to_string(config_.width) + "x" + std::to_string(config_.height));
    
    return core::CoreResult<void>::Ok();
#else
    LOG_WARN("Graphics support not compiled in - window creation skipped");
    return core::CoreResult<void>::Err(core::CoreError::NotSupported);
#endif
}

void Window::destroy() {
#ifdef ECSCOPE_HAS_GRAPHICS
    if (gl_context_) {
        SDL_GL_DeleteContext(gl_context_);
        gl_context_ = nullptr;
    }
    
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    
    if (is_open_) {
        SDL_Quit();
        is_open_ = false;
        LOG_INFO("Window destroyed");
    }
#endif
}

void Window::clear(f32 r, f32 g, f32 b, f32 a) {
#ifdef ECSCOPE_HAS_GRAPHICS
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#else
    (void)r; (void)g; (void)b; (void)a;
#endif
}

void Window::swap_buffers() {
#ifdef ECSCOPE_HAS_GRAPHICS
    if (window_) {
        SDL_GL_SwapWindow(window_);
    }
#endif
}

WindowEvent Window::poll_event() {
#ifdef ECSCOPE_HAS_GRAPHICS
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                return WindowEvent::Close;
                
            case SDL_WINDOWEVENT:
                switch (event.window.event) {
                    case SDL_WINDOWEVENT_RESIZED:
                        config_.width = static_cast<u32>(event.window.data1);
                        config_.height = static_cast<u32>(event.window.data2);
                        glViewport(0, 0, static_cast<int>(config_.width), static_cast<int>(config_.height));
                        return WindowEvent::Resize;
                        
                    case SDL_WINDOWEVENT_MINIMIZED:
                        return WindowEvent::Minimize;
                        
                    case SDL_WINDOWEVENT_MAXIMIZED:
                        return WindowEvent::Maximize;
                        
                    case SDL_WINDOWEVENT_FOCUS_GAINED:
                        return WindowEvent::Focus;
                        
                    case SDL_WINDOWEVENT_FOCUS_LOST:
                        return WindowEvent::Unfocus;
                }
                break;
        }
    }
#endif
    return WindowEvent::None;
}

void Window::set_title(const std::string& title) {
    config_.title = title;
#ifdef ECSCOPE_HAS_GRAPHICS
    if (window_) {
        SDL_SetWindowTitle(window_, title.c_str());
    }
#endif
}

void Window::set_size(u32 width, u32 height) {
    config_.width = width;
    config_.height = height;
#ifdef ECSCOPE_HAS_GRAPHICS
    if (window_) {
        SDL_SetWindowSize(window_, static_cast<int>(width), static_cast<int>(height));
        glViewport(0, 0, static_cast<int>(width), static_cast<int>(height));
    }
#endif
}

void Window::set_vsync(bool enabled) {
    config_.vsync = enabled;
#ifdef ECSCOPE_HAS_GRAPHICS
    if (gl_context_) {
        SDL_GL_SetSwapInterval(enabled ? 1 : 0);
        stats_.vsync_enabled = enabled;
    }
#endif
}

void Window::update_stats(f64 frame_time) {
    stats_.last_frame_time = frame_time;
    stats_.frame_count++;
    
    // Rolling average over last 60 frames
    constexpr f64 alpha = 1.0 / 60.0;
    stats_.average_frame_time = stats_.average_frame_time * (1.0 - alpha) + frame_time * alpha;
}

void Window::update_config_from_window() {
#ifdef ECSCOPE_HAS_GRAPHICS
    if (window_) {
        int w, h;
        SDL_GetWindowSize(window_, &w, &h);
        config_.width = static_cast<u32>(w);
        config_.height = static_cast<u32>(h);
    }
#endif
}

// Global window management
Window& get_main_window() {
    if (!g_main_window) {
        g_main_window = std::make_unique<Window>();
        LOG_INFO("Main window instance created");
    }
    return *g_main_window;
}

void set_main_window(std::unique_ptr<Window> window) {
    g_main_window = std::move(window);
    LOG_INFO("Main window instance set");
}

} // namespace ecscope::renderer