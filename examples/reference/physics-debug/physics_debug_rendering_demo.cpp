/**
 * @file examples/physics_debug_rendering_demo.cpp
 * @brief Physics Debug Rendering Integration Demo - Educational Showcase
 * 
 * This demo showcases the seamless integration between ECScope's physics simulation
 * and modern 2D rendering pipeline for comprehensive debug visualization. It demonstrates
 * world-class integration patterns while providing educational insights into both
 * physics simulation and modern graphics programming.
 * 
 * Educational Objectives:
 * - Understand integration patterns between simulation and rendering systems
 * - Learn about component-based debug visualization architecture
 * - Explore performance optimization techniques for real-time debug rendering
 * - Analyze memory management patterns in integrated systems
 * - Compare different debug rendering approaches and their trade-offs
 * 
 * Key Demonstrations:
 * - Physics simulation with real-time debug visualization
 * - ECS component-based debug visualization management
 * - BatchRenderer integration for optimized debug shape rendering
 * - Interactive physics parameter tuning with immediate visual feedback
 * - Performance comparison between immediate and batched debug rendering
 * - Educational overlays showing physics concepts and mathematics
 * 
 * @author ECScope Educational ECS Framework - Physics Debug Integration Demo
 * @date 2024
 */

#include "../src/ecs/registry.hpp"
#include "../src/physics/physics_system.hpp"
#include "../src/physics/debug_integration_system.hpp"
#include "../src/physics/debug_renderer_2d.hpp"
#include "../src/renderer/renderer_2d.hpp"
#include "../src/renderer/batch_renderer.hpp"
#include "../src/ui/panels/panel_rendering_debug.hpp"
#include "../src/core/log.hpp"

#include <iostream>
#include <random>
#include <chrono>
#include <memory>

using namespace ecscope;
using namespace ecscope::physics;
using namespace ecscope::physics::debug;
using namespace ecscope::renderer;
using namespace ecscope::ecs;

//=============================================================================
// Demo Configuration
//=============================================================================

struct DemoConfig {
    // Scene configuration
    u32 num_physics_entities{50};          ///< Number of physics entities to create
    f32 world_width{800.0f};               ///< World width in units
    f32 world_height{600.0f};              ///< World height in units
    bool enable_gravity{true};              ///< Enable gravity simulation
    Vec2 gravity{0.0f, -9.81f};            ///< Gravity vector
    
    // Debug visualization configuration
    bool enable_debug_rendering{true};     ///< Enable debug rendering
    bool enable_educational_mode{true};    ///< Enable educational features
    bool enable_performance_analysis{true}; ///< Enable performance analysis
    bool enable_interactive_mode{false};   ///< Enable interactive manipulation
    
    // Rendering configuration
    bool enable_batching{true};             ///< Enable sprite batching
    bool enable_comparison_mode{false};    ///< Enable rendering comparison
    u32 max_sprites_per_batch{500};        ///< Sprites per batch limit
    
    // Educational features
    bool show_physics_equations{true};     ///< Show physics equations overlay
    bool show_performance_metrics{true};   ///< Show performance metrics
    bool show_memory_usage{true};          ///< Show memory usage information
    bool show_algorithm_breakdown{false};  ///< Show algorithm step breakdown
};

//=============================================================================
// Demo Scene Setup
//=============================================================================

class PhysicsDebugDemo {
private:
    // Core systems
    std::unique_ptr<Registry> registry_;
    std::unique_ptr<PhysicsSystem> physics_system_;
    std::unique_ptr<Renderer2D> renderer_2d_;
    std::unique_ptr<BatchRenderer> batch_renderer_;
    std::unique_ptr<PhysicsDebugIntegrationSystem> debug_integration_;
    
    // Demo state
    DemoConfig config_;
    std::vector<Entity> physics_entities_;
    bool demo_running_;
    f32 demo_time_;
    u32 demo_frame_;
    
