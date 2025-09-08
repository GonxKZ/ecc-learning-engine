/**
 * @file examples/physics_debug_integration_tests.cpp
 * @brief Physics Debug Integration Test Suite - Comprehensive Validation Framework
 * 
 * This test suite provides comprehensive validation of the physics debug rendering integration,
 * ensuring correctness, performance, and educational value. It serves as both a quality
 * assurance tool and educational resource for understanding system integration testing.
 * 
 * Educational Objectives:
 * - Learn comprehensive system integration testing approaches
 * - Understand performance validation methodologies
 * - Explore memory safety and resource management testing
 * - Analyze correctness verification in complex integrated systems
 * - Study educational feature validation and user experience testing
 * 
 * Test Categories:
 * - Unit tests for individual components
 * - Integration tests for system coordination
 * - Performance regression tests
 * - Memory safety and leak detection tests
 * - Educational feature validation tests
 * - Stress tests with extreme scenarios
 * 
 * @author ECScope Educational ECS Framework - Integration Test Suite
 * @date 2024
 */

#include "../src/ecs/registry.hpp"
#include "../src/physics/physics_system.hpp"
#include "../src/physics/debug_integration_system.hpp"
#include "../src/physics/debug_renderer_2d.hpp"
#include "../src/physics/components/debug_components.hpp"
#include "../src/renderer/renderer_2d.hpp"
#include "../src/renderer/batch_renderer.hpp"
#include "../src/core/log.hpp"

#include <cassert>
#include <memory>
#include <vector>
#include <chrono>
#include <random>
#include <algorithm>
#include <thread>

using namespace ecscope;
using namespace ecscope::physics;
using namespace ecscope::physics::debug;
using namespace ecscope::renderer;
using namespace ecscope::ecs;

//=============================================================================
// Test Framework Infrastructure
//=============================================================================

class TestResult {
private:
    std::string test_name_;
    bool passed_;
    std::string failure_message_;
    std::chrono::high_resolution_clock::time_point start_time_;
    f64 execution_time_ms_;

public:
    explicit TestResult(const std::string& name) 
        : test_name_(name), passed_(true), execution_time_ms_(0.0) {
        start_time_ = std::chrono::high_resolution_clock::now();
    }
    
    ~TestResult() {
        auto end_time = std::chrono::high_resolution_clock::now();
        execution_time_ms_ = std::chrono::duration<f64, std::milli>(end_time - start_time_).count();
    }
    
    void pass() { passed_ = true; }
    
    void fail(const std::string& message) {
        passed_ = false;
        failure_message_ = message;
    }
    
    bool passed() const { return passed_; }
    const std::string& name() const { return test_name_; }
    const std::string& failure_message() const { return failure_message_; }
    f64 execution_time() const { return execution_time_ms_; }
};

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            result.fail(std::string("Assertion failed: ") + (message) + " at line " + std::to_string(__LINE__)); \
            return result; \
        } \
    } while(0)

#define TEST_ASSERT_NEAR(actual, expected, tolerance, message) \
    do { \
        if (std::abs((actual) - (expected)) > (tolerance)) { \
            result.fail(std::string("Assertion failed: ") + (message) + \
                       " (expected: " + std::to_string(expected) + \
                       ", actual: " + std::to_string(actual) + \
                       ", tolerance: " + std::to_string(tolerance) + ")" + \
                       " at line " + std::to_string(__LINE__)); \
            return result; \
        } \
    } while(0)

//=============================================================================
// Test Suite Base Class
//=============================================================================

class PhysicsDebugIntegrationTestSuite {
protected:
    std::unique_ptr<Registry> registry_;
    std::unique_ptr<PhysicsSystem> physics_system_;
    std::unique_ptr<Renderer2D> renderer_2d_;
    std::unique_ptr<BatchRenderer> batch_renderer_;
    std::unique_ptr<PhysicsDebugIntegrationSystem> debug_integration_;
    
    std::vector<TestResult> test_results_;
    u32 total_tests_;
    u32 passed_tests_;

public:
    PhysicsDebugIntegrationTestSuite() : total_tests_(0), passed_tests_(0) {
        setup_test_environment();
    }
    
    virtual ~PhysicsDebugIntegrationTestSuite() {
        cleanup_test_environment();
    }
    
    /** @brief Run all tests in the suite */
    void run_all_tests() {
        LOG_INFO("=== Physics Debug Integration Test Suite ===");
        LOG_INFO("Running comprehensive integration validation tests");
        
        // Component tests
        run_component_tests();
        
        // Integration tests
        run_integration_tests();
        
        // Performance tests
        run_performance_tests();
        
        // Memory safety tests
        run_memory_safety_tests();
        
        // Educational feature tests
        run_educational_feature_tests();
        
        // Stress tests
        run_stress_tests();
        
        // Report results
        report_test_results();
    }

protected:
    void setup_test_environment() {
        LOG_DEBUG("Setting up test environment...");
        
        // Create test systems
        registry_ = std::make_unique<Registry>();
        
        PhysicsSystemConfig physics_config = PhysicsSystemConfig::create_educational();
        physics_system_ = std::make_unique<PhysicsSystem>(*registry_, physics_config);
        
        renderer_2d_ = std::make_unique<Renderer2D>();
        batch_renderer_ = std::make_unique<BatchRenderer>();
        
        PhysicsDebugIntegrationSystem::Config debug_config = 
            PhysicsDebugIntegrationSystem::Config::create_educational();
        debug_integration_ = std::make_unique<PhysicsDebugIntegrationSystem>(
            *registry_, *physics_system_, *renderer_2d_, *batch_renderer_, debug_config);
        
        // Initialize systems
        physics_system_->initialize();
        debug_integration_->initialize();
        
        LOG_DEBUG("Test environment setup complete");
    }
    
    void cleanup_test_environment() {
        if (debug_integration_) debug_integration_->cleanup();
        if (physics_system_) physics_system_->cleanup();
        
        LOG_DEBUG("Test environment cleaned up");
    }
    
    void record_test_result(TestResult&& result) {
        bool passed = result.passed();
        total_tests_++;
        if (passed) passed_tests_++;
        
        LOG_INFO("Test '{}': {} ({:.3f} ms)", 
                result.name(), 
                passed ? "PASSED" : "FAILED",
                result.execution_time());
        
        if (!passed) {
            LOG_ERROR("  Failure: {}", result.failure_message());
        }
        
        test_results_.push_back(std::move(result));
    }
    
    //-------------------------------------------------------------------------
    // Component Tests
    //-------------------------------------------------------------------------
    
