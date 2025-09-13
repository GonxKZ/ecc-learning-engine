#include "../include/ecscope/gui/performance_optimizer.hpp"
#include "../include/ecscope/gui/gui_core.hpp"
#include "../include/ecscope/gui/gui_widgets.hpp"
#include "../include/ecscope/gui/gui_layout.hpp"
#include "../include/ecscope/gui/gui_renderer.hpp"
#include <iostream>
#include <iomanip>
#include <random>
#include <vector>
#include <chrono>
#include <thread>

using namespace ecscope::gui;
using namespace ecscope::gui::performance;

class UIPerformanceBenchmark {
public:
    UIPerformanceBenchmark() 
        : m_rng(std::chrono::steady_clock::now().time_since_epoch().count()) {
        Initialize();
    }
    
    void Run() {
        std::cout << "=== ECScope UI Performance Benchmark Suite ===" << std::endl;
        std::cout << "Platform: " << GetPlatformString() << std::endl;
        std::cout << "CPU Cores: " << std::thread::hardware_concurrency() << std::endl;
        std::cout << "Optimization Level: " << GetOptimizationLevelString() << std::endl;
        std::cout << std::endl;
        
        // Run all benchmarks
        RunRenderingBenchmarks();
        RunMemoryBenchmarks();
        RunLayoutBenchmarks();
        RunEventBenchmarks();
        RunCacheBenchmarks();
        RunMultithreadingBenchmarks();
        RunPlatformSpecificBenchmarks();
        
        // Generate report
        GenerateReport();
    }
    
private:
    void Initialize() {
        // Initialize performance optimization manager
        auto& perf_manager = PerformanceOptimizationManager::Instance();
        perf_manager.EnableProfiling(true);
        perf_manager.EnableMonitoring(false); // Disable visual monitoring for benchmarks
        
        // Set up benchmark configurations
        SetupBenchmarks();
    }
    
    void SetupBenchmarks() {
        auto& benchmark_system = PerformanceOptimizationManager::Instance().GetBenchmarkSystem();
        
        // Widget rendering benchmark
        benchmark_system.RegisterBenchmark({
            "Widget Rendering (1000 widgets)",
            [this]() { SetupWidgets(1000); },
            [this]() { BenchmarkWidgetRendering(); },
            [this]() { CleanupWidgets(); },
            100, 10, std::chrono::seconds(5)
        });
        
        // Text rendering benchmark
        benchmark_system.RegisterBenchmark({
            "Text Rendering (10000 glyphs)",
            [this]() { SetupText(10000); },
            [this]() { BenchmarkTextRendering(); },
            [this]() { CleanupText(); },
            100, 10, std::chrono::seconds(5)
        });
        
        // Layout calculation benchmark
        benchmark_system.RegisterBenchmark({
            "Layout Calculation (Complex)",
            [this]() { SetupComplexLayout(); },
            [this]() { BenchmarkLayoutCalculation(); },
            [this]() { CleanupLayout(); },
            100, 10, std::chrono::seconds(5)
        });
        
        // Event processing benchmark
        benchmark_system.RegisterBenchmark({
            "Event Processing (10000 events)",
            [this]() { SetupEvents(10000); },
            [this]() { BenchmarkEventProcessing(); },
            [this]() { CleanupEvents(); },
            100, 10, std::chrono::seconds(5)
        });
        
        // Memory allocation benchmark
        benchmark_system.RegisterBenchmark({
            "Memory Pool Allocation",
            nullptr,
            [this]() { BenchmarkMemoryPoolAllocation(); },
            nullptr,
            10000, 100, std::chrono::seconds(5)
        });
        
        // Cache performance benchmark
        benchmark_system.RegisterBenchmark({
            "Cache Hit Rate",
            [this]() { SetupCache(); },
            [this]() { BenchmarkCachePerformance(); },
            [this]() { CleanupCache(); },
            1000, 10, std::chrono::seconds(5)
        });
        
        // Batch rendering benchmark
        benchmark_system.RegisterBenchmark({
            "Batch Rendering (10000 quads)",
            nullptr,
            [this]() { BenchmarkBatchRendering(); },
            nullptr,
            100, 10, std::chrono::seconds(5)
        });
        
        // SIMD optimization benchmark
        benchmark_system.RegisterBenchmark({
            "SIMD Transform (100000 vertices)",
            [this]() { SetupVertices(100000); },
            [this]() { BenchmarkSIMDTransform(); },
            [this]() { CleanupVertices(); },
            100, 10, std::chrono::seconds(5)
        });
    }
    
