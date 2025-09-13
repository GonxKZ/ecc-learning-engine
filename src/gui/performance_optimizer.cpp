#include "../../include/ecscope/gui/performance_optimizer.hpp"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <fstream>
#include <sstream>

namespace ecscope::gui::performance {

// PerformanceAnalyzer Implementation
PerformanceAnalyzer::PerformanceAnalyzer() {
    m_history.reserve(60); // 1 second of history at 60 FPS
}

void PerformanceAnalyzer::Analyze(const PerformanceMetrics& metrics) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_history.push_back(metrics);
    if (m_history.size() > 60) {
        m_history.pop_front();
    }
    
    if (m_history.size() < 10) return; // Need enough samples
    
    DetectBottlenecks(metrics);
    GenerateSuggestions();
    
    // Calculate statistics
    float total_fps = 0;
    std::vector<float> frame_times;
    for (const auto& m : m_history) {
        total_fps += m.fps;
        frame_times.push_back(m.frame_time_ms);
    }
    
    m_current_analysis.avg_fps = total_fps / m_history.size();
    
    // Calculate 95th percentile
    std::sort(frame_times.begin(), frame_times.end());
    size_t idx_95 = static_cast<size_t>(frame_times.size() * 0.95f);
    m_current_analysis.percentile_95_frame_time = frame_times[idx_95];
    
    // Calculate variance
    float mean = std::accumulate(frame_times.begin(), frame_times.end(), 0.0f) / frame_times.size();
    float variance = 0;
    for (float ft : frame_times) {
        variance += (ft - mean) * (ft - mean);
    }
    m_current_analysis.frame_time_variance = std::sqrt(variance / frame_times.size());
    
    // Count frame drops and stutters
    m_current_analysis.frame_drops = 0;
    m_current_analysis.stutters = 0;
    for (size_t i = 1; i < m_history.size(); ++i) {
        if (m_history[i].frame_time_ms > 33.33f) {
            m_current_analysis.frame_drops++;
        }
        if (m_history[i].frame_time_ms > m_history[i-1].frame_time_ms * 2.0f) {
            m_current_analysis.stutters++;
        }
    }
}

void PerformanceAnalyzer::DetectBottlenecks(const PerformanceMetrics& metrics) {
    m_current_analysis.issues.clear();
    m_current_analysis.warnings.clear();
    
    // Detect primary bottleneck
    if (metrics.gpu_time_ms > metrics.frame_time_ms * 0.9f) {
        m_current_analysis.primary_bottleneck = Analysis::Bottleneck::GPU_BOUND;
        m_current_analysis.issues.push_back("GPU is the primary bottleneck");
    } else if (metrics.cpu_usage_percent > 90.0f) {
        m_current_analysis.primary_bottleneck = Analysis::Bottleneck::CPU_BOUND;
        m_current_analysis.issues.push_back("CPU is the primary bottleneck");
    } else if (metrics.memory_allocated_bytes > metrics.memory_reserved_bytes * 0.95f) {
        m_current_analysis.primary_bottleneck = Analysis::Bottleneck::MEMORY_BOUND;
        m_current_analysis.issues.push_back("Memory pressure detected");
    } else if (std::abs(metrics.frame_time_ms - 16.67f) < 0.5f || 
               std::abs(metrics.frame_time_ms - 33.33f) < 0.5f) {
        m_current_analysis.primary_bottleneck = Analysis::Bottleneck::VSYNC_LIMITED;
    } else {
        m_current_analysis.primary_bottleneck = Analysis::Bottleneck::NONE;
    }
    
    // Detect specific issues
    if (metrics.draw_calls > 2000) {
        m_current_analysis.issues.push_back("Excessive draw calls: " + std::to_string(metrics.draw_calls));
    }
    
    if (metrics.cache_hit_rate < 0.7f) {
        m_current_analysis.warnings.push_back("Low cache hit rate: " + 
                                             std::to_string(static_cast<int>(metrics.cache_hit_rate * 100)) + "%");
    }
    
    if (metrics.memory_fragmentation > 0.3f) {
        m_current_analysis.warnings.push_back("High memory fragmentation: " + 
                                             std::to_string(static_cast<int>(metrics.memory_fragmentation * 100)) + "%");
    }
    
    if (metrics.shader_switches > 100) {
        m_current_analysis.warnings.push_back("Too many shader switches: " + std::to_string(metrics.shader_switches));
    }
}

