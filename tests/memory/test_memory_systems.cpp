#include "../framework/ecscope_test_framework.hpp"
#include <ecscope/arena.hpp>
#include <ecscope/pool.hpp>
#include <ecscope/memory_tracker_example.hpp>
#include <ecscope/bandwidth_analyzer.hpp>

#ifdef ECSCOPE_ENABLE_NUMA
#include <ecscope/numa_manager.hpp>
#endif

namespace ECScope::Testing {

class MemorySystemTest : public PerformanceTestFixture {
protected:
    void SetUp() override {
        PerformanceTestFixture::SetUp();
        
        // Initialize memory tracking with detailed analysis
        detailed_tracker_ = std::make_unique<MemoryTracker>("DetailedMemoryTest");
        detailed_tracker_->start_tracking();
        
        // Initialize bandwidth analyzer
        bandwidth_analyzer_ = std::make_unique<BandwidthAnalyzer>();
        
#ifdef ECSCOPE_ENABLE_NUMA
        // Initialize NUMA manager if available
        if (NumaManager::is_numa_available()) {
            numa_manager_ = std::make_unique<NumaManager>();
            numa_manager_->initialize();
        }
#endif
    }

    void TearDown() override {
#ifdef ECSCOPE_ENABLE_NUMA
        if (numa_manager_) {
            numa_manager_->shutdown();
        }
#endif
        
        if (detailed_tracker_) {
            detailed_tracker_->stop_tracking();
            
            // Log detailed memory statistics
            auto stats = detailed_tracker_->get_detailed_stats();
            std::cout << "Memory Test Statistics:\n"
                      << "  Total Allocations: " << stats.total_allocations << "\n"
                      << "  Total Deallocations: " << stats.total_deallocations << "\n"
                      << "  Peak Memory Usage: " << stats.peak_memory_usage << " bytes\n"
                      << "  Average Allocation Size: " << stats.average_allocation_size << " bytes\n";
        }
        
        PerformanceTestFixture::TearDown();
    }

protected:
    std::unique_ptr<MemoryTracker> detailed_tracker_;
    std::unique_ptr<BandwidthAnalyzer> bandwidth_analyzer_;
    
#ifdef ECSCOPE_ENABLE_NUMA
    std::unique_ptr<NumaManager> numa_manager_;
#endif
};

// =============================================================================
// Arena Allocator Tests
// =============================================================================

TEST_F(MemorySystemTest, ArenaBasicOperations) {
    constexpr size_t arena_size = 1024 * 1024; // 1MB
    Arena arena(arena_size);
    
    EXPECT_EQ(arena.size(), arena_size);
    EXPECT_EQ(arena.used(), 0);
    EXPECT_EQ(arena.remaining(), arena_size);
    
    // Test basic allocation
    void* ptr1 = arena.allocate(100);
    EXPECT_NE(ptr1, nullptr);
    EXPECT_EQ(arena.used(), 100);
    EXPECT_EQ(arena.remaining(), arena_size - 100);
    
    // Test aligned allocation
    void* ptr2 = arena.allocate(64, 16);
    EXPECT_NE(ptr2, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr2) % 16, 0);
    
    // Test arena reset
    arena.reset();
    EXPECT_EQ(arena.used(), 0);
    EXPECT_EQ(arena.remaining(), arena_size);
}

TEST_F(MemorySystemTest, ArenaExhaustionHandling) {
    constexpr size_t arena_size = 1024;
    Arena arena(arena_size);
    
    // Fill the arena
    std::vector<void*> ptrs;
    size_t total_allocated = 0;
    
    while (total_allocated < arena_size) {
        void* ptr = arena.allocate(64);
        if (ptr == nullptr) break;
        ptrs.push_back(ptr);
        total_allocated += 64;
    }
    
    // Next allocation should fail
    void* ptr = arena.allocate(64);
    EXPECT_EQ(ptr, nullptr);
    
    // After reset, allocation should work again
    arena.reset();
    ptr = arena.allocate(64);
    EXPECT_NE(ptr, nullptr);
}

