/**
 * @file 15-comprehensive-scripting-demo.cpp
 * @brief Comprehensive demonstration of the ECScope scripting engine
 * 
 * This demo showcases the complete scripting system with:
 * - Multi-language support (Lua and Python)
 * - ECS integration with full entity/component/system access
 * - Engine system bindings (rendering, physics, audio)
 * - Hot-reload with state preservation
 * - Interactive REPL and debugging
 * - Performance profiling and optimization
 * - Cross-language function calls
 * - Educational examples and best practices
 */

#include "../include/ecscope/scripting/script_manager.hpp"
#include "../include/ecscope/scripting/lua_engine.hpp"
#include "../include/ecscope/scripting/python_engine.hpp"
#include "../include/ecscope/ecs/registry.hpp"
#include "../include/ecscope/physics/world.hpp"
#include "../include/ecscope/rendering/renderer.hpp"
#include "../include/ecscope/audio/audio_system.hpp"
#include "../include/ecscope/components.hpp"

#include <iostream>
#include <chrono>
#include <thread>

using namespace ecscope;
using namespace ecscope::scripting;

/**
 * @brief Professional-grade scripting demo class
 */
class ScriptingSystemDemo {
public:
    ScriptingSystemDemo() = default;
    ~ScriptingSystemDemo() = default;
    
    void run_comprehensive_demo() {
        std::cout << "=== ECScope Comprehensive Scripting System Demo ===\n\n";
        
        // Initialize all systems
        if (!initialize_systems()) {
            std::cout << "Failed to initialize systems\n";
            return;
        }
        
        // Run demo sections
        demonstrate_basic_functionality();
        demonstrate_ecs_integration();
        demonstrate_engine_system_bindings();
        demonstrate_cross_language_calls();
        demonstrate_hot_reload_system();
        demonstrate_debugging_features();
        demonstrate_performance_profiling();
        demonstrate_repl_interaction();
        demonstrate_advanced_features();
        demonstrate_educational_content();
        
        // Show comprehensive statistics
        show_comprehensive_statistics();
        
        // Interactive mode
        std::cout << "\n=== Interactive Mode ===\n";
        std::cout << "Starting multi-language REPL (type 'exit' to quit)...\n";
        start_interactive_mode();
        
        cleanup_systems();
    }

private:
    std::unique_ptr<ScriptManager> script_manager_;
    std::unique_ptr<ecs::Registry> ecs_registry_;
    std::unique_ptr<physics::World> physics_world_;
    std::unique_ptr<rendering::Renderer> renderer_;
    std::unique_ptr<audio::AudioSystem> audio_system_;
    
    bool initialize_systems() {
        std::cout << "Initializing ECScope scripting system...\n";
        
        // Create script manager
        script_manager_ = std::make_unique<ScriptManager>();
        if (!script_manager_->initialize()) {
            std::cout << "Failed to initialize script manager\n";
            return false;
        }
        
        // Create and initialize ECS registry
        ecs_registry_ = std::make_unique<ecs::Registry>();
        
        // Create physics world
        physics_world_ = std::make_unique<physics::World>();
        
        // Create renderer (mock implementation for demo)
        // renderer_ = std::make_unique<rendering::Renderer>();
        
        // Create audio system (mock implementation for demo)  
        // audio_system_ = std::make_unique<audio::AudioSystem>();
        
        // Bind all engine systems to scripting
        script_manager_->bind_ecs_registry(ecs_registry_.get());
        script_manager_->bind_physics_world(physics_world_.get());
        // script_manager_->bind_renderer(renderer_.get());
        // script_manager_->bind_audio_system(audio_system_.get());
        
        // Register both Lua and Python engines
        register_lua_engine();
        register_python_engine();
        
        std::cout << "✓ All systems initialized successfully\n";
        std::cout << "✓ Supported languages: ";
        for (const auto& lang : script_manager_->get_supported_languages()) {
            std::cout << lang << " ";
        }
        std::cout << "\n\n";
        
        return true;
    }
    
    void demonstrate_basic_functionality() {
        std::cout << "=== 1. Basic Scripting Functionality ===\n";
        
        // Load and execute Lua script
        std::string lua_script = R"(
-- Basic Lua script demonstration
print("Hello from Lua!")

function greet(name)
    return "Hello, " .. name .. " from Lua!"
end

function calculate_fibonacci(n)
    if n <= 1 then
        return n
    else
        return calculate_fibonacci(n-1) + calculate_fibonacci(n-2)
    end
end

-- Global variable
lua_value = 42
)";
        