void PerformanceAnalyzer::GenerateSuggestions() {
    m_current_analysis.suggestions.clear();
    
    switch (m_current_analysis.primary_bottleneck) {
        case Analysis::Bottleneck::GPU_BOUND:
            m_current_analysis.suggestions.push_back("Enable occlusion culling");
            m_current_analysis.suggestions.push_back("Reduce shadow quality");
            m_current_analysis.suggestions.push_back("Lower texture resolution");
            m_current_analysis.suggestions.push_back("Simplify shaders");
            break;
            
        case Analysis::Bottleneck::CPU_BOUND:
            m_current_analysis.suggestions.push_back("Enable multi-threading");
            m_current_analysis.suggestions.push_back("Use object pooling");
            m_current_analysis.suggestions.push_back("Optimize update loops");
            m_current_analysis.suggestions.push_back("Reduce script complexity");
            break;
            
        case Analysis::Bottleneck::MEMORY_BOUND:
            m_current_analysis.suggestions.push_back("Enable texture streaming");
            m_current_analysis.suggestions.push_back("Reduce texture sizes");
            m_current_analysis.suggestions.push_back("Enable memory compaction");
            m_current_analysis.suggestions.push_back("Clear unused caches");
            break;
            
        default:
            break;
    }
    
    // Specific suggestions based on metrics
    if (!m_current_analysis.issues.empty()) {
        for (const auto& issue : m_current_analysis.issues) {
            if (issue.find("draw calls") != std::string::npos) {
                m_current_analysis.suggestions.push_back("Enable batching and instancing");
            }
        }
    }
}

PerformanceAnalyzer::Analysis PerformanceAnalyzer::GetAnalysis() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_current_analysis;
}

// AutoOptimizer Implementation
AutoOptimizer::AutoOptimizer(PerformanceTargets targets) 
    : m_targets(targets) {
    m_last_adjustment = std::chrono::steady_clock::now();
}

void AutoOptimizer::Update(const PerformanceMetrics& metrics) {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_adjustment);
    
    // Only adjust every 500ms to avoid oscillation
    if (elapsed.count() < 500) return;
    
    if (m_optimization_level == OptimizationLevel::OFF) return;
    
    AdjustRenderingQuality(metrics);
    AdjustMemoryUsage(metrics);
    AdjustCPUUsage(metrics);
    
    m_last_adjustment = now;
}

void AutoOptimizer::AdjustRenderingQuality(const PerformanceMetrics& metrics) {
    float target_quality = m_quality_scale.load();
    
    // Check if we're meeting FPS target
    if (metrics.fps < m_targets.min_fps) {
        // Reduce quality
        target_quality *= 0.95f;
        
        if (m_dynamic_resolution) {
            m_resolution_scale = std::max(0.5f, m_resolution_scale.load() - 0.05f);
        }
        
        if (m_dynamic_lod) {
            m_lod_bias = std::min(3, m_lod_bias.load() + 1);
        }
        
        if (m_dynamic_shadows) {
            m_shadow_quality = std::max(0, m_shadow_quality.load() - 1);
        }
    } else if (metrics.fps > m_targets.target_fps * 1.2f) {
        // Increase quality if we have headroom
        target_quality *= 1.05f;
        
        if (m_dynamic_resolution) {
            m_resolution_scale = std::min(1.0f, m_resolution_scale.load() + 0.05f);
        }
        
        if (m_dynamic_lod) {
            m_lod_bias = std::max(0, m_lod_bias.load() - 1);
        }
        
        if (m_dynamic_shadows) {
            m_shadow_quality = std::min(3, m_shadow_quality.load() + 1);
        }
    }
    
    // Smooth quality changes
    target_quality = std::clamp(target_quality, 0.25f, 1.0f);
    float current = m_quality_scale.load();
    m_quality_scale = current + (target_quality - current) * m_quality_change_rate;
}

void AutoOptimizer::AdjustMemoryUsage(const PerformanceMetrics& metrics) {
    size_t memory_mb = metrics.memory_allocated_bytes / (1024 * 1024);
    
    if (memory_mb > m_targets.max_memory_mb) {
        // Trigger memory cleanup
        if (m_strategies.enable_memory_compaction) {
            memory::MemoryOptimizer::Instance().CompactMemory();
        }
        
        if (m_strategies.enable_aggressive_caching) {
            memory::MemoryOptimizer::Instance().FlushCaches();
        }
    }
}

