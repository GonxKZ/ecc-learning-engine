#include <benchmark/benchmark.h>
#include <random>
#include <vector>
#include <memory>
#include <chrono>

#include "../include/ecscope/query/query_engine.hpp"
#include "../include/ecscope/query/query_builder.hpp"
#include "../include/ecscope/query/query_cache.hpp"
#include "../include/ecscope/query/advanced.hpp"
#include "../include/ecscope/registry.hpp"

using namespace ecscope::ecs;
using namespace ecscope::ecs::query;
using namespace ecscope::ecs::query::advanced;

// Benchmark components
struct BenchPosition {
    f32 x, y, z;
    BenchPosition(f32 x_ = 0, f32 y_ = 0, f32 z_ = 0) : x(x_), y(y_), z(z_) {}
};

struct BenchVelocity {
    f32 dx, dy, dz;
    f32 magnitude() const { return std::sqrt(dx*dx + dy*dy + dz*dz); }
    BenchVelocity(f32 dx_ = 0, f32 dy_ = 0, f32 dz_ = 0) : dx(dx_), dy(dy_), dz(dz_) {}
};

struct BenchHealth {
    f32 current, maximum;
    bool is_alive() const { return current > 0.0f; }
    f32 percentage() const { return maximum > 0 ? (current / maximum) : 0.0f; }
    BenchHealth(f32 max_hp = 100.0f) : current(max_hp), maximum(max_hp) {}
};

struct BenchTag {
    u32 value;
    BenchTag(u32 val = 0) : value(val) {}
};

// Register components
namespace ecscope::ecs {
    template<> struct Component<BenchPosition> : std::true_type {};
    template<> struct Component<BenchVelocity> : std::true_type {};
    template<> struct Component<BenchHealth> : std::true_type {};
    template<> struct Component<BenchTag> : std::true_type {};
}

// Benchmark fixture for consistent setup
class QueryEngineBenchmark : public benchmark::Fixture {
protected:
    void SetUp(const ::benchmark::State& state) override {
        // Setup registry with performance-optimized configuration
        auto config = AllocatorConfig::create_performance_optimized();
        registry = std::make_unique<Registry>(config, "BenchmarkRegistry");
        
        // Setup query engine with performance configuration
        auto query_config = QueryConfig::create_performance_optimized();
        engine = std::make_unique<QueryEngine>(registry.get(), query_config);
        
        // Setup advanced engine for comparison
        advanced_engine = std::make_unique<AdvancedQueryEngine>(registry.get(), query_config);
        
        // Create test dataset
        createBenchmarkDataset(static_cast<usize>(state.range(0)));
    }
    
    void TearDown(const ::benchmark::State& state) override {
        advanced_engine.reset();
        engine.reset();
        registry.reset();
    }
    
    void createBenchmarkDataset(usize entity_count) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<f32> pos_dist(-1000.0f, 1000.0f);
        std::uniform_real_distribution<f32> vel_dist(-50.0f, 50.0f);
        std::uniform_real_distribution<f32> health_dist(1.0f, 100.0f);
        std::uniform_int_distribution<u32> tag_dist(0, 100);
        
        entities.reserve(entity_count);
        
        // Create entities with all components (80% of entities)
        usize full_entities = static_cast<usize>(entity_count * 0.8);
        for (usize i = 0; i < full_entities; ++i) {
            auto entity = registry->create_entity<BenchPosition, BenchVelocity, BenchHealth, BenchTag>(
                BenchPosition(pos_dist(gen), pos_dist(gen), pos_dist(gen)),
                BenchVelocity(vel_dist(gen), vel_dist(gen), vel_dist(gen)),
                BenchHealth(health_dist(gen)),
                BenchTag(tag_dist(gen))
            );
            entities.push_back(entity);
        }
        
        // Create entities with position and velocity only (15% of entities)
        usize partial_entities = static_cast<usize>(entity_count * 0.15);
        for (usize i = 0; i < partial_entities; ++i) {
            auto entity = registry->create_entity<BenchPosition, BenchVelocity>(
                BenchPosition(pos_dist(gen), pos_dist(gen), pos_dist(gen)),
                BenchVelocity(vel_dist(gen), vel_dist(gen), vel_dist(gen))
            );
            entities.push_back(entity);
        }
        
        // Create entities with position only (5% of entities)
        usize remaining = entity_count - full_entities - partial_entities;
        for (usize i = 0; i < remaining; ++i) {
            auto entity = registry->create_entity<BenchPosition>(
                BenchPosition(pos_dist(gen), pos_dist(gen), pos_dist(gen))
            );
            entities.push_back(entity);
        }
    }
    
    std::unique_ptr<Registry> registry;
    std::unique_ptr<QueryEngine> engine;
    std::unique_ptr<AdvancedQueryEngine> advanced_engine;
    std::vector<Entity> entities;
};

