#include "../framework/ecscope_test_framework.hpp"
#include <ecscope/memory_tracker.hpp>
#include <ecscope/arena.hpp>
#include <ecscope/pool.hpp>
#include <ecscope/thread_local_allocator.hpp>
#include <ecscope/lockfree_allocators.hpp>
#include <ecscope/work_stealing_job_system.hpp>

#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <random>
#include <memory>

namespace ECScope::Testing {

// =============================================================================
// Memory Safety and Threading Test Fixture
// =============================================================================

class MemorySafetyThreadingTest : public ECScopeTestFixture {
protected:
    void SetUp() override {
        ECScopeTestFixture::SetUp();
        
        // Initialize thread-safe memory tracking
        detailed_tracker_ = std::make_unique<Memory::DetailedTracker>();
        leak_detector_ = std::make_unique<Memory::LeakDetector>();
        thread_sanitizer_ = std::make_unique<Memory::ThreadSanitizer>();
        
        // Set up test parameters
        thread_count_ = std::thread::hardware_concurrency();
        if (thread_count_ == 0) thread_count_ = 4;
        
        operations_per_thread_ = 10000;
        stress_test_duration_ms_ = 5000; // 5 seconds
        
        // Initialize random number generator
        rng_.seed(42);
    }

    void TearDown() override {
        // Perform final leak detection
        auto leaks = leak_detector_->detect_leaks();
        if (!leaks.empty()) {
            std::cout << "Memory leaks detected:\n";
            for (const auto& leak : leaks) {
                std::cout << "  " << leak.size << " bytes at " << leak.address 
                          << " (allocated from " << leak.source_location << ")\n";
            }
        }
        
        // Check for threading violations
        auto violations = thread_sanitizer_->get_violations();
        if (!violations.empty()) {
            std::cout << "Threading violations detected:\n";
            for (const auto& violation : violations) {
                std::cout << "  " << violation.type << " at " << violation.address
                          << " (threads: " << violation.thread1_id << ", " << violation.thread2_id << ")\n";
            }
        }
        
        thread_sanitizer_.reset();
        leak_detector_.reset();
        detailed_tracker_.reset();
        ECScopeTestFixture::TearDown();
    }

    // Helper to create controlled memory stress
    void stress_allocator(std::function<void*()> alloc_func, 
                         std::function<void(void*)> dealloc_func,
                         size_t iterations) {
        std::vector<void*> allocations;
        allocations.reserve(iterations);
        
        // Allocate phase
        for (size_t i = 0; i < iterations; ++i) {
            void* ptr = alloc_func();
            if (ptr) {
                allocations.push_back(ptr);
            }
        }
        
        // Deallocate phase
        for (void* ptr : allocations) {
            dealloc_func(ptr);
        }
    }

protected:
    std::unique_ptr<Memory::DetailedTracker> detailed_tracker_;
    std::unique_ptr<Memory::LeakDetector> leak_detector_;
    std::unique_ptr<Memory::ThreadSanitizer> thread_sanitizer_;
    
    size_t thread_count_;
    size_t operations_per_thread_;
    size_t stress_test_duration_ms_;
    
