# ECScope Comprehensive Tutorial

**A complete hands-on guide to mastering ECScope Educational ECS Engine**

## Table of Contents

1. [Tutorial Overview](#tutorial-overview)
2. [Chapter 1: ECS Fundamentals](#chapter-1-ecs-fundamentals)
3. [Chapter 2: Memory Management](#chapter-2-memory-management)
4. [Chapter 3: Physics Integration](#chapter-3-physics-integration)
5. [Chapter 4: Rendering and Graphics](#chapter-4-rendering-and-graphics)
6. [Chapter 5: Performance Optimization](#chapter-5-performance-optimization)
7. [Chapter 6: WebAssembly Deployment](#chapter-6-webassembly-deployment)
8. [Chapter 7: Building Real Applications](#chapter-7-building-real-applications)
9. [Final Project](#final-project)

## Tutorial Overview

This comprehensive tutorial guides you through building increasingly complex applications with ECScope, from basic ECS concepts to advanced optimization techniques. Each chapter builds upon the previous one, culminating in a complete game engine application.

### Learning Objectives
- Understand Entity-Component-System architecture deeply
- Master custom memory management techniques
- Implement real-time physics simulation
- Build efficient 2D rendering systems
- Optimize for high performance
- Deploy to web browsers using WebAssembly
- Create educational and practical applications

### Prerequisites
- Intermediate C++ knowledge (templates, RAII, smart pointers)
- Basic understanding of game development concepts
- Familiarity with command-line tools and build systems
- Mathematical concepts (linear algebra basics)

### Setup Requirements
```bash
# Clone ECScope
git clone https://github.com/GonxKZ/entities-cpp2.git
cd entities-cpp2

# Build with all features
mkdir build && cd build
cmake .. -DECSCOPE_ENABLE_GRAPHICS=ON -DECSCOPE_BUILD_EXAMPLES=ON
cmake --build . --parallel

# Verify installation
./ecscope_console
```

## Chapter 1: ECS Fundamentals

### 1.1: Understanding the ECS Pattern

**Traditional OOP vs ECS:**

```cpp
// Traditional OOP - Inheritance hierarchy (avoid this pattern)
class GameObject {
public:
    virtual void update() = 0;
    virtual void render() = 0;  // What if object doesn't render?
protected:
    float x, y;  // All objects have position - but what about UI?
};

class Enemy : public GameObject, public Collidable, public Animated {
    // Diamond inheritance problem
    // Tight coupling
    // Difficult to compose behaviors
};
```

```cpp
// ECS Pattern - Composition over inheritance
#include <ecscope/ecscope.hpp>
using namespace ecscope;

// Components are pure data
struct Position { 
    f32 x, y; 
    Position(f32 x = 0.0f, f32 y = 0.0f) : x(x), y(y) {}
};

struct Velocity { 
    f32 dx, dy; 
    Velocity(f32 dx = 0.0f, f32 dy = 0.0f) : dx(dx), dy(dy) {}
};

struct Health { 
    i32 current, maximum; 
    Health(i32 current = 100, i32 maximum = 100) : current(current), maximum(maximum) {}
    
    bool is_alive() const { return current > 0; }  // Pure functions OK
};

// Systems contain behavior
void movement_system(ecs::Registry& registry, f32 dt) {
    registry.for_each<Position, Velocity>([dt](Position& pos, const Velocity& vel) {
        pos.x += vel.dx * dt;
        pos.y += vel.dy * dt;
    });
}

void health_system(ecs::Registry& registry) {
    // Process damage, healing, etc.
    registry.for_each<Health>([](Health& health) {
        // Health logic here
    });
}
```

### 1.2: First ECS Application

Create your first ECS application:

```cpp
// File: tutorial_01_basic_ecs.cpp
#include <ecscope/ecscope.hpp>
#include <iostream>
#include <chrono>

using namespace ecscope;

// Define our game components
struct Position { 
    f32 x, y; 
    Position(f32 x = 0.0f, f32 y = 0.0f) : x(x), y(y) {}
};

struct Velocity { 
    f32 dx, dy; 
    Velocity(f32 dx = 0.0f, f32 dy = 0.0f) : dx(dx), dy(dy) {}
};

struct Health { 
    i32 current, maximum;
    Health(i32 current = 100, i32 maximum = 100) : current(current), maximum(maximum) {}
};

struct Name {
    std::string value;
    Name(const std::string& name = "") : value(name) {}
};

// Game systems
void movement_system(ecs::Registry& registry, f32 delta_time) {
    LOG_DEBUG("Running movement system");
    
    registry.for_each<Position, Velocity>([delta_time](Position& pos, const Velocity& vel) {
        pos.x += vel.dx * delta_time;
        pos.y += vel.dy * delta_time;
        
        // Simple boundary wrapping
        if (pos.x > 1000.0f) pos.x = 0.0f;
        if (pos.x < 0.0f) pos.x = 1000.0f;
        if (pos.y > 1000.0f) pos.y = 0.0f;
        if (pos.y < 0.0f) pos.y = 1000.0f;
    });
}

void health_system(ecs::Registry& registry) {
    // Simulate health regeneration
    registry.for_each<Health>([](Health& health) {
        if (health.current < health.maximum && health.current > 0) {
            health.current += 1;  // Regenerate 1 HP per frame
        }
    });
}

void cleanup_system(ecs::Registry& registry) {
    // Find and remove dead entities
    std::vector<ecs::Entity> dead_entities;
    
    registry.for_each<Health>([&dead_entities](ecs::Entity entity, const Health& health) {
        if (health.current <= 0) {
            dead_entities.push_back(entity);
        }
    });
    
    for (auto entity : dead_entities) {
        LOG_INFO("Entity {} died, removing from registry", entity.to_string());
        registry.destroy_entity(entity);
    }
}

void debug_system(ecs::Registry& registry) {
    // Print entity information every 60 frames
    static u32 frame_count = 0;
    if (++frame_count % 60 == 0) {
        LOG_INFO("=== Frame {} Debug Info ===", frame_count);
        
        registry.for_each<Position, Name>([](const Position& pos, const Name& name) {
            LOG_INFO("Entity '{}' at ({:.2f}, {:.2f})", name.value, pos.x, pos.y);
        });
        
        auto stats = registry.get_memory_statistics();
        LOG_INFO("Registry stats: {} entities, {} archetypes", 
                stats.active_entities, stats.total_archetypes);
    }
}

int main() {
    LOG_INFO("Starting ECScope Tutorial 01 - Basic ECS");
    
    // Initialize ECScope
    if (!ecscope::initialize()) {
        LOG_ERROR("Failed to initialize ECScope");
        return 1;
    }
    
    // Create educational registry
    auto registry = ecs::create_educational_registry("Tutorial01");
    
    // Create game entities
    LOG_INFO("Creating game entities...");
    
    // Create a player entity
    auto player = registry->create_entity<Position, Velocity, Health, Name>(
        Position{500.0f, 500.0f},
        Velocity{10.0f, 5.0f},
        Health{100, 100},
        Name{"Player"}
    );
    
    // Create enemy entities
    for (int i = 0; i < 5; ++i) {
        registry->create_entity<Position, Velocity, Health, Name>(
            Position{static_cast<f32>(i * 100), static_cast<f32>(i * 50)},
            Velocity{static_cast<f32>(-i * 2), static_cast<f32>(i * 3)},
            Health{50, 50},
            Name{"Enemy_" + std::to_string(i)}
        );
    }
    
    // Create static objects (no velocity)
    for (int i = 0; i < 3; ++i) {
        registry->create_entity<Position, Health, Name>(
            Position{static_cast<f32>(200 + i * 200), 800.0f},
            Health{200, 200},
            Name{"Tower_" + std::to_string(i)}
        );
    }
    
    LOG_INFO("Created {} entities", registry->get_entity_count());
    
    // Game loop simulation
    const f32 target_fps = 60.0f;
    const f32 delta_time = 1.0f / target_fps;
    const u32 total_frames = 300;  // 5 seconds at 60 FPS
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (u32 frame = 0; frame < total_frames; ++frame) {
        // Simulate frame timing
        auto frame_start = std::chrono::high_resolution_clock::now();
        
        // Run all systems in order
        movement_system(*registry, delta_time);
        health_system(*registry);
        cleanup_system(*registry);
        debug_system(*registry);
        
        // Simulate some damage occasionally
        if (frame % 120 == 0 && frame > 0) {  // Every 2 seconds
            registry->for_each<Health, Name>([](Health& health, const Name& name) {
                if (name.value.find("Enemy") != std::string::npos) {
                    health.current -= 25;  // Damage enemies
                    LOG_INFO("Damaged {}: health now {}", name.value, health.current);
                }
            });
        }
        
        auto frame_end = std::chrono::high_resolution_clock::now();
        auto frame_duration = std::chrono::duration<f32>(frame_end - frame_start).count();
        
        if (frame_duration > delta_time) {
            LOG_WARN("Frame {} took {:.4f}s (target: {:.4f}s)", frame, frame_duration, delta_time);
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_duration = std::chrono::duration<f32>(end_time - start_time).count();
    
    LOG_INFO("Simulation completed in {:.3f} seconds", total_duration);
    LOG_INFO("Average FPS: {:.1f}", total_frames / total_duration);
    
    // Final statistics
    auto final_stats = registry->get_memory_statistics();
    LOG_INFO("Final registry state:");
    LOG_INFO("  Active entities: {}", final_stats.active_entities);
    LOG_INFO("  Total archetypes: {}", final_stats.total_archetypes);
    LOG_INFO("  Memory efficiency: {:.2f}%", final_stats.memory_efficiency * 100.0);
    
    // Generate detailed memory report
    std::string memory_report = registry->generate_memory_report();
    LOG_INFO("Memory Report:\n{}", memory_report);
    
    // Cleanup
    ecscope::shutdown();
    
    LOG_INFO("Tutorial 01 completed successfully!");
    return 0;
}
```

**Build and run:**
```bash
# Compile
g++ -std=c++20 -I../include tutorial_01_basic_ecs.cpp -lecscope_core -o tutorial_01

# Run
./tutorial_01
```

### 1.3: Understanding Archetypes

ECScope organizes entities by **archetype** (unique component combinations):

```cpp
// File: tutorial_01b_archetypes.cpp
#include <ecscope/ecscope.hpp>

using namespace ecscope;

struct Position { f32 x, y; };
struct Velocity { f32 dx, dy; };
struct Health { i32 current, max; };
struct Sprite { f32 width, height; };

void demonstrate_archetypes() {
    auto registry = ecs::create_educational_registry("ArchetypeDemo");
    
    LOG_INFO("=== Archetype Demonstration ===");
    
    // Archetype 1: Position only
    auto static_entity = registry->create_entity<Position>(Position{0, 0});
    LOG_INFO("Created entity with Position - Archetype count: {}", 
             registry->get_memory_statistics().total_archetypes);
    
    // Archetype 2: Position + Velocity  
    auto moving_entity = registry->create_entity<Position, Velocity>(
        Position{100, 100}, Velocity{1, 1});
    LOG_INFO("Created entity with Position+Velocity - Archetype count: {}", 
             registry->get_memory_statistics().total_archetypes);
    
    // Same archetype as previous (no new archetype created)
    auto another_moving = registry->create_entity<Position, Velocity>(
        Position{200, 200}, Velocity{-1, 2});
    LOG_INFO("Created another Position+Velocity entity - Archetype count: {}", 
             registry->get_memory_statistics().total_archetypes);
    
    // Archetype 3: Position + Velocity + Health
    auto living_moving = registry->create_entity<Position, Velocity, Health>(
        Position{300, 300}, Velocity{0.5, -0.5}, Health{100, 100});
    LOG_INFO("Created entity with Position+Velocity+Health - Archetype count: {}", 
             registry->get_memory_statistics().total_archetypes);
    
    // Archetype 4: All components
    auto complex_entity = registry->create_entity<Position, Velocity, Health, Sprite>(
        Position{400, 400}, Velocity{2, -1}, Health{150, 150}, Sprite{32, 32});
    LOG_INFO("Created entity with all components - Archetype count: {}", 
             registry->get_memory_statistics().total_archetypes);
    
    // Demonstrate archetype migration
    LOG_INFO("\\n=== Archetype Migration ===");
    
    // Add Health to the static entity (migrates to new archetype)
    bool success = registry->add_component<Health>(static_entity, Health{50, 50});
    if (success) {
        LOG_INFO("Added Health to static entity - Archetype count: {}", 
                 registry->get_memory_statistics().total_archetypes);
    }
    
    // Query different archetypes
    LOG_INFO("\\n=== Archetype Queries ===");
    
    auto position_only = registry->get_entities_with<Position>();
    LOG_INFO("Entities with Position: {}", position_only.size());
    
    auto moving = registry->get_entities_with<Position, Velocity>();
    LOG_INFO("Entities with Position+Velocity: {}", moving.size());
    
    auto living = registry->get_entities_with<Health>();
    LOG_INFO("Entities with Health: {}", living.size());
    
    auto complex = registry->get_entities_with<Position, Velocity, Health, Sprite>();
    LOG_INFO("Entities with all four components: {}", complex.size());
}

int main() {
    ecscope::initialize();
    demonstrate_archetypes();
    ecscope::shutdown();
    return 0;
}
```

### 1.4: Chapter 1 Exercises

**Exercise 1.1: Component Design**
Create components for a simple RPG game:
```cpp
// Define these components:
struct RPGPosition { /* your implementation */ };
struct RPGStats { /* strength, dexterity, intelligence */ };
struct Inventory { /* item storage */ };
struct Equipment { /* equipped items */ };

// Create entities: warrior, mage, archer, merchant
// Implement appropriate systems
```

**Exercise 1.2: System Dependencies**
```cpp
// Design systems that depend on each other:
// 1. Input system (updates velocity based on input)
// 2. Movement system (updates position based on velocity)  
// 3. Collision system (detects and resolves collisions)
// 4. Render system (draws entities)
// 
// Ensure proper execution order
```

## Chapter 2: Memory Management

### 2.1: Understanding Custom Allocators

ECScope provides several custom allocators optimized for different use cases:

```cpp
// File: tutorial_02_memory.cpp
#include <ecscope/ecscope.hpp>
#include <chrono>

using namespace ecscope;

void demonstrate_allocator_types() {
    LOG_INFO("=== Allocator Comparison Demo ===");
    
    // 1. Educational Registry - Balanced for learning
    auto educational = ecs::create_educational_registry("Educational");
    LOG_INFO("Educational registry created with balanced allocators");
    
    // 2. Performance Registry - Optimized for speed
    auto performance = ecs::create_performance_registry("Performance");
    LOG_INFO("Performance registry created with speed-optimized allocators");
    
    // 3. Conservative Registry - Uses standard allocators
    auto conservative = ecs::create_conservative_registry("Conservative");
    LOG_INFO("Conservative registry created with standard allocators");
    
    const usize entity_count = 10000;
    
    // Benchmark entity creation
    LOG_INFO("\\nBenchmarking entity creation ({} entities)...", entity_count);
    
    // Educational allocators
    auto start = std::chrono::high_resolution_clock::now();
    for (usize i = 0; i < entity_count; ++i) {
        educational->create_entity<Position, Velocity>(
            Position{static_cast<f32>(i), static_cast<f32>(i)},
            Velocity{1.0f, -1.0f}
        );
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto educational_time = std::chrono::duration<f64, std::milli>(end - start).count();
    
    // Performance allocators
    start = std::chrono::high_resolution_clock::now();
    for (usize i = 0; i < entity_count; ++i) {
        performance->create_entity<Position, Velocity>(
            Position{static_cast<f32>(i), static_cast<f32>(i)},
            Velocity{1.0f, -1.0f}
        );
    }
    end = std::chrono::high_resolution_clock::now();
    auto performance_time = std::chrono::duration<f64, std::milli>(end - start).count();
    
    // Conservative allocators
    start = std::chrono::high_resolution_clock::now();
    for (usize i = 0; i < entity_count; ++i) {
        conservative->create_entity<Position, Velocity>(
            Position{static_cast<f32>(i), static_cast<f32>(i)},
            Velocity{1.0f, -1.0f}
        );
    }
    end = std::chrono::high_resolution_clock::now();
    auto conservative_time = std::chrono::duration<f64, std::milli>(end - start).count();
    
    // Results
    LOG_INFO("\\nBenchmark Results:");
    LOG_INFO("Educational:  {:.2f}ms ({:.0f} entities/ms)", educational_time, entity_count / educational_time);
    LOG_INFO("Performance:  {:.2f}ms ({:.0f} entities/ms)", performance_time, entity_count / performance_time);
    LOG_INFO("Conservative: {:.2f}ms ({:.0f} entities/ms)", conservative_time, entity_count / conservative_time);
    
    f64 educational_speedup = conservative_time / educational_time;
    f64 performance_speedup = conservative_time / performance_time;
    
    LOG_INFO("Educational is {:.2f}x faster than conservative", educational_speedup);
    LOG_INFO("Performance is {:.2f}x faster than conservative", performance_speedup);
    
    // Memory statistics
    LOG_INFO("\\nMemory Statistics:");
    
    auto edu_stats = educational->get_memory_statistics();
    auto perf_stats = performance->get_memory_statistics();
    auto cons_stats = conservative->get_memory_statistics();
    
    LOG_INFO("Educational - Efficiency: {:.1f}%, Arena: {:.1f}%", 
             edu_stats.memory_efficiency * 100, edu_stats.arena_utilization() * 100);
    LOG_INFO("Performance - Efficiency: {:.1f}%, Arena: {:.1f}%", 
             perf_stats.memory_efficiency * 100, perf_stats.arena_utilization() * 100);
    LOG_INFO("Conservative - Efficiency: {:.1f}%", 
             cons_stats.memory_efficiency * 100);
}

void demonstrate_memory_pressure() {
    LOG_INFO("\\n=== Memory Pressure Demonstration ===");
    
    // Create registry with limited memory for demonstration
    ecs::AllocatorConfig config = ecs::AllocatorConfig::create_educational_focused();
    config.archetype_arena_size = 1 * MB;  // Small arena
    config.entity_pool_capacity = 2000;    // Small pool
    
    ecs::Registry limited_registry(config, "LimitedMemory");
    
    LOG_INFO("Created registry with limited memory:");
    LOG_INFO("  Arena size: {} MB", config.archetype_arena_size / MB);
    LOG_INFO("  Entity pool: {} entities", config.entity_pool_capacity);
    
    // Create entities until we approach limits
    usize entities_created = 0;
    bool memory_warning_shown = false;
    
    for (usize i = 0; i < 5000; ++i) {  // Try to create more than pool allows
        try {
            auto entity = limited_registry.create_entity<Position, Velocity, Health>(
                Position{static_cast<f32>(i % 1000), static_cast<f32>(i % 500)},
                Velocity{static_cast<f32>((i % 3) - 1), static_cast<f32>((i % 3) - 1)},
                Health{100, 100}
            );
            entities_created++;
            
            // Check memory pressure every 100 entities
            if (i % 100 == 0) {
                auto stats = limited_registry.get_memory_statistics();
                
                if ((stats.arena_utilization() > 0.8 || stats.pool_utilization() > 0.8) && !memory_warning_shown) {
                    LOG_WARN("High memory usage detected at {} entities!", entities_created);
                    LOG_WARN("  Arena: {:.1f}%", stats.arena_utilization() * 100);
                    LOG_WARN("  Pool: {:.1f}%", stats.pool_utilization() * 100);
                    memory_warning_shown = true;
                }
            }
            
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to create entity {}: {}", i, e.what());
            break;
        }
    }
    
    LOG_INFO("Successfully created {} entities before hitting limits", entities_created);
    
    auto final_stats = limited_registry.get_memory_statistics();
    LOG_INFO("Final memory usage:");
    LOG_INFO("  Arena: {:.1f}%", final_stats.arena_utilization() * 100);
    LOG_INFO("  Pool: {:.1f}%", final_stats.pool_utilization() * 100);
    LOG_INFO("  Efficiency: {:.1f}%", final_stats.memory_efficiency * 100);
    
    // Demonstrate memory cleanup
    LOG_INFO("\\nDemonstrating memory cleanup...");
    limited_registry.compact_memory();
    
    auto compacted_stats = limited_registry.get_memory_statistics();
    LOG_INFO("After compaction:");
    LOG_INFO("  Arena: {:.1f}%", compacted_stats.arena_utilization() * 100);
    LOG_INFO("  Pool: {:.1f}%", compacted_stats.pool_utilization() * 100);
}

int main() {
    ecscope::initialize();
    
    demonstrate_allocator_types();
    demonstrate_memory_pressure();
    
    ecscope::shutdown();
    return 0;
}
```

### 2.2: Memory Tracking and Analysis

```cpp
// File: tutorial_02b_memory_tracking.cpp
#include <ecscope/ecscope.hpp>

using namespace ecscope;

void demonstrate_memory_tracking() {
    LOG_INFO("=== Memory Tracking Demonstration ===");
    
    auto registry = ecs::create_educational_registry("MemoryTracking");
    
    // Enable detailed memory tracking
    registry->enable_memory_tracking(true);
    
    // Create different entity patterns to see memory layout
    LOG_INFO("Creating different entity patterns...");
    
    // Pattern 1: Simple entities
    for (usize i = 0; i < 1000; ++i) {
        registry->create_entity<Position>(Position{static_cast<f32>(i), 0.0f});
    }
    
    auto stats_1 = registry->get_memory_statistics();
    LOG_INFO("After 1000 Position entities:");
    LOG_INFO("  Archetypes: {}", stats_1.total_archetypes);
    LOG_INFO("  Arena usage: {:.1f}%", stats_1.arena_utilization() * 100);
    
    // Pattern 2: Complex entities
    for (usize i = 0; i < 500; ++i) {
        registry->create_entity<Position, Velocity, Health>(
            Position{static_cast<f32>(i), 100.0f},
            Velocity{1.0f, -1.0f},
            Health{100, 100}
        );
    }
    
    auto stats_2 = registry->get_memory_statistics();
    LOG_INFO("After adding 500 Position+Velocity+Health entities:");
    LOG_INFO("  Archetypes: {}", stats_2.total_archetypes);
    LOG_INFO("  Arena usage: {:.1f}%", stats_2.arena_utilization() * 100);
    
    // Pattern 3: Sparse component additions (causes archetype migrations)
    LOG_INFO("\\nDemonstrating archetype migrations...");
    
    auto simple_entities = registry->get_entities_with<Position>();
    usize migration_count = 0;
    
    for (usize i = 0; i < 100 && i < simple_entities.size(); ++i) {
        auto entity = simple_entities[i];
        
        // Add Velocity component (triggers migration)
        if (registry->add_component<Velocity>(entity, Velocity{static_cast<f32>(i), 0.0f})) {
            migration_count++;
        }
    }
    
    auto stats_3 = registry->get_memory_statistics();
    LOG_INFO("After {} archetype migrations:", migration_count);
    LOG_INFO("  Archetypes: {}", stats_3.total_archetypes);
    LOG_INFO("  Arena usage: {:.1f}%", stats_3.arena_utilization() * 100);
    LOG_INFO("  Cache hit ratio: {:.1f}%", stats_3.cache_hit_ratio * 100);
    
    // Generate detailed memory report
    std::string detailed_report = registry->generate_memory_report();
    LOG_INFO("\\nDetailed Memory Report:\\n{}", detailed_report);
    
    // Benchmark different access patterns
    LOG_INFO("\\n=== Access Pattern Benchmarks ===");
    
    const usize benchmark_iterations = 1000;
    
    // Sequential access (cache-friendly)
    auto start = std::chrono::high_resolution_clock::now();
    for (usize iter = 0; iter < benchmark_iterations; ++iter) {
        registry->for_each<Position, Velocity>([](Position& pos, const Velocity& vel) {
            pos.x += vel.dx * 0.016f;
            pos.y += vel.dy * 0.016f;
        });
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto sequential_time = std::chrono::duration<f64, std::micro>(end - start).count();
    
    // Random access (cache-unfriendly)
    auto moving_entities = registry->get_entities_with<Position, Velocity>();
    start = std::chrono::high_resolution_clock::now();
    for (usize iter = 0; iter < benchmark_iterations; ++iter) {
        for (auto entity : moving_entities) {
            auto* pos = registry->get_component<Position>(entity);
            auto* vel = registry->get_component<Velocity>(entity);
            if (pos && vel) {
                pos->x += vel->dx * 0.016f;
                pos->y += vel->dy * 0.016f;
            }
        }
    }
    end = std::chrono::high_resolution_clock::now();
    auto random_time = std::chrono::duration<f64, std::micro>(end - start).count();
    
    LOG_INFO("Access pattern benchmark ({} iterations):", benchmark_iterations);
    LOG_INFO("  Sequential (for_each): {:.2f}μs", sequential_time);
    LOG_INFO("  Random (get_component): {:.2f}μs", random_time);
    LOG_INFO("  Sequential is {:.2f}x faster", random_time / sequential_time);
    
    auto final_stats = registry->get_memory_statistics();
    LOG_INFO("\\nFinal cache hit ratio: {:.1f}%", final_stats.cache_hit_ratio * 100);
}

int main() {
    ecscope::initialize();
    demonstrate_memory_tracking();
    ecscope::shutdown();
    return 0;
}
```

### 2.3: Chapter 2 Exercises

**Exercise 2.1: Custom Allocator Configuration**
```cpp
// Create a registry optimized for your specific use case:
// 1. High entity count (100k+) with simple components
// 2. Complex entities (10+ components each) but low count (1k)
// 3. Frequently changing component combinations (dynamic gameplay)

ecs::AllocatorConfig create_custom_config() {
    // Your implementation here
}
```

**Exercise 2.2: Memory Budget System**
```cpp
// Implement a memory budget system that:
// 1. Monitors memory usage in real-time
// 2. Warns when approaching limits
// 3. Automatically cleans up old/unused entities
// 4. Reports memory efficiency metrics
```

## Chapter 3: Physics Integration

### 3.1: Basic Physics Simulation

```cpp
// File: tutorial_03_physics.cpp
#include <ecscope/ecscope.hpp>
#include <ecscope/physics.hpp>

using namespace ecscope;
using namespace ecscope::physics;

// Physics components for ECS integration
struct PhysicsBody {
    u32 body_id;        // ID in physics world
    f32 mass;
    
    PhysicsBody(u32 id = 0, f32 m = 1.0f) : body_id(id), mass(m) {}
};

struct Transform2D {
    f32 x, y;
    f32 rotation;
    
    Transform2D(f32 x = 0.0f, f32 y = 0.0f, f32 rot = 0.0f) 
        : x(x), y(y), rotation(rot) {}
};

struct RenderInfo {
    f32 width, height;
    f32 r, g, b, a;
    
    RenderInfo(f32 w = 32.0f, f32 h = 32.0f, f32 r = 1.0f, f32 g = 1.0f, f32 b = 1.0f, f32 a = 1.0f)
        : width(w), height(h), r(r), g(g), b(b), a(a) {}
};

class PhysicsDemo {
    std::unique_ptr<ecs::Registry> registry;
    std::unique_ptr<World2D> physics_world;
    
public:
    PhysicsDemo() {
        registry = ecs::create_educational_registry("PhysicsDemo");
        physics_world = std::make_unique<World2D>();
        
        // Configure physics world
        physics_world->set_gravity(0.0f, -300.0f);  // Downward gravity
        physics_world->set_damping(0.99f, 0.95f);   // Linear and angular damping
    }
    
    void create_scene() {
        LOG_INFO("Creating physics scene...");
        
        // Create ground (static body)
        create_ground();
        
        // Create falling boxes
        create_falling_boxes();
        
        // Create bouncing balls
        create_bouncing_balls();
        
        LOG_INFO("Scene created with {} entities", registry->get_entity_count());
    }
    
private:
    void create_ground() {
        // Ground body in physics world
        World2D::BodyDef ground_def;
        ground_def.x = 400.0f;
        ground_def.y = 550.0f;
        ground_def.rotation = 0.0f;
        ground_def.is_static = true;
        
        u32 ground_body_id = physics_world->create_body(ground_def);
        
        // Attach box shape to ground
        World2D::BoxShape ground_shape;
        ground_shape.width = 800.0f;
        ground_shape.height = 50.0f;
        ground_shape.density = 1.0f;
        ground_shape.friction = 0.7f;
        ground_shape.restitution = 0.3f;
        
        physics_world->attach_box(ground_body_id, ground_shape);
        
        // Create ECS entity for ground
        auto ground_entity = registry->create_entity<Transform2D, PhysicsBody, RenderInfo>(
            Transform2D{400.0f, 550.0f, 0.0f},
            PhysicsBody{ground_body_id, 0.0f},  // Static bodies have zero mass
            RenderInfo{800.0f, 50.0f, 0.5f, 0.3f, 0.1f, 1.0f}  // Brown ground
        );
        
        LOG_INFO("Created ground entity: {}", ground_entity.to_string());
    }
    
    void create_falling_boxes() {
        for (int i = 0; i < 5; ++i) {
            // Physics body
            World2D::BodyDef body_def;
            body_def.x = 100.0f + i * 80.0f;
            body_def.y = 100.0f + i * 50.0f;
            body_def.rotation = static_cast<f32>(i) * 0.3f;
            body_def.is_static = false;
            body_def.linear_damping = 0.1f;
            body_def.angular_damping = 0.1f;
            
            u32 body_id = physics_world->create_body(body_def);
            
            // Box shape
            World2D::BoxShape box_shape;
            box_shape.width = 30.0f;
            box_shape.height = 30.0f;
            box_shape.density = 1.0f;
            box_shape.friction = 0.5f;
            box_shape.restitution = 0.4f;
            
            physics_world->attach_box(body_id, box_shape);
            
            // ECS entity
            auto box_entity = registry->create_entity<Transform2D, PhysicsBody, RenderInfo>(
                Transform2D{body_def.x, body_def.y, body_def.rotation},
                PhysicsBody{body_id, 1.0f},
                RenderInfo{30.0f, 30.0f, 0.8f, 0.2f, 0.2f, 1.0f}  // Red boxes
            );
            
            LOG_DEBUG("Created box entity: {}", box_entity.to_string());
        }
    }
    
    void create_bouncing_balls() {
        for (int i = 0; i < 8; ++i) {
            // Physics body
            World2D::BodyDef body_def;
            body_def.x = 500.0f + (i % 4) * 60.0f;
            body_def.y = 50.0f + (i / 4) * 60.0f;
            body_def.rotation = 0.0f;
            body_def.is_static = false;
            body_def.linear_damping = 0.05f;
            body_def.angular_damping = 0.05f;
            
            u32 body_id = physics_world->create_body(body_def);
            
            // Circle shape
            World2D::CircleShape circle_shape;
            circle_shape.radius = 15.0f;
            circle_shape.density = 0.8f;
            circle_shape.friction = 0.3f;
            circle_shape.restitution = 0.9f;  // Very bouncy
            
            physics_world->attach_circle(body_id, circle_shape);
            
            // Give initial velocity
            physics_world->apply_impulse(body_id, 
                static_cast<f32>((i % 3 - 1) * 50),  // Random horizontal
                static_cast<f32>(-100 - i * 10)       // Upward
            );
            
            // ECS entity
            auto ball_entity = registry->create_entity<Transform2D, PhysicsBody, RenderInfo>(
                Transform2D{body_def.x, body_def.y, 0.0f},
                PhysicsBody{body_id, 0.8f},
                RenderInfo{30.0f, 30.0f, 0.2f, 0.8f, 0.2f, 1.0f}  // Green balls
            );
            
            LOG_DEBUG("Created ball entity: {}", ball_entity.to_string());
        }
    }
    
public:
    void run_simulation(f32 duration = 10.0f) {
        LOG_INFO("Running physics simulation for {:.1f} seconds...", duration);
        
        const f32 time_step = 1.0f / 60.0f;  // 60 FPS
        const u32 velocity_iterations = 8;
        const u32 position_iterations = 3;
        
        u32 frame = 0;
        f32 elapsed_time = 0.0f;
        
        while (elapsed_time < duration) {
            auto frame_start = std::chrono::high_resolution_clock::now();
            
            // Step physics simulation
            physics_world->step(time_step, velocity_iterations, position_iterations);
            
            // Sync physics to ECS
            sync_physics_to_ecs();
            
            // Apply random forces occasionally
            if (frame % 120 == 0 && frame > 0) {  // Every 2 seconds
                apply_random_forces();
            }
            
            // Log state occasionally
            if (frame % 300 == 0) {  // Every 5 seconds
                log_simulation_state(elapsed_time);
            }
            
            frame++;
            elapsed_time += time_step;
            
            auto frame_end = std::chrono::high_resolution_clock::now();
            auto frame_time = std::chrono::duration<f32>(frame_end - frame_start).count();
            
            if (frame_time > time_step) {
                LOG_WARN("Frame {} took {:.4f}s (target: {:.4f}s)", frame, frame_time, time_step);
            }
        }
        
        LOG_INFO("Simulation completed: {} frames in {:.3f} seconds", frame, elapsed_time);
        
        // Final statistics
        log_final_statistics();
    }
    
private:
    void sync_physics_to_ecs() {
        registry->for_each<Transform2D, PhysicsBody>([this](Transform2D& transform, const PhysicsBody& body) {
            if (body.body_id != 0) {
                auto state = physics_world->get_body_state(body.body_id);
                transform.x = state.x;
                transform.y = state.y;
                transform.rotation = state.rotation;
            }
        });
    }
    
    void apply_random_forces() {
        registry->for_each<PhysicsBody>([this](const PhysicsBody& body) {
            if (body.mass > 0.0f) {  // Only apply to dynamic bodies
                f32 force_x = static_cast<f32>((rand() % 201 - 100));  // -100 to 100
                f32 force_y = static_cast<f32>((rand() % 101));        // 0 to 100 (upward)
                
                physics_world->apply_force(body.body_id, force_x, force_y);
            }
        });
        
        LOG_DEBUG("Applied random forces to dynamic bodies");
    }
    
    void log_simulation_state(f32 elapsed_time) {
        LOG_INFO("=== Simulation State at {:.1f}s ===", elapsed_time);
        
        usize dynamic_bodies = 0;
        usize static_bodies = 0;
        f32 total_kinetic_energy = 0.0f;
        
        registry->for_each<Transform2D, PhysicsBody>([&](const Transform2D& transform, const PhysicsBody& body) {
            if (body.mass > 0.0f) {
                dynamic_bodies++;
                
                auto state = physics_world->get_body_state(body.body_id);
                f32 velocity_squared = state.vx * state.vx + state.vy * state.vy;
                total_kinetic_energy += 0.5f * body.mass * velocity_squared;
            } else {
                static_bodies++;
            }
        });
        
        LOG_INFO("Bodies: {} dynamic, {} static", dynamic_bodies, static_bodies);
        LOG_INFO("Total kinetic energy: {:.2f}", total_kinetic_energy);
        
        // Get contact information
        auto contacts = physics_world->get_contacts();
        LOG_INFO("Active contacts: {}", contacts.size());
        
        // Memory statistics
        auto mem_stats = registry->get_memory_statistics();
        LOG_INFO("Memory efficiency: {:.1f}%", mem_stats.memory_efficiency * 100);
    }
    
    void log_final_statistics() {
        LOG_INFO("\\n=== Final Simulation Statistics ===");
        
        // Final positions
        registry->for_each<Transform2D, PhysicsBody, RenderInfo>([](
            const Transform2D& transform, 
            const PhysicsBody& body,
            const RenderInfo& render
        ) {
            LOG_INFO("Body {}: pos=({:.1f}, {:.1f}), rot={:.2f}°, mass={:.1f}", 
                    body.body_id, transform.x, transform.y, 
                    transform.rotation * 180.0f / 3.14159f, body.mass);
        });
        
        // Physics world statistics
        auto contacts = physics_world->get_contacts();
        LOG_INFO("Final contact count: {}", contacts.size());
        
        // ECS statistics
        auto stats = registry->get_memory_statistics();
        LOG_INFO("ECS Statistics:");
        LOG_INFO("  Entities: {}", stats.active_entities);
        LOG_INFO("  Archetypes: {}", stats.total_archetypes);
        LOG_INFO("  Memory efficiency: {:.1f}%", stats.memory_efficiency * 100);
        
        std::string memory_report = registry->generate_memory_report();
        LOG_INFO("\\nMemory Report:\\n{}", memory_report);
    }
};

int main() {
    ecscope::initialize();
    
    PhysicsDemo demo;
    demo.create_scene();
    demo.run_simulation(10.0f);  // Run for 10 seconds
    
    ecscope::shutdown();
    return 0;
}
```

### 3.2: Advanced Physics Features

```cpp
// File: tutorial_03b_advanced_physics.cpp
#include <ecscope/ecscope.hpp>
#include <ecscope/physics.hpp>

using namespace ecscope;
using namespace ecscope::physics;

// Advanced physics components
struct CollisionFilter {
    u16 category_bits;
    u16 mask_bits;
    i16 group_index;
    
    CollisionFilter(u16 category = 1, u16 mask = 0xFFFF, i16 group = 0)
        : category_bits(category), mask_bits(mask), group_index(group) {}
};

struct PhysicsMaterial {
    f32 density;
    f32 friction;
    f32 restitution;
    bool is_sensor;
    
    PhysicsMaterial(f32 d = 1.0f, f32 f = 0.3f, f32 r = 0.0f, bool sensor = false)
        : density(d), friction(f), restitution(r), is_sensor(sensor) {}
};

struct TriggerZone {
    bool is_triggered;
    std::vector<ecs::Entity> entities_inside;
    
    TriggerZone() : is_triggered(false) {}
};

void demonstrate_collision_filtering() {
    LOG_INFO("=== Collision Filtering Demo ===");
    
    auto registry = ecs::create_educational_registry("CollisionFiltering");
    auto physics_world = std::make_unique<World2D>();
    
    physics_world->set_gravity(0.0f, -200.0f);
    
    // Define collision categories
    const u16 GROUND_CATEGORY = 0x0001;
    const u16 PLAYER_CATEGORY = 0x0002;  
    const u16 ENEMY_CATEGORY = 0x0004;
    const u16 PICKUP_CATEGORY = 0x0008;
    
    // Create ground (collides with everything)
    World2D::BodyDef ground_def;
    ground_def.x = 400.0f;
    ground_def.y = 550.0f;
    ground_def.is_static = true;
    
    u32 ground_body = physics_world->create_body(ground_def);
    World2D::BoxShape ground_shape{800.0f, 50.0f, 1.0f, 0.7f, 0.3f};
    physics_world->attach_box(ground_body, ground_shape);
    
    auto ground_entity = registry->create_entity<Transform2D, PhysicsBody, CollisionFilter>(
        Transform2D{400.0f, 550.0f, 0.0f},
        PhysicsBody{ground_body, 0.0f},
        CollisionFilter{GROUND_CATEGORY, 0xFFFF, 0}  // Collides with all
    );
    
    // Create player (doesn't collide with pickups, but triggers them)
    World2D::BodyDef player_def;
    player_def.x = 200.0f;
    player_def.y = 100.0f;
    player_def.is_static = false;
    
    u32 player_body = physics_world->create_body(player_def);
    World2D::CircleShape player_shape{20.0f, 1.0f, 0.3f, 0.0f};
    physics_world->attach_circle(player_body, player_shape);
    
    auto player_entity = registry->create_entity<Transform2D, PhysicsBody, CollisionFilter>(
        Transform2D{200.0f, 100.0f, 0.0f},
        PhysicsBody{player_body, 1.0f},
        CollisionFilter{PLAYER_CATEGORY, GROUND_CATEGORY | ENEMY_CATEGORY, 0}  // Doesn't collide with pickups
    );
    
    // Create enemies (collide with ground and player)
    for (int i = 0; i < 3; ++i) {
        World2D::BodyDef enemy_def;
        enemy_def.x = 300.0f + i * 100.0f;
        enemy_def.y = 200.0f;
        enemy_def.is_static = false;
        
        u32 enemy_body = physics_world->create_body(enemy_def);
        World2D::BoxShape enemy_shape{25.0f, 25.0f, 1.2f, 0.4f, 0.2f};
        physics_world->attach_box(enemy_body, enemy_shape);
        
        auto enemy_entity = registry->create_entity<Transform2D, PhysicsBody, CollisionFilter>(
            Transform2D{enemy_def.x, enemy_def.y, 0.0f},
            PhysicsBody{enemy_body, 1.2f},
            CollisionFilter{ENEMY_CATEGORY, GROUND_CATEGORY | PLAYER_CATEGORY, 0}
        );
    }
    
    // Create pickups (sensors that don't physically collide)
    for (int i = 0; i < 5; ++i) {
        World2D::BodyDef pickup_def;
        pickup_def.x = 150.0f + i * 125.0f;
        pickup_def.y = 400.0f;
        pickup_def.is_static = false;
        
        u32 pickup_body = physics_world->create_body(pickup_def);
        World2D::CircleShape pickup_shape{15.0f, 0.1f, 0.0f, 0.0f};
        physics_world->attach_circle(pickup_body, pickup_shape);
        
        auto pickup_entity = registry->create_entity<Transform2D, PhysicsBody, CollisionFilter, TriggerZone>(
            Transform2D{pickup_def.x, pickup_def.y, 0.0f},
            PhysicsBody{pickup_body, 0.1f},
            CollisionFilter{PICKUP_CATEGORY, PLAYER_CATEGORY, 0},  // Only detects player
            TriggerZone{}
        );
    }
    
    LOG_INFO("Created collision filtering demo scene");
    
    // Simulate collision detection
    const f32 time_step = 1.0f / 60.0f;
    usize collected_pickups = 0;
    
    for (u32 frame = 0; frame < 600; ++frame) {  // 10 seconds
        physics_world->step(time_step, 8, 3);
        
        // Sync physics positions
        registry->for_each<Transform2D, PhysicsBody>([&](Transform2D& transform, const PhysicsBody& body) {
            if (body.body_id != 0) {
                auto state = physics_world->get_body_state(body.body_id);
                transform.x = state.x;
                transform.y = state.y;
                transform.rotation = state.rotation;
            }
        });
        
        // Check for pickup collections (simplified trigger detection)
        if (frame % 60 == 0) {  // Check every second
            auto player_pos = registry->get_component<Transform2D>(player_entity);
            
            registry->for_each<Transform2D, TriggerZone>([&](const Transform2D& pickup_pos, TriggerZone& trigger) {
                if (player_pos) {
                    f32 dx = pickup_pos.x - player_pos->x;
                    f32 dy = pickup_pos.y - player_pos->y;
                    f32 distance = std::sqrt(dx * dx + dy * dy);
                    
                    if (distance < 35.0f && !trigger.is_triggered) {  // Within pickup range
                        trigger.is_triggered = true;
                        collected_pickups++;
                        LOG_INFO("Pickup collected! Total: {}", collected_pickups);
                    }
                }
            });
        }
        
        // Apply player controls (simplified)
        if (frame % 180 == 0) {  // Every 3 seconds, change direction
            physics_world->apply_impulse(player_body, 
                static_cast<f32>((frame / 180) % 2 == 0 ? 100 : -100), 0);
        }
    }
    
    LOG_INFO("Collision filtering demo completed. Collected {} pickups", collected_pickups);
}

int main() {
    ecscope::initialize();
    demonstrate_collision_filtering();
    ecscope::shutdown();
    return 0;
}
```

### 3.3: Chapter 3 Exercises

**Exercise 3.1: Platformer Physics**
```cpp
// Create a simple platformer with:
// 1. Player character with jump mechanics
// 2. Moving platforms
// 3. One-way platforms (can jump through from below)
// 4. Collectible items
// 5. Enemy AI with basic collision
```

**Exercise 3.2: Particle Physics System**
```cpp
// Implement a particle system with:
// 1. Particle emitters with different shapes
// 2. Physics interactions (gravity, wind, collisions)
// 3. Particle pooling for performance
// 4. Visual effects (trails, fading)
// 5. Performance monitoring and optimization
```

This comprehensive tutorial provides a solid foundation for understanding and using ECScope effectively. Each chapter builds upon the previous one, providing both theoretical knowledge and practical implementation experience.

The tutorial emphasizes:
- **Educational value** with clear explanations and well-commented code
- **Practical examples** that demonstrate real-world usage patterns
- **Progressive complexity** from basic concepts to advanced techniques
- **Performance awareness** with optimization tips and benchmarking
- **Best practices** for maintainable and efficient code

Continue with the remaining chapters to master rendering, optimization, WebAssembly deployment, and building complete applications with ECScope.