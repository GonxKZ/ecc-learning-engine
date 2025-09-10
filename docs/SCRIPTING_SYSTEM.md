# ECScope Scripting System

A professional-grade, multi-language scripting engine designed for game development, modding, and rapid prototyping. The ECScope scripting system provides comprehensive integration with the ECS architecture and all engine systems.

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Supported Languages](#supported-languages)
- [Quick Start](#quick-start)
- [Architecture](#architecture)
- [ECS Integration](#ecs-integration)
- [Engine System Bindings](#engine-system-bindings)
- [Hot-Reload System](#hot-reload-system)
- [Debugging Features](#debugging-features)
- [Performance Profiling](#performance-profiling)
- [Cross-Language Calls](#cross-language-calls)
- [Interactive REPL](#interactive-repl)
- [Advanced Features](#advanced-features)
- [Best Practices](#best-practices)
- [API Reference](#api-reference)

## Overview

The ECScope scripting system is built from the ground up to provide:

- **Multi-language support**: Lua 5.4+ and Python 3.11+ with seamless interoperability
- **Deep ECS integration**: Direct access to entities, components, and systems
- **Engine system bindings**: Full access to rendering, physics, audio, and networking
- **Professional tooling**: Hot-reload, debugging, profiling, and interactive REPL
- **Performance-focused**: Optimized execution with memory management and caching
- **Educational**: Comprehensive examples and tutorials for learning game scripting

## Features

### Core Features
- ✅ **Multi-language support** (Lua and Python)
- ✅ **Type-safe function calls** with automatic marshaling
- ✅ **Comprehensive error handling** with detailed stack traces
- ✅ **Memory management** with configurable limits and garbage collection
- ✅ **Script compilation** and bytecode caching
- ✅ **Async execution** with coroutine support
- ✅ **Thread safety** with proper synchronization

### Development Features
- ✅ **Hot-reload** with state preservation
- ✅ **Interactive REPL** with syntax highlighting and auto-completion
- ✅ **Advanced debugging** with breakpoints and variable inspection
- ✅ **Performance profiling** with detailed metrics and optimization suggestions
- ✅ **Cross-language calls** and data sharing
- ✅ **Sandboxing** for secure script execution

### ECS Integration
- ✅ **Entity management** (create, destroy, clone)
- ✅ **Component operations** (add, remove, get, has)
- ✅ **System registration** and execution
- ✅ **Query execution** with filtering and iteration
- ✅ **Event system** integration
- ✅ **Archetype operations** and optimization

### Engine System Bindings
- ✅ **Rendering system** (shaders, materials, meshes, cameras)
- ✅ **Physics system** (rigid bodies, collisions, constraints)
- ✅ **Audio system** (3D positional audio, music, effects)
- ✅ **Asset system** (loading textures, models, shaders, audio)
- ✅ **GUI system** (widgets, layouts, styling)
- ✅ **Networking system** (client/server, packet handling)

## Supported Languages

### Lua 5.4+
- **Strengths**: Fast execution, small memory footprint, simple syntax
- **Use cases**: Game logic, AI behaviors, configuration scripts
- **Features**: Coroutines, metatables, function caching, JIT compilation
- **Performance**: ~2-10x faster than Python for computational tasks

### Python 3.11+
- **Strengths**: Rich ecosystem, scientific libraries, readable syntax
- **Use cases**: Complex algorithms, data analysis, AI/ML integration
- **Features**: Async/await, NumPy integration, comprehensive debugging
- **Performance**: Slower than Lua but excellent for complex logic

## Quick Start

### Basic Setup

```cpp
#include <ecscope/scripting/script_manager.hpp>

// Initialize the scripting system
auto script_manager = std::make_unique<ScriptManager>();
script_manager->initialize();

// Bind ECS registry
script_manager->bind_ecs_registry(ecs_registry);
```

### Loading and Executing Scripts

```cpp
// Load Lua script
std::string lua_code = R"(
    print("Hello from Lua!")
    
    function greet(name)
        return "Hello, " .. name .. "!"
    end
)";

script_manager->load_script("greeting", lua_code, "lua");
script_manager->execute_script("greeting");

// Call Lua function
auto result = script_manager->call_function<std::string>("greeting", "greet", "World");
if (result.is_success()) {
    std::cout << result.value() << std::endl; // "Hello, World!"
}
```

```cpp
// Load Python script
std::string python_code = R"(
import math

def calculate_distance(x1, y1, x2, y2):
    return math.sqrt((x2 - x1)**2 + (y2 - y1)**2)
    
class AIBehavior:
    def __init__(self, name):
        self.name = name
        self.state = "idle"
    
    def update(self, dt):
        # AI logic here
        return f"{self.name} is {self.state}"
)";

script_manager->load_script("ai_system", python_code, "python");
script_manager->execute_script("ai_system");

// Call Python function
auto distance = script_manager->call_function<double>("ai_system", "calculate_distance", 0, 0, 3, 4);
// Result: 5.0
```

### ECS Integration

```cpp
// Lua ECS script
std::string ecs_lua = R"(
-- Create entities
local player = create_entity()
local enemy = create_entity()

-- Add components
add_transform(player, 0, 0, 0)
add_velocity(player, 1, 0, 0)
add_health(player, 100)

-- Query entities
function movement_system()
    local entities = query_entities_with_components({"transform", "velocity"})
    for _, entity in ipairs(entities) do
        local transform = get_transform(entity)
        local velocity = get_velocity(entity)
        
        -- Update position
        transform.x = transform.x + velocity.x * dt
        set_transform(entity, transform.x, transform.y, transform.z)
    end
end

-- Register system
register_system("movement_system", movement_system)
)";
```

```python
# Python ECS script
import numpy as np

def ai_system():
    """Advanced AI system with NumPy calculations"""
    entities = query_entities_with_components(["ai_component", "transform"])
    
    for entity in entities:
        ai = get_ai_component(entity)
        transform = get_transform(entity)
        
        # Use NumPy for pathfinding
        current_pos = np.array([transform.x, transform.y, transform.z])
        target_pos = np.array([ai.target_x, ai.target_y, ai.target_z])
        
        direction = target_pos - current_pos
        if np.linalg.norm(direction) > 0.1:
            direction = direction / np.linalg.norm(direction)
            new_pos = current_pos + direction * ai.speed * dt
            set_transform(entity, new_pos[0], new_pos[1], new_pos[2])

# Register system
register_system("ai_system", ai_system)
```

## Architecture

The scripting system follows a modular, extensible architecture:

```
ScriptManager (Main Interface)
├── ScriptEngine (Abstract Base)
│   ├── LuaEngine (Lua Implementation)
│   └── PythonEngine (Python Implementation)
├── ScriptHotReloader (File Watching & Reload)
├── ScriptDebugManager (Debugging & Profiling)
├── MultiLanguageREPL (Interactive Console)
└── Language Bindings
    ├── LuaECSBinder (ECS Integration)
    ├── PythonECSBinder (ECS Integration)
    └── EngineBindings (System Integration)
```

### Key Components

1. **ScriptManager**: Central orchestrator for all scripting operations
2. **ScriptEngine**: Abstract base class for language-specific implementations
3. **Language Engines**: Lua and Python engines with full feature support
4. **Bindings**: Type-safe bridges between C++ and scripting languages
5. **Development Tools**: Hot-reload, debugging, profiling, and REPL

## ECS Integration

The scripting system provides complete access to the ECS architecture:

### Entity Operations
```lua
-- Lua
local entity = create_entity()
destroy_entity(entity)
local clone = clone_entity(entity)
local archetype = get_entity_archetype(entity)
```

```python
# Python
entity = create_entity()
destroy_entity(entity)
clone = clone_entity(entity)
archetype = get_entity_archetype(entity)
```

### Component Operations
```lua
-- Lua
add_transform(entity, x, y, z)
remove_component(entity, "transform")
local transform = get_transform(entity)
local has_health = has_component(entity, "health")
local components = list_components(entity)
```

```python
# Python
add_transform(entity, x, y, z)
remove_component(entity, "transform")
transform = get_transform(entity)
has_health = has_component(entity, "health")
components = list_components(entity)
```

### System Registration
```lua
-- Lua
function movement_system()
    -- System logic
end

register_system("movement", movement_system, {"transform", "velocity"})
```

```python
# Python
def physics_system():
    # System logic
    pass

register_system("physics", physics_system, dependencies=["transform", "rigidbody"])
```

### Query System
```lua
-- Lua
local entities = query_entities_with_components({"transform", "renderable"})
local count = count_entities_with_components({"health"})

for_each_entity({"transform"}, function(entity, transform)
    -- Process each entity
end)
```

```python
# Python
entities = query_entities_with_components(["transform", "renderable"])
count = count_entities_with_components(["health"])

def process_entity(entity, transform):
    # Process each entity
    pass

for_each_entity(["transform"], process_entity)
```

## Engine System Bindings

### Rendering System
```lua
-- Lua
render_mesh(mesh_handle, transform_matrix)
set_camera_transform(camera_entity, x, y, z)
local shader = create_shader("vertex.glsl", "fragment.glsl")
bind_texture(texture_handle, slot)
```

```python
# Python
render_mesh(mesh_handle, transform_matrix)
set_camera_transform(camera_entity, x, y, z)
shader = create_shader("vertex.glsl", "fragment.glsl")
bind_texture(texture_handle, slot)
```

### Physics System
```lua
-- Lua
add_rigidbody(entity, mass)
apply_force(entity, fx, fy, fz)
set_velocity(entity, vx, vy, vz)
local hit = raycast(x, y, z, dx, dy, dz, max_distance)
```

```python
# Python
add_rigidbody(entity, mass)
apply_force(entity, fx, fy, fz)
set_velocity(entity, vx, vy, vz)
hit = raycast(x, y, z, dx, dy, dz, max_distance)
```

### Audio System
```lua
-- Lua
play_sound("explosion.wav", volume, pitch)
play_music("theme.ogg", loop)
set_audio_listener_position(x, y, z)
```

```python
# Python
play_sound("explosion.wav", volume, pitch)
play_music("theme.ogg", loop)
set_audio_listener_position(x, y, z)
```

## Hot-Reload System

The hot-reload system monitors script files and automatically reloads them when changed:

```cpp
// Enable hot-reload
script_manager->enable_hot_reload(true);

// Watch directories
script_manager->watch_directory("scripts/");
script_manager->watch_directory("ai/");

// Set reload callback
script_manager->get_hot_reloader()->set_reload_callback([](const std::string& script_name) {
    std::cout << "Script reloaded: " << script_name << std::endl;
});

// Enable state preservation
script_manager->enable_state_preservation(true);
```

### State Preservation
The hot-reload system can preserve script state across reloads:

```lua
-- Before reload
player_score = 1500
current_level = 3
inventory = {"sword", "potion", "key"}

-- After reload, these variables are preserved
```

## Debugging Features

### Breakpoints
```cpp
// Set breakpoints
script_manager->get_debug_manager()->set_global_breakpoint("ai_script", 25);
script_manager->get_debug_manager()->set_global_breakpoint("physics_script", 42);
```

### Variable Inspection
```cpp
// Get local variables
auto locals = debugger->get_local_variables();
for (const auto& [name, value] : locals) {
    std::cout << name << " = " << /* convert value */ << std::endl;
}

// Get global variables
auto globals = debugger->get_global_variables();
```

### Execution Control
```cpp
// Step through code
debugger->step_over();
debugger->step_into();
debugger->step_out();
debugger->continue_execution();
```

### Error Handling
```cpp
// Set global error handler
script_manager->set_error_handler([](const ScriptError& error) {
    std::cout << "Script Error: " << error.format_error() << std::endl;
});

// Get recent errors
auto errors = script_manager->get_debug_manager()->get_all_recent_errors();
```

## Performance Profiling

### Enable Profiling
```cpp
// Start global profiling
script_manager->start_global_profiling();

// Execute scripts
script_manager->execute_script("game_logic");
script_manager->call_function("ai_script", "update", dt);

// Stop profiling and generate report
script_manager->stop_global_profiling();
auto report = script_manager->generate_comprehensive_report();
```

### Performance Metrics
```cpp
// Get metrics for specific script
auto metrics = script_manager->get_script_metrics("game_logic");
std::cout << "Execution time: " << metrics.execution_time.count() << "ns" << std::endl;
std::cout << "Memory usage: " << metrics.memory_usage_bytes << " bytes" << std::endl;
std::cout << "Function calls: " << metrics.function_calls << std::endl;
```

### Optimization Suggestions
```cpp
// Get optimization suggestions
auto suggestions = script_manager->get_engine("lua")->get_optimization_suggestions("ai_script");
for (const auto& suggestion : suggestions) {
    std::cout << "Suggestion: " << suggestion << std::endl;
}
```

## Cross-Language Calls

Scripts can call functions from different languages:

```cpp
// Share variables between languages
script_manager->set_global_shared_variable("shared_data", 42);
script_manager->set_global_shared_variable("config", config_object);

// Call function in different language
auto result = script_manager->call_cross_language_function(
    "lua_script", "python_script", "advanced_calculation", {data1, data2});
```

```lua
-- Lua script
function lua_fibonacci(n)
    if n <= 1 then return n end
    return lua_fibonacci(n-1) + lua_fibonacci(n-2)
end

-- Access shared data
local shared = get_shared_variable("shared_data")
```

```python
# Python script
import numpy as np

def advanced_calculation(data1, data2):
    # Use NumPy for complex calculations
    arr1 = np.array(data1)
    arr2 = np.array(data2)
    return np.dot(arr1, arr2).tolist()

# Call Lua function from Python
fib_result = call_function("lua_script", "lua_fibonacci", 10)
```

## Interactive REPL

The multi-language REPL provides an interactive development environment:

```cpp
// Start REPL
auto repl = script_manager->get_repl();
repl->start();
```

### REPL Commands
```
>>> print("Hello, REPL!")
Hello, REPL!

>>> %switch python
Switched to Python

>>> import math
>>> math.sqrt(16)
4.0

>>> %switch lua
Switched to Lua

>>> function greet(name) return "Hello, " .. name end
>>> greet("REPL")
Hello, REPL

>>> %help
Available commands:
  %switch <language> - Switch language
  %list_scripts      - List loaded scripts
  %profile <script>  - Profile script performance
  %help             - Show help

>>> %list_scripts
Loaded scripts:
  - ai_system (Python)
  - game_logic (Lua)
  - physics_system (Lua)

>>> exit
```

## Advanced Features

### Coroutines (Lua)
```lua
function async_task()
    for i = 1, 10 do
        print("Task step " .. i)
        coroutine.yield(i)
    end
    return "Task completed"
end

local co = coroutine.create(async_task)
repeat
    local success, value = coroutine.resume(co)
    if success then
        print("Yielded:", value)
    end
until coroutine.status(co) == "dead"
```

### Async/Await (Python)
```python
import asyncio

async def async_ai_behavior():
    await asyncio.sleep(0.1)  # Simulate AI processing
    return "AI decision made"

async def main():
    tasks = [async_ai_behavior() for _ in range(5)]
    results = await asyncio.gather(*tasks)
    return results
```

### Sandboxing
```cpp
// Enable sandbox mode
script_manager->enable_script_sandboxing(true);

// Set allowed modules
script_manager->get_engine("python")->set_allowed_modules("restricted_script", {"math", "random"});

// Restrict I/O operations
script_manager->get_engine("lua")->set_io_restrictions("user_script", false);
```

### Memory Management
```cpp
// Set memory limits
script_manager->set_memory_limit("game_script", 16 * 1024 * 1024); // 16MB

// Force garbage collection
script_manager->collect_all_garbage();

// Monitor memory usage
auto memory_usage = script_manager->get_memory_usage_by_language();
```

## Best Practices

### Performance
1. **Use function caching**: Enable for frequently called functions
2. **Batch operations**: Group related calls together
3. **Monitor memory**: Set appropriate limits and collect garbage regularly
4. **Profile regularly**: Identify bottlenecks and optimize hot paths
5. **Choose the right language**: Lua for performance, Python for complexity

### Development
1. **Enable hot-reload**: For rapid iteration during development
2. **Use breakpoints**: Debug complex logic step by step
3. **Handle errors gracefully**: Set up comprehensive error handling
4. **Document scripts**: Use clear naming and comments
5. **Test extensively**: Validate scripts with unit tests

### Architecture
1. **Separate concerns**: Keep game logic, AI, and UI scripts separate
2. **Use events**: Communicate between scripts via the event system
3. **Minimize cross-language calls**: They have overhead
4. **Cache frequently accessed data**: Reduce engine system queries
5. **Plan for modding**: Design scripts to be modifiable by users

## API Reference

### ScriptManager

```cpp
class ScriptManager {
public:
    // Lifecycle
    bool initialize();
    void shutdown();
    
    // Script Management
    ScriptResult<void> load_script(const std::string& name, const std::string& source, const std::string& language);
    ScriptResult<void> load_script_file(const std::string& filepath);
    ScriptResult<void> execute_script(const std::string& name);
    
    // Function Calls
    template<typename ReturnType = void, typename... Args>
    ScriptResult<ReturnType> call_function(const std::string& script_name, const std::string& function_name, Args&&... args);
    
    // Engine Integration
    void bind_ecs_registry(ecs::Registry* registry);
    void bind_physics_world(physics::World* world);
    void bind_renderer(rendering::Renderer* renderer);
    void bind_audio_system(audio::AudioSystem* audio);
    
    // Development Tools
    ScriptHotReloader* get_hot_reloader();
    ScriptDebugManager* get_debug_manager();
    MultiLanguageREPL* get_repl();
    
    // Performance
    void start_global_profiling();
    void stop_global_profiling();
    std::string generate_comprehensive_report();
};
```

### Script Functions (Available in Scripts)

#### ECS Functions
```
# Entity Management
create_entity() -> EntityID
destroy_entity(entity: EntityID)
clone_entity(entity: EntityID) -> EntityID

# Component Operations
add_<component>(entity: EntityID, ...)
remove_<component>(entity: EntityID)
get_<component>(entity: EntityID) -> Component
has_<component>(entity: EntityID) -> bool

# System Operations
register_system(name: string, function, dependencies: list)
unregister_system(name: string)

# Queries
query_entities_with_components(components: list) -> list[EntityID]
count_entities_with_components(components: list) -> int
for_each_entity(components: list, callback: function)
```

#### Engine System Functions
```
# Rendering
render_mesh(mesh, transform)
set_camera_transform(entity, x, y, z)
create_shader(vertex, fragment) -> ShaderID
bind_texture(texture, slot)

# Physics
add_rigidbody(entity, mass)
apply_force(entity, fx, fy, fz)
raycast(x, y, z, dx, dy, dz, distance) -> RaycastHit

# Audio
play_sound(filename, volume, pitch)
play_music(filename, loop)
set_listener_position(x, y, z)

# Assets
load_texture(filename) -> TextureID
load_model(filename) -> ModelID
load_sound(filename) -> SoundID
```

For complete API documentation, see the generated documentation in `docs/api/`.

---

This comprehensive scripting system provides everything needed for professional game development with robust multi-language support, advanced tooling, and seamless engine integration. Whether you're developing game logic, creating mod support, or building educational content, the ECScope scripting system has you covered.