#pragma once

#include "performance_profiler.hpp"
#include "memory_optimization.hpp"
#include "cpu_gpu_optimization.hpp"
#include "caching_system.hpp"
#include "platform_optimization.hpp"
#include "../core/types.hpp"
#include <memory>
#include <atomic>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <thread>

namespace ecscope::gui::performance {

// Performance optimization levels
enum class OptimizationLevel {
    OFF,           // No optimization
    CONSERVATIVE,  // Safe optimizations only
    BALANCED,      // Balance between performance and quality
    AGGRESSIVE,    // Maximum performance
    ADAPTIVE       // Dynamically adjust based on metrics
};

// Performance targets
struct PerformanceTargets {
    float target_fps = 60.0f;
    float min_fps = 30.0f;
    float target_frame_time_ms = 16.67f;
    float max_frame_time_ms = 33.33f;
    size_t max_memory_mb = 512;
    float max_cpu_usage_percent = 50.0f;
    float max_gpu_usage_percent = 80.0f;
    size_t max_draw_calls = 1000;
    float min_cache_hit_rate = 0.85f;
};

// Optimization strategies
struct OptimizationStrategies {
    // Rendering optimizations
    bool enable_batching = true;
    bool enable_instancing = true;
    bool enable_occlusion_culling = true;
    bool enable_lod = true;
    bool enable_texture_streaming = true;
    bool enable_gpu_driven_rendering = false;
    
    // Memory optimizations
    bool enable_object_pooling = true;
    bool enable_lazy_loading = true;
    bool enable_compression = true;
    bool enable_memory_compaction = true;
    bool enable_aggressive_caching = true;
    
    // CPU optimizations
    bool enable_simd = true;
    bool enable_multithreading = true;
    bool enable_job_system = true;
    bool enable_command_buffering = true;
    
    // Platform-specific
    bool enable_platform_specific = true;
    bool enable_hardware_acceleration = true;
};

// Real-time performance analyzer
class PerformanceAnalyzer {
public:
    struct Analysis {
        // Bottleneck detection
        enum class Bottleneck {
            NONE,
            CPU_BOUND,
            GPU_BOUND,
            MEMORY_BOUND,
            IO_BOUND,
            VSYNC_LIMITED
        } primary_bottleneck = Bottleneck::NONE;
        
        // Performance issues
        std::vector<std::string> issues;
        std::vector<std::string> warnings;
        
        // Optimization suggestions
        std::vector<std::string> suggestions;
        
        // Metrics summary
        float avg_fps = 0.0f;
        float percentile_95_frame_time = 0.0f;
        float frame_time_variance = 0.0f;
        size_t frame_drops = 0;
        size_t stutters = 0;
    };
    
    PerformanceAnalyzer();
    
    void Analyze(const PerformanceMetrics& metrics);
    Analysis GetAnalysis() const;
    
    void SetSensitivity(float sensitivity) { m_sensitivity = sensitivity; }
    void EnableAutoTuning(bool enable) { m_auto_tuning = enable; }
    
private:
    void DetectBottlenecks(const PerformanceMetrics& metrics);
    void GenerateSuggestions();
    
    Analysis m_current_analysis;
    std::deque<PerformanceMetrics> m_history;
    float m_sensitivity = 1.0f;
    bool m_auto_tuning = false;
    
    mutable std::mutex m_mutex;
};

// Automatic performance optimizer
class AutoOptimizer {
public:
    AutoOptimizer(PerformanceTargets targets = {});
    
    void Update(const PerformanceMetrics& metrics);
    void ApplyOptimizations();
    
    void SetTargets(const PerformanceTargets& targets) { m_targets = targets; }
    void SetStrategies(const OptimizationStrategies& strategies) { m_strategies = strategies; }
    void SetOptimizationLevel(OptimizationLevel level) { m_optimization_level = level; }
    
    OptimizationLevel GetCurrentLevel() const { return m_current_level; }
    float GetQualityScale() const { return m_quality_scale; }
    
    // Dynamic quality adjustment
    void EnableDynamicResolution(bool enable) { m_dynamic_resolution = enable; }
    void EnableDynamicLOD(bool enable) { m_dynamic_lod = enable; }
    void EnableDynamicShadowQuality(bool enable) { m_dynamic_shadows = enable; }
    
private:
    void AdjustRenderingQuality(const PerformanceMetrics& metrics);
    void AdjustMemoryUsage(const PerformanceMetrics& metrics);
    void AdjustCPUUsage(const PerformanceMetrics& metrics);
    
    PerformanceTargets m_targets;
    OptimizationStrategies m_strategies;
    OptimizationLevel m_optimization_level = OptimizationLevel::BALANCED;
    OptimizationLevel m_current_level = OptimizationLevel::BALANCED;
    