void AutoOptimizer::AdjustCPUUsage(const PerformanceMetrics& metrics) {
    if (metrics.cpu_usage_percent > m_targets.max_cpu_usage_percent) {
        // Reduce CPU load
        if (m_optimization_level == OptimizationLevel::ADAPTIVE) {
            m_current_level = OptimizationLevel::AGGRESSIVE;
        }
    } else if (metrics.cpu_usage_percent < m_targets.max_cpu_usage_percent * 0.5f) {
        // Can afford more CPU usage for better quality
        if (m_optimization_level == OptimizationLevel::ADAPTIVE) {
            m_current_level = OptimizationLevel::BALANCED;
        }
    }
}

void AutoOptimizer::ApplyOptimizations() {
    // Apply current optimization settings
    auto& batch_renderer = PerformanceOptimizationManager::Instance().GetBatchRenderer();
    batch_renderer.EnableInstancing(m_strategies.enable_instancing);
    batch_renderer.EnableSorting(m_strategies.enable_batching);
    
    // Apply memory optimizations
    auto& mem_optimizer = memory::MemoryOptimizer::Instance();
    mem_optimizer.EnableAggressiveCaching(m_strategies.enable_aggressive_caching);
    mem_optimizer.EnableMemoryCompaction(m_strategies.enable_memory_compaction);
}

// BenchmarkSystem Implementation
BenchmarkSystem::BenchmarkSystem() {
    // Register default benchmarks
    RegisterBenchmark({
        "UI Rendering",
        nullptr,
        [this]() { BenchmarkUIRendering(); },
        nullptr,
        100,
        10,
        std::chrono::milliseconds(5000)
    });
}

void BenchmarkSystem::RegisterBenchmark(const BenchmarkConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_benchmarks[config.name] = config;
}

BenchmarkSystem::BenchmarkResult BenchmarkSystem::RunBenchmark(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_benchmarks.find(name);
    if (it == m_benchmarks.end()) {
        return {}; // Benchmark not found
    }
    
    const auto& config = it->second;
    BenchmarkResult result;
    result.name = name;
    
    std::vector<float> timings;
    
    // Setup
    if (config.setup) config.setup();
    
    // Warmup
    for (size_t i = 0; i < config.warmup_iterations; ++i) {
        config.benchmark();
    }
    
    // Actual benchmark
    auto start_time = std::chrono::steady_clock::now();
    size_t iterations = 0;
    
    while (iterations < config.iterations) {
        auto iter_start = std::chrono::high_resolution_clock::now();
        config.benchmark();
        auto iter_end = std::chrono::high_resolution_clock::now();
        
        float time_ms = std::chrono::duration<float, std::milli>(iter_end - iter_start).count();
        timings.push_back(time_ms);
        
        iterations++;
        
        // Check if we've exceeded the time limit
        auto current_time = std::chrono::steady_clock::now();
        if (current_time - start_time > config.duration) {
            break;
        }
    }
    
    // Cleanup
    if (config.cleanup) config.cleanup();
    
    // Calculate statistics
    result.iterations = iterations;
    
    if (!timings.empty()) {
        // Average
        result.avg_time_ms = std::accumulate(timings.begin(), timings.end(), 0.0f) / timings.size();
        
        // Min/Max
        auto minmax = std::minmax_element(timings.begin(), timings.end());
        result.min_time_ms = *minmax.first;
        result.max_time_ms = *minmax.second;
        
        // Standard deviation
        float variance = 0;
        for (float t : timings) {
            variance += (t - result.avg_time_ms) * (t - result.avg_time_ms);
        }
        result.std_dev_ms = std::sqrt(variance / timings.size());
        
        // Percentiles
        std::sort(timings.begin(), timings.end());
        result.percentile_50_ms = timings[timings.size() / 2];
        result.percentile_95_ms = timings[static_cast<size_t>(timings.size() * 0.95f)];
        result.percentile_99_ms = timings[static_cast<size_t>(timings.size() * 0.99f)];
        
        // Operations per second
        if (result.avg_time_ms > 0) {
            result.operations_per_second = static_cast<size_t>(1000.0f / result.avg_time_ms);
        }
    }
    
    m_results.push_back(result);
    return result;
}

std::vector<BenchmarkSystem::BenchmarkResult> BenchmarkSystem::RunAllBenchmarks() {
    std::vector<BenchmarkResult> results;
    
    for (const auto& [name, config] : m_benchmarks) {
        results.push_back(RunBenchmark(name));
    }
    
    return results;
}

