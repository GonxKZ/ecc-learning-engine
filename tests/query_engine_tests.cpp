#include <gtest/gtest.h>
#include <chrono>
#include <random>
#include <vector>
#include <algorithm>

#include "../include/ecscope/query/query_engine.hpp"
#include "../include/ecscope/query/query_builder.hpp"
#include "../include/ecscope/query/query_cache.hpp"
#include "../include/ecscope/query/query_optimizer.hpp"
#include "../include/ecscope/query/spatial_queries.hpp"
#include "../include/ecscope/query/query_operators.hpp"
#include "../include/ecscope/query/advanced.hpp"
#include "../include/ecscope/registry.hpp"
#include "../include/ecscope/component.hpp"

using namespace ecscope::ecs;
using namespace ecscope::ecs::query;
using namespace ecscope::ecs::query::advanced;

// Test components
struct Position {
    f32 x, y, z;
    Position(f32 x_ = 0, f32 y_ = 0, f32 z_ = 0) : x(x_), y(y_), z(z_) {}
};

struct Velocity {
    f32 dx, dy, dz;
    f32 speed() const { return std::sqrt(dx*dx + dy*dy + dz*dz); }
    Velocity(f32 dx_ = 0, f32 dy_ = 0, f32 dz_ = 0) : dx(dx_), dy(dy_), dz(dz_) {}
};

struct Health {
    f32 current, maximum;
    bool is_alive() const { return current > 0.0f; }
    Health(f32 max_hp = 100.0f) : current(max_hp), maximum(max_hp) {}
};

struct Name {
    std::string value;
    Name(const std::string& name = "") : value(name) {}
};

struct Level {
    u32 value;
    Level(u32 lvl = 1) : value(lvl) {}
};

// Register components with the ECS system
namespace ecscope::ecs {
    template<> struct Component<Position> : std::true_type {};
    template<> struct Component<Velocity> : std::true_type {};
    template<> struct Component<Health> : std::true_type {};
    template<> struct Component<Name> : std::true_type {};
    template<> struct Component<Level> : std::true_type {};
}

class QueryEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create registry with performance-optimized config
        auto config = AllocatorConfig::create_performance_optimized();
        registry_ = std::make_unique<Registry>(config, "TestRegistry");
        
        // Create query engine
        query_config_ = QueryConfig::create_performance_optimized();
        query_engine_ = std::make_unique<QueryEngine>(registry_.get(), query_config_);
        
        // Create some test entities
        createTestEntities();
    }
    
    void TearDown() override {
        query_engine_.reset();
        registry_.reset();
    }
    
    void createTestEntities() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<f32> pos_dist(-100.0f, 100.0f);
        std::uniform_real_distribution<f32> vel_dist(-10.0f, 10.0f);
        std::uniform_real_distribution<f32> health_dist(1.0f, 100.0f);
        std::uniform_int_distribution<u32> level_dist(1, 50);
        
        // Create entities with different component combinations
        for (usize i = 0; i < 1000; ++i) {
            auto entity = registry_->create_entity<Position, Velocity, Health>(
                Position(pos_dist(gen), pos_dist(gen), pos_dist(gen)),
                Velocity(vel_dist(gen), vel_dist(gen), vel_dist(gen)),
                Health(health_dist(gen))
            );
            test_entities_.push_back(entity);
        }
        
        // Create some entities with names and levels
        std::vector<std::string> names = {
            "Warrior", "Mage", "Archer", "Rogue", "Paladin", "Necromancer", "Barbarian"
        };
        
        for (usize i = 0; i < 200; ++i) {
            auto entity = registry_->create_entity<Position, Name, Level>(
                Position(pos_dist(gen), pos_dist(gen), pos_dist(gen)),
                Name(names[i % names.size()] + std::to_string(i)),
                Level(level_dist(gen))
            );
            named_entities_.push_back(entity);
        }
        
        // Create some entities with only position for spatial tests
        for (usize i = 0; i < 500; ++i) {
            auto entity = registry_->create_entity<Position>(
                Position(pos_dist(gen), pos_dist(gen), pos_dist(gen))
            );
            position_only_entities_.push_back(entity);
        }
    }
    
    std::unique_ptr<Registry> registry_;
    std::unique_ptr<QueryEngine> query_engine_;
    QueryConfig query_config_;
    std::vector<Entity> test_entities_;
    std::vector<Entity> named_entities_;
    std::vector<Entity> position_only_entities_;
};