    // Dynamic adjustments
    std::atomic<float> m_quality_scale{1.0f};
    std::atomic<float> m_resolution_scale{1.0f};
    std::atomic<int> m_lod_bias{0};
    std::atomic<int> m_shadow_quality{2};
    
    bool m_dynamic_resolution = true;
    bool m_dynamic_lod = true;
    bool m_dynamic_shadows = true;
    
    // Smoothing
    float m_target_quality = 1.0f;
    float m_quality_change_rate = 0.1f;
    
    std::chrono::steady_clock::time_point m_last_adjustment;
};

// Performance benchmark system
class BenchmarkSystem {
public:
    struct BenchmarkConfig {
        std::string name;
        std::function<void()> setup;
        std::function<void()> benchmark;
        std::function<void()> cleanup;
        size_t iterations = 100;
        size_t warmup_iterations = 10;
        std::chrono::milliseconds duration = std::chrono::milliseconds(5000);
    };
    
    struct BenchmarkResult {
        std::string name;
        float avg_time_ms = 0.0f;
        float min_time_ms = 0.0f;
        float max_time_ms = 0.0f;
        float std_dev_ms = 0.0f;
        float percentile_50_ms = 0.0f;
        float percentile_95_ms = 0.0f;
        float percentile_99_ms = 0.0f;
        size_t iterations = 0;
        size_t operations_per_second = 0;
    };
    
    BenchmarkSystem();
    
    void RegisterBenchmark(const BenchmarkConfig& config);
    BenchmarkResult RunBenchmark(const std::string& name);
    std::vector<BenchmarkResult> RunAllBenchmarks();
    
    void RunContinuousBenchmark(const std::string& name, 
                                std::function<void(const BenchmarkResult&)> callback);
    void StopContinuousBenchmark();
    
    // Predefined benchmarks
    BenchmarkResult BenchmarkUIRendering();
    BenchmarkResult BenchmarkTextRendering();
    BenchmarkResult BenchmarkLayoutCalculation();
    BenchmarkResult BenchmarkEventProcessing();
    BenchmarkResult BenchmarkMemoryAllocation();
    BenchmarkResult BenchmarkCachePerformance();
    
    void ExportResults(const std::string& filepath);
    void CompareResults(const std::vector<BenchmarkResult>& baseline,
                       const std::vector<BenchmarkResult>& current);
    
private:
    std::unordered_map<std::string, BenchmarkConfig> m_benchmarks;
    std::vector<BenchmarkResult> m_results;
    
    std::thread m_continuous_thread;
    std::atomic<bool> m_stop_continuous{false};
    
    mutable std::mutex m_mutex;
};

// Performance monitoring dashboard
class PerformanceMonitor {
public:
    struct MonitorConfig {
        bool show_fps = true;
        bool show_frame_time = true;
        bool show_cpu_usage = true;
        bool show_gpu_usage = true;
        bool show_memory_usage = true;
        bool show_draw_calls = true;
        bool show_cache_stats = true;
        bool show_warnings = true;
        bool show_graph = true;
        float update_interval_ms = 100.0f;
    };
    
    PerformanceMonitor(MonitorConfig config = {});
    
    void Update(float delta_time);
    void Render();
    
    void SetVisible(bool visible) { m_visible = visible; }
    bool IsVisible() const { return m_visible; }
    
    void SetPosition(float x, float y) { m_position = {x, y}; }
    void SetSize(float width, float height) { m_size = {width, height}; }
    
    // Alert system
    void SetAlertThreshold(const std::string& metric, float threshold);
    void EnableAlerts(bool enable) { m_alerts_enabled = enable; }
    
    // Recording
    void StartRecording(const std::string& filepath);
    void StopRecording();
    bool IsRecording() const { return m_recording; }
    
private:
    void UpdateMetrics();
    void RenderOverlay();
    void RenderGraph();
    void CheckAlerts();
    
    MonitorConfig m_config;
    bool m_visible = true;
    std::pair<float, float> m_position = {10, 10};
    std::pair<float, float> m_size = {400, 300};
    
    // Metrics history
    static constexpr size_t HISTORY_SIZE = 300;
    std::deque<PerformanceMetrics> m_metrics_history;
    
    // Alerts
    bool m_alerts_enabled = true;
    std::unordered_map<std::string, float> m_alert_thresholds;
    std::vector<std::string> m_active_alerts;
    
    // Recording
    bool m_recording = false;
    std::string m_record_filepath;
    std::chrono::steady_clock::time_point m_record_start;
    
    mutable std::mutex m_mutex;
};

// Performance regression detector
class RegressionDetector {
public:
    struct Regression {
        std::string metric_name;
        float baseline_value;
        float current_value;
        float change_percent;
        float confidence;
        std::chrono::steady_clock::time_point detected_at;
    };
    
