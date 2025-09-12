/**
 * @file gui_manager.hpp
 * @brief ECScope GUI System Manager
 * 
 * Central manager for coordinating GUI systems, windows, and user interfaces
 * across the entire ECScope engine. Handles initialization, lifecycle, and
 * communication between different GUI components.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

#ifdef ECSCOPE_HAS_IMGUI
#include <imgui.h>
#endif

#ifdef ECSCOPE_HAS_GLFW
struct GLFWwindow;
#endif

// ECScope includes
#include "dashboard.hpp"
#include "../rendering/renderer.hpp"

namespace ecscope::gui {

// =============================================================================
// FORWARD DECLARATIONS
// =============================================================================

class Window;
class GUIComponent;

// =============================================================================
// ENUMERATIONS & TYPES
// =============================================================================

/**
 * @brief GUI initialization flags
 */
enum class GUIFlags : uint32_t {
    None = 0,
    EnableDocking = 1 << 0,
    EnableViewports = 1 << 1,
    EnableKeyboardNav = 1 << 2,
    EnableGamepadNav = 1 << 3,
    DarkTheme = 1 << 4,
    LightTheme = 1 << 5,
    HighDPI = 1 << 6
};

inline GUIFlags operator|(GUIFlags a, GUIFlags b) {
    return static_cast<GUIFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline bool operator&(GUIFlags a, GUIFlags b) {
    return (static_cast<uint32_t>(a) & static_cast<uint32_t>(b)) != 0;
}

/**
 * @brief Window creation parameters
 */
struct WindowConfig {
    std::string title = "ECScope Window";
    int width = 1280;
    int height = 720;
    bool fullscreen = false;
    bool resizable = true;
    bool vsync = true;
    bool decorated = true;
    int samples = 0; // MSAA samples
};

/**
 * @brief GUI component interface
 */
class GUIComponent {
public:
    virtual ~GUIComponent() = default;
    
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update(float delta_time) = 0;
    virtual void render() = 0;
    
    virtual const std::string& get_name() const = 0;
    virtual bool is_enabled() const = 0;
    virtual void set_enabled(bool enabled) = 0;
};

// =============================================================================
// MAIN GUI MANAGER CLASS
// =============================================================================

/**
 * @brief Central GUI System Manager
 * 
 * Manages the entire GUI system for ECScope, including:
 * - Window management and creation
 * - ImGui context and backend setup
 * - Dashboard integration
 * - Component lifecycle management
 * - Input handling and event routing
 * - Theme and style management
 */
class GUIManager {
public:
    GUIManager();
    ~GUIManager();

    // Non-copyable
    GUIManager(const GUIManager&) = delete;
    GUIManager& operator=(const GUIManager&) = delete;

    // =============================================================================
    // INITIALIZATION & LIFECYCLE
    // =============================================================================

    /**
     * @brief Initialize the GUI system
     * @param config Window configuration
     * @param flags GUI initialization flags
     * @param renderer Optional renderer for integration
     * @return true on success
     */
    bool initialize(const WindowConfig& config = WindowConfig{}, 
                   GUIFlags flags = GUIFlags::EnableDocking | GUIFlags::DarkTheme,
                   rendering::IRenderer* renderer = nullptr);

    /**
     * @brief Shutdown the GUI system
     */
    void shutdown();

    /**
     * @brief Check if GUI system is initialized
     */
    bool is_initialized() const { return initialized_; }

    // =============================================================================
    // MAIN LOOP INTEGRATION
    // =============================================================================

    /**
     * @brief Begin GUI frame
     * Called at the start of each frame before rendering GUI
     */
    void begin_frame();

    /**
     * @brief End GUI frame and present
     * Called at the end of each frame after all GUI rendering
     */
    void end_frame();

    /**
     * @brief Update GUI system
     * @param delta_time Time since last update
     */
    void update(float delta_time);

    /**
     * @brief Check if the main window should close
     */
    bool should_close() const;

    /**
     * @brief Poll window events
     */
    void poll_events();

    // =============================================================================
    // COMPONENT MANAGEMENT
    // =============================================================================

    /**
     * @brief Register a GUI component
     * @param component Unique pointer to the component
     */
    void register_component(std::unique_ptr<GUIComponent> component);

    /**
     * @brief Unregister a GUI component
     * @param name Component name
     */
    void unregister_component(const std::string& name);

    /**
     * @brief Get a GUI component by name
     * @param name Component name
     * @return Pointer to component or nullptr if not found
     */
    GUIComponent* get_component(const std::string& name);

    /**
     * @brief Enable/disable a component
     * @param name Component name
     * @param enabled Enable state
     */
    void set_component_enabled(const std::string& name, bool enabled);

    // =============================================================================
    // DASHBOARD INTEGRATION
    // =============================================================================

    /**
     * @brief Get the main dashboard
     */
    Dashboard* get_dashboard() const { return dashboard_.get(); }

    /**
     * @brief Show/hide the dashboard
     */
    void show_dashboard(bool show = true);

    /**
     * @brief Check if dashboard is visible
     */
    bool is_dashboard_visible() const { return dashboard_visible_; }

    // =============================================================================
    // WINDOW MANAGEMENT
    // =============================================================================

    /**
     * @brief Get the main window handle
     */
#ifdef ECSCOPE_HAS_GLFW
    GLFWwindow* get_main_window() const { return main_window_; }
#endif

    /**
     * @brief Get window size
     */
    std::pair<int, int> get_window_size() const;

    /**
     * @brief Set window size
     */
    void set_window_size(int width, int height);

    /**
     * @brief Get window title
     */
    const std::string& get_window_title() const { return window_config_.title; }

    /**
     * @brief Set window title
     */
    void set_window_title(const std::string& title);

    /**
     * @brief Toggle fullscreen mode
     */
    void toggle_fullscreen();

    /**
     * @brief Check if window is fullscreen
     */
    bool is_fullscreen() const { return fullscreen_; }

    // =============================================================================
    // STYLE & THEME MANAGEMENT
    // =============================================================================

    /**
     * @brief Set GUI theme
     */
    void set_theme(DashboardTheme theme);

    /**
     * @brief Get current theme
     */
    DashboardTheme get_theme() const { return current_theme_; }

    /**
     * @brief Set UI scale factor
     */
    void set_ui_scale(float scale);

    /**
     * @brief Get current UI scale
     */
    float get_ui_scale() const { return ui_scale_; }

    // =============================================================================
    // INPUT HANDLING
    // =============================================================================

    /**
     * @brief Check if GUI wants to capture mouse
     */
    bool wants_capture_mouse() const;

    /**
     * @brief Check if GUI wants to capture keyboard
     */
    bool wants_capture_keyboard() const;

    /**
     * @brief Add input callback
     */
    void add_input_callback(std::function<void(int, int, int, int)> callback);

    // =============================================================================
    // UTILITY FUNCTIONS
    // =============================================================================

    /**
     * @brief Show a modal message dialog
     * @param title Dialog title
     * @param message Dialog message
     * @param type Message type (info, warning, error)
     */
    void show_message_dialog(const std::string& title, const std::string& message, 
                           const std::string& type = "info");

    /**
     * @brief Show file dialog
     * @param title Dialog title
     * @param filters File filters
     * @param save True for save dialog, false for open dialog
     * @return Selected file path or empty string if cancelled
     */
    std::string show_file_dialog(const std::string& title, 
                               const std::vector<std::string>& filters = {},
                               bool save = false);

    /**
     * @brief Get performance metrics
     */
    struct GUIPerformanceMetrics {
        float frame_rate = 0.0f;
        float cpu_time_ms = 0.0f;
        float gpu_time_ms = 0.0f;
        size_t memory_usage = 0;
        uint32_t draw_calls = 0;
        uint32_t vertices = 0;
    };

    GUIPerformanceMetrics get_performance_metrics() const;

private:
    // =============================================================================
    // PRIVATE METHODS
    // =============================================================================

    bool initialize_glfw(const WindowConfig& config);
    bool initialize_imgui(GUIFlags flags);
    void setup_imgui_style(DashboardTheme theme);
    void cleanup_imgui();
    void cleanup_glfw();
    
    // Callback functions
    static void glfw_error_callback(int error, const char* description);
    static void glfw_framebuffer_size_callback(GLFWwindow* window, int width, int height);
    static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

    // =============================================================================
    // MEMBER VARIABLES
    // =============================================================================

    // Core state
    bool initialized_ = false;
    WindowConfig window_config_;
    GUIFlags gui_flags_ = GUIFlags::None;
    
    // Window management
#ifdef ECSCOPE_HAS_GLFW
    GLFWwindow* main_window_ = nullptr;
#endif
    bool fullscreen_ = false;
    int windowed_width_ = 1280;
    int windowed_height_ = 720;
    
    // GUI system
    std::unique_ptr<Dashboard> dashboard_;
    bool dashboard_visible_ = true;
    DashboardTheme current_theme_ = DashboardTheme::Dark;
    float ui_scale_ = 1.0f;
    
    // Component management
    std::unordered_map<std::string, std::unique_ptr<GUIComponent>> components_;
    
    // Rendering integration
    rendering::IRenderer* renderer_ = nullptr;
    
    // Input handling
    std::vector<std::function<void(int, int, int, int)>> input_callbacks_;
    
    // Performance tracking
    mutable GUIPerformanceMetrics performance_metrics_;
    
    // Message dialogs
    struct MessageDialog {
        std::string title;
        std::string message;
        std::string type;
        bool show = false;
    } pending_message_dialog_;
};

// =============================================================================
// UTILITY CLASSES
// =============================================================================

/**
 * @brief Simple GUI component base class
 */
class SimpleGUIComponent : public GUIComponent {
public:
    SimpleGUIComponent(const std::string& name) : name_(name) {}
    virtual ~SimpleGUIComponent() = default;
    
    const std::string& get_name() const override { return name_; }
    bool is_enabled() const override { return enabled_; }
    void set_enabled(bool enabled) override { enabled_ = enabled; }
    
    bool initialize() override { return true; }
    void shutdown() override {}
    void update(float delta_time) override { (void)delta_time; }
    
protected:
    std::string name_;
    bool enabled_ = true;
};

/**
 * @brief RAII GUI frame helper
 */
class ScopedGUIFrame {
public:
    ScopedGUIFrame(GUIManager* manager) : manager_(manager) {
        if (manager_) {
            manager_->begin_frame();
        }
    }
    
    ~ScopedGUIFrame() {
        if (manager_) {
            manager_->end_frame();
        }
    }

private:
    GUIManager* manager_;
};

// =============================================================================
// GLOBAL ACCESS
// =============================================================================

/**
 * @brief Get the global GUI manager instance
 */
GUIManager* GetGUIManager();

/**
 * @brief Initialize global GUI manager
 */
bool InitializeGlobalGUI(const WindowConfig& config = WindowConfig{}, 
                        GUIFlags flags = GUIFlags::EnableDocking | GUIFlags::DarkTheme,
                        rendering::IRenderer* renderer = nullptr);

/**
 * @brief Shutdown global GUI manager
 */
void ShutdownGlobalGUI();

} // namespace ecscope::gui