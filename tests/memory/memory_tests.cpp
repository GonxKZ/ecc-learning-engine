#include <gtest/gtest.h>
#include <ecscope/memory/allocators.hpp>
#include <ecscope/memory/numa_support.hpp>
#include <ecscope/memory/memory_pools.hpp>
#include <ecscope/memory/memory_tracker.hpp>
#include <ecscope/memory/memory_utils.hpp>
#include <ecscope/memory/memory_manager.hpp>
#include <thread>
#include <vector>
#include <chrono>
#include <random>

using namespace ecscope::memory;

// ==== ALLOCATOR TESTS ====
class AllocatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup common test resources
    }
    
    void TearDown() override {
        // Cleanup
    }
};

TEST_F(AllocatorTest, LinearAllocatorBasicOperations) {
    LinearAllocator allocator(1024 * 1024); // 1MB
    
    // Test basic allocation
    void* ptr1 = allocator.allocate(100);
    ASSERT_NE(ptr1, nullptr);
    EXPECT_TRUE(allocator.owns(ptr1));
    
    void* ptr2 = allocator.allocate(200);
    ASSERT_NE(ptr2, nullptr);
    EXPECT_TRUE(allocator.owns(ptr2));
    
    // Check that pointers are different
    EXPECT_NE(ptr1, ptr2);
    
    // Check utilization
    EXPECT_GT(allocator.used(), 300);
    EXPECT_LT(allocator.available(), 1024 * 1024);
    
    // Test reset
    allocator.reset();
    EXPECT_EQ(allocator.used(), 0);
    EXPECT_EQ(allocator.available(), 1024 * 1024);
}

TEST_F(AllocatorTest, LinearAllocatorAlignment) {
    LinearAllocator allocator(1024 * 1024);
    
    // Test various alignments
    void* ptr16 = allocator.allocate(100, 16);
    ASSERT_NE(ptr16, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr16) % 16, 0);
    
    void* ptr64 = allocator.allocate(100, 64);
    ASSERT_NE(ptr64, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr64) % 64, 0);
    
    void* ptr256 = allocator.allocate(100, 256);
    ASSERT_NE(ptr256, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr256) % 256, 0);
}

TEST_F(AllocatorTest, StackAllocatorMarkers) {
    StackAllocator allocator(1024 * 1024);
    
    // Get initial marker
    auto marker1 = allocator.get_marker();
    EXPECT_EQ(marker1, 0);
    
    // Allocate some memory
    void* ptr1 = allocator.allocate(100);
    void* ptr2 = allocator.allocate(200);
    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);
    
    auto marker2 = allocator.get_marker();
    EXPECT_GT(marker2, marker1);
    
    // Allocate more memory
    void* ptr3 = allocator.allocate(300);
    ASSERT_NE(ptr3, nullptr);
    
    auto marker3 = allocator.get_marker();
    EXPECT_GT(marker3, marker2);
    
    // Unwind to marker2 (should free ptr3)
    allocator.unwind_to_marker(marker2);
    EXPECT_EQ(allocator.get_marker(), marker2);
    
    // Unwind to marker1 (should free ptr1 and ptr2)
    allocator.unwind_to_marker(marker1);
    EXPECT_EQ(allocator.get_marker(), marker1);
}

TEST_F(AllocatorTest, ObjectPoolOperations) {
    struct TestObject {
        int value = 42;
        std::string name = "test";
    };
    
    ObjectPool<TestObject> pool(100);
    
    // Test basic allocation/deallocation
    TestObject* obj1 = pool.allocate();
    ASSERT_NE(obj1, nullptr);
    EXPECT_TRUE(pool.owns(obj1));
    
    TestObject* obj2 = pool.allocate();
    ASSERT_NE(obj2, nullptr);
    EXPECT_NE(obj1, obj2);
    
    EXPECT_EQ(pool.used(), 2);
    EXPECT_EQ(pool.available(), 98);
    
    pool.deallocate(obj1);
    EXPECT_EQ(pool.used(), 1);
    EXPECT_EQ(pool.available(), 99);
    
    // Test construction/destruction
    TestObject* obj3 = pool.construct(123, "constructed");
    ASSERT_NE(obj3, nullptr);
    EXPECT_EQ(obj3->value, 123);
    EXPECT_EQ(obj3->name, "constructed");
    
    pool.destroy(obj3);
    EXPECT_EQ(pool.used(), 1);
}