void BenchmarkSystem::ExportResults(const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) return;
    
    file << "Benchmark Results\n";
    file << "=================\n\n";
    
    for (const auto& result : m_results) {
        file << "Name: " << result.name << "\n";
        file << "  Iterations: " << result.iterations << "\n";
        file << "  Avg Time: " << result.avg_time_ms << " ms\n";
        file << "  Min Time: " << result.min_time_ms << " ms\n";
        file << "  Max Time: " << result.max_time_ms << " ms\n";
        file << "  Std Dev: " << result.std_dev_ms << " ms\n";
        file << "  50th %ile: " << result.percentile_50_ms << " ms\n";
        file << "  95th %ile: " << result.percentile_95_ms << " ms\n";
        file << "  99th %ile: " << result.percentile_99_ms << " ms\n";
        file << "  Ops/sec: " << result.operations_per_second << "\n\n";
    }
}

BenchmarkSystem::BenchmarkResult BenchmarkSystem::BenchmarkUIRendering() {
    // Placeholder for UI rendering benchmark
    BenchmarkResult result;
    result.name = "UI Rendering";
    // Actual implementation would render UI elements and measure time
    return result;
}

BenchmarkSystem::BenchmarkResult BenchmarkSystem::BenchmarkTextRendering() {
    // Placeholder for text rendering benchmark
    BenchmarkResult result;
    result.name = "Text Rendering";
    // Actual implementation would render text and measure time
    return result;
}

BenchmarkSystem::BenchmarkResult BenchmarkSystem::BenchmarkMemoryAllocation() {
    BenchmarkConfig config;
    config.name = "Memory Allocation";
    config.iterations = 10000;
    config.benchmark = []() {
        // Allocate and deallocate memory blocks
        for (int i = 0; i < 100; ++i) {
            void* ptr = malloc(1024 * (i % 64 + 1));
            memset(ptr, 0, 1024 * (i % 64 + 1));
            free(ptr);
        }
    };
    
    return RunBenchmark("Memory Allocation");
}

// PerformanceMonitor Implementation
PerformanceMonitor::PerformanceMonitor(MonitorConfig config) 
    : m_config(config) {
    m_metrics_history.reserve(HISTORY_SIZE);
}

void PerformanceMonitor::Update(float delta_time) {
    if (!m_visible) return;
    
    UpdateMetrics();
    CheckAlerts();
    
    if (m_recording) {
        // Record metrics to file
        // Implementation would write metrics to file
    }
}

void PerformanceMonitor::Render() {
    if (!m_visible) return;
    
    RenderOverlay();
    if (m_config.show_graph) {
        RenderGraph();
    }
}

void PerformanceMonitor::UpdateMetrics() {
    auto& profiler = PerformanceOptimizationManager::Instance().GetProfiler();
    auto metrics = profiler.CollectMetrics();
    
    std::lock_guard<std::mutex> lock(m_mutex);
    m_metrics_history.push_back(metrics);
    
    if (m_metrics_history.size() > HISTORY_SIZE) {
        m_metrics_history.pop_front();
    }
}

void PerformanceMonitor::RenderOverlay() {
    // This would render the performance overlay using the GUI system
    // For now, this is a placeholder
}

void PerformanceMonitor::RenderGraph() {
    // This would render performance graphs using the GUI system
    // For now, this is a placeholder
}

void PerformanceMonitor::CheckAlerts() {
    if (!m_alerts_enabled) return;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    m_active_alerts.clear();
    
    if (m_metrics_history.empty()) return;
    
    const auto& metrics = m_metrics_history.back();
    
    // Check FPS threshold
    auto fps_it = m_alert_thresholds.find("fps");
    if (fps_it != m_alert_thresholds.end() && metrics.fps < fps_it->second) {
        m_active_alerts.push_back("Low FPS: " + std::to_string(static_cast<int>(metrics.fps)));
    }
    
    // Check memory threshold
    auto mem_it = m_alert_thresholds.find("memory");
    if (mem_it != m_alert_thresholds.end()) {
        size_t memory_mb = metrics.memory_allocated_bytes / (1024 * 1024);
        if (memory_mb > mem_it->second) {
            m_active_alerts.push_back("High memory usage: " + std::to_string(memory_mb) + " MB");
        }
    }
}

// RegressionDetector Implementation
RegressionDetector::RegressionDetector() {
    m_samples.reserve(MIN_SAMPLES);
}

void RegressionDetector::SetBaseline(const PerformanceMetrics& metrics) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_baseline = metrics;
}