    // Performance tracking
    std::chrono::high_resolution_clock::time_point frame_start_;
    f32 physics_time_;
    f32 rendering_time_;
    f32 debug_rendering_time_;

public:
    explicit PhysicsDebugDemo(const DemoConfig& config = DemoConfig{}) 
        : config_(config), demo_running_(true), demo_time_(0.0f), demo_frame_(0) {
        
        LOG_INFO("=== Physics Debug Rendering Integration Demo ===");
        LOG_INFO("Educational Objectives:");
        LOG_INFO("- Physics simulation with real-time debug visualization");
        LOG_INFO("- Component-based debug visualization architecture");
        LOG_INFO("- Performance optimization through batched rendering");
        LOG_INFO("- Interactive educational features");
        
        initialize_demo();
    }
    
    ~PhysicsDebugDemo() {
        cleanup_demo();
        
        LOG_INFO("=== Demo Session Summary ===");
        LOG_INFO("Total frames: {}", demo_frame_);
        LOG_INFO("Total demo time: {:.2f} seconds", demo_time_);
        if (debug_integration_) {
            LOG_INFO("\n{}", debug_integration_->generate_integration_report());
        }
    }
    
    //-------------------------------------------------------------------------
    // Demo Execution
    //-------------------------------------------------------------------------
    
    /** @brief Run the physics debug rendering demo */
    void run() {
        LOG_INFO("Starting physics debug rendering demo...");
        
        // Demo phases
        run_basic_physics_demo();
        run_debug_visualization_demo();
        run_performance_comparison_demo();
        run_educational_features_demo();
        run_interactive_demo();
        
        LOG_INFO("Physics debug rendering demo completed!");
    }
    
    //-------------------------------------------------------------------------
    // Demo Phases
    //-------------------------------------------------------------------------
    
    /** @brief Phase 1: Basic physics simulation */
    void run_basic_physics_demo() {
        LOG_INFO("\n--- Phase 1: Basic Physics Simulation ---");
        LOG_INFO("Demonstrating basic physics simulation without debug visualization");
        
        // Disable debug rendering temporarily
        debug_integration_->set_debug_enabled(false);
        
        // Run physics simulation for a few seconds
        simulate_demo_time(3.0f);
        
        LOG_INFO("Basic physics simulation phase completed");
        LOG_INFO("Average physics update time: {:.3f} ms", physics_time_ / demo_frame_);
    }
    
    /** @brief Phase 2: Debug visualization showcase */
    void run_debug_visualization_demo() {
        LOG_INFO("\n--- Phase 2: Debug Visualization Showcase ---");
        LOG_INFO("Enabling comprehensive debug visualization");
        
        // Enable debug rendering
        debug_integration_->set_debug_enabled(true);
        
        // Enable basic debug features
        debug_integration_->set_global_debug_flags(
            (1 << 0) |  // Collision shapes
            (1 << 1) |  // Velocity vectors
            (1 << 2) |  // Force vectors
            (1 << 3),   // Center of mass
            true
        );
        
        // Apply educational color scheme
        PhysicsDebugVisualization::ColorScheme educational_colors;
        educational_colors.collision_shape_color = Color::green();
        educational_colors.velocity_vector_color = Color::blue();
        educational_colors.force_vector_color = Color::red();
        educational_colors.center_of_mass_color = Color::yellow();
        debug_integration_->set_global_color_scheme(educational_colors);
        
        // Run with debug visualization
        simulate_demo_time(5.0f);
        
        LOG_INFO("Debug visualization phase completed");
        LOG_INFO("Average debug rendering time: {:.3f} ms", debug_rendering_time_ / demo_frame_);
        
        // Demonstrate different visualization modes
        demonstrate_visualization_modes();
    }
    
    /** @brief Phase 3: Performance comparison */
    void run_performance_comparison_demo() {
        LOG_INFO("\n--- Phase 3: Performance Comparison ---");
        LOG_INFO("Comparing different debug rendering approaches");
        
        // Test immediate mode rendering
        LOG_INFO("Testing immediate mode debug rendering...");
        auto immediate_config = PhysicsDebugIntegrationSystem::Config::create_performance();
        immediate_config.enable_batch_optimization = false;
        
        f32 immediate_time = benchmark_rendering_approach(immediate_config, 2.0f);
        
        // Test batched rendering
        LOG_INFO("Testing batched debug rendering...");
        auto batched_config = PhysicsDebugIntegrationSystem::Config::create_performance();
        batched_config.enable_batch_optimization = true;
        
        f32 batched_time = benchmark_rendering_approach(batched_config, 2.0f);
        
        // Report comparison results
        LOG_INFO("\n=== Performance Comparison Results ===");
        LOG_INFO("Immediate mode average time: {:.3f} ms", immediate_time);
        LOG_INFO("Batched mode average time: {:.3f} ms", batched_time);
        LOG_INFO("Performance improvement: {:.2f}x", immediate_time / batched_time);
        
        // Get comprehensive comparison from debug integration system
        if (debug_integration_) {
            auto comparison = debug_integration_->compare_integration_approaches();
            LOG_INFO("\n=== Integration Approach Comparison ===");
            LOG_INFO("Performance improvement ratio: {:.2f}x", comparison.performance_improvement_ratio);
            LOG_INFO("Memory efficiency improvement: {:.2f}x", comparison.memory_efficiency_improvement);
            LOG_INFO("Recommended approach: {}", comparison.recommended_approach);
        }
    }
    
