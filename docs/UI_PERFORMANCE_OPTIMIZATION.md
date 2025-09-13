# ECScope UI Performance Optimization Guide

## Overview

The ECScope UI system includes a comprehensive performance optimization framework designed to achieve 60+ FPS consistent frame rates across all platforms while minimizing memory usage and maximizing responsiveness.

## Architecture

### Core Components

1. **Performance Profiler** - Real-time performance monitoring and analysis
2. **Memory Optimizer** - Advanced memory management and pooling
3. **CPU/GPU Optimizer** - Hardware acceleration and parallel processing
4. **Cache System** - Multi-level caching with intelligent eviction
5. **Platform Optimizer** - Platform-specific optimizations
6. **Auto-Tuner** - Automatic quality adjustment based on performance

## Performance Profiling

### Using the Profiler

```cpp
#include <ecscope/gui/performance_profiler.hpp>

// Create and activate a profiling session
auto session = std::make_unique<ProfilerSession>("MyApp");
ProfilerSession::SetGlobalSession(session.get());
session->Start();

// Profile code blocks
{
    PROFILE_SCOPE("UpdateEntities");
    // Your code here
}

// Collect metrics
auto metrics = session->CollectMetrics();
std::cout << "FPS: " << metrics.fps << std::endl;
std::cout << "Frame time: " << metrics.frame_time_ms << " ms" << std::endl;

// Export results
session->ExportToJSON("profile_results.json");
session->ExportToChrome("chrome_trace.json"); // For chrome://tracing
```

### Key Metrics

- **Frame Time**: Target < 16.67ms for 60 FPS
- **CPU Usage**: Keep below 80% for responsiveness
- **Memory Usage**: Monitor for leaks and fragmentation
- **Cache Hit Rate**: Target > 90% for optimal performance
- **Draw Calls**: Minimize through batching

## Memory Optimization

### Object Pooling

```cpp
#include <ecscope/gui/memory_optimization.hpp>

// Get the global object pool for a type
auto& pool = MemoryOptimizer::Instance().GetObjectPool<MyWidget>();

// Acquire object from pool
MyWidget* widget = pool.Acquire(constructor_args...);

// Use the widget...

// Release back to pool (reused later)
pool.Release(widget);
```

### String Interning

```cpp
auto& interner = MemoryOptimizer::Instance().GetStringInterner();

// Intern frequently used strings
auto id = interner.Intern("frequently_used_string");

// Get string back (no allocation)
const std::string& str = interner.GetString(id);
```

### Memory Pressure Handling

```cpp
auto& pressure_handler = MemoryOptimizer::Instance().GetPressureHandler();

// Register cleanup callback
pressure_handler.RegisterCleanupCallback(
    "TextureCache",
    MemoryPriority::CACHE,
    [](PressureLevel level) -> size_t {
        // Free memory based on pressure level
        if (level >= PressureLevel::HIGH) {
            return texture_cache.Clear(); // Returns bytes freed
        }
        return texture_cache.Compact();
    }
);
```

### Memory Budget

```cpp
auto& budget = MemoryOptimizer::Instance().GetMemoryBudget();

MemoryBudget::Budget limits;
limits.total_mb = 512;
limits.ui_mb = 128;
limits.texture_mb = 256;
limits.cache_mb = 64;
budget.SetBudget(limits);

// Track allocations
budget.TrackAllocation("UI", widget_size);

// Check if within budget
if (!budget.IsWithinBudget("UI")) {
    // Trigger cleanup
}
```

## CPU Optimization

### Batch Rendering

```cpp
#include <ecscope/gui/cpu_gpu_optimization.hpp>

BatchRenderer renderer;
renderer.EnableInstancing(true);
renderer.EnableSorting(true);

renderer.BeginBatch();

// Add multiple quads (automatically batched)
for (const auto& element : ui_elements) {
    renderer.AddQuad(positions, uvs, color, texture_id);
}

renderer.EndBatch();
renderer.Flush(); // Minimized draw calls
```

### SIMD Optimization

```cpp
// Transform vertices using SIMD
simd::TransformVertices4x4_SSE(
    input_vertices, 
    output_vertices, 
    transform_matrix, 
    vertex_count
);

// Color conversion using SIMD
simd::ConvertRGBA8ToFloat_SSE(
    rgba_bytes, 
    float_colors, 
    pixel_count
);
```

### Parallel Command Generation

```cpp
ParallelCommandGenerator cmd_gen(std::thread::hardware_concurrency());

std::vector<CommandGenFunc> generators;
for (size_t i = 0; i < num_threads; ++i) {
    generators.push_back([](CommandBuffer& buffer, size_t thread_id) {
        buffer.Begin();
        // Generate commands for this thread's work
        buffer.End();
    });
}

cmd_gen.GenerateCommands(generators);
cmd_gen.ExecuteCommands();
```

## GPU Optimization

### Command Buffer Optimization

```cpp
CommandBuffer cmd_buffer;
cmd_buffer.Begin();

// State changes are automatically deduplicated
cmd_buffer.SetShader(shader_id);
cmd_buffer.SetTexture(0, texture_id);

// Batch similar draw calls
for (const auto& mesh : meshes) {
    cmd_buffer.DrawIndexed(mesh.index_count, mesh.index_offset);
}

cmd_buffer.End();
cmd_buffer.Optimize(); // Remove redundant commands
cmd_buffer.Execute();
```