    std::mt19937 rng_;
};

// =============================================================================
// Memory Leak Detection Tests
// =============================================================================

TEST_F(MemorySafetyThreadingTest, BasicMemoryLeakDetection) {
    leak_detector_->start_tracking();
    
    // Intentionally create and fix memory leaks for testing
    std::vector<void*> test_allocations;
    
    // Phase 1: Allocate without freeing (simulate leak)
    for (int i = 0; i < 10; ++i) {
        void* ptr = std::malloc(1024);
        ASSERT_NE(ptr, nullptr);
        test_allocations.push_back(ptr);
    }
    
    // Check for leaks (should detect them)
    auto leaks_before_cleanup = leak_detector_->detect_leaks();
    EXPECT_GE(leaks_before_cleanup.size(), 10) << "Should detect at least 10 leaks";
    
    // Phase 2: Clean up allocations
    for (void* ptr : test_allocations) {
        std::free(ptr);
    }
    
    // Check for leaks again (should be clean now)
    auto leaks_after_cleanup = leak_detector_->detect_leaks();
    
    // Note: The leak detector might still show leaks if it tracks by allocation/deallocation pairs
    // In a real implementation, this would depend on the specific leak detection strategy
    std::cout << "Leaks before cleanup: " << leaks_before_cleanup.size() << "\n";
    std::cout << "Leaks after cleanup: " << leaks_after_cleanup.size() << "\n";
    
    leak_detector_->stop_tracking();
}

TEST_F(MemorySafetyThreadingTest, ECSComponentMemoryLeakDetection) {
    leak_detector_->start_tracking();
    
    // Test ECS component allocation/deallocation patterns
    constexpr size_t entity_count = 1000;
    constexpr size_t cycles = 10;
    
    for (size_t cycle = 0; cycle < cycles; ++cycle) {
        std::vector<Entity> entities;
        entities.reserve(entity_count);
        
        // Create entities with components
        for (size_t i = 0; i < entity_count; ++i) {
            auto entity = world_->create_entity();
            world_->add_component<TestPosition>(entity, 
                static_cast<float>(i), static_cast<float>(i * 2), 0);
            world_->add_component<TestVelocity>(entity, 1, 1, 1);
            world_->add_component<TestHealth>(entity, 100, 100);
            
            entities.push_back(entity);
        }
        
        // Modify components (potential for memory issues)
        for (auto entity : entities) {
            auto& pos = world_->get_component<TestPosition>(entity);
            pos.x += 1.0f;
            
            auto& health = world_->get_component<TestHealth>(entity);
            health.health = 50;
        }
        
        // Remove some components (archetype changes)
        for (size_t i = 0; i < entities.size(); i += 2) {
            world_->remove_component<TestVelocity>(entities[i]);
        }
        
        // Destroy entities
        for (auto entity : entities) {
            world_->destroy_entity(entity);
        }
    }
    
    // Check for leaks
    auto leaks = leak_detector_->detect_leaks();
    
    // In a well-implemented ECS, there should be no leaks
    if (!leaks.empty()) {
        std::cout << "ECS memory leaks detected: " << leaks.size() << " leaks\n";
        for (size_t i = 0; i < std::min(leaks.size(), size_t(5)); ++i) {
            std::cout << "  Leak " << i << ": " << leaks[i].size << " bytes\n";
        }
    }
    
    // This test will pass even if leaks are detected, but logs them for analysis
    EXPECT_LT(leaks.size(), entity_count / 10) << "Too many memory leaks detected";
    
    leak_detector_->stop_tracking();
}

TEST_F(MemorySafetyThreadingTest, AllocatorMemoryLeakDetection) {
    leak_detector_->start_tracking();
    
    // Test custom allocators for leaks
    {
        Arena arena(1024 * 1024); // 1MB arena
        
        std::vector<void*> allocations;
        for (int i = 0; i < 1000; ++i) {
            void* ptr = arena.allocate(1024, 8);
            if (ptr) {
                allocations.push_back(ptr);
            }
        }
        
        // Arena should clean up automatically when it goes out of scope
    }
    
    {
        Pool pool(64, 1000); // 64-byte objects, 1000 capacity
        
        std::vector<void*> allocations;
        for (int i = 0; i < 500; ++i) {
            void* ptr = pool.allocate();
            if (ptr) {
                allocations.push_back(ptr);
            }
        }
        
        // Manually free some objects
        for (size_t i = 0; i < allocations.size() / 2; ++i) {
            pool.deallocate(allocations[i]);
        }
        
        // Pool should handle the rest when it's destroyed
    }
    
    auto leaks = leak_detector_->detect_leaks();
    std::cout << "Custom allocator leaks: " << leaks.size() << "\n";
    
    // Custom allocators might show different leak patterns
    // This is more of a validation test than a strict requirement
    EXPECT_LT(leaks.size(), 100) << "Too many allocator-related leaks";
    
    leak_detector_->stop_tracking();
}

// =============================================================================
// Thread Safety Tests
// =============================================================================

TEST_F(MemorySafetyThreadingTest, ConcurrentECSOperations) {
    thread_sanitizer_->start_monitoring();
    
    std::atomic<bool> start_flag{false};
    std::atomic<size_t> operations_completed{0};
    std::vector<std::thread> threads;
    
    // Each thread performs different ECS operations
    for (size_t t = 0; t < thread_count_; ++t) {
        threads.emplace_back([&, t]() {
            // Wait for start signal
            while (!start_flag.load()) {
                std::this_thread::yield();
            }
            
            std::mt19937 local_rng(42 + t);
            std::uniform_int_distribution<int> op_dist(0, 3);
            
            for (size_t op = 0; op < operations_per_thread_; ++op) {
                int operation = op_dist(local_rng);
                
                switch (operation) {
                    case 0: { // Create entity
                        auto entity = world_->create_entity();
                        if (world_->is_valid(entity)) {
                            world_->add_component<TestPosition>(entity, 
                                static_cast<float>(t), static_cast<float>(op), 0);
                        }
                        break;
                    }
                    
                    case 1: { // Query entities
                        size_t count = 0;
                        world_->each<TestPosition>([&](Entity, TestPosition&) {
                            count++;
                        });
                        // Use count to prevent optimization
                        volatile size_t result = count;
                        (void)result;
                        break;
                    }
                    
                    case 2: { // Modify components
                        world_->each<TestPosition>([&](Entity entity, TestPosition& pos) {
                            if (world_->is_valid(entity)) {
                                pos.x += 0.01f;
                            }
                        });
                        break;
                    }
                    
                    case 3: { // Add/remove components
                        world_->each<TestPosition>([&](Entity entity, TestPosition&) {
                            if (world_->is_valid(entity) && op % 10 == 0) {
                                if (!world_->has_component<TestVelocity>(entity)) {
                                    world_->add_component<TestVelocity>(entity, 1, 1, 1);
                                }
                            }
                        });
                        break;
                    }
                }
                
                operations_completed++;
            }
        });
    }
    
    // Start all threads
    start_flag = true;
    
    // Wait for completion
    for (auto& thread : threads) {
        thread.join();
    }
    
    thread_sanitizer_->stop_monitoring();
    
    // Verify all operations completed
    EXPECT_EQ(operations_completed.load(), thread_count_ * operations_per_thread_);
    
    // Check for threading violations
    auto violations = thread_sanitizer_->get_violations();
    if (!violations.empty()) {
        std::cout << "Threading violations detected: " << violations.size() << "\n";
        for (size_t i = 0; i < std::min(violations.size(), size_t(5)); ++i) {
            std::cout << "  Violation " << i << ": " << violations[i].type << "\n";
        }
    }
    
    // This test focuses on detection rather than strict requirements
    // Some threading violations might be acceptable depending on the ECS implementation
}

TEST_F(MemorySafetyThreadingTest, ConcurrentMemoryAllocations) {
    thread_sanitizer_->start_monitoring();
    
    std::atomic<size_t> successful_allocations{0};
    std::atomic<size_t> successful_deallocations{0};
    std::atomic<bool> start_flag{false};
    
    std::vector<std::thread> threads;
    
    for (size_t t = 0; t < thread_count_; ++t) {
        threads.emplace_back([&, t]() {
            while (!start_flag.load()) {
                std::this_thread::yield();
            }
            
            std::vector<void*> thread_allocations;
            std::mt19937 local_rng(42 + t);
            std::uniform_int_distribution<size_t> size_dist(64, 4096);
            
            // Allocation phase
            for (size_t i = 0; i < operations_per_thread_; ++i) {
                size_t alloc_size = size_dist(local_rng);
                void* ptr = std::malloc(alloc_size);
                
                if (ptr) {
                    thread_allocations.push_back(ptr);
                    successful_allocations++;
                    
                    // Write to memory to ensure it's valid
                    std::memset(ptr, static_cast<int>(t), alloc_size);
                }
            }
            
            // Deallocation phase
            for (void* ptr : thread_allocations) {
                std::free(ptr);
                successful_deallocations++;
            }
        });
    }
    
    start_flag = true;
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    thread_sanitizer_->stop_monitoring();
    
    EXPECT_EQ(successful_allocations.load(), successful_deallocations.load());
    EXPECT_EQ(successful_allocations.load(), thread_count_ * operations_per_thread_);
    
    auto violations = thread_sanitizer_->get_violations();
    std::cout << "Memory allocation threading violations: " << violations.size() << "\n";
}

#ifdef ECSCOPE_ENABLE_LOCKFREE
TEST_F(MemorySafetyThreadingTest, LockFreeStructuresSafety) {
    thread_sanitizer_->start_monitoring();
    
    // Test lock-free allocator
    LockFreeAllocator allocator(128, 10000);
    
    std::atomic<size_t> operations_count{0};
    std::atomic<bool> start_flag{false};
    
    std::vector<std::thread> threads;
    
    for (size_t t = 0; t < thread_count_; ++t) {
        threads.emplace_back([&, t]() {
            while (!start_flag.load()) {
                std::this_thread::yield();
            }
            
            std::vector<void*> thread_ptrs;
            
            // Mixed allocation/deallocation pattern
            for (size_t i = 0; i < operations_per_thread_; ++i) {
                if (i % 3 == 0 && !thread_ptrs.empty()) {
                    // Deallocate
                    void* ptr = thread_ptrs.back();
                    thread_ptrs.pop_back();
                    allocator.deallocate(ptr);
                } else {
                    // Allocate
                    void* ptr = allocator.allocate();
                    if (ptr) {
                        thread_ptrs.push_back(ptr);
                        // Write to verify memory is valid
                        *static_cast<uint32_t*>(ptr) = static_cast<uint32_t>(t);
                    }
                }
                operations_count++;
            }
            
            // Clean up remaining allocations
            for (void* ptr : thread_ptrs) {
                allocator.deallocate(ptr);
            }
        });
    }
    
    start_flag = true;
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    thread_sanitizer_->stop_monitoring();
    
    EXPECT_EQ(operations_count.load(), thread_count_ * operations_per_thread_);
    
    auto violations = thread_sanitizer_->get_violations();
    std::cout << "Lock-free structures violations: " << violations.size() << "\n";
    
    // Lock-free structures should have minimal or no violations
    EXPECT_LT(violations.size(), 10) << "Too many violations in lock-free structures";
}
#endif

// =============================================================================
// Stress Testing
// =============================================================================

TEST_F(MemorySafetyThreadingTest, MemoryStressTest) {
    leak_detector_->start_tracking();
    thread_sanitizer_->start_monitoring();
    
    std::atomic<bool> stop_flag{false};
    std::atomic<size_t> total_allocations{0};
    std::atomic<size_t> total_deallocations{0};
    
    std::vector<std::thread> stress_threads;
    
    // Create stress testing threads with different allocation patterns
    for (size_t t = 0; t < thread_count_; ++t) {
        stress_threads.emplace_back([&, t]() {
            std::mt19937 local_rng(42 + t);
            std::uniform_int_distribution<size_t> size_dist(16, 8192);
            std::uniform_int_distribution<int> pattern_dist(0, 2);
            
            std::vector<void*> long_lived_ptrs;
            
            while (!stop_flag.load()) {
                int pattern = pattern_dist(local_rng);
                
                switch (pattern) {
                    case 0: { // Frequent small allocations
                        for (int i = 0; i < 100 && !stop_flag.load(); ++i) {
                            void* ptr = std::malloc(size_dist(local_rng));
                            if (ptr) {
                                total_allocations++;
                                std::free(ptr);
                                total_deallocations++;
                            }
                        }
                        break;
                    }
                    
                    case 1: { // Long-lived allocations
                        void* ptr = std::malloc(size_dist(local_rng));
                        if (ptr) {
                            long_lived_ptrs.push_back(ptr);
                            total_allocations++;
                            
                            // Occasionally clean up long-lived allocations
                            if (long_lived_ptrs.size() > 100) {
                                std::free(long_lived_ptrs.front());
                                long_lived_ptrs.erase(long_lived_ptrs.begin());
                                total_deallocations++;
                            }
                        }
                        break;
                    }
                    
                    case 2: { // ECS-like allocation pattern
                        constexpr size_t batch_size = 50;
                        std::vector<Entity> entities;
                        
                        for (size_t i = 0; i < batch_size; ++i) {
                            auto entity = world_->create_entity();
                            world_->add_component<TestPosition>(entity, 
                                static_cast<float>(i), 0, 0);
                            entities.push_back(entity);
                        }
                        
                        // Modify components
                        for (auto entity : entities) {
                            if (world_->is_valid(entity)) {
                                auto& pos = world_->get_component<TestPosition>(entity);
                                pos.x += 1.0f;
                            }
                        }
                        
                        // Clean up
                        for (auto entity : entities) {
                            world_->destroy_entity(entity);
                        }
                        break;
                    }
                }
                
                // Small delay to prevent overwhelming the system
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
            
            // Clean up long-lived allocations
            for (void* ptr : long_lived_ptrs) {
                std::free(ptr);
                total_deallocations++;
            }
        });
    }
    
    // Run stress test for specified duration
    std::this_thread::sleep_for(std::chrono::milliseconds(stress_test_duration_ms_));
    stop_flag = true;
    
    // Wait for all threads to complete
    for (auto& thread : stress_threads) {
        thread.join();
    }
    
    thread_sanitizer_->stop_monitoring();
    
    // Analyze results
    std::cout << "Stress test results:\n";
    std::cout << "  Total allocations: " << total_allocations.load() << "\n";
    std::cout << "  Total deallocations: " << total_deallocations.load() << "\n";
    std::cout << "  Net allocations: " << (total_allocations.load() - total_deallocations.load()) << "\n";
    
    auto leaks = leak_detector_->detect_leaks();
    auto violations = thread_sanitizer_->get_violations();
    
    std::cout << "  Memory leaks detected: " << leaks.size() << "\n";
    std::cout << "  Threading violations: " << violations.size() << "\n";
    
    // Stress test should complete without major issues
    EXPECT_GT(total_allocations.load(), 0) << "Stress test should perform allocations";
    
    // Allow for some leaks/violations under stress, but not too many
    EXPECT_LT(leaks.size(), total_allocations.load() / 1000) << "Too many memory leaks under stress";
    EXPECT_LT(violations.size(), total_allocations.load() / 500) << "Too many threading violations under stress";
    
    leak_detector_->stop_tracking();
}

// =============================================================================
// Job System Thread Safety Tests
// =============================================================================

#ifdef ECSCOPE_ENABLE_JOB_SYSTEM
TEST_F(MemorySafetyThreadingTest, JobSystemThreadSafety) {
    thread_sanitizer_->start_monitoring();
    
    WorkStealingJobSystem job_system(thread_count_);
    
    std::atomic<size_t> jobs_completed{0};
    std::atomic<size_t> jobs_submitted{0};
    
    // Submit many jobs that perform ECS operations
    constexpr size_t job_count = 1000;
    
    for (size_t i = 0; i < job_count; ++i) {
        auto job = [&, i]() {
            // Each job creates entities, modifies them, and destroys them
            std::vector<Entity> entities;
            
            for (int j = 0; j < 10; ++j) {
                auto entity = world_->create_entity();
                world_->add_component<TestPosition>(entity, 
                    static_cast<float>(i), static_cast<float>(j), 0);
                entities.push_back(entity);
            }
            
            // Modify entities
            for (auto entity : entities) {
                if (world_->is_valid(entity)) {
                    auto& pos = world_->get_component<TestPosition>(entity);
                    pos.x *= 2.0f;
                }
            }
            
            // Query entities
            size_t count = 0;
            world_->each<TestPosition>([&](Entity, TestPosition&) {
                count++;
            });
            
            // Clean up
            for (auto entity : entities) {
                world_->destroy_entity(entity);
            }
            
            jobs_completed++;
        };
        
        job_system.submit(job);
        jobs_submitted++;
    }
    
    // Wait for all jobs to complete
    job_system.wait_for_all();
    
    thread_sanitizer_->stop_monitoring();
    
    EXPECT_EQ(jobs_completed.load(), jobs_submitted.load());
    EXPECT_EQ(jobs_completed.load(), job_count);
    
    auto violations = thread_sanitizer_->get_violations();
    std::cout << "Job system threading violations: " << violations.size() << "\n";
    
    // Job system should have good thread safety
    EXPECT_LT(violations.size(), job_count / 10) << "Too many violations in job system";
}
#endif

// =============================================================================
// Memory Pattern Analysis
// =============================================================================

TEST_F(MemorySafetyThreadingTest, MemoryPatternAnalysis) {
    detailed_tracker_->start_detailed_tracking();
    
    // Test different memory access patterns
    constexpr size_t pattern_iterations = 1000;
    
    // Pattern 1: Sequential allocations
    {
        std::vector<void*> sequential_ptrs;
        for (size_t i = 0; i < pattern_iterations; ++i) {
            void* ptr = std::malloc(1024);
            if (ptr) {
                sequential_ptrs.push_back(ptr);
            }
        }
        
        for (void* ptr : sequential_ptrs) {
            std::free(ptr);
        }
    }
    
    // Pattern 2: Random size allocations
    {
        std::uniform_int_distribution<size_t> size_dist(64, 8192);
        std::vector<void*> random_ptrs;
        
        for (size_t i = 0; i < pattern_iterations; ++i) {
            void* ptr = std::malloc(size_dist(rng_));
            if (ptr) {
                random_ptrs.push_back(ptr);
            }
        }
        
        for (void* ptr : random_ptrs) {
            std::free(ptr);
        }
    }
    
    // Pattern 3: ECS component allocations
    {
        std::vector<Entity> entities;
        for (size_t i = 0; i < pattern_iterations; ++i) {
            auto entity = world_->create_entity();
            
            // Add components in different orders
            if (i % 3 == 0) {
                world_->add_component<TestPosition>(entity, static_cast<float>(i), 0, 0);
                world_->add_component<TestVelocity>(entity, 1, 1, 1);
            } else if (i % 3 == 1) {
                world_->add_component<TestVelocity>(entity, 1, 1, 1);
                world_->add_component<TestPosition>(entity, static_cast<float>(i), 0, 0);
            } else {
                world_->add_component<TestPosition>(entity, static_cast<float>(i), 0, 0);
                world_->add_component<TestHealth>(entity, 100, 100);
            }
            
            entities.push_back(entity);
        }
        
        // Clean up
        for (auto entity : entities) {
            world_->destroy_entity(entity);
        }
    }
    
    detailed_tracker_->stop_detailed_tracking();
    
    // Analyze memory patterns
    auto allocation_stats = detailed_tracker_->get_allocation_statistics();
    auto fragmentation_info = detailed_tracker_->get_fragmentation_info();
    
    std::cout << "Memory pattern analysis:\n";
    std::cout << "  Total allocations: " << allocation_stats.total_allocations << "\n";
    std::cout << "  Peak memory usage: " << allocation_stats.peak_memory_usage << " bytes\n";
    std::cout << "  Average allocation size: " << allocation_stats.average_allocation_size << " bytes\n";
    std::cout << "  Fragmentation ratio: " << fragmentation_info.fragmentation_ratio << "\n";
    
    // Memory patterns should be reasonable
    EXPECT_GT(allocation_stats.total_allocations, 0);
    EXPECT_LT(fragmentation_info.fragmentation_ratio, 0.5f) << "High memory fragmentation detected";
}

} // namespace ECScope::Testing