    RegressionDetector();
    
    void SetBaseline(const PerformanceMetrics& metrics);
    void LoadBaseline(const std::string& filepath);
    void SaveBaseline(const std::string& filepath);
    
    void CheckForRegressions(const PerformanceMetrics& metrics);
    std::vector<Regression> GetRegressions() const;
    
    void SetThreshold(float percent) { m_threshold_percent = percent; }
    void SetConfidenceLevel(float confidence) { m_confidence_level = confidence; }
    
    // Automated testing
    void RunRegressionTest(std::function<void()> test_func);
    bool HasRegressions() const { return !m_regressions.empty(); }
    
private:
    PerformanceMetrics m_baseline;
    std::vector<Regression> m_regressions;
    
    float m_threshold_percent = 5.0f;
    float m_confidence_level = 0.95f;
    
    // Statistical analysis
    std::deque<PerformanceMetrics> m_samples;
    static constexpr size_t MIN_SAMPLES = 30;
    
    mutable std::mutex m_mutex;
};

// Main performance optimization manager
class PerformanceOptimizationManager {
public:
    static PerformanceOptimizationManager& Instance();
    
    void Initialize();
    void Shutdown();
    
    void Update(float delta_time);
    
    // Profiling
    ProfilerSession& GetProfiler() { return *m_profiler; }
    
    // Analysis
    PerformanceAnalyzer& GetAnalyzer() { return *m_analyzer; }
    
    // Optimization
    AutoOptimizer& GetAutoOptimizer() { return *m_auto_optimizer; }
    void SetOptimizationLevel(OptimizationLevel level);
    
    // Benchmarking
    BenchmarkSystem& GetBenchmarkSystem() { return *m_benchmark_system; }
    
    // Monitoring
    PerformanceMonitor& GetMonitor() { return *m_monitor; }
    
    // Regression detection
    RegressionDetector& GetRegressionDetector() { return *m_regression_detector; }
    
    // Memory optimization
    memory::MemoryOptimizer& GetMemoryOptimizer() { return memory::MemoryOptimizer::Instance(); }
    
    // CPU/GPU optimization
    optimization::BatchRenderer& GetBatchRenderer() { return *m_batch_renderer; }
    optimization::CommandBuffer& GetCommandBuffer() { return *m_command_buffer; }
    optimization::TextureStreamer& GetTextureStreamer() { return *m_texture_streamer; }
    
    // Platform optimization
    platform::PlatformOptimizer& GetPlatformOptimizer() { return *m_platform_optimizer; }
    
    // Global settings
    void EnableProfiling(bool enable);
    void EnableAutoOptimization(bool enable);
    void EnableMonitoring(bool enable);
    
    // Performance queries
    float GetCurrentFPS() const;
    float GetAverageFrameTime() const;
    size_t GetMemoryUsage() const;
    float GetCPUUsage() const;
    float GetGPUUsage() const;
    
private:
    PerformanceOptimizationManager();
    ~PerformanceOptimizationManager();
    
    // Core systems
    std::unique_ptr<ProfilerSession> m_profiler;
    std::unique_ptr<PerformanceAnalyzer> m_analyzer;
    std::unique_ptr<AutoOptimizer> m_auto_optimizer;
    std::unique_ptr<BenchmarkSystem> m_benchmark_system;
    std::unique_ptr<PerformanceMonitor> m_monitor;
    std::unique_ptr<RegressionDetector> m_regression_detector;
    
    // Optimization systems
    std::unique_ptr<optimization::BatchRenderer> m_batch_renderer;
    std::unique_ptr<optimization::CommandBuffer> m_command_buffer;
    std::unique_ptr<optimization::TextureStreamer> m_texture_streamer;
    std::unique_ptr<platform::PlatformOptimizer> m_platform_optimizer;
    
    // State
    std::atomic<bool> m_profiling_enabled{true};
    std::atomic<bool> m_auto_optimization_enabled{true};
    std::atomic<bool> m_monitoring_enabled{true};
    
    // Metrics
    std::atomic<float> m_current_fps{0.0f};
    std::atomic<float> m_avg_frame_time{0.0f};
    
    mutable std::mutex m_mutex;
};

// Convenience macros
#define PERF_OPTIMIZE() ecscope::gui::performance::PerformanceOptimizationManager::Instance()
#define PERF_PROFILE_SCOPE(name) PROFILE_SCOPE(name)
#define PERF_BENCHMARK(name, code) \
    PERF_OPTIMIZE().GetBenchmarkSystem().RegisterBenchmark({ \
        name, nullptr, [](){ code; }, nullptr \
    })

} // namespace ecscope::gui::performance