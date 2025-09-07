// =============================================================================
// ECScope WebAssembly Core Implementation
// =============================================================================

#include "wasm_core.hpp"
#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <memory>
#include <chrono>

namespace ecscope::wasm {

// =============================================================================
// WEBASSEMBLY CORE MANAGER
// =============================================================================

class WasmCoreManager {
private:
    static std::unique_ptr<WasmCoreManager> instance_;
    WasmMemoryManager memory_manager_;
    WasmPerformanceMonitor performance_monitor_;
    WasmGraphicsContext graphics_context_;
    bool initialized_ = false;

public:
    static WasmCoreManager& getInstance() {
        if (!instance_) {
            instance_ = std::make_unique<WasmCoreManager>();
        }
        return *instance_;
    }

    bool initialize(const WasmConfig& config) {
        if (initialized_) {
            return true;
        }

        try {
            // Initialize memory management
            if (!memory_manager_.initialize(config.memory_config)) {
                EM_ASM(console.error('Failed to initialize WebAssembly memory manager'));
                return false;
            }

            // Initialize performance monitoring
            if (!performance_monitor_.initialize(config.performance_config)) {
                EM_ASM(console.error('Failed to initialize WebAssembly performance monitor'));
                return false;
            }

            // Initialize graphics context if enabled
            if (config.enable_graphics) {
                if (!graphics_context_.initialize(config.graphics_config)) {
                    EM_ASM(console.error('Failed to initialize WebAssembly graphics context'));
                    return false;
                }
            }

            initialized_ = true;
            EM_ASM(console.log('ECScope WebAssembly core initialized successfully'));
            return true;
        }
        catch (const std::exception& e) {
            EM_ASM({
                console.error('Exception during WebAssembly initialization: ' + UTF8ToString($0));
            }, e.what());
            return false;
        }
    }

    void shutdown() {
        if (!initialized_) {
            return;
        }

        graphics_context_.shutdown();
        performance_monitor_.shutdown();
        memory_manager_.shutdown();
        
        initialized_ = false;
        EM_ASM(console.log('ECScope WebAssembly core shut down'));
    }

