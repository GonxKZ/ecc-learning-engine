#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// Core ECScope includes
#include <ecscope/component.hpp>
#include <ecscope/registry.hpp>
#include <ecscope/world.hpp>
#include <ecscope/memory/mem_tracker.hpp>
#include <ecscope/benchmarks.hpp>

#ifdef ECSCOPE_ENABLE_PHYSICS
#include <ecscope/world3d.hpp>
#include <ecscope/advanced_physics_complete.hpp>
#endif

#ifdef ECSCOPE_ENABLE_JOB_SYSTEM
#include <ecscope/work_stealing_job_system.hpp>
#include <ecscope/ecs_parallel_scheduler.hpp>
#endif

namespace ECScope::Testing {

// =============================================================================
// Test Framework Configuration
// =============================================================================

constexpr size_t DEFAULT_ENTITY_COUNT = 10000;
constexpr size_t STRESS_ENTITY_COUNT = 100000;
constexpr size_t PERFORMANCE_ENTITY_COUNT = 1000000;
constexpr std::chrono::milliseconds DEFAULT_TIMEOUT{5000};
constexpr double PERFORMANCE_TOLERANCE = 0.15; // 15% tolerance for performance tests

// =============================================================================
// Test Fixtures
// =============================================================================

/**
 * Base test fixture providing common ECScope infrastructure
 */
class ECScopeTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize memory tracking
        memory_tracker_ = std::make_unique<MemoryTracker>("ECScopeTest");
        memory_tracker_->start_tracking();

        // Initialize registry
        registry_ = std::make_unique<Registry>();

        // Initialize world
        world_ = std::make_unique<World>();

        // Record start time
        start_time_ = std::chrono::high_resolution_clock::now();
    }

    void TearDown() override {
        // Clean up world first
        world_.reset();
        registry_.reset();

        // Stop memory tracking and validate
        if (memory_tracker_) {
            memory_tracker_->stop_tracking();
            EXPECT_EQ(memory_tracker_->get_allocation_count(), 
                     memory_tracker_->get_deallocation_count())
                << "Memory leak detected!";
        }

        // Record end time and log duration
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time_);
        
        // Warn if test takes too long
        if (duration > DEFAULT_TIMEOUT) {
            std::cout << "WARNING: Test took " << duration.count() 
                      << "ms (longer than " << DEFAULT_TIMEOUT.count() << "ms)\n";
        }
    }

protected:
    std::unique_ptr<MemoryTracker> memory_tracker_;
    std::unique_ptr<Registry> registry_;
    std::unique_ptr<World> world_;
    std::chrono::high_resolution_clock::time_point start_time_;
};

/**
 * Performance test fixture with benchmarking capabilities
 */
class PerformanceTestFixture : public ECScopeTestFixture {
protected:
    void SetUp() override {
        ECScopeTestFixture::SetUp();
        benchmarker_ = std::make_unique<Benchmarks::Suite>("PerformanceTest");
    }

    void TearDown() override {
        if (benchmarker_) {
            benchmarker_->finalize();
            // Log benchmark results
            auto results = benchmarker_->get_results();
            for (const auto& [name, result] : results) {
                std::cout << "Benchmark " << name << ": " 
                          << result.average_time << "ms avg, "
                          << result.min_time << "ms min, "
                          << result.max_time << "ms max\n";
            }
        }
        ECScopeTestFixture::TearDown();
    }

    void benchmark(const std::string& name, std::function<void()> func, int iterations = 1000) {
        benchmarker_->add_benchmark(name, std::move(func), iterations);
    }

protected:
    std::unique_ptr<Benchmarks::Suite> benchmarker_;
};

#ifdef ECSCOPE_ENABLE_PHYSICS
/**
 * Physics test fixture with 3D world setup
 */
class PhysicsTestFixture : public ECScopeTestFixture {
protected:
    void SetUp() override {
        ECScopeTestFixture::SetUp();
        physics_world_ = std::make_unique<Physics3D::World>();
        physics_world_->set_gravity({0.0f, -9.81f, 0.0f});
    }

    void TearDown() override {
        physics_world_.reset();
        ECScopeTestFixture::TearDown();
    }

    // Helper to create basic physics entities
    Entity create_physics_entity(const Vec3& position = {0, 0, 0}, 
                                 const Vec3& velocity = {0, 0, 0}) {
        auto entity = world_->create_entity();
        world_->add_component<Transform3D>(entity, position);
        world_->add_component<RigidBody3D>(entity, velocity);
        return entity;
    }

protected:
    std::unique_ptr<Physics3D::World> physics_world_;
};
#endif

#ifdef ECSCOPE_ENABLE_JOB_SYSTEM
/**
 * Multithreading test fixture with job system
 */
class ThreadingTestFixture : public ECScopeTestFixture {
protected:
    void SetUp() override {
        ECScopeTestFixture::SetUp();
        job_system_ = std::make_unique<WorkStealingJobSystem>();
        scheduler_ = std::make_unique<ECSParallelScheduler>(*world_);
    }

    void TearDown() override {
        scheduler_.reset();
        job_system_.reset();
        ECScopeTestFixture::TearDown();
    }

protected:
    std::unique_ptr<WorkStealingJobSystem> job_system_;
    std::unique_ptr<ECSParallelScheduler> scheduler_;
};
#endif

// =============================================================================
// Test Component Types for Testing
// =============================================================================