TEST_F(AllocatorTest, FreeListAllocatorCoalescing) {
    FreeListAllocator allocator(1024 * 1024);
    
    // Allocate several blocks
    void* ptr1 = allocator.allocate(100);
    void* ptr2 = allocator.allocate(100);
    void* ptr3 = allocator.allocate(100);
    
    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);
    ASSERT_NE(ptr3, nullptr);
    
    size_t used_after_alloc = allocator.used();
    
    // Free middle block
    allocator.deallocate(ptr2, 100);
    
    // Free adjacent blocks (should coalesce)
    allocator.deallocate(ptr1, 100);
    allocator.deallocate(ptr3, 100);
    
    // Memory should be mostly recovered due to coalescing
    EXPECT_LT(allocator.used(), used_after_alloc / 2);
}

// ==== NUMA TESTS ====
TEST_F(AllocatorTest, NUMATopologyDetection) {
    auto& topology = NUMATopology::instance();
    
    // Should detect at least one node
    EXPECT_GT(topology.get_num_nodes(), 0);
    
    // Current node should be valid
    uint32_t current_node = topology.get_current_node();
    EXPECT_LT(current_node, topology.get_num_nodes());
    
    // Node info should be valid
    const auto& node_info = topology.get_node_info(current_node);
    EXPECT_EQ(node_info.node_id, current_node);
    EXPECT_GT(node_info.cpu_ids.size(), 0);
}

TEST_F(AllocatorTest, NUMAAllocator) {
    NUMAAllocator allocator;
    
    // Test basic allocation
    void* ptr = allocator.allocate(1024);
    ASSERT_NE(ptr, nullptr);
    EXPECT_TRUE(allocator.owns(ptr));
    
    allocator.deallocate(ptr, 1024);
    
    // Test statistics
    auto stats = allocator.get_node_statistics();
    EXPECT_GT(stats.size(), 0);
    
    for (const auto& [node_id, node_stats] : stats) {
        EXPECT_GT(node_stats.capacity, 0);
    }
}

