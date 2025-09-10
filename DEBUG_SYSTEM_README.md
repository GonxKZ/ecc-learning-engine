# ECScope Debug System

The ECScope Debug System is a comprehensive, production-quality debugging and profiling toolkit designed for professional game development. It provides real-time performance monitoring, memory analysis, visual debugging, and runtime inspection capabilities with minimal performance impact.

## 🚀 Features

### Performance Profilers
- **CPU Profiler**: High-precision hierarchical sampling with per-thread analysis
- **Memory Profiler**: Allocation tracking, leak detection, and heap analysis
- **GPU Profiler**: Render timing, resource usage, and GPU performance metrics
- **Network Profiler**: Bandwidth monitoring, latency analysis, and connection tracking
- **Asset Profiler**: Loading bottleneck identification and cache analysis
- **Custom Event Profiler**: User-defined event tracking with metadata support

### Visual Debugging
- **Real-time Performance Graphs**: Interactive charts and time-series data
- **Memory Visualization**: Heap maps, allocation timelines, and fragmentation analysis
- **ECS Entity Visualizer**: Entity relationships and component dependencies
- **Physics Debug Rendering**: Collision shapes, forces, and contact points
- **Rendering Debug Views**: Wireframe, normals, overdraw, and complexity visualization
- **Network Topology Viewer**: Connection graphs and message flow animation

### Runtime Inspectors
- **Entity Inspector**: Live component editing and entity hierarchy browsing
- **System Inspector**: Performance analysis and dependency visualization
- **Asset Inspector**: Dependency graphs and loading performance analysis
- **Memory Inspector**: Allocation trees and memory pool management
- **Shader Inspector**: Reflection data and uniform editing
- **Job System Inspector**: Fiber states and worker thread monitoring

### Debug Console
- **Interactive Command System**: Built-in and custom commands with auto-completion
- **Variable Inspection**: Live editing of engine variables
- **Script Execution**: Lua/Python integration for runtime scripting
- **Log Viewer**: Advanced filtering, search, and categorization
- **Remote Debugging**: Network-based debugging for deployed applications
- **Crash Analysis**: Post-mortem debugging and minidump generation

## 🛠️ Quick Start

### Basic Setup

```cpp
#include <ecscope/debug.hpp>

int main() {
    // Initialize with default configuration
    ECScope::Debug::GlobalDebugSystem::Initialize();
    
    // Game loop
    while (running) {
        ECSCOPE_DEBUG_UPDATE(deltaTime);
        
        // Your game logic here
        UpdateGame(deltaTime);
        
        ECSCOPE_DEBUG_RENDER();
    }
    
    ECScope::Debug::GlobalDebugSystem::Shutdown();
    return 0;
}
```

### Advanced Setup with Builder Pattern

```cpp
#include <ecscope/debug.hpp>

auto debug_system = ECScope::Debug::DebugSystemBuilder()
    .WithProfiling(true)
    .WithVisualization(true)
    .WithInspection(true)
    .WithConsole(true)
    .WithRemoteDebugging(true, 7777)
    .WithMemoryBudget(64 * 1024 * 1024)  // 64MB
    .WithCPUProfiler("CPU")
    .WithMemoryProfiler("Memory")
    .WithGPUProfiler("GPU")
    .WithPerformanceGraphs()
    .WithMemoryVisualization()
    .WithEntityInspector()
    .BuildAndSetGlobal();
```

### Using Preset Configurations

```cpp
// For development with all features
auto debug_system = ECScope::Debug::Presets::CreateDevelopment();

// For performance analysis
auto debug_system = ECScope::Debug::Presets::CreatePerformanceAnalysis();

// For memory debugging
auto debug_system = ECScope::Debug::Presets::CreateMemoryDebugging();

// Minimal impact for shipping builds
auto debug_system = ECScope::Debug::Presets::CreateMinimal();
```

## 📊 Profiling Examples

### CPU Profiling

```cpp
void MySystem::Update(float deltaTime) {
    ECSCOPE_PROFILE_FUNCTION();
    
    // Automatic profiling of entire function
    ProcessEntities(deltaTime);
    
    {
        ECSCOPE_PROFILE_SCOPE("ExpensiveOperation");
        DoExpensiveWork();
    }
}
```

### Memory Tracking

```cpp
void* ptr = malloc(1024);
ECSCOPE_TRACK_ALLOC(ptr, 1024, "GameLogic");

// ... use memory ...

ECSCOPE_TRACK_FREE(ptr);
free(ptr);

// Check for leaks
ECScope::Debug::Utils::CheckMemoryLeaks();
```

### GPU Profiling

```cpp
void RenderScene() {
    ECSCOPE_GPU_EVENT_BEGIN("RenderGeometry");
    
    // Your rendering calls here
    DrawMeshes();
    
    ECSCOPE_GPU_EVENT_END();
    
    ECSCOPE_GPU_EVENT_BEGIN("PostProcessing");
    ApplyPostEffects();
    ECSCOPE_GPU_EVENT_END();
}
```