struct TestPosition {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    
    bool operator==(const TestPosition& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

struct TestVelocity {
    float vx = 0.0f, vy = 0.0f, vz = 0.0f;
    
    bool operator==(const TestVelocity& other) const {
        return vx == other.vx && vy == other.vy && vz == other.vz;
    }
};

struct TestHealth {
    int health = 100;
    int max_health = 100;
    
    bool operator==(const TestHealth& other) const {
        return health == other.health && max_health == other.max_health;
    }
};

struct TestTag {
    std::string tag = "default";
    
    bool operator==(const TestTag& other) const {
        return tag == other.tag;
    }
};

// Large component for memory testing
struct LargeTestComponent {
    std::array<double, 1024> data;
    
    LargeTestComponent() { data.fill(0.0); }
    
    bool operator==(const LargeTestComponent& other) const {
        return data == other.data;
    }
};

// =============================================================================
// Test Utilities
// =============================================================================

/**
 * Helper class for creating test entities with various component combinations
 */
class EntityFactory {
public:
    explicit EntityFactory(World& world) : world_(world) {}

    // Create entity with position only
    Entity create_positioned(float x = 0, float y = 0, float z = 0) {
        auto entity = world_.create_entity();
        world_.add_component<TestPosition>(entity, x, y, z);
        return entity;
    }

    // Create entity with position and velocity
    Entity create_moving(float x = 0, float y = 0, float z = 0,
                        float vx = 0, float vy = 0, float vz = 0) {
        auto entity = world_.create_entity();
        world_.add_component<TestPosition>(entity, x, y, z);
        world_.add_component<TestVelocity>(entity, vx, vy, vz);
        return entity;
    }

    // Create entity with health component
    Entity create_with_health(int health = 100, int max_health = 100) {
        auto entity = world_.create_entity();
        world_.add_component<TestHealth>(entity, health, max_health);
        return entity;
    }

    // Create entity with all test components
    Entity create_full_entity(const TestPosition& pos = {},
                             const TestVelocity& vel = {},
                             const TestHealth& health = {},
                             const TestTag& tag = {}) {
        auto entity = world_.create_entity();
        world_.add_component<TestPosition>(entity, pos);
        world_.add_component<TestVelocity>(entity, vel);
        world_.add_component<TestHealth>(entity, health);
        world_.add_component<TestTag>(entity, tag);
        return entity;
    }

    // Create many entities for performance testing
    std::vector<Entity> create_many(size_t count, bool with_velocity = true) {
        std::vector<Entity> entities;
        entities.reserve(count);
        
        for (size_t i = 0; i < count; ++i) {
            if (with_velocity) {
                entities.push_back(create_moving(
                    static_cast<float>(i),
                    static_cast<float>(i * 2),
                    static_cast<float>(i * 3),
                    1.0f, 1.0f, 1.0f
                ));
            } else {
                entities.push_back(create_positioned(
                    static_cast<float>(i),
                    static_cast<float>(i * 2),
                    static_cast<float>(i * 3)
                ));
            }
        }
        
        return entities;
    }

private:
    World& world_;
};

/**
 * Performance measurement utilities
 */
class PerformanceMeter {
public:
    template<typename Func>
    static std::chrono::nanoseconds time_execution(Func&& func) {
        auto start = std::chrono::high_resolution_clock::now();
        func();
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    }

    template<typename Func>
    static double benchmark_average(Func&& func, int iterations = 1000) {
        std::vector<std::chrono::nanoseconds> times;
        times.reserve(iterations);
        
        for (int i = 0; i < iterations; ++i) {
            times.push_back(time_execution(func));
        }
        
        auto total = std::accumulate(times.begin(), times.end(), 
                                   std::chrono::nanoseconds{0});
        return static_cast<double>(total.count()) / iterations / 1e6; // Convert to ms
    }

    template<typename Func>
    static bool is_within_tolerance(Func&& func, double expected_ms, 
                                  double tolerance = PERFORMANCE_TOLERANCE) {
        double actual_ms = benchmark_average(func);
        double diff = std::abs(actual_ms - expected_ms);
        return (diff / expected_ms) <= tolerance;
    }
};

// =============================================================================
// Test Macros
// =============================================================================

#define EXPECT_PERFORMANCE_WITHIN(func, expected_ms, tolerance) \
    EXPECT_TRUE(ECScope::Testing::PerformanceMeter::is_within_tolerance( \
        [&]() { func; }, expected_ms, tolerance)) \
    << "Performance expectation failed for " #func

#define EXPECT_NO_MEMORY_LEAKS() \
    EXPECT_EQ(memory_tracker_->get_allocation_count(), \
             memory_tracker_->get_deallocation_count()) \
    << "Memory leak detected!"

#define BENCHMARK_TEST(fixture, name, iterations) \
    TEST_F(fixture, Benchmark_##name) { \
        benchmark(#name, [this]() { \
            /* Benchmark code goes here */ \
        }, iterations); \
    }

// =============================================================================
// Mock Objects for Testing
// =============================================================================

/**
 * Mock system for testing system integration
 */
class MockSystem {
public:
    MOCK_METHOD(void, update, (float dt), ());
    MOCK_METHOD(void, initialize, (), ());
    MOCK_METHOD(void, shutdown, (), ());
    MOCK_METHOD(bool, is_enabled, (), (const));
};

/**
 * Mock allocator for testing memory systems
 */
class MockAllocator {
public:
    MOCK_METHOD(void*, allocate, (size_t size, size_t alignment), ());
    MOCK_METHOD(void, deallocate, (void* ptr, size_t size), ());
    MOCK_METHOD(size_t, get_allocated_bytes, (), (const));
    MOCK_METHOD(bool, owns, (void* ptr), (const));
};

} // namespace ECScope::Testing