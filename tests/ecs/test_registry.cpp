#include "../framework/ecscope_test_framework.hpp"

namespace ECScope::Testing {

class RegistryTest : public ECScopeTestFixture {
protected:
    void SetUp() override {
        ECScopeTestFixture::SetUp();
        factory_ = std::make_unique<EntityFactory>(*world_);
    }

protected:
    std::unique_ptr<EntityFactory> factory_;
};

// =============================================================================
// Basic Entity Operations Tests
// =============================================================================

TEST_F(RegistryTest, CreateEntity) {
    auto entity = world_->create_entity();
    EXPECT_NE(entity, INVALID_ENTITY);
    EXPECT_TRUE(world_->is_valid(entity));
}

TEST_F(RegistryTest, CreateMultipleEntities) {
    constexpr size_t count = 1000;
    std::vector<Entity> entities;
    entities.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        entities.push_back(world_->create_entity());
    }

    // Verify all entities are valid and unique
    for (size_t i = 0; i < count; ++i) {
        EXPECT_TRUE(world_->is_valid(entities[i]));
        for (size_t j = i + 1; j < count; ++j) {
            EXPECT_NE(entities[i], entities[j]);
        }
    }
}

TEST_F(RegistryTest, DestroyEntity) {
    auto entity = world_->create_entity();
    EXPECT_TRUE(world_->is_valid(entity));
    
    world_->destroy_entity(entity);
    EXPECT_FALSE(world_->is_valid(entity));
}

TEST_F(RegistryTest, EntityReuse) {
    auto entity1 = world_->create_entity();
    auto original_id = entity1;
    
    world_->destroy_entity(entity1);
    auto entity2 = world_->create_entity();
    
    // Entity IDs should be different due to generation counter
    EXPECT_NE(original_id, entity2);
    EXPECT_FALSE(world_->is_valid(entity1));
    EXPECT_TRUE(world_->is_valid(entity2));
}

// =============================================================================
// Component Management Tests
// =============================================================================

TEST_F(RegistryTest, AddComponent) {
    auto entity = world_->create_entity();
    TestPosition pos{1.0f, 2.0f, 3.0f};
    
    world_->add_component<TestPosition>(entity, pos);
    
    EXPECT_TRUE(world_->has_component<TestPosition>(entity));
    auto& retrieved_pos = world_->get_component<TestPosition>(entity);
    EXPECT_EQ(retrieved_pos, pos);
}

TEST_F(RegistryTest, AddMultipleComponents) {
    auto entity = world_->create_entity();
    TestPosition pos{1.0f, 2.0f, 3.0f};
    TestVelocity vel{4.0f, 5.0f, 6.0f};
    TestHealth health{50, 100};
    
    world_->add_component<TestPosition>(entity, pos);
    world_->add_component<TestVelocity>(entity, vel);
    world_->add_component<TestHealth>(entity, health);
    
    EXPECT_TRUE(world_->has_component<TestPosition>(entity));
    EXPECT_TRUE(world_->has_component<TestVelocity>(entity));
    EXPECT_TRUE(world_->has_component<TestHealth>(entity));
    
    EXPECT_EQ(world_->get_component<TestPosition>(entity), pos);
    EXPECT_EQ(world_->get_component<TestVelocity>(entity), vel);
    EXPECT_EQ(world_->get_component<TestHealth>(entity), health);
}

TEST_F(RegistryTest, RemoveComponent) {
    auto entity = factory_->create_full_entity();
    
    EXPECT_TRUE(world_->has_component<TestPosition>(entity));
    world_->remove_component<TestPosition>(entity);
    EXPECT_FALSE(world_->has_component<TestPosition>(entity));
    
    // Other components should remain
    EXPECT_TRUE(world_->has_component<TestVelocity>(entity));
    EXPECT_TRUE(world_->has_component<TestHealth>(entity));
    EXPECT_TRUE(world_->has_component<TestTag>(entity));
}

TEST_F(RegistryTest, ComponentModification) {
    auto entity = factory_->create_positioned(1.0f, 2.0f, 3.0f);
    
    auto& pos = world_->get_component<TestPosition>(entity);
    pos.x = 10.0f;
    pos.y = 20.0f;
    pos.z = 30.0f;
    
    auto& retrieved_pos = world_->get_component<TestPosition>(entity);
    EXPECT_EQ(retrieved_pos.x, 10.0f);
    EXPECT_EQ(retrieved_pos.y, 20.0f);
    EXPECT_EQ(retrieved_pos.z, 30.0f);
}