    void run_component_tests() {
        LOG_INFO("\n--- Component Tests ---");
        
        record_test_result(test_debug_visualization_component());
        record_test_result(test_debug_shape_component());
        record_test_result(test_debug_stats_component());
        record_test_result(test_component_relationships());
    }
    
    TestResult test_debug_visualization_component() {
        TestResult result("PhysicsDebugVisualization Component");
        
        // Test construction
        PhysicsDebugVisualization debug_viz;
        TEST_ASSERT(debug_viz.visualization_flags.flags == 0, "Default flags should be zero");
        TEST_ASSERT(debug_viz.is_valid(), "Default debug visualization should be valid");
        
        // Test basic configuration
        debug_viz = PhysicsDebugVisualization::create_basic();
        TEST_ASSERT(debug_viz.visualization_flags.show_collision_shape == 1, "Basic config should show collision shapes");
        TEST_ASSERT(debug_viz.visualization_flags.show_velocity_vector == 1, "Basic config should show velocity vectors");
        
        // Test educational configuration
        debug_viz = PhysicsDebugVisualization::create_educational();
        TEST_ASSERT(debug_viz.educational_info.show_physics_equations, "Educational config should show equations");
        TEST_ASSERT(debug_viz.educational_info.show_numerical_values, "Educational config should show values");
        
        // Test flag manipulation
        debug_viz.enable_visualization(1 << 5); // Enable bounding box
        TEST_ASSERT(debug_viz.is_visualization_enabled(1 << 5), "Flag should be enabled");
        
        debug_viz.disable_visualization(1 << 5);
        TEST_ASSERT(!debug_viz.is_visualization_enabled(1 << 5), "Flag should be disabled");
        
        // Test performance tracking
        debug_viz.debug_performance.update_stats(2.5f, 10);
        TEST_ASSERT(debug_viz.debug_performance.frames_visualized == 1, "Frame count should increment");
        TEST_ASSERT_NEAR(debug_viz.debug_performance.average_render_time, 2.5f, 0.01f, "Average time should be updated");
        
        result.pass();
        return result;
    }
    
    TestResult test_debug_shape_component() {
        TestResult result("PhysicsDebugShape Component");
        
        // Test circle shape creation
        Vec2 center{10.0f, 20.0f};
        f32 radius = 15.0f;
        Color color = Color::red();
        
        auto circle_shape = PhysicsDebugShape::create_circle(center, radius, color, true);
        TEST_ASSERT(circle_shape.primary_shape_type == PhysicsDebugShape::ShapeType::Circle, "Shape type should be Circle");
        TEST_ASSERT(circle_shape.geometry.circle.radius == radius, "Radius should match");
        TEST_ASSERT(circle_shape.render_props.color == color, "Color should match");
        TEST_ASSERT(circle_shape.render_props.filled, "Shape should be filled");
        TEST_ASSERT(circle_shape.is_valid(), "Circle shape should be valid");
        
        // Test rectangle shape creation
        Vec2 min{0.0f, 0.0f};
        Vec2 max{50.0f, 30.0f};
        
        auto rect_shape = PhysicsDebugShape::create_rectangle(min, max, Color::blue(), false);
        TEST_ASSERT(rect_shape.primary_shape_type == PhysicsDebugShape::ShapeType::Rectangle, "Shape type should be Rectangle");
        TEST_ASSERT(!rect_shape.render_props.filled, "Rectangle should not be filled");
        TEST_ASSERT(rect_shape.is_valid(), "Rectangle shape should be valid");
        
        // Test polygon creation
        std::vector<Vec2> vertices = {{0.0f, 0.0f}, {10.0f, 0.0f}, {5.0f, 10.0f}};
        auto poly_shape = PhysicsDebugShape::create_polygon(vertices, Color::green());
        TEST_ASSERT(poly_shape.primary_shape_type == PhysicsDebugShape::ShapeType::Polygon, "Shape type should be Polygon");
        TEST_ASSERT(poly_shape.polygon_vertex_count == 3, "Should have 3 vertices");
        TEST_ASSERT(poly_shape.is_valid(), "Polygon should be valid");
        
        // Test additional shapes
        PhysicsDebugShape::GeometryData line_geom;
        line_geom.line.start = Vec2{0.0f, 0.0f};
        line_geom.line.end = Vec2{100.0f, 100.0f};
        
        PhysicsDebugShape::RenderProperties line_props;
        line_props.color = Color::white();
        line_props.thickness = 2.0f;
        
        bool added = rect_shape.add_additional_shape(PhysicsDebugShape::ShapeType::Line, line_geom, line_props);
        TEST_ASSERT(added, "Should be able to add additional shape");
        TEST_ASSERT(rect_shape.additional_shape_count == 1, "Should have one additional shape");
        TEST_ASSERT(rect_shape.get_total_shape_count() == 2, "Should have 2 total shapes");
        
        // Test render complexity calculation
        f32 complexity = rect_shape.get_render_complexity();
        TEST_ASSERT(complexity > 1.0f, "Multi-shape complexity should be greater than base");
        
        result.pass();
        return result;
    }
    
    TestResult test_debug_stats_component() {
        TestResult result("PhysicsDebugStats Component");
        
        PhysicsDebugStats stats;
        
        // Test initial state
        TEST_ASSERT(stats.current_frame.debug_render_time_ms == 0.0f, "Initial render time should be zero");
        TEST_ASSERT(stats.accumulated_stats.total_frames_with_debug == 0, "Initial frame count should be zero");
        
        // Test frame update
        stats.update_frame_stats(5.2f, 0.8f, 15, 3, 60, 2);
        TEST_ASSERT(stats.current_frame.debug_render_time_ms == 5.2f, "Current frame time should be updated");
        TEST_ASSERT(stats.current_frame.debug_shapes_rendered == 15, "Shapes count should be updated");
        TEST_ASSERT(stats.accumulated_stats.total_frames_with_debug == 1, "Frame count should increment");
        
        // Test statistics calculation
        stats.current_frame.debug_batches_created = 2;
        stats.current_frame.debug_shapes_rendered = 15;
        stats.current_frame.update_batching_efficiency();
        
        f32 expected_efficiency = 1.0f / 2.0f; // 1 ideal batch / 2 actual batches
        TEST_ASSERT_NEAR(stats.current_frame.batching_efficiency, expected_efficiency, 0.01f, 
                        "Batching efficiency should be calculated correctly");
        
        // Test memory reporting
        stats.memory_stats.debug_geometry_memory = 1024;
        stats.memory_stats.debug_vertex_memory = 2048;
        stats.memory_stats.debug_component_memory = 512;
        stats.memory_stats.update();
        
        TEST_ASSERT(stats.memory_stats.total_debug_memory == 3584, "Total memory should be sum of components");
        
        auto memory_report = stats.memory_stats.get_memory_report();
        TEST_ASSERT_NEAR(memory_report.total_mb, 3584.0f / (1024.0f * 1024.0f), 0.01f, "Memory MB should be correct");
        
        // Test report generation
        std::string report = stats.generate_statistics_report();
        TEST_ASSERT(!report.empty(), "Statistics report should not be empty");
        TEST_ASSERT(report.find("Physics Debug Rendering Statistics") != std::string::npos, 
                   "Report should contain expected header");
        
        result.pass();
        return result;
    }
    
