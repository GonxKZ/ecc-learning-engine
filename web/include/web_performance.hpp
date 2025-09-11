#pragma once

#include "ecscope/web/web_types.hpp"
#include <chrono>
#include <vector>
#include <unordered_map>
#include <memory>

namespace ecscope::web {

/**
 * @brief WebAssembly performance optimization and monitoring system
 * 
 * This class provides comprehensive performance optimization features
 * including SIMD utilization, threading coordination, and real-time
 * performance monitoring for WebAssembly applications.
 */
class WebPerformance {
public:
    /**
     * @brief Performance optimization levels
     */
    enum class OptimizationLevel {
        None,        // No optimizations
        Basic,       // Basic optimizations
        Aggressive,  // Aggressive optimizations
        Ultra        // Maximum optimizations (may impact compatibility)
    };
    
    /**
     * @brief SIMD instruction set support
     */
    enum class SIMDSupport {
        None,
        SIMD128,     // WebAssembly SIMD 128-bit
        AVX,         // AVX support (future)
        AVX2,        // AVX2 support (future)
        AVX512       // AVX-512 support (future)
    };
    
    /**
     * @brief Performance measurement categories
     */
    enum class MeasurementCategory {
        FrameTime,
        UpdateTime,
        RenderTime,
        AudioTime,
        InputTime,
        NetworkTime,
        FileIOTime,
        MemoryOps,
        CustomCategory
    };
    
    /**
     * @brief Performance measurement data
     */
    struct Measurement {
        MeasurementCategory category;
        std::string name;
        std::chrono::steady_clock::time_point start_time;
        std::chrono::steady_clock::time_point end_time;
        double duration_ms;
        std::uint64_t call_count;
        std::unordered_map<std::string, double> metadata;
    };
    
    /**
     * @brief Performance profile data
     */
    struct ProfileData {
        std::string name;
        std::vector<Measurement> measurements;
        double total_time_ms;
        double average_time_ms;
        double min_time_ms;
        double max_time_ms;
        std::uint64_t total_calls;
        double overhead_ms;
    };
    
    /**
     * @brief System performance metrics
     */
    struct SystemMetrics {
        // CPU metrics
        double cpu_usage_percent;
        std::uint32_t core_count;
        std::uint32_t thread_count;
        double instruction_throughput;
        
        // Memory metrics
        std::size_t memory_used_bytes;
        std::size_t memory_peak_bytes;
        std::size_t memory_allocated_bytes;
        double memory_pressure;
        std::size_t gc_collections;
        
        // Graphics metrics
        std::uint32_t fps;
        double frame_time_ms;
        std::uint32_t draw_calls;
        std::uint32_t triangles_rendered;
        std::uint32_t texture_switches;
        std::uint32_t shader_switches;
        
        // Browser metrics
        bool vsync_enabled;
        double display_refresh_rate;
        std::string gpu_vendor;
        std::string gpu_renderer;
        std::string browser_engine;
        
        // WebAssembly metrics
        std::size_t wasm_module_size;
        double wasm_compile_time_ms;
        double wasm_instantiate_time_ms;
        bool simd_enabled;
        bool threads_enabled;
        bool bulk_memory_enabled;
    };
    
    /**
     * @brief Thread pool configuration
     */
    struct ThreadConfig {
        std::uint32_t worker_count;
        std::uint32_t stack_size;
        bool shared_memory;
        bool enable_atomic_wait;
        std::uint32_t queue_size;
    };
    
    /**
     * @brief Construct WebPerformance system
     * @param optimization_level Default optimization level
     */
    explicit WebPerformance(OptimizationLevel optimization_level = OptimizationLevel::Basic);
    
    /**
     * @brief Destructor
     */
    ~WebPerformance();
    
    // Non-copyable, movable
    WebPerformance(const WebPerformance&) = delete;
    WebPerformance& operator=(const WebPerformance&) = delete;
    WebPerformance(WebPerformance&&) noexcept = default;
    WebPerformance& operator=(WebPerformance&&) noexcept = default;
    
    /**
     * @brief Initialize performance system
     * @return true if initialization succeeded
     */
    bool initialize();
    
    /**
     * @brief Shutdown performance system
     */
    void shutdown();
    
