/**
 * @file examples/scripting_examples.cpp
 * @brief Comprehensive Educational Examples for ECScope Scripting Integration
 * 
 * This file provides extensive examples demonstrating the scripting capabilities
 * of the ECScope ECS engine with educational focus and real-world use cases.
 * 
 * Examples Include:
 * - Basic Python and Lua scripting integration
 * - ECS component manipulation from scripts
 * - Advanced hot-reload scenarios with state preservation
 * - Performance profiling and optimization techniques
 * - Multi-language interoperability examples
 * - Real-world game development scenarios
 * - Educational debugging and performance analysis
 * 
 * @author ECScope Educational ECS Framework - Scripting Examples
 * @date 2025
 */

#include "scripting/python_integration.hpp"
#include "scripting/lua_integration.hpp"
#include "scripting/hot_reload_system.hpp"
#include "scripting/ecs_script_interface.hpp"
#include "scripting/script_profiler.hpp"
#include "ecs/registry.hpp"
#include "ecs/components/transform.hpp"
#include "core/log.hpp"
#include "memory/advanced_memory_system.hpp"
#include "job_system/work_stealing_job_system.hpp"

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <fstream>
#include <string>

using namespace ecscope;

//=============================================================================
// Example Components for Scripting Demonstrations
//=============================================================================

struct Position {
    f32 x = 0.0f;
    f32 y = 0.0f;
    f32 z = 0.0f;
    
    Position() = default;
    Position(f32 x_, f32 y_, f32 z_ = 0.0f) : x(x_), y(y_), z(z_) {}
};

struct Velocity {
    f32 dx = 0.0f;
    f32 dy = 0.0f;
    f32 dz = 0.0f;
    
    Velocity() = default;
    Velocity(f32 dx_, f32 dy_, f32 dz_ = 0.0f) : dx(dx_), dy(dy_), dz(dz_) {}
};

struct Health {
    i32 current = 100;
    i32 maximum = 100;
    f32 regeneration_rate = 1.0f;
    
    Health() = default;
    Health(i32 max_health) : current(max_health), maximum(max_health) {}
};

struct PlayerTag {};

struct AIController {
    std::string behavior = "idle";
    f32 decision_timer = 0.0f;
    f32 decision_interval = 1.0f;
};

//=============================================================================
// Educational Example Classes
//=============================================================================

class ScriptingExamples {
private:
    std::unique_ptr<memory::AdvancedMemorySystem> memory_system_;
    std::unique_ptr<job_system::JobSystem> job_system_;
    std::unique_ptr<ecs::Registry> registry_;
    std::unique_ptr<scripting::python::PythonEngine> python_engine_;
    std::unique_ptr<scripting::lua::LuaEngine> lua_engine_;
    std::unique_ptr<scripting::hotreload::HotReloadSystem> hot_reload_system_;
    std::unique_ptr<scripting::ecs_interface::ECSScriptInterface> ecs_interface_;
    std::unique_ptr<scripting::profiling::FunctionProfiler> function_profiler_;
    std::unique_ptr<scripting::profiling::MemoryProfiler> memory_profiler_;
    std::unique_ptr<scripting::profiling::PerformanceAnalyzer> performance_analyzer_;
    
public:
    ScriptingExamples() {
        initialize_systems();
        setup_scripting_environment();
    }
    
    ~ScriptingExamples() {
        shutdown_systems();
    }
    
    void run_all_examples() {
        LOG_INFO("=== ECScope Scripting Integration Examples ===");
        
        // Basic examples
        example_01_basic_python_scripting();
        example_02_basic_lua_scripting();
        example_03_ecs_component_manipulation();
        
        // Intermediate examples
        example_04_cross_language_communication();
        example_05_hot_reload_demonstration();
        example_06_performance_profiling();
        
        // Advanced examples
        example_07_game_ai_scripting();
        example_08_parallel_script_execution();
        example_09_memory_optimization();
        
        // Real-world scenarios
        example_10_complete_game_system();
        
        LOG_INFO("=== All Scripting Examples Completed ===");
    }

private:
    void initialize_systems() {
        LOG_INFO("Initializing ECScope systems for scripting examples...");
        
        // Memory system
        memory_system_ = std::make_unique<memory::AdvancedMemorySystem>(
            memory::AdvancedMemorySystem::Config{
                .enable_pool_allocation = true,
                .enable_numa_awareness = true,
                .enable_profiling = true
            }
        );
        
        // Job system
        job_system_ = std::make_unique<job_system::JobSystem>(
            job_system::JobSystem::Config::create_educational()
        );
        job_system_->initialize();
        
        // ECS registry
        registry_ = std::make_unique<ecs::Registry>();
        
        LOG_INFO("Core systems initialized successfully");
    }
    
    void setup_scripting_environment() {
        LOG_INFO("Setting up scripting environment...");
        
        // Python engine
        python_engine_ = std::make_unique<scripting::python::PythonEngine>(*memory_system_);
        if (!python_engine_->initialize()) {
            LOG_ERROR("Failed to initialize Python engine");
            return;
        }
        
        // Register components for Python
        python_engine_->register_component<Position>("Position");
        python_engine_->register_component<Velocity>("Velocity");
        python_engine_->register_component<Health>("Health");
        
        // Lua engine would be initialized similarly
        // lua_engine_ = std::make_unique<scripting::lua::LuaEngine>(*memory_system_);
        
        // ECS script interface
        ecs_interface_ = std::make_unique<scripting::ecs_interface::ECSScriptInterface>(
            registry_.get(), python_engine_.get(), lua_engine_.get()
        );
        
        // Register component types for scripting
        ecs_interface_->register_component_type<Position>("Position");
        ecs_interface_->register_component_type<Velocity>("Velocity");
        ecs_interface_->register_component_type<Health>("Health");
        ecs_interface_->register_component_type<PlayerTag>("PlayerTag");
        ecs_interface_->register_component_type<AIController>("AIController");
        
        // Profiling setup
        function_profiler_ = std::make_unique<scripting::profiling::FunctionProfiler>(
            scripting::profiling::FunctionProfiler::ProfilingMode::Full
        );
        
        memory_profiler_ = std::make_unique<scripting::profiling::MemoryProfiler>();
        
        performance_analyzer_ = std::make_unique<scripting::profiling::PerformanceAnalyzer>(
            function_profiler_.get(), memory_profiler_.get()
        );
        
        LOG_INFO("Scripting environment setup completed");
    }
    
