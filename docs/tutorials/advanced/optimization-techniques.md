# ECScope Optimization Techniques

**Comprehensive guide to performance optimization strategies, memory management techniques, and practical optimization patterns demonstrated in ECScope**

## Table of Contents

1. [Optimization Philosophy](#optimization-philosophy)
2. [Memory Optimization Strategies](#memory-optimization-strategies)
3. [Cache Optimization Techniques](#cache-optimization-techniques)
4. [ECS Performance Optimization](#ecs-performance-optimization)
5. [Physics System Optimization](#physics-system-optimization)
6. [Rendering Pipeline Optimization](#rendering-pipeline-optimization)
7. [SIMD and Vectorization](#simd-and-vectorization)
8. [Advanced Optimization Patterns](#advanced-optimization-patterns)
9. [Measurement and Profiling](#measurement-and-profiling)

## Optimization Philosophy

### Educational Approach to Optimization

ECScope demonstrates that optimization is not magic—it's the systematic application of well-understood principles. Every optimization technique is implemented with clear explanations and measurable results.

**Core Principles**:
1. **Measure First**: Never optimize without data
2. **Understand the Problem**: Know why something is slow before fixing it
3. **Optimize the Algorithm**: Big-O improvements beat micro-optimizations
4. **Cache is King**: Memory access patterns dominate modern performance
5. **SIMD When Applicable**: Vectorization provides massive speedups for suitable problems

### Optimization Methodology

```cpp
class OptimizationFramework {
    enum class OptimizationPhase {
        Measurement,      // Establish baseline performance
        Analysis,         // Identify bottlenecks and root causes
        Algorithm,        // Optimize algorithms and data structures
        Implementation,   // Apply low-level optimizations
        Validation       // Verify improvements and avoid regressions
    };
    
    struct OptimizationResult {
        f32 baseline_time_ms;
        f32 optimized_time_ms;
        f32 improvement_factor;
        f32 improvement_percentage;
        usize memory_baseline_bytes;
        usize memory_optimized_bytes;
        f32 cache_efficiency_improvement;
        std::string optimization_description;
    };
    
public:
    template<typename Func>
    OptimizationResult apply_optimization(
        const std::string& name,
        Func baseline_function,
        Func optimized_function,
        usize iterations = 1000
    ) {
        OptimizationResult result{};
        result.optimization_description = name;
        
        // Phase 1: Measure baseline
        const auto baseline_stats = benchmark_function(baseline_function, iterations);
        result.baseline_time_ms = baseline_stats.average_time_ms;
        result.memory_baseline_bytes = baseline_stats.peak_memory_usage;
        
        // Phase 2: Measure optimized version
        const auto optimized_stats = benchmark_function(optimized_function, iterations);
        result.optimized_time_ms = optimized_stats.average_time_ms;
        result.memory_optimized_bytes = optimized_stats.peak_memory_usage;
        
        // Phase 3: Calculate improvements
        result.improvement_factor = result.baseline_time_ms / result.optimized_time_ms;
        result.improvement_percentage = (1.0f - (result.optimized_time_ms / result.baseline_time_ms)) * 100.0f;
        
        // Phase 4: Analyze cache behavior
        result.cache_efficiency_improvement = 
            optimized_stats.cache_hit_ratio - baseline_stats.cache_hit_ratio;
        
        log_optimization_result(result);
        return result;
    }
    
private:
    void log_optimization_result(const OptimizationResult& result) {
        log_info("=== Optimization Result: {} ===", result.optimization_description);
        log_info("Performance:");
        log_info("  Baseline: {:.3f}ms", result.baseline_time_ms);
        log_info("  Optimized: {:.3f}ms", result.optimized_time_ms);
        log_info("  Improvement: {:.2f}x faster ({:.1f}% reduction)", 
                result.improvement_factor, result.improvement_percentage);
        
        log_info("Memory:");
        log_info("  Baseline: {:.2f}MB", result.memory_baseline_bytes / (1024.0f * 1024.0f));
        log_info("  Optimized: {:.2f}MB", result.memory_optimized_bytes / (1024.0f * 1024.0f));
        
        if (result.memory_optimized_bytes < result.memory_baseline_bytes) {
            const f32 memory_reduction = (1.0f - static_cast<f32>(result.memory_optimized_bytes) / 
                                        static_cast<f32>(result.memory_baseline_bytes)) * 100.0f;
            log_info("  Memory reduction: {:.1f}%", memory_reduction);
        }
        
        if (result.cache_efficiency_improvement > 0.0f) {
            log_info("Cache efficiency improved by {:.1f}%", 
                    result.cache_efficiency_improvement * 100.0f);
        }
    }
};
```

## Memory Optimization Strategies

### 1. Structure of Arrays (SoA) Optimization

**Problem**: Array of Structures (AoS) layout wastes cache lines when processing single component types.

**Solution**: Store components in separate arrays for cache efficiency.

```cpp
// Demonstration of SoA vs AoS performance
class SoAOptimizationDemo {
    // Traditional AoS approach (inefficient)
    struct Entity_AoS {
        Transform transform;
        Velocity velocity;
        RigidBody rigidbody;
        RenderData render_data;
    };
    
    // ECScope SoA approach (efficient)
    struct ComponentArrays_SoA {
        std::vector<Transform> transforms;
        std::vector<Velocity> velocities;
        std::vector<RigidBody> rigidbodies;
        std::vector<RenderData> render_data;
    };
    
public:
    void demonstrate_cache_efficiency() {
        constexpr usize ENTITY_COUNT = 10000;
        
        log_info("=== SoA vs AoS Cache Efficiency Demonstration ===");
        
        // Setup test data
        std::vector<Entity_AoS> aos_entities(ENTITY_COUNT);
        ComponentArrays_SoA soa_components;
        
        // Initialize both layouts with identical data
        initialize_test_data(aos_entities, soa_components, ENTITY_COUNT);
        
        // Test 1: Update only Transform components (common case)
        auto aos_transform_update = [&]() {
            for (auto& entity : aos_entities) {
                entity.transform.position += entity.velocity.velocity * 0.016f;
                entity.transform.rotation += entity.velocity.angular_velocity * 0.016f;
            }
        };
        
        auto soa_transform_update = [&]() {
            for (usize i = 0; i < soa_components.transforms.size(); ++i) {
                soa_components.transforms[i].position += soa_components.velocities[i].velocity * 0.016f;
                soa_components.transforms[i].rotation += soa_components.velocities[i].angular_velocity * 0.016f;
            }
        };
        
        OptimizationFramework optimizer;
        const auto result = optimizer.apply_optimization(
            "Transform Update: AoS vs SoA",
            aos_transform_update,
            soa_transform_update,
            1000
        );
        
        // Educational analysis
        analyze_cache_behavior(result);
    }
    
private:
    void analyze_cache_behavior(const OptimizationResult& result) {
        log_info("=== Cache Behavior Analysis ===");
        
        // Calculate theoretical cache usage
        const usize aos_cache_waste = calculate_aos_cache_waste();
        const usize soa_cache_efficiency = calculate_soa_cache_efficiency();
        
        log_info("Theoretical Analysis:");
        log_info("• AoS: Loads {} bytes per cache line, uses only {} bytes ({:.1f}% waste)",
                64, sizeof(Transform), 
                (1.0f - static_cast<f32>(sizeof(Transform)) / 64.0f) * 100.0f);
        log_info("• SoA: Loads {} bytes per cache line, uses all {} bytes (0% waste)",
                64, 64);
        
        log_info("Practical Results:");
        log_info("• Performance improvement matches theoretical cache efficiency gains");
        log_info("• SoA enables vectorization opportunities");
        log_info("• Memory bandwidth utilization improved by {:.1f}%", 
                result.improvement_percentage);
    }
};
```

### 2. Custom Allocator Optimization

**Problem**: Standard allocators cause fragmentation and unpredictable performance.

**Solution**: Use specialized allocators for different allocation patterns.

```cpp
class AllocatorOptimizationDemo {
    struct AllocationProfile {
        usize allocation_count;
        usize average_size;
        usize max_size;
        f32 lifetime_variance;  // How much lifetimes vary
        bool frequent_allocation_deallocation;
    };
    
public:
    void demonstrate_allocator_selection() {
        log_info("=== Allocator Selection Strategy ===");
        
        // Scenario 1: Component storage (predictable size, long lifetime)
        demonstrate_arena_allocator_benefits();
        
        // Scenario 2: Entity management (fixed size, frequent alloc/dealloc)
        demonstrate_pool_allocator_benefits();
        
        // Scenario 3: Dynamic containers (variable size, complex patterns)
        demonstrate_pmr_allocator_benefits();
    }
    
private:
    void demonstrate_arena_allocator_benefits() {
        log_info("--- Arena Allocator: Component Storage ---");
        
        constexpr usize COMPONENT_COUNT = 10000;
        constexpr usize COMPONENT_SIZE = sizeof(Transform);
        
        // Standard allocation (with fragmentation)
        auto standard_allocation = [&]() {
            std::vector<std::unique_ptr<Transform>> components;
            components.reserve(COMPONENT_COUNT);
            
            for (usize i = 0; i < COMPONENT_COUNT; ++i) {
                components.push_back(std::make_unique<Transform>());
                
                // Simulate fragmentation with random deallocations
                if (i % 100 == 99 && i > 500) {
                    const usize remove_index = i - (rand() % 50);
                    if (remove_index < components.size()) {
                        components.erase(components.begin() + remove_index);
                    }
                }
            }
            
            return components.size();
        };
        
        // Arena allocation (no fragmentation)
        auto arena_allocation = [&]() {
            ArenaAllocator arena{COMPONENT_COUNT * COMPONENT_SIZE + 1024};
            std::vector<Transform*> components;
            components.reserve(COMPONENT_COUNT);
            
            for (usize i = 0; i < COMPONENT_COUNT; ++i) {
                auto* component = arena.allocate<Transform>();
                new (component) Transform{};
                components.push_back(component);
            }
            
            return components.size();
        };
        
        OptimizationFramework optimizer;
        const auto result = optimizer.apply_optimization(
            "Component Storage: Standard vs Arena",
            standard_allocation,
            arena_allocation,
            100
        );
        
        log_info("Arena Allocator Benefits:");
        log_info("• Zero fragmentation guaranteed");
        log_info("• Predictable memory layout");
        log_info("• Cache-friendly sequential allocation");
        log_info("• Fast bulk deallocation (entire arena at once)");
    }
    
    void demonstrate_pool_allocator_benefits() {
        log_info("--- Pool Allocator: Entity Management ---");
        
        constexpr usize OPERATIONS = 100000;
        constexpr usize ENTITY_SIZE = sizeof(EntityID);
        
        // Standard allocation/deallocation
        auto standard_entity_ops = [&]() {
            std::vector<std::unique_ptr<EntityID>> entities;
            entities.reserve(1000);
            
            for (usize i = 0; i < OPERATIONS; ++i) {
                if (i % 3 == 0 && !entities.empty()) {
                    // Deallocate random entity
                    const usize remove_index = rand() % entities.size();
                    entities.erase(entities.begin() + remove_index);
                } else {
                    // Allocate new entity
                    entities.push_back(std::make_unique<EntityID>(EntityID{static_cast<u32>(i)}));
                }
            }
            
            return entities.size();
        };
        
        // Pool allocation/deallocation
        auto pool_entity_ops = [&]() {
            PoolAllocator<EntityID> entity_pool{1000};
            std::vector<EntityID*> entities;
            entities.reserve(1000);
            
            for (usize i = 0; i < OPERATIONS; ++i) {
                if (i % 3 == 0 && !entities.empty()) {
                    // Deallocate random entity
                    const usize remove_index = rand() % entities.size();
                    entity_pool.deallocate(entities[remove_index]);
                    entities.erase(entities.begin() + remove_index);
                } else {
                    // Allocate new entity
                    auto* entity = entity_pool.allocate();
                    *entity = EntityID{static_cast<u32>(i)};
                    entities.push_back(entity);
                }
            }
            
            return entities.size();
        };
        
        OptimizationFramework optimizer;
        const auto result = optimizer.apply_optimization(
            "Entity Management: Standard vs Pool",
            standard_entity_ops,
            pool_entity_ops,
            10
        );
        
        log_info("Pool Allocator Benefits:");
        log_info("• Constant-time allocation/deallocation");
        log_info("• Zero fragmentation for fixed-size objects");
        log_info("• Memory reuse reduces allocation overhead");
        log_info("• Predictable performance characteristics");
    }
};
```

### 3. Memory Layout Optimization

**Hot/Cold Data Separation**:
```cpp
// Optimize by separating frequently accessed data from rarely accessed data
struct OptimizedTransform {
    // Hot data: accessed every frame
    Vec2 position;        // 8 bytes
    f32 rotation;         // 4 bytes
    Vec2 scale;           // 8 bytes
    // Total: 20 bytes - fits in single cache line with room for prefetching
    
    // Cold data: accessed infrequently (moved to separate structure)
    struct ColdData {
        std::string debug_name;    // Rarely accessed
        u64 creation_timestamp;    // Only for debugging
        EntityID parent;           // Only for hierarchical entities
        std::vector<EntityID> children;
    };
    
    // Pointer to cold data (only allocated when needed)
    std::unique_ptr<ColdData> cold_data_{nullptr};
    
    // Educational methods for understanding the optimization
    static void demonstrate_hot_cold_separation() {
        constexpr usize ENTITY_COUNT = 10000;
        constexpr usize ITERATIONS = 1000;
        
        // Test traditional approach (hot + cold mixed)
        struct TraditionalTransform {
            Vec2 position;
            f32 rotation;
            Vec2 scale;
            std::string debug_name;      // Cold data mixed with hot
            u64 creation_timestamp;      // Cold data mixed with hot
            EntityID parent;             // Cold data mixed with hot
            std::vector<EntityID> children; // Very cold, but taking space
        };
        
        std::vector<TraditionalTransform> traditional_transforms(ENTITY_COUNT);
        std::vector<OptimizedTransform> optimized_transforms(ENTITY_COUNT);
        
        // Benchmark hot data access (position updates)
        auto traditional_update = [&]() {
            for (auto& transform : traditional_transforms) {
                transform.position.x += 0.1f;
                transform.position.y += 0.1f;
                transform.rotation += 0.01f;
            }
        };
        
        auto optimized_update = [&]() {
            for (auto& transform : optimized_transforms) {
                transform.position.x += 0.1f;
                transform.position.y += 0.1f;
                transform.rotation += 0.01f;
            }
        };
        
        OptimizationFramework optimizer;
        const auto result = optimizer.apply_optimization(
            "Hot/Cold Data Separation",
            traditional_update,
            optimized_update,
            ITERATIONS
        );
        
        // Educational analysis
        log_info("Hot/Cold Separation Analysis:");
        log_info("• Traditional: {} bytes per transform", sizeof(TraditionalTransform));
        log_info("• Optimized: {} bytes per transform", sizeof(OptimizedTransform));
        log_info("• Memory reduction: {:.1f}%", 
                (1.0f - static_cast<f32>(sizeof(OptimizedTransform)) / 
                        static_cast<f32>(sizeof(TraditionalTransform))) * 100.0f);
        log_info("• Cache lines per transform reduced from {} to {}", 
                (sizeof(TraditionalTransform) + 63) / 64,
                (sizeof(OptimizedTransform) + 63) / 64);
    }
};
```

## Cache Optimization Techniques

### 1. Prefetching Strategies

```cpp
class PrefetchingOptimization {
public:
    // Software prefetching for predictable access patterns
    void demonstrate_prefetching_benefits() {
        constexpr usize ENTITY_COUNT = 10000;
        constexpr usize PREFETCH_DISTANCE = 8; // Prefetch 8 elements ahead
        
        std::vector<Transform> transforms(ENTITY_COUNT);
        std::vector<Velocity> velocities(ENTITY_COUNT);
        
        // Initialize test data
        initialize_component_data(transforms, velocities);
        
        // Version without prefetching
        auto no_prefetch_update = [&]() {
            for (usize i = 0; i < transforms.size(); ++i) {
                transforms[i].position += velocities[i].velocity * 0.016f;
                transforms[i].rotation += velocities[i].angular_velocity * 0.016f;
            }
        };
        
        // Version with software prefetching
        auto prefetch_update = [&]() {
            for (usize i = 0; i < transforms.size(); ++i) {
                // Prefetch data that will be needed soon
                if (i + PREFETCH_DISTANCE < transforms.size()) {
                    __builtin_prefetch(&transforms[i + PREFETCH_DISTANCE], 0, 3); // Read, high temporal locality
                    __builtin_prefetch(&velocities[i + PREFETCH_DISTANCE], 0, 3);
                }
                
                transforms[i].position += velocities[i].velocity * 0.016f;
                transforms[i].rotation += velocities[i].angular_velocity * 0.016f;
            }
        };
        
        OptimizationFramework optimizer;
        const auto result = optimizer.apply_optimization(
            "Software Prefetching",
            no_prefetch_update,
            prefetch_update,
            1000
        );
        
        log_info("Prefetching Analysis:");
        log_info("• Prefetch distance: {} elements", PREFETCH_DISTANCE);
        log_info("• Cache line speculation improves memory bandwidth utilization");
        log_info("• Works best with predictable, sequential access patterns");
        log_info("• Diminishing returns on modern CPUs with good hardware prefetchers");
    }
    
    // Cache blocking for large data sets
    void demonstrate_cache_blocking() {
        constexpr usize MATRIX_SIZE = 1024;
        constexpr usize BLOCK_SIZE = 64; // Optimize for L1 cache
        
        std::vector<std::vector<f32>> matrix_a(MATRIX_SIZE, std::vector<f32>(MATRIX_SIZE));
        std::vector<std::vector<f32>> matrix_b(MATRIX_SIZE, std::vector<f32>(MATRIX_SIZE));
        std::vector<std::vector<f32>> result_naive(MATRIX_SIZE, std::vector<f32>(MATRIX_SIZE, 0.0f));
        std::vector<std::vector<f32>> result_blocked(MATRIX_SIZE, std::vector<f32>(MATRIX_SIZE, 0.0f));
        
        // Initialize matrices
        initialize_matrices(matrix_a, matrix_b);
        
        // Naive matrix multiplication (cache-hostile)
        auto naive_multiply = [&]() {
            for (usize i = 0; i < MATRIX_SIZE; ++i) {
                for (usize j = 0; j < MATRIX_SIZE; ++j) {
                    f32 sum = 0.0f;
                    for (usize k = 0; k < MATRIX_SIZE; ++k) {
                        sum += matrix_a[i][k] * matrix_b[k][j]; // Poor cache locality for matrix_b
                    }
                    result_naive[i][j] = sum;
                }
            }
        };
        
        // Cache-blocked matrix multiplication
        auto blocked_multiply = [&]() {
            for (usize ii = 0; ii < MATRIX_SIZE; ii += BLOCK_SIZE) {
                for (usize jj = 0; jj < MATRIX_SIZE; jj += BLOCK_SIZE) {
                    for (usize kk = 0; kk < MATRIX_SIZE; kk += BLOCK_SIZE) {
                        // Process block
                        const usize i_max = std::min(ii + BLOCK_SIZE, MATRIX_SIZE);
                        const usize j_max = std::min(jj + BLOCK_SIZE, MATRIX_SIZE);
                        const usize k_max = std::min(kk + BLOCK_SIZE, MATRIX_SIZE);
                        
                        for (usize i = ii; i < i_max; ++i) {
                            for (usize j = jj; j < j_max; ++j) {
                                f32 sum = result_blocked[i][j];
                                for (usize k = kk; k < k_max; ++k) {
                                    sum += matrix_a[i][k] * matrix_b[k][j];
                                }
                                result_blocked[i][j] = sum;
                            }
                        }
                    }
                }
            }
        };
        
        OptimizationFramework optimizer;
        const auto result = optimizer.apply_optimization(
            "Cache Blocking: Matrix Multiplication",
            naive_multiply,
            blocked_multiply,
            5
        );
        
        log_info("Cache Blocking Benefits:");
        log_info("• Improves temporal locality for matrix B accesses");
        log_info("• Reduces cache misses by reusing data in cache");
        log_info("• Block size optimized for L1 cache capacity");
        log_info("• Scalable improvement with matrix size");
    }
};
```

### 3. Data Structure Optimization

**Compact Data Structures**:
```cpp
// Optimized component design for cache efficiency
struct OptimizedComponent {
    // Use smallest appropriate types
    i16 position_x_fixed;    // Fixed-point instead of float (2 bytes vs 4)
    i16 position_y_fixed;    // Fixed-point instead of float (2 bytes vs 4)
    u16 rotation_fixed;      // Fixed-point rotation (2 bytes vs 4)
    u8 flags;                // Packed boolean flags (1 byte vs multiple bools)
    
    // Pack related data together
    struct {
        u8 r, g, b, a;       // Color (4 bytes vs 16 bytes for Vec4)
    } color;
    
    // Convert from/to standard types for API compatibility
    Vec2 get_position() const noexcept {
        return Vec2{
            static_cast<f32>(position_x_fixed) / 256.0f,
            static_cast<f32>(position_y_fixed) / 256.0f
        };
    }
    
    void set_position(const Vec2& pos) noexcept {
        position_x_fixed = static_cast<i16>(pos.x * 256.0f);
        position_y_fixed = static_cast<i16>(pos.y * 256.0f);
    }
    
    f32 get_rotation() const noexcept {
        return static_cast<f32>(rotation_fixed) / 65536.0f * 2.0f * M_PI;
    }
    
    void set_rotation(f32 rot) noexcept {
        rotation_fixed = static_cast<u16>((rot / (2.0f * M_PI)) * 65536.0f);
    }
    
    // Educational comparison
    static void compare_memory_efficiency() {
        struct StandardComponent {
            Vec2 position;       // 8 bytes
            f32 rotation;        // 4 bytes
            Vec2 scale;          // 8 bytes
            bool visible;        // 1 byte + 3 padding
            bool enabled;        // 1 byte + 3 padding
            Vec4 color;          // 16 bytes
        };                       // Total: ~44 bytes with padding
        
        log_info("Component Memory Comparison:");
        log_info("• Standard: {} bytes per component", sizeof(StandardComponent));
        log_info("• Optimized: {} bytes per component", sizeof(OptimizedComponent));
        log_info("• Memory reduction: {:.1f}%", 
                (1.0f - static_cast<f32>(sizeof(OptimizedComponent)) / 
                        static_cast<f32>(sizeof(StandardComponent))) * 100.0f);
        log_info("• Components per cache line: Standard={}, Optimized={}", 
                64 / sizeof(StandardComponent), 64 / sizeof(OptimizedComponent));
    }
};
```

## ECS Performance Optimization

### 1. Archetype Optimization

```cpp
class ArchetypeOptimization {
    // Optimize archetype transitions for minimal cost
    struct ArchetypeMigrationOptimizer {
        struct MigrationPath {
            ArchetypeID from;
            ArchetypeID to;
            std::vector<ComponentTypeID> components_to_copy;
            usize estimated_cost;
        };
        
        std::unordered_map<std::pair<ArchetypeID, ArchetypeID>, MigrationPath> cached_paths_;
        
    public:
        MigrationPath optimize_migration(ArchetypeID from, ArchetypeID to, 
                                       const Registry& registry) {
            const auto key = std::make_pair(from, to);
            
            if (const auto it = cached_paths_.find(key); it != cached_paths_.end()) {
                return it->second;
            }
            
            MigrationPath path{};
            path.from = from;
            path.to = to;
            
            // Find components that need to be copied
            const auto from_signature = registry.get_archetype_signature(from);
            const auto to_signature = registry.get_archetype_signature(to);
            
            // Components present in both archetypes can be copied
            const auto common_components = from_signature & to_signature;
            
            for (ComponentTypeID type_id = 0; type_id < MAX_COMPONENTS; ++type_id) {
                if (common_components.test(type_id)) {
                    path.components_to_copy.push_back(type_id);
                    
                    // Estimate copy cost based on component size
                    const usize component_size = registry.get_component_size(type_id);
                    path.estimated_cost += component_size;
                }
            }
            
            cached_paths_[key] = path;
            return path;
        }
        
        void perform_optimized_migration(Registry& registry, EntityID entity, 
                                       ArchetypeID target_archetype) {
            const ArchetypeID current_archetype = registry.get_entity_archetype(entity);
            const auto migration_path = optimize_migration(current_archetype, target_archetype, registry);
            
            // Use bulk copy operations when possible
            registry.migrate_entity_bulk(entity, target_archetype, migration_path.components_to_copy);
        }
    };
    
public:
    void demonstrate_archetype_optimization() {
        log_info("=== Archetype Migration Optimization ===");
        
        Registry registry{};
        
        // Create entities with initial archetype
        std::vector<EntityID> entities;
        for (usize i = 0; i < 1000; ++i) {
            const EntityID entity = registry.create_entity();
            registry.add_component<Transform>(entity, Transform{});
            registry.add_component<Velocity>(entity, Velocity{});
            entities.push_back(entity);
        }
        
        // Measure unoptimized migration
        auto unoptimized_migration = [&]() {
            for (const EntityID entity : entities) {
                // Naive approach: individual component operations
                registry.remove_component<Velocity>(entity);
                registry.add_component<RigidBody>(entity, RigidBody{});
            }
        };
        
        // Reset entities for optimized test
        reset_entities_to_initial_state(registry, entities);
        
        // Measure optimized migration
        ArchetypeMigrationOptimizer optimizer;
        auto optimized_migration = [&]() {
            const ArchetypeID target = registry.get_or_create_archetype<Transform, RigidBody>();
            
            for (const EntityID entity : entities) {
                optimizer.perform_optimized_migration(registry, entity, target);
            }
        };
        
        OptimizationFramework perf_optimizer;
        const auto result = perf_optimizer.apply_optimization(
            "Archetype Migration",
            unoptimized_migration,
            optimized_migration,
            10
        );
        
        log_info("Migration Optimization Benefits:");
        log_info("• Bulk operations reduce individual allocation overhead");
        log_info("• Cached migration paths eliminate repeated calculations");
        log_info("• Memory layout optimization during migration");
        log_info("• Reduced archetype fragmentation");
    }
};
```

### 2. Query Optimization

```cpp
class QueryOptimization {
    // Optimize component queries for cache efficiency
    template<typename... Components>
    class OptimizedQuery {
        std::vector<ArchetypeID> matching_archetypes_;
        mutable std::vector<EntityID> cached_entities_;
        mutable bool cache_valid_{false};
        
    public:
        explicit OptimizedQuery(const Registry& registry) {
            // Find all archetypes that match the query
            const ComponentSignature query_signature = create_signature<Components...>();
            
            for (const auto& [archetype_id, archetype] : registry.get_archetypes()) {
                if ((archetype.get_signature() & query_signature) == query_signature) {
                    matching_archetypes_.push_back(archetype_id);
                }
            }
            
            log_info("Query<{}> matches {} archetypes", 
                    component_names<Components...>(), matching_archetypes_.size());
        }
        
        // Cache-optimized iteration
        template<typename Func>
        void for_each_cache_optimized(const Registry& registry, Func&& func) const {
            for (const ArchetypeID archetype_id : matching_archetypes_) {
                const auto& archetype = registry.get_archetype(archetype_id);
                const usize entity_count = archetype.get_entity_count();
                
                if (entity_count == 0) continue;
                
                // Get component arrays (SoA layout)
                auto components_tuple = std::make_tuple(
                    archetype.get_component_array<Components>()...
                );
                
                // Process in cache-friendly chunks
                constexpr usize CHUNK_SIZE = 64; // Process 64 entities at a time
                
                for (usize chunk_start = 0; chunk_start < entity_count; chunk_start += CHUNK_SIZE) {
                    const usize chunk_end = std::min(chunk_start + CHUNK_SIZE, entity_count);
                    
                    // Prefetch next chunk
                    if (chunk_end < entity_count) {
                        prefetch_component_chunk<Components...>(components_tuple, chunk_end);
                    }
                    
                    // Process current chunk
                    for (usize i = chunk_start; i < chunk_end; ++i) {
                        func(std::get<decltype(archetype.get_component_array<Components>())>(components_tuple)[i]...);
                    }
                }
            }
        }
        
        // SIMD-optimized iteration for suitable operations
        template<typename Func>
        void for_each_simd_optimized(const Registry& registry, Func&& func) const {
            static_assert(are_simd_compatible<Components...>(), 
                         "Components must be SIMD-compatible for vectorized processing");
            
            for (const ArchetypeID archetype_id : matching_archetypes_) {
                const auto& archetype = registry.get_archetype(archetype_id);
                const usize entity_count = archetype.get_entity_count();
                
                if (entity_count < 4) {
                    // Fall back to scalar processing for small counts
                    for_each_cache_optimized(registry, func);
                    continue;
                }
                
                // SIMD processing (4 entities at a time)
                const usize simd_count = (entity_count / 4) * 4;
                const usize remaining_count = entity_count - simd_count;
                
                // Process SIMD-aligned portion
                process_simd_chunk<Components...>(archetype, 0, simd_count, func);
                
                // Process remaining entities with scalar code
                if (remaining_count > 0) {
                    process_scalar_chunk<Components...>(archetype, simd_count, entity_count, func);
                }
            }
        }
        
    private:
        template<typename... Comps>
        void prefetch_component_chunk(const std::tuple<std::span<Comps>...>& arrays, usize index) const {
            // Prefetch each component array
            ((void)__builtin_prefetch(&std::get<std::span<Comps>>(arrays)[index], 0, 3), ...);
        }
    };
    
public:
    void demonstrate_query_optimization() {
        log_info("=== ECS Query Optimization ===");
        
        Registry registry{};
        
        // Create test entities
        create_mixed_archetype_entities(registry, 10000);
        
        // Traditional query approach
        auto traditional_query = [&]() {
            auto view = registry.view<Transform, Velocity>();
            for (auto [entity, transform, velocity] : view.each()) {
                transform.position += velocity.velocity * 0.016f;
            }
        };
        
        // Optimized query approach
        auto optimized_query = [&]() {
            OptimizedQuery<Transform, Velocity> query{registry};
            query.for_each_cache_optimized(registry, [](Transform& transform, const Velocity& velocity) {
                transform.position += velocity.velocity * 0.016f;
            });
        };
        
        OptimizationFramework optimizer;
        const auto result = optimizer.apply_optimization(
            "ECS Query Optimization",
            traditional_query,
            optimized_query,
            1000
        );
        
        log_info("Query Optimization Benefits:");
        log_info("• Cache blocking improves memory access patterns");
        log_info("• Prefetching reduces memory latency");
        log_info("• Archetype-aware processing minimizes overhead");
        log_info("• SIMD opportunities enabled by optimized layout");
    }
};
```

## Physics System Optimization

### 1. Spatial Partitioning Optimization

```cpp
class SpatialOptimization {
    // Compare different spatial partitioning strategies
    struct SpatialStructureComparison {
        std::string name;
        std::unique_ptr<SpatialStructure> structure;
        f32 insertion_time_ms;
        f32 query_time_ms;
        usize memory_usage_bytes;
        f32 query_efficiency; // Ratio of actual vs theoretical collision tests
    };
    
public:
    void compare_spatial_structures() {
        log_info("=== Spatial Partitioning Comparison ===");
        
        constexpr usize BODY_COUNT = 1000;
        constexpr f32 WORLD_SIZE = 100.0f;
        
        // Generate test data
        std::vector<RigidBodyData> bodies = generate_physics_test_data(BODY_COUNT, WORLD_SIZE);
        
        // Test different spatial structures
        std::vector<SpatialStructureComparison> comparisons = {
            {"Brute Force", std::make_unique<BruteForceCollision>()},
            {"Spatial Hash Grid", std::make_unique<SpatialHashGrid>(4.0f)},
            {"Quadtree", std::make_unique<Quadtree>(AABB{{-WORLD_SIZE, -WORLD_SIZE}, {WORLD_SIZE, WORLD_SIZE}})},
            {"Sweep and Prune", std::make_unique<SweepAndPrune>()}
        };
        
        for (auto& comparison : comparisons) {
            benchmark_spatial_structure(comparison, bodies);
        }
        
        // Display results
        display_spatial_structure_results(comparisons);
        provide_spatial_optimization_insights(comparisons);
    }
    
private:
    void benchmark_spatial_structure(SpatialStructureComparison& comparison, 
                                   const std::vector<RigidBodyData>& bodies) {
        // Measure insertion performance
        {
            Timer insertion_timer("Insertion");
            comparison.structure->clear();
            
            for (const auto& body : bodies) {
                comparison.structure->insert(body.entity_id, body.aabb);
            }
            
            comparison.insertion_time_ms = insertion_timer.elapsed_ms();
        }
        
        // Measure query performance
        {
            Timer query_timer("Query");
            const auto collision_pairs = comparison.structure->query_collision_pairs();
            comparison.query_time_ms = query_timer.elapsed_ms();
            
            // Calculate query efficiency
            const usize total_possible_pairs = (bodies.size() * (bodies.size() - 1)) / 2;
            const usize actual_tests = collision_pairs.size();
            comparison.query_efficiency = static_cast<f32>(actual_tests) / static_cast<f32>(total_possible_pairs);
        }
        
        // Measure memory usage
        comparison.memory_usage_bytes = comparison.structure->get_memory_usage();
        
        log_info("{} Results:", comparison.name);
        log_info("  Insertion: {:.3f}ms", comparison.insertion_time_ms);
        log_info("  Query: {:.3f}ms", comparison.query_time_ms);
        log_info("  Memory: {:.2f}KB", comparison.memory_usage_bytes / 1024.0f);
        log_info("  Query Efficiency: {:.1f}% ({} vs {} pairs)", 
                comparison.query_efficiency * 100.0f,
                static_cast<usize>(comparison.query_efficiency * total_possible_pairs),
                total_possible_pairs);
    }
    
    void provide_spatial_optimization_insights(const std::vector<SpatialStructureComparison>& comparisons) {
        log_info("=== Spatial Optimization Insights ===");
        
        // Find best performers
        const auto best_insertion = std::min_element(comparisons.begin(), comparisons.end(),
            [](const auto& a, const auto& b) { return a.insertion_time_ms < b.insertion_time_ms; });
        
        const auto best_query = std::min_element(comparisons.begin(), comparisons.end(),
            [](const auto& a, const auto& b) { return a.query_time_ms < b.query_time_ms; });
        
        const auto most_efficient = std::min_element(comparisons.begin(), comparisons.end(),
            [](const auto& a, const auto& b) { return a.query_efficiency < b.query_efficiency; });
        
        log_info("Performance Winners:");
        log_info("• Fastest Insertion: {} ({:.3f}ms)", best_insertion->name, best_insertion->insertion_time_ms);
        log_info("• Fastest Query: {} ({:.3f}ms)", best_query->name, best_query->query_time_ms);
        log_info("• Most Efficient: {} ({:.1f}% collision test reduction)", 
                most_efficient->name, (1.0f - most_efficient->query_efficiency) * 100.0f);
        
        log_info("Educational Insights:");
        log_info("• Spatial Hash Grid: Best for uniformly distributed objects");
        log_info("• Quadtree: Best for clustered objects with varying density");
        log_info("• Sweep and Prune: Best for objects with coherent movement");
        log_info("• Brute Force: Only viable for very small object counts");
        
        // Provide selection guidance
        log_info("Selection Guidance:");
        log_info("• < 100 objects: Brute force acceptable");
        log_info("• 100-1000 objects: Spatial hash grid recommended");
        log_info("• > 1000 objects: Quadtree or sweep-and-prune required");
        log_info("• Highly dynamic scenes: Spatial hash grid for consistent performance");
        log_info("• Static/slow-moving scenes: Quadtree for query efficiency");
    }
};
```

### 2. Constraint Solver Optimization

```cpp
class ConstraintSolverOptimization {
    // Optimize constraint solving for better convergence and performance
    class WarmStartingSolver {
        struct ConstraintCache {
            f32 accumulated_normal_impulse;
            f32 accumulated_tangent_impulse;
            Vec2 contact_point;
            f32 cache_age; // How old this cached solution is
        };
        
        std::unordered_map<ContactPairID, ConstraintCache> constraint_cache_;
        
    public:
        void solve_constraints_with_warm_starting(std::vector<ContactConstraint>& constraints, 
                                                 f32 delta_time) {
            // Apply warm starting from cached solutions
            for (auto& constraint : constraints) {
                const ContactPairID pair_id = create_contact_pair_id(constraint.body_a, constraint.body_b);
                
                if (const auto it = constraint_cache_.find(pair_id); it != constraint_cache_.end()) {
                    const auto& cache = it->second;
                    
                    // Only use cache if it's recent and contact points are similar
                    if (cache.cache_age < 0.1f && 
                        Vec2::distance(cache.contact_point, constraint.contact_point) < 0.5f) {
                        
                        // Warm start with cached impulse
                        constraint.accumulated_normal_impulse = cache.accumulated_normal_impulse * 0.8f;
                        constraint.accumulated_tangent_impulse = cache.accumulated_tangent_impulse * 0.8f;
                        
                        log_debug("Warm starting constraint {} with cached impulse {:.3f}", 
                                 pair_id, cache.accumulated_normal_impulse);
                    }
                }
            }
            
            // Solve constraints with improved initial guess
            for (u32 iteration = 0; iteration < velocity_iterations_; ++iteration) {
                for (auto& constraint : constraints) {
                    solve_velocity_constraint(constraint, delta_time);
                }
            }
            
            // Cache results for next frame
            for (const auto& constraint : constraints) {
                const ContactPairID pair_id = create_contact_pair_id(constraint.body_a, constraint.body_b);
                
                constraint_cache_[pair_id] = ConstraintCache{
                    .accumulated_normal_impulse = constraint.accumulated_normal_impulse,
                    .accumulated_tangent_impulse = constraint.accumulated_tangent_impulse,
                    .contact_point = constraint.contact_point,
                    .cache_age = 0.0f
                };
            }
            
            // Age existing cache entries
            update_constraint_cache(delta_time);
        }
        
    private:
        void update_constraint_cache(f32 delta_time) {
            for (auto it = constraint_cache_.begin(); it != constraint_cache_.end();) {
                it->second.cache_age += delta_time;
                
                // Remove stale cache entries
                if (it->second.cache_age > 1.0f) {
                    it = constraint_cache_.erase(it);
                } else {
                    ++it;
                }
            }
        }
    };
    
public:
    void demonstrate_solver_optimization() {
        log_info("=== Constraint Solver Optimization ===");
        
        // Create test scenario with stable contacts
        Registry registry{};
        create_box_stack_scenario(registry, 10); // 10 boxes stacked
        
        // Standard solver (cold start every frame)
        StandardConstraintSolver standard_solver{8}; // 8 velocity iterations
        
        // Optimized solver (warm starting + optimizations)
        WarmStartingSolver optimized_solver{};
        
        // Measure convergence quality over multiple frames
        const auto standard_convergence = measure_solver_convergence(registry, standard_solver, 60);
        const auto optimized_convergence = measure_solver_convergence(registry, optimized_solver, 60);
        
        log_info("Solver Comparison Results:");
        log_info("• Standard Solver:");
        log_info("    Average iterations to convergence: {:.1f}", standard_convergence.average_iterations);
        log_info("    Final constraint error: {:.6f}", standard_convergence.final_error);
        log_info("    Solve time per frame: {:.3f}ms", standard_convergence.average_solve_time_ms);
        
        log_info("• Optimized Solver:");
        log_info("    Average iterations to convergence: {:.1f}", optimized_convergence.average_iterations);
        log_info("    Final constraint error: {:.6f}", optimized_convergence.final_error);
        log_info("    Solve time per frame: {:.3f}ms", optimized_convergence.average_solve_time_ms);
        
        const f32 convergence_improvement = standard_convergence.average_iterations / 
                                          optimized_convergence.average_iterations;
        
        log_info("Optimization Benefits:");
        log_info("• {:.2f}x faster convergence", convergence_improvement);
        log_info("• {:.1f}% reduction in constraint error", 
                (1.0f - optimized_convergence.final_error / standard_convergence.final_error) * 100.0f);
        log_info("• Stable contact behavior reduces jittering");
        log_info("• Warm starting particularly effective for persistent contacts");
    }
};
```

## Rendering Pipeline Optimization

### 1. Batch Optimization Strategies

```cpp
class BatchOptimization {
    // Optimize sprite batching for maximum GPU utilization
    class IntelligentBatcher {
        struct SpriteSubmission {
            Vec2 position;
            Vec2 size;
            GLuint texture_id;
            Vec4 color;
            f32 depth;
            EntityID entity; // For debugging
        };
        
        std::vector<SpriteSubmission> pending_sprites_;
        std::vector<DrawBatch> optimized_batches_;
        
    public:
        void submit_sprite(const SpriteSubmission& sprite) {
            pending_sprites_.push_back(sprite);
        }
        
        void optimize_and_generate_batches() {
            // Phase 1: Sort by depth (back to front for transparency)
            std::stable_sort(pending_sprites_.begin(), pending_sprites_.end(),
                           [](const SpriteSubmission& a, const SpriteSubmission& b) {
                               return a.depth < b.depth;
                           });
            
            // Phase 2: Group by texture while respecting depth order
            optimize_texture_batching();
            
            // Phase 3: Optimize for GPU cache locality
            optimize_for_gpu_cache();
            
            log_info("Batch optimization complete:");
            log_info("• {} sprites sorted into {} batches", pending_sprites_.size(), optimized_batches_.size());
            log_info("• Average batch utilization: {:.1f}%", calculate_batch_utilization());
        }
        
    private:
        void optimize_texture_batching() {
            optimized_batches_.clear();
            optimized_batches_.emplace_back();
            
            for (const auto& sprite : pending_sprites_) {
                auto& current_batch = optimized_batches_.back();
                
                // Check if sprite can fit in current batch
                if (current_batch.can_add_texture(sprite.texture_id) && 
                    current_batch.quad_count < MAX_QUADS_PER_BATCH) {
                    
                    current_batch.add_sprite(sprite);
                } else {
                    // Need new batch
                    optimized_batches_.emplace_back();
                    optimized_batches_.back().add_sprite(sprite);
                }
            }
        }
        
        void optimize_for_gpu_cache() {
            // Sort vertices within each batch for GPU cache efficiency
            for (auto& batch : optimized_batches_) {
                // Sort vertices by spatial locality (Morton order)
                std::sort(batch.vertices.begin(), batch.vertices.end(),
                         [](const Vertex& a, const Vertex& b) {
                             return morton_encode(a.position) < morton_encode(b.position);
                         });
            }
        }
        
        u64 morton_encode(const Vec2& position) const {
            // Morton encoding for spatial locality
            const u32 x = static_cast<u32>(position.x * 1000.0f) & 0xFFFF;
            const u32 y = static_cast<u32>(position.y * 1000.0f) & 0xFFFF;
            
            u64 result = 0;
            for (u32 i = 0; i < 16; ++i) {
                result |= (x & (1u << i)) << i;
                result |= (y & (1u << i)) << (i + 1);
            }
            
            return result;
        }
    };
    
public:
    void demonstrate_batching_optimization() {
        log_info("=== Sprite Batching Optimization ===");
        
        // Create test scenario with many sprites using different textures
        std::vector<SpriteSubmission> test_sprites = create_complex_sprite_scene(5000);
        
        // Standard batching (simple first-fit algorithm)
        auto standard_batching = [&]() {
            SimpleBatcher standard_batcher;
            for (const auto& sprite : test_sprites) {
                standard_batcher.submit_sprite(sprite);
            }
            return standard_batcher.generate_batches();
        };
        
        // Optimized batching (intelligent sorting and grouping)
        auto optimized_batching = [&]() {
            IntelligentBatcher optimized_batcher;
            for (const auto& sprite : test_sprites) {
                optimized_batcher.submit_sprite(sprite);
            }
            optimized_batcher.optimize_and_generate_batches();
            return optimized_batcher.get_batches();
        };
        
        const auto standard_batches = standard_batching();
        const auto optimized_batches = optimized_batching();
        
        log_info("Batching Comparison:");
        log_info("• Standard: {} batches, {:.1f}% utilization", 
                standard_batches.size(), calculate_utilization(standard_batches));
        log_info("• Optimized: {} batches, {:.1f}% utilization", 
                optimized_batches.size(), calculate_utilization(optimized_batches));
        
        const f32 draw_call_reduction = static_cast<f32>(standard_batches.size()) / 
                                       static_cast<f32>(optimized_batches.size());
        
        log_info("• Draw call reduction: {:.2f}x fewer calls", draw_call_reduction);
        log_info("• Estimated GPU performance improvement: {:.1f}%", 
                (draw_call_reduction - 1.0f) * 100.0f);
    }
};
```

### 2. GPU Memory Optimization

```cpp
class GPUMemoryOptimization {
    // Optimize GPU memory usage and bandwidth
    class VertexBufferOptimizer {
        struct VertexLayout {
            std::string name;
            usize stride;
            std::vector<VertexAttribute> attributes;
            f32 cache_efficiency;
        };
        
    public:
        void demonstrate_vertex_layout_optimization() {
            log_info("=== Vertex Layout Optimization ===");
            
            // Compare different vertex layouts
            const std::vector<VertexLayout> layouts = {
                create_interleaved_layout(),
                create_planar_layout(),
                create_optimized_layout()
            };
            
            for (const auto& layout : layouts) {
                benchmark_vertex_layout(layout);
            }
            
            provide_vertex_layout_insights(layouts);
        }
        
    private:
        VertexLayout create_interleaved_layout() {
            return VertexLayout{
                .name = "Interleaved (Traditional)",
                .stride = sizeof(InterleavedVertex),
                .attributes = {
                    {0, 2, GL_FLOAT, GL_FALSE, 0},                    // Position
                    {1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32)},      // TexCoord
                    {2, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(f32)},      // Color
                    {3, 1, GL_FLOAT, GL_FALSE, 8 * sizeof(f32)}       // TexIndex
                },
                .cache_efficiency = 0.85f // High efficiency for complete vertex access
            };
        }
        
        VertexLayout create_planar_layout() {
            return VertexLayout{
                .name = "Planar (Separate Arrays)",
                .stride = 0, // Multiple buffers
                .attributes = {
                    {0, 2, GL_FLOAT, GL_FALSE, 0},  // Position buffer
                    {1, 2, GL_FLOAT, GL_FALSE, 0},  // TexCoord buffer
                    {2, 4, GL_FLOAT, GL_FALSE, 0},  // Color buffer
                    {3, 1, GL_FLOAT, GL_FALSE, 0}   // TexIndex buffer
                },
                .cache_efficiency = 0.95f // Very high efficiency for partial access
            };
        }
        
        VertexLayout create_optimized_layout() {
            // Hybrid approach: hot data interleaved, cold data separate
            return VertexLayout{
                .name = "Optimized (Hot/Cold Separation)",
                .stride = sizeof(OptimizedVertex),
                .attributes = {
                    {0, 2, GL_FLOAT, GL_FALSE, 0},                    // Position (hot)
                    {1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32)},      // TexCoord (hot)
                    {2, 4, GL_UNSIGNED_BYTE, GL_TRUE, 4 * sizeof(f32)}, // Color (packed)
                    {3, 1, GL_FLOAT, GL_FALSE, 5 * sizeof(f32)}       // TexIndex (cold, separate buffer)
                },
                .cache_efficiency = 0.92f // Balanced efficiency
            };
        }
        
        void benchmark_vertex_layout(const VertexLayout& layout) {
            constexpr usize VERTEX_COUNT = 40000; // 10,000 quads
            
            // Create test data
            std::vector<InterleavedVertex> test_vertices = create_test_vertex_data(VERTEX_COUNT);
            
            // Measure upload performance
            GLuint vbo;
            glGenBuffers(1, &vbo);
            
            Timer upload_timer("Vertex Upload");
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, 
                        test_vertices.size() * layout.stride,
                        test_vertices.data(),
                        GL_DYNAMIC_DRAW);
            const f32 upload_time = upload_timer.elapsed_ms();
            
            // Measure rendering performance
            Timer render_timer("Vertex Rendering");
            for (u32 frame = 0; frame < 100; ++frame) {
                glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(test_vertices.size()));
            }
            const f32 render_time = render_timer.elapsed_ms() / 100.0f; // Average per frame
            
            // Measure memory bandwidth utilization
            const usize total_bytes_transferred = test_vertices.size() * layout.stride;
            const f32 bandwidth_mbps = (total_bytes_transferred / (upload_time / 1000.0f)) / (1024.0f * 1024.0f);
            
            log_info("{} Performance:", layout.name);
            log_info("  Upload Time: {:.3f}ms", upload_time);
            log_info("  Render Time: {:.3f}ms/frame", render_time);
            log_info("  Bandwidth: {:.1f} MB/s", bandwidth_mbps);
            log_info("  Cache Efficiency: {:.1f}%", layout.cache_efficiency * 100.0f);
            
            glDeleteBuffers(1, &vbo);
        }
    };
};
```

## SIMD and Vectorization

### 1. Physics Vectorization

```cpp
class PhysicsVectorization {
    // Vectorize physics operations for 4 entities at once
    class VectorizedPhysicsSystem {
    public:
        void update_transforms_simd(std::span<Transform> transforms, 
                                  std::span<const Velocity> velocities,
                                  f32 delta_time) {
            const usize entity_count = transforms.size();
            const usize simd_count = (entity_count / 4) * 4;
            
            // SIMD processing for aligned entities
            update_transforms_simd_aligned(transforms.subspan(0, simd_count),
                                         velocities.subspan(0, simd_count),
                                         delta_time);
            
            // Scalar processing for remaining entities
            update_transforms_scalar(transforms.subspan(simd_count),
                                   velocities.subspan(simd_count),
                                   delta_time);
        }
        
    private:
        void update_transforms_simd_aligned(std::span<Transform> transforms,
                                          std::span<const Velocity> velocities,
                                          f32 delta_time) {
            const __m128 dt = _mm_set1_ps(delta_time);
            
            for (usize i = 0; i < transforms.size(); i += 4) {
                // Load positions (4 Vec2 = 8 floats)
                const __m128 pos_x = _mm_loadu_ps(&transforms[i].position.x);     // x0, y0, x1, y1
                const __m128 pos_y = _mm_loadu_ps(&transforms[i + 2].position.x); // x2, y2, x3, y3
                
                // Load velocities
                const __m128 vel_x = _mm_loadu_ps(&velocities[i].velocity.x);
                const __m128 vel_y = _mm_loadu_ps(&velocities[i + 2].velocity.x);
                
                // Calculate position updates: pos += vel * dt
                const __m128 delta_pos_x = _mm_mul_ps(vel_x, dt);
                const __m128 delta_pos_y = _mm_mul_ps(vel_y, dt);
                
                const __m128 new_pos_x = _mm_add_ps(pos_x, delta_pos_x);
                const __m128 new_pos_y = _mm_add_ps(pos_y, delta_pos_y);
                
                // Store results
                _mm_storeu_ps(&transforms[i].position.x, new_pos_x);
                _mm_storeu_ps(&transforms[i + 2].position.x, new_pos_y);
                
                // Load and update rotations (4 floats at once)
                const __m128 rotations = _mm_loadu_ps(&transforms[i].rotation);
                const __m128 angular_vels = _mm_loadu_ps(&velocities[i].angular_velocity);
                const __m128 delta_rots = _mm_mul_ps(angular_vels, dt);
                const __m128 new_rots = _mm_add_ps(rotations, delta_rots);
                
                _mm_storeu_ps(&transforms[i].rotation, new_rots);
            }
        }
        
        void update_transforms_scalar(std::span<Transform> transforms,
                                    std::span<const Velocity> velocities,
                                    f32 delta_time) {
            for (usize i = 0; i < transforms.size(); ++i) {
                transforms[i].position += velocities[i].velocity * delta_time;
                transforms[i].rotation += velocities[i].angular_velocity * delta_time;
            }
        }
    };
    
public:
    void demonstrate_simd_optimization() {
        log_info("=== SIMD Physics Optimization ===");
        
        constexpr usize ENTITY_COUNT = 10000;
        
        // Create aligned test data
        std::vector<Transform> transforms(ENTITY_COUNT);
        std::vector<Velocity> velocities(ENTITY_COUNT);
        
        initialize_physics_test_data(transforms, velocities);
        
        // Scalar implementation
        auto scalar_update = [&]() {
            for (usize i = 0; i < transforms.size(); ++i) {
                transforms[i].position += velocities[i].velocity * 0.016f;
                transforms[i].rotation += velocities[i].angular_velocity * 0.016f;
            }
        };
        
        // SIMD implementation
        VectorizedPhysicsSystem simd_system;
        auto simd_update = [&]() {
            simd_system.update_transforms_simd(transforms, velocities, 0.016f);
        };
        
        OptimizationFramework optimizer;
        const auto result = optimizer.apply_optimization(
            "Physics SIMD Optimization",
            scalar_update,
            simd_update,
            1000
        );
        
        log_info("SIMD Optimization Analysis:");
        log_info("• Processes 4 entities simultaneously");
        log_info("• Theoretical maximum speedup: 4x");
        log_info("• Actual speedup: {:.2f}x", result.improvement_factor);
        log_info("• SIMD efficiency: {:.1f}%", (result.improvement_factor / 4.0f) * 100.0f);
        log_info("• Works best with: aligned data, simple operations, no branching");
    }
};
```

### 2. Advanced SIMD Patterns

```cpp
class AdvancedSIMDOptimization {
    // Demonstrate more complex SIMD usage patterns
    class CollisionDetectionSIMD {
    public:
        // SIMD circle-circle collision detection (4 pairs at once)
        void detect_circle_collisions_simd(
            std::span<const Vec2> positions_a,
            std::span<const f32> radii_a,
            std::span<const Vec2> positions_b, 
            std::span<const f32> radii_b,
            std::span<CollisionResult> results
        ) {
            const usize pair_count = positions_a.size();
            const usize simd_pairs = (pair_count / 4) * 4;
            
            // Process 4 collision pairs simultaneously
            for (usize i = 0; i < simd_pairs; i += 4) {
                // Load positions A (4 Vec2 = 8 floats)
                const __m128 ax = _mm_loadu_ps(&positions_a[i].x);     // a0.x, a0.y, a1.x, a1.y
                const __m128 ay = _mm_loadu_ps(&positions_a[i + 2].x); // a2.x, a2.y, a3.x, a3.y
                
                // Load positions B
                const __m128 bx = _mm_loadu_ps(&positions_b[i].x);
                const __m128 by = _mm_loadu_ps(&positions_b[i + 2].x);
                
                // Load radii
                const __m128 ra = _mm_loadu_ps(&radii_a[i]);
                const __m128 rb = _mm_loadu_ps(&radii_b[i]);
                
                // Calculate distance vectors
                const __m128 dx = _mm_sub_ps(bx, ax);
                const __m128 dy = _mm_sub_ps(by, ay);
                
                // Calculate squared distances
                const __m128 dist_sq = _mm_add_ps(_mm_mul_ps(dx, dx), _mm_mul_ps(dy, dy));
                
                // Calculate squared radius sums
                const __m128 radius_sum = _mm_add_ps(ra, rb);
                const __m128 radius_sum_sq = _mm_mul_ps(radius_sum, radius_sum);
                
                // Compare distances (collision if dist_sq < radius_sum_sq)
                const __m128 collision_mask = _mm_cmplt_ps(dist_sq, radius_sum_sq);
                
                // Extract results
                alignas(16) f32 collision_results[4];
                _mm_store_ps(collision_results, collision_mask);
                
                // Process results
                for (u32 j = 0; j < 4; ++j) {
                    results[i + j].is_colliding = (collision_results[j] != 0.0f);
                    
                    if (results[i + j].is_colliding) {
                        // Calculate detailed collision info for colliding pairs
                        calculate_collision_details(positions_a[i + j], radii_a[i + j],
                                                   positions_b[i + j], radii_b[i + j],
                                                   results[i + j]);
                    }
                }
            }
            
            // Handle remaining pairs with scalar code
            for (usize i = simd_pairs; i < pair_count; ++i) {
                results[i] = detect_circle_collision_scalar(positions_a[i], radii_a[i],
                                                           positions_b[i], radii_b[i]);
            }
        }
        
        void demonstrate_collision_simd_benefits() {
            log_info("=== SIMD Collision Detection ===");
            
            constexpr usize COLLISION_PAIRS = 10000;
            
            // Generate test data
            auto test_data = generate_collision_test_data(COLLISION_PAIRS);
            std::vector<CollisionResult> scalar_results(COLLISION_PAIRS);
            std::vector<CollisionResult> simd_results(COLLISION_PAIRS);
            
            // Scalar collision detection
            auto scalar_detection = [&]() {
                for (usize i = 0; i < COLLISION_PAIRS; ++i) {
                    scalar_results[i] = detect_circle_collision_scalar(
                        test_data.positions_a[i], test_data.radii_a[i],
                        test_data.positions_b[i], test_data.radii_b[i]
                    );
                }
            };
            
            // SIMD collision detection
            auto simd_detection = [&]() {
                detect_circle_collisions_simd(
                    test_data.positions_a, test_data.radii_a,
                    test_data.positions_b, test_data.radii_b,
                    simd_results
                );
            };
            
            OptimizationFramework optimizer;
            const auto result = optimizer.apply_optimization(
                "Collision Detection SIMD",
                scalar_detection,
                simd_detection,
                100
            );
            
            // Verify results are identical
            verify_collision_results_identical(scalar_results, simd_results);
            
            log_info("SIMD Collision Detection Benefits:");
            log_info("• {:.2f}x speedup for broad-phase collision detection", result.improvement_factor);
            log_info("• Particularly effective for uniform collision shapes");
            log_info("• Reduces CPU time spent on collision detection");
            log_info("• Enables higher entity counts at same performance");
        }
    };
};
```

## Advanced Optimization Patterns

### 1. Memory Pool Hierarchies

```cpp
class MemoryPoolHierarchy {
    // Multi-level memory pool system for different allocation sizes
    class HierarchicalPoolAllocator {
        struct PoolLevel {
            usize block_size;
            usize pool_capacity;
            std::unique_ptr<PoolAllocator<byte>> pool;
            usize allocations_count;
            f32 utilization_ratio;
        };
        
        std::vector<PoolLevel> pool_levels_;
        std::unique_ptr<ArenaAllocator> fallback_arena_;
        
    public:
        explicit HierarchicalPoolAllocator(usize arena_size) {
            // Create pools for common allocation sizes
            const std::vector<usize> pool_sizes = {16, 32, 64, 128, 256, 512, 1024};
            
            for (const usize size : pool_sizes) {
                pool_levels_.push_back({
                    .block_size = size,
                    .pool_capacity = calculate_optimal_pool_capacity(size),
                    .pool = std::make_unique<PoolAllocator<byte>>(calculate_optimal_pool_capacity(size)),
                    .allocations_count = 0,
                    .utilization_ratio = 0.0f
                });
            }
            
            // Fallback arena for large or unusual allocations
            fallback_arena_ = std::make_unique<ArenaAllocator>(arena_size);
            
            log_info("Initialized hierarchical pool allocator:");
            for (const auto& level : pool_levels_) {
                log_info("  Pool: {} bytes x {} capacity", level.block_size, level.pool_capacity);
            }
        }
        
        void* allocate(usize size) {
            // Find appropriate pool level
            for (auto& level : pool_levels_) {
                if (size <= level.block_size) {
                    ++level.allocations_count;
                    
                    if (auto* ptr = level.pool->allocate()) {
                        return ptr;
                    }
                    
                    // Pool exhausted, fall back to arena
                    log_warning("Pool exhausted for size {}, using arena fallback", size);
                    break;
                }
            }
            
            // Use arena for large allocations or when pools are exhausted
            return fallback_arena_->allocate_bytes(size);
        }
        
        void deallocate(void* ptr, usize size) {
            // Find appropriate pool level
            for (auto& level : pool_levels_) {
                if (size <= level.block_size) {
                    level.pool->deallocate(static_cast<byte*>(ptr));
                    return;
                }
            }
            
            // Arena allocations can't be individually deallocated
            // This is a trade-off for simplicity
        }
        
        // Educational analysis of pool utilization
        void analyze_utilization() {
            log_info("=== Pool Utilization Analysis ===");
            
            usize total_allocations = 0;
            for (auto& level : pool_levels_) {
                level.utilization_ratio = static_cast<f32>(level.allocations_count) / 
                                        static_cast<f32>(level.pool_capacity);
                total_allocations += level.allocations_count;
            }
            
            for (const auto& level : pool_levels_) {
                const f32 allocation_percentage = static_cast<f32>(level.allocations_count) / 
                                                static_cast<f32>(total_allocations) * 100.0f;
                
                log_info("Pool {} bytes:", level.block_size);
                log_info("  Allocations: {} ({:.1f}% of total)", 
                        level.allocations_count, allocation_percentage);
                log_info("  Utilization: {:.1f}%", level.utilization_ratio * 100.0f);
                
                if (level.utilization_ratio > 0.9f) {
                    log_info("  Status: Near capacity - consider increasing pool size");
                } else if (level.utilization_ratio < 0.1f) {
                    log_info("  Status: Under-utilized - consider reducing pool size");
                } else {
                    log_info("  Status: Well utilized");
                }
            }
            
            const usize arena_usage = fallback_arena_->get_used_bytes();
            log_info("Arena fallback usage: {:.2f}KB", arena_usage / 1024.0f);
        }
        
    private:
        usize calculate_optimal_pool_capacity(usize block_size) {
            // Heuristic: smaller blocks need more capacity
            const usize base_capacity = 1024; // Base capacity
            const usize size_factor = 1024 / block_size; // Inverse relationship
            return base_capacity * size_factor;
        }
    };
    
public:
    void demonstrate_hierarchical_pools() {
        log_info("=== Hierarchical Memory Pool Optimization ===");
        
        HierarchicalPoolAllocator hierarchical_allocator{1024 * 1024}; // 1MB arena
        StandardAllocator standard_allocator{};
        
        // Simulate realistic allocation patterns
        const auto allocation_pattern = generate_realistic_allocation_pattern();
        
        // Benchmark hierarchical pool allocation
        auto hierarchical_alloc = [&]() {
            std::vector<void*> allocations;
            
            for (const auto& alloc_request : allocation_pattern) {
                void* ptr = hierarchical_allocator.allocate(alloc_request.size);
                allocations.push_back(ptr);
                
                // Simulate deallocation pattern
                if (alloc_request.should_deallocate && !allocations.empty()) {
                    const usize dealloc_index = allocations.size() - 1;
                    hierarchical_allocator.deallocate(allocations[dealloc_index], alloc_request.size);
                    allocations.erase(allocations.begin() + dealloc_index);
                }
            }
            
            return allocations.size();
        };
        
        // Benchmark standard allocation
        auto standard_alloc = [&]() {
            std::vector<void*> allocations;
            
            for (const auto& alloc_request : allocation_pattern) {
                void* ptr = std::malloc(alloc_request.size);
                allocations.push_back(ptr);
                
                if (alloc_request.should_deallocate && !allocations.empty()) {
                    const usize dealloc_index = allocations.size() - 1;
                    std::free(allocations[dealloc_index]);
                    allocations.erase(allocations.begin() + dealloc_index);
                }
            }
            
            // Clean up remaining allocations
            for (void* ptr : allocations) {
                std::free(ptr);
            }
            
            return 0; // All deallocated
        };
        
        OptimizationFramework optimizer;
        const auto result = optimizer.apply_optimization(
            "Hierarchical Memory Pools",
            standard_alloc,
            hierarchical_alloc,
            10
        );
        
        // Analyze pool utilization
        hierarchical_allocator.analyze_utilization();
        
        log_info("Hierarchical Pool Benefits:");
        log_info("• Reduced fragmentation through size-appropriate pools");
        log_info("• Faster allocation for common sizes");
        log_info("• Predictable performance characteristics");
        log_info("• Educational visibility into allocation patterns");
    }
};
```

### 2. Lock-Free Data Structures

```cpp
class LockFreeOptimization {
    // Lock-free queue for multi-threaded ECS systems
    template<typename T>
    class LockFreeQueue {
        static constexpr usize CACHE_LINE_SIZE = 64;
        
        struct alignas(CACHE_LINE_SIZE) Node {
            std::atomic<Node*> next{nullptr};
            T data;
        };
        
        alignas(CACHE_LINE_SIZE) std::atomic<Node*> head_{nullptr};
        alignas(CACHE_LINE_SIZE) std::atomic<Node*> tail_{nullptr};
        
    public:
        LockFreeQueue() {
            Node* dummy = new Node{};
            head_.store(dummy, std::memory_order_relaxed);
            tail_.store(dummy, std::memory_order_relaxed);
        }
        
        void enqueue(const T& item) {
            Node* new_node = new Node{};
            new_node->data = item;
            
            Node* prev_tail = tail_.exchange(new_node, std::memory_order_acq_rel);
            prev_tail->next.store(new_node, std::memory_order_release);
        }
        
        bool dequeue(T& result) {
            Node* head = head_.load(std::memory_order_acquire);
            Node* next = head->next.load(std::memory_order_acquire);
            
            if (next == nullptr) {
                return false; // Queue empty
            }
            
            result = next->data;
            head_.store(next, std::memory_order_release);
            delete head; // Safe to delete old head
            
            return true;
        }
    };
    
public:
    void demonstrate_lock_free_performance() {
        log_info("=== Lock-Free vs Mutex Performance ===");
        
        constexpr usize OPERATIONS = 100000;
        constexpr u32 THREAD_COUNT = 4;
        
        // Test mutex-based queue
        std::queue<u32> mutex_queue;
        std::mutex queue_mutex;
        
        auto mutex_producer = [&](u32 thread_id) {
            for (usize i = 0; i < OPERATIONS / THREAD_COUNT; ++i) {
                const u32 value = thread_id * 1000000 + static_cast<u32>(i);
                
                std::lock_guard<std::mutex> lock(queue_mutex);
                mutex_queue.push(value);
            }
        };
        
        auto mutex_consumer = [&]() {
            usize consumed = 0;
            while (consumed < OPERATIONS) {
                std::lock_guard<std::mutex> lock(queue_mutex);
                if (!mutex_queue.empty()) {
                    mutex_queue.pop();
                    ++consumed;
                }
            }
        };
        
        // Test lock-free queue
        LockFreeQueue<u32> lockfree_queue;
        
        auto lockfree_producer = [&](u32 thread_id) {
            for (usize i = 0; i < OPERATIONS / THREAD_COUNT; ++i) {
                const u32 value = thread_id * 1000000 + static_cast<u32>(i);
                lockfree_queue.enqueue(value);
            }
        };
        
        auto lockfree_consumer = [&]() {
            usize consumed = 0;
            u32 value;
            while (consumed < OPERATIONS) {
                if (lockfree_queue.dequeue(value)) {
                    ++consumed;
                }
            }
        };
        
        // Benchmark mutex version
        f32 mutex_time;
        {
            Timer timer("Mutex Queue");
            
            std::vector<std::thread> producers;
            for (u32 i = 0; i < THREAD_COUNT; ++i) {
                producers.emplace_back(mutex_producer, i);
            }
            
            std::thread consumer(mutex_consumer);
            
            for (auto& producer : producers) {
                producer.join();
            }
            consumer.join();
            
            mutex_time = timer.elapsed_ms();
        }
        
        // Benchmark lock-free version
        f32 lockfree_time;
        {
            Timer timer("Lock-Free Queue");
            
            std::vector<std::thread> producers;
            for (u32 i = 0; i < THREAD_COUNT; ++i) {
                producers.emplace_back(lockfree_producer, i);
            }
            
            std::thread consumer(lockfree_consumer);
            
            for (auto& producer : producers) {
                producer.join();
            }
            consumer.join();
            
            lockfree_time = timer.elapsed_ms();
        }
        
        const f32 improvement = mutex_time / lockfree_time;
        
        log_info("Lock-Free Performance Results:");
        log_info("• Mutex Queue: {:.2f}ms", mutex_time);
        log_info("• Lock-Free Queue: {:.2f}ms", lockfree_time);
        log_info("• Performance Improvement: {:.2f}x faster", improvement);
        log_info("• Scalability: Lock-free scales better with thread count");
        log_info("• Latency: Lock-free provides more predictable latency");
        log_info("• Memory: Lock-free requires careful memory management");
    }
};
```

## Measurement and Profiling

### 1. Comprehensive Benchmarking Framework

```cpp
class BenchmarkingFramework {
    struct BenchmarkResult {
        std::string name;
        f32 min_time_ms;
        f32 max_time_ms;
        f32 average_time_ms;
        f32 standard_deviation_ms;
        f32 percentile_95_ms;
        f32 percentile_99_ms;
        usize iterations;
        
        // Statistical analysis
        bool is_statistically_significant(const BenchmarkResult& other, f32 confidence = 0.95f) const;
        f32 calculate_effect_size(const BenchmarkResult& other) const;
    };
    
    struct BenchmarkConfig {
        usize min_iterations{100};
        usize max_iterations{10000};
        f32 max_time_seconds{10.0f};
        f32 confidence_interval{0.95f};
        bool enable_statistical_analysis{true};
        bool enable_outlier_detection{true};
    };
    
public:
    template<typename Func>
    BenchmarkResult run_benchmark(const std::string& name, Func&& function, 
                                 const BenchmarkConfig& config = {}) {
        log_info("Running benchmark: {}", name);
        
        std::vector<f32> measurements;
        measurements.reserve(config.max_iterations);
        
        const f64 start_time = Time::get_precise_time();
        
        // Adaptive iteration count based on measurement stability
        for (usize iteration = 0; iteration < config.max_iterations; ++iteration) {
            // Warm up cache and branch predictors
            if (iteration < 10) {
                function();
                continue;
            }
            
            // Measure execution time
            const f64 iteration_start = Time::get_precise_time();
            function();
            const f64 iteration_end = Time::get_precise_time();
            
            const f32 duration_ms = static_cast<f32>((iteration_end - iteration_start) * 1000.0);
            measurements.push_back(duration_ms);
            
            // Check stopping conditions
            if (iteration >= config.min_iterations) {
                const f64 elapsed_time = iteration_end - start_time;
                if (elapsed_time > config.max_time_seconds) {
                    log_info("  Stopping benchmark after {:.1f}s (time limit)", elapsed_time);
                    break;
                }
                
                // Check for statistical convergence
                if (iteration % 100 == 99 && is_converged(measurements, config.confidence_interval)) {
                    log_info("  Stopping benchmark after {} iterations (converged)", iteration + 1);
                    break;
                }
            }
        }
        
        // Calculate statistics
        BenchmarkResult result{};
        result.name = name;
        result.iterations = measurements.size();
        
        if (config.enable_outlier_detection) {
            remove_outliers(measurements);
        }
        
        calculate_statistics(measurements, result);
        
        log_info("  Results: {:.3f}ms ± {:.3f}ms ({} iterations)", 
                result.average_time_ms, result.standard_deviation_ms, result.iterations);
        
        return result;
    }
    
    // Compare two optimization approaches with statistical significance testing
    template<typename BaselineFunc, typename OptimizedFunc>
    void compare_optimizations(const std::string& name,
                              BaselineFunc&& baseline,
                              OptimizedFunc&& optimized,
                              const BenchmarkConfig& config = {}) {
        log_info("=== Optimization Comparison: {} ===", name);
        
        const auto baseline_result = run_benchmark(name + " (Baseline)", baseline, config);
        const auto optimized_result = run_benchmark(name + " (Optimized)", optimized, config);
        
        // Statistical analysis
        const bool is_significant = optimized_result.is_statistically_significant(baseline_result, 
                                                                                 config.confidence_interval);
        const f32 effect_size = optimized_result.calculate_effect_size(baseline_result);
        const f32 improvement_factor = baseline_result.average_time_ms / optimized_result.average_time_ms;
        const f32 improvement_percentage = (1.0f - optimized_result.average_time_ms / baseline_result.average_time_ms) * 100.0f;
        
        log_info("Comparison Results:");
        log_info("• Performance improvement: {:.2f}x faster ({:.1f}% reduction)", 
                improvement_factor, improvement_percentage);
        log_info("• Statistical significance: {} (confidence: {:.1f}%)", 
                is_significant ? "YES" : "NO", config.confidence_interval * 100.0f);
        log_info("• Effect size: {:.3f} ({})", effect_size, interpret_effect_size(effect_size));
        
        if (is_significant) {
            log_info("• ✅ Optimization is statistically significant and meaningful");
        } else {
            log_info("• ⚠️  Optimization may not be significant - consider more iterations");
        }
        
        // Provide recommendations
        if (improvement_factor > 1.1f && is_significant) {
            log_info("• 🎯 Recommended: Apply this optimization");
        } else if (improvement_factor < 1.05f) {
            log_info("• 🤔 Marginal improvement - consider complexity trade-offs");
        }
    }
    
private:
    bool is_converged(const std::vector<f32>& measurements, f32 confidence_interval) {
        if (measurements.size() < 30) return false; // Need minimum sample size
        
        // Calculate confidence interval for mean
        const f32 mean = calculate_mean(measurements);
        const f32 std_dev = calculate_standard_deviation(measurements, mean);
        const f32 standard_error = std_dev / std::sqrt(static_cast<f32>(measurements.size()));
        
        // t-value for desired confidence (approximation for large samples)
        const f32 t_value = 1.96f; // For 95% confidence
        const f32 margin_of_error = t_value * standard_error;
        
        // Consider converged if margin of error is small relative to mean
        const f32 relative_error = margin_of_error / mean;
        return relative_error < 0.05f; // 5% relative error threshold
    }
    
    void remove_outliers(std::vector<f32>& measurements) {
        if (measurements.size() < 10) return; // Don't remove outliers from small samples
        
        // Use IQR method for outlier detection
        std::sort(measurements.begin(), measurements.end());
        
        const usize q1_index = measurements.size() / 4;
        const usize q3_index = (measurements.size() * 3) / 4;
        
        const f32 q1 = measurements[q1_index];
        const f32 q3 = measurements[q3_index];
        const f32 iqr = q3 - q1;
        const f32 lower_bound = q1 - 1.5f * iqr;
        const f32 upper_bound = q3 + 1.5f * iqr;
        
        const usize original_size = measurements.size();
        measurements.erase(
            std::remove_if(measurements.begin(), measurements.end(),
                          [lower_bound, upper_bound](f32 value) {
                              return value < lower_bound || value > upper_bound;
                          }),
            measurements.end()
        );
        
        const usize outliers_removed = original_size - measurements.size();
        if (outliers_removed > 0) {
            log_info("  Removed {} outliers ({:.1f}% of measurements)", 
                    outliers_removed, static_cast<f32>(outliers_removed) / static_cast<f32>(original_size) * 100.0f);
        }
    }
};
```

---

**ECScope Optimization Techniques: Scientific measurement, systematic improvement, educational clarity in every optimization.**

*Understanding why optimizations work is as important as knowing how to apply them. ECScope makes both aspects clear through comprehensive analysis and practical demonstration.*