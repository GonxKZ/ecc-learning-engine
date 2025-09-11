#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <emscripten/fetch.h>
#include <emscripten/threading.h>
#endif

namespace ecscope::web {

// Forward declarations
class WebApplication;
class WebRenderer;
class WebAudio;
class WebInput;
class WebFileSystem;
class WebNetworking;

// Type aliases for WebAssembly integration
using JSValue = emscripten::val;
using JSFunction = std::function<void(const JSValue&)>;
using JSPromise = emscripten::val;

// Canvas and rendering types
struct CanvasInfo {
    std::string canvas_id;
    int width;
    int height;
    bool has_webgl2;
    bool has_webgpu;
};

// WebGL context configuration
struct WebGLConfig {
    bool alpha = true;
    bool depth = true;
    bool stencil = false;
    bool antialias = true;
    bool premultiplied_alpha = true;
    bool preserve_drawing_buffer = false;
    bool power_preference_high_performance = true;
    bool fail_if_major_performance_caveat = false;
    int major_version = 2;
    int minor_version = 0;
};

// Audio context configuration
struct WebAudioConfig {
    float sample_rate = 44100.0f;
    std::uint32_t buffer_size = 1024;
    std::uint32_t channels = 2;
    bool enable_spatial_audio = true;
    bool enable_effects = true;
};

// Input event types
enum class InputEventType {
    KeyDown,
    KeyUp,
    MouseDown,
    MouseUp,
    MouseMove,
    MouseWheel,
    TouchStart,
    TouchMove,
    TouchEnd,
    GamepadConnected,
    GamepadDisconnected
};

// Input event data
struct InputEvent {
    InputEventType type;
    std::uint32_t timestamp;
    
    // Key events
    std::string key;
    std::uint32_t key_code;
    bool ctrl_key;
    bool alt_key;
    bool shift_key;
    bool meta_key;
    
    // Mouse events
    float mouse_x;
    float mouse_y;
    float delta_x;
    float delta_y;
    std::uint32_t mouse_button;
    
    // Touch events
    std::vector<std::pair<float, float>> touch_points;
    
    // Gamepad events
    std::uint32_t gamepad_index;
};

// File system types
enum class FileAccessMode {
    Read,
    Write,
    ReadWrite
};

struct FileHandle {
    std::string name;
    std::string type;
    std::size_t size;
    void* data;
    bool is_directory;
};

// Network types
enum class NetworkProtocol {
    HTTP,
    HTTPS,
    WebSocket,
    WebSocketSecure
};

struct NetworkRequest {
    std::string url;
    std::string method;
    std::unordered_map<std::string, std::string> headers;
    std::vector<std::uint8_t> body;
    std::uint32_t timeout_ms;
};

struct NetworkResponse {
    std::uint32_t status_code;
    std::string status_text;
    std::unordered_map<std::string, std::string> headers;
    std::vector<std::uint8_t> data;
    bool success;
};

// Memory management types
struct MemoryInfo {
    std::size_t heap_size;
    std::size_t heap_used;
    std::size_t heap_limit;
    std::size_t stack_size;
    std::size_t stack_used;
    float memory_pressure;
};

// Performance monitoring
struct PerformanceMetrics {
    double frame_time_ms;
    double update_time_ms;
    double render_time_ms;
    std::uint32_t fps;
    std::uint32_t draw_calls;
    std::uint32_t triangles;
    MemoryInfo memory;
};

// Browser feature detection
struct BrowserCapabilities {
    bool webgl2_support;
    bool webgpu_support;
    bool simd_support;
    bool threads_support;
    bool shared_array_buffer;
    bool wasm_bulk_memory;
    bool file_system_access;
    bool web_audio_worklet;
    bool offscreen_canvas;
    std::string user_agent;
    std::string webgl_renderer;
    std::string webgl_vendor;
};

// Error handling
enum class WebErrorType {
    None,
    WebGLContextLost,
    AudioContextSuspended,
    NetworkError,
    FileSystemError,
    MemoryError,
    SecurityError,
    NotSupportedError
};

struct WebError {
    WebErrorType type;
    std::string message;
    std::string stack_trace;
    std::uint32_t error_code;
};

// Event callbacks
using ErrorCallback = std::function<void(const WebError&)>;
using InputCallback = std::function<void(const InputEvent&)>;
using NetworkCallback = std::function<void(const NetworkResponse&)>;
using FileCallback = std::function<void(const FileHandle&)>;
using PerformanceCallback = std::function<void(const PerformanceMetrics&)>;

// Configuration for web application
struct WebApplicationConfig {
    std::string title = "ECScope WebAssembly Application";
    CanvasInfo canvas;
    WebGLConfig webgl;
    WebAudioConfig audio;
    bool enable_input = true;
    bool enable_networking = true;
    bool enable_filesystem = true;
    bool enable_performance_monitoring = true;
    bool enable_error_reporting = true;
    
    // Callbacks
    ErrorCallback error_callback;
    InputCallback input_callback;
    PerformanceCallback performance_callback;
};

} // namespace ecscope::web