    /** @brief Phase 4: Educational features demonstration */
    void run_educational_features_demo() {
        LOG_INFO("\n--- Phase 4: Educational Features ---");
        LOG_INFO("Demonstrating educational debug visualization features");
        
        // Enable educational mode
        debug_integration_->set_educational_mode(true);
        
        // Enable comprehensive debug visualization
        debug_integration_->set_global_debug_flags(0xFFFFFFFF, true); // Enable all flags
        
        // Create some interesting physics scenarios for education
        create_educational_scenarios();
        
        // Run educational simulation
        simulate_demo_time(4.0f);
        
        // Demonstrate physics concepts
        demonstrate_physics_concepts();
        
        LOG_INFO("Educational features phase completed");
    }
    
    /** @brief Phase 5: Interactive demonstration */
    void run_interactive_demo() {
        if (!config_.enable_interactive_mode) {
            LOG_INFO("\n--- Phase 5: Interactive Demo (Skipped - Interactive mode disabled) ---");
            return;
        }
        
        LOG_INFO("\n--- Phase 5: Interactive Demo ---");
        LOG_INFO("Demonstrating interactive physics manipulation");
        LOG_INFO("Note: This would normally include mouse/keyboard interaction");
        
        // Enable interactive features
        enable_interactive_features();
        
        // Simulate interactive session
        simulate_interactive_session();
        
        LOG_INFO("Interactive demo phase completed");
    }

private:
    //-------------------------------------------------------------------------
    // Initialization and Cleanup
    //-------------------------------------------------------------------------
    
    /** @brief Initialize the demo systems */
    void initialize_demo() {
        LOG_INFO("Initializing demo systems...");
        
        // Create ECS registry
        registry_ = std::make_unique<Registry>();
        
        // Create physics system
        PhysicsSystemConfig physics_config = PhysicsSystemConfig::create_educational();
        physics_config.world_config.gravity = config_.gravity;
        physics_config.enable_component_visualization = true;
        physics_system_ = std::make_unique<PhysicsSystem>(*registry_, physics_config);
        
        // Create rendering systems (placeholder initialization)
        renderer_2d_ = std::make_unique<Renderer2D>();
        batch_renderer_ = std::make_unique<BatchRenderer>();
        
        // Create debug integration system
        PhysicsDebugIntegrationSystem::Config debug_config = config_.enable_educational_mode ?
            PhysicsDebugIntegrationSystem::Config::create_educational() :
            PhysicsDebugIntegrationSystem::Config::create_performance();
        debug_config.enable_batch_optimization = config_.enable_batching;
        debug_config.enable_performance_analysis = config_.enable_performance_analysis;
        
        debug_integration_ = std::make_unique<PhysicsDebugIntegrationSystem>(
            *registry_, *physics_system_, *renderer_2d_, *batch_renderer_, debug_config);
        
        // Initialize systems
        physics_system_->initialize();
        debug_integration_->initialize();
        
        // Create demo scene
        create_demo_scene();
        
        LOG_INFO("Demo systems initialized successfully");
    }
    
    /** @brief Cleanup demo resources */
    void cleanup_demo() {
        if (debug_integration_) debug_integration_->cleanup();
        if (physics_system_) physics_system_->cleanup();
        
        physics_entities_.clear();
        
        LOG_DEBUG("Demo resources cleaned up");
    }
    
    //-------------------------------------------------------------------------
    // Scene Creation
    //-------------------------------------------------------------------------
    