### Render State Cache

```cpp
RenderStateCache state_cache;

// Only applies state if different from current
if (state_cache.SetShader(shader_id)) {
    // Shader was actually changed
}

// Get statistics
std::cout << "State changes: " << state_cache.GetStateChangeCount() << std::endl;
std::cout << "Redundant changes avoided: " << state_cache.GetRedundantStateChanges() << std::endl;
```

### Texture Streaming

```cpp
TextureStreamer streamer(256); // 256MB budget

// Request texture with priority
streamer.RequestTexture(
    "path/to/texture.png",
    TextureStreamer::Priority::HIGH,
    [](uint32_t texture_id) {
        // Texture loaded callback
    }
);

// Enable predictive loading
streamer.EnablePredictiveLoading(true);
streamer.TrackCameraMovement(camera_pos, camera_dir);
```

## Caching System

### Multi-Level Cache

```cpp
#include <ecscope/gui/caching_system.hpp>

// Define cache levels
std::vector<MultiLevelCache::Level> levels = {
    {100, 10*1024*1024, 100ms, EvictionPolicy::LRU},    // L1: Hot cache
    {1000, 50*1024*1024, 1000ms, EvictionPolicy::LFU},  // L2: Warm cache
    {10000, 200*1024*1024, 0ms, EvictionPolicy::SIZE}   // L3: Cold cache
};

MultiLevelCache<Key, Value> cache(levels);

// Cache operations
if (auto value = cache.Get(key)) {
    // Cache hit
} else {
    // Cache miss - compute and store
    auto computed = ComputeExpensiveValue(key);
    cache.Put(key, computed);
}
```

### Layout Cache

```cpp
auto& layout_cache = CacheManager::Instance().GetLayoutCache();

LayoutCache::LayoutKey key{
    element_id,
    container_width,
    container_height,
    constraints_hash
};

if (auto layout = layout_cache.Get(key)) {
    // Use cached layout
    ApplyLayout(*layout);
} else {
    // Compute and cache
    auto computed = ComputeLayout(element);
    layout_cache.Put(key, computed);
    ApplyLayout(computed);
}
```

### Lazy Loading

```cpp
LazyAsset<Texture> texture([path]() {
    return LoadTexture(path);
});

// First access triggers load
if (texture.IsLoaded() || texture.Get()) {
    RenderWithTexture(texture.Get());
}

// Async loading
LazyAsset<Model> model([path]() {
    return std::async(std::launch::async, [path]() {
        return LoadModel(path);
    });
});
```

## Platform-Specific Optimization

### Hardware Detection

```cpp
auto optimizer = PlatformOptimizer::Create();
auto capabilities = optimizer->DetectHardware();

std::cout << "CPU: " << capabilities.cpu_brand << std::endl;
std::cout << "Cores: " << capabilities.cpu_cores << std::endl;
std::cout << "GPU: " << capabilities.gpu_renderer << std::endl;
std::cout << "GPU Memory: " << capabilities.gpu_memory_mb << " MB" << std::endl;

// Check SIMD support
if (capabilities.has_avx2) {
    // Use AVX2 optimized paths
}
```

### Platform Hints

```cpp
auto rendering_hints = optimizer->GetRenderingHints();
if (rendering_hints.use_persistent_mapping) {
    // Use persistent mapped buffers
}

auto memory_hints = optimizer->GetMemoryHints();
if (memory_hints.use_large_pages) {
    // Allocate with large pages
}

auto threading_hints = optimizer->GetThreadingHints();
optimizer->SetThreadAffinity(
    std::this_thread::get_id(),
    threading_hints.ui_thread_cores
);
```

### Power Management

```cpp
// Request high performance mode
optimizer->RequestHighPerformance();

// Check battery status
if (optimizer->IsOnBattery()) {
    // Reduce quality settings
    float battery = optimizer->GetBatteryLevel();
    if (battery < 20.0f) {
        // Enable aggressive power saving
    }
}
```

## Auto-Tuning

### Performance Auto-Tuner

```cpp
PerformanceAutoTuner tuner;

// Set target profile
PerformanceAutoTuner::TuningProfile target;
target.target_fps = 60;
target.min_fps = 30;
target.max_memory_mb = 512;
tuner.SetTargetProfile(target);

// Enable auto-tuning
tuner.EnableAutoTuning(true);

// Set quality change callback
tuner.SetQualityChangeCallback([](const auto& profile) {
    graphics.SetTextureQuality(profile.texture_quality);
    graphics.SetShadowQuality(profile.shadow_quality);
    graphics.SetEffectQuality(profile.effect_quality);
});

// Update metrics each frame
tuner.UpdateMetrics(frame_time_ms, gpu_time_ms, memory_mb);
```

## Performance Budgets