    // Optimization control
    
    /**
     * @brief Set optimization level
     * @param level Optimization level
     */
    void set_optimization_level(OptimizationLevel level);
    
    /**
     * @brief Get current optimization level
     * @return Current optimization level
     */
    OptimizationLevel get_optimization_level() const noexcept { return optimization_level_; }
    
    /**
     * @brief Detect SIMD support
     * @return Supported SIMD instruction set
     */
    SIMDSupport detect_simd_support() const;
    
    /**
     * @brief Enable/disable SIMD optimizations
     * @param enable Whether to enable SIMD
     */
    void set_simd_enabled(bool enable);
    
    /**
     * @brief Check if SIMD is enabled
     * @return true if SIMD is enabled
     */
    bool is_simd_enabled() const noexcept { return simd_enabled_; }
    
    /**
     * @brief Configure thread pool
     * @param config Thread pool configuration
     * @return true if configuration succeeded
     */
    bool configure_thread_pool(const ThreadConfig& config);
    
    /**
     * @brief Get thread pool configuration
     * @return Current thread pool configuration
     */
    const ThreadConfig& get_thread_config() const noexcept { return thread_config_; }
    
    // Performance measurement
    
    /**
     * @brief Start performance measurement
     * @param name Measurement name
     * @param category Measurement category
     * @return Measurement ID
     */
    std::uint64_t start_measurement(const std::string& name, 
                                   MeasurementCategory category = MeasurementCategory::CustomCategory);
    
    /**
     * @brief End performance measurement
     * @param measurement_id Measurement ID
     */
    void end_measurement(std::uint64_t measurement_id);
    
    /**
     * @brief Add measurement metadata
     * @param measurement_id Measurement ID
     * @param key Metadata key
     * @param value Metadata value
     */
    void add_measurement_metadata(std::uint64_t measurement_id, 
                                 const std::string& key, double value);
    
    /**
     * @brief Get measurement results
     * @param name Measurement name
     * @return Profile data for measurement
     */
    ProfileData get_measurement_results(const std::string& name) const;
    
    /**
     * @brief Get all measurement results
     * @return Map of all profile data
     */
    std::unordered_map<std::string, ProfileData> get_all_measurement_results() const;
    
    /**
     * @brief Clear measurement data
     * @param name Measurement name (empty = clear all)
     */
    void clear_measurements(const std::string& name = "");
    
    // System monitoring
    
    /**
     * @brief Update system metrics
     */
    void update_system_metrics();
    
    /**
     * @brief Get current system metrics
     * @return Current system metrics
     */
    const SystemMetrics& get_system_metrics() const noexcept { return system_metrics_; }
    
    /**
     * @brief Set metrics update interval
     * @param interval_ms Update interval in milliseconds
     */
    void set_metrics_update_interval(std::uint32_t interval_ms);
    
    /**
     * @brief Enable/disable automatic metrics collection
     * @param enable Whether to enable automatic collection
     */
    void set_auto_metrics_collection(bool enable);
    
    // Performance profiling
    
    /**
     * @brief Start performance profiling session
     * @param name Profile session name
     */
    void start_profiling_session(const std::string& name);
    
    /**
     * @brief Stop performance profiling session
     * @param name Profile session name
     */
    void stop_profiling_session(const std::string& name);
    
    /**
     * @brief Export profiling data
     * @param format Export format ("json", "csv", "chrome_trace")
     * @return Exported data as string
     */
    std::string export_profiling_data(const std::string& format = "json") const;
    
    /**
     * @brief Save profiling data to IndexedDB
     * @param key Storage key
     * @return JavaScript promise
     */
    JSValue save_profiling_data(const std::string& key) const;
    
    /**
     * @brief Load profiling data from IndexedDB
     * @param key Storage key
     * @return JavaScript promise
     */
    JSValue load_profiling_data(const std::string& key);
    
    // Optimization suggestions
    
    /**
     * @brief Analyze performance and get optimization suggestions
     * @return Vector of optimization suggestions
     */
    std::vector<std::string> get_optimization_suggestions() const;
    
