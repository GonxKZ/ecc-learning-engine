/**
 * @file wasm_core.hpp
 * @brief ECScope WebAssembly Core - Clean and Secure Implementation
 * 
 * Provides essential WebAssembly integration for ECScope with:
 * - Safe memory management with proper RAII
 * - Thread-safe singleton pattern
 * - Minimal API surface for security
 * - Clear error handling
 * 
 * @author ECScope Educational ECS Framework
 * @date 2024
 */

#pragma once

#include <memory>
#include <string>
#include <atomic>
#include <mutex>
#include <chrono>

namespace ecscope::wasm {

/**
 * @brief Configuration for WebAssembly runtime
 */
struct WasmConfig {
    size_t initial_memory_mb = 64;    // 64MB initial memory
    size_t max_memory_mb = 256;       // 256MB maximum memory
    bool enable_debugging = false;    // Debug mode flag
    std::string canvas_id = "ecscope-canvas";
    
    // Validation
    bool is_valid() const {
        return initial_memory_mb <= max_memory_mb && 
               max_memory_mb <= 1024 && // Reasonable 1GB limit
               !canvas_id.empty();
    }
};

/**
 * @brief Simple performance metrics for WebAssembly
 */
struct WasmPerformanceMetrics {
    std::atomic<uint64_t> frame_count{0};
    std::atomic<double> total_frame_time{0.0};
    std::atomic<size_t> memory_allocated{0};
    
    double average_frame_time() const {
        uint64_t frames = frame_count.load();
        return frames > 0 ? total_frame_time.load() / frames : 0.0;
    }
    
    double fps() const {
        double avg_time = average_frame_time();
        return avg_time > 0.0 ? 1000.0 / avg_time : 0.0;
    }
    
    void add_frame_time(double time_ms) {
        frame_count.fetch_add(1);
        total_frame_time.fetch_add(time_ms);
    }
};

/**
 * @brief Core WebAssembly manager with proper RAII and thread safety
 */
class WasmCore {
public:
    /**
     * @brief Get singleton instance (thread-safe)
     */
    static WasmCore& instance();
    
    /**
     * @brief Initialize WebAssembly runtime
     * @param config Configuration settings
     * @return true if successful, false otherwise
     */
    bool initialize(const WasmConfig& config = WasmConfig{});
    
    /**
     * @brief Shutdown WebAssembly runtime
     */
    void shutdown();
    
    /**
     * @brief Check if runtime is initialized
     */
    bool is_initialized() const { return initialized_.load(); }
    
    /**
     * @brief Begin frame processing
     */
    void begin_frame();
    
    /**
     * @brief End frame processing
     */
    void end_frame();
    
    /**
     * @brief Get current configuration
     */
    const WasmConfig& config() const { return config_; }
    
    /**
     * @brief Get performance metrics
     */
    const WasmPerformanceMetrics& metrics() const { return metrics_; }
    
    /**
     * @brief Get allocated memory in bytes
     */
    size_t allocated_memory() const { return metrics_.memory_allocated.load(); }

    // Delete copy/move constructors for singleton
    WasmCore(const WasmCore&) = delete;
    WasmCore& operator=(const WasmCore&) = delete;
    WasmCore(WasmCore&&) = delete;
    WasmCore& operator=(WasmCore&&) = delete;

private:
    WasmCore() = default;
    ~WasmCore();
    
    static std::unique_ptr<WasmCore> instance_;
    static std::once_flag init_flag_;
    
    WasmConfig config_;
    std::atomic<bool> initialized_{false};
    WasmPerformanceMetrics metrics_;
    std::chrono::high_resolution_clock::time_point frame_start_time_;
    
    // Thread safety
    mutable std::mutex config_mutex_;
    
    void log_info(const std::string& message) const;
    void log_error(const std::string& message) const;
};

/**
 * @brief RAII frame timer for automatic performance tracking
 */
class WasmFrameTimer {
public:
    WasmFrameTimer() : start_time_(std::chrono::high_resolution_clock::now()) {}
    
    ~WasmFrameTimer() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - start_time_).count();
        double ms = duration / 1000.0;
        WasmCore::instance().metrics().add_frame_time(ms);
    }

private:
    std::chrono::high_resolution_clock::time_point start_time_;
};

/**
 * @brief Safe memory allocation wrapper
 */
template<typename T>
class WasmAllocator {
public:
    using value_type = T;
    
    T* allocate(std::size_t n) {
        if (n > std::size_t(-1) / sizeof(T)) {
            throw std::bad_alloc{};
        }
        
        auto& core = WasmCore::instance();
        if (!core.is_initialized()) {
            throw std::runtime_error("WasmCore not initialized");
        }
        
        size_t bytes = n * sizeof(T);
        size_t current_memory = core.allocated_memory();
        size_t max_memory = core.config().max_memory_mb * 1024 * 1024;
        
        if (current_memory + bytes > max_memory) {
            throw std::bad_alloc{};
        }
        
        T* ptr = static_cast<T*>(std::malloc(bytes));
        if (!ptr) {
            throw std::bad_alloc{};
        }
        
        core.metrics().memory_allocated.fetch_add(bytes);
        return ptr;
    }
    
    void deallocate(T* p, std::size_t n) noexcept {
        if (p) {
            size_t bytes = n * sizeof(T);
            WasmCore::instance().metrics().memory_allocated.fetch_sub(bytes);
            std::free(p);
        }
    }
};

} // namespace ecscope::wasm

// C API for JavaScript integration
extern "C" {
    bool ecscope_wasm_initialize();
    void ecscope_wasm_shutdown();
    bool ecscope_wasm_is_initialized();
    void ecscope_wasm_begin_frame();
    void ecscope_wasm_end_frame();
    double ecscope_wasm_get_fps();
    double ecscope_wasm_get_frame_time();
    size_t ecscope_wasm_get_memory_usage();
}