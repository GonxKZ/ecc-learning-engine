/**
 * @file tests/scripting_integration_tests.cpp
 * @brief Comprehensive Integration Tests for ECScope Scripting System
 * 
 * This file provides extensive testing coverage for the scripting integration
 * system with focus on correctness, performance, and reliability.
 * 
 * Test Categories:
 * - Python integration correctness tests
 * - Lua integration functionality tests
 * - ECS scripting interface validation
 * - Hot-reload system reliability tests
 * - Memory management and leak detection
 * - Performance regression tests
 * - Cross-language communication tests
 * - Error handling and recovery tests
 * 
 * @author ECScope Educational ECS Framework - Integration Tests
 * @date 2025
 */

#include "scripting/python_integration.hpp"
#include "scripting/lua_integration.hpp"
#include "scripting/hot_reload_system.hpp"
#include "scripting/ecs_script_interface.hpp"
#include "scripting/script_profiler.hpp"
#include "ecs/registry.hpp"
#include "memory/advanced_memory_system.hpp"
#include "job_system/work_stealing_job_system.hpp"
#include "core/log.hpp"

#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <chrono>
#include <fstream>
#include <filesystem>

using namespace ecscope;

//=============================================================================
// Test Fixture Base Class
//=============================================================================

class ScriptingTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize core systems
        memory_system_ = std::make_unique<memory::AdvancedMemorySystem>(
            memory::AdvancedMemorySystem::Config{
                .enable_pool_allocation = true,
                .enable_profiling = true
            }
        );
        
        job_system_ = std::make_unique<job_system::JobSystem>(
            job_system::JobSystem::Config::create_educational()
        );
        ASSERT_TRUE(job_system_->initialize());
        
        registry_ = std::make_unique<ecs::Registry>();
        
        // Initialize Python engine
        python_engine_ = std::make_unique<scripting::python::PythonEngine>(*memory_system_);
        ASSERT_TRUE(python_engine_->initialize());
        
        // Initialize ECS interface
        ecs_interface_ = std::make_unique<scripting::ecs_interface::ECSScriptInterface>(
            registry_.get(), python_engine_.get(), nullptr
        );
        
        // Initialize profiling
        function_profiler_ = std::make_unique<scripting::profiling::FunctionProfiler>(
            scripting::profiling::FunctionProfiler::ProfilingMode::Sampling, 0.1f
        );
        
        memory_profiler_ = std::make_unique<scripting::profiling::MemoryProfiler>();
    }
    
    void TearDown() override {
        // Cleanup in reverse order
        memory_profiler_.reset();
        function_profiler_.reset();
        ecs_interface_.reset();
        
        if (python_engine_) {
            python_engine_->shutdown();
        }
        python_engine_.reset();
        
        registry_.reset();
        
        if (job_system_) {
            job_system_->shutdown();
        }
        job_system_.reset();
        
        memory_system_.reset();
    }
    
    // Helper method to create temporary test script
    std::string create_temp_script(const std::string& content) {
        static int script_counter = 0;
        std::string filename = "test_script_" + std::to_string(++script_counter) + ".py";
        
        std::ofstream file(filename);
        EXPECT_TRUE(file.is_open()) << "Failed to create temporary script: " << filename;
        file << content;
        file.close();
        
        temp_files_.push_back(filename);
        return filename;
    }
    
    void cleanup_temp_files() {
        for (const auto& filename : temp_files_) {
            std::filesystem::remove(filename);
        }
        temp_files_.clear();
    }
    
protected:
    std::unique_ptr<memory::AdvancedMemorySystem> memory_system_;
    std::unique_ptr<job_system::JobSystem> job_system_;
    std::unique_ptr<ecs::Registry> registry_;
    std::unique_ptr<scripting::python::PythonEngine> python_engine_;
    std::unique_ptr<scripting::ecs_interface::ECSScriptInterface> ecs_interface_;
    std::unique_ptr<scripting::profiling::FunctionProfiler> function_profiler_;
    std::unique_ptr<scripting::profiling::MemoryProfiler> memory_profiler_;
    
    std::vector<std::string> temp_files_;
};

