/**
 * @file examples/physics_demo.cpp
 * @brief Physics System Demonstration for ECScope Phase 5: Física 2D
 * 
 * This example demonstrates the complete physics system implementation,
 * showcasing educational features, performance analysis, and interactive
 * physics simulation.
 * 
 * Features Demonstrated:
 * - Complete physics system setup and configuration
 * - Entity creation with different physics behaviors
 * - Educational step-by-step physics simulation
 * - Performance profiling and optimization analysis
 * - Interactive physics parameter tuning
 * - Comprehensive statistics and reporting
 * 
 * Educational Value:
 * Students can use this example to understand:
 * - How modern physics engines are structured
 * - The relationship between math, components, and systems
 * - Performance characteristics of different algorithms
 * - Memory management in real-time simulation
 * - Educational debugging and visualization techniques
 * 
 * Usage:
 * Compile and run this example to see the physics system in action.
 * Use keyboard inputs to interact with the simulation and explore
 * different educational features.
 * 
 * @author ECScope Educational ECS Framework - Phase 5: Física 2D
 * @date 2024
 */

#include "../src/physics/physics.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <ctime>

using namespace ecscope;
using namespace ecscope::physics;

/**
 * @brief Interactive Physics Demo Application
 * 
 * This class demonstrates the complete physics system with educational
 * features and interactive controls.
 */
class PhysicsDemo {
private:
    std::unique_ptr<ecs::Registry> registry_;
    std::unique_ptr<PhysicsSystem> physics_system_;
    std::unique_ptr<benchmarks::PhysicsBenchmarkRunner> benchmark_runner_;
    
    bool running_{true};
    bool paused_{false};
    f32 simulation_time_{0.0f};
    u32 frame_count_{0};
    
    // Demo state
    bool step_mode_{true};      // Start in educational step mode
    bool show_help_{true};
    u32 active_scenario_{0};
    
public:
    /** @brief Initialize the physics demo */
    bool initialize() {
        std::cout << "=== ECScope Physics Demo - Educational 2D Physics Engine ===\n";
        std::cout << "Phase 5: Física 2D - Complete Implementation\n\n";
        
        // Initialize random seed
        std::srand(static_cast<u32>(std::time(nullptr)));
        
        // Create ECS registry with educational configuration
        LOG_INFO("Creating ECS Registry with educational memory management...");
        auto allocator_config = ecs::AllocatorConfig::create_educational_focused();
        registry_ = std::make_unique<ecs::Registry>(allocator_config, "Physics_Demo_Registry");
        
        // Create educational physics system
        LOG_INFO("Creating Physics System with educational features...");
        physics_system_ = PhysicsFactory::create_educational_system(*registry_);
        
        // Create benchmark runner for performance analysis
        LOG_INFO("Initializing performance benchmark system...");
        auto benchmark_config = benchmarks::BenchmarkConfig::create_quick_test();
        benchmark_runner_ = std::make_unique<benchmarks::PhysicsBenchmarkRunner>(benchmark_config);
        
        // Setup initial scenario
        setup_scenario_1();
        
        // Enable educational features
        physics_system_->enable_step_mode(step_mode_);
        
        std::cout << "\n=== Initialization Complete ===\n";
        print_help();
        
        return true;
    }
    
    /** @brief Main simulation loop */
    void run() {
        const f32 target_fps = 60.0f;
        const f32 frame_time = 1.0f / target_fps;
        
        auto last_time = std::chrono::high_resolution_clock::now();
        f32 accumulator = 0.0f;
        
        std::cout << "\n=== Starting Physics Simulation ===\n";
        std::cout << "Target FPS: " << target_fps << "\n";
        std::cout << "Step Mode: " << (step_mode_ ? "ON (Press SPACE to step)" : "OFF") << "\n\n";
        
        while (running_) {
            auto current_time = std::chrono::high_resolution_clock::now();
            f32 delta_time = std::chrono::duration<f32>(current_time - last_time).count();
            last_time = current_time;
            
            // Clamp delta time to prevent spiral of death
            delta_time = std::min(delta_time, 0.25f);
            
            // Handle input (in real application this would be event-driven)
            handle_input();
            
            // Update physics
            if (!paused_) {
                update_physics(frame_time);
                simulation_time_ += frame_time;
                ++frame_count_;
                
                // Print periodic updates
                if (frame_count_ % 300 == 0) {  // Every 5 seconds at 60fps
                    print_simulation_status();
                }
            }
            
            // Sleep to maintain target framerate (in real app, this would be handled by the engine)
            std::this_thread::sleep_for(std::chrono::milliseconds(16));  // ~60 FPS
        }
        
        std::cout << "\n=== Physics Simulation Ended ===\n";
        print_final_statistics();
    }
    
