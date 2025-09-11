#include "../include/web_application.hpp"
#include "../include/web_renderer.hpp"
#include "../include/web_audio.hpp"
#include "../include/web_input.hpp"
#include "../include/web_filesystem.hpp"
#include "../include/web_networking.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/val.h>
#include <emscripten/bind.h>
#endif

#include <chrono>
#include <stdexcept>

namespace ecscope::web {

WebApplication::WebApplication(const WebApplicationConfig& config)
    : config_(config) {
    
    last_update_time_ = std::chrono::steady_clock::now();
    last_render_time_ = std::chrono::steady_clock::now();
    
    // Initialize performance metrics
    performance_metrics_ = {};
    
    // Set default error handler if none provided
    if (!config_.error_callback) {
        error_handler_ = [](const WebError& error) {
            // Log to browser console
            emscripten::val::global("console").call<void>("error", 
                std::string("ECScope Error: ") + error.message);
        };
    } else {
        error_handler_ = config_.error_callback;
    }
}

WebApplication::~WebApplication() {
    if (initialized_) {
        shutdown();
    }
}

bool WebApplication::initialize() {
    if (initialized_) {
        return true;
    }
    
    try {
        // Detect browser capabilities first
        detect_browser_capabilities();
        
        // Initialize subsystems
        initialize_subsystems();
        
        // Register global callbacks
        #ifdef __EMSCRIPTEN__
        emscripten_set_visibilitychange_callback(this, EM_TRUE, visibility_change_callback);
        emscripten_set_focus_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, this, EM_TRUE, focus_callback);
        emscripten_set_blur_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, this, EM_TRUE, focus_callback);
        emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, EM_TRUE, resize_callback);
        #endif
        
        // Start main loop
        running_ = true;
        initialized_ = true;
        
        #ifdef __EMSCRIPTEN__
        emscripten_request_animation_frame_loop(animation_frame_callback, this);
        #endif
        
        return true;
    } catch (const std::exception& e) {
        WebError error{
            WebErrorType::NotSupportedError,
            std::string("Failed to initialize WebApplication: ") + e.what(),
            "",
            0
        };
        handle_error(error);
        return false;
    }
}

void WebApplication::shutdown() {
    if (!initialized_) {
        return;
    }
    
    running_ = false;
    
    // Unregister callbacks
    #ifdef __EMSCRIPTEN__
    emscripten_set_visibilitychange_callback(nullptr, EM_FALSE, nullptr);
    emscripten_set_focus_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, nullptr, EM_FALSE, nullptr);
    emscripten_set_blur_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, nullptr, EM_FALSE, nullptr);
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_FALSE, nullptr);
    #endif
    
    // Shutdown subsystems
    shutdown_subsystems();
    
    // Clear callbacks
    js_callbacks_.clear();
    
    initialized_ = false;
}

void WebApplication::update(double delta_time) {
    if (!running_ || !visible_) {
        return;
    }
    
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        // Update subsystems
        if (audio_ && config_.audio.enable_spatial_audio) {
            audio_->update(delta_time);
        }
        
        if (input_ && config_.enable_input) {
            input_->update(delta_time);
        }
        
        // Update performance metrics
        update_performance_metrics();
        
        // Call performance callback if enabled
        if (config_.enable_performance_monitoring && config_.performance_callback) {
            config_.performance_callback(performance_metrics_);
        }
        
    } catch (const std::exception& e) {
        WebError error{
            WebErrorType::NotSupportedError,
            std::string("Error during update: ") + e.what(),
            "",
            0
        };
        handle_error(error);
    }
    
    auto end_time = std::chrono::steady_clock::now();
    performance_metrics_.update_time_ms = 
        std::chrono::duration<double, std::milli>(end_time - start_time).count();
    
    last_update_time_ = std::chrono::steady_clock::now();
}