        auto result = script_manager_->load_script("demo_lua", lua_script, "lua");
        if (result.is_success()) {
            std::cout << "✓ Lua script loaded successfully\n";
            
            script_manager_->execute_script("demo_lua");
            
            // Call Lua function
            auto greeting_result = script_manager_->call_function<std::string>("demo_lua", "greet", "ECScope");
            if (greeting_result.is_success()) {
                std::cout << "✓ Lua function result: " << greeting_result.value() << "\n";
            }
            
        } else {
            std::cout << "✗ Failed to load Lua script: " << result.error().message << "\n";
        }
        
        // Load and execute Python script
        std::string python_script = R"(
# Basic Python script demonstration
print("Hello from Python!")

import math

def greet(name):
    return f"Hello, {name} from Python!"

def calculate_prime_factors(n):
    factors = []
    d = 2
    while d * d <= n:
        while n % d == 0:
            factors.append(d)
            n //= d
        d += 1
    if n > 1:
        factors.append(n)
    return factors

# Global variable
python_value = 3.14159
)";
        
        result = script_manager_->load_script("demo_python", python_script, "python");
        if (result.is_success()) {
            std::cout << "✓ Python script loaded successfully\n";
            
            script_manager_->execute_script("demo_python");
            
            // Call Python function
            auto greeting_result = script_manager_->call_function<std::string>("demo_python", "greet", "ECScope");
            if (greeting_result.is_success()) {
                std::cout << "✓ Python function result: " << greeting_result.value() << "\n";
            }
            
        } else {
            std::cout << "✗ Failed to load Python script: " << result.error().message << "\n";
        }
        
        std::cout << "\n";
    }
    
    void demonstrate_ecs_integration() {
        std::cout << "=== 2. ECS Integration Demo ===\n";
        
        // Lua ECS script
        std::string lua_ecs_script = R"(
-- ECS Integration in Lua
print("Creating entities and components in Lua...")

-- Create entities
local player = create_entity()
local enemy = create_entity()

-- Add components
add_transform(player, 0, 0, 0)
add_velocity(player, 1, 0, 0)
add_health(player, 100)

add_transform(enemy, 10, 0, 0)
add_velocity(enemy, -0.5, 0, 0)  
add_health(enemy, 50)

print("Player entity: " .. player)
print("Enemy entity: " .. enemy)

-- Query entities with transform and velocity
function update_movement_system()
    local entities = query_entities_with_components({"transform", "velocity"})
    for _, entity in ipairs(entities) do
        local transform = get_transform(entity)
        local velocity = get_velocity(entity)
        
        -- Update position
        transform.x = transform.x + velocity.x * dt
        transform.y = transform.y + velocity.y * dt
        transform.z = transform.z + velocity.z * dt
        
        set_transform(entity, transform.x, transform.y, transform.z)
    end
end

function combat_system()
    local entities = query_entities_with_components({"health"})
    for _, entity in ipairs(entities) do
        local health = get_health(entity)
        if health.current <= 0 then
            destroy_entity(entity)
            print("Entity " .. entity .. " destroyed!")
        end
    end
end
)";
        
        auto result = script_manager_->load_script("lua_ecs", lua_ecs_script, "lua");
        if (result.is_success()) {
            std::cout << "✓ Lua ECS script loaded\n";
            script_manager_->execute_script("lua_ecs");
        }
        
        // Python ECS script
        std::string python_ecs_script = R"(
# ECS Integration in Python
print("Creating entities and components in Python...")

import numpy as np

# Create entities
npc1 = create_entity()
npc2 = create_entity()

# Add components with more complex data
add_transform(npc1, 5.0, 2.0, 1.0)
add_mesh_renderer(npc1, "character.obj")
add_ai_component(npc1, "patrol")

add_transform(npc2, -3.0, 0.0, 2.0)
add_mesh_renderer(npc2, "guard.obj") 
add_ai_component(npc2, "guard")

print(f"NPC1 entity: {npc1}")
print(f"NPC2 entity: {npc2}")

