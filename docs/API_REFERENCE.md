# ECScope API Reference

**Complete API documentation for ECScope Educational ECS Engine**

## Table of Contents

1. [API Overview](#api-overview)
2. [Core Types and Concepts](#core-types-and-concepts)
3. [ECS System API](#ecs-system-api)
4. [Memory Management API](#memory-management-api)
5. [Physics System API](#physics-system-api)
6. [Rendering System API](#rendering-system-api)
7. [Debugging and Profiling API](#debugging-and-profiling-api)
8. [Educational Features API](#educational-features-api)

## API Overview

### Design Principles

ECScope's API is designed with educational clarity and performance in mind:

- **Type Safety**: Strong typing prevents common errors
- **Zero-Cost Abstractions**: Template-based design with no runtime overhead
- **Clear Ownership**: RAII and unique_ptr for automatic resource management
- **Educational Transparency**: All operations can be monitored and analyzed
- **Modern C++20**: Leverage concepts, ranges, and other modern features

### Header Organization

```cpp
#include <ecscope/core.hpp>          // Core types, Result<T,E>, EntityID
#include <ecscope/ecs.hpp>           // ECS registry, components, systems
#include <ecscope/memory.hpp>        // Custom allocators, memory tracking
#include <ecscope/physics.hpp>       // 2D physics components and systems
#include <ecscope/renderer.hpp>      // 2D rendering pipeline
#include <ecscope/ui.hpp>            // ImGui integration and debug panels
#include <ecscope/performance.hpp>   // Performance laboratory and analysis

// Convenience header (includes everything)
#include <ecscope/ecscope.hpp>
```

## Core Types and Concepts

### Basic Types (`include/core/types.hpp`)

```cpp
namespace ecscope {
    // Educational type aliases for clarity
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
    using byte = std::byte;
    
    // Size constants
    constexpr usize KB = 1024;
    constexpr usize MB = 1024 * KB;
    constexpr usize GB = 1024 * MB;
    
    // Common limits
    constexpr u32 MAX_ENTITIES = 1048576;        // 2^20
    constexpr u32 MAX_COMPONENTS = 64;           // Bitset limit
    constexpr u32 MAX_SYSTEMS = 32;              // Reasonable limit
}
```

### EntityID System (`include/core/id.hpp`)

```cpp
namespace ecscope {
    // Generational entity identifier for safety
    struct EntityID {
        u32 index : 20;      // Entity index (up to 1M entities)
        u32 generation : 12; // Generation counter (up to 4K reuses)
        
        static constexpr EntityID INVALID{0, 0};
        
        // Comparison operators
        bool operator==(const EntityID& other) const noexcept;
        bool operator!=(const EntityID& other) const noexcept;
        bool operator<(const EntityID& other) const noexcept;
        
        // Utility methods
        bool is_valid() const noexcept { return index != 0 || generation != 0; }
        u32 value() const noexcept { return (generation << 20) | index; }
        
        // Educational methods
        std::string to_string() const;
        void print_debug_info() const;
    };
    
    // Hash support for EntityID
    template<>
    struct std::hash<EntityID> {
        usize operator()(const EntityID& id) const noexcept {
            return std::hash<u32>{}(id.value());
        }
    };
}
```

### Error Handling (`include/core/result.hpp`)

```cpp
namespace ecscope {
    // Rust-inspired error handling without exceptions
    template<typename T, typename E>
    class Result {
        std::variant<T, E> value_;
        
    public:
        // Constructors
        explicit Result(T value) : value_(std::move(value)) {}
        explicit Result(E error) : value_(std::move(error)) {}
        
        // State checking
        bool is_ok() const noexcept { return std::holds_alternative<T>(value_); }
        bool is_err() const noexcept { return std::holds_alternative<E>(value_); }
        
        // Value access (safe)
        const T& unwrap() const;
        T unwrap_or(T default_value) const;
        const E& unwrap_err() const;
        
        // Monadic operations
        template<typename Func>
        auto map(Func&& func) -> Result<std::invoke_result_t<Func, T>, E>;
        
        template<typename Func>
        auto and_then(Func&& func) -> std::invoke_result_t<Func, T>;
        
        template<typename Func>
        auto or_else(Func&& func) -> Result<T, E>;
        
        // Educational methods
        void explain_error() const; // Detailed error explanation
        std::string get_error_context() const;
    };
    
    // Common error types
    enum class ECSError {
        EntityNotFound,
        ComponentNotFound,
        ArchetypeNotFound,
        InvalidOperation,
        MemoryAllocationFailed,
        SystemNotFound
    };
    
    // Convenience type aliases
    template<typename T>
    using ECSResult = Result<T, ECSError>;
    
    using VoidResult = Result<void, ECSError>;
}
```

### Mathematical Types (`include/core/math.hpp`)

```cpp
namespace ecscope {
    // 2D Vector with educational methods
    struct Vec2 {
        f32 x, y;
        
        // Constructors
        constexpr Vec2() noexcept : x(0.0f), y(0.0f) {}
        constexpr Vec2(f32 x, f32 y) noexcept : x(x), y(y) {}
        explicit constexpr Vec2(f32 scalar) noexcept : x(scalar), y(scalar) {}
        
        // Arithmetic operators
        Vec2 operator+(const Vec2& other) const noexcept { return {x + other.x, y + other.y}; }
        Vec2 operator-(const Vec2& other) const noexcept { return {x - other.x, y - other.y}; }
        Vec2 operator*(f32 scalar) const noexcept { return {x * scalar, y * scalar}; }
        Vec2 operator/(f32 scalar) const noexcept { return {x / scalar, y / scalar}; }
        
        Vec2& operator+=(const Vec2& other) noexcept { x += other.x; y += other.y; return *this; }
        Vec2& operator-=(const Vec2& other) noexcept { x -= other.x; y -= other.y; return *this; }
        Vec2& operator*=(f32 scalar) noexcept { x *= scalar; y *= scalar; return *this; }
        Vec2& operator/=(f32 scalar) noexcept { x /= scalar; y /= scalar; return *this; }
        
        // Mathematical operations
        f32 magnitude() const noexcept { return std::sqrt(x * x + y * y); }
        f32 magnitude_squared() const noexcept { return x * x + y * y; }
        Vec2 normalized() const noexcept;
        void normalize() noexcept;
        
        f32 distance(const Vec2& other) const noexcept { return (*this - other).magnitude(); }
        f32 distance_squared(const Vec2& other) const noexcept { return (*this - other).magnitude_squared(); }
        
        // Static utility functions
        static f32 dot(const Vec2& a, const Vec2& b) noexcept { return a.x * b.x + a.y * b.y; }
        static f32 cross(const Vec2& a, const Vec2& b) noexcept { return a.x * b.y - a.y * b.x; }
        static Vec2 lerp(const Vec2& a, const Vec2& b, f32 t) noexcept;
        static Vec2 slerp(const Vec2& a, const Vec2& b, f32 t) noexcept; // Spherical interpolation
        
        // Educational methods
        f32 angle() const noexcept { return std::atan2(y, x); }
        Vec2 perpendicular() const noexcept { return {-y, x}; }
        Vec2 reflect(const Vec2& normal) const noexcept;
        Vec2 project(const Vec2& onto) const noexcept;
        
        // String representation
        std::string to_string() const { return std::format("({:.3f}, {:.3f})", x, y); }
    };
    
    // 4x4 Matrix for transformations
    struct Mat4 {
        alignas(16) std::array<f32, 16> data;
        
        // Constructors
        Mat4() noexcept; // Identity matrix
        explicit Mat4(f32 diagonal) noexcept;
        explicit Mat4(const std::array<f32, 16>& values) noexcept;
        
        // Matrix operations
        Mat4 operator*(const Mat4& other) const noexcept;
        Vec2 transform_point(const Vec2& point) const noexcept;
        Vec2 transform_vector(const Vec2& vector) const noexcept;
        
        // Factory methods for common transformations
        static Mat4 identity() noexcept;
        static Mat4 translation(const Vec2& translation) noexcept;
        static Mat4 rotation(f32 angle_radians) noexcept;
        static Mat4 scale(const Vec2& scale) noexcept;
        static Mat4 orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far) noexcept;
        
        // Educational methods
        Mat4 inverse() const noexcept;
        Mat4 transpose() const noexcept;
        f32 determinant() const noexcept;
        void decompose(Vec2& translation, f32& rotation, Vec2& scale) const noexcept;
    };
    
    // Axis-Aligned Bounding Box
    struct AABB {
        Vec2 min, max;
        
        // Constructors
        AABB() noexcept : min(0, 0), max(0, 0) {}
        AABB(const Vec2& min, const Vec2& max) noexcept : min(min), max(max) {}
        
        // Properties
        Vec2 center() const noexcept { return (min + max) * 0.5f; }
        Vec2 size() const noexcept { return max - min; }
        f32 area() const noexcept { const Vec2 s = size(); return s.x * s.y; }
        f32 perimeter() const noexcept { const Vec2 s = size(); return 2.0f * (s.x + s.y); }
        
        // Geometric tests
        bool contains(const Vec2& point) const noexcept;
        bool contains(const AABB& other) const noexcept;
        bool intersects(const AABB& other) const noexcept;
        AABB intersection(const AABB& other) const noexcept;
        AABB union_with(const AABB& other) const noexcept;
        
        // Educational methods
        std::array<Vec2, 4> get_corners() const noexcept;
        f32 distance_to_point(const Vec2& point) const noexcept;
        Vec2 closest_point(const Vec2& point) const noexcept;
    };
}
```

## ECS System API

### Registry (`include/ecs/registry.hpp`)

```cpp
namespace ecscope {
    class Registry {
    public:
        // Entity management
        EntityID create_entity();
        void destroy_entity(EntityID entity);
        bool is_entity_valid(EntityID entity) const noexcept;
        usize get_entity_count() const noexcept;
        std::vector<EntityID> get_all_entities() const;
        
        // Component management
        template<typename Component>
        Component& add_component(EntityID entity, Component component = {});
        
        template<typename Component>
        bool remove_component(EntityID entity);
        
        template<typename Component>
        Component* get_component(EntityID entity);
        
        template<typename Component>
        const Component* get_component(EntityID entity) const;
        
        template<typename Component>
        bool has_component(EntityID entity) const noexcept;
        
        // Multi-component operations
        template<typename... Components>
        bool has_components(EntityID entity) const noexcept;
        
        template<typename... Components>
        std::tuple<Components*...> get_components(EntityID entity);
        
        // Component queries
        template<typename... Components>
        auto view() -> BasicView<Components...>;
        
        template<typename... Components>
        auto view() const -> BasicView<const Components...>;
        
        // System management
        template<typename System, typename... Args>
        System& add_system(Args&&... args);
        
        template<typename System>
        System* get_system();
        
        template<typename System>
        bool remove_system();
        
        void update_systems(f32 delta_time);
        
        // Archetype management
        template<typename... Components>
        ArchetypeID get_or_create_archetype();
        
        ArchetypeID get_entity_archetype(EntityID entity) const;
        const Archetype& get_archetype(ArchetypeID archetype_id) const;
        std::vector<ArchetypeID> get_all_archetypes() const;
        
        // Educational features
        void enable_detailed_logging(bool enable) noexcept;
        void enable_performance_tracking(bool enable) noexcept;
        RegistryStatistics get_statistics() const;
        void print_debug_info() const;
        
        // Memory management
        AllocatorStatistics get_allocator_statistics() const;
        void configure_allocators(const AllocatorConfig& config);
        
        // Performance comparison
        template<typename Func>
        BenchmarkResult benchmark_operation(const std::string& name, Func&& operation, 
                                           usize iterations = 1000);
    };
    
    // Registry configuration for different use cases
    struct RegistryConfig {
        AllocatorConfig allocator_config;
        bool enable_instrumentation{true};
        bool enable_validation{true};
        usize initial_entity_capacity{1000};
        usize initial_component_capacity{10000};
        
        // Factory methods for common configurations
        static RegistryConfig create_educational();
        static RegistryConfig create_performance_optimized();
        static RegistryConfig create_memory_conservative();
        static RegistryConfig create_research();
    };
}
```

### Component System (`include/ecs/component.hpp`)

```cpp
namespace ecscope {
    // Component concept for compile-time validation
    template<typename T>
    concept Component = requires {
        typename T::component_tag;
    } && std::is_trivially_copyable_v<T> 
      && std::is_standard_layout_v<T>;
    
    // Base class for components (provides required type alias)
    struct ComponentBase {
        using component_tag = void;
        
        // Educational methods available to all components
        virtual std::string to_string() const { return "Component"; }
        virtual void print_debug_info() const {}
        virtual usize get_memory_usage() const { return sizeof(*this); }
    };
    
    // Component type registration
    template<typename T>
    ComponentTypeID register_component_type() {
        static_assert(Component<T>, "Type must satisfy Component concept");
        
        static const ComponentTypeID type_id = generate_component_type_id<T>();
        
        // Register component metadata for educational purposes
        ComponentMetadata metadata{};
        metadata.name = typeid(T).name();
        metadata.size = sizeof(T);
        metadata.alignment = alignof(T);
        metadata.is_trivially_copyable = std::is_trivially_copyable_v<T>;
        metadata.is_standard_layout = std::is_standard_layout_v<T>;
        
        register_component_metadata(type_id, metadata);
        return type_id;
    }
    
    // Component signature for archetype matching
    using ComponentSignature = std::bitset<MAX_COMPONENTS>;
    
    template<typename... Components>
    ComponentSignature create_signature() {
        ComponentSignature signature;
        ((signature.set(register_component_type<Components>())), ...);
        return signature;
    }
}
```

### Standard Components

```cpp
namespace ecscope {
    // Transform component for 2D spatial representation
    struct Transform : ComponentBase {
        Vec2 position{0.0f, 0.0f};
        f32 rotation{0.0f};              // Radians
        Vec2 scale{1.0f, 1.0f};
        
        // Transformation methods
        Vec2 transform_point(const Vec2& local_point) const noexcept;
        Vec2 transform_vector(const Vec2& local_vector) const noexcept;
        Vec2 inverse_transform_point(const Vec2& world_point) const noexcept;
        
        Mat4 get_transform_matrix() const noexcept;
        Mat4 get_inverse_transform_matrix() const noexcept;
        
        // Direction vectors
        Vec2 forward() const noexcept { return {std::cos(rotation), std::sin(rotation)}; }
        Vec2 right() const noexcept { return {-std::sin(rotation), std::cos(rotation)}; }
        
        // Educational methods
        void rotate(f32 angle) noexcept { rotation += angle; }
        void look_at(const Vec2& target) noexcept;
        f32 distance_to(const Transform& other) const noexcept;
        
        std::string to_string() const override;
    };
    
    // Velocity component for movement
    struct Velocity : ComponentBase {
        Vec2 velocity{0.0f, 0.0f};       // Linear velocity (units/second)
        f32 angular_velocity{0.0f};      // Angular velocity (radians/second)
        Vec2 acceleration{0.0f, 0.0f};   // Linear acceleration (units/second²)
        f32 angular_acceleration{0.0f};  // Angular acceleration (radians/second²)
        
        // Physics integration
        void integrate(Transform& transform, f32 delta_time) noexcept;
        void apply_force(const Vec2& force, f32 mass) noexcept;
        void apply_torque(f32 torque, f32 moment_of_inertia) noexcept;
        
        // Educational methods
        f32 get_speed() const noexcept { return velocity.magnitude(); }
        f32 get_kinetic_energy(f32 mass) const noexcept;
        Vec2 get_momentum(f32 mass) const noexcept { return velocity * mass; }
        
        std::string to_string() const override;
    };
    
    // Sprite component for 2D rendering
    struct SpriteComponent : ComponentBase {
        Vec2 size{1.0f, 1.0f};
        Vec4 color{1.0f, 1.0f, 1.0f, 1.0f}; // RGBA
        std::string texture_name;
        Vec2 texture_offset{0.0f, 0.0f};     // UV offset
        Vec2 texture_scale{1.0f, 1.0f};      // UV scale
        i32 sort_order{0};                   // Render depth order
        bool visible{true};
        
        // Rendering properties
        enum class BlendMode : u8 {
            Alpha,      // Standard alpha blending
            Additive,   // Additive blending
            Multiply,   // Multiplicative blending
            None        // No blending (opaque)
        };
        
        BlendMode blend_mode{BlendMode::Alpha};
        
        // Educational methods
        f32 get_area() const noexcept { return size.x * size.y; }
        AABB get_local_bounds() const noexcept;
        AABB get_world_bounds(const Transform& transform) const noexcept;
        
        std::string to_string() const override;
    };
}
```

### Query System (`include/ecs/query.hpp`)

```cpp
namespace ecscope {
    // Basic view for iterating entities with specific components
    template<typename... Components>
    class BasicView {
        Registry* registry_;
        std::vector<ArchetypeID> matching_archetypes_;
        
    public:
        explicit BasicView(Registry& registry);
        
        // Iterator support
        class Iterator {
            // Implementation details...
        public:
            std::tuple<EntityID, Components&...> operator*() const;
            Iterator& operator++();
            bool operator!=(const Iterator& other) const;
        };
        
        Iterator begin();
        Iterator end();
        
        // Range-based access
        template<typename Func>
        void each(Func&& func); // func(EntityID, Components&...)
        
        template<typename Func>
        void each_chunk(Func&& func); // func(std::span<Components>...)
        
        // Query information
        usize size() const;
        bool empty() const;
        std::vector<EntityID> entities() const;
        
        // Educational features
        void print_archetype_info() const;
        QueryStatistics get_statistics() const;
    };
    
    // Advanced query with filtering
    template<typename... Components>
    class FilteredView : public BasicView<Components...> {
        std::function<bool(EntityID, const Components&...)> filter_;
        
    public:
        template<typename FilterFunc>
        explicit FilteredView(Registry& registry, FilterFunc&& filter);
        
        // Additional filtering methods
        template<typename Func>
        FilteredView& where(Func&& predicate);
        
        FilteredView& with_component_value(const auto& component, const auto& value);
        FilteredView& within_distance(const Vec2& center, f32 radius);
        FilteredView& in_region(const AABB& region);
    };
    
    // Query statistics for educational analysis
    struct QueryStatistics {
        usize entities_matched;
        usize archetypes_searched;
        f32 query_time_ms;
        f32 cache_hit_ratio;
        usize memory_accesses;
        
        void print_analysis() const;
    };
}
```

## Memory Management API

### Allocator Interfaces (`include/memory/allocators.hpp`)

```cpp
namespace ecscope {
    // Base allocator interface
    class Allocator {
    public:
        virtual ~Allocator() = default;
        
        // Core allocation interface
        virtual void* allocate(usize size, usize alignment = alignof(std::max_align_t)) = 0;
        virtual void deallocate(void* ptr, usize size, usize alignment = alignof(std::max_align_t)) = 0;
        
        // Typed allocation helpers
        template<typename T>
        T* allocate() { return static_cast<T*>(allocate(sizeof(T), alignof(T))); }
        
        template<typename T>
        T* allocate_array(usize count) { 
            return static_cast<T*>(allocate(sizeof(T) * count, alignof(T))); 
        }
        
        template<typename T>
        void deallocate(T* ptr) { deallocate(ptr, sizeof(T), alignof(T)); }
        
        // Educational interface
        virtual std::string get_name() const = 0;
        virtual usize get_total_allocated() const = 0;
        virtual usize get_total_used() const = 0;
        virtual f32 get_fragmentation_ratio() const = 0;
        virtual void print_debug_info() const = 0;
    };
    
    // Arena allocator for linear allocation
    class ArenaAllocator : public Allocator {
        std::unique_ptr<byte[]> memory_;
        usize size_;
        usize offset_;
        
    public:
        explicit ArenaAllocator(usize size);
        
        void* allocate(usize size, usize alignment = alignof(std::max_align_t)) override;
        void deallocate(void* ptr, usize size, usize alignment = alignof(std::max_align_t)) override {} // No-op
        
        // Arena-specific operations
        void reset() noexcept { offset_ = 0; } // Reset entire arena
        usize get_used_bytes() const noexcept { return offset_; }
        usize get_available_bytes() const noexcept { return size_ - offset_; }
        f32 get_utilization() const noexcept { return static_cast<f32>(offset_) / static_cast<f32>(size_); }
        
        // Educational interface implementation
        std::string get_name() const override { return "Arena"; }
        usize get_total_allocated() const override { return size_; }
        usize get_total_used() const override { return offset_; }
        f32 get_fragmentation_ratio() const override { return 0.0f; } // No fragmentation
        void print_debug_info() const override;
        
        // Educational visualization
        struct MemoryMap {
            std::vector<std::pair<usize, usize>> allocated_ranges;
            usize total_size;
            usize used_size;
        };
        
        MemoryMap get_memory_map() const;
        void visualize_memory_layout() const;
    };
    
    // Pool allocator for fixed-size allocations
    template<typename T>
    class PoolAllocator : public Allocator {
        struct Block {
            alignas(T) byte data[sizeof(T)];
            Block* next;
        };
        
        std::unique_ptr<Block[]> memory_;
        Block* free_list_;
        usize capacity_;
        usize allocated_count_;
        
    public:
        explicit PoolAllocator(usize capacity);
        
        T* allocate();
        void deallocate(T* ptr);
        
        // Pool-specific operations
        usize get_capacity() const noexcept { return capacity_; }
        usize get_allocated_count() const noexcept { return allocated_count_; }
        usize get_available_count() const noexcept { return capacity_ - allocated_count_; }
        bool is_full() const noexcept { return allocated_count_ >= capacity_; }
        
        // Educational interface implementation
        void* allocate(usize size, usize alignment) override;
        void deallocate(void* ptr, usize size, usize alignment) override;
        std::string get_name() const override { return std::format("Pool<{}>", typeid(T).name()); }
        usize get_total_allocated() const override { return capacity_ * sizeof(T); }
        usize get_total_used() const override { return allocated_count_ * sizeof(T); }
        f32 get_fragmentation_ratio() const override { return 0.0f; } // No fragmentation
        void print_debug_info() const override;
        
        // Educational features
        std::vector<bool> get_allocation_bitmap() const;
        void visualize_pool_state() const;
    };
}
```

### Memory Tracking (`include/memory/mem_tracker.hpp`)

```cpp
namespace ecscope {
    class MemoryTracker {
    public:
        struct AllocationInfo {
            void* address;
            usize size;
            usize alignment;
            f64 timestamp;
            std::string allocator_name;
            const char* source_location;
            
            // Educational data
            usize allocation_id;         // Unique allocation identifier
            f32 lifetime_seconds;        // How long allocation lived
            bool was_leaked;             // Whether this allocation was leaked
        };
        
        struct MemorySnapshot {
            f64 timestamp;
            usize total_allocated_bytes;
            usize total_used_bytes;
            usize active_allocations;
            f32 fragmentation_percentage;
            std::unordered_map<std::string, usize> allocator_breakdown;
            
            // Cache behavior metrics
            f32 cache_hit_ratio;
            usize cache_misses;
            usize cache_lines_touched;
        };
        
        // Tracking interface
        void record_allocation(const AllocationInfo& info);
        void record_deallocation(void* address);
        void record_cache_access(void* address, bool was_hit);
        
        // Snapshot management
        MemorySnapshot capture_snapshot() const;
        std::vector<MemorySnapshot> get_history(f32 time_window_seconds = 60.0f) const;
        
        // Analysis methods
        std::vector<AllocationInfo> find_potential_leaks(f32 age_threshold_seconds = 30.0f) const;
        std::vector<AllocationInfo> find_short_lived_allocations(f32 lifetime_threshold_ms = 1.0f) const;
        std::unordered_map<std::string, usize> get_allocation_size_distribution() const;
        
        // Educational features
        void enable_access_pattern_tracking(bool enable) noexcept;
        void enable_leak_detection(bool enable) noexcept;
        void enable_cache_analysis(bool enable) noexcept;
        
        // Reporting
        void generate_memory_report(const std::string& filename) const;
        void print_memory_summary() const;
        
        // Performance impact
        void set_tracking_overhead_budget(f32 max_overhead_percentage);
        f32 get_current_tracking_overhead() const;
    };
    
    // Global memory tracking (educational mode)
    void enable_global_memory_tracking(bool enable);
    MemoryTracker& get_global_memory_tracker();
    
    // RAII memory tracking scope
    class MemoryTrackingScope {
        std::string scope_name_;
        MemorySnapshot start_snapshot_;
        
    public:
        explicit MemoryTrackingScope(std::string name);
        ~MemoryTrackingScope();
        
        MemorySnapshot get_delta_snapshot() const;
        void print_scope_summary() const;
    };
    
    #define TRACK_MEMORY_SCOPE(name) \
        MemoryTrackingScope _mem_scope(name)
}
```

## Physics System API

### Physics World (`include/physics/world.hpp`)

```cpp
namespace ecscope {
    class PhysicsWorld {
    public:
        struct Config {
            Vec2 gravity{0.0f, -9.81f};     // World gravity (m/s²)
            u32 velocity_iterations{8};      // Constraint solver iterations
            u32 position_iterations{3};      // Position correction iterations
            f32 time_step{1.0f / 60.0f};    // Fixed time step
            bool enable_sleeping{true};      // Put inactive bodies to sleep
            bool enable_continuous_collision{false}; // CCD for fast objects
            f32 linear_damping{0.01f};       // Global linear damping
            f32 angular_damping{0.01f};      // Global angular damping
        };
        
        explicit PhysicsWorld(const Config& config = {});
        
        // Simulation control
        void update(Registry& registry, f32 delta_time);
        void step(Registry& registry); // Single fixed time step
        void reset(Registry& registry); // Reset all physics state
        
        // World properties
        void set_gravity(const Vec2& gravity) noexcept;
        Vec2 get_gravity() const noexcept;
        void set_time_scale(f32 scale) noexcept;
        f32 get_time_scale() const noexcept;
        
        // Collision queries
        std::vector<EntityID> query_point(const Vec2& point) const;
        std::vector<EntityID> query_region(const AABB& region) const;
        std::vector<EntityID> query_ray(const Vec2& start, const Vec2& direction, f32 max_distance) const;
        
        // Force application
        void apply_force_at_point(EntityID entity, const Vec2& force, const Vec2& point);
        void apply_impulse_at_point(EntityID entity, const Vec2& impulse, const Vec2& point);
        void apply_explosion(const Vec2& center, f32 force, f32 radius);
        
        // Educational features
        void enable_step_by_step_debugging(bool enable);
        void enable_visual_debugging(bool enable);
        PhysicsStatistics get_statistics() const;
        
        // Debug stepping
        void set_debug_step_mode(bool enable);
        void execute_single_debug_step();
        std::string get_current_debug_phase() const;
        
        // Performance analysis
        void enable_performance_profiling(bool enable);
        PerformanceProfile get_performance_profile() const;
    };
    
    // Physics statistics for educational analysis
    struct PhysicsStatistics {
        u32 active_rigidbodies;
        u32 active_colliders;
        u32 sleeping_bodies;
        u32 collision_pairs_tested;
        u32 collision_pairs_colliding;
        u32 active_constraints;
        f32 total_kinetic_energy;
        f32 total_potential_energy;
        
        // Performance metrics
        f32 broad_phase_time_ms;
        f32 narrow_phase_time_ms;
        f32 constraint_solve_time_ms;
        f32 integration_time_ms;
        f32 total_physics_time_ms;
        
        // Educational metrics
        u32 spatial_grid_cells_active;
        f32 spatial_grid_efficiency;
        f32 solver_convergence_ratio;
        
        void print_summary() const;
    };
}
```

### Physics Components (`include/physics/components.hpp`)

```cpp
namespace ecscope {
    // RigidBody component for physics simulation
    struct RigidBody : ComponentBase {
        // Linear properties
        Vec2 velocity{0.0f, 0.0f};
        Vec2 acceleration{0.0f, 0.0f};
        Vec2 force{0.0f, 0.0f};
        f32 mass{1.0f};
        f32 inv_mass{1.0f};
        
        // Angular properties
        f32 angular_velocity{0.0f};
        f32 angular_acceleration{0.0f};
        f32 torque{0.0f};
        f32 moment_of_inertia{1.0f};
        f32 inv_moment_of_inertia{1.0f};
        
        // Material properties
        f32 restitution{0.5f};           // Coefficient of restitution [0,1]
        f32 friction{0.5f};              // Friction coefficient [0,∞)
        f32 density{1.0f};               // Material density (kg/m³)
        
        // Simulation control
        bool is_static{false};           // Static bodies don't move
        bool is_kinematic{false};        // Kinematic bodies ignore forces
        bool enable_gravity{true};       // Apply world gravity
        bool enable_sleeping{true};      // Allow body to sleep when inactive
        bool is_sleeping{false};         // Current sleep state
        
        // Physics methods
        void apply_force(const Vec2& force, const Vec2& local_point = Vec2{0,0}) noexcept;
        void apply_impulse(const Vec2& impulse, const Vec2& local_point = Vec2{0,0}) noexcept;
        void set_mass(f32 new_mass) noexcept;
        void wake_up() noexcept { is_sleeping = false; }
        
        // Educational methods
        f32 get_kinetic_energy() const noexcept;
        f32 get_potential_energy(const Vec2& gravity) const noexcept;
        Vec2 get_velocity_at_point(const Vec2& local_point) const noexcept;
        Vec2 get_momentum() const noexcept { return velocity * mass; }
        f32 get_angular_momentum() const noexcept { return angular_velocity * moment_of_inertia; }
        
        std::string to_string() const override;
    };
    
    // Base collider component
    struct Collider : ComponentBase {
        enum class Type : u8 { Circle, Box, Polygon, Capsule };
        
        Type type;
        Vec2 offset{0.0f, 0.0f};         // Local offset from transform
        f32 rotation{0.0f};              // Local rotation
        bool is_trigger{false};          // Trigger vs solid collider
        u32 collision_layer{0};          // Collision layer bitmask
        u32 collision_mask{0xFFFFFFFF};  // What layers this collider interacts with
        
        // Material properties
        f32 restitution{-1.0f};          // -1 uses RigidBody restitution
        f32 friction{-1.0f};             // -1 uses RigidBody friction
        
        // Virtual interface
        virtual AABB get_local_aabb() const noexcept = 0;
        virtual AABB get_world_aabb(const Transform& transform) const noexcept = 0;
        virtual bool contains_point(const Vec2& local_point) const noexcept = 0;
        virtual f32 calculate_moment_of_inertia(f32 mass) const noexcept = 0;
        
        // Educational methods
        virtual void debug_draw(DebugRenderer& renderer, const Transform& transform) const = 0;
        virtual std::string get_shape_description() const = 0;
    };
    
    // Circle collider implementation
    struct CircleCollider : Collider {
        f32 radius{1.0f};
        
        explicit CircleCollider(f32 r = 1.0f) : radius(r) { type = Type::Circle; }
        
        // Virtual interface implementation
        AABB get_local_aabb() const noexcept override;
        AABB get_world_aabb(const Transform& transform) const noexcept override;
        bool contains_point(const Vec2& local_point) const noexcept override;
        f32 calculate_moment_of_inertia(f32 mass) const noexcept override;
        
        // Educational implementation
        void debug_draw(DebugRenderer& renderer, const Transform& transform) const override;
        std::string get_shape_description() const override;
        
        // Circle-specific methods
        f32 get_area() const noexcept { return M_PI * radius * radius; }
        f32 get_circumference() const noexcept { return 2.0f * M_PI * radius; }
        
        std::string to_string() const override;
    };
    
    // Box collider implementation
    struct BoxCollider : Collider {
        Vec2 half_extents{1.0f, 1.0f};   // Half-width and half-height
        
        explicit BoxCollider(const Vec2& extents = Vec2{1,1}) : half_extents(extents) { type = Type::Box; }
        
        // Virtual interface implementation
        AABB get_local_aabb() const noexcept override;
        AABB get_world_aabb(const Transform& transform) const noexcept override;
        bool contains_point(const Vec2& local_point) const noexcept override;
        f32 calculate_moment_of_inertia(f32 mass) const noexcept override;
        
        // Educational implementation
        void debug_draw(DebugRenderer& renderer, const Transform& transform) const override;
        std::string get_shape_description() const override;
        
        // Box-specific methods
        Vec2 get_size() const noexcept { return half_extents * 2.0f; }
        f32 get_area() const noexcept { return 4.0f * half_extents.x * half_extents.y; }
        f32 get_perimeter() const noexcept { return 4.0f * (half_extents.x + half_extents.y); }
        std::array<Vec2, 4> get_vertices(const Transform& transform) const noexcept;
        
        std::string to_string() const override;
    };
}
```

## Rendering System API

### Renderer Interface (`include/renderer/renderer_2d.hpp`)

```cpp
namespace ecscope {
    class Renderer2D {
    public:
        struct Config {
            u32 max_quads_per_batch{1000};
            u32 max_textures_per_batch{16};
            bool enable_batching{true};
            bool enable_frustum_culling{true};
            bool enable_depth_sorting{true};
            bool enable_debug_visualization{false};
            
            // Educational features
            bool enable_batch_analysis{true};
            bool enable_performance_tracking{true};
            bool enable_gpu_timing{true};
        };
        
        explicit Renderer2D(const Config& config = {});
        
        // Frame management
        void begin_frame(const Camera2D& camera);
        void end_frame();
        void clear(const Vec4& color = Vec4{0.1f, 0.1f, 0.1f, 1.0f});
        
        // Sprite rendering
        void submit_sprite(const Transform& transform, const SpriteComponent& sprite);
        void submit_sprite(const Vec2& position, const Vec2& size, const std::string& texture,
                          const Vec4& color = Vec4{1,1,1,1}, f32 rotation = 0.0f);
        
        // Primitive rendering
        void submit_line(const Vec2& start, const Vec2& end, const Vec4& color, f32 width = 1.0f);
        void submit_circle(const Vec2& center, f32 radius, const Vec4& color, bool filled = false);
        void submit_rectangle(const Vec2& position, const Vec2& size, const Vec4& color, bool filled = true);
        
        // Text rendering
        void submit_text(const Vec2& position, const std::string& text, const Vec4& color = Vec4{1,1,1,1});
        
        // Advanced rendering
        void submit_particles(const ParticleSystem& particles);
        void submit_debug_visualization(const DebugVisualization& debug_vis);
        
        // Render state management
        void set_camera(const Camera2D& camera);
        void push_transform(const Mat4& transform);
        void pop_transform();
        
        // Educational features
        RenderStatistics get_frame_statistics() const;
        std::vector<BatchInfo> get_batch_information() const;
        void enable_wireframe_mode(bool enable);
        void enable_batch_visualization(bool enable);
        
        // Resource management
        TextureHandle load_texture(const std::string& filename);
        void unload_texture(const std::string& name);
        ShaderHandle load_shader(const std::string& vertex_path, const std::string& fragment_path);
    };
    
    // Render statistics for performance analysis
    struct RenderStatistics {
        u32 total_draw_calls;
        u32 total_batches;
        u32 total_vertices;
        u32 total_primitives;
        u32 texture_switches;
        f32 batch_efficiency_percentage;
        f32 gpu_time_ms;
        f32 cpu_time_ms;
        
        // Educational metrics
        u32 entities_culled;
        u32 entities_rendered;
        f32 overdraw_factor;
        usize gpu_memory_used_bytes;
        
        void print_analysis() const;
    };
    
    // Batch information for educational analysis
    struct BatchInfo {
        u32 quad_count;
        u32 texture_count;
        f32 utilization_percentage;
        std::vector<std::string> texture_names;
        AABB bounding_box;
        
        void print_details() const;
    };
}
```

### Camera System (`include/renderer/camera.hpp`)

```cpp
namespace ecscope {
    struct Camera2D : ComponentBase {
        // Basic properties
        Vec2 position{0.0f, 0.0f};
        f32 zoom{1.0f};
        f32 rotation{0.0f};
        
        // Projection settings
        f32 viewport_width{1920.0f};
        f32 viewport_height{1080.0f};
        f32 near_plane{-100.0f};
        f32 far_plane{100.0f};
        
        // Camera behavior
        EntityID follow_target{EntityID::INVALID};
        Vec2 follow_offset{0.0f, 0.0f};
        f32 follow_smoothing{5.0f};
        
        // Camera bounds (optional)
        bool has_bounds{false};
        AABB bounds{Vec2{-1000, -1000}, Vec2{1000, 1000}};
        
        // Screen shake
        f32 shake_intensity{0.0f};
        f32 shake_duration{0.0f};
        f32 shake_frequency{20.0f};
        
        // Matrix calculations
        Mat4 get_view_matrix() const noexcept;
        Mat4 get_projection_matrix() const noexcept;
        Mat4 get_view_projection_matrix() const noexcept;
        
        // Coordinate conversions
        Vec2 screen_to_world(const Vec2& screen_pos) const noexcept;
        Vec2 world_to_screen(const Vec2& world_pos) const noexcept;
        
        // Visibility testing
        bool is_point_visible(const Vec2& world_pos) const noexcept;
        bool is_aabb_visible(const AABB& world_aabb) const noexcept;
        AABB get_visible_world_bounds() const noexcept;
        
        // Camera effects
        void shake(f32 intensity, f32 duration, f32 frequency = 20.0f) noexcept;
        void set_follow_target(EntityID target, const Vec2& offset = Vec2{0,0}) noexcept;
        void clear_follow_target() noexcept { follow_target = EntityID::INVALID; }
        
        std::string to_string() const override;
    };
    
    // Multi-camera viewport management
    struct Viewport {
        i32 x, y;                        // Screen position
        u32 width, height;               // Screen size
        EntityID camera;                 // Associated camera entity
        f32 depth_priority{0.0f};        // Render order
        bool is_active{true};
        Vec4 clear_color{0.1f, 0.1f, 0.1f, 1.0f};
        
        // Post-processing
        bool enable_post_processing{false};
        std::string post_process_shader;
        
        // Educational features
        bool show_debug_overlay{false};
        bool show_performance_info{false};
    };
    
    class ViewportManager {
    public:
        // Viewport management
        ViewportID create_viewport(const Viewport& viewport);
        void destroy_viewport(ViewportID viewport_id);
        void update_viewport(ViewportID viewport_id, const Viewport& viewport);
        Viewport* get_viewport(ViewportID viewport_id);
        
        // Rendering
        void render_all_viewports(Registry& registry, Renderer2D& renderer);
        void render_viewport(ViewportID viewport_id, Registry& registry, Renderer2D& renderer);
        
        // Educational features
        std::vector<ViewportID> get_all_viewports() const;
        ViewportStatistics get_viewport_statistics(ViewportID viewport_id) const;
    };
}
```

## Debugging and Profiling API

### Debug Renderer (`include/debug/debug_renderer.hpp`)

```cpp
namespace ecscope {
    class DebugRenderer {
    public:
        // Immediate mode drawing (rendered next frame)
        void draw_line(const Vec2& start, const Vec2& end, const Vec4& color, f32 width = 1.0f, f32 duration = 0.0f);
        void draw_arrow(const Vec2& start, const Vec2& end, const Vec4& color, f32 head_size = 0.2f, f32 duration = 0.0f);
        void draw_circle(const Vec2& center, f32 radius, const Vec4& color, bool filled = false, f32 duration = 0.0f);
        void draw_rectangle(const Vec2& center, const Vec2& size, const Vec4& color, bool filled = true, f32 duration = 0.0f);
        void draw_text(const Vec2& position, const std::string& text, const Vec4& color = Vec4{1,1,1,1}, f32 duration = 0.0f);
        
        // Physics-specific visualization
        void draw_velocity_vector(const Transform& transform, const Velocity& velocity, const Vec4& color = Vec4{0,1,0,1});
        void draw_force_vector(const Transform& transform, const Vec2& force, const Vec4& color = Vec4{1,0,0,1});
        void draw_collision_info(const CollisionInfo& collision);
        void draw_constraint(const Constraint& constraint);
        
        // Coordinate system helpers
        void draw_coordinate_system(const Transform& transform, f32 scale = 1.0f);
        void draw_grid(f32 spacing, const Vec4& color = Vec4{0.3f, 0.3f, 0.3f, 1.0f});
        
        // Performance visualization
        void draw_performance_graph(const Vec2& position, const Vec2& size, 
                                  const std::vector<f32>& data, const std::string& label);
        void draw_memory_usage_bar(const Vec2& position, const Vec2& size, f32 usage_ratio);
        
        // Debug modes
        void enable_physics_debug(bool enable);
        void enable_memory_debug(bool enable);
        void enable_performance_debug(bool enable);
        void set_debug_camera(EntityID camera_entity);
        
        // Batch management
        void flush_debug_draws(const Camera2D& camera);
        void clear_temporary_draws();
        
        // Educational features
        void explain_draw_call(const std::string& explanation);
        void highlight_entity(EntityID entity, f32 duration = 1.0f);
        void trace_memory_access(void* address, const std::string& description);
    };
    
    // Global debug renderer access (for convenience)
    DebugRenderer& get_debug_renderer();
    
    // RAII debug scope for automatic cleanup
    class DebugScope {
        std::string scope_name_;
        Vec4 scope_color_;
        
    public:
        explicit DebugScope(std::string name, const Vec4& color = Vec4{1,1,1,1});
        ~DebugScope();
        
        void add_comment(const std::string& comment);
        void highlight_area(const AABB& area);
    };
    
    #define DEBUG_SCOPE(name) DebugScope _debug_scope(name)
    #define DEBUG_SCOPE_COLOR(name, color) DebugScope _debug_scope(name, color)
}
```

### Performance Profiler (`include/instrumentation/profiler.hpp`)

```cpp
namespace ecscope {
    class PerformanceProfiler {
    public:
        struct ProfileScope {
            std::string name;
            f64 start_time;
            f64 end_time;
            f32 duration_ms;
            u32 call_count;
            usize memory_used_bytes;
            
            // Educational metrics
            f32 cpu_utilization;
            f32 cache_hit_ratio;
            usize instructions_executed;
        };
        
        // Profiling control
        void begin_frame();
        void end_frame();
        void begin_scope(const std::string& name);
        void end_scope(const std::string& name);
        
        // Automatic scope timing
        class ScopeTimer {
            PerformanceProfiler& profiler_;
            std::string scope_name_;
            
        public:
            ScopeTimer(PerformanceProfiler& profiler, std::string name);
            ~ScopeTimer();
        };
        
        // Query interface
        std::vector<ProfileScope> get_frame_scopes() const;
        std::vector<ProfileScope> get_scope_history(const std::string& scope_name, 
                                                   f32 time_window_seconds = 10.0f) const;
        f32 get_average_frame_time(f32 time_window_seconds = 5.0f) const;
        
        // Statistical analysis
        ProfileStatistics calculate_statistics(const std::string& scope_name) const;
        std::vector<std::string> identify_bottlenecks(f32 threshold_percentage = 10.0f) const;
        
        // Educational features
        void enable_detailed_tracking(bool enable);
        void enable_cache_analysis(bool enable);
        void enable_instruction_counting(bool enable);
        
        void print_frame_summary() const;
        void generate_performance_report(const std::string& filename) const;
        
        // Real-time monitoring
        void set_performance_alert_threshold(f32 frame_time_ms);
        bool has_performance_alerts() const;
        std::vector<std::string> get_performance_alerts() const;
    };
    
    // Global profiler instance
    PerformanceProfiler& get_global_profiler();
    
    // Convenience macros
    #define PROFILE_SCOPE(name) \
        PerformanceProfiler::ScopeTimer _timer(get_global_profiler(), name)
    
    #define PROFILE_FUNCTION() \
        PROFILE_SCOPE(__FUNCTION__)
    
    // Educational profiling helpers
    #define PROFILE_COMPARE(name, func1, func2) \
        compare_performance(name, func1, func2)
    
    template<typename Func1, typename Func2>
    void compare_performance(const std::string& name, Func1&& baseline, Func2&& optimized) {
        auto& profiler = get_global_profiler();
        
        // Benchmark baseline
        profiler.begin_scope(name + "_baseline");
        baseline();
        profiler.end_scope(name + "_baseline");
        
        // Benchmark optimized
        profiler.begin_scope(name + "_optimized");
        optimized();
        profiler.end_scope(name + "_optimized");
        
        // Generate comparison report
        profiler.generate_comparison_report(name + "_baseline", name + "_optimized");
    }
}
```

## Educational Features API

### Memory Laboratory (`include/performance/memory_lab.hpp`)

```cpp
namespace ecscope {
    class MemoryLaboratory {
    public:
        enum class ExperimentType {
            AllocationPattern,
            CacheEfficiency,
            FragmentationAnalysis,
            AllocatorComparison,
            AccessPatternOptimization
        };
        
        struct ExperimentConfig {
            ExperimentType type;
            usize entity_count{1000};
            usize iteration_count{100};
            f32 duration_seconds{5.0f};
            bool enable_visualization{true};
            bool enable_detailed_analysis{true};
            
            // Experiment-specific parameters
            std::unordered_map<std::string, f32> parameters;
        };
        
        struct ExperimentResult {
            std::string experiment_name;
            f32 execution_time_ms;
            usize memory_usage_bytes;
            f32 cache_efficiency;
            f32 fragmentation_ratio;
            std::vector<f32> performance_data;
            std::vector<std::string> insights;
            
            void print_summary() const;
            void export_data(const std::string& filename) const;
        };
        
        // Experiment management
        ExperimentResult run_experiment(const ExperimentConfig& config);
        std::vector<ExperimentResult> run_experiment_suite();
        
        // Interactive laboratory
        void start_interactive_session();
        void render_laboratory_ui();
        void update_current_experiment(f32 delta_time);
        
        // Predefined experiments
        ExperimentResult compare_allocators(usize entity_count = 10000);
        ExperimentResult analyze_cache_patterns(usize access_count = 100000);
        ExperimentResult measure_fragmentation(f32 duration_seconds = 10.0f);
        ExperimentResult benchmark_soa_vs_aos(usize component_count = 10000);
        
        // Educational guidance
        std::vector<std::string> suggest_next_experiments(const ExperimentResult& result) const;
        std::string explain_result(const ExperimentResult& result) const;
        void generate_learning_report(const std::vector<ExperimentResult>& results) const;
    };
    
    // Global memory laboratory instance
    MemoryLaboratory& get_memory_laboratory();
}
```

### Performance Analysis (`include/performance/analyzer.hpp`)

```cpp
namespace ecscope {
    class PerformanceAnalyzer {
    public:
        struct SystemProfile {
            std::string system_name;
            f32 average_time_ms;
            f32 max_time_ms;
            f32 time_percentage;
            u32 call_count;
            
            // Bottleneck classification
            bool is_cpu_bound;
            bool is_memory_bound;
            bool is_cache_bound;
            
            // Optimization potential
            f32 optimization_potential_percentage;
            std::vector<std::string> optimization_suggestions;
        };
        
        // Analysis methods
        std::vector<SystemProfile> analyze_system_performance(Registry& registry, f32 analysis_duration = 5.0f);
        BottleneckAnalysis identify_bottlenecks(Registry& registry);
        OptimizationRecommendations generate_recommendations(const std::vector<SystemProfile>& profiles);
        
        // Real-time monitoring
        void start_monitoring(Registry& registry);
        void stop_monitoring();
        bool is_monitoring() const;
        
        // Interactive analysis
        void render_analysis_ui(Registry& registry);
        void render_system_performance_graphs();
        void render_optimization_suggestions();
        
        // Comparative analysis
        template<typename Func>
        ComparisonResult compare_implementations(const std::string& name, 
                                               Func&& implementation_a,
                                               Func&& implementation_b,
                                               usize iterations = 100);
        
        // Educational features
        void explain_performance_characteristics(const SystemProfile& profile);
        void demonstrate_optimization_impact(const std::string& optimization_name);
        void provide_learning_path(const BottleneckAnalysis& analysis);
    };
    
    // Bottleneck analysis results
    struct BottleneckAnalysis {
        std::string primary_bottleneck;
        f32 bottleneck_severity; // 0.0 = no issue, 1.0 = severe
        std::vector<std::string> contributing_factors;
        std::vector<std::string> immediate_actions;
        std::vector<std::string> long_term_optimizations;
        
        void print_analysis() const;
    };
    
    // Optimization recommendations
    struct OptimizationRecommendations {
        struct Recommendation {
            std::string name;
            std::string description;
            f32 estimated_impact; // Expected performance improvement ratio
            f32 implementation_difficulty; // 1.0 = easy, 5.0 = very difficult
            std::vector<std::string> implementation_steps;
            std::vector<std::string> prerequisites;
        };
        
        std::vector<Recommendation> high_impact_optimizations;
        std::vector<Recommendation> quick_wins;
        std::vector<Recommendation> long_term_projects;
        
        void print_recommendations() const;
        void export_action_plan(const std::string& filename) const;
    };
}
```

### Educational UI System (`include/ui/educational_panels.hpp`)

```cpp
namespace ecscope {
    // Base class for educational debug panels
    class EducationalPanel {
    protected:
        std::string panel_name_;
        bool is_visible_{true};
        bool show_educational_overlay_{true};
        
    public:
        explicit EducationalPanel(std::string name) : panel_name_(std::move(name)) {}
        virtual ~EducationalPanel() = default;
        
        // Pure virtual interface
        virtual void update(Registry& registry, f32 delta_time) = 0;
        virtual void render(Registry& registry) = 0;
        
        // Panel management
        void set_visible(bool visible) noexcept { is_visible_ = visible; }
        bool is_visible() const noexcept { return is_visible_; }
        const std::string& get_name() const noexcept { return panel_name_; }
        
        // Educational features
        virtual void show_help_overlay() const {}
        virtual std::vector<std::string> get_learning_objectives() const { return {}; }
        virtual std::string get_educational_description() const { return ""; }
        
        // Panel state persistence
        virtual void save_state(const std::string& filename) const {}
        virtual void load_state(const std::string& filename) {}
    };
    
    // ECS Inspector Panel
    class ECSInspectorPanel : public EducationalPanel {
        EntityID selected_entity_{EntityID::INVALID};
        std::string entity_filter_;
        bool show_component_details_{true};
        bool show_archetype_info_{true};
        
    public:
        ECSInspectorPanel() : EducationalPanel("ECS Inspector") {}
        
        void update(Registry& registry, f32 delta_time) override;
        void render(Registry& registry) override;
        
        // Inspector-specific methods
        void select_entity(EntityID entity) { selected_entity_ = entity; }
        EntityID get_selected_entity() const { return selected_entity_; }
        void set_entity_filter(const std::string& filter) { entity_filter_ = filter; }
        
        // Educational implementation
        void show_help_overlay() const override;
        std::vector<std::string> get_learning_objectives() const override;
        std::string get_educational_description() const override;
    };
    
    // Memory Analysis Panel
    class MemoryAnalysisPanel : public EducationalPanel {
        f32 update_interval_{0.1f};
        f32 time_since_update_{0.0f};
        CircularBuffer<MemorySnapshot, 300> memory_history_;
        
    public:
        MemoryAnalysisPanel() : EducationalPanel("Memory Analysis") {}
        
        void update(Registry& registry, f32 delta_time) override;
        void render(Registry& registry) override;
        
        // Memory analysis methods
        void analyze_allocation_patterns(Registry& registry);
        void suggest_optimizations(Registry& registry);
        void compare_allocator_performance(Registry& registry);
        
        // Educational implementation
        void show_help_overlay() const override;
        std::vector<std::string> get_learning_objectives() const override;
    };
    
    // Physics Debug Panel
    class PhysicsDebugPanel : public EducationalPanel {
        bool show_collision_shapes_{true};
        bool show_velocity_vectors_{false};
        bool show_force_vectors_{false};
        bool enable_step_mode_{false};
        
    public:
        PhysicsDebugPanel() : EducationalPanel("Physics Debug") {}
        
        void update(Registry& registry, f32 delta_time) override;
        void render(Registry& registry) override;
        
        // Physics debugging methods
        void toggle_collision_visualization();
        void toggle_constraint_visualization();
        void set_step_by_step_mode(bool enable);
        void execute_physics_step(Registry& registry);
        
        // Educational implementation
        void show_help_overlay() const override;
        std::vector<std::string> get_learning_objectives() const override;
    };
    
    // Panel manager for organizing multiple panels
    class PanelManager {
        std::vector<std::unique_ptr<EducationalPanel>> panels_;
        std::unordered_map<std::string, bool> panel_visibility_;
        
    public:
        // Panel management
        template<typename PanelType, typename... Args>
        PanelType& add_panel(Args&&... args);
        
        void remove_panel(const std::string& name);
        EducationalPanel* get_panel(const std::string& name);
        
        // Update and rendering
        void update_all_panels(Registry& registry, f32 delta_time);
        void render_all_panels(Registry& registry);
        
        // UI management
        void render_panel_menu();
        void render_educational_overlay();
        
        // Panel state management
        void save_panel_layout(const std::string& filename) const;
        void load_panel_layout(const std::string& filename);
        
        // Educational features
        void show_welcome_tutorial();
        void show_learning_path_guide();
        std::vector<std::string> get_recommended_panels_for_topic(const std::string& topic) const;
    };
}
```

## Usage Examples

### Basic ECS Usage

```cpp
#include <ecscope/ecscope.hpp>

int main() {
    using namespace ecscope;
    
    // Create registry with educational configuration
    Registry registry{RegistryConfig::create_educational()};
    
    // Create entity with components
    const EntityID player = registry.create_entity();
    registry.add_component<Transform>(player, Transform{
        .position = Vec2{0.0f, 0.0f},
        .rotation = 0.0f,
        .scale = Vec2{1.0f, 1.0f}
    });
    registry.add_component<Velocity>(player, Velocity{
        .velocity = Vec2{5.0f, 0.0f},
        .angular_velocity = 1.0f
    });
    
    // Create movement system
    registry.add_system<MovementSystem>();
    
    // Game loop
    f32 delta_time = 1.0f / 60.0f;
    for (u32 frame = 0; frame < 600; ++frame) { // 10 seconds at 60 FPS
        registry.update_systems(delta_time);
        
        // Print educational information every second
        if (frame % 60 == 0) {
            const auto* transform = registry.get_component<Transform>(player);
            log_info("Frame {}: Player at ({:.2f}, {:.2f})", 
                    frame, transform->position.x, transform->position.y);
        }
    }
    
    // Display performance statistics
    const auto stats = registry.get_statistics();
    stats.print_summary();
    
    return 0;
}
```

### Physics Simulation Example

```cpp
#include <ecscope/ecscope.hpp>

int main() {
    using namespace ecscope;
    
    // Create physics-enabled registry
    Registry registry{RegistryConfig::create_educational()};
    
    // Create physics world
    PhysicsWorld physics_world{PhysicsWorld::Config{
        .gravity = Vec2{0.0f, -9.81f},
        .velocity_iterations = 8,
        .position_iterations = 3
    }};
    
    // Add physics system
    registry.add_system<PhysicsSystem>(physics_world);
    
    // Create ground
    const EntityID ground = registry.create_entity();
    registry.add_component<Transform>(ground, Transform{.position = Vec2{0, -5}});
    registry.add_component<BoxCollider>(ground, BoxCollider{Vec2{10, 0.5f}});
    registry.add_component<RigidBody>(ground, RigidBody{.is_static = true});
    
    // Create falling ball
    const EntityID ball = registry.create_entity();
    registry.add_component<Transform>(ball, Transform{.position = Vec2{0, 10}});
    registry.add_component<CircleCollider>(ball, CircleCollider{1.0f});
    registry.add_component<RigidBody>(ball, RigidBody{
        .mass = 1.0f,
        .restitution = 0.8f
    });
    
    // Enable debug visualization
    physics_world.enable_visual_debugging(true);
    
    // Simulation loop
    for (u32 frame = 0; frame < 600; ++frame) {
        const f32 delta_time = 1.0f / 60.0f;
        registry.update_systems(delta_time);
        
        // Educational output
        if (frame % 60 == 0) {
            const auto* ball_transform = registry.get_component<Transform>(ball);
            const auto* ball_rigidbody = registry.get_component<RigidBody>(ball);
            
            log_info("Second {}: Ball at height {:.2f}, velocity {:.2f} m/s", 
                    frame / 60, ball_transform->position.y, ball_rigidbody->velocity.magnitude());
        }
    }
    
    // Display physics statistics
    const auto physics_stats = physics_world.get_statistics();
    physics_stats.print_summary();
    
    return 0;
}
```

### Memory Laboratory Example

```cpp
#include <ecscope/ecscope.hpp>

int main() {
    using namespace ecscope;
    
    // Start memory laboratory session
    auto& memory_lab = get_memory_laboratory();
    
    // Configure experiment
    MemoryLaboratory::ExperimentConfig config{
        .type = MemoryLaboratory::ExperimentType::AllocatorComparison,
        .entity_count = 10000,
        .iteration_count = 100,
        .enable_visualization = true,
        .parameters = {
            {"use_arena_allocator", 1.0f},
            {"use_pool_allocator", 1.0f},
            {"enable_soa_layout", 1.0f}
        }
    };
    
    // Run experiment
    const auto result = memory_lab.run_experiment(config);
    
    // Display results
    result.print_summary();
    
    // Get educational insights
    const auto insights = memory_lab.explain_result(result);
    log_info("Educational insights: {}", insights);
    
    // Suggest follow-up experiments
    const auto suggestions = memory_lab.suggest_next_experiments(result);
    log_info("Suggested next experiments:");
    for (const auto& suggestion : suggestions) {
        log_info("• {}", suggestion);
    }
    
    return 0;
}
```

---

**ECScope API Reference: Clear interfaces, educational transparency, performance excellence in every call.**

*The API is designed to be discoverable, educational, and performant—making complex systems accessible while maintaining production-quality performance.*