```cpp
PerformanceBudget::Budget budget;
budget.frame_time_ms = 16.67f;  // 60 FPS
budget.memory_mb = 512;
budget.gpu_time_ms = 10.0f;
budget.draw_calls = 1000;
budget.triangles = 1000000;
budget.cache_hit_rate = 0.9f;

performance_budget.SetBudget(budget);

// Check budget
if (!performance_budget.CheckBudget(current_metrics)) {
    auto violations = performance_budget.GetBudgetViolations(current_metrics);
    for (const auto& violation : violations) {
        LOG_WARNING(violation);
    }
}
```

## Best Practices

### 1. Profile First
- Always profile before optimizing
- Focus on the biggest bottlenecks
- Use Chrome tracing for detailed timeline analysis

### 2. Memory Management
- Use object pools for frequently allocated objects
- Intern strings that are used repeatedly
- Set up memory budgets and stick to them
- Handle memory pressure gracefully

### 3. Batch Operations
- Batch draw calls whenever possible
- Sort by texture/shader to minimize state changes
- Use instancing for repeated geometry

### 4. Cache Aggressively
- Cache computed layouts
- Cache rendered text
- Cache style calculations
- Use multi-level caches with appropriate eviction

### 5. Platform Awareness
- Detect hardware capabilities at startup
- Use platform-specific optimizations
- Adjust quality based on hardware
- Handle thermal throttling

### 6. Threading
- Use parallel command generation
- Keep UI thread responsive
- Use worker threads for heavy computation
- Set appropriate thread priorities

### 7. GPU Optimization
- Minimize state changes
- Use persistent mapped buffers
- Stream textures based on visibility
- Use compute shaders where appropriate

## Troubleshooting

### High Frame Times
1. Check profiler for hotspots
2. Verify batching is working
3. Check for excessive allocations
4. Look for cache misses

### Memory Issues
1. Check for memory leaks
2. Monitor fragmentation
3. Verify pools are being used
4. Check cache sizes

### Platform-Specific Issues
1. Verify SIMD paths are used
2. Check thread affinity
3. Monitor thermal throttling
4. Check power management settings

## Benchmarking

Run the comprehensive benchmark:

```bash
./ui_performance_benchmark --entities 10000 --ui-elements 5000 --frames 1000
```

This will generate:
- Frame time statistics (avg, min, max, p95, p99)
- Memory usage and fragmentation
- Cache performance metrics
- CPU/GPU utilization
- Platform-specific metrics
- Performance grade (A+ to F)

Results are exported to:
- `benchmark_results.json` - Detailed JSON data
- `benchmark_results.csv` - CSV for analysis
- Console output with summary

## Performance Targets

### Minimum Requirements
- 30 FPS consistent (33.33ms frame time)
- < 256MB memory usage
- > 70% cache hit rate
- < 500 draw calls per frame

### Recommended Targets
- 60 FPS consistent (16.67ms frame time)
- < 512MB memory usage
- > 90% cache hit rate
- < 100 draw calls per frame
- < 5ms GPU time
- < 8ms CPU time

### Optimal Performance
- 120+ FPS (8.33ms frame time)
- < 256MB memory usage
- > 95% cache hit rate
- < 50 draw calls per frame
- < 3ms GPU time
- < 4ms CPU time

## Integration Example

```cpp
class OptimizedUIApplication {
public:
    void Initialize() {
        // Setup profiling
        m_profiler = std::make_unique<ProfilerSession>("MyApp");
        ProfilerSession::SetGlobalSession(m_profiler.get());
        
        // Setup platform optimization
        m_platform = PlatformOptimizer::Create();
        auto caps = m_platform->DetectHardware();
        
        // Setup memory optimization
        auto& mem_opt = MemoryOptimizer::Instance();
        mem_opt.SetMemoryLimit(caps.total_memory_mb / 4);
        
        // Setup caching
        auto& cache_mgr = CacheManager::Instance();
        cache_mgr.SetMemoryBudget(128);
        
        // Setup auto-tuning
        m_tuner.SetTargetProfile({60, 30, 10.0f, 8.0f});
        m_tuner.EnableAutoTuning(true);
        
        // Setup batch renderer
        m_renderer = std::make_unique<BatchRenderer>();
        m_renderer->EnableInstancing(caps.has_instancing);
    }
    
    void Frame() {
        PROFILE_SCOPE("Frame");
        
        m_profiler->GetFrameProfiler().BeginFrame();
        
        Update();
        Render();
        
        m_profiler->GetFrameProfiler().EndFrame();
        
        // Auto-tune quality
        auto metrics = m_profiler->CollectMetrics();
        m_tuner.UpdateMetrics(
            metrics.frame_time_ms,
            metrics.gpu_time_ms,
            metrics.memory_allocated_bytes / (1024*1024)
        );
    }
    
private:
    std::unique_ptr<ProfilerSession> m_profiler;
    std::unique_ptr<PlatformOptimizer> m_platform;
    std::unique_ptr<BatchRenderer> m_renderer;
    PerformanceAutoTuner m_tuner;
};
```

## Conclusion

The ECScope UI performance optimization system provides all the tools needed to achieve professional-grade performance across all platforms. By following the best practices and using the provided optimization systems, you can ensure your UI maintains consistent 60+ FPS while minimizing resource usage.