# Getting Started with ECScope

**Complete learning path for ECScope Educational ECS Engine**

## Table of Contents

1. [Prerequisites and Setup](#prerequisites-and-setup)
2. [Learning Path Overview](#learning-path-overview)
3. [Phase-by-Phase Tutorials](#phase-by-phase-tutorials)
4. [Interactive Laboratories](#interactive-laboratories)
5. [Performance Analysis](#performance-analysis)
6. [Advanced Topics](#advanced-topics)
7. [Next Steps](#next-steps)

## Prerequisites and Setup

### Required Knowledge

**Recommended Background**:
- Intermediate C++ knowledge (classes, templates, RAII)
- Basic understanding of memory management concepts
- Familiarity with game development concepts (optional but helpful)
- Understanding of basic linear algebra (vectors, matrices)

**Learning Prerequisites**:
- Willingness to experiment and modify code
- Interest in understanding "why" not just "how"
- Comfort with command-line tools and build systems

### System Requirements

**Development Environment**:
- **C++20** compatible compiler:
  - GCC 10+ (recommended: GCC 11+)
  - Clang 12+ (recommended: Clang 14+)
  - MSVC 2022+ (Visual Studio 2022)
- **CMake 3.22+** for build system
- **Git** for version control

**Optional Graphics Dependencies**:
- **SDL2** development libraries
- **OpenGL 3.3+** drivers
- **ImGui** (included in external/)

### Installation Guide

#### Option 1: Console-Only Mode (Fastest Start)

```bash
# Clone the repository
git clone https://github.com/your-repo/ecscope.git
cd ecscope

# Build console version (no graphics dependencies needed)
mkdir build && cd build
cmake ..
make

# Test your installation
./ecscope_performance_lab_console
```

#### Option 2: Full Graphics Mode (Complete Experience)

**Ubuntu/Debian**:
```bash
# Install graphics dependencies
sudo apt update
sudo apt install libsdl2-dev libgl1-mesa-dev

# Build with graphics support
mkdir build && cd build
cmake -DECSCOPE_ENABLE_GRAPHICS=ON ..
make

# Run interactive interface
./ecscope_ui
```

**macOS**:
```bash
# Install dependencies with Homebrew
brew install sdl2

# Build with graphics
mkdir build && cd build
cmake -DECSCOPE_ENABLE_GRAPHICS=ON ..
make
```

**Windows**:
```bash
# Using vcpkg for dependencies
vcpkg install sdl2

# Configure with vcpkg
cmake -DECSCOPE_ENABLE_GRAPHICS=ON -DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg.cmake ..
```

#### Option 3: Educational Development Setup

```bash
# Clone with full history for learning
git clone --recursive https://github.com/your-repo/ecscope.git
cd ecscope

# Create educational build configuration
mkdir build-educational && cd build-educational
cmake -DECSCOPE_ENABLE_GRAPHICS=ON \
      -DECSCOPE_BUILD_TESTS=ON \
      -DECSCOPE_BUILD_EXAMPLES=ON \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
make

# Verify all components built successfully
ls -la ecscope_*
```

## Learning Path Overview

ECScope is designed for progressive learning through hands-on experimentation:

### Learning Journey Map

```
Beginner Path (1-2 weeks)
├── Phase 1: Core Foundation
│   ├── Understanding modern C++ types
│   ├── EntityID and generational counters
│   └── Error handling with Result<T,E>
├── Phase 2: Basic ECS
│   ├── Component design patterns
│   ├── Entity creation and management
│   └── Simple system implementation
└── Memory Fundamentals
    ├── Basic allocation patterns
    ├── Performance measurement
    └── Memory usage analysis

Intermediate Path (2-4 weeks)
├── Phase 3: Interactive Debugging
│   ├── UI system integration
│   ├── Real-time performance monitoring
│   └── Interactive parameter modification
├── Phase 4: Memory Mastery
│   ├── Custom allocator implementation
│   ├── Cache behavior analysis
│   └── Performance optimization techniques
└── Phase 5: Physics Understanding
    ├── Collision detection algorithms
    ├── Constraint solving methods
    └── Spatial optimization techniques

Advanced Path (4+ weeks)
├── Phase 6: Advanced ECS Patterns
│   ├── Complex component relationships
│   ├── System scheduling optimization
│   └── Event-driven architectures
├── Phase 7: Graphics Programming
│   ├── Modern OpenGL pipeline
│   ├── GPU optimization techniques
│   └── Render analysis and debugging
└── Research and Extension
    ├── Custom system implementation
    ├── Performance research projects
    └── Educational tool development
```

## Phase-by-Phase Tutorials

### Phase 1: Core Foundation Tutorial

**Objective**: Understand the foundational types and systems that make ECScope educational and performant.

**Key Concepts**:
- Modern C++ type design for educational clarity
- Generational EntityID system for safety
- Result<T, E> error handling
- High-precision timing for benchmarking

**Hands-on Exercise**:
```cpp
#include "core/types.hpp"
#include "core/id.hpp"
#include "core/result.hpp"
#include "core/time.hpp"

// Exercise 1: Understanding type safety
void type_safety_demo() {
    // ECScope uses educational type names
    u32 entity_count = 1000;        // Clear intent: unsigned 32-bit
    f64 delta_time = 0.016667;      // Clear intent: 64-bit float
    usize memory_size = 4 * MB;     // Clear intent: size type with readable constants
    
    std::cout << "Entity count: " << entity_count << std::endl;
    std::cout << "Delta time: " << delta_time << "s" << std::endl;
    std::cout << "Memory size: " << (memory_size / MB) << " MB" << std::endl;
}

// Exercise 2: EntityID safety
void entity_id_demo() {
    EntityIDGenerator generator;
    
    // Create entities with generational safety
    auto entity1 = generator.generate();
    auto entity2 = generator.generate();
    
    std::cout << "Entity 1 - ID: " << entity1.id() 
              << ", Generation: " << entity1.generation() << std::endl;
    std::cout << "Entity 2 - ID: " << entity2.id() 
              << ", Generation: " << entity2.generation() << std::endl;
    
    // Demonstrate stale reference detection
    generator.recycle(entity1);
    auto entity3 = generator.generate(); // Reuses ID but increments generation
    
    // entity1 is now stale and can be detected
    std::cout << "Entity 3 (reused) - ID: " << entity3.id() 
              << ", Generation: " << entity3.generation() << std::endl;
}

// Exercise 3: Performance timing
void timing_demo() {
    Timer timer;
    
    // Measure operation performance
    timer.start();
    
    // Simulate some work
    std::vector<i32> data(10000);
    std::iota(data.begin(), data.end(), 0);
    
    auto elapsed = timer.elapsed_ms();
    std::cout << "Vector initialization took: " << elapsed << " ms" << std::endl;
}
```

**Learning Outcomes**:
- Understand the importance of type safety in systems programming
- See how generational IDs prevent common entity management bugs
- Learn performance measurement techniques essential for optimization

### Phase 2: ECS Fundamentals Tutorial

**Objective**: Master the core ECS concepts through hands-on implementation and experimentation.

**Key Concepts**:
- Component design with C++20 concepts
- Entity creation and archetype management
- Component storage and access patterns
- Basic system implementation

**Hands-on Exercise**:
```cpp
#include "ecs/registry.hpp"
#include "ecs/components/transform.hpp"

// Exercise 1: Component creation and validation
struct Velocity : ComponentBase {
    Vec2 velocity{0.0f, 0.0f};
    f32 max_speed{100.0f};
    
    void apply_force(const Vec2& force, f32 mass) {
        velocity += force / mass;
        
        // Clamp to max speed
        f32 speed = velocity.magnitude();
        if (speed > max_speed) {
            velocity = velocity.normalized() * max_speed;
        }
    }
};

static_assert(Component<Velocity>); // Compile-time validation

// Exercise 2: Entity creation and management
void ecs_basics_demo() {
    auto registry = std::make_unique<ecs::Registry>(
        ecs::AllocatorConfig::create_educational_focused(),
        "Tutorial_Registry"
    );
    
    // Create entities with different component combinations
    auto static_entity = registry->create_entity(
        Transform{Vec2{0, 0}, 0.0f, Vec2{1, 1}}
    );
    
    auto moving_entity = registry->create_entity(
        Transform{Vec2{100, 100}, 0.0f, Vec2{1, 1}},
        Velocity{Vec2{50, 0}, 200.0f}
    );
    
    // Query entities with specific components
    auto moving_entities = registry->get_entities_with<Transform, Velocity>();
    std::cout << "Found " << moving_entities.size() << " moving entities" << std::endl;
    
    // Iterate over entities with lambda
    registry->for_each<Transform, Velocity>([](Entity e, Transform& t, Velocity& v) {
        std::cout << "Entity " << e << " at position (" 
                  << t.position.x << ", " << t.position.y << ")" << std::endl;
    });
}

// Exercise 3: Understanding archetype behavior
void archetype_demo() {
    auto registry = std::make_unique<ecs::Registry>(
        ecs::AllocatorConfig::create_educational_focused(),
        "Archetype_Tutorial"
    );
    
    // Create entities that will be stored in different archetypes
    auto transform_only = registry->create_entity(Transform{});
    auto transform_velocity = registry->create_entity(Transform{}, Velocity{});
    
    // Check archetype statistics
    auto archetype_stats = registry->get_archetype_stats();
    for (const auto& [signature, count] : archetype_stats) {
        std::cout << "Archetype " << signature.to_string() 
                  << " contains " << count << " entities" << std::endl;
    }
    
    // Demonstrate archetype migration by adding component
    registry->add_component(transform_only, Velocity{Vec2{10, 20}});
    
    // Check updated archetype statistics
    std::cout << "\nAfter component addition:" << std::endl;
    archetype_stats = registry->get_archetype_stats();
    for (const auto& [signature, count] : archetype_stats) {
        std::cout << "Archetype " << signature.to_string() 
                  << " contains " << count << " entities" << std::endl;
    }
}
```

**Learning Outcomes**:
- Understand how components define entity behavior
- See how archetypes optimize memory layout
- Learn component query patterns and performance implications
- Experience the benefits of SoA storage firsthand

### Phase 3: Memory Management Mastery

**Objective**: Master custom allocators and understand their impact on performance and cache behavior.

**Key Concepts**:
- Arena allocator implementation and benefits
- Pool allocator for fixed-size allocations
- PMR integration with standard containers
- Memory access pattern analysis

**Hands-on Exercise**:
```cpp
#include "memory/arena.hpp"
#include "memory/pool.hpp"
#include "memory/memory_tracker.hpp"

// Exercise 1: Arena allocator comparison
void arena_allocator_demo() {
    const usize num_allocations = 10000;
    const usize allocation_size = sizeof(Transform);
    
    // Test standard allocation
    {
        ScopeTimer timer("Standard Allocation");
        std::vector<Transform*> ptrs;
        ptrs.reserve(num_allocations);
        
        for (usize i = 0; i < num_allocations; ++i) {
            ptrs.push_back(new Transform{});
        }
        
        // Cleanup
        for (auto* ptr : ptrs) {
            delete ptr;
        }
    }
    
    // Test arena allocation
    {
        ScopeTimer timer("Arena Allocation");
        memory::ArenaAllocator arena{allocation_size * num_allocations};
        std::vector<Transform*> ptrs;
        ptrs.reserve(num_allocations);
        
        for (usize i = 0; i < num_allocations; ++i) {
            ptrs.push_back(arena.allocate<Transform>());
        }
        
        // Arena automatically cleans up all allocations
    }
    
    // Compare results
    std::cout << "Check the timing results above to see the performance difference!" << std::endl;
}

// Exercise 2: Memory access pattern analysis
void memory_pattern_demo() {
    // Create registry with memory tracking enabled
    auto registry = std::make_unique<ecs::Registry>(
        ecs::AllocatorConfig::create_educational_focused(),
        "Memory_Pattern_Demo"
    );
    
    // Create entities for pattern analysis
    std::vector<Entity> entities;
    for (i32 i = 0; i < 1000; ++i) {
        entities.push_back(registry->create_entity(
            Transform{Vec2{static_cast<f32>(i), static_cast<f32>(i)}}
        ));
    }
    
    // Sequential access pattern (cache-friendly)
    {
        memory::tracker::start_analysis("Sequential Access");
        registry->for_each<Transform>([](Entity e, Transform& t) {
            t.position.x += 1.0f; // Sequential memory access
        });
        memory::tracker::end_analysis();
    }
    
    // Random access pattern (cache-hostile)
    {
        memory::tracker::start_analysis("Random Access");
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, entities.size() - 1);
        
        for (i32 i = 0; i < 1000; ++i) {
            Entity random_entity = entities[dis(gen)];
            auto* transform = registry->get_component<Transform>(random_entity);
            if (transform) {
                transform->position.x += 1.0f; // Random memory access
            }
        }
        memory::tracker::end_analysis();
    }
    
    // Analyze and display results
    auto analysis = memory::tracker::get_analysis_results();
    for (const auto& result : analysis) {
        std::cout << "Analysis: " << result.name << std::endl;
        std::cout << "  Cache hit ratio: " << (result.cache_hit_ratio * 100.0) << "%" << std::endl;
        std::cout << "  Average access time: " << result.average_access_time << " ns" << std::endl;
        std::cout << "  Memory efficiency: " << (result.memory_efficiency * 100.0) << "%" << std::endl;
    }
}
```

### Phase 4: Physics Engine Understanding

**Objective**: Understand 2D physics algorithms through step-by-step visualization and interactive experimentation.

**Key Concepts**:
- Collision detection algorithms (SAT, GJK)
- Constraint solving with sequential impulse method
- Spatial partitioning for performance
- Numerical integration techniques

**Hands-on Exercise**:
```cpp
#include "physics/physics_world.hpp"
#include "physics/components/physics_components.hpp"

// Exercise 1: Basic physics simulation
void basic_physics_demo() {
    // Create physics world with educational configuration
    physics::PhysicsWorldConfig config;
    config.gravity = Vec2{0.0f, -9.81f};
    config.enable_step_visualization = true;
    config.enable_profiling = true;
    
    auto world = std::make_unique<physics::PhysicsWorld>(config);
    auto registry = std::make_unique<ecs::Registry>(
        ecs::AllocatorConfig::create_educational_focused(),
        "Physics_Demo"
    );
    
    // Create a bouncing ball
    auto ball = registry->create_entity(
        Transform{Vec2{0, 100}},
        physics::RigidBody2D{
            .mass = 1.0f,
            .restitution = 0.8f,
            .friction = 0.3f
        },
        physics::CircleCollider{.radius = 5.0f}
    );
    
    // Create ground
    auto ground = registry->create_entity(
        Transform{Vec2{0, -50}},
        physics::RigidBody2D{
            .mass = 0.0f, // Static body
            .restitution = 0.6f,
            .friction = 0.8f
        },
        physics::BoxCollider{.half_extents = Vec2{100, 5}}
    );
    
    // Run simulation with step-by-step analysis
    const f32 time_step = 1.0f / 60.0f;
    for (i32 step = 0; step < 300; ++step) { // 5 seconds at 60 FPS
        // Enable step-by-step mode for first 10 steps
        if (step < 10) {
            world->enable_step_mode(true);
            std::cout << "\n=== Physics Step " << step << " ===" << std::endl;
        } else {
            world->enable_step_mode(false);
        }
        
        world->update(*registry, time_step);
        
        if (step < 10) {
            // Get step breakdown for educational analysis
            auto breakdown = world->get_debug_step_breakdown();
            for (const auto& step_info : breakdown) {
                std::cout << "  " << step_info << std::endl;
            }
            
            // Display ball position
            auto* ball_transform = registry->get_component<Transform>(ball);
            if (ball_transform) {
                std::cout << "Ball position: (" 
                          << ball_transform->position.x << ", " 
                          << ball_transform->position.y << ")" << std::endl;
            }
        }
    }
    
    // Display final performance statistics
    auto stats = world->get_performance_statistics();
    std::cout << "\nFinal Performance Statistics:" << std::endl;
    std::cout << "Average update time: " << stats.average_update_time << " ms" << std::endl;
    std::cout << "Collision checks: " << stats.total_collision_checks << std::endl;
    std::cout << "Active contacts: " << stats.active_contact_count << std::endl;
}
```

### Phase 5: Rendering Pipeline Understanding

**Objective**: Master modern 2D rendering techniques and GPU optimization through interactive visualization.

**Hands-on Exercise**:
```cpp
// Exercise: Progressive rendering complexity
void rendering_progression_demo() {
    // Start with console-based "rendering" for concept understanding
    std::cout << "=== Rendering Concept Demo (Console) ===" << std::endl;
    
    // Demonstrate render command generation
    struct SimpleRenderCommand {
        Vec2 position;
        Vec2 size;
        Color color;
    };
    
    std::vector<SimpleRenderCommand> render_commands;
    
    // Generate render commands for 100 sprites
    for (i32 i = 0; i < 100; ++i) {
        render_commands.push_back({
            .position = Vec2{static_cast<f32>(i * 10), static_cast<f32>(i * 5)},
            .size = Vec2{8, 8},
            .color = Color{static_cast<u8>(i * 2), 100, 150, 255}
        });
    }
    
    std::cout << "Generated " << render_commands.size() << " render commands" << std::endl;
    
    // Demonstrate batching concept
    std::unordered_map<u32, std::vector<SimpleRenderCommand>> batches;
    for (const auto& cmd : render_commands) {
        u32 batch_key = cmd.color.r; // Simple batching by red component
        batches[batch_key].push_back(cmd);
    }
    
    std::cout << "Batched into " << batches.size() << " draw calls" << std::endl;
    for (const auto& [key, commands] : batches) {
        std::cout << "  Batch " << key << ": " << commands.size() << " sprites" << std::endl;
    }
}
```

## Interactive Laboratories

### Memory Laboratory

**Location**: `./ecscope_performance_lab_console`

**Features**:
- **Allocation Strategy Comparison**: See Arena vs Pool vs Standard allocation performance
- **Cache Behavior Analysis**: Understand cache-friendly vs cache-hostile patterns
- **Memory Usage Tracking**: Monitor memory pressure and fragmentation
- **Interactive Experiments**: Modify parameters and see immediate results

**Tutorial Session**:
```bash
# Run the memory laboratory
./ecscope_performance_lab_console

# Follow the interactive prompts to:
# 1. Compare allocation strategies
# 2. Analyze memory access patterns  
# 3. Understand cache behavior
# 4. Experiment with different configurations
```

### Performance Laboratory

**Location**: `./ecscope_ui` (Graphics Mode)

**Features**:
- **Real-time Performance Monitoring**: Live graphs of system performance
- **Memory Visualization**: See allocation patterns and cache behavior
- **Physics Algorithm Visualization**: Watch collision detection in action
- **Interactive Parameter Modification**: Change system behavior in real-time

## Performance Analysis

### Understanding ECScope Performance

**Key Performance Metrics**:

1. **Entity Creation Rate**
   - **Measurement**: Entities created per second
   - **Target**: 10,000 entities in ~8ms
   - **Factors**: Allocator strategy, component complexity, archetype migration

2. **Component Access Performance**
   - **Measurement**: Component accesses per second
   - **Target**: 1000 accesses in 1.4ms
   - **Factors**: Cache locality, memory layout, access patterns

3. **Memory Efficiency**
   - **Measurement**: Actual data / Total allocated memory
   - **Target**: >90% efficiency with custom allocators
   - **Factors**: Allocator choice, fragmentation, overhead

**Performance Analysis Tools**:

```cpp
// Built-in performance analysis
void analyze_performance() {
    auto registry = std::make_unique<ecs::Registry>(
        ecs::AllocatorConfig::create_educational_focused(),
        "Performance_Analysis"
    );
    
    // Run built-in benchmarks
    registry->benchmark_allocators("Entity_Creation_Test", 10000);
    registry->benchmark_allocators("Component_Access_Test", 5000);
    
    // Get detailed performance comparisons
    auto comparisons = registry->get_performance_comparisons();
    for (const auto& comp : comparisons) {
        std::cout << "Test: " << comp.operation_name << std::endl;
        std::cout << "  Standard time: " << comp.standard_allocator_time << " ms" << std::endl;
        std::cout << "  Custom time: " << comp.custom_allocator_time << " ms" << std::endl;
        std::cout << "  Speedup: " << comp.speedup_factor << "x" << std::endl;
        std::cout << "  Improvement: " << comp.improvement_percentage() << "%" << std::endl;
    }
    
    // Generate comprehensive memory report
    std::cout << "\nMemory Report:" << std::endl;
    std::cout << registry->generate_memory_report() << std::endl;
}
```

## Advanced Topics

### Custom System Implementation

**Creating Educational Systems**:
```cpp
#include "ecs/system.hpp"

// Example: Movement system with educational instrumentation
class MovementSystem : public ecs::System {
public:
    MovementSystem(ecs::Registry& registry) 
        : System(registry, "Movement_System") {}
    
    void update(f32 delta_time) override {
        ScopeTimer timer("MovementSystem::update");
        
        // Process all entities with Transform and Velocity
        registry_.for_each<Transform, Velocity>([delta_time](Entity e, Transform& t, Velocity& v) {
            // Update position based on velocity
            t.position += v.velocity * delta_time;
            
            // Educational: Log significant movements for analysis
            if (v.velocity.magnitude() > 100.0f) {
                LOG_DEBUG("Fast entity {} at position ({}, {}), velocity ({}, {})",
                         e, t.position.x, t.position.y, v.velocity.x, v.velocity.y);
            }
        });
        
        // Update system statistics for educational analysis
        auto stats = get_system_statistics();
        stats.entities_processed = registry_.get_entities_with<Transform, Velocity>().size();
        update_system_statistics(stats);
    }
};
```

### Memory Optimization Techniques

**Progressive Optimization Tutorial**:
1. **Baseline Measurement**: Start with standard allocation
2. **Arena Optimization**: Switch to arena allocators for component storage
3. **Access Pattern Optimization**: Modify iteration patterns for cache efficiency
4. **SIMD Optimization**: Vectorize mathematical operations
5. **Memory Layout Optimization**: Organize hot/cold data separation

### Research and Extension Opportunities

**Potential Research Projects**:
- Custom allocator strategy development
- Advanced ECS query optimization
- Novel physics algorithm implementation
- GPU-accelerated ECS processing
- Educational visualization technique development

## Next Steps

### Beginner Next Steps
1. Complete all phase tutorials in order
2. Experiment with different allocator configurations
3. Try modifying component structures and see performance impact
4. Explore the interactive UI tools for real-time analysis

### Intermediate Next Steps
1. Implement a custom component and system
2. Create performance benchmarks for your implementations
3. Experiment with memory allocation strategies
4. Contribute improvements to existing systems

### Advanced Next Steps
1. Research and implement novel ECS optimization techniques
2. Develop new educational visualization tools
3. Create comprehensive performance analysis studies
4. Extend the system with additional physics or rendering features

### Contributing to ECScope

See [Contributing Guide](CONTRIBUTING.md) for:
- Development setup and guidelines
- Code style and documentation standards
- How to submit educational improvements
- Research collaboration opportunities

## Learning Resources

### Recommended Reading
- **"Data-Oriented Design"** by Richard Fabian
- **"Game Programming Patterns"** by Robert Nystrom  
- **"Real-Time Collision Detection"** by Christer Ericson
- **"Game Engine Architecture"** by Jason Gregory

### Online Resources
- **ECScope Documentation**: Complete reference in `docs/` directory
- **Interactive Examples**: Progressive tutorials in `examples/` directory
- **Performance Labs**: Built-in benchmarking and analysis tools
- **Community**: GitHub discussions and educational collaboration

---

**Ready to start your journey? Begin with the console performance laboratory and work your way through each phase at your own pace.**

*ECScope: Making complex systems understandable through hands-on experimentation and visual analysis.*