// ==== THREAD SAFETY TESTS ====
TEST_F(AllocatorTest, ThreadSafeAllocatorConcurrency) {
    ThreadSafeAllocator allocator;
    constexpr int NUM_THREADS = 8;
    constexpr int ALLOCATIONS_PER_THREAD = 1000;
    
    std::vector<std::thread> threads;
    std::vector<std::vector<void*>> thread_ptrs(NUM_THREADS);
    
    // Launch threads that allocate memory
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([&, t]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<size_t> size_dist(16, 1024);
            
            for (int i = 0; i < ALLOCATIONS_PER_THREAD; ++i) {
                size_t size = size_dist(gen);
                void* ptr = allocator.allocate(size);
                if (ptr) {
                    thread_ptrs[t].push_back(ptr);
                    // Write to memory to ensure it's valid
                    std::memset(ptr, t & 0xFF, size);
                }
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify allocations and deallocate
    size_t total_allocations = 0;
    for (int t = 0; t < NUM_THREADS; ++t) {
        total_allocations += thread_ptrs[t].size();
        for (void* ptr : thread_ptrs[t]) {
            EXPECT_TRUE(allocator.owns(ptr));
            allocator.deallocate(ptr, 0); // Size not needed for cleanup
        }
    }
    
    EXPECT_GT(total_allocations, NUM_THREADS * ALLOCATIONS_PER_THREAD * 0.8);
}

// ==== MEMORY TRACKING TESTS ====
class MemoryTrackingTest : public ::testing::Test {
protected:
    void SetUp() override {
        leak_detector_ = std::make_unique<MemoryLeakDetector>(false); // Disable stack traces for speed
    }
    
    std::unique_ptr<MemoryLeakDetector> leak_detector_;
};

TEST_F(MemoryTrackingTest, LeakDetection) {
    // Simulate allocations without corresponding deallocations
    void* ptr1 = std::malloc(100);
    void* ptr2 = std::malloc(200);
    
    leak_detector_->record_allocation(ptr1, 100, alignof(std::max_align_t), "test_leak_1");
    leak_detector_->record_allocation(ptr2, 200, alignof(std::max_align_t), "test_leak_2");
    
    // Only deallocate one
    leak_detector_->record_deallocation(ptr1);
    std::free(ptr1);
    
    // Generate leak report
    auto report = leak_detector_->generate_leak_report();
    
    EXPECT_EQ(report.leaked_allocation_count, 1);
    EXPECT_EQ(report.total_leaked_bytes, 200);
    EXPECT_EQ(report.leaks.size(), 1);
    EXPECT_EQ(report.leaks[0].address, ptr2);
    EXPECT_EQ(report.leaks[0].size, 200);
    EXPECT_EQ(report.leaks[0].tag, "test_leak_2");
    
    // Cleanup
    std::free(ptr2);
}

TEST_F(MemoryTrackingTest, DoubleFreDetection) {
    void* ptr = std::malloc(100);
    
    leak_detector_->record_allocation(ptr, 100, alignof(std::max_align_t));
    leak_detector_->record_deallocation(ptr);
    leak_detector_->record_deallocation(ptr); // Double free
    
    auto report = leak_detector_->generate_corruption_report();
    
    EXPECT_GT(report.double_frees, 0);
    
    std::free(ptr);
}

TEST_F(MemoryTrackingTest, AllocationStatistics) {
    const int NUM_ALLOCS = 100;
    std::vector<void*> ptrs;
    
    for (int i = 0; i < NUM_ALLOCS; ++i) {
        void* ptr = std::malloc(i * 10 + 16); // Variable sizes
        ptrs.push_back(ptr);
        leak_detector_->record_allocation(ptr, i * 10 + 16, alignof(std::max_align_t));
    }
    
    auto stats = leak_detector_->get_statistics();
    
    EXPECT_EQ(stats.total_allocations, NUM_ALLOCS);
    EXPECT_EQ(stats.current_allocations, NUM_ALLOCS);
    EXPECT_GT(stats.total_bytes_allocated, 0);
    EXPECT_GT(stats.average_allocation_size, 16);
    
    // Deallocate half
    for (int i = 0; i < NUM_ALLOCS / 2; ++i) {
        leak_detector_->record_deallocation(ptrs[i]);
        std::free(ptrs[i]);
    }
    
    stats = leak_detector_->get_statistics();
    EXPECT_EQ(stats.total_allocations, NUM_ALLOCS);
    EXPECT_EQ(stats.total_deallocations, NUM_ALLOCS / 2);
    EXPECT_EQ(stats.current_allocations, NUM_ALLOCS / 2);
    
    // Cleanup remaining
    for (int i = NUM_ALLOCS / 2; i < NUM_ALLOCS; ++i) {
        std::free(ptrs[i]);
    }
}

// ==== SIMD OPERATIONS TESTS ====
class SIMDTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Allocate aligned test buffers
        src_buffer_ = std::aligned_alloc(32, BUFFER_SIZE);
        dst_buffer_ = std::aligned_alloc(32, BUFFER_SIZE);
        ref_buffer_ = std::aligned_alloc(32, BUFFER_SIZE);
        
        // Fill source buffer with test data
        auto* src = static_cast<uint8_t*>(src_buffer_);
        for (size_t i = 0; i < BUFFER_SIZE; ++i) {
            src[i] = static_cast<uint8_t>(i & 0xFF);
        }
    }
    
    void TearDown() override {
        std::free(src_buffer_);
        std::free(dst_buffer_);
        std::free(ref_buffer_);
    }
    
    static constexpr size_t BUFFER_SIZE = 64 * 1024; // 64KB
    void* src_buffer_;
    void* dst_buffer_;
    void* ref_buffer_;
};

TEST_F(SIMDTest, FastCopyCorrectness) {
    // Reference copy using memcpy
    std::memcpy(ref_buffer_, src_buffer_, BUFFER_SIZE);
    
    // SIMD copy
    SIMDMemoryOps::fast_copy(dst_buffer_, src_buffer_, BUFFER_SIZE);
    
    // Compare results
    EXPECT_EQ(std::memcmp(dst_buffer_, ref_buffer_, BUFFER_SIZE), 0);
}