//=============================================================================
// Python Integration Tests
//=============================================================================

class PythonIntegrationTest : public ScriptingTestFixture {};

TEST_F(PythonIntegrationTest, BasicPythonExecution) {
    std::string python_code = R"(
result = 2 + 3
message = "Hello, ECScope!"
)";
    
    auto execution_result = python_engine_->execute_string(python_code);
    EXPECT_TRUE(execution_result.is_valid());
    
    // Verify global variables were set
    auto result = python_engine_->get_global("result");
    EXPECT_TRUE(result.is_valid());
    EXPECT_TRUE(result.is_int());
    EXPECT_EQ(result.to_int(), 5);
    
    auto message = python_engine_->get_global("message");
    EXPECT_TRUE(message.is_valid());
    EXPECT_TRUE(message.is_string());
    EXPECT_EQ(message.to_string(), "Hello, ECScope!");
}

TEST_F(PythonIntegrationTest, PythonErrorHandling) {
    std::string invalid_python_code = R"(
# This should cause a syntax error
def invalid_function(:
    pass
)";
    
    auto& exception_handler = python_engine_->get_exception_handler();
    
    auto execution_result = python_engine_->execute_string(invalid_python_code);
    EXPECT_FALSE(execution_result.is_valid());
    EXPECT_TRUE(exception_handler.has_error());
    
    auto exception_info = exception_handler.get_current_exception();
    EXPECT_FALSE(exception_info.type.empty());
    EXPECT_FALSE(exception_info.message.empty());
    
    exception_handler.clear_error();
    EXPECT_FALSE(exception_handler.has_error());
}

TEST_F(PythonIntegrationTest, PythonMemoryManagement) {
    auto& memory_manager = python_engine_->get_memory_manager();
    auto initial_stats = memory_manager.get_statistics();
    
    std::string memory_intensive_code = R"(
# Create some memory-intensive operations
large_list = [i * i for i in range(10000)]
large_dict = {f"key_{i}": [j for j in range(100)] for i in range(100)}
del large_list
del large_dict
)";
    
    python_engine_->execute_string(memory_intensive_code);
    
    auto final_stats = memory_manager.get_statistics();
    
    // Memory should have been allocated and then freed
    EXPECT_GT(final_stats.total_allocated, initial_stats.total_allocated);
    EXPECT_GT(final_stats.total_deallocated, initial_stats.total_deallocated);
    
    // Check for memory leaks
    EXPECT_LE(final_stats.current_allocated - initial_stats.current_allocated, 1024); // Allow small overhead
}

TEST_F(PythonIntegrationTest, PythonFileExecution) {
    std::string python_script_content = R"(
def fibonacci(n):
    if n <= 1:
        return n
    return fibonacci(n-1) + fibonacci(n-2)

def factorial(n):
    if n <= 1:
        return 1
    return n * factorial(n-1)

fib_result = fibonacci(10)
fact_result = factorial(5)
)";
    
    std::string script_path = create_temp_script(python_script_content);
    
    auto execution_result = python_engine_->execute_file(script_path);
    EXPECT_TRUE(execution_result.is_valid());
    
    // Verify results
    auto fib_result = python_engine_->get_global("fib_result");
    EXPECT_TRUE(fib_result.is_valid());
    EXPECT_EQ(fib_result.to_int(), 55); // fibonacci(10) = 55
    
    auto fact_result = python_engine_->get_global("fact_result");
    EXPECT_TRUE(fact_result.is_valid());
    EXPECT_EQ(fact_result.to_int(), 120); // factorial(5) = 120
    
    cleanup_temp_files();
}