    /** @brief Cleanup and shutdown */
    void shutdown() {
        LOG_INFO("Shutting down Physics Demo");
        
        physics_system_.reset();
        registry_.reset();
        benchmark_runner_.reset();
        
        std::cout << "\n=== Physics Demo Shutdown Complete ===\n";
    }

private:
    /** @brief Update physics simulation */
    void update_physics(f32 delta_time) {
        physics_system_->update(delta_time);
    }
    
    /** @brief Handle user input */
    void handle_input() {
        // In a real application, this would be proper input handling
        // For demo purposes, we'll simulate some key presses based on frame count
        
        static u32 last_input_frame = 0;
        
        // Auto-step in step mode for demonstration
        if (step_mode_ && (frame_count_ - last_input_frame) > 120) {  // Every 2 seconds
            physics_system_->request_step();
            last_input_frame = frame_count_;
            std::cout << "Auto-stepping physics simulation (frame " << frame_count_ << ")\n";
        }
        
        // Demonstrate input handling
        if (frame_count_ == 600) {  // 10 seconds in
            handle_key_input('f');  // Create falling box
        }
        
        if (frame_count_ == 900) {  // 15 seconds in
            handle_key_input('2');  // Switch to scenario 2
        }
        
        if (frame_count_ == 1800) {  // 30 seconds in
            handle_key_input('b');  // Run benchmark
        }
    }
    
    /** @brief Handle individual key input */
    void handle_key_input(char key) {
        switch (key) {
            case ' ':  // Space - single step
                if (step_mode_) {
                    physics_system_->request_step();
                    std::cout << "Physics step requested\n";
                }
                break;
                
            case 'p':  // Toggle pause
                paused_ = !paused_;
                std::cout << "Simulation " << (paused_ ? "PAUSED" : "RESUMED") << "\n";
                break;
                
            case 't':  // Toggle step mode
                step_mode_ = !step_mode_;
                physics_system_->enable_step_mode(step_mode_);
                std::cout << "Step mode " << (step_mode_ ? "ENABLED" : "DISABLED") << "\n";
                break;
                
            case 'r':  // Reset simulation
                reset_simulation();
                break;
                
            case 'f':  // Create falling box
                create_random_falling_box();
                break;
                
            case 'b':  // Run benchmark
                run_benchmark();
                break;
                
            case 's':  // Show statistics
                print_detailed_statistics();
                break;
                
            case 'h':  // Toggle help
                show_help_ = !show_help_;
                if (show_help_) print_help();
                break;
                
            case '1':  // Scenario 1: Basic falling objects
                setup_scenario_1();
                break;
                
            case '2':  // Scenario 2: Collision stress test
                setup_scenario_2();
                break;
                
            case '3':  // Scenario 3: Stacking demo
                setup_scenario_3();
                break;
                
            case 'q':  // Quit
                running_ = false;
                break;
        }
    }
    
    /** @brief Print help information */
    void print_help() {
        std::cout << "\n=== Physics Demo Controls ===\n";
        std::cout << "SPACE - Single step (when in step mode)\n";
        std::cout << "p     - Pause/Resume simulation\n";
        std::cout << "t     - Toggle step mode ON/OFF\n";
        std::cout << "r     - Reset simulation\n";
        std::cout << "f     - Create falling box\n";
        std::cout << "b     - Run performance benchmark\n";
        std::cout << "s     - Show detailed statistics\n";
        std::cout << "h     - Toggle this help\n";
        std::cout << "1     - Load scenario 1 (Basic falling objects)\n";
        std::cout << "2     - Load scenario 2 (Collision stress test)\n";
        std::cout << "3     - Load scenario 3 (Stacking demo)\n";
        std::cout << "q     - Quit demo\n";
        std::cout << "===============================\n\n";
    }
    