TEST_F(SIMDTest, FastSetCorrectness) {
    constexpr uint8_t PATTERN = 0xAB;
    
    // Reference set using memset
    std::memset(ref_buffer_, PATTERN, BUFFER_SIZE);
    
    // SIMD set
    SIMDMemoryOps::fast_set(dst_buffer_, PATTERN, BUFFER_SIZE);
    
    // Compare results
    EXPECT_EQ(std::memcmp(dst_buffer_, ref_buffer_, BUFFER_SIZE), 0);
}

TEST_F(SIMDTest, FastCompareCorrectness) {
    // Make two identical buffers
    SIMDMemoryOps::fast_copy(dst_buffer_, src_buffer_, BUFFER_SIZE);
    
    // Should be equal
    EXPECT_EQ(SIMDMemoryOps::fast_compare(src_buffer_, dst_buffer_, BUFFER_SIZE), 0);
    EXPECT_EQ(std::memcmp(src_buffer_, dst_buffer_, BUFFER_SIZE), 0);
    
    // Modify one byte
    static_cast<uint8_t*>(dst_buffer_)[BUFFER_SIZE / 2] = 0xFF;
    
    // Should not be equal
    EXPECT_NE(SIMDMemoryOps::fast_compare(src_buffer_, dst_buffer_, BUFFER_SIZE), 0);
    EXPECT_NE(std::memcmp(src_buffer_, dst_buffer_, BUFFER_SIZE), 0);
}

// ==== MEMORY MANAGER INTEGRATION TESTS ====
class MemoryManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize memory manager with test configuration
        MemoryPolicy policy;
        policy.enable_tracking = true;
        policy.enable_leak_detection = true;
        policy.enable_stack_traces = false; // Disable for speed
        
        MemoryManager::instance().initialize(policy);
    }
    
    void TearDown() override {
        MemoryManager::instance().reset_statistics();
    }
};

TEST_F(MemoryManagerTest, BasicAllocationStrategies) {
    auto& manager = MemoryManager::instance();
    
    // Test different allocation strategies
    std::vector<AllocationStrategy> strategies = {
        AllocationStrategy::FASTEST,
        AllocationStrategy::MOST_EFFICIENT,
        AllocationStrategy::BALANCED,
        AllocationStrategy::NUMA_AWARE,
        AllocationStrategy::THREAD_LOCAL,
        AllocationStrategy::SIZE_SEGREGATED
    };
    
    for (auto strategy : strategies) {
        MemoryPolicy policy;
        policy.strategy = strategy;
        policy.allocation_tag = "strategy_test";
        
        void* ptr = manager.allocate(1024, policy);
        ASSERT_NE(ptr, nullptr) << "Strategy " << static_cast<int>(strategy) << " failed";
        
        manager.deallocate(ptr, 1024, policy);
    }
}

TEST_F(MemoryManagerTest, TypedAllocations) {
    struct TestStruct {
        int a = 42;
        double b = 3.14;
        std::string c = "test";
        
        TestStruct(int x, double y, const std::string& z) : a(x), b(y), c(z) {}
    };
    
    auto& manager = MemoryManager::instance();
    
    // Test object allocation with constructor arguments
    TestStruct* obj = manager.allocate_object<TestStruct>(MemoryPolicy{}, 100, 2.71, "constructed");
    ASSERT_NE(obj, nullptr);
    EXPECT_EQ(obj->a, 100);
    EXPECT_EQ(obj->b, 2.71);
    EXPECT_EQ(obj->c, "constructed");
    
    manager.deallocate_object(obj);
    
    // Test array allocation
    int* array = manager.allocate_array<int>(1000);
    ASSERT_NE(array, nullptr);
    
    // Use the array
    for (int i = 0; i < 1000; ++i) {
        array[i] = i;
    }
    
    manager.deallocate_array(array, 1000);
}

TEST_F(MemoryManagerTest, MemoryUtilities) {
    auto& manager = MemoryManager::instance();
    
    constexpr size_t SIZE = 1024;
    void* src = manager.allocate(SIZE);
    void* dst = manager.allocate(SIZE);
    
    ASSERT_NE(src, nullptr);
    ASSERT_NE(dst, nullptr);
    
    // Fill source with pattern
    manager.set_memory(src, 0xAB, SIZE);
    
    // Copy to destination
    manager.copy_memory(dst, src, SIZE);
    
    // Verify copy
    EXPECT_EQ(manager.compare_memory(src, dst, SIZE), 0);
    
    // Zero destination
    manager.zero_memory(dst, SIZE);
    
    // Verify zero
    auto* bytes = static_cast<uint8_t*>(dst);
    for (size_t i = 0; i < SIZE; ++i) {
        EXPECT_EQ(bytes[i], 0);
    }
    
    manager.deallocate(src, SIZE);
    manager.deallocate(dst, SIZE);
}

