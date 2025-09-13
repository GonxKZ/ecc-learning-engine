#include <ecscope/gui/performance_profiler.hpp>
#include <ecscope/gui/memory_optimization.hpp>
#include <ecscope/gui/cpu_gpu_optimization.hpp>
#include <ecscope/gui/caching_system.hpp>
#include <ecscope/gui/platform_optimization.hpp>

#include <iostream>
#include <random>
#include <chrono>
#include <thread>
#include <vector>
#include <memory>
#include <functional>
#include <iomanip>

using namespace ecscope::gui;

// Test UI element for performance testing
struct TestUIElement {
    float x, y, width, height;
    uint32_t color;
    std::string text;
    bool visible;
    uint32_t texture_id;
    
    void Update(float delta_time) {
        // Simulate some computation
        x += std::sin(delta_time) * 0.1f;
        y += std::cos(delta_time) * 0.1f;
    }
    
    void Render() {
        // Simulate rendering
        if (visible) {
            // Would render here
        }
    }
};

// Performance test scenarios
class PerformanceTestSuite {
public:
    PerformanceTestSuite() 
        : m_profiler_session("Performance Test")
        , m_batch_renderer()
        , m_command_buffer()
        , m_memory_optimizer(memory::MemoryOptimizer::Instance())
        , m_platform_optimizer(platform::PlatformOptimizer::Create()) {
        
        // Set global profiler session
        performance::ProfilerSession::SetGlobalSession(&m_profiler_session);
        
        // Initialize platform-specific optimizations
        InitializePlatformOptimizations();
        
        // Set up performance budget
        performance::PerformanceBudget::Budget budget;
        budget.frame_time_ms = 16.67f;  // 60 FPS target
        budget.memory_mb = 512;
        budget.gpu_time_ms = 10.0f;
        budget.draw_calls = 1000;
        budget.triangles = 1000000;
        budget.cache_hit_rate = 0.9f;
        m_performance_budget.SetBudget(budget);
    }
    
    void RunAllTests() {
        std::cout << "\n========================================\n";
        std::cout << "   ECScope Performance Optimization Suite\n";
        std::cout << "========================================\n\n";
        
        // Detect hardware capabilities
        DetectHardware();
        
        // Run test scenarios
        TestMemoryOptimization();
        TestBatchRendering();
        TestCaching();
        TestMultithreadedRendering();
        TestGPUOptimization();
        TestPlatformSpecificOptimizations();
        TestAutoTuning();
        
        // Generate performance report
        GeneratePerformanceReport();
    }
    
private:
    void InitializePlatformOptimizations() {
        auto caps = m_platform_optimizer->DetectHardware();
        
        std::cout << "Hardware Detection:\n";
        std::cout << "  CPU: " << caps.cpu_brand << " (" << caps.cpu_cores << " cores)\n";
        std::cout << "  GPU: " << caps.gpu_renderer << "\n";
        std::cout << "  Memory: " << caps.total_memory_mb << " MB\n";
        std::cout << "  Display: " << caps.primary_display_width << "x" 
                  << caps.primary_display_height << " @ " 
                  << caps.primary_display_refresh_rate << "Hz\n";
        
        // Apply platform-specific optimizations
        auto render_hints = m_platform_optimizer->GetRenderingHints();
        if (render_hints.use_instancing) {
            m_batch_renderer.EnableInstancing(true);
        }
        
        auto memory_hints = m_platform_optimizer->GetMemoryHints();
        if (memory_hints.use_memory_pools) {
            m_memory_optimizer.EnableMemoryCompaction(true);
        }
        
        // Request high performance mode
        m_platform_optimizer->RequestHighPerformance();
    }
    
    void DetectHardware() {
        std::cout << "\n--- Hardware Capabilities ---\n";
        
        auto caps = m_platform_optimizer->DetectHardware();
        
        // SIMD support
        std::cout << "SIMD Support:\n";
        if (caps.has_sse) std::cout << "  SSE: Yes\n";
        if (caps.has_sse2) std::cout << "  SSE2: Yes\n";
        if (caps.has_avx) std::cout << "  AVX: Yes\n";
        if (caps.has_avx2) std::cout << "  AVX2: Yes\n";
        
        // Cache sizes
        std::cout << "Cache Hierarchy:\n";
        std::cout << "  L1: " << caps.l1_cache_size / 1024 << " KB\n";
        std::cout << "  L2: " << caps.l2_cache_size / 1024 << " KB\n";
        std::cout << "  L3: " << caps.l3_cache_size / (1024 * 1024) << " MB\n";
        
        std::cout << std::endl;
    }
    