    /** @brief Create the demo physics scene */
    void create_demo_scene() {
        LOG_INFO("Creating demo physics scene with {} entities", config_.num_physics_entities);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<f32> pos_dist_x(-config_.world_width * 0.4f, config_.world_width * 0.4f);
        std::uniform_real_distribution<f32> pos_dist_y(0.0f, config_.world_height * 0.8f);
        std::uniform_real_distribution<f32> size_dist(5.0f, 20.0f);
        std::uniform_real_distribution<f32> mass_dist(1.0f, 10.0f);
        
        // Create physics entities
        for (u32 i = 0; i < config_.num_physics_entities; ++i) {
            Entity entity = registry_->create();
            
            // Add Transform component
            Transform transform;
            transform.position.x = pos_dist_x(gen);
            transform.position.y = pos_dist_y(gen);
            transform.rotation = 0.0f;
            transform.scale = Vec2{1.0f, 1.0f};
            registry_->add_component(entity, transform);
            
            // Add RigidBody2D component
            RigidBody2D rigidbody;
            rigidbody.mass = mass_dist(gen);
            rigidbody.body_type = RigidBodyType::Dynamic;
            rigidbody.velocity = Vec2{0.0f, 0.0f};
            rigidbody.angular_velocity = 0.0f;
            registry_->add_component(entity, rigidbody);
            
            // Add Collider2D component (simple circle)
            Collider2D collider;
            f32 radius = size_dist(gen);
            collider.shape = Circle{Vec2{0.0f, 0.0f}, radius};
            collider.material = PhysicsMaterial::create_default();
            registry_->add_component(entity, collider);
            
            // Add ForceAccumulator component
            ForceAccumulator forces;
            registry_->add_component(entity, forces);
            
            // Add to physics system
            physics_system_->add_physics_entity(entity);
            
            physics_entities_.push_back(entity);
        }
        
        // Add debug visualization to all physics entities
        debug_integration_->auto_add_debug_visualization();
        
        // Create ground/walls for containment
        create_world_boundaries();
        
        LOG_INFO("Created {} physics entities with debug visualization", physics_entities_.size());
    }
    
    /** @brief Create world boundaries */
    void create_world_boundaries() {
        f32 wall_thickness = 10.0f;
        f32 half_width = config_.world_width * 0.5f;
        f32 half_height = config_.world_height * 0.5f;
        
        // Bottom wall (ground)
        Entity ground = create_static_box(
            Vec2{0.0f, -half_height - wall_thickness * 0.5f},
            Vec2{config_.world_width, wall_thickness}
        );
        
        // Left wall
        Entity left_wall = create_static_box(
            Vec2{-half_width - wall_thickness * 0.5f, 0.0f},
            Vec2{wall_thickness, config_.world_height}
        );
        
        // Right wall
        Entity right_wall = create_static_box(
            Vec2{half_width + wall_thickness * 0.5f, 0.0f},
            Vec2{wall_thickness, config_.world_height}
        );
        
        // Top wall (ceiling)
        Entity ceiling = create_static_box(
            Vec2{0.0f, half_height + wall_thickness * 0.5f},
            Vec2{config_.world_width, wall_thickness}
        );
        
        LOG_DEBUG("Created world boundaries");
    }
    
    /** @brief Create a static box entity */
    Entity create_static_box(const Vec2& position, const Vec2& size) {
        Entity entity = registry_->create();
        
        // Add Transform component
        Transform transform;
        transform.position = position;
        transform.scale = Vec2{1.0f, 1.0f};
        registry_->add_component(entity, transform);
        
        // Add RigidBody2D component (static)
        RigidBody2D rigidbody;
        rigidbody.body_type = RigidBodyType::Static;
        rigidbody.mass = 0.0f; // Infinite mass for static bodies
        registry_->add_component(entity, rigidbody);
        
        // Add Collider2D component (AABB)
        Collider2D collider;
        Vec2 half_size = size * 0.5f;
        collider.shape = AABB{position - half_size, position + half_size};
        collider.material = PhysicsMaterial::create_default();
        registry_->add_component(entity, collider);
        
        // Add to physics system
        physics_system_->add_physics_entity(entity);
        
        // Add debug visualization
        debug_integration_->add_debug_visualization(entity, 
            PhysicsDebugVisualization::create_basic());
        
        return entity;
    }
    