// Basic Query Benchmarks
BENCHMARK_DEFINE_F(QueryEngineBenchmark, SingleComponentQuery)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = engine->query<BenchPosition>();
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * entities.size());
    state.SetLabel("entities=" + std::to_string(entities.size()));
}

BENCHMARK_REGISTER_F(QueryEngineBenchmark, SingleComponentQuery)
    ->Arg(1000)->Arg(5000)->Arg(10000)->Arg(50000)->Arg(100000);

BENCHMARK_DEFINE_F(QueryEngineBenchmark, MultiComponentQuery)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = engine->query<BenchPosition, BenchVelocity, BenchHealth>();
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * entities.size());
    state.SetLabel("entities=" + std::to_string(entities.size()));
}

BENCHMARK_REGISTER_F(QueryEngineBenchmark, MultiComponentQuery)
    ->Arg(1000)->Arg(5000)->Arg(10000)->Arg(50000)->Arg(100000);

// Predicate Query Benchmarks
BENCHMARK_DEFINE_F(QueryEngineBenchmark, PredicateQuery)(benchmark::State& state) {
    auto predicate = QueryPredicate<BenchHealth>([](const auto& tuple) {
        auto [entity, health] = tuple;
        return health && health->is_alive() && health->percentage() > 0.5f;
    }, "alive_above_half");
    
    for (auto _ : state) {
        auto result = engine->query_with_predicate<BenchHealth>(predicate);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * entities.size());
}

BENCHMARK_REGISTER_F(QueryEngineBenchmark, PredicateQuery)
    ->Arg(10000)->Arg(50000)->Arg(100000);

// Query Builder Benchmarks
BENCHMARK_DEFINE_F(QueryEngineBenchmark, FluentQueryBuilder)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = QueryBuilder<BenchPosition, BenchVelocity>{}
            .where([](BenchPosition* pos, BenchVelocity* vel) {
                return vel && vel->magnitude() > 10.0f;
            })
            .sort_by_entity([](const auto& a, const auto& b) {
                auto [a_entity, a_pos, a_vel] = a;
                auto [b_entity, b_pos, b_vel] = b;
                return a_vel->magnitude() > b_vel->magnitude();
            })
            .limit(100)
            .execute();
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * entities.size());
}

BENCHMARK_REGISTER_F(QueryEngineBenchmark, FluentQueryBuilder)
    ->Arg(10000)->Arg(50000);

// Cache Performance Benchmarks
BENCHMARK_DEFINE_F(QueryEngineBenchmark, CachedQueries)(benchmark::State& state) {
    // Warm up cache with identical queries
    for (int i = 0; i < 10; ++i) {
        auto result = engine->query<BenchPosition, BenchVelocity>();
        benchmark::DoNotOptimize(result);
    }
    
    for (auto _ : state) {
        auto result = engine->query<BenchPosition, BenchVelocity>();
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * entities.size());
}

BENCHMARK_REGISTER_F(QueryEngineBenchmark, CachedQueries)
    ->Arg(10000)->Arg(50000);

BENCHMARK_DEFINE_F(QueryEngineBenchmark, UncachedQueries)(benchmark::State& state) {
    usize query_variant = 0;
    
    for (auto _ : state) {
        // Clear cache to force fresh queries
        engine->clear_caches();
        
        // Vary the query slightly to avoid cache hits
        auto predicate = QueryPredicate<BenchTag>([query_variant](const auto& tuple) {
            auto [entity, tag] = tuple;
            return tag && tag->value == (query_variant % 100);
        }, "tag_query_" + std::to_string(query_variant));
        
        auto result = engine->query_with_predicate<BenchTag>(predicate);
        benchmark::DoNotOptimize(result);
        
        ++query_variant;
    }
    
    state.SetItemsProcessed(state.iterations() * entities.size());
}

BENCHMARK_REGISTER_F(QueryEngineBenchmark, UncachedQueries)
    ->Arg(10000)->Arg(50000);

// Spatial Query Benchmarks
BENCHMARK_DEFINE_F(QueryEngineBenchmark, SpatialRadiusQuery)(benchmark::State& state) {
    spatial::Vec3 center(0, 0, 0);
    
    for (auto _ : state) {
        auto result = engine->query_nearest<BenchPosition>(center, 50);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * entities.size());
}

BENCHMARK_REGISTER_F(QueryEngineBenchmark, SpatialRadiusQuery)
    ->Arg(10000)->Arg(50000);