    TestResult test_component_relationships() {
        TestResult result("Component Relationships");
        
        // Create test entity with physics components
        Entity entity = registry_->create();
        
        Transform transform;
        transform.position = Vec2{100.0f, 200.0f};
        registry_->add_component(entity, transform);
        
        RigidBody2D rigidbody;
        rigidbody.mass = 5.0f;
        rigidbody.velocity = Vec2{10.0f, -5.0f};
        registry_->add_component(entity, rigidbody);
        
        // Add debug visualization
        bool added = debug_integration_->add_debug_visualization(entity);
        TEST_ASSERT(added, "Should be able to add debug visualization to physics entity");
        
        // Verify components were added
        TEST_ASSERT(registry_->has_component<PhysicsDebugVisualization>(entity), 
                   "Entity should have debug visualization component");
        TEST_ASSERT(registry_->has_component<PhysicsDebugShape>(entity), 
                   "Entity should have debug shape component");
        
        // Test component data consistency
        auto* debug_viz = registry_->get_component<PhysicsDebugVisualization>(entity);
        TEST_ASSERT(debug_viz != nullptr, "Debug visualization component should exist");
        TEST_ASSERT(debug_viz->is_valid(), "Debug visualization should be valid");
        
        // Test component removal
        debug_integration_->remove_debug_visualization(entity);
        TEST_ASSERT(!registry_->has_component<PhysicsDebugVisualization>(entity), 
                   "Debug visualization component should be removed");
        TEST_ASSERT(!registry_->has_component<PhysicsDebugShape>(entity), 
                   "Debug shape component should be removed");
        
        registry_->destroy(entity);
        
        result.pass();
        return result;
    }
    
    //-------------------------------------------------------------------------
    // Integration Tests
    //-------------------------------------------------------------------------
    
    void run_integration_tests() {
        LOG_INFO("\n--- Integration Tests ---");
        
        record_test_result(test_physics_debug_integration());
        record_test_result(test_rendering_pipeline_integration());
        record_test_result(test_ecs_system_coordination());
        record_test_result(test_debug_data_flow());
    }
    
    TestResult test_physics_debug_integration() {
        TestResult result("Physics-Debug Integration");
        
        // Create physics entities
        std::vector<Entity> entities;
        for (int i = 0; i < 10; ++i) {
            Entity entity = create_test_physics_entity(Vec2{i * 20.0f, 100.0f}, 10.0f, 2.0f);
            entities.push_back(entity);
        }
        
        // Add debug visualization
        debug_integration_->auto_add_debug_visualization();
        
        // Verify all entities have debug components
        for (Entity entity : entities) {
            TEST_ASSERT(registry_->has_component<PhysicsDebugVisualization>(entity),
                       "Entity should have debug visualization after auto-add");
        }
        
        // Test debug state synchronization
        debug_integration_->set_debug_enabled(true);
        TEST_ASSERT(debug_integration_->is_debug_enabled(), "Debug should be enabled");
        
        // Simulate physics and debug updates
        for (int frame = 0; frame < 30; ++frame) {
            physics_system_->update(1.0f / 60.0f);
            debug_integration_->update(1.0f / 60.0f);
        }
        
        // Verify integration statistics
        auto integration_stats = debug_integration_->get_integration_statistics();
        TEST_ASSERT(integration_stats.total_updates > 0, "Integration should have updates");
        TEST_ASSERT(integration_stats.active_debug_entities == entities.size(),
                   "All entities should be active for debug");
        
        // Clean up
        for (Entity entity : entities) {
            registry_->destroy(entity);
        }
        
        result.pass();
        return result;
    }
    
    TestResult test_rendering_pipeline_integration() {
        TestResult result("Rendering Pipeline Integration");
        
        // Create test entity with debug visualization
        Entity entity = create_test_physics_entity(Vec2{0.0f, 0.0f}, 15.0f, 3.0f);
        debug_integration_->add_debug_visualization(entity);
        
        // Test debug shape generation
        auto* debug_shape = registry_->get_component<PhysicsDebugShape>(entity);
        TEST_ASSERT(debug_shape != nullptr, "Entity should have debug shape component");
        
        // Update systems to generate debug data
        physics_system_->update(1.0f / 60.0f);
        debug_integration_->update(1.0f / 60.0f);
        
        // Verify debug shape is valid and renderable
        TEST_ASSERT(debug_shape->is_valid(), "Debug shape should be valid after update");
        TEST_ASSERT(debug_shape->render_props.visible, "Debug shape should be visible");
        
        // Test different visualization modes
        auto* debug_viz = registry_->get_component<PhysicsDebugVisualization>(entity);
        debug_viz->enable_visualization(0xFFFF); // Enable all visualizations
        
        debug_integration_->update(1.0f / 60.0f);
        
        // Verify complex visualization doesn't break rendering
        f32 complexity = debug_shape->get_render_complexity();
        TEST_ASSERT(complexity > 1.0f, "Complex visualization should have higher complexity");
        
        registry_->destroy(entity);
        
        result.pass();
        return result;
    }
    
