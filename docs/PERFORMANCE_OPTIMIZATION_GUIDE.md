# ECScope UI Performance Optimization Guide

## Overview

This guide provides comprehensive best practices and strategies for optimizing UI performance in the ECScope game engine. The optimization system targets 60+ FPS consistent frame rates across Windows, Linux, and macOS platforms.

## Performance Architecture

### Core Components

1. **Performance Profiler** - Real-time metrics collection
2. **Memory Optimizer** - Object pooling and caching
3. **Batch Renderer** - Draw call minimization
4. **Platform Optimizer** - Hardware-specific optimizations
5. **Auto Optimizer** - Dynamic quality adjustment

## Performance Targets

### Recommended Targets
- **Frame Rate**: 60+ FPS (16.67ms frame time)
- **Memory Usage**: < 512MB for UI systems
- **CPU Usage**: < 50% on UI thread
- **GPU Usage**: < 80% for UI rendering
- **Draw Calls**: < 1000 per frame
- **Cache Hit Rate**: > 85%

## Optimization Strategies

### 1. Rendering Optimization

#### Batching
```cpp
// Enable batching for similar UI elements
auto& batch_renderer = PERF_OPTIMIZE().GetBatchRenderer();
batch_renderer.EnableInstancing(true);
batch_renderer.SetMaxBatchSize(65536, 98304);

// Batch multiple quads
batch_renderer.BeginBatch();
for (const auto& widget : widgets) {
    batch_renderer.AddQuad(positions, uvs, color, texture_id);
}
batch_renderer.EndBatch();
batch_renderer.Flush();
```

#### Occlusion Culling
```cpp
// Skip rendering of hidden elements
if (!IsVisible(widget.bounds)) {
    continue;
}
```

#### Texture Atlasing
```cpp
// Combine multiple small textures
auto& atlas = TextureAtlas(2048, 2048, RGBA8);
auto region = atlas.AddTexture(icon_data, 64, 64);
// Use region.u0, v0, u1, v1 for rendering
```

### 2. Memory Optimization

#### Object Pooling
```cpp
// Reuse UI objects instead of allocating new ones
auto& pool = memory::MemoryOptimizer::Instance().GetObjectPool<Widget>();

// Acquire from pool
Widget* widget = pool.Acquire();

// Release back to pool
pool.Release(widget);
```

#### String Interning
```cpp
// Share string instances
auto& interner = memory::MemoryOptimizer::Instance().GetStringInterner();
auto string_id = interner.Intern("Frequently Used String");
```

#### Lazy Loading
```cpp
// Load resources only when needed
LazyLoader<Texture> texture([](){ 
    return LoadTexture("path/to/texture.png");
});

// Access triggers loading
texture->Bind();
```

### 3. CPU Optimization

#### SIMD Operations
```cpp
// Use SIMD for batch transforms
optimization::simd::TransformVertices4x4_SSE(
    vertices, output, matrix, vertex_count
);
```

#### Multi-threading
```cpp
// Parallel command generation
ParallelCommandGenerator cmd_gen(std::thread::hardware_concurrency());
cmd_gen.GenerateCommands(command_generators);
```

#### Command Buffering
```cpp
// Record commands once, execute multiple times
CommandBuffer cmd_buffer;
cmd_buffer.Begin();
cmd_buffer.SetShader(shader_id);
cmd_buffer.Draw(vertex_count);
cmd_buffer.End();
cmd_buffer.Execute();
```

### 4. Caching Strategies

#### Multi-level Cache
```cpp
// Configure cache levels
MultiLevelCache<Key, Value> cache({
    {1000, 10_MB, 100ms, EvictionPolicy::LRU},   // L1: Fast, small
    {10000, 100_MB, 1s, EvictionPolicy::LFU},    // L2: Larger, slower
    {100000, 1_GB, 10s, EvictionPolicy::FIFO}    // L3: Disk cache
});
```

#### Cache Warming
```cpp
// Preload frequently used resources
cache.Preload({"ui_background", "button_normal", "button_hover"});
```