// Parallel vs Sequential Benchmarks
BENCHMARK_DEFINE_F(QueryEngineBenchmark, SequentialExecution)(benchmark::State& state) {
    // Force sequential execution by setting low parallel threshold
    auto config = engine->get_config();
    config.enable_parallel_execution = false;
    engine->update_config(config);
    
    for (auto _ : state) {
        auto result = engine->query<BenchPosition, BenchVelocity, BenchHealth>();
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * entities.size());
    state.SetLabel("sequential");
}

BENCHMARK_REGISTER_F(QueryEngineBenchmark, SequentialExecution)
    ->Arg(50000)->Arg(100000);

BENCHMARK_DEFINE_F(QueryEngineBenchmark, ParallelExecution)(benchmark::State& state) {
    // Enable parallel execution
    auto config = engine->get_config();
    config.enable_parallel_execution = true;
    config.parallel_threshold = 1000;
    engine->update_config(config);
    
    for (auto _ : state) {
        auto result = engine->query<BenchPosition, BenchVelocity, BenchHealth>();
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * entities.size());
    state.SetLabel("parallel");
}

BENCHMARK_REGISTER_F(QueryEngineBenchmark, ParallelExecution)
    ->Arg(50000)->Arg(100000);

// Advanced Engine Benchmarks
BENCHMARK_DEFINE_F(QueryEngineBenchmark, AdvancedEngine)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = advanced_engine->query<BenchPosition, BenchVelocity, BenchHealth>();
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * entities.size());
    state.SetLabel("advanced");
}

BENCHMARK_REGISTER_F(QueryEngineBenchmark, AdvancedEngine)
    ->Arg(50000)->Arg(100000);

// Streaming Benchmarks
BENCHMARK_DEFINE_F(QueryEngineBenchmark, StreamingQuery)(benchmark::State& state) {
    auto streaming_processor = advanced_engine->create_streaming_processor<BenchPosition, BenchHealth>();
    
    for (auto _ : state) {
        usize count = 0;
        streaming_processor.stream_filter(
            QueryPredicate<BenchPosition, BenchHealth>([](const auto&) { return true; }, "all"),
            [&count](const auto& tuple) { ++count; }
        );
        benchmark::DoNotOptimize(count);
    }
    
    state.SetItemsProcessed(state.iterations() * entities.size());
}

BENCHMARK_REGISTER_F(QueryEngineBenchmark, StreamingQuery)
    ->Arg(50000);

// Memory Pressure Benchmarks
BENCHMARK_DEFINE_F(QueryEngineBenchmark, MemoryPressure)(benchmark::State& state) {
    std::vector<std::vector<std::tuple<Entity, BenchPosition*>>> results;
    
    for (auto _ : state) {
        auto result = engine->query<BenchPosition>();
        results.push_back(result.data()); // Force memory allocation
        
        // Periodically clear to simulate memory pressure
        if (results.size() > 10) {
            results.clear();
        }
        
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * entities.size());
}

BENCHMARK_REGISTER_F(QueryEngineBenchmark, MemoryPressure)
    ->Arg(10000);

// Component Access Pattern Benchmarks
BENCHMARK_DEFINE_F(QueryEngineBenchmark, RandomAccessPattern)(benchmark::State& state) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<usize> entity_dist(0, entities.size() - 1);
    
    for (auto _ : state) {
        // Access random entities
        for (int i = 0; i < 1000; ++i) {
            Entity entity = entities[entity_dist(gen)];
            auto pos = registry->get_component<BenchPosition>(entity);
            auto vel = registry->get_component<BenchVelocity>(entity);
            benchmark::DoNotOptimize(pos);
            benchmark::DoNotOptimize(vel);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * 1000);
}

BENCHMARK_REGISTER_F(QueryEngineBenchmark, RandomAccessPattern)
    ->Arg(10000);