    void shutdown_systems() {
        LOG_INFO("Shutting down ECScope systems...");
        
        if (python_engine_) {
            python_engine_->shutdown();
        }
        
        if (job_system_) {
            job_system_->shutdown();
        }
        
        LOG_INFO("Systems shutdown completed");
    }
    
    // Example 1: Basic Python Scripting
    void example_01_basic_python_scripting() {
        LOG_INFO("=== Example 1: Basic Python Scripting ===");
        
        // Simple Python code execution
        std::string python_code = R"(
# Basic ECScope Python scripting example
import math

def calculate_distance(x1, y1, x2, y2):
    """Calculate distance between two points."""
    return math.sqrt((x2 - x1)**2 + (y2 - y1)**2)

def fibonacci(n):
    """Calculate nth Fibonacci number."""
    if n <= 1:
        return n
    return fibonacci(n-1) + fibonacci(n-2)

# Test the functions
distance = calculate_distance(0, 0, 3, 4)
fib_10 = fibonacci(10)

print(f"Distance from (0,0) to (3,4): {distance}")
print(f"10th Fibonacci number: {fib_10}")
)";
        
        auto result = python_engine_->execute_string(python_code);
        if (!result) {
            LOG_ERROR("Failed to execute Python code");
            return;
        }
        
        // Demonstrate calling Python functions from C++
        python_engine_->set_global("x1", PyFloat_FromDouble(1.0));
        python_engine_->set_global("y1", PyFloat_FromDouble(2.0));
        python_engine_->set_global("x2", PyFloat_FromDouble(4.0));
        python_engine_->set_global("y2", PyFloat_FromDouble(6.0));
        
        auto distance_result = python_engine_->execute_string("calculate_distance(x1, y1, x2, y2)");
        if (distance_result && distance_result.is_float()) {
            LOG_INFO("Distance calculated from C++: {}", distance_result.to_float());
        }
        
        // Show Python engine statistics
        auto stats = python_engine_->get_statistics();
        LOG_INFO("Python execution stats - Scripts: {}, Exceptions: {}, Memory: {} KB",
                stats.scripts_executed, stats.exceptions_thrown, 
                stats.memory_stats.current_allocated / 1024);
        