TEST_F(MemorySystemTest, ArenaPerformance) {
    constexpr size_t arena_size = 16 * 1024 * 1024; // 16MB
    constexpr size_t allocation_count = 100000;
    
    Arena arena(arena_size);
    
    // Benchmark arena allocations
    benchmark("ArenaAllocation", [&]() {
        arena.reset();
        for (size_t i = 0; i < allocation_count; ++i) {
            void* ptr = arena.allocate(64);
            EXPECT_NE(ptr, nullptr);
        }
    }, 100);
    
    // Compare with standard malloc
    benchmark("MallocAllocation", [&]() {
        std::vector<void*> ptrs;
        ptrs.reserve(allocation_count);
        
        for (size_t i = 0; i < allocation_count; ++i) {
            ptrs.push_back(std::malloc(64));
        }
        
        for (void* ptr : ptrs) {
            std::free(ptr);
        }
    }, 100);
}

// =============================================================================
// Pool Allocator Tests
// =============================================================================

TEST_F(MemorySystemTest, PoolBasicOperations) {
    constexpr size_t block_size = 64;
    constexpr size_t block_count = 1000;
    
    Pool pool(block_size, block_count);
    
    EXPECT_EQ(pool.block_size(), block_size);
    EXPECT_EQ(pool.total_blocks(), block_count);
    EXPECT_EQ(pool.available_blocks(), block_count);
    
    // Allocate a block
    void* ptr1 = pool.allocate();
    EXPECT_NE(ptr1, nullptr);
    EXPECT_EQ(pool.available_blocks(), block_count - 1);
    
    // Allocate another block
    void* ptr2 = pool.allocate();
    EXPECT_NE(ptr2, nullptr);
    EXPECT_NE(ptr1, ptr2);
    EXPECT_EQ(pool.available_blocks(), block_count - 2);
    
    // Deallocate blocks
    pool.deallocate(ptr1);
    EXPECT_EQ(pool.available_blocks(), block_count - 1);
    
    pool.deallocate(ptr2);
    EXPECT_EQ(pool.available_blocks(), block_count);
}

TEST_F(MemorySystemTest, PoolExhaustion) {
    constexpr size_t block_count = 10;
    Pool pool(64, block_count);
    
    std::vector<void*> ptrs;
    
    // Allocate all blocks
    for (size_t i = 0; i < block_count; ++i) {
        void* ptr = pool.allocate();
        EXPECT_NE(ptr, nullptr);
        ptrs.push_back(ptr);
    }
    
    EXPECT_EQ(pool.available_blocks(), 0);
    
    // Next allocation should fail
    void* ptr = pool.allocate();
    EXPECT_EQ(ptr, nullptr);
    
    // Deallocate one block and try again
    pool.deallocate(ptrs[0]);
    ptr = pool.allocate();
    EXPECT_NE(ptr, nullptr);
}

TEST_F(MemorySystemTest, PoolPerformance) {
    constexpr size_t block_count = 10000;
    constexpr size_t iterations = 1000;
    
    Pool pool(64, block_count);
    
    // Benchmark pool allocations and deallocations
    benchmark("PoolAllocationDeallocation", [&]() {
        std::vector<void*> ptrs;
        ptrs.reserve(block_count);
        
        // Allocate all blocks
        for (size_t i = 0; i < block_count; ++i) {
            ptrs.push_back(pool.allocate());
        }
        
        // Deallocate all blocks
        for (void* ptr : ptrs) {
            pool.deallocate(ptr);
        }
    }, iterations);
}

// =============================================================================
// Memory Tracking Tests
// =============================================================================

TEST_F(MemorySystemTest, MemoryTrackingAccuracy) {
    MemoryTracker tracker("AccuracyTest");
    tracker.start_tracking();
    
    auto initial_stats = tracker.get_stats();
    
    // Allocate some memory
    constexpr size_t allocation_size = 1024;
    void* ptr1 = tracker.track_allocation(std::malloc(allocation_size), allocation_size);
    void* ptr2 = tracker.track_allocation(std::malloc(allocation_size * 2), allocation_size * 2);
    
    auto mid_stats = tracker.get_stats();
    EXPECT_EQ(mid_stats.total_allocations, initial_stats.total_allocations + 2);
    EXPECT_EQ(mid_stats.current_memory_usage, initial_stats.current_memory_usage + allocation_size * 3);
    
    // Deallocate memory
    tracker.track_deallocation(ptr1, allocation_size);
    std::free(ptr1);
    
    auto final_stats = tracker.get_stats();
    EXPECT_EQ(final_stats.total_deallocations, initial_stats.total_deallocations + 1);
    EXPECT_EQ(final_stats.current_memory_usage, initial_stats.current_memory_usage + allocation_size * 2);
    
    // Clean up
    tracker.track_deallocation(ptr2, allocation_size * 2);
    std::free(ptr2);
    
    tracker.stop_tracking();
    EXPECT_NO_MEMORY_LEAKS();
}