    /** @brief Setup scenario 1: Basic falling objects */
    void setup_scenario_1() {
        std::cout << "\n=== Loading Scenario 1: Basic Falling Objects ===\n";
        registry_->clear();
        physics_system_->reset();
        
        // Create ground
        utils::create_ground(*registry_, Vec2{0.0f, -50.0f}, Vec2{400.0f, 20.0f});
        
        // Create falling boxes
        for (int i = 0; i < 5; ++i) {
            f32 x = (i - 2) * 30.0f;
            utils::create_falling_box(*registry_, Vec2{x, 100.0f}, Vec2{10.0f, 10.0f});
        }
        
        // Create bouncing balls
        for (int i = 0; i < 3; ++i) {
            f32 x = (i - 1) * 40.0f;
            auto ball = utils::create_bouncing_ball(*registry_, Vec2{x, 150.0f}, 8.0f);
            
            // Add initial velocity
            auto* rb = registry_->get_component<RigidBody2D>(ball);
            if (rb) {
                rb->velocity = Vec2{(i - 1) * 15.0f, -10.0f};
            }
        }
        
        active_scenario_ = 1;
        std::cout << "Scenario 1 loaded with " << registry_->active_entities() << " entities\n";
    }
    
    /** @brief Setup scenario 2: Collision stress test */
    void setup_scenario_2() {
        std::cout << "\n=== Loading Scenario 2: Collision Stress Test ===\n";
        registry_->clear();
        physics_system_->reset();
        
        // Create ground
        utils::create_ground(*registry_, Vec2{0.0f, -50.0f}, Vec2{400.0f, 20.0f});
        
        // Create densely packed objects for collision testing
        const u32 object_count = 20;
        const f32 spacing = 15.0f;
        const u32 grid_size = static_cast<u32>(std::sqrt(object_count));
        
        for (u32 i = 0; i < object_count; ++i) {
            f32 x = (i % grid_size) * spacing - (grid_size * spacing * 0.5f);
            f32 y = (i / grid_size) * spacing + 50.0f;
            
            if (i % 2 == 0) {
                // Alternating boxes and circles
                utils::create_falling_box(*registry_, Vec2{x, y}, Vec2{8.0f, 8.0f}, 0.8f);
            } else {
                auto ball = utils::create_bouncing_ball(*registry_, Vec2{x, y}, 6.0f, 0.6f);
                
                // Add random initial velocity
                auto* rb = registry_->get_component<RigidBody2D>(ball);
                if (rb) {
                    rb->velocity = Vec2{static_cast<f32>((rand() % 40) - 20), 
                                       static_cast<f32>((rand() % 20) - 10)};
                }
            }
        }
        
        active_scenario_ = 2;
        std::cout << "Scenario 2 loaded with " << registry_->active_entities() << " entities\n";
        std::cout << "This scenario will stress test collision detection and resolution\n";
    }
    
    /** @brief Setup scenario 3: Stacking demo */
    void setup_scenario_3() {
        std::cout << "\n=== Loading Scenario 3: Stacking Simulation ===\n";
        registry_->clear();
        physics_system_->reset();
        
        // Create ground
        utils::create_ground(*registry_, Vec2{0.0f, -50.0f}, Vec2{400.0f, 20.0f});
        
        // Create tower of boxes
        const f32 box_size = 12.0f;
        const u32 tower_height = 8;
        
        for (u32 i = 0; i < tower_height; ++i) {
            f32 y = -50.0f + 20.0f + i * box_size;  // Start above ground
            utils::create_falling_box(*registry_, Vec2{0.0f, y}, Vec2{box_size, box_size}, 1.0f);
        }
        
        // Create some disrupting balls
        for (int i = 0; i < 2; ++i) {
            auto ball = utils::create_bouncing_ball(*registry_, Vec2{-100.0f + i * 200.0f, 100.0f}, 10.0f, 2.0f);
            
            // Give them velocity toward the tower
            auto* rb = registry_->get_component<RigidBody2D>(ball);
            if (rb) {
                rb->velocity = Vec2{(i == 0 ? 50.0f : -50.0f), 0.0f};
            }
        }
        
        active_scenario_ = 3;
        std::cout << "Scenario 3 loaded with " << registry_->active_entities() << " entities\n";
        std::cout << "This scenario demonstrates constraint solving and stability\n";
    }
    