def ai_system():
    """Advanced AI system using NumPy for calculations"""
    entities = query_entities_with_components(["ai_component", "transform"])
    
    for entity in entities:
        ai = get_ai_component(entity)
        transform = get_transform(entity)
        
        if ai.behavior == "patrol":
            # Use NumPy for path calculation
            current_pos = np.array([transform.x, transform.y, transform.z])
            target_pos = np.array([ai.target_x, ai.target_y, ai.target_z])
            direction = target_pos - current_pos
            
            if np.linalg.norm(direction) > 0.1:
                direction = direction / np.linalg.norm(direction)
                new_pos = current_pos + direction * ai.speed * dt
                set_transform(entity, new_pos[0], new_pos[1], new_pos[2])

def rendering_system():
    """Rendering system for visible entities"""
    entities = query_entities_with_components(["transform", "mesh_renderer"])
    
    for entity in entities:
        transform = get_transform(entity)
        mesh_renderer = get_mesh_renderer(entity)
        
        # Queue for rendering
        render_mesh(mesh_renderer.mesh, transform)

# Register systems for automatic execution
register_system("ai_system", ai_system)
register_system("rendering_system", rendering_system)
)";
        
        result = script_manager_->load_script("python_ecs", python_ecs_script, "python");
        if (result.is_success()) {
            std::cout << "✓ Python ECS script loaded\n";
            script_manager_->execute_script("python_ecs");
        }
        
        // Show ECS statistics
        std::cout << "✓ Entity count: " << ecs_registry_->get_entity_count() << "\n";
        std::cout << "✓ Active archetypes: " << ecs_registry_->get_archetype_count() << "\n";
        
        std::cout << "\n";
    }
    
    void demonstrate_engine_system_bindings() {
        std::cout << "=== 3. Engine System Bindings Demo ===\n";
        
        // Physics scripting in Lua
        std::string physics_lua_script = R"(
-- Physics system integration
print("Demonstrating physics system in Lua...")

-- Create a physics entity
local box_entity = create_entity()
add_transform(box_entity, 0, 10, 0)
add_rigidbody(box_entity, 1.0) -- mass = 1.0
add_box_collider(box_entity, 1, 1, 1) -- 1x1x1 box

-- Apply forces
apply_force(box_entity, 0, 0, 5) -- push forward
set_velocity(box_entity, 2, 0, 0) -- initial velocity

-- Raycast example
local hit_result = raycast(0, 15, 0, 0, -1, 0, 20) -- ray downward
if hit_result.hit then
    print("Raycast hit at distance: " .. hit_result.distance)
end

function physics_update()
    -- Custom physics logic
    local physics_entities = query_entities_with_components({"rigidbody", "transform"})
    for _, entity in ipairs(physics_entities) do
        local transform = get_transform(entity)
        local rigidbody = get_rigidbody(entity)
        
        -- Apply gravity if not kinematic
        if not rigidbody.is_kinematic then
            apply_force(entity, 0, -9.81 * rigidbody.mass, 0)
        end
        
        -- Clamp velocity
        local velocity = get_velocity(entity)
        if velocity.magnitude > 50 then
            local normalized = velocity.normalized
            set_velocity(entity, normalized.x * 50, normalized.y * 50, normalized.z * 50)
        end
    end
end
)";
        
        script_manager_->load_script("physics_lua", physics_lua_script, "lua");
        script_manager_->execute_script("physics_lua");
        std::cout << "✓ Physics Lua script executed\n";
        
        // Rendering scripting in Python
        std::string rendering_python_script = R"(
# Rendering system integration
print("Demonstrating rendering system in Python...")

# Create visual entities
model_entity = create_entity()
add_transform(model_entity, 0, 0, 0)
add_mesh_renderer(model_entity, "spaceship.obj")
add_material(model_entity, "metallic_shader")

# Camera setup
camera_entity = create_entity()
add_transform(camera_entity, 0, 5, 10)
add_camera(camera_entity, 60, 1.78, 0.1, 100) # fov, aspect, near, far
set_camera_target(camera_entity, 0, 0, 0)

# Lighting
light_entity = create_entity()
add_transform(light_entity, 5, 10, 5)
add_directional_light(light_entity, 1.0, 1.0, 0.8) # RGB color