void WebApplication::render() {
    if (!running_ || !visible_) {
        return;
    }
    
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        if (renderer_ && renderer_->is_initialized()) {
            renderer_->begin_frame();
            
            // Clear with a default color
            renderer_->clear(0.1f, 0.1f, 0.1f, 1.0f);
            
            // TODO: Add actual rendering logic here
            // This would typically involve:
            // - Setting up camera matrices
            // - Rendering ECS systems
            // - UI rendering
            // - Post-processing effects
            
            renderer_->end_frame();
            
            // Update render statistics
            auto stats = renderer_->get_render_stats();
            performance_metrics_.draw_calls = stats.draw_calls;
            performance_metrics_.triangles = stats.triangles;
        }
        
    } catch (const std::exception& e) {
        WebError error{
            WebErrorType::WebGLContextLost,
            std::string("Error during render: ") + e.what(),
            "",
            0
        };
        handle_error(error);
    }
    
    auto end_time = std::chrono::steady_clock::now();
    performance_metrics_.render_time_ms = 
        std::chrono::duration<double, std::milli>(end_time - start_time).count();
    
    last_render_time_ = std::chrono::steady_clock::now();
}

void WebApplication::resize(int width, int height) {
    config_.canvas.width = width;
    config_.canvas.height = height;
    
    if (renderer_ && renderer_->is_initialized()) {
        renderer_->resize(width, height);
    }
}

void WebApplication::set_visibility(bool visible) {
    visible_ = visible;
    
    if (!visible && audio_ && audio_->is_context_running()) {
        // Suspend audio context when not visible to save resources
        audio_->suspend_context();
    } else if (visible && audio_ && !audio_->is_context_running()) {
        // Resume audio context when visible
        audio_->resume_context();
    }
}

void WebApplication::set_focus(bool focused) {
    focused_ = focused;
    
    if (input_) {
        input_->set_focus(focused);
        if (!focused) {
            input_->clear_state();
        }
    }
}

PerformanceMetrics WebApplication::get_performance_metrics() const {
    return performance_metrics_;
}

BrowserCapabilities WebApplication::get_browser_capabilities() const {
    if (!capabilities_cached_) {
        detect_browser_capabilities();
        capabilities_cached_ = true;
    }
    return browser_capabilities_;
}

WebRenderer& WebApplication::get_renderer() const {
    if (!renderer_) {
        throw std::runtime_error("Renderer not initialized");
    }
    return *renderer_;
}

WebAudio& WebApplication::get_audio() const {
    if (!audio_) {
        throw std::runtime_error("Audio system not initialized");
    }
    return *audio_;
}

WebInput& WebApplication::get_input() const {
    if (!input_) {
        throw std::runtime_error("Input system not initialized");
    }
    return *input_;
}

WebFileSystem& WebApplication::get_filesystem() const {
    if (!filesystem_) {
        throw std::runtime_error("Filesystem not initialized");
    }
    return *filesystem_;
}

WebNetworking& WebApplication::get_networking() const {
    if (!networking_) {
        throw std::runtime_error("Networking not initialized");
    }
    return *networking_;
}

void WebApplication::load_asset(const std::string& url, 
                               std::function<void(bool, const std::vector<std::uint8_t>&)> callback) {
    // Use Emscripten fetch API for loading assets
    #ifdef __EMSCRIPTEN__
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    
    // Create callback wrapper
    auto* callback_ptr = new std::function<void(bool, const std::vector<std::uint8_t>&)>(callback);
    attr.userData = callback_ptr;
    
    attr.onsuccess = [](emscripten_fetch_t* fetch) {
        auto* cb = static_cast<std::function<void(bool, const std::vector<std::uint8_t>&)>*>(fetch->userData);
        
        std::vector<std::uint8_t> data(fetch->data, fetch->data + fetch->numBytes);
        (*cb)(true, data);
        
        delete cb;
        emscripten_fetch_close(fetch);
    };
    
    attr.onerror = [](emscripten_fetch_t* fetch) {
        auto* cb = static_cast<std::function<void(bool, const std::vector<std::uint8_t>&)>*>(fetch->userData);
        
        std::vector<std::uint8_t> empty_data;
        (*cb)(false, empty_data);
        
        delete cb;
        emscripten_fetch_close(fetch);
    };
    
    emscripten_fetch(&attr, url.c_str());
    #endif
}

