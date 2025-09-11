#include <ecscope/testing/test_framework.hpp>
#include <ecscope/testing/ecs_testing.hpp>
#include <ecscope/testing/memory_testing.hpp>
#include <ecscope/testing/physics_testing.hpp>
#include <ecscope/world.hpp>
#include <thread>
#include <random>

using namespace ecscope::testing;

// Massive entity creation stress test
class MassiveEntityCreationStressTest : public TestCase {
public:
    MassiveEntityCreationStressTest() : TestCase("Massive Entity Creation Stress Test", TestCategory::STRESS) {
        context_.timeout_seconds = 300; // 5 minutes
        context_.tags.push_back("memory");
        context_.tags.push_back("ecs");
    }

    void run() override {
        auto world = std::make_unique<ecscope::World>();
        
        constexpr size_t target_entities = 1000000; // 1 million entities
        std::vector<Entity> entities;
        entities.reserve(target_entities);
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Create entities in batches to monitor progress
        constexpr size_t batch_size = 10000;
        for (size_t batch = 0; batch < target_entities / batch_size; ++batch) {
            for (size_t i = 0; i < batch_size; ++i) {
                entities.push_back(world->create_entity());
            }
            
            // Check for memory issues every batch
            if (batch % 10 == 0) {
                auto memory_usage = memory_tracker_.get_metrics().current_usage;
                ASSERT_LT(memory_usage, 1000 * 1024 * 1024); // Less than 1GB
                
                // Test that we can still create entities efficiently
                auto batch_start = std::chrono::high_resolution_clock::now();
                for (size_t i = 0; i < 1000; ++i) {
                    world->create_entity();
                }
                auto batch_end = std::chrono::high_resolution_clock::now();
                auto batch_time = std::chrono::duration_cast<std::chrono::microseconds>(batch_end - batch_start);
                
                // Should still be able to create 1000 entities in under 10ms
                ASSERT_LT(batch_time.count(), 10000);
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        // Verify we created the target number
        ASSERT_EQ(entities.size(), target_entities);
        
        // Test world operations still work
        ASSERT_TRUE(world->is_valid());
        
        // Performance should still be reasonable (less than 30 seconds for 1M entities)
        ASSERT_LT(total_time.count(), 30000);
        
        std::cout << "Created " << target_entities << " entities in " << total_time.count() << "ms" << std::endl;
    }
};

// Concurrent access stress test
class ConcurrentAccessStressTest : public TestCase {
public:
    ConcurrentAccessStressTest() : TestCase("Concurrent Access Stress Test", TestCategory::STRESS) {
        context_.timeout_seconds = 180; // 3 minutes
        context_.tags.push_back("multithreaded");
        context_.tags.push_back("ecs");
        context_.parallel_unsafe(); // This test manages its own threading
    }

    void run() override {
        auto world = std::make_unique<ecscope::World>();
        
        // Pre-create entities for testing
        std::vector<Entity> entities;
        for (int i = 0; i < 10000; ++i) {
            entities.push_back(world->create_entity());
        }
        
        std::atomic<bool> stop_flag{false};
        std::atomic<int> operations_performed{0};
        std::atomic<int> errors_detected{0};
        
        constexpr int num_threads = 8;
        std::vector<std::future<void>> futures;
        
        // Reader threads
        for (int t = 0; t < num_threads / 2; ++t) {
            futures.emplace_back(std::async(std::launch::async, [&, t]() {
                std::random_device rd;
                std::mt19937 gen(rd() + t);
                std::uniform_int_distribution<size_t> entity_dist(0, entities.size() - 1);
                
                while (!stop_flag.load()) {
                    try {
                        // Random entity access
                        auto entity = entities[entity_dist(gen)];
                        bool exists = world->has_entity(entity);
                        (void)exists; // Suppress unused variable warning
                        
                        operations_performed.fetch_add(1);
                    } catch (...) {
                        errors_detected.fetch_add(1);
                    }
                    
                    // Small delay to prevent overwhelming the system
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                }
            }));
        }
        
        // Writer threads
        for (int t = num_threads / 2; t < num_threads; ++t) {
            futures.emplace_back(std::async(std::launch::async, [&, t]() {
                std::random_device rd;
                std::mt19937 gen(rd() + t);
                
                while (!stop_flag.load()) {
                    try {
                        // Create and destroy entities
                        auto entity = world->create_entity();
                        
                        // Simulate some work
                        std::this_thread::sleep_for(std::chrono::microseconds(50));
                        
                        world->destroy_entity(entity);
                        
                        operations_performed.fetch_add(1);
                    } catch (...) {
                        errors_detected.fetch_add(1);
                    }
                    
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                }
            }));
        }
        
        // Let threads run for specified duration
        std::this_thread::sleep_for(std::chrono::seconds(30));
        
        // Signal threads to stop
        stop_flag = true;
        
        // Wait for all threads to complete
        for (auto& future : futures) {
            future.wait();
        }
        
        // Verify results
        ASSERT_GT(operations_performed.load(), 1000); // Should have performed many operations
        ASSERT_EQ(errors_detected.load(), 0);         // No errors should occur
        ASSERT_TRUE(world->is_valid());               // World should still be valid
        
        std::cout << "Performed " << operations_performed.load() << " concurrent operations with " 
                  << errors_detected.load() << " errors" << std::endl;
    }
};

// Memory pressure stress test
class MemoryPressureStressTest : public MemoryTestFixture {
public:
    MemoryPressureStressTest() : MemoryTestFixture() {
        context_.name = "Memory Pressure Stress Test";
        context_.category = TestCategory::STRESS;
        context_.timeout_seconds = 240; // 4 minutes
        context_.tags.push_back("memory");
    }

    void run() override {
        MemoryStressTester::StressTestConfig config;
        config.min_allocation_size = 1;
        config.max_allocation_size = 10 * 1024 * 1024; // 10MB max
        config.target_memory_usage = 500 * 1024 * 1024; // 500MB target
        config.test_duration = std::chrono::seconds(120); // 2 minutes
        config.allocation_probability = 0.7; // 70% allocate, 30% deallocate
        config.enable_fragmentation_test = true;
        config.enable_random_access = true;
        
        bool stress_passed = MemoryStressTester::run_stress_test(config);
        ASSERT_TRUE(stress_passed);
        
        // Check final memory state
        auto stats = tracker_->get_statistics();
        std::cout << "Memory stress test completed:" << std::endl;
        std::cout << "  Peak usage: " << stats.peak_usage << " bytes" << std::endl;
        std::cout << "  Total allocations: " << stats.allocation_count << std::endl;
        std::cout << "  Leaked allocations: " << stats.leaked_allocations << std::endl;
        
        // Should have minimal leaks
        ASSERT_LT(stats.leaked_allocations, 10);
    }
};

// Physics simulation stress test
class PhysicsSimulationStressTest : public PhysicsTestFixture {
public:
    PhysicsSimulationStressTest() : PhysicsTestFixture() {
        context_.name = "Physics Simulation Stress Test";
        context_.category = TestCategory::STRESS;
        context_.timeout_seconds = 300; // 5 minutes
        context_.tags.push_back("physics");
    }

    void run() override {
        // Test with many rigid bodies
        constexpr size_t body_count = 10000;
        constexpr int simulation_steps = 3600; // 1 minute at 60 FPS
        
        bool stress_passed = stress_tester_->stress_test_many_bodies(
            *world_, body_count, simulation_steps);
        
        ASSERT_TRUE(stress_passed);
        
        // Verify world is still stable
        ASSERT_TRUE(world_->is_valid());
        
        std::cout << "Physics stress test completed with " << body_count 
                  << " bodies for " << simulation_steps << " steps" << std::endl;
    }
};

// Resource exhaustion stress test
class ResourceExhaustionStressTest : public TestCase {
public:
    ResourceExhaustionStressTest() : TestCase("Resource Exhaustion Stress Test", TestCategory::STRESS) {
        context_.timeout_seconds = 180; // 3 minutes
        context_.tags.push_back("resource");
    }

    void run() override {
        auto world = std::make_unique<ecscope::World>();
        
        // Test file handle exhaustion resilience
        std::vector<std::unique_ptr<std::ofstream>> files;
        
        try {
            // Try to create many temporary files
            for (int i = 0; i < 1000; ++i) {
                std::string filename = "temp_file_" + std::to_string(i) + ".tmp";
                auto file = std::make_unique<std::ofstream>(filename);
                if (file->is_open()) {
                    files.push_back(std::move(file));
                } else {
                    // Hit resource limit, break gracefully
                    break;
                }
            }
            
            // Test that the engine still works under resource pressure
            for (int i = 0; i < 1000; ++i) {
                auto entity = world->create_entity();
                ASSERT_TRUE(world->has_entity(entity));
            }
            
            // Test world operations
            world->update(1.0f / 60.0f);
            ASSERT_TRUE(world->is_valid());
            
        } catch (const std::exception& e) {
            // Resource exhaustion is expected, ensure engine handles it gracefully
            ASSERT_TRUE(world->is_valid());
        }
        
        // Cleanup
        files.clear();
        
        // Remove temporary files
        for (size_t i = 0; i < files.size(); ++i) {
            std::string filename = "temp_file_" + std::to_string(i) + ".tmp";
            std::remove(filename.c_str());
        }
        
        std::cout << "Resource exhaustion test completed, created " << files.size() 
                  << " temporary files" << std::endl;
    }
};

// Long-running stability stress test
class LongRunningStabilityStressTest : public TestCase {
public:
    LongRunningStabilityStressTest() : TestCase("Long Running Stability Stress Test", TestCategory::STRESS) {
        context_.timeout_seconds = 600; // 10 minutes
        context_.tags.push_back("stability");
        context_.tags.push_back("long-running");
    }

    void run() override {
        auto world = std::make_unique<ecscope::World>();
        
        constexpr int total_minutes = 5;
        constexpr int fps = 60;
        constexpr int total_frames = total_minutes * 60 * fps;
        
        std::vector<Entity> entities;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> prob_dist(0.0f, 1.0f);
        
        auto start_time = std::chrono::steady_clock::now();
        
        for (int frame = 0; frame < total_frames; ++frame) {
            // Simulate realistic game loop
            
            // Occasionally create entities
            if (prob_dist(gen) < 0.01f && entities.size() < 50000) { // 1% chance
                entities.push_back(world->create_entity());
            }
            
            // Occasionally destroy entities
            if (prob_dist(gen) < 0.005f && !entities.empty()) { // 0.5% chance
                auto it = entities.begin() + gen() % entities.size();
                world->destroy_entity(*it);
                entities.erase(it);
            }
            
            // Update world
            world->update(1.0f / fps);
            
            // Validate world state periodically
            if (frame % (fps * 30) == 0) { // Every 30 seconds
                ASSERT_TRUE(world->is_valid());
                
                auto current_time = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time);
                
                std::cout << "Stability test running: " << elapsed.count() << "s, " 
                          << entities.size() << " entities" << std::endl;
            }
            
            // Small delay to prevent overwhelming the system
            if (frame % 1000 == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
        
        auto end_time = std::chrono::steady_clock::now();
        auto total_duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
        
        // Final validation
        ASSERT_TRUE(world->is_valid());
        ASSERT_LT(entities.size(), 100000); // Shouldn't have grown too large
        
        std::cout << "Long-running stability test completed in " << total_duration.count() 
                  << " seconds with " << entities.size() << " final entities" << std::endl;
    }
};

// Register stress tests
REGISTER_TEST(MassiveEntityCreationStressTest);
REGISTER_TEST(ConcurrentAccessStressTest);
REGISTER_TEST(MemoryPressureStressTest);
REGISTER_TEST(PhysicsSimulationStressTest);
REGISTER_TEST(ResourceExhaustionStressTest);
REGISTER_TEST(LongRunningStabilityStressTest);