### 5. Platform-Specific Optimizations

#### Hardware Detection
```cpp
auto& platform = PERF_OPTIMIZE().GetPlatformOptimizer();
auto hw_caps = platform.DetectHardware();

if (hw_caps.has_avx2) {
    // Use AVX2 optimized paths
}

if (hw_caps.gpu_memory_mb > 4096) {
    // Enable high-quality textures
}
```

#### Memory Alignment
```cpp
// Align to cache line for better performance
void* data = platform.AllocateAligned(size, hw_caps.cache_line_size);
```

## Dynamic Quality Adjustment

### Auto-Optimization
```cpp
// Enable adaptive quality
auto& auto_opt = PERF_OPTIMIZE().GetAutoOptimizer();
auto_opt.SetOptimizationLevel(OptimizationLevel::ADAPTIVE);
auto_opt.EnableDynamicResolution(true);
auto_opt.EnableDynamicLOD(true);

// Set performance targets
PerformanceTargets targets;
targets.target_fps = 60.0f;
targets.min_fps = 30.0f;
targets.max_memory_mb = 512;
auto_opt.SetTargets(targets);
```

### Quality Levels
1. **OFF** - No optimizations
2. **CONSERVATIVE** - Safe optimizations only
3. **BALANCED** - Default balanced settings
4. **AGGRESSIVE** - Maximum performance
5. **ADAPTIVE** - Dynamic adjustment based on metrics

## Performance Monitoring

### Real-time Monitoring
```cpp
// Enable performance overlay
auto& monitor = PERF_OPTIMIZE().GetMonitor();
monitor.SetVisible(true);
monitor.SetPosition(10, 10);

// Set alert thresholds
monitor.SetAlertThreshold("fps", 30.0f);
monitor.SetAlertThreshold("memory", 512.0f);
```

### Profiling Scopes
```cpp
void RenderUI() {
    PROFILE_SCOPE("UI::Render");
    
    {
        PROFILE_SCOPE("UI::Layout");
        CalculateLayout();
    }
    
    {
        PROFILE_SCOPE("UI::Draw");
        DrawWidgets();
    }
}
```

## Benchmarking

### Running Benchmarks
```bash
# Run with different optimization levels
./ui_performance_benchmark --balanced
./ui_performance_benchmark --aggressive
./ui_performance_benchmark --adaptive
```

### Custom Benchmarks
```cpp
PERF_BENCHMARK("Custom Operation", {
    // Code to benchmark
    for (int i = 0; i < 1000; ++i) {
        PerformExpensiveOperation();
    }
});
```

## Memory Management Best Practices

### 1. Avoid Frequent Allocations
```cpp
// Bad
for (int i = 0; i < 1000; ++i) {
    Widget* w = new Widget();
    // ...
    delete w;
}

// Good
auto& pool = GetObjectPool<Widget>();
for (int i = 0; i < 1000; ++i) {
    Widget* w = pool.Acquire();
    // ...
    pool.Release(w);
}
```

### 2. Use Move Semantics
```cpp
// Bad
std::vector<LargeObject> objects;
objects.push_back(CreateLargeObject()); // Copy

// Good
std::vector<LargeObject> objects;
objects.push_back(std::move(CreateLargeObject())); // Move
```

### 3. Reserve Container Capacity
```cpp
// Bad
std::vector<Widget> widgets;
for (int i = 0; i < 1000; ++i) {
    widgets.push_back(Widget()); // Multiple reallocations
}

// Good
std::vector<Widget> widgets;
widgets.reserve(1000); // Single allocation
for (int i = 0; i < 1000; ++i) {
    widgets.emplace_back();
}
```

## Rendering Best Practices