void RegressionDetector::CheckForRegressions(const PerformanceMetrics& metrics) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_samples.push_back(metrics);
    if (m_samples.size() > MIN_SAMPLES) {
        m_samples.pop_front();
    }
    
    if (m_samples.size() < MIN_SAMPLES) return;
    
    m_regressions.clear();
    
    // Calculate averages from samples
    float avg_fps = 0;
    float avg_frame_time = 0;
    size_t avg_memory = 0;
    
    for (const auto& sample : m_samples) {
        avg_fps += sample.fps;
        avg_frame_time += sample.frame_time_ms;
        avg_memory += sample.memory_allocated_bytes;
    }
    
    avg_fps /= m_samples.size();
    avg_frame_time /= m_samples.size();
    avg_memory /= m_samples.size();
    
    // Check for FPS regression
    if (m_baseline.fps > 0) {
        float change = (m_baseline.fps - avg_fps) / m_baseline.fps * 100.0f;
        if (change > m_threshold_percent) {
            Regression reg;
            reg.metric_name = "FPS";
            reg.baseline_value = m_baseline.fps;
            reg.current_value = avg_fps;
            reg.change_percent = change;
            reg.confidence = 0.95f; // Would calculate actual confidence
            reg.detected_at = std::chrono::steady_clock::now();
            m_regressions.push_back(reg);
        }
    }
    
    // Check for frame time regression
    if (m_baseline.frame_time_ms > 0) {
        float change = (avg_frame_time - m_baseline.frame_time_ms) / m_baseline.frame_time_ms * 100.0f;
        if (change > m_threshold_percent) {
            Regression reg;
            reg.metric_name = "Frame Time";
            reg.baseline_value = m_baseline.frame_time_ms;
            reg.current_value = avg_frame_time;
            reg.change_percent = change;
            reg.confidence = 0.95f;
            reg.detected_at = std::chrono::steady_clock::now();
            m_regressions.push_back(reg);
        }
    }
    
    // Check for memory regression
    if (m_baseline.memory_allocated_bytes > 0) {
        float change = static_cast<float>(avg_memory - m_baseline.memory_allocated_bytes) / 
                      m_baseline.memory_allocated_bytes * 100.0f;
        if (change > m_threshold_percent * 2) { // Higher threshold for memory
            Regression reg;
            reg.metric_name = "Memory Usage";
            reg.baseline_value = static_cast<float>(m_baseline.memory_allocated_bytes);
            reg.current_value = static_cast<float>(avg_memory);
            reg.change_percent = change;
            reg.confidence = 0.95f;
            reg.detected_at = std::chrono::steady_clock::now();
            m_regressions.push_back(reg);
        }
    }
}

std::vector<RegressionDetector::Regression> RegressionDetector::GetRegressions() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_regressions;
}

// PerformanceOptimizationManager Implementation
PerformanceOptimizationManager& PerformanceOptimizationManager::Instance() {
    static PerformanceOptimizationManager instance;
    return instance;
}

PerformanceOptimizationManager::PerformanceOptimizationManager() {
    Initialize();
}

PerformanceOptimizationManager::~PerformanceOptimizationManager() {
    Shutdown();
}

void PerformanceOptimizationManager::Initialize() {
    // Initialize all subsystems
    m_profiler = std::make_unique<ProfilerSession>("Main");
    m_analyzer = std::make_unique<PerformanceAnalyzer>();
    m_auto_optimizer = std::make_unique<AutoOptimizer>();
    m_benchmark_system = std::make_unique<BenchmarkSystem>();
    m_monitor = std::make_unique<PerformanceMonitor>();
    m_regression_detector = std::make_unique<RegressionDetector>();
    
    // Initialize optimization systems
    m_batch_renderer = std::make_unique<optimization::BatchRenderer>();
    m_command_buffer = std::make_unique<optimization::CommandBuffer>();
    m_texture_streamer = std::make_unique<optimization::TextureStreamer>();
    m_platform_optimizer = platform::PlatformOptimizer::Create();
    
    // Start profiling
    if (m_profiling_enabled) {
        m_profiler->Start();
    }
}

void PerformanceOptimizationManager::Shutdown() {
    // Stop all systems
    if (m_profiler) {
        m_profiler->Stop();
    }
    
    // Clean up
    m_profiler.reset();
    m_analyzer.reset();
    m_auto_optimizer.reset();
    m_benchmark_system.reset();
    m_monitor.reset();
    m_regression_detector.reset();
    m_batch_renderer.reset();
    m_command_buffer.reset();
    m_texture_streamer.reset();
    m_platform_optimizer.reset();
}

