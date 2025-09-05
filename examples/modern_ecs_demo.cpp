/**
 * @file examples/modern_ecs_demo.cpp
 * @brief Educational Demo of Modern ECS Enhancements and Performance Comparisons
 * 
 * This comprehensive educational demo showcases the modern ECS enhancements
 * including sparse sets, enhanced queries, dependency resolution, and performance
 * monitoring. It provides clear, practical examples of when to use different
 * storage strategies and demonstrates the performance trade-offs.
 * 
 * Learning Objectives:
 * 1. Understand when to use archetype vs sparse set storage
 * 2. See the performance impact of different storage strategies
 * 3. Learn about automatic system dependency resolution
 * 4. Experience modern C++20 concepts for type safety
 * 5. Observe memory allocation patterns and optimization
 * 
 * Demo Scenarios:
 * - Sparse vs Dense component scenarios
 * - Query performance comparisons
 * - System dependency resolution examples
 * - Memory allocation strategy impacts
 * - Real-time performance monitoring
 * 
 * @author ECScope Educational ECS Framework
 * @date 2024
 */

#include "core/pch.hpp"
#include "core/log.hpp"
#include "ecs/registry.hpp"
#include "ecs/sparse_set.hpp"
#include "ecs/enhanced_query.hpp"
#include "ecs/dependency_resolver.hpp"
#include "ecs/performance_integration.hpp"
#include "ecs/modern_concepts.hpp"
#include "memory/allocators/arena.hpp"
#include "performance/performance_lab.hpp"

#include <iostream>
#include <random>
#include <chrono>
#include <iomanip>

using namespace ecscope;
using namespace ecscope::ecs;
using namespace ecscope::ecs::sparse;
using namespace ecscope::ecs::enhanced;
using namespace ecscope::ecs::dependency;
using namespace ecscope::ecs::performance;

//=============================================================================
// Educational Component Types for Demonstrations
//=============================================================================

/**
 * @brief Small, frequently accessed component - ideal for archetype storage
 */
struct Position {
    f32 x, y, z;
    
    Position() : x(0.0f), y(0.0f), z(0.0f) {}
    Position(f32 x_, f32 y_, f32 z_) : x(x_), y(y_), z(z_) {}
    