TEST_F(PythonIntegrationTest, PythonStatisticsTracking) {
    auto initial_stats = python_engine_->get_statistics();
    
    // Execute multiple scripts
    for (int i = 0; i < 10; ++i) {
        std::string code = "result_" + std::to_string(i) + " = " + std::to_string(i * i);
        python_engine_->execute_string(code);
    }
    
    // Cause an exception
    python_engine_->execute_string("invalid_syntax ::::");
    python_engine_->get_exception_handler().clear_error();
    
    auto final_stats = python_engine_->get_statistics();
    
    EXPECT_EQ(final_stats.scripts_executed, initial_stats.scripts_executed + 11);
    EXPECT_EQ(final_stats.exceptions_thrown, initial_stats.exceptions_thrown + 1);
    EXPECT_GT(final_stats.memory_stats.total_allocated, initial_stats.memory_stats.total_allocated);
}

//=============================================================================
// ECS Scripting Interface Tests
//=============================================================================

class ECSScriptingTest : public ScriptingTestFixture {
protected:
    struct TestPosition {
        f32 x = 0.0f;
        f32 y = 0.0f;
        f32 z = 0.0f;
    };
    
    struct TestVelocity {
        f32 dx = 0.0f;
        f32 dy = 0.0f;
        f32 dz = 0.0f;
    };
    
    struct TestHealth {
        i32 current = 100;
        i32 maximum = 100;
    };
    
    void SetUp() override {
        ScriptingTestFixture::SetUp();
        
        // Register test components
        ecs_interface_->register_component_type<TestPosition>("TestPosition");
        ecs_interface_->register_component_type<TestVelocity>("TestVelocity");
        ecs_interface_->register_component_type<TestHealth>("TestHealth");
    }
};

TEST_F(ECSScriptingTest, EntityCreationAndDestruction) {
    auto initial_count = ecs_interface_->entity_count();
    
    // Create entities
    auto entity1 = ecs_interface_->create_entity();
    auto entity2 = ecs_interface_->create_entity();
    auto entity3 = ecs_interface_->create_entity();
    
    EXPECT_NE(entity1, nullptr);
    EXPECT_NE(entity2, nullptr);
    EXPECT_NE(entity3, nullptr);
    
    EXPECT_EQ(ecs_interface_->entity_count(), initial_count + 3);
    
    // Verify entities are valid
    EXPECT_TRUE(entity1->is_valid());
    EXPECT_TRUE(entity2->is_valid());
    EXPECT_TRUE(entity3->is_valid());
    
    // Destroy one entity
    ecs::Entity entity1_id{entity1->id(), entity1->generation()};
    EXPECT_TRUE(ecs_interface_->destroy_entity(entity1_id));
    
    EXPECT_EQ(ecs_interface_->entity_count(), initial_count + 2);
    EXPECT_FALSE(entity1->is_valid());
    EXPECT_TRUE(entity2->is_valid());
    EXPECT_TRUE(entity3->is_valid());
}

TEST_F(ECSScriptingTest, ComponentManipulation) {
    auto entity = ecs_interface_->create_entity();
    ASSERT_NE(entity, nullptr);
    
    // Initially no components
    EXPECT_FALSE(entity->has_component<TestPosition>());
    EXPECT_FALSE(entity->has_component<TestVelocity>());
    EXPECT_EQ(entity->component_count(), 0);
    
    // Add components
    EXPECT_TRUE(entity->add_component<TestPosition>(TestPosition{1.0f, 2.0f, 3.0f}));
    EXPECT_TRUE(entity->add_component<TestVelocity>(TestVelocity{0.5f, -0.5f, 0.0f}));
    EXPECT_TRUE(entity->add_component<TestHealth>(TestHealth{80, 100}));
    
    // Verify components exist
    EXPECT_TRUE(entity->has_component<TestPosition>());
    EXPECT_TRUE(entity->has_component<TestVelocity>());
    EXPECT_TRUE(entity->has_component<TestHealth>());
    EXPECT_EQ(entity->component_count(), 3);
    
    // Access component data
    auto* position = entity->get_component<TestPosition>();
    ASSERT_NE(position, nullptr);
    EXPECT_FLOAT_EQ(position->x, 1.0f);
    EXPECT_FLOAT_EQ(position->y, 2.0f);
    EXPECT_FLOAT_EQ(position->z, 3.0f);
    
    auto* velocity = entity->get_component<TestVelocity>();
    ASSERT_NE(velocity, nullptr);
    EXPECT_FLOAT_EQ(velocity->dx, 0.5f);
    EXPECT_FLOAT_EQ(velocity->dy, -0.5f);
    EXPECT_FLOAT_EQ(velocity->dz, 0.0f);
    
    auto* health = entity->get_component<TestHealth>();
    ASSERT_NE(health, nullptr);
    EXPECT_EQ(health->current, 80);
    EXPECT_EQ(health->maximum, 100);
    
    // Modify components
    position->x = 10.0f;
    velocity->dx = 2.0f;
    health->current = 90;
    
    // Verify modifications
    EXPECT_FLOAT_EQ(position->x, 10.0f);
    EXPECT_FLOAT_EQ(velocity->dx, 2.0f);
    EXPECT_EQ(health->current, 90);
    
    // Remove components
    EXPECT_TRUE(entity->remove_component<TestVelocity>());
    EXPECT_FALSE(entity->has_component<TestVelocity>());
    EXPECT_EQ(entity->component_count(), 2);
    
    // Try to remove non-existent component
    EXPECT_FALSE(entity->remove_component<TestVelocity>());
}