    WasmMemoryManager& getMemoryManager() { return memory_manager_; }
    WasmPerformanceMonitor& getPerformanceMonitor() { return performance_monitor_; }
    WasmGraphicsContext& getGraphicsContext() { return graphics_context_; }
    bool isInitialized() const { return initialized_; }
};

std::unique_ptr<WasmCoreManager> WasmCoreManager::instance_ = nullptr;

// =============================================================================
// WEBASSEMBLY MEMORY MANAGER
// =============================================================================

bool WasmMemoryManager::initialize(const WasmMemoryConfig& config) {
    config_ = config;
    
    // Set up memory pools for different allocation patterns
    small_block_pool_.reserve(config.small_block_pool_size);
    medium_block_pool_.reserve(config.medium_block_pool_size);
    large_block_pool_.reserve(config.large_block_pool_size);
    
    // Initialize memory tracking
    total_allocated_ = 0;
    peak_usage_ = 0;
    allocation_count_ = 0;
    
    // Set up JavaScript memory reporting
    EM_ASM({
        window.ECScope = window.ECScope || {};
        window.ECScope.memoryStats = {
            totalAllocated: 0,
            peakUsage: 0,
            allocationCount: 0,
            activeBlocks: 0
        };
    });
    
    initialized_ = true;
    return true;
}

void WasmMemoryManager::shutdown() {
    // Clean up all memory pools
    small_block_pool_.clear();
    medium_block_pool_.clear();
    large_block_pool_.clear();
    
    // Report final memory statistics
    reportMemoryStats();
    initialized_ = false;
}

void* WasmMemoryManager::allocate(size_t size, size_t alignment) {
    if (!initialized_) {
        return nullptr;
    }
    
    void* ptr = nullptr;
    
    // Choose appropriate pool based on size
    if (size <= 64) {
        ptr = allocateFromPool(small_block_pool_, size, alignment);
    } else if (size <= 1024) {
        ptr = allocateFromPool(medium_block_pool_, size, alignment);
    } else {
        ptr = allocateFromPool(large_block_pool_, size, alignment);
    }
    
    if (ptr) {
        total_allocated_ += size;
        peak_usage_ = std::max(peak_usage_, total_allocated_);
        allocation_count_++;
        
        // Update JavaScript memory stats periodically
        if (allocation_count_ % 100 == 0) {
            updateJavaScriptMemoryStats();
        }
    }
    
    return ptr;
}

void WasmMemoryManager::deallocate(void* ptr, size_t size) {
    if (!ptr || !initialized_) {
        return;
    }
    
    // Return to appropriate pool
    if (size <= 64) {
        deallocateToPool(small_block_pool_, ptr, size);
    } else if (size <= 1024) {
        deallocateToPool(medium_block_pool_, ptr, size);
    } else {
        deallocateToPool(large_block_pool_, ptr, size);
    }
    
    total_allocated_ -= size;
}

void WasmMemoryManager::reportMemoryStats() const {
    EM_ASM({
        const stats = {
            totalAllocated: $0,
            peakUsage: $1,
            allocationCount: $2,
            activeBlocks: $3
        };
        console.log('ECScope Memory Statistics:', stats);
        
        if (window.ECScope && window.ECScope.onMemoryReport) {
            window.ECScope.onMemoryReport(stats);
        }
    }, total_allocated_, peak_usage_, allocation_count_, getActiveBlockCount());
}

void WasmMemoryManager::updateJavaScriptMemoryStats() {
    EM_ASM({
        if (window.ECScope && window.ECScope.memoryStats) {
            window.ECScope.memoryStats.totalAllocated = $0;
            window.ECScope.memoryStats.peakUsage = $1;
            window.ECScope.memoryStats.allocationCount = $2;
            window.ECScope.memoryStats.activeBlocks = $3;
        }
    }, total_allocated_, peak_usage_, allocation_count_, getActiveBlockCount());
}

// =============================================================================
// WEBASSEMBLY PERFORMANCE MONITOR
// =============================================================================

bool WasmPerformanceMonitor::initialize(const WasmPerformanceConfig& config) {
    config_ = config;
    
    // Initialize timing structures
    frame_times_.reserve(config.max_frame_samples);
    timing_stack_.reserve(32);  // Support up to 32 nested timings
    
    // Set up performance reporting to JavaScript
    EM_ASM({
        window.ECScope = window.ECScope || {};
        window.ECScope.performanceStats = {
            frameTime: 0,
            fps: 0,
            averageFrameTime: 0,
            worstFrameTime: 0,
            memoryUsage: 0
        };
    });
    
    last_frame_time_ = std::chrono::high_resolution_clock::now();
    initialized_ = true;
    return true;
}

void WasmPerformanceMonitor::shutdown() {
    frame_times_.clear();
    timing_stack_.clear();
    initialized_ = false;
}

void WasmPerformanceMonitor::beginFrame() {
    if (!initialized_) return;
    
    auto current_time = std::chrono::high_resolution_clock::now();
    auto frame_duration = std::chrono::duration_cast<std::chrono::microseconds>(
        current_time - last_frame_time_).count();
    
    frame_times_.push_back(frame_duration);
    if (frame_times_.size() > config_.max_frame_samples) {
        frame_times_.erase(frame_times_.begin());
    }
    
    last_frame_time_ = current_time;
    
    // Update JavaScript stats periodically
    if (frame_times_.size() % 60 == 0) {  // Every 60 frames
        updateJavaScriptPerformanceStats();
    }
}

void WasmPerformanceMonitor::endFrame() {
    // Currently handled in beginFrame for simplicity
}

void WasmPerformanceMonitor::beginTiming(const std::string& name) {
    if (!initialized_) return;
    
    TimingEntry entry;
    entry.name = name;
    entry.start_time = std::chrono::high_resolution_clock::now();
    timing_stack_.push_back(entry);
}

void WasmPerformanceMonitor::endTiming() {
    if (!initialized_ || timing_stack_.empty()) return;
    
    auto end_time = std::chrono::high_resolution_clock::now();
    TimingEntry& entry = timing_stack_.back();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - entry.start_time).count();
    
    // Store timing result
    timing_results_[entry.name].push_back(duration);
    
    // Limit stored results
    auto& results = timing_results_[entry.name];
    if (results.size() > config_.max_timing_samples) {
        results.erase(results.begin());
    }
    
    timing_stack_.pop_back();
}

double WasmPerformanceMonitor::getAverageFrameTime() const {
    if (frame_times_.empty()) return 0.0;
    
    double sum = 0.0;
    for (auto time : frame_times_) {
        sum += time;
    }
    return sum / frame_times_.size() / 1000.0;  // Convert to milliseconds
}

double WasmPerformanceMonitor::getCurrentFPS() const {
    double avg_frame_time = getAverageFrameTime();
    return avg_frame_time > 0.0 ? 1000.0 / avg_frame_time : 0.0;
}

