# ECScope Scripting Integration Guide

## Overview

ECScope provides a world-class scripting integration system that enables seamless Python and Lua scripting within the ECS framework. This system is designed with both production performance and educational value in mind, offering comprehensive tools for game development, system prototyping, and learning advanced programming concepts.

## Table of Contents

1. [Quick Start](#quick-start)
2. [Python Integration](#python-integration)
3. [Lua Integration](#lua-integration)
4. [ECS Scripting Interface](#ecs-scripting-interface)
5. [Hot-Reload System](#hot-reload-system)
6. [Performance Profiling](#performance-profiling)
7. [Advanced Features](#advanced-features)
8. [Best Practices](#best-practices)
9. [Troubleshooting](#troubleshooting)
10. [API Reference](#api-reference)

## Quick Start

### Basic Setup

```cpp
#include "scripting/python_integration.hpp"
#include "scripting/ecs_script_interface.hpp"
#include "ecs/registry.hpp"
#include "memory/advanced_memory_system.hpp"

// Initialize core systems
auto memory_system = std::make_unique<memory::AdvancedMemorySystem>();
auto registry = std::make_unique<ecs::Registry>();

// Initialize Python engine
auto python_engine = std::make_unique<scripting::python::PythonEngine>(*memory_system);
if (!python_engine->initialize()) {
    LOG_ERROR("Failed to initialize Python engine");
    return false;
}

// Create ECS scripting interface
auto ecs_interface = std::make_unique<scripting::ecs_interface::ECSScriptInterface>(
    registry.get(), python_engine.get()
);
```

### Your First Script

```python
# hello_ecscope.py
print("Hello, ECScope!")

def calculate_distance(x1, y1, x2, y2):
    """Calculate distance between two points."""
    return ((x2 - x1)**2 + (y2 - y1)**2)**0.5

# Use the function
distance = calculate_distance(0, 0, 3, 4)
print(f"Distance: {distance}")
```

```cpp
// Execute the script
auto result = python_engine->execute_file("hello_ecscope.py");
if (!result) {
    LOG_ERROR("Script execution failed");
}
```

## Python Integration

### Core Features

The Python integration system provides:

- **Automatic Memory Management**: Custom allocators integrate with ECScope's memory system
- **Exception Handling**: Comprehensive error reporting with stack traces
- **Type Safety**: Automatic type checking and conversion
- **Performance Monitoring**: Built-in profiling and optimization suggestions

### Python Engine Configuration

```cpp
// Create Python engine with custom memory system
scripting::python::PythonEngine python_engine(*memory_system);

// Initialize with custom Python path
python_engine.add_to_sys_path("./scripts");
python_engine.add_to_sys_path("./game_logic");

// Load required modules
python_engine.load_module("math");
python_engine.load_module("json");
```

### Memory Management

The Python integration uses ECScope's advanced memory system:

```cpp
// Memory statistics
auto memory_stats = python_engine.get_memory_manager().get_statistics();
LOG_INFO("Python memory usage: {} KB", memory_stats.current_allocated / 1024);
LOG_INFO("Peak usage: {} KB", memory_stats.peak_memory / 1024);

// Check for memory leaks
auto allocation_report = python_engine.get_memory_manager().get_allocation_report();
for (const auto& alloc : allocation_report) {
    LOG_INFO("Allocation: {} bytes at {}:{}", alloc.size, alloc.filename, alloc.line_number);
}
```

### Exception Handling

```cpp
auto& exception_handler = python_engine.get_exception_handler();

auto result = python_engine.execute_string("invalid_syntax ::::");
if (!result) {
    auto exception_info = exception_handler.get_current_exception();
    LOG_ERROR("Python error [{}] in {}:{}: {}", 
             exception_info.type, 
             exception_info.script_file,
             exception_info.line_number, 
             exception_info.message);
}
```

### Component Binding

Register your components for Python access:

```cpp
struct Position {
    f32 x, y, z;
};

struct Velocity {
    f32 dx, dy, dz;
};

// Register components
python_engine.register_component<Position>("Position");
python_engine.register_component<Velocity>("Velocity");
```

Then use them in Python:

```python
# Create and manipulate components
position = Position()
position.x = 10.0
position.y = 5.0
position.z = 0.0

velocity = Velocity()
velocity.dx = 1.0
velocity.dy = 0.0
velocity.dz = 0.0

# Components are automatically synchronized with the ECS
```

## Lua Integration

### LuaJIT Performance

ECScope's Lua integration uses LuaJIT for maximum performance:

```cpp
#include "scripting/lua_integration.hpp"

// Initialize Lua engine
auto lua_engine = std::make_unique<scripting::lua::LuaEngine>(*memory_system);
if (!lua_engine->initialize()) {
    LOG_ERROR("Failed to initialize Lua engine");
}
```

### Coroutine System

Lua's coroutines are perfect for game logic:

```lua
-- game_ai.lua
function ai_behavior(entity)
    while entity:is_valid() do
        -- Patrol behavior
        move_to_position(entity, get_patrol_point())
        coroutine.yield()
        
        -- Look around
        for i = 1, 10 do
            rotate(entity, math.pi / 10)
            coroutine.yield()
        end
        
        -- Check for threats
        if detect_threat(entity) then
            -- Switch to combat behavior
            combat_ai(entity)
        end
    end
end
```

```cpp
// C++ coroutine management
auto& scheduler = lua_engine->get_coroutine_scheduler();

// Create AI coroutine for each enemy
for (auto* enemy : enemies) {
    u32 coroutine_id = scheduler.create_coroutine(lua_state, "ai_" + std::to_string(enemy->id()));
    scheduler.start_coroutine(coroutine_id, "ai_behavior", false);
}

// Update coroutines each frame
scheduler.update();
```

### Memory Integration

```cpp
// Lua memory statistics
auto& memory_manager = lua_engine->get_memory_manager();
auto stats = memory_manager.get_statistics();

LOG_INFO("Lua memory - Total: {} KB, Peak: {} KB", 
         stats.current_allocated / 1024, 
         stats.peak_memory / 1024);
```

## ECS Scripting Interface

### Entity Management

```cpp
// Create ECS scripting interface
auto ecs_interface = std::make_unique<scripting::ecs_interface::ECSScriptInterface>(
    registry.get(), python_engine.get(), lua_engine.get()
);

// Register component types
ecs_interface->register_component_type<Position>("Position");
ecs_interface->register_component_type<Velocity>("Velocity");
ecs_interface->register_component_type<Health>("Health");
```

### Script Entity Operations

```python
# Python script for entity manipulation
import ecscope_ecs as ecs

# Create entities
player = ecs.create_entity()
enemy = ecs.create_entity()

# Add components
player.add_component("Position", x=0.0, y=0.0, z=0.0)
player.add_component("Velocity", dx=1.0, dy=0.0, dz=0.0)
player.add_component("Health", current=100, maximum=100)

enemy.add_component("Position", x=10.0, y=5.0, z=0.0)
enemy.add_component("Health", current=50, maximum=50)

# Query entities
moving_entities = ecs.query_entities("Position", "Velocity")
for entity in moving_entities:
    pos = entity.get_component("Position")
    vel = entity.get_component("Velocity")
    
    # Update position
    pos.x += vel.dx * delta_time
    pos.y += vel.dy * delta_time
    pos.z += vel.dz * delta_time

# Remove components
enemy.remove_component("Health")

# Destroy entities
enemy.destroy()
```

### Query System

The query system provides efficient entity filtering:

```cpp
// Create queries
auto moving_query = ecs_interface->create_query<Position, Velocity>();
auto damaged_query = ecs_interface->create_query<Health>();

// Use queries
moving_query->for_each([](auto& entity, Position& pos, Velocity& vel) {
    pos.x += vel.dx * 0.016f;
    pos.y += vel.dy * 0.016f;
    pos.z += vel.dz * 0.016f;
});

// Parallel processing
moving_query->for_each_parallel([](auto& entity, Position& pos, Velocity& vel) {
    // Physics update
    apply_physics(pos, vel);
}, job_system.get());
```

### System Creation

Create systems directly from scripts:

```python
# movement_system.py
class MovementSystem:
    def __init__(self):
        self.name = "MovementSystem"
        self.execution_order = "Update"
    
    def update(self, delta_time):
        # Get all entities with position and velocity
        moving_entities = ecs.query_entities("Position", "Velocity")
        
        for entity in moving_entities:
            pos = entity.get_component("Position")
            vel = entity.get_component("Velocity")
            
            # Basic physics integration
            pos.x += vel.dx * delta_time
            pos.y += vel.dy * delta_time
            pos.z += vel.dz * delta_time
            
            # Apply damping
            vel.dx *= 0.99
            vel.dy *= 0.99
            vel.dz *= 0.99
            
            # Boundary checking
            if abs(pos.x) > 100:
                vel.dx *= -1
            if abs(pos.y) > 100:
                vel.dy *= -1

# Register system
movement_system = MovementSystem()
ecs.register_system(movement_system)
```

## Hot-Reload System

### File Monitoring

The hot-reload system automatically detects script changes:

```cpp
#include "scripting/hot_reload_system.hpp"

// Create hot-reload system
auto hot_reload = std::make_unique<scripting::hotreload::HotReloadSystem>(
    python_engine.get(), lua_engine.get()
);

// Add watch directories
hot_reload->add_watch_directory("./scripts", true, "*.py");
hot_reload->add_watch_directory("./game_logic", true, "*.lua");

// Start monitoring
hot_reload->start_monitoring();
```

### Dependency Tracking

The system automatically tracks script dependencies:

```cpp
auto& dependency_tracker = hot_reload->get_dependency_tracker();

// Analyze dependencies
dependency_tracker.analyze_file_dependencies("main_game.py");

// Get affected files when a dependency changes
auto affected_files = dependency_tracker.get_affected_files("shared_utils.py");
for (const auto& file : affected_files) {
    LOG_INFO("File {} will be reloaded due to dependency change", file.string());
}
```

### State Preservation

During hot-reload, important state is preserved:

```python
# persistent_state.py
class GameState:
    def __init__(self):
        self.player_score = 0
        self.level = 1
        self.entities = []
    
    def __ecscope_preserve_state__(self):
        """Called before hot-reload to preserve state."""
        return {
            'score': self.player_score,
            'level': self.level,
            'entity_count': len(self.entities)
        }
    
    def __ecscope_restore_state__(self, preserved_data):
        """Called after hot-reload to restore state."""
        self.player_score = preserved_data['score']
        self.level = preserved_data['level']
        # Rebuild entities if necessary
```

### Rollback on Failure

If a hot-reload fails, the system can rollback:

```cpp
// Configure rollback behavior
hot_reload->set_rollback_on_failure(true);
hot_reload->set_max_rollback_versions(5);

// Check reload status
if (hot_reload->last_reload_failed()) {
    auto error_info = hot_reload->get_last_error();
    LOG_ERROR("Hot-reload failed: {}", error_info.message);
    
    // Manual rollback if needed
    hot_reload->rollback_to_previous_version();
}
```

## Performance Profiling

### Function Profiling

Profile script execution with microsecond precision:

```cpp
#include "scripting/script_profiler.hpp"

// Create profiler
auto profiler = std::make_unique<scripting::profiling::FunctionProfiler>(
    scripting::profiling::FunctionProfiler::ProfilingMode::Full
);

profiler->start_profiling();

// Execute profiled code
{
    auto scoped_profiler = profiler->profile_function("game_update_loop");
    
    // Your game logic here
    update_physics(delta_time);
    update_ai(delta_time);
    render_frame();
}

profiler->stop_profiling();

// Analyze results
auto hotspots = profiler->get_hotspots(10);
for (const auto& hotspot : hotspots) {
    LOG_INFO("Hotspot: {} - {:.3f}ms avg, {} calls", 
             hotspot.function_name, 
             hotspot.average_time_ms(), 
             hotspot.call_count);
}
```

### Memory Profiling

Track memory allocations and detect leaks:

```cpp
auto memory_profiler = std::make_unique<scripting::profiling::MemoryProfiler>();
memory_profiler->start_tracking();

// Execute memory-intensive operations
execute_scripts();

memory_profiler->stop_tracking();

// Check for leaks
auto leaks = memory_profiler->get_memory_leaks();
if (!leaks.empty()) {
    LOG_WARN("Detected {} memory leaks:", leaks.size());
    for (const auto& leak : leaks) {
        LOG_WARN("  {} bytes at {} (lifetime: {:.2f}ms)", 
                leak.size, leak.source_location, leak.lifetime_ms());
    }
}
```

### Performance Analysis

Generate comprehensive performance reports:

```cpp
auto analyzer = std::make_unique<scripting::profiling::PerformanceAnalyzer>(
    function_profiler.get(), memory_profiler.get()
);

// Generate report
auto report = analyzer->generate_report();

LOG_INFO("Performance Report:");
LOG_INFO("  Overall Score: {}/100", report.overall_performance_score);
LOG_INFO("  Memory Efficiency: {}/100", report.memory_efficiency_score);
LOG_INFO("  CPU Efficiency: {}/100", report.cpu_efficiency_score);

// Export detailed report
analyzer->export_performance_report("performance_analysis.txt");

// Generate flame graph
std::string flame_graph = analyzer->export_flame_graph();
// Save flame graph for visualization tools
```

## Advanced Features

### Multi-Language Interoperability

Share data between Python and Lua scripts:

```cpp
// Data sharing through JSON
auto shared_data = python_engine->execute_string(R"(
import json
game_data = {
    'player_pos': [10.0, 5.0, 0.0],
    'score': 1500,
    'level': 3
}
json.dumps(game_data)
)");

if (shared_data && shared_data.is_string()) {
    // Pass to Lua
    lua_engine->set_global("shared_json", shared_data.to_string());
    lua_engine->execute_string(R"(
        local json = require('json')
        local data = json.decode(shared_json)
        print("Lua received data - Score: " .. data.score)
    )");
}
```

### Parallel Script Execution

Execute scripts in parallel using the job system:

```cpp
std::vector<job_system::JobID> script_jobs;

// Submit multiple script execution jobs
for (const auto& script_file : script_files) {
    auto job_id = job_system->submit_job(
        "script_" + script_file,
        [&python_engine, script_file]() {
            python_engine->execute_file(script_file);
        }
    );
    script_jobs.push_back(job_id);
}

// Wait for completion
job_system->wait_for_batch(script_jobs);
```

### Custom Allocators

Use custom allocators for specific use cases:

```cpp
// Create specialized allocator for script objects
auto script_allocator = memory_system->create_pool_allocator<ScriptObject>(1000);

// Configure Python engine to use custom allocator
python_engine->set_custom_allocator(script_allocator.get());
```

## Best Practices

### Performance Optimization

1. **Use Sampling Profiling in Production**
   ```cpp
   profiler->set_profiling_mode(ProfilingMode::Sampling);
   profiler->set_sampling_rate(0.01f); // 1% sampling
   ```

2. **Cache Query Results**
   ```cpp
   auto query = ecs_interface->create_query<Position, Velocity>();
   query->set_cache_duration(std::chrono::milliseconds(16)); // One frame
   ```

3. **Batch Entity Operations**
   ```python
   # Instead of processing entities one by one
   entities = ecs.query_entities("Position", "Velocity")
   ecs.batch_update(entities, update_movement_function)
   ```

4. **Use Object Pooling**
   ```python
   class EntityPool:
       def __init__(self, size):
           self.available = [create_entity() for _ in range(size)]
           self.in_use = []
       
       def acquire(self):
           if self.available:
               entity = self.available.pop()
               self.in_use.append(entity)
               return entity
           return None
       
       def release(self, entity):
           if entity in self.in_use:
               self.in_use.remove(entity)
               reset_entity(entity)
               self.available.append(entity)
   ```

### Memory Management

1. **Monitor Memory Usage**
   ```cpp
   auto stats = memory_profiler->get_statistics();
   if (stats.current_allocated > threshold) {
       trigger_garbage_collection();
   }
   ```

2. **Use RAII Patterns**
   ```cpp
   {
       auto entity_guard = ecs_interface->create_entity_guard();
       // Entity automatically cleaned up when guard goes out of scope
   }
   ```

3. **Avoid Memory Leaks in Scripts**
   ```python
   # Use context managers
   with entity_manager.create_entity() as entity:
       entity.add_component("Position")
       # Entity automatically destroyed
   ```

### Error Handling

1. **Graceful Degradation**
   ```cpp
   try {
       python_engine->execute_file(script_file);
   } catch (const ScriptException& e) {
       LOG_ERROR("Script error: {}", e.what());
       // Continue with default behavior
       execute_fallback_logic();
   }
   ```

2. **Validate Script Input**
   ```python
   def safe_move_entity(entity, dx, dy):
       if not entity or not entity.is_valid():
           return False
       
       if not isinstance(dx, (int, float)) or not isinstance(dy, (int, float)):
           return False
       
       position = entity.get_component("Position")
       if position:
           position.x += float(dx)
           position.y += float(dy)
           return True
       return False
   ```

### Code Organization

1. **Separate Script Categories**
   ```
   scripts/
   ├── systems/           # ECS systems
   ├── components/        # Component definitions
   ├── ai/               # AI behaviors
   ├── gameplay/         # Game logic
   └── utils/            # Utility functions
   ```

2. **Use Configuration Files**
   ```python
   # config.py
   PHYSICS_SETTINGS = {
       'gravity': 9.81,
       'damping': 0.99,
       'max_velocity': 100.0
   }
   
   AI_SETTINGS = {
       'decision_interval': 1.0,
       'sight_range': 10.0,
       'attack_range': 2.0
   }
   ```

## Troubleshooting

### Common Issues

#### Python Import Errors

**Problem**: `ModuleNotFoundError` when importing custom modules.

**Solution**: Add script directories to Python path:
```cpp
python_engine->add_to_sys_path("./scripts");
python_engine->add_to_sys_path("./game_modules");
```

#### Memory Leaks

**Problem**: Memory usage keeps growing over time.

**Solution**: Use memory profiler to identify leaks:
```cpp
auto leaks = memory_profiler->get_memory_leaks();
for (const auto& leak : leaks) {
    LOG_WARN("Memory leak: {} bytes at {}", leak.size, leak.source_location);
}
```

#### Performance Issues

**Problem**: Scripts causing frame rate drops.

**Solution**: Use profiler to identify bottlenecks:
```cpp
auto hotspots = profiler->get_hotspots(10);
// Optimize the top hotspots
```

#### Hot-Reload Failures

**Problem**: Script changes not being detected or causing errors.

**Solution**: Check dependency tracking and error logs:
```cpp
if (hot_reload->last_reload_failed()) {
    auto error = hot_reload->get_last_error();
    LOG_ERROR("Reload failed: {}", error.message);
    hot_reload->rollback_to_previous_version();
}
```

### Debugging Tips

1. **Enable Detailed Logging**
   ```cpp
   python_engine->set_log_level(LogLevel::DEBUG);
   hot_reload->enable_verbose_logging(true);
   ```

2. **Use Script Debugger Integration**
   ```python
   import pdb
   
   def debug_entity_behavior(entity):
       pdb.set_trace()  # Breakpoint for debugging
       # Examine entity state
   ```

3. **Monitor System Resources**
   ```cpp
   // Regular health checks
   auto python_stats = python_engine->get_statistics();
   auto ecs_stats = ecs_interface->get_statistics();
   
   LOG_INFO("Python: {} scripts, {} exceptions", 
           python_stats.scripts_executed, 
           python_stats.exceptions_thrown);
   LOG_INFO("ECS: {} entities, {} component accesses", 
           ecs_stats.current_entities, 
           ecs_stats.component_accesses);
   ```

### Performance Tuning

1. **Profile Before Optimizing**
   ```cpp
   profiler->start_profiling();
   // Run your game loop
   profiler->stop_profiling();
   
   auto report = analyzer->generate_report();
   // Focus on the top hotspots
   ```

2. **Use Appropriate Profiling Modes**
   ```cpp
   // Development: Full profiling
   profiler->set_profiling_mode(ProfilingMode::Full);
   
   // Production: Sampling
   profiler->set_profiling_mode(ProfilingMode::Sampling);
   profiler->set_sampling_rate(0.01f);
   ```

3. **Optimize Based on Data**
   - High call frequency + short duration = candidate for inlining
   - High memory allocation = consider object pooling
   - Long execution time = algorithm optimization needed

## API Reference

### Python Engine API

```cpp
class PythonEngine {
public:
    bool initialize();
    void shutdown();
    
    PyObjectWrapper execute_string(const std::string& code);
    PyObjectWrapper execute_file(const std::string& filepath);
    
    void set_global(const std::string& name, PyObject* value);
    PyObjectWrapper get_global(const std::string& name);
    
    template<typename T>
    void register_component(const std::string& name);
    
    Statistics get_statistics() const;
    PythonMemoryManager& get_memory_manager();
    PythonExceptionHandler& get_exception_handler();
};
```

### ECS Scripting Interface API

```cpp
class ECSScriptInterface {
public:
    ScriptEntity* create_entity();
    bool destroy_entity(ecs::Entity entity);
    ScriptEntity* get_entity(ecs::Entity entity);
    
    template<ecs::Component... Components>
    std::unique_ptr<ScriptQuery<Components...>> create_query();
    
    template<typename T>
    void register_component_type(const std::string& name);
    
    Statistics get_statistics() const;
};
```

### Profiler API

```cpp
class FunctionProfiler {
public:
    void start_profiling();
    void stop_profiling();
    
    ScopedProfiler profile_function(const std::string& name);
    
    std::vector<FunctionCallInfo> get_function_statistics() const;
    std::vector<FunctionCallInfo> get_hotspots(usize count) const;
    
    void set_profiling_mode(ProfilingMode mode);
    void set_sampling_rate(f32 rate);
};
```

### Hot-Reload API

```cpp
class HotReloadSystem {
public:
    bool add_watch_directory(const std::filesystem::path& directory,
                           bool recursive, const std::string& pattern);
    
    void start_monitoring();
    void stop_monitoring();
    
    bool last_reload_failed() const;
    void rollback_to_previous_version();
    
    DependencyTracker& get_dependency_tracker();
};
```

## Conclusion

ECScope's scripting integration provides a powerful, flexible, and educational platform for game development and system prototyping. By combining high-performance execution with comprehensive debugging and profiling tools, it enables both learning and production use cases.

The system's emphasis on memory management, error handling, and performance monitoring makes it suitable for professional game development, while its educational features and extensive documentation make it an excellent learning platform for understanding advanced programming concepts.

For more examples and advanced usage patterns, see the `examples/scripting_examples.cpp` file and the comprehensive test suite in `tests/scripting_integration_tests.cpp`.