    void RunRenderingBenchmarks() {
        std::cout << "--- Rendering Performance ---" << std::endl;
        
        auto& benchmark_system = PerformanceOptimizationManager::Instance().GetBenchmarkSystem();
        
        // Test different widget counts
        for (int count : {100, 500, 1000, 5000, 10000}) {
            auto start = std::chrono::high_resolution_clock::now();
            
            SetupWidgets(count);
            float total_time = 0;
            const int iterations = 100;
            
            for (int i = 0; i < iterations; ++i) {
                auto frame_start = std::chrono::high_resolution_clock::now();
                BenchmarkWidgetRendering();
                auto frame_end = std::chrono::high_resolution_clock::now();
                
                total_time += std::chrono::duration<float, std::milli>(frame_end - frame_start).count();
            }
            
            CleanupWidgets();
            
            float avg_time = total_time / iterations;
            float fps = 1000.0f / avg_time;
            
            std::cout << "  " << std::setw(6) << count << " widgets: " 
                     << std::fixed << std::setprecision(2) 
                     << avg_time << " ms/frame (" 
                     << fps << " FPS)" << std::endl;
        }
        
        // Run batching comparison
        std::cout << "\n  Batching Performance:" << std::endl;
        
        // Without batching
        auto& batch_renderer = PerformanceOptimizationManager::Instance().GetBatchRenderer();
        batch_renderer.EnableInstancing(false);
        auto result_no_batch = benchmark_system.RunBenchmark("Batch Rendering (10000 quads)");
        std::cout << "    Without batching: " << result_no_batch.avg_time_ms << " ms" << std::endl;
        
        // With batching
        batch_renderer.EnableInstancing(true);
        auto result_batch = benchmark_system.RunBenchmark("Batch Rendering (10000 quads)");
        std::cout << "    With batching: " << result_batch.avg_time_ms << " ms" << std::endl;
        
        float improvement = (result_no_batch.avg_time_ms - result_batch.avg_time_ms) / 
                          result_no_batch.avg_time_ms * 100.0f;
        std::cout << "    Improvement: " << improvement << "%" << std::endl;
        
        std::cout << std::endl;
    }
    
    void RunMemoryBenchmarks() {
        std::cout << "--- Memory Performance ---" << std::endl;
        
        auto& mem_optimizer = memory::MemoryOptimizer::Instance();
        auto& benchmark_system = PerformanceOptimizationManager::Instance().GetBenchmarkSystem();
        
        // Test object pool performance
        auto pool_result = benchmark_system.RunBenchmark("Memory Pool Allocation");
        std::cout << "  Object Pool Allocation: " << pool_result.avg_time_ms << " ms" 
                 << " (" << pool_result.operations_per_second << " ops/sec)" << std::endl;
        
        // Test string interning
        auto& string_interner = mem_optimizer.GetStringInterner();
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < 10000; ++i) {
            std::string str = "TestString_" + std::to_string(i % 100);
            string_interner.Intern(str);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        float intern_time = std::chrono::duration<float, std::milli>(end - start).count();
        
        std::cout << "  String Interning (10000 strings): " << intern_time << " ms" << std::endl;
        std::cout << "  Interned strings: " << string_interner.GetInternedCount() << std::endl;
        std::cout << "  Memory saved: ~" 
                 << (10000 - string_interner.GetInternedCount()) * 32 / 1024 
                 << " KB" << std::endl;
        
        // Test memory compaction
        size_t before_compaction = mem_optimizer.GetMemoryBudget().GetCurrentUsage()["total"];
        size_t freed = mem_optimizer.CompactMemory();
        size_t after_compaction = mem_optimizer.GetMemoryBudget().GetCurrentUsage()["total"];
        
        std::cout << "  Memory Compaction:" << std::endl;
        std::cout << "    Before: " << before_compaction / 1024 << " KB" << std::endl;
        std::cout << "    After: " << after_compaction / 1024 << " KB" << std::endl;
        std::cout << "    Freed: " << freed / 1024 << " KB" << std::endl;
        
        std::cout << std::endl;
    }
    
