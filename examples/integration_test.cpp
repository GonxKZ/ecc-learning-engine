// ECScope Integration Test
// Tests all major systems working together

#include "ecscope/ecs/registry.hpp"
#include "ecscope/ecs/system.hpp"
#include "ecscope/ecs/query.hpp"
#include "ecscope/memory/arena.hpp"
#include "ecscope/memory/pool_allocator.hpp"
#include "ecscope/jobs/fiber_job_system.hpp"
#include "ecscope/physics/world.hpp"
#include "ecscope/physics/components.hpp"
#include "ecscope/core/time.hpp"
#include "ecscope/core/log.hpp"
#include "ecscope/instrumentation/trace.hpp"
#include "ecscope/profiling/ecs_profiler.hpp"

#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <thread>
#include <iomanip>

using namespace ecscope;

// Test Components
struct Position {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    
    Position() = default;
    Position(float x, float y, float z) : x(x), y(y), z(z) {}
};

struct Velocity {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    
    Velocity() = default;
    Velocity(float x, float y, float z) : x(x), y(y), z(z) {}
};

struct Health {
    float current = 100.0f;
    float maximum = 100.0f;
    
    Health() = default;
    Health(float max_health) : current(max_health), maximum(max_health) {}
};

struct PhysicsBody {
    float mass = 1.0f;
    bool is_static = false;
    
    PhysicsBody() = default;
    PhysicsBody(float m, bool static_body = false) : mass(m), is_static(static_body) {}
};

// Test Systems
class MovementSystem : public ecs::System {
public:
    void update(ecs::Registry& registry, float delta_time) override {
        ECSCOPE_TRACE_FUNCTION();
        
        auto view = registry.view<Position, Velocity>();
        view.each([delta_time](auto entity, Position& pos, const Velocity& vel) {
            pos.x += vel.x * delta_time;
            pos.y += vel.y * delta_time;
            pos.z += vel.z * delta_time;
        });
    }
    
    std::string get_name() const override {
        return "MovementSystem";
    }
};

class HealthSystem : public ecs::System {
public:
    void update(ecs::Registry& registry, float delta_time) override {
        ECSCOPE_TRACE_FUNCTION();
        
        auto view = registry.view<Health>();
        std::vector<ecs::Entity> to_remove;
        
        view.each([&](auto entity, Health& health) {
            // Simulate some health decay
            health.current -= 1.0f * delta_time;
            
            // Mark dead entities for removal
            if (health.current <= 0.0f) {
                to_remove.push_back(entity);
            }
        });
        
        // Remove dead entities
        for (auto entity : to_remove) {
            registry.destroy(entity);
        }
    }
    
    std::string get_name() const override {
        return "HealthSystem";
    }
};

// Integration Test Runner
class IntegrationTestRunner {
public:
    IntegrationTestRunner() : 
        arena_(1024 * 1024 * 10), // 10MB arena
        pool_(sizeof(Position), 1000),
        job_system_(std::thread::hardware_concurrency()) {}
    
    bool run_all_tests() {
        std::cout << "=== ECScope Engine Integration Test ===" << std::endl;
        std::cout << "Testing all major systems working together..." << std::endl;
        std::cout << std::endl;
        
        bool all_passed = true;
        
        all_passed &= test_memory_management();
        all_passed &= test_ecs_basic_functionality();
        all_passed &= test_ecs_performance_large_scale();
        all_passed &= test_job_system_integration();
        all_passed &= test_physics_integration();
        all_passed &= test_system_coordination();
        all_passed &= test_stress_scenario();
        
        std::cout << std::endl;
        if (all_passed) {
            std::cout << "✓ ALL INTEGRATION TESTS PASSED!" << std::endl;
        } else {
            std::cout << "✗ Some integration tests failed." << std::endl;
        }
        
        return all_passed;
    }

private:
    memory::ArenaAllocator arena_;
    memory::PoolAllocator pool_;
    jobs::FiberJobSystem job_system_;
    ecs::ProfilerData profiler_data_;
    