TEST_F(MemorySystemTest, MemoryLeakDetection) {
    MemoryTracker tracker("LeakTest");
    tracker.start_tracking();
    
    // Intentionally create a "leak" (we'll clean it up manually)
    void* leaked_ptr = tracker.track_allocation(std::malloc(1024), 1024);
    
    tracker.stop_tracking();
    
    auto stats = tracker.get_stats();
    EXPECT_GT(stats.total_allocations, stats.total_deallocations);
    
    // Clean up the "leak"
    std::free(leaked_ptr);
}

// =============================================================================
// Bandwidth Analysis Tests
// =============================================================================

TEST_F(MemorySystemTest, BandwidthMeasurement) {
    constexpr size_t buffer_size = 1024 * 1024; // 1MB
    constexpr size_t iterations = 100;
    
    std::vector<uint8_t> source(buffer_size, 0xAA);
    std::vector<uint8_t> destination(buffer_size);
    
    bandwidth_analyzer_->start_measurement("MemoryBandwidthTest");
    
    for (size_t i = 0; i < iterations; ++i) {
        std::memcpy(destination.data(), source.data(), buffer_size);
    }
    
    auto result = bandwidth_analyzer_->end_measurement("MemoryBandwidthTest");
    
    EXPECT_GT(result.bandwidth_mb_per_sec, 0.0);
    EXPECT_GT(result.total_bytes, 0);
    
    std::cout << "Memory bandwidth: " << result.bandwidth_mb_per_sec << " MB/s\n";
}

TEST_F(MemorySystemTest, CacheAwareAccess) {
    constexpr size_t array_size = 1024 * 1024;
    std::vector<int> data(array_size);
    
    // Sequential access (cache-friendly)
    bandwidth_analyzer_->start_measurement("SequentialAccess");
    volatile int sum1 = 0;
    for (size_t i = 0; i < array_size; ++i) {
        sum1 += data[i];
    }
    auto sequential_result = bandwidth_analyzer_->end_measurement("SequentialAccess");
    
    // Random access (cache-unfriendly)
    std::vector<size_t> random_indices(array_size);
    std::iota(random_indices.begin(), random_indices.end(), 0);
    std::random_shuffle(random_indices.begin(), random_indices.end());
    
    bandwidth_analyzer_->start_measurement("RandomAccess");
    volatile int sum2 = 0;
    for (size_t idx : random_indices) {
        sum2 += data[idx];
    }
    auto random_result = bandwidth_analyzer_->end_measurement("RandomAccess");
    
    // Sequential access should be faster
    EXPECT_GT(sequential_result.bandwidth_mb_per_sec, random_result.bandwidth_mb_per_sec);
    
    std::cout << "Sequential access: " << sequential_result.bandwidth_mb_per_sec << " MB/s\n";
    std::cout << "Random access: " << random_result.bandwidth_mb_per_sec << " MB/s\n";
}

// =============================================================================
// NUMA Tests (if available)
// =============================================================================

#ifdef ECSCOPE_ENABLE_NUMA
TEST_F(MemorySystemTest, NumaBasicOperations) {
    if (!numa_manager_) {
        GTEST_SKIP() << "NUMA not available on this system";
    }
    
    auto node_count = numa_manager_->get_node_count();
    EXPECT_GT(node_count, 0);
    
    // Test allocation on specific NUMA node
    constexpr size_t allocation_size = 1024 * 1024; // 1MB
    void* ptr = numa_manager_->allocate_on_node(0, allocation_size);
    EXPECT_NE(ptr, nullptr);
    
    // Verify the allocation is on the correct node
    int allocated_node = numa_manager_->get_node_of_address(ptr);
    EXPECT_EQ(allocated_node, 0);
    
    numa_manager_->deallocate(ptr, allocation_size);
}