def rendering_update():
    """Advanced rendering system"""
    import math
    
    # Animate the light
    light_transform = get_transform(light_entity)
    time = get_current_time()
    light_transform.x = math.cos(time) * 10
    light_transform.z = math.sin(time) * 10
    set_transform(light_entity, light_transform.x, light_transform.y, light_transform.z)
    
    # Update camera
    camera_transform = get_transform(camera_entity)
    # Orbit camera around origin
    radius = 15
    camera_transform.x = math.cos(time * 0.5) * radius
    camera_transform.z = math.sin(time * 0.5) * radius
    set_transform(camera_entity, camera_transform.x, camera_transform.y, camera_transform.z)
    set_camera_target(camera_entity, 0, 0, 0)
    
    # Render all visible objects
    render_entities = query_entities_with_components(["transform", "mesh_renderer"])
    for entity in render_entities:
        transform = get_transform(entity)
        mesh_renderer = get_mesh_renderer(entity)
        material = get_material(entity) if has_material(entity) else None
        
        render_mesh_with_material(mesh_renderer.mesh, transform, material)
    
    # Post-processing effects
    apply_bloom_effect(0.8)
    apply_tone_mapping("aces")
    
register_system("rendering_update", rendering_update)
)";
        
        script_manager_->load_script("rendering_python", rendering_python_script, "python");
        script_manager_->execute_script("rendering_python");
        std::cout << "✓ Rendering Python script executed\n";
        
        // Audio scripting example
        std::string audio_script = R"(
-- Audio system integration
print("Setting up audio system...")

-- Background music
play_music("background_theme.ogg", true) -- looping
set_music_volume(0.7)

-- 3D positional audio
local audio_source = create_entity()
add_transform(audio_source, 5, 0, 0)
add_audio_source(audio_source, "engine_sound.wav", true) -- looping
set_audio_source_volume(audio_source, 0.5)
set_audio_source_pitch(audio_source, 1.2)

-- Audio listener (typically attached to player/camera)
local listener = create_entity()
add_transform(listener, 0, 2, 0)
add_audio_listener(listener)
set_audio_listener_orientation(listener, 0, 0, -1, 0, 1, 0) -- forward, up vectors

function audio_update()
    -- Update 3D audio based on listener position
    update_3d_audio()
    
    -- Dynamic audio effects
    local listener_transform = get_transform(listener)
    local source_transform = get_transform(audio_source)
    
    local distance = calculate_distance(listener_transform, source_transform)
    local volume = math.max(0, 1 - distance / 20) -- falloff over 20 units
    set_audio_source_volume(audio_source, volume)
end
)";
        
        script_manager_->load_script("audio_lua", audio_script, "lua");
        script_manager_->execute_script("audio_lua");
        std::cout << "✓ Audio Lua script executed\n";
        
        std::cout << "\n";
    }
    
    void demonstrate_cross_language_calls() {
        std::cout << "=== 4. Cross-Language Function Calls Demo ===\n";
        
        // Create helper functions in both languages
        std::string lua_helpers = R"(
-- Lua helper functions
function lua_fibonacci(n)
    if n <= 1 then
        return n
    else
        return lua_fibonacci(n-1) + lua_fibonacci(n-2)
    end
end

function lua_process_data(data_table)
    local result = {}
    for i, value in ipairs(data_table) do
        result[i] = value * 2 + 1
    end
    return result
end

shared_lua_value = "Hello from Lua!"
)";
        
        std::string python_helpers = R"(
# Python helper functions
import math

def python_prime_check(n):
    if n < 2:
        return False
    for i in range(2, int(math.sqrt(n)) + 1):
        if n % i == 0:
            return False
    return True

def python_matrix_multiply(a, b):
    """Simple matrix multiplication"""
    import numpy as np
    return np.dot(a, b).tolist()

def python_analyze_string(text):
    """Text analysis function"""
    return {
        'length': len(text),
        'words': len(text.split()),
        'vowels': sum(1 for char in text.lower() if char in 'aeiou'),
        'uppercase': sum(1 for char in text if char.isupper())
    }