    /** @brief Reset current simulation */
    void reset_simulation() {
        std::cout << "\n=== Resetting Simulation ===\n";
        
        switch (active_scenario_) {
            case 1: setup_scenario_1(); break;
            case 2: setup_scenario_2(); break;
            case 3: setup_scenario_3(); break;
            default: setup_scenario_1(); break;
        }
        
        simulation_time_ = 0.0f;
        frame_count_ = 0;
    }
    
    /** @brief Create a random falling box */
    void create_random_falling_box() {
        f32 x = static_cast<f32>((rand() % 200) - 100);
        f32 y = 200.0f;
        f32 size = 5.0f + static_cast<f32>(rand() % 10);
        
        utils::create_falling_box(*registry_, Vec2{x, y}, Vec2{size, size});
        std::cout << "Created falling box at (" << x << ", " << y << ") with size " << size << "\n";
    }
    
    /** @brief Run performance benchmark */
    void run_benchmark() {
        std::cout << "\n=== Running Performance Benchmark ===\n";
        std::cout << "This may take a few seconds...\n";
        
        if (benchmark_runner_->initialize()) {
            auto results = benchmark_runner_->run_all_benchmarks();
            
            std::cout << "\n=== Benchmark Results ===\n";
            std::cout << "Tests run: " << results.results.size() << "\n";
            std::cout << "Performance grade: " << results.analysis.overall_grade << "\n";
            
            if (!results.results.empty()) {
                const auto& best = results.analysis.best_performance;
                const auto& worst = results.analysis.worst_performance;
                
                std::cout << "Best performance: " << best.test_name 
                         << " (" << best.average_frame_time << "ms avg)\n";
                std::cout << "Worst performance: " << worst.test_name 
                         << " (" << worst.average_frame_time << "ms avg)\n";
            }
            
            // Show optimization recommendations
            if (!results.analysis.optimization_recommendations.empty()) {
                std::cout << "\nOptimization recommendations:\n";
                for (const auto& rec : results.analysis.optimization_recommendations) {
                    std::cout << "- " << rec << "\n";
                }
            }
            
            std::cout << "========================\n\n";
        } else {
            std::cout << "ERROR: Failed to initialize benchmark runner\n";
        }
    }
    
    /** @brief Print current simulation status */
    void print_simulation_status() {
        auto stats = physics_system_->get_system_statistics();
        
        std::cout << "[" << std::fixed << std::setprecision(1) << simulation_time_ 
                  << "s] Entities: " << stats.component_stats.total_rigid_bodies
                  << ", Performance: " << stats.performance_rating
                  << ", Avg Frame: " << std::setprecision(2) << stats.profile_data.average_update_time << "ms"
                  << ", Contacts: " << stats.world_stats.active_contacts << "\n";
    }
    
    /** @brief Print detailed statistics */
    void print_detailed_statistics() {
        std::cout << "\n" << physics_system_->generate_performance_report() << "\n";
        std::cout << registry_->generate_memory_report() << "\n";
    }
    
    /** @brief Print final statistics before shutdown */
    void print_final_statistics() {
        std::cout << "\n=== Final Statistics ===\n";
        std::cout << "Total simulation time: " << simulation_time_ << " seconds\n";
        std::cout << "Total frames: " << frame_count_ << "\n";
        std::cout << "Average FPS: " << (frame_count_ / simulation_time_) << "\n";
        
        print_detailed_statistics();
    }
};

/**
 * @brief Main function - Entry point for physics demo
 */
int main() {
    // Initialize logging
    ecscope::Log::initialize();
    
    std::cout << "ECScope Physics Demo Starting...\n\n";
    
    try {
        PhysicsDemo demo;
        
        if (demo.initialize()) {
            demo.run();
        } else {
            std::cerr << "Failed to initialize physics demo\n";
            return -1;
        }
        
        demo.shutdown();
        
    } catch (const std::exception& e) {
        std::cerr << "Physics Demo Error: " << e.what() << "\n";
        return -1;
    }
    
    std::cout << "\nPhysics Demo completed successfully!\n";
    return 0;
}