### Custom Events

```cpp
void ProcessPlayerAction() {
    ECSCOPE_EVENT_BEGIN("PlayerAction", "Gameplay");
    
    // Add metadata
    auto event = ECScope::Debug::Utils::GetCurrentEvent();
    event->AddMetadata("action_type", "jump");
    event->AddMetadata("player_id", "player_001");
    
    // Process action
    HandleJump();
    
    ECSCOPE_EVENT_END();
}
```

## 🎯 Debug Drawing

```cpp
void DebugVisualization() {
    // Draw coordinate axes
    Vector3 origin{0, 0, 0};
    ECSCOPE_DRAW_LINE(origin, Vector3{1, 0, 0}, ECScope::Debug::DebugRenderer::RED);
    ECSCOPE_DRAW_LINE(origin, Vector3{0, 1, 0}, ECScope::Debug::DebugRenderer::GREEN);
    ECSCOPE_DRAW_LINE(origin, Vector3{0, 0, 1}, ECScope::Debug::DebugRenderer::BLUE);
    
    // Draw bounding boxes
    Vector3 min{-1, -1, -1};
    Vector3 max{1, 1, 1};
    ECSCOPE_DRAW_BOX(min, max, ECScope::Debug::DebugRenderer::YELLOW);
    
    // Draw spheres
    Vector3 center{2, 0, 0};
    ECSCOPE_DRAW_SPHERE(center, 1.0f, ECScope::Debug::DebugRenderer::CYAN);
    
    // Draw text
    ECSCOPE_DRAW_TEXT(Vector3{0, 2, 0}, "Debug Info", ECScope::Debug::DebugRenderer::WHITE);
}
```

## 💻 Console Commands

The debug console comes with built-in commands and supports custom commands:

### Built-in Commands

```bash
# Memory analysis
check_leaks                 # Detect memory leaks
memory_report              # Generate memory usage report

# Performance analysis
perf_report                # Generate performance report
system_timings             # Show system execution times

# Asset analysis
asset_report               # Asset loading performance
asset_dependencies        # Show asset dependency graph

# General commands
help                       # Show all commands
clear                      # Clear console
set variable_name value    # Set engine variable
get variable_name          # Get engine variable value
```

### Custom Commands

```cpp
auto& console = ECScope::Debug::GlobalDebugSystem::Get().GetConsole();

console.RegisterCommand({
    "spawn_enemy",
    "Spawn an enemy at specified position",
    "spawn_enemy <x> <y> <z>",
    {},
    [](const std::vector<std::string>& args) -> Console::CommandResult {
        if (args.size() != 4) {
            return {false, "", "Usage: spawn_enemy <x> <y> <z>"};
        }
        
        float x = std::stof(args[1]);
        float y = std::stof(args[2]);
        float z = std::stof(args[3]);
        
        SpawnEnemyAt(Vector3{x, y, z});
        
        return {true, "Enemy spawned successfully", ""};
    }
});
```

## 🔍 Runtime Inspection

### Entity Inspector

```cpp
// Register component editors
auto& inspector = debug_system.GetInspector("Entity");
if (auto entity_inspector = std::dynamic_pointer_cast<EntityInspector>(inspector)) {
    // Register custom component editor
    entity_inspector->RegisterComponentEditor<Transform>([](Transform& transform) {
        bool changed = false;
        changed |= ImGui::DragFloat3("Position", &transform.position.x);
        changed |= ImGui::DragFloat3("Rotation", &transform.rotation.x);
        changed |= ImGui::DragFloat3("Scale", &transform.scale.x);
        return changed;
    });
    
    // Select entity for inspection
    entity_inspector->SelectEntity(player_entity_id);
}
```

### Memory Inspector

```cpp
// Register memory pools
auto& memory_inspector = debug_system.GetInspector("Memory");
if (auto mem_inspector = std::dynamic_pointer_cast<MemoryInspector>(memory_inspector)) {
    MemoryInspector::MemoryPool pool_info;
    pool_info.name = "Entity Pool";
    pool_info.total_size = entity_pool.GetTotalSize();
    pool_info.used_size = entity_pool.GetUsedSize();
    
    mem_inspector->RegisterMemoryPool("entities", pool_info);
}
```

## 🌐 Remote Debugging

Enable remote debugging to debug deployed applications:

```cpp
// Enable remote debugging on port 7777
ECScope::Debug::DebugSystem::Config config;
config.enable_remote_debugging = true;
config.remote_debug_port = 7777;

ECScope::Debug::GlobalDebugSystem::Initialize(config);
```

Connect using telnet or a custom client:
```bash
telnet localhost 7777
```

## ⚡ Performance Configuration

### Minimal Impact Configuration