BENCHMARK_DEFINE_F(QueryEngineBenchmark, SequentialAccessPattern)(benchmark::State& state) {
    for (auto _ : state) {
        // Access entities sequentially
        for (usize i = 0; i < std::min(1000ul, entities.size()); ++i) {
            Entity entity = entities[i];
            auto pos = registry->get_component<BenchPosition>(entity);
            auto vel = registry->get_component<BenchVelocity>(entity);
            benchmark::DoNotOptimize(pos);
            benchmark::DoNotOptimize(vel);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * std::min(1000ul, entities.size()));
}

BENCHMARK_REGISTER_F(QueryEngineBenchmark, SequentialAccessPattern)
    ->Arg(10000);

// Query Optimization Benchmarks
BENCHMARK_DEFINE_F(QueryEngineBenchmark, OptimizedQuery)(benchmark::State& state) {
    // Enable all optimizations
    auto config = engine->get_config();
    config.enable_caching = true;
    config.enable_parallel_execution = true;
    config.enable_spatial_optimization = true;
    config.enable_hot_path_optimization = true;
    engine->update_config(config);
    
    // Warm up optimizations
    for (int i = 0; i < 100; ++i) {
        auto result = engine->query<BenchPosition, BenchVelocity>();
        benchmark::DoNotOptimize(result);
    }
    
    for (auto _ : state) {
        auto result = engine->query<BenchPosition, BenchVelocity>();
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * entities.size());
    state.SetLabel("optimized");
}

BENCHMARK_REGISTER_F(QueryEngineBenchmark, OptimizedQuery)
    ->Arg(50000);

BENCHMARK_DEFINE_F(QueryEngineBenchmark, UnoptimizedQuery)(benchmark::State& state) {
    // Disable optimizations
    auto config = engine->get_config();
    config.enable_caching = false;
    config.enable_parallel_execution = false;
    config.enable_spatial_optimization = false;
    config.enable_hot_path_optimization = false;
    engine->update_config(config);
    
    for (auto _ : state) {
        auto result = engine->query<BenchPosition, BenchVelocity>();
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * entities.size());
    state.SetLabel("unoptimized");
}

BENCHMARK_REGISTER_F(QueryEngineBenchmark, UnoptimizedQuery)
    ->Arg(50000);

// Aggregation Benchmarks
BENCHMARK_DEFINE_F(QueryEngineBenchmark, AggregationQuery)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = QueryBuilder<BenchHealth>{}
            .sum([](const auto& tuple) -> f64 {
                auto [entity, health] = tuple;
                return health ? static_cast<f64>(health->current) : 0.0;
            })
            .execute_aggregation<f64>();
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * entities.size());
}

BENCHMARK_REGISTER_F(QueryEngineBenchmark, AggregationQuery)
    ->Arg(10000);

// Real-world scenario benchmarks
BENCHMARK_DEFINE_F(QueryEngineBenchmark, GameUpdateLoop)(benchmark::State& state) {
    // Simulate a typical game update loop with multiple queries
    for (auto _ : state) {
        // Query moving entities
        auto moving_entities = QueryBuilder<BenchPosition, BenchVelocity>{}
            .where([](BenchPosition* pos, BenchVelocity* vel) {
                return vel->magnitude() > 0.1f;
            })
            .execute();
        
        // Query entities needing health updates
        auto damaged_entities = QueryBuilder<BenchHealth>{}
            .where_component<BenchHealth>([](const BenchHealth& health) {
                return health.current < health.maximum;
            })
            .execute();
        
        // Spatial query for nearby entities
        auto nearby_entities = engine->query_nearest<BenchPosition>(spatial::Vec3(0, 0, 0), 20);
        
        benchmark::DoNotOptimize(moving_entities);
        benchmark::DoNotOptimize(damaged_entities);
        benchmark::DoNotOptimize(nearby_entities);
    }
    
    state.SetItemsProcessed(state.iterations() * entities.size());
}

BENCHMARK_REGISTER_F(QueryEngineBenchmark, GameUpdateLoop)
    ->Arg(10000)->Arg(50000);

// Comparative benchmark against raw iteration
static void RawIteration(benchmark::State& state) {
    // Setup similar to benchmark fixture but simpler
    std::vector<BenchPosition> positions;
    std::vector<BenchVelocity> velocities;
    std::vector<bool> has_velocity;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<f32> pos_dist(-1000.0f, 1000.0f);
    std::uniform_real_distribution<f32> vel_dist(-50.0f, 50.0f);
    
    usize entity_count = state.range(0);
    positions.reserve(entity_count);
    velocities.reserve(entity_count);
    has_velocity.reserve(entity_count);
    
    for (usize i = 0; i < entity_count; ++i) {
        positions.emplace_back(pos_dist(gen), pos_dist(gen), pos_dist(gen));
        velocities.emplace_back(vel_dist(gen), vel_dist(gen), vel_dist(gen));
        has_velocity.push_back(i % 5 != 0); // 80% have velocity
    }
    
    for (auto _ : state) {
        std::vector<std::pair<BenchPosition*, BenchVelocity*>> results;
        
        for (usize i = 0; i < entity_count; ++i) {
            if (has_velocity[i]) {
                results.emplace_back(&positions[i], &velocities[i]);
            }
        }
        
        benchmark::DoNotOptimize(results);
    }
    
    state.SetItemsProcessed(state.iterations() * entity_count);
    state.SetLabel("raw_iteration");
}

BENCHMARK(RawIteration)->Arg(10000)->Arg(50000)->Arg(100000);

BENCHMARK_MAIN();