    void RunLayoutBenchmarks() {
        std::cout << "--- Layout Performance ---" << std::endl;
        
        auto& benchmark_system = PerformanceOptimizationManager::Instance().GetBenchmarkSystem();
        
        // Test different layout complexities
        std::vector<std::pair<std::string, int>> layout_tests = {
            {"Simple Grid (10x10)", 100},
            {"Complex Grid (50x50)", 2500},
            {"Nested Flexbox (5 levels)", 500},
            {"Mixed Layout (Grid + Flex)", 1000}
        };
        
        for (const auto& [name, complexity] : layout_tests) {
            SetupComplexLayout(complexity);
            
            auto start = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < 100; ++i) {
                BenchmarkLayoutCalculation();
            }
            auto end = std::chrono::high_resolution_clock::now();
            
            float total_time = std::chrono::duration<float, std::milli>(end - start).count();
            float avg_time = total_time / 100.0f;
            
            CleanupLayout();
            
            std::cout << "  " << name << ": " << avg_time << " ms/calculation" << std::endl;
        }
        
        std::cout << std::endl;
    }
    
    void RunEventBenchmarks() {
        std::cout << "--- Event Processing Performance ---" << std::endl;
        
        // Test event throughput
        std::vector<int> event_counts = {100, 1000, 10000, 100000};
        
        for (int count : event_counts) {
            SetupEvents(count);
            
            auto start = std::chrono::high_resolution_clock::now();
            BenchmarkEventProcessing();
            auto end = std::chrono::high_resolution_clock::now();
            
            float time_ms = std::chrono::duration<float, std::milli>(end - start).count();
            float events_per_sec = count / (time_ms / 1000.0f);
            
            CleanupEvents();
            
            std::cout << "  " << std::setw(7) << count << " events: " 
                     << std::fixed << std::setprecision(2) 
                     << time_ms << " ms (" 
                     << static_cast<int>(events_per_sec) << " events/sec)" << std::endl;
        }
        
        std::cout << std::endl;
    }
    
    void RunCacheBenchmarks() {
        std::cout << "--- Cache Performance ---" << std::endl;
        
        auto& benchmark_system = PerformanceOptimizationManager::Instance().GetBenchmarkSystem();
        
        SetupCache();
        
        // Warm up cache
        for (int i = 0; i < 1000; ++i) {
            BenchmarkCachePerformance();
        }
        
        // Measure cache hit rate
        auto& cache_profiler = PerformanceOptimizationManager::Instance()
                                .GetProfiler().GetCacheProfiler();
        
        auto cache_stats = cache_profiler.GetAllCacheStats();
        
        for (const auto& [name, stats] : cache_stats) {
            std::cout << "  " << name << " Cache:" << std::endl;
            std::cout << "    Hit Rate: " << stats.hit_rate * 100.0f << "%" << std::endl;
            std::cout << "    Hits: " << stats.hits << std::endl;
            std::cout << "    Misses: " << stats.misses << std::endl;
            std::cout << "    Memory: " << stats.memory_used / 1024 << " KB" << std::endl;
        }
        
        CleanupCache();
        
        std::cout << std::endl;
    }
    
    void RunMultithreadingBenchmarks() {
        std::cout << "--- Multithreading Performance ---" << std::endl;
        
        // Test parallel command generation
        auto& perf_manager = PerformanceOptimizationManager::Instance();
        
        std::vector<int> thread_counts = {1, 2, 4, 8};
        
        for (int threads : thread_counts) {
            if (threads > static_cast<int>(std::thread::hardware_concurrency())) {
                continue;
            }
            
            optimization::ParallelCommandGenerator cmd_gen(threads);
            
            std::vector<optimization::ParallelCommandGenerator::CommandGenFunc> generators;
            for (int i = 0; i < 100; ++i) {
                generators.push_back([](optimization::CommandBuffer& buffer, size_t thread_id) {
                    // Simulate command generation work
                    for (int j = 0; j < 1000; ++j) {
                        buffer.SetShader(j % 10);
                        buffer.Draw(100);
                    }
                });
            }
            
            auto start = std::chrono::high_resolution_clock::now();
            cmd_gen.GenerateCommands(generators);
            auto end = std::chrono::high_resolution_clock::now();
            
            float time_ms = std::chrono::duration<float, std::milli>(end - start).count();
            
            std::cout << "  " << threads << " threads: " << time_ms << " ms" << std::endl;
        }
        
        std::cout << std::endl;
    }
    
    void RunPlatformSpecificBenchmarks() {
        std::cout << "--- Platform-Specific Optimizations ---" << std::endl;
        
        auto& platform_opt = PerformanceOptimizationManager::Instance().GetPlatformOptimizer();
        auto hw_caps = platform_opt.DetectHardware();
        
        std::cout << "  CPU: " << hw_caps.cpu_brand << std::endl;
        std::cout << "  Cores: " << hw_caps.cpu_cores << " (" << hw_caps.cpu_threads << " threads)" << std::endl;
        std::cout << "  SIMD Support:" << std::endl;
        
        if (hw_caps.has_sse) std::cout << "    SSE: Yes" << std::endl;
        if (hw_caps.has_sse2) std::cout << "    SSE2: Yes" << std::endl;
        if (hw_caps.has_avx) std::cout << "    AVX: Yes" << std::endl;
        if (hw_caps.has_avx2) std::cout << "    AVX2: Yes" << std::endl;
        if (hw_caps.has_avx512) std::cout << "    AVX512: Yes" << std::endl;
        
        // Test SIMD performance
        if (hw_caps.has_sse) {
            auto result = PerformanceOptimizationManager::Instance()
                         .GetBenchmarkSystem().RunBenchmark("SIMD Transform (100000 vertices)");
            std::cout << "  SIMD Transform: " << result.avg_time_ms << " ms" 
                     << " (" << result.operations_per_second << " transforms/sec)" << std::endl;
        }
        
        // Test platform-specific memory allocation
        const size_t alloc_size = 1024 * 1024; // 1 MB
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < 1000; ++i) {
            void* ptr = platform_opt.AllocateAligned(alloc_size, hw_caps.cache_line_size);
            platform_opt.FreeAligned(ptr);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        float alloc_time = std::chrono::duration<float, std::milli>(end - start).count();
        
        std::cout << "  Aligned Allocation (1000x 1MB): " << alloc_time << " ms" << std::endl;
        
        std::cout << std::endl;
    }
    
    void GenerateReport() {
        std::cout << "=== Performance Report ===" << std::endl;
        
        auto& perf_manager = PerformanceOptimizationManager::Instance();
        auto& analyzer = perf_manager.GetAnalyzer();
        auto analysis = analyzer.GetAnalysis();
        
        // Overall performance
        std::cout << "Overall Performance:" << std::endl;
        std::cout << "  Average FPS: " << analysis.avg_fps << std::endl;
        std::cout << "  95th Percentile Frame Time: " << analysis.percentile_95_frame_time << " ms" << std::endl;
        std::cout << "  Frame Time Variance: " << analysis.frame_time_variance << " ms" << std::endl;
        std::cout << "  Frame Drops: " << analysis.frame_drops << std::endl;
        std::cout << "  Stutters: " << analysis.stutters << std::endl;
        
        // Bottleneck analysis
        std::cout << "\nBottleneck Analysis:" << std::endl;
        std::string bottleneck = "None";
        switch (analysis.primary_bottleneck) {
            case PerformanceAnalyzer::Analysis::Bottleneck::CPU_BOUND:
                bottleneck = "CPU Bound";
                break;
            case PerformanceAnalyzer::Analysis::Bottleneck::GPU_BOUND:
                bottleneck = "GPU Bound";
                break;
            case PerformanceAnalyzer::Analysis::Bottleneck::MEMORY_BOUND:
                bottleneck = "Memory Bound";
                break;
            case PerformanceAnalyzer::Analysis::Bottleneck::IO_BOUND:
                bottleneck = "I/O Bound";
                break;
            case PerformanceAnalyzer::Analysis::Bottleneck::VSYNC_LIMITED:
                bottleneck = "VSync Limited";
                break;
        }
        std::cout << "  Primary Bottleneck: " << bottleneck << std::endl;
        
        // Issues and warnings
        if (!analysis.issues.empty()) {
            std::cout << "\nIssues Detected:" << std::endl;
            for (const auto& issue : analysis.issues) {
                std::cout << "  - " << issue << std::endl;
            }
        }
        
        if (!analysis.warnings.empty()) {
            std::cout << "\nWarnings:" << std::endl;
            for (const auto& warning : analysis.warnings) {
                std::cout << "  - " << warning << std::endl;
            }
        }
        
        // Optimization suggestions
        if (!analysis.suggestions.empty()) {
            std::cout << "\nOptimization Suggestions:" << std::endl;
            for (const auto& suggestion : analysis.suggestions) {
                std::cout << "  - " << suggestion << std::endl;
            }
        }
        
        // Export results
        auto& benchmark_system = perf_manager.GetBenchmarkSystem();
        benchmark_system.ExportResults("benchmark_results.txt");
        std::cout << "\nDetailed results exported to: benchmark_results.txt" << std::endl;
    }
    
    // Benchmark helper methods
    void SetupWidgets(int count) {
        m_test_widgets.clear();
        m_test_widgets.reserve(count);
        
        for (int i = 0; i < count; ++i) {
            // Create random widget properties
            float x = m_dist(m_rng) * 1920.0f;
            float y = m_dist(m_rng) * 1080.0f;
            float width = 50.0f + m_dist(m_rng) * 200.0f;
            float height = 30.0f + m_dist(m_rng) * 100.0f;
            
            m_test_widgets.push_back({x, y, width, height});
        }
    }
    
    void BenchmarkWidgetRendering() {
        // Simulate widget rendering
        for (const auto& widget : m_test_widgets) {
            // Simulate rendering operations
            volatile float result = widget.x * widget.y + widget.width * widget.height;
            (void)result;
        }
    }
    
    void CleanupWidgets() {
        m_test_widgets.clear();
    }
    
    void SetupText(int glyph_count) {
        m_test_text.clear();
        m_test_text.reserve(glyph_count);
        
        for (int i = 0; i < glyph_count; ++i) {
            m_test_text.push_back(static_cast<char>('A' + (i % 26)));
        }
    }
    
    void BenchmarkTextRendering() {
        // Simulate text rendering
        for (size_t i = 0; i < m_test_text.size(); ++i) {
            // Simulate glyph rendering
            volatile int result = m_test_text[i] * i;
            (void)result;
        }
    }
    
    void CleanupText() {
        m_test_text.clear();
    }
    
    void SetupComplexLayout(int complexity = 1000) {
        m_layout_complexity = complexity;
    }
    
    void BenchmarkLayoutCalculation() {
        // Simulate complex layout calculation
        float total = 0;
        for (int i = 0; i < m_layout_complexity; ++i) {
            total += std::sin(i) * std::cos(i);
        }
        volatile float result = total;
        (void)result;
    }
    
    void CleanupLayout() {
        m_layout_complexity = 0;
    }
    
    void SetupEvents(int count) {
        m_test_events.clear();
        m_test_events.reserve(count);
        
        for (int i = 0; i < count; ++i) {
            m_test_events.push_back({i, m_dist(m_rng), m_dist(m_rng)});
        }
    }
    
    void BenchmarkEventProcessing() {
        // Simulate event processing
        for (const auto& event : m_test_events) {
            volatile float result = event.type * event.x + event.y;
            (void)result;
        }
    }
    
    void CleanupEvents() {
        m_test_events.clear();
    }
    
    void SetupCache() {
        // Populate cache with test data
        for (int i = 0; i < 1000; ++i) {
            m_cache_data[i] = i * 3.14159f;
        }
    }
    
    void BenchmarkCachePerformance() {
        // Simulate cache access patterns
        float sum = 0;
        for (int i = 0; i < 10000; ++i) {
            int index = static_cast<int>(m_dist(m_rng) * 1000);
            sum += m_cache_data[index % 1000];
        }
        volatile float result = sum;
        (void)result;
    }
    
    void CleanupCache() {
        m_cache_data.clear();
    }
    
    void BenchmarkBatchRendering() {
        auto& batch_renderer = PerformanceOptimizationManager::Instance().GetBatchRenderer();
        
        batch_renderer.BeginBatch();
        
        for (int i = 0; i < 10000; ++i) {
            float positions[8] = {
                0.0f, 0.0f,
                1.0f, 0.0f,
                1.0f, 1.0f,
                0.0f, 1.0f
            };
            float uvs[8] = {
                0.0f, 0.0f,
                1.0f, 0.0f,
                1.0f, 1.0f,
                0.0f, 1.0f
            };
            batch_renderer.AddQuad(positions, uvs, 0xFFFFFFFF, i % 10);
        }
        
        batch_renderer.EndBatch();
        batch_renderer.Flush();
    }
    
    void BenchmarkMemoryPoolAllocation() {
        struct TestObject {
            float data[16];
        };
        
        auto& pool = memory::MemoryOptimizer::Instance().GetObjectPool<TestObject>();
        
        std::vector<TestObject*> objects;
        objects.reserve(1000);
        
        // Allocate
        for (int i = 0; i < 1000; ++i) {
            objects.push_back(pool.Acquire());
        }
        
        // Deallocate
        for (auto* obj : objects) {
            pool.Release(obj);
        }
    }
    
    void SetupVertices(int count) {
        m_vertices.resize(count * 4);
        for (int i = 0; i < count * 4; ++i) {
            m_vertices[i] = m_dist(m_rng);
        }
        
        m_transformed_vertices.resize(count * 4);
        
        // Setup transformation matrix
        for (int i = 0; i < 16; ++i) {
            m_transform_matrix[i] = (i % 5 == 0) ? 1.0f : 0.0f;
        }
    }
    
    void BenchmarkSIMDTransform() {
        optimization::simd::TransformVertices4x4_SSE(
            m_vertices.data(),
            m_transformed_vertices.data(),
            m_transform_matrix,
            m_vertices.size() / 4
        );
    }
    
    void CleanupVertices() {
        m_vertices.clear();
        m_transformed_vertices.clear();
    }
    
    std::string GetPlatformString() {
        auto platform = platform::PlatformOptimizer::GetCurrentPlatform();
        switch (platform) {
            case platform::Platform::WINDOWS: return "Windows";
            case platform::Platform::LINUX: return "Linux";
            case platform::Platform::MACOS: return "macOS";
            default: return "Unknown";
        }
    }
    
    std::string GetOptimizationLevelString() {
        auto& auto_optimizer = PerformanceOptimizationManager::Instance().GetAutoOptimizer();
        auto level = auto_optimizer.GetCurrentLevel();
        
        switch (level) {
            case OptimizationLevel::OFF: return "Off";
            case OptimizationLevel::CONSERVATIVE: return "Conservative";
            case OptimizationLevel::BALANCED: return "Balanced";
            case OptimizationLevel::AGGRESSIVE: return "Aggressive";
            case OptimizationLevel::ADAPTIVE: return "Adaptive";
            default: return "Unknown";
        }
    }
    
    // Test data
    struct TestWidget {
        float x, y, width, height;
    };
    
    struct TestEvent {
        int type;
        float x, y;
    };
    
    std::vector<TestWidget> m_test_widgets;
    std::vector<char> m_test_text;
    std::vector<TestEvent> m_test_events;
    std::unordered_map<int, float> m_cache_data;
    std::vector<float> m_vertices;
    std::vector<float> m_transformed_vertices;
    float m_transform_matrix[16];
    int m_layout_complexity = 0;
    
    // Random number generation
    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_dist{0.0f, 1.0f};
};