    /**
     * @brief Get performance bottlenecks
     * @param threshold_ms Minimum time threshold for bottlenecks
     * @return Vector of bottleneck descriptions
     */
    std::vector<std::string> get_performance_bottlenecks(double threshold_ms = 1.0) const;
    
    /**
     * @brief Get performance score (0-100)
     * @return Performance score
     */
    double get_performance_score() const;
    
    // Utility functions
    
    /**
     * @brief Get high-resolution timestamp
     * @return Timestamp in microseconds
     */
    static std::uint64_t get_timestamp_us();
    
    /**
     * @brief Get browser performance API data
     * @return JavaScript performance object
     */
    static JSValue get_browser_performance();
    
    /**
     * @brief Force garbage collection
     * @return true if GC was triggered
     */
    static bool force_garbage_collection();
    
    /**
     * @brief Yield to browser event loop
     */
    static void yield_to_browser();
    
    /**
     * @brief Request idle callback
     * @param callback Function to call during idle time
     * @param timeout_ms Maximum time to wait for idle
     */
    static void request_idle_callback(std::function<void()> callback, 
                                     std::uint32_t timeout_ms = 5000);
    
    /**
     * @brief Check if performance system is initialized
     * @return true if initialized
     */
    bool is_initialized() const noexcept { return initialized_; }
    
private:
    // Configuration
    OptimizationLevel optimization_level_;
    ThreadConfig thread_config_;
    
    // State
    bool initialized_ = false;
    bool simd_enabled_ = false;
    bool profiling_active_ = false;
    bool auto_metrics_collection_ = true;
    std::uint32_t metrics_update_interval_ms_ = 1000;
    
    // Measurements
    std::uint64_t next_measurement_id_ = 1;
    std::unordered_map<std::uint64_t, Measurement> active_measurements_;
    std::unordered_map<std::string, std::vector<Measurement>> completed_measurements_;
    mutable std::mutex measurements_mutex_;
    
    // System metrics
    SystemMetrics system_metrics_;
    std::chrono::steady_clock::time_point last_metrics_update_;
    
    // Profiling sessions
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> profiling_sessions_;
    
    // Thread pool (placeholder for future implementation)
    std::unique_ptr<class WebThreadPool> thread_pool_;
    
    // Internal methods
    void apply_optimization_settings();
    void initialize_simd_support();
    void initialize_thread_pool();
    void update_browser_metrics();
    void update_webassembly_metrics();
    void calculate_performance_score();
    ProfileData calculate_profile_data(const std::vector<Measurement>& measurements) const;
    
    // Browser integration
    void setup_browser_performance_observers();
    void cleanup_browser_performance_observers();
    
    // Static callbacks for browser integration
    static void performance_observer_callback(const JSValue& entries, void* user_data);
    static void idle_callback_wrapper(const JSValue& deadline, void* user_data);
};

/**
 * @brief RAII performance measurement scope
 */
class PerformanceScope {
public:
    /**
     * @brief Create performance scope
     * @param perf Performance system reference
     * @param name Measurement name
     * @param category Measurement category
     */
    PerformanceScope(WebPerformance& perf, const std::string& name,
                    WebPerformance::MeasurementCategory category = WebPerformance::MeasurementCategory::CustomCategory);
    
    /**
     * @brief Destructor - automatically ends measurement
     */
    ~PerformanceScope();
    
    // Non-copyable, non-movable
    PerformanceScope(const PerformanceScope&) = delete;
    PerformanceScope& operator=(const PerformanceScope&) = delete;
    PerformanceScope(PerformanceScope&&) = delete;
    PerformanceScope& operator=(PerformanceScope&&) = delete;
    
    /**
     * @brief Add metadata to this measurement
     * @param key Metadata key
     * @param value Metadata value
     */
    void add_metadata(const std::string& key, double value);
    
private:
    WebPerformance& performance_;
    std::uint64_t measurement_id_;
};

/**
 * @brief Macro for easy performance measurement
 */
#define ECSCOPE_PERF_SCOPE(perf, name) \
    ecscope::web::PerformanceScope _perf_scope(perf, name)

#define ECSCOPE_PERF_SCOPE_CATEGORY(perf, name, category) \
    ecscope::web::PerformanceScope _perf_scope(perf, name, category)

} // namespace ecscope::web