// Basic Query Engine Tests
TEST_F(QueryEngineTest, BasicQueryExecution) {
    auto result = query_engine_->query<Position>();
    
    EXPECT_GT(result.size(), 0);
    EXPECT_EQ(result.size(), test_entities_.size() + named_entities_.size() + position_only_entities_.size());
    
    // Check that all results have valid Position components
    for (const auto& [entity, position] : result) {
        EXPECT_NE(position, nullptr);
        EXPECT_TRUE(registry_->is_valid(entity));
    }
}

TEST_F(QueryEngineTest, MultiComponentQuery) {
    auto result = query_engine_->query<Position, Velocity, Health>();
    
    EXPECT_EQ(result.size(), test_entities_.size());
    
    for (const auto& [entity, pos, vel, health] : result) {
        EXPECT_NE(pos, nullptr);
        EXPECT_NE(vel, nullptr);
        EXPECT_NE(health, nullptr);
    }
}

TEST_F(QueryEngineTest, PredicateQuery) {
    using namespace operators;
    
    auto predicate = QueryPredicate<Health>([](const auto& tuple) {
        auto [entity, health] = tuple;
        return health && health->current > 50.0f;
    }, "health_above_50");
    
    auto result = query_engine_->query_with_predicate<Health>(predicate);
    
    for (const auto& [entity, health] : result) {
        EXPECT_GT(health->current, 50.0f);
    }
}

// Query Builder Tests
TEST_F(QueryEngineTest, FluentQueryBuilder) {
    auto result = QueryBuilder<Position, Velocity>{}
        .where([](Position* pos, Velocity* vel) {
            return pos && vel && vel->speed() > 5.0f;
        }, "fast_entities")
        .limit(10)
        .execute();
    
    EXPECT_LE(result.size(), 10);
    
    for (const auto& [entity, pos, vel] : result) {
        EXPECT_GT(vel->speed(), 5.0f);
    }
}

TEST_F(QueryEngineTest, QueryBuilderSorting) {
    auto result = QueryBuilder<Health>{}
        .sort_by_member(&Health::current, false) // Descending order
        .limit(5)
        .execute();
    
    EXPECT_LE(result.size(), 5);
    
    // Check that results are sorted in descending order of health
    for (usize i = 1; i < result.size(); ++i) {
        auto [prev_entity, prev_health] = result[i-1];
        auto [curr_entity, curr_health] = result[i];
        EXPECT_GE(prev_health->current, curr_health->current);
    }
}

TEST_F(QueryEngineTest, QueryBuilderRange) {
    auto result = QueryBuilder<Level>{}
        .where_range(&Level::value, 10u, 20u)
        .execute();
    
    for (const auto& [entity, level] : result) {
        EXPECT_GE(level->value, 10u);
        EXPECT_LE(level->value, 20u);
    }
}

// Cache Tests
TEST_F(QueryEngineTest, QueryCaching) {
    QueryCache cache(1000, 5.0);
    
    // Create a test result
    std::vector<std::tuple<Entity, Position*>> test_data;
    for (usize i = 0; i < 10; ++i) {
        test_data.emplace_back(test_entities_[i], 
                              registry_->get_component<Position>(test_entities_[i]));
    }
    QueryResult<Position> original_result(std::move(test_data));
    
    // Store in cache
    std::string test_key = "test_query";
    cache.store(test_key, original_result);
    
    // Retrieve from cache
    auto cached_result = cache.get<Position>(test_key);
    ASSERT_TRUE(cached_result.has_value());
    EXPECT_EQ(cached_result->size(), 10);
}

TEST_F(QueryEngineTest, BloomFilterTest) {
    BloomFilter bloom(1000, 0.01);
    
    std::vector<std::string> test_keys = {
        "query_1", "query_2", "query_3", "query_4", "query_5"
    };
    
    // Add keys to bloom filter
    for (const auto& key : test_keys) {
        bloom.add(key);
    }
    
    // Check that all added keys might be present
    for (const auto& key : test_keys) {
        EXPECT_TRUE(bloom.might_contain(key));
    }
    
    // Check that definitely not present keys return false
    EXPECT_FALSE(bloom.might_contain("definitely_not_present_key"));
}