// =============================================================================
// Query System Tests
// =============================================================================

TEST_F(RegistryTest, SimpleQuery) {
    // Create entities with different component combinations
    factory_->create_positioned(1, 2, 3);
    factory_->create_moving(4, 5, 6, 1, 1, 1);
    factory_->create_with_health(75, 100);
    factory_->create_full_entity();
    
    size_t position_count = 0;
    world_->each<TestPosition>([&](Entity entity, TestPosition& pos) {
        EXPECT_TRUE(world_->has_component<TestPosition>(entity));
        position_count++;
    });
    
    EXPECT_EQ(position_count, 3); // positioned, moving, and full entity
}

TEST_F(RegistryTest, MultiComponentQuery) {
    factory_->create_positioned(1, 2, 3);
    factory_->create_moving(4, 5, 6, 1, 1, 1);
    factory_->create_with_health(75, 100);
    factory_->create_full_entity();
    
    size_t moving_count = 0;
    world_->each<TestPosition, TestVelocity>([&](Entity entity, TestPosition& pos, TestVelocity& vel) {
        EXPECT_TRUE(world_->has_component<TestPosition>(entity));
        EXPECT_TRUE(world_->has_component<TestVelocity>(entity));
        moving_count++;
    });
    
    EXPECT_EQ(moving_count, 2); // moving and full entity
}

TEST_F(RegistryTest, QueryWithEntityDestruction) {
    auto entities = factory_->create_many(100, true);
    
    // Destroy every other entity
    for (size_t i = 0; i < entities.size(); i += 2) {
        world_->destroy_entity(entities[i]);
    }
    
    size_t remaining_count = 0;
    world_->each<TestPosition, TestVelocity>([&](Entity entity, TestPosition& pos, TestVelocity& vel) {
        EXPECT_TRUE(world_->is_valid(entity));
        remaining_count++;
    });
    
    EXPECT_EQ(remaining_count, 50); // Half of the original entities
}

// =============================================================================
// Archetype System Tests
// =============================================================================

TEST_F(RegistryTest, ArchetypeCreation) {
    // Create entities with different component signatures
    auto entity1 = factory_->create_positioned();
    auto entity2 = factory_->create_moving();
    auto entity3 = factory_->create_with_health();
    auto entity4 = factory_->create_full_entity();
    
    // Each should create different archetypes
    EXPECT_TRUE(world_->is_valid(entity1));
    EXPECT_TRUE(world_->is_valid(entity2));
    EXPECT_TRUE(world_->is_valid(entity3));
    EXPECT_TRUE(world_->is_valid(entity4));
}

TEST_F(RegistryTest, ArchetypeTransition) {
    auto entity = factory_->create_positioned();
    
    // Add velocity component - should trigger archetype change
    world_->add_component<TestVelocity>(entity, TestVelocity{1, 2, 3});
    
    EXPECT_TRUE(world_->has_component<TestPosition>(entity));
    EXPECT_TRUE(world_->has_component<TestVelocity>(entity));
    
    // Remove position component - should trigger another archetype change
    world_->remove_component<TestPosition>(entity);
    
    EXPECT_FALSE(world_->has_component<TestPosition>(entity));
    EXPECT_TRUE(world_->has_component<TestVelocity>(entity));
}

// =============================================================================
// Performance Tests
// =============================================================================