### 1. Minimize State Changes
```cpp
// Bad
for (const auto& object : objects) {
    SetShader(object.shader);
    SetTexture(object.texture);
    Draw(object);
}

// Good
// Sort by shader and texture first
std::sort(objects.begin(), objects.end(), [](auto& a, auto& b) {
    return std::tie(a.shader, a.texture) < std::tie(b.shader, b.texture);
});

uint32_t current_shader = 0;
uint32_t current_texture = 0;
for (const auto& object : objects) {
    if (object.shader != current_shader) {
        SetShader(object.shader);
        current_shader = object.shader;
    }
    if (object.texture != current_texture) {
        SetTexture(object.texture);
        current_texture = object.texture;
    }
    Draw(object);
}
```

### 2. Use Instancing
```cpp
// Bad - Individual draw calls
for (const auto& instance : instances) {
    DrawMesh(mesh, instance.transform);
}

// Good - Instanced rendering
DrawMeshInstanced(mesh, instance_transforms, instance_count);
```

### 3. Batch Similar Operations
```cpp
// Bad
DrawText("Hello", x1, y1);
DrawText("World", x2, y2);
DrawText("!", x3, y3);

// Good
TextBatch batch;
batch.Add("Hello", x1, y1);
batch.Add("World", x2, y2);
batch.Add("!", x3, y3);
batch.Render();
```

## Common Performance Issues

### Issue: High Draw Call Count
**Symptoms**: Low FPS, high CPU usage
**Solution**: Enable batching, use texture atlases, instance rendering

### Issue: Memory Fragmentation
**Symptoms**: Increasing memory usage over time
**Solution**: Use object pools, enable memory compaction

### Issue: Cache Misses
**Symptoms**: Stuttering, inconsistent frame times
**Solution**: Optimize data layout, use cache-friendly algorithms

### Issue: GPU Bottleneck
**Symptoms**: GPU usage at 100%, low FPS
**Solution**: Reduce resolution, simplify shaders, enable LOD

### Issue: CPU Bottleneck
**Symptoms**: CPU usage at 100%, GPU idle
**Solution**: Enable multi-threading, optimize algorithms, use SIMD

## Regression Testing

### Automated Testing
```cpp
// Set baseline performance
auto& regression_detector = PERF_OPTIMIZE().GetRegressionDetector();
regression_detector.SetBaseline(current_metrics);

// Run tests
RunPerformanceTests();

// Check for regressions
if (regression_detector.HasRegressions()) {
    for (const auto& regression : regression_detector.GetRegressions()) {
        LOG_ERROR("Performance regression: {} is {}% slower", 
                  regression.metric_name, regression.change_percent);
    }
}
```

### CI Integration
```yaml
# .github/workflows/performance.yml
- name: Run Performance Tests
  run: |
    ./ui_performance_benchmark --aggressive
    if [ $? -ne 0 ]; then
      echo "Performance regression detected!"
      exit 1
    fi
```

## Platform-Specific Tips

### Windows
- Use Direct3D 12 for best performance
- Enable GPU scheduling in Windows 10/11
- Use WDDM 2.7+ drivers

### Linux
- Use Vulkan for best performance
- Enable huge pages for large allocations
- Use real-time kernel for consistent timing

### macOS
- Use Metal for best performance
- Enable automatic graphics switching
- Profile with Instruments

## Debugging Performance Issues

### 1. Profile First
Always profile before optimizing:
```cpp
PROFILE_SCOPE("Suspected Slow Function");
```

### 2. Use Performance Monitor
```cpp
monitor.StartRecording("performance_trace.json");
// Reproduce issue
monitor.StopRecording();
```

### 3. Analyze Bottlenecks
```cpp
auto analysis = PERF_OPTIMIZE().GetAnalyzer().GetAnalysis();
LOG_INFO("Bottleneck: {}", analysis.primary_bottleneck);
for (const auto& suggestion : analysis.suggestions) {
    LOG_INFO("Suggestion: {}", suggestion);
}
```

## Conclusion

Achieving optimal UI performance requires a combination of:
- Efficient rendering techniques
- Smart memory management
- Effective caching strategies
- Platform-specific optimizations
- Continuous monitoring and adjustment

Use the provided tools and follow these best practices to ensure your UI runs at peak performance across all platforms.