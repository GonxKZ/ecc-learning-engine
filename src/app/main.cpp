#include "core/types.hpp"
#include "core/id.hpp"
#include "core/result.hpp"
#include "core/time.hpp"
#include "core/log.hpp"

// ECS includes
#include "ecs/entity.hpp"
#include "ecs/registry.hpp"
#include "ecs/components/transform.hpp"

#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include <random>
#include <vector>

using namespace ecscope::core;
using namespace ecscope::ecs;
using namespace ecscope::ecs::components;

// Demo function to showcase Result type
CoreResult<f64> divide(f64 a, f64 b) {
    if (b == 0.0) {
        return CoreResult<f64>::Err(CoreError::InvalidArgument);
    }
    return CoreResult<f64>::Ok(a / b);
}

int main() {
    // Welcome message
    LOG_INFO("ECScope v0.1.0 - Educational ECS Engine");
    LOG_INFO("Memory Observatory & Data Layout Laboratory");
    LOG_INFO("Built with C++20");
    
    // Show instrumentation status
#ifdef ECSCOPE_ENABLE_INSTRUMENTATION
    LOG_INFO("Instrumentation: ENABLED");
#else
    LOG_INFO("Instrumentation: DISABLED");
#endif
    
    LOG_INFO("=== Core Types Demo ===");
    
    // Demonstrate basic types
    u32 unsigned_val = 42;
    i32 signed_val = -42;
    f64 float_val = 3.14159;
    
    std::ostringstream oss;
    oss << "Types test - u32: " << unsigned_val << ", i32: " << signed_val << ", f64: " << float_val;
    LOG_INFO(oss.str());
    
    // Demonstrate EntityID system
    LOG_INFO("=== EntityID System Demo ===");
    
    auto& id_gen = entity_id_generator();
    EntityID entity1 = id_gen.create();
    EntityID entity2 = id_gen.create();
    EntityID entity3 = id_gen.create();
    
    oss.str("");
    oss << "Created entities - ID1: " << entity1.index << "/" << entity1.generation 
        << ", ID2: " << entity2.index << "/" << entity2.generation
        << ", ID3: " << entity3.index << "/" << entity3.generation;
    LOG_INFO(oss.str());
    
    // Test entity ID recycling
    EntityID recycled = id_gen.recycle(entity1.index, entity1.generation);
    oss.str("");
    oss << "Recycled entity1 - New: " << recycled.index << "/" << recycled.generation;
    LOG_INFO(oss.str());
    
    // Demonstrate Result type
    LOG_INFO("=== Result Type Demo ===");
    
    auto result1 = divide(10.0, 2.0);
    if (result1.is_ok()) {
        oss.str("");
        oss << "Division successful: 10.0 / 2.0 = " << result1.unwrap();
        LOG_INFO(oss.str());
    }
    
    auto result2 = divide(10.0, 0.0);
    if (result2.is_err()) {
        LOG_WARN("Division by zero detected - error handled gracefully");
    }
    
    // Demonstrate timing system
    LOG_INFO("=== Timing System Demo ===");
    
    Timer timer;
    
    // Simulate some work
    for (volatile int i = 0; i < 1000000; ++i) {
        // Busy work
    }
    
    oss.str("");
    oss << "Work completed in " << timer.elapsed_milliseconds() << " ms";
    LOG_INFO(oss.str());
    
    // Test scope timer
    {
        f64 scoped_time = 0.0;
        {
            ScopeTimer scope_timer(&scoped_time);
            // More work
            for (volatile int i = 0; i < 500000; ++i) {
                // Busy work
            }
        }
        oss.str("");
        oss << "Scoped work took " << scoped_time << " ms";
        LOG_INFO(oss.str());
    }
    
    // Demo delta time system
    auto& dt = delta_time();
    dt.update(); // Initialize
    
    // Simulate a few frame updates
    for (int frame = 1; frame <= 3; ++frame) {
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
        dt.update();
        
        oss.str("");
        oss << "Frame " << frame << " - Delta: " << dt.delta_milliseconds() 
            << " ms, FPS: " << static_cast<int>(dt.fps());
        LOG_INFO(oss.str());
    }
    
    // Show different log levels
    LOG_INFO("=== Logging System Demo ===");
    LOG_TRACE("This is a trace message (might not show depending on log level)");
    LOG_DEBUG("This is a debug message");
    LOG_INFO("This is an info message");
    LOG_WARN("This is a warning message");
    LOG_ERROR("This is an error message");
    
    LOG_INFO("=== Core Systems Initialized Successfully ===");
    
    // ECS System Demo
    LOG_INFO("=== ECS System Demo ===");
    
    // Get the global registry
    Registry& registry = get_registry();
    
    // Demonstrate ECS scalability test - create 10,000 entities
    LOG_INFO("Creating 10,000 entities with Transform components...");
    
    std::vector<Entity> entities;
    entities.reserve(10000);
    
    // Random number generator for diverse transform data
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<f32> pos_dist(-1000.0f, 1000.0f);
    std::uniform_real_distribution<f32> rot_dist(0.0f, 6.28318f); // 0 to 2π
    std::uniform_real_distribution<f32> scale_dist(0.5f, 2.0f);
    
    // Create entities with Transform components - benchmark this operation
    Timer creation_timer;
    for (usize i = 0; i < 10000; ++i) {
        Transform transform{
            Vec2{pos_dist(gen), pos_dist(gen)}, // Random position
            rot_dist(gen),                      // Random rotation
            Vec2{scale_dist(gen), scale_dist(gen)} // Random scale
        };
        
        Entity entity = registry.create_entity(std::move(transform));
        entities.push_back(entity);
    }
    
    oss.str("");
    oss << "Created 10,000 entities in " << creation_timer.elapsed_milliseconds() << " ms";
    LOG_INFO(oss.str());
    
    // Show registry statistics
    oss.str("");
    oss << "Registry stats - Total entities: " << registry.total_entities_created()
        << ", Active entities: " << registry.active_entities()
        << ", Archetypes: " << registry.archetype_count();
    LOG_INFO(oss.str());
    
    // Memory usage analysis
    oss.str("");
    oss << "Registry memory usage: " << (registry.memory_usage() / 1024.0f / 1024.0f) << " MB";
    LOG_INFO(oss.str());
    
    // Test component queries - find entities with Transform component
    Timer query_timer;
    auto entities_with_transform = registry.get_entities_with<Transform>();
    
    oss.str("");
    oss << "Query completed in " << query_timer.elapsed_microseconds() 
        << " μs - Found " << entities_with_transform.size() << " entities with Transform";
    LOG_INFO(oss.str());
    
    // Test component access - random sampling
    LOG_INFO("Testing component access performance...");
    Timer access_timer;
    usize valid_access_count = 0;
    
    std::uniform_int_distribution<usize> entity_dist(0, entities.size() - 1);
    for (int i = 0; i < 1000; ++i) {
        Entity random_entity = entities[entity_dist(gen)];
        Transform* transform = registry.get_component<Transform>(random_entity);
        if (transform != nullptr) {
            ++valid_access_count;
            // Do something with the transform to prevent optimization
            transform->translate(Vec2{0.01f, 0.01f});
        }
    }
    
    oss.str("");
    oss << "1000 component accesses completed in " << access_timer.elapsed_microseconds()
        << " μs - " << valid_access_count << " successful accesses";
    LOG_INFO(oss.str());
    
    // Test iteration over all entities
    LOG_INFO("Testing entity iteration performance...");
    Timer iteration_timer;
    usize iteration_count = 0;
    
    registry.for_each<Transform>([&iteration_count](Entity entity, Transform& transform) {
        ++iteration_count;
        // Simple transformation to simulate work
        transform.rotate(0.001f);
    });
    
    oss.str("");
    oss << "Iterated over " << iteration_count << " entities in " 
        << iteration_timer.elapsed_milliseconds() << " ms";
    LOG_INFO(oss.str());
    
    // Test component addition (on a subset to avoid performance hit)
    LOG_INFO("Testing component addition...");
    Timer addition_timer;
    usize added_components = 0;
    
    // Add Vec2 component to first 100 entities as velocity
    for (usize i = 0; i < 100 && i < entities.size(); ++i) {
        Vec2 velocity{pos_dist(gen) * 0.1f, pos_dist(gen) * 0.1f};
        if (registry.add_component<Vec2>(entities[i], velocity)) {
            ++added_components;
        }
    }
    
    oss.str("");
    oss << "Added " << added_components << " Vec2 components in " 
        << addition_timer.elapsed_milliseconds() << " ms";
    LOG_INFO(oss.str());
    
    // Test entity destruction (destroy 1000 entities)
    LOG_INFO("Testing entity destruction...");
    Timer destruction_timer;
    usize destroyed_entities = 0;
    
    for (usize i = 0; i < 1000 && i < entities.size(); ++i) {
        if (registry.destroy_entity(entities[i])) {
            ++destroyed_entities;
        }
    }
    
    oss.str("");
    oss << "Destroyed " << destroyed_entities << " entities in " 
        << destruction_timer.elapsed_milliseconds() << " ms";
    LOG_INFO(oss.str());
    
    // Final registry statistics
    oss.str("");
    oss << "Final registry stats - Active entities: " << registry.active_entities()
        << ", Memory usage: " << (registry.memory_usage() / 1024.0f / 1024.0f) << " MB";
    LOG_INFO(oss.str());
    
    // Show archetype distribution
    auto archetype_stats = registry.get_archetype_stats();
    LOG_INFO("Archetype distribution:");
    for (const auto& [signature, count] : archetype_stats) {
        oss.str("");
        oss << "  Archetype (components: " << signature.count() << "): " << count << " entities";
        LOG_INFO(oss.str());
    }
    
    LOG_INFO("=== ECS Demo Completed Successfully ===");
    LOG_INFO("Phase 2: ECS Mínimo - ✓ Complete");
    LOG_INFO("Ready for Phase 3: UI Base implementation...");
    
    return 0;
}