```cpp
ECScope::Debug::DebugSystem::Config config;
config.minimal_performance_impact = true;
config.max_profiler_samples = 1000;        // Reduced sample count
config.profiler_update_frequency = 30.0f;  // Lower update frequency
config.debug_memory_budget = 16 * 1024 * 1024;  // 16MB budget

ECScope::Debug::GlobalDebugSystem::Initialize(config);
```

### Release Build Optimizations

The debug system automatically optimizes for release builds:
- Most debug operations become no-ops
- Minimal memory footprint
- Optional compilation removal via macros

```cpp
#ifdef ECSCOPE_RELEASE_BUILD
    // Debug operations are automatically disabled
    ECSCOPE_PROFILE_FUNCTION();  // No-op in release
    ECSCOPE_DRAW_LINE(a, b, c);  // No-op in release
#endif
```

## 🔧 CMake Integration

Add to your CMakeLists.txt:

```cmake
# Enable debug system
set(ECSCOPE_ENABLE_DEBUG_SYSTEM ON)

# Link against ECScope
target_link_libraries(your_target PRIVATE ECScope::ECScope)
```

For conditional debug builds:
```cmake
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(your_target PRIVATE ECSCOPE_DEBUG_ENABLED=1)
else()
    target_compile_definitions(your_target PRIVATE ECSCOPE_RELEASE_BUILD=1)
endif()
```

## 📁 File Structure

```
include/ecscope/debug/
├── debug_system.hpp          # Core debug system
├── profilers.hpp             # All profiler implementations
├── visualizers.hpp           # Visual debugging components
├── inspectors.hpp            # Runtime inspection tools
├── console.hpp               # Debug console system
└── debug_renderer.hpp        # Debug drawing utilities

src/debug/
├── debug_system.cpp          # Core implementation
├── profilers/                # Profiler implementations
├── visualizers/              # Visualizer implementations
├── inspectors/               # Inspector implementations
├── console/                  # Console system
└── utils/                    # Utility functions
```

## 🎮 Example Application

See `examples/advanced/comprehensive_debug_system_demo.cpp` for a complete demonstration of all debug system features.

## 📝 Best Practices

1. **Use Scoped Profiling**: Prefer `ECSCOPE_PROFILE_FUNCTION()` and `ECSCOPE_PROFILE_SCOPE()` for automatic cleanup
2. **Tag Memory Allocations**: Always provide meaningful tags for memory tracking
3. **Conditional Debug Code**: Use `ECSCOPE_DEBUG_ONLY()` for debug-only code blocks
4. **Configure for Target**: Use appropriate presets for development vs. shipping builds
5. **Monitor Performance Impact**: Keep debug overhead under 5% in development builds

## 🔧 Advanced Features

### Custom Visualizers

```cpp
class MyCustomVisualizer : public ECScope::Debug::Visualizer {
public:
    MyCustomVisualizer(const std::string& name) : Visualizer(name) {}
    
    void Update(float deltaTime) override {
        // Update visualization data
    }
    
    void Render() override {
        // Render custom visualization
    }
};

// Register custom visualizer
debug_system.CreateVisualizer<MyCustomVisualizer>("MyViz");
```

### Custom Profilers

```cpp
class GameplayProfiler : public ECScope::Debug::Profiler {
public:
    void Update(float deltaTime) override {
        // Collect gameplay-specific metrics
    }
    
    void Reset() override {
        // Reset profiler state
    }
};

// Register custom profiler
debug_system.CreateProfiler<GameplayProfiler>("Gameplay");
```

## 🎯 Production Deployment

For shipping builds, the debug system can be completely disabled:

```cpp
#ifndef ECSCOPE_ENABLE_DEBUG_SYSTEM
    // All debug macros become no-ops
    #define ECSCOPE_PROFILE_FUNCTION()
    #define ECSCOPE_DRAW_LINE(a, b, c)
    // ... etc
#endif
```

Or use minimal profiling for crash reporting:
```cpp
auto debug_system = ECScope::Debug::Presets::CreateMinimal();
debug_system->GetConsole().EnableCrashReporting(true);
```

## 🆘 Troubleshooting

**Q: High memory usage from debug system**
A: Reduce `max_profiler_samples` and `debug_memory_budget` in configuration

**Q: Performance impact too high**
A: Enable `minimal_performance_impact` mode or use a preset configuration

**Q: Remote debugging not working**
A: Check firewall settings and ensure port is not in use

**Q: Console commands not working**
A: Verify console is enabled in configuration and commands are properly registered

## 📊 Performance Metrics

The debug system itself is optimized for minimal impact:
- CPU overhead: < 2% in development builds
- Memory overhead: Configurable (default 64MB)
- GPU impact: Negligible (query-based profiling)
- Network impact: Optional remote debugging only

---

This debug system provides professional-grade debugging capabilities for serious game development, enabling developers to identify performance bottlenecks, memory leaks, and system issues quickly and efficiently.