JSValue WebApplication::execute_javascript(const std::string& code) {
    #ifdef __EMSCRIPTEN__
    try {
        return emscripten::val::global("eval")(code);
    } catch (const std::exception& e) {
        WebError error{
            WebErrorType::SecurityError,
            std::string("JavaScript execution error: ") + e.what(),
            "",
            0
        };
        handle_error(error);
        return emscripten::val::undefined();
    }
    #else
    return {};
    #endif
}

void WebApplication::register_callback(const std::string& name, JSFunction callback) {
    js_callbacks_[name] = callback;
    
    #ifdef __EMSCRIPTEN__
    // Make callback available to JavaScript
    emscripten::val::global().set(name.c_str(), emscripten::val::undefined());
    #endif
}

void WebApplication::unregister_callback(const std::string& name) {
    js_callbacks_.erase(name);
    
    #ifdef __EMSCRIPTEN__
    emscripten::val::global().delete_(name.c_str());
    #endif
}

void WebApplication::set_error_handler(ErrorCallback handler) {
    error_handler_ = handler;
}

// Private methods

void WebApplication::initialize_subsystems() {
    // Initialize renderer
    WebRenderer::RenderTarget target{
        config_.canvas.canvas_id,
        config_.canvas.width,
        config_.canvas.height,
        1.0f,  // device pixel ratio - will be detected
        false  // not offscreen
    };
    
    renderer_ = std::make_unique<WebRenderer>(target, WebRenderer::Backend::Auto);
    if (!renderer_->initialize()) {
        throw std::runtime_error("Failed to initialize renderer");
    }
    
    // Initialize audio system
    if (config_.audio.enable_spatial_audio || config_.audio.enable_effects) {
        audio_ = std::make_unique<WebAudio>(config_.audio);
        if (!audio_->initialize()) {
            throw std::runtime_error("Failed to initialize audio system");
        }
    }
    
    // Initialize input system
    if (config_.enable_input) {
        input_ = std::make_unique<WebInput>(config_.canvas.canvas_id);
        if (!input_->initialize()) {
            throw std::runtime_error("Failed to initialize input system");
        }
        
        // Set input callback if provided
        if (config_.input_callback) {
            input_->set_input_callback(config_.input_callback);
        }
    }
    
    // Initialize filesystem (placeholder)
    if (config_.enable_filesystem) {
        // filesystem_ = std::make_unique<WebFileSystem>();
        // if (!filesystem_->initialize()) {
        //     throw std::runtime_error("Failed to initialize filesystem");
        // }
    }
    
    // Initialize networking (placeholder)
    if (config_.enable_networking) {
        // networking_ = std::make_unique<WebNetworking>();
        // if (!networking_->initialize()) {
        //     throw std::runtime_error("Failed to initialize networking");
        // }
    }
}

void WebApplication::shutdown_subsystems() {
    if (networking_) {
        networking_->shutdown();
        networking_.reset();
    }
    
    if (filesystem_) {
        filesystem_->shutdown();
        filesystem_.reset();
    }
    
    if (input_) {
        input_->shutdown();
        input_.reset();
    }
    
    if (audio_) {
        audio_->shutdown();
        audio_.reset();
    }
    
    if (renderer_) {
        renderer_->shutdown();
        renderer_.reset();
    }
}

