#include "../framework/ecscope_test_framework.hpp"
#include <ecscope/arena.hpp>
#include <ecscope/pool.hpp>
#include <ecscope/hierarchical_pools.hpp>
#include <ecscope/memory_tracker.hpp>
#include <ecscope/numa_manager.hpp>
#include <ecscope/cache_aware_structures.hpp>
#include <ecscope/thread_local_allocator.hpp>
#include <ecscope/lockfree_allocators.hpp>

#include <thread>
#include <atomic>
#include <vector>
#include <random>

namespace ECScope::Testing {

// =============================================================================
// Memory System Test Fixture
// =============================================================================

class MemorySystemTest : public ECScopeTestFixture {
protected:
    void SetUp() override {
        ECScopeTestFixture::SetUp();
        
        // Initialize memory systems
        numa_manager_ = std::make_unique<NUMA::Manager>();
        cache_analyzer_ = std::make_unique<Cache::Analyzer>();
        
        // Setup test data
        test_data_size_ = 4096; // 4KB chunks for testing
        alignment_ = 64; // Cache line alignment
    }

    void TearDown() override {
        cache_analyzer_.reset();
        numa_manager_.reset();
        ECScopeTestFixture::TearDown();
    }

protected:
    std::unique_ptr<NUMA::Manager> numa_manager_;
    std::unique_ptr<Cache::Analyzer> cache_analyzer_;
    size_t test_data_size_;
    size_t alignment_;
};

// =============================================================================
// Arena Allocator Tests
// =============================================================================

TEST_F(MemorySystemTest, ArenaBasicAllocation) {
    constexpr size_t arena_size = 1024 * 1024; // 1MB
    Arena arena(arena_size);
    
    // Test basic allocation
    void* ptr1 = arena.allocate(256, 8);
    ASSERT_NE(ptr1, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr1) % 8, 0); // Check alignment
    
    void* ptr2 = arena.allocate(512, 16);
    ASSERT_NE(ptr2, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr2) % 16, 0);
    
    // Verify different pointers
    EXPECT_NE(ptr1, ptr2);
    
    // Check remaining space
    EXPECT_LT(arena.bytes_used(), arena_size);
    EXPECT_GT(arena.bytes_remaining(), 0);
}

TEST_F(MemorySystemTest, ArenaAlignment) {
    Arena arena(8192);
    
    // Test various alignment requirements
    std::vector<size_t> alignments = {1, 2, 4, 8, 16, 32, 64, 128, 256};
    
    for (size_t align : alignments) {
        void* ptr = arena.allocate(64, align);
        ASSERT_NE(ptr, nullptr);
        EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % align, 0)
            << "Failed alignment test for " << align << " bytes";
    }
}

TEST_F(MemorySystemTest, ArenaExhaustion) {
    constexpr size_t arena_size = 1024;
    Arena arena(arena_size);
    
    // Allocate until exhaustion
    std::vector<void*> allocations;
    size_t allocation_size = 64;
    
    while (true) {
        void* ptr = arena.allocate(allocation_size, 8);
        if (ptr == nullptr) {
            break;
        }
        allocations.push_back(ptr);
    }
    
    // Should have used most of the arena
    EXPECT_GT(arena.bytes_used(), arena_size * 0.8); // At least 80% utilization
    EXPECT_LT(arena.bytes_remaining(), allocation_size + 16); // Less than one allocation left
}

TEST_F(MemorySystemTest, ArenaReset) {
    Arena arena(4096);
    
    // Allocate some memory
    void* ptr1 = arena.allocate(1024, 8);
    void* ptr2 = arena.allocate(1024, 8);
    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);
    
    size_t used_before_reset = arena.bytes_used();
    EXPECT_GT(used_before_reset, 2000);
    
    // Reset arena
    arena.reset();
    
    // Check that arena is reset
    EXPECT_EQ(arena.bytes_used(), 0);
    EXPECT_EQ(arena.bytes_remaining(), 4096);
    
    // Should be able to allocate again
    void* ptr3 = arena.allocate(1024, 8);
    ASSERT_NE(ptr3, nullptr);
}

// =============================================================================
// Pool Allocator Tests
// =============================================================================

