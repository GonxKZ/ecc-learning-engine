// =============================================================================
// ECScope WebAssembly Core Implementation Header
// =============================================================================

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>

namespace ecscope::wasm {

// =============================================================================
// FORWARD DECLARATIONS
// =============================================================================

class WasmMemoryManager;
class WasmPerformanceMonitor;
class WasmGraphicsContext;

// =============================================================================
// CONFIGURATION STRUCTURES
// =============================================================================

struct WasmMemoryConfig {
    size_t small_block_pool_size = 1000;
    size_t medium_block_pool_size = 500;
    size_t large_block_pool_size = 100;
    size_t max_heap_size = 512 * 1024 * 1024;  // 512MB
    bool enable_tracking = true;
    bool enable_debugging = false;
};

struct WasmPerformanceConfig {
    size_t max_frame_samples = 300;
    size_t max_timing_samples = 100;
    bool enable_profiling = true;
    bool enable_memory_profiling = true;
    double target_fps = 60.0;
};

struct WasmGraphicsConfig {
    std::string canvas_id = "ecscope-canvas";
    int width = 800;
    int height = 600;
    bool enable_vsync = true;
    bool enable_alpha = false;
    bool enable_depth = true;
    bool enable_stencil = false;
    bool enable_antialias = true;
    bool premultiplied_alpha = false;
    bool preserve_drawing_buffer = false;
};

struct WasmConfig {
    WasmMemoryConfig memory_config;
    WasmPerformanceConfig performance_config;
    WasmGraphicsConfig graphics_config;
    bool enable_graphics = true;
    bool enable_physics = true;
    bool enable_job_system = true;
    bool enable_debugging = false;
};

// =============================================================================
// WEBASSEMBLY MEMORY MANAGER
// =============================================================================

class WasmMemoryManager {
private:
    struct MemoryBlock {
        void* ptr;
        size_t size;
        bool in_use;
        
        MemoryBlock(void* p, size_t s) : ptr(p), size(s), in_use(true) {}
    };
    
    WasmMemoryConfig config_;
    std::vector<MemoryBlock> small_block_pool_;
    std::vector<MemoryBlock> medium_block_pool_;
    std::vector<MemoryBlock> large_block_pool_;
    
    size_t total_allocated_ = 0;
    size_t peak_usage_ = 0;
    size_t allocation_count_ = 0;
    bool initialized_ = false;
    
    void* allocateFromPool(std::vector<MemoryBlock>& pool, size_t size, size_t alignment);
    void deallocateToPool(std::vector<MemoryBlock>& pool, void* ptr, size_t size);
    void updateJavaScriptMemoryStats();
    size_t getActiveBlockCount() const;

public:
    bool initialize(const WasmMemoryConfig& config);
    void shutdown();
    
    void* allocate(size_t size, size_t alignment = sizeof(void*));
    void deallocate(void* ptr, size_t size);
    
    size_t getTotalAllocated() const { return total_allocated_; }
    size_t getPeakUsage() const { return peak_usage_; }
    size_t getAllocationCount() const { return allocation_count_; }
    
    void reportMemoryStats() const;
    bool isInitialized() const { return initialized_; }
};

// =============================================================================
// WEBASSEMBLY PERFORMANCE MONITOR
// =============================================================================

class WasmPerformanceMonitor {
private:
    struct TimingEntry {
        std::string name;
        std::chrono::high_resolution_clock::time_point start_time;
    };
    
    WasmPerformanceConfig config_;
    std::vector<int64_t> frame_times_;  // In microseconds
    std::vector<TimingEntry> timing_stack_;
    std::unordered_map<std::string, std::vector<int64_t>> timing_results_;
    
    std::chrono::high_resolution_clock::time_point last_frame_time_;
    bool initialized_ = false;
    
    void updateJavaScriptPerformanceStats();

public:
    bool initialize(const WasmPerformanceConfig& config);
    void shutdown();
    
    void beginFrame();
    void endFrame();
    
    void beginTiming(const std::string& name);
    void endTiming();
    
    double getAverageFrameTime() const;  // In milliseconds
    double getCurrentFPS() const;
    
    const std::vector<int64_t>& getFrameTimes() const { return frame_times_; }
    const std::unordered_map<std::string, std::vector<int64_t>>& getTimingResults() const { 
        return timing_results_; 
    }
    
    bool isInitialized() const { return initialized_; }
};

// =============================================================================
// WEBASSEMBLY GRAPHICS CONTEXT
// =============================================================================

class WasmGraphicsContext {
private:
    WasmGraphicsConfig config_;
    bool initialized_ = false;

public:
    bool initialize(const WasmGraphicsConfig& config);
    void shutdown();
    
    void beginFrame();
    void endFrame();
    
    void resize(int width, int height);
    
    int getWidth() const { return config_.width; }
    int getHeight() const { return config_.height; }
    const std::string& getCanvasId() const { return config_.canvas_id; }
    
    bool isInitialized() const { return initialized_; }
};

// =============================================================================
// WEBASSEMBLY UTILITY FUNCTIONS
// =============================================================================

// Memory allocation wrappers
template<typename T>
T* wasm_allocate(size_t count = 1) {
    auto& manager = WasmCoreManager::getInstance().getMemoryManager();
    return static_cast<T*>(manager.allocate(sizeof(T) * count, alignof(T)));
}

template<typename T>
void wasm_deallocate(T* ptr, size_t count = 1) {
    auto& manager = WasmCoreManager::getInstance().getMemoryManager();
    manager.deallocate(ptr, sizeof(T) * count);
}

// Performance timing utilities
class WasmScopedTimer {
private:
    std::string name_;
    WasmPerformanceMonitor* monitor_;

public:
    WasmScopedTimer(const std::string& name) : name_(name) {
        monitor_ = &WasmCoreManager::getInstance().getPerformanceMonitor();
        monitor_->beginTiming(name_);
    }
    
    ~WasmScopedTimer() {
        monitor_->endTiming();
    }
};

#define WASM_PROFILE(name) WasmScopedTimer timer(name)

// =============================================================================
// C API DECLARATIONS
// =============================================================================

extern "C" {
    bool ecscope_wasm_initialize(const char* config_json);
    void ecscope_wasm_shutdown();
    bool ecscope_wasm_is_initialized();
    void ecscope_wasm_begin_frame();
    void ecscope_wasm_end_frame();
    void ecscope_wasm_report_memory_stats();
    double ecscope_wasm_get_fps();
    double ecscope_wasm_get_frame_time();
}

}  // namespace ecscope::wasm