int main(int argc, char* argv[]) {
    try {
        // Parse command line arguments
        OptimizationLevel opt_level = OptimizationLevel::BALANCED;
        
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--aggressive") {
                opt_level = OptimizationLevel::AGGRESSIVE;
            } else if (arg == "--conservative") {
                opt_level = OptimizationLevel::CONSERVATIVE;
            } else if (arg == "--adaptive") {
                opt_level = OptimizationLevel::ADAPTIVE;
            } else if (arg == "--off") {
                opt_level = OptimizationLevel::OFF;
            }
        }
        
        // Initialize performance manager
        auto& perf_manager = PerformanceOptimizationManager::Instance();
        perf_manager.SetOptimizationLevel(opt_level);
        
        // Run benchmarks
        UIPerformanceBenchmark benchmark;
        benchmark.Run();
        
        // Check for regressions
        auto& regression_detector = perf_manager.GetRegressionDetector();
        if (regression_detector.HasRegressions()) {
            std::cout << "\n=== PERFORMANCE REGRESSIONS DETECTED ===" << std::endl;
            for (const auto& regression : regression_detector.GetRegressions()) {
                std::cout << "  " << regression.metric_name << ": " 
                         << regression.change_percent << "% slower" << std::endl;
            }
            return 1; // Exit with error code
        }
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Benchmark failed: " << e.what() << std::endl;
        return 1;
    }
}