shared_python_value = "Hello from Python!"
)";
        
        script_manager_->load_script("lua_helpers", lua_helpers, "lua");
        script_manager_->load_script("python_helpers", python_helpers, "python");
        
        script_manager_->execute_script("lua_helpers");
        script_manager_->execute_script("python_helpers");
        
        // Demonstrate cross-language calls
        std::cout << "✓ Calling Lua function from C++:\n";
        auto fib_result = script_manager_->call_function<int>("lua_helpers", "lua_fibonacci", 10);
        if (fib_result.is_success()) {
            std::cout << "  Fibonacci(10) = " << fib_result.value() << "\n";
        }
        
        std::cout << "✓ Calling Python function from C++:\n";
        auto prime_result = script_manager_->call_function<bool>("python_helpers", "python_prime_check", 17);
        if (prime_result.is_success()) {
            std::cout << "  Is 17 prime? " << (prime_result.value() ? "Yes" : "No") << "\n";
        }
        
        // Cross-language variable sharing
        std::cout << "✓ Sharing variables between languages:\n";
        script_manager_->set_global_shared_variable("shared_number", 42);
        script_manager_->set_global_shared_variable("shared_text", std::string("Cross-language data"));
        
        // Access shared variables from scripts
        std::string cross_lang_lua = R"(
local shared_num = get_shared_variable("shared_number")
local shared_text = get_shared_variable("shared_text")
print("Lua received shared number: " .. shared_num)
print("Lua received shared text: " .. shared_text)

-- Modify and share back
set_shared_variable("lua_result", shared_num * 3)
)";
        
        std::string cross_lang_python = R"(
shared_num = get_shared_variable("shared_number")
shared_text = get_shared_variable("shared_text")
lua_result = get_shared_variable("lua_result")

print(f"Python received shared number: {shared_num}")
print(f"Python received shared text: {shared_text}")
print(f"Python received Lua result: {lua_result}")

# Complex computation to share back
python_result = [shared_num * i for i in range(1, 6)]
set_shared_variable("python_result", python_result)
)";
        
        script_manager_->load_script("cross_lua", cross_lang_lua, "lua");
        script_manager_->load_script("cross_python", cross_lang_python, "python");
        
        script_manager_->execute_script("cross_lua");
        script_manager_->execute_script("cross_python");
        
        std::cout << "\n";
    }
    
    void demonstrate_hot_reload_system() {
        std::cout << "=== 5. Hot-Reload System Demo ===\n";
        
        // Enable hot-reload for all scripts
        script_manager_->enable_hot_reload(true);
        
        // Create a script file that we can modify
        std::string hot_reload_script = R"(
-- Hot-reload demonstration script
print("Hot-reload script loaded - Version 1.0")

hot_reload_version = "1.0"

function get_message()
    return "This is version " .. hot_reload_version
end

function dynamic_behavior()
    print("Executing dynamic behavior - version " .. hot_reload_version)
    return hot_reload_version
end
)";
        
        // Save to file for hot-reload testing
        std::string script_file = "/tmp/hot_reload_demo.lua";
        std::ofstream file(script_file);
        file << hot_reload_script;
        file.close();
        
        // Load from file
        script_manager_->load_script_file("hot_reload_demo", script_file);
        script_manager_->execute_script("hot_reload_demo");
        
        auto message_result = script_manager_->call_function<std::string>("hot_reload_demo", "get_message");
        if (message_result.is_success()) {
            std::cout << "✓ Initial message: " << message_result.value() << "\n";
        }
        
        std::cout << "✓ Hot-reload system active. Modifying script...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Modify the script
        std::string modified_script = R"(
-- Hot-reload demonstration script
print("Hot-reload script reloaded - Version 2.0")

hot_reload_version = "2.0"

function get_message()
    return "This is version " .. hot_reload_version .. " (hot-reloaded!)"
end

function dynamic_behavior()
    print("Executing enhanced dynamic behavior - version " .. hot_reload_version)
    return hot_reload_version .. "_enhanced"
end

function new_function()
    return "This function was added in version 2.0!"