TEST_F(MemorySystemTest, NumaPerformance) {
    if (!numa_manager_) {
        GTEST_SKIP() << "NUMA not available on this system";
    }
    
    constexpr size_t buffer_size = 4 * 1024 * 1024; // 4MB
    constexpr size_t iterations = 100;
    
    // Allocate on local node
    void* local_ptr = numa_manager_->allocate_on_current_node(buffer_size);
    EXPECT_NE(local_ptr, nullptr);
    
    // Allocate on remote node (if available)
    void* remote_ptr = nullptr;
    if (numa_manager_->get_node_count() > 1) {
        int current_node = numa_manager_->get_current_node();
        int remote_node = (current_node + 1) % numa_manager_->get_node_count();
        remote_ptr = numa_manager_->allocate_on_node(remote_node, buffer_size);
        EXPECT_NE(remote_ptr, nullptr);
    }
    
    // Benchmark local access
    bandwidth_analyzer_->start_measurement("LocalNumaAccess");
    volatile int sum1 = 0;
    int* local_data = static_cast<int*>(local_ptr);
    for (size_t iter = 0; iter < iterations; ++iter) {
        for (size_t i = 0; i < buffer_size / sizeof(int); ++i) {
            sum1 += local_data[i];
        }
    }
    auto local_result = bandwidth_analyzer_->end_measurement("LocalNumaAccess");
    
    if (remote_ptr) {
        // Benchmark remote access
        bandwidth_analyzer_->start_measurement("RemoteNumaAccess");
        volatile int sum2 = 0;
        int* remote_data = static_cast<int*>(remote_ptr);
        for (size_t iter = 0; iter < iterations; ++iter) {
            for (size_t i = 0; i < buffer_size / sizeof(int); ++i) {
                sum2 += remote_data[i];
            }
        }
        auto remote_result = bandwidth_analyzer_->end_measurement("RemoteNumaAccess");
        
        // Local access should typically be faster
        std::cout << "Local NUMA access: " << local_result.bandwidth_mb_per_sec << " MB/s\n";
        std::cout << "Remote NUMA access: " << remote_result.bandwidth_mb_per_sec << " MB/s\n";
        
        numa_manager_->deallocate(remote_ptr, buffer_size);
    }
    
    numa_manager_->deallocate(local_ptr, buffer_size);
}
#endif

// =============================================================================
// Memory Pattern Analysis Tests
// =============================================================================

TEST_F(MemorySystemTest, AllocationPatternAnalysis) {
    MemoryTracker tracker("PatternAnalysis");
    tracker.start_tracking();
    tracker.enable_pattern_analysis();
    
    // Simulate different allocation patterns
    
    // Pattern 1: Many small allocations
    std::vector<void*> small_ptrs;
    for (int i = 0; i < 1000; ++i) {
        void* ptr = tracker.track_allocation(std::malloc(32), 32);
        small_ptrs.push_back(ptr);
    }
    
    // Pattern 2: Few large allocations
    std::vector<void*> large_ptrs;
    for (int i = 0; i < 10; ++i) {
        void* ptr = tracker.track_allocation(std::malloc(1024 * 1024), 1024 * 1024);
        large_ptrs.push_back(ptr);
    }
    
    // Pattern 3: Mixed allocation/deallocation
    for (size_t i = 0; i < small_ptrs.size(); i += 2) {
        tracker.track_deallocation(small_ptrs[i], 32);
        std::free(small_ptrs[i]);
    }
    
    auto patterns = tracker.get_allocation_patterns();
    
    // Verify pattern detection
    EXPECT_GT(patterns.small_allocation_frequency, 0);
    EXPECT_GT(patterns.large_allocation_frequency, 0);
    EXPECT_GT(patterns.fragmentation_ratio, 0.0);
    
    // Clean up remaining allocations
    for (size_t i = 1; i < small_ptrs.size(); i += 2) {
        tracker.track_deallocation(small_ptrs[i], 32);
        std::free(small_ptrs[i]);
    }
    
    for (void* ptr : large_ptrs) {
        tracker.track_deallocation(ptr, 1024 * 1024);
        std::free(ptr);
    }
    
    tracker.stop_tracking();
}

// =============================================================================
// Garbage Collection Simulation Tests
// =============================================================================