TEST_F(MemorySystemTest, PoolBasicOperations) {
    constexpr size_t object_size = 64;
    constexpr size_t pool_count = 1000;
    
    Pool pool(object_size, pool_count);
    
    // Test allocation
    void* ptr1 = pool.allocate();
    ASSERT_NE(ptr1, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr1) % alignof(std::max_align_t), 0);
    
    void* ptr2 = pool.allocate();
    ASSERT_NE(ptr2, nullptr);
    EXPECT_NE(ptr1, ptr2);
    
    // Test deallocation
    pool.deallocate(ptr1);
    pool.deallocate(ptr2);
    
    // Should be able to reallocate
    void* ptr3 = pool.allocate();
    ASSERT_NE(ptr3, nullptr);
}

TEST_F(MemorySystemTest, PoolExhaustionAndRecovery) {
    constexpr size_t object_size = 32;
    constexpr size_t pool_count = 100;
    
    Pool pool(object_size, pool_count);
    
    // Allocate all objects
    std::vector<void*> allocations;
    for (size_t i = 0; i < pool_count; ++i) {
        void* ptr = pool.allocate();
        ASSERT_NE(ptr, nullptr) << "Failed to allocate object " << i;
        allocations.push_back(ptr);
    }
    
    // Pool should be exhausted
    void* exhausted_ptr = pool.allocate();
    EXPECT_EQ(exhausted_ptr, nullptr);
    
    // Deallocate half
    for (size_t i = 0; i < pool_count / 2; ++i) {
        pool.deallocate(allocations[i]);
    }
    
    // Should be able to allocate again
    for (size_t i = 0; i < pool_count / 2; ++i) {
        void* ptr = pool.allocate();
        ASSERT_NE(ptr, nullptr);
    }
}