        LOG_INFO("Basic Python scripting example completed");
    }
    
    // Example 2: Basic Lua Scripting  
    void example_02_basic_lua_scripting() {
        LOG_INFO("=== Example 2: Basic Lua Scripting ===");
        
        // Note: This is a conceptual example - actual Lua integration would be implemented
        LOG_INFO("Lua scripting example - would demonstrate:");
        LOG_INFO("- Basic Lua function calls");
        LOG_INFO("- Coroutine usage for game logic");
        LOG_INFO("- C++ to Lua data passing");
        LOG_INFO("- Performance comparison with Python");
        
        LOG_INFO("Basic Lua scripting example completed");
    }
    
    // Example 3: ECS Component Manipulation from Scripts
    void example_03_ecs_component_manipulation() {
        LOG_INFO("=== Example 3: ECS Component Manipulation ===");
        
        // Create some entities
        auto player = ecs_interface_->create_entity();
        auto enemy1 = ecs_interface_->create_entity();
        auto enemy2 = ecs_interface_->create_entity();
        
        // Add components via script interface
        player->add_component<Position>(0.0f, 0.0f, 0.0f);
        player->add_component<Velocity>(1.0f, 0.0f, 0.0f);
        player->add_component<Health>(100);
        player->add_component<PlayerTag>();
        
        enemy1->add_component<Position>(10.0f, 5.0f, 0.0f);
        enemy1->add_component<Velocity>(-0.5f, 0.0f, 0.0f);
        enemy1->add_component<Health>(50);
        enemy1->add_component<AIController>();
        
        enemy2->add_component<Position>(-5.0f, -3.0f, 0.0f);
        enemy2->add_component<Health>(30);
        
        LOG_INFO("Created entities - Player: {}, Enemy1: {}, Enemy2: {}", 
                player->id(), enemy1->id(), enemy2->id());
        
        // Python script to manipulate components
        std::string component_script = R"(
# ECS Component manipulation example
print("=== ECS Component Manipulation from Python ===")

# This would be the actual implementation with proper bindings
# For demonstration, we'll show the conceptual approach

def update_player_position(entity, delta_time):
    """Update player position based on velocity."""
    position = entity.get_component('Position')
    velocity = entity.get_component('Velocity')
    
    if position and velocity:
        position.x += velocity.dx * delta_time
        position.y += velocity.dy * delta_time
        position.z += velocity.dz * delta_time
        
        print(f"Player moved to: ({position.x:.2f}, {position.y:.2f}, {position.z:.2f})")

def apply_damage(entity, damage):
    """Apply damage to an entity's health."""
    health = entity.get_component('Health')
    
    if health:
        health.current -= damage
        health.current = max(0, health.current)
        
        print(f"Entity took {damage} damage. Health: {health.current}/{health.maximum}")
        
        if health.current <= 0:
            print("Entity destroyed!")
            return True  # Entity should be destroyed
    
    return False

def ai_behavior(entity, delta_time):
    """Simple AI behavior for enemies."""
    ai = entity.get_component('AIController')
    position = entity.get_component('Position')
    
    if ai and position:
        ai.decision_timer += delta_time
        
        if ai.decision_timer >= ai.decision_interval:
            # Make AI decision
            if ai.behavior == "idle":
                ai.behavior = "patrol"
                print(f"AI switching to patrol at position ({position.x:.1f}, {position.y:.1f})")
            elif ai.behavior == "patrol":
                ai.behavior = "idle"
                print(f"AI switching to idle")
            
            ai.decision_timer = 0.0

# Demonstration of script-driven entity updates
print("Script-driven ECS updates would be called here")
)";
        
        auto script_result = python_engine_->execute_string(component_script);
        
        // Query demonstration
        auto position_query = ecs_interface_->create_query<Position>();
        auto moving_entities_query = ecs_interface_->create_query<Position, Velocity>();
        
        LOG_INFO("Total entities with Position: {}", position_query->count());
        LOG_INFO("Total moving entities: {}", moving_entities_query->count());
        
        // Demonstrate component access
        if (auto* pos = player->get_component<Position>()) {
            LOG_INFO("Player position: ({}, {}, {})", pos->x, pos->y, pos->z);
        }
        
        LOG_INFO("ECS component manipulation example completed");
    }
    
    // Example 4: Cross-Language Communication
    void example_04_cross_language_communication() {
        LOG_INFO("=== Example 4: Cross-Language Communication ===");
        
        // Demonstrate data sharing between Python and potential Lua scripts
        std::string python_producer = R"(
# Python data producer
import json
import time

class DataProducer:
    def __init__(self):
        self.data = {
            'timestamp': time.time(),
            'entities': [
                {'id': 1, 'type': 'player', 'health': 100},
                {'id': 2, 'type': 'enemy', 'health': 50},
                {'id': 3, 'type': 'item', 'value': 25}
            ],
            'world_state': {
                'time_of_day': 'noon',
                'weather': 'clear',
                'temperature': 22.5
            }
        }
    
    def get_json_data(self):
        return json.dumps(self.data, indent=2)
    
    def update_entity_health(self, entity_id, new_health):
        for entity in self.data['entities']:
            if entity['id'] == entity_id:
                entity['health'] = new_health
                print(f"Updated entity {entity_id} health to {new_health}")
                break

# Create producer and generate data
producer = DataProducer()
shared_data = producer.get_json_data()
print("Generated shared data for inter-language communication")
)";
        
        python_engine_->execute_string(python_producer);
        
        // Get shared data from Python
        auto shared_data = python_engine_->get_global("shared_data");
        if (shared_data && shared_data.is_string()) {
            std::string json_data = shared_data.to_string();
            LOG_INFO("Received data from Python: {} bytes", json_data.length());
            
            // This data could now be passed to Lua or used in C++
            // In a real implementation, this would demonstrate:
            // - Python generating AI behavior data
            // - Lua consuming and executing behaviors  
            // - C++ coordinating the communication
        }
        
        LOG_INFO("Cross-language communication example completed");
    }
    
    // Example 5: Hot-Reload Demonstration
    void example_05_hot_reload_demonstration() {
        LOG_INFO("=== Example 5: Hot-Reload Demonstration ===");
        
        // Create a temporary script file for hot-reload testing
        std::string temp_script_path = "temp_hotreload_example.py";
        create_temporary_script(temp_script_path, 1);
        
        // Initial script execution
        LOG_INFO("Executing initial script version...");
        python_engine_->execute_file(temp_script_path);
        
        // Simulate script modification and reload
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        LOG_INFO("Modifying script and demonstrating hot-reload...");
        create_temporary_script(temp_script_path, 2);
        
        // In a real hot-reload system, this would be automatically detected
        LOG_INFO("Script modified - hot-reload would be triggered automatically");
        python_engine_->execute_file(temp_script_path);
        
        // Cleanup
        std::remove(temp_script_path.c_str());
        
        LOG_INFO("Hot-reload demonstration completed");
    }
    
    // Example 6: Performance Profiling
    void example_06_performance_profiling() {
        LOG_INFO("=== Example 6: Performance Profiling ===");
        
        function_profiler_->start_profiling();
        memory_profiler_->start_tracking();
        
        // Execute performance-intensive Python code
        std::string performance_test = R"(
import time

def expensive_function():
    """Simulate expensive computation."""
    result = 0
    for i in range(100000):
        result += i * i
    return result

def memory_intensive_function():
    """Simulate memory-intensive operations."""
    data = []
    for i in range(10000):
        data.append([x * x for x in range(100)])
    return len(data)

def recursive_fibonacci(n):
    """Inefficient recursive Fibonacci for profiling demonstration."""
    if n <= 1:
        return n
    return recursive_fibonacci(n-1) + recursive_fibonacci(n-2)

# Profile function calls
start_time = time.time()

for i in range(10):
    expensive_function()
    
memory_data = memory_intensive_function()
fib_result = recursive_fibonacci(25)

end_time = time.time()

print(f"Performance test completed in {(end_time - start_time)*1000:.2f} ms")
print(f"Memory data size: {memory_data}")
print(f"Fibonacci result: {fib_result}")
)";
        
        auto profiler = function_profiler_->profile_function("performance_test_execution");
        python_engine_->execute_string(performance_test);
        
        // Stop profiling and analyze results
        function_profiler_->stop_profiling();
        memory_profiler_->stop_tracking();
        
        // Generate performance report
        auto performance_report = performance_analyzer_->generate_report();
        
        LOG_INFO("=== Performance Analysis Results ===");
        LOG_INFO("Total execution time: {:.2f} ms", performance_report.total_execution_time_ms);
        LOG_INFO("Memory usage: {} KB", performance_report.memory_usage_bytes / 1024);
        LOG_INFO("Memory peak: {} KB", performance_report.memory_peak_bytes / 1024);
        LOG_INFO("Overall performance score: {}/100", performance_report.overall_performance_score);
        
        // Show top functions
        LOG_INFO("Top performance hotspots:");
        for (usize i = 0; i < std::min(size_t(5), performance_report.top_functions.size()); ++i) {
            const auto& func = performance_report.top_functions[i];
            LOG_INFO("  {}. {} - {:.3f} ms ({} calls)", 
                    i + 1, func.function_name, func.average_time_ms(), func.call_count);
        }
        
        // Export detailed report
        performance_analyzer_->export_performance_report("performance_report.txt");
        
        LOG_INFO("Performance profiling example completed");
    }
    
    // Example 7: Game AI Scripting
    void example_07_game_ai_scripting() {
        LOG_INFO("=== Example 7: Game AI Scripting ===");
        
        // Create AI entities
        std::vector<scripting::ecs_interface::ScriptEntity*> ai_entities;
        
        for (int i = 0; i < 5; ++i) {
            auto entity = ecs_interface_->create_entity();
            entity->add_component<Position>(
                static_cast<f32>(i * 2), 
                static_cast<f32>(i % 2), 
                0.0f
            );
            entity->add_component<Health>(50 + i * 10);
            entity->add_component<AIController>();
            
            auto* ai = entity->get_component<AIController>();
            ai->behavior = (i % 2 == 0) ? "patrol" : "guard";
            ai->decision_interval = 1.0f + i * 0.5f;
            
            ai_entities.push_back(entity);
        }
        
        LOG_INFO("Created {} AI entities", ai_entities.size());
        
        // AI behavior script
        std::string ai_script = R"(
import random
import math

class AIBehaviorSystem:
    def __init__(self):
        self.behaviors = {
            'idle': self.idle_behavior,
            'patrol': self.patrol_behavior,
            'guard': self.guard_behavior,
            'pursue': self.pursue_behavior,
            'flee': self.flee_behavior
        }
    
    def update_ai(self, entity, delta_time, player_position=None):
        """Update AI behavior for an entity."""
        ai = entity.get_component('AIController')
        position = entity.get_component('Position')
        health = entity.get_component('Health')
        
        if not (ai and position and health):
            return
        
        # Update decision timer
        ai.decision_timer += delta_time
        
        # Make behavior decisions
        if ai.decision_timer >= ai.decision_interval:
            self.make_decision(entity, ai, position, health, player_position)
            ai.decision_timer = 0.0
        
        # Execute current behavior
        if ai.behavior in self.behaviors:
            self.behaviors[ai.behavior](entity, delta_time)
    
    def make_decision(self, entity, ai, position, health, player_pos):
        """Make AI behavioral decisions based on current state."""
        # Health-based decisions
        health_ratio = health.current / health.maximum
        
        if health_ratio < 0.3:
            ai.behavior = 'flee'
            print(f"AI Entity {entity.id()} is fleeing (low health: {health.current})")
        elif health_ratio < 0.6 and random.random() < 0.3:
            ai.behavior = 'guard'
            print(f"AI Entity {entity.id()} switching to defensive guard")
        elif player_pos and self.distance_to_player(position, player_pos) < 5.0:
            ai.behavior = 'pursue'
            print(f"AI Entity {entity.id()} detected player, pursuing!")
        elif ai.behavior == 'flee' and health_ratio > 0.5:
            ai.behavior = 'patrol'
            print(f"AI Entity {entity.id()} recovered, resuming patrol")
        elif random.random() < 0.2:
            # Random behavior change
            new_behavior = random.choice(['idle', 'patrol', 'guard'])
            if new_behavior != ai.behavior:
                ai.behavior = new_behavior
                print(f"AI Entity {entity.id()} randomly switching to {new_behavior}")
    
    def distance_to_player(self, ai_pos, player_pos):
        """Calculate distance between AI and player."""
        dx = ai_pos.x - player_pos[0]
        dy = ai_pos.y - player_pos[1]
        return math.sqrt(dx*dx + dy*dy)
    
    def idle_behavior(self, entity, delta_time):
        """AI stands still and occasionally looks around."""
        pass  # No movement for idle
    
    def patrol_behavior(self, entity, delta_time):
        """AI moves in a pattern."""
        position = entity.get_component('Position')
        if position:
            # Simple circular patrol pattern
            position.x += math.sin(entity.id() * 0.1) * delta_time * 2.0
            position.y += math.cos(entity.id() * 0.1) * delta_time * 2.0
    
    def guard_behavior(self, entity, delta_time):
        """AI stays in place but rotates to watch area."""
        pass  # Guarding - minimal movement
    
    def pursue_behavior(self, entity, delta_time):
        """AI moves toward player position."""
        # Would implement pathfinding toward player
        pass
    
    def flee_behavior(self, entity, delta_time):
        """AI runs away from threats."""
        position = entity.get_component('Position')
        if position:
            # Simple flee pattern - move away from center
            direction_x = position.x / max(abs(position.x), 1.0)
            direction_y = position.y / max(abs(position.y), 1.0)
            
            position.x += direction_x * delta_time * 3.0
            position.y += direction_y * delta_time * 3.0

# Create AI system
ai_system = AIBehaviorSystem()
print("Game AI behavior system initialized")
)";
        
        python_engine_->execute_string(ai_script);
        
        // Simulate AI updates
        f32 delta_time = 0.016f; // 60 FPS
        f32 total_time = 0.0f;
        
        LOG_INFO("Simulating AI behavior updates...");
        
        for (int frame = 0; frame < 180; ++frame) { // 3 seconds at 60 FPS
            total_time += delta_time;
            
            // Update each AI entity
            for (auto* entity : ai_entities) {
                // In a real implementation, this would call the Python AI system
                if (auto* ai = entity->get_component<AIController>()) {
                    ai->decision_timer += delta_time;
                    
                    if (ai->decision_timer >= ai->decision_interval) {
                        // Simulate behavior changes
                        if (total_time > 1.0f && ai->behavior == "idle") {
                            ai->behavior = "patrol";
                        } else if (total_time > 2.0f && ai->behavior == "patrol") {
                            ai->behavior = "guard";
                        }
                        ai->decision_timer = 0.0f;
                    }
                }
            }
            
            // Log periodic updates
            if (frame % 60 == 0) {
                LOG_INFO("AI simulation time: {:.1f}s", total_time);
            }
        }
        
        LOG_INFO("Game AI scripting example completed");
    }
    
    // Example 8: Parallel Script Execution
    void example_08_parallel_script_execution() {
        LOG_INFO("=== Example 8: Parallel Script Execution ===");
        
        // Create multiple entities for parallel processing
        std::vector<scripting::ecs_interface::ScriptEntity*> entities;
        
        for (int i = 0; i < 100; ++i) {
            auto entity = ecs_interface_->create_entity();
            entity->add_component<Position>(
                static_cast<f32>(i % 10), 
                static_cast<f32>(i / 10), 
                0.0f
            );
            entity->add_component<Velocity>(
                (i % 2 == 0 ? 1.0f : -1.0f),
                (i % 3 == 0 ? 1.0f : -1.0f),
                0.0f
            );
            entities.push_back(entity);
        }
        
        LOG_INFO("Created {} entities for parallel processing", entities.size());
        
        // Parallel processing script
        std::string parallel_script = R"(
import math
import time

def parallel_physics_update(entities, delta_time):
    """Update physics for multiple entities in parallel."""
    
    def update_single_entity(entity):
        """Update a single entity's physics."""
        position = entity.get_component('Position')
        velocity = entity.get_component('Velocity')
        
        if position and velocity:
            # Simple physics integration
            position.x += velocity.dx * delta_time
            position.y += velocity.dy * delta_time
            
            # Apply some interesting physics
            # Gravity-like effect toward center
            center_x, center_y = 5.0, 5.0
            dx = center_x - position.x
            dy = center_y - position.y
            distance = math.sqrt(dx*dx + dy*dy)
            
            if distance > 0:
                # Apply attraction force
                force = 0.1 / (distance + 1.0)
                velocity.dx += (dx / distance) * force * delta_time
                velocity.dy += (dy / distance) * force * delta_time
            
            # Apply damping
            velocity.dx *= 0.99
            velocity.dy *= 0.99
    
    # Process entities (would be parallelized in real implementation)
    start_time = time.time()
    
    for entity in entities:
        update_single_entity(entity)
    
    end_time = time.time()
    processing_time = (end_time - start_time) * 1000
    
    print(f"Processed {len(entities)} entities in {processing_time:.2f} ms")
    return processing_time

# This would be called with actual entity data
print("Parallel script processing system ready")
)";
        
        python_engine_->execute_string(parallel_script);
        
        // Simulate parallel processing using job system
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Create a query for entities with position and velocity
        auto moving_query = ecs_interface_->create_query<Position, Velocity>();
        
        // Process entities in parallel using job system
        moving_query->for_each_parallel([](auto& script_entity, Position& pos, Velocity& vel) {
            // Simple physics update
            f32 delta_time = 0.016f;
            pos.x += vel.dx * delta_time;
            pos.y += vel.dy * delta_time;
            
            // Apply center attraction
            f32 center_x = 5.0f, center_y = 5.0f;
            f32 dx = center_x - pos.x;
            f32 dy = center_y - pos.y;
            f32 distance = std::sqrt(dx*dx + dy*dy);
            
            if (distance > 0.0f) {
                f32 force = 0.1f / (distance + 1.0f);
                vel.dx += (dx / distance) * force * delta_time;
                vel.dy += (dy / distance) * force * delta_time;
            }
            
            // Apply damping
            vel.dx *= 0.99f;
            vel.dy *= 0.99f;
            
        }, job_system_.get());
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration_ms = std::chrono::duration<f64, std::milli>(end_time - start_time).count();
        
        LOG_INFO("Parallel processing completed in {:.2f} ms", duration_ms);
        LOG_INFO("Job system stats: {} worker threads", job_system_->worker_count());
        
        LOG_INFO("Parallel script execution example completed");
    }
    
    // Example 9: Memory Optimization
    void example_09_memory_optimization() {
        LOG_INFO("=== Example 9: Memory Optimization ===");
        
        memory_profiler_->start_tracking();
        
        // Demonstrate memory optimization techniques
        std::string memory_optimization_script = R"(
import gc
import sys

def memory_optimization_demo():
    """Demonstrate memory optimization techniques for scripts."""
    
    print("=== Memory Optimization Techniques ===")
    
    # 1. Object pooling pattern
    class ObjectPool:
        def __init__(self, create_func, reset_func, initial_size=10):
            self.create_func = create_func
            self.reset_func = reset_func
            self.available = [create_func() for _ in range(initial_size)]
            self.in_use = []
        
        def acquire(self):
            if self.available:
                obj = self.available.pop()
                self.in_use.append(obj)
                return obj
            else:
                obj = self.create_func()
                self.in_use.append(obj)
                return obj
        
        def release(self, obj):
            if obj in self.in_use:
                self.in_use.remove(obj)
                self.reset_func(obj)
                self.available.append(obj)
    
    # 2. Memory-efficient data structures
    def create_position():
        return {'x': 0.0, 'y': 0.0, 'z': 0.0}
    
    def reset_position(pos):
        pos['x'] = pos['y'] = pos['z'] = 0.0
    
    position_pool = ObjectPool(create_position, reset_position, 50)
    
    # 3. Batch processing to reduce allocations
    def batch_process_entities(entities, batch_size=32):
        """Process entities in batches to improve memory locality."""
        for i in range(0, len(entities), batch_size):
            batch = entities[i:i + batch_size]
            # Process batch
            for entity in batch:
                # Simulate entity processing
                pass
    
    # 4. Generator-based iteration to reduce memory usage
    def entity_generator(count):
        """Generate entities on-demand instead of storing all in memory."""
        for i in range(count):
            yield {
                'id': i,
                'position': position_pool.acquire(),
                'data': f"entity_{i}"
            }
    
    # Demonstrate optimization techniques
    print("1. Object pooling demonstration")
    positions = [position_pool.acquire() for _ in range(20)]
    for pos in positions:
        position_pool.release(pos)
    print(f"   Pool stats: {len(position_pool.available)} available, {len(position_pool.in_use)} in use")
    
    print("2. Batch processing demonstration")
    test_entities = list(range(100))
    batch_process_entities(test_entities)
    print("   Batch processing completed")
    
    print("3. Generator-based processing")
    processed_count = 0
    for entity in entity_generator(1000):
        processed_count += 1
        if processed_count % 100 == 0:
            position_pool.release(entity['position'])
    print(f"   Processed {processed_count} entities using generators")
    
    # 4. Garbage collection optimization
    print("4. Memory management")
    initial_objects = len(gc.get_objects())
    
    # Force garbage collection
    collected = gc.collect()
    
    final_objects = len(gc.get_objects())
    print(f"   Garbage collected: {collected} objects")
    print(f"   Objects before/after: {initial_objects} -> {final_objects}")
    
    return {
        'techniques_demonstrated': 4,
        'objects_pooled': len(position_pool.available),
        'gc_collected': collected
    }

# Run memory optimization demo
result = memory_optimization_demo()
print(f"Memory optimization demo completed: {result}")
)";
        
        python_engine_->execute_string(memory_optimization_script);
        
        memory_profiler_->stop_tracking();
        
        // Analyze memory usage
        auto memory_stats = memory_profiler_->get_statistics();
        auto memory_leaks = memory_profiler_->get_memory_leaks();
        
        LOG_INFO("=== Memory Analysis Results ===");
        LOG_INFO("Total allocated: {} KB", memory_stats.total_allocated / 1024);
        LOG_INFO("Peak memory: {} KB", memory_stats.peak_memory / 1024);
        LOG_INFO("Current allocated: {} KB", memory_stats.current_allocated / 1024);
        LOG_INFO("Active allocations: {}", memory_stats.active_allocations);
        LOG_INFO("Average allocation size: {:.2f} bytes", memory_stats.average_allocation_size);
        
        if (!memory_leaks.empty()) {
            LOG_WARN("Detected {} potential memory leaks", memory_leaks.size());
            for (usize i = 0; i < std::min(size_t(5), memory_leaks.size()); ++i) {
                const auto& leak = memory_leaks[i];
                LOG_WARN("  Leak {}: {} bytes, lifetime: {:.2f} ms", 
                        i + 1, leak.size, leak.lifetime_ms());
            }
        }
        
        LOG_INFO("Memory optimization example completed");
    }
    
    // Example 10: Complete Game System
    void example_10_complete_game_system() {
        LOG_INFO("=== Example 10: Complete Game System ===");
        
        // This example demonstrates a complete mini-game implemented with scripts
        
        // Create game world
        struct GameWorld {
            scripting::ecs_interface::ScriptEntity* player;
            std::vector<scripting::ecs_interface::ScriptEntity*> enemies;
            std::vector<scripting::ecs_interface::ScriptEntity*> items;
            f32 game_time = 0.0f;
            bool game_running = true;
        };
        
        GameWorld world;
        
        // Create player
        world.player = ecs_interface_->create_entity();
        world.player->add_component<Position>(0.0f, 0.0f, 0.0f);
        world.player->add_component<Velocity>(0.0f, 0.0f, 0.0f);
        world.player->add_component<Health>(100);
        world.player->add_component<PlayerTag>();
        
        // Create enemies
        for (int i = 0; i < 10; ++i) {
            auto enemy = ecs_interface_->create_entity();
            f32 angle = (i * 2.0f * 3.14159f) / 10.0f;
            enemy->add_component<Position>(
                std::cos(angle) * 8.0f,
                std::sin(angle) * 8.0f,
                0.0f
            );
            enemy->add_component<Health>(30 + i * 5);
            enemy->add_component<AIController>();
            world.enemies.push_back(enemy);
        }
        
        LOG_INFO("Created game world with {} enemies", world.enemies.size());
        
        // Game logic script
        std::string game_script = R"(
import math
import random

class MiniGameSystem:
    def __init__(self):
        self.score = 0
        self.wave = 1
        self.enemies_defeated = 0
    
    def update_game(self, world, delta_time):
        """Main game update loop."""
        world.game_time += delta_time
        
        # Update player
        self.update_player(world.player, delta_time)
        
        # Update enemies
        for enemy in world.enemies:
            if enemy.is_valid():
                self.update_enemy(enemy, world.player, delta_time)
        
        # Check collisions
        self.check_collisions(world)
        
        # Update game state
        self.update_game_state(world)
        
        return world.game_running
    
    def update_player(self, player, delta_time):
        """Update player movement and behavior."""
        position = player.get_component('Position')
        velocity = player.get_component('Velocity')
        health = player.get_component('Health')
        
        if position and velocity and health:
            # Simple player AI (could be replaced with input)
            # Move in a small circle
            position.x += velocity.dx * delta_time
            position.y += velocity.dy * delta_time
            
            # Keep player in bounds
            max_distance = 10.0
            distance = math.sqrt(position.x**2 + position.y**2)
            if distance > max_distance:
                position.x = (position.x / distance) * max_distance
                position.y = (position.y / distance) * max_distance
    
    def update_enemy(self, enemy, player, delta_time):
        """Update enemy AI behavior."""
        enemy_pos = enemy.get_component('Position')
        enemy_health = enemy.get_component('Health')
        enemy_ai = enemy.get_component('AIController')
        player_pos = player.get_component('Position')
        
        if not (enemy_pos and enemy_health and enemy_ai and player_pos):
            return
        
        # Simple AI: move toward player
        dx = player_pos.x - enemy_pos.x
        dy = player_pos.y - enemy_pos.y
        distance = math.sqrt(dx**2 + dy**2)
        
        if distance > 0.1:
            speed = 2.0
            enemy_pos.x += (dx / distance) * speed * delta_time
            enemy_pos.y += (dy / distance) * speed * delta_time
    
    def check_collisions(self, world):
        """Check for collisions between game objects."""
        player_pos = world.player.get_component('Position')
        if not player_pos:
            return
        
        for enemy in world.enemies[:]:  # Copy list to avoid modification during iteration
            if not enemy.is_valid():
                continue
                
            enemy_pos = enemy.get_component('Position')
            if not enemy_pos:
                continue
            
            # Check collision distance
            dx = player_pos.x - enemy_pos.x
            dy = player_pos.y - enemy_pos.y
            distance = math.sqrt(dx**2 + dy**2)
            
            if distance < 1.0:  # Collision threshold
                # Player takes damage
                player_health = world.player.get_component('Health')
                if player_health:
                    player_health.current -= 10
                    print(f"Player hit! Health: {player_health.current}")
                    
                    if player_health.current <= 0:
                        world.game_running = False
                        print("Game Over!")
                
                # Enemy is destroyed
                self.enemies_defeated += 1
                self.score += 100
                enemy.destroy()
                world.enemies.remove(enemy)
                print(f"Enemy defeated! Score: {self.score}")
    
    def update_game_state(self, world):
        """Update overall game state."""
        if len(world.enemies) == 0:
            # Spawn new wave
            self.wave += 1
            print(f"Wave {self.wave} started!")
            # In a real game, this would spawn new enemies
        
        # Game timer
        if world.game_time > 30.0:  # 30 second demo
            world.game_running = False
            print(f"Demo completed! Final score: {self.score}")

# Initialize game system
game_system = MiniGameSystem()
print("Complete game system initialized")
)";
        
        python_engine_->execute_string(game_script);
        
        // Game simulation loop
        f32 delta_time = 0.016f;
        int frame_count = 0;
        
        LOG_INFO("Starting game simulation...");
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        while (world.game_running && frame_count < 1800) { // Max 30 seconds
            world.game_time += delta_time;
            
            // Simulate player movement (simple circular pattern)
            if (auto* player_pos = world.player->get_component<Position>()) {
                f32 angle = world.game_time * 0.5f;
                auto* player_vel = world.player->get_component<Velocity>();
                if (player_vel) {
                    player_vel->dx = std::cos(angle) * 2.0f;
                    player_vel->dy = std::sin(angle) * 2.0f;
                }
                
                player_pos->x += player_vel->dx * delta_time;
                player_pos->y += player_vel->dy * delta_time;
            }
            
            // Update enemies (move toward player)
            if (auto* player_pos = world.player->get_component<Position>()) {
                for (auto* enemy : world.enemies) {
                    if (!enemy->is_valid()) continue;
                    
                    auto* enemy_pos = enemy->get_component<Position>();
                    if (!enemy_pos) continue;
                    
                    f32 dx = player_pos->x - enemy_pos->x;
                    f32 dy = player_pos->y - enemy_pos->y;
                    f32 distance = std::sqrt(dx*dx + dy*dy);
                    
                    if (distance > 0.1f) {
                        f32 speed = 1.5f;
                        enemy_pos->x += (dx / distance) * speed * delta_time;
                        enemy_pos->y += (dy / distance) * speed * delta_time;
                    }
                    
                    // Simple collision check
                    if (distance < 1.0f) {
                        auto* player_health = world.player->get_component<Health>();
                        if (player_health) {
                            player_health->current -= 1;
                            if (player_health->current <= 0) {
                                world.game_running = false;
                                LOG_INFO("Game Over! Player defeated at {:.1f}s", world.game_time);
                            }
                        }
                    }
                }
            }
            
            // Log periodic updates
            if (frame_count % 300 == 0) { // Every 5 seconds
                auto* player_health = world.player->get_component<Health>();
                LOG_INFO("Game time: {:.1f}s, Player health: {}, Active enemies: {}", 
                        world.game_time, 
                        player_health ? player_health->current : 0,
                        world.enemies.size());
            }
            
            ++frame_count;
            
            // Exit condition
            if (world.game_time > 10.0f) { // 10 second demo
                world.game_running = false;
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto simulation_duration = std::chrono::duration<f64, std::milli>(end_time - start_time).count();
        
        LOG_INFO("Game simulation completed");
        LOG_INFO("Simulation time: {:.2f} ms ({} frames)", simulation_duration, frame_count);
        LOG_INFO("Average FPS: {:.1f}", frame_count * 1000.0 / simulation_duration);
        
        // Final statistics
        auto ecs_stats = ecs_interface_->get_statistics();
        LOG_INFO("Final ECS stats - Entities: {}, Component accesses: {}", 
                ecs_stats.current_entities, ecs_stats.component_accesses);
        
        LOG_INFO("Complete game system example completed");
    }
    
    void create_temporary_script(const std::string& filename, int version) {
        std::ofstream file(filename);
        if (!file) return;
        
        if (version == 1) {
            file << R"(
# Hot-reload example script - Version 1
print("Hot-reload script version 1.0")

def greet(name):
    return f"Hello, {name}! This is version 1."

def calculate_something():
    result = 10 + 20
    return result

message = greet("ECScope")
calculation = calculate_something()
print(f"Message: {message}")
print(f"Calculation result: {calculation}")
)";
        } else {
            file << R"(
# Hot-reload example script - Version 2
print("Hot-reload script version 2.0 - Now with improvements!")

def greet(name):
    return f"Greetings, {name}! This is the enhanced version 2."

def calculate_something():
    # Improved calculation with more complexity
    result = sum(i * i for i in range(10))
    return result

def new_feature():
    return "This is a new feature added in version 2!"

message = greet("ECScope User")
calculation = calculate_something()
feature = new_feature()

print(f"Enhanced message: {message}")
print(f"Advanced calculation: {calculation}")
print(f"New feature: {feature}")
)";
        }
    }
};