// Spatial Query Tests
TEST_F(QueryEngineTest, SpatialRegionQuery) {
    using namespace spatial;
    
    AABB test_region(Vec3(-50, -50, -50), Vec3(50, 50, 50));
    auto region = Region::box(test_region.min, test_region.max);
    
    auto result = query_engine_->query_spatial<Position>(region);
    
    for (const auto& [entity, pos] : result) {
        Vec3 position(pos->x, pos->y, pos->z);
        EXPECT_TRUE(test_region.contains(position));
    }
}

TEST_F(QueryEngineTest, SpatialRadiusQuery) {
    spatial::Vec3 center(0, 0, 0);
    f32 radius = 25.0f;
    
    auto result = query_engine_->query_nearest<Position>(center, 10);
    
    EXPECT_LE(result.size(), 10);
    
    // Check that results are sorted by distance
    for (usize i = 1; i < result.size(); ++i) {
        auto [prev_entity, prev_pos] = result[i-1];
        auto [curr_entity, curr_pos] = result[i];
        
        f32 prev_dist = (spatial::Vec3(prev_pos->x, prev_pos->y, prev_pos->z) - center).length();
        f32 curr_dist = (spatial::Vec3(curr_pos->x, curr_pos->y, curr_pos->z) - center).length();
        
        EXPECT_LE(prev_dist, curr_dist);
    }
}

// Optimizer Tests
TEST_F(QueryEngineTest, QueryOptimizer) {
    QueryOptimizer optimizer;
    
    // Create a test predicate
    auto predicate = QueryPredicate<Position, Velocity>([](const auto& tuple) {
        return true; // Accept all
    }, "test_predicate");
    
    auto plan = optimizer.create_plan(*registry_, predicate);
    
    EXPECT_GT(plan.estimated_entities_to_process(), 0);
    EXPECT_GT(plan.estimated_selectivity(), 0);
    EXPECT_FALSE(plan.optimization_steps().empty());
}

// Performance Benchmarks
TEST_F(QueryEngineTest, PerformanceBenchmark) {
    const usize iterations = 100;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (usize i = 0; i < iterations; ++i) {
        auto result = query_engine_->query<Position, Velocity>();
        EXPECT_GT(result.size(), 0);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    f64 avg_time_us = static_cast<f64>(duration.count()) / iterations;
    
    // Performance target: queries should complete in under 1000 microseconds on average
    EXPECT_LT(avg_time_us, 1000.0);
    
    LOG_INFO("Average query time: {:.2f} µs", avg_time_us);
}

TEST_F(QueryEngineTest, CachePerformanceBenchmark) {
    const usize iterations = 1000;
    
    // First run without cache
    query_engine_->clear_caches();
    
    auto start_no_cache = std::chrono::high_resolution_clock::now();
    
    for (usize i = 0; i < iterations; ++i) {
        auto result = query_engine_->query<Position>();
    }
    
    auto end_no_cache = std::chrono::high_resolution_clock::now();
    auto duration_no_cache = std::chrono::duration_cast<std::chrono::microseconds>(end_no_cache - start_no_cache);
    
    // Second run with cache (same query repeated)
    auto start_with_cache = std::chrono::high_resolution_clock::now();
    
    for (usize i = 0; i < iterations; ++i) {
        auto result = query_engine_->query<Position>(); // Same query should hit cache
    }
    
    auto end_with_cache = std::chrono::high_resolution_clock::now();
    auto duration_with_cache = std::chrono::duration_cast<std::chrono::microseconds>(end_with_cache - start_with_cache);
    
    f64 speedup = static_cast<f64>(duration_no_cache.count()) / duration_with_cache.count();
    
    LOG_INFO("Cache speedup: {:.2f}x", speedup);
    
    // Cache should provide some speedup for repeated queries
    EXPECT_GT(speedup, 1.0);
}

// Advanced Features Tests
TEST_F(QueryEngineTest, AdvancedQueryEngine) {
    AdvancedQueryEngine advanced_engine(registry_.get());
    
    auto result = advanced_engine.query<Position, Velocity>();
    EXPECT_GT(result.size(), 0);
    
    // Test streaming processor
    auto streaming_processor = advanced_engine.create_streaming_processor<Position>();
    
    usize streamed_count = 0;
    streaming_processor.stream_filter(
        QueryPredicate<Position>([](const auto&) { return true; }, "all"),
        [&streamed_count](const auto& tuple) {
            streamed_count++;
        }
    );
    
    EXPECT_GT(streamed_count, 0);
}

TEST_F(QueryEngineTest, ParallelExecution) {
    ParallelQueryExecutor parallel_executor;
    
    std::vector<i32> test_data;
    for (i32 i = 0; i < 10000; ++i) {
        test_data.push_back(i);
    }
    
    auto filtered_data = parallel_executor.execute_parallel_filter(
        test_data, [](i32 value) { return value % 2 == 0; });
    
    EXPECT_EQ(filtered_data.size(), 5000);
    
    for (i32 value : filtered_data) {
        EXPECT_EQ(value % 2, 0);
    }
}

// Stress Tests
TEST_F(QueryEngineTest, StressTestLargeDataset) {
    // Create a larger dataset for stress testing
    std::vector<Entity> large_dataset;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<f32> dist(-1000.0f, 1000.0f);
    
    const usize entity_count = 10000;
    
    auto start_creation = std::chrono::high_resolution_clock::now();
    
    for (usize i = 0; i < entity_count; ++i) {
        auto entity = registry_->create_entity<Position, Velocity>(
            Position(dist(gen), dist(gen), dist(gen)),
            Velocity(dist(gen), dist(gen), dist(gen))
        );
        large_dataset.push_back(entity);
    }
    
    auto end_creation = std::chrono::high_resolution_clock::now();
    auto creation_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_creation - start_creation);
    
    LOG_INFO("Created {} entities in {} ms", entity_count, creation_time.count());
    
    // Query performance on large dataset
    auto start_query = std::chrono::high_resolution_clock::now();
    
    auto result = query_engine_->query<Position, Velocity>();
    
    auto end_query = std::chrono::high_resolution_clock::now();
    auto query_time = std::chrono::duration_cast<std::chrono::microseconds>(end_query - start_query);
    
    LOG_INFO("Queried {} entities in {} µs", result.size(), query_time.count());
    
    // Performance targets for large datasets
    EXPECT_GT(result.size(), entity_count); // Should include original test entities
    EXPECT_LT(query_time.count(), 10000); // Should complete in under 10ms
}