    //-------------------------------------------------------------------------
    // Demo Simulation
    //-------------------------------------------------------------------------
    
    /** @brief Simulate demo for specified time */
    void simulate_demo_time(f32 duration) {
        LOG_INFO("Simulating for {:.1f} seconds...", duration);
        
        const f32 dt = 1.0f / 60.0f; // 60 FPS
        f32 elapsed_time = 0.0f;
        u32 frame_count = 0;
        
        while (elapsed_time < duration) {
            frame_start_ = std::chrono::high_resolution_clock::now();
            
            // Update physics
            auto physics_start = std::chrono::high_resolution_clock::now();
            physics_system_->update(dt);
            auto physics_end = std::chrono::high_resolution_clock::now();
            physics_time_ += std::chrono::duration<f32, std::milli>(physics_end - physics_start).count();
            
            // Update debug integration
            auto debug_start = std::chrono::high_resolution_clock::now();
            debug_integration_->update(dt);
            auto debug_end = std::chrono::high_resolution_clock::now();
            debug_rendering_time_ += std::chrono::duration<f32, std::milli>(debug_end - debug_start).count();
            
            // Update demo state
            elapsed_time += dt;
            demo_time_ += dt;
            ++demo_frame_;
            ++frame_count;
            
            // Log progress every second
            if (frame_count % 60 == 0) {
                LOG_DEBUG("Demo time: {:.1f}s, Physics entities: {}", 
                         demo_time_, physics_entities_.size());
            }
        }
        
        LOG_INFO("Simulation completed - {} frames in {:.1f} seconds", frame_count, duration);
    }
    
    /** @brief Benchmark specific rendering approach */
    f32 benchmark_rendering_approach(const PhysicsDebugIntegrationSystem::Config& config, f32 duration) {
        // Create temporary debug integration system with specified config
        auto temp_debug_integration = std::make_unique<PhysicsDebugIntegrationSystem>(
            *registry_, *physics_system_, *renderer_2d_, *batch_renderer_, config);
        temp_debug_integration->initialize();
        
        // Measure performance
        const f32 dt = 1.0f / 60.0f;
        f32 elapsed_time = 0.0f;
        f32 total_debug_time = 0.0f;
        u32 frame_count = 0;
        
        while (elapsed_time < duration) {
            // Update physics
            physics_system_->update(dt);
            
            // Measure debug integration time
            auto debug_start = std::chrono::high_resolution_clock::now();
            temp_debug_integration->update(dt);
            auto debug_end = std::chrono::high_resolution_clock::now();
            
            total_debug_time += std::chrono::duration<f32, std::milli>(debug_end - debug_start).count();
            
            elapsed_time += dt;
            ++frame_count;
        }
        
        temp_debug_integration->cleanup();
        
        return total_debug_time / frame_count;
    }
    
    //-------------------------------------------------------------------------
    // Educational Demonstrations
    //-------------------------------------------------------------------------
    
    /** @brief Demonstrate different visualization modes */
    void demonstrate_visualization_modes() {
        LOG_INFO("\nDemonstrating different debug visualization modes:");
        
        // Mode 1: Collision shapes only
        LOG_INFO("Mode 1: Collision shapes only");
        debug_integration_->set_global_debug_flags(0xFFFFFFFF, false); // Clear all
        debug_integration_->set_global_debug_flags(1 << 0, true);      // Collision shapes only
        simulate_demo_time(1.5f);
        
        // Mode 2: Velocity vectors
        LOG_INFO("Mode 2: Collision shapes + velocity vectors");
        debug_integration_->set_global_debug_flags(1 << 1, true);      // Add velocity vectors
        simulate_demo_time(1.5f);
        
        // Mode 3: Forces and contacts
        LOG_INFO("Mode 3: Forces and contact visualization");
        debug_integration_->set_global_debug_flags((1 << 2) | (1 << 6) | (1 << 7), true);
        simulate_demo_time(1.5f);
        
        // Mode 4: Comprehensive visualization
        LOG_INFO("Mode 4: Comprehensive debug visualization");
        debug_integration_->set_global_debug_flags(0x0000FFFF, true); // Enable many features
        simulate_demo_time(2.0f);
    }
    