TEST_F(MemoryManagerTest, PerformanceMetrics) {
    auto& manager = MemoryManager::instance();
    
    // Perform some allocations
    std::vector<void*> ptrs;
    for (int i = 0; i < 100; ++i) {
        void* ptr = manager.allocate(i * 10 + 16);
        if (ptr) ptrs.push_back(ptr);
    }
    
    // Get metrics
    auto metrics = manager.get_performance_metrics();
    
    EXPECT_GT(metrics.total_allocations, 0);
    EXPECT_GT(metrics.current_allocated_bytes, 0);
    EXPECT_GE(metrics.memory_efficiency, 0.0);
    EXPECT_LE(metrics.memory_efficiency, 1.0);
    
    // Cleanup
    for (void* ptr : ptrs) {
        manager.deallocate(ptr);
    }
}

TEST_F(MemoryManagerTest, HealthReport) {
    auto& manager = MemoryManager::instance();
    
    // Create some allocations without freeing (simulated leaks)
    void* leak1 = manager.allocate(100);
    void* leak2 = manager.allocate(200);
    
    ASSERT_NE(leak1, nullptr);
    ASSERT_NE(leak2, nullptr);
    
    // Generate health report
    auto report = manager.generate_health_report();
    
    // Should detect leaks (if tracking is enabled)
    if (manager.get_default_policy().enable_leak_detection) {
        EXPECT_TRUE(report.has_memory_leaks);
        EXPECT_GT(report.leaked_bytes, 0);
        EXPECT_GT(report.leaked_allocations, 0);
    }
    
    // Cleanup leaks
    manager.deallocate(leak1, 100);
    manager.deallocate(leak2, 200);
}

// ==== CONVENIENCE API TESTS ====
TEST_F(MemoryManagerTest, ConvenienceAPI) {
    struct TestClass {
        int value;
        TestClass(int v) : value(v) {}
    };
    
    // Test unique_memory_ptr
    auto ptr = make_unique_memory_ptr<TestClass>(42);
    ASSERT_TRUE(ptr);
    EXPECT_EQ(ptr->value, 42);
    
    // Test array allocation
    auto array = make_array_memory<int>(100);
    ASSERT_NE(array, nullptr);
    
    // Use array
    for (int i = 0; i < 100; ++i) {
        array[i] = i;
    }
    
    free_array_memory(array, 100);
    
    // ptr will automatically clean up when going out of scope
}

// ==== BENCHMARK HELPER ====
class BenchmarkTimer {
public:
    void start() {
        start_time_ = std::chrono::high_resolution_clock::now();
    }
    
    double stop() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
            end_time - start_time_);
        return duration.count();
    }

private:
    std::chrono::high_resolution_clock::time_point start_time_;
};

// ==== PERFORMANCE BENCHMARKS ====
class MemoryBenchmark : public ::testing::Test {
protected:
    void SetUp() override {
        MemoryManager::instance().initialize();
    }
    
    static constexpr int BENCHMARK_ITERATIONS = 10000;
};

