#pragma once

#include "ecscope/web/web_types.hpp"
#include "web_application.hpp"
#include "web_renderer.hpp"
#include "web_audio.hpp"
#include "web_input.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
#include <emscripten/val.h>
#endif

namespace ecscope::web::bindings {

/**
 * @brief Register all WebAssembly bindings for ECScope engine
 * 
 * This function registers all C++ classes and functions with Embind,
 * making them accessible from JavaScript.
 */
void register_all_bindings();

/**
 * @brief Register core application bindings
 */
void register_application_bindings();

/**
 * @brief Register renderer bindings
 */
void register_renderer_bindings();

/**
 * @brief Register audio system bindings
 */
void register_audio_bindings();

/**
 * @brief Register input system bindings
 */
void register_input_bindings();

/**
 * @brief Register utility and helper bindings
 */
void register_utility_bindings();

/**
 * @brief Register ECS core bindings
 */
void register_ecs_bindings();

/**
 * @brief Register physics system bindings
 */
void register_physics_bindings();

/**
 * @brief Register memory management bindings
 */
void register_memory_bindings();

// JavaScript interop helpers

/**
 * @brief Convert C++ vector to JavaScript array
 */
template<typename T>
emscripten::val vector_to_js_array(const std::vector<T>& vec);

/**
 * @brief Convert JavaScript array to C++ vector
 */
template<typename T>
std::vector<T> js_array_to_vector(const emscripten::val& array);

/**
 * @brief Convert C++ map to JavaScript object
 */
template<typename K, typename V>
emscripten::val map_to_js_object(const std::unordered_map<K, V>& map);

/**
 * @brief Convert JavaScript object to C++ map
 */
template<typename K, typename V>
std::unordered_map<K, V> js_object_to_map(const emscripten::val& object);

/**
 * @brief Promise wrapper for async operations
 */
class JSPromiseWrapper {
public:
    explicit JSPromiseWrapper();
    ~JSPromiseWrapper();
    
    /**
     * @brief Get JavaScript promise
     */
    emscripten::val get_promise() const;
    
    /**
     * @brief Resolve the promise
     */
    void resolve(const emscripten::val& value = emscripten::val::undefined());
    
    /**
     * @brief Reject the promise
     */
    void reject(const emscripten::val& reason);
    
private:
    emscripten::val promise_;
    emscripten::val resolve_func_;
    emscripten::val reject_func_;
};

/**
 * @brief Callback manager for JavaScript callbacks
 */
class CallbackManager {
public:
    using CallbackId = std::uint32_t;
    
    /**
     * @brief Register a callback
     */
    CallbackId register_callback(const emscripten::val& callback);
    
    /**
     * @brief Unregister a callback
     */
    void unregister_callback(CallbackId id);
    
    /**
     * @brief Call a registered callback
     */
    void call_callback(CallbackId id, const emscripten::val& args = emscripten::val::undefined());
    
    /**
     * @brief Call all registered callbacks
     */
    void call_all_callbacks(const emscripten::val& args = emscripten::val::undefined());
    
    /**
     * @brief Clear all callbacks
     */
    void clear_callbacks();
    
private:
    CallbackId next_id_ = 1;
    std::unordered_map<CallbackId, emscripten::val> callbacks_;
};

/**
 * @brief Error handler for WebAssembly exceptions
 */
class ErrorHandler {
public:
    /**
     * @brief Set global error handler
     */
    static void set_global_handler(const emscripten::val& handler);
    
    /**
     * @brief Handle C++ exception and convert to JavaScript error
     */
    static void handle_exception(const std::exception& e);
    