//=============================================================================
// Performance Benchmark Suite
//=============================================================================

class ScriptingBenchmarks {
private:
    std::unique_ptr<memory::AdvancedMemorySystem> memory_system_;
    std::unique_ptr<scripting::python::PythonEngine> python_engine_;
    std::unique_ptr<scripting::profiling::FunctionProfiler> profiler_;
    
public:
    ScriptingBenchmarks() {
        memory_system_ = std::make_unique<memory::AdvancedMemorySystem>(
            memory::AdvancedMemorySystem::Config{}
        );
        
        python_engine_ = std::make_unique<scripting::python::PythonEngine>(*memory_system_);
        python_engine_->initialize();
        
        profiler_ = std::make_unique<scripting::profiling::FunctionProfiler>(
            scripting::profiling::FunctionProfiler::ProfilingMode::Full
        );
    }
    
    void run_performance_benchmarks() {
        LOG_INFO("=== ECScope Scripting Performance Benchmarks ===");
        
        benchmark_script_execution_speed();
        benchmark_memory_allocation_performance();
        benchmark_ecs_integration_overhead();
        benchmark_cross_language_communication();
        benchmark_hot_reload_performance();
        
        LOG_INFO("=== Performance Benchmarks Completed ===");
    }

private:
    void benchmark_script_execution_speed() {
        LOG_INFO("--- Script Execution Speed Benchmark ---");
        
        std::string benchmark_script = R"(
import time

# Benchmark 1: Function calls
def simple_function(x):
    return x * x + 1

# Benchmark 2: Loop performance
def loop_benchmark(iterations):
    result = 0
    for i in range(iterations):
        result += simple_function(i)
    return result

# Benchmark 3: Data structure operations
def data_structure_benchmark():
    data = {}
    for i in range(1000):
        data[f"key_{i}"] = i * 2
    
    total = sum(data.values())
    return total

# Run benchmarks
start_time = time.perf_counter()

loop_result = loop_benchmark(10000)
data_result = data_structure_benchmark()

end_time = time.perf_counter()
execution_time = (end_time - start_time) * 1000

print(f"Script execution benchmark completed in {execution_time:.3f} ms")
print(f"Loop result: {loop_result}")
print(f"Data structure result: {data_result}")
)";
        
