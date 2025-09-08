# ECScope Comprehensive API Reference

**Complete API documentation for ECScope Educational ECS Engine**

## Table of Contents

1. [Quick Reference](#quick-reference)
2. [Core System API](#core-system-api)
3. [ECS Registry API](#ecs-registry-api)
4. [Memory Management API](#memory-management-api)
5. [Physics System API](#physics-system-api)
6. [Rendering System API](#rendering-system-api)
7. [Shader System API](#shader-system-api)
8. [WebAssembly API](#webassembly-api)
9. [Debug and Profiling API](#debug-and-profiling-api)
10. [Educational Features API](#educational-features-api)

## Quick Reference

### Essential Includes

```cpp
// Core functionality (always needed)
#include <ecscope/ecscope.hpp>

// Or include specific systems
#include <ecscope/registry.hpp>      // ECS Registry
#include <ecscope/memory_tracker.hpp> // Memory management
#include <ecscope/physics.hpp>       // 2D physics
#include <ecscope/renderer_2d.hpp>   // 2D rendering
#include <ecscope/shader.hpp>        // Shader system
```

### Basic Usage Pattern

```cpp
#include <ecscope/ecscope.hpp>
using namespace ecscope;

int main() {
    // Initialize ECScope
    ecscope::initialize();
    
    // Create ECS registry
    auto registry = ecs::create_educational_registry("MyApp");
    
    // Create entities and components
    auto entity = registry->create_entity<Position, Velocity>(
        Position{0.0f, 0.0f}, 
        Velocity{1.0f, 0.0f}
    );
    
    // Update systems
    registry->for_each<Position, Velocity>([](auto& pos, auto& vel) {
        pos.x += vel.x * 0.016f; // 60 FPS update
        pos.y += vel.y * 0.016f;
    });
    
    // Cleanup
    ecscope::shutdown();
    return 0;
}
```

## Core System API

### Types and Constants (`include/ecscope/types.hpp`)

```cpp
namespace ecscope {
    // Basic types
    using i8 = std::int8_t;
    using i16 = std::int16_t;
    using i32 = std::int32_t;
    using i64 = std::int64_t;
    using u8 = std::uint8_t;
    using u16 = std::uint16_t;
    using u32 = std::uint32_t;
    using u64 = std::uint64_t;
    using f32 = float;
    using f64 = double;
    using usize = std::size_t;
    
    // Memory size constants
    constexpr usize KB = 1024;
    constexpr usize MB = 1024 * KB;
    constexpr usize GB = 1024 * MB;
    
    // ECS limits
    constexpr u32 MAX_ENTITIES = 1048576;    // 2^20
    constexpr u32 MAX_COMPONENTS = 64;       // Component bitset limit
}
```

### Entity ID System (`include/ecscope/entity.hpp`)

```cpp
namespace ecscope::ecs {
    struct Entity {
        u32 index;
        u32 generation;
        
        static constexpr Entity INVALID{0, 0};
        
        // Construction
        Entity() : index(0), generation(0) {}
        Entity(u32 idx, u32 gen) : index(idx), generation(gen) {}
        
        // Comparison
        bool operator==(const Entity& other) const;
        bool operator!=(const Entity& other) const;
        bool operator<(const Entity& other) const;
        
        // Validation
        bool is_valid() const { return index != 0 || generation != 0; }
        
        // Debugging
        std::string to_string() const;
        void print_debug_info() const;
    };
}
```

### Logging System (`include/ecscope/log.hpp`)

```cpp
namespace ecscope::log {
    enum class Level { TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL };
    
    // Core logging functions
    void initialize();
    void shutdown();
    void set_level(Level level);
    
    // Logging macros (use these in your code)
    #define LOG_TRACE(...)    // Detailed tracing information
    #define LOG_DEBUG(...)    // Debug information
    #define LOG_INFO(...)     // General information
    #define LOG_WARN(...)     // Warning messages
    #define LOG_ERROR(...)    // Error messages
    #define LOG_CRITICAL(...) // Critical errors
}
```

## ECS Registry API

### Registry Creation and Management

```cpp
namespace ecscope::ecs {
    class Registry {
    public:
        // Construction with different allocation strategies
        static std::unique_ptr<Registry> create_educational_registry(
            const std::string& name = "Educational"
        );
        static std::unique_ptr<Registry> create_performance_registry(
            const std::string& name = "Performance"
        );
        static std::unique_ptr<Registry> create_conservative_registry(
            const std::string& name = "Conservative"
        );
        
        // Entity management
        template<typename... Components>
        Entity create_entity(Components&&... components);
        
        bool destroy_entity(Entity entity);
        bool is_valid(Entity entity) const;
        usize get_entity_count() const;
        
        // Component management
        template<typename Component>
        bool add_component(Entity entity, Component&& component);
        
        template<typename Component>
        bool remove_component(Entity entity);
        
        template<typename Component>
        Component* get_component(Entity entity);
        
        template<typename Component>
        const Component* get_component(Entity entity) const;
        
        template<typename Component>
        bool has_component(Entity entity) const;
        
        // System execution
        template<typename... Components, typename Func>
        void for_each(Func&& func);
        
        template<typename... Components>
        std::vector<Entity> get_entities_with();
        
        // Memory and performance
        struct MemoryStatistics {
            usize active_entities;
            usize total_archetypes;
            usize arena_used_bytes;
            usize arena_total_bytes;
            usize pool_used_entities;
            usize pool_total_entities;
            f64 memory_efficiency;
            f64 cache_hit_ratio;
            
            f64 arena_utilization() const { 
                return static_cast<f64>(arena_used_bytes) / arena_total_bytes; 
            }
            f64 pool_utilization() const {
                return static_cast<f64>(pool_used_entities) / pool_total_entities;
            }
        };
        
        MemoryStatistics get_memory_statistics() const;
        std::string generate_memory_report() const;
        
        void benchmark_allocators(const std::string& test_name, usize entity_count);
        void compact_memory();
        void clear();
    };
}
```

### Component Definition

```cpp
// Define components as simple structs
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
    Health(i32 current = 100, i32 maximum = 100) 
        : current(current), maximum(maximum) {}
};

// Usage
auto entity = registry->create_entity<Position, Velocity>(
    Position{10.0f, 20.0f},
    Velocity{1.0f, -0.5f}
);
```

## Memory Management API

### Memory Tracker (`include/ecscope/memory_tracker.hpp`)

```cpp
namespace ecscope::memory {
    class MemoryTracker {
    public:
        // Global tracking control
        static void initialize();
        static void shutdown();
        static void enable_tracking(bool enabled);
        
        // Allocation tracking
        static void track_allocation(void* ptr, usize size, const char* category);
        static void track_deallocation(void* ptr);
        
        // Statistics
        struct Statistics {
            usize total_allocated;
            usize total_deallocated;
            usize current_usage;
            usize peak_usage;
            usize allocation_count;
            usize deallocation_count;
            f64 average_allocation_size;
        };
        
        static Statistics get_statistics();
        static Statistics get_category_statistics(const char* category);
        
        // Reporting
        static std::string generate_report();
        static void print_summary();
        
        // Leak detection
        static std::vector<LeakInfo> detect_leaks();
        static void dump_leak_report();
    };
}
```

### Custom Allocators

```cpp
namespace ecscope::memory {
    // Arena allocator for fast sequential allocation
    class Arena {
    public:
        Arena(usize size);
        ~Arena();
        
        void* allocate(usize size, usize alignment = alignof(std::max_align_t));
        void deallocate(void* ptr); // No-op for arena
        void reset(); // Reset to beginning
        
        usize get_used_bytes() const;
        usize get_total_bytes() const;
        f64 get_utilization() const;
    };
    
    // Pool allocator for fixed-size objects
    template<typename T>
    class Pool {
    public:
        Pool(usize capacity);
        ~Pool();
        
        T* allocate();
        void deallocate(T* ptr);
        
        usize get_used_count() const;
        usize get_capacity() const;
        bool is_full() const;
    };
}
```

## Physics System API

### 2D Physics World (`include/ecscope/physics.hpp`)

```cpp
namespace ecscope::physics {
    class World2D {
    public:
        World2D();
        ~World2D();
        
        // World configuration
        void set_gravity(f32 x, f32 y);
        void set_damping(f32 linear, f32 angular);
        
        // Body management
        struct BodyDef {
            f32 x, y;           // Position
            f32 rotation;       // Rotation in radians
            f32 linear_damping; // Linear damping factor
            f32 angular_damping; // Angular damping factor
            bool is_static;     // Static body flag
        };
        
        u32 create_body(const BodyDef& def);
        void destroy_body(u32 body_id);
        
        // Shape attachment
        struct CircleShape {
            f32 radius;
            f32 density;
            f32 friction;
            f32 restitution;
        };
        
        struct BoxShape {
            f32 width, height;
            f32 density;
            f32 friction;
            f32 restitution;
        };
        
        void attach_circle(u32 body_id, const CircleShape& shape);
        void attach_box(u32 body_id, const BoxShape& shape);
        
        // Forces and impulses
        void apply_force(u32 body_id, f32 fx, f32 fy);
        void apply_impulse(u32 body_id, f32 jx, f32 jy);
        void apply_torque(u32 body_id, f32 torque);
        
        // Simulation
        void step(f32 dt, u32 velocity_iterations = 8, u32 position_iterations = 3);
        
        // Query
        struct BodyState {
            f32 x, y;           // Position
            f32 rotation;       // Rotation
            f32 vx, vy;         // Linear velocity
            f32 angular_velocity; // Angular velocity
        };
        
        BodyState get_body_state(u32 body_id) const;
        void set_body_state(u32 body_id, const BodyState& state);
        
        // Collision detection
        struct ContactPoint {
            f32 x, y;           // Contact point
            f32 nx, ny;         // Contact normal
            f32 penetration;    // Penetration depth
        };
        
        std::vector<ContactPoint> get_contacts() const;
        
        // Debug visualization
        void debug_draw(class DebugRenderer* renderer);
    };
    
    // Physics components for ECS integration
    struct RigidBody {
        u32 physics_id;
        f32 mass;
        f32 inverse_mass;
        
        RigidBody(f32 mass = 1.0f) : mass(mass), inverse_mass(1.0f / mass) {}
    };
    
    struct Transform2D {
        f32 x, y;
        f32 rotation;
        
        Transform2D(f32 x = 0.0f, f32 y = 0.0f, f32 rot = 0.0f) 
            : x(x), y(y), rotation(rot) {}
    };
}
```

## Rendering System API

### 2D Renderer (`include/ecscope/renderer_2d.hpp`)

```cpp
namespace ecscope::renderer {
    class Renderer2D {
    public:
        Renderer2D();
        ~Renderer2D();
        
        // Initialization
        bool initialize(u32 window_width, u32 window_height);
        void shutdown();
        
        // Frame management
        void begin_frame();
        void end_frame();
        void clear(f32 r, f32 g, f32 b, f32 a = 1.0f);
        
        // Camera
        struct Camera2D {
            f32 x, y;           // Position
            f32 zoom;           // Zoom level
            f32 rotation;       // Rotation in radians
            
            Camera2D(f32 x = 0.0f, f32 y = 0.0f, f32 zoom = 1.0f, f32 rot = 0.0f)
                : x(x), y(y), zoom(zoom), rotation(rot) {}
        };
        
        void set_camera(const Camera2D& camera);
        Camera2D get_camera() const;
        
        // Basic drawing
        void draw_sprite(f32 x, f32 y, f32 width, f32 height, 
                        f32 r = 1.0f, f32 g = 1.0f, f32 b = 1.0f, f32 a = 1.0f);
        
        void draw_sprite_textured(f32 x, f32 y, f32 width, f32 height,
                                 u32 texture_id,
                                 f32 r = 1.0f, f32 g = 1.0f, f32 b = 1.0f, f32 a = 1.0f);
        
        void draw_line(f32 x1, f32 y1, f32 x2, f32 y2,
                      f32 r = 1.0f, f32 g = 1.0f, f32 b = 1.0f, f32 a = 1.0f);
        
        void draw_circle(f32 x, f32 y, f32 radius, u32 segments = 32,
                        f32 r = 1.0f, f32 g = 1.0f, f32 b = 1.0f, f32 a = 1.0f);
        
        void draw_rectangle(f32 x, f32 y, f32 width, f32 height,
                           f32 r = 1.0f, f32 g = 1.0f, f32 b = 1.0f, f32 a = 1.0f);
        
        // Batch rendering
        void begin_batch();
        void end_batch();
        void flush_batch();
        
        // Statistics
        struct RenderStats {
            u32 draw_calls;
            u32 sprites_rendered;
            u32 vertices_rendered;
            u32 indices_rendered;
            f32 frame_time_ms;
        };
        
        RenderStats get_stats() const;
        void reset_stats();
    };
    
    // Rendering components for ECS
    struct Sprite {
        f32 width, height;
        f32 r, g, b, a;         // Color
        u32 texture_id;         // 0 = no texture
        
        Sprite(f32 w = 32.0f, f32 h = 32.0f, f32 r = 1.0f, f32 g = 1.0f, f32 b = 1.0f, f32 a = 1.0f)
            : width(w), height(h), r(r), g(g), b(b), a(a), texture_id(0) {}
    };
}
```

## Shader System API

### Shader Management (`include/ecscope/shader.hpp`)

```cpp
namespace ecscope::renderer {
    class Shader {
    public:
        Shader();
        ~Shader();
        
        // Compilation
        bool compile_from_source(const std::string& vertex_source,
                               const std::string& fragment_source);
        bool compile_from_files(const std::string& vertex_path,
                              const std::string& fragment_path);
        
        // Usage
        void use();
        void unuse();
        
        // Uniform setting
        void set_bool(const std::string& name, bool value);
        void set_int(const std::string& name, i32 value);
        void set_float(const std::string& name, f32 value);
        void set_vec2(const std::string& name, f32 x, f32 y);
        void set_vec3(const std::string& name, f32 x, f32 y, f32 z);
        void set_vec4(const std::string& name, f32 x, f32 y, f32 z, f32 w);
        void set_matrix4(const std::string& name, const f32* matrix);
        
        // Query
        bool is_compiled() const;
        std::string get_compile_log() const;
        u32 get_program_id() const;
        
        // Hot reload support
        void enable_hot_reload(const std::string& vertex_path,
                              const std::string& fragment_path);
        bool check_and_reload();
    };
    
    // Shader manager for efficient resource handling
    class ShaderManager {
    public:
        static ShaderManager& instance();
        
        // Shader loading
        std::shared_ptr<Shader> load_shader(const std::string& name,
                                           const std::string& vertex_path,
                                           const std::string& fragment_path);
        
        std::shared_ptr<Shader> get_shader(const std::string& name);
        void reload_all_shaders();
        
        // Built-in shaders
        std::shared_ptr<Shader> get_default_sprite_shader();
        std::shared_ptr<Shader> get_default_line_shader();
    };
}
```

## WebAssembly API

### WASM Bindings (`src/wasm/bindings/`)

```cpp
// C++ side - automatic binding generation
namespace ecscope::wasm {
    void initialize_bindings();
    
    // ECS bindings
    void bind_ecs_registry();
    void bind_entity_management();
    void bind_component_systems();
    
    // Physics bindings  
    void bind_physics_world();
    void bind_collision_detection();
    
    // Memory tracking bindings
    void bind_memory_tracker();
    void bind_allocator_stats();
}
```

JavaScript side usage:

```javascript
// Initialize ECScope WASM module
const ecscope = await ECScope();

// Create ECS registry
const registry = new ecscope.Registry("WebDemo");

// Create entities
const player = registry.createEntity();
registry.addPosition(player, 100, 100);
registry.addVelocity(player, 1, 0);

// System update
function gameLoop() {
    registry.updateMovementSystem(0.016); // 60 FPS
    
    // Query entities
    const entities = registry.getEntitiesWith("Position", "Velocity");
    entities.forEach(entity => {
        const pos = registry.getPosition(entity);
        console.log(`Entity ${entity}: (${pos.x}, ${pos.y})`);
    });
    
    requestAnimationFrame(gameLoop);
}
gameLoop();
```

## Debug and Profiling API

### Performance Profiler

```cpp
namespace ecscope::profiler {
    class Profiler {
    public:
        static void begin_sample(const std::string& name);
        static void end_sample(const std::string& name);
        
        struct ProfileData {
            std::string name;
            f64 total_time_ms;
            f64 average_time_ms;
            u32 call_count;
        };
        
        static std::vector<ProfileData> get_frame_data();
        static void reset_frame_data();
        
        // Automatic scoped profiling
        class ScopedProfiler {
        public:
            ScopedProfiler(const std::string& name);
            ~ScopedProfiler();
        };
    };
    
    // Macro for easy profiling
    #define PROFILE_SCOPE(name) ecscope::profiler::Profiler::ScopedProfiler _prof(name)
    #define PROFILE_FUNCTION() PROFILE_SCOPE(__FUNCTION__)
}
```

### Memory Debugging

```cpp
namespace ecscope::debug {
    class MemoryDebugger {
    public:
        // Leak detection
        static void enable_leak_detection(bool enabled);
        static std::vector<LeakInfo> get_current_leaks();
        static void print_leak_summary();
        
        // Allocation tracking
        static void track_allocation_source(bool enabled);
        static void dump_allocation_callstacks();
        
        // Heap validation
        static bool validate_heap();
        static void check_memory_corruption();
    };
}
```

## Educational Features API

### Interactive Learning

```cpp
namespace ecscope::educational {
    // Memory allocation demonstrations
    void run_memory_allocation_demo();
    void compare_allocator_performance();
    void visualize_memory_fragmentation();
    
    // ECS architecture tutorials
    void demonstrate_archetype_migration();
    void show_component_storage_layouts();
    void benchmark_query_performance();
    
    // Physics learning modules
    void explain_collision_detection_phases();
    void visualize_constraint_solving();
    void demonstrate_numerical_integration();
    
    // Performance analysis tools
    void run_cache_behavior_analysis();
    void measure_branch_prediction_impact();
    void profile_memory_bandwidth();
}
```

## Usage Examples

### Complete Application Setup

```cpp
#include <ecscope/ecscope.hpp>
using namespace ecscope;

class GameApplication {
    std::unique_ptr<ecs::Registry> registry;
    std::unique_ptr<renderer::Renderer2D> renderer;
    std::unique_ptr<physics::World2D> physics;
    
public:
    bool initialize() {
        // Initialize ECScope
        if (!ecscope::initialize()) return false;
        
        // Create systems
        registry = ecs::create_educational_registry("Game");
        renderer = std::make_unique<renderer::Renderer2D>();
        physics = std::make_unique<physics::World2D>();
        
        // Initialize renderer
        if (!renderer->initialize(800, 600)) return false;
        
        // Set up physics
        physics->set_gravity(0.0f, -9.81f);
        
        return true;
    }
    
    void update(f32 dt) {
        // Update physics
        physics->step(dt);
        
        // Update ECS systems
        registry->for_each<physics::Transform2D, physics::RigidBody>(
            [&](physics::Transform2D& transform, physics::RigidBody& body) {
                auto state = physics->get_body_state(body.physics_id);
                transform.x = state.x;
                transform.y = state.y;
                transform.rotation = state.rotation;
            }
        );
    }
    
    void render() {
        renderer->begin_frame();
        renderer->clear(0.2f, 0.3f, 0.3f, 1.0f);
        
        // Render sprites
        registry->for_each<physics::Transform2D, renderer::Sprite>(
            [&](const physics::Transform2D& transform, const renderer::Sprite& sprite) {
                renderer->draw_sprite(transform.x, transform.y, 
                                    sprite.width, sprite.height,
                                    sprite.r, sprite.g, sprite.b, sprite.a);
            }
        );
        
        renderer->end_frame();
    }
    
    void shutdown() {
        renderer->shutdown();
        ecscope::shutdown();
    }
};
```

This comprehensive API reference covers the main systems and usage patterns for ECScope. Each system is designed to be both educational and practical, with extensive documentation and debugging capabilities built-in.