TEST_F(MemoryBenchmark, AllocationSpeed) {
    auto& manager = MemoryManager::instance();
    BenchmarkTimer timer;
    
    // Benchmark different strategies
    std::vector<std::pair<AllocationStrategy, std::string>> strategies = {
        {AllocationStrategy::FASTEST, "FASTEST"},
        {AllocationStrategy::MOST_EFFICIENT, "MOST_EFFICIENT"},
        {AllocationStrategy::BALANCED, "BALANCED"},
        {AllocationStrategy::SIZE_SEGREGATED, "SIZE_SEGREGATED"}
    };
    
    for (auto& [strategy, name] : strategies) {
        MemoryPolicy policy;
        policy.strategy = strategy;
        policy.enable_tracking = false; // Disable for pure speed test
        
        std::vector<void*> ptrs;
        ptrs.reserve(BENCHMARK_ITERATIONS);
        
        timer.start();
        for (int i = 0; i < BENCHMARK_ITERATIONS; ++i) {
            void* ptr = manager.allocate(64, policy); // 64-byte allocations
            if (ptr) ptrs.push_back(ptr);
        }
        double alloc_time = timer.stop();
        
        timer.start();
        for (void* ptr : ptrs) {
            manager.deallocate(ptr, 64, policy);
        }
        double dealloc_time = timer.stop();
        
        double avg_alloc_time = alloc_time / BENCHMARK_ITERATIONS;
        double avg_dealloc_time = dealloc_time / BENCHMARK_ITERATIONS;
        
        std::cout << "Strategy " << name << ":\n";
        std::cout << "  Average allocation time: " << avg_alloc_time << " ns\n";
        std::cout << "  Average deallocation time: " << avg_dealloc_time << " ns\n";
        std::cout << "  Successful allocations: " << ptrs.size() << "/" << BENCHMARK_ITERATIONS << "\n\n";
        
        // Performance targets (these are aspirational and may need adjustment)
        if (strategy == AllocationStrategy::FASTEST) {
            EXPECT_LT(avg_alloc_time, 50.0); // < 50ns per allocation
        }
    }
}

TEST_F(MemoryBenchmark, SIMDOperationSpeed) {
    constexpr size_t BUFFER_SIZE = 1024 * 1024; // 1MB
    constexpr int ITERATIONS = 100;
    
    void* src = std::aligned_alloc(32, BUFFER_SIZE);
    void* dst = std::aligned_alloc(32, BUFFER_SIZE);
    void* ref = std::aligned_alloc(32, BUFFER_SIZE);
    
    ASSERT_NE(src, nullptr);
    ASSERT_NE(dst, nullptr);
    ASSERT_NE(ref, nullptr);
    
    // Fill source with data
    std::memset(src, 0xAB, BUFFER_SIZE);
    
    BenchmarkTimer timer;
    
    // Benchmark memcpy
    timer.start();
    for (int i = 0; i < ITERATIONS; ++i) {
        std::memcpy(ref, src, BUFFER_SIZE);
    }
    double memcpy_time = timer.stop();
    
    // Benchmark SIMD copy
    timer.start();
    for (int i = 0; i < ITERATIONS; ++i) {
        SIMDMemoryOps::fast_copy(dst, src, BUFFER_SIZE);
    }
    double simd_time = timer.stop();
    
    double memcpy_bandwidth = (BUFFER_SIZE * ITERATIONS * 2.0) / (memcpy_time / 1e9) / (1024 * 1024 * 1024);
    double simd_bandwidth = (BUFFER_SIZE * ITERATIONS * 2.0) / (simd_time / 1e9) / (1024 * 1024 * 1024);
    
    std::cout << "Memory Copy Benchmark (1MB x " << ITERATIONS << " iterations):\n";
    std::cout << "  memcpy bandwidth: " << memcpy_bandwidth << " GB/s\n";
    std::cout << "  SIMD bandwidth: " << simd_bandwidth << " GB/s\n";
    std::cout << "  SIMD speedup: " << (simd_bandwidth / memcpy_bandwidth) << "x\n\n";
    
    // SIMD should be at least as fast as memcpy
    EXPECT_GE(simd_bandwidth, memcpy_bandwidth * 0.95);
    
    std::free(src);
    std::free(dst);
    std::free(ref);
}

// Main test runner
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    std::cout << "Running ECScope Memory Management System Tests\n";
    std::cout << "==============================================\n\n";
    
    // Print system information
    std::cout << "System Information:\n";
    std::cout << "  Hardware threads: " << std::thread::hardware_concurrency() << "\n";
    std::cout << "  Cache line size: " << get_cache_line_size() << " bytes\n";
    
    if (SIMDMemoryOps::has_sse2()) std::cout << "  SSE2 support: Yes\n";
    if (SIMDMemoryOps::has_avx2()) std::cout << "  AVX2 support: Yes\n";
    if (SIMDMemoryOps::has_avx512()) std::cout << "  AVX512 support: Yes\n";
    
    auto& topology = NUMATopology::instance();
    std::cout << "  NUMA nodes: " << topology.get_num_nodes() << "\n";
    if (topology.is_numa_available()) {
        std::cout << "  NUMA available: Yes\n";
    }
    
    std::cout << "\n";
    
    return RUN_ALL_TESTS();
}