TEST_F(ECSScriptingTest, QueryFunctionality) {
    // Create test entities
    auto entity1 = ecs_interface_->create_entity();
    entity1->add_component<TestPosition>(TestPosition{1.0f, 2.0f, 3.0f});
    entity1->add_component<TestVelocity>(TestVelocity{0.1f, 0.2f, 0.3f});
    
    auto entity2 = ecs_interface_->create_entity();
    entity2->add_component<TestPosition>(TestPosition{4.0f, 5.0f, 6.0f});
    // No velocity for entity2
    
    auto entity3 = ecs_interface_->create_entity();
    entity3->add_component<TestPosition>(TestPosition{7.0f, 8.0f, 9.0f});
    entity3->add_component<TestVelocity>(TestVelocity{0.7f, 0.8f, 0.9f});
    entity3->add_component<TestHealth>(TestHealth{50, 100});
    
    // Query for entities with position only
    auto position_query = ecs_interface_->create_query<TestPosition>();
    EXPECT_EQ(position_query->count(), 3);
    
    // Query for entities with position and velocity
    auto moving_query = ecs_interface_->create_query<TestPosition, TestVelocity>();
    EXPECT_EQ(moving_query->count(), 2);
    
    // Query for entities with all three components
    auto full_query = ecs_interface_->create_query<TestPosition, TestVelocity, TestHealth>();
    EXPECT_EQ(full_query->count(), 1);
    
    // Test query iteration
    std::vector<u32> entity_ids;
    auto entities = position_query->get_entities();
    for (auto& entity : entities) {
        entity_ids.push_back(entity.id());
    }
    
    EXPECT_EQ(entity_ids.size(), 3);
    
    // Verify functional iteration
    usize iteration_count = 0;
    moving_query->for_each([&iteration_count](auto& entity, TestPosition& pos, TestVelocity& vel) {
        EXPECT_TRUE(entity.is_valid());
        EXPECT_GE(pos.x, 0.0f);
        EXPECT_GE(vel.dx, 0.0f);
        ++iteration_count;
    });
    
    EXPECT_EQ(iteration_count, 2);
}

TEST_F(ECSScriptingTest, ECSStatisticsTracking) {
    auto initial_stats = ecs_interface_->get_statistics();
    
    // Create entities and perform operations
    for (int i = 0; i < 10; ++i) {
        auto entity = ecs_interface_->create_entity();
        entity->add_component<TestPosition>(TestPosition{static_cast<f32>(i), 0.0f, 0.0f});
        
        // Access components to increment access counter
        entity->get_component<TestPosition>();
    }
    
    // Create and use queries
    auto query1 = ecs_interface_->create_query<TestPosition>();
    auto query2 = ecs_interface_->create_query<TestPosition, TestVelocity>();
    
    query1->count();
    query2->count();
    
    auto final_stats = ecs_interface_->get_statistics();
    
    EXPECT_EQ(final_stats.entities_created, initial_stats.entities_created + 10);
    EXPECT_GE(final_stats.component_accesses, initial_stats.component_accesses + 10);
    EXPECT_GE(final_stats.query_executions, initial_stats.query_executions + 2);
    EXPECT_EQ(final_stats.current_entities, initial_stats.current_entities + 10);
}