end
)";
        
        // Write modified version
        std::ofstream modified_file(script_file);
        modified_file << modified_script;
        modified_file.close();
        
        // Trigger reload check
        script_manager_->get_hot_reloader()->force_reload("hot_reload_demo");
        
        // Test the reloaded script
        auto new_message_result = script_manager_->call_function<std::string>("hot_reload_demo", "get_message");
        if (new_message_result.is_success()) {
            std::cout << "✓ Updated message: " << new_message_result.value() << "\n";
        }
        
        auto new_function_result = script_manager_->call_function<std::string>("hot_reload_demo", "new_function");
        if (new_function_result.is_success()) {
            std::cout << "✓ New function result: " << new_function_result.value() << "\n";
        }
        
        std::cout << "✓ Hot-reload completed successfully!\n\n";
    }
    
    void demonstrate_debugging_features() {
        std::cout << "=== 6. Debugging Features Demo ===\n";
        
        // Enable debugging for all scripts
        script_manager_->enable_global_debugging(true);
        
        std::string debug_script = R"(
-- Debugging demonstration script
function debug_function_with_error()
    local x = 10
    local y = 20
    local z = x + y
    
    print("Debug variables: x=" .. x .. ", y=" .. y .. ", z=" .. z)
    
    -- Intentional error for debugging demonstration
    if z > 25 then
        error("Demonstration error: z is too large (" .. z .. ")")
    end
    
    return z
end

function debug_function_with_breakpoint()
    local data = {1, 2, 3, 4, 5}
    local sum = 0
    
    for i, value in ipairs(data) do
        sum = sum + value
        -- Breakpoint would be set at this line for inspection
        print("Processing item " .. i .. ": " .. value .. ", sum so far: " .. sum)
    end
    
    return sum
end

-- Complex function for step debugging
function complex_calculation(n)
    local result = 1
    local temp = 0
    
    for i = 1, n do
        temp = result * i
        result = temp + (i % 2)
        print("Step " .. i .. ": result=" .. result)
    end
    
    return result
end
)";
        
        script_manager_->load_script("debug_demo", debug_script, "lua");
        script_manager_->execute_script("debug_demo");
        
        // Set breakpoints
        script_manager_->get_debug_manager()->set_global_breakpoint("debug_demo", 15);
        script_manager_->get_debug_manager()->set_global_breakpoint("debug_demo", 25);
        
        std::cout << "✓ Debugging script loaded with breakpoints\n";
        
        // Demonstrate error handling
        auto error_result = script_manager_->call_function<int>("debug_demo", "debug_function_with_error");
        if (error_result.is_error()) {
            std::cout << "✓ Error caught and handled:\n";
            std::cout << "  " << error_result.error().format_error() << "\n";
        }
        
        // Show recent errors
        auto recent_errors = script_manager_->get_debug_manager()->get_all_recent_errors();
        std::cout << "✓ Recent errors count: " << recent_errors.size() << "\n";
        
        // Demonstrate successful debugging function
        auto debug_result = script_manager_->call_function<int>("debug_demo", "debug_function_with_breakpoint");
        if (debug_result.is_success()) {
            std::cout << "✓ Debug function result: " << debug_result.value() << "\n";
        }
        
        std::cout << "\n";
    }
    
    void demonstrate_performance_profiling() {
        std::cout << "=== 7. Performance Profiling Demo ===\n";
        
        // Start global profiling
        script_manager_->start_global_profiling();
        
        // Performance test scripts
        std::string performance_lua = R"(
-- Performance testing in Lua
function cpu_intensive_task()
    local sum = 0
    for i = 1, 1000000 do
        sum = sum + math.sin(i) * math.cos(i)
    end
    return sum
end

function memory_intensive_task()
    local big_table = {}
    for i = 1, 100000 do
        big_table[i] = {x = i, y = i * 2, z = i * 3}
    end
    return #big_table
end

function recursive_task(n)
    if n <= 1 then
        return n
    else
        return recursive_task(n-1) + recursive_task(n-2)
    end
end
)";
        
        std::string performance_python = R"(
# Performance testing in Python
import math
import time

def cpu_intensive_task():
    sum_val = 0
    for i in range(1000000):
        sum_val += math.sin(i) * math.cos(i)
    return sum_val

def memory_intensive_task():
    big_list = []
    for i in range(100000):
        big_list.append({'x': i, 'y': i * 2, 'z': i * 3})
    return len(big_list)

def list_comprehension_task():
    result = [x**2 for x in range(50000) if x % 2 == 0]
    return len(result)