TEST_F(QueryEngineTest, MemoryUsageTest) {
    usize initial_memory = registry_->memory_usage();
    
    // Create many entities
    std::vector<Entity> entities;
    for (usize i = 0; i < 5000; ++i) {
        auto entity = registry_->create_entity<Position, Health>(
            Position(i, i, i), Health(100.0f)
        );
        entities.push_back(entity);
    }
    
    usize after_creation_memory = registry_->memory_usage();
    
    // Run queries
    for (usize i = 0; i < 100; ++i) {
        auto result = query_engine_->query<Position, Health>();
        EXPECT_GT(result.size(), 0);
    }
    
    usize after_queries_memory = registry_->memory_usage();
    
    LOG_INFO("Memory usage - Initial: {} KB, After creation: {} KB, After queries: {} KB",
             initial_memory / 1024, after_creation_memory / 1024, after_queries_memory / 1024);
    
    // Memory should not grow excessively from queries
    f64 memory_growth_ratio = static_cast<f64>(after_queries_memory) / after_creation_memory;
    EXPECT_LT(memory_growth_ratio, 1.5); // Less than 50% growth from queries
}

// Integration Tests
TEST_F(QueryEngineTest, IntegrationWithRegistry) {
    // Test that queries stay consistent as entities are added/removed
    auto initial_result = query_engine_->query<Position>();
    usize initial_count = initial_result.size();
    
    // Add more entities
    std::vector<Entity> new_entities;
    for (usize i = 0; i < 100; ++i) {
        auto entity = registry_->create_entity<Position>(Position(i, i, i));
        new_entities.push_back(entity);
    }
    
    auto after_add_result = query_engine_->query<Position>();
    EXPECT_EQ(after_add_result.size(), initial_count + 100);
    
    // Remove some entities
    for (usize i = 0; i < 50; ++i) {
        registry_->destroy_entity(new_entities[i]);
    }
    
    auto after_remove_result = query_engine_->query<Position>();
    EXPECT_EQ(after_remove_result.size(), initial_count + 50);
}

} // anonymous namespace

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}