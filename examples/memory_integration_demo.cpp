/**
 * @file memory_integration_demo.cpp
 * @brief Educational demonstration of ECS Registry memory integration
 * 
 * This example shows how to use the custom memory allocators with the ECS Registry
 * and demonstrates the performance and educational benefits of different allocation strategies.
 */

#include "ecs/registry.hpp"
#include "ecs/components/transform.hpp"
#include "ecs/components/renderable.hpp"
#include "core/log.hpp"
#include <iostream>
#include <chrono>

using namespace ecscope;

// Demo components for testing
struct Position {
    f32 x, y, z;
    Position(f32 x = 0.0f, f32 y = 0.0f, f32 z = 0.0f) : x(x), y(y), z(z) {}
};

struct Velocity {
    f32 dx, dy, dz;
    Velocity(f32 dx = 0.0f, f32 dy = 0.0f, f32 dz = 0.0f) : dx(dx), dy(dy), dz(dz) {}
};

struct Health {
    i32 current, maximum;
    Health(i32 current = 100, i32 maximum = 100) : current(current), maximum(maximum) {}
};

void demonstrate_basic_usage() {
    LOG_INFO("=== Basic Usage Demonstration ===");
    
    // Create educational registry with custom memory management
    auto registry = ecs::create_educational_registry("Demo_Registry");
    
    // Create entities with different component combinations
    LOG_INFO("Creating entities with different component combinations...");
    
    // Player entity with position, velocity, and health
    ecs::Entity player = registry->create_entity<Position, Velocity, Health>(
        Position{0.0f, 0.0f, 0.0f},
        Velocity{1.0f, 0.0f, 0.0f},
        Health{100, 100}
    );
    LOG_INFO("Created player entity: {}", player);
    
    // Enemy entities with position and health only
    std::vector<ecs::Entity> enemies;
    for (int i = 0; i < 10; ++i) {
        ecs::Entity enemy = registry->create_entity<Position, Health>(
            Position{static_cast<f32>(i * 5), 0.0f, 10.0f},
            Health{50, 50}
        );
        enemies.push_back(enemy);
    }
    LOG_INFO("Created {} enemy entities", enemies.size());
    
    // Projectile entities with position and velocity only
    std::vector<ecs::Entity> projectiles;
    for (int i = 0; i < 50; ++i) {
        ecs::Entity projectile = registry->create_entity<Position, Velocity>(
            Position{0.0f, 0.0f, static_cast<f32>(i)},
            Velocity{0.0f, 0.0f, -10.0f}
        );
        projectiles.push_back(projectile);
    }
    LOG_INFO("Created {} projectile entities", projectiles.size());
    
    // Access and modify components
    LOG_INFO("Accessing and modifying components...");
    Position* player_pos = registry->get_component<Position>(player);
    if (player_pos) {
        LOG_INFO("Player position: ({}, {}, {})", player_pos->x, player_pos->y, player_pos->z);
        player_pos->x += 5.0f;
        LOG_INFO("Updated player position: ({}, {}, {})", player_pos->x, player_pos->y, player_pos->z);
    }
    
    // Query entities with specific components
    LOG_INFO("Querying entities with Position and Velocity components...");
    auto moving_entities = registry->get_entities_with<Position, Velocity>();
    LOG_INFO("Found {} entities with Position and Velocity", moving_entities.size());
    
    // Iterate over entities with specific components
    registry->for_each<Position, Velocity>([](ecs::Entity entity, Position& pos, Velocity& vel) {
        pos.x += vel.dx * 0.016f; // Simulate 60 FPS update
        pos.y += vel.dy * 0.016f;
        pos.z += vel.dz * 0.016f;
    });
    
    // Display memory statistics
    auto stats = registry->get_memory_statistics();
    LOG_INFO("Memory Statistics:");
    LOG_INFO("  - Active entities: {}", stats.active_entities);
    LOG_INFO("  - Total archetypes: {}", stats.total_archetypes);
    LOG_INFO("  - Arena utilization: {:.2f}%", stats.arena_utilization() * 100.0);
    LOG_INFO("  - Memory efficiency: {:.2f}%", stats.memory_efficiency * 100.0);
    LOG_INFO("  - Cache hit ratio: {:.2f}%", stats.cache_hit_ratio * 100.0);
    
    // Generate detailed report
    std::string report = registry->generate_memory_report();
    LOG_INFO("\\n{}", report);
}

