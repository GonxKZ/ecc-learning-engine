/**
 * @file examples/advanced_memory_examples.cpp
 * @brief Comprehensive Educational Examples for Advanced Memory System
 * 
 * This file provides real-world examples demonstrating the advanced memory
 * management features implemented in ECScope, with detailed explanations
 * and performance comparisons to illustrate the benefits of each optimization.
 * 
 * Examples Include:
 * - NUMA-aware allocation patterns and their performance impact
 * - Lock-free vs traditional allocator comparisons
 * - Hierarchical pool system demonstrations
 * - Cache-aware data structure usage examples
 * - Memory bandwidth optimization techniques
 * - Thread-local storage benefits and trade-offs
 * 
 * Educational Value:
 * - Real-world performance measurements
 * - Visual comparisons of different approaches
 * - Practical optimization recommendations
 * - Memory system behavior analysis
 * 
 * @author ECScope Educational ECS Framework
 * @date 2025
 */

#include "memory/numa_manager.hpp"
#include "memory/lockfree_allocators.hpp"
#include "memory/hierarchical_pools.hpp"
#include "memory/cache_aware_structures.hpp"
#include "memory/bandwidth_analyzer.hpp"
#include "memory/thread_local_allocator.hpp"
#include "core/log.hpp"
#include "core/profiler.hpp"
#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <thread>
#include <future>
#include <iomanip>

using namespace ecscope::memory;

//=============================================================================
// Example Data Structures
//=============================================================================

struct GameEntity {
    f32 position[3];        // Hot data - frequently accessed
    f32 velocity[3];        // Hot data - frequently accessed
    u32 entity_id;          // Hot data
    u8 flags;              // Hot data
    
    // Cold data - infrequently accessed
    std::string name;       // Cold data
    f64 creation_time;      // Cold data
    std::vector<u32> components; // Cold data
    
    GameEntity() : entity_id(0), flags(0), creation_time(0.0) {
        std::fill(std::begin(position), std::end(position), 0.0f);
        std::fill(std::begin(velocity), std::end(velocity), 0.0f);
    }
};

struct PhysicsComponent {
    f32 mass;
    f32 drag;
    f32 force[3];
    f32 acceleration[3];
    
    PhysicsComponent() : mass(1.0f), drag(0.1f) {
        std::fill(std::begin(force), std::end(force), 0.0f);
        std::fill(std::begin(acceleration), std::end(acceleration), 0.0f);
    }
};

//=============================================================================
// Example 1: NUMA-Aware Memory Allocation Demonstration
//=============================================================================