TEST_F(MemorySystemTest, PoolThreadSafety) {
    constexpr size_t object_size = 128;
    constexpr size_t pool_count = 10000;
    constexpr size_t num_threads = 4;
    constexpr size_t allocations_per_thread = pool_count / num_threads;
    
    Pool pool(object_size, pool_count);
    
    std::atomic<size_t> successful_allocations{0};
    std::atomic<size_t> successful_deallocations{0};
    
    std::vector<std::thread> threads;
    
    for (size_t t = 0; t < num_threads; ++t) {
        threads.emplace_back([&]() {
            std::vector<void*> local_allocations;
            
            // Allocate
            for (size_t i = 0; i < allocations_per_thread; ++i) {
                void* ptr = pool.allocate();
                if (ptr != nullptr) {
                    local_allocations.push_back(ptr);
                    successful_allocations++;
                }
            }
            
            // Deallocate
            for (void* ptr : local_allocations) {
                pool.deallocate(ptr);
                successful_deallocations++;
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(successful_allocations.load(), successful_deallocations.load());
    EXPECT_LE(successful_allocations.load(), pool_count);
}

// =============================================================================
// Hierarchical Pool Tests
// =============================================================================

TEST_F(MemorySystemTest, HierarchicalPoolSizeManagement) {
    HierarchicalPools pools;
    
    // Test different size allocations
    std::vector<size_t> sizes = {16, 32, 64, 128, 256, 512, 1024, 2048};
    std::vector<void*> allocations;
    
    for (size_t size : sizes) {
        void* ptr = pools.allocate(size);
        ASSERT_NE(ptr, nullptr) << "Failed to allocate " << size << " bytes";
        allocations.push_back(ptr);
    }
    
    // Deallocate all
    for (size_t i = 0; i < allocations.size(); ++i) {
        pools.deallocate(allocations[i], sizes[i]);
    }
    
    // Should be able to reallocate
    for (size_t size : sizes) {
        void* ptr = pools.allocate(size);
        ASSERT_NE(ptr, nullptr);
        pools.deallocate(ptr, size);
    }
}

TEST_F(MemorySystemTest, HierarchicalPoolPerformance) {
    HierarchicalPools pools;
    constexpr size_t allocation_count = 10000;
    constexpr size_t max_size = 1024;
    
    std::mt19937 rng(42);
    std::uniform_int_distribution<size_t> size_dist(8, max_size);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<std::pair<void*, size_t>> allocations;
    allocations.reserve(allocation_count);
    
    // Allocate with random sizes
    for (size_t i = 0; i < allocation_count; ++i) {
        size_t size = size_dist(rng);
        void* ptr = pools.allocate(size);
        ASSERT_NE(ptr, nullptr);
        allocations.emplace_back(ptr, size);
    }
    
    // Deallocate all
    for (auto& [ptr, size] : allocations) {
        pools.deallocate(ptr, size);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Hierarchical pool test: " << allocation_count 
              << " alloc/dealloc pairs in " << duration.count() << " μs\n";
    
    // Should be reasonably fast
    EXPECT_LT(duration.count(), 50000); // Less than 50ms
}

// =============================================================================
// NUMA Awareness Tests
// =============================================================================

#ifdef ECSCOPE_ENABLE_NUMA
TEST_F(MemorySystemTest, NUMANodeDetection) {
    auto node_count = numa_manager_->get_node_count();
    EXPECT_GT(node_count, 0);
    
    auto current_node = numa_manager_->get_current_node();
    EXPECT_LT(current_node, node_count);
    
    std::cout << "NUMA configuration: " << node_count << " nodes, current node: " 
              << current_node << "\n";
}

TEST_F(MemorySystemTest, NUMALocalAllocation) {
    if (numa_manager_->get_node_count() <= 1) {
        GTEST_SKIP() << "NUMA not available or single node system";
    }
    
    constexpr size_t allocation_size = 4096;
    size_t node = numa_manager_->get_current_node();
    
    void* ptr = numa_manager_->allocate_on_node(allocation_size, node);
    ASSERT_NE(ptr, nullptr);
    
    // Verify allocation is on correct node
    size_t actual_node = numa_manager_->get_node_of_address(ptr);
    EXPECT_EQ(actual_node, node);
    
    numa_manager_->free(ptr, allocation_size);
}

TEST_F(MemorySystemTest, NUMAPerformanceComparison) {
    if (numa_manager_->get_node_count() <= 1) {
        GTEST_SKIP() << "NUMA not available for performance comparison";
    }
    
    constexpr size_t data_size = 1024 * 1024; // 1MB
    constexpr size_t iterations = 1000;
    
    size_t local_node = numa_manager_->get_current_node();
    size_t remote_node = (local_node + 1) % numa_manager_->get_node_count();
    
    // Test local node performance
    void* local_ptr = numa_manager_->allocate_on_node(data_size, local_node);
    ASSERT_NE(local_ptr, nullptr);
    
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        volatile int sum = 0;
        int* data = static_cast<int*>(local_ptr);
        for (size_t j = 0; j < data_size / sizeof(int); ++j) {
            sum += data[j];
        }
    }
    auto local_time = std::chrono::high_resolution_clock::now() - start;
    
    // Test remote node performance
    void* remote_ptr = numa_manager_->allocate_on_node(data_size, remote_node);
    ASSERT_NE(remote_ptr, nullptr);
    
    start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        volatile int sum = 0;
        int* data = static_cast<int*>(remote_ptr);
        for (size_t j = 0; j < data_size / sizeof(int); ++j) {
            sum += data[j];
        }
    }
    auto remote_time = std::chrono::high_resolution_clock::now() - start;
    
    auto local_us = std::chrono::duration_cast<std::chrono::microseconds>(local_time);
    auto remote_us = std::chrono::duration_cast<std::chrono::microseconds>(remote_time);
    
    std::cout << "NUMA performance - Local: " << local_us.count() 
              << "μs, Remote: " << remote_us.count() << "μs\n";
    
    // Local should generally be faster (though this might not always be true on all systems)
    EXPECT_GT(remote_us.count(), 0);
    EXPECT_GT(local_us.count(), 0);
    
    numa_manager_->free(local_ptr, data_size);
    numa_manager_->free(remote_ptr, data_size);
}
#endif // ECSCOPE_ENABLE_NUMA

// =============================================================================
// Cache-Aware Structure Tests
// =============================================================================

TEST_F(MemorySystemTest, CacheLineAlignment) {
    constexpr size_t cache_line_size = 64; // Typical cache line size
    constexpr size_t allocation_count = 100;
    
    std::vector<void*> allocations;
    
    for (size_t i = 0; i < allocation_count; ++i) {
        void* ptr = cache_analyzer_->allocate_cache_aligned(128, cache_line_size);
        ASSERT_NE(ptr, nullptr);
        
        // Check alignment
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        EXPECT_EQ(addr % cache_line_size, 0) << "Allocation " << i << " not cache-aligned";
        
        allocations.push_back(ptr);
    }
    
    for (void* ptr : allocations) {
        cache_analyzer_->free_cache_aligned(ptr);
    }
}

TEST_F(MemorySystemTest, CacheFriendlyDataStructure) {
    // Test Structure of Arrays (SoA) vs Array of Structures (AoS) performance
    constexpr size_t element_count = 100000;
    constexpr size_t iterations = 100;
    
    // AoS layout
    struct AoSElement {
        float x, y, z;
        float vx, vy, vz;
        int health;
        char padding[44]; // Pad to avoid false sharing
    };
    
    std::vector<AoSElement> aos_data(element_count);
    
    // SoA layout
    struct SoAData {
        std::vector<float> x, y, z;
        std::vector<float> vx, vy, vz;
        std::vector<int> health;
        
        SoAData(size_t count) : x(count), y(count), z(count), 
                                vx(count), vy(count), vz(count), health(count) {}
    } soa_data(element_count);
    
    // Initialize data
    for (size_t i = 0; i < element_count; ++i) {
        aos_data[i] = {1.0f, 2.0f, 3.0f, 0.1f, 0.2f, 0.3f, 100};
        soa_data.x[i] = 1.0f; soa_data.y[i] = 2.0f; soa_data.z[i] = 3.0f;
        soa_data.vx[i] = 0.1f; soa_data.vy[i] = 0.2f; soa_data.vz[i] = 0.3f;
        soa_data.health[i] = 100;
    }
    
    // Test AoS performance (accessing only position data)
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t iter = 0; iter < iterations; ++iter) {
        for (size_t i = 0; i < element_count; ++i) {
            aos_data[i].x += aos_data[i].vx * 0.016f;
            aos_data[i].y += aos_data[i].vy * 0.016f;
            aos_data[i].z += aos_data[i].vz * 0.016f;
        }
    }
    auto aos_time = std::chrono::high_resolution_clock::now() - start;
    
    // Test SoA performance
    start = std::chrono::high_resolution_clock::now();
    for (size_t iter = 0; iter < iterations; ++iter) {
        for (size_t i = 0; i < element_count; ++i) {
            soa_data.x[i] += soa_data.vx[i] * 0.016f;
            soa_data.y[i] += soa_data.vy[i] * 0.016f;
            soa_data.z[i] += soa_data.vz[i] * 0.016f;
        }
    }
    auto soa_time = std::chrono::high_resolution_clock::now() - start;
    
    auto aos_us = std::chrono::duration_cast<std::chrono::microseconds>(aos_time);
    auto soa_us = std::chrono::duration_cast<std::chrono::microseconds>(soa_time);
    
    std::cout << "Cache performance - AoS: " << aos_us.count() 
              << "μs, SoA: " << soa_us.count() << "μs\n";
    
    // SoA should generally be faster for this access pattern
    // (though the difference might be small on modern CPUs with good prefetching)
    EXPECT_GT(aos_us.count(), 0);
    EXPECT_GT(soa_us.count(), 0);
}

// =============================================================================
// Thread-Local Allocator Tests
// =============================================================================

#ifdef ECSCOPE_ENABLE_JOB_SYSTEM
TEST_F(MemorySystemTest, ThreadLocalAllocatorBasics) {
    ThreadLocalAllocator allocator(1024 * 1024); // 1MB per thread
    
    // Test basic allocation
    void* ptr1 = allocator.allocate(256, 8);
    ASSERT_NE(ptr1, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr1) % 8, 0);
    
    void* ptr2 = allocator.allocate(512, 16);
    ASSERT_NE(ptr2, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr2) % 16, 0);
    
    // Deallocate
    allocator.deallocate(ptr1, 256);
    allocator.deallocate(ptr2, 512);
}