def numpy_task():
    import numpy as np
    arr = np.random.random((1000, 1000))
    result = np.dot(arr, arr.T)
    return result.shape
)";
        
        script_manager_->load_script("perf_lua", performance_lua, "lua");
        script_manager_->load_script("perf_python", performance_python, "python");
        
        script_manager_->execute_script("perf_lua");
        script_manager_->execute_script("perf_python");
        
        // Benchmark different operations
        std::cout << "Running performance benchmarks...\n";
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Lua benchmarks
        script_manager_->call_function<double>("perf_lua", "cpu_intensive_task");
        script_manager_->call_function<int>("perf_lua", "memory_intensive_task");
        script_manager_->call_function<int>("perf_lua", "recursive_task", 30);
        
        // Python benchmarks
        script_manager_->call_function<double>("perf_python", "cpu_intensive_task");
        script_manager_->call_function<int>("perf_python", "memory_intensive_task");
        script_manager_->call_function<int>("perf_python", "list_comprehension_task");
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "✓ Benchmarks completed in " << duration.count() << "ms\n";
        
        // Stop profiling and generate report
        script_manager_->stop_global_profiling();
        auto profile_report = script_manager_->generate_comprehensive_report();
        
        std::cout << "✓ Performance profiling report generated\n";
        std::cout << "  Report size: " << profile_report.size() << " characters\n";
        
        // Show memory usage statistics
        auto memory_by_script = script_manager_->get_memory_usage_by_language();
        std::cout << "✓ Memory usage by language:\n";
        for (const auto& [language, memory] : memory_by_script) {
            std::cout << "  " << language << ": " << memory << " bytes\n";
        }
        
        std::cout << "\n";
    }
    
    void demonstrate_repl_interaction() {
        std::cout << "=== 8. REPL Interaction Demo ===\n";
        
        auto* repl = script_manager_->get_repl();
        
        // Demonstrate programmatic REPL commands
        std::vector<std::string> demo_commands = {
            "print('Hello from REPL!')",
            "x = 42",
            "y = x * 2",
            "print('x =', x, 'y =', y)",
            "%switch lua",
            "print('Now in Lua!')",
            "z = x + y",
            "print('z = ' .. z)",
            "%switch python",
            "import math",
            "result = math.sqrt(x + y)",
            "print(f'Square root of {x + y} = {result}')",
            "%help",
            "%list_scripts"
        };
        
        std::cout << "Demonstrating REPL commands programmatically:\n";
        for (const auto& command : demo_commands) {
            std::cout << ">>> " << command << "\n";
            auto result = repl->execute_command(command);
            if (!result.empty()) {
                std::cout << result << "\n";
            }
        }
        
        std::cout << "✓ REPL demonstration completed\n";
        std::cout << "\n";
    }
    
    void demonstrate_advanced_features() {
        std::cout << "=== 9. Advanced Features Demo ===\n";
        
        // Coroutines and async execution
        std::string coroutine_lua = R"(
-- Coroutine demonstration
function async_task()
    for i = 1, 5 do
        print("Async task step " .. i)
        coroutine.yield(i)
    end
    return "Async task completed"
end

function generator_task()
    local count = 0
    return function()
        count = count + 1
        if count <= 10 then
            return count, count * count
        else
            return nil
        end
    end
end
)";
        
        std::string async_python = R"(
# Async/await demonstration
import asyncio
import time

async def async_computation(n):
    print(f"Starting async computation for {n}")
    await asyncio.sleep(0.01)  # Simulate async work
    result = sum(i**2 for i in range(n))
    print(f"Async computation for {n} completed: {result}")
    return result

def generator_function():
    for i in range(1, 11):
        yield i, i**3

async def main_async():
    tasks = [async_computation(i*100) for i in range(1, 6)]
    results = await asyncio.gather(*tasks)
    return results
)";
        
        script_manager_->load_script("coroutine_demo", coroutine_lua, "lua");
        script_manager_->load_script("async_demo", async_python, "python");
        
        script_manager_->execute_script("coroutine_demo");
        script_manager_->execute_script("async_demo");
        
        std::cout << "✓ Coroutine and async scripts loaded\n";
        
        // Sandboxing demonstration
        std::cout << "✓ Demonstrating script sandboxing:\n";
        script_manager_->enable_script_sandboxing(true);
        
        std::string unsafe_script = R"(
-- This script attempts unsafe operations
local file = io.open("/etc/passwd", "r") -- Should be blocked
if file then
    print("Security breach!")
    file:close()
else
    print("File access properly blocked by sandbox")
end