    /** @brief Create educational physics scenarios */
    void create_educational_scenarios() {
        LOG_INFO("Creating educational physics scenarios...");
        
        // Scenario 1: Pendulum system
        Entity pendulum_anchor = create_static_box(Vec2{-200.0f, 200.0f}, Vec2{10.0f, 10.0f});
        Entity pendulum_bob = create_dynamic_circle(Vec2{-200.0f, 150.0f}, 15.0f, 5.0f);
        // In a complete implementation, we would create a constraint between anchor and bob
        
        // Scenario 2: Newton's cradle setup
        for (int i = 0; i < 5; ++i) {
            f32 x = 50.0f + i * 25.0f;
            Entity ball = create_dynamic_circle(Vec2{x, 180.0f}, 12.0f, 2.0f);
            // In a complete implementation, we would create pendulum constraints
        }
        
        // Scenario 3: Collision demonstration
        Entity moving_ball = create_dynamic_circle(Vec2{-300.0f, 50.0f}, 20.0f, 10.0f);
        Entity stationary_ball = create_dynamic_circle(Vec2{300.0f, 50.0f}, 20.0f, 10.0f);
        
        // Give initial velocity to moving ball
        auto* rigidbody = registry_->get_component<RigidBody2D>(moving_ball);
        if (rigidbody) {
            rigidbody->velocity = Vec2{50.0f, 0.0f};
        }
        
        LOG_INFO("Educational scenarios created");
    }
    
    /** @brief Create dynamic circle entity */
    Entity create_dynamic_circle(const Vec2& position, f32 radius, f32 mass) {
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
        
        // Add comprehensive debug visualization
        debug_integration_->add_debug_visualization(entity, 
            PhysicsDebugVisualization::create_educational());
        
        return entity;
    }
    
    /** @brief Demonstrate physics concepts */
    void demonstrate_physics_concepts() {
        LOG_INFO("\nDemonstrating key physics concepts:");
        
        // Demonstrate energy conservation
        LOG_INFO("Concept 1: Energy Conservation");
        LOG_INFO("- Watch kinetic energy transfer during collisions");
        LOG_INFO("- Observe potential energy conversion during falls");
        
        // Demonstrate momentum conservation
        LOG_INFO("Concept 2: Momentum Conservation"); 
        LOG_INFO("- Total momentum is conserved in collisions");
        LOG_INFO("- Individual object momentum changes based on mass and velocity");
        
        // Demonstrate forces and acceleration
        LOG_INFO("Concept 3: Forces and Acceleration (F = ma)");
        LOG_INFO("- Gravity applies constant downward force");
        LOG_INFO("- Acceleration is proportional to force and inversely proportional to mass");
        
        // Let simulation run to show these concepts
        simulate_demo_time(3.0f);
    }
    
    /** @brief Enable interactive features */
    void enable_interactive_features() {
        // Enable interactive mode for all debug entities
        registry_->for_each<PhysicsDebugVisualization>(
            [](Entity entity, PhysicsDebugVisualization& debug_viz) {
                debug_viz.visualization_flags.interactive_mode = 1;
                debug_viz.interaction_settings.allow_drag_entity = true;
                debug_viz.interaction_settings.allow_force_application = true;
                debug_viz.interaction_settings.show_interaction_hints = true;
            });
        
        LOG_INFO("Interactive features enabled - entities can be manipulated");
    }
    
    /** @brief Simulate interactive session */
    void simulate_interactive_session() {
        LOG_INFO("Simulating interactive physics manipulation...");
        
        // Simulate applying forces to random entities
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<usize> entity_dist(0, physics_entities_.size() - 1);
        std::uniform_real_distribution<f32> force_dist(-100.0f, 100.0f);
        
        for (int interaction = 0; interaction < 5; ++interaction) {
            // Select random entity
            Entity target_entity = physics_entities_[entity_dist(gen)];
            
            // Apply random force
            Vec2 force{force_dist(gen), force_dist(gen)};
            physics_system_->apply_force(target_entity, force);
            
            LOG_INFO("Applied force ({:.1f}, {:.1f}) to entity {}", 
                    force.x, force.y, target_entity);
            
            // Simulate for a bit to see the effects
            simulate_demo_time(1.0f);
        }
        
        LOG_INFO("Interactive session simulation completed");
    }
};