    TestResult test_ecs_system_coordination() {
        TestResult result("ECS System Coordination");
        
        // Test system initialization and dependencies
        TEST_ASSERT(physics_system_->get_config().enable_component_visualization,
                   "Physics system should support visualization");
        
        // Create entities and test system coordination
        std::vector<Entity> entities;
        for (int i = 0; i < 5; ++i) {
            Entity entity = create_test_physics_entity(
                Vec2{i * 30.0f, 50.0f}, 8.0f + i, 1.0f + i * 0.5f);
            entities.push_back(entity);
        }
        
        debug_integration_->auto_add_debug_visualization();
        
        // Test coordinated updates
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (int frame = 0; frame < 60; ++frame) { // 1 second at 60 FPS
            // Update in proper order
            physics_system_->update(1.0f / 60.0f);
            debug_integration_->update(1.0f / 60.0f);
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        f64 total_time = std::chrono::duration<f64, std::milli>(end_time - start_time).count();
        
        // Verify reasonable performance
        f64 average_frame_time = total_time / 60.0;
        TEST_ASSERT(average_frame_time < 16.67, "System coordination should maintain 60 FPS");
        
        // Test component consistency after updates
        for (Entity entity : entities) {
            auto* debug_viz = registry_->get_component<PhysicsDebugVisualization>(entity);
            auto* debug_shape = registry_->get_component<PhysicsDebugShape>(entity);
            
            TEST_ASSERT(debug_viz != nullptr, "Debug visualization should persist");
            TEST_ASSERT(debug_shape != nullptr, "Debug shape should persist");
            TEST_ASSERT(debug_viz->is_valid(), "Debug visualization should remain valid");
            TEST_ASSERT(debug_shape->is_valid(), "Debug shape should remain valid");
        }
        
        // Clean up
        for (Entity entity : entities) {
            registry_->destroy(entity);
        }
        
        result.pass();
        return result;
    }
    
    TestResult test_debug_data_flow() {
        TestResult result("Debug Data Flow");
        
        // Create physics entity with specific properties
        Entity entity = create_test_physics_entity(Vec2{100.0f, 100.0f}, 12.0f, 4.0f);
        
        // Add comprehensive debug visualization
        debug_integration_->add_debug_visualization(entity, 
            PhysicsDebugVisualization::create_educational());
        
        // Apply forces to create interesting debug data
        physics_system_->apply_force(entity, Vec2{50.0f, 100.0f});
        physics_system_->apply_impulse(entity, Vec2{-20.0f, 30.0f});
        
        // Update to process physics and debug data
        physics_system_->update(1.0f / 60.0f);
        debug_integration_->update(1.0f / 60.0f);
        
        // Verify debug data reflects physics state
        auto* debug_viz = registry_->get_component<PhysicsDebugVisualization>(entity);
        auto* rigidbody = registry_->get_component<RigidBody2D>(entity);
        
        TEST_ASSERT(debug_viz != nullptr && rigidbody != nullptr, "Components should exist");
        
        // Check that debug cache was updated
        TEST_ASSERT(debug_viz->debug_cache.velocity_cache_valid, "Velocity cache should be valid");
        
        // Verify debug cache data matches physics data
        Vec2 cached_velocity = debug_viz->debug_cache.cached_velocity;
        Vec2 physics_velocity = rigidbody->velocity;
        
        TEST_ASSERT_NEAR(cached_velocity.x, physics_velocity.x, 0.01f, "Cached velocity X should match");
        TEST_ASSERT_NEAR(cached_velocity.y, physics_velocity.y, 0.01f, "Cached velocity Y should match");
        
        // Test debug shape reflects physics state
        auto* debug_shape = registry_->get_component<PhysicsDebugShape>(entity);
        auto* transform = registry_->get_component<Transform>(entity);
        
        if (debug_shape->primary_shape_type == PhysicsDebugShape::ShapeType::Circle) {
            Vec2 shape_center = debug_shape->geometry.circle.center;
            Vec2 transform_pos = transform->position;
            
            TEST_ASSERT_NEAR(shape_center.x, transform_pos.x, 0.01f, "Debug shape position should match transform");
            TEST_ASSERT_NEAR(shape_center.y, transform_pos.y, 0.01f, "Debug shape position should match transform");
        }
        
        registry_->destroy(entity);
        
        result.pass();
        return result;
    }
    
    //-------------------------------------------------------------------------
    // Performance Tests
    //-------------------------------------------------------------------------
    
    void run_performance_tests() {
        LOG_INFO("\n--- Performance Tests ---");
        
        record_test_result(test_debug_rendering_performance());
        record_test_result(test_memory_efficiency());
        record_test_result(test_batching_effectiveness());
        record_test_result(test_scalability_limits());
    }
    
    TestResult test_debug_rendering_performance() {
        TestResult result("Debug Rendering Performance");
        
        // Create moderate number of entities for performance test
        std::vector<Entity> entities;
        for (int i = 0; i < 100; ++i) {
            Vec2 pos{(i % 10) * 20.0f, (i / 10) * 25.0f};
            Entity entity = create_test_physics_entity(pos, 8.0f, 2.0f);
            entities.push_back(entity);
        }
        
        debug_integration_->auto_add_debug_visualization();
        
        // Measure baseline physics performance
        auto baseline_start = std::chrono::high_resolution_clock::now();
        for (int frame = 0; frame < 120; ++frame) {
            physics_system_->update(1.0f / 60.0f);
        }
        auto baseline_end = std::chrono::high_resolution_clock::now();
        f64 baseline_time = std::chrono::duration<f64, std::milli>(baseline_end - baseline_start).count();
        
        // Measure physics + debug performance
        auto debug_start = std::chrono::high_resolution_clock::now();
        for (int frame = 0; frame < 120; ++frame) {
            physics_system_->update(1.0f / 60.0f);
            debug_integration_->update(1.0f / 60.0f);
        }
        auto debug_end = std::chrono::high_resolution_clock::now();
        f64 debug_time = std::chrono::duration<f64, std::milli>(debug_end - debug_start).count();
        
        // Calculate overhead
        f64 overhead_percentage = ((debug_time - baseline_time) / baseline_time) * 100.0;
        
        // Verify reasonable performance overhead
        TEST_ASSERT(overhead_percentage < 100.0, "Debug rendering overhead should be less than 100%");
        TEST_ASSERT(debug_time / 120.0 < 16.67, "Average frame time should maintain 60 FPS");
        
        LOG_DEBUG("Performance test results: Baseline {:.3f}ms, Debug {:.3f}ms, Overhead {:.1f}%",
                 baseline_time / 120.0, debug_time / 120.0, overhead_percentage);
        
        // Clean up
        for (Entity entity : entities) {
            registry_->destroy(entity);
        }
        
        result.pass();
        return result;
    }
    
    TestResult test_memory_efficiency() {
        TestResult result("Memory Efficiency");
        
        // Get initial memory baseline
        auto initial_stats = debug_integration_->get_integration_statistics();
        usize initial_memory = initial_stats.debug_memory_used;
        
        // Create entities and measure memory growth
        std::vector<Entity> entities;
        std::vector<usize> memory_measurements;
        
        for (int batch = 0; batch < 5; ++batch) {
            // Add 20 entities per batch
            for (int i = 0; i < 20; ++i) {
                Entity entity = create_test_physics_entity(
                    Vec2{batch * 100.0f + i * 5.0f, 100.0f}, 10.0f, 2.0f);
                entities.push_back(entity);
            }
            
            debug_integration_->auto_add_debug_visualization();
            
            // Update once to process new entities
            physics_system_->update(1.0f / 60.0f);
            debug_integration_->update(1.0f / 60.0f);
            
            auto current_stats = debug_integration_->get_integration_statistics();
            memory_measurements.push_back(current_stats.debug_memory_used);
        }
        
        // Verify memory scales reasonably with entity count
        TEST_ASSERT(memory_measurements.size() == 5, "Should have 5 memory measurements");
        
        for (usize i = 1; i < memory_measurements.size(); ++i) {
            TEST_ASSERT(memory_measurements[i] >= memory_measurements[i-1],
                       "Memory should not decrease when adding entities");
            
            // Memory growth should be reasonable (not exponential)
            f64 growth_factor = static_cast<f64>(memory_measurements[i]) / memory_measurements[i-1];
            TEST_ASSERT(growth_factor < 2.0, "Memory growth should be reasonable");
        }
        
        // Test memory cleanup
        for (Entity entity : entities) {
            debug_integration_->remove_debug_visualization(entity);
            registry_->destroy(entity);
        }
        
        // Update to process cleanup
        debug_integration_->update(1.0f / 60.0f);
        
        auto final_stats = debug_integration_->get_integration_statistics();
        usize final_memory = final_stats.debug_memory_used;
        
        // Memory should return close to initial level
        f64 memory_retention = static_cast<f64>(final_memory) / std::max(1UL, initial_memory);
        TEST_ASSERT(memory_retention < 1.5, "Memory retention should be reasonable after cleanup");
        
        result.pass();
        return result;
    }
    
    TestResult test_batching_effectiveness() {
        TestResult result("Batching Effectiveness");
        
        // Create many entities to test batching
        std::vector<Entity> entities;
        for (int i = 0; i < 200; ++i) {
            Vec2 pos{(i % 20) * 15.0f, (i / 20) * 20.0f};
            Entity entity = create_test_physics_entity(pos, 8.0f, 1.5f);
            entities.push_back(entity);
        }
        
        debug_integration_->auto_add_debug_visualization();
        
        // Update to generate debug rendering data
        for (int frame = 0; frame < 10; ++frame) {
            physics_system_->update(1.0f / 60.0f);
            debug_integration_->update(1.0f / 60.0f);
        }
        
        // Check batching statistics
        auto integration_stats = debug_integration_->get_integration_statistics();
        
        TEST_ASSERT(integration_stats.debug_shapes_rendered > 0, "Should have rendered debug shapes");
        TEST_ASSERT(integration_stats.batching_efficiency > 0.0f, "Should have batching efficiency data");
        
        // Verify reasonable batching effectiveness
        TEST_ASSERT(integration_stats.batching_efficiency > 0.5f, 
                   "Batching efficiency should be reasonable");
        
        // Verify batch count is reasonable (not one batch per shape)
        u32 shapes = integration_stats.debug_shapes_rendered;
        u32 batches = integration_stats.debug_batches_generated;
        
        if (batches > 0) {
            f32 shapes_per_batch = static_cast<f32>(shapes) / batches;
            TEST_ASSERT(shapes_per_batch > 1.0f, "Should have multiple shapes per batch on average");
        }
        
        // Clean up
        for (Entity entity : entities) {
            registry_->destroy(entity);
        }
        
        result.pass();
        return result;
    }
    
    TestResult test_scalability_limits() {
        TestResult result("Scalability Limits");
        
        // Test performance at different scales
        std::vector<u32> test_scales = {50, 100, 200, 500};
        std::vector<f64> frame_times;
        
        for (u32 entity_count : test_scales) {
            LOG_DEBUG("Testing scalability with {} entities", entity_count);
            
            // Create entities
            std::vector<Entity> entities;
            for (u32 i = 0; i < entity_count; ++i) {
                Vec2 pos{(i % 25) * 12.0f, (i / 25) * 15.0f};
                Entity entity = create_test_physics_entity(pos, 6.0f + (i % 5), 1.0f + (i % 3));
                entities.push_back(entity);
            }
            
            debug_integration_->auto_add_debug_visualization();
            
            // Measure performance
            auto start_time = std::chrono::high_resolution_clock::now();
            
            for (int frame = 0; frame < 60; ++frame) {
                physics_system_->update(1.0f / 60.0f);
                debug_integration_->update(1.0f / 60.0f);
            }
            
            auto end_time = std::chrono::high_resolution_clock::now();
            f64 total_time = std::chrono::duration<f64, std::milli>(end_time - start_time).count();
            f64 avg_frame_time = total_time / 60.0;
            
            frame_times.push_back(avg_frame_time);
            
            LOG_DEBUG("  {} entities: {:.3f} ms per frame", entity_count, avg_frame_time);
            
            // Clean up for next test
            for (Entity entity : entities) {
                registry_->destroy(entity);
            }
        }
        
        // Verify performance doesn't degrade too dramatically
        TEST_ASSERT(frame_times.size() == test_scales.size(), "Should have frame time for each scale");
        
        for (usize i = 1; i < frame_times.size(); ++i) {
            f64 scale_ratio = static_cast<f64>(test_scales[i]) / test_scales[i-1];
            f64 time_ratio = frame_times[i] / frame_times[i-1];
            
            // Performance degradation should be sub-quadratic
            TEST_ASSERT(time_ratio < scale_ratio * scale_ratio, 
                       "Performance should not degrade quadratically");
        }
        
        // Verify reasonable absolute performance for largest scale
        TEST_ASSERT(frame_times.back() < 33.33, "Should maintain at least 30 FPS with 500 entities");
        
        result.pass();
        return result;
    }
    
    //-------------------------------------------------------------------------
    // Memory Safety Tests
    //-------------------------------------------------------------------------
    
    void run_memory_safety_tests() {
        LOG_INFO("\n--- Memory Safety Tests ---");
        
        record_test_result(test_component_lifecycle_safety());
        record_test_result(test_system_shutdown_safety());
        record_test_result(test_memory_leak_detection());
    }
    
    TestResult test_component_lifecycle_safety() {
        TestResult result("Component Lifecycle Safety");
        
        std::vector<Entity> entities;
        
        // Create and destroy entities multiple times
        for (int cycle = 0; cycle < 10; ++cycle) {
            // Create entities
            for (int i = 0; i < 20; ++i) {
                Entity entity = create_test_physics_entity(
                    Vec2{i * 10.0f, cycle * 15.0f}, 8.0f, 2.0f);
                entities.push_back(entity);
            }
            
            debug_integration_->auto_add_debug_visualization();
            
            // Update systems
            physics_system_->update(1.0f / 60.0f);
            debug_integration_->update(1.0f / 60.0f);
            
            // Remove debug visualization from half the entities
            for (usize i = 0; i < entities.size() / 2; ++i) {
                debug_integration_->remove_debug_visualization(entities[i]);
            }
            
            // Update again
            physics_system_->update(1.0f / 60.0f);
            debug_integration_->update(1.0f / 60.0f);
            
            // Destroy all entities
            for (Entity entity : entities) {
                registry_->destroy(entity);
            }
            entities.clear();
            
            // Update to process destruction
            debug_integration_->update(1.0f / 60.0f);
        }
        
        // Verify system is still in valid state
        auto final_stats = debug_integration_->get_integration_statistics();
        TEST_ASSERT(final_stats.active_debug_entities == 0, 
                   "Should have no active debug entities after cleanup");
        
        result.pass();
        return result;
    }
    
    TestResult test_system_shutdown_safety() {
        TestResult result("System Shutdown Safety");
        
        // Create entities with debug visualization
        std::vector<Entity> entities;
        for (int i = 0; i < 50; ++i) {
            Entity entity = create_test_physics_entity(Vec2{i * 8.0f, 100.0f}, 10.0f, 2.0f);
            entities.push_back(entity);
        }
        
        debug_integration_->auto_add_debug_visualization();
        
        // Update systems
        for (int frame = 0; frame < 30; ++frame) {
            physics_system_->update(1.0f / 60.0f);
            debug_integration_->update(1.0f / 60.0f);
        }
        
        // Shutdown debug integration while entities still exist
        debug_integration_->cleanup();
        
        // Physics system should still function
        for (int frame = 0; frame < 10; ++frame) {
            physics_system_->update(1.0f / 60.0f);
        }
        
        // Clean up entities
        for (Entity entity : entities) {
            registry_->destroy(entity);
        }
        
        // Re-initialize debug integration
        debug_integration_->initialize();
        
        // Verify system can be reinitialized successfully
        Entity test_entity = create_test_physics_entity(Vec2{0.0f, 0.0f}, 15.0f, 3.0f);
        bool added = debug_integration_->add_debug_visualization(test_entity);
        TEST_ASSERT(added, "Should be able to add debug visualization after re-initialization");
        
        registry_->destroy(test_entity);
        
        result.pass();
        return result;
    }
    
    TestResult test_memory_leak_detection() {
        TestResult result("Memory Leak Detection");
        
        // Get baseline memory usage
        auto initial_stats = debug_integration_->get_integration_statistics();
        usize baseline_memory = initial_stats.debug_memory_used;
        
        // Create and destroy entities in batches
        for (int batch = 0; batch < 20; ++batch) {
            std::vector<Entity> entities;
            
            // Create entities
            for (int i = 0; i < 25; ++i) {
                Entity entity = create_test_physics_entity(
                    Vec2{i * 12.0f, batch * 20.0f}, 9.0f, 2.5f);
                entities.push_back(entity);
            }
            
            debug_integration_->auto_add_debug_visualization();
            
            // Update systems
            for (int frame = 0; frame < 5; ++frame) {
                physics_system_->update(1.0f / 60.0f);
                debug_integration_->update(1.0f / 60.0f);
            }
            
            // Destroy entities
            for (Entity entity : entities) {
                debug_integration_->remove_debug_visualization(entity);
                registry_->destroy(entity);
            }
            
            // Update to process cleanup
            for (int frame = 0; frame < 2; ++frame) {
                debug_integration_->update(1.0f / 60.0f);
            }
        }
        
        // Check final memory usage
        auto final_stats = debug_integration_->get_integration_statistics();
        usize final_memory = final_stats.debug_memory_used;
        
        // Memory should not have grown significantly
        f64 memory_growth_factor = static_cast<f64>(final_memory) / std::max(1UL, baseline_memory);
        TEST_ASSERT(memory_growth_factor < 2.0, "Memory should not grow significantly after cleanup cycles");
        
        result.pass();
        return result;
    }
    
    //-------------------------------------------------------------------------
    // Educational Feature Tests
    //-------------------------------------------------------------------------
    
    void run_educational_feature_tests() {
        LOG_INFO("\n--- Educational Feature Tests ---");
        
        record_test_result(test_educational_mode_features());
        record_test_result(test_performance_analysis_accuracy());
        record_test_result(test_debug_visualization_correctness());
    }
    
    TestResult test_educational_mode_features() {
        TestResult result("Educational Mode Features");
        
        // Enable educational mode
        debug_integration_->set_educational_mode(true);
        
        // Create entity with educational debug visualization
        Entity entity = create_test_physics_entity(Vec2{50.0f, 150.0f}, 12.0f, 3.0f);
        debug_integration_->add_debug_visualization(entity, 
            PhysicsDebugVisualization::create_educational());
        
        // Verify educational features are enabled
        auto* debug_viz = registry_->get_component<PhysicsDebugVisualization>(entity);
        TEST_ASSERT(debug_viz != nullptr, "Entity should have debug visualization");
        TEST_ASSERT(debug_viz->educational_info.show_physics_equations, 
                   "Educational mode should show physics equations");
        TEST_ASSERT(debug_viz->educational_info.show_numerical_values,
                   "Educational mode should show numerical values");
        
        // Apply forces and update to generate educational data
        physics_system_->apply_force(entity, Vec2{100.0f, 50.0f});
        
        for (int frame = 0; frame < 60; ++frame) {
            physics_system_->update(1.0f / 60.0f);
            debug_integration_->update(1.0f / 60.0f);
        }
        
        // Check if debug stats component was added for educational analysis
        auto* debug_stats = registry_->get_component<PhysicsDebugStats>(entity);
        TEST_ASSERT(debug_stats != nullptr, "Educational mode should add debug stats");
        
        // Verify stats are being collected
        TEST_ASSERT(debug_stats->accumulated_stats.total_frames_with_debug > 0,
                   "Stats should be collected in educational mode");
        
        // Test educational analysis generation
        std::string report = debug_stats->generate_statistics_report();
        TEST_ASSERT(!report.empty(), "Educational report should not be empty");
        TEST_ASSERT(report.find("Educational Analysis") != std::string::npos,
                   "Report should contain educational analysis");
        
        registry_->destroy(entity);
        
        result.pass();
        return result;
    }
    
    TestResult test_performance_analysis_accuracy() {
        TestResult result("Performance Analysis Accuracy");
        
        // Create entities for performance testing
        std::vector<Entity> entities;
        for (int i = 0; i < 30; ++i) {
            Entity entity = create_test_physics_entity(
                Vec2{i * 15.0f, 100.0f}, 10.0f, 2.0f);
            entities.push_back(entity);
        }
        
        debug_integration_->auto_add_debug_visualization();
        
        // Run simulation to generate performance data
        for (int frame = 0; frame < 120; ++frame) {
            physics_system_->update(1.0f / 60.0f);
            debug_integration_->update(1.0f / 60.0f);
        }
        
        // Get integration statistics
        auto integration_stats = debug_integration_->get_integration_statistics();
        
        // Verify statistics are reasonable
        TEST_ASSERT(integration_stats.total_updates > 0, "Should have update statistics");
        TEST_ASSERT(integration_stats.average_update_time > 0.0f, "Should have timing data");
        TEST_ASSERT(integration_stats.integration_efficiency > 0.0f, "Should calculate efficiency");
        TEST_ASSERT(integration_stats.integration_efficiency <= 1.0f, "Efficiency should be normalized");
        
        // Verify entity tracking accuracy
        TEST_ASSERT(integration_stats.active_debug_entities == entities.size(),
                   "Should accurately track active debug entities");
        
        // Test performance rating classification
        const char* rating = integration_stats.performance_rating;
        TEST_ASSERT(rating != nullptr, "Should have performance rating");
        TEST_ASSERT(strlen(rating) > 0, "Performance rating should not be empty");
        
        // Verify reasonable performance values
        TEST_ASSERT(integration_stats.average_update_time < 100.0f, 
                   "Average update time should be reasonable");
        
        // Clean up
        for (Entity entity : entities) {
            registry_->destroy(entity);
        }
        
        result.pass();
        return result;
    }
    
    TestResult test_debug_visualization_correctness() {
        TestResult result("Debug Visualization Correctness");
        
        // Create entity with known physics properties
        Entity entity = create_test_physics_entity(Vec2{100.0f, 200.0f}, 15.0f, 5.0f);
        debug_integration_->add_debug_visualization(entity, 
            PhysicsDebugVisualization::create_comprehensive());
        
        // Apply known forces
        Vec2 test_force{75.0f, -25.0f};
        physics_system_->apply_force(entity, test_force);
        
        // Update systems
        physics_system_->update(1.0f / 60.0f);
        debug_integration_->update(1.0f / 60.0f);
        
        // Verify debug visualization reflects physics state
        auto* rigidbody = registry_->get_component<RigidBody2D>(entity);
        auto* transform = registry_->get_component<Transform>(entity);
        auto* debug_viz = registry_->get_component<PhysicsDebugVisualization>(entity);
        auto* debug_shape = registry_->get_component<PhysicsDebugShape>(entity);
        
        TEST_ASSERT(rigidbody && transform && debug_viz && debug_shape, 
                   "All required components should exist");
        
        // Check position synchronization
        if (debug_shape->primary_shape_type == PhysicsDebugShape::ShapeType::Circle) {
            Vec2 shape_pos = debug_shape->geometry.circle.center;
            Vec2 transform_pos = transform->position;
            
            TEST_ASSERT_NEAR(shape_pos.x, transform_pos.x, 0.1f, 
                           "Debug shape position should match transform position");
            TEST_ASSERT_NEAR(shape_pos.y, transform_pos.y, 0.1f,
                           "Debug shape position should match transform position");
        }
        
        // Check velocity caching
        if (debug_viz->debug_cache.velocity_cache_valid) {
            Vec2 cached_vel = debug_viz->debug_cache.cached_velocity;
            Vec2 physics_vel = rigidbody->velocity;
            
            TEST_ASSERT_NEAR(cached_vel.x, physics_vel.x, 0.01f,
                           "Cached velocity should match physics velocity");
            TEST_ASSERT_NEAR(cached_vel.y, physics_vel.y, 0.01f,
                           "Cached velocity should match physics velocity");
        }
        
        // Test visualization flag functionality
        debug_viz->enable_visualization(1 << 10); // Enable trajectory
        TEST_ASSERT(debug_viz->is_visualization_enabled(1 << 10), 
                   "Visualization flag should be enabled");
        
        debug_viz->disable_visualization(1 << 10);
        TEST_ASSERT(!debug_viz->is_visualization_enabled(1 << 10), 
                   "Visualization flag should be disabled");
        
        registry_->destroy(entity);
        
        result.pass();
        return result;
    }
    
    //-------------------------------------------------------------------------
    // Stress Tests
    //-------------------------------------------------------------------------
    
    void run_stress_tests() {
        LOG_INFO("\n--- Stress Tests ---");
        
        record_test_result(test_high_entity_count_stress());
        record_test_result(test_rapid_creation_destruction_stress());
        record_test_result(test_extreme_physics_conditions_stress());
    }
    
    TestResult test_high_entity_count_stress() {
        TestResult result("High Entity Count Stress");
        
        LOG_DEBUG("Creating 1000 entities for stress test...");
        
        std::vector<Entity> entities;
        
        // Create many entities
        for (int i = 0; i < 1000; ++i) {
            Vec2 pos{(i % 50) * 8.0f, (i / 50) * 10.0f};
            Entity entity = create_test_physics_entity(pos, 5.0f + (i % 3), 1.0f + (i % 2));
            entities.push_back(entity);
            
            // Add debug visualization every 10th entity to test mixed scenarios
            if (i % 10 == 0) {
                debug_integration_->add_debug_visualization(entity);
            }
        }
        
        LOG_DEBUG("Running stress simulation...");
        
        // Run simulation for extended period
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (int frame = 0; frame < 300; ++frame) { // 5 seconds at 60 FPS
            physics_system_->update(1.0f / 60.0f);
            debug_integration_->update(1.0f / 60.0f);
            
            // Log progress occasionally
            if (frame % 60 == 0) {
                LOG_DEBUG("  Stress test frame {}/300", frame);
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        f64 total_time = std::chrono::duration<f64, std::milli>(end_time - start_time).count();
        f64 avg_frame_time = total_time / 300.0;
        
        // Verify system remained stable
        auto final_stats = debug_integration_->get_integration_statistics();
        TEST_ASSERT(final_stats.total_updates > 0, "System should have processed updates");
        
        // Performance should be reasonable even under stress
        TEST_ASSERT(avg_frame_time < 50.0f, "Should maintain reasonable performance under stress");
        
        LOG_DEBUG("Stress test completed: {:.3f} ms average frame time", avg_frame_time);
        
        // Clean up
        for (Entity entity : entities) {
            registry_->destroy(entity);
        }
        
        result.pass();
        return result;
    }
    
    TestResult test_rapid_creation_destruction_stress() {
        TestResult result("Rapid Creation/Destruction Stress");
        
        // Rapidly create and destroy entities
        for (int cycle = 0; cycle < 100; ++cycle) {
            std::vector<Entity> entities;
            
            // Create batch of entities
            for (int i = 0; i < 20; ++i) {
                Entity entity = create_test_physics_entity(
                    Vec2{i * 15.0f, cycle * 5.0f}, 8.0f, 2.0f);
                entities.push_back(entity);
            }
            
            // Add debug visualization
            debug_integration_->auto_add_debug_visualization();
            
            // Update once
            physics_system_->update(1.0f / 60.0f);
            debug_integration_->update(1.0f / 60.0f);
            
            // Destroy all entities
            for (Entity entity : entities) {
                registry_->destroy(entity);
            }
            
            // Update to process destruction
            debug_integration_->update(1.0f / 60.0f);
        }
        
        // System should remain stable
        auto final_stats = debug_integration_->get_integration_statistics();
        TEST_ASSERT(final_stats.active_debug_entities == 0, 
                   "Should have no active entities after stress test");
        
        result.pass();
        return result;
    }
    
    TestResult test_extreme_physics_conditions_stress() {
        TestResult result("Extreme Physics Conditions Stress");
        
        // Create entities with extreme properties
        std::vector<Entity> entities;
        
        // Very large entities
        Entity large_entity = create_test_physics_entity(Vec2{0.0f, 0.0f}, 1000.0f, 100.0f);
        entities.push_back(large_entity);
        
        // Very small entities
        Entity small_entity = create_test_physics_entity(Vec2{100.0f, 0.0f}, 0.1f, 0.01f);
        entities.push_back(small_entity);
        
        // Very fast entities
        Entity fast_entity = create_test_physics_entity(Vec2{200.0f, 0.0f}, 10.0f, 2.0f);
        auto* fast_rb = registry_->get_component<RigidBody2D>(fast_entity);
        fast_rb->velocity = Vec2{1000.0f, 500.0f};
        entities.push_back(fast_entity);
        
        // Add debug visualization
        for (Entity entity : entities) {
            debug_integration_->add_debug_visualization(entity, 
                PhysicsDebugVisualization::create_educational());
        }
        
        // Apply extreme forces
        physics_system_->apply_force(large_entity, Vec2{10000.0f, -5000.0f});
        physics_system_->apply_force(small_entity, Vec2{-1.0f, 2.0f});
        
        // Run simulation with extreme conditions
        bool system_remained_stable = true;
        
        try {
            for (int frame = 0; frame < 60; ++frame) {
                physics_system_->update(1.0f / 60.0f);
                debug_integration_->update(1.0f / 60.0f);
                
                // Check that debug components remain valid
                for (Entity entity : entities) {
                    auto* debug_viz = registry_->get_component<PhysicsDebugVisualization>(entity);
                    auto* debug_shape = registry_->get_component<PhysicsDebugShape>(entity);
                    
                    if (debug_viz && !debug_viz->is_valid()) {
                        system_remained_stable = false;
                        break;
                    }
                    
                    if (debug_shape && !debug_shape->is_valid()) {
                        system_remained_stable = false;
                        break;
                    }
                }
                
                if (!system_remained_stable) break;
            }
        } catch (const std::exception& e) {
            system_remained_stable = false;
            LOG_WARN("Exception during extreme conditions test: {}", e.what());
        }
        
        TEST_ASSERT(system_remained_stable, "System should remain stable under extreme conditions");
        
        // Clean up
        for (Entity entity : entities) {
            registry_->destroy(entity);
        }
        
        result.pass();
        return result;
    }
    
    //-------------------------------------------------------------------------
    // Utility Methods
    //-------------------------------------------------------------------------
    
    Entity create_test_physics_entity(const Vec2& position, f32 radius, f32 mass) {
        Entity entity = registry_->create();
        
        // Add Transform component
        Transform transform;
        transform.position = position;
        transform.scale = Vec2{1.0f, 1.0f};
        registry_->add_component(entity, transform);
        
        // Add RigidBody2D component
        RigidBody2D rigidbody;
        rigidbody.mass = mass;
        rigidbody.body_type = RigidBodyType::Dynamic;
        rigidbody.velocity = Vec2{0.0f, 0.0f};
        registry_->add_component(entity, rigidbody);
        
        // Add Collider2D component
        Collider2D collider;
        collider.shape = Circle{Vec2{0.0f, 0.0f}, radius};
        collider.material = PhysicsMaterial::create_default();
        registry_->add_component(entity, collider);
        
        // Add ForceAccumulator component
        ForceAccumulator forces;
        registry_->add_component(entity, forces);
        
        // Add to physics system
        physics_system_->add_physics_entity(entity);
        
        return entity;
    }
    
    void report_test_results() {
        LOG_INFO("\n=== Test Results Summary ===");
        LOG_INFO("Tests passed: {}/{}", passed_tests_, total_tests_);
        
        f32 pass_percentage = (static_cast<f32>(passed_tests_) / total_tests_) * 100.0f;
        LOG_INFO("Pass rate: {:.1f}%", pass_percentage);
        
        // Calculate total execution time
        f64 total_time = 0.0;
        for (const auto& result : test_results_) {
            total_time += result.execution_time();
        }
        LOG_INFO("Total execution time: {:.3f} ms", total_time);
        
        // Report failed tests
        if (passed_tests_ < total_tests_) {
            LOG_ERROR("\nFailed tests:");
            for (const auto& result : test_results_) {
                if (!result.passed()) {
                    LOG_ERROR("  - {}: {}", result.name(), result.failure_message());
                }
            }
        } else {
            LOG_INFO("All tests passed! Physics debug integration is working correctly.");
        }
        
        // Educational insights
        LOG_INFO("\n=== Educational Insights ===");
        LOG_INFO("- Comprehensive testing validates system integration correctness");
        LOG_INFO("- Performance tests ensure real-time constraints are met");
        LOG_INFO("- Memory safety tests prevent resource leaks and crashes");
        LOG_INFO("- Stress tests validate system robustness under extreme conditions");
        LOG_INFO("- Educational feature tests ensure learning objectives are met");
    }
};

//=============================================================================
// Main Test Entry Point
//=============================================================================

int main() {
    try {
        LOG_INFO("ECScope Physics Debug Integration Test Suite");
        LOG_INFO("Comprehensive validation of physics debug rendering integration");
        
        // Run test suite
        PhysicsDebugIntegrationTestSuite test_suite;
        test_suite.run_all_tests();
        
        return 0;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Test suite failed with exception: {}", e.what());
        return 1;
    }
}