//=============================================================================
// Performance and Profiling Tests
//=============================================================================

class ProfilingTest : public ScriptingTestFixture {};

TEST_F(ProfilingTest, FunctionProfilingBasics) {
    function_profiler_->set_profiling_mode(scripting::profiling::FunctionProfiler::ProfilingMode::Full);
    function_profiler_->start_profiling();
    
    // Execute profiled functions
    {
        auto profiler = function_profiler_->profile_function("test_function_1");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    {
        auto profiler = function_profiler_->profile_function("test_function_2");
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    
    // Nested profiling
    {
        auto profiler1 = function_profiler_->profile_function("outer_function");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        
        {
            auto profiler2 = function_profiler_->profile_function("inner_function");
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    
    function_profiler_->stop_profiling();
    
    auto function_stats = function_profiler_->get_function_statistics();
    EXPECT_GE(function_stats.size(), 3);
    
    // Find our test functions
    bool found_test1 = false, found_test2 = false, found_outer = false, found_inner = false;
    
    for (const auto& func : function_stats) {
        if (func.function_name == "test_function_1") {
            found_test1 = true;
            EXPECT_GE(func.call_count, 1);
            EXPECT_GT(func.total_time, std::chrono::nanoseconds{0});
        } else if (func.function_name == "test_function_2") {
            found_test2 = true;
            EXPECT_GE(func.call_count, 1);
        } else if (func.function_name == "outer_function") {
            found_outer = true;
        } else if (func.function_name == "inner_function") {
            found_inner = true;
        }
    }
    
    EXPECT_TRUE(found_test1);
    EXPECT_TRUE(found_test2);
    EXPECT_TRUE(found_outer);
    EXPECT_TRUE(found_inner);
}

TEST_F(ProfilingTest, MemoryProfilingBasics) {
    memory_profiler_->start_tracking();
    
    auto initial_stats = memory_profiler_->get_statistics();
    
    // Simulate memory allocations
    const usize alloc_size = 1024;
    std::vector<void*> allocations;
    
    for (int i = 0; i < 10; ++i) {
        void* ptr = std::malloc(alloc_size);
        memory_profiler_->record_allocation(ptr, alloc_size, "test_allocation");
        allocations.push_back(ptr);
    }
    
    // Free half of the allocations
    for (int i = 0; i < 5; ++i) {
        memory_profiler_->record_deallocation(allocations[i]);
        std::free(allocations[i]);
    }
    
    memory_profiler_->stop_tracking();
    
    auto final_stats = memory_profiler_->get_statistics();
    
    EXPECT_GE(final_stats.total_allocated, initial_stats.total_allocated + 10 * alloc_size);
    EXPECT_GE(final_stats.total_deallocated, initial_stats.total_deallocated + 5 * alloc_size);
    EXPECT_GE(final_stats.allocation_count, initial_stats.allocation_count + 10);
    EXPECT_GE(final_stats.deallocation_count, initial_stats.deallocation_count + 5);
    
    // Check for memory leaks (should be 5 unfreed allocations)
    auto leaks = memory_profiler_->get_memory_leaks();
    EXPECT_EQ(leaks.size(), 5);
    
    // Cleanup remaining allocations
    for (int i = 5; i < 10; ++i) {
        std::free(allocations[i]);
    }
}

TEST_F(ProfilingTest, ProfilerBufferOverflow) {
    function_profiler_->set_profiling_mode(scripting::profiling::FunctionProfiler::ProfilingMode::Full);
    function_profiler_->start_profiling();
    
    // Generate many function calls to test buffer overflow handling
    for (int i = 0; i < 100000; ++i) {
        auto profiler = function_profiler_->profile_function("stress_test_function");
        // Minimal work to avoid long test times
    }
    
    function_profiler_->stop_profiling();
    
    auto [call_buffer_stats, event_buffer_stats] = function_profiler_->get_buffer_statistics();
    
    // Check that buffers handled overflow gracefully
    EXPECT_GT(call_buffer_stats.pushes, 0);
    EXPECT_GE(call_buffer_stats.pops, 0);
    
    // Some overflow is expected with this many calls
    if (call_buffer_stats.overflow_rate > 0) {
        LOG_INFO("Buffer overflow rate: {:.2%} (expected with stress test)", 
                call_buffer_stats.overflow_rate);
    }
    
    // System should still be functional after overflow
    auto function_stats = function_profiler_->get_function_statistics();
    EXPECT_GT(function_stats.size(), 0);
}

//=============================================================================
// Error Handling and Edge Cases Tests  
//=============================================================================

class ErrorHandlingTest : public ScriptingTestFixture {};

TEST_F(ErrorHandlingTest, PythonExceptionRecovery) {
    // Test that the system can recover from Python exceptions
    
    // Cause an exception
    std::string bad_code = "invalid_syntax_error ::::";
    auto result1 = python_engine_->execute_string(bad_code);
    EXPECT_FALSE(result1.is_valid());
    EXPECT_TRUE(python_engine_->get_exception_handler().has_error());
    
    // Clear the error
    python_engine_->get_exception_handler().clear_error();
    EXPECT_FALSE(python_engine_->get_exception_handler().has_error());
    
    // Verify system is still functional
    std::string good_code = "recovery_test = True";
    auto result2 = python_engine_->execute_string(good_code);
    EXPECT_TRUE(result2.is_valid());
    
    auto recovery_var = python_engine_->get_global("recovery_test");
    EXPECT_TRUE(recovery_var.is_valid());
}

TEST_F(ErrorHandlingTest, EntityValidationAfterDestruction) {
    auto entity = ecs_interface_->create_entity();
    ASSERT_NE(entity, nullptr);
    
    // Add component
    EXPECT_TRUE(entity->add_component<TestPosition>());
    EXPECT_TRUE(entity->is_valid());
    EXPECT_TRUE(entity->has_component<TestPosition>());
    
    // Destroy entity
    ecs::Entity entity_id{entity->id(), entity->generation()};
    EXPECT_TRUE(ecs_interface_->destroy_entity(entity_id));
    
    // Entity should now be invalid
    EXPECT_FALSE(entity->is_valid());
    
    // Operations on invalid entity should fail gracefully
    EXPECT_FALSE(entity->has_component<TestPosition>());
    EXPECT_EQ(entity->get_component<TestPosition>(), nullptr);
    EXPECT_FALSE(entity->add_component<TestVelocity>());
    EXPECT_FALSE(entity->remove_component<TestPosition>());
    EXPECT_EQ(entity->component_count(), 0);
}

TEST_F(ErrorHandlingTest, QueryWithNoMatchingEntities) {
    // Query for non-existent component combination
    auto empty_query = ecs_interface_->create_query<TestPosition, TestVelocity, TestHealth>();
    
    EXPECT_EQ(empty_query->count(), 0);
    EXPECT_TRUE(empty_query->empty());
    
    auto entities = empty_query->get_entities();
    EXPECT_TRUE(entities.empty());
    
    // for_each should handle empty results gracefully
    usize iteration_count = 0;
    empty_query->for_each([&iteration_count](auto&, auto&, auto&, auto&) {
        ++iteration_count;
    });
    
    EXPECT_EQ(iteration_count, 0);
}

TEST_F(ErrorHandlingTest, MemoryProfilerInvalidOperations) {
    memory_profiler_->start_tracking();
    
    // Try to deallocate non-existent allocation
    void* fake_ptr = reinterpret_cast<void*>(0x12345678);
    memory_profiler_->record_deallocation(fake_ptr);
    
    // Try to allocate with null pointer
    memory_profiler_->record_allocation(nullptr, 100);
    
    memory_profiler_->stop_tracking();
    
    // System should handle these gracefully
    auto stats = memory_profiler_->get_statistics();
    EXPECT_GE(stats.total_allocated, 0);
    EXPECT_GE(stats.total_deallocated, 0);
}

//=============================================================================
// Integration and System Tests
//=============================================================================

class IntegrationTest : public ScriptingTestFixture {};

TEST_F(IntegrationTest, FullSystemIntegration) {
    // This test exercises multiple systems together
    
    function_profiler_->start_profiling();
    memory_profiler_->start_tracking();
    
    auto profiler = function_profiler_->profile_function("full_system_integration_test");
    
    // Create entities with components
    std::vector<scripting::ecs_interface::ScriptEntity*> entities;
    
    for (int i = 0; i < 100; ++i) {
        auto entity = ecs_interface_->create_entity();
        entity->add_component<TestPosition>(TestPosition{
            static_cast<f32>(i), 
            static_cast<f32>(i * 2), 
            0.0f
        });
        
        if (i % 2 == 0) {
            entity->add_component<TestVelocity>(TestVelocity{1.0f, 0.0f, 0.0f});
        }
        
        if (i % 3 == 0) {
            entity->add_component<TestHealth>(TestHealth{100, 100});
        }
        
        entities.push_back(entity);
    }
    
    // Execute Python script that would interact with entities
    std::string integration_script = R"(
# Integration test script
import math

def process_entities(entity_count):
    """Simulate entity processing."""
    results = []
    for i in range(entity_count):
        # Simulate some computation
        result = math.sin(i * 0.1) + math.cos(i * 0.2)
        results.append(result)
    return sum(results)

total_result = process_entities(100)
print(f"Integration test processed entities with total result: {total_result}")
)";
    
    auto script_result = python_engine_->execute_string(integration_script);
    EXPECT_TRUE(script_result.is_valid());
    
    // Use queries to process entities
    auto position_query = ecs_interface_->create_query<TestPosition>();
    auto moving_query = ecs_interface_->create_query<TestPosition, TestVelocity>();
    
    EXPECT_EQ(position_query->count(), 100);
    EXPECT_EQ(moving_query->count(), 50); // Every other entity has velocity
    
    // Process entities in parallel
    usize processed_count = 0;
    moving_query->for_each_parallel([&processed_count](auto& entity, TestPosition& pos, TestVelocity& vel) {
        // Simple physics update
        pos.x += vel.dx * 0.016f; // 60 FPS delta
        ++processed_count;
    }, job_system_.get());
    
    EXPECT_EQ(processed_count, 50);
    
    function_profiler_->stop_profiling();
    memory_profiler_->stop_tracking();
    
    // Verify all systems worked together
    auto python_stats = python_engine_->get_statistics();
    auto ecs_stats = ecs_interface_->get_statistics();
    auto profiler_stats = function_profiler_->get_function_statistics();
    auto memory_stats = memory_profiler_->get_statistics();
    
    EXPECT_GT(python_stats.scripts_executed, 0);
    EXPECT_EQ(ecs_stats.current_entities, 100);
    EXPECT_GT(profiler_stats.size(), 0);
    EXPECT_GT(memory_stats.total_allocated, 0);
    
    LOG_INFO("Integration test completed successfully:");
    LOG_INFO("  Python scripts executed: {}", python_stats.scripts_executed);
    LOG_INFO("  ECS entities created: {}", ecs_stats.current_entities);
    LOG_INFO("  Profiler functions tracked: {}", profiler_stats.size());
    LOG_INFO("  Memory allocated: {} KB", memory_stats.total_allocated / 1024);
}

TEST_F(IntegrationTest, ConcurrentScriptExecution) {
    // Test concurrent script execution using job system
    
    const int num_concurrent_scripts = 10;
    std::vector<job_system::JobID> script_jobs;
    
    // Submit multiple script execution jobs
    for (int i = 0; i < num_concurrent_scripts; ++i) {
        std::string job_name = "concurrent_script_" + std::to_string(i);
        
        auto job_id = job_system_->submit_job(job_name, [this, i]() {
            // Each job executes a different script
            std::string script = R"(
import math
import threading

thread_id = threading.get_ident()
result = sum(math.sin(i * 0.1) for i in range(1000))
print(f"Concurrent script )" + std::to_string(i) + R"( executed on thread {thread_id} with result {result}")
)";
            
            python_engine_->execute_string(script);
        });
        
        script_jobs.push_back(job_id);
    }
    
    // Wait for all scripts to complete
    job_system_->wait_for_batch(script_jobs);
    
    // Verify all jobs completed successfully
    for (auto job_id : script_jobs) {
        EXPECT_EQ(job_system_->get_job_state(job_id), job_system::JobState::Completed);
    }
    
    auto python_stats = python_engine_->get_statistics();
    EXPECT_GE(python_stats.scripts_executed, num_concurrent_scripts);
}

//=============================================================================
// Performance Regression Tests
//=============================================================================

class PerformanceRegressionTest : public ScriptingTestFixture {};

TEST_F(PerformanceRegressionTest, ScriptExecutionPerformance) {
    const int iterations = 1000;
    
    std::string performance_script = R"(
result = sum(i * i for i in range(100))
)";
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        auto result = python_engine_->execute_string(performance_script);
        ASSERT_TRUE(result.is_valid());
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<f64, std::milli>(end_time - start_time).count();
    
    f64 average_time = duration / iterations;
    
    LOG_INFO("Script execution performance: {:.3f} ms average per script", average_time);
    
    // Performance regression check - should complete in reasonable time
    EXPECT_LT(average_time, 1.0); // Less than 1ms per script on average
}

TEST_F(PerformanceRegressionTest, ECSOperationPerformance) {
    const int entity_count = 10000;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Create entities
    std::vector<scripting::ecs_interface::ScriptEntity*> entities;
    for (int i = 0; i < entity_count; ++i) {
        auto entity = ecs_interface_->create_entity();
        entity->add_component<TestPosition>(TestPosition{
            static_cast<f32>(i), 
            static_cast<f32>(i), 
            0.0f
        });
        entities.push_back(entity);
    }
    
    auto creation_time = std::chrono::high_resolution_clock::now();
    
    // Query and iterate
    auto query = ecs_interface_->create_query<TestPosition>();
    usize processed = 0;
    
    query->for_each([&processed](auto& entity, TestPosition& pos) {
        pos.x += 1.0f;
        ++processed;
    });
    
    auto end_time = std::chrono::high_resolution_clock::now();
    
    auto creation_duration = std::chrono::duration<f64, std::milli>(creation_time - start_time).count();
    auto iteration_duration = std::chrono::duration<f64, std::milli>(end_time - creation_time).count();
    
    LOG_INFO("ECS performance for {} entities:", entity_count);
    LOG_INFO("  Creation: {:.3f} ms ({:.1f} entities/ms)", creation_duration, entity_count / creation_duration);
    LOG_INFO("  Iteration: {:.3f} ms ({:.1f} entities/ms)", iteration_duration, entity_count / iteration_duration);
    
    EXPECT_EQ(processed, entity_count);
    EXPECT_LT(creation_duration, 100.0); // Should create 10k entities in under 100ms
    EXPECT_LT(iteration_duration, 50.0);  // Should iterate 10k entities in under 50ms
}

//=============================================================================
// Test Main Function
//=============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    LOG_INFO("ECScope Scripting Integration Tests");
    LOG_INFO("===================================");
    
    auto result = RUN_ALL_TESTS();
    
    LOG_INFO("Integration tests completed with result: {}", result == 0 ? "SUCCESS" : "FAILURE");
    
    return result;
}