void demonstrate_performance_comparison() {
    LOG_INFO("\\n=== Performance Comparison Demonstration ===");
    
    // Create registries with different allocation strategies
    auto educational_registry = ecs::create_educational_registry("Educational_Test");
    auto performance_registry = ecs::create_performance_registry("Performance_Test");
    auto conservative_registry = ecs::create_conservative_registry("Conservative_Test");
    
    const usize test_entities = 5000;
    
    LOG_INFO("Running performance benchmarks with {} entities...", test_entities);
    
    // Benchmark entity creation
    educational_registry->benchmark_allocators("Entity_Creation", test_entities);
    performance_registry->benchmark_allocators("Entity_Creation", test_entities);
    conservative_registry->benchmark_allocators("Entity_Creation", test_entities);
    
    // Compare results
    LOG_INFO("\\nPerformance Comparison Results:");
    
    auto educational_comparisons = educational_registry->get_performance_comparisons();
    auto performance_comparisons = performance_registry->get_performance_comparisons();
    auto conservative_comparisons = conservative_registry->get_performance_comparisons();
    
    if (!educational_comparisons.empty()) {
        auto& comp = educational_comparisons.back();
        LOG_INFO("Educational Registry: {:.2f}x speedup ({:.2f}ms vs {:.2f}ms)",
                comp.speedup_factor, comp.custom_allocator_time, comp.standard_allocator_time);
    }
    
    if (!performance_comparisons.empty()) {
        auto& comp = performance_comparisons.back();
        LOG_INFO("Performance Registry: {:.2f}x speedup ({:.2f}ms vs {:.2f}ms)",
                comp.speedup_factor, comp.custom_allocator_time, comp.standard_allocator_time);
    }
    
    if (!conservative_comparisons.empty()) {
        auto& comp = conservative_comparisons.back();
        LOG_INFO("Conservative Registry: {:.2f}x speedup ({:.2f}ms vs {:.2f}ms)",
                comp.speedup_factor, comp.custom_allocator_time, comp.standard_allocator_time);
    }
    
    // Display memory usage comparison
    auto edu_stats = educational_registry->get_memory_statistics();
    auto perf_stats = performance_registry->get_memory_statistics();
    auto cons_stats = conservative_registry->get_memory_statistics();
    
    LOG_INFO("\\nMemory Usage Comparison:");
    LOG_INFO("Educational: {:.2f}% arena, {:.2f}% efficiency", 
             edu_stats.arena_utilization() * 100.0, edu_stats.memory_efficiency * 100.0);
    LOG_INFO("Performance: {:.2f}% arena, {:.2f}% efficiency",
             perf_stats.arena_utilization() * 100.0, perf_stats.memory_efficiency * 100.0);
    LOG_INFO("Conservative: {:.2f}% arena, {:.2f}% efficiency",
             cons_stats.arena_utilization() * 100.0, cons_stats.memory_efficiency * 100.0);
}

