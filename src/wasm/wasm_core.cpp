/**
 * @file wasm_core.cpp
 * @brief ECScope WebAssembly Core Implementation - Clean and Secure
 */

#include "wasm_core.hpp"
#include <iostream>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#else
// Fallback for native builds
#define EM_ASM(code) do { } while(0)
#define EM_ASM_(code, ...) do { } while(0)
#define EMSCRIPTEN_KEEPALIVE
#endif

namespace ecscope::wasm {

// Static member definitions
std::unique_ptr<WasmCore> WasmCore::instance_;
std::once_flag WasmCore::init_flag_;

WasmCore& WasmCore::instance() {
    std::call_once(init_flag_, []() {
        instance_ = std::unique_ptr<WasmCore>(new WasmCore());
    });
    return *instance_;
}

WasmCore::~WasmCore() {
    shutdown();
}

bool WasmCore::initialize(const WasmConfig& config) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    if (initialized_.load()) {
        log_info("WasmCore already initialized");
        return true;
    }
    
    if (!config.is_valid()) {
        log_error("Invalid WasmConfig provided");
        return false;
    }
    
    config_ = config;
    
    // Initialize WebGL context if running in browser
    #ifdef __EMSCRIPTEN__
    EM_ASM({
        try {
            const canvas = document.getElementById(UTF8ToString($0));
            if (!canvas) {
                const newCanvas = document.createElement('canvas');
                newCanvas.id = UTF8ToString($0);
                newCanvas.width = 800;
                newCanvas.height = 600;
                document.body.appendChild(newCanvas);
                console.log('Created WebGL canvas:', newCanvas.id);
            }
            
            const gl = canvas.getContext('webgl2') || canvas.getContext('webgl');
            if (gl) {
                window.ECScope = window.ECScope || {};
                window.ECScope.gl = gl;
                window.ECScope.canvas = canvas;
                console.log('WebGL context initialized successfully');
            } else {
                console.error('Failed to initialize WebGL context');
            }
        } catch (e) {
            console.error('WebGL initialization error:', e);
        }
    }, config_.canvas_id.c_str());
    #endif
    
    initialized_.store(true);
    log_info("WasmCore initialized successfully");
    return true;
}

void WasmCore::shutdown() {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    if (!initialized_.load()) {
        return;
    }
    
    #ifdef __EMSCRIPTEN__
    EM_ASM({
        if (window.ECScope) {
            delete window.ECScope.gl;
            delete window.ECScope.canvas;
            delete window.ECScope;
            console.log('WebGL context cleaned up');
        }
    });
    #endif
    
    initialized_.store(false);
    log_info("WasmCore shutdown completed");
}

void WasmCore::begin_frame() {
    if (!initialized_.load()) {
        return;
    }
    
    frame_start_time_ = std::chrono::high_resolution_clock::now();
    
    #ifdef __EMSCRIPTEN__
    EM_ASM({
        if (window.ECScope && window.ECScope.gl) {
            const gl = window.ECScope.gl;
            gl.viewport(0, 0, gl.canvas.width, gl.canvas.height);
            gl.clearColor(0.0, 0.0, 0.0, 1.0);
            gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
        }
    });
    #endif
}

void WasmCore::end_frame() {
    if (!initialized_.load()) {
        return;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - frame_start_time_).count();
    double frame_time_ms = duration / 1000.0;
    
    metrics_.add_frame_time(frame_time_ms);
    
    #ifdef __EMSCRIPTEN__
    EM_ASM({
        if (window.ECScope && window.ECScope.gl) {
            const gl = window.ECScope.gl;
            gl.flush();
        }
    });
    #endif
}

void WasmCore::log_info(const std::string& message) const {
    #ifdef __EMSCRIPTEN__
    EM_ASM({
        console.log('[ECScope WASM] ' + UTF8ToString($0));
    }, message.c_str());
    #else
    std::cout << "[ECScope WASM] " << message << std::endl;
    #endif
}

void WasmCore::log_error(const std::string& message) const {
    #ifdef __EMSCRIPTEN__
    EM_ASM({
        console.error('[ECScope WASM ERROR] ' + UTF8ToString($0));
    }, message.c_str());
    #else
    std::cerr << "[ECScope WASM ERROR] " << message << std::endl;
    #endif
}

} // namespace ecscope::wasm

// C API implementation
extern "C" {

EMSCRIPTEN_KEEPALIVE
bool ecscope_wasm_initialize() {
    try {
        return ecscope::wasm::WasmCore::instance().initialize();
    } catch (const std::exception& e) {
        std::cerr << "WASM initialization error: " << e.what() << std::endl;
        return false;
    }
}

EMSCRIPTEN_KEEPALIVE
void ecscope_wasm_shutdown() {
    try {
        ecscope::wasm::WasmCore::instance().shutdown();
    } catch (const std::exception& e) {
        std::cerr << "WASM shutdown error: " << e.what() << std::endl;
    }
}

EMSCRIPTEN_KEEPALIVE
bool ecscope_wasm_is_initialized() {
    try {
        return ecscope::wasm::WasmCore::instance().is_initialized();
    } catch (const std::exception& e) {
        std::cerr << "WASM status check error: " << e.what() << std::endl;
        return false;
    }
}

EMSCRIPTEN_KEEPALIVE
void ecscope_wasm_begin_frame() {
    try {
        ecscope::wasm::WasmCore::instance().begin_frame();
    } catch (const std::exception& e) {
        std::cerr << "WASM begin frame error: " << e.what() << std::endl;
    }
}

EMSCRIPTEN_KEEPALIVE
void ecscope_wasm_end_frame() {
    try {
        ecscope::wasm::WasmCore::instance().end_frame();
    } catch (const std::exception& e) {
        std::cerr << "WASM end frame error: " << e.what() << std::endl;
    }
}

EMSCRIPTEN_KEEPALIVE
double ecscope_wasm_get_fps() {
    try {
        return ecscope::wasm::WasmCore::instance().metrics().fps();
    } catch (const std::exception& e) {
        std::cerr << "WASM FPS error: " << e.what() << std::endl;
        return 0.0;
    }
}

EMSCRIPTEN_KEEPALIVE
double ecscope_wasm_get_frame_time() {
    try {
        return ecscope::wasm::WasmCore::instance().metrics().average_frame_time();
    } catch (const std::exception& e) {
        std::cerr << "WASM frame time error: " << e.what() << std::endl;
        return 0.0;
    }
}

EMSCRIPTEN_KEEPALIVE
size_t ecscope_wasm_get_memory_usage() {
    try {
        return ecscope::wasm::WasmCore::instance().allocated_memory();
    } catch (const std::exception& e) {
        std::cerr << "WASM memory usage error: " << e.what() << std::endl;
        return 0;
    }
}

} // extern "C"