    bool test_memory_management() {
        std::cout << "Testing Memory Management..." << std::endl;
        
        try {
            // Test arena allocator
            void* ptr1 = arena_.allocate(1024, 16);
            void* ptr2 = arena_.allocate(2048, 32);
            
            if (!ptr1 || !ptr2) {
                std::cout << "  ✗ Arena allocation failed" << std::endl;
                return false;
            }
            
            // Test pool allocator
            void* pool_ptr1 = pool_.allocate();
            void* pool_ptr2 = pool_.allocate();
            
            if (!pool_ptr1 || !pool_ptr2) {
                std::cout << "  ✗ Pool allocation failed" << std::endl;
                return false;
            }
            
            pool_.deallocate(pool_ptr1);
            pool_.deallocate(pool_ptr2);
            
            std::cout << "  ✓ Memory allocators working correctly" << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            std::cout << "  ✗ Memory management test failed: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool test_ecs_basic_functionality() {
        std::cout << "Testing ECS Basic Functionality..." << std::endl;
        
        try {
            ecs::Registry registry;
            
            // Create entities with components
            auto entity1 = registry.create();
            auto entity2 = registry.create();
            auto entity3 = registry.create();
            
            registry.emplace<Position>(entity1, 10.0f, 20.0f, 30.0f);
            registry.emplace<Velocity>(entity1, 1.0f, 0.0f, 0.0f);
            registry.emplace<Health>(entity1, 100.0f);
            
            registry.emplace<Position>(entity2, 0.0f, 0.0f, 0.0f);
            registry.emplace<Velocity>(entity2, -1.0f, 1.0f, 0.0f);
            
            registry.emplace<Position>(entity3, 5.0f, 5.0f, 5.0f);
            registry.emplace<Health>(entity3, 50.0f);
            
            // Test queries
            auto pos_vel_view = registry.view<Position, Velocity>();
            size_t entities_with_pos_vel = 0;
            pos_vel_view.each([&](auto entity, const Position&, const Velocity&) {
                entities_with_pos_vel++;
            });
            
            if (entities_with_pos_vel != 2) {
                std::cout << "  ✗ Query failed - expected 2 entities with Position+Velocity, got " 
                          << entities_with_pos_vel << std::endl;
                return false;
            }
            
            // Test component access
            auto* pos = registry.try_get<Position>(entity1);
            if (!pos || pos->x != 10.0f) {
                std::cout << "  ✗ Component access failed" << std::endl;
                return false;
            }
            
            // Test entity destruction
            registry.destroy(entity2);
            
            auto all_positions = registry.view<Position>();
            size_t remaining_entities = 0;
            all_positions.each([&](auto entity, const Position&) {
                remaining_entities++;
            });
            
            if (remaining_entities != 2) {
                std::cout << "  ✗ Entity destruction failed - expected 2 remaining, got " 
                          << remaining_entities << std::endl;
                return false;
            }
            
            std::cout << "  ✓ ECS basic functionality working correctly" << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            std::cout << "  ✗ ECS test failed: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool test_ecs_performance_large_scale() {
        std::cout << "Testing ECS Large Scale Performance..." << std::endl;
        
        try {
            ecs::Registry registry;
            ecs::Profiler profiler;
            
            const size_t entity_count = 100000;
            std::vector<ecs::Entity> entities;
            entities.reserve(entity_count);
            
            // Create large number of entities
            auto start_time = std::chrono::high_resolution_clock::now();
            
            for (size_t i = 0; i < entity_count; ++i) {
                auto entity = registry.create();
                entities.push_back(entity);
                
                registry.emplace<Position>(entity, 
                    static_cast<float>(i % 1000),
                    static_cast<float>((i / 1000) % 1000),
                    static_cast<float>(i / 1000000)
                );
                
                if (i % 2 == 0) {
                    registry.emplace<Velocity>(entity,
                        static_cast<float>(i % 10 - 5),
                        static_cast<float>((i / 10) % 10 - 5),
                        0.0f
                    );
                }
                
                if (i % 3 == 0) {
                    registry.emplace<Health>(entity, 100.0f);
                }
            }
            
            auto creation_time = std::chrono::high_resolution_clock::now();
            auto creation_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                creation_time - start_time).count();
            
            // Test large scale queries
            auto query_start = std::chrono::high_resolution_clock::now();
            
            size_t position_count = 0;
            auto pos_view = registry.view<Position>();
            pos_view.each([&](auto entity, const Position& pos) {
                position_count++;
                // Simulate some work
                volatile float result = pos.x * pos.y + pos.z;
                (void)result;
            });
            
            size_t pos_vel_count = 0;
            auto pos_vel_view = registry.view<Position, Velocity>();
            pos_vel_view.each([&](auto entity, const Position& pos, const Velocity& vel) {
                pos_vel_count++;
                // Simulate movement update
                volatile float new_x = pos.x + vel.x * 0.016f;
                volatile float new_y = pos.y + vel.y * 0.016f;
                (void)new_x; (void)new_y;
            });
            
            auto query_end = std::chrono::high_resolution_clock::now();
            auto query_duration = std::chrono::duration_cast<std::chrono::microseconds>(
                query_end - query_start).count();
            
            // Verify counts
            if (position_count != entity_count) {
                std::cout << "  ✗ Position count mismatch - expected " << entity_count 
                          << ", got " << position_count << std::endl;
                return false;
            }
            
            if (pos_vel_count != entity_count / 2) {
                std::cout << "  ✗ Position+Velocity count mismatch - expected " << entity_count / 2
                          << ", got " << pos_vel_count << std::endl;
                return false;
            }
            
            std::cout << "  ✓ Created " << entity_count << " entities in " << creation_duration << "ms" << std::endl;
            std::cout << "  ✓ Processed " << position_count << " position components in " 
                      << query_duration << "μs" << std::endl;
            std::cout << "  ✓ Query performance: " << std::fixed << std::setprecision(2)
                      << (static_cast<double>(position_count) / query_duration) << " entities/μs" << std::endl;
            
            return true;
            
        } catch (const std::exception& e) {
            std::cout << "  ✗ Large scale ECS test failed: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool test_job_system_integration() {
        std::cout << "Testing Job System Integration..." << std::endl;
        
        try {
            if (!job_system_.initialize()) {
                std::cout << "  ✗ Job system initialization failed" << std::endl;
                return false;
            }
            
            // Test parallel job execution
            std::atomic<int> counter{0};
            const int job_count = 1000;
            
            for (int i = 0; i < job_count; ++i) {
                job_system_.enqueue([&counter]() {
                    counter.fetch_add(1);
                    // Simulate some work
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                });
            }
            
            // Wait for jobs to complete
            auto start = std::chrono::steady_clock::now();
            while (counter.load() < job_count) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                
                // Timeout after 5 seconds
                auto elapsed = std::chrono::steady_clock::now() - start;
                if (elapsed > std::chrono::seconds(5)) {
                    std::cout << "  ✗ Job system timeout - completed " << counter.load() 
                              << "/" << job_count << " jobs" << std::endl;
                    return false;
                }
            }
            
            job_system_.shutdown();
            
            std::cout << "  ✓ Successfully executed " << job_count << " parallel jobs" << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            std::cout << "  ✗ Job system test failed: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool test_physics_integration() {
        std::cout << "Testing Physics Integration..." << std::endl;
        
        try {
            // Test physics world creation and basic operations
            physics::World2D world;
            
            // Create physics bodies (simplified test)
            physics::Body body1, body2;
            body1.position = {0.0f, 0.0f};
            body1.velocity = {1.0f, 0.0f};
            body1.mass = 1.0f;
            
            body2.position = {5.0f, 0.0f};
            body2.velocity = {-1.0f, 0.0f};
            body2.mass = 1.0f;
            
            // Simulate physics step
            world.step(0.016f); // 60 FPS
            
            std::cout << "  ✓ Physics world simulation completed" << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            std::cout << "  ✗ Physics integration test failed: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool test_system_coordination() {
        std::cout << "Testing System Coordination..." << std::endl;
        
        try {
            ecs::Registry registry;
            MovementSystem movement_system;
            HealthSystem health_system;
            
            // Create test entities
            const size_t entity_count = 1000;
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<float> pos_dist(-100.0f, 100.0f);
            std::uniform_real_distribution<float> vel_dist(-10.0f, 10.0f);
            std::uniform_real_distribution<float> health_dist(50.0f, 150.0f);
            
            for (size_t i = 0; i < entity_count; ++i) {
                auto entity = registry.create();
                
                registry.emplace<Position>(entity, 
                    pos_dist(gen), pos_dist(gen), pos_dist(gen));
                registry.emplace<Velocity>(entity,
                    vel_dist(gen), vel_dist(gen), vel_dist(gen));
                registry.emplace<Health>(entity, health_dist(gen));
            }
            
            // Run multiple update cycles
            const float delta_time = 1.0f/60.0f;
            const int update_cycles = 300; // 5 seconds at 60 FPS
            
            auto start_time = std::chrono::high_resolution_clock::now();
            
            for (int cycle = 0; cycle < update_cycles; ++cycle) {
                movement_system.update(registry, delta_time);
                health_system.update(registry, delta_time);
            }
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                end_time - start_time).count();
            
            // Check remaining entities
            size_t remaining_entities = 0;
            registry.view<Position>().each([&](auto entity, const Position&) {
                remaining_entities++;
            });
            
            std::cout << "  ✓ Processed " << update_cycles << " update cycles in " 
                      << duration << "ms" << std::endl;
            std::cout << "  ✓ " << remaining_entities << "/" << entity_count 
                      << " entities survived health decay" << std::endl;
            std::cout << "  ✓ Average cycle time: " << std::fixed << std::setprecision(2)
                      << (static_cast<double>(duration) / update_cycles) << "ms" << std::endl;
            
            return true;
            
        } catch (const std::exception& e) {
            std::cout << "  ✗ System coordination test failed: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool test_stress_scenario() {
        std::cout << "Testing Stress Scenario..." << std::endl;
        
        try {
            ecs::Registry registry;
            MovementSystem movement_system;
            
            // Create large number of entities with complex interactions
            const size_t entity_count = 50000;
            std::cout << "  Creating " << entity_count << " entities for stress test..." << std::endl;
            
            auto creation_start = std::chrono::high_resolution_clock::now();
            
            for (size_t i = 0; i < entity_count; ++i) {
                auto entity = registry.create();
                
                registry.emplace<Position>(entity, 
                    static_cast<float>(i % 100),
                    static_cast<float>((i / 100) % 100),
                    static_cast<float>(i / 10000)
                );
                
                registry.emplace<Velocity>(entity,
                    static_cast<float>((i % 3) - 1),
                    static_cast<float>(((i / 3) % 3) - 1),
                    0.0f
                );
                
                registry.emplace<Health>(entity, 1000.0f); // Long lived entities
                
                if (i % 2 == 0) {
                    registry.emplace<PhysicsBody>(entity, 1.0f, false);
                }
            }
            
            auto creation_end = std::chrono::high_resolution_clock::now();
            auto creation_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                creation_end - creation_start).count();
            
            // Run stress simulation
            const int stress_cycles = 100;
            const float delta_time = 1.0f/60.0f;
            
            auto simulation_start = std::chrono::high_resolution_clock::now();
            
            for (int cycle = 0; cycle < stress_cycles; ++cycle) {
                movement_system.update(registry, delta_time);
                
                // Additional stress operations
                if (cycle % 10 == 0) {
                    // Query stress test
                    size_t query_count = 0;
                    auto view = registry.view<Position, Velocity, Health>();
                    view.each([&](auto entity, const Position& pos, const Velocity& vel, const Health& health) {
                        query_count++;
                        // Simulate complex calculations
                        volatile float distance = std::sqrt(pos.x * pos.x + pos.y * pos.y + pos.z * pos.z);
                        volatile float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y + vel.z * vel.z);
                        (void)distance; (void)speed;
                    });
                }
            }
            
            auto simulation_end = std::chrono::high_resolution_clock::now();
            auto simulation_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                simulation_end - simulation_start).count();
            
            std::cout << "  ✓ Entity creation: " << creation_time << "ms" << std::endl;
            std::cout << "  ✓ Stress simulation: " << simulation_time << "ms" << std::endl;
            std::cout << "  ✓ Average frame time: " << std::fixed << std::setprecision(2)
                      << (static_cast<double>(simulation_time) / stress_cycles) << "ms" << std::endl;
            std::cout << "  ✓ Theoretical FPS: " << std::fixed << std::setprecision(1)
                      << (1000.0 * stress_cycles / simulation_time) << " FPS" << std::endl;
            
            return true;
            
        } catch (const std::exception& e) {
            std::cout << "  ✗ Stress scenario test failed: " << e.what() << std::endl;
            return false;
        }
    }
};

int main() {
    try {
        // Initialize logging
        core::Logger::initialize(core::LogLevel::Info);
        
        // Initialize timing
        core::Time::initialize();
        
        // Run integration tests
        IntegrationTestRunner test_runner;
        bool success = test_runner.run_all_tests();
        
        return success ? 0 : 1;
        
    } catch (const std::exception& e) {
        std::cerr << "Integration test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}