TEST_F(RegistryTest, EntityCreationPerformance) {
    constexpr size_t entity_count = 100000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<Entity> entities;
    entities.reserve(entity_count);
    for (size_t i = 0; i < entity_count; ++i) {
        entities.push_back(world_->create_entity());
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Created " << entity_count << " entities in " 
              << duration.count() << " microseconds\n";
    
    // Should be able to create entities relatively quickly
    EXPECT_LT(duration.count(), 100000); // Less than 100ms
}

TEST_F(RegistryTest, ComponentAdditionPerformance) {
    constexpr size_t entity_count = 50000;
    auto entities = factory_->create_many(entity_count, false);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (auto entity : entities) {
        world_->add_component<TestVelocity>(entity, TestVelocity{1, 2, 3});
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Added components to " << entity_count << " entities in " 
              << duration.count() << " microseconds\n";
    
    // Component addition should be efficient
    EXPECT_LT(duration.count(), 200000); // Less than 200ms
}

TEST_F(RegistryTest, QueryPerformance) {
    constexpr size_t entity_count = 100000;
    factory_->create_many(entity_count, true);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    size_t processed = 0;
    world_->each<TestPosition, TestVelocity>([&](Entity entity, TestPosition& pos, TestVelocity& vel) {
        // Simulate simple processing
        pos.x += vel.vx * 0.016f;
        pos.y += vel.vy * 0.016f;
        pos.z += vel.vz * 0.016f;
        processed++;
    });
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Processed " << processed << " entities in " 
              << duration.count() << " microseconds\n";
    
    EXPECT_EQ(processed, entity_count);
    // Query processing should be very fast
    EXPECT_LT(duration.count(), 50000); // Less than 50ms
}

// =============================================================================
// Sparse Set Tests
// =============================================================================

TEST_F(RegistryTest, SparseSetDensityOptimization) {
    constexpr size_t total_entities = 10000;
    constexpr size_t component_entities = 100; // Only 1% have the component
    
    // Create many entities
    auto entities = factory_->create_many(total_entities, false);
    
    // Add TestPosition to only a small subset (sparse scenario)
    for (size_t i = 0; i < component_entities; ++i) {
        world_->add_component<TestPosition>(entities[i], TestPosition{static_cast<float>(i), 0, 0});
    }
    
    // Query should still be efficient despite sparse distribution
    auto start = std::chrono::high_resolution_clock::now();
    size_t count = 0;
    world_->each<TestPosition>([&](Entity entity, TestPosition& pos) {
        count++;
        EXPECT_GE(pos.x, 0.0f);
    });
    auto end = std::chrono::high_resolution_clock::now();
    
    EXPECT_EQ(count, component_entities);
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Sparse set query (" << component_entities << "/" << total_entities 
              << ") took " << duration.count() << " microseconds\n";
    
    // Should be fast despite sparsity
    EXPECT_LT(duration.count(), 1000); // Less than 1ms
}

TEST_F(RegistryTest, SparseSetFragmentation) {
    constexpr size_t entity_count = 1000;
    auto entities = factory_->create_many(entity_count, false);
    
    // Add components in a fragmented pattern (every 3rd entity)
    for (size_t i = 0; i < entity_count; i += 3) {
        world_->add_component<TestPosition>(entities[i], TestPosition{static_cast<float>(i), 0, 0});
    }
    
    // Remove every 2nd component-bearing entity
    for (size_t i = 0; i < entity_count; i += 6) {
        world_->remove_component<TestPosition>(entities[i]);
    }
    
    // Count remaining components
    size_t remaining = 0;
    world_->each<TestPosition>([&](Entity, TestPosition&) { remaining++; });
    
    size_t expected = entity_count / 3 - entity_count / 6;
    EXPECT_EQ(remaining, expected);
}

// =============================================================================
// Enhanced Query System Tests
// =============================================================================

TEST_F(RegistryTest, ComplexQueryCombinations) {
    // Create entities with various component combinations
    auto e1 = world_->create_entity(); // Position only
    world_->add_component<TestPosition>(e1, TestPosition{1, 0, 0});
    
    auto e2 = world_->create_entity(); // Position + Velocity
    world_->add_component<TestPosition>(e2, TestPosition{2, 0, 0});
    world_->add_component<TestVelocity>(e2, TestVelocity{1, 0, 0});
    
    auto e3 = world_->create_entity(); // Position + Health
    world_->add_component<TestPosition>(e3, TestPosition{3, 0, 0});
    world_->add_component<TestHealth>(e3, TestHealth{50, 100});
    
    auto e4 = world_->create_entity(); // All components
    world_->add_component<TestPosition>(e4, TestPosition{4, 0, 0});
    world_->add_component<TestVelocity>(e4, TestVelocity{2, 0, 0});
    world_->add_component<TestHealth>(e4, TestHealth{75, 100});
    world_->add_component<TestTag>(e4, TestTag{"complex"});
    
    // Test different query combinations
    size_t pos_only = 0;
    world_->each<TestPosition>([&](Entity, TestPosition&) { pos_only++; });
    EXPECT_EQ(pos_only, 4);
    
    size_t pos_vel = 0;
    world_->each<TestPosition, TestVelocity>([&](Entity, TestPosition&, TestVelocity&) { pos_vel++; });
    EXPECT_EQ(pos_vel, 2);
    
    size_t pos_health = 0;
    world_->each<TestPosition, TestHealth>([&](Entity, TestPosition&, TestHealth&) { pos_health++; });
    EXPECT_EQ(pos_health, 2);
    
    size_t all_four = 0;
    world_->each<TestPosition, TestVelocity, TestHealth, TestTag>(
        [&](Entity, TestPosition&, TestVelocity&, TestHealth&, TestTag&) { all_four++; });
    EXPECT_EQ(all_four, 1);
}

TEST_F(RegistryTest, QueryWithFiltering) {
    // Create entities with health components
    for (int i = 0; i < 100; ++i) {
        auto entity = world_->create_entity();
        world_->add_component<TestHealth>(entity, TestHealth{i, 100});
        world_->add_component<TestPosition>(entity, TestPosition{static_cast<float>(i), 0, 0});
    }
    
    // Count entities with health > 50
    size_t healthy_count = 0;
    world_->each<TestHealth, TestPosition>([&](Entity, TestHealth& health, TestPosition& pos) {
        if (health.health > 50) {
            healthy_count++;
            EXPECT_GT(pos.x, 50.0f); // Should correlate with health
        }
    });
    
    EXPECT_EQ(healthy_count, 49); // Health 51-99
}

// =============================================================================
// System Dependencies Tests
// =============================================================================

TEST_F(RegistryTest, SystemDependencyTracking) {
    // Create a mock system dependency scenario
    struct SystemATag {};
    struct SystemBTag {};
    struct SystemCTag {};
    
    auto entity = world_->create_entity();
    
    // System A processes first, adds its tag
    world_->add_component<TestPosition>(entity, TestPosition{0, 0, 0});
    world_->add_component<SystemATag>(entity);
    
    // System B depends on A, processes position
    bool system_a_present = world_->has_component<SystemATag>(entity);
    EXPECT_TRUE(system_a_present);
    
    if (system_a_present) {
        auto& pos = world_->get_component<TestPosition>(entity);
        pos.x = 10.0f; // System B processing
        world_->add_component<SystemBTag>(entity);
    }
    
    // System C depends on B
    if (world_->has_component<SystemBTag>(entity)) {
        auto& pos = world_->get_component<TestPosition>(entity);
        EXPECT_EQ(pos.x, 10.0f); // Verify B ran first
        pos.y = 20.0f; // System C processing
        world_->add_component<SystemCTag>(entity);
    }
    
    // Verify proper execution order
    EXPECT_TRUE(world_->has_component<SystemATag>(entity));
    EXPECT_TRUE(world_->has_component<SystemBTag>(entity));
    EXPECT_TRUE(world_->has_component<SystemCTag>(entity));
    
    auto& final_pos = world_->get_component<TestPosition>(entity);
    EXPECT_EQ(final_pos.x, 10.0f);
    EXPECT_EQ(final_pos.y, 20.0f);
}

// =============================================================================
// Memory Management Tests
// =============================================================================

TEST_F(RegistryTest, MemoryLeakDetection) {
    constexpr size_t entity_count = 1000;
    
    // Create and destroy entities multiple times
    for (int cycle = 0; cycle < 10; ++cycle) {
        auto entities = factory_->create_many(entity_count, true);
        
        // Add more components
        for (auto entity : entities) {
            world_->add_component<TestHealth>(entity, TestHealth{100, 100});
            world_->add_component<TestTag>(entity, TestTag{"test"});
        }
        
        // Destroy all entities
        for (auto entity : entities) {
            world_->destroy_entity(entity);
        }
    }
    
    // Memory should be properly cleaned up
    EXPECT_NO_MEMORY_LEAKS();
}

TEST_F(RegistryTest, LargeComponentHandling) {
    constexpr size_t entity_count = 1000;
    std::vector<Entity> entities;
    
    for (size_t i = 0; i < entity_count; ++i) {
        auto entity = world_->create_entity();
        world_->add_component<LargeTestComponent>(entity);
        entities.push_back(entity);
    }
    
    // Verify all components are properly stored
    for (auto entity : entities) {
        EXPECT_TRUE(world_->has_component<LargeTestComponent>(entity));
        auto& component = world_->get_component<LargeTestComponent>(entity);
        EXPECT_EQ(component.data[0], 0.0); // Default initialized
    }
    
    EXPECT_NO_MEMORY_LEAKS();
}

// =============================================================================
// Component Storage Optimization Tests
// =============================================================================

TEST_F(RegistryTest, ComponentStorageAlignment) {
    // Test that components are properly aligned in memory
    constexpr size_t entity_count = 100;
    
    std::vector<Entity> entities;
    for (size_t i = 0; i < entity_count; ++i) {
        auto entity = world_->create_entity();
        world_->add_component<TestPosition>(entity, TestPosition{static_cast<float>(i), 0, 0});
        entities.push_back(entity);
    }
    
    // Check that components are stored contiguously for cache efficiency
    TestPosition* first_component = nullptr;
    world_->each<TestPosition>([&](Entity entity, TestPosition& pos) {
        if (first_component == nullptr) {
            first_component = &pos;
        }
    });
    
    ASSERT_NE(first_component, nullptr);
    
    // Verify memory alignment (components should be aligned to their natural alignment)
    uintptr_t addr = reinterpret_cast<uintptr_t>(first_component);
    EXPECT_EQ(addr % alignof(TestPosition), 0);
}

TEST_F(RegistryTest, ArchetypeStability) {
    // Test that archetype storage remains stable during component operations
    auto entity = world_->create_entity();
    world_->add_component<TestPosition>(entity, TestPosition{1, 2, 3});
    
    TestPosition* pos_ptr = &world_->get_component<TestPosition>(entity);
    
    // Add another component to the same entity (might trigger archetype change)
    world_->add_component<TestVelocity>(entity, TestVelocity{4, 5, 6});
    
    // Original component should still be accessible and have correct data
    EXPECT_TRUE(world_->has_component<TestPosition>(entity));
    auto& pos = world_->get_component<TestPosition>(entity);
    EXPECT_EQ(pos.x, 1.0f);
    EXPECT_EQ(pos.y, 2.0f);
    EXPECT_EQ(pos.z, 3.0f);
    
    // Note: After archetype transition, pointer might be different
    // This is expected behavior for optimal memory layout
}

TEST_F(RegistryTest, ConcurrentComponentAccess) {
    // Test thread-safety of component access (if applicable)
    constexpr size_t entity_count = 1000;
    auto entities = factory_->create_many(entity_count, true);
    
    std::atomic<size_t> processed_count{0};
    
    // Simulate concurrent read access
    auto read_worker = [&]() {
        world_->each<TestPosition, TestVelocity>([&](Entity entity, TestPosition& pos, TestVelocity& vel) {
            // Read-only operations
            volatile float sum = pos.x + pos.y + pos.z + vel.vx + vel.vy + vel.vz;
            (void)sum; // Suppress unused variable warning
            processed_count++;
        });
    };
    
    // Run the read operation (single-threaded for now, but tests the pattern)
    read_worker();
    
    EXPECT_EQ(processed_count.load(), entity_count);
}

// =============================================================================
// Advanced Performance Tests
// =============================================================================

TEST_F(RegistryTest, CacheLocalityBenchmark) {
    constexpr size_t entity_count = 100000;
    factory_->create_many(entity_count, true);
    
    // Test cache-friendly iteration
    auto start = std::chrono::high_resolution_clock::now();
    
    size_t iterations = 0;
    for (int pass = 0; pass < 10; ++pass) {
        world_->each<TestPosition, TestVelocity>([&](Entity, TestPosition& pos, TestVelocity& vel) {
            // Cache-friendly operations
            pos.x += vel.vx * 0.016f;
            pos.y += vel.vy * 0.016f;
            pos.z += vel.vz * 0.016f;
            iterations++;
        });
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    
    double ns_per_iteration = static_cast<double>(duration.count()) / iterations;
    std::cout << "Cache locality test: " << ns_per_iteration << " ns/iteration\n";
    
    // Should be very fast due to good cache locality
    EXPECT_LT(ns_per_iteration, 50.0); // Less than 50ns per entity per pass
}

} // namespace ECScope::Testing