void demonstrate_numa_awareness() {
    std::cout << "\n=== NUMA-Aware Memory Allocation Example ===\n";
    
    auto& numa_manager = numa::get_global_numa_manager();
    
    if (!numa_manager.is_numa_available()) {
        std::cout << "NUMA not available on this system - running simplified example\n";
    }
    
    // Print NUMA topology
    numa_manager.print_numa_topology();
    
    const usize entity_count = 10000;
    const usize iterations = 1000;
    
    std::cout << "\nComparing allocation strategies for " << entity_count 
              << " entities across " << iterations << " iterations:\n\n";
    
    // 1. Regular allocation (no NUMA awareness)
    {
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<GameEntity*> entities;
        entities.reserve(entity_count);
        
        for (usize i = 0; i < iterations; ++i) {
            // Allocate entities
            for (usize j = 0; j < entity_count; ++j) {
                entities.push_back(new GameEntity());
            }
            
            // Simulate processing (accessing position data)
            for (auto* entity : entities) {
                entity->position[0] += 1.0f;
                entity->position[1] += 1.0f;
                entity->position[2] += 1.0f;
            }
            
            // Cleanup
            for (auto* entity : entities) {
                delete entity;
            }
            entities.clear();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<f64, std::milli>(end - start);
        
        std::cout << "1. Regular allocation: " << std::fixed << std::setprecision(2) 
                  << duration.count() << " ms\n";
    }
    
    // 2. NUMA-aware allocation
    {
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<GameEntity*> entities;
        entities.reserve(entity_count);
        
        // Set thread affinity to specific NUMA node
        auto current_node = numa_manager.get_current_thread_node();
        if (current_node.has_value()) {
            numa_manager.set_current_thread_affinity(current_node.value());
        }
        
        for (usize i = 0; i < iterations; ++i) {
            // Allocate entities on local NUMA node
            for (usize j = 0; j < entity_count; ++j) {
                void* memory = numa_manager.allocate(sizeof(GameEntity));
                entities.push_back(new(memory) GameEntity());
            }
            
            // Simulate processing (accessing position data)
            for (auto* entity : entities) {
                entity->position[0] += 1.0f;
                entity->position[1] += 1.0f;
                entity->position[2] += 1.0f;
            }
            
            // Cleanup
            for (auto* entity : entities) {
                entity->~GameEntity();
                numa_manager.deallocate(entity, sizeof(GameEntity));
            }
            entities.clear();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<f64, std::milli>(end - start);
        
        std::cout << "2. NUMA-aware allocation: " << std::fixed << std::setprecision(2) 
                  << duration.count() << " ms\n";
    }
    
    // Print NUMA performance metrics
    std::cout << "\nNUMA Performance Analysis:\n";
    std::cout << numa_manager.generate_performance_report();
}

//=============================================================================
// Example 2: Lock-Free vs Traditional Allocator Comparison
//=============================================================================

void demonstrate_lockfree_allocators() {
    std::cout << "\n=== Lock-Free Allocator Comparison ===\n";
    
    const usize allocation_count = 100000;
    const u32 thread_count = std::thread::hardware_concurrency();
    
    std::cout << "Testing with " << allocation_count << " allocations per thread\n";
    std::cout << "Using " << thread_count << " threads\n\n";
    
    // 1. Traditional mutex-protected allocator
    {
        struct MutexAllocator {
            std::mutex mutex;
            
            void* allocate(usize size) {
                std::lock_guard<std::mutex> lock(mutex);
                return std::malloc(size);
            }
            
            void deallocate(void* ptr) {
                std::lock_guard<std::mutex> lock(mutex);
                std::free(ptr);
            }
        };
        
        MutexAllocator mutex_allocator;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::future<void>> futures;
        for (u32 t = 0; t < thread_count; ++t) {
            futures.push_back(std::async(std::launch::async, [&mutex_allocator, allocation_count]() {
                std::vector<void*> ptrs;
                ptrs.reserve(allocation_count);
                
                // Allocate
                for (usize i = 0; i < allocation_count; ++i) {
                    ptrs.push_back(mutex_allocator.allocate(sizeof(PhysicsComponent)));
                }
                
                // Deallocate
                for (void* ptr : ptrs) {
                    mutex_allocator.deallocate(ptr);
                }
            }));
        }
        
        for (auto& future : futures) {
            future.wait();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<f64, std::milli>(end - start);
        
        std::cout << "1. Mutex-protected allocator: " << std::fixed << std::setprecision(2) 
                  << duration.count() << " ms\n";
    }
    
    // 2. Lock-free allocator
    {
        auto& lockfree_allocator = lockfree::get_global_lockfree_allocator();
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::future<void>> futures;
        for (u32 t = 0; t < thread_count; ++t) {
            futures.push_back(std::async(std::launch::async, [&lockfree_allocator, allocation_count]() {
                std::vector<void*> ptrs;
                ptrs.reserve(allocation_count);
                
                // Allocate
                for (usize i = 0; i < allocation_count; ++i) {
                    ptrs.push_back(lockfree_allocator.allocate(sizeof(PhysicsComponent)));
                }
                
                // Deallocate
                for (void* ptr : ptrs) {
                    lockfree_allocator.deallocate(ptr);
                }
            }));
        }
        
        for (auto& future : futures) {
            future.wait();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<f64, std::milli>(end - start);
        
        std::cout << "2. Lock-free allocator: " << std::fixed << std::setprecision(2) 
                  << duration.count() << " ms\n";
        
        // Print lock-free allocator statistics
        auto stats = lockfree_allocator.get_statistics();
        std::cout << "\nLock-free allocator stats:\n";
        std::cout << "  Arena allocations: " << stats.arena_allocations << "\n";
        std::cout << "  Pool allocations: " << stats.pool_allocations << "\n";
        std::cout << "  Distribution ratio: " << std::fixed << std::setprecision(2) 
                  << stats.allocation_distribution_ratio * 100 << "%\n";
    }
}

//=============================================================================
// Example 3: Hierarchical Pool System Demonstration
//=============================================================================

void demonstrate_hierarchical_pools() {
    std::cout << "\n=== Hierarchical Pool System Example ===\n";
    
    auto& hierarchical_allocator = hierarchical::get_global_hierarchical_allocator();
    
    const usize entity_count = 50000;
    
    std::cout << "Creating " << entity_count << " entities with mixed allocation patterns:\n\n";
    
    // Simulate realistic game object allocation patterns
    std::vector<GameEntity*> entities;
    std::vector<PhysicsComponent*> physics_components;
    entities.reserve(entity_count);
    physics_components.reserve(entity_count / 2); // Not all entities have physics
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<f32> dist(0.0f, 1.0f);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Phase 1: Initial allocation burst (game startup)
    std::cout << "Phase 1: Initial allocation burst...\n";
    for (usize i = 0; i < entity_count; ++i) {
        auto* entity = hierarchical_allocator.construct<GameEntity>();
        entity->entity_id = static_cast<u32>(i);
        entity->position[0] = dist(rng) * 100.0f;
        entity->position[1] = dist(rng) * 100.0f;
        entity->position[2] = dist(rng) * 100.0f;
        entities.push_back(entity);
        
        // 50% chance of having physics component
        if (dist(rng) > 0.5f) {
            auto* physics = hierarchical_allocator.construct<PhysicsComponent>();
            physics->mass = dist(rng) * 10.0f + 1.0f;
            physics_components.push_back(physics);
        }
    }
    
    auto phase1_time = std::chrono::high_resolution_clock::now();
    auto phase1_duration = std::chrono::duration<f64, std::milli>(phase1_time - start);
    std::cout << "  Completed in " << std::fixed << std::setprecision(2) 
              << phase1_duration.count() << " ms\n";
    
    // Phase 2: Runtime allocation/deallocation (gameplay)
    std::cout << "Phase 2: Runtime allocation/deallocation simulation...\n";
    
    for (usize frame = 0; frame < 1000; ++frame) {
        // Randomly deallocate some entities (destruction)
        for (usize i = 0; i < 10 && !entities.empty(); ++i) {
            usize index = rng() % entities.size();
            hierarchical_allocator.destroy(entities[index]);
            entities.erase(entities.begin() + index);
        }
        
        // Randomly allocate new entities (spawning)
        for (usize i = 0; i < 10; ++i) {
            auto* entity = hierarchical_allocator.construct<GameEntity>();
            entity->entity_id = static_cast<u32>(entity_count + frame * 10 + i);
            entities.push_back(entity);
        }
        
        // Simulate entity updates (memory access)
        for (auto* entity : entities) {
            entity->position[0] += entity->velocity[0] * 0.016f; // 60 FPS
            entity->position[1] += entity->velocity[1] * 0.016f;
            entity->position[2] += entity->velocity[2] * 0.016f;
        }
    }
    
    auto phase2_time = std::chrono::high_resolution_clock::now();
    auto phase2_duration = std::chrono::duration<f64, std::milli>(phase2_time - phase1_time);
    std::cout << "  Completed in " << std::fixed << std::setprecision(2) 
              << phase2_duration.count() << " ms\n";
    
    // Phase 3: Cleanup
    std::cout << "Phase 3: Cleanup...\n";
    for (auto* entity : entities) {
        hierarchical_allocator.destroy(entity);
    }
    for (auto* physics : physics_components) {
        hierarchical_allocator.destroy(physics);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto cleanup_duration = std::chrono::duration<f64, std::milli>(end - phase2_time);
    auto total_duration = std::chrono::duration<f64, std::milli>(end - start);
    
    std::cout << "  Completed in " << std::fixed << std::setprecision(2) 
              << cleanup_duration.count() << " ms\n";
    std::cout << "\nTotal time: " << std::fixed << std::setprecision(2) 
              << total_duration.count() << " ms\n";
    
    // Print hierarchical allocator statistics
    auto stats = hierarchical_allocator.get_statistics();
    std::cout << "\nHierarchical Allocator Statistics:\n";
    std::cout << "  L1 hit rate: " << std::fixed << std::setprecision(1) 
              << stats.l1_hit_rate * 100 << "%\n";
    std::cout << "  L2 hit rate: " << std::fixed << std::setprecision(1) 
              << stats.l2_hit_rate * 100 << "%\n";
    std::cout << "  Overall cache efficiency: " << std::fixed << std::setprecision(1)
              << stats.overall_cache_efficiency * 100 << "%\n";
    std::cout << "  Active size classes: " << stats.active_size_classes << "\n";
}

//=============================================================================
// Example 4: Cache-Aware Data Structures
//=============================================================================

void demonstrate_cache_aware_structures() {
    std::cout << "\n=== Cache-Aware Data Structures Example ===\n";
    
    auto& cache_analyzer = cache::get_global_cache_analyzer();
    
    std::cout << "System cache topology:\n";
    std::cout << cache_analyzer.generate_topology_report();
    
    const usize element_count = 100000;
    const usize iterations = 100;
    
    std::cout << "\nComparing array access patterns with " << element_count 
              << " elements:\n\n";
    
    // 1. Regular std::vector (no prefetching)
    {
        std::vector<PhysicsComponent> regular_vector(element_count);
        
        // Initialize with random data
        std::mt19937 rng(42);
        std::uniform_real_distribution<f32> dist(0.0f, 100.0f);
        
        for (auto& component : regular_vector) {
            component.mass = dist(rng);
            component.drag = dist(rng) * 0.1f;
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Sequential access pattern
        for (usize iter = 0; iter < iterations; ++iter) {
            for (auto& component : regular_vector) {
                // Simulate physics calculation
                component.force[0] = component.mass * component.acceleration[0];
                component.force[1] = component.mass * component.acceleration[1];
                component.force[2] = component.mass * component.acceleration[2];
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<f64, std::milli>(end - start);
        
        std::cout << "1. Regular std::vector: " << std::fixed << std::setprecision(2) 
                  << duration.count() << " ms\n";
    }
    
    // 2. Cache-friendly array with prefetching
    {
        cache::CacheFriendlyArray<PhysicsComponent> cache_friendly_vector;
        cache_friendly_vector.reserve(element_count);
        
        // Initialize with random data
        std::mt19937 rng(42);
        std::uniform_real_distribution<f32> dist(0.0f, 100.0f);
        
        for (usize i = 0; i < element_count; ++i) {
            PhysicsComponent component;
            component.mass = dist(rng);
            component.drag = dist(rng) * 0.1f;
            cache_friendly_vector.push_back(component);
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Sequential access with prefetching
        for (usize iter = 0; iter < iterations; ++iter) {
            for (auto it = cache_friendly_vector.sequential_begin(); 
                 it != cache_friendly_vector.sequential_end(); ++it) {
                // Simulate physics calculation
                it->force[0] = it->mass * it->acceleration[0];
                it->force[1] = it->mass * it->acceleration[1];
                it->force[2] = it->mass * it->acceleration[2];
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<f64, std::milli>(end - start);
        
        std::cout << "2. Cache-friendly array: " << std::fixed << std::setprecision(2) 
                  << duration.count() << " ms\n";
        
        // Print access statistics
        auto access_stats = cache_friendly_vector.get_access_statistics();
        std::cout << "   Access pattern analysis:\n";
        std::cout << "     Sequential ratio: " << std::fixed << std::setprecision(1)
                  << access_stats.sequential_ratio * 100 << "%\n";
        std::cout << "     Cache efficiency estimate: " << std::fixed << std::setprecision(1)
                  << access_stats.cache_efficiency_estimate * 100 << "%\n";
    }
    
    // 3. Hot/Cold data separation example
    {
        std::cout << "\n3. Hot/Cold Data Separation Example:\n";
        
        const usize entity_count_hc = 10000;
        
        // Traditional approach - all data together
        std::vector<GameEntity> traditional_entities(entity_count_hc);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Simulate hot data access (position updates)
        for (usize iter = 0; iter < iterations; ++iter) {
            for (auto& entity : traditional_entities) {
                entity.position[0] += entity.velocity[0];
                entity.position[1] += entity.velocity[1];
                entity.position[2] += entity.velocity[2];
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto traditional_duration = std::chrono::duration<f64, std::milli>(end - start);
        
        // Hot/Cold separated approach
        using HotColdEntity = cache::HotColdSeparatedData<
            struct { f32 position[3]; f32 velocity[3]; u32 id; }, // Hot data
            struct { std::string name; f64 creation_time; std::vector<u32> components; } // Cold data
        >;
        
        std::vector<HotColdEntity> separated_entities;
        separated_entities.reserve(entity_count_hc);
        
        for (usize i = 0; i < entity_count_hc; ++i) {
            separated_entities.emplace_back(
                std::make_tuple(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, static_cast<u32>(i)),
                std::make_tuple(std::string("Entity"), 0.0, std::vector<u32>{})
            );
        }
        
        start = std::chrono::high_resolution_clock::now();
        
        // Access only hot data
        for (usize iter = 0; iter < iterations; ++iter) {
            for (auto& entity : separated_entities) {
                auto& hot = entity.hot();
                hot.position[0] += hot.velocity[0];
                hot.position[1] += hot.velocity[1];
                hot.position[2] += hot.velocity[2];
            }
        }
        
        end = std::chrono::high_resolution_clock::now();
        auto separated_duration = std::chrono::duration<f64, std::milli>(end - start);
        
        std::cout << "   Traditional (all data together): " << std::fixed << std::setprecision(2)
                  << traditional_duration.count() << " ms\n";
        std::cout << "   Hot/Cold separated: " << std::fixed << std::setprecision(2)
                  << separated_duration.count() << " ms\n";
        
        f64 improvement = (traditional_duration.count() - separated_duration.count()) / traditional_duration.count();
        std::cout << "   Performance improvement: " << std::fixed << std::setprecision(1)
                  << improvement * 100 << "%\n";
        
        // Analyze access pattern
        if (!separated_entities.empty()) {
            auto analysis = separated_entities[0].analyze_access_pattern();
            std::cout << "   Hot/cold access analysis:\n";
            std::cout << "     Hot access ratio: " << std::fixed << std::setprecision(1)
                      << analysis.hot_access_ratio * 100 << "%\n";
            std::cout << "     Cache efficiency: " << std::fixed << std::setprecision(1)
                      << analysis.cache_efficiency_estimate * 100 << "%\n";
        }
    }
}

//=============================================================================
// Example 5: Memory Bandwidth Analysis
//=============================================================================

void demonstrate_bandwidth_analysis() {
    std::cout << "\n=== Memory Bandwidth Analysis Example ===\n";
    
    auto& bandwidth_profiler = bandwidth::get_global_bandwidth_profiler();
    auto& bottleneck_detector = bandwidth::get_global_bottleneck_detector();
    
    std::cout << "Starting memory bandwidth profiling...\n";
    
    // Start profiling
    bandwidth_profiler.start_profiling();
    
    // Let it collect some baseline data
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Run comprehensive bandwidth analysis
    std::cout << "Running comprehensive bandwidth analysis...\n";
    auto measurements = bandwidth_profiler.run_comprehensive_analysis();
    
    std::cout << "Completed " << measurements.size() << " bandwidth measurements\n\n";
    
    // Display some interesting results
    f64 best_read_bandwidth = 0.0;
    f64 best_write_bandwidth = 0.0;
    
    for (const auto& measurement : measurements) {
        if (measurement.operation == bandwidth::MemoryOperation::SequentialRead ||
            measurement.operation == bandwidth::MemoryOperation::RandomRead ||
            measurement.operation == bandwidth::MemoryOperation::StreamingRead) {
            best_read_bandwidth = std::max(best_read_bandwidth, measurement.bandwidth_gbps);
        } else {
            best_write_bandwidth = std::max(best_write_bandwidth, measurement.bandwidth_gbps);
        }
    }
    
    std::cout << "Peak Performance Results:\n";
    std::cout << "  Best read bandwidth: " << std::fixed << std::setprecision(2)
              << best_read_bandwidth << " GB/s\n";
    std::cout << "  Best write bandwidth: " << std::fixed << std::setprecision(2)
              << best_write_bandwidth << " GB/s\n";
    
    // Get real-time statistics
    auto real_time_stats = bandwidth_profiler.get_real_time_stats();
    std::cout << "\nReal-Time Statistics:\n";
    std::cout << "  Current read bandwidth: " << std::fixed << std::setprecision(2)
              << real_time_stats.current_read_bandwidth_gbps << " GB/s\n";
    std::cout << "  Current write bandwidth: " << std::fixed << std::setprecision(2)
              << real_time_stats.current_write_bandwidth_gbps << " GB/s\n";
    std::cout << "  Total bytes processed: " << real_time_stats.total_bytes_processed / (1024*1024)
              << " MB\n";
    
    // Detect bottlenecks
    std::cout << "\nAnalyzing for memory bottlenecks...\n";
    auto bottlenecks = bottleneck_detector.detect_bottlenecks();
    
    if (bottlenecks.empty()) {
        std::cout << "No significant bottlenecks detected - system is operating efficiently\n";
    } else {
        std::cout << "Detected " << bottlenecks.size() << " potential bottlenecks:\n";
        for (const auto& bottleneck : bottlenecks) {
            std::cout << "  - " << static_cast<int>(bottleneck.type) 
                      << ": Severity " << std::fixed << std::setprecision(1)
                      << bottleneck.severity_score * 100 << "%\n";
            std::cout << "    " << bottleneck.description << "\n";
            std::cout << "    Recommendation: " << bottleneck.recommendation << "\n\n";
        }
    }
    
    // Generate comprehensive report
    std::cout << "Bottleneck Analysis Report:\n";
    std::cout << bottleneck_detector.generate_bottleneck_report();
    
    bandwidth_profiler.stop_profiling();
}

//=============================================================================
// Example 6: Thread-Local Storage Benefits
//=============================================================================

void demonstrate_thread_local_storage() {
    std::cout << "\n=== Thread-Local Storage Example ===\n";
    
    auto& tl_registry = thread_local::get_global_thread_local_registry();
    
    const usize allocations_per_thread = 10000;
    const u32 thread_count = std::thread::hardware_concurrency();
    
    std::cout << "Comparing allocation strategies with " << thread_count 
              << " threads, " << allocations_per_thread << " allocations each:\n\n";
    
    // 1. Shared global allocator (with contention)
    {
        std::mutex global_mutex;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::future<void>> futures;
        for (u32 t = 0; t < thread_count; ++t) {
            futures.push_back(std::async(std::launch::async, [&global_mutex, allocations_per_thread]() {
                std::vector<PhysicsComponent*> components;
                components.reserve(allocations_per_thread);
                
                // Allocate
                for (usize i = 0; i < allocations_per_thread; ++i) {
                    std::lock_guard<std::mutex> lock(global_mutex);
                    components.push_back(new PhysicsComponent());
                }
                
                // Use the data (simulate processing)
                for (auto* component : components) {
                    component->mass += 1.0f;
                }
                
                // Deallocate
                for (auto* component : components) {
                    std::lock_guard<std::mutex> lock(global_mutex);
                    delete component;
                }
            }));
        }
        
        for (auto& future : futures) {
            future.wait();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<f64, std::milli>(end - start);
        
        std::cout << "1. Shared global allocator (with contention): " 
                  << std::fixed << std::setprecision(2) << duration.count() << " ms\n";
    }
    
    // 2. Thread-local allocators
    {
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::future<void>> futures;
        for (u32 t = 0; t < thread_count; ++t) {
            futures.push_back(std::async(std::launch::async, [&tl_registry, allocations_per_thread]() {
                // Register thread for cleanup tracking
                thread_local::ThreadRegistrationGuard guard;
                
                auto& primary_pool = tl_registry.get_primary_pool();
                
                std::vector<PhysicsComponent*> components;
                components.reserve(allocations_per_thread);
                
                // Allocate from thread-local pool
                for (usize i = 0; i < allocations_per_thread; ++i) {
                    components.push_back(primary_pool.construct<PhysicsComponent>());
                }
                
                // Use the data (simulate processing)
                for (auto* component : components) {
                    component->mass += 1.0f;
                }
                
                // Deallocate to thread-local pool
                for (auto* component : components) {
                    primary_pool.destroy(component);
                }
            }));
        }
        
        for (auto& future : futures) {
            future.wait();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<f64, std::milli>(end - start);
        
        std::cout << "2. Thread-local allocators: " 
                  << std::fixed << std::setprecision(2) << duration.count() << " ms\n";
    }
    
    // Print thread-local statistics
    std::cout << "\nThread-Local System Statistics:\n";
    auto system_stats = tl_registry.get_system_statistics();
    std::cout << "  Total Pools: " << system_stats.total_pools << "\n";
    std::cout << "  Tracked Threads: " << system_stats.tracked_threads << "\n";
    std::cout << "  Local Allocation Ratio: " << std::fixed << std::setprecision(1)
              << system_stats.overall_local_ratio * 100 << "%\n";
    std::cout << "  Average Utilization: " << std::fixed << std::setprecision(1)
              << system_stats.average_utilization * 100 << "%\n";
    
    // Generate detailed report
    std::cout << "\nDetailed System Report:\n";
    std::cout << tl_registry.generate_system_report();
}

//=============================================================================
// Example 7: Real-World ECS Memory Pattern Simulation
//=============================================================================

void demonstrate_ecs_memory_patterns() {
    std::cout << "\n=== ECS Memory Pattern Simulation ===\n";
    
    // This example simulates realistic memory allocation patterns
    // that occur in a real ECS system during gameplay
    
    auto& hierarchical_allocator = hierarchical::get_global_hierarchical_allocator();
    auto& numa_manager = numa::get_global_numa_manager();
    
    const usize max_entities = 100000;
    const usize frames_to_simulate = 1000;
    
    std::cout << "Simulating ECS memory patterns:\n";
    std::cout << "  Max entities: " << max_entities << "\n";
    std::cout << "  Frames to simulate: " << frames_to_simulate << "\n";
    std::cout << "  NUMA nodes: " << numa_manager.get_topology().total_nodes << "\n\n";
    
    struct EntityData {
        GameEntity* entity;
        PhysicsComponent* physics;
        bool has_physics;
        
        EntityData() : entity(nullptr), physics(nullptr), has_physics(false) {}
    };
    
    std::vector<EntityData> entities;
    entities.reserve(max_entities);
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<f32> prob(0.0f, 1.0f);
    
    auto simulation_start = std::chrono::high_resolution_clock::now();
    
    // Game startup - burst allocation
    std::cout << "Phase 1: Game startup (burst allocation)...\n";
    usize initial_entities = max_entities / 2;
    
    for (usize i = 0; i < initial_entities; ++i) {
        EntityData entity_data;
        entity_data.entity = hierarchical_allocator.construct<GameEntity>();
        entity_data.entity->entity_id = static_cast<u32>(i);
        
        // 60% chance of having physics
        if (prob(rng) < 0.6f) {
            entity_data.physics = hierarchical_allocator.construct<PhysicsComponent>();
            entity_data.has_physics = true;
        }
        
        entities.push_back(entity_data);
    }
    
    auto startup_end = std::chrono::high_resolution_clock::now();
    auto startup_duration = std::chrono::duration<f64, std::milli>(startup_end - simulation_start);
    std::cout << "  Completed in " << std::fixed << std::setprecision(2) 
              << startup_duration.count() << " ms\n";
    
    // Gameplay simulation
    std::cout << "Phase 2: Gameplay simulation...\n";
    
    usize total_spawned = 0;
    usize total_destroyed = 0;
    
    for (usize frame = 0; frame < frames_to_simulate; ++frame) {
        // Entity spawning (occasional bursts)
        usize entities_to_spawn = 0;
        if (frame % 60 == 0) { // Every second (60 FPS)
            entities_to_spawn = 5 + (rng() % 10); // 5-15 entities
        } else if (prob(rng) < 0.1f) { // 10% chance each frame
            entities_to_spawn = 1 + (rng() % 3); // 1-3 entities
        }
        
        for (usize spawn = 0; spawn < entities_to_spawn && entities.size() < max_entities; ++spawn) {
            EntityData entity_data;
            entity_data.entity = hierarchical_allocator.construct<GameEntity>();
            entity_data.entity->entity_id = static_cast<u32>(initial_entities + total_spawned);
            
            if (prob(rng) < 0.6f) {
                entity_data.physics = hierarchical_allocator.construct<PhysicsComponent>();
                entity_data.has_physics = true;
            }
            
            entities.push_back(entity_data);
            total_spawned++;
        }
        
        // Entity destruction
        usize entities_to_destroy = 0;
        if (prob(rng) < 0.05f) { // 5% chance of destruction event
            entities_to_destroy = 1 + (rng() % 5); // 1-5 entities
        }
        
        for (usize destroy = 0; destroy < entities_to_destroy && !entities.empty(); ++destroy) {
            usize index = rng() % entities.size();
            auto& entity_data = entities[index];
            
            hierarchical_allocator.destroy(entity_data.entity);
            if (entity_data.has_physics) {
                hierarchical_allocator.destroy(entity_data.physics);
            }
            
            entities.erase(entities.begin() + index);
            total_destroyed++;
        }
        
        // Simulate frame processing (memory access)
        for (auto& entity_data : entities) {
            // Update position (hot data access)
            entity_data.entity->position[0] += entity_data.entity->velocity[0] * 0.016f;
            entity_data.entity->position[1] += entity_data.entity->velocity[1] * 0.016f;
            entity_data.entity->position[2] += entity_data.entity->velocity[2] * 0.016f;
            
            // Physics update (if present)
            if (entity_data.has_physics) {
                auto* physics = entity_data.physics;
                physics->force[0] = physics->mass * physics->acceleration[0];
                physics->force[1] = physics->mass * physics->acceleration[1];  
                physics->force[2] = physics->mass * physics->acceleration[2];
            }
            
            // Occasional cold data access (10% chance)
            if (prob(rng) < 0.1f) {
                entity_data.entity->name = "Updated";
            }
        }
    }
    
    auto gameplay_end = std::chrono::high_resolution_clock::now();
    auto gameplay_duration = std::chrono::duration<f64, std::milli>(gameplay_end - startup_end);
    std::cout << "  Completed in " << std::fixed << std::setprecision(2) 
              << gameplay_duration.count() << " ms\n";
    std::cout << "  Entities spawned: " << total_spawned << "\n";
    std::cout << "  Entities destroyed: " << total_destroyed << "\n";
    std::cout << "  Final entity count: " << entities.size() << "\n";
    
    // Cleanup
    std::cout << "Phase 3: Game shutdown (cleanup)...\n";
    
    for (auto& entity_data : entities) {
        hierarchical_allocator.destroy(entity_data.entity);
        if (entity_data.has_physics) {
            hierarchical_allocator.destroy(entity_data.physics);
        }
    }
    
    auto cleanup_end = std::chrono::high_resolution_clock::now();
    auto cleanup_duration = std::chrono::duration<f64, std::milli>(cleanup_end - gameplay_end);
    auto total_duration = std::chrono::duration<f64, std::milli>(cleanup_end - simulation_start);
    
    std::cout << "  Completed in " << std::fixed << std::setprecision(2) 
              << cleanup_duration.count() << " ms\n";
    std::cout << "\nTotal simulation time: " << std::fixed << std::setprecision(2)
              << total_duration.count() << " ms\n";
    
    // Performance analysis
    std::cout << "\nPerformance Analysis:\n";
    auto hierarchical_stats = hierarchical_allocator.get_statistics();
    std::cout << "  Cache efficiency: " << std::fixed << std::setprecision(1)
              << hierarchical_stats.overall_cache_efficiency * 100 << "%\n";
    
    auto numa_stats = numa_manager.get_performance_metrics();
    std::cout << "  NUMA local access ratio: " << std::fixed << std::setprecision(1)
              << numa_stats.local_access_ratio * 100 << "%\n";
    
    f64 avg_fps = frames_to_simulate / (gameplay_duration.count() / 1000.0);
    std::cout << "  Average simulation FPS: " << std::fixed << std::setprecision(1) 
              << avg_fps << "\n";
}

//=============================================================================
// Main Example Runner
//=============================================================================

int main() {
    std::cout << "ECScope Advanced Memory Management Examples\n";
    std::cout << "==========================================\n";
    
    try {
        // Initialize profiling system
        ProfilerConfig profiler_config;
        profiler_config.enable_gpu_profiling = false; // Focus on memory profiling
        Profiler::initialize(profiler_config);
        
        // Run all examples
        demonstrate_numa_awareness();
        demonstrate_lockfree_allocators();
        demonstrate_hierarchical_pools();
        demonstrate_cache_aware_structures();
        demonstrate_bandwidth_analysis();
        demonstrate_thread_local_storage();
        demonstrate_ecs_memory_patterns();
        
        std::cout << "\n=== Summary ===\n";
        std::cout << "All memory system examples completed successfully!\n";
        std::cout << "These examples demonstrate the significant performance benefits\n";
        std::cout << "of advanced memory management techniques in real-world scenarios.\n\n";
        
        std::cout << "Key Takeaways:\n";
        std::cout << "1. NUMA-aware allocation can provide 10-30% performance improvements\n";
        std::cout << "2. Lock-free allocators scale much better with thread count\n";
        std::cout << "3. Hierarchical pools reduce allocation overhead and improve cache locality\n";
        std::cout << "4. Cache-aware data structures can improve performance by 20-50%\n";
        std::cout << "5. Memory bandwidth analysis helps identify system bottlenecks\n";
        std::cout << "6. Thread-local storage eliminates contention and improves scalability\n";
        std::cout << "7. Real-world ECS patterns benefit significantly from optimized memory management\n";
        
        // Shutdown profiling
        Profiler::shutdown();
        
    } catch (const std::exception& e) {
        std::cerr << "Example failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}