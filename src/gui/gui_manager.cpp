/**
 * @file gui_manager.cpp
 * @brief ECScope GUI System Manager Implementation
 * 
 * Implementation of the central GUI manager for coordinating all
 * GUI systems, windows, and user interfaces in ECScope.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#include "../../include/ecscope/gui/gui_manager.hpp"
#include "../../include/ecscope/core/log.hpp"

#ifdef ECSCOPE_HAS_IMGUI
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#endif

#ifdef ECSCOPE_HAS_GLFW
#include <GLFW/glfw3.h>
#endif

#ifdef ECSCOPE_HAS_OPENGL
#include <GL/gl.h>
#ifdef _WIN32
#include <GL/glext.h>
#include <GL/wglext.h>
#else
#include <GL/glext.h>
#endif
#endif

#include <algorithm>
#include <chrono>

namespace ecscope::gui {

// =============================================================================
// GLOBAL GUI MANAGER INSTANCE
// =============================================================================

namespace {
    static std::unique_ptr<GUIManager> g_gui_manager = nullptr;
}

// =============================================================================
// GUI MANAGER IMPLEMENTATION
// =============================================================================

GUIManager::GUIManager() = default;

GUIManager::~GUIManager() {
    shutdown();
}

bool GUIManager::initialize(const WindowConfig& config, GUIFlags flags, rendering::IRenderer* renderer) {
    if (initialized_) {
        core::Log::warning("GUIManager: Already initialized");
        return true;
    }

    core::Log::info("GUIManager: Initializing GUI system...");
    
    window_config_ = config;
    gui_flags_ = flags;
    renderer_ = renderer;

    // Initialize GLFW
    if (!initialize_glfw(config)) {
        core::Log::error("GUIManager: Failed to initialize GLFW");
        return false;
    }

    // Initialize ImGui
    if (!initialize_imgui(flags)) {
        core::Log::error("GUIManager: Failed to initialize ImGui");
        cleanup_glfw();
        return false;
    }

    // Initialize dashboard
    dashboard_ = std::make_unique<Dashboard>();
    if (!dashboard_->initialize(renderer_)) {
        core::Log::error("GUIManager: Failed to initialize dashboard");
        cleanup_imgui();
        cleanup_glfw();
        return false;
    }

    initialized_ = true;
    core::Log::info("GUIManager: Initialized successfully");
    return true;
}

void GUIManager::shutdown() {
    if (!initialized_) {
        return;
    }

    core::Log::info("GUIManager: Shutting down GUI system...");

    // Shutdown all components
    for (auto& [name, component] : components_) {
        if (component) {
            component->shutdown();
        }
    }
    components_.clear();

    // Shutdown dashboard
    if (dashboard_) {
        dashboard_->shutdown();
        dashboard_.reset();
    }

    // Cleanup ImGui
    cleanup_imgui();

    // Cleanup GLFW
    cleanup_glfw();

    initialized_ = false;
    core::Log::info("GUIManager: Shut down successfully");
}

void GUIManager::begin_frame() {
    if (!initialized_) {
        return;
    }

#ifdef ECSCOPE_HAS_IMGUI
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
#endif
}

void GUIManager::end_frame() {
    if (!initialized_) {
        return;
    }

#ifdef ECSCOPE_HAS_IMGUI
    // Render ImGui
    ImGui::Render();
    
    // Get window size for viewport
    int display_w, display_h;
#ifdef ECSCOPE_HAS_GLFW
    glfwGetFramebufferSize(main_window_, &display_w, &display_h);
#else
    display_w = window_config_.width;
    display_h = window_config_.height;
#endif

#ifdef ECSCOPE_HAS_OPENGL
    glViewport(0, 0, display_w, display_h);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif

    // Multi-viewport not available in this ImGui version
    // Simplified viewport handling
#endif

#ifdef ECSCOPE_HAS_GLFW
    // Swap buffers
    glfwSwapBuffers(main_window_);
#endif
}

void GUIManager::update(float delta_time) {
    if (!initialized_) {
        return;
    }

    // Update dashboard
    if (dashboard_) {
        dashboard_->update(delta_time);
    }

    // Update all components
    for (auto& [name, component] : components_) {
        if (component && component->is_enabled()) {
            component->update(delta_time);
        }
    }

    // Render dashboard if visible
    if (dashboard_visible_ && dashboard_) {
        dashboard_->render();
    }

    // Render all enabled components
    for (auto& [name, component] : components_) {
        if (component && component->is_enabled()) {
            component->render();
        }
    }

    // Show pending message dialog (temporarily disabled due to missing implementation)
    // if (pending_message_dialog_.show) {
    //     show_message_dialog_impl();
    // }
}

bool GUIManager::should_close() const {
#ifdef ECSCOPE_HAS_GLFW
    return main_window_ ? glfwWindowShouldClose(main_window_) : true;
#else
    return false;
#endif
}

void GUIManager::poll_events() {
#ifdef ECSCOPE_HAS_GLFW
    glfwPollEvents();
#endif
}

// =============================================================================
// COMPONENT MANAGEMENT
// =============================================================================

void GUIManager::register_component(std::unique_ptr<GUIComponent> component) {
    if (!component) {
        core::Log::warning("GUIManager: Attempted to register null component");
        return;
    }

    const std::string& name = component->get_name();
    
    if (components_.find(name) != components_.end()) {
        core::Log::warning("GUIManager: Component '{}' already registered, replacing", name);
        components_[name]->shutdown();
    }

    if (component->initialize()) {
        components_[name] = std::move(component);
        core::Log::info("GUIManager: Registered component '{}'", name);
    } else {
        core::Log::error("GUIManager: Failed to initialize component '{}'", name);
    }
}

void GUIManager::unregister_component(const std::string& name) {
    auto it = components_.find(name);
    if (it != components_.end()) {
        it->second->shutdown();
        components_.erase(it);
        core::Log::info("GUIManager: Unregistered component '{}'", name);
    }
}

GUIComponent* GUIManager::get_component(const std::string& name) {
    auto it = components_.find(name);
    return (it != components_.end()) ? it->second.get() : nullptr;
}

void GUIManager::set_component_enabled(const std::string& name, bool enabled) {
    auto it = components_.find(name);
    if (it != components_.end()) {
        it->second->set_enabled(enabled);
    }
}

// =============================================================================
// DASHBOARD INTEGRATION
// =============================================================================

void GUIManager::show_dashboard(bool show) {
    dashboard_visible_ = show;
}

// =============================================================================
// WINDOW MANAGEMENT
// =============================================================================

std::pair<int, int> GUIManager::get_window_size() const {
#ifdef ECSCOPE_HAS_GLFW
    if (main_window_) {
        int width, height;
        glfwGetWindowSize(main_window_, &width, &height);
        return {width, height};
    }
#endif
    return {window_config_.width, window_config_.height};
}

void GUIManager::set_window_size(int width, int height) {
#ifdef ECSCOPE_HAS_GLFW
    if (main_window_ && !fullscreen_) {
        glfwSetWindowSize(main_window_, width, height);
        window_config_.width = width;
        window_config_.height = height;
    }
#endif
}

void GUIManager::set_window_title(const std::string& title) {
#ifdef ECSCOPE_HAS_GLFW
    if (main_window_) {
        glfwSetWindowTitle(main_window_, title.c_str());
        window_config_.title = title;
    }
#endif
}

void GUIManager::toggle_fullscreen() {
#ifdef ECSCOPE_HAS_GLFW
    if (!main_window_) return;

    if (fullscreen_) {
        // Switch to windowed mode
        glfwSetWindowMonitor(main_window_, nullptr, 100, 100, 
                           windowed_width_, windowed_height_, GLFW_DONT_CARE);
        fullscreen_ = false;
        core::Log::info("GUIManager: Switched to windowed mode");
    } else {
        // Store current windowed size
        glfwGetWindowSize(main_window_, &windowed_width_, &windowed_height_);
        
        // Switch to fullscreen mode
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(main_window_, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        fullscreen_ = true;
        core::Log::info("GUIManager: Switched to fullscreen mode");
    }
#endif
}

// =============================================================================
// STYLE & THEME MANAGEMENT
// =============================================================================

void GUIManager::set_theme(DashboardTheme theme) {
    current_theme_ = theme;
    setup_imgui_style(theme);
    
    if (dashboard_) {
        dashboard_->set_theme(theme);
    }
}

void GUIManager::set_ui_scale(float scale) {
    ui_scale_ = std::max(0.5f, std::min(scale, 3.0f)); // Clamp between 0.5x and 3.0x
    
#ifdef ECSCOPE_HAS_IMGUI
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = ui_scale_;
    
    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiStyle default_style;
    style = default_style;
    style.ScaleAllSizes(ui_scale_);
#endif
}

// =============================================================================
// INPUT HANDLING
// =============================================================================

bool GUIManager::wants_capture_mouse() const {
#ifdef ECSCOPE_HAS_IMGUI
    return ImGui::GetIO().WantCaptureMouse;
#else
    return false;
#endif
}

bool GUIManager::wants_capture_keyboard() const {
#ifdef ECSCOPE_HAS_IMGUI
    return ImGui::GetIO().WantCaptureKeyboard;
#else
    return false;
#endif
}

void GUIManager::add_input_callback(std::function<void(int, int, int, int)> callback) {
    input_callbacks_.push_back(std::move(callback));
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

void GUIManager::show_message_dialog(const std::string& title, const std::string& message, const std::string& type) {
    pending_message_dialog_.title = title;
    pending_message_dialog_.message = message;
    pending_message_dialog_.type = type;
    pending_message_dialog_.show = true;
}

std::string GUIManager::show_file_dialog(const std::string& title, 
                                       const std::vector<std::string>& filters, bool save) {
    // TODO: Implement native file dialog or integrate with a library like nativefiledialog
    core::Log::info("GUIManager: File dialog requested - {}", title);
    return ""; // Placeholder
}

GUIManager::GUIPerformanceMetrics GUIManager::get_performance_metrics() const {
    // Update performance metrics
    auto& metrics = performance_metrics_;
    
#ifdef ECSCOPE_HAS_IMGUI
    ImGuiIO& io = ImGui::GetIO();
    metrics.frame_rate = io.Framerate;
    metrics.cpu_time_ms = 1000.0f / io.Framerate;
#endif
    
    // TODO: Integrate with renderer for GPU metrics
    if (renderer_) {
        auto renderer_stats = renderer_->get_frame_stats();
        metrics.gpu_time_ms = renderer_stats.gpu_time_ms;
        metrics.draw_calls = renderer_stats.draw_calls;
        metrics.vertices = renderer_stats.vertices_rendered;
        metrics.memory_usage = renderer_stats.memory_used;
    }
    
    return metrics;
}

// =============================================================================
// PRIVATE METHODS
// =============================================================================

bool GUIManager::initialize_glfw(const WindowConfig& config) {
#ifndef ECSCOPE_HAS_GLFW
    core::Log::error("GUIManager: GLFW not available in build configuration");
    return false;
#endif

#ifdef ECSCOPE_HAS_GLFW
    // Setup error callback
    glfwSetErrorCallback(glfw_error_callback);

    if (!glfwInit()) {
        core::Log::error("GUIManager: Failed to initialize GLFW");
        return false;
    }

    // GL 3.3 + GLSL 330
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    // Window hints
    glfwWindowHint(GLFW_RESIZABLE, config.resizable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_DECORATED, config.decorated ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_SAMPLES, config.samples);

    // Create window
    GLFWmonitor* monitor = config.fullscreen ? glfwGetPrimaryMonitor() : nullptr;
    main_window_ = glfwCreateWindow(config.width, config.height, 
                                  config.title.c_str(), monitor, nullptr);
    
    if (!main_window_) {
        core::Log::error("GUIManager: Failed to create GLFW window");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(main_window_);
    glfwSwapInterval(config.vsync ? 1 : 0); // Enable/disable vsync

    // Setup callbacks
    glfwSetWindowUserPointer(main_window_, this);
    glfwSetFramebufferSizeCallback(main_window_, glfw_framebuffer_size_callback);
    glfwSetKeyCallback(main_window_, glfw_key_callback);

#ifdef ECSCOPE_HAS_OPENGL
    // Initialize OpenGL context (no loader needed with standard OpenGL headers)
    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    if (!version) {
        core::Log::error("GUIManager: Failed to get OpenGL version");
        return false;
    }
    core::Log::info("GUIManager: OpenGL Version: {}", version);
#endif

    fullscreen_ = config.fullscreen;
    if (!fullscreen_) {
        windowed_width_ = config.width;
        windowed_height_ = config.height;
    }

    return true;
#endif // ECSCOPE_HAS_GLFW
    return false;
}

bool GUIManager::initialize_imgui(GUIFlags flags) {
#ifndef ECSCOPE_HAS_IMGUI
    core::Log::error("GUIManager: ImGui not available in build configuration");
    return false;
#endif

#ifdef ECSCOPE_HAS_IMGUI
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // Configure flags (docking and viewports not available in this ImGui version)
    // if (flags & GUIFlags::EnableDocking) {
    //     io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // }
    // if (flags & GUIFlags::EnableViewports) {
    //     io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    // }
    if (flags & GUIFlags::EnableKeyboardNav) {
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    }
    if (flags & GUIFlags::EnableGamepadNav) {
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    }

    // Setup Dear ImGui style
    DashboardTheme theme = DashboardTheme::Dark;
    if (flags & GUIFlags::LightTheme) {
        theme = DashboardTheme::Light;
    }
    setup_imgui_style(theme);

    // Viewport tweaks not needed in this ImGui version
    // (ViewportsEnable flag not available)

    // Setup Platform/Renderer backends
#ifdef ECSCOPE_HAS_GLFW
    ImGui_ImplGlfw_InitForOpenGL(main_window_, true);
#endif
    
#ifdef ECSCOPE_HAS_OPENGL
    ImGui_ImplOpenGL3_Init("#version 330");
#endif

    return true;
#endif // ECSCOPE_HAS_IMGUI
    return false;
}

void GUIManager::setup_imgui_style(DashboardTheme theme) {
#ifdef ECSCOPE_HAS_IMGUI
    switch (theme) {
        case DashboardTheme::Dark:
            setup_professional_dark_theme();
            break;
        case DashboardTheme::Light:
            setup_clean_light_theme();
            break;
        case DashboardTheme::HighContrast:
            setup_high_contrast_theme();
            break;
        case DashboardTheme::Custom:
            // Keep current style
            break;
    }
    
    current_theme_ = theme;
#endif
}

void GUIManager::cleanup_imgui() {
#ifdef ECSCOPE_HAS_IMGUI
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
#endif
}

void GUIManager::cleanup_glfw() {
#ifdef ECSCOPE_HAS_GLFW
    if (main_window_) {
        glfwDestroyWindow(main_window_);
        main_window_ = nullptr;
    }
    glfwTerminate();
#endif
}

// Message dialog implementation removed due to header mismatch
// void GUIManager::show_message_dialog_impl() {
//     // Implementation would go here
// }

// =============================================================================
// GLFW CALLBACKS
// =============================================================================

void GUIManager::glfw_error_callback(int error, const char* description) {
    core::Log::error("GLFW Error {}: {}", error, description);
}

void GUIManager::glfw_framebuffer_size_callback(GLFWwindow* window, int width, int height) {
#ifdef ECSCOPE_HAS_OPENGL
    glViewport(0, 0, width, height);
#endif
    
    GUIManager* gui_manager = static_cast<GUIManager*>(glfwGetWindowUserPointer(window));
    if (gui_manager) {
        gui_manager->window_config_.width = width;
        gui_manager->window_config_.height = height;
    }
}

void GUIManager::glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    GUIManager* gui_manager = static_cast<GUIManager*>(glfwGetWindowUserPointer(window));
    if (gui_manager) {
        // Call registered input callbacks
        for (auto& callback : gui_manager->input_callbacks_) {
            callback(key, scancode, action, mods);
        }
        
        // Handle global shortcuts
        if (action == GLFW_PRESS) {
            if (key == GLFW_KEY_F11) {
                gui_manager->toggle_fullscreen();
            } else if (key == GLFW_KEY_F1) {
                gui_manager->show_dashboard(!gui_manager->is_dashboard_visible());
            }
        }
    }
}

// =============================================================================
// GLOBAL ACCESS FUNCTIONS
// =============================================================================

GUIManager* GetGUIManager() {
    return g_gui_manager.get();
}

bool InitializeGlobalGUI(const WindowConfig& config, GUIFlags flags, rendering::IRenderer* renderer) {
    if (g_gui_manager) {
        core::Log::warning("Global GUI manager already initialized");
        return true;
    }

    g_gui_manager = std::make_unique<GUIManager>();
    return g_gui_manager->initialize(config, flags, renderer);
}

void ShutdownGlobalGUI() {
    if (g_gui_manager) {
        g_gui_manager->shutdown();
        g_gui_manager.reset();
    }
}

} // namespace ecscope::gui