    void TestMemoryOptimization() {
        std::cout << "\n--- Memory Optimization Test ---\n";
        
        PROFILE_SCOPE("Memory Optimization Test");
        
        // Test object pooling
        {
            PROFILE_SCOPE("Object Pool Test");
            
            auto& pool = m_memory_optimizer.GetObjectPool<TestUIElement>();
            std::vector<TestUIElement*> elements;
            
            // Allocate from pool
            auto start = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < 10000; ++i) {
                elements.push_back(pool.Acquire());
            }
            auto alloc_time = std::chrono::high_resolution_clock::now() - start;
            
            // Release back to pool
            start = std::chrono::high_resolution_clock::now();
            for (auto* elem : elements) {
                pool.Release(elem);
            }
            auto release_time = std::chrono::high_resolution_clock::now() - start;
            
            std::cout << "Object Pool Performance:\n";
            std::cout << "  Allocation time: " 
                     << std::chrono::duration_cast<std::chrono::microseconds>(alloc_time).count() 
                     << " us\n";
            std::cout << "  Release time: " 
                     << std::chrono::duration_cast<std::chrono::microseconds>(release_time).count() 
                     << " us\n";
            std::cout << "  Pool utilization: " << pool.GetUtilization() * 100 << "%\n";
        }
        
        // Test string interning
        {
            PROFILE_SCOPE("String Interning Test");
            
            auto& interner = m_memory_optimizer.GetStringInterner();
            
            std::vector<memory::StringInterner::StringId> ids;
            auto start = std::chrono::high_resolution_clock::now();
            
            for (int i = 0; i < 1000; ++i) {
                std::string str = "Test String " + std::to_string(i % 100);
                ids.push_back(interner.Intern(str));
            }
            
            auto intern_time = std::chrono::high_resolution_clock::now() - start;
            
            std::cout << "String Interning Performance:\n";
            std::cout << "  Intern time: " 
                     << std::chrono::duration_cast<std::chrono::microseconds>(intern_time).count() 
                     << " us\n";
            std::cout << "  Unique strings: " << interner.GetInternedCount() << "\n";
            std::cout << "  Memory usage: " << interner.GetMemoryUsage() / 1024 << " KB\n";
        }
        