//=============================================================================
// Performance Analysis Tutorial
//=============================================================================

class PerformanceAnalysisTutorial {
public:
    /** @brief Run performance analysis tutorial */
    static void run_performance_tutorial() {
        LOG_INFO("\n=== Performance Analysis Tutorial ===");
        LOG_INFO("Learning how to analyze and optimize physics debug rendering performance");
        
        // Create demo with performance focus
        DemoConfig perf_config;
        perf_config.num_physics_entities = 200; // More entities for performance testing
        perf_config.enable_performance_analysis = true;
        perf_config.enable_educational_mode = false;
        perf_config.show_performance_metrics = true;
        
        PhysicsDebugDemo perf_demo(perf_config);
        
        // Run performance-focused phases
        LOG_INFO("\nPhase 1: Baseline performance measurement");
        perf_demo.run_basic_physics_demo();
        
        LOG_INFO("\nPhase 2: Debug rendering impact analysis");
        perf_demo.run_debug_visualization_demo();
        
        LOG_INFO("\nPhase 3: Optimization comparison");
        perf_demo.run_performance_comparison_demo();
        
        LOG_INFO("\nPerformance tutorial completed!");
        LOG_INFO("Key takeaways:");
        LOG_INFO("- Debug rendering can significantly impact performance if not optimized");
        LOG_INFO("- Batching reduces draw calls and improves performance substantially");
        LOG_INFO("- Memory-efficient debug data structures reduce cache misses");
        LOG_INFO("- Educational features add overhead but provide valuable insights");
    }
};

//=============================================================================
// Integration Patterns Tutorial
//=============================================================================

class IntegrationPatternsTutorial {
public:
    /** @brief Run integration patterns tutorial */
    static void run_integration_tutorial() {
        LOG_INFO("\n=== System Integration Patterns Tutorial ===");
        LOG_INFO("Learning advanced patterns for integrating simulation and rendering systems");
        
        // Create educational demo configuration
        DemoConfig integration_config;
        integration_config.enable_educational_mode = true;
        integration_config.show_algorithm_breakdown = true;
        integration_config.enable_performance_analysis = true;
        
        PhysicsDebugDemo integration_demo(integration_config);
        
        LOG_INFO("\nKey Integration Patterns Demonstrated:");
        LOG_INFO("1. Component-based debug visualization architecture");
        LOG_INFO("2. System coordination and data flow management");
        LOG_INFO("3. Memory-efficient temporary data structures");
        LOG_INFO("4. Performance monitoring and optimization feedback loops");
        LOG_INFO("5. Educational feature integration without performance loss");
        
        // Run educational phases
        integration_demo.run_educational_features_demo();
        
        LOG_INFO("\nIntegration patterns tutorial completed!");
        LOG_INFO("These patterns can be applied to other engine system integrations");
    }
};

//=============================================================================
// Main Demo Entry Point
//=============================================================================

int main() {
    try {
        LOG_INFO("ECScope Physics Debug Rendering Integration Demo");
        LOG_INFO("Educational ECS Framework - Advanced System Integration");
        
        // Initialize logging
        // (In a real implementation, this would set up the logging system)
        
        // Run main demo
        LOG_INFO("\n=== Main Demo ===");
        DemoConfig main_config;
        main_config.enable_educational_mode = true;
        main_config.enable_performance_analysis = true;
        main_config.enable_interactive_mode = false; // Disable for automated demo
        
        PhysicsDebugDemo main_demo(main_config);
        main_demo.run();
        
        // Run specialized tutorials
        PerformanceAnalysisTutorial::run_performance_tutorial();
        IntegrationPatternsTutorial::run_integration_tutorial();
        
        LOG_INFO("\n=== Demo Session Complete ===");
        LOG_INFO("Key Learning Outcomes:");
        LOG_INFO("- Understanding of physics simulation and rendering integration");
        LOG_INFO("- Component-based debug visualization architecture");
        LOG_INFO("- Performance optimization techniques for real-time systems");
        LOG_INFO("- Educational system design without compromising performance");
        LOG_INFO("- Advanced ECS integration patterns");
        
        return 0;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Demo failed with exception: {}", e.what());
        return 1;
    }
}