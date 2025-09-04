# ECScope Examples Guide

**Complete walkthrough of all educational examples, tutorials, and interactive demonstrations**

## Table of Contents

1. [Examples Overview](#examples-overview)
2. [Core ECS Examples](#core-ecs-examples)
3. [Memory Management Examples](#memory-management-examples)
4. [Physics System Examples](#physics-system-examples)
5. [Rendering System Examples](#rendering-system-examples)
6. [Performance Laboratory](#performance-laboratory)
7. [Educational Tutorials](#educational-tutorials)
8. [Running Examples](#running-examples)

## Examples Overview

ECScope provides a comprehensive suite of examples designed for progressive learning. Each example demonstrates specific concepts while building understanding of the complete system.

### Learning Progression

**Beginner Level** (Understanding Basics):
1. Basic ECS concepts and component creation
2. Simple physics simulation
3. Basic 2D rendering
4. Memory allocation basics

**Intermediate Level** (Applying Concepts):
1. Custom allocator implementation
2. Advanced ECS patterns
3. Physics optimization techniques
4. Rendering pipeline optimization

**Advanced Level** (Mastering Performance):
1. Memory access pattern optimization
2. Cache behavior analysis
3. Multi-threaded system design
4. GPU programming optimization

### Example Categories

```
examples/
├── Core Examples
│   ├── memory_integration_demo.cpp      # Memory allocator comparison
│   └── performance_laboratory.cpp       # Interactive memory lab
├── Physics Examples
│   ├── physics_demo.cpp                 # Basic physics simulation
│   ├── physics_math_demo.cpp            # Mathematical foundations
│   ├── physics_components_demo.cpp      # Component interaction
│   ├── physics_debug_integration_tests.cpp # Debug visualization
│   ├── physics_debug_performance_benchmark.cpp # Performance analysis
│   └── physics_debug_rendering_demo.cpp # Visual debugging
├── Rendering Examples
│   ├── rendering_2d_demo.cpp            # Complete rendering demo
│   ├── rendering_benchmarks.cpp         # Performance benchmarks
│   └── rendering_physics_integration_demo.cpp # Physics + rendering
└── Rendering Tutorials (Progressive Learning)
    ├── 01_basic_sprite_rendering.cpp    # Foundation concepts
    ├── 02_batching_performance.cpp      # Batching optimization
    ├── 02_sprite_batching_fundamentals.cpp # Batching theory
    ├── 03_advanced_cameras.cpp          # Camera system mastery
    ├── 03_camera_systems_and_viewports.cpp # Viewport management
    ├── 04_shader_programming_and_materials.cpp # GPU programming
    ├── 05_texture_atlasing_and_optimization.cpp # Memory optimization
    ├── 06_particle_systems_and_effects.cpp # Advanced rendering
    └── 07_multi_layer_rendering_and_depth.cpp # Complex scenes
```

## Core ECS Examples

### 1. Memory Integration Demo (`memory_integration_demo.cpp`)

**Purpose**: Demonstrate the performance impact of different memory allocation strategies in ECS systems.

**Key Learning Objectives**:
- Understand SoA vs AoS performance differences
- Compare custom allocators vs standard allocation
- Analyze cache behavior in ECS operations
- Measure memory access patterns

**Code Structure**:
```cpp
int main() {
    // Setup different allocator configurations
    auto standard_config = AllocatorConfig::create_standard();
    auto optimized_config = AllocatorConfig::create_performance_optimized();
    auto educational_config = AllocatorConfig::create_educational_focused();
    
    // Run comparative benchmarks
    run_entity_creation_benchmark(standard_config, optimized_config);
    run_component_access_benchmark(standard_config, optimized_config);
    run_system_update_benchmark(standard_config, optimized_config);
    
    // Display results with educational insights
    display_performance_comparison();
    display_memory_usage_analysis();
    display_cache_behavior_analysis();
    
    return 0;
}
```

**Expected Output**:
```
=== ECScope Memory Integration Demo ===

Entity Creation Benchmark:
  Standard Allocation:  10000 entities in 15.23ms
  Optimized Allocation: 10000 entities in  8.42ms
  Performance Gain: 1.81x faster (44.7% improvement)

Component Access Benchmark:
  Standard Layout (AoS): 1000 accesses in 2.15ms
  Optimized Layout (SoA): 1000 accesses in 1.42ms
  Performance Gain: 1.51x faster (33.9% improvement)

Memory Usage Analysis:
  Standard: 0.89MB total, 64% cache efficiency
  Optimized: 0.53MB total, 89% cache efficiency
  Memory Savings: 40.4% reduction

Educational Insights:
• SoA layout improves cache efficiency by 39%
• Arena allocation reduces memory fragmentation
• Sequential access patterns are 1.5x faster than random access
```

**Educational Value**:
- Concrete performance numbers demonstrate theoretical concepts
- Side-by-side comparison makes optimization benefits clear
- Real memory usage data supports learning about efficiency

### 2. Performance Laboratory (`performance_laboratory.cpp`)

**Purpose**: Interactive environment for exploring memory allocation patterns and performance optimization techniques.

**Key Features**:
- Real-time parameter modification
- Live performance visualization
- Memory pattern analysis
- Educational recommendations

**Code Highlights**:
```cpp
class InteractiveMemoryLab {
    struct ExperimentConfig {
        u32 entity_count{1000};
        bool use_arena_allocator{true};
        bool use_pool_allocator{true};
        bool enable_soa_layout{true};
        bool enable_prefetching{false};
        f32 access_pattern_randomness{0.0f}; // 0=sequential, 1=random
    };
    
    ExperimentConfig config_;
    Registry registry_;
    std::unique_ptr<MemoryProfiler> profiler_;
    
public:
    void run_interactive_session() {
        while (true) {
            render_configuration_ui();
            
            if (ImGui::Button("Run Experiment")) {
                run_current_experiment();
            }
            
            render_results_visualization();
            render_educational_insights();
            
            if (should_exit()) break;
        }
    }
    
private:
    void render_configuration_ui() {
        ImGui::SliderInt("Entity Count", reinterpret_cast<int*>(&config_.entity_count), 
                        100, 50000);
        ImGui::Checkbox("Arena Allocator", &config_.use_arena_allocator);
        ImGui::Checkbox("Pool Allocator", &config_.use_pool_allocator);
        ImGui::Checkbox("SoA Layout", &config_.enable_soa_layout);
        ImGui::SliderFloat("Access Randomness", &config_.access_pattern_randomness, 
                          0.0f, 1.0f);
        
        // Real-time prediction
        const auto predicted_performance = predict_performance(config_);
        ImGui::Text("Predicted Performance:");
        ImGui::Text("  Entity Creation: %.2f ms", predicted_performance.entity_creation_time);
        ImGui::Text("  Memory Usage: %.2f MB", predicted_performance.memory_usage_mb);
        ImGui::Text("  Cache Efficiency: %.1f%%", predicted_performance.cache_efficiency * 100.0f);
    }
    
    void run_current_experiment() {
        // Configure registry based on current settings
        setup_registry_with_config(registry_, config_);
        
        // Run benchmark suite
        profiler_->begin_experiment("Interactive_Lab");
        
        const auto creation_time = measure_entity_creation();
        const auto access_time = measure_component_access();
        const auto memory_usage = measure_memory_consumption();
        const auto cache_behavior = analyze_cache_behavior();
        
        profiler_->end_experiment();
        
        // Store results for visualization
        store_experiment_results(creation_time, access_time, memory_usage, cache_behavior);
    }
    
    void render_educational_insights() {
        ImGui::Separator();
        ImGui::Text("Educational Insights:");
        
        const auto& latest_results = get_latest_results();
        
        if (latest_results.cache_efficiency < 0.6f) {
            ImGui::TextColored({1,0.5f,0,1}, 
                "• Low cache efficiency detected!");
            ImGui::Text("  Try enabling SoA layout for better cache utilization");
            ImGui::Text("  Consider reducing access pattern randomness");
        }
        
        if (latest_results.memory_usage_mb > 1.0f) {
            ImGui::TextColored({1,0.5f,0,1}, 
                "• High memory usage detected!");
            ImGui::Text("  Arena allocator can reduce fragmentation");
            ImGui::Text("  Pool allocator optimizes fixed-size allocations");
        }
        
        if (latest_results.entity_creation_time < 5.0f && latest_results.cache_efficiency > 0.8f) {
            ImGui::TextColored({0,1,0,1}, 
                "• Excellent performance configuration!");
            ImGui::Text("  This setup demonstrates production-quality optimization");
        }
        
        // Educational recommendations
        ImGui::Separator();
        ImGui::Text("Try These Experiments:");
        ImGui::BulletText("Disable SoA layout and observe cache efficiency drop");
        ImGui::BulletText("Increase access randomness to see cache misses");
        ImGui::BulletText("Compare arena vs standard allocation patterns");
        ImGui::BulletText("Test scalability with different entity counts");
    }
};
```

## Memory Management Examples

### Advanced Allocator Comparison

**Custom Arena Allocator Implementation**:
```cpp
// From memory_integration_demo.cpp
void demonstrate_arena_allocator_benefits() {
    constexpr usize ENTITY_COUNT = 10000;
    constexpr usize COMPONENT_SIZE = sizeof(Transform);
    
    log_info("=== Arena Allocator Demonstration ===");
    
    // Standard allocation approach
    {
        Timer timer("Standard Allocation");
        std::vector<std::unique_ptr<Transform>> transforms;
        transforms.reserve(ENTITY_COUNT);
        
        for (usize i = 0; i < ENTITY_COUNT; ++i) {
            transforms.push_back(std::make_unique<Transform>());
            transforms.back()->position = Vec2{
                static_cast<f32>(i % 100), 
                static_cast<f32>(i / 100)
            };
        }
        
        // Measure fragmentation
        const usize theoretical_memory = ENTITY_COUNT * COMPONENT_SIZE;
        const usize actual_memory = measure_heap_usage();
        const f32 fragmentation = (static_cast<f32>(actual_memory) / 
                                 static_cast<f32>(theoretical_memory) - 1.0f) * 100.0f;
        
        log_info("Standard allocation results:");
        log_info("  Theoretical memory: {} bytes", theoretical_memory);
        log_info("  Actual memory: {} bytes", actual_memory);
        log_info("  Fragmentation overhead: {:.1f}%", fragmentation);
    }
    
    // Arena allocation approach
    {
        Timer timer("Arena Allocation");
        
        ArenaAllocator arena{ENTITY_COUNT * COMPONENT_SIZE + 1024}; // Small overhead
        std::vector<Transform*> transforms;
        transforms.reserve(ENTITY_COUNT);
        
        for (usize i = 0; i < ENTITY_COUNT; ++i) {
            auto* transform = arena.allocate<Transform>();
            new (transform) Transform{}; // Placement new
            transform->position = Vec2{
                static_cast<f32>(i % 100), 
                static_cast<f32>(i / 100)
            };
            transforms.push_back(transform);
        }
        
        // Measure efficiency
        const usize arena_used = arena.get_used_bytes();
        const usize arena_total = arena.get_total_bytes();
        const f32 utilization = (static_cast<f32>(arena_used) / 
                               static_cast<f32>(arena_total)) * 100.0f;
        
        log_info("Arena allocation results:");
        log_info("  Arena size: {} bytes", arena_total);
        log_info("  Arena used: {} bytes", arena_used);
        log_info("  Utilization: {:.1f}%", utilization);
        log_info("  Zero fragmentation guaranteed");
        
        // Cache behavior test
        Timer cache_timer("Cache Behavior Test");
        volatile f32 sum = 0.0f;
        for (const auto* transform : transforms) {
            sum += transform->position.x + transform->position.y;
        }
        
        log_info("Sequential access time: {:.3f}ms for {} components", 
                cache_timer.elapsed_ms(), ENTITY_COUNT);
    }
    
    log_info("=== Educational Insight ===");
    log_info("Arena allocation provides:");
    log_info("• Zero fragmentation");
    log_info("• Predictable memory layout");
    log_info("• Cache-friendly access patterns");
    log_info("• Faster allocation/deallocation");
}
```

### Memory Pattern Analysis

```cpp
void demonstrate_memory_access_patterns() {
    log_info("=== Memory Access Pattern Analysis ===");
    
    constexpr usize ENTITY_COUNT = 1000;
    Registry registry{};
    
    // Create entities with components
    std::vector<EntityID> entities;
    for (usize i = 0; i < ENTITY_COUNT; ++i) {
        const EntityID entity = registry.create_entity();
        registry.add_component<Transform>(entity, Transform{
            .position = Vec2{static_cast<f32>(i), static_cast<f32>(i * 2)},
            .rotation = static_cast<f32>(i) * 0.1f,
            .scale = Vec2{1.0f, 1.0f}
        });
        registry.add_component<Velocity>(entity, Velocity{
            .velocity = Vec2{static_cast<f32>(i % 10), static_cast<f32>(i % 7)},
            .angular_velocity = static_cast<f32>(i) * 0.01f
        });
        entities.push_back(entity);
    }
    
    // Test 1: Sequential access (cache-friendly)
    {
        CacheProfiler cache_profiler{"Sequential Access"};
        Timer timer("Sequential Access");
        
        auto view = registry.view<Transform, Velocity>();
        for (auto [entity, transform, velocity] : view.each()) {
            transform.position += velocity.velocity * 0.016f;
            transform.rotation += velocity.angular_velocity * 0.016f;
        }
        
        const auto cache_stats = cache_profiler.get_statistics();
        log_info("Sequential access results:");
        log_info("  Time: {:.3f}ms", timer.elapsed_ms());
        log_info("  Cache hit ratio: {:.1f}%", cache_stats.hit_ratio * 100.0f);
        log_info("  Cache lines accessed: {}", cache_stats.cache_lines_touched);
    }
    
    // Test 2: Random access (cache-hostile)
    {
        CacheProfiler cache_profiler{"Random Access"};
        Timer timer("Random Access");
        
        // Shuffle entity access order
        std::vector<EntityID> shuffled_entities = entities;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::shuffle(shuffled_entities.begin(), shuffled_entities.end(), gen);
        
        for (const EntityID entity : shuffled_entities) {
            auto* transform = registry.get_component<Transform>(entity);
            auto* velocity = registry.get_component<Velocity>(entity);
            if (transform && velocity) {
                transform->position += velocity->velocity * 0.016f;
                transform->rotation += velocity->angular_velocity * 0.016f;
            }
        }
        
        const auto cache_stats = cache_profiler.get_statistics();
        log_info("Random access results:");
        log_info("  Time: {:.3f}ms", timer.elapsed_ms());
        log_info("  Cache hit ratio: {:.1f}%", cache_stats.hit_ratio * 100.0f);
        log_info("  Cache lines accessed: {}", cache_stats.cache_lines_touched);
    }
    
    log_info("=== Educational Insight ===");
    log_info("Memory access patterns dramatically impact performance:");
    log_info("• Sequential access utilizes CPU cache effectively");
    log_info("• Random access causes cache misses and memory stalls");
    log_info("• SoA layout in ECScope enables sequential access patterns");
}
```

## Physics System Examples

### 1. Basic Physics Demo (`physics_demo.cpp`)

**Purpose**: Introduction to 2D physics concepts with visual demonstration.

**Key Learning Objectives**:
- Understand basic physics simulation loop
- Learn about collision detection
- Explore constraint solving
- Analyze performance scaling

**Code Structure**:
```cpp
class PhysicsDemo {
    Registry registry_;
    PhysicsWorld physics_world_;
    RenderSystem render_system_;
    
    // Demo scenarios
    std::vector<std::function<void()>> demo_scenarios_;
    usize current_scenario_{0};
    
public:
    void initialize() {
        setup_demo_scenarios();
        load_initial_scenario();
    }
    
    void update(f32 delta_time) {
        // Update physics
        physics_world_.update(registry_, delta_time);
        
        // Update rendering
        render_system_.update(registry_, delta_time);
        
        // Handle scenario switching
        handle_input();
        
        // Display educational information
        render_physics_education_ui();
    }
    
private:
    void setup_demo_scenarios() {
        demo_scenarios_ = {
            [this]() { create_bouncing_balls_scenario(); },
            [this]() { create_collision_types_scenario(); },
            [this]() { create_constraint_demo_scenario(); },
            [this]() { create_performance_stress_test(); }
        };
    }
    
    void create_bouncing_balls_scenario() {
        // Clear existing entities
        registry_.clear();
        
        // Create ground
        const EntityID ground = registry_.create_entity();
        registry_.add_component<Transform>(ground, Transform{
            .position = Vec2{0.0f, -5.0f},
            .scale = Vec2{20.0f, 1.0f}
        });
        registry_.add_component<BoxCollider>(ground, BoxCollider{Vec2{10.0f, 0.5f}});
        registry_.add_component<RigidBody>(ground, RigidBody{
            .mass = 0.0f, // Infinite mass (static)
            .is_static = true,
            .restitution = 0.8f
        });
        
        // Create bouncing balls with different properties
        for (u32 i = 0; i < 10; ++i) {
            const EntityID ball = registry_.create_entity();
            
            const f32 x_pos = static_cast<f32>(i) * 2.0f - 9.0f;
            const f32 y_pos = 10.0f + static_cast<f32>(i) * 2.0f;
            const f32 restitution = 0.1f + static_cast<f32>(i) * 0.08f; // Varying bounciness
            
            registry_.add_component<Transform>(ball, Transform{
                .position = Vec2{x_pos, y_pos}
            });
            registry_.add_component<CircleCollider>(ball, CircleCollider{0.5f});
            registry_.add_component<RigidBody>(ball, RigidBody{
                .mass = 1.0f,
                .restitution = restitution,
                .friction = 0.3f
            });
            registry_.add_component<SpriteComponent>(ball, SpriteComponent{
                .color = Color{restitution, 0.5f, 1.0f - restitution, 1.0f},
                .size = Vec2{1.0f, 1.0f}
            });
        }
        
        log_info("Created bouncing balls scenario:");
        log_info("• 10 balls with varying restitution (0.1 to 0.8)");
        log_info("• Static ground with high restitution");
        log_info("• Observe energy conservation and material properties");
    }
    
    void render_physics_education_ui() {
        if (ImGui::Begin("Physics Education")) {
            ImGui::Text("Current Scenario: %zu/%zu", current_scenario_ + 1, demo_scenarios_.size());
            
            if (ImGui::Button("Previous") && current_scenario_ > 0) {
                --current_scenario_;
                demo_scenarios_[current_scenario_]();
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Next") && current_scenario_ < demo_scenarios_.size() - 1) {
                ++current_scenario_;
                demo_scenarios_[current_scenario_]();
            }
            
            ImGui::Separator();
            
            // Educational content based on current scenario
            render_scenario_education();
            
            // Physics parameters
            ImGui::Separator();
            ImGui::Text("Physics Parameters:");
            Vec2 gravity = physics_world_.get_gravity();
            if (ImGui::SliderFloat2("Gravity", &gravity.x, -20.0f, 20.0f)) {
                physics_world_.set_gravity(gravity);
            }
            
            u32 iterations = physics_world_.get_constraint_iterations();
            if (ImGui::SliderInt("Constraint Iterations", reinterpret_cast<int*>(&iterations), 1, 20)) {
                physics_world_.set_constraint_iterations(iterations);
            }
            
            // Performance display
            ImGui::Separator();
            const auto physics_stats = physics_world_.get_statistics();
            ImGui::Text("Performance:");
            ImGui::Text("  Bodies: %u", physics_stats.active_bodies);
            ImGui::Text("  Constraints: %u", physics_stats.active_constraints);
            ImGui::Text("  Physics Time: %.3f ms", physics_stats.last_update_time_ms);
            ImGui::Text("  Collision Pairs: %u", physics_stats.collision_pairs_tested);
        }
        ImGui::End();
    }
    
    void render_scenario_education() {
        switch (current_scenario_) {
            case 0: // Bouncing balls
                ImGui::Text("Learning Focus: Material Properties");
                ImGui::Text("Concepts:");
                ImGui::BulletText("Coefficient of restitution affects bounciness");
                ImGui::BulletText("Energy conservation in elastic collisions");
                ImGui::BulletText("Friction affects sliding and rolling");
                ImGui::Text("Observe how different colored balls bounce differently!");
                break;
                
            case 1: // Collision types
                ImGui::Text("Learning Focus: Collision Detection");
                ImGui::Text("Concepts:");
                ImGui::BulletText("Circle-circle collision detection");
                ImGui::BulletText("Box-box collision using SAT");
                ImGui::BulletText("Circle-box hybrid detection");
                ImGui::Text("Watch the debug visualization show detection algorithms!");
                break;
                
            case 2: // Constraints
                ImGui::Text("Learning Focus: Constraint Solving");
                ImGui::Text("Concepts:");
                ImGui::BulletText("Distance joints maintain fixed distances");
                ImGui::BulletText("Revolute joints allow rotation around a point");
                ImGui::BulletText("Sequential impulse method for stability");
                ImGui::Text("See how joints maintain physical relationships!");
                break;
                
            case 3: // Performance
                ImGui::Text("Learning Focus: Performance Optimization");
                ImGui::Text("Concepts:");
                ImGui::BulletText("Spatial partitioning reduces collision tests");
                ImGui::BulletText("Broad-phase vs narrow-phase optimization");
                ImGui::BulletText("Integration method stability vs performance");
                ImGui::Text("Monitor performance with 1000+ dynamic bodies!");
                break;
        }
    }
};
```

### 2. Physics Math Demo (`physics_math_demo.cpp`)

**Purpose**: Deep dive into the mathematical foundations of 2D physics.

**Educational Content**:
```cpp
void demonstrate_vector_mathematics() {
    log_info("=== Vector Mathematics in Physics ===");
    
    // Basic vector operations
    const Vec2 position{3.0f, 4.0f};
    const Vec2 velocity{-2.0f, 1.0f};
    const f32 delta_time = 1.0f / 60.0f;
    
    log_info("Initial state:");
    log_info("  Position: ({:.2f}, {:.2f})", position.x, position.y);
    log_info("  Velocity: ({:.2f}, {:.2f})", velocity.x, velocity.y);
    log_info("  Speed: {:.2f} m/s", velocity.magnitude());
    
    // Integration demonstration
    const Vec2 new_position = position + velocity * delta_time;
    log_info("After integration (dt = {:.4f}s):", delta_time);
    log_info("  New Position: ({:.2f}, {:.2f})", new_position.x, new_position.y);
    
    // Dot product applications
    const Vec2 surface_normal{0.0f, 1.0f}; // Upward normal
    const f32 velocity_normal = Vec2::dot(velocity, surface_normal);
    const Vec2 velocity_tangent = velocity - surface_normal * velocity_normal;
    
    log_info("Collision analysis:");
    log_info("  Surface normal: ({:.2f}, {:.2f})", surface_normal.x, surface_normal.y);
    log_info("  Normal velocity: {:.2f} m/s", velocity_normal);
    log_info("  Tangent velocity: ({:.2f}, {:.2f})", velocity_tangent.x, velocity_tangent.y);
    
    // Reflection demonstration
    const f32 restitution = 0.8f;
    const Vec2 reflected_velocity = velocity - surface_normal * (1.0f + restitution) * velocity_normal;
    
    log_info("After collision (restitution = {:.1f}):", restitution);
    log_info("  Reflected velocity: ({:.2f}, {:.2f})", reflected_velocity.x, reflected_velocity.y);
    log_info("  Energy retained: {:.1f}%", (reflected_velocity.magnitude_squared() / 
                                           velocity.magnitude_squared()) * 100.0f);
}

void demonstrate_collision_detection_math() {
    log_info("=== Collision Detection Mathematics ===");
    
    // Circle-circle collision
    const Vec2 center_a{0.0f, 0.0f};
    const f32 radius_a = 2.0f;
    const Vec2 center_b{3.0f, 0.0f};
    const f32 radius_b = 1.5f;
    
    const Vec2 center_to_center = center_b - center_a;
    const f32 distance = center_to_center.magnitude();
    const f32 radius_sum = radius_a + radius_b;
    
    log_info("Circle-Circle collision test:");
    log_info("  Circle A: center({:.1f}, {:.1f}), radius={:.1f}", 
            center_a.x, center_a.y, radius_a);
    log_info("  Circle B: center({:.1f}, {:.1f}), radius={:.1f}", 
            center_b.x, center_b.y, radius_b);
    log_info("  Distance between centers: {:.2f}", distance);
    log_info("  Sum of radii: {:.2f}", radius_sum);
    log_info("  Collision detected: {}", (distance < radius_sum) ? "YES" : "NO");
    
    if (distance < radius_sum) {
        const f32 penetration = radius_sum - distance;
        const Vec2 normal = center_to_center.normalized();
        const Vec2 contact_point = center_a + normal * radius_a;
        
        log_info("  Penetration depth: {:.2f}", penetration);
        log_info("  Collision normal: ({:.3f}, {:.3f})", normal.x, normal.y);
        log_info("  Contact point: ({:.2f}, {:.2f})", contact_point.x, contact_point.y);
    }
    
    // Box-box collision using SAT
    demonstrate_sat_algorithm();
}

void demonstrate_sat_algorithm() {
    log_info("=== Separating Axis Theorem (SAT) ===");
    
    // Define two boxes
    const std::array<Vec2, 4> box_a_vertices = {
        Vec2{-1.0f, -1.0f}, Vec2{1.0f, -1.0f}, 
        Vec2{1.0f, 1.0f}, Vec2{-1.0f, 1.0f}
    };
    
    const std::array<Vec2, 4> box_b_vertices = {
        Vec2{0.5f, -0.5f}, Vec2{2.5f, -0.5f}, 
        Vec2{2.5f, 1.5f}, Vec2{0.5f, 1.5f}
    };
    
    // Get potential separating axes (normals of edges)
    std::vector<Vec2> axes;
    
    // Box A axes
    for (usize i = 0; i < 4; ++i) {
        const Vec2 edge = box_a_vertices[(i + 1) % 4] - box_a_vertices[i];
        axes.push_back(Vec2{-edge.y, edge.x}.normalized()); // Perpendicular to edge
    }
    
    // Box B axes (in this case same as Box A since both are axis-aligned)
    
    log_info("Testing separating axes:");
    
    f32 minimum_overlap = std::numeric_limits<f32>::max();
    Vec2 separation_axis{};
    bool is_separated = false;
    
    for (usize axis_index = 0; axis_index < axes.size(); ++axis_index) {
        const Vec2& axis = axes[axis_index];
        
        // Project Box A onto axis
        f32 min_a = std::numeric_limits<f32>::max();
        f32 max_a = std::numeric_limits<f32>::lowest();
        for (const Vec2& vertex : box_a_vertices) {
            const f32 projection = Vec2::dot(vertex, axis);
            min_a = std::min(min_a, projection);
            max_a = std::max(max_a, projection);
        }
        
        // Project Box B onto axis
        f32 min_b = std::numeric_limits<f32>::max();
        f32 max_b = std::numeric_limits<f32>::lowest();
        for (const Vec2& vertex : box_b_vertices) {
            const f32 projection = Vec2::dot(vertex, axis);
            min_b = std::min(min_b, projection);
            max_b = std::max(max_b, projection);
        }
        
        log_info("  Axis {}: ({:.3f}, {:.3f})", axis_index, axis.x, axis.y);
        log_info("    Box A projection: [{:.2f}, {:.2f}]", min_a, max_a);
        log_info("    Box B projection: [{:.2f}, {:.2f}]", min_b, max_b);
        
        // Check for separation
        if (max_a < min_b || max_b < min_a) {
            log_info("    SEPARATED on this axis!");
            is_separated = true;
            break;
        }
        
        // Calculate overlap
        const f32 overlap = std::min(max_a, max_b) - std::max(min_a, min_b);
        log_info("    Overlap: {:.2f}", overlap);
        
        if (overlap < minimum_overlap) {
            minimum_overlap = overlap;
            separation_axis = axis;
        }
    }
    
    if (is_separated) {
        log_info("SAT Result: NO COLLISION (shapes are separated)");
    } else {
        log_info("SAT Result: COLLISION DETECTED");
        log_info("  Minimum overlap: {:.2f}", minimum_overlap);
        log_info("  Separation axis: ({:.3f}, {:.3f})", separation_axis.x, separation_axis.y);
        log_info("  Penetration vector: ({:.2f}, {:.2f})", 
                separation_axis.x * minimum_overlap, 
                separation_axis.y * minimum_overlap);
    }
}
```

### 3. Physics Debug Integration (`physics_debug_integration_tests.cpp`)

**Purpose**: Comprehensive testing and visualization of physics debugging tools.

**Features**:
- Step-by-step collision detection visualization
- Constraint solving algorithm demonstration
- Performance profiling integration
- Interactive parameter modification

## Rendering System Examples

### 1. Rendering Tutorials (Progressive Learning)

#### Tutorial 1: Basic Sprite Rendering (`01_basic_sprite_rendering.cpp`)

**Purpose**: Introduction to modern 2D rendering concepts.

**Learning Objectives**:
- Understand OpenGL context setup
- Learn about shaders and VAOs
- Practice basic sprite rendering
- Understand coordinate systems

**Code Structure**:
```cpp
class BasicSpriteRenderer {
    OpenGLContext gl_context_;
    std::unique_ptr<Shader> sprite_shader_;
    GLuint quad_vao_, quad_vbo_, quad_ebo_;
    GLuint texture_id_;
    
public:
    bool initialize(u32 window_width, u32 window_height) {
        // Initialize OpenGL context
        if (!gl_context_.initialize(window_width, window_height, "Tutorial 1: Basic Sprites")) {
            return false;
        }
        
        // Create and compile sprite shader
        sprite_shader_ = std::make_unique<Shader>();
        if (!sprite_shader_->compile_from_source(basic_vertex_shader, basic_fragment_shader)) {
            log_error("Failed to compile basic sprite shader");
            return false;
        }
        
        // Setup quad geometry
        setup_quad_geometry();
        
        // Load test texture
        load_test_texture();
        
        log_info("Basic sprite renderer initialized successfully");
        return true;
    }
    
    void render_frame() {
        // Clear screen
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Use shader
        sprite_shader_->use();
        
        // Set up view-projection matrix (orthographic 2D)
        const Mat4 projection = Mat4::orthographic(
            -10.0f, 10.0f,  // left, right
            -7.5f, 7.5f,    // bottom, top
            -1.0f, 1.0f     // near, far
        );
        sprite_shader_->set_matrix4("u_projection", projection);
        
        // Render multiple sprites at different positions
        const std::array<Vec2, 5> sprite_positions = {
            Vec2{-6.0f, 0.0f}, Vec2{-3.0f, 2.0f}, Vec2{0.0f, 0.0f}, 
            Vec2{3.0f, -2.0f}, Vec2{6.0f, 1.0f}
        };
        
        for (usize i = 0; i < sprite_positions.size(); ++i) {
            const Vec2& pos = sprite_positions[i];
            const f32 scale = 1.0f + static_cast<f32>(i) * 0.2f;
            const f32 rotation = static_cast<f32>(i) * 45.0f * (M_PI / 180.0f);
            
            // Create model matrix
            const Mat4 model = Mat4::translation(Vec3{pos.x, pos.y, 0.0f}) *
                              Mat4::rotation_z(rotation) *
                              Mat4::scale(Vec3{scale, scale, 1.0f});
            
            sprite_shader_->set_matrix4("u_model", model);
            
            // Set sprite color
            const Vec4 color{
                static_cast<f32>(i) / 4.0f,
                1.0f - static_cast<f32>(i) / 4.0f,
                0.5f,
                1.0f
            };
            sprite_shader_->set_vec4("u_color", color);
            
            // Bind texture and render
            glBindTexture(GL_TEXTURE_2D, texture_id_);
            glBindVertexArray(quad_vao_);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }
        
        // Swap buffers
        gl_context_.swap_buffers();
    }
    
private:
    void setup_quad_geometry() {
        // Quad vertices (position + texture coordinates)
        const f32 vertices[] = {
            // Positions    // Texture Coords
            -0.5f, -0.5f,   0.0f, 0.0f,
             0.5f, -0.5f,   1.0f, 0.0f,
             0.5f,  0.5f,   1.0f, 1.0f,
            -0.5f,  0.5f,   0.0f, 1.0f
        };
        
        const u32 indices[] = {
            0, 1, 2,  // First triangle
            2, 3, 0   // Second triangle
        };
        
        // Generate and bind VAO
        glGenVertexArrays(1, &quad_vao_);
        glBindVertexArray(quad_vao_);
        
        // Generate and setup VBO
        glGenBuffers(1, &quad_vbo_);
        glBindBuffer(GL_ARRAY_BUFFER, quad_vbo_);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        // Generate and setup EBO
        glGenBuffers(1, &quad_ebo_);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad_ebo_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        
        // Position attribute
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void*)0);
        glEnableVertexAttribArray(0);
        
        // Texture coordinate attribute
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), 
                             (void*)(2 * sizeof(f32)));
        glEnableVertexAttribArray(1);
        
        // Unbind VAO
        glBindVertexArray(0);
        
        log_info("Quad geometry setup complete:");
        log_info("  4 vertices, 6 indices (2 triangles)");
        log_info("  Position attribute at location 0");
        log_info("  Texture coordinate attribute at location 1");
    }
};
```

#### Tutorial 2: Batching Performance (`02_batching_performance.cpp`)

**Purpose**: Demonstrate the dramatic performance impact of sprite batching.

**Learning Objectives**:
- Understand draw call overhead
- Learn batching implementation
- Measure performance differences
- Optimize for GPU efficiency

**Key Demonstration**:
```cpp
class BatchingPerformanceDemo {
    struct PerformanceComparison {
        f32 individual_draw_time;
        f32 batched_draw_time;
        u32 individual_draw_calls;
        u32 batched_draw_calls;
        f32 performance_improvement;
    };
    
public:
    PerformanceComparison run_comparison(u32 sprite_count) {
        PerformanceComparison results{};
        
        // Test 1: Individual draw calls (inefficient)
        {
            Timer timer("Individual Draws");
            
            for (u32 i = 0; i < sprite_count; ++i) {
                // Bind shader
                sprite_shader_->use();
                
                // Set transform matrix
                const Mat4 model = create_sprite_transform(i);
                sprite_shader_->set_matrix4("u_model", model);
                
                // Bind texture
                glBindTexture(GL_TEXTURE_2D, sprite_texture_);
                
                // Draw single quad
                glBindVertexArray(quad_vao_);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                
                ++results.individual_draw_calls;
            }
            
            results.individual_draw_time = timer.elapsed_ms();
        }
        
        // Test 2: Batched rendering (efficient)
        {
            Timer timer("Batched Draws");
            
            batch_renderer_.begin_batch();
            
            for (u32 i = 0; i < sprite_count; ++i) {
                const Vec2 position = calculate_sprite_position(i);
                const Vec2 size{1.0f, 1.0f};
                const Vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
                
                batch_renderer_.submit_sprite(position, size, sprite_texture_, color);
            }
            
            const auto batch_stats = batch_renderer_.end_batch_and_render();
            
            results.batched_draw_time = timer.elapsed_ms();
            results.batched_draw_calls = batch_stats.total_draw_calls;
        }
        
        // Calculate improvement
        results.performance_improvement = results.individual_draw_time / results.batched_draw_time;
        
        log_info("Batching Performance Comparison ({} sprites):", sprite_count);
        log_info("  Individual Draws: {:.2f}ms ({} draw calls)", 
                results.individual_draw_time, results.individual_draw_calls);
        log_info("  Batched Draws: {:.2f}ms ({} draw calls)", 
                results.batched_draw_time, results.batched_draw_calls);
        log_info("  Performance Improvement: {:.1f}x faster", results.performance_improvement);
        log_info("  Draw Call Reduction: {:.1f}x fewer calls", 
                static_cast<f32>(results.individual_draw_calls) / 
                static_cast<f32>(results.batched_draw_calls));
        
        return results;
    }
    
    void run_scalability_test() {
        log_info("=== Batching Scalability Analysis ===");
        
        const std::vector<u32> sprite_counts = {100, 500, 1000, 2000, 5000, 10000};
        
        for (const u32 count : sprite_counts) {
            const auto results = run_comparison(count);
            
            // Analyze scaling behavior
            const f32 individual_time_per_sprite = results.individual_draw_time / count;
            const f32 batched_time_per_sprite = results.batched_draw_time / count;
            
            log_info("Sprite Count: {} | Individual: {:.4f}ms/sprite | Batched: {:.4f}ms/sprite", 
                    count, individual_time_per_sprite, batched_time_per_sprite);
        }
        
        log_info("=== Educational Insights ===");
        log_info("• Individual draws scale linearly with sprite count");
        log_info("• Batched draws scale sub-linearly due to GPU parallelization");
        log_info("• Draw call overhead becomes dominant at higher sprite counts");
        log_info("• Modern GPUs are optimized for large batch processing");
    }
};
```

#### Tutorial 3: Advanced Cameras (`03_advanced_cameras.cpp`)

**Purpose**: Master camera systems, viewports, and advanced rendering techniques.

**Learning Objectives**:
- Multi-camera rendering setup
- Viewport management
- Camera following and smoothing
- Render target optimization

## Performance Laboratory

### Interactive Memory Experiments

The Performance Laboratory provides hands-on experiments for understanding memory behavior:

**Experiment 1: Allocation Strategy Comparison**
```cpp
void run_allocation_strategy_experiment() {
    const ExperimentConfig config{
        .entity_count = 10000,
        .component_types = {"Transform", "Velocity", "RigidBody"},
        .access_pattern = AccessPattern::Sequential,
        .measurement_duration = 5.0f // seconds
    };
    
    // Test different allocator combinations
    const std::vector<AllocatorConfig> test_configs = {
        AllocatorConfig::create_standard(),
        AllocatorConfig::create_arena_optimized(),
        AllocatorConfig::create_pool_optimized(),
        AllocatorConfig::create_hybrid_optimized()
    };
    
    std::vector<ExperimentResult> results;
    
    for (const auto& allocator_config : test_configs) {
        log_info("Testing allocator configuration: {}", allocator_config.name);
        
        Registry test_registry{allocator_config};
        const auto result = run_benchmark_suite(test_registry, config);
        results.push_back(result);
        
        log_info("Results:");
        log_info("  Entity creation: {:.2f}ms", result.entity_creation_time);
        log_info("  Component access: {:.2f}ms", result.component_access_time);
        log_info("  Memory usage: {:.2f}MB", result.memory_usage_mb);
        log_info("  Cache efficiency: {:.1f}%", result.cache_efficiency * 100.0f);
    }
    
    // Generate comparative analysis
    generate_experiment_report(results);
}
```

**Experiment 2: Cache Behavior Analysis**
```cpp
void run_cache_behavior_experiment() {
    log_info("=== Cache Behavior Analysis Experiment ===");
    
    // Test different access patterns
    const std::vector<AccessPattern> patterns = {
        AccessPattern::Sequential,     // Cache-friendly
        AccessPattern::Random,         // Cache-hostile
        AccessPattern::Strided_2,      // Every other element
        AccessPattern::Strided_4,      // Every 4th element
        AccessPattern::Reverse         // Backward sequential
    };
    
    for (const auto pattern : patterns) {
        const auto cache_stats = measure_cache_behavior_with_pattern(pattern);
        
        log_info("Access Pattern: {}", access_pattern_name(pattern));
        log_info("  Cache hit ratio: {:.1f}%", cache_stats.hit_ratio * 100.0f);
        log_info("  Cache misses: {}", cache_stats.total_misses);
        log_info("  Access time: {:.3f}ms", cache_stats.access_time_ms);
        log_info("  Cache lines touched: {}", cache_stats.cache_lines_touched);
        
        // Educational explanation
        explain_cache_behavior(pattern, cache_stats);
    }
}
```

## Running Examples

### Build and Run Instructions

**Console Examples** (No graphics required):
```bash
# Build core examples
cmake -DECSCOPE_BUILD_EXAMPLES=ON ..
make

# Run memory laboratory
./ecscope_performance_lab_console

# Run physics mathematics demo
./physics_math_demo

# Run memory integration demo
./memory_integration_demo
```

**Graphics Examples** (Requires SDL2 and OpenGL):
```bash
# Build with graphics support
cmake -DECSCOPE_ENABLE_GRAPHICS=ON -DECSCOPE_BUILD_EXAMPLES=ON ..
make

# Run complete rendering demo
./ecscope_rendering_demo

# Run progressive tutorials
./ecscope_tutorial_01  # Basic sprite rendering
./ecscope_tutorial_02  # Batching performance
./ecscope_tutorial_03  # Advanced cameras

# Run performance laboratory (interactive)
./ecscope_performance_laboratory

# Run physics visualization
./physics_debug_rendering_demo
```

### Expected Learning Outcomes

**After Core Examples**:
- Understand ECS architecture principles
- Appreciate memory allocation impact on performance
- Recognize cache-friendly vs cache-hostile patterns
- Implement basic physics simulation

**After Physics Examples**:
- Implement collision detection algorithms
- Understand constraint-based physics solving
- Optimize spatial partitioning systems
- Debug physics systems effectively

**After Rendering Examples**:
- Create modern OpenGL rendering pipelines
- Implement automatic sprite batching
- Optimize GPU memory usage
- Debug rendering performance issues

**After Performance Laboratory**:
- Analyze and optimize memory access patterns
- Implement custom allocators for specific use cases
- Measure and improve cache efficiency
- Design performance-conscious system architectures

### Troubleshooting Common Issues

**Graphics Examples Not Building**:
```bash
# Install SDL2 (Ubuntu/Debian)
sudo apt install libsdl2-dev

# Install SDL2 (macOS)
brew install sdl2

# Install SDL2 (Windows with vcpkg)
vcpkg install sdl2
```

**Performance Numbers Don't Match Documentation**:
- Performance varies significantly between hardware
- Compile with optimizations enabled (`-O2` or `-O3`)
- Disable debugging features for production benchmarks
- Consider thermal throttling on laptops

**Examples Crash or Show Errors**:
- Check OpenGL driver support (3.3+ required)
- Verify all dependencies are installed
- Check console output for detailed error messages
- Enable debug mode for additional diagnostics

### Advanced Experimentation

**Custom Experiment Development**:
```cpp
// Template for creating new experiments
class CustomExperiment {
    std::string name_;
    std::string description_;
    std::vector<Parameter> parameters_;
    
public:
    CustomExperiment(std::string name, std::string description) 
        : name_(std::move(name)), description_(std::move(description)) {}
    
    // Add configurable parameters
    void add_parameter(const std::string& name, f32 min_val, f32 max_val, f32 default_val) {
        parameters_.emplace_back(name, min_val, max_val, default_val);
    }
    
    // Implement experiment logic
    virtual ExperimentResult run_experiment(const ExperimentConfig& config) = 0;
    
    // Provide educational insights
    virtual std::vector<std::string> get_insights(const ExperimentResult& result) = 0;
    
    // Render custom UI
    virtual void render_custom_ui() {}
};

// Example: Custom cache line analysis experiment
class CacheLineAnalysisExperiment : public CustomExperiment {
public:
    CacheLineAnalysisExperiment() 
        : CustomExperiment("Cache Line Analysis", 
                          "Analyze the impact of cache line size on performance") {
        add_parameter("data_stride", 4.0f, 128.0f, 64.0f);
        add_parameter("access_count", 1000.0f, 100000.0f, 10000.0f);
    }
    
    ExperimentResult run_experiment(const ExperimentConfig& config) override {
        // Implement custom cache line analysis
        // Return detailed results
    }
    
    std::vector<std::string> get_insights(const ExperimentResult& result) override {
        return {
            "Cache line size affects memory bandwidth utilization",
            "Aligned access patterns improve performance",
            "False sharing can degrade multi-threaded performance"
        };
    }
};
```

---

**ECScope Examples: Learning through experimentation, understanding through visualization, mastering through practice.**

*Each example builds understanding incrementally, from fundamental concepts to advanced optimization techniques, ensuring comprehensive learning of modern game engine development.*