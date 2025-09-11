#include "../include/web_bindings.hpp"
#include "ecscope/ecs/world.hpp"
#include "ecscope/ecs/entity.hpp"
#include "ecscope/ecs/component.hpp"
#include "ecscope/ecs/system.hpp"
#include "ecscope/physics/world.hpp"
#include "ecscope/memory/pool_allocator.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <emscripten/fetch.h>

using namespace emscripten;

namespace ecscope::web::bindings {

void register_all_bindings() {
    register_application_bindings();
    register_renderer_bindings();
    register_audio_bindings();
    register_input_bindings();
    register_utility_bindings();
    register_ecs_bindings();
    register_physics_bindings();
    register_memory_bindings();
}

void register_application_bindings() {
    // WebApplicationConfig
    value_object<WebApplicationConfig>("WebApplicationConfig")
        .field("title", &WebApplicationConfig::title)
        .field("canvas", &WebApplicationConfig::canvas)
        .field("webgl", &WebApplicationConfig::webgl)
        .field("audio", &WebApplicationConfig::audio)
        .field("enable_input", &WebApplicationConfig::enable_input)
        .field("enable_networking", &WebApplicationConfig::enable_networking)
        .field("enable_filesystem", &WebApplicationConfig::enable_filesystem)
        .field("enable_performance_monitoring", &WebApplicationConfig::enable_performance_monitoring)
        .field("enable_error_reporting", &WebApplicationConfig::enable_error_reporting);
    
    // CanvasInfo
    value_object<CanvasInfo>("CanvasInfo")
        .field("canvas_id", &CanvasInfo::canvas_id)
        .field("width", &CanvasInfo::width)
        .field("height", &CanvasInfo::height)
        .field("has_webgl2", &CanvasInfo::has_webgl2)
        .field("has_webgpu", &CanvasInfo::has_webgpu);
    
    // WebGLConfig
    value_object<WebGLConfig>("WebGLConfig")
        .field("alpha", &WebGLConfig::alpha)
        .field("depth", &WebGLConfig::depth)
        .field("stencil", &WebGLConfig::stencil)
        .field("antialias", &WebGLConfig::antialias)
        .field("premultiplied_alpha", &WebGLConfig::premultiplied_alpha)
        .field("preserve_drawing_buffer", &WebGLConfig::preserve_drawing_buffer)
        .field("power_preference_high_performance", &WebGLConfig::power_preference_high_performance)
        .field("fail_if_major_performance_caveat", &WebGLConfig::fail_if_major_performance_caveat)
        .field("major_version", &WebGLConfig::major_version)
        .field("minor_version", &WebGLConfig::minor_version);
    
    // WebAudioConfig
    value_object<WebAudioConfig>("WebAudioConfig")
        .field("sample_rate", &WebAudioConfig::sample_rate)
        .field("buffer_size", &WebAudioConfig::buffer_size)
        .field("channels", &WebAudioConfig::channels)
        .field("enable_spatial_audio", &WebAudioConfig::enable_spatial_audio)
        .field("enable_effects", &WebAudioConfig::enable_effects);
    
    // PerformanceMetrics
    value_object<PerformanceMetrics>("PerformanceMetrics")
        .field("frame_time_ms", &PerformanceMetrics::frame_time_ms)
        .field("update_time_ms", &PerformanceMetrics::update_time_ms)
        .field("render_time_ms", &PerformanceMetrics::render_time_ms)
        .field("fps", &PerformanceMetrics::fps)
        .field("draw_calls", &PerformanceMetrics::draw_calls)
        .field("triangles", &PerformanceMetrics::triangles)
        .field("memory", &PerformanceMetrics::memory);
    
    // MemoryInfo
    value_object<MemoryInfo>("MemoryInfo")
        .field("heap_size", &MemoryInfo::heap_size)
        .field("heap_used", &MemoryInfo::heap_used)
        .field("heap_limit", &MemoryInfo::heap_limit)
        .field("stack_size", &MemoryInfo::stack_size)
        .field("stack_used", &MemoryInfo::stack_used)
        .field("memory_pressure", &MemoryInfo::memory_pressure);
    
    // BrowserCapabilities
    value_object<BrowserCapabilities>("BrowserCapabilities")
        .field("webgl2_support", &BrowserCapabilities::webgl2_support)
        .field("webgpu_support", &BrowserCapabilities::webgpu_support)
        .field("simd_support", &BrowserCapabilities::simd_support)
        .field("threads_support", &BrowserCapabilities::threads_support)
        .field("shared_array_buffer", &BrowserCapabilities::shared_array_buffer)
        .field("wasm_bulk_memory", &BrowserCapabilities::wasm_bulk_memory)
        .field("file_system_access", &BrowserCapabilities::file_system_access)
        .field("web_audio_worklet", &BrowserCapabilities::web_audio_worklet)
        .field("offscreen_canvas", &BrowserCapabilities::offscreen_canvas)
        .field("user_agent", &BrowserCapabilities::user_agent)
        .field("webgl_renderer", &BrowserCapabilities::webgl_renderer)
        .field("webgl_vendor", &BrowserCapabilities::webgl_vendor);
    
    // WebApplication
    class_<WebApplication>("WebApplication")
        .constructor<const WebApplicationConfig&>()
        .function("initialize", &WebApplication::initialize)
        .function("shutdown", &WebApplication::shutdown)
        .function("update", &WebApplication::update)
        .function("render", &WebApplication::render)
        .function("resize", &WebApplication::resize)
        .function("set_visibility", &WebApplication::set_visibility)
        .function("set_focus", &WebApplication::set_focus)
        .function("get_performance_metrics", &WebApplication::get_performance_metrics)
        .function("get_browser_capabilities", &WebApplication::get_browser_capabilities)
        .function("is_initialized", &WebApplication::is_initialized)
        .function("is_running", &WebApplication::is_running)
        .function("execute_javascript", &WebApplication::execute_javascript)
        .function("get_renderer", &WebApplication::get_renderer, allow_raw_pointers())
        .function("get_audio", &WebApplication::get_audio, allow_raw_pointers())
        .function("get_input", &WebApplication::get_input, allow_raw_pointers());
}

void register_renderer_bindings() {
    // WebRenderer::Backend enum
    enum_<WebRenderer::Backend>("RendererBackend")
        .value("WebGL2", WebRenderer::Backend::WebGL2)
        .value("WebGPU", WebRenderer::Backend::WebGPU)
        .value("Auto", WebRenderer::Backend::Auto);
    
    // WebRenderer::RenderTarget
    value_object<WebRenderer::RenderTarget>("RenderTarget")
        .field("canvas_id", &WebRenderer::RenderTarget::canvas_id)
        .field("width", &WebRenderer::RenderTarget::width)
        .field("height", &WebRenderer::RenderTarget::height)
        .field("device_pixel_ratio", &WebRenderer::RenderTarget::device_pixel_ratio)
        .field("is_offscreen", &WebRenderer::RenderTarget::is_offscreen);
    
    // WebRenderer::RenderStats
    value_object<WebRenderer::RenderStats>("RenderStats")
        .field("draw_calls", &WebRenderer::RenderStats::draw_calls)
        .field("triangles", &WebRenderer::RenderStats::triangles)
        .field("vertices", &WebRenderer::RenderStats::vertices)
        .field("texture_switches", &WebRenderer::RenderStats::texture_switches)
        .field("shader_switches", &WebRenderer::RenderStats::shader_switches)
        .field("frame_time_ms", &WebRenderer::RenderStats::frame_time_ms);
    
    // WebRenderer
    class_<WebRenderer>("WebRenderer")
        .constructor<const WebRenderer::RenderTarget&, WebRenderer::Backend>()
        .function("initialize", &WebRenderer::initialize)
        .function("shutdown", &WebRenderer::shutdown)
        .function("begin_frame", &WebRenderer::begin_frame)
        .function("end_frame", &WebRenderer::end_frame)
        .function("resize", &WebRenderer::resize)
        .function("set_viewport", &WebRenderer::set_viewport)
        .function("clear", &WebRenderer::clear)
        .function("get_backend", &WebRenderer::get_backend)
        .function("get_target", &WebRenderer::get_target)
        .function("is_initialized", &WebRenderer::is_initialized)
        .function("create_shader_program", &WebRenderer::create_shader_program)
        .function("delete_shader_program", &WebRenderer::delete_shader_program)
        .function("create_vertex_buffer", select_overload<std::uint32_t(const void*, std::size_t)>(&WebRenderer::create_vertex_buffer), allow_raw_pointers())
        .function("create_index_buffer", select_overload<std::uint32_t(const void*, std::size_t)>(&WebRenderer::create_index_buffer), allow_raw_pointers())
        .function("update_buffer", &WebRenderer::update_buffer, allow_raw_pointers())
        .function("delete_buffer", &WebRenderer::delete_buffer)
        .function("create_texture", select_overload<std::uint32_t(int, int, unsigned int, unsigned int, const void*)>(&WebRenderer::create_texture), allow_raw_pointers())
        .function("delete_texture", &WebRenderer::delete_texture)
        .function("draw_indexed", &WebRenderer::draw_indexed)
        .function("draw_arrays", &WebRenderer::draw_arrays)
        .function("get_render_stats", &WebRenderer::get_render_stats)
        .function("reset_render_stats", &WebRenderer::reset_render_stats);
}

void register_audio_bindings() {
    // WebAudio::NodeType enum
    enum_<WebAudio::NodeType>("AudioNodeType")
        .value("Source", WebAudio::NodeType::Source)
        .value("Gain", WebAudio::NodeType::Gain)
        .value("Filter", WebAudio::NodeType::Filter)
        .value("Delay", WebAudio::NodeType::Delay)
        .value("Reverb", WebAudio::NodeType::Reverb)
        .value("Compressor", WebAudio::NodeType::Compressor)
        .value("Analyzer", WebAudio::NodeType::Analyzer)
        .value("Panner", WebAudio::NodeType::Panner)
        .value("Destination", WebAudio::NodeType::Destination);
    
    // WebAudio::AudioBuffer
    value_object<WebAudio::AudioBuffer>("AudioBuffer")
        .field("id", &WebAudio::AudioBuffer::id)
        .field("sample_rate", &WebAudio::AudioBuffer::sample_rate)
        .field("channels", &WebAudio::AudioBuffer::channels)
        .field("length", &WebAudio::AudioBuffer::length)
        .field("duration", &WebAudio::AudioBuffer::duration);
    
    // WebAudio::AudioListener
    value_object<WebAudio::AudioListener>("AudioListener")
        .field("x", &WebAudio::AudioListener::x)
        .field("y", &WebAudio::AudioListener::y)
        .field("z", &WebAudio::AudioListener::z)
        .field("forward_x", &WebAudio::AudioListener::forward_x)
        .field("forward_y", &WebAudio::AudioListener::forward_y)
        .field("forward_z", &WebAudio::AudioListener::forward_z)
        .field("up_x", &WebAudio::AudioListener::up_x)
        .field("up_y", &WebAudio::AudioListener::up_y)
        .field("up_z", &WebAudio::AudioListener::up_z);
    
    // WebAudio::EffectParameters
    value_object<WebAudio::EffectParameters>("EffectParameters")
        .field("frequency", &WebAudio::EffectParameters::frequency)
        .field("q", &WebAudio::EffectParameters::q)
        .field("gain", &WebAudio::EffectParameters::gain)
        .field("delay_time", &WebAudio::EffectParameters::delay_time)
        .field("feedback", &WebAudio::EffectParameters::feedback)
        .field("mix", &WebAudio::EffectParameters::mix)
        .field("room_size", &WebAudio::EffectParameters::room_size)
        .field("decay_time", &WebAudio::EffectParameters::decay_time)
        .field("damping", &WebAudio::EffectParameters::damping)
        .field("wet_mix", &WebAudio::EffectParameters::wet_mix);
    
    // WebAudio
    class_<WebAudio>("WebAudio")
        .constructor<const WebAudioConfig&>()
        .function("initialize", &WebAudio::initialize)
        .function("shutdown", &WebAudio::shutdown)
        .function("update", &WebAudio::update)
        .function("resume_context", &WebAudio::resume_context)
        .function("suspend_context", &WebAudio::suspend_context)
        .function("is_context_running", &WebAudio::is_context_running)
        .function("create_audio_buffer", &WebAudio::create_audio_buffer)
        .function("delete_audio_buffer", &WebAudio::delete_audio_buffer)
        .function("create_audio_source", &WebAudio::create_audio_source)
        .function("delete_audio_source", &WebAudio::delete_audio_source)
        .function("play_source", select_overload<void(std::uint32_t)>(&WebAudio::play_source))
        .function("stop_source", select_overload<void(std::uint32_t)>(&WebAudio::stop_source))
        .function("pause_source", &WebAudio::pause_source)
        .function("resume_source", &WebAudio::resume_source)
        .function("set_source_volume", &WebAudio::set_source_volume)
        .function("set_source_pitch", &WebAudio::set_source_pitch)
        .function("set_source_looping", &WebAudio::set_source_looping)
        .function("set_source_position", &WebAudio::set_source_position)
        .function("set_listener", &WebAudio::set_listener)
        .function("set_master_volume", &WebAudio::set_master_volume)
        .function("get_master_volume", &WebAudio::get_master_volume)
        .function("generate_tone", &WebAudio::generate_tone)
        .function("get_current_time", &WebAudio::get_current_time)
        .function("get_sample_rate", &WebAudio::get_sample_rate)
        .function("is_initialized", &WebAudio::is_initialized);
}

void register_input_bindings() {
    // InputEventType enum
    enum_<InputEventType>("InputEventType")
        .value("KeyDown", InputEventType::KeyDown)
        .value("KeyUp", InputEventType::KeyUp)
        .value("MouseDown", InputEventType::MouseDown)
        .value("MouseUp", InputEventType::MouseUp)
        .value("MouseMove", InputEventType::MouseMove)
        .value("MouseWheel", InputEventType::MouseWheel)
        .value("TouchStart", InputEventType::TouchStart)
        .value("TouchMove", InputEventType::TouchMove)
        .value("TouchEnd", InputEventType::TouchEnd)
        .value("GamepadConnected", InputEventType::GamepadConnected)
        .value("GamepadDisconnected", InputEventType::GamepadDisconnected);
    
    // WebInput::MouseButton enum
    enum_<WebInput::MouseButton>("MouseButton")
        .value("Left", WebInput::MouseButton::Left)
        .value("Middle", WebInput::MouseButton::Middle)
        .value("Right", WebInput::MouseButton::Right)
        .value("Back", WebInput::MouseButton::Back)
        .value("Forward", WebInput::MouseButton::Forward);
    
    // WebInput::KeyCode enum (partial - common keys)
    enum_<WebInput::KeyCode>("KeyCode")
        .value("A", WebInput::KeyCode::A)
        .value("D", WebInput::KeyCode::D)
        .value("S", WebInput::KeyCode::S)
        .value("W", WebInput::KeyCode::W)
        .value("Space", WebInput::KeyCode::Space)
        .value("Enter", WebInput::KeyCode::Enter)
        .value("Escape", WebInput::KeyCode::Escape)
        .value("Left", WebInput::KeyCode::Left)
        .value("Right", WebInput::KeyCode::Right)
        .value("Up", WebInput::KeyCode::Up)
        .value("Down", WebInput::KeyCode::Down)
        .value("Shift", WebInput::KeyCode::Shift)
        .value("Ctrl", WebInput::KeyCode::Ctrl)
        .value("Alt", WebInput::KeyCode::Alt);
    
    // InputEvent
    value_object<InputEvent>("InputEvent")
        .field("type", &InputEvent::type)
        .field("timestamp", &InputEvent::timestamp)
        .field("key", &InputEvent::key)
        .field("key_code", &InputEvent::key_code)
        .field("ctrl_key", &InputEvent::ctrl_key)
        .field("alt_key", &InputEvent::alt_key)
        .field("shift_key", &InputEvent::shift_key)
        .field("meta_key", &InputEvent::meta_key)
        .field("mouse_x", &InputEvent::mouse_x)
        .field("mouse_y", &InputEvent::mouse_y)
        .field("delta_x", &InputEvent::delta_x)
        .field("delta_y", &InputEvent::delta_y)
        .field("mouse_button", &InputEvent::mouse_button);
    
    // WebInput::TouchPoint
    value_object<WebInput::TouchPoint>("TouchPoint")
        .field("identifier", &WebInput::TouchPoint::identifier)
        .field("x", &WebInput::TouchPoint::x)
        .field("y", &WebInput::TouchPoint::y)
        .field("radius_x", &WebInput::TouchPoint::radius_x)
        .field("radius_y", &WebInput::TouchPoint::radius_y)
        .field("rotation_angle", &WebInput::TouchPoint::rotation_angle)
        .field("force", &WebInput::TouchPoint::force)
        .field("active", &WebInput::TouchPoint::active);
    
    // WebInput::Gesture::Type enum
    enum_<WebInput::Gesture::Type>("GestureType")
        .value("None", WebInput::Gesture::Type::None)
        .value("Tap", WebInput::Gesture::Type::Tap)
        .value("DoubleTap", WebInput::Gesture::Type::DoubleTap)
        .value("LongPress", WebInput::Gesture::Type::LongPress)
        .value("Swipe", WebInput::Gesture::Type::Swipe)
        .value("Pinch", WebInput::Gesture::Type::Pinch)
        .value("Rotate", WebInput::Gesture::Type::Rotate)
        .value("Pan", WebInput::Gesture::Type::Pan);
    
    // WebInput::Gesture
    value_object<WebInput::Gesture>("Gesture")
        .field("type", &WebInput::Gesture::type)
        .field("x", &WebInput::Gesture::x)
        .field("y", &WebInput::Gesture::y)
        .field("delta_x", &WebInput::Gesture::delta_x)
        .field("delta_y", &WebInput::Gesture::delta_y)
        .field("scale", &WebInput::Gesture::scale)
        .field("rotation", &WebInput::Gesture::rotation)
        .field("velocity_x", &WebInput::Gesture::velocity_x)
        .field("velocity_y", &WebInput::Gesture::velocity_y)
        .field("duration", &WebInput::Gesture::duration);
    
    // WebInput
    class_<WebInput>("WebInput")
        .constructor<const std::string&>()
        .function("initialize", &WebInput::initialize)
        .function("shutdown", &WebInput::shutdown)
        .function("update", &WebInput::update)
        .function("is_key_down", &WebInput::is_key_down)
        .function("is_key_pressed", &WebInput::is_key_pressed)
        .function("is_key_released", &WebInput::is_key_released)
        .function("get_typed_text", &WebInput::get_typed_text)
        .function("is_mouse_button_down", &WebInput::is_mouse_button_down)
        .function("is_mouse_button_pressed", &WebInput::is_mouse_button_pressed)
        .function("is_mouse_button_released", &WebInput::is_mouse_button_released)
        .function("set_cursor_visible", &WebInput::set_cursor_visible)
        .function("lock_pointer", &WebInput::lock_pointer)
        .function("unlock_pointer", &WebInput::unlock_pointer)
        .function("is_pointer_locked", &WebInput::is_pointer_locked)
        .function("get_current_gesture", &WebInput::get_current_gesture)
        .function("set_gesture_recognition", &WebInput::set_gesture_recognition)
        .function("clear_state", &WebInput::clear_state)
        .function("set_focus", &WebInput::set_focus)
        .function("has_focus", &WebInput::has_focus)
        .function("is_initialized", &WebInput::is_initialized);
}

void register_utility_bindings() {
    // JSPromiseWrapper
    class_<JSPromiseWrapper>("JSPromiseWrapper")
        .constructor<>()
        .function("get_promise", &JSPromiseWrapper::get_promise)
        .function("resolve", select_overload<void()>(&JSPromiseWrapper::resolve))
        .function("reject", &JSPromiseWrapper::reject);
    
    // CallbackManager
    class_<CallbackManager>("CallbackManager")
        .constructor<>()
        .function("register_callback", &CallbackManager::register_callback)
        .function("unregister_callback", &CallbackManager::unregister_callback)
        .function("call_callback", select_overload<void(CallbackManager::CallbackId)>(&CallbackManager::call_callback))
        .function("call_all_callbacks", select_overload<void()>(&CallbackManager::call_all_callbacks))
        .function("clear_callbacks", &CallbackManager::clear_callbacks);
    
    // AsyncLoader
    class_<AsyncLoader>("AsyncLoader")
        .class_function("load_binary", &AsyncLoader::load_binary)
        .class_function("load_text", &AsyncLoader::load_text)
        .class_function("load_json", &AsyncLoader::load_json)
        .class_function("load_image", &AsyncLoader::load_image)
        .class_function("load_audio", &AsyncLoader::load_audio);
    
    // PerformanceProfiler
    class_<PerformanceProfiler>("PerformanceProfiler")
        .class_function("start_profiling", &PerformanceProfiler::start_profiling)
        .class_function("stop_profiling", &PerformanceProfiler::stop_profiling)
        .class_function("mark_event", &PerformanceProfiler::mark_event)
        .class_function("begin_measure", &PerformanceProfiler::begin_measure)
        .class_function("end_measure", &PerformanceProfiler::end_measure)
        .class_function("get_results", &PerformanceProfiler::get_results);
}

void register_ecs_bindings() {
    // Note: These would be bindings for the ECS system
    // For brevity, including basic bindings - full implementation would be much larger
    
    // Entity
    value_object<ecscope::ecs::Entity>("Entity")
        .field("id", &ecscope::ecs::Entity::id)
        .field("version", &ecscope::ecs::Entity::version);
    
    // Basic component registration would go here
    // This would need to be extended based on actual component types
}

void register_physics_bindings() {
    // Note: Physics bindings would be similar to ECS bindings
    // Including placeholder for physics world and basic types
}

void register_memory_bindings() {
    // Memory management utility bindings
    // Would include pool allocator and memory tracking utilities
}

// Helper function implementations

template<typename T>
val vector_to_js_array(const std::vector<T>& vec) {
    val array = val::array();
    for (size_t i = 0; i < vec.size(); ++i) {
        array.call<void>("push", vec[i]);
    }
    return array;
}

template<typename T>
std::vector<T> js_array_to_vector(const val& array) {
    std::vector<T> vec;
    size_t length = array["length"].as<size_t>();
    vec.reserve(length);
    
    for (size_t i = 0; i < length; ++i) {
        vec.push_back(array[i].as<T>());
    }
    return vec;
}

val performance_metrics_to_js(const PerformanceMetrics& metrics) {
    val obj = val::object();
    obj.set("frameTimeMs", metrics.frame_time_ms);
    obj.set("updateTimeMs", metrics.update_time_ms);
    obj.set("renderTimeMs", metrics.render_time_ms);
    obj.set("fps", metrics.fps);
    obj.set("drawCalls", metrics.draw_calls);
    obj.set("triangles", metrics.triangles);
    
    val memory = val::object();
    memory.set("heapSize", metrics.memory.heap_size);
    memory.set("heapUsed", metrics.memory.heap_used);
    memory.set("heapLimit", metrics.memory.heap_limit);
    memory.set("stackSize", metrics.memory.stack_size);
    memory.set("stackUsed", metrics.memory.stack_used);
    memory.set("memoryPressure", metrics.memory.memory_pressure);
    obj.set("memory", memory);
    
    return obj;
}

val browser_capabilities_to_js(const BrowserCapabilities& caps) {
    val obj = val::object();
    obj.set("webgl2Support", caps.webgl2_support);
    obj.set("webgpuSupport", caps.webgpu_support);
    obj.set("simdSupport", caps.simd_support);
    obj.set("threadsSupport", caps.threads_support);
    obj.set("sharedArrayBuffer", caps.shared_array_buffer);
    obj.set("wasmBulkMemory", caps.wasm_bulk_memory);
    obj.set("fileSystemAccess", caps.file_system_access);
    obj.set("webAudioWorklet", caps.web_audio_worklet);
    obj.set("offscreenCanvas", caps.offscreen_canvas);
    obj.set("userAgent", caps.user_agent);
    obj.set("webglRenderer", caps.webgl_renderer);
    obj.set("webglVendor", caps.webgl_vendor);
    return obj;
}

} // namespace ecscope::web::bindings

// Register bindings on module load
EMSCRIPTEN_BINDINGS(ecscope_web) {
    ecscope::web::bindings::register_all_bindings();
}

#endif // __EMSCRIPTEN__