void WasmPerformanceMonitor::updateJavaScriptPerformanceStats() {
    double avg_frame_time = getAverageFrameTime();
    double current_fps = getCurrentFPS();
    double worst_frame_time = frame_times_.empty() ? 0.0 : 
        *std::max_element(frame_times_.begin(), frame_times_.end()) / 1000.0;
    
    EM_ASM({
        if (window.ECScope && window.ECScope.performanceStats) {
            window.ECScope.performanceStats.frameTime = $0;
            window.ECScope.performanceStats.fps = $1;
            window.ECScope.performanceStats.averageFrameTime = $2;
            window.ECScope.performanceStats.worstFrameTime = $3;
        }
    }, avg_frame_time, current_fps, avg_frame_time, worst_frame_time);
}

// =============================================================================
// WEBASSEMBLY GRAPHICS CONTEXT
// =============================================================================

bool WasmGraphicsContext::initialize(const WasmGraphicsConfig& config) {
    config_ = config;
    
    // Initialize WebGL context
    EM_ASM({
        const canvas = document.getElementById($0) || document.createElement('canvas');
        if (!document.getElementById($0)) {
            canvas.id = UTF8ToString($0);
            canvas.width = $1;
            canvas.height = $2;
            document.body.appendChild(canvas);
        }
        
        const gl = canvas.getContext('webgl2') || canvas.getContext('webgl');
        if (!gl) {
            console.error('Failed to initialize WebGL context');
        } else {
            console.log('WebGL context initialized successfully');
            window.ECScope = window.ECScope || {};
            window.ECScope.gl = gl;
            window.ECScope.canvas = canvas;
        }
    }, config.canvas_id.c_str(), config.width, config.height);
    
    initialized_ = true;
    return true;
}

void WasmGraphicsContext::shutdown() {
    if (!initialized_) return;
    
    EM_ASM({
        if (window.ECScope && window.ECScope.gl) {
            // Clean up WebGL resources
            const gl = window.ECScope.gl;
            // Additional cleanup can be added here
            
            delete window.ECScope.gl;
            delete window.ECScope.canvas;
        }
    });
    
    initialized_ = false;
}

void WasmGraphicsContext::beginFrame() {
    if (!initialized_) return;
    
    EM_ASM({
        if (window.ECScope && window.ECScope.gl) {
            const gl = window.ECScope.gl;
            gl.viewport(0, 0, gl.canvas.width, gl.canvas.height);
            gl.clearColor(0.0, 0.0, 0.0, 1.0);
            gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
        }
    });
}

void WasmGraphicsContext::endFrame() {
    if (!initialized_) return;
    
    EM_ASM({
        if (window.ECScope && window.ECScope.gl) {
            const gl = window.ECScope.gl;
            gl.flush();
        }
    });
}

// =============================================================================
// C++ API FUNCTIONS
// =============================================================================

extern "C" {

EMSCRIPTEN_KEEPALIVE
bool ecscope_wasm_initialize(const char* config_json) {
    WasmConfig config;
    // Parse JSON config (simplified for this example)
    config.enable_graphics = true;
    config.memory_config.small_block_pool_size = 1000;
    config.memory_config.medium_block_pool_size = 500;
    config.memory_config.large_block_pool_size = 100;
    config.performance_config.max_frame_samples = 300;
    config.performance_config.max_timing_samples = 100;
    config.graphics_config.canvas_id = "ecscope-canvas";
    config.graphics_config.width = 800;
    config.graphics_config.height = 600;
    
    return WasmCoreManager::getInstance().initialize(config);
}

EMSCRIPTEN_KEEPALIVE
void ecscope_wasm_shutdown() {
    WasmCoreManager::getInstance().shutdown();
}

EMSCRIPTEN_KEEPALIVE
bool ecscope_wasm_is_initialized() {
    return WasmCoreManager::getInstance().isInitialized();
}

EMSCRIPTEN_KEEPALIVE
void ecscope_wasm_begin_frame() {
    auto& core = WasmCoreManager::getInstance();
    core.getPerformanceMonitor().beginFrame();
    core.getGraphicsContext().beginFrame();
}

EMSCRIPTEN_KEEPALIVE
void ecscope_wasm_end_frame() {
    auto& core = WasmCoreManager::getInstance();
    core.getGraphicsContext().endFrame();
    core.getPerformanceMonitor().endFrame();
}

EMSCRIPTEN_KEEPALIVE
void ecscope_wasm_report_memory_stats() {
    WasmCoreManager::getInstance().getMemoryManager().reportMemoryStats();
}

EMSCRIPTEN_KEEPALIVE
double ecscope_wasm_get_fps() {
    return WasmCoreManager::getInstance().getPerformanceMonitor().getCurrentFPS();
}

EMSCRIPTEN_KEEPALIVE
double ecscope_wasm_get_frame_time() {
    return WasmCoreManager::getInstance().getPerformanceMonitor().getAverageFrameTime();
}

}  // extern "C"

}  // namespace ecscope::wasm