void PerformanceOptimizationManager::Update(float delta_time) {
    PROFILE_SCOPE("PerformanceOptimizationManager::Update");
    
    if (!m_profiling_enabled) return;
    
    // Collect current metrics
    auto metrics = m_profiler->CollectMetrics();
    
    // Update current stats
    m_current_fps = metrics.fps;
    m_avg_frame_time = metrics.frame_time_ms;
    
    // Analyze performance
    m_analyzer->Analyze(metrics);
    
    // Auto-optimize if enabled
    if (m_auto_optimization_enabled) {
        m_auto_optimizer->Update(metrics);
        m_auto_optimizer->ApplyOptimizations();
    }
    
    // Update monitor
    if (m_monitoring_enabled) {
        m_monitor->Update(delta_time);
    }
    
    // Check for regressions
    m_regression_detector->CheckForRegressions(metrics);
    
    // Update texture streaming
    m_texture_streamer->Update(delta_time);
}

void PerformanceOptimizationManager::SetOptimizationLevel(OptimizationLevel level) {
    m_auto_optimizer->SetOptimizationLevel(level);
    
    // Adjust strategies based on level
    OptimizationStrategies strategies;
    
    switch (level) {
        case OptimizationLevel::OFF:
            // Disable all optimizations
            strategies.enable_batching = false;
            strategies.enable_instancing = false;
            strategies.enable_occlusion_culling = false;
            strategies.enable_object_pooling = false;
            break;
            
        case OptimizationLevel::CONSERVATIVE:
            // Only safe optimizations
            strategies.enable_batching = true;
            strategies.enable_instancing = true;
            strategies.enable_occlusion_culling = false;
            strategies.enable_object_pooling = true;
            strategies.enable_simd = false;
            break;
            
        case OptimizationLevel::BALANCED:
            // Default balanced settings
            strategies.enable_batching = true;
            strategies.enable_instancing = true;
            strategies.enable_occlusion_culling = true;
            strategies.enable_object_pooling = true;
            strategies.enable_simd = true;
            strategies.enable_multithreading = true;
            break;
            
        case OptimizationLevel::AGGRESSIVE:
            // Maximum performance
            strategies.enable_batching = true;
            strategies.enable_instancing = true;
            strategies.enable_occlusion_culling = true;
            strategies.enable_lod = true;
            strategies.enable_texture_streaming = true;
            strategies.enable_gpu_driven_rendering = true;
            strategies.enable_object_pooling = true;
            strategies.enable_lazy_loading = true;
            strategies.enable_compression = true;
            strategies.enable_memory_compaction = true;
            strategies.enable_aggressive_caching = true;
            strategies.enable_simd = true;
            strategies.enable_multithreading = true;
            strategies.enable_job_system = true;
            strategies.enable_command_buffering = true;
            strategies.enable_platform_specific = true;
            strategies.enable_hardware_acceleration = true;
            break;
            
        case OptimizationLevel::ADAPTIVE:
            // Let the system decide
            break;
    }
    
    m_auto_optimizer->SetStrategies(strategies);
}

void PerformanceOptimizationManager::EnableProfiling(bool enable) {
    m_profiling_enabled = enable;
    
    if (enable && m_profiler) {
        m_profiler->Start();
    } else if (!enable && m_profiler) {
        m_profiler->Stop();
    }
}

void PerformanceOptimizationManager::EnableAutoOptimization(bool enable) {
    m_auto_optimization_enabled = enable;
}

void PerformanceOptimizationManager::EnableMonitoring(bool enable) {
    m_monitoring_enabled = enable;
    m_monitor->SetVisible(enable);
}

float PerformanceOptimizationManager::GetCurrentFPS() const {
    return m_current_fps.load();
}

float PerformanceOptimizationManager::GetAverageFrameTime() const {
    return m_avg_frame_time.load();
}

size_t PerformanceOptimizationManager::GetMemoryUsage() const {
    return memory::MemoryOptimizer::Instance().GetMemoryBudget().GetCurrentUsage()["total"];
}

float PerformanceOptimizationManager::GetCPUUsage() const {
    return m_platform_optimizer->DetectHardware().cpu_cores > 0 ? 
           50.0f : 0.0f; // Placeholder - would get actual CPU usage
}

float PerformanceOptimizationManager::GetGPUUsage() const {
    auto metrics = m_profiler->CollectMetrics();
    return metrics.gpu_usage_percent;
}

} // namespace ecscope::gui::performance