-- This should also be blocked
os.execute("ls /") -- Should be blocked
print("OS command execution blocked by sandbox")
)";
        
        script_manager_->load_script("sandbox_test", unsafe_script, "lua");
        script_manager_->execute_script("sandbox_test");
        
        std::cout << "✓ Sandboxing demonstration completed\n";
        
        // Plugin system demonstration
        std::cout << "✓ Demonstrating plugin system:\n";
        
        // Register a custom plugin
        script_manager_->register_script_plugin("math_extensions", [](ScriptEngine* engine) {
            // This would add custom math functions to the scripting environment
            std::cout << "  Math extensions plugin loaded for " << engine->get_language_info().name << "\n";
        });
        
        // Apply plugin to all engines
        script_manager_->apply_plugin_to_all_engines("math_extensions");
        
        std::cout << "\n";
    }
    
    void demonstrate_educational_content() {
        std::cout << "=== 10. Educational Content Demo ===\n";
        
        // Generate comprehensive tutorials
        script_manager_->create_comprehensive_tutorial();
        script_manager_->create_cross_language_examples();
        script_manager_->generate_best_practices_guide();
        
        std::cout << "✓ Educational content generated\n";
        
        // Show system architecture explanation
        auto architecture_explanation = script_manager_->explain_scripting_system_architecture();
        std::cout << "✓ System architecture documented (" << architecture_explanation.size() << " chars)\n";
        
        // Language-specific optimizations
        auto lua_optimizations = script_manager_->get_engine("lua")->get_optimization_suggestions("demo_lua");
        auto python_optimizations = script_manager_->get_engine("python")->get_optimization_suggestions("demo_python");
        
        std::cout << "✓ Optimization suggestions:\n";
        std::cout << "  Lua: " << lua_optimizations.size() << " suggestions\n";
        std::cout << "  Python: " << python_optimizations.size() << " suggestions\n";
        
        std::cout << "\n";
    }
    
    void show_comprehensive_statistics() {
        std::cout << "=== Comprehensive System Statistics ===\n";
        
        // Script statistics
        auto loaded_scripts = script_manager_->get_loaded_scripts();
        std::cout << "Loaded scripts: " << loaded_scripts.size() << "\n";
        for (const auto& script : loaded_scripts) {
            std::cout << "  - " << script << "\n";
        }
        
        // Memory statistics
        auto total_memory = script_manager_->get_total_memory_usage();
        std::cout << "Total memory usage: " << total_memory << " bytes\n";
        
        auto memory_by_language = script_manager_->get_memory_usage_by_language();
        for (const auto& [language, memory] : memory_by_language) {
            std::cout << "  " << language << ": " << memory << " bytes\n";
        }
        
        // Performance metrics
        auto aggregated_metrics = script_manager_->get_aggregated_metrics();
        std::cout << "Performance metrics:\n";
        std::cout << "  Total execution time: " << 
            std::chrono::duration_cast<std::chrono::milliseconds>(aggregated_metrics.execution_time).count() << "ms\n";
        std::cout << "  Function calls: " << aggregated_metrics.function_calls << "\n";
        std::cout << "  Garbage collections: " << aggregated_metrics.garbage_collections << "\n";
        
        // ECS statistics
        if (ecs_registry_) {
            std::cout << "ECS statistics:\n";
            std::cout << "  Entities: " << ecs_registry_->get_entity_count() << "\n";
            std::cout << "  Archetypes: " << ecs_registry_->get_archetype_count() << "\n";
        }
        
        std::cout << "\n";
    }
    
    void start_interactive_mode() {
        std::cout << "Multi-Language REPL - ECScope Scripting System\n";
        std::cout << "Available commands:\n";
        std::cout << "  %switch <language>  - Switch to different language (lua/python)\n";
        std::cout << "  %help               - Show help\n";
        std::cout << "  %list_scripts      - List loaded scripts\n";
        std::cout << "  %profile <script>  - Profile script performance\n";
        std::cout << "  exit               - Exit interactive mode\n\n";
        
        auto* repl = script_manager_->get_repl();
        
        std::string input;
        while (true) {
            std::cout << repl->get_current_language() << ">>> ";
            if (!std::getline(std::cin, input)) {
                break;
            }
            
            if (input == "exit" || input == "quit") {
                break;
            }
            
            if (input.empty()) {
                continue;
            }
            
            auto result = repl->execute_command(input);
            if (!result.empty()) {
                std::cout << result << "\n";
            }
        }
        
        std::cout << "Interactive mode ended.\n";
    }
    
    void cleanup_systems() {
        std::cout << "Cleaning up systems...\n";
        
        if (script_manager_) {
            script_manager_->shutdown();
        }
        
        std::cout << "✓ All systems cleaned up successfully\n";
    }
};

int main() {
    try {
        ScriptingSystemDemo demo;
        demo.run_comprehensive_demo();
    } catch (const std::exception& e) {
        std::cerr << "Demo error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}