        // Test memory pressure handling
        {
            PROFILE_SCOPE("Memory Pressure Test");
            
            auto& pressure_handler = m_memory_optimizer.GetPressureHandler();
            
            // Register cleanup callbacks
            pressure_handler.RegisterCleanupCallback(
                "UI Cache",
                memory::MemoryPriority::CACHE,
                [](memory::MemoryPressureHandler::PressureLevel level) {
                    // Simulate cache cleanup
                    return size_t(1024 * 1024); // 1MB freed
                }
            );
            
            // Simulate memory pressure
            size_t freed = pressure_handler.HandleMemoryPressure(
                memory::MemoryPressureHandler::PressureLevel::MEDIUM
            );
            
            std::cout << "Memory Pressure Handling:\n";
            std::cout << "  Memory freed: " << freed / (1024 * 1024) << " MB\n";
        }
    }
    
    void TestBatchRendering() {
        std::cout << "\n--- Batch Rendering Test ---\n";
        
        PROFILE_SCOPE("Batch Rendering Test");
        
        // Generate test geometry
        std::vector<float> positions(10000 * 8);  // 10000 quads
        std::vector<float> uvs(10000 * 8);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> pos_dist(-100, 100);
        std::uniform_real_distribution<> uv_dist(0, 1);
        
        for (size_t i = 0; i < positions.size(); ++i) {
            positions[i] = pos_dist(gen);
            uvs[i] = uv_dist(gen);
        }
        
        // Test batching performance
        m_batch_renderer.BeginBatch();
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < 10000; ++i) {
            m_batch_renderer.AddQuad(
                &positions[i * 8],
                &uvs[i * 8],
                0xFFFFFFFF,
                i % 10  // Use 10 different textures
            );
        }
        
        m_batch_renderer.EndBatch();
        
        auto batch_time = std::chrono::high_resolution_clock::now() - start;
        
        std::cout << "Batch Rendering Performance:\n";
        std::cout << "  Batch creation time: " 
                 << std::chrono::duration_cast<std::chrono::microseconds>(batch_time).count() 
                 << " us\n";
        std::cout << "  Draw calls: " << m_batch_renderer.GetDrawCallCount() << "\n";
        std::cout << "  Vertices: " << m_batch_renderer.GetVertexCount() << "\n";
        std::cout << "  Batched commands: " << m_batch_renderer.GetBatchedCommandCount() << "\n";
        
        // Test command buffer optimization
        {
            PROFILE_SCOPE("Command Buffer Test");
            
            m_command_buffer.Begin();
            
            // Record many redundant state changes
            for (int i = 0; i < 1000; ++i) {
                m_command_buffer.SetShader(1);  // Same shader
                m_command_buffer.SetTexture(0, 1);  // Same texture
                m_command_buffer.Draw(6, i * 6);
            }
            
            m_command_buffer.End();  // Will optimize out redundant changes
            
            std::cout << "Command Buffer Optimization:\n";
            std::cout << "  Commands after optimization: " << m_command_buffer.GetCommandCount() << "\n";
        }
    }
    
    void TestCaching() {
        std::cout << "\n--- Caching System Test ---\n";
        
        PROFILE_SCOPE("Caching Test");
        
        // Test multi-level cache
        {
            std::vector<cache::MultiLevelCache<int, std::string>::Level> levels = {
                {100, 1024 * 1024, std::chrono::milliseconds(100), cache::EvictionPolicy::LRU},     // L1
                {1000, 10 * 1024 * 1024, std::chrono::milliseconds(1000), cache::EvictionPolicy::LFU}, // L2
                {10000, 100 * 1024 * 1024, std::chrono::milliseconds(10000), cache::EvictionPolicy::FIFO} // L3
            };
            
            cache::MultiLevelCache<int, std::string> cache(levels);
            
            // Populate cache
            for (int i = 0; i < 500; ++i) {
                cache.Put(i, "Value " + std::to_string(i));
            }
            
            // Test cache hits
            int hits = 0;
            int misses = 0;
            
            auto start = std::chrono::high_resolution_clock::now();
            
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dist(0, 999);
            
            for (int i = 0; i < 10000; ++i) {
                auto result = cache.Get(dist(gen));
                if (result.has_value()) {
                    hits++;
                } else {
                    misses++;
                }
            }
            
            auto access_time = std::chrono::high_resolution_clock::now() - start;
            
            auto stats = cache.GetStats();
            
            std::cout << "Multi-Level Cache Performance:\n";
            std::cout << "  Access time: " 
                     << std::chrono::duration_cast<std::chrono::microseconds>(access_time).count() 
                     << " us\n";
            std::cout << "  Hit rate: " << stats.hit_rate * 100 << "%\n";
            std::cout << "  Average access time: " << stats.avg_access_time_ms << " ms\n";
            std::cout << "  Evictions: " << stats.eviction_count << "\n";
        }
        
        // Test lazy loading
        {
            PROFILE_SCOPE("Lazy Loading Test");
            
            cache::LazyAsset<TestUIElement> lazy_asset([]() {
                // Simulate expensive loading
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                return std::make_shared<TestUIElement>();
            });
            
            auto start = std::chrono::high_resolution_clock::now();
            auto asset = lazy_asset.Get();  // First access triggers load
            auto first_access = std::chrono::high_resolution_clock::now() - start;
            
            start = std::chrono::high_resolution_clock::now();
            asset = lazy_asset.Get();  // Second access is cached
            auto second_access = std::chrono::high_resolution_clock::now() - start;
            
            std::cout << "Lazy Loading Performance:\n";
            std::cout << "  First access: " 
                     << std::chrono::duration_cast<std::chrono::microseconds>(first_access).count() 
                     << " us\n";
            std::cout << "  Second access: " 
                     << std::chrono::duration_cast<std::chrono::microseconds>(second_access).count() 
                     << " us\n";
        }
    }
    
    void TestMultithreadedRendering() {
        std::cout << "\n--- Multi-threaded Rendering Test ---\n";
        
        PROFILE_SCOPE("Multithreaded Rendering");
        
        optimization::ParallelCommandGenerator parallel_gen(std::thread::hardware_concurrency());
        parallel_gen.SetMainCommandBuffer(&m_command_buffer);
        
        std::vector<optimization::ParallelCommandGenerator::CommandGenFunc> generators;
        
        // Create work for each thread
        for (size_t t = 0; t < std::thread::hardware_concurrency(); ++t) {
            generators.push_back([t](optimization::CommandBuffer& buffer, size_t thread_id) {
                // Each thread generates commands for a portion of the scene
                for (int i = 0; i < 100; ++i) {
                    buffer.SetShader(thread_id + 1);
                    buffer.Draw(6, i * 6);
                }
            });
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        parallel_gen.GenerateCommands(generators);
        parallel_gen.ExecuteCommands();
        auto parallel_time = std::chrono::high_resolution_clock::now() - start;
        
        std::cout << "Parallel Command Generation:\n";
        std::cout << "  Generation time: " 
                 << std::chrono::duration_cast<std::chrono::microseconds>(parallel_time).count() 
                 << " us\n";
        std::cout << "  Threads used: " << std::thread::hardware_concurrency() << "\n";
    }
    
    void TestGPUOptimization() {
        std::cout << "\n--- GPU Optimization Test ---\n";
        
        PROFILE_SCOPE("GPU Optimization");
        
        // Test occlusion culling
        {
            optimization::OcclusionCuller culler;
            culler.SetResolution(1920, 1080);
            
            float view_matrix[16] = {
                1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 1, 0,
                0, 0, -5, 1
            };
            
            float proj_matrix[16] = {
                1.3f, 0, 0, 0,
                0, 1.7f, 0, 0,
                0, 0, -1.1f, -1,
                0, 0, -2.2f, 0
            };
            
            culler.BeginFrame(view_matrix, proj_matrix);
            
            // Test visibility of many objects
            int visible_count = 0;
            for (int i = 0; i < 1000; ++i) {
                optimization::OcclusionCuller::BoundingBox bbox;
                bbox.min[0] = i * 2.0f - 1000.0f;
                bbox.min[1] = 0;
                bbox.min[2] = -10;
                bbox.max[0] = bbox.min[0] + 1.0f;
                bbox.max[1] = bbox.min[1] + 1.0f;
                bbox.max[2] = bbox.min[2] + 1.0f;
                
                if (culler.IsVisible(bbox)) {
                    visible_count++;
                }
            }
            
            culler.EndFrame();
            
            std::cout << "Occlusion Culling:\n";
            std::cout << "  Objects tested: 1000\n";
            std::cout << "  Visible objects: " << culler.GetVisibleObjectCount() << "\n";
            std::cout << "  Culled objects: " << culler.GetCulledObjectCount() << "\n";
        }
        
        // Test texture streaming
        {
            optimization::TextureStreamer streamer(256);  // 256MB budget
            
            std::atomic<int> loaded_count{0};
            
            auto start = std::chrono::high_resolution_clock::now();
            
            // Request multiple textures
            for (int i = 0; i < 50; ++i) {
                streamer.RequestTexture(
                    "texture_" + std::to_string(i),
                    optimization::TextureStreamer::Priority::NORMAL,
                    [&loaded_count](uint32_t id) {
                        loaded_count++;
                    }
                );
            }
            
            // Wait for streaming to complete
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            auto stream_time = std::chrono::high_resolution_clock::now() - start;
            
            std::cout << "Texture Streaming:\n";
            std::cout << "  Textures requested: 50\n";
            std::cout << "  Textures loaded: " << loaded_count.load() << "\n";
            std::cout << "  Stream time: " 
                     << std::chrono::duration_cast<std::chrono::milliseconds>(stream_time).count() 
                     << " ms\n";
        }
        
        // Test GPU memory management
        {
            optimization::GPUMemoryManager gpu_mem(1024);  // 1GB budget
            
            // Allocate various GPU resources
            auto* vb = gpu_mem.Allocate(
                optimization::GPUMemoryManager::MemoryType::VERTEX_BUFFER,
                10 * 1024 * 1024,  // 10MB
                true
            );
            
            auto* ib = gpu_mem.Allocate(
                optimization::GPUMemoryManager::MemoryType::INDEX_BUFFER,
                5 * 1024 * 1024,  // 5MB
                true
            );
            
            std::cout << "GPU Memory Management:\n";
            std::cout << "  Used memory: " << gpu_mem.GetUsedMemory() / (1024 * 1024) << " MB\n";
            std::cout << "  Available memory: " << gpu_mem.GetAvailableMemory() / (1024 * 1024) << " MB\n";
            std::cout << "  Fragmentation: " << gpu_mem.GetFragmentation() * 100 << "%\n";
            
            gpu_mem.Free(vb);
            gpu_mem.Free(ib);
        }
    }
    
    void TestPlatformSpecificOptimizations() {
        std::cout << "\n--- Platform-Specific Optimizations ---\n";
        
        PROFILE_SCOPE("Platform Optimizations");
        
        // Test SIMD optimizations
        {
            const size_t count = 10000;
            std::vector<float> vertices(count * 4);
            std::vector<float> output(count * 4);
            float matrix[16] = {
                1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1
            };
            
            // Fill with test data
            for (size_t i = 0; i < vertices.size(); ++i) {
                vertices[i] = i * 0.1f;
            }
            
            auto start = std::chrono::high_resolution_clock::now();
            
            // Use SIMD transformation
            optimization::simd::TransformVertices4x4_SSE(
                vertices.data(), output.data(), matrix, count / 4
            );
            
            auto simd_time = std::chrono::high_resolution_clock::now() - start;
            
            std::cout << "SIMD Performance:\n";
            std::cout << "  Transform time: " 
                     << std::chrono::duration_cast<std::chrono::microseconds>(simd_time).count() 
                     << " us for " << count << " vertices\n";
        }
        
        // Test platform hints
        {
            auto render_hints = m_platform_optimizer->GetRenderingHints();
            auto memory_hints = m_platform_optimizer->GetMemoryHints();
            auto thread_hints = m_platform_optimizer->GetThreadingHints();
            
            std::cout << "Platform Optimization Hints:\n";
            std::cout << "  Use instancing: " << (render_hints.use_instancing ? "Yes" : "No") << "\n";
            std::cout << "  Use persistent mapping: " << (render_hints.use_persistent_mapping ? "Yes" : "No") << "\n";
            std::cout << "  Use memory pools: " << (memory_hints.use_memory_pools ? "Yes" : "No") << "\n";
            std::cout << "  Cache line size: " << memory_hints.cache_line_size << " bytes\n";
            std::cout << "  Worker threads: " << thread_hints.worker_thread_count << "\n";
        }
    }
    
    void TestAutoTuning() {
        std::cout << "\n--- Auto-Tuning Test ---\n";
        
        PROFILE_SCOPE("Auto Tuning");
        
        platform::PerformanceAutoTuner tuner;
        
        platform::PerformanceAutoTuner::TuningProfile profile;
        profile.target_fps = 60;
        profile.min_fps = 30;
        profile.gpu_budget_ms = 16.0f;
        profile.cpu_budget_ms = 8.0f;
        profile.max_memory_mb = 512;
        
        tuner.SetTargetProfile(profile);
        tuner.EnableAutoTuning(true);
        
        // Simulate performance metrics over time
        for (int frame = 0; frame < 100; ++frame) {
            float frame_time = 16.0f + std::sin(frame * 0.1f) * 8.0f;
            float gpu_time = 10.0f + std::sin(frame * 0.15f) * 5.0f;
            size_t memory = 400 + frame;
            
            tuner.UpdateMetrics(frame_time, gpu_time, memory);
            
            if (frame % 20 == 0) {
                auto optimal = tuner.GetOptimalProfile();
                std::cout << "  Frame " << frame << ": Quality=" << optimal.texture_quality 
                         << ", Shadows=" << optimal.shadow_quality << "\n";
            }
        }
        
        auto final_profile = tuner.GetOptimalProfile();
        
        std::cout << "Auto-Tuning Results:\n";
        std::cout << "  Final texture quality: " << final_profile.texture_quality << "\n";
        std::cout << "  Final shadow quality: " << final_profile.shadow_quality << "\n";
        std::cout << "  Final effect quality: " << final_profile.effect_quality << "\n";
        std::cout << "  UI scale: " << final_profile.ui_scale << "%\n";
    }
    
    void GeneratePerformanceReport() {
        std::cout << "\n========================================\n";
        std::cout << "   Performance Analysis Report\n";
        std::cout << "========================================\n\n";
        
        // Collect final metrics
        auto metrics = m_profiler_session.CollectMetrics();
        
        std::cout << "Frame Performance:\n";
        std::cout << "  Average FPS: " << metrics.fps << "\n";
        std::cout << "  Min FPS: " << metrics.fps_min << "\n";
        std::cout << "  Max FPS: " << metrics.fps_max << "\n";
        std::cout << "  Frame time variance: " << metrics.frame_time_variance << " ms\n";
        
        std::cout << "\nMemory Usage:\n";
        std::cout << "  Allocated: " << metrics.memory_allocated_bytes / (1024 * 1024) << " MB\n";
        std::cout << "  Peak: " << metrics.memory_peak_bytes / (1024 * 1024) << " MB\n";
        std::cout << "  Fragmentation: " << metrics.memory_fragmentation * 100 << "%\n";
        
        std::cout << "\nGPU Performance:\n";
        std::cout << "  GPU time: " << metrics.gpu_time_ms << " ms\n";
        std::cout << "  GPU memory: " << metrics.gpu_memory_used_bytes / (1024 * 1024) << " MB\n";
        
        std::cout << "\nCache Performance:\n";
        std::cout << "  Hit rate: " << metrics.cache_hit_rate * 100 << "%\n";
        std::cout << "  Cache memory: " << metrics.cache_memory_bytes / 1024 << " KB\n";
        
        // Check performance budget
        bool within_budget = m_performance_budget.CheckBudget(metrics);
        
        std::cout << "\nPerformance Budget: " << (within_budget ? "PASS" : "FAIL") << "\n";
        
        if (!within_budget) {
            auto violations = m_performance_budget.GetBudgetViolations(metrics);
            std::cout << "Budget Violations:\n";
            for (const auto& violation : violations) {
                std::cout << "  - " << violation << "\n";
            }
        }
        
        // Get optimization suggestions
        auto warnings = m_profiler_session.GetPerformanceWarnings();
        if (!warnings.empty()) {
            std::cout << "\nPerformance Warnings:\n";
            for (const auto& warning : warnings) {
                std::cout << "  - " << warning << "\n";
            }
        }
        
        auto suggestions = m_profiler_session.GetOptimizationSuggestions();
        if (!suggestions.empty()) {
            std::cout << "\nOptimization Suggestions:\n";
            for (const auto& suggestion : suggestions) {
                std::cout << "  - " << suggestion << "\n";
            }
        }
        
        // Export profiling data
        m_profiler_session.ExportToJSON("performance_report.json");
        m_profiler_session.ExportToCSV("performance_metrics.csv");
        m_profiler_session.ExportToChrome("chrome_trace.json");
        
        std::cout << "\nPerformance data exported to:\n";
        std::cout << "  - performance_report.json\n";
        std::cout << "  - performance_metrics.csv\n";
        std::cout << "  - chrome_trace.json (viewable in Chrome DevTools)\n";
    }
    
private:
    performance::ProfilerSession m_profiler_session;
    performance::PerformanceBudget m_performance_budget;
    optimization::BatchRenderer m_batch_renderer;
    optimization::CommandBuffer m_command_buffer;
    memory::MemoryOptimizer& m_memory_optimizer;
    std::unique_ptr<platform::PlatformOptimizer> m_platform_optimizer;
};

int main() {
    try {
        std::cout << "ECScope UI Performance Optimization Demo\n";
        std::cout << "=========================================\n\n";
        
        PerformanceTestSuite test_suite;
        test_suite.RunAllTests();
        
        std::cout << "\n========================================\n";
        std::cout << "Performance optimization demo completed!\n";
        std::cout << "========================================\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}