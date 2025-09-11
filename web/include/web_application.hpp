#pragma once

#include "ecscope/web/web_types.hpp"
#include <memory>
#include <chrono>

namespace ecscope::web {

/**
 * @brief Main WebAssembly application class for ECScope engine
 * 
 * This class manages the entire web application lifecycle, coordinates
 * all web-specific subsystems, and provides the main entry point for
 * JavaScript interaction.
 */
class WebApplication {
public:
    /**
     * @brief Construct a new WebApplication
     * @param config Application configuration
     */
    explicit WebApplication(const WebApplicationConfig& config);
    
    /**
     * @brief Destructor
     */
    ~WebApplication();
    
    // Non-copyable, movable
    WebApplication(const WebApplication&) = delete;
    WebApplication& operator=(const WebApplication&) = delete;
    WebApplication(WebApplication&&) noexcept = default;
    WebApplication& operator=(WebApplication&&) noexcept = default;
    
    /**
     * @brief Initialize the application
     * @return true if initialization succeeded
     */
    bool initialize();
    
    /**
     * @brief Shutdown the application
     */
    void shutdown();
    
    /**
     * @brief Update the application (called from JavaScript)
     * @param delta_time Time since last update in seconds
     */
    void update(double delta_time);
    
    /**
     * @brief Render the application (called from JavaScript)
     */
    void render();
    
    /**
     * @brief Handle resize event
     * @param width New canvas width
     * @param height New canvas height
     */
    void resize(int width, int height);
    
    /**
     * @brief Handle visibility change
     * @param visible Whether the application is visible
     */
    void set_visibility(bool visible);
    
    /**
     * @brief Handle focus change
     * @param focused Whether the application has focus
     */
    void set_focus(bool focused);
    
    /**
     * @brief Get current performance metrics
     * @return Performance metrics
     */
    PerformanceMetrics get_performance_metrics() const;
    
    /**
     * @brief Get browser capabilities
     * @return Browser capabilities
     */
    BrowserCapabilities get_browser_capabilities() const;
    
    /**
     * @brief Check if application is initialized
     * @return true if initialized
     */
    bool is_initialized() const noexcept { return initialized_; }
    
    /**
     * @brief Check if application is running
     * @return true if running
     */
    bool is_running() const noexcept { return running_; }
    
    /**
     * @brief Get renderer instance
     * @return WebRenderer reference
     */
    WebRenderer& get_renderer() const;
    
    /**
     * @brief Get audio system instance
     * @return WebAudio reference
     */
    WebAudio& get_audio() const;
    
    /**
     * @brief Get input system instance
     * @return WebInput reference
     */
    WebInput& get_input() const;
    
    /**
     * @brief Get filesystem instance
     * @return WebFileSystem reference
     */
    WebFileSystem& get_filesystem() const;
    
    /**
     * @brief Get networking instance
     * @return WebNetworking reference
     */
    WebNetworking& get_networking() const;
    
    /**
     * @brief Load asset from URL
     * @param url Asset URL
     * @param callback Completion callback
     */
    void load_asset(const std::string& url, std::function<void(bool, const std::vector<std::uint8_t>&)> callback);
    
    /**
     * @brief Execute JavaScript code
     * @param code JavaScript code to execute
     * @return Result value
     */
    JSValue execute_javascript(const std::string& code);
    
    /**
     * @brief Register JavaScript callback
     * @param name Callback name
     * @param callback C++ function to call
     */
    void register_callback(const std::string& name, JSFunction callback);
    
    /**
     * @brief Unregister JavaScript callback
     * @param name Callback name
     */
    void unregister_callback(const std::string& name);
    
    /**
     * @brief Set error handler
     * @param handler Error handler function
     */
    void set_error_handler(ErrorCallback handler);
    
private:
    // Configuration
    WebApplicationConfig config_;
    
    // State
    bool initialized_ = false;
    bool running_ = false;
    bool visible_ = true;
    bool focused_ = true;
    
    // Timing
    std::chrono::steady_clock::time_point last_update_time_;
    std::chrono::steady_clock::time_point last_render_time_;
    
    // Subsystems
    std::unique_ptr<WebRenderer> renderer_;
    std::unique_ptr<WebAudio> audio_;
    std::unique_ptr<WebInput> input_;
    std::unique_ptr<WebFileSystem> filesystem_;
    std::unique_ptr<WebNetworking> networking_;
    
    // Performance monitoring
    mutable PerformanceMetrics performance_metrics_;
    std::chrono::steady_clock::time_point frame_start_time_;
    
    // Browser capabilities cache
    mutable BrowserCapabilities browser_capabilities_;
    mutable bool capabilities_cached_ = false;
    
    // JavaScript callbacks
    std::unordered_map<std::string, JSFunction> js_callbacks_;
    
    // Error handling
    ErrorCallback error_handler_;
    
    // Internal methods
    void initialize_subsystems();
    void shutdown_subsystems();
    void update_performance_metrics();
    void detect_browser_capabilities() const;
    void handle_error(const WebError& error);
    
    // Static callbacks for Emscripten
    static void animation_frame_callback(double time, void* user_data);
    static void visibility_change_callback(int event_type, const EmscriptenVisibilityChangeEvent* event, void* user_data);
    static void focus_callback(int event_type, const EmscriptenFocusEvent* event, void* user_data);
    static void resize_callback(int event_type, const EmscriptenUiEvent* event, void* user_data);
};

} // namespace ecscope::web