void WebApplication::update_performance_metrics() {
    auto current_time = std::chrono::steady_clock::now();
    
    // Calculate frame time
    auto frame_duration = std::chrono::duration<double, std::milli>(
        current_time - frame_start_time_);
    performance_metrics_.frame_time_ms = frame_duration.count();
    
    // Calculate FPS
    if (performance_metrics_.frame_time_ms > 0) {
        performance_metrics_.fps = static_cast<std::uint32_t>(1000.0 / performance_metrics_.frame_time_ms);
    }
    
    // Update memory info
    #ifdef __EMSCRIPTEN__
    performance_metrics_.memory.heap_size = EM_ASM_INT({
        return HEAP8.length;
    });
    
    performance_metrics_.memory.heap_used = EM_ASM_INT({
        return Module.getTotalMemory ? Module.getTotalMemory() : 0;
    });
    #endif
    
    frame_start_time_ = current_time;
}

void WebApplication::detect_browser_capabilities() const {
    #ifdef __EMSCRIPTEN__
    // Detect WebGL2 support
    browser_capabilities_.webgl2_support = EM_ASM_INT({
        try {
            var canvas = document.createElement('canvas');
            var gl = canvas.getContext('webgl2');
            return gl !== null;
        } catch (e) {
            return 0;
        }
    }) != 0;
    
    // Detect WebGPU support
    browser_capabilities_.webgpu_support = EM_ASM_INT({
        return typeof navigator.gpu !== 'undefined';
    }) != 0;
    
    // Detect SIMD support
    browser_capabilities_.simd_support = EM_ASM_INT({
        return typeof WebAssembly.SIMD !== 'undefined';
    }) != 0;
    
    // Detect SharedArrayBuffer support
    browser_capabilities_.shared_array_buffer = EM_ASM_INT({
        return typeof SharedArrayBuffer !== 'undefined';
    }) != 0;
    
    // Get user agent
    char* user_agent = (char*)EM_ASM_PTR({
        var ua = navigator.userAgent;
        var lengthBytes = lengthBytesUTF8(ua) + 1;
        var stringOnWasmHeap = _malloc(lengthBytes);
        stringToUTF8(ua, stringOnWasmHeap, lengthBytes);
        return stringOnWasmHeap;
    });
    
    if (user_agent) {
        browser_capabilities_.user_agent = std::string(user_agent);
        free(user_agent);
    }
    
    // Additional capability detection would go here
    #endif
}

void WebApplication::handle_error(const WebError& error) {
    if (error_handler_) {
        error_handler_(error);
    }
}

// Static callbacks

EM_BOOL WebApplication::animation_frame_callback(double time, void* user_data) {
    auto* app = static_cast<WebApplication*>(user_data);
    if (!app || !app->running_) {
        return EM_FALSE;
    }
    
    // Calculate delta time
    auto current_time = std::chrono::steady_clock::now();
    auto delta_time = std::chrono::duration<double>(
        current_time - app->last_update_time_).count();
    
    // Update and render
    app->update(delta_time);
    app->render();
    
    return EM_TRUE;  // Continue the loop
}

EM_BOOL WebApplication::visibility_change_callback(int event_type, 
                                                  const EmscriptenVisibilityChangeEvent* event, 
                                                  void* user_data) {
    auto* app = static_cast<WebApplication*>(user_data);
    if (app) {
        app->set_visibility(!event->hidden);
    }
    return EM_TRUE;
}

EM_BOOL WebApplication::focus_callback(int event_type, 
                                      const EmscriptenFocusEvent* event, 
                                      void* user_data) {
    auto* app = static_cast<WebApplication*>(user_data);
    if (app) {
        app->set_focus(event_type == EMSCRIPTEN_EVENT_FOCUS);
    }
    return EM_TRUE;
}

EM_BOOL WebApplication::resize_callback(int event_type, 
                                       const EmscriptenUiEvent* event, 
                                       void* user_data) {
    auto* app = static_cast<WebApplication*>(user_data);
    if (app && event) {
        // Get canvas size from browser
        int width, height;
        emscripten_get_canvas_element_size(app->config_.canvas.canvas_id.c_str(), &width, &height);
        app->resize(width, height);
    }
    return EM_TRUE;
}

} // namespace ecscope::web