    /**
     * @brief Create JavaScript error object
     */
    static emscripten::val create_js_error(const std::string& message, 
                                          const std::string& type = "Error");
    
private:
    static emscripten::val global_handler_;
};

// Type conversion utilities

/**
 * @brief Convert PerformanceMetrics to JavaScript object
 */
emscripten::val performance_metrics_to_js(const PerformanceMetrics& metrics);

/**
 * @brief Convert BrowserCapabilities to JavaScript object
 */
emscripten::val browser_capabilities_to_js(const BrowserCapabilities& caps);

/**
 * @brief Convert InputEvent to JavaScript object
 */
emscripten::val input_event_to_js(const InputEvent& event);

/**
 * @brief Convert WebError to JavaScript error
 */
emscripten::val web_error_to_js(const WebError& error);

/**
 * @brief Convert JavaScript object to WebApplicationConfig
 */
WebApplicationConfig js_to_web_application_config(const emscripten::val& config);

/**
 * @brief Convert JavaScript object to WebGLConfig
 */
WebGLConfig js_to_webgl_config(const emscripten::val& config);

/**
 * @brief Convert JavaScript object to WebAudioConfig
 */
WebAudioConfig js_to_web_audio_config(const emscripten::val& config);

// Memory management helpers

/**
 * @brief Typed array wrapper for efficient data transfer
 */
template<typename T>
class TypedArrayWrapper {
public:
    explicit TypedArrayWrapper(const std::vector<T>& data);
    explicit TypedArrayWrapper(T* data, std::size_t size);
    ~TypedArrayWrapper();
    
    /**
     * @brief Get JavaScript typed array
     */
    emscripten::val get_typed_array() const;
    
    /**
     * @brief Get data pointer
     */
    T* get_data() const { return data_; }
    
    /**
     * @brief Get size
     */
    std::size_t get_size() const { return size_; }
    
private:
    T* data_;
    std::size_t size_;
    bool owns_data_;
};

/**
 * @brief Shared buffer for zero-copy data transfer
 */
class SharedBuffer {
public:
    explicit SharedBuffer(std::size_t size);
    ~SharedBuffer();
    
    /**
     * @brief Get buffer as typed array
     */
    template<typename T>
    emscripten::val get_typed_array() const;
    
    /**
     * @brief Get raw data pointer
     */
    void* get_data() const { return data_; }
    
    /**
     * @brief Get buffer size
     */
    std::size_t get_size() const { return size_; }
    
    /**
     * @brief Resize buffer
     */
    void resize(std::size_t new_size);
    
private:
    void* data_;
    std::size_t size_;
};

// Async operation helpers

/**
 * @brief Async loader for resources
 */
class AsyncLoader {
public:
    /**
     * @brief Load URL as binary data
     */
    static emscripten::val load_binary(const std::string& url);
    
    /**
     * @brief Load URL as text
     */
    static emscripten::val load_text(const std::string& url);
    
    /**
     * @brief Load URL as JSON
     */
    static emscripten::val load_json(const std::string& url);
    
    /**
     * @brief Load image from URL
     */
    static emscripten::val load_image(const std::string& url);
    
    /**
     * @brief Load audio from URL
     */
    static emscripten::val load_audio(const std::string& url);
    
private:
    static void setup_fetch_handlers();
};

// Performance monitoring

/**
 * @brief Performance profiler for WebAssembly
 */
class PerformanceProfiler {
public:
    /**
     * @brief Start profiling session
     */
    static void start_profiling();
    
    /**
     * @brief Stop profiling session
     */
    static void stop_profiling();
    
    /**
     * @brief Mark performance event
     */
    static void mark_event(const std::string& name);
    
    /**
     * @brief Begin performance measure
     */
    static void begin_measure(const std::string& name);
    
    /**
     * @brief End performance measure
     */
    static void end_measure(const std::string& name);
    
    /**
     * @brief Get profiling results
     */
    static emscripten::val get_results();
    
private:
    static bool profiling_active_;
    static std::chrono::steady_clock::time_point session_start_;
    static std::unordered_map<std::string, std::chrono::steady_clock::time_point> active_measures_;
};

} // namespace ecscope::web::bindings

// Template implementations
#include "web_bindings_impl.hpp"