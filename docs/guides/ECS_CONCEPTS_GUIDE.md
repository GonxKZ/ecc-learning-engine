# Understanding ECS Concepts with ECScope

**A practical guide to Entity-Component-System architecture using ECScope**

## Table of Contents

1. [What is ECS?](#what-is-ecs)
2. [Core Concepts](#core-concepts)
3. [ECScope's ECS Implementation](#ecscopes-ecs-implementation)
4. [Practical Examples](#practical-examples)
5. [Memory Layout and Performance](#memory-layout-and-performance)
6. [Advanced Patterns](#advanced-patterns)
7. [Common Pitfalls](#common-pitfalls)

## What is ECS?

Entity-Component-System (ECS) is an architectural pattern used primarily in game development that emphasizes **composition over inheritance** and **data-oriented design**. Instead of having complex object hierarchies, ECS breaks down game objects into three fundamental parts:

- **Entities**: Unique identifiers (just IDs)
- **Components**: Plain data structures (no behavior)
- **Systems**: Pure functions that operate on components

### Why Use ECS?

**Traditional OOP Problems:**
```cpp
class GameObject {
    virtual void update() = 0;
    virtual void render() = 0;
    // What if an object doesn't need to render?
    // What about objects that move differently?
};

class Player : public GameObject, public Moveable, public Renderable {
    // Complex inheritance hierarchy
    // Diamond problem potential
    // Tight coupling
};
```

**ECS Solution:**
```cpp
// Entities are just IDs
struct Entity { u32 id; };

// Components are pure data
struct Position { f32 x, y; };
struct Velocity { f32 dx, dy; };
struct Health { i32 current, max; };

// Systems are pure functions
void movement_system(Position& pos, const Velocity& vel, f32 dt) {
    pos.x += vel.dx * dt;
    pos.y += vel.dy * dt;
}
```

## Core Concepts

### 1. Entities

Entities in ECScope are lightweight identifiers with generation counters for safety:

```cpp
struct Entity {
    u32 index;      // Position in entity array
    u32 generation; // Prevents use-after-free
};

// Usage
auto player = registry->create_entity<Position, Health>();
auto bullet = registry->create_entity<Position, Velocity>();

// Safe - even if entity is destroyed and recreated
if (registry->is_valid(player)) {
    // Entity still exists
}
```

**Key Points:**
- Entities are just unique IDs
- No behavior, no data storage
- Generation counter prevents dangling references
- Can be created and destroyed dynamically

### 2. Components

Components are plain data structures that represent different aspects of entities:

```cpp
// Good component design - just data
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
    bool is_alive() const { return current > 0; }  // Pure functions OK
};

// Bad component design - behavior mixed with data
struct BadComponent {
    f32 value;
    void update(Registry* registry) { /* Don't do this! */ }
    void render() { /* Components shouldn't render! */ }
};
```

**Component Best Practices:**
- Keep components small and focused
- Avoid behavior in components
- Use descriptive names
- Consider memory layout (group related data)

### 3. Systems

Systems contain all the behavior and operate on entities with specific components:

```cpp
// System function signature
void movement_system(Registry& registry, f32 delta_time) {
    // Operate on all entities with Position and Velocity
    registry.for_each<Position, Velocity>([=](Position& pos, const Velocity& vel) {
        pos.x += vel.dx * delta_time;
        pos.y += vel.dy * delta_time;
    });
}

// More complex system with multiple component queries
void collision_system(Registry& registry) {
    auto moving_entities = registry.get_entities_with<Position, Velocity, Collider>();
    auto static_entities = registry.get_entities_with<Position, Collider>();
    
    for (auto moving : moving_entities) {
        for (auto static_entity : static_entities) {
            if (moving == static_entity) continue;
            
            auto* moving_pos = registry.get_component<Position>(moving);
            auto* moving_col = registry.get_component<Collider>(moving);
            auto* static_pos = registry.get_component<Position>(static_entity);
            auto* static_col = registry.get_component<Collider>(static_entity);
            
            if (check_collision(*moving_pos, *moving_col, *static_pos, *static_col)) {
                // Handle collision
                resolve_collision(moving, static_entity, registry);
            }
        }
    }
}
```

## ECScope's ECS Implementation

### Registry Architecture

ECScope uses an **archetype-based** ECS implementation for optimal memory layout and performance:

```cpp
// Create registry with educational features
auto registry = ecs::create_educational_registry("MyGame");

// Archetype = unique combination of components
// Entities with {Position, Velocity} stored together
// Entities with {Position, Health} stored separately  
// Entities with {Position, Velocity, Health} in another archetype
```

### Component Storage

ECScope organizes entities by **archetype** (unique component combination):

```
Archetype 1: Position + Velocity
[Entity1][Entity2][Entity3]...
[Pos1  ][Pos2  ][Pos3  ]... (contiguous in memory)
[Vel1  ][Vel2  ][Vel3  ]... (contiguous in memory)

Archetype 2: Position + Health  
[Entity4][Entity5]...
[Pos4  ][Pos5  ]... (separate contiguous block)
[Health4][Health5]...
```

**Benefits:**
- Cache-friendly iteration
- No sparse storage for unused components
- Fast component access
- Memory efficiency

### Memory Management Integration

ECScope's ECS integrates with custom allocators for educational purposes:

```cpp
// Different allocation strategies
auto educational_registry = ecs::create_educational_registry("Learn");  // Arena + Pool
auto performance_registry = ecs::create_performance_registry("Fast");   // Optimized allocators
auto conservative_registry = ecs::create_conservative_registry("Safe"); // Standard allocators

// Memory analysis
auto stats = registry->get_memory_statistics();
std::cout << "Arena utilization: " << stats.arena_utilization() * 100 << "%\n";
std::cout << "Cache hit ratio: " << stats.cache_hit_ratio * 100 << "%\n";
```

## Practical Examples

### Example 1: Simple Game Loop

```cpp
#include <ecscope/ecscope.hpp>
using namespace ecscope;

// Define components
struct Position { f32 x, y; };
struct Velocity { f32 dx, dy; };  
struct Health { i32 current, max; };
struct Sprite { f32 width, height; f32 r, g, b, a; };

int main() {
    ecscope::initialize();
    auto registry = ecs::create_educational_registry("SimpleGame");
    
    // Create entities
    auto player = registry->create_entity<Position, Velocity, Health, Sprite>(
        Position{100.0f, 100.0f},
        Velocity{0.0f, 0.0f},
        Health{100, 100},
        Sprite{32.0f, 32.0f, 1.0f, 0.0f, 0.0f, 1.0f}
    );
    
    // Create enemies
    for (int i = 0; i < 10; ++i) {
        registry->create_entity<Position, Velocity, Health, Sprite>(
            Position{static_cast<f32>(i * 50), 0.0f},
            Velocity{0.0f, 50.0f},
            Health{50, 50},
            Sprite{24.0f, 24.0f, 0.0f, 1.0f, 0.0f, 1.0f}
        );
    }
    
    // Game loop
    f32 dt = 1.0f / 60.0f; // 60 FPS
    for (int frame = 0; frame < 3600; ++frame) { // Run for 1 minute
        
        // Movement system
        registry->for_each<Position, Velocity>([dt](Position& pos, const Velocity& vel) {
            pos.x += vel.dx * dt;
            pos.y += vel.dy * dt;
        });
        
        // Health system
        registry->for_each<Health>([](Health& health) {
            if (health.current <= 0) {
                // Mark for deletion (in a real game)
            }
        });
        
        // Print stats every 60 frames (1 second)
        if (frame % 60 == 0) {
            auto stats = registry->get_memory_statistics();
            LOG_INFO("Frame {}: {} entities, {:.1f}% memory efficiency", 
                    frame, stats.active_entities, stats.memory_efficiency * 100.0f);
        }
    }
    
    ecscope::shutdown();
    return 0;
}
```

### Example 2: Component Addition and Removal

```cpp
void demonstrate_component_lifecycle() {
    auto registry = ecs::create_educational_registry("ComponentDemo");
    
    // Start with basic entity
    auto entity = registry->create_entity<Position>(Position{0.0f, 0.0f});
    LOG_INFO("Created entity with Position only");
    
    // Add velocity to make it move
    bool success = registry->add_component<Velocity>(entity, Velocity{10.0f, 0.0f});
    if (success) {
        LOG_INFO("Added Velocity component - entity can now move!");
        
        // Entity automatically migrates to new archetype: Position + Velocity
        auto stats = registry->get_memory_statistics();
        LOG_INFO("Now have {} archetypes", stats.total_archetypes);
    }
    
    // Add health for gameplay
    registry->add_component<Health>(entity, Health{100, 100});
    LOG_INFO("Added Health - entity is now: Position + Velocity + Health");
    
    // Remove velocity to make it static
    registry->remove_component<Velocity>(entity);
    LOG_INFO("Removed Velocity - entity is now: Position + Health");
    
    // Query current components
    if (registry->has_component<Position>(entity)) LOG_INFO("✓ Has Position");
    if (registry->has_component<Velocity>(entity)) LOG_INFO("✓ Has Velocity");
    else LOG_INFO("✗ No Velocity");
    if (registry->has_component<Health>(entity)) LOG_INFO("✓ Has Health");
}
```

### Example 3: Advanced Queries and Systems

```cpp
void demonstrate_advanced_queries() {
    auto registry = ecs::create_educational_registry("AdvancedDemo");
    
    // Create different entity types
    // Moving entities
    for (int i = 0; i < 100; ++i) {
        registry->create_entity<Position, Velocity>(
            Position{static_cast<f32>(i), 0.0f},
            Velocity{1.0f, 0.0f}
        );
    }
    
    // Living entities
    for (int i = 0; i < 50; ++i) {
        registry->create_entity<Position, Health>(
            Position{static_cast<f32>(i), 100.0f},
            Health{100, 100}
        );
    }
    
    // Player-like entities (all components)
    for (int i = 0; i < 10; ++i) {
        registry->create_entity<Position, Velocity, Health>(
            Position{static_cast<f32>(i), 200.0f},
            Velocity{0.5f, -0.5f},
            Health{150, 150}
        );
    }
    
    // Query specific combinations
    LOG_INFO("=== Query Results ===");
    
    auto moving = registry->get_entities_with<Position, Velocity>();
    LOG_INFO("Entities that can move: {}", moving.size());
    
    auto living = registry->get_entities_with<Position, Health>();
    LOG_INFO("Entities with health: {}", living.size());
    
    auto players = registry->get_entities_with<Position, Velocity, Health>();
    LOG_INFO("Player-like entities: {}", players.size());
    
    // Complex system: damage over time for moving entities with health
    registry->for_each<Position, Velocity, Health>([](
        const Position& pos, const Velocity& vel, Health& health
    ) {
        f32 speed = std::sqrt(vel.dx * vel.dx + vel.dy * vel.dy);
        if (speed > 1.0f) {
            health.current -= 1; // Moving fast damages health
        }
    });
    
    // Count damaged entities
    usize damaged = 0;
    registry->for_each<Health>([&damaged](const Health& health) {
        if (health.current < health.maximum) damaged++;
    });
    
    LOG_INFO("Entities damaged by movement: {}", damaged);
}
```

## Memory Layout and Performance

### Understanding Archetypes

ECScope automatically groups entities by their component signature:

```cpp
// These entities will be stored in different archetypes:
auto bullet = registry->create_entity<Position, Velocity>();           // Archetype A
auto target = registry->create_entity<Position, Health>();             // Archetype B  
auto player = registry->create_entity<Position, Velocity, Health>();   // Archetype C

// Archetype storage layout:
// Archetype A: [Entity IDs...] [Positions...] [Velocities...]
// Archetype B: [Entity IDs...] [Positions...] [Healths...]  
// Archetype C: [Entity IDs...] [Positions...] [Velocities...] [Healths...]
```

### Performance Implications

```cpp
void analyze_performance_characteristics() {
    auto registry = ecs::create_educational_registry("PerfAnalysis");
    
    // Create many entities of the same archetype (good for cache)
    for (int i = 0; i < 10000; ++i) {
        registry->create_entity<Position, Velocity>(
            Position{static_cast<f32>(i), 0.0f},
            Velocity{1.0f, 0.0f}
        );
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // This will be VERY fast due to data locality
    registry->for_each<Position, Velocity>([](Position& pos, const Velocity& vel) {
        pos.x += vel.dx * 0.016f;
        pos.y += vel.dy * 0.016f;
    });
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    LOG_INFO("Updated 10000 entities in {} microseconds", duration.count());
    LOG_INFO("That's {} entities per microsecond!", 10000.0 / duration.count());
    
    // Memory statistics
    auto stats = registry->get_memory_statistics();
    LOG_INFO("Memory efficiency: {:.2f}%", stats.memory_efficiency * 100.0);
    LOG_INFO("Cache hit ratio: {:.2f}%", stats.cache_hit_ratio * 100.0);
}
```

### Cache-Friendly Iteration

ECScope's archetype storage ensures optimal cache usage:

```cpp
// BAD: Iterating sparse arrays
for (int i = 0; i < MAX_ENTITIES; ++i) {
    if (has_position[i] && has_velocity[i]) {  // Branch misprediction
        position[i].x += velocity[i].dx;       // Random memory access
    }
}

// GOOD: ECScope archetype iteration  
registry->for_each<Position, Velocity>([](Position& pos, const Velocity& vel) {
    pos.x += vel.dx;  // Sequential memory access, no branches
});
```

## Advanced Patterns

### 1. System Dependencies

```cpp
class SystemManager {
    std::vector<std::function<void()>> systems;
    
public:
    void add_system(std::function<void()> system) {
        systems.push_back(system);
    }
    
    void run_frame() {
        // Systems run in order
        for (auto& system : systems) {
            system();
        }
    }
};

void setup_game_systems(SystemManager& manager, ecs::Registry& registry) {
    // Order matters!
    manager.add_system([&]() { input_system(registry); });
    manager.add_system([&]() { movement_system(registry); });
    manager.add_system([&]() { physics_system(registry); });
    manager.add_system([&]() { collision_system(registry); });
    manager.add_system([&]() { damage_system(registry); });
    manager.add_system([&]() { cleanup_system(registry); });
    manager.add_system([&]() { render_system(registry); });
}
```

### 2. Component Relationships

```cpp
// Parent-child relationships using components
struct Parent {
    Entity parent_entity;
    Parent(Entity parent) : parent_entity(parent) {}
};

struct Children {
    std::vector<Entity> child_entities;
    void add_child(Entity child) { child_entities.push_back(child); }
};

// Transform inheritance system
void transform_hierarchy_system(ecs::Registry& registry) {
    registry.for_each<Position, Parent>([&](Position& pos, const Parent& parent) {
        if (auto* parent_pos = registry.get_component<Position>(parent.parent_entity)) {
            // Apply parent's transform to child (simplified)
            pos.x += parent_pos->x;
            pos.y += parent_pos->y;
        }
    });
}
```

### 3. Event Systems

```cpp
// Event components for loose coupling
struct CollisionEvent {
    Entity other_entity;
    f32 impact_force;
};

struct DamageEvent {
    i32 damage_amount;
    Entity source;
};

// Systems can add events as components
void collision_detection_system(ecs::Registry& registry) {
    // ... collision detection logic ...
    
    if (collision_detected) {
        // Add collision event to entity
        registry.add_component<CollisionEvent>(entity1, 
            CollisionEvent{entity2, impact_force});
    }
}

// Other systems respond to events
void damage_system(ecs::Registry& registry) {
    registry.for_each<CollisionEvent, Health>([&](
        const CollisionEvent& collision, Health& health
    ) {
        health.current -= static_cast<i32>(collision.impact_force);
    });
    
    // Remove processed events
    auto entities_with_collision_events = registry.get_entities_with<CollisionEvent>();
    for (auto entity : entities_with_collision_events) {
        registry.remove_component<CollisionEvent>(entity);
    }
}
```

## Common Pitfalls

### 1. Storing Behavior in Components

```cpp
// DON'T DO THIS
struct BadComponent {
    f32 value;
    
    void update(Registry& registry) {  // No behavior in components!
        // This breaks ECS principles
    }
    
    void render() {  // Components shouldn't know about rendering!
        // This creates tight coupling
    }
};

// DO THIS INSTEAD
struct GoodComponent {
    f32 value;  // Just data
    
    // Pure functions are OK
    bool is_zero() const { return value == 0.0f; }
};

// Behavior goes in systems
void update_system(Registry& registry) {
    registry.for_each<GoodComponent>([](GoodComponent& comp) {
        // System contains the behavior
        comp.value += 1.0f;
    });
}
```

### 2. Over-Engineering Component Granularity

```cpp
// TOO GRANULAR - creates many archetypes
struct X { f32 value; };
struct Y { f32 value; };  
struct Z { f32 value; };

// BETTER - group related data
struct Position { f32 x, y, z; };

// CONTEXT MATTERS
// If you often need X without Y, separate them
// If you always use X and Y together, combine them
```

### 3. Ignoring Memory Layout

```cpp
// BAD - frequent archetype changes
for (auto entity : entities) {
    if (some_condition) {
        registry.add_component<TempComponent>(entity, {});  // Archetype migration
    }
    // Later...
    registry.remove_component<TempComponent>(entity);      // Another migration
}

// BETTER - use flags or state components
struct StateFlags {
    bool is_active : 1;
    bool is_visible : 1;
    bool is_moving : 1;
    // Pack boolean state into single component
};
```

### 4. Circular Dependencies Between Systems

```cpp
// BAD - systems modifying components other systems need
void system_a(Registry& registry) {
    registry.for_each<Position>([](Position& pos) {
        pos.x += 1.0f;
    });
}

void system_b(Registry& registry) {
    registry.for_each<Position>([](Position& pos) {
        pos.x -= 1.0f;  // Conflicts with system_a!
    });
}

// BETTER - clear ownership and ordering
void movement_system(Registry& registry, f32 dt) {
    registry.for_each<Position, Velocity>([=](Position& pos, const Velocity& vel) {
        pos.x += vel.dx * dt;  // Only movement system modifies position
    });
}

void control_system(Registry& registry) {
    registry.for_each<Velocity, InputComponent>([](Velocity& vel, const InputComponent& input) {
        vel.dx = input.horizontal * 100.0f;  // Control system modifies velocity
    });
}
```

## Educational Exercises

### Exercise 1: Build a Particle System

Create a simple particle system using ECScope:

```cpp
struct Particle {
    f32 life_time;
    f32 max_life;
};

struct Color {
    f32 r, g, b, a;
};

// Your task: implement particle_system() that:
// 1. Updates particle lifetime
// 2. Fades color based on remaining life
// 3. Removes dead particles
// 4. Spawns new particles
```

### Exercise 2: Implement a Spatial Hash

Create a spatial partitioning system for efficient collision detection:

```cpp
struct SpatialHash {
    std::unordered_map<u32, std::vector<Entity>> grid;
    f32 cell_size;
};

// Your task: implement spatial hashing system that:
// 1. Places entities in grid cells based on position
// 2. Only checks collisions within same/adjacent cells
// 3. Updates entity positions efficiently
```

### Exercise 3: Create a Component Dependency System

Build a system that automatically manages component dependencies:

```cpp
// Example: MovingSprite requires Position, Velocity, and Sprite
struct MovingSprite {};

// Your task: create a system that:
// 1. Automatically adds required components when MovingSprite is added
// 2. Validates component combinations
// 3. Warns about missing dependencies
```

ECScope's ECS implementation provides a solid foundation for understanding these concepts while maintaining high performance and educational value. Experiment with different patterns and use the built-in profiling tools to understand the performance implications of your design decisions.