        profiler_->start_profiling();
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Execute multiple times for statistical accuracy
        for (int i = 0; i < 100; ++i) {
            python_engine_->execute_string(benchmark_script);
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto total_duration = std::chrono::duration<f64, std::milli>(end_time - start_time).count();
        
        profiler_->stop_profiling();
        
        LOG_INFO("Python execution benchmark: {:.3f} ms average per run", total_duration / 100.0);
        
        auto hotspots = profiler_->get_hotspots(5);
        for (const auto& hotspot : hotspots) {
            LOG_INFO("  Hotspot: {} - {:.3f} ms average", hotspot.function_name, hotspot.average_time_ms());
        }
    }
    
    void benchmark_memory_allocation_performance() {
        LOG_INFO("--- Memory Allocation Performance Benchmark ---");
        
        std::string memory_benchmark = R"(
import gc

def memory_allocation_benchmark():
    """Benchmark memory allocation patterns."""
    
    # Test 1: Many small allocations
    small_objects = []
    for i in range(10000):
        small_objects.append({'id': i, 'value': i * 2})
    
    # Test 2: Fewer large allocations  
    large_objects = []
    for i in range(100):
        large_objects.append([j for j in range(1000)])
    
    # Test 3: Allocation and deallocation cycles
    for cycle in range(100):
        temp_data = [i for i in range(100)]
        del temp_data
    
    # Force garbage collection
    collected = gc.collect()
    
    return {
        'small_objects': len(small_objects),
        'large_objects': len(large_objects),
        'gc_collected': collected
    }

result = memory_allocation_benchmark()
print(f"Memory benchmark result: {result}")
)";
        