    bool operator==(const Position& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

/**
 * @brief Small velocity component - also ideal for SoA archetype layout
 */
struct Velocity {
    f32 vx, vy, vz;
    
    Velocity() : vx(0.0f), vy(0.0f), vz(0.0f) {}
    Velocity(f32 vx_, f32 vy_, f32 vz_) : vx(vx_), vy(vy_), vz(vz_) {}
    
    bool operator==(const Velocity& other) const {
        return vx == other.vx && vy == other.vy && vz == other.vz;
    }
};

/**
 * @brief Large, infrequently accessed component - ideal for sparse set storage
 */
struct DetailedStats {
    std::array<f32, 64> statistics;  // Large component
    std::string description;
    u64 last_updated;
    
    DetailedStats() : last_updated(0) {
        statistics.fill(0.0f);
        description = "Default stats";
    }
    
    bool operator==(const DetailedStats& other) const {
        return statistics == other.statistics && description == other.description;
    }
};

/**
 * @brief Sparse component that only some entities have
 */
struct SpecialAbility {
    std::string ability_name;
    f32 cooldown;
    u32 level;
    
    SpecialAbility() : ability_name("None"), cooldown(0.0f), level(1) {}
    SpecialAbility(const std::string& name, f32 cd, u32 lvl) 
        : ability_name(name), cooldown(cd), level(lvl) {}
    
    bool operator==(const SpecialAbility& other) const {
        return ability_name == other.ability_name && cooldown == other.cooldown && level == other.level;
    }
};

// Validate component designs with our modern concepts
ECSCOPE_VALIDATE_COMPONENT(Position);
ECSCOPE_VALIDATE_COMPONENT(Velocity);
ECSCOPE_VALIDATE_COMPONENT(DetailedStats);
ECSCOPE_VALIDATE_COMPONENT(SpecialAbility);

// Check SoA suitability
ECSCOPE_CHECK_SOA_SUITABILITY(Position);
ECSCOPE_CHECK_SOA_SUITABILITY(Velocity);
// Note: DetailedStats and SpecialAbility intentionally not SoA suitable

//=============================================================================
// Educational System Examples
//=============================================================================

/**
 * @brief Movement system demonstrating archetype-friendly processing
 */
class MovementSystem : public System {
public:
    MovementSystem() : System("MovementSystem", SystemPhase::Update) {
        reads<Position>();
        writes<Position>();
        reads<Velocity>();
    }
    
    void update(const SystemContext& context) override {
        f64 dt = context.delta_time();
        
        // Use enhanced query for optimal performance
        auto query = make_performance_query<Position, Velocity>(
            context.registry(), get_sparse_registry());
        
        query.named("MovementQuery")
             .use_strategy(StorageStrategy::Archetype) // Force archetype for educational comparison
             .for_each([dt](Entity entity, Position& pos, const Velocity& vel) {
                 pos.x += vel.vx * dt;
                 pos.y += vel.vy * dt;
                 pos.z += vel.vz * dt;
             });
    }
    
private:
    // In a real implementation, this would be injected
    static SparseSetRegistry& get_sparse_registry() {
        static SparseSetRegistry registry;
        return registry;
    }
};

/**
 * @brief Statistics system demonstrating sparse set optimization
 */
class StatisticsSystem : public System {
public:
    StatisticsSystem() : System("StatisticsSystem", SystemPhase::Update) {
        writes<DetailedStats>();
        depends_on("MovementSystem"); // Run after movement
    }
    
    void update(const SystemContext& context) override {
        // This system only processes entities with DetailedStats (sparse)
        auto query = make_enhanced_query<DetailedStats>(
            context.registry(), get_sparse_registry());
        
        query.named("StatisticsQuery")
             .use_strategy(StorageStrategy::SparseSet) // Optimal for sparse data
             .for_each([](Entity entity, DetailedStats& stats) {
                 // Update statistics - expensive operation on sparse data
                 for (usize i = 0; i < stats.statistics.size(); ++i) {
                     stats.statistics[i] += 0.1f; // Simulate processing
                 }
                 stats.last_updated = std::chrono::steady_clock::now().time_since_epoch().count();
             });
    }
    
private:
    static SparseSetRegistry& get_sparse_registry() {
        static SparseSetRegistry registry;
        return registry;
    }
};

ECSCOPE_VALIDATE_SYSTEM(MovementSystem);
ECSCOPE_VALIDATE_SYSTEM(StatisticsSystem);

//=============================================================================
// Educational Demo Functions
//=============================================================================

/**
 * @brief Demonstrate the difference between sparse and dense component scenarios
 */
void demo_storage_strategy_comparison() {
    LOG_INFO("=== Storage Strategy Comparison Demo ===");
    
    // Create memory allocators for educational tracking
    auto arena = std::make_unique<memory::ArenaAllocator>(4 * MB, "Demo Arena");
    
    // Create registries
    Registry registry(AllocatorConfig::create_educational_focused(), "Demo Registry");
    SparseSetRegistry sparse_registry(arena.get());
    
    // Create entities with different sparsity patterns
    const usize entity_count = 10000;
    const f64 sparse_component_ratio = 0.1; // Only 10% of entities have sparse components
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<f64> dist(0.0, 1.0);
    
    LOG_INFO("Creating {} entities with {:.1%} sparse component density", 
             entity_count, sparse_component_ratio);
    
    // Create entities
    std::vector<Entity> entities;
    entities.reserve(entity_count);
    
    for (usize i = 0; i < entity_count; ++i) {
        // All entities get Position and Velocity (dense components)
        Position pos(dist(gen) * 100.0f, dist(gen) * 100.0f, dist(gen) * 100.0f);
        Velocity vel(dist(gen) * 10.0f, dist(gen) * 10.0f, dist(gen) * 10.0f);
        
        Entity entity = registry.create_entity(pos, vel);
        entities.push_back(entity);
        
        // Only some entities get DetailedStats (sparse component)
        if (dist(gen) < sparse_component_ratio) {
            DetailedStats stats;
            stats.description = "Entity " + std::to_string(i) + " detailed stats";
            
            // Add to sparse set
            sparse_registry.get_or_create_sparse_set<DetailedStats>().insert(entity, stats);
        }
        
        // Even fewer get SpecialAbility (very sparse)
        if (dist(gen) < sparse_component_ratio / 2) {
            SpecialAbility ability("Special Power", 5.0f, static_cast<u32>(i % 10) + 1);
            sparse_registry.get_or_create_sparse_set<SpecialAbility>().insert(entity, ability);
        }
    }
    
    LOG_INFO("Entity creation completed");
    LOG_INFO("  Total entities: {}", entity_count);
    LOG_INFO("  Entities with DetailedStats: {}", 
             sparse_registry.get_or_create_sparse_set<DetailedStats>().size());
    LOG_INFO("  Entities with SpecialAbility: {}", 
             sparse_registry.get_or_create_sparse_set<SpecialAbility>().size());
    
    // Demonstrate query performance differences
    LOG_INFO("\n--- Query Performance Comparison ---");
    
    // Dense component query (Position + Velocity)
    auto dense_query = make_enhanced_query<Position, Velocity>(registry, sparse_registry);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    auto dense_entities = dense_query.named("Dense Query")
                                    .use_strategy(StorageStrategy::Archetype)
                                    .entities();
    auto end_time = std::chrono::high_resolution_clock::now();
    
    auto dense_query_time = std::chrono::duration<f64>(end_time - start_time).count();
    LOG_INFO("Dense component query (Archetype): {:.3f} ms, {} entities", 
             dense_query_time * 1000.0, dense_entities.size());
    
    // Sparse component query (DetailedStats only)
    start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<Entity> sparse_entities;
    sparse_registry.get_or_create_sparse_set<DetailedStats>().for_each(
        [&sparse_entities](Entity entity, const DetailedStats&) {
            sparse_entities.push_back(entity);
        });
    
    end_time = std::chrono::high_resolution_clock::now();
    auto sparse_query_time = std::chrono::duration<f64>(end_time - start_time).count();
    LOG_INFO("Sparse component query (Sparse Set): {:.3f} ms, {} entities", 
             sparse_query_time * 1000.0, sparse_entities.size());
    
    // Hybrid query (Position + DetailedStats)
    auto hybrid_query = make_enhanced_query<Position>(registry, sparse_registry);
    
    start_time = std::chrono::high_resolution_clock::now();
    std::vector<Entity> hybrid_entities;
    
    hybrid_query.named("Hybrid Query")
               .use_strategy(StorageStrategy::Hybrid)
               .for_each([&hybrid_entities, &sparse_registry](Entity entity, const Position&) {
                   if (sparse_registry.get_or_create_sparse_set<DetailedStats>().contains(entity)) {
                       hybrid_entities.push_back(entity);
                   }
               });
    
    end_time = std::chrono::high_resolution_clock::now();
    auto hybrid_query_time = std::chrono::duration<f64>(end_time - start_time).count();
    LOG_INFO("Hybrid query (Position + DetailedStats): {:.3f} ms, {} entities", 
             hybrid_query_time * 1000.0, hybrid_entities.size());
    
    // Performance analysis
    LOG_INFO("\n--- Performance Analysis ---");
    LOG_INFO("Dense query throughput: {:.0f} entities/ms", dense_entities.size() / (dense_query_time * 1000.0));
    LOG_INFO("Sparse query throughput: {:.0f} entities/ms", sparse_entities.size() / (sparse_query_time * 1000.0));
    LOG_INFO("Hybrid query throughput: {:.0f} entities/ms", hybrid_entities.size() / (hybrid_query_time * 1000.0));
    
    if (sparse_query_time < dense_query_time) {
        LOG_INFO("✅ Sparse set query is {:.2f}x faster for sparse data", dense_query_time / sparse_query_time);
    } else {
        LOG_INFO("ℹ️  Archetype query is {:.2f}x faster for dense data", sparse_query_time / dense_query_time);
    }
    
    // Memory usage comparison
    LOG_INFO("\n--- Memory Usage Analysis ---");
    LOG_INFO("Registry memory usage: {} KB", registry.memory_usage() / 1024);
    LOG_INFO("Arena utilization: {:.1%}", arena->usage_ratio());
    
    // Get sparse set performance metrics
    auto stats_metrics = sparse_registry.get_or_create_sparse_set<DetailedStats>().get_performance_metrics();
    LOG_INFO("Sparse set (DetailedStats):");
    LOG_INFO("  Memory efficiency: {:.1%}", stats_metrics.memory_efficiency);
    LOG_INFO("  Cache hit ratio: {:.1%}", stats_metrics.cache_hit_ratio);
    LOG_INFO("  Sparsity ratio: {:.1%}", stats_metrics.sparsity_ratio);
    
    LOG_INFO("=== Storage Strategy Demo Complete ===\n");
}

/**
 * @brief Demonstrate automatic system dependency resolution
 */
void demo_dependency_resolution() {
    LOG_INFO("=== System Dependency Resolution Demo ===");
    
    auto arena = std::make_unique<memory::ArenaAllocator>(1 * MB, "System Arena");
    DependencyResolver resolver(arena.get());
    
    // Create systems with dependencies
    auto movement_system = std::make_unique<MovementSystem>();
    auto stats_system = std::make_unique<StatisticsSystem>();
    
    // Add systems to resolver
    resolver.add_system(movement_system.get());
    resolver.add_system(stats_system.get());
    
    LOG_INFO("Added systems to dependency resolver");
    LOG_INFO("  MovementSystem: no dependencies");
    LOG_INFO("  StatisticsSystem: depends on MovementSystem");
    
    // Resolve execution order
    try {
        auto execution_order = resolver.resolve_execution_order(SystemPhase::Update);
        
        LOG_INFO("\nResolved execution order for Update phase:");
        for (usize i = 0; i < execution_order.size(); ++i) {
            LOG_INFO("  {}. {}", i + 1, execution_order[i]->name());
        }
        
        // Generate parallel groups
        auto parallel_groups = resolver.resolve_parallel_groups(SystemPhase::Update);
        
        LOG_INFO("\nParallel execution groups:");
        for (usize i = 0; i < parallel_groups.size(); ++i) {
            LOG_INFO("  Group {}: ", i + 1);
            for (const auto* system : parallel_groups[i]) {
                LOG_INFO("    - {}", system->name());
            }
        }
        
        // Get comprehensive statistics
        auto stats = resolver.get_comprehensive_statistics();
        LOG_INFO("\nDependency Resolution Statistics:");
        LOG_INFO("  Total systems: {}", stats.total_systems);
        LOG_INFO("  Total dependencies: {}", stats.total_dependencies);
        LOG_INFO("  Parallel efficiency: {:.1%}", stats.overall_parallelization_efficiency);
        LOG_INFO("  Average resolution time: {:.2f} μs", stats.average_resolution_time * 1e6);
        
    } catch (const std::exception& e) {
        LOG_ERROR("Dependency resolution failed: {}", e.what());
    }
    
    // Validate dependencies
    auto validation_errors = resolver.validate_all_dependencies();
    if (validation_errors.empty()) {
        LOG_INFO("✅ All system dependencies are valid");
    } else {
        LOG_WARN("⚠️  Dependency validation errors:");
        for (const auto& error : validation_errors) {
            LOG_WARN("  {}", error);
        }
    }
    
    LOG_INFO("=== Dependency Resolution Demo Complete ===\n");
}

/**
 * @brief Demonstrate comprehensive performance benchmarking
 */
void demo_performance_benchmarking() {
    LOG_INFO("=== Performance Benchmarking Demo ===");
    
    // Create full ECS setup
    auto arena = std::make_unique<memory::ArenaAllocator>(8 * MB, "Benchmark Arena");
    Registry registry(AllocatorConfig::create_performance_optimized(), "Benchmark Registry");
    SparseSetRegistry sparse_registry(arena.get());
    DependencyResolver dependency_resolver(arena.get());
    
    // Create benchmark suite
    ECSBenchmarkSuite benchmark_suite(&registry, &sparse_registry, &dependency_resolver, arena.get());
    
    // Configure benchmark
    ECSBenchmarkSuite::BenchmarkConfig config;
    config.entity_count = 50000;
    config.component_types = 5;
    config.sparsity_ratio = 0.3; // 30% of entities have sparse components
    config.iterations = 50;
    config.enable_parallel_benchmarks = true;
    config.enable_memory_profiling = true;
    config.enable_cache_analysis = true;
    
    LOG_INFO("Starting comprehensive benchmark suite...");
    LOG_INFO("Configuration: {} entities, {} component types, {:.1%} sparsity, {} iterations",
             config.entity_count, config.component_types, config.sparsity_ratio, config.iterations);
    
    // Run full benchmark
    auto metrics = benchmark_suite.run_full_benchmark(config);
    
    // Display results
    LOG_INFO("\n--- Benchmark Results ---");
    
    // Storage strategy results
    LOG_INFO("Storage Strategy Performance:");
    LOG_INFO("  Archetype queries: {:.2f} μs", metrics.storage.archetype_query_time_ns / 1000.0);
    LOG_INFO("  Sparse set queries: {:.2f} μs", metrics.storage.sparse_set_query_time_ns / 1000.0);
    LOG_INFO("  Hybrid queries: {:.2f} μs", metrics.storage.hybrid_query_time_ns / 1000.0);
    
    if (metrics.storage.sparse_set_query_time_ns < metrics.storage.archetype_query_time_ns) {
        f64 speedup = metrics.storage.archetype_query_time_ns / metrics.storage.sparse_set_query_time_ns;
        LOG_INFO("  ✅ Sparse sets are {:.2f}x faster for this data pattern", speedup);
    } else {
        f64 speedup = metrics.storage.sparse_set_query_time_ns / metrics.storage.archetype_query_time_ns;
        LOG_INFO("  ✅ Archetypes are {:.2f}x faster for this data pattern", speedup);
    }
    
    // Memory results
    LOG_INFO("\nMemory Performance:");
    LOG_INFO("  Archetype memory: {} KB", metrics.storage.archetype_memory_bytes / 1024);
    LOG_INFO("  Sparse set memory: {} KB", metrics.storage.sparse_set_memory_bytes / 1024);
    LOG_INFO("  Memory efficiency: {:.1%}", metrics.memory.allocation_efficiency);
    LOG_INFO("  Fragmentation ratio: {:.1%}", metrics.memory.memory_fragmentation_ratio);
    
    // Query performance
    LOG_INFO("\nQuery Performance:");
    LOG_INFO("  Simple queries: {:.2f} μs", metrics.query.simple_query_time_ns / 1000.0);
    LOG_INFO("  Complex queries: {:.2f} μs", metrics.query.complex_query_time_ns / 1000.0);
    LOG_INFO("  Parallel queries: {:.2f} μs", metrics.query.parallel_query_time_ns / 1000.0);
    LOG_INFO("  Cache hit ratio: {:.1%}", metrics.query.query_cache_hit_ratio);
    
    // System performance
    LOG_INFO("\nSystem Performance:");
    LOG_INFO("  Dependency resolution: {:.2f} μs", metrics.system.dependency_resolution_time_ns / 1000.0);
    LOG_INFO("  Parallel efficiency: {:.1%}", metrics.system.parallel_execution_efficiency);
    LOG_INFO("  Critical path time: {:.2f} ms", metrics.system.critical_path_time_ms);
    
    // Overall performance
    LOG_INFO("\nOverall Performance:");
    LOG_INFO("  Entities per second: {:.0f}", metrics.entities_per_second);
    LOG_INFO("  Components per second: {:.0f}", metrics.components_per_second);
    LOG_INFO("  Frame budget utilization: {:.1%}", metrics.frame_time_budget_utilization * 100.0);
    
    // Generate performance report
    auto report = benchmark_suite.generate_performance_report();
    
    LOG_INFO("\n--- Performance Analysis ---");
    LOG_INFO(report.analysis.performance_summary);
    
    LOG_INFO("Optimization Recommendations:");
    for (const auto& recommendation : report.analysis.optimization_recommendations) {
        LOG_INFO("  • {}", recommendation);
    }
    
    LOG_INFO("Best storage strategy: {}", report.analysis.best_storage_strategy);
    LOG_INFO("Overall performance score: {:.1f}/100", report.analysis.overall_performance_score);
    
    // Export results
    benchmark_suite.export_results("benchmark_results.csv");
    LOG_INFO("✅ Benchmark results exported to benchmark_results.csv");
    
    LOG_INFO("=== Performance Benchmarking Demo Complete ===\n");
}

/**
 * @brief Demonstrate modern C++20 concepts and type safety
 */
void demo_concepts_and_type_safety() {
    LOG_INFO("=== C++20 Concepts and Type Safety Demo ===");
    
    // Demonstrate compile-time validation
    LOG_INFO("Demonstrating compile-time type validation...");
    
    // These will compile successfully because components meet requirements
    static_assert(concepts::PerformantComponent<Position>);
    static_assert(concepts::PerformantComponent<Velocity>);
    static_assert(concepts::SoATransformable<Position>);
    static_assert(concepts::SoATransformable<Velocity>);
    
    // These demonstrate different concept satisfaction
    static_assert(concepts::PerformantComponent<DetailedStats>);
    static_assert(!concepts::SoATransformable<DetailedStats>); // Too large for efficient SoA
    
    LOG_INFO("✅ All component types pass compile-time validation");
    
    // Demonstrate query type safety
    Registry registry;
    SparseSetRegistry sparse_registry;
    
    // This will compile - valid queryable components
    auto valid_query = make_enhanced_query<Position, Velocity>(registry, sparse_registry);
    
    // Educational: Show what concepts prevent
    LOG_INFO("\nConcepts prevent common ECS mistakes:");
    LOG_INFO("  ✅ Components must be trivially copyable (prevents complex destructors)");
    LOG_INFO("  ✅ Components must have reasonable size (prevents cache misses)");
    LOG_INFO("  ✅ Systems must declare dependencies (enables automatic scheduling)");
    LOG_INFO("  ✅ Query types must be compatible (prevents runtime errors)");
    
    // Demonstrate storage strategy recommendations
    LOG_INFO("\nAutomatic storage strategy recommendations:");
    
    constexpr auto pos_strategy = recommend_storage_strategy<Position>();
    constexpr auto stats_strategy = recommend_storage_strategy<DetailedStats>();
    
    LOG_INFO("  Position: {} (small, dense component)", 
             pos_strategy == StorageStrategy::Archetype ? "Archetype" : "Sparse Set");
    LOG_INFO("  DetailedStats: {} (large, sparse component)", 
             stats_strategy == StorageStrategy::Archetype ? "Archetype" : "Sparse Set");
    
    LOG_INFO("=== Concepts and Type Safety Demo Complete ===\n");
}

//=============================================================================
// Main Demo Entry Point
//=============================================================================

/**
 * @brief Main function demonstrating all modern ECS features
 */
int main() {
    LOG_INFO("🚀 Modern ECS Educational Demo Starting");
    LOG_INFO("This demo showcases advanced ECS patterns and performance optimizations\n");
    
    try {
        // Demo 1: Storage strategy comparison
        demo_storage_strategy_comparison();
        
        // Demo 2: System dependency resolution
        demo_dependency_resolution();
        
        // Demo 3: Performance benchmarking
        demo_performance_benchmarking();
        
        // Demo 4: C++20 concepts and type safety
        demo_concepts_and_type_safety();
        
        LOG_INFO("🎉 All demos completed successfully!");
        
        // Educational summary
        LOG_INFO("\n=== Educational Summary ===");
        LOG_INFO("Key Takeaways:");
        LOG_INFO("1. Use archetype storage for dense, frequently accessed components");
        LOG_INFO("2. Use sparse set storage for sparse, large components");
        LOG_INFO("3. Hybrid approaches can optimize mixed component patterns");
        LOG_INFO("4. Automatic dependency resolution prevents system ordering issues");
        LOG_INFO("5. C++20 concepts provide compile-time safety and better error messages");
        LOG_INFO("6. Performance monitoring helps validate optimization choices");
        LOG_INFO("7. Custom memory allocators significantly impact ECS performance");
        
        LOG_INFO("\nNext Steps for Learning:");
        LOG_INFO("• Experiment with different entity counts and sparsity ratios");
        LOG_INFO("• Try different memory allocator configurations");
        LOG_INFO("• Implement custom systems with complex dependencies");
        LOG_INFO("• Profile real applications to validate performance improvements");
        
    } catch (const std::exception& e) {
        LOG_ERROR("Demo failed with exception: {}", e.what());
        return -1;
    }
    
    return 0;
}