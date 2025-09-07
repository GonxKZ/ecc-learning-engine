// =============================================================================
// ECScope WebAssembly Main Application
// =============================================================================

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/html5.h>
#include <iostream>
#include <memory>
#include <chrono>

// ECScope includes
#include "wasm_core.hpp"
#include "bindings/ecs_bindings.cpp"
#include "bindings/physics_bindings.cpp"

using namespace ecscope::wasm;
using namespace emscripten;

// =============================================================================
// GLOBAL STATE
// =============================================================================

namespace {
    std::unique_ptr<WasmCoreManager> g_core_manager = nullptr;
    std::chrono::high_resolution_clock::time_point g_last_frame_time;
    bool g_is_running = false;
    int g_frame_count = 0;
}

// =============================================================================
// WEBASSEMBLY MAIN LOOP
// =============================================================================

void main_loop() {
    if (!g_is_running || !g_core_manager) {
        return;
    }
    
    auto current_time = std::chrono::high_resolution_clock::now();
    auto delta_time = std::chrono::duration<float>(current_time - g_last_frame_time).count();
    g_last_frame_time = current_time;
    g_frame_count++;
    
    // Begin frame
    auto& core = WasmCoreManager::getInstance();
    core.getPerformanceMonitor().beginFrame();
    core.getGraphicsContext().beginFrame();
    
    // Update systems would go here
    // For now, just basic frame timing
    
    // End frame
    core.getGraphicsContext().endFrame();
    core.getPerformanceMonitor().endFrame();
    
    // Report performance stats periodically
    if (g_frame_count % 300 == 0) {  // Every 5 seconds at 60 FPS
        core.getMemoryManager().reportMemoryStats();
        core.getPerformanceMonitor().updateJavaScriptPerformanceStats();
    }
}

// =============================================================================
// WEB API FUNCTIONS
// =============================================================================

extern "C" {

EMSCRIPTEN_KEEPALIVE
bool web_initialize() {
    try {
        WasmConfig config;
        config.enable_graphics = true;
        config.memory_config.small_block_pool_size = 1000;
        config.memory_config.medium_block_pool_size = 500;
        config.memory_config.large_block_pool_size = 100;
        config.performance_config.max_frame_samples = 300;
        config.performance_config.max_timing_samples = 100;
        config.graphics_config.canvas_id = "ecscope-canvas";
        config.graphics_config.width = 800;
        config.graphics_config.height = 600;
        
        bool success = WasmCoreManager::getInstance().initialize(config);
        
        if (success) {
            g_last_frame_time = std::chrono::high_resolution_clock::now();
            EM_ASM({
                console.log('ECScope WebAssembly core initialized successfully');
            });
        }
        
        return success;
    }
    catch (const std::exception& e) {
        EM_ASM({
            console.error('ECScope initialization failed: ' + UTF8ToString($0));
        }, e.what());
        return false;
    }
}

EMSCRIPTEN_KEEPALIVE
void web_start_main_loop() {
    if (!WasmCoreManager::getInstance().isInitialized()) {
        return;
    }
    
    g_is_running = true;
    g_frame_count = 0;
    g_last_frame_time = std::chrono::high_resolution_clock::now();
    
    // Set up the main loop
    emscripten_set_main_loop(main_loop, 0, 1);  // 0 = use requestAnimationFrame, 1 = simulate infinite loop
    
    EM_ASM({
        console.log('ECScope main loop started');
    });
}

EMSCRIPTEN_KEEPALIVE
void web_stop_main_loop() {
    g_is_running = false;
    emscripten_cancel_main_loop();
    
    EM_ASM({
        console.log('ECScope main loop stopped');
    });
}

EMSCRIPTEN_KEEPALIVE
bool web_is_running() {
    return g_is_running;
}

EMSCRIPTEN_KEEPALIVE
void web_shutdown() {
    web_stop_main_loop();
    WasmCoreManager::getInstance().shutdown();
    
    EM_ASM({
        console.log('ECScope WebAssembly shutdown complete');
    });
}

EMSCRIPTEN_KEEPALIVE
int web_get_frame_count() {
    return g_frame_count;
}

EMSCRIPTEN_KEEPALIVE
double web_get_current_fps() {
    if (!WasmCoreManager::getInstance().isInitialized()) {
        return 0.0;
    }
    
    return WasmCoreManager::getInstance().getPerformanceMonitor().getCurrentFPS();
}

EMSCRIPTEN_KEEPALIVE
double web_get_average_frame_time() {
    if (!WasmCoreManager::getInstance().isInitialized()) {
        return 0.0;
    }
    
    return WasmCoreManager::getInstance().getPerformanceMonitor().getAverageFrameTime();
}

EMSCRIPTEN_KEEPALIVE
void web_report_memory_stats() {
    if (!WasmCoreManager::getInstance().isInitialized()) {
        return;
    }
    
    WasmCoreManager::getInstance().getMemoryManager().reportMemoryStats();
}

}  // extern "C"

// =============================================================================
// CANVAS AND INPUT HANDLING
// =============================================================================

EM_BOOL canvas_click_callback(int eventType, const EmscriptenMouseEvent* e, void* userData) {
    if (!g_is_running) {
        return EM_FALSE;
    }
    
    EM_ASM({
        console.log('Canvas clicked at: ' + $0 + ', ' + $1);
    }, e->targetX, e->targetY);
    
    return EM_TRUE;
}

EM_BOOL keyboard_callback(int eventType, const EmscriptenKeyboardEvent* e, void* userData) {
    if (!g_is_running) {
        return EM_FALSE;
    }
    
    // Handle keyboard input for demos
    if (eventType == EMSCRIPTEN_EVENT_KEYDOWN) {
        switch (e->keyCode) {
            case 32: // Space - pause/resume
                EM_ASM({
                    console.log('Space key pressed');
                });
                break;
            case 82: // R - reset
                EM_ASM({
                    console.log('Reset key pressed');
                });
                break;
        }
    }
    
    return EM_TRUE;
}

// =============================================================================
// MODULE INITIALIZATION
// =============================================================================

int main() {
    EM_ASM({
        console.log('ECScope WebAssembly module loading...');
        
        // Set up global ECScope object
        window.ECScope = window.ECScope || {};
        window.ECScope.moduleReady = true;
        
        // Notify JavaScript that the module is ready
        if (window.ECScope.onModuleReady) {
            window.ECScope.onModuleReady();
        }
    });
    
    // Set up input callbacks
    emscripten_set_click_callback("ecscope-canvas", nullptr, EM_FALSE, canvas_click_callback);
    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_FALSE, keyboard_callback);
    emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_FALSE, keyboard_callback);
    
    EM_ASM({
        console.log('ECScope WebAssembly module initialized');
    });
    
    return 0;
}

// =============================================================================
// ADDITIONAL EMBIND BINDINGS
// =============================================================================

EMSCRIPTEN_BINDINGS(ecscope_web_main) {
    // Web-specific functions
    function("web_initialize", &web_initialize);
    function("web_start_main_loop", &web_start_main_loop);
    function("web_stop_main_loop", &web_stop_main_loop);
    function("web_is_running", &web_is_running);
    function("web_shutdown", &web_shutdown);
    function("web_get_frame_count", &web_get_frame_count);
    function("web_get_current_fps", &web_get_current_fps);
    function("web_get_average_frame_time", &web_get_average_frame_time);
    function("web_report_memory_stats", &web_report_memory_stats);
}