TEST_F(MemorySystemTest, GarbageCollectionSimulation) {
    // Simulate a simple mark-and-sweep garbage collector
    struct GCObject {
        bool marked = false;
        std::vector<GCObject*> references;
    };
    
    std::vector<std::unique_ptr<GCObject>> objects;
    constexpr size_t object_count = 1000;
    
    // Create objects with random references
    for (size_t i = 0; i < object_count; ++i) {
        objects.emplace_back(std::make_unique<GCObject>());
    }
    
    // Create random reference graph
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, object_count - 1);
    
    for (auto& obj : objects) {
        size_t ref_count = dist(gen) % 5; // 0-4 references per object
        for (size_t j = 0; j < ref_count; ++j) {
            size_t target = dist(gen);
            obj->references.push_back(objects[target].get());
        }
    }
    
    // Mark phase: mark all reachable objects from roots
    auto mark_phase = [&]() {
        // Assume first 10 objects are roots
        std::vector<GCObject*> work_list;
        for (size_t i = 0; i < 10; ++i) {
            objects[i]->marked = true;
            work_list.push_back(objects[i].get());
        }
        
        while (!work_list.empty()) {
            GCObject* current = work_list.back();
            work_list.pop_back();
            
            for (GCObject* ref : current->references) {
                if (!ref->marked) {
                    ref->marked = true;
                    work_list.push_back(ref);
                }
            }
        }
    };
    
    // Sweep phase: count unmarked objects
    auto sweep_phase = [&]() {
        size_t collected = 0;
        for (auto& obj : objects) {
            if (!obj->marked) {
                collected++;
            }
            obj->marked = false; // Reset for next cycle
        }
        return collected;
    };
    
    // Run garbage collection cycle
    benchmark("GCMarkPhase", mark_phase, 100);
    size_t collected = sweep_phase();
    
    std::cout << "Garbage collector collected " << collected 
              << " objects out of " << object_count << "\n";
    
    EXPECT_GT(collected, 0); // Should collect some unreachable objects
    EXPECT_LT(collected, object_count); // Should not collect everything
}

// =============================================================================
// Advanced Memory Experiments Tests
// =============================================================================

#ifdef ECSCOPE_ENABLE_MEMORY_ANALYSIS
TEST_F(MemorySystemTest, MemoryExperimentsIntegration) {
    // Create memory experiment configuration
    memory::MemoryExperimentConfig experiment_config;
    experiment_config.enable_cache_analysis = true;
    experiment_config.enable_allocation_tracking = true;
    experiment_config.enable_fragmentation_analysis = true;
    
    auto memory_lab = std::make_unique<memory::MemoryExperiments>(experiment_config);
    
    // Test allocation pattern experiments
    auto sequential_experiment = memory_lab->create_sequential_allocation_experiment(
        1000, 64, memory::AllocationStrategy::Arena);
    sequential_experiment->run();
    
    auto random_experiment = memory_lab->create_random_allocation_experiment(
        1000, 64, memory::AllocationStrategy::Pool);
    random_experiment->run();
    
    // Compare results
    auto sequential_result = sequential_experiment->get_result();
    auto random_result = random_experiment->get_result();
    
    EXPECT_TRUE(sequential_result.is_valid);
    EXPECT_TRUE(random_result.is_valid);
    EXPECT_GT(sequential_result.cache_efficiency, 0.0);
    EXPECT_GT(random_result.allocation_time_ns, 0.0);
    
    // Sequential should typically be more cache-efficient
    EXPECT_GE(sequential_result.cache_efficiency, random_result.cache_efficiency * 0.8);
}
#endif

TEST_F(MemorySystemTest, AdvancedArenaManagement) {
    constexpr size_t arena_size = 2 * 1024 * 1024; // 2MB
    Arena parent_arena(arena_size);
    
    // Test sub-arena creation
    Arena sub_arena1 = parent_arena.create_sub_arena(512 * 1024); // 512KB
    Arena sub_arena2 = parent_arena.create_sub_arena(512 * 1024); // 512KB
    
    EXPECT_EQ(sub_arena1.size(), 512 * 1024);
    EXPECT_EQ(sub_arena2.size(), 512 * 1024);
    
    // Test allocation in sub-arenas
    void* ptr1 = sub_arena1.allocate(1024);
    void* ptr2 = sub_arena2.allocate(1024);
    
    EXPECT_NE(ptr1, nullptr);
    EXPECT_NE(ptr2, nullptr);
    EXPECT_NE(ptr1, ptr2);
    
    // Test arena bounds checking
    EXPECT_TRUE(parent_arena.contains(ptr1));
    EXPECT_TRUE(parent_arena.contains(ptr2));
    EXPECT_FALSE(sub_arena1.contains(ptr2));
    EXPECT_FALSE(sub_arena2.contains(ptr1));
}

TEST_F(MemorySystemTest, HierarchicalPoolSystem) {
    hierarchical::PoolManager pool_manager;
    
    // Create pools of different sizes
    auto small_pool = pool_manager.create_pool("small", 32, 1000);
    auto medium_pool = pool_manager.create_pool("medium", 128, 500);
    auto large_pool = pool_manager.create_pool("large", 512, 100);
    
    EXPECT_NE(small_pool, nullptr);
    EXPECT_NE(medium_pool, nullptr);
    EXPECT_NE(large_pool, nullptr);
    
    // Test best-fit allocation
    void* ptr_30 = pool_manager.allocate(30);   // Should use small pool
    void* ptr_100 = pool_manager.allocate(100); // Should use medium pool
    void* ptr_400 = pool_manager.allocate(400); // Should use large pool
    
    EXPECT_NE(ptr_30, nullptr);
    EXPECT_NE(ptr_100, nullptr);
    EXPECT_NE(ptr_400, nullptr);
    
    // Verify pool usage
    EXPECT_EQ(small_pool->available_blocks(), 999);
    EXPECT_EQ(medium_pool->available_blocks(), 499);
    EXPECT_EQ(large_pool->available_blocks(), 99);
    
    // Test deallocation
    pool_manager.deallocate(ptr_30, 30);
    pool_manager.deallocate(ptr_100, 100);
    pool_manager.deallocate(ptr_400, 400);
    
    EXPECT_EQ(small_pool->available_blocks(), 1000);
    EXPECT_EQ(medium_pool->available_blocks(), 500);
    EXPECT_EQ(large_pool->available_blocks(), 100);
}

#ifdef ECSCOPE_ENABLE_JOB_SYSTEM
TEST_F(MemorySystemTest, ThreadLocalAllocators) {
    constexpr size_t thread_count = 4;
    constexpr size_t allocations_per_thread = 1000;
    constexpr size_t allocation_size = 64;
    
    thread_local::AllocatorManager thread_manager;
    thread_manager.initialize_for_threads(thread_count);
    
    std::vector<std::thread> threads;
    std::vector<std::vector<void*>> thread_allocations(thread_count);
    
    // Launch threads that perform allocations
    for (size_t t = 0; t < thread_count; ++t) {
        threads.emplace_back([&, t]() {
            auto& allocator = thread_manager.get_thread_local_allocator();
            thread_allocations[t].reserve(allocations_per_thread);
            
            // Allocate
            for (size_t i = 0; i < allocations_per_thread; ++i) {
                void* ptr = allocator.allocate(allocation_size);
                EXPECT_NE(ptr, nullptr);
                thread_allocations[t].push_back(ptr);
            }
            
            // Deallocate in reverse order
            for (auto it = thread_allocations[t].rbegin(); 
                 it != thread_allocations[t].rend(); ++it) {
                allocator.deallocate(*it, allocation_size);
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify no memory leaks across all thread-local allocators
    auto total_stats = thread_manager.get_aggregate_stats();
    EXPECT_EQ(total_stats.total_allocations, total_stats.total_deallocations);
    EXPECT_EQ(total_stats.current_memory_usage, 0);
}
#endif

#ifdef ECSCOPE_ENABLE_LOCKFREE
TEST_F(MemorySystemTest, LockFreeAllocators) {
    constexpr size_t allocator_size = 1024 * 1024; // 1MB
    lockfree::LockFreeArena lockfree_arena(allocator_size);
    
    constexpr size_t thread_count = 8;
    constexpr size_t allocations_per_thread = 100;
    std::atomic<int> success_count{0};
    std::atomic<int> failure_count{0};
    
    std::vector<std::thread> threads;
    
    for (size_t t = 0; t < thread_count; ++t) {
        threads.emplace_back([&]() {
            std::vector<void*> ptrs;
            ptrs.reserve(allocations_per_thread);
            
            // Allocate
            for (size_t i = 0; i < allocations_per_thread; ++i) {
                void* ptr = lockfree_arena.try_allocate(64);
                if (ptr != nullptr) {
                    ptrs.push_back(ptr);
                    success_count++;
                } else {
                    failure_count++;
                }
            }
            
            // Deallocate
            for (void* ptr : ptrs) {
                lockfree_arena.deallocate(ptr, 64);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    int total_attempts = success_count.load() + failure_count.load();
    EXPECT_EQ(total_attempts, thread_count * allocations_per_thread);
    EXPECT_GT(success_count.load(), 0);
    
    // Should have reasonable success rate
    double success_rate = static_cast<double>(success_count.load()) / total_attempts;
    EXPECT_GT(success_rate, 0.5); // At least 50% success rate
}
#endif

TEST_F(MemorySystemTest, MemoryDebugTools) {
    memory::MemoryDebugger debugger;
    debugger.enable_heap_corruption_detection(true);
    debugger.enable_double_free_detection(true);
    debugger.enable_leak_detection(true);
    
    // Test normal allocation/deallocation
    void* ptr = debugger.debug_malloc(1024);
    EXPECT_NE(ptr, nullptr);
    EXPECT_TRUE(debugger.is_valid_pointer(ptr));
    
    debugger.debug_free(ptr);
    EXPECT_FALSE(debugger.is_valid_pointer(ptr));
    
    // Test double-free detection
    bool double_free_detected = false;
    debugger.set_double_free_callback([&](void* p) {
        double_free_detected = true;
    });
    
    void* ptr2 = debugger.debug_malloc(512);
    debugger.debug_free(ptr2);
    debugger.debug_free(ptr2); // This should trigger callback
    
    EXPECT_TRUE(double_free_detected);
    
    // Test leak detection
    void* leaked_ptr = debugger.debug_malloc(256); // Intentionally not freed
    
    auto leaks = debugger.detect_leaks();
    EXPECT_GE(leaks.size(), 1);
    
    // Clean up
    debugger.debug_free(leaked_ptr);
}

TEST_F(MemorySystemTest, GarbageCollectorSimulation) {
    memory::GenerationalGC gc;
    gc.configure_generations(3); // Young, middle, old
    gc.set_collection_thresholds({1024, 4096, 16384}); // Bytes per generation
    
    // Allocate objects in young generation
    std::vector<memory::GCObject*> young_objects;
    for (int i = 0; i < 100; ++i) {
        auto obj = gc.allocate_object(64);
        young_objects.push_back(obj);
    }
    
    EXPECT_EQ(gc.get_generation_object_count(0), 100); // Young generation
    EXPECT_EQ(gc.get_generation_object_count(1), 0);   // Middle generation
    EXPECT_EQ(gc.get_generation_object_count(2), 0);   // Old generation
    
    // Create references between objects
    for (size_t i = 0; i < young_objects.size() - 1; ++i) {
        young_objects[i]->add_reference(young_objects[i + 1]);
    }
    
    // Force young generation collection
    gc.collect_generation(0);
    
    // Some objects should survive to middle generation
    EXPECT_LT(gc.get_generation_object_count(0), 100);
    EXPECT_GT(gc.get_generation_object_count(1), 0);
    
    // Test full collection
    gc.collect_all_generations();
    
    auto collection_stats = gc.get_collection_stats();
    EXPECT_GT(collection_stats.total_collections, 0);
    EXPECT_GT(collection_stats.total_objects_collected, 0);
}

TEST_F(MemorySystemTest, MemoryMappingAndVirtualMemory) {
#ifdef ECSCOPE_ENABLE_VIRTUAL_MEMORY
    constexpr size_t virtual_size = 64 * 1024 * 1024; // 64MB virtual
    constexpr size_t physical_size = 4 * 1024 * 1024;  // 4MB physical
    
    memory::VirtualMemoryManager vm_manager;
    
    auto vm_region = vm_manager.create_region(virtual_size);
    EXPECT_NE(vm_region, nullptr);
    EXPECT_EQ(vm_region->get_virtual_size(), virtual_size);
    EXPECT_EQ(vm_region->get_committed_size(), 0);
    
    // Commit some physical memory
    bool commit_success = vm_region->commit_range(0, physical_size);
    EXPECT_TRUE(commit_success);
    EXPECT_EQ(vm_region->get_committed_size(), physical_size);
    
    // Test access to committed region
    void* base_ptr = vm_region->get_base_address();
    EXPECT_NE(base_ptr, nullptr);
    
    // Write and read data
    int* test_data = static_cast<int*>(base_ptr);
    *test_data = 42;
    EXPECT_EQ(*test_data, 42);
    
    // Test memory protection
    bool protect_success = vm_region->protect_range(
        physical_size / 2, physical_size / 2, 
        memory::MemoryProtection::ReadOnly);
    EXPECT_TRUE(protect_success);
    
    // Decommit memory
    bool decommit_success = vm_region->decommit_range(physical_size / 2, physical_size / 2);
    EXPECT_TRUE(decommit_success);
    EXPECT_EQ(vm_region->get_committed_size(), physical_size / 2);
#else
    GTEST_SKIP() << "Virtual memory support not enabled";
#endif
}

TEST_F(MemorySystemTest, MemoryCompressionAndDeduplication) {
#ifdef ECSCOPE_ENABLE_MEMORY_COMPRESSION
    memory::MemoryCompressor compressor;
    compressor.set_compression_algorithm(memory::CompressionAlgorithm::LZ4);
    compressor.set_compression_threshold(1024); // Compress blocks > 1KB
    
    // Create compressible data (repeated pattern)
    std::vector<uint8_t> test_data(4096, 0xAA);
    for (size_t i = 0; i < test_data.size(); i += 16) {
        test_data[i] = static_cast<uint8_t>(i % 256);
    }
    
    auto compressed_block = compressor.compress(test_data.data(), test_data.size());
    EXPECT_NE(compressed_block, nullptr);
    EXPECT_LT(compressed_block->get_compressed_size(), test_data.size());
    
    // Test decompression
    std::vector<uint8_t> decompressed_data(test_data.size());
    bool decompress_success = compressor.decompress(
        compressed_block, decompressed_data.data(), decompressed_data.size());
    
    EXPECT_TRUE(decompress_success);
    EXPECT_EQ(decompressed_data, test_data);
    
    // Test deduplication
    memory::MemoryDeduplicator deduplicator;
    
    // Create identical blocks
    std::vector<uint8_t> block1(1024, 0x55);
    std::vector<uint8_t> block2(1024, 0x55);
    std::vector<uint8_t> block3(1024, 0x33);
    
    auto handle1 = deduplicator.register_block(block1.data(), block1.size());
    auto handle2 = deduplicator.register_block(block2.data(), block2.size());
    auto handle3 = deduplicator.register_block(block3.data(), block3.size());
    
    // Blocks 1 and 2 should be deduplicated (same content)
    EXPECT_EQ(deduplicator.get_unique_blocks_count(), 2);
    EXPECT_LT(deduplicator.get_total_memory_usage(), 
              block1.size() + block2.size() + block3.size());
    
    // Verify data integrity
    auto retrieved1 = deduplicator.get_block_data(handle1);
    auto retrieved3 = deduplicator.get_block_data(handle3);
    
    EXPECT_EQ(std::memcmp(retrieved1, block1.data(), block1.size()), 0);
    EXPECT_EQ(std::memcmp(retrieved3, block3.data(), block3.size()), 0);
#else
    GTEST_SKIP() << "Memory compression support not enabled";
#endif
}

TEST_F(MemorySystemTest, AdvancedMemoryProfiling) {
    memory::MemoryProfiler profiler;
    profiler.start_profiling_session("AdvancedProfilingTest");
    
    // Enable various profiling features
    profiler.enable_allocation_stack_traces(true);
    profiler.enable_lifetime_analysis(true);
    profiler.enable_fragmentation_tracking(true);
    profiler.enable_cache_behavior_analysis(true);
    
    // Simulate complex allocation patterns
    std::vector<void*> allocations;
    
    // Pattern 1: Sequential allocations
    profiler.start_allocation_group("Sequential");
    for (int i = 0; i < 100; ++i) {
        void* ptr = profiler.tracked_malloc(64 + i);
        allocations.push_back(ptr);
    }
    profiler.end_allocation_group();
    
    // Pattern 2: Random size allocations
    profiler.start_allocation_group("Random");
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> size_dist(32, 1024);
    
    for (int i = 0; i < 50; ++i) {
        void* ptr = profiler.tracked_malloc(size_dist(gen));
        allocations.push_back(ptr);
    }
    profiler.end_allocation_group();
    
    // Pattern 3: Partial deallocation (creates fragmentation)
    for (size_t i = 0; i < allocations.size(); i += 2) {
        profiler.tracked_free(allocations[i]);
    }
    
    profiler.stop_profiling_session();
    
    auto profile_report = profiler.generate_report();
    EXPECT_FALSE(profile_report.allocation_groups.empty());
    EXPECT_EQ(profile_report.allocation_groups.size(), 2); // Sequential and Random
    
    // Check fragmentation analysis
    EXPECT_GT(profile_report.fragmentation_analysis.internal_fragmentation, 0.0);
    EXPECT_GT(profile_report.fragmentation_analysis.external_fragmentation, 0.0);
    
    // Check lifetime analysis
    EXPECT_GT(profile_report.lifetime_analysis.average_object_lifetime, 0.0);
    EXPECT_GT(profile_report.lifetime_analysis.short_lived_objects, 0);
    
    // Clean up remaining allocations
    for (size_t i = 1; i < allocations.size(); i += 2) {
        profiler.tracked_free(allocations[i]);
    }
}

} // namespace ECScope::Testing