void demonstrate_memory_pressure_handling() {
    LOG_INFO("\\n=== Memory Pressure Handling Demonstration ===");
    
    // Create registry with limited memory for demonstration
    ecs::AllocatorConfig limited_config = ecs::AllocatorConfig::create_educational_focused();
    limited_config.archetype_arena_size = 1 * MB; // Small arena for demo
    limited_config.entity_pool_capacity = 1000;   // Small pool
    
    ecs::Registry limited_registry(limited_config, "Limited_Memory_Registry");
    
    LOG_INFO("Creating entities to demonstrate memory pressure...");
    
    std::vector<ecs::Entity> stress_test_entities;
    
    // Create entities until we approach memory limits
    for (usize i = 0; i < 2000; ++i) {
        ecs::Entity entity = limited_registry.create_entity<Position, Velocity, Health>(
            Position{static_cast<f32>(i), static_cast<f32>(i), static_cast<f32>(i)},
            Velocity{1.0f, 1.0f, 1.0f},
            Health{100, 100}
        );
        stress_test_entities.push_back(entity);
        
        // Check memory pressure every 100 entities
        if (i % 100 == 0) {
            auto stats = limited_registry.get_memory_statistics();
            LOG_INFO("Created {} entities - Arena: {:.1f}%, Pool: {:.1f}%",
                    i + 1,
                    stats.arena_utilization() * 100.0,
                    stats.pool_utilization() * 100.0);
            
            if (stats.arena_utilization() > 0.8 || stats.pool_utilization() > 0.8) {
                LOG_WARN("High memory usage detected!");
            }
        }
    }
    
    // Demonstrate memory cleanup
    LOG_INFO("Demonstrating memory cleanup...");
    limited_registry.compact_memory();
    
    // Clear and show memory reset
    limited_registry.clear();
    auto final_stats = limited_registry.get_memory_statistics();
    LOG_INFO("After cleanup - Arena: {:.1f}%, Pool: {:.1f}%",
            final_stats.arena_utilization() * 100.0,
            final_stats.pool_utilization() * 100.0);
}

void demonstrate_archetype_migration() {
    LOG_INFO("\\n=== Archetype Migration Demonstration ===");
    
    auto registry = ecs::create_educational_registry("Migration_Demo");
    
    // Create entity with just Position
    ecs::Entity entity = registry->create_entity<Position>(Position{0.0f, 0.0f, 0.0f});
    LOG_INFO("Created entity with Position component");
    
    auto stats_before = registry->get_memory_statistics();
    LOG_INFO("Archetypes before migration: {}", stats_before.total_archetypes);
    
    // Add Velocity component (should trigger archetype migration)
    bool success = registry->add_component<Velocity>(entity, Velocity{1.0f, 1.0f, 1.0f});
    if (success) {
        LOG_INFO("Successfully added Velocity component");
    }
    
    auto stats_after = registry->get_memory_statistics();
    LOG_INFO("Archetypes after migration: {}", stats_after.total_archetypes);
    
    // Add Health component (another migration)
    success = registry->add_component<Health>(entity, Health{100, 100});
    if (success) {
        LOG_INFO("Successfully added Health component");
    }
    
    auto stats_final = registry->get_memory_statistics();
    LOG_INFO("Final archetypes: {}", stats_final.total_archetypes);
    
    // Verify entity still has all components
    Position* pos = registry->get_component<Position>(entity);
    Velocity* vel = registry->get_component<Velocity>(entity);
    Health* health = registry->get_component<Health>(entity);
    
    LOG_INFO("Entity components after migration:");
    if (pos) LOG_INFO("  - Position: ({}, {}, {})", pos->x, pos->y, pos->z);
    if (vel) LOG_INFO("  - Velocity: ({}, {}, {})", vel->dx, vel->dy, vel->dz);
    if (health) LOG_INFO("  - Health: {}/{}", health->current, health->maximum);
}

int main() {
    LOG_INFO("ECScope ECS Memory Integration Demo");
    LOG_INFO("===================================");
    
    try {
        // Run all demonstrations
        demonstrate_basic_usage();
        demonstrate_performance_comparison();
        demonstrate_memory_pressure_handling();
        demonstrate_archetype_migration();
        
        // Run the comprehensive educational demo
        LOG_INFO("\\n=== Running Comprehensive Educational Demo ===");
        ecs::educational::run_memory_allocation_demo();
        
        LOG_INFO("\\n=== Demo Completed Successfully ===");
        
    } catch (const std::exception& e) {
        LOG_ERROR("Demo failed with exception: {}", e.what());
        return 1;
    } catch (...) {
        LOG_ERROR("Demo failed with unknown exception");
        return 1;
    }
    
    return 0;
}