        auto memory_stats_before = memory_system_->get_statistics();
        
        auto start_time = std::chrono::high_resolution_clock::now();
        python_engine_->execute_string(memory_benchmark);
        auto end_time = std::chrono::high_resolution_clock::now();
        
        auto memory_stats_after = memory_system_->get_statistics();
        auto duration = std::chrono::duration<f64, std::milli>(end_time - start_time).count();
        
        LOG_INFO("Memory allocation benchmark: {:.3f} ms", duration);
        LOG_INFO("Memory allocated: {} KB", 
                (memory_stats_after.total_allocated - memory_stats_before.total_allocated) / 1024);
        LOG_INFO("Peak memory increase: {} KB",
                (memory_stats_after.peak_usage - memory_stats_before.peak_usage) / 1024);
    }
    
    void benchmark_ecs_integration_overhead() {
        LOG_INFO("--- ECS Integration Overhead Benchmark ---");
        
        // This would benchmark the overhead of ECS operations through scripting
        // compared to native C++ operations
        
        LOG_INFO("ECS integration benchmarks would measure:");
        LOG_INFO("- Component access overhead through scripts");
        LOG_INFO("- Entity creation/destruction performance");
        LOG_INFO("- Query execution performance comparison");
        LOG_INFO("- Script vs native system execution times");
    }
    
    void benchmark_cross_language_communication() {
        LOG_INFO("--- Cross-Language Communication Benchmark ---");
        
        std::string communication_benchmark = R"(
import json
import time

def communication_benchmark():
    """Benchmark data serialization for cross-language communication."""
    
    # Create complex data structure
    data = {
        'entities': [
            {
                'id': i,
                'components': {
                    'position': {'x': i * 1.5, 'y': i * 2.0, 'z': 0.0},
                    'health': {'current': 100 - i, 'maximum': 100},
                    'metadata': {'name': f'entity_{i}', 'type': 'test'}
                }
            }
            for i in range(1000)
        ],
        'world_state': {
            'time': time.time(),
            'settings': {'difficulty': 'medium', 'player_count': 4}
        }
    }
    
    # Benchmark serialization
    start_time = time.perf_counter()
    json_data = json.dumps(data)
    serialization_time = time.perf_counter() - start_time
    
    # Benchmark deserialization
    start_time = time.perf_counter()
    parsed_data = json.loads(json_data)
    deserialization_time = time.perf_counter() - start_time
    
    return {
        'data_size_kb': len(json_data) / 1024,
        'serialization_ms': serialization_time * 1000,
        'deserialization_ms': deserialization_time * 1000,
        'entities_processed': len(parsed_data['entities'])
    }

result = communication_benchmark()
print(f"Communication benchmark: {result}")
)";
        
        auto start_time = std::chrono::high_resolution_clock::now();
        python_engine_->execute_string(communication_benchmark);
        auto end_time = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration<f64, std::milli>(end_time - start_time).count();
        LOG_INFO("Cross-language communication benchmark: {:.3f} ms", duration);
    }
    
    void benchmark_hot_reload_performance() {
        LOG_INFO("--- Hot-Reload Performance Benchmark ---");
        
        // Create temporary script for hot-reload testing
        std::string temp_script = "temp_hotreload_benchmark.py";
        
        std::ofstream file(temp_script);
        file << R"(
# Hot-reload benchmark script
import time

def benchmark_function():
    result = sum(i*i for i in range(1000))
    return result

start_time = time.perf_counter()
result = benchmark_function()
end_time = time.perf_counter()

execution_time = (end_time - start_time) * 1000
print(f"Hot-reload benchmark: {execution_time:.3f} ms, result: {result}")
)";
        file.close();
        
        // Benchmark initial load
        auto start_time = std::chrono::high_resolution_clock::now();
        python_engine_->execute_file(temp_script);
        auto initial_load_time = std::chrono::high_resolution_clock::now();
        
        // Benchmark reload (simulate file change)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        python_engine_->execute_file(temp_script);
        auto reload_time = std::chrono::high_resolution_clock::now();
        
        auto initial_duration = std::chrono::duration<f64, std::milli>(initial_load_time - start_time).count();
        auto reload_duration = std::chrono::duration<f64, std::milli>(reload_time - initial_load_time).count();
        
        LOG_INFO("Hot-reload performance - Initial: {:.3f} ms, Reload: {:.3f} ms", 
                initial_duration, reload_duration);
        
        // Cleanup
        std::remove(temp_script.c_str());
    }
};

//=============================================================================
// Main Example Runner
//=============================================================================

int main(int argc, char* argv[]) {
    LOG_INFO("ECScope Scripting Integration Examples and Benchmarks");
    LOG_INFO("====================================================");
    
    try {
        // Run educational examples
        LOG_INFO("Starting educational examples...");
        ScriptingExamples examples;
        examples.run_all_examples();
        
        LOG_INFO("\n");
        
        // Run performance benchmarks
        LOG_INFO("Starting performance benchmarks...");
        ScriptingBenchmarks benchmarks;
        benchmarks.run_performance_benchmarks();
        
    } catch (const std::exception& e) {
        LOG_ERROR("Exception in scripting examples: {}", e.what());
        return 1;
    }
    
    LOG_INFO("\nECScope Scripting Integration demonstration completed successfully!");
    return 0;
}