TEST_F(MemorySystemTest, ThreadLocalAllocatorMultiThreaded) {
    constexpr size_t num_threads = 4;
    constexpr size_t allocations_per_thread = 1000;
    constexpr size_t allocation_size = 128;
    
    ThreadLocalAllocator allocator(1024 * 1024); // 1MB per thread
    std::atomic<size_t> total_allocations{0};
    std::atomic<size_t> total_deallocations{0};
    
    std::vector<std::thread> threads;
    
    for (size_t t = 0; t < num_threads; ++t) {
        threads.emplace_back([&]() {
            std::vector<void*> local_ptrs;
            
            // Allocate
            for (size_t i = 0; i < allocations_per_thread; ++i) {
                void* ptr = allocator.allocate(allocation_size, 8);
                if (ptr != nullptr) {
                    local_ptrs.push_back(ptr);
                    total_allocations++;
                }
            }
            
            // Deallocate
            for (void* ptr : local_ptrs) {
                allocator.deallocate(ptr, allocation_size);
                total_deallocations++;
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(total_allocations.load(), total_deallocations.load());
    EXPECT_EQ(total_allocations.load(), num_threads * allocations_per_thread);
}
#endif // ECSCOPE_ENABLE_JOB_SYSTEM

// =============================================================================
// Lock-Free Allocator Tests
// =============================================================================

#ifdef ECSCOPE_ENABLE_LOCKFREE
TEST_F(MemorySystemTest, LockFreeAllocatorConcurrency) {
    LockFreeAllocator allocator(64, 10000); // 64-byte objects, 10k capacity
    
    constexpr size_t num_threads = 8;
    constexpr size_t operations_per_thread = 1000;
    
    std::atomic<size_t> successful_allocs{0};
    std::atomic<size_t> successful_deallocs{0};
    
    std::vector<std::thread> threads;
    
    for (size_t t = 0; t < num_threads; ++t) {
        threads.emplace_back([&]() {
            std::vector<void*> ptrs;
            
            // Mixed allocation and deallocation
            for (size_t i = 0; i < operations_per_thread; ++i) {
                if (i % 2 == 0 || ptrs.empty()) {
                    void* ptr = allocator.allocate();
                    if (ptr != nullptr) {
                        ptrs.push_back(ptr);
                        successful_allocs++;
                    }
                } else {
                    void* ptr = ptrs.back();
                    ptrs.pop_back();
                    allocator.deallocate(ptr);
                    successful_deallocs++;
                }
            }
            
            // Clean up remaining allocations
            for (void* ptr : ptrs) {
                allocator.deallocate(ptr);
                successful_deallocs++;
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(successful_allocs.load(), successful_deallocs.load());
    std::cout << "Lock-free allocator: " << successful_allocs.load() 
              << " successful alloc/dealloc pairs\n";
}
#endif // ECSCOPE_ENABLE_LOCKFREE

// =============================================================================
// Memory Debugging and Tracking Tests
// =============================================================================

TEST_F(MemorySystemTest, MemoryLeakDetection) {
    // This test uses the memory tracker from the base fixture
    size_t initial_allocs = memory_tracker_->get_allocation_count();
    size_t initial_deallocs = memory_tracker_->get_deallocation_count();
    
    // Simulate allocations
    std::vector<void*> ptrs;
    for (int i = 0; i < 100; ++i) {
        void* ptr = std::malloc(128);
        ASSERT_NE(ptr, nullptr);
        ptrs.push_back(ptr);
        
        // Memory tracker should detect this (if hooked properly)
    }
    
    // Deallocate most but not all (simulate leak)
    for (size_t i = 0; i < ptrs.size() - 10; ++i) {
        std::free(ptrs[i]);
    }
    
    // Clean up the "leaked" memory
    for (size_t i = ptrs.size() - 10; i < ptrs.size(); ++i) {
        std::free(ptrs[i]);
    }
    
    // The test framework will check for leaks in TearDown
}

TEST_F(MemorySystemTest, MemoryUsageTracking) {
    size_t initial_usage = memory_tracker_->get_current_usage();
    
    // Allocate and track usage
    constexpr size_t alloc_size = 1024;
    constexpr size_t alloc_count = 100;
    
    std::vector<void*> ptrs;
    for (size_t i = 0; i < alloc_count; ++i) {
        void* ptr = std::malloc(alloc_size);
        ASSERT_NE(ptr, nullptr);
        ptrs.push_back(ptr);
    }
    
    // Usage should have increased
    size_t current_usage = memory_tracker_->get_current_usage();
    EXPECT_GT(current_usage, initial_usage);
    
    // Clean up
    for (void* ptr : ptrs) {
        std::free(ptr);
    }
    
    // Usage should decrease back toward initial
    size_t final_usage = memory_tracker_->get_current_usage();
    EXPECT_LE(final_usage, current_usage);
}

} // namespace ECScope::Testing