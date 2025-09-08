/**
 * @file registry_performance_benchmark.cpp
 * @brief Comprehensive performance benchmarks for the world-class ECS Registry
 * 
 * This benchmark suite validates the performance goals:
 * - Handle millions of entities efficiently
 * - Sub-microsecond component access
 * - Vectorized bulk operations
 * - Cache-friendly memory patterns
 * - Lock-free hot paths where possible
 * 
 * Benchmarks include:
 * - Entity creation/destruction throughput
 * - Component access latency and bandwidth
 * - Query performance with various complexity
 * - Archetype transition performance
 * - Memory usage efficiency
 * - Thread scalability
 * - Cache performance analysis
 */

#include <ecscope/registry/registry.hpp>
#include <ecscope/components.hpp>
#include <iostream>
#include <chrono>
#include <random>
#include <vector>
#include <thread>
#include <algorithm>
#include <numeric>
#include <iomanip>

using namespace ecscope::registry;
using namespace ecscope::foundation;
using namespace ecscope::core;

// Benchmark components
struct Position { float x, y, z; };
struct Velocity { float dx, dy, dz; };
struct Mass { float value; };
struct Tag { uint32_t id; };

ECSCOPE_REGISTER_COMPONENT(Position, "Position");
ECSCOPE_REGISTER_COMPONENT(Velocity, "Velocity");
ECSCOPE_REGISTER_COMPONENT(Mass, "Mass");
ECSCOPE_REGISTER_COMPONENT(Tag, "Tag");

class RegistryBenchmark {
public:
    RegistryBenchmark() {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "=== ECS Registry Performance Benchmark Suite ===\n\n";
    }
    
    void run_all_benchmarks() {
        benchmark_entity_operations();
        benchmark_component_access();
        benchmark_bulk_operations();
        benchmark_query_performance();
        benchmark_archetype_transitions();
        benchmark_memory_efficiency();
        benchmark_thread_scalability();
        benchmark_cache_performance();
        
        std::cout << "\n=== Benchmark Suite Complete ===\n";
        print_performance_summary();
    }

private:
    struct BenchmarkResult {
        std::string name;
        double operations_per_second;
        double average_latency_ns;
        double memory_mb;
        bool passed_target;
    };
    
    std::vector<BenchmarkResult> results_;

    void benchmark_entity_operations() {
        std::cout << "🚀 Entity Operations Benchmark\n";
        std::cout << "================================\n";
        
        auto registry = registry_factory::create_simulation_registry(10000000);
        
        // Test 1: Entity creation throughput
        {
            const size_t entity_count = 1000000;
            auto start = std::chrono::high_resolution_clock::now();
            
            auto entities = registry->create_entities(entity_count);
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            double ops_per_second = (entity_count * 1000000.0) / duration.count();
            double latency_ns = duration.count() * 1000.0 / entity_count;
            
            std::cout << "Entity Creation:\n";
            std::cout << "  Created " << entity_count << " entities in " << duration.count() << " μs\n";
            std::cout << "  Throughput: " << ops_per_second << " entities/second\n";
            std::cout << "  Average latency: " << latency_ns << " ns/entity\n";
            
            bool passed = ops_per_second > 1000000.0; // Target: > 1M entities/second
            std::cout << "  " << (passed ? "✅ PASSED" : "❌ FAILED") << " (Target: > 1M entities/sec)\n\n";
            
            results_.push_back({"Entity Creation", ops_per_second, latency_ns, 0.0, passed});
        }
        
        // Test 2: Entity destruction throughput
        {
            const size_t entity_count = 500000;
            auto entities = registry->create_entities(entity_count);
            
            auto start = std::chrono::high_resolution_clock::now();
            
            auto destroyed = registry->destroy_entities(entities);
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            double ops_per_second = (destroyed * 1000000.0) / duration.count();
            double latency_ns = duration.count() * 1000.0 / destroyed;
            
            std::cout << "Entity Destruction:\n";
            std::cout << "  Destroyed " << destroyed << " entities in " << duration.count() << " μs\n";
            std::cout << "  Throughput: " << ops_per_second << " entities/second\n";
            std::cout << "  Average latency: " << latency_ns << " ns/entity\n";
            
            bool passed = ops_per_second > 800000.0; // Target: > 800K entities/second
            std::cout << "  " << (passed ? "✅ PASSED" : "❌ FAILED") << " (Target: > 800K entities/sec)\n\n";
            
            results_.push_back({"Entity Destruction", ops_per_second, latency_ns, 0.0, passed});
        }
    }
    
    void benchmark_component_access() {
        std::cout << "⚡ Component Access Benchmark\n";
        std::cout << "===============================\n";
        
        auto registry = registry_factory::create_game_registry(1000000);
        
        // Create test entities with components
        const size_t entity_count = 100000;
        std::vector<EntityHandle> entities;
        entities.reserve(entity_count);
        
        for (size_t i = 0; i < entity_count; ++i) {
            auto entity = registry->create_entity();
            entities.push_back(entity);
            
            registry->add_component<Position>(entity, Position{
                static_cast<float>(i % 1000),
                static_cast<float>((i + 1) % 1000),
                static_cast<float>((i + 2) % 1000)
            });
        }
        
        // Test 1: Component read access
        {
            const size_t iterations = 1000000;
            volatile float sum = 0.0f; // Prevent optimization
            
            auto start = std::chrono::high_resolution_clock::now();
            
            for (size_t i = 0; i < iterations; ++i) {
                auto entity = entities[i % entities.size()];
                const auto& pos = registry->get_component<Position>(entity);
                sum += pos.x + pos.y + pos.z;
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
            
            double ops_per_second = (iterations * 1000000000.0) / duration.count();
            double latency_ns = static_cast<double>(duration.count()) / iterations;
            
            std::cout << "Component Read Access:\n";
            std::cout << "  " << iterations << " reads in " << duration.count() << " ns\n";
            std::cout << "  Throughput: " << ops_per_second << " reads/second\n";
            std::cout << "  Average latency: " << latency_ns << " ns/read\n";
            
            bool passed = latency_ns < 1000.0; // Target: < 1 microsecond
            std::cout << "  " << (passed ? "✅ PASSED" : "❌ FAILED") << " (Target: < 1000 ns)\n\n";
            
            results_.push_back({"Component Read", ops_per_second, latency_ns, 0.0, passed});
        }
        
        // Test 2: Component write access
        {
            const size_t iterations = 500000;
            
            auto start = std::chrono::high_resolution_clock::now();
            
            for (size_t i = 0; i < iterations; ++i) {
                auto entity = entities[i % entities.size()];
                auto& pos = registry->get_component<Position>(entity);
                pos.x += 0.01f;
                pos.y += 0.02f;
                pos.z += 0.03f;
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
            
            double ops_per_second = (iterations * 1000000000.0) / duration.count();
            double latency_ns = static_cast<double>(duration.count()) / iterations;
            
            std::cout << "Component Write Access:\n";
            std::cout << "  " << iterations << " writes in " << duration.count() << " ns\n";
            std::cout << "  Throughput: " << ops_per_second << " writes/second\n";
            std::cout << "  Average latency: " << latency_ns << " ns/write\n";
            
            bool passed = latency_ns < 1500.0; // Target: < 1.5 microseconds (writes slightly slower)
            std::cout << "  " << (passed ? "✅ PASSED" : "❌ FAILED") << " (Target: < 1500 ns)\n\n";
            
            results_.push_back({"Component Write", ops_per_second, latency_ns, 0.0, passed});
        }
    }
    
    void benchmark_bulk_operations() {
        std::cout << "📦 Bulk Operations Benchmark\n";
        std::cout << "==============================\n";
        
        auto registry = registry_factory::create_simulation_registry(1000000);
        auto batch = registry->batch();
        
        // Test 1: Bulk component addition
        {
            const size_t entity_count = 100000;
            auto entities = registry->create_entities(entity_count);
            
            Position default_pos{0.0f, 0.0f, 0.0f};
            
            auto start = std::chrono::high_resolution_clock::now();
            
            batch.batch_add_component<Position>(entities, default_pos);
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            double ops_per_second = (entity_count * 1000000.0) / duration.count();
            double latency_ns = duration.count() * 1000.0 / entity_count;
            
            std::cout << "Bulk Component Addition:\n";
            std::cout << "  Added components to " << entity_count << " entities in " << duration.count() << " μs\n";
            std::cout << "  Throughput: " << ops_per_second << " additions/second\n";
            std::cout << "  Average latency: " << latency_ns << " ns/addition\n";
            
            bool passed = ops_per_second > 500000.0; // Target: > 500K additions/second
            std::cout << "  " << (passed ? "✅ PASSED" : "❌ FAILED") << " (Target: > 500K additions/sec)\n\n";
            
            results_.push_back({"Bulk Component Add", ops_per_second, latency_ns, 0.0, passed});
        }
        
        // Test 2: Parallel query processing
        {
            const size_t entity_count = 200000;
            auto entities = registry->create_entities(entity_count);
            
            // Add components to all entities
            for (auto entity : entities) {
                registry->add_component<Position>(entity, Position{
                    static_cast<float>(rand() % 1000),
                    static_cast<float>(rand() % 1000),
                    static_cast<float>(rand() % 1000)
                });
                registry->add_component<Velocity>(entity, Velocity{
                    static_cast<float>((rand() % 21) - 10), // -10 to 10
                    static_cast<float>((rand() % 21) - 10),
                    static_cast<float>((rand() % 21) - 10)
                });
            }
            
            auto start = std::chrono::high_resolution_clock::now();
            
            // Simulate physics update
            batch.parallel_query<Position, Velocity>([](EntityHandle entity, Position& pos, Velocity& vel) {
                pos.x += vel.dx * 0.016f; // 60 FPS timestep
                pos.y += vel.dy * 0.016f;
                pos.z += vel.dz * 0.016f;
                
                // Simple bounds checking
                if (pos.x > 1000.0f || pos.x < -1000.0f) vel.dx *= -1.0f;
                if (pos.y > 1000.0f || pos.y < -1000.0f) vel.dy *= -1.0f;
                if (pos.z > 1000.0f || pos.z < -1000.0f) vel.dz *= -1.0f;
            }, 1024);
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            double ops_per_second = (entity_count * 1000000.0) / duration.count();
            double latency_ns = duration.count() * 1000.0 / entity_count;
            
            std::cout << "Parallel Query Processing:\n";
            std::cout << "  Processed " << entity_count << " entities in " << duration.count() << " μs\n";
            std::cout << "  Throughput: " << ops_per_second << " entities/second\n";
            std::cout << "  Average latency: " << latency_ns << " ns/entity\n";
            
            bool passed = ops_per_second > 10000000.0; // Target: > 10M entities/second
            std::cout << "  " << (passed ? "✅ PASSED" : "❌ FAILED") << " (Target: > 10M entities/sec)\n\n";
            
            results_.push_back({"Parallel Processing", ops_per_second, latency_ns, 0.0, passed});
        }
    }
    
    void benchmark_query_performance() {
        std::cout << "🔍 Query Performance Benchmark\n";
        std::cout << "===============================\n";
        
        auto registry = registry_factory::create_game_registry();
        
        // Create diverse entity set
        const size_t entity_count = 50000;
        for (size_t i = 0; i < entity_count; ++i) {
            auto entity = registry->create_entity();
            
            // All entities have Position
            registry->add_component<Position>(entity, Position{
                static_cast<float>(i % 1000),
                static_cast<float>((i + 1) % 1000),
                static_cast<float>((i + 2) % 1000)
            });
            
            // 70% have Velocity
            if (i % 10 < 7) {
                registry->add_component<Velocity>(entity, Velocity{
                    static_cast<float>((i % 21) - 10),
                    static_cast<float>(((i + 1) % 21) - 10),
                    static_cast<float>(((i + 2) % 21) - 10)
                });
            }
            
            // 40% have Mass
            if (i % 10 < 4) {
                registry->add_component<Mass>(entity, Mass{static_cast<float>((i % 100) + 1)});
            }
            
            // 20% have Tag
            if (i % 5 == 0) {
                registry->add_component<Tag>(entity, Tag{static_cast<uint32_t>(i % 1000)});
            }
        }
        
        std::cout << "Created " << entity_count << " entities with varying component combinations\n";
        
        // Test 1: Single component query
        {
            const size_t iterations = 1000;
            
            auto start = std::chrono::high_resolution_clock::now();
            
            for (size_t i = 0; i < iterations; ++i) {
                std::vector<EntityHandle> results;
                registry->query_entities<Position>(results);
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
            
            double ops_per_second = (iterations * 1000000000.0) / duration.count();
            double latency_ns = static_cast<double>(duration.count()) / iterations;
            
            std::cout << "Single Component Query (Position):\n";
            std::cout << "  " << iterations << " queries in " << duration.count() << " ns\n";
            std::cout << "  Throughput: " << ops_per_second << " queries/second\n";
            std::cout << "  Average latency: " << latency_ns << " ns/query\n";
            
            bool passed = latency_ns < 50000.0; // Target: < 50 microseconds
            std::cout << "  " << (passed ? "✅ PASSED" : "❌ FAILED") << " (Target: < 50000 ns)\n\n";
            
            results_.push_back({"Single Component Query", ops_per_second, latency_ns, 0.0, passed});
        }
        
        // Test 2: Multi-component query
        {
            const size_t iterations = 1000;
            
            auto start = std::chrono::high_resolution_clock::now();
            
            for (size_t i = 0; i < iterations; ++i) {
                std::vector<EntityHandle> results;
                registry->query_entities<Position, Velocity>(results);
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
            
            double ops_per_second = (iterations * 1000000000.0) / duration.count();
            double latency_ns = static_cast<double>(duration.count()) / iterations;
            
            std::cout << "Multi-Component Query (Position + Velocity):\n";
            std::cout << "  " << iterations << " queries in " << duration.count() << " ns\n";
            std::cout << "  Throughput: " << ops_per_second << " queries/second\n";
            std::cout << "  Average latency: " << latency_ns << " ns/query\n";
            
            bool passed = latency_ns < 100000.0; // Target: < 100 microseconds
            std::cout << "  " << (passed ? "✅ PASSED" : "❌ FAILED") << " (Target: < 100000 ns)\n\n";
            
            results_.push_back({"Multi-Component Query", ops_per_second, latency_ns, 0.0, passed});
        }
    }
    
    void benchmark_archetype_transitions() {
        std::cout << "🔄 Archetype Transition Benchmark\n";
        std::cout << "==================================\n";
        
        auto registry = registry_factory::create_game_registry();
        
        // Test 1: Component addition transitions
        {
            const size_t entity_count = 10000;
            auto entities = registry->create_entities(entity_count);
            
            // Add Position to all (empty -> Position archetype)
            auto start = std::chrono::high_resolution_clock::now();
            
            for (auto entity : entities) {
                registry->add_component<Position>(entity, Position{1.0f, 2.0f, 3.0f});
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            double ops_per_second = (entity_count * 1000000.0) / duration.count();
            double latency_ns = duration.count() * 1000.0 / entity_count;
            
            std::cout << "Component Addition Transitions:\n";
            std::cout << "  " << entity_count << " transitions in " << duration.count() << " μs\n";
            std::cout << "  Throughput: " << ops_per_second << " transitions/second\n";
            std::cout << "  Average latency: " << latency_ns << " ns/transition\n";
            
            bool passed = latency_ns < 5000.0; // Target: < 5 microseconds per transition
            std::cout << "  " << (passed ? "✅ PASSED" : "❌ FAILED") << " (Target: < 5000 ns)\n\n";
            
            results_.push_back({"Addition Transitions", ops_per_second, latency_ns, 0.0, passed});
        }
        
        // Test 2: Complex transition patterns
        {
            const size_t entity_count = 5000;
            auto entities = registry->create_entities(entity_count);
            
            auto start = std::chrono::high_resolution_clock::now();
            
            for (auto entity : entities) {
                // Complex transition: empty -> Position -> Position+Velocity -> Position+Velocity+Mass -> Position+Mass
                registry->add_component<Position>(entity, Position{});
                registry->add_component<Velocity>(entity, Velocity{});
                registry->add_component<Mass>(entity, Mass{1.0f});
                registry->remove_component<Velocity>(entity);
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            double ops_per_second = (entity_count * 4 * 1000000.0) / duration.count(); // 4 operations per entity
            double latency_ns = duration.count() * 1000.0 / (entity_count * 4);
            
            std::cout << "Complex Transition Patterns:\n";
            std::cout << "  " << (entity_count * 4) << " operations in " << duration.count() << " μs\n";
            std::cout << "  Throughput: " << ops_per_second << " operations/second\n";
            std::cout << "  Average latency: " << latency_ns << " ns/operation\n";
            
            bool passed = latency_ns < 10000.0; // Target: < 10 microseconds per operation
            std::cout << "  " << (passed ? "✅ PASSED" : "❌ FAILED") << " (Target: < 10000 ns)\n\n";
            
            results_.push_back({"Complex Transitions", ops_per_second, latency_ns, 0.0, passed});
        }
    }
    
    void benchmark_memory_efficiency() {
        std::cout << "💾 Memory Efficiency Benchmark\n";
        std::cout << "===============================\n";
        
        auto registry = registry_factory::create_simulation_registry(1000000);
        
        // Create large entity set with components
        const size_t entity_count = 100000;
        std::vector<EntityHandle> entities;
        entities.reserve(entity_count);
        
        for (size_t i = 0; i < entity_count; ++i) {
            auto entity = registry->create_entity();
            entities.push_back(entity);
            
            registry->add_component<Position>(entity, Position{
                static_cast<float>(i % 1000),
                static_cast<float>((i + 1) % 1000),
                static_cast<float>((i + 2) % 1000)
            });
            
            if (i % 2 == 0) {
                registry->add_component<Velocity>(entity, Velocity{
                    static_cast<float>((i % 21) - 10),
                    static_cast<float>(((i + 1) % 21) - 10),
                    static_cast<float>(((i + 2) % 21) - 10)
                });
            }
            
            if (i % 3 == 0) {
                registry->add_component<Mass>(entity, Mass{static_cast<float>((i % 100) + 1)});
            }
        }
        
        auto stats = registry->get_stats();
        
        std::cout << "Memory Usage Analysis for " << entity_count << " entities:\n";
        std::cout << "  Total Memory: " << (stats.total_memory_usage / 1024.0 / 1024.0) << " MB\n";
        std::cout << "  Entity Memory: " << (stats.entity_memory_usage / 1024.0 / 1024.0) << " MB\n";
        std::cout << "  Component Memory: " << (stats.component_memory_usage / 1024.0 / 1024.0) << " MB\n";
        std::cout << "  Archetype Memory: " << (stats.archetype_memory_usage / 1024.0 / 1024.0) << " MB\n";
        
        double bytes_per_entity = static_cast<double>(stats.total_memory_usage) / entity_count;
        double theoretical_minimum = (sizeof(Position) + sizeof(Velocity) * 0.5 + sizeof(Mass) * 0.33);
        double memory_overhead = (bytes_per_entity - theoretical_minimum) / theoretical_minimum * 100.0;
        
        std::cout << "  Bytes per Entity: " << bytes_per_entity << "\n";
        std::cout << "  Theoretical Minimum: " << theoretical_minimum << "\n";
        std::cout << "  Memory Overhead: " << memory_overhead << "%\n";
        
        bool passed = memory_overhead < 50.0; // Target: < 50% overhead
        std::cout << "  " << (passed ? "✅ PASSED" : "❌ FAILED") << " (Target: < 50% overhead)\n\n";
        
        results_.push_back({"Memory Efficiency", 0.0, 0.0, stats.total_memory_usage / 1024.0 / 1024.0, passed});
    }
    
    void benchmark_thread_scalability() {
        std::cout << "🧵 Thread Scalability Benchmark\n";
        std::cout << "================================\n";
        
        const std::vector<size_t> thread_counts = {1, 2, 4, 8};
        
        for (auto num_threads : thread_counts) {
            auto registry = registry_factory::create_simulation_registry(100000);
            
            const size_t entities_per_thread = 10000;
            const size_t total_entities = num_threads * entities_per_thread;
            
            std::vector<std::thread> threads;
            std::vector<std::vector<EntityHandle>> thread_results(num_threads);
            
            auto start = std::chrono::high_resolution_clock::now();
            
            // Create entities from multiple threads
            for (size_t t = 0; t < num_threads; ++t) {
                threads.emplace_back([&, t]() {
                    thread_results[t].reserve(entities_per_thread);
                    
                    for (size_t i = 0; i < entities_per_thread; ++i) {
                        auto entity = registry->create_entity();
                        thread_results[t].push_back(entity);
                        
                        registry->add_component<Position>(entity, Position{
                            static_cast<float>(t * 1000 + i),
                            static_cast<float>(t * 1000 + i + 1),
                            static_cast<float>(t * 1000 + i + 2)
                        });
                    }
                });
            }
            
            for (auto& thread : threads) {
                thread.join();
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            double ops_per_second = (total_entities * 1000000.0) / duration.count();
            double efficiency = (num_threads == 1) ? 1.0 : 
                               (ops_per_second / num_threads) / (results_[0].operations_per_second);
            
            std::cout << "Thread Count: " << num_threads << "\n";
            std::cout << "  Created " << total_entities << " entities in " << duration.count() << " μs\n";
            std::cout << "  Throughput: " << ops_per_second << " entities/second\n";
            std::cout << "  Parallel Efficiency: " << (efficiency * 100.0) << "%\n";
            
            bool passed = efficiency > 0.7; // Target: > 70% efficiency
            std::cout << "  " << (passed ? "✅ PASSED" : "❌ FAILED") << " (Target: > 70% efficiency)\n\n";
            
            if (num_threads == 1) {
                // Store single-thread baseline for efficiency calculations
                results_.insert(results_.begin(), {"Single Thread Baseline", ops_per_second, 0.0, 0.0, true});
            }
        }
    }
    
    void benchmark_cache_performance() {
        std::cout << "🏎️ Cache Performance Benchmark\n";
        std::cout << "===============================\n";
        
        auto registry = registry_factory::create_game_registry();
        
        // Create entities with predictable memory layout
        const size_t entity_count = 50000;
        std::vector<EntityHandle> entities;
        entities.reserve(entity_count);
        
        for (size_t i = 0; i < entity_count; ++i) {
            auto entity = registry->create_entity();
            entities.push_back(entity);
            registry->add_component<Position>(entity, Position{
                static_cast<float>(i), static_cast<float>(i + 1), static_cast<float>(i + 2)
            });
        }
        
        // Test 1: Sequential access (cache-friendly)
        {
            const size_t iterations = 100;
            volatile float sum = 0.0f;
            
            auto start = std::chrono::high_resolution_clock::now();
            
            for (size_t iter = 0; iter < iterations; ++iter) {
                std::vector<EntityHandle> query_results;
                registry->query_entities<Position>(query_results);
                
                for (auto entity : query_results) {
                    const auto& pos = registry->get_component<Position>(entity);
                    sum += pos.x + pos.y + pos.z;
                }
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            double ops_per_second = (iterations * entity_count * 1000000.0) / duration.count();
            
            std::cout << "Sequential Access (Cache-Friendly):\n";
            std::cout << "  Processed " << (iterations * entity_count) << " components in " << duration.count() << " μs\n";
            std::cout << "  Throughput: " << ops_per_second << " accesses/second\n";
            
            // Store for comparison
            double sequential_throughput = ops_per_second;
            
            // Test 2: Random access (cache-hostile)
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<size_t> dist(0, entities.size() - 1);
            
            sum = 0.0f;
            start = std::chrono::high_resolution_clock::now();
            
            for (size_t iter = 0; iter < iterations; ++iter) {
                for (size_t i = 0; i < entity_count; ++i) {
                    auto entity = entities[dist(gen)];
                    const auto& pos = registry->get_component<Position>(entity);
                    sum += pos.x + pos.y + pos.z;
                }
            }
            
            end = std::chrono::high_resolution_clock::now();
            duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            double random_ops_per_second = (iterations * entity_count * 1000000.0) / duration.count();
            
            std::cout << "Random Access (Cache-Hostile):\n";
            std::cout << "  Processed " << (iterations * entity_count) << " components in " << duration.count() << " μs\n";
            std::cout << "  Throughput: " << random_ops_per_second << " accesses/second\n";
            
            double cache_efficiency = sequential_throughput / random_ops_per_second;
            std::cout << "Cache Efficiency Ratio: " << cache_efficiency << "x\n";
            
            bool passed = cache_efficiency > 2.0; // Target: Sequential should be at least 2x faster
            std::cout << "  " << (passed ? "✅ PASSED" : "❌ FAILED") << " (Target: > 2x improvement)\n\n";
            
            results_.push_back({"Cache Performance", sequential_throughput, 0.0, 0.0, passed});
        }
    }
    
    void print_performance_summary() {
        std::cout << "📊 Performance Summary\n";
        std::cout << "======================\n";
        
        size_t passed_count = 0;
        for (const auto& result : results_) {
            if (result.passed_target) passed_count++;
        }
        
        std::cout << "Overall Results: " << passed_count << "/" << results_.size() 
                  << " benchmarks passed\n\n";
        
        std::cout << std::left << std::setw(30) << "Benchmark" 
                  << std::setw(15) << "Throughput" 
                  << std::setw(15) << "Latency (ns)" 
                  << std::setw(12) << "Status" << "\n";
        std::cout << std::string(72, '-') << "\n";
        
        for (const auto& result : results_) {
            std::cout << std::setw(30) << result.name;
            
            if (result.operations_per_second > 0) {
                std::cout << std::setw(15) << (result.operations_per_second / 1000000.0) << "M ops/s";
            } else {
                std::cout << std::setw(15) << "N/A";
            }
            
            if (result.average_latency_ns > 0) {
                std::cout << std::setw(15) << result.average_latency_ns;
            } else {
                std::cout << std::setw(15) << "N/A";
            }
            
            std::cout << (result.passed_target ? "✅ PASS" : "❌ FAIL") << "\n";
        }
        
        std::cout << "\n";
        
        if (passed_count == results_.size()) {
            std::cout << "🎉 All performance targets achieved!\n";
            std::cout << "This ECS Registry meets world-class performance standards.\n";
        } else {
            std::cout << "⚠️  " << (results_.size() - passed_count) 
                      << " benchmark(s) did not meet targets.\n";
            std::cout << "Consider optimization for production use.\n";
        }
    }
};

int main() {
    try {
        RegistryBenchmark benchmark;
        benchmark.run_all_benchmarks();
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Benchmark